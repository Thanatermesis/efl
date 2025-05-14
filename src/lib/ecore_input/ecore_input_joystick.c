#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_EEZE
# if defined(__linux__)
#  include <linux/joystick.h>
# endif
# include "Eeze.h"
#endif
#include "Ecore.h"
#include "Ecore_Input.h"
#include "ecore_input_private.h"

static int _ecore_input_joystick_init_count = 0; /**< Counter for joystick subsystem initialization. */
static int _event_axis_deadzone = 200; /**< Deadzone value for joystick axis events. Axis events with absolute value less than this will be ignored. */

#ifdef HAVE_EEZE

/**
 * @brief Function pointer type for joystick event mapping.
 * @param event The raw joystick event.
 * @param e The Ecore joystick event to be filled.
 */
typedef void (*Joystick_Mapper)(struct js_event *event, Ecore_Event_Joystick *e);
static void _joystick_xbox360_mapper(struct js_event *event, Ecore_Event_Joystick *e);
static void _joystick_xboxone_mapper(struct js_event *event, Ecore_Event_Joystick *e);
static void _joystick_ps4_mapper(struct js_event *event, Ecore_Event_Joystick *e);

/**
 * @brief Structure to hold information about a connected joystick.
 */
struct _Joystick_Info
{
   Ecore_Fd_Handler *fd_handler; /**< File descriptor handler for the joystick device. */
   Eina_Stringshare *system_path; /**< System path of the joystick device. */
   int index; /**< Index of the joystick. */
   Joystick_Mapper mapper; /**< Mapper function for this joystick type. */
};
typedef struct _Joystick_Info Joystick_Info;

/**
 * @brief Structure to map vendor and product IDs to specific joystick mappers.
 */
struct _Joystick_Mapping_Info
{
   const char *vendor; /**< Vendor ID string. */
   const char *product; /**< Product ID string. */
   Joystick_Mapper mapper; /**< Mapper function for this joystick. */
} Joystick_Mapping_Info[] = {
   {"045e", "028e", _joystick_xbox360_mapper}, /* Microsoft X-Box 360 pad */
   {"045e", "02dd", _joystick_xboxone_mapper}, /* Microsoft X-Box One pad (Covert Forces) */
   {"054c", "05c4", _joystick_ps4_mapper} /* Sony Computer Entertainment Wireless Controller */
};

static const char joystickPrefix[] = "/dev/input/js"; /**< Prefix for joystick device nodes. */
static Eina_List *joystick_list; /**< List of currently connected and managed joysticks (_Joystick_Info). */
static Eeze_Udev_Watch *watch = NULL; /**< Udev watch for joystick hotplug events. */

/**
 * @brief Adds a joystick connected or disconnected event.
 * @param index The index of the joystick.
 * @param connected EINA_TRUE if connected, EINA_FALSE if disconnected.
 */
static void
_joystick_connected_event_add(int index, Eina_Bool connected)
{
   Ecore_Event_Joystick *e;
   if (!(e = calloc(1, sizeof(Ecore_Event_Joystick)))) return;

   e->index = index;
   if (connected)
     e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_CONNECTED;
   else
     e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_DISCONNECTED;

   INF("index: %d, connected: %d", index, connected);
   ecore_event_add(ECORE_EVENT_JOYSTICK, e, NULL, NULL);
}

/**
 * @brief Maps raw PS4 controller events to Ecore_Event_Joystick events.
 * @param event The raw joystick event from the PS4 controller.
 * @param e The Ecore_Event_Joystick structure to populate.
 */
