#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef USE_UNSTABLE_LIBMOUNT_API
# define USE_UNSTABLE_LIBMOUNT_API 1
#endif

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#include <mount/mount.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"
/*
 *
 * PRIVATE
 *
 */
static Ecore_File_Monitor *_mtab_mon = NULL; /**< Ecore file monitor for /etc/mtab */
static Ecore_File_Monitor *_fstab_mon = NULL; /**< Ecore file monitor for /etc/fstab */
static Eina_Bool _watching = EINA_FALSE; /**< Flag indicating if mtab/fstab are being watched */
static Eina_Bool _mtab_scan_active = EINA_FALSE; /**< Flag to prevent recursive mtab scanning */
static Eina_Bool _fstab_scan_active = EINA_FALSE; /**< Flag to prevent recursive fstab scanning */
static mnt_tab *_eeze_mount_mtab = NULL; /**< Parsed representation of /etc/mtab */
static mnt_tab *_eeze_mount_fstab = NULL; /**< Parsed representation of /etc/fstab */
static mnt_lock *_eeze_mtab_lock = NULL; /**< Lock for accessing /etc/mtab */
extern Eina_List *_eeze_disks; /**< External list of known disks */

/**
 * @brief Parses a mount table file (e.g., /etc/mtab, /etc/fstab).
 *
 * This function uses libmount to parse the specified file and returns a
 * handle to the parsed table. It provides detailed error reporting.
 *
 * @param filename The full path to the mount table file.
 * @return A pointer to the mnt_tab structure on success, NULL on failure.
 */
static mnt_tab *_eeze_mount_tab_parse(const char *filename);
/**
 * @brief Callback function for Ecore_File_Monitor on mtab/fstab changes.
 *
 * This function is triggered when /etc/mtab or /etc/fstab changes.
 * It re-parses the modified file and updates the internal representation.
 * For mtab changes, it also checks for externally initiated mounts/unmounts
 * and emits corresponding Eeze events.
 *
 * @param data User data passed to the callback. (void*)1 for mtab, NULL for fstab.
 * @param mon The Ecore_File_Monitor that triggered the event (unused).
 * @param event The type of file event (unused).
 * @param path The path to the modified file.
 */
static void _eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon __UNUSED__, Ecore_File_Event event __UNUSED__, const char *path);

/**
 * @brief Locks the mtab file for safe access.
 *
 * Uses libmount's locking mechanism to prevent race conditions when
 * reading or modifying mtab.
 *
 * @note The actual locking call `mnt_lock_file` is currently disabled
 *       due to issues with libmount.
 * @return EINA_TRUE if locking was (notionally) successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_eeze_mount_lock_mtab(void)
{
    DBG("Locking mlock: %s", mnt_lock_get_linkfile(_eeze_mtab_lock));
#if 0
#warning this code is broken with current libmount!
    if (mnt_lock_file(_eeze_mtab_lock))
     {
        ERR("Couldn't lock mtab!");
        return EINA_FALSE;
     }
#endif
   return EINA_TRUE;
}

/**
 * @brief Unlocks the mtab file.
 *
 * Releases the lock obtained by _eeze_mount_lock_mtab().
 */
static void
_eeze_mount_unlock_mtab(void)
{
   DBG("Unlocking mlock: %s", mnt_lock_get_linkfile(_eeze_mtab_lock));
   mnt_unlock_file(_eeze_mtab_lock);
}

/*
 * I could use mnt_new_tab_from_file() but this way gives much more detailed output
 * on failure so why not
 */
static mnt_tab *
_eeze_mount_tab_parse(const char *filename)
{
   mnt_tab *tab;

   if (!(tab = mnt_new_tab(filename)))
     return NULL;
   if (!mnt_tab_parse_file(tab))
     return tab;

   if (mnt_tab_get_nerrs(tab))
     {  /* parse error */
        char buf[1024];

        mnt_tab_strerror(tab, buf, sizeof(buf));
        ERR("%s", buf);
     }
   else
     /* system error */
     ERR("%s", mnt_tab_get_name(tab));
   mnt_free_tab(tab);
   return NULL;
}

