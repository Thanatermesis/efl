#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_EVENT_PROTECTED

#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EFL_INPUT_HOLD_CLASS

/**
 * @brief Sets the hold flag for the input event.
 *
 * The hold flag indicates if an event is held. For example, a mouse button
 * press is held until the button is released.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @param val EINA_TRUE if the event is held, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_input_hold_input_hold_set(Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd, Eina_Bool val)
{
   pd->hold = !!val;
}

/**
 * @brief Gets the hold flag for the input event.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @return EINA_TRUE if the event is held, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_hold_input_hold_get(const Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd)
{
   return pd->hold;
}

/**
 * @brief Sets the input device associated with this event.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @param dev The input device to set.
 */
EOLIAN static void
_efl_input_hold_efl_input_event_device_set(Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd, Efl_Input_Device *dev)
{
   efl_replace(&pd->device, dev);
}

/**
 * @brief Gets the input device associated with this event.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @return The input device.
 */
EOLIAN static Efl_Input_Device *
_efl_input_hold_efl_input_event_device_get(const Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd)
{
   return pd->device;
}

/**
 * @brief Gets the timestamp of the event in milliseconds.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @return The timestamp in milliseconds.
 */
EOLIAN static double
_efl_input_hold_efl_input_event_timestamp_get(const Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd)
{
   return pd->timestamp;
}

/**
 * @brief Sets the timestamp of the event.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @param ms The timestamp in milliseconds.
 */
EOLIAN static void
_efl_input_hold_efl_input_event_timestamp_set(Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd, double ms)
{
   pd->timestamp = ms;
}

/**
 * @brief Sets the event flags for this input event.
 *
 * Event flags provide additional information about the event, such as
 * whether a modifier key (Shift, Ctrl, Alt) was pressed.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @param flags The event flags to set.
 */
EOLIAN static void
_efl_input_hold_efl_input_event_event_flags_set(Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd, Efl_Input_Flags flags)
{
   pd->event_flags = flags;
}

/**
 * @brief Gets the event flags for this input event.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @return The event flags.
 */
EOLIAN static Efl_Input_Flags
_efl_input_hold_efl_input_event_event_flags_get(const Eo *obj EINA_UNUSED, Efl_Input_Hold_Data *pd)
{
   return pd->event_flags;
}

/**
 * @brief Constructor for the Efl_Input_Hold object.
 *
 * Initializes the object and resets its input state.
 *
 * @param obj The Eolian object being constructed.
 * @param pd The private data of the Eolian object.
 * @return The constructed Eolian object.
 */
EOLIAN static Eo *
_efl_input_hold_efl_object_constructor(Eo *obj, Efl_Input_Hold_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_input_reset(obj);
   return obj;
}

/**
 * @brief Frees resources associated with the Efl_Input_Hold_Data.
 *
 * This function unreferences the device and frees the legacy event info.
 *
 * @param pd The private data of the Eolian object.
 */
static inline void
_efl_input_hold_free(Efl_Input_Hold_Data *pd)
{
   efl_unref(pd->device);
   free(pd->legacy);
}

/**
 * @brief Destructor for the Efl_Input_Hold object.
 *
 * Frees resources held by the object before destroying it.
 *
 * @param obj The Eolian object being destructed.
 * @param pd The private data of the Eolian object.
 */
EOLIAN static void
_efl_input_hold_efl_object_destructor(Eo *obj, Efl_Input_Hold_Data *pd)
{
   _efl_input_hold_free(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Retrieves an instance of Efl_Input_Hold.
 *
 * This function gets a cached instance of an Efl_Input_Hold event object.
 * If `priv` is not NULL, it will be filled with the private data of the event.
 *
 * @param owner The owner object for which to get the event instance.
 * @param priv Pointer to store the private data, or NULL.
 * @return The Efl_Input_Hold event object, or NULL on failure.
 */
EVAS_API EVAS_API_WEAK Eo*
efl_input_hold_instance_get(Efl_Object *owner, void **priv)
{
   Efl_Input_Event *evt = efl_input_event_instance_get(EFL_INPUT_HOLD_CLASS, owner);;

   if (!evt) return NULL;
   if (priv) *priv = efl_data_scope_get(evt, MY_CLASS);
   return evt;
}

/**
 * @brief Class destructor for Efl_Input_Hold.
 *
 * Cleans up any cached event instances associated with this class.
 *
 * @param klass The Efl_Class being destructed.
 */
EOLIAN static void
_efl_input_hold_class_destructor(Efl_Class *klass)
{
   efl_input_event_instance_clean(klass);
}

/**
 * @brief Resets the input event to its default state.
 *
 * This involves freeing associated data and zeroing out the private data structure.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 */
EOLIAN static void
_efl_input_hold_efl_input_event_reset(Eo *obj, Efl_Input_Hold_Data *pd)
{
   _efl_input_hold_free(pd);
   memset(pd, 0, sizeof(*pd));
   pd->eo = obj;
}

/**
 * @brief Duplicates an Efl_Input_Hold event.
 *
 * Creates a new Efl_Input_Hold event with the same properties as the original.
 * The new event's device is a reference to the original's device.
 *
 * @param obj The Eolian object to duplicate.
 * @param pd The private data of the Eolian object to duplicate.
 * @return A new Efl_Input_Event object that is a duplicate of obj, or NULL on failure.
 */
EOLIAN static Efl_Input_Event *
_efl_input_hold_efl_duplicate_duplicate(const Eo *obj, Efl_Input_Hold_Data *pd)
{
   Efl_Input_Hold_Data *ev;
   Efl_Input_Hold *evt;

   evt = efl_add(MY_CLASS, efl_parent_get(obj),
                 efl_allow_parent_unref_set(efl_added, EINA_TRUE));
   ev = efl_data_scope_get(evt, MY_CLASS);
   if (!ev) return NULL;

   memcpy(ev, pd, sizeof(*ev));
   ev->eo = evt;
   ev->legacy = NULL;
   ev->evas_done = 0;
   ev->device = efl_ref(pd->device);

   return evt;
}

/**
 * @brief Gets legacy event information.
 *
 * If legacy information is already cached, it's returned. Otherwise,
 * it's generated and cached. This is for compatibility with older Evas event structures.
 *
 * @param obj The Eolian object.
 * @param pd The private data of the Eolian object.
 * @return A pointer to the legacy event information.
 */
EOLIAN static void *
_efl_input_hold_efl_input_event_legacy_info_get(Eo *obj, Efl_Input_Hold_Data *pd)
{
   if (pd->legacy) return pd->legacy;
   return efl_input_hold_legacy_info_fill(obj, NULL);
}

/* Internal EO APIs */

#define EFL_INPUT_HOLD_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_input_legacy_info_get, _efl_input_hold_efl_input_event_legacy_info_get)

#include "efl_input_hold.eo.c"
