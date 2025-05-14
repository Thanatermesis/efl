#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#define BUTTON_TEXT_SET(BT, TEXT) \
   elm_object_text_set((BT), (TEXT)); \
   elm_object_tooltip_text_set((BT), (TEXT)); \
   elm_object_tooltip_window_mode_set((BT), EINA_TRUE)

static char img1[PATH_MAX];
static char img2[PATH_MAX];
static char img3[PATH_MAX];
static char img4[PATH_MAX];
static char img5[PATH_MAX];
static char img6[PATH_MAX];
static char img7[PATH_MAX];

/**
 * @brief Creates a new photo object to be used as content in a naviframe item.
 * @param parent The parent widget.
 * @param img Path to the image file.
 * @return The new photo object.
 */
Evas_Object *
_content_new(Evas_Object *parent, const char *img)
{
   Evas_Object *photo = elm_photo_add(parent);
   elm_photo_file_set(photo, img);
   elm_photo_fill_inside_set(photo, EINA_TRUE);
   elm_object_style_set(photo, "shadow");
   return photo;
}

/**
 * @brief Callback function to pop an item from the naviframe.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_navi_pop(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_pop(data);
}

/**
 * @brief Callback function to delete a specific naviframe item.
 * @param data The naviframe item to be deleted.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_navi_it_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_object_item_del(data);
}

/**
 * @brief Callback function for the 'title,clicked' smart event of the naviframe.
 * @param data User data (unused).
 * @param obj The naviframe widget (unused).
 * @param event_info The event-specific data (unused).
 */
void
_title_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("Title Clicked!\n");
}

/**
 * @brief Callback function for the 'item,activated' smart event of the naviframe.
 * This is called when an item is shown and its transition has finished.
 * @param data User data (unused).
 * @param obj The naviframe widget (unused).
 * @param event_info The activated naviframe item.
 */
void
_item_activated(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;
   printf("Item(%p) is activated! The Title is \"%s\"\n", it, elm_object_item_text_get(it));
}

/**
 * @brief Toggles the visibility of the naviframe item's title with an animation.
 * @param data The naviframe item whose title visibility will be toggled.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_title_visible(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_title_enabled_set(data,
                                        !elm_naviframe_item_title_enabled_get(data),
                                        EINA_TRUE);
}

/**
 * @brief Promotes a naviframe item to be the top item on the stack.
 * @param data The naviframe item to promote.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_promote(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_naviframe_item_promote(data);
}

/**
 * @brief A callback function called when a naviframe item is being popped.
 * This function manually deletes the item and returns EINA_FALSE to prevent
 * the default pop transition and automatic deletion.
 * @param data User data (unused).
 * @param it The naviframe item being popped.
 * @return EINA_FALSE to prevent default pop behavior. Returning EINA_TRUE
 *         would allow the default transition and deletion.
 */
Eina_Bool
_pop_cb(void *data EINA_UNUSED, Elm_Object_Item *it)
{
   elm_object_item_del(it);

   /* If EINA_TRUE is returned, pop transition effect happens and then the item
    * is automatically deleted.
    * If EINA_FALSE is returned, pop transition effect does not happen and the
    * item is not automatically deleted.
    */
   return EINA_FALSE;
}

/**
 * @brief Creates and pushes page 9 onto the naviframe stack.
 * This page sets a custom pop callback (`_pop_cb`) for its item.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page9(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *bt2, *nf = data;
   Elm_Object_Item *it;

   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt, "Page 8");

   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Page 1");
   evas_object_smart_callback_add(bt2, "clicked", _promote,
                                  evas_object_data_get(nf, "page1"));

   it = elm_naviframe_item_push(nf, "Page 9", bt, bt2, NULL, NULL);
   elm_object_item_part_text_set(it, "subtitle", "Callback for naviframe item pop is set");

   elm_naviframe_item_pop_cb_set(it, _pop_cb, NULL);

   evas_object_smart_callback_add(bt, "clicked", _navi_pop, nf);
}

/**
 * @brief Creates and pushes page 8 onto the naviframe stack.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page8(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *bt2, *content, *nf = data;
   Elm_Object_Item *it;

   snprintf(img6, sizeof(img6), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt, "Page 7");

   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Page 9");
   evas_object_smart_callback_add(bt2, "clicked", _page9, nf);

   content = _content_new(nf, img6);
   it = elm_naviframe_item_push(nf, "Page 8", bt, bt2, content, NULL);
   elm_object_item_part_text_set(it, "subtitle", "Overlap style!");

   evas_object_smart_callback_add(bt, "clicked", _navi_pop, nf);
}

/**
 * @brief Callback for mouse down event on a button in page 7.
 * It changes the button color and pushes page 8.
 * @param data The naviframe widget.
 * @param e The Evas canvas (unused).
 * @param obj The button object.
 * @param event_info The event-specific data (unused).
 */
static void
_page7_btn_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   evas_object_color_set(obj, 100, 0, 0, 100);
   _page8(data, NULL, NULL);
}

