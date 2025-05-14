/**
 * @file
 * @brief Legacy Efl_Ui_Win_Socket class implementation.
 *
 * This file contains the implementation of the Efl_Ui_Win_Socket_Legacy class,
 * which provides a legacy window socket functionality.
 */

Efl_Object *_efl_ui_win_socket_legacy_efl_object_finalize(Eo *obj, void *pd);

/**
 * @internal
 * @brief Initializes the Efl_Ui_Win_Socket_Legacy class.
 *
 * This function is called once when the class is first used.
 * It sets up the operations and reflections for the class.
 *
 * @param klass The class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_win_socket_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_WIN_SOCKET_LEGACY_EXTRA_OPS
#define EFL_UI_WIN_SOCKET_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_finalize, _efl_ui_win_socket_legacy_efl_object_finalize),
      EFL_UI_WIN_SOCKET_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Efl_Ui_Win_Socket_Legacy class.
 *
 * This structure contains metadata about the Efl_Ui_Win_Socket_Legacy class,
 * such as its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_win_socket_legacy_class_desc = {
   EO_VERSION,
   "Efl.Ui.Win_Socket_Legacy",
   EFL_CLASS_TYPE_REGULAR,
   0,
   _efl_ui_win_socket_legacy_class_initializer,
   _efl_ui_win_socket_legacy_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Defines the Efl_Ui_Win_Socket_Legacy class.
 *
 * This macro defines the Efl_Ui_Win_Socket_Legacy class, associating it with its
 * description, parent class (EFL_UI_WIN_SOCKET_CLASS), and implemented interfaces
 * (EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(efl_ui_win_socket_legacy_class_get, &_efl_ui_win_socket_legacy_class_desc, EFL_UI_WIN_SOCKET_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
