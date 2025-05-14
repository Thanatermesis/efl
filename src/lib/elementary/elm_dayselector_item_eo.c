/**
 * @file
 * @brief These routines are EFL Dayselector Item class private functions.
 *
 * Elm_Dayselector_Item is a an item for the Elm_Dayselector widget.
 * It is a themeable object that inherits from Elm_Widget_Item.
 */

Efl_Object *_elm_dayselector_item_efl_object_constructor(Eo *obj, Elm_Dayselector_Item_Data *pd);

/**
 * @internal
 * @brief Initializes a new instance of the Elm_Dayselector_Item class.
 *
 * This function is called when a new Elm_Dayselector_Item object is created.
 * It sets up the internal data structures and prepares the object for use.
 *
 * @param klass The Efl_Class of the object to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_dayselector_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_DAYSELECTOR_ITEM_EXTRA_OPS
#define ELM_DAYSELECTOR_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_dayselector_item_efl_object_constructor),
      ELM_DAYSELECTOR_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Dayselector_Item class.
 *
 * This structure provides metadata about the Elm_Dayselector_Item class,
 * including its version, name, type, size of instance data, and initializer
 * functions.
 */
static const Efl_Class_Description _elm_dayselector_item_class_desc = {
   EO_VERSION,
   "Elm.Dayselector.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Dayselector_Item_Data),
   _elm_dayselector_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_dayselector_item_class_get, &_elm_dayselector_item_class_desc, ELM_WIDGET_ITEM_CLASS, NULL);
