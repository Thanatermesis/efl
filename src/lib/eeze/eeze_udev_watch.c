#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <Ecore.h>
#include <Eeze.h>
#include "eeze_udev_private.h"

/**
 * @internal
 * @brief List of active udev watches.
 */
static Eina_Inlist *watches;

/**
 * @internal
 * @brief Structure representing a udev watch.
 *
 * This structure holds all necessary information for a udev watch,
 * including the file descriptor handler, callback function, user data,
 * event type, udev monitor instance, and the type of udev device being watched.
 */
struct Eeze_Udev_Watch
{
   EINA_INLIST;
   Ecore_Fd_Handler *handler;
   Eeze_Udev_Watch_Cb func;
   void            *data;
   Eeze_Udev_Event  event;
   _udev_monitor   *mon;
   Eeze_Udev_Type   type;
   Eeze_Udev_Watch *watch;
};

/* private function to further filter watch results based on Eeze_Udev_Type
 * specified; helpful for new udev versions, but absolutely required for
 * old udev, which does not implement filtering in device monitors.
 */
/**
 * @internal
 * @brief Callback function for Ecore_Fd_Handler to process udev monitor events.
 *
 * This function is called when there is activity on the udev monitor's
 * file descriptor. It receives the udev device, filters it based on the
 * watch type and event, and then calls the user-provided callback.
 *
 * @param data The Eeze_Udev_Watch instance.
 * @param fd_handler The Ecore_Fd_Handler that triggered this callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active, ECORE_CALLBACK_CANCEL otherwise.
 */
