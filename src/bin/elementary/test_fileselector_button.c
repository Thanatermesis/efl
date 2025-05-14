#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#endif

#include <Elementary.h>

/**
 * @brief Test-specific data structure to hold UI state and widgets.
 */
struct _api_data
{
   unsigned int state;  /**< Current state of the API test sequence. */
   Evas_Object *box;    /**< A box widget used as a container for other UI elements. */
   Eina_Bool free_data; /**< Flag to indicate whether this struct should be freed on cleanup. */
};
typedef struct _api_data api_data;

/**
 * @brief States for the API test cycle.
 *
 * Each state corresponds to a specific API function call on the
 * fileselector button widget.
 */
enum _api_state
{
   ICON_UNSET, /**< Unset the icon of the fileselector button. */
   WINDOW_TITLE_SET, /**< Set a custom window title for the fileselector. */
   API_STATE_LAST /**< Marker for the end of the test sequence. */
};
typedef enum _api_state api_state;

/**
 * @brief Applies an API function to the fileselector button based on the
 *        current test state.
 * @param api The test data structure, containing the current state.
 *
 * This function retrieves the fileselector button from the UI and modifies
 * one of its properties according to the `api->state` value. It is called
 * to cycle through different API function tests.
 */
static void
set_api_state(api_data *api)
{
   const Eina_List *items = elm_box_children_get(api->box);
   if (!eina_list_count(items))
     return;

   /* Get first item of list of vbox children */
   Evas_Object *fs_bt = eina_list_nth(items, 0);

   /* use elm_box_children_get() to get list of children */
   switch(api->state)
     { /* Put all api-changes under switch */
      case ICON_UNSET:
         elm_object_part_content_unset(fs_bt, NULL);
         break;

      case WINDOW_TITLE_SET:
         elm_fileselector_button_window_title_set(fs_bt, "Custom title from API");
         break;

      case API_STATE_LAST:

         break;
      default:
         return;
     }
}

/**
 * @brief Callback for the "Next API function" button.
 * @param data The api_data struct.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 *
 * This function advances the API test to the next state, calls set_api_state()
 * to apply the change, and updates the button's text to reflect the next
 * state. It disables the button when the last state is reached.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{  /* Will add here a SWITCH command containing code to modify test-object */
   /* in accordance a->state value. */
   api_data *a = data;
   char str[128];

   printf("clicked event on API Button: api_state=<%d>\n", a->state);
   set_api_state(a);
   a->state++;
   sprintf(str, "Next API function (%u)", a->state);
   elm_object_text_set(obj, str);
   elm_object_disabled_set(obj, a->state == API_STATE_LAST);
}

/**
 * @brief Creates a temporary directory structure for testing.
 *
 * This function sets up a directory `/tmp/test_fs_bt` containing a few
 * files and a subdirectory. This provides a consistent environment for
 * testing the fileselector button.
 */
static void
_create_dir_struct(void)
{
   FILE *fp;
   if (mkdir("/tmp/test_fs_bt", S_IRWXU) < 0)
     printf("make dir /tmp/test_fs_bt failed!\n");
   fp = fopen("/tmp/test_fs_bt/a_file.txt", "w");
   if (fp) fclose(fp);
   fp = fopen("/tmp/test_fs_bt/k_file.txt", "w");
   if (fp) fclose(fp);
   fp = fopen("/tmp/test_fs_bt/m_file.txt", "w");
   if (fp) fclose(fp);

   if (mkdir("/tmp/test_fs_bt/a_subdir", S_IRWXU) < 0)
     printf("make dir /tmp/test_fs_bt/a_subdir failed!\n");
   fp = fopen("/tmp/test_fs_bt/a_subdir/d_sub_file.txt", "w");
   if (fp) fclose(fp);
   fp = fopen("/tmp/test_fs_bt/a_subdir/j_sub_file.txt", "w");
   if (fp) fclose(fp);
}

/**
 * @brief Callback for the "file,chosen" event from the fileselector button.
 * @param data The entry widget to display the selected path.
 * @param obj The fileselector button that emitted the event.
 * @param event_info A string containing the full path to the chosen file.
 *
 * This function is called when a user selects a file from the fileselector.
 * It updates an entry widget to show the path of the selected file. If no
 * file is selected (e.g., the user cancels), `event_info` will be NULL.
 */
static void
_file_chosen(void            *data,
             Evas_Object *obj EINA_UNUSED,
             void            *event_info)
{
   Evas_Object *entry = data;
   const char *file = event_info;
   if (file)
     {
        elm_object_text_set(entry, file);
        printf("File chosen: %s\n", file);
     }
   else
     printf("File selection canceled.\n");
}

/**
 * @brief Toggles the "in-window" mode of the fileselector button.
 * @param data The fileselector button widget.
 * @param obj The checkbox that triggered the callback.
 * @param event_info Not used.
 *
 * "In-window" mode means the fileselector opens within the current window's
 * space rather than as a separate new window. This callback inverts the
 * current setting.
 */
static void
_inwin_mode_toggle(void            *data,
                   Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   Evas_Object *fs_bt = data;
   Eina_Bool value = elm_fileselector_button_inwin_mode_get(fs_bt);
   elm_fileselector_button_inwin_mode_set(fs_bt, !value);
   printf("Inwin mode set to: %s\n", value ? "false" : "true");
}

