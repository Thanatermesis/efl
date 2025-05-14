/**
 * @file
 * @brief This file implements the Efl.Canvas.Gesture.Recognizer.Momentum class.
 * This recognizer detects momentum gestures, typically after a finger lift,
 * to enable inertial scrolling or other physics-based animations.
 */
#include "efl_canvas_gesture_private.h"

#define MY_CLASS                       EFL_CANVAS_GESTURE_RECOGNIZER_MOMENTUM_CLASS

#define MOMENTUM_TIMEOUT               50
#define THUMBSCROLL_FRICTION           0.95
#define THUMBSCROLL_MOMENTUM_THRESHOLD 100.0
#define EFL_GESTURE_MINIMUM_MOMENTUM   0.001

/**
 * @brief Gets the specific gesture class type for this recognizer.
 * @param[in] obj The Efl.Canvas.Gesture.Recognizer.Momentum object.
 * @param[in] pd Private data for the recognizer (unused in this function).
 * @return The Efl.Canvas.Gesture.Momentum class.
 */
EOLIAN static const Efl_Class *
_efl_canvas_gesture_recognizer_momentum_efl_canvas_gesture_recognizer_type_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Momentum_Data *pd EINA_UNUSED)
{
   return EFL_CANVAS_GESTURE_MOMENTUM_CLASS;
}

/**
 * @brief Calculates and sets the momentum based on two points and timestamps.
 *
 * This function computes the velocity between two touch points (v1 at t1, v2 at t2)
 * and stores it in the Efl_Canvas_Gesture_Momentum_Data if it exceeds a threshold.
 * Configuration values for friction and threshold are read from the recognizer object.
 *
 * @param[in] obj The Efl.Canvas.Gesture.Recognizer.Momentum object, used to get configuration.
 * @param[out] md The momentum data structure to be updated with calculated momentum.
 *                Example: md->momentum.x will be set to the calculated x-velocity.
 * @param[in] v1 The coordinates of the first touch point. Example: { .x = 10, .y = 20 }
 * @param[in] v2 The coordinates of the second touch point. Example: { .x = 50, .y = 60 }
 * @param[in] t1 Timestamp of the first touch point in milliseconds. Example: 1000
 * @param[in] t2 Timestamp of the second touch point in milliseconds. Example: 1050
 */
static void
_momentum_set(Eo *obj,
              Efl_Canvas_Gesture_Momentum_Data *md,
              Eina_Position2D v1,
              Eina_Position2D v2,
              unsigned int t1,
              unsigned int t2)
{
   Evas_Coord velx = 0, vely = 0, vel;
   Evas_Coord dx = v2.x - v1.x;
   Evas_Coord dy = v2.y - v1.y;
   int dt = t2 - t1;
   Eina_Value *tf, *tmt;
   double thumbscroll_momentum_friction, thumbscroll_momentum_threshold;

   if (dt > 0)
     {
        velx = (dx * 1000) / dt;
        vely = (dy * 1000) / dt;
     }

   vel = sqrt((velx * velx) + (vely * vely));

   tf = _recognizer_config_get(obj, "thumbscroll_momentum_friction");
   if (tf) eina_value_get(tf, &thumbscroll_momentum_friction);
   else thumbscroll_momentum_friction = THUMBSCROLL_FRICTION;

   tmt = _recognizer_config_get(obj, "thumbscroll_momentum_threshold");
   if (tmt) eina_value_get(tmt, &thumbscroll_momentum_threshold);
   else thumbscroll_momentum_threshold = THUMBSCROLL_MOMENTUM_THRESHOLD;

   if ((thumbscroll_momentum_friction > 0.0) &&
       (vel > thumbscroll_momentum_threshold)) /* report
                                                * momentum */
     {
        md->momentum.x = velx;
        md->momentum.y = vely;
     }
   else
     {
        md->momentum.x = 0;
        md->momentum.y = 0;
     }
}

