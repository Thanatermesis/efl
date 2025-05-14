#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

static Eina_Bool eb;

/**
 * @brief Callback for when a check widget's state changes.
 *
 * This function synchronizes the state of a second check widget (`ck2`)
 * with the one that triggered the callback (`obj`). It also prints the
 * states of both check widgets for debugging purposes.
 *
 * @param data A pointer to the second check widget (ck2).
 * @param obj The check widget that changed state.
 * @param event_info Evas event info (unused).
 */
static void
changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ck2 = data;
   printf("ck %p to %i\n", obj, elm_check_state_get(obj));
   elm_check_state_set(ck2, elm_check_state_get(obj));
   printf("ck2 %p is now %i\n", ck2, elm_check_state_get(ck2));
}

/**
 * @brief Callback to demonstrate state pointer usage.
 *
 * This callback is triggered on a "changed" event and prints the value
 * of the global boolean variable `eb`. This is used to test that changes
 * to a check widget update the variable linked via
 * elm_check_state_pointer_set().
 *
 * @param data User data (unused).
 * @param obj The object that triggered the callback (unused).
 * @param event_info Evas event info (unused).
 */
static void
state_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("State Pointer Value: %d\n", eb);
}

/**
 * @brief Callback to change widget text based on state.
 *
 * This callback shows or hides the text of a check widget based on its
 * checked state. When checked, a descriptive text is shown. When unchecked,
 * the text is cleared.
 *
 * @param data User data (unused).
 * @param obj The check widget whose text will be changed.
 * @param event_info Evas event info (unused).
 */
static void
state_changed_cb2(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   if (elm_check_state_get(obj) == EINA_FALSE)
      elm_object_text_set(obj, NULL);
   else
      elm_object_text_set(obj, "text is visible when check state is true.");
}

/**
 * @brief Create and display a column of check widgets with a specific style.
 *
 * This function populates a given box container (`bx`) with a variety of
 * pre-configured check widgets. All created check widgets will share the
 * same specified `style`.
 *
 * The configurations include:
 * - Icon sized to the check.
 * - Icon with a fixed size.
 * - Label only.
 * - Using a state pointer.
 * - Disabled check with icon and label.
 * - Disabled check with no label or icon.
 * - ...and more, to test various visual states and functionalities.
 *
 * @param win The parent window.
 * @param bx The box container to pack the check widgets into.
 * @param style The visual style to apply to all created check widgets
 *              (e.g., "default", "plain", "icon").
 */
static void
check_style(Evas_Object *win, Evas_Object *bx, const char *style)
{
   Evas_Object *ic, *ck, *ck0;
   char buf[PATH_MAX];

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Icon sized to check");
   elm_object_part_content_set(ck, "icon", ic);
   elm_check_state_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
   evas_object_show(ic);

   ck0 = ck;

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Icon not resizable");
   elm_object_part_content_set(ck, "icon", ic);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
   evas_object_show(ic);

   evas_object_smart_callback_add(ck, "changed", changed_cb, ck0);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Label Only");
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Use State Pointer");
   elm_check_state_pointer_set(ck, &eb);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Print State Pointer Value");
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
   evas_object_smart_callback_add(ck, "changed", state_changed_cb, NULL);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_text_set(ck, "Disabled check");
   elm_object_part_content_set(ck, "icon", ic);
   elm_check_state_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   elm_object_disabled_set(ck, EINA_TRUE);
   evas_object_show(ck);
   evas_object_show(ic);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_box_pack_end(bx, ck);
   elm_object_disabled_set(ck, EINA_TRUE);
   evas_object_show(ck);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_object_part_content_set(ck, "icon", ic);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);
   evas_object_show(ic);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_style_set(ck, style);
   elm_box_pack_end(bx, ck);
   elm_object_text_set(ck, "text is visible when check state is true.");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_show(ck);
   evas_object_smart_callback_add(ck, "changed", state_changed_cb2, NULL);
}

/**
 * @brief Test function for standard (non-toggle) check widgets.
 *
 * This function creates a window containing three vertical columns of check
 * widgets. Each column demonstrates a different style: "default", "plain",
 * and "icon". It serves as the main entry point for testing the visual
 * appearance and basic functionality of elm_check widgets.
 *
 * @param data User data (unused).
 * @param obj The object that triggered the test (unused).
 * @param event_info Evas event info (unused).
 */
void
test_check(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx0;

   win = elm_win_util_standard_add("check", "Check");
   elm_win_autodel_set(win, EINA_TRUE);

   bx0 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx0, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bx0, EINA_TRUE);
   elm_win_resize_object_add(win, bx0);
   evas_object_show(bx0);

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_FALSE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   check_style(win, bx, "default");

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_FALSE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   check_style(win, bx, "plain");

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_FALSE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx0, bx);
   evas_object_show(bx);

   check_style(win, bx, "icon");

   evas_object_show(win);
}

/**
 * @brief Test function for check widgets with the "toggle" style.
 *
 * This function creates a window to specifically test the "toggle" switch
 * style of elm_check. It displays various configurations of toggle widgets,
 * including:
 * - With an icon.
 * - With custom "on"/"off" labels.
 * - Disabled.
 * - With different label lengths.
 *
 * @param data User data (unused).
 * @param obj The object that triggered the test (unused).
 * @param event_info Evas event info (unused).
 */
void
test_check_toggle(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *ic, *tg;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("check-toggle", "Check Toggle");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

   tg = elm_check_add(win);
   elm_object_style_set(tg, "toggle");
   elm_object_text_set(tg, "Icon sized to toggle");
   elm_object_part_content_set(tg, "icon", ic);
   elm_check_state_set(tg, EINA_TRUE);
   elm_object_part_text_set(tg, "on", "Yes");
   elm_object_part_text_set(tg, "off", "No");
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);

   tg = elm_check_add(win);
   elm_object_style_set(tg, "toggle");
   elm_object_text_set(tg, "Icon no scale");
   elm_object_part_content_set(tg, "icon", ic);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);

   tg = elm_check_add(win);
   elm_object_style_set(tg, "toggle");
   elm_object_text_set(tg, "Disabled toggle");
   elm_object_part_content_set(tg, "icon", ic);
   elm_object_disabled_set(tg, EINA_TRUE);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   tg = elm_check_add(win);
   elm_object_style_set(tg, "toggle");
   elm_object_text_set(tg, "Label Only");
   elm_object_part_text_set(tg, "on", "Big long fun times label");
   elm_object_part_text_set(tg, "off", "Small long happy fun label");
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);

   tg = elm_check_add(win);
   elm_object_style_set(tg, "toggle");
   elm_object_part_content_set(tg, "icon", ic);
   elm_box_pack_end(bx, tg);
   evas_object_show(tg);
   evas_object_show(ic);

   evas_object_show(win);
}
