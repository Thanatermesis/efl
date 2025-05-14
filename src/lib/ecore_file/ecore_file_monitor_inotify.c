#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "ecore_file_private.h"

/*
 * TODO:
 *
 * - Listen to these events:
 *   IN_ACCESS, IN_ATTRIB, IN_CLOSE_WRITE, IN_CLOSE_NOWRITE, IN_OPEN
 * - Read all events first, then call the callbacks. This will prevent several
 *   callbacks with the typic save cycle (delete file, new file)
 * - Listen to IN_IGNORED, emitted when the watch is removed
 */

#include <sys/inotify.h>

/**
 * @file ecore_file_monitor_inotify.c
 * @brief Ecore file monitor backend implementation using inotify.
 *
 * This file provides the inotify-specific logic for monitoring file
 * system events.
 */

typedef struct _Ecore_File_Monitor_Inotify Ecore_File_Monitor_Inotify;

#define ECORE_FILE_MONITOR_INOTIFY(x) ((Ecore_File_Monitor_Inotify *)(x))

/**
 * @brief Structure extending Ecore_File_Monitor for inotify-specific data.
 */
struct _Ecore_File_Monitor_Inotify
{
   Ecore_File_Monitor  monitor; /**< Base Ecore_File_Monitor structure. */
   int                 wd;      /**< Inotify watch descriptor. */
};

static Ecore_Fd_Handler *_fdh = NULL; /**< FD handler for the inotify file descriptor. */
static Ecore_File_Monitor    *_monitors = NULL; /**< Inlist of active monitors. */
static pid_t             _inotify_fd_pid = -1; /**< PID of the process that initialized inotify, used for fork detection. */

static Eina_Bool           _ecore_file_monitor_inotify_handler(void *data, Ecore_Fd_Handler *fdh);
static Eina_List *_ecore_file_monitor_inotify_monitor_find(int wd);
static void                _ecore_file_monitor_inotify_events(Ecore_File_Monitor *em, char *file, int mask);
static int                 _ecore_file_monitor_inotify_monitor(Ecore_File_Monitor *em, const char *path);
#if 0
static void                _ecore_file_monitor_inotify_print(char *file, int mask);
#endif

static Eina_Bool reseting; /**< Flag to indicate if the monitor system is currently being reset (e.g., after a fork). */
static Eina_Hash *monitor_hash; /**< Hash table mapping watch descriptors (wd) to a list of Ecore_File_Monitor instances. */

/**
 * @brief Resets the inotify monitoring system.
 *
 * This function is typically called after a fork to re-initialize inotify
 * watches in the new process. It shuts down and re-initializes the backend,
 * then re-adds all existing monitors.
 *
 * @param data Unused.
 */
static void
_ecore_file_monitor_inotify_reset(void *data EINA_UNUSED)
{
   Eina_Iterator *it;
   Ecore_File_Monitor *em;
   Eina_Hash *h = monitor_hash;
   monitor_hash = NULL;
   reseting = 1;
   ecore_file_monitor_backend_shutdown();
   ecore_file_monitor_backend_init();
   it = eina_hash_iterator_data_new(h);
   EINA_ITERATOR_FOREACH(it, em)
     _ecore_file_monitor_inotify_monitor(em, em->path);
   eina_iterator_free(it);
   eina_hash_free(h);
   reseting = 0;
}

/**
 * @brief Initializes the inotify backend for file monitoring.
 *
 * Sets up the inotify file descriptor and registers an FD handler
 * to process inotify events. Also registers a callback to handle
 * resetting the monitor after a fork.
 *
 * @return 1 on success, 0 on failure.
 */
int
ecore_file_monitor_backend_init(void)
{
   int fd;

   fd = inotify_init();
   if (fd < 0)
     return 0;

   eina_file_close_on_exec(fd, EINA_TRUE);

   _fdh = ecore_main_fd_handler_add(fd, ECORE_FD_READ, _ecore_file_monitor_inotify_handler,
                                    NULL, NULL, NULL);
   if (!_fdh)
     {
        close(fd);
        return 0;
     }

   if (!reseting)
     ecore_fork_reset_callback_add(_ecore_file_monitor_inotify_reset, NULL);
   _inotify_fd_pid = getpid();
   monitor_hash = eina_hash_int32_new(NULL);
   return 1;
}

/**
 * @brief Shuts down the inotify backend for file monitoring.
 *
 * Removes all active monitors, closes the inotify file descriptor,
 * and unregisters the FD handler and fork callback.
 *
 * @return 1 on success.
 */
int
ecore_file_monitor_backend_shutdown(void)
{
   int fd;

   while(_monitors)
        ecore_file_monitor_backend_del(_monitors);

   if (_fdh)
     {
        fd = ecore_main_fd_handler_fd_get(_fdh);
        ecore_main_fd_handler_del(_fdh);
        if (fd > -1)
          close(fd);
        _fdh = NULL;
     }
   eina_hash_free(monitor_hash);
   monitor_hash = NULL;
   _inotify_fd_pid = -1;
   if (!reseting)
     ecore_fork_reset_callback_del(_ecore_file_monitor_inotify_reset, NULL);
   return 1;
}

