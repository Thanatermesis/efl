/* EINA - EFL data type library
 * Copyright (C) 2012 Cedric Bail
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

#include <stdlib.h>

#include "eina_config.h"
#include "eina_lock.h" /* it will include pthread.h with proper flags */
#include "eina_thread.h"
#include "eina_cpu.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"

#include "eina_debug_private.h"

#include <pthread.h>
#include <errno.h>
#ifndef _WIN32
# include <signal.h>
#endif
# include <string.h>

#if defined(EINA_HAVE_PTHREAD_AFFINITY) || defined(EINA_HAVE_PTHREAD_SETNAME)
#ifndef __linux__
#include <pthread_np.h>
#define cpu_set_t cpuset_t
#endif
#endif

#ifdef __linux__
# include <sched.h>
# include <sys/time.h>
# include <sys/resource.h>
#endif

#include "eina_log.h"

#define RTNICENESS 1
#define NICENESS 5

/**
 * @internal
 * @brief Waits for a thread to terminate.
 * This is a wrapper around pthread_join.
 * @param t The thread to wait for.
 * @return The value passed to pthread_exit() by the terminated thread.
 *         Returns NULL on error.
 */
static inline void *
_eina_thread_join(Eina_Thread t)
{
   void *ret = NULL;
   int err = pthread_join((pthread_t)t, &ret);

   if (err == 0) return ret;
   return NULL;
}

/**
 * @internal
 * @brief Creates a new thread.
 * This is a wrapper around pthread_create, with additional handling
 * for thread affinity and signal masking.
 * @param t Pointer to an Eina_Thread variable, which will store the ID of the new thread.
 * @param affinity The CPU core to which the thread should be affinitized.
 *                 If negative, no affinity is set.
 * @param func The function to be executed by the new thread.
 * @param data A pointer to data that will be passed to the thread function.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 */
static inline Eina_Bool
_eina_thread_create(Eina_Thread *t, int affinity, void *(*func)(void *data), void *data)
{
   int err;
   pthread_attr_t attr;
   sigset_t oldset, newset;

   if (pthread_attr_init(&attr) != 0)
     {
        return EINA_FALSE;
     }
   if (affinity >= 0)
     {
#ifdef EINA_HAVE_PTHREAD_AFFINITY
        cpu_set_t cpu;

        CPU_ZERO(&cpu);
        CPU_SET(affinity, &cpu);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu), &cpu);
#endif
     }

   /* setup initial locks */
   sigemptyset(&newset);
   sigaddset(&newset, SIGPIPE);
   sigaddset(&newset, SIGALRM);
   sigaddset(&newset, SIGCHLD);
   sigaddset(&newset, SIGUSR1);
   sigaddset(&newset, SIGUSR2);
   sigaddset(&newset, SIGHUP);
   sigaddset(&newset, SIGQUIT);
   sigaddset(&newset, SIGINT);
   sigaddset(&newset, SIGTERM);
# ifdef SIGPWR
   sigaddset(&newset, SIGPWR);
# endif
   pthread_sigmask(SIG_BLOCK, &newset, &oldset);
   err = pthread_create((pthread_t *)t, &attr, func, data);
   pthread_sigmask(SIG_SETMASK, &oldset, NULL);
   pthread_attr_destroy(&attr);

   if (err == 0) return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Compares two thread IDs.
 * This is a wrapper around pthread_equal.
 * @param t1 The first thread ID.
 * @param t2 The second thread ID.
 * @return EINA_TRUE if the thread IDs are equal, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_eina_thread_equal(Eina_Thread t1, Eina_Thread t2)
{
   return pthread_equal((pthread_t)t1, (pthread_t)t2);
}

/**
 * @internal
 * @brief Gets the ID of the calling thread.
 * This is a wrapper around pthread_self.
 * @return The ID of the calling thread.
 */
static inline Eina_Thread
_eina_thread_self(void)
{
   return (Eina_Thread)pthread_self();
}

/**
 * @internal
 * @struct _Eina_Thread_Call
 * @brief Structure to hold data for the internal thread call.
 *
 * This structure is used to pass the user's callback function, data,
 * priority, and affinity settings to the newly created thread.
 */
