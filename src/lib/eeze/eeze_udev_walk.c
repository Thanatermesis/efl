#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eeze.h>
#include "eeze_udev_private.h"

/**
 * @brief Walks up the device tree from @p syspath, checking for a sysattr @p sysattr
 * which matches @p value.
 *
 * @param syspath The starting syspath for the device tree walk.
 *                Example: "/sys/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0/block/sda"
 * @param sysattr The sysattr to check for. Example: "removable"
 * @param value The value to compare the sysattr against. If NULL, any value will match.
 *              Example: "1"
 * @return @c EINA_TRUE if a matching sysattr is found, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
eeze_udev_walk_check_sysattr(const char *syspath,
                             const char *sysattr,
                             const char *value)
{
   _udev_device *device, *child, *parent;
   Eina_Bool ret = EINA_FALSE;
   const char *test = NULL;

   if (!udev)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   for (parent = device; parent;
        child = parent, parent = udev_device_get_parent(child))
     {
        if (!(test = udev_device_get_sysattr_value(parent, sysattr)))
          continue;
        if ((value && (!strcmp(test, value))) || (!value))
          {
             ret = EINA_TRUE;
             break;
          }
     }

   udev_device_unref(device);
   return ret;
}

/**
 * @brief Walks up the device tree from @p syspath, returning the first value
 * found for the sysattr @p sysattr.
 *
 * @param syspath The starting syspath for the device tree walk.
 *                Example: "/sys/devices/pci0000:00/0000:00:1f.2/ata1/host0/target0:0:0/0:0:0:0/block/sda"
 * @param sysattr The sysattr whose value should be retrieved. Example: "idVendor"
 * @return A stringshared pointer to the value of the sysattr if found,
 *         @c NULL otherwise. The returned string should be freed with
 *         eina_stringshare_del() when no longer needed.
 *         Example return: "0x8086"
 */
EAPI const char *
eeze_udev_walk_get_sysattr(const char *syspath,
                           const char *sysattr)
{
   _udev_device *device, *parent;
   const char *test = NULL;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   for (parent = device; parent && !test;)
     {
        test = udev_device_get_sysattr_value(parent, sysattr);
        parent = udev_device_get_parent(parent);
     }

   udev_device_unref(device);
   return eina_stringshare_add(test);
}
