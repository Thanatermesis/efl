#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_PACK_LAYOUT_PROTECTED
#include <Efl_Ui.h>
#include <Elementary.h>

static void _custom_layout_update(Eo *pack, void *_pd EINA_UNUSED);

/**
 * @brief An array to hold references to UI objects created in the test.
 *
 * The first element (index 0) is a background rectangle, and the subsequent
 * elements (indices 1 to 6) are buttons placed within the table. This array
 * allows various callback functions to easily access and manipulate these
 * objects.
 *
 * The structure is as follows:
 * - objects[0]: Background Efl_Canvas_Rectangle.
 * - objects[1-6]: Efl_Ui_Button objects.
 */
static Evas_Object *objects[7] = {};

/**
 * @brief Defines different modes for setting weight hints on table children.
 *
 * This enum is used by the radio button group to control how child objects
 * within the table expand or shrink.
 */
typedef enum {
   NONE,           /**< No weight, children will not expand. */
   NONE_BUT_FILL,  /**< No weight, but table fills available space. */
   EQUAL,          /**< All children have equal weight and expand equally. */
   ONE,            /**< Only one child (objects[2]) has weight. */
   TWO,            /**< Two children (objects[2], objects[3]) have weight. */
   CUSTOM          /**< A custom layout function is used. */
} Weight_Mode;

/**
 * @brief Callback for the weight mode radio buttons.
 * @param data The Efl_Ui_Table object.
 * @param obj The radio button that triggered the event.
 * @param event_info Not used.
 *
 * This function changes the weight hints of the objects in the table based on
 * the selected radio button mode. It also handles overriding the layout function
 * for the CUSTOM mode.
 */
static void
weights_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   EFL_OPS_DEFINE(custom_layout_ops,
                  EFL_OBJECT_OP_FUNC(efl_pack_layout_update, _custom_layout_update));

   Weight_Mode mode = elm_radio_state_value_get(obj);
   Eo *table = data;

   if (mode != CUSTOM)
     efl_object_override(table, NULL);

   switch (mode)
     {
      case NONE:
        efl_gfx_hint_align_set(table, 0.5, 0.5);
        for (int i = 1; i < 7; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
      case NONE_BUT_FILL:
        efl_gfx_hint_fill_set(table, EINA_TRUE, EINA_TRUE);
        for (int i = 1; i < 7; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
      case EQUAL:
        efl_gfx_hint_align_set(table, 0.5, 0.5);
        for (int i = 1; i < 7; i++)
          efl_gfx_hint_weight_set(objects[i], 1, 1);
        break;
      case ONE:
        efl_gfx_hint_align_set(table, 0.5, 0.5);
        for (int i = 1; i < 7; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        efl_gfx_hint_weight_set(objects[2], 1, 1);
        break;
      case TWO:
        efl_gfx_hint_align_set(table, 0.5, 0.5);
        for (int i = 1; i < 7; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        efl_gfx_hint_weight_set(objects[2], 1, 1);
        efl_gfx_hint_weight_set(objects[3], 1, 1);
        break;
      case CUSTOM:
        efl_object_override(table, &custom_layout_ops);
        break;
     }
}

/**
 * @brief Callback for the user minimum size slider.
 * @param data Not used.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Sets the minimum size hint for all objects in the `objects` array.
 */
static void
user_min_slider_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);
   for (int i = 0; i < 6; i++)
     efl_gfx_hint_size_min_set(objects[i], EINA_SIZE2D(val, val));
}

/**
 * @brief Callback for the padding slider.
 * @param data The Efl_Ui_Table object.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Sets the internal content padding for the table.
 */
static void
padding_slider_cb(void *data, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);
   efl_gfx_arrangement_content_padding_set(data, val, val);
}

/**
 * @brief Callback for the table margin slider.
 * @param data The Efl_Ui_Table object.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Sets the margin hint for the entire table.
 */
static void
margin_slider_cb(void *data, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);
   efl_gfx_hint_margin_set(data, val, val, val, val);
}

/**
 * @brief Callback for the button margins slider.
 * @param data Not used.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Sets the margin hint for each of the buttons in the table.
 */
static void
btnmargins_slider_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);
   for (int i = 1; i < 7; i++)
     efl_gfx_hint_margin_set(objects[i], val, val, val, val);
}

/**
 * @brief Callback for the vertical alignment slider.
 * @param data Not used.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Adjusts the vertical alignment hint of the first button (objects[1]).
 */
static void
alignv_slider_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   double ax, val;

   val = elm_slider_value_get(event->object);
   efl_gfx_hint_align_get(objects[1], &ax, NULL);
   efl_gfx_hint_align_set(objects[1], ax, val);
}

