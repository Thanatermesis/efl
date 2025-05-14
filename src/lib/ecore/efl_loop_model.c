#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eina.h"
#include "Efl.h"
#include <Ecore.h>
#include "Eo.h"

#include "ecore_private.h"
#include "efl_loop_model.eo.h"

/**
 * @internal
 * @brief Structure to hold data for a property watcher.
 *
 * This structure is used to manage the necessary information for watching
 * a specific property on an Eo object and fulfilling a promise when that
 * property changes or becomes available.
 */
typedef struct _Efl_Loop_Model_Watcher_Data Efl_Loop_Model_Watcher_Data;

struct _Efl_Loop_Model_Watcher_Data
{
   const char *property; /**< The name of the property being watched (stringshared). */
   Eina_Promise *p;      /**< The promise to be resolved/rejected when the property changes or an error occurs. */
   Eo *obj;              /**< The Eo object whose property is being watched. */
};

/**
 * @internal
 * @brief Event callback triggered when properties change on a watched model.
 *
 * This function is called when an EFL_MODEL_EVENT_PROPERTIES_CHANGED event
 * occurs on the object `wd->obj`. It checks if the specific property
 * `wd->property` is among the changed properties. If so, it retrieves the
 * new value and resolves or rejects the associated promise `wd->p`.
 * If the property value indicates it's not ready yet (EAGAIN), it simply returns,
 * keeping the watcher active. Otherwise, it cleans up the watcher resources.
 *
 * @param data Pointer to the Efl_Loop_Model_Watcher_Data for this watcher.
 * @param event The Efl_Event structure containing event details.
 */
static void _propagate_future(void *data, const Efl_Event *event);

/**
 * @internal
 * @brief Frees resources associated with an Efl_Loop_Model_Watcher_Data instance.
 *
 * This function removes the event callback, unreferences the stringshared
 * property name, and frees the watcher data structure itself.
 *
 * @param wd The watcher data to free.
 */
static void
_efl_loop_model_wathcer_free(Efl_Loop_Model_Watcher_Data *wd)
{
   efl_event_callback_del(wd->obj, EFL_MODEL_EVENT_PROPERTIES_CHANGED,
                          _propagate_future, wd);
   eina_stringshare_del(wd->property);
   free(wd);
}

static void
_propagate_future(void *data, const Efl_Event *event)
{
   Efl_Model_Property_Event *ev = event->info;
   const char *property;
   unsigned int i;
   Eina_Array_Iterator it;
   Efl_Loop_Model_Watcher_Data *wd = data;

   EINA_ARRAY_ITER_NEXT(ev->changed_properties, i, property, it)
     if (property == wd->property || eina_streq(property, wd->property))
       {
          Eina_Value *v = efl_model_property_get(wd->obj, wd->property);

          if (eina_value_type_get(v) == EINA_VALUE_TYPE_ERROR)
            {
               Eina_Error err = 0;

               eina_value_get(v, &err);

               if (err == EAGAIN)
                 return ; // Not ready yet

               eina_promise_reject(wd->p, err);
            }
          else
            {
               eina_promise_resolve(wd->p, eina_value_reference_copy(v));
            }

          eina_value_free(v);
          _efl_loop_model_wathcer_free(wd);
          break ;
       }
}

/**
 * @internal
 * @brief Callback invoked when a promise associated with a property watcher is cancelled.
 *
 * This function ensures that if the promise `wd->p` (from `Efl_Loop_Model_Watcher_Data`)
 * is cancelled externally, the associated watcher resources are cleaned up.
 *
 * @param data Pointer to the Efl_Loop_Model_Watcher_Data for the watcher.
 * @param dead_ptr The promise that was cancelled (unused).
 */
static void
_event_cancel(void *data, const Eina_Promise *dead_ptr EINA_UNUSED)
{
   _efl_loop_model_wathcer_free(data);
}

/**
 * @internal
 * @brief Implements Efl.Model.property_ready_get.
 *
 * Returns a future that resolves with the property's value once it's available
 * (i.e., not in an EAGAIN error state). If the property is immediately available,
 * the future is resolved right away. If it's not ready (EAGAIN), a watcher is
 * set up to resolve the future when the property changes.
 *
 * @param obj The Efl_Loop_Model object.
 * @param pd Private data for the Efl_Loop_Model (unused).
 * @param property The name of the property to get.
 * @return A future that will resolve to an Eina_Value containing the property value,
 *         or reject with an Eina_Error. The resolved Eina_Value must be freed by the caller
 *         (e.g. by `eina_value_free()`).
 */
