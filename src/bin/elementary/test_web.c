#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Holds all the UI components and state for the web test application.
 *
 * This structure is passed around as a data pointer to callbacks to allow
 * them to access the various Evas_Object widgets and other state information
 * like the list of sub-windows.
 */
typedef struct
{
   Evas_Object *web;
   Evas_Object *btn_back;
   Evas_Object *btn_fwd;
   Evas_Object *url_entry;
   Evas_Object *bx;
   Evas_Object *hoversel;
   Eina_List *sub_wins;
   Eina_Bool js_hooks : 1;
} Web_Test;

/**
 * @brief Callback for the "Back" button.
 * @param data The web object.
 * @param obj The button object that triggered the callback.
 * @param event_info Event-specific information (unused).
 *
 * This function navigates the web object back to the previous page in history.
 */
static void
_btn_back_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *web = data;

   elm_web_back(web);
}

/**
 * @brief Callback for the "Forward" button.
 * @param data The web object.
 * @param obj The button object that triggered the callback.
 * @param event_info Event-specific information (unused).
 *
 * This function navigates the web object forward to the next page in history.
 */
static void
_btn_fwd_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *web = data;

   elm_web_forward(web);
}

/**
 * @brief Callback for the "Reload" button.
 * @param data The web object.
 * @param obj The button object that triggered the callback.
 * @param event_info Event-specific information (unused).
 *
 * This function reloads the current page in the web object.
 */
static void
_btn_reload_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *web = data;

   elm_web_reload(web);
}

/**
 * @brief Callback for the URL entry "activated" event (e.g., Enter pressed).
 * @param data The web object.
 * @param obj The entry object.
 * @param event_info Event-specific information (unused).
 *
 * This function gets the URL from the entry and instructs the web object to
 * load it.
 */
static void
_url_entry_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *web = data;
   const char *url = elm_object_text_get(obj);

   elm_web_url_set(web, url);
}

/**
 * @brief Toggles the "inwin" mode for the web view.
 * @param data The web object.
 * @param obj The button object that triggered the callback.
 * @param event_info Event-specific information (unused).
 *
 * Inwin mode means that instead of opening a new window for popups, it will
 * try to open them in a small area on top of the current web view.
 */
static void
_toggle_inwin_mode_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_web_inwin_mode_set(data, !elm_web_inwin_mode_get(data));
}

/**
 * @brief Callback for the "title,changed" smart event from the web object.
 * @param data The window object.
 * @param obj The web object that triggered the event.
 * @param event_info The new page title as a const char*.
 *
 * Updates the window title to reflect the current page's title.
 */
static void
_title_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   char buf[512];
   snprintf(buf, sizeof(buf), "Web - %s", (const char *)event_info);
   elm_win_title_set(data, buf);
}

/**
 * @brief Callback for the "url,changed" smart event from the web object.
 * @param data The Web_Test struct.
 * @param obj The web object that triggered the event.
 * @param event_info The new URL as a const char*.
 *
 * Updates the URL entry to show the new URL, and enables/disables the
 * back and forward buttons based on navigation history availability.
 */
static void
_url_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Web_Test *wt = data;

   elm_object_text_set(wt->url_entry, event_info);

   elm_object_disabled_set(wt->btn_back, !elm_web_back_possible_get(wt->web));
   elm_object_disabled_set(wt->btn_fwd, !elm_web_forward_possible_get(wt->web));
}

/**
 * @brief Callback for when a sub-window (popup) is deleted.
 * @param data The Web_Test struct.
 * @param obj The sub-window that is being deleted.
 * @param event_info Event-specific information (unused).
 *
 * Removes the sub-window from the list of managed sub-windows.
 */
static void
_new_win_del_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   wt->sub_wins = eina_list_remove(wt->sub_wins, obj);
}

/**
 * @brief Callback for when a web page requests to close its window.
 * @param data The window object to be closed.
 * @param obj The web object that made the request.
 * @param event_info Event-specific information (unused).
 *
 * This handles JavaScript's `window.close()` and deletes the corresponding
 * popup window.
 */
static void
_web_win_close_request_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

/**
 * @brief Hook to handle JavaScript's `window.open()`.
 * @param data The Web_Test struct.
 * @param obj The parent web object.
 * @param js Whether the call was initiated by JavaScript (unused).
 * @param wf Window features requested (e.g. width, height) (unused).
 * @return The new web object created for the popup window.
 *
 * This function creates a new standard window, places a new web object in it,
 * and sets up callbacks to manage the new window's lifecycle.
 */