/**
 * @brief Callback for mouse up event on a button in page 7.
 * It resets the button color.
 * @param data User data (unused).
 * @param e The Evas canvas (unused).
 * @param obj The button object.
 * @param event_info The event-specific data (unused).
 */
static void
_page7_btn_up_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                 void *event_info EINA_UNUSED)
{
   evas_object_color_set(obj, 255, 255, 255, 255);
   printf("Page7 Button Mouse Up!\n");
}

/**
 * @brief Creates a custom content object for page 7.
 * The content is a button with specific mouse down/up event handlers.
 * @param nf The naviframe widget, used as parent and passed to callbacks.
 * @return The new content object (a button).
 */
Evas_Object *
_page7_content_new(Evas_Object *nf)
{
   Evas_Object *bt;

   bt = elm_button_add(nf);
   elm_object_text_set(bt, "Page 8");
   evas_object_event_callback_add(bt, EVAS_CALLBACK_MOUSE_DOWN,
                                  _page7_btn_down_cb, nf);
   evas_object_event_callback_add(bt, EVAS_CALLBACK_MOUSE_UP,
                                  _page7_btn_up_cb, NULL);

   return bt;
}

/**
 * @brief Creates and pushes page 7 onto the naviframe stack.
 * This page uses the "overlap" style, where the new page content
 * animates over the previous page's content.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page7(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *bt2, *content, *nf = data;
   Elm_Object_Item *it;

   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt, "Page 6");

   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Page 8");
   evas_object_smart_callback_add(bt2, "clicked", _page8, nf);
   content = _page7_content_new(nf);
   it = elm_naviframe_item_push(nf, "Page 7", bt, bt2, content, "overlap");
   elm_object_item_part_text_set(it, "subtitle", "Overlap style!");

   evas_object_smart_callback_add(bt, "clicked", _navi_pop, nf);
}

/**
 * @brief Creates and pushes page 6 onto the naviframe stack.
 * This page also uses the "overlap" style.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page6(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *bt2, *content, *nf = data;
   Elm_Object_Item *it;

   snprintf(img7, sizeof(img7), "%s/images/sky_03.jpg", elm_app_data_dir_get());
   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt, "Page 5");

   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Page 7");
   evas_object_smart_callback_add(bt2, "clicked", _page7, nf);

   content = _content_new(nf, img7);
   it = elm_naviframe_item_push(nf, "Page 6", bt, bt2, content, "overlap");
   elm_object_item_part_text_set(it, "subtitle", "Overlap style!");

   evas_object_smart_callback_add(bt, "clicked", _navi_pop, nf);
}

/**
 * @brief Creates and inserts page 5 into the naviframe stack.
 * Instead of pushing on top, this page is inserted after the current top item
 * without any transition animation.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page5(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *bt2, *content, *nf = data;
   Elm_Object_Item *it;

   snprintf(img5, sizeof(img5), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt, "Page 4");

   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Page 6");
   evas_object_smart_callback_add(bt2, "clicked", _page6, nf);

   content = _content_new(nf, img5);
   it = elm_naviframe_item_insert_after(nf,
                                        elm_naviframe_top_item_get(nf),
                                        "Page 5",
                                        bt,
                                        bt2,
                                        content,
                                        NULL);
   elm_object_item_part_text_set(it, "subtitle", "This page is inserted after top item without transition");
   evas_object_smart_callback_add(bt, "clicked", _navi_it_del, it);
}

/**
 * @brief Creates and pushes page 4 onto the naviframe stack.
 * This page demonstrates hiding the title bar initially and allowing the user
 * to toggle its visibility by clicking the content area.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page4(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *ic, *content, *nf = data;
   char buf[PATH_MAX];
   Elm_Object_Item *it;

   snprintf(img4, sizeof(img4), "%s/images/rock_02.jpg", elm_app_data_dir_get());
   ic = elm_icon_add(nf);
   elm_icon_standard_set(ic, "go-right");

   bt = elm_button_add(nf);
   evas_object_smart_callback_add(bt, "clicked", _page5, nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_content_set(bt, "icon", ic);

   content = _content_new(nf, img4);

   it = elm_naviframe_item_push(nf,
                                "Page 4",
                                NULL,
                                bt,
                                content,
                                NULL);
   elm_object_item_part_text_set(it, "subtitle", "Title area visibility test");

   ic = elm_icon_add(nf);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_object_item_part_content_set(it, "icon", ic);
   elm_naviframe_item_title_enabled_set(it, EINA_FALSE, EINA_FALSE);
   evas_object_smart_callback_add(content, "clicked", _title_visible, it);
}

/**
 * @brief Creates and pushes page 3 onto the naviframe stack.
 * This page has a "Next" button but no "Back" button initially.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page3(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt2, *content, *nf = data;

   snprintf(img3, sizeof(img3), "%s/images/rock_01.jpg", elm_app_data_dir_get());
   bt2 = elm_button_add(nf);
   evas_object_size_hint_align_set(bt2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   BUTTON_TEXT_SET(bt2, "Next");
   evas_object_smart_callback_add(bt2, "clicked", _page4, nf);

   content = _content_new(nf, img3);

   elm_naviframe_item_push(nf,
                           "Page 3",
                           NULL,
                           bt2,
                           content,
                           NULL);
}

/**
 * @brief Creates and pushes page 2 onto the naviframe stack.
 * This page demonstrates using a long title and a subtitle.
 * @param data The naviframe widget.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The event-specific data (unused).
 */
