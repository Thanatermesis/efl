#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback to toggle map-buffering on/off for all map-buffers.
 *
 * This function is called when the "Map" checkbox is toggled. It iterates
 * through all map-buffer objects associated with the window and flips their
 * enabled state.
 *
 * @param data The window Evas_Object.
 * @param obj The checkbox object that triggered the callback (unused).
 * @param event_info Extra event information (unused).
 */
static void
mode_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        if (elm_mapbuf_enabled_get(mb))
          elm_mapbuf_enabled_set(mb, EINA_FALSE);
        else
          elm_mapbuf_enabled_set(mb, EINA_TRUE);
     }
}

/**
 * @brief Callback to toggle fullscreen mode for the window.
 *
 * @param data The window Evas_Object.
 * @param obj The checkbox object that triggered the callback (unused).
 * @param event_info Extra event information (unused).
 */
static void
full_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   elm_win_fullscreen_set(win, !elm_win_fullscreen_get(win));
}

/**
 * @brief Callback to toggle alpha on/off for all map-buffers.
 *
 * Based on the state of a checkbox, this enables or disables the alpha
 * channel for all map-buffer objects associated with the window.
 *
 * @param data The window Evas_Object.
 * @param obj The checkbox object.
 * @param event_info Extra event information (unused).
 */
static void
alpha_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        elm_mapbuf_alpha_set(mb, elm_check_state_get(obj));
     }
}

/**
 * @brief Callback to toggle smooth rendering on/off for all map-buffers.
 *
 * Based on the state of a checkbox, this enables or disables smooth
 * scaling for all map-buffer objects associated with the window.
 *
 * @param data The window Evas_Object.
 * @param obj The checkbox object.
 * @param event_info Extra event information (unused).
 */
static void
smooth_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        elm_mapbuf_smooth_set(mb, elm_check_state_get(obj));
     }
}

/**
 * @brief Callback to close and delete the main window.
 * @param data The window Evas_Object to be deleted.
 * @param obj The button object that triggered the callback (unused).
 * @param event_info Extra event information (unused).
 */
static void
close_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

/**
 * @brief Timer callback to initiate icon dragging.
 *
 * This function is triggered after a short delay when the user presses and
 * holds on an icon. It transitions the icon to a "dragging" state.
 * It freezes the parent scroller, detaches the icon from its table layout,
 * and changes the background colors of the map buffers to indicate a drag
 * operation is in progress.
 *
 * @param data The icon Evas_Object being dragged.
 * @return ECORE_CALLBACK_CANCEL (EINA_FALSE) to stop the timer from recurring.
 */
static Eina_Bool
tim_cb(void *data)
{
   Evas_Object *tb, *sc, *mb;
   Eina_List *list, *l;

   evas_object_data_del(data, "timer");
   tb = evas_object_data_get(data, "tb");
   sc = evas_object_data_get(data, "sc");
   elm_object_scroll_freeze_push(sc);
   evas_object_data_set(data, "dragging", (void *)(long)(1));
   evas_object_color_set(data, 255, 255, 255, 255);
   list = (Eina_List *)evas_object_data_get
      (elm_object_top_widget_get(data), "mbs");
   EINA_LIST_FOREACH(list, l, mb)
      evas_object_color_set(mb, 128, 128, 128, 128);
   elm_table_unpack(tb, data);
   return EINA_FALSE;
}

/**
 * @brief Callback invoked when an icon is deleted.
 *
 * Ensures that any pending timer for a drag operation is cancelled and
 * freed to prevent memory leaks or use-after-free errors.
 *
 * @param data Custom data associated with the callback (unused).
 * @param e The Evas canvas (unused).
 * @param obj The icon Evas_Object that is being deleted.
 * @param event_info Extra event information (unused).
 */
static void
ic_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *tim;

   tim = evas_object_data_get(obj, "timer");
   if (tim)
     {
        evas_object_data_del(obj, "timer");
        ecore_timer_del(tim);
     }
}