static Evas_Object *
_new_window_hook(void *data, Evas_Object *obj, Eina_Bool js EINA_UNUSED, const Elm_Web_Window_Features *wf EINA_UNUSED)
{
   Web_Test *wt = data;
   Evas_Object *new_win, *new_web;

   new_win = elm_win_util_standard_add("elm-web-test-popup", "Elm Web Test Popup");
   elm_win_autodel_set(new_win, EINA_TRUE);
   evas_object_resize(new_win, 300, 300);
   evas_object_show(new_win);

   new_web = elm_web_add(new_win);
   elm_web_useragent_set(new_web, elm_web_useragent_get(obj));
   evas_object_size_hint_weight_set(new_web, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(new_win, new_web);
   evas_object_show(new_web);

   evas_object_smart_callback_add(new_win, "delete,request", _new_win_del_cb,
                                  wt);
   evas_object_smart_callback_add(new_web, "windows,close,request",
                                  _web_win_close_request_cb, new_win);
   wt->sub_wins = eina_list_append(wt->sub_wins, new_win);

   return new_web;
}

/**
 * @brief Deletes the object when the alert popup is clicked.
 * @param data Custom data (unused).
 * @param obj The popup object to be deleted.
 * @param event_info Event-specific information (unused).
 */
static void
_alert_del(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   evas_object_del(obj);
}

/**
 * @brief Hook to handle JavaScript's `window.alert()`.
 * @param data Custom data (unused).
 * @param obj The parent web object.
 * @param message The message passed to `alert()`.
 * @return A new popup object (an elm_notify widget).
 *
 * This function creates and shows a simple notification popup with the
 * alert message. The popup is dismissed when clicked.
 */
static Evas_Object *
_alert_hook(void *data EINA_UNUSED, Evas_Object *obj, const char *message)
{
   Evas_Object *popup, *label;

   popup = elm_notify_add(obj);
   elm_notify_align_set(popup, 0.5, 0.5);
   // Using the timeout doesn't seem to go well with the second main loop
   //elm_notify_timeout_set(popup, 2.0);
   elm_notify_allow_events_set(popup, EINA_FALSE);
   evas_object_show(popup);

   evas_object_smart_callback_add(popup, "block,clicked", _alert_del, NULL);

   label = elm_label_add(obj);
   elm_object_text_set(label, message);
   elm_object_content_set(popup, label);
   evas_object_show(label);

   return popup;
}

/**
 * @brief Callback for the "Ok" button in the confirm dialog.
 * @param data A pointer to an Eina_Bool to store the result.
 * @param obj The button that was clicked.
 * @param event_info Event-specific information (unused).
 *
 * Sets the response to EINA_TRUE, indicating the user confirmed.
 */
static void
_confirm_ok_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_Bool *response = data;
   *response = EINA_TRUE;
}

/**
 * @brief Callback for the "Cancel" button in the confirm dialog.
 * @param data A pointer to an Eina_Bool to store the result.
 * @param obj The button that was clicked.
 * @param event_info Event-specific information (unused).
 *
 * Sets the response to EINA_FALSE, indicating the user canceled.
 */
static void
_confirm_cancel_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eina_Bool *response = data;
   *response = EINA_FALSE;
}

/**
 * @brief Callback to dismiss the confirm dialog.
 * @param data The popup object to be deleted.
 * @param obj The button that was clicked.
 * @param event_info Event-specific information (unused).
 */
static void
_confirm_dismiss_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

/**
 * @brief Hook to handle JavaScript's `window.confirm()`.
 * @param data Custom data (unused).
 * @param obj The parent web object.
 * @param message The message for the confirmation dialog.
 * @param response A pointer to an Eina_Bool where the result (TRUE for OK,
 *                 FALSE for Cancel) will be stored.
 * @return A new popup object representing the confirm dialog.
 *
 * Creates a notification with "Ok" and "Cancel" buttons. The user's choice
 * is written to the `response` pointer.
 */
