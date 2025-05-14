#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_engine.h"

/**
 * @brief Initializes the buffer outbuf system.
 *
 * This function is called to set up any necessary global state for the
 * buffer outbuf operations. Currently, it's a no-op.
 */
void
evas_buffer_outbuf_buf_init(void)
{
}

/**
 * @brief Frees an Outbuf structure.
 *
 * Releases the resources associated with an Outbuf, including its
 * private back buffer if one exists.
 *
 * @param buf The Outbuf to free.
 */
void
evas_buffer_outbuf_buf_free(Outbuf *buf)
{
   if (buf->priv.back_buf)
     evas_cache_image_drop(&buf->priv.back_buf->cache_entry);
   free(buf);
}

/**
 * @brief Updates an existing framebuffer Outbuf with new parameters.
 *
 * This function reconfigures an existing Outbuf, typically used for a
 * framebuffer, with new dimensions, depth, destination buffer, and
 * callback functions. It also handles the creation or update of an
 * internal back buffer if the destination buffer is directly usable.
 *
 * @param buf The Outbuf to update.
 * @param w The new width of the buffer.
 * @param h The new height of the buffer.
 * @param depth The new color depth of the buffer.
 * @param dest Pointer to the destination memory for rendering.
 * @param dest_row_bytes The number of bytes per row in the destination memory.
 * @param use_color_key Non-zero if color keying should be used.
 * @param color_key The color key value if use_color_key is active.
 * @param alpha_level The global alpha level to apply.
 * @param new_update_region Callback to get a memory region for updates.
 *                          Example: `my_new_update_region(x, y, w, h, &row_bytes)`
 *                          returns `void*` to the region.
 * @param free_update_region Callback to free a memory region after updates.
 *                           Example: `my_free_update_region(x, y, w, h, data)`
 * @param switch_buffer Callback to switch buffers (for double buffering).
 *                      Example: `my_switch_buffer(switch_data, current_dest_buffer)`
 *                      returns `void*` to the new destination buffer.
 * @param switch_data User data for the switch_buffer callback.
 */
void
evas_buffer_outbuf_buf_update_fb(Outbuf *buf, int w, int h, Outbuf_Depth depth, void *dest, int dest_row_bytes, int use_color_key, DATA32 color_key, int alpha_level,
                                void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes),
                                void   (*free_update_region) (int x, int y, int w, int h, void *data),
                                void * (*switch_buffer) (void *data, void *dest_buffer),
                                void *switch_data)
{
   buf->w = w;
   buf->h = h;
   buf->depth = depth;

   buf->dest = dest;
   buf->dest_row_bytes = dest_row_bytes;

   buf->alpha_level = alpha_level;
   buf->color_key = color_key;
   buf->use_color_key = use_color_key;
   buf->first_frame = 1;

   buf->func.new_update_region = new_update_region;
   buf->func.free_update_region = free_update_region;
   buf->func.switch_buffer = switch_buffer;
   buf->switch_data = switch_data;

   if ((buf->depth == OUTBUF_DEPTH_ARGB_32BPP_8888_8888) &&
       (buf->dest) && (buf->dest_row_bytes == (buf->w * sizeof(DATA32))))
     {
        memset(buf->dest, 0, h * buf->dest_row_bytes);
        if (buf->priv.back_buf) evas_cache_image_drop(&buf->priv.back_buf->cache_entry);
        buf->priv.back_buf =
          (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                               w, h, buf->dest,
                                               1, EVAS_COLORSPACE_ARGB8888);
     }
   else if ((buf->depth == OUTBUF_DEPTH_RGB_32BPP_888_8888) &&
       (buf->dest) && (buf->dest_row_bytes == (buf->w * sizeof(DATA32))))
     {
        if (buf->priv.back_buf) evas_cache_image_drop(&buf->priv.back_buf->cache_entry);
        buf->priv.back_buf =
          (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                               w, h, buf->dest,
                                               0, EVAS_COLORSPACE_ARGB8888);
     }
}

