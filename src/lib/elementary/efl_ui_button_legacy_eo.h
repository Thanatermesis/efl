/**
 * @file
 * @brief These routines are legacy routines for the Efl Ui Button class.
 *
 * Users should use the new APIs whenever possible.
 *
 * @ingroup Efl_Ui_Button_Legacy
 */
#ifndef _EFL_UI_BUTTON_LEGACY_EO_H_
#define _EFL_UI_BUTTON_LEGACY_EO_H_

#ifndef _EFL_UI_BUTTON_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_BUTTON_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Efl Ui Button Legacy instance.
 * @ingroup Efl_Ui_Button_Legacy
 */
typedef Eo Efl_Ui_Button_Legacy;

#endif

#ifndef _EFL_UI_BUTTON_LEGACY_EO_TYPES
#define _EFL_UI_BUTTON_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/**
 * @brief Push-button widget
 *
 * Press it and run some function. It can contain a simple label and icon
 * object and it also has an autorepeat feature.
 *
 * @ingroup Efl_Ui_Button_Legacy
 */
#define EFL_UI_BUTTON_LEGACY_CLASS efl_ui_button_legacy_class_get()

EWAPI const Efl_Class *efl_ui_button_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
