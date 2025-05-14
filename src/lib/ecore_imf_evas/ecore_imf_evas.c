
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"
#include "Ecore_IMF_Evas.h"

/**
 * @internal
 * @brief An empty string constant used as a default for event string fields.
 * This is used to avoid NULL strings when Evas event fields are NULL.
 */
static const char *_ecore_imf_evas_event_empty = "";

/**
 * @internal
 * @brief Converts Evas_Modifier to Ecore_IMF_Keyboard_Modifiers.
 *
 * This function maps modifier keys like Control, Alt, Shift, Super, Hyper,
 * and AltGr from the Evas representation to the Ecore_IMF representation.
 *
 * @param evas_modifiers Pointer to the Evas_Modifier structure.
 * @param imf_keyboard_modifiers Pointer to the Ecore_IMF_Keyboard_Modifiers
 *                               variable to store the converted modifiers.
 */
static void
_ecore_imf_evas_event_modifiers_wrap(Evas_Modifier *evas_modifiers,
                                     Ecore_IMF_Keyboard_Modifiers *imf_keyboard_modifiers)
{
   if (!evas_modifiers || !imf_keyboard_modifiers)
     return;

   *imf_keyboard_modifiers = ECORE_IMF_KEYBOARD_MODIFIER_NONE;
   if (evas_key_modifier_is_set(evas_modifiers, "Control"))
     *imf_keyboard_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
   if (evas_key_modifier_is_set(evas_modifiers, "Alt"))
     *imf_keyboard_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
   if (evas_key_modifier_is_set(evas_modifiers, "Shift"))
     *imf_keyboard_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
   if (evas_key_modifier_is_set(evas_modifiers, "Super") || evas_key_modifier_is_set(evas_modifiers, "Hyper"))
     *imf_keyboard_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_WIN;
   if (evas_key_modifier_is_set(evas_modifiers, "AltGr"))
     *imf_keyboard_modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_ALTGR;
}

/**
 * @internal
 * @brief Converts Evas_Lock to Ecore_IMF_Keyboard_Locks.
 *
 * This function maps lock keys like Num_Lock, Caps_Lock, and Scroll_Lock
 * from the Evas representation to the Ecore_IMF representation.
 *
 * @param evas_locks Pointer to the Evas_Lock structure.
 * @param imf_keyboard_locks Pointer to the Ecore_IMF_Keyboard_Locks
 *                           variable to store the converted locks.
 */
static void
_ecore_imf_evas_event_locks_wrap(Evas_Lock *evas_locks,
                                 Ecore_IMF_Keyboard_Locks *imf_keyboard_locks)
{
   if (!evas_locks || !imf_keyboard_locks)
     return;

   *imf_keyboard_locks = ECORE_IMF_KEYBOARD_LOCK_NONE;
   if (evas_key_lock_is_set(evas_locks, "Num_Lock"))
     *imf_keyboard_locks |= ECORE_IMF_KEYBOARD_LOCK_NUM;
   if (evas_key_lock_is_set(evas_locks, "Caps_Lock"))
     *imf_keyboard_locks |= ECORE_IMF_KEYBOARD_LOCK_CAPS;
   if (evas_key_lock_is_set(evas_locks, "Scroll_Lock"))
     *imf_keyboard_locks |= ECORE_IMF_KEYBOARD_LOCK_SCROLL;
}

/**
 * @internal
 * @brief Converts Evas_Button_Flags to Ecore_IMF_Mouse_Flags.
 *
 * This function maps mouse button flags like double-click and triple-click
 * from the Evas representation to the Ecore_IMF representation.
 *
 * @param evas_flags The Evas_Button_Flags value.
 * @param imf_flags Pointer to the Ecore_IMF_Mouse_Flags variable to store
 *                  the converted flags.
 */
static void
_ecore_imf_evas_event_mouse_flags_wrap(Evas_Button_Flags evas_flags,
                                       Ecore_IMF_Mouse_Flags *imf_flags)
{
   if (!imf_flags)
     return;

   *imf_flags = ECORE_IMF_MOUSE_NONE;
   if (evas_flags & EVAS_BUTTON_DOUBLE_CLICK)
     *imf_flags |= ECORE_IMF_MOUSE_DOUBLE_CLICK;
   if (evas_flags & EVAS_BUTTON_TRIPLE_CLICK)
     *imf_flags |= ECORE_IMF_MOUSE_TRIPLE_CLICK;
}

/**
 * @brief Wraps an Evas_Event_Mouse_In event into an Ecore_IMF_Event_Mouse_In event.
 *
 * This function copies data from an Evas mouse in event structure to an
 * Ecore_IMF mouse in event structure, including coordinates, button state,
 * timestamp, modifiers, and locks.
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_In structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_In structure.
 */
