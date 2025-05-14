/**
 * @file
 * @brief Legacy implementation for the Efl Ui Win Inlined object.
 *
 * This class provides a legacy inlined window, primarily for compatibility.
 * It inherits from Efl.Ui.Win_Inlined.
 */

Efl_Object *_efl_ui_win_inlined_legacy_efl_object_finalize(Eo *obj, void *pd);

/**
 * @brief Initializes the Efl_Ui_Win_Inlined_Legacy class.
 *
 * This function is called once when the class is first used.
 * It sets up the operations (methods) for the class.
 *
 * @param klass The class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_win_inlined_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_WIN_INLINED_LEGACY_EXTRA_OPS
#define EFL_UI_WIN_INLINED_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_finalize, _efl_ui_win_inlined_legacy_efl_object_finalize),
      EFL_UI_WIN_INLINED_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Class description for Efl_Ui_Win_Inlined_Legacy.
 *
 * This structure provides metadata about the class, such as its version,
 * name, type, and pointers to its initializer and constructor functions.
 */
static const Efl_Class_Description _efl_ui_win_inlined_legacy_class_desc = {
   EO_VERSION, /**< Class version. */
   "Efl.Ui.Win_Inlined_Legacy", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   0,
   _efl_ui_win_inlined_legacy_class_initializer,
   _efl_ui_win_inlined_legacy_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(efl_ui_win_inlined_legacy_class_get, &_efl_ui_win_inlined_legacy_class_desc, EFL_UI_WIN_INLINED_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
