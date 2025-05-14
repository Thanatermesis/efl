/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_text_matches_highlight_set property.
 *
 * This function is called when the "text_matches_highlight" property is set via Eolian.
 * It converts the Eina_Value to a Eina_Bool and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_text_matches_highlight_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_text_matches_highlight_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_FUNC_BODYV(elm_obj_web_text_matches_highlight_set, Eina_Bool, 0, EFL_FUNC_CALL(highlight), Eina_Bool highlight);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_text_matches_highlight_get property.
 *
 * This function is called when the "text_matches_highlight" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned Eina_Bool to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state of text matches highlighting.
 */
static Eina_Value
__eolian_elm_web_text_matches_highlight_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_web_text_matches_highlight_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_text_matches_highlight_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_useragent_set property.
 *
 * This function is called when the "useragent" property is set via Eolian.
 * It converts the Eina_Value (string) to a const char* and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the useragent string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_useragent_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_useragent_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_useragent_set, EFL_FUNC_CALL(user_agent), const char *user_agent);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_useragent_get property.
 *
 * This function is called when the "useragent" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned const char* to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the useragent string.
 */
static Eina_Value
__eolian_elm_web_useragent_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_web_useragent_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_useragent_get, const char *, NULL);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_url_set property.
 *
 * This function is called when the "url" property is set via Eolian.
 * It converts the Eina_Value (string) to a const char* and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the URL string.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_url_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_url_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_FUNC_BODYV(elm_obj_web_url_set, Eina_Bool, 0, EFL_FUNC_CALL(url), const char *url);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_url_get property.
 *
 * This function is called when the "url" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned const char* to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the URL string.
 */
static Eina_Value
__eolian_elm_web_url_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_web_url_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_url_get, const char *, NULL);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_bg_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);
EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_web_bg_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_inwin_mode_set property.
 *
 * This function is called when the "inwin_mode" property is set via Eolian.
 * It converts the Eina_Value (boolean) to a Eina_Bool and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean value for inwin mode.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_inwin_mode_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_inwin_mode_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_inwin_mode_set, EFL_FUNC_CALL(value), Eina_Bool value);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_inwin_mode_get property.
 *
 * This function is called when the "inwin_mode" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned Eina_Bool to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state of inwin mode.
 */
static Eina_Value
__eolian_elm_web_inwin_mode_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_web_inwin_mode_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_inwin_mode_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_tab_propagate_set property.
 *
 * This function is called when the "tab_propagate" property is set via Eolian.
 * It converts the Eina_Value (boolean) to a Eina_Bool and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean value for tab propagation.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_tab_propagate_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_tab_propagate_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_tab_propagate_set, EFL_FUNC_CALL(propagate), Eina_Bool propagate);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_tab_propagate_get property.
 *
 * This function is called when the "tab_propagate" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned Eina_Bool to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state of tab propagation.
 */
static Eina_Value
__eolian_elm_web_tab_propagate_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_web_tab_propagate_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_tab_propagate_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_history_enabled_set property.
 *
 * This function is called when the "history_enabled" property is set via Eolian.
 * It converts the Eina_Value (boolean) to a Eina_Bool and calls the underlying C implementation.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean value for history enabled state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_history_enabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_history_enabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_history_enabled_set, EFL_FUNC_CALL(enable), Eina_Bool enable);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_history_enabled_get property.
 *
 * This function is called when the "history_enabled" property is read via Eolian.
 * It calls the underlying C implementation and converts the returned Eina_Bool to an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state of history enabled.
 */
static Eina_Value
__eolian_elm_web_history_enabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_web_history_enabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_history_enabled_get, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_console_message_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Console_Message func, void *data);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_window_create_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Window_Open func, void *data);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_dialog_file_selector_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Dialog_File_Selector func, void *data);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_dialog_confirm_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Dialog_Confirm func, void *data);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_web_popup_selected_set property.
 *
 * This function is called when the "popup_selected" property is set via Eolian.
 * It converts the Eina_Value (integer) to an int and calls the underlying C implementation.
 * Note: This property is write-only from Eolian perspective, so there is no corresponding _get_reflect.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the integer index of the selected popup item.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_web_popup_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_web_popup_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_popup_selected_set, EFL_FUNC_CALL(idx), int idx);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_dialog_prompt_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Dialog_Prompt func, void *data);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_dialog_alert_hook_set, EFL_FUNC_CALL(func, data), Elm_Web_Dialog_Alert func, void *data);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_forward_possible_get, Eina_Bool, 0);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_webkit_view_get, Efl_Canvas_Object *, NULL);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_back_possible_get, Eina_Bool, 0);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_load_progress_get, double, 0);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_selection_get, char *, NULL);
