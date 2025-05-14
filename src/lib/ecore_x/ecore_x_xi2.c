#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <string.h>
#include <math.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"

#ifdef ECORE_XI2
#include "Ecore_Input.h"
#endif /* ifdef ECORE_XI2 */

/**
 * @internal
 * @brief Opcode for the XInputExtension.
 *
 * This variable stores the opcode for the XInputExtension, which is obtained
 * by querying the X server. It is used to identify XInput2 events.
 * A value of -1 indicates that the XInputExtension is not available or
 * has not been initialized.
 */
int _ecore_x_xi2_opcode = -1;

#ifndef XIPointerEmulated
#define XIPointerEmulated (1 << 16)
#endif

#ifdef ECORE_XI2
#ifdef ECORE_XI2_2
#ifndef XITouchEmulatingPointer
#define XITouchEmulatingPointer (1 << 17)
#endif

/**
 * @internal
 * @brief Structure to store information about a touch device.
 *
 * This structure holds details specific to an XInput2 touch device,
 * including its ID, mode of operation (direct or dependent), name,
 * maximum number of touch points, and an array to track active touch slots.
 */
typedef struct _Ecore_X_Touch_Device_Info
{
   EINA_INLIST; /**< Macro for Eina Inlist integration. */
   int devid; /**< The XInput device ID. */
   int mode;  /**< The touch mode (XIDependentTouch or XIDirectTouch). */
   const char *name; /**< The name of the touch device. */
   int max_touch; /**< Maximum number of simultaneous touch points supported. */
   int *slot; /**< Array to map touch detail IDs to touch point indices. Each element stores a detail ID or -1 if the slot is free. */
} Ecore_X_Touch_Device_Info;
#endif /* ifdef ECORE_XI2_2 */

static Ecore_Job *update_devices_job = NULL;
static XIDeviceInfo *_ecore_x_xi2_devs = NULL;
static int _ecore_x_xi2_num = 0;
#ifdef ECORE_XI2_2
static Eina_Inlist *_ecore_x_xi2_touch_info_list = NULL;
#endif /* ifdef ECORE_XI2_2 */
static Eina_List *_ecore_x_xi2_grabbed_devices_list;
#endif /* ifdef ECORE_XI2 */

/**
 * @internal
 * @brief Initializes the XInput2 extension.
 *
 * Queries the X server for the XInputExtension and its version.
 * If available and compatible, it registers to listen for device changes,
 * hierarchy changes, and property events on all devices. It also queries
 * the initial list of available XInput2 devices.
 */
void
_ecore_x_input_init(void)
{
#ifdef ECORE_XI2
   int event, error;
   int major = XI_2_Major, minor = XI_2_Minor;

   if (!XQueryExtension(_ecore_x_disp, "XInputExtension",
                        &_ecore_x_xi2_opcode, &event, &error))
     {
        _ecore_x_xi2_opcode = -1;
        return;
     }

   if (XIQueryVersion(_ecore_x_disp, &major, &minor) == BadRequest)
     {
        _ecore_x_xi2_opcode = -1;
        return;
     }

   // listen for device changes
   XIEventMask m;
   m.deviceid = XIAllDevices;
   m.mask_len = XIMaskLen(XI_LASTEVENT);
   m.mask = calloc( m.mask_len, 1);
   if (!m.mask) return;
   XISetMask(m.mask, XI_DeviceChanged);
   XISetMask(m.mask, XI_HierarchyChanged);
   XISetMask(m.mask, XI_PropertyEvent);
   XISelectEvents(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp), &m, 1);
   free(m.mask);

   _ecore_x_xi2_devs = XIQueryDevice(_ecore_x_disp, XIAllDevices,
                                     &_ecore_x_xi2_num);
#endif /* ifdef ECORE_XI2 */
}

#ifdef ECORE_XI2
#ifdef ECORE_XI2_2
/**
 * @internal
 * @brief Clears all stored touch device information.
 *
 * Iterates through the list of known touch devices and frees all
 * associated memory, including the slot arrays. This is typically
 * called during shutdown or when device information needs to be refreshed.
 */
static void
_ecore_x_input_touch_info_clear(void)
{
   Eina_Inlist *l = _ecore_x_xi2_touch_info_list;
   Ecore_X_Touch_Device_Info *info = NULL;

   while (l)
     {
        info = EINA_INLIST_CONTAINER_GET(l, Ecore_X_Touch_Device_Info);
        l = eina_inlist_remove(l, l);
        if (info->slot) free(info->slot);
        free(info);
     }

   _ecore_x_xi2_touch_info_list = NULL;
}
#endif /* ifdef ECORE_XI2_2 */
#endif /* ifdef ECORE_XI2 */

#ifdef ECORE_XI2
/**
 * @internal
 * @brief Retrieves the X Atom for a given axis label string.
 *
 * This function maps human-readable axis names (e.g., "Abs X", "Abs Pressure")
 * to their corresponding X Atoms. It caches the atoms for efficiency.
 *
 * @param axis_name The string name of the axis.
 * @return The X Atom corresponding to the axis_name, or 0 if not found or on error.
 *
 * Example axis names:
 * - "Abs X"
 * - "Abs Y"
 * - "Abs Pressure"
 * - "Rel X"
 */
static Atom
_ecore_x_input_get_axis_label(char *axis_name)
{
   static Atom *atoms = NULL;
   static char *names[] =
     {
        "Abs X", "Abs Y", "Abs Pressure",
        "Abs Distance", "Abs Rotary Z",
        "Abs Wheel", "Abs Tilt X", "Abs Tilt Y",
        "Rel X", "Rel Y", "Rel Dial", "Rel Horiz Wheel", "Rel Vert Wheel"
     };
   int n = sizeof(names) / sizeof(names[0]);
   int i;

   if (EINA_UNLIKELY(atoms == NULL))
     {
        atoms = calloc(n, sizeof(Atom));
        if (!atoms) return 0;

        if (!XInternAtoms(_ecore_x_disp, names, n, 1, atoms))
          {
             free(atoms);
             atoms = NULL;
             return 0;
          }
     }

   for (i = 0; i < n; i++)
     {
        if (!strcmp(axis_name, names[i])) return atoms[i];
     }

   return 0;
}
#endif /* ifdef ECORE_XI2 */

/**
 * @internal
 * @brief Shuts down the XInput2 extension handling.
 *
 * Unregisters event listeners, frees device information, and cleans up
 * any resources allocated for XInput2. This includes freeing the list
 * of XInput2 devices, clearing touch device information, and canceling
 * any pending device update jobs.
 */