static void
_joystick_ps4_mapper(struct js_event *event, Ecore_Event_Joystick *e)
{
   if (event->type == JS_EVENT_BUTTON)
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_BUTTON;
        e->button.value = event->value;
        switch (event->number)
          {
           case 0:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_0;
             break;

           case 1:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_1;
             break;

           case 2:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_2;
             break;

           case 3:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_3;
             break;

           case 4:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_SHOULDER;
             break;

           case 5:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_SHOULDER;
             break;

           case 8:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_SELECT;
             break;

           case 9:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_META;
             break;

           case 10:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_ANALOG_STICK;
             break;

           case 11:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_ANALOG_STICK;
             break;

           case 12:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_START;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
   else
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_AXIS;
        e->axis.value = event->value / 32767.0f;;
        switch (event->number)
          {
           case 0:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_HOR;
             break;

           case 1:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_VER;
             break;

           case 2:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_HOR;
             break;

           case 3:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_SHOULDER;
             break;

           case 4:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_SHOULDER;
             break;

           case 5:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_VER;
             break;

           case 6:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_X;
             break;

           case 7:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_Y;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
}

/**
 * @brief Maps raw Xbox 360 controller events to Ecore_Event_Joystick events.
 * @param event The raw joystick event from the Xbox 360 controller.
 * @param e The Ecore_Event_Joystick structure to populate.
 */
static void
_joystick_xbox360_mapper(struct js_event *event, Ecore_Event_Joystick *e)
{
   if (event->type == JS_EVENT_BUTTON)
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_BUTTON;
        e->button.value = event->value;
        switch (event->number)
          {
           case 0:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_0;
             break;

           case 1:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_1;
             break;

           case 2:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_2;
             break;

           case 3:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_3;
             break;

           case 4:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_SHOULDER;
             break;

           case 5:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_SHOULDER;
             break;

           case 6:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_SELECT;
             break;

           case 7:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_START;
             break;

           case 8:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_META;
             break;

           case 9:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_ANALOG_STICK;
             break;

           case 10:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_ANALOG_STICK;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
   else
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_AXIS;
        e->axis.value = event->value / 32767.0f;;
        switch (event->number)
          {
           case 0:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_HOR;
             break;

           case 1:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_VER;
             break;

           case 2:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_SHOULDER;
             break;

           case 3:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_HOR;
             break;

           case 4:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_VER;
             break;

           case 5:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_SHOULDER;
             break;

           case 6:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_X;
             break;

           case 7:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_Y;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
}

/**
 * @brief Maps raw Xbox One controller events to Ecore_Event_Joystick events.
 * @param event The raw joystick event from the Xbox One controller.
 * @param e The Ecore_Event_Joystick structure to populate.
 */
static void
_joystick_xboxone_mapper(struct js_event *event, Ecore_Event_Joystick *e)
{
   if (event->type == JS_EVENT_BUTTON)
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_BUTTON;
        e->button.value = event->value;
        switch (event->number)
          {
           case 0:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_0;
             break;

           case 1:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_1;
             break;

           case 2:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_2;
             break;

           case 3:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_FACE_3;
             break;

           case 4:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_SHOULDER;
             break;

           case 5:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_SHOULDER;
             break;

           case 6:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_META;
             break;

           case 7:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_SELECT;
             break;

           case 8:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_START;
             break;

           case 9:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_LEFT_ANALOG_STICK;
             break;

           case 10:
             e->button.index = ECORE_EVENT_JOYSTICK_BUTTON_RIGHT_ANALOG_STICK;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
   else
     {
        e->type = ECORE_EVENT_JOYSTICK_EVENT_TYPE_AXIS;
        e->axis.value = event->value / 32767.0f;;
        switch (event->number)
          {
           case 0:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_HOR;
             break;

           case 1:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_ANALOG_VER;
             break;

           case 2:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_LEFT_SHOULDER;
             break;

           case 3:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_HOR;
             break;

           case 4:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_ANALOG_VER;
             break;

           case 5:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_RIGHT_SHOULDER;
             break;

           case 6:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_X;
             break;

           case 7:
             e->axis.index = ECORE_EVENT_JOYSTICK_AXIS_HAT_Y;
             break;

           default:
             ERR("Unsupported joystick event: %d", event->number);
             break;
          }
     }
}

/**
 * @brief Processes a raw joystick event, maps it, and adds it to the Ecore event queue.
 * @param event The raw joystick event.
 * @param ji Information about the joystick that generated the event.
 */
static void
_joystick_event_add(struct js_event *event, Joystick_Info *ji)
{
   Ecore_Event_Joystick *e;

   if ((event->type != JS_EVENT_BUTTON) && (event->type != JS_EVENT_AXIS)) return;
   if ((event->type == JS_EVENT_AXIS) &&
       ((event->value != 0) && (abs(event->value) < _event_axis_deadzone)))
     {
        INF("axis event value(%d) is less than deadzone(%d)\n",
            event->value,_event_axis_deadzone);
        return;
     }
   if (!(e = calloc(1, sizeof(Ecore_Event_Joystick)))) return;
   e->index = ji->index;
   e->timestamp = event->time;

   ji->mapper(event, e);

   ecore_event_add(ECORE_EVENT_JOYSTICK, e, NULL, NULL);
}

/**
 * @brief Callback function for Ecore_Fd_Handler to read joystick events.
 * @param userData Pointer to Joystick_Info for this joystick.
 * @param fdHandler The Ecore_Fd_Handler that triggered the callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active, ECORE_CALLBACK_CANCEL to remove it.
 */
static Eina_Bool
_fd_handler_cb(void* userData, Ecore_Fd_Handler* fdHandler)
{
   int fd;
   Joystick_Info *ji = userData;
   struct js_event event;
   ssize_t len;

   fd = ecore_main_fd_handler_fd_get(fdHandler);
   if (fd < 0) return ECORE_CALLBACK_RENEW;

   len = read(fd, &event, sizeof(event));
   if (len == -1) return ECORE_CALLBACK_RENEW;

   INF("index: %d, type: %d, number: %d, value: %d",
       ji->index, event.type, event.number, event.value);

   _joystick_event_add(&event, ji);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Retrieves the appropriate joystick mapper function based on vendor and product ID.
 * @param syspath The system path of the joystick device.
 * @return A Joystick_Mapper function pointer if a mapping is found, otherwise NULL.
 * @note This function inspects the parent device in sysfs to get vendor/product IDs.
 */
static Joystick_Mapper
_joystick_mapping_info_get(const char* syspath)
{
   int index, mapping_info_size;
   const char *parent, *vendor, *product;
   Joystick_Mapper ret;

   ret = NULL;
   parent = eeze_udev_syspath_get_parent_filtered(syspath, "input", NULL);
   vendor = eeze_udev_syspath_get_sysattr(parent, "id/vendor");
   product = eeze_udev_syspath_get_sysattr(parent, "id/product");

   mapping_info_size = (int)(sizeof(Joystick_Mapping_Info) / sizeof(Joystick_Mapping_Info[0]));
   for (index = 0; index < mapping_info_size; index++)
     {
        if ((vendor && !strcmp(vendor, Joystick_Mapping_Info[index].vendor)) &&
            (product && !strcmp(product, Joystick_Mapping_Info[index].product)))
          {
             INF("joystick mapping info found (vendor: %s, product: %s)", vendor, product);
             ret = Joystick_Mapping_Info[index].mapper;
             break;
          }
     }

   eina_stringshare_del(parent);
   eina_stringshare_del(vendor);
   eina_stringshare_del(product);

   return ret;
}

/**
 * @brief Extracts the joystick index from its device node path (e.g., /dev/input/jsX).
 * @param dev The device node path string (e.g., "/dev/input/js0").
 * @return The joystick index (e.g., 0 for "/dev/input/js0"), or -1 on failure.
 */
static int
_joystick_index_get(const char *dev)
{
   int plen, dlen, diff, ret = -1;

   dlen = strlen(dev);
   plen = strlen(joystickPrefix);
   diff = dlen - plen;

   if (diff > 0)
     {
        ret = atoi(dev + plen);
     }

   return ret;
}

/**
 * @brief Registers a new joystick device.
 *
 * This function is called when a new joystick is detected (e.g., via udev).
 * It opens the device, sets up an fd handler for reading events,
 * and adds it to the internal list of joysticks.
 *
 * @param syspath The system path of the joystick device to register.
 */
static void
_joystick_register(const char* syspath)
{
   int fd, index;
   const char *devnode;
   Joystick_Info *ji;
   Joystick_Mapper mapper;

   devnode = eeze_udev_syspath_get_devpath(syspath);
   if (!devnode) return;
   if (!eina_str_has_prefix(devnode, joystickPrefix)) goto register_failed;

   mapper = _joystick_mapping_info_get(syspath);
   if (!mapper)
     {
        WRN("Unsupported joystick.");
        goto register_failed;
     }

   index = _joystick_index_get(devnode);
   if (index == -1)
     {
        ERR("Invalid index value.");
        goto register_failed;
     }

   ji = calloc(1, sizeof(Joystick_Info));
   if (!ji)
     {
        ERR("Cannot allocate memory.");
        goto register_failed;
     }

   ji->index = index;
   ji->mapper = mapper;
   ji->system_path = eina_stringshare_ref(syspath);

   fd = open(devnode, O_RDONLY | O_NONBLOCK);
   ji->fd_handler = ecore_main_fd_handler_add(fd, ECORE_FD_READ,
                                             _fd_handler_cb, ji, 0, 0);

   joystick_list = eina_list_append(joystick_list, ji);
   _joystick_connected_event_add(index, EINA_TRUE);

register_failed:
   eina_stringshare_del(devnode);
}

/**
 * @brief Unregisters a joystick device.
 *
 * This function is called when a joystick is removed.
 * It closes the device, removes the fd handler, and cleans up associated resources.
 *
 * @param syspath The system path of the joystick device to unregister.
 */
static void
_joystick_unregister(const char *syspath)
{
   int fd;
   Eina_List *l, *l2;
   Joystick_Info *ji;

   EINA_LIST_FOREACH_SAFE(joystick_list, l, l2, ji)
     {
        if (syspath == ji->system_path)
          {
             fd = ecore_main_fd_handler_fd_get(ji->fd_handler);
             if (fd < 0) continue;

             close(fd);
             ecore_main_fd_handler_del(ji->fd_handler);
             joystick_list = eina_list_remove(joystick_list, ji);
             _joystick_connected_event_add(ji->index, EINA_FALSE);
             eina_stringshare_del(ji->system_path);
             free(ji);
             break;
          }
     }
}

/**
 * @brief Callback for udev watch events (joystick add/remove).
 * @param syspath The system path of the device that triggered the event.
 * @param event The type of udev event (add, remove, etc.).
 * @param data User data associated with the watch (unused here).
 * @param w The Eeze_Udev_Watch that triggered the callback (unused here).
 */
static void
_watch_cb(const char *syspath, Eeze_Udev_Event  event,
          void *data EINA_UNUSED, Eeze_Udev_Watch *w EINA_UNUSED)
{
   switch (event) {
   case EEZE_UDEV_EVENT_ADD:
       _joystick_register(syspath);
       break;
   case EEZE_UDEV_EVENT_REMOVE:
        _joystick_unregister(syspath);
       break;
   default:
       break;
   }

   eina_stringshare_del(syspath);
}
#endif

/**
 * @brief Initializes the Ecore input joystick subsystem.
 *
 * This function sets up udev monitoring for joystick devices if Eeze is available.
 * It should be called before any other ecore_input_joystick functions.
 *
 * @return The new reference count for the joystick subsystem.
 *         Returns 0 on failure or if Eeze initialization fails.
 * @see ecore_input_joystick_shutdown()
 */
int
ecore_input_joystick_init(void)
{
#ifdef HAVE_EEZE
   Eina_List *syspaths;
   const char *syspath;

   if (++_ecore_input_joystick_init_count != 1)
     return _ecore_input_joystick_init_count;

   if (!eeze_init())
     return --_ecore_input_joystick_init_count;

   watch = eeze_udev_watch_add(EEZE_UDEV_TYPE_JOYSTICK,
                               (EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE),
                               _watch_cb, NULL);

   syspaths = eeze_udev_find_by_type(EEZE_UDEV_TYPE_JOYSTICK, NULL);
   EINA_LIST_FREE(syspaths, syspath)
     {
        _joystick_register(syspath);
        eina_stringshare_del(syspath);
     }
#endif

   return _ecore_input_joystick_init_count;
}

/**
 * @brief Shuts down the Ecore input joystick subsystem.
 *
 * This function cleans up resources used by the joystick subsystem,
 * including stopping udev monitoring.
 *
 * @return The new reference count for the joystick subsystem.
 * @see ecore_input_joystick_init()
 */
int
ecore_input_joystick_shutdown(void)
{
#ifdef HAVE_EEZE
   if (--_ecore_input_joystick_init_count != 0)
      return _ecore_input_joystick_init_count;

   if (watch)
     {
        eeze_udev_watch_del(watch);
        watch = NULL;
     }
   eeze_shutdown();
#endif

   return _ecore_input_joystick_init_count;
}

EAPI void
ecore_input_joystick_event_axis_deadzone_set(int event_axis_deadzone)
{
   event_axis_deadzone = abs(event_axis_deadzone);
   if (event_axis_deadzone > 32767) event_axis_deadzone = 32767;

   _event_axis_deadzone = event_axis_deadzone;
}

EAPI int
ecore_input_joystick_event_axis_deadzone_get(void)
{
   return _event_axis_deadzone;
}

EAPI const char *
ecore_input_joystick_name_get(int index)
{
#if defined(HAVE_EEZE) && defined(JSIOCGNAME)
   int fd;
   char name[128];
   Eina_List *l;
   Joystick_Info *ji;

   EINA_LIST_FOREACH(joystick_list, l, ji)
     {
        if (index == ji->index)
          {
             fd = ecore_main_fd_handler_fd_get(ji->fd_handler);
             if (fd < 0) return NULL;

             if (ioctl(fd, JSIOCGNAME(sizeof(name)), name) < 0)
                strncpy(name, "Unknown", sizeof(name));
             return eina_slstr_copy_new(name);
          }
     }
#else
   (void) index;
#endif
   return NULL;
}
