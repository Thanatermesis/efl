/**
 * @file
 * @brief Implementation of Efreetd caching mechanisms.
 *
 * This file contains the core logic for managing caches of desktop files,
 * icons, and MIME types. It handles file system monitoring, cache generation
 * via external helper programs, and inter-process communication for cache
 * update notifications. It also implements a persistent cache for subdirectory
 * listings to speed up recursive monitoring.
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_File.h>
#include <Eio.h>
#include <Eet.h>
#include "efreetd.h"
#include "efreetd_ipc.h"

#include "Efreet.h"
#define EFREET_MODULE_LOG_DOM efreetd_log_dom
#include "efreet_private.h"
#include "efreetd_cache.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern FILE *efreetd_log_file;

/* Hash table mapping directory paths to Eio_Monitor objects for icon directories. */
static Eina_Hash *icon_change_monitors = NULL;
/* Hash table mapping Eio_Monitor pointers to themselves, for quick lookup from events. */
static Eina_Hash *icon_change_monitors_mon = NULL;
/* Hash table mapping directory paths to Eio_Monitor objects for desktop directories. */
static Eina_Hash *desktop_change_monitors = NULL;
/* Hash table mapping Eio_Monitor pointers to themselves, for quick lookup from events. */
static Eina_Hash *desktop_change_monitors_mon = NULL;

/* Event handler for Ecore_Exe deletion events. */
static Ecore_Event_Handler *cache_exe_del_handler = NULL;
/* Event handler for Ecore_Exe data events. */
static Ecore_Event_Handler *cache_exe_data_handler = NULL;
/* Ecore_Exe process for icon cache generation. */
static Ecore_Exe           *icon_cache_exe = NULL;
/* Ecore_Exe process for desktop cache generation. */
static Ecore_Exe           *desktop_cache_exe = NULL;
/* Timer for debouncing icon cache updates. */
static Ecore_Timer         *icon_cache_timer = NULL;
/* Timer for debouncing desktop cache updates. */
static Ecore_Timer         *desktop_cache_timer = NULL;
/* Efreet prefix utility for finding helper executables. */
static Eina_Prefix         *pfx = NULL;

/* Flag indicating if the desktop cache has been successfully built at least once. */
static Eina_Bool  desktop_exists = EINA_FALSE;

/* List of system-defined desktop directories. (e.g., XDG_DATA_DIRS/applications) */
static Eina_List *desktop_system_dirs = NULL;
/* List of user-added or dynamically discovered desktop directories. */
static Eina_List *desktop_extra_dirs = NULL;
/* List of user-added or dynamically discovered icon directories. */
static Eina_List *icon_extra_dirs = NULL;
/* List of icon extensions to monitor (e.g., "png", "svg"). */
static Eina_List *icon_exts = NULL;
/* Flag to indicate if the icon cache needs a full flush. */
static Eina_Bool  icon_flush = EINA_FALSE;

/* Flag to indicate if a desktop cache update is queued due to an ongoing update. */
static Eina_Bool desktop_queue = EINA_FALSE;
/* Flag to indicate if an icon cache update is queued due to an ongoing update. */
static Eina_Bool icon_queue = EINA_FALSE;

/* List of Ecore_Event_Handler pointers for file system monitor events. */
static Eina_List *_handlers = NULL;

static void icon_changes_listen(void);
static void desktop_changes_listen(void);

/* internal */
/**
 * @brief Represents the cache of subdirectory listings.
 * This is used to avoid repeated readdir/stat calls when monitoring
 * directory trees.
 */
typedef struct _Subdir_Cache Subdir_Cache;
/**
 * @brief Represents a cached directory's metadata and its subdirectories.
 */
typedef struct _Subdir_Cache_Dir Subdir_Cache_Dir;

struct _Subdir_Cache
{
   Eina_Hash *dirs; /**< Hash table mapping directory paths (const char *) to Subdir_Cache_Dir objects. */
};

struct _Subdir_Cache_Dir
{
   unsigned long long dev;    /**< Device ID of the directory. */
   unsigned long long ino;    /**< Inode number of the directory. */
   unsigned long long mode;   /**< File mode (type and permissions). */
   unsigned long long uid;    /**< User ID of owner. */
   unsigned long long gid;    /**< Group ID of owner. */
   unsigned long long size;   /**< Total size, in bytes. */
   unsigned long long mtim;   /**< Time of last modification. */
   unsigned long long ctim;   /**< Time of last status change. */
   const char **dirs;         /**< Array of stringshared subdirectory names. Example: {"subdir1", "subdir2", NULL} */
   unsigned int dirs_count;   /**< Number of entries in the dirs array. */
};

/* Eet data descriptor for Subdir_Cache. */
static Eet_Data_Descriptor *subdir_edd = NULL;
/* Eet data descriptor for Subdir_Cache_Dir. */
static Eet_Data_Descriptor *subdir_dir_edd = NULL;
/* The global instance of the subdirectory cache. */
static Subdir_Cache        *subdir_cache = NULL;
/* Flag indicating if the subdir_cache needs to be saved to disk. */
static Eina_Bool            subdir_need_save = EINA_FALSE;

/* Hash table mapping directory paths to Eio_Monitor objects for MIME directories. */
static Eina_Hash *mime_monitors = NULL;
/* Hash table mapping Eio_Monitor pointers to themselves, for quick MIME lookup from events. */
static Eina_Hash *mime_monitors_mon = NULL;
/* Timer for debouncing MIME cache updates. */
static Ecore_Timer *mime_update_timer = NULL;
/* Ecore_Exe process for MIME cache generation. */
static Ecore_Exe *mime_cache_exe = NULL;

static void mime_cache_init(void);
static void mime_cache_shutdown(void);
static Eina_Bool mime_update_cache_cb(void *data EINA_UNUSED);

/**
 * @brief Frees a Subdir_Cache_Dir structure.
 * This function is suitable for use as an Eina_Free_Cb.
 * @param cd The Subdir_Cache_Dir to free.
 */
static void
subdir_cache_dir_free(Subdir_Cache_Dir *cd)
{
   unsigned int i;
   if (!cd) return;
   if (cd->dirs)
     {
        for (i = 0; i < cd->dirs_count; i++)
          eina_stringshare_del(cd->dirs[i]);
        free(cd->dirs);
     }
   free(cd);
}

/**
 * @brief Adds an entry to a hash table, creating the hash if it doesn't exist.
 * This is a helper function for Eet to populate the Subdir_Cache's hash table.
 * @param hash The hash table (may be NULL).
 * @param key The key for the new entry.
 * @param data The data for the new entry.
 * @return The hash table, or NULL on failure.
 */
