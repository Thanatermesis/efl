#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Structure to hold private data for the progressbar test.
 *
 * This structure contains pointers to all the progressbar widgets,
 * control buttons, and the timer used for animations. It is passed
 * as user data to callbacks.
 */
typedef struct Progressbar_Data
{
   Evas_Object *pb1;
   Evas_Object *pb2;
   Evas_Object *pb3;
   Evas_Object *pb4;
   Evas_Object *pb5;
   Evas_Object *pb6;
   Evas_Object *pb7;
   Evas_Object *pb8;
   Ecore_Timer *timer;
   Evas_Object *btn_start;
   Evas_Object *btn_stop;
} Progressbar_Data;

/**
 * @brief Timer callback to animate the progress bars in the first test.
 * @param data The Progressbar_Data structure.
 * @return ECORE_CALLBACK_RENEW to continue the timer, ECORE_CALLBACK_CANCEL to stop.
 *
 * This function is called repeatedly by an Ecore_Timer to increment
 * the value of several progress bars, simulating progress. When the
 * progress reaches 1.0, it resets to 0.0 and the timer is stopped.
 */
static Eina_Bool
_my_progressbar_value_set(void *data)
{
   double progress;
   Progressbar_Data *pd = data;

   progress = elm_progressbar_value_get (pd->pb1);
   if (progress < 1.0) progress += 0.0123;
   else progress = 0.0;
   elm_progressbar_value_set(pd->pb1, progress);
   elm_progressbar_value_set(pd->pb4, progress);
   elm_progressbar_value_set(pd->pb3, progress);
   elm_progressbar_value_set(pd->pb6, progress);

   if (progress < 1.0) return ECORE_CALLBACK_RENEW;

   pd->timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback function for the "Start" button in the first test.
 * @param data The Progressbar_Data structure.
 * @param obj The button object that was clicked.
 * @param event_info Extra event information.
 *
 * Starts the animation for the progress bars. This includes setting
 * some progress bars to "pulse" mode and starting a timer to update
 * the value of others. It also disables the start button and enables
 * the stop button.
 */
static void
my_progressbar_test_start(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Progressbar_Data *pd = data;
   if (!pd) return;

   fprintf(stderr, "s1\n");
   elm_progressbar_pulse(pd->pb2, EINA_TRUE);
   fprintf(stderr, "s2\n");
   elm_progressbar_pulse(pd->pb5, EINA_TRUE);
   fprintf(stderr, "s3 %p\n", pd->pb7);
   elm_progressbar_pulse(pd->pb7, EINA_TRUE);
   fprintf(stderr, "s4 %p\n", pd->pb8);
   elm_progressbar_pulse(pd->pb8, EINA_TRUE);
   fprintf(stderr, "s5\n");

   elm_object_disabled_set(pd->btn_start, EINA_TRUE);
   elm_object_disabled_set(pd->btn_stop, EINA_FALSE);

   if (!pd->timer)
     pd->timer = ecore_timer_add(0.1, _my_progressbar_value_set, pd);
}

/**
 * @brief Callback function for the "Stop" button in the first test.
 * @param data The Progressbar_Data structure.
 * @param obj The button object that was clicked.
 * @param event_info Extra event information.
 *
 * Stops the animation for the progress bars. This includes stopping
 * the "pulse" mode and deleting the timer. It also enables the start
 * button and disables the stop button.
 */
static void
my_progressbar_test_stop(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Progressbar_Data *pd = data;
   if (!pd) return;

   elm_progressbar_pulse(pd->pb2, EINA_FALSE);
   elm_progressbar_pulse(pd->pb5, EINA_FALSE);
   elm_progressbar_pulse(pd->pb7, EINA_FALSE);
   elm_progressbar_pulse(pd->pb8, EINA_FALSE);
   elm_object_disabled_set(pd->btn_start, EINA_FALSE);
   elm_object_disabled_set(pd->btn_stop, EINA_TRUE);

   if (pd->timer)
     {
        ecore_timer_del(pd->timer);
        pd->timer = NULL;
     }
}

/**
 * @brief Callback for the window's "delete,request" event.
 * @param data The Progressbar_Data structure.
 * @param obj The window object.
 * @param event_info Extra event information.
 *
 * This function is responsible for cleaning up resources when the
 * window is closed. It deletes the timer, frees the Progressbar_Data
 * structure, stops any running animations, and deletes the window object.
 */
static void
_progressbar_destroy_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Progressbar_Data *pd = data;
   if (!pd) return;

   ecore_timer_del(pd->timer);
   free(pd);
   my_progressbar_test_stop(NULL, NULL, NULL);
   evas_object_del(obj);
}

