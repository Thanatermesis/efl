#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#include "test.h"
#endif
#include <Elementary.h>

/**
 * @brief Structure to hold data for the API tests.
 *
 * This structure contains the current state of the test and a pointer
 * to the container box of the clock widget being tested.
 */
struct _api_data
{
   unsigned int state;  /**< What state we are testing */
   Evas_Object *box;    /**< Used in set_api_state */
};
typedef struct _api_data api_data;

/**
 * @brief Enumeration of API test states for the clock widget.
 *
 * Each value corresponds to a specific API function or a combination
 * of them to be tested on the clock widget.
 */
enum _api_state
{
   CLOCK_HIDE_SEC,
   CLOCK_SHOW_AM_PM,
   CLOCK_SHOW_SEC,
   CLOCK_EDIT_MIN,
   CLOCK_EDIT_HOUR,
   CLOCK_EDIT_ALL,
   CLOCK_HIDE_AM_PM,
   API_STATE_LAST
};
typedef enum _api_state api_state;

/**
 * @brief Array of strings describing the API test states.
 *
 * This array maps the enum _api_state values to human-readable strings
 * for display and debugging purposes. The order of strings must match the
 * order of values in the _api_state enum.
 *
 * @details The array elements correspond to the following tests:
 * - "Hide Sec": Test for `elm_clock_show_seconds_set(ck, EINA_FALSE)`.
 * - "Show AM/PM": Test for `elm_clock_show_am_pm_set(ck, EINA_TRUE)`.
 * - "Show Sec": Test for `elm_clock_show_seconds_set(ck, EINA_TRUE)`.
 * - "Edit Min": Test for setting edit mode to minutes only.
 * - "Edit Hour": Test for setting edit mode to hours only.
 * - "Edit All": Test for setting edit mode to all fields.
 * - "Hide AM/PM": Test for `elm_clock_show_am_pm_set(ck, EINA_FALSE)`.
 */
static const char* api_state_description[] = {
   "Hide Sec",
   "Show AM/PM",
   "Show Sec",
   "Edit Min",
   "Edit Hour",
   "Edit All",
   "Hide AM/PM",
   NULL
};

/**
 * @brief Apply a specific API test state to the clock widget.
 * @param api The API test data structure, containing the state to apply.
 *
 * This function gets the first clock widget from the box in `api->box`
 * and applies a configuration to it based on `api->state`. This is
 * used to cycle through different clock API tests.
 */
static void
set_api_state(api_data *api)
{
   const Eina_List *items = elm_box_children_get(api->box);
   Evas_Object *ck = eina_list_nth(items, 0);
   if (!eina_list_count(items))
     return;

   /* use elm_box_children_get() to get list of children */
   switch(api->state)
     { /* Put all api-changes under switch */
      case CLOCK_HIDE_SEC:
        elm_clock_show_seconds_set(ck, EINA_FALSE);
        break;

      case CLOCK_SHOW_AM_PM:
        elm_clock_show_am_pm_set(ck,  EINA_TRUE);
        break;

      case CLOCK_SHOW_SEC:
        elm_clock_show_seconds_set(ck, EINA_TRUE);
        break;

      case CLOCK_EDIT_MIN:
        elm_clock_edit_set(ck, EINA_TRUE);
        elm_clock_edit_mode_set(ck, ELM_CLOCK_EDIT_MIN_DECIMAL | ELM_CLOCK_EDIT_MIN_UNIT);
        break;

      case CLOCK_EDIT_HOUR:
        elm_clock_edit_set(ck, EINA_TRUE);
        elm_clock_edit_mode_set(ck, ELM_CLOCK_EDIT_HOUR_DECIMAL | ELM_CLOCK_EDIT_HOUR_UNIT);
        break;

      case CLOCK_EDIT_ALL:
        elm_clock_edit_set(ck, EINA_TRUE);
        elm_clock_edit_mode_set(ck, ELM_CLOCK_EDIT_ALL);
        break;

      case CLOCK_HIDE_AM_PM:
        elm_clock_show_am_pm_set(ck,  EINA_FALSE);
        break;

      case API_STATE_LAST:

        break;
      default:
        return;
     }
}

