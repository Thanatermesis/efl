/* EIO - EFL data type library
 * Copyright (C) 2011 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "eio_private.h"
#include "Eio.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Hash table storing all active Eio_Monitor instances.
 *
 * The keys are the stringshared paths being monitored, and the values
 * are pointers to Eio_Monitor structures.
 */
static Eina_Hash *_eio_monitors = NULL;

/**
 * @brief Frees an Eio_Monitor instance and its associated resources.
 *
 * This function is responsible for cleaning up an Eio_Monitor. It removes
 * the monitor from the global hash table (if not already marked for deletion),
 * cancels any pending file existence checks, and releases backend resources.
 * It also unreferences the path stringshare.
 *
 * @param monitor The Eio_Monitor instance to free.
 */
static void
_eio_monitor_free(Eio_Monitor *monitor)
{
   if (!monitor->delete_me)
     eina_hash_del(_eio_monitors, monitor->path, monitor);

   if (monitor->exist)
     {
        eio_file_cancel(monitor->exist);
        monitor->exist = NULL;
     }

   if (monitor->backend)
     {
        if (!monitor->fallback)
          eio_monitor_backend_del(monitor);
        else
          eio_monitor_fallback_del(monitor);
     }

   if (_eio_log_dom_global != -1)
     INF("Stopping monitor on '%s'.", monitor->path);

   eina_stringshare_del(monitor->path);
   free(monitor);
}

/**
 * @brief Callback function to clean up an Eio_Monitor_Error event data.
 *
 * This function is called by Ecore when an EIO_MONITOR_ERROR event is
 * processed and its data needs to be freed. It unreferences the associated
 * Eio_Monitor and frees the event structure.
 *
 * @param user_data Unused user data.
 * @param func_data Pointer to the Eio_Monitor_Error event data to be freed.
 */
static void
_eio_monitor_error_cleanup_cb(EINA_UNUSED void *user_data, void *func_data)
{
   Eio_Monitor_Error *ev = func_data;

   EINA_REFCOUNT_UNREF(ev->monitor)
     _eio_monitor_free(ev->monitor);
   free(ev);
}

/**
 * @brief Callback function to clean up an Eio_Monitor_Event data.
 *
 * This function is called by Ecore when various EIO_MONITOR_* events
 * (e.g., EIO_MONITOR_FILE_CREATED) are processed and their data needs to be
 * freed. It unreferences the associated Eio_Monitor, unreferences the
 * event's filename stringshare, and frees the event structure.
 *
 * @param user_data Unused user data.
 * @param func_data Pointer to the Eio_Monitor_Event data to be freed.
 */
static void
_eio_monitor_event_cleanup_cb(EINA_UNUSED void *user_data, void *func_data)
{
   Eio_Monitor_Event *ev = func_data;

   EINA_REFCOUNT_UNREF(ev->monitor)
     _eio_monitor_free(ev->monitor);
   eina_stringshare_del(ev->filename);
   free(ev);
}

/**
 * @brief Callback for successful stat operation on a monitored path.
 *
 * This function is called when eio_file_direct_stat successfully retrieves
 * information about the monitored path. It indicates the path exists.
 * If the monitor is still active (refcount > 1), it proceeds to add
 * the backend monitor. Finally, it unreferences the monitor, potentially
 * freeing it if this was the last reference (e.g., initial stat check).
 *
 * @param data Pointer to the Eio_Monitor instance.
 * @param handler The Eio_File handler for the stat operation (unused).
 * @param st The stat information (unused in this function, but signifies success).
 */
static void
_eio_monitor_stat_cb(void *data, EINA_UNUSED Eio_File *handler, EINA_UNUSED const Eina_Stat *st)
{
   Eio_Monitor *monitor = data;

   monitor->exist = NULL;

   if (EINA_REFCOUNT_GET(monitor) > 1)
     eio_monitor_backend_add(monitor);

   EINA_REFCOUNT_UNREF(monitor)
     _eio_monitor_free(monitor);
}

/**
 * @brief Creates and sends an EIO_MONITOR_ERROR event.
 *
 * This function is called when an error occurs related to a monitor,
 * such as failure to stat the monitored path. It allocates an
 * Eio_Monitor_Error structure, populates it, and adds it to the
 * Ecore event queue.
 *
 * @param monitor The Eio_Monitor instance associated with the error.
 * @param error The error code (typically an errno value).
 */
