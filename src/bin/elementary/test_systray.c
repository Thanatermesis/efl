#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

// FIXME: systray has no proper legacy API and clearly isn't ready for prime
// time with EO either.
#include "elm_systray_eo.h"

#define WIDTH  320
#define HEIGHT 160

static Evas_Object *ai   = NULL;
static Evas_Object *i    = NULL;
static Evas_Object *r    = NULL;
static Eo          *item = NULL;

/**
 * @brief Handles the ELM_EVENT_SYSTRAY_READY event.
 *
 * This function is called when the system tray is ready to accept
 * new items. It attempts to register the global systray item 'item'.
 *
 * @param data User data, unused.
 * @param type The event type, unused.
 * @param event The event information, unused.
 * @return ECORE_CALLBACK_DONE to signify the event has been handled.
 */
static Eina_Bool
_ev_handler(void *data EINA_UNUSED,
            int type EINA_UNUSED,
            void *event EINA_UNUSED)
{
   Eina_Bool ret = EINA_FALSE;

   printf("systray ready event\n");

   ret = elm_obj_systray_register(item);

   printf("Item Registration: ");
   if (ret)
     printf("OK!\n");
   else
     printf("Failed!\n");

   return ECORE_CALLBACK_DONE;
}

/**
 * @brief Callback for the 'Update' button's 'clicked' event.
 *
 * This function updates the main icon and attention icon of the systray item
 * based on the current text in the icon name and attention icon name
 * entry fields.
 *
 * @param data User data, unused.
 * @param obj The button object, unused.
 * @param event_info The event information, unused.
 */
static void
_bt_clicked(void *data EINA_UNUSED,
            Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   elm_obj_systray_icon_name_set(item, elm_entry_entry_get(i));
   elm_obj_systray_att_icon_name_set(item, elm_entry_entry_get(ai));
}

/**
 * @brief Callback for the radio buttons' 'changed' event.
 *
 * Updates the systray item's status based on the selected radio button's
 * value. The status can be Passive, Active, or Needs Attention.
 *
 * @param data User data, unused.
 * @param obj The radio button object, unused.
 * @param event_info The event information, unused.
 */
static void
 _r_clicked(void *data EINA_UNUSED,
            Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   elm_obj_systray_status_set(item, elm_radio_value_get(r));
}

/**
 * @brief Callback for systray menu item 'clicked' events.
 *
 * Prints the name of the clicked menu item to standard output.
 * The item's name is passed as the @p data parameter.
 *
 * @param data The menu item's data, which is its name.
 * @param obj The menu object, unused.
 * @param event_info The event information, unused.
 */
static void
_menu_clicked(void *data,
            Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   printf("Menu Item Clicked: %s\n", (const char *) data);
}

/**
 * @brief Main function for the systray test application.
 *
 * This function creates a window with controls to manipulate a systray item.
 * It initializes the systray, creates a status notifier item, and provides
 * UI elements to change the item's icon, attention icon, and status. It also
 * attaches a context menu to the systray item.
 *
 * @param data User data, unused.
 * @param obj The object that triggered this test function, unused.
 * @param event_info The event information, unused.
 */
void
test_systray(void *data EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *it;
   Elm_Object_Item *it2;
   int st = 1;

   Eina_Bool init_ok = elm_need_systray();

   ecore_event_handler_add(ELM_EVENT_SYSTRAY_READY,
                           _ev_handler, NULL);

   win = elm_win_add(NULL, "Systray", ELM_WIN_BASIC);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_win_title_set(win, "Systemtray - Status Notifier Item");
   elm_win_autodel_set(win, EINA_TRUE);

   it = elm_menu_add(win);
   elm_menu_item_add(it, NULL, NULL, "root 1", _menu_clicked, "root 1");
   it2 = elm_menu_item_add(it, NULL, NULL, "root 2", _menu_clicked, "root 2");
   elm_menu_item_add(it, it2, NULL, "item 1",  _menu_clicked, "item 1");
   elm_menu_item_add(it, it2, NULL, "item 2",  _menu_clicked, "item 2");

   // Status Notifier Item Handler
   item = efl_add(ELM_SYSTRAY_CLASS, win);
   elm_obj_systray_icon_name_set(item, "elementary");
   elm_obj_systray_att_icon_name_set(item, "elementary");
   elm_obj_systray_menu_set(item, it);

   it = elm_bg_add(win);
   elm_win_resize_object_add(win, it);
   evas_object_size_hint_min_set(it, WIDTH, HEIGHT);
   evas_object_size_hint_max_set(it, WIDTH, HEIGHT);
   evas_object_show(it);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   if (!init_ok)
     {
        it = elm_label_add(win);
        evas_object_size_hint_align_set(it, 0, EVAS_HINT_FILL);
        elm_object_text_set(it, "Systray initialization failed. "
                            "This test will do nothing.");
        elm_box_pack_end(bx, it);
        evas_object_show(it);
     }

   it = elm_label_add(win);
   evas_object_size_hint_align_set(it, 0, EVAS_HINT_FILL);
   elm_object_text_set(it, "Icon:");
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   i = elm_entry_add(win);
   elm_entry_single_line_set(i, EINA_TRUE);
   elm_entry_scrollable_set(i, EINA_TRUE);
   elm_entry_entry_set(i, "elementary");
   evas_object_size_hint_align_set(i, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, i);
   evas_object_show(i);

   it = elm_label_add(win);
   evas_object_size_hint_align_set(it, 0, EVAS_HINT_FILL);
   elm_object_text_set(it, "Attention Icon:");
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   ai = elm_entry_add(win);
   elm_entry_single_line_set(ai, EINA_TRUE);
   elm_entry_scrollable_set(ai, EINA_TRUE);
   elm_entry_entry_set(ai, "elementary");
   evas_object_size_hint_align_set(ai, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, ai);
   evas_object_show(ai);

   it = elm_button_add(win);
   elm_object_text_set(it, "Update");
   evas_object_smart_callback_add(it, "clicked", _bt_clicked, NULL);
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   it = elm_separator_add(win);
   elm_separator_horizontal_set(it, EINA_TRUE);
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   it = elm_label_add(win);
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(it, "Status:");
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   it = elm_box_add(win);
   elm_box_horizontal_set(it, EINA_TRUE);
   elm_box_homogeneous_set(it, EINA_TRUE);
   elm_box_pack_end(bx, it);
   evas_object_show(it);
   bx = it;

   r = elm_radio_add(win);
   evas_object_size_hint_align_set(r, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(r, "changed", _r_clicked, NULL);
   elm_radio_value_pointer_set(r, &st);
   elm_radio_state_value_set(r, 0);
   elm_object_text_set(r, "Passive");
   elm_box_pack_end(bx, r);
   evas_object_show(r);

   it = elm_radio_add(win);
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(it, "changed", _r_clicked, NULL);
   elm_radio_state_value_set(it, 1);
   elm_radio_group_add(it, r);
   elm_object_text_set(it, "Active");
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   it = elm_radio_add(win);
   evas_object_size_hint_align_set(it, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(it, "changed", _r_clicked, NULL);
   elm_radio_state_value_set(it, 2);
   elm_radio_group_add(it, r);
   elm_object_text_set(it, "Needs Attention");
   elm_box_pack_end(bx, it);
   evas_object_show(it);

   evas_object_resize(win, WIDTH, HEIGHT);
   evas_object_show(win);
}
