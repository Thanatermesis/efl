#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Efl.h"
#include "Ecore.h"
#include <Eo.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
#  define EAPI __declspec(dllexport)
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif /* ! _WIN32 */


#include "efl_mono_model_internal.eo.h"
#include "efl_mono_model_internal_child.eo.h"

#include "assert.h"

typedef struct _Efl_Mono_Model_Internal_Data Efl_Mono_Model_Internal_Data;

/**
 * @brief Structure to hold information about a single property.
 */
typedef struct _Properties_Info Properties_Info;
struct _Properties_Info
{
  const char* name; /**< The name of the property. */
  const Eina_Value_Type* type; /**< The Eina_Value_Type of the property. */
};

/**
 * @brief Internal data structure for the Efl_Mono_Model_Internal class.
 *
 * This structure holds the core data for the model, including information
 * about the properties it supports and the child items it contains.
 */
typedef struct _Efl_Mono_Model_Internal_Data
{
  Eina_Array *properties_info; /**< Array of Properties_Info structures. Each element is a `Properties_Info*`. Example: `[{name="property1", type=EINA_VALUE_TYPE_STRING}, {name="property2", type=EINA_VALUE_TYPE_INT}]` */
  Eina_Array *properties_names; /**< Array of property names (const char*). Used for quick lookups. Example: `["property1", "property2"]` */
  Eina_Array *items; /**< Array of child items. Each element is an `Efl_Mono_Model_Internal_Child_Data*`. */
} _Efl_Mono_Model_Internal_Data;


#define MY_CLASS EFL_MONO_MODEL_INTERNAL_CLASS

/**
 * @brief Internal data structure for the Efl_Mono_Model_Internal_Child class.
 *
 * This structure holds data specific to a child item within the model,
 * including a reference to the parent model's data, its index, and its property values.
 */
typedef struct _Efl_Mono_Model_Internal_Child_Data
{
  Efl_Mono_Model_Internal_Data* model_pd; /**< Pointer to the parent model's private data. */
  size_t index; /**< Index of this child within the parent model's items array. */
  Eina_Array *values; /**< Array of Eina_Value* storing the property values for this child. The array is indexed by the property's index in `model_pd->properties_names`. Example: if `properties_names` is `["name", "age"]`, `values` might be `[eina_value_string_new("John"), eina_value_int_new(30)]`. Unset values can be NULL or the array might be sparse up to the required index. */
  Eo* child; /**< Pointer to the Eo object representing this child. */
  //Eina_Array *items;
} Efl_Mono_Model_Internal_Child_Data;

/**
 * @brief Finds the index of a property by its name.
 *
 * @param name The name of the property to find.
 * @param properties_name An Eina_Array of const char* property names.
 * @return The index of the property if found, -1 otherwise.
 */
static int _find_property_index (const char* name, Eina_Array* properties_name)
{
   int i, size = eina_array_count_get(properties_name);
   for (i = 0; i != size; ++i)
   {
     if (!strcmp(properties_name->data[i], name))
       {
         return i;
       }
   }
   return -1;
}

/**
 * @brief Constructor for Efl_Mono_Model_Internal.
 *
 * Initializes the internal data structures for the model.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @return The constructed Eo object, or NULL on failure.
 */
static Eo *
_efl_mono_model_internal_efl_object_constructor(Eo *obj, Efl_Mono_Model_Internal_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->properties_info = eina_array_new(5);
   pd->properties_names = eina_array_new(5);
   pd->items = eina_array_new(5);

   if (!obj) return NULL;

   return obj;
}

/**
 * @brief Destructor for Efl_Mono_Model_Internal.
 *
 * Frees resources allocated by the model.
 *
 * @param obj The Eo object to destruct.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 */
