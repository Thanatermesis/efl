#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Test function for an Edje external button.
 *
 * This function creates a new window and loads an Edje layout
 * that contains an external button. This is used to test the
 * integration of an Elementary button widget within an Edje design.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_button(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_button", "Edje External Button");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/button");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for an Edje external slider.
 *
 * This function creates a new window and loads an Edje layout
 * that contains an external slider. This is used to test the
 * integration of an Elementary slider widget within an Edje design.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_slider(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_slider", "Edje External Slider");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/slider");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for an Edje external scroller.
 *
 * This function creates a new window and loads an Edje layout
 * that contains an external scroller. This is used to test the
 * integration of an Elementary scroller widget within an Edje design.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_scroller(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_scroller", "Edje External Scroller");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/scroller");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Deletes an Ecore_Timer when its associated object is deleted.
 *
 * This callback is triggered by the EFL_EVENT_DEL event. It ensures that
 * the timer is properly cleaned up to prevent leaks when the object
 * it's associated with is destroyed.
 *
 * @param data The Ecore_Timer to delete.
 * @param ev The Efl_Event data (unused).
 */
static void
_timer_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Ecore_Timer *t = data;
   ecore_timer_del(t);
}

/**
 * @brief Timer callback to animate progress bars.
 *
 * This function is called periodically by an Ecore_Timer. It updates
 * the value of several progress bars embedded in an Edje object to
 * demonstrate their animation. It also tests different methods for
 * updating external parts, including direct object manipulation,
 * the Edje_External_Param API, and the EFL Efl.Ui.Range API.
 *
 * When the progress reaches 1.0, the animation stops, the timer is
 * cancelled, and related buttons are re-enabled.
 *
 * @param data The Edje Evas_Object containing the progress bars.
 * @return ECORE_CALLBACK_RENEW to continue the timer, or
 *         ECORE_CALLBACK_CANCEL to stop it.
 */
static Eina_Bool
_timer_cb(void *data)
{
   Evas_Object *edje = data;
   Evas_Object *bt1, *bt2, *bt3, *pb1, *pb2, *pb4, *pb5;
   Edje_External_Param param;
   double progress;
   Ecore_Timer *t;

   pb1 = edje_object_part_external_object_get(edje, "ext_pbar1");
   pb2 = edje_object_part_external_object_get(edje, "ext_pbar2");
   pb4 = edje_object_part_external_object_get(edje, "ext_pbar4");
   progress = elm_progressbar_value_get(pb1) + 0.0123;

   elm_progressbar_value_set(pb1, progress);
   elm_progressbar_value_set(pb2, progress);
   elm_progressbar_value_set(pb4, progress);

   /* Test external parameter API */
   param.name = "value";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
   param.d = progress;
   edje_object_part_external_param_set(edje, "ext_pbar6", &param);

   param.name = "pulsing";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
   param.i = EINA_TRUE;
   edje_object_part_external_param_set(edje, "ext_pbar7", &param);

   /* Test EO API for direct function calls */
   efl_ui_range_value_set(efl_part(edje, "ext_pbar3"), progress);

   if (progress < 1.0)
     return ECORE_CALLBACK_RENEW;

   pb5 = edje_object_part_external_object_get(edje, "ext_pbar5");
   elm_progressbar_pulse(pb2, EINA_FALSE);
   elm_progressbar_pulse(pb5, EINA_FALSE);

   bt1 = edje_object_part_external_object_get(edje, "ext_button1");
   bt2 = edje_object_part_external_object_get(edje, "ext_button2");
   bt3 = edje_object_part_external_object_get(edje, "ext_button3");
   elm_object_disabled_set(bt1, EINA_FALSE);
   elm_object_disabled_set(bt2, EINA_FALSE);
   elm_object_disabled_set(bt3, EINA_FALSE);

   /* Test external parameter API */
   param.name = "value";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
   param.d = 0.0;
   edje_object_part_external_param_set(edje, "ext_pbar6", &param);

   param.name = "pulsing";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
   param.i = EINA_FALSE;
   edje_object_part_external_param_set(edje, "ext_pbar7", &param);

   t = efl_key_data_get(edje, "timer");
   efl_event_callback_del(edje, EFL_EVENT_DEL, _timer_del, t);
   efl_key_data_set(edje, "timer", NULL);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback for the "clicked" event on a button.
 *
 * This function is called when the "Start" button in the progress bar
 * test is clicked. It disables the control buttons, resets the progress
 * bars to their initial state, and starts an Ecore_Timer to drive the
 * progress animation via `_timer_cb`.
 *
 * @param data The Edje Evas_Object containing the progress bars and buttons.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *edje = data;
   Evas_Object *bt1, *bt2, *bt3, *pb1, *pb2, *pb3, *pb4, *pb5;
   Edje_External_Param param;
   Ecore_Timer *t;

   /* Test direct API calls on embedded objects */
   bt1 = edje_object_part_external_object_get(edje, "ext_button1");
   bt2 = edje_object_part_external_object_get(edje, "ext_button2");
   bt3 = edje_object_part_external_object_get(edje, "ext_button3");
   elm_object_disabled_set(bt1, EINA_TRUE);
   elm_object_disabled_set(bt2, EINA_TRUE);
   elm_object_disabled_set(bt3, EINA_TRUE);

   pb1 = edje_object_part_external_object_get(edje, "ext_pbar1");
   pb2 = edje_object_part_external_object_get(edje, "ext_pbar2");
   pb3 = edje_object_part_external_object_get(edje, "ext_pbar3");
   pb4 = edje_object_part_external_object_get(edje, "ext_pbar4");
   pb5 = edje_object_part_external_object_get(edje, "ext_pbar5");

   elm_progressbar_value_set(pb1, 0.0);
   elm_progressbar_value_set(pb3, 0.0);
   elm_progressbar_value_set(pb4, 0.0);

   elm_progressbar_pulse(pb2, EINA_TRUE);
   elm_progressbar_pulse(pb5, EINA_TRUE);

   /* Test external parameter API */
   param.name = "value";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
   param.d = 0.0;
   edje_object_part_external_param_set(edje, "ext_pbar6", &param);

   param.name = "pulsing";
   param.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
   param.i = EINA_TRUE;
   edje_object_part_external_param_set(edje, "ext_pbar7", &param);

   /* Test EO API for direct function calls */
   efl_ui_range_value_set(efl_part(edje, "ext_pbar3"), 0.0);

   t = ecore_timer_add(0.1, _timer_cb, edje);
   efl_key_data_set(edje, "timer", t);
   efl_event_callback_add(edje, EFL_EVENT_DEL, _timer_del, t);
}

