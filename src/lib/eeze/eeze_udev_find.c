#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Eeze.h>
#include "eeze_udev_private.h"

/**
 * @brief Finds udev devices similar to the one specified by syspath.
 *
 * This function searches for udev devices that share the same vendor, model,
 * and revision ID as the device identified by the given syspath.
 * It also includes parent devices of the found similar devices if they
 * have an "idVendor" sysattr, indicating they are device roots.
 *
 * @param syspath The syspath of the device to find similar devices for.
 *                Example: "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0"
 * @return A list of syspaths (Eina_Stringshare *) for similar devices,
 *         or NULL on failure or if no similar devices are found.
 *         The caller is responsible for freeing the list and its contents
 *         using eina_list_free() and eina_stringshare_del() for each item.
 *         Example of returned list structure:
 *         ["/sys/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0/video4linux/video0",
 *          "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-2/1-2:1.0/sound/card1"]
 */
EAPI Eina_List *
eeze_udev_find_similar_from_syspath(const char *syspath)
{
   _udev_device *device;
   _udev_list_entry *devs, *cur;
   _udev_enumerate *en;
   Eina_List *l, *ret = NULL;
   const char *vendor, *model, *revision, *devname, *dev;

   if (!syspath)
     return NULL;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   if (!(device = _new_device(syspath)))
     {
        udev_enumerate_unref(en);
        return NULL;
     }

   vendor = udev_device_get_property_value(device, "ID_VENDOR_ID");

   if (vendor)
     udev_enumerate_add_match_property(en, "ID_VENDOR_ID", vendor);

   model = udev_device_get_property_value(device, "ID_MODEL_ID");

   if (model)
     udev_enumerate_add_match_property(en, "ID_MODEL_ID", model);

   revision = udev_device_get_property_value(device, "ID_REVISION");

   if (revision)
     udev_enumerate_add_match_property(en, "ID_REVISION", revision);

   udev_enumerate_scan_devices(en);
   udev_device_unref(device);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);
        /* verify unlisted device */

        EINA_LIST_FOREACH(ret, l, dev)
          if (!strcmp(dev, devname))
            continue;

        ret = eina_list_prepend(ret, eina_stringshare_add(devname));
        device = udev_device_new_from_syspath(udev, devname);

        /* only device roots have this sysattr,
         * and we only need to check parents of the roots
         */
        if (udev_device_get_sysattr_value(device, "idVendor"))
          ret = _get_unlisted_parents(ret, device);

        udev_device_unref(device);
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @brief Finds udev devices similar to those in the provided list, including unlisted parents.
 *
 * This function iterates through a given list of device syspaths. For each device,
 * it attempts to find other devices with matching vendor, model, and revision
 * properties or sysattrs. It also includes parent devices of the found similar
 * devices if they have an "idVendor" sysattr.
 *
 * @param list An Eina_List of device syspaths (const char *) to search for similar devices.
 *             Example: A list containing strings like
 *             "/sys/devices/pci0000:00/0000:00:1a.0/usb1/1-1/1-1.5/1-1.5:1.0".
 * @return The original list, potentially augmented with syspaths (Eina_Stringshare *)
 *         of newly found similar devices and their relevant parents.
 *         Returns NULL if the input list is NULL or if critical udev operations fail.
 *         The caller is responsible for managing the memory of the returned list
 *         and its stringshared items if it's different from the input or if new items were added.
 *         Example of returned list structure (if new items are added):
 *         Original items +
 *         ["/sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.6/2-1.6:1.0/host0/target0:0:0/0:0:0:0/block/sda",
 *          "/sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.6"]
 */