/**
 * @brief Callback for the "Next API function" button click.
 * @param data A pointer to the api_data structure.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 *
 * This function is called when the "Next API function" button is clicked.
 * It advances the API test state, applies the new state to the clock widget,
 * and updates the button's text to show the next state. When all states
 * have been tested, it disables the button.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   api_data *a = data;
   char str[128];

   printf("clicked event on API Button: api_state=<%s>\n", api_state_description[a->state]);
   set_api_state(a);
   a->state++;
   sprintf(str, "Next API function (%u)", a->state);
   elm_object_text_set(obj, str);
   elm_object_disabled_set(obj, a->state == API_STATE_LAST);
}

/**
 * @brief Callback to free allocated memory when the window is destroyed.
 * @param data A pointer to the data to be freed (api_data structure).
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function is registered with the EVAS_CALLBACK_FREE event on the
 * window. It is responsible for freeing the `api_data` structure
 * allocated for the test.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Main test function for the clock widget.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window and displays multiple instances of the
 * clock widget with different configurations to test various features
 * like AM/PM mode, seconds display, and edit mode. It also includes
 * a button to cycle through API tests on one of the clock instances.
 */
void
test_clock(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *ck, *bt, *bxx;
   unsigned int digedit;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("clock", "Clock");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   bx = elm_box_add(bxx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   api->box = bx;
   evas_object_show(bx);

   bt = elm_button_add(bxx);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, EINA_TRUE);
   evas_object_show(bt);

   elm_box_pack_end(bxx, bx);

   ck = elm_clock_add(bx);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_show_am_pm_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   elm_clock_time_set(ck, 17, 25, 0);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_show_am_pm_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   elm_clock_time_set(ck, 11, 59, 57);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_show_am_pm_set(ck, EINA_FALSE);
   elm_clock_time_set(ck, 23, 59, 57);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_show_am_pm_set(ck, EINA_TRUE);
   elm_clock_time_set(ck, 10, 11, 12);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(bx);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_edit_set(ck, EINA_TRUE);
   digedit = ELM_CLOCK_EDIT_HOUR_UNIT | ELM_CLOCK_EDIT_MIN_UNIT | ELM_CLOCK_EDIT_SEC_UNIT;
   elm_clock_edit_mode_set(ck, digedit);
   elm_box_pack_end(bx, ck);
   elm_clock_time_set(ck, 0, 0, 0);
   evas_object_show(ck);

   evas_object_show(win);
}

/**
 * @brief Callback to toggle the edit mode of a clock widget.
 * @param data A pointer to the clock widget.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 *
 * This function is called when the "Edit"/"Done" button is clicked. It
 * toggles the clock's edit mode via `elm_clock_edit_set()` and updates
 * the button's label accordingly.
 */
static void
_edit_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ck = data;

   if (!elm_clock_edit_get(ck))
     {
        elm_object_text_set(obj, "Done");
        elm_clock_edit_set(ck, EINA_TRUE);
        return;
     }
   elm_object_text_set(obj, "Edit");
   elm_clock_edit_set(ck, EINA_FALSE);
}

/**
 * @brief Callback to toggle the 12/24 hour mode of a clock widget.
 * @param data A pointer to the clock widget.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 *
 * This function is called when the "12h"/"24h" button is clicked. It
 * toggles the AM/PM display on the clock via `elm_clock_show_am_pm_set()`
 * and updates the button's label accordingly.
 */
static void
_hmode_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ck = data;

   if (!elm_clock_show_am_pm_get(ck))
     {
        elm_object_text_set(obj, "24h");
        elm_clock_show_am_pm_set(ck, EINA_TRUE);
        return;
     }
   elm_object_text_set(obj, "12h");
   elm_clock_show_am_pm_set(ck, EINA_FALSE);
}

/**
 * @brief Timer callback to pause the clock.
 * @param data A pointer to the clock widget to be paused.
 * @return EINA_FALSE to stop the timer from recurring.
 *
 * This function is called by a timer. It pauses the given clock widget
 * and updates the corresponding button's text to "Resume".
 */
static Eina_Bool
_pause_cb(void *data)
{
   Evas_Object *ck = data;
   Evas_Object *bt = evas_object_data_get(ck, "_button");
   elm_clock_pause_set(ck, EINA_TRUE);
   elm_object_text_set(bt, "Resume");
   return EINA_FALSE;
}

