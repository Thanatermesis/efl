/**
 * @file
 * @brief Implementation of the Efl Ui Button Legacy class.
 *
 * This file contains the C implementation for the legacy button widget.
 * It handles the object construction, theme application, sub-object management,
 * and part retrieval for the legacy button.
 *
 * @ingroup Efl_Ui_Button_Legacy
 */

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Button_Legacy object.
 *
 * This function is called when a new Efl_Ui_Button_Legacy object is created.
 * It initializes the object and its private data.
 *
 * @param[in] obj The Efl object to construct.
 * @param[in] pd The private data for the object.
 * @return The constructed Efl object.
 */
Efl_Object *_efl_ui_button_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Applies the theme to the Efl_Ui_Button_Legacy widget.
 *
 * This function is called when the theme of the widget needs to be updated.
 *
 * @param[in] obj The Efl object.
 * @param[in] pd The private data for the object.
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _efl_ui_button_legacy_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Deletes a sub-object of the Efl_Ui_Button_Legacy widget.
 *
 * This function is called when a sub-object of the widget is deleted.
 *
 * @param[in] obj The Efl object.
 * @param[in] pd The private data for the object.
 * @param[in] sub_obj The sub-object to delete.
 * @return #EINA_TRUE if the sub-object was successfully deleted, #EINA_FALSE otherwise.
 */
Eina_Bool _efl_ui_button_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Gets a part of the Efl_Ui_Button_Legacy widget.
 *
 * This function is used to retrieve a named part of the widget, such as "icon" or "label".
 *
 * @param[in] obj The Efl object.
 * @param[in] pd The private data for the object.
 * @param[in] name The name of the part to retrieve.
 * @return The Efl object representing the part, or @c NULL if the part is not found.
 */
Efl_Object *_efl_ui_button_legacy_efl_part_part_get(const Eo *obj, void *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Button_Legacy class.
 *
 * This function is called once when the class is first loaded.
 * It sets up the operations (member functions) for the class.
 *
 * @param[in] klass The Efl class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_button_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_BUTTON_LEGACY_EXTRA_OPS
#define EFL_UI_BUTTON_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_button_legacy_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _efl_ui_button_legacy_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _efl_ui_button_legacy_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_part_get, _efl_ui_button_legacy_efl_part_part_get),
      EFL_UI_BUTTON_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _efl_ui_button_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Button_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_button_legacy_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_button_legacy_class_get, &_efl_ui_button_legacy_class_desc, EFL_UI_BUTTON_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
