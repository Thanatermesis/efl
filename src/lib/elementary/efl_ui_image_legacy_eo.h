/**
 * @file
 * @brief Definition of the Efl.Ui.Image_Legacy class.
 * @ingroup Efl_Ui_Image_Legacy
 */
#ifndef _EFL_UI_IMAGE_LEGACY_EO_H_
#define _EFL_UI_IMAGE_LEGACY_EO_H_

#ifndef _EFL_UI_IMAGE_LEGACY_EO_CLASS_TYPE
#define _EFL_UI_IMAGE_LEGACY_EO_CLASS_TYPE
/**
 * @brief Represents a legacy Efl UI Image object.
 * @ingroup Efl_Ui_Image_Legacy
 */
typedef Eo Efl_Ui_Image_Legacy;

#endif

#ifndef _EFL_UI_IMAGE_LEGACY_EO_TYPES
#define _EFL_UI_IMAGE_LEGACY_EO_TYPES


#endif
#ifdef EFL_BETA_API_SUPPORT
/** Efl UI image class
 *
 * @ingroup Efl_Ui_Image_Legacy
 */
#define EFL_UI_IMAGE_LEGACY_CLASS efl_ui_image_legacy_class_get()

/**
 * @brief Retrieves the Efl.Ui.Image_Legacy class.
 *
 * This function returns a pointer to the Efl.Ui.Image_Legacy class description.
 * It is part of the beta API.
 *
 * @return A pointer to the Efl_Class structure for Efl.Ui.Image_Legacy.
 * @ingroup Efl_Ui_Image_Legacy
 */
EWAPI const Efl_Class *efl_ui_image_legacy_class_get(void) EINA_CONST;
#endif /* EFL_BETA_API_SUPPORT */

#endif