static void *
subdir_cache_hash_add(void *hash, const char *key, void *data)
{
   if (!hash) hash = eina_hash_string_superfast_new(EINA_FREE_CB(subdir_cache_dir_free));
   if (!hash) return NULL;
   eina_hash_add(hash, key, data);
   return hash;
}

/**
 * @brief Initializes the subdirectory cache.
 * Sets up Eet data descriptors for Subdir_Cache and Subdir_Cache_Dir,
 * and loads the cache from a file (e.g., ~/.cache/efreet/subdirs_$(hostname).eet).
 * If the cache file doesn't exist or is invalid, an empty cache is created.
 */
static void
subdir_cache_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   Eet_File *ef;
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return;

   // set up data codecs for subdirs in memory
   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(Subdir_Cache_Dir), "D", sizeof(Subdir_Cache_Dir));
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Subdir_Cache_Dir);
   subdir_dir_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "0", dev, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "1", ino, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "2", mode, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "3", uid, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "4", gid, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "5", size, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "6", mtim, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_BASIC(subdir_dir_edd, Subdir_Cache_Dir, "7", ctim, EET_T_ULONG_LONG);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY_STRING(subdir_dir_edd, Subdir_Cache_Dir, "d", dirs);

   eet_eina_stream_data_descriptor_class_set(&eddc, sizeof(Subdir_Cache), "C", sizeof(Subdir_Cache));
   eddc.func.hash_add = subdir_cache_hash_add;
   subdir_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_HASH(subdir_edd, Subdir_Cache, "dirs", dirs, subdir_dir_edd);

   // load subdirs from the cache file
   eina_strbuf_append_printf(buf, "%s/efreet/subdirs_%s.eet",
                             efreet_cache_home_get(), efreet_hostname_get());
   ef = eet_open(eina_strbuf_string_get(buf), EET_FILE_MODE_READ);
   if (ef)
     {
        subdir_cache = eet_data_read(ef, subdir_edd, "subdirs");
        eet_close(ef);
     }
   eina_strbuf_free(buf);

   // if we don't have a decoded subdir cache - allocate one
   if (!subdir_cache) subdir_cache = calloc(1, sizeof(Subdir_Cache));
   if (!subdir_cache)
     {
        ERR("Cannot allocate subdir cache in memory");
        return;
     }

   // if we don't have a hash in the subdir cache - allocate it
   if (!subdir_cache->dirs)
     subdir_cache->dirs = eina_hash_string_superfast_new(EINA_FREE_CB(subdir_cache_dir_free));
}

/**
 * @brief Shuts down the subdirectory cache.
 * Frees the in-memory cache data and Eet data descriptors.
 * Note: This does not save the cache; subdir_cache_save() should be called
 * if changes need to be persisted.
 */
static void
subdir_cache_shutdown(void)
{
   // free up in-memory subdir scan info - don't need it anymore
   if (subdir_cache)
     {
        if (subdir_cache->dirs) eina_hash_free(subdir_cache->dirs);
        free(subdir_cache);
     }
   eet_data_descriptor_free(subdir_dir_edd);
   eet_data_descriptor_free(subdir_edd);
   subdir_cache = NULL;
   subdir_dir_edd = NULL;
   subdir_edd = NULL;
}

/**
 * @brief Saves the subdirectory cache to a persistent file.
 * The cache is saved to a temporary file first, then atomically renamed
 * to the final cache file (e.g., ~/.cache/efreet/subdirs_$(hostname).eet).
 * This function only saves if `subdir_need_save` is EINA_TRUE.
 */
static void
subdir_cache_save(void)
{
   Eina_Strbuf *buf;
   Eet_File *ef;
   Eina_Tmpstr *tmpstr = NULL;
   int tmpfd;

   // only if subdirs need saving... and we have subdirs.
   if (!subdir_need_save) return;
   if (!subdir_cache) return;
   if (!subdir_cache->dirs) return;

   buf = eina_strbuf_new();
   if (!buf) return;

   // save to tmp file first
   eina_strbuf_append_printf(buf, "%s/efreet/subdirs_%s.eet.XXXXXX.cache",
                             efreet_cache_home_get(), efreet_hostname_get());

   tmpfd = eina_file_mkstemp(eina_strbuf_string_get(buf), &tmpstr);
   if (tmpfd < 0)
     {
        eina_strbuf_free(buf);
        return;
     }

   eina_strbuf_reset(buf);

   // write out eet file to tmp file
   ef = eet_open(tmpstr, EET_FILE_MODE_WRITE);
   eet_data_write(ef, subdir_edd, "subdirs", subdir_cache, EET_COMPRESSION_SUPERFAST);
   eet_close(ef);

   /*
    * On Windows, buf2 has one remaining ref, hence it can not be renamed below.
    * Stupid NTFS... So we close it first. "Magically", on Windows, this
    * temporary file is not deleted...
    */
#ifdef _WIN32
   close(tmpfd);
#endif

   // atomically rename subdirs file on top from tmp file
   eina_strbuf_append_printf(buf, "%s/efreet/subdirs_%s.eet",
                             efreet_cache_home_get(), efreet_hostname_get());

   if (rename(tmpstr, eina_strbuf_string_get(buf)) < 0)
     {
        unlink(tmpstr);
        ERR("Can't save subdir cache %s", eina_strbuf_string_get(buf));
     }
   // we dont need saving anymore - we just did
   subdir_need_save = EINA_FALSE;
   eina_tmpstr_del(tmpstr);
   eina_strbuf_free(buf);
}

/**
 * @brief Retrieves or creates a cache entry for a given directory path.
 * If a valid cache entry exists for the path and its stat information matches,
 * it is returned. Otherwise, the directory is scanned, a new cache entry is
 * created and stored, and `subdir_need_save` is set to EINA_TRUE.
 * @param st The stat structure of the directory.
 * @param path The full path to the directory.
 * @return A pointer to the Subdir_Cache_Dir entry, or NULL on failure.
 *         The returned pointer is valid until the cache is modified or shut down.
 */
