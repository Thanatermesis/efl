#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Eina.h>
#include <Elementary.h>

//#define DEBUGON
#ifdef DEBUGON
# define gl_debug(x...) fprintf(stderr, __FILE__": " x)
#else
# define gl_debug(x...) do { } while (0)
#endif

/**
 * @brief Structure to hold callback function and user data.
 * This structure is used to manage callbacks associated with gesture events.
 */
struct _Func_Data
{
   EINA_INLIST; /**< Macro for Eina_Inlist node */
   void                 *user_data; /**< User data to be passed to the callback function. */
   Elm_Gesture_Event_Cb cb; /**< The callback function pointer. */
};
typedef struct _Func_Data Func_Data;

/**
 * @brief Structure to manage tap and long-press gesture information.
 * This structure holds all necessary data to detect and handle
 * tap, multi-tap, and long-press gestures on an Evas_Object.
 */
struct _Tap_Longpress_Info
{
   Evas_Object          *obj; /**< The Evas_Object this gesture info is associated with. */
   Eina_Inlist          *cbs[ELM_GESTURE_STATE_ABORT + 1]; /**< Array of Eina_Inlist, storing Func_Data for each gesture state (START, MOVE, END, ABORT).
                                                              Each list contains callbacks registered for that specific state.
                                                              Example: cbs[ELM_GESTURE_STATE_START] might contain Func_Data for multiple start callbacks. */
   void                 *data; /**< Generic data pointer, can be used by the gesture logic. (Currently unused in this specific implementation for Tap_Longpress_Info itself, but Func_Data within cbs uses its own user_data). */

   Ecore_Timer          *timer_between_taps; /**< Timer to detect if subsequent taps form a multi-tap or if a long-press timeout occurs. */
   unsigned int         nb_taps_on_single : 7; /**< Number of taps detected in the current sequence (e.g., 1 for single tap, 2 for double tap). Limited to 7 bits. */
   Eina_Bool            long_tap_started  : 1; /**< Boolean flag indicating if a long-press gesture has started. Limited to 1 bit. */
};

typedef struct _Tap_Longpress_Info Tap_Longpress_Info;

/**
 * @brief Calls registered callbacks for a given gesture state.
 * Iterates through the list of callbacks registered for the specified state
 * and executes them.
 * @param info The gesture information structure.
 * @param state The gesture state for which to call callbacks.
 * @param event_info Event-specific information to pass to the callback.
 * @return Evas_Event_Flags indicating how the event was handled by the callbacks.
 */
static Evas_Event_Flags
_cb_call(Tap_Longpress_Info *info, Elm_Gesture_State state, void *event_info)
{
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   Func_Data *cb_info;
   EINA_INLIST_FOREACH(info->cbs[state], cb_info)
      flags |= cb_info->cb(cb_info->user_data, event_info);
   return flags;
}

