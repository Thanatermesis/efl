#ifndef _ELM_SEPARATOR_EO_LEGACY_H_
#define _ELM_SEPARATOR_EO_LEGACY_H_

#ifndef _ELM_SEPARATOR_EO_CLASS_TYPE
#define _ELM_SEPARATOR_EO_CLASS_TYPE

/**
 * @brief Opaque handle to an Elementary Separator widget.
 * @ingroup Elm_Separator_Group
 */
typedef Eo Elm_Separator;

#endif

#ifndef _ELM_SEPARATOR_EO_TYPES
#define _ELM_SEPARATOR_EO_TYPES

/**
 * @brief Placeholder for future Elementary Separator specific types.
 * @ingroup Elm_Separator_Group
 */

#endif

/**
 * @brief Set the horizontal mode of a separator object
 *
 * @param[in] obj The object.
 * @param[in] horizontal If true, the separator is horizontal
 *
 * @ingroup Elm_Separator_Group
 */
EAPI void elm_separator_horizontal_set(Elm_Separator *obj, Eina_Bool horizontal);

/**
 * @brief Get the horizontal mode of a separator object
 *
 * @param[in] obj The object.
 *
 * @return If true, the separator is horizontal
 *
 * @ingroup Elm_Separator_Group
 */
EAPI Eina_Bool elm_separator_horizontal_get(const Elm_Separator *obj);

#endif