/**
 * @brief Callback for mouse-down events on an icon.
 *
 * This function initiates a timer for a potential drag-and-drop operation.
 * If the user holds the mouse button down, the timer will fire and start the
 * drag. It also stores initial mouse and icon coordinates for calculating
 * movement.
 *
 * @param data The icon Evas_Object.
 * @param e The Evas canvas (unused).
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Down structure.
 */
static void
ic_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Ecore_Timer *tim;
   Evas_Coord x, y, w, h;

   evas_object_color_set(data, 128, 0, 0, 128);

   tim = evas_object_data_get(obj, "timer");
   if (tim) evas_object_data_del(obj, "timer");
   tim = ecore_timer_add(1.0, tim_cb, obj);
   evas_object_data_set(obj, "timer", tim);

   evas_object_geometry_get(data, &x, &y, &w, &h);
   evas_object_data_set(obj, "x", (void *)(uintptr_t)(ev->canvas.x));
   evas_object_data_set(obj, "y", (void *)(uintptr_t)(ev->canvas.y));
   evas_object_data_set(obj, "px", (void *)(uintptr_t)(x));
   evas_object_data_set(obj, "py", (void *)(uintptr_t)(y));

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        printf("double click %p\n", obj);
     }
}

/**
 * @brief Callback for mouse-up events on an icon.
 *
 * This function completes a click or a drag-and-drop operation.
 * If no drag was initiated, it simply cancels the drag timer.
 * If the icon was being dragged, it places the icon back into the table
 * layout at the nearest cell, restores map-buffer colors, and unfreezes the
 * scroller. It ignores events that are on hold.
 *
 * @param data The icon Evas_Object.
 * @param e The Evas canvas (unused).
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Up structure.
 */
static void
ic_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Ecore_Timer *tim;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   evas_object_color_set(data, 255, 255, 255, 255);
   tim = evas_object_data_get(obj, "timer");
   if (tim)
     {
        evas_object_data_del(obj, "timer");
        ecore_timer_del(tim);
     }
   if (evas_object_data_get(obj, "dragging"))
     {
        Evas_Object *tb, *sc, *mb;
        Eina_List *list, *l;
        int tbx, tby;

        evas_object_data_del(obj, "dragging");
        tb = evas_object_data_get(obj, "tb");
        sc = evas_object_data_get(obj, "sc");
        elm_object_scroll_freeze_pop(sc);
        tbx = (int)(uintptr_t)evas_object_data_get(obj, "tbx");
        tby = (int)(uintptr_t)evas_object_data_get(obj, "tby");
        elm_table_pack(tb, obj, tbx, tby, 1, 1);
        list = (Eina_List *)evas_object_data_get
           (elm_object_top_widget_get(obj), "mbs");
        EINA_LIST_FOREACH(list, l, mb)
           evas_object_color_set(mb, 255, 255, 255, 255);
     }
}

/**
 * @brief Callback for mouse-move events on an icon.
 *
 * If a drag operation is in progress (indicated by the "dragging" data flag),
 * this function updates the icon's position to follow the mouse cursor.
 * If the event is put on hold (e.g. by a scroller), it cancels the drag
 * timer to prevent the drag from starting.
 *
 * @param data The icon Evas_Object.
 * @param e The Evas canvas (unused).
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Move structure.
 */
static void
ic_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   if (evas_object_data_get(obj, "dragging"))
     {
        Evas_Coord x, y, px, py;

        x = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "x");
        y = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "y");
        px = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "px");
        py = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "py");
        evas_object_move(obj,
                         px + ev->cur.canvas.x - x,
                         py + ev->cur.canvas.y - y);
    }
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        Ecore_Timer *tim;

        tim = evas_object_data_get(obj, "timer");
        if (tim)
          {
             evas_object_data_del(obj, "timer");
             ecore_timer_del(tim);
          }
        evas_object_color_set(data, 255, 255, 255, 255);
        return;
     }
}