/**
 * @brief Callback for the start of a single tap gesture.
 * This function is invoked when the gesture layer detects the beginning
 * of a tap sequence (N_TAPS START). It triggers the ELM_GESTURE_STATE_START
 * callbacks for the tap-longpress gesture if it's the first tap in a potential sequence.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information from the gesture layer.
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_single_tap_start_cb(void *data, void *event_info)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (!info->nb_taps_on_single)
     {
        gl_debug("\n%s\n", __func__);
        _cb_call(info, ELM_GESTURE_STATE_START, event_info);
     }
   return flags;
}

/**
 * @brief Callback for the abortion of a single tap gesture.
 * This function is invoked when the gesture layer aborts a tap sequence
 * (N_TAPS ABORT). It triggers the ELM_GESTURE_STATE_ABORT callbacks
 * for the tap-longpress gesture if a long tap hasn't started.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information (unused).
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_single_tap_abort_cb(void *data, void *event_info EINA_UNUSED)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (!info->long_tap_started)
     {
        gl_debug("%s\n", __func__);
        _cb_call(info, ELM_GESTURE_STATE_ABORT, NULL);
        info->nb_taps_on_single = 0;
     }
   return flags;
}

/**
 * @brief Timeout callback for detecting the end of a multi-tap sequence.
 * If this timer expires, it means no further taps occurred within the
 * double_tap_timeout window, so the current tap sequence is considered complete
 * and aborted if it wasn't already part of a long press.
 * @param data The Tap_Longpress_Info structure.
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_tap_long_timeout(void *data)
{
   gl_debug("%s\n", __func__);
   Tap_Longpress_Info *info = data;
   _tap_long_single_tap_abort_cb(info, NULL);
   info->timer_between_taps = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback for the end of a single tap gesture.
 * This function is invoked when the gesture layer detects the end of a tap
 * (N_TAPS END). It starts a timer to listen for subsequent taps (for multi-tap)
 * and updates the tap count.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information, contains tap count.
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_single_tap_end_cb(void *data, void *event_info)
{
   gl_debug("%s\n", __func__);
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   double timeout_between_taps = elm_gesture_layer_double_tap_timeout_get(info->obj);
   info->timer_between_taps = ecore_timer_add(timeout_between_taps,
                            _tap_long_timeout, info);
   info->nb_taps_on_single = ((Elm_Gesture_Taps_Info *)event_info)->n;
   return flags;
}

/**
 * @brief Callback for the start of a long tap gesture.
 * This function is invoked when the gesture layer detects the start of a long tap
 * (N_LONG_TAPS START). If a single tap sequence was active (timer running),
 * it cancels the timer and marks that a long tap has started.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information (unused).
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_long_tap_start_cb(void *data, void *event_info EINA_UNUSED)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (info->nb_taps_on_single && info->timer_between_taps)
     {
        gl_debug("%s\n", __func__);
        info->long_tap_started = EINA_TRUE;
        ecore_timer_del(info->timer_between_taps);
        info->timer_between_taps = NULL;
     }
   return flags;
}

/**
 * @brief Callback for the abortion of a long tap gesture.
 * This function is invoked when the gesture layer aborts a long tap
 * (N_LONG_TAPS ABORT). If a long tap was active, it calls the
 * ELM_GESTURE_STATE_ABORT callbacks and resets the long tap state.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information (unused).
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_long_tap_abort_cb(void *data, void *event_info EINA_UNUSED)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (info->long_tap_started)
     {
        gl_debug("%s\n", __func__);
        _cb_call(info, ELM_GESTURE_STATE_ABORT, NULL);
        info->nb_taps_on_single = 0;
        info->long_tap_started = EINA_FALSE;
     }
   return flags;
}

/**
 * @brief Callback for movement during a long tap gesture.
 * This function is invoked when movement occurs while a long tap is active
 * (N_LONG_TAPS MOVE). It checks if the number of fingers involved in the
 * gesture has changed; if so, it aborts the long tap. Otherwise, it calls
 * the ELM_GESTURE_STATE_MOVE callbacks.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information, contains tap count.
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_long_tap_move_cb(void *data, void *event_info)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (info->long_tap_started)
     {
        if (((Elm_Gesture_Taps_Info *)event_info)->n != info->nb_taps_on_single)
          {
             _tap_long_long_tap_abort_cb(info, NULL);
          }
        else
          {
             gl_debug("%s\n", __func__);
             _cb_call(info, ELM_GESTURE_STATE_MOVE, event_info);
          }
     }
   return flags;
}

/**
 * @brief Callback for the end of a long tap gesture.
 * This function is invoked when the gesture layer detects the end of a long tap
 * (N_LONG_TAPS END). If a long tap was active, it calls the
 * ELM_GESTURE_STATE_END callbacks and resets the gesture state.
 * @param data The Tap_Longpress_Info structure.
 * @param event_info Event-specific information.
 * @return Evas_Event_Flags, always EVAS_EVENT_FLAG_NONE in this implementation.
 */
static Evas_Event_Flags
_tap_long_long_tap_end_cb(void *data, void *event_info)
{
   Tap_Longpress_Info *info = data;
   Evas_Event_Flags flags = EVAS_EVENT_FLAG_NONE;
   if (info->long_tap_started)
     {
        gl_debug("%s\n", __func__);
        _cb_call(info, ELM_GESTURE_STATE_END, event_info);
        info->long_tap_started = EINA_FALSE;
        info->nb_taps_on_single = 0;
     }
   return flags;
}

