#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>

#include "Ecore.h"
#include "ecore_private.h"

# define LK(x)        Eina_Lock x
# define LKI(x)       eina_lock_new(&(x))
# define LKD(x)       eina_lock_free(&(x))
# define LKL(x)       eina_lock_take(&(x))
# define LKU(x)       eina_lock_release(&(x))

# define SLK(x)       Eina_Spinlock x
# define SLKI(x)      eina_spinlock_new(&(x))
# define SLKD(x)      eina_spinlock_free(&(x))
# define SLKL(x)      eina_spinlock_take(&(x))
# define SLKU(x)      eina_spinlock_release(&(x))

# define CD(x)        Eina_Condition x
# define CDI(x, m)    eina_condition_new(&(x), &(m))
# define CDD(x)       eina_condition_free(&(x))
# define CDB(x)       eina_condition_broadcast(&(x))
# define CDW(x, t)    eina_condition_timedwait(&(x), t)

# define LRWK(x)      Eina_RWLock x
# define LRWKI(x)     eina_rwlock_new(&(x));
# define LRWKD(x)     eina_rwlock_free(&(x));
# define LRWKWL(x)    eina_rwlock_take_write(&(x));
# define LRWKRL(x)    eina_rwlock_take_read(&(x));
# define LRWKU(x)     eina_rwlock_release(&(x));

# define PH(x)        Eina_Thread x
# define PHE(x, y)    eina_thread_equal(x, y)
# define PHS()        eina_thread_self()
# define PHC(x, f, d) eina_thread_create(&(x), EINA_THREAD_BACKGROUND, -1, (void *)f, d)
# define PHC2(x, f, d)eina_thread_create(&(x), EINA_THREAD_URGENT, -1, (void *)f, d)
# define PHJ(x)       eina_thread_join(x)

typedef struct _Ecore_Pthread_Worker Ecore_Pthread_Worker;
typedef struct _Ecore_Pthread        Ecore_Pthread;
typedef struct _Ecore_Thread_Data    Ecore_Thread_Data;
typedef struct _Ecore_Thread_Waiter  Ecore_Thread_Waiter;

/**
 * @brief Structure to manage waiting for a thread's completion.
 * Used by ecore_thread_wait() to temporarily replace and then restore
 * the original end and cancel callbacks of a thread.
 */
struct _Ecore_Thread_Waiter
{
   Ecore_Thread_Cb func_cancel; /**< Original cancel callback of the thread. */
   Ecore_Thread_Cb func_end;    /**< Original end callback of the thread. */
   Eina_Bool       waiting;     /**< Flag indicating if ecore_thread_wait() is still waiting. */
};

/**
 * @brief Structure to hold thread-local or global data with an associated free callback.
 */
struct _Ecore_Thread_Data
{
   void        *data; /**< Pointer to the stored data. */
   Eina_Free_Cb cb;   /**< Callback function to free the data when it's no longer needed. */
};

/**
 * @brief Core structure representing a worker thread and its associated task.
 * This structure holds all information related to a thread managed by Ecore,
 * including its callback functions, data, state, and synchronization primitives.
 */
struct _Ecore_Pthread_Worker
{
   union
   {
      struct
      {
         Ecore_Thread_Cb func_blocking;
      } short_run;
      struct
      {
         Ecore_Thread_Cb        func_heavy;
         Ecore_Thread_Notify_Cb func_notify;

         Ecore_Pthread_Worker  *direct_worker;

         int                    send;
         int                    received;
      } feedback_run;
      struct
      {
         Ecore_Thread_Cb        func_main;
         Ecore_Thread_Notify_Cb func_notify;

         Ecore_Pipe            *send;
         Ecore_Pthread_Worker  *direct_worker;

         struct
         {
            int send;
            int received;
         } from, to;
      } message_run;
   } u; /**< Union holding type-specific function pointers and data for different thread modes. */

   Ecore_Thread_Waiter *waiter;     /**< Pointer to a waiter structure if ecore_thread_wait() is active for this thread. */
   Ecore_Thread_Cb      func_cancel; /**< Callback function executed when the thread is cancelled. */
   Ecore_Thread_Cb      func_end;    /**< Callback function executed when the thread finishes normally. */
   PH(self);                        /**< The actual thread identifier (pthread_t or similar). */
   Eina_Hash           *hash;        /**< Hash table for thread-local data. */
   CD(cond);                        /**< Condition variable for thread synchronization. */
   LK(mutex);                       /**< Mutex for protecting shared access to this worker structure. */

   const void          *data;       /**< User-provided data passed to the thread functions. */

   int                  cancel;      /**< Flag indicating if a cancellation request is pending. Protected by cancel_mutex. */

   SLK(cancel_mutex);               /**< Spinlock for protecting the cancel flag. */

   Eina_Bool            message_run : 1;  /**< True if this is a message-passing thread. */
   Eina_Bool            feedback_run : 1; /**< True if this is a feedback-style thread. */
   Eina_Bool            kill : 1;         /**< True if the thread is marked to be killed (e.g., due to pending notifications on cancel). */
   Eina_Bool            reschedule : 1;   /**< True if the thread should be rescheduled after completion. */
   Eina_Bool            no_queue : 1;     /**< True if the thread was run directly without queuing (try_no_queue). */
};

/**
 * @brief Structure used for passing notification data from a worker thread to the main loop.
 */
typedef struct _Ecore_Pthread_Notify Ecore_Pthread_Notify;
struct _Ecore_Pthread_Notify
{
   Ecore_Pthread_Worker *work;       /**< The worker thread that sent the notification. */
   const void           *user_data;  /**< Data associated with the notification. */
};

/**
 * @brief Function pointer type for synchronous callback messages.
 * @param data User data passed to the callback.
 * @param thread The Ecore_Thread context.
 * @return Data to be sent back to the calling thread.
 */
typedef void                       *(*Ecore_Thread_Sync_Cb)(void *data, Ecore_Thread *thread);

/**
 * @brief Structure representing a message for inter-thread communication.
 * Used in message-passing threads.
 */
typedef struct _Ecore_Pthread_Message Ecore_Pthread_Message;
struct _Ecore_Pthread_Message
{
   union
   {
      Ecore_Thread_Cb      async; /**< Asynchronous callback function. */
      Ecore_Thread_Sync_Cb sync;  /**< Synchronous callback function. */
   } u;

   const void *data; /**< Data payload of the message. */

   int         code; /**< Status code, e.g., INT_MAX for synchronous reply. */

   Eina_Bool   callback : 1; /**< True if this message involves a callback. */
   Eina_Bool   sync : 1;     /**< True if this is a synchronous message. */
};

static int _ecore_thread_count_max = 0; /**< Maximum number of concurrent threads allowed in the thread pool. */

static void _ecore_thread_handler(void *data);

static int _ecore_thread_count = 0; /**< Current number of active pooled worker threads. */
static int _ecore_thread_count_no_queue = 0; /**< Current number of active 'direct' (no_queue) worker threads. */

static Eina_List *_ecore_running_job = NULL; /**< List of Ecore_Pthread_Worker currently executing jobs. */
static Eina_List *_ecore_pending_job_threads = NULL; /**< List of pending short-running Ecore_Pthread_Worker jobs. */
static Eina_List *_ecore_pending_job_threads_feedback = NULL; /**< List of pending feedback-style Ecore_Pthread_Worker jobs. */
static SLK(_ecore_pending_job_threads_mutex); /**< Spinlock protecting access to pending job lists. */
static SLK(_ecore_running_job_mutex); /**< Spinlock protecting access to the running job list. */

static Eina_Hash *_ecore_thread_global_hash = NULL; /**< Hash table for global data accessible by all threads. */
static LRWK(_ecore_thread_global_hash_lock); /**< Read-write lock for the global hash table. */
static LK(_ecore_thread_global_hash_mutex); /**< Mutex for the global hash condition variable. */
static CD(_ecore_thread_global_hash_cond); /**< Condition variable for ecore_thread_global_data_wait(). */

static Eina_Bool have_main_loop_thread = 0; /**< Flag indicating if the main loop thread ID has been captured. */

static Eina_Trash *_ecore_thread_worker_trash = NULL; /**< Trash stack for recycling Ecore_Pthread_Worker structures. */
static int _ecore_thread_worker_count = 0; /**< Total number of Ecore_Pthread_Worker structures allocated (live + trashed). */

static void                 *_ecore_thread_worker(void *, Eina_Thread);
static Ecore_Pthread_Worker *_ecore_thread_worker_new(void);

/**
 * @brief Gets the thread identifier of the main loop.
 * Caches the main loop thread ID on first call or if process ID changes.
 * @return The thread identifier (PH(type)) of the main loop.
 */
static PH(get_main_loop_thread) (void)
{
   static PH(main_loop_thread);
   static pid_t main_loop_pid;
   pid_t pid = getpid();

   if (pid != main_loop_pid)
     {
        main_loop_pid = pid;
        main_loop_thread = PHS();
        have_main_loop_thread = 1;
     }

   return main_loop_thread;
}

/**
 * @brief Frees or recycles an Ecore_Pthread_Worker structure.
 * If the total number of worker structures exceeds a threshold,
 * the worker is freed. Otherwise, it's pushed onto a trash stack
 * for later reuse by _ecore_thread_worker_new().
 * @param worker The worker structure to free or recycle.
 */
static void
_ecore_thread_worker_free(Ecore_Pthread_Worker *worker)
{
   SLKD(worker->cancel_mutex);
   CDD(worker->cond);
   LKD(worker->mutex);

   if (_ecore_thread_worker_count > ((_ecore_thread_count_max + 1) * 16))
     {
        _ecore_thread_worker_count--;
        free(worker);
        return;
     }

   eina_trash_push(&_ecore_thread_worker_trash, worker);
}

