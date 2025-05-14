#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif
#include <ctype.h>
#include <errno.h>

#include <pulse/pulseaudio.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ecore_audio_private.h"

/* Ecore mainloop integration start */
/**
 * @brief Structure to hold data for a PulseAudio I/O event integrated with Ecore.
 */
struct pa_io_event
{
   pa_mainloop_api *mainloop; /**< Pointer to the PulseAudio mainloop API. */
   Ecore_Fd_Handler               *handler; /**< Ecore file descriptor handler. */

   void                           *userdata; /**< User data passed to callbacks. */

   pa_io_event_flags_t             flags; /**< PulseAudio I/O event flags. */
   pa_io_event_cb_t                callback; /**< Callback function for I/O events. */
   pa_io_event_destroy_cb_t        destroy_callback; /**< Callback function for event destruction. */
};

/**
 * @brief Maps PulseAudio I/O event flags to Ecore file descriptor handler flags.
 * @param flags The PulseAudio I/O event flags.
 * @return The corresponding Ecore file descriptor handler flags.
 */
static Ecore_Fd_Handler_Flags
map_flags_to_ecore(pa_io_event_flags_t flags)
{
   return (Ecore_Fd_Handler_Flags)((flags & PA_IO_EVENT_INPUT ? ECORE_FD_READ : 0) | (flags & PA_IO_EVENT_OUTPUT ? ECORE_FD_WRITE : 0) | (flags & PA_IO_EVENT_ERROR ? ECORE_FD_ERROR : 0) | (flags & PA_IO_EVENT_HANGUP ? ECORE_FD_READ : 0));
}

/**
 * @brief Wrapper function called by Ecore when an I/O event occurs on a file descriptor.
 * This function translates Ecore events back to PulseAudio events.
 * @param data Pointer to the pa_io_event structure.
 * @param handler The Ecore file descriptor handler that triggered the event.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_io_wrapper(void *data, Ecore_Fd_Handler *handler)
{
   char buf[64];
   pa_io_event_flags_t flags = 0;
   pa_io_event *event = (pa_io_event *)data;
   int fd = 0;
   char *disp = NULL;

   fd = ecore_main_fd_handler_fd_get(handler);
   if (fd < 0) return ECORE_CALLBACK_RENEW;

   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_READ))
     {
        flags |= PA_IO_EVENT_INPUT;

        /* Check for HUP and report */
        if (recv(fd, buf, 64, MSG_PEEK))
          {
             if (errno == ESHUTDOWN || errno == ECONNRESET || errno == ECONNABORTED || errno == ENETRESET)
               {
                  DBG("HUP condition detected");
                  flags |= PA_IO_EVENT_HANGUP;
               }
          }
     }

   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_WRITE))
     flags |= PA_IO_EVENT_OUTPUT;
   if (ecore_main_fd_handler_active_get(handler, ECORE_FD_ERROR))
     flags |= PA_IO_EVENT_ERROR;

   if (getenv("WAYLAND_DISPLAY"))
     {
        disp = eina_strdup(getenv("DISPLAY"));
        unsetenv("DISPLAY");
     }
   event->callback(event->mainloop, event, fd, flags, event->userdata);
   if (disp) setenv("DISPLAY", disp, 1);
   free(disp);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Creates a new PulseAudio I/O event integrated with Ecore.
 * @param api Pointer to the PulseAudio mainloop API.
 * @param fd The file descriptor to monitor.
 * @param flags The PulseAudio I/O event flags to monitor for.
 * @param cb The callback function to execute when an event occurs.
 * @param userdata User data to pass to the callback.
 * @return A pointer to the newly created pa_io_event, or NULL on failure.
 */
static pa_io_event *
_ecore_pa_io_new(pa_mainloop_api *api, int fd, pa_io_event_flags_t flags, pa_io_event_cb_t cb, void *userdata)
{
   pa_io_event *event;

   event = calloc(1, sizeof(pa_io_event));
   if (!event)
     {
        ERR("Failed to allocate memory!");
        return NULL;
     }

   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;
   event->flags = flags;
   event->handler = ecore_main_fd_handler_add(fd, map_flags_to_ecore(flags), _ecore_io_wrapper, event, NULL, NULL);

   return event;
}

/**
 * @brief Enables or disables specific I/O event flags for an existing event.
 * @param event The I/O event to modify.
 * @param flags The new set of PulseAudio I/O event flags.
 */
static void
_ecore_pa_io_enable(pa_io_event *event, pa_io_event_flags_t flags)
{
   event->flags = flags;
   ecore_main_fd_handler_active_set(event->handler, map_flags_to_ecore(flags));
}

/**
 * @brief Frees an I/O event.
 * This function also calls the destroy_callback if it was set.
 * @param event The I/O event to free.
 */
