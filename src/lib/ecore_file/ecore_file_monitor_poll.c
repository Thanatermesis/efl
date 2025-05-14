#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ecore_file_private.h"

/**
 * @file
 * @brief This file implements the polling backend for Ecore_File_Monitor.
 *
 * This backend periodically checks files and directories for modifications,
 * creations, or deletions. It's a fallback mechanism when more efficient
 * OS-specific monitoring (like inotify) is not available or not working.
 */

/*
 * TODO:
 * - Implement recursive as an option!
 * - Keep whole path or just name of file? (Memory or CPU...)
 * - Remove requests without files?
 * - Change poll time
 */

typedef struct _Ecore_File_Monitor_Poll Ecore_File_Monitor_Poll;

#define ECORE_FILE_MONITOR_POLL(x) ((Ecore_File_Monitor_Poll *)(x))

/**
 * @brief Structure extending Ecore_File_Monitor for polling-specific data.
 */
struct _Ecore_File_Monitor_Poll
{
   Ecore_File_Monitor  monitor; /**< Base monitor structure. */
   int                 mtime;   /**< Last known modification time of the monitored path. */
   unsigned char       deleted; /**< Flag indicating if the monitor is marked for deletion. */
};

#define ECORE_FILE_INTERVAL_MIN  1.0
#define ECORE_FILE_INTERVAL_STEP 0.5
#define ECORE_FILE_INTERVAL_MAX  5.0

static double         _interval = ECORE_FILE_INTERVAL_MIN; /**< Current polling interval. */
static Ecore_Timer   *_timer = NULL; /**< Timer for polling. */
static Ecore_File_Monitor *_monitors = NULL; /**< List of active monitors. Uses EINA_INLIST. */
static int          _lock = 0; /**< Lock to prevent modification of the monitors list while iterating. */

static Eina_Bool   _ecore_file_monitor_poll_handler(void *data);
static void        _ecore_file_monitor_poll_check(Ecore_File_Monitor *em);
static int         _ecore_file_monitor_poll_checking(Ecore_File_Monitor *em, char *name);

/**
 * @brief Initializes the polling monitor backend.
 * @return 1 on success, 0 on failure.
 */
int
ecore_file_monitor_backend_init(void)
{
   return 1;
}

/**
 * @brief Shuts down the polling monitor backend.
 * Cleans up all active monitors and the polling timer.
 * @return 1 on success.
 */
int
ecore_file_monitor_backend_shutdown(void)
{
   while(_monitors)
        ecore_file_monitor_backend_del(_monitors);

   if (_timer)
     {
        ecore_timer_del(_timer);
        _timer = NULL;
     }
   return 1;
}

/**
 * @brief Adds a new path to monitor using the polling backend.
 * @param path The file or directory path to monitor.
 * @param func The callback function to execute when an event occurs.
 *             Example: void my_callback(void *data, Ecore_File_Monitor *mon,
 *                                       Ecore_File_Event event, const char *path) {}
 * @param data User data to pass to the callback function.
 * @return A new Ecore_File_Monitor instance, or NULL on failure.
 *         The returned monitor structure contains an EINA_INLIST of Ecore_File elements
 *         if the monitored path is a directory. Each Ecore_File represents a file
 *         or subdirectory within the monitored directory and has the following structure:
 *         - name: (char *) Name of the file/subdirectory.
 *         - mtime: (int) Last modification time.
 *         - is_dir: (Eina_Bool) True if it's a directory.
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

   if (!path) return NULL;
   if (!func) return NULL;

   em = calloc(1, sizeof(Ecore_File_Monitor_Poll));
   if (!em) return NULL;

   if (!_timer)
     _timer = ecore_timer_add(_interval, _ecore_file_monitor_poll_handler, NULL);
   else
     ecore_timer_interval_set(_timer, ECORE_FILE_INTERVAL_MIN);

   em->func = func;
   em->data = data;

   len = strlen(path);
   path2 = alloca(len + 1);
   strcpy(path2, path);
   if (path2[len - 1] == '/' && strcmp(path2, "/")) path2[len - 1] = 0;
   em->path = eina_stringshare_add(path2);

   ECORE_FILE_MONITOR_POLL(em)->mtime = ecore_file_mod_time(em->path);
   _monitors = ECORE_FILE_MONITOR(eina_inlist_append(EINA_INLIST_GET(_monitors), EINA_INLIST_GET(em)));

   if (ecore_file_exists(em->path))
     {
        if (ecore_file_is_dir(em->path))
          {
             /* Check for subdirs */
             Eina_List *files;
             char *file;

             files = ecore_file_ls(em->path);
             EINA_LIST_FREE(files, file)
               {
                  Ecore_File *f;
                  char buf[PATH_MAX];

                  f = calloc(1, sizeof(Ecore_File));
                  if (!f)
                    {
                       free(file);
                       continue;
                    }

                  snprintf(buf, sizeof(buf), "%s/%s", em->path, file);
                  f->name = file;
                  f->mtime = ecore_file_mod_time(buf);
                  f->is_dir = ecore_file_is_dir(buf);
                  em->files =
                    (Ecore_File *) eina_inlist_append(EINA_INLIST_GET(em->files),
                                                      EINA_INLIST_GET(f));
               }
          }
     }
   else
     {
        ecore_file_monitor_backend_del(em);
        return NULL;
     }

   return em;
}

