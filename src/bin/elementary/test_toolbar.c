#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Creates a frame for focus autoscroll mode settings.
 * @param parent The parent object.
 * @return The created frame object.
 *
 * This function is declared here but defined elsewhere. It is used to create a UI
 * component for configuring the focus autoscroll mode.
 */
Evas_Object * _focus_autoscroll_mode_frame_create(Evas_Object *parent);

/**
 * @brief Callback function for "selected" and "unselected" events on the toolbar.
 * @param data A string indicating the event type ("selected" or "unselected").
 * @param obj The toolbar object.
 * @param event_info The toolbar item that was selected or unselected.
 *
 * This function prints details about the selected/unselected item to the console.
 */
static void
_selected_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   const char *str = elm_object_item_text_get(event_info);
   printf("item %p is %s.", event_info, (char *)data);
   if (str)
     printf(" string is \"%s\"\n", str);
   else
     printf("\n");
}

/**
 * @brief Callback for the first toolbar item selection.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Sets the image of the photo widget to "panel_01.jpg".
 */
static void
_tb_sel1_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/panel_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(data, buf);
}

/**
 * @brief Callback for the second toolbar item selection.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Sets the image of the photo widget to "rock_01.jpg".
 */
static void
_tb_sel2_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/rock_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(data, buf);
}

/**
 * @brief Callback for the third toolbar item selection.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Sets the image of the photo widget to "wood_01.jpg".
 */
static void
_tb_sel3_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/wood_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(data, buf);
}

/**
 * @brief Callback for a stateful toolbar item.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * This function first performs the action of _tb_sel3_cb, then transitions
 * the toolbar item to its next state. Used in test_toolbar5.
 */
static void
_tb_sel3a_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _tb_sel3_cb(data, obj, event_info);
   elm_toolbar_item_state_set(event_info, elm_toolbar_item_state_next(event_info));
}

/**
 * @brief Callback for a stateful toolbar item to unset its state.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * This function first performs the action of _tb_sel3_cb, then unsets
 * the state of the toolbar item, returning it to the default state.
 * Used in test_toolbar5.
 */
static void
_tb_sel3b_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _tb_sel3_cb(data, obj, event_info);
   elm_toolbar_item_state_unset(event_info);
}

/**
 * @brief Callback for the fourth toolbar item selection.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Sets the image of the photo widget to "sky_03.jpg".
 */
static void
_tb_sel4_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "%s/images/sky_03.jpg", elm_app_data_dir_get());
   elm_photo_file_set(data, buf);
}

/**
 * @brief Callback for another stateful toolbar item.
 * @param data Not used.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Transitions the toolbar item to its previous state. Used in test_toolbar5.
 */
static void
_tb_sel4a_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_toolbar_item_state_set(event_info, elm_toolbar_item_state_prev(event_info));
}

/**
 * @brief Callback for the fifth toolbar item selection.
 * @param data The photo widget to update.
 * @param obj The toolbar object.
 * @param event_info The selected toolbar item.
 *
 * Clears the image from the photo widget.
 */
static void
_tb_sel5_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_photo_file_set(data, NULL);
}

/**
 * @brief Callback for a "clicked" event on the toolbar.
 * @param data Not used.
 * @param obj The toolbar object.
 * @param event_info The clicked toolbar item.
 *
 * This function specifically handles clicks on the "more" item in toolbars
 * with ELM_TOOLBAR_SHRINK_EXPAND shrink mode. It toggles the text and icon
 * between an "Open" and "Close" state.
 */
static void
toolbar_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Elm_Object_Item *more_it = elm_toolbar_more_item_get(obj);

   printf("toolbar item %p is clicked\n", event_info);

   if (!strcmp(elm_object_item_text_get(more_it), "Open") &&
       (more_it == elm_toolbar_selected_item_get(obj)))
     {
        elm_toolbar_item_icon_set(more_it, "arrow_up");
        elm_object_item_text_set(more_it, "Close");
     }
   else if (!strcmp(elm_object_item_text_get(more_it), "Close"))
     {
        elm_toolbar_item_icon_set(more_it, "arrow_down");
        elm_object_item_text_set(more_it, "Open");
     }
}