/**
 * @brief Adds a new file or directory to monitor using the inotify backend.
 *
 * Creates a new monitor for the given path and registers it with the
 * inotify system. If the inotify system was initialized by a different
 * process (e.g., after a fork), it will be reset.
 *
 * @param path The file or directory path to monitor.
 * @param func The callback function to execute when an event occurs.
 * @param data User data to pass to the callback function.
 * @return A pointer to the new Ecore_File_Monitor on success, or NULL on failure.
 */
Ecore_File_Monitor *
ecore_file_monitor_backend_add(const char *path,
                               void (*func) (void *data, Ecore_File_Monitor *em,
                                             Ecore_File_Event event,
                                             const char *path),
                               void *data)
{
   Ecore_File_Monitor *em;
   char *path2;
   size_t len;

   if (_inotify_fd_pid == -1) return NULL;

   if (_inotify_fd_pid != getpid())
     _ecore_file_monitor_inotify_reset(NULL);

   em = (Ecore_File_Monitor *)calloc(1, sizeof(Ecore_File_Monitor_Inotify));
   if (!em) return NULL;

   em->func = func;
   em->data = data;

   len = strlen(path);
   path2 = alloca(len + 1);
   strcpy(path2, path);
   if (path2[len - 1] == '/' && strcmp(path2, "/")) path2[len - 1] = 0;
   em->path = eina_stringshare_add(path2);

   _monitors = ECORE_FILE_MONITOR(eina_inlist_append(EINA_INLIST_GET(_monitors), EINA_INLIST_GET(em)));

   if (!_ecore_file_monitor_inotify_monitor(em, em->path))
     return NULL;

   return em;
}

/**
 * @brief Deletes a file monitor from the inotify backend.
 *
 * Removes the specified monitor from the list of active monitors and
 * tells inotify to stop watching the associated path.
 *
 * @param em The Ecore_File_Monitor to delete.
 */
void
ecore_file_monitor_backend_del(Ecore_File_Monitor *em)
{
   int fd;

   if (_monitors)
     _monitors = ECORE_FILE_MONITOR(eina_inlist_remove(EINA_INLIST_GET(_monitors), EINA_INLIST_GET(em)));
   if (ECORE_FILE_MONITOR_INOTIFY(em)->wd >= 0)
     eina_hash_list_remove(monitor_hash, &ECORE_FILE_MONITOR_INOTIFY(em)->wd, em);

   fd = ecore_main_fd_handler_fd_get(_fdh);
   if (ECORE_FILE_MONITOR_INOTIFY(em)->wd >= 0)
     inotify_rm_watch(fd, ECORE_FILE_MONITOR_INOTIFY(em)->wd);
   eina_stringshare_del(em->path);
   free(em);
}

/**
 * @brief Handles incoming inotify events from the file descriptor.
 *
 * Reads events from the inotify file descriptor, finds the corresponding
 * monitor(s), and dispatches the events.
 *
 * @param data Unused.
 * @param fdh The Ecore_Fd_Handler that triggered this callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_file_monitor_inotify_handler(void *data EINA_UNUSED, Ecore_Fd_Handler *fdh)
{
   Eina_List *l, *ll, *ll2;
   Ecore_File_Monitor *em;
   char buffer[16384];
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

        l = _ecore_file_monitor_inotify_monitor_find(event->wd);
        EINA_LIST_FOREACH_SAFE(l, ll, ll2, em)
          _ecore_file_monitor_inotify_events(em, (event->len ? event->name : NULL), event->mask);
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Finds the list of monitors associated with a watch descriptor.
 *
 * @param wd The inotify watch descriptor.
 * @return An Eina_List of Ecore_File_Monitor instances associated with the wd,
 *         or NULL if not found.
 */
static Eina_List *
_ecore_file_monitor_inotify_monitor_find(int wd)
{
   return eina_hash_find(monitor_hash, &wd);
}

/**
 * @brief Processes inotify events and triggers corresponding Ecore_File_Event callbacks.
 *
 * Translates inotify event masks into Ecore_File_Event types and calls the
 * monitor's callback function.
 *
 * @param em The Ecore_File_Monitor that received the event.
 * @param file The name of the file or directory within the monitored path,
 *             or NULL if the event pertains to the monitored path itself.
 * @param mask The inotify event mask.
 */