/**
 * @brief Callback to pause or resume a clock widget.
 * @param data A pointer to the clock widget.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 *
 * Toggles the paused state of the clock. When resuming, it also starts a
 * 2-second timer that will pause the clock again automatically. This is
 * to test the pause functionality.
 */
static void
_pause_resume_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *ck = data;

   if (!elm_clock_pause_get(ck))
     {
        elm_object_text_set(obj, "Resume");
        elm_clock_pause_set(ck, EINA_TRUE);
        return;
     }
   elm_object_text_set(obj, "Pause");
   elm_clock_pause_set(ck, EINA_FALSE);
   ecore_timer_add(2, _pause_cb, ck);
}

/**
 * @brief Test function for clock editing features.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window with a clock widget and buttons to test
 * its editing capabilities. It includes a button to cycle through various
 * API states related to editing, and buttons to toggle edit mode and
 * 12/24 hour format.
 */
void
test_clock_edit(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *hbx, *ck, *bt, *bxx;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("clock-edit", "Clock Edit");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   bx = elm_box_add(bxx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   api->box = bx;
   evas_object_show(bx);

   bt = elm_button_add(bxx);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   elm_box_pack_end(bxx, bx);

   ck = elm_clock_add(bx);
   elm_clock_time_set(ck, 0, 15, 3);
   elm_clock_edit_mode_set(ck, ELM_CLOCK_EDIT_DEFAULT);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_show_am_pm_set(ck, EINA_TRUE);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   hbx = elm_box_add(bx);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   elm_box_pack_end(bx, hbx);
   evas_object_show(hbx);

   bt = elm_button_add(hbx);
   elm_object_text_set(bt, "Edit");
   evas_object_smart_callback_add(bt, "clicked", _edit_bt_clicked, ck);
   elm_box_pack_end(hbx, bt);
   evas_object_show(bt);
   _edit_bt_clicked(ck, bt, NULL);

   bt = elm_button_add(hbx);
   elm_object_text_set(bt, "24h");
   evas_object_smart_callback_add(bt, "clicked", _hmode_bt_clicked, ck);
   elm_box_pack_end(hbx, bt);
   evas_object_show(bt);

   evas_object_show(win);
}

/**
 * @brief Test function for clock's "first interval" feature in edit mode.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function tests the `elm_clock_first_interval_set()` API. It creates
 * three clock widgets in edit mode, each with a different first interval
 * value (0.4s, 1.2s, 2.0s). This interval is the delay before the time value
 * starts changing continuously when an up/down arrow is held down.
 */
void
test_clock_edit2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *ck, *lb;

   win = elm_win_util_standard_add("clock-edit2", "Clock Edit 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "Check time updates for a user mouse button hold."
                       );
   evas_object_size_hint_weight_set(lb, 0.0, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   ck = elm_clock_add(win);
   elm_clock_show_seconds_set(ck, 1);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_first_interval_set(ck, 0.4);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(win);
   elm_clock_show_seconds_set(ck, 1);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_first_interval_set(ck, 1.2);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   ck = elm_clock_add(win);
   elm_clock_show_seconds_set(ck, 1);
   elm_clock_edit_set(ck, EINA_TRUE);
   elm_clock_first_interval_set(ck, 2.0);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   evas_object_show(win);
}

/**
 * @brief Test function for clock's pause and resume functionality.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window with a clock and a "Pause" button.
 * Clicking the button toggles the clock's paused state. The test also
 * includes a timer that automatically pauses the clock 2 seconds after it's
 * been resumed, to demonstrate the functionality.
 */
void
test_clock_pause(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *ck, *bt;

   win = elm_win_util_standard_add("clock4", "Clock4");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   ck = elm_clock_add(win);
   elm_clock_show_seconds_set(ck, EINA_TRUE);
   elm_clock_time_set(ck, 0, 0, 0);
   elm_box_pack_end(bx, ck);
   evas_object_show(ck);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Pause");
   evas_object_smart_callback_add(bt, "clicked", _pause_resume_bt_clicked, ck);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_data_set(ck, "_button", bt);
   ecore_timer_add(2, _pause_cb, ck);

   evas_object_show(win);
}
