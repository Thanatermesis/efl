/**
 * @file
 * @brief These routines are legacy routines from the old EFL UI BG.
 */
#ifndef _EFL_UI_BG_LEGACY_EO_H_
#define _EFL_UI_BG_LEGACY_EO_H_

#ifndef _EFL_UI_BG_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_BG_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to Efl_Ui_Bg_Legacy instances.
 * @ingroup Efl_Ui_Bg_Legacy
 */
typedef Eo Efl_Ui_Bg_Legacy;

#endif

#ifndef _EFL_UI_BG_LEGACY_EO_TYPES
#define _EFL_UI_BG_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/**
 * @brief The bg (background) widget is used for setting (solid) background
 * decorations
 *
 * for a window (unless it has transparency enabled) or for any container
 * object. It works just like an image, but has some properties useful for
 * backgrounds, such as setting it to tiled, centered, scaled or stretched.
 *
 * @ingroup Efl_Ui_Bg_Legacy
 */
#define EFL_UI_BG_LEGACY_CLASS efl_ui_bg_legacy_class_get()

/**
 * @brief Get the Efl_Class for Efl_Ui_Bg_Legacy.
 *
 * @return The Efl_Class for Efl_Ui_Bg_Legacy.
 * @ingroup Efl_Ui_Bg_Legacy
 */
EWAPI const Efl_Class *efl_ui_bg_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
