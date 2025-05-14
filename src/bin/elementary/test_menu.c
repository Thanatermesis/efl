#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function called when a menu is dismissed.
 *
 * This function is registered with the "dismissed" smart callback of a menu
 * object. It simply prints a message to standard output indicating that it has
 * been called.
 */
static void
_menu_dismissed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   printf("menu dismissed callback is called!\n");
}

/**
 * @brief Callback function to show a menu on a mouse down event.
 *
 * This function is registered as an event callback for EVAS_CALLBACK_MOUSE_DOWN
 * on a clickable area. It retrieves the mouse coordinates from the event and
 * moves the menu to that position, then shows it.
 *
 * @param data The menu Evas_Object to be shown.
 * @param event_info The Evas_Event_Mouse_Down event structure containing
 *        cursor coordinates.
 */
static void
_menu_show_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   if (evas_object_visible_get(data)) return;
   elm_menu_move(data, ev->canvas.x, ev->canvas.y);
   evas_object_show(data);
}

/**
 * @brief Populates a menu item with a set of sub-items, including disabled ones.
 *
 * This is a helper function to add a specific set of items and separators
 * to a given parent menu item. It demonstrates adding standard items and
 * items that are explicitly disabled.
 *
 * @param menu The menu widget.
 * @param menu_it The parent menu item to which new items will be added.
 */
static void
_populate_4(Evas_Object *menu, Elm_Object_Item *menu_it)
{
   Elm_Object_Item *menu_it2;

   elm_menu_item_add(menu, menu_it, "go-bottom", "menu 2", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "go-first", "menu 3", NULL, NULL);
   elm_menu_item_separator_add(menu, menu_it);
   menu_it2 = elm_menu_item_add(menu, menu_it, "go-last", "Disabled item", NULL,
                                NULL);
   elm_object_item_disabled_set(menu_it2, EINA_TRUE);
   menu_it2 = elm_menu_item_add(menu, menu_it, "go-next", "Disabled item", NULL,
                                NULL);
   elm_object_item_disabled_set(menu_it2, EINA_TRUE);
   menu_it2 = elm_menu_item_add(menu, menu_it, "go-up", "Disabled item", NULL,
                                NULL);
   elm_object_item_disabled_set(menu_it2, EINA_TRUE);
}

/**
 * @brief Populates a menu item with media-related sub-items.
 *
 * This helper function adds media control-themed items to a parent menu item.
 * It includes a separator and a disabled item.
 *
 * @param menu The menu widget.
 * @param menu_it The parent menu item.
 */
static void
_populate_3(Evas_Object *menu, Elm_Object_Item *menu_it)
{
   Elm_Object_Item *menu_it2;

   elm_menu_item_add(menu, menu_it, "media-eject", "menu 2", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "media-playback-start", "menu 3", NULL,
                     NULL);
   elm_menu_item_separator_add(menu, menu_it);
   menu_it2 = elm_menu_item_add(menu, menu_it, "media-playback-stop",
                                "Disabled item", NULL, NULL);
   elm_object_item_disabled_set(menu_it2, EINA_TRUE);
}

/**
 * @brief Recursively populates a menu item with sub-items and sub-menus.
 *
 * This function builds a more complex menu structure. It adds items directly,
 * then creates sub-menus by calling _populate_3() and _populate_4() on newly
 * added items. It also demonstrates adding multiple separators.
 *
 * @param menu The menu widget.
 * @param menu_it The parent menu item to populate.
 */
static void
_populate_2(Evas_Object *menu, Elm_Object_Item *menu_it)
{
   Elm_Object_Item *menu_it2, *menu_it3;

   elm_menu_item_add(menu, menu_it, "system-reboot", "menu 2", NULL, NULL);
   menu_it2 = elm_menu_item_add(menu, menu_it, "system-shutdown", "menu 3",
                                NULL, NULL);
   _populate_3(menu, menu_it2);

   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_separator_add(menu, menu_it);

   menu_it2 = elm_menu_item_add(menu, menu_it, "system-lock-screen", "menu 2",
                                NULL, NULL);
   elm_menu_item_separator_add(menu, menu_it);

   menu_it3 = elm_menu_item_add(menu, menu_it, "system-run", "Disabled item",
                                NULL, NULL);
   elm_object_item_disabled_set(menu_it3, EINA_TRUE);

   _populate_4(menu, menu_it2);
}

