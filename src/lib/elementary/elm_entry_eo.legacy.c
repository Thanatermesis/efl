
/**
 * @brief Legacy EAPI wrapper to enable or disable scrolling in an entry.
 * @param[in] obj The entry object.
 * @param[in] scroll @c EINA_TRUE if it is to be scrollable, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_scrollable_set().
 *          See elm_entry_scrollable_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_scrollable_set(Elm_Entry *obj, Eina_Bool scroll)
{
   elm_obj_entry_scrollable_set(obj, scroll);
}

/**
 * @brief Legacy EAPI wrapper to get the scrollable state of an entry.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE if scrollable, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_scrollable_get().
 *          See elm_entry_scrollable_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_scrollable_get(const Elm_Entry *obj)
{
   return elm_obj_entry_scrollable_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set showing the input panel on user's explicit event.
 * @param[in] obj The entry object.
 * @param[in] ondemand If @c EINA_TRUE, the input panel will be shown only on a Mouse Up event.
 * @details This function calls elm_obj_entry_input_panel_show_on_demand_set().
 *          See elm_entry_input_panel_show_on_demand_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.9
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_show_on_demand_set(Elm_Entry *obj, Eina_Bool ondemand)
{
   elm_obj_entry_input_panel_show_on_demand_set(obj, ondemand);
}

/**
 * @brief Legacy EAPI wrapper to get showing the input panel on user's explicit event.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, the input panel will be shown only on a Mouse Up event.
 * @details This function calls elm_obj_entry_input_panel_show_on_demand_get().
 *          See elm_entry_input_panel_show_on_demand_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.9
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_input_panel_show_on_demand_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_show_on_demand_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to disable the entry's contextual (longpress) menu.
 * @param[in] obj The entry object.
 * @param[in] disabled If @c EINA_TRUE, the menu is disabled.
 * @details This function calls elm_obj_entry_context_menu_disabled_set().
 *          See elm_entry_context_menu_disabled_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_context_menu_disabled_set(Elm_Entry *obj, Eina_Bool disabled)
{
   elm_obj_entry_context_menu_disabled_set(obj, disabled);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry's contextual (longpress) menu is disabled.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, the menu is disabled.
 * @details This function calls elm_obj_entry_context_menu_disabled_get().
 *          See elm_entry_context_menu_disabled_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_context_menu_disabled_get(const Elm_Entry *obj)
{
   return elm_obj_entry_context_menu_disabled_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to control pasting of text and images for the widget.
 * @param[in] obj The entry object.
 * @param[in] cnp_mode One of #Elm_Cnp_Mode.
 * @details This function calls elm_obj_entry_cnp_mode_set().
 *          See elm_entry_cnp_mode_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cnp_mode_set(Elm_Entry *obj, Elm_Cnp_Mode cnp_mode)
{
   elm_obj_entry_cnp_mode_set(obj, cnp_mode);
}

/**
 * @brief Legacy EAPI wrapper for getting elm_entry text paste/drop mode.
 * @param[in] obj The entry object.
 * @return One of #Elm_Cnp_Mode.
 * @details This function calls elm_obj_entry_cnp_mode_get().
 *          See elm_entry_cnp_mode_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Cnp_Mode
elm_entry_cnp_mode_get(const Elm_Entry *obj)
{
   return elm_obj_entry_cnp_mode_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the text format used to load and save the file.
 * @param[in] obj The entry object.
 * @param[in] format The file format (#Elm_Text_Format).
 * @details This function calls elm_obj_entry_file_text_format_set().
 *          See elm_entry_file_text_format_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_file_text_format_set(Elm_Entry *obj, Elm_Text_Format format)
{
   elm_obj_entry_file_text_format_set(obj, format);
}

/**
 * @brief Legacy EAPI wrapper to set the language mode of the input panel.
 * @param[in] obj The entry object.
 * @param[in] lang Language to be set to the input panel (#Elm_Input_Panel_Lang).
 * @details This function calls elm_obj_entry_input_panel_language_set().
 *          See elm_entry_input_panel_language_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_language_set(Elm_Entry *obj, Elm_Input_Panel_Lang lang)
{
   elm_obj_entry_input_panel_language_set(obj, lang);
}

/**
 * @brief Legacy EAPI wrapper to get the language mode of the input panel.
 * @param[in] obj The entry object.
 * @return Language to be set to the input panel (#Elm_Input_Panel_Lang).
 * @details This function calls elm_obj_entry_input_panel_language_get().
 *          See elm_entry_input_panel_language_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Input_Panel_Lang
elm_entry_input_panel_language_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_language_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to disable the entry's selection handlers.
 * @param[in] obj The entry object.
 * @param[in] disabled If @c EINA_TRUE, the selection handlers are disabled.
 * @details This function calls elm_obj_entry_selection_handler_disabled_set().
 *          See elm_entry_selection_handler_disabled_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_selection_handler_disabled_set(Elm_Entry *obj, Eina_Bool disabled)
{
   elm_obj_entry_selection_handler_disabled_set(obj, disabled);
}

/**
 * @brief Legacy EAPI wrapper to set the input panel layout variation of the entry.
 * @param[in] obj The entry object.
 * @param[in] variation Layout variation type.
 * @details This function calls elm_obj_entry_input_panel_layout_variation_set().
 *          See elm_entry_input_panel_layout_variation_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.8
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_layout_variation_set(Elm_Entry *obj, int variation)
{
   elm_obj_entry_input_panel_layout_variation_set(obj, variation);
}

/**
 * @brief Legacy EAPI wrapper to get the input panel layout variation of the entry.
 * @param[in] obj The entry object.
 * @return Layout variation type.
 * @details This function calls elm_obj_entry_input_panel_layout_variation_get().
 *          See elm_entry_input_panel_layout_variation_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.8
 * @ingroup Elm_Entry_Group
 */
