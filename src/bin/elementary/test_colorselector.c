#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback for the "changed" event of the color selector.
 *
 * This function is called whenever the color in the color selector is
 * changed by the user. It retrieves the new color, prints it to stdout,
 * and applies it to a rectangle object provided in the @p data parameter.
 * The color is premultiplied before being set to the rectangle.
 *
 * @param data The Evas_Object (a rectangle) to apply the color to.
 * @param obj The color selector widget that triggered the event.
 * @param event_info Unused event information.
 */
static void
_colorselector_changed_cb(void *data, Evas_Object *obj,
                          void *event_info EINA_UNUSED)
{
   Evas_Object *re = data;
   int r, g, b, a;

   elm_colorselector_color_get(obj, &r, &g, &b, &a);
   printf("Changed Color [r=%d g=%d b=%d a=%d]\n", r, g, b, a);
   evas_color_argb_premul(a, &r, &g, &b);
   evas_object_color_set(re, r, g, b, a);
}

/**
 * @brief Callback for the "color,item,selected" event of the color selector.
 *
 * This function is called when a color item from the palette is selected.
 * It retrieves the color of the selected item and prints it to stdout.
 *
 * @param data Unused user data.
 * @param obj Unused object that triggered the event.
 * @param event_info The selected Elm_Object_Item.
 */
static void
_color_item_selected_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                        void *event_info)
{
   int r = 0, g = 0, b = 0 ,a = 0;
   Elm_Object_Item *color_it = event_info;

   elm_colorselector_palette_item_color_get(color_it, &r, &g, &b, &a);
   printf("Selected Color Palette [r=%d g=%d b=%d a=%d]\n", r, g, b, a);
}

/**
 * @brief Callback for the "color,item,longpressed" event of the color selector.
 *
 * This function is called when a color item from the palette is long-pressed.
 * It retrieves the color of the long-pressed item and prints its details to stdout.
 *
 * @param data Unused user data.
 * @param obj Unused object that triggered the event.
 * @param event_info The long-pressed Elm_Object_Item.
 */
static void
_color_item_longpressed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                           void *event_info)
{
   int r = 0,g = 0,b = 0 ,a = 0;
   Elm_Object_Item *color_it = event_info;

   elm_colorselector_palette_item_color_get(color_it, &r, &g, &b, &a);
   printf("Longpressed color item : %p, color = %d-%d-%d-%d\n",
          color_it, r, g, b, a);
}

/**
 * @brief Callback to set the color selector mode to PALETTE.
 * @param data The color selector widget.
 * @param obj Unused button object.
 * @param event_info Unused event info.
 */
static void
_palette_cb(void *data, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *cs = data;
   elm_colorselector_mode_set(cs, ELM_COLORSELECTOR_PALETTE);
}

/**
 * @brief Callback to set the color selector mode to COMPONENTS.
 * @param data The color selector widget.
 * @param obj Unused button object.
 * @param event_info Unused event info.
 */
static void
_components_cb(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   Evas_Object *cs = data;
   elm_colorselector_mode_set(cs, ELM_COLORSELECTOR_COMPONENTS);
}

/**
 * @brief Callback to set the color selector mode to BOTH.
 * @param data The color selector widget.
 * @param obj Unused button object.
 * @param event_info Unused event info.
 */
static void
_both_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *cs = data;
   elm_colorselector_mode_set(cs, ELM_COLORSELECTOR_BOTH);
}

/**
 * @brief Callback to set the color selector mode to PICKER.
 * @param data The color selector widget.
 * @param obj Unused button object.
 * @param event_info Unused event info.
 */
static void
_picker_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *cs = data;
   elm_colorselector_mode_set(cs, ELM_COLORSELECTOR_PICKER);
}

/**
 * @brief Callback to set the color selector mode to ALL.
 * @param data The color selector widget.
 * @param obj Unused button object.
 * @param event_info Unused event info.
 */
static void
_all_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *cs = data;
   elm_colorselector_mode_set(cs, ELM_COLORSELECTOR_ALL);
}

/**
 * @brief Main function to set up and run the color selector test.
 *
 * This function creates a window, a color selector widget, a rectangle
 * to display the selected color, and several buttons to change the
 * color selector's mode. It demonstrates how to create a color selector,
 * populate its palette, set an initial color, and connect callbacks for
 * various events.
 *
 * @param data Unused user data.
 * @param obj Unused object.
 * @param event_info Unused event info.
 */
