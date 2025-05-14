/**
 * @internal
 * @brief Set the id of the Status Notifier Item.
 * @deprecated Use efl_key_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_id_set().
 *
 * @param obj The systray object.
 * @param id The ID to set.
 */
EAPI void
elm_systray_id_set(Elm_Systray *obj, const char *id)
{
   elm_obj_systray_id_set(obj, id);
}

/**
 * @internal
 * @brief Get the id of the Status Notifier Item.
 * @deprecated Use efl_key_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_id_get().
 *
 * @param obj The systray object.
 * @return The ID of the item.
 */
EAPI const char *
elm_systray_id_get(const Elm_Systray *obj)
{
   return elm_obj_systray_id_get(obj);
}

/**
 * @internal
 * @brief Set the category of the Status Notifier Item.
 * @deprecated Use efl_ui_systray_category_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_category_set().
 *
 * @param obj The systray object.
 * @param cat The category to set.
 */
EAPI void
elm_systray_category_set(Elm_Systray *obj, Elm_Systray_Category cat)
{
   elm_obj_systray_category_set(obj, cat);
}

/**
 * @internal
 * @brief Get the category of the Status Notifier Item.
 * @deprecated Use efl_ui_systray_category_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_category_get().
 *
 * @param obj The systray object.
 * @return The category of the item.
 */
EAPI Elm_Systray_Category
elm_systray_category_get(const Elm_Systray *obj)
{
   return elm_obj_systray_category_get(obj);
}

/**
 * @internal
 * @brief Set the path to the theme for icons.
 * @deprecated Use efl_ui_systray_icon_theme_path_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_icon_theme_path_set().
 *
 * @param obj The systray object.
 * @param icon_theme_path The icon theme path.
 */
EAPI void
elm_systray_icon_theme_path_set(Elm_Systray *obj, const char *icon_theme_path)
{
   elm_obj_systray_icon_theme_path_set(obj, icon_theme_path);
}

/**
 * @internal
 * @brief Get the path to the icon's theme.
 * @deprecated Use efl_ui_systray_icon_theme_path_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_icon_theme_path_get().
 *
 * @param obj The systray object.
 * @return The icon theme path.
 */
EAPI const char *
elm_systray_icon_theme_path_get(const Elm_Systray *obj)
{
   return elm_obj_systray_icon_theme_path_get(obj);
}

/**
 * @internal
 * @brief Set the D-Bus Menu object path.
 * @deprecated Use efl_ui_systray_menu_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_menu_set().
 *
 * @param obj The systray object.
 * @param menu The D-Bus menu object.
 */
EAPI void
elm_systray_menu_set(Elm_Systray *obj, const Efl_Object *menu)
{
   elm_obj_systray_menu_set(obj, menu);
}

/**
 * @internal
 * @brief Get the D-Bus Menu object path.
 * @deprecated Use efl_ui_systray_menu_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_menu_get().
 *
 * @param obj The systray object.
 * @return The D-Bus menu object.
 */
EAPI const Efl_Object *
elm_systray_menu_get(const Elm_Systray *obj)
{
   return elm_obj_systray_menu_get(obj);
}

/**
 * @internal
 * @brief Set the name of the attention icon.
 * @deprecated Use efl_ui_systray_attention_icon_name_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_att_icon_name_set().
 *
 * @param obj The systray object.
 * @param att_icon_name The attention icon name.
 */
EAPI void
elm_systray_att_icon_name_set(Elm_Systray *obj, const char *att_icon_name)
{
   elm_obj_systray_att_icon_name_set(obj, att_icon_name);
}

/**
 * @internal
 * @brief Get the name of the attention icon.
 * @deprecated Use efl_ui_systray_attention_icon_name_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_att_icon_name_get().
 *
 * @param obj The systray object.
 * @return The attention icon name.
 */
EAPI const char *
elm_systray_att_icon_name_get(const Elm_Systray *obj)
{
   return elm_obj_systray_att_icon_name_get(obj);
}

/**
 * @internal
 * @brief Set the status of the Status Notifier Item.
 * @deprecated Use efl_ui_systray_status_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_status_set().
 *
 * @param obj The systray object.
 * @param st The status to set.
 */
EAPI void
elm_systray_status_set(Elm_Systray *obj, Elm_Systray_Status st)
{
   elm_obj_systray_status_set(obj, st);
}

/**
 * @internal
 * @brief Get the status of the Status Notifier Item.
 * @deprecated Use efl_ui_systray_status_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_status_get().
 *
 * @param obj The systray object.
 * @return The status of the item.
 */
EAPI Elm_Systray_Status
elm_systray_status_get(const Elm_Systray *obj)
{
   return elm_obj_systray_status_get(obj);
}

/**
 * @internal
 * @brief Set the name of the icon.
 * @deprecated Use efl_ui_icon_icon_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_icon_name_set().
 *
 * @param obj The systray object.
 * @param icon_name The icon name.
 */
EAPI void
elm_systray_icon_name_set(Elm_Systray *obj, const char *icon_name)
{
   elm_obj_systray_icon_name_set(obj, icon_name);
}

/**
 * @internal
 * @brief Get the name of the icon.
 * @deprecated Use efl_ui_icon_icon_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_icon_name_get().
 *
 * @param obj The systray object.
 * @return The icon name.
 */
EAPI const char *
elm_systray_icon_name_get(const Elm_Systray *obj)
{
   return elm_obj_systray_icon_name_get(obj);
}

/**
 * @internal
 * @brief Set the title of the Status Notifier Item.
 * @deprecated Use efl_text_set() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_title_set().
 *
 * @param obj The systray object.
 * @param title The title to set.
 */
EAPI void
elm_systray_title_set(Elm_Systray *obj, const char *title)
{
   elm_obj_systray_title_set(obj, title);
}

/**
 * @internal
 * @brief Get the title of the Status Notifier Item.
 * @deprecated Use efl_text_get() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_title_get().
 *
 * @param obj The systray object.
 * @return The title of the item.
 */
EAPI const char *
elm_systray_title_get(const Elm_Systray *obj)
{
   return elm_obj_systray_title_get(obj);
}

/**
 * @internal
 * @brief Register this Status Notifier Item.
 * @deprecated Use efl_ui_systray_register() instead.
 *
 * This is a legacy function. It calls the Eo method elm_obj_systray_register().
 *
 * @param obj The systray object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_systray_register(Elm_Systray *obj)
{
   return elm_obj_systray_register(obj);
}
