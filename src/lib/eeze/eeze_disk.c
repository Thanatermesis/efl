#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>
#include <unistd.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

/**
 * @internal
 * @brief Log domain for the Eeze_Disk module.
 */
int _eeze_disk_log_dom = -1;
/**
 * @internal
 * @brief List of all currently tracked Eeze_Disk objects.
 */
Eina_List *_eeze_disks = NULL;

/**
 * @internal
 * @brief Determines the type of a disk.
 *
 * This function inspects various udev properties of the disk and its children
 * to determine if it's a CD-ROM, internal drive, USB drive, flash drive, etc.
 *
 * @param disk The Eeze_Disk object to analyze.
 * @return The determined Eeze_Disk_Type.
 */
static Eeze_Disk_Type
_eeze_disk_type_find(Eeze_Disk *disk)
{
   const char *test;
   Eeze_Disk_Type ret;
   Eina_Bool filesystem = EINA_FALSE; /* this will have no children */

   if (udev_device_get_property_value(disk->device, "ID_CDROM"))
     return EEZE_DISK_TYPE_CDROM;
   test = udev_device_get_property_value(disk->device, "ID_FS_USAGE");
   if ((!test) || strcmp(test, "filesystem"))
     {
        test = _walk_children_get_attr(disk->syspath, "ID_CDROM", "block", EINA_TRUE);
        if (test)
          {
             eina_stringshare_del(test);
             return EEZE_DISK_TYPE_CDROM;
          }
     }
   else
     filesystem = EINA_TRUE;
   if (udev_device_get_property_value(disk->device, "ID_ATA"))
     return EEZE_DISK_TYPE_INTERNAL;
   if (!filesystem)
     {
        test = _walk_children_get_attr(disk->syspath, "ID_ATA", "block", EINA_TRUE);
        if (test)
          {
             eina_stringshare_del(test);
             return EEZE_DISK_TYPE_INTERNAL;
          }
     }
   test = udev_device_get_property_value(disk->device, "ID_BUS");
   if (test)
     {
        if (!strcmp(test, "ata")) return EEZE_DISK_TYPE_INTERNAL;
        if (!strcmp(test, "usb")) return EEZE_DISK_TYPE_USB;
        return EEZE_DISK_TYPE_UNKNOWN; /* FIXME */
     }
   if (!filesystem)
     test = _walk_children_get_attr(disk->syspath, "ID_BUS", "block", EINA_TRUE);
   if (!test)
     {
        _udev_device *dev;

        for (dev = udev_device_get_parent(disk->device); dev; dev = udev_device_get_parent(dev))
          {
             test = udev_device_get_subsystem(dev);
             if (!test) return EEZE_DISK_TYPE_UNKNOWN;
             if (!strcmp(test, "block")) continue;
             if (!strcmp(test, "mmc")) return EEZE_DISK_TYPE_FLASH;
             break;
          }
        return EEZE_DISK_TYPE_UNKNOWN;  /* FIXME */
     }

   if (!strcmp(test, "ata")) ret = EEZE_DISK_TYPE_INTERNAL;
   else if (!strcmp(test, "usb")) ret = EEZE_DISK_TYPE_USB;
   else ret = EEZE_DISK_TYPE_UNKNOWN; /* FIXME */

   eina_stringshare_del(test);

   return ret;
}

/**
 * @internal
 * @brief Finds a udev device based on a filesystem property (UUID or label).
 *
 * This function enumerates block devices and matches them against the provided
 * property value.
 *
 * @param prop The property value to search for (e.g., a UUID string or a label string).
 * @param uuid EINA_TRUE if @p prop is a UUID, EINA_FALSE if it's a label.
 * @return A pointer to the found _udev_device, or NULL if not found or on error.
 *         The caller is responsible for unreferencing the returned device.
 */
static _udev_device *
_eeze_disk_device_from_property(const char *prop, Eina_Bool uuid)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   _udev_device *device = NULL;
   const char *devname;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   if (uuid)
     udev_enumerate_add_match_property(en, "ID_FS_UUID", prop);
   else
     udev_enumerate_add_match_property(en, "ID_FS_LABEL", prop);
   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);
        device = udev_device_new_from_syspath(udev, devname);
        break;
     }
   udev_enumerate_unref(en);
   return device;

}

