/**
 * @file
 * @brief Implementation of the Elm_Web_None class (no-op web engine).
 *
 * This file contains the no-op implementations for the Elm_Web interface methods.
 * These functions generally do nothing or return default/empty values,
 * fulfilling the API contract when a real web engine is not present.
 */

/**
 * @internal
 * @brief Sets whether the tab is propagated. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param propagate EINA_TRUE to propagate the tab, EINA_FALSE otherwise.
 */
void _elm_web_none_elm_web_tab_propagate_set(Eo *obj, Elm_Web_None_Data *pd, Eina_Bool propagate);


/**
 * @internal
 * @brief Gets whether the tab is propagated. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_tab_propagate_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Gets the webkit view object. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return NULL always.
 */
Efl_Canvas_Object *_elm_web_none_elm_web_webkit_view_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the window creation hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when a window needs to be created.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_window_create_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Window_Open func, void *data);

/**
 * @internal
 * @brief Sets the alert dialog hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when an alert dialog is triggered.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_dialog_alert_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Dialog_Alert func, void *data);

/**
 * @internal
 * @brief Sets the confirm dialog hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when a confirm dialog is triggered.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_dialog_confirm_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Dialog_Confirm func, void *data);

/**
 * @internal
 * @brief Sets the prompt dialog hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when a prompt dialog is triggered.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_dialog_prompt_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Dialog_Prompt func, void *data);

/**
 * @internal
 * @brief Sets the file selector dialog hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when a file selector dialog is triggered.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_dialog_file_selector_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Dialog_File_Selector func, void *data);

/**
 * @internal
 * @brief Sets the console message hook. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param func The function to call when a console message is available.
 * @param data User data to pass to @p func.
 */
void _elm_web_none_elm_web_console_message_hook_set(Eo *obj, Elm_Web_None_Data *pd, Elm_Web_Console_Message func, void *data);

/**
 * @internal
 * @brief Sets the user agent string. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param user_agent The user agent string.
 */
void _elm_web_none_elm_web_useragent_set(Eo *obj, Elm_Web_None_Data *pd, const char *user_agent);

/**
 * @internal
 * @brief Gets the user agent string. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return NULL always.
 */
const char *_elm_web_none_elm_web_useragent_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the current URL. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param url The URL to set.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_url_set(Eo *obj, Elm_Web_None_Data *pd, const char *url);

/**
 * @internal
 * @brief Gets the current URL. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return NULL always.
 */
const char *_elm_web_none_elm_web_url_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Loads an HTML string. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param html The HTML string to load.
 * @param base_url The base URL for relative resources.
 * @param unreachable_url The URL to load if @p base_url is unreachable.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_html_string_load(Eo *obj, Elm_Web_None_Data *pd, const char *html, const char *base_url, const char *unreachable_url);

/**
 * @internal
 * @brief Gets the page title. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return NULL always.
 */
Eina_Stringshare *_elm_web_none_elm_web_title_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the background color. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
void _elm_web_none_elm_web_bg_color_set(Eo *obj, Elm_Web_None_Data *pd, int r, int g, int b, int a);

/**
 * @internal
 * @brief Gets the background color. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param r Pointer to store the red component.
 * @param g Pointer to store the green component.
 * @param b Pointer to store the blue component.
 * @param a Pointer to store the alpha component.
 *          (All are set to 0).
 */
void _elm_web_none_elm_web_bg_color_get(const Eo *obj, Elm_Web_None_Data *pd, int *r, int *g, int *b, int *a);

/**
 * @internal
 * @brief Gets the current text selection. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return NULL always.
 */
char *_elm_web_none_elm_web_selection_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the selected item in a popup/select element. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param idx The index of the item to select.
 */
void _elm_web_none_elm_web_popup_selected_set(Eo *obj, Elm_Web_None_Data *pd, int idx);

/**
 * @internal
 * @brief Destroys the current popup. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_popup_destroy(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Searches for text in the page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param string The text to search for.
 * @param case_sensitive Whether the search is case sensitive.
 * @param forward Whether to search forward.
 * @param wrap Whether to wrap around the document.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_text_search(const Eo *obj, Elm_Web_None_Data *pd, const char *string, Eina_Bool case_sensitive, Eina_Bool forward, Eina_Bool wrap);

/**
 * @internal
 * @brief Marks text matches in the page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param string The text to mark.
 * @param case_sensitive Whether the matching is case sensitive.
 * @param highlight Whether to highlight the matches.
 * @param limit The maximum number of matches to mark.
 * @return 0 always.
 */