static void
_efl_mono_model_internal_efl_object_destructor(Eo *obj, Efl_Mono_Model_Internal_Data *pd EINA_UNUSED)
{
   // TODO: Free contents of pd->properties_info, pd->properties_names, pd->items
   // eina_array_free(pd->properties_info);
   // eina_array_free(pd->properties_names);
   // eina_array_free(pd->items);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Adds a property definition to the model.
 *
 * This function is called to define the schema of the model, specifying
 * the names and types of properties that child items can have.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @param name The name of the property to add.
 * @param type The Eina_Value_Type of the property.
 */
static void
_efl_mono_model_internal_add_property(Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Data *pd, const char *name, const Eina_Value_Type *type)
{
  Properties_Info* info = malloc(sizeof(Properties_Info));
  info->name = eina_stringshare_add(name);
  info->type = type;
  eina_array_push (pd->properties_info, info);
  eina_array_push (pd->properties_names, eina_stringshare_add(info->name));
}

static const char* _efl_mono_model_properties_names[] = { }; // This seems unused or placeholder, model properties are dynamically added.

/**
 * @brief Gets an iterator over the names of the properties supported by the model.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @return An Eina_Iterator that yields const char* property names.
 *         Currently, this returns an iterator over an empty array, which might
 *         not be the intended behavior if properties are added via `_efl_mono_model_internal_add_property`.
 *         It should likely iterate over `pd->properties_names`.
 */
static Eina_Iterator *
_efl_mono_model_internal_efl_model_properties_get(const Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Data *pd EINA_UNUSED)
{
  // FIXME: This should iterate over pd->properties_names if properties are dynamically added.
  // return eina_array_iterator_new(pd->properties_names);
  return EINA_C_ARRAY_ITERATOR_NEW(_efl_mono_model_properties_names);
}

/**
 * @brief Adds a new child item to the model.
 *
 * Creates a new Efl_Mono_Model_Internal_Child object, initializes its data,
 * and adds it to the model's list of items.
 *
 * @param obj The Efl_Mono_Model_Internal object (parent).
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @return The newly created Efl_Object (Efl_Mono_Model_Internal_Child), or NULL on failure.
 */
static Efl_Object*
_efl_mono_model_internal_efl_model_child_add(Eo *obj, Efl_Mono_Model_Internal_Data *pd)
{
  Efl_Mono_Model_Internal_Child* child = efl_add (EFL_MONO_MODEL_INTERNAL_CHILD_CLASS, obj);
  assert (child != NULL);
  Efl_Mono_Model_Internal_Child_Data* pcd = efl_data_xref (child, EFL_MONO_MODEL_INTERNAL_CHILD_CLASS, obj);
  pcd->model_pd = pd;
  pcd->index = eina_array_count_get(pd->items);
  pcd->child = child;
  pcd->values = eina_array_new(5);
  eina_array_push (pd->items, pcd);

  return child;
}

/**
 * @brief Gets the number of child items in the model.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @return The count of child items.
 */
static unsigned int
_efl_mono_model_internal_efl_model_children_count_get(const Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Data *pd)
{
  return eina_array_count_get(pd->items);
}

/**
 * @brief Gets a slice of child items from the model.
 *
 * Retrieves a range of child items and returns them as an Eina_Value array
 * wrapped in an Eina_Future.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @param start The starting index of the slice.
 * @param count The number of items in the slice.
 * @return An Eina_Future that resolves to an Eina_Value of type array,
 *         containing Efl_Object pointers to the child items.
 *         The Eina_Value array contains `Efl_Mono_Model_Internal_Child*` objects.
 *         Example: If children at `start` and `start+1` are `child_A` and `child_B`,
 *         the future resolves to an Eina_Value array `[child_A, child_B]`.
 */
static Eina_Future *
_efl_mono_model_internal_efl_model_children_slice_get(Eo *obj, Efl_Mono_Model_Internal_Data *pd, unsigned int start, unsigned int count EINA_UNUSED)
{
  unsigned int i;
  Eina_Value array = EINA_VALUE_EMPTY;
  Efl_Mono_Model_Internal_Child_Data* pcd;

  eina_value_array_setup(&array, EINA_VALUE_TYPE_OBJECT, count % 8);

  for (i = start; i != start + count; ++i)
  {
    pcd = eina_array_data_get(pd->items, i);
    eina_value_array_append (&array, pcd->child);
  }

  return efl_loop_future_resolved(obj, array);
}

/**
 * @brief Sets a property on the model itself.
 * @note This function is currently not implemented and returns ENOSYS.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set for the property.
 * @return An Eina_Future that resolves on success or rejects on error.
 */
static Eina_Future *
_efl_mono_model_internal_efl_model_property_set(Eo *obj, Efl_Mono_Model_Internal_Data *pd EINA_UNUSED, const char *property EINA_UNUSED, Eina_Value *value EINA_UNUSED)
{
  return efl_loop_future_rejected(obj, ENOSYS);
}

/**
 * @brief Gets a property from the model itself.
 * @note This function is currently not implemented and returns an Eina_Value error with ENOSYS.
 *
 * @param obj The Efl_Mono_Model_Internal object.
 * @param pd The private data for the Efl_Mono_Model_Internal instance.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value, or an Eina_Value error on failure.
 */
static Eina_Value *
_efl_mono_model_internal_efl_model_property_get(const Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Data *pd EINA_UNUSED, const char *property EINA_UNUSED)
{
  return eina_value_error_new(ENOSYS);
}

/// Model_Internal_Child implementations

/**
 * @brief Sets a property on a child item.
 *
 * Finds the property by name, and if found, updates its value in the child's
 * `values` array. The `values` array is expanded if necessary to accommodate
 * the property at its correct index.
 *
 * @param obj The Efl_Mono_Model_Internal_Child object.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set for the property.
 * @return An Eina_Future that resolves with a copy of the set Eina_Value on success,
 *         or rejects with EAGAIN if the property is not found.
 */
static Eina_Future *
_efl_mono_model_internal_child_efl_model_property_set(Eo *obj, Efl_Mono_Model_Internal_Child_Data *pd, const char *property, Eina_Value *value)
{
  int i = _find_property_index (property, pd->model_pd->properties_names);
  int j;
  Eina_Value* old_value;
  Eina_Value* new_value;
  Eina_Value tmp_value;

  if (i >= 0)
  {
    for (j = i - eina_array_count_get (pd->values); j >= 0; --j)
    {
      eina_array_push (pd->values, (void*)1);
      pd->values->data[pd->values->count-1] = NULL;
    }

    old_value = eina_array_data_get (pd->values, i);
    if (old_value)
      eina_value_free (old_value);
    new_value = malloc (sizeof(Eina_Value));
    eina_value_copy (value, new_value);
    eina_value_copy (value, &tmp_value);
    eina_array_data_set (pd->values, i, new_value);


    return efl_loop_future_resolved(obj, tmp_value);
  }
  else
  {
    // not found property
    return efl_loop_future_rejected(obj, EAGAIN);
  }
}

/**
 * @brief Gets a property from a child item.
 *
 * Finds the property by name and retrieves its value from the child's `values` array.
 *
 * @param obj The Efl_Mono_Model_Internal_Child object.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 * @param property The name of the property to get.
 * @return A new Eina_Value containing a copy of the property's value if found and set.
 *         Returns an Eina_Value error with EAGAIN if the property is not found,
 *         not set, or if the index is out of bounds. The caller is responsible for freeing the returned Eina_Value.
 */
static Eina_Value *
_efl_mono_model_internal_child_efl_model_property_get(const Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Child_Data *pd EINA_UNUSED, const char *property EINA_UNUSED)
{
  unsigned int i = _find_property_index (property, pd->model_pd->properties_names);
  if(eina_array_count_get (pd->values) <= i
     || eina_array_data_get (pd->values, i) == NULL)
    return eina_value_error_new(EAGAIN);
  else
    {
      Eina_Value* src = eina_array_data_get(pd->values, i);
      Eina_Value* clone = malloc (sizeof(Eina_Value)); // Caller must free this
      eina_value_copy (src, clone);
      return clone;
    }
}

/**
 * @brief Constructor for Efl_Mono_Model_Internal_Child.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 * @return The constructed Eo object, or NULL on failure.
 */
static Eo *
_efl_mono_model_internal_child_efl_object_constructor(Eo *obj, Efl_Mono_Model_Internal_Child_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_MONO_MODEL_INTERNAL_CHILD_CLASS));

   return obj;
}