static Eina_Bool
_get_syspath_from_watch(void             *data,
                        Ecore_Fd_Handler *fd_handler)
{
   Eeze_Udev_Watch *watch = data;
   _udev_device *device = NULL, *parent, *tmpdev;
   const char *ret, *test;
   Eeze_Udev_Watch_Cb func = watch->func;
   void *sdata = watch->data;
   int event = 0;

   if (!ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_READ))
     return EINA_TRUE;

   device = udev_monitor_receive_device(watch->mon);

   if (!device)
     return EINA_TRUE;

   if ((!(test = udev_device_get_action(device)))
       || (!(ret = udev_device_get_syspath(device))))
     goto error;

   if (watch->event)
     {
        if (!strcmp(test, "add"))
          {
             if ((watch->event != EEZE_UDEV_EVENT_NONE) &&
                 ((watch->event & EEZE_UDEV_EVENT_ADD) != EEZE_UDEV_EVENT_ADD))
               goto error;

             event |= EEZE_UDEV_EVENT_ADD;
          }
        else if (!strcmp(test, "remove"))
          {
             if ((watch->event != EEZE_UDEV_EVENT_NONE) &&
                 ((watch->event & EEZE_UDEV_EVENT_REMOVE) != EEZE_UDEV_EVENT_REMOVE))
               goto error;

             event |= EEZE_UDEV_EVENT_REMOVE;
          }
        else if (!strcmp(test, "change"))
          {
             if ((watch->event != EEZE_UDEV_EVENT_NONE) &&
                 ((watch->event & EEZE_UDEV_EVENT_CHANGE) != EEZE_UDEV_EVENT_CHANGE))
               goto error;

             event |= EEZE_UDEV_EVENT_CHANGE;
          }
        else if (!strcmp(test, "online"))
          {
             if ((watch->event != EEZE_UDEV_EVENT_NONE) &&
                 ((watch->event & EEZE_UDEV_EVENT_ONLINE) != EEZE_UDEV_EVENT_ONLINE))
               goto error;

             event |= EEZE_UDEV_EVENT_ONLINE;
          }
        else
          {
             if ((watch->event != EEZE_UDEV_EVENT_NONE) &&
                 ((watch->event & EEZE_UDEV_EVENT_OFFLINE) != EEZE_UDEV_EVENT_OFFLINE))
               goto error;

             event |= EEZE_UDEV_EVENT_OFFLINE;
          }
     }

   if ((event & EEZE_UDEV_EVENT_OFFLINE) || (event & EEZE_UDEV_EVENT_REMOVE))
     goto out;
   switch (watch->type)
     {
      case EEZE_UDEV_TYPE_KEYBOARD:
        if ((!udev_device_get_property_value(device, "ID_INPUT_KEYBOARD")) &&
            (!udev_device_get_property_value(device, "ID_INPUT_KEY")))
          goto error;

        break;

      case EEZE_UDEV_TYPE_MOUSE:
        if (!udev_device_get_property_value(device, "ID_INPUT_MOUSE"))
          goto error;

        break;

      case EEZE_UDEV_TYPE_TOUCHPAD:
        if (!udev_device_get_property_value(device, "ID_INPUT_TOUCHPAD"))
          goto error;

        break;

      case EEZE_UDEV_TYPE_JOYSTICK:
        if (!udev_device_get_property_value(device, "ID_INPUT_JOYSTICK"))
          goto error;

        break;

      case EEZE_UDEV_TYPE_DRIVE_MOUNTABLE:
        if (!(test = (udev_device_get_property_value(device, "ID_FS_USAGE"))) ||
            (strcmp("filesystem", test)))
        {
           if (event & EEZE_UDEV_EVENT_CHANGE)
             {
                test = udev_device_get_sysname(device);
                if (!test) goto error;
                if (!strncmp(test, "loop", 4)) break;
             }
           goto error;
        }
        {
           int devcheck;

           devcheck = open(udev_device_get_devnode(device), O_RDONLY);
           if (devcheck < 0) goto error;
           close(devcheck);
        }

        break;

      case EEZE_UDEV_TYPE_DRIVE_INTERNAL:
        if (udev_device_get_property_value(device, "ID_FS_USAGE")) goto error;
        test = udev_device_get_sysattr_value(device, "removable");
        if (test && test[0] == '1') goto error;
        test = udev_device_get_property_value(device, "ID_BUS");
        if ((!test) || strcmp(test, "ata")) goto error;
        test = udev_device_get_property_value(device, "ID_TYPE");
        if (!(event & EEZE_UDEV_EVENT_CHANGE) && ((!test) || strcmp(test, "disk"))) goto error;
        break;

      case EEZE_UDEV_TYPE_DRIVE_REMOVABLE:
        if (udev_device_get_sysattr_value(device, "partition")) goto error;
        test = udev_device_get_sysattr_value(device, "removable");
        if ((!test) || (test[0] == '0')) goto error;
        test = udev_device_get_property_value(device, "ID_TYPE");
        if ((!test) || strcmp(test, "disk")) goto error;

        break;

      case EEZE_UDEV_TYPE_DRIVE_CDROM:
        if (!udev_device_get_property_value(device, "ID_CDROM"))
          goto error;

        break;

      case EEZE_UDEV_TYPE_POWER_AC:
        test = udev_device_get_property_value(device, "POWER_SUPPLY_ONLINE");
        if (!test) goto error;
        break;

      case EEZE_UDEV_TYPE_POWER_BAT:
        test = udev_device_get_property_value(device, "POWER_SUPPLY_PRESENT");
        if ((!test) || (strcmp(test, "1"))) goto error;
        break;

      case EEZE_UDEV_TYPE_NET:
        break;

      case EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR:
      {
        Eina_Bool one, two;
        const char *t;

        one = _walk_parents_test_attr(device, "temp1_input", NULL);
        two = _walk_parents_test_attr(device, "temp2_input", NULL);
        if ((!one) && (!two)) goto error;

        t = one ? "temp1_input" : "temp2_input";
        /* if device is not the one which has the temp input, we must go up the chain */
        if (!udev_device_get_sysattr_value(device, t))
          {
             for (parent = udev_device_get_parent(device); parent; parent = udev_device_get_parent(parent)) /*check for parent */
               if (udev_device_get_sysattr_value(parent, t))
                 {
                    tmpdev = device;

                    if (!(device = _copy_device(parent)))
                      goto error;

                    udev_device_unref(tmpdev);
                    break;
                 }
          }

        break;
      }
      case EEZE_UDEV_TYPE_V4L:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "video4linux")))
          goto error;
        break;

      case EEZE_UDEV_TYPE_BLUETOOTH:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "bluetooth")))
          goto error;
        break;

      case EEZE_UDEV_TYPE_DRM:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "drm")))
          goto error;

        test = udev_device_get_property_value(device, "HOTPLUG");
        if ((!test) || (strcmp(test, "1"))) goto error;

        break;

      case EEZE_UDEV_TYPE_BACKLIGHT:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "backlight")))
          goto error;
        break;

      case EEZE_UDEV_TYPE_LEDS:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "leds")))
          goto error;
        break;

      case EEZE_UDEV_TYPE_GRAPHICS:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "graphics")))
          goto error;
        break;
      case EEZE_UDEV_TYPE_GPIO:
        if ((!(test = udev_device_get_subsystem(device)))
            || (strcmp(test, "gpio")))
          goto error;
        break;

      default:
        break;
     }