/**
 * @brief Populates a top-level menu item, including a custom widget.
 *
 * This function demonstrates more advanced menu item creation. It adds a
 * standard item, then adds an item containing an elm_radio widget as its
 * content. It then calls _populate_2() to create a sub-menu.
 *
 * @param menu The menu widget.
 * @param menu_it The parent menu item to populate.
 */
static void
_populate_1(Evas_Object *menu, Elm_Object_Item *menu_it)
{
   Elm_Object_Item *menu_it2, *menu_it3;
   Evas_Object *radio;

   radio = elm_radio_add(menu);
   elm_radio_state_value_set(radio, 0);
   elm_radio_value_set(radio, 0);
   elm_object_text_set(radio, "radio in menu");
   menu_it2 = elm_menu_item_add(menu, menu_it, "object-rotate-left", "menu 1",
                                NULL, NULL);
   menu_it3 = elm_menu_item_add(menu, menu_it, NULL, NULL, NULL, NULL);
   elm_object_item_content_set(menu_it3, radio);

   _populate_2(menu, menu_it2);
}

/**
 * @brief Test function for a complex, nested menu.
 *
 * This test creates a window with a label and an invisible rectangle.
 * Clicking the rectangle area triggers a menu to appear. The menu is
 * populated with a deep hierarchy of items and sub-menus using the
 * _populate_* helper functions. This demonstrates how to create and show
 * a menu in response to a user event.
 */
void
test_menu(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
          void *event_info EINA_UNUSED)
{
   Evas_Object *win, *rect, *lbl, *menu;
   Elm_Object_Item *menu_it;

   win = elm_win_util_standard_add("menu", "Menu");
   elm_win_autodel_set(win, EINA_TRUE);

   lbl = elm_label_add(win);
   elm_object_text_set(lbl, "Click background to populate menu!");
   evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, lbl);
   evas_object_show(lbl);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, rect);
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_show(rect);

   menu = elm_menu_add(win);
   evas_object_smart_callback_add(menu, "dismissed", _menu_dismissed_cb, NULL);

   elm_menu_item_add(menu, NULL, NULL, "first item", NULL, NULL);

   menu_it = elm_menu_item_add(menu, NULL, "mail-reply-all", "second item",
                               NULL, NULL);
   _populate_1(menu, menu_it);

   elm_menu_item_add(menu, menu_it, "window-new", "sub menu", NULL, NULL);

   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN,
                                  _menu_show_cb, menu);

   evas_object_resize(win, 350 * elm_config_scale_get(),
                           200 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback to toggle the parent of a menu object.
 *
 * This function is triggered by a button click. It retrieves two potential
 * parent objects stored in the menu's data. It checks the current parent of
 * the menu and switches it to the other one, demonstrating dynamic
 * reparenting of a menu.
 *
 * @param data The menu Evas_Object whose parent will be changed.
 */
static void
_parent_set_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *mn = data;
   if (!mn) return;

   Evas_Object *parent = evas_object_data_get(mn, "parent_1");
   if (elm_menu_parent_get(mn) == parent)
     {
        parent = evas_object_data_get(mn, "parent_2");
     }

   elm_menu_parent_set(mn, parent);
}

/**
 * @brief Callback to toggle the icon of a menu item.
 *
 * Triggered by a button click, this function gets the icon name of a specific
 * menu item. It toggles the icon between "home" and "file", demonstrating
 * how to dynamically change a menu item's icon.
 *
 * @param data The Elm_Object_Item whose icon will be changed.
 */
static void
_icon_set_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   Elm_Object_Item *menu_it = data;
   const char *icon_name = NULL;
   if (!menu_it) return;

   icon_name = elm_menu_item_icon_name_get(menu_it);
   if ((icon_name) && !strcmp(icon_name, "home"))
     {
        elm_menu_item_icon_name_set(menu_it, "file");
        return;
     }
   elm_menu_item_icon_name_set(menu_it, "home");
}