EAPI Eina_List *
eeze_udev_find_unlisted_similar(Eina_List *list)
{
   _udev_device *device;
   _udev_list_entry *devs, *cur;
   _udev_enumerate *en;
   Eina_List *l;
   const char *vendor, *model, *revision, *devname, *dev;

   if (!list)
     return NULL;

   EINA_LIST_FOREACH(list, l, dev)
     {
        en = udev_enumerate_new(udev);

        if (!en)
          return NULL;

        device = _new_device(dev);
        if (!device)
          {
             udev_enumerate_unref(en);
             continue;
          }

        if ((vendor = udev_device_get_property_value(device, "ID_VENDOR_ID")))
          udev_enumerate_add_match_property(en, "ID_VENDOR_ID", vendor);
        else if ((vendor = udev_device_get_property_value(device, "ID_VENDOR")))
          udev_enumerate_add_match_property(en, "ID_VENDOR", vendor);
        else if ((vendor = udev_device_get_sysattr_value(device, "vendor")))
          udev_enumerate_add_match_sysattr(en, "vendor", vendor);
        else if ((vendor = udev_device_get_sysattr_value(device, "manufacturer")))
          udev_enumerate_add_match_sysattr(en, "manufacturer", vendor);

        if ((model = udev_device_get_property_value(device, "ID_MODEL_ID")))
          udev_enumerate_add_match_property(en, "ID_MODEL_ID", model);
        else if ((model = udev_device_get_property_value(device, "ID_MODEL")))
          udev_enumerate_add_match_property(en, "ID_MODEL", model);
        else if ((model = udev_device_get_sysattr_value(device, "model")))
          udev_enumerate_add_match_sysattr(en, "model", model);
        else if ((model = udev_device_get_sysattr_value(device, "product")))
          udev_enumerate_add_match_sysattr(en, "product", model);

        if ((revision = udev_device_get_property_value(device, "ID_REVISION")))
          udev_enumerate_add_match_property(en, "ID_REVISION", revision);
        else if ((revision = udev_device_get_sysattr_value(device, "revision")))
          udev_enumerate_add_match_sysattr(en, "revision", revision);

        udev_enumerate_add_match_subsystem(en, udev_device_get_subsystem(device));

        udev_enumerate_scan_devices(en);
        udev_device_unref(device);
        devs = udev_enumerate_get_list_entry(en);
        udev_list_entry_foreach(cur, devs)
          {
             devname = udev_list_entry_get_name(cur);
             device = udev_device_new_from_syspath(udev, devname);

             /* only device roots have this sysattr,
              * and we only need to check parents of the roots
              */
             if (udev_device_get_sysattr_value(device, "idVendor"))
               list = _get_unlisted_parents(list, device);

             udev_device_unref(device);
          }
        udev_enumerate_unref(en);
     }
   return list;
}

/**
 * @brief Finds udev devices by a predefined Eeze_Udev_Type and optionally by name.
 *
 * This function scans for udev devices matching a specific type (e.g., keyboard, mouse,
 * storage device) and can further filter them if their syspath contains the given name string.
 *
 * @param etype The type of device to search for (Eeze_Udev_Type).
 *              Example: EEZE_UDEV_TYPE_KEYBOARD
 * @param name An optional string to filter devices by. If provided, only devices
 *             whose syspath contains this string will be returned. Can be NULL.
 *             Example: "event" (to find event-based input devices)
 * @return A list of syspaths (Eina_Stringshare *) for matching devices,
 *         or NULL on failure or if no devices are found.
 *         The caller is responsible for freeing the list and its contents.
 *         Example of returned list structure:
 *         ["/sys/devices/platform/i8042/serio0/input/input0/event0",
 *          "/sys/devices/pci0000:00/0000:00:14.0/usb1/1-1/1-1:1.0/input/input5/mouse0"]
 */
