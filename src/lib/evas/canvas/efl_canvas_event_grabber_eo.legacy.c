/**
 * @internal
 * @brief Implements the legacy Evas function evas_object_event_grabber_freeze_when_visible_set.
 *
 * This function is a wrapper around the EFL call efl_canvas_event_grabber_freeze_when_visible_set.
 * Refer to the declaration in efl_canvas_event_grabber_eo.legacy.h for detailed API documentation.
 */
EVAS_API void
evas_object_event_grabber_freeze_when_visible_set(Efl_Canvas_Event_Grabber *obj, Eina_Bool set)
{
   efl_canvas_event_grabber_freeze_when_visible_set(obj, set);
}

/**
 * @internal
 * @brief Implements the legacy Evas function evas_object_event_grabber_freeze_when_visible_get.
 *
 * This function is a wrapper around the EFL call efl_canvas_event_grabber_freeze_when_visible_get.
 * Refer to the declaration in efl_canvas_event_grabber_eo.legacy.h for detailed API documentation.
 */
EVAS_API Eina_Bool
evas_object_event_grabber_freeze_when_visible_get(const Efl_Canvas_Event_Grabber *obj)
{
   return efl_canvas_event_grabber_freeze_when_visible_get(obj);
}
