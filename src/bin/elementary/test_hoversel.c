#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

struct _api_data
{
   unsigned int state;  /* What state we are testing       */
   void *box;           /* Use this to get box content     */
};
typedef struct _api_data api_data;

enum _api_state
{
   HOVERSEL_HORIZ,
   HOVERSEL_END,
   HOVERSEL_LABAL_SET,
   HOVERSEL_ICON_UNSET,
   HOVERSEL_CLEAR_OPEN,
   HOVERSEL_CLEAR,
   API_STATE_LAST
};
typedef enum _api_state api_state;

/**
 * @brief Sequentially test hoversel API functions.
 * @param api The api_data struct with the test state.
 *
 * This function is part of the API test harness. It is called when the
 * "Next API function" button is clicked. It modifies the hoversel widgets
 * based on the current test state.
 */
static void
set_api_state(api_data *api)
{
   Evas_Object *icon;

   const Eina_List *items = elm_box_children_get(api->box);
   if (!eina_list_count(items))
     return;

   /* use elm_box_children_get() to get list of children */
   switch(api->state)
     { /* Put all api-changes under switch */
      case HOVERSEL_HORIZ:  /* Make first hover horiz (0) */
         elm_hoversel_horizontal_set(eina_list_nth(items, 0), EINA_TRUE);
         elm_hoversel_hover_begin(eina_list_nth(items, 0));
         break;

      case HOVERSEL_END:  /* Make first hover horiz (1) */
         elm_hoversel_hover_begin(eina_list_nth(items, 1));
         elm_hoversel_hover_end(eina_list_nth(items, 1));
         break;

      case HOVERSEL_LABAL_SET: /* set second hover label (2) */
         elm_object_text_set(eina_list_nth(items, 1), "Label from API");
         break;

      case HOVERSEL_ICON_UNSET: /* 3 */
         elm_object_text_set(eina_list_nth(items, 5), "Label only");
         icon = elm_object_part_content_unset(eina_list_nth(items, 5), "icon");
         evas_object_del(icon);
         break;

      case HOVERSEL_CLEAR_OPEN: /* 4 */
         elm_hoversel_hover_begin(eina_list_nth(items, 1));
         elm_hoversel_clear(eina_list_nth(items, 1));
         break;

      case HOVERSEL_CLEAR: /* 5 */
         elm_hoversel_clear(eina_list_nth(items, 0));
         break;

      case API_STATE_LAST:
         break;

      default:
         return;
     }
}

/**
 * @brief Callback for the "Next API function" button.
 * @param data The api_data struct.
 * @param obj The button object.
 * @param event_info Not used.
 *
 * This function advances the API test to the next state, calls set_api_state()
 * to apply the changes for the new state, and updates the button label.
 * The button is disabled when the last test state is reached.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{  /* Will add here a SWITCH command containing code to modify test-object */
   /* in accordance a->state value. */
   api_data *a = data;
   char str[128];

   printf("clicked event on API Button: api_state=<%d>\n", a->state);
   set_api_state(a);
   a->state++;
   sprintf(str, "Next API function (%u)", a->state);
   elm_object_text_set(obj, str);
   elm_object_disabled_set(obj, a->state == API_STATE_LAST);
}

/**
 * @brief Frees the api_data structure when the window is closed.
 * @param data The api_data struct to free.
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Callback for the "clicked" smart event of a hoversel.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function is called when the hoversel button itself is clicked,
 * before it expands.
 */
static void
_hoversel_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   printf("Hover button is clicked and 'clicked' callback is called.\n");
}

/**
 * @brief Callback for the "clicked" smart event to dynamically populate items.
 * @param data Not used.
 * @param obj The hoversel object to populate.
 * @param event_info Not used.
 *
 * This demonstrates adding hoversel items dynamically when the hoversel
 * button is clicked.
 */