out:
   (*func)(eina_stringshare_add(ret), event, sdata, watch);
error:
   if (device)
     udev_device_unref(device);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Initializes a udev monitor for a given watch.
 *
 * Sets up the udev monitor, applies necessary filters based on the
 * Eeze_Udev_Type, enables receiving, and adds an Ecore_Fd_Handler
 * to listen for events.
 *
 * @param watch The Eeze_Udev_Watch to initialize.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_watch_init(Eeze_Udev_Watch *watch)
{
   _udev_monitor *mon = NULL;
   int fd;
   Ecore_Fd_Handler *handler;
   if (!(mon = udev_monitor_new_from_netlink(udev, "udev")))
     goto error;

   switch (watch->type)
     {
      case EEZE_UDEV_TYPE_JOYSTICK:
      case EEZE_UDEV_TYPE_KEYBOARD:
      case EEZE_UDEV_TYPE_MOUSE:
      case EEZE_UDEV_TYPE_TOUCHPAD:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
        break;

      case EEZE_UDEV_TYPE_DRIVE_MOUNTABLE:
      case EEZE_UDEV_TYPE_DRIVE_INTERNAL:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
        break;

      case EEZE_UDEV_TYPE_DRIVE_REMOVABLE:
      case EEZE_UDEV_TYPE_DRIVE_CDROM:
        break;

      case EEZE_UDEV_TYPE_POWER_AC:
      case EEZE_UDEV_TYPE_POWER_BAT:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply",
                                                        NULL);
        break;

      case EEZE_UDEV_TYPE_NET:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "net", NULL);
        break;

      case EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "hwmon", NULL);
        break;

      /*
              case EEZE_UDEV_TYPE_ANDROID:
                udev_monitor_filter_add_match_subsystem_devtype(mon, "input", "usb_interface");
                break;
       */

      case EEZE_UDEV_TYPE_V4L:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "video4linux",
                                                        NULL);
        break;

      case EEZE_UDEV_TYPE_BLUETOOTH:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "bluetooth",
                                                        NULL);
        break;

      case EEZE_UDEV_TYPE_DRM:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "drm", NULL);
        break;

      case EEZE_UDEV_TYPE_GRAPHICS:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "graphics", NULL);
        break;
      case EEZE_UDEV_TYPE_GPIO:
        udev_monitor_filter_add_match_subsystem_devtype(mon, "gpio", NULL);
        break;

      default:
        break;
     }

   if (udev_monitor_enable_receiving(mon))
     goto error;

   fd = udev_monitor_get_fd(mon);
   watch->mon = mon;

   if (!(handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ,
                                             _get_syspath_from_watch, watch, NULL, NULL)))
     goto error;

   watch->handler = handler;
   return EINA_TRUE;
error:
   if (mon)
     udev_monitor_unref(mon);
   ERR("Could not create watch!");
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Resets all active udev watches.
 *
 * This function is typically called after a fork to re-initialize
 * udev monitors in the new process. It iterates through all active
 * watches, cleans up their existing monitor and fd_handler, and
 * then re-initializes them.
 *
 * @param data Unused.
 */
static void
_eeze_udev_watch_reset(void *data EINA_UNUSED)
{
   Eeze_Udev_Watch *watch;

   EINA_INLIST_FOREACH(watches, watch)
     {
        ecore_main_fd_handler_del(watch->handler);
        udev_monitor_unref(watch->mon);
        watch->handler = NULL;
        watch->mon = NULL;
        _watch_init(watch);
     }
}