void
_ecore_x_input_shutdown(void)
{
   XIEventMask m;
   m.deviceid = XIAllDevices;
   m.mask_len = XIMaskLen(XI_LASTEVENT);
   m.mask = calloc( m.mask_len, 1);
   if (m.mask)
     {
        XISelectEvents(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp), &m, 1);
        free(m.mask);
     }
#ifdef ECORE_XI2
   if (_ecore_x_xi2_devs)
     {
        XIFreeDeviceInfo(_ecore_x_xi2_devs);
        _ecore_x_xi2_devs = NULL;
#ifdef ECORE_XI2_2
        _ecore_x_input_touch_info_clear();
#endif /* ifdef ECORE_XI2_2 */
     }

   _ecore_x_xi2_num = 0;
   _ecore_x_xi2_opcode = -1;

   if (_ecore_x_xi2_grabbed_devices_list)
     eina_list_free(_ecore_x_xi2_grabbed_devices_list);
   _ecore_x_xi2_grabbed_devices_list = NULL;
#endif /* ifdef ECORE_XI2 */
   if (update_devices_job) ecore_job_del(update_devices_job);
   update_devices_job = NULL;
}

#ifdef ECORE_XI2
#ifdef ECORE_XI2_2

# ifdef XI_TouchCancel
/**
 * @internal
 * @brief Checks if a device ID corresponds to a known touch device.
 *
 * Iterates through the cached list of touch device information to see
 * if the given device ID is present.
 *
 * @param devid The XInput device ID to check.
 * @return EINA_TRUE if the device is a known touch device, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_x_input_touch_device_check(int devid)
{
   Eina_Inlist *l = _ecore_x_xi2_touch_info_list;
   Ecore_X_Touch_Device_Info *info = NULL;

   if ((!_ecore_x_xi2_devs) || (!_ecore_x_xi2_touch_info_list))
     return EINA_FALSE;

   EINA_INLIST_FOREACH(l, info)
     if (info->devid == devid) return EINA_TRUE;
   return EINA_FALSE;
}
#endif

/**
 * @internal
 * @brief Gets or assigns a touch point index for a given touch event.
 *
 * For a given touch device ID and event detail (touch ID from the hardware),
 * this function finds an existing touch point index or assigns a new one
 * if the event is XI_TouchBegin and a slot is available.
 *
 * @param devid The XInput device ID of the touch device.
 * @param detail The detail ID from the XITouchEvent (distinguishes simultaneous touches).
 * @param event_type The type of the touch event (e.g., XI_TouchBegin, XI_TouchUpdate).
 * @return The touch point index (0 to max_touch - 1). Returns 0 if the device
 *         is not found, not a touch device, or no slot is available.
 */
static int
_ecore_x_input_touch_index_get(int devid, int detail, int event_type)
{
   int i;
   Eina_Inlist *l = _ecore_x_xi2_touch_info_list;
   Ecore_X_Touch_Device_Info *info = NULL;

   if ((!_ecore_x_xi2_devs) || (!_ecore_x_xi2_touch_info_list))
     return 0;

   EINA_INLIST_FOREACH(l, info)
     if (info->devid == devid) break;

   if ((!info) || (!info->slot)) return 0;

   for (i = 0; i < info->max_touch ; i++)
     {
        int *p = &(info->slot[i]);

        if ((event_type == XI_TouchBegin) && (*p < 0))
          {
             *p = detail;
             return i;
          }
       else if (*p == detail)
         {
            return i;
         }
     }

   return 0;
}

/**
 * @internal
 * @brief Clears a touch point index for a given touch device.
 *
 * Marks the specified touch point index (slot) as free for the given
 * touch device ID. This is typically called when a touch ends (XI_TouchEnd).
 *
 * @param devid The XInput device ID of the touch device.
 * @param idx The touch point index to clear.
 */
static void
_ecore_x_input_touch_index_clear(int devid, int idx)
{
   Eina_Inlist *l = _ecore_x_xi2_touch_info_list;
   Ecore_X_Touch_Device_Info *info = NULL;

   if ((!_ecore_x_xi2_devs) || (!_ecore_x_xi2_touch_info_list))
     return;

   EINA_INLIST_FOREACH(l, info)
     {
        if ((info->devid == devid) && (info->slot))
          {
             info->slot[idx] = -1;
             return;
          }
     }
}

/**
 * @internal
 * @brief Extracts and stores touch-specific information from an XIDeviceInfo structure.
 *
 * If the given XIDeviceInfo represents a touch device (has an XITouchClass),
 * this function allocates an Ecore_X_Touch_Device_Info structure, populates it
 * with details like device ID, touch mode, name, maximum touch points, and
 * initializes the slot array for tracking touch points.
 *
 * @param dev Pointer to the XIDeviceInfo structure for the device.
 * @return A pointer to a newly allocated Ecore_X_Touch_Device_Info structure
 *         if the device is a touch device, NULL otherwise or on allocation failure.
 *         The caller is responsible for freeing the returned structure if it's not
 *         added to the global `_ecore_x_xi2_touch_info_list`.
 */
static Ecore_X_Touch_Device_Info *
_ecore_x_input_touch_info_get(XIDeviceInfo *dev)
{
   int k;
   int *slot = NULL;
   XITouchClassInfo *t = NULL;
   Ecore_X_Touch_Device_Info *info = NULL;

   if (!dev)
     return NULL;

   for (k = 0; k < dev->num_classes; k++)
     {
        XIAnyClassInfo *clas = dev->classes[k];

        if (clas && (clas->type == XITouchClass))
          {
             t = (XITouchClassInfo *)clas;
             break;
          }
     }

   if (t && (t->type == XITouchClass))
     {
        info = calloc(1, sizeof(Ecore_X_Touch_Device_Info));
        if (!info) return NULL;

        slot = malloc(sizeof(int) * (t->num_touches + 1));
        if (!slot)
          {
             free(info);
             return NULL;
          }

        info->devid = dev->deviceid;
        info->max_touch = t->num_touches + 1;
        info->mode = t->mode;
        info->name = dev->name;
        memset(slot, -1, sizeof(int) * info->max_touch);
        info->slot = slot;
     }

   return info;
}
#endif /* ifdef ECORE_XI2_2 */
#endif

/**
 * @internal
 * @brief Handles raw XInput2 events.
 *
 * This function processes raw XInput2 events such as XI_RawButtonPress,
 * XI_RawButtonRelease, and XI_RawMotion. It converts these X events into
 * corresponding Ecore_X raw events.
 *
 * @param xevent Pointer to the XEvent structure.
 */
