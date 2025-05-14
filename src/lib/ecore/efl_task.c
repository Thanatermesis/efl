#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_TASK_CLASS

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Sets the priority of the task.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in,out] pd The Efl_Task private data.
 * @param[in] priority The priority to set.
 */
EOLIAN static void
_efl_task_priority_set(Eo *obj EINA_UNUSED, Efl_Task_Data *pd, Efl_Task_Priority priority)
{
   pd->priority = priority;
}

/**
 * @brief Gets the priority of the task.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in] pd The Efl_Task private data.
 * @return The priority of the task.
 */
EOLIAN static Efl_Task_Priority
_efl_task_priority_get(const Eo *obj EINA_UNUSED, Efl_Task_Data *pd)
{
   return pd->priority;
}

/**
 * @brief Gets the exit code of the task.
 *
 * This is only valid after the task has finished running.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in] pd The Efl_Task private data.
 * @return The exit code of the task.
 */
EOLIAN static int
_efl_task_exit_code_get(const Eo *obj EINA_UNUSED, Efl_Task_Data *pd)
{
   return pd->exit_code;
}

/**
 * @brief Sets the flags for the task.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in,out] pd The Efl_Task private data.
 * @param[in] flags The flags to set.
 * For example, EFL_TASK_FLAGS_EXIT_WITH_PARENT.
 */
EOLIAN static void
_efl_task_flags_set(Eo *obj EINA_UNUSED, Efl_Task_Data *pd, Efl_Task_Flags flags)
{
   pd->flags = flags;
}

/**
 * @brief Gets the flags for the task.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in] pd The Efl_Task private data.
 * @return The flags of the task.
 */
EOLIAN static Efl_Task_Flags
_efl_task_flags_get(const Eo *obj EINA_UNUSED, Efl_Task_Data *pd)
{
   return pd->flags;
}

/**
 * @brief Destructor for the Efl_Task object.
 *
 * This function is called when the object is being destroyed.
 * It cleans up any resources allocated by the task, such as the command string.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in,out] pd The Efl_Task private data.
 */
EOLIAN static void
_efl_task_efl_object_destructor(Eo *obj EINA_UNUSED, Efl_Task_Data *pd)
{
   eina_stringshare_del(pd->command);
   pd->command = NULL;
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Constructor for the Efl_Task object.
 *
 * This function is called when a new Efl_Task object is created.
 * It initializes the object's private data, setting default flags.
 *
 * @param[in] obj The Efl_Task object being constructed.
 * @param[in,out] pd The Efl_Task private data.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_task_efl_object_constructor(Eo *obj, Efl_Task_Data *pd)
{
   obj = efl_constructor(efl_super(obj, EFL_TASK_CLASS));
   pd->flags = EFL_TASK_FLAGS_EXIT_WITH_PARENT; // Default flag
   return obj;
}

/**
 * @brief Sets the parent of the Efl_Task object.
 *
 * This function is called when the parent of the task object is set.
 * It propagates the parent_set call to the superclass.
 *
 * @param[in] obj The Efl_Task object.
 * @param[in] pd The Efl_Task private data.
 * @param[in] parent The new parent object.
 */
EOLIAN static void
_efl_task_efl_object_parent_set(Eo *obj, Efl_Task_Data *pd EINA_UNUSED, Efl_Object *parent)
{
   efl_parent_set(efl_super(obj, MY_CLASS), parent);
}

//////////////////////////////////////////////////////////////////////////

#include "efl_task.eo.c"