/**
 * @brief Destructor for Efl_Mono_Model_Internal_Child.
 *
 * Frees resources allocated by the child item, including its property values.
 *
 * @param obj The Eo object to destruct.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 */
static void
_efl_mono_model_internal_child_efl_object_destructor(Eo *obj, Efl_Mono_Model_Internal_Child_Data *pd EINA_UNUSED)
{
   // TODO: Free contents of pd->values
   // Eina_Value *val;
   // EINA_ARRAY_FOREACH(pd->values, val) if (val) eina_value_free(val);
   // eina_array_free(pd->values);
   efl_destructor(efl_super(obj, EFL_MONO_MODEL_INTERNAL_CHILD_CLASS));
}

/**
 * @brief Adds a child to a child item.
 * @note This function is not supported for child items and will abort.
 *       Child items are considered leaf nodes in this model structure.
 *
 * @param obj The Efl_Mono_Model_Internal_Child object.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 * @return This function does not return normally; it calls abort().
 */
static Efl_Object*
_efl_mono_model_internal_child_efl_model_child_add(Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Child_Data *pd EINA_UNUSED)
{
  abort(); // Children of children are not supported by this model
  return NULL;
}

/**
 * @brief Gets an iterator over the names of the properties supported by this child item.
 *
 * The properties are defined by the parent model.
 *
 * @param obj The Efl_Mono_Model_Internal_Child object.
 * @param pd The private data for the Efl_Mono_Model_Internal_Child instance.
 * @return An Eina_Iterator that yields const char* property names,
 *         derived from the parent model's `properties_names` array.
 */
static Eina_Iterator *
_efl_mono_model_internal_child_efl_model_properties_get(const Eo *obj EINA_UNUSED, Efl_Mono_Model_Internal_Child_Data *pd)
{
  return eina_array_iterator_new (pd->model_pd->properties_names);
}

#include "efl_mono_model_internal.eo.c"
#include "efl_mono_model_internal_child.eo.c"