/**
 * @brief Callback for Evas object deletion.
 * This function is called when the Evas_Object associated with the
 * tap-longpress gesture is deleted. It cleans up all resources,
 * including registered callbacks, timers, and allocated memory.
 * @param data User data associated with the event callback (unused).
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object being deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_object_delete(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Tap_Longpress_Info *info = evas_object_data_get(obj, "Tap-Longpress");
   if (info)
     {
        Eina_Inlist *itr;
        Func_Data *cb_info;
        int state;
        for (state = ELM_GESTURE_STATE_START; state <= ELM_GESTURE_STATE_ABORT; state++)
          {
             EINA_INLIST_FOREACH_SAFE(info->cbs[state], itr, cb_info)
               {
                  info->cbs[state] = eina_inlist_remove(
                        info->cbs[state], EINA_INLIST_GET(cb_info));
                  free(cb_info);
               }
          }
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_START, _tap_long_single_tap_start_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_ABORT, _tap_long_single_tap_abort_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_END, _tap_long_single_tap_end_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_START, _tap_long_long_tap_start_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_MOVE, _tap_long_long_tap_move_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_ABORT, _tap_long_long_tap_abort_cb, info);
        elm_gesture_layer_cb_del(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_END, _tap_long_long_tap_end_cb, info);
        evas_object_data_del(obj, "Tap-Longpress");
        free(info);
     }
}

/**
 * @brief Adds a callback for a specific tap-longpress gesture state.
 * Registers a callback function to be invoked when the tap-longpress gesture
 * reaches the specified state on the given Evas_Object.
 * If this is the first callback added for this object, it initializes
 * the Tap_Longpress_Info structure and sets up internal gesture layer callbacks.
 * @param obj The Evas_Object to attach the gesture callback to.
 * @param state The Elm_Gesture_State to listen for (START, MOVE, END, ABORT).
 * @param cb The user-defined callback function.
 * @param data User data to be passed to the callback function.
 *
 * @see Elm_Gesture_State
 * @see Elm_Gesture_Event_Cb
 *
 * Example:
 * @code
 * void my_tap_start_cb(void *my_data, void *event_info) {
 *   printf("Tap started!\n");
 *   Elm_Gesture_Taps_Info *tap_info = event_info;
 *   // Access tap_info->x, tap_info->y, tap_info->n (number of taps)
 * }
 *
 * elm_gesture_layer_tap_longpress_cb_add(my_object, ELM_GESTURE_STATE_START, my_tap_start_cb, user_specific_data);
 * @endcode
 */
EAPI void elm_gesture_layer_tap_longpress_cb_add(Evas_Object *obj, Elm_Gesture_State state, Elm_Gesture_Event_Cb cb, void *data)
{
   Tap_Longpress_Info *info = evas_object_data_get(obj, "Tap-Longpress");
   if (!info)
     {
        info = calloc(1, sizeof(*info));
        if (!info) return;

        info->obj = obj;
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_START, _tap_long_single_tap_start_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_ABORT, _tap_long_single_tap_abort_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_END, _tap_long_single_tap_end_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_START, _tap_long_long_tap_start_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_MOVE, _tap_long_long_tap_move_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_ABORT, _tap_long_long_tap_abort_cb, info);
        elm_gesture_layer_cb_add(obj, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_END, _tap_long_long_tap_end_cb, info);
        evas_object_data_set(obj, "Tap-Longpress", info);
        evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, _object_delete, NULL);
     }

   Func_Data *cb_info = calloc(1, sizeof(*cb_info));
   if (!cb_info) return;
   cb_info->cb = cb;
   cb_info->user_data = data;
   info->cbs[state] = eina_inlist_append(info->cbs[state],
         EINA_INLIST_GET(cb_info));
}

/**
 * @brief Deletes a previously added tap-longpress gesture callback.
 * Removes a specific callback function that was registered for a given state
 * on an Evas_Object. If this is the last callback for the object,
 * it cleans up the associated Tap_Longpress_Info and internal gesture callbacks.
 * @param obj The Evas_Object from which to remove the gesture callback.
 * @param state The Elm_Gesture_State the callback was registered for.
 * @param cb The callback function to remove.
 * @param data The user data that was passed when adding the callback.
 *
 * @see elm_gesture_layer_tap_longpress_cb_add
 */
EAPI void elm_gesture_layer_tap_longpress_cb_del(Evas_Object *obj, Elm_Gesture_State state, Elm_Gesture_Event_Cb cb, void *data)
{
   Tap_Longpress_Info *info = evas_object_data_get(obj, "Tap-Longpress");
   if (!info) return;

   Eina_Inlist *itr;
   Func_Data *cb_info;
   EINA_INLIST_FOREACH_SAFE(info->cbs[state], itr, cb_info)
     {
        if (cb_info->cb == cb && cb_info->user_data == data)
          {
             info->cbs[state] = eina_inlist_remove(
                   info->cbs[state], EINA_INLIST_GET(cb_info));
             free(cb_info);
             break;
          }
     }
   if (!info->cbs[ELM_GESTURE_STATE_START] &&
         !info->cbs[ELM_GESTURE_STATE_MOVE] &&
         !info->cbs[ELM_GESTURE_STATE_END] &&
         !info->cbs[ELM_GESTURE_STATE_ABORT])
     {
        _object_delete(NULL, NULL, obj, NULL);
     }
}