static Evas_Object *
_confirm_hook(void *data EINA_UNUSED, Evas_Object *obj, const char *message, Eina_Bool *response)
{
   Evas_Object *popup, *box, *box2, *label, *btn_ok, *btn_cancel;

   popup = elm_notify_add(obj);
   elm_notify_align_set(popup, 0.5, 0.5);
   elm_notify_allow_events_set(popup, EINA_FALSE);
   evas_object_show(popup);

   box = elm_box_add(obj);
   elm_object_content_set(popup, box);
   evas_object_show(box);

   label = elm_label_add(obj);
   elm_object_text_set(label, message);
   elm_box_pack_end(box, label);
   evas_object_show(label);

   box2 = elm_box_add(obj);
   elm_box_horizontal_set(box2, EINA_TRUE);
   elm_box_pack_end(box, box2);
   evas_object_show(box2);

   btn_ok = elm_button_add(obj);
   elm_object_text_set(btn_ok, "Ok");
   elm_box_pack_end(box2, btn_ok);
   evas_object_show(btn_ok);

   btn_cancel = elm_button_add(obj);
   elm_object_text_set(btn_cancel, "Cancel");
   elm_box_pack_end(box2, btn_cancel);
   evas_object_show(btn_cancel);

   evas_object_smart_callback_add(btn_ok, "clicked", _confirm_dismiss_cb,
                                  popup);
   evas_object_smart_callback_add(btn_cancel, "clicked", _confirm_dismiss_cb,
                                  popup);
   evas_object_smart_callback_add(btn_ok, "clicked", _confirm_ok_cb, response);
   evas_object_smart_callback_add(btn_cancel, "clicked", _confirm_cancel_cb,
                                  response);

   return popup;
}

/**
 * @brief Hook to handle JavaScript's `window.prompt()`.
 * @param data Custom data (unused).
 * @param obj The parent web object (unused).
 * @param message The message for the prompt dialog (unused).
 * @param default_value The default value for the text input.
 * @param value A pointer to a char* where the entered string will be stored.
 * @param response A pointer to an Eina_Bool for the result (OK/Cancel).
 * @return NULL, as this hook provides a synchronous response.
 *
 * This is a simplified implementation that does not show a real prompt dialog.
 * It immediately returns success and provides either the default value or
 * a hardcoded string as the result. The returned string must be freed by the
 * caller.
 */
static Evas_Object *
_prompt_hook(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *message EINA_UNUSED, const char *default_value, const char **value, Eina_Bool *response)
{
   *response = EINA_TRUE;
   *value = default_value ? strdup(default_value) : "No default!";
   return NULL;
}

/**
 * @brief Hook for file selector requests.
 * @param data Custom data (unused).
 * @param obj The parent web object (unused).
 * @param allow_multiple Whether multiple files can be selected (unused).
 * @param accept_types A list of accepted MIME types (unused).
 * @param selected_files A pointer to an Eina_List* to store selected file paths.
 * @param response A pointer to an Eina_Bool for the result (OK/Cancel).
 * @return NULL, as this hook provides a synchronous response.
 *
 * This is a simplified implementation that does not show a file dialog.
 * It immediately returns success and provides a hardcoded, non-existing
 * file path. The returned list and its string content must be freed.
 *
 * Example of `selected_files` structure after return:
 * Eina_List -> ["/path/to/non_existing_file"]
 */
static Evas_Object *
_file_selector_hook(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, Eina_Bool allow_multiple EINA_UNUSED, Eina_List *accept_types EINA_UNUSED, Eina_List **selected_files, Eina_Bool *response)
{
   *selected_files = eina_list_append(NULL,
                                      strdup("/path/to/non_existing_file"));
   *response = EINA_TRUE;
   return NULL;
}

/**
 * @brief Hook for JavaScript's `console.log()` messages.
 * @param data Custom data (unused).
 * @param obj The web object (unused).
 * @param message The console message.
 * @param line_number The line number in the source where the message originated.
 * @param source_id The ID of the source file or script.
 *
 * This function simply prints the console message to standard output.
 */
static void
_console_message_hook(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *message, unsigned int line_number, const char *source_id)
{
   printf("CONSOLE: %s:%u:%s\n", source_id, line_number, message);
}

/**
 * @brief Toggles the installation of custom JavaScript popup hooks.
 * @param data The Web_Test struct.
 * @param obj The button object (unused).
 * @param event_info Event-specific data (unused).
 *
 * This function enables or disables all the custom hooks for handling
 * `alert`, `confirm`, `prompt`, file selectors, and console messages. When
 * hooks are disabled, the web engine's default handlers are used.
 */
