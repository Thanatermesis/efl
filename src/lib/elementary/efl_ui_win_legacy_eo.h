/**
 * @file
 * @brief These routines are legacy routines for the Efl Ui Win Legacy class.
 *
 * This is a legacy class for Efl.Ui.Win. It should not be used in new code.
 *
 * @ingroup Efl_Ui_Win_Legacy
 */
#ifndef _EFL_UI_WIN_LEGACY_EO_H_
#define _EFL_UI_WIN_LEGACY_EO_H_

#ifndef _EFL_UI_WIN_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_WIN_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Efl Ui Win Legacy instance.
 * @ingroup Efl_Ui_Win_Legacy
 */
typedef Eo Efl_Ui_Win_Legacy;

#endif

#ifndef _EFL_UI_WIN_LEGACY_EO_TYPES
#define _EFL_UI_WIN_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/** Efl UI window class
 *
 * @ingroup Efl_Ui_Win_Legacy
 */
#define EFL_UI_WIN_LEGACY_CLASS efl_ui_win_legacy_class_get()

/**
 * @brief Get the Efl_Class for Efl.Ui.Win_Legacy.
 *
 * @return The Efl_Class for Efl.Ui.Win_Legacy.
 *
 * @ingroup Efl_Ui_Win_Legacy
 */
EWAPI const Efl_Class *efl_ui_win_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
