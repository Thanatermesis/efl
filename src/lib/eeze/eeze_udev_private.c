#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eeze.h>
#include "eeze_udev_private.h"

/**
 * @internal
 * @brief Helper function to set up a new device from a syspath.
 *
 * This function creates a udev_device object from a given sysfs path.
 * The syspath can optionally start with "/sys".
 *
 * @param syspath The sysfs path of the device.
 *                Example: "/sys/class/power_supply/BAT0" or "class/power_supply/BAT0".
 * @return A pointer to the newly created _udev_device, or NULL on failure.
 *         The caller is responsible for unref'ing the device using udev_device_unref().
 */
_udev_device *
_new_device(const char *syspath)
{
   _udev_device *device;

   device = udev_device_new_from_syspath(udev, syspath);
   if (!device)
     ERR("device %s does not exist!", syspath);
   return device;
}

/**
 * @internal
 * @brief Creates a new reference to an existing udev device.
 *
 * This function increments the reference count of the given udev_device.
 *
 * @param device The _udev_device to copy (reference).
 * @return A pointer to the same _udev_device with an incremented reference count.
 *         The caller is responsible for unref'ing the device using udev_device_unref().
 */
_udev_device *
_copy_device(_udev_device *device)
{
   return udev_device_ref(device);
}

/**
 * @internal
 * @brief Walks up the device tree to test for a sysattr, optionally with a specific value.
 *
 * This function simulates a part of `udevadm info -a` functionality by
 * traversing upwards from the given device, checking each parent (and the
 * device itself) for the presence of a specified sysfs attribute.
 * If a `value` is provided, it also checks if the attribute's content matches.
 *
 * @param device The starting _udev_device.
 * @param sysattr The name of the sysfs attribute to look for (e.g., "power/control").
 * @param value Optional: The expected value of the sysfs attribute. If NULL,
 *              only the existence of the attribute is checked.
 * @return EINA_TRUE if the attribute (and value, if specified) is found in the
 *         device or any of its parents, EINA_FALSE otherwise.
 */