/**
 * @brief Creates the main test window for a launcher-like icon grid.
 *
 * This function sets up a window containing multiple "pages" of icons.
 * Each page is a table within a map-buffer, and all pages are hosted in a
 * horizontal box inside a scroller.
 *
 * It demonstrates:
 * - A paged scroller interface.
 * - Icon drag-and-drop within a table layout.
 * - Use of map-buffers for visual effects (alpha, smoothing).
 *
 * The array of names[] provides labels for icons:
 * @code
 * const char *names[] =
 *   {
 *      "Hello",    "World",    "Spam",  "Egg",
 *      "Ham",      "Good",     "Bad",   "Milk",
 *      "Smell",    "Of",       "Sky",   "Gold",
 *      "Hole",     "Pig",      "And",   "Calm"
 *   };
 * @endcode
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_launcher(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *sc, *tb, *pad, *bt, *ic, *lb, *tb2, *mb, *ck, *bx, *bx2;
   int i, j, k, n, m;
   char buf[PATH_MAX];
   const char *names[] =
     {
        "Hello",    "World",    "Spam",  "Egg",
        "Ham",      "Good",     "Bad",   "Milk",
        "Smell",    "Of",       "Sky",   "Gold",
        "Hole",     "Pig",      "And",   "Calm"
     };
   Eina_List *mbs = NULL;

   win = elm_win_add(NULL, "launcher", ELM_WIN_BASIC);
   elm_win_title_set(win, "Launcher");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/sky_04.jpg", elm_app_data_dir_get());
   elm_bg_file_set(bg, buf, NULL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   elm_box_homogeneous_set(bx, EINA_TRUE);
   elm_box_horizontal_set(bx, EINA_TRUE);

   sc = elm_scroller_add(win);
   elm_scroller_bounce_set(sc, EINA_TRUE, EINA_FALSE);
   elm_scroller_policy_set(sc, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);

   n = 0; m = 0;
   for (k = 0 ; k < 8; k++)
     {
        tb = elm_table_add(win);
        evas_object_size_hint_weight_set(tb, 0.0, 0.0);
        evas_object_size_hint_align_set(tb, 0.5, 0.5);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 470, 4);
        evas_object_size_hint_weight_set(pad, 0.0, 0.0);
        evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(tb, pad, 1, 0, 5, 1);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 470, 4);
        evas_object_size_hint_weight_set(pad, 0.0, 0.0);
        evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(tb, pad, 1, 11, 5, 1);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 4, 4);
        evas_object_size_hint_weight_set(pad, 0.0, 0.0);
        evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(tb, pad, 0, 1, 1, 10);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 4, 4);
        evas_object_size_hint_weight_set(pad, 0.0, 0.0);
        evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(tb, pad, 6, 1, 1, 10);

        mb = elm_mapbuf_add(win);
        elm_object_content_set(mb, tb);
        evas_object_show(tb);

        for (j = 0; j < 5; j++)
          {
             for (i = 0; i < 5; i++)
               {
                  ic = elm_icon_add(win);
                  elm_object_scale_set(ic, 0.5);
                  snprintf(buf, sizeof(buf), "%s/images/icon_%02i.png", elm_app_data_dir_get(), n);
                  elm_image_file_set(ic, buf, NULL);
                  elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
                  evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                  evas_object_size_hint_align_set(ic, 0.5, 0.5);
                  elm_table_pack(tb, ic, 1 + i, 1 + (j * 2), 1, 1);
                  evas_object_show(ic);

                  lb = elm_label_add(win);
                  elm_object_style_set(lb, "marker");
                  elm_object_text_set(lb, names[m]);
                  evas_object_size_hint_weight_set(lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                  evas_object_size_hint_align_set(lb, 0.5, 0.5);
                  elm_table_pack(tb, lb, 1 + i, 1 + (j * 2) + 1, 1, 1);
                  evas_object_show(lb);

                  evas_object_event_callback_add(ic, EVAS_CALLBACK_DEL, ic_del_cb, ic);

                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_DOWN, ic_down_cb, ic);
                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_UP,   ic_up_cb,   ic);
                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_MOVE, ic_move_cb, ic);

                  evas_object_data_set(ic, "lb", lb);
                  evas_object_data_set(ic, "tb", tb);
                  evas_object_data_set(ic, "sc", sc);
                  evas_object_data_set(ic, "bx", bx);
                  evas_object_data_set(ic, "mb", mb);
                  evas_object_data_set(ic, "tbx", (void *)(uintptr_t)(1 + i));
                  evas_object_data_set(ic, "tby", (void *)(uintptr_t)(1 + (j * 2)));

                  n++; if (n > 23) n = 0;
                  m++; if (m > 15) m = 0;
               }
          }

        elm_box_pack_end(bx, mb);
        evas_object_show(mb);

        mbs = eina_list_append(mbs, mb);
     }

   // fixme: free mbs
   evas_object_data_set(win, "mbs", mbs);

   bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bx2, EINA_FALSE);
   elm_win_resize_object_add(win, bx2);
   evas_object_show(bx2);

   elm_object_content_set(sc, bx);
   evas_object_show(bx);

   elm_scroller_page_relative_set(sc, 1.0, 1.0);
   evas_object_show(sc);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_fill_set(tb2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx2, tb2);

   elm_box_pack_end(bx2, sc);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Map");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", mode_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.05, 0.99);
   elm_table_pack(tb2, ck, 0, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "A");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", alpha_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.20, 0.99);
   elm_table_pack(tb2, ck, 1, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Smo");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", smooth_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.40, 0.99);
   elm_table_pack(tb2, ck, 2, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "FS");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", full_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.5, 0.99);
   elm_table_pack(tb2, ck, 3, 0, 1, 1);
   evas_object_show(ck);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", close_cb, win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.95, 0.99);
   elm_table_pack(tb2, bt, 4, 0, 1, 1);
   evas_object_show(bt);

   evas_object_show(tb2);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Creates a second test window using layouts for pages.
 *
 * This function sets up a window similar to test_launcher(), but each page
 * is an `Elm_Layout` object loaded from an EDJE file ("test.edj").
 * Buttons are placed into specific parts of the layout.
 *
 * It demonstrates:
 * - Using `Elm_Layout` for complex page structure.
 * - Paging with `elm_scroller_page_size_set()`.
 * - Use of map-buffers on layouts.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_launcher2(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *sc, *bt, *tb2, *mb, *ck, *bx, *ly;
   int k;
   char buf[PATH_MAX];
   Eina_List *mbs = NULL;

   win = elm_win_add(NULL, "launcher2", ELM_WIN_BASIC);
   elm_win_title_set(win, "Launcher 2");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/sky_03.jpg", elm_app_data_dir_get());
   elm_bg_file_set(bg, buf, NULL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   elm_box_homogeneous_set(bx, EINA_TRUE);
   elm_box_horizontal_set(bx, EINA_TRUE);

   for (k = 0 ; k < 8; k++)
     {
        ly = elm_layout_add(win);
        snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
        elm_layout_file_set(ly, buf, "layout");
        evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Button 1");
        elm_object_part_content_set(ly, "element1", bt);
        evas_object_show(bt);

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Button 2");
        elm_object_part_content_set(ly, "element2", bt);
        evas_object_show(bt);

        bt = elm_button_add(win);
        elm_object_text_set(bt, "Button 3");
        elm_object_part_content_set(ly, "element3", bt);
        evas_object_show(bt);

        mb = elm_mapbuf_add(win);
        elm_object_content_set(mb, ly);
        evas_object_show(ly);

        elm_box_pack_end(bx, mb);
        evas_object_show(mb);

        mbs = eina_list_append(mbs, mb);
     }

   // fixme: free mbs
   evas_object_data_set(win, "mbs", mbs);

   sc = elm_scroller_add(win);
   elm_scroller_bounce_set(sc, EINA_TRUE, EINA_FALSE);
   elm_scroller_policy_set(sc, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, sc);

   elm_object_content_set(sc, bx);
   evas_object_show(bx);

   //content size of one page is 550 x 410
   elm_scroller_page_size_set(sc, 555, 410);
   evas_object_show(sc);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb2);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Map");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", mode_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.05, 0.99);
   elm_table_pack(tb2, ck, 0, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "A");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", alpha_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.20, 0.99);
   elm_table_pack(tb2, ck, 1, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Smo");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", smooth_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.40, 0.99);
   elm_table_pack(tb2, ck, 2, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "FS");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", full_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.5, 0.99);
   elm_table_pack(tb2, ck, 3, 0, 1, 1);
   evas_object_show(ck);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", close_cb, win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.95, 0.99);
   elm_table_pack(tb2, bt, 4, 0, 1, 1);
   evas_object_show(bt);

   evas_object_show(tb2);

   evas_object_resize(win, 555 * elm_config_scale_get(),
                           410 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback to toggle map-buffering for test_launcher3.
 * @see mode_cb
 * @param data The window Evas_Object.
 * @param obj The checkbox object (unused).
 * @param event_info Extra event information (unused).
 */
