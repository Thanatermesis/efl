#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_UNSTABLE_LIBMOUNT_API
# define USE_UNSTABLE_LIBMOUNT_API 1
#endif

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#include <libmount.h>
#include <unistd.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

/*
 *
 * PRIVATE
 *
 */

/**
 * @brief Mapping of mount option strings to Eeze_Disk_Mountopt flags.
 * This array is used by libmount's mnt_optstr_get_flags function to parse
 * mount option strings.
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
typedef struct libmnt_table libmnt_table; /**< Typedef for libmount's table structure. */
typedef struct libmnt_lock libmnt_lock;   /**< Typedef for libmount's lock structure. */
typedef struct libmnt_fs libmnt_fs;       /**< Typedef for libmount's filesystem structure. */
typedef struct libmnt_cache libmnt_cache; /**< Typedef for libmount's cache structure. */

static Ecore_File_Monitor *_mtab_mon = NULL; /**< Ecore file monitor for /etc/mtab. */
static Ecore_File_Monitor *_fstab_mon = NULL; /**< Ecore file monitor for /etc/fstab. */
static Eina_Bool _watching = EINA_FALSE; /**< Flag indicating if mtab/fstab watching is active. */
static Eina_Bool _mtab_scan_active = EINA_FALSE; /**< Flag to prevent re-entrant mtab scans. */
static Eina_Bool _mtab_locked = EINA_FALSE; /**< Flag indicating if mtab is currently locked. */
static Eina_Bool _fstab_scan_active = EINA_FALSE; /**< Flag to prevent re-entrant fstab scans. */
static libmnt_cache *_eeze_mount_mtab_cache = NULL; /**< Libmount cache for mtab. */
static libmnt_cache *_eeze_mount_fstab_cache = NULL; /**< Libmount cache for fstab. */
static libmnt_table *_eeze_mount_mtab = NULL; /**< Libmount table for mtab entries. */
static libmnt_table *_eeze_mount_fstab = NULL; /**< Libmount table for fstab entries. */
static libmnt_lock *_eeze_mtab_lock = NULL; /**< Libmount lock for mtab. */
extern Eina_List *_eeze_disks; /**< External list of known Eeze_Disk objects. */

static libmnt_table *_eeze_mount_tab_parse(const char *filename);
static void _eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon EINA_UNUSED, Ecore_File_Event event EINA_UNUSED, const char *path);

/**
 * @brief Locks the mtab file.
 * This function attempts to acquire a lock on /etc/mtab.
 * It checks for write access first and proceeds without a lock if permissions are insufficient.
 * @return EINA_TRUE if the lock was acquired or if locking was skipped due to permissions,
 *         EINA_FALSE if locking failed.
 */
static Eina_Bool
_eeze_mount_lock_mtab(void)
{
//    DBG("Locking mlock: %s", mnt_lock_get_linkfile(_eeze_mtab_lock));
    if (EINA_LIKELY(!eina_file_access("/etc/mtab", EINA_FILE_ACCESS_MODE_WRITE)))
      {
         INF("Insufficient privs for mtab lock, continuing without lock");
         return EINA_TRUE;
      }
    if (mnt_lock_file(_eeze_mtab_lock))
     {
        ERR("Couldn't lock mtab!");
        return EINA_FALSE;
     }
   _mtab_locked = EINA_TRUE;
   return EINA_TRUE;
}

/**
 * @brief Unlocks the mtab file.
 * This function releases the lock on /etc/mtab if it was previously acquired.
 */
static void
_eeze_mount_unlock_mtab(void)
{
//   DBG("Unlocking mlock: %s", mnt_lock_get_linkfile(_eeze_mtab_lock));
   if (_mtab_locked) mnt_unlock_file(_eeze_mtab_lock);
   _mtab_locked = EINA_FALSE;
}

/**
 * @brief Callback function for libmount table parsing errors.
 * This function is called by libmount when an error occurs while parsing
 * a mount table file (e.g., /etc/mtab or /etc/fstab).
 * @param tab The libmount table being parsed (unused).
 * @param filename The name of the file where the error occurred.
 * @param line The line number in the file where the error occurred.
 * @return Always returns -1 to indicate an error.
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
 * @brief Parses a mount table file (e.g., /etc/mtab, /etc/fstab) into a libmnt_table.
 * This function creates a new libmount table and parses the specified file into it.
 * It sets a custom error callback for more detailed error reporting.
 * @param filename The path to the mount table file to parse.
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
 * @brief Callback function for Ecore file monitor events on /etc/mtab or /etc/fstab.
 * This function is triggered when /etc/mtab or /etc/fstab changes. It re-parses
 * the modified file and updates the internal cache. For mtab changes, it also
 * checks for externally initiated mounts/unmounts and emits corresponding Eeze events.
 * @param data User data passed to the callback. Non-NULL for mtab, NULL for fstab.
 *             Used to differentiate which file triggered the event and to avoid
 *             re-entrant scans.
 * @param mon The Ecore_File_Monitor that triggered the event (unused).
 * @param event The type of file event that occurred (unused).
 * @param path The path to the file that changed (either /etc/mtab or /etc/fstab).
 */
