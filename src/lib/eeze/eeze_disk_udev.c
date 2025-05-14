#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Ecore.h>
#include <Eeze.h>
#include <Eeze_Disk.h>

#include "eeze_udev_private.h"
#include "eeze_disk_private.h"

/**
 * @brief Get a udev property value for a disk.
 * @param disk The Eeze_Disk object.
 * @param property The name of the property to get (e.g., "ID_FS_UUID", "ID_MODEL").
 * @return The property value as a stringshared string, or NULL on failure.
 *         The returned string must be freed with eina_stringshare_del().
 */
EAPI const char *
eeze_disk_udev_get_property(Eeze_Disk *disk, const char *property)
{
   const char *ret;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(property, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!*property, NULL);

   ret = udev_device_get_property_value(disk->device, property);
   return eina_stringshare_add(ret);
}

/**
 * @brief Get a udev sysattr value for a disk.
 * @param disk The Eeze_Disk object.
 * @param sysattr The name of the sysattr to get (e.g., "removable", "size").
 * @return The sysattr value as a stringshared string, or NULL on failure.
 *         The returned string must be freed with eina_stringshare_del().
 */
EAPI const char *
eeze_disk_udev_get_sysattr(Eeze_Disk *disk, const char *sysattr)
{
   const char *ret;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sysattr, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!*sysattr, NULL);

   ret = udev_device_get_sysattr_value(disk->device, sysattr);
   return eina_stringshare_add(ret);
}

/**
 * @brief Get the syspath of the parent device for a disk.
 * @param disk The Eeze_Disk object.
 * @return The syspath of the parent device as a stringshared string, or NULL on failure.
 *         The returned string must be freed with eina_stringshare_del().
 *         Example: "/sys/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0"
 */
EAPI const char *
eeze_disk_udev_get_parent(Eeze_Disk *disk)
{
   _udev_device *parent;
   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);

   parent = udev_device_get_parent(disk->device);
   return eina_stringshare_add(udev_device_get_syspath(parent));
}

/**
 * @brief Walk up the device tree from the given disk and check if a sysattr matches a value.
 *
 * This function iterates upwards through the parent devices of the given disk.
 * For each parent, it retrieves the specified sysattr. If the sysattr exists,
 * it compares its value with the provided 'value'.
 *
 * @param disk The Eeze_Disk object to start the walk from.
 * @param sysattr The name of the sysattr to check (e.g., "subsystem").
 * @param value The value to compare the sysattr against. If NULL, the function
 *              checks only for the existence of the sysattr.
 * @return EINA_TRUE if the sysattr is found and (if 'value' is not NULL) matches the value,
 *         EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_disk_udev_walk_check_sysattr(Eeze_Disk *disk,
                                  const char *sysattr,
                                  const char *value)
{
   _udev_device *child, *parent;
   const char *test = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sysattr, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!*sysattr, EINA_FALSE);

   for (parent = disk->device; parent;
        child = parent, parent = udev_device_get_parent(child))
     {
        if (!(test = udev_device_get_sysattr_value(parent, sysattr)))
          continue;
        if ((value && (!strcmp(test, value))) || (!value))
          {
             return EINA_TRUE;
             break;
          }
     }
   return EINA_FALSE;
}

/**
 * @brief Walk up the device tree from the given disk and get the first matching sysattr value.
 *
 * This function iterates upwards through the parent devices of the given disk.
 * For each parent, it attempts to retrieve the specified sysattr.
 *
 * @param disk The Eeze_Disk object to start the walk from.
 * @param sysattr The name of the sysattr to get (e.g., "idVendor", "idProduct").
 * @return The value of the first sysattr found as a stringshared string, or NULL if not found.
 *         The returned string must be freed with eina_stringshare_del().
 */
EAPI const char *
eeze_disk_udev_walk_get_sysattr(Eeze_Disk *disk,
                                  const char *sysattr)
{
   _udev_device *child, *parent;
   const char *test = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(disk, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sysattr, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!*sysattr, NULL);

   for (parent = disk->device; parent;
        child = parent, parent = udev_device_get_parent(child))
     {
        test = udev_device_get_sysattr_value(parent, sysattr);
        if (test) return eina_stringshare_add(test);
     }
   return NULL;
}