static void
l3_mode_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        if (elm_mapbuf_enabled_get(mb))
          elm_mapbuf_enabled_set(mb, EINA_FALSE);
        else
          elm_mapbuf_enabled_set(mb, EINA_TRUE);
     }
}

/**
 * @brief Callback to toggle fullscreen for test_launcher3.
 * @see full_cb
 * @param data The window Evas_Object.
 * @param obj The checkbox object (unused).
 * @param event_info Extra event information (unused).
 */
static void
l3_full_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   elm_win_fullscreen_set(win, !elm_win_fullscreen_get(win));
}

/**
 * @brief Callback to toggle alpha for test_launcher3 map-buffers.
 * @see alpha_cb
 * @param data The window Evas_Object.
 * @param obj The checkbox object.
 * @param event_info Extra event information (unused).
 */
static void
l3_alpha_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        elm_mapbuf_alpha_set(mb, elm_check_state_get(obj));
     }
}

/**
 * @brief Callback to toggle smooth rendering for test_launcher3 map-buffers.
 * @see smooth_cb
 * @param data The window Evas_Object.
 * @param obj The checkbox object.
 * @param event_info Extra event information (unused).
 */
static void
l3_smooth_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        elm_mapbuf_smooth_set(mb, elm_check_state_get(obj));
     }
}

