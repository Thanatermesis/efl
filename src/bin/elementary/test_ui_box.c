#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_PACK_LAYOUT_PROTECTED
#include <Efl_Ui.h>
#include <Elementary.h>
#include <assert.h>

#define CNT 10
static Evas_Object *objects[CNT] = {};

typedef enum {
   NONE,
   NONE_BUT_FILL,
   EQUAL,
   ONE,
   TWO
} Weight_Mode;

/**
 * @brief Callback function to adjust the weight hints of child objects.
 *
 * This function is triggered by a radio button group. Based on the selected
 * option, it changes the weight hints of the objects within the main box,
 * demonstrating different weight configurations. The `objects` array contains
 * all the children of the box.
 *
 * @param data The container box whose alignment is modified.
 * @param obj The radio button that triggered the event.
 * @param event_info Unused event information.
 */
static void
weights_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Weight_Mode mode = elm_radio_state_value_get(obj);

   switch (mode)
     {
      case NONE:
        efl_gfx_hint_align_set(data, 0.5, 0.5);
        for (int i = 0; i < CNT; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
      case NONE_BUT_FILL:
        efl_gfx_hint_align_set(data, -1, -1);
        for (int i = 0; i < CNT; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
      case EQUAL:
        efl_gfx_hint_align_set(data, 0.5, 0.5);
        for (int i = 0; i < CNT; i++)
          efl_gfx_hint_weight_set(objects[i], 1, 1);
        break;
      case ONE:
        efl_gfx_hint_align_set(data, 0.5, 0.5);
        for (int i = 0; i < 6; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        efl_gfx_hint_weight_set(objects[6], 1, 1);
        for (int i = 7; i < CNT; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
      case TWO:
        efl_gfx_hint_align_set(data, 0.5, 0.5);
        for (int i = 0; i < 5; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        efl_gfx_hint_weight_set(objects[5], 1, 1);
        efl_gfx_hint_weight_set(objects[6], 1, 1);
        for (int i = 7; i < CNT; i++)
          efl_gfx_hint_weight_set(objects[i], 0, 0);
        break;
     }
}

/**
 * @brief Callback to set the minimum size of a specific UI object.
 *
 * Triggered by a slider, this function adjusts the minimum width and height
 * of the fourth object (`objects[3]`) in the `objects` array.
 *
 * @param data Unused.
 * @param event The event structure containing the slider object.
 */
static void
user_min_slider_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);

   efl_gfx_hint_size_min_set(objects[3], EINA_SIZE2D(val, val));
}

/**
 * @brief Callback to adjust the padding between content of the main box.
 *
 * This function is connected to a slider and dynamically changes the horizontal
 * and vertical padding of the box container.
 *
 * @param data The window containing the box.
 * @param event The event structure containing the slider object.
 */
static void
padding_slider_cb(void *data, const Efl_Event *event)
{
   unsigned int val = lround(elm_slider_value_get(event->object));
   Eo *win = data, *box;

   box = efl_key_wref_get(win, "box");
   efl_gfx_arrangement_content_padding_set(box, val, val);
}

/**
 * @brief Callback to adjust the margin of the main box.
 *
 * This function is connected to a slider and dynamically changes the
 * margin on all sides of the box container.
 *
 * @param data The window containing the box.
 * @param event The event structure containing the slider object.
 */
static void
margin_slider_cb(void *data, const Efl_Event *event)
{
   int val = elm_slider_value_get(event->object);
   Eo *win = data, *box;

   box = efl_key_wref_get(win, "box");
   efl_gfx_hint_margin_set(box, val, val, val, val);
}

/**
 * @brief Callback to adjust the horizontal alignment of content within the main box.
 *
 * This function is connected to a slider and dynamically changes the
 * horizontal alignment of all content within the box.
 *
 * @param data The window containing the box.
 * @param event The event structure containing the slider object.
 */
static void
alignh_slider_cb(void *data, const Efl_Event *event)
{
   double av, val;
   Eo *win = data, *box;

   box = efl_key_wref_get(win, "box");
   val = elm_slider_value_get(event->object);
   efl_gfx_arrangement_content_align_get(box, NULL, &av);
   efl_gfx_arrangement_content_align_set(box, val, av);
}

/**
 * @brief Callback to adjust the vertical alignment of content within the main box.
 *
 * This function is connected to a slider and dynamically changes the
 * vertical alignment of all content within the box.
 *
 * @param data The window containing the box.
 * @param event The event structure containing the slider object.
 */
static void
alignv_slider_cb(void *data, const Efl_Event *event)
{
   double ah, val;
   Eo *win = data, *box;

   box = efl_key_wref_get(win, "box");
   val = elm_slider_value_get(event->object);
   efl_gfx_arrangement_content_align_get(box, &ah, NULL);
   efl_gfx_arrangement_content_align_set(box, ah, val);
}

/**
 * @brief Callback to adjust the horizontal alignment hint of a specific button.
 *
 * This function is connected to a slider and dynamically changes the
 * horizontal alignment hint for a specific button within the box.
 *
 * @param data The window containing the button.
 * @param event The event structure containing the slider object.
 */
static void
alignh_btn_slider_cb(void *data, const Efl_Event *event)
{
   double av, val;
   Eo *win = data, *btn;

   btn = efl_key_wref_get(win, "button");
   val = elm_slider_value_get(event->object);
   efl_gfx_hint_align_get(btn, NULL, &av);
   efl_gfx_hint_align_set(btn, val, av);
}

/**
 * @brief Callback to adjust the vertical alignment hint of a specific button.
 *
 * This function is connected to a slider and dynamically changes the
 * vertical alignment hint for a specific button within the box.
 *
 * @param data The window containing the button.
 * @param event The event structure containing the slider object.
 */
static void
alignv_btn_slider_cb(void *data, const Efl_Event *event)
{
   double ah, val;
   Eo *win = data, *btn;

   btn = efl_key_wref_get(win, "button");
   val = elm_slider_value_get(event->object);
   efl_gfx_hint_align_get(btn, &ah, NULL);
   efl_gfx_hint_align_set(btn, ah, val);
}

/**
 * @brief Callback to toggle between a standard box and a flow box layout.
 *
 * When the checkbox state changes, this function unpacks all children from
 * the current box, deletes the box, and creates a new one (either
 * EFL_UI_BOX_CLASS or EFL_UI_BOX_FLOW_CLASS). It then repacks the children
 * into the new box. This demonstrates how to dynamically change layout types.
 *
 * @param data The window containing the box.
 * @param obj The checkbox that triggered the event.
 * @param event_info Unused event information.
 */
static void
flow_check_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eina_Bool chk = elm_check_selected_get(obj);
   Eina_List *list = NULL;
   Eina_Iterator *it;
   Eo *box, *win, *sobj, *parent;

   // Unpack all children from the box, delete it and repack into the new box

   win = data;
   box = efl_key_wref_get(win, "box");
   parent = efl_parent_get(box);
   it = efl_content_iterate(box);
   EINA_ITERATOR_FOREACH(it, sobj)
     list = eina_list_append(list, sobj);
   eina_iterator_free(it);
   efl_pack_unpack_all(box);
   efl_del(box);

   box = efl_add(chk ? EFL_UI_BOX_FLOW_CLASS : EFL_UI_BOX_CLASS, win,
                 efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
   efl_content_set(parent, box);
   efl_key_wref_set(win, "box", box);

   EINA_LIST_FREE(list, sobj)
     efl_pack(box, sobj);
}

/**
 * @brief Callback to toggle the orientation of the main box.
 *
 * This function is connected to a checkbox and switches the box layout
 * orientation between horizontal and vertical.
 *
 * @param data The window containing the box.
 * @param obj The checkbox that triggered the event.
 * @param event_info Unused event information.
 */
static void
horiz_check_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eina_Bool chk = elm_check_selected_get(obj);
   Eo *box = efl_key_wref_get(data, "box");
   efl_ui_layout_orientation_set(box, chk ? EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL : EFL_UI_LAYOUT_ORIENTATION_VERTICAL);
}

/**
 * @brief Callback to toggle homogeneous mode for the main box.
 *
 * This function is connected to a checkbox and enables or disables
 * homogeneous mode, where all children are allocated equal space.
 *
 * @param data The window containing the box.
 * @param obj The checkbox that triggered the event.
 * @param event_info Unused event information.
 */
static void
homo_check_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eina_Bool chk = elm_check_selected_get(obj);
   Eo *box = efl_key_wref_get(data, "box");
   efl_ui_box_homogeneous_set(box, chk);
}