/**
 * @brief Callback to toggle the selected state of a menu item.
 *
 * Triggered by a button click, this function inverts the 'selected' state
 * of the given menu item.
 *
 * @param data The Elm_Object_Item to be selected/deselected.
 */
static void
_item_select_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   Elm_Object_Item *menu_it = data;
   if (!menu_it) return;

   elm_menu_item_selected_set(menu_it, !elm_menu_item_selected_get(menu_it));
}

/**
 * @brief Callback to count and print the number of separators in a menu item's sub-menu.
 *
 * This function retrieves the list of sub-items for a given menu item,
 * iterates through them, and counts how many are separators. The result is
 * printed to standard output.
 *
 * @param data The Elm_Object_Item whose sub-items will be inspected.
 */
static void
_separators_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   const Eina_List *sis = NULL;
   const Eina_List *l = NULL;
   Elm_Object_Item *si;
   int separators = 0;

   Elm_Object_Item *menu_it = data;
   if (!menu_it) return;

   sis = elm_menu_item_subitems_get(menu_it);

   EINA_LIST_FOREACH(sis, l, si)
     if (elm_menu_item_is_separator(si)) separators++;

   printf("The number of separators: %d\n", separators);
}

/**
 * @brief Callback to programmatically open a menu.
 *
 * @param data The menu Evas_Object to be opened.
 */
static void
_open_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   Evas_Object *mn = data;
   if (!mn) return;

   elm_menu_open(mn);
}

/**
 * @brief Callback to programmatically close a menu.
 *
 * @param data The menu Evas_Object to be closed.
 */
static void
_close_bt_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *mn = data;
   if (!mn) return;

   elm_menu_close(mn);
}

/**
 * @brief Test function for menu manipulation.
 *
 * This test creates a window with a background image and a vertical box of
 * buttons. These buttons demonstrate various menu API functions:
 * - Opening and closing the menu programmatically.
 * - Changing a menu item's icon.
 * - Changing the menu's parent object.
 * - Selecting an item.
 * - Counting separators.
 * This provides a comprehensive test case for dynamic menu control.
 */
void
test_menu2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *o_bg, *rect, *rect2, *mn, *bt, *vbx;
   Elm_Object_Item *menu_it, *menu_it2;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("menu2", "Menu 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   o_bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/twofish.jpg", elm_app_data_dir_get());
   elm_bg_file_set(o_bg, buf, NULL);
   evas_object_size_hint_weight_set(o_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o_bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, o_bg);
   evas_object_show(o_bg);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_move(rect, 0, 0);
   evas_object_resize(rect, 124, 320);
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_show(rect);

   rect2 = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_move(rect2, 124, 0);
   evas_object_resize(rect2, 124, 320);
   evas_object_color_set(rect2, 0, 0, 0, 0);
   evas_object_show(rect2);

   mn = elm_menu_add(win);
   elm_menu_item_add(mn, NULL, NULL, "first item", NULL, NULL);
   menu_it = elm_menu_item_add(mn, NULL, NULL, "second item", NULL, NULL);
   elm_menu_item_add(mn, menu_it, NULL, "item 1", NULL, NULL);
   elm_menu_item_separator_add(mn, menu_it);
   elm_menu_item_add(mn, menu_it, NULL, "item 2", NULL, NULL);
   menu_it2 = elm_menu_item_add(mn, NULL, NULL, "third item", NULL, NULL);
   evas_object_data_set(mn, "parent_1", rect);
   evas_object_data_set(mn, "parent_2", rect2);

   vbx = elm_box_add(win);
   evas_object_show(vbx);
   elm_box_pack_end(bx, vbx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Menu Open");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _open_bt_clicked, mn);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Icon Set");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _icon_set_bt_clicked, menu_it);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Parent Set");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _parent_set_bt_clicked, mn);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Item Select");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _item_select_bt_clicked, menu_it2);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Separators");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _separators_bt_clicked, menu_it);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Menu Close");
   elm_box_pack_end(vbx, bt);
   evas_object_smart_callback_add(bt, "clicked", _close_bt_clicked, mn);
   evas_object_show(bt);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           320 * elm_config_scale_get());
   evas_object_show(win);
}
