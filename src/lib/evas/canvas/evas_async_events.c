/**
 * @file
 * @brief Evas asynchronous event handling.
 *
 * This file implements the mechanisms for handling events asynchronously
 * within Evas, allowing for thread-safe event posting and processing.
 * It uses an Ecore_Pipe to signal the main loop when new events are pending.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <errno.h>

#ifdef _WIN32
# include <winsock2.h>
# include <evil_private.h> /* fcntl */
#endif

#include <fcntl.h>

#include "evas_common_private.h"
#include "evas_private.h"
#include "Ecore.h"

/**
 * @brief Structure to hold asynchronous event data.
 */
typedef struct _Evas_Event_Async	Evas_Event_Async;
struct _Evas_Event_Async
{
   const void		    *target; /**< Target object for the event. */
   void			    *event_info; /**< Event specific data. */
   Evas_Async_Events_Put_Cb  func; /**< Callback function to execute for this event. */
   Evas_Callback_Type	     type; /**< Type of the Evas callback. */
};

/**
 * @brief Structure for managing safe calls across threads, particularly for main loop locking.
 */
typedef struct _Evas_Safe_Call Evas_Safe_Call;
struct _Evas_Safe_Call
{
   Eina_Condition c; /**< Condition variable for synchronization. */
   Eina_Lock      m; /**< Mutex for protecting access to this structure. */

   int            current_id; /**< Identifier for the current lock request. */
};

static Eina_Lock _thread_mutex; /**< Mutex for general thread synchronization related to main loop locking. */
static Eina_Condition _thread_cond; /**< Condition variable for general thread synchronization. */

static Eina_Lock _thread_feedback_mutex; /**< Mutex for feedback synchronization from the main loop. */
static Eina_Condition _thread_feedback_cond; /**< Condition variable for feedback synchronization. */

static int _thread_loop = 0; /**< Counter for nested main loop locks. */

static Eina_Spinlock _thread_id_lock; /**< Spinlock for protecting thread ID generation. */
static int _thread_id = -1; /**< ID of the thread currently holding the main loop lock. -1 if none. */
static int _thread_id_max = 0; /**< Maximum thread ID assigned so far, used for generating unique IDs. */
static int _thread_id_update = 0; /**< Used to signal which thread ID's lock has been processed. */

static Eina_Bool _write_error = EINA_TRUE; /**< Flag indicating if a pipe write error has occurred. */
static Eina_Bool _read_error = EINA_TRUE; /**< Flag indicating if a pipe read error has occurred. */

static Eina_Spinlock async_lock; /**< Spinlock for protecting access to the async event queue. */
static Eina_Inarray async_queue; /**< Inarray storing pending asynchronous events. */
static Evas_Event_Async *async_queue_cache = NULL; /**< Cache for the async_queue's members array to reduce reallocations. */
static unsigned int async_queue_cache_max = 0; /**< Maximum size of the async_queue_cache. */

static Ecore_Pipe *_async_pipe = NULL; /**< Ecore_Pipe used to signal the main loop about pending events. */
static int _event_count = 0; /**< Counter for events processed in one go. */


static int _init_evas_event = 0; /**< Initialization counter for async event system. */
static const int wakeup = 1; /**< Integer value written to the pipe to wake it up. */

static void _evas_async_events_fd_blocking_set(Eina_Bool blocking EINA_UNUSED);

/**
 * @brief Callback executed when data is available on the async event pipe.
 *
 * This function is called by Ecore when the pipe receives data, indicating
 * that new asynchronous events have been queued. It processes all events
 * currently in the queue.
 *
 * @param data User data associated with the pipe (unused).
 * @param buf Buffer containing data read from the pipe. Expected to be &wakeup.
 * @param len Length of the data read. Expected to be sizeof(int).
 */