/**
 * @brief Callback to toggle visibility of all map-buffers.
 *
 * This is connected to the "Hid" checkbox in test_launcher3.
 *
 * @param data The window Evas_Object.
 * @param obj The checkbox object (unused).
 * @param event_info Extra event information (unused).
 */
static void
l3_hidden_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *mb;
   Eina_List *mbs, *l;

   mbs = evas_object_data_get(win, "mbs");
   EINA_LIST_FOREACH(mbs, l, mb)
     {
        if (evas_object_visible_get(mb)) evas_object_hide(mb);
        else evas_object_show(mb);
     }
}

/**
 * @brief Callback to close the window for test_launcher3.
 * @see close_cb
 * @param data The window Evas_Object to be deleted.
 * @param obj The button object (unused).
 * @param event_info Extra event information (unused).
 */
static void
l3_close_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

/**
 * @brief Timer callback to initiate icon dragging in test_launcher3.
 *
 * Similar to tim_cb(), this function is triggered on a long press. It
 * prepares an icon for dragging by unsetting its content from the parent
 * layout's slot (e.g., "slot.0.1") and emitting a "drag" signal to the
 * icon's own layout to trigger a visual change.
 *
 * @param data The icon Evas_Object being dragged.
 * @return ECORE_CALLBACK_CANCEL (EINA_FALSE) to stop the timer from recurring.
 */
static Eina_Bool
l3_tim_cb(void *data)
{
   Evas_Object *ly, *ly2, *sc, *mb;
   Eina_List *list, *l;
   char buf[256];
   int slx, sly;

   evas_object_data_del(data, "timer");
   ly = evas_object_data_get(data, "ly");
   ly2 = evas_object_data_get(data, "ly2");
   sc = evas_object_data_get(data, "sc");
   elm_object_scroll_freeze_push(sc);
   evas_object_data_set(data, "dragging", (void *)(long)1);
   evas_object_color_set(data, 255, 255, 255, 255);
   list = (Eina_List *)evas_object_data_get
      (elm_object_top_widget_get(data), "mbs");
   EINA_LIST_FOREACH(list, l, mb)
      evas_object_color_set(mb, 128, 128, 128, 128);
   slx = (int)(uintptr_t)evas_object_data_get(data, "slx");
   sly = (int)(uintptr_t)evas_object_data_get(data, "sly");
   snprintf(buf, sizeof(buf), "slot.%i.%i", slx, sly);
   elm_object_part_content_unset(ly, buf);
   elm_layout_signal_emit(ly2, "drag", "app");
   return EINA_FALSE;
}