/**
 * @brief Shuts down the Eeze_Disk subsystem.
 *
 * This function cleans up resources used by the Eeze_Disk module,
 * including shutting down the eeze_mount and ecore_file subsystems
 * and unregistering the log domain.
 */
void
eeze_disk_shutdown(void)
{
   eeze_mount_shutdown();
   ecore_file_shutdown();
   eina_log_domain_unregister(_eeze_disk_log_dom);
   _eeze_disk_log_dom = -1;
}

/**
 * @brief Initializes the Eeze_Disk subsystem.
 *
 * This function sets up the necessary components for Eeze_Disk to operate,
 * including registering a log domain and initializing ecore_file and eeze_mount.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
eeze_disk_init(void)
{
   _eeze_disk_log_dom = eina_log_domain_register("eeze_disk", EINA_COLOR_LIGHTBLUE);

   if (_eeze_disk_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register 'eeze_disk' log domain.");
        goto disk_fail;
     }

   if (!ecore_file_init())
     goto disk_fail;
   if  (!eeze_mount_init())
     goto ecore_file_fail;

   return EINA_TRUE;

ecore_file_fail:
   ecore_file_shutdown();
disk_fail:
   eina_log_domain_unregister(_eeze_disk_log_dom);
   _eeze_disk_log_dom = -1;
   return EINA_FALSE;
}

/**
 * @brief A placeholder or example function for Eeze_Disk.
 * @since 1.23
 *
 * This function currently does nothing. It might be a template or
 * a function reserved for future use.
 */
EAPI void
eeze_disk_function(void)
{
}

/**
 * @brief Creates a new Eeze_Disk object from a device path or syspath.
 *
 * If @p path starts with "/dev/", it's treated as a device node path (e.g., "/dev/sda1").
 * Otherwise, it's treated as a syspath (e.g., "/sys/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0/block/sda/sda1").
 *
 * @param path The device path or syspath of the disk.
 * @return A pointer to the newly created Eeze_Disk object, or NULL on failure.
 *         The caller is responsible for freeing the returned object using eeze_disk_free().
 */
EAPI Eeze_Disk *
eeze_disk_new(const char *path)
{
   Eeze_Disk *disk;
   _udev_device *dev;
   const char *syspath = NULL;
   Eina_Bool is_dev = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   if (!strncmp(path, "/dev/", 5))
     {
        is_dev = EINA_TRUE;
        syspath = eeze_udev_devpath_get_syspath(path);
        if (!syspath)
          return NULL;

        if (!(dev = _new_device(syspath)))
          {
             eina_stringshare_del(syspath);
             return NULL;
          }
     }
   else if (!(dev = _new_device(path)))
     return NULL;

   if (!(disk = calloc(1, sizeof(Eeze_Disk))))
     {
        eina_stringshare_del(syspath);
        udev_device_unref(dev);
        return NULL;
     }

   if (is_dev)
     {
        disk->devpath = eina_stringshare_add(path);
        disk->syspath = syspath;
     }
   else
     disk->syspath = eina_stringshare_add(udev_device_get_syspath(dev));

   disk->device = dev;
   disk->mount_opts = EEZE_DISK_MOUNTOPT_DEFAULTS;
   disk->mount_cmd_changed = EINA_TRUE;
   disk->unmount_cmd_changed = EINA_TRUE;

   _eeze_disks = eina_list_append(_eeze_disks, disk);

   return disk;
}

/**
 * @brief Creates a new Eeze_Disk object from a mount point.
 *
 * This function identifies the source device associated with the given
 * @p mount_point (e.g., "/media/usb_stick") and creates an Eeze_Disk object for it.
 * It can resolve the source device from UUID, LABEL, or device path.
 *
 * @param mount_point The mount point string (e.g., "/mnt/my_disk").
 * @return A pointer to the newly created Eeze_Disk object, or NULL on failure or if the mount point is not found.
 *         The caller is responsible for freeing the returned object using eeze_disk_free().
 */
