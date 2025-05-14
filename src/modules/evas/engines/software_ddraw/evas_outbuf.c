#include "evas_common_private.h"
#include "evas_engine.h"

/**
 * @brief Initializes the DirectDraw output buffer system.
 *
 * This function is called once to set up any necessary global state for
 * DirectDraw output buffers. Currently, it does nothing.
 */
void
evas_software_ddraw_outbuf_init(void)
{
}

/**
 * @brief Frees the resources associated with an output buffer.
 *
 * @param buf The output buffer to free.
 *
 * Shuts down the DirectDraw components associated with the buffer and
 * frees the memory allocated for the Outbuf structure.
 */
void
evas_software_ddraw_outbuf_free(Outbuf *buf)
{
   if (!buf)
     return;

   evas_software_ddraw_shutdown(buf);
   free(buf);
}

/**
 * @brief Sets up a new DirectDraw output buffer.
 *
 * @param width The desired width of the buffer.
 * @param height The desired height of the buffer.
 * @param rotation The rotation angle (0, 90, 180, 270).
 * @param window The handle to the window (HWND) where the buffer will be displayed.
 * @param fullscreen Non-zero if fullscreen mode is requested, 0 otherwise.
 * @return A pointer to the newly created Outbuf structure, or NULL on failure.
 *
 * This function allocates and initializes an Outbuf structure, sets up the
 * DirectDraw environment for the specified window and dimensions, and checks
 * if a suitable color conversion function is available for the detected
 * screen format.
 */
Outbuf *
evas_software_ddraw_outbuf_setup(int          width,
                                 int          height,
                                 int          rotation,
                                 HWND         window,
                                 int          fullscreen)
{
   Outbuf *buf;

   buf = (Outbuf *)calloc(1, sizeof(Outbuf));
   if (!buf)
      return NULL;

   buf->width = width;
   buf->height = height;
   buf->rot = rotation;

   if (!evas_software_ddraw_init(window, fullscreen, buf))
     {
        free(buf);
        return NULL;
     }

   {
      Gfx_Func_Convert  conv_func;
      DD_Output_Buffer *ddob;

      ddob = evas_software_ddraw_output_buffer_new(1, 1, NULL);

      conv_func = NULL;
      if (ddob)
        {
           if (evas_software_ddraw_masks_get(buf))
             {
                if ((rotation == 0) || (rotation == 180))
                  conv_func = evas_common_convert_func_get(0,
                                                           width,
                                                           height,
                                                           32,
                                                           buf->priv.mask.r,
                                                           buf->priv.mask.g,
                                                           buf->priv.mask.b,
                                                           PAL_MODE_NONE,
                                                           rotation);
                else if ((rotation == 90) || (rotation == 270))
                  conv_func = evas_common_convert_func_get(0,
                                                           height,
                                                           width,
                                                           32,
                                                           buf->priv.mask.r,
                                                           buf->priv.mask.g,
                                                           buf->priv.mask.b,
                                                           PAL_MODE_NONE,
                                                           rotation);
             }

           evas_software_ddraw_output_buffer_free(ddob);

           if (!conv_func)
             {
                ERR("DDraw engine Error"
                    " {"
                    "  At depth         32:"
                    "  RGB format mask: %08x, %08x, %08x"
                    "  Not supported by and compiled in converters!"
                    " }",
                    buf->priv.mask.r,
                    buf->priv.mask.g,
                    buf->priv.mask.b);
             }
        }
   }

   return buf;
}

/**
 * @brief Reconfigures an existing output buffer.
 *
 * @param buf The output buffer to reconfigure.
 * @param width The new width.
 * @param height The new height.
 * @param rotation The new rotation angle (0, 90, 180, 270).
 * @param depth The new color depth (currently unused).
 *
 * Updates the dimensions and rotation of the output buffer. If the dimensions
 * or rotation have changed, it triggers a resize of the underlying DirectDraw
 * surface.
 */
void
evas_software_ddraw_outbuf_reconfigure(Outbuf      *buf,
                                       int          width,
                                       int          height,
                                       int          rotation,
                                       Outbuf_Depth depth EINA_UNUSED)
{
   /* Avoid unnecessary resizing if configuration hasn't changed */
   if ((width == buf->width) && (height == buf->height) &&
       (rotation == buf->rot))
     return;
   buf->width = width;
   buf->height = height;
   buf->rot = rotation;
   evas_software_ddraw_surface_resize(buf);
}

/**
 * @brief Creates a new image buffer for updating a region of the output buffer.
 *
 * @param buf The output buffer.
 * @param x The x-coordinate of the region to update.
 * @param y The y-coordinate of the region to update.
 * @param w The width of the region to update.
 * @param h The height of the region to update.
 * @param[out] cx Pointer to store the relative x-coordinate within the returned image (always 0).
 * @param[out] cy Pointer to store the relative y-coordinate within the returned image (always 0).
 * @param[out] cw Pointer to store the width of the returned image (same as w).
 * @param[out] ch Pointer to store the height of the returned image (same as h).
 * @return A pointer to an RGBA_Image structure representing the update region, or NULL on failure.
 *
 * This function allocates an RGBA_Image buffer that Evas can render into.
 * It might reuse DirectDraw buffers directly if the format and rotation allow,
 * otherwise, it allocates a separate buffer. The returned image is added to
 * the list of pending writes for the output buffer.
 * The `obr` (Outbuf_Region) structure stored in `im->extended_info` holds
 * metadata about the update region, including the associated DirectDraw buffer (`ddob`).
 */
