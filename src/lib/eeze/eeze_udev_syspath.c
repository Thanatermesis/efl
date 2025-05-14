#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eeze.h>
#include "eeze_udev_private.h"

/**
 * @brief Get the syspath of the parent device matching a subsystem and devtype.
 *
 * This function walks up the device chain from the given @p syspath and
 * returns the syspath of the first parent device that matches the specified
 * @p subsystem and @p devtype.
 *
 * @param syspath The syspath of the child device.
 * @param subsystem The subsystem to filter by (e.g., "usb"). Can be NULL.
 * @param devtype The devtype to filter by (e.g., "usb_device"). Can be NULL.
 * @return The syspath of the matching parent device as an Eina_Stringshare,
 *         or @c NULL if no matching parent is found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI Eina_Stringshare *
eeze_udev_syspath_get_parent_filtered(const char *syspath, const char *subsystem, const char *devtype)
{
   _udev_device *device, *parent;
   Eina_Stringshare *ret = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(syspath, NULL);

   if (!(device = _new_device(syspath)))
     return NULL;
   parent = udev_device_get_parent_with_subsystem_devtype(device, subsystem, devtype);
   if (parent)
     ret = eina_stringshare_add(udev_device_get_syspath(parent));
   udev_device_unref(device);
   return ret;
}

/**
 * @brief Get the syspath of the immediate parent device.
 *
 * @param syspath The syspath of the child device.
 * @return The syspath of the parent device as an Eina_Stringshare,
 *         or @c NULL if no parent is found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_parent(const char *syspath)
{
   _udev_device *device, *parent;
   const char *ret;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;
   parent = udev_device_get_parent(device);
   ret = eina_stringshare_add(udev_device_get_syspath(parent));
   udev_device_unref(device);
   return ret;
}

/**
 * @brief Get a list of syspaths for all parent devices.
 *
 * This function walks up the device chain from the given @p syspath
 * and returns a list of syspaths for all ancestor devices. The list
 * is ordered from the closest parent to the furthest ancestor.
 *
 * @param syspath The syspath of the child device.
 * @return An Eina_List containing Eina_Stringshare elements for each parent's
 *         syspath, or @c NULL if no parents are found or on error.
 *         The list and its stringshare elements must be freed by the caller.
 *         Example: eina_list_free() after eina_stringshare_del() on each item.
 */
EAPI Eina_List *
eeze_udev_syspath_get_parents(const char *syspath)
{
   _udev_device *child, *parent, *device;
   const char *path;
   Eina_List *devlist = NULL;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   if (!(parent = udev_device_get_parent(device)))
     {
        udev_device_unref(device);
        return NULL;
     }

   for (; parent; child = parent, parent = udev_device_get_parent(child))
     {
        path = udev_device_get_syspath(parent);
        devlist = eina_list_append(devlist, eina_stringshare_add(path));
     }

   udev_device_unref(device);
   return devlist;
}

