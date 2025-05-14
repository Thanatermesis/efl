#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function for the "press" event on the panes widget.
 *
 * This function is called when the user presses the bar of the panes widget.
 *
 * @param data Unused user data pointer.
 * @param obj Unused Evas_Object pointer to the panes widget.
 * @param event_info Unused event information.
 */
static void
_press(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("press\n");
}

/**
 * @brief Callback function for the "unpress" event on the panes widget.
 *
 * This function is called when the user releases the bar of the panes widget.
 * It prints the size of the left content pane.
 *
 * @param data Unused user data pointer.
 * @param obj The Evas_Object pointer to the panes widget.
 * @param event_info Unused event information.
 */
static void
_unpress(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   printf("unpress, size : %f\n", elm_panes_content_left_size_get(obj));
}

/**
 * @brief Callback function for the "clicked" event on the panes widget.
 *
 * This function is called when the user clicks the bar of the panes widget.
 *
 * @param data Unused user data pointer.
 * @param obj Unused Evas_Object pointer to the panes widget.
 * @param event_info Unused event information.
 */
static void
_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("clicked\n");
}

/**
 * @brief Callback for a double-click event on the panes widget's bar.
 *
 * This function toggles the size of the left pane. If the pane is visible,
 * it collapses it to 0.0 and stores its original size. If it's collapsed,
 * it restores it to its previous size. This provides a hide/show functionality
 * on double-click.
 *
 * @param data A pointer to a double, used to store the pane's size.
 * @param obj The panes widget that was double-clicked.
 * @param event_info Unused event information.
 */
static void
_clicked_double(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   double *size = data;
   double tmp_size = 0.0;

   tmp_size = elm_panes_content_left_size_get(obj);
   printf("clicked double\n");
   if (tmp_size > 0)
     {
        elm_panes_content_left_size_set(obj, 0.0);
        *size = tmp_size;
     }
   else
     elm_panes_content_left_size_set(obj, *size);
}

/**
 * @brief Creates and sets up the main test window with panes widgets.
 *
 * This function initializes a window and a vertical panes widget. The left
 * pane contains a button, and the right pane contains a horizontal panes
 * widget. This nested panes widget then contains two more buttons in its top
 * and bottom panes. Callbacks for various events on the panes' bars are set up.
 *
 * @param style The style to be applied to the panes widgets.
 *              Example values: "default", "flush", "left-fold".
 */
static void
_test_panes(const char *style)
{
   Evas_Object *win, *bg, *panes, *panes_h, *bt;
   static double vbar_size = 0.0;
   static double hbar_size = 0.0;

   win = elm_win_add(NULL, "panes", ELM_WIN_BASIC);
   elm_win_title_set(win, "Panes");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   evas_object_show(bg);

   panes = elm_panes_add(win);
   elm_object_style_set(panes, style);
   evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_panes_content_left_min_size_set(panes, 100);
   elm_panes_content_left_size_set(panes, 0.7);
   elm_win_resize_object_add(win, panes);
   evas_object_show(panes);

   evas_object_smart_callback_add(panes, "clicked", _clicked, panes);
   evas_object_smart_callback_add(panes, "clicked,double", _clicked_double, &vbar_size);

   evas_object_smart_callback_add(panes, "press", _press, panes);
   evas_object_smart_callback_add(panes, "unpress", _unpress, panes);

   // add left button
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Left - min size 100 - size 70%");
   evas_object_show(bt);
   elm_object_part_content_set(panes, "left", bt);

   // add panes
   panes_h = elm_panes_add(win);
   elm_object_style_set(panes_h, style);
   elm_panes_horizontal_set(panes_h, EINA_TRUE);
   elm_panes_content_right_min_size_set(panes_h, 100);
   elm_panes_content_right_size_set(panes_h, 0.3);
   evas_object_show(panes_h);

   evas_object_smart_callback_add(panes_h, "clicked", _clicked, panes_h);
   evas_object_smart_callback_add(panes_h, "clicked,double", _clicked_double, &hbar_size);

   evas_object_smart_callback_add(panes_h, "press", _press, panes_h);
   evas_object_smart_callback_add(panes_h, "unpress", _unpress, panes_h);
   elm_object_part_content_set(panes, "right", panes_h);

   // add up button
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Up");
   evas_object_show(bt);
   elm_object_part_content_set(panes_h, "top", bt);

   // add down button
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Down - min size 100 size 30%");
   evas_object_show(bt);
   elm_object_part_content_set(panes_h, "bottom", bt);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function to create a panes widget with the "default" style.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("default");
}

/**
 * @brief Test function to create a panes widget with the "flush" style.
 *
 * The "flush" style typically renders the panes without a visible border
 * or handle, making the separation appear seamless.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_flush(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("flush");
}

/**
 * @brief Test function to create a panes widget with the "left-fold" style.
 *
 * This style might provide a specific visual effect for folding or collapsing
 * the left pane.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_left_fold(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("left-fold");
}

/**
 * @brief Test function to create a panes widget with the "right-fold" style.
 *
 * This style might provide a specific visual effect for folding or collapsing
 * the right pane.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_right_fold(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("right-fold");
}

/**
 * @brief Test function to create a panes widget with the "up-fold" style.
 *
 * This style might provide a specific visual effect for folding or collapsing
 * the top pane in a horizontal panes widget.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_up_fold(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("up-fold");
}

/**
 * @brief Test function to create a panes widget with the "down-fold" style.
 *
 * This style might provide a specific visual effect for folding or collapsing
 * the bottom pane in a horizontal panes widget.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_panes_down_fold(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   _test_panes("down-fold");
}



