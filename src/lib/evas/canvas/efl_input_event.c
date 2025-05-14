#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_EVENT_PROTECTED

#include <Evas.h>
#define EFL_INTERNAL_UNSTABLE
#include <Evas_Internal.h>
#include "canvas/evas_canvas_eo.h"

#define MY_CLASS EFL_INPUT_EVENT_MIXIN

/**
 * @brief Sets the processed flag for the event.
 *
 * This flag indicates whether the event has been handled by any object.
 *
 * @param obj The event object.
 * @param pd Private data, unused.
 * @param val EINA_TRUE to mark as processed, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_input_event_processed_set(Eo *obj, void *pd EINA_UNUSED, Eina_Bool val)
{
   if (val)
     efl_input_event_flags_set(obj, efl_input_event_flags_get(obj) | EFL_INPUT_FLAGS_PROCESSED);
   else
     efl_input_event_flags_set(obj, efl_input_event_flags_get(obj) & ~EFL_INPUT_FLAGS_PROCESSED);
}

/**
 * @brief Gets the processed flag for the event.
 *
 * @param obj The event object, unused.
 * @param pd Private data, unused.
 * @return EINA_TRUE if the event is marked as processed, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_event_processed_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   return !!(efl_input_event_flags_get(obj) & EFL_INPUT_FLAGS_PROCESSED);
}

/**
 * @brief Sets the scrolling flag for the event.
 *
 * This flag indicates if the event is part of a scrolling sequence.
 *
 * @param obj The event object, unused.
 * @param pd Private data, unused.
 * @param val EINA_TRUE to mark as scrolling, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_input_event_scrolling_set(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, Eina_Bool val)
{
   if (val)
     efl_input_event_flags_set(obj, efl_input_event_flags_get(obj) | EFL_INPUT_FLAGS_SCROLLING);
   else
     efl_input_event_flags_set(obj, efl_input_event_flags_get(obj) & ~EFL_INPUT_FLAGS_SCROLLING);
}

/**
 * @brief Gets the scrolling flag for the event.
 *
 * @param obj The event object, unused.
 * @param pd Private data, unused.
 * @return EINA_TRUE if the event is marked as scrolling, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_input_event_scrolling_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   return !!(efl_input_event_flags_get(obj) & EFL_INPUT_FLAGS_SCROLLING);
}

/**
 * @brief Gets the fake flag for the event.
 *
 * This flag indicates if the event was artificially generated (e.g., by code).
 * Currently, this always returns EINA_FALSE.
 *
 * @param obj The event object, unused.
 * @param pd Private data, unused.
 * @return EINA_FALSE.
 */
EOLIAN static Eina_Bool
_efl_input_event_fake_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Finds a provider for a given class, typically the Evas canvas.
 *
 * This function searches up the parent hierarchy of the event object to find
 * an object that implements the specified class. It's primarily used to
 * locate the Evas canvas associated with an event.
 *
 * @param obj The event object.
 * @param pd Private data, unused.
 * @param klass The class to search for (e.g., EVAS_CANVAS_CLASS).
 * @return The provider object if found, otherwise the result of the superclass's provider_find.
 */
EOLIAN static Efl_Object *
_efl_input_event_efl_object_provider_find(const Eo *obj, void *pd EINA_UNUSED, const Efl_Object *klass)
{
   // Note: provider_find should probably return self if self is a klass object
   if (klass == EVAS_CANVAS_CLASS)
     {
        for (Eo *parent = efl_parent_get(obj); parent; parent = efl_parent_get(parent))
          {
             if (efl_isa(parent, klass))
               return parent;
          }
     }
   return efl_provider_find(efl_super(obj, MY_CLASS), klass);
}

/**
 * @brief A hash table to cache unused event objects for reuse.
 * The key is the Efl_Class of the event, and the value is the event object itself.
 */
static Eina_Hash *_cached_events = NULL;

/**
 * @brief Deletion hook for cached event objects.
 *
 * This function is called when an event object with a del_intercept is deleted.
 * Instead of fully deleting, it attempts to cache the event for later reuse
 * if no other event of the same class is already cached. Otherwise, it unrefs
 * the event, potentially leading to its destruction if no other references exist.
 *
 * @param evt The event object being deleted.
 */
static void
_del_hook(Eo *evt)
{
   Efl_Input_Event *cached;
   const Eo *klass = efl_class_get(evt);

   efl_del_intercept_set(evt, NULL);

   cached = eina_hash_find(_cached_events, &klass);
   if (!cached)
     {
        efl_reuse(evt);
        eina_hash_add(_cached_events, &klass, evt);
        efl_input_reset(evt);
     }
   else
     {
        efl_unref(evt);
     }
}

/**
 * @brief Callback for the EFL_EVENT_NOREF event.
 *
 * This function is called when an event object has no more references.
 * It ensures that the object is properly deleted by removing its own
 * callback and then calling efl_del(). This is part of the self-destruction
 * mechanism for event instances retrieved from the cache.
 *
 * @param data User data associated with the callback, unused.
 * @param event The EFL_EVENT_NOREF event information.
 */
static void
_noref_death(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_event_callback_del(event->object, EFL_EVENT_NOREF, _noref_death, NULL);
   efl_del(event->object);
}

/**
 * @brief Retrieves or creates an instance of an input event.
 *
 * This function attempts to reuse a cached event object of the specified class.
 * If a cached instance is found, it's re-parented, and its self-destruction
 * mechanism (del_hook and NOREF callback) is re-enabled.
 * If no cached instance is available, a new one is created.
 * The returned event is ref'd.
 *
 * @param klass The Efl_Class of the event to get (e.g., EFL_INPUT_EVENT_POINTER_MOVE_CLASS).
 * @param owner The Efl_Object that will own this event instance.
 * @return A new or reused Efl_Input_Event instance, or NULL if the owner is invalidated.
 */
Efl_Input_Event *
efl_input_event_instance_get(const Eo *klass, Eo *owner)
{
   Efl_Input_Event *evt;

   if (efl_invalidated_get(owner)) return NULL;

   if (!_cached_events)
     _cached_events = eina_hash_pointer_new(EINA_FREE_CB(efl_unref));

   evt = eina_hash_find(_cached_events, &klass);
   if (evt)
     {
        // eina_hash_del will call efl_unref, so prevent the destruction of the object
        evt = efl_ref(evt);
        eina_hash_del(_cached_events, &klass, evt);
        efl_parent_set(evt, owner);
        efl_unref(evt); // Remove reference before turning on self destruction
        efl_del_intercept_set(evt, _del_hook);
        efl_event_callback_add(evt, EFL_EVENT_NOREF, _noref_death, NULL);
     }
   else
     {
        evt = efl_add(klass, owner);
        efl_event_callback_add(evt, EFL_EVENT_NOREF, _noref_death, NULL);
        efl_del_intercept_set(evt, _del_hook);
     }

   return efl_ref(evt);
}

/**
 * @brief Cleans cached event instances of a specific class.
 *
 * This function removes and unrefs any cached event object matching the given class.
 * This is typically called when a class is being destroyed or when memory needs
 * to be reclaimed.
 *
 * @param klass The Efl_Class of the event instances to clean from the cache.
 */
void
efl_input_event_instance_clean(Eo *klass)
{
   if (!_cached_events) return ;

   eina_hash_del(_cached_events, &klass, NULL);
}

/* Internal EO APIs */

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(efl_input_legacy_info_get, void *, NULL)

#define EFL_INPUT_EVENT_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_input_legacy_info_get, NULL)

#include "efl_input_event.eo.c"
#include "efl_input_state.eo.c"
#include "efl_input_interface.eo.c"
