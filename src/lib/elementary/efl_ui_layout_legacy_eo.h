#ifndef _EFL_UI_LAYOUT_LEGACY_EO_H_
#define _EFL_UI_LAYOUT_LEGACY_EO_H_

/**
 * @file
 * @brief Efl Ui Layout Legacy class (Eo API)
 */

#ifndef _EFL_UI_LAYOUT_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_LAYOUT_LEGACY_EO_CLASS_TYPE

/**
 * @brief Represents a legacy UI layout object.
 * @ingroup Efl_Ui_Layout_Legacy
 */
typedef Eo Efl_Ui_Layout_Legacy;

#endif

#ifndef _EFL_UI_LAYOUT_LEGACY_EO_TYPES
#define _EFL_UI_LAYOUT_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/** Elementary layout class
 *
 * @ingroup Efl_Ui_Layout_Legacy
 */
#define EFL_UI_LAYOUT_LEGACY_CLASS efl_ui_layout_legacy_class_get()
/**
 * @brief Gets the Elm_Layout mixin class.
 * @ingroup Efl_Ui_Layout_Legacy
 */
#define ELM_LAYOUT_MIXIN elm_layout_mixin_get()

/**
 * @brief Retrieves the Efl_Ui_Layout_Legacy class.
 *
 * @return The Efl_Ui_Layout_Legacy class.
 * @ingroup Efl_Ui_Layout_Legacy
 */
EWAPI const Efl_Class *efl_ui_layout_legacy_class_get(void) EINA_CONST;
/**
 * @brief Retrieves the Elm_Layout mixin class.
 *
 * @return The Elm_Layout mixin class.
 * @ingroup Efl_Ui_Layout_Legacy
 */
EWAPI const Efl_Class *elm_layout_mixin_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
