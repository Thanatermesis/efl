#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl.h>
#include <Eina.h>
#include <Eo.h>
#include <Ecore.h>

#include "ecore_internal.h"

typedef struct _Efl_Container_Property_Data Efl_Container_Property_Data;
typedef struct _Efl_Container_Model_Data Efl_Container_Model_Data;

/**
 * @brief Structure to hold data for a single property of children in a container model.
 *
 * This structure stores the name of the property, its Eina_Value_Type, and an array
 * of Eina_Value instances representing the property's value for each child.
 */
struct _Efl_Container_Property_Data
{
   Eina_Stringshare *name; /**< The name of the property. */

   const Eina_Value_Type *type; /**< The Eina_Value_Type of the property. */
   Eina_Array *values; /**< An array of Eina_Value, where each element corresponds to a child's property value.
                         * For example, if children have a "name" property (string) and a "size" property (int):
                         * Property "name": values = [Eina_Value(string_child1_name), Eina_Value(string_child2_name), ...]
                         * Property "size": values = [Eina_Value(int_child1_size), Eina_Value(int_child2_size), ...]
                         */
};

/**
 * @brief Structure to hold data for an Efl_Container_Model instance.
 *
 * This structure stores a pointer to its parent container model (if any),
 * a hash table of all properties defined for its children, and the current count of children
 * as determined by the properties.
 */
struct _Efl_Container_Model_Data
{
   Efl_Container_Model_Data *parent; /**< Pointer to the parent Efl_Container_Model_Data, if this model is a child. NULL otherwise. */

   Eina_Hash *properties; /**< A hash table where keys are property names (Eina_Stringshare)
                            * and values are Efl_Container_Property_Data pointers.
                            * This stores all property values for all children managed by this container model.
                            */
   unsigned int children_count; /**< The number of children, typically derived from the longest property array. */
};

/**
 * @internal
 * @brief Frees an Eina_Array of Eina_Value pointers.
 *
 * Iterates through the array, frees each Eina_Value, and then frees the array itself.
 *
 * @param values The Eina_Array to free.
 */
static void
_property_values_free(Eina_Array *values)
{
   Eina_Value *v;

   while ((v = eina_array_pop(values)))
     eina_value_free(v);
   eina_array_free(values);
}

/**
 * @internal
 * @brief Callback function to free Efl_Container_Property_Data.
 *
 * Used as a callback for eina_hash_free_cb to clean up property data
 * when a property is removed or the hash table is destroyed.
 *
 * @param data A pointer to the Efl_Container_Property_Data to be freed.
 */
static void
_property_data_free_cb(void *data)
{
   Efl_Container_Property_Data *cpd = data;

   eina_stringshare_del(cpd->name);
   _property_values_free(cpd->values);
   free(cpd);
}

/**
 * @internal
 * @brief Constructor for Efl_Container_Model.
 *
 * Initializes the Efl_Container_Model_Data structure, including setting up
 * the parent relationship if applicable and initializing the properties hash table.
 *
 * @param obj The Eo object being constructed.
 * @param sd Pointer to the Efl_Container_Model_Data for this object.
 * @return The constructed Eo object, or NULL on failure.
 */
static Efl_Object *
_efl_container_model_efl_object_constructor(Eo *obj,
                                            Efl_Container_Model_Data *sd)
{
   Eo *parent;

   obj = efl_constructor(efl_super(obj, EFL_CONTAINER_MODEL_CLASS));
   if (!obj) return NULL;

   parent = efl_parent_get(obj);
   if (efl_isa(parent, EFL_CONTAINER_MODEL_CLASS))
     sd->parent = efl_data_scope_get(parent, EFL_CONTAINER_MODEL_CLASS);

   sd->properties = eina_hash_stringshared_new(_property_data_free_cb);

   return obj;
}

/**
 * @internal
 * @brief Finalizer for Efl_Container_Model.
 *
 * Ensures that a model source (Efl_Model) is set. If no source model is
 * explicitly set on this container model, a default EFL_GENERIC_MODEL_CLASS
 * instance is created and set as the source. This allows the container model
 * to function even without an external data source, managing its children's
 * properties internally.
 *
 * @param obj The Eo object being finalized.
 * @param sd Pointer to the Efl_Container_Model_Data for this object.
 * @return The finalized Eo object.
 */
