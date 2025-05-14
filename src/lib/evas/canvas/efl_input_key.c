#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_EVENT_PROTECTED

#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EFL_INPUT_KEY_CLASS

/**
 * @brief Retrieves or creates an instance of an Efl_Input_Key event.
 *
 * This function is typically used to get a pooled event object that can be
 * configured and dispatched.
 *
 * @param owner The Efl_Object that owns this event, often a canvas or a specific UI element.
 * @param priv Pointer to a location where the private data of the event (Efl_Input_Key_Data) will be stored.
 * @return A pointer to the Efl_Input_Key instance, or NULL on failure.
 */
EVAS_API EVAS_API_WEAK Eo*
efl_input_key_instance_get(Efl_Object *owner, void **priv)
{
   Efl_Input_Key_Data *ev;
   Efl_Input_Key *evt;
   Evas *evas;

   evt = efl_input_event_instance_get(EFL_INPUT_KEY_CLASS, owner);
   if (!evt) return NULL;

   ev = efl_data_scope_get(evt, EFL_INPUT_KEY_CLASS);
   ev->fake = EINA_FALSE;
   if (priv) *priv = ev;

   evas = efl_provider_find(owner, EVAS_CANVAS_CLASS);
   if (evas)
     {
        Evas_Public_Data *e = efl_data_scope_get(evas, EVAS_CANVAS_CLASS);
        ev->modifiers = &e->modifiers;
        ev->locks = &e->locks;
     }

   return evt;
}

/**
 * @brief Class destructor for Efl_Input_Key.
 *
 * Cleans up resources associated with the Efl_Input_Key class,
 * specifically by cleaning event instances.
 *
 * @param klass The Efl_Class being destructed.
 */
EOLIAN static void
_efl_input_key_class_destructor(Efl_Class *klass)
{
   efl_input_event_instance_clean(klass);
}

/**
 * @brief Constructor for Efl_Input_Key objects.
 *
 * Initializes a new Efl_Input_Key object by calling the parent constructor
 * and then resetting its input state.
 *
 * @param obj The Efl_Input_Key object to construct.
 * @param pd Private data for the Efl_Input_Key object (unused in this function).
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_input_key_efl_object_constructor(Eo *obj, Efl_Input_Key_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_input_reset(obj);
   return obj;
}

/**
 * @brief Frees resources held by an Efl_Input_Key_Data structure.
 *
 * This function releases memory allocated for legacy information,
 * unreferences the associated device, and deletes stringshared strings
 * if `no_stringshare` is false.
 *
 * @param pd Pointer to the Efl_Input_Key_Data structure to free.
 */
static inline void
_efl_input_key_free(Efl_Input_Key_Data *pd)
{
   free(pd->legacy);
   efl_unref(pd->device);
   if (pd->no_stringshare) return;
   eina_stringshare_del(pd->key);
   eina_stringshare_del(pd->keyname);
   eina_stringshare_del(pd->string);
   eina_stringshare_del(pd->compose);
}

/**
 * @brief Destructor for Efl_Input_Key objects.
 *
 * Frees the private data associated with the Efl_Input_Key object and
 * then calls the parent class's destructor.
 *
 * @param obj The Efl_Input_Key object being destructed.
 * @param pd Private data of the Efl_Input_Key object.
 */
EOLIAN static void
_efl_input_key_efl_object_destructor(Eo *obj, Efl_Input_Key_Data *pd)
{
   _efl_input_key_free(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the pressed state of the key.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val EINA_TRUE if the key is pressed, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_input_key_pressed_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, Eina_Bool val)
{
   pd->pressed = !!val;
}

/**
 * @brief Gets the pressed state of the key.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return EINA_TRUE if the key is pressed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_key_pressed_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->pressed;
}

/**
 * @brief Sets the name of the key (e.g., "LeftShift", "A").
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val The key name string. This string is stringshared.
 */
EOLIAN static void
_efl_input_key_key_name_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, const char *val)
{
   eina_stringshare_replace(&pd->keyname, val);
}

/**
 * @brief Gets the name of the key.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The stringshared key name (e.g., "Return", "Escape").
 */
EOLIAN static const char *
_efl_input_key_key_name_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->keyname;
}

/**
 * @brief Sets the symbolic name of the key (keysym) (e.g., "Shift_L", "a").
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val The keysym string. This string is stringshared.
 */