EAPI Eeze_Disk *
eeze_disk_new_from_mount(const char *mount_point)
{
   Eeze_Disk *disk = NULL;
   _udev_device *dev = NULL;
   const char *source, *uuid = NULL, *label = NULL, *devpath = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(mount_point, NULL);

   if (!(source = eeze_disk_libmount_mp_find_source(mount_point)))
     return NULL;

   if (source[4] == '=')
     {
        source += 5;
        uuid = eina_stringshare_add(source);
        dev = _eeze_disk_device_from_property(uuid, EINA_TRUE);
     }
   else if (source[5] == '=')
     {
        source += 6;
        label = eina_stringshare_add(source);
        dev = _eeze_disk_device_from_property(label, EINA_FALSE);
     }
   else
     {
        const char *spath;

        devpath = eina_stringshare_add(source);
        spath = eeze_udev_devpath_get_syspath(devpath);
        dev = _new_device(spath);
        eina_stringshare_del(spath);
     }

   if (!dev)
     goto error;

   if (!(disk = calloc(1, sizeof(Eeze_Disk))))
     goto error;

   disk->syspath = udev_device_get_syspath(dev);

   disk->device = dev;
   disk->mount_cmd_changed = EINA_TRUE;
   disk->unmount_cmd_changed = EINA_TRUE;
   if (uuid)
     disk->cache.uuid = uuid;
   else if (label)
     disk->cache.label = label;
   else
     disk->devpath = devpath;
   disk->mount_point = eina_stringshare_add(mount_point);

   _eeze_disks = eina_list_append(_eeze_disks, disk);

   return disk;
error:
   if (uuid)
     eina_stringshare_del(uuid);
   else if (label)
     eina_stringshare_del(label);
   else if (devpath)
     eina_stringshare_del(devpath);
   if (dev)
     udev_device_unref(dev);
   return NULL;
}

/**
 * @brief Frees an Eeze_Disk object.
 *
 * This function releases all resources associated with the given @p disk,
 * including unreferencing its udev device, freeing command string buffers,
 * killing any associated mounter process, and removing it from internal lists.
 *
 * @param disk The Eeze_Disk object to free.
 */
EAPI void
eeze_disk_free(Eeze_Disk *disk)
{
   extern Eina_List *eeze_events;
   EINA_SAFETY_ON_NULL_RETURN(disk);

   udev_device_unref(disk->device);
   if (disk->mount_cmd)
     eina_strbuf_free(disk->mount_cmd);
   if (disk->unmount_cmd)
     eina_strbuf_free(disk->unmount_cmd);
   if (disk->eject_cmd)
     eina_strbuf_free(disk->eject_cmd);
   if (disk->mounter) ecore_exe_kill(disk->mounter);
   _eeze_disks = eina_list_remove(_eeze_disks, disk);
   eeze_events = eina_list_remove(eeze_events, disk);
   free(disk);
}

/**
 * @brief Scans and caches properties of an Eeze_Disk object.
 *
 * This function populates the internal cache of the @p disk with properties
 * like vendor, model, serial number, UUID, label, type, and removability.
 * The scan is performed only once; subsequent calls for a disk with
 * a filled cache will do nothing.
 *
 * @param disk The Eeze_Disk object to scan.
 */
EAPI void
eeze_disk_scan(Eeze_Disk *disk)
{
   const char *test;
   EINA_SAFETY_ON_NULL_RETURN(disk);
   /* never rescan; if these values change then something is seriously wrong */
   if (disk->cache.filled) return;

   if (!disk->cache.vendor)
     disk->cache.vendor = udev_device_get_property_value(disk->device, "ID_VENDOR");
   if (!disk->cache.vendor)
     disk->cache.vendor = udev_device_get_sysattr_value(disk->device, "vendor");
   if (!disk->cache.model)
     disk->cache.model = udev_device_get_property_value(disk->device, "ID_MODEL");
   if (!disk->cache.model)
     disk->cache.model = udev_device_get_sysattr_value(disk->device, "model");
   if (!disk->cache.serial)
     disk->cache.serial = udev_device_get_property_value(disk->device, "ID_SERIAL_SHORT");
   if (!disk->cache.uuid)
     disk->cache.uuid = udev_device_get_property_value(disk->device, "ID_FS_UUID");
   if (!disk->cache.type)
     disk->cache.type = _eeze_disk_type_find(disk);
   if (!disk->cache.label)
     disk->cache.label = udev_device_get_property_value(disk->device, "ID_FS_LABEL");
   test = udev_device_get_sysattr_value(disk->device, "removable");
   if (test) disk->cache.removable = !!strtol(test, NULL, 10);
   else
     test = _walk_children_get_attr(disk->syspath, "removable", "block", EINA_FALSE);
   if (test)
     {
        disk->cache.removable = !!strtol(test, NULL, 10);
        eina_stringshare_del(test);
     }

   disk->cache.filled = EINA_TRUE;
}

