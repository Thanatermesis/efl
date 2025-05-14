/* EINA - EFL data type library
 * Copyright (C) 2020 Expertise Solutions Cons em Inf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eina_types.h"
#include "eina_config.h"
#include "eina_array.h"
#include "eina_thread.h"
#include "eina_main.h"
#include "eina_debug_private.h"
#include "eina_log.h"

#include <assert.h>
#include <evil_private.h>
#include <process.h>

#define RTNICENESS 1
#define NICENESS 5

/**
 * @internal
 * @brief Represents a thread in the Eina library on Windows.
 *
 * This structure holds all the necessary information for managing a thread,
 * including its handle, ID, state, and cleanup functions.
 */
struct Thread
{
   CRITICAL_SECTION cancel_lock; /**< Mutex to protect cancellation-related members. */
   char name[16];                /**< The name of the thread, for debugging. */
   HANDLE handle;                /**< The native Windows thread handle. */
   void *data;                   /**< User-provided data passed to the thread function, and stores the return value on exit. */
   Eina_Thread_Cb fn;            /**< The function to be executed by the thread. */
   Eina_Array *cleanup_fn;       /**< Array of cleanup callback functions (Eina_Thread_Cleanup_Cb). */
   Eina_Array *cleanup_arg;      /**< Array of arguments for the cleanup callback functions. */
   unsigned id;                  /**< The Windows thread ID. */
   Eina_Bool free_on_exit;       /**< Flag indicating if this structure should be freed when the thread exits (used for threads not created by eina_thread_create). */
   volatile Eina_Bool cancel;    /**< Flag indicating if a cancellation request has been made. */
   volatile Eina_Bool cancellable; /**< Flag indicating if the thread is currently cancellable. */
};

typedef struct Thread Thread_t;

/**
 * @internal
 * @brief Thread-Local Storage (TLS) key for storing the current thread's Thread_t structure.
 *
 * This allows `eina_thread_self()` to quickly retrieve the Eina_Thread handle
 * for the calling thread.
 */
static DWORD tls_thread_self = 0;

/**
 * @internal
 * @brief Represents the main application thread.
 *
 * This structure is initialized by `eina_thread_init()` and used to manage
 * cleanup handlers and cancellation state for the main thread.
 */
static Thread_t main_thread = { 0 };

/**
 * @internal
 * @brief Frees the Thread_t structure associated with the current thread if it was dynamically allocated.
 *
 * This function is typically registered as a TLS cleanup callback or called
 * when a thread created outside of Eina's management (but later wrapped by
 * `eina_thread_self()`) exits. It ensures that resources like cleanup function
 * arrays are released.
 */
void
free_thread(void)
{
   Thread_t *t = TlsGetValue(tls_thread_self);
   if (t && t->free_on_exit)
     {
        if (t) eina_array_free(t->cleanup_fn);
        if (t) eina_array_free(t->cleanup_arg);
        free(t);
     }
}

/**
 * @internal
 * @brief The actual function executed by a new thread created via `eina_thread_create`.
 *
 * This function sets up the thread-local storage for `eina_thread_self()`,
 * registers the thread with the Eina debug system, executes the user-provided
 * thread function, and performs cleanup.
 *
 * @param arg A pointer to the Thread_t structure for this thread.
 * @return Always 0. The actual return value of the user's thread function
 *         is stored in `thr->data`.
 */
static unsigned
thread_fn(void *arg)
{
   Thread_t *thr = arg;
   TlsSetValue(tls_thread_self, thr);
   _eina_debug_thread_add(&thr);
   EINA_THREAD_CLEANUP_PUSH(_eina_debug_thread_del, &thr);
   thr->data = thr->fn(thr->data, (Eina_Thread) thr);
   EINA_THREAD_CLEANUP_POP(EINA_TRUE);
   return 0;
}

