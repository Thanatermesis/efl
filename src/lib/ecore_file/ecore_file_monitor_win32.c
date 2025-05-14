/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <process.h>

#include <evil_private.h> /* evil_wchar_to_char */

#include "ecore_file_private.h"

/**
 * @file ecore_file_monitor_win32.c
 * @brief Ecore file monitor Win32 backend implementation.
 */

typedef struct _Ecore_File_Monitor_Win32      Ecore_File_Monitor_Win32; /**< Alias for the main Win32 monitor structure */
typedef struct _Ecore_File_Monitor_Win32_Data Ecore_File_Monitor_Win32_Data; /**< Alias for the Win32 monitor data structure */

/* 4096 = 256 * sizeof(FILE_NOTIFY_INFORMATION) */
# define ECORE_FILE_MONITOR_WIN32_BUFFER_SIZE 4096 /**< Buffer size for ReadDirectoryChangesW */
# define ECORE_FILE_MONITOR_WIN32(x) ((Ecore_File_Monitor_Win32 *)(x)) /**< Macro to cast to Ecore_File_Monitor_Win32 */

/**
 * @brief Structure to hold data for a single monitored item (file or directory).
 */
struct _Ecore_File_Monitor_Win32_Data
{
   char                 buffer[ECORE_FILE_MONITOR_WIN32_BUFFER_SIZE]; /**< Buffer for ReadDirectoryChangesW */
   OVERLAPPED           overlapped; /**< OVERLAPPED structure for asynchronous I/O */
   HANDLE               handle; /**< Handle to the directory being monitored */
   HANDLE               event; /**< Event handle for asynchronous I/O completion */
   Ecore_File_Monitor  *monitor; /**< Pointer to the parent Ecore_File_Monitor */
   Ecore_Win32_Handler *h; /**< Ecore Win32 handler for the event */
   DWORD                buf_length; /**< Length of the data returned by ReadDirectoryChangesW */
   int                  is_dir; /**< Flag indicating if this monitors a directory (1) or file (0) changes within a directory */
};

/**
 * @brief Main structure for a Win32 file monitor.
 *
 * This structure extends Ecore_File_Monitor and holds separate data
 * for monitoring file-specific changes and directory-specific changes
 * within the monitored path.
 */
struct _Ecore_File_Monitor_Win32
{
   Ecore_File_Monitor             monitor; /**< Base Ecore_File_Monitor structure */
   Ecore_File_Monitor_Win32_Data *file; /**< Data for monitoring file changes */
   Ecore_File_Monitor_Win32_Data *dir; /**< Data for monitoring directory changes */
};

static Ecore_File_Monitor *_monitors = NULL; /**< Global list of active monitors */

static Eina_Bool _ecore_file_monitor_win32_cb(void *data, Ecore_Win32_Handler *wh);

/**
 * @brief Creates and initializes a new Ecore_File_Monitor_Win32_Data structure.
 *
 * This function sets up monitoring for either file changes or directory changes
 * within the path specified by the Ecore_File_Monitor.
 *
 * @param monitor The parent Ecore_File_Monitor.
 * @param type 0 for monitoring file name changes, 1 for directory name changes.
 * @return A pointer to the newly created Ecore_File_Monitor_Win32_Data, or NULL on failure.
 */
static Ecore_File_Monitor_Win32_Data *
_ecore_file_monitor_win32_data_new(Ecore_File_Monitor *monitor, int type)
{
   Ecore_File_Monitor_Win32_Data *md;
   DWORD                          filter;

   md = (Ecore_File_Monitor_Win32_Data *)calloc(1, sizeof(Ecore_File_Monitor_Win32_Data));
   if (!md) return NULL;

   md->handle = CreateFile(monitor->path,
                           FILE_LIST_DIRECTORY,
                           FILE_SHARE_READ |
                           FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS |
                           FILE_FLAG_OVERLAPPED,
                           NULL);
   if (md->handle == INVALID_HANDLE_VALUE)
     goto free_md;

   md->event = CreateEvent(NULL, FALSE, FALSE, NULL);
   if (!md->event)
     goto close_handle;

   ZeroMemory (&md->overlapped, sizeof(md->overlapped));
   md->overlapped.hEvent = md->event;

   filter = (type == 0) ? FILE_NOTIFY_CHANGE_FILE_NAME : FILE_NOTIFY_CHANGE_DIR_NAME;
   filter |=
     FILE_NOTIFY_CHANGE_ATTRIBUTES |
     FILE_NOTIFY_CHANGE_SIZE |
     FILE_NOTIFY_CHANGE_LAST_WRITE |
     FILE_NOTIFY_CHANGE_LAST_ACCESS |
     FILE_NOTIFY_CHANGE_CREATION |
     FILE_NOTIFY_CHANGE_SECURITY;

   if (!ReadDirectoryChangesW(md->handle,
                              md->buffer,
                              ECORE_FILE_MONITOR_WIN32_BUFFER_SIZE,
                              FALSE,
                              filter,
                              &md->buf_length,
                              &md->overlapped,
                              NULL))
     goto close_event;

   md->h = ecore_main_win32_handler_add(md->event,
                                        _ecore_file_monitor_win32_cb,
                                        md);
   if (!md->h)
     goto close_event;

   md->monitor = monitor;
   md->is_dir = type;

   return md;

 close_event:
   CloseHandle(md->event);
 close_handle:
   CloseHandle(md->handle);
 free_md:
   free(md);

   return NULL;
}

