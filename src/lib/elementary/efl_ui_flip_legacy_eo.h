#ifndef _EFL_UI_FLIP_LEGACY_EO_H_
#define _EFL_UI_FLIP_LEGACY_EO_H_

#ifndef _EFL_UI_FLIP_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_FLIP_LEGACY_EO_CLASS_TYPE

/**
 * @brief Opaque handle to the Efl_Ui_Flip_Legacy class instance.
 * @ingroup Efl_Ui_Flip_Legacy
 */
typedef Eo Efl_Ui_Flip_Legacy;

#endif

#ifndef _EFL_UI_FLIP_LEGACY_EO_TYPES
#define _EFL_UI_FLIP_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/**
 * @brief Get the Efl_Ui_Flip_Legacy class.
 *
 * This is a macro that calls efl_ui_flip_legacy_class_get().
 *
 * @ingroup Efl_Ui_Flip_Legacy
 */
#define EFL_UI_FLIP_LEGACY_CLASS efl_ui_flip_legacy_class_get()

/**
 * @brief Retrieves the Efl_Ui_Flip_Legacy class.
 *
 * @return The Efl_Ui_Flip_Legacy class.
 * @ingroup Efl_Ui_Flip_Legacy
 */
EWAPI const Efl_Class *efl_ui_flip_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