/**
 * @brief Test case for a basic toolbar.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window with a toolbar to demonstrate basic
 * functionality. The toolbar uses ELM_TOOLBAR_SHRINK_MENU, which means
 * items that don't fit are moved into a menu.
 *
 * The test includes:
 * - Appending items with icons, labels, and callbacks.
 * - Setting item priority to influence which items are moved to the menu.
 * - Disabling an item.
 * - A special menu item that hosts a submenu.
 */
void
test_toolbar(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar", "Toolbar");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   evas_object_smart_callback_add(tb, "selected", _selected_cb, "selected");
   evas_object_smart_callback_add(tb, "unselected", _selected_cb, "unselected");
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
//   elm_object_scale_set(tb, 0.9);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Menu", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   //elm_table_homogeneous_set(tb, EINA_TRUE);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a toolbar with scrolling.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function demonstrates a toolbar with ELM_TOOLBAR_SHRINK_SCROLL mode.
 * In this mode, items that do not fit in the toolbar can be scrolled into view.
 * The toolbar is also set to be non-homogeneous, allowing items to have
 * different widths.
 */
void
test_toolbar2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar2", "Toolbar 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_SCROLL);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Scroll", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   //elm_table_homogeneous_set(tb, EINA_TRUE);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a non-shrinking toolbar.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test shows a toolbar with ELM_TOOLBAR_SHRINK_NONE. The toolbar will not
 * shrink and may overflow its container if items do not fit.
 * This test also shows using EINA_TRUE/EINA_FALSE for priority.
 */
void
test_toolbar3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar3", "Toolbar 3");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_NONE);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, EINA_FALSE);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -10);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 50);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "None", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a toolbar with hide shrink mode.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates a toolbar with ELM_TOOLBAR_SHRINK_HIDE mode.
 * Items that do not fit are simply hidden from view. This is similar to
 * the menu shrink mode but without providing a menu to access the hidden items.
 */