/**
 * @brief Custom formatting function for a progress bar's unit label.
 * @param val The current progress value, from 0.0 to 1.0.
 * @return A newly allocated string with the formatted text. The caller is
 *         responsible for freeing this string.
 *
 * This function formats the progress value to a string that shows a
 * countdown of "files left", simulating a file operation.
 * For example, if val is 0.25, it will show "10500 files left".
 */
static char *
my_progressbar_format_cb(double val)
{
   char buf[1024];
   int files;

   files = (1-val)*14000;
   if(snprintf(buf, 30, "%i files left", files) > 0)
     return strdup(buf);
   return NULL;
}

/**
 * @brief Main function for the first progressbar test.
 *
 * Creates a window and populates it with various progressbar widgets
 * to test different features and styles:
 * - A standard horizontal progressbar with a custom unit format function.
 * - A horizontal "infinite bounce" (pulsing) progressbar.
 * - An inverted horizontal progressbar with an icon and custom text.
 * - A vertical progressbar with percentage formatting.
 * - A vertical "infinite bounce" (pulsing) progressbar.
 * - An inverted vertical progressbar with an icon.
 * - "wheel" style progressbars, horizontal and vertical.
 * It also adds "Start" and "Stop" buttons to control animations.
 */
void
test_progressbar(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *pb, *bx, *hbx, *bt, *bt_bx, *ic1, *ic2;
   char buf[PATH_MAX], form[20];
   Progressbar_Data *pd;

   pd = (Progressbar_Data *)calloc(1, sizeof(Progressbar_Data));

   win = elm_win_util_standard_add("progressbar", "Progressbar");
   evas_object_smart_callback_add(win, "delete,request",
                                  _progressbar_destroy_cb, pd);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   pb = elm_progressbar_add(win);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, pb);
   //elm_progressbar_horizontal_set(pb, EINA_TRUE);
   //elm_object_text_set(pb, "Progression %");
   elm_progressbar_unit_format_function_set(pb, my_progressbar_format_cb,
                                            (void (*)(char *)) free);
   evas_object_show(pb);
   pd->pb1 = pb;

   pb = elm_progressbar_add(win);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_object_text_set(pb, "Infinite bounce");
   elm_progressbar_pulse_set(pb, EINA_TRUE);
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb2 = pb;

   ic1 = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic1, buf, NULL);
   evas_object_size_hint_aspect_set(ic1, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

   pb = elm_progressbar_add(win);
   elm_object_text_set(pb, "Label");
   elm_object_part_content_set(pb, "icon", ic1);
   elm_progressbar_inverted_set(pb, EINA_TRUE);
   elm_progressbar_unit_format_set(pb, "%1.1f units");
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(bx, pb);
   evas_object_show(ic1);
   evas_object_show(pb);
   pd->pb3 = pb;

   hbx = elm_box_add(win);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, hbx);
   evas_object_show(hbx);

   snprintf(form, sizeof(form), "%d percent (%d%%%%)", 50, 50);

   pb = elm_progressbar_add(win);
   elm_progressbar_horizontal_set(pb, EINA_FALSE);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbx, pb);
   elm_progressbar_unit_format_set(pb, form);
   elm_progressbar_value_set(pb, .50);
   elm_object_text_set(pb, "percent");
   evas_object_show(pb);
   pd->pb4 = pb;

   pb = elm_progressbar_add(win);
   elm_progressbar_horizontal_set(pb, EINA_FALSE);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(80));
   elm_progressbar_pulse_set(pb, EINA_TRUE);
   elm_progressbar_unit_format_set(pb, NULL);
   elm_object_text_set(pb, "Infinite bounce");
   elm_box_pack_end(hbx, pb);
   evas_object_show(pb);
   pd->pb5 = pb;

   ic2 = elm_icon_add(win);
   elm_image_file_set(ic2, buf, NULL);
   evas_object_size_hint_aspect_set(ic2, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);

   pb = elm_progressbar_add(win);
   elm_progressbar_horizontal_set(pb, EINA_FALSE);
   elm_object_text_set(pb, "Label");
   elm_object_part_content_set(pb, "icon", ic2);
   elm_progressbar_inverted_set(pb, EINA_TRUE);
   elm_progressbar_unit_format_set(pb, "%1.2f%%");
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(hbx, pb);
   evas_object_show(ic2);
   evas_object_show(pb);
   pd->pb6 = pb;

   pb = elm_progressbar_add(win);
   elm_object_style_set(pb, "wheel");
   elm_object_text_set(pb, "Style: wheel");
   elm_progressbar_pulse_set(pb, EINA_TRUE);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb7 = pb;

   pb = elm_progressbar_add(win);
   elm_progressbar_horizontal_set(pb, EINA_FALSE);
   elm_object_style_set(pb, "wheel");
   elm_object_text_set(pb, "Style: wheel vert");
   elm_progressbar_pulse_set(pb, EINA_TRUE);
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb8 = pb;

   bt_bx = elm_box_add(win);
   elm_box_horizontal_set(bt_bx, EINA_TRUE);
   evas_object_size_hint_weight_set(bt_bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, bt_bx);
   evas_object_show(bt_bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Start");
   evas_object_smart_callback_add(bt, "clicked", my_progressbar_test_start, pd);
   elm_box_pack_end(bt_bx, bt);
   evas_object_show(bt);
   pd->btn_start = bt;

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Stop");
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_smart_callback_add(bt, "clicked", my_progressbar_test_stop, pd);
   elm_box_pack_end(bt_bx, bt);
   evas_object_show(bt);
   pd->btn_stop = bt;

   evas_object_show(win);
}

