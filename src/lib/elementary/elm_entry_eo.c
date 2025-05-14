/** @brief Event descriptor for the "activated" event (e.g., Enter pressed). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ACTIVATED =
   EFL_EVENT_DESCRIPTION("activated");
/** @brief Event descriptor for the "changed" event (content modified). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");
/** @brief Event descriptor for the "changed,user" event (content modified by user). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_CHANGED_USER =
   EFL_EVENT_DESCRIPTION("changed,user");
/** @brief Event descriptor for the "validate" event (content validation requested). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_VALIDATE =
   EFL_EVENT_DESCRIPTION("validate");
/** @brief Event descriptor for the "context,open" event (context menu opened). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_CONTEXT_OPEN =
   EFL_EVENT_DESCRIPTION("context,open");
/** @brief Event descriptor for the "anchor,clicked" event. */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_CLICKED =
   EFL_EVENT_DESCRIPTION("anchor,clicked");
/** @brief Event descriptor for the "rejected" event (input rejected). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_REJECTED =
   EFL_EVENT_DESCRIPTION("rejected");
/** @brief Event descriptor for the "maxlength,reached" event. */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_MAXLENGTH_REACHED =
   EFL_EVENT_DESCRIPTION("maxlength,reached");
/** @brief Event descriptor for the "preedit,changed" event (IME preedit string changed). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_PREEDIT_CHANGED =
   EFL_EVENT_DESCRIPTION("preedit,changed");
/** @brief Event descriptor for the "press" event (mouse button pressed on entry). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_PRESS =
   EFL_EVENT_DESCRIPTION("press");
/** @brief Event descriptor for the "redo,request" event. */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_REDO_REQUEST =
   EFL_EVENT_DESCRIPTION("redo,request");
/** @brief Event descriptor for the "undo,request" event. */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_UNDO_REQUEST =
   EFL_EVENT_DESCRIPTION("undo,request");
/** @brief Event descriptor for the "text,set,done" event (text setting operation finished). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_TEXT_SET_DONE =
   EFL_EVENT_DESCRIPTION("text,set,done");
/** @brief Event descriptor for the "aborted" event (entry operation aborted). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ABORTED =
   EFL_EVENT_DESCRIPTION("aborted");
/** @brief Event descriptor for the "anchor,down" event (mouse down on an anchor). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_DOWN =
   EFL_EVENT_DESCRIPTION("anchor,down");
/** @brief Event descriptor for the "anchor,hover,opened" event (anchor hover popup opened). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_HOVER_OPENED =
   EFL_EVENT_DESCRIPTION("anchor,hover,opened");
/** @brief Event descriptor for the "anchor,in" event (mouse cursor entered an anchor). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_IN =
   EFL_EVENT_DESCRIPTION("anchor,in");
/** @brief Event descriptor for the "anchor,out" event (mouse cursor left an anchor). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_OUT =
   EFL_EVENT_DESCRIPTION("anchor,out");
/** @brief Event descriptor for the "anchor,up" event (mouse up on an anchor). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_ANCHOR_UP =
   EFL_EVENT_DESCRIPTION("anchor,up");
/** @brief Event descriptor for the "cursor,changed" event. */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_CURSOR_CHANGED =
   EFL_EVENT_DESCRIPTION("cursor,changed");
/** @brief Event descriptor for the "cursor,changed,manual" event (cursor changed by user). */
EWAPI const Efl_Event_Description _ELM_ENTRY_EVENT_CURSOR_CHANGED_MANUAL =
   EFL_EVENT_DESCRIPTION("cursor,changed,manual");

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_scrollable_set().
 * @details Sets whether the entry content should be scrollable.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param scroll EINA_TRUE to enable scrolling, EINA_FALSE to disable.
 * @see elm_obj_entry_scrollable_set
 */
void _elm_entry_scrollable_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool scroll);

/**
 * @internal
 * @brief Reflection function for the "scrollable" property setter.
 * @details Converts an Eina_Value to Eina_Bool and calls elm_obj_entry_scrollable_set().
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for scrollable state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_scrollable_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_scrollable_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_scrollable_set, EFL_FUNC_CALL(scroll), Eina_Bool scroll);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_scrollable_get().
 * @details Retrieves whether the entry content is scrollable.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if scrollable, EINA_FALSE otherwise.
 * @see elm_obj_entry_scrollable_get
 */
Eina_Bool _elm_entry_scrollable_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "scrollable" property getter.
 * @details Calls elm_obj_entry_scrollable_get() and returns the result as an Eina_Value.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean scrollable state.
 */
static Eina_Value
__eolian_elm_entry_scrollable_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_scrollable_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_scrollable_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_show_on_demand_set().
 * @details Sets whether the input panel should appear only on explicit user interaction.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param ondemand EINA_TRUE to enable on-demand showing, EINA_FALSE otherwise.
 * @see elm_obj_entry_input_panel_show_on_demand_set
 */
void _elm_entry_input_panel_show_on_demand_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool ondemand);

/**
 * @internal
 * @brief Reflection function for the "input_panel_show_on_demand" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for on-demand state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_input_panel_show_on_demand_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_input_panel_show_on_demand_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_show_on_demand_set, EFL_FUNC_CALL(ondemand), Eina_Bool ondemand);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_show_on_demand_get().
 * @details Retrieves whether the input panel appears only on explicit user interaction.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if on-demand showing is enabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_input_panel_show_on_demand_get
 */
Eina_Bool _elm_entry_input_panel_show_on_demand_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "input_panel_show_on_demand" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean on-demand state.
 */
static Eina_Value
__eolian_elm_entry_input_panel_show_on_demand_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_input_panel_show_on_demand_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_show_on_demand_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_context_menu_disabled_set().
 * @details Sets whether the context menu (e.g., on long press) is disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param disabled EINA_TRUE to disable the context menu, EINA_FALSE to enable.
 * @see elm_obj_entry_context_menu_disabled_set
 */
void _elm_entry_context_menu_disabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function for the "context_menu_disabled" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for disabled state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_context_menu_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_context_menu_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_context_menu_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_context_menu_disabled_get().
 * @details Retrieves whether the context menu is disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if the context menu is disabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_context_menu_disabled_get
 */
Eina_Bool _elm_entry_context_menu_disabled_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "context_menu_disabled" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean disabled state.
 */
static Eina_Value
__eolian_elm_entry_context_menu_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_context_menu_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_context_menu_disabled_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cnp_mode_set().
 * @details Sets the copy-and-paste mode for the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param cnp_mode The Elm_Cnp_Mode to set.
 * @see elm_obj_entry_cnp_mode_set
 */
void _elm_entry_cnp_mode_set(Eo *obj, Elm_Entry_Data *pd, Elm_Cnp_Mode cnp_mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_cnp_mode_set, EFL_FUNC_CALL(cnp_mode), Elm_Cnp_Mode cnp_mode);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cnp_mode_get().
 * @details Retrieves the copy-and-paste mode of the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Cnp_Mode.
 * @see elm_obj_entry_cnp_mode_get
 */
Elm_Cnp_Mode _elm_entry_cnp_mode_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_cnp_mode_get, Elm_Cnp_Mode, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_file_text_format_set().
 * @details Sets the text format for file loading/saving operations.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param format The Elm_Text_Format to use.
 * @see elm_obj_entry_file_text_format_set
 */
void _elm_entry_file_text_format_set(Eo *obj, Elm_Entry_Data *pd, Elm_Text_Format format);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_file_text_format_set, EFL_FUNC_CALL(format), Elm_Text_Format format);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_language_set().
 * @details Sets the language for the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param lang The Elm_Input_Panel_Lang to set.
 * @see elm_obj_entry_input_panel_language_set
 */
void _elm_entry_input_panel_language_set(Eo *obj, Elm_Entry_Data *pd, Elm_Input_Panel_Lang lang);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_language_set, EFL_FUNC_CALL(lang), Elm_Input_Panel_Lang lang);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_language_get().
 * @details Retrieves the language of the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Input_Panel_Lang.
 * @see elm_obj_entry_input_panel_language_get
 */
Elm_Input_Panel_Lang _elm_entry_input_panel_language_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_language_get, Elm_Input_Panel_Lang, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_handler_disabled_set().
 * @details Sets whether the selection handlers are disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param disabled EINA_TRUE to disable selection handlers, EINA_FALSE to enable.
 * @see elm_obj_entry_selection_handler_disabled_set
 */
