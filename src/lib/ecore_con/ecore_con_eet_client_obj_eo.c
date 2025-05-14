/**
 * @file
 * @brief Ecore Connection Eet Client class - Implementation
 *
 * This file implements the Ecore Connection Eet Client class.
 * It handles the lifecycle and class initialization for Eet client objects.
 */

Efl_Object *_ecore_con_eet_client_obj_efl_object_constructor(Eo *obj, Ecore_Con_Eet_Client_Obj_Data *pd);


void _ecore_con_eet_client_obj_efl_object_destructor(Eo *obj, Ecore_Con_Eet_Client_Obj_Data *pd);

/**
 * @internal
 * @brief Initializes the Ecore_Con_Eet_Client_Obj class.
 *
 * This function is called once when the class is first used.
 * It sets up the Efl_Object operations for the class, such as
 * constructor and destructor.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_con_eet_client_obj_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ECORE_CON_EET_CLIENT_OBJ_EXTRA_OPS
#define ECORE_CON_EET_CLIENT_OBJ_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _ecore_con_eet_client_obj_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _ecore_con_eet_client_obj_efl_object_destructor),
      ECORE_CON_EET_CLIENT_OBJ_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Ecore_Con_Eet_Client_Obj class.
 *
 * This structure provides metadata about the Ecore_Con_Eet_Client_Obj class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer functions.
 */
static const Efl_Class_Description _ecore_con_eet_client_obj_class_desc = {
   EO_VERSION,
   "Ecore.Con.Eet.Client.Obj",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Ecore_Con_Eet_Client_Obj_Data),
   _ecore_con_eet_client_obj_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(ecore_con_eet_client_obj_class_get, &_ecore_con_eet_client_obj_class_desc, ECORE_CON_EET_BASE_CLASS, NULL);
/**
 * @internal
 * @brief Implements the Efl_Object constructor for Ecore_Con_Eet_Client_Obj.
 *
 * This function is called when a new Ecore_Con_Eet_Client_Obj instance is created.
 * It should perform any necessary initialization for the object.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the object.
 * @return The constructed Efl_Object, or NULL on failure.
 */

/**
 * @internal
 * @brief Implements the Efl_Object destructor for Ecore_Con_Eet_Client_Obj.
 *
 * This function is called when an Ecore_Con_Eet_Client_Obj instance is being destroyed.
 * It should perform any necessary cleanup for the object.
 *
 * @param obj The Eo object to destruct.
 * @param pd The private data for the object.
 */