static void
_async_events_pipe_read_cb(void *data EINA_UNUSED, void *buf, unsigned int len)
{
   if (!buf || wakeup != *(int*)buf || sizeof(int) != len)
     return;

   Evas_Event_Async *ev;
   unsigned int ln, max;
   int nr;

   eina_spinlock_take(&async_lock);

   ev = async_queue.members;
   async_queue.members = async_queue_cache;
   async_queue_cache = ev;

   max = async_queue.max;
   async_queue.max = async_queue_cache_max;
   async_queue_cache_max = max;

   ln = async_queue.len;
   async_queue.len = 0;

   eina_spinlock_release(&async_lock);

   DBG("Evas async events queue length: %u", len);
   nr = ln;

   while (ln > 0)
    {
       if (ev->func) ev->func((void *)ev->target, ev->type, ev->event_info);
       ev++;
       ln--;
    }
   _event_count += nr;

   _evas_async_events_fd_blocking_set(EINA_FALSE);
}

/**
 * @brief Handles Evas async events after a fork.
 *
 * This function is registered as an Ecore fork reset callback.
 * It re-initializes the Ecore_Pipe used for async event notification
 * in the child process after a fork, as pipe file descriptors are
 * not typically inherited or may not work correctly without re-creation.
 *
 * @param data User data associated with the callback (unused).
 */
static void
_evas_async_events_fork_handle(void *data EINA_UNUSED)
{
   ecore_pipe_del(_async_pipe);
   _async_pipe = ecore_pipe_add(_async_events_pipe_read_cb, NULL);
}

/**
 * @brief Initializes the Evas asynchronous event system.
 *
 * Sets up the necessary Ecore_Pipe for inter-thread communication,
 * initializes locks, and prepares the event queue.
 * This function is ref-counted.
 *
 * @return The new reference count, or 0 on failure.
 */
int
evas_async_events_init(void)
{

   if (_init_evas_event++)
     return _init_evas_event;

   ecore_fork_reset_callback_add(_evas_async_events_fork_handle, NULL);

   _async_pipe = ecore_pipe_add(_async_events_pipe_read_cb, NULL);
   if ( !_async_pipe )
     {
        _init_evas_event = 0;
        return 0;
     }

   _read_error = EINA_FALSE;
   _write_error = EINA_FALSE;

   eina_spinlock_new(&async_lock);
   eina_inarray_step_set(&async_queue, sizeof (Eina_Inarray), sizeof (Evas_Event_Async), 16);

   eina_lock_new(&_thread_mutex);
   eina_condition_new(&_thread_cond, &_thread_mutex);

   eina_lock_new(&_thread_feedback_mutex);
   eina_condition_new(&_thread_feedback_cond, &_thread_feedback_mutex);

   eina_spinlock_new(&_thread_id_lock);

   return _init_evas_event;
}

/**
 * @brief Shuts down the Evas asynchronous event system.
 *
 * Cleans up resources used by the async event system, including
 * the Ecore_Pipe, locks, and event queue.
 * This function is ref-counted.
 *
 * @return The new reference count.
 */
int
evas_async_events_shutdown(void)
{
   if (--_init_evas_event)
     return _init_evas_event;

   eina_condition_free(&_thread_cond);
   eina_lock_free(&_thread_mutex);
   eina_condition_free(&_thread_feedback_cond);
   eina_lock_free(&_thread_feedback_mutex);
   eina_spinlock_free(&_thread_id_lock);

   free(async_queue_cache);
   async_queue_cache = NULL;
   async_queue_cache_max = 0;

   eina_spinlock_free(&async_lock);
   eina_inarray_flush(&async_queue);

   ecore_fork_reset_callback_del(_evas_async_events_fork_handle, NULL);

   ecore_pipe_del(_async_pipe);
   _read_error = EINA_TRUE;
   _write_error = EINA_TRUE;

   return _init_evas_event;
}

/**
 * @brief Gets the file descriptor for asynchronous events.
 * @deprecated This function is deprecated and always returns -1.
 *             The underlying mechanism no longer exposes a raw FD directly
 *             in a way that's safe or useful for external polling.
 *             Use evas_async_events_process() or integrate with Ecore's main loop.
 *
 * @return -1, as this functionality is deprecated.
 */
EVAS_API int
evas_async_events_fd_get(void)
{
   return -1;
}

/**
 * @brief Processes pending asynchronous events in a non-blocking manner.
 *
 * Checks the Ecore_Pipe for pending data (which signals new events) and
 * processes them if any are available. This function will not block.
 *
 * @return The number of events processed, or -1 if a read error occurred on the pipe.
 */
