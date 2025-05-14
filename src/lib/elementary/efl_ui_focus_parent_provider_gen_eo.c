/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_gen_content_item_map_set.
 */
void _efl_ui_focus_parent_provider_gen_content_item_map_set(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Eina_Hash *map);

EOAPI EFL_VOID_FUNC_BODYV(efl_ui_focus_parent_provider_gen_content_item_map_set, EFL_FUNC_CALL(map), Eina_Hash *map);
/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_gen_content_item_map_get.
 */
Eina_Hash *_efl_ui_focus_parent_provider_gen_content_item_map_get(const Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(efl_ui_focus_parent_provider_gen_content_item_map_get, Eina_Hash *, NULL);
/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_gen_container_set.
 */
void _efl_ui_focus_parent_provider_gen_container_set(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Efl_Ui_Widget *container);

EOAPI EFL_VOID_FUNC_BODYV(efl_ui_focus_parent_provider_gen_container_set, EFL_FUNC_CALL(container), Efl_Ui_Widget *container);
/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_gen_container_get.
 */
Efl_Ui_Widget *_efl_ui_focus_parent_provider_gen_container_get(const Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(efl_ui_focus_parent_provider_gen_container_get, Efl_Ui_Widget *, NULL);
/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_gen_item_fetch.
 */
Efl_Ui_Widget *_efl_ui_focus_parent_provider_gen_item_fetch(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Efl_Ui_Widget *widget);

EOAPI EFL_FUNC_BODYV(efl_ui_focus_parent_provider_gen_item_fetch, Efl_Ui_Widget *, NULL, EFL_FUNC_CALL(widget), Efl_Ui_Widget *widget);
/**
 * @internal
 * @brief Internal implementation for efl_ui_focus_parent_provider_find_logical_parent.
 */
Efl_Ui_Focus_Object *_efl_ui_focus_parent_provider_gen_efl_ui_focus_parent_provider_find_logical_parent(Eo *obj, Efl_Ui_Focus_Parent_Provider_Gen_Data *pd, Efl_Ui_Focus_Object *widget);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Focus_Parent_Provider_Gen class.
 *
 * This function is called once during class construction to set up the
 * Efl_Object operations and other class-specific initializations.
 *
 * @param klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_focus_parent_provider_gen_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_FOCUS_PARENT_PROVIDER_GEN_EXTRA_OPS
#define EFL_UI_FOCUS_PARENT_PROVIDER_GEN_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_gen_content_item_map_set, _efl_ui_focus_parent_provider_gen_content_item_map_set),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_gen_content_item_map_get, _efl_ui_focus_parent_provider_gen_content_item_map_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_gen_container_set, _efl_ui_focus_parent_provider_gen_container_set),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_gen_container_get, _efl_ui_focus_parent_provider_gen_container_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_gen_item_fetch, _efl_ui_focus_parent_provider_gen_item_fetch),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_parent_provider_find_logical_parent, _efl_ui_focus_parent_provider_gen_efl_ui_focus_parent_provider_find_logical_parent),
      EFL_UI_FOCUS_PARENT_PROVIDER_GEN_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}
/**
 * @internal
 * @brief Describes the Efl_Ui_Focus_Parent_Provider_Gen class.
 *
 * This static structure holds metadata about the Efl_Ui_Focus_Parent_Provider_Gen class,
 * including its version, name, type, instance size, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_focus_parent_provider_gen_class_desc = {
   EO_VERSION,
   "Efl.Ui.Focus.Parent_Provider_Gen",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Efl_Ui_Focus_Parent_Provider_Gen_Data),
   _efl_ui_focus_parent_provider_gen_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_focus_parent_provider_gen_class_get, &_efl_ui_focus_parent_provider_gen_class_desc, EFL_OBJECT_CLASS, EFL_UI_FOCUS_PARENT_PROVIDER_INTERFACE, NULL);