static Efl_Object *
_efl_container_model_efl_object_finalize(Eo *obj, Efl_Container_Model_Data *sd EINA_UNUSED)
{
   if (!efl_ui_view_model_get(obj))
     {
        // Add a dummy object as source if there isn't any to allow using this class without source.
        efl_ui_view_model_set(obj, efl_add(EFL_GENERIC_MODEL_CLASS, obj));
     }
   return efl_finalize(efl_super(obj, EFL_CONTAINER_MODEL_CLASS));
}

/**
 * @internal
 * @brief Destructor for Efl_Container_Model.
 *
 * Frees resources allocated by the Efl_Container_Model, specifically the
 * properties hash table.
 *
 * @param obj The Eo object being destructed.
 * @param sd Pointer to the Efl_Container_Model_Data for this object.
 */
static void
_efl_container_model_efl_object_destructor(Eo *obj,
                                           Efl_Container_Model_Data *sd)
{
   eina_hash_free(sd->properties);

   efl_destructor(efl_super(obj, EFL_CONTAINER_MODEL_CLASS));
}

/**
 * @internal
 * @brief Gets the Eina_Value_Type of a given child property.
 *
 * This function is part of the Efl.Container_Model interface.
 * It retrieves the type of a property managed by this container model.
 *
 * @param obj The Efl_Container_Model object.
 * @param sd Pointer to the Efl_Container_Model_Data.
 * @param property The name of the property to query.
 * @return The Eina_Value_Type of the property, or NULL if the property does not exist.
 */
static const Eina_Value_Type *
_efl_container_model_child_property_value_type_get(Eo *obj EINA_UNUSED,
                                                   Efl_Container_Model_Data *sd,
                                                   const char *property)
{
   Efl_Container_Property_Data *cpd;
   Eina_Stringshare *key;

   key = eina_stringshare_add(property);
   cpd = eina_hash_find(sd->properties, key);
   eina_stringshare_del(key);

   if (!cpd) return NULL;
   return cpd->type;
}

