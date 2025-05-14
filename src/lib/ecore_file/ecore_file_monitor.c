#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_file_private.h"

/**
 * @brief Initializes the Ecore_File_Monitor system.
 *
 * This function sets up the necessary resources for file monitoring.
 * It should be called before any other ecore_file_monitor functions.
 *
 * @return 1 on success, 0 on failure.
 */
int
ecore_file_monitor_init(void)
{
   if (ecore_file_monitor_backend_init())
     return 1;
   return 0;
}

/**
 * @brief Shuts down the Ecore_File_Monitor system.
 *
 * This function releases all resources used by the file monitoring system.
 * It should be called when file monitoring is no longer needed.
 */
void
ecore_file_monitor_shutdown(void)
{
   ecore_file_monitor_backend_shutdown();
}

/**
 * @brief Adds a new file or directory to monitor for changes.
 *
 * This function creates a monitor for the specified @p path. When changes
 * occur to the path, the @p func callback is invoked with the @p data
 * pointer.
 *
 * @param path The path to the file or directory to monitor. Must not be NULL or empty.
 * @param func The callback function to execute when an event occurs. Must not be NULL.
 * @param data User data to be passed to the callback function.
 * @return A new Ecore_File_Monitor handle on success, or NULL on failure.
 *         The returned monitor should be freed using ecore_file_monitor_del()
 *         when no longer needed.
 *
 * @see ecore_file_monitor_del()
 */
EAPI Ecore_File_Monitor *
ecore_file_monitor_add(const char           *path,
                       Ecore_File_Monitor_Cb func,
                       void                 *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(path[0] == '\0', NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   return ecore_file_monitor_backend_add(path, func, data);
}

/**
 * @brief Deletes a file monitor.
 *
 * This function stops monitoring the path associated with the given
 * Ecore_File_Monitor handle and frees the resources used by it.
 *
 * @param em The Ecore_File_Monitor handle to delete. If NULL, the function does nothing.
 *
 * @see ecore_file_monitor_add()
 */
EAPI void
ecore_file_monitor_del(Ecore_File_Monitor *em)
{
   if (!em) return;
   EINA_SAFETY_ON_NULL_RETURN(em);
   ecore_file_monitor_backend_del(em);
}

/**
 * @brief Gets the path being monitored by an Ecore_File_Monitor.
 *
 * This function returns the path that was set when the monitor @p em was created.
 *
 * @param em The Ecore_File_Monitor handle. Must not be NULL.
 * @return The path being monitored, or NULL if @p em is NULL. The returned string
 *         is valid as long as @p em is valid and should not be modified or freed.
 */
EAPI const char *
ecore_file_monitor_path_get(Ecore_File_Monitor *em)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(em, NULL);
   return em->path;
}