/**
 * @brief Deletes a monitor from the polling backend.
 * @param em The monitor to delete.
 */
void
ecore_file_monitor_backend_del(Ecore_File_Monitor *em)
{
   Ecore_File *l;

   if (_lock)
     {
        ECORE_FILE_MONITOR_POLL(em)->deleted = 1;
        return;
     }

   /* Remove files */
   /*It's possible there weren't any files to monitor, so check if the list is init*/
   if (em->files)
     {
        for (l = em->files; l;)
          {
             Ecore_File *file = l;

             l = (Ecore_File *) EINA_INLIST_GET(l)->next;
             free(file->name);
             free(file);
          }
     }

   if (_monitors)
     _monitors = ECORE_FILE_MONITOR(eina_inlist_remove(EINA_INLIST_GET(_monitors), EINA_INLIST_GET(em)));

   eina_stringshare_del(em->path);
   free(em);

   if (_timer)
     {
        if (!_monitors)
          {
             ecore_timer_del(_timer);
             _timer = NULL;
          }
        else
          ecore_timer_interval_set(_timer, ECORE_FILE_INTERVAL_MIN);
     }
}

/**
 * @brief Timer callback function that triggers polling checks.
 * This function iterates over all registered monitors and checks for changes.
 * It also handles dynamic adjustment of the polling interval.
 * @param data Unused.
 * @return ECORE_CALLBACK_RENEW to keep the timer active.
 */