/**
 * @brief Frees an Ecore_Thread_Data structure.
 * If a callback `cb` is set in the Ecore_Thread_Data, it is called
 * with `d->data` before freeing the structure itself.
 * This function is typically used as an Eina_Free_Cb for hash tables.
 * @param data Pointer to the Ecore_Thread_Data to be freed.
 */
static void
_ecore_thread_data_free(void *data)
{
   Ecore_Thread_Data *d = data;

   if (d->cb) d->cb(d->data);
   free(d);
}

/**
 * @brief Joins a finished Ecore thread.
 * This function is typically called from the main loop via
 * ecore_main_loop_thread_safe_call_async to ensure thread cleanup.
 * @param data The thread identifier (PH(type)) cast to void*.
 */
void
_ecore_thread_join(void *data)
{
   PH(thread) = (uintptr_t)data;
   DBG("joining thread=%" PRIu64, (uint64_t)thread);
   PHJ(thread);
}

/**
 * @brief Finalizes a worker thread after its execution.
 * Calls the appropriate end or cancel callback, frees associated resources
 * (like direct_worker for feedback_run, or hash table), and then
 * frees/recycles the worker structure itself.
 * @param work The Ecore_Pthread_Worker to finalize.
 */
static void
_ecore_thread_kill(Ecore_Pthread_Worker *work)
{
   if (work->cancel)
     {
        if (work->func_cancel)
          work->func_cancel((void *)work->data, (Ecore_Thread *)work);
     }
   else
     {
        if (work->func_end)
          work->func_end((void *)work->data, (Ecore_Thread *)work);
     }

   if (work->feedback_run)
     {
        if (work->u.feedback_run.direct_worker)
          _ecore_thread_worker_free(work->u.feedback_run.direct_worker);
     }
   if (work->hash)
     eina_hash_free(work->hash);
   _ecore_thread_worker_free(work);
}

/**
 * @brief Main loop handler for completed/cancelled threads.
 * This function is called asynchronously in the main loop thread
 * to safely clean up a worker thread.
 * For feedback_run threads, it checks if all sent notifications have been
 * received before killing the thread. If not, it sets the kill flag
 * and returns, allowing pending notifications to be processed.
 * @param data Pointer to the Ecore_Pthread_Worker that has finished.
 */
static void
_ecore_thread_handler(void *data)
{
   Ecore_Pthread_Worker *work = data;

   if (work->feedback_run)
     {
        // If there are outstanding feedback messages, mark for kill
        // and wait for them to be processed by _ecore_notify_handler.
        if (work->u.feedback_run.send != work->u.feedback_run.received)
          {
             work->kill = EINA_TRUE;
             return;
          }
     }

   _ecore_thread_kill(work);
}

#if 0
static void
_ecore_nothing_handler(void *data EINA_UNUSED, void *buffer EINA_UNUSED, unsigned int nbyte EINA_UNUSED)
{
}

#endif

/**
 * @brief Main loop handler for feedback notifications from worker threads.
 * This function is called asynchronously in the main loop thread.
 * It increments the received count for the worker, calls the user's
 * notification callback (func_notify), and if the thread is marked for
 * killing and all notifications are received, it finalizes the thread.
 * @param data Pointer to an Ecore_Pthread_Notify structure.
 */
static void
_ecore_notify_handler(void *data)
{
   Ecore_Pthread_Notify *notify = data;
   Ecore_Pthread_Worker *work = notify->work;
   void *user_data = (void *)notify->user_data;

   work->u.feedback_run.received++;

   if (work->u.feedback_run.func_notify)
     work->u.feedback_run.func_notify((void *)work->data, (Ecore_Thread *)work, user_data);

   /* Force reading all notify event before killing the thread */
   if (work->kill && work->u.feedback_run.send == work->u.feedback_run.received)
     {
        _ecore_thread_kill(work);
     }

   free(notify);
}

/**
 * @brief Main loop handler for messages from message-passing worker threads.
 * This function is called asynchronously in the main loop thread.
 * It processes incoming messages, which can be simple data notifications
 * or requests to execute a callback (either synchronously or asynchronously)
 * in the main thread's context.
 * @param data Pointer to an Ecore_Pthread_Notify structure, where
 *             `notify->user_data` is an Ecore_Pthread_Message.
 */
static void
_ecore_message_notify_handler(void *data)
{
   Ecore_Pthread_Notify *notify = data;
   Ecore_Pthread_Worker *work = notify->work;
   Ecore_Pthread_Message *user_data = (void *)notify->user_data;
   Eina_Bool delete = EINA_TRUE;

   work->u.message_run.from.received++;

   if (!user_data->callback)
     {
        if (work->u.message_run.func_notify)
          work->u.message_run.func_notify((void *)work->data, (Ecore_Thread *)work, (void *)user_data->data);
     }
   else
     {
        if (user_data->sync)
          {
             user_data->data = user_data->u.sync((void *)user_data->data, (Ecore_Thread *)work);
             user_data->callback = EINA_FALSE;
             user_data->code = INT_MAX;
             ecore_pipe_write(work->u.message_run.send, &user_data, sizeof (Ecore_Pthread_Message *));

             delete = EINA_FALSE;
          }
        else
          {
             user_data->u.async((void *)user_data->data, (Ecore_Thread *)work);
          }
     }

   if (delete)
     {
        free(user_data);
     }

   /* Force reading all notify event before killing the thread */
   if (work->kill && work->u.message_run.from.send == work->u.message_run.from.received)
     {
        _ecore_thread_kill(work);
     }
   free(notify);
}

/**
 * @brief Cleanup handler for short-running jobs.
 * This function is registered with EINA_THREAD_CLEANUP_PUSH and is
 * called when a short job thread exits (normally or due to cancellation).
 * It removes the job from the running list. If `reschedule` is true,
 * it re-adds the job to the pending list; otherwise, it schedules
 * `_ecore_thread_handler` to finalize the job in the main loop.
 * @param data Pointer to the Ecore_Pthread_Worker of the completed job.
 */
static void
_ecore_short_job_cleanup(void *data)
{
   Ecore_Pthread_Worker *work = data;

   DBG("cleanup work=%p, thread=%" PRIu64, work, (uint64_t)work->self);

   SLKL(_ecore_running_job_mutex);
   _ecore_running_job = eina_list_remove(_ecore_running_job, work);
   SLKU(_ecore_running_job_mutex);

   if (work->reschedule)
     {
        work->reschedule = EINA_FALSE;

        SLKL(_ecore_pending_job_threads_mutex);
        _ecore_pending_job_threads = eina_list_append(_ecore_pending_job_threads, work);
        SLKU(_ecore_pending_job_threads_mutex);
     }
   else
     {
        ecore_main_loop_thread_safe_call_async(_ecore_thread_handler, work);
     }
}

static void
_ecore_short_job(PH(thread))
{
   Ecore_Pthread_Worker *work;
   int cancel;

   SLKL(_ecore_pending_job_threads_mutex);

   // If no pending short jobs, return.
   if (!_ecore_pending_job_threads)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        return;
     }

   // Dequeue the next short job.
   work = eina_list_data_get(_ecore_pending_job_threads);
   _ecore_pending_job_threads = eina_list_remove_list(_ecore_pending_job_threads,
                                                      _ecore_pending_job_threads);
   SLKU(_ecore_pending_job_threads_mutex);

   // Add to running jobs list.
   SLKL(_ecore_running_job_mutex);
   _ecore_running_job = eina_list_append(_ecore_running_job, work);
   SLKU(_ecore_running_job_mutex);

   // Check for cancellation before starting.
   SLKL(work->cancel_mutex);
   cancel = work->cancel;
   SLKU(work->cancel_mutex);
   work->self = thread; // Store the actual thread ID.

   EINA_THREAD_CLEANUP_PUSH(_ecore_short_job_cleanup, work);
   if (!cancel)
     work->u.short_run.func_blocking((void *)work->data, (Ecore_Thread *)work);
   eina_thread_cancellable_set(EINA_FALSE, NULL); // Disable cancellability after user code.
   EINA_THREAD_CLEANUP_POP(EINA_TRUE); // Execute cleanup.
}

/**
 * @brief Cleanup handler for feedback-style jobs.
 * Similar to _ecore_short_job_cleanup, but for feedback jobs.
 * It removes the job from the running list. If `reschedule` is true,
 * it re-adds the job to the feedback pending list; otherwise, it schedules
 * `_ecore_thread_handler` to finalize the job in the main loop.
 * @param data Pointer to the Ecore_Pthread_Worker of the completed job.
 */
static void
_ecore_feedback_job_cleanup(void *data)
{
   Ecore_Pthread_Worker *work = data;

   DBG("cleanup work=%p, thread=%" PRIu64, work, (uint64_t)work->self);

   SLKL(_ecore_running_job_mutex);
   _ecore_running_job = eina_list_remove(_ecore_running_job, work);
   SLKU(_ecore_running_job_mutex);

   if (work->reschedule)
     {
        work->reschedule = EINA_FALSE;

        SLKL(_ecore_pending_job_threads_mutex);
        _ecore_pending_job_threads_feedback = eina_list_append(_ecore_pending_job_threads_feedback, work);
        SLKU(_ecore_pending_job_threads_mutex);
     }
   else
     {
        ecore_main_loop_thread_safe_call_async(_ecore_thread_handler, work);
     }
}