void
_ecore_x_input_raw_handler(XEvent *xevent)
{
#ifdef ECORE_XI2
   if (xevent->type != GenericEvent) return;

   switch (xevent->xcookie.evtype)
     {
#ifdef XI_RawButtonPress
      case XI_RawButtonPress:
         ecore_event_add(ECORE_X_RAW_BUTTON_PRESS, NULL, NULL, NULL);
         break;
#endif
#ifdef XI_RawButtonRelease
      case XI_RawButtonRelease:
         ecore_event_add(ECORE_X_RAW_BUTTON_RELEASE, NULL, NULL, NULL);
         break;
#endif
#ifdef XI_RawMotion
      case XI_RawMotion:
         ecore_event_add(ECORE_X_RAW_MOTION, NULL, NULL, NULL);
         break;
#endif
     }
#endif /* ifdef ECORE_XI2 */
}

#ifdef ECORE_XI2_2
/**
 * @internal
 * @brief Checks if a specific device ID is in the list of grabbed devices.
 *
 * @param deviceId The ID of the device to check.
 * @return EINA_TRUE if the device is currently grabbed, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_x_input_grabbed_is(int deviceId)
{
   void *id;
   Eina_List *l;

   EINA_LIST_FOREACH(_ecore_x_xi2_grabbed_devices_list, l, id)
     {
        if (deviceId == (intptr_t)id)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}
#endif /* ifdef ECORE_XI2_2 */

/**
 * @internal
 * @brief Handles XInput2 mouse events.
 *
 * This function processes XInput2 events related to mouse actions,
 * such as XI_Motion, XI_ButtonPress, and XI_ButtonRelease, for devices
 * that are not primarily touch devices (e.g., traditional mice,
 * or touch devices emulating a pointer when not grabbed for touch).
 * It converts these X events into Ecore mouse events.
 *
 * @param xevent Pointer to the XEvent structure.
 */
void
_ecore_x_input_mouse_handler(XEvent *xevent)
{
#ifdef ECORE_XI2
   if (xevent->type != GenericEvent) return;

   XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
   int devid = evd->deviceid;

   switch (xevent->xcookie.evtype)
     {
      case XI_Motion:
        INF("Handling XI_Motion");
        _ecore_mouse_move
          (evd->time,
          0,   // state
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y,
          evd->event,
          (evd->child ? evd->child : evd->event),
          evd->root,
          1,   // same_screen
          devid, 1, 1,
          1.0,   // pressure
          0.0,   // angle
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y);
        break;

      case XI_ButtonPress:
        INF("ButtonEvent:multi press time=%u x=%d y=%d devid=%d", (unsigned int)evd->time, (int)evd->event_x, (int)evd->event_y, devid);
        _ecore_mouse_button
          (ECORE_EVENT_MOUSE_BUTTON_DOWN,
          evd->time,
          0,   // state
          0,   // button
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y,
          evd->event,
          (evd->child ? evd->child : evd->event),
          evd->root,
          1,   // same_screen
          devid, 1, 1,
          1.0,   // pressure
          0.0,   // angle
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y);
        break;

      case XI_ButtonRelease:
        INF("ButtonEvent:multi release time=%u x=%d y=%d devid=%d", (unsigned int)evd->time, (int)evd->event_x, (int)evd->event_y, devid);
        _ecore_mouse_button
          (ECORE_EVENT_MOUSE_BUTTON_UP,
          evd->time,
          0,   // state
          0,   // button
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y,
          evd->event,
          (evd->child ? evd->child : evd->event),
          evd->root,
          1,   // same_screen
          devid, 1, 1,
          1.0,   // pressure
          0.0,   // angle
          evd->event_x, evd->event_y,
          evd->root_x, evd->root_y);
        break;
      }
#endif /* ifdef ECORE_XI2 */
}

//XI_TouchUpdate, XI_TouchBegin, XI_TouchEnd only available in XI2_2
//So it is better using ECORE_XI2_2 define than XI_TouchXXX defines.
/**
 * @internal
 * @brief Handles XInput2 multi-touch events.
 *
 * This function processes XInput2 events specific to multi-touch devices,
 * such as XI_TouchUpdate, XI_TouchBegin, and XI_TouchEnd. It maps these
 * events to Ecore mouse move and button events, using touch indices to
 * differentiate between multiple touch points.
 * It also handles the XITouchEmulatingPointer flag to avoid duplicate events
 * when a touch device is emulating a mouse pointer and has not been specifically
 * grabbed for touch input.
 *
 * @param xevent Pointer to the XEvent structure.
 */
void
_ecore_x_input_multi_handler(XEvent *xevent)
{
#ifdef ECORE_XI2
   if (xevent->type != GenericEvent) return;

   switch (xevent->xcookie.evtype)
     {
#ifdef ECORE_XI2_2
      case XI_TouchUpdate:
          {
             XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
             int devid = evd->deviceid;
             int i = _ecore_x_input_touch_index_get(devid, evd->detail, XI_TouchUpdate);
             if ((i == 0) && (evd->flags & XITouchEmulatingPointer) && !_ecore_x_input_grabbed_is(devid)) return;
             INF("Handling XI_TouchUpdate");
             _ecore_mouse_move(evd->time,
                               0,   // state
                               evd->event_x, evd->event_y,
                               evd->root_x, evd->root_y,
                               evd->event,
                               (evd->child ? evd->child : evd->event),
                               evd->root,
                               1,   // same_screen
                               i, 1, 1,
                               1.0,   // pressure
                               0.0,   // angle
                               evd->event_x, evd->event_y,
                               evd->root_x, evd->root_y);
          }
        break;

      case XI_TouchBegin:
          {
             XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
             int devid = evd->deviceid;
             int i = _ecore_x_input_touch_index_get(devid, evd->detail, XI_TouchBegin);
             if ((i == 0) && (evd->flags & XITouchEmulatingPointer) && !_ecore_x_input_grabbed_is(devid)) return;
             INF("Handling XI_TouchBegin");
             _ecore_mouse_button(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                 evd->time,
                                 0,   // state
                                 0,   // button
                                 evd->event_x, evd->event_y,
                                 evd->root_x, evd->root_y,
                                 evd->event,
                                 (evd->child ? evd->child : evd->event),
                                 evd->root,
                                 1,   // same_screen
                                 i, 1, 1,
                                 1.0,   // pressure
                                 0.0,   // angle
                                 evd->event_x, evd->event_y,
                                 evd->root_x, evd->root_y);
          }
        break;

      case XI_TouchEnd:
          {
             XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
             int devid = evd->deviceid;
             int i = _ecore_x_input_touch_index_get(devid, evd->detail, XI_TouchEnd);
             if ((i == 0) && (evd->flags & XITouchEmulatingPointer) && !_ecore_x_input_grabbed_is(devid))
               {
                  _ecore_x_input_touch_index_clear(devid,  i);
                  return;
               }
             INF("Handling XI_TouchEnd");
             _ecore_mouse_button(ECORE_EVENT_MOUSE_BUTTON_UP,
                                 evd->time,
                                 0,   // state
                                 0,   // button
                                 evd->event_x, evd->event_y,
                                 evd->root_x, evd->root_y,
                                 evd->event,
                                 (evd->child ? evd->child : evd->event),
                                 evd->root,
                                 1,   // same_screen
                                 i, 1, 1,
                                 1.0,   // pressure
                                 0.0,   // angle
                                 evd->event_x, evd->event_y,
                                 evd->root_x, evd->root_y);
             _ecore_x_input_touch_index_clear(devid,  i);
          }
        break;
#endif /* ifdef ECORE_XI2_2 */
      default:
        break;
      }
#endif /* ifdef ECORE_XI2 */
}

