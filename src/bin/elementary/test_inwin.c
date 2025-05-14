#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Structure to hold data for the inwin API test.
 * This structure contains the current state of the test and a pointer to the
 * inwin widget.
 */
struct _api_data
{
   unsigned int state;  /**< Current test state, corresponds to api_state enum. */
   void *inwin;         /**< The inwin widget being tested. */
};
typedef struct _api_data api_data;

/**
 * @brief Defines the different states for the inwin API test.
 * These states are used to cycle through different API function tests.
 */
enum _api_state
{
   CONTENT_UNSET, /**< State to test unsetting the inwin content. */
   API_STATE_LAST /**< Marker for the last state, used to end the test cycle. */
};
typedef enum _api_state api_state;

/**
 * @brief Executes a test case based on the current API state.
 *
 * This function modifies the inwin widget based on the state stored in @p api.
 * It's part of a sequence of tests that are run by clicking a button in the UI.
 *
 * @param api The API test data, containing the current state and the inwin object.
 */
static void
set_api_state(api_data *api)
{
   Evas_Object *t;
   switch(api->state)
     {
      case CONTENT_UNSET:
         t = elm_win_inwin_content_unset(api->inwin);
         evas_object_del(t);
         t = elm_label_add(elm_object_parent_widget_get(api->inwin));
         elm_object_text_set(t, "Content was unset.<br>DONE!");
         elm_win_inwin_content_set(api->inwin, t);
         evas_object_show(t);

      default:
         return;
     }
}

/**
 * @brief Callback for the "Next API function" button's "clicked" event.
 *
 * This function is called when the user clicks the button to proceed to the
 * next test case. It calls set_api_state() to perform the test for the
 * current state, then increments the state for the next click. It also
 * updates the button's text to reflect the next state and disables the
 * button when all tests are done.
 *
 * @param data The api_data struct.
 * @param obj The button object that was clicked.
 * @param event_info Not used.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   api_data *a = data;
   char str[128];

   printf("clicked event on API Button: api_state=<%d>\n", a->state);
   set_api_state(a);
   a->state++;
   sprintf(str, "Next API function (%u)", a->state);
   elm_object_text_set(obj, str);
   elm_object_disabled_set(obj, a->state == API_STATE_LAST);
}

/**
 * @brief Callback to free the api_data struct when the window is freed.
 * @param data The api_data struct to free.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Test case for inwin widget API.
 *
 * This test creates a window with an inwin widget. The inwin contains a
 * button that allows cycling through different API function tests on the
 * inwin. This demonstrates the basic usage and programmatic manipulation of
 * an inwin.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_inwin(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *inwin, *lb, *bxx, *bt;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("inwin", "InWin");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   inwin = elm_win_inwin_add(win);
   api->inwin = inwin;
   evas_object_show(inwin);

   bxx = elm_box_add(inwin);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bxx);

   bt = elm_button_add(inwin);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This is an \"inwin\" - a window in a<br/>"
                       "window. This is handy for quick popups<br/>"
                       "you want centered, taking over the window<br/>"
                       "until dismissed somehow. Unlike hovers they<br/>"
                       "don't hover over their target.");
   elm_box_pack_end(bxx, lb);
   elm_win_inwin_content_set(inwin, bxx);
   evas_object_show(lb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           240 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for inwin with "minimal_vertical" style.
 *
 * This test demonstrates an inwin widget using the "minimal_vertical" style.
 * This style makes the inwin compact itself vertically to the minimum size
 * of its content.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_inwin2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *inwin, *lb;

   win = elm_win_util_standard_add("inwin2", "InWin 2");
   elm_win_autodel_set(win, EINA_TRUE);

   inwin = elm_win_inwin_add(win);
   elm_object_style_set(inwin, "minimal_vertical");
   evas_object_show(inwin);

   lb = elm_label_add(win);
   elm_object_text_set(lb,
                       "This is an \"inwin\" - a window in a<br/>"
                       "window. This is handy for quick popups<br/>"
                       "you want centered, taking over the window<br/>"
                       "until dismissed somehow. Unlike hovers they<br/>"
                       "don't hover over their target.<br/>"
                       "<br/>"
                       "This \"minimal_vertical\" inwin style compacts<br/>"
                       "itself vertically to the size of its contents<br/> "
                       "minimum size.");
   elm_win_inwin_content_set(inwin, lb);
   evas_object_show(lb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           240 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Another test case for "minimal_vertical" style inwin.
 *
 * This test shows an inwin with the "minimal_vertical" style. The content
 * is a box containing two buttons, demonstrating how the inwin sizes to
 * fit multiple child objects.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_inwin3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *inwin, *lb, *box;

   win = elm_win_util_standard_add("inwin2", "InWin 2");
   elm_win_autodel_set(win, EINA_TRUE);

   inwin = elm_win_inwin_add(win);
   elm_object_style_set(inwin, "minimal_vertical");
   evas_object_show(inwin);

   box = elm_box_add(win);
   elm_win_inwin_content_set(inwin, box);
   evas_object_show(box);

   lb = elm_button_add(box);
   elm_object_text_set(lb,
                       "Test BTN 1");
   elm_box_pack_end(box, lb);
   evas_object_show(lb);

   lb = elm_button_add(box);
   elm_object_text_set(lb,
                       "Test BTN 1");
   elm_box_pack_end(box, lb);
   evas_object_show(lb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           240 * elm_config_scale_get());
   evas_object_show(win);
}
