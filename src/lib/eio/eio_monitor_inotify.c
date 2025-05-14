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

#ifdef HAVE_SYS_INOTIFY_H
# include <sys/inotify.h>
#endif

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Maps inotify event masks to Eio_Monitor event codes.
 */
typedef struct _Eio_Inotify_Table Eio_Inotify_Table;

/**
 * @brief Structure to hold the mapping between inotify mask and Eio event codes.
 */
struct _Eio_Inotify_Table
{
   int mask; /**< The inotify event mask (e.g., IN_ATTRIB). */
   int *ev_file_code; /**< Pointer to the EIO_MONITOR event code for a file event. */
   int *ev_dir_code; /**< Pointer to the EIO_MONITOR event code for a directory event. */
};

/**
 * @brief Backend-specific data for an Eio_Monitor instance using inotify.
 */
struct _Eio_Monitor_Backend
{
   Eio_Monitor *parent; /**< Pointer to the parent Eio_Monitor object. */

   int hwnd; /**< The watch descriptor returned by inotify_add_watch. */
};

static Ecore_Fd_Handler *_inotify_fdh = NULL; /**< Ecore file descriptor handler for the inotify file descriptor. */
static Eina_Hash *_inotify_monitors = NULL; /**< Hash table mapping watch descriptors (int) to Eio_Monitor_Backend pointers. Key: &backend->hwnd, Value: backend */

