/**
 * @file
 * @brief These routines are for the Elementary web module (no-op implementation).
 *
 * This is a no-op implementation of the Elm_Web interface, used when no
 * actual web engine is available or enabled. It provides the necessary
 * API symbols but performs no actual web browsing operations.
 */
#ifndef _ELM_WEB_NONE_EO_H_
#define _ELM_WEB_NONE_EO_H_

#ifndef _ELM_WEB_NONE_EO_CLASS_TYPE
#define _ELM_WEB_NONE_EO_CLASS_TYPE

/**
 * @brief Opaque type for the Elm_Web_None object.
 * @ingroup Elm_Web_None
 */
typedef Eo Elm_Web_None;

#endif

#ifndef _ELM_WEB_NONE_EO_TYPES
#define _ELM_WEB_NONE_EO_TYPES


#endif
/** Elementary web module class
 *
 * @ingroup Elm_Web_None
 */
#define ELM_WEB_NONE_CLASS elm_web_none_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Web_None class.
 *
 * @return The Efl_Class for Elm_Web_None.
 * @ingroup Elm_Web_None
 */
EMODAPI EMODAPI_WEAK const Efl_Class *elm_web_none_class_get(void) EINA_CONST;

#endif
