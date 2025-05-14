
/**
 * @brief Legacy wrapper for efl_canvas_video_option_set.
 * @deprecated Use efl_canvas_video_option_set() instead.
 *
 * @param obj The Efl_Canvas_Video object.
 * @param opt The option to set (e.g., "video", "audio").
 * @param val The value for the option (e.g., "off").
 */
EMOTION_API void
emotion_object_module_option_set(Efl_Canvas_Video *obj, const char *opt, const char *val)
{
   efl_canvas_video_option_set(obj, opt, val);
}

/**
 * @brief Legacy wrapper for efl_canvas_video_engine_set.
 * @deprecated Use efl_canvas_video_engine_set() instead.
 *
 * @param obj The Efl_Canvas_Video object.
 * @param module_filename The name of the video engine module to use (e.g., "gstreamer", "xine").
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EMOTION_API Eina_Bool
emotion_object_init(Efl_Canvas_Video *obj, const char *module_filename)
{
   return efl_canvas_video_engine_set(obj, module_filename);
}