static void
_eio_monitor_error(Eio_Monitor *monitor, int error)
{
   Eio_Monitor_Error *ev;

   ev = calloc(1, sizeof (Eio_Monitor_Error));
   if (!ev) return;

   ev->monitor = monitor;
   EINA_REFCOUNT_REF(ev->monitor);
   ev->error = error;

   ecore_event_add(EIO_MONITOR_ERROR, ev, _eio_monitor_error_cleanup_cb, NULL);
}

/**
 * @brief Callback for an error during stat operation on a monitored path.
 *
 * This function is called when eio_file_direct_stat encounters an error
 * while trying to get information about the monitored path. It sets the
 * error code on the monitor and, if the monitor is still active,
 * triggers an EIO_MONITOR_ERROR event. Finally, it unreferences the
 * monitor, potentially freeing it.
 *
 * @param data Pointer to the Eio_Monitor instance.
 * @param handler The Eio_File handler for the stat operation (unused).
 * @param error The error code from the stat operation.
 */
static void
_eio_monitor_error_cb(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   Eio_Monitor *monitor = data;

   monitor->error = error;
   monitor->exist = NULL;

   if (EINA_REFCOUNT_GET(monitor) >= 1)
     _eio_monitor_error(monitor, error);

   EINA_REFCOUNT_UNREF(monitor)
     _eio_monitor_free(monitor);

   return;
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Initializes the Eio_Monitor subsystem.
 *
 * This function must be called before any other eio_monitor_* functions.
 * It registers new Ecore event types for all monitor-related events,
 * initializes the monitor backend and fallback mechanisms, and creates
 * the global hash table for storing active monitors.
 * It will abort if the hash table cannot be created.
 */
void
eio_monitor_init(void)
{
   EIO_MONITOR_ERROR = ecore_event_type_new();
   EIO_MONITOR_SELF_RENAME = ecore_event_type_new();
   EIO_MONITOR_SELF_DELETED = ecore_event_type_new();
   EIO_MONITOR_FILE_CREATED = ecore_event_type_new();
   EIO_MONITOR_FILE_DELETED = ecore_event_type_new();
   EIO_MONITOR_FILE_MODIFIED = ecore_event_type_new();
   EIO_MONITOR_FILE_CLOSED = ecore_event_type_new();
   EIO_MONITOR_DIRECTORY_CREATED = ecore_event_type_new();
   EIO_MONITOR_DIRECTORY_DELETED = ecore_event_type_new();
   EIO_MONITOR_DIRECTORY_MODIFIED = ecore_event_type_new();
   EIO_MONITOR_DIRECTORY_CLOSED = ecore_event_type_new();

   eio_monitor_backend_init();
   eio_monitor_fallback_init();

   _eio_monitors = eina_hash_stringshared_new(NULL);
   /* FIXME: this check is optional, but if it is kept then failure should be handled more gracefully */
   if (!_eio_monitors) abort();
}

/**
 * @brief Shuts down the Eio_Monitor subsystem.
 *
 * This function cleans up resources used by the Eio_Monitor subsystem.
 * It flushes all pending monitor-related Ecore events, iterates through
 * active monitors to mark them for deletion and cancel pending operations,
 * frees the global monitor hash table, and shuts down the monitor backend
 * and fallback mechanisms.
 */
void
eio_monitor_shutdown(void)
{
   Eina_Iterator *it;
   Eio_Monitor *monitor;

   ecore_event_type_flush(EIO_MONITOR_ERROR,
                          EIO_MONITOR_SELF_RENAME,
                          EIO_MONITOR_SELF_DELETED,
                          EIO_MONITOR_FILE_CREATED,
                          EIO_MONITOR_FILE_DELETED,
                          EIO_MONITOR_FILE_MODIFIED,
                          EIO_MONITOR_FILE_CLOSED,
                          EIO_MONITOR_DIRECTORY_CREATED,
                          EIO_MONITOR_DIRECTORY_DELETED,
                          EIO_MONITOR_DIRECTORY_MODIFIED,
                          EIO_MONITOR_DIRECTORY_CLOSED);

   it = eina_hash_iterator_data_new(_eio_monitors);
   EINA_ITERATOR_FOREACH(it, monitor)
     {
        if (monitor->exist)
          {
             eio_file_cancel(monitor->exist);
             monitor->exist = NULL;
          }
        monitor->delete_me = EINA_TRUE;
     }
   eina_iterator_free(it);
   eina_hash_free(_eio_monitors);
   _eio_monitors = NULL;

   eio_monitor_backend_shutdown();
   eio_monitor_fallback_shutdown();
}

/**
 * @brief Returns a string representation of an EIO monitor event code.
 *
 * This is a helper function primarily for debugging and logging purposes.
 *
 * @param event_code The integer code of the EIO monitor event.
 * @return A static string naming the event, or "Unknown" if the code is not recognized.
 * @note The returned string should not be modified or freed.
 */
static const char *
_eio_naming_event(int event_code)
{
#define EVENT_CHECK(Code, Ev) if (Code == Ev) return #Ev;

   EVENT_CHECK(event_code, EIO_MONITOR_ERROR);
   EVENT_CHECK(event_code, EIO_MONITOR_FILE_CREATED);
   EVENT_CHECK(event_code, EIO_MONITOR_FILE_DELETED);
   EVENT_CHECK(event_code, EIO_MONITOR_FILE_MODIFIED);
   EVENT_CHECK(event_code, EIO_MONITOR_FILE_CLOSED);
   EVENT_CHECK(event_code, EIO_MONITOR_DIRECTORY_CREATED);
   EVENT_CHECK(event_code, EIO_MONITOR_DIRECTORY_DELETED);
   EVENT_CHECK(event_code, EIO_MONITOR_DIRECTORY_MODIFIED);
   EVENT_CHECK(event_code, EIO_MONITOR_DIRECTORY_CLOSED);
   EVENT_CHECK(event_code, EIO_MONITOR_SELF_RENAME);
   EVENT_CHECK(event_code, EIO_MONITOR_SELF_DELETED);
   return "Unknown";
}

/**
 * @brief Creates and sends a generic Eio_Monitor_Event.
 *
 * This function is used to dispatch various file/directory monitoring events
 * (e.g., created, deleted, modified). It allocates an Eio_Monitor_Event
 * structure, populates it with the monitor, filename, and event type,
 * and adds it to the Ecore event queue.
 *
 * @param monitor The Eio_Monitor instance that detected the event.
 * @param filename The name of the file or directory related to the event.
 * @param event_code The specific Ecore event type (e.g., EIO_MONITOR_FILE_CREATED).
 */
void
_eio_monitor_send(Eio_Monitor *monitor, const char *filename, int event_code)
{
   Eio_Monitor_Event *ev;

   if (monitor->delete_me)
     return;

   INF("Event '%s' for monitored path '%s'.",
       _eio_naming_event(event_code), filename);

   ev = calloc(1, sizeof (Eio_Monitor_Event));
   if (!ev) return;

   ev->monitor = monitor;
   EINA_REFCOUNT_REF(ev->monitor);
   ev->filename = eina_stringshare_add(filename);

   ecore_event_add(event_code, ev, _eio_monitor_event_cleanup_cb, NULL);
}

/**
 * @brief Handles the renaming of a monitored path.
 *
 * When a monitored path is reported as renamed (e.g., by the backend),
 * this function updates the Eio_Monitor's internal state. It cancels
 * any existing operations, updates the path in the global hash table,
 * and re-initiates the stat check on the new path to re-establish
 * monitoring. An EIO_MONITOR_SELF_RENAME event is sent to notify
 * the application.
 *
 * @param monitor The Eio_Monitor instance whose path is being renamed.
 * @param newpath The new path for the monitored item.
 */
void
_eio_monitor_rename(Eio_Monitor *monitor, const char *newpath)
{
  const char *tmp;

  if (monitor->delete_me)
    return;

  /* destroy old state */
  if (monitor->exist)
    {
       eio_file_cancel(monitor->exist);
       monitor->exist = NULL;
    }

  if (monitor->backend)
    {
       if (!monitor->fallback)
         eio_monitor_backend_del(monitor);
       else
         eio_monitor_fallback_del(monitor);
    }

  INF("Renaming path '%s' to '%s'.",
      monitor->path, newpath);

  /* rename */
  tmp = monitor->path;
  monitor->path = eina_stringshare_add(newpath);
  eina_hash_move(_eio_monitors, tmp, monitor->path);
  eina_stringshare_del(tmp);

  /* That means death (cmp pointer and not content) */
  /* this - i think, is wrong. if the paths are the same, we should just
   * re-stat anyway. imagine the file was renamed and then replaced?
   * disable this as this was part of a possible crash due to eio.
  if (tmp == monitor->path)
    {
      _eio_monitor_error(monitor, -1);
      return;
    }
   */

  EINA_REFCOUNT_REF(monitor); /* as we spawn a thread for this monitor, we need to refcount specifically for it */

  /* restart */
  monitor->rename = EINA_TRUE;
  monitor->exist = eio_file_direct_stat(monitor->path,
                                        _eio_monitor_stat_cb,
                                        _eio_monitor_error_cb,
                                        monitor);

  /* FIXME: probably should handle this more gracefully */
  if (!monitor->exist) abort();
  /* and notify the app */
  _eio_monitor_send(monitor, newpath, EIO_MONITOR_SELF_RENAME);
}

/**
 * @endcond
 */


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @addtogroup Eio_Monitor_Events
 * @{
 *
 * @brief Ecore event types for file system monitoring.
 *
 * These variables hold the unique integer identifiers for Ecore events
 * generated by the Eio_Monitor system. Applications can register handlers
 * for these event types using `ecore_event_handler_add()`.
 */
EIO_API int EIO_MONITOR_ERROR;
EIO_API int EIO_MONITOR_FILE_CREATED;
EIO_API int EIO_MONITOR_FILE_DELETED;
EIO_API int EIO_MONITOR_FILE_MODIFIED;
EIO_API int EIO_MONITOR_FILE_CLOSED;
EIO_API int EIO_MONITOR_DIRECTORY_CREATED;
EIO_API int EIO_MONITOR_DIRECTORY_DELETED;
EIO_API int EIO_MONITOR_DIRECTORY_MODIFIED;
EIO_API int EIO_MONITOR_DIRECTORY_CLOSED;
EIO_API int EIO_MONITOR_SELF_RENAME;
EIO_API int EIO_MONITOR_SELF_DELETED;
/** @} */

/**
 * @brief Adds a new monitor for the specified file or directory path.
 *
 * This function creates and starts a monitor for the given path.
 * If a monitor for this path already exists and its mtime hasn't changed,
 * its reference count is incremented and the existing monitor is returned.
 * If mtime has changed, the old monitor is marked for deletion and a new one is created.
 *
 * The function first checks if the path exists. If not, it returns NULL.
 * It then attempts to use the platform's native backend for monitoring.
 * If the native backend fails or if the EIO_MONITOR_POLL environment
 * variable is set, it falls back to a polling mechanism.
 *
 * @param path The absolute or relative path to the file or directory to monitor.
 * @return A pointer to an Eio_Monitor instance on success, or NULL on failure
 *         (e.g., path does not exist, memory allocation failure, or backend
 *         initialization failure). The returned monitor is refcounted.
 * @see eio_monitor_del()
 * @see eio_monitor_stringshared_add()
 */
EIO_API Eio_Monitor *
eio_monitor_add(const char *path)
{
   const char *tmp;
   Eio_Monitor *ret;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   tmp = eina_stringshare_add(path);
   ret = eio_monitor_stringshared_add(tmp);
   eina_stringshare_del(tmp);
   return ret;
}

/**
 * @brief Adds a new monitor for a stringshared file or directory path.
 *
 * This function is similar to eio_monitor_add(), but it takes a
 * pre-existing stringshared path. This can be more efficient if the path
 * string is already managed by Eina's stringshare mechanism.
 * The provided `path` stringshare's reference count is not incremented by this
 * function directly for its own storage; it expects the caller to manage the
 * lifetime of the passed stringshare if it's needed beyond this call, or
 * it creates its own ref if a new monitor is truly created.
 *
 * @param path The stringshared path to monitor. Must not be NULL.
 * @return A pointer to an Eio_Monitor instance on success, or NULL on failure.
 *         The returned monitor is refcounted.
 * @see eio_monitor_del()
 * @see eio_monitor_add()
 */
EIO_API Eio_Monitor *
eio_monitor_stringshared_add(const char *path)
{
   Eio_Monitor *monitor;
   struct stat st;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(_eio_monitors, NULL);

   if (stat(path, &st) != 0)
     {
        ERR("monitored path '%s' not found.", path);
        return NULL;
     }

   monitor = eina_hash_find(_eio_monitors, path);

   if (monitor)
     {
        if (st.st_mtime != monitor->mtime)
          {
             monitor->delete_me = EINA_TRUE;
             eina_hash_del(_eio_monitors, monitor->path, monitor);
          }
        else
          {
             EINA_REFCOUNT_REF(monitor);
             return monitor;
          }
     }

   monitor = malloc(sizeof (Eio_Monitor));
   if (!monitor) return NULL;

   monitor->mtime = st.st_mtime;
   monitor->backend = NULL; // This is needed to avoid race condition
   monitor->path = eina_stringshare_ref(path);
   monitor->fallback = EINA_FALSE;
   monitor->rename = EINA_FALSE;
   monitor->delete_me = EINA_FALSE;
   monitor->exist = NULL;

   EINA_REFCOUNT_INIT(monitor);

   static signed char monpoll = -1;

   if (monpoll == -1)
     {
        if (getenv("EIO_MONITOR_POLL")) monpoll = 1;
        else monpoll = 0;
     }
   if (monpoll)
     eio_monitor_fallback_add(monitor);
   else
     eio_monitor_backend_add(monitor);

   if (!monitor->backend)
     {
        WRN("Impossible to create a monitor for '%s'.", monitor->path);
        eina_stringshare_del(monitor->path);
        free(monitor);
        return NULL;
     }

   eina_hash_direct_add(_eio_monitors, path, monitor);
   INF("New monitor on '%s'.", path);

   return monitor;
}

/**
 * @brief Deletes an Eio_Monitor instance.
 *
 * This function decrements the reference count of the given Eio_Monitor.
 * If the reference count reaches zero, the monitor and its associated
 * resources are freed.
 *
 * @param monitor The Eio_Monitor instance to delete. If NULL, the function
 *                does nothing.
 */
EIO_API void
eio_monitor_del(Eio_Monitor *monitor)
{
   if (!monitor) return;
   EINA_REFCOUNT_UNREF(monitor)
     _eio_monitor_free(monitor);
}

/**
 * @brief Retrieves the path associated with an Eio_Monitor.
 *
 * @param monitor The Eio_Monitor instance. Must not be NULL.
 * @return A pointer to the stringshared path being monitored.
 *         The returned string is owned by the monitor and should not be
 *         modified or freed by the caller. It remains valid as long as
 *         the monitor itself is valid. Returns NULL if monitor is NULL.
 */
EIO_API const char *
eio_monitor_path_get(Eio_Monitor *monitor)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(monitor, NULL);
   return monitor->path;
}

/**
 * @brief Checks if a given path is within the monitoring context of an Eio_Monitor.
 *
 * This function is used to determine if an event reported for a specific `path`
 * (e.g., a file within a monitored directory) should be considered relevant
 * to this `monitor`. The check is delegated to either the fallback mechanism
 * or the native backend, depending on which one is active for the monitor.
 *
 * This is particularly useful for directory monitoring, where events might
 * occur for files inside the directory. This function helps filter events
 * that are not directly related to the monitored item itself but are within
 * its scope (e.g. a file created inside a monitored directory).
 *
 * @param monitor The Eio_Monitor instance.
 * @param path The path to check against the monitor's context.
 * @return EINA_TRUE if the path is within the monitor's context,
 *         EINA_FALSE otherwise.
 */
EIO_API Eina_Bool
eio_monitor_has_context(const Eio_Monitor *monitor, const char *path)
{
   if (monitor->fallback)
     {
        return eio_monitor_fallback_context_check(monitor, path);
     }
   else
     {
        return eio_monitor_context_check(monitor, path);
     }
}