static void
_js_popup_hooks_set(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;

   wt->js_hooks = !wt->js_hooks;
   if (wt->js_hooks)
     {
        elm_web_dialog_alert_hook_set(wt->web, _alert_hook, NULL);
        elm_web_dialog_confirm_hook_set(wt->web, _confirm_hook, NULL);
        elm_web_dialog_prompt_hook_set(wt->web, _prompt_hook, NULL);
        elm_web_dialog_file_selector_hook_set(wt->web, _file_selector_hook,
                                              NULL);
        elm_web_console_message_hook_set(wt->web, _console_message_hook, NULL);
     }
   else
     {
        elm_web_dialog_alert_hook_set(wt->web, NULL, NULL);
        elm_web_dialog_confirm_hook_set(wt->web, NULL, NULL);
        elm_web_dialog_prompt_hook_set(wt->web, NULL, NULL);
        elm_web_dialog_file_selector_hook_set(wt->web, NULL, NULL);
        elm_web_console_message_hook_set(wt->web, NULL, NULL);
     }
}

/**
 * @brief Callback for the zoom out button.
 * @param data The Web_Test struct.
 * @param obj The button object (unused).
 * @param event_info Event-specific data (unused).
 *
 * Decreases the zoom level of the web view, with a lower bound.
 */
static void
_zoom_out_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   double zoom;

   zoom = elm_web_zoom_get(wt->web);
   if (zoom > 1)
     zoom -= .5;
   else
     zoom /= 2;
   if (zoom < .05)
     zoom = .05;
   elm_web_zoom_set(wt->web, zoom);
}

/**
 * @brief Callback for the zoom in button.
 * @param data The Web_Test struct.
 * @param obj The button object (unused).
 * @param event_info Event-specific data (unused).
 *
 * Increases the zoom level of the web view, with an upper bound.
 */
static void
_zoom_in_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   double zoom;

   zoom = elm_web_zoom_get(wt->web);

   if (zoom < 1)
     zoom *= 2;
   else
     zoom += .5;
   if (zoom > 4)
     zoom = 4;
   elm_web_zoom_set(wt->web, zoom);
}

/**
 * @brief Callback for selecting a zoom mode from the hoversel.
 * @param data The Web_Test struct.
 * @param obj The hoversel object (unused).
 * @param event_info The selected hoversel item.
 *
 * Sets the zoom mode of the web view based on the selected item.
 * Modes can be Manual, Auto Fit, or Auto Fill.
 */
static void
_zoom_mode_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Web_Test *wt = data;
   Elm_Object_Item *hoversel_it = event_info;
   const char *lbl = elm_object_item_text_get(hoversel_it);

   if (!strcmp(lbl, "Manual"))
     elm_web_zoom_mode_set(wt->web, ELM_WEB_ZOOM_MODE_MANUAL);
   else if (!strcmp(lbl, "Fit"))
     elm_web_zoom_mode_set(wt->web, ELM_WEB_ZOOM_MODE_AUTO_FIT);
   else
     elm_web_zoom_mode_set(wt->web, ELM_WEB_ZOOM_MODE_AUTO_FILL);
}

/**
 * @brief Callback to show a specific region of the web page.
 * @param data The Web_Test struct.
 * @param obj The button object (unused).
 * @param event_info Event-specific data (unused).
 *
 * Scrolls the web view to show the region at coordinates (300, 300).
 */
static void
_show_region_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   elm_web_region_show(wt->web, 300, 300, 1, 1);
}

/**
 * @brief Callback to bring a specific region into view.
 * @param data The Web_Test struct.
 * @param obj The button object (unused).
 * @param event_info Event-specific data (unused).
 *
 * Scrolls the web view to make the region at coordinates (50, 0) visible.
 */
static void
_bring_in_region_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   elm_web_region_bring_in(wt->web, 50, 0, 1, 1);
}

/**
 * @brief Callback executed when the window enters fullscreen mode.
 * @param data The Web_Test struct.
 * @param obj The window object (unused).
 * @param event_info Event-specific data (unused).
 *
 * This is used in the UI test scenario (`test_web_ui`) to hide the
 * test case hoversel when fullscreen is activated.
 */
static void
_on_fullscreen_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   elm_box_unpack(wt->bx, wt->hoversel);
   evas_object_hide(wt->hoversel);
}

