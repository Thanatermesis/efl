/**
 * @file
 * @brief Ecore Connection Eet Client class - EO API
 *
 * This header defines the Ecore Connection Eet Client class,
 * which provides an Eet-based client connection.
 *
 * @ingroup Ecore_Con_Eet_Client_Obj
 */
#ifndef _ECORE_CON_EET_CLIENT_OBJ_EO_H_
#define _ECORE_CON_EET_CLIENT_OBJ_EO_H_

#ifndef _ECORE_CON_EET_CLIENT_OBJ_EO_CLASS_TYPE
#define _ECORE_CON_EET_CLIENT_OBJ_EO_CLASS_TYPE

/**
 * @brief Represents an Ecore Connection Eet Client object.
 * @ingroup Ecore_Con_Eet_Client_Obj
 */
typedef Eo Ecore_Con_Eet_Client_Obj;

#endif

#ifndef _ECORE_CON_EET_CLIENT_OBJ_EO_TYPES
#define _ECORE_CON_EET_CLIENT_OBJ_EO_TYPES

/**
 * @brief Represents any specific types related to Ecore_Con_Eet_Client_Obj.
 * Currently empty, but defined for future extension.
 * @ingroup Ecore_Con_Eet_Client_Obj
 */

#endif
/**
 * @brief Ecore Connection Eet Client class.
 *
 * This macro provides a convenient way to get the Efl_Class for
 * Ecore_Con_Eet_Client_Obj.
 *
 * @ingroup Ecore_Con_Eet_Client_Obj
 */
#define ECORE_CON_EET_CLIENT_OBJ_CLASS ecore_con_eet_client_obj_class_get()

/**
 * @brief Gets the Efl_Class for Ecore_Con_Eet_Client_Obj.
 *
 * This function returns the Efl_Class object associated with the
 * Ecore_Con_Eet_Client_Obj type. It is used internally by the EO system.
 *
 * @return The constant Efl_Class object for Ecore_Con_Eet_Client_Obj.
 * @ingroup Ecore_Con_Eet_Client_Obj
 */
ECORE_CON_API ECORE_CON_API_WEAK const Efl_Class *ecore_con_eet_client_obj_class_get(void) EINA_CONST;

#endif