#define EIO_INOTIFY_LINE(Ino, Ef, Ed)		\
  { Ino, &EIO_MONITOR_##Ef, &EIO_MONITOR_##Ed }

static const Eio_Inotify_Table match[] = {
  EIO_INOTIFY_LINE(IN_ATTRIB, FILE_MODIFIED, DIRECTORY_MODIFIED),
  EIO_INOTIFY_LINE(IN_CLOSE_WRITE, FILE_CLOSED, DIRECTORY_CLOSED),
  EIO_INOTIFY_LINE(IN_MODIFY, FILE_MODIFIED, DIRECTORY_MODIFIED),
  EIO_INOTIFY_LINE(IN_MOVED_FROM, FILE_DELETED, DIRECTORY_DELETED),
  EIO_INOTIFY_LINE(IN_MOVED_TO, FILE_CREATED, DIRECTORY_CREATED),
  EIO_INOTIFY_LINE(IN_DELETE, FILE_DELETED, DIRECTORY_DELETED),
  EIO_INOTIFY_LINE(IN_CREATE, FILE_CREATED, DIRECTORY_CREATED),
  EIO_INOTIFY_LINE(IN_DELETE_SELF, SELF_DELETED, SELF_DELETED),
  EIO_INOTIFY_LINE(IN_MOVE_SELF, SELF_DELETED, SELF_DELETED),
  EIO_INOTIFY_LINE(IN_UNMOUNT, SELF_DELETED, SELF_DELETED)
};

/**
 * @brief Frees an Eio_Monitor_Backend structure.
 * This function is used as a callback for eina_hash_free_buckets.
 * It removes the inotify watch if it's still active.
 *
 * @param data Pointer to the Eio_Monitor_Backend to free.
 */
static void
_eio_inotify_del(void *data)
{
   Eio_Monitor_Backend *emb = data;
   int fd;

   if (emb->hwnd)
     {
        fd = ecore_main_fd_handler_fd_get(_inotify_fdh);
        inotify_rm_watch(fd, emb->hwnd);
        emb->hwnd = 0;
     }

   free(emb);
}

/**
 * @brief Processes inotify events and sends corresponding Eio_Monitor events.
 *
 * @param backend The Eio_Monitor_Backend associated with the event.
 * @param file The name of the file or directory that triggered the event (can be NULL).
 * @param mask The inotify event mask.
 */
static void
_eio_inotify_events(Eio_Monitor_Backend *backend, const char *file, int mask)
{
   char *tmp;
   unsigned int length;
   unsigned int tmp_length;
   unsigned int i;
   Eina_Bool is_dir;

   if (backend->parent->delete_me)
     return;

   length = file ? strlen(file) : 0;
   tmp_length = eina_stringshare_strlen(backend->parent->path) + length + 2;
   tmp = alloca(sizeof (char) * tmp_length);

   if (length > 0)
     snprintf(tmp, tmp_length, "%s/%s", backend->parent->path, file);
   else
     snprintf(tmp, tmp_length, "%s", backend->parent->path);


   is_dir = !!(mask & IN_ISDIR);

   for (i = 0; i < sizeof (match) / sizeof (Eio_Inotify_Table); ++i)
     if (match[i].mask & mask)
       {
          _eio_monitor_send(backend->parent, tmp, is_dir ? *match[i].ev_dir_code : *match[i].ev_file_code);
       }

   /* special case for IN_IGNORED */
   if (mask & IN_IGNORED)
     {
        _eio_monitor_rename(backend->parent, tmp);
     }
}

/**
 * @brief Ecore_Fd_Handler callback for inotify events.
 * Reads events from the inotify file descriptor and dispatches them.
 *
 * @param data User data (unused).
 * @param fdh The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_eio_inotify_handler(void *data EINA_UNUSED, Ecore_Fd_Handler *fdh)
{
   Eio_Monitor_Backend *backend;
   unsigned char buffer[16384];
   struct inotify_event *event;
   int i = 0, fd;
   int event_size;
   ssize_t size;

   fd = ecore_main_fd_handler_fd_get(fdh);
   if (fd < 0) return ECORE_CALLBACK_RENEW;

   size = read(fd, buffer, sizeof(buffer));
   while ((i + (int) sizeof(struct inotify_event)) <= (int) size)
     {
        event = (struct inotify_event *)&buffer[i];
        event_size = sizeof(struct inotify_event) + event->len;
        if ((event_size + i) > size) break ;
        i += event_size;

        // No need to waste time looking up for just destroyed handler
        if ((event->mask & IN_IGNORED)) continue ;

        backend = eina_hash_find(_inotify_monitors, &event->wd);
        if (!backend) continue ;
        if (!backend->parent) continue ;

        _eio_inotify_events(backend, (event->len ? event->name : NULL), event->mask);
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @endcond
 */
static Eina_Bool reseting = EINA_FALSE; /**< Flag to indicate if the monitor is currently being reset (e.g., after a fork). */

/**
 * @brief Resets the inotify monitoring system.
 * This is typically called after a fork to re-initialize inotify watches
 * in the new process. It shuts down and re-initializes the backend,
 * then re-adds all existing monitors.
 *
 * @param data User data (unused).
 */
static void
_eio_monitor_reset(void *data EINA_UNUSED)
{
   Eina_Hash *h = _inotify_monitors;
   Eina_Iterator *it;
   Eio_Monitor_Backend *backend;

   _inotify_monitors = NULL;
   reseting = 1;
   eio_monitor_backend_shutdown();
   eio_monitor_backend_init();
   it = eina_hash_iterator_data_new(h);
   EINA_ITERATOR_FOREACH(it, backend)
     eio_monitor_backend_add(backend->parent);
   reseting = 0;
   eina_iterator_free(it);
   eina_hash_free(h);
}
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
 * @brief Initializes the inotify backend for Eio_Monitor.
 * Sets up the inotify file descriptor and the Ecore_Fd_Handler to listen for events.
 * Registers a callback to handle reset after fork.
 */
void eio_monitor_backend_init(void)
{
   int fd;

   fd = inotify_init();
   if (fd < 0)
     return;

   eina_file_close_on_exec(fd, EINA_TRUE);

   _inotify_fdh = ecore_main_fd_handler_add(fd, ECORE_FD_READ, _eio_inotify_handler, NULL, NULL, NULL);
   if (!_inotify_fdh)
     {
        close(fd);
        return;
     }

   _inotify_monitors = eina_hash_int32_new(_eio_inotify_del);
   if (!reseting)
     ecore_fork_reset_callback_add(_eio_monitor_reset, NULL);
}

/**
 * @brief Shuts down the inotify backend for Eio_Monitor.
 * Frees the hash of monitors, closes the inotify file descriptor,
 * and removes the Ecore_Fd_Handler.
 * Unregisters the fork reset callback.
 */
void eio_monitor_backend_shutdown(void)
{
   int fd;

   if (!_inotify_fdh) return;

   eina_hash_free(_inotify_monitors);

   fd = ecore_main_fd_handler_fd_get(_inotify_fdh);
   ecore_main_fd_handler_del(_inotify_fdh);
   _inotify_fdh = NULL;

   if (fd < 0)
     return;

   close(fd);
   if (!reseting)
     ecore_fork_reset_callback_del(_eio_monitor_reset, NULL);
}

/**
 * @brief Adds a path to be monitored by the inotify backend.
 * If inotify is not available or fails, it falls back to a generic polling mechanism.
 *
 * @param monitor The Eio_Monitor instance to add.
 */
void eio_monitor_backend_add(Eio_Monitor *monitor)
{
   Eio_Monitor_Backend *backend;
   int mask =
     IN_ATTRIB |
     IN_CLOSE_WRITE |
     IN_MOVED_FROM |
     IN_MOVED_TO |
     IN_DELETE |
     IN_CREATE |
     IN_MODIFY |
     IN_DELETE_SELF |
     IN_MOVE_SELF |
     IN_UNMOUNT;

   if (!_inotify_fdh)
     {
        eio_monitor_fallback_add(monitor);
        return;
     }

   backend = calloc(1, sizeof (Eio_Monitor_Backend));
   if (!backend)
     {
        eio_monitor_fallback_add(monitor);
        return;
     }

   backend->parent = monitor;
   backend->hwnd = inotify_add_watch(ecore_main_fd_handler_fd_get(_inotify_fdh), monitor->path, mask);
   if (backend->hwnd < 0)
     {
        if (errno != EACCES)
          eio_monitor_fallback_add(monitor);

        free(backend);
        return;
     }

   monitor->backend = backend;

   eina_hash_direct_add(_inotify_monitors, &backend->hwnd, backend);
}

/**
 * @brief Removes a path from being monitored by the inotify backend.
 *
 * @param monitor The Eio_Monitor instance to remove.
 */
void eio_monitor_backend_del(Eio_Monitor *monitor)
{
   Eio_Monitor_Backend *backend;

   if (!_inotify_fdh)
     eio_monitor_fallback_del(monitor);

   backend = monitor->backend;
   monitor->backend = NULL;
   if (!backend) return;

   backend->parent = NULL;

   eina_hash_del(_inotify_monitors, &backend->hwnd, backend);
}

/**
 * @brief Checks if the given path is valid in the current monitoring context.
 * For inotify, this always returns EINA_TRUE as it doesn't have specific
 * context restrictions like some other backends might (e.g., kernel-level
 * limitations for certain path types).
 *
 * @param monitor The Eio_Monitor instance (unused).
 * @param path The path to check (unused).
 * @return EINA_TRUE, indicating the path is considered valid for inotify monitoring.
 */
Eina_Bool eio_monitor_context_check(const Eio_Monitor *monitor EINA_UNUSED, const char *path EINA_UNUSED)
{
   return EINA_TRUE;
}


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
