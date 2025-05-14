/**
 * @file
 * @brief Ecore Connection Eet Server class - Implementation
 *
 * This file implements the Ecore Connection Eet Server class.
 * It handles the lifecycle and class initialization for Eet server objects.
 * @ingroup Ecore_Con_Eet_Server_Obj
 */

Efl_Object *_ecore_con_eet_server_obj_efl_object_constructor(Eo *obj, Ecore_Con_Eet_Server_Obj_Data *pd);


void _ecore_con_eet_server_obj_efl_object_destructor(Eo *obj, Ecore_Con_Eet_Server_Obj_Data *pd);

/**
 * @internal
 * @brief Initializes the Ecore_Con_Eet_Server_Obj class.
 *
 * This function is called once when the class is first used.
 * It sets up the Efl_Object operations for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_con_eet_server_obj_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ECORE_CON_EET_SERVER_OBJ_EXTRA_OPS
#define ECORE_CON_EET_SERVER_OBJ_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _ecore_con_eet_server_obj_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _ecore_con_eet_server_obj_efl_object_destructor),
      ECORE_CON_EET_SERVER_OBJ_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Ecore_Con_Eet_Server_Obj class.
 *
 * This structure contains metadata about the Ecore_Con_Eet_Server_Obj class,
 * such as its version, name, type, size of instance data, and initializer functions.
 */
static const Efl_Class_Description _ecore_con_eet_server_obj_class_desc = {
   EO_VERSION, /**< The EO API version for this class. */
   "Ecore.Con.Eet.Server.Obj", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Ecore_Con_Eet_Server_Obj_Data), /**< The size of the instance data. */
   _ecore_con_eet_server_obj_class_initializer, /**< The class initializer function. */
   NULL, /**< The class constructor function (optional). */
   NULL /**< The class destructor function (optional). */
};

EFL_DEFINE_CLASS(ecore_con_eet_server_obj_class_get, &_ecore_con_eet_server_obj_class_desc, ECORE_CON_EET_BASE_CLASS, NULL);
