/* EINA - EFL data type library
 * Copyright (C) 2017 Carsten Haitzler
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

# ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
# endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#ifdef HAVE_SYS_EPOLL_H
# include <sys/epoll.h>
#endif
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "eina_debug.h"
#include "eina_debug_private.h"

static Eina_Spinlock _lock; /**< Spinlock to protect access to shared timer data. */

/**
 * @internal
 * @struct _Eina_Debug_Timer
 * @brief Represents a timer for debugging purposes.
 *
 * This structure holds information about a single debug timer, including its
 * relative time, total timeout, callback function, and associated data.
 */
struct _Eina_Debug_Timer
{
   unsigned int rel_time;       /**< Relative time in milliseconds until this timer fires, based on the previous timer in the sorted list. */
   unsigned int timeout;        /**< Absolute timeout in milliseconds from the start or last firing. */
   Eina_Debug_Timer_Cb cb;      /**< Callback function to execute when the timer expires. */
   void *data;                 /**< User data to pass to the callback function. */
};

static Eina_List *_timers = NULL; /**< List of active debug timers, sorted by timeout. */

static Eina_Bool _thread_runs = EINA_FALSE; /**< Flag indicating if the monitor thread is currently running. */
static pthread_t _thread; /**< Identifier for the monitor thread. */

static int pipeToThread[2]; /**< Pipe used to communicate with the monitor thread, typically to wake it up. pipeToThread[0] is read end, pipeToThread[1] is write end. */

/**
 * @internal
 * @brief Appends a new timer to the sorted list of timers.
 *
 * The timer is inserted into the `_timers` list based on its `timeout` value,
 * maintaining a sorted order. After insertion, the monitor thread is signaled
 * via `pipeToThread` to re-evaluate its wait time.
 *
 * @param t The timer to append.
 */
static void
_timer_append(Eina_Debug_Timer *t)
{
   Eina_Debug_Timer *t2;
   Eina_List *itr;
   unsigned int prev_time = 0;
   char c = '\0';
   EINA_LIST_FOREACH(_timers, itr, t2)
     {
        if (t2->timeout > t->timeout) goto end;
        prev_time = t2->timeout;
     }
   t2 = NULL;
end:
   t->rel_time = t->timeout - prev_time;
   if (!t2) _timers = eina_list_append(_timers, t);
   else _timers = eina_list_prepend_relative(_timers, t, t2);
   if (write(pipeToThread[1], &c, 1) != 1)
     e_debug("EINA DEBUG ERROR: Can't wake up thread for debug timer");
}

/**
 * @internal
 * @brief Main function for the timer monitor thread.
 *
 * This thread waits for timer events or signals to update the timer list.
 * It uses `epoll` (if available) to wait on a pipe for notifications
 * or until the next timer is due. When a timer expires, its callback
 * is executed. If the callback returns `EINA_TRUE`, the timer is rescheduled.
 *
 * @param _data Unused thread data.
 * @return NULL when the thread exits.
 */
static void *
_monitor(void *_data EINA_UNUSED)
{
#ifdef HAVE_SYS_EPOLL_H
#define MAX_EVENTS   4
   struct epoll_event event;
   struct epoll_event events[MAX_EVENTS];
   int epfd = epoll_create(MAX_EVENTS), ret;

   event.data.fd = pipeToThread[0];
   event.events = EPOLLIN;
   ret = epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);
   if (ret) perror("epoll_ctl/add");
#ifdef EINA_HAVE_PTHREAD_SETNAME
# ifndef __linux__
   pthread_set_name_np
# else
   pthread_setname_np
# endif
     (pthread_self(), "Edbg-tim");
#endif
   while (1)
     {
        int timeout = -1; //in milliseconds
        pthread_testcancel();
        eina_spinlock_take(&_lock);
        if (_timers)
          {
             Eina_Debug_Timer *t = eina_list_data_get(_timers);
             timeout = t->timeout;
          }
        eina_spinlock_release(&_lock);

        ret = epoll_wait(epfd, events, MAX_EVENTS, timeout);
        pthread_testcancel();

        /* Some timer has been add/removed or we need to exit */
        if (ret)
          {
             char c;
             if (read(pipeToThread[0], &c, 1) != 1) break;
          }
        else
          {
             Eina_List *itr, *itr2, *renew = NULL;
             Eina_Debug_Timer *t;
             eina_spinlock_take(&_lock);
             EINA_LIST_FOREACH_SAFE(_timers, itr, itr2, t)
               {
                  if (itr == _timers || t->rel_time == 0)
                    {
                       _timers = eina_list_remove(_timers, t);
                       if (t->cb(t->data)) renew = eina_list_append(renew, t);
                       else free(t);
                    }
               }
             EINA_LIST_FREE(renew, t) _timer_append(t);
             eina_spinlock_release(&_lock);
          }
     }
