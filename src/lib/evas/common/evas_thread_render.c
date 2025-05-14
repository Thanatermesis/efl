/**
 * @file
 * @brief Evas threaded rendering module.
 *
 * This file implements the threading mechanism for Evas rendering operations.
 * It manages a worker thread that processes a queue of rendering commands,
 * allowing for asynchronous execution of these commands.
 */

#include "evas_common_private.h"

#include "Ecore.h"
#include <assert.h>

static Eina_Thread evas_thread_worker; /**< The worker thread instance. */
static Eina_Condition evas_thread_queue_condition; /**< Condition variable to signal command queue readiness. */
static Eina_Lock evas_thread_queue_lock; /**< Lock for accessing the command queue. */
static Eina_Bool evas_thread_queue_ready = EINA_FALSE; /**< Flag indicating if the command queue has new commands. */
static Eina_Inarray evas_thread_queue; /**< The main command queue. Elements are of type Evas_Thread_Command. */
static Evas_Thread_Command *evas_thread_queue_cache = NULL; /**< A cache for the command queue's internal buffer to reduce reallocations. */
static unsigned int evas_thread_queue_cache_max = 0; /**< The maximum size of the command queue cache. */

static Eina_Lock evas_thread_exited_lock; /**< Lock for accessing the evas_thread_exited flag. */
static int evas_thread_exited = 0; /**< Flag indicating if the worker thread has exited. */
static Eina_Bool exit_thread = EINA_FALSE; /**< Flag to signal the worker thread to exit. */
static int init_count = 0; /**< Reference counter for module initialization. */

#define SHUTDOWN_TIMEOUT_RESET (0)
#define SHUTDOWN_TIMEOUT_CHECK (1)
#define SHUTDOWN_TIMEOUT (3000)

struct fence_stuff {
   Eina_Lock lock;
   Eina_Condition cond; /**< Condition variable for the fence. */
};

/**
 * @brief Callback function used as a fence in the thread queue.
 *
 * This function is enqueued and, when processed by the worker thread,
 * signals a condition variable. This is used by evas_thread_queue_wait()
 * to block until all preceding commands in the queue have been processed.
 *
 * @param data A pointer to a struct fence_stuff containing the lock and condition.
 */
static void
_evas_thread_queue_fence(void *data)
{
   struct fence_stuff *f;

   f = data;

   eina_lock_take(&f->lock);
   eina_condition_signal(&f->cond);
   eina_lock_release(&f->lock);
}

/**
 * @brief Waits for all currently enqueued commands in the worker thread to complete.
 *
 * This function enqueues a special "fence" command and blocks until the worker
 * thread processes it. This ensures that all commands submitted before this
 * call have finished executing.
 */
EVAS_API void
evas_thread_queue_wait(void)
{
   struct fence_stuff f;

   /* No shortcuts here - if the thread queue looks empty it could just mean
    * it's being processed.  Need a round trip.
    */
   if (!eina_lock_new(&f.lock)) return;
   if (!eina_condition_new(&f.cond, &f.lock))
     {
        eina_lock_free(&f.lock);
        return;
     }

   eina_lock_take(&f.lock);
   evas_thread_queue_flush(_evas_thread_queue_fence, &f);
   eina_condition_wait(&f.cond);
   eina_lock_release(&f.lock);

   eina_lock_free(&f.lock);
   eina_condition_free(&f.cond);
}

/**
 * @brief Manages a timeout for shutting down the worker thread.
 *
 * This function can reset a timer or check if a timeout has occurred.
 *
 * @param time Pointer to a double storing the start time in milliseconds.
 *             This value is updated when mode is SHUTDOWN_TIMEOUT_RESET.
 * @param mode Operation mode:
 *             - SHUTDOWN_TIMEOUT_RESET: Resets the timer.
 *             - SHUTDOWN_TIMEOUT_CHECK: Checks if the timeout has expired.
 * @param timeout_ms The timeout duration in milliseconds.
 * @return EINA_TRUE if the timeout has occurred (only in SHUTDOWN_TIMEOUT_CHECK mode),
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_shutdown_timeout(double *time, int mode, int timeout_ms)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);

   if ( mode == SHUTDOWN_TIMEOUT_RESET )
     *time = (tv.tv_sec + tv.tv_usec / 1000000.0) * 1000.0;
   return ((tv.tv_sec + tv.tv_usec / 1000000.0) * 1000.0 - (*time)) > timeout_ms ;
}

/**
 * @brief Appends a command to the worker thread's queue.
 *
 * This is an internal helper function.
 *
 * @param cb The callback function to execute in the worker thread.
 * @param data The data to pass to the callback function.
 * @param do_flush If EINA_TRUE, signals the worker thread that new commands are ready.
 *                 If EINA_FALSE, the signal is deferred (e.g., for batching commands).
 */
