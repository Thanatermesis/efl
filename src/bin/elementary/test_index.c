#include "test.h"
#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Structure to hold data for the index test.
 *
 * This structure contains a pointer to the index widget and an item
 * that is used for search operations within the test.
 */
struct _Idx_Data_Type
{
   Evas_Object *id;  /**< Pointer to Index widget */
   Elm_Object_Item *item; /**< Item we use for search */
};
typedef struct _Idx_Data_Type Idx_Data_Type;

/**
 * @brief Structure to hold API test data.
 *
 * This contains the current state of the API test and the
 * index data associated with it.
 */
struct _api_data
{
   unsigned int state;  /**< What state we are testing */
   Idx_Data_Type dt; /**< Index data for the test */
};
typedef struct _api_data api_data;

/**
 * @brief Enumeration of API test states.
 *
 * Each value corresponds to a specific API function of the index
 * widget that is being tested.
 */
enum _api_state
{
   INDEX_LEVEL_SET, /**< Test elm_index_item_level_set() */
   INDEX_ACTIVE_SET, /**< Test elm_index_autohide_disabled_set() */
   INDEX_DELAY_CHANGE_TIME_SET, /**< Test elm_index_delay_change_time_set() */
   INDEX_APPEND_RELATIVE, /**< Test elm_index_item_insert_after() and elm_index_item_insert_before() */
   INDEX_PREPEND, /**< Test elm_index_item_prepend() */
   INDEX_ITEM_DEL, /**< Test elm_object_item_del() on an index item */
   INDEX_ITEM_FIND, /**< Test elm_index_item_find() */
   INDEX_HORIZONTAL, /**< Test elm_index_horizontal_set() */
   INDEX_INDICATOR_DISABLED, /**< Test elm_index_indicator_disabled_set() */
   INDEX_CLEAR, /**< Test elm_index_item_clear() */
   API_STATE_LAST /**< Marker for the end of API states */
};
typedef enum _api_state api_state;

/**
 * @brief Sequentially tests the API of the Index widget.
 * @param api The API data structure containing the current test state.
 *
 * This function is called to apply a specific test case on the index
 * widget based on the state value in the @p api parameter.
 */
static void
set_api_state(api_data *api)
{
   Idx_Data_Type *d = &api->dt;
   switch(api->state)
     { /* Put all api-changes under switch */
      case INDEX_LEVEL_SET:
         elm_index_autohide_disabled_set(d->id, EINA_TRUE);
         elm_index_item_level_set(d->id, (elm_index_item_level_get(d->id) ? 0 : 1));
         break;

      case INDEX_ACTIVE_SET:
         elm_index_autohide_disabled_set(d->id, EINA_FALSE);
         break;

      case INDEX_DELAY_CHANGE_TIME_SET:
         elm_index_delay_change_time_set(d->id, 1.0);
         break;

      case INDEX_APPEND_RELATIVE:
         elm_index_item_insert_after(d->id,
                                     elm_index_item_find(d->id, d->item),
                                     "W", NULL, d->item);
         elm_index_item_insert_before(d->id,
                                      elm_index_item_find(d->id, d->item),
                                      "V", NULL, d->item);
         break;

      case INDEX_PREPEND:
         elm_index_item_prepend(d->id, "D", NULL, d->item);
         break;

      case INDEX_ITEM_DEL:
         elm_object_item_del(elm_index_item_find(d->id, d->item));
         break;

      case INDEX_ITEM_FIND:
           {
              Elm_Object_Item *i = elm_index_item_find(d->id, d->item);
              if (i)
                {
                   printf("Item Find - Found Item.\n");
                   elm_object_item_del(i);
                }
           }
         break;

      case INDEX_HORIZONTAL:
         elm_index_horizontal_set(d->id, EINA_TRUE);
         break;

      case INDEX_INDICATOR_DISABLED:
         elm_index_indicator_disabled_set(d->id, EINA_TRUE);
         break;

      case INDEX_CLEAR:
         elm_index_item_clear(d->id);
         break;

      case API_STATE_LAST:
         break;

      default:
         return;
     }
}

