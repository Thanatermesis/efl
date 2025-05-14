#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"

#define RED_MASK 0xff0000
#define GREEN_MASK 0x00ff00
#define BLUE_MASK 0x0000ff

/**
 * @brief Sets up and initializes an Evas output buffer (Outbuf).
 *
 * This function allocates and configures an Outbuf structure, which represents
 * the drawing surface for an Evas canvas, typically associated with a Wayland
 * window. It creates the necessary Wayland surface resources.
 *
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @param info Pointer to the Evas Wayland engine specific information.
 * @return A pointer to the newly created Outbuf structure on success,
 *         or NULL on allocation failure or surface creation failure.
 */
Outbuf *
_evas_outbuf_setup(int w, int h, Evas_Engine_Info_Wayland *info)
{
   Outbuf *ob = NULL;

   LOGFN;

   /* try to allocate space for new Outbuf */
   if (!(ob = calloc(1, sizeof(Outbuf)))) return NULL;

   /* set outbuf properties */
   ob->w = w;
   ob->h = h;
   ob->info = info;
   ob->rotation = info->info.rotation;
   ob->depth = info->info.depth;
   ob->priv.destination_alpha = info->info.destination_alpha;
   ob->ewd = ecore_wl2_window_display_get(info->info.wl2_win);

   ob->surface = ecore_wl2_surface_create(info->info.wl2_win,
                                          ob->priv.destination_alpha);
   if (!ob->surface) goto surf_err;

   eina_array_step_set(&ob->priv.onebuf_regions, sizeof(Eina_Array), 8);

   return ob;

surf_err:
   free(ob);
   return NULL;
}

/**
 * @brief Frees the resources associated with an Evas output buffer.
 *
 * This function cleans up and deallocates all resources used by the Outbuf,
 * including pending image writes, Wayland surfaces, and internal arrays.
 *
 * @param ob The Outbuf structure to free.
 */
void
_evas_outbuf_free(Outbuf *ob)
{
   LOGFN;

   while (ob->priv.pending_writes)
     {
        RGBA_Image *img;
        Eina_Rectangle *rect;

        img = ob->priv.pending_writes->data;
        ob->priv.pending_writes =
          eina_list_remove_list(ob->priv.pending_writes, ob->priv.pending_writes);

        rect = img->extended_info;

        evas_cache_image_drop(&img->cache_entry);

        eina_rectangle_free(rect);
     }

   _evas_outbuf_flush(ob, NULL, NULL, EVAS_RENDER_MODE_UNDEF);
   _evas_outbuf_idle_flush(ob);

   if (ob->surface) ecore_wl2_surface_destroy(ob->surface);

   eina_array_flush(&ob->priv.onebuf_regions);

   free(ob);
}

/**
 * @brief Flushes idle resources associated with the output buffer.
 *
 * This function is typically called when the buffer is idle. It releases
 * cached image data (either the single buffer `onebuf` or previous pending
 * writes) that are no longer needed for the current rendering cycle.
 *
 * @param ob The Outbuf structure to flush.
 */
void
_evas_outbuf_idle_flush(Outbuf *ob)
{
   RGBA_Image *img;
   Eina_Rectangle *rect;

   if (ob->priv.onebuf)
     {
        img = ob->priv.onebuf;
        ob->priv.onebuf = NULL;

        rect = img->extended_info;
        eina_rectangle_free(rect);

        evas_cache_image_drop(&img->cache_entry);
     }
   else
     {
        while (ob->priv.prev_pending_writes)
          {
             img = ob->priv.prev_pending_writes->data;
             ob->priv.prev_pending_writes =
               eina_list_remove_list(ob->priv.prev_pending_writes,
                                     ob->priv.prev_pending_writes);
             rect = img->extended_info;
             evas_cache_image_drop(&img->cache_entry);

             eina_rectangle_free(rect);
          }
     }
}

/**
 * @brief Processes and prepares damage regions for rendering.
 *
 * This function consolidates the updated regions (damage) recorded either
 * in `ob->priv.onebuf_regions` (for non-rotated cases) or
 * `ob->priv.pending_writes` (for rotated cases). It calculates the final
 * rectangles to be redrawn, taking rotation into account, and stores them
 * in `ob->priv.rects`. It also releases the associated image cache entries.
 *
 * @param ob The Outbuf structure.
 * @param surface_damage Unused parameter.
 * @param buffer_damage Unused parameter.
 * @param render_mode The current render mode. If ASYNC_INIT, the function returns early.
 */