static void
_ecore_file_monitor_inotify_events(Ecore_File_Monitor *em, char *file, int mask)
{
   char buf[PATH_MAX];
   int isdir;

   if ((file) && (file[0]))
     snprintf(buf, sizeof(buf), "%s/%s", em->path, file);
   else
     {
        strncpy(buf, em->path, sizeof(buf));
        buf[PATH_MAX - 1] = 0;
     }
   isdir = mask & IN_ISDIR;

#if 0
   _ecore_file_monitor_inotify_print(buf, mask);
#endif

   if (mask & IN_ATTRIB)
     {
        em->func(em->data, em, ECORE_FILE_EVENT_MODIFIED, buf);
     }
   if (mask & IN_CLOSE_WRITE)
     {
        if (!isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_CLOSED, buf);
     }
   if (mask & IN_MODIFY)
     {
        if (!isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_MODIFIED, buf);
     }
   if (mask & IN_MOVED_FROM)
     {
        if (isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_DELETED_DIRECTORY, buf);
        else
          em->func(em->data, em, ECORE_FILE_EVENT_DELETED_FILE, buf);
     }
   if (mask & IN_MOVED_TO)
     {
        if (isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_CREATED_DIRECTORY, buf);
        else
          em->func(em->data, em, ECORE_FILE_EVENT_CREATED_FILE, buf);
     }
   if (mask & IN_DELETE)
     {
        if (isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_DELETED_DIRECTORY, buf);
        else
          em->func(em->data, em, ECORE_FILE_EVENT_DELETED_FILE, buf);
     }
   if (mask & IN_CREATE)
     {
        if (isdir)
          em->func(em->data, em, ECORE_FILE_EVENT_CREATED_DIRECTORY, buf);
        else
          em->func(em->data, em, ECORE_FILE_EVENT_CREATED_FILE, buf);
     }
   if (mask & IN_DELETE_SELF)
     {
        em->func(em->data, em, ECORE_FILE_EVENT_DELETED_SELF, em->path);
     }
   if (mask & IN_MOVE_SELF)
     {
        /* We just call delete. The dir is gone... */
        em->func(em->data, em, ECORE_FILE_EVENT_DELETED_SELF, em->path);
     }
   if (mask & IN_UNMOUNT)
     {
        /* We just call delete. The dir is gone... */
        em->func(em->data, em, ECORE_FILE_EVENT_DELETED_SELF, em->path);
     }
   if (mask & IN_IGNORED)
     /* The watch is removed. If the file name still exists monitor the new one,
      * else delete it */
     _ecore_file_monitor_inotify_monitor(em, em->path);
}

/**
 * @brief Adds or re-adds an inotify watch for a given monitor and path.
 *
 * This function is called to initially set up a watch or to re-establish
 * a watch (e.g., after an IN_IGNORED event).
 *
 * @param em The Ecore_File_Monitor to associate with the watch.
 * @param path The file system path to monitor.
 * @return 1 on success, 0 on failure (e.g., if inotify_add_watch fails).
 *         If it fails, the monitor `em` is deleted.
 */
static int
_ecore_file_monitor_inotify_monitor(Ecore_File_Monitor *em, const char *path)
{
   int mask = IN_ATTRIB | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO |
     IN_DELETE | IN_CREATE | IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF |
     IN_UNMOUNT;

   ECORE_FILE_MONITOR_INOTIFY(em)->wd =
      inotify_add_watch(ecore_main_fd_handler_fd_get(_fdh), path, mask);
   if (ECORE_FILE_MONITOR_INOTIFY(em)->wd < 0)
     {
        INF("inotify_add_watch failed, %s", strerror(errno));
        ecore_file_monitor_backend_del(em);
        return 0;
     }
   eina_hash_list_append(monitor_hash, &ECORE_FILE_MONITOR_INOTIFY(em)->wd, em);
   return 1;
}

#if 0
static void
_ecore_file_monitor_inotify_print(char *file, int mask)
{
   const char *type;

   if (mask & IN_ISDIR)
     type = "dir";
   else
     type = "file";

   if (mask & IN_ACCESS)
     INF("Inotify accessed %s: %s", type, file);
   if (mask & IN_MODIFY)
     INF("Inotify modified %s: %s", type, file);
   if (mask & IN_ATTRIB)
     INF("Inotify attributes %s: %s", type, file);
   if (mask & IN_CLOSE_WRITE)
     INF("Inotify close write %s: %s", type, file);
   if (mask & IN_CLOSE_NOWRITE)
     INF("Inotify close write %s: %s", type, file);
   if (mask & IN_OPEN)
     INF("Inotify open %s: %s", type, file);
   if (mask & IN_MOVED_FROM)
     INF("Inotify moved from %s: %s", type, file);
   if (mask & IN_MOVED_TO)
     INF("Inotify moved to %s: %s", type, file);
   if (mask & IN_DELETE)
     INF("Inotify delete %s: %s", type, file);
   if (mask & IN_CREATE)
     INF("Inotify create %s: %s", type, file);
   if (mask & IN_DELETE_SELF)
     INF("Inotify delete self %s: %s", type, file);
   if (mask & IN_MOVE_SELF)
     INF("Inotify move self %s: %s", type, file);
   if (mask & IN_UNMOUNT)
     INF("Inotify unmount %s: %s", type, file);
}
#endif