/**
 * @brief Callback for the 'Next API' button click.
 * @param data The api_data struct.
 * @param obj The button object.
 * @param event_info Not used.
 *
 * This function is called when the "Next API function" button is clicked.
 * It triggers the next API test case, updates the button's text to
 * reflect the new state, and disables the button when all tests are done.
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

static Elm_Genlist_Item_Class itci;
/**
 * @brief Get the text for a genlist item.
 * @param data The item data.
 * @param obj The genlist object.
 * @param part The part name.
 * @return A newly allocated string for the item's text.
 *
 * This function generates a two-character text label for genlist items,
 * based on the integer value passed in @p data. For example, if data is 0,
 * it returns "Aa".
 */
static char *
_gli_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   char buf[256];
   int j = (uintptr_t)data;
   snprintf(buf, sizeof(buf), "%c%c",
            'A' + ((j >> 4) & 0xf),
            'a' + ((j     ) & 0xf)
            );
   return strdup(buf);
}

/**
 * @brief Callback for the "delay,changed" smart event of the index.
 * @param data Not used.
 * @param obj The index object.
 * @param event_info The selected index item.
 *
 * This callback is invoked after a series of rapid changes to the index have
 * ceased, after a certain delay. It brings the corresponding genlist item
 * to the top of the viewport.
 */
static void
_index_delay_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   // called on a change but delayed in case multiple changes happen in a
   // short timespan
   elm_genlist_item_bring_in(elm_object_item_data_get(event_info),
                              ELM_GENLIST_ITEM_SCROLLTO_TOP);
}

/**
 * @brief Callback for the "changed" smart event of the index.
 * @param data Not used.
 * @param obj The index object.
 * @param event_info The selected index item.
 *
 * This is called on every single change of the selected item in the index.
 */
static void
_index_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   // this is called on every change, no matter how often
   // elm_genlist_item_bring_in(event_info);
}

/**
 * @brief Callback for the "selected" smart event of the index.
 * @param data Not used.
 * @param obj The index object.
 * @param event_info The selected index item.
 *
 * This callback is invoked when an index item is selected (e.g., on mouse up).
 * It brings the corresponding genlist item to the top of the viewport.
 */
static void
_index_selected_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   // called on final select
   elm_genlist_item_bring_in(elm_object_item_data_get(event_info),
                              ELM_GENLIST_ITEM_SCROLLTO_TOP);
}

/**
 * @brief Cleanup callback to free allocated data.
 * @param data The data to be freed (api_data struct).
 * @param e Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This is called when the window is destroyed, ensuring that dynamically
 * allocated memory for test data is released.
 */
static void
_cleanup_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Callback function for an index item selection.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The selected index item.
 *
 * This function is associated with individual index items. When an item
 * is selected, it prints the letter of that item.
 */
static void
_id_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("Current Index : %s\n", elm_index_item_letter_get((const Elm_Object_Item *)event_info));
}

/**
 * @brief Callback for the 'Omit mode' checkbox change.
 * @param data The index widget.
 * @param obj The checkbox object.
 * @param event_info Not used.
 *
 * This is called when the state of the "Omit mode" checkbox changes.
 * It enables or disables the omit feature on the index widget, which
 * hides index items that don't have corresponding active items in the list.
 */
static void
_omit_check_changed_cb(void *data, Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *id = data;
   Eina_Bool omit = elm_check_state_get(obj);
   if (!id) return;

   printf("Omit feature enabled : %d\n", omit);
   elm_index_omit_enabled_set(id, omit);
}

/**
 * @brief The main test function for the Index widget.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test creates a window with a Genlist and an Index widget.
 * It demonstrates the basic functionality and provides a button to
 * step through various API functions for testing purposes. It also
 * includes a checkbox to toggle the "omit" feature.
 */
