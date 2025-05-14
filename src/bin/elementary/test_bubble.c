#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#include "test_explode.h"

/**
 * @brief Callback function for the "clicked" event on a bubble.
 *
 * This function is called when a bubble widget is clicked. It simply prints
 * a message to standard output.
 * @param data User data, unused in this case.
 * @param obj The Evas_Object that emitted the event, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_print_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("bubble clicked\n");
}

/**
 * @brief Structure to hold data for the API test.
 *
 * This structure contains the necessary data to manage the state of the
 * bubble API test, including the current test state, the parent window,
 * and the container for the bubble widgets.
 */
struct _api_data
{
   unsigned int state;  /**< The current state of the API test, from api_state enum. */
   Evas_Object *win;    /**< The parent window containing the widgets. */
   void *box;           /**< The box container for the bubble widgets. */
};
typedef struct _api_data api_data;

/**
 * @brief Defines the different states for the bubble API test.
 *
 * Each state corresponds to a specific API function or property being tested.
 */
enum _api_state
{
   BUBBLE_SET_CORNER_1,       /**< Test setting bubble corners to bottom-left and top-right. */
   BUBBLE_SET_CORNER_2,       /**< Test setting bubble corners to top-right and bottom-left. */
   BUBBLE_SET_ICON_CONTENT,   /**< Test setting an icon and a label as content. */
   API_STATE_LAST             /**< Marker for the last state. */
};
typedef enum _api_state api_state;

/**
 * @brief Apply changes to the bubble widgets based on the current API test state.
 *
 * This function modifies the properties of the bubble widgets being tested
 * according to the current state in the api_data structure. It's called
 * to progress through the various API tests.
 * @param api The API test data structure.
 */
static void
set_api_state(api_data *api)
{
   const Eina_List *items = elm_box_children_get(api->box);
   if (!eina_list_count(items))
     return;

   switch(api->state)
     { /* Put all api-changes under switch */
      case BUBBLE_SET_CORNER_1:
         elm_bubble_pos_set(eina_list_nth(items, 0), ELM_BUBBLE_POS_BOTTOM_LEFT);
         elm_object_text_set(elm_object_content_get(eina_list_nth(items, 0)),
                  "Corner: base (bottom-left) - with icon");
         elm_bubble_pos_set(eina_list_nth(items, 1), ELM_BUBBLE_POS_TOP_RIGHT);
         elm_object_text_set(elm_object_content_get(eina_list_nth(items, 1)),
                  "Corner: base (top-right) - no icon");
         break;

      case BUBBLE_SET_CORNER_2:
         elm_bubble_pos_set(eina_list_nth(items, 0), ELM_BUBBLE_POS_TOP_RIGHT);
         elm_object_text_set(elm_object_content_get(eina_list_nth(items, 0)),
                  "Corner: base (top-right) - with icon");
         elm_bubble_pos_set(eina_list_nth(items, 1), ELM_BUBBLE_POS_BOTTOM_LEFT);
         elm_object_text_set(elm_object_content_get(eina_list_nth(items, 1)),
                  "Corner: base (bottom-left) - no icon");
         break;

      case BUBBLE_SET_ICON_CONTENT:
           {
              char buf[PATH_MAX];
              Evas_Object *ct, *ic = elm_icon_add(api->win);

              snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
              elm_image_file_set(ic, buf, NULL);
              elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
              elm_object_content_set(eina_list_nth(items, 0), ic);
              ct = elm_label_add(api->win);
              elm_object_text_set(ct, "Using icon as top-bubble content");
              elm_object_content_set(eina_list_nth(items, 1), ct);
              evas_object_size_hint_align_set(ic, 0.5, 0.5);
              evas_object_show(ic);
           }
         break;

      case API_STATE_LAST:

         break;
      default:
         return;
     }
}

/**
 * @brief Callback for the "Next API function" button.
 *
 * This function is triggered when the user clicks the button to advance to
 * the next API test. It increments the test state, calls set_api_state()
 * to apply the changes for the new state, and updates the button's text.
 * When the final state is reached, the button is disabled.
 * @param data The api_data structure for the test.
 * @param obj The button object that was clicked.
 * @param event_info Evas event info, unused.
 */
static void
_api_bt_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
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
 * @brief Callback to free allocated resources on window close.
 *
 * This function is registered with the EVAS_CALLBACK_FREE event on the
 * main window. It is responsible for freeing the api_data structure
 * when the window is destroyed.
 * @param data The api_data structure to be freed.
 * @param e The Evas canvas, unused.
 * @param obj The Evas_Object that is being freed, unused.
 * @param event_info Event-specific information, unused.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Main function to set up and run the bubble widget test.
 *
 * This function creates a window and populates it with two elm_bubble widgets
 * to demonstrate and test their functionality. It also sets up a button to
 * cycle through different API tests that modify the bubbles' properties,
 * such as their corner position and content.
 *
 * The test initializes two bubbles:
 * - The first bubble has an icon and text content.
 * - The second bubble has only text content.
 *
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 */
void
test_bubble(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *ic, *bb, *ct, *bxx, *bt;
   char buf[PATH_MAX];
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("bubble", "Bubble");
   explode_win_enable(win);
   api->win = win;
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   api->box = bx;
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   elm_box_pack_end(bxx, bx);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);

   bb = elm_bubble_add(win);
   elm_object_text_set(bb, "Message 1");
   elm_object_part_text_set(bb, "info", "Corner: bottom_right");
   elm_object_part_content_set(bb, "icon", ic);
   elm_bubble_pos_set(bb, ELM_BUBBLE_POS_BOTTOM_RIGHT);
   evas_object_smart_callback_add(bb, "clicked", _print_clicked, NULL);
   evas_object_show(ic);
   evas_object_size_hint_weight_set(bb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ct = elm_label_add(win);
   elm_object_text_set(ct,
                       "\"The future of the art: R or G or B?\",  by Rusty");
   elm_object_content_set(bb, ct);

   elm_box_pack_end(bx, bb);
   evas_object_show(bb);

   bb = elm_bubble_add(win);
   elm_object_text_set(bb, "Message 2");
   elm_object_part_text_set(bb, "info", "10:32 4/11/2008");
   evas_object_smart_callback_add(bb, "clicked", _print_clicked, NULL);
   evas_object_size_hint_weight_set(bb, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bb, EVAS_HINT_FILL, EVAS_HINT_FILL);

   ct = elm_label_add(win);
   elm_object_text_set(ct, "Corner: base (top-left) - no icon");
   elm_object_content_set(bb, ct);

   elm_box_pack_end(bx, bb);
   evas_object_show(bb);

   evas_object_show(win);
}
