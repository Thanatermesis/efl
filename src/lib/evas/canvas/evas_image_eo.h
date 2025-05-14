/**
 * @file
 * @brief These routines are legacy routines used for Evas.
 */
#ifndef _EVAS_IMAGE_EO_H_
#define _EVAS_IMAGE_EO_H_

#ifndef _EVAS_IMAGE_EO_CLASS_TYPE
#define _EVAS_IMAGE_EO_CLASS_TYPE

/**
 * @typedef Evas_Image
 * @brief Represents an Evas image object. This is an alias for the Eo type.
 * @ingroup Evas_Image
 */
typedef Eo Evas_Image;

#endif

#ifndef _EVAS_IMAGE_EO_TYPES
#define _EVAS_IMAGE_EO_TYPES


#endif
/**
 * @def EVAS_IMAGE_CLASS
 * @brief Macro to get the Evas Image class.
 *
 * This macro provides a convenient way to access the Efl_Class object
 * for Evas_Image.
 *
 * @return The Efl_Class object for Evas_Image.
 * @ingroup Evas_Image
 */
#define EVAS_IMAGE_CLASS evas_image_class_get()

/**
 * @brief Retrieves the Efl_Class for the Evas_Image.
 *
 * This function returns the Efl_Class object associated with the Evas_Image type.
 * It is used internally for class management and type checking.
 *
 * @return A pointer to the constant Efl_Class object for Evas_Image.
 *         Returns NULL if the class cannot be retrieved.
 * @ingroup Evas_Image
 */
EVAS_API EVAS_API_WEAK const Efl_Class *evas_image_class_get(void) EINA_CONST;

#endif