static void
evas_thread_queue_append(Evas_Thread_Command_Cb cb, void *data, Eina_Bool do_flush)
{
   Evas_Thread_Command *cmd;

   eina_lock_take(&evas_thread_queue_lock);

   cmd = eina_inarray_grow(&evas_thread_queue, 1);
   if (cmd)
     {
        cmd->cb = cb;
        cmd->data = data;
     }
   else
     {
        ERR("Out of memory allocating thread command.");
        goto out;
     }

   if (do_flush)
     {
        evas_thread_queue_ready = EINA_TRUE;
        eina_condition_signal(&evas_thread_queue_condition);
     }

out:
   eina_lock_release(&evas_thread_queue_lock);
}

/**
 * @brief Enqueues a command to be executed by the worker thread.
 *
 * The command is added to the queue, but the worker thread is not
 * necessarily signaled immediately. This allows for batching multiple
 * commands before a flush.
 *
 * @param cb The callback function to execute.
 * @param data The data to pass to the callback function.
 */
EVAS_API void
evas_thread_cmd_enqueue(Evas_Thread_Command_Cb cb, void *data)
{
   evas_thread_queue_append(cb, data, EINA_FALSE);
}

/**
 * @brief Enqueues a command and flushes the queue, signaling the worker thread.
 *
 * This function ensures that the worker thread is awakened to process
 * the newly added command and any other pending commands.
 *
 * @param cb The callback function to execute.
 * @param data The data to pass to the callback function.
 */
EVAS_API void
evas_thread_queue_flush(Evas_Thread_Command_Cb cb, void *data)
{
   evas_thread_queue_append(cb, data, EINA_TRUE);
}

/**
 * @brief The main function for the Evas worker thread.
 *
 * This function runs in a loop, waiting for commands to be added to the
 * queue. When commands are available, it processes them sequentially.
 * It uses a double-buffering technique for the command queue (evas_thread_queue
 * and evas_thread_queue_cache) to minimize lock contention.
 *
 * @param data Unused.
 * @param thread Unused.
 * @return NULL when the thread exits.
 */
static void*
evas_thread_worker_func(void *data EINA_UNUSED, Eina_Thread thread EINA_UNUSED)
{
   eina_thread_name_set(eina_thread_self(), "Eevas-thread-wk");
   while (1)
     {
        Evas_Thread_Command *cmd;
        unsigned int len, max;

        eina_lock_take(&evas_thread_queue_lock);

        while (!evas_thread_queue_ready)
          {
             if (exit_thread)
               {
                  eina_lock_release(&evas_thread_queue_lock);
                  goto out;
               }
             eina_condition_timedwait(&evas_thread_queue_condition, SHUTDOWN_TIMEOUT * 0.75);
          }

        if (!eina_inarray_count(&evas_thread_queue))
          {
             ERR("Signaled to find an empty queue. BUG!");
             evas_thread_queue_ready = EINA_FALSE;
             eina_lock_release(&evas_thread_queue_lock);
             continue;
          }

        cmd = evas_thread_queue.members;
        evas_thread_queue.members = evas_thread_queue_cache;
        evas_thread_queue_cache = cmd;

        max = evas_thread_queue.max;
        evas_thread_queue.max = evas_thread_queue_cache_max;
        evas_thread_queue_cache_max = max;

        len = evas_thread_queue.len;
        evas_thread_queue.len = 0;

        evas_thread_queue_ready = EINA_FALSE;

        eina_lock_release(&evas_thread_queue_lock);

        DBG("Evas render thread command queue length: %u", len);

        eina_evlog("+thread", NULL, 0.0, NULL);
        while (len)
          {
             assert(cmd->cb);

             eina_evlog("+thread_do", cmd->data, 0.0, NULL);
             cmd->cb(cmd->data);
             eina_evlog("-thread_do", cmd->data, 0.0, NULL);

             cmd++;
             len--;
          }
        eina_evlog("-thread", NULL, 0.0, NULL);
     }

out:
   eina_lock_take(&evas_thread_exited_lock);
   evas_thread_exited = 1;
   eina_lock_release(&evas_thread_exited_lock);
   return NULL;
}

/**
 * @brief Callback function to re-initialize thread resources after a fork.
 *
 * This function is registered with Ecore's fork reset mechanism. If the
 * process forks, this function is called in the child process to recreate
 * locks, condition variables, and the worker thread itself, as these
 * resources are not typically inherited correctly across a fork.
 *
 * @param data Unused.
 */
static void
evas_thread_fork_reset(void *data EINA_UNUSED)
{
   if (!eina_lock_new(&evas_thread_exited_lock))
     {
        CRI("Could not create exit thread lock (%m)");
        goto on_error;
     }
   if (!eina_lock_new(&evas_thread_queue_lock))
     {
        CRI("Could not create draw thread lock (%m)");
        goto on_error;
     }
   if (!eina_condition_new(&evas_thread_queue_condition, &evas_thread_queue_lock))
     {
        CRI("Could not create draw thread condition (%m)");
        goto on_error;
     }

   if (!eina_thread_create(&evas_thread_worker, EINA_THREAD_NORMAL, -1,
                           evas_thread_worker_func, NULL))
     {
        CRI("Could not recreate draw thread.");
        goto on_error;
     }

   return ;

 on_error:
   eina_lock_free(&evas_thread_exited_lock);
   eina_lock_free(&evas_thread_queue_lock);
   eina_condition_free(&evas_thread_queue_condition);

   evas_thread_worker = 0;

   free(evas_thread_queue_cache);
   evas_thread_queue_cache = NULL;
   evas_thread_queue_cache_max = 0;
   eina_inarray_flush(&evas_thread_queue);

   eina_threads_shutdown();
}

