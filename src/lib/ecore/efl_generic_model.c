#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl.h>
#include <Ecore.h>

#include "ecore_internal.h"

#define MY_CLASS EFL_GENERIC_MODEL_CLASS

/**
 * @brief Private data structure for Efl_Generic_Model.
 *
 * This structure holds the properties and children of a generic model instance.
 */
typedef struct _Efl_Generic_Model_Data Efl_Generic_Model_Data;
struct _Efl_Generic_Model_Data
{
   Eina_Hash                        *properties; /**< Hash table storing model properties. Keys are Eina_Stringshare, values are Eina_Value. */
   Eina_List                        *childrens;  /**< List of child Efl_Model objects. */
};

/**
 * @brief Callback function to free an Eina_Value.
 *
 * Used as a callback for eina_hash_stringshared_new to free the values
 * stored in the properties hash when the hash is destroyed or items are removed.
 *
 * @param data Pointer to the Eina_Value to be freed.
 */
static void
_item_value_free_cb(void *data)
{
   eina_value_free(data);
}

/**
 * @brief Callback function to free Eina_Stringshare keys from a hash.
 *
 * Used as a callback for eina_hash_foreach to release the stringshared keys
 * when the properties hash is being destroyed.
 *
 * @param hash The hash being iterated (unused).
 * @param key The stringshared key to be deleted.
 * @param data The data associated with the key (unused).
 * @param fdata User data passed to eina_hash_foreach (unused).
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
static Eina_Bool
_stringshared_keys_free(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data EINA_UNUSED, void *fdata EINA_UNUSED)
{
   eina_stringshare_del(key);
   return EINA_TRUE;
}

/**
 * @brief Constructor for Efl_Generic_Model.
 *
 * Initializes the private data structure, including the properties hash.
 *
 * @param obj The Eo object to construct.
 * @param sd Pointer to the private data of the Efl_Generic_Model.
 * @return The constructed Eo object, or NULL on failure.
 */
static Efl_Object *
_efl_generic_model_efl_object_constructor(Eo *obj, Efl_Generic_Model_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   if (!obj)
     return NULL;

   sd->properties = eina_hash_stringshared_new(_item_value_free_cb);

   return obj;
}

/**
 * @brief Destructor for Efl_Generic_Model.
 *
 * Frees all allocated resources, including children list and properties hash.
 *
 * @param obj The Eo object to destruct.
 * @param sd Pointer to the private data of the Efl_Generic_Model.
 */
