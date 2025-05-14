#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_ZOOM_CLASS

/**
 * @brief Gets the radius of the zoom gesture.
 *
 * This function returns the average distance between fingers for a zoom gesture.
 *
 * @param[in] obj The Efl_Canvas_Gesture_Zoom object.
 * @param[in] pd The Efl_Canvas_Gesture_Zoom_Data private data.
 * @return The radius of the zoom gesture.
 */
EOLIAN static double
_efl_canvas_gesture_zoom_radius_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Zoom_Data *pd)
{
   return pd->radius;
}

/**
 * @brief Gets the zoom factor of the gesture.
 *
 * This function returns the amount of zoom applied in the gesture.
 * A value of 1.0 means no zoom.
 * A value greater than 1.0 means zoom in (magnification).
 * A value less than 1.0 means zoom out (minification).
 *
 * @param[in] obj The Efl_Canvas_Gesture_Zoom object.
 * @param[in] pd The Efl_Canvas_Gesture_Zoom_Data private data.
 * @return The zoom factor.
 */
EOLIAN static double
_efl_canvas_gesture_zoom_zoom_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Zoom_Data *pd)
{
   return pd->zoom;
}

#include "efl_canvas_gesture_zoom.eo.c"
