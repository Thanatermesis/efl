/**
 * @brief Sets the resize object for the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] sobj A canvas object (often a @ref Efl_Canvas_Layout object).
 * @see efl_ui_widget_resize_object_set() for the Efl Core API.
 */
EAPI void
elm_widget_resize_object_set(Efl_Ui_Widget *obj, Efl_Canvas_Object *sobj)
{
   efl_ui_widget_resize_object_set(obj, sobj);
}

/**
 * @brief Enables or disables the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] disabled @c EINA_TRUE to disable, @c EINA_FALSE to enable.
 * @see efl_ui_widget_disabled_set() for the Efl Core API.
 */
EAPI void
elm_widget_disabled_set(Efl_Ui_Widget *obj, Eina_Bool disabled)
{
   efl_ui_widget_disabled_set(obj, disabled);
}

/**
 * @brief Gets whether the widget is disabled. Legacy API.
 * @param[in] obj The object.
 * @return @c EINA_TRUE if disabled, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_disabled_get() for the Efl Core API.
 */
EAPI Eina_Bool
elm_widget_disabled_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_disabled_get(obj);
}

/**
 * @brief Sets the style for the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] style The name of the style to use.
 * @return Eina_Error indicating success or failure.
 * @see efl_ui_widget_style_set() for the Efl Core API.
 */
EAPI Eina_Error
elm_widget_style_set(Efl_Ui_Widget *obj, const char *style)
{
   return efl_ui_widget_style_set(obj, style);
}

/**
 * @brief Gets the style of the widget. Legacy API.
 * @param[in] obj The object.
 * @return The name of the style in use.
 * @see efl_ui_widget_style_get() for the Efl Core API.
 */
EAPI const char *
elm_widget_style_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_style_get(obj);
}

/**
 * @brief Sets whether the widget can be focused. Legacy API.
 * @param[in] obj The object.
 * @param[in] can_focus @c EINA_TRUE if the object is focusable, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_focus_allow_set() for the Efl Core API.
 */
EAPI void
elm_widget_can_focus_set(Efl_Ui_Widget *obj, Eina_Bool can_focus)
{
   efl_ui_widget_focus_allow_set(obj, can_focus);
}

/**
 * @brief Gets whether the widget can be focused. Legacy API.
 * @param[in] obj The object.
 * @return @c EINA_TRUE if the object is focusable, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_focus_allow_get() for the Efl Core API.
 */
EAPI Eina_Bool
elm_widget_can_focus_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_focus_allow_get(obj);
}

/**
 * @brief Sets the parent of the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] parent The widget parent object.
 * @see efl_ui_widget_parent_set() for the Efl Core API.
 */
EAPI void
elm_widget_parent_set(Efl_Ui_Widget *obj, Efl_Ui_Widget *parent)
{
   efl_ui_widget_parent_set(obj, parent);
}

/**
 * @brief Gets the parent of the widget. Legacy API.
 * @param[in] obj The object.
 * @return The widget parent object.
 * @see efl_ui_widget_parent_get() for the Efl Core API.
 */
EAPI Efl_Ui_Widget *
elm_widget_parent_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_parent_get(obj);
}

/**
 * @brief Adds a sub-object to the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] sub_obj The sub-object to add.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_sub_object_add() for the Efl Core API.
 */
EAPI Eina_Bool
elm_widget_sub_object_add(Efl_Ui_Widget *obj, Efl_Canvas_Object *sub_obj)
{
   return efl_ui_widget_sub_object_add(obj, sub_obj);
}

/**
 * @brief Deletes a sub-object from the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] sub_obj The sub-object to delete.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_sub_object_del() for the Efl Core API.
 */
EAPI Eina_Bool
elm_widget_sub_object_del(Efl_Ui_Widget *obj, Efl_Canvas_Object *sub_obj)
{
   return efl_ui_widget_sub_object_del(obj, sub_obj);
}

/**
 * @brief Applies the theme to the widget. Legacy API.
 * @param[in] obj The object.
 * @return Eina_Error indicating success or failure.
 * @see efl_ui_widget_theme_apply() for the Efl Core API.
 */
EAPI Eina_Error
elm_widget_theme_apply(Efl_Ui_Widget *obj)
{
   return efl_ui_widget_theme_apply(obj);
}

/**
 * @brief Gets the focus region of the widget. Legacy API.
 * @param[in] obj The object.
 * @return The relative region to show.
 * @see efl_ui_widget_interest_region_get() for the Efl Core API.
 */
EAPI Eina_Rect
elm_widget_focus_region_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_interest_region_get(obj);
}

/**
 * @brief Gets the focus highlight geometry for the widget. Legacy API.
 * @param[in] obj The object.
 * @return The rectangle area for focus highlight.
 * @see efl_ui_widget_focus_highlight_geometry_get() for the Efl Core API.
 */
EAPI Eina_Rect
elm_widget_focus_highlight_geometry_get(const Efl_Ui_Widget *obj)
{
   return efl_ui_widget_focus_highlight_geometry_get(obj);
}

/**
 * @brief Applies the focus state to the widget. Legacy API.
 * @param[in] obj The object.
 * @param[in] current_state The current focus state.
 * @param[in,out] configured_state The configured focus state to be applied.
 * @param[in] redirect The redirect object.
 * @return @c EINA_TRUE if the widget is registered, @c EINA_FALSE otherwise.
 * @see efl_ui_widget_focus_state_apply() for the Efl Core API.
 */
EAPI Eina_Bool
elm_widget_focus_state_apply(Efl_Ui_Widget *obj, Efl_Ui_Widget_Focus_State current_state, Efl_Ui_Widget_Focus_State *configured_state, Efl_Ui_Widget *redirect)
{
   return efl_ui_widget_focus_state_apply(obj, current_state, configured_state, redirect);
}