EAPI void
ecore_imf_evas_event_mouse_in_wrap(Evas_Event_Mouse_In *evas_event,
                                   Ecore_IMF_Event_Mouse_In *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->buttons = evas_event->buttons;
   imf_event->output.x = evas_event->output.x;
   imf_event->output.y = evas_event->output.y;
   imf_event->canvas.x = evas_event->canvas.x;
   imf_event->canvas.y = evas_event->canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
}

/**
 * @brief Wraps an Evas_Event_Mouse_Out event into an Ecore_IMF_Event_Mouse_Out event.
 *
 * This function copies data from an Evas mouse out event structure to an
 * Ecore_IMF mouse out event structure, including coordinates, button state,
 * timestamp, modifiers, and locks.
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_Out structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_Out structure.
 */
EAPI void
ecore_imf_evas_event_mouse_out_wrap(Evas_Event_Mouse_Out *evas_event,
                                    Ecore_IMF_Event_Mouse_Out *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->buttons = evas_event->buttons;
   imf_event->output.x = evas_event->output.x;
   imf_event->output.y = evas_event->output.y;
   imf_event->canvas.x = evas_event->canvas.x;
   imf_event->canvas.y = evas_event->canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
}

/**
 * @brief Wraps an Evas_Event_Mouse_Move event into an Ecore_IMF_Event_Mouse_Move event.
 *
 * This function copies data from an Evas mouse move event structure to an
 * Ecore_IMF mouse move event structure, including current and previous coordinates,
 * button state, timestamp, modifiers, and locks.
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_Move structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_Move structure.
 */
EAPI void
ecore_imf_evas_event_mouse_move_wrap(Evas_Event_Mouse_Move *evas_event,
                                     Ecore_IMF_Event_Mouse_Move *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->buttons = evas_event->buttons;
   imf_event->cur.output.x = evas_event->cur.output.x;
   imf_event->cur.output.y = evas_event->cur.output.y;
   imf_event->prev.output.x = evas_event->prev.output.x;
   imf_event->prev.output.y = evas_event->prev.output.y;
   imf_event->cur.canvas.x = evas_event->cur.canvas.x;
   imf_event->cur.canvas.y = evas_event->cur.canvas.y;
   imf_event->prev.canvas.x = evas_event->prev.canvas.x;
   imf_event->prev.canvas.y = evas_event->prev.canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
}

/**
 * @brief Wraps an Evas_Event_Mouse_Down event into an Ecore_IMF_Event_Mouse_Down event.
 *
 * This function copies data from an Evas mouse down event structure to an
 * Ecore_IMF mouse down event structure, including button number, coordinates,
 * timestamp, modifiers, locks, and mouse flags (e.g., double/triple click).
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_Down structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_Down structure.
 */
EAPI void
ecore_imf_evas_event_mouse_down_wrap(Evas_Event_Mouse_Down *evas_event,
                                     Ecore_IMF_Event_Mouse_Down *imf_event)
{
   if (!evas_event || !imf_event)
      return;

   imf_event->button = evas_event->button;
   imf_event->output.x = evas_event->output.x;
   imf_event->output.y = evas_event->output.y;
   imf_event->canvas.x = evas_event->canvas.x;
   imf_event->canvas.y = evas_event->canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
   _ecore_imf_evas_event_mouse_flags_wrap(evas_event->flags, &imf_event->flags);
}

/**
 * @brief Wraps an Evas_Event_Mouse_Up event into an Ecore_IMF_Event_Mouse_Up event.
 *
 * This function copies data from an Evas mouse up event structure to an
 * Ecore_IMF mouse up event structure, including button number, coordinates,
 * timestamp, modifiers, locks, and mouse flags.
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_Up structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_Up structure.
 */
EAPI void
ecore_imf_evas_event_mouse_up_wrap(Evas_Event_Mouse_Up *evas_event,
                                   Ecore_IMF_Event_Mouse_Up *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->button = evas_event->button;
   imf_event->output.x = evas_event->output.x;
   imf_event->output.y = evas_event->output.y;
   imf_event->canvas.x = evas_event->canvas.x;
   imf_event->canvas.y = evas_event->canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
   _ecore_imf_evas_event_mouse_flags_wrap(evas_event->flags, &imf_event->flags);
}

/**
 * @brief Wraps an Evas_Event_Mouse_Wheel event into an Ecore_IMF_Event_Mouse_Wheel event.
 *
 * This function copies data from an Evas mouse wheel event structure to an
 * Ecore_IMF mouse wheel event structure, including direction, wheel value (z),
 * coordinates, timestamp, modifiers, and locks.
 *
 * @param evas_event Pointer to the source Evas_Event_Mouse_Wheel structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Mouse_Wheel structure.
 */