static void
_efl_generic_model_efl_object_destructor(Eo *obj, Efl_Generic_Model_Data *sd)
{
   eina_list_free(sd->childrens);
   eina_hash_foreach(sd->properties, _stringshared_keys_free, NULL);
   eina_hash_free(sd->properties);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets an iterator over the property names of the model.
 *
 * @param obj The Eo object (unused).
 * @param pd Pointer to the private data of the Efl_Generic_Model.
 * @return An Eina_Iterator that yields Eina_Stringshare property names.
 *         The caller is responsible for freeing the iterator.
 */
static Eina_Iterator *
_efl_generic_model_efl_model_properties_get(const Eo *obj EINA_UNUSED, Efl_Generic_Model_Data *pd)
{
   return eina_hash_iterator_key_new(pd->properties);
}

/**
 * @brief Sets a property on the model.
 *
 * If the property already exists, its value is updated. Otherwise, a new
 * property is created. An EFL_MODEL_EVENT_PROPERTIES_CHANGED event is emitted.
 *
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the Efl_Generic_Model.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set for the property. The function will
 *              internally copy this value.
 * @return A resolved Eina_Future containing a reference to the set Eina_Value on success,
 *         or a rejected Eina_Future with ENOMEM on failure.
 *         Example of a resolved future value:
 *         If `value` was an Eina_Value of type EINA_VALUE_TYPE_INT holding `123`,
 *         the future will resolve to a new Eina_Value also of type EINA_VALUE_TYPE_INT holding `123`.
 */
static Eina_Future *
_efl_generic_model_efl_model_property_set(Eo *obj, Efl_Generic_Model_Data *pd, const char *property, Eina_Value *value)
{
   Eina_Stringshare *prop;
   Eina_Value *exist;
   Efl_Model_Property_Event evt;

   prop = eina_stringshare_add(property);
   exist = eina_hash_find(pd->properties, prop);

   if (!exist)
     {
        exist = eina_value_new(eina_value_type_get(value));
        if (!exist)
          goto value_failed;

        if (!eina_hash_direct_add(pd->properties, eina_stringshare_ref(prop), exist))
          goto hash_failed;
     }

   if (!eina_value_copy(value, exist))
     goto value_failed;

   {
      char *v = eina_value_to_string(value);
      char *e = eina_value_to_string(exist);

      free(v);
      free(e);
   }

   evt.changed_properties = eina_array_new(1);
   eina_array_push(evt.changed_properties, prop);

   efl_event_callback_call(obj, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &evt);

   eina_stringshare_del(prop);
   eina_array_free(evt.changed_properties);

   return efl_loop_future_resolved(obj,
                               eina_value_reference_copy(value));

 hash_failed:
   eina_value_free(exist);
 value_failed:
   eina_stringshare_del(prop);

   return efl_loop_future_rejected(obj,
                               ENOMEM);
}

/**
 * @brief Gets the value of a property from the model.
 *
 * @param obj The Eo object (unused).
 * @param pd Pointer to the private data of the Efl_Generic_Model.
 * @param property The name of the property to get.
 * @return A new Eina_Value containing a copy of the property's value if found.
 *         Returns an Eina_Value of type EINA_VALUE_TYPE_ERROR with code
 *         EFL_MODEL_ERROR_NOT_FOUND if the property does not exist.
 *         The caller is responsible for freeing the returned Eina_Value.
 *         Example of a returned value:
 *         If property "name" holds "Test", this returns an Eina_Value of type
 *         EINA_VALUE_TYPE_STRING holding "Test".
 */
static Eina_Value *
_efl_generic_model_efl_model_property_get(const Eo *obj EINA_UNUSED,
                                          Efl_Generic_Model_Data *pd,
                                          const char *property)
{
   Eina_Stringshare *prop;
   Eina_Value *value;

   prop = eina_stringshare_add(property);
   value = eina_hash_find(pd->properties, prop);
   eina_stringshare_del(prop);

   if (!value)
     return eina_value_error_new(EFL_MODEL_ERROR_NOT_FOUND);

   return eina_value_dup(value);
}

/**
 * @brief Gets a slice of children from the model.
 *
 * The returned Eina_Value will be an array of Efl_Model objects.
 *
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the Efl_Generic_Model.
 * @param start The starting index of the slice.
 * @param count The number of children to retrieve.
 * @return A resolved Eina_Future containing an Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 *         The array holds Efl_Model pointers (Eo *).
 *         Example of the Eina_Value structure if `start=0`, `count=2` and there are two children `child1`, `child2`:
 *         The Eina_Value is an array `[child1_ptr, child2_ptr]`.
 */
static Eina_Future *
_efl_generic_model_efl_model_children_slice_get(Eo *obj, Efl_Generic_Model_Data *pd, unsigned int start, unsigned int count)
{
   Eina_Value v;

   v = efl_model_list_value_get(pd->childrens, start, count);
   return efl_loop_future_resolved(obj, v);
}

/**
 * @brief Gets the total number of children in the model.
 *
 * @param obj The Eo object (unused).
 * @param pd Pointer to the private data of the Efl_Generic_Model.
 * @return The number of children.
 */
static unsigned int
_efl_generic_model_efl_model_children_count_get(const Eo *obj EINA_UNUSED, Efl_Generic_Model_Data *pd)
{
   return eina_list_count(pd->childrens);
}

/**
 * @brief Adds a new child to the model.
 *
 * A new Efl_Generic_Model instance is created and added as a child.
 * Emits EFL_MODEL_EVENT_CHILD_ADDED and EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED events.
 *
 * @param obj The Eo object (parent model).
 * @param sd Pointer to the private data of the Efl_Generic_Model.
 * @return The newly added child Efl_Model object, or NULL on failure.
 */
static Eo *
_efl_generic_model_efl_model_child_add(Eo *obj, Efl_Generic_Model_Data *sd)
{
   Efl_Model_Children_Event cevt = { 0 };
   Efl_Model *child;

   child = efl_add(EFL_GENERIC_MODEL_CLASS, obj);
   if (!child)
     {
        EINA_LOG_ERR("Could not allocate Efl.Generic_Model");
        eina_error_set(EFL_MODEL_ERROR_UNKNOWN);
        return NULL;
     }

   cevt.index = eina_list_count(sd->childrens);
   sd->childrens = eina_list_append(sd->childrens, child);

   efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILD_ADDED, &cevt);
   efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

   return child;
}

/**
 * @brief Deletes a child from the model.
 *
 * Removes the specified child from the model's children list.
 * Emits EFL_MODEL_EVENT_CHILD_REMOVED and EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED events.
 * The child's parent is set to NULL.
 *
 * @param obj The Eo object (parent model).
 * @param sd Pointer to the private data of the Efl_Generic_Model.
 * @param child The Efl_Model object to delete.
 */
static void
_efl_generic_model_efl_model_child_del(Eo *obj, Efl_Generic_Model_Data *sd, Eo *child)
{
   Efl_Model *data;
   Eina_List *l;
   unsigned int i = 0;

   EINA_LIST_FOREACH(sd->childrens, l, data)
     {
        if (data == child)
          {
             Efl_Model_Children_Event cevt = { 0 };

             sd->childrens = eina_list_remove_list(sd->childrens, l);

             cevt.index = i;

             efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILD_REMOVED, &cevt);

             efl_parent_set(child, NULL);

             efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

             break;
          }
        ++i;
     }
}

#include "efl_generic_model.eo.c"