void _elm_entry_selection_handler_disabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function for the "selection_handler_disabled" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for disabled state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_selection_handler_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_selection_handler_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_selection_handler_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_handler_disabled_get().
 * @details Retrieves whether the selection handlers are disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if selection handlers are disabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_selection_handler_disabled_get
 */
Eina_Bool _elm_entry_selection_handler_disabled_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "selection_handler_disabled" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean disabled state.
 */
static Eina_Value
__eolian_elm_entry_selection_handler_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_selection_handler_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_selection_handler_disabled_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_layout_variation_set().
 * @details Sets the layout variation for the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param variation The layout variation code.
 * @see elm_obj_entry_input_panel_layout_variation_set
 */
void _elm_entry_input_panel_layout_variation_set(Eo *obj, Elm_Entry_Data *pd, int variation);

/**
 * @internal
 * @brief Reflection function for the "input_panel_layout_variation" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the integer for layout variation.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_input_panel_layout_variation_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_input_panel_layout_variation_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_layout_variation_set, EFL_FUNC_CALL(variation), int variation);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_layout_variation_get().
 * @details Retrieves the layout variation of the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The layout variation code.
 * @see elm_obj_entry_input_panel_layout_variation_get
 */
int _elm_entry_input_panel_layout_variation_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "input_panel_layout_variation" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the integer layout variation.
 */
static Eina_Value
__eolian_elm_entry_input_panel_layout_variation_get_reflect(const Eo *obj)
{
   int val = elm_obj_entry_input_panel_layout_variation_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_layout_variation_get, int, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_autocapital_type_set().
 * @details Sets the auto-capitalization type for the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param autocapital_type The Elm_Autocapital_Type to set.
 * @see elm_obj_entry_autocapital_type_set
 */
void _elm_entry_autocapital_type_set(Eo *obj, Elm_Entry_Data *pd, Elm_Autocapital_Type autocapital_type);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_autocapital_type_set, EFL_FUNC_CALL(autocapital_type), Elm_Autocapital_Type autocapital_type);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_autocapital_type_get().
 * @details Retrieves the auto-capitalization type of the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Autocapital_Type.
 * @see elm_obj_entry_autocapital_type_get
 */
Elm_Autocapital_Type _elm_entry_autocapital_type_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_autocapital_type_get, Elm_Autocapital_Type, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_editable_set().
 * @details Sets whether the entry is editable by the user.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param editable EINA_TRUE if editable, EINA_FALSE if read-only.
 * @see elm_obj_entry_editable_set
 */
void _elm_entry_editable_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool editable);

/**
 * @internal
 * @brief Reflection function for the "editable" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for editable state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_editable_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_editable_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_editable_set, EFL_FUNC_CALL(editable), Eina_Bool editable);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_editable_get().
 * @details Retrieves whether the entry is editable.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if editable, EINA_FALSE if read-only.
 * @see elm_obj_entry_editable_get
 */
Eina_Bool _elm_entry_editable_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "editable" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean editable state.
 */
static Eina_Value
__eolian_elm_entry_editable_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_editable_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_editable_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_anchor_hover_style_set().
 * @details Sets the style for the anchor hover popup.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param style The style string to apply to the hover.
 * @see elm_obj_entry_anchor_hover_style_set
 */
void _elm_entry_anchor_hover_style_set(Eo *obj, Elm_Entry_Data *pd, const char *style);

/**
 * @internal
 * @brief Reflection function for the "anchor_hover_style" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the string for the style.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_anchor_hover_style_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_anchor_hover_style_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_anchor_hover_style_set, EFL_FUNC_CALL(style), const char *style);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_anchor_hover_style_get().
 * @details Retrieves the style used for the anchor hover popup.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The style string.
 * @see elm_obj_entry_anchor_hover_style_get
 */
const char *_elm_entry_anchor_hover_style_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "anchor_hover_style" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the style string.
 */
static Eina_Value
__eolian_elm_entry_anchor_hover_style_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_entry_anchor_hover_style_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_anchor_hover_style_get, const char *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_single_line_set().
 * @details Sets the entry to single-line or multi-line mode.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param single_line EINA_TRUE for single-line mode, EINA_FALSE for multi-line.
 * @see elm_obj_entry_single_line_set
 */
void _elm_entry_single_line_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool single_line);

/**
 * @internal
 * @brief Reflection function for the "single_line" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for single-line state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_single_line_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_single_line_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_single_line_set, EFL_FUNC_CALL(single_line), Eina_Bool single_line);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_single_line_get().
 * @details Retrieves whether the entry is in single-line mode.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if in single-line mode, EINA_FALSE otherwise.
 * @see elm_obj_entry_single_line_get
 */
Eina_Bool _elm_entry_single_line_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "single_line" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean single-line state.
 */
static Eina_Value
__eolian_elm_entry_single_line_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_single_line_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_single_line_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_password_set().
 * @details Sets the entry to password mode (text obscured).
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param password EINA_TRUE to enable password mode, EINA_FALSE to disable.
 * @see elm_obj_entry_password_set
 */
void _elm_entry_password_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool password);

/**
 * @internal
 * @brief Reflection function for the "password" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for password mode state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_password_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_password_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_password_set, EFL_FUNC_CALL(password), Eina_Bool password);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_password_get().
 * @details Retrieves whether the entry is in password mode.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if in password mode, EINA_FALSE otherwise.
 * @see elm_obj_entry_password_get
 */
Eina_Bool _elm_entry_password_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "password" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean password mode state.
 */
static Eina_Value
__eolian_elm_entry_password_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_password_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_password_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_return_key_disabled_set().
 * @details Sets whether the return key on the input panel is disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param disabled EINA_TRUE to disable the return key, EINA_FALSE to enable.
 * @see elm_obj_entry_input_panel_return_key_disabled_set
 */
void _elm_entry_input_panel_return_key_disabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function for the "input_panel_return_key_disabled" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for disabled state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_input_panel_return_key_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_input_panel_return_key_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_return_key_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_return_key_disabled_get().
 * @details Retrieves whether the return key on the input panel is disabled.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if the return key is disabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_input_panel_return_key_disabled_get
 */
Eina_Bool _elm_entry_input_panel_return_key_disabled_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "input_panel_return_key_disabled" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean disabled state.
 */
static Eina_Value
__eolian_elm_entry_input_panel_return_key_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_input_panel_return_key_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_return_key_disabled_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_autosave_set().
 * @details Sets whether the entry should automatically save its content.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param auto_save EINA_TRUE to enable autosave, EINA_FALSE to disable.
 * @see elm_obj_entry_autosave_set
 */
void _elm_entry_autosave_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool auto_save);

/**
 * @internal
 * @brief Reflection function for the "autosave" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for autosave state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_autosave_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_autosave_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_autosave_set, EFL_FUNC_CALL(auto_save), Eina_Bool auto_save);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_autosave_get().
 * @details Retrieves whether autosave is enabled for the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if autosave is enabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_autosave_get
 */
Eina_Bool _elm_entry_autosave_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "autosave" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean autosave state.
 */
static Eina_Value
__eolian_elm_entry_autosave_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_autosave_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_autosave_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_anchor_hover_parent_set().
 * @details Sets the parent object for the anchor hover popup.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param parent The Efl_Canvas_Object to use as parent.
 * @see elm_obj_entry_anchor_hover_parent_set
 */
void _elm_entry_anchor_hover_parent_set(Eo *obj, Elm_Entry_Data *pd, Efl_Canvas_Object *parent);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_anchor_hover_parent_set, EFL_FUNC_CALL(parent), Efl_Canvas_Object *parent);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_anchor_hover_parent_get().
 * @details Retrieves the parent object of the anchor hover popup.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The parent Efl_Canvas_Object.
 * @see elm_obj_entry_anchor_hover_parent_get
 */
Efl_Canvas_Object *_elm_entry_anchor_hover_parent_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_anchor_hover_parent_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_prediction_allow_set().
 * @details Sets whether text prediction is allowed.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param prediction EINA_TRUE to allow prediction, EINA_FALSE to disallow.
 * @see elm_obj_entry_prediction_allow_set
 */
void _elm_entry_prediction_allow_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool prediction);

/**
 * @internal
 * @brief Reflection function for the "prediction_allow" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for prediction state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_prediction_allow_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_prediction_allow_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_prediction_allow_set, EFL_FUNC_CALL(prediction), Eina_Bool prediction);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_prediction_allow_get().
 * @details Retrieves whether text prediction is allowed.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if prediction is allowed, EINA_FALSE otherwise.
 * @see elm_obj_entry_prediction_allow_get
 */