/**
 * @brief Initializes the Evas threaded rendering module.
 *
 * This function sets up the necessary synchronization primitives (locks,
 * condition variables), initializes the command queue, and creates the
 * worker thread. It uses a reference counter (init_count) to allow
 * multiple initializations and shutdowns.
 *
 * @return The current initialization count (1 on first successful init, >1 on subsequent),
 *         or 0 on failure.
 */
int
evas_thread_init(void)
{
   if (init_count++)
     return init_count;

   exit_thread = EINA_FALSE;
   evas_thread_exited = 0;

   if(!eina_threads_init())
     {
        CRI("Could not init eina threads");
        goto fail_on_eina_thread_init;
     }

   eina_inarray_step_set(&evas_thread_queue, sizeof (Eina_Inarray), sizeof (Evas_Thread_Command), 128);

   if (!eina_lock_new(&evas_thread_exited_lock))
     {
        CRI("Could not create exit thread lock (%m)");
        goto fail_on_lock_creation;
     }

   if (!eina_lock_new(&evas_thread_queue_lock))
     {
        CRI("Could not create draw thread lock (%m)");
        goto fail_on_lock_creation;
     }
   if (!eina_condition_new(&evas_thread_queue_condition, &evas_thread_queue_lock))
     {
        CRI("Could not create draw thread condition (%m)");
        goto fail_on_cond_creation;
     }

   if (!eina_thread_create(&evas_thread_worker, EINA_THREAD_NORMAL, -1,
                           evas_thread_worker_func, NULL))
     {
        CRI("Could not create draw thread (%m)");
        goto fail_on_thread_creation;
     }

   ecore_fork_reset_callback_add(evas_thread_fork_reset, NULL);

   return init_count;

fail_on_thread_creation:
   evas_thread_worker = 0;
   eina_condition_free(&evas_thread_queue_condition);
fail_on_cond_creation:
   eina_lock_free(&evas_thread_exited_lock);
   eina_lock_free(&evas_thread_queue_lock);
fail_on_lock_creation:
   eina_threads_shutdown();
fail_on_eina_thread_init:
   exit_thread = EINA_TRUE;
   evas_thread_exited = 1;
   return --init_count;
}

/**
 * @brief Shuts down the Evas threaded rendering module.
 *
 * This function signals the worker thread to exit, waits for it to terminate
 * (with a timeout), and then cleans up all allocated resources including
 * locks, condition variables, and the command queue. It uses a reference
 * counter (init_count) and only performs a full shutdown when the count
 * reaches zero.
 *
 * @return The current initialization count (0 if fully shut down), or the
 *         previous count if not fully shut down yet. Returns 0 if called
 *         when already fully shut down (init_count <= 0).
 */
int
evas_thread_shutdown(void)
{
   double to = 0 ;

   if (init_count <= 0)
     {
        ERR("Too many calls to shutdown, ignored.");
        return 0;
     }

   if (--init_count)
     return init_count;

   if (!evas_thread_worker)
     return init_count;

   eina_lock_take(&evas_thread_queue_lock);

   ecore_fork_reset_callback_del(evas_thread_fork_reset, NULL);

   exit_thread = EINA_TRUE;
   eina_condition_signal(&evas_thread_queue_condition);

   eina_lock_release(&evas_thread_queue_lock);

   _shutdown_timeout(&to, SHUTDOWN_TIMEOUT_RESET, SHUTDOWN_TIMEOUT);
   while (1)
     {
        int exited;
        eina_lock_take(&evas_thread_exited_lock);
        exited = evas_thread_exited;
        eina_lock_release(&evas_thread_exited_lock);
        if (exited || (evas_async_events_process() == -1)) break;
        if(_shutdown_timeout(&to, SHUTDOWN_TIMEOUT_CHECK, SHUTDOWN_TIMEOUT))
          {
             CRI("Timeout shutdown thread. Skipping thread_join. Some resources could be leaked");
             goto timeout_shutdown;
          }
     }

   eina_thread_join(evas_thread_worker);
timeout_shutdown:
   eina_lock_free(&evas_thread_exited_lock);
   eina_lock_free(&evas_thread_queue_lock);
   eina_condition_free(&evas_thread_queue_condition);

   evas_thread_worker = 0;

   free(evas_thread_queue_cache);
   evas_thread_queue_cache = NULL;
   evas_thread_queue_cache_max = 0;
   eina_inarray_flush(&evas_thread_queue);

   eina_threads_shutdown();

   return 0;
}