EVAS_API int
evas_async_events_process(void)
{
   int count = 0;

   if (_read_error) return -1;

   _event_count = 0;
   /* ecore_pipe_wait with 0.0 timeout is non-blocking */
   while (ecore_pipe_wait(_async_pipe, 1, 0.0))
     count = _event_count;

   return count;
}

/**
 * @brief Sets the blocking mode of the read end of the async event pipe.
 *
 * This is an internal helper function.
 *
 * @param blocking EINA_TRUE to set to blocking, EINA_FALSE for non-blocking.
 */
static void
_evas_async_events_fd_blocking_set(Eina_Bool blocking)
{
#ifdef HAVE_FCNTL
   int _fd_read = ecore_pipe_read_fd(_async_pipe);
   long flags = fcntl(_fd_read, F_GETFL);

   if (blocking) flags &= ~O_NONBLOCK;
   else flags |= O_NONBLOCK;

   if (fcntl(_fd_read, F_SETFL, flags) < 0) ERR("cannot set fd flags");
#else
   (void) blocking;
#endif
}

/**
 * @brief Processes pending asynchronous events in a blocking manner.
 *
 * Waits for data on the Ecore_Pipe (signaling new events) and processes them.
 * This function will block until at least one event is processed or an error occurs.
 * Note: The ecore_pipe_wait call inside is currently configured with a 0.0 timeout,
 * which makes it behave non-blockingly despite the function's name and intent.
 * This might be a bug or a misunderstanding of its historical usage.
 * For true blocking, the timeout in ecore_pipe_wait should be > 0 or negative.
 *
 * @return The number of events processed, or -1 if a read error occurred on the pipe.
 */
EVAS_API int
evas_async_events_process_blocking(void)
{
   int ret;
   if (_read_error) return -1;

   _evas_async_events_fd_blocking_set(EINA_TRUE);

   _event_count = 0;
   /* FIXME: ecore_pipe_wait with 0.0 timeout is non-blocking.
    * For blocking behavior, timeout should be > 0 or negative.
    * This might be intentional to process only currently available events
    * after setting the FD to blocking, but the name is misleading.
    */
   ecore_pipe_wait(_async_pipe, 1, 0.0);
   ret = _event_count;

   _evas_async_events_fd_blocking_set(EINA_FALSE);

   return ret;
}

/**
 * @brief Puts an asynchronous event into the queue.
 *
 * This function is thread-safe. It adds an event to a queue, and if the
 * queue was empty, it signals the main loop via an Ecore_Pipe.
 *
 * @param target The target object for the event (e.g., an Evas_Object or Evas_Canvas).
 * @param type The type of Evas callback.
 * @param event_info Event-specific data.
 * @param func The callback function to execute for this event in the main thread.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., queue full, pipe write error).
 */
EVAS_API Eina_Bool
evas_async_events_put(const void *target, Evas_Callback_Type type, void *event_info, Evas_Async_Events_Put_Cb func)
{
   Evas_Event_Async *ev;
   unsigned int count;
   Eina_Bool ret = EINA_TRUE;;

   if (!func) return EINA_FALSE;
   if (_write_error) return EINA_FALSE;

   eina_spinlock_take(&async_lock);

   count = async_queue.len;
   ev = eina_inarray_grow(&async_queue, 1);
   if (!ev)
     {
        eina_spinlock_release(&async_lock);
        return EINA_FALSE;
     }

   ev->func = func;
   ev->target = target;
   ev->type = type;
   ev->event_info = event_info;

   eina_spinlock_release(&async_lock);

   if (count == 0)
     {
        if (!ecore_pipe_write(_async_pipe, &wakeup, sizeof(int)))
          {
             ret = EINA_FALSE;
             _write_error = EINA_TRUE;
          }
     }

   return ret;
}

/**
 * @brief Callback function executed in the main Ecore thread to acquire the Evas main loop lock.
 *
 * This function is posted as an async event by evas_thread_main_loop_begin().
 * It synchronizes with the calling thread, assigns a thread ID for the lock,
 * and then calls eina_main_loop_define() to mark the current thread context
 * as the one "owning" the Eina main loop, effectively serializing access.
 *
 * @param target Unused.
 * @param type Unused.
 * @param event_info Pointer to an Evas_Safe_Call structure containing synchronization primitives
 *                   and the ID for this lock request.
 */
