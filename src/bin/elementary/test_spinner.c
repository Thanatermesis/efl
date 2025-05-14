#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function for the "spinner,drag,start" smart event.
 *
 * This function is called when the user starts dragging the spinner handle.
 *
 * @param data User data pointer (unused).
 * @param obj The spinner object that emitted the signal (unused).
 * @param event The event information (unused).
 */
static void
_spinner_drag_start_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event EINA_UNUSED)
{
  printf("spinner drag started\n");
}

/**
 * @brief Callback function for the "spinner,drag,stop" smart event.
 *
 * This function is called when the user stops dragging the spinner handle.
 *
 * @param data User data pointer (unused).
 * @param obj The spinner object that emitted the signal (unused).
 * @param event The event information (unused).
 */
static void
_spinner_drag_stop_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event EINA_UNUSED)
{
  printf("spinner drag stopped\n");
}

/**
 * @brief Create a test window for the spinner widget.
 *
 * This function creates a window and populates it with various spinner
 * widgets, each configured differently to test various features of the
 * spinner widget such as:
 * - Editable and non-editable modes.
 * - Custom label formats.
 * - Step values, min/max ranges, and wrapping.
 * - Rounding and base values.
 * - Vertical orientation.
 * - Disabled state.
 * - Special text values for specific numeric values (e.g., months).
 *
 * @param data User data pointer (unused).
 * @param obj The object that triggered this test function (unused).
 * @param event_info Event information (unused).
 */
void
test_spinner(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *sp;

   win = elm_win_util_standard_add("spinner", "Spinner");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   sp = elm_spinner_add(win);
   elm_spinner_editable_set(sp, EINA_TRUE);
   elm_spinner_label_format_set(sp, "%1.1f units");
   elm_spinner_step_set(sp, 1.3);
   elm_spinner_wrap_set(sp, EINA_TRUE);
   elm_spinner_min_max_set(sp, -5000.0, 5000.0);
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(sp, "spinner,drag,start",
                                  _spinner_drag_start_cb, NULL);
   evas_object_smart_callback_add(sp, "spinner,drag,stop",
                                  _spinner_drag_stop_cb, NULL);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   sp = elm_spinner_add(win);
   elm_spinner_label_format_set(sp, "Base 5.5, Round 2 : %1.1f");
   elm_spinner_min_max_set(sp, -100.0, 100.0);
   elm_spinner_round_set(sp, 2);
   elm_spinner_base_set(sp, 5.5);
   elm_spinner_value_set(sp, 5.5);
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   sp = elm_spinner_add(win);
   elm_spinner_label_format_set(sp, "Percentage %%%1.2f something");
   elm_spinner_step_set(sp, 5.0);
   elm_spinner_min_max_set(sp, 0.0, 100.0);
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   sp = elm_spinner_add(win);
   elm_spinner_label_format_set(sp, "%1.1f units");
   elm_spinner_step_set(sp, 1.3);
   elm_spinner_wrap_set(sp, EINA_TRUE);
   elm_object_style_set (sp, "vertical");
   elm_spinner_min_max_set(sp, -50.0, 250.0);
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   sp = elm_spinner_add(win);
   elm_spinner_label_format_set(sp, "Disabled %.0f");
   elm_object_disabled_set(sp, EINA_TRUE);
   elm_spinner_min_max_set(sp, -50.0, 250.0);
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   sp = elm_spinner_add(win);
   elm_spinner_wrap_set(sp, EINA_TRUE);
   elm_spinner_min_max_set(sp, 1, 12);
   elm_spinner_value_set(sp, 1);
   elm_spinner_label_format_set(sp, "%.0f");
   elm_spinner_editable_set(sp, EINA_FALSE);
   elm_spinner_special_value_add(sp, 1, "January");
   elm_spinner_special_value_add(sp, 2, "February");
   elm_spinner_special_value_add(sp, 3, "March");
   elm_spinner_special_value_add(sp, 4, "April");
   elm_spinner_special_value_add(sp, 5, "May");
   elm_spinner_special_value_add(sp, 6, "June");
   elm_spinner_special_value_add(sp, 7, "July");
   elm_spinner_special_value_add(sp, 8, "August");
   elm_spinner_special_value_add(sp, 9, "September");
   elm_spinner_special_value_add(sp, 10, "October");
   elm_spinner_special_value_add(sp, 11, "November");
   elm_spinner_special_value_add(sp, 12, "December");
   evas_object_size_hint_align_set(sp, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(sp, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, sp);
   evas_object_show(sp);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}
