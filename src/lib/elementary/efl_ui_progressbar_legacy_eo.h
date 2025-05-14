/**
 * @file
 * @brief These routines are legacy routines for the Efl Ui Progressbar.
 */
#ifndef _EFL_UI_PROGRESSBAR_LEGACY_EO_H_
#define _EFL_UI_PROGRESSBAR_LEGACY_EO_H_

#ifndef _EFL_UI_PROGRESSBAR_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_PROGRESSBAR_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Efl Ui Progressbar Legacy object.
 * @ingroup Efl_Ui_Progressbar_Legacy
 */
typedef Eo Efl_Ui_Progressbar_Legacy;

#endif

#ifndef _EFL_UI_PROGRESSBAR_LEGACY_EO_TYPES
#define _EFL_UI_PROGRESSBAR_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/** Elementary progressbar class
 *
 * @ingroup Efl_Ui_Progressbar_Legacy
 */
#define EFL_UI_PROGRESSBAR_LEGACY_CLASS efl_ui_progressbar_legacy_class_get()

EWAPI const Efl_Class *efl_ui_progressbar_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
