/**
 * @file
 * @brief Efl anvas pan specific for Elm_Genlist.
 *
 * This is the internal pan object used by @ref Elm_Genlist.
 * It handles the scrolling and layout of genlist items.
 * Users should not interact with this object directly, but rather
 * through the Elm_Genlist API.
 */
#ifndef _ELM_GENLIST_PAN_EO_H_
#define _ELM_GENLIST_PAN_EO_H_

#ifndef _ELM_GENLIST_PAN_EO_CLASS_TYPE
#define _ELM_GENLIST_PAN_EO_CLASS_TYPE

/**
 * @brief Represents an instance of the Elm_Genlist_Pan class.
 * @ingroup Elm_Genlist_Pan
 */
typedef Eo Elm_Genlist_Pan;

#endif

#ifndef _ELM_GENLIST_PAN_EO_TYPES
#define _ELM_GENLIST_PAN_EO_TYPES

/**
 * @brief Represents additional types specific to Elm_Genlist_Pan.
 * Currently empty, but defined for future extensibility.
 */

#endif
/**
 * @brief Macro to get the Efl_Class for Elm_Genlist_Pan.
 *
 * This macro provides a convenient way to access the class description
 * for Elm_Genlist_Pan.
 *
 * @ingroup Elm_Genlist_Pan
 */
#define ELM_GENLIST_PAN_CLASS elm_genlist_pan_class_get()

/**
 * @brief Retrieves the Efl_Class definition for the Elm_Genlist_Pan class.
 *
 * This function returns a pointer to the constant Efl_Class structure
 * that describes the Elm_Genlist_Pan class. This is used by the Efl
 * object system for type checking, inheritance, and other class-related
 * operations.
 *
 * @return A const pointer to the Efl_Class for Elm_Genlist_Pan.
 * @see ELM_GENLIST_PAN_CLASS
 * @ingroup Elm_Genlist_Pan
 */
EWAPI const Efl_Class *elm_genlist_pan_class_get(void) EINA_CONST;

#endif
