#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"
#include "eo_internal.h"

#ifndef _WIN32
# include <sys/resource.h>
#endif

#define MY_CLASS EFL_APP_CLASS

EFL_CLASS_SIMPLE_CLASS(efl_app, "Efl.App", EFL_APP_CLASS)

Efl_Version _app_efl_version = { 0, 0, 0, 0, NULL, NULL };

/**
 * @brief Gets the main application singleton.
 *
 * This function returns the singleton instance of the Efl_App. If one does
 * not exist, it creates and initializes it.
 *
 * @return The main application instance.
 */
EOLIAN static Efl_App*
_efl_app_app_main_get(void)
{
   if (_mainloop_singleton) return _mainloop_singleton;
   _mainloop_singleton = efl_add_ref(efl_app_realized_class_get(), NULL);
   _mainloop_singleton_data = efl_data_scope_get(_mainloop_singleton, EFL_LOOP_CLASS);
   return _mainloop_singleton;
}

/**
 * @brief Gets the EFL version used to build the application.
 *
 * @param obj The Efl_App object (unused).
 * @param pd Private data (unused).
 * @return A pointer to the Efl_Version struct representing the build-time EFL version.
 *         This version information is typically set during the build process.
 */
EOLIAN static const Efl_Version *
_efl_app_build_efl_version_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   return &_app_efl_version;
}

/**
 * @brief Gets the current EFL version.
 *
 * This function returns the runtime version of the EFL library.
 *
 * @param obj The Efl_App object (unused).
 * @param pd Private data (unused).
 * @return A pointer to a static Efl_Version struct representing the current EFL version.
 */
EOLIAN static const Efl_Version *
_efl_app_efl_version_get(const Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   /* vanilla EFL: flavor = NULL */
   static const Efl_Version version = {
      .major = VMAJ,
      .minor = VMIN,
      .micro = VMIC,
      .revision = VREV,
      .build_id = EFL_BUILD_ID,
      .flavor = NULL
   };
   return &version;
}

#ifdef _WIN32
#else
/**
 * @brief Maps Efl_Task_Priority enum values to system-specific priority values.
 *
 * This array is used on non-Windows systems to translate EFL task priorities
 * to values understandable by `setpriority()`. The index corresponds to the
 * Efl_Task_Priority enum.
 * For example:
 * - `primap[EFL_TASK_PRIORITY_NORMAL]` is 10.
 * - `primap[EFL_TASK_PRIORITY_BACKGROUND]` is 19.
 * - `primap[EFL_TASK_PRIORITY_LOW]` is 15.
 * - `primap[EFL_TASK_PRIORITY_HIGH]` is 5.
 * - `primap[EFL_TASK_PRIORITY_ULTRA]` is 0.
 * Lower numerical values generally indicate higher priority for `setpriority`.
 */
static const signed char primap[EFL_TASK_PRIORITY_ULTRA + 1] =
{
      10, // EFL_TASK_PRIORITY_NORMAL
      19, // EFL_TASK_PRIORITY_BACKGROUND
      15, // EFL_TASK_PRIORITY_LOW
      5, // EFL_TASK_PRIORITY_HIGH
      0  // EFL_TASK_PRIORITY_ULTRA
};
#endif

/**
 * @brief Sets the task priority for the application.
 *
 * @param obj The Efl_App object.
 * @param pd Private data (unused).
 * @param priority The desired task priority level (Efl_Task_Priority).
 *                 Example: EFL_TASK_PRIORITY_HIGH.
 */
EOLIAN static void
_efl_app_efl_task_priority_set(Eo *obj, void *pd EINA_UNUSED, Efl_Task_Priority priority)
{
   efl_task_priority_set(efl_super(obj, MY_CLASS), priority);
#ifdef _WIN32
#else
   // -20 (high) -> 19 (low)
   int p = 0;

   if ((priority >= EFL_TASK_PRIORITY_NORMAL) &&
       (priority <= EFL_TASK_PRIORITY_ULTRA))
     p = primap[priority];
   setpriority(PRIO_PROCESS, 0, p);
#endif
}

/**
 * @brief Gets the current task priority of the application.
 *
 * On non-Windows systems, this function retrieves the process priority using
 * `getpriority()` and then maps it to the closest Efl_Task_Priority value.
 *
 * @param obj The Efl_App object.
 * @param pd Private data (unused).
 * @return The current task priority level (Efl_Task_Priority).
 *         Example: EFL_TASK_PRIORITY_NORMAL.
 */
EOLIAN static Efl_Task_Priority
_efl_app_efl_task_priority_get(const Eo *obj, void *pd EINA_UNUSED)
{
   Efl_Task_Priority pri = EFL_TASK_PRIORITY_NORMAL;
#ifdef _WIN32
#else
   int p, i, dist = 0x7fffffff, d;

   errno = 0;
   p = getpriority(PRIO_PROCESS, 0);
   if (errno != 0)
     return efl_task_priority_get(efl_super(obj, MY_CLASS));

   // find the closest matching priority in primap
   for (i = EFL_TASK_PRIORITY_NORMAL; i <= EFL_TASK_PRIORITY_ULTRA; i++)
     {
        d = primap[i] - p;
        if (d < 0) d = -d;
        if (d < dist)
          {
             pri = i;
             dist = d;
          }
     }

   Efl_Task_Data *td = efl_data_scope_get(obj, EFL_TASK_CLASS);
   if (td) td->priority = pri;
#endif
   return pri;
}

//////////////////////////////////////////////////////////////////////////

#include "efl_app.eo.c"
