#ifndef _ELM_PHOTO_EO_H_
#define _ELM_PHOTO_EO_H_

#ifndef _ELM_PHOTO_EO_CLASS_TYPE
#define _ELM_PHOTO_EO_CLASS_TYPE

/**
 * @brief Represents an Elementary Photo object.
 * @ingroup Elm_Photo
 */
typedef Eo Elm_Photo;

#endif

#ifndef _ELM_PHOTO_EO_TYPES
#define _ELM_PHOTO_EO_TYPES


#endif
/** Elementary photo class
 *
 * @ingroup Elm_Photo
 */
#define ELM_PHOTO_CLASS elm_photo_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Photo class.
 *
 * @return The Efl_Class for Elm_Photo.
 * @ingroup Elm_Photo
 */
EWAPI const Efl_Class *elm_photo_class_get(void) EINA_CONST;

#endif