/**
 * @brief Get the device node path (devpath) for a given syspath.
 *
 * The devpath is the path to the device file in the /dev directory
 * (e.g., "/dev/sda1").
 *
 * @param syspath The syspath of the device.
 * @return The devpath as an Eina_Stringshare, or @c NULL if not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_devpath(const char *syspath)
{
   _udev_device *device;
   const char *name = NULL;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   if (!(name = udev_device_get_devnode(device)))
     {
        udev_device_unref(device);
        return NULL;
     }

   name = eina_stringshare_add(name);
   udev_device_unref(device);
   return name;
}

/**
 * @brief Get the system name (sysname) for a given syspath.
 *
 * The sysname is the kernel's internal name for the device (e.g., "sda1").
 *
 * @param syspath The syspath of the device.
 * @return The sysname as an Eina_Stringshare, or @c NULL if not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_devname(const char *syspath)
{
   _udev_device *device;
   const char *name = NULL;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   if (!(name = udev_device_get_sysname(device)))
     {
        udev_device_unref(device);
        return NULL;
     }

   name = eina_stringshare_add(name);
   udev_device_unref(device);
   return name;
}

/**
 * @brief Get the subsystem of a device for a given syspath.
 *
 * Example subsystems include "block", "input", "pci", etc.
 *
 * @param syspath The syspath of the device.
 * @return The subsystem name as an Eina_Stringshare, or @c NULL if not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_subsystem(const char *syspath)
{
   _udev_device *device;
   const char *subsystem;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;
   subsystem = eina_stringshare_add(udev_device_get_property_value(device, "SUBSYSTEM"));
   udev_device_unref(device);
   return subsystem;
}

/**
 * @brief Check if a device property matches a given value.
 *
 * @param syspath The syspath of the device.
 * @param property The name of the property to check (e.g., "ID_VENDOR_ID").
 * @param value The value to compare against.
 * @return @c EINA_TRUE if the property exists and its value matches @p value,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_check_property(const char *syspath, const char *property, const char *value)
{
   _udev_device *device;
   const char *test;
   Eina_Bool ret = EINA_FALSE;

   if (!syspath || !property || !value)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;
   if ((test = udev_device_get_property_value(device, property)))
     ret = !strcmp(test, value);

   udev_device_unref(device);
   return ret;
}

/**
 * @brief Get the value of a device property.
 *
 * @param syspath The syspath of the device.
 * @param property The name of the property to retrieve (e.g., "ID_MODEL_ID").
 * @return The property value as an Eina_Stringshare, or @c NULL if the property
 *         is not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_property(const char *syspath,
                               const char *property)
{
   _udev_device *device;
   const char *test;
   Eina_Stringshare *value = NULL;

   if (!syspath || !property)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;
   if ((test = udev_device_get_property_value(device, property)))
     value = eina_stringshare_add(test);

   udev_device_unref(device);
   return value;
}

/**
 * @brief Check if a device sysattr matches a given value.
 *
 * Sysattrs are kernel attributes of a device, found in sysfs.
 *
 * @param syspath The syspath of the device.
 * @param sysattr The name of the sysattr to check (e.g., "power/control").
 * @param value The value to compare against.
 * @return @c EINA_TRUE if the sysattr exists and its value matches @p value,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_check_sysattr(const char *syspath, const char *sysattr, const char *value)
{
   _udev_device *device;
   const char *test;
   Eina_Bool ret = EINA_FALSE;

   if (!syspath || !sysattr || !value)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   if ((test = udev_device_get_sysattr_value(device, sysattr)))
     ret = !strcmp(test, value);

   udev_device_unref(device);
   return ret;
}

/**
 * @brief Get the value of a device sysattr.
 *
 * @param syspath The syspath of the device.
 * @param sysattr The name of the sysattr to retrieve (e.g., "manufacturer").
 * @return The sysattr value as an Eina_Stringshare, or @c NULL if the sysattr
 *         is not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_syspath_get_sysattr(const char *syspath,
                              const char *sysattr)
{
   _udev_device *device;
   const char *value = NULL, *test;

   if (!syspath || !sysattr)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   if ((test = udev_device_get_sysattr_value(device, sysattr)))
     value = eina_stringshare_add(test);

   udev_device_unref(device);
   return value;
}

/**
 * @brief Set the value of a device sysattr.
 *
 * Note: This function attempts to write a double value as a string to the sysattr.
 * This operation might not be supported for all sysattrs or may require
 * specific privileges.
 * The functionality depends on the version of libudev being used (OLD_LIBUDEV macro).
 *
 * @param syspath The syspath of the device.
 * @param sysattr The name of the sysattr to set (e.g., "brightness").
 * @param value The double value to set.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if not supported.
 */
EAPI Eina_Bool
eeze_udev_syspath_set_sysattr(const char *syspath,
                              const char *sysattr,
                              double value)
{
   _udev_device *device;
   Eina_Bool ret = EINA_FALSE;

   if (!syspath || !sysattr)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

#ifndef OLD_LIBUDEV
   char val[16];
   int test;

   sprintf(val, "%f", value);
   test = udev_device_set_sysattr_value(device, sysattr, val);
   if (test == 0)
     ret = EINA_TRUE;
#else
   (void)value;
#endif

  udev_device_unref(device);
  return ret;
}

/**
 * @brief Get a list of all sysattr names for a device.
 *
 * @param syspath The syspath of the device.
 * @return An Eina_List containing Eina_Stringshare elements for each sysattr name,
 *         or @c NULL if no sysattrs are found or on error.
 *         The list and its stringshare elements must be freed by the caller.
 */
EAPI Eina_List *
eeze_udev_syspath_get_sysattr_list(const char *syspath)
{
   _udev_device *device;
   _udev_list_entry *devs, *cur;
   Eina_List *syslist = NULL;

   if (!syspath)
     return NULL;

   if (!(device = _new_device(syspath)))
     return NULL;

   devs = udev_device_get_sysattr_list_entry(device);
   udev_list_entry_foreach(cur, devs)
     {
        syslist = eina_list_append(syslist,
                   eina_stringshare_add(udev_list_entry_get_name(cur)));
     }

   udev_device_unref(device);
   return syslist;
}

