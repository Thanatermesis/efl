/**
 * @file
 * @brief These routines are legacy routines for the Efl Ui Radio widget.
 *
 * This is an internal Eo file, not to be used from application code.
 *
 * @ingroup Efl_Ui_Radio_Legacy
 */

Efl_Object *_efl_ui_radio_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor method.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 *
 * @return The new object instance.
 */
Eina_Error _efl_ui_radio_legacy_efl_ui_widget_theme_apply(Eo *obj, void *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply method.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 *
 * @return #EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Bool _efl_ui_radio_legacy_efl_ui_widget_widget_sub_object_del(Eo *obj, void *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_sub_object_del method.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] sub_obj The sub_obj to be deleted.
 *
 * @return #EINA_TRUE if the sub_obj was deleted, #EINA_FALSE otherwise.
 */
Efl_Object *_efl_ui_radio_legacy_efl_part_part_get(const Eo *obj, void *pd, const char *name);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get method.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data.
 * @param[in] name The name of the part to get.
 *
 * @return The part object, or @c NULL if not found.
 */
static Eina_Bool
_efl_ui_radio_legacy_class_initializer(Efl_Class *klass)
/**
 * @internal
 * @brief Initializes the Efl.Ui.Radio_Legacy class.
 *
 * This function is called once when the class is first used.
 * It sets up the operations (methods) for the class.
 *
 * @param[in] klass The class to initialize.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_RADIO_LEGACY_EXTRA_OPS
#define EFL_UI_RADIO_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_radio_legacy_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _efl_ui_radio_legacy_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _efl_ui_radio_legacy_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_part_get, _efl_ui_radio_legacy_efl_part_part_get),
      EFL_UI_RADIO_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl.Ui.Radio_Legacy class.
 *
 * This structure provides metadata about the class, such as its version,
 * name, type, and initializer/constructor functions.
 */
static const Efl_Class_Description _efl_ui_radio_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Radio_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_radio_legacy_class_initializer,
   _efl_ui_radio_legacy_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Gets the Efl.Ui.Radio_Legacy class.
 *
 * This macro defines the efl_ui_radio_legacy_class_get() function, which
 * returns a pointer to the Efl.Ui.Radio_Legacy class description.
 * It also specifies the parent class and any mixins or interfaces.
 *
 * @ingroup Efl_Ui_Radio_Legacy
 */
EFL_DEFINE_CLASS(efl_ui_radio_legacy_class_get, &_efl_ui_radio_legacy_class_desc, EFL_UI_RADIO_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
