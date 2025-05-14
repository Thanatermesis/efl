#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Creates a window to test separators.
 * @ingroup elementary_tests
 *
 * This function demonstrates the usage of the elm_separator widget. It creates
 * a window with four buttons arranged in a 2x2 grid-like layout.
 * A main horizontal box holds two vertical boxes, with a vertical separator
 * between them. Each vertical box, in turn, contains two buttons with a
 * horizontal separator between them. This layout effectively demonstrates both
 * vertical and horizontal separator usage.
 *
 * @param data The data passed to the callback function. Unused in this test.
 * @param obj The object that triggered the event. Unused in this test.
 * @param event_info The event-specific information. Unused in this test.
 */
void
test_separator(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx0, *bx, *bt, *sp;

   win = elm_win_util_standard_add("separators", "Separators");
   elm_win_autodel_set(win, EINA_TRUE);

   bx0 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx0, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bx0, EINA_TRUE);
   elm_win_resize_object_add(win, bx0);
   evas_object_show(bx0);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Left upper corner");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   sp = elm_separator_add(win);
   elm_separator_horizontal_set(sp, EINA_TRUE); // by default, separator is vertical, we must set it horizontal
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Left lower corner");
   elm_object_disabled_set(bt, EINA_TRUE);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   sp = elm_separator_add(win); // now we need vertical separator
   elm_box_pack_end(bx0, sp);
   evas_object_show(sp);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Right upper corner");
   elm_object_disabled_set(bt, EINA_TRUE);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   sp = elm_separator_add(win);
   elm_separator_horizontal_set(sp, EINA_TRUE);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Right lower corner");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_show(win);
}