/**
 * @brief Associates arbitrary user data with an Eeze_Disk object.
 *
 * @param disk The Eeze_Disk object.
 * @param data A pointer to the user data to associate.
 */
EAPI void
eeze_disk_data_set(Eeze_Disk *disk, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(disk);

   disk->data = data;
}

/**
 * @brief Retrieves arbitrary user data associated with an Eeze_Disk object.
 *
 * @param disk The Eeze_Disk object.
 * @return A pointer to the user data, or NULL if no data is set or @p disk is NULL.
 */
EAPI void *
eeze_disk_data_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   return disk->data;
}

/**
 * @brief Gets the syspath of an Eeze_Disk object.
 *
 * The syspath is the path to the device within the sysfs filesystem
 * (e.g., "/sys/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0/block/sda").
 *
 * @param disk The Eeze_Disk object.
 * @return The syspath string, or NULL if @p disk is NULL. The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_syspath_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   return disk->syspath;
}

/**
 * @brief Gets the device path (devnode) of an Eeze_Disk object.
 *
 * The device path is the path to the device node in the /dev directory
 * (e.g., "/dev/sda1"). If the devpath was not set during creation (e.g., if
 * created from syspath), this function will query udev for it.
 *
 * @param disk The Eeze_Disk object.
 * @return The device path string, or NULL if @p disk is NULL or the devnode cannot be determined.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_devpath_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->devpath)
     return disk->devpath;
   disk->devpath = udev_device_get_devnode(disk->device);
   return disk->devpath;
}

/**
 * @brief Gets the filesystem type of an Eeze_Disk object.
 *
 * This typically refers to the filesystem detected on the disk, like "ext4", "vfat", etc.
 * Note: This field might not always be populated directly from udev properties in the current implementation
 * and might rely on other mechanisms or be set after a mount operation.
 *
 * @param disk The Eeze_Disk object.
 * @return The filesystem type string (e.g., "ext4"), or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_fstype_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   return disk->fstype;
}

/**
 * @brief Gets the vendor name of an Eeze_Disk object.
 *
 * This function retrieves the vendor information from the disk's cached properties.
 * If not cached, it queries udev properties ("ID_VENDOR", "vendor" sysattr).
 *
 * @param disk The Eeze_Disk object.
 * @return The vendor string (e.g., "ATA"), or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_vendor_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->cache.vendor)
     return disk->cache.vendor;

   disk->cache.vendor = udev_device_get_property_value(disk->device, "ID_VENDOR");
   if (!disk->cache.vendor) disk->cache.vendor = udev_device_get_sysattr_value(disk->device, "vendor");
   return disk->cache.vendor;
}

/**
 * @brief Gets the model name of an Eeze_Disk object.
 *
 * This function retrieves the model information from the disk's cached properties.
 * If not cached, it queries udev properties ("ID_MODEL", "model" sysattr).
 *
 * @param disk The Eeze_Disk object.
 * @return The model string (e.g., "VBOX_HARDDISK"), or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_model_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->cache.model)
     return disk->cache.model;

   disk->cache.model = udev_device_get_property_value(disk->device, "ID_MODEL");
   if (!disk->cache.model) disk->cache.model = udev_device_get_sysattr_value(disk->device, "model");
   return disk->cache.model;
}

/**
 * @brief Gets the serial number of an Eeze_Disk object.
 *
 * This function retrieves the serial number from the disk's cached properties.
 * If not cached, it queries the udev property "ID_SERIAL_SHORT".
 *
 * @param disk The Eeze_Disk object.
 * @return The serial number string, or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_serial_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->cache.serial)
     return disk->cache.serial;
   disk->cache.serial = udev_device_get_property_value(disk->device, "ID_SERIAL_SHORT");
   return disk->cache.serial;
}

/**
 * @brief Gets the filesystem UUID of an Eeze_Disk object.
 *
 * This function retrieves the UUID from the disk's cached properties.
 * If not cached, it queries the udev property "ID_FS_UUID".
 *
 * @param disk The Eeze_Disk object.
 * @return The UUID string (e.g., "1234-ABCD"), or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_uuid_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->cache.uuid)
     return disk->cache.uuid;
   disk->cache.uuid = udev_device_get_property_value(disk->device, "ID_FS_UUID");
   return disk->cache.uuid;
}

/**
 * @brief Gets the filesystem label of an Eeze_Disk object.
 *
 * This function retrieves the label from the disk's cached properties.
 * If not cached, it queries the udev property "ID_FS_LABEL".
 *
 * @param disk The Eeze_Disk object.
 * @return The label string (e.g., "MyUSB"), or NULL if not available or @p disk is NULL.
 *         The string is an Eina_Stringshare.
 */
