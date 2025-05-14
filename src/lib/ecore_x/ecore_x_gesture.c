#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "ecore_x_private.h"

/*
 * @brief Query whether gesture is available or not.
 *
 * @return @c EINA_TRUE, if extension is available, @c EINA_FALSE otherwise.
 *
 * @deprecated
 */
EAPI Eina_Bool
ecore_x_gesture_supported(void)
{
   return EINA_FALSE;
}

/**
 * @brief Select gesture events for a given window.
 * @param win The window to select events for.
 * @param mask A bitmask of gesture events to select.
 *        (e.g., ECORE_X_GESTURE_EVENT_MASK_TAP, ECORE_X_GESTURE_EVENT_MASK_SWIPE)
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @note This function currently always returns @c EINA_FALSE as gesture support
 *       is not implemented.
 */
EAPI Eina_Bool
ecore_x_gesture_events_select(Ecore_X_Window win EINA_UNUSED,
                              Ecore_X_Gesture_Event_Mask mask EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Get the mask of selected gesture events for a given window.
 * @param win The window to query.
 * @return The mask of currently selected gesture events.
 * @note This function currently always returns @c ECORE_X_GESTURE_EVENT_MASK_NONE
 *       as gesture support is not implemented.
 */
EAPI Ecore_X_Gesture_Event_Mask
ecore_x_gesture_events_selected_get(Ecore_X_Window win EINA_UNUSED)
{
   return ECORE_X_GESTURE_EVENT_MASK_NONE;
}

/**
 * @brief Grab a specific gesture event on a window.
 * @param win The window on which to grab the gesture.
 * @param type The type of gesture event to grab (e.g., ECORE_X_GESTURE_EVENT_TYPE_TAP).
 * @param num_fingers The number of fingers involved in the gesture.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @note This function currently always returns @c EINA_FALSE as gesture support
 *       is not implemented.
 */
EAPI Eina_Bool
ecore_x_gesture_event_grab(Ecore_X_Window win EINA_UNUSED,
                           Ecore_X_Gesture_Event_Type type EINA_UNUSED,
                           int num_fingers EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Ungrab a specific gesture event on a window.
 * @param win The window from which to ungrab the gesture.
 * @param type The type of gesture event to ungrab.
 * @param num_fingers The number of fingers involved in the gesture.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @note This function currently always returns @c EINA_FALSE as gesture support
 *       is not implemented.
 */
EAPI Eina_Bool
ecore_x_gesture_event_ungrab(Ecore_X_Window win EINA_UNUSED,
                             Ecore_X_Gesture_Event_Type type EINA_UNUSED,
                             int num_fingers EINA_UNUSED)
{
   return EINA_FALSE;
}