#endif
   _thread_runs = EINA_FALSE;
   close(pipeToThread[0]);
   close(pipeToThread[1]);
   return NULL;
}

/**
 * @brief Adds a new debug timer.
 *
 * Creates and adds a new timer that will execute the given callback function
 * after the specified timeout. If this is the first timer or the monitor thread
 * is not running, the thread is created.
 *
 * @param timeout_ms The timeout in milliseconds. Must be greater than 0.
 * @param cb The callback function to execute when the timer expires. Must not be NULL.
 *           The callback should return #EINA_TRUE to reschedule the timer with the
 *           same timeout, or #EINA_FALSE to remove it.
 * @param data User data to be passed to the callback function.
 * @return A pointer to the newly created Eina_Debug_Timer on success, or NULL on failure
 *         (e.g., if `cb` is NULL or `timeout_ms` is 0).
 *
 * @note The timer system uses a dedicated thread to manage timers.
 *       Signal handling is configured in this thread to avoid interference.
 *
 * Example:
 * @code
 * Eina_Bool my_timer_callback(void *data)
 * {
 *     printf("Timer fired with data: %s\n", (char *)data);
 *     return EINA_FALSE; // Do not repeat
 * }
 *
 * Eina_Debug_Timer *timer = eina_debug_timer_add(1000, my_timer_callback, "hello world");
 * if (!timer)
 * {
 *     fprintf(stderr, "Failed to add timer\n");
 * }
 * @endcode
 */
EINA_API Eina_Debug_Timer *
eina_debug_timer_add(unsigned int timeout_ms, Eina_Debug_Timer_Cb cb, void *data)
{
   if (!cb || !timeout_ms) return NULL;
   Eina_Debug_Timer *t = calloc(1, sizeof(*t));
   t->cb = cb;
   t->data = data;
   t->timeout = timeout_ms;
   eina_spinlock_take(&_lock);
   _timer_append(t);
   if (!_thread_runs)
     {
#ifndef _WIN32
        sigset_t oldset, newset;

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
#endif
        int err = pthread_create(&_thread, NULL, _monitor, NULL);
#ifndef _WIN32
        pthread_sigmask(SIG_SETMASK, &oldset, NULL);
#endif
        if (err != 0)
          {
             e_debug("EINA DEBUG ERROR: Can't create debug timer thread!");
             abort();
          }
        _thread_runs = EINA_TRUE;
     }
   eina_spinlock_release(&_lock);
   return t;
}

/**
 * @brief Deletes a debug timer.
 *
 * Removes the specified timer from the active list and frees its resources.
 * If the timer is not found in the list, this function does nothing.
 *
 * @param t The timer to delete. If NULL, the function does nothing.
 *
 * Example:
 * @code
 * Eina_Debug_Timer *timer = eina_debug_timer_add(1000, my_callback, NULL);
 * // ... later ...
 * if (timer)
 * {
 *     eina_debug_timer_del(timer);
 *     timer = NULL; // Good practice to NULLify dangling pointers
 * }
 * @endcode
 */
EINA_API void
eina_debug_timer_del(Eina_Debug_Timer *t)
{
   eina_spinlock_take(&_lock);
   Eina_List *itr = eina_list_data_find_list(_timers, t);
   if (itr)
     {
        _timers = eina_list_remove_list(_timers, itr);
        free(t);
     }
   eina_spinlock_release(&_lock);
}

/**
 * @internal
 * @brief Initializes the debug timer system.
 *
 * Sets up the spinlock and the communication pipe for the monitor thread.
 * This function is called during Eina library initialization.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure (e.g., pipe creation failed).
 */
Eina_Bool
_eina_debug_timer_init(void)
{
   eina_spinlock_new(&_lock);
#ifndef _WIN32
   if (pipe(pipeToThread) == -1)
     return  EINA_FALSE;
#endif
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the debug timer system.
 *
 * Cleans up all active timers, cancels the monitor thread if it's running,
 * closes the communication pipe, and frees the spinlock.
 * This function is called during Eina library shutdown.
 *
 * @return #EINA_TRUE always.
 */
Eina_Bool
_eina_debug_timer_shutdown(void)
{
   Eina_Debug_Timer *t;

   eina_spinlock_take(&_lock);
   EINA_LIST_FREE(_timers, t)
     free(t);
   close(pipeToThread[0]);
   close(pipeToThread[1]);
   if (_thread_runs)
     pthread_cancel(_thread);
   _thread_runs = 0;
   eina_spinlock_release(&_lock);
   eina_spinlock_free(&_lock);

   return EINA_TRUE;
}