/**
 * @brief A custom layout update function for a box.
 *
 * This function demonstrates how to provide a custom layout logic for a box
 * object. It arranges children in a diagonal line from top-left to
 * bottom-right. This is not intended as a practical layout but as an
 * example of the API.
 *
 * @param pack The box object to be laid out.
 * @param data Unused user data.
 */
static void
_custom_layout_update(Eo *pack, const void *data EINA_UNUSED)
{
   Eina_Iterator *it = efl_content_iterate(pack);
   int count = efl_content_count(pack), i = 0;
   Eina_Rect rp;
   Eo *sobj;

   // Note: This is a TERRIBLE layout. Just an example of the API, not showing
   // how to write a proper layout function.

   if (!count) return;

   rp = efl_gfx_entity_geometry_get(pack);
   EINA_ITERATOR_FOREACH(it, sobj)
     {
        Eina_Rect r;

        r.x = (rp.w / count) * i;
        r.y = (rp.h / count) * i;
        r.size = efl_gfx_hint_size_combined_min_get(sobj);
        efl_gfx_entity_geometry_set(sobj, r);
        i++;
     }
   eina_iterator_free(it);
}

/**
 * @brief Callback to enable or disable a custom layout function on the box.
 *
 * When the checkbox is toggled, this function uses `efl_object_override`
 * to either set a custom layout function (`_custom_layout_update`) on the
 * box or revert to the default layout function by passing NULL.
 * A layout update is then requested to apply the change.
 *
 * @param data The window containing the box.
 * @param obj The checkbox that triggered the event.
 * @param event_info Unused event information.
 */