static void
_evas_thread_main_loop_lock(void *target EINA_UNUSED,
                            Evas_Callback_Type type EINA_UNUSED,
                            void *event_info)
{
   Evas_Safe_Call *call = event_info;

   eina_lock_take(&_thread_mutex);

   eina_lock_take(&call->m);
   _thread_id = call->current_id;
   eina_condition_broadcast(&call->c);
   eina_lock_release(&call->m);

   while (_thread_id_update != _thread_id)
     eina_condition_wait(&_thread_cond);
   eina_lock_release(&_thread_mutex);

   eina_main_loop_define();

   eina_lock_take(&_thread_feedback_mutex);

   _thread_id = -1;

   eina_condition_broadcast(&_thread_feedback_cond);
   eina_lock_release(&_thread_feedback_mutex);

   eina_condition_free(&call->c);
   eina_lock_free(&call->m);
   free(call);
}

/**
 * @brief Acquires a lock on the Evas/Eina main loop for the calling thread.
 *
 * This function allows a thread to safely interact with Evas or other
 * Eina main loop dependent operations. It ensures that only one thread
 * "owns" the main loop context at a time. This is a blocking call;
 * it will wait until the main loop lock is acquired.
 * This function is nestable; a thread can call it multiple times, and the
 * lock will only be released after a corresponding number of calls to
 * evas_thread_main_loop_end().
 *
 * @return The current lock nesting level for this thread (>= 1), or -1 on failure.
 *         If the calling thread already owns the main loop (e.g., it's the Ecore main thread
 *         or has already called this function), it increments a counter and returns immediately.
 */
EVAS_API int
evas_thread_main_loop_begin(void)
{
   Evas_Safe_Call *order;

   // If the current thread already "is" the main loop (e.g. it's the ecore main thread
   // or it has called evas_thread_main_loop_begin() before and not yet ended it),
   // just increment the nesting counter.
   if (eina_main_loop_is())
     {
        return ++_thread_loop;
     }

   order = malloc(sizeof (Evas_Safe_Call));
   if (!order) return -1;

   eina_spinlock_take(&_thread_id_lock);
   order->current_id = ++_thread_id_max;
   if (order->current_id < 0)
     {
        _thread_id_max = 0;
        order->current_id = ++_thread_id_max;
     }
   eina_spinlock_release(&_thread_id_lock);

   eina_lock_new(&order->m);
   eina_condition_new(&order->c, &order->m);

   evas_async_events_put(NULL, 0, order, _evas_thread_main_loop_lock);

   eina_lock_take(&order->m);
   while (order->current_id != _thread_id)
     eina_condition_wait(&order->c);
   eina_lock_release(&order->m);

   eina_main_loop_define();

   _thread_loop = 1;

   return _thread_loop;
}

/**
 * @brief Releases a lock on the Evas/Eina main loop previously acquired by the calling thread.
 *
 * This function must be called by a thread that has successfully called
 * evas_thread_main_loop_begin(). It decrements a nesting counter. If the
 * counter reaches zero, it releases the main loop lock, allowing another
 * waiting thread to acquire it.
 *
 * @return The current lock nesting level (0 if fully released), or -1 if called
 *         by a thread that does not currently own the lock.
 *         It will abort if called when the lock count is already zero.
 */
EVAS_API int
evas_thread_main_loop_end(void)
{
   int current_id;

   if (_thread_loop == 0)
     abort(); // Abort if trying to end a loop that was never begun or already fully ended.

   /* until we unlock the main loop, this thread has the main loop id */
   if (!eina_main_loop_is())
     {
        ERR("Not in a locked thread !");
        return -1;
     }

   _thread_loop--;
   if (_thread_loop > 0)
     return _thread_loop;

   current_id = _thread_id;

   eina_lock_take(&_thread_mutex);
   _thread_id_update = _thread_id;
   eina_condition_broadcast(&_thread_cond);
   eina_lock_release(&_thread_mutex);

   eina_lock_take(&_thread_feedback_mutex);
   while (current_id == _thread_id && _thread_id != -1)
     eina_condition_wait(&_thread_feedback_cond);
   eina_lock_release(&_thread_feedback_mutex);

   return 0;
}