/**
 * @brief Callback executed when the window leaves fullscreen mode.
 * @param data The Web_Test struct.
 * @param obj The window object (unused).
 * @param event_info Event-specific data (unused).
 *
 * This is used in the UI test scenario (`test_web_ui`) to restore the
 * test case hoversel when fullscreen is exited.
 */
static void
_on_unfullscreen_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   elm_box_pack_start(wt->bx, wt->hoversel);
   evas_object_show(wt->hoversel);
}

/**
 * @brief Associates a friendly name with a full user agent string.
 */
typedef struct
{
   const char* name;      /**< A short, readable name for the user agent. */
   const char* useragent; /**< The full user agent string. NULL for default. */
} User_Agent;

static User_Agent ua[] = {
    {"Default", NULL},
    {"Mobile/Iphone", "Mozilla/5.0 (iPhone; CPU iPhone OS 6_1 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10B142 Safari/8536.25"},
    {"Mobile/Android(Chrome)", "Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 Mobile Safari/535.19"},
    {"Mobile/Android", "Mozilla/5.0 (Linux; U; Android 4.0.2; en-us; Galaxy Nexus Build/ICL53F) AppleWebKit/534.30 (KHTML, like Gecko) Version/4.0 Mobile Safari/534.30"},
    {"Desktop/Firefox", "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:25.0) Gecko/20100101 Firefox/25.0"},
    {"Desktop/Chrome", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.17 (KHTML, like Gecko) Chrome/24.0.1312.57 Safari/537.17"}
};

/**
 * @brief Callback for selecting a user agent from the hoversel.
 * @param data The Web_Test struct.
 * @param obj The hoversel object (unused).
 * @param event_info The selected hoversel item.
 *
 * Sets the user agent string for the web view based on the user's selection.
 */
static void
_useragent_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Web_Test *wt = data;
   Elm_Object_Item *hoversel_it = event_info;
   const char *lbl = elm_object_item_text_get(hoversel_it);
   unsigned i;

   for (i = 0; i < sizeof(ua) / sizeof(ua[0]); ++i)
     if (!strcmp(lbl, ua[i].name))
       {
           printf("New user agent : %s\n", ua[i].useragent ? ua[i].useragent : "Default");
           elm_web_useragent_set(wt->web, ua[i].useragent);
       }
}

/**
 * @brief Loads an HTML string to test JavaScript dialogs (alert, confirm, prompt).
 * @param data The Web_Test struct.
 * @param obj The hoversel object.
 * @param event_info The selected hoversel item.
 *
 * This function is a test case selected from the `test_web_ui` hoversel.
 * It loads a specific HTML content into the web view to trigger and test
 * the dialog handling hooks.
 */
static void
_dialog_test_cb(void *data, Evas_Object *obj, void *event_info)
{
   Web_Test *wt = data;
   const char *selected = elm_object_item_text_get(event_info);
   const char dialog_html[] = "<!doctype html><body>"
       "<script>"
       "var confirm_test = function() {"
       " if (window.confirm('confirm') == true) {"
       "   document.getElementById('r').innerHTML = 'You pressed OK';"
       " } else {"
       "   document.getElementById('r').innerHTML = 'You pressed Cancel';"
       " }"
       "};"
       "var prompt_test = function() {"
       " document.getElementById('r').innerHTML = window.prompt('Enter your name', 'EFL');"
       "};"
       "</script>"
       "Result: <div id='r'> </div>"
       "<input type='button' value='alert' onclick=\"window.alert('alert pressed');\">"
       "<input type='button' value='confirm' onclick=\"confirm_test();\">"
       "<input type='button' value='prompt' onclick=\"prompt_test();\">"
       "</body>";

   printf("selected test : %s\n", selected);
   elm_object_text_set(obj, selected);

   elm_web_html_string_load(wt->web, dialog_html, NULL, NULL);
}

/**
 * @brief Loads an HTML string to test the `<select>` tag.
 * @param data The Web_Test struct.
 * @param obj The hoversel object.
 * @param event_info The selected hoversel item.
 *
 * This function is a test case selected from the `test_web_ui` hoversel.
 * It loads HTML containing a `<select>` element to test its rendering
 * and interaction.
 */