/**
 * @brief Helper function to set the value of a specific progressbar part.
 * @param pb The progressbar object.
 * @param inc_value The value to increment the progress by.
 * @param part_name The name of the progressbar part to update (e.g., "elm.cur.progressbar").
 * @return EINA_TRUE if progress is complete (>= 1.0), EINA_FALSE otherwise.
 *
 * This is used for progressbars that have multiple parts, like the "double" style.
 */
static Eina_Bool
_set_progress_val(Evas_Object *pb, double inc_value, char *part_name)
{
   double progress;

   progress = elm_progressbar_part_value_get(pb, part_name);

   if (progress < 1.0)
     progress += inc_value;
   else
     return EINA_TRUE;

   elm_progressbar_part_value_set(pb, part_name, progress);

   return EINA_FALSE;
}

/**
 * @brief Timer callback for the second progressbar test.
 * @param data The Progressbar_Data structure.
 * @return ECORE_CALLBACK_RENEW to continue, ECORE_CALLBACK_CANCEL to stop.
 *
 * This timer callback updates multiple parts of different progressbars
 * at different rates, demonstrating the multi-part progress feature.
 */
static Eina_Bool
_progressbar2_timer_cb(void *data)
{
   Progressbar_Data *pd = data;
   double progress;

   if (!pd) return ECORE_CALLBACK_CANCEL;

   progress = elm_progressbar_value_get (pd->pb1);

   if (progress < 1.0)
     {
        progress += 0.0123;
        elm_progressbar_part_value_set(pd->pb1, "elm.cur.progressbar", progress);
        elm_progressbar_part_value_set(pd->pb2, "elm.cur.progressbar1", progress);
        elm_progressbar_part_value_set(pd->pb3, "elm.cur.progressbar1", progress);
        elm_progressbar_part_value_set(pd->pb4, "elm.cur.progressbar1", progress);
     }

   _set_progress_val(pd->pb2, 0.00723, "elm.cur.progressbar");
   _set_progress_val(pd->pb3, 0.00523, "elm.cur.progressbar");

   if (!_set_progress_val(pd->pb4, 0.00423, "elm.cur.progressbar"))
     return ECORE_CALLBACK_RENEW;

   pd->timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback function for the "Start" button in the second test.
 * @param data The Progressbar_Data structure.
 * @param obj The button object that was clicked.
 * @param event_info Extra event information.
 *
 * Starts the timer for the second progressbar test.
 */
static void
_pg2_start_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Progressbar_Data *pd = data;
   if (!pd) return;

   fprintf(stderr, "s1\n");

   elm_object_disabled_set(pd->btn_start, EINA_TRUE);
   elm_object_disabled_set(pd->btn_stop, EINA_FALSE);

   if (!pd->timer)
     pd->timer = ecore_timer_add(0.1, _progressbar2_timer_cb, pd);
}

