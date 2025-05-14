#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

/** @internal
 * @brief Property name for item width. */
const char *_efl_model_property_itemw = "item.width";
/** @internal
 * @brief Property name for item height. */
const char *_efl_model_property_itemh = "item.height";
/** @internal
 * @brief Property name for self width. */
const char *_efl_model_property_selfw = "self.width";
/** @internal
 * @brief Property name for self height. */
const char *_efl_model_property_selfh = "self.height";
/** @internal
 * @brief Property name for total width. */
const char *_efl_model_property_totalw = "total.width";
/** @internal
 * @brief Property name for total height. */
const char *_efl_model_property_totalh = "total.height";

/**
 * @internal
 * @brief Provides an iterator for properties specific to child size models.
 *
 * These properties include item width, item height, self height, and self width.
 *
 * @return An Eina_Iterator for the child properties.
 *   The iterator will yield strings:
 *   - "item.width"
 *   - "item.height"
 *   - "self.height"
 *   - "self.width"
 */
static Eina_Iterator *
_efl_ui_size_model_properties_child(void)
{
   const char *properties[] = {
     _efl_model_property_itemw, _efl_model_property_itemh, _efl_model_property_selfh, _efl_model_property_selfw
   };
   return EINA_C_ARRAY_ITERATOR_NEW(properties);
}

/**
 * @internal
 * @brief Provides an iterator for properties specific to root size models.
 *
 * These properties include item width and item height.
 *
 * @return An Eina_Iterator for the root properties.
 *   The iterator will yield strings:
 *   - "item.width"
 *   - "item.height"
 */
static Eina_Iterator *
_efl_ui_size_model_properties_root(void)
{
   const char *properties[] = {
     _efl_model_property_itemw, _efl_model_property_itemh
   };
   return EINA_C_ARRAY_ITERATOR_NEW(properties);
}

/**
 * @internal
 * @brief Gets all properties for a size model.
 *
 * This function combines properties from the superclass with properties specific
 * to this size model. It differentiates between root and child models to provide
 * the correct set of additional properties.
 *
 * @param obj The Efl_Ui_Size_Model object.
 * @param pd Private data for the Efl_Ui_Size_Model class (unused).
 * @return An Eina_Iterator containing all relevant property names.
 *   The iterator will yield strings representing property names. For example:
 *   - Properties from efl_model_properties_get(efl_super(obj, EFL_UI_SIZE_MODEL_CLASS))
 *   - Plus, if it's a child model (parent is also EFL_UI_SIZE_MODEL_CLASS):
 *     - "item.width"
 *     - "item.height"
 *     - "self.height"
 *     - "self.width"
 *   - Or, if it's a root model:
 *     - "item.width"
 *     - "item.height"
 */
static Eina_Iterator *
_efl_ui_size_model_efl_model_properties_get(const Eo *obj, void *pd EINA_UNUSED)
{
   Eina_Iterator *super;
   Eina_Iterator *prop;

   super = efl_model_properties_get(efl_super(obj, EFL_UI_SIZE_MODEL_CLASS));
   if (efl_isa(efl_parent_get(obj), EFL_UI_SIZE_MODEL_CLASS))
     prop = _efl_ui_size_model_properties_child();
   else
     prop = _efl_ui_size_model_properties_root();

   return eina_multi_iterator_new(super, prop);
}

#include "efl_ui_size_model.eo.c"
