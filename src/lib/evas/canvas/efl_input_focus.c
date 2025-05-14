#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_private.h"

#define EFL_INPUT_EVENT_PROTECTED

#include <Evas.h>

#define MY_CLASS EFL_INPUT_FOCUS_CLASS

/**
 * @brief Frees the resources associated with the Efl_Input_Focus_Data.
 *
 * This function is responsible for cleaning up any allocated resources
 * within the Efl_Input_Focus_Data structure, such as weak references
 * and device references.
 *
 * @param pd Pointer to the Efl_Input_Focus_Data structure to free.
 */
static void
_efl_input_focus_free(Efl_Input_Focus_Data *pd)
{
   efl_wref_del_safe(&pd->object_wref);
   efl_unref(pd->device);
}

/**
 * @brief Constructor for the Efl_Input_Focus object.
 *
 * Initializes a new instance of the Efl_Input_Focus class.
 *
 * @param obj The Efl_Input_Focus object to construct.
 * @param pd The private data associated with the object (unused in this constructor).
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_input_focus_efl_object_constructor(Eo *obj,
                                        Efl_Input_Focus_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @brief Destructor for the Efl_Input_Focus object.
 *
 * Cleans up resources used by the Efl_Input_Focus object.
 *
 * @param obj The Efl_Input_Focus object to destruct.
 * @param pd The private data associated with the object.
 */
EOLIAN static void
_efl_input_focus_efl_object_destructor(Eo *obj,
                                       Efl_Input_Focus_Data *pd)
{
   _efl_input_focus_free(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the object associated with this focus event.
 *
 * This is typically the object that gained or lost focus.
 * A weak reference is stored.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @param object The Efl_Object that is the target of this focus event.
 */
EOLIAN static void
_efl_input_focus_object_set(Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd,
                            Efl_Object *object)
{
   pd->object_wref = object;
}

/**
 * @brief Gets the object associated with this focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @return The Efl_Object that is the target of this focus event, or NULL if not set.
 */
EOLIAN static Efl_Object *
_efl_input_focus_object_get(const Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd)
{
   return pd->object_wref;
}

/**
 * @brief Sets the input device that generated this focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @param device The Efl_Input_Device that generated the event.
 */
EOLIAN static void
_efl_input_focus_efl_input_event_device_set(Eo *obj EINA_UNUSED,
                                            Efl_Input_Focus_Data *pd,
                                            Efl_Input_Device *device)
{
   efl_replace(&pd->device, device);
}

/**
 * @brief Gets the input device that generated this focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @return The Efl_Input_Device that generated the event.
 */
EOLIAN static Efl_Input_Device *
_efl_input_focus_efl_input_event_device_get(const Eo *obj EINA_UNUSED,
                                            Efl_Input_Focus_Data *pd)
{
   return pd->device;
}

/**
 * @brief Sets the timestamp of the focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @param ms The timestamp in milliseconds.
 */
EOLIAN static void
_efl_input_focus_efl_input_event_timestamp_set(Eo *obj EINA_UNUSED,
                                               Efl_Input_Focus_Data *pd,
                                               double ms)
{
   pd->timestamp = ms;
}

/**
 * @brief Gets the timestamp of the focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @return The timestamp in milliseconds.
 */
EOLIAN static double
_efl_input_focus_efl_input_event_timestamp_get(const Eo *obj EINA_UNUSED,
                                               Efl_Input_Focus_Data *pd)
{
   return pd->timestamp;
}

/**
 * @brief Duplicates an Efl_Input_Focus event.
 *
 * Creates a new Efl_Input_Focus event object that is a copy of the original.
 *
 * @param obj The original Efl_Input_Focus event object to duplicate.
 * @param pd The private data of the original event.
 * @return A new Efl_Input_Focus event object, or NULL on failure.
 */
EOLIAN static Efl_Input_Focus *
_efl_input_focus_efl_duplicate_duplicate(const Eo *obj, Efl_Input_Focus_Data *pd)
{
   Efl_Input_Focus_Data *ev;
   Efl_Input_Focus *evt;

   evt = efl_add(MY_CLASS, efl_parent_get(obj),
                 efl_allow_parent_unref_set(efl_added, EINA_TRUE));
   ev = efl_data_scope_get(evt, MY_CLASS);
   if (!ev) return NULL;

   memcpy(ev, pd, sizeof(*ev));
   ev->eo        = evt;
   ev->device    = efl_ref(pd->device);
   efl_wref_add(ev->object_wref, &ev->object_wref);

   return evt;
}

/**
 * @brief Gets an instance of an Efl_Input_Focus event.
 *
 * This function retrieves a reusable Efl_Input_Focus event object.
 * If `priv` is not NULL, it will be set to point to the private data
 * of the event object.
 *
 * @param owner The owner object for the event instance.
 * @param priv Pointer to a void pointer that will be set to the private data, or NULL.
 * @return An Efl_Input_Focus event object, or NULL on failure.
 */
EVAS_API EVAS_API_WEAK Eo*
efl_input_focus_instance_get(Efl_Object *owner, void **priv)
{
   Efl_Input_Focus_Data *ev;
   Efl_Input_Focus *evt;

   evt = efl_input_event_instance_get(EFL_INPUT_FOCUS_CLASS, owner);
   if (!evt) return NULL;

   ev = efl_data_scope_get(evt, MY_CLASS);
   if (priv) *priv = ev;
   return evt;
}

/**
 * @brief Class destructor for Efl_Input_Focus.
 *
 * Cleans up resources associated with the Efl_Input_Focus class,
 * specifically the event instances.
 *
 * @param klass The Efl_Class to destruct.
 */
EOLIAN static void
_efl_input_focus_class_destructor(Efl_Class *klass)
{
   efl_input_event_instance_clean(klass);
}

/**
 * @brief Resets an Efl_Input_Focus event to its default state.
 *
 * Frees associated data and zeroes out the private data structure.
 *
 * @param obj The Efl_Input_Focus event object to reset.
 * @param pd The private data for this focus event.
 */
EOLIAN static void
_efl_input_focus_efl_input_event_reset(Eo *obj, Efl_Input_Focus_Data *pd)
{
   _efl_input_focus_free(pd);
   memset(pd, 0, sizeof(Efl_Input_Focus_Data));
   pd->eo = obj;
}

/**
 * @brief Sets the event flags for the focus event.
 *
 * Note: EFL_INPUT_FLAGS_SCROLLING is not a valid flag for focus events.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @param flags The Efl_Input_Flags to set.
 */
EOLIAN static void
_efl_input_focus_efl_input_event_event_flags_set(Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd, Efl_Input_Flags flags)
{
   if (flags == EFL_INPUT_FLAGS_SCROLLING)
     ERR("A focus event cannot be created based on scrolling");
   else
     pd->event_flags |= flags;
}

/**
 * @brief Gets the event flags for the focus event.
 *
 * @param obj The Efl_Input_Focus event object (unused).
 * @param pd The private data for this focus event.
 * @return The Efl_Input_Flags associated with the event.
 */
EOLIAN static Efl_Input_Flags
_efl_input_focus_efl_input_event_event_flags_get(const Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd)
{
   return pd->event_flags;
}


/* Internal EO APIs */

#include "efl_input_focus.eo.c"