/* The same test of toolbar, but using hide shrink mode instead of menu */
void
test_toolbar4(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar4", "Toolbar 4");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_HIDE);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Menu", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a toolbar with stateful items.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates toolbar items that can have multiple states, each with
 * a different icon and label. Callbacks can be used to transition between
 * states. It also shows setting select mode to ELM_OBJECT_SELECT_MODE_NONE,
 * so items don't stay selected.
 */
/* Toolbar with multiple state buttons */
void
test_toolbar5(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar5", "Toolbar 5");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_NONE);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 100);

   snprintf(buf, sizeof(buf), "%s/images/icon_04.png", elm_app_data_dir_get());
   tb_it = elm_toolbar_item_append(tb, buf, "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3a_cb, ph4);
   elm_toolbar_item_state_add(tb_it, "object-rotate-left", "H2", _tb_sel3b_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4a_cb, ph4);
   elm_toolbar_item_state_add(tb_it, "emptytrash", "Comes2", _tb_sel4a_cb, ph4);
   elm_toolbar_item_state_add(tb_it, "trashcan_full", "Comes3", _tb_sel4a_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Menu", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a vertical toolbar.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates a vertical toolbar by setting its orientation.
 * It uses ELM_TOOLBAR_SHRINK_MENU for items that don't fit.
 */
void
test_toolbar_vertical(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar-vertical", "Toolbar Vertical");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_horizontal_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Menu", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a toolbar with horizontal item style.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates the "item_horizontal" style, where the icon and
 * label of each item are arranged horizontally next to each other instead of
 * the default vertical stacking.
 */
void
test_toolbar6(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *menu;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   Elm_Object_Item *menu_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar6", "Toolbar 6");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_object_style_set(tb, "item_horizontal");
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_MENU);
   evas_object_size_hint_weight_set(tb, 0.0, 0.0);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, 0.0);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, 100);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, -100);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 150);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 0);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   tb_it = elm_toolbar_item_append(tb, "refresh", "Menu", NULL, NULL);
   elm_toolbar_item_menu_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -9999);
   elm_toolbar_menu_parent_set(tb, win);
   menu = elm_toolbar_item_menu_get(tb_it);

   elm_menu_item_add(menu, NULL, "edit-cut", "Shrink", _tb_sel3_cb, ph4);
   menu_it = elm_menu_item_add(menu, NULL, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, menu_it, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_menu_item_add(menu, NULL, "edit-delete", "Menu", _tb_sel5_cb, ph4);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   //elm_table_homogeneous_set(tb, EINA_TRUE);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 80);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 20);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 60);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for an expanding toolbar.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates a toolbar with ELM_TOOLBAR_SHRINK_EXPAND mode.
 * In this mode, when the "more" item is clicked, the toolbar expands
 * transversely to show hidden items.
 *
 * This test also includes:
 * - Separators between items.
 * - An item containing a custom widget (a slider).
 * - A custom "clicked" callback to handle the "more" item's state.
 */
void
test_toolbar7(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph, *sl;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   Elm_Object_Item *tb_it;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar7", "Toolbar 7");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_EXPAND);
   elm_toolbar_transverse_expanded_set(tb, EINA_TRUE);
   elm_toolbar_standard_priority_set(tb, 0);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   tb_it = elm_toolbar_item_append(tb, "document-print", "Hello", _tb_sel1_cb, ph1);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_priority_set(tb_it, -100);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "folder-new", "World", _tb_sel2_cb, ph1);
   elm_toolbar_item_priority_set(tb_it, 100);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "object-rotate-right", "H", _tb_sel3_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -150);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   sl = elm_slider_add(win);
   evas_object_size_hint_min_set(sl, 100, 50);
   tb_it = elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL);
   elm_object_item_part_content_set(tb_it, "object", sl);

   elm_toolbar_item_priority_set(tb_it, 500);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-cut", "Shrink", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-copy", "Mode", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-paste", "is set to", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "edit-delete", "Menu", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "mail-send", "Comes", _tb_sel4_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, 200);

   elm_toolbar_item_separator_set(elm_toolbar_item_append(tb, NULL, NULL, NULL, NULL), EINA_TRUE);

   tb_it = elm_toolbar_item_append(tb, "clock", "Elementary", _tb_sel5_cb, ph4);
   elm_toolbar_item_priority_set(tb_it, -300);

   elm_object_item_text_set(elm_toolbar_more_item_get(tb), "Open");

   evas_object_smart_callback_add(tb, "clicked", toolbar_clicked_cb, NULL);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 80);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 160);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 120);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 420 * elm_config_scale_get(),
                           250 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test case for a reorderable and centered toolbar.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates an expanding toolbar with additional features:
 * - Reorder mode is enabled, allowing the user to drag and drop items to
 *   rearrange them.
 * - The "item_centered" style is used to center the item content.
 * - The select mode is ELM_OBJECT_SELECT_MODE_ALWAYS, so an item always
 *   remains selected.
 * - Items are created without labels, showing an icon-only toolbar.
 */
void
test_toolbar8(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *tb, *ph;
   Evas_Object *ph1, *ph2, *ph3, *ph4;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("toolbar8", "Toolbar 8");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   elm_win_resize_object_add(win, bx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bx);

   tb = elm_toolbar_add(win);
   elm_toolbar_homogeneous_set(tb, EINA_FALSE);
   elm_toolbar_shrink_mode_set(tb, ELM_TOOLBAR_SHRINK_EXPAND);
   elm_toolbar_transverse_expanded_set(tb, EINA_TRUE);
   elm_toolbar_reorder_mode_set(tb, EINA_TRUE);
   elm_toolbar_select_mode_set(tb, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_object_style_set(tb, "item_centered");
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph1 = elm_photo_add(win);
   ph2 = elm_photo_add(win);
   ph3 = elm_photo_add(win);
   ph4 = elm_photo_add(win);

   elm_toolbar_item_append(tb, "document-print", NULL, _tb_sel1_cb, ph1);
   elm_toolbar_item_append(tb, "folder-new", NULL, _tb_sel2_cb, ph1);
   elm_toolbar_item_append(tb, "object-rotate-right", NULL, _tb_sel3_cb, ph4);
   elm_toolbar_item_append(tb, "mail-send", NULL, _tb_sel4_cb, ph4);
   elm_toolbar_item_append(tb, "edit-cut", NULL, _tb_sel4_cb, ph4);
   elm_toolbar_item_append(tb, "edit-copy", NULL, _tb_sel4_cb, ph4);
   elm_toolbar_item_append(tb, "edit-paste", NULL, _tb_sel4_cb, ph4);
   elm_toolbar_item_append(tb, "edit-delete", NULL, _tb_sel4_cb, ph4);
   elm_toolbar_item_append(tb, "clock", NULL, _tb_sel5_cb, ph4);

   evas_object_smart_callback_add(tb, "clicked", toolbar_clicked_cb, NULL);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ph = ph1;
   elm_photo_size_set(ph, 80);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 0, 1, 1);
   evas_object_show(ph);

   ph = ph2;
   elm_photo_size_set(ph, 160);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 0, 1, 1);
   evas_object_show(ph);

   ph = ph3;
   elm_photo_size_set(ph, 40);
   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 0, 1, 1, 1);
   evas_object_show(ph);

   ph = ph4;
   elm_photo_size_set(ph, 120);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_photo_file_set(ph, buf);
   evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ph, 0.5, 0.5);
   elm_table_pack(tb, ph, 1, 1, 1, 1);
   evas_object_show(ph);

   elm_box_pack_end(bx, tb);
   evas_object_show(tb);

   evas_object_resize(win, 420 * elm_config_scale_get(),
                           250 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Generic callback for toolbar item events.
 * @param data A string describing the event (e.g., "clicked", "item,focused").
 * @param obj The toolbar object.
 * @param event_info The relevant item.
 *
 * Prints a message indicating which event was triggered on which item.
 * Used for debugging focus events in test_toolbar_focus.
 */
static void
_item_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("%s: %p\n", (char *)data, event_info);
}