/**
 * @brief Toggles the "is-save" property of the fileselector.
 * @param data The fileselector button widget.
 * @param obj The checkbox that triggered the callback.
 * @param event_info Not used.
 *
 * When set, the fileselector is in "save" mode, which typically provides
 * an editable text entry for the filename. This callback toggles that state
 * based on a checkbox.
 */
static void
_current_sel_toggle(void            *data,
                    Evas_Object *obj,
                    void *event_info EINA_UNUSED)
{
   Evas_Object *fs_bt = data;
   Eina_Bool value = elm_check_state_get(obj);
   elm_fileselector_is_save_set(fs_bt, value);
   printf("Current selection editable entry set to: %s\n",
          value ? "true" : "false");
}

/**
 * @brief Toggles the "folder-only" mode of the fileselector.
 * @param data The fileselector button widget.
 * @param obj The checkbox that triggered the callback.
 * @param event_info Not used.
 *
 * In "folder-only" mode, the fileselector will only allow selecting directories,
 * not files. This callback toggles that mode.
 */
static void
_folder_only_toggle(void            *data,
                    Evas_Object *obj,
                    void *event_info EINA_UNUSED)
{
   Evas_Object *fs_bt = data;
   Eina_Bool value = elm_check_state_get(obj);
   elm_fileselector_folder_only_set(fs_bt, value);
   printf("Folder only flag set to: %s\n", value ? "true" : "false");
}

/**
 * @brief Toggles the "expandable" mode of the fileselector.
 * @param data The fileselector button widget.
 * @param obj The checkbox that triggered the callback.
 * @param event_info Not used.
 *
 * In "expandable" mode, the directory view can be expanded or collapsed.
 * This callback toggles that mode.
 */
static void
_expandable_toggle(void            *data,
                   Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   Evas_Object *fs_bt = data;
   Eina_Bool value = elm_check_state_get(obj);
   elm_fileselector_expandable_set(fs_bt, value);
   printf("Expandable flag set to: %s\n", value ? "true" : "false");
}

/**
 * @brief Frees test-specific data on window close.
 * @param data The api_data struct to be freed.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This is a callback for the EVAS_CALLBACK_FREE event on the main window,
 * ensuring that the allocated api_data structure is released.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Main function for the fileselector button test.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window and populates it with a fileselector button
 * and various controls (buttons, checkboxes) to test its functionality and
 * API. It sets up the initial state and connects all the callbacks.
 */
void
test_fileselector_button(void *data       EINA_UNUSED,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   Evas_Object *win, *vbox, *hbox, *ic, *bt, *fs_bt, *en, *lb, *bxx;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("fileselector-button", "File Selector Button");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   vbox = elm_box_add(win);
   api->box = vbox;
   evas_object_show(vbox);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   elm_box_pack_end(bxx, vbox);

   _create_dir_struct(); /* Create a dir struct in /tmp */
   /* file selector button */
   ic = elm_icon_add(win);
   elm_icon_standard_set(ic, "file");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   fs_bt = elm_fileselector_button_add(win);
   elm_object_text_set(fs_bt, "Select a file");
   elm_object_part_content_set(fs_bt, "icon", ic);
   elm_fileselector_path_set(fs_bt, "/tmp/test_fs_bt");
   evas_object_show(ic);

   elm_box_pack_end(vbox, fs_bt);
   evas_object_show(fs_bt);
   evas_object_show(ic);

   /* attribute setting buttons */
   hbox = elm_box_add(win);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   elm_box_pack_end(vbox, hbox);
   evas_object_show(hbox);

   bt = elm_check_add(win);
   elm_object_style_set(bt, "toggle");
   elm_object_text_set(bt, "current selection text entry");
   evas_object_smart_callback_add(bt, "changed", _current_sel_toggle, fs_bt);
   elm_box_pack_end(hbox, bt);
   evas_object_show(bt);

   bt = elm_check_add(win);
   elm_object_style_set(bt, "toggle");
   elm_object_text_set(bt, "Inwin mode");
   elm_check_state_set(bt, elm_fileselector_button_inwin_mode_get(fs_bt));
   evas_object_smart_callback_add(bt, "changed", _inwin_mode_toggle, fs_bt);
   elm_box_pack_end(hbox, bt);
   evas_object_show(bt);

   bt = elm_check_add(win);
   elm_object_style_set(bt, "toggle");
   elm_object_text_set(bt, "Folder only mode");
   evas_object_smart_callback_add(bt, "changed", _folder_only_toggle, fs_bt);
   elm_box_pack_end(hbox, bt);
   evas_object_show(bt);

   bt = elm_check_add(win);
   elm_object_style_set(bt, "toggle");
   elm_object_text_set(bt, "Expandable mode");
   evas_object_smart_callback_add(bt, "changed", _expandable_toggle, fs_bt);
   elm_box_pack_end(hbox, bt);
   evas_object_show(bt);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Last selection:");
   elm_box_pack_end(vbox, lb);
   evas_object_show(lb);

   en = elm_entry_add(win);
   elm_entry_line_wrap_set(en, ELM_WRAP_NONE);
   elm_entry_editable_set(en, EINA_FALSE);
   evas_object_smart_callback_add(fs_bt, "file,chosen", _file_chosen, en);
   elm_box_pack_end(vbox, en);
   evas_object_show(en);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