static void
_select_tag_test_cb(void *data, Evas_Object *obj, void *event_info)
{
   Web_Test *wt = data;
   const char *selected = elm_object_item_text_get(event_info);
   const char select_html[] = "<!doctype html><body>"
       "<select>"
       "<option>eina</option>"
       "<option>ecore</option>"
       "<option>evas</option>"
       "<option>edje</option>"
       "<option>eet</option>"
       "<option>emotion</option>"
       "<option>elementary</option>"
       "</select>"
       "</body>";

   printf("selected test : %s\n", selected);
   elm_object_text_set(obj, selected);

   elm_web_html_string_load(wt->web, select_html, NULL, NULL);
}

/**
 * @brief Loads an HTML string to test `window.open()`.
 * @param data The Web_Test struct.
 * @param obj The hoversel object.
 * @param event_info The selected hoversel item.
 *
 * This function is a test case selected from the `test_web_ui` hoversel.
 * It loads HTML with JavaScript that calls `window.open()` and `window.close()`
 * to test the new window creation hook.
 */
static void
_new_window_test_cb(void *data, Evas_Object *obj, void *event_info)
{
   Web_Test *wt = data;
   const char *selected = elm_object_item_text_get(event_info);
   const char new_window_html[] = "<!doctype html><body>"
       "<script>"
       "var h = null;"
       "var test = function() {"
       "  if (!h) {"
       "    h = window.open('http://www.enlightenment.org','','width=400,height=300');"
       "    document.getElementById('btn').value='close window';"
       "  } else {"
       "    h.close();"
       "    h = null;"
       "    document.getElementById('btn').value='open new window';"
       "  }"
       "}"
       "</script>"
       "<input type='button' id='btn' onclick='test();' value='open new window'>"
       "</body>";

   printf("selected test : %s\n", selected);
   elm_object_text_set(obj, selected);

   elm_web_html_string_load(wt->web, new_window_html, NULL, NULL);
}

/**
 * @brief Loads an HTML string to test the JavaScript Fullscreen API.
 * @param data The Web_Test struct.
 * @param obj The hoversel object.
 * @param event_info The selected hoversel item.
 *
 * This function is a test case selected from the `test_web_ui` hoversel.
 * It loads HTML with JavaScript that uses `webkitRequestFullscreen` to test
 * the fullscreen functionality.
 */
static void
_fullscreen_test_cb(void *data, Evas_Object *obj, void *event_info)
{
   Web_Test *wt = data;
   const char *selected = elm_object_item_text_get(event_info);
   const char fullscreen_html[] = "<!doctype html><body>"
       "<script>"
       "var launch = function(obj) {"
       "  var f = document.webkitFullscreenElement;"
       "  if (f) document.webkitExitFullscreen();"
       "  if (f != obj) obj.webkitRequestFullscreen();"
       "}\n"
       "var test_full = function() { launch(document.documentElement); }\n"
       "var test_small = function() { launch(document.getElementById('box')); }\n"
       "</script>"
       "<input type='button' onclick='test_full();' value='request fullscreen'>"
       "<div id='box' style='width:100px;height:100px;background-color:blue;' onclick='test_small();'>small box</div>"
       "<input type='button' onclick='test_small();' value='request fullscreen of box'>"
       "</body>";

   printf("selected test : %s\n", selected);
   elm_object_text_set(obj, selected);

   elm_web_html_string_load(wt->web, fullscreen_html, NULL, NULL);
}

/**
 * @brief Callback for when the main web object is deleted.
 * @param data The Web_Test struct.
 * @param e The Evas canvas (unused).
 * @param obj The web object being deleted (unused).
 * @param event_info Event-specific data (unused).
 *
 * This function performs cleanup when the application is closing. It deletes
 * any open sub-windows and frees the Web_Test struct.
 */
static void
_main_web_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Web_Test *wt = data;
   Evas_Object *sub_win;

   EINA_LIST_FREE(wt->sub_wins, sub_win)
      evas_object_del(sub_win);

   free(wt);
}

/**
 * @brief Main function to create the primary web test window.
 *
 * This test creates a window with a web view, navigation controls (back,
 * forward, reload), a URL entry bar, and buttons for various features like
 * zoom, inwin mode, custom JS hooks, and user agent switching. It's a general
 * purpose browser-like interface for testing the web widget.
 */