/**
 * @internal
 * @brief Adds or updates a property for all children in the container model.
 *
 * This function is part of the Efl.Container_Model interface.
 * It takes a property name, its type, and an iterator of values. Each value
 * from the iterator corresponds to a child at the same index.
 * If the property already exists, its type and values are updated.
 * If new values extend beyond the current children_count, CHILD_ADDED events
 * are emitted, and CHILDREN_COUNT_CHANGED is emitted if the count increases.
 *
 * @param obj The Efl_Container_Model object.
 * @param sd Pointer to the Efl_Container_Model_Data.
 * @param name The name of the property to add or update.
 * @param type The Eina_Value_Type of the property.
 * @param values An Eina_Iterator providing the values for the property for each child.
 *               The iterator will be consumed by this function.
 *               For example, to set a "color" property for 3 children:
 *               values might yield: Eina_Value(red_for_child1), Eina_Value(green_for_child2), Eina_Value(blue_for_child3)
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_efl_container_model_child_property_add(Eo *obj,
                                        Efl_Container_Model_Data *sd,
                                        const char *name,
                                        const Eina_Value_Type *type,
                                        Eina_Iterator *values)
{
   Eina_Array *arr = NULL;
   void *original;
   Efl_Container_Property_Data *cpd = NULL;
   unsigned int i;
   Eina_Error err = EFL_MODEL_ERROR_INCORRECT_VALUE;

   name = eina_stringshare_add(name);

   if (!type || !values)
     {
        EINA_LOG_WARN("Invalid input data");
        goto on_error;
     }

   err = ENOMEM;
   arr = eina_array_new(4);
   if (!arr) goto on_error;

   EINA_ITERATOR_FOREACH(values, original)
     {
        Eina_Value *copy = eina_value_new(type);
        Eina_Bool r;

        if (type == EINA_VALUE_TYPE_STRINGSHARE ||
            type == EINA_VALUE_TYPE_STRING)
          r = eina_value_set(copy, original);
        else
          r = eina_value_pset(copy, original);

        if (!r)
          {
             eina_value_free(copy);
             copy = eina_value_error_new(EINA_ERROR_VALUE_FAILED);
          }

        eina_array_push(arr, copy);
     }
   eina_iterator_free(values);

   err = EFL_MODEL_ERROR_UNKNOWN;

   cpd = eina_hash_find(sd->properties, name);
   if (!cpd)
     {
        cpd = calloc(1, sizeof(Efl_Container_Property_Data));
        if (!cpd)
          goto on_error;

        cpd->name = eina_stringshare_ref(name);
        cpd->type = type;
        cpd->values = arr;

        if (!eina_hash_direct_add(sd->properties, name, cpd))
          goto on_error;
     }
   else
     {
        _property_values_free(cpd->values);

        cpd->type = type;
        cpd->values = arr;
     }

   for (i = sd->children_count; i < eina_array_count(arr); ++i)
     {
        Efl_Model_Children_Event cevt = { 0 };

        cevt.index = i;

        efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILD_ADDED, &cevt);
     }

   if (eina_array_count(arr) > sd->children_count)
     {
        sd->children_count = eina_array_count(arr);
        efl_event_callback_call(obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);
     }

   eina_stringshare_del(name);
   return EINA_TRUE;

 on_error:
   eina_stringshare_del(name);

   if (cpd) free(cpd);
   if (arr) _property_values_free(arr);
   eina_error_set(err);
   return EINA_FALSE;
}

static Eina_Iterator *
_efl_container_model_efl_model_properties_get(const Eo *obj EINA_UNUSED,
                                              Efl_Container_Model_Data *sd)
{
   // This model represents children. The properties of these children are defined
   // in the parent's Efl_Container_Model_Data's 'properties' hash.
   // So, we iterate over the keys of the parent's properties hash.
   EFL_COMPOSITE_MODEL_PROPERTIES_SUPER(props,
                                        obj, EFL_CONTAINER_MODEL_CLASS,
                                        eina_hash_iterator_key_new(sd->parent->properties));
   return props;
}

/**
 * @internal
 * @brief Sets a property value for a specific child model.
 *
 * This function is part of the Efl.Model interface, overridden to handle
 * properties managed by the Efl_Container_Model. It locates the child's
 * index and updates the corresponding value in the property's Eina_Array
 * stored in the parent's Efl_Container_Model_Data.
 *
 * If the property is not managed by this container model (i.e., not found in
 * `pd->parent->properties`), it delegates to the superclass's implementation.
 *
 * @param obj The child Eo object (an Efl_Model representing a child) whose property is being set.
 * @param pd Pointer to the Efl_Container_Model_Data of the child object.
 *           Note: `pd->parent` refers to the Efl_Container_Model_Data of the actual container.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set for the property. The function takes ownership of this value.
 * @return A Eina_Future that resolves with the set Eina_Value on success, or rejects with an error.
 *         Example of resolved value: Eina_Value(int: 42) if an integer property was set to 42.
 */
static Eina_Future *
_efl_container_model_efl_model_property_set(Eo *obj,
                                            Efl_Container_Model_Data *pd,
                                            const char *property,
                                            Eina_Value *value)
{
   if (pd->parent)
     {
        Efl_Container_Property_Data *cpd;
        Eina_Stringshare *name;
        unsigned int index;
        Eina_Value r = EINA_VALUE_EMPTY;

        name = eina_stringshare_add(property);
        // Properties are stored in the parent (the container model itself)
        cpd = eina_hash_find(pd->parent->properties, name);
        eina_stringshare_del(name);

        // Check if this is a property of this object (i.e., managed by the container)
        if (!cpd) goto not_mine;
        // Check if we can get the expected type for this property
        if (!cpd->values)
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

        // Get the index of this child model within its parent container
        index = efl_composite_model_index_get(obj);

        // Try converting the type before touching any data inside the Model
        if (eina_value_type_get(value) != cpd->type)
          {
             eina_value_setup(&r, cpd->type);

             if (!eina_value_convert(value, &r))
               return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
          }
        else
          {
             eina_value_copy(&r, value);
          }

        // Expand the array to have enough space for this value if it wasn't already the case
        // This might happen if properties are set on children that were not yet accounted for
        // by an efl_container_model_child_property_add operation.
        while (index >= eina_array_count(cpd->values))
          eina_array_push(cpd->values, eina_value_error_new(EFL_MODEL_ERROR_INCORRECT_VALUE));

        // Clean the previous value and introduce the new one
        eina_value_free(eina_array_data_get(cpd->values, index));
        // Same type as the expected type, we can just replace the value
        eina_array_data_set(cpd->values, index, eina_value_dup(&r));

        eina_value_free(value); // Original value is consumed or copied
        return efl_loop_future_resolved(obj, r); // Return the (potentially converted) value
     }

 not_mine: // Property is not managed by this container model, delegate to superclass.
   return efl_model_property_set(efl_super(obj, EFL_CONTAINER_MODEL_CLASS), property, value);
}

