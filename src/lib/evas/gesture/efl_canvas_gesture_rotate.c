#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_ROTATE_CLASS

/**
 * @brief Gets the radius of the rotation gesture.
 *
 * This function returns the radius of the circular motion detected during
 * a rotation gesture. The radius is typically measured from the center
 * point between the two touch points initiating the gesture.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Gesture_Rotate.
 * @return The radius of the rotation in canvas units.
 */
EOLIAN static unsigned int
_efl_canvas_gesture_rotate_radius_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Rotate_Data *pd)
{
   return pd->radius;
}

/**
 * @brief Gets the angle of the rotation gesture.
 *
 * This function returns the amount of rotation that has occurred, in degrees.
 * A positive value indicates a clockwise rotation, while a negative value
 * indicates a counter-clockwise rotation. The angle is relative to the
 * initial orientation when the gesture began.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd The private data for the Efl_Canvas_Gesture_Rotate.
 * @return The angle of rotation in degrees. For example, a value of 45.0
 *         represents a 45-degree clockwise rotation.
 */
EOLIAN static double
_efl_canvas_gesture_rotate_angle_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Rotate_Data *pd)
{
   return pd->angle;
}

#include "efl_canvas_gesture_rotate.eo.c"