/**
 * @brief Callback for the "Focus Highlight" checkbox.
 * @param data The window object.
 * @param obj The checkbox object.
 * @param event_info Not used.
 *
 * Toggles the focus highlight feature on the main window based on the
 * checkbox state.
 */
static void
_test_toolbar_focus_focus_highlight_check_changed(void *data, Evas_Object *obj,
                                                  void *event_info EINA_UNUSED)
{
   elm_win_focus_highlight_enabled_set(data,
                                       elm_check_state_get(obj));
}

/**
 * @brief Callback for the "Focus Animation" checkbox.
 * @param data The window object.
 * @param obj The checkbox object.
 * @param event_info Not used.
 *
 * Toggles the focus highlight animation on the main window based on the
 * checkbox state.
 */
static void
_test_toolbar_focus_focus_animate_check_changed(void *data, Evas_Object *obj,
                                                void *event_info EINA_UNUSED)
{
   elm_win_focus_highlight_animate_set(data,
                                       elm_check_state_get(obj));
}

/**
 * @brief Callback for the "First item focus on first focus in" checkbox.
 * @param data Not used.
 * @param obj The checkbox object.
 * @param event_info Not used.
 *
 * Configures whether the first focusable widget in a container should
 * receive focus when the container itself is first focused.
 */
static void
_test_toolbar_first_item_focus_on_first_focus_in_cb(void *data EINA_UNUSED, Evas_Object *obj,
                                                    void *event_info  EINA_UNUSED)
{
   elm_config_first_item_focus_on_first_focusin_set(elm_check_state_get(obj));
}

/**
 * @brief Callback for key down events on the toolbar.
 * @param data Not used.
 * @param e The Evas canvas.
 * @param obj The toolbar object.
 * @param event_info The key down event details.
 *
 * Prints the name of the key that was pressed while the toolbar had focus.
 */
static void
_toolbar_focus_key_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                           Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   printf("\n=== Key Down : %s ===\n", ev->keyname);
}

static Ecore_Timer *timer = NULL;
/**
 * @brief Callback for the test window's deletion.
 * @param data Not used.
 * @param e The Evas canvas.
 * @param obj The window object.
 * @param event_info Not used.
 *
 * Cleans up the focus timer when the window is closed to prevent it from
 * firing after the target object is destroyed.
 */
static void
_test_toolbar_focus_win_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED,
                               Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
   ecore_timer_del(timer);
   timer = NULL;
}

/**
 * @brief Timer callback to set focus on a toolbar item.
 * @param data The toolbar item to focus.
 * @return ECORE_CALLBACK_CANCEL to stop the timer from repeating.
 *
 * This function is called by a timer to set focus to a specific toolbar item.
 */
