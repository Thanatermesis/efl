/**
 * @brief Legacy wrapper for efl_ui_image_zoomable_gesture_enabled_set().
 * @deprecated Use efl_ui_image_zoomable_gesture_enabled_set() instead.
 */
EAPI void
elm_photocam_gesture_enabled_set(Efl_Ui_Image_Zoomable *obj, Eina_Bool gesture)
{
   efl_ui_image_zoomable_gesture_enabled_set(obj, gesture);
}

/**
 * @brief Legacy wrapper for efl_ui_image_zoomable_gesture_enabled_get().
 * @deprecated Use efl_ui_image_zoomable_gesture_enabled_get() instead.
 */
EAPI Eina_Bool
elm_photocam_gesture_enabled_get(const Efl_Ui_Image_Zoomable *obj)
{
   return efl_ui_image_zoomable_gesture_enabled_get(obj);
}