static void
_hoversel_clicked_populate_cb(void *data EINA_UNUSED, Evas_Object *obj,
                              void *event_info EINA_UNUSED)
{
   printf("Hover button is clicked and 'clicked' callback is called.\n");
   elm_hoversel_item_add(obj, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(obj, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(obj, "Item 3", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(obj, "Item 4", NULL, ELM_ICON_NONE, NULL, NULL);
}

/**
 * @brief Callback for the "selected" smart event.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The selected Elm_Object_Item.
 *
 * This function is called when an item in the hoversel is selected.
 */
static void
_hoversel_selected_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info)
{
   const char *txt = elm_object_item_text_get(event_info);

   printf("'selected' callback is called. (selected item : %s)\n", txt);
}

/**
 * @brief Callback for the "dismissed" smart event.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This is called when the hoversel popup is dismissed, either by selecting an
 * item or by clicking outside the popup.
 */
static void
_hoversel_dismissed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   printf("'dismissed' callback is called.\n");
}

/**
 * @brief Callback for the "expanded" smart event.
 * @param data An Elm_Object_Item to be modified.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This is called when the hoversel expands to show its items. If data is
 * not NULL, it demonstrates changing the style of the provided hoversel item
 * dynamically.
 */
static void
_hoversel_expanded_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   Elm_Object_Item *it = data;

   printf("'expanded' callback is called.\n");
   if (it)
     {
        printf("old style: %s\n", elm_object_item_style_get(it));
        //item type is button. set the style of button
        elm_object_item_style_set(it, "anchor");
        printf("new style: %s\n", elm_object_item_style_get(it));
     }
}

/**
 * @brief Main test function for hoversel widgets.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window and populates it with various hoversel
 * widgets to test different configurations and features, including:
 * - Simple hoversel with text items.
 * - Hoversel with icons.
 * - Dynamically populated hoversel.
 * - Disabled hoversel.
 * - Hoversel with custom item style.
 * - An API test runner to check various API functions.
 */
