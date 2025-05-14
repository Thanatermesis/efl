#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ecore.h>

#include "Ecore_Avahi.h"

#ifdef HAVE_AVAHI
#include <avahi-common/watch.h>

/**
 * @brief Structure to hold an Avahi watch integrated with Ecore's main loop.
 */
typedef struct _Ecore_Avahi_Watch Ecore_Avahi_Watch;
/**
 * @brief Structure to hold an Avahi timeout integrated with Ecore's main loop.
 */
typedef struct _Ecore_Avahi_Timeout Ecore_Avahi_Timeout;

struct _Ecore_Avahi_Watch
{
   Ecore_Fd_Handler  *handler; /**< Ecore file descriptor handler for this watch. */
   Ecore_Avahi       *parent; /**< Pointer to the parent Ecore_Avahi instance. */

   AvahiWatchCallback callback; /**< User-provided callback function for Avahi watch events. */
   void              *callback_data; /**< User-provided data for the Avahi watch callback. */
};

struct _Ecore_Avahi_Timeout
{
   Ecore_Timer         *timer; /**< Ecore timer for this timeout. */
   Ecore_Avahi         *parent; /**< Pointer to the parent Ecore_Avahi instance. */

   AvahiTimeoutCallback callback; /**< User-provided callback function for Avahi timeout events. */
   void                *callback_data; /**< User-provided data for the Avahi timeout callback. */
};

/**
 * @brief Main structure for Ecore Avahi integration.
 * This structure holds the AvahiPoll API functions and lists of active watches and timeouts.
 */
struct _Ecore_Avahi
{
   AvahiPoll  api; /**< AvahiPoll structure with function pointers for main loop integration. */

   Eina_List *watches; /**< List of active Ecore_Avahi_Watch instances. */
   Eina_List *timeouts; /**< List of active Ecore_Avahi_Timeout instances. */
};

/**
 * @brief Converts AvahiWatchEvent flags to Ecore_Fd_Handler_Flags.
 * @param events The Avahi watch events.
 * @return The corresponding Ecore file descriptor handler flags.
 */
static Ecore_Fd_Handler_Flags
_ecore_avahi_events2ecore(AvahiWatchEvent events)
{
   return (events & AVAHI_WATCH_IN ? ECORE_FD_READ : 0) |
     (events & AVAHI_WATCH_OUT ? ECORE_FD_WRITE : 0) |
     ECORE_FD_ERROR;
}

/**
 * @brief Callback function for Ecore file descriptor handlers.
 * This function is called when there is activity on a watched file descriptor.
 * It translates Ecore FD events back to Avahi watch events and calls the user's AvahiWatchCallback.
 * @param data Pointer to the Ecore_Avahi_Watch structure.
 * @param fd_handler The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_avahi_watch_cb(void *data, Ecore_Fd_Handler *fd_handler)
{
   Ecore_Avahi_Watch *watch = data;
   AvahiWatchEvent flags = 0;

   flags = ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ) ? AVAHI_WATCH_IN : 0;
   flags |= ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_WRITE) ? AVAHI_WATCH_OUT : 0;
   flags |= ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR) ? AVAHI_WATCH_ERR : 0;

   watch->callback((AvahiWatch*) watch, ecore_main_fd_handler_fd_get(fd_handler), flags, watch->callback_data);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Creates a new Avahi watch.
 * This function is part of the AvahiPoll API and is called by Avahi to register a new file descriptor watch.
 * @param api The AvahiPoll API structure.
 * @param fd The file descriptor to watch.
 * @param events The events to watch for (AVAHI_WATCH_IN, AVAHI_WATCH_OUT, etc.).
 * @param callback The function to call when an event occurs.
 * @param userdata User data to pass to the callback.
 * @return A pointer to the new AvahiWatch structure, or NULL on failure.
 */
static AvahiWatch *
_ecore_avahi_watch_new(const AvahiPoll *api,
                       int fd, AvahiWatchEvent events,
                       AvahiWatchCallback callback, void *userdata)
{
   Ecore_Avahi_Watch *watch;
   Ecore_Avahi *ea;

   ea = api->userdata;
   watch = calloc(1, sizeof (Ecore_Avahi_Watch));
   if (!watch) return NULL;

   watch->handler = ecore_main_fd_handler_add(fd, _ecore_avahi_events2ecore(events),
                                              _ecore_avahi_watch_cb, watch, NULL, NULL);
   watch->callback = callback;
   watch->callback_data = userdata;
   watch->parent = ea;

   ea->watches = eina_list_append(ea->watches, watch);

   return (AvahiWatch*) watch;
}