static void
_ecore_feedback_job(PH(thread))
{
   Ecore_Pthread_Worker *work;
   int cancel;

   SLKL(_ecore_pending_job_threads_mutex);

   // If no pending feedback jobs, return.
   if (!_ecore_pending_job_threads_feedback)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        return;
     }

   // Dequeue the next feedback job.
   work = eina_list_data_get(_ecore_pending_job_threads_feedback);
   _ecore_pending_job_threads_feedback = eina_list_remove_list(_ecore_pending_job_threads_feedback,
                                                               _ecore_pending_job_threads_feedback);
   SLKU(_ecore_pending_job_threads_mutex);

   // Add to running jobs list.
   SLKL(_ecore_running_job_mutex);
   _ecore_running_job = eina_list_append(_ecore_running_job, work);
   SLKU(_ecore_running_job_mutex);

   // Check for cancellation before starting.
   SLKL(work->cancel_mutex);
   cancel = work->cancel;
   SLKU(work->cancel_mutex);
   work->self = thread; // Store the actual thread ID.

   EINA_THREAD_CLEANUP_PUSH(_ecore_feedback_job_cleanup, work);
   if (!cancel)
     work->u.feedback_run.func_heavy((void *)work->data, (Ecore_Thread *)work);
   eina_thread_cancellable_set(EINA_FALSE, NULL); // Disable cancellability after user code.
   EINA_THREAD_CLEANUP_POP(EINA_TRUE); // Execute cleanup.
}

/**
 * @brief Cleanup handler for "direct" (no_queue) worker threads.
 * This is called when a thread started with `try_no_queue` (for feedback or message run) exits.
 * It decrements the `_ecore_thread_count_no_queue`, schedules `_ecore_thread_handler`
 * for final cleanup in the main loop, and schedules `_ecore_thread_join` to join the
 * thread itself.
 * @param data Pointer to the Ecore_Pthread_Worker of the completed direct job.
 */
static void
_ecore_direct_worker_cleanup(void *data)
{
   Ecore_Pthread_Worker *work = data;

   DBG("cleanup work=%p, thread=%" PRIu64 " (should join)", work, (uint64_t)work->self);

   SLKL(_ecore_pending_job_threads_mutex);
   _ecore_thread_count_no_queue--;
   ecore_main_loop_thread_safe_call_async(_ecore_thread_handler, work);

   ecore_main_loop_thread_safe_call_async((Ecore_Cb)_ecore_thread_join,
                                          (void *)(intptr_t)PHS());
   SLKU(_ecore_pending_job_threads_mutex);
}

static void *
_ecore_direct_worker(void *data, Eina_Thread t EINA_UNUSED)
{
   Ecore_Pthread_Worker *work = data;
   eina_thread_cancellable_set(EINA_FALSE, NULL); // Not cancellable by default.
   eina_thread_name_set(eina_thread_self(), "Ethread-feedback"); // Set a descriptive name.
   work->self = PHS(); // Store the actual thread ID.

   EINA_THREAD_CLEANUP_PUSH(_ecore_direct_worker_cleanup, work);
   if (work->message_run)
     work->u.message_run.func_main((void *)work->data, (Ecore_Thread *)work);
   else // feedback_run
     work->u.feedback_run.func_heavy((void *)work->data, (Ecore_Thread *)work);
   eina_thread_cancellable_set(EINA_FALSE, NULL); // Disable cancellability after user code.
   EINA_THREAD_CLEANUP_POP(EINA_TRUE); // Execute cleanup.

   return NULL;
}

/**
 * @brief Cleanup handler for general pooled worker threads.
 * This is called when a pooled worker thread (one that processes jobs from
 * `_ecore_pending_job_threads` or `_ecore_pending_job_threads_feedback`) exits.
 * It decrements `_ecore_thread_count` and schedules `_ecore_thread_join`
 * to join the thread itself from the main loop.
 * @param data Unused.
 */
static void
_ecore_thread_worker_cleanup(void *data EINA_UNUSED)
{
   DBG("cleanup thread=%" PRIuPTR " (should join)", PHS());
   SLKL(_ecore_pending_job_threads_mutex);
   _ecore_thread_count--;
   ecore_main_loop_thread_safe_call_async((Ecore_Cb)_ecore_thread_join,
                                          (void *)(intptr_t)PHS());
   SLKU(_ecore_pending_job_threads_mutex);
}

static void *
_ecore_thread_worker(void *data EINA_UNUSED, Eina_Thread t EINA_UNUSED)
{
   eina_thread_cancellable_set(EINA_FALSE, NULL);
   EINA_THREAD_CLEANUP_PUSH(_ecore_thread_worker_cleanup, NULL);
restart: // Loop to process multiple jobs if available.

   /* These functions (_ecore_short_job, _ecore_feedback_job) handle one job each.
    * They contain cancellation points if the user's callback enables them.
    * The thread itself is generally not cancellable outside these job executions.
    */
   _ecore_short_job(PHS());
   _ecore_feedback_job(PHS());

   /* From here on, cancellations are guaranteed to be disabled for this iteration. */

   // FIXME: The comment below seems outdated or refers to a potential future optimization.
   // The current logic processes one short job then one feedback job if available,
   // then checks again.
   /* FIXME: Check if there is feedback running task todo, and switch to feedback run handler. */
   eina_thread_name_set(eina_thread_self(), "Ethread-worker"); // Set a generic name.

   // Check if more jobs were added while processing.
   SLKL(_ecore_pending_job_threads_mutex);
   if (_ecore_pending_job_threads || _ecore_pending_job_threads_feedback)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        goto restart; // More work to do, loop back.
     }
   SLKU(_ecore_pending_job_threads_mutex);

   /* Sleep a little to prevent premature death of the worker thread.
    * This gives a small window for new jobs to be added before the thread exits.
    */
#ifdef _WIN32
   Sleep(1); /* Approx 1ms on Windows, but granularity can be ~15ms. */
#else
   usleep(50); /* 50 microseconds. */
#endif

   // Final check for new jobs after the sleep.
   SLKL(_ecore_pending_job_threads_mutex);
   if (_ecore_pending_job_threads || _ecore_pending_job_threads_feedback)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        goto restart;
     }
   SLKU(_ecore_pending_job_threads_mutex);

   EINA_THREAD_CLEANUP_POP(EINA_TRUE); // Execute cleanup.

   return NULL; // Thread exits.
}

/**
 * @brief Allocates and initializes a new Ecore_Pthread_Worker structure.
 * Tries to pop a worker from the `_ecore_thread_worker_trash` first.
 * If the trash is empty, allocates a new one. Initializes locks and
 * condition variables.
 * @return A pointer to the initialized Ecore_Pthread_Worker, or NULL on failure.
 */
static Ecore_Pthread_Worker *
_ecore_thread_worker_new(void)
{
   Ecore_Pthread_Worker *result;

   result = eina_trash_pop(&_ecore_thread_worker_trash);

   if (!result)
     {
        result = calloc(1, sizeof(Ecore_Pthread_Worker));
        _ecore_thread_worker_count++;
     }
   else
     {
        memset(result, 0, sizeof(Ecore_Pthread_Worker));
     }

   SLKI(result->cancel_mutex);
   LKI(result->mutex);
   CDI(result->cond, result->mutex);

   return result;
}

/**
 * @internal
 * @brief Initializes the Ecore thread system.
 * Sets the default maximum thread count based on CPU cores.
 * Initializes global mutexes, spinlocks, read-write locks, and condition variables
 * used by the Ecore thread management.
 * This function is called by ecore_init().
 */
void
_ecore_thread_init(void)
{
   // Default max threads: 4 per CPU core, minimum 1.
   _ecore_thread_count_max = eina_cpu_count() * 4;
   if (_ecore_thread_count_max <= 0)
     _ecore_thread_count_max = 1;

   SLKI(_ecore_pending_job_threads_mutex);
   LRWKI(_ecore_thread_global_hash_lock);
   LKI(_ecore_thread_global_hash_mutex);
   SLKI(_ecore_running_job_mutex);
   CDI(_ecore_thread_global_hash_cond, _ecore_thread_global_hash_mutex);
}

/**
 * @internal
 * @brief Shuts down the Ecore thread system.
 * Attempts to cancel all pending and running jobs.
 * Waits for a short period for threads to terminate.
 * Frees all allocated resources including worker structures and global hash.
 * This function is called by ecore_shutdown().
 * @note The FIXME comment indicates a concern about forcefully killing threads
 *       that might still be running. The current implementation cancels them
 *       and waits, but doesn't guarantee immediate termination if threads
 *       are non-cooperative or stuck.
 */
