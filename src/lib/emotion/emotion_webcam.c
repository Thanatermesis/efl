#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_EEZE
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# ifdef HAVE_V4L2
#  include <sys/ioctl.h>
#  include <linux/videodev2.h>
# endif
# include <Eeze.h>
#endif

#include <Ecore.h>

#include "emotion_private.h"

/**
 * @brief Event type for webcam list updates.
 * This event is triggered when a webcam is added or removed.
 */
EMOTION_API int EMOTION_WEBCAM_UPDATE = 0;
/**
 * @brief Event type for webcam addition.
 * This event is triggered when a new webcam is detected and added.
 * The event_info for this event will be an Emotion_Webcam pointer.
 */
EMOTION_API int EMOTION_WEBCAM_ADD = 0;
/**
 * @brief Event type for webcam deletion.
 * This event is triggered when a webcam is removed.
 * The event_info for this event will be an Emotion_Webcam pointer.
 */
EMOTION_API int EMOTION_WEBCAM_DEL = 0;

typedef struct _Emotion_Webcams Emotion_Webcams;

/**
 * @internal
 * @brief Structure to manage the list of webcams and related data.
 */
struct _Emotion_Webcams
{
   Eina_List *webcams;    /**< List of detected Emotion_Webcam objects */
   Ecore_Idler *idler;    /**< Idler for processing webcam checks */
   Eina_List *check_list; /**< List of syspaths to check for webcams */
   Eina_Bool init : 1;    /**< Flag indicating if webcam detection has been initialized */
};

/**
 * @brief Represents a webcam device.
 *
 * This structure holds information about a detected webcam,
 * including its system path, device node, name, and filename.
 */
struct _Emotion_Webcam
{
   EINA_REFCOUNT; /**< Reference count for the webcam object */

   const char *syspath;  /**< System path of the webcam (e.g., /sys/devices/pci0000:00/0000:00:14.0/usb1/1-7/1-7:1.0/video4linux/video0) */
   const char *device;   /**< Device identifier string (e.g., v4l2:///dev/video0) */
   const char *name;     /**< Human-readable name of the webcam (e.g., "Integrated Camera") */
   const char *filename; /**< Device file path (e.g., /dev/video0, derived from device) */
   Eina_Bool in_list : 1;/**< Flag indicating if the webcam is currently in the global list */
};

static Emotion_Webcams *_emotion_webcams = NULL;

/**
 * @internal
 * @brief Destroys an Emotion_Webcam object.
 * Frees the memory associated with the webcam, including stringshared fields.
 * @param ew The Emotion_Webcam object to destroy.
 */
static void
emotion_webcam_destroy(Emotion_Webcam *ew)
{
   eina_stringshare_del(ew->syspath);
   eina_stringshare_del(ew->device);
   eina_stringshare_del(ew->name);
   free(ew);
}

#ifdef HAVE_EEZE
static Eeze_Udev_Watch *eeze_watcher = NULL;

/**
 * @internal
 * @brief Checks if a given device path corresponds to a valid webcam.
 * This function attempts to open the device, query its capabilities (if V4L2 is available),
 * and ensures it's a video capture device and not already in the list.
 * If the device is valid, it's added to the `_emotion_webcams->webcams` list.
 * Otherwise, the Emotion_Webcam object is destroyed.
 * @param ew The Emotion_Webcam object to check.
 * @return EINA_TRUE if the device is a valid webcam and added, EINA_FALSE otherwise.
 */
static Eina_Bool
_emotion_check_device(Emotion_Webcam *ew)
{
#ifdef HAVE_V4L2
   Emotion_Webcam *check;
   Eina_List *l;
   struct v4l2_capability caps;
   int fd = -1;
#endif

   if (!ew) return EINA_FALSE;
#ifdef HAVE_V4L2
   if (!ew->device) goto on_error;

   fd = open(ew->filename, O_RDONLY);
   if (fd < 0) goto on_error;

   if (ioctl(fd, VIDIOC_QUERYCAP, &caps) == -1) goto on_error;

   // Likely not a webcam
   if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)) goto on_error;
   if (caps.capabilities &
       (V4L2_CAP_TUNER | V4L2_CAP_RADIO | V4L2_CAP_MODULATOR))
     goto on_error;

   EINA_LIST_FOREACH(_emotion_webcams->webcams, l, check)
     {
        if (check->device == ew->device) goto on_error;
     }
   _emotion_webcams->webcams = eina_list_append(_emotion_webcams->webcams, ew);
   ew->in_list = EINA_TRUE;
   if (fd >= 0) close(fd);
   return EINA_TRUE;

 on_error:
#endif
   INF("'%s' is not a webcam ['%s']", ew->name, strerror(errno));
   emotion_webcam_destroy(ew);
#ifdef HAVE_V4L2
   if (fd >= 0) close(fd);
#endif
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Creates a new Emotion_Webcam object from a syspath.
 * Initializes the webcam structure by querying udev properties for the name and device node.
 * @param syspath The system path of the udev device.
 * @return A new Emotion_Webcam object, or NULL on failure.
 */
static Emotion_Webcam *
_emotion_webcam_new(const char *syspath)
{
   Emotion_Webcam *ew;
   const char *device;
   char *local;

   ew = calloc(1, sizeof(Emotion_Webcam));
   if (!ew) return NULL;

   EINA_REFCOUNT_INIT(ew);
   ew->syspath = eina_stringshare_ref(syspath);
   ew->name = eeze_udev_syspath_get_sysattr(syspath, "name");

   device = eeze_udev_syspath_get_property(syspath, "DEVNAME");
   local = alloca(eina_stringshare_strlen(device) + 8);
   snprintf(local, eina_stringshare_strlen(device) + 8, "v4l2://%s", device);
   ew->device = eina_stringshare_add(local);
   eina_stringshare_del(device);
   ew->filename = ew->device + 7;

   return ew;
}

/**
 * @internal
 * @brief Decrements the reference count of an Emotion_Webcam object.
 * If the reference count reaches zero, it removes the webcam from the global list
 * (if it's present) and then destroys the webcam object.
 * @param ew The Emotion_Webcam object to unreference.
 */
static void
_emotion_webcam_unref(Emotion_Webcam *ew)
{
   EINA_REFCOUNT_UNREF(ew)
     {
        if ((ew->in_list) && (_emotion_webcams))
          {
             _emotion_webcams->webcams =
               eina_list_remove(_emotion_webcams->webcams, ew);
             ew->in_list = EINA_FALSE;
          }
        emotion_webcam_destroy(ew);
     }
}

/**
 * @internal
 * @brief Callback function to free an Emotion_Webcam object when an ecore event is processed.
 * This is typically used as the `free_func` for ecore_event_add.
 * It calls _emotion_webcam_unref on the event data.
 * @param data Unused user data.
 * @param ev The event data, expected to be an Emotion_Webcam pointer.
 */
static void
_emotion_eeze_event_free(void *data EINA_UNUSED, void *ev)
{
   _emotion_webcam_unref(ev);
}

/**
 * @internal
 * @brief Handles the addition of a new webcam device.
 * Creates a new Emotion_Webcam object, checks if it's a valid webcam,
 * and if so, adds an EMOTION_WEBCAM_ADD event.
 * @param syspath The system path of the newly added udev device.
 */
static void
_emotion_webcam_ev_add(const char *syspath)
{
   Emotion_Webcam *ew = _emotion_webcam_new(syspath);
   if (!ew) return;
   if (!_emotion_check_device(ew)) return;
   EINA_REFCOUNT_REF(ew);
   ecore_event_add(EMOTION_WEBCAM_ADD, ew, _emotion_eeze_event_free, NULL);
}

/**
 * @internal
 * @brief Idler callback to process webcams from the check_list.
 * This function is called repeatedly by an ecore idler to process devices
 * found during initial enumeration. It takes one syspath from `webcams->check_list`
 * at a time, attempts to add it as a webcam, and removes it from the list.
 * When the list is empty, the idler is removed.
 * @param data Pointer to the Emotion_Webcams structure.
 * @return ECORE_CALLBACK_RENEW (EINA_TRUE) if there are more webcams to process,
 *         ECORE_CALLBACK_CANCEL (EINA_FALSE) otherwise.
 */
