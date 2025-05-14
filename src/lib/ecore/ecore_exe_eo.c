/**
 * @internal
 * @brief Event descriptor for data received from the child process's stdout.
 * @see ECORE_EXE_EVENT_DATA_GET
 */
EWAPI const Efl_Event_Description _ECORE_EXE_EVENT_DATA_GET =
   EFL_EVENT_DESCRIPTION("data,get");
/**
 * @internal
 * @brief Event descriptor for data received from the child process's stderr.
 * @see ECORE_EXE_EVENT_DATA_ERROR
 */
EWAPI const Efl_Event_Description _ECORE_EXE_EVENT_DATA_ERROR =
   EFL_EVENT_DESCRIPTION("data,error");

/**
 * @internal
 * @brief Implements ecore_obj_exe_command_set.
 * @see ecore_obj_exe_command_set
 */
void _ecore_exe_command_set(Eo *obj, Ecore_Exe_Data *pd, const char *exe_cmd, Ecore_Exe_Flags flags);

EOAPI EFL_VOID_FUNC_BODYV(ecore_obj_exe_command_set, EFL_FUNC_CALL(exe_cmd, flags), const char *exe_cmd, Ecore_Exe_Flags flags);

/**
 * @internal
 * @brief Implements ecore_obj_exe_command_get.
 * @see ecore_obj_exe_command_get
 */
void _ecore_exe_command_get(const Eo *obj, Ecore_Exe_Data *pd, const char **exe_cmd, Ecore_Exe_Flags *flags);

EOAPI EFL_VOID_FUNC_BODYV_CONST(ecore_obj_exe_command_get, EFL_FUNC_CALL(exe_cmd, flags), const char **exe_cmd, Ecore_Exe_Flags *flags);

/**
 * @internal
 * @brief Implements efl_object_destructor for Ecore_Exe.
 * Cleans up resources associated with the Ecore_Exe object.
 */
void _ecore_exe_efl_object_destructor(Eo *obj, Ecore_Exe_Data *pd);

/**
 * @internal
 * @brief Implements efl_object_finalize for Ecore_Exe.
 * Finalizes the Ecore_Exe object, potentially starting the process if auto-start is configured.
 */
Efl_Object *_ecore_exe_efl_object_finalize(Eo *obj, Ecore_Exe_Data *pd);

/**
 * @internal
 * @brief Implements efl_control_suspend_set for Ecore_Exe.
 * Pauses or resumes the Ecore_Exe (e.g., by sending SIGSTOP/SIGCONT to the child process).
 * @param suspend EINA_TRUE to suspend (pause), EINA_FALSE to resume.
 */
void _ecore_exe_efl_control_suspend_set(Eo *obj, Ecore_Exe_Data *pd, Eina_Bool suspend);

/**
 * @internal
 * @brief Class initializer for Ecore_Exe.
 * Sets up the Efl_Object operations for the Ecore_Exe class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_exe_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ECORE_EXE_EXTRA_OPS
#define ECORE_EXE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(ecore_obj_exe_command_set, _ecore_exe_command_set),
      EFL_OBJECT_OP_FUNC(ecore_obj_exe_command_get, _ecore_exe_command_get),
      EFL_OBJECT_OP_FUNC(efl_destructor, _ecore_exe_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_finalize, _ecore_exe_efl_object_finalize),
      EFL_OBJECT_OP_FUNC(efl_control_suspend_set, _ecore_exe_efl_control_suspend_set),
      ECORE_EXE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Efl_Class_Description for the Ecore_Exe class.
 * Defines metadata for the Ecore_Exe class, such as its name, version,
 * size of instance data, and initializer functions.
 */
static const Efl_Class_Description _ecore_exe_class_desc = {
   EO_VERSION, /**< EO_VERSION as defined in Eo.h */
   "Ecore.Exe", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR, /**< Specifies this as a regular, instantiable class. */
   sizeof(Ecore_Exe_Data), /**< The size of the instance-specific data structure. */
   _ecore_exe_class_initializer, /**< The function called to initialize the class. */
   NULL, /**< The function called to construct an object of this class (class constructor). */
   NULL /**< The function called to destruct an object of this class (class destructor). */
};

EFL_DEFINE_CLASS(ecore_exe_class_get, &_ecore_exe_class_desc, EFL_OBJECT_CLASS, EFL_CONTROL_INTERFACE, NULL);
