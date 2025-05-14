#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

// Stack API is still beta (and EO only)
#define EFL_UI_WIN_BETA
#include <Efl_Ui.h>
#include <Elementary.h>

static int level = 0; /**< Current window stack level. */
static Evas_Object *popto_win = NULL; /**< Window to pop to when "Pop to level 3" is clicked. */

/**
 * @brief Callback function for the "Pop to level 3" button.
 *
 * This function pops the window stack to the window stored in popto_win.
 *
 * @param data User data, not used.
 * @param obj The button object that was clicked.
 * @param event_info Event specific information, not used.
 */
static void _bt_popto(void *data, Evas_Object *obj, void *event_info);
/**
 * @brief Callback function for the "Push" button.
 *
 * This function creates and pushes a new window onto the stack.
 *
 * @param data The parent window object.
 * @param obj The button object that was clicked.
 * @param event_info Event specific information, not used.
 */
static void _bt_pressed(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Creates a new window.
 *
 * This function creates a new window with a title, a label, and buttons.
 * The window type (NAVIFRAME_BASIC or DIALOG_BASIC) depends on the current stack level.
 *
 * @param parent The parent Evas_Object for the new window.
 * @param title The title for the new window.
 * @return The newly created window object.
 */
static Evas_Object *
_win_new(Evas_Object* parent, const char *title)
{
   Evas_Object *bg, *bx, *bt, *lb, *win;

   if (level >= 3)
     win = efl_add(EFL_UI_WIN_CLASS, parent,
                   efl_ui_win_name_set(efl_added, "window-stack"),
                   efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_NAVIFRAME_BASIC),
                   efl_text_set(efl_added, title),
                   efl_ui_win_autodel_set(efl_added, EINA_TRUE));
   else
     win = efl_add(EFL_UI_WIN_CLASS, parent,
                   efl_ui_win_name_set(efl_added, "window-stack"),
                   efl_ui_win_type_set(efl_added, EFL_UI_WIN_TYPE_DIALOG_BASIC),
                   efl_text_set(efl_added, title),
                   efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   if (level == 3) popto_win = win;

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Press below to push another window on the stack");
   evas_object_size_hint_weight_set(lb, 1.0, 1.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   if (level > 7)
     {
        bt = elm_button_add(win);
        elm_object_text_set(bt, "Pop to level 3");
        evas_object_smart_callback_add(bt, "clicked", _bt_popto, NULL);
        evas_object_size_hint_fill_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
     }

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Push");
   evas_object_smart_callback_add(bt, "clicked", _bt_pressed, parent);
   evas_object_size_hint_fill_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_resize(win, 280, 400);
   return win;
}

static void
_bt_popto(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   efl_ui_win_stack_pop_to(popto_win);
}

static void
_bt_pressed(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = (Evas_Object*) data;
   char buf[100];

   level++;
   snprintf(buf, sizeof(buf), "Level %i", level);
   win = _win_new(win, buf);
   efl_ui_win_stack_master_id_set(win, efl_ui_win_stack_id_get(data));
}

/**
 * @brief Callback function for window deletion.
 *
 * Resets the global level and popto_win variables when the base window is deleted.
 *
 * @param data User data, not used.
 * @param e The Evas canvas, not used.
 * @param o The Evas_Object being deleted, not used.
 * @param info Event specific information, not used.
 */
static void
_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *info EINA_UNUSED)
{
   level = 0;
   popto_win = NULL;
}

/**
 * @brief Main function for the window stack test.
 *
 * This function creates the initial window and sets up the UI for testing
 * the window stacking functionality.
 *
 * @param data User data, not used.
 * @param obj The object that triggered this test, not used.
 * @param event_info Event specific information, not used.
 */
void
test_win_stack(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *bg, *bx, *bt, *lb, *win;

   win = elm_win_add(NULL, "window-stack", ELM_WIN_BASIC);
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _del, NULL);
   efl_ui_win_stack_base_set(win, EINA_TRUE);
   elm_win_title_set(win, "Window Stack");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Press below to push another window on the stack");
   evas_object_size_hint_weight_set(lb, 1.0, 1.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lb);
   evas_object_show(lb);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Push");
   evas_object_smart_callback_add(bt, "clicked", _bt_pressed, win);
   evas_object_size_hint_fill_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}
