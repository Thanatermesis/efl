/**
 * @file
 * @brief These routines are legacy routines from the old EFL UI Panes.
 */
#ifndef _EFL_UI_PANES_LEGACY_EO_H_
#define _EFL_UI_PANES_LEGACY_EO_H_

#ifndef _EFL_UI_PANES_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_PANES_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Efl_Ui_Panes_Legacy instance.
 * @ingroup Efl_Ui_Panes_Legacy
 */
typedef Eo Efl_Ui_Panes_Legacy;

#endif

#ifndef _EFL_UI_PANES_LEGACY_EO_TYPES
#define _EFL_UI_PANES_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/** Elementary panes class
 *
 * @ingroup Efl_Ui_Panes_Legacy
 */
#define EFL_UI_PANES_LEGACY_CLASS efl_ui_panes_legacy_class_get()

/**
 * @brief Get the Efl_Ui_Panes_Legacy class.
 *
 * @return The Efl_Ui_Panes_Legacy class.
 * @ingroup Efl_Ui_Panes_Legacy
 */
EWAPI const Efl_Class *efl_ui_panes_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