void
test_colorselector(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx2, *cs, *fr, *re, *bt;
   const Eina_List *item_list, *last_item_list;
   Elm_Object_Item *color_item;
   int r, g, b, a;
   Elm_Object_Item *item;

   win = elm_win_util_standard_add("colorselector", "ColorSelector");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   fr = elm_frame_add(bx);
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(fr, "Color View");
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   re = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_min_set(re, 1, ELM_SCALE_SIZE(100));
   evas_object_show(re);
   elm_object_content_set(fr, re);

   fr = elm_frame_add(bx);
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(fr, "Color Selector");
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   cs = elm_colorselector_add(fr);
   item = elm_colorselector_palette_color_add(cs, 0, 0, 0, 255);
   elm_colorselector_palette_item_selected_set(item, EINA_TRUE);

   elm_colorselector_palette_color_add(cs, 255, 255, 255, 255);
   elm_colorselector_palette_color_add(cs, 255, 0, 0, 255);
   elm_colorselector_palette_color_add(cs, 0, 255, 0, 255);
   elm_colorselector_palette_color_add(cs, 0, 0, 255, 255);
   elm_colorselector_palette_color_add(cs, 255, 255, 0, 255);
   elm_colorselector_palette_color_add(cs, 0, 255, 255, 255);
   elm_colorselector_palette_color_add(cs, 255, 0, 255, 255);
   elm_colorselector_palette_color_add(cs, 192, 192, 192, 255);
   elm_colorselector_palette_color_add(cs, 128, 128, 128, 255);
   elm_colorselector_palette_color_add(cs, 128, 0, 0, 255);
   elm_colorselector_palette_color_add(cs, 128, 128, 0, 255);
   elm_colorselector_palette_color_add(cs, 0, 128, 0, 255);
   elm_colorselector_palette_color_add(cs, 128, 0, 128, 255);
   elm_colorselector_palette_color_add(cs, 0, 128, 128, 255);
   elm_colorselector_palette_color_add(cs, 0, 100, 128, 255);

   item_list = elm_colorselector_palette_items_get(cs);
   last_item_list = eina_list_last(item_list);
   color_item = eina_list_data_get(last_item_list);
   elm_colorselector_palette_item_color_set(color_item, 0, 0, 128, 255);

   a = 180;
   r = 255;
   g = 160;
   b = 132;

   elm_colorselector_color_set(cs, r, g, b, a);
   evas_object_size_hint_weight_set(cs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(cs, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(fr, cs);
   evas_object_show(cs);
   evas_object_smart_callback_add(cs, "changed", _colorselector_changed_cb, re);
   evas_object_smart_callback_add(cs, "color,item,selected",
                                  _color_item_selected_cb, re);
   evas_object_smart_callback_add(cs, "color,item,longpressed",
                                  _color_item_longpressed_cb, re);

   elm_colorselector_color_get(cs, &r, &g, &b, &a);
   /* Fix Alpha pre multiplication by edje */
   r = (r * a) / 255;
   g = (g * a) / 255;
   b = (b * a) / 255;
   evas_object_color_set(re, r, g, b, a);

   bx2 = elm_box_add(bx);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bx2);
   elm_box_pack_end(bx, bx2);

   bt = elm_button_add(bx2);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _palette_cb, cs);
   elm_object_text_set(bt, "Palette");
   evas_object_show(bt);
   elm_box_pack_end(bx2, bt);

   bt = elm_button_add(bx2);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _components_cb, cs);
   elm_object_text_set(bt, "Components");
   evas_object_show(bt);
   elm_box_pack_end(bx2, bt);

   bt = elm_button_add(bx2);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _both_cb, cs);
   elm_object_text_set(bt, "Both");
   evas_object_show(bt);
   elm_box_pack_end(bx2, bt);

   bt = elm_button_add(bx2);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _picker_cb, cs);
   elm_object_text_set(bt, "Picker");
   evas_object_show(bt);
   elm_box_pack_end(bx2, bt);

   bt = elm_button_add(bx2);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _all_cb, cs);
   elm_object_text_set(bt, "All");
   evas_object_show(bt);
   elm_box_pack_end(bx2, bt);

   evas_object_show(win);
}
