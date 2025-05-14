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
#include "Eina.h"
#include "Ecore_File.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/stat.h>

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

#define KEVENT_NUM_EVENTS 5

/**
 * @brief Backend-specific data for kqueue-based Eio_Monitor.
 *
 * This structure holds all the necessary information for the kqueue backend
 * to monitor a specific path. It includes a reference to the parent Eio_Monitor,
 * a list of previously seen files (for detecting changes), and the file
 * descriptor associated with the kqueue watch.
 */
struct _Eio_Monitor_Backend
{
   Eio_Monitor *parent; /**< Pointer to the parent Eio_Monitor instance. */
   Eina_List *prev_list; /**< List of Eio_File_Info for the previously scanned directory state. Used to detect created/deleted files. */
   int fd; /**< File descriptor for the kqueue event source (the monitored file or directory). */
};

/**
 * @brief Structure to hold information about a file.
 *
 * This is used to store the path and stat information for files within
 * a monitored directory, primarily to detect changes between scans.
 */
typedef struct _Eio_File_Info Eio_File_Info;
struct _Eio_File_Info
{
   const char *path; /**< The full path to the file, stringshared. */
   Eina_Stat st;     /**< Stat information for the file. */
};

static Ecore_Fd_Handler *_kqueue_fd = NULL; /**< Global Ecore_Fd_Handler for the kqueue instance. All kqueue events are processed through this. */
static Eina_Hash *_kevent_monitors = NULL; /**< Hash table mapping kqueue file descriptors (ident) to their corresponding Eio_Monitor_Backend. Key: (int *)fd, Value: (Eio_Monitor_Backend *)backend. */

/**
 * @brief Frees a list of Eio_File_Info structures.
 *
 * Iterates through the list, unreferences the stringshared path,
 * and frees the Eio_File_Info structure itself.
 *
 * @param list The Eina_List of Eio_File_Info to free.
 */
static void
_eio_kevent_ls_free(Eina_List *list)
{
   Eio_File_Info *file;

   EINA_LIST_FREE(list, file)
     {
        eina_stringshare_del(file->path);
        free(file);
     }
}

/**
 * @brief Cleans up an Eio_Monitor_Backend structure.
 *
 * This function is typically used as a callback when an Eio_Monitor_Backend
 * is removed from the _kevent_monitors hash. It frees the list of
 * previous file states, closes the associated file descriptor, and frees
 * the backend structure itself.
 *
 * @param data Pointer to the Eio_Monitor_Backend to be deleted.
 */
static void
_eio_kevent_del(void *data)
{
   Eio_Monitor_Backend *emb = data;

   _eio_kevent_ls_free(emb->prev_list);

   if (emb->fd)
      close(emb->fd);

   free(emb);
}

/**
 * @brief Lists the contents of a directory and stores their stat info.
 *
 * Creates a list of Eio_File_Info structures, each representing a file or
 * subdirectory within the given directory. This list is used to compare
 * against a previous state to detect changes.
 *
 * @param directory The path to the directory to list.
 * @return An Eina_List of Eio_File_Info structures, or NULL on failure.
 *         The caller is responsible for freeing this list using _eio_kevent_ls_free().
 *         Example of Eina_List structure:
 *         [
 *           { path: "/path/to/dir/file1.txt", st: { ino: 123, mtime: ..., ... } },
 *           { path: "/path/to/dir/subdir", st: { ino: 456, mtime: ..., ... } }
 *         ]
 */
static Eina_List *
_eio_kevent_ls(const char *directory)
{
   Eina_Iterator *it;
   Eina_File_Direct_Info *info;
   Eina_List *files = NULL;

   it = eina_file_direct_ls(directory);
   if (!it) return NULL;

   EINA_ITERATOR_FOREACH(it, info)
     {
        Eio_File_Info *file = malloc(sizeof(Eio_File_Info));
        if (eina_file_statat(eina_iterator_container_get(it), info, &file->st))
          {
             free(file);
             continue;
          }
        file->path = eina_stringshare_add(info->path);
        files = eina_list_append(files, file);
     }

   eina_iterator_free(it);

   return files;
}

