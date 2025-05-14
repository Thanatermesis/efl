#define EFL_CANVAS_GESTURE_RECOGNIZER_PROTECTED
#include "efl_canvas_gesture_private.h"
#define RAD2DEG(x) ((x) * 57.295779513)

#define MY_CLASS                                    EFL_CANVAS_GESTURE_RECOGNIZER_CLASS
#include "efl_canvas_gesture_recognizer.eo.h"

/**
 * @internal
 * @brief Retrieves a configuration value for the gesture recognizer.
 *
 * This function finds the EFL_CONFIG_INTERFACE provider for the given Evas object
 * and retrieves the configuration value associated with the specified name.
 *
 * @param[in] obj The Evas object (gesture recognizer).
 * @param[in] name The name of the configuration property to retrieve.
 * @return A pointer to an Eina_Value containing the configuration value,
 *         or @c NULL if the provider or configuration is not found.
 */
Eina_Value *
_recognizer_config_get(const Eo *obj, const char *name)
{
   Eo *config = efl_provider_find(obj, EFL_CONFIG_INTERFACE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(config, NULL);
   return efl_config_get(config, name);
}

EOLIAN static Eina_Bool
_efl_canvas_gesture_recognizer_continues_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Data *pd)
{
   return pd->continues;
}

EOLIAN static void
_efl_canvas_gesture_recognizer_continues_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Data *pd, Eina_Bool value)
{
   pd->continues = !!value;
}

/**
 * @internal
 * @brief Determines the direction of movement along one axis.
 *
 * Compares two coordinate values (e.g., x-coordinates) to determine
 * if the movement is negative, positive, or none.
 *
 * @param[in] xx1 The starting coordinate.
 * @param[in] xx2 The ending coordinate.
 * @return -1 if xx2 < xx1 (movement in negative direction).
 * @return  1 if xx2 > xx1 (movement in positive direction).
 * @return  0 if xx2 == xx1 (no movement).
 */
int
_direction_get(Evas_Coord xx1, Evas_Coord xx2)
{
   if (xx2 < xx1) return -1;
   if (xx2 > xx1) return 1;

   return 0;
}

/**
 * @internal
 * @brief Checks if a touch event involves multiple touch points.
 *
 * @param[in] event The gesture touch event.
 * @return @c EINA_TRUE if the event has more than one touch point,
 *         @c EINA_FALSE otherwise.
 */
Eina_Bool
_event_multi_touch_get(const Efl_Canvas_Gesture_Touch *event)
{
   return efl_gesture_touch_points_count_get(event) > 1;
}

/**
 * @internal
 * @brief Calculates the angle in degrees between two points (xx1,yy1) and (xx2,yy2).
 *
 * The angle is calculated with respect to a coordinate system where 0 degrees
 * is to the right (positive x-axis), and angles increase counter-clockwise.
 * The function then transforms this angle to a system where 0 degrees is upwards
 * (negative y-axis), and angles increase clockwise.
 *
 * Example:
 * - (0,0) to (10,0)  -> result is 0 in atan, transformed to 90.
 * - (0,0) to (0,-10) -> result is 90 in atan, transformed to 0.
 *
 * @param[in] xx1 X-coordinate of the first point.
 * @param[in] yy1 Y-coordinate of the first point.
 * @param[in] xx2 X-coordinate of the second point.
 * @param[in] yy2 Y-coordinate of the second point.
 * @return The calculated angle in degrees, ranging from 0 to 359.9...
 *         Returns -1 if calculation is not possible (e.g. points are identical and not on axis lines).
 */
double
_angle_get(Evas_Coord xx1, Evas_Coord yy1, Evas_Coord xx2, Evas_Coord yy2)
{
   double a, xx, yy, rt = (-1);

   xx = abs(xx2 - xx1);
   yy = abs(yy2 - yy1);

   if (((int)xx) && ((int)yy))
     {
        rt = a = RAD2DEG(atan(yy / xx));
        if (xx1 < xx2)
          {
             if (yy1 < yy2) rt = 360 - a;
             else rt = a;
          }
        else
          {
             if (yy1 < yy2) rt = 180 + a;
             else rt = 180 - a;
          }
     }

   if (rt < 0) /* Do this only if rt is not set */
     {
        if (((int)xx)) /* Horizontal line */
          {
             if (xx2 < xx1) rt = 180;
             else rt = 0.0;
          }
        else /* Vertical line */
          {
             if (yy2 < yy1) rt = 90;
             else rt = 270;
          }
     }

   /* Now we want to change from:
    *                      90                   0
    * original circle   180   0   We want:  270   90
    *                     270                 180
    */
   rt = 450 - rt;
   if (rt >= 360) rt -= 360;

   return rt;
}

/**
 * @internal
 * @brief Calculates the distance (gap) between two touch points and their midpoint.
 *
 * This function computes the Euclidean distance between (xx1, yy1) and (xx2, yy2).
 * It also calculates the coordinates of the midpoint between these two points.
 *
 * @param[in] xx1 X-coordinate of the first touch point.
 * @param[in] yy1 Y-coordinate of the first touch point.
 * @param[in] xx2 X-coordinate of the second touch point.
 * @param[in] yy2 Y-coordinate of the second touch point.
 * @param[out] x Pointer to store the X-coordinate of the midpoint.
 * @param[out] y Pointer to store the Y-coordinate of the midpoint.
 * @return The distance (gap) between the two touch points as an Evas_Coord.
 */
Evas_Coord
_finger_gap_length_get(Evas_Coord xx1,
                       Evas_Coord yy1,
                       Evas_Coord xx2,
                       Evas_Coord yy2,
                       Evas_Coord *x,
                       Evas_Coord *y)
{
   double a, b, xx, yy, gap;
   xx = abs(xx2 - xx1);
   yy = abs(yy2 - yy1);
   gap = sqrt((xx * xx) + (yy * yy));

   /* START - Compute zoom center point */
   /* The triangle defined as follows:
    *             B
    *           / |
    *          /  |
    *     gap /   | a
    *        /    |
    *       A-----C
    *          b
    * http://en.wikipedia.org/wiki/Trigonometric_functions
    *************************************/
   if (((int)xx) && ((int)yy))
     {
        double A = atan((yy / xx));
        a = (Evas_Coord)((gap / 2) * sin(A));
        b = (Evas_Coord)((gap / 2) * cos(A));
        *x = (Evas_Coord)((xx2 > xx1) ? (xx1 + b) : (xx2 + b));
        *y = (Evas_Coord)((yy2 > yy1) ? (yy1 + a) : (yy2 + a));
     }
   else
     {
        if ((int)xx) /* horiz line, take half width */
          {
             *x = (Evas_Coord)((xx1 + xx2) / 2);
             *y = (Evas_Coord)(yy1);
          }

        if ((int)yy) /* vert line, take half width */
          {
             *x = (Evas_Coord)(xx1);
             *y = (Evas_Coord)((yy1 + yy2) / 2);
          }
     }
   /* END   - Compute zoom center point */

   return (Evas_Coord)gap;
}

#include "efl_canvas_gesture_recognizer.eo.c"
