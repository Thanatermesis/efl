#include "efl_canvas_gesture_private.h"

#define MY_CLASS     EFL_CANVAS_GESTURE_RECOGNIZER_DOUBLE_TAP_CLASS

#define TAP_TIME_OUT 0.33

/**
 * @brief Gets the gesture type for the double tap recognizer.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data for the double tap recognizer.
 * @return The Efl_Canvas_Gesture_Double_Tap class.
 */
EOLIAN static const Efl_Class *
_efl_canvas_gesture_recognizer_double_tap_efl_canvas_gesture_recognizer_type_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Double_Tap_Data *pd EINA_UNUSED)
{
   return EFL_CANVAS_GESTURE_DOUBLE_TAP_CLASS;
}

/**
 * @brief Destructor for the Efl_Canvas_Gesture_Recognizer_Double_Tap object.
 *
 * This function is called when the Eolian object is being destroyed.
 * It cleans up any resources allocated by the recognizer, such as timers.
 *
 * @param[in] obj The Eolian object being destroyed.
 * @param[in] pd The private data associated with the object.
 */
EOLIAN static void
_efl_canvas_gesture_recognizer_double_tap_efl_object_destructor(Eo *obj,
                                                                Efl_Canvas_Gesture_Recognizer_Double_Tap_Data *pd)
{
   if (pd->timeout)
     ecore_timer_del(pd->timeout);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Callback function for the tap timeout.
 *
 * This function is called when the timer for detecting a double tap expires.
 * If the timer expires, it means a double tap did not occur within the
 * allowed time frame. The gesture state is set to CANCELED, and a
 * corresponding event is triggered.
 *
 * @param[in] data The Eolian object (recognizer) associated with this timer.
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_tap_timeout_cb(void *data)
{
   Efl_Canvas_Gesture_Recognizer_Double_Tap_Data *pd;

   pd = efl_data_scope_get(data, EFL_CANVAS_GESTURE_RECOGNIZER_DOUBLE_TAP_CLASS);

   efl_gesture_state_set(pd->gesture, EFL_GESTURE_STATE_CANCELED);
   efl_event_callback_call(pd->target, EFL_EVENT_GESTURE_DOUBLE_TAP, pd->gesture);

   efl_gesture_manager_recognizer_cleanup(efl_provider_find(data, EFL_CANVAS_GESTURE_MANAGER_CLASS), data,
     pd->target);

   pd->timeout = NULL;
   pd->tap_count = 0;

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Recognizes a double tap gesture based on touch events.
 *
 * This function processes incoming touch events to determine if a double tap
 * gesture has occurred. It manages tap counting, timing, and finger movement
 * thresholds.
 *
 * @param[in] obj The Eolian object (recognizer).
 * @param[in] pd The private data for the double tap recognizer.
 * @param[in] gesture The gesture object to update.
 * @param[in] watched The object being watched for gestures.
 * @param[in] event The touch event data.
 * @return An Efl_Canvas_Gesture_Recognizer_Result indicating the outcome of
 *         the recognition process (e.g., TRIGGER, FINISH, CANCEL, IGNORE).
 */
EOLIAN static Efl_Canvas_Gesture_Recognizer_Result
_efl_canvas_gesture_recognizer_double_tap_efl_canvas_gesture_recognizer_recognize(Eo *obj,
                                                                                  Efl_Canvas_Gesture_Recognizer_Double_Tap_Data *pd,
                                                                                  Efl_Canvas_Gesture *gesture, Efl_Object *watched,
                                                                                  Efl_Canvas_Gesture_Touch *event)
{
   double length, start_timeout = pd->start_timeout;
   double timeout = TAP_TIME_OUT;
   Eina_Position2D pos;
   Eina_Vector2 dist;
   Efl_Canvas_Gesture_Recognizer_Result result = EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;

   pd->target = watched;
   pd->gesture = gesture;

   if (!EINA_DBL_NONZERO(start_timeout))
     {
        double time;
        Eina_Value *val = _recognizer_config_get(obj, "glayer_double_tap_timeout");

        if (val)
          {
             eina_value_get(val, &time);
             pd->start_timeout = timeout = time;
          }
     }
   else
     timeout = start_timeout;

   switch (efl_gesture_touch_state_get(event))
     {
      case EFL_GESTURE_TOUCH_STATE_BEGIN:
      {
         /// A new touch sequence has started.
         /// Set the initial hotspot for the gesture.
         pos = efl_gesture_touch_start_point_get(event);
         efl_gesture_hotspot_set(gesture, pos);

         /// If a timeout timer is already running (e.g., from a previous tap),
         /// reset it. Otherwise, start a new timer.
         if (pd->timeout)
           ecore_timer_reset(pd->timeout);
         else
           pd->timeout = ecore_timer_add(timeout, _tap_timeout_cb, obj);

         result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;

         break;
      }

      case EFL_GESTURE_TOUCH_STATE_UPDATE:
      {
        /* multi-touch */
        /// Check for a second finger press occurring almost simultaneously with the first.
        /// If so, this is likely part of a multi-touch interaction, not a double tap,
        /// so ignore it for this recognizer.
        if (efl_gesture_touch_current_data_get(event)->action == EFL_POINTER_ACTION_DOWN)
          {
             /* a second finger was pressed at the same time-ish as the first: combine into same event */
             if (efl_gesture_touch_current_timestamp_get(event) - efl_gesture_timestamp_get(gesture) < TAP_TOUCH_TIME_THRESHOLD)
               {
                  result = EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
                  break;
               }
          }
         result = EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;

         /// If the gesture is active and it's not a multi-touch event,
         /// check for finger movement.
         if (efl_gesture_state_get(gesture) != EFL_GESTURE_STATE_NONE &&
             !_event_multi_touch_get(event))
           {
              dist = efl_gesture_touch_distance(event, efl_gesture_touch_current_data_get(event)->id);
              length = fabs(dist.x) + fabs(dist.y);

              /// If the finger has moved beyond the allowed threshold,
              /// cancel the double tap recognition.
              if (length > pd->finger_size)
                {
                   if (pd->timeout)
                     {
                        ecore_timer_del(pd->timeout);
                        pd->timeout = NULL;
                     }

                   result = EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;

                   pd->tap_count = 0;
                }
           }

         break;
      }

      case EFL_GESTURE_TOUCH_STATE_END:
      {
         /// A touch sequence has ended (finger lifted).
         if (efl_gesture_state_get(gesture) != EFL_GESTURE_STATE_NONE &&
             !_event_multi_touch_get(event))
           {
              /// Check for multi-touch scenarios where a second finger lift might occur
              /// close in time to the first, which should be ignored for double tap.
              if (efl_gesture_touch_previous_data_get(event))
                {
                   Efl_Pointer_Action prev_act = efl_gesture_touch_previous_data_get(event)->action;
                   /* multi-touch */
                   if ((prev_act == EFL_POINTER_ACTION_UP) || (prev_act == EFL_POINTER_ACTION_CANCEL))
                     {
                        /* a second finger was pressed at the same time-ish as the first: combine into same event */
                        if (efl_gesture_touch_current_timestamp_get(event) - efl_gesture_timestamp_get(gesture) < TAP_TOUCH_TIME_THRESHOLD)
                          {
                             result = EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
                             break;
                          }
                     }
                }
              dist = efl_gesture_touch_distance(event, efl_gesture_touch_current_data_get(event)->id);
              length = fabs(dist.x) + fabs(dist.y);

              /// If the finger movement was within the allowed threshold for a tap.
              if (length <= pd->finger_size)
                {
                   pd->tap_count++;
                   if (pd->tap_count == 1) /// First tap detected.
                     {
                        /// Reset the timeout timer to wait for the second tap.
                        if (pd->timeout)
                          ecore_timer_reset(pd->timeout);

                        result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;
                     }
                   else /// Second tap detected (pd->tap_count would be 2).
                     {
                        /// Double tap successful, cancel the timeout timer.
                        if (pd->timeout)
                          {
                             ecore_timer_del(pd->timeout);
                             pd->timeout = NULL;
                          }

                        /// If the touch state is END, the gesture is finished.
                        /// Otherwise, it's still triggering (e.g. if it was a CANCEL state that still met tap criteria).
                        if (efl_gesture_touch_state_get(event) == EFL_GESTURE_TOUCH_STATE_END)
                          result = EFL_GESTURE_RECOGNIZER_RESULT_FINISH;
                        else
                          result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;

                        pd->tap_count = 0; /// Reset tap count for the next potential double tap.
                     }
                }
              else /// Finger moved too much, not a valid tap.
                {
                   /// Cancel the timeout and the gesture recognition.
                   if (pd->timeout)
                     {
                        ecore_timer_del(pd->timeout);
                        pd->timeout = NULL;
                     }

                   result = EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;

                   pd->tap_count = 0;
                }
           }

         break;
      }

      default:

        break;
     }

   return result;
}

#include "efl_canvas_gesture_recognizer_double_tap.eo.c"