/**
 * @brief Sets up a new framebuffer Outbuf.
 *
 * Allocates and initializes a new Outbuf structure for framebuffer rendering
 * with the specified parameters and callback functions.
 *
 * @param w The width of the buffer.
 * @param h The height of the buffer.
 * @param depth The color depth of the buffer.
 * @param dest Pointer to the destination memory for rendering.
 * @param dest_row_bytes The number of bytes per row in the destination memory.
 * @param use_color_key Non-zero if color keying should be used.
 * @param color_key The color key value if use_color_key is active.
 * @param alpha_level The global alpha level to apply.
 * @param new_update_region Callback to get a memory region for updates.
 * @param free_update_region Callback to free a memory region after updates.
 * @param switch_buffer Callback to switch buffers (for double buffering).
 * @param switch_data User data for the switch_buffer callback.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *
evas_buffer_outbuf_buf_setup_fb(int w, int h, Outbuf_Depth depth, void *dest, int dest_row_bytes, int use_color_key, DATA32 color_key, int alpha_level,
                                void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes),
                                void   (*free_update_region) (int x, int y, int w, int h, void *data),
                                void * (*switch_buffer) (void *data, void *dest_buffer),
                                void *switch_data)
{
   Outbuf *buf;

   buf = calloc(1, sizeof(Outbuf));
   if (!buf) return NULL;

   evas_buffer_outbuf_buf_update_fb(buf,
                                    w,
                                    h,
                                    depth,
                                    dest,
                                    dest_row_bytes,
                                    use_color_key,
                                    color_key,
                                    alpha_level,
                                    new_update_region,
                                    free_update_region,
                                    switch_buffer,
                                    switch_data);

   return buf;
}

/**
 * @brief Gets a new region for update.
 *
 * Provides a memory region (RGBA_Image) suitable for rendering an update.
 * If the Outbuf has a direct back buffer, it returns that buffer and sets
 * cx, cy, cw, ch to the requested x, y, w, h. Otherwise, it allocates a
 * new temporary RGBA_Image from the cache, sized w, h, and sets cx, cy to 0, 0
 * and cw, ch to w, h.
 *
 * @param buf The Outbuf.
 * @param x The x-coordinate of the desired update region.
 * @param y The y-coordinate of the desired update region.
 * @param w The width of the desired update region.
 * @param h The height of the desired update region.
 * @param[out] cx The x-coordinate of the returned image region.
 * @param[out] cy The y-coordinate of the returned image region.
 * @param[out] cw The width of the returned image region.
 * @param[out] ch The height of the returned image region.
 * @return A pointer to an RGBA_Image for updating, or NULL on failure.
 *         The caller should not free this image directly; use
 *         evas_buffer_outbuf_buf_free_region_for_update.
 */
void *
evas_buffer_outbuf_buf_new_region_for_update(Outbuf *buf, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch)
{
   RGBA_Image *im;

   if (buf->priv.back_buf)
     {
	*cx = x; *cy = y; *cw = w; *ch = h;
	return buf->priv.back_buf;
     }
   else
     {
	*cx = 0; *cy = 0; *cw = w; *ch = h;
	im = (RGBA_Image *) evas_cache_image_empty(evas_common_image_cache_get());
        if (im)
          {
	     if (((buf->depth == OUTBUF_DEPTH_ARGB_32BPP_8888_8888)) ||
		 ((buf->depth == OUTBUF_DEPTH_BGRA_32BPP_8888_8888)))
	       {
		  im->cache_entry.flags.alpha = 1;
               }

	       im = (RGBA_Image *) evas_cache_image_size_set(&im->cache_entry, w, h);
          }
     }
   return im;
}

/**
 * @brief Frees a region obtained for update.
 *
 * Releases an RGBA_Image that was acquired using
 * evas_buffer_outbuf_buf_new_region_for_update. If the image was a
 * temporary buffer (not the Outbuf's direct back_buf), it's returned
 * to the image cache.
 *
 * @param buf The Outbuf.
 * @param update The RGBA_Image to free/release.
 */
void
evas_buffer_outbuf_buf_free_region_for_update(Outbuf *buf, RGBA_Image *update)
{
   if (update != buf->priv.back_buf)
     evas_cache_image_drop(&update->cache_entry);
}