void
_ecore_thread_shutdown(void)
{
   /* FIXME: If function are still running in the background, should we kill them ? */
   // Current behavior: cancel them and wait for a bit.
   Ecore_Pthread_Worker *work;
   Eina_List *l;
   Eina_Bool test;
   int iteration = 0;

   SLKL(_ecore_pending_job_threads_mutex);

   EINA_LIST_FREE(_ecore_pending_job_threads, work)
     {
        if (work->func_cancel)
          work->func_cancel((void *)work->data, (Ecore_Thread *)work);
        free(work);
     }

   EINA_LIST_FREE(_ecore_pending_job_threads_feedback, work)
     {
        if (work->func_cancel)
          work->func_cancel((void *)work->data, (Ecore_Thread *)work);
        free(work);
     }

   SLKU(_ecore_pending_job_threads_mutex);
   SLKL(_ecore_running_job_mutex);

   EINA_LIST_FOREACH(_ecore_running_job, l, work)
     ecore_thread_cancel((Ecore_Thread *)work);

   SLKU(_ecore_running_job_mutex);

   do
     {
        SLKL(_ecore_pending_job_threads_mutex);
        if (_ecore_thread_count + _ecore_thread_count_no_queue > 0)
          {
             test = EINA_TRUE;
          }
        else
          {
             test = EINA_FALSE;
          }
        SLKU(_ecore_pending_job_threads_mutex);
        iteration++;
        if (test)
          {
             _ecore_main_call_flush();
             usleep(1000);
          }
     } while (test == EINA_TRUE && iteration < 50);

   if (iteration == 20 && _ecore_thread_count > 0)
     {
        ERR("%i of the child thread are still running after 1s. This can lead to a segv. Sorry.", _ecore_thread_count);
     }

   if (_ecore_thread_global_hash)
     eina_hash_free(_ecore_thread_global_hash);
   have_main_loop_thread = 0;

   while ((work = eina_trash_pop(&_ecore_thread_worker_trash)))
     {
        free(work);
     }

   SLKD(_ecore_pending_job_threads_mutex);
   LRWKD(_ecore_thread_global_hash_lock);
   LKD(_ecore_thread_global_hash_mutex);
   SLKD(_ecore_running_job_mutex);
   CDD(_ecore_thread_global_hash_cond);
}

/**
 * @brief Runs a function in a separate thread.
 *
 * This function creates a new thread and runs @p func_blocking in it.
 * When @p func_blocking finishes, @p func_end will be called. If the
 * thread is cancelled, @p func_cancel will be called.
 *
 * @param func_blocking The function to run in the thread. This function
 *                      will be passed @p data as its first argument and
 *                      the Ecore_Thread handle as its second.
 * @param func_end The function to call when @p func_blocking finishes.
 *                 This function will be called in the main loop. It will
 *                 be passed @p data as its first argument and the
 *                 Ecore_Thread handle as its second. Can be @c NULL.
 * @param func_cancel The function to call when the thread is cancelled.
 *                    This function will be called in the main loop. It will
 *                    be passed @p data as its first argument and the
 *                    Ecore_Thread handle as its second. Can be @c NULL.
 * @param data The data to pass to the functions.
 * @return A handle to the new thread, or @c NULL on failure.
 *
 * @note The Ecore_Thread handle is only valid as long as the thread is running.
 *       Once @p func_end or @p func_cancel is called, the handle is invalid.
 * @note @p func_blocking must not call any Ecore functions that are not
 *       thread-safe. See individual function documentation for thread-safety.
 *       Typically, only Ecore functions that get or set data are thread-safe.
 *       Main loop iteration, event handling, etc., are not.
 *
 * Example:
 * @code
 * static void
 * _my_thread_blocking_func(void *data, Ecore_Thread *thread)
 * {
 *    // Perform long computation
 *    printf("Thread running with data: %s\n", (char *)data);
 *    // ecore_thread_check() can be used here to allow cancellation
 *    // if (ecore_thread_check(thread)) return;
 * }
 *
 * static void
 * _my_thread_end_func(void *data, Ecore_Thread *thread)
 * {
 *    printf("Thread finished with data: %s\n", (char *)data);
 *    // free(data) if it was allocated
 * }
 *
 * static void
 * _my_thread_cancel_func(void *data, Ecore_Thread *thread)
 * {
 *    printf("Thread cancelled with data: %s\n", (char *)data);
 *    // free(data) if it was allocated
 * }
 *
 * // ... in some function:
 * char *my_data = strdup("hello world");
 * ecore_thread_run(_my_thread_blocking_func,
 *                  _my_thread_end_func,
 *                  _my_thread_cancel_func,
 *                  my_data);
 * @endcode
 */
EAPI Ecore_Thread *
ecore_thread_run(Ecore_Thread_Cb func_blocking,
                 Ecore_Thread_Cb func_end,
                 Ecore_Thread_Cb func_cancel,
                 const void *data)
{
   Ecore_Pthread_Worker *work;
   Eina_Bool tried = EINA_FALSE;
   PH(thread);

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (!func_blocking) return NULL;

   work = _ecore_thread_worker_new();
   if (!work)
     {
        if (func_cancel)
          func_cancel((void *)data, NULL);
        return NULL;
     }

   work->u.short_run.func_blocking = func_blocking;
   work->func_end = func_end;
   work->func_cancel = func_cancel;
   work->cancel = EINA_FALSE;
   work->feedback_run = EINA_FALSE;
   work->message_run = EINA_FALSE;
   work->kill = EINA_FALSE;
   work->reschedule = EINA_FALSE;
   work->no_queue = EINA_FALSE;
   work->data = data;

   work->self = 0;
   work->hash = NULL;

   SLKL(_ecore_pending_job_threads_mutex);
   _ecore_pending_job_threads = eina_list_append(_ecore_pending_job_threads, work);

   if (_ecore_thread_count == _ecore_thread_count_max)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        return (Ecore_Thread *)work;
     }

   SLKU(_ecore_pending_job_threads_mutex);

   /* One more thread could be created. */
   eina_threads_init();

   SLKL(_ecore_pending_job_threads_mutex);

retry:
   if (PHC(thread, _ecore_thread_worker, NULL))
     {
        _ecore_thread_count++;
        SLKU(_ecore_pending_job_threads_mutex);
        return (Ecore_Thread *)work;
     }
   if (!tried)
     {
        _ecore_main_call_flush();
        tried = EINA_TRUE;
        goto retry;
     }

   if (_ecore_thread_count == 0)
     {
        _ecore_pending_job_threads = eina_list_remove(_ecore_pending_job_threads, work);

        if (work->func_cancel)
          work->func_cancel((void *)work->data, (Ecore_Thread *)work);

        _ecore_thread_worker_free(work);
        work = NULL;
     }
   SLKU(_ecore_pending_job_threads_mutex);

   eina_threads_shutdown();

   return (Ecore_Thread *)work;
}

/**
 * @brief Cancels a running thread.
 *
 * This function requests the cancellation of the thread @p thread.
 * If the thread is pending in a queue and hasn't started execution yet,
 * it is removed from the queue and its @p func_cancel callback is invoked.
 * In this case, the function returns @c EINA_TRUE.
 *
 * If the thread is already running, it is marked for cancellation.
 * The actual cancellation depends on the thread's execution function
 * periodically calling ecore_thread_check() or being at a cancellable point.
 * When the running thread acknowledges the cancellation (or finishes),
 * its @p func_cancel (or @p func_end if it finished before cancelling)
 * callback is invoked. In this case (thread was running), the function
 * returns @c EINA_FALSE, as cancellation is pending.
 *
 * If the thread is a feedback thread with pending notifications, cancellation
 * is deferred until all notifications are processed.
 *
 * @param thread The handle of the thread to cancel.
 * @return @c EINA_TRUE if the thread was cancelled immediately (e.g., it was
 *         pending and not yet running), or if @p thread was @c NULL.
 *         @c EINA_FALSE if the thread was running and cancellation is pending,
 *         or if the thread was already cancelled.
 *
 * @note This function can be called from the main loop or from another thread.
 *       However, if called from a different thread than the main loop, it might
 *       not be able to immediately cancel a pending job if that job is about to
 *       be picked up by a worker thread.
 */
EAPI Eina_Bool
ecore_thread_cancel(Ecore_Thread *thread)
{
   Ecore_Pthread_Worker *volatile work = (Ecore_Pthread_Worker *)thread;
   Eina_List *l;
   int cancel;

   if (!work)
     return EINA_TRUE;
   SLKL(work->cancel_mutex);
   cancel = work->cancel;
   SLKU(work->cancel_mutex);
   if (cancel)
     return EINA_FALSE;

   if (work->feedback_run)
     {
        if (work->kill)
          return EINA_TRUE;
        if (work->u.feedback_run.send != work->u.feedback_run.received)
          goto on_exit;
     }

   SLKL(_ecore_pending_job_threads_mutex);

   if ((have_main_loop_thread) &&
       (PHE(get_main_loop_thread(), PHS())))
     {
        if (!work->feedback_run)
          EINA_LIST_FOREACH(_ecore_pending_job_threads, l, work)
            {
               if ((void *)work == (void *)thread)
                 {
                    _ecore_pending_job_threads = eina_list_remove_list(_ecore_pending_job_threads, l);

                    SLKU(_ecore_pending_job_threads_mutex);

                    if (work->func_cancel)
                      work->func_cancel((void *)work->data, (Ecore_Thread *)work);
                    free(work);

                    return EINA_TRUE;
                 }
            }
        else
          EINA_LIST_FOREACH(_ecore_pending_job_threads_feedback, l, work)
            {
               if ((void *)work == (void *)thread)
                 {
                    _ecore_pending_job_threads_feedback = eina_list_remove_list(_ecore_pending_job_threads_feedback, l);

                    SLKU(_ecore_pending_job_threads_mutex);

                    if (work->func_cancel)
                      work->func_cancel((void *)work->data, (Ecore_Thread *)work);
                    free(work);

                    return EINA_TRUE;
                 }
            }
     }

   SLKU(_ecore_pending_job_threads_mutex);

   work = (Ecore_Pthread_Worker *)thread;

   /* Delay the destruction */
on_exit:
   eina_thread_cancel(work->self); /* noop unless eina_thread_cancellable_set() was used by user */
   SLKL(work->cancel_mutex);
   work->cancel = EINA_TRUE;
   SLKU(work->cancel_mutex);

   return EINA_FALSE;
}

/**
 * @brief Resets the original func_cancel and func_end callbacks on a worker
 *        after ecore_thread_wait has finished or timed out.
 * @param waiter The Ecore_Thread_Waiter structure used during the wait.
 * @param worker The Ecore_Pthread_Worker whose callbacks are being restored.
 */
