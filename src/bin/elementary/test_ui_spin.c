#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

#define STEP_SIZE 2

/**
 * @brief Callback for when the spinner value changes.
 *
 * This function is invoked whenever the value of the spinner widget is modified
 * by the user or programmatically. It prints the new value to stdout.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure.
 */
static void
_spin_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   printf("Value changed %d\n", (int)efl_ui_range_value_get(ev->object));
}

/**
 * @brief Callback for when the spinner reaches its minimum value.
 *
 * This function is invoked when the spinner's value attempts to go below
 * its defined minimum limit. It prints the minimum value to stdout.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure.
 */
static void
_spin_min_reached_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   printf("Min reached %d\n", (int)efl_ui_range_value_get(ev->object));
}

/**
 * @brief Callback for when the spinner reaches its maximum value.
 *
 * This function is invoked when the spinner's value attempts to go above
 * its defined maximum limit. It prints the maximum value to stdout.
 *
 * @param data User data pointer (unused).
 * @param ev The event information structure.
 */
static void
_spin_max_reached_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   printf("Max reached %d\n", (int)efl_ui_range_value_get(ev->object));
}

/**
 * @brief Callback for the increment button's click event.
 *
 * This function increases the spinner's value by a predefined STEP_SIZE.
 *
 * @param data A pointer to the spinner widget.
 * @param ev The event information structure (unused).
 */
static void
_inc_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   efl_ui_range_value_set(data, (efl_ui_range_value_get(data) + STEP_SIZE));
}

/**
 * @brief Callback for the decrement button's click event.
 *
 * This function decreases the spinner's value by a predefined STEP_SIZE.
 *
 * @param data A pointer to the spinner widget.
 * @param ev The event information structure (unused).
 */
static void
_dec_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   efl_ui_range_value_set(data, (efl_ui_range_value_get(data) - STEP_SIZE));
}

/**
 * @brief Test function for the Efl.Ui.Spin widget.
 *
 * This function creates a window containing a spinner widget and two buttons.
 * The buttons allow for incrementing and decrementing the spinner's value.
 * Callbacks are set up to report value changes and when range limits are reached.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_ui_spin(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *bx, *sp;

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Spin"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_content_set(win, efl_added),
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));

   sp = efl_add(EFL_UI_SPIN_CLASS, bx,
                efl_ui_range_limits_set(efl_added, 0, 10),
                efl_ui_range_value_set(efl_added, 6),
                efl_ui_format_string_set(efl_added, "test %d", EFL_UI_FORMAT_STRING_TYPE_SIMPLE),
                efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_CHANGED,_spin_changed_cb, NULL),
                efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_MIN_REACHED,_spin_min_reached_cb, NULL),
                efl_event_callback_add(efl_added, EFL_UI_RANGE_EVENT_MAX_REACHED,_spin_max_reached_cb, NULL),
                efl_pack(bx, efl_added));

   efl_add(EFL_UI_BUTTON_CLASS, bx,
           efl_text_set(efl_added, "Increse Spinner value"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _inc_clicked, sp),
           efl_pack(bx, efl_added));

   efl_add(EFL_UI_BUTTON_CLASS, bx,
           efl_text_set(efl_added, "Decrease Spinner value"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _dec_clicked, sp),
           efl_pack(bx, efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(100, 120));
}