/**
 * @brief Compares the current state of a directory with its previous state to find changes.
 *
 * This function is called when a directory modification event (NOTE_WRITE or NOTE_ATTRIB
 * on the directory itself) is detected by kqueue. It lists the current directory
 * contents and compares it with the `prev_list` stored in the backend.
 * It then sends appropriate EIO_MONITOR events (CREATED, DELETED, MODIFIED)
 * for files and directories.
 *
 * @param backend The Eio_Monitor_Backend associated with the monitored directory.
 */
static void
_eio_kevent_event_find(Eio_Monitor_Backend *backend)
{
   Eina_List *l, *l2;
   Eio_File_Info *file, *file2;
   Eina_List *next_list = _eio_kevent_ls(backend->parent->path);

   EINA_LIST_FOREACH(backend->prev_list, l, file)
     {
        Eina_Bool exists = EINA_FALSE;
        EINA_LIST_FOREACH(next_list, l2, file2)
          {
             if (file->st.ino == file2->st.ino)
               {
                  if (file->path == file2->path)
                    exists = EINA_TRUE;

                  if (file->st.mtime != file2->st.mtime)
                    {
                       if (S_ISDIR(file->st.mode))
                         _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_DIRECTORY_MODIFIED);
                       else
                         _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_FILE_MODIFIED);
                    }
               }
          }

        if (!exists)
          {
             if (S_ISDIR(file->st.mode))
               _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_DIRECTORY_DELETED);
             else
               _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_FILE_DELETED);
          }
     }

   EINA_LIST_FOREACH(next_list, l, file)
     {
        Eina_Bool exists = EINA_FALSE;
        EINA_LIST_FOREACH(backend->prev_list, l2, file2)
          {
             if ((file->path == file2->path) &&
                         (file->st.ino == file2->st.ino))
               {
                  exists = EINA_TRUE;
                  break;
               }
          }

        if (!exists)
          {
             if (S_ISDIR(file->st.mode))
               _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_DIRECTORY_CREATED);
             else
               _eio_monitor_send(backend->parent, file->path, EIO_MONITOR_FILE_CREATED);
          }
     }

   _eio_kevent_ls_free(backend->prev_list);

   backend->prev_list = next_list;
}

/**
 * @brief Ecore_Fd_Handler callback for kqueue events.
 *
 * This function is called by the Ecore main loop when there is activity
 * on the global kqueue file descriptor. It retrieves events from kqueue,
 * finds the corresponding Eio_Monitor_Backend, and processes the events.
 *
 * @param data User data, unused in this handler.
 * @param fdh The Ecore_Fd_Handler that triggered this callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_eio_kevent_handler(void *data EINA_UNUSED, Ecore_Fd_Handler *fdh)
{
   Eio_Monitor_Backend *backend;
   struct kevent evs[KEVENT_NUM_EVENTS];
   int event_code = 0;
   const struct timespec timeout = { 0, 0 };

   int res = kevent(ecore_main_fd_handler_fd_get(fdh), 0, 0, evs, KEVENT_NUM_EVENTS, &timeout);

   for(int i=0; i<res; ++i)
     {
        backend = eina_hash_find(_kevent_monitors, &evs[i].ident);
        if(evs[i].fflags & NOTE_DELETE)
          {
             event_code = EIO_MONITOR_SELF_DELETED;
             _eio_monitor_send(backend->parent, backend->parent->path, event_code);
          }
        if(evs[i].fflags & NOTE_WRITE || evs[i].fflags & NOTE_ATTRIB)
          {
             /* Handle directory/file creation and deletion */
             if (ecore_file_is_dir(backend->parent->path))
               _eio_kevent_event_find(backend);
             else
               {
                  event_code = EIO_MONITOR_FILE_MODIFIED;
                  _eio_monitor_send(backend->parent, backend->parent->path, event_code);
               }
          }
     }

   return ECORE_CALLBACK_RENEW;
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
 * @brief Initializes the kqueue backend for Eio_Monitor.
 *
 * This function sets up the global kqueue file descriptor and the
 * Ecore_Fd_Handler to process kqueue events. It also initializes the hash
 * table used to store active monitors. This must be called before any
 * kqueue-based monitors can be added.
 * It is called from eio_monitor_init().
 */
