/* EIO - EFL data type library
 * Copyright (C) 2015 Enlightenment Developers:
 *           Pierre Lamot <pierre.lamot@openwide.fr>
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

#import <CoreServices/CoreServices.h>

static CFTimeInterval _latency  = 0.1;

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Backend data structure for an Eio_Monitor instance.
 * This structure holds the necessary information for the FSEvents backend
 * to monitor a specific path.
 */
struct _Eio_Monitor_Backend
{
   Eio_Monitor *parent; /**< Pointer to the parent Eio_Monitor object. */
   ///the monitored path
   char *mon_path; /**< The path that is being monitored, potentially a parent directory. */
   ///the actual file path
   char *real_path; /**< The canonicalized, absolute path of the item being monitored. */
};

/**
 * @brief Structure to hold information about a single FSEvent.
 * This is used to pass event details from the FSEvents callback
 * to the main loop for processing.
 */
typedef struct _FSEvent_Info FSEvent_Info;

struct _FSEvent_Info {
   char *path; /**< The path associated with the event. */
   FSEventStreamEventFlags flags; /**< The FSEvent flags for this event. */

};

/**
 * @brief Structure to map FSEvent flags to EIO_MONITOR event codes.
 */
typedef struct _Eio_FSEvent_Table Eio_FSEvent_Table;

struct _Eio_FSEvent_Table
{
   int mask; /**< The FSEventStreamEventFlag to match. */
   int *ev_file_code;
   int *ev_dir_code;
};

