/**
 * @file
 * @brief Implementation of the Efl Ui Win Legacy class.
 *
 * This file contains the implementation of the Efl.Ui.Win_Legacy Eo class,
 * which provides legacy window functionalities.
 *
 * @ingroup Efl_Ui_Win_Legacy
 */

/**
 * @internal
 * @brief Finalizes an Efl.Ui.Win_Legacy object.
 *
 * This function is called when an Efl.Ui.Win_Legacy object is being finalized.
 * It should perform any necessary cleanup operations.
 *
 * @param[in] obj The Efl.Ui.Win_Legacy object to finalize.
 * @param[in] pd Private data for the Efl.Ui.Win_Legacy object.
 * @return The finalized Efl_Object.
 */
Efl_Object *_efl_ui_win_legacy_efl_object_finalize(Eo *obj, void *pd);


/**
 * @internal
 * @brief Initializes the Efl.Ui.Win_Legacy class.
 *
 * This function is called when the Efl.Ui.Win_Legacy class is being initialized.
 * It sets up the operations and properties for the class.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_win_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_WIN_LEGACY_EXTRA_OPS
#define EFL_UI_WIN_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_finalize, _efl_ui_win_legacy_efl_object_finalize),
      EFL_UI_WIN_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl.Ui.Win_Legacy class.
 *
 * This structure contains metadata about the Efl.Ui.Win_Legacy class,
 * such as its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_win_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Win_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_win_legacy_class_initializer,
   _efl_ui_win_legacy_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Defines the Efl.Ui.Win_Legacy class.
 *
 * This macro defines the Efl.Ui.Win_Legacy class, associating it with its
 * class description, parent class, and any mixins or interfaces it implements.
 */
EFL_DEFINE_CLASS(efl_ui_win_legacy_class_get, &_efl_ui_win_legacy_class_desc, EFL_UI_WIN_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
