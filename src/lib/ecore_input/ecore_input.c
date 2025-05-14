#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "Ecore.h"
#include "ecore_private.h"

#include "Ecore_Input.h"
#include "ecore_input_private.h"


/**
 * @internal
 * @brief Log domain for the Ecore_Input module.
 *
 * This variable stores the log domain identifier used by Ecore_Input
 * for logging messages. It is initialized by eina_log_domain_register()
 * in ecore_event_init() and unregistered in ecore_event_shutdown().
 */
int _ecore_input_log_dom = -1;

/** @brief Event type for a key press. */
EAPI int ECORE_EVENT_KEY_DOWN = 0;
/** @brief Event type for a key release. */
EAPI int ECORE_EVENT_KEY_UP = 0;
/** @brief Event type for a mouse button press. */
EAPI int ECORE_EVENT_MOUSE_BUTTON_DOWN = 0;
/** @brief Event type for a mouse button release. */
EAPI int ECORE_EVENT_MOUSE_BUTTON_UP = 0;
/** @brief Event type for mouse movement. */
EAPI int ECORE_EVENT_MOUSE_MOVE = 0;
/** @brief Event type for mouse wheel scroll. */
EAPI int ECORE_EVENT_MOUSE_WHEEL = 0;
/** @brief Event type for mouse cursor entering a window. */
EAPI int ECORE_EVENT_MOUSE_IN = 0;
/** @brief Event type for mouse cursor leaving a window. */
EAPI int ECORE_EVENT_MOUSE_OUT = 0;
/** @brief Event type for axis updates (e.g., from a joystick or tablet). */
EAPI int ECORE_EVENT_AXIS_UPDATE = 0;
/** @brief Event type for a mouse button cancel event (e.g. when a drag is cancelled). */
EAPI int ECORE_EVENT_MOUSE_BUTTON_CANCEL = 0;
/** @brief Event type for joystick events (deprecated or generic, specific events might be preferred). */
EAPI int ECORE_EVENT_JOYSTICK = 0;

/**
 * @internal
 * @brief Initialization counter for the Ecore event system.
 *
 * This counter tracks the number of times ecore_event_init() has been
 * called, ensuring that initialization and shutdown logic are performed
 * only once.
 */
static int _ecore_event_init_count = 0;

/**
 * @brief Initializes the Ecore event system.
 *
 * This function sets up the necessary resources for Ecore event handling.
 * It initializes Ecore itself if not already done, registers a log domain
 * for Ecore_Input, and creates new event types for various input events
 * (keyboard, mouse, joystick). It also initializes the joystick subsystem.
 *
 * This function uses a reference counter (_ecore_event_init_count).
 * Actual initialization occurs only on the first call. Subsequent calls
 * increment the counter.
 *
 * @return The current initialization count. Returns 0 on failure during
 *         the first initialization.
 * @see ecore_event_shutdown()
 */
EAPI int
ecore_event_init(void)
{
   if (++_ecore_event_init_count != 1)
     return _ecore_event_init_count;
   if (!ecore_init())
     {
        _ecore_event_init_count--;
        return 0;
     }

   _ecore_input_log_dom = eina_log_domain_register
     ("ecore_input", ECORE_INPUT_DEFAULT_LOG_COLOR);
   if(_ecore_input_log_dom < 0)
     {
       EINA_LOG_ERR("Impossible to create a log domain for the ecore input module.");
       return --_ecore_event_init_count;
     }

   ECORE_EVENT_KEY_DOWN = ecore_event_type_new();
   ECORE_EVENT_KEY_UP = ecore_event_type_new();
   ECORE_EVENT_MOUSE_BUTTON_DOWN = ecore_event_type_new();
   ECORE_EVENT_MOUSE_BUTTON_UP = ecore_event_type_new();
   ECORE_EVENT_MOUSE_MOVE = ecore_event_type_new();
   ECORE_EVENT_MOUSE_WHEEL = ecore_event_type_new();
   ECORE_EVENT_MOUSE_IN = ecore_event_type_new();
   ECORE_EVENT_MOUSE_OUT = ecore_event_type_new();
   ECORE_EVENT_AXIS_UPDATE = ecore_event_type_new();
   ECORE_EVENT_MOUSE_BUTTON_CANCEL = ecore_event_type_new();
   ECORE_EVENT_JOYSTICK = ecore_event_type_new();

   ecore_input_joystick_init();

   return _ecore_event_init_count;
}