EOLIAN static void
_efl_input_key_key_sym_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, const char *val)
{
   eina_stringshare_replace(&pd->key, val);
}

/**
 * @brief Gets the symbolic name of the key (keysym).
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The stringshared keysym (e.g., "Shift_L", "a").
 */
EOLIAN static const char *
_efl_input_key_key_sym_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->key;
}

/**
 * @brief Sets the UTF-8 string generated by the key press (e.g., "a", "é").
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val The UTF-8 string. This string is stringshared.
 */
EOLIAN static void
_efl_input_key_string_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, const char *val)
{
   eina_stringshare_replace(&pd->string, val);
}

/**
 * @brief Gets the UTF-8 string generated by the key press.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The stringshared UTF-8 string.
 */
EOLIAN static const char *
_efl_input_key_string_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->string;
}

/**
 * @brief Sets the compose string for this key event (e.g., "acute" then "e" for "é").
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val The compose string. This string is stringshared.
 */
EOLIAN static void
_efl_input_key_compose_string_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, const char *val)
{
   eina_stringshare_replace(&pd->compose, val);
}

/**
 * @brief Gets the compose string for this key event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The stringshared compose string.
 */
EOLIAN static const char *
_efl_input_key_compose_string_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->compose;
}

/**
 * @brief Sets the hardware key code for this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param val The hardware key code.
 */
EOLIAN static void
_efl_input_key_key_code_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, int val)
{
   pd->keycode = val;
}

/**
 * @brief Gets the hardware key code for this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The hardware key code.
 */
EOLIAN static int
_efl_input_key_key_code_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->keycode;
}

/**
 * @brief Resets the Efl_Input_Key event data to a default state.
 *
 * This involves freeing existing data and zeroing out the private data structure.
 * The event is marked as a fake event after reset.
 *
 * @param obj The Efl_Input_Key object (unused, but its pointer is stored in pd->eo).
 * @param pd Private data of the Efl_Input_Key object.
 */
EOLIAN static void
_efl_input_key_efl_input_event_reset(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   _efl_input_key_free(pd);
   memset(pd, 0, sizeof(*pd));
   pd->eo = obj;
   pd->fake = EINA_TRUE;
}

/**
 * @brief Duplicates an Efl_Input_Key event.
 *
 * Creates a new Efl_Input_Key event and copies the data from the original event.
 * Stringshared fields are re-stringshared, and the device is referenced.
 * The duplicated event is marked as a fake event.
 *
 * @param obj The original Efl_Input_Key object to duplicate.
 * @param pd Private data of the original Efl_Input_Key object.
 * @return A new Efl_Input_Key event, or NULL on failure.
 */
EOLIAN static Efl_Input_Event *
_efl_input_key_efl_duplicate_duplicate(const Eo *obj, Efl_Input_Key_Data *pd)
{
   Efl_Input_Key_Data *ev;
   Efl_Input_Key *evt;

   evt = efl_add(MY_CLASS, efl_parent_get(obj),
                 efl_allow_parent_unref_set(efl_added, EINA_TRUE));
   ev = efl_data_scope_get(evt, MY_CLASS);
   if (!ev) return NULL;

   memcpy(ev, pd, sizeof(*ev));
   ev->eo        = evt;
   ev->legacy    = NULL;
   ev->key       = eina_stringshare_add(pd->key);
   ev->keyname   = eina_stringshare_add(pd->keyname);
   ev->string    = eina_stringshare_add(pd->string);
   ev->compose   = eina_stringshare_add(pd->compose);
   ev->evas_done = 0;
   ev->win_fed   = 0;
   ev->fake      = 1;
   ev->legacy    = NULL;
   ev->device    = efl_ref(pd->device);

   return evt;
}

/**
 * @brief Sets the timestamp of the event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param ms The timestamp in milliseconds.
 */
EOLIAN static void
_efl_input_key_efl_input_event_timestamp_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, double ms)
{
   pd->timestamp = ms;
}

/**
 * @brief Gets the timestamp of the event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The timestamp in milliseconds.
 */
EOLIAN static double
_efl_input_key_efl_input_event_timestamp_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->timestamp;
}

/**
 * @brief Sets the event flags for this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param flags The Efl_Input_Flags to set (e.g., EFL_INPUT_FLAGS_NONE, EFL_INPUT_FLAGS_SCROLL).
 */