void
_page2(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bt, *content, *ic, *nf = data;
   Elm_Object_Item *it;

   snprintf(img2, sizeof(img2), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   bt = elm_button_add(nf);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(bt, "clicked", _page3, nf);

   ic = elm_icon_add(nf);
   elm_icon_standard_set(ic, "arrow_right");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_layout_content_set(bt, "icon", ic);

   content = _content_new(nf, img2);

   it = elm_naviframe_item_push(nf, "Page 2 - Long Title Here",
                                NULL, bt, content,  NULL);
   elm_object_item_part_text_set(it, "subtitle", "Here is sub-title part!");
}

/**
 * @brief Test case for basic naviframe functionality.
 *
 * This test sets up a window with a naviframe and demonstrates:
 * - Pushing items (pages) onto the stack.
 * - Callbacks for title clicks and item activation.
 * - Page transitions.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_naviframe(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *nf, *btn, *content;
   Elm_Object_Item *it;

   snprintf(img1, sizeof(img1), "%s/images/logo.png", elm_app_data_dir_get());
   win = elm_win_util_standard_add("naviframe", "Naviframe");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   nf = elm_naviframe_add(win);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, nf);
   evas_object_show(nf);
   evas_object_smart_callback_add(nf, "title,clicked", _title_clicked, 0);
   evas_object_smart_callback_add(nf, "item,activated", _item_activated, NULL);

   btn = elm_button_add(nf);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(btn, "clicked", _page2, nf);
   BUTTON_TEXT_SET(btn, "Next");
   evas_object_show(btn);

   content = _content_new(nf, img1);
   it = elm_naviframe_item_push(nf, "Page 1", NULL, btn, content, NULL);
   evas_object_data_set(nf, "page1", it);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for placing widgets in the naviframe title bar.
 *
 * This test sets up a naviframe and places a segment control and a button
 * in the title bar area of an item. The title text itself is hidden.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_naviframe2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *nf, *sc, *btn, *ico, *content;
   Elm_Object_Item *it;

   snprintf(img1, sizeof(img1), "%s/images/logo.png", elm_app_data_dir_get());
   win = elm_win_util_standard_add("naviframe", "Naviframe");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   nf = elm_naviframe_add(win);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, nf);
   evas_object_show(nf);

   sc = elm_segment_control_add(nf);
   elm_segment_control_item_add(sc, NULL, "Show All");
   elm_segment_control_item_add(sc, NULL, "Just Filtered");

   btn = elm_button_add(nf);
   ico = elm_icon_add(btn);
   elm_icon_standard_set(ico, "refresh");
   elm_layout_content_set(btn, "icon", ico);

   content = _content_new(nf, img1);
   it = elm_naviframe_item_push(nf, NULL, NULL, btn, content, NULL);
   evas_object_data_set(nf, "page1", it);

   elm_object_item_part_content_set(it, "icon", sc);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback to pop all items from the naviframe stack until the root item is reached.
 * @param data The naviframe widget.
 * @param obj The button that was clicked. It holds a pointer to the root item.
 * @param event_info Not used.
 */
static void
_bt_pop_all(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Object_Item *it = evas_object_data_get(obj, "root");

   while (elm_naviframe_top_item_get(data) != it)
     elm_naviframe_item_pop(data);
}

/**
 * @brief Test case for naviframe used alongside another widget (toolbar).
 *
 * This test places a naviframe and a toolbar side-by-side in a box.
 * The toolbar contains a button to pop all pages from the naviframe,
 * demonstrating interaction between separate widgets.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_naviframe3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *tb, *bxh, *nf, *btn, *content;
   Elm_Object_Item *it;

   snprintf(img1, sizeof(img1), "%s/images/logo.png", elm_app_data_dir_get());
   win = elm_win_util_standard_add("naviframe", "Naviframe");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   bxh = elm_box_add(win);
   evas_object_size_hint_weight_set(bxh, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bxh, EINA_TRUE);
   elm_win_resize_object_add(win, bxh);
   evas_object_show(bxh);

   tb = elm_toolbar_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_toolbar_horizontal_set(tb, EINA_FALSE);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_box_pack_end(bxh, tb);
   evas_object_show(tb);

   nf = elm_naviframe_add(win);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(nf, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bxh, nf);
   evas_object_show(nf);

   btn = elm_button_add(nf);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(btn, "clicked", _page2, nf);
   BUTTON_TEXT_SET(btn, "Next");
   evas_object_show(btn);

   content = _content_new(nf, img1);
   it = elm_naviframe_item_push(nf, "Page 1", NULL, btn, content, NULL);
   evas_object_data_set(nf, "page1", it);

   evas_object_data_set(tb, "root", it);
   elm_toolbar_item_append(tb, NULL, "Pop all", _bt_pop_all, nf);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