void
_evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   Eina_Rectangle *result;
   RGBA_Image *img;
   unsigned int i = 0;

   LOGFN;

   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   if (ob->priv.rect_count) free(ob->priv.rects);

   /* check for pending writes */
   if (!ob->priv.pending_writes)
     {
        Eina_Rectangle *rect;
        Eina_Array_Iterator it;

        /* get number of buffer regions */
        ob->priv.rect_count = eina_array_count_get(&ob->priv.onebuf_regions);
        if (ob->priv.rect_count == 0) return;

        /* allocate rectangles */
        ob->priv.rects = malloc(ob->priv.rect_count * sizeof(Eina_Rectangle));
        if (!ob->priv.rects) return;
        result = ob->priv.rects;

        /* loop the buffer regions and assign to result */
        EINA_ARRAY_ITER_NEXT(&ob->priv.onebuf_regions, i, rect, it)
          {
             result[i] = *rect;
             eina_rectangle_free(rect);
          }

        /* clean array */
        eina_array_clean(&ob->priv.onebuf_regions);

        img = ob->priv.onebuf;
        ob->priv.onebuf = NULL;
        if (img)
          evas_cache_image_drop(&img->cache_entry);
     }
   else
     {
        /* get number of pending writes */
        ob->priv.rect_count = eina_list_count(ob->priv.pending_writes);
        if (ob->priv.rect_count == 0) return;

        ob->priv.rects = malloc(ob->priv.rect_count * sizeof(Eina_Rectangle));
        if (!ob->priv.rects) return;
        result = ob->priv.rects;

        /* loop the pending writes */
        EINA_LIST_FREE(ob->priv.pending_writes, img)
          {
             Eina_Rectangle *rect;
             int x = 0, y = 0, w = 0, h = 0;

             if (!(rect = img->extended_info)) continue;

             x = rect->x; y = rect->y; w = rect->w; h = rect->h;

             /* based on rotation, set rectangle position */
             if (ob->rotation == 0)
               {
                  result[i].x = x;
                  result[i].y = y;
               }
             else if (ob->rotation == 90)
               {
                  result[i].x = y;
                  result[i].y = (ob->w - x - w);
               }
             else if (ob->rotation == 180)
               {
                  result[i].x = (ob->w - x - w);
                  result[i].y = (ob->h - y - h);
               }
             else if (ob->rotation == 270)
               {
                  result[i].x = (ob->h - y - h);
                  result[i].y = x;
               }

             /* based on rotation, set rectangle size */
             if ((ob->rotation == 0) || (ob->rotation == 180))
               {
                  result[i].w = w;
                  result[i].h = h;
               }
             else if ((ob->rotation == 90) || (ob->rotation == 270))
               {
                  result[i].w = h;
                  result[i].h = w;
               }

             eina_rectangle_free(rect);

             evas_cache_image_drop(&img->cache_entry);

             i++;
          }
     }
}

/**
 * @brief Determines the buffer swap mode based on surface age.
 *
 * Queries the underlying Wayland surface for its age (number of frames
 * since last update) to determine the optimal swap mode (e.g., full redraw,
 * copy, double/triple/quadruple buffering).
 *
 * @param ob The Outbuf structure.
 * @return The recommended Render_Output_Swap_Mode.
 * @see Render_Output_Swap_Mode
 */
Render_Output_Swap_Mode
_evas_outbuf_swap_mode_get(Outbuf *ob)
{
   int age;

   LOGFN;

   age = ecore_wl2_surface_assign(ob->surface);
   if (!age) return MODE_FULL;

   else if (age == 1) return MODE_COPY;
   else if (age == 2) return MODE_DOUBLE;
   else if (age == 3) return MODE_TRIPLE;
   else if (age == 4) return MODE_QUADRUPLE;

   return MODE_FULL;
}

/**
 * @brief Gets the current rotation of the output buffer.
 *
 * @param ob The Outbuf structure.
 * @return The rotation angle (0, 90, 180, or 270 degrees).
 */
int
_evas_outbuf_rotation_get(Outbuf *ob)
{
   LOGFN;

   return ob->rotation;
}

/**
 * @brief Reconfigures the output buffer properties.
 *
 * Updates the dimensions, rotation, depth, and alpha properties of the
 * output buffer and its associated Wayland surface. Flushes idle resources
 * if any properties change.
 *
 * @param ob The Outbuf structure to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation (0, 90, 180, 270).
 * @param depth The new color depth (e.g., OUTBUF_DEPTH_RGB_32BPP_ARGB_8888).
 *              If OUTBUF_DEPTH_NONE or OUTBUF_DEPTH_INHERIT, the existing depth is kept.
 * @param alpha EINA_TRUE if the destination surface should have an alpha channel, EINA_FALSE otherwise.
 * @param resize EINA_TRUE if the underlying Wayland surface should be resized, EINA_FALSE otherwise.
 */