void eio_monitor_backend_init(void)
{
   int fd;

   if (_kqueue_fd != NULL) return; // already initialized

   fd = kqueue();
   if (fd < 0) return;

   _kqueue_fd = ecore_main_fd_handler_add(fd, ECORE_FD_READ, _eio_kevent_handler, NULL, NULL, NULL);
   if (!_kqueue_fd)
     {
        close(fd);
        return;
     }

   _kevent_monitors = eina_hash_int32_new(_eio_kevent_del);
}

/**
 * @brief Shuts down the kqueue backend for Eio_Monitor.
 *
 * Cleans up resources used by the kqueue backend, including freeing the
 * hash table of monitors, deleting the Ecore_Fd_Handler, and closing
 * the global kqueue file descriptor.
 * It is called from eio_monitor_shutdown().
 */
void eio_monitor_backend_shutdown(void)
{
   int fd;

   if (!_kqueue_fd) return;

   eina_hash_free(_kevent_monitors);

   fd = ecore_main_fd_handler_fd_get(_kqueue_fd);
   ecore_main_fd_handler_del(_kqueue_fd);
   _kqueue_fd = NULL;

   if (fd < 0)
     return;

   close(fd);
}

/**
 * @brief Adds a new path to be monitored using the kqueue backend.
 *
 * Creates an Eio_Monitor_Backend for the given Eio_Monitor, opens the
 * specified path, and registers it with the kqueue system for event
 * notification. If the path is a directory, its initial contents are listed.
 *
 * @param monitor The Eio_Monitor instance representing the path to monitor.
 *                The `monitor->backend` will be populated by this function.
 */
void eio_monitor_backend_add(Eio_Monitor *monitor)
{
   struct kevent e;
   struct stat st;
   Eio_Monitor_Backend* backend;
   int fd, res = 0;

   if (!_kqueue_fd)
     {
        return;
     }

   backend = calloc(1, sizeof (Eio_Monitor_Backend));
   if (!backend) return;

   res = stat(monitor->path, &st);
   if (res) goto error;

   fd = open(monitor->path, O_RDONLY);
   if (fd < 0) goto error;

   eina_file_close_on_exec(fd, EINA_TRUE);
   backend->fd = fd;
   backend->parent = monitor;
   monitor->backend = backend;

   if (ecore_file_is_dir(backend->parent->path))
     backend->prev_list = _eio_kevent_ls(backend->parent->path);

   eina_hash_direct_add(_kevent_monitors, &backend->fd, backend);

   EV_SET(&e, fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
          NOTE_DELETE | NOTE_WRITE | NOTE_ATTRIB, 0, NULL);
   res = kevent(ecore_main_fd_handler_fd_get(_kqueue_fd), &e, 1, 0, 0, 0);
   if (res)
     {
        eina_hash_del(_kevent_monitors, &backend->fd, backend);
     }

   return;

error:
   free(backend);
}

/**
 * @brief Stops monitoring a path with the kqueue backend.
 *
 * Removes the Eio_Monitor_Backend associated with the given Eio_Monitor
 * from the internal hash table. This implicitly triggers the cleanup
 * of the backend resources (closing fd, freeing lists) via the
 * _eio_kevent_del hash free callback.
 *
 * @param monitor The Eio_Monitor instance to stop monitoring.
 *                `monitor->backend` will be set to NULL.
 */
void eio_monitor_backend_del(Eio_Monitor *monitor)
{
   Eio_Monitor_Backend *backend;

   backend = monitor->backend;
   monitor->backend = NULL;

   eina_hash_del(_kevent_monitors, &backend->fd, backend);
}

/**
 * @brief Checks if a given path was part of the last known state of a monitored directory.
 *
 * This function is used to determine if a path reported in an event (e.g., a deleted file)
 * was actually known to be part of the monitored directory's contents before the event.
 * It iterates through the `prev_list` of the monitor's backend.
 *
 * @param monitor The Eio_Monitor instance.
 * @param path The path to check.
 * @return EINA_TRUE if the path was found in the previous list of directory contents,
 *         EINA_FALSE otherwise.
 */
Eina_Bool eio_monitor_context_check(const Eio_Monitor *monitor, const char *path)
{
   Eio_Monitor_Backend *backend = monitor->backend;
   Eina_List *l;
   Eio_File_Info *file;

   EINA_LIST_FOREACH(backend->prev_list, l, file)
     {
        if (eina_streq(file->path, path))
          {
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}



/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