/**
 * @brief Get a handle to the calling thread.
 *
 * If the calling thread was created using eina_thread_create(), this
 * function returns the Eina_Thread handle associated with it.
 * If the thread was not created by Eina (e.g., the main thread or a
 * thread created by other means), a new Thread_t structure is allocated,
 * initialized, and associated with the current native thread. This new
 * structure will be marked for automatic freeing (`free_on_exit = EINA_TRUE`)
 * when the thread exits, though the mechanism for this relies on `free_thread`
 * being called, which might not be guaranteed for non-Eina managed threads
 * without explicit TLS destructor support or manual cleanup.
 *
 * @return A handle to the calling thread. Returns @c NULL on failure (e.g. if
 *         `eina_thread_init` was not called and TLS allocation fails).
 */
EINA_API Eina_Thread
eina_thread_self(void)
{
    Thread_t *self = TlsGetValue(tls_thread_self);
    /*
     * If self is NULL this means:
     * 1) This function was called before eina_thread_init (for the main thread).
     * 2) This thread wasn't created by eina_thread_create (it's a foreign thread).
     *
     * In either case, we allocate a new Thread_t struct, associate it with
     * the current native thread, and return it. This allows foreign threads
     * to use Eina thread features like cancellation and cleanup handlers.
     */
    if (!self)
      {
         self = calloc(1, sizeof(*self));
         self->handle = GetCurrentThread();
         self->id = GetCurrentThreadId();
         self->free_on_exit = EINA_TRUE;
         self->cleanup_fn = eina_array_new(4);
         self->cleanup_arg = eina_array_new(4);
         if (tls_thread_self)
            TlsSetValue(tls_thread_self, self);
      }
    return (Eina_Thread) self;
}

/**
 * @brief Compare two thread handles.
 *
 * @param t1 The first thread handle.
 * @param t2 The second thread handle.
 * @return @c EINA_TRUE if the threads are the same, @c EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_thread_equal(Eina_Thread t1, Eina_Thread t2)
{
   return ((Thread_t *) t1)->id == ((Thread_t *) t2)->id;
}

/**
 * @brief Create a new thread.
 *
 * @param[out] t Pointer to store the new thread handle.
 * @param prio The priority of the new thread.
 * @param affinity The CPU affinity for the thread. If negative, no affinity is set.
 *                 The value is a bitmask, where `1 << N` means CPU N.
 * @param func The function to be executed by the new thread.
 * @param data User-defined data to be passed to @p func.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EINA_API Eina_Bool
eina_thread_create(Eina_Thread *t, Eina_Thread_Priority prio,
                   int affinity, Eina_Thread_Cb func, const void *data)
{
   Thread_t *thr = calloc(1, sizeof(Thread_t));
   if (!thr)
      return EINA_FALSE;

   thr->data = (void *) data;
   thr->fn = func;

   thr->handle = (HANDLE) _beginthreadex(NULL, 0, thread_fn, thr, CREATE_SUSPENDED, &thr->id);
   if (!thr->handle)
      goto fail;

   int priority;
   switch (prio)
     {
        case EINA_THREAD_URGENT:
           priority = THREAD_PRIORITY_HIGHEST;
           break;
        case EINA_THREAD_BACKGROUND:
           priority = THREAD_PRIORITY_BELOW_NORMAL;
           break;
        case EINA_THREAD_IDLE:
           priority = THREAD_PRIORITY_IDLE;
           break;
        default:
           priority = THREAD_PRIORITY_NORMAL;
     }

   if (!SetThreadPriority(thr->handle, priority))
      goto fail;

   if ((affinity >= 0) && (!SetThreadAffinityMask(thr->handle, 1 << affinity)))
      goto fail;

   thr->id = GetThreadId(thr->handle);
   if (!thr->id)
      goto fail;

   thr->cleanup_fn = eina_array_new(4);
   thr->cleanup_arg = eina_array_new(4);
   if ((!thr->cleanup_fn) || (!thr->cleanup_arg))
      goto fail;

   InitializeCriticalSection(&thr->cancel_lock);

   if (!ResumeThread(thr->handle))
      goto cs_fail;

   GetModuleFileNameA(NULL, thr->name, sizeof(thr->name));
   *t = (Eina_Thread) thr;
   return EINA_TRUE;

cs_fail:
   DeleteCriticalSection(&thr->cancel_lock);
fail:
   if (thr)
     {
        if (thr->handle) CloseHandle(thr->handle);
        if (thr->cleanup_fn) eina_array_free(thr->cleanup_fn);
        if (thr->cleanup_arg) eina_array_free(thr->cleanup_arg);
        free(thr);
     }
   return EINA_FALSE;
}

/**
 * @brief Wait for a thread to terminate.
 *
 * This function blocks the calling thread until the specified thread @p t
 * terminates. After the thread terminates, its resources are cleaned up.
 *
 * @param t The thread handle to wait for.
 * @return The return value of the thread function, or @c EINA_THREAD_JOIN_CANCELED
 *         if the thread was canceled, or @c NULL on error (e.g., if waiting fails).
 */