void
_evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize)
{
   LOGFN;

   if ((depth == OUTBUF_DEPTH_NONE) ||
       (depth == OUTBUF_DEPTH_INHERIT))
     depth = ob->depth;

   if ((ob->w == w) && (ob->h == h) &&
       (ob->rotation == rot) && (ob->depth == depth) &&
       (ob->priv.destination_alpha == alpha))
     return;

   ob->w = w;
   ob->h = h;
   ob->rotation = rot;
   ob->depth = depth;
   ob->priv.destination_alpha = alpha;

   if ((ob->rotation == 0) || (ob->rotation == 180))
     {
        ecore_wl2_surface_reconfigure(ob->surface, w, h, resize, alpha);
     }
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     {
        ecore_wl2_surface_reconfigure(ob->surface, h, w, resize, alpha);
     }

   _evas_outbuf_idle_flush(ob);
}

/**
 * @brief Creates a new region within the output buffer for updating.
 *
 * This function prepares a portion of the output buffer to receive pixel data.
 * Depending on the rotation:
 * - If rotation is 0: It potentially reuses a single backing buffer (`ob->priv.onebuf`)
 *   and adds the specified rectangle (`x, y, w, h`) to the `ob->priv.onebuf_regions` array.
 *   `ob->priv.onebuf_regions` is an Eina_Array containing pointers to Eina_Rectangle structs,
 *   e.g., `[Eina_Rectangle*, Eina_Rectangle*, ...]`.
 *   The returned image points to this shared buffer, and the output parameters
 *   `cx, cy, cw, ch` reflect the coordinates within this buffer.
 * - If rotation is non-0: It allocates a new temporary image buffer for the update.
 *   This image is added to the `ob->priv.pending_writes` list. The output parameters
 *   `cx, cy, cw, ch` are set to 0, 0, w, h, indicating the update should be written
 *   to the beginning of this temporary buffer.
 *
 * The function clips the requested region to the buffer boundaries.
 *
 * @param ob The Outbuf structure.
 * @param x The x-coordinate of the region's top-left corner.
 * @param y The y-coordinate of the region's top-left corner.
 * @param w The width of the region.
 * @param h The height of the region.
 * @param[out] cx Pointer to store the clipped x-coordinate within the returned buffer.
 * @param[out] cy Pointer to store the clipped y-coordinate within the returned buffer.
 * @param[out] cw Pointer to store the clipped width within the returned buffer.
 * @param[out] ch Pointer to store the clipped height within the returned buffer.
 * @return A pointer to an RGBA_Image representing the buffer region to update,
 *         or NULL if the region is invalid, clipping results in an empty area,
 *         or allocation fails. The `extended_info` field of the returned image
 *         stores the original requested Eina_Rectangle.
 */
void *
_evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch)
{
   RGBA_Image *img;
   Eina_Rectangle *rect;

   LOGFN;

   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, ob->w, ob->h);
   if ((w <= 0) || (h <= 0)) return NULL;

   if (ob->rotation == 0)
     {
        if (!(img = ob->priv.onebuf))
          {
             int bw = 0, bh = 0;
             void *data;

             if (!(data = ecore_wl2_surface_data_get(ob->surface, &bw, &bh)))
               {
                  /* ERR("Could not get surface data"); */
                  return NULL;
               }

             img = (RGBA_Image *)
               evas_cache_image_data(evas_common_image_cache_get(),
                                     bw, bh, data,
                                     ob->priv.destination_alpha,
                                     EVAS_COLORSPACE_ARGB8888);

             ob->priv.onebuf = img;
             if (!img) return NULL;
          }

        if (!(rect = eina_rectangle_new(x, y, w, h)))
          return NULL;

        if (!eina_array_push(&ob->priv.onebuf_regions, rect))
          {
             evas_cache_image_drop(&img->cache_entry);

             eina_rectangle_free(rect);
             return NULL;
          }

        if (cx) *cx = x;
        if (cy) *cy = y;
        if (cw) *cw = w;
        if (ch) *ch = h;

        img->extended_info = rect;

        return img;
     }
   else
     {
        if (!(rect = eina_rectangle_new(x, y, w, h)))
          return NULL;

        img = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());

        if (!img)
          {
             eina_rectangle_free(rect);
             return NULL;
          }

        img->cache_entry.w = w;
        img->cache_entry.h = h;
        img->cache_entry.flags.alpha |= ob->priv.destination_alpha ? 1 : 0;

        evas_cache_image_surface_alloc(&img->cache_entry, w, h);

        img->extended_info = rect;

        ob->priv.pending_writes =
          eina_list_append(ob->priv.pending_writes, img);

        if (cx) *cx = 0;
        if (cy) *cy = 0;
        if (cw) *cw = w;
        if (ch) *ch = h;
        return img;
     }

   return NULL;
}