/**
 * @brief Callback for the horizontal alignment slider.
 * @param data Not used.
 * @param event The EFL_UI_RANGE_EVENT_CHANGED event from the slider.
 *
 * Adjusts the horizontal alignment hint of the first button (objects[1]).
 */
static void
alignh_slider_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   double ay, val;

   val = elm_slider_value_get(event->object);
   efl_gfx_hint_align_get(objects[1], NULL, &ay);
   efl_gfx_hint_align_set(objects[1], val, ay);
}

/**
 * @brief Callback for the EFL_PACK_EVENT_LAYOUT_UPDATED event on the table.
 * @param data The Elm_Label to update with layout info.
 * @param event The layout updated event.
 *
 * Updates a label to show the current number of items, columns, and rows in
 * the table whenever its layout is recalculated.
 */
static void
layout_updated_cb(void *data, const Efl_Event *event)
{
   Elm_Label *o = data;
   char buf[64];
   int rows, cols, count;

   efl_pack_table_size_get(event->object, &cols, &rows);
   count = efl_content_count(event->object);
   sprintf(buf, "%d items (%dx%d)", count, cols, rows);
   elm_object_text_set(o, buf);
}

/**
 * @brief Callback for content added/removed events on the table.
 * @param data The Elm_Label to update with event info.
 * @param event The container event.
 *
 * Updates a label with information about the child that was added or removed,
 * including its cell position and span.
 */
static void
child_evt_cb(void *data, const Efl_Event *event)
{
   Elm_Label *o = data;
   Efl_Gfx_Entity *it = event->info;
   int col, row, colspan, rowspan;
   char buf[64];

   efl_pack_table_cell_column_get(event->object, it, &col, &colspan);
   efl_pack_table_cell_row_get(event->object, it, &row, &rowspan);
   if (event->desc == EFL_CONTAINER_EVENT_CONTENT_ADDED)
     sprintf(buf, "pack %d,%d %dx%d", col, row, colspan, rowspan);
   else
     sprintf(buf, "unpack %d,%d %dx%d", col, row, colspan, rowspan);
   elm_object_text_set(o, buf);
}

/**
 * @brief An example custom layout function for an Efl_Ui_Table.
 * @param pack The table object to be laid out.
 * @param _pd Not used.
 *
 * This function provides a simplistic custom layout. It divides the available
 * space into equally sized regions based on the number of columns and rows,
 * then centers each child object within its assigned cell(s) using its minimum
 * size hint.
 *
 * @note This is a demonstrative layout function and is intentionally simple.
 * It does not respect standard layout hints like alignment or weight, which
 * a production-ready layout function should.
 */
static void
_custom_layout_update(Eo *pack, void *_pd EINA_UNUSED)
{

   int rows, cols, c, r, cs, rs, gmw = 0, gmh = 0;
   Eina_Iterator *it;
   Eina_Rect g;
   Eo *item;

   g = efl_gfx_entity_geometry_get(pack);

   efl_pack_table_size_get(pack, &cols, &rows);
   if (!cols || !rows) goto end;

   it = efl_content_iterate(pack);
   EINA_ITERATOR_FOREACH(it, item)
     {
        if (efl_pack_table_cell_column_get(pack, item, &c, &cs) &&
            efl_pack_table_cell_row_get(pack, item, &r, &rs))
          {
             Eina_Rect m;

             m.x = g.x + c * g.w / cols + (cs * g.w / cols - g.x) / 2;
             m.y = g.y + r * g.h / rows + (rs * g.h / rows - g.y) / 2;
             m.size = efl_gfx_hint_size_combined_min_get(item);
             efl_gfx_entity_geometry_set(item, m);

             gmw = MAX(gmw, m.w);
             gmh = MAX(gmh, m.h);
          }
     }
   eina_iterator_free(it);

end:
   efl_gfx_hint_size_min_set(pack, EINA_SIZE2D(gmw * cols, gmh * rows));
}

/**
 * @brief Main test function for Efl.Ui.Table with cell-based packing.
 *
 * This function sets up a window containing an Efl_Ui_Table and a control
 * panel. The control panel includes various widgets (radio buttons, sliders)
 * to dynamically change the table's properties and the properties of its
 * children. This allows for testing features like packing, spanning, alignment,
 * weighting, margins, and padding.
 *
 * The test demonstrates:
 * - Packing objects into specific table cells with row/column spans.
 * - Dynamically changing layout properties via UI controls.
 * - Using event callbacks to monitor layout changes and content modifications.
 * - Overriding the default layout logic with a custom layout function.
 */
