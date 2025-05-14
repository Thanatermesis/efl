#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#include <Elementary_Cursor.h>

/**
 * @brief Structure to hold data for the layout API tests.
 *
 * This structure maintains the state of the test, references to the
 * layout objects being tested, and references to child objects
 * that are manipulated during the tests.
 */
struct _api_data
{
   unsigned int state;  /**< The current test case from _api_state */
   Evas_Object *box_layout; /**< The layout object with a box part */
   Evas_Object *table_layout; /**< The layout object with a table part */
   Evas_Object *ref; /**< A reference object for insertion/removal tests in box_layout */
   Evas_Object *ref2; /**< A reference object for unpack tests in table_layout */
};
typedef struct _api_data api_data;

/**
 * @brief Defines the different states for the API tests.
 *
 * Each state corresponds to a specific elm_layout_* function call
 * that will be tested when the "Next API function" button is clicked.
 */
enum _api_state
{
   LAYOUT_BOX_INSERT_AT,
   LAYOUT_BOX_INSERT_BEFORE,
   LAYOUT_BOX_PREPEND,
   LAYOUT_BOX_REMOVE,
   LAYOUT_BOX_REMOVE_ALL,
   LAYOUT_TABLE_UNPACK,
   LAYOUT_TABLE_CLEAR,
   API_STATE_LAST
};

typedef enum _api_state api_state;

/**
 * @brief Callback for button clicks in the first test window.
 *
 * When a button is clicked, this function takes the button's text and
 * sets it as the text for the "text" part of the main layout object.
 *
 * @param data The layout object to modify.
 * @param obj The button that was clicked.
 * @param event_info Not used.
 */
static void
_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   elm_object_part_text_set(data, "text", elm_object_text_get(obj));
}

/**
 * @brief Callback for signals emitted by the layout.
 *
 * This function is registered to receive all signals ("*", "*") from the
 * layout and prints the emission and source of the signal to stdout.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param emission The signal string.
 * @param source The source of the signal.
 */
static void
_cb_signal(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission, const char *source)
{
   printf("signal: '%s' '%s'\n", emission, source);
}

/**
 * @brief Creates the main test window for `elm_layout`.
 *
 * This test function sets up a window demonstrating basic elm_layout usage.
 * It shows:
 * - Setting a layout from a theme.
 * - Setting part text and icons.
 * - Setting a layout from a custom EDJE file.
 * - Swallowing objects into layout parts.
 * - Setting a cursor for a layout part.
 * - Handling signals from the layout.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_layout(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *box, *ly, *bt;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("layout", "Layout");
   elm_win_autodel_set(win, EINA_TRUE);

   box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   ly = elm_layout_add(box);

   if (!elm_layout_theme_set(
         ly, "layout", "application", "titlebar"))
     fprintf(stderr, "Failed to set layout");

   elm_object_part_text_set(ly, "elm.text", "Some title");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, ly);
   evas_object_show(ly);

   bt = elm_icon_add(ly);
   elm_icon_standard_set(bt, "chat");
   evas_object_size_hint_min_set(bt, 20, 20);
   elm_layout_icon_set(ly, bt);

   bt = elm_icon_add(ly);
   elm_icon_standard_set(bt, "close");
   evas_object_size_hint_min_set(bt, 20, 20);
   elm_layout_end_set(ly, bt);

   ly = elm_layout_add(box);
   snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
   elm_layout_file_set(ly, buf, "layout");
   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(box, ly);
   evas_object_show(ly);

   elm_layout_signal_callback_add(ly, "*", "*", _cb_signal, NULL);

   bt = elm_button_add(ly);
   elm_object_text_set(bt, "Button 1");
   elm_object_part_content_set(ly, "element1", bt);
   evas_object_smart_callback_add(bt, "clicked", _clicked_cb, ly);

   bt = elm_button_add(ly);
   elm_object_text_set(bt, "Button 2");
   elm_object_part_content_set(ly, "element2", bt);
   evas_object_smart_callback_add(bt, "clicked", _clicked_cb, ly);

   bt = elm_button_add(ly);
   elm_object_text_set(bt, "Button 3");
   elm_object_part_content_set(ly, "element3", bt);
   evas_object_smart_callback_add(bt, "clicked", _clicked_cb, ly);

   elm_layout_part_cursor_set(ly, "text", ELM_CURSOR_WATCH);

   evas_object_show(win);
}

/**
 * @brief Executes a layout API function based on the current test state.
 *
 * This function is called to perform a specific layout operation corresponding
 * to the value of @p api->state. It demonstrates various box and table
 * manipulation functions of `elm_layout`.
 *
 * @param api The test state data.
 */