static void
_ecore_thread_wait_reset(Ecore_Thread_Waiter *waiter,
                         Ecore_Pthread_Worker *worker)
{
   worker->func_cancel = waiter->func_cancel; // Restore original cancel func
   worker->func_end = waiter->func_end;       // Restore original end func
   worker->waiter = NULL;                     // Clear waiter link

   // Clear waiter's stored functions to prevent double calls if reset is called multiple times
   waiter->func_end = NULL;
   waiter->func_cancel = NULL;
   waiter->waiting = EINA_FALSE;              // Mark as no longer waiting
}

/**
 * @brief Wrapper cancel function used during ecore_thread_wait.
 * This function is temporarily set as the worker's func_cancel.
 * It calls the original func_cancel (if any) and then resets the
 * worker's callbacks using _ecore_thread_wait_reset.
 * @param data User data associated with the thread.
 * @param thread The Ecore_Thread being cancelled.
 */
static void
_ecore_thread_wait_cancel(void *data EINA_UNUSED, Ecore_Thread *thread)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Waiter *waiter = worker->waiter;

   if (waiter->func_cancel) waiter->func_cancel(data, thread); // Call original
   _ecore_thread_wait_reset(waiter, worker); // Restore and cleanup
}

/**
 * @brief Wrapper end function used during ecore_thread_wait.
 * This function is temporarily set as the worker's func_end.
 * It calls the original func_end (if any) and then resets the
 * worker's callbacks using _ecore_thread_wait_reset.
 * @param data User data associated with the thread.
 * @param thread The Ecore_Thread that has ended.
 */
static void
_ecore_thread_wait_end(void *data EINA_UNUSED, Ecore_Thread *thread)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Waiter *waiter = worker->waiter;

   if (waiter->func_end) waiter->func_end(data, thread); // Call original
   _ecore_thread_wait_reset(waiter, worker); // Restore and cleanup
}

/**
 * @brief Waits for a thread to complete, with a timeout.
 *
 * This function blocks the calling (main) thread until the specified
 * Ecore_Thread @p thread finishes its execution (either normally or by
 * cancellation), or until the @p wait timeout (in seconds) expires.
 *
 * While waiting, this function will periodically process main loop events
 * using `_ecore_main_call_flush()` and `ecore_main_loop_thread_safe_call_wait()`.
 * This allows callbacks (like `func_end` or `func_cancel` of the waited thread)
 * to be dispatched.
 *
 * @param thread The Ecore_Thread to wait for.
 * @param wait The maximum time in seconds to wait. If @c 0.0 or negative,
 *             it checks once and returns. To wait indefinitely, a very
 *             large value should be used (though not directly supported,
 *             the loop structure implies it would keep checking).
 * @return @c EINA_TRUE if the thread completed (its `func_end` or
 *         `func_cancel` was called) within the timeout.
 *         @c EINA_FALSE if the timeout was reached before the thread completed,
 *         or if @p thread was @c NULL.
 *
 * @note This function temporarily replaces the `func_end` and `func_cancel`
 *       callbacks of the @p thread with internal wrappers. These wrappers
 *       call the original callbacks and then signal completion to
 *       `ecore_thread_wait`.
 * @warning This function should only be called from the main Ecore loop thread.
 */
EAPI Eina_Bool
ecore_thread_wait(Ecore_Thread *thread, double wait)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Waiter waiter;

   if (!thread) return EINA_TRUE;

   waiter.func_end = worker->func_end;
   waiter.func_cancel = worker->func_cancel;
   waiter.waiting = EINA_TRUE;

   // Now trick the thread to call the wrapper function
   worker->waiter = &waiter;
   worker->func_cancel = _ecore_thread_wait_cancel;
   worker->func_end = _ecore_thread_wait_end;

   while (waiter.waiting == EINA_TRUE)
     {
        double start, end;

        start = ecore_time_get();
        _ecore_main_call_flush();
        ecore_main_loop_thread_safe_call_wait(0.0001);
        end = ecore_time_get();

        wait -= end - start;

        if (wait <= 0) break;
     }

   if (waiter.waiting == EINA_FALSE)
     {
        return EINA_TRUE;
     }
   else
     {
        _ecore_thread_wait_reset(&waiter, worker);
        return EINA_FALSE;
     }
}

EAPI Eina_Bool
ecore_thread_check(Ecore_Thread *thread)
{
   Ecore_Pthread_Worker *volatile worker = (Ecore_Pthread_Worker *)thread;
   int cancel;

   if (!worker) return EINA_TRUE;
   SLKL(worker->cancel_mutex);

   cancel = worker->cancel;
   /* FIXME: there is an insane bug driving me nuts here. I don't know if
      it's a race condition, some cache issue or some alien attack on our software.
      But ecore_thread_check will only work correctly with a printf, all the volatile,
      lock and even usleep don't help here... */
   /* fprintf(stderr, "wc: %i\n", cancel); */
   // The above comment suggests potential memory visibility issues or subtle race conditions
   // when reading the 'cancel' flag. Using a spinlock should mitigate this, but
   // the persistence of the comment implies there might have been deeper issues
   // observed under specific conditions. The `volatile` keyword on `worker` in the
   // function signature is also an attempt to address such issues.
   SLKU(worker->cancel_mutex);
   return cancel;
}

/**
 * @brief Runs a function in a separate thread with feedback capabilities.
 *
 * This function is similar to ecore_thread_run(), but is designed for
 * threads that need to send notifications back to the main loop during
 * their execution.
 *
 * The @p func_heavy is executed in the new thread. This function can call
 * ecore_thread_feedback() to send data to the main loop. When data is
 * sent via ecore_thread_feedback(), the @p func_notify callback will be
 * invoked in the main loop with the sent data.
 *
 * @param func_heavy The main function to run in the thread.
 *                   Passed @p data and the Ecore_Thread handle.
 * @param func_notify The function to call in the main loop when the thread
 *                    sends feedback via ecore_thread_feedback().
 *                    Passed @p data (original from run), the Ecore_Thread handle,
 *                    and the data sent by ecore_thread_feedback(). Can be @c NULL.
 * @param func_end The function to call in the main loop when @p func_heavy finishes.
 *                 Passed @p data and the Ecore_Thread handle. Can be @c NULL.
 * @param func_cancel The function to call in the main loop if the thread is cancelled.
 *                    Passed @p data and the Ecore_Thread handle. Can be @c NULL.
 * @param data User data to pass to the callback functions.
 * @param try_no_queue If @c EINA_TRUE, Ecore will attempt to run this thread
 *                     immediately without placing it in the general thread pool queue.
 *                     This creates a new dedicated system thread. If this fails
 *                     (e.g., resource limits), it falls back to the pooled approach.
 *                     If @c EINA_FALSE, it uses the standard thread pool.
 * @return A handle to the new thread, or @c NULL on failure.
 *
 * Example:
 * @code
 * static void
 * _my_feedback_heavy(void *data, Ecore_Thread *thread)
 * {
 *    for (int i = 0; i < 5; ++i)
 *    {
 *        if (ecore_thread_check(thread)) return; // Check for cancellation
 *        char *feedback_data = eina_slstr_printf("Progress: %d%%", (i + 1) * 20);
 *        ecore_thread_feedback(thread, feedback_data); // Send feedback
 *        // Note: feedback_data should be managed (e.g. freed in func_notify)
 *        usleep(500000); // Simulate work
 *    }
 * }
 *
 * static void
 * _my_feedback_notify(void *data, Ecore_Thread *thread, const void *feedback_data)
 * {
 *    printf("Main loop received: %s (Original data: %s)\n",
 *           (const char *)feedback_data, (const char *)data);
 *    eina_slstr_free((char *)feedback_data); // Free the string sent from thread
 * }
 *
 * static void
 * _my_feedback_end(void *data, Ecore_Thread *thread)
 * {
 *    printf("Feedback thread finished. Original data: %s\n", (const char *)data);
 * }
 *
 * // ...
 * ecore_thread_feedback_run(_my_feedback_heavy,
 *                           _my_feedback_notify,
 *                           _my_feedback_end,
 *                           NULL, // No specific cancel func here
 *                           "User Data For Feedback Thread",
 *                           EINA_FALSE);
 * @endcode
 */