EAPI const char *
eeze_disk_label_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   if (disk->cache.label)
     return disk->cache.label;
   disk->cache.label = udev_device_get_property_value(disk->device, "ID_FS_LABEL");
   return disk->cache.label;
}

/**
 * @brief Gets the type of an Eeze_Disk object.
 *
 * This function retrieves the disk type (e.g., CD-ROM, USB, internal)
 * from the disk's cached properties. If not cached, it calls _eeze_disk_type_find()
 * to determine and cache the type.
 *
 * @param disk The Eeze_Disk object.
 * @return The Eeze_Disk_Type, or EEZE_DISK_TYPE_UNKNOWN if @p disk is NULL or type cannot be determined.
 */
EAPI Eeze_Disk_Type
eeze_disk_type_get(Eeze_Disk *disk)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EEZE_DISK_TYPE_UNKNOWN);

   if (disk->cache.type)
     return disk->cache.type;
   disk->cache.type = _eeze_disk_type_find(disk);
   return disk->cache.type;
}

/**
 * @brief Checks if an Eeze_Disk object represents a removable device.
 *
 * This function retrieves the removability status from the disk's cached properties.
 * If the cache is not filled (i.e., eeze_disk_scan() hasn't been effectively run),
 * it queries udev sysattr "removable" or walks children devices to determine this.
 *
 * @param disk The Eeze_Disk object.
 * @return EINA_TRUE if the disk is removable, EINA_FALSE otherwise or if @p disk is NULL.
 */
EAPI Eina_Bool
eeze_disk_removable_get(Eeze_Disk *disk)
{
   const char *test;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);

   if (disk->cache.filled)
     return disk->cache.removable;

   test = udev_device_get_sysattr_value(disk->device, "removable");
   if (test) disk->cache.removable = !!strtol(test, NULL, 10);
   else
     test = _walk_children_get_attr(disk->syspath, "removable", "block", EINA_FALSE);
   if (test)
     {
        disk->cache.removable = !!strtol(test, NULL, 10);
        eina_stringshare_del(test);
     }
   return disk->cache.removable;
}

/**
 * @brief Checks if the system has the capability to mount disks using the configured mount utility.
 *
 * This function checks for the existence and executability of the mount binary
 * (defined by EEZE_MOUNT_BIN).
 *
 * @return EINA_TRUE if mounting is possible, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_disk_can_mount(void)
{
   return eina_file_access(EEZE_MOUNT_BIN,
                           EINA_FILE_ACCESS_MODE_EXEC |
                           EINA_FILE_ACCESS_MODE_READ);
}

/**
 * @brief Checks if the system has the capability to unmount disks using the configured unmount utility.
 *
 * This function checks for the existence and executability of the unmount binary
 * (defined by EEZE_UNMOUNT_BIN).
 *
 * @return EINA_TRUE if unmounting is possible, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_disk_can_unmount(void)
{
   return eina_file_access(EEZE_UNMOUNT_BIN,
                           EINA_FILE_ACCESS_MODE_EXEC |
                           EINA_FILE_ACCESS_MODE_READ);
}

/**
 * @brief Checks if the system has the capability to eject disks using the configured eject utility.
 *
 * This function checks for the existence and executability of the eject binary
 * (defined by EEZE_EJECT_BIN).
 *
 * @return EINA_TRUE if ejecting is possible, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_disk_can_eject(void)
{
   return eina_file_access(EEZE_EJECT_BIN,
                           EINA_FILE_ACCESS_MODE_EXEC |
                           EINA_FILE_ACCESS_MODE_READ);
}