/**
 * @brief Callback for icon deletion in test_launcher3.
 * @see ic_del_cb
 * @param data Unused.
 * @param e Unused.
 * @param obj The icon Evas_Object being deleted.
 * @param event_info Unused.
 */
static void
l3_ic_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Ecore_Timer *tim;

   tim = evas_object_data_get(obj, "timer");
   if (tim)
     {
        evas_object_data_del(obj, "timer");
        ecore_timer_del(tim);
     }
}

/**
 * @brief Callback for mouse-down events on an icon in test_launcher3.
 *
 * Similar to ic_down_cb(), this starts a timer for a drag operation and
 * stores initial coordinates. It also emits a "click" signal to the icon's
 * layout to change its visual state (e.g., show a glow).
 *
 * @param data Unused.
 * @param e Unused.
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Down structure.
 */
static void
l3_ic_down_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Ecore_Timer *tim;
   Evas_Coord x, y, w, h;
   Evas_Object *ly2;

   tim = evas_object_data_get(obj, "timer");
   if (tim) evas_object_data_del(obj, "timer");
   tim = ecore_timer_add(1.0, l3_tim_cb, obj);
   evas_object_data_set(obj, "timer", tim);

   ly2 = evas_object_data_get(obj, "ly2");
   evas_object_geometry_get(ly2, &x, &y, &w, &h);
   evas_object_data_set(obj, "x", (void *)(uintptr_t)(ev->canvas.x));
   evas_object_data_set(obj, "y", (void *)(uintptr_t)(ev->canvas.y));
   evas_object_data_set(obj, "px", (void *)(uintptr_t)(x));
   evas_object_data_set(obj, "py", (void *)(uintptr_t)(y));

   elm_layout_signal_emit(ly2, "click", "app");

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     {
        printf("double click %p\n", obj);
     }
}

/**
 * @brief Callback for mouse-up events on an icon in test_launcher3.
 *
 * Completes a click or drag operation.
 * If dragging, it re-parents the icon's layout into the main page layout slot
 * and emits a "drop" signal.
 * If not dragging, it cancels the drag timer and emits an "unclick" signal
 * to revert the icon's visual state from "clicked".
 *
 * @param data Unused.
 * @param e Unused.
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Up structure.
 */
static void
l3_ic_up_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Ecore_Timer *tim;
   Evas_Object *ly2;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;

   ly2 = evas_object_data_get(obj, "ly2");
   tim = evas_object_data_get(obj, "timer");
   if (tim)
     {
        evas_object_data_del(obj, "timer");
        ecore_timer_del(tim);
     }
   if (evas_object_data_get(obj, "dragging"))
     {
        Evas_Object *ly, *sc, *mb;
        Eina_List *list, *l;
        int slx, sly;
        char buf[256];

        evas_object_data_del(obj, "dragging");
        ly = evas_object_data_get(obj, "ly");
        sc = evas_object_data_get(obj, "sc");
        elm_object_scroll_freeze_pop(sc);
        slx = (int)(uintptr_t)evas_object_data_get(obj, "slx");
        sly = (int)(uintptr_t)evas_object_data_get(obj, "sly");
        snprintf(buf, sizeof(buf), "slot.%i.%i", slx, sly);
        elm_object_part_content_set(ly, buf, ly2);
        list = (Eina_List *)evas_object_data_get
           (elm_object_top_widget_get(obj), "mbs");
        EINA_LIST_FOREACH(list, l, mb)
           evas_object_color_set(mb, 255, 255, 255, 255);
        elm_layout_signal_emit(ly2, "drop", "app");
     }
   else
      elm_layout_signal_emit(ly2, "unclick", "app");
}