Eina_Bool
_walk_parents_test_attr(_udev_device *device,
                        const char   *sysattr,
                        const char   *value)
{
   _udev_device *parent, *child = device;
   const char *test;

   if (udev_device_get_sysattr_value(device, sysattr))
     return EINA_TRUE;

   parent = udev_device_get_parent(child);

   for (; parent; child = parent, parent = udev_device_get_parent(child))
     {
        if (!(test = udev_device_get_sysattr_value(parent, sysattr)))
          continue;

        if (!value)
          return EINA_TRUE;
        else
        if (!strcmp(test, value))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Walks up the device tree to get a sysattr or property value.
 *
 * This function traverses upwards from the given device, checking each parent
 * (and the device itself) for a specified sysfs attribute or udev property.
 * It returns the value of the first occurrence found.
 *
 * @param device The starting _udev_device.
 * @param sysattr The name of the sysfs attribute or udev property to retrieve
 *                (e.g., "manufacturer" or "ID_VENDOR_FROM_DATABASE").
 * @param property If EINA_TRUE, `sysattr` is treated as a udev property name.
 *                 If EINA_FALSE, `sysattr` is treated as a sysfs attribute name.
 * @return A stringshared copy of the attribute/property value if found,
 *         otherwise NULL. The caller owns the returned stringshare and must free it
 *         using eina_stringshare_del() when no longer needed.
 */
const char *
_walk_parents_get_attr(_udev_device *device,
                       const char   *sysattr,
                       Eina_Bool property)
{
   _udev_device *parent, *child = device;
   const char *test;

   if (property)
     test = udev_device_get_property_value(device, sysattr);
   else
     test = udev_device_get_sysattr_value(device, sysattr);
   if (test) return eina_stringshare_add(test);

   parent = udev_device_get_parent(child);

   for (; parent; child = parent, parent = udev_device_get_parent(child))
     {
        if (property)
          test = udev_device_get_property_value(parent, sysattr);
        else
          test = udev_device_get_sysattr_value(parent, sysattr);
        if (test) return eina_stringshare_add(test);
     }

   return NULL;
}

/**
 * @internal
 * @brief Walks down the device tree from a syspath to get a sysattr or property value from a child.
 *
 * This function enumerates devices under a given syspath (typically a parent device)
 * and searches for a child device that has the specified sysfs attribute or udev property.
 * It can optionally filter by subsystem.
 *
 * @param syspath The sysfs path of the parent device from which to start searching children.
 *                Example: "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-1".
 * @param sysattr The name of the sysfs attribute or udev property to retrieve from a child
 *                (e.g., "bmAttributes" or "ID_MODEL").
 * @param subsystem Optional: A subsystem name to filter child devices (e.g., "usb").
 *                  If NULL, children from any subsystem are considered.
 * @param property If EINA_TRUE, `sysattr` is treated as a udev property name.
 *                 If EINA_FALSE, `sysattr` is treated as a sysfs attribute name.
 * @return A stringshared copy of the attribute/property value if found in a child,
 *         otherwise NULL. The caller owns the returned stringshare and must free it
 *         using eina_stringshare_del() when no longer needed.
 */
const char *
_walk_children_get_attr(const char *syspath,
                        const char *sysattr,
                        const char *subsystem,
                        Eina_Bool property)
{
   char buf[PATH_MAX];
   const char *path, *ret = NULL;
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;

   en = udev_enumerate_new(udev);
   EINA_SAFETY_ON_NULL_RETURN_VAL(en, NULL);
   path = strrchr(syspath, '/');
   if (path) path++;
   else path = syspath;
   snprintf(buf, sizeof(buf), "%s*", path);
   udev_enumerate_add_match_sysname(en, buf);
   if (subsystem) udev_enumerate_add_match_subsystem(en, subsystem);
   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        const char *devname, *test;
        _udev_device *device;

        devname = udev_list_entry_get_name(cur);
        device = _new_device(devname);
        if (property)
          test = udev_device_get_property_value(device, sysattr);
        else
          test = udev_device_get_sysattr_value(device, sysattr);
        if (test)
          {
             ret = eina_stringshare_add(test);
             udev_device_unref(device);
             break;
          }
        udev_device_unref(device);
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @internal
 * @brief Finds and adds syspaths of parent devices to a list if not already present
 *        and if they share similar identifying characteristics (vendor/model).
 *
 * This function walks up the device tree from the given `device`. For each parent,
 * it compares vendor and model information (obtained from various properties and
 * sysattrs like ID_VENDOR_ID, ID_MODEL_ID, vendor, model, product, manufacturer).
 * If a parent's identifiers are consistent with the child's (or if identifiers are missing
 * but the chain continues), and its syspath is not already in the `list`, the parent's
 * syspath is stringshared and prepended to the `list`.
 * The traversal stops if a parent has mismatching vendor/model information,
 * indicating a boundary (e.g., a USB hub connected to a PCI controller).
 *
 * @param list An Eina_List of stringshared syspaths. This list is modified by
 *             prepending new parent syspaths.
 *             Example of list elements:
 *             - "/sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1" (a USB device)
 *             - "/sys/devices/pci0000:00/0000:00:1d.0/usb2" (its parent USB port)
 * @param device The _udev_device from which to start finding parents.
 * @return The modified Eina_List. The caller is responsible for freeing the list
 *         and its stringshared contents when no longer needed.
 */
Eina_List *
_get_unlisted_parents(Eina_List    *list,
                      _udev_device *device)
{
   _udev_device *parent, *child = device;
   const char *test, *devname, *vendor, *vendor2, *model, *model2;
   Eina_List *l;
   Eina_Bool found;

   if (!(vendor = udev_device_get_property_value(child, "ID_VENDOR_ID")))
     vendor = udev_device_get_property_value(child, "ID_VENDOR");
   if (!vendor) vendor = udev_device_get_sysattr_value(child, "vendor");
   if (!vendor) vendor = udev_device_get_sysattr_value(child, "manufacturer");

   if (!(model = udev_device_get_property_value(child, "ID_MODEL_ID")))
     model = udev_device_get_property_value(child, "ID_MODEL");
   if (!model) model = udev_device_get_sysattr_value(child, "model");
   if (!model) model = udev_device_get_sysattr_value(child, "product");

   parent = udev_device_get_parent(child);

   for (; parent; child = parent, parent = udev_device_get_parent(child))
     {
        found = EINA_FALSE;

        if (!(vendor2 = udev_device_get_property_value(child, "ID_VENDOR_ID")))
          vendor2 = udev_device_get_property_value(child, "ID_VENDOR");
        if (!vendor2) vendor2 = udev_device_get_sysattr_value(child, "vendor");
        if (!vendor2) vendor2 = udev_device_get_sysattr_value(child, "manufacturer");

        if (!(model2 = udev_device_get_property_value(child, "ID_MODEL_ID")))
          model2 = udev_device_get_property_value(child, "ID_MODEL");
        if (!model2) model2 = udev_device_get_sysattr_value(child, "model");
        if (!model2) model2 = udev_device_get_sysattr_value(child, "product");

        if ((!model2 && model) || (model2 && !model) || (!vendor2 && vendor)
            || (vendor2 && !vendor))
          break;
        else
        if (((model && model2) && (strcmp(model, model2))) ||
            ((vendor && vendor2) && (strcmp(vendor, vendor2))))
          break;

        devname = udev_device_get_syspath(parent);
        EINA_LIST_FOREACH(list, l, test)
          {
             if (!strcmp(test, devname))
               {
                  found = EINA_TRUE;
                  break;
               }
          }

        if (!found)
          list = eina_list_prepend(list, eina_stringshare_add(devname));
     }

   return list;
}