void
test_index(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bxx, *gl, *id, *bt, *tb, *ck;
   Elm_Object_Item *glit;
   int i, j;
   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("index", "Index");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   bxx = elm_box_add(win);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(tb);

   gl = elm_genlist_add(win);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, gl, 0, 0, 1, 1);
   evas_object_show(gl);

   api->dt.id = id = elm_index_add(win);
   evas_object_size_hint_weight_set(id, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(id, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_index_autohide_disabled_set(id, EINA_FALSE);
   elm_table_pack(tb, id, 0, 0, 1, 1);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Next API function");
   evas_object_smart_callback_add(bt, "clicked", _api_bt_clicked, (void *) api);
   elm_box_pack_end(bxx, bt);
   elm_object_disabled_set(bt, api->state == API_STATE_LAST);
   evas_object_show(bt);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Omit mode : ");
   elm_object_style_set(ck, "toggle");
   elm_box_pack_end(bxx, ck);
   evas_object_smart_callback_add(ck, "changed", _omit_check_changed_cb, id);
   evas_object_show(ck);

   elm_box_pack_end(bxx, tb);

   evas_object_show(id);

   itci.item_style     = "default";
   itci.func.text_get = _gli_text_get;
   itci.func.content_get  = NULL;
   itci.func.state_get = NULL;
   itci.func.del       = NULL;

   j = 0;
   for (i = 0; i < 100; i++)
     {
        glit = elm_genlist_item_append(gl, &itci,
                                       (void *)(uintptr_t)j/* item data */,
                                       NULL/* parent */,
                                       ELM_GENLIST_ITEM_NONE,
                                       NULL/* func */, NULL/* func data */);
        if (!(j & 0xf))
          {
             char buf[32];

             snprintf(buf, sizeof(buf), "%c", 'A' + ((j >> 4) & 0xf));
             elm_index_item_append(id, buf, _id_cb, glit);
             if (*buf == 'G')  /* Just init dt->item later used in API test */
               api->dt.item = glit;
          }
        j += 2;
     }
   evas_object_smart_callback_add(id, "delay,changed", _index_delay_changed_cb, NULL);
   evas_object_smart_callback_add(id, "changed", _index_changed_cb, NULL);
   evas_object_smart_callback_add(id, "selected", _index_selected_cb, NULL);
   elm_index_level_go(id, 0);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           270 * elm_config_scale_get());
   evas_object_show(win);
}

/***********/

/**
 * @brief Structure to hold UI elements for the second index test.
 *
 * This structure aggregates pointers to the main widgets used in
 * the test_index2 function for easier access in callbacks.
 */
typedef struct _Test_Index2_Elements
{
   Evas_Object *entry; /**< The entry widget for new item labels */
   Evas_Object *lst;   /**< The list widget */
   Evas_Object *id;    /**< The index widget */
} Test_Index2_Elements;

/**
 * @brief Deletion callback for the second index test.
 * @param data The Test_Index2_Elements struct.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * Frees the memory allocated for the GUI elements structure when
 * the window is closed.
 */
static void
_test_index2_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   /* FIXME it won't be called if elm_test main window is closed */
   free(data);
}

/**
 * @brief Comparison function for list items.
 * @param data1 First list item (Elm_Object_Item *).
 * @param data2 Second list item (Elm_Object_Item *).
 * @return An integer less than, equal to, or greater than zero if the
 *         first item's label is found, respectively, to be less than, to
 *         match, or be greater than the second.
 *
 * Used for sorted insertion into the list widget. It performs a
 * case-insensitive comparison of the item labels.
 */
static int
_test_index2_cmp(const void *data1, const void *data2)
{
   const char *label1, *label2;
   const Elm_Object_Item *li_it1 = data1;
   const Elm_Object_Item *li_it2 = data2;

   label1 = elm_object_item_text_get(li_it1);
   label2 = elm_object_item_text_get(li_it2);

   return strcasecmp(label1, label2);
}

/**
 * @brief Comparison function for index items.
 * @param data1 First index item (Elm_Object_Item *).
 * @param data2 Second index item (Elm_Object_Item *).
 * @return An integer less than, equal to, or greater than zero if the
 *         first item's letter is found, respectively, to be less than, to
 *         match, or be greater than the second.
 *
 * Used for sorted insertion into the index widget. It performs a
 * case-insensitive comparison of the item letters.
 */
static int
_test_index2_icmp(const void *data1, const void *data2)
{
   const char *label1, *label2;
   const Elm_Object_Item *index_it1 = data1;
   const Elm_Object_Item *index_it2 = data2;

   label1 = elm_index_item_letter_get(index_it1);
   label2 = elm_index_item_letter_get(index_it2);

   return strcasecmp(label1, label2);
}

/**
 * @brief Adds a new item to the sorted list and index.
 * @param data The Test_Index2_Elements struct.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function is called to add a new item. It takes the text from the
 * entry widget, inserts it into the list in sorted order, and adds a
 * corresponding item to the index, also in sorted order.
 */