void
test_ui_table(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *o, *vbox, *f, *hbox, *chk, *table;
   int i = 0;

   win = elm_win_util_standard_add("ui-table", "Efl.Ui.Table");
   elm_win_autodel_set(win, EINA_TRUE);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(600,  400));

   vbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_arrangement_content_padding_set(vbox, 10, 10);
   efl_gfx_hint_weight_set(vbox, 1, 1);
   efl_gfx_hint_margin_set(vbox, 5, 5, 5, 5);
   elm_win_resize_object_add(win, vbox);


   // create here to pass in cb
   table = efl_add(EFL_UI_TABLE_CLASS, win);


   /* controls */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Controls");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 0);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   hbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
   elm_object_content_set(f, hbox);
   efl_gfx_arrangement_content_padding_set(hbox, 10, 0);


   /* weights radio group */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_pack(hbox, bx);

   chk = o = elm_radio_add(win);
   elm_object_text_set(o, "No weight");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, NONE);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "No weight + table fill");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, NONE_BUT_FILL);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "Equal weights");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, EQUAL);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "One weight only");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, ONE);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "Two weights");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, TWO);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "Custom layout");
   evas_object_smart_callback_add(o, "changed", weights_cb, table);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, CUSTOM);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   elm_radio_value_set(chk, EQUAL);


   /* min size setter */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 0, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "User min size");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.0fpx");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, user_min_slider_cb, NULL);
   elm_slider_min_max_set(o, 0, 250);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* inner box padding */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 0, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Padding");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.0fpx");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, padding_slider_cb, table);
   elm_slider_min_max_set(o, 0, 40);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* outer margin */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 0, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Margin");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.0fpx");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, margin_slider_cb, table);
   elm_slider_min_max_set(o, 0, 40);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* button margins */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Buttons margins");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.0fpx");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, btnmargins_slider_cb, table);
   elm_slider_min_max_set(o, 0, 40);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* button1 aligns */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Button1 align");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignv_slider_cb, NULL);
   elm_slider_min_max_set(o, 0, 1);
   elm_slider_value_set(o, 0.3);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 1);
   efl_gfx_hint_align_set(o, -1, -1);
   efl_gfx_hint_weight_set(o, 1, 0);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignh_slider_cb, NULL);
   elm_slider_min_max_set(o, 0, 1);
   elm_slider_value_set(o, 0.3);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   /* ro info */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "<b>Properties</>");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_label_add(win);
   efl_event_callback_add(table, EFL_PACK_EVENT_LAYOUT_UPDATED, layout_updated_cb, o);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_label_add(win);
   efl_event_callback_add(table, EFL_CONTAINER_EVENT_CONTENT_ADDED, child_evt_cb, o);
   efl_event_callback_add(table, EFL_CONTAINER_EVENT_CONTENT_REMOVED, child_evt_cb, o);
   efl_gfx_hint_align_set(o, 0.5, 0);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* contents */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Contents");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 1);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   efl_gfx_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_content_set(f, table);
   efl_gfx_entity_visible_set(table, 1);

   objects[i++] = o = efl_add(EFL_CANVAS_RECTANGLE_CLASS, win);
   efl_gfx_hint_size_min_set(o, EINA_SIZE2D(10, 10));
   efl_gfx_color_set(o, 64, 96, 128, 255);
   efl_pack_table(table, o, 0, 0, 3, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 1");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_gfx_hint_align_set(o, 0.3, 0.3);
   efl_gfx_hint_size_max_set(o, EINA_SIZE2D(100, 100));
   efl_pack_table(table, o, 0, 0, 1, 1);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 2");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack_table(table, o, 1, 0, 1, 1);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 3");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack_table(table, o, 2, 0, 1, 1);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 4");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack_table(table, o, 0, 1, 2, 1);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 5");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack_table(table, o, 2, 1, 1, 2);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, "Button 6");
   efl_gfx_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   efl_gfx_hint_fill_set(o, EINA_TRUE, EINA_TRUE);
   efl_pack_table(table, o, 0, 2, 2, 1);
   efl_gfx_entity_visible_set(o, 1);

   efl_gfx_entity_visible_set(win, 1);
}

/**
 * @brief Generates a unique, static string for button labels.
 * @param str A base string, or NULL to use a default "item" string.
 * @return A statically allocated string in the format "base_string id".
 *
 * This helper function is used to create distinct labels for dynamically
 * added buttons. It is not thread-safe.
 *
 * @warning The returned buffer is overwritten on each call.
 */
static const char *
btn_text(const char *str)
{
   static char buf[64];
   static int id = 0;
   sprintf(buf, "%s %d", str ?: "item", ++id);
   return buf;
}

