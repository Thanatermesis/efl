#define EFL_CANVAS_GESTURE_PROTECTED
#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_CLASS

/**
 * @brief Gets the current state of the gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @return The current state of the gesture.
 */
EOLIAN static Efl_Canvas_Gesture_State
_efl_canvas_gesture_state_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd)
{
   return pd->state;
}

/**
 * @brief Sets the state of the gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @param[in] state The new state to set for the gesture.
 */
EOLIAN static void
_efl_canvas_gesture_state_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd, Efl_Canvas_Gesture_State state)
{
   pd->state = state;
}

/**
 * @brief Sets the hotspot for the gesture.
 * The hotspot is typically the center point of the gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @param[in] hotspot The 2D position to set as the hotspot.
 *                    Example: { .x = 100, .y = 150 }
 */
EOLIAN static void
_efl_canvas_gesture_hotspot_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd, Eina_Position2D hotspot)
{
   pd->hotspot = hotspot;
}

/**
 * @brief Gets the hotspot for the gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @return The 2D position of the hotspot.
 */
EOLIAN static Eina_Position2D
_efl_canvas_gesture_hotspot_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd)
{
   return pd->hotspot;
}

/**
 * @brief Sets the timestamp for the gesture event.
 * This usually represents the time when the gesture event occurred.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @param[in] timestamp The timestamp to set, typically in milliseconds.
 */
EOLIAN static void
_efl_canvas_gesture_timestamp_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd, unsigned int timestamp)
{
   pd->timestamp = timestamp;
}

/**
 * @brief Gets the timestamp for the gesture event.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @return The timestamp of the gesture event.
 */
EOLIAN static unsigned int
_efl_canvas_gesture_timestamp_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd)
{
   return pd->timestamp;
}

/**
 * @brief Sets the number of touch points involved in the gesture.
 * For example, a pinch gesture typically involves two touch points.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @param[in] touch_count The number of touch points.
 */
EOLIAN static void
_efl_canvas_gesture_touch_count_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd, unsigned int touch_count)
{
   pd->touch_count = touch_count;
}

/**
 * @brief Gets the number of touch points involved in the gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture object.
 * @param[in] pd The private data of the Efl_Canvas_Gesture object.
 * @return The number of touch points.
 */
EOLIAN static unsigned int
_efl_canvas_gesture_touch_count_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Data *pd)
{
   return pd->touch_count;
}

#include "efl_canvas_gesture.eo.c"
#include "efl_canvas_gesture_events.eo.c"
