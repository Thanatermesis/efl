#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <Ecore.h>
#include "ecore_private.h"
#include <Ecore_Input.h>
#include <Ecore_Input_Evas.h>

#include "Ecore_Evas.h"
#include "ecore_evas_buffer.h"
#include "ecore_evas_private.h"

/**
 * @internal
 * @brief Frees resources associated with an Ecore_Evas buffer instance.
 *
 * This function cleans up the Ecore_Evas buffer engine data, including
 * unregistering input events, freeing pixel data (or an associated Evas image),
 * and shutting down Ecore_Event_Evas if necessary.
 *
 * @param ee The Ecore_Evas instance to free.
 */
static void
_ecore_evas_buffer_free(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;

   ecore_evas_input_event_unregister(ee);

   if (!bdata) return;
   if (bdata->image)
     {
        Ecore_Evas *ee2;

        ee2 = evas_object_data_get(bdata->image, "Ecore_Evas_Parent");
        evas_object_del(bdata->image);
        if (ee2)
          ee2->sub_ecore_evas = eina_list_remove(ee2->sub_ecore_evas, ee);
     }
   else
     {
        bdata->free_func(bdata->data, bdata->pixels);
     }

   free(bdata);
   ee->engine.data = NULL;

   ecore_event_evas_shutdown();
}

/**
 * @internal
 * @brief Moves the Ecore_Evas buffer instance.
 *
 * This function updates the position of the Ecore_Evas buffer.
 * If the buffer is associated with an Evas image object, this function does nothing
 * as the image object's position is managed separately.
 *
 * @param ee The Ecore_Evas instance to move.
 * @param x The new x-coordinate.
 * @param y The new y-coordinate.
 */
static void
_ecore_evas_move(Ecore_Evas *ee, int x, int y)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;

   if (bdata->image) return;
   ee->x = ee->req.x = x;
   ee->y = ee->req.y = y;
}

/**
 * @internal
 * @brief Resizes the Ecore_Evas buffer instance.
 *
 * This function handles resizing of the Ecore_Evas buffer. It updates
 * the Evas canvas size, reallocates pixel data if necessary (for non-image
 * backed buffers), and updates the Evas engine information with the new
 * buffer parameters.
 *
 * @param ee The Ecore_Evas instance to resize.
 * @param w The new width.
 * @param h The new height.
 */
static void
_ecore_evas_resize(Ecore_Evas *ee, int w, int h)
{
   Evas_Engine_Info_Buffer *einfo;
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;
   int stride = 0;

   if (w < 1) w = 1;
   if (h < 1) h = 1;
   ee->req.w = w;
   ee->req.h = h;
   if ((w == ee->w) && (h == ee->h)) return;
   ee->w = w;
   ee->h = h;
   evas_output_size_set(ee->evas, ee->w, ee->h);
   evas_output_viewport_set(ee->evas, 0, 0, ee->w, ee->h);
   evas_damage_rectangle_add(ee->evas, 0, 0, ee->w, ee->h);

   if (bdata->image)
     {
        bdata->pixels = evas_object_image_data_get(bdata->image, 1);
        stride = evas_object_image_stride_get(bdata->image);
     }
   else
     {
        if (bdata->pixels)
          bdata->free_func(bdata->data, bdata->pixels);
        bdata->pixels =
          bdata->alloc_func(bdata->data, ee->w * ee->h * sizeof(int));
        stride = ee->w * sizeof(int);
     }

   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        if (ee->alpha)
          einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        else
          einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
        einfo->info.dest_buffer = bdata->pixels;
        einfo->info.dest_buffer_row_bytes = stride;
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
     }
   if (bdata->image)
     evas_object_image_data_set(bdata->image, bdata->pixels);
   else
     bdata->resized = 1;
}

/**
 * @internal
 * @brief Moves and resizes the Ecore_Evas buffer instance.
 *
 * Currently, this function only calls the resize function, as the move
 * operation for buffer Ecore_Evas instances (not backed by an image object)
 * is handled by _ecore_evas_move. The x and y parameters are unused.
 *
 * @param ee The Ecore_Evas instance.
 * @param x The new x-coordinate (unused).
 * @param y The new y-coordinate (unused).
 * @param w The new width.
 * @param h The new height.
 */
static void
_ecore_evas_move_resize(Ecore_Evas *ee, int x EINA_UNUSED, int y EINA_UNUSED, int w, int h)
{
   _ecore_evas_resize(ee, w, h);
}

/**
 * @internal
 * @brief Sets whether the Ecore_Evas buffer instance ignores events.
 *
 * If the buffer is associated with an Evas image object, this function
 * will also set the pass_events property on the image object.
 *
 * @param ee The Ecore_Evas instance.
 * @param val 1 to ignore events, 0 to process them.
 */
static void
_ecore_evas_buffer_ignore_events_set(Ecore_Evas *ee, int val)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;

   if (ee->ignore_events == val) return;
   ee->ignore_events = val;
   if (bdata->image)
     evas_object_pass_events_set(bdata->image, val);
}

/**
 * @internal
 * @brief Shows the Ecore_Evas buffer instance.
 *
 * This function marks the Ecore_Evas as not withdrawn and triggers state change
 * and focus callbacks. It does nothing if the buffer is associated with an
 * Evas image object, as visibility is handled by the image object itself.
 *
 * @param ee The Ecore_Evas instance to show.
 */
static void
_ecore_evas_show(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;

   if (bdata->image) return;
   if (ecore_evas_focus_device_get(ee, NULL)) return;
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
}

/**
 * @internal
 * @brief Sets the title for the Ecore_Evas buffer instance.
 *
 * This function updates the title property of the Ecore_Evas.
 * The title is typically used for window titles, but in a buffer context,
 * it's stored for informational purposes or potential use by parent systems.
 *
 * @param ee The Ecore_Evas instance.
 * @param t The new title string.
 */