EOAPI EFL_FUNC_BODY_CONST(elm_obj_web_title_get, Eina_Stringshare *, NULL);
EOAPI EFL_FUNC_BODYV(elm_obj_web_navigate, Eina_Bool, 0, EFL_FUNC_CALL(steps), int steps);
EOAPI EFL_FUNC_BODY(elm_obj_web_back, Eina_Bool, 0);
EOAPI EFL_FUNC_BODYV(elm_obj_web_html_string_load, Eina_Bool, 0, EFL_FUNC_CALL(html, base_url, unreachable_url), const char *html, const char *base_url, const char *unreachable_url);
EOAPI EFL_FUNC_BODYV_CONST(elm_obj_web_text_search, Eina_Bool, 0, EFL_FUNC_CALL(string, case_sensitive, forward, wrap), const char *string, Eina_Bool case_sensitive, Eina_Bool forward, Eina_Bool wrap);
EOAPI EFL_FUNC_BODY(elm_obj_web_popup_destroy, Eina_Bool, 0);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_region_show, EFL_FUNC_CALL(x, y, w, h), int x, int y, int w, int h);
EOAPI EFL_FUNC_BODY(elm_obj_web_forward, Eina_Bool, 0);
EOAPI EFL_FUNC_BODYV(elm_obj_web_text_matches_mark, unsigned int, 0, EFL_FUNC_CALL(string, case_sensitive, highlight, limit), const char *string, Eina_Bool case_sensitive, Eina_Bool highlight, unsigned int limit);
EOAPI EFL_VOID_FUNC_BODYV(elm_obj_web_region_bring_in, EFL_FUNC_CALL(x, y, w, h), int x, int y, int w, int h);
EOAPI EFL_FUNC_BODY(elm_obj_web_stop, Eina_Bool, 0);
EOAPI EFL_FUNC_BODYV(elm_obj_web_navigate_possible_get, Eina_Bool, 0, EFL_FUNC_CALL(steps), int steps);
EOAPI EFL_FUNC_BODY(elm_obj_web_reload_full, Eina_Bool, 0);
EOAPI EFL_FUNC_BODY(elm_obj_web_text_matches_unmark_all, Eina_Bool, 0);
EOAPI EFL_FUNC_BODY(elm_obj_web_reload, Eina_Bool, 0);

Efl_Object *_elm_web_efl_object_constructor(Eo *obj, Elm_Web_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Web Efl_Class.
 *
 * This function is called once when the Elm_Web class is being set up.
 * It defines the Eolian operations (methods) and property reflection
 * capabilities for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_web_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_WEB_EXTRA_OPS
#define ELM_WEB_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_highlight_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_highlight_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_useragent_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_useragent_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_url_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_url_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_bg_color_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_bg_color_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_inwin_mode_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_inwin_mode_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_tab_propagate_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_tab_propagate_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_history_enabled_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_history_enabled_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_console_message_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_window_create_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_file_selector_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_confirm_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_popup_selected_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_prompt_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_alert_hook_set, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_forward_possible_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_webkit_view_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_back_possible_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_load_progress_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_selection_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_title_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_navigate, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_back, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_html_string_load, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_search, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_popup_destroy, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_region_show, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_forward, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_mark, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_region_bring_in, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_stop, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_navigate_possible_get, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_reload_full, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_unmark_all, NULL),
      EFL_OBJECT_OP_FUNC(elm_obj_web_reload, NULL),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_web_efl_object_constructor),
      ELM_WEB_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"text_matches_highlight", __eolian_elm_web_text_matches_highlight_set_reflect, __eolian_elm_web_text_matches_highlight_get_reflect},
      {"useragent", __eolian_elm_web_useragent_set_reflect, __eolian_elm_web_useragent_get_reflect},
      {"url", __eolian_elm_web_url_set_reflect, __eolian_elm_web_url_get_reflect},
      {"inwin_mode", __eolian_elm_web_inwin_mode_set_reflect, __eolian_elm_web_inwin_mode_get_reflect},
      {"tab_propagate", __eolian_elm_web_tab_propagate_set_reflect, __eolian_elm_web_tab_propagate_get_reflect},
      {"history_enabled", __eolian_elm_web_history_enabled_set_reflect, __eolian_elm_web_history_enabled_get_reflect},
      {"popup_selected", __eolian_elm_web_popup_selected_set_reflect, NULL},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_web_class_desc = {
   EO_VERSION,
   "Elm.Web",
   EFL_CLASS_TYPE_REGULAR_NO_INSTANT,
   sizeof(Elm_Web_Data),
   _elm_web_class_initializer,
   _elm_web_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_web_class_get, &_elm_web_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_LEGACY_INTERFACE, EFL_UI_ZOOM_INTERFACE, NULL);

#include "elm_web_eo.legacy.c"