/**
 * @brief Callback function for the "Stop" button in the second test.
 * @param data The Progressbar_Data structure.
 * @param obj The button object that was clicked.
 * @param event_info Extra event information.
 *
 * Stops the timer for the second progressbar test.
 */
static void
_pg2_stop_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Progressbar_Data *pd = data;
   if (!pd) return;

   elm_object_disabled_set(pd->btn_start, EINA_FALSE);
   elm_object_disabled_set(pd->btn_stop, EINA_TRUE);

   if (pd->timer)
     {
        ecore_timer_del(pd->timer);
        pd->timer = NULL;
     }
}

/**
 * @brief Main function for the second progressbar test.
 *
 * Creates a window to test progressbars with multiple indicator parts.
 * It sets up several progressbars with the "double" style, where two
 * different progress values can be displayed simultaneously on the same
 * widget. A timer is used to update these parts at different rates.
 */
void
test_progressbar2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *pb, *bx, *bt, *bt_bx;
   Progressbar_Data *pd;

   pd = (Progressbar_Data *)calloc(1, sizeof(Progressbar_Data));

   win = elm_win_util_standard_add("progressbar", "Progressbar2");
   evas_object_smart_callback_add(win, "delete,request",
                                  _progressbar_destroy_cb, pd);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   pb = elm_progressbar_add(win);
   elm_object_text_set(pb, "Style: default");
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb1 = pb;

   pb = elm_progressbar_add(win);
   elm_object_style_set(pb, "double");
   elm_object_text_set(pb, "Style: double");
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb2 = pb;

   pb = elm_progressbar_add(win);
   elm_object_style_set(pb, "double");
   elm_object_text_set(pb, "Style: double 2");
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb3 = pb;

   pb = elm_progressbar_add(win);
   elm_object_style_set(pb, "double");
   elm_progressbar_horizontal_set(pb, EINA_FALSE);
   elm_object_text_set(pb, "Style: Vertical");
   evas_object_size_hint_weight_set(pb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(pb, EVAS_HINT_FILL, 0.5);
   elm_progressbar_span_size_set(pb, ELM_SCALE_SIZE(200));
   elm_box_pack_end(bx, pb);
   evas_object_show(pb);
   pd->pb4 = pb;

   bt_bx = elm_box_add(win);
   elm_box_horizontal_set(bt_bx, EINA_TRUE);
   evas_object_size_hint_weight_set(bt_bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, bt_bx);
   evas_object_show(bt_bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Start");
   evas_object_smart_callback_add(bt, "clicked", _pg2_start_btn_clicked_cb, pd);
   elm_box_pack_end(bt_bx, bt);
   evas_object_show(bt);
   pd->btn_start = bt;

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Stop");
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_smart_callback_add(bt, "clicked", _pg2_stop_btn_clicked_cb, pd);
   elm_box_pack_end(bt_bx, bt);
   evas_object_show(bt);
   pd->btn_stop = bt;

   evas_object_show(win);
}
