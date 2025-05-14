#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
// FIXME: use smart cb
#include "elm_notify_eo.h"

#define MAX_TRY 40

static int try_num = 0;

/**
 * @brief Callback function to delete a timer associated with an Evas object.
 *
 * This function is typically called when the Evas object is deleted.
 * It retrieves a timer stored in the object's data and deletes it.
 *
 * @param data User data, unused in this function.
 * @param e The Evas canvas, unused in this function.
 * @param obj The Evas object from which to delete the timer.
 * @param event_info Event specific information, unused in this function.
 */
static void
_timer_del(void *data       EINA_UNUSED,
           Evas *e          EINA_UNUSED,
           Evas_Object     *obj,
           void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_del(obj, "test-timer");
   if (!timer) return;
   ecore_timer_del(timer);
}

/**
 * @brief Callback function to attempt connecting a plug object to a server.
 *
 * This function is called periodically by a timer to try and establish
 * a connection to a named socket ("ello"). It retries up to MAX_TRY times.
 * On successful connection, the timer is cancelled.
 *
 * @param data Pointer to the Evas_Object (plug) to connect.
 * @return ECORE_CALLBACK_RENEW to continue trying, or ECORE_CALLBACK_CANCEL to stop.
 */
static Eina_Bool
cb_plug_connect(void *data)
{
   Evas_Object *obj = data;
   Ecore_Timer *timer;

   if (!obj) return ECORE_CALLBACK_CANCEL;

   try_num++;
   if (try_num > MAX_TRY) return ECORE_CALLBACK_CANCEL;

   timer= evas_object_data_get(obj, "test-timer");
   if (!timer) return ECORE_CALLBACK_CANCEL;

   if (elm_plug_connect(obj, "ello", 0, EINA_FALSE))
     {
        printf("plug connect to server[ello]\n");
        evas_object_data_del(obj, "test-timer");
        return ECORE_CALLBACK_CANCEL;
     }

   ecore_timer_interval_set(timer, 1);
   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Callback function invoked when the plug object is disconnected from the server.
 *
 * This function cleans up any existing connection attempt timer and
 * starts a new timer to attempt reconnection via cb_plug_connect().
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (plug) that was disconnected.
 * @param event_info Event specific information, unused in this function.
 */
static void
cb_plug_disconnected(void *data EINA_UNUSED,
                    Evas_Object *obj,
                    void *event_info EINA_UNUSED)
{
   Ecore_Timer *timer = evas_object_data_get(obj, "test-timer");
   if (timer)
     {
        ecore_timer_del(timer);
        evas_object_data_del(obj, "test-timer");
     }

   timer = ecore_timer_add(1, cb_plug_connect, obj);
   evas_object_data_set(obj, "test-timer", timer);
}

/**
 * @brief Callback function invoked when the server-side image of the plug is resized.
 *
 * Prints the new dimensions of the server image.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object (plug) whose server image was resized, unused.
 * @param event_info Pointer to Evas_Coord_Size containing the new width and height.
 *                   Example: Evas_Coord_Size size = { .w = 640, .h = 480 };
 */
static void
cb_plug_resized(void *data EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info)
{
   Evas_Coord_Size *size = event_info;
   printf("server image resized to %dx%d\n", size->w, size->h);
}

/**
 * @brief Callback function for mouse down events on the plug's image object.
 *
 * If the left mouse button (button 1) is pressed, it sets focus to the object.
 *
 * @param data User data, unused in this function.
 * @param evas The Evas canvas, unused in this function.
 * @param obj The Evas_Object that received the mouse down event.
 * @param event_info Pointer to Evas_Event_Mouse_Down containing event details.
 */
static void
cb_mouse_down(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   if (ev->button == 1) elm_object_focus_set(obj, EINA_TRUE);
}

/**
 * @brief Callback function for mouse move events on a handle object.
 *
 * This function is responsible for moving the main plug image object (`orig`)
 * when one of its handles is dragged. It also updates the Evas_Map of the
 * `orig` object to create a perspective distortion effect based on the
 * positions of the four handles.
 *
 * @param data Pointer to the main Evas_Object (the plug image) that is being manipulated.
 * @param evas The Evas canvas, unused in this function.
 * @param obj The Evas_Object (a handle) that received the mouse move event.
 * @param event_info Pointer to Evas_Event_Mouse_Move containing event details.
 */
static void
cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Object *orig = data;
   Evas_Coord x, y;
   Evas_Map *p;
   int i, w, h;

   if (!ev->buttons) return;
   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_move(obj,
                    x + (ev->cur.canvas.x - ev->prev.output.x),
                    y + (ev->cur.canvas.y - ev->prev.output.y));
   evas_object_image_size_get(orig, &w, &h);
   p = evas_map_new(4);
   evas_object_map_enable_set(orig, EINA_TRUE);
   evas_object_raise(orig);
   for (i = 0; i < 4; i++)
     {
        Evas_Object *hand;
        char key[32];

        snprintf(key, sizeof(key), "h-%i\n", i);
        hand = evas_object_data_get(orig, key);
        evas_object_raise(hand);
        evas_object_geometry_get(hand, &x, &y, NULL, NULL);
        x += 15;
        y += 15;
        evas_map_point_coord_set(p, i, x, y, 0);
        if (i == 0) evas_map_point_image_uv_set(p, i, 0, 0);
        else if (i == 1) evas_map_point_image_uv_set(p, i, w, 0);
        else if (i == 2) evas_map_point_image_uv_set(p, i, w, h);
        else if (i == 3) evas_map_point_image_uv_set(p, i, 0, h);
     }
   evas_object_map_set(orig, p);
   evas_map_free(p);
}

