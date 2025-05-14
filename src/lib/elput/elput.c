#include "elput_private.h"

/**
 * @file
 * @brief Core initialization and shutdown logic for the Elput library.
 *
 * This file contains the functions responsible for initializing and shutting down
 * the Elput library, including its dependencies like Eina, Ecore, Ecore_Event,
 * and Eeze. It also handles the registration and unregistration of custom
 * Ecore event types used by Elput.
 */

/* local variables */
/**
 * @internal
 * @brief Counter for elput_init() calls.
 *
 * This variable tracks the number of times elput_init() has been called.
 * Elput is only truly initialized on the first call and shut down on the
 * corresponding last call.
 */
static int _elput_init_count = 0;

/* external variables */
/**
 * @internal
 * @brief Log domain for Elput.
 *
 * Used by Eina_Log to categorize log messages originating from Elput.
 * It is registered during elput_init() and unregistered during elput_shutdown().
 */
int _elput_log_dom = -1;

/**
 * @brief Event type for seat capability changes.
 *
 * This event is triggered when the capabilities of a seat change
 * (e.g., a new device is added or an existing one is removed,
 * affecting pointer, keyboard, or touch capabilities).
 */
EAPI int ELPUT_EVENT_SEAT_CAPS = 0;
/**
 * @brief Event type for seat frame events.
 *
 * This event is triggered to signal a logical end of a set of input events
 * that should be processed together.
 */
EAPI int ELPUT_EVENT_SEAT_FRAME = 0;
/**
 * @brief Event type for sending keymap information.
 *
 * This event is triggered when a new keymap should be sent to a client
 * or when the current keymap changes.
 */
EAPI int ELPUT_EVENT_KEYMAP_SEND = 0;
/**
 * @brief Event type for sending modifier state.
 *
 * This event is triggered when the state of keyboard modifiers (Shift, Ctrl, Alt, etc.)
 * changes.
 */
EAPI int ELPUT_EVENT_MODIFIERS_SEND = 0;
/**
 * @brief Event type for device change notifications.
 *
 * This event is triggered when a device is added to or removed from a seat.
 */
EAPI int ELPUT_EVENT_DEVICE_CHANGE = 0;
/**
 * @brief Event type for session active state changes.
 *
 * This event is triggered when the session associated with Elput
 * becomes active or inactive.
 */
EAPI int ELPUT_EVENT_SESSION_ACTIVE = 0;
/**
 * @brief Event type for pointer motion.
 *
 * This event is triggered when a pointer device (e.g., mouse) moves.
 */
EAPI int ELPUT_EVENT_POINTER_MOTION = 0;
/**
 * @brief Event type for switch events.
 *
 * This event is triggered by switch devices (e.g., lid switch).
 */
EAPI int ELPUT_EVENT_SWITCH = 0;
/**
 * @brief Event type for swipe gesture begin.
 *
 * This event is triggered when a multi-finger swipe gesture starts.
 */
EAPI int ELPUT_EVENT_SWIPE_BEGIN = 0;
/**
 * @brief Event type for swipe gesture update.
 *
 * This event is triggered during a multi-finger swipe gesture,
 * providing updates on its progress.
 */
EAPI int ELPUT_EVENT_SWIPE_UPDATE = 0;
/**
 * @brief Event type for swipe gesture end.
 *
 * This event is triggered when a multi-finger swipe gesture ends.
 */
EAPI int ELPUT_EVENT_SWIPE_END = 0;

/**
 * @brief Initializes the Elput library.
 *
 * This function initializes Elput and its dependencies: Eina, Ecore,
 * Ecore_Event, and Eeze. It also registers a logging domain for Elput
 * and creates new Ecore event types for various Elput events.
 *
 * This function uses a counter to track the number of initialization calls.
 * It will fully initialize all subsystems only on the first call.
 * Subsequent calls will increment the counter and return its new value.
 *
 * @return The current initialization count. Returns 0 or a negative value
 *         on failure, a positive value on success. Specifically, it returns
 *         the new value of the internal init counter.
 * @see elput_shutdown()
 */
EAPI int
elput_init(void)
{
   if (++_elput_init_count != 1) return _elput_init_count;

   if (!eina_init()) goto eina_err;
   if (!ecore_init()) goto ecore_err;
   if (!ecore_event_init()) goto ecore_event_err;
   if (!eeze_init()) goto eeze_err;

   _elput_log_dom = eina_log_domain_register("elput", ELPUT_DEFAULT_LOG_COLOR);
   if (!_elput_log_dom)
     {
        EINA_LOG_ERR("Could not create logging domain for Elput");
        goto log_err;
     }

   ELPUT_EVENT_SEAT_CAPS = ecore_event_type_new();
   ELPUT_EVENT_SEAT_FRAME = ecore_event_type_new();
   ELPUT_EVENT_KEYMAP_SEND = ecore_event_type_new();
   ELPUT_EVENT_MODIFIERS_SEND = ecore_event_type_new();
   ELPUT_EVENT_DEVICE_CHANGE = ecore_event_type_new();
   ELPUT_EVENT_SESSION_ACTIVE = ecore_event_type_new();
   ELPUT_EVENT_POINTER_MOTION = ecore_event_type_new();
   ELPUT_EVENT_SWITCH = ecore_event_type_new();
   ELPUT_EVENT_SWIPE_BEGIN = ecore_event_type_new();
   ELPUT_EVENT_SWIPE_UPDATE = ecore_event_type_new();
   ELPUT_EVENT_SWIPE_END = ecore_event_type_new();

   return _elput_init_count;

log_err:
   eeze_shutdown();
eeze_err:
   ecore_event_shutdown();
ecore_event_err:
   ecore_shutdown();
ecore_err:
   eina_shutdown();
eina_err:
   return --_elput_init_count;
}

/**
 * @brief Shuts down the Elput library.
 *
 * This function shuts down Elput and its dependencies. It should be called
 * as many times as elput_init() was successfully called.
 *
 * It flushes any pending Elput-specific Ecore events, unregisters the Elput
 * logging domain, and then shuts down Eeze, Ecore_Event, Ecore, and Eina
 * in reverse order of their initialization.
 *
 * This function uses a counter to track the number of shutdown calls.
 * It will fully shut down all subsystems only when the counter reaches zero.
 *
 * @return The current initialization count after decrementing. Returns 0
 *         when Elput is fully shut down.
 * @see elput_init()
 */
EAPI int
elput_shutdown(void)
{
   if (_elput_init_count < 1) return 0;
   if (--_elput_init_count != 0) return _elput_init_count;

   ecore_event_type_flush(ELPUT_EVENT_SEAT_CAPS,
                          ELPUT_EVENT_SEAT_FRAME,
                          ELPUT_EVENT_KEYMAP_SEND,
                          ELPUT_EVENT_MODIFIERS_SEND,
                          ELPUT_EVENT_DEVICE_CHANGE,
                          ELPUT_EVENT_SESSION_ACTIVE,
                          ELPUT_EVENT_POINTER_MOTION,
                          ELPUT_EVENT_SWITCH);

   eina_log_domain_unregister(_elput_log_dom);
   _elput_log_dom = -1;

   eeze_shutdown();
   ecore_event_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return _elput_init_count;
}