/**
 * @internal
 * @brief Gets a property value for a specific child model.
 *
 * This function is part of the Efl.Model interface, overridden to handle
 * properties managed by the Efl_Container_Model. It locates the child's
 * index and retrieves the corresponding value from the property's Eina_Array
 * stored in the parent's Efl_Container_Model_Data.
 *
 * If the property is not managed by this container model, it delegates to the
 * superclass's implementation.
 *
 * @param obj The child Eo object (an Efl_Model representing a child) whose property is being retrieved.
 * @param pd Pointer to the Efl_Container_Model_Data of the child object.
 *           Note: `pd->parent` refers to the Efl_Container_Model_Data of the actual container.
 * @param property The name of the property to get.
 * @return A new Eina_Value containing the property's value, or an Eina_Value with an error
 *         if the property doesn't exist, the index is out of bounds, or another error occurs.
 *         The caller is responsible for freeing the returned Eina_Value.
 *         Example: Eina_Value(string: "Child Name")
 */
static Eina_Value *
_efl_container_model_efl_model_property_get(const Eo *obj,
                                            Efl_Container_Model_Data *pd,
                                            const char *property)
{
   if (pd->parent)
     {
        Efl_Container_Property_Data *cpd;
        Eina_Stringshare *name;
        Eina_Value *copy;
        unsigned int index;

        name = eina_stringshare_add(property);
        // Properties are stored in the parent (the container model itself)
        cpd = eina_hash_find(pd->parent->properties, name);
        eina_stringshare_del(name);

        // Check if this is a property of this object (i.e., managed by the container)
        if (!cpd) goto not_mine;
        // Check if we can get the expected type for this property
        if (!cpd->values) return eina_value_error_new(EFL_MODEL_ERROR_INCORRECT_VALUE);

        // Get the index of this child model within its parent container
        index = efl_composite_model_index_get(obj);

        if (index >= eina_array_count(cpd->values))
          return eina_value_error_new(EFL_MODEL_ERROR_INCORRECT_VALUE); // Index out of bounds

        // Duplicate the value so the caller owns it
        copy = eina_value_dup(eina_array_data_get(cpd->values, index));

        return copy;
     }

 not_mine: // Property is not managed by this container model, delegate to superclass.
   return efl_model_property_get(efl_super(obj, EFL_CONTAINER_MODEL_CLASS), property);
}

/**
 * @internal
 * @brief Gets the number of children in the container model.
 *
 * This function is part of the Efl.Model interface.
 * It returns the maximum of the children count derived from the properties
 * managed by this container model (`sd->children_count`) and the children
 * count from the superclass (which might come from a wrapped model).
 * This ensures that the reported count covers all children, whether defined
 * by added properties or by an underlying model.
 *
 * @param obj The Efl_Container_Model object.
 * @param sd Pointer to the Efl_Container_Model_Data.
 * @return The total number of children.
 */
static unsigned int
_efl_container_model_efl_model_children_count_get(const Eo *obj EINA_UNUSED, Efl_Container_Model_Data *sd)
{
   unsigned int pcount;

   pcount = efl_model_children_count_get(efl_super(obj, EFL_CONTAINER_MODEL_CLASS));

   return pcount > sd->children_count ? pcount : sd->children_count;
}

#include "efl_container_model.eo.c"