static Eina_Future *
_efl_loop_model_efl_model_property_ready_get(Eo *obj, void *pd EINA_UNUSED, const char *property)
{
   Eina_Value *value = efl_model_property_get(obj, property);
   Eina_Future *f;

   if (eina_value_type_get(value) == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err = 0;

        eina_value_get(value, &err);
        eina_value_free(value);

        if (err == EAGAIN)
          {
             Efl_Loop_Model_Watcher_Data *wd = calloc(1, sizeof (Efl_Loop_Model_Watcher_Data));

             wd->obj = obj;
             wd->property = eina_stringshare_add(property);
             wd->p = eina_promise_new(efl_loop_future_scheduler_get(obj),
                                      _event_cancel, wd);

             efl_event_callback_add(obj,
                                    EFL_MODEL_EVENT_PROPERTIES_CHANGED,
                                    _propagate_future, wd);
             return efl_future_then(obj, eina_future_new(wd->p));
          }

        return eina_future_rejected(efl_loop_future_scheduler_get(obj), err);
     }
   f = eina_future_resolved(efl_loop_future_scheduler_get(obj),
                            eina_value_reference_copy(value));
   eina_value_free(value);
   return efl_future_then(obj, f);
}

/**
 * @internal
 * @brief Callback for eina_future_then, used to extract a single Eo object from an Eina_Value array.
 *
 * This function is typically used when a model operation that is expected to return a
 * single object (e.g., fetching a child at a specific index via `efl_model_children_slice_get`
 * with count 1) actually returns an Eina_Value of type EINA_VALUE_TYPE_ARRAY containing
 * that single object as its only element. This function unpacks it.
 *
 * @param data User data passed to eina_future_then (unused).
 * @param v The Eina_Value to process. Expected to be an array with one Eo object.
 *          Example: Eina_Value(Type: ARRAY, Content: [Eo* child_object])
 * @param f The future that resolved with value `v` (unused).
 * @return An Eina_Value of type EINA_VALUE_TYPE_OBJECT containing the extracted Eo object,
 *         or an Eina_Value of type EINA_VALUE_TYPE_ERROR if `v` is not a single-element array
 *         or if `v` itself is an error.
 *         Example success return: Eina_Value(Type: OBJECT, Content: Eo* child_object)
 */
static Eina_Value
_unpack_from_array(void *data EINA_UNUSED, Eina_Value v, const Eina_Future *f EINA_UNUSED)
{
   Eo *object = NULL;

   if (eina_value_type_get(&v) == EINA_VALUE_TYPE_ERROR) return v;
   if (eina_value_type_get(&v) != EINA_VALUE_TYPE_ARRAY) return eina_value_error_init(EINVAL);
   if (eina_value_array_count(&v) != 1) return eina_value_error_init(EINVAL);

   eina_value_array_get(&v, 0, &object);

   return eina_value_object_init(object);
}

/**
 * @internal
 * @brief Implements Efl.Model.children_index_get.
 *
 * Retrieves children at multiple specified indices. For each index, it fetches
 * a slice of one child and then unpacks the child object from the slice result.
 * It uses `eina_future_all_iterator` to combine the futures for each child into
 * a single future that resolves with an array of all requested child objects.
 *
 * @param obj The Efl_Loop_Model object.
 * @param pd Private data for the Efl_Loop_Model (unused).
 * @param indexes An iterator providing the unsigned integer indexes of the children to retrieve.
 * @return A future that will resolve to an Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 *         This array will contain Eina_Value elements of type EINA_VALUE_TYPE_OBJECT,
 *         each representing a child Eo object. The order of children in the resolved array
 *         corresponds to the order of indexes from the input iterator.
 *         Example of resolved Eina_Value structure:
 *         Eina_Value (Type: ARRAY)
 *         |
 *         +-- [0]: Eina_Value (Type: OBJECT, Value: Eo* child_at_first_index)
 *         +-- [1]: Eina_Value (Type: OBJECT, Value: Eo* child_at_second_index)
 *         ...
 *
 *         The caller is responsible for freeing the resolved Eina_Value (e.g. using `eina_value_free()`),
 *         which will also free the contained array and its elements if they are set up to be.
 *         The individual Eo* objects within the array are typically owned by the model or
 *         their respective parents and should not be freed directly by the caller unless
 *         their lifecycle management dictates otherwise.
 */
static Eina_Future *
_efl_loop_model_efl_model_children_index_get(Eo *obj, void *pd EINA_UNUSED, Eina_Iterator *indexes)
{
   Eina_Future *r;
   Eina_Array futures; /* Stores Eina_Future* objects */
   unsigned int idx;

   eina_array_step_set(&futures, sizeof (Eina_Array), 8);

   EINA_ITERATOR_FOREACH(indexes, idx)
     eina_array_push(&futures, eina_future_then(efl_model_children_slice_get(obj, idx, 1), _unpack_from_array, NULL));
   eina_iterator_free(indexes);

   r = efl_future_then(obj, eina_future_all_iterator(eina_array_iterator_new(&futures)),
                       .success = _efl_future_all_repack,
                       .success_type = EINA_VALUE_TYPE_ARRAY);

   eina_array_flush(&futures);

   return r;
}

