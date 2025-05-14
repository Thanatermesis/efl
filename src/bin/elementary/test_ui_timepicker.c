#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback function for the "time_changed" event of the timepicker.
 *
 * This function is called whenever the time on the timepicker is changed by the user.
 * It retrieves the new hour and minute from the timepicker widget and prints them to the console.
 *
 * @param data User data pointer (unused in this case).
 * @param ev The event information structure.
 */
static void
_time_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   int hour, min;

   efl_ui_timepicker_time_get(ev->object, &hour, &min);
   printf("Current time is %d %d\n", hour, min);
}


/**
 * @brief Test function for Efl.Ui.Timepicker.
 *
 * This function creates a new window and adds two timepicker widgets to it.
 * The first timepicker uses the default 12-hour format.
 * The second timepicker is configured to use the 24-hour format.
 * Both are initialized to 11:35.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_ui_timepicker(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Timepicker"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added),
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));

   efl_add(EFL_UI_TIMEPICKER_CLASS, bx,
           efl_ui_timepicker_time_set(efl_added, 11, 35),
           efl_event_callback_add(efl_added, EFL_UI_TIMEPICKER_EVENT_TIME_CHANGED,_time_changed_cb, NULL),
           efl_pack(bx, efl_added));

   efl_add(EFL_UI_TIMEPICKER_CLASS, bx,
           efl_ui_timepicker_time_set(efl_added, 11, 35),
           efl_ui_timepicker_is_24hour_set(efl_added, EINA_TRUE),
           efl_event_callback_add(efl_added, EFL_UI_TIMEPICKER_EVENT_TIME_CHANGED,_time_changed_cb, NULL),
           efl_pack(bx, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(150, 170));
}