/**
 * @brief Frees an Ecore_File_Monitor_Win32_Data structure and its resources.
 *
 * @param md The Ecore_File_Monitor_Win32_Data structure to free.
 */
static void
_ecore_file_monitor_win32_data_free(Ecore_File_Monitor_Win32_Data *md)
{
   if (!md) return;

   CloseHandle(md->event);
   CloseHandle (md->handle);
   free (md);
}

/**
 * @brief Callback function triggered when a file system change is detected.
 *
 * This function is called by the Ecore main loop when the event associated
 * with a directory change is signaled. It processes the notification
 * information and triggers the user-defined callback.
 *
 * @param data Pointer to the Ecore_File_Monitor_Win32_Data structure.
 * @param wh The Ecore_Win32_Handler that triggered the callback (unused).
 * @return EINA_TRUE to keep the handler active, EINA_FALSE to remove it.
 */
static Eina_Bool
_ecore_file_monitor_win32_cb(void *data, Ecore_Win32_Handler *wh EINA_UNUSED)
{
   char                           filename[PATH_MAX];
   PFILE_NOTIFY_INFORMATION       fni;
   Ecore_File_Monitor_Win32_Data *md;
   wchar_t                       *wname;
   char                          *name;
   DWORD                          filter;
   DWORD                          offset;
   DWORD                          buf_length;
   Ecore_File_Event               event = ECORE_FILE_EVENT_NONE;

   md = (Ecore_File_Monitor_Win32_Data *)data;

   if (!GetOverlappedResult (md->handle, &md->overlapped, &buf_length, TRUE))
     return 1;

   fni = (PFILE_NOTIFY_INFORMATION)md->buffer;
   do {
      if (!fni)
        break;
      offset = fni->NextEntryOffset;

      wname = (wchar_t *)malloc(sizeof(wchar_t) * (fni->FileNameLength + 1));
      if (!wname)
        return 0;

      memcpy(wname, fni->FileName, fni->FileNameLength);
      wname[fni->FileNameLength]='\0';
      name = evil_wchar_to_char(wname);
      free(wname);
      if (!name)
        return 0;

      _snprintf(filename, PATH_MAX, "%s\\%s", md->monitor->path, name);
      free(name);

      switch (fni->Action)
        {
        case FILE_ACTION_ADDED:
          if (md->is_dir)
            event = ECORE_FILE_EVENT_CREATED_DIRECTORY;
          else
            event = ECORE_FILE_EVENT_CREATED_FILE;
          break;
        case FILE_ACTION_REMOVED:
          if (md->is_dir)
            event = ECORE_FILE_EVENT_DELETED_DIRECTORY;
          else
            event = ECORE_FILE_EVENT_DELETED_FILE;
          break;
        case FILE_ACTION_MODIFIED:
          if (!md->is_dir)
            event = ECORE_FILE_EVENT_MODIFIED;
          break;
        case FILE_ACTION_RENAMED_OLD_NAME:
          if (md->is_dir)
            event = ECORE_FILE_EVENT_DELETED_DIRECTORY;
          else
            event = ECORE_FILE_EVENT_DELETED_FILE;
          break;
        case FILE_ACTION_RENAMED_NEW_NAME:
          if (md->is_dir)
            event = ECORE_FILE_EVENT_CREATED_DIRECTORY;
          else
            event = ECORE_FILE_EVENT_CREATED_FILE;
          break;
        default:
          fprintf(stderr, "unknown event\n");
          event = ECORE_FILE_EVENT_NONE;
          break;
        }
      if (event != ECORE_FILE_EVENT_NONE)
        md->monitor->func(md->monitor->data, md->monitor, event, filename);

      fni = (PFILE_NOTIFY_INFORMATION)((LPBYTE)fni + offset);
   } while (offset);

   filter = (md->is_dir == 0) ? FILE_NOTIFY_CHANGE_FILE_NAME : FILE_NOTIFY_CHANGE_DIR_NAME;
   filter |=
     FILE_NOTIFY_CHANGE_ATTRIBUTES |
     FILE_NOTIFY_CHANGE_SIZE |
     FILE_NOTIFY_CHANGE_LAST_WRITE |
     FILE_NOTIFY_CHANGE_LAST_ACCESS |
     FILE_NOTIFY_CHANGE_CREATION |
     FILE_NOTIFY_CHANGE_SECURITY;

    ReadDirectoryChangesW(md->handle,
                          md->buffer,
                          ECORE_FILE_MONITOR_WIN32_BUFFER_SIZE,
                          FALSE,
                          filter,
                          &md->buf_length,
                          &md->overlapped,
                          NULL);
   return 1;
}