Eina_Bool _elm_entry_prediction_allow_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "prediction_allow" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean prediction state.
 */
static Eina_Value
__eolian_elm_entry_prediction_allow_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_prediction_allow_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_prediction_allow_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_hint_set().
 * @details Sets input hints for the input method.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param hints The Elm_Input_Hints to set.
 * @see elm_obj_entry_input_hint_set
 */
void _elm_entry_input_hint_set(Eo *obj, Elm_Entry_Data *pd, Elm_Input_Hints hints);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_hint_set, EFL_FUNC_CALL(hints), Elm_Input_Hints hints);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_hint_get().
 * @details Retrieves the input hints for the input method.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Input_Hints.
 * @see elm_obj_entry_input_hint_get
 */
Elm_Input_Hints _elm_entry_input_hint_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_hint_get, Elm_Input_Hints, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_layout_set().
 * @details Sets the layout for the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param layout The Elm_Input_Panel_Layout to set.
 * @see elm_obj_entry_input_panel_layout_set
 */
void _elm_entry_input_panel_layout_set(Eo *obj, Elm_Entry_Data *pd, Elm_Input_Panel_Layout layout);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_layout_set, EFL_FUNC_CALL(layout), Elm_Input_Panel_Layout layout);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_layout_get().
 * @details Retrieves the layout of the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Input_Panel_Layout.
 * @see elm_obj_entry_input_panel_layout_get
 */
Elm_Input_Panel_Layout _elm_entry_input_panel_layout_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_layout_get, Elm_Input_Panel_Layout, 8 /* Elm.Input.Panel.Layout.invalid */);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_return_key_type_set().
 * @details Sets the type of the return key on the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param return_key_type The Elm_Input_Panel_Return_Key_Type to set.
 * @see elm_obj_entry_input_panel_return_key_type_set
 */
void _elm_entry_input_panel_return_key_type_set(Eo *obj, Elm_Entry_Data *pd, Elm_Input_Panel_Return_Key_Type return_key_type);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_return_key_type_set, EFL_FUNC_CALL(return_key_type), Elm_Input_Panel_Return_Key_Type return_key_type);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_return_key_type_get().
 * @details Retrieves the type of the return key on the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Input_Panel_Return_Key_Type.
 * @see elm_obj_entry_input_panel_return_key_type_get
 */
Elm_Input_Panel_Return_Key_Type _elm_entry_input_panel_return_key_type_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_return_key_type_get, Elm_Input_Panel_Return_Key_Type, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_enabled_set().
 * @details Sets whether the input panel is automatically shown/hidden.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param enabled EINA_TRUE to enable automatic behavior, EINA_FALSE for manual.
 * @see elm_obj_entry_input_panel_enabled_set
 */
void _elm_entry_input_panel_enabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool enabled);

/**
 * @internal
 * @brief Reflection function for the "input_panel_enabled" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for enabled state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_input_panel_enabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_input_panel_enabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_enabled_set, EFL_FUNC_CALL(enabled), Eina_Bool enabled);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_enabled_get().
 * @details Retrieves whether the input panel is automatically shown/hidden.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if automatic behavior is enabled, EINA_FALSE otherwise.
 * @see elm_obj_entry_input_panel_enabled_get
 */
Eina_Bool _elm_entry_input_panel_enabled_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "input_panel_enabled" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean enabled state.
 */
static Eina_Value
__eolian_elm_entry_input_panel_enabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_input_panel_enabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_input_panel_enabled_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_line_wrap_set().
 * @details Sets the line wrapping type for multi-line entries.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param wrap The Elm_Wrap_Type to set.
 * @see elm_obj_entry_line_wrap_set
 */
void _elm_entry_line_wrap_set(Eo *obj, Elm_Entry_Data *pd, Elm_Wrap_Type wrap);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_line_wrap_set, EFL_FUNC_CALL(wrap), Elm_Wrap_Type wrap);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_line_wrap_get().
 * @details Retrieves the line wrapping type of the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The current Elm_Wrap_Type.
 * @see elm_obj_entry_line_wrap_get
 */
Elm_Wrap_Type _elm_entry_line_wrap_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_line_wrap_get, Elm_Wrap_Type, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_pos_set().
 * @details Sets the text cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param pos The cursor position (character index).
 * @see elm_obj_entry_cursor_pos_set
 */
void _elm_entry_cursor_pos_set(Eo *obj, Elm_Entry_Data *pd, int pos);

/**
 * @internal
 * @brief Reflection function for the "cursor_pos" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the integer for cursor position.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_cursor_pos_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_cursor_pos_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_cursor_pos_set, EFL_FUNC_CALL(pos), int pos);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_pos_get().
 * @details Retrieves the current text cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The cursor position (character index).
 * @see elm_obj_entry_cursor_pos_get
 */
int _elm_entry_cursor_pos_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "cursor_pos" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the integer cursor position.
 */
static Eina_Value
__eolian_elm_entry_cursor_pos_get_reflect(const Eo *obj)
{
   int val = elm_obj_entry_cursor_pos_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_cursor_pos_get, int, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_icon_visible_set().
 * @details Sets the visibility of the icon content part.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param setting EINA_TRUE to show the icon, EINA_FALSE to hide.
 * @see elm_obj_entry_icon_visible_set
 */
void _elm_entry_icon_visible_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool setting);

/**
 * @internal
 * @brief Reflection function for the "icon_visible" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for icon visibility.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_icon_visible_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_icon_visible_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_icon_visible_set, EFL_FUNC_CALL(setting), Eina_Bool setting);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_line_end_set().
 * @details Moves the cursor to the end of the current line.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_line_end_set
 */
void _elm_entry_cursor_line_end_set(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_line_end_set);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_region_set().
 * @details Selects a region of text.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start The start position of the selection.
 * @param end The end position of the selection.
 * @see elm_obj_entry_select_region_set
 */
void _elm_entry_select_region_set(Eo *obj, Elm_Entry_Data *pd, int start, int end);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_select_region_set, EFL_FUNC_CALL(start, end), int start, int end);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_region_get().
 * @details Gets the current selected region.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param[out] start Pointer to store the start position of the selection.
 * @param[out] end Pointer to store the end position of the selection.
 * @see elm_obj_entry_select_region_get
 */
void _elm_entry_select_region_get(const Eo *obj, Elm_Entry_Data *pd, int *start, int *end);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_entry_select_region_get, EFL_FUNC_CALL(start, end), int *start, int *end);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_return_key_autoenabled_set().
 * @details Sets whether the return key is auto-enabled based on text presence.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param enabled EINA_TRUE to enable auto-enabling, EINA_FALSE otherwise.
 * @see elm_obj_entry_input_panel_return_key_autoenabled_set
 */
void _elm_entry_input_panel_return_key_autoenabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool enabled);

/**
 * @internal
 * @brief Reflection function for the "input_panel_return_key_autoenabled" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for auto-enabled state.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_input_panel_return_key_autoenabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_input_panel_return_key_autoenabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_return_key_autoenabled_set, EFL_FUNC_CALL(enabled), Eina_Bool enabled);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_end_visible_set().
 * @details Sets the visibility of the end content part.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param setting EINA_TRUE to show the end part, EINA_FALSE to hide.
 * @see elm_obj_entry_end_visible_set
 */
void _elm_entry_end_visible_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool setting);

/**
 * @internal
 * @brief Reflection function for the "end_visible" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for end part visibility.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_end_visible_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_end_visible_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_end_visible_set, EFL_FUNC_CALL(setting), Eina_Bool setting);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_begin_set().
 * @details Moves the cursor to the beginning of the entry text.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_begin_set
 */
void _elm_entry_cursor_begin_set(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_begin_set);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_line_begin_set().
 * @details Moves the cursor to the beginning of the current line.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_line_begin_set
 */
void _elm_entry_cursor_line_begin_set(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_line_begin_set);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_end_set().
 * @details Moves the cursor to the end of the entry text.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_end_set
 */
void _elm_entry_cursor_end_set(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_end_set);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_textblock_get().
 * @details Retrieves the internal Evas_Object used for text rendering.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The Efl_Canvas_Object for the textblock.
 * @see elm_obj_entry_textblock_get
 */
Efl_Canvas_Object *_elm_entry_textblock_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_textblock_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_textblock_cursor_geometry_get().
 * @details Retrieves the geometry of the text cursor.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param[out] x Pointer to store the x-coordinate.
 * @param[out] y Pointer to store the y-coordinate.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_textblock_cursor_geometry_get
 */