/**
 * @brief Switches the output buffer, if double buffering is enabled.
 *
 * If a `switch_buffer` callback was provided during setup, this function
 * calls it to perform the buffer swap. It then updates the internal
 * destination pointer and, if a back buffer is used, re-associates it
 * with the new destination memory.
 *
 * @param buf The Outbuf.
 * @param surface_damage Regions damaged on the surface (unused).
 * @param buffer_damage Regions damaged in the buffer (unused).
 * @param render_mode The current render mode (unused).
 */
void
evas_buffer_outbuf_buf_switch_buffer(Outbuf *buf, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode EINA_UNUSED)
{
   if (buf->func.switch_buffer)
     {
        buf->dest = buf->func.switch_buffer(buf->switch_data, buf->dest);
        if (buf->priv.back_buf)
          {
             evas_cache_image_drop(&buf->priv.back_buf->cache_entry);
             buf->priv.back_buf =
               (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                                    buf->w, buf->h,
                                                    buf->dest,
                                                    buf->depth == OUTBUF_DEPTH_ARGB_32BPP_8888_8888 ? 1 : 0,
                                                    EVAS_COLORSPACE_ARGB8888);
          }
     }
}

/**
 * @brief Pushes an updated region to the output buffer.
 *
 * Copies data from the source `update` RGBA_Image to the Outbuf's
 * destination memory, performing color space conversion, color keying,
 * and alpha blending as configured for the Outbuf. It handles various
 * output depths. If `new_update_region` and `free_update_region` callbacks
 * are set, it uses them to get/free the destination memory for the update.
 *
 * @param buf The Outbuf.
 * @param update The RGBA_Image containing the updated pixel data.
 *               The image data is assumed to be in ARGB8888 format.
 *               Example structure of `update->image.data` for a 2x2 image:
 *               `[R1G1B1A1, R2G1B1A1, R1G2B1A1, R2G2B1A1]` where each element is a DATA32.
 * @param x The x-coordinate of the update region in the destination.
 * @param y The y-coordinate of the update region in the destination.
 * @param w The width of the update region.
 * @param h The height of the update region.
 */
