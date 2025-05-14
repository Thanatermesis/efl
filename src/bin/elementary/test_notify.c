#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function to show a notification.
 * @param data The notification widget to show.
 * @param obj The object that triggered the event.
 * @param event_info The event information.
 */
static void
_bt(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *notify = data;
   evas_object_show(notify);
   elm_object_focus_set(notify, EINA_TRUE);
}

/**
 * @brief Callback function to hide a notification.
 * @param data The notification widget to hide.
 * @param obj The object that triggered the event.
 * @param event_info The event information.
 */
static void
_bt_close(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *notify = data;
   evas_object_hide(notify);
}

/**
 * @brief Callback function to set a timeout for closing a notification.
 * This function sets a 2-second timeout on the notification, after which it will be hidden.
 * @param data The notification widget.
 * @param obj The object that triggered the event.
 * @param event_info The event information.
 */
static void
_bt_timer_close(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *notify = data;
   elm_notify_timeout_set(notify, 2.0);
}

/**
 * @brief Callback function for when a notification times out.
 * This is called when the timeout set on a notification expires.
 * @param data User data.
 * @param obj The notification object.
 * @param event_info The event information.
 */
static void
_notify_timeout(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("Notify timed out!\n");
}

/**
 * @brief Callback function for clicks on the blocked event area of a notification.
 * This is triggered when events are disallowed outside the notification and the user clicks
 * in that blocked area.
 * @param data User data.
 * @param obj The notification object.
 * @param event_info The event information.
 */
static void
_notify_block(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("Notify block area clicked!!\n");
}

/**
 * @brief Callback function for key down events on the notification.
 * This is useful for making notifications interactive or dismissible with the keyboard.
 * @param data User data.
 * @param e The Evas canvas.
 * @param obj The notification object that received the event.
 * @param event_info The key down event details.
 */
static void
_notify_key_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   printf("Key down: %s\n", ev->keyname);
}

/**
 * @brief Test function for the Elementary notify widget.
 * This function creates a window with a grid of buttons. Each button demonstrates a
 * notification with different alignment, timeout, and interaction settings. It serves as
 * a visual test case for the various features of the elm_notify widget.
 * @param data User data for the test function.
 * @param obj The parent object.
 * @param event_info The event information.
 */
void
test_notify(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *notify, *bt, *lb;

   win = elm_win_util_standard_add("notify", "Notify");
   elm_win_autodel_set(win, EINA_TRUE);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb);
   evas_object_show(tb);

   // Notify top
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.5, 0.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_label_line_wrap_set(lb, ELM_WRAP_MIXED);
   elm_label_wrap_width_set(lb, ELM_SCALE_SIZE(140));
   elm_object_text_set(lb, "This position is the default. This is multiline text.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Top");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 2, 1, 1, 1);
   evas_object_show(bt);

   // Notify bottom
   notify = elm_notify_add(win);
   elm_notify_allow_events_set(notify, EINA_FALSE);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.5, 1.0);

   elm_notify_timeout_set(notify, 5.0);
   evas_object_smart_callback_add(notify, "timeout", _notify_timeout, NULL);
   evas_object_smart_callback_add(notify, "block,clicked", _notify_block, NULL);
   evas_object_event_callback_add(notify, EVAS_CALLBACK_KEY_DOWN,
                                  _notify_key_down_cb, NULL);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Bottom position. This notify uses a timeout of 5 sec.<br/>"
                       "<b>The events outside the window are blocked.</b>");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Bottom");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 2, 3, 1, 1);
   evas_object_show(bt);

   // Notify left
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.0, 0.5);
   elm_notify_timeout_set(notify, 10.0);
   evas_object_smart_callback_add(notify, "timeout", _notify_timeout, NULL);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Left position. This notify uses a timeout of 10 sec.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Left");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 1, 2, 1, 1);
   evas_object_show(bt);

   // Notify center
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.5, 0.5);
   elm_notify_timeout_set(notify, 10.0);
   evas_object_smart_callback_add(notify, "timeout", _notify_timeout, NULL);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Center position. This notify uses a timeout of 10 sec.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Center");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 2, 2, 1, 1);
   evas_object_show(bt);

   // Notify right
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 1.0, 0.5);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Right position.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Right");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 3, 2, 1, 1);
   evas_object_show(bt);

   // Notify top left
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.0, 0.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Top Left position.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Top Left");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 1, 1, 1, 1);
   evas_object_show(bt);

   // Notify top right
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 1.0, 0.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Top Right position.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Top Right");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 3, 1, 1, 1);
   evas_object_show(bt);

   // Notify bottom left
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.0, 1.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Bottom Left position.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Bottom Left");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 1, 3, 1, 1);
   evas_object_show(bt);

   // Notify bottom right
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 1.0, 1.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Bottom Right position.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close in 2s");
   evas_object_smart_callback_add(bt, "clicked", _bt_timer_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Bottom Right");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 3, 3, 1, 1);
   evas_object_show(bt);

   // Notify top fill
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 0.0);
   elm_notify_timeout_set(notify, 5.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Fill top. This notify fills horizontal area.<br/>"
         "<b>elm_notify_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 0.0); </b>");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(bt, "Top fill");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 1, 0, 3, 1);
   evas_object_show(bt);

   // Notify bottom fill
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0);
   elm_notify_timeout_set(notify, 5.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   lb = elm_label_add(win);
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lb, 0.0, 0.5);
   elm_object_text_set(lb, "Fill Bottom. This notify fills horizontal area.<br/>"
                       "<b>elm_notify_align_set(notify, ELM_NOTIFY_ALIGN_FILL, 1.0); </b>");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(bt, "Bottom fill");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 1, 4, 3, 1);
   evas_object_show(bt);

   // Notify left fill
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 0.0, EVAS_HINT_FILL);
   elm_notify_timeout_set(notify, 5.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Left fill.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, 0.5, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Left fill");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 0, 1, 1, 3);
   evas_object_show(bt);

   // Notify right fill
   notify = elm_notify_add(win);
   evas_object_size_hint_weight_set(notify, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notify, 1.0, EVAS_HINT_FILL);
   elm_notify_timeout_set(notify, 5.0);

   bx = elm_box_add(win);
   elm_object_content_set(notify, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Right fill.");
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _bt_close, notify);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   evas_object_size_hint_align_set(bt, 0.5, EVAS_HINT_FILL);
   elm_object_text_set(bt, "Right fill");
   evas_object_smart_callback_add(bt, "clicked", _bt, notify);
   elm_table_pack(tb, bt, 4, 1, 1, 3);
   evas_object_show(bt);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
