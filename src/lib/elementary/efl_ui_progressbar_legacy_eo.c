/**
 * @file
 * @brief Implementation of the Efl Ui Progressbar Legacy class.
 *
 * This file contains the internal methods and class construction for the
 * legacy progressbar widget.
 */

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Progressbar_Legacy object.
 *
 * @param[in] obj The object instance.
 * @param[in] pd The private data for the object.
 * @return The new object instance, or @c NULL on failure.
 */
Efl_Object *_efl_ui_progressbar_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Applies the theme to the Efl_Ui_Progressbar_Legacy widget.
 *
 * @param[in] obj The object instance.
 * @param[in] pd The private data for the object.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _efl_ui_progressbar_legacy_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Handles the deletion of a sub-object from the Efl_Ui_Progressbar_Legacy widget.
 *
 * @param[in] obj The object instance.
 * @param[in] pd The private data for the object.
 * @param[in] sub_obj The sub-object to delete.
 * @return @c EINA_TRUE if the sub-object was successfully deleted, @c EINA_FALSE otherwise.
 */
Eina_Bool _efl_ui_progressbar_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Retrieves a part from the Efl_Ui_Progressbar_Legacy widget.
 *
 * @param[in] obj The object instance.
 * @param[in] pd The private data for the object.
 * @param[in] name The name of the part to retrieve.
 * @return The part object, or @c NULL if the part is not found.
 */
Efl_Object *_efl_ui_progressbar_legacy_efl_part_part_get(const Eo *obj, void *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Progressbar_Legacy class.
 *
 * Sets up the operations and properties for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_progressbar_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_PROGRESSBAR_LEGACY_EXTRA_OPS
#define EFL_UI_PROGRESSBAR_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_progressbar_legacy_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _efl_ui_progressbar_legacy_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _efl_ui_progressbar_legacy_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_part_get, _efl_ui_progressbar_legacy_efl_part_part_get),
      EFL_UI_PROGRESSBAR_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _efl_ui_progressbar_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Progressbar_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_progressbar_legacy_class_initializer,
   _efl_ui_progressbar_legacy_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_progressbar_legacy_class_get, &_efl_ui_progressbar_legacy_class_desc, EFL_UI_PROGRESSBAR_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