#define EIO_FSEVENT_LINE(FSe, Ef, Ed)		\
  { kFSEventStreamEventFlag##FSe, &EIO_MONITOR_##Ef, &EIO_MONITOR_##Ed }

static const Eio_FSEvent_Table match[] = {
  EIO_FSEVENT_LINE(ItemChangeOwner, FILE_MODIFIED, DIRECTORY_MODIFIED), // Owner changed
  EIO_FSEVENT_LINE(ItemInodeMetaMod, FILE_MODIFIED, DIRECTORY_MODIFIED), // Inode metadata changed
  EIO_FSEVENT_LINE(ItemXattrMod, FILE_MODIFIED, DIRECTORY_MODIFIED), // Extended attributes modified
  EIO_FSEVENT_LINE(ItemModified, FILE_MODIFIED, DIRECTORY_MODIFIED), // Content modified
  EIO_FSEVENT_LINE(ItemRemoved, FILE_DELETED, DIRECTORY_DELETED), // Item removed
  EIO_FSEVENT_LINE(ItemCreated, FILE_CREATED, DIRECTORY_CREATED), // Item created
  EIO_FSEVENT_LINE(RootChanged, SELF_DELETED, SELF_DELETED) // Monitored root path changed or unmounted
};

/**
 * @brief Global FSEventStream reference.
 * This stream monitors all paths registered by Eio_Monitor instances.
 */
static FSEventStreamRef _stream = NULL;
/**
 * @brief Hash table storing Eio_Monitor_Backend instances.
 * Keyed by the original monitor path provided by the user.
 * Value is a pointer to Eio_Monitor_Backend.
 */
static Eina_Hash *_fsevent_monitors = NULL;
/**
 * @brief CoreFoundation mutable array holding CFStringRefs of paths to watch.
 * This array is passed to FSEventStreamCreate.
 */
static CFMutableArrayRef _paths_to_watch = NULL;
/**
 * @brief Grand Central Dispatch queue for FSEvents.
 * Callbacks from FSEvents are dispatched on this queue.
 */
static dispatch_queue_t _dispatch_queue;

/**
 * @brief Processes an FSEvent for a specific monitor.
 * This function is called via eina_hash_foreach for each active monitor.
 * It checks if the event path matches the monitor's path and, if so,
 * translates the FSEvent flags into EIO_MONITOR event codes and sends
 * the event.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The key of the hash entry (unused).
 * @param data Pointer to the Eio_Monitor_Backend for the current monitor.
 * @param fdata Pointer to the FSEvent_Info containing event details.
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
static Eina_Bool
_handle_fsevent_with_monitor(const Eina_Hash *hash EINA_UNUSED,
                             const void *key EINA_UNUSED,
                             void *data,
                             void *fdata)
{
   FSEvent_Info *event_info = (FSEvent_Info*)fdata;
   Eio_Monitor_Backend *backend = (Eio_Monitor_Backend*)data;
   FSEventStreamEventFlags flags = event_info->flags;
   unsigned int i;
   Eina_Bool is_dir;
   unsigned int length, tmp_length;

   char *tmp = NULL;

   if (backend->parent->delete_me)
     return 1;

   if (!eina_str_has_prefix(event_info->path, backend->real_path))
     {
        return 1;
     }

   length = strlen(event_info->path) - strlen(backend->real_path);
   if (length == 0)
     {
        tmp = strdup(backend->parent->path);
     }
   else
     {
        tmp_length =
          eina_stringshare_strlen(backend->parent->path) + length + 2;

        tmp = malloc(sizeof(char) * tmp_length);
        snprintf(tmp, tmp_length, "%s/%s",
                 backend->parent->path,
                 &(event_info->path[strlen(backend->real_path) + 1]));
     }

   is_dir = !!(flags & kFSEventStreamEventFlagItemIsDir);

   for (i = 0; i < sizeof (match) / sizeof (Eio_FSEvent_Table); ++i)
     if (match[i].mask & flags)
       {
          DBG("send event from %s with event %X\n", event_info->path, flags);
          _eio_monitor_send(backend->parent,
                            tmp,
                            is_dir ? *match[i].ev_dir_code : *match[i].ev_file_code);
       }

   free(tmp);
   //we have found the right event, no need to continue
   //we have found the right event, no need to continue
   return 0;
}

/**
 * @brief Handles FSEvents in the main Ecore loop.
 * This function is called asynchronously in the main thread via
 * ecore_main_loop_thread_safe_call_async. It iterates through all
 * registered monitors and dispatches events appropriately.
 * It also handles kernel dropped events by sending a generic error.
 *
 * @param data Pointer to an FSEvent_Info structure containing event details.
 *             This structure is freed by this function.
 */
static void
_main_loop_send_event(void *data)
{
   FSEvent_Info *info = (FSEvent_Info*)data;

   if (!_stream)
     {
        //this can happen, when eio_shutdown is called
        goto cleanup;
     }

   if ((info->flags & kFSEventStreamEventFlagKernelDropped) != 0)
     {
        _eio_monitor_send(NULL, "", EIO_MONITOR_ERROR);
        goto cleanup;
     }

   eina_hash_foreach(_fsevent_monitors,
                     _handle_fsevent_with_monitor,
                     info);

 cleanup:
   free(info->path);
   free(info->path);
   free(info);
}

/**
 * @brief Callback function for FSEvents.
 * This function is invoked by the FSEvents service when file system
 * events occur for the monitored paths. It processes each event,
 * packages its information into an FSEvent_Info struct, and schedules
 * _main_loop_send_event to be called on the main Ecore loop.
 *
 * @param stream_ref The FSEventStream that generated the event (unused).
 * @param ctx User-defined context data (unused).
 * @param count The number of events being reported.
 * @param event_paths An array of C strings, each representing a path where an event occurred.
 *                    Example: {"/path/to/file.txt", "/path/to/another_dir"}
 * @param event_flags An array of FSEventStreamEventFlags, corresponding to each path in event_paths.
 *                    Example: {kFSEventStreamEventFlagItemCreated, kFSEventStreamEventFlagItemRemoved}
 * @param event_ids An array of FSEventStreamEventId, corresponding to each event (unused).
 */
static void
_eio_fsevent_cb(ConstFSEventStreamRef stream_ref EINA_UNUSED,
                void *ctx EINA_UNUSED,
                size_t count,
                void *event_paths,
                const FSEventStreamEventFlags event_flags[],
                const FSEventStreamEventId event_ids[] EINA_UNUSED
                )
{
   size_t i;
   FSEvent_Info *event_info;

   for (i = 0; i < count; i++)
     {
        event_info = malloc(sizeof(FSEvent_Info));
        event_info->path = strdup(((char**)event_paths)[i]);
        event_info->flags = event_flags[i];

        ecore_main_loop_thread_safe_call_async(_main_loop_send_event,
                                               event_info);
     }
                                               event_info);
     }
}

/**
 * @brief Frees an Eio_Monitor_Backend structure.
 * This function is used as a callback for eina_hash when deleting entries.
 *
 * @param data Pointer to the Eio_Monitor_Backend to be freed.
 */