void
test_hoversel(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bt, *ic, *bxx, *hoversel;
   Elm_Object_Item *it;
   char buf[PATH_MAX];
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("hoversel", "HoverSel");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   api->box = bx;
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   elm_box_pack_end(bxx, bx);

   hoversel = elm_hoversel_add(win);
// FIXME: need to add horizontal hoversel theme to default some day
//   elm_hoversel_horizontal_set(bt, EINA_TRUE);
   elm_hoversel_auto_update_set(hoversel, EINA_TRUE);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Labels");
   elm_hoversel_item_add(hoversel, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 4 - Long Label Here", "close", ELM_ICON_STANDARD, NULL, NULL);
   it = elm_hoversel_item_add(hoversel, "Item 5 - Disabled", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_object_item_disabled_set(it, EINA_TRUE);
   evas_object_smart_callback_add(hoversel, "clicked",
                                  _hoversel_clicked_cb, NULL);
   evas_object_smart_callback_add(hoversel, "selected",
                                  _hoversel_selected_cb, NULL);
   evas_object_smart_callback_add(hoversel, "dismissed",
                                  _hoversel_dismissed_cb, NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_auto_update_set(hoversel, EINA_TRUE);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Some Icons");
   elm_hoversel_item_add(hoversel, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 5 - Long Label Here", NULL, ELM_ICON_NONE, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 6", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 7", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 8", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 9", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 10", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 11", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 12", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "All Icons");
   elm_hoversel_item_add(hoversel, "Item 1", "apps", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 2", "arrow_down", ELM_ICON_STANDARD,
                         NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "All Icons");
   elm_hoversel_item_add(hoversel, "Item 1", "apps", ELM_ICON_STANDARD, NULL,
                         NULL);
   snprintf(buf, sizeof(buf), "%s/images/sky_02.jpg", elm_app_data_dir_get());
   elm_hoversel_item_add(hoversel, "Item 2", buf, ELM_ICON_FILE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Disabled Hoversel");
   elm_hoversel_item_add(hoversel, "Item 1", "apps", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 2", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_object_disabled_set(hoversel, EINA_TRUE);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Icon + Label");

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/sky_03.jpg", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_object_part_content_set(hoversel, "icon", ic);
   evas_object_show(ic);

   elm_hoversel_item_add(hoversel, "Item 1", "apps", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 2", "arrow_down", ELM_ICON_STANDARD,
                         NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_auto_update_set(hoversel, EINA_TRUE);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Custom Item Style");
   elm_hoversel_item_add(hoversel, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 4", NULL, ELM_ICON_NONE, NULL, NULL);
   it = elm_hoversel_item_add(hoversel, "Manage items", NULL, ELM_ICON_NONE, NULL, NULL);
   evas_object_smart_callback_add(hoversel, "clicked",
                                  _hoversel_clicked_cb, NULL);
   evas_object_smart_callback_add(hoversel, "selected",
                                  _hoversel_selected_cb, NULL);
   evas_object_smart_callback_add(hoversel, "dismissed",
                                  _hoversel_dismissed_cb, NULL);
   //pass the last item as data and use elm_object_item_style_set() to change the item style.
   evas_object_smart_callback_add(hoversel, "expanded",
                                  _hoversel_expanded_cb, it);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Add items when clicked");
   evas_object_smart_callback_add(hoversel, "clicked",
                                  _hoversel_clicked_populate_cb, NULL);
   evas_object_smart_callback_add(hoversel, "selected",
                                  _hoversel_selected_cb, NULL);
   evas_object_smart_callback_add(hoversel, "dismissed",
                                  _hoversel_dismissed_cb, NULL);
   evas_object_smart_callback_add(hoversel, "expanded",
                                  _hoversel_expanded_cb, NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           500 * elm_config_scale_get());

   evas_object_show(win);
}

/**
 * @brief Callback for the "item,focused" smart event.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The Elm_Object_Item that received focus.
 *
 * This function is called when an item within the hoversel's popup gains focus.
 */
static void
_item_focused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;

   printf("item,focused: %p\n", it);
}

/**
 * @brief Callback for the "item,unfocused" smart event.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The Elm_Object_Item that lost focus.
 *
 * This function is called when an item within the hoversel's popup loses focus.
 */
static void
_item_unfocused_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Object_Item *it = event_info;

   printf("item,unfocused: %p\n", it);
}
/**
 * @brief Another "expanded" callback for focus testing.
 * @param data The Elm_Object_Item to focus.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This callback is used in the focus test to programmatically set focus to a
 * specific item when the hoversel is expanded.
 */
static void
_hoversel_expanded_cb2(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Object_Item *it = data;

   printf("expanded\n");
   elm_object_item_focus_set(it, EINA_TRUE);
   printf("focus set to item: %p\n", it);
}

/**
 * @brief Test function for hoversel focus handling.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function creates a window to test focus behavior on hoversel widgets and
 * their items. It includes tests for:
 * - Vertical and horizontal hoversels.
 * - Programmatically setting focus on an item.
 * - Callbacks for item focus events.
 */
void
test_hoversel_focus(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *hoversel;
   Elm_Object_Item *it;

   win = elm_win_util_standard_add("hoversel focus", "Hoversel Focus");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_focus_highlight_animate_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Vertical");
   elm_hoversel_item_add(hoversel, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 4 - Long Label Here", "close", ELM_ICON_STANDARD, NULL, NULL);
   evas_object_smart_callback_add(hoversel, "clicked",
                                  _hoversel_clicked_cb, NULL);
   evas_object_smart_callback_add(hoversel, "selected",
                                  _hoversel_selected_cb, NULL);
   evas_object_smart_callback_add(hoversel, "dismissed",
                                  _hoversel_dismissed_cb, NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);
   elm_object_focus_set(hoversel, EINA_TRUE);
   evas_object_smart_callback_add(hoversel, "item,focused", _item_focused_cb, NULL);
   evas_object_smart_callback_add(hoversel, "item,unfocused", _item_unfocused_cb, NULL);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_horizontal_set(hoversel, EINA_TRUE);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Horizontal");
   elm_hoversel_item_add(hoversel, "Item 1", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 2", NULL, ELM_ICON_NONE, NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);
   evas_object_smart_callback_add(hoversel, "item,focused", _item_focused_cb, NULL);
   evas_object_smart_callback_add(hoversel, "item,unfocused", _item_unfocused_cb, NULL);

   hoversel = elm_hoversel_add(win);
   elm_hoversel_hover_parent_set(hoversel, win);
   elm_object_text_set(hoversel, "Icons");
   elm_hoversel_item_add(hoversel, "Item 1", "apps", ELM_ICON_STANDARD, NULL,
                         NULL);
   it = elm_hoversel_item_add(hoversel, "Item 2", "arrow_down", ELM_ICON_STANDARD,
                              NULL, NULL);
   elm_hoversel_item_add(hoversel, "Item 3", "home", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_hoversel_item_add(hoversel, "Item 4", "close", ELM_ICON_STANDARD, NULL,
                         NULL);
   elm_box_pack_end(bx, hoversel);
   evas_object_show(hoversel);
   evas_object_smart_callback_add(hoversel, "item,focused", _item_focused_cb, NULL);
   evas_object_smart_callback_add(hoversel, "item,unfocused", _item_unfocused_cb, NULL);
   evas_object_smart_callback_add(hoversel, "expanded",
                                  _hoversel_expanded_cb2, it);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           500 * elm_config_scale_get());
   evas_object_show(win);
}
