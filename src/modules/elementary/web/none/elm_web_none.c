#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED

#include "Elementary.h"
#include "elm_module_helper.h"
#include "elm_priv.h"
#include "elm_widget_web.h"

#ifndef EFL_BUILD
# define EFL_BUILD
#endif
#undef ELM_MODULE_HELPER_H
#include "elm_module_helper.h"
#include "elm_web_none_eo.h"

#define MY_CLASS ELM_WEB_CLASS

#define MY_CLASS_NAME "Elm_Web_None"

/**
 * @internal
 * @brief Private data for the Elm_Web_None class.
 *
 * This structure is empty as the "none" web engine implementation
 * does not require any specific data.
 */
typedef struct _Elm_Web_None_Data Elm_Web_None_Data;
struct _Elm_Web_None_Data
{
};

/**
 * @internal
 * @brief Log domain for the 'none' web engine backend.
 *
 * This is initialized to -1 and registered in @ref ewm_need_web.
 */
static int _none_log_dom = -1;

#undef CRI
#undef ERR
#undef WRN
#undef INF
#undef DBG
#define CRI(...)      EINA_LOG_DOM_CRIT(_none_log_dom, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_none_log_dom, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_none_log_dom, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_none_log_dom, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_none_log_dom, __VA_ARGS__)

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_tab_propagate_get.
 * @return Always @c EINA_FALSE as there is no web content.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_tab_propagate_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_tab_propagate_set.
 *
 * This is a no-op as there is no web engine functionality.
 */
EOLIAN static void
_elm_web_none_elm_web_tab_propagate_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Eina_Bool propagate EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Overridden group_add method to display a message.
 *
 * When the web widget is created, this function adds a label to it
 * indicating that WebKit is not supported. This provides a clear message
 * to the user about the lack of web functionality.
 */