#ifdef ECORE_XI2
/**
 * @internal
 * @brief Counts the number of set bits in an unsigned long.
 *
 * This is a utility function used, for example, to determine the number
 * of active valuators in an XIDeviceEvent.
 *
 * @param n The unsigned long integer whose bits are to be counted.
 * @return The number of bits set to 1 in @p n.
 */
static unsigned int
_ecore_x_count_bits(unsigned long n)
{
   unsigned int c; /* c accumulates the total bits set in v */
   for (c = 0; n; c++) n &= n - 1; /* clear the least significant bit set */
   return c;
}
#endif

#ifdef ECORE_XI2
/**
 * @internal
 * @brief Handles XInput2 axis events from valuators.
 *
 * This function processes XInput2 events that contain valuator data (axis information).
 * It extracts data for various axes like X, Y, pressure, tilt, etc., normalizes
 * them where appropriate, and packages them into an array of Ecore_Axis structures.
 * An Ecore_X_Event_Axis_Update event is then generated with this data.
 *
 * @param xevent Pointer to the XEvent structure (must be a GenericEvent with valuator data).
 * @param dev Pointer to the XIDeviceInfo for the device that generated the event.
 *
 * The Ecore_Axis array can contain:
 * - ECORE_AXIS_LABEL_X: Raw X coordinate.
 * - ECORE_AXIS_LABEL_NORMAL_X: Normalized X coordinate (0.0 to 1.0).
 * - ECORE_AXIS_LABEL_Y: Raw Y coordinate.
 * - ECORE_AXIS_LABEL_NORMAL_Y: Normalized Y coordinate (0.0 to 1.0).
 * - ECORE_AXIS_LABEL_PRESSURE: Normalized pressure (0.0 to 1.0).
 * - ECORE_AXIS_LABEL_DISTANCE: Normalized distance (0.0 to 1.0).
 * - ECORE_AXIS_LABEL_TWIST: Twist/rotation value, often in radians.
 * - ECORE_AXIS_LABEL_TILT: Tilt angle, calculated from tilt X and Y.
 * - ECORE_AXIS_LABEL_AZIMUTH: Azimuth angle, calculated from tilt X and Y.
 * - ECORE_AXIS_LABEL_WINDOW_X: X coordinate relative to the event window.
 * - ECORE_AXIS_LABEL_WINDOW_Y: Y coordinate relative to the event window.
 * - ECORE_AXIS_LABEL_UNKNOWN: For unrecognized axes.
 */
void
_ecore_x_input_axis_handler(XEvent *xevent, XIDeviceInfo *dev)
{
   if (xevent->type != GenericEvent) return;
   XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
   unsigned int n = _ecore_x_count_bits(*evd->valuators.mask) + 4;
   int i;
   int j = 0;
   double tiltx = 0, tilty = 0;
   Eina_Bool compute_tilt = EINA_FALSE;
   Ecore_Axis *axis = calloc(n, sizeof(Ecore_Axis));
   if (!axis) return;
   Ecore_Axis *axis_ptr = axis;
   Ecore_Axis *shrunk_axis;

   for (i = 0; i < dev->num_classes; i++)
     {
        if (dev->classes[i]->type == XIValuatorClass)
          {
             XIValuatorClassInfo *inf = ((XIValuatorClassInfo *)dev->classes[i]);

             if (*evd->valuators.mask & (1 << inf->number))
               {
                  if (inf->label == _ecore_x_input_get_axis_label("Abs X"))
                    {
                       int x = evd->valuators.values[j];
                       axis_ptr->label = ECORE_AXIS_LABEL_X;
                       axis_ptr->value = x;
                       axis_ptr++;
                       if (inf->max > inf->min)
                         {
                            axis_ptr->label = ECORE_AXIS_LABEL_NORMAL_X;
                            axis_ptr->value = (x - inf->min) / (inf->max - inf->min);
                            axis_ptr++;
                         }
                    }
                  else if (inf->label == _ecore_x_input_get_axis_label("Abs Y"))
                    {
                       int y = evd->valuators.values[j];
                       axis_ptr->label = ECORE_AXIS_LABEL_Y;
                       axis_ptr->value = y;
                       axis_ptr++;
                       if (inf->max > inf->min)
                         {
                            axis_ptr->label = ECORE_AXIS_LABEL_NORMAL_Y;
                            axis_ptr->value = (y - inf->min) / (inf->max - inf->min);
                            axis_ptr++;
                         }
                    }
                  else if (inf->label == _ecore_x_input_get_axis_label("Abs Pressure"))
                    {
                       axis_ptr->label = ECORE_AXIS_LABEL_PRESSURE;
                       axis_ptr->value = (evd->valuators.values[j] - inf->min) / (inf->max - inf->min);
                       axis_ptr++;
                    }
                  else if (inf->label == _ecore_x_input_get_axis_label("Abs Distance"))
                    {
                       axis_ptr->label = ECORE_AXIS_LABEL_DISTANCE;
                       axis_ptr->value = (evd->valuators.values[j] - inf->min) / (inf->max - inf->min);
                       axis_ptr++;
                    }
                  else if ((inf->label == _ecore_x_input_get_axis_label("Abs Rotary Z")) ||
                           (inf->label == _ecore_x_input_get_axis_label("Abs Wheel")))
                    {
                       axis_ptr->label = ECORE_AXIS_LABEL_TWIST;
                       if (inf->resolution == 1)
                         {
                            /* some wacom drivers do not correctly report resolution, so pre-normalize */
                            axis_ptr->value = 2*((evd->valuators.values[j] - inf->min) / (inf->max - inf->min)) - 1;
                            axis_ptr->value *= M_PI;
                         }
                       else
                         {
                            axis_ptr->value = evd->valuators.values[j] / inf->resolution;
                         }
                       axis_ptr++;
                    }
                  else if (inf->label == _ecore_x_input_get_axis_label("Abs Tilt X"))
                    {
                       tiltx = evd->valuators.values[j] / inf->resolution;
                       compute_tilt = EINA_TRUE;
                       /* don't increment axis_ptr */
                    }
                  else if (inf->label == _ecore_x_input_get_axis_label("Abs Tilt Y"))
                    {
                       tilty = -evd->valuators.values[j] / inf->resolution;
                       compute_tilt = EINA_TRUE;
                       /* don't increment axis_ptr */
                    }
                  else if ((inf->label == _ecore_x_input_get_axis_label("Rel X")) ||
                           (inf->label == _ecore_x_input_get_axis_label("Rel Y")) ||
                           (inf->label == _ecore_x_input_get_axis_label("Rel Vert Wheel")) ||
                           (inf->label == _ecore_x_input_get_axis_label("Rel Horiz Wheel")) ||
                           (inf->label == _ecore_x_input_get_axis_label("Rel Dial")))
                    {
                       /* Ignore those: mouse. Values are in fact not relative.
                        * No idea what is a "dial" event. */
                    }
                  else
                    {
                       axis_ptr->label = ECORE_AXIS_LABEL_UNKNOWN;
                       axis_ptr->value = evd->valuators.values[j];
                       axis_ptr++;
                    }
                  j++;
               }
          }
     }

   if ((compute_tilt) && ((axis_ptr + 2) <= (axis + n)))
     {
        double x = sin(tiltx);
        double y = sin(tilty);
        axis_ptr->label = ECORE_AXIS_LABEL_TILT;
        axis_ptr->value = asin(sqrt((x * x) + (y * y)));
        axis_ptr++;

        /* note: the value of atan2(0,0) is implementation-defined */
        axis_ptr->label = ECORE_AXIS_LABEL_AZIMUTH;
        axis_ptr->value = atan2(y, x);
        axis_ptr++;
     }

   /* update n to reflect actual count and realloc array to free excess */
   n = (axis_ptr - axis);
   if (n > 0)
     {
        /* event position in the window - most useful */
        axis_ptr->label = ECORE_AXIS_LABEL_WINDOW_X;
        axis_ptr->value = evd->event_x;
        axis_ptr++;
        axis_ptr->label = ECORE_AXIS_LABEL_WINDOW_Y;
        axis_ptr->value = evd->event_y;
        axis_ptr++;
        n += 2;

        shrunk_axis = realloc(axis, n * sizeof(Ecore_Axis));
        if (shrunk_axis != NULL) axis = shrunk_axis;
        _ecore_x_axis_update(evd->child ? evd->child : evd->event,
                             evd->event, evd->root, evd->time, evd->deviceid,
                             evd->detail, n, axis);
     }
   else
     free(axis);
}
#endif /* ifdef ECORE_XI2 */