static void
_eio_fsevent_del(void *data)
{
   Eio_Monitor_Backend *backend = (Eio_Monitor_Backend *)data;
   // Note: backend->mon_path and backend->real_path are freed when the monitor is deleted
   // or when the hash is freed during shutdown.
   free(backend);
}

/**
 * @brief Determines the actual path to monitor and the canonical full path.
 * If the given path is a directory, monpath and fullpath will be the same
 * canonicalized path. If the given path is a file, monpath will be the
 * canonicalized path of its parent directory, and fullpath will be the
 * canonicalized path of the file itself.
 * This is necessary because FSEvents monitors directories.
 *
 * @param path The user-provided path to monitor.
 * @param[out] monpath Pointer to a string that will be allocated and filled
 *                     with the path to be actually monitored by FSEvents.
 *                     The caller is responsible for freeing this string.
 * @param[out] fullpath Pointer to a string that will be allocated and filled
 *                      with the canonicalized, absolute version of the input path.
 *                      The caller is responsible for freeing this string.
 */
static void
_eio_get_monitor_path(const char *path, char **monpath, char **fullpath)
{
   char realPath[PATH_MAX];
   char *realPathOk;
   char *dname = NULL;
   struct stat sb;

   realPathOk = realpath(path, realPath);
   if (realPathOk == NULL)
     {
        dname = dirname((char*)path);
        if (strcmp(dname, ".") == 0)
          {
             realPathOk = realpath("./", realPath);
          }
        else
          {
             realPathOk = realpath(dname, realPath);
          }

        if (realPathOk == NULL)
          return;
     }

   if (stat(realPath, &sb) < 0)
     {
        return;
     }

   if (S_ISDIR(sb.st_mode))
     {
        if (fullpath)
          *fullpath = strdup(realPath);
        if (monpath)
          *monpath = strdup(realPath);
     }
   else
     {
        //not a directory, monitor parent
        if (fullpath)
          *fullpath = strdup(realPath);
        dname = dirname(realPath);
        if (monpath)
          *monpath = strdup(dname);
     }
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
 * @endcond
 */

/**
 * @brief Initializes the FSEvents monitoring backend.
 * Sets up the dispatch queue, hash table for monitors, and the array
 * for paths to watch. This must be called before any monitors are added.
 */
void eio_monitor_backend_init(void)
{
   _dispatch_queue = dispatch_queue_create("org.elf.fseventqueue", NULL);
   _fsevent_monitors = eina_hash_string_small_new(_eio_fsevent_del);
   _paths_to_watch = CFArrayCreateMutable(kCFAllocatorDefault,
                                          0,
                                          &kCFTypeArrayCallBacks);
                                          &kCFTypeArrayCallBacks);
}

/**
 * @brief Shuts down the FSEvents monitoring backend.
 * Stops and releases the FSEventStream, releases the dispatch queue,
 * frees the hash table of monitors, and releases the paths array.
 * This should be called when EIO is shutting down.
 */
void eio_monitor_backend_shutdown(void)
{
   if (_stream)
     {
        FSEventStreamStop(_stream);
        FSEventStreamInvalidate(_stream);
        FSEventStreamRelease(_stream);
        _stream = NULL;
     }
   dispatch_release(_dispatch_queue);
   eina_hash_free(_fsevent_monitors);
   CFRelease(_paths_to_watch);
   CFRelease(_paths_to_watch);
}

/**
 * @brief Adds a path to be monitored by the FSEvents backend.
 * This function determines the correct directory to monitor (as FSEvents
 * works on directories), creates or updates the FSEventStream to include
 * this path, and stores backend-specific data.
 * If FSEvents setup fails, it falls back to eio_monitor_fallback_add().
 *
 * @param monitor The Eio_Monitor instance requesting to watch a path.
 *                The monitor->path field specifies the path to watch.
 *                The monitor->backend field will be set by this function.
 */