EAPI Ecore_Thread *
ecore_thread_feedback_run(Ecore_Thread_Cb func_heavy,
                          Ecore_Thread_Notify_Cb func_notify,
                          Ecore_Thread_Cb func_end,
                          Ecore_Thread_Cb func_cancel,
                          const void *data,
                          Eina_Bool try_no_queue)
{
   Ecore_Pthread_Worker *worker;
   Eina_Bool tried = EINA_FALSE;
   PH(thread);

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(NULL);

   if (!func_heavy) return NULL;

   worker = _ecore_thread_worker_new();
   if (!worker) goto on_error;

   worker->u.feedback_run.func_heavy = func_heavy;
   worker->u.feedback_run.func_notify = func_notify;
   worker->hash = NULL;
   worker->func_cancel = func_cancel;
   worker->func_end = func_end;
   worker->data = data;
   worker->cancel = EINA_FALSE;
   worker->message_run = EINA_FALSE;
   worker->feedback_run = EINA_TRUE;
   worker->kill = EINA_FALSE;
   worker->reschedule = EINA_FALSE;
   worker->self = 0;

   worker->u.feedback_run.send = 0;
   worker->u.feedback_run.received = 0;

   worker->u.feedback_run.direct_worker = NULL;

   if (try_no_queue)
     {
        PH(t);

        worker->u.feedback_run.direct_worker = _ecore_thread_worker_new();
        worker->no_queue = EINA_TRUE;

        eina_threads_init();

retry_direct:
        if (PHC2(t, _ecore_direct_worker, worker))
          {
             SLKL(_ecore_pending_job_threads_mutex);
             _ecore_thread_count_no_queue++;
             SLKU(_ecore_pending_job_threads_mutex);
             return (Ecore_Thread *)worker;
          }
        if (!tried)
          {
             _ecore_main_call_flush();
             tried = EINA_TRUE;
             goto retry_direct;
          }

        if (worker->u.feedback_run.direct_worker)
          {
             _ecore_thread_worker_free(worker->u.feedback_run.direct_worker);
             worker->u.feedback_run.direct_worker = NULL;
          }

        eina_threads_shutdown();
     }

   worker->no_queue = EINA_FALSE;

   SLKL(_ecore_pending_job_threads_mutex);
   _ecore_pending_job_threads_feedback = eina_list_append(_ecore_pending_job_threads_feedback, worker);

   if (_ecore_thread_count == _ecore_thread_count_max)
     {
        SLKU(_ecore_pending_job_threads_mutex);
        return (Ecore_Thread *)worker;
     }

   SLKU(_ecore_pending_job_threads_mutex);

   /* One more thread could be created. */
   eina_threads_init();

   SLKL(_ecore_pending_job_threads_mutex);
retry:
   if (PHC(thread, _ecore_thread_worker, NULL))
     {
        _ecore_thread_count++;
        SLKU(_ecore_pending_job_threads_mutex);
        return (Ecore_Thread *)worker;
     }
   if (!tried)
     {
        _ecore_main_call_flush();
        tried = EINA_TRUE;
        goto retry;
     }
   SLKU(_ecore_pending_job_threads_mutex);

   eina_threads_shutdown();

on_error:
   SLKL(_ecore_pending_job_threads_mutex);
   if (_ecore_thread_count == 0)
     {
        _ecore_pending_job_threads_feedback = eina_list_remove(_ecore_pending_job_threads_feedback,
                                                               worker);

        if (func_cancel) func_cancel((void *)data, NULL);

        if (worker)
          {
             CDD(worker->cond);
             LKD(worker->mutex);
             free(worker);
             worker = NULL;
          }
     }
   SLKU(_ecore_pending_job_threads_mutex);

   return (Ecore_Thread *)worker;
}

/**
 * @brief Sends feedback data from a worker thread to the main loop.
 *
 * This function must be called from within the thread function (@p func_heavy)
 * of a thread started with ecore_thread_feedback_run() or from the
 * @p func_main of a thread started with ecore_thread_message_run().
 *
 * The @p data provided will be passed to the @p func_notify callback
 * registered when the thread was started. This callback is executed in the
 * main Ecore loop.
 *
 * @param thread The Ecore_Thread handle of the currently executing thread.
 * @param data The data to send as feedback. The ownership of this data is
 *             transferred to the Ecore system, and it's the responsibility
 *             of the @p func_notify callback to manage/free it if necessary.
 * @return @c EINA_TRUE if the feedback was successfully queued,
 *         @c EINA_FALSE otherwise (e.g., if @p thread is invalid, not a
 *         feedback/message thread, or if called from the wrong thread, or
 *         memory allocation failure).
 *
 * @note This function is thread-safe when called correctly from the worker thread.
 * @warning The @p data pointer is passed as is. If it points to stack-allocated
 *          memory in the worker thread, it will be invalid by the time
 *          @p func_notify is called. Ensure @p data points to heap-allocated
 *          or static memory that remains valid.
 */
EAPI Eina_Bool
ecore_thread_feedback(Ecore_Thread *thread,
                      const void *data)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;

   if (!worker) return EINA_FALSE;

   // Ensure this is called from the worker thread itself.
   if (!PHE(worker->self, PHS())) return EINA_FALSE;

   if (worker->feedback_run)
     {
        Ecore_Pthread_Notify *notify;

        notify = malloc(sizeof (Ecore_Pthread_Notify));
        if (!notify) return EINA_FALSE;

        notify->user_data = data; // Data to be passed to func_notify
        notify->work = worker;
        worker->u.feedback_run.send++; // Increment sent count

        // Schedule _ecore_notify_handler to run in the main loop
        ecore_main_loop_thread_safe_call_async(_ecore_notify_handler, notify);
     }
   else if (worker->message_run) // Also used by message_run for async messages from thread to main
     {
        Ecore_Pthread_Message *msg;
        Ecore_Pthread_Notify *notify;

        msg = malloc(sizeof (Ecore_Pthread_Message));
        if (!msg) return EINA_FALSE;
        msg->data = data;
        msg->callback = EINA_FALSE; // This is a simple data feedback, not a callback request
        msg->sync = EINA_FALSE;

        notify = malloc(sizeof (Ecore_Pthread_Notify));
        if (!notify)
          {
             free(msg);
             return EINA_FALSE;
          }
        notify->work = worker;
        notify->user_data = msg; // The Ecore_Pthread_Message is the user_data for _ecore_message_notify_handler

        worker->u.message_run.from.send++;
        ecore_main_loop_thread_safe_call_async(_ecore_message_notify_handler, notify);
     }
   else
     return EINA_FALSE; // Not a feedback or message thread

   return EINA_TRUE;
}

#if 0
EAPI Ecore_Thread *
ecore_thread_message_run(Ecore_Thread_Cb func_main,
                         Ecore_Thread_Notify_Cb func_notify,
                         Ecore_Thread_Cb func_end,
                         Ecore_Thread_Cb func_cancel,
                         const void *data)
{
   Ecore_Pthread_Worker *worker;
   PH(t);

   if (!func_main) return NULL;

   worker = _ecore_thread_worker_new();
   if (!worker) return NULL;

   worker->u.message_run.func_main = func_main;
   worker->u.message_run.func_notify = func_notify;
   worker->u.message_run.direct_worker = _ecore_thread_worker_new();
   worker->u.message_run.send = ecore_pipe_add(_ecore_nothing_handler, worker);
   worker->u.message_run.from.send = 0;
   worker->u.message_run.from.received = 0;
   worker->u.message_run.to.send = 0;
   worker->u.message_run.to.received = 0;

   ecore_pipe_freeze(worker->u.message_run.send);

   worker->func_cancel = func_cancel;
   worker->func_end = func_end;
   worker->hash = NULL;
   worker->data = data;

   worker->cancel = EINA_FALSE;
   worker->message_run = EINA_TRUE;
   worker->feedback_run = EINA_FALSE;
   worker->kill = EINA_FALSE;
   worker->reschedule = EINA_FALSE;
   worker->no_queue = EINA_FALSE;
   worker->self = 0;

   eina_threads_init();

   if (PHC(t, _ecore_direct_worker, worker))
     return (Ecore_Thread *)worker;

   eina_threads_shutdown();

   if (worker->u.message_run.direct_worker) _ecore_thread_worker_free(worker->u.message_run.direct_worker);
   if (worker->u.message_run.send) ecore_pipe_del(worker->u.message_run.send);

   CDD(worker->cond);
   LKD(worker->mutex);

   func_cancel((void *)data, NULL);

   return NULL;
}

#endif

/**
 * @brief Marks a thread to be rescheduled after it finishes.
 *
 * This function, when called from within a thread's execution function
 * (e.g., `func_blocking` or `func_heavy`), signals that the thread's task
 * should be run again after the current execution completes. The thread
 * will be added back to the pending queue.
 *
 * The same callback functions and data will be used for the rescheduled execution.
 *
 * @param thread The Ecore_Thread handle of the currently executing thread.
 * @return @c EINA_TRUE if the reschedule flag was successfully set.
 *         @c EINA_FALSE if @p thread is invalid or if called from a thread
 *         other than the one identified by @p thread.
 *
 * @note This is useful for periodic tasks or tasks that need to run multiple
 *       times without the overhead of creating a new thread structure each time.
 * @warning Must be called from the thread itself that is to be rescheduled.
 */
EAPI Eina_Bool
ecore_thread_reschedule(Ecore_Thread *thread)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;

   if (!worker) return EINA_FALSE;

   // Ensure this is called from the worker thread itself.
   if (!PHE(worker->self, PHS())) return EINA_FALSE;

   worker->reschedule = EINA_TRUE;
   return EINA_TRUE;
}

