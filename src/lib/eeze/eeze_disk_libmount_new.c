#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_UNSTABLE_LIBMOUNT_API
# define USE_UNSTABLE_LIBMOUNT_API 1
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#ifdef HAVE_EEZE_MOUNT
# include <libmount.h>
#endif
#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

/*
 *
 * PRIVATE
 *
 */

/**
 * @brief Mapping of mount option strings to Eeze_Disk_Mountopt flags.
 * Used by mnt_optstr_get_flags() to parse mount options.
 */
static struct libmnt_optmap eeze_optmap[] =
{
   { "loop[=]", EEZE_DISK_MOUNTOPT_LOOP, 0 },
   { "utf8", EEZE_DISK_MOUNTOPT_UTF8, 0 },
   { "noexec", EEZE_DISK_MOUNTOPT_NOEXEC, 0 },
   { "nosuid", EEZE_DISK_MOUNTOPT_NOSUID, 0 },
   { "remount", EEZE_DISK_MOUNTOPT_REMOUNT, 0 },
   { "uid[=]", EEZE_DISK_MOUNTOPT_UID, 0 },
   { "nodev", EEZE_DISK_MOUNTOPT_NODEV, 0 },
   { NULL, 0, 0 }
};
typedef struct libmnt_table libmnt_table; /**< Typedef for libmount table structure. */
typedef struct libmnt_fs libmnt_fs; /**< Typedef for libmount filesystem structure. */
typedef struct libmnt_cache libmnt_cache; /**< Typedef for libmount cache structure. */
static Ecore_File_Monitor *_fstab_mon = NULL; /**< File monitor for /etc/fstab. */
static Eina_Bool _watching = EINA_FALSE; /**< Flag indicating if mount tabs are being watched. */
static Eina_Bool _fstab_scan_active = EINA_FALSE; /**< Flag to prevent recursive scans of fstab. */
static libmnt_cache *_eeze_mount_mtab_cache = NULL; /**< Cache for mtab data. */
static libmnt_cache *_eeze_mount_fstab_cache = NULL; /**< Cache for fstab data. */
static libmnt_table *_eeze_mount_mtab = NULL; /**< Parsed mtab data. */
static libmnt_table *_eeze_mount_fstab = NULL; /**< Parsed fstab data. */
extern Eina_List *_eeze_disks; /**< External list of known Eeze_Disk objects. */

static Ecore_Fd_Handler *_mountinfo_fdh = NULL; /**< FD handler for /proc/self/mountinfo. */
static int _mountinfo = -1; /**< File descriptor for /proc/self/mountinfo. */

static libmnt_table *_eeze_mount_tab_parse(const char *filename);
static void _eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon EINA_UNUSED, Ecore_File_Event event EINA_UNUSED, const char *path);

/**
 * @brief Callback function for libmount table parsing errors.
 *
 * This function is invoked by libmount when an error occurs while parsing
 * a mount table file (e.g., /etc/fstab or /proc/self/mountinfo).
 *
 * @param tab The libmount table being parsed (unused).
 * @param filename The name of the file where the parsing error occurred.
 * @param line The line number in the file where the error occurred.
 * @return Always returns -1 to indicate an error to libmount.
 */
static int
_eeze_mount_tab_parse_errcb(libmnt_table *tab EINA_UNUSED, const char *filename, int line)
{
   ERR("%s:%d: could not parse line!", filename, line); /* most worthless error reporting ever. */
   return -1;
}

/*
 * I could use mnt_new_table_from_file() but this way gives much more detailed output
 * on failure so why not
 */
/**
 * @brief Parses a mount table file (e.g., fstab, mtab) into a libmnt_table.
 *
 * This function initializes a new libmount table and attempts to parse the
 * specified file. It sets a custom error callback for more detailed error
 * reporting.
 *
 * @param filename The path to the mount table file to parse.
 *                 Example: "/etc/fstab", "/proc/self/mountinfo".
 * @return A pointer to the newly created and parsed libmnt_table on success,
 *         or NULL on failure (e.g., allocation error, parse error).
 */