/**
 * @brief Adds a new udev watch.
 *
 * Creates and starts a new watch for udev events of a specific type.
 *
 * @param type The type of udev device to watch (e.g., EEZE_UDEV_TYPE_KEYBOARD).
 * @param event A bitmask of events to watch for (e.g., EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE).
 *              Use EEZE_UDEV_EVENT_NONE to watch for all event types.
 * @param cb The callback function to be executed when an event occurs.
 *           The callback will receive:
 *           - const char *device_syspath: The syspath of the device.
 *           - Eeze_Udev_Event event_type: The type of event that occurred.
 *           - void *user_data: The user_data pointer passed to eeze_udev_watch_add().
 *           - Eeze_Udev_Watch *watch: The watch that triggered this callback.
 * @param user_data Custom data to be passed to the callback function.
 * @return A pointer to the new Eeze_Udev_Watch on success, or NULL on failure.
 *         This pointer is used to delete the watch later with eeze_udev_watch_del().
 *
 * @see Eeze_Udev_Type
 * @see Eeze_Udev_Event
 * @see Eeze_Udev_Watch_Cb
 * @see eeze_udev_watch_del()
 *
 * Example:
 * @code
 * void my_device_event_cb(const char *syspath, Eeze_Udev_Event event, void *data, Eeze_Udev_Watch *watch)
 * {
 *   if (event & EEZE_UDEV_EVENT_ADD)
 *     printf("Device added: %s\n", syspath);
 *   else if (event & EEZE_UDEV_EVENT_REMOVE)
 *     printf("Device removed: %s\n", syspath);
 * }
 *
 * // Watch for USB drive additions and removals
 * Eeze_Udev_Watch *usb_watch = eeze_udev_watch_add(EEZE_UDEV_TYPE_DRIVE_REMOVABLE,
 *                                                EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE,
 *                                                my_device_event_cb,
 *                                                NULL);
 * if (!usb_watch)
 *   fprintf(stderr, "Failed to add USB watch\n");
 * @endcode
 */
EAPI Eeze_Udev_Watch *
eeze_udev_watch_add(Eeze_Udev_Type     type,
                    int                event,
                    Eeze_Udev_Watch_Cb cb,
                    void              *user_data)
{
   Eeze_Udev_Watch *watch = NULL;

   watch = calloc(1, sizeof(Eeze_Udev_Watch));
   EINA_SAFETY_ON_NULL_RETURN_VAL(watch, NULL);

   watch->func = cb;
   watch->data = user_data;
   watch->type = type;
   watch->watch = watch;
   watch->event = event;
   if (!_watch_init(watch)) goto error;
   if (!watches)
     ecore_fork_reset_callback_add(_eeze_udev_watch_reset, NULL);
   watches = eina_inlist_append(watches, EINA_INLIST_GET(watch));
   return watch;
error:
   free(watch);
   return NULL;
}

/**
 * @brief Deletes a udev watch.
 *
 * Stops watching for udev events associated with the given watch and frees
 * allocated resources.
 *
 * @param watch The Eeze_Udev_Watch to delete, as returned by eeze_udev_watch_add().
 * @return The user_data pointer originally passed to eeze_udev_watch_add(),
 *         or NULL if the watch was NULL.
 *
 * @see eeze_udev_watch_add()
 *
 * Example:
 * @code
 * // Assuming usb_watch is a valid Eeze_Udev_Watch* from eeze_udev_watch_add()
 * void *user_data = eeze_udev_watch_del(usb_watch);
 * // user_data can now be freed if it was dynamically allocated
 * @endcode
 */
EAPI void *
eeze_udev_watch_del(Eeze_Udev_Watch *watch)
{
   void *ret = NULL;

   if (!watch)
     return NULL;

   ecore_main_fd_handler_del(watch->handler);
   udev_monitor_unref(watch->mon);

   ret = watch->data;
   watches = eina_inlist_remove(watches, EINA_INLIST_GET(watch));
   if (!watches)
     ecore_fork_reset_callback_del(_eeze_udev_watch_reset, NULL);
   free(watch);
   return ret;
}