/**
 * @brief Callback to remove an object when it is clicked.
 * @param data Not used.
 * @param ev The click event. The object to be deleted is `ev->object`.
 */
static void
remove_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   efl_del(ev->object);
}

/**
 * @brief Callback for the "Append" button.
 * @param data The Efl_Ui_Table to which a new item will be appended.
 * @param ev Not used.
 *
 * Creates and appends a new button to the table using the linear packing API
 * (`efl_pack`).
 */
static void
append_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *table = data;
   Eo *o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, btn_text("appended"));
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, remove_cb, NULL);
   elm_object_tooltip_text_set(o, "Click to unpack");
   efl_pack(table, o);
   efl_gfx_entity_visible_set(o, 1);
}

/**
 * @brief Callback for the "Clear" button.
 * @param data The Efl_Ui_Table to be cleared.
 * @param ev Not used.
 *
 * Removes all children from the table.
 */
static void
clear_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *table = data;
   efl_pack_clear(table);
}

/**
 * @brief Test function for Efl.Ui.Table's linear packing APIs.
 *
 * This function sets up a window to test the "linear" or "flow" packing
 * behavior of Efl_Ui_Table, where items are added sequentially using
 * `efl_pack` rather than being placed in specific cells.
 *
 * The test demonstrates:
 * - Setting a fixed number of columns or rows to control flow.
 * - Appending and clearing items from the table.
 * - Monitoring table properties and events in a linear packing context.
 */
void
test_ui_table_linear(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Evas_Object *win, *o, *vbox, *f, *hbox, *table, *ico, *bx;

   win = elm_win_util_standard_add("ui-table-linear", "Efl.Ui.Table Linear APIs");
   elm_win_autodel_set(win, EINA_TRUE);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(600,  400));

   vbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_arrangement_content_padding_set(vbox, 10, 10);
   efl_gfx_hint_margin_set(vbox, 5, 5, 5, 5);
   elm_win_resize_object_add(win, vbox);
   efl_gfx_entity_visible_set(vbox, 1);


   // create here to pass in cb
   table = efl_add(EFL_UI_TABLE_CLASS, win);


   /* controls */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Controls");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 0);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   hbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
   elm_object_content_set(f, hbox);
   efl_gfx_arrangement_content_padding_set(hbox, 5, 0);
   efl_gfx_entity_visible_set(hbox, 1);

   ico = elm_icon_add(win);
   elm_icon_standard_set(ico, "list-add");
   o = elm_button_add(win);
   elm_object_content_set(o, ico);
   elm_object_text_set(o, "Append");
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, append_cb, table);
   efl_pack(hbox, o);
   efl_gfx_entity_visible_set(o, 1);

   ico = elm_icon_add(win);
   elm_icon_standard_set(ico, "edit-clear-all");
   o = elm_button_add(win);
   elm_object_content_set(o, ico);
   elm_object_text_set(o, "Clear");
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, clear_cb, table);
   efl_pack(hbox, o);
   efl_gfx_entity_visible_set(o, 1);


   /* ro info */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);
   efl_gfx_entity_visible_set(bx, 1);

   o = elm_label_add(win);
   elm_object_text_set(o, "<b>Properties</>");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_label_add(win);
   efl_event_callback_add(table, EFL_PACK_EVENT_LAYOUT_UPDATED, layout_updated_cb, o);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_label_add(win);
   efl_event_callback_add(table, EFL_CONTAINER_EVENT_CONTENT_ADDED, child_evt_cb, o);
   efl_event_callback_add(table, EFL_CONTAINER_EVENT_CONTENT_REMOVED, child_evt_cb, o);
   efl_gfx_hint_align_set(o, 0.5, 0);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* contents */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Contents");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 1);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   efl_pack_table_columns_set(table, 4);
   efl_ui_layout_orientation_set(table, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL);
   efl_gfx_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_content_set(f, table);
   efl_gfx_entity_visible_set(table, 1);

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, btn_text(NULL));
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, remove_cb, NULL);
   efl_pack(table, o);
   efl_gfx_entity_visible_set(o, 1);

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, btn_text(NULL));
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, remove_cb, NULL);
   efl_pack(table, o);
   efl_gfx_entity_visible_set(o, 1);

   o = efl_add(EFL_UI_BUTTON_CLASS, table);
   efl_text_set(o, btn_text(NULL));
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_event_callback_add(o, EFL_INPUT_EVENT_CLICKED, remove_cb, NULL);
   efl_pack(table, o);
   efl_gfx_entity_visible_set(o, 1);

   efl_gfx_entity_visible_set(win, 1);
}
