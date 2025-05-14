#include "elementary_config.h"
#include <Elementary.h>

/**
 * @brief Callback for the "dismissed" smart event of the hover.
 *
 * This function is called when the hover object is dismissed (hidden).
 * It simply prints a message to standard output.
 *
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_dismissed_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   printf("hover dismissed callback is called!\n");
}

/**
 * @brief Callback to show a hover object.
 *
 * This function is typically connected to a button's "clicked" event.
 * It shows the hover Evas_Object passed in the @p data parameter.
 *
 * @param data The hover object (Evas_Object *) to be shown.
 * @param obj The button object that was clicked (unused).
 * @param event_info Event-specific information (unused).
 */
static void
my_hover_bt(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *hv = data;

   evas_object_show(hv);
}

/**
 * @brief Callback to dismiss a hover object.
 *
 * This function is connected to a button's "clicked" event inside the hover.
 * It dismisses the hover Evas_Object passed in the @p data parameter.
 *
 * @param data The hover object (Evas_Object *) to be dismissed.
 * @param obj The button object that was clicked (unused).
 * @param event_info Event-specific information (unused).
 */
static void
_dismiss_hover(void *data, Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   Evas_Object *hv = data;

   elm_hover_dismiss(hv);
}

/**
 * @brief Test function for a standard hover widget.
 *
 * This test creates a window with a button. Clicking the button shows a
 * hover widget. The hover has content set for its "top", "bottom", "left",
 * "right", and "middle" parts. One of the buttons inside the hover content
 * can dismiss it.
 *
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that initiated the test (unused).
 * @param event_info Event-specific information (unused).
 */
void
test_hover(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bt, *hv, *ic;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("hover", "Hover");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   hv = elm_hover_add(win);
   evas_object_smart_callback_add(hv, "dismissed", _dismissed_cb, NULL);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   evas_object_smart_callback_add(bt, "clicked", my_hover_bt, hv);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   elm_hover_parent_set(hv, win);
   elm_hover_target_set(hv, bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Popup");
   elm_object_part_content_set(hv, "middle", bt);
   evas_object_show(bt);

   bx = elm_box_add(win);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_box_pack_end(bx, ic);
   evas_object_show(ic);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 1");
   evas_object_smart_callback_add(bt, "clicked", _dismiss_hover, hv);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 2");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 3");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_show(bx);
   elm_object_part_content_set(hv, "top", bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Bottom");
   elm_object_part_content_set(hv, "bottom", bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Left");
   elm_object_part_content_set(hv, "left", bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Right");
   elm_object_part_content_set(hv, "right", bt);
   evas_object_show(bt);

   evas_object_resize(win, 440 * elm_config_scale_get(),
                           440 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test function for a "popout" style hover widget.
 *
 * This test is similar to test_hover(), but it sets the hover's style to
 * "popout". This style can provide a different visual appearance for the hover.
 *
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that initiated the test (unused).
 * @param event_info Event-specific information (unused).
 */
void
test_hover2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bt, *hv, *ic;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("hover2", "Hover 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   hv = elm_hover_add(win);
   elm_object_style_set(hv, "popout");

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   evas_object_smart_callback_add(bt, "clicked", my_hover_bt, hv);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   elm_hover_parent_set(hv, win);
   elm_hover_target_set(hv, bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Popup");
   elm_object_part_content_set(hv, "middle", bt);
   evas_object_show(bt);

   bx = elm_box_add(win);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_box_pack_end(bx, ic);
   evas_object_show(ic);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 1");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 2");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   bt = elm_button_add(win);
   elm_object_text_set(bt, "Top 3");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_show(bx);
   elm_object_part_content_set(hv, "top", bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Bot");
   elm_object_part_content_set(hv, "bottom", bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Left");
   elm_object_part_content_set(hv, "left", bt);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Right");
   elm_object_part_content_set(hv, "right", bt);
   evas_object_show(bt);

   evas_object_resize(win, 440 * elm_config_scale_get(),
                           440 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback to show a hover at a specific mouse position.
 *
 * This function is called on a EVAS_CALLBACK_MOUSE_DOWN event. It retrieves a
 * "fake" Evas_Object from the object that received the event. This fake object
 * is used as the hover's target, allowing the hover to appear anywhere. The
 * function moves the fake object to the mouse down coordinates and then shows
 * the hover, which is passed as the @p data parameter.
 *
 * @param data The hover object (Evas_Object *) to show.
 * @param e The Evas canvas (unused).
 * @param obj The object that received the mouse down event.
 * @param event_info Pointer to an Evas_Event_Mouse_Down struct.
 */
static void
_hover_show_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj,
               void *event_info)
{
   Evas_Object *fake_obj = evas_object_data_get(obj, "fake_obj");
   if (!fake_obj) return;

   Evas_Event_Mouse_Down *ev = event_info;
   printf("position x: %d, y: %d \n", ev->canvas.x, ev->canvas.y);

   evas_object_move(fake_obj, ev->canvas.x, ev->canvas.y);
   evas_object_show(data);
}

/**
 * @brief Test function for a hover that acts like a context menu.
 *
 * This test creates a window with a transparent rectangle covering the entire
 * area. When the user clicks on this rectangle, a hover appears at the
 * cursor's position. This is achieved by using a "fake" object as the hover's
 * target, and moving this fake object to the click location before showing
 * the hover. The hover contains several widgets, including a close button.
 *
 * This demonstrates how a hover can be used to implement right-click menus
 * or other context-sensitive popups.
 *
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that initiated the test (unused).
 * @param event_info Event-specific information (unused).
 */
/*
 * hover acts like elm_menu but it has all the hover features such as:
 * 1. positioning: left, top-left, top, top-right, right, bottom-right, bottom,
 *                 bottom-left, middle
 * 2. content: one can set any object object as hover content
 */
void
test_hover3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
            void *event_info EINA_UNUSED)
{
   Evas_Object *win, *fake_obj, *bx, *bt, *hv, *ic, *rect;
   char buf[PATH_MAX];

   win = elm_win_util_standard_add("hover3", "Hover 3");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   rect = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, rect);
   evas_object_color_set(rect, 0, 0, 0, 0);
   evas_object_show(rect);

   // fake object to move hover object as we wish
   fake_obj = elm_box_add(win);
   evas_object_data_set(rect, "fake_obj", fake_obj);

   hv = elm_hover_add(win);
   elm_hover_parent_set(hv, win);
   elm_hover_target_set(hv, fake_obj);

   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN,
                                  _hover_show_cb, hv);

   bx = elm_box_add(win);
   elm_object_part_content_set(hv, "smart", bx);
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Button");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_smart_callback_add(bt, "clicked", _dismiss_hover, hv);

   ic = elm_icon_add(win);
   snprintf(buf, sizeof(buf), "%s/images/logo_small.png",
            elm_app_data_dir_get());
   elm_image_file_set(ic, buf, NULL);
   elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
   elm_box_pack_end(bx, ic);
   evas_object_show(ic);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);
   evas_object_smart_callback_add(bt, "clicked", _dismiss_hover, hv);

   evas_object_resize(win, 440 * elm_config_scale_get(),
                           440 * elm_config_scale_get());
   evas_object_show(win);
}
