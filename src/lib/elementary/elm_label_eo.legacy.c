/**
 * @brief Control wrap width of the label.
 * @param[in] obj The object.
 * @param[in] w The wrap width in pixels.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_wrap_width_set(Elm_Label *obj, int w)
{
   elm_obj_label_wrap_width_set(obj, w);
}

/**
 * @brief Get the wrap width of the label.
 * @param[in] obj The object.
 * @return The wrap width in pixels.
 * @ingroup Elm_Label_Group
 */
EAPI int
elm_label_wrap_width_get(const Elm_Label *obj)
{
   return elm_obj_label_wrap_width_get(obj);
}

/**
 * @brief Control the slide speed of the label.
 * @param[in] obj The object.
 * @param[in] speed The speed of the slide animation in px per second.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_slide_speed_set(Elm_Label *obj, double speed)
{
   elm_obj_label_slide_speed_set(obj, speed);
}

/**
 * @brief Get the slide speed of the label.
 * @param[in] obj The object.
 * @return The speed of the slide animation in px per second.
 * @ingroup Elm_Label_Group
 */
EAPI double
elm_label_slide_speed_get(const Elm_Label *obj)
{
   return elm_obj_label_slide_speed_get(obj);
}

/**
 * @brief Control the slide mode of the label widget.
 * @param[in] obj The object.
 * @param[in] mode The slide mode.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_slide_mode_set(Elm_Label *obj, Elm_Label_Slide_Mode mode)
{
   elm_obj_label_slide_mode_set(obj, mode);
}

/**
 * @brief Get the slide mode of the label widget.
 * @param[in] obj The object.
 * @return The slide mode.
 * @ingroup Elm_Label_Group
 */
EAPI Elm_Label_Slide_Mode
elm_label_slide_mode_get(const Elm_Label *obj)
{
   return elm_obj_label_slide_mode_get(obj);
}

/**
 * @brief Control the slide duration of the label.
 * @param[in] obj The object.
 * @param[in] duration The duration in seconds.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_slide_duration_set(Elm_Label *obj, double duration)
{
   elm_obj_label_slide_duration_set(obj, duration);
}

/**
 * @brief Get the slide duration of the label.
 * @param[in] obj The object.
 * @return The duration in seconds.
 * @ingroup Elm_Label_Group
 */
EAPI double
elm_label_slide_duration_get(const Elm_Label *obj)
{
   return elm_obj_label_slide_duration_get(obj);
}

/**
 * @brief Control the wrapping behavior of the label.
 * @param[in] obj The object.
 * @param[in] wrap The wrap type.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_line_wrap_set(Elm_Label *obj, Elm_Wrap_Type wrap)
{
   elm_obj_label_line_wrap_set(obj, wrap);
}

/**
 * @brief Get the wrapping behavior of the label.
 * @param[in] obj The object.
 * @return The wrap type.
 * @ingroup Elm_Label_Group
 */
EAPI Elm_Wrap_Type
elm_label_line_wrap_get(const Elm_Label *obj)
{
   return elm_obj_label_line_wrap_get(obj);
}

/**
 * @brief Control the ellipsis behavior of the label.
 * @param[in] obj The object.
 * @param[in] ellipsis EINA_TRUE to enable ellipsis, EINA_FALSE to disable.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_ellipsis_set(Elm_Label *obj, Eina_Bool ellipsis)
{
   elm_obj_label_ellipsis_set(obj, ellipsis);
}

/**
 * @brief Get the ellipsis behavior of the label.
 * @param[in] obj The object.
 * @return EINA_TRUE if ellipsis is enabled, EINA_FALSE otherwise.
 * @ingroup Elm_Label_Group
 */
EAPI Eina_Bool
elm_label_ellipsis_get(const Elm_Label *obj)
{
   return elm_obj_label_ellipsis_get(obj);
}

/**
 * @brief Start slide effect.
 * @param[in] obj The object.
 * @ingroup Elm_Label_Group
 */
EAPI void
elm_label_slide_go(Elm_Label *obj)
{
   elm_obj_label_slide_go(obj);
}