void *
evas_software_ddraw_outbuf_new_region_for_update(Outbuf *buf,
                                                 int     x,
                                                 int     y,
                                                 int     w,
                                                 int     h,
                                                 int    *cx,
                                                 int    *cy,
                                                 int    *cw,
                                                 int    *ch)
{
   RGBA_Image    *im;
   Outbuf_Region *obr;
   int            bpl = 0;
   int            alpha = 0;

   obr = calloc(1, sizeof(Outbuf_Region));
   obr->x = x;
   obr->y = y;
   obr->width = w;
   obr->height = h;
   *cx = 0;
   *cy = 0;
   *cw = w;
   *ch = h;

   if ((buf->rot == 0) &&
       (buf->priv.mask.r == 0xff0000) &&
       (buf->priv.mask.g == 0x00ff00) &&
       (buf->priv.mask.b == 0x0000ff))
     {
        obr->ddob = evas_software_ddraw_output_buffer_new(w, h, NULL);
        im = (RGBA_Image *)evas_cache_image_data(evas_common_image_cache_get(),
                                                 w, h,
                                                 (DATA32 *) evas_software_ddraw_output_buffer_data(obr->ddob, &bpl),
                                                 alpha, EVAS_COLORSPACE_ARGB8888);
        im->extended_info = obr;
     }
   else
     {
        im = (RGBA_Image *) evas_cache_image_empty(evas_common_image_cache_get());
        im->cache_entry.flags.alpha |= alpha ? 1 : 0;
        evas_cache_image_surface_alloc(&im->cache_entry, w, h);
        im->extended_info = obr;
        if ((buf->rot == 0) || (buf->rot == 180))
          obr->ddob = evas_software_ddraw_output_buffer_new(w, h, NULL);
        else if ((buf->rot == 90) || (buf->rot == 270))
          obr->ddob = evas_software_ddraw_output_buffer_new(h, w, NULL);
     }

   buf->priv.pending_writes = eina_list_append(buf->priv.pending_writes, im);
   buf->priv.pending_writes = eina_list_append(buf->priv.pending_writes, im);
   return im;
}

/**
 * @brief Processes an updated region before flushing.
 *
 * @param buf The output buffer.
 * @param update The RGBA_Image containing the updated pixel data.
 * @param x The x-coordinate of the updated region.
 * @param y The y-coordinate of the updated region.
 * @param w The width of the updated region.
 * @param h The height of the updated region.
 *
 * This function takes the pixel data from the `update` image (which was
 * previously obtained from evas_software_ddraw_outbuf_new_region_for_update)
 * and prepares it for drawing onto the DirectDraw surface. This involves:
 * 1. Finding the appropriate color conversion function based on buffer properties.
 * 2. Calculating the target coordinates (`obr->x`, `obr->y`) on the DirectDraw
 *    surface based on the buffer's rotation.
 * 3. If necessary (i.e., if the update image data is not already in the
 *    DirectDraw buffer), performing the color conversion from the source
 *    RGBA data to the DirectDraw buffer associated with the Outbuf_Region (`obr->ddob`).
 */
void
evas_software_ddraw_outbuf_push_updated_region(Outbuf     *buf,
                                               RGBA_Image *update,
                                               int        x,
                                               int        y,
                                               int        w,
                                               int        h)
{
   Gfx_Func_Convert    conv_func;
   Outbuf_Region      *obr;
   DATA32             *src_data;
   void               *data;
   int                 bpl = 0;

   conv_func = NULL;
   obr = update->extended_info;

   if ((buf->rot == 0) || (buf->rot == 180))
     conv_func = evas_common_convert_func_get(0, w, h,
                                              32,
                                              buf->priv.mask.r,
                                              buf->priv.mask.g,
                                              buf->priv.mask.b,
                                              PAL_MODE_NONE,
                                              buf->rot);
   else if ((buf->rot == 90) || (buf->rot == 270))
     conv_func = evas_common_convert_func_get(0, h, w,
                                              32,
                                              buf->priv.mask.r,
                                              buf->priv.mask.g,
                                              buf->priv.mask.b,
                                              PAL_MODE_NONE, buf->rot);
   if (!conv_func) return;

   data = evas_software_ddraw_output_buffer_data(obr->ddob, &bpl);
   src_data = update->image.data;
   if (buf->rot == 0)
     {
	obr->x = x;
	obr->y = y;
     }
   else if (buf->rot == 90)
     {
	obr->x = y;
	obr->y = buf->width - x - w;
     }
   else if (buf->rot == 180)
     {
	obr->x = buf->width - x - w;
	obr->y = buf->height - y - h;
     }
   else if (buf->rot == 270)
     {
	obr->x = buf->height - y - h;
	obr->y = x;
     }
   if ((buf->rot == 0) || (buf->rot == 180))
     {
	obr->width = w;
	obr->height = h;
     }
   else if ((buf->rot == 90) || (buf->rot == 270))
     {
	obr->width = h;
	obr->height = w;
     }

   if (data != src_data)
     conv_func(src_data, data,
               0,
               bpl / 4 - obr->width,
               obr->width,
               obr->height,
               x,
               y,
               NULL);
               NULL);
}

