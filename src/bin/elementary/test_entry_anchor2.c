#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function to close the anchor hover pop-up.
 *
 * This function is called when a button within the anchor's hover pop-up
 * is clicked. It programmatically ends the hover state, effectively
 * closing the pop-up.
 *
 * @param data The entry widget (Evas_Object *) that contains the anchor.
 * @param obj The Evas_Object that triggered the callback (unused).
 * @param event_info The event-specific information (unused).
 */
static void
my_entry_anchor_bt(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *av = data;
   elm_entry_anchor_hover_end(av);
}

/**
 * @brief Callback function for an anchor click event.
 *
 * This function is invoked when an anchor within the entry widget is clicked.
 * It prints details about the click event, such as the mouse button used,
 * the anchor's name, and the coordinates of the click.
 *
 * @param data Custom data passed to the callback (unused).
 * @param obj The Evas_Object that triggered the callback (unused).
 * @param event_info A pointer to an Elm_Entry_Anchor_Info struct containing
 *        details about the anchor click event. For example:
 *        - ev->button: The mouse button that was clicked (e.g., 1 for left-click).
 *        - ev->name: The string from the 'href' attribute of the anchor tag.
 *        - ev->x, ev->y: The coordinates of the click.
 */
static void
_anchor_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Entry_Anchor_Info *ev = event_info;
   printf("anchor click %d: '%s' (%d, %d)\n", ev->button, ev->name, ev->x, ev->y);
}

/**
 * @brief Callback function for an anchor hover opened event.
 *
 * This function is called when the user hovers over an anchor in the entry
 * widget and the hover pop-up is created. It's responsible for populating
 * the content of this pop-up. The pop-up has several content areas
 * ("middle", "top", "bottom", "left", "right") that can be filled.
 *
 * The function checks which areas are available for content (e.g., `ei->hover_top`)
 * and adds widgets (buttons, in this case) to them.
 *
 * @param data The entry widget (Evas_Object *) that contains the anchor.
 * @param obj The entry widget object.
 * @param event_info A pointer to an Elm_Entry_Anchor_Hover_Info struct which
 *        contains information about the hover, including:
 *        - ei->anchor_info: Details about the anchor being hovered.
 *        - ei->hover: The hover object itself, to which content is added.
 *        - ei->hover_top, ei->hover_bottom, ei->hover_left, ei->hover_right: Booleans
 *          indicating if the respective content areas are available.
 */
static void
_anchor_hover_opened_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *av = data;
   Elm_Entry_Anchor_Hover_Info *ei = event_info;
   Evas_Object *bt, *bx;

   bt = elm_button_add(obj);
   elm_object_text_set(bt, ei->anchor_info->name);
   elm_object_part_content_set(ei->hover, "middle", bt);
   evas_object_show(bt);

   // hints as to where we probably should put hover contents (buttons etc.).
   if (ei->hover_top)
     {
        bx = elm_box_add(obj);
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Top 1");
        elm_box_pack_end(bx, bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Top 2");
        elm_box_pack_end(bx, bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Top 3");
        elm_box_pack_end(bx, bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
        elm_object_part_content_set(ei->hover, "top", bx);
        evas_object_show(bx);
     }
   if (ei->hover_bottom)
     {
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Bot");
        elm_object_part_content_set(ei->hover, "bottom", bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
     }
   if (ei->hover_left)
     {
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Left");
        elm_object_part_content_set(ei->hover, "left", bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
     }
   if (ei->hover_right)
     {
        bt = elm_button_add(obj);
        elm_object_text_set(bt, "Right");
        elm_object_part_content_set(ei->hover, "right", bt);
        evas_object_smart_callback_add(bt, "clicked", my_entry_anchor_bt, av);
        evas_object_show(bt);
     }
}

/**
 * @brief Main function for the Entry Anchor 2 test.
 *
 * This test demonstrates the functionality of anchors within an Elm_Entry widget,
 * specifically focusing on the hover pop-up feature. It creates a window with
 * an entry widget containing text formatted with HTML-like markup, including
 * anchor tags (`<a href=... >`).
 *
 * It sets up callbacks to handle:
 * 1. Clicks on anchors (`"anchor,clicked"` event).
 * 2. The creation of a hover pop-up when the mouse is over an anchor
 *    (`"anchor,hover,opened"` event).
 *
 * The hover style is set to "popout", which creates a separate window for the
 * hover content.
 *
 * @param data Custom data passed to the callback (unused).
 * @param obj The Evas_Object that triggered the callback (unused).
 * @param event_info The event-specific information (unused).
 */
void
test_entry_anchor2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *en;

   win = elm_win_util_standard_add("entry_anchor2", "Entry Anchor 2");
   elm_win_autodel_set(win, EINA_TRUE);

   en = elm_entry_add(win);
   elm_entry_anchor_hover_style_set(en, "popout");
   elm_entry_anchor_hover_parent_set(en, win);
   elm_object_text_set(en,
                       "This is an entry widget in this window that<br/>"
                       "uses markup <b>like this</> for styling and<br/>"
                       "formatting <em>like this</>, as well as<br/>"
                       "<a href=X><link>links in the text</></a>, so enter text<br/>"
                       "in here to edit it. By the way, links are<br/>"
                       "called <a href=anc-02>Anchors</a> so you will need<br/>"
                       "to refer to them this way. <item relsize=16x16 vsize=full href=emoticon/guilty-smile></item>");
   evas_object_smart_callback_add(en, "anchor,hover,opened", _anchor_hover_opened_cb, en);
   evas_object_smart_callback_add(en, "anchor,clicked", _anchor_clicked_cb, en);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, en);
   evas_object_show(en);

   elm_object_focus_set(win, EINA_TRUE);
   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}
