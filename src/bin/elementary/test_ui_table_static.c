#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Data for the table API test.
 *
 * This structure holds pointers to the UI elements used in the API tests.
 */
struct _Api_Data
{
   Eo *table; /**< The table widget being tested. */
   Eo *child; /**< A child widget used for packing/unpacking tests. */
};
typedef struct _Api_Data Api_Data;

/**
 * @brief Test context data.
 *
 * This structure holds the overall state for the API tests, including which
 * test is currently being run.
 */
struct _api_data
{
   unsigned int state;  /**< The current test state, corresponds to api_state enum. */
   Api_Data data;       /**< Pointers to the widgets under test. */
};
typedef struct _api_data api_data;

/**
 * @brief Defines the different states of the API test.
 *
 * Each state corresponds to a specific API function that is being tested.
 * The test cycles through these states in order.
 */
enum _api_state
{
   TABLE_PACK_SET,   /**< Test efl_pack_table() */
   TABLE_UNPACK,     /**< Test efl_pack_unpack() */
   TABLE_SIZE,       /**< Test efl_pack_table_size_get/set() */
   TABLE_CLEAR,      /**< Test efl_pack_clear() */
   API_STATE_LAST    /**< Marker for the end of the test sequence. */
};
typedef enum _api_state api_state;

/**
 * @brief Executes a table API function based on the current test state.
 *
 * @param api The test context data, containing the current state and widgets.
 *
 * This function is the core of the API test. It uses a switch statement
 * on the current state to call a specific function on the table widget.
 * This allows testing different table API calls sequentially.
 */
static void
set_api_state(api_data *api)
{
   Api_Data *dt = &api->data;
   int w, h;

   switch(api->state)
     { /* Put all api-changes under switch */
      case TABLE_PACK_SET: /* 0 */
         efl_pack_table(dt->table, dt->child, 5, 15, 60, 40);
         break;

      case TABLE_UNPACK: /* 1 */
         efl_pack_unpack(dt->table, dt->child);
         efl_del(dt->child);
         break;

      case TABLE_SIZE: /* 2 */
         efl_pack_table_size_get(dt->table, &w, &h);
         printf("size w=<%d> h=<%d>\n", w, h);
         w = h = 100; /* table size returns wrong values */
         w += 30;
         h += 10;

         efl_pack_table_size_set(dt->table, w, h);
         break;

      case TABLE_CLEAR: /* 3 */
         efl_pack_clear(dt->table);
         break;

      default:
         return;
     }
}

/**
 * @brief Callback for the "Next API function" button.
 *
 * @param data The api_data structure for the test.
 * @param ev The event information.
 *
 * This function is called when the user clicks the button to advance the
 * API test. It calls set_api_state() to execute the current test,
 * increments the state, and updates the button's text to reflect the
 * next test.
 */
static void
_api_bt_clicked(void *data, const Efl_Event *ev)
{  /* Will add here a SWITCH command containing code to modify test-object */
   /* in accordance a->state value. */
   api_data *a = data;
   char str[128];

   printf("clicked event on API Button: api_state=<%d>\n", a->state);
   set_api_state(a);
   a->state++;
   sprintf(str, "Next API function (%u)", a->state);
   elm_object_text_set(ev->object, str);
   elm_object_disabled_set(ev->object, a->state == API_STATE_LAST);
}

/**
 * @brief Callback to demonstrate dynamic modification of a child's packing.
 *
 * @param data The table widget.
 * @param ev The event information, where ev->object is the clicked child.
 *
 * When the "Change" button is clicked, this function retrieves its current
 * table cell position and size, then repacks it with slightly larger
 * dimensions and a shifted position. This demonstrates the dynamic nature
 * of the table layout.
 */
static void
_ch_table(void *data, const Efl_Event *ev)
{
   Eo *table = data;
   int x, y, w, h;

   efl_pack_table_cell_column_get(table, ev->object, &x, &w);
   efl_pack_table_cell_row_get(table, ev->object, &y, &h);
   efl_pack_table(table, ev->object, x - 1, y - 1, w + 2, h + 2);
}

/**
 * @brief Callback for window deletion.
 *
 * @param data The api_data structure to be freed.
 * @param ev The event information (unused).
 *
 * Cleans up the allocated test data when the window is closed.
 */
static void
_win_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Test case for Efl.Ui.Table_Static.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 *
 * This function sets up a window with an Efl_Ui_Table_Static widget.
 * It populates the table with various widgets (buttons, entries, rectangles)
 * to demonstrate and test its packing capabilities. It also includes an
 * interactive API test driven by a button, which cycles through various
 * table API functions like packing, unpacking, and clearing.
 */
void
test_ui_table_static(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *table, *bt, *rc, *en;
   api_data *api = calloc(1, sizeof(*api));

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                efl_ui_win_name_set(efl_added, "table"),
                efl_text_set(efl_added, "Table"),
                efl_ui_win_autodel_set(efl_added, EINA_TRUE),
                efl_event_callback_add(efl_added, EFL_EVENT_DEL, _win_del, api));

   table = efl_add(EFL_UI_TABLE_STATIC_CLASS, win);
   efl_content_set(win, table);
   api->data.table = table;

   /* FIXME: EO-ify entry & button & test_set! */

   en = elm_entry_add(win);
   elm_entry_scrollable_set(en, EINA_TRUE);
   elm_object_text_set(en, "Entry text");
   elm_entry_single_line_set(en, EINA_TRUE);
   efl_pack_table(table, en, 50, 10, 40, 10);
   efl_gfx_entity_visible_set(en, 1);

   en = elm_entry_add(win);
   elm_entry_scrollable_set(en, EINA_TRUE);
   elm_object_text_set(en, "Entry text 2");
   elm_entry_single_line_set(en, EINA_TRUE);
   efl_pack_table(table, en, 60, 20, 30, 10);
   efl_gfx_entity_visible_set(en, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   efl_event_callback_add(bt, EFL_INPUT_EVENT_CLICKED, _api_bt_clicked, api);
   efl_pack_table(table, bt, 30, 0, 40, 10);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   efl_gfx_entity_visible_set(bt, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   efl_pack_table(table, bt,  0,  0, 20, 20);
   efl_gfx_entity_visible_set(bt, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   efl_pack_table(table, bt, 10, 10, 40, 20);
   api->data.child = bt;
   efl_gfx_entity_visible_set(bt, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   efl_pack_table(table, bt, 10, 30, 20, 50);
   efl_gfx_entity_visible_set(bt, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   efl_pack_table(table, bt, 80, 80, 20, 20);
   efl_gfx_entity_visible_set(bt, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Change");
   efl_pack_table(table, bt, 40, 40, 20, 20);
   efl_event_callback_add(bt, EFL_INPUT_EVENT_CLICKED, _ch_table, table);
   efl_gfx_entity_visible_set(bt, 1);

   rc = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_color_set(rc, 128, 0, 0, 128);
   efl_pack_table(table, rc, 40, 70, 20, 10);

   rc = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_color_set(rc, 0, 128, 0, 128);
   efl_pack_table(table, rc, 60, 70, 10, 10);

   rc = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_color_set(rc, 0, 0, 128, 128);
   efl_pack_table(table, rc, 40, 80, 10, 10);

   rc = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_color_set(rc, 128, 0, 128, 128);
   efl_pack_table(table, rc, 50, 80, 10, 10);

   rc = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_color_set(rc, 128, 64, 0, 128);
   efl_pack_table(table, rc, 60, 80, 10, 10);

   efl_gfx_entity_size_set(win, EINA_SIZE2D(480,  480));
}
