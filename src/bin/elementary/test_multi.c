#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#define IND_NUM 20

static Evas_Object *indicator[IND_NUM];

/**
 * @brief Callback for mouse down events.
 *
 * This function handles mouse down events on the main window's event
 * rectangle. It is used to demonstrate single-touch or mouse input.
 * It shows and positions the first indicator at the event coordinates.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information.
 */
static void
_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button != 1) return;
   printf("MOUSE: down @ %4i %4i\n", ev->canvas.x, ev->canvas.y);
   evas_object_move(indicator[0], ev->canvas.x, ev->canvas.y);
   evas_object_resize(indicator[0], 1, 1);
   evas_object_show(indicator[0]);
}

/**
 * @brief Callback for mouse up events.
 *
 * This function handles mouse up events, hiding the primary indicator
 * used for single-touch or mouse input.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information.
 */
static void
_mouse_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   if (ev->button != 1) return;
   printf("MOUSE: up   @ %4i %4i\n", ev->canvas.x, ev->canvas.y);
   evas_object_hide(indicator[0]);
}

/**
 * @brief Callback for mouse move events.
 *
 * This function handles mouse move events, repositioning the primary
 * indicator to follow the cursor's movement.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information.
 */
static void
_mouse_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   printf("MOUSE: move @ %4i %4i\n", ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_move(indicator[0], ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_resize(indicator[0], 1, 1);
}

/**
 * @brief Callback for multi-touch down events.
 *
 * This function handles the start of a new touch point in a multi-touch
 * sequence. It uses the device ID from the event to select a unique
 * indicator from the `indicator` array, which it then shows and positions
 * at the touch coordinates. The `indicator` array is a static global array
 * of Evas_Object pointers, with `IND_NUM` elements.
 * For example, `indicator[0]` for device 0, `indicator[1]` for device 1, etc.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information, containing details
 *                   like coordinates and device ID.
 */
static void
_multi_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Multi_Down *ev = event_info;
   printf("MULTI: down @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_move(indicator[ev->device], ev->canvas.x, ev->canvas.y);
   evas_object_resize(indicator[ev->device], 1, 1);
   evas_object_show(indicator[ev->device]);
}

/**
 * @brief Callback for multi-touch up events.
 *
 * This function handles the end of a touch point (finger lifted). It uses
 * the device ID to identify the correct indicator and hides it.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information, containing the device ID.
 */
static void
_multi_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Multi_Up *ev = event_info;
   printf("MULTI: up    @ %4i %4i | dev: %i\n", ev->canvas.x, ev->canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_hide(indicator[ev->device]);
}

/**
 * @brief Callback for multi-touch move events.
 *
 * This function tracks the movement of an active touch point. It uses the
 * device ID to update the position of the corresponding indicator.
 *
 * @param data Unused user data.
 * @param e The Evas canvas.
 * @param o The Evas object that triggered the event.
 * @param event_info The event-specific information, containing coordinates
 *                   and device ID.
 */
static void
_multi_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Evas_Event_Multi_Move *ev = event_info;
   printf("MULTI: move @ %4i %4i | dev: %i\n", ev->cur.canvas.x,
          ev->cur.canvas.y, ev->device);
   if (ev->device >= IND_NUM) return;
   evas_object_move(indicator[ev->device], ev->cur.canvas.x, ev->cur.canvas.y);
   evas_object_resize(indicator[ev->device], 1, 1);
}

/**
 * @brief Rotates the main window by 90 degrees.
 *
 * This is a callback function for a button's "clicked" event. Each click
 * rotates the window by an additional 90 degrees clockwise.
 *
 * @param data The main window Evas_Object.
 * @param obj The button object that was clicked.
 * @param event_info Unused event information.
 */
void
my_bt_rot(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   elm_win_rotation_set(win, elm_win_rotation_get(win) + 90);
}

/**
 * @brief Sets up and runs the multi-touch test window.
 *
 * This function creates the main window, a background, and a transparent
 * rectangle on top to capture all mouse and touch events. It initializes
 * an array of visual indicators for touch points and registers the
 * necessary event callbacks for single and multi-touch input. It also
 * adds a button to rotate the window.
 *
 * @param data Unused user data.
 * @param obj Unused object.
 * @param event_info Unused event information.
 */
void
test_multi(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *r, *bx, *bt;
   int i;

   win = elm_win_add(NULL, "multi-touch", ELM_WIN_BASIC);
   elm_win_title_set(win, "Multi touch");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   r = evas_object_rectangle_add(evas_object_evas_get(win));
   evas_object_size_hint_weight_set(r, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_color_set(r, 0, 0, 0, 0);
   elm_win_resize_object_add(win, r);
   evas_object_show(r);

   for (i = 0; i < IND_NUM; i++)
     {
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "%s/objects/multip.edj", elm_app_data_dir_get());
        indicator[i] = edje_object_add(evas_object_evas_get(win));
        edje_object_file_set(indicator[i], buf, "point");
     }

   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_DOWN, _mouse_down, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_UP, _mouse_up, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MOUSE_MOVE, _mouse_move, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_DOWN, _multi_down, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_UP, _multi_up, win);
   evas_object_event_callback_add(r, EVAS_CALLBACK_MULTI_MOVE, _multi_move, win);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Rotate");
   evas_object_smart_callback_add(bt, "clicked", my_bt_rot, win);
   evas_object_size_hint_align_set(bt, 0.0, 0.0);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx, bt);
   evas_object_show(bt);

   evas_object_size_hint_min_set(bg, 160 * elm_config_scale_get(),
                                     160 * elm_config_scale_get());
   evas_object_resize(win, 480 * elm_config_scale_get(),
                           800 * elm_config_scale_get());

   evas_object_show(win);
}
