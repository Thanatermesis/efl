/**
 * @file
 * @brief These routines are legacy routines for the Efl Ui Textpath widget.
 *
 * This is a legacy compatibility layer.
 *
 * @ingroup Efl_Ui_Textpath_Legacy
 */

Efl_Object *_efl_ui_textpath_legacy_efl_object_constructor(Eo *obj, void *pd);


/**
 * @internal
 * @brief Class initializer for Efl.Ui.Textpath_Legacy.
 *
 * This function is called once when the class is initialized.
 * It sets up the Efl_Object operations for the Efl.Ui.Textpath_Legacy class.
 *
 * @param[in] klass The class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_textpath_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_TEXTPATH_LEGACY_EXTRA_OPS
#define EFL_UI_TEXTPATH_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_textpath_legacy_efl_object_constructor),
      EFL_UI_TEXTPATH_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Class description for Efl.Ui.Textpath_Legacy.
 *
 * This structure provides metadata for the Efl.Ui.Textpath_Legacy class,
 * including its version, name, type, and initializer/constructor functions.
 */
static const Efl_Class_Description _efl_ui_textpath_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Textpath_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_textpath_legacy_class_initializer,
   _efl_ui_textpath_legacy_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_textpath_legacy_class_get, &_efl_ui_textpath_legacy_class_desc, EFL_UI_TEXTPATH_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