EAPI int
elm_entry_input_panel_layout_variation_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_layout_variation_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the autocapitalization type on the immodule.
 * @param[in] obj The entry object.
 * @param[in] autocapital_type The type of autocapitalization (#Elm_Autocapital_Type).
 * @details This function calls elm_obj_entry_autocapital_type_set().
 *          See elm_entry_autocapital_type_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_autocapital_type_set(Elm_Entry *obj, Elm_Autocapital_Type autocapital_type)
{
   elm_obj_entry_autocapital_type_set(obj, autocapital_type);
}

/**
 * @brief Legacy EAPI wrapper to get the autocapitalization type on the immodule.
 * @param[in] obj The entry object.
 * @return The type of autocapitalization (#Elm_Autocapital_Type).
 * @details This function calls elm_obj_entry_autocapital_type_get().
 *          See elm_entry_autocapital_type_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Autocapital_Type
elm_entry_autocapital_type_get(const Elm_Entry *obj)
{
   return elm_obj_entry_autocapital_type_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set if the entry is to be editable or not.
 * @param[in] obj The entry object.
 * @param[in] editable If @c EINA_TRUE, user input will be inserted; if @c EINA_FALSE, read-only.
 * @details This function calls elm_obj_entry_editable_set().
 *          See elm_entry_editable_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_editable_set(Elm_Entry *obj, Eina_Bool editable)
{
   elm_obj_entry_editable_set(obj, editable);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry is editable or not.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, user input will be inserted; if @c EINA_FALSE, read-only.
 * @details This function calls elm_obj_entry_editable_get().
 *          See elm_entry_editable_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_editable_get(const Elm_Entry *obj)
{
   return elm_obj_entry_editable_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the style that the hover should use.
 * @param[in] obj The entry object.
 * @param[in] style The style to use for the underlying hover. NULL to disable.
 * @details This function calls elm_obj_entry_anchor_hover_style_set().
 *          See elm_entry_anchor_hover_style_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_anchor_hover_style_set(Elm_Entry *obj, const char *style)
{
   elm_obj_entry_anchor_hover_style_set(obj, style);
}

/**
 * @brief Legacy EAPI wrapper to get the style that the hover should use.
 * @param[in] obj The entry object.
 * @return The style to use for the underlying hover.
 * @details This function calls elm_obj_entry_anchor_hover_style_get().
 *          See elm_entry_anchor_hover_style_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI const char *
elm_entry_anchor_hover_style_get(const Elm_Entry *obj)
{
   return elm_obj_entry_anchor_hover_style_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the entry to single line mode.
 * @param[in] obj The entry object.
 * @param[in] single_line If @c EINA_TRUE, the text in the entry will be on a single line.
 * @details This function calls elm_obj_entry_single_line_set().
 *          See elm_entry_single_line_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_single_line_set(Elm_Entry *obj, Eina_Bool single_line)
{
   elm_obj_entry_single_line_set(obj, single_line);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry is set to be single line.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, the text in the entry will be on a single line.
 * @details This function calls elm_obj_entry_single_line_get().
 *          See elm_entry_single_line_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_single_line_get(const Elm_Entry *obj)
{
   return elm_obj_entry_single_line_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the entry to password mode.
 * @param[in] obj The entry object.
 * @param[in] password If @c EINA_TRUE, password mode is enabled.
 * @details This function calls elm_obj_entry_password_set().
 *          See elm_entry_password_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_password_set(Elm_Entry *obj, Eina_Bool password)
{
   elm_obj_entry_password_set(obj, password);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry is set to password mode.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, password mode is enabled.
 * @details This function calls elm_obj_entry_password_get().
 *          See elm_entry_password_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_password_get(const Elm_Entry *obj)
{
   return elm_obj_entry_password_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the return key on the input panel to be disabled.
 * @param[in] obj The entry object.
 * @param[in] disabled The state to put in in: @c EINA_TRUE for disabled, @c EINA_FALSE for enabled.
 * @details This function calls elm_obj_entry_input_panel_return_key_disabled_set().
 *          See elm_entry_input_panel_return_key_disabled_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_return_key_disabled_set(Elm_Entry *obj, Eina_Bool disabled)
{
   elm_obj_entry_input_panel_return_key_disabled_set(obj, disabled);
}

/**
 * @brief Legacy EAPI wrapper to get whether the return key on the input panel should be disabled.
 * @param[in] obj The entry object.
 * @return The state: @c EINA_TRUE for disabled, @c EINA_FALSE for enabled.
 * @details This function calls elm_obj_entry_input_panel_return_key_disabled_get().
 *          See elm_entry_input_panel_return_key_disabled_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_input_panel_return_key_disabled_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_return_key_disabled_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the entry object to 'autosave' the loaded text file or not.
 * @param[in] obj The entry object.
 * @param[in] auto_save Autosave the loaded file or not.
 * @details This function calls elm_obj_entry_autosave_set().
 *          See elm_entry_autosave_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_autosave_set(Elm_Entry *obj, Eina_Bool auto_save)
{
   elm_obj_entry_autosave_set(obj, auto_save);
}

/**
 * @brief Legacy EAPI wrapper to get the entry object's 'autosave' status.
 * @param[in] obj The entry object.
 * @return Autosave the loaded file or not.
 * @details This function calls elm_obj_entry_autosave_get().
 *          See elm_entry_autosave_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_autosave_get(const Elm_Entry *obj)
{
   return elm_obj_entry_autosave_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the parent of the hover popup.
 * @param[in] obj The entry object.
 * @param[in] parent The object to use as parent for the hover.
 * @details This function calls elm_obj_entry_anchor_hover_parent_set().
 *          See elm_entry_anchor_hover_parent_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_anchor_hover_parent_set(Elm_Entry *obj, Efl_Canvas_Object *parent)
{
   elm_obj_entry_anchor_hover_parent_set(obj, parent);
}

/**
 * @brief Legacy EAPI wrapper to get the parent of the hover popup.
 * @param[in] obj The entry object.
 * @return The object to use as parent for the hover.
 * @details This function calls elm_obj_entry_anchor_hover_parent_get().
 *          See elm_entry_anchor_hover_parent_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Efl_Canvas_Object *
elm_entry_anchor_hover_parent_get(const Elm_Entry *obj)
{
   return elm_obj_entry_anchor_hover_parent_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set whether the entry should allow to use text prediction.
 * @param[in] obj The entry object.
 * @param[in] prediction Whether the entry should allow to use text prediction.
 * @details This function calls elm_obj_entry_prediction_allow_set().
 *          See elm_entry_prediction_allow_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_prediction_allow_set(Elm_Entry *obj, Eina_Bool prediction)
{
   elm_obj_entry_prediction_allow_set(obj, prediction);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry should allow to use text prediction.
 * @param[in] obj The entry object.
 * @return Whether the entry should allow to use text prediction.
 * @details This function calls elm_obj_entry_prediction_allow_get().
 *          See elm_entry_prediction_allow_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_prediction_allow_get(const Elm_Entry *obj)
{
   return elm_obj_entry_prediction_allow_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the input hint.
 * @param[in] obj The entry object.
 * @param[in] hints Input hint (#Elm_Input_Hints).
 * @details This function calls elm_obj_entry_input_hint_set().
 *          See elm_entry_input_hint_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_hint_set(Elm_Entry *obj, Elm_Input_Hints hints)
{
   elm_obj_entry_input_hint_set(obj, hints);
}

/**
 * @brief Legacy EAPI wrapper to get the value of input hint.
 * @param[in] obj The entry object.
 * @return Input hint (#Elm_Input_Hints).
 * @details This function calls elm_obj_entry_input_hint_get().
 *          See elm_entry_input_hint_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Input_Hints
elm_entry_input_hint_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_hint_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the input panel layout of the entry.
 * @param[in] obj The entry object.
 * @param[in] layout Layout type (#Elm_Input_Panel_Layout).
 * @details This function calls elm_obj_entry_input_panel_layout_set().
 *          See elm_entry_input_panel_layout_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_layout_set(Elm_Entry *obj, Elm_Input_Panel_Layout layout)
{
   elm_obj_entry_input_panel_layout_set(obj, layout);
}

/**
 * @brief Legacy EAPI wrapper to get the input panel layout of the entry.
 * @param[in] obj The entry object.
 * @return Layout type (#Elm_Input_Panel_Layout).
 * @details This function calls elm_obj_entry_input_panel_layout_get().
 *          See elm_entry_input_panel_layout_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Input_Panel_Layout
elm_entry_input_panel_layout_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_layout_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the "return" key type.
 * @param[in] obj The entry object.
 * @param[in] return_key_type The type of "return" key on the input panel (#Elm_Input_Panel_Return_Key_Type).
 * @details This function calls elm_obj_entry_input_panel_return_key_type_set().
 *          See elm_entry_input_panel_return_key_type_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_return_key_type_set(Elm_Entry *obj, Elm_Input_Panel_Return_Key_Type return_key_type)
{
   elm_obj_entry_input_panel_return_key_type_set(obj, return_key_type);
}

/**
 * @brief Legacy EAPI wrapper to get the "return" key type.
 * @param[in] obj The entry object.
 * @return The type of "return" key on the input panel (#Elm_Input_Panel_Return_Key_Type).
 * @details This function calls elm_obj_entry_input_panel_return_key_type_get().
 *          See elm_entry_input_panel_return_key_type_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Input_Panel_Return_Key_Type
elm_entry_input_panel_return_key_type_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_return_key_type_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the attribute to show the input panel automatically.
 * @param[in] obj The entry object.
 * @param[in] enabled If @c EINA_TRUE, the input panel is appeared when entry is clicked or has a focus.
 * @details This function calls elm_obj_entry_input_panel_enabled_set().
 *          See elm_entry_input_panel_enabled_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_enabled_set(Elm_Entry *obj, Eina_Bool enabled)
{
   elm_obj_entry_input_panel_enabled_set(obj, enabled);
}

/**
 * @brief Legacy EAPI wrapper to get the attribute to show the input panel automatically.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, the input panel is appeared when entry is clicked or has a focus.
 * @details This function calls elm_obj_entry_input_panel_enabled_get().
 *          See elm_entry_input_panel_enabled_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_input_panel_enabled_get(const Elm_Entry *obj)
{
   return elm_obj_entry_input_panel_enabled_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the line wrap type to use on multi-line entries.
 * @param[in] obj The entry object.
 * @param[in] wrap The wrap mode to use (#Elm_Wrap_Type).
 * @details This function calls elm_obj_entry_line_wrap_set().
 *          See elm_entry_line_wrap_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_line_wrap_set(Elm_Entry *obj, Elm_Wrap_Type wrap)
{
   elm_obj_entry_line_wrap_set(obj, wrap);
}

/**
 * @brief Legacy EAPI wrapper to get the wrap mode the entry was set to use.
 * @param[in] obj The entry object.
 * @return The wrap mode to use (#Elm_Wrap_Type).
 * @details This function calls elm_obj_entry_line_wrap_get().
 *          See elm_entry_line_wrap_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Elm_Wrap_Type
elm_entry_line_wrap_get(const Elm_Entry *obj)
{
   return elm_obj_entry_line_wrap_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the cursor position in the entry.
 * @param[in] obj The entry object.
 * @param[in] pos The position of the cursor.
 * @details This function calls elm_obj_entry_cursor_pos_set().
 *          See elm_entry_cursor_pos_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_pos_set(Elm_Entry *obj, int pos)
{
   elm_obj_entry_cursor_pos_set(obj, pos);
}

/**
 * @brief Legacy EAPI wrapper to get the current position of the cursor in the entry.
 * @param[in] obj The entry object.
 * @return The position of the cursor.
 * @details This function calls elm_obj_entry_cursor_pos_get().
 *          See elm_entry_cursor_pos_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI int
elm_entry_cursor_pos_get(const Elm_Entry *obj)
{
   return elm_obj_entry_cursor_pos_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to set the visibility of the left-side widget of the entry.
 * @param[in] obj The entry object.
 * @param[in] setting @c EINA_TRUE if the object should be displayed, @c EINA_FALSE if not.
 * @details This function calls elm_obj_entry_icon_visible_set().
 *          See elm_entry_icon_visible_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_icon_visible_set(Elm_Entry *obj, Eina_Bool setting)
{
   elm_obj_entry_icon_visible_set(obj, setting);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor to the end of the current line.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_line_end_set().
 *          See elm_entry_cursor_line_end_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_line_end_set(Elm_Entry *obj)
{
   elm_obj_entry_cursor_line_end_set(obj);
}

/**
 * @brief Legacy EAPI wrapper to select a region of text within the entry.
 * @param[in] obj The entry object.
 * @param[in] start The starting position.
 * @param[in] end The end position.
 * @details This function calls elm_obj_entry_select_region_set().
 *          See elm_entry_select_region_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.9
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_select_region_set(Elm_Entry *obj, int start, int end)
{
   elm_obj_entry_select_region_set(obj, start, end);
}

/**
 * @brief Legacy EAPI wrapper to get the current position of the selection cursors in the entry.
 * @param[in] obj The entry object.
 * @param[out] start The starting position.
 * @param[out] end The end position.
 * @details This function calls elm_obj_entry_select_region_get().
 *          See elm_entry_select_region_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.18
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_select_region_get(const Elm_Entry *obj, int *start, int *end)
{
   elm_obj_entry_select_region_get(obj, start, end);
}

/**
 * @brief Legacy EAPI wrapper to set whether the return key on the input panel is disabled automatically.
 * @param[in] obj The entry object.
 * @param[in] enabled If @c EINA_TRUE, the return key is automatically disabled when the entry has no text.
 * @details This function calls elm_obj_entry_input_panel_return_key_autoenabled_set().
 *          See elm_entry_input_panel_return_key_autoenabled_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_return_key_autoenabled_set(Elm_Entry *obj, Eina_Bool enabled)
{
   elm_obj_entry_input_panel_return_key_autoenabled_set(obj, enabled);
}

/**
 * @brief Legacy EAPI wrapper to set the visibility of the end widget of the entry.
 * @param[in] obj The entry object.
 * @param[in] setting @c EINA_TRUE if the object should be displayed, @c EINA_FALSE if not.
 * @details This function calls elm_obj_entry_end_visible_set().
 *          See elm_entry_end_visible_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_end_visible_set(Elm_Entry *obj, Eina_Bool setting)
{
   elm_obj_entry_end_visible_set(obj, setting);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor to the beginning of the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_begin_set().
 *          See elm_entry_cursor_begin_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_begin_set(Elm_Entry *obj)
{
   elm_obj_entry_cursor_begin_set(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor to the beginning of the current line.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_line_begin_set().
 *          See elm_entry_cursor_line_begin_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_line_begin_set(Elm_Entry *obj)
{
   elm_obj_entry_cursor_line_begin_set(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor to the end of the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_end_set().
 *          See elm_entry_cursor_end_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_end_set(Elm_Entry *obj)
{
   elm_obj_entry_cursor_end_set(obj);
}

/**
 * @brief Legacy EAPI wrapper to return the actual textblock object of the entry.
 * @param[in] obj The entry object.
 * @return Textblock object.
 * @details This function calls elm_obj_entry_textblock_get().
 *          See elm_entry_textblock_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Efl_Canvas_Object *
elm_entry_textblock_get(const Elm_Entry *obj)
{
   return elm_obj_entry_textblock_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to get the geometry of the cursor.
 * @param[in] obj The entry object.
 * @param[out] x X coordinate.
 * @param[out] y Y coordinate.
 * @param[out] w Width.
 * @param[out] h Height.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_textblock_cursor_geometry_get().
 *          See elm_entry_cursor_geometry_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_geometry_get(const Elm_Entry *obj, int *x, int *y, int *w, int *h)
{
   return elm_obj_entry_textblock_cursor_geometry_get(obj, x, y, w, h);
}

/**
 * @brief Legacy EAPI wrapper to return the input method context of the entry.
 * @param[in] obj The entry object.
 * @return Input method context.
 * @details This function calls elm_obj_entry_imf_context_get().
 *          See elm_entry_imf_context_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void *
elm_entry_imf_context_get(const Elm_Entry *obj)
{
   return elm_obj_entry_imf_context_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to get whether a format node exists at the current cursor position.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE if format node exists, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_is_format_get().
 *          See elm_entry_cursor_is_format_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_is_format_get(const Elm_Entry *obj)
{
   return elm_obj_entry_cursor_is_format_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to get the character pointed by the cursor at its current position.
 * @param[in] obj The entry object.
 * @return Character (must be freed by the caller).
 * @details This function calls elm_obj_entry_textblock_cursor_content_get().
 *          See elm_entry_cursor_content_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI char *
elm_entry_cursor_content_get(const Elm_Entry *obj)
{
   return elm_obj_entry_textblock_cursor_content_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to get any selected text within the entry.
 * @param[in] obj The entry object.
 * @return Selected string (internal, do not free or modify).
 * @details This function calls elm_obj_entry_selection_get().
 *          See elm_entry_selection_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI const char *
elm_entry_selection_get(const Elm_Entry *obj)
{
   return elm_obj_entry_selection_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to get if the current cursor position holds a visible format node.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE if position has a visible format, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_is_visible_format_get().
 *          See elm_entry_cursor_is_visible_format_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_is_visible_format_get(const Elm_Entry *obj)
{
   return elm_obj_entry_cursor_is_visible_format_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to allow selection in the entry.
 * @param[in] obj The entry object.
 * @param[in] allow If @c EINA_TRUE, text selection is allowed.
 * @details This function calls elm_obj_entry_select_allow_set().
 *          See elm_entry_select_allow_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.18
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_select_allow_set(Elm_Entry *obj, Eina_Bool allow)
{
   elm_obj_entry_select_allow_set(obj, allow);
}

/**
 * @brief Legacy EAPI wrapper to check if selection is allowed in the entry.
 * @param[in] obj The entry object.
 * @return If @c EINA_TRUE, text selection is allowed.
 * @details This function calls elm_obj_entry_select_allow_get().
 *          See elm_entry_select_allow_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.18
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_select_allow_get(const Elm_Entry *obj)
{
   return elm_obj_entry_select_allow_get(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor one place to the left within the entry.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_prev().
 *          See elm_entry_cursor_prev() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_prev(Elm_Entry *obj)
{
   return elm_obj_entry_cursor_prev(obj);
}

/**
 * @brief Legacy EAPI wrapper to remove the style from the top of user style stack.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_text_style_user_pop().
 *          See elm_entry_text_style_user_pop() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.7
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_text_style_user_pop(Elm_Entry *obj)
{
   elm_obj_entry_text_style_user_pop(obj);
}

/**
 * @brief Legacy EAPI wrapper to prepend a custom item provider to the list for that entry.
 * @param[in] obj The entry object.
 * @param[in] func The function called to provide the item object.
 * @param[in] data The data passed to @p func.
 * @details This function calls elm_obj_entry_item_provider_prepend().
 *          See elm_entry_item_provider_prepend() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_item_provider_prepend(Elm_Entry *obj, Elm_Entry_Item_Provider_Cb func, void *data)
{
   elm_obj_entry_item_provider_prepend(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to show the input panel (virtual keyboard).
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_input_panel_show().
 *          See elm_entry_input_panel_show() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_show(Elm_Entry *obj)
{
   elm_obj_entry_input_panel_show(obj);
}

/**
 * @brief Legacy EAPI wrapper to reset the input method context of the entry if needed.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_imf_context_reset().
 *          See elm_entry_imf_context_reset() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_imf_context_reset(Elm_Entry *obj)
{
   elm_obj_entry_imf_context_reset(obj);
}

/**
 * @brief Legacy EAPI wrapper to end the hover popup in the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_anchor_hover_end().
 *          See elm_entry_anchor_hover_end() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_anchor_hover_end(Elm_Entry *obj)
{
   elm_obj_entry_anchor_hover_end(obj);
}

/**
 * @brief Legacy EAPI wrapper to begin a selection within the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_selection_begin().
 *          See elm_entry_cursor_selection_begin() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_selection_begin(Elm_Entry *obj)
{
   elm_obj_entry_cursor_selection_begin(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor one line down within the entry.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_down().
 *          See elm_entry_cursor_down() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_down(Elm_Entry *obj)
{
   return elm_obj_entry_cursor_down(obj);
}

/**
 * @brief Legacy EAPI wrapper to write any changes made to the file set with elm_entry_file_set().
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_file_save().
 *          See elm_entry_file_save() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_file_save(Elm_Entry *obj)
{
   elm_obj_entry_file_save(obj);
}

/**
 * @brief Legacy EAPI wrapper to execute a "copy" action on the selected text in the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_selection_copy().
 *          See elm_entry_selection_copy() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_selection_copy(Elm_Entry *obj)
{
   elm_obj_entry_selection_copy(obj);
}

/**
 * @brief Legacy EAPI wrapper to push the style to the top of user style stack.
 * @param[in] obj The entry object.
 * @param[in] style The style user to push.
 * @details This function calls elm_obj_entry_text_style_user_push().
 *          See elm_entry_text_style_user_push() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.7
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_text_style_user_push(Elm_Entry *obj, const char *style)
{
   elm_obj_entry_text_style_user_push(obj, style);
}

/**
 * @brief Legacy EAPI wrapper to remove a custom item provider from the list for that entry.
 * @param[in] obj The entry object.
 * @param[in] func The function called to provide the item object.
 * @param[in] data The data passed to @p func.
 * @details This function calls elm_obj_entry_item_provider_remove().
 *          See elm_entry_item_provider_remove() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_item_provider_remove(Elm_Entry *obj, Elm_Entry_Item_Provider_Cb func, void *data)
{
   elm_obj_entry_item_provider_remove(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to get the style on the top of user style stack.
 * @param[in] obj The entry object.
 * @return Style string.
 * @details This function calls elm_obj_entry_text_style_user_peek().
 *          See elm_entry_text_style_user_peek() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.7
 * @ingroup Elm_Entry_Group
 */
EAPI const char *
elm_entry_text_style_user_peek(const Elm_Entry *obj)
{
   return elm_obj_entry_text_style_user_peek(obj);
}

/**
 * @brief Legacy EAPI wrapper to clear and free the items in an entry's contextual menu.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_context_menu_clear().
 *          See elm_entry_context_menu_clear() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_context_menu_clear(Elm_Entry *obj)
{
   elm_obj_entry_context_menu_clear(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor one line up within the entry.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_up().
 *          See elm_entry_cursor_up() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_up(Elm_Entry *obj)
{
   return elm_obj_entry_cursor_up(obj);
}

/**
 * @brief Legacy EAPI wrapper to insert the given text into the entry at the current cursor position.
 * @param[in] obj The entry object.
 * @param[in] entry The text to insert (markup allowed).
 * @details This function calls elm_obj_entry_insert().
 *          See elm_entry_entry_insert() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_entry_insert(Elm_Entry *obj, const char *entry)
{
   elm_obj_entry_insert(obj, entry);
}

/**
 * @brief Legacy EAPI wrapper to set the input panel-specific data.
 * @param[in] obj The entry object.
 * @param[in] data The specific data to be set to the input panel.
 * @param[in] len The length of data, in bytes.
 * @details This function calls elm_obj_entry_input_panel_imdata_set().
 *          See elm_entry_input_panel_imdata_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_imdata_set(Elm_Entry *obj, const void *data, int len)
{
   elm_obj_entry_input_panel_imdata_set(obj, data, len);
}

/**
 * @brief Legacy EAPI wrapper to get the specific data of the current input panel.
 * @param[in] obj The entry object.
 * @param[out] data The specific data to be got from the input panel.
 * @param[out] len The length of data.
 * @details This function calls elm_obj_entry_input_panel_imdata_get().
 *          See elm_entry_input_panel_imdata_get() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_imdata_get(const Elm_Entry *obj, void *data, int *len)
{
   elm_obj_entry_input_panel_imdata_get(obj, data, len);
}

/**
 * @brief Legacy EAPI wrapper to execute a "paste" action in the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_selection_paste().
 *          See elm_entry_selection_paste() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_selection_paste(Elm_Entry *obj)
{
   elm_obj_entry_selection_paste(obj);
}

/**
 * @brief Legacy EAPI wrapper to move the cursor one place to the right within the entry.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_cursor_next().
 *          See elm_entry_cursor_next() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_cursor_next(Elm_Entry *obj)
{
   return elm_obj_entry_cursor_next(obj);
}

/**
 * @brief Legacy EAPI wrapper to drop any existing text selection within the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_select_none().
 *          See elm_entry_select_none() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_select_none(Elm_Entry *obj)
{
   elm_obj_entry_select_none(obj);
}

/**
 * @brief Legacy EAPI wrapper to hide the input panel (virtual keyboard).
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_input_panel_hide().
 *          See elm_entry_input_panel_hide() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_input_panel_hide(Elm_Entry *obj)
{
   elm_obj_entry_input_panel_hide(obj);
}

/**
 * @brief Legacy EAPI wrapper to select all text within the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_select_all().
 *          See elm_entry_select_all() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_select_all(Elm_Entry *obj)
{
   elm_obj_entry_select_all(obj);
}

/**
 * @brief Legacy EAPI wrapper to end a selection within the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_cursor_selection_end().
 *          See elm_entry_cursor_selection_end() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_cursor_selection_end(Elm_Entry *obj)
{
   elm_obj_entry_cursor_selection_end(obj);
}

/**
 * @brief Legacy EAPI wrapper to execute a "cut" action on the selected text in the entry.
 * @param[in] obj The entry object.
 * @details This function calls elm_obj_entry_selection_cut().
 *          See elm_entry_selection_cut() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_selection_cut(Elm_Entry *obj)
{
   elm_obj_entry_selection_cut(obj);
}

/**
 * @brief Legacy EAPI wrapper to get whether the entry is empty.
 * @param[in] obj The entry object.
 * @return @c EINA_TRUE if empty, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_is_empty().
 *          See elm_entry_is_empty() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_is_empty(const Elm_Entry *obj)
{
   return elm_obj_entry_is_empty(obj);
}

/**
 * @brief Legacy EAPI wrapper to remove a markup filter from the list.
 * @param[in] obj The entry object.
 * @param[in] func The filter function to remove.
 * @param[in] data The user data passed when adding the function.
 * @details This function calls elm_obj_entry_markup_filter_remove().
 *          See elm_entry_markup_filter_remove() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_markup_filter_remove(Elm_Entry *obj, Elm_Entry_Filter_Cb func, void *data)
{
   elm_obj_entry_markup_filter_remove(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to append a custom item provider to the list for that entry.
 * @param[in] obj The entry object.
 * @param[in] func The function called to provide the item object.
 * @param[in] data The data passed to @p func.
 * @details This function calls elm_obj_entry_item_provider_append().
 *          See elm_entry_item_provider_append() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_item_provider_append(Elm_Entry *obj, Elm_Entry_Item_Provider_Cb func, void *data)
{
   elm_obj_entry_item_provider_append(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to append a markup filter function for text inserted in the entry.
 * @param[in] obj The entry object.
 * @param[in] func The function to use as text filter.
 * @param[in] data User data to pass to @p func.
 * @details This function calls elm_obj_entry_markup_filter_append().
 *          See elm_entry_markup_filter_append() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_markup_filter_append(Elm_Entry *obj, Elm_Entry_Filter_Cb func, void *data)
{
   elm_obj_entry_markup_filter_append(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to append text to the entry.
 * @param[in] obj The entry object.
 * @param[in] str The text to be appended.
 * @details This function calls elm_obj_entry_append().
 *          See elm_entry_entry_append() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_entry_append(Elm_Entry *obj, const char *str)
{
   elm_obj_entry_append(obj, str);
}

/**
 * @brief Legacy EAPI wrapper to add an item to the entry's contextual menu.
 * @param[in] obj The entry object.
 * @param[in] label The item's text label.
 * @param[in] icon_file The item's icon file.
 * @param[in] icon_type The item's icon type.
 * @param[in] func The callback to execute when the item is clicked.
 * @param[in] data The data to associate with the item.
 * @details This function calls elm_obj_entry_context_menu_item_add().
 *          See elm_entry_context_menu_item_add() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_context_menu_item_add(Elm_Entry *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data)
{
   elm_obj_entry_context_menu_item_add(obj, label, icon_file, icon_type, func, data);
}

/**
 * @brief Legacy EAPI wrapper to prepend a markup filter function for text inserted in the entry.
 * @param[in] obj The entry object.
 * @param[in] func The function to use as text filter.
 * @param[in] data User data to pass to @p func.
 * @details This function calls elm_obj_entry_markup_filter_prepend().
 *          See elm_entry_markup_filter_prepend() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_markup_filter_prepend(Elm_Entry *obj, Elm_Entry_Filter_Cb func, void *data)
{
   elm_obj_entry_markup_filter_prepend(obj, func, data);
}

/**
 * @brief Legacy EAPI wrapper to set the prediction hint for intelligent reply suggestion.
 * @param[in] obj The entry object.
 * @param[in] prediction_hint The prediction hint text.
 * @details This function calls elm_obj_entry_prediction_hint_set().
 *          See elm_entry_prediction_hint_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.20
 * @ingroup Elm_Entry_Group
 */
EAPI void
elm_entry_prediction_hint_set(Elm_Entry *obj, const char *prediction_hint)
{
   elm_obj_entry_prediction_hint_set(obj, prediction_hint);
}

/**
 * @brief Legacy EAPI wrapper to set prediction hint data at the specified key.
 * @param[in] obj The entry object.
 * @param[in] key The key of the prediction hint.
 * @param[in] value The data to replace.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_prediction_hint_hash_set().
 *          See elm_entry_prediction_hint_hash_set() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.21
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_prediction_hint_hash_set(Elm_Entry *obj, const char *key, const char *value)
{
   return elm_obj_entry_prediction_hint_hash_set(obj, key, value);
}

/**
 * @brief Legacy EAPI wrapper to remove prediction hint data identified by a key.
 * @param[in] obj The entry object.
 * @param[in] key The key of the prediction hint.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 * @details This function calls elm_obj_entry_prediction_hint_hash_del().
 *          See elm_entry_prediction_hint_hash_del() in elm_entry_eo.legacy.h for full EAPI documentation.
 * @since 1.21
 * @ingroup Elm_Entry_Group
 */
EAPI Eina_Bool
elm_entry_prediction_hint_hash_del(Elm_Entry *obj, const char *key)
{
   return elm_obj_entry_prediction_hint_hash_del(obj, key);
}
