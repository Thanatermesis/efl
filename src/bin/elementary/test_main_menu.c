#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>
/**
 * @brief Callback function for the "click me" menu item.
 *
 * This function is called when the "click me" menu item is selected.
 * It toggles the disabled state of another menu item, which is passed
 * as the @p data parameter.
 *
 * @param data The menu item to be enabled/disabled.
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info The event-specific information (unused).
 */
static void
_click_me(void *data, Evas_Object *obj EINA_UNUSED,
          void *event_info EINA_UNUSED)
{
   Elm_Object_Item *it = data;
   Eina_Bool disabled = elm_object_item_disabled_get(it);
   printf("The first item is now %s\n", disabled ? "enabled" : "disabled");
   elm_object_item_disabled_set(it, !disabled);
}

/**
 * @brief Creates a test window to demonstrate the main menu functionality.
 *
 * This function sets up a window with a main menu containing various
 * items, submenus, separators, and items with callbacks. It also
 * demonstrates how to check for an environment variable
 * (`ELM_DISABLE_EXTERNAL_MENU`) to conditionally use a local menu
 * instead of a desktop-environment-provided one.
 *
 * The menu structure created is as follows:
 * - first item
 *   - elementary
 *   - submenu
 *     - first item
 *     - second item (with icon)
 * - second item
 *   - disabled item
 *   - --- (separator)
 *   - click me :-) (triggers _click_me callback)
 *   - third item (with icon)
 *   - sub menu
 *     - first item
 *
 * @param data Custom data pointer (unused).
 * @param obj The Evas_Object that triggered the event (unused).
 * @param event_info The event-specific information (unused).
 */
void
test_main_menu(void *data EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   Evas_Object *win, *menu, *label, *bx;
   Elm_Object_Item *menu_it, *menu_it1;
   char *s;
   Eina_Bool enabled = EINA_TRUE;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   win = elm_win_util_standard_add("menu", "Menu");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   label = elm_label_add(win);
   elm_object_text_set(label, "Note: the D-Bus menu example requires support from the "
                              "desktop environment to display the application menu");
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_label_line_wrap_set(label, ELM_WRAP_WORD);
   elm_box_pack_end(bx, label);
   evas_object_show(label);

   s = getenv("ELM_DISABLE_EXTERNAL_MENU");
   if (s)
     enabled = !atoi(s);

   if (!enabled)
     {
        label = elm_label_add(win);
        elm_object_text_set(label, "(ELM_DISABLE_EXTERNAL_MENU environment "
                            "variable is set. Using local menu instead)");
        evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_label_line_wrap_set(label, ELM_WRAP_WORD);
        elm_box_pack_end(bx, label);
        evas_object_show(label);
     }

   menu = elm_win_main_menu_get(win);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "first item", NULL, NULL);
   elm_menu_item_add(menu, menu_it, "elementary", "first item", NULL, NULL);
   menu_it1 = elm_menu_item_add(menu, menu_it, NULL, "submenu", NULL, NULL);
   elm_menu_item_add(menu, menu_it1, NULL, "first item", NULL, NULL);
   elm_menu_item_add(menu, menu_it1, "gimp", "second item", NULL, NULL);

   menu_it = elm_menu_item_add(menu, NULL, NULL, "second item", NULL, NULL);
   menu_it1 = elm_menu_item_add(menu, menu_it, NULL, "disabled item", NULL, NULL);
   elm_object_item_disabled_set(menu_it1, EINA_TRUE);
   elm_menu_item_separator_add(menu, menu_it);
   elm_menu_item_add(menu, menu_it, NULL, "click me :-)", _click_me, menu_it1);
   elm_menu_item_add(menu, menu_it, "applications-email-panel", "third item", NULL, NULL);
   menu_it1 = elm_menu_item_add(menu, menu_it, NULL, "sub menu", NULL, NULL);
   elm_menu_item_add(menu, menu_it1, NULL, "first item", NULL, NULL);

   evas_object_resize(win, 250 * elm_config_scale_get(),
                           350 * elm_config_scale_get());
   evas_object_show(win);
}