void
test_web(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx2, *bt, *web, *url;
   Web_Test *wt;
   unsigned i;

   elm_need_web();

   wt = calloc(1, sizeof(*wt));
   win = elm_win_util_standard_add("web", "Web");

   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   web = elm_web_add(win);
   evas_object_size_hint_weight_set(web, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(web, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, web);
   evas_object_show(web);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "<");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _btn_back_cb, web);
   wt->btn_back = bt;

   bt = elm_button_add(win);
   elm_object_text_set(bt, "R");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _btn_reload_cb, web);

   bt = elm_button_add(win);
   elm_object_text_set(bt, ">");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _btn_fwd_cb, web);
   wt->btn_fwd = bt;

   url = elm_entry_add(win);
   elm_entry_single_line_set(url, EINA_TRUE);
   elm_entry_scrollable_set(url, EINA_TRUE);
   evas_object_size_hint_weight_set(url, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(url, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx2, url);
   evas_object_show(url);

   evas_object_smart_callback_add(url, "activated", _url_entry_changed_cb, web);
   wt->url_entry = url;

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Inwin Mode");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _toggle_inwin_mode_cb, web);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Custom Hooks");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _js_popup_hooks_set, wt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "-");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _zoom_out_cb, wt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "+");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _zoom_in_cb, wt);

   bt = elm_hoversel_add(win);
   elm_object_text_set(bt, "Zoom Mode");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   elm_hoversel_item_add(bt, "Manual", NULL, ELM_ICON_NONE, _zoom_mode_cb, wt);
   elm_hoversel_item_add(bt, "Fit", NULL, ELM_ICON_NONE, _zoom_mode_cb, wt);
   elm_hoversel_item_add(bt, "Fill", NULL, ELM_ICON_NONE, _zoom_mode_cb, wt);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Show 300, 300");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _show_region_cb, wt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Bring in 50, 0");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(bt, "clicked", _bring_in_region_cb, wt);

   bt = elm_hoversel_add(win);
   elm_object_text_set(bt, "User agent");
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   for (i = 0; i < sizeof(ua) / sizeof(ua[0]); ++i)
     elm_hoversel_item_add(bt, ua[i].name, NULL, ELM_ICON_NONE, _useragent_cb, wt);

   evas_object_smart_callback_add(web, "title,changed", _title_changed_cb, win);
   evas_object_smart_callback_add(web, "url,changed", _url_changed_cb, wt);

   evas_object_event_callback_add(web, EVAS_CALLBACK_DEL, _main_web_del_cb, wt);

   wt->web = web;

   elm_web_url_set(web, "http://www.enlightenment.org");

   elm_web_window_create_hook_set(web, _new_window_hook, wt);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Creates a test window focused on specific UI interaction scenarios.
 *
 * This test creates a window with a web view and a hoversel menu. The menu
 * allows loading different HTML content to test specific features like
 * dialogs, `<select>` tags, new windows, and fullscreen. This is more of a
 * targeted test case runner than a general browser.
 */
void
test_web_ui(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *web, *hoversel;
   Web_Test *wt;

   elm_need_web();

   wt = calloc(1, sizeof(*wt));
   win = elm_win_util_standard_add("web", "Web");

   evas_object_smart_callback_add(win, "fullscreen", _on_fullscreen_cb, wt);
   evas_object_smart_callback_add(win, "unfullscreen", _on_unfullscreen_cb, wt);
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   hoversel = elm_hoversel_add(bx);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Test cases");

   elm_hoversel_item_add(hoversel, "alert/confirm/prompt", NULL, ELM_ICON_NONE,
                         _dialog_test_cb, wt);
   elm_hoversel_item_add(hoversel, "<select> tag", NULL, ELM_ICON_NONE,
                         _select_tag_test_cb, wt);
   elm_hoversel_item_add(hoversel, "new window", NULL, ELM_ICON_NONE,
                         _new_window_test_cb, wt);
   elm_hoversel_item_add(hoversel, "fullscreen", NULL, ELM_ICON_NONE,
                         _fullscreen_test_cb, wt);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   web = elm_web_add(win);
   evas_object_size_hint_weight_set(web, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(web, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, web);
   evas_object_show(web);

   elm_web_window_create_hook_set(web, _new_window_hook, wt);

   evas_object_event_callback_add(web, EVAS_CALLBACK_DEL, _main_web_del_cb, wt);
   wt->web = web;
   wt->bx = bx;
   wt->hoversel = hoversel;


   elm_web_html_string_load(wt->web,
                            "<!doctype html><body>Hello, WebKit/Efl</body>",
                            NULL, NULL);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}
