/**
 * @file
 * @brief Ecore Connection Eet Server class - EO API
 *
 * This header defines the Ecore Connection Eet Server class and its related types.
 * It provides an abstraction for Eet-based server connections.
 * @ingroup Ecore_Con_Eet_Server_Obj
 */
#ifndef _ECORE_CON_EET_SERVER_OBJ_EO_H_
#define _ECORE_CON_EET_SERVER_OBJ_EO_H_

#ifndef _ECORE_CON_EET_SERVER_OBJ_EO_CLASS_TYPE
#define _ECORE_CON_EET_SERVER_OBJ_EO_CLASS_TYPE
/**
 * @brief Represents an Ecore Connection Eet Server object.
 * @ingroup Ecore_Con_Eet_Server_Obj
 */
typedef Eo Ecore_Con_Eet_Server_Obj;

#endif

#ifndef _ECORE_CON_EET_SERVER_OBJ_EO_TYPES
#define _ECORE_CON_EET_SERVER_OBJ_EO_TYPES


#endif
/** Ecore Connection Eet Server class.
 *
 * @ingroup Ecore_Con_Eet_Server_Obj
 */
#define ECORE_CON_EET_SERVER_OBJ_CLASS ecore_con_eet_server_obj_class_get()

/**
 * @brief Retrieves the Efl_Class for the Ecore_Con_Eet_Server_Obj.
 *
 * This function returns a pointer to the Efl_Class structure that
 * describes the Ecore_Con_Eet_Server_Obj class. This is used internally
 * by the EO system for object instantiation and type checking.
 *
 * @return A const pointer to the Efl_Class for Ecore_Con_Eet_Server_Obj.
 * @ingroup Ecore_Con_Eet_Server_Obj
 */
ECORE_CON_API ECORE_CON_API_WEAK const Efl_Class *ecore_con_eet_server_obj_class_get(void) EINA_CONST;

#endif