EAPI void
ecore_imf_evas_event_mouse_wheel_wrap(Evas_Event_Mouse_Wheel *evas_event,
                                      Ecore_IMF_Event_Mouse_Wheel *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->direction = evas_event->direction;
   imf_event->z = evas_event->z;
   imf_event->output.x = evas_event->output.x;
   imf_event->output.y = evas_event->output.y;
   imf_event->canvas.x = evas_event->canvas.x;
   imf_event->canvas.y = evas_event->canvas.y;
   imf_event->timestamp = evas_event->timestamp;
   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
   imf_event->timestamp = evas_event->timestamp;
}

/**
 * @brief Wraps an Evas_Event_Key_Down event into an Ecore_IMF_Event_Key_Down event.
 *
 * This function copies data from an Evas key down event structure to an
 * Ecore_IMF key down event structure. This includes key name, key string,
 * compose string, timestamp, keycode, device information, modifiers, and locks.
 * If string fields in the Evas event are NULL, they are replaced with an empty string.
 *
 * @param evas_event Pointer to the source Evas_Event_Key_Down structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Key_Down structure.
 */
EAPI void
ecore_imf_evas_event_key_down_wrap(Evas_Event_Key_Down *evas_event,
                                   Ecore_IMF_Event_Key_Down *imf_event)
{
   if (!evas_event || !imf_event)
     return;

   imf_event->keyname = evas_event->keyname ? evas_event->keyname : _ecore_imf_evas_event_empty;
   imf_event->key = evas_event->key ? evas_event->key : _ecore_imf_evas_event_empty;
   imf_event->string = evas_event->string ? evas_event->string : _ecore_imf_evas_event_empty;
   imf_event->compose = evas_event->compose ? evas_event->compose : _ecore_imf_evas_event_empty;
   imf_event->timestamp = evas_event->timestamp;
   imf_event->keycode = evas_event->keycode;

   if (evas_event->dev)
     {
        imf_event->dev_name = evas_device_name_get(evas_event->dev) ? evas_device_name_get(evas_event->dev) : _ecore_imf_evas_event_empty;
        imf_event->dev_class = (Ecore_IMF_Device_Class)evas_device_class_get(evas_event->dev);
        imf_event->dev_subclass = (Ecore_IMF_Device_Subclass)evas_device_subclass_get(evas_event->dev);
     }
   else
     {
        imf_event->dev_name = _ecore_imf_evas_event_empty;
        imf_event->dev_class = ECORE_IMF_DEVICE_CLASS_NONE;
        imf_event->dev_subclass = ECORE_IMF_DEVICE_SUBCLASS_NONE;
     }

   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
}

/**
 * @brief Wraps an Evas_Event_Key_Up event into an Ecore_IMF_Event_Key_Up event.
 *
 * This function copies data from an Evas key up event structure to an
 * Ecore_IMF key up event structure. This includes key name, key string,
 * compose string, timestamp, keycode, device information, modifiers, and locks.
 * If string fields in the Evas event are NULL, they are replaced with an empty string.
 * Error logging is performed if either input event structure is NULL.
 *
 * @param evas_event Pointer to the source Evas_Event_Key_Up structure.
 * @param imf_event Pointer to the destination Ecore_IMF_Event_Key_Up structure.
 */
EAPI void
ecore_imf_evas_event_key_up_wrap(Evas_Event_Key_Up *evas_event,
                                 Ecore_IMF_Event_Key_Up *imf_event)
{
   if (!evas_event)
     {
        EINA_LOG_ERR("Evas event is missing");
        return;
     }

   if (!imf_event)
     {
        EINA_LOG_ERR("Imf event is missing");
        return;
     }

   imf_event->keyname = evas_event->keyname ? evas_event->keyname : _ecore_imf_evas_event_empty;
   imf_event->key = evas_event->key ? evas_event->key : _ecore_imf_evas_event_empty;
   imf_event->string = evas_event->string ? evas_event->string : _ecore_imf_evas_event_empty;
   imf_event->compose = evas_event->compose ? evas_event->compose : _ecore_imf_evas_event_empty;
   imf_event->timestamp = evas_event->timestamp;
   imf_event->keycode = evas_event->keycode;

   if (evas_event->dev)
     {
        imf_event->dev_name = evas_device_name_get(evas_event->dev) ? evas_device_name_get(evas_event->dev) : _ecore_imf_evas_event_empty;
        imf_event->dev_class = (Ecore_IMF_Device_Class)evas_device_class_get(evas_event->dev);
        imf_event->dev_subclass = (Ecore_IMF_Device_Subclass)evas_device_subclass_get(evas_event->dev);
     }
   else
     {
        imf_event->dev_name = _ecore_imf_evas_event_empty;
        imf_event->dev_class = ECORE_IMF_DEVICE_CLASS_NONE;
        imf_event->dev_subclass = ECORE_IMF_DEVICE_SUBCLASS_NONE;
     }

   _ecore_imf_evas_event_modifiers_wrap(evas_event->modifiers, &imf_event->modifiers);
   _ecore_imf_evas_event_locks_wrap(evas_event->locks, &imf_event->locks);
}