EAPI Eina_List *
eeze_udev_find_by_type(Eeze_Udev_Type etype,
                       const char    *name)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   _udev_device *device, *parent;
   const char *devname;
   Eina_List *ret = NULL;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   switch (etype)
     {
      case EEZE_UDEV_TYPE_NONE:
        break;

      case EEZE_UDEV_TYPE_KEYBOARD:
        udev_enumerate_add_match_subsystem(en, "input");
        udev_enumerate_add_match_property(en, "ID_INPUT_KEYBOARD", "1");
        break;

      case EEZE_UDEV_TYPE_MOUSE:
        udev_enumerate_add_match_subsystem(en, "input");
        udev_enumerate_add_match_property(en, "ID_INPUT_MOUSE", "1");
        break;

      case EEZE_UDEV_TYPE_TOUCHPAD:
        udev_enumerate_add_match_subsystem(en, "input");
        udev_enumerate_add_match_property(en, "ID_INPUT_TOUCHPAD", "1");
        break;

      case EEZE_UDEV_TYPE_JOYSTICK:
        udev_enumerate_add_match_subsystem(en, "input");
        udev_enumerate_add_match_property(en, "ID_INPUT_JOYSTICK", "1");
        break;

      case EEZE_UDEV_TYPE_DRIVE_MOUNTABLE:
        udev_enumerate_add_match_subsystem(en, "block");
        udev_enumerate_add_match_property(en, "ID_FS_USAGE", "filesystem");
        break;

      case EEZE_UDEV_TYPE_DRIVE_INTERNAL:
        udev_enumerate_add_match_subsystem(en, "block");
        udev_enumerate_add_match_property(en, "ID_TYPE", "disk");
        udev_enumerate_add_match_property(en, "ID_BUS", "ata");
        udev_enumerate_add_match_sysattr(en, "removable", "0");
        break;

      case EEZE_UDEV_TYPE_DRIVE_REMOVABLE:
        udev_enumerate_add_match_sysattr(en, "removable", "1");
        udev_enumerate_add_match_property(en, "ID_TYPE", "disk");
        break;

      case EEZE_UDEV_TYPE_DRIVE_CDROM:
        udev_enumerate_add_match_property(en, "ID_CDROM", "1");
        break;

      case EEZE_UDEV_TYPE_POWER_AC:
        udev_enumerate_add_match_subsystem(en, "power_supply");
        udev_enumerate_add_match_sysattr(en, "type", "Mains");
        break;

      case EEZE_UDEV_TYPE_POWER_BAT:
        udev_enumerate_add_match_subsystem(en, "power_supply");
        udev_enumerate_add_match_sysattr(en, "type", "Battery");
        udev_enumerate_add_match_sysattr(en, "present", "1");
        break;

      case EEZE_UDEV_TYPE_NET:
        udev_enumerate_add_match_subsystem(en, "net");
        break;

      case EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR:
        udev_enumerate_add_match_subsystem(en, "hwmon");
        break;

      /*
              case EEZE_UDEV_TYPE_ANDROID:
                udev_enumerate_add_match_subsystem(en, "block");
                udev_enumerate_add_match_property(en, "ID_MODEL", "Android_*");
                break;
       */
      case EEZE_UDEV_TYPE_V4L:
        udev_enumerate_add_match_subsystem(en, "video4linux");
        break;

      case EEZE_UDEV_TYPE_BLUETOOTH:
        udev_enumerate_add_match_subsystem(en, "bluetooth");
        break;

      case EEZE_UDEV_TYPE_DRM:
        udev_enumerate_add_match_subsystem(en, "drm");
        udev_enumerate_add_match_subsystem(en, "card[0-9]*");
        break;

      case EEZE_UDEV_TYPE_BACKLIGHT:
        udev_enumerate_add_match_subsystem(en, "backlight");
        break;

      case EEZE_UDEV_TYPE_LEDS:
        udev_enumerate_add_match_subsystem(en, "leds");
        break;

      case EEZE_UDEV_TYPE_GRAPHICS:
        udev_enumerate_add_match_subsystem(en, "graphics");
        break;

      case EEZE_UDEV_TYPE_GPIO:
        udev_enumerate_add_match_subsystem(en, "gpio");
        break;

      default:
        break;
     }

   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);
        device = udev_device_new_from_syspath(udev, devname);


        if (etype == EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR) /* ensure that temp input exists somewhere in this device chain */
          {
             Eina_Bool one, two;
             const char *t;

             one = _walk_parents_test_attr(device, "temp1_input", NULL);
             two = _walk_parents_test_attr(device, "temp2_input", NULL);
             if ((!one) && (!two)) goto out;

             t = one ? "temp1_input" : "temp2_input";
             /* if device is not the one which has the temp input, we must go up the chain */
             if (!udev_device_get_sysattr_value(device, t))
               {
                  devname = NULL;

                  for (parent = udev_device_get_parent(device); parent; parent = udev_device_get_parent(parent)) /*check for parent */
                    if ((udev_device_get_sysattr_value(parent, t)))
                      {
                         devname = udev_device_get_syspath(parent);
                         break;
                      }

                  if (!devname)
                    goto out;
               }
          }
        else if (etype == EEZE_UDEV_TYPE_DRIVE_REMOVABLE)
          {
             /* this yields the actual hw device, not to be confused with the filesystem */
             devname = udev_device_get_syspath(udev_device_get_parent(device));
          }
        else if (etype == EEZE_UDEV_TYPE_DRIVE_MOUNTABLE)
          {
             int devcheck;

             devcheck = open(udev_device_get_devnode(device), O_RDONLY);
             if (devcheck < 0) goto out;
             close(devcheck);
          }

        if (name && (!strstr(devname, name)))
          goto out;

        ret = eina_list_append(ret, eina_stringshare_add(devname));
