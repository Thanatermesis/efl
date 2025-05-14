#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_FLICK_CLASS

/**
 * @brief Gets the momentum of the flick gesture.
 *
 * The momentum is a vector representing the speed and direction of the flick.
 * The x and y components represent the flick's speed in pixels per second along each axis.
 *
 * @param[in] obj The Efl_Canvas_Gesture_Flick object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture_Flick object.
 * @return The momentum vector (Eina_Vector2) of the flick.
 *         Example: { .x = 100.0, .y = -50.0 } indicates a flick moving 100 pixels/sec
 *                  to the right and 50 pixels/sec upwards.
 */
EOLIAN static Eina_Vector2
_efl_canvas_gesture_flick_momentum_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Flick_Data *pd)
{
   return pd->momentum;
}

/**
 * @brief Gets the angle of the flick gesture.
 *
 * The angle is measured in degrees, counter-clockwise from the positive x-axis.
 *
 * @param[in] obj The Efl_Canvas_Gesture_Flick object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture_Flick object.
 * @return The angle of the flick in degrees.
 *         Example: 0.0 for a flick to the right, 90.0 for a flick upwards.
 */
EOLIAN static double
_efl_canvas_gesture_flick_angle_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Flick_Data *pd)
{
   return pd->angle;
}

#include "efl_canvas_gesture_flick.eo.c"
