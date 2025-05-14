EOAPI EFL_FUNC_BODY_CONST(elm_widget_item_container_focused_item_get, Elm_Widget_Item *, NULL);

/**
 * @brief Initializes the Elm_Widget_Item_Container interface class.
 *
 * @details This function is called once when the class is first constructed.
 * It sets up the vtable for the interface, binding the Efl_Object operations
 * to their implementations. In this case, it registers a default (NULL)
 * implementation for elm_widget_item_container_focused_item_get.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @internal
 */
static Eina_Bool
_elm_widget_item_container_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_WIDGET_ITEM_CONTAINER_EXTRA_OPS
#define ELM_WIDGET_ITEM_CONTAINER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_widget_item_container_focused_item_get, NULL),
      ELM_WIDGET_ITEM_CONTAINER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_widget_item_container_class_desc = {
   EO_VERSION,
   "Elm.Widget_Item_Container",
   EFL_CLASS_TYPE_INTERFACE,
   0,
   _elm_widget_item_container_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_widget_item_container_interface_get, &_elm_widget_item_container_class_desc, NULL, NULL);