EINA_API void *
eina_thread_join(Eina_Thread t)
{
   void *data;
   Thread_t *thr = (Thread_t *) t;

   if (WAIT_OBJECT_0 == WaitForSingleObject(thr->handle, INFINITE))
      data = thr->data;
   else
      data = NULL;

   DeleteCriticalSection(&thr->cancel_lock);
   CloseHandle(thr->handle);
   eina_array_free(thr->cleanup_fn);
   eina_array_free(thr->cleanup_arg);
   free(thr);

   return data;
}

/**
 * @brief Set the name of a thread.
 *
 * This function sets a human-readable name for the given thread.
 * The name is truncated if it's longer than 15 characters.
 * This is primarily used for debugging purposes.
 *
 * @param t The thread handle.
 * @param name The name to set for the thread.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
EINA_API Eina_Bool
eina_thread_name_set(Eina_Thread t, const char *name)
{
   Thread_t *thr = (Thread_t *) t;
   strncpy(thr->name, name, sizeof(thr->name));
   thr->name[sizeof(thr->name)-1] = '\0';
   return EINA_TRUE;
}

/**
 * @brief Request cancellation of a thread.
 *
 * This function sets a flag requesting that the specified thread @p t should
 * cancel its execution. The thread itself must periodically call
 * eina_thread_cancel_checkpoint() to check this flag and act upon it.
 *
 * @param t The thread handle to cancel.
 * @return @c EINA_TRUE if the cancellation request was successfully made
 *         (i.e., the thread was cancellable), @c EINA_FALSE otherwise (e.g., if @p t is NULL
 *         or the thread is not currently cancellable).
 */
EINA_API Eina_Bool
eina_thread_cancel(Eina_Thread t)
{
    Eina_Bool ret = EINA_FALSE;
    Thread_t *thr = (Thread_t *) t;

    if (thr)
      {
         EnterCriticalSection(&thr->cancel_lock);
         if (thr->cancellable)
           {
              thr->cancel = EINA_TRUE;
              ret = EINA_TRUE;
           }
         LeaveCriticalSection(&thr->cancel_lock);
      }
    return ret;
}

/**
 * @brief Set the cancellability state of the calling thread.
 *
 * This function allows a thread to define sections of code where it can
 * or cannot be canceled.
 *
 * @param cancellable If @c EINA_TRUE, the thread can be canceled.
 *                    If @c EINA_FALSE, cancellation requests are ignored.
 * @param[out] was_cancellable Optional pointer to store the previous cancellability state.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
EINA_API Eina_Bool
eina_thread_cancellable_set(Eina_Bool cancellable, Eina_Bool *was_cancellable)
{
   Thread_t *t = (Thread_t *) eina_thread_self();

   EnterCriticalSection(&t->cancel_lock);
   if (was_cancellable) *was_cancellable = t->cancellable;
   t->cancellable = cancellable;
   LeaveCriticalSection(&t->cancel_lock);

   return EINA_TRUE;
}

/**
 * @brief Check if a cancellation request is pending for the calling thread.
 *
 * If the thread is cancellable and a cancellation request has been made
 * (via eina_thread_cancel()), this function will execute all registered
 * cleanup handlers and then terminate the calling thread. Otherwise, it
 * does nothing.
 *
 * This function should be called periodically in long-running computations
 * or blocking operations within a thread to allow for timely cancellation.
 */