/**
 * @brief Updates the events for an existing Avahi watch.
 * This function is part of the AvahiPoll API.
 * @param w The AvahiWatch to update.
 * @param events The new set of events to watch for.
 */
static void
_ecore_avahi_watch_update(AvahiWatch *w, AvahiWatchEvent events)
{
   Ecore_Avahi_Watch *watch = (Ecore_Avahi_Watch *) w;

   ecore_main_fd_handler_active_set(watch->handler, _ecore_avahi_events2ecore(events));
}

/**
 * @brief Frees an Avahi watch.
 * This function is part of the AvahiPoll API.
 * @param w The AvahiWatch to free.
 */
static void
_ecore_avahi_watch_free(AvahiWatch *w)
{
   Ecore_Avahi_Watch *watch = (Ecore_Avahi_Watch *) w;

   ecore_main_fd_handler_del(watch->handler);
   watch->parent->watches = eina_list_remove(watch->parent->watches, watch);
   free(watch);
}

/**
 * @brief Gets the currently monitored events for an Avahi watch.
 * This function is part of the AvahiPoll API.
 * @param w The AvahiWatch to query.
 * @return The AvahiWatchEvent flags representing the monitored events.
 */
static AvahiWatchEvent
_ecore_avahi_watch_get_events(AvahiWatch *w)
{
   Ecore_Avahi_Watch *watch = (Ecore_Avahi_Watch *) w;
   AvahiWatchEvent flags = 0;

   flags = ecore_main_fd_handler_active_get(watch->handler, ECORE_FD_READ) ? AVAHI_WATCH_IN : 0;
   flags |= ecore_main_fd_handler_active_get(watch->handler, ECORE_FD_WRITE) ? AVAHI_WATCH_OUT : 0;
   flags |= ecore_main_fd_handler_active_get(watch->handler, ECORE_FD_ERROR) ? AVAHI_WATCH_ERR : 0;

   return flags;
}

/**
 * @brief Converts a struct timeval to a double representing seconds from now.
 * If the timeval is in the past or NULL, it returns a small positive value or a default large value respectively.
 * @param tv Pointer to the struct timeval to convert. If NULL, a default timeout of 3600 seconds is assumed.
 * @return The time difference in seconds as a double.
 */
static double
_ecore_avahi_timeval2double(const struct timeval *tv)
{
   struct timeval now;
   double tm;

   if (!tv) return 3600;

   gettimeofday(&now, NULL);

   tm = tv->tv_sec - now.tv_sec + (double) (tv->tv_usec - now.tv_usec) / 1000000;
   if (tm < 0) tm = 0.001;

   return tm;
}

/**
 * @brief Callback function for Ecore timers.
 * This function is called when an Avahi timeout expires.
 * It calls the user's AvahiTimeoutCallback.
 * @param data Pointer to the Ecore_Avahi_Timeout structure.
 * @return ECORE_CALLBACK_CANCEL to remove the timer after it fires.
 */