static void
_eeze_mount_tab_watcher(void *data, Ecore_File_Monitor *mon __UNUSED__, Ecore_File_Event event __UNUSED__, const char *path)
{
   mnt_tab *bak;

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

   mnt_free_tab(bak);
   return;

error:
   mnt_free_tab(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
}

/*
 *
 * INVISIBLE
 *
 */

/**
 * @brief Initializes the libmount integration for Eeze.
 *
 * Sets up the mtab lock. This function should be called once during
 * Eeze initialization if libmount features are to be used.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., lock creation failed).
 */
Eina_Bool
eeze_libmount_init(void)
{
   if (_eeze_mtab_lock)
     return EINA_TRUE;
   if (!(_eeze_mtab_lock = mnt_new_lock(NULL, 0)))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Shuts down the libmount integration for Eeze.
 *
 * Releases the mtab lock and frees associated resources. This function
 * should be called during Eeze shutdown.
 */
void
eeze_libmount_shutdown(void)
{
   if (!_eeze_mtab_lock)
     return;

   mnt_unlock_file(_eeze_mtab_lock);
   mnt_free_lock(_eeze_mtab_lock);
   _eeze_mtab_lock = NULL;
}

/**
 * @brief Checks if a given Eeze_Disk is currently mounted.
 *
 * This function queries the internal mtab representation (after ensuring it's up-to-date)
 * to determine if the specified disk (by its device path) is mounted.
 * If mounted, it also updates the disk's mount_point field.
 *
 * @param disk Pointer to the Eeze_Disk structure to check.
 * @return EINA_TRUE if the disk is mounted, EINA_FALSE otherwise or on error.
 */
Eina_Bool
eeze_disk_libmount_mounted_get(Eeze_Disk *disk)
{
   mnt_fs *mnt;

   if (!disk)
     return EINA_FALSE;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return EINA_FALSE;

   mnt = mnt_tab_find_srcpath(_eeze_mount_mtab, eeze_disk_devpath_get(disk), MNT_ITER_BACKWARD);
   if (!mnt)
     {
        disk->mounted = EINA_FALSE;
        return EINA_FALSE;
     }

   disk->mount_point = eina_stringshare_add(mnt_fs_get_target(mnt));
   disk->mounted = EINA_TRUE;
   return EINA_TRUE;
}


/**
 * @brief Finds the source device path for a given mount point.
 *
 * Searches both mtab and fstab for the specified mount point and returns
 * the corresponding source device path (e.g., "/dev/sda1").
 *
 * @param mount_point The mount point to look up (e.g., "/media/usb").
 * @return A stringshared pointer to the source device path if found, NULL otherwise.
 *         The caller should not free this string.
 */
const char *
eeze_disk_libmount_mp_find_source(const char *mount_point)
{
   mnt_fs *mnt;

   if (!mount_point)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_tab_find_target(_eeze_mount_mtab, mount_point, MNT_ITER_BACKWARD);
   if (!mnt)
     mnt = mnt_tab_find_target(_eeze_mount_fstab, mount_point, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_source(mnt);
}

/**
 * @brief Looks up a mount point in fstab by its filesystem UUID.
 *
 * @param uuid The UUID of the filesystem to search for.
 * @return A stringshared pointer to the mount point if found in fstab, NULL otherwise.
 *         The caller should not free this string.
 */
const char *
eeze_disk_libmount_mp_lookup_by_uuid(const char *uuid)
{
   mnt_fs *mnt;

   if (!uuid)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_tab_find_tag(_eeze_mount_fstab, "UUID", uuid, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_target(mnt);
}

/**
 * @brief Looks up a mount point in fstab by its filesystem LABEL.
 *
 * @param label The LABEL of the filesystem to search for.
 * @return A stringshared pointer to the mount point if found in fstab, NULL otherwise.
 *         The caller should not free this string.
 */
const char *
eeze_disk_libmount_mp_lookup_by_label(const char *label)
{
   mnt_fs *mnt;

   if (!label)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_tab_find_tag(_eeze_mount_fstab, "LABEL", label, MNT_ITER_BACKWARD);

   if (!mnt)
     return NULL;

   return mnt_fs_get_target(mnt);
}

/**
 * @brief Looks up a mount point by its device path.
 *
 * Searches mtab first, then fstab, for an entry corresponding to the given
 * device path (e.g., "/dev/sdb1").
 *
 * @param devpath The device path to search for.
 * @return A stringshared pointer to the mount point if found, NULL otherwise.
 *         The caller should not free this string.
 */
const char *
eeze_disk_libmount_mp_lookup_by_devpath(const char *devpath)
{
   mnt_fs *mnt;

   if (!devpath)
     return NULL;

   if (!eeze_mount_mtab_scan() || !eeze_mount_fstab_scan())
     return NULL;

   mnt = mnt_tab_find_srcpath(_eeze_mount_mtab, devpath, MNT_ITER_BACKWARD);
   if (!mnt)
     mnt = mnt_tab_find_srcpath(_eeze_mount_fstab, devpath, MNT_ITER_BACKWARD);

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
 *
 * Parses the initial state of mtab and fstab, then sets up ecore file monitors
 * to detect subsequent changes. If already watching, this function does nothing.
 *
 * @return EINA_TRUE if watching was started successfully or was already active.
 *         EINA_FALSE if there was an error parsing initial mtab/fstab or
 *         setting up monitors.
 */
EAPI Eina_Bool
eeze_mount_tabs_watch(void)
{
   mnt_tab *bak;

   if (_watching)
     return EINA_TRUE;

   if (!_eeze_mount_lock_mtab())
     return EINA_FALSE;

   bak = _eeze_mount_tab_parse("/etc/mtab");
   _eeze_mount_unlock_mtab();
   if (!bak)
     goto error;

   mnt_free_tab(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
   if (!(bak = _eeze_mount_tab_parse("/etc/fstab")))
     goto error;

   mnt_free_tab(_eeze_mount_fstab);
   _eeze_mount_fstab = bak;

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
        mnt_free_tab(_eeze_mount_mtab);
     }
   return EINA_FALSE;
}

/**
 * @brief Stops watching /etc/mtab and /etc/fstab for changes.
 *
 * Removes the ecore file monitors. If not currently watching,
 * this function does nothing.
 */
EAPI void
eeze_mount_tabs_unwatch(void)
{
   if (!_watching)
     return;

   ecore_file_monitor_del(_mtab_mon);
   ecore_file_monitor_del(_fstab_mon);
}

/**
 * @brief Manually scans /etc/mtab and updates the internal representation.
 *
 * This function is typically used when not actively watching mtab (i.e.,
 * eeze_mount_tabs_watch() has not been called or _watching is false).
 * It locks, parses, and unlocks mtab.
 *
 * @return EINA_TRUE if the scan was successful and mtab was updated.
 *         EINA_FALSE if there was an error (e.g., couldn't lock or parse mtab).
 *         If already watching mtab, returns EINA_TRUE without re-scanning.
 */
EAPI Eina_Bool
eeze_mount_mtab_scan(void)
{
   mnt_tab *bak;

   if (_watching)
     return EINA_TRUE;

   if (!_eeze_mount_lock_mtab())
     return EINA_FALSE;
   bak = _eeze_mount_tab_parse("/etc/mtab");
   _eeze_mount_unlock_mtab();
   if (!bak)
     goto error;
   if (_eeze_mount_mtab)
     mnt_free_tab(_eeze_mount_mtab);
   _eeze_mount_mtab = bak;
   return EINA_TRUE;

error:
   return EINA_FALSE;
}

/**
 * @brief Manually scans /etc/fstab and updates the internal representation.
 *
 * This function is typically used when not actively watching fstab (i.e.,
 * eeze_mount_tabs_watch() has not been called or _watching is false).
 *
 * @return EINA_TRUE if the scan was successful and fstab was updated.
 *         EINA_FALSE if there was an error (e.g., couldn't parse fstab).
 *         If already watching fstab, returns EINA_TRUE without re-scanning.
 */
EAPI Eina_Bool
eeze_mount_fstab_scan(void)
{
   mnt_tab *bak;
   if (_watching)
     return EINA_TRUE;

   bak = _eeze_mount_tab_parse("/etc/fstab");
   if (!bak)
     goto error;
   if (_eeze_mount_fstab)
     mnt_free_tab(_eeze_mount_fstab);
   _eeze_mount_fstab = bak;

   return EINA_TRUE;

error:
   return EINA_FALSE;
}