/**
 * @brief Test function for Edje external progress bars.
 *
 * This function creates a window with a layout containing several
 * external progress bar widgets and a button to control them. It demonstrates
 * how to interact with embedded Elementary widgets from the C code,
 * including setting up callbacks for user interaction.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_pbar(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly, *edje, *bt;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_pbar", "Edje External ProgressBar");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/pbar");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   edje = elm_layout_edje_get(ly);
   bt = edje_object_part_external_object_get(edje, "ext_button3");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, edje);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for an Edje external video player.
 *
 * Creates a window and loads an Edje layout that embeds a video player
 * widget. This tests the capability of integrating a video player as an
 * external part within an Edje design.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_video(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_video", "Edje External Video");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/video");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for an Edje external icon.
 *
 * Creates a window and loads an Edje layout that contains an external
 * icon. It also emits a signal to start animations defined in the
 * Edje theme, testing the interaction between C code and Edje signals.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_icon(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("ext_icon", "Edje External Icon");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/icon");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   elm_layout_signal_emit(ly, "elm_test,animations,start", "elm_test");

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Genlist item text provider callback.
 *
 * This function is called by the genlist widget to get the text for an
 * item. It simply duplicates the string passed as the `data` parameter.
 * The caller is responsible for freeing the returned string.
 *
 * @param data A pointer to a string to be displayed as the item's label.
 * @param obj The genlist object (unused).
 * @param part The theme part name (unused).
 * @return A newly allocated string containing the item's text.
 */
static char *
text_get_cb(void        *data,
            Evas_Object *obj  EINA_UNUSED,
            const char  *part EINA_UNUSED)
{
   return strdup(data);
}

/**
 * @brief Callback for genlist item selection.
 *
 * This function is called when a genlist item is selected. It retrieves
 * the data associated with the selected item and sets it as the text
 * of the "info" part in the main layout.
 *
 * @param data The main layout Evas_Object.
 * @param obj The genlist widget (unused).
 * @param info The selected Elm_Object_Item.
 */
static void
action_cb(void        *data,
          Evas_Object *obj  EINA_UNUSED,
          void        *info)
{
   Evas_Object *const lay = data;
   Elm_Object_Item *const item = info;

   elm_layout_text_set(lay, "info", elm_object_item_data_get(item));
}

/**
 * @brief Callback for a combobox item "pressed" event.
 *
 * This function is called when an item in the combobox's dropdown list
 * is pressed. It updates the combobox's main text to match the selected
 * item's text, closes the dropdown, and moves the cursor to the end of
 * the entry.
 *
 * @param data Unused.
 * @param obj The combobox Evas_Object.
 * @param info The pressed Elm_Object_Item.
 */
static void
_cb_pressed_cb(void        *data EINA_UNUSED,
               Evas_Object *obj,
               void        *info)
{
   const char *txt;

   txt = elm_object_item_text_get(info);
   elm_object_text_set(obj, txt);
   elm_combobox_hover_end(obj);
   elm_entry_cursor_end_set(obj);
}

/**
 * @brief Test function for an Edje external combobox.
 *
 * This function creates a window and a layout containing an external
 * combobox widget. It populates the combobox with a list of strings
 * from an array and sets up a callback to handle item selection.
 * This demonstrates how to programmatically control an external
 * combobox defined in an Edje file.
 * The `info` array holds the string values for the combobox items, e.g.:
 * `{"Label", "Button", "Combobox", ...}`
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_external_combobox(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *ly, *cb, *edj;
   char buf[PATH_MAX];
   Elm_Genlist_Item_Class *itc;
   const char *info[] = {
      "Label", "Button", "Combobox", "Icon", "Scroller", "Layout",
      "Naviframe", "Bubble"
   };
   const unsigned int size = EINA_C_ARRAY_LENGTH(info);
   unsigned int i;

   win = elm_win_util_standard_add("ext_combobox", "Edje External Combobox");
   elm_win_autodel_set(win, EINA_TRUE);

   ly = elm_layout_add(win);
   snprintf(buf, sizeof(buf), "%s/objects/test_external.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "external/combobox");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, ly);
   evas_object_show(ly);

   itc = elm_genlist_item_class_new();
   itc->item_style = "default";
   itc->func.text_get = text_get_cb;

   edj = elm_layout_edje_get(ly);
   cb = edje_object_part_external_object_get(edj, "combobox");
   evas_object_smart_callback_add(cb, "item,pressed", _cb_pressed_cb, NULL);

   for (i = 0; i < size; i++)
     {
        elm_genlist_item_append(cb, itc, info[i], NULL,
                                ELM_GENLIST_ITEM_NONE, action_cb, ly);
     }

   elm_genlist_item_class_free(itc);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