static void
custom_check_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   EFL_OPS_DEFINE(custom_layout_ops,
                  EFL_OBJECT_OP_FUNC(efl_pack_layout_update, _custom_layout_update));

   Eina_Bool chk = elm_check_selected_get(obj);
   Eo *box, *win = data;

   box = efl_key_wref_get(win, "box");

   // Overriding just the one function we need
   efl_object_override(box, chk ? &custom_layout_ops : NULL);

   // Layout request is required as the pack object doesn't know the layout
   // function was just overridden.
   efl_pack_layout_request(box);
}

/**
 * @brief Main function to set up and run the Efl.Ui.Box test.
 *
 * This function creates the main window and populates it with various controls
 * (sliders, checkboxes, radio buttons) to manipulate the properties of an
 * Efl.Ui.Box widget. It also creates the box and a set of child objects
 * within it to demonstrate the effects of different properties like padding,
 * margin, alignment, weight, and layout modes.
 *
 * The `objects` array holds static references to child elements of the box
 * for individual manipulation. Its structure is:
 * - `objects[0]`: Button "Btn1"
 * - `objects[1]`: Button "Button 2"
 * - `objects[2]`: Label "This label is not marked as fill"
 * - `objects[3]`: Button "Min size"
 * - `objects[4]`: Button "Quit!"
 * - `objects[5]`: Label "This label on the other hand..."
 * - `objects[6]`: Button "Button with a quite long text."
 * - `objects[7]`: Button "BtnA"
 * - `objects[8]`: Button "BtnB"
 * - `objects[9]`: Button "BtnC"
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_ui_box(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *o, *vbox, *f, *hbox, *chk;
   int i = 0;

   win = elm_win_util_standard_add("ui-box", "Efl.Ui.Box");
   elm_win_autodel_set(win, EINA_TRUE);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(600,  400));

   vbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_arrangement_content_padding_set(vbox, 10, 10);
   efl_gfx_hint_margin_set(vbox, 5, 5, 5, 5);
   elm_win_resize_object_add(win, vbox);


   /* controls */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Controls");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 0);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   hbox = efl_add(EFL_UI_BOX_CLASS, win,
                  efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
   efl_content_set(f, hbox);
   efl_gfx_arrangement_content_padding_set(hbox, 10, 0);


   /* weights radio group */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_pack(hbox, bx);

   chk = o = elm_radio_add(win);
   elm_object_text_set(o, "No weight");
   evas_object_smart_callback_add(o, "changed", weights_cb, win);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, NONE);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "No weight + box fill");
   evas_object_smart_callback_add(o, "changed", weights_cb, win);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, NONE_BUT_FILL);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "Equal weights");
   evas_object_smart_callback_add(o, "changed", weights_cb, win);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, EQUAL);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "One weight only");
   evas_object_smart_callback_add(o, "changed", weights_cb, win);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, ONE);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_radio_add(win);
   elm_object_text_set(o, "Two weights");
   evas_object_smart_callback_add(o, "changed", weights_cb, win);
   efl_gfx_hint_align_set(o, 0, 0.5);
   elm_radio_state_value_set(o, TWO);
   elm_radio_group_add(o, chk);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   elm_radio_value_set(chk, NONE);


   /* misc */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 0, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Misc");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_check_add(win);
   elm_check_selected_set(o, 0);
   elm_object_text_set(o, "Flow");
   evas_object_smart_callback_add(o, "changed", flow_check_cb, win);
   efl_gfx_hint_align_set(o, 0, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_check_add(win);
   elm_check_selected_set(o, 1);
   elm_object_text_set(o, "Horizontal");
   evas_object_smart_callback_add(o, "changed", horiz_check_cb, win);
   efl_gfx_hint_align_set(o, 0, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_check_add(win);
   elm_check_selected_set(o, 0);
   elm_object_text_set(o, "Homogenous");
   evas_object_smart_callback_add(o, "changed", homo_check_cb, win);
   efl_gfx_hint_align_set(o, 0, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_check_add(win);
   elm_check_selected_set(o, 0);
   elm_object_text_set(o, "Custom layout");
   evas_object_smart_callback_add(o, "changed", custom_check_cb, win);
   efl_gfx_hint_align_set(o, 0, 0);
   efl_gfx_hint_weight_set(o, 0, 1);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* user min size setter */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
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
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, padding_slider_cb, win);
   elm_slider_min_max_set(o, 0, 40);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 10);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* outer margin */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
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
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, margin_slider_cb, win);
   elm_slider_min_max_set(o, 0, 40);
   elm_slider_inverted_set(o, 1);
   elm_slider_value_set(o, 10);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* Box align */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Box align");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignv_slider_cb, win);
   elm_slider_min_max_set(o, -0.1, 1.0);
   elm_slider_step_set(o, 0.1);
   elm_slider_value_set(o, 0.5);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 1);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 0);
   efl_gfx_hint_size_min_set(o, EINA_SIZE2D(100, 0));
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignh_slider_cb, win);
   elm_slider_min_max_set(o, -0.1, 1.0);
   elm_slider_step_set(o, 0.1);
   elm_slider_value_set(o, 0.5);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   /* Button align */
   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_VERTICAL));
   efl_gfx_hint_align_set(bx, 0, 0.5);
   efl_gfx_hint_fill_set(bx, EINA_FALSE, EINA_TRUE);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_pack(hbox, bx);

   o = elm_label_add(win);
   elm_object_text_set(o, "Button align");
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 0);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 1);
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignv_btn_slider_cb, win);
   elm_slider_min_max_set(o, 0.0, 1.0);
   elm_slider_step_set(o, 0.1);
   elm_slider_value_set(o, 0.5);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   o = elm_slider_add(win);
   elm_slider_indicator_format_set(o, "%.1f");
   elm_slider_indicator_show_set(o, 1);
   elm_slider_horizontal_set(o, 1);
   efl_gfx_hint_align_set(o, 0.5, -1);
   efl_gfx_hint_weight_set(o, 1, 0);
   efl_gfx_hint_size_min_set(o, EINA_SIZE2D(100, 0));
   efl_event_callback_add(o, EFL_UI_RANGE_EVENT_CHANGED, alignh_btn_slider_cb, win);
   elm_slider_min_max_set(o, -0.1, 1.0);
   elm_slider_step_set(o, 0.1);
   elm_slider_value_set(o, 0.5);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);


   /* contents */
   f = elm_frame_add(win);
   elm_object_text_set(f, "Contents");
   efl_gfx_hint_align_set(f, -1, -1);
   efl_gfx_hint_weight_set(f, 1, 1);
   efl_pack(vbox, f);
   efl_gfx_entity_visible_set(f, 1);

   bx = efl_add(EFL_UI_BOX_CLASS, win,
                efl_ui_layout_orientation_set(efl_added, EFL_UI_LAYOUT_ORIENTATION_HORIZONTAL));
   efl_key_wref_set(win, "box", bx);
   efl_gfx_arrangement_content_padding_set(bx, 10, 10);
   efl_gfx_hint_align_set(bx, 0.5, 0.5);
   efl_gfx_hint_weight_set(bx, 1, 1);
   efl_content_set(f, bx);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "Btn1");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_align_set(o, 0.5, 0.5);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "Button 2");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = elm_label_add(win);
   elm_label_line_wrap_set(o, ELM_WRAP_WORD);
   elm_object_text_set(o, "This label is not marked as fill");
   efl_gfx_hint_align_set(o, 0.5, 0.5);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "Min size");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_gfx_hint_align_set(o, 0.5, 1.0);
   efl_gfx_hint_aspect_set(o, EFL_GFX_HINT_ASPECT_BOTH, EINA_SIZE2D(1, 1));
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "Quit!");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_gfx_hint_align_set(o, 0.5, 0.0);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = elm_label_add(win);
   elm_label_line_wrap_set(o, ELM_WRAP_WORD);
   elm_object_text_set(o, "This label on the other hand<br/>is marked as align=fill.");
   efl_gfx_hint_align_set(o, -1, -1);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_key_wref_set(win, "button", o);
   efl_text_set(o, "Button with a quite long text.");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_size_max_set(o, EINA_SIZE2D(200, 100));
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "BtnA");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_gfx_hint_aspect_set(o, EFL_GFX_HINT_ASPECT_BOTH, EINA_SIZE2D(1, 2));
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "BtnB");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   objects[i++] = o = efl_add(EFL_UI_BUTTON_CLASS, bx);
   efl_text_set(o, "BtnC");
   efl_gfx_hint_weight_set(o, 0, 0);
   efl_gfx_hint_fill_set(o, EINA_FALSE, EINA_FALSE);
   efl_pack(bx, o);
   efl_gfx_entity_visible_set(o, 1);

   assert(i == CNT);

   efl_gfx_entity_visible_set(win, 1);
}