/**
 * @internal
 * @brief Event callback for EFL_EVENT_NOREF on "volatile" objects.
 *
 * This function is registered on objects marked as volatile via
 * `_efl_loop_model_volatile_make`. When the object's refcount (excluding
 * internal Efl refs) drops to zero and EFL_EVENT_NOREF is emitted, this
 * callback is triggered. It first unregisters itself to prevent re-entry
 * and then explicitly deletes the object using `efl_del()`. This ensures
 * that volatile loop model objects are cleaned up when they are no longer
 * externally referenced.
 *
 * @param data User data passed to efl_event_callback_add (unused).
 * @param event The Efl_Event structure for EFL_EVENT_NOREF.
 */
static void
_noref_death(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_event_callback_del(event->object, EFL_EVENT_NOREF, _noref_death, NULL);
   // For safety reason and in case multiple call to volatile has been made
   // we check that there is still a parent at this point in EFL_EVENT_NOREF
   efl_del(event->object);
}

/**
 * @internal
 * @brief Implements Efl.Loop.Model.volatile_make.
 *
 * Marks the Efl_Loop_Model object as "volatile". A volatile object will
 * automatically call `efl_del()` on itself when its last external reference
 * is gone (specifically, when EFL_EVENT_NOREF is triggered). This is useful
 * for objects that are managed by the loop model and should be cleaned up
 * when no longer actively used or referenced elsewhere.
 *
 * It achieves this by registering the `_noref_death` callback for the
 * EFL_EVENT_NOREF event on the object.
 *
 * @param obj The Efl_Loop_Model object to make volatile.
 * @param pd Private data for the Efl_Loop_Model (unused).
 */
static void
_efl_loop_model_volatile_make(Eo *obj, void *pd EINA_UNUSED)
{
   // Just to make sure we do not double register this callback, we first remove
   // any potentially previous one.
   efl_event_callback_del(obj, EFL_EVENT_NOREF, _noref_death, NULL);
   efl_event_callback_add(obj, EFL_EVENT_NOREF, _noref_death, NULL);
}

/**
 * @internal
 * @brief Implements Efl.Model.property_set.
 *
 * Sets a property on the object using reflection.
 *
 * @param obj The Efl_Loop_Model object.
 * @param pd Private data for the Efl_Loop_Model (unused).
 * @param property The name of the property to set.
 * @param value An Eina_Value containing the new value for the property.
 *              The function expects a valid, non-NULL Eina_Value.
 * @return A future that resolves with an Eina_Value containing the actual value
 *         of the property after being set (obtained via reflection), or rejects
 *         with an Eina_Error if the operation fails (e.g., incorrect value type,
 *         property not found, or `value` is NULL).
 *         The resolved Eina_Value must be freed by the caller.
 */
static Eina_Future *
_efl_loop_model_efl_model_property_set(Eo *obj, void *pd EINA_UNUSED,
                                       const char *property, Eina_Value *value)
{
   Eina_Error err;

   if (!value) return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
   err = efl_property_reflection_set(obj, property, *value);
   if (err) return efl_loop_future_rejected(obj, err);

   return efl_loop_future_resolved(obj, efl_property_reflection_get(obj, property));
}

/**
 * @internal
 * @brief Implements Efl.Model.property_get.
 *
 * Retrieves a property value from the object using reflection.
 *
 * @param obj The Efl_Loop_Model object.
 * @param pd Private data for the Efl_Loop_Model (unused).
 * @param property The name of the property to get.
 * @return A new Eina_Value* containing the property's value. The caller is
 *         responsible for freeing this Eina_Value using `eina_value_free()`.
 *         If the property does not exist or an error occurs during retrieval,
 *         the returned Eina_Value will be of type EINA_VALUE_TYPE_ERROR.
 */
static Eina_Value *
_efl_loop_model_efl_model_property_get(const Eo *obj, void *pd EINA_UNUSED,
                                       const char *property)
{
   Eina_Value *r;
   Eina_Value direct;

   direct = efl_property_reflection_get(obj, property);
   r = eina_value_dup(&direct);
   eina_value_flush(&direct);

   return r;
}

/**
 * @internal
 * @brief Overrides Efl.Object.invalidate.
 *
 * Performs cleanup specific to Efl_Loop_Model during object invalidation.
 * This primarily involves ensuring that the `_noref_death` callback,
 * potentially added by `_efl_loop_model_volatile_make`, is removed.
 * After performing its specific cleanup, it calls the invalidate method
 * of the superclass.
 *
 * @param obj The Efl_Loop_Model object being invalidated.
 * @param pd Private data for the Efl_Loop_Model (unused).
 */
static void
_efl_loop_model_efl_object_invalidate(Eo *obj, void *pd EINA_UNUSED)
{
   efl_event_callback_del(obj, EFL_EVENT_NOREF, _noref_death, NULL);

   efl_invalidate(efl_super(obj, EFL_LOOP_MODEL_CLASS));
}

#include "efl_loop_model.eo.c"