static Eina_Bool
_emotion_process_webcam(void *data)
{
   Emotion_Webcams *webcams = data;
   const char *syspath;

   syspath = eina_list_data_get(webcams->check_list);
   if (!syspath)
     {
        webcams->idler = NULL;
        webcams->init = EINA_TRUE;
        return EINA_FALSE;
     }
   webcams->check_list = eina_list_remove_list(webcams->check_list,
                                               webcams->check_list);
   _emotion_webcam_ev_add(syspath);
   eina_stringshare_del(syspath);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function to handle webcam removal event.
 * This is used as the `free_func` for the EMOTION_WEBCAM_DEL ecore event.
 * It calls _emotion_webcam_unref on the event data.
 * @param data Unused user data.
 * @param ev The event data, expected to be an Emotion_Webcam pointer.
 */
static void
_emotion_webcam_remove_cb(void *data EINA_UNUSED, void *ev)
{
   _emotion_webcam_unref(ev);
}

/**
 * @internal
 * @brief Callback for Eeze udev events (webcam add/remove).
 * This function is triggered by eeze when a V4L device is added or removed.
 * If a device is removed, it finds the corresponding Emotion_Webcam in the list,
 * removes it, and triggers an EMOTION_WEBCAM_DEL event.
 * If a device is added, it calls _emotion_webcam_ev_add.
 * In both cases, an EMOTION_WEBCAM_UPDATE event is triggered.
 * @param syspath The system path of the udev device that changed.
 * @param ev The type of udev event (EEZE_UDEV_EVENT_ADD or EEZE_UDEV_EVENT_REMOVE).
 * @param data Unused user data.
 * @param watcher Unused Eeze_Udev_Watch pointer.
 */
static void
_emotion_eeze_events(const char *syspath, Eeze_Udev_Event ev,
                     void *data EINA_UNUSED,
                     Eeze_Udev_Watch *watcher EINA_UNUSED)
{
   if (ev == EEZE_UDEV_EVENT_REMOVE)
     {
        Emotion_Webcam *ew;
        Eina_List *l;

        EINA_LIST_FOREACH(_emotion_webcams->webcams, l, ew)
          {
             if (ew->syspath == syspath)
               {
                  if (ew->in_list)
                    {
                       _emotion_webcams->webcams =
                         eina_list_remove_list(_emotion_webcams->webcams, l);
                       ew->in_list = EINA_FALSE;
                    }
                  ecore_event_add(EMOTION_WEBCAM_DEL, ew,
                                  _emotion_webcam_remove_cb, NULL);
                  break;
               }
          }
     }
   else if (ev == EEZE_UDEV_EVENT_ADD)
     {
        _emotion_webcam_ev_add(syspath);
     }
   ecore_event_add(EMOTION_WEBCAM_UPDATE, NULL, NULL, NULL);
}

#endif

/**
 * @internal
 * @brief Enumerates all V4L webcams using Eeze.
 * This function is called to perform an initial scan for webcams.
 * It finds all devices of type EEZE_UDEV_TYPE_V4L, adds their syspaths
 * to `_emotion_webcams->check_list`, and starts an ecore idler
 * (`_emotion_process_webcam`) to process them.
 * This function does nothing if `_emotion_webcams->init` is true.
 */
static void
_emotion_enumerate_all_webcams(void)
{
#ifdef HAVE_EEZE
   Eina_List *devices;
   if (_emotion_webcams->init) return;
   devices = eeze_udev_find_by_type(EEZE_UDEV_TYPE_V4L, NULL);
   _emotion_webcams->check_list = devices;
   _emotion_webcams->idler = ecore_idler_add(_emotion_process_webcam,
                                             _emotion_webcams);
#endif
}

/**
 * @brief Initializes the webcam detection system.
 * This function sets up the necessary ecore event types for webcam
 * notifications (add, delete, update). It allocates the global
 * `_emotion_webcams` structure if it doesn't exist.
 * If HAVE_EEZE is defined, it initializes Eeze and sets up a udev watch
 * for V4L devices.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., memory allocation failed).
 */
Eina_Bool emotion_webcam_init(void)
{
   EMOTION_WEBCAM_UPDATE = ecore_event_type_new();
   EMOTION_WEBCAM_ADD = ecore_event_type_new();
   EMOTION_WEBCAM_DEL = ecore_event_type_new();

   if (!_emotion_webcams)
     {
        _emotion_webcams = calloc(1, sizeof (Emotion_Webcams));
        EINA_SAFETY_ON_NULL_RETURN_VAL(_emotion_webcams, EINA_FALSE);
     }

#ifdef HAVE_EEZE
   eeze_init();
   eeze_watcher = eeze_udev_watch_add
     (EEZE_UDEV_TYPE_V4L, (EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE),
     _emotion_eeze_events, NULL);
#endif

   return EINA_TRUE;
}

/**
 * @brief Shuts down the webcam detection system.
 * Flushes any pending webcam events. Stops the idler if it's running.
 * Frees all Emotion_Webcam objects in the list and the list itself.
 * Frees the `_emotion_webcams` global structure.
 * If HAVE_EEZE is defined, it removes the udev watch and shuts down Eeze.
 */
void
emotion_webcam_shutdown(void)
{
   Emotion_Webcam *ew;
   const char *syspath;

   ecore_event_type_flush(EMOTION_WEBCAM_UPDATE, EMOTION_WEBCAM_ADD,
                          EMOTION_WEBCAM_DEL);

   if (_emotion_webcams->idler)
     {
        ecore_idler_del(_emotion_webcams->idler);
        _emotion_webcams->idler = NULL;
     }

   EINA_LIST_FREE(_emotion_webcams->check_list, syspath)
     {
        eina_stringshare_del(syspath);
     }

   _emotion_webcams->init = EINA_FALSE;

   EINA_LIST_FREE(_emotion_webcams->webcams, ew)
     {
        ew->in_list = EINA_FALSE;
        // There is currently no way to refcount from the outside, this helps
        // but could lead to some issues
        EINA_REFCOUNT_UNREF(ew)
          {
             emotion_webcam_destroy(ew);
          }
     }
   free(_emotion_webcams);
   _emotion_webcams = NULL;

#ifdef HAVE_EEZE
   eeze_udev_watch_del(eeze_watcher);
   eeze_watcher = NULL;
   eeze_shutdown();
#endif
}

/**
 * @brief Gets the list of currently detected webcams.
 * If webcam enumeration hasn't been performed yet (first call),
 * it triggers `_emotion_enumerate_all_webcams`.
 * @return A const Eina_List of Emotion_Webcam objects.
 *         The list contains pointers to Emotion_Webcam structures.
 *         Example of iterating the list:
 *         @code
 *         const Eina_List *webcams, *l;
 *         Emotion_Webcam *cam;
 *         webcams = emotion_webcams_get();
 *         EINA_LIST_FOREACH(webcams, l, cam)
 *           {
 *              const char *name = emotion_webcam_name_get(cam);
 *              const char *device = emotion_webcam_device_get(cam);
 *              printf("Webcam: %s, Device: %s\n", name, device);
 *           }
 *         @endcode
 *         Do not modify the returned list or its contents.
 *         Returns NULL if the webcam system is not initialized.
 */
EMOTION_API const Eina_List *
emotion_webcams_get(void)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_emotion_webcams, NULL);
   _emotion_enumerate_all_webcams();
   return _emotion_webcams->webcams;
}

/**
 * @brief Gets the human-readable name of the webcam.
 * @param ew Pointer to the Emotion_Webcam object.
 * @return The name of the webcam (e.g., "Integrated Camera").
 *         The returned string is an Eina_Stringshare, do not free it.
 *         Returns NULL if ew is NULL.
 */
EMOTION_API const char *
emotion_webcam_name_get(const Emotion_Webcam *ew)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ew, NULL);
   return ew->name;
}

/**
 * @brief Gets the device identifier string for the webcam.
 * This is typically in the format "v4l2:///dev/videoX".
 * @param ew Pointer to the Emotion_Webcam object.
 * @return The device identifier string.
 *         The returned string is an Eina_Stringshare, do not free it.
 *         Returns NULL if ew is NULL.
 */
EMOTION_API const char *
emotion_webcam_device_get(const Emotion_Webcam *ew)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ew, NULL);
   return ew->device;
}

/**
 * @brief Gets custom data associated with a webcam device.
 * @note This function currently always returns NULL as custom data is not implemented.
 * @param device The device identifier (unused).
 * @return Currently always NULL.
 */
EMOTION_API const char *
emotion_webcam_custom_get(const char *device EINA_UNUSED)
{
   return NULL;
}