static void
set_api_state(api_data *api)
{
   Evas_Object *box_layout = api->box_layout;
   Evas_Object *table_layout = api->table_layout;
   Evas_Object *obj;

   /* use elm_box_children_get() to get list of children */
   switch (api->state)
     { /* Put all api-changes under switch */
      case LAYOUT_BOX_INSERT_AT:
        {
           obj = elm_label_add(box_layout);
           elm_object_text_set(obj, "elm_layout_box_insert_at(); is called");
           evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
                                            EVAS_HINT_EXPAND);
           elm_layout_box_insert_at(box_layout, "elm.box.content", obj, 0);
           evas_object_show(obj);
           break;
        }

      case LAYOUT_BOX_INSERT_BEFORE:
        {
           obj = elm_label_add(box_layout);
           elm_object_text_set(obj, "elm_layout_box_insert_before(); is called");
           evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
                                            EVAS_HINT_EXPAND);
           elm_layout_box_insert_before(box_layout, "elm.box.content",
                                        obj, api->ref);
           evas_object_show(obj);
           break;
        }

      case LAYOUT_BOX_PREPEND:
        {
           obj = elm_label_add(box_layout);
           elm_object_text_set(obj, "elm_layout_box_prepend(); is called");
           evas_object_size_hint_weight_set(obj, EVAS_HINT_EXPAND,
                                            EVAS_HINT_EXPAND);
           elm_layout_box_prepend(box_layout, "elm.box.content", obj);
           evas_object_show(obj);
           break;
        }

      case LAYOUT_BOX_REMOVE:
        {
           elm_layout_box_remove(box_layout, "elm.box.content", api->ref);
           evas_object_del(api->ref);
           break;
        }

      case LAYOUT_BOX_REMOVE_ALL:
        {
           elm_layout_box_remove_all(box_layout, "elm.box.content", EINA_TRUE);
           break;
        }

      case LAYOUT_TABLE_UNPACK:
        {
           elm_layout_table_unpack(table_layout, "elm.table.content", api->ref2);
           evas_object_del(api->ref2);
           break;
        }

      case LAYOUT_TABLE_CLEAR:
        {
           elm_layout_table_clear(table_layout, "elm.table.content", EINA_TRUE);
           break;
        }

      default: return;
     }
}

/**
 * @brief Callback for the "Next API function" button click.
 *
 * This function advances the API test to the next state. It calls
 * set_api_state() to perform the test, increments the state counter,
 * updates the button's text to show the next state, and disables the
 * button when all tests are done.
 *
 * @param data The api_data struct.
 * @param obj The button object.
 * @param event_info Not used.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{  /* Will add here a SWITCH command containing code to modify test-object */
   /* in accordance a->state value. */
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
 * @brief Cleans up allocated resources.
 *
 * This callback is attached to the window's "free" event and is responsible
 * for freeing the api_data structure allocated for the test.
 *
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
 * @brief Creates a window for interactively testing layout API functions.
 *
 * This test sets up a window with two layouts (one box, one table) and
 * a button. Clicking the button cycles through a series of API tests
 * defined in _api_state, demonstrating programmatic manipulation of
 * layout contents (inserting, removing, clearing, etc.).
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_layout2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *box, *bt, *ly, *lb;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("layout2", "Layout 2");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked,
                                  (void *)api);
   elm_box_pack_end(box, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   /* Layout Box Test */
   api->box_layout = ly = elm_layout_add(win);

   if (!elm_layout_theme_set(
         ly, "layout", "application", "toolbar-vbox"))
     fprintf(stderr, "Failed to set layout");

   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, ly);
   evas_object_show(ly);

   api->ref = lb = elm_label_add(win);
   elm_object_text_set(lb, "This is a label for the box content");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(lb);

   elm_layout_box_append(ly, "elm.box.content", lb);

   /* Layout Table Test */
   api->table_layout = ly = elm_layout_add(win);

   if (!elm_layout_theme_set(
         ly, "layout", "application", "toolbar-table"))
     fprintf(stderr, "Failed to set layout");

   evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, ly);
   evas_object_show(ly);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "This is a label for the table content");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(lb);
   elm_layout_table_pack(ly, "elm.table.content", lb, 0, 0, 1, 1);

   api->ref2 = lb = elm_label_add(win);
   elm_object_text_set(lb, "col = 0, row = 1");
   evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(lb);
   elm_layout_table_pack(ly, "elm.table.content", lb, 0, 1, 1, 1);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           320 * elm_config_scale_get());
   evas_object_show(win);
}