EINA_API void
eina_thread_cancel_checkpoint(void)
{
   Eina_Bool cancel;
   Thread_t *t = (Thread_t *) eina_thread_self();

   EnterCriticalSection(&t->cancel_lock);
   cancel = t->cancellable && t->cancel;
   LeaveCriticalSection(&t->cancel_lock);

   if (cancel)
     {
        t->data = (void *) EINA_THREAD_JOIN_CANCELED;
        while (eina_array_count(t->cleanup_fn))
          {
             Eina_Thread_Cleanup_Cb fn = (Eina_Thread_Cleanup_Cb) eina_array_pop(t->cleanup_fn);
             void *arg = eina_array_pop(t->cleanup_arg);

             if (fn)
               fn(arg);
          }

        ExitThread(0);
     }
}

/**
 * @brief Push a cleanup handler onto the calling thread's cleanup stack.
 *
 * Cleanup handlers are functions that are called when a thread exits,
 * either normally or due to cancellation (via eina_thread_cancel_checkpoint()).
 * They are called in LIFO (Last-In, First-Out) order.
 *
 * @param fn The cleanup handler function.
 * @param data User-defined data to be passed to the cleanup handler.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., memory allocation error).
 *
 * @par Example
 * @code
 * FILE *f = fopen("example.txt", "w");
 * if (f) {
 *     eina_thread_cleanup_push(fclose, f);
 *     // ... do operations with f ...
 *     // If thread is cancelled here, fclose(f) will be called.
 *     eina_thread_cleanup_pop(EINA_TRUE); // fclose(f) called here if not cancelled.
 * }
 * @endcode
 */
