#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Callback function for when an item is added to the tags widget.
 *
 * This function is called when the EFL_UI_TAGS_EVENT_ITEM_ADDED event is
 * triggered. It adds the new item's string to a persistent Eina_Array.
 *
 * @param data An Eina_Array to which the new item's string will be added.
 * @param ev The Efl_Event structure containing event information. The
 *        event info (`ev->info`) is the string of the added item.
 */
static void
_item_added_cb(void *data, const Efl_Event *ev)
{
   Eina_Array *array = data;
   const char *str = ev->info;

   printf("item added %s\n", str);
   eina_array_push(array, str);
}

/**
 * @brief Callback function for when an item is deleted from the tags widget.
 *
 * This function is called when the EFL_UI_TAGS_EVENT_ITEM_DELETED event is
 * triggered. It synchronizes an Eina_Array with the tags widget's current
 * items by clearing the array and refilling it.
 *
 * @param data An Eina_Array which is synchronized with the tags widget's items.
 * @param ev The Efl_Event structure. The event info (`ev->info`) is the
 *        string of the deleted item.
 */
static void
_item_deleted_cb(void *data, const Efl_Event *ev)
{
   Eina_Array_Iterator iterator;
   const char *item;
   unsigned int i;
   Eina_Array *array = data;
   const char *str = ev->info;

   printf("item deleted %s\n", str);
   eina_array_clean(array);

   const Eina_Array *tags_array = efl_ui_tags_items_get(ev->object);
   EINA_ARRAY_ITER_NEXT(tags_array, i, item, iterator)
     {
        eina_array_push(array, item);
        printf("item #%u: %s\n", i, item);
     }
}

/**
 * @brief Callback function for a button click to toggle the tags widget's mode.
 *
 * This function toggles the expanded state of the tags widget between
 * expanded and collapsed.
 *
 * @param data The tags widget object (Eo *tags).
 * @param ev The Efl_Event structure for the click event (unused).
 */
static void
_clicked(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Eo *tags = data;

   if (efl_ui_tags_expanded_get(tags))
     efl_ui_tags_expanded_set(tags, EINA_FALSE);
   else
     efl_ui_tags_expanded_set(tags, EINA_TRUE);
}


/**
 * @brief Main test function for the Efl.Ui.Tags widget.
 *
 * This function sets up a window containing an Efl.Ui.Tags widget to
 * demonstrate its functionality. It initializes the widget with a predefined
 * set of tags and sets up callbacks for adding, deleting, and toggling
 * the expanded view.
 *
 * The Eina_Array `array` is initialized with string pointers. For example:
 * @code
 * Eina_Array *array;
 * array = eina_array_new(10);
 * eina_array_push(array, "one");
 * eina_array_push(array, "two");
 * // ...and so on.
 * @endcode
 * This array is then passed to the tags widget and also to the callbacks
 * to keep track of the items.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_ui_tags(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *win, *layout, *tags;

   const char* strs[] = {
        "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
   };

   Eina_Array *array;
   unsigned int i;
   char buf[PATH_MAX];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                                  efl_text_set(efl_added, "Efl.Ui.Tags"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   snprintf(buf, sizeof(buf), "%s/objects/multibuttonentry.edj", elm_app_data_dir_get());
   layout = efl_add(EFL_UI_LAYOUT_CLASS, win,
                    elm_layout_file_set(efl_added, buf, "multibuttonentry_test"),
                    efl_content_set(win, efl_added));

   array = eina_array_new(10);
   for (i = 0; i < 9; i++)
       eina_array_push(array, strs[i]);

   tags = efl_add(EFL_UI_TAGS_CLASS, layout,
                  efl_ui_tags_items_set(efl_added, array),
                  efl_event_callback_add(efl_added, EFL_UI_TAGS_EVENT_ITEM_ADDED, _item_added_cb, array),
                  efl_event_callback_add(efl_added, EFL_UI_TAGS_EVENT_ITEM_DELETED, _item_deleted_cb, array),
                  efl_text_set(efl_added, "To :"),
                  efl_ui_format_string_set(efl_added, "+ %d items", EFL_UI_FORMAT_STRING_TYPE_SIMPLE),
                  elm_object_part_content_set(layout, "multibuttonentry", efl_added));

   efl_add(EFL_UI_BUTTON_CLASS, layout,
           efl_text_set(efl_added, "Change mode"),
           efl_event_callback_add(efl_added, EFL_INPUT_EVENT_CLICKED, _clicked, tags),
           elm_object_part_content_set(layout, "box", efl_added));

   efl_gfx_entity_size_set(win, EINA_SIZE2D(320, 480));
}