void
evas_buffer_outbuf_buf_push_updated_region(Outbuf *buf, RGBA_Image *update, int x, int y, int w, int h)
{
   /* copy update image to out buf & convert */
   switch (buf->depth)
     {
      case OUTBUF_DEPTH_RGB_24BPP_888_888:
	/* copy & pack into 24bpp - if colorkey is enabled... etc. */
	  {
	     DATA8 thresh;
	     int xx, yy;
	     int row_bytes;
	     DATA8 *dest;
	     DATA32 colorkey;
	     DATA32 *src;
	     DATA8 *dst;

	     colorkey = buf->color_key;
	     thresh = buf->alpha_level;
	     row_bytes = buf->dest_row_bytes;
	     dest = (DATA8 *)(buf->dest) + (y * row_bytes) + (x * 3);
	     if (buf->func.new_update_region)
	       {
		  dest = buf->func.new_update_region(x, y, w, h, &row_bytes);
	       }
	     if (!dest) break;
	     if (buf->use_color_key)
	       {
		  for (yy = 0; yy < h; yy++)
		    {
		       dst = dest + (yy * row_bytes);
		       src = update->image.data + (yy * update->cache_entry.w);
		       for (xx = 0; xx < w; xx++)
			 {
			    if (A_VAL(src) > thresh)
			      {
				 *dst++ = R_VAL(src);
				 *dst++ = G_VAL(src);
				 *dst++ = B_VAL(src);
			      }
			    else
			      {
				 *dst++ = R_VAL(&colorkey);
				 *dst++ = G_VAL(&colorkey);
				 *dst++ = B_VAL(&colorkey);
			      }
			    src++;
			 }
		    }
	       }
	     else
	       {
		  for (yy = 0; yy < h; yy++)
		    {
		       dst = dest + (yy * row_bytes);
		       src = update->image.data + (yy * update->cache_entry.w);
		       for (xx = 0; xx < w; xx++)
			 {
			    *dst++ = R_VAL(src);
			    *dst++ = G_VAL(src);
			    *dst++ = B_VAL(src);
			    src++;
			 }
		    }
	       }
	     if (buf->func.free_update_region)
	       {
		  buf->func.free_update_region(x, y, w, h, dest);
	       }
	  }
	break;
      case OUTBUF_DEPTH_BGR_24BPP_888_888:
	/* copy & pack into 24bpp - if colorkey is enabled... etc. */
	  {
	     DATA8 thresh;
	     int xx, yy;
	     int row_bytes;
	     DATA8 *dest;
	     DATA32 colorkey;
	     DATA32 *src;
	     DATA8 *dst;

	     colorkey = buf->color_key;
	     thresh = buf->alpha_level;
	     row_bytes = buf->dest_row_bytes;
	     dest = (DATA8 *)(buf->dest) + (y * row_bytes) + (x * 3);
	     if (buf->func.new_update_region)
	       {
		  dest = buf->func.new_update_region(x, y, w, h, &row_bytes);
	       }
	     if (!dest) break;
	     if (buf->use_color_key)
	       {
		  for (yy = 0; yy < h; yy++)
		    {
		       dst = dest + (yy * row_bytes);
		       src = update->image.data + (yy * update->cache_entry.w);
		       for (xx = 0; xx < w; xx++)
			 {
			    if (A_VAL(src) > thresh)
			      {
				 *dst++ = B_VAL(src);
				 *dst++ = G_VAL(src);
				 *dst++ = R_VAL(src);
			      }
			    else
			      {
				 *dst++ = B_VAL(&colorkey);
				 *dst++ = G_VAL(&colorkey);
				 *dst++ = R_VAL(&colorkey);
			      }
			    src++;
			 }
		    }
	       }
	     else
	       {
		  for (yy = 0; yy < h; yy++)
		    {
		       dst = dest + (yy * row_bytes);
		       src = update->image.data + (yy * update->cache_entry.w);
		       for (xx = 0; xx < w; xx++)
			 {
			    *dst++ = B_VAL(src);
			    *dst++ = G_VAL(src);
			    *dst++ = R_VAL(src);
			    src++;
			 }
		    }
	       }
	     if (buf->func.free_update_region)
	       {
		  buf->func.free_update_region(x, y, w, h, dest);
	       }
	  }
	break;
      case OUTBUF_DEPTH_RGB_32BPP_888_8888:
      case OUTBUF_DEPTH_ARGB_32BPP_8888_8888:
	  {
	     DATA32 *dest, *src, *dst;
	     int yy, row_bytes;

	     row_bytes = buf->dest_row_bytes;
	     dest = (DATA32 *)((DATA8 *)(buf->dest) + (y * row_bytes) + (x * 4));
	     if (buf->func.new_update_region)
	       {
		  dest = buf->func.new_update_region(x, y, w, h, &row_bytes);
	       }
	     /* no need src == dest */
	     if (!buf->priv.back_buf)
	       {
		  Gfx_Func_Copy func;

		  func = evas_common_draw_func_copy_get(w, 0);
		  if (func)
		    {
		       for (yy = 0; yy < h; yy++)
			 {
			    src = update->image.data + (yy * update->cache_entry.w);
			    dst = (DATA32 *)((DATA8 *)(buf->dest) + ((y + yy) * row_bytes));
			    func(src, dst, w);
			 }

		    }
	       }
	     if (buf->func.free_update_region)
	       {
		  buf->func.free_update_region(x, y, w, h, dest);
	       }
	  }
	break;
      case OUTBUF_DEPTH_BGR_32BPP_888_8888:
	  {
	     DATA32 *src, *dst;
	     DATA8 *dest;
	     int xx, yy, row_bytes;

	     row_bytes = buf->dest_row_bytes;
	     dest = (DATA8 *)(buf->dest) + (y * row_bytes) + (x * 4);
	     if (buf->func.new_update_region)
	       {
		  dest = buf->func.new_update_region(x, y, w, h, &row_bytes);
	       }
	     for (yy = 0; yy < h; yy++)
	       {
		  dst = (DATA32 *)(dest + (yy * row_bytes));
		  src = update->image.data + (yy * update->cache_entry.w);
		  for (xx = 0; xx < w; xx++)
		    {
		       A_VAL(dst) = B_VAL(src);
		       R_VAL(dst) = G_VAL(src);
		       G_VAL(dst) = R_VAL(src);
		       dst++;
		       src++;
		    }
	       }
	     if (buf->func.free_update_region)
	       {
		  buf->func.free_update_region(x, y, w, h, dest);
	       }
	 }
	break;
      case OUTBUF_DEPTH_BGRA_32BPP_8888_8888:
	  {
	     DATA32 *src, *dst;
	     DATA8 *dest;
	     int xx, yy, row_bytes;

	     row_bytes = buf->dest_row_bytes;
	     dest = (DATA8 *)(buf->dest) + (y * row_bytes) + (x * 4);
	     if (buf->func.new_update_region)
	       {
		  dest = buf->func.new_update_region(x, y, w, h, &row_bytes);
	       }
	     for (yy = 0; yy < h; yy++)
	       {
		  dst = (DATA32 *)(dest + (yy * row_bytes));
		  src = update->image.data + (yy * update->cache_entry.w);
		  for (xx = 0; xx < w; xx++)
		    {
		       A_VAL(dst) = B_VAL(src);
		       R_VAL(dst) = G_VAL(src);
		       G_VAL(dst) = R_VAL(src);
		       dst++;
		       src++;
		    }
	       }
	     if (buf->func.free_update_region)
	       {
		  buf->func.free_update_region(x, y, w, h, dest);
	       }
	 }
	break;
      default:
	break;
     }
}