/**
 * @brief Recognizes a momentum gesture based on touch events.
 *
 * This is the core logic for the momentum recognizer. It processes touch events
 * (begin, update, end) to determine if a momentum gesture should be triggered,
 * continued, finished, or cancelled.
 *
 * The recognizer tracks touch points and timestamps to calculate velocity.
 * If the "glayer_continues_enable" config is true, it allows the gesture to
 * start even if the first event is not a BEGIN state, useful for continuous
 * gesture layers.
 *
 * It handles multi-touch scenarios by potentially ignoring events if a new
 * touch point appears too quickly after a previous one or if the direction
 * of movement changes significantly for a different touch ID.
 *
 * @param[in] obj The Efl.Canvas.Gesture.Recognizer.Momentum object.
 * @param[in,out] pd Private data for the recognizer, storing state like `touched`,
 *                   `t_st`, `t_end`, `st_line`, `end_line`, `xdir`, `ydir`.
 * @param[in,out] gesture The Efl.Canvas.Gesture object associated with this recognition attempt.
 *                        The hotspot and momentum data (via EFL_CANVAS_GESTURE_MOMENTUM_CLASS scope)
 *                        are updated here.
 * @param[in] watched The Efl.Object being watched for gestures (unused in this function).
 * @param[in] event The Efl.Canvas.Gesture.Touch event data.
 *                  Contains information about the current touch state, points, and timestamps.
 *                  Example: event->state can be EFL_GESTURE_TOUCH_STATE_BEGIN,
 *                           event->current_data->cur.pos are current coordinates.
 * @return An Efl_Canvas_Gesture_Recognizer_Result indicating the outcome:
 *         - EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER: Gesture is active and ongoing.
 *         - EFL_GESTURE_RECOGNIZER_RESULT_FINISH: Gesture completed successfully (momentum detected).
 *         - EFL_GESTURE_RECOGNIZER_RESULT_CANCEL: Gesture conditions not met or invalidated.
 *         - EFL_GESTURE_RECOGNIZER_RESULT_IGNORE: Event is not relevant to this gesture at this time.
 */
