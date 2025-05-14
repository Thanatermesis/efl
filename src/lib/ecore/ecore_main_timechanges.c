#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#include "Ecore.h"
#include "ecore_private.h"
#include "ecore_main_common.h"

#ifdef HAVE_SYS_TIMERFD_H

/**
 * @brief Stores the Efl_Loop_Handler object used for monitoring CLOCK_REALTIME.
 * This is used to detect system time changes. It is NULL if not currently active.
 */
static Eo *realtime_obj = NULL;

/**
 * @brief Callback function triggered when the timerfd for CLOCK_REALTIME has data to read.
 * This indicates a potential system time change.
 *
 * @param data User data, unused in this callback.
 * @param event The Efl_Event structure containing event information.
 */
static void
_cb_read(void *data EINA_UNUSED, const Efl_Event *event)
{
   Eo *loop = efl_provider_find(event->object, EFL_LOOP_CLASS);
   int fd = efl_loop_handler_fd_get(event->object);
   char buf[8];

   if (read(fd, buf, sizeof(buf)) >= 0) return;
   DBG("system clock changed");
   // XXX: ecore event needs to be eo loop api's
   ecore_event_add(ECORE_EVENT_SYSTEM_TIMEDATE_CHANGED, NULL, NULL, NULL);
   _ecore_main_timechanges_stop(loop);
   _ecore_main_timechanges_start(loop);
}

/**
 * @brief Callback function triggered when the Efl_Loop_Handler object is deleted.
 * This function closes the associated file descriptor and resets realtime_obj if it matches.
 *
 * @param data User data, unused in this callback.
 * @param event The Efl_Event structure containing event information.
 */
static void
_cb_del(void *data EINA_UNUSED, const Efl_Event *event)
{
   int fd = efl_loop_handler_fd_get(event->object);
   if (event->object == realtime_obj) realtime_obj = NULL;
   close(fd);
}

EFL_CALLBACKS_ARRAY_DEFINE(_event_watch,
                           { EFL_LOOP_HANDLER_EVENT_READ, _cb_read },
                           { EFL_EVENT_DEL, _cb_del });
#endif

/**
 * @brief Starts monitoring for system time changes.
 *
 * This function creates a timerfd for CLOCK_REALTIME. The timer is set to
 * expire at a very distant point in the future (TIME_T_MAX). However, if
 * the system clock is changed (e.g., by NTP or manually), the timerfd
 * becomes readable. This event is then used to signal a system time change.
 *
 * @param obj The Efl_Loop object to associate the handler with.
 */
void
_ecore_main_timechanges_start(Eo *obj)
{
#ifdef HAVE_SYS_TIMERFD_H
   struct itimerspec its;
   int fd;

   if (realtime_obj) return;

   fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
   if (fd < 0) return;

   memset(&its, 0, sizeof(its));
   its.it_value.tv_sec = TIME_T_MAX; // end of time - 0xf
   if (timerfd_settime(fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET,
                       &its, NULL) < 0)
     {
        WRN("Couldn't arm timerfd to detect clock changes: %s", strerror(errno));
        close(fd);
        return;
     }
   realtime_obj =
     efl_add(EFL_LOOP_HANDLER_CLASS, obj,
             efl_loop_handler_fd_set(efl_added, fd),
             efl_loop_handler_active_set(efl_added, EFL_LOOP_HANDLER_FLAGS_READ),
             efl_event_callback_array_add(efl_added, _event_watch(), NULL));
#endif
}

/**
 * @brief Stops monitoring for system time changes.
 *
 * This function deletes the Efl_Loop_Handler object that was created by
 * _ecore_main_timechanges_start(), effectively stopping the monitoring
 * of CLOCK_REALTIME for changes.
 *
 * @param obj The Efl_Loop object, currently unused but kept for API consistency.
 */
void
_ecore_main_timechanges_stop(Eo *obj EINA_UNUSED)
{
#ifdef HAVE_SYS_TIMERFD_H
   Eo *o = realtime_obj;
   if (!o) return;
   realtime_obj = NULL;
   efl_del(o);
#endif
}