typedef struct _Eina_Thread_Call Eina_Thread_Call;
struct _Eina_Thread_Call
{
   Eina_Thread_Cb func; /**< The user-provided callback function. */
   const void *data; /**< The user-provided data for the callback. */

   Eina_Thread_Priority prio; /**< The priority for the new thread. */
   int affinity; /**< The CPU affinity for the new thread. */
};

/**
 * @internal
 * @brief Internal function executed by newly created Eina threads.
 * This function sets up the thread environment (cancellability, priority),
 * calls the user-provided callback, and handles cleanup.
 * @param context A pointer to an Eina_Thread_Call structure containing
 *                the callback function and its arguments.
 * @return The return value of the user-provided callback function.
 */
static void *
_eina_internal_call(void *context)
{
   Eina_Thread_Call *c = context;
   void *r;
   pthread_t self;

   // Default this thread to not cancellable as per Eina documentation
   eina_thread_cancellable_set(EINA_FALSE, NULL);

   EINA_THREAD_CLEANUP_PUSH(free, c);

   self = pthread_self();

   if (c->prio == EINA_THREAD_IDLE)
     {
        struct sched_param params;
        int min;
#ifdef SCHED_IDLE
        int pol = SCHED_IDLE;
#else
        int pol;
        pthread_getschedparam(self, &pol, &params);
#endif
        min = sched_get_priority_min(pol);
        params.sched_priority = min;
        pthread_setschedparam(self, pol, &params);
     }
   else if (c->prio == EINA_THREAD_BACKGROUND)
     {
        struct sched_param params;
        int min, max;
#ifdef SCHED_BATCH
        int pol = SCHED_BATCH;
#else
        int pol;
        pthread_getschedparam(self, &pol, &params);
#endif
        min = sched_get_priority_min(pol);
        max = sched_get_priority_max(pol);
        params.sched_priority = (max - min) / 2;
        pthread_setschedparam(self, pol, &params);
     }
// do nothing for normal
//   else if (c->prio == EINA_THREAD_NORMAL)
//     {
//     }
   else if (c->prio == EINA_THREAD_URGENT)
     {
        struct sched_param params;
        int max, pol;

        pthread_getschedparam(self, &pol, &params);
        max = sched_get_priority_max(pol);
        params.sched_priority += 5;
        if (params.sched_priority > max) params.sched_priority = max;
        pthread_setschedparam(self, pol, &params);
     }

   _eina_debug_thread_add(&self);
   EINA_THREAD_CLEANUP_PUSH(_eina_debug_thread_del, &self);
   r = c->func((void*) c->data, eina_thread_self());
   EINA_THREAD_CLEANUP_POP(EINA_TRUE);
   EINA_THREAD_CLEANUP_POP(EINA_TRUE);

   return r;
}