static void
_ecore_evas_buffer_title_set(Ecore_Evas *ee, const char *t)
{
   if (eina_streq(ee->prop.title, t)) return;
   if (ee->prop.title) free(ee->prop.title);
   ee->prop.title = NULL;
   if (!t) return;
   ee->prop.title = strdup(t);
}

/**
 * @internal
 * @brief Sets the name and class for the Ecore_Evas buffer instance.
 *
 * These properties are typically used for window management hints. In a
 * buffer context, they are stored for informational purposes.
 *
 * @param ee The Ecore_Evas instance.
 * @param n The new name string.
 * @param c The new class string.
 */
static void
_ecore_evas_buffer_name_class_set(Ecore_Evas *ee, const char *n, const char *c)
{
   if (!eina_streq(n, ee->prop.name))
     {
       	free(ee->prop.name);
       	ee->prop.name = NULL;
       	if (n) ee->prop.name = strdup(n);
     }
   if (!eina_streq(c, ee->prop.clas))
     {
       	free(ee->prop.clas);
       	ee->prop.clas = NULL;
       	if (c) ee->prop.clas = strdup(c);
     }
}

/**
 * @internal
 * @brief Prepares the Ecore_Evas buffer for rendering.
 *
 * This function is called before rendering. It checks if the associated
 * Evas image object (if any) has been resized and updates the Ecore_Evas
 * accordingly. It also locks the image data if changes are detected and
 * data is not already locked. For non-image backed buffers, it calls the
 * resize callback if the buffer was marked as resized.
 *
 * @param ee The Ecore_Evas instance.
 * @return EINA_TRUE always.
 */
