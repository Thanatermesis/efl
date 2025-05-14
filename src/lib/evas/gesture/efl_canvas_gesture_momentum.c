#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_MOMENTUM_CLASS

/**
 * @brief Gets the momentum of the gesture.
 *
 * This function retrieves the current momentum vector (speed and direction)
 * of the gesture. The momentum is typically set when the gesture is
 * recognized or updated.
 *
 * @param[in] obj The Efl_Canvas_Gesture_Momentum object.
 * @param[in] pd The private data for the Efl_Canvas_Gesture_Momentum object.
 * @return The momentum vector. For example, if x=10 and y=-5, it means
 *         the gesture has a momentum of 10 units in the positive x-direction
 *         and 5 units in the negative y-direction per internal time unit.
 */
EOLIAN static Eina_Vector2
_efl_canvas_gesture_momentum_momentum_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Momentum_Data *pd)
{
   return pd->momentum;
}

#include "efl_canvas_gesture_momentum.eo.c"