static const Subdir_Cache_Dir *
subdir_cache_get(const struct stat *st, const char *path)
{
   Eina_Iterator *it;
   Eina_File_Direct_Info *info;
   Subdir_Cache_Dir *cd;
   Eina_List *files = NULL;
   int i = 0;
   const char *file;

   // if no subdir cache at all - return null
   if (!subdir_cache) return NULL;
   if (!subdir_cache->dirs) return NULL;

   // if found but something invalid in stored stat info...
   cd = eina_hash_find(subdir_cache->dirs, path);
   if ((cd) &&
       ((cd->dev != (unsigned long long)st->st_dev) ||
        (cd->ino != (unsigned long long)st->st_ino) ||
        (cd->mode != (unsigned long long)st->st_mode) ||
        (cd->uid != (unsigned long long)st->st_uid) ||
        (cd->gid != (unsigned long long)st->st_gid) ||
        (cd->size != (unsigned long long)st->st_size) ||
        (cd->mtim != (unsigned long long)st->st_mtime) ||
        (cd->ctim != (unsigned long long)st->st_ctime)))
     {
        // delete old node and prepare to scan a new one
        eina_hash_del(subdir_cache->dirs, path, cd);
        cd = NULL;
     }
   // if cached dir is ok by now - return it
   if (cd) return cd;

   // we need a new node (fesh or invalid)
   cd = calloc(1, sizeof(Subdir_Cache_Dir));
   if (!cd) return NULL;

   // store stat info
   cd->dev = (unsigned long long)st->st_dev;
   cd->ino = (unsigned long long)st->st_ino;
   cd->mode = (unsigned long long)st->st_mode;
   cd->uid = (unsigned long long)st->st_uid;
   cd->gid = (unsigned long long)st->st_gid;
   cd->size = (unsigned long long)st->st_size;
   cd->mtim = (unsigned long long)st->st_mtime;
   cd->ctim = (unsigned long long)st->st_ctime;

   // go through content finding directories
   it = eina_file_stat_ls(path);
   if (!it) return cd;
   
   EINA_ITERATOR_FOREACH(it, info)
     {
        // if ., .. or other "hidden" dot files - ignore
        if (info->path[info->name_start] == '.') continue;
        // if it's a dir or link to a dir - store it.
        if (((info->type == EINA_FILE_LNK) && (ecore_file_is_dir(info->path))) ||
            (info->type == EINA_FILE_DIR))
          {
             // store just the name, not the full path
             files = eina_list_append
               (files, eina_stringshare_add(info->path + info->name_start));
          }
     }
   eina_iterator_free(it);
   
   // now convert our temporary list into an array of stringshare strings
   cd->dirs_count = eina_list_count(files);
   if (cd->dirs_count > 0)
     {
        cd->dirs = malloc(cd->dirs_count * sizeof(char *));
        EINA_LIST_FREE(files, file)
          {
             cd->dirs[i] = file;
             i++;
          }
     }
   // add cache dir to hash with full path as key
   eina_hash_add(subdir_cache->dirs, path, cd);
   // mark subdirs as needing a save - something changed
   subdir_need_save = EINA_TRUE;
   return cd;
}

/**
 * @brief Ecore_Timer callback to trigger an icon cache update.
 * This function is called after a short delay to debounce multiple
 * rapid requests for icon cache updates. It constructs and executes
 * the `efreet_icon_cache_create` helper program.
 * If an update is already in progress, it queues the request.
 * @param data Unused.
 * @return ECORE_CALLBACK_CANCEL to remove the timer.
 */