Eina_Bool _elm_entry_textblock_cursor_geometry_get(const Eo *obj, Elm_Entry_Data *pd, int *x, int *y, int *w, int *h);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_entry_textblock_cursor_geometry_get, Eina_Bool, 0, EFL_FUNC_CALL(x, y, w, h), int *x, int *y, int *w, int *h);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_imf_context_get().
 * @details Retrieves the Input Method Framework (IMF) context.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return Pointer to the IMF context.
 * @see elm_obj_entry_imf_context_get
 */
void *_elm_entry_imf_context_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_imf_context_get, void *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_is_format_get().
 * @details Checks if the cursor is currently over a format tag.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if over a format tag, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_is_format_get
 */
Eina_Bool _elm_entry_cursor_is_format_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_cursor_is_format_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_textblock_cursor_content_get().
 * @details Retrieves the character content at the current cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return A newly allocated string with the character, or NULL. Must be freed.
 * @see elm_obj_entry_textblock_cursor_content_get
 */
char *_elm_entry_textblock_cursor_content_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_textblock_cursor_content_get, char *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_get().
 * @details Retrieves the currently selected text.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return A const pointer to the selected text (markup), or NULL. Do not free.
 * @see elm_obj_entry_selection_get
 */
const char *_elm_entry_selection_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_selection_get, const char *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_is_visible_format_get().
 * @details Checks if the cursor is over a visible format node (e.g., line break).
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if over a visible format node, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_is_visible_format_get
 */
Eina_Bool _elm_entry_cursor_is_visible_format_get(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_cursor_is_visible_format_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_allow_set().
 * @details Sets whether text selection is allowed in the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param allow EINA_TRUE to allow selection, EINA_FALSE to disallow.
 * @see elm_obj_entry_select_allow_set
 */
void _elm_entry_select_allow_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool allow);

/**
 * @internal
 * @brief Reflection function for the "select_allow" property setter.
 * @param obj The Eo object.
 * @param val The Eina_Value containing the boolean for selection allowance.
 * @return EINA_ERROR_NONE on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_entry_select_allow_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_entry_select_allow_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_select_allow_set, EFL_FUNC_CALL(allow), Eina_Bool allow);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_allow_get().
 * @details Retrieves whether text selection is allowed.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if selection is allowed, EINA_FALSE otherwise.
 * @see elm_obj_entry_select_allow_get
 */
Eina_Bool _elm_entry_select_allow_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the "select_allow" property getter.
 * @param obj The Eo object.
 * @return An Eina_Value containing the boolean selection allowance state.
 */
static Eina_Value
__eolian_elm_entry_select_allow_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_entry_select_allow_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_select_allow_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_prev().
 * @details Moves the cursor one position to the left.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_prev
 */