static void
_test_index2_it_add(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Test_Index2_Elements *gui = data;
   Elm_Object_Item *list_it;
   const char *label;
   char letter[2];

   label = elm_object_text_get(gui->entry);
   snprintf(letter, sizeof(letter), "%c", label[0]);
   list_it = elm_list_item_sorted_insert(gui->lst, label, NULL, NULL, NULL,
                                         NULL, _test_index2_cmp);
   elm_index_item_sorted_insert(gui->id, letter, NULL, list_it, _test_index2_icmp,
                                _test_index2_cmp);
   elm_index_level_go(gui->id, 0);
   elm_list_go(gui->lst);
   /* FIXME it's not showing the recently added item */
   elm_list_item_show(list_it);
}

/**
 * @brief Clears all items from the list and index.
 * @param data The Test_Index2_Elements struct.
 * @param obj Not used.
 * @param event_info Not used.
 */
static void
_test_index2_clear(void *data, Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   Test_Index2_Elements *gui = data;

   elm_list_clear(gui->lst);
   elm_index_item_clear(gui->id);
}

/**
 * @brief Deletes a selected item from the list and index.
 * @param data The Test_Index2_Elements struct.
 * @param obj Not used.
 * @param event_info The list item to be deleted.
 *
 * This function handles the deletion of an item from the list. It also
 * finds and removes the corresponding item from the index, or updates
 * it if another item shares the same index letter.
 */
static void
_test_index2_it_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Test_Index2_Elements *gui = data;
   const char *label, *label_next;
   Elm_Object_Item *list_it = event_info, *list_it_next, *iit;

   list_it_next = elm_list_item_next(list_it);
   if (!list_it_next)
     {
        iit = elm_index_item_find(gui->id, list_it);
        if (iit) elm_object_item_del(iit);
        elm_object_item_del(list_it);
        return;
     }

   label = elm_object_item_text_get(list_it);
   label_next = elm_object_item_text_get(list_it_next);

   iit = elm_index_item_find(gui->id, list_it);
   if (iit)
     {
        if (label[0] == label_next[0])
          elm_object_item_data_set(iit, list_it_next);
        else
          elm_object_item_del(iit);
     }

   elm_object_item_del(list_it);
}

/**
 * @brief Callback for index changes in the second test.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The selected index item.
 *
 * When the index selection changes, this function brings the corresponding
 * list item into the visible area of the list.
 */
static void
_test_index2_id_changed(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   elm_list_item_show(elm_object_item_data_get(event_info));
}

/**
 * @brief The main function for the second index test (sorted list).
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates the use of an index with a sorted list.
 * It provides UI to add, delete, and clear items, showing how the
 * index and list can be kept in sync. This is useful for applications
 * like contact lists.
 */
void
test_index2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *box, *bt;
   Test_Index2_Elements *gui;

   gui = malloc(sizeof(*gui));

   win = elm_win_util_standard_add("sorted-index-list", "Sorted Index and List");
   evas_object_smart_callback_add(win, "delete,request", _test_index2_del, gui);
   elm_win_autodel_set(win, EINA_TRUE);

   box = elm_box_add(win);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, box);
   evas_object_show(box);

   gui->id = elm_index_add(win);
   evas_object_size_hint_weight_set(gui->id, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, gui->id);
   evas_object_smart_callback_add(gui->id, "delay,changed",
                                  _test_index2_id_changed, NULL);
   evas_object_show(gui->id);

   gui->entry = elm_entry_add(win);
   elm_entry_scrollable_set(gui->entry, EINA_TRUE);
   elm_object_text_set(gui->entry, "Label");
   elm_entry_single_line_set(gui->entry, EINA_TRUE);
   evas_object_size_hint_weight_set(gui->entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_fill_set(gui->entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(gui->entry, "activated", _test_index2_it_add, gui);
   elm_box_pack_end(box, gui->entry);
   evas_object_show(gui->entry);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Add");
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_fill_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, bt);
   evas_object_smart_callback_add(bt, "clicked", _test_index2_it_add, gui);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Clear");
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_fill_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, bt);
   evas_object_smart_callback_add(bt, "clicked", _test_index2_clear, gui);
   evas_object_show(bt);

   gui->lst = elm_list_add(win);
   elm_box_pack_end(box, gui->lst);
   evas_object_size_hint_weight_set(gui->lst, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(gui->lst, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(gui->lst, "selected", _test_index2_it_del,
                                  gui);
   elm_list_go(gui->lst);
   evas_object_show(gui->lst);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}

/***** Index 3 Mode ******/

/**
 * @brief Callback to change the priority of index items.
 * @param data The index widget.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This function toggles the standard priority of the index between 0 and 1.
 * When the priority is changed, the index will display items of that
 * priority level. This is useful for multilingual applications where the user
 * might want to switch between different character sets (e.g., English and Korean).
 */
static void
_index_priority_change_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *index = data;
   int priority;

   priority = elm_index_standard_priority_get(index);

   if (priority == 0)
     elm_index_standard_priority_set(index, 1);
   else
     elm_index_standard_priority_set(index, 0);

   printf("Priority changed to : %d\n", elm_index_standard_priority_get(index));
}