static libmnt_table *
_eeze_mount_tab_parse(const char *filename)
{
   libmnt_table *tab;

   if (!(tab = mnt_new_table())) return NULL;
   if (mnt_table_set_parser_errcb(tab, _eeze_mount_tab_parse_errcb))
     {
        ERR("Alloc!");
        mnt_free_table(tab);
        return NULL;
     }

   if (!mnt_table_parse_file(tab, filename))
     return tab;

   mnt_free_table(tab);
   return NULL;
}

/**
 * @brief Callback function for Ecore_File_Monitor watching /etc/fstab.
 *
 * This function is triggered when /etc/fstab changes. It re-parses the
 * file and updates the internal cache. The `data` parameter is used to
 * distinguish between mtab and fstab updates, though in this specific
 * callback, it's implicitly for fstab.
 *
 * @param data User data passed to ecore_file_monitor_add. If non-NULL,
 *             it implies an update related to mtab cache (though this
 *             function is registered for fstab). If NULL, it implies
 *             an update related to fstab cache.
 * @param mon The Ecore_File_Monitor that triggered the event (unused).
 * @param event The type of file event that occurred (unused).
 * @param path The path to the file that changed (expected to be /etc/fstab).
 */
static void
_eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon EINA_UNUSED, Ecore_File_Event event EINA_UNUSED, const char *path)
{
   libmnt_table *bak;

   if (_fstab_scan_active)
     /* prevent scans from triggering a scan */
     return;

   bak = _eeze_mount_mtab;
   _eeze_mount_mtab = _eeze_mount_tab_parse(path);
   if (!_eeze_mount_mtab)
     {
        ERR("Could not parse %s! keeping old tab...", path);
        goto error;
     }

   mnt_free_table(bak);
   if (data)
     {
        mnt_free_cache(_eeze_mount_mtab_cache);
        _eeze_mount_mtab_cache = mnt_new_cache();
        mnt_table_set_cache(_eeze_mount_mtab, _eeze_mount_mtab_cache);
     }
   else
     {
        mnt_free_cache(_eeze_mount_fstab_cache);
        _eeze_mount_fstab_cache = mnt_new_cache();
        mnt_table_set_cache(_eeze_mount_fstab, _eeze_mount_fstab_cache);
     }
   return;

error:
   mnt_free_table(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
}

/* on tab change, check differences
 * based on code from findmnt
 */
/**
 * @brief Ecore_Fd_Handler callback for monitoring /proc/self/mountinfo changes.
 *
 * This function is called when there's activity on the file descriptor for
 * /proc/self/mountinfo, typically indicating a change in the system's mount points.
 * It re-parses mountinfo, compares it with the previous state, and emits
 * EEZE_EVENT_DISK_MOUNT or EEZE_EVENT_DISK_UNMOUNT events for affected Eeze_Disk
 * objects.
 *
 * @param d User data associated with the Fd_Handler (unused).
 * @param fdh The Ecore_Fd_Handler that triggered the callback (unused).
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_eeze_mount_fdh(void *d EINA_UNUSED, Ecore_Fd_Handler *fdh EINA_UNUSED)
{
   libmnt_table *tb_new;
   libmnt_fs *old, *new;
   int change;
   struct libmnt_iter *itr = NULL;
   struct libmnt_tabdiff *diff = NULL;

   tb_new = mnt_new_table();
   EINA_SAFETY_ON_NULL_RETURN_VAL(tb_new, ECORE_CALLBACK_RENEW);
   EINA_SAFETY_ON_TRUE_GOTO(mnt_table_set_parser_errcb(tb_new, _eeze_mount_tab_parse_errcb), err);
   itr = mnt_new_iter(MNT_ITER_BACKWARD);
   EINA_SAFETY_ON_NULL_GOTO(itr, err);
   diff = mnt_new_tabdiff();
   EINA_SAFETY_ON_NULL_GOTO(diff, err);
   if (mnt_table_parse_file(tb_new, "/proc/self/mountinfo"))
     {
        ERR("PARSING FAILED FOR /proc/self/mountinfo! THIS IS WEIRD!");
        goto err;
     }
   change = mnt_diff_tables(diff, _eeze_mount_mtab, tb_new);
   if (change < 0)
     {
        ERR("DIFFING FAILED FOR /proc/self/mountinfo! THIS IS ALSO WEIRD!");
        goto err;
     }
   if (!change) goto err;
   while (!mnt_tabdiff_next_change(diff, itr, &old, &new, &change))
     {
        const char *src;
        Eeze_Disk *disk;
        Eina_Bool found =  EINA_FALSE;
        Eeze_Event_Disk_Mount *e;
        Eina_List *l;

        src = mnt_fs_get_source(new);
        if (!src) continue;
        EINA_LIST_FOREACH(_eeze_disks, l, disk)
          {
             if (!strcmp(src, eeze_disk_devpath_get(disk)))
               {
                  found = EINA_TRUE;
                  break;
               }
          }
        if (!found) continue;
        switch (change)
          {
           case MNT_TABDIFF_MOUNT:
             disk->mounted = EINA_TRUE;
             eina_stringshare_replace(&disk->mount_point, mnt_fs_get_target(new));
             if (disk->mount_status) break;
             e = malloc(sizeof(Eeze_Event_Disk_Mount));
             if (e)
               {
                  e->disk = disk;
                  ecore_event_add(EEZE_EVENT_DISK_MOUNT, e, NULL, NULL);
               }
             break;
           case MNT_TABDIFF_UMOUNT:
             if (!mnt_fs_get_target(new))
               disk->mounted = EINA_FALSE;
             eina_stringshare_replace(&disk->mount_point, NULL);
             if (disk->mount_status) break;
             e = malloc(sizeof(Eeze_Event_Disk_Mount));
             if (e)
               {
                  e->disk = disk;
                  ecore_event_add(EEZE_EVENT_DISK_UNMOUNT, e, NULL, NULL);
               }
             break;
           /* anything could have happened here, send both events to flush */
           case MNT_TABDIFF_REMOUNT:
           case MNT_TABDIFF_MOVE:
             if (!mnt_fs_get_target(new))
               disk->mounted = EINA_FALSE;
             eina_stringshare_replace(&disk->mount_point, mnt_fs_get_target(new));
             if (disk->mount_status) break;
             e = malloc(sizeof(Eeze_Event_Disk_Mount));
             if (e)
               {
                  e->disk = disk;
                  ecore_event_add(EEZE_EVENT_DISK_UNMOUNT, e, NULL, NULL);
               }
             e = malloc(sizeof(Eeze_Event_Disk_Mount));
             if (e)
               {
                  e->disk = disk;
                  ecore_event_add(EEZE_EVENT_DISK_MOUNT, e, NULL, NULL);
               }
           default:
             break;
          }
     }

   mnt_free_cache(_eeze_mount_mtab_cache);
   _eeze_mount_mtab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_mtab, _eeze_mount_mtab_cache);
   mnt_free_table(_eeze_mount_mtab);
   _eeze_mount_mtab = tb_new;
   return ECORE_CALLBACK_RENEW;