/**
 * @brief Reconfigures an Outbuf with new dimensions and depth.
 *
 * This function updates the width, height, and optionally the depth of an
 * existing Outbuf. Other parameters like destination pointers and callbacks
 * are preserved from the existing Outbuf. It essentially calls
 * evas_buffer_outbuf_buf_update_fb with the new geometry and existing settings.
 *
 * @param ob The Outbuf to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The rotation (currently unused).
 * @param depth The new color depth. If OUTBUF_DEPTH_INHERIT, the existing
 *              depth is used.
 */
void
evas_buffer_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot EINA_UNUSED, Outbuf_Depth depth)
{
   void    *dest;
   int      dest_row_bytes;
   int      alpha_level;
   DATA32   color_key;
   char     use_color_key;
   void * (*new_update_region) (int x, int y, int w, int h, int *row_bytes);
   void   (*free_update_region) (int x, int y, int w, int h, void *data);
   void * (*switch_buffer) (void *switch_data, void *dest);
   void    *switch_data;

   if (depth == OUTBUF_DEPTH_INHERIT) depth = ob->depth;
   dest = ob->dest;
   dest_row_bytes = ob->dest_row_bytes;
   alpha_level = ob->alpha_level;
   color_key = ob->color_key;
   use_color_key = ob->use_color_key;
   new_update_region = ob->func.new_update_region;
   free_update_region = ob->func.free_update_region;
   switch_buffer = ob->func.switch_buffer;
   switch_data = ob->switch_data;

   evas_buffer_outbuf_buf_update_fb(ob,
                                    w,
                                    h,
                                    depth,
                                    dest,
                                    dest_row_bytes,
                                    use_color_key,
                                    color_key,
                                    alpha_level,
                                    new_update_region,
                                    free_update_region,
                                    switch_buffer,
                                    switch_data);
}

/**
 * @brief Gets the swap mode of the Outbuf.
 *
 * Determines if the Outbuf is configured for double buffering
 * (MODE_DOUBLE) or full repaint (MODE_FULL) based on the presence
 * of a `switch_buffer` callback.
 *
 * @param ob The Outbuf.
 * @return The swap mode (MODE_DOUBLE or MODE_FULL).
 */
Render_Output_Swap_Mode
evas_buffer_outbuf_buf_swap_mode_get(Outbuf *ob)
{
   if (ob->func.switch_buffer) return MODE_DOUBLE;
   return MODE_FULL;
}

/**
 * @brief Gets the rotation of the Outbuf.
 *
 * Currently, rotation is not supported by this buffer implementation,
 * so this function always returns 0.
 *
 * @param buf The Outbuf (unused).
 * @return Always returns 0.
 */
int
evas_buffer_outbuf_buf_rot_get(Outbuf *buf EINA_UNUSED)
{
   return 0;
}