/**
 * @brief Get the ID of the calling thread.
 * @return The ID of the calling thread.
 * @see pthread_self()
 *
 * This function returns the Eina_Thread ID of the calling thread.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Thread
eina_thread_self(void)
{
   return _eina_thread_self();
}

/**
 * @brief Compare two thread IDs.
 * @param t1 The first thread ID.
 * @param t2 The second thread ID.
 * @return EINA_TRUE if the thread IDs refer to the same thread,
 *         EINA_FALSE otherwise.
 * @see pthread_equal()
 *
 * This function checks if two Eina_Thread IDs refer to the same thread.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_equal(Eina_Thread t1, Eina_Thread t2)
{
   return !!_eina_thread_equal(t1, t2);
}

/**
 * @brief Create a new thread.
 * @param t Pointer to an Eina_Thread variable, which will store the ID of
 *        the new thread upon successful creation.
 * @param prio The priority of the new thread. See #Eina_Thread_Priority.
 * @param affinity The CPU core to which the thread should be affinitized.
 *                 A value less than 0 means no specific affinity.
 * @param func The function to be executed by the new thread. This function
 *             will receive the @p data pointer and the new thread's ID as arguments.
 * @param data A pointer to data that will be passed to the @p func.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @see pthread_create()
 *
 * This function creates a new thread that will execute the given @p func
 * with @p data. The thread's priority and CPU affinity can be specified.
 * The new thread is created with deferred cancellation type and cancellability
 * initially disabled.
 *
 * Example:
 * @code
 * void *my_thread_func(void *data, Eina_Thread thread_id)
 * {
 *    printf("Hello from thread %p with data: %s\n", thread_id, (char *)data);
 *    return NULL;
 * }
 *
 * Eina_Thread my_thread;
 * if (!eina_thread_create(&my_thread, EINA_THREAD_NORMAL, -1, my_thread_func, "my_data"))
 * {
 *    fprintf(stderr, "Error creating thread\n");
 * }
 * else
 * {
 *    // Thread created successfully
 *    eina_thread_join(my_thread); // Wait for it to finish
 * }
 * @endcode
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_create(Eina_Thread *t,
                   Eina_Thread_Priority prio, int affinity,
                   Eina_Thread_Cb func, const void *data)
{
   Eina_Thread_Call *c;

   EINA_SAFETY_ON_NULL_RETURN_VAL(t, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, EINA_FALSE);

   c = malloc(sizeof (Eina_Thread_Call));
   if (!c) return EINA_FALSE;

   c->func = func;
   c->data = data;
   c->prio = prio;
   c->affinity = affinity;

   // valgrind complains c is lost - but it's not - it is handed to the
   // child thread to be freed when c->func returns in _eina_internal_call().
   if (_eina_thread_create(t, affinity, _eina_internal_call, c))
     return EINA_TRUE;

   free(c);
   return EINA_FALSE;
}

/**
 * @brief Wait for a thread to terminate.
 * @param t The thread to wait for.
 * @return The value passed to pthread_exit() or returned by the thread function.
 *         Returns #EINA_THREAD_JOIN_CANCELED if the thread was canceled.
 *         Returns NULL on other errors (e.g., invalid thread ID).
 * @see pthread_join()
 *
 * This function blocks the calling thread until the specified thread @p t
 * terminates.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API void *
eina_thread_join(Eina_Thread t)
{
   return _eina_thread_join(t);
}

/**
 * @brief Set the name of a thread.
 * @param t The thread whose name is to be set.
 * @param name The new name for the thread. The name is usually truncated
 *             to a system-defined limit (e.g., 15 characters on Linux).
 *             If NULL or empty, the thread name might be cleared or set to empty,
 *             depending on the system.
 * @return EINA_TRUE on success, EINA_FALSE on error or if the feature is
 *         not supported on the current platform.
 * @see pthread_setname_np()
 *
 * This function attempts to set the name of the specified thread. This can be
 * useful for debugging. The actual behavior and support for this feature
 * depend on the operating system.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_name_set(Eina_Thread t, const char *name)
{
#ifdef EINA_HAVE_PTHREAD_SETNAME
   char buf[16];
   if (name)
     {
        strncpy(buf, name, 15);
        buf[15] = 0;
     }
   else buf[0] = 0;
#ifndef __linux__
   pthread_set_name_np((pthread_t)t, buf);
   return EINA_TRUE;
#else
   if (pthread_setname_np((pthread_t)t, buf) == 0) return EINA_TRUE;
#endif
#else
   (void)t;
   (void)name;
#endif
   return EINA_FALSE;
}

/**
 * @brief Request cancellation of a thread.
 * @param t The thread to be canceled.
 * @return EINA_TRUE on success, EINA_FALSE on error (e.g., invalid thread ID).
 * @see pthread_cancel()
 *
 * This function sends a cancellation request to the specified thread @p t.
 * Whether and when the thread is actually canceled depends on its
 * cancellability state and type, and whether it calls a cancellation point.
 * By default, Eina threads are created with cancellability disabled.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_cancel(Eina_Thread t)
{
   if (!t) return EINA_FALSE;
   return pthread_cancel((pthread_t)t) == 0;
}

/**
 * @brief Set the cancellability state of the calling thread.
 * @param cancellable If EINA_TRUE, enable cancellation; if EINA_FALSE, disable it.
 * @param was_cancellable If not NULL, this will be set to the previous
 *                        cancellability state (EINA_TRUE if was enabled,
 *                        EINA_FALSE if was disabled).
 * @return EINA_TRUE on success, EINA_FALSE on error.
 * @see pthread_setcancelstate()
 * @see pthread_setcanceltype()
 *
 * This function sets the cancellability state of the calling thread.
 * It also ensures that the cancellation type is set to PTHREAD_CANCEL_DEFERRED,
 * which is the Eina default.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_cancellable_set(Eina_Bool cancellable, Eina_Bool *was_cancellable)
{
   int state = cancellable ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE;
   int old = 0;
   int r;

   /* enforce deferred in case users changed to asynchronous themselves */
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old);

   r = pthread_setcancelstate(state, &old);
   if (was_cancellable && r == 0)
     *was_cancellable = (old == PTHREAD_CANCEL_ENABLE);

   return r == 0;
}