void eio_monitor_backend_add(Eio_Monitor *monitor)
{
   Eio_Monitor_Backend *backend;

   FSEventStreamEventId eventid;

   CFStringRef path = NULL;

   //the path we should monitor
   char *monitor_path = NULL;
   //the real file path
   char *real_path = NULL;

   _eio_get_monitor_path(monitor->path, &monitor_path, &real_path);

   backend = calloc(1, sizeof (Eio_Monitor_Backend));
   if (!backend)
     {
        free(monitor_path);
        eio_monitor_fallback_add(monitor);
        return;
     }

   path = CFStringCreateWithCString(NULL,
                                    monitor_path,
                                    kCFStringEncodingUTF8);

   CFArrayAppendValue(_paths_to_watch, path);

   if (_stream)
     {
        eventid = FSEventStreamGetLatestEventId(_stream);
        FSEventStreamRelease(_stream);
        _stream = NULL;
     }
   else
     {
        eventid = kFSEventStreamEventIdSinceNow;
     }

   _stream = FSEventStreamCreate(NULL,
                                 _eio_fsevent_cb,
                                 NULL,
                                 _paths_to_watch,
                                 eventid,
                                 _latency,
                                 kFSEventStreamCreateFlagFileEvents
                                 | kFSEventStreamCreateFlagNoDefer
                                );

   if (!_stream)
     {
        free(monitor_path);
        free(backend);
        eio_monitor_fallback_add(monitor);
        return;
     }

   backend->parent = monitor;
   backend->mon_path = monitor_path;
   backend->real_path = real_path;
   monitor->backend = backend;

   eina_hash_direct_add(_fsevent_monitors, monitor->path, backend);

   FSEventStreamSetDispatchQueue(_stream, _dispatch_queue);
   FSEventStreamStart(_stream);


   FSEventStreamSetDispatchQueue(_stream, _dispatch_queue);
   FSEventStreamStart(_stream);


}

/**
 * @brief Removes a path from being monitored by the FSEvents backend.
 * This function updates the FSEventStream to no longer watch the specified
 * path. It also cleans up backend-specific data associated with the monitor.
 * If the FSEventStream was not active, it falls back to eio_monitor_fallback_del().
 *
 * @param monitor The Eio_Monitor instance whose path should be removed.
 *                The monitor->path field specifies the path to stop watching.
 *                The monitor->backend field will be cleared by this function.
 */
void eio_monitor_backend_del(Eio_Monitor *monitor)
{
   Eio_Monitor_Backend *backend;
   CFStringRef path = NULL;
   FSEventStreamEventId eventid;
   char *monitor_path;

   if (!_stream)
     {
        eio_monitor_fallback_del(monitor);
        return;
     }

   _eio_get_monitor_path(monitor->path, &monitor_path, NULL);

   eventid = FSEventStreamGetLatestEventId(_stream);
   FSEventStreamRelease(_stream);
   _stream = NULL;

   path = CFStringCreateWithCString(NULL,
                                    monitor_path,
                                    kCFStringEncodingUTF8);

   CFIndex idx =
     CFArrayGetFirstIndexOfValue(_paths_to_watch,
                                 CFRangeMake(0,
                                             CFArrayGetCount(_paths_to_watch)
                                             ),
                                 path);

   if (idx != -1)
     {
        CFArrayRemoveValueAtIndex(_paths_to_watch, idx);
     }

   if (CFArrayGetCount(_paths_to_watch) > 0)
     {
        _stream = FSEventStreamCreate(NULL,
                                      _eio_fsevent_cb,
                                      NULL,
                                      _paths_to_watch,
                                      eventid,
                                      _latency,
                                      kFSEventStreamCreateFlagFileEvents
                                      | kFSEventStreamCreateFlagNoDefer
                                      );
     }
   backend = monitor->backend;
   monitor->backend = NULL;
   if (!backend) return;

   // Free the paths stored in the backend before deleting from hash
   free(backend->mon_path);
   free(backend->real_path);
   eina_hash_del(_fsevent_monitors, monitor->path, backend);
}

/**
 * @brief Checks if a given path is relevant to a specific monitor.
 * In the Cocoa FSEvents backend, this function currently always returns EINA_TRUE,
 * as filtering is primarily handled by comparing the event path prefix with
 * the monitor's real_path in _handle_fsevent_with_monitor.
 * More specific context checking could be implemented here if needed.
 *
 * @param monitor The monitor instance.
 * @param path The path of the event to check.
 * @return EINA_TRUE if the path is relevant to the monitor, EINA_FALSE otherwise.
 */
Eina_Bool eio_monitor_context_check(const Eio_Monitor *monitor EINA_UNUSED, const char *path EINA_UNUSED)
{
   return EINA_TRUE;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
