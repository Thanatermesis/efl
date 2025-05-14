/**
 * @brief Sets the zoom step for the gesture layer.
 * @param obj The gesture layer object.
 * @param step The zoom step value.
 * @see elm_obj_gesture_layer_zoom_step_set()
 */
EAPI void
elm_gesture_layer_zoom_step_set(Elm_Gesture_Layer *obj, double step)
{
   elm_obj_gesture_layer_zoom_step_set(obj, step);
}

/**
 * @brief Gets the zoom step for the gesture layer.
 * @param obj The gesture layer object.
 * @return The zoom step value.
 * @see elm_obj_gesture_layer_zoom_step_get()
 */
EAPI double
elm_gesture_layer_zoom_step_get(const Elm_Gesture_Layer *obj)
{
   return elm_obj_gesture_layer_zoom_step_get(obj);
}

/**
 * @brief Sets the finger size for tap gestures.
 * @param obj The gesture layer object.
 * @param sz The finger size. Use 0 for system default.
 * @see elm_obj_gesture_layer_tap_finger_size_set()
 */
EAPI void
elm_gesture_layer_tap_finger_size_set(Elm_Gesture_Layer *obj, int sz)
{
   elm_obj_gesture_layer_tap_finger_size_set(obj, sz);
}

/**
 * @brief Gets the finger size for tap gestures.
 * @param obj The gesture layer object.
 * @return The finger size.
 * @see elm_obj_gesture_layer_tap_finger_size_get()
 */
EAPI int
elm_gesture_layer_tap_finger_size_get(const Elm_Gesture_Layer *obj)
{
   return elm_obj_gesture_layer_tap_finger_size_get(obj);
}

/**
 * @brief Sets whether to hold events if no gesture is detected.
 * @param obj The gesture layer object.
 * @param hold_events EINA_TRUE to hold events, EINA_FALSE otherwise.
 * @see elm_obj_gesture_layer_hold_events_set()
 */
EAPI void
elm_gesture_layer_hold_events_set(Elm_Gesture_Layer *obj, Eina_Bool hold_events)
{
   elm_obj_gesture_layer_hold_events_set(obj, hold_events);
}

/**
 * @brief Gets whether events are held if no gesture is detected.
 * @param obj The gesture layer object.
 * @return EINA_TRUE if events are held, EINA_FALSE otherwise.
 * @see elm_obj_gesture_layer_hold_events_get()
 */
EAPI Eina_Bool
elm_gesture_layer_hold_events_get(const Elm_Gesture_Layer *obj)
{
   return elm_obj_gesture_layer_hold_events_get(obj);
}

/**
 * @brief Sets the rotate step for the gesture layer.
 * @param obj The gesture layer object.
 * @param step The rotate step value.
 * @see elm_obj_gesture_layer_rotate_step_set()
 */
EAPI void
elm_gesture_layer_rotate_step_set(Elm_Gesture_Layer *obj, double step)
{
   elm_obj_gesture_layer_rotate_step_set(obj, step);
}

/**
 * @brief Gets the rotate step for the gesture layer.
 * @param obj The gesture layer object.
 * @return The rotate step value.
 * @see elm_obj_gesture_layer_rotate_step_get()
 */
EAPI double
elm_gesture_layer_rotate_step_get(const Elm_Gesture_Layer *obj)
{
   return elm_obj_gesture_layer_rotate_step_get(obj);
}

/**
 * @brief Sets a gesture callback for a specific gesture type and state.
 * @param obj The gesture layer object.
 * @param idx The gesture type.
 * @param cb_type The gesture state.
 * @param cb The callback function.
 * @param data User data to be passed to the callback.
 * @see elm_obj_gesture_layer_cb_set()
 */
EAPI void
elm_gesture_layer_cb_set(Elm_Gesture_Layer *obj, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data)
{
   elm_obj_gesture_layer_cb_set(obj, idx, cb_type, cb, data);
}

/**
 * @brief Attaches the gesture layer to a target Evas object.
 * @param obj The gesture layer object.
 * @param target The Evas object to attach to.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_gesture_layer_attach()
 */
EAPI Eina_Bool
elm_gesture_layer_attach(Elm_Gesture_Layer *obj, Efl_Canvas_Object *target)
{
   return elm_obj_gesture_layer_attach(obj, target);
}

/**
 * @brief Deletes a gesture callback.
 * @param obj The gesture layer object.
 * @param idx The gesture type.
 * @param cb_type The gesture state.
 * @param cb The callback function to remove.
 * @param data The user data associated with the callback.
 * @see elm_obj_gesture_layer_cb_del()
 */
EAPI void
elm_gesture_layer_cb_del(Elm_Gesture_Layer *obj, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data)
{
   elm_obj_gesture_layer_cb_del(obj, idx, cb_type, cb, data);
}

/**
 * @brief Adds a gesture callback for a specific gesture type and state.
 * @param obj The gesture layer object.
 * @param idx The gesture type.
 * @param cb_type The gesture state.
 * @param cb The callback function.
 * @param data User data to be passed to the callback.
 * @see elm_obj_gesture_layer_cb_add()
 */
EAPI void
elm_gesture_layer_cb_add(Elm_Gesture_Layer *obj, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data)
{
   elm_obj_gesture_layer_cb_add(obj, idx, cb_type, cb, data);
}