EOLIAN static void
_efl_input_key_efl_input_event_event_flags_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, Efl_Input_Flags flags)
{
   pd->event_flags = flags;
}

/**
 * @brief Gets the event flags for this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The Efl_Input_Flags for this event.
 */
EOLIAN static Efl_Input_Flags
_efl_input_key_efl_input_event_event_flags_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->event_flags;
}

/**
 * @brief Sets the input device that originated this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param dev The Efl_Input_Device. The event will take a reference.
 */
EOLIAN static void
_efl_input_key_efl_input_event_device_set(Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd, Efl_Input_Device *dev)
{
   efl_replace(&pd->device, dev);
}

/**
 * @brief Gets the input device that originated this event.
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return The Efl_Input_Device. The returned object is referenced.
 */
EOLIAN static Efl_Input_Device *
_efl_input_key_efl_input_event_device_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->device;
}

/**
 * @brief Checks if a specific modifier (e.g., Shift, Ctrl) is active for this event.
 *
 * If no seat is provided, the seat associated with the event's device is used.
 *
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param mod The Efl_Input_Modifier to check (e.g., EFL_INPUT_MODIFIER_SHIFT).
 * @param seat The Efl_Input_Device representing the seat to check against, or NULL to use the event's device seat.
 * @return EINA_TRUE if the modifier is active, EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_efl_input_key_efl_input_state_modifier_enabled_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd,
                                                    Efl_Input_Modifier mod, const Efl_Input_Device *seat)
{
   const char *name;

   if (!pd->modifiers) return EINA_FALSE;
   if (!seat)
     {
        seat = efl_input_device_seat_get(pd->device);
        if (!seat) return EINA_FALSE;
     }
   name = _efl_input_modifier_to_string(mod);
   if (!name) return EINA_FALSE;
   return evas_seat_key_modifier_is_set(pd->modifiers, name, seat);
}

/**
 * @brief Checks if a specific lock (e.g., CapsLock, NumLock) is active for this event.
 *
 * If no seat is provided, the seat associated with the event's device is used.
 *
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @param lock The Efl_Input_Lock to check (e.g., EFL_INPUT_LOCK_CAPS).
 * @param seat The Efl_Input_Device representing the seat to check against, or NULL to use the event's device seat.
 * @return EINA_TRUE if the lock is active, EINA_FALSE otherwise or on error.
 */
EOLIAN static Eina_Bool
_efl_input_key_efl_input_state_lock_enabled_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd,
                                                Efl_Input_Lock lock, const Efl_Input_Device *seat)
{
   const char *name;

   if (!pd->locks) return EINA_FALSE;
   if (!seat)
     {
        seat = efl_input_device_seat_get(pd->device);
        if (!seat) return EINA_FALSE;
     }
   name = _efl_input_lock_to_string(lock);
   if (!name) return EINA_FALSE;
   return evas_seat_key_lock_is_set(pd->locks, name, seat);
}

/**
 * @brief Checks if this event was artificially generated (i.e., "fake").
 * @param obj The Efl_Input_Key object (unused).
 * @param pd Private data of the Efl_Input_Key object.
 * @return EINA_TRUE if the event is fake, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_key_efl_input_event_fake_get(const Eo *obj EINA_UNUSED, Efl_Input_Key_Data *pd)
{
   return pd->fake;
}

/**
 * @brief Retrieves legacy event information (Evas_Event_Key_Down/Up) for this key event.
 *
 * If legacy information has not been generated yet, this function will create it.
 * The returned pointer is owned by the event object and should not be freed by the caller.
 *
 * @param obj The Efl_Input_Key object.
 * @param pd Private data of the Efl_Input_Key object.
 * @return A pointer to the legacy event information (e.g., Evas_Event_Key_Down *), or NULL on failure.
 */
EOLIAN static void *
_efl_input_key_efl_input_event_legacy_info_get(Eo *obj, Efl_Input_Key_Data *pd)
{
   if (pd->legacy) return pd->legacy;
   return efl_input_key_legacy_info_fill(obj, NULL);
}

/* Internal EO APIs */

#define EFL_INPUT_KEY_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_input_legacy_info_get, _efl_input_key_efl_input_event_legacy_info_get)

#include "efl_input_key.eo.c"