static Eina_Bool
icon_cache_update_cache_cb(void *data EINA_UNUSED)
{
   Eina_Strbuf *file = eina_strbuf_new();
   if (!file) return EINA_FALSE;

   icon_cache_timer = NULL;

   if (icon_cache_exe)
     {
        icon_queue = EINA_TRUE;
        eina_strbuf_free(file);
        return ECORE_CALLBACK_CANCEL;
     }
   icon_queue = EINA_FALSE;
   if ((!icon_flush) && (!icon_exts))
     {
        eina_strbuf_free(file);
        return ECORE_CALLBACK_CANCEL;
     }

   if (icon_change_monitors) eina_hash_free(icon_change_monitors);
   if (icon_change_monitors_mon) eina_hash_free(icon_change_monitors_mon);
   icon_change_monitors = eina_hash_string_superfast_new
     (EINA_FREE_CB(eio_monitor_del));
   icon_change_monitors_mon = eina_hash_pointer_new(NULL);
   icon_changes_listen();
   subdir_cache_save();

   /* TODO: Queue if already running */
   eina_strbuf_append_printf(file, "%s/efreet/" MODULE_ARCH "/efreet_icon_cache_create",
                             eina_prefix_lib_get(pfx));
   if (icon_extra_dirs)
     {
        Eina_List *ll;
        char *p;

        eina_strbuf_append(file, " -d");
        EINA_LIST_FOREACH(icon_extra_dirs, ll, p)
          {
             eina_strbuf_append(file, " ");
             eina_strbuf_append(file, p);
          }
     }
   if (icon_exts)
     {
        Eina_List *ll;
        char *p;

        eina_strbuf_append(file, " -e");
        EINA_LIST_FOREACH(icon_exts, ll, p)
          {
             eina_strbuf_append(file, " ");
             eina_strbuf_append(file, p);
          }
     }
   if (icon_flush)
     eina_strbuf_append(file, " -f");
   icon_flush = EINA_FALSE;
   fprintf(efreetd_log_file, "[%09.3f] Run:\n  %s\n", ecore_time_get(),
           eina_strbuf_string_get(file));
   fflush(efreetd_log_file);
   icon_cache_exe = ecore_exe_pipe_run
     (eina_strbuf_string_get(file),
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED,
      NULL);

   eina_strbuf_free(file);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Ecore_Timer callback to trigger a desktop cache update.
 * This function is called after a short delay to debounce multiple
 * rapid requests for desktop cache updates. It constructs and executes
 * the `efreet_desktop_cache_create` helper program.
 * If an update is already in progress, it queues the request.
 * @param data Unused.
 * @return ECORE_CALLBACK_CANCEL to remove the timer.
 */
static Eina_Bool
desktop_cache_update_cache_cb(void *data EINA_UNUSED)
{
   Eina_Strbuf *file;

   desktop_cache_timer = NULL;

   if (desktop_cache_exe)
     {
        desktop_queue = EINA_TRUE;
        return ECORE_CALLBACK_CANCEL;
     }
   desktop_queue = EINA_FALSE;
   file = eina_strbuf_new();

   if (desktop_change_monitors) eina_hash_free(desktop_change_monitors);
   if (desktop_change_monitors_mon) eina_hash_free(desktop_change_monitors_mon);
   desktop_change_monitors = eina_hash_string_superfast_new
     (EINA_FREE_CB(eio_monitor_del));
   desktop_change_monitors_mon = eina_hash_pointer_new(NULL);
   desktop_changes_listen();
   subdir_cache_save();

   eina_strbuf_append_printf(file, "%s/efreet/" MODULE_ARCH "/efreet_desktop_cache_create",
                            eina_prefix_lib_get(pfx));
   if (desktop_extra_dirs)
     {
        Eina_List *ll;
        const char *str;

        eina_strbuf_append(file, " -d");
        EINA_LIST_FOREACH(desktop_extra_dirs, ll, str)
          {
             eina_strbuf_append(file, " ");
             eina_strbuf_append(file, str);
          }
     }
   INF("Run desktop cache creation: %s", eina_strbuf_string_get(file));
   fprintf(efreetd_log_file, "[%09.3f] Run:\n  %s\n", ecore_time_get(),
           eina_strbuf_string_get(file));
   fflush(efreetd_log_file);
   desktop_cache_exe = ecore_exe_pipe_run
     (eina_strbuf_string_get(file),
      ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_READ_LINE_BUFFERED,
      NULL);

   eina_strbuf_free(file);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Schedules an update for the icon cache.
 * This function will trigger a rebuild of the icon cache after a short delay.
 * @param flush If EINA_TRUE, the cache generator will be instructed to perform a full flush.
 */
static void
cache_icon_update(Eina_Bool flush)
{
   if (icon_cache_timer) ecore_timer_del(icon_cache_timer);
   if (flush) icon_flush = flush;
   icon_cache_timer = ecore_timer_add(0.2, icon_cache_update_cache_cb, NULL);
}

void
cache_desktop_update(void)
{
   if (desktop_cache_timer) ecore_timer_del(desktop_cache_timer);
   desktop_cache_timer = ecore_timer_add(0.2, desktop_cache_update_cache_cb, NULL);
}

/**
 * @brief Ecore_Event_Handler callback for EIO_MONITOR events.
 * This function is triggered by file system changes in monitored directories.
 * It determines if the change affects icons, desktops, or MIME types and
 * schedules the appropriate cache update.
 * @param data Unused.
 * @param type The type of the event (unused).
 * @param event The Eio_Monitor_Event structure.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_cb_monitor_event(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Eio_Monitor_Event *ev = event;

   // if it's an icon
   if (eina_hash_find(icon_change_monitors_mon, &(ev->monitor)))
     {
        cache_icon_update(EINA_FALSE);
     }
   // if it's a desktop
   else if (eina_hash_find(desktop_change_monitors_mon, &(ev->monitor)))
     {
        cache_desktop_update();
     }
   // if it's a mime file
   else if (eina_hash_find(mime_monitors_mon, &(ev->monitor)))
     {
        if ((!strcmp("/etc/mime.types", ev->filename)) ||
            (!strcmp("globs", ecore_file_file_get(ev->filename))))
          {
             mime_cache_shutdown();
             mime_cache_init();
             if (mime_update_timer) ecore_timer_del(mime_update_timer);
             mime_update_timer = ecore_timer_add(0.2, mime_update_cache_cb, NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Adds a directory to the icon monitoring system.
 * If the path is a directory and not already monitored, an Eio_Monitor
 * is created for it. Handles symbolic links by monitoring their real path.
 * @param st The stat structure of the path.
 * @param path The directory path to monitor.
 */
static void
icon_changes_monitor_add(const struct stat *st, const char *path)
{
   Eio_Monitor *mon;
   char *realp = NULL;
   const char *monpath = path;

   if (eina_hash_find(icon_change_monitors, path)) return;
#ifndef _WIN32
   if (S_ISLNK(st->st_mode))
     {
        realp = ecore_file_realpath(path);
        if (!realp) return;
        monpath = realp;
     }
#endif
   if (ecore_file_is_dir(monpath))
     {
        mon = eio_monitor_add(monpath);
        if (mon)
          {
             eina_hash_add(icon_change_monitors, path, mon);
             eina_hash_add(icon_change_monitors_mon, &mon, mon);
          }
     }
   free(realp);
}

/**
 * @brief Adds a directory to the desktop monitoring system.
 * If the path is a directory and not already monitored, an Eio_Monitor
 * is created for it. Handles symbolic links by monitoring their real path.
 * @param st The stat structure of the path.
 * @param path The directory path to monitor.
 */
static void
desktop_changes_monitor_add(const struct stat *st, const char *path)
{
   Eio_Monitor *mon;
   char *realp = NULL;
   const char *monpath = path;

   if (eina_hash_find(desktop_change_monitors, path)) return;
#ifndef _WIN32
   if (S_ISLNK(st->st_mode))
     {
        realp = ecore_file_realpath(path);
        if (!realp) return;
        monpath = realp;
     }
#endif
   if (ecore_file_is_dir(monpath))
     {
        mon = eio_monitor_add(monpath);
        if (mon)
          {
             eina_hash_add(desktop_change_monitors, path, mon);
             eina_hash_add(desktop_change_monitors_mon, &mon, mon);
          }
     }
   free(realp);
}

/**
 * @brief Comparison function for struct stat, used for Eina_Inarray searches.
 * Compares two stat structures based on their device ID (st_dev) and
 * inode number (st_ino).
 * @param a Pointer to the first struct stat.
 * @param b Pointer to the second struct stat.
 * @return 0 if they refer to the same file system object, 1 otherwise.
 */
static int
stat_cmp(const void *a, const void *b)
{
   const struct stat *st1 = a;
   const struct stat *st2 = b;

   if ((st2->st_dev == st1->st_dev) && (st2->st_ino == st1->st_ino))
     return 0;
   return 1;
}

/**
 * @brief Performs sanity checks before recursing into a directory for monitoring.
 * Prevents excessive recursion depth and monitoring of the user's home directory,
 * which could be a sign of a misconfiguration or symlink loop.
 * @param stack An Eina_Inarray used to track the recursion path (dev/inode pairs).
 * @param path The path being considered for recursion.
 * @param stack_limit The maximum allowed recursion depth.
 * @return EINA_TRUE if it's safe to recurse, EINA_FALSE otherwise.
 */
static Eina_Bool
_check_recurse_monitor_sanity(Eina_Inarray *stack, const char *path, unsigned int stack_limit)
{
   const char *home = eina_environment_home_get();

   // protect against too deep recursion even if it's valid.
   if (eina_inarray_count(stack) >= stack_limit)
     {
        ERR("Recursing too far. Level %i. Stopping at %s\n", stack_limit, path);
        return EINA_FALSE;
     }
   // detect if we start recursing at $HOME - a sign of something wrong
   if ((home) && (!strcmp(home, path)))
     {
        ERR("Recursively monitor homedir! Ignore.");
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Recursively sets up Eio_Monitors for icon directories.
 * Traverses a directory tree, adding monitors for each directory found.
 * Uses the `subdir_cache` to quickly get subdirectory listings and avoid
 * redundant `stat` calls. It also uses an `Eina_Inarray` (stack) to detect
 * and prevent recursion loops (e.g. symlink loops).
 * @param stack An Eina_Inarray to track visited (dev_t, ino_t) pairs to prevent loops.
 *              The caller is responsible for initializing and flushing it for base paths.
 * @param path The current directory path to process.
 * @param base EINA_TRUE if this is a base directory (e.g., from XDG_DATA_DIRS),
 *             EINA_FALSE for subdirectories found during recursion.
 */
static void
icon_changes_listen_recursive(Eina_Inarray *stack, const char *path, Eina_Bool base)
{
   struct stat *st = eina_mempool_malloc(efreetd_mp_stat, sizeof(struct stat));
   if (!st) return;

   if (stat(path, st) == -1) return;
   if (eina_inarray_search(stack, st, stat_cmp) >= 0) return;
   if (!_check_recurse_monitor_sanity(stack, path, 10)) return;
   eina_inarray_push(stack, st);

   if ((!S_ISDIR(st->st_mode)) && (base))
     {
        // XXX: if it doesn't exist... walk the parent dirs back down
        // to this path until we find one that doesn't exist, then
        // monitor its parent, and treat it specially as it needs
        // to look for JUST the creation of this specific child
        // and when this child is created, replace this monitor with
        // monitoring the next specific child dir down until we are
        // monitoring the original path again.
     }
   if (S_ISDIR(st->st_mode))
     {
        unsigned int i;
        const Subdir_Cache_Dir *cd = subdir_cache_get(st, path);
        icon_changes_monitor_add(st, path);
        if (cd)
          {
             Eina_Strbuf *buf = eina_strbuf_new();
             if (!buf) return;
             for (i = 0; i < cd->dirs_count; i++)
               {

                  eina_strbuf_append_printf(buf,  "%s/%s", path, cd->dirs[i]);
                  icon_changes_listen_recursive(stack, eina_strbuf_string_get(buf), EINA_FALSE);
                  eina_strbuf_reset(buf);
               }
             eina_strbuf_free(buf);
          }
     }
   eina_inarray_pop(stack);
   eina_mempool_free(efreetd_mp_stat, st);
}

/**
 * @brief Recursively sets up Eio_Monitors for desktop directories.
 * Similar to `icon_changes_listen_recursive`, but for desktop file directories.
 * Traverses a directory tree, adding monitors for each directory found.
 * Uses the `subdir_cache` and an `Eina_Inarray` (stack) for efficiency and loop prevention.
 * @param stack An Eina_Inarray to track visited (dev_t, ino_t) pairs to prevent loops.
 *              The caller is responsible for initializing and flushing it for base paths.
 * @param path The current directory path to process.
 * @param base EINA_TRUE if this is a base directory, EINA_FALSE for subdirectories.
 */
static void
desktop_changes_listen_recursive(Eina_Inarray *stack, const char *path, Eina_Bool base)
{
   struct stat *st = eina_mempool_malloc(efreetd_mp_stat, sizeof(struct stat));
   if (!st) return;

   if (stat(path, st) == -1) return;
   if (eina_inarray_search(stack, st, stat_cmp) >= 0) return;
   if (!_check_recurse_monitor_sanity(stack, path, 10)) return;
   eina_inarray_push(stack, st);

   if ((!S_ISDIR(st->st_mode)) && (base))
     {
        // XXX: if it doesn't exist... walk the parent dirs back down
        // to this path until we find one that doesn't exist, then
        // monitor its parent, and treat it specially as it needs
        // to look for JUST the creation of this specific child
        // and when this child is created, replace this monitor with
        // monitoring the next specific child dir down until we are
        // monitoring the original path again.
     }
   if (S_ISDIR(st->st_mode))
     {
        unsigned int i;
        const Subdir_Cache_Dir *cd = subdir_cache_get(st, path);
        desktop_changes_monitor_add(st, path);
        if (cd)
          {
             Eina_Strbuf *buf = eina_strbuf_new();
             if (!buf) return;
             for (i = 0; i < cd->dirs_count; i++)
               {
                  eina_strbuf_append_printf(buf, "%s/%s", path, cd->dirs[i]);
                  desktop_changes_listen_recursive(stack, eina_strbuf_string_get(buf), EINA_FALSE);
                  eina_strbuf_reset(buf);
               }
             eina_strbuf_free(buf);
          }
     }
   eina_inarray_pop(stack);
   eina_mempool_free(efreetd_mp_stat, st);
}

/**
 * @brief Sets up monitoring for all relevant icon directories.
 * This includes user-specific icon directories, XDG data directories (e.g., /usr/share/icons),
 * and legacy pixmap directories. It uses `icon_changes_listen_recursive` to
 * traverse each base directory.
 */
static void
icon_changes_listen(void)
{
   Eina_List *l;
   Eina_List *xdg_dirs;
   const char *dir;
   Eina_Inarray *stack;
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return;

   stack = eina_inarray_new(sizeof(struct stat), 16);
   if (!stack)
     {
        eina_strbuf_free(buf);
        return;
     }
   icon_changes_listen_recursive(stack, efreet_icon_deprecated_user_dir_get(), EINA_TRUE);
   eina_inarray_flush(stack);
   icon_changes_listen_recursive(stack, efreet_icon_user_dir_get(), EINA_TRUE);
   EINA_LIST_FOREACH(icon_extra_dirs, l, dir)
     {
        if (!strcmp(dir, "/")) continue;
        eina_inarray_flush(stack);
        icon_changes_listen_recursive(stack, dir, EINA_TRUE);
     }

   xdg_dirs = efreet_data_dirs_get();
   EINA_LIST_FOREACH(xdg_dirs, l, dir)
     {
        eina_strbuf_append_printf(buf, "%s/icons", dir);
        eina_inarray_flush(stack);
        icon_changes_listen_recursive(stack, eina_strbuf_string_get(buf), EINA_TRUE);
        eina_strbuf_reset(buf);
     }

#ifndef STRICT_SPEC
   EINA_LIST_FOREACH(xdg_dirs, l, dir)
     {
        eina_strbuf_append_printf(buf, "%s/pixmaps", dir);
        eina_inarray_flush(stack);
        icon_changes_listen_recursive(stack, eina_strbuf_string_get(buf), EINA_TRUE);
        eina_strbuf_reset(buf);
     }
#endif
   eina_inarray_flush(stack);
   icon_changes_listen_recursive(stack, "/usr/local/share/pixmaps", EINA_TRUE);
   icon_changes_listen_recursive(stack, "/usr/share/pixmaps", EINA_TRUE);
   eina_inarray_free(stack);
   eina_strbuf_free(buf);
}

/**
 * @brief Sets up monitoring for all relevant desktop file directories.
 * This includes system-defined desktop directories and any extra directories
 * added by the user or applications. It uses `desktop_changes_listen_recursive`
 * to traverse each base directory.
 */
static void
desktop_changes_listen(void)
{
   Eina_List *l;
   const char *path;
   Eina_Inarray *stack;

   stack = eina_inarray_new(sizeof(struct stat), 16);
   if (!stack) return;
   EINA_LIST_FOREACH(desktop_system_dirs, l, path)
     {
        eina_inarray_flush(stack);
        desktop_changes_listen_recursive(stack, path, EINA_TRUE);
     }
   EINA_LIST_FOREACH(desktop_extra_dirs, l, path)
     {
        eina_inarray_flush(stack);
        desktop_changes_listen_recursive(stack, path, EINA_TRUE);
     }
   eina_inarray_free(stack);
}

/**
 * @brief Reads a list of strings from a cache file into an Eina_List.
 * Each line in the file becomes a stringshared item in the list.
 * The file is expected to be in `efreet_cache_home_get()/efreet/`.
 * @param file The name of the file (e.g., "extra_icons.dirs").
 * @param l A pointer to an Eina_List* to populate. The list will be appended to.
 */
static void
fill_list(const char *file, Eina_List **l)
{
   Eina_File *f = NULL;
   Eina_Iterator *it = NULL;
   Eina_File_Line *line = NULL;
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return;

   eina_strbuf_append_printf(buf, "%s/efreet/%s", efreet_cache_home_get(), file);
   f = eina_file_open(eina_strbuf_string_get(buf), EINA_FALSE);
   if (!f) goto error_buf;
   it = eina_file_map_lines(f);
   if (!it) goto error;
   EINA_ITERATOR_FOREACH(it, line)
     {
        if (line->end > line->start)
          {
             const char *s = eina_stringshare_add_length(line->start, line->end - line->start);
             if (s) *l = eina_list_append(*l, s);
          }
     }
   eina_iterator_free(it);
error:
   eina_file_close(f);
error_buf:
   eina_strbuf_free(buf);
}

/**
 * @brief Reads persisted lists of extra icon directories and icon extensions.
 * Loads data from `extra_icons.dirs` and `icons.exts` in the Efreet cache directory.
 */
static void
read_lists(void)
{
// dont use extra dirs as the only way to get extra dirs is by loading a
// specific desktop file at a specific path, and this is wrong
//   fill_list("extra_desktops.dirs", &desktop_extra_dirs);
   fill_list("extra_icons.dirs", &icon_extra_dirs);
   fill_list("icons.exts", &icon_exts);
}

/**
 * @brief Saves an Eina_List of strings to a cache file.
 * Each stringshared item in the list is written as a line in the file.
 * The file is created in `efreet_cache_home_get()/efreet/`.
 * @param file The name of the file (e.g., "extra_icons.dirs").
 * @param l The Eina_List of (const char *) strings to save.
 */
static void
save_list(const char *file, Eina_List *l)
{
   FILE *f;
   Eina_List *ll;
   const char *path;
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return;

   eina_strbuf_append_printf(buf, "%s/efreet/%s", efreet_cache_home_get(), file);
   f = fopen(eina_strbuf_string_get(buf), "wb");
   if (!f)
     {
        eina_strbuf_free(buf);
        return;
     }
   EINA_LIST_FOREACH(l, ll, path)
      fprintf(f, "%s\n", path);
   fclose(f);
   eina_strbuf_free(buf);
}

/**
 * @brief Comparison function for eina_list_search_unsorted_list.
 * Compares two C strings using strncmp, where the length of the comparison
 * is determined by the length of the first string (data1).
 * This is specifically used for checking if a path from `desktop_system_dirs`
 * is already present, potentially as a prefix, in another list.
 * @param data1 The first string (typically from `desktop_system_dirs`).
 * @param data2 The second string to compare against.
 * @return An integer less than, equal to, or greater than zero if data1 is found
 *         to be, respectively, less than, to match, or be greater than data2.
 */
static int
strcmplen(const void *data1, const void *data2)
{
   return strncmp(data1, data2, eina_stringshare_strlen(data1));
}

/**
 * @brief Ecore_Event_Handler callback for ECORE_EXE_EVENT_DATA.
 * Handles data (stdout) received from cache generation helper programs
 * (`efreet_icon_cache_create`, `efreet_desktop_cache_create`, `efreet_mime_cache_create`).
 * For desktop and icon caches, it checks if the output indicates a change ('c')
 * and sends appropriate signals to clients.
 * @param data Unused.
 * @param type The type of the event (unused).
 * @param event The Ecore_Exe_Event_Data structure.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
cache_exe_data_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Data *ev = event;

   if (ev->exe == desktop_cache_exe)
     {
        Eina_Bool update = EINA_FALSE;

        fprintf(efreetd_log_file, "[%09.3f] Data desktop_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        if ((ev->lines) && (*ev->lines->line == 'c')) update = EINA_TRUE;
        if (!desktop_exists)
          send_signal_desktop_cache_build();
        desktop_exists = EINA_TRUE;
        send_signal_desktop_cache_update(update);
     }
   else if (ev->exe == icon_cache_exe)
     {
        Eina_Bool update = EINA_FALSE;

        fprintf(efreetd_log_file, "[%09.3f] Data icon_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        if ((ev->lines) && (*ev->lines->line == 'c')) update = EINA_TRUE;
        send_signal_icon_cache_update(update);
     }
   else if (ev->exe == mime_cache_exe)
     {
        fprintf(efreetd_log_file, "[%09.3f] Data mime_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        // XXX: ZZZ: handle stdout here from cache updater... if needed
     }
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Ecore_Event_Handler callback for ECORE_EXE_EVENT_DEL.
 * Handles the termination of cache generation helper programs.
 * It updates the status of the respective cache process (e.g., `desktop_cache_exe = NULL`).
 * If a cache update was queued while the previous one was running, it triggers the queued update.
 * For MIME cache, it sends a signal indicating the build is complete.
 * @param data Unused.
 * @param type The type of the event (unused).
 * @param event The Ecore_Exe_Event_Del structure.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
cache_exe_del_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *ev = event;

   if (ev->exe == desktop_cache_exe)
     {
        fprintf(efreetd_log_file, "[%09.3f] Exit desktop_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        desktop_cache_exe = NULL;
        if (desktop_queue) cache_desktop_update();
     }
   else if (ev->exe == icon_cache_exe)
     {
        fprintf(efreetd_log_file, "[%09.3f] Exit icon_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        icon_cache_exe = NULL;
        if (icon_queue) cache_icon_update(EINA_FALSE);
     }
   else if (ev->exe == mime_cache_exe)
     {
        fprintf(efreetd_log_file, "[%09.3f] Exit mime_cache_create\n", ecore_time_get());
        fflush(efreetd_log_file);
        mime_cache_exe = NULL;
        send_signal_mime_cache_build();
     }
   return ECORE_CALLBACK_RENEW;
}

/* external */
void
cache_desktop_dir_add(const char *dir)
{
   char *san;
   Eina_List *l;

   san = eina_file_path_sanitize(dir);
   if (!san) return;
   if ((l = eina_list_search_unsorted_list(desktop_system_dirs, strcmplen, san)))
     {
        /* Path is registered, but maybe not monitored */
        const char *path = eina_list_data_get(l);
        if (!eina_hash_find(desktop_change_monitors, path))
          cache_desktop_update();
     }
   else if (!eina_list_search_unsorted_list(desktop_extra_dirs, EINA_COMPARE_CB(strcmp), san))
     {
        /* Not a registered path */
        desktop_extra_dirs = eina_list_append(desktop_extra_dirs, eina_stringshare_add(san));
        save_list("extra_desktops.dirs", desktop_extra_dirs);
        cache_desktop_update();
     }
   free(san);
}

void
cache_icon_dir_add(const char *dir)
{
   char *san;

   san = eina_file_path_sanitize(dir);
   if (!san) return;
   if (!eina_list_search_unsorted_list(icon_extra_dirs, EINA_COMPARE_CB(strcmp), san))
     {
        if (!strcmp(san, "/")) goto out;
        icon_extra_dirs = eina_list_append(icon_extra_dirs, eina_stringshare_add(san));
        save_list("extra_icons.dirs", icon_extra_dirs);
        cache_icon_update(EINA_TRUE);
     }
out:
   free(san);
}

void
cache_icon_ext_add(const char *ext)
{
   if (!eina_list_search_unsorted_list(icon_exts, EINA_COMPARE_CB(strcmp), ext))
     {
        icon_exts = eina_list_append(icon_exts, eina_stringshare_add(ext));
        save_list("icons.exts", icon_exts);
        cache_icon_update(EINA_TRUE);
     }
}

Eina_Bool
cache_desktop_exists(void)
{
   return desktop_exists;
}

/**
 * @brief Launches the `efreet_mime_cache_create` helper program.
 * This function constructs the command and executes it using `ecore_exe_pipe_run`.
 */
static void
mime_update_launch(void)
{
   Eina_Strbuf *file = eina_strbuf_new();
   if (!file) return;

   eina_strbuf_append_printf(file,
            "%s/efreet/" MODULE_ARCH "/efreet_mime_cache_create",
            eina_prefix_lib_get(pfx));
   mime_cache_exe = ecore_exe_pipe_run(eina_strbuf_string_get(file),
                                       ECORE_EXE_PIPE_READ |
                                       ECORE_EXE_PIPE_READ_LINE_BUFFERED,
                                       NULL);
   eina_strbuf_free(file);
}

/**
 * @brief Ecore_Timer callback to trigger a MIME cache update.
 * This function is called after a short delay to debounce multiple
 * rapid requests for MIME cache updates. It ensures any existing
 * MIME cache process is killed before launching a new one.
 * @param data Unused.
 * @return EINA_FALSE to remove the timer.
 */
static Eina_Bool
mime_update_cache_cb(void *data EINA_UNUSED)
{
   mime_update_timer = NULL;
   if (mime_cache_exe)
     {
        ecore_exe_kill(mime_cache_exe);
        ecore_exe_free(mime_cache_exe);
     }
   mime_update_launch();
   return EINA_FALSE;
}

/**
 * @brief Initializes monitoring for MIME type related files and directories.
 * Sets up Eio_Monitors for standard MIME locations like `/etc/mime.types`,
 * `/usr/share/mime/globs`, and `XDG_DATA_DIRS/mime/globs`.
 * Changes in these locations will trigger `mime_update_cache_cb`.
 */
static void
mime_cache_init(void)
{
   Eio_Monitor *mon;
   Eina_List *datadirs, *l;
   const char *s;
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!buf) return;

   mime_monitors = eina_hash_string_superfast_new
     (EINA_FREE_CB(eio_monitor_del));
   mime_monitors_mon = eina_hash_pointer_new(NULL);

   if (ecore_file_is_dir("/etc"))
     {
        mon = eio_monitor_add("/etc"); // specifically look at /etc/mime.types
        if (mon)
          {
             eina_hash_add(mime_monitors, "/etc", mon);
             eina_hash_add(mime_monitors_mon, &mon, mon);
          }
     }
   if (ecore_file_is_dir("/usr/share/mime"))
     {
        mon = eio_monitor_add("/usr/share/mime"); // specifically look at /usr/share/mime/globs
        if (mon)
          {
             eina_hash_add(mime_monitors, "/usr/share/mime", mon);
             eina_hash_add(mime_monitors_mon, &mon, mon);
          }
     }

   datadirs = efreet_data_dirs_get();
   EINA_LIST_FOREACH(datadirs, l, s)
     {
        eina_strbuf_append_printf(buf, "%s/mime", s); // specifically lok at XXX/mime/globs
        if (ecore_file_is_dir(eina_strbuf_string_get(buf)))
          {
             if (!eina_hash_find(mime_monitors, eina_strbuf_string_get(buf)))
               {
                  mon = eio_monitor_add(eina_strbuf_string_get(buf));
                  if (mon)
                    {
                       eina_hash_add(mime_monitors, eina_strbuf_string_get(buf), mon);
                       eina_hash_add(mime_monitors_mon, &mon, mon);
                    }
               }
          }
     }
   eina_strbuf_free(buf);
}

/**
 * @brief Shuts down the MIME cache monitoring system.
 * Deletes the MIME update timer and frees the hash tables used for
 * storing Eio_Monitor objects.
 */
static void
mime_cache_shutdown(void)
{
   if (mime_update_timer)
     {
        ecore_timer_del(mime_update_timer);
        mime_update_timer = NULL;
     }
   if (mime_monitors)
     {
        eina_hash_free(mime_monitors);
        mime_monitors = NULL;
     }
   if (mime_monitors_mon)
     {
        eina_hash_free(mime_monitors_mon);
        mime_monitors_mon = NULL;
     }
}

/**
 * @brief Initializes the Efreetd caching system.
 *
 * This function performs the following steps:
 * 1. Initializes Eina_Prefix to locate helper executables.
 * 2. Sets up Ecore event handlers for process completion (ECORE_EXE_EVENT_DEL)
 *    and data output (ECORE_EXE_EVENT_DATA) from cache helper programs.
 * 3. Initializes hash tables for icon and desktop directory monitors.
 * 4. Initializes the Efreet library itself.
 * 5. Initializes EIO for file system monitoring.
 * 6. Registers Ecore event handlers for various EIO_MONITOR events, all
 *    pointing to `_cb_monitor_event`.
 * 7. Initializes the subdirectory cache (`subdir_cache_init`).
 * 8. Initializes MIME cache monitoring (`mime_cache_init`) and launches an
 *    initial MIME cache update (`mime_update_launch`).
 * 9. Reads persisted lists of extra icon directories and extensions (`read_lists`).
 * 10. Populates the list of system desktop directories.
 * 11. Sets up recursive listeners for icon and desktop directory changes
 *     (`icon_changes_listen`, `desktop_changes_listen`).
 * 12. Triggers initial updates for icon and desktop caches.
 * 13. Saves the subdirectory cache if it was modified.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE on failure.
 */
Eina_Bool
cache_init(void)
{
   char **argv;

   ecore_app_args_get(NULL, &argv);

   pfx = eina_prefix_new(argv[0], cache_init,
                         "EFREET", "efreet", "checkme",
                         PACKAGE_BIN_DIR,
                         PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR,
                         PACKAGE_DATA_DIR);

   cache_exe_del_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                                                   cache_exe_del_cb, NULL);
   if (!cache_exe_del_handler)
     {
        ERR("Failed to add exe del handler");
        goto error;
     }
   cache_exe_data_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
                                                    cache_exe_data_cb, NULL);
   if (!cache_exe_data_handler)
     {
        ERR("Failed to add exe data handler");
        goto error;
     }

   icon_change_monitors = eina_hash_string_superfast_new
     (EINA_FREE_CB(eio_monitor_del));
   icon_change_monitors_mon = eina_hash_pointer_new(NULL);
   desktop_change_monitors = eina_hash_string_superfast_new
     (EINA_FREE_CB(eio_monitor_del));
   desktop_change_monitors_mon = eina_hash_pointer_new(NULL);

   efreet_cache_update = 0;
   if (!efreet_init()) goto error;
   eio_init();

#define MONITOR_EVENT(ev, fn) \
_handlers = eina_list_append(_handlers, ecore_event_handler_add(ev, fn, NULL))
   MONITOR_EVENT(EIO_MONITOR_FILE_CREATED,       _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_FILE_DELETED,       _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_FILE_MODIFIED,      _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_DIRECTORY_CREATED,  _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_DIRECTORY_DELETED,  _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_DIRECTORY_MODIFIED, _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_SELF_RENAME,        _cb_monitor_event);
   MONITOR_EVENT(EIO_MONITOR_SELF_DELETED,       _cb_monitor_event);

   subdir_cache_init();
   mime_cache_init();
   mime_update_launch();
   read_lists();
   /* TODO: Should check if system dirs has changed and handles extra_dirs */
   desktop_system_dirs = efreet_default_dirs_get(efreet_data_home_get(),
                                                 efreet_data_dirs_get(), "applications");
   desktop_system_dirs =
      eina_list_merge(
         desktop_system_dirs, efreet_default_dirs_get(efreet_data_home_get(),
                                                      efreet_data_dirs_get(), "desktop-directories"));
   icon_changes_listen();
   desktop_changes_listen();
   cache_icon_update(EINA_FALSE);
   cache_desktop_update();
   subdir_cache_save();

   return EINA_TRUE;
error:
   if (cache_exe_del_handler) ecore_event_handler_del(cache_exe_del_handler);
   cache_exe_del_handler = NULL;
   if (cache_exe_data_handler) ecore_event_handler_del(cache_exe_data_handler);
   cache_exe_data_handler = NULL;
   return EINA_FALSE;
}

/**
 * @brief Shuts down the Efreetd caching system.
 *
 * This function performs the following cleanup steps:
 * 1. Frees the Eina_Prefix object.
 * 2. Shuts down MIME cache monitoring (`mime_cache_shutdown`).
 * 3. Shuts down the subdirectory cache (`subdir_cache_shutdown`). Note: This does
 *    not save any pending changes; `subdir_cache_save()` should be called before
 *    shutdown if persistence is needed.
 * 4. Shuts down the Efreet library itself (`efreet_shutdown`).
 * 5. Deletes Ecore event handlers for ECORE_EXE_EVENT_DEL and ECORE_EXE_EVENT_DATA.
 * 6. Frees hash tables used for icon and desktop directory monitors.
 * 7. Frees lists of desktop system directories, extra desktop directories,
 *    extra icon directories, and icon extensions.
 * 8. Deletes Ecore event handlers for EIO_MONITOR events.
 * 9. Shuts down EIO.
 *
 * @return EINA_TRUE always.
 */
Eina_Bool
cache_shutdown(void)
{
   const char *data;
   Ecore_Event_Handler *handler;

   eina_prefix_free(pfx);
   pfx = NULL;

   mime_cache_shutdown();
   subdir_cache_shutdown();
   efreet_shutdown();

   if (cache_exe_del_handler) ecore_event_handler_del(cache_exe_del_handler);
   cache_exe_del_handler = NULL;
   if (cache_exe_data_handler) ecore_event_handler_del(cache_exe_data_handler);
   cache_exe_data_handler = NULL;

   if (icon_change_monitors) eina_hash_free(icon_change_monitors);
   icon_change_monitors = NULL;
   if (icon_change_monitors_mon) eina_hash_free(icon_change_monitors_mon);
   icon_change_monitors_mon = NULL;
   if (desktop_change_monitors) eina_hash_free(desktop_change_monitors);
   desktop_change_monitors = NULL;
   if (desktop_change_monitors_mon) eina_hash_free(desktop_change_monitors_mon);
   desktop_change_monitors_mon = NULL;
   EINA_LIST_FREE(desktop_system_dirs, data)
      eina_stringshare_del(data);
   EINA_LIST_FREE(desktop_extra_dirs, data)
      eina_stringshare_del(data);
   EINA_LIST_FREE(icon_extra_dirs, data)
      eina_stringshare_del(data);
   EINA_LIST_FREE(icon_exts, data)
      eina_stringshare_del(data);
   EINA_LIST_FREE(_handlers, handler)
      ecore_event_handler_del(handler);
   eio_shutdown();
   return EINA_TRUE;
}
