#include "efl_canvas_gesture_private.h"

#define MY_CLASS     EFL_CANVAS_GESTURE_RECOGNIZER_TRIPLE_TAP_CLASS

#define TAP_TIME_OUT 0.33

/**
 * @brief Gets the gesture type class for the triple tap recognizer.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data for the triple tap recognizer.
 * @return The Efl_Canvas_Gesture_Triple_Tap class.
 */
EOLIAN static const Efl_Class *
_efl_canvas_gesture_recognizer_triple_tap_efl_canvas_gesture_recognizer_type_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Triple_Tap_Data *pd EINA_UNUSED)
{
   return EFL_CANVAS_GESTURE_TRIPLE_TAP_CLASS;
}

/**
 * @brief Destructor for the Efl_Canvas_Gesture_Recognizer_Triple_Tap object.
 *
 * This function is called when the triple tap recognizer object is being destroyed.
 * It ensures that any active timers are deleted.
 *
 * @param[in] obj The Eolian object to destruct.
 * @param[in] pd The private data associated with the object.
 */
EOLIAN static void
_efl_canvas_gesture_recognizer_triple_tap_efl_object_destructor(Eo *obj,
                                                                Efl_Canvas_Gesture_Recognizer_Triple_Tap_Data *pd)
{
   if (pd->timeout)
     ecore_timer_del(pd->timeout);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Callback function for the tap timeout.
 *
 * This function is called when the time between taps exceeds the allowed limit.
 * It cancels the current gesture recognition attempt and resets the tap count.
 *
 * @param[in] data The Eolian object (recognizer) associated with this timer.
 * @return ECORE_CALLBACK_CANCEL to automatically delete the timer.
 */
static Eina_Bool
_tap_timeout_cb(void *data)
{
   Efl_Canvas_Gesture_Recognizer_Triple_Tap_Data *pd;

   pd = efl_data_scope_get(data, EFL_CANVAS_GESTURE_RECOGNIZER_TRIPLE_TAP_CLASS);

   efl_gesture_state_set(pd->gesture, EFL_GESTURE_STATE_CANCELED);
   efl_event_callback_call(pd->target, EFL_EVENT_GESTURE_TRIPLE_TAP, pd->gesture);

   efl_gesture_manager_recognizer_cleanup(efl_provider_find(data, EFL_CANVAS_GESTURE_MANAGER_CLASS), data, pd->target);

   pd->timeout = NULL;
   pd->tap_count = 0;

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Recognizes a triple tap gesture based on touch events.
 *
 * This function processes incoming touch events to determine if a triple tap
 * gesture has occurred. It manages tap counting, timeout between taps, and
 * finger movement tolerance.
 *
 * @param[in] obj The Eolian object (recognizer).
 * @param[in] pd The private data for the triple tap recognizer.
 * @param[in] gesture The gesture object to be updated.
 * @param[in] watched The Efl_Object being watched for gestures.
 * @param[in] event The touch event data.
 * @return An Efl_Canvas_Gesture_Recognizer_Result indicating the outcome of the
 *         recognition process for this event (e.g., TRIGGER, FINISH, CANCEL, IGNORE).
 */
EOLIAN static Efl_Canvas_Gesture_Recognizer_Result
_efl_canvas_gesture_recognizer_triple_tap_efl_canvas_gesture_recognizer_recognize(Eo *obj,
                                                                                  Efl_Canvas_Gesture_Recognizer_Triple_Tap_Data *pd,
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

   // Initialize timeout if not already set, possibly from configuration.
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
         pos = efl_gesture_touch_start_point_get(event);
         efl_gesture_hotspot_set(gesture, pos);

         // Start or reset the tap timeout timer.
         if (pd->timeout)
           ecore_timer_reset(pd->timeout);
         else
           pd->timeout = ecore_timer_add(timeout, _tap_timeout_cb, obj);

         // Indicate that this event could be part of a gesture.
         result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;

         break;
      }

      case EFL_GESTURE_TOUCH_STATE_UPDATE:
      {
        /* multi-touch */
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

         // Check for finger movement if a gesture is already in progress and it's not a multi-touch event.
         if (efl_gesture_state_get(gesture) != EFL_GESTURE_STATE_NONE &&
             !_event_multi_touch_get(event))
           {
              dist = efl_gesture_touch_distance(event, efl_gesture_touch_current_data_get(event)->id);
              length = fabs(dist.x) + fabs(dist.y);

              // If finger moved too far, cancel the gesture.
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
         // Process tap if a gesture is active and it's not a multi-touch event.
         if (efl_gesture_state_get(gesture) != EFL_GESTURE_STATE_NONE &&
             !_event_multi_touch_get(event))
           {
              // Handle potential multi-touch scenarios where a second finger lift might be misinterpreted.
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

              // Check if finger moved within tolerance.
              if (length <= pd->finger_size)
                {
                   pd->tap_count++;
                   if (pd->tap_count < 3) // Not yet a triple tap
                     {
                        if (pd->timeout) // Reset timeout for the next tap.
                          ecore_timer_reset(pd->timeout);

                        result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;
                     }
                   else // Triple tap achieved.
                     {
                        if (pd->timeout) // Clean up timer.
                          {
                             ecore_timer_del(pd->timeout);
                             pd->timeout = NULL;
                          }

                        // Finalize the gesture.
                        if (efl_gesture_touch_state_get(event) == EFL_GESTURE_TOUCH_STATE_END)
                          result = EFL_GESTURE_RECOGNIZER_RESULT_FINISH;
                        else // Should ideally be END, but handle other cases by triggering.
                          result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;

                        pd->tap_count = 0; // Reset tap count for next gesture.
                     }
                }
              else // Finger moved too far, cancel.
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

      default:
        // Other touch states are not handled for triple tap.
        break;
     }

   return result;
}

#include "efl_canvas_gesture_recognizer_triple_tap.eo.c"
