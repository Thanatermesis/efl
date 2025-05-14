#ifndef _ELM_ATSPI_APP_OBJECT_EO_H_
#define _ELM_ATSPI_APP_OBJECT_EO_H_

/**
 * @file
 * @brief EFL Atspi App Object class
 */

#ifndef _ELM_ATSPI_APP_OBJECT_EO_CLASS_TYPE
#define _ELM_ATSPI_APP_OBJECT_EO_CLASS_TYPE

/**
 * @brief Opaque handle to an AT-SPI application object.
 * @ingroup Elm_Atspi_App_Object
 */
typedef Eo Elm_Atspi_App_Object;

#endif

#ifndef _ELM_ATSPI_APP_OBJECT_EO_TYPES
#define _ELM_ATSPI_APP_OBJECT_EO_TYPES

/**
 * @brief Represents the types specific to Elm_Atspi_App_Object.
 * @ingroup Elm_Atspi_App_Object
 */

#endif
/**
 * @brief AT-SPI application object class.
 *
 * This macro provides a convenient way to get the Efl_Class for
 * Elm_Atspi_App_Object.
 *
 * @ingroup Elm_Atspi_App_Object
 */
#define ELM_ATSPI_APP_OBJECT_CLASS elm_atspi_app_object_class_get()

/**
 * @brief Get the Efl_Class for the Elm_Atspi_App_Object.
 *
 * @return The Efl_Class for Elm_Atspi_App_Object.
 * @ingroup Elm_Atspi_App_Object
 */
EWAPI const Efl_Class *elm_atspi_app_object_class_get(void) EINA_CONST;

#endif
