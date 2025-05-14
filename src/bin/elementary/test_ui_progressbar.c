#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Structure to hold all the widgets and timer for the progressbar test UI.
 *
 * This struct is passed around as user data to callbacks.
 */
typedef struct _pbdata
{
   Eo *win; /**< The main window. */
   Eo *pb1; /**< Progressbar with an image as content. */
   Eo *pb2; /**< Progressbar in infinite mode. */
   Eo *pb3; /**< Progressbar with a togglable progress label. */
   Eo *pb4; /**< Progressbar with a custom format string. */
   Eo *pb5; /**< Progressbar with a custom format function. */
   Eo *check; /**< Checkbox to toggle the label visibility on pb3. */
   Eo *btn_start; /**< Button to start the progress animation. */
   Eo *btn_stop; /**< Button to stop the progress animation. */
   Eo *btn_reset; /**< Button to reset all progressbars to their initial state. */
   Ecore_Timer *timer; /**< Timer to drive the progress animation. */
} pbdata;

/**
 * @brief Increments the value of a progressbar.
 *
 * @param pb The progressbar widget.
 * @param inc_value The value to increment the progress by.
 * @return EINA_TRUE when progress reaches 100.0, EINA_FALSE otherwise.
 *         The return value is used to decide whether to renew an Ecore_Timer.
 */
static Eina_Bool
_set_progress_val(Eo *pb, double inc_value)
{
   double progress;

   progress = efl_ui_range_value_get(pb);

   if (progress < 100.0)
     progress += inc_value;
   else
     return EINA_TRUE;

   efl_ui_range_value_set(pb, progress);

   return EINA_FALSE;
}

/**
 * @brief Timer callback to update progressbars' values.
 *
 * This function is called periodically to simulate progress. It updates
 * several progressbars and then checks if the main progressbar (pb1) has
 * completed.
 *
 * @param d User data, a pointer to a pbdata struct.
 * @return ECORE_CALLBACK_RENEW to continue the timer, or ECORE_CALLBACK_CANCEL to stop it.
 */