#ifdef ECORE_XI2
/**
 * @internal
 * @brief Looks up XIDeviceInfo for a given device ID.
 *
 * Searches the cached list of XInput2 devices (`_ecore_x_xi2_devs`)
 * for a device matching the provided `deviceid`.
 *
 * @param deviceid The XInput device ID to look up.
 * @return A pointer to the XIDeviceInfo structure if found, NULL otherwise.
 */
static XIDeviceInfo *
_ecore_x_input_device_lookup(int deviceid)
{
   XIDeviceInfo *dev;
   int i;

   if (_ecore_x_xi2_devs)
     {
        for (i = 0; i < _ecore_x_xi2_num; i++)
          {
             dev = &(_ecore_x_xi2_devs[i]);
             if (deviceid == dev->deviceid) return dev;
          }
     }
   return NULL;
}
#endif

/**
 * @internal
 * @brief Callback function for the ecore_job that updates input devices.
 *
 * This function is scheduled as an Ecore_Job when an XI_DeviceChanged,
 * XI_HierarchyChanged, or XI_PropertyEvent occurs. It calls
 * ecore_x_input_devices_update() to refresh the list of input devices
 * and then emits an ECORE_X_DEVICES_CHANGE event.
 *
 * @param data User data passed to the job (unused in this case).
 */
static void
_cb_update_devices(void *data EINA_UNUSED)
{
   update_devices_job = NULL;
   ecore_x_input_devices_update();
   ecore_event_add(ECORE_X_DEVICES_CHANGE, NULL, NULL, NULL);
}

/**
 * @internal
 * @brief Main XInput2 event handler.
 *
 * This function is the primary dispatcher for XInput2 GenericEvents.
 * It determines the specific XInput2 event type (e.g., XI_DeviceChanged,
 * XI_Motion, XI_TouchBegin) and routes the event to the appropriate
 * specialized handler (_ecore_x_input_raw_handler, _ecore_x_input_multi_handler,
 * _ecore_x_input_mouse_handler, _ecore_x_input_axis_handler, or schedules
 * a device update).
 *
 * @param xevent Pointer to the XEvent structure.
 */
void
_ecore_x_input_handler(XEvent *xevent)
{
#ifdef ECORE_XI2
   if (xevent->type != GenericEvent) return;

   switch (xevent->xcookie.evtype)
     {
      case XI_DeviceChanged:
      case XI_HierarchyChanged:
      case XI_PropertyEvent:
        if (update_devices_job) ecore_job_del(update_devices_job);
        update_devices_job = ecore_job_add(_cb_update_devices, NULL);
        // XXX: post change event
        break;

      case XI_RawMotion:
      case XI_RawButtonPress:
      case XI_RawButtonRelease:
        _ecore_x_input_raw_handler(xevent);
        break;

      case XI_Motion:
      case XI_ButtonPress:
      case XI_ButtonRelease:
#ifdef ECORE_XI2_2
      case XI_TouchUpdate:
      case XI_TouchBegin:
      case XI_TouchEnd:
#endif
          {
             XIDeviceEvent *evd = (XIDeviceEvent *)(xevent->xcookie.data);
             XIDeviceInfo *dev = _ecore_x_input_device_lookup(evd->deviceid);

             if (!dev) return;

             if ((dev->use == XISlavePointer) &&
                 !(evd->flags & XIPointerEmulated))
               {
                  _ecore_x_input_multi_handler(xevent);
               }
             else if (dev->use == XIFloatingSlave)
               _ecore_x_input_mouse_handler(xevent);

             if (dev->use != XIMasterPointer)
               _ecore_x_input_axis_handler(xevent, dev);
          }
        break;
#ifdef XI_TouchCancel
      case XI_TouchCancel:
          {
             XITouchCancelEvent *evd = (XITouchCancelEvent *)(xevent->xcookie.data);
             int devid = evd->deviceid;

             if(!_ecore_x_input_touch_device_check(devid)) return;

             INF("Handling XI_TouchCancel device(%d)", devid);

             /* Currently X sends only one cancel event according to the touch device.
                But in the future, it maybe need several cancel events according to the touch.
                So it is better use button structure instead of creating new cancel structure.
              */
             _ecore_mouse_button(ECORE_EVENT_MOUSE_BUTTON_CANCEL,
                                 evd->time,
                                 0,   // state
                                 0,   // button
                                 0, 0,
                                 0, 0,
                                 evd->event,
                                (evd->child ? evd->child : evd->event),
                                 evd->root,
                                 1,   // same_screen
                                 0, 1, 1,
                                 0.0,   // pressure
                                 0.0,   // angle
                                 0, 0,
                                 0, 0);
          }
        break;
#endif
      default:
        break;
     }
#endif /* ifdef ECORE_XI2 */
}

