#ifndef _EFL_UI_WIN_SOCKET_LEGACY_EO_H_
#define _EFL_UI_WIN_SOCKET_LEGACY_EO_H_

/**
 * @file
 * @brief Efl Ui Win Socket Legacy class
 */

#ifndef _EFL_UI_WIN_SOCKET_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_WIN_SOCKET_LEGACY_EO_CLASS_TYPE

/**
 * @brief Represents a legacy Elementary window socket object.
 * @ingroup Efl_Ui_Win_Socket_Legacy
 */
typedef Eo Efl_Ui_Win_Socket_Legacy;

#endif

#ifndef _EFL_UI_WIN_SOCKET_LEGACY_EO_TYPES
#define _EFL_UI_WIN_SOCKET_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/**
 * @brief Elementary window socket class.
 *
 * This class provides legacy support for window socket functionality.
 * It allows embedding an external application window within an EFL application.
 *
 * @ingroup Efl_Ui_Win_Socket_Legacy
 */
#define EFL_UI_WIN_SOCKET_LEGACY_CLASS efl_ui_win_socket_legacy_class_get()

/**
 * @brief Get the Efl_Class for Efl_Ui_Win_Socket_Legacy.
 *
 * @return The Efl_Class for Efl_Ui_Win_Socket_Legacy.
 * @ingroup Efl_Ui_Win_Socket_Legacy
 */
EWAPI const Efl_Class *efl_ui_win_socket_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