/**
 * @brief Shuts down the Ecore event system.
 *
 * This function releases resources used by the Ecore event system.
 * It flushes any pending events of the types managed by Ecore_Input,
 * shuts down the joystick subsystem, unregisters the Ecore_Input log
 * domain, and then calls ecore_shutdown().
 *
 * This function uses a reference counter (_ecore_event_init_count).
 * Actual shutdown occurs only when the counter reaches zero (i.e.,
 * for every call to ecore_event_init() there has been a corresponding
 * call to ecore_event_shutdown()).
 *
 * @return The current initialization count. After the final shutdown,
 *         this will be 0.
 * @see ecore_event_init()
 */
EAPI int
ecore_event_shutdown(void)
{
   if (--_ecore_event_init_count != 0)
     return _ecore_event_init_count;

   ecore_event_type_flush(ECORE_EVENT_KEY_DOWN,
                          ECORE_EVENT_KEY_UP,
                          ECORE_EVENT_MOUSE_BUTTON_DOWN,
                          ECORE_EVENT_MOUSE_BUTTON_UP,
                          ECORE_EVENT_MOUSE_MOVE,
                          ECORE_EVENT_MOUSE_WHEEL,
                          ECORE_EVENT_MOUSE_IN,
                          ECORE_EVENT_MOUSE_OUT,
                          ECORE_EVENT_AXIS_UPDATE,
                          ECORE_EVENT_MOUSE_BUTTON_CANCEL,
                          ECORE_EVENT_JOYSTICK);
   ecore_input_joystick_shutdown();
   eina_log_domain_unregister(_ecore_input_log_dom);
   _ecore_input_log_dom = -1;
   ecore_shutdown();
   return _ecore_event_init_count;
}

/**
 * @internal
 * @brief Structure to map key names to modifier types and event flags.
 *
 * This structure is used internally to define a correspondence between
 * string representations of modifier keys (like "Shift_L"), their
 * Ecore_Event_Modifier enum representation, and their
 * ECORE_EVENT_MODIFIER_* bitmask flag.
 */
typedef struct _Ecore_Event_Modifier_Match Ecore_Event_Modifier_Match;
struct _Ecore_Event_Modifier_Match
{
   const char *key;             /**< The string name of the key (e.g., "Shift_L"). */
   Ecore_Event_Modifier modifier; /**< The Ecore_Event_Modifier enum value (e.g., ECORE_SHIFT). */
   unsigned int event_modifier; /**< The ECORE_EVENT_MODIFIER_* bitmask flag (e.g., ECORE_EVENT_MODIFIER_SHIFT). */
};

/**
 * @internal
 * @brief Table mapping modifier key names to their Ecore representations.
 *
 * This static constant array holds the definitions for known modifier keys.
 * It is used by ecore_event_modifier_mask() and ecore_event_update_modifier()
 * to translate between key names, Ecore_Event_Modifier enums, and
 * ECORE_EVENT_MODIFIER_* flags.
 *
 * Example entry:
 * @code
 * { "Shift_L", ECORE_SHIFT, ECORE_EVENT_MODIFIER_SHIFT }
 * @endcode
 * This means the key named "Shift_L" corresponds to the ECORE_SHIFT modifier
 * and the ECORE_EVENT_MODIFIER_SHIFT flag.
 */
static const Ecore_Event_Modifier_Match matchs[] = {
  { "Shift_L", ECORE_SHIFT, ECORE_EVENT_MODIFIER_SHIFT },
  { "Shift_R", ECORE_SHIFT, ECORE_EVENT_MODIFIER_SHIFT },
  { "Alt_L", ECORE_ALT, ECORE_EVENT_MODIFIER_ALT },
  { "Alt_R", ECORE_ALT, ECORE_EVENT_MODIFIER_ALT },
  { "Control_L", ECORE_CTRL, ECORE_EVENT_MODIFIER_CTRL },
  { "Control_R", ECORE_CTRL, ECORE_EVENT_MODIFIER_CTRL },
  { "Caps_Lock", ECORE_CAPS, ECORE_EVENT_MODIFIER_CAPS },
  { "Super_L", ECORE_WIN, ECORE_EVENT_MODIFIER_WIN },
  { "Super_R", ECORE_WIN, ECORE_EVENT_MODIFIER_WIN },
  { "ISO_Level3_Shift", ECORE_MODE, ECORE_EVENT_MODIFIER_ALTGR },
  { "Scroll_Lock", ECORE_SCROLL, ECORE_EVENT_MODIFIER_SCROLL }
};