err:
   if (tb_new) mnt_free_table(tb_new);
   if (itr) mnt_free_iter(itr);
   if (diff) mnt_free_tabdiff(diff);
   return ECORE_CALLBACK_RENEW;
}

/*
 *
 * INVISIBLE
 *
 */

/**
 * @brief Initializes the eeze libmount integration.
 *
 * Currently a placeholder.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eeze_libmount_init(void)
{
   /* placeholder */
   return EINA_TRUE;
}

/**
 * @brief Shuts down the eeze libmount integration.
 *
 * Frees allocated libmount tables and caches, and stops watching
 * mount tabs if currently active.
 */
void
eeze_libmount_shutdown(void)
{
   if (_eeze_mount_fstab)
     {
        mnt_free_table(_eeze_mount_fstab);
        mnt_free_cache(_eeze_mount_fstab_cache);
     }
   if (_eeze_mount_mtab)
     {
        mnt_free_table(_eeze_mount_mtab);
        mnt_free_cache(_eeze_mount_mtab_cache);
     }
   eeze_mount_tabs_unwatch();
}

/**
 * @brief Retrieves the mount options for a given disk.
 *
 * Scans mtab and fstab (if not already cached) and looks up the disk
 * by its UUID. It then parses the mount options string associated with
 * the disk.
 *
 * @param disk The Eeze_Disk object to get mount options for.
 * @return A bitmask of Eeze_Disk_Mountopt flags representing the active
 *         mount options for the disk. Returns 0 if the disk is not found,
 *         has no options, or an error occurs.
 *         Example: (EEZE_DISK_MOUNTOPT_NOEXEC | EEZE_DISK_MOUNTOPT_NOSUID)
 */