/**
 * @brief Flushes all pending updates to the screen.
 *
 * @param buf The output buffer.
 * @param surface_damage Regions damaged on the surface (unused).
 * @param buffer_damage Regions damaged in the buffer (unused).
 * @param render_mode The rendering mode (sync/async).
 *
 * This function takes all the processed update regions (stored in
 * `buf->priv.pending_writes`), copies them to the DirectDraw back buffer,
 * and then flips the back buffer to the primary surface, making the updates
 * visible. It also manages the lifecycle of the update images and associated
 * DirectDraw buffers.
 * If render_mode is EVAS_RENDER_MODE_ASYNC_INIT, it returns immediately.
 */
void
evas_software_ddraw_outbuf_flush(Outbuf *buf, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   Eina_List *l;
   RGBA_Image       *im;
   Outbuf_Region    *obr;
   void      *ddraw_data;
   int        ddraw_width;
   int        ddraw_height;
   int        ddraw_pitch;

   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   /* lock the back surface */
   if (!(ddraw_data = evas_software_ddraw_lock(buf,
                                               &ddraw_width,
                                               &ddraw_height,
                                               &ddraw_pitch)))
     goto free_images;

   /* copy safely the images that need to be drawn onto the back surface */
   EINA_LIST_FOREACH(buf->priv.pending_writes, l, im)
     {
	DD_Output_Buffer *ddob;

        obr = im->extended_info;
        ddob = obr->ddob;
        evas_software_ddraw_output_buffer_paste(ddob,
                                                ddraw_data,
                                                ddraw_width,
                                                ddraw_height,
                                                ddraw_pitch,
						obr->x,
						obr->y);
     }

   /* unlock the back surface and flip the surface */
   evas_software_ddraw_unlock_and_flip(buf);

 free_images:
   while (buf->priv.prev_pending_writes)
     {
        im = buf->priv.prev_pending_writes->data;
        buf->priv.prev_pending_writes =
          eina_list_remove_list(buf->priv.prev_pending_writes,
                                buf->priv.prev_pending_writes);
        obr = im->extended_info;
        evas_cache_image_drop(&im->cache_entry);
        if (obr->ddob)
          evas_software_ddraw_output_buffer_free(obr->ddob);
        free(obr);
     }
   buf->priv.prev_pending_writes = buf->priv.pending_writes;
   buf->priv.pending_writes = NULL;

   evas_common_cpu_end_opt();
   evas_common_cpu_end_opt();
}

/**
 * @brief Frees resources associated with previously flushed updates during idle time.
 *
 * @param buf The output buffer.
 *
 * This function cleans up the image buffers and DirectDraw buffers that were
 * used in the *previous* flush operation (`buf->priv.prev_pending_writes`).
 * It's typically called when the application is idle to release memory.
 */
void
evas_software_ddraw_outbuf_idle_flush(Outbuf *buf)
{
   /* Free images and buffers from the previous flush cycle */
   while (buf->priv.prev_pending_writes)
     {
        RGBA_Image *im;
        Outbuf_Region *obr;

        im = buf->priv.prev_pending_writes->data;
        buf->priv.prev_pending_writes =
          eina_list_remove_list(buf->priv.prev_pending_writes,
                                buf->priv.prev_pending_writes);
        obr = im->extended_info;
        evas_cache_image_drop((Image_Entry *)im);
        if (obr->ddob)
          evas_software_ddraw_output_buffer_free(obr->ddob);
        free(obr);
     }
}

/**
 * @brief Gets the width of the output buffer.
 * @param buf The output buffer.
 * @return The width in pixels.
 */
int
evas_software_ddraw_outbuf_width_get(Outbuf *buf)
{
   return buf->width;
}

/**
 * @brief Gets the height of the output buffer.
 * @param buf The output buffer.
 * @return The height in pixels.
 */
int
evas_software_ddraw_outbuf_height_get(Outbuf *buf)
{
   return buf->height;
}

/**
 * @brief Gets the rotation angle of the output buffer.
 * @param buf The output buffer.
 * @return The rotation angle (0, 90, 180, or 270).
 */
int
evas_software_ddraw_outbuf_rot_get(Outbuf *buf)
{
   return buf->rot;
}

/* vim:ts=8 sw=3 expandtab cindent */
