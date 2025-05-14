#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Gets the top-level window for a given Evas object.
 *
 * This function traverses up the widget hierarchy from the given object
 * to find the root window object.
 *
 * @param obj The Evas object whose parent window is to be found.
 * @return The top-level Evas_Object (window) or the object itself if it has no parent widget.
 */
static Evas_Object *
_parent_win_get(Evas_Object *obj)
{
   Evas_Object *parent = obj;

   while ((parent = elm_object_parent_widget_get(obj)) != NULL)
     obj = parent;

   return obj;
}

/**
 * @brief Callback function to close a window.
 *
 * This function is typically used as a callback for a "clicked" event
 * on a close button. It deletes the window object passed in the data parameter.
 *
 * @param data A pointer to the Evas_Object (window) to be closed.
 * @param obj The Evas_Object that triggered the callback (unused).
 * @param event_info Additional event information (unused).
 */
static void
_close_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;

   evas_object_del(win);
}

/**
 * @brief Creates and displays a modal window.
 *
 * This function demonstrates the creation of a modal window in Elementary.
 * The modal window blocks input to other windows in the application until
 * it is closed. It includes a label and buttons to open another modal
 * window or to close the current one.
 *
 * @param data Custom data pointer (unused).
 * @param obj The Evas_Object that triggered this function, typically a button
 *            in a parent window. Used to get the parent window.
 * @param event_info Additional event information (unused).
 */
void
test_win_modal(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *parent_win, *win, *bg, *txt, *bx, *bt;

   if (!obj)
     {
        fprintf(stderr, "The modal win test can only be run by clicking the button for it in the main elementary_test window.\n");
        return;
     }
   parent_win = _parent_win_get(obj);

   win = elm_win_add(parent_win, "modal", ELM_WIN_BASIC);
   elm_win_title_set(win, "Window Modal");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   elm_win_resize_object_add(win, bg);
   evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bg);

   bx = elm_box_add(win);
   elm_win_resize_object_add(win, bx);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bx);

   txt = elm_label_add(win);
   elm_object_text_set(txt, "This is a modal window.<br>"
                       "It will block all inputs on all the other opened windows.");
   evas_object_size_hint_align_set(txt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(txt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_label_line_wrap_set(txt, ELM_WRAP_WORD);
   elm_box_pack_end(bx, txt);
   evas_object_show(txt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Open another modal window");
   evas_object_size_hint_align_set(bt, 0.5, 0.5);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_smart_callback_add(bt, "clicked", test_win_modal, NULL);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_size_hint_align_set(bt, 0.5, 0.5);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0);
   evas_object_smart_callback_add(bt, "clicked", _close_cb, win);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   /* make the window modal */
   elm_win_modal_set(win, EINA_TRUE);

   evas_object_resize(win, 250 * elm_config_scale_get(),
                           150 * elm_config_scale_get());
   evas_object_show(win);
}
