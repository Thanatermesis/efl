#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

// For now only vertical logic is implemented. Horizontal list and grid are not supported.

/**
 * @internal
 * @brief Private data for the Efl_Ui_Homogeneous_Model class.
 *
 * This structure holds the data necessary for managing a homogeneous model,
 * where all items are assumed to have the same dimensions.
 */
typedef struct _Efl_Ui_Homogeneous_Model_Data Efl_Ui_Homogeneous_Model_Data;
struct _Efl_Ui_Homogeneous_Model_Data
{
   Efl_Ui_Homogeneous_Model_Data *parent; /**< Pointer to the parent model's data, if this is a sub-model. */

   struct {
      unsigned int width; /**< Width of a single item. */
      unsigned int height; /**< Height of a single item. */

      struct {
         Eina_Bool width; /**< Flag indicating if item width has been defined. */
         Eina_Bool height; /**< Flag indicating if item height has been defined. */
      } defined; /**< Flags to track if item dimensions are set. */
   } item; /**< Homogeneous item properties. */
};

/**
 * @internal
 * @brief Sets a specific dimension property (width or height) for an item.
 *
 * This function is a helper to set either the width or height of items
 * in the homogeneous model. It ensures that the property is set only once.
 *
 * @param[in] obj The Efl_Model object.
 * @param[in] value The Eina_Value containing the new dimension (unsigned int).
 * @param[out] defined Pointer to a boolean flag that tracks if the dimension has been set.
 * @param[out] r Pointer to store the dimension value.
 * @return A resolved Eina_Future on success, or a rejected one on error (e.g., read-only, incorrect value).
 */
static Eina_Future *
_efl_ui_homogeneous_model_property_set(Eo *obj, Eina_Value *value,
                                       Eina_Bool *defined, unsigned int *r)
{
   Eina_Future *f;

   if (*defined)
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
   if (!eina_value_uint_convert(value, r))
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
   *defined = EINA_TRUE;
   f = efl_loop_future_resolved(obj, *value);
   return f;
}

/**
 * @internal
 * @brief Implements Efl_Model_Property_Set for Efl_Ui_Homogeneous_Model.
 *
 * Handles setting properties like item width/height. If a parent model exists,
 * it can also set properties related to the parent's item dimensions (selfw, selfh).
 * Total width/height properties are read-only.
 *
 * @param[in] obj The Efl_Model object.
 * @param[in] pd Private data for the homogeneous model.
 * @param[in] property The name of the property to set.
 * @param[in] value The Eina_Value to set for the property.
 * @return A resolved Eina_Future on success, or a rejected one on error.
 */
static Eina_Future *
_efl_ui_homogeneous_model_efl_model_property_set(Eo *obj,
                                                 Efl_Ui_Homogeneous_Model_Data *pd,
                                                 const char *property, Eina_Value *value)
{
   // If this model has a parent, certain properties relate to the parent's item dimensions.
   if (pd->parent)
     {
        if (eina_streq(property, _efl_model_property_selfw))
          return _efl_ui_homogeneous_model_property_set(obj, value,
                                                        &pd->parent->item.defined.width,
                                                        &pd->parent->item.width);
        if (eina_streq(property, _efl_model_property_selfh))
          return _efl_ui_homogeneous_model_property_set(obj, value,
                                                        &pd->parent->item.defined.height,
                                                        &pd->parent->item.height);
        if (eina_streq(property, _efl_model_property_totalw) ||
            eina_streq(property, _efl_model_property_totalh))
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
     }
   if (eina_streq(property, _efl_model_property_itemw))
     {
        return _efl_ui_homogeneous_model_property_set(obj, value,
                                                      &pd->item.defined.width,
                                                      &pd->item.width);
     }
   if (eina_streq(property, _efl_model_property_itemh))
     {
        return _efl_ui_homogeneous_model_property_set(obj, value,
                                                      &pd->item.defined.height,
                                                      &pd->item.height);
     }

   return efl_model_property_set(efl_super(obj, EFL_UI_HOMOGENEOUS_MODEL_CLASS),
                                 property, value);
}