unsigned long
eeze_disk_libmount_opts_get(Eeze_Disk *disk)
{
   libmnt_fs *mnt;
   const char *opts;
   unsigned long f = 0;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return 0;

   mnt = mnt_table_find_tag(_eeze_mount_mtab, "UUID", eeze_disk_uuid_get(disk), MNT_ITER_BACKWARD);
   if (!mnt)
     mnt = mnt_table_find_tag(_eeze_mount_fstab, "UUID", eeze_disk_uuid_get(disk), MNT_ITER_BACKWARD);

   if (!mnt) return 0;

   opts = mnt_fs_get_fs_options(mnt);
   if (!opts) return 0;
   if (!mnt_optstr_get_flags(opts, &f, eeze_optmap)) return 0;
   return f;
}

/*
 * helper function to return whether a disk is mounted
 */
/**
 * @brief Checks if a disk is currently mounted and updates its mount point.
 *
 * Scans mtab (if not already cached) to determine if the given disk's
 * device path is listed as mounted. If it is, the disk's `mounted` status
 * and `mount_point` are updated.
 *
 * @param disk The Eeze_Disk object to check.
 * @return EINA_TRUE if the disk is mounted, EINA_FALSE otherwise or if an
 *         error occurs.
 */
Eina_Bool
eeze_disk_libmount_mounted_get(Eeze_Disk *disk)
{
   libmnt_fs *mnt;

   if (!disk)
     return EINA_FALSE;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return EINA_FALSE;

   mnt = mnt_table_find_source(_eeze_mount_mtab, eeze_disk_devpath_get(disk), MNT_ITER_BACKWARD);
   if (!mnt)
     {
        disk->mounted = EINA_FALSE;
        return EINA_FALSE;
     }

   eina_stringshare_replace(&disk->mount_point, mnt_fs_get_target(mnt));
   disk->mounted = EINA_TRUE;
   return EINA_TRUE;
}


/*
 * helper function to return the device that is mounted at a mount point
 */
/**
 * @brief Finds the source device path for a given mount point.
 *
 * Scans mtab and fstab (if not already cached) to find an entry
 * whose target matches the given mount_point.
 *
 * @param mount_point The mount point to search for. Example: "/mnt/data".
 * @return The source device path (e.g., "/dev/sda1") if found,
 *         NULL otherwise or if an error occurs.
 */