Eina_Bool _elm_entry_cursor_prev(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_entry_cursor_prev, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_text_style_user_pop().
 * @details Pops the last user-defined text style from the stack.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_text_style_user_pop
 */
void _elm_entry_text_style_user_pop(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_text_style_user_pop);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_item_provider_prepend().
 * @details Prepends a custom item provider callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The item provider callback function.
 * @param data User data for the callback.
 * @see elm_obj_entry_item_provider_prepend
 */
void _elm_entry_item_provider_prepend(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Item_Provider_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_item_provider_prepend, EFL_FUNC_CALL(func, data), Elm_Entry_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_show().
 * @details Manually shows the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_input_panel_show
 */
void _elm_entry_input_panel_show(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_input_panel_show);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_imf_context_reset().
 * @details Resets the IMF context.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_imf_context_reset
 */
void _elm_entry_imf_context_reset(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_imf_context_reset);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_anchor_hover_end().
 * @details Closes the anchor hover popup.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_anchor_hover_end
 */
void _elm_entry_anchor_hover_end(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_anchor_hover_end);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_selection_begin().
 * @details Starts a text selection at the current cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_selection_begin
 */
void _elm_entry_cursor_selection_begin(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_selection_begin);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_down().
 * @details Moves the cursor one line down.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_down
 */
Eina_Bool _elm_entry_cursor_down(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_entry_cursor_down, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_file_save().
 * @details Saves the entry content to the currently set file.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_file_save
 */
void _elm_entry_file_save(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_file_save);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_copy().
 * @details Copies the selected text to the clipboard.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_selection_copy
 */
void _elm_entry_selection_copy(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_selection_copy);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_text_style_user_push().
 * @details Pushes a user-defined text style onto the stack.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param style The style string to push.
 * @see elm_obj_entry_text_style_user_push
 */
void _elm_entry_text_style_user_push(Eo *obj, Elm_Entry_Data *pd, const char *style);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_text_style_user_push, EFL_FUNC_CALL(style), const char *style);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_item_provider_remove().
 * @details Removes a custom item provider callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The item provider callback function to remove.
 * @param data User data associated with the callback.
 * @see elm_obj_entry_item_provider_remove
 */
void _elm_entry_item_provider_remove(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Item_Provider_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_item_provider_remove, EFL_FUNC_CALL(func, data), Elm_Entry_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_text_style_user_peek().
 * @details Peeks at the top user-defined text style on the stack.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The style string, or NULL if stack is empty. Do not free.
 * @see elm_obj_entry_text_style_user_peek
 */
const char *_elm_entry_text_style_user_peek(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_text_style_user_peek, const char *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_context_menu_clear().
 * @details Clears all items from the context menu.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_context_menu_clear
 */
void _elm_entry_context_menu_clear(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_context_menu_clear);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_up().
 * @details Moves the cursor one line up.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_up
 */
Eina_Bool _elm_entry_cursor_up(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_entry_cursor_up, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_insert().
 * @details Inserts text at the current cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param entry The text (markup) to insert.
 * @see elm_obj_entry_insert
 */
void _elm_entry_entry_insert(Eo *obj, Elm_Entry_Data *pd, const char *entry);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_insert, EFL_FUNC_CALL(entry), const char *entry);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_imdata_set().
 * @details Sets input method specific data for the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param data Pointer to the data.
 * @param len Length of the data in bytes.
 * @see elm_obj_entry_input_panel_imdata_set
 */
void _elm_entry_input_panel_imdata_set(Eo *obj, Elm_Entry_Data *pd, const void *data, int len);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_input_panel_imdata_set, EFL_FUNC_CALL(data, len), const void *data, int len);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_imdata_get().
 * @details Gets input method specific data from the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param[out] data Pointer to store the data.
 * @param[out] len Pointer to store the length of the data.
 * @see elm_obj_entry_input_panel_imdata_get
 */
void _elm_entry_input_panel_imdata_get(const Eo *obj, Elm_Entry_Data *pd, void *data, int *len);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_entry_input_panel_imdata_get, EFL_FUNC_CALL(data, len), void *data, int *len);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_paste().
 * @details Pastes text from the clipboard at the current cursor position.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_selection_paste
 */
void _elm_entry_selection_paste(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_selection_paste);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_next().
 * @details Moves the cursor one position to the right.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_cursor_next
 */
Eina_Bool _elm_entry_cursor_next(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY(elm_obj_entry_cursor_next, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_none().
 * @details Clears any active text selection.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_select_none
 */
void _elm_entry_select_none(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_select_none);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_input_panel_hide().
 * @details Manually hides the input panel.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_input_panel_hide
 */
void _elm_entry_input_panel_hide(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_input_panel_hide);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_select_all().
 * @details Selects all text in the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_select_all
 */
void _elm_entry_select_all(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_select_all);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_cursor_selection_end().
 * @details Ends the current text selection process.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_cursor_selection_end
 */
void _elm_entry_cursor_selection_end(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_cursor_selection_end);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_selection_cut().
 * @details Cuts the selected text to the clipboard.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see elm_obj_entry_selection_cut
 */
void _elm_entry_selection_cut(Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_entry_selection_cut);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_is_empty().
 * @details Checks if the entry contains any text.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if empty, EINA_FALSE otherwise.
 * @see elm_obj_entry_is_empty
 */
Eina_Bool _elm_entry_is_empty(const Eo *obj, Elm_Entry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_entry_is_empty, Eina_Bool, EINA_TRUE /* true */);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_markup_filter_remove().
 * @details Removes a markup filter callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The filter callback function to remove.
 * @param data User data associated with the callback.
 * @see elm_obj_entry_markup_filter_remove
 */
void _elm_entry_markup_filter_remove(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_markup_filter_remove, EFL_FUNC_CALL(func, data), Elm_Entry_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_item_provider_append().
 * @details Appends a custom item provider callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The item provider callback function.
 * @param data User data for the callback.
 * @see elm_obj_entry_item_provider_append
 */
void _elm_entry_item_provider_append(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Item_Provider_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_item_provider_append, EFL_FUNC_CALL(func, data), Elm_Entry_Item_Provider_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_markup_filter_append().
 * @details Appends a markup filter callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The filter callback function.
 * @param data User data for the callback.
 * @see elm_obj_entry_markup_filter_append
 */
void _elm_entry_markup_filter_append(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_markup_filter_append, EFL_FUNC_CALL(func, data), Elm_Entry_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_append().
 * @details Appends text to the end of the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param str The text (markup) to append.
 * @see elm_obj_entry_append
 */
void _elm_entry_entry_append(Eo *obj, Elm_Entry_Data *pd, const char *str);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_append, EFL_FUNC_CALL(str), const char *str);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_context_menu_item_add().
 * @details Adds an item to the context menu.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param label Text label for the menu item.
 * @param icon_file Path to an icon file for the menu item.
 * @param icon_type Type of the icon.
 * @param func Callback function when the item is selected.
 * @param data User data for the callback.
 * @see elm_obj_entry_context_menu_item_add
 */
void _elm_entry_context_menu_item_add(Eo *obj, Elm_Entry_Data *pd, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_context_menu_item_add, EFL_FUNC_CALL(label, icon_file, icon_type, func, data), const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_markup_filter_prepend().
 * @details Prepends a markup filter callback.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param func The filter callback function.
 * @param data User data for the callback.
 * @see elm_obj_entry_markup_filter_prepend
 */
void _elm_entry_markup_filter_prepend(Eo *obj, Elm_Entry_Data *pd, Elm_Entry_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_markup_filter_prepend, EFL_FUNC_CALL(func, data), Elm_Entry_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_prediction_hint_set().
 * @details Sets a prediction hint string for the entry.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param prediction_hint The prediction hint string.
 * @see elm_obj_entry_prediction_hint_set
 */
void _elm_entry_prediction_hint_set(Eo *obj, Elm_Entry_Data *pd, const char *prediction_hint);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_entry_prediction_hint_set, EFL_FUNC_CALL(prediction_hint), const char *prediction_hint);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_prediction_hint_hash_set().
 * @details Sets a key-value pair in the prediction hint hash.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param key The key for the hint.
 * @param value The value for the hint.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_prediction_hint_hash_set
 */
Eina_Bool _elm_entry_prediction_hint_hash_set(Eo *obj, Elm_Entry_Data *pd, const char *key, const char *value);

EOAPI EFL_FUNC_BODYV(elm_obj_entry_prediction_hint_hash_set, Eina_Bool, 0, EFL_FUNC_CALL(key, value), const char *key, const char *value);

/**
 * @internal
 * @brief Internal implementation for elm_obj_entry_prediction_hint_hash_del().
 * @details Deletes a key-value pair from the prediction hint hash.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param key The key to delete.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see elm_obj_entry_prediction_hint_hash_del
 */
Eina_Bool _elm_entry_prediction_hint_hash_del(Eo *obj, Elm_Entry_Data *pd, const char *key);

EOAPI EFL_FUNC_BODYV(elm_obj_entry_prediction_hint_hash_del, Eina_Bool, 0, EFL_FUNC_CALL(key), const char *key);

/**
 * @internal
 * @brief Constructor for Elm_Entry objects.
 * @details This function is called when a new Elm_Entry object is created.
 *          It performs initial setup for the entry instance.
 * @param obj The Eo object being constructed.
 * @param pd Pointer to the private data of the object.
 * @return The constructed Efl_Object, or NULL on failure.
 * @see efl_constructor
 */
Efl_Object *_elm_entry_efl_object_constructor(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Sets the visibility of the Elm_Entry object.
 * @details Implements the efl_gfx_entity_visible_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param v EINA_TRUE if visible, EINA_FALSE otherwise.
 * @see efl_gfx_entity_visible_set
 */
void _elm_entry_efl_gfx_entity_visible_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool v);


/**
 * @internal
 * @brief Sets the position of the Elm_Entry object.
 * @details Implements the efl_gfx_entity_position_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param pos The 2D position (Eina_Position2D).
 * @see efl_gfx_entity_position_set
 */
void _elm_entry_efl_gfx_entity_position_set(Eo *obj, Elm_Entry_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Sets the size of the Elm_Entry object.
 * @details Implements the efl_gfx_entity_size_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param size The 2D size (Eina_Size2D).
 * @see efl_gfx_entity_size_set
 */
void _elm_entry_efl_gfx_entity_size_set(Eo *obj, Elm_Entry_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Adds a sub-object to the Elm_Entry's canvas group.
 * @details Implements the efl_canvas_group_member_add interface.
 * @param obj The Eo object (Elm_Entry).
 * @param pd Pointer to the private data of the object.
 * @param sub_obj The Efl_Canvas_Object to add as a member.
 * @see efl_canvas_group_member_add
 */
void _elm_entry_efl_canvas_group_group_member_add(Eo *obj, Elm_Entry_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Adds a callback for a layout signal.
 * @details Implements the efl_layout_signal_callback_add interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 * @param func_data User data for the callback.
 * @param func The callback function.
 * @param func_free_cb Optional callback to free func_data.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_layout_signal_callback_add
 */
Eina_Bool _elm_entry_efl_layout_signal_signal_callback_add(Eo *obj, Elm_Entry_Data *pd, const char *emission, const char *source, void *func_data, EflLayoutSignalCb func, Eina_Free_Cb func_free_cb);

/**
 * @internal
 * @brief Deletes a callback for a layout signal.
 * @details Implements the efl_layout_signal_callback_del interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 * @param func_data User data for the callback (must match the one used in add).
 * @param func The callback function (must match the one used in add).
 * @param func_free_cb Optional callback to free func_data (ignored if NULL during add).
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_layout_signal_callback_del
 */
Eina_Bool _elm_entry_efl_layout_signal_signal_callback_del(Eo *obj, Elm_Entry_Data *pd, const char *emission, const char *source, void *func_data, EflLayoutSignalCb func, Eina_Free_Cb func_free_cb);

/**
 * @internal
 * @brief Emits a layout signal.
 * @details Implements the efl_layout_signal_emit interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param emission The emission string of the signal.
 * @param source The source string of the signal.
 * @see efl_layout_signal_emit
 */
void _elm_entry_efl_layout_signal_signal_emit(Eo *obj, Elm_Entry_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Forces a recalculation of the layout.
 * @details Implements the efl_layout_calc_force interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see efl_layout_calc_force
 */
void _elm_entry_efl_layout_calc_calc_force(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Handles accessibility activation.
 * @details Implements the efl_ui_widget_on_access_activate interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param act The type of activation.
 * @return EINA_TRUE if handled, EINA_FALSE otherwise.
 * @see efl_ui_widget_on_access_activate
 */
Eina_Bool _elm_entry_efl_ui_widget_on_access_activate(Eo *obj, Elm_Entry_Data *pd, Efl_Ui_Activate act);

/**
 * @internal
 * @brief Applies the theme to the Elm_Entry widget.
 * @details Implements the efl_ui_widget_theme_apply interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_ERROR_NONE on success, or an error code.
 * @see efl_ui_widget_theme_apply
 */
Eina_Error _elm_entry_efl_ui_widget_theme_apply(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Creates a focus manager for the widget.
 * @details Implements the efl_ui_widget_focus_manager_create interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param root The root focus object.
 * @return The created Efl_Ui_Focus_Manager, or NULL on failure.
 * @see efl_ui_widget_focus_manager_create
 */
Efl_Ui_Focus_Manager *_elm_entry_efl_ui_widget_focus_manager_focus_manager_create(Eo *obj, Elm_Entry_Data *pd, Efl_Ui_Focus_Object *root);

/**
 * @internal
 * @brief Handles focus updates for the widget.
 * @details Implements the efl_ui_focus_object_on_focus_update interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_TRUE if focus state changed, EINA_FALSE otherwise.
 * @see efl_ui_focus_object_on_focus_update
 */
Eina_Bool _elm_entry_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets the widget's interest region for accessibility.
 * @details Implements the efl_ui_widget_interest_region_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The interest region as an Eina_Rect.
 * @see efl_ui_widget_interest_region_get
 */
Eina_Rect _elm_entry_efl_ui_widget_interest_region_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Sets the disabled state of the widget.
 * @details Implements the efl_ui_widget_disabled_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param disabled EINA_TRUE to disable, EINA_FALSE to enable.
 * @see efl_ui_widget_disabled_set
 */
void _elm_entry_efl_ui_widget_disabled_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Deletes a sub-object from the widget.
 * @details Implements the efl_ui_widget_sub_object_del interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param sub_obj The sub-object to delete.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_ui_widget_sub_object_del
 */
Eina_Bool _elm_entry_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Entry_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Sets the scrollbar policies for the entry.
 * @details Implements the elm_interface_scrollable_policy_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param hbar Horizontal scrollbar policy.
 * @param vbar Vertical scrollbar policy.
 * @see elm_interface_scrollable_policy_set
 */
void _elm_entry_elm_interface_scrollable_policy_set(Eo *obj, Elm_Entry_Data *pd, Elm_Scroller_Policy hbar, Elm_Scroller_Policy vbar);

/**
 * @internal
 * @brief Sets whether scrolling bounce is allowed.
 * @details Implements the elm_interface_scrollable_bounce_allow_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param horiz EINA_TRUE to allow horizontal bounce.
 * @param vert EINA_TRUE to allow vertical bounce.
 * @see elm_interface_scrollable_bounce_allow_set
 */
void _elm_entry_elm_interface_scrollable_bounce_allow_set(Eo *obj, Elm_Entry_Data *pd, Eina_Bool horiz, Eina_Bool vert);

/**
 * @internal
 * @brief Gets the accessibility state set.
 * @details Implements the efl_access_object_state_set_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The Efl_Access_State_Set.
 * @see efl_access_object_state_set_get
 */
Efl_Access_State_Set _elm_entry_efl_access_object_state_set_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets the internationalized accessibility name.
 * @details Implements the efl_access_object_i18n_name_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The i18n name string.
 * @see efl_access_object_i18n_name_get
 */
const char *_elm_entry_efl_access_object_i18n_name_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets a portion of the accessible text.
 * @details Implements the efl_access_text_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start_offset Start offset of the text portion.
 * @param end_offset End offset of the text portion.
 * @return The text string (must be freed).
 * @see efl_access_text_get
 */
char *_elm_entry_efl_access_text_access_text_get(const Eo *obj, Elm_Entry_Data *pd, int start_offset, int end_offset);

/**
 * @internal
 * @brief Gets an accessible text string based on granularity.
 * @details Implements the efl_access_text_string_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param granularity The granularity (char, word, sentence, etc.).
 * @param[out] start_offset Start offset of the returned string.
 * @param[out] end_offset End offset of the returned string.
 * @param[out] ret Pointer to store the returned string (must be freed).
 * @see efl_access_text_string_get
 */
void _elm_entry_efl_access_text_string_get(const Eo *obj, Elm_Entry_Data *pd, Efl_Access_Text_Granularity granularity, int *start_offset, int *end_offset, char **ret EFL_TRANSFER_OWNERSHIP);

/**
 * @internal
 * @brief Gets an accessible text attribute.
 * @details Implements the efl_access_text_attribute_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param name Name of the attribute.
 * @param[out] start_offset Start offset of the attribute range.
 * @param[out] end_offset End offset of the attribute range.
 * @param[out] value Pointer to store the attribute value string (must be freed).
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_attribute_get
 */
Eina_Bool _elm_entry_efl_access_text_attribute_get(const Eo *obj, Elm_Entry_Data *pd, const char *name, int *start_offset, int *end_offset, char **value);

/**
 * @internal
 * @brief Gets all accessible text attributes for a range.
 * @details Implements the efl_access_text_attributes_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param[in,out] start_offset Start offset of the range.
 * @param[in,out] end_offset End offset of the range.
 * @param[out] attributes Pointer to store the list of attributes.
 * @see efl_access_text_attributes_get
 */
void _elm_entry_efl_access_text_text_attributes_get(const Eo *obj, Elm_Entry_Data *pd, int *start_offset, int *end_offset, Eina_List **attributes);

/**
 * @internal
 * @brief Gets the default accessible text attributes.
 * @details Implements the efl_access_text_default_attributes_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return List of default attributes.
 * @see efl_access_text_default_attributes_get
 */
Eina_List *_elm_entry_efl_access_text_default_attributes_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Sets the accessible text caret offset.
 * @details Implements the efl_access_text_caret_offset_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param offset The new caret offset.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_caret_offset_set
 */
Eina_Bool _elm_entry_efl_access_text_caret_offset_set(Eo *obj, Elm_Entry_Data *pd, int offset);

/**
 * @internal
 * @brief Gets the accessible text caret offset.
 * @details Implements the efl_access_text_caret_offset_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The caret offset.
 * @see efl_access_text_caret_offset_get
 */
int _elm_entry_efl_access_text_caret_offset_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets the character at a specific offset in accessible text.
 * @details Implements the efl_access_text_character_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param offset The character offset.
 * @return The Unicode character.
 * @see efl_access_text_character_get
 */
Eina_Unicode _elm_entry_efl_access_text_character_get(const Eo *obj, Elm_Entry_Data *pd, int offset);

/**
 * @internal
 * @brief Gets the extents of a character in accessible text.
 * @details Implements the efl_access_text_character_extents_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param offset The character offset.
 * @param screen_coords Whether to use screen coordinates.
 * @param[out] rect Pointer to store the character extents.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_character_extents_get
 */
Eina_Bool _elm_entry_efl_access_text_character_extents_get(const Eo *obj, Elm_Entry_Data *pd, int offset, Eina_Bool screen_coords, Eina_Rect *rect);

/**
 * @internal
 * @brief Gets the total character count in accessible text.
 * @details Implements the efl_access_text_character_count_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The character count.
 * @see efl_access_text_character_count_get
 */
int _elm_entry_efl_access_text_character_count_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets the text offset at a given point.
 * @details Implements the efl_access_text_offset_at_point_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param screen_coords Whether to use screen coordinates.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return The text offset.
 * @see efl_access_text_offset_at_point_get
 */
int _elm_entry_efl_access_text_offset_at_point_get(const Eo *obj, Elm_Entry_Data *pd, Eina_Bool screen_coords, int x, int y);

/**
 * @internal
 * @brief Gets bounded ranges of accessible text within a rectangle.
 * @details Implements the efl_access_text_bounded_ranges_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param screen_coords Whether to use screen coordinates.
 * @param rect The bounding rectangle.
 * @param xclip X-axis clipping type.
 * @param yclip Y-axis clipping type.
 * @return List of bounded ranges.
 * @see efl_access_text_bounded_ranges_get
 */
Eina_List *_elm_entry_efl_access_text_bounded_ranges_get(const Eo *obj, Elm_Entry_Data *pd, Eina_Bool screen_coords, Eina_Rect rect, Efl_Access_Text_Clip_Type xclip, Efl_Access_Text_Clip_Type yclip);

/**
 * @internal
 * @brief Gets the extents of a range of accessible text.
 * @details Implements the efl_access_text_range_extents_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param screen_coords Whether to use screen coordinates.
 * @param start_offset Start offset of the range.
 * @param end_offset End offset of the range.
 * @param[out] rect Pointer to store the range extents.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_range_extents_get
 */
Eina_Bool _elm_entry_efl_access_text_range_extents_get(const Eo *obj, Elm_Entry_Data *pd, Eina_Bool screen_coords, int start_offset, int end_offset, Eina_Rect *rect);

/**
 * @internal
 * @brief Sets an accessible text selection.
 * @details Implements the efl_access_text_access_selection_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param selection_number The selection index.
 * @param start_offset Start offset of the selection.
 * @param end_offset End offset of the selection.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_access_selection_set
 */
Eina_Bool _elm_entry_efl_access_text_access_selection_set(Eo *obj, Elm_Entry_Data *pd, int selection_number, int start_offset, int end_offset);

/**
 * @internal
 * @brief Gets an accessible text selection.
 * @details Implements the efl_access_text_access_selection_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param selection_number The selection index.
 * @param[out] start_offset Pointer to store the start offset.
 * @param[out] end_offset Pointer to store the end offset.
 * @see efl_access_text_access_selection_get
 */
void _elm_entry_efl_access_text_access_selection_get(const Eo *obj, Elm_Entry_Data *pd, int selection_number, int *start_offset, int *end_offset);

/**
 * @internal
 * @brief Gets the count of accessible text selections.
 * @details Implements the efl_access_text_selections_count_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return The number of selections.
 * @see efl_access_text_selections_count_get
 */
int _elm_entry_efl_access_text_selections_count_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Adds an accessible text selection.
 * @details Implements the efl_access_text_selection_add interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start_offset Start offset of the new selection.
 * @param end_offset End offset of the new selection.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_selection_add
 */
Eina_Bool _elm_entry_efl_access_text_selection_add(Eo *obj, Elm_Entry_Data *pd, int start_offset, int end_offset);

/**
 * @internal
 * @brief Removes an accessible text selection.
 * @details Implements the efl_access_text_selection_remove interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param selection_number The index of the selection to remove.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_text_selection_remove
 */
Eina_Bool _elm_entry_efl_access_text_selection_remove(Eo *obj, Elm_Entry_Data *pd, int selection_number);

/**
 * @internal
 * @brief Sets the content of editable accessible text.
 * @details Implements the efl_access_editable_text_content_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param string The new text content.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_content_set
 */
Eina_Bool _elm_entry_efl_access_editable_text_text_content_set(Eo *obj, Elm_Entry_Data *pd, const char *string);

/**
 * @internal
 * @brief Inserts text into editable accessible text.
 * @details Implements the efl_access_editable_text_insert interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param string The text to insert.
 * @param position The position at which to insert.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_insert
 */
Eina_Bool _elm_entry_efl_access_editable_text_insert(Eo *obj, Elm_Entry_Data *pd, const char *string, int position);

/**
 * @internal
 * @brief Copies a range of editable accessible text.
 * @details Implements the efl_access_editable_text_copy interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start Start offset of the range.
 * @param end End offset of the range.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_copy
 */
Eina_Bool _elm_entry_efl_access_editable_text_copy(Eo *obj, Elm_Entry_Data *pd, int start, int end);

/**
 * @internal
 * @brief Cuts a range of editable accessible text.
 * @details Implements the efl_access_editable_text_cut interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start Start offset of the range.
 * @param end End offset of the range.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_cut
 */
Eina_Bool _elm_entry_efl_access_editable_text_cut(Eo *obj, Elm_Entry_Data *pd, int start, int end);

/**
 * @internal
 * @brief Deletes a range of editable accessible text.
 * @details Implements the efl_access_editable_text_delete interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param start Start offset of the range.
 * @param end End offset of the range.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_delete
 */
Eina_Bool _elm_entry_efl_access_editable_text_delete(Eo *obj, Elm_Entry_Data *pd, int start, int end);

/**
 * @internal
 * @brief Pastes text into editable accessible text.
 * @details Implements the efl_access_editable_text_paste interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param position The position at which to paste.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 * @see efl_access_editable_text_paste
 */
Eina_Bool _elm_entry_efl_access_editable_text_paste(Eo *obj, Elm_Entry_Data *pd, int position);

/**
 * @internal
 * @brief Gets the Elementary accessibility actions for the widget.
 * @details Implements the efl_access_widget_action_elm_actions_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return Pointer to an array of Efl_Access_Action_Data.
 * @see efl_access_widget_action_elm_actions_get
 */
const Efl_Access_Action_Data *_elm_entry_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Sets the file associated with the Elm_Entry.
 * @details Implements the efl_file_set interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param file Path to the file.
 * @return EINA_ERROR_NONE on success, or an error code.
 * @see efl_file_set
 */
Eina_Error _elm_entry_efl_file_file_set(Eo *obj, Elm_Entry_Data *pd, const char *file);

/**
 * @internal
 * @brief Loads content from the associated file into the Elm_Entry.
 * @details Implements the efl_file_load interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @return EINA_ERROR_NONE on success, or an error code.
 * @see efl_file_load
 */
Eina_Error _elm_entry_efl_file_load(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Unloads content related to the associated file from the Elm_Entry.
 * @details Implements the efl_file_unload interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @see efl_file_unload
 */
void _elm_entry_efl_file_unload(Eo *obj, Elm_Entry_Data *pd);

/**
 * @internal
 * @brief Gets a part of the Elm_Entry object by name.
 * @details Implements the efl_part_get interface.
 * @param obj The Eo object.
 * @param pd Pointer to the private data of the object.
 * @param name Name of the part.
 * @return The Efl_Object representing the part, or NULL if not found.
 * @see efl_part_get
 */
Efl_Object *_elm_entry_efl_part_part_get(const Eo *obj, Elm_Entry_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Entry class.
 * @details This function is called once when the Elm_Entry class is being set up.
 *          It defines the operations (member functions) for the class, mapping
 *          EAPI function names to their internal implementations (e.g.,
 *          elm_obj_entry_scrollable_set to _elm_entry_scrollable_set).
 *          It also sets up the property reflection table.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_entry_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ENTRY_EXTRA_OPS
#define ELM_ENTRY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_entry_scrollable_set, _elm_entry_scrollable_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_scrollable_get, _elm_entry_scrollable_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_show_on_demand_set, _elm_entry_input_panel_show_on_demand_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_show_on_demand_get, _elm_entry_input_panel_show_on_demand_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_context_menu_disabled_set, _elm_entry_context_menu_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_context_menu_disabled_get, _elm_entry_context_menu_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cnp_mode_set, _elm_entry_cnp_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cnp_mode_get, _elm_entry_cnp_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_file_text_format_set, _elm_entry_file_text_format_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_language_set, _elm_entry_input_panel_language_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_language_get, _elm_entry_input_panel_language_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_handler_disabled_set, _elm_entry_selection_handler_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_handler_disabled_get, _elm_entry_selection_handler_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_layout_variation_set, _elm_entry_input_panel_layout_variation_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_layout_variation_get, _elm_entry_input_panel_layout_variation_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_autocapital_type_set, _elm_entry_autocapital_type_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_autocapital_type_get, _elm_entry_autocapital_type_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_editable_set, _elm_entry_editable_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_editable_get, _elm_entry_editable_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_anchor_hover_style_set, _elm_entry_anchor_hover_style_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_anchor_hover_style_get, _elm_entry_anchor_hover_style_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_single_line_set, _elm_entry_single_line_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_single_line_get, _elm_entry_single_line_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_password_set, _elm_entry_password_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_password_get, _elm_entry_password_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_return_key_disabled_set, _elm_entry_input_panel_return_key_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_return_key_disabled_get, _elm_entry_input_panel_return_key_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_autosave_set, _elm_entry_autosave_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_autosave_get, _elm_entry_autosave_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_anchor_hover_parent_set, _elm_entry_anchor_hover_parent_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_anchor_hover_parent_get, _elm_entry_anchor_hover_parent_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_prediction_allow_set, _elm_entry_prediction_allow_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_prediction_allow_get, _elm_entry_prediction_allow_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_hint_set, _elm_entry_input_hint_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_hint_get, _elm_entry_input_hint_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_layout_set, _elm_entry_input_panel_layout_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_layout_get, _elm_entry_input_panel_layout_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_return_key_type_set, _elm_entry_input_panel_return_key_type_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_return_key_type_get, _elm_entry_input_panel_return_key_type_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_enabled_set, _elm_entry_input_panel_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_enabled_get, _elm_entry_input_panel_enabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_line_wrap_set, _elm_entry_line_wrap_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_line_wrap_get, _elm_entry_line_wrap_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_pos_set, _elm_entry_cursor_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_pos_get, _elm_entry_cursor_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_icon_visible_set, _elm_entry_icon_visible_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_line_end_set, _elm_entry_cursor_line_end_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_region_set, _elm_entry_select_region_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_region_get, _elm_entry_select_region_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_return_key_autoenabled_set, _elm_entry_input_panel_return_key_autoenabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_end_visible_set, _elm_entry_end_visible_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_begin_set, _elm_entry_cursor_begin_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_line_begin_set, _elm_entry_cursor_line_begin_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_end_set, _elm_entry_cursor_end_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_textblock_get, _elm_entry_textblock_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_textblock_cursor_geometry_get, _elm_entry_textblock_cursor_geometry_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_imf_context_get, _elm_entry_imf_context_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_is_format_get, _elm_entry_cursor_is_format_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_textblock_cursor_content_get, _elm_entry_textblock_cursor_content_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_get, _elm_entry_selection_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_is_visible_format_get, _elm_entry_cursor_is_visible_format_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_allow_set, _elm_entry_select_allow_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_allow_get, _elm_entry_select_allow_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_prev, _elm_entry_cursor_prev),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_text_style_user_pop, _elm_entry_text_style_user_pop),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_item_provider_prepend, _elm_entry_item_provider_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_show, _elm_entry_input_panel_show),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_imf_context_reset, _elm_entry_imf_context_reset),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_anchor_hover_end, _elm_entry_anchor_hover_end),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_selection_begin, _elm_entry_cursor_selection_begin),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_down, _elm_entry_cursor_down),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_file_save, _elm_entry_file_save),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_copy, _elm_entry_selection_copy),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_text_style_user_push, _elm_entry_text_style_user_push),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_item_provider_remove, _elm_entry_item_provider_remove),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_text_style_user_peek, _elm_entry_text_style_user_peek),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_context_menu_clear, _elm_entry_context_menu_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_up, _elm_entry_cursor_up),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_insert, _elm_entry_entry_insert),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_imdata_set, _elm_entry_input_panel_imdata_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_imdata_get, _elm_entry_input_panel_imdata_get),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_paste, _elm_entry_selection_paste),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_next, _elm_entry_cursor_next),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_none, _elm_entry_select_none),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_input_panel_hide, _elm_entry_input_panel_hide),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_select_all, _elm_entry_select_all),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_cursor_selection_end, _elm_entry_cursor_selection_end),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_selection_cut, _elm_entry_selection_cut),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_is_empty, _elm_entry_is_empty),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_markup_filter_remove, _elm_entry_markup_filter_remove),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_item_provider_append, _elm_entry_item_provider_append),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_markup_filter_append, _elm_entry_markup_filter_append),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_append, _elm_entry_entry_append),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_context_menu_item_add, _elm_entry_context_menu_item_add),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_markup_filter_prepend, _elm_entry_markup_filter_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_prediction_hint_set, _elm_entry_prediction_hint_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_prediction_hint_hash_set, _elm_entry_prediction_hint_hash_set),
      EFL_OBJECT_OP_FUNC(elm_obj_entry_prediction_hint_hash_del, _elm_entry_prediction_hint_hash_del),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_entry_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_entry_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_entry_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_entry_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_entry_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_layout_signal_callback_add, _elm_entry_efl_layout_signal_signal_callback_add),
      EFL_OBJECT_OP_FUNC(efl_layout_signal_callback_del, _elm_entry_efl_layout_signal_signal_callback_del),
      EFL_OBJECT_OP_FUNC(efl_layout_signal_emit, _elm_entry_efl_layout_signal_signal_emit),
      EFL_OBJECT_OP_FUNC(efl_layout_calc_force, _elm_entry_efl_layout_calc_calc_force),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_activate, _elm_entry_efl_ui_widget_on_access_activate),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_entry_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_manager_create, _elm_entry_efl_ui_widget_focus_manager_focus_manager_create),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_entry_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_interest_region_get, _elm_entry_efl_ui_widget_interest_region_get),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_disabled_set, _elm_entry_efl_ui_widget_disabled_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_entry_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_policy_set, _elm_entry_elm_interface_scrollable_policy_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_bounce_allow_set, _elm_entry_elm_interface_scrollable_bounce_allow_set),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_entry_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_entry_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_get, _elm_entry_efl_access_text_access_text_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_string_get, _elm_entry_efl_access_text_string_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_attribute_get, _elm_entry_efl_access_text_attribute_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_attributes_get, _elm_entry_efl_access_text_text_attributes_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_default_attributes_get, _elm_entry_efl_access_text_default_attributes_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_caret_offset_set, _elm_entry_efl_access_text_caret_offset_set),
      EFL_OBJECT_OP_FUNC(efl_access_text_caret_offset_get, _elm_entry_efl_access_text_caret_offset_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_character_get, _elm_entry_efl_access_text_character_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_character_extents_get, _elm_entry_efl_access_text_character_extents_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_character_count_get, _elm_entry_efl_access_text_character_count_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_offset_at_point_get, _elm_entry_efl_access_text_offset_at_point_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_bounded_ranges_get, _elm_entry_efl_access_text_bounded_ranges_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_range_extents_get, _elm_entry_efl_access_text_range_extents_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_access_selection_set, _elm_entry_efl_access_text_access_selection_set),
      EFL_OBJECT_OP_FUNC(efl_access_text_access_selection_get, _elm_entry_efl_access_text_access_selection_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_selections_count_get, _elm_entry_efl_access_text_selections_count_get),
      EFL_OBJECT_OP_FUNC(efl_access_text_selection_add, _elm_entry_efl_access_text_selection_add),
      EFL_OBJECT_OP_FUNC(efl_access_text_selection_remove, _elm_entry_efl_access_text_selection_remove),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_content_set, _elm_entry_efl_access_editable_text_text_content_set),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_insert, _elm_entry_efl_access_editable_text_insert),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_copy, _elm_entry_efl_access_editable_text_copy),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_cut, _elm_entry_efl_access_editable_text_cut),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_delete, _elm_entry_efl_access_editable_text_delete),
      EFL_OBJECT_OP_FUNC(efl_access_editable_text_paste, _elm_entry_efl_access_editable_text_paste),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_entry_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_file_set, _elm_entry_efl_file_file_set),
      EFL_OBJECT_OP_FUNC(efl_file_load, _elm_entry_efl_file_load),
      EFL_OBJECT_OP_FUNC(efl_file_unload, _elm_entry_efl_file_unload),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_entry_efl_part_part_get),
      ELM_ENTRY_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"scrollable", __eolian_elm_entry_scrollable_set_reflect, __eolian_elm_entry_scrollable_get_reflect},
      {"input_panel_show_on_demand", __eolian_elm_entry_input_panel_show_on_demand_set_reflect, __eolian_elm_entry_input_panel_show_on_demand_get_reflect},
      {"context_menu_disabled", __eolian_elm_entry_context_menu_disabled_set_reflect, __eolian_elm_entry_context_menu_disabled_get_reflect},
      {"selection_handler_disabled", __eolian_elm_entry_selection_handler_disabled_set_reflect, __eolian_elm_entry_selection_handler_disabled_get_reflect},
      {"input_panel_layout_variation", __eolian_elm_entry_input_panel_layout_variation_set_reflect, __eolian_elm_entry_input_panel_layout_variation_get_reflect},
      {"editable", __eolian_elm_entry_editable_set_reflect, __eolian_elm_entry_editable_get_reflect},
      {"anchor_hover_style", __eolian_elm_entry_anchor_hover_style_set_reflect, __eolian_elm_entry_anchor_hover_style_get_reflect},
      {"single_line", __eolian_elm_entry_single_line_set_reflect, __eolian_elm_entry_single_line_get_reflect},
      {"password", __eolian_elm_entry_password_set_reflect, __eolian_elm_entry_password_get_reflect},
      {"input_panel_return_key_disabled", __eolian_elm_entry_input_panel_return_key_disabled_set_reflect, __eolian_elm_entry_input_panel_return_key_disabled_get_reflect},
      {"autosave", __eolian_elm_entry_autosave_set_reflect, __eolian_elm_entry_autosave_get_reflect},
      {"prediction_allow", __eolian_elm_entry_prediction_allow_set_reflect, __eolian_elm_entry_prediction_allow_get_reflect},
      {"input_panel_enabled", __eolian_elm_entry_input_panel_enabled_set_reflect, __eolian_elm_entry_input_panel_enabled_get_reflect},
      {"cursor_pos", __eolian_elm_entry_cursor_pos_set_reflect, __eolian_elm_entry_cursor_pos_get_reflect},
      {"icon_visible", __eolian_elm_entry_icon_visible_set_reflect, NULL},
      {"input_panel_return_key_autoenabled", __eolian_elm_entry_input_panel_return_key_autoenabled_set_reflect, NULL},
      {"end_visible", __eolian_elm_entry_end_visible_set_reflect, NULL},
      {"select_allow", __eolian_elm_entry_select_allow_set_reflect, __eolian_elm_entry_select_allow_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Class description for the Elm.Entry Eo class.
 * @details Specifies metadata for the Elm.Entry class, including its version,
 *          name, type, instance data size, and pointers to initializer,
 *          constructor, and destructor functions.
 */
static const Efl_Class_Description _elm_entry_class_desc = {
   EO_VERSION,
   "Elm.Entry",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Entry_Data),
   _elm_entry_class_initializer,
   _elm_entry_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_entry_class_get, &_elm_entry_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_ACCESS_TEXT_INTERFACE, EFL_ACCESS_EDITABLE_TEXT_INTERFACE, EFL_FILE_MIXIN, EFL_UI_SCROLLABLE_INTERFACE, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_entry_eo.legacy.c"