/**
 * @brief Check if a device is a mouse.
 *
 * This function checks the "ID_INPUT_MOUSE" property of the device.
 *
 * @param syspath The syspath of the device.
 * @return @c EINA_TRUE if the device is identified as a mouse,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_is_mouse(const char *syspath)
{
   _udev_device *device = NULL;
   Eina_Bool mouse = EINA_FALSE;
   const char *test = NULL;

   if (!syspath)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   test = udev_device_get_property_value(device, "ID_INPUT_MOUSE");

   if (test && (test[0] == '1'))
     mouse = EINA_TRUE;

   udev_device_unref(device);
   return mouse;
}

/**
 * @brief Check if a device is a keyboard.
 *
 * This function checks the "ID_INPUT_KEYBOARD" property of the device.
 *
 * @param syspath The syspath of the device.
 * @return @c EINA_TRUE if the device is identified as a keyboard,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_is_kbd(const char *syspath)
{
   _udev_device *device = NULL;
   Eina_Bool kbd = EINA_FALSE;
   const char *test = NULL;

   if (!syspath)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   test = udev_device_get_property_value(device, "ID_INPUT_KEYBOARD");

   if (test && (test[0] == '1'))
     kbd = EINA_TRUE;

   udev_device_unref(device);
   return kbd;
}

/**
 * @brief Check if a device is a touchpad.
 *
 * This function checks the "ID_INPUT_TOUCHPAD" property of the device.
 *
 * @param syspath The syspath of the device.
 * @return @c EINA_TRUE if the device is identified as a touchpad,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_is_touchpad(const char *syspath)
{
   _udev_device *device = NULL;
   Eina_Bool touchpad = EINA_FALSE;
   const char *test;

   if (!syspath)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   test = udev_device_get_property_value(device, "ID_INPUT_TOUCHPAD");

   if (test && (test[0] == '1'))
     touchpad = EINA_TRUE;

   udev_device_unref(device);
   return touchpad;
}

/**
 * @brief Check if a device is a joystick.
 *
 * This function checks the "ID_INPUT_JOYSTICK" property of the device.
 *
 * @param syspath The syspath of the device.
 * @return @c EINA_TRUE if the device is identified as a joystick,
 *         @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
eeze_udev_syspath_is_joystick(const char *syspath)
{
   _udev_device *device = NULL;
   Eina_Bool joystick = EINA_FALSE;
   const char *test;

   if (!syspath)
     return EINA_FALSE;

   if (!(device = _new_device(syspath)))
     return EINA_FALSE;

   test = udev_device_get_property_value(device, "ID_INPUT_JOYSTICK");

   if (test && (test[0] == '1'))
     joystick = EINA_TRUE;

   udev_device_unref(device);
   return joystick;
}

/**
 * @brief Get the syspath for a given device node path (devpath).
 *
 * This function queries udev to find a device matching the given @p devpath
 * (e.g., "/dev/input/event0") and returns its corresponding syspath.
 *
 * @param devpath The device node path (e.g., "/dev/ttyS0").
 * @return The syspath as an Eina_Stringshare, or @c NULL if not found or on error.
 *         The returned stringshare must be freed by the caller using
 *         eina_stringshare_del().
 */
EAPI const char *
eeze_udev_devpath_get_syspath(const char *devpath)
{
   _udev_enumerate *en;
   _udev_list_entry *devs, *cur;
   const char *ret = NULL;

   if (!devpath)
     return NULL;

   en = udev_enumerate_new(udev);

   if (!en)
     return NULL;

   udev_enumerate_add_match_property(en, "DEVNAME", devpath);
   udev_enumerate_scan_devices(en);
   devs = udev_enumerate_get_list_entry(en);
   udev_list_entry_foreach(cur, devs)
     {
        ret = eina_stringshare_add(udev_list_entry_get_name(cur));
        break; /*just in case there's more than one somehow */
     }
   udev_enumerate_unref(en);
   return ret;
}

/**
 * @brief Get the sysnum (kernel number) of a device.
 *
 * The sysnum is a string representation of the kernel number for the device.
 * This function converts it to an integer.
 *
 * @param syspath The syspath of the device.
 * @return The sysnum as an integer, or -1 if not found or on error.
 */
EAPI int
eeze_udev_syspath_get_sysnum(const char *syspath)
{
   _udev_device *device;
   const char *test;
   int ret = -1;

   if (!syspath)
     return -1;

   if (!(device = _new_device(syspath)))
     return -1;

   if ((test = udev_device_get_sysnum(device)))
     ret = atoi(test);

   udev_device_unref(device);
   return ret;
}