/**
 * @brief Callback for mouse-move events on an icon in test_launcher3.
 *
 * Updates the position of the icon's layout (`ly2`) during a drag.
 * If the event is put on hold, it cancels the drag timer and emits a "cancel"
 * signal to the layout to abort the click/drag visual state.
 *
 * @param data Unused.
 * @param e Unused.
 * @param obj The icon Evas_Object that received the event.
 * @param event_info Pointer to Evas_Event_Mouse_Move structure.
 */
static void
l3_ic_move_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Object *ly2;

   ly2 = evas_object_data_get(obj, "ly2");

   if (evas_object_data_get(obj, "dragging"))
     {
        Evas_Coord x, y, px, py;

        x = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "x");
        y = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "y");
        px = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "px");
        py = (Evas_Coord)(uintptr_t)evas_object_data_get(obj, "py");
        evas_object_move(ly2,
                         px + ev->cur.canvas.x - x,
                         py + ev->cur.canvas.y - y);
    }
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        Ecore_Timer *tim;

        tim = evas_object_data_get(obj, "timer");
        if (tim)
          {
             evas_object_data_del(obj, "timer");
             ecore_timer_del(tim);
          }
        elm_layout_signal_emit(ly2, "cancel", "app");
        return;
     }
}

/**
 * @brief Creates a third, more advanced launcher test window.
 *
 * This function builds on test_launcher() but uses `Elm_Layout` for both
 * the pages and the individual icons, allowing for more complex visuals and
 * state changes via EDJE signals.
 *
 * It demonstrates:
 * - A fully layout-based icon grid.
 * - Drag-and-drop implemented by re-parenting layout objects.
 * - Using EDJE signals ("click", "drag", "drop", "cancel") to coordinate
 *   visual feedback between C code and the theme file.
 *
 * The array of names[] provides labels for icons:
 * @code
 * const char *names[] =
 *   {
 *      "Hello",    "World",    "Spam",  "Egg",
 *      "Ham",      "Good",     "Bad",   "Milk",
 *      "Smell",    "Of",       "Sky",   "Gold",
 *      "Hole",     "Pig",      "And",   "Calm"
 *   };
 * @endcode
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_launcher3(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *sc, *tb, *pad, *bt, *ic, *tb2, *mb, *ck, *bx, *bx2, *ly, *ly2;
   int i, j, k, n, m;
   char buf[PATH_MAX];
   const char *names[] =
     {
        "Hello",    "World",    "Spam",  "Egg",
        "Ham",      "Good",     "Bad",   "Milk",
        "Smell",    "Of",       "Sky",   "Gold",
        "Hole",     "Pig",      "And",   "Calm"
     };
   Eina_List *mbs = NULL;

   win = elm_win_add(NULL, "launcher", ELM_WIN_BASIC);
   elm_win_title_set(win, "Launcher");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   snprintf(buf, sizeof(buf), "%s/images/sky_04.jpg", elm_app_data_dir_get());
   elm_bg_file_set(bg, buf, NULL);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_homogeneous_set(bx, EINA_TRUE);
   elm_box_horizontal_set(bx, EINA_TRUE);

   sc = elm_scroller_add(win);
   elm_scroller_bounce_set(sc, EINA_TRUE, EINA_FALSE);
   elm_scroller_policy_set(sc, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set(sc, EVAS_HINT_FILL, EVAS_HINT_FILL);

   n = 0; m = 0;
   for (k = 0 ; k < 8; k++)
     {
        tb = elm_table_add(win);
        evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(tb, EVAS_HINT_FILL, EVAS_HINT_FILL);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 450, 10);
        elm_table_pack(tb, pad, 1, 0, 1, 1);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 450, 10);
        elm_table_pack(tb, pad, 1, 2, 1, 1);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 10, 450);
        elm_table_pack(tb, pad, 0, 1, 1, 1);

        pad = evas_object_rectangle_add(evas_object_evas_get(win));
        evas_object_size_hint_min_set(pad, 10, 450);
        elm_table_pack(tb, pad, 2, 1, 1, 1);

        mb = elm_mapbuf_add(win);
        mbs = eina_list_append(mbs, mb);

        elm_object_content_set(mb, tb);
        evas_object_show(tb);
        elm_box_pack_end(bx, mb);
        evas_object_show(mb);

        ly = elm_layout_add(win);
        snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
        elm_layout_file_set(ly, buf, "launcher_page");
        evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(tb, ly, 1, 1, 1, 1);
        evas_object_show(ly);

        for (j = 0; j < 4; j++)
          {
             for (i = 0; i < 4; i++)
               {
                  ly2 = elm_layout_add(win);
                  snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
                  elm_layout_file_set(ly2, buf, "launcher_icon");
                  evas_object_size_hint_weight_set(ly2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                  evas_object_size_hint_align_set(ly2, EVAS_HINT_FILL, EVAS_HINT_FILL);
                  elm_layout_text_set(ly2, "label", names[m]);

                  ic = elm_icon_add(win);
                  elm_object_scale_set(ic, 0.5);
                  snprintf(buf, sizeof(buf), "%s/images/icon_%02i.png", elm_app_data_dir_get(), n);
                  elm_image_file_set(ic, buf, NULL);
                  elm_image_resizable_set(ic, EINA_FALSE, EINA_FALSE);
                  evas_object_size_hint_weight_set(ic, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                  evas_object_size_hint_align_set(ic, 0.5, 0.5);
                  elm_object_part_content_set(ly2, "slot", ic);
                  evas_object_show(ic);

                  evas_object_event_callback_add(ic, EVAS_CALLBACK_DEL, l3_ic_del_cb, ic);

                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_DOWN, l3_ic_down_cb, ic);
                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_UP,   l3_ic_up_cb,   ic);
                  evas_object_event_callback_add(ic, EVAS_CALLBACK_MOUSE_MOVE, l3_ic_move_cb, ic);

                  evas_object_data_set(ic, "ly", ly);
                  evas_object_data_set(ic, "ly2", ly2);
                  evas_object_data_set(ic, "sc", sc);
                  evas_object_data_set(ic, "bx", bx);
                  evas_object_data_set(ic, "mb", mb);
                  evas_object_data_set(ic, "slx", (void *)(uintptr_t)(i));
                  evas_object_data_set(ic, "sly", (void *)(uintptr_t)(j));

                  snprintf(buf, sizeof(buf), "slot.%i.%i", i, j);
                  elm_object_part_content_set(ly, buf, ly2);
                  evas_object_show(ly2);

                  n++; if (n > 23) n = 0;
                  m++; if (m > 15) m = 0;
               }
          }
     }

   // fixme: free mbs
   evas_object_data_set(win, "mbs", mbs);

   bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_horizontal_set(bx2, EINA_FALSE);
   elm_win_resize_object_add(win, bx2);
   evas_object_show(bx2);

   elm_object_content_set(sc, bx);
   evas_object_show(bx);

   elm_scroller_page_relative_set(sc, 1.0, 1.0);
   evas_object_show(sc);

   tb2 = elm_table_add(win);
   evas_object_size_hint_weight_set(tb2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_fill_set(tb2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx2, tb2);

   elm_box_pack_end(bx2, sc);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Map");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", l3_mode_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.05, 0.99);
   elm_table_pack(tb2, ck, 0, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "A");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", l3_alpha_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.20, 0.99);
   elm_table_pack(tb2, ck, 1, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Smo");
   elm_check_state_set(ck, EINA_TRUE);
   evas_object_smart_callback_add(ck, "changed", l3_smooth_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.40, 0.99);
   elm_table_pack(tb2, ck, 2, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "FS");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", l3_full_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.5, 0.99);
   elm_table_pack(tb2, ck, 3, 0, 1, 1);
   evas_object_show(ck);

   ck = elm_check_add(win);
   elm_object_text_set(ck, "Hid");
   elm_check_state_set(ck, EINA_FALSE);
   evas_object_smart_callback_add(ck, "changed", l3_hidden_cb, win);
   evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ck, 0.5, 0.99);
   elm_table_pack(tb2, ck, 4, 0, 1, 1);
   evas_object_show(ck);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", l3_close_cb, win);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.95, 0.99);
   elm_table_pack(tb2, bt, 5, 0, 1, 1);
   evas_object_show(bt);

   evas_object_show(tb2);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