/**
 * @brief The main function for the third index test (item priority).
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test demonstrates the item priority feature of the index widget.
 * It creates an index with items from different character sets (Korean and
 * English) assigned to different priorities. A button allows toggling
 * which priority level is displayed.
 */
void
test_index3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *index, *bt;
   Elm_Object_Item *it;
   int i, j, len;
   char *str;
   char buf[PATH_MAX] = {0, };

   win = elm_win_util_standard_add("Index-priority", "Index priority for multilingual");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   index = elm_index_add(win);
   evas_object_size_hint_weight_set(index, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(index, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_index_autohide_disabled_set(index, EINA_TRUE);
   elm_index_omit_enabled_set(index, EINA_TRUE);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Priority Change");
   evas_object_smart_callback_add(bt, "clicked", _index_priority_change_cb, index);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   elm_box_pack_end(bx, index);

   evas_object_show(index);

   //1. Special character & Numbers
   elm_index_item_append(index, "#", NULL, NULL);

   //2. Local language
   str = "ㄱㄴㄷㄹㅁㅂㅅㅇㅈㅊㅍㅎ";
   len = strlen(str);

   i = 0;
   while (i < len)
     {
        j = i;
        eina_unicode_utf8_next_get(str, &i);
        snprintf(buf, i - j + 1, "%s", str + j);
        buf[i - j + 1] = 0;

        it = elm_index_item_append(index, buf, NULL, NULL);
        elm_index_item_priority_set(it, 0);
     }

   //3. English
   str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   len = strlen(str);

   i = 0;
   while (i < len)
     {
        j = i;
        eina_unicode_utf8_next_get(str, &i);
        snprintf(buf, i - j + 1, "%s", str + j);
        buf[i - j + 1] = 0;

        it = elm_index_item_append(index, buf, NULL, NULL);
        elm_index_item_priority_set(it, 1);
     }

   elm_index_level_go(index, 0);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           300 * elm_config_scale_get());
   evas_object_show(win);
}

/***** Index Horizontal Mode ******/

/**
 * @brief Callback for index changes in the horizontal test.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info The selected index item.
 *
 * When the index selection changes, this function brings the corresponding
 * horizontal list item into view.
 */
static void
_index_list_changed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                       void *event_info)
{
   elm_list_item_bring_in(elm_object_item_data_get(event_info));
}

/**
 * @brief The main function for the horizontal index test.
 * @param data Not used.
 * @param obj Not used.
 * @param event_info Not used.
 *
 * This test creates a window with a horizontal list and a corresponding
 * horizontal index widget, demonstrating the horizontal mode for both.
 */
void
test_index_horizontal(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   Evas_Object *win, *list, *id, *tb;
   Elm_Object_Item *lit;
   int i;
   char buf[30];

   api_data *api = calloc(1, sizeof(api_data));

   win = elm_win_util_standard_add("index-horizontal", "Index Horizontal");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_event_callback_add(win, EVAS_CALLBACK_FREE, _cleanup_cb, api);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb);
   evas_object_show(tb);

   list = elm_list_add(win);
   elm_list_horizontal_set(list, EINA_TRUE);
   evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, list, 0, 0, 1, 1);
   evas_object_show(list);

   api->dt.id = id = elm_index_add(win);
   elm_index_horizontal_set(id, EINA_TRUE);
   evas_object_size_hint_weight_set(id, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(id, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, id, 0, 0, 1, 1);
   evas_object_show(id);

   for (i = 1; i < 15; i++)
     {
        sprintf(buf, "Item #%d", i);
        lit = elm_list_item_append(list, buf, NULL, NULL, NULL, NULL);
        sprintf(buf, "%d", i);
        elm_index_item_append(id, buf, _id_cb, lit);
     }
   evas_object_smart_callback_add(id, "changed", _index_list_changed_cb, NULL);
   elm_index_level_go(id, 0);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           320 * elm_config_scale_get());
   evas_object_show(win);
}