static Eina_Bool
_focus_timer_cb(void *data)
{
   Elm_Object_Item *it = data;
   elm_object_item_focus_set(it, EINA_TRUE);
   timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback for the button that sets focus to an item.
 * @param data The toolbar item to be focused.
 * @param obj The button object.
 * @param event_info Not used.
 *
 * Starts a timer that will focus the given toolbar item after a delay.
 * This allows observing the focus change visually.
 */
static void
_test_toolbar_focus_item_set_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                                    void *event_info EINA_UNUSED)
{
   ecore_timer_del(timer);
   timer = ecore_timer_add(1.5, _focus_timer_cb, data);
}

/**
 * @brief Callback for the button that disables a toolbar item.
 * @param data The toolbar item to be disabled.
 * @param obj The button object.
 * @param event_info Not used.
 *
 * Disables the specified toolbar item, making it non-interactive and
 * unfocusable.
 */
static void
_test_toolbar_focus_disable_item_btn_cb(void *data, Evas_Object *obj EINA_UNUSED,
                                        void *event_info EINA_UNUSED)
{
   elm_object_item_disabled_set(data, EINA_TRUE);
}

/**
 * @brief Callback for the focus movement policy radio buttons.
 * @param data Not used.
 * @param obj The radio button that changed.
 * @param event_info Not used.
 *
 * Sets the global focus movement policy based on which radio button is selected.
 * The policies are ELM_FOCUS_MOVE_POLICY_CLICK (focus follows clicks) and
 * ELM_FOCUS_MOVE_POLICY_IN (focus follows mouse pointer).
 */
static void
test_toolbar_focus_focus_move_policy_changed(void *data EINA_UNUSED,
                                             Evas_Object *obj,
                                             void *event_info EINA_UNUSED)
{
   int val = elm_radio_value_get(obj);

   if (val == 0)
     elm_config_focus_move_policy_set(ELM_FOCUS_MOVE_POLICY_CLICK);
   else if (val == 1)
     elm_config_focus_move_policy_set(ELM_FOCUS_MOVE_POLICY_IN);
}

/**
 * @brief Test case for toolbar focus handling.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window to test various aspects of focus management
 * within a toolbar widget. It includes:
 * - A scrollable toolbar with many items.
 * - Options to toggle focus highlight and animation.
 * - An option to test programmatic focus setting.
 * - An option to disable items and observe focus behavior.
 * - Options to change the focus movement policy.
 * - A key-down event listener to debug keyboard navigation.
 */