/**
 * @brief Selects XInput2 events for multi-touch and mouse on a window.
 *
 * For all suitable slave pointer and floating slave devices, this function
 * selects for XI_ButtonPress, XI_ButtonRelease, and XI_Motion events on the
 * specified window. If ECORE_XI2_2 is defined, it also selects for
 * XI_TouchUpdate, XI_TouchBegin, XI_TouchEnd, and XI_TouchCancel events
 * for touch-capable devices and updates the internal touch device info list.
 *
 * @param win The Ecore_X_Window on which to select events.
 * @return EINA_TRUE if events were successfully selected for at least one device,
 *         EINA_FALSE otherwise (e.g., XInput2 not available or no suitable devices).
 */
EAPI Eina_Bool
ecore_x_input_multi_select(Ecore_X_Window win)
{
#ifdef ECORE_XI2
   int i;
   Eina_Bool find = EINA_FALSE;

   if (!_ecore_x_xi2_devs)
     return EINA_FALSE;

   LOGFN;
   for (i = 0; i < _ecore_x_xi2_num; i++)
     {
        XIDeviceInfo *dev = &(_ecore_x_xi2_devs[i]);
        XIEventMask eventmask;
        unsigned char mask[4] = { 0 };
        int update = 0;

        eventmask.deviceid = dev->deviceid;
        eventmask.mask_len = sizeof(mask);
        eventmask.mask = mask;

        if ((dev->use == XIFloatingSlave) || (dev->use == XISlavePointer))
          {
             XISetMask(mask, XI_ButtonPress);
             XISetMask(mask, XI_ButtonRelease);
             XISetMask(mask, XI_Motion);

#ifdef ECORE_XI2_2
             Eina_Inlist *l = _ecore_x_xi2_touch_info_list;
             Ecore_X_Touch_Device_Info *info;
             info = _ecore_x_input_touch_info_get(dev);

             if (info)
               {
                  XISetMask(mask, XI_TouchUpdate);
                  XISetMask(mask, XI_TouchBegin);
                  XISetMask(mask, XI_TouchEnd);
#ifdef XI_TouchCancel
                  XISetMask(mask, XI_TouchCancel);
#endif
                  update = 1;

                  l = eina_inlist_append(l, (Eina_Inlist *)info);
                  _ecore_x_xi2_touch_info_list = l;
               }
#endif /* #ifdef ECORE_XI2_2 */
             update = 1;
          }

        if (update)
          {
             XISelectEvents(_ecore_x_disp, win, &eventmask, 1);
             if (_ecore_xlib_sync) ecore_x_sync();
             find = EINA_TRUE;
          }
     }

   return find;
#else /* ifdef ECORE_XI2 */
   return EINA_FALSE;
#endif /* ifdef ECORE_XI2 */
}

/**
 * @brief Selects XInput2 raw events on a window.
 *
 * This function selects for raw input events (XI_RawButtonPress,
 * XI_RawButtonRelease, XI_RawMotion) from all master devices on the
 * specified window. Raw events provide device data unfiltered by
 * window system transformations.
 *
 * @param win The Ecore_X_Window on which to select raw events.
 * @return EINA_TRUE if events were successfully selected, EINA_FALSE otherwise
 *         (e.g., XInput2 not available).
 */
EAPI Eina_Bool
ecore_x_input_raw_select(Ecore_X_Window win)
{
#ifdef ECORE_XI2
   XIEventMask emask;
   unsigned char mask[4] = { 0 };

   if (!_ecore_x_xi2_devs)
     return EINA_FALSE;

   LOGFN;
   emask.deviceid = XIAllMasterDevices;
   emask.mask_len = sizeof(mask);
   emask.mask = mask;
#ifdef XI_RawButtonPress
   XISetMask(emask.mask, XI_RawButtonPress);
#endif
#ifdef XI_RawButtonRelease
   XISetMask(emask.mask, XI_RawButtonRelease);
#endif
#ifdef XI_RawMotion
   XISetMask(emask.mask, XI_RawMotion);
#endif

   XISelectEvents(_ecore_x_disp, win, &emask, 1);
   if (_ecore_xlib_sync) ecore_x_sync();

   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif
}

/**
 * @internal
 * @brief Grabs or ungrabs touch events for all touch-capable slave pointer devices.
 *
 * This function iterates through all known XInput2 devices. For each device
 * identified as a touch-capable slave pointer, it either grabs touch events
 * (XI_TouchUpdate, XI_TouchBegin, XI_TouchEnd, XI_TouchCancel) for the
 * specified `grab_win` or ungrabs them if `grab` is EINA_FALSE.
 *
 * @param grab_win The Ecore_X_Window to grab events for. Pass 0 when ungrabbing.
 * @param grab EINA_TRUE to grab, EINA_FALSE to ungrab.
 * @return EINA_TRUE if the grab/ungrab operation was successful for at least
 *         one device, EINA_FALSE otherwise or if XInput2 is not available.
 */