static Eina_Bool
_ecore_file_monitor_poll_handler(void *data EINA_UNUSED)
{
   Ecore_File_Monitor *l;

   _interval += ECORE_FILE_INTERVAL_STEP;

   _lock = 1;
   EINA_INLIST_FOREACH(_monitors, l)
        _ecore_file_monitor_poll_check(l);
   _lock = 0;

   if (_interval > ECORE_FILE_INTERVAL_MAX)
     _interval = ECORE_FILE_INTERVAL_MAX;
   ecore_timer_interval_set(_timer, _interval);

   for (l = _monitors; l;)
     {
        Ecore_File_Monitor *em = l;

        l = ECORE_FILE_MONITOR(EINA_INLIST_GET(l)->next);
        if (ECORE_FILE_MONITOR_POLL(em)->deleted)
          ecore_file_monitor_del(em);
     }
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Checks a single monitor for file system events.
 * This function compares the current state of the monitored path and its contents
 * (if it's a directory) with the last known state. It triggers appropriate
 * event callbacks for creations, deletions, and modifications.
 * @param em The monitor to check.
 */
static void
_ecore_file_monitor_poll_check(Ecore_File_Monitor *em)
{
   int mtime;

   mtime = ecore_file_mod_time(em->path);
   if (mtime < ECORE_FILE_MONITOR_POLL(em)->mtime)
     {
        Ecore_File *l;
        Ecore_File_Event event;

        /* Notify all files deleted */
        for (l = em->files; l;)
          {
             Ecore_File *f = l;
             char buf[PATH_MAX];

             l = (Ecore_File *) EINA_INLIST_GET(l)->next;

             snprintf(buf, sizeof(buf), "%s/%s", em->path, f->name);
             if (f->is_dir)
               event = ECORE_FILE_EVENT_DELETED_DIRECTORY;
             else
               event = ECORE_FILE_EVENT_DELETED_FILE;
             em->func(em->data, em, event, buf);
             free(f->name);
             free(f);
          }
        em->files = NULL;
        em->func(em->data, em, ECORE_FILE_EVENT_DELETED_SELF, em->path);
        _interval = ECORE_FILE_INTERVAL_MIN;
     }
   else
     {
        Ecore_File *l;

        /* Check for changed files */
        for (l = em->files; l;)
          {
             Ecore_File *f = l;
             char buf[PATH_MAX];
             int mt;
             Ecore_File_Event event;

             l = (Ecore_File *) EINA_INLIST_GET(l)->next;

             snprintf(buf, sizeof(buf), "%s/%s", em->path, f->name);
             mt = ecore_file_mod_time(buf);
             if (mt < f->mtime)
               {
                  if (f->is_dir)
                    event = ECORE_FILE_EVENT_DELETED_DIRECTORY;
                  else
                    event = ECORE_FILE_EVENT_DELETED_FILE;

                  em->func(em->data, em, event, buf);
                  em->files = (Ecore_File *) eina_inlist_remove(EINA_INLIST_GET(em->files), EINA_INLIST_GET(f));
                  free(f->name);
                  free(f);
                  _interval = ECORE_FILE_INTERVAL_MIN;
               }
             else if ((mt > f->mtime) && !(f->is_dir))
               {
                  em->func(em->data, em, ECORE_FILE_EVENT_MODIFIED, buf);
                  _interval = ECORE_FILE_INTERVAL_MIN;
                  f->mtime = mt;
               }
             else
               f->mtime = mt;
          }

        /* Check for new files */
        if (ECORE_FILE_MONITOR_POLL(em)->mtime < mtime)
          {
             Eina_List *files;
             Eina_List *fl;
             char *file;

             /* Files have been added or removed */
             files = ecore_file_ls(em->path);
             if (files)
               {
                  /* Are we a directory? We should check first, rather than rely on null here*/
                  EINA_LIST_FOREACH(files, fl, file)
                    {
                       Ecore_File *f;
                       char buf[PATH_MAX];
                       Ecore_File_Event event;

                       if (_ecore_file_monitor_poll_checking(em, file))
                         continue;

                       snprintf(buf, sizeof(buf), "%s/%s", em->path, file);
                       f = calloc(1, sizeof(Ecore_File));
                       if (!f)
                         continue;

                       f->name = strdup(file);
                       f->mtime = ecore_file_mod_time(buf);
                       f->is_dir = ecore_file_is_dir(buf);
                       if (f->is_dir)
                         event = ECORE_FILE_EVENT_CREATED_DIRECTORY;
                       else
                         event = ECORE_FILE_EVENT_CREATED_FILE;
                       em->func(em->data, em, event, buf);
                       em->files = (Ecore_File *) eina_inlist_append(EINA_INLIST_GET(em->files), EINA_INLIST_GET(f));
                    }
                  while (files)
                    {
                       file = eina_list_data_get(files);
                       free(file);
                       files = eina_list_remove_list(files, files);
                    }
               }

             if (!ecore_file_is_dir(em->path))
               em->func(em->data, em, ECORE_FILE_EVENT_MODIFIED, em->path);
             _interval = ECORE_FILE_INTERVAL_MIN;
          }
     }
   ECORE_FILE_MONITOR_POLL(em)->mtime = mtime;
}

/**
 * @brief Checks if a file/directory name already exists in the monitor's list of files.
 * This is used to determine if a file found during a directory scan is new or existing.
 * @param em The monitor whose file list is to be checked.
 * @param name The name of the file or directory to check for.
 * @return 1 if the name exists in the list, 0 otherwise.
 */
static int
_ecore_file_monitor_poll_checking(Ecore_File_Monitor *em, char *name)
{
   Ecore_File *l;

   EINA_INLIST_FOREACH(em->files, l)
     {
        if (!strcmp(l->name, name))
          return 1;
     }
   return 0;
}