const char *
eeze_disk_libmount_mp_find_source(const char *mount_point)
{
   libmnt_fs *mnt;

   if (!mount_point)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_table_find_target(_eeze_mount_mtab, mount_point, MNT_ITER_BACKWARD);
   if (!mnt)
     mnt = mnt_table_find_target(_eeze_mount_fstab, mount_point, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_source(mnt);
}

/*
 * helper function to return a mount point from a uuid
 */
/**
 * @brief Looks up a mount point in fstab by its UUID.
 *
 * Scans fstab (if not already cached) for an entry with a matching UUID tag.
 *
 * @param uuid The UUID string to search for. Example: "a1b2c3d4-e5f6-7890-1234-567890abcdef".
 * @return The target mount point (e.g., "/media/usb_drive") if found,
 *         NULL otherwise or if an error occurs.
 */
const char *
eeze_disk_libmount_mp_lookup_by_uuid(const char *uuid)
{
   libmnt_fs *mnt;

   if (!uuid)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_table_find_tag(_eeze_mount_fstab, "UUID", uuid, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_target(mnt);
}

/*
 * helper function to return a mount point from a label
 */
/**
 * @brief Looks up a mount point in fstab by its filesystem label.
 *
 * Scans fstab (if not already cached) for an entry with a matching LABEL tag.
 *
 * @param label The filesystem label to search for. Example: "MyUSB".
 * @return The target mount point (e.g., "/media/MyUSB") if found,
 *         NULL otherwise or if an error occurs.
 */
const char *
eeze_disk_libmount_mp_lookup_by_label(const char *label)
{
   libmnt_fs *mnt;

   if (!label)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_table_find_tag(_eeze_mount_fstab, "LABEL", label, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_target(mnt);
}

/*
 * helper function to return a mount point from a /dev/ path
 */
/**
 * @brief Looks up a mount point in mtab or fstab by its device path.
 *
 * Scans mtab and then fstab (if not already cached) for an entry
 * whose source path matches the given devpath.
 *
 * @param devpath The device path to search for. Example: "/dev/sdb1".
 * @return The target mount point (e.g., "/var/log") if found,
 *         NULL otherwise or if an error occurs.
 */
const char *
eeze_disk_libmount_mp_lookup_by_devpath(const char *devpath)
{
   libmnt_fs *mnt;

   if (!devpath)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_table_find_srcpath(_eeze_mount_mtab, devpath, MNT_ITER_BACKWARD);
   if (!mnt)
     mnt = mnt_table_find_srcpath(_eeze_mount_fstab, devpath, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_target(mnt);
}

/*
 *
 * API
 *
 */

/**
 * @brief Starts watching /proc/self/mountinfo and /etc/fstab for changes.
 *
 * This function parses the current mountinfo and fstab, caches them,
 * and sets up monitoring mechanisms:
 * - An Ecore_Fd_Handler for /proc/self/mountinfo to detect mount/unmount events.
 * - An Ecore_File_Monitor for /etc/fstab to detect changes to static mount configurations.
 *
 * If already watching, this function does nothing and returns EINA_TRUE.
 *
 * @return EINA_TRUE if watching was successfully started or was already active.
 *         EINA_FALSE if an error occurred during setup (e.g., parsing failed,
 *         could not open files, could not set up monitors).
 */
EAPI Eina_Bool
eeze_mount_tabs_watch(void)
{
   libmnt_table *bak;

   if (_watching)
     return EINA_TRUE;

   bak = _eeze_mount_tab_parse("/proc/self/mountinfo");
   EINA_SAFETY_ON_NULL_GOTO(bak, error);

   mnt_free_table(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
   bak = _eeze_mount_tab_parse("/etc/fstab");
   EINA_SAFETY_ON_NULL_GOTO(bak, error);

   mnt_free_table(_eeze_mount_fstab);
   _eeze_mount_fstab = bak;

   _eeze_mount_mtab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_mtab, _eeze_mount_mtab_cache);

   _eeze_mount_fstab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_fstab, _eeze_mount_fstab_cache);

   _mountinfo = open("/proc/self/mountinfo", O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
   if (_mountinfo < 0) goto error;
   if (fcntl(_mountinfo, F_SETFL, O_NONBLOCK) < 0) goto error;

   _mountinfo_fdh = ecore_main_fd_handler_file_add(_mountinfo, ECORE_FD_ERROR, _eeze_mount_fdh, NULL, NULL, NULL);
   if (!_mountinfo_fdh) goto error;
   _fstab_mon = ecore_file_monitor_add("/etc/fstab", _eeze_mount_tab_watcher, NULL);
   _watching = EINA_TRUE;

  return EINA_TRUE;

error:
   if (_mountinfo >= 0) close(_mountinfo);
   _mountinfo = -1;
   if (!_eeze_mount_mtab)
     ERR("Could not parse /proc/self/mountinfo!");
   else
     {
        ERR("Could not parse /etc/fstab!");
        mnt_free_table(_eeze_mount_mtab);
     }
   _eeze_mount_mtab = _eeze_mount_fstab = NULL;
   return EINA_FALSE;
}

/**
 * @brief Stops watching /proc/self/mountinfo and /etc/fstab for changes.
 *
 * Removes the Ecore_Fd_Handler for mountinfo and the Ecore_File_Monitor
 * for fstab. Closes the file descriptor for mountinfo.
 * If not currently watching, this function does nothing.
 */
EAPI void
eeze_mount_tabs_unwatch(void)
{
   if (!_watching)
     return;

   ecore_main_fd_handler_del(_mountinfo_fdh);
   ecore_file_monitor_del(_fstab_mon);
   close(_mountinfo);
   _mountinfo = -1;
   _fstab_mon = NULL;
   _watching = EINA_FALSE;
}

/**
 * @brief Scans and caches /proc/self/mountinfo (mtab).
 *
 * This function parses /proc/self/mountinfo and updates the internal mtab cache.
 * If watching is active (via eeze_mount_tabs_watch()), this function assumes
 * the cache is up-to-date and does nothing, returning EINA_TRUE.
 * This is useful for a one-time scan if continuous monitoring is not required.
 *
 * @return EINA_TRUE if the scan was successful or if watching is active.
 *         EINA_FALSE if parsing /proc/self/mountinfo failed.
 */
EAPI Eina_Bool
eeze_mount_mtab_scan(void)
{
   libmnt_table *bak;

   if (_watching)
     return EINA_TRUE;

   bak = _eeze_mount_tab_parse("/proc/self/mountinfo");
   if (!bak)
     goto error;
   if (_eeze_mount_mtab)
     {
        mnt_free_table(_eeze_mount_mtab);
        mnt_free_cache(_eeze_mount_mtab_cache);
     }
   _eeze_mount_mtab = bak;
   _eeze_mount_mtab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_mtab, _eeze_mount_mtab_cache);

   return EINA_TRUE;

error:
   return EINA_FALSE;
}

/**
 * @brief Scans and caches /etc/fstab.
 *
 * This function parses /etc/fstab and updates the internal fstab cache.
 * If watching is active (via eeze_mount_tabs_watch()), this function assumes
 * the cache is up-to-date and does nothing, returning EINA_TRUE.
 * This is useful for a one-time scan if continuous monitoring is not required.
 *
 * @return EINA_TRUE if the scan was successful or if watching is active.
 *         EINA_FALSE if parsing /etc/fstab failed.
 */
EAPI Eina_Bool
eeze_mount_fstab_scan(void)
{
   libmnt_table *bak;
   if (_watching)
     return EINA_TRUE;

   bak = _eeze_mount_tab_parse("/etc/fstab");
   if (!bak)
     goto error;
   if (_eeze_mount_fstab)
     {
        mnt_free_table(_eeze_mount_fstab);
        mnt_free_cache(_eeze_mount_fstab_cache);
     }
   _eeze_mount_fstab = bak;
   _eeze_mount_fstab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_fstab, _eeze_mount_fstab_cache);

   return EINA_TRUE;

error:
   return EINA_FALSE;
}