EINA_API Eina_Bool
eina_thread_cleanup_push(Eina_Thread_Cleanup_Cb fn, void *data)
{
   Thread_t *t = TlsGetValue(tls_thread_self);
   assert(t); // Should always have a Thread_t via eina_thread_self or thread_fn

   // The cleanup_fn array stores Eina_Thread_Cleanup_Cb function pointers.
   // Example: [fclose_ptr, free_ptr, custom_cleanup_ptr]
   if (!eina_array_push(t->cleanup_fn, fn))
      return EINA_FALSE;

   // The cleanup_arg array stores void* arguments corresponding to each function.
   // Example: [file_handle_ptr, memory_block_ptr, custom_data_ptr]
   if (!eina_array_push(t->cleanup_arg, data))
     {
        eina_array_pop(t->cleanup_fn); // Rollback push to cleanup_fn
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Pop a cleanup handler from the calling thread's cleanup stack.
 *
 * @param execute If non-zero, the popped cleanup handler is executed.
 *                If zero, the handler is simply removed without execution.
 *
 * @par Example
 * @code
 * // Assuming a handler was pushed with: eina_thread_cleanup_push(my_cleanup_func, my_data);
 *
 * // To execute and remove:
 * eina_thread_cleanup_pop(EINA_TRUE);
 *
 * // To remove without executing:
 * eina_thread_cleanup_pop(EINA_FALSE);
 * @endcode
 */
EINA_API void
eina_thread_cleanup_pop(int execute)
{
   Thread_t *t = TlsGetValue(tls_thread_self);
   assert(t); // Should always have a Thread_t

   if (eina_array_count(t->cleanup_fn))
     {
        Eina_Thread_Cleanup_Cb fn = (Eina_Thread_Cleanup_Cb) eina_array_pop(t->cleanup_fn);
        void *arg = eina_array_pop(t->cleanup_arg);

        if (execute && fn)
           fn(arg);
     }
}

/**
 * @brief A special value returned by eina_thread_join() if the joined thread
 * was canceled.
 */
EINA_API const void *EINA_THREAD_JOIN_CANCELED = (void *) -1L;

/**
 * @internal
 * @brief Lowers the scheduling priority of the calling thread.
 *
 * This function attempts to make the current thread less favored by the
 * scheduler. If the thread is a real-time thread (THREAD_PRIORITY_TIME_CRITICAL),
 * its priority is decreased by `RTNICENESS`. Otherwise, its "niceness" is
 * increased (priority decreased) by `NICENESS`.
 *
 * This function is not part of the public Eina API and is likely used
 * internally for specific scheduling adjustments.
 */
void
eina_sched_prio_drop(void)
{
   Thread_t *thread;
   int sched_priority;

   thread = (Thread_t *) eina_thread_self();

   sched_priority = GetThreadPriority(thread->handle);

   if (EINA_UNLIKELY(sched_priority == THREAD_PRIORITY_TIME_CRITICAL))
     {
        sched_priority -= RTNICENESS;

        /* We don't change the policy */
        if (sched_priority < 1)
          {
             EINA_LOG_INFO("RT prio < 1, setting to 1 instead");
             sched_priority = 1;
          }
        if (!SetThreadPriority(thread->handle, sched_priority))
          {
             EINA_LOG_ERR("Unable to query sched parameters");
          }
     }
   else
     {
        sched_priority += NICENESS;

        /* We don't change the policy */
        if (sched_priority > THREAD_PRIORITY_TIME_CRITICAL)
          {
             EINA_LOG_INFO("Max niceness reached; keeping max (THREAD_PRIORITY_TIME_CRITICAL)");
             sched_priority = THREAD_PRIORITY_TIME_CRITICAL;
          }
        if (!SetThreadPriority(thread->handle, sched_priority))
          {
             EINA_LOG_ERR("Unable to query sched parameters");
          }
     }
}

/**
 * @brief Initialize the Eina thread system.
 *
 * This function must be called from the main thread before any other
 * Eina thread functions are used. It sets up thread-local storage for
 * managing Eina_Thread handles and initializes the main thread's
 * Thread_t structure.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *         Failure can occur if not called from the main loop context
 *         or if TLS allocation fails.
 */
EINA_API Eina_Bool
eina_thread_init(void)
{
   if (!eina_main_loop_is()) // Ensures this is called in a context where Eina's main loop is recognized
      return EINA_FALSE;

   tls_thread_self = TlsAlloc(); // Allocate a TLS index
   if (TLS_OUT_OF_INDEXES == tls_thread_self)
      return EINA_FALSE;

   if (!TlsSetValue(tls_thread_self, &main_thread))
     {
        assert(0);
        TlsFree(tls_thread_self);
        return EINA_FALSE;
     }

   main_thread.cancellable = EINA_FALSE;
   main_thread.cancel = EINA_FALSE;
   main_thread.handle = GetCurrentThread();
   main_thread.id = GetCurrentThreadId();

   InitializeCriticalSection(&main_thread.cancel_lock);
   main_thread.cleanup_fn = eina_array_new(2);
   main_thread.cleanup_arg = eina_array_new(2);

   GetModuleFileNameA(NULL, main_thread.name, sizeof(main_thread.name)/sizeof(main_thread.name[0]));

   return EINA_TRUE;
}

/**
 * @brief Shut down the Eina thread system.
 *
 * This function should be called from the main thread when Eina thread
 * support is no longer needed, typically during application shutdown.
 * It releases resources allocated by eina_thread_init(), such as
 * the TLS index and resources associated with the main thread's
 * Thread_t structure.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
EINA_API Eina_Bool
eina_thread_shutdown(void)
{
   DeleteCriticalSection(&main_thread.cancel_lock);
   eina_array_free(main_thread.cleanup_fn);
   eina_array_free(main_thread.cleanup_arg);
   TlsFree(tls_thread_self);
   return EINA_TRUE;
}