EAPI Eina_Bool
_ecore_x_input_touch_devices_grab(Ecore_X_Window grab_win, Eina_Bool grab)
{
#ifdef ECORE_XI2
   int i;

   if (!_ecore_x_xi2_devs)
     return EINA_FALSE;

   Eina_Bool status = EINA_FALSE;

   LOGFN;
   for (i = 0; i < _ecore_x_xi2_num; i++)
     {
        XIDeviceInfo *dev = &(_ecore_x_xi2_devs[i]);
        int update = 0;
        XIEventMask eventmask;
        unsigned char mask[4] = { 0 };

        eventmask.deviceid = XISlavePointer;
        eventmask.mask_len = sizeof(mask);
        eventmask.mask = mask;

        if (dev->use == XISlavePointer)
          {
#ifdef ECORE_XI2_2
             Ecore_X_Touch_Device_Info *info;
             info = _ecore_x_input_touch_info_get(dev);

             if (info)
               {
                  XISetMask(mask, XI_TouchUpdate);
                  XISetMask(mask, XI_TouchBegin);
                  XISetMask(mask, XI_TouchEnd);
#ifdef XI_TouchCancel
                  XISetMask(mask, XI_TouchCancel);
#endif
                  update = 1;
                  free(info);
               }
#endif /* #ifdef ECORE_XI2_2 */
          }

        if (update)
          {
             if (grab) {
                status |= (XIGrabDevice(_ecore_x_disp, dev->deviceid, grab_win, CurrentTime,
                           None, GrabModeAsync, GrabModeAsync, False, &eventmask) == GrabSuccess);
                _ecore_x_xi2_grabbed_devices_list = eina_list_append(_ecore_x_xi2_grabbed_devices_list, (void*)(intptr_t)dev->deviceid);
             }
             else {
                status |= (XIUngrabDevice(_ecore_x_disp, dev->deviceid, CurrentTime) == Success);
                _ecore_x_xi2_grabbed_devices_list = eina_list_remove(_ecore_x_xi2_grabbed_devices_list, (void*)(intptr_t)dev->deviceid);
             }
             if (_ecore_xlib_sync) ecore_x_sync();
          }
     }

   return status;
#endif
   return EINA_FALSE;
}

/**
 * @brief Grabs touch events for all touch-capable devices on a given window.
 *
 * This is a convenience function that calls _ecore_x_input_touch_devices_grab
 * to grab touch events (XI_TouchUpdate, XI_TouchBegin, XI_TouchEnd, XI_TouchCancel)
 * for all touch-capable slave pointer devices on the specified `grab_win`.
 *
 * @param grab_win The Ecore_X_Window to grab touch events for.
 * @return EINA_TRUE if the grab operation was successful for at least one device,
 *         EINA_FALSE otherwise.
 * @see _ecore_x_input_touch_devices_grab
 */
EAPI Eina_Bool
ecore_x_input_touch_devices_grab(Ecore_X_Window grab_win)
{
   return _ecore_x_input_touch_devices_grab(grab_win, EINA_TRUE);
}

/**
 * @brief Ungrabs touch events for all previously grabbed touch-capable devices.
 *
 * This is a convenience function that calls _ecore_x_input_touch_devices_grab
 * to ungrab touch events from all devices that might have been previously
 * grabbed by ecore_x_input_touch_devices_grab().
 *
 * @return EINA_TRUE if the ungrab operation was successful for at least one device,
 *         EINA_FALSE otherwise.
 * @see _ecore_x_input_touch_devices_grab
 */
EAPI Eina_Bool
ecore_x_input_touch_devices_ungrab(void)
{
   return _ecore_x_input_touch_devices_grab(0, EINA_FALSE);
}

// XXX
/**
 * @brief Updates the cached list of XInput2 devices.
 *
 * Frees the current list of XInput2 devices and queries the X server
 * for an updated list. This should be called when a device change
 * notification is received (e.g., XI_DeviceChanged).
 * The global variable `_ecore_x_xi2_devs` will point to the new list,
 * and `_ecore_x_xi2_num` will be updated with the new count.
 */
EAPI void
ecore_x_input_devices_update(void)
{
   if (_ecore_x_xi2_devs) XIFreeDeviceInfo(_ecore_x_xi2_devs);
   _ecore_x_xi2_num = 0;
   _ecore_x_xi2_devs = XIQueryDevice(_ecore_x_disp, XIAllDevices,
                                     &_ecore_x_xi2_num);
}

/**
 * @brief Gets the number of available XInput2 devices.
 *
 * @return The total number of XInput2 devices currently known.
 *         This value is updated by ecore_x_input_devices_update().
 */
EAPI int
ecore_x_input_device_num_get(void)
{
   return _ecore_x_xi2_num;
}

/**
 * @brief Gets the device ID of an XInput2 device by its slot number.
 *
 * The slot number is an index into the internal array of XInput2 devices.
 *
 * @param slot The slot number (index) of the device. Must be between 0
 *             and ecore_x_input_device_num_get() - 1.
 * @return The XInput device ID, or 0 if the slot number is invalid.
 */
EAPI int
ecore_x_input_device_id_get(int slot)
{
   if ((slot < 0) || (slot >= _ecore_x_xi2_num)) return 0;
   return _ecore_x_xi2_devs[slot].deviceid;
}

/**
 * @brief Gets the name of an XInput2 device by its slot number.
 *
 * The slot number is an index into the internal array of XInput2 devices.
 *
 * @param slot The slot number (index) of the device. Must be between 0
 *             and ecore_x_input_device_num_get() - 1.
 * @return A pointer to the device name string, or NULL if the slot number
 *         is invalid. The returned string is owned by Ecore and should not
 *         be modified or freed.
 */
EAPI const char *
ecore_x_input_device_name_get(int slot)
{
   if ((slot < 0) || (slot >= _ecore_x_xi2_num)) return NULL;
   return _ecore_x_xi2_devs[slot].name;
}

/**
 * @brief Lists the names of properties available for an XInput2 device.
 *
 * Retrieves all properties associated with the XInput2 device at the given slot.
 *
 * @param slot The slot number (index) of the device.
 * @param[out] num_ret Pointer to an integer where the number of properties will be stored.
 * @return A newly allocated array of strings, where each string is a property name.
 *         The caller is responsible for freeing this array and its contents using
 *         ecore_x_input_device_properties_free().
 *         Returns NULL on error (e.g., invalid slot, no properties, memory allocation failure),
 *         and `*num_ret` will be set to 0.
 *
 * Example usage:
 * @code
 * int num_props;
 * char **props = ecore_x_input_device_properties_list(device_slot, &num_props);
 * if (props) {
 *     for (int i = 0; i < num_props; i++) {
 *         printf("Property: %s\n", props[i]);
 *     }
 *     ecore_x_input_device_properties_free(props, num_props);
 * }
 * @endcode
 */
