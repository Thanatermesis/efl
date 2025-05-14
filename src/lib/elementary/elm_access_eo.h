/**
 * @file
 * @brief These routines are bindings to an Efl_Access_Object.
 */
#ifndef _ELM_ACCESS_EO_H_
#define _ELM_ACCESS_EO_H_

#ifndef _ELM_ACCESS_EO_CLASS_TYPE
#define _ELM_ACCESS_EO_CLASS_TYPE

/**
 * @brief Opaque handle to an Elm_Access object.
 * @ingroup Elm_Access
 */
typedef Eo Elm_Access;

#endif

#ifndef _ELM_ACCESS_EO_TYPES
#define _ELM_ACCESS_EO_TYPES


#endif
/**
 * @brief Elm abstract accessibility class.
 *
 * This class provides an abstract interface for accessibility features.
 *
 * @ingroup Elm_Access
 */
#define ELM_ACCESS_CLASS elm_access_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Access class.
 *
 * @return The Efl_Class for Elm_Access.
 * @ingroup Elm_Access
 */
EWAPI const Efl_Class *elm_access_class_get(void) EINA_CONST;

#endif