static Eina_Bool
_ecore_evas_buffer_prepare(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Buffer_Data *bdata;

   bdata = ee->engine.data;
   if (bdata->image)
     {
        int w, h;

        evas_object_image_size_get(bdata->image, &w, &h);
        if ((w != ee->w) || (h != ee->h))
          _ecore_evas_resize(ee, w, h);
        if (evas_changed_get(ee->evas) && !bdata->lock_data)
          {
             bdata->pixels = evas_object_image_data_get(bdata->image, 1);
             bdata->lock_data = EINA_TRUE;
          }
     }
   else if (bdata->resized)
     {
        if (ee->func.fn_resize) ee->func.fn_resize(ee);
        bdata->resized = 0;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function called after an Evas render cycle for image-backed buffers.
 *
 * This function is registered as an EVAS_CALLBACK_RENDER_POST callback.
 * It updates the associated Evas image object with the new pixel data from the
 * Ecore_Evas buffer and marks the updated regions on the image. It then unlocks
 * the pixel data.
 *
 * @param data The Ecore_Evas instance (passed as user data).
 * @param e The Evas canvas (unused).
 * @param event_info Pointer to Evas_Event_Render_Post structure containing update regions.
 */
static void
_ecore_evas_buffer_update_image(void *data, Evas *e EINA_UNUSED, void *event_info)
{
   Evas_Event_Render_Post *post = event_info;
   Ecore_Evas *ee = data;
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;
   Eina_Rectangle *r;
   Eina_List *l;

   evas_object_image_data_set(bdata->image, bdata->pixels);
   EINA_LIST_FOREACH(post->updated_area, l, r)
     evas_object_image_data_update_add(bdata->image,
                                       r->x, r->y, r->w, r->h);

   bdata->lock_data = EINA_FALSE;
}

/**
 * @brief Renders the Ecore_Evas buffer and waits for completion.
 *
 * This function performs a synchronous render of the Ecore_Evas buffer.
 * It calls ecore_evas_render() to trigger rendering and then
 * ecore_evas_render_wait() to ensure rendering is finished before returning.
 *
 * @param ee The Ecore_Evas instance to render.
 * @return The number of updates rendered.
 */
EAPI int
ecore_evas_buffer_render(Ecore_Evas *ee)
{
   int r;

   r = ecore_evas_render(ee);
   ecore_evas_render_wait(ee);
   return r;
}

/**
 * @internal
 * @brief Translates coordinates from the parent Evas canvas to the buffer's Evas canvas.
 *
 * This function is used when the Ecore_Evas buffer is rendered onto an
 * Evas image object in a parent Evas. It adjusts mouse coordinates based on the
 * image object's geometry, fill properties, and mapping state to correctly
 * propagate events to the buffer's Evas instance.
 *
 * @param ee The Ecore_Evas instance (the buffer).
 * @param[in,out] x Pointer to the x-coordinate to translate.
 * @param[in,out] y Pointer to the y-coordinate to translate.
 */
static void
_ecore_evas_buffer_coord_translate(Ecore_Evas *ee, Evas_Coord *x, Evas_Coord *y)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;
   Evas_Coord xx, yy, ww, hh, fx, fy, fw, fh;

   evas_object_geometry_get(bdata->image, &xx, &yy, &ww, &hh);
   evas_object_image_fill_get(bdata->image, &fx, &fy, &fw, &fh);

   if (fw < 1) fw = 1;
   if (fh < 1) fh = 1;

   if (evas_object_map_get(bdata->image) &&
       evas_object_map_enable_get(bdata->image))
     {
        fx = 0; fy = 0;
        fw = ee->w; fh = ee->h;
        ww = ee->w; hh = ee->h;
     }

   if ((fx == 0) && (fy == 0) && (fw == ww) && (fh == hh))
     {
        *x = (ee->w * (*x - xx)) / fw;
        *y = (ee->h * (*y - yy)) / fh;
     }
   else
     {
        xx = (*x - xx) - fx;
        while (xx < 0) xx += fw;
        while (xx > fw) xx -= fw;
        *x = (ee->w * xx) / fw;

        yy = (*y - yy) - fy;
        while (yy < 0) yy += fh;
        while (yy > fh) yy -= fh;
        *y = (ee->h * yy) / fh;
     }
}

/**
 * @internal
 * @brief Transfers key modifiers and lock states from one Evas canvas to another.
 *
 * This is used to ensure that the Ecore_Evas buffer's internal Evas canvas
 * has the same modifier (Shift, Ctrl, Alt, etc.) and lock (Caps_Lock, Num_Lock, etc.)
 * states as the parent Evas canvas when events are being propagated.
 *
 * @param e The source Evas canvas (typically the parent/outer Evas).
 * @param e2 The destination Evas canvas (typically the buffer's internal Evas).
 */
static void
_ecore_evas_buffer_transfer_modifiers_locks(Evas *e, Evas *e2)
{
   const char *mods[] =
     { "Shift", "Control", "Alt", "Meta", "Hyper", "Super", NULL };
   const char *locks[] =
     { "Scroll_Lock", "Num_Lock", "Caps_Lock", NULL };
   int i;

   for (i = 0; mods[i]; i++)
     {
        if (evas_key_modifier_is_set(evas_key_modifier_get(e), mods[i]))
          evas_key_modifier_on(e2, mods[i]);
        else
          evas_key_modifier_off(e2, mods[i]);
     }
   for (i = 0; locks[i]; i++)
     {
        if (evas_key_lock_is_set(evas_key_lock_get(e), locks[i]))
          evas_key_lock_on(e2, locks[i]);
        else
          evas_key_lock_off(e2, locks[i]);
     }
}

/**
 * @internal
 * @brief Callback for mouse_in events on the associated Evas image object.
 *
 * This function is called when the mouse enters the Evas image object that
 * represents the Ecore_Evas buffer. It transfers modifier/lock states and
 * feeds a mouse_in event to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_In event data (unused here, but cast internally).
 */
static void
_ecore_evas_buffer_cb_mouse_in(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_In *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_in(ee->evas, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for mouse_out events on the associated Evas image object.
 *
 * This function is called when the mouse leaves the Evas image object that
 * represents the Ecore_Evas buffer. It transfers modifier/lock states and
 * feeds a mouse_out event to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_Out event data (unused here, but cast internally).
 */
static void
_ecore_evas_buffer_cb_mouse_out(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Out *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_out(ee->evas, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for mouse_down events on the associated Evas image object.
 *
 * This function is called when a mouse button is pressed on the Evas image
 * object. It transfers modifier/lock states and feeds a mouse_down event
 * to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_Down event data.
 */
static void
_ecore_evas_buffer_cb_mouse_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Down *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_down(ee->evas, ev->button, ev->flags, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for mouse_up events on the associated Evas image object.
 *
 * This function is called when a mouse button is released on the Evas image
 * object. It transfers modifier/lock states and feeds a mouse_up event
 * to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_Up event data.
 */
static void
_ecore_evas_buffer_cb_mouse_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Up *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_up(ee->evas, ev->button, ev->flags, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for mouse_move events on the associated Evas image object.
 *
 * This function is called when the mouse moves over the Evas image object.
 * It translates the coordinates, transfers modifier/lock states, and processes
 * the mouse move event for the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_Move event data.
 */
static void
_ecore_evas_buffer_cb_mouse_move(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Move *ev;
   Evas_Coord x, y;

   ee = data;
   ev = event_info;
   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;
   if (!ee->evas) return;
   _ecore_evas_buffer_coord_translate(ee, &x, &y);
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   _ecore_evas_mouse_move_process(ee, x, y, ev->timestamp);
}

/**
 * @internal
 * @brief Callback for mouse_wheel events on the associated Evas image object.
 *
 * This function is called when the mouse wheel is scrolled over the Evas image
 * object. It transfers modifier/lock states and feeds a mouse_wheel event
 * to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Mouse_Wheel event data.
 */
static void
_ecore_evas_buffer_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Mouse_Wheel *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_mouse_wheel(ee->evas, ev->direction, ev->z, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for multi_down events (multi-touch) on the associated Evas image object.
 *
 * This function is called when a multi-touch down event occurs on the Evas
 * image object. It translates coordinates, transfers modifier/lock states,
 * and feeds a multi_down event to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Multi_Down event data.
 */
static void
_ecore_evas_buffer_cb_multi_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Down *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->canvas.x;
   y = ev->canvas.y;
   xx = x;
   yy = y;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_coord_translate(ee, &x, &y);
   xf = (ev->canvas.xsub - (double)xx) + (double)x;
   yf = (ev->canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_down(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->flags, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for multi_up events (multi-touch) on the associated Evas image object.
 *
 * This function is called when a multi-touch up event occurs on the Evas
 * image object. It translates coordinates, transfers modifier/lock states,
 * and feeds a multi_up event to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Multi_Up event data.
 */
static void
_ecore_evas_buffer_cb_multi_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Up *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->canvas.x;
   y = ev->canvas.y;
   xx = x;
   yy = y;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_coord_translate(ee, &x, &y);
   xf = (ev->canvas.xsub - (double)xx) + (double)x;
   yf = (ev->canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_up(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->flags, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for multi_move events (multi-touch) on the associated Evas image object.
 *
 * This function is called when a multi-touch move event occurs on the Evas
 * image object. It translates coordinates, transfers modifier/lock states,
 * and feeds a multi_move event to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Multi_Move event data.
 */
static void
_ecore_evas_buffer_cb_multi_move(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Multi_Move *ev;
   Evas_Coord x, y, xx, yy;
   double xf, yf;

   ee = data;
   ev = event_info;
   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;
   xx = x;
   yy = y;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_coord_translate(ee, &x, &y);
   xf = (ev->cur.canvas.xsub - (double)xx) + (double)x;
   yf = (ev->cur.canvas.ysub - (double)yy) + (double)y;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_multi_move(ee->evas, ev->device, x, y, ev->radius, ev->radius_x, ev->radius_y, ev->pressure, ev->angle, xf, yf, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for EVAS_CALLBACK_FREE on the associated Evas image object.
 *
 * This function is called when the Evas image object representing the
 * Ecore_Evas buffer is freed. It triggers the freeing of the
 * Ecore_Evas instance itself if its driver is still set (meaning it
 * hasn't been freed by other means).
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas image object being freed (unused).
 * @param event_info Event specific information (unused).
 */
static void
_ecore_evas_buffer_cb_free(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   if (ee->driver) _ecore_evas_free(ee);
}

/**
 * @internal
 * @brief Callback for key_down events on the associated Evas image object.
 *
 * This function is called when a key is pressed while the Evas image object
 * has focus. It transfers modifier/lock states and feeds a key_down event
 * to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Key_Down event data.
 */
static void
_ecore_evas_buffer_cb_key_down(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Key_Down *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_key_down(ee->evas, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for key_up events on the associated Evas image object.
 *
 * This function is called when a key is released while the Evas image object
 * has focus. It transfers modifier/lock states and feeds a key_up event
 * to the buffer's internal Evas canvas.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas of the image object.
 * @param obj The Evas image object (unused).
 * @param event_info The Evas_Event_Key_Up event data.
 */
static void
_ecore_evas_buffer_cb_key_up(void *data, Evas *e, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Ecore_Evas *ee;
   Evas_Event_Key_Up *ev;

   ee = data;
   ev = event_info;
   if (!ee->evas) return;
   if (ee->ignore_events) return;
   _ecore_evas_buffer_transfer_modifiers_locks(e, ee->evas);
   evas_event_feed_key_up(ee->evas, ev->keyname, ev->key, ev->string, ev->compose, ev->timestamp, NULL);
}

/**
 * @internal
 * @brief Callback for focus_in events on the associated Evas image object.
 *
 * This function is called when the Evas image object gains focus.
 * It sets the focus state on the Ecore_Evas buffer instance.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas image object (unused).
 * @param event_info Event specific information (unused).
 */
static void
_ecore_evas_buffer_cb_focus_in(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   if (!ee->evas) return;
   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);
}

/**
 * @internal
 * @brief Callback for focus_out events on the associated Evas image object.
 *
 * This function is called when the Evas image object loses focus.
 * It unsets the focus state on the Ecore_Evas buffer instance.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas image object (unused).
 * @param event_info Event specific information (unused).
 */
static void
_ecore_evas_buffer_cb_focus_out(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   if (!ee->evas) return;
   _ecore_evas_focus_device_set(ee, NULL, EINA_FALSE);
}

/**
 * @internal
 * @brief Callback for show events on the associated Evas image object.
 *
 * This function is called when the Evas image object representing the
 * Ecore_Evas buffer is shown. It updates the withdrawn and visible
 * properties of the Ecore_Evas instance and calls relevant state change
 * and show callbacks.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas image object (unused).
 * @param event_info Event specific information (unused).
 */
static void
_ecore_evas_buffer_cb_show(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.withdrawn = EINA_FALSE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   ee->visible = 1;
   if (ee->func.fn_show) ee->func.fn_show(ee);
}

/**
 * @internal
 * @brief Callback for hide events on the associated Evas image object.
 *
 * This function is called when the Evas image object representing the
 * Ecore_Evas buffer is hidden. It updates the withdrawn and visible
 * properties of the Ecore_Evas instance and calls relevant state change
 * and hide callbacks.
 *
 * @param data The Ecore_Evas instance (user data).
 * @param e The Evas canvas (unused).
 * @param obj The Evas image object (unused).
 * @param event_info Event specific information (unused).
 */
static void
_ecore_evas_buffer_cb_hide(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Ecore_Evas *ee;

   ee = data;
   ee->prop.withdrawn = EINA_TRUE;
   if (ee->func.fn_state_change) ee->func.fn_state_change(ee);
   ee->visible = 0;
   if (ee->func.fn_hide) ee->func.fn_hide(ee);
}

/**
 * @internal
 * @brief Sets the alpha channel state for the Ecore_Evas buffer.
 *
 * If the buffer is associated with an Evas image object, its alpha flag
 * is updated. Otherwise, the Evas engine info for the buffer's Evas
 * canvas is updated to reflect whether an alpha channel is used
 * (ARGB32) or not (RGB32).
 *
 * @param ee The Ecore_Evas instance.
 * @param alpha 1 to enable alpha channel, 0 to disable.
 */
static void
_ecore_evas_buffer_alpha_set(Ecore_Evas *ee, int alpha)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;
   if (((ee->alpha) && (alpha)) || ((!ee->alpha) && (!alpha))) return;
   ee->alpha = alpha;
   if (bdata->image)
     evas_object_image_alpha_set(bdata->image, ee->alpha);
   else
     {
        Evas_Engine_Info_Buffer *einfo;

        einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(ee->evas);
        if (einfo)
          {
             if (ee->alpha)
               einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
             else
               einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
             if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
               ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
          }
     }
}

/**
 * @internal
 * @brief Sets the profile for the Ecore_Evas buffer instance.
 *
 * This function updates the profile name associated with the Ecore_Evas.
 * Profiles can be used to hint at preferred window behaviors or appearances.
 * It also triggers a state change callback if one is set.
 *
 * @param ee The Ecore_Evas instance.
 * @param profile The new profile name string. Can be NULL to clear the profile.
 */
static void
_ecore_evas_buffer_profile_set(Ecore_Evas *ee, const char *profile)
{
   _ecore_evas_window_profile_free(ee);
   ee->prop.profile.name = NULL;

   if (profile)
     {
        ee->prop.profile.name = (char *)eina_stringshare_add(profile);

        /* just change ee's state.*/
        if (ee->func.fn_state_change)
          ee->func.fn_state_change(ee);
     }
}

/**
 * @internal
 * @brief Sends a message from the Ecore_Evas buffer to its parent.
 *
 * This function facilitates communication between a child Ecore_Evas (the buffer)
 * and its parent Ecore_Evas. If a parent exists and has a message handler,
 * the message is passed to it. Otherwise, if the buffer itself has a parent
 * message handler, it's called (e.g., for top-level buffers).
 *
 * @param ee The Ecore_Evas instance sending the message.
 * @param msg_domain The domain of the message.
 * @param msg_id The ID of the message within the domain.
 * @param data A pointer to the message data.
 * @param size The size of the message data.
 */
static void
_ecore_evas_buffer_msg_parent_send(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
   Ecore_Evas *parent_ee = NULL;
   parent_ee = ecore_evas_data_get(ee, "parent");

   if (parent_ee)
     {
        if (parent_ee->func.fn_msg_parent_handle)
          parent_ee ->func.fn_msg_parent_handle(parent_ee, msg_domain, msg_id, data, size);
     }
   else
     {
        if (ee->func.fn_msg_parent_handle)
          ee ->func.fn_msg_parent_handle(ee, msg_domain, msg_id, data, size);
     }
}

/**
 * @internal
 * @brief Sends a message from the Ecore_Evas buffer to its child.
 *
 * This function facilitates communication from a parent Ecore_Evas (the buffer)
 * to its child Ecore_Evas. If a child exists and has a message handler,
 * the message is passed to it. Otherwise, if the buffer itself has a
 * message handler, it's called.
 *
 * @param ee The Ecore_Evas instance sending the message.
 * @param msg_domain The domain of the message.
 * @param msg_id The ID of the message within the domain.
 * @param data A pointer to the message data.
 * @param size The size of the message data.
 */
static void
_ecore_evas_buffer_msg_send(Ecore_Evas *ee, int msg_domain, int msg_id, void *data, int size)
{
   Ecore_Evas *child_ee = NULL;
   child_ee = ecore_evas_data_get(ee, "child");

   if (child_ee)
     {
        if (child_ee->func.fn_msg_handle)
          child_ee->func.fn_msg_handle(child_ee, msg_domain, msg_id, data, size);
     }
   else
     {
        if (ee->func.fn_msg_handle)
          ee->func.fn_msg_handle(ee, msg_domain, msg_id, data, size);
     }
}

/**
 * @internal
 * @brief Gets the screen geometry of the Ecore_Evas buffer.
 *
 * For a buffer Ecore_Evas, the "screen" geometry is simply its own
 * position (x, y) and dimensions (w, h).
 *
 * @param ee The Ecore_Evas instance.
 * @param[out] x Pointer to store the x-coordinate.
 * @param[out] y Pointer to store the y-coordinate.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
static void
_ecore_evas_buffer_screen_geometry_get(const Ecore_Evas *ee, int *x, int *y, int *w, int *h)
{
   if (x) *x = ee->x;
   if (y) *y = ee->y;
   if (w) *w = ee->w;
   if (h) *h = ee->h;
}

/**
 * @internal
 * @brief Gets the current pointer (mouse) coordinates relative to the Ecore_Evas buffer's canvas.
 *
 * @param ee The Ecore_Evas instance.
 * @param[out] x Pointer to store the x-coordinate of the mouse.
 * @param[out] y Pointer to store the y-coordinate of the mouse.
 */
static void
_ecore_evas_buffer_pointer_xy_get(const Ecore_Evas *ee, Evas_Coord *x, Evas_Coord *y)
{
   evas_pointer_canvas_xy_get(ee->evas, x, y);
}

/**
 * @internal
 * @brief Warps (moves) the mouse pointer to a specific coordinate within the Ecore_Evas buffer.
 *
 * If the buffer is associated with an Evas image object, this simulates a mouse move
 * event. If it's a standalone buffer and not ignoring events, it creates and adds
 * an ECORE_EVENT_MOUSE_MOVE to the event queue.
 *
 * @param ee The Ecore_Evas instance.
 * @param x The target x-coordinate for the pointer.
 * @param y The target y-coordinate for the pointer.
 * @return EINA_TRUE on success, EINA_FALSE otherwise (though currently always returns EINA_TRUE).
 */
static Eina_Bool
_ecore_evas_buffer_pointer_warp(const Ecore_Evas *ee, Evas_Coord x, Evas_Coord y)
{
   Ecore_Evas_Engine_Buffer_Data *bdata = ee->engine.data;

   if (bdata->image)
     _ecore_evas_mouse_move_process((Ecore_Evas*)ee, x, y, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff));
   else if (!ee->ignore_events)
     {
        Ecore_Event_Mouse_Move *ev;

        ev = calloc(1, sizeof(Ecore_Event_Mouse_Move));
        EINA_SAFETY_ON_NULL_RETURN_VAL(ev, EINA_FALSE);

        ev->window = ee->prop.window;
        ev->event_window = ee->prop.window;
        ev->root_window = ee->prop.window;
        ev->timestamp = (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff);
        ev->same_screen = 1;

        ev->x = x;
        ev->y = y;
        ev->root.x = x;
        ev->root.y = y;

        {
           const char *mods[] =
             { "Shift", "Control", "Alt", "Super", NULL };
           int modifiers[] =
             { ECORE_EVENT_MODIFIER_SHIFT, ECORE_EVENT_MODIFIER_CTRL, ECORE_EVENT_MODIFIER_ALT,
               ECORE_EVENT_MODIFIER_WIN, 0 };
           int i;

           for (i = 0; mods[i]; i++)
             if (evas_key_modifier_is_set(evas_key_modifier_get(ee->evas), mods[i]))
               ev->modifiers |= modifiers[i];
        }

        //FIXME ev->multi.device = ???

        ev->multi.radius = 1;
        ev->multi.radius_x = 1;
        ev->multi.radius_y = 1;
        ev->multi.pressure = 1.0;
        ev->multi.angle = 0.0;
        ev->multi.x = ev->x;
        ev->multi.y = ev->y;
        ev->multi.root.x = ev->x;
        ev->multi.root.y = ev->y;

        ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, NULL, NULL);
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Table of engine functions for the Ecore_Evas buffer engine.
 *
 * This structure maps generic Ecore_Evas operations to their specific
 * implementations for the buffer engine.
 */
static Ecore_Evas_Engine_Func _ecore_buffer_engine_func =
{
   _ecore_evas_buffer_free,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_move,
   NULL,
   _ecore_evas_resize,
   _ecore_evas_move_resize,
   NULL,
   NULL,
   _ecore_evas_show,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_buffer_title_set,
   _ecore_evas_buffer_name_class_set,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   _ecore_evas_buffer_ignore_events_set,
   _ecore_evas_buffer_alpha_set,
   NULL, //transparent
   NULL, // profiles_set
   _ecore_evas_buffer_profile_set,

   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,

   NULL,
   _ecore_evas_buffer_screen_geometry_get,
   NULL,  // screen_dpi_get
   _ecore_evas_buffer_msg_parent_send,
   _ecore_evas_buffer_msg_send,

   _ecore_evas_buffer_pointer_xy_get, // pointer_xy_get
   _ecore_evas_buffer_pointer_warp, // pointer_warp

   NULL, // wm_rot_preferred_rotation_set
   NULL, // wm_rot_available_rotations_set
   NULL, // wm_rot_manual_rotation_done_set
   NULL, // wm_rot_manual_rotation_done

   NULL, // aux_hints_set

   NULL, // fn_animator_register
   NULL, // fn_animator_unregister

   NULL, // fn_evas_changed
   NULL, //fn_focus_device_set
   NULL, //fn_callback_focus_device_in_set
   NULL, //fn_callback_focus_device_out_set
   NULL, //fn_callback_device_mouse_in_set
   NULL, //fn_callback_device_mouse_out_set
   NULL, //fn_pointer_device_xy_get
   _ecore_evas_buffer_prepare,
   NULL // fn_last_tick_get
};

/**
 * @internal
 * @brief Default pixel allocation function for Ecore_Evas buffer.
 *
 * This function is used if no custom allocation function is provided
 * to ecore_evas_buffer_allocfunc_new(). It simply uses malloc.
 *
 * @param data User data (unused in this default implementation).
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
static void *
_ecore_evas_buffer_pix_alloc(void *data EINA_UNUSED, int size)
{
   return malloc(size);
}

/**
 * @internal
 * @brief Default pixel freeing function for Ecore_Evas buffer.
 *
 * This function is used if no custom freeing function is provided
 * to ecore_evas_buffer_allocfunc_new(). It simply uses free.
 *
 * @param data User data (unused in this default implementation).
 * @param pix A pointer to the memory to free.
 */
static void
_ecore_evas_buffer_pix_free(void *data EINA_UNUSED, void *pix)
{
   free(pix);
}

/**
 * @brief Creates a new Ecore_Evas backed by a pixel buffer with custom memory allocators.
 *
 * This function allows creation of an Ecore_Evas instance that renders to an
 * in-memory pixel buffer. It provides flexibility by allowing custom functions
 * for allocating and freeing the pixel buffer memory.
 *
 * @param w The initial width of the buffer.
 * @param h The initial height of the buffer.
 * @param alloc_func A function pointer to allocate memory for the pixel buffer.
 *                   It takes user data and size as parameters and returns a void pointer.
 *                   Example: `void *my_alloc(void *data, int size);`
 * @param free_func A function pointer to free memory used by the pixel buffer.
 *                  It takes user data and a pointer to the pixels as parameters.
 *                  Example: `void my_free(void *data, void *pixels);`
 * @param data A pointer to user-defined data that will be passed to alloc_func and free_func.
 * @return A new Ecore_Evas handle on success, NULL on failure.
 */
EAPI Ecore_Evas *
ecore_evas_buffer_allocfunc_new(int w, int h,
                                void *(*alloc_func) (void *data, int size),
                                void (*free_func) (void *data, void *pix),
                                const void *data)
{
   Evas_Engine_Info_Buffer *einfo;
   Ecore_Evas_Engine_Buffer_Data *bdata;
   Ecore_Evas *ee;
   int rmethod;

   EINA_SAFETY_ON_NULL_RETURN_VAL(alloc_func, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(free_func, NULL);

   rmethod = evas_render_method_lookup("buffer");
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rmethod == 0, NULL);

   ee = calloc(1, sizeof(Ecore_Evas));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   bdata = calloc(1, sizeof(Ecore_Evas_Engine_Buffer_Data));
   if (!bdata)
     {
	free(ee);
	return NULL;
     }

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_buffer_engine_func;
   ee->engine.data = bdata;
   bdata->alloc_func = alloc_func;
   bdata->free_func = free_func;
   bdata->data = (void *)data;

   ee->driver = "buffer";

   if (w < 1) w = 1;
   if (h < 1) h = 1;
   ee->rotation = 0;
   ee->visible = 1;
   ee->w = w;
   ee->h = h;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->profile_supported = 1;

   if (getenv("ECORE_EVAS_FORCE_SYNC_RENDER"))
     ee->can_async_render = 0;
   else
     ee->can_async_render = 1;

   ee->prop.max.w = 0;
   ee->prop.max.h = 0;
   ee->prop.layer = 0;
   ee->prop.borderless = EINA_TRUE;
   ee->prop.override = EINA_TRUE;
   ee->prop.maximized = EINA_TRUE;
   ee->prop.fullscreen = EINA_FALSE;
   ee->prop.withdrawn = EINA_FALSE;
   ee->prop.sticky = EINA_FALSE;

   /* init evas here */
   if (!ecore_evas_evas_new(ee, w, h))
     {
        ERR("Can not create a Canvas.");
        ecore_evas_free(ee);
        return NULL;
     }

   evas_output_method_set(ee->evas, rmethod);

   bdata->pixels = bdata->alloc_func(bdata->data, w * h * sizeof(int));

   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_RGB32;
        einfo->info.dest_buffer = bdata->pixels;
        einfo->info.dest_buffer_row_bytes = ee->w * sizeof(int);
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
     }
   else
     {
        ERR("evas_engine_info_set() init engine '%s' failed.", ee->driver);
        ecore_evas_free(ee);
        return NULL;
     }
   evas_key_modifier_add(ee->evas, "Shift");
   evas_key_modifier_add(ee->evas, "Control");
   evas_key_modifier_add(ee->evas, "Alt");
   evas_key_modifier_add(ee->evas, "Meta");
   evas_key_modifier_add(ee->evas, "Hyper");
   evas_key_modifier_add(ee->evas, "Super");
   evas_key_lock_add(ee->evas, "Caps_Lock");
   evas_key_lock_add(ee->evas, "Num_Lock");
   evas_key_lock_add(ee->evas, "Scroll_Lock");

   if (!_ecore_evas_cursors_init(ee))
     {
        ERR("Could not init the Ecore Evas cursors");
        ecore_evas_free(ee);
        return NULL;
     }
   evas_event_feed_mouse_in(ee->evas, 0, NULL);

   _ecore_evas_register(ee);

   evas_event_feed_mouse_in(ee->evas, (unsigned int)((unsigned long long)(ecore_time_get() * 1000.0) & 0xffffffff), NULL);
   _ecore_evas_focus_device_set(ee, NULL, EINA_TRUE);

   return ee;
}

/**
 * @brief Creates a new Ecore_Evas backed by a pixel buffer using default memory allocators.
 *
 * This function creates an Ecore_Evas instance that renders to an in-memory
 * pixel buffer. It uses standard `malloc` and `free` for memory management
 * of the pixel data.
 *
 * @param w The initial width of the buffer.
 * @param h The initial height of the buffer.
 * @return A new Ecore_Evas handle on success, NULL on failure.
 * @see ecore_evas_buffer_allocfunc_new() for using custom allocators.
 */
EAPI Ecore_Evas *
ecore_evas_buffer_new(int w, int h)
{
   Ecore_Evas *ee;

   ecore_event_evas_init();

   ee =
     ecore_evas_buffer_allocfunc_new(w, h, _ecore_evas_buffer_pix_alloc,
                                     _ecore_evas_buffer_pix_free, NULL);

   if (!ee)
     {
        ecore_event_evas_shutdown();
        return NULL;
     }

   ecore_evas_done(ee, EINA_TRUE);

   return ee;
}

/**
 * @brief Retrieves a direct pointer to the pixel data of an Ecore_Evas buffer.
 *
 * This function first ensures that the Ecore_Evas buffer is fully rendered
 * and up-to-date by calling ecore_evas_render() and ecore_evas_render_wait().
 * It then returns a constant pointer to the raw pixel data.
 * The format of the pixel data depends on whether the Ecore_Evas buffer
 * has an alpha channel (ARGB32) or not (RGB32, though typically still 4 bytes per pixel
 * with the alpha byte unused or set to opaque).
 *
 * @warning The returned pointer is valid only until the next resize or
 *          rendering operation that might reallocate the buffer. Do not store
 *          this pointer for long-term use.
 *
 * @param ee The Ecore_Evas buffer instance.
 * @return A constant void pointer to the pixel data, or NULL if `ee` is NULL.
 */
EAPI const void *
ecore_evas_buffer_pixels_get(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Buffer_Data *bdata;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   bdata = ee->engine.data;
   ecore_evas_render(ee);
   ecore_evas_render_wait(ee);
   return bdata->pixels;
}

/**
 * @brief Retrieves the parent Ecore_Evas of an Ecore_Evas buffer that is used as an Evas image source.
 *
 * If the given Ecore_Evas buffer (`ee`) was created via
 * `ecore_evas_object_image_new()`, it is associated with an Evas_Object image
 * that belongs to a target (parent) Ecore_Evas. This function returns
 * that parent Ecore_Evas.
 *
 * @param ee The Ecore_Evas buffer instance (presumably created from an Evas_Object image).
 * @return The parent Ecore_Evas instance if `ee` is an image-backed buffer and
 *         has a parent, otherwise NULL. Returns NULL if `ee` is NULL.
 */
EAPI Ecore_Evas *
ecore_evas_buffer_ecore_evas_parent_get(Ecore_Evas *ee)
{
   Ecore_Evas_Engine_Buffer_Data *bdata;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   bdata = ee->engine.data;
   return evas_object_data_get(bdata->image, "Ecore_Evas_Parent");
}

/**
 * @brief Creates a new Evas image object that is rendered by an Ecore_Evas buffer.
 *
 * This function sets up a special Ecore_Evas instance that renders its content
 * into an Evas image object. This image object can then be added to another
 * Evas canvas (managed by `ee_target`). This allows embedding one Evas scene
 * (from the new Ecore_Evas buffer) as an image within another.
 *
 * The returned Evas_Object is an image. The Ecore_Evas that renders to this
 * image can be retrieved using `evas_object_data_get(o, "Ecore_Evas")`.
 *
 * Event propagation (mouse, key, focus, etc.) from the `ee_target`'s canvas
 * to the embedded Ecore_Evas buffer is handled automatically.
 *
 * @param ee_target The target Ecore_Evas whose Evas canvas will contain the new image object.
 * @return A new Evas_Object (image type) on success, NULL on failure.
 *         The Evas_Object's data "Ecore_Evas" will point to the Ecore_Evas
 *         instance that draws into this image.
 *         The Evas_Object's data "Ecore_Evas_Parent" will point to `ee_target`.
 */
EAPI Evas_Object *
ecore_evas_object_image_new(Ecore_Evas *ee_target)
{
   Evas_Object *o;
   Ecore_Evas_Engine_Buffer_Data *bdata;
   Evas_Engine_Info_Buffer *einfo;
   Ecore_Evas *ee;
   int rmethod;
   int w = 1, h = 1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ee_target, NULL);

   rmethod = evas_render_method_lookup("buffer");
   EINA_SAFETY_ON_TRUE_RETURN_VAL(rmethod == 0, NULL);

   ee = calloc(1, sizeof(Ecore_Evas));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ee, NULL);

   bdata = calloc(1, sizeof(Ecore_Evas_Engine_Buffer_Data));
   if (!bdata)
     {
	free(ee);
	return NULL;
     }

   ee->engine.data = bdata;

   o = evas_object_image_add(ee_target->evas);
   evas_object_image_content_hint_set(o, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, 0);
   evas_object_image_size_set(o, w, h);

   ECORE_MAGIC_SET(ee, ECORE_MAGIC_EVAS);

   ee->engine.func = (Ecore_Evas_Engine_Func *)&_ecore_buffer_engine_func;

   ee->driver = "buffer";

   ee->rotation = 0;
   ee->visible = 0;
   ee->w = w;
   ee->h = h;
   ee->req.w = ee->w;
   ee->req.h = ee->h;
   ee->profile_supported = 1;
   ee->can_async_render = 0;

   ee->prop.max.w = 0;
   ee->prop.max.h = 0;
   ee->prop.layer = 0;
   ee->prop.borderless = EINA_TRUE;
   ee->prop.override = EINA_TRUE;
   ee->prop.maximized = EINA_FALSE;
   ee->prop.fullscreen = EINA_FALSE;
   ee->prop.withdrawn = EINA_TRUE;
   ee->prop.sticky = EINA_FALSE;

   /* init evas here */
   ee->evas = evas_new();
   evas_data_attach_set(ee->evas, ee);
   evas_output_method_set(ee->evas, rmethod);
   evas_output_size_set(ee->evas, w, h);
   evas_output_viewport_set(ee->evas, 0, 0, w, h);
   evas_event_callback_add(ee->evas, EVAS_CALLBACK_RENDER_POST, _ecore_evas_buffer_update_image, ee);

   bdata->image = o;
   evas_object_data_set(bdata->image, "Ecore_Evas", ee);
   evas_object_data_set(bdata->image, "Ecore_Evas_Parent", ee_target);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_IN,
                                  _ecore_evas_buffer_cb_mouse_in, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_OUT,
                                  _ecore_evas_buffer_cb_mouse_out, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _ecore_evas_buffer_cb_mouse_down, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_UP,
                                  _ecore_evas_buffer_cb_mouse_up, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_MOVE,
                                  _ecore_evas_buffer_cb_mouse_move, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MOUSE_WHEEL,
                                  _ecore_evas_buffer_cb_mouse_wheel, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MULTI_DOWN,
                                  _ecore_evas_buffer_cb_multi_down, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MULTI_UP,
                                  _ecore_evas_buffer_cb_multi_up, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_MULTI_MOVE,
                                  _ecore_evas_buffer_cb_multi_move, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_FREE,
                                  _ecore_evas_buffer_cb_free, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_KEY_DOWN,
                                  _ecore_evas_buffer_cb_key_down, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_KEY_UP,
                                  _ecore_evas_buffer_cb_key_up, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_FOCUS_IN,
                                  _ecore_evas_buffer_cb_focus_in, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_FOCUS_OUT,
                                  _ecore_evas_buffer_cb_focus_out, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_SHOW,
                                  _ecore_evas_buffer_cb_show, ee);
   evas_object_event_callback_add(bdata->image,
                                  EVAS_CALLBACK_HIDE,
                                  _ecore_evas_buffer_cb_hide, ee);
   einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(ee->evas);
   if (einfo)
     {
        bdata->pixels = evas_object_image_data_get(o, 1);
        einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
        einfo->info.dest_buffer = bdata->pixels;
        einfo->info.dest_buffer_row_bytes = evas_object_image_stride_get(o);
        einfo->info.use_color_key = 0;
        einfo->info.alpha_threshold = 0;
        einfo->info.func.new_update_region = NULL;
        einfo->info.func.free_update_region = NULL;
        evas_object_image_data_set(o, bdata->pixels);
        if (!evas_engine_info_set(ee->evas, (Evas_Engine_Info *)einfo))
          {
             ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
             ecore_evas_free(ee);
             return NULL;
          }
     }
   else
     {
        ERR("evas_engine_info_set() for engine '%s' failed.", ee->driver);
        ecore_evas_free(ee);
        return NULL;
     }
   evas_key_modifier_add(ee->evas, "Shift");
   evas_key_modifier_add(ee->evas, "Control");
   evas_key_modifier_add(ee->evas, "Alt");
   evas_key_modifier_add(ee->evas, "Meta");
   evas_key_modifier_add(ee->evas, "Hyper");
   evas_key_modifier_add(ee->evas, "Super");
   evas_key_lock_add(ee->evas, "Caps_Lock");
   evas_key_lock_add(ee->evas, "Num_Lock");
   evas_key_lock_add(ee->evas, "Scroll_Lock");

   if (!_ecore_evas_cursors_init(ee))
     {
        ERR("Could not init the Ecore Evas cursors");
        ecore_evas_free(ee);
        return NULL;
     }

   _ecore_evas_subregister(ee_target, ee);
   ecore_event_evas_init();
   return o;
}
