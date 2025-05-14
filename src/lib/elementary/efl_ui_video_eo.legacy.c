/**
 * @brief Implements the legacy function elm_video_remember_position_set.
 * @details This is the legacy C API wrapper for efl_ui_video_remember_position_set().
 * Refer to the function declaration in the corresponding .h file for full documentation.
 */
EAPI void
elm_video_remember_position_set(Efl_Ui_Video *obj, Eina_Bool remember)
{
   efl_ui_video_remember_position_set(obj, remember);
}
/**
 * @brief Implements the legacy function elm_video_remember_position_get.
 * @details This is the legacy C API wrapper for efl_ui_video_remember_position_get().
 * Refer to the function declaration in the corresponding .h file for full documentation.
 */
EAPI Eina_Bool
elm_video_remember_position_get(const Efl_Ui_Video *obj)
{
   return efl_ui_video_remember_position_get(obj);
}
/**
 * @brief Implements the legacy function elm_video_emotion_get.
 * @details This is the legacy C API wrapper for efl_ui_video_emotion_get().
 * Refer to the function declaration in the corresponding .h file for full documentation.
 */
EAPI Efl_Canvas_Object *
elm_video_emotion_get(const Efl_Ui_Video *obj)
{
   return efl_ui_video_emotion_get(obj);
}
/**
 * @brief Implements the legacy function elm_video_title_get.
 * @details This is the legacy C API wrapper for efl_ui_video_title_get().
 * Refer to the function declaration in the corresponding .h file for full documentation.
 */
EAPI const char *
elm_video_title_get(const Efl_Ui_Video *obj)
{
   return efl_ui_video_title_get(obj);
}
