#include <elput_private.h>

/**
 * @brief Macro to generate accessor functions for Elput_Swipe_Gesture fields.
 *
 * This macro creates a function of the form elput_STRUCT_NAME_FIELD_NAME_get
 * that retrieves the value of FIELD_NAME from an Elput_Swipe_Gesture object.
 *
 * @param STRUCT_NAME The name of the structure (e.g., swipe).
 * @param FIELD_NAME The name of the field (e.g., dx, dy, finger_count).
 * @param TYPE The data type of the field.
 * @param FALLBACK The value to return if the gesture object is NULL.
 */
#define ACCESSOR(STRUCT_NAME, FIELD_NAME, TYPE, FALLBACK) \
EAPI TYPE \
elput_##STRUCT_NAME##_##FIELD_NAME##_get(Elput_Swipe_Gesture *gesture) \
{ \
   EINA_SAFETY_ON_NULL_RETURN_VAL(gesture, FALLBACK); \
   return gesture->FIELD_NAME; \
}

ACCESSOR(swipe, dy, double, 0.0f)
ACCESSOR(swipe, dx, double, 0.0f)
ACCESSOR(swipe, finger_count, int, 0)
ACCESSOR(swipe, window, int, 0)
ACCESSOR(swipe, device, Elput_Device*, NULL)

/**
 * @brief Connects to the Elput manager and configures it for gesture events only.
 *
 * This function establishes a connection to the Elput manager for the specified
 * seat and TTY. It then configures the manager to only process gesture events,
 * ignoring other input events.
 *
 * @param seat The seat identifier (e.g., "seat0").
 * @param tty The TTY number.
 * @return A pointer to the Elput_Manager instance on success, or NULL on failure.
 */
EAPI Elput_Manager*
elput_manager_connect_gestures(const char *seat, unsigned int tty)
{
   Elput_Manager *em;

   em = elput_manager_connect(seat, tty);
   if (em)
     em->only_gesture_events = EINA_TRUE;

   return em;
}

/**
 * @brief Frees an Elput_Swipe_Gesture and unreferences its associated libinput device.
 *
 * This function is intended to be used as a callback for ecore_event_add when
 * an Elput_Swipe_Gesture is no longer needed. It frees the memory allocated
 * for the gesture and decrements the reference count of the libinput device.
 *
 * @param userdata Unused.
 * @param data A pointer to the Elput_Swipe_Gesture to be freed.
 */
static void
_free_and_unref_device(void *userdata EINA_UNUSED, void *data)
{
   Elput_Swipe_Gesture *gesture;

   gesture = data;
   libinput_device_unref(gesture->device->device);
   free(gesture);
}

/**
 * @brief Processes a libinput gesture event and creates an Ecore event.
 *
 * This function is called when a libinput gesture event occurs. It extracts
 * gesture data (delta x, delta y, finger count), associates it with the
 * Elput_Device and window, and then creates an Ecore event of the specified type
 * (e.g., ELPUT_EVENT_SWIPE_BEGIN, ELPUT_EVENT_SWIPE_UPDATE, ELPUT_EVENT_SWIPE_END).
 * The libinput device is referenced to ensure it remains valid until the Ecore
 * event is processed.
 *
 * @param event The Ecore event type to create (e.g., ELPUT_EVENT_SWIPE_BEGIN).
 * @param device The libinput device that generated the event.
 * @param gesture The libinput gesture event data.
 */
static void
_eval_callback(int event, struct libinput_device *device, struct libinput_event_gesture *gesture)
{
   Elput_Device *dev;
   Elput_Swipe_Gesture *elput_gesture;

   dev = libinput_device_get_user_data(device);
   elput_gesture = calloc(1, sizeof(Elput_Swipe_Gesture));

   elput_gesture->dx = libinput_event_gesture_get_dx(gesture);
   elput_gesture->dy = libinput_event_gesture_get_dy(gesture);
   elput_gesture->finger_count =
     libinput_event_gesture_get_finger_count(gesture);
   elput_gesture->window = dev->seat->manager->window;
   elput_gesture->device = dev;

   libinput_device_ref(device);
   ecore_event_add(event, elput_gesture, _free_and_unref_device, NULL);
}

/**
 * @brief Processes a generic libinput event and dispatches gesture events.
 *
 * This function is the main handler for libinput events when gesture processing
 * is enabled. It checks the type of the libinput event and, if it's a swipe
 * gesture event (begin, update, or end), it calls _eval_callback to create
 * the corresponding Ecore event.
 *
 * @param event The libinput event to process.
 * @return 1 if the event was a processed gesture event, 0 otherwise.
 */
int
_gesture_event_process(struct libinput_event *event)
{
   struct libinput_device *dev;
   int ret = 1;

   dev = libinput_event_get_device(event);

   switch (libinput_event_get_type(event))
     {
      case LIBINPUT_EVENT_GESTURE_SWIPE_BEGIN:
        _eval_callback(ELPUT_EVENT_SWIPE_BEGIN, dev,
                       libinput_event_get_gesture_event(event));
        break;
      case LIBINPUT_EVENT_GESTURE_SWIPE_UPDATE:
        _eval_callback(ELPUT_EVENT_SWIPE_UPDATE, dev,
                       libinput_event_get_gesture_event(event));
        break;
      case LIBINPUT_EVENT_GESTURE_SWIPE_END:
        _eval_callback(ELPUT_EVENT_SWIPE_END, dev,
                       libinput_event_get_gesture_event(event));
        break;
      default:
        ret = 0;
        break;
     }

   return ret;
}