unsigned int _elm_web_none_elm_web_text_matches_mark(Eo *obj, Elm_Web_None_Data *pd, const char *string, Eina_Bool case_sensitive, Eina_Bool highlight, unsigned int limit);

/**
 * @internal
 * @brief Unmarks all text matches. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_text_matches_unmark_all(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets whether text matches are highlighted. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param highlight EINA_TRUE to highlight, EINA_FALSE otherwise.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_text_matches_highlight_set(Eo *obj, Elm_Web_None_Data *pd, Eina_Bool highlight);

/**
 * @internal
 * @brief Gets whether text matches are highlighted. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_text_matches_highlight_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Gets the page load progress. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return 0.0 always.
 */
double _elm_web_none_elm_web_load_progress_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Stops loading the current page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_stop(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Reloads the current page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_reload(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Reloads the current page, bypassing caches. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_reload_full(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Navigates back in history. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_back(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Navigates forward in history. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_forward(Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Navigates a number of steps in history. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param steps Number of steps to navigate (positive for forward, negative for backward).
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_navigate(Eo *obj, Elm_Web_None_Data *pd, int steps);

/**
 * @internal
 * @brief Checks if navigation back is possible. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_back_possible_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Checks if navigation forward is possible. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_forward_possible_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Checks if navigation by a number of steps is possible. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param steps Number of steps to check.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_navigate_possible_get(Eo *obj, Elm_Web_None_Data *pd, int steps);

/**
 * @internal
 * @brief Sets whether history is enabled. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param enable EINA_TRUE to enable history, EINA_FALSE otherwise.
 */
void _elm_web_none_elm_web_history_enabled_set(Eo *obj, Elm_Web_None_Data *pd, Eina_Bool enable);

/**
 * @internal
 * @brief Gets whether history is enabled. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_history_enabled_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the zoom level. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param zoom The zoom level.
 */
void _elm_web_none_efl_ui_zoom_zoom_level_set(Eo *obj, Elm_Web_None_Data *pd, double zoom);

/**
 * @internal
 * @brief Gets the zoom level. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return 1.0 always (default zoom level).
 */
double _elm_web_none_efl_ui_zoom_zoom_level_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Sets the zoom mode. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param mode The zoom mode.
 */
void _elm_web_none_efl_ui_zoom_zoom_mode_set(Eo *obj, Elm_Web_None_Data *pd, Efl_Ui_Zoom_Mode mode);

/**
 * @internal
 * @brief Gets the zoom mode. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EFL_UI_ZOOM_MODE_MANUAL always.
 */
Efl_Ui_Zoom_Mode _elm_web_none_efl_ui_zoom_zoom_mode_get(const Eo *obj, Elm_Web_None_Data *pd);

/**
 * @internal
 * @brief Shows a specific region of the page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 */
void _elm_web_none_elm_web_region_show(Eo *obj, Elm_Web_None_Data *pd, int x, int y, int w, int h);

/**
 * @internal
 * @brief Brings in a specific region of the page. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 */
void _elm_web_none_elm_web_region_bring_in(Eo *obj, Elm_Web_None_Data *pd, int x, int y, int w, int h);

/**
 * @internal
 * @brief Sets the in-window mode. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @param value EINA_TRUE for in-window mode, EINA_FALSE otherwise.
 */
void _elm_web_none_elm_web_inwin_mode_set(Eo *obj, Elm_Web_None_Data *pd, Eina_Bool value);

/**
 * @internal
 * @brief Gets the in-window mode. (No-op)
 * @param obj The Evas object.
 * @param pd Private data for the object.
 * @return EINA_FALSE always.
 */
Eina_Bool _elm_web_none_elm_web_inwin_mode_get(const Eo *obj, Elm_Web_None_Data *pd);


/**
 * @internal
 * @brief Class initializer for Elm_Web_None.
 *
 * This function sets up the operations for the Elm_Web_None class.
 * It maps the Elm_Web interface methods to their no-op implementations.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_web_none_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_WEB_NONE_EXTRA_OPS
#define ELM_WEB_NONE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_web_tab_propagate_set, _elm_web_none_elm_web_tab_propagate_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_tab_propagate_get, _elm_web_none_elm_web_tab_propagate_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_webkit_view_get, _elm_web_none_elm_web_webkit_view_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_window_create_hook_set, _elm_web_none_elm_web_window_create_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_alert_hook_set, _elm_web_none_elm_web_dialog_alert_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_confirm_hook_set, _elm_web_none_elm_web_dialog_confirm_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_prompt_hook_set, _elm_web_none_elm_web_dialog_prompt_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_dialog_file_selector_hook_set, _elm_web_none_elm_web_dialog_file_selector_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_console_message_hook_set, _elm_web_none_elm_web_console_message_hook_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_useragent_set, _elm_web_none_elm_web_useragent_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_useragent_get, _elm_web_none_elm_web_useragent_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_url_set, _elm_web_none_elm_web_url_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_url_get, _elm_web_none_elm_web_url_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_html_string_load, _elm_web_none_elm_web_html_string_load),
      EFL_OBJECT_OP_FUNC(elm_obj_web_title_get, _elm_web_none_elm_web_title_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_bg_color_set, _elm_web_none_elm_web_bg_color_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_bg_color_get, _elm_web_none_elm_web_bg_color_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_selection_get, _elm_web_none_elm_web_selection_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_popup_selected_set, _elm_web_none_elm_web_popup_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_popup_destroy, _elm_web_none_elm_web_popup_destroy),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_search, _elm_web_none_elm_web_text_search),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_mark, _elm_web_none_elm_web_text_matches_mark),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_unmark_all, _elm_web_none_elm_web_text_matches_unmark_all),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_highlight_set, _elm_web_none_elm_web_text_matches_highlight_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_text_matches_highlight_get, _elm_web_none_elm_web_text_matches_highlight_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_load_progress_get, _elm_web_none_elm_web_load_progress_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_stop, _elm_web_none_elm_web_stop),
      EFL_OBJECT_OP_FUNC(elm_obj_web_reload, _elm_web_none_elm_web_reload),
      EFL_OBJECT_OP_FUNC(elm_obj_web_reload_full, _elm_web_none_elm_web_reload_full),
      EFL_OBJECT_OP_FUNC(elm_obj_web_back, _elm_web_none_elm_web_back),
      EFL_OBJECT_OP_FUNC(elm_obj_web_forward, _elm_web_none_elm_web_forward),
      EFL_OBJECT_OP_FUNC(elm_obj_web_navigate, _elm_web_none_elm_web_navigate),
      EFL_OBJECT_OP_FUNC(elm_obj_web_back_possible_get, _elm_web_none_elm_web_back_possible_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_forward_possible_get, _elm_web_none_elm_web_forward_possible_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_navigate_possible_get, _elm_web_none_elm_web_navigate_possible_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_history_enabled_set, _elm_web_none_elm_web_history_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_history_enabled_get, _elm_web_none_elm_web_history_enabled_get),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_level_set, _elm_web_none_efl_ui_zoom_zoom_level_set),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_level_get, _elm_web_none_efl_ui_zoom_zoom_level_get),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_mode_set, _elm_web_none_efl_ui_zoom_zoom_mode_set),
      EFL_OBJECT_OP_FUNC(efl_ui_zoom_mode_get, _elm_web_none_efl_ui_zoom_zoom_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_web_region_show, _elm_web_none_elm_web_region_show),
      EFL_OBJECT_OP_FUNC(elm_obj_web_region_bring_in, _elm_web_none_elm_web_region_bring_in),
      EFL_OBJECT_OP_FUNC(elm_obj_web_inwin_mode_set, _elm_web_none_elm_web_inwin_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_web_inwin_mode_get, _elm_web_none_elm_web_inwin_mode_get),
      ELM_WEB_NONE_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Class description for Elm_Web_None.
 *
 * This structure provides metadata for the Elm_Web_None class,
 * including its name, type, data size, and initializer functions.
 */
static const Efl_Class_Description _elm_web_none_class_desc = {
   EO_VERSION,
   "Elm.Web.None",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Web_None_Data),
   _elm_web_none_class_initializer,
   NULL,
   NULL
};

/**
 * @internal
 * @brief Defines the Elm_Web_None class.
 *
 * This macro utilizes the Efl_Class_Description to formally define
 * the Elm_Web_None class, making it available to the Efl object system.
 * It inherits from ELM_WEB_CLASS and implements EFL_UI_LEGACY_INTERFACE.
 */
EFL_DEFINE_CLASS(elm_web_none_class_get, &_elm_web_none_class_desc, ELM_WEB_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);