/**
 * @brief Pushes updated pixel data to a previously created region.
 *
 * This function copies pixel data from the source `update` image (obtained
 * from `_evas_outbuf_update_region_new` when rotation is non-0) into the
 * actual Wayland surface buffer associated with the Outbuf. It handles
 * color conversion and rotation adjustments.
 *
 * This function is primarily used when rotation is non-zero, as the update
 * needs to be copied from a temporary buffer (`update`) to the final
 * destination surface, applying rotation during the copy. For zero rotation,
 * updates are typically written directly to the buffer obtained via
 * `_evas_outbuf_update_region_new`.
 *
 * @param ob The Outbuf structure.
 * @param update The RGBA_Image containing the updated pixel data.
 * @param x The original x-coordinate of the update region (before rotation).
 * @param y The original y-coordinate of the update region (before rotation).
 * @param w The original width of the update region (before rotation).
 * @param h The original height of the update region (before rotation).
 */
void
_evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h)
{
   Gfx_Func_Convert func = NULL;
   Eina_Rectangle rect = {0, 0, 0, 0}, pr;
   DATA32 *src;
   DATA8 *dst;
   int depth = 32, bpp = 0, bpl = 0, wid = 0;
   int ww = 0, hh = 0;
   int rx = 0, ry = 0;

   LOGFN;

   /* check for valid output buffer */
   if (!ob) return;

   /* check for pending writes */
   if (!ob->priv.pending_writes) return;

   if ((ob->rotation == 0) || (ob->rotation == 180))
     {
        func =
          evas_common_convert_func_get(0, w, h, depth,
                                       RED_MASK, GREEN_MASK, BLUE_MASK,
                                       PAL_MODE_NONE, ob->rotation);
     }
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     {
        func =
          evas_common_convert_func_get(0, h, w, depth,
                                       RED_MASK, GREEN_MASK, BLUE_MASK,
                                       PAL_MODE_NONE, ob->rotation);
     }

   /* make sure we have a valid convert function */
   if (!func) return;

   /* based on rotation, set rectangle position */
   if (ob->rotation == 0)
     {
        rect.x = x;
        rect.y = y;
     }
   else if (ob->rotation == 90)
     {
        rect.x = y;
        rect.y = (ob->w - x - w);
     }
   else if (ob->rotation == 180)
     {
        rect.x = (ob->w - x - w);
        rect.y = (ob->h - y - h);
     }
   else if (ob->rotation == 270)
     {
        rect.x = (ob->h - y - h);
        rect.y = x;
     }

   /* based on rotation, set rectangle size */
   if ((ob->rotation == 0) || (ob->rotation == 180))
     {
        rect.w = w;
        rect.h = h;
     }
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     {
        rect.w = h;
        rect.h = w;
     }

   /* check for valid update image data */
   if (!(src = update->image.data)) return;

   bpp = depth / 8;

   /* check for valid desination data */
   if (!(dst = ecore_wl2_surface_data_get(ob->surface, &ww, &hh)))
     {
        /* ERR("Could not get surface data"); */
        return;
     }

   bpl = (ww * sizeof(int));

   if (ob->rotation == 0)
     {
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h, 0, 0, ww, hh);
        dst += (bpl * rect.y) + (rect.x * bpp);
        w -= rx;
     }
   else if (ob->rotation == 180)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h, 0, 0, ww, hh);
        rx = pr.w - rect.w;
        ry = pr.h - rect.h;
        src += (update->cache_entry.w * ry) + rx;
        w -= rx;
     }
   else if (ob->rotation == 90)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h, 0, 0, ww, hh);
        rx = pr.w - rect.w; ry = pr.h - rect.h;
        src += ry;
        w -= ry;
     }
   else if (ob->rotation == 270)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h, 0, 0, ww, hh);
        rx = pr.w - rect.w; ry = pr.h - rect.h;
        src += (update->cache_entry.w * rx);
        w -= ry;
     }

   if ((rect.w <= 0) || (rect.h <= 0)) return;

   wid = bpl / bpp;

   dst += (bpl * rect.y) + (rect.x * bpp);

   func(src, dst, (update->cache_entry.w - w), (wid - rect.w),
        rect.w, rect.h, x + rx, y + ry, NULL);
}

/**
 * @brief Clears the redraw regions and posts updates to the Wayland surface.
 *
 * After `_evas_outbuf_flush` has prepared the list of rectangles to be
 * redrawn (`ob->priv.rects`), this function sends these damage regions
 * to the Wayland compositor via `ecore_wl2_surface_post`. It then frees
 * the rectangle list and resets the count.
 *
 * @param ob The Outbuf structure.
 */
void
_evas_outbuf_redraws_clear(Outbuf *ob)
{
   struct wl_surface *wls;

   if (!ob->priv.rect_count) return;
   wls = ecore_wl2_window_surface_get(ob->info->info.wl2_win);
   if (wls)
     ecore_wl2_surface_post(ob->surface, ob->priv.rects, ob->priv.rect_count);
   free(ob->priv.rects);
   ob->priv.rect_count = 0;
}