EOLIAN static Efl_Canvas_Gesture_Recognizer_Result
_efl_canvas_gesture_recognizer_momentum_efl_canvas_gesture_recognizer_recognize(Eo *obj,
                                                                                Efl_Canvas_Gesture_Recognizer_Momentum_Data *pd,
                                                                                Efl_Canvas_Gesture *gesture, Efl_Object *watched EINA_UNUSED,
                                                                                Efl_Canvas_Gesture_Touch *event)
{
   Eina_Value *val;
   unsigned char glayer_continues_enable;
   Efl_Canvas_Gesture_Recognizer_Result result = EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;
   Efl_Canvas_Gesture_Recognizer_Data *rd = efl_data_scope_get(obj, EFL_CANVAS_GESTURE_RECOGNIZER_CLASS);
   Efl_Canvas_Gesture_Momentum_Data *md = efl_data_scope_get(gesture, EFL_CANVAS_GESTURE_MOMENTUM_CLASS);

   val = _recognizer_config_get(obj, "glayer_continues_enable");
   if (val) eina_value_get(val, &glayer_continues_enable);
   else glayer_continues_enable = 1;

   /*
    * If glayer_continues_enable is true, this recognizer can start processing
    * events even if it missed the initial "BEGIN" state. This is useful when
    * gestures are layered.
    * The `pd->touched` flag ensures that we only do this for the very first
    * event sequence after the recognizer is reset or initialized.
    * If it's the first touch and not an END event, mark as touched and set
    * the recognizer to continuous mode.
    */
   if (glayer_continues_enable && !pd->touched)
     {
        if (efl_gesture_touch_state_get(event) != EFL_GESTURE_TOUCH_STATE_END)
          {
             /* guard against successive multi-touch cancels */
             if (efl_gesture_touch_points_count_get(event) == 1)
               {
                  pd->touched = EINA_TRUE;
                  rd->continues = EINA_TRUE;
                  md->id = -1;
               }
          }

        return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
     }

   /*
    * If already touched and a new "DOWN" action occurs (e.g., a second finger):
    * - If multiple points exist and the new touch is very close in time to the previous one,
    *   ignore it, possibly to combine it into the same event processing elsewhere or avoid jitter.
    * - If it's a single point event but very close in time to the gesture's start timestamp, ignore.
    * This helps filter out rapid multi-touch events that might not be intended for momentum.
    */
   if (pd->touched && (efl_gesture_touch_current_data_get(event)->action == EFL_POINTER_ACTION_DOWN))
     {
        /* a second finger was pressed at the same time-ish as the first: combine into same event */
        if (efl_gesture_touch_points_count_get(event) > 1)
          {
             if (efl_gesture_touch_current_timestamp_get(event) - efl_gesture_touch_previous_data_get(event)->cur.timestamp < TAP_TOUCH_TIME_THRESHOLD)
               return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
          }
        else if (efl_gesture_touch_current_timestamp_get(event) - efl_gesture_timestamp_get(gesture) < TAP_TOUCH_TIME_THRESHOLD)
          return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
     }

   /*
    * If a gesture has started (pd->t_st is non-zero), we are tracking a specific touch ID (md->id).
    * If the current event is from a *different* touch ID:
    * Check if the direction of movement for this new touch ID is different from the
    * currently tracked direction (pd->xdir, pd->ydir).
    * If the direction changes, cancel the current gesture recognition attempt and reset.
    * This prevents interference from other fingers if they move differently.
    */
   if (pd->t_st && (md->id != -1) && (md->id != efl_gesture_touch_current_data_get(event)->id))
     {
        int xdir, ydir;
        const Efl_Gesture_Touch_Point_Data *data = efl_gesture_touch_current_data_get(event);
        xdir = _direction_get(data->prev.pos.x, data->cur.pos.x);
        ydir = _direction_get(data->prev.pos.y, data->cur.pos.y);
        if ((xdir != pd->xdir) || (ydir != pd->ydir))
          {
             memset(pd, 0, sizeof(Efl_Canvas_Gesture_Recognizer_Momentum_Data));
             rd->continues = EINA_FALSE;
             return EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;
          }
        return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
     }

   switch (efl_gesture_touch_state_get(event))
     {
      case EFL_GESTURE_TOUCH_STATE_BEGIN:
      case EFL_GESTURE_TOUCH_STATE_UPDATE:
      {
         if (!pd->t_st) // If gesture hasn't started tracking points yet
           {
              // Start tracking if it's a BEGIN state, or if glayer_continues_enable is on
              if (efl_gesture_touch_state_get(event) == EFL_GESTURE_TOUCH_STATE_BEGIN ||
                  glayer_continues_enable)
                {
                   // Initialize start and end timestamps and points
                   pd->t_st = pd->t_end = efl_gesture_touch_current_timestamp_get(event);

                   pd->st_line = pd->end_line =
                       efl_gesture_touch_start_point_get(event);

                   efl_gesture_hotspot_set(gesture, pd->st_line);
                   md->id = efl_gesture_touch_current_data_get(event)->id;
                   if (efl_gesture_touch_previous_data_get(event))
                     {
                        /* if multiple fingers are pressed simultaneously, start tracking the latest finger for gesture */
                        if (efl_gesture_touch_previous_data_get(event)->action == efl_gesture_touch_current_data_get(event)->action)
                          return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
                     }
                   return EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER; // Start the gesture
                }
           }

         /*
          * If the time since the last recorded event (pd->t_end) exceeds MOMENTUM_TIMEOUT,
          * it means there was a pause in movement. Reset the start point (st_line)
          * and start time (t_st) for momentum calculation to the current event's data.
          * Also, reset the tracked direction.
          */
         if ((efl_gesture_touch_current_timestamp_get(event) - MOMENTUM_TIMEOUT) >
             pd->t_end)
           {
              pd->st_line = efl_gesture_touch_current_point_get(event);
              pd->t_st = efl_gesture_touch_current_timestamp_get(event);
              pd->xdir = pd->ydir = 0;
           }
         else // If within MOMENTUM_TIMEOUT, update based on direction changes
           {
              int xdir, ydir;
              Eina_Position2D cur_p = efl_gesture_touch_current_point_get(event);

              xdir = _direction_get(pd->end_line.x, cur_p.x);
              ydir = _direction_get(pd->end_line.y, cur_p.y);

              /*
               * If the direction of movement in X changes, update the start point's X
               * and the start time to the previous end point/time. This ensures
               * momentum is calculated from the point where the direction changed.
               */
              if (xdir && (xdir != pd->xdir))
                {
                   pd->st_line.x = pd->end_line.x;
                   pd->t_st = pd->t_end; // Use common t_st for both x/y dir change
                   pd->xdir = xdir;
                }

              /*
               * Similarly, if the direction of movement in Y changes, update the
               * start point's Y and the start time.
               */
              if (ydir && (ydir != pd->ydir))
                {
                   pd->st_line.y = pd->end_line.y;
                   // If xdir also changed, t_st is already set. If not, set it.
                   if (!(xdir && (xdir != pd->xdir))) pd->t_st = pd->t_end;
                   pd->ydir = ydir;
                }
           }

         // Update the end point and timestamp to the current event
         pd->end_line = efl_gesture_touch_current_point_get(event);
         pd->t_end = efl_gesture_touch_current_timestamp_get(event);
         efl_gesture_hotspot_set(gesture, pd->end_line);

         _momentum_set(obj, md, pd->st_line, efl_gesture_touch_current_point_get(event),
                       pd->t_st, efl_gesture_touch_current_timestamp_get(event));

         result = EFL_GESTURE_RECOGNIZER_RESULT_TRIGGER;

         break;
      }

      case EFL_GESTURE_TOUCH_STATE_END:
      {
         Eina_Bool touched = !!efl_gesture_touch_points_count_get(event);
         // If gesture tracking hadn't started (e.g. only DOWN then UP with no MOVE)
         if (!pd->t_st)
           {
              Eina_Bool prev_touched = pd->touched;

              // Update recognizer's general 'touched' and 'continues' state
              rd->continues = pd->touched = touched;

              // If it was previously touched (e.g. via glayer_continues_enable) but no movement, cancel.
              // Otherwise, if it wasn't touched and no movement, ignore.
              if (prev_touched)
                return EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;
              return EFL_GESTURE_RECOGNIZER_RESULT_IGNORE;
           }

         /*
          * Similar to the UPDATE case, if there was a pause before the END event,
          * update the start point/time for momentum calculation.
          */
         if ((efl_gesture_touch_current_timestamp_get(event) - MOMENTUM_TIMEOUT) > pd->t_end)
           {
              pd->st_line = efl_gesture_touch_current_point_get(event);
              pd->t_st = efl_gesture_touch_current_timestamp_get(event);
              pd->xdir = pd->ydir = 0;
           }

         // Final update of end point, timestamp, and hotspot
         pd->end_line = efl_gesture_touch_current_point_get(event);
         pd->t_end = efl_gesture_touch_current_timestamp_get(event);
         rd->continues = touched; // Update continues based on remaining touch points
         efl_gesture_hotspot_set(gesture, pd->end_line);

         // If calculated momentum is above a minimum threshold, finish the gesture.
         // Otherwise, cancel it.
         if ((fabs(md->momentum.x) > EFL_GESTURE_MINIMUM_MOMENTUM) ||
             (fabs(md->momentum.y) > EFL_GESTURE_MINIMUM_MOMENTUM))
           result = EFL_GESTURE_RECOGNIZER_RESULT_FINISH;
         else
           result = EFL_GESTURE_RECOGNIZER_RESULT_CANCEL;

         // Reset recognizer's private data for the next gesture, preserving 'touched' state.
         memset(pd, 0, sizeof(Efl_Canvas_Gesture_Recognizer_Momentum_Data));
         pd->touched = touched;

         break;
      }

      default:

        break;
     }

   return result;
}

#include "efl_canvas_gesture_recognizer_momentum.eo.c"