/**
 * @brief Retrieves the event mask for a given Ecore_Event_Modifier.
 *
 * This function looks up the provided @p modifier in the internal `matchs`
 * table and returns the corresponding `event_modifier` bitmask (one of
 * ECORE_EVENT_MODIFIER_* flags).
 *
 * @param modifier The Ecore_Event_Modifier enum value (e.g., ECORE_SHIFT).
 * @return The corresponding ECORE_EVENT_MODIFIER_* bitmask if found,
 *         otherwise 0.
 *
 * Example:
 * @code
 * unsigned int mask = ecore_event_modifier_mask(ECORE_SHIFT);
 * // mask would now typically be ECORE_EVENT_MODIFIER_SHIFT
 * @endcode
 */
EAPI unsigned int
ecore_event_modifier_mask(Ecore_Event_Modifier modifier)
{
   size_t i;

   for (i = 0; i < sizeof (matchs) / sizeof (Ecore_Event_Modifier_Match); i++)
     if (matchs[i].modifier == modifier)
       return matchs[i].event_modifier;

   return 0;
}

/**
 * @brief Updates the count of an active modifier based on a key name.
 *
 * This function searches for the given @p key string (e.g., "Shift_L",
 * "Control_R") in an internal table. If found, it updates the count
 * for the corresponding modifier in the @p modifiers structure.
 * The count is incremented if @p inc is positive (key press) and
 * decremented if @p inc is negative (key release).
 *
 * The @p modifiers structure holds an array where each index corresponds
 * to an Ecore_Event_Modifier enum value. The value at that index is the
 * count of active keys for that modifier (e.g., if both left and right
 * shift keys are pressed, the count for ECORE_SHIFT would be 2).
 *
 * @param key The string name of the key (e.g., "Shift_L", "Alt_R").
 * @param modifiers A pointer to an Ecore_Event_Modifiers structure whose
 *                  counts are to be updated. If NULL, no counts are updated,
 *                  but the function still returns the identified modifier.
 *                  The `modifiers->array[modifier_enum]` is updated.
 *                  `modifiers->size` must be large enough to hold all
 *                  `Ecore_Event_Modifier` enum values.
 * @param inc An integer indicating whether to increment (typically +1 for
 *            key press) or decrement (typically -1 for key release) the
 *            modifier count.
 * @return The Ecore_Event_Modifier enum value corresponding to the @p key
 *         if found (e.g., ECORE_SHIFT, ECORE_CTRL). Returns ECORE_NONE if
 *         the key is not a recognized modifier.
 *
 * Example:
 * @code
 * Ecore_Event_Modifiers current_modifiers;
 * // Initialize current_modifiers.array and current_modifiers.size appropriately
 *
 * // On Shift_L press:
 * ecore_event_update_modifier("Shift_L", &current_modifiers, 1);
 * // current_modifiers.array[ECORE_SHIFT] would be incremented.
 *
 * // On Shift_L release:
 * ecore_event_update_modifier("Shift_L", &current_modifiers, -1);
 * // current_modifiers.array[ECORE_SHIFT] would be decremented.
 * @endcode
 */
EAPI Ecore_Event_Modifier
ecore_event_update_modifier(const char *key, Ecore_Event_Modifiers *modifiers, int inc)
{
   size_t i;

   for (i = 0; i < sizeof (matchs) / sizeof (Ecore_Event_Modifier_Match); i++)
     if (strcmp(matchs[i].key, key) == 0)
       {
          if (modifiers && matchs[i].modifier < modifiers->size)
            modifiers->array[matchs[i].modifier] += inc;
          return matchs[i].modifier;
       }

   return ECORE_NONE;
}