static void
_ecore_pa_io_free(pa_io_event *event)
{
   // The destroy_callback is called by PulseAudio's mainloop abstraction
   // when pa_mainloop_free() is called, or when the event source itself is freed.
   // Here, we only clean up Ecore resources.
   ecore_main_fd_handler_del(event->handler);
   free(event);
}

/**
 * @brief Sets a destroy callback for an I/O event.
 * @param event The I/O event.
 * @param cb The destroy callback function.
 */
static void
_ecore_pa_io_set_destroy(pa_io_event *event, pa_io_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

/* Timed events */
/**
 * @brief Structure to hold data for a PulseAudio timed event integrated with Ecore.
 */
struct pa_time_event
{
   pa_mainloop_api *mainloop; /**< Pointer to the PulseAudio mainloop API. */
   Ecore_Timer                    *timer; /**< Ecore timer associated with this event. */
   struct timeval                  tv; /**< The time at which the event should trigger. */

   void                           *userdata; /**< User data passed to callbacks. */

   pa_time_event_cb_t              callback; /**< Callback function for timed events. */
   pa_time_event_destroy_cb_t      destroy_callback; /**< Callback function for event destruction. */
   Eina_Bool in_event : 1; /**< Flag to indicate if the event callback is currently running. */
   Eina_Bool dead : 1; /**< Flag to indicate if the event is marked for deletion. */
};

/**
 * @brief Frees a timed event.
 * If the event callback is currently running, freeing is deferred.
 * @param event The timed event to free.
 */
void
_ecore_pa_time_free(pa_time_event *event)
{
   event->dead = 1;
   if (event->in_event) return;
   if (event->timer)
     ecore_timer_del(event->timer);

   event->timer = NULL;

   free(event);
}

/**
 * @brief Wrapper function called by Ecore when a timer expires.
 * This function calls the PulseAudio timed event callback.
 * @param data Pointer to the pa_time_event structure.
 * @return ECORE_CALLBACK_CANCEL to indicate the timer should not run again (unless restarted).
 */
Eina_Bool
_ecore_time_wrapper(void *data)
{
   pa_time_event *event = (pa_time_event *)data;
   char *disp = NULL;

   if (getenv("WAYLAND_DISPLAY"))
     {
        disp = eina_strdup(getenv("DISPLAY"));
        unsetenv("DISPLAY");
     }
   event->in_event = 1;
   event->callback(event->mainloop, event, &event->tv, event->userdata);
   if (disp) setenv("DISPLAY", disp, 1);
   free(disp);
   event->in_event = 0;
   event->timer = NULL;
   if (event->dead)
     _ecore_pa_time_free(event);
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Creates a new PulseAudio timed event integrated with Ecore.
 * @param api Pointer to the PulseAudio mainloop API.
 * @param tv The absolute time at which the event should trigger.
 * @param cb The callback function to execute when the timer expires.
 * @param userdata User data to pass to the callback.
 * @return A pointer to the newly created pa_time_event, or NULL on failure.
 */
pa_time_event *
_ecore_pa_time_new(pa_mainloop_api *api, const struct timeval *tv, pa_time_event_cb_t cb, void *userdata)
{
   pa_time_event *event;
   struct timeval now;
   double interval;

   event = calloc(1, sizeof(pa_time_event));
   if (!event)
     {
        ERR("Failed to allocate memory!");
        return NULL;
     }

   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;
   event->tv = *tv;

   if (gettimeofday(&now, NULL) == -1)
     {
        ERR("Failed to get the current time!");
        free(event);
        return NULL;
     }

   interval = (tv->tv_sec - now.tv_sec) + (tv->tv_usec - now.tv_usec) / 1000;
   event->timer = ecore_timer_add(interval, _ecore_time_wrapper, event);

   return event;
}

/**
 * @brief Restarts a timed event with a new trigger time.
 * @param event The timed event to restart.
 * @param tv The new absolute time at which the event should trigger. If NULL, the timer is disabled.
 */
void
_ecore_pa_time_restart(pa_time_event *event, const struct timeval *tv)
{
   struct timeval now;
   double interval;

   /* If tv is NULL disable timer */
   if (!tv)
     {
        ecore_timer_del(event->timer);
        event->timer = NULL;
        return;
     }

   event->tv = *tv;

   if (gettimeofday(&now, NULL) == -1)
     {
        ERR("Failed to get the current time!");
        return;
     }

   interval = (tv->tv_sec - now.tv_sec) + (tv->tv_usec - now.tv_usec) / 1000;
   if (!event->timer)
     {
        event->timer = ecore_timer_add(interval, _ecore_time_wrapper, event);
     }
   else
     {
        ecore_timer_interval_set(event->timer, interval);
        ecore_timer_reset(event->timer);
     }
}

/**
 * @brief Sets a destroy callback for a timed event.
 * @param event The timed event.
 * @param cb The destroy callback function.
 */
void
_ecore_pa_time_set_destroy(pa_time_event *event, pa_time_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

/* Deferred events */
/**
 * @brief Structure to hold data for a PulseAudio deferred event integrated with Ecore.
 * Deferred events are executed when the mainloop is idle.
 */
struct pa_defer_event
{
   pa_mainloop_api *mainloop; /**< Pointer to the PulseAudio mainloop API. */
   Ecore_Idler                    *idler; /**< Ecore idler associated with this event. */

   void                           *userdata; /**< User data passed to callbacks. */

   pa_defer_event_cb_t             callback; /**< Callback function for deferred events. */
   pa_defer_event_destroy_cb_t     destroy_callback; /**< Callback function for event destruction. */
};

/**
 * @brief Wrapper function called by Ecore when the mainloop is idle.
 * This function calls the PulseAudio deferred event callback.
 * @param data Pointer to the pa_defer_event structure.
 * @return ECORE_CALLBACK_CANCEL to indicate the idler should run only once.
 */
Eina_Bool
_ecore_defer_wrapper(void *data)
{
   pa_defer_event *event = (pa_defer_event *)data;

   event->idler = NULL;
   event->callback(event->mainloop, event, event->userdata);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Creates a new PulseAudio deferred event integrated with Ecore.
 * @param api Pointer to the PulseAudio mainloop API.
 * @param cb The callback function to execute when the mainloop is idle.
 * @param userdata User data to pass to the callback.
 * @return A pointer to the newly created pa_defer_event, or NULL on failure.
 */
pa_defer_event *
_ecore_pa_defer_new(pa_mainloop_api *api, pa_defer_event_cb_t cb, void *userdata)
{
   pa_defer_event *event;

   event = calloc(1, sizeof(pa_defer_event));
   if (!event)
     {
        ERR("Failed to allocate memory!");
        return NULL;
     }

   event->mainloop = api;
   event->userdata = userdata;
   event->callback = cb;

   event->idler = ecore_idler_add(_ecore_defer_wrapper, event);

   return event;
}

/**
 * @brief Enables or disables a deferred event.
 * @param event The deferred event.
 * @param b Non-zero to enable, zero to disable.
 */
void
_ecore_pa_defer_enable(pa_defer_event *event, int b)
{
   if (!b && event->idler)
     {
        ecore_idler_del(event->idler);
        event->idler = NULL;
     }
   else if (b && !event->idler)
     {
        event->idler = ecore_idler_add(_ecore_defer_wrapper, event);
     }
}

/**
 * @brief Frees a deferred event.
 * This function also calls the destroy_callback if it was set.
 * @param event The deferred event to free.
 */
void
_ecore_pa_defer_free(pa_defer_event *event)
{
   // The destroy_callback is called by PulseAudio's mainloop abstraction
   // when pa_mainloop_free() is called, or when the event source itself is freed.
   // Here, we only clean up Ecore resources.
   if (event->idler)
     ecore_idler_del(event->idler);

   event->idler = NULL;

   free(event);
}

/**
 * @brief Sets a destroy callback for a deferred event.
 * @param event The deferred event.
 * @param cb The destroy callback function.
 */
void
_ecore_pa_defer_set_destroy(pa_defer_event *event, pa_defer_event_destroy_cb_t cb)
{
   event->destroy_callback = cb;
}

/**
 * @brief Called by PulseAudio when it wants to quit the mainloop.
 * Currently, this function only logs a warning and does not quit the Ecore mainloop.
 * @param api Pointer to the PulseAudio mainloop API (unused).
 * @param retval The return value for the quit operation (unused).
 */
static void
_ecore_pa_quit(pa_mainloop_api *api EINA_UNUSED, int retval EINA_UNUSED)
{
   /* FIXME: Need to clean up timers, etc.? */
   WRN("Not quitting mainloop, although PA requested it");
}

/**
 * @brief The Ecore mainloop integration function table for PulseAudio.
 * This table maps PulseAudio mainloop operations to their Ecore equivalents.
 */
/* Function table for PA mainloop integration */
const pa_mainloop_api functable = {
   .userdata = NULL, /**< User data for the mainloop API (not used here). */

   .io_new = _ecore_pa_io_new, /**< Creates a new I/O event source. */
   .io_enable = _ecore_pa_io_enable,
   .io_free = _ecore_pa_io_free,
   .io_set_destroy = _ecore_pa_io_set_destroy,

   .time_new = _ecore_pa_time_new,
   .time_restart = _ecore_pa_time_restart,
   .time_free = _ecore_pa_time_free,
   .time_set_destroy = _ecore_pa_time_set_destroy,

   .defer_new = _ecore_pa_defer_new,
   .defer_enable = _ecore_pa_defer_enable,
   .defer_free = _ecore_pa_defer_free,
   .defer_set_destroy = _ecore_pa_defer_set_destroy,

   .quit = _ecore_pa_quit,
};

/* *****************************************************
 * Ecore mainloop integration end
 */