/**
 * @brief Create a cancellation point in the calling thread.
 * @see pthread_testcancel()
 *
 * If thread cancellation is enabled and a cancellation request is pending
 * for the calling thread, this function will cause the thread to terminate.
 * If cancellation is disabled or no request is pending, this function has no effect.
 * This should be called periodically in long-running computations or blocking
 * operations within a cancellable thread to ensure timely response to cancellation
 * requests.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API void
eina_thread_cancel_checkpoint(void)
{
   pthread_testcancel();
}

/**
 * @brief Value returned by eina_thread_join() if the joined thread was canceled.
 * @see PTHREAD_CANCELED
 * @ingroup Eina_Thread_Group
 */
EINA_API const void *EINA_THREAD_JOIN_CANCELED = PTHREAD_CANCELED;

/**
 * @brief Lower the scheduling priority of the current thread.
 *
 * This function attempts to lower the scheduling priority of the calling thread.
 * For real-time scheduling policies (SCHED_RR, SCHED_FIFO), it decreases the
 * priority by #RTNICENESS. For other policies (e.g., SCHED_OTHER on Linux),
 * it increases the "nice" value by #NICENESS, effectively lowering its priority.
 * The exact behavior is system-dependent.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API void
eina_sched_prio_drop(void)
{
   struct sched_param param;
   int pol, ret;
   pthread_t pthread_id;

   pthread_id = pthread_self();
   ret = pthread_getschedparam(pthread_id, &pol, &param);
   if (ret)
     {
        EINA_LOG_ERR("Unable to query sched parameters");
        return;
     }

   if (EINA_UNLIKELY(pol == SCHED_RR || pol == SCHED_FIFO))
     {
        param.sched_priority -= RTNICENESS;

        /* We don't change the policy */
        if (param.sched_priority < 1)
          {
             EINA_LOG_INFO("RT prio < 1, setting to 1 instead");
             param.sched_priority = 1;
          }

        pthread_setschedparam(pthread_id, pol, &param);
     }
# ifdef __linux__
   else
     {
        int prio;
        errno = 0;
        prio = getpriority(PRIO_PROCESS, 0);
        if (errno == 0)
          {
             prio += NICENESS;
             if (prio > 19)
               {
                  EINA_LOG_INFO("Max niceness reached; keeping max (19)");
                  prio = 19;
               }

             setpriority(PRIO_PROCESS, 0, prio);
          }
     }
# endif
}

/**
 * @brief Initialize the Eina thread module.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function initializes the Eina threading subsystem.
 * Currently, this function does nothing and always returns EINA_TRUE,
 * as pthreads typically do not require explicit library-level initialization
 * beyond what the system provides.
 * It is provided for consistency and future extensibility.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_init(void)
{
   return EINA_TRUE;
}

/**
 * @brief Shut down the Eina thread module.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function shuts down the Eina threading subsystem.
 * Currently, this function does nothing and always returns EINA_TRUE,
 * as pthreads typically do not require explicit library-level cleanup
 * that Eina would manage globally.
 * It is provided for consistency and future extensibility.
 *
 * @ingroup Eina_Thread_Group
 */
EINA_API Eina_Bool
eina_thread_shutdown(void)
{
   return EINA_TRUE;
}