EOLIAN static void
_elm_web_none_efl_canvas_group_group_add(Eo *obj, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   Evas_Object *resize_obj;

   resize_obj = elm_label_add(obj);
   elm_object_text_set(resize_obj, "WebKit not supported!");
   elm_widget_resize_object_set(obj, resize_obj);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_webkit_view_get.
 * @return Always @c NULL as there is no EWebKit view.
 */
EOLIAN static Evas_Object*
_elm_web_none_elm_web_webkit_view_get(const Eo *obj, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   (void)obj;
   ERR("Elementary not compiled with EWebKit support.");
   return NULL;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_window_create_hook_set.
 *
 * This is a no-op as no new windows can be created.
 */
EOLIAN static void
_elm_web_none_elm_web_window_create_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Elm_Web_Window_Open func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_dialog_alert_hook_set.
 *
 * This is a no-op as no dialogs will be shown.
 */
EOLIAN static void
_elm_web_none_elm_web_dialog_alert_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Elm_Web_Dialog_Alert func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_dialog_confirm_hook_set.
 *
 * This is a no-op as no dialogs will be shown.
 */
EOLIAN static void
_elm_web_none_elm_web_dialog_confirm_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Elm_Web_Dialog_Confirm func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_dialog_prompt_hook_set.
 *
 * This is a no-op as no dialogs will be shown.
 */
EOLIAN static void
_elm_web_none_elm_web_dialog_prompt_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Elm_Web_Dialog_Prompt func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_dialog_file_selector_hook_set.
 *
 * This is a no-op as no dialogs will be shown.
 */
EOLIAN static void
_elm_web_none_elm_web_dialog_file_selector_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, Elm_Web_Dialog_File_Selector func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_console_message_hook_set.
 *
 * This is a no-op as there is no web console.
 */
EOLIAN static void
_elm_web_none_elm_web_console_message_hook_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, Elm_Web_Console_Message func EINA_UNUSED, void *data EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_useragent_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_useragent_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, const char *user_agent EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_useragent_get.
 * @return Always @c NULL.
 */
EOLIAN static const char*
_elm_web_none_elm_web_useragent_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_url_set.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_url_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, const char *url EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_url_get.
 * @return Always @c NULL.
 */
EOLIAN static const char*
_elm_web_none_elm_web_url_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_html_string_load.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_html_string_load(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, const char *html EINA_UNUSED, const char *base_url EINA_UNUSED, const char *unreachable_url EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_title_get.
 * @return Always @c NULL.
 */
EOLIAN static const char*
_elm_web_none_elm_web_title_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_bg_color_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_bg_color_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int r EINA_UNUSED, int g EINA_UNUSED, int b EINA_UNUSED, int a EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_bg_color_get.
 *
 * This function sets the color components to 0.
 */
EOLIAN static void
_elm_web_none_elm_web_bg_color_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int *r, int *g, int *b, int *a)
{
   if (r) *r = 0;
   if (g) *g = 0;
   if (b) *b = 0;
   if (a) *a = 0;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_selection_get.
 * @return Always @c NULL.
 */
EOLIAN static char*
_elm_web_none_elm_web_selection_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_popup_selected_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_popup_selected_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int idx EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_popup_destroy.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_popup_destroy(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_text_search.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_text_search(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, const char *string EINA_UNUSED, Eina_Bool case_sensitive EINA_UNUSED, Eina_Bool forward EINA_UNUSED, Eina_Bool wrap EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_text_matches_mark.
 * @return Always 0.
 */
EOLIAN static unsigned int
_elm_web_none_elm_web_text_matches_mark(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, const char *string EINA_UNUSED, Eina_Bool case_sensitive EINA_UNUSED, Eina_Bool highlight EINA_UNUSED, unsigned int limit EINA_UNUSED)
{
   return 0;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_text_matches_unmark_all.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_text_matches_unmark_all(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_text_matches_highlight_set.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_text_matches_highlight_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, Eina_Bool highlight EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_text_matches_highlight_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_text_matches_highlight_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_load_progress_get.
 * @return Always -1.0.
 */
EOLIAN static double
_elm_web_none_elm_web_load_progress_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return -1.0;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_stop.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_stop(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_reload.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_reload(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_reload_full.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_reload_full(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_back.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_back(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_forward.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_forward(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_navigate.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_navigate(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int steps EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_back_possible_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_back_possible_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_forward_possible_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_forward_possible_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_navigate_possible_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_navigate_possible_get(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int steps EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_history_enabled_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_history_enabled_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_history_enabled_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_history_enabled_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, Eina_Bool enable EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref efl_ui_zoom_level_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_efl_ui_zoom_zoom_level_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, double zoom EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref efl_ui_zoom_level_get.
 * @return Always -1.
 */
EOLIAN static double
_elm_web_none_efl_ui_zoom_zoom_level_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return -1;
}

/**
 * @internal
 * @brief Stub implementation for @ref efl_ui_zoom_mode_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_efl_ui_zoom_zoom_mode_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, Efl_Ui_Zoom_Mode mode EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref efl_ui_zoom_mode_get.
 * @return Always @c EFL_UI_ZOOM_MODE_LAST.
 */
EOLIAN static Efl_Ui_Zoom_Mode
_elm_web_none_efl_ui_zoom_zoom_mode_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED)
{
   return EFL_UI_ZOOM_MODE_LAST;
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_region_show.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_region_show(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_region_bring_in.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_region_bring_in(Eo *obj EINA_UNUSED, Elm_Web_None_Data *_pd EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_inwin_mode_set.
 *
 * This is a no-op.
 */
EOLIAN static void
_elm_web_none_elm_web_inwin_mode_set(Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED, Eina_Bool value EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Stub implementation for @ref elm_web_inwin_mode_get.
 * @return Always @c EINA_FALSE.
 */
EOLIAN static Eina_Bool
_elm_web_none_elm_web_inwin_mode_get(const Eo *obj EINA_UNUSED, Elm_Web_None_Data *sd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Stub for ewm_window_features_ref.
 *
 * A no-op in the "none" engine.
 */
EMODAPI void
ewm_window_features_ref(Elm_Web_Window_Features *wf EINA_UNUSED)
{
}

/**
 * @brief Stub for ewm_window_features_unref.
 *
 * A no-op in the "none" engine.
 */
EMODAPI void
ewm_window_features_unref(Elm_Web_Window_Features *wf EINA_UNUSED)
{
}

/**
 * @brief Stub for ewm_window_features_property_get.
 *
 * @return Always @c EINA_FALSE.
 */
EMODAPI Eina_Bool
ewm_window_features_property_get(const Elm_Web_Window_Features *wf EINA_UNUSED,
                                 Elm_Web_Window_Feature_Flag flag EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Stub for ewm_window_features_region_get.
 *
 * This function sets the coordinates to 0.
 */
EMODAPI void
ewm_window_features_region_get(const Elm_Web_Window_Features *wf EINA_UNUSED,
                               Evas_Coord *x,
                               Evas_Coord *y,
                               Evas_Coord *w,
                               Evas_Coord *h)
{
   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;
}

/**
 * @brief Called when the web engine is no longer needed.
 *
 * A no-op in the "none" engine.
 */
EMODAPI void
ewm_unneed_web(void)
{
}

/**
 * @brief Called when the web engine is needed.
 *
 * This function ensures the log domain for the "none" web engine is
 * registered.
 * @return @c EINA_TRUE on success.
 */
EMODAPI Eina_Bool
ewm_need_web(void)
{
   if (_none_log_dom == -1)
     _none_log_dom =  eina_log_domain_register("elm_none", EINA_COLOR_LIGHTBLUE);
   return EINA_TRUE;
}

/**
 * @brief Gets the Elm_Web_None class.
 *
 * @return The EFL class for the "none" web widget.
 */
EMODAPI const Efl_Class *
ewm_class_get(void)
{
   return elm_web_none_class_get();
}

#undef ELM_WEB_CLASS
#define ELM_WEB_CLASS elm_web_class_get()

/* Internal EO APIs and hidden overrides */

#define ELM_WEB_NONE_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(elm_web_none)

#include "elm_web_none_eo.c"