/**
 * @brief Initializes the Win32 file monitor backend.
 * @return 1 on success, 0 on failure.
 */
int
ecore_file_monitor_backend_init(void)
{
   return 1;
}

/**
 * @brief Shuts down the Win32 file monitor backend.
 * @return 1 on success, 0 on failure.
 */
int
ecore_file_monitor_backend_shutdown(void)
{
   return 1;
}

/**
 * @brief Adds a new file monitor for the given path using the Win32 backend.
 *
 * This function creates a new file monitor for the specified directory path.
 * It sets up two internal monitors: one for file changes (creation, deletion, modification)
 * and one for subdirectory changes (creation, deletion).
 *
 * @param path The directory path to monitor. Must be an existing directory.
 * @param func The callback function to execute when an event occurs.
 *             Example: `void my_callback(void *data, Ecore_File_Monitor *mon, Ecore_File_Event event, const char *path)`
 * @param data User-specific data to pass to the callback function.
 * @return A pointer to the newly created Ecore_File_Monitor, or NULL on failure.
 *         The returned monitor should be freed using ecore_file_monitor_del().
 */
Ecore_File_Monitor *
ecore_file_monitor_backend_add(const char *path,
                             void (*func) (void *data, Ecore_File_Monitor *em,
                                           Ecore_File_Event event,
                                           const char *path),
                             void *data)
{
   Ecore_File_Monitor_Win32 *m;
   Ecore_File_Monitor       *em;
   char                     *path2;
   size_t                    len;

   if (!path || (*path == '\0')) return NULL;
   if (!ecore_file_exists(path) || !ecore_file_is_dir(path))
     return NULL;
   if (!func) return NULL;

   em = (Ecore_File_Monitor *)calloc(1, sizeof(Ecore_File_Monitor_Win32));
   if (!em) return NULL;

   em->func = func;
   em->data = data;

   len = strlen(path);
   path2 = alloca(len + 1);
   strcpy(path2, path);
   if (path2[len - 1] == '/' || path2[len - 1] == '\\') path2[len - 1] = 0;
   em->path = eina_stringshare_add(path2);

   m = ECORE_FILE_MONITOR_WIN32(em);

   m->file = _ecore_file_monitor_win32_data_new(em, 0);
   if (!m->file)
     {
        eina_stringshare_del(em->path);
        free(em);
        return NULL;
     }

   m->dir = _ecore_file_monitor_win32_data_new(em, 1);
   if (!m->dir)
     {
        _ecore_file_monitor_win32_data_free(m->file);
        eina_stringshare_del(em->path);
        free(em);
        return NULL;
     }

   _monitors = ECORE_FILE_MONITOR(eina_inlist_append(EINA_INLIST_GET(_monitors), EINA_INLIST_GET(em)));

   return em;
}

/**
 * @brief Deletes a file monitor previously added with ecore_file_monitor_backend_add().
 *
 * @param em The Ecore_File_Monitor to delete.
 */
void
ecore_file_monitor_backend_del(Ecore_File_Monitor *em)
{
   Ecore_File_Monitor_Win32 *m;

   if (!em)
     return;

   m = ECORE_FILE_MONITOR_WIN32(em);
   _ecore_file_monitor_win32_data_free(m->dir);
   _ecore_file_monitor_win32_data_free(m->file);
   eina_stringshare_del(em->path);
   free(em);
}