static void
_eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon EINA_UNUSED, Ecore_File_Event event EINA_UNUSED, const char *path)
{
   libmnt_table *bak;

   if (
       ((_mtab_scan_active) && (data)) || /* mtab has non-null data to avoid needing strcmp */
       ((_fstab_scan_active) && (!data))
      )
     /* prevent scans from triggering a scan */
     return;

   bak = _eeze_mount_mtab;
   if (data)
     if (!_eeze_mount_lock_mtab())
       {  /* FIXME: maybe queue job here? */
          ERR("Losing events...");
          return;
       }
   _eeze_mount_mtab = _eeze_mount_tab_parse(path);
   if (data)
     _eeze_mount_unlock_mtab();
   if (!_eeze_mount_mtab)
     {
        ERR("Could not parse %s! keeping old tab...", path);
        goto error;
     }
   if (data)
     {
        Eina_List *l;
        Eeze_Disk *disk;

        /* catch externally initiated mounts on existing disks by comparing known mount state to current state */
        EINA_LIST_FOREACH(_eeze_disks, l, disk)
          {
             Eina_Bool mounted;

             mounted = disk->mounted;

             if ((eeze_disk_libmount_mounted_get(disk) != mounted) && (!disk->mount_status))
               {
                  if (!mounted)
                    {
                        Eeze_Event_Disk_Mount *e;
                        e = malloc(sizeof(Eeze_Event_Disk_Mount));
                        if (e)
                          {
                             e->disk = disk;
                             ecore_event_add(EEZE_EVENT_DISK_MOUNT, e, NULL, NULL);
                          }
                    }
                  else
                    {
                       Eeze_Event_Disk_Unmount *e;
                       e = malloc(sizeof(Eeze_Event_Disk_Unmount));
                       if (e)
                         {
                            e->disk = disk;
                            ecore_event_add(EEZE_EVENT_DISK_UNMOUNT, e, NULL, NULL);
                         }
                    }
               }
          }
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

/*
 *
 * INVISIBLE
 *
 */

/**
 * @brief Initializes the libmount integration for Eeze.
 * This function creates a new libmount lock for /etc/mtab. It should be called
 * once during Eeze initialization if libmount support is enabled.
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise (e.g., if
 *         the mtab lock cannot be created).
 * @note This function is idempotent; subsequent calls after successful initialization
 *       will return EINA_TRUE without re-initializing.
 */
Eina_Bool
eeze_libmount_init(void)
{
   if (_eeze_mtab_lock)
     return EINA_TRUE;
   if (!(_eeze_mtab_lock = mnt_new_lock("/etc/mtab", 0)))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Shuts down the libmount integration for Eeze.
 * This function frees resources allocated for libmount integration, including
 * mtab/fstab tables, caches, and the mtab lock. It also stops watching
 * mtab/fstab for changes. It should be called once during Eeze shutdown.
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
   if (!_eeze_mtab_lock)
     return;

   mnt_unlock_file(_eeze_mtab_lock);
   mnt_free_lock(_eeze_mtab_lock);
   _eeze_mtab_lock = NULL;
}

/**
 * @brief Retrieves the mount options for a given disk.
 * This function queries the parsed mtab and fstab data to find the mount
 * options associated with the provided Eeze_Disk. It searches by UUID.
 * @param disk The Eeze_Disk object to get mount options for.
 * @return A bitmask of EEZE_DISK_MOUNTOPT flags representing the mount options.
 *         Returns 0 if the disk is not found, has no options, or if mtab/fstab
 *         scanning fails.
 * @see eeze_optmap for the mapping of option strings to flags.
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
 * @brief Checks if a given disk is currently mounted.
 * This function consults the parsed mtab data to determine if the specified
 * Eeze_Disk (identified by its device path) is listed as mounted.
 * If mounted, it updates the disk's mount_point and mounted status.
 * @param disk The Eeze_Disk object to check.
 * @return EINA_TRUE if the disk is mounted, EINA_FALSE otherwise or if an error occurs
 *         (e.g., disk is NULL, mtab scan fails).
 */
Eina_Bool
eeze_disk_libmount_mounted_get(Eeze_Disk *disk)
{
   libmnt_fs *mnt;

   if (!disk)
     return EINA_FALSE;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return EINA_FALSE;

   mnt = mnt_table_find_srcpath(_eeze_mount_mtab, eeze_disk_devpath_get(disk), MNT_ITER_BACKWARD);
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
 * This function searches the parsed mtab and fstab data to find the
 * device path (e.g., "/dev/sda1") that is mounted at the specified `mount_point`.
 * @param mount_point The mount point to search for (e.g., "/media/usb").
 * @return The source device path as a stringshared string if found,
 *         NULL otherwise or if an error occurs (e.g., mount_point is NULL,
 *         mtab/fstab scan fails).
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
 * @brief Looks up the mount point associated with a given UUID.
 * This function searches the parsed fstab data to find the mount point
 * configured for the specified `uuid`.
 * @param uuid The UUID to search for.
 * @return The mount point as a stringshared string if found in fstab,
 *         NULL otherwise or if an error occurs (e.g., uuid is NULL,
 *         fstab scan fails).
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
 * @brief Looks up the mount point associated with a given filesystem label.
 * This function searches the parsed fstab data to find the mount point
 * configured for the specified `label`.
 * @param label The filesystem label to search for.
 * @return The mount point as a stringshared string if found in fstab,
 *         NULL otherwise or if an error occurs (e.g., label is NULL,
 *         fstab scan fails).
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
 * @brief Looks up the mount point associated with a given device path.
 * This function searches the parsed mtab and then fstab data to find the
 * mount point for the specified `devpath`.
 * @param devpath The device path to search for (e.g., "/dev/sda1").
 * @return The mount point as a stringshared string if found,
 *         NULL otherwise or if an error occurs (e.g., devpath is NULL,
 *         mtab/fstab scan fails).
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
 * @brief Starts watching /etc/mtab and /etc/fstab for changes.
 * This function parses the current mtab and fstab, caches them, and sets up
 * Ecore file monitors to detect modifications to these files.
 * If already watching, this function returns EINA_TRUE without re-initializing.
 * @return EINA_TRUE if watching was successfully started or was already active.
 *         EINA_FALSE if an error occurred (e.g., mtab/fstab parsing failed,
 *         mtab lock failed).
 * @see _eeze_mount_tab_watcher
 */
EAPI Eina_Bool
eeze_mount_tabs_watch(void)
{
   libmnt_table *bak;

   if (_watching)
     return EINA_TRUE;

   if (!_eeze_mount_lock_mtab())
     return EINA_FALSE;

   bak = _eeze_mount_tab_parse("/etc/mtab");
   _eeze_mount_unlock_mtab();
   if (!bak)
     goto error;

   mnt_free_table(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
   if (!(bak = _eeze_mount_tab_parse("/etc/fstab")))
     goto error;

   mnt_free_table(_eeze_mount_fstab);
   _eeze_mount_fstab = bak;

   _eeze_mount_mtab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_mtab, _eeze_mount_mtab_cache);

   _eeze_mount_fstab_cache = mnt_new_cache();
   mnt_table_set_cache(_eeze_mount_fstab, _eeze_mount_fstab_cache);

   _mtab_mon = ecore_file_monitor_add("/etc/mtab", _eeze_mount_tab_watcher, (void*)1);
   _fstab_mon = ecore_file_monitor_add("/etc/fstab", _eeze_mount_tab_watcher, NULL);
   _watching = EINA_TRUE;

  return EINA_TRUE;

error:
   if (!_eeze_mount_mtab)
     ERR("Could not parse /etc/mtab!");
   else
     {
        ERR("Could not parse /etc/fstab!");
        mnt_free_table(_eeze_mount_mtab);
     }
   return EINA_FALSE;
}

/**
 * @brief Stops watching /etc/mtab and /etc/fstab for changes.
 * This function removes the Ecore file monitors for mtab and fstab.
 * If not currently watching, this function does nothing.
 */
EAPI void
eeze_mount_tabs_unwatch(void)
{
   if (!_watching)
     return;

   ecore_file_monitor_del(_mtab_mon);
   _mtab_mon = NULL;
   ecore_file_monitor_del(_fstab_mon);
   _fstab_mon = NULL;
   _watching = EINA_FALSE;
}

/**
 * @brief Manually scans and updates the internal /etc/mtab representation.
 * This function is typically used when not actively watching mtab (i.e.,
 * eeze_mount_tabs_watch() has not been called or _watching is false).
 * It locks mtab, re-parses it, and updates the internal cache.
 * If mtab is already being watched, this function returns EINA_TRUE without
 * performing a scan.
 * @return EINA_TRUE if the scan was successful or if mtab is already being watched.
 *         EINA_FALSE if an error occurred (e.g., mtab lock failed, parsing failed).
 */
EAPI Eina_Bool
eeze_mount_mtab_scan(void)
{
   libmnt_table *bak;

   if (_watching)
     return EINA_TRUE;

   if (!_eeze_mount_lock_mtab())
     return EINA_FALSE;
   bak = _eeze_mount_tab_parse("/etc/mtab");
   _eeze_mount_unlock_mtab();
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
 * @brief Manually scans and updates the internal /etc/fstab representation.
 * This function is typically used when not actively watching fstab (i.e.,
 * eeze_mount_tabs_watch() has not been called or _watching is false).
 * It re-parses fstab and updates the internal cache.
 * If fstab is already being watched, this function returns EINA_TRUE without
 * performing a scan.
 * @return EINA_TRUE if the scan was successful or if fstab is already being watched.
 *         EINA_FALSE if parsing fstab failed.
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