/**
 * @brief Gets the number of currently active (running) pooled Ecore threads.
 *
 * This counts threads managed by the Ecore thread pool that are currently
 * executing jobs. It does not include "no_queue" threads.
 *
 * @return The number of active pooled threads.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI int
ecore_thread_active_get(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   return _ecore_thread_count;
}

/**
 * @brief Gets the number of pending short-running (non-feedback) Ecore threads.
 *
 * This counts jobs submitted via ecore_thread_run() that are waiting in
 * the queue to be processed by a pooled worker thread.
 *
 * @return The number of pending short-running threads.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI int
ecore_thread_pending_get(void)
{
   int ret;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   SLKL(_ecore_pending_job_threads_mutex);
   ret = eina_list_count(_ecore_pending_job_threads);
   SLKU(_ecore_pending_job_threads_mutex);
   return ret;
}

/**
 * @brief Gets the number of pending feedback-style Ecore threads.
 *
 * This counts jobs submitted via ecore_thread_feedback_run() (that are not
 * using `try_no_queue` successfully) and are waiting in the queue to be
 * processed by a pooled worker thread.
 *
 * @return The number of pending feedback threads.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI int
ecore_thread_pending_feedback_get(void)
{
   int ret;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   SLKL(_ecore_pending_job_threads_mutex);
   ret = eina_list_count(_ecore_pending_job_threads_feedback);
   SLKU(_ecore_pending_job_threads_mutex);
   return ret;
}

/**
 * @brief Gets the total number of pending Ecore threads (both short-running and feedback).
 *
 * This is the sum of counts from ecore_thread_pending_get() and
 * ecore_thread_pending_feedback_get().
 *
 * @return The total number of pending threads in the pool queues.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI int
ecore_thread_pending_total_get(void)
{
   int ret;

   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   SLKL(_ecore_pending_job_threads_mutex);
   ret = eina_list_count(_ecore_pending_job_threads) + eina_list_count(_ecore_pending_job_threads_feedback);
   SLKU(_ecore_pending_job_threads_mutex);
   return ret;
}

/**
 * @brief Gets the maximum number of concurrent pooled Ecore threads allowed.
 *
 * This is the limit on how many worker threads Ecore will spawn for its pool.
 *
 * @return The maximum number of pooled threads.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI int
ecore_thread_max_get(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN_VAL(0);
   return _ecore_thread_count_max;
}

/**
 * @brief Sets the maximum number of concurrent pooled Ecore threads.
 *
 * Adjusts the size of the Ecore thread pool.
 *
 * @param num The new maximum number of threads. Must be at least 1.
 *            It's capped internally at 32 * eina_cpu_count() to prevent
 *            excessive thread creation.
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI void
ecore_thread_max_set(int num)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   if (num < 1) return;
   /* avoid doing something hilarious by blocking dumb users */
   // Cap the maximum to a sane value related to CPU count.
   if (num > (32 * eina_cpu_count())) num = 32 * eina_cpu_count();

   _ecore_thread_count_max = num;
}

/**
 * @brief Resets the maximum number of Ecore threads to the default value.
 *
 * The default is typically `eina_cpu_count() * 4`.
 *
 * @note This function must be called from the main Ecore loop thread.
 */
EAPI void
ecore_thread_max_reset(void)
{
   EINA_MAIN_LOOP_CHECK_RETURN;
   _ecore_thread_count_max = eina_cpu_count() * 4;
   if (_ecore_thread_count_max <= 0) _ecore_thread_count_max = 1; // Ensure at least 1
}

/**
 * @brief Gets the number of available slots for new pooled Ecore threads.
 *
 * This is calculated as `ecore_thread_max_get() - ecore_thread_active_get()`.
 * It indicates how many more pooled threads can be started before reaching the limit.
 *
 * @return The number of available thread slots in the pool.
 */
EAPI int
ecore_thread_available_get(void)
{
   int ret;

   SLKL(_ecore_pending_job_threads_mutex);
   ret = _ecore_thread_count_max - _ecore_thread_count;
   SLKU(_ecore_pending_job_threads_mutex);
   return ret;
}

/**
 * @brief Sets the name of an Ecore thread.
 *
 * This function allows setting a descriptive name for the specified Ecore thread.
 * The name is typically visible in debuggers and system monitoring tools.
 *
 * @param thread The Ecore_Thread handle whose name is to be set.
 * @param name The desired name for the thread.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *         Failure can occur if @p thread or @p name is @c NULL,
 *         if the thread handle is invalid (e.g., thread not running),
 *         or if this function is called from a thread other than the
 *         one identified by @p thread.
 *
 * @warning This function MUST be called from within the Ecore thread itself
 *          (i.e., from `func_blocking`, `func_heavy`, etc.).
 *          It cannot be used to name a thread from the outside.
 */
EAPI Eina_Bool
ecore_thread_name_set(Ecore_Thread *thread, const char *name)
{
   Ecore_Pthread_Worker *work = (Ecore_Pthread_Worker *) thread;

   if ((!work) || (!work->self) || (!name)) // Basic validation
     return EINA_FALSE;

   // Critical check: Must be called from the thread itself.
   if (eina_thread_self() != work->self) return EINA_FALSE;

   return eina_thread_name_set(work->self, name);
}

/**
 * @brief Adds thread-local data associated with a key.
 *
 * Stores a @p value associated with a @p key for the specified @p thread.
 * This data is local to the thread and can be retrieved using
 * ecore_thread_local_data_find().
 *
 * @param thread The Ecore_Thread to associate the data with.
 * @param key The string key for the data.
 * @param value The data pointer to store.
 * @param cb An optional Eina_Free_Cb callback that will be called with @p value
 *           when the data is removed (e.g., by ecore_thread_local_data_del(),
 *           ecore_thread_local_data_set() with the same key, or when the
 *           thread finishes and its local data store is freed). Can be @c NULL.
 * @param direct If @c EINA_TRUE, use eina_hash_direct_add (key must be
 *               persistent and unique, e.g., from eina_stringshare_add).
 *               If @c EINA_FALSE, use eina_hash_add (key will be copied or
 *               stringshared by the hash).
 * @return @c EINA_TRUE if the data was successfully added.
 *         @c EINA_FALSE on failure (e.g., @p thread, @p key, or @p value is @c NULL,
 *         memory allocation failure, or if the key already exists).
 *
 * @note The internal hash table for local data is created on demand.
 */
EAPI Eina_Bool
ecore_thread_local_data_add(Ecore_Thread *thread,
                            const char *key,
                            void *value,
                            Eina_Free_Cb cb,
                            Eina_Bool direct)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Data *d;
   Eina_Bool ret;

   if ((!thread) || (!key) || (!value))
     return EINA_FALSE;

   LKL(worker->mutex);
   if (!worker->hash)
     worker->hash = eina_hash_string_small_new(_ecore_thread_data_free);
   LKU(worker->mutex);

   if (!worker->hash)
     return EINA_FALSE;

   if (!(d = malloc(sizeof(Ecore_Thread_Data))))
     return EINA_FALSE;

   d->data = value;
   d->cb = cb;

   LKL(worker->mutex);
   if (direct)
     ret = eina_hash_direct_add(worker->hash, key, d);
   else
     ret = eina_hash_add(worker->hash, key, d);
   LKU(worker->mutex);
   CDB(worker->cond);
   return ret;
}

/**
 * @brief Sets or replaces thread-local data associated with a key.
 *
 * Stores a @p value associated with a @p key for the specified @p thread.
 * If data already exists for @p key, it is replaced. The `Eina_Free_Cb`
 * of the old data (if any) will be called.
 *
 * @param thread The Ecore_Thread to associate the data with.
 * @param key The string key for the data.
 * @param value The data pointer to store.
 * @param cb An optional Eina_Free_Cb callback that will be called with @p value
 *           when this new data is eventually removed or replaced. Can be @c NULL.
 * @return The previously stored data pointer for @p key if it was replaced,
 *         otherwise @c NULL. Returns @c NULL on failure (e.g., @p thread,
 *         @p key, or @p value is @c NULL, or memory allocation failure).
 *
 * @note The internal hash table for local data is created on demand.
 */
EAPI void *
ecore_thread_local_data_set(Ecore_Thread *thread,
                            const char *key,
                            void *value,
                            Eina_Free_Cb cb)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Data *d, *r_td; // r_td is the Ecore_Thread_Data struct being replaced
   void *ret_data = NULL; // The actual user data being replaced

   if ((!thread) || (!key) || (!value))
     return NULL;

   LKL(worker->mutex);
   if (!worker->hash) // Create hash on demand
     worker->hash = eina_hash_string_small_new(_ecore_thread_data_free);
   LKU(worker->mutex);

   if (!worker->hash)
     return NULL;

   if (!(d = malloc(sizeof(Ecore_Thread_Data)))) // New container for the value
     return NULL;

   d->data = value;
   d->cb = cb;

   LKL(worker->mutex);
   // eina_hash_set replaces if key exists, and returns the old Ecore_Thread_Data container.
   // _ecore_thread_data_free (the hash's free_cb) will be called on the old container by eina_hash.
   r_td = eina_hash_set(worker->hash, key, d);
   LKU(worker->mutex);
   CDB(worker->cond); // Broadcast, though not obviously used by local data set/get.

   if (r_td)
     {
        ret_data = r_td->data; // This is the user's old data pointer.
        // The Ecore_Thread_Data container 'r_td' itself is freed by the hash's
        // internal mechanisms when eina_hash_set replaces an item, which calls
        // _ecore_thread_data_free(r_td). _ecore_thread_data_free then calls r_td->cb(r_td->data).
        // So, we don't free r_td here.
        // The old Ecore_Thread_Data (r_td) is already handled by the hash replacement.
        // The value r_td->data is what the user might want back.
        // The Ecore_Thread_Data struct `r_td` itself is freed by the hash's free callback.
        // We only need to return the `r_td->data`.
        // The `free(r)` in the original code was incorrect as `r` (now `r_td`)
        // is managed by the hash.
        // The `_ecore_thread_data_free` callback (passed to eina_hash_string_small_new)
        // is responsible for calling `r_td->cb(r_td->data)` and then `free(r_td)`.
        // So, the user's old data's free callback has already been triggered if it existed.
        // We just return the pointer to the old user data.
     }
   return ret_data; // Return the old user data pointer, or NULL if no replacement.
}

/**
 * @brief Finds thread-local data associated with a key.
 *
 * Retrieves the data pointer stored for @p key within the specified @p thread.
 *
 * @param thread The Ecore_Thread to search within.
 * @param key The string key for the data.
 * @return The data pointer if found, otherwise @c NULL.
 *         Returns @c NULL if @p thread or @p key is @c NULL, or if the
 *         thread has no local data store.
 */