static Eina_Bool
_pb_timer_cb(void *d)
{
   pbdata *pd = d;
   double progress_val;

   if (!pd) return ECORE_CALLBACK_CANCEL;

   progress_val = efl_ui_range_value_get(pd->pb1);
   if (progress_val < 100.0)
     {
        progress_val += 1;
        efl_ui_range_value_set(pd->pb2, progress_val);
        efl_ui_range_value_set(pd->pb3, progress_val);
        efl_ui_range_value_set(pd->pb4, progress_val);
        efl_ui_range_value_set(pd->pb5, progress_val);
     }

   if (!_set_progress_val(pd->pb1, 0.5))
     return ECORE_CALLBACK_RENEW;

   pd->timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback for the "start" button's clicked event.
 *
 * Disables the start button, enables the stop button, and starts the
 * animation timer if it's not already running.
 *
 * @param d User data, a pointer to a pbdata struct.
 * @param ev The event information.
 */
static void
_start_btn_clicked_cb(void *d, const Efl_Event *ev EINA_UNUSED)
{
   pbdata *pd = d;

   if (!pd) return;

   printf("start button is clicked\n");

   efl_ui_widget_disabled_set(pd->btn_start, EINA_TRUE);
   efl_ui_widget_disabled_set(pd->btn_stop, EINA_FALSE);

   if (!pd->timer) pd->timer = ecore_timer_add(0.1, _pb_timer_cb, pd);
}

/**
 * @brief Callback for the "stop" button's clicked event.
 *
 * Enables the start button, disables the stop button, and stops (deletes)
 * the animation timer if it is running.
 *
 * @param d User data, a pointer to a pbdata struct.
 * @param ev The event information.
 */
static void
_stop_btn_clicked_cb(void *d, const Efl_Event *ev EINA_UNUSED)
{
   pbdata *pd = d;

   if (!pd) return;
   printf("stop button is clicked\n");

   efl_ui_widget_disabled_set(pd->btn_start, EINA_FALSE);
   efl_ui_widget_disabled_set(pd->btn_stop, EINA_TRUE);

   if (pd->timer)
     {
        ecore_timer_del(pd->timer);
        pd->timer = NULL;
     }
}

/**
 * @brief Callback for the "reset" button's clicked event.
 *
 * Resets the value of all progressbars to 0.0.
 *
 * @param d User data, a pointer to a pbdata struct.
 * @param ev The event information.
 */
static void
_reset_btn_clicked_cb(void *d, const Efl_Event *ev EINA_UNUSED)
{
   pbdata *pd = d;

   if (!pd) return;
   printf("reset button is clicked\n");

   efl_ui_range_value_set(pd->pb1, 0.0);
   efl_ui_range_value_set(pd->pb2, 0.0);
   efl_ui_range_value_set(pd->pb3, 0.0);
   efl_ui_range_value_set(pd->pb4, 0.0);
   efl_ui_range_value_set(pd->pb5, 0.0);
}

/**
 * @brief Callback for the window's delete request event.
 *
 * Cleans up resources, including the animation timer and the pbdata struct.
 *
 * @param d User data, a pointer to a pbdata struct.
 * @param ev The event information.
 */
static void
_win_delete_req_cb(void *d, const Efl_Event *ev EINA_UNUSED)
{
   pbdata *pd = d;

   if (pd->timer) ecore_timer_del(pd->timer);
   efl_unref(pd->win);
   free(pd);
}

/**
 * @brief Custom formatting function for a progressbar label.
 *
 * Provides different text strings for the label based on the progress value.
 *
 * @param data User data (unused).
 * @param str The string buffer to append the formatted string to.
 * @param value The current progress value.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_custom_format_cb(void *data EINA_UNUSED, Eina_Strbuf *str, const Eina_Value value)
{
   double v;
   eina_value_get(&value, &v);
   if (v < 25.f) eina_strbuf_append_printf(str, "Starting up...");
   else if (v < 50.f) eina_strbuf_append_printf(str, "Working...");
   else if (v < 75.f) eina_strbuf_append_printf(str, "Getting there...");
   else if (v < 100.f) eina_strbuf_append_printf(str, "Almost done...");
   else eina_strbuf_append_printf(str, "Done!");
   return EINA_TRUE;
}

/**
 * @brief Callback for the checkbox's "selected,changed" event.
 *
 * Toggles the visibility of the progress label on a progressbar based on the
 * checkbox's state.
 *
 * @param data The progressbar widget (pb3) whose label will be toggled.
 * @param ev The event information, containing the checkbox object.
 */
static void
_toggle_progress_label(void *data, const Efl_Event *ev)
{
   Efl_Ui_Check *check = ev->object;
   Efl_Ui_Progressbar *pb3 = data;
   Eina_Bool state = efl_ui_selectable_selected_get(check);

   efl_ui_progressbar_show_progress_label_set(pb3, state);
}

/**
 * @brief Test function for Efl.Ui.Progressbar.
 *
 * This function creates a window with several progressbar widgets to demonstrate
 * different features like custom labels, infinite mode, and user interaction
 * via buttons.
 */
void
test_ui_progressbar(void *data EINA_UNUSED, Eo *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx, *btbx;
   pbdata *pd;
   char buf[PATH_MAX];

   pd = (pbdata *)calloc(1, sizeof(pbdata));

   pd->win = win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                                      efl_text_set(efl_added, "Efl.Ui.Progressbar"),
                           efl_ui_win_autodel_set(efl_added, EINA_TRUE),
                           efl_event_callback_add(efl_added, EFL_UI_WIN_EVENT_DELETE_REQUEST,
                                                  _win_delete_req_cb, pd)
                          );

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added)
               );

   pd->pb1 = efl_add(EFL_UI_PROGRESSBAR_CLASS, bx,
                     efl_pack(bx, efl_added),
                     efl_text_set(efl_added, "percent"),
                     efl_ui_range_limits_set(efl_added, 0, 100),
                     efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20))
                    );

   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   efl_add(EFL_UI_IMAGE_CLASS, pd->pb1,
           efl_file_set(efl_added, buf),
           efl_file_load(efl_added),
           efl_content_set(pd->pb1, efl_added)
          );

   pd->pb2 = efl_add(EFL_UI_PROGRESSBAR_CLASS, bx,
                     efl_pack(bx, efl_added),
                     efl_text_set(efl_added, "10-100"),
                     efl_ui_progressbar_infinite_mode_set(efl_added, EINA_TRUE),
                     efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20)),
                     efl_ui_range_limits_set(efl_added, 10, 100),
                     efl_ui_range_value_set(efl_added, 10)
                    );

   pd->pb3 = efl_add(EFL_UI_PROGRESSBAR_CLASS, bx,
                     efl_pack(bx, efl_added),
                     efl_text_set(efl_added, "Toggle progress label"),
                     efl_ui_range_limits_set(efl_added, 0, 100),
                     efl_ui_progressbar_show_progress_label_set(efl_added, EINA_FALSE),
                     efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20))
                    );
   pd->check = efl_add(EFL_UI_CHECK_CLASS, bx,
                       efl_pack(bx, efl_added),
                       efl_event_callback_add(efl_added, EFL_UI_EVENT_SELECTED_CHANGED,
                                              _toggle_progress_label, pd->pb3),
                       efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20))
                      );
   efl_text_set(pd->check, "Show progress label of above progressbar"),

   pd->pb4 = efl_add(EFL_UI_PROGRESSBAR_CLASS, bx,
                     efl_pack(bx, efl_added),
                     efl_text_set(efl_added, "Custom string"),
                     efl_ui_range_limits_set(efl_added, 0, 100),
                     efl_ui_format_string_set(efl_added, "%d rabbits", EFL_UI_FORMAT_STRING_TYPE_SIMPLE),
                     efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20))
                    );

   pd->pb5 = efl_add(EFL_UI_PROGRESSBAR_CLASS, bx,
                     efl_pack(bx, efl_added),
                     efl_text_set(efl_added, "Custom func"),
                     efl_ui_range_limits_set(efl_added, 0, 100),
                     efl_ui_format_func_set(efl_added, NULL, _custom_format_cb, NULL),
                     efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(250, 20))
                    );

   btbx = efl_add(EFL_UI_BOX_CLASS, bx,
                  efl_pack(bx, efl_added),
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL)
                 );

   pd->btn_start = efl_add(EFL_UI_BUTTON_CLASS, btbx,
                           efl_text_set(efl_added, "start"),
                           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED,
                                                  _start_btn_clicked_cb, pd),
                           efl_pack(btbx, efl_added)
                          );

   pd->btn_stop = efl_add(EFL_UI_BUTTON_CLASS, btbx,
                          efl_text_set(efl_added, "stop"),
                          efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED,
                                                 _stop_btn_clicked_cb, pd),
                          efl_pack(btbx, efl_added)
                         );
   pd->btn_reset = efl_add(EFL_UI_BUTTON_CLASS, btbx,
                           efl_text_set(efl_added, "reset"),
                           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED,
                                                  _reset_btn_clicked_cb, pd),
                           efl_pack(btbx, efl_added)
                          );
}
