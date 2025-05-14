/**
 * @internal
 * @brief Legacy constructor for Efl_Ui_Check.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return The new object, or @c NULL on errors.
 */
Efl_Object *_efl_ui_check_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Legacy theme apply function for Efl_Ui_Check.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _efl_ui_check_legacy_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Legacy sub-object deletion function for Efl_Ui_Check.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] sub_obj The sub-object to delete.
 * @return #EINA_TRUE if the sub-object was deleted, #EINA_FALSE otherwise.
 */
Eina_Bool _efl_ui_check_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Legacy part get function for Efl_Ui_Check.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] name The name of the part to get.
 * @return The part object, or @c NULL if not found.
 */
Efl_Object *_efl_ui_check_legacy_efl_part_part_get(const Eo *obj, void *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Check_Legacy class.
 *
 * This function sets up the operations for the Efl_Ui_Check_Legacy class.
 *
 * @param[in] klass The class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_check_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_CHECK_LEGACY_EXTRA_OPS
#define EFL_UI_CHECK_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_check_legacy_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _efl_ui_check_legacy_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _efl_ui_check_legacy_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_part_get, _efl_ui_check_legacy_efl_part_part_get),
      EFL_UI_CHECK_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl_Ui_Check_Legacy class.
 *
 * This structure contains metadata about the Efl_Ui_Check_Legacy class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_check_legacy_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Efl.Ui.Check_Legacy", /**< The full name of this class. */
   EFL_CLASS_TYPE_REGULAR, /**< Specifies that this is a regular EFL class. */
   0,
   _efl_ui_check_legacy_class_initializer,
   _efl_ui_check_legacy_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_check_legacy_class_get, &_efl_ui_check_legacy_class_desc, EFL_UI_CHECK_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