EAPI char **
ecore_x_input_device_properties_list(int slot, int *num_ret)
{
   char **atoms = NULL;
   int num = 0, i;
   Atom *a;

   if ((slot < 0) || (slot >= _ecore_x_xi2_num)) goto err;
   a = XIListProperties(_ecore_x_disp, _ecore_x_xi2_devs[slot].deviceid, &num);
   if (!a) goto err;
   atoms = calloc(num, sizeof(char *));
   if (!atoms) goto err;
   for (i = 0; i < num; i++)
     {
        Ecore_X_Atom at = a[i];
        atoms[i] = ecore_x_atom_name_get(at);
        if (!atoms[i]) goto err;
     }
   XFree(a);
   *num_ret = num;
   return atoms;
err:
   if (atoms)
     {
        for (i = 0; i < num; i++) free(atoms[i]);
        free(atoms);
     }
   *num_ret = 0;
   return NULL;
}

/**
 * @brief Frees the list of device property names.
 *
 * Frees the memory allocated by ecore_x_input_device_properties_list().
 *
 * @param list The array of property name strings to free.
 * @param num The number of strings in the list (as returned by
 *            ecore_x_input_device_properties_list() in `num_ret`).
 */
EAPI void
ecore_x_input_device_properties_free(char **list, int num)
{
   int i;

   for (i = 0; i < num; i++) free(list[i]);
   free(list);
}

// unit_size_ret will be 8, 16, 32
// fromat_ret will almost be one of:
// ECORE_X_ATOM_CARDINAL
// ECORE_X_ATOM_INTEGER
// ECORE_X_ATOM_FLOAT (unit_size 32 only)
// ECORE_X_ATOM_ATOM // very rare
// ECORE_X_ATOM_STRING (unit_size 8 only - guaratee nul termination)

/**
 * @brief Gets the value of a specific property for an XInput2 device.
 *
 * Retrieves the data, type, format, and number of items for a named property
 * of the XInput2 device at the given slot.
 *
 * @param slot The slot number (index) of the device.
 * @param prop The name of the property to retrieve.
 * @param[out] num_ret Pointer to an integer where the number of items in the
 *                     property data will be stored.
 * @param[out] format_ret Pointer to an Ecore_X_Atom where the actual type (atom)
 *                        of the property will be stored. Common values include:
 *                        - ECORE_X_ATOM_CARDINAL (unsigned integers)
 *                        - ECORE_X_ATOM_INTEGER (signed integers)
 *                        - ECORE_X_ATOM_FLOAT (floating point numbers, unit_size_ret will be 32)
 *                        - ECORE_X_ATOM_ATOM (Atom values)
 *                        - ECORE_X_ATOM_STRING (string data, unit_size_ret will be 8)
 * @param[out] unit_size_ret Pointer to an integer where the size (in bits) of each
 *                           item in the property data will be stored (e.g., 8, 16, 32).
 * @return A pointer to the property data. The caller is responsible for freeing
 *         this memory using free(). If the property type is ECORE_X_ATOM_STRING
 *         and format is 8-bit, the returned string is guaranteed to be NUL-terminated.
 *         Returns NULL on error (e.g., invalid slot, property not found, memory
 *         allocation failure), and `*num_ret`, `*format_ret`, `*unit_size_ret` will be set to 0.
 *
 * Example for reading an integer property:
 * @code
 * int num_items, unit_size;
 * Ecore_X_Atom format;
 * int *values = ecore_x_input_device_property_get(slot, "My Integer Property", &num_items, &format, &unit_size);
 * if (values && format == ECORE_X_ATOM_INTEGER && unit_size == 32) {
 *     for (int i = 0; i < num_items; i++) {
 *         printf("Value %d: %d\n", i, values[i]);
 *     }
 *     free(values);
 * }
 * @endcode
 */
EAPI void *
ecore_x_input_device_property_get(int slot, const char *prop, int *num_ret,
                                  Ecore_X_Atom *format_ret, int *unit_size_ret)
{
   Atom a, a_type = 0;
   int fmt = 0;
   unsigned long num = 0, dummy;
   unsigned char *data = NULL;
   unsigned char *d = NULL;

   if ((slot < 0) || (slot >= _ecore_x_xi2_num)) goto err;
   a = XInternAtom(_ecore_x_disp, prop, False);
   // XIGetProperty returns 0 AKA `Success` if everything is good
   if (XIGetProperty(_ecore_x_disp, _ecore_x_xi2_devs[slot].deviceid,
                      a, 0, 65536, False, AnyPropertyType, &a_type, &fmt,
                      &num, &dummy, &data) != 0) goto err;
   *format_ret = a_type;
   *num_ret = num;
   *unit_size_ret = fmt;
   if ((a_type == ECORE_X_ATOM_STRING) && (fmt == 8))
     {
        d = malloc(num + 1);
        if (!d) goto err2;
        memcpy(d, data, num);
        d[num] = 0;
     }
   else
     {
        if      (fmt == 8 ) d = malloc(num);
        else if (fmt == 16) d = malloc(num * 2);
        else if (fmt == 32) d = malloc(num * 4);
        if (!d) goto err2;
        memcpy(d, data, num * (fmt / 8));
     }
   XFree(data);
   return d;
err2:
   XFree(data);
err:
   *num_ret = 0;
   *format_ret = 0;
   *unit_size_ret = 0;
   return NULL;
}

/**
 * @brief Sets the value of a specific property for an XInput2 device.
 *
 * Changes or creates a property for the XInput2 device at the given slot.
 *
 * @param slot The slot number (index) of the device.
 * @param prop The name of the property to set.
 * @param data Pointer to the data for the property.
 * @param num The number of items in the `data` array.
 * @param format The Ecore_X_Atom representing the type of the property
 *               (e.g., ECORE_X_ATOM_INTEGER, ECORE_X_ATOM_STRING).
 * @param unit_size The size (in bits) of each item in the `data`
 *                  (e.g., 8, 16, 32).
 *
 * Example for setting an integer property:
 * @code
 * int my_value = 123;
 * ecore_x_input_device_property_set(slot, "My Integer Property", &my_value, 1, ECORE_X_ATOM_INTEGER, 32);
 * @endcode
 */
EAPI void
ecore_x_input_device_property_set(int slot, const char *prop, void *data,
                                  int num, Ecore_X_Atom format, int unit_size)
{
   Atom a, a_type = 0;
   if ((slot < 0) || (slot >= _ecore_x_xi2_num)) return;
   a = XInternAtom(_ecore_x_disp, prop, False);
   a_type = format;
   XIChangeProperty(_ecore_x_disp, _ecore_x_xi2_devs[slot].deviceid,
                    a, a_type, unit_size, XIPropModeReplace, data, num);
}

// XXX: add api's to get XIDeviceInfo->... stuff like
// use, attachement, enabled, num_classes, classes (which list number of
// buttons and their names, keycodes, valuators (mouse etc.), touch devices
// etc.