static Eina_Bool
_ecore_avahi_timeout_cb(void *data)
{
   Ecore_Avahi_Timeout *timeout = data;

   timeout->callback((AvahiTimeout*) timeout, timeout->callback_data);

   timeout->timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Creates a new Avahi timeout.
 * This function is part of the AvahiPoll API and is called by Avahi to register a new timeout.
 * @param api The AvahiPoll API structure.
 * @param tv The timeval specifying when the timeout should fire. If NULL, the timeout is initially disabled.
 * @param callback The function to call when the timeout expires.
 * @param userdata User data to pass to the callback.
 * @return A pointer to the new AvahiTimeout structure, or NULL on failure.
 */
static AvahiTimeout *
_ecore_avahi_timeout_new(const AvahiPoll *api, const struct timeval *tv,
                         AvahiTimeoutCallback callback, void *userdata)
{
   Ecore_Avahi_Timeout *timeout;
   Ecore_Avahi *ea;

   ea = api->userdata;
   timeout = calloc(1, sizeof (Ecore_Avahi_Timeout));
   if (!timeout) return NULL;
   if (tv) timeout->timer = ecore_timer_add(_ecore_avahi_timeval2double(tv),
                                            _ecore_avahi_timeout_cb, timeout);
   timeout->callback = callback;
   timeout->callback_data = userdata;
   timeout->parent = ea;

   ea->timeouts = eina_list_append(ea->timeouts, timeout);

   return (AvahiTimeout*) timeout;
}

/**
 * @brief Updates an existing Avahi timeout.
 * This function is part of the AvahiPoll API.
 * @param t The AvahiTimeout to update.
 * @param tv The new timeval for the timeout. If NULL, the timeout is disabled.
 */
static void
_ecore_avahi_timeout_update(AvahiTimeout *t, const struct timeval *tv)
{
   Ecore_Avahi_Timeout *timeout = (Ecore_Avahi_Timeout *) t;

   if (timeout->timer) ecore_timer_del(timeout->timer);
   timeout->timer = NULL;

   if (tv)
     timeout->timer = ecore_timer_add(_ecore_avahi_timeval2double(tv),
                                      _ecore_avahi_timeout_cb, timeout);
}

/**
 * @brief Frees an Avahi timeout.
 * This function is part of the AvahiPoll API.
 * @param t The AvahiTimeout to free.
 */
static void
_ecore_avahi_timeout_free(AvahiTimeout *t)
{
   Ecore_Avahi_Timeout *timeout = (Ecore_Avahi_Timeout *) t;

   if (timeout->timer) ecore_timer_del(timeout->timer);
   timeout->parent->timeouts = eina_list_remove(timeout->parent->timeouts, timeout);
   free(timeout);
}
#endif

/**
 * @brief Creates and initializes a new Ecore_Avahi handler.
 * This handler provides the AvahiPoll API implementation for Ecore.
 * @return A pointer to the new Ecore_Avahi handler, or NULL on failure or if Avahi support is not compiled in.
 * @see ecore_avahi_del()
 * @see ecore_avahi_poll_get()
 *
 * Example:
 * @code
 * Ecore_Avahi *ea = ecore_avahi_add();
 * if (ea) {
 *     const AvahiPoll *poll_api = ecore_avahi_poll_get(ea);
 *     // Use poll_api with Avahi client creation
 * }
 * @endcode
 */
EAPI Ecore_Avahi *
ecore_avahi_add(void)
{
#ifdef HAVE_AVAHI
   Ecore_Avahi *handler;

   handler = calloc(1, sizeof (Ecore_Avahi));
   if (!handler) return NULL;

   handler->api.userdata = handler;
   handler->api.watch_new = _ecore_avahi_watch_new;
   handler->api.watch_free = _ecore_avahi_watch_free;
   handler->api.watch_update = _ecore_avahi_watch_update;
   handler->api.watch_get_events = _ecore_avahi_watch_get_events;

   handler->api.timeout_new = _ecore_avahi_timeout_new;
   handler->api.timeout_free = _ecore_avahi_timeout_free;
   handler->api.timeout_update = _ecore_avahi_timeout_update;

   return handler;
#else
   return NULL;
#endif
}

/**
 * @brief Deletes an Ecore_Avahi handler and frees associated resources.
 * This function cleans up all watches and timers associated with the handler.
 * @param handler The Ecore_Avahi handler to delete.
 * @see ecore_avahi_add()
 *
 * Example:
 * @code
 * Ecore_Avahi *ea = ecore_avahi_add();
 * // ... use ea ...
 * ecore_avahi_del(ea);
 * @endcode
 */
EAPI void
ecore_avahi_del(Ecore_Avahi *handler)
{
#ifdef HAVE_AVAHI
   Ecore_Avahi_Timeout *timeout;
   Ecore_Avahi_Watch *watch;

   EINA_LIST_FREE(handler->watches, watch)
     {
        ecore_main_fd_handler_del(watch->handler);
        free(watch);
     }

   EINA_LIST_FREE(handler->timeouts, timeout)
     {
        ecore_timer_del(timeout->timer);
        free(timeout);
     }

   free(handler);
#else
   (void) handler;
#endif
}

/**
 * @brief Gets the AvahiPoll API structure from an Ecore_Avahi handler.
 * This structure contains function pointers that Avahi uses to integrate with the Ecore main loop.
 * @param handler The Ecore_Avahi handler.
 * @return A pointer to the const AvahiPoll API structure, or NULL if the handler is NULL or Avahi support is not compiled in.
 * @see ecore_avahi_add()
 *
 * Example:
 * @code
 * Ecore_Avahi *ea = ecore_avahi_add();
 * const AvahiPoll *poll_api = ecore_avahi_poll_get(ea);
 * if (poll_api) {
 *     // Pass poll_api to avahi_client_new()
 * }
 * @endcode
 */
EAPI const void *
ecore_avahi_poll_get(Ecore_Avahi *handler)
{
#ifdef HAVE_AVAHI
   if (!handler) return NULL;
   return &handler->api;
#else
   (void)handler;
   return NULL;
#endif
}

