#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback for the date_changed event of the datepicker.
 *
 * This function is called whenever the user changes the date in the
 * datepicker widget. It retrieves the newly selected date and prints
 * it to the console.
 *
 * @param data User data pointer (unused).
 * @param ev The event information. ev->object is the datepicker widget.
 */
static void
_date_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   int year, month, day;

   efl_ui_datepicker_date_get(ev->object, &year, &month, &day);
   printf("Current date is %d %d %d\n", year, month, day);
}

/**
 * @brief Creates a datepicker test window.
 *
 * This function sets up a new window containing an Efl.Ui.Datepicker widget.
 * The datepicker is initialized with a specific date, and constrained by
 * minimum and maximum dates. A callback is registered to print the selected
 * date whenever it's changed by the user.
 *
 * This is an elementary test function. The parameters are passed by the
 * test infrastructure.
 *
 * @param data Unused user data.
 * @param obj Unused parent object.
 * @param event_info Unused event information.
 */
void
test_ui_datepicker(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Datepicker"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added),
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));

   efl_add(EFL_UI_DATEPICKER_CLASS, bx,
           efl_ui_datepicker_date_set(efl_added, 1987, 9, 17),
           efl_ui_datepicker_date_max_set(efl_added, 1990, 9, 17),
           efl_ui_datepicker_date_min_set(efl_added, 1980, 9, 17),
           efl_event_callback_add(efl_added, EFL_UI_DATEPICKER_EVENT_DATE_CHANGED,_date_changed_cb, NULL),
           efl_pack(bx, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(150, 170));
}