void
test_toolbar_focus(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *toolbar, *fr, *btn, *bx_opt, *chk, *bx_mv, *rd, *rdg;
   Elm_Object_Item *tb_it, *it_0, *it_3;

   win = elm_win_util_standard_add("toolbar-focus", "Toolbar Focus");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL,
                                  _test_toolbar_focus_win_del_cb, NULL);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_focus_highlight_animate_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   btn = elm_button_add(win);
   elm_object_text_set(btn, "Up");
   elm_box_pack_end(bx, btn);
   evas_object_show(btn);

   toolbar = elm_toolbar_add(win);
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
   evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0.0);
   it_0 = elm_toolbar_item_append(toolbar, "document-print", "Print", NULL, NULL);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   it_3 = elm_toolbar_item_append(toolbar, "clock", "Clock", NULL, NULL);
   elm_toolbar_item_append(toolbar, "refresh", "Update", NULL, NULL);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   elm_toolbar_item_append(toolbar, "clock", "Clock", NULL, NULL);
   tb_it = elm_toolbar_item_append(toolbar, "document-print", "Print", NULL, NULL);
   elm_object_item_disabled_set(tb_it, EINA_TRUE);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   elm_toolbar_item_append(toolbar, "refresh", "Update", NULL, NULL);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   elm_toolbar_item_append(toolbar, "clock", "Clock", NULL, NULL);
   elm_toolbar_item_append(toolbar, "document-print", "Print", NULL, NULL);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   elm_toolbar_item_append(toolbar, "refresh", "Update", NULL, NULL);
   elm_toolbar_item_append(toolbar, "folder-new", "Folder", NULL, NULL);
   elm_box_pack_end(bx, toolbar);
   evas_object_show(toolbar);
   evas_object_smart_callback_add(toolbar, "clicked", _item_cb, "clicked");
   evas_object_smart_callback_add(toolbar, "item,focused", _item_cb, "item,focused");
   evas_object_smart_callback_add(toolbar, "item,unfocused", _item_cb, "item,unfcoused");
   evas_object_event_callback_add(toolbar, EVAS_CALLBACK_KEY_DOWN, _toolbar_focus_key_down_cb, NULL);

   btn = elm_button_add(win);
   elm_object_text_set(btn, "Down");
   elm_box_pack_end(bx, btn);
   evas_object_show(btn);

   //Options
   fr = elm_frame_add(bx);
   elm_object_text_set(fr, "Options");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   bx_opt = elm_box_add(fr);
   elm_box_horizontal_set(bx_opt, EINA_TRUE);
   elm_object_content_set(fr, bx_opt);
   evas_object_show(bx_opt);

   chk = elm_check_add(bx_opt);
   elm_object_text_set(chk, "Focus Highlight");
   elm_check_state_set(chk, EINA_TRUE);
   evas_object_size_hint_weight_set(chk, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx_opt, chk);
   evas_object_show(chk);
   evas_object_smart_callback_add(chk, "changed",
                                  _test_toolbar_focus_focus_highlight_check_changed,
                                  win);

   chk = elm_check_add(bx_opt);
   elm_object_text_set(chk, "Focus Animation");
   elm_check_state_set(chk, EINA_TRUE);
   evas_object_size_hint_weight_set(chk, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx_opt, chk);
   evas_object_show(chk);
   evas_object_smart_callback_add(chk, "changed",
                                  _test_toolbar_focus_focus_animate_check_changed,
                                  win);

   chk = elm_check_add(bx_opt);
   elm_object_text_set(chk, "First item focus on first focus in");
   elm_check_state_set(chk, elm_config_first_item_focus_on_first_focusin_get());
   evas_object_size_hint_weight_set(chk, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx_opt, chk);
   evas_object_show(chk);
   evas_object_smart_callback_add(chk, "changed",
                                  _test_toolbar_first_item_focus_on_first_focus_in_cb,
                                  win);

   // Focus Autoscroll Mode
   fr = _focus_autoscroll_mode_frame_create(bx);
   elm_box_pack_end(bx, fr);

   // Focus movement policy
   fr = elm_frame_add(bx);
   elm_object_text_set(fr, "Focus Movement Policy");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   bx_mv = elm_box_add(fr);
   elm_box_horizontal_set(bx_mv, EINA_TRUE);
   elm_object_content_set(fr, bx_mv);
   evas_object_show(bx_mv);

   rdg = rd = elm_radio_add(bx_mv);
   elm_object_text_set(rd, "Focus Move by Click");
   elm_radio_state_value_set(rd, 0);
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, 0.0);
   evas_object_smart_callback_add(rd, "changed",
                                  test_toolbar_focus_focus_move_policy_changed,
                                  NULL);
   elm_box_pack_end(bx_mv, rd);
   evas_object_show(rd);

   rd = elm_radio_add(bx_mv);
   elm_object_text_set(rd, "Focus Move by Mouse-In");
   elm_radio_group_add(rd, rdg);
   elm_radio_state_value_set(rd, 1);
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, 0.0);
   evas_object_smart_callback_add(rd, "changed",
                                  test_toolbar_focus_focus_move_policy_changed,
                                  NULL);
   elm_box_pack_end(bx_mv, rd);
   evas_object_show(rd);

   fr = elm_frame_add(bx);
   elm_object_text_set(fr, "Focus");
   evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, fr);
   evas_object_show(fr);

   bx_opt = elm_box_add(fr);
   elm_object_content_set(fr, bx_opt);
   evas_object_show(bx_opt);

   btn = elm_button_add(bx_opt);
   elm_object_text_set(btn, "Set focus to 3rd toolbar item after 1.5 seconds.");
   evas_object_size_hint_weight_set(btn, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx_opt, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked",
                                  _test_toolbar_focus_item_set_btn_cb,
                                  it_3);

   btn = elm_button_add(bx_opt);
   elm_object_text_set(btn, "Disable 1st item.");
   evas_object_size_hint_weight_set(btn, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx_opt, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked",
                                  _test_toolbar_focus_disable_item_btn_cb,
                                  it_0);

   evas_object_resize(win, 420 * elm_config_scale_get(),
                           200 * elm_config_scale_get());
   evas_object_show(win);
}