EAPI void *
ecore_thread_local_data_find(Ecore_Thread *thread,
                             const char *key)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Ecore_Thread_Data *d;

   if ((!thread) || (!key))
     return NULL;

   if (!worker->hash)
     return NULL;

   LKL(worker->mutex);
   d = eina_hash_find(worker->hash, key);
   LKU(worker->mutex);
   if (d)
     return d->data;
   return NULL;
}

/**
 * @brief Deletes thread-local data associated with a key.
 *
 * Removes the data stored for @p key from the specified @p thread's local
 * store. If an `Eina_Free_Cb` was provided when the data was added/set,
 * it will be called with the data pointer.
 *
 * @param thread The Ecore_Thread from which to delete the data.
 * @param key The string key of the data to delete.
 * @return @c EINA_TRUE if the data was successfully deleted or if the key
 *         was not found.
 *         @c EINA_FALSE if @p thread or @p key is @c NULL, or if the thread
 *         has no local data store.
 *
 * @note `eina_hash_del_by_key` will trigger the hash's `free_cb`
 *       (`_ecore_thread_data_free`) for the removed item, which in turn
 *       calls the user's `cb` on the data.
 */
EAPI Eina_Bool
ecore_thread_local_data_del(Ecore_Thread *thread,
                            const char *key)
{
   Ecore_Pthread_Worker *worker = (Ecore_Pthread_Worker *)thread;
   Eina_Bool r;

   if ((!thread) || (!key))
     return EINA_FALSE;

   if (!worker->hash) // No hash means key cannot exist
     return EINA_FALSE; // Or EINA_TRUE if "not found" is success for del. Current eina_hash_del returns false if not found.

   LKL(worker->mutex);
   r = eina_hash_del_by_key(worker->hash, key);
   LKU(worker->mutex);
   return r;
}

/**
 * @brief Adds global data associated with a key, accessible from any thread.
 *
 * Stores a @p value associated with a @p key in a global hash table.
 * This data can be accessed by any thread using ecore_thread_global_data_find().
 *
 * @param key The string key for the data.
 * @param value The data pointer to store.
 * @param cb An optional Eina_Free_Cb callback that will be called with @p value
 *           when the data is removed. Can be @c NULL.
 * @param direct If @c EINA_TRUE, use eina_hash_direct_add.
 *               If @c EINA_FALSE, use eina_hash_add.
 * @return @c EINA_TRUE if the data was successfully added.
 *         @c EINA_FALSE on failure (e.g., @p key or @p value is @c NULL,
 *         memory allocation failure, or key already exists).
 *
 * @note The global hash table is created on demand. Access is protected by a read-write lock.
 */
EAPI Eina_Bool
ecore_thread_global_data_add(const char *key,
                             void *value,
                             Eina_Free_Cb cb,
                             Eina_Bool direct)
{
   Ecore_Thread_Data *d;
   Eina_Bool ret;

   if ((!key) || (!value))
     return EINA_FALSE;

   LRWKWL(_ecore_thread_global_hash_lock);
   if (!_ecore_thread_global_hash)
     _ecore_thread_global_hash = eina_hash_string_small_new(_ecore_thread_data_free);
   LRWKU(_ecore_thread_global_hash_lock);

   if (!(d = malloc(sizeof(Ecore_Thread_Data))))
     return EINA_FALSE;

   d->data = value;
   d->cb = cb;

   if (!_ecore_thread_global_hash)
     {
        free(d);
        return EINA_FALSE;
     }

   LRWKWL(_ecore_thread_global_hash_lock);
   if (direct)
     ret = eina_hash_direct_add(_ecore_thread_global_hash, key, d);
   else
     ret = eina_hash_add(_ecore_thread_global_hash, key, d);
   LRWKU(_ecore_thread_global_hash_lock);
   CDB(_ecore_thread_global_hash_cond);
   return ret;
}

/**
 * @brief Sets or replaces global data associated with a key.
 *
 * Stores a @p value associated with a @p key in the global hash table.
 * If data already exists for @p key, it is replaced. The `Eina_Free_Cb`
 * of the old data (if any) will be called.
 *
 * @param key The string key for the data.
 * @param value The data pointer to store.
 * @param cb An optional Eina_Free_Cb callback for @p value. Can be @c NULL.
 * @return The previously stored data pointer for @p key if replaced,
 *         otherwise @c NULL. Returns @c NULL on failure.
 *
 * @note Access is protected by a read-write lock.
 *       The `_ecore_thread_data_free` callback (set on the hash) handles
 *       calling the user's `cb` for the replaced data and freeing the
 *       internal `Ecore_Thread_Data` container.
 */
EAPI void *
ecore_thread_global_data_set(const char *key,
                             void *value,
                             Eina_Free_Cb cb)
{
   Ecore_Thread_Data *d, *r_td; // r_td is the Ecore_Thread_Data struct being replaced
   void *ret_data = NULL; // The actual user data being replaced

   if ((!key) || (!value))
     return NULL;

   LRWKWL(_ecore_thread_global_hash_lock); // Write lock for potential hash creation
   if (!_ecore_thread_global_hash)
     _ecore_thread_global_hash = eina_hash_string_small_new(_ecore_thread_data_free);
   LRWKU(_ecore_thread_global_hash_lock);

   if (!_ecore_thread_global_hash)
     return NULL;

   if (!(d = malloc(sizeof(Ecore_Thread_Data))))
     return NULL;

   d->data = value;
   d->cb = cb;

   LRWKWL(_ecore_thread_global_hash_lock); // Write lock for hash modification
   r_td = eina_hash_set(_ecore_thread_global_hash, key, d);
   LRWKU(_ecore_thread_global_hash_lock);
   CDB(_ecore_thread_global_hash_cond); // Broadcast for ecore_thread_global_data_wait

   if (r_td)
     {
        ret_data = r_td->data;
        // As with local_data_set, the Ecore_Thread_Data container r_td is
        // managed by the hash and its free_cb (_ecore_thread_data_free).
        // _ecore_thread_data_free calls r_td->cb(r_td->data) and then frees r_td.
        // The `free(r)` in the original code was incorrect.
     }
   return ret_data; // Return old user data pointer
}

/**
 * @brief Finds global data associated with a key.
 *
 * Retrieves the data pointer stored for @p key from the global hash table.
 *
 * @param key The string key for the data.
 * @return The data pointer if found, otherwise @c NULL.
 *         Returns @c NULL if @p key is @c NULL or the global store is not initialized.
 *
 * @note Access is protected by a read-lock.
 */
EAPI void *
ecore_thread_global_data_find(const char *key)
{
   Ecore_Thread_Data *ret;

   if (!key)
     return NULL;

   if (!_ecore_thread_global_hash) return NULL;

   LRWKRL(_ecore_thread_global_hash_lock);
   ret = eina_hash_find(_ecore_thread_global_hash, key);
   LRWKU(_ecore_thread_global_hash_lock);
   if (ret)
     return ret->data;
   return NULL;
}

/**
 * @brief Deletes global data associated with a key.
 *
 * Removes the data stored for @p key from the global hash table.
 * If an `Eina_Free_Cb` was provided, it will be called with the data.
 *
 * @param key The string key of the data to delete.
 * @return @c EINA_TRUE if data was deleted or key not found.
 *         @c EINA_FALSE if @p key is @c NULL or global store not initialized.
 *
 * @note Access is protected by a write-lock.
 *       `eina_hash_del_by_key` triggers `_ecore_thread_data_free` for the item.
 */
EAPI Eina_Bool
ecore_thread_global_data_del(const char *key)
{
   Eina_Bool ret;

   if (!key)
     return EINA_FALSE;

   if (!_ecore_thread_global_hash) // No hash, key cannot exist.
     return EINA_FALSE; // eina_hash_del returns false if key not found.

   LRWKWL(_ecore_thread_global_hash_lock); // Write lock for hash modification
   ret = eina_hash_del_by_key(_ecore_thread_global_hash, key);
   LRWKU(_ecore_thread_global_hash_lock);
   return ret;
}

/**
 * @brief Waits for global data associated with a key to become available.
 *
 * This function blocks the calling thread until data for the given @p key
 * is found in the global data store, or until the @p seconds timeout expires.
 *
 * @param key The string key for the data to wait for.
 * @param seconds The maximum time in seconds to wait.
 *                If @c 0.0, checks once and returns immediately (non-blocking).
 *                If negative, waits indefinitely (or until data appears).
 *                (Note: EINA_DBL_EQ(seconds, 0.0) is used, so negative might behave like 0 for timeout calc).
 * @return The data pointer if found within the timeout, otherwise @c NULL.
 *
 * @note This uses a condition variable (`_ecore_thread_global_hash_cond`)
 *       that is broadcast whenever global data is added or set.
 */
EAPI void *
ecore_thread_global_data_wait(const char *key,
                              double seconds)
{
   double tm = 0; // Target time for timeout
   Ecore_Thread_Data *ret = NULL;

   if (!key)
     return NULL;

   if (seconds > 0)
     tm = ecore_time_get() + seconds;

   while (1)
     {
        LRWKRL(_ecore_thread_global_hash_lock);
        if (_ecore_thread_global_hash)
          ret = eina_hash_find(_ecore_thread_global_hash, key);
        LRWKU(_ecore_thread_global_hash_lock);
        if ((ret) ||
            (!EINA_DBL_EQ(seconds, 0.0)) ||
            ((seconds > 0) && (tm <= ecore_time_get())))
          break;
        LKL(_ecore_thread_global_hash_mutex);
        CDW(_ecore_thread_global_hash_cond, tm - ecore_time_get());
        LKU(_ecore_thread_global_hash_mutex);
     }
   if (ret) return ret->data;
   return NULL;
}

