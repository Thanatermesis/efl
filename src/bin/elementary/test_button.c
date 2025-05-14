#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback for the "repeated" event of a button.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal.
 * @param event_info The event-specific information (unused).
 *
 * This function is called when a button's "repeated" event is triggered.
 * It increments a static counter and updates the button's text to show
 * the current count. The counter resets to 0 after reaching 9999.
 */
static void
_bt_repeated(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   static int count;
   char buf[16];

   snprintf(buf, sizeof(buf), "count=%d", count++);
   if (count >= 10000)
     count = 0;
   elm_object_text_set(obj, buf);
}

/**
 * @brief Callback for the "clicked" event of a button.
 * @param data User data pointer, cast to an integer for identification.
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information (unused).
 *
 * This function is called when a button is clicked. It prints a message
 * to standard output, indicating which button was clicked based on the
 * integer value passed in @p data.
 */
static void
_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   int param = (int)(uintptr_t)(data);

   printf("clicked event on Button:%d\n", param);
}

/**
 * @brief An array of strings used as labels for a button.
 *
 * The `_bt_clicked_label_change_cb` callback cycles through these labels
 * when the associated button is clicked.
 * The elements are:
 * - "Label Only"
 * - "Label Only Test"
 * - "Label Only Test 2"
 * - "Short"
 */
#define NUM_LABEL 4
const char *btn_label[NUM_LABEL] =
{
   "Label Only", "Label Only Test", "Label Only Test 2", "Short"
};

/**
 * @brief Callback to change a button's label on click.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object (button) that was clicked.
 * @param event_info The event-specific information (unused).
 *
 * This function cycles through the labels defined in the global `btn_label`
 * array and sets the button's text to the next label in the sequence.
 */
static void
_bt_clicked_label_change_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   static int i = 1;
   elm_object_text_set(obj, btn_label[i]);
   i++;
   if (i >= NUM_LABEL) i = 0;
}

/**
 * @brief Callback for the "unpressed" event of a button.
 * @param data User data pointer, cast to an integer for identification.
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information (unused).
 *
 * This function is called when a button is released after being pressed.
 * It prints a message to standard output, indicating which button was
 * unpressed based on the integer value passed in @p data.
 */
static void
_bt_unpressed(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   int param = (int)(uintptr_t)(data);

   printf("unpressed event on Button:%d\n", param);
}

/**
 * @brief Main function to create and run the button test window.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information (unused).
 *
 * This function sets up a new window with a scroller and a box layout.
 * It then populates the box with various `elm_button` widgets to test
 * different styles, content (icons, labels), states (disabled), and
 * events (clicked, unpressed, repeated).
 */
void
test_button(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *scr, *bx, *ic, *bt;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("buttons", "Buttons");
   elm_win_autodel_set(win, EINA_TRUE);

   scr = elm_scroller_add(win);
   elm_scroller_bounce_set(scr, EINA_FALSE, EINA_TRUE);
   elm_scroller_policy_set(scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
   evas_object_size_hint_weight_set(scr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, scr);
   evas_object_show(scr);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_content_set(scr, bx);
   evas_object_show(bx);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Icon sized to button");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)1);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)1);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(bx);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Icon no scale");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)2);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)2);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Disabled Button");
   elm_object_part_content_set(bt, "icon", ic);
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)3);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)3);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_part_content_set(bt, "icon", ic);
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)4);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)4);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Label Only");
   evas_object_smart_callback_add(bt, "clicked",
                                  _bt_clicked_label_change_cb, NULL);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)5);
   elm_box_pack_end(bx, bt);
   evas_object_smart_callback_add(bt, "repeated", _bt_repeated, NULL);
   elm_button_autorepeat_set(bt, EINA_TRUE);
   elm_button_autorepeat_initial_timeout_set(bt, 2.0);
   elm_button_autorepeat_gap_timeout_set(bt, 0.5);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)6);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)6);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_style_set(bt, "anchor");
   elm_object_text_set(bt, "Anchor style");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)7);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)7);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_style_set(bt, "overlay");
   elm_object_text_set(bt, "Overlay style");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)77);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)77);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "overlay");
   elm_object_part_content_set(bt, "icon", ic);
   elm_object_text_set(bt, "Overlay style");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)77);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)77);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "overlay");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)77);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)77);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_style_set(bt, "blank");
   elm_object_text_set(bt, "Blank style");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)88);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)88);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "blank");
   elm_object_part_content_set(bt, "icon", ic);
   elm_object_text_set(bt, "Blank style");
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)88);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)88);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "blank");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)88);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)88);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "anchor");
   elm_object_text_set(bt, "Icon sized to button");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)8);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)8);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(bx);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "anchor");
   elm_object_text_set(bt, "Icon no scale");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)9);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)9);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_style_set(bt, "anchor");
   elm_object_part_content_set(bt, "icon", ic);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)10);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)10);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   bt = elm_button_add(win);
   elm_object_access_info_set(bt, "This is some custom text describing this widget");
   elm_object_style_set(bt, "anchor");
   elm_object_text_set(bt, "Disabled");
   elm_object_part_content_set(bt, "icon", ic);
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_smart_callback_add(bt, "clicked", _bt_clicked, (void *)11);
   evas_object_smart_callback_add(bt, "unpressed", _bt_unpressed, (void *)11);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}