out:
        udev_device_unref(device);
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @brief Finds udev devices by subsystem, a generic type property, and optionally by name.
 *
 * This function allows for a more generic search based on subsystem, a udev property
 * (often used as a type identifier, e.g., "ID_INPUT_KEYBOARD"), and an optional name filter.
 *
 * @param subsystem The subsystem to match (e.g., "input", "block"). Can be NULL.
 *                  Example: "input"
 * @param type The udev property to match, expecting its value to be "1" (e.g., "ID_INPUT_MOUSE").
 *             Can be NULL. Example: "ID_INPUT_TOUCHPAD"
 * @param name An optional string to filter devices by. If provided, only devices
 *             whose syspath contains this string will be returned. Can be NULL.
 *             Example: "serio"
 * @return A list of syspaths (Eina_Stringshare *) for matching devices,
 *         or NULL on failure, if no criteria are provided, or if no devices are found.
 *         The caller is responsible for freeing the list and its contents.
 *         Example of returned list structure:
 *         ["/sys/devices/platform/i8042/serio1/input/input1",
 *          "/sys/devices/platform/i8042/serio1/input/input1/mouse1"]
 */
EAPI Eina_List *
eeze_udev_find_by_filter(const char *subsystem,
                         const char *type,
                         const char *name)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   const char *devname;
   Eina_List *ret = NULL;

   if ((!subsystem) && (!type) && (!name))
     return NULL;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   if (subsystem)
     udev_enumerate_add_match_subsystem(en, subsystem);

   if (type)
     udev_enumerate_add_match_property(en, type, "1");
   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);

        if (name && (!strstr(devname, name)))
          continue;

        ret = eina_list_append(ret, eina_stringshare_add(devname));
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @brief Finds udev devices by a specific sysattr and its value.
 *
 * This function searches for devices that have a given sysattr matching a specific value.
 * If value is NULL, it matches devices that have the sysattr present, regardless of its value.
 *
 * @param sysattr The sysattr to match (e.g., "removable", "idVendor"). Must not be NULL.
 *                Example: "removable"
 * @param value The expected value of the sysattr. If NULL, matches any device
 *              that has the sysattr. Example: "1"
 * @return A list of syspaths (Eina_Stringshare *) for matching devices,
 *         or NULL on failure, if sysattr is NULL, or if no devices are found.
 *         The caller is responsible for freeing the list and its contents.
 *         Example of returned list structure:
 *         ["/sys/devices/pci0000:00/0000:00:1d.0/usb2/2-1/2-1.6/2-1.6:1.0/host0/target0:0:0/0:0:0:0/block/sda",
 *          "/sys/devices/pci0000:00/0000:00:1a.0/usb1/1-1/1-1.2/1-1.2:1.0/host1/target1:0:0/1:0:0:0/block/sdb"]
 */
EAPI Eina_List *
eeze_udev_find_by_sysattr(const char *sysattr,
                          const char *value)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   const char *devname;
   Eina_List *ret = NULL;

   if (!sysattr)
     return NULL;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   udev_enumerate_add_match_sysattr(en, sysattr, value);
   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);
        ret = eina_list_append(ret, eina_stringshare_add(devname));
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @brief Finds udev devices by subsystem and sysname.
 *
 * This function searches for devices matching a given subsystem and/or sysname.
 * Either subsystem or sysname (or both) can be provided for filtering.
 *
 * @param subsystem The subsystem to match (e.g., "net", "drm"). Can be NULL.
 *                  Example: "drm"
 * @param sysname The sysname of the device (e.g., "card0", "eth0"). Can be NULL.
 *                Example: "card0"
 * @return A list of syspaths (Eina_Stringshare *) for matching devices,
 *         or NULL on failure or if no devices are found.
 *         The caller is responsible for freeing the list and its contents.
 *         Example of returned list structure:
 *         ["/sys/devices/pci0000:00/0000:00:02.0/drm/card0",
 *          "/sys/devices/pci0000:00/0000:00:02.0/drm/card0/card0-VGA-1"]
 */
EAPI Eina_List *
eeze_udev_find_by_subsystem_sysname(const char *subsystem, const char *sysname)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   const char *devname;
   Eina_List *ret = NULL;

   en = udev_enumerate_new(udev);
   if (!en) return NULL;

   if (subsystem) udev_enumerate_add_match_subsystem(en, subsystem);
   if (sysname) udev_enumerate_add_match_sysname(en, sysname);

   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        devname = udev_list_entry_get_name(cur);
        ret = eina_list_append(ret, eina_stringshare_add(devname));
     }
   udev_enumerate_unref(en);
   return ret;
}