/**
 * @internal
 * @brief Implements Efl_Model_Property_Get for Efl_Ui_Homogeneous_Model.
 *
 * Retrieves properties such as item width/height, total width/height.
 * If a parent model exists, it can also retrieve properties related to the
 * parent's item dimensions (selfw, selfh).
 * If a requested dimension is not yet defined, it returns an error with EAGAIN.
 *
 * @param[in] obj The Efl_Model object.
 * @param[in] pd Private data for the homogeneous model.
 * @param[in] property The name of the property to get.
 * @return An Eina_Value containing the property value on success, or an Eina_Value
 *         error (e.g., EAGAIN if not yet defined).
 */
static Eina_Value *
_efl_ui_homogeneous_model_efl_model_property_get(const Eo *obj,
                                                 Efl_Ui_Homogeneous_Model_Data *pd,
                                                 const char *property)
{
   // If this model has a parent, certain properties relate to the parent's item dimensions.
   if (pd->parent)
     {
        if (eina_streq(property, _efl_model_property_selfw))
          {
             if (pd->parent->item.defined.width)
               return eina_value_uint_new(pd->parent->item.width);
             goto not_ready;
          }
        if (eina_streq(property, _efl_model_property_selfh))
          {
             if (pd->parent->item.defined.height)
               return eina_value_uint_new(pd->parent->item.height);
             goto not_ready;
          }
     }
   if (eina_streq(property, _efl_model_property_itemw))
     {
        if (pd->item.defined.width)
          return eina_value_uint_new(pd->item.width);
        goto not_ready;
     }
   if (eina_streq(property, _efl_model_property_itemh))
     {
        if (pd->item.defined.height)
          return eina_value_uint_new(pd->item.height);
        goto not_ready;
     }
   if (eina_streq(property, _efl_model_property_totalh))
     {
        if (pd->item.defined.height)
          return eina_value_uint_new(pd->item.height *
                                     efl_model_children_count_get(obj));
        goto not_ready;
     }
   if (eina_streq(property, _efl_model_property_totalw))
     {
        if (pd->item.defined.width)
          // We only handle vertical list at this point, so total width is the width of one item.
          return eina_value_uint_new(pd->item.width);
        goto not_ready;
     }

   return efl_model_property_get(efl_super(obj, EFL_UI_HOMOGENEOUS_MODEL_CLASS), property);

 not_ready:
   return eina_value_error_new(EAGAIN);
}

/**
 * @internal
 * @brief Constructor for Efl_Ui_Homogeneous_Model.
 *
 * Initializes the homogeneous model. If the model is a child of another
 * Efl_Ui_Homogeneous_Model, it links to the parent's private data.
 * This allows child models to access properties (like item dimensions)
 * defined by their parent.
 *
 * @param[in] obj The Efl_Object being constructed.
 * @param[in] pd Private data for the homogeneous model.
 * @return The constructed Efl_Object.
 */
static Efl_Object *
_efl_ui_homogeneous_model_efl_object_constructor(Eo *obj, Efl_Ui_Homogeneous_Model_Data *pd)
{
   Eo *parent = efl_parent_get(obj);

   // Check if the parent is also a homogeneous model. If so, store a reference
   // to its private data. This is useful for nested homogeneous structures
   // where child items might inherit or relate to parent item dimensions.
   if (parent && efl_isa(parent, EFL_UI_HOMOGENEOUS_MODEL_CLASS))
     pd->parent = efl_data_scope_get(efl_parent_get(obj), EFL_UI_HOMOGENEOUS_MODEL_CLASS);

   return efl_constructor(efl_super(obj, EFL_UI_HOMOGENEOUS_MODEL_CLASS));
}

#include "efl_ui_homogeneous_model.eo.c"