/**
 * @brief Creates four draggable handle objects at the corners of a given Evas object.
 *
 * These handles are small images. When moved (see cb_mouse_move()),
 * they control the mapping (perspective distortion) of the parent object.
 * Each handle is stored in the parent object's data using keys like "h-0", "h-1", etc.
 *
 * @param obj The Evas_Object for which to create corner handles. This is typically
 *            the image object obtained from an elm_plug.
 */
static void
create_handles(Evas_Object *obj)
{
   int i;
   Evas_Coord x, y, w, h;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   for (i = 0; i < 4; i++)
     {
        Evas_Object *hand;
        char buf[PATH_MAX];
        char key[32];

        hand = evas_object_image_filled_add(evas_object_evas_get(obj));
        evas_object_resize(hand, 31, 31);
        snprintf(buf, sizeof(buf), "%s/images/pt.png", elm_app_data_dir_get());
        evas_object_image_file_set(hand, buf, NULL);
        if (i == 0)      evas_object_move(hand, x     - 15, y     - 15);
        else if (i == 1) evas_object_move(hand, x + w - 15, y     - 15);
        else if (i == 2) evas_object_move(hand, x + w - 15, y + h - 15);
        else if (i == 3) evas_object_move(hand, x     - 15, y + h - 15);
        evas_object_event_callback_add(hand, EVAS_CALLBACK_MOUSE_MOVE, cb_mouse_move, obj);
        evas_object_show(hand);
        snprintf(key, sizeof(key), "h-%i\n", i);
        evas_object_data_set(obj, key, hand);
     }
}

/**
 * @brief Callback function invoked when a notification is dismissed.
 *
 * Deletes the notification object and stops further event callbacks for it.
 *
 * @param data User data, unused in this function.
 * @param event The Efl_Event structure containing event details.
 *              The event->object is the notification object itself.
 */
static void
_notify_end(void *data EINA_UNUSED, const Efl_Event *event)
{
   efl_del(event->object);
   efl_event_callback_stop(event->object);
}

/**
 * @brief Displays an error message using an Elementary notification.
 *
 * Creates and shows a temporary notification pop-up with the given error message.
 * The notification auto-dismisses after a timeout.
 *
 * @param parent The parent Evas_Object for the notification.
 * @param msg The error message string to display. Example: "Connection failed."
 */
static inline void
_notify_error(Evas_Object *parent, const char *msg)
{
   Evas_Object *notif, *txt;

   printf("%s\n", msg);

   notif = elm_notify_add(parent);
   evas_object_size_hint_weight_set(notif, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_notify_align_set(notif, 0.5, 1.0);
   elm_notify_timeout_set(notif, 3.0);
   efl_event_callback_add(notif, ELM_NOTIFY_EVENT_DISMISSED, _notify_end, NULL);

   txt = elm_label_add(notif);
   elm_object_text_set(txt, msg);
   elm_object_content_set(notif, txt);

   evas_object_show(txt);
   evas_object_show(notif);
}

/**
 * @brief Main function to create and run the Elm_Plug test window.
 *
 * This function sets up a window with a background and an Elm_Plug object.
 * The plug attempts to connect to a socket named "ello". If successful,
 * it displays the content from the socket. Draggable handles are created
 * to manipulate the perspective of the plug's displayed image.
 * If the connection fails, an error notification is shown (if `data` is provided)
 * or an error is printed to stderr.
 *
 * @param data Optional parent Evas_Object for displaying error notifications.
 *             If NULL, errors are printed to stderr.
 * @param obj Unused parameter.
 * @param event_info Unused parameter.
 */
void
test_win_plug(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *plug;
   char buf[PATH_MAX];

   win = elm_win_add(NULL, "window-plug", ELM_WIN_BASIC);
   elm_win_title_set(win, "Window Plug");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/plant_01.jpg", elm_app_data_dir_get());
   elm_bg_file_set(bg, buf, NULL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   plug = elm_plug_add(win);
   evas_object_event_callback_add(elm_plug_image_object_get(plug), EVAS_CALLBACK_MOUSE_DOWN, cb_mouse_down, NULL);
   evas_object_event_callback_add(plug, EVAS_CALLBACK_DEL, _timer_del, NULL);
   if (!elm_plug_connect(plug, "ello", 0, EINA_FALSE))
     {
        if (data)
          _notify_error(data, "Unable to connect to the Window Socket!");
        else
          fprintf(stderr, "Unable to connect to the Window Socket!\n");
        evas_object_del(win);
        return;
     }

   evas_object_smart_callback_add(plug, "image,deleted", cb_plug_disconnected, NULL);
   evas_object_smart_callback_add(plug, "image,resized", cb_plug_resized, NULL);

   evas_object_resize(plug, 380, 500);
   evas_object_move(plug, 10, 10);
   evas_object_show(plug);

   create_handles(elm_plug_image_object_get(plug));

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           600 * elm_config_scale_get());
   evas_object_show(win);
}
