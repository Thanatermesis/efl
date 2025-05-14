#include "evas_engine.h"

/* FIXME: We NEED to get the color map from the VT and use that for the mask */
#define RED_MASK 0xff0000
#define GREEN_MASK 0x00ff00
#define BLUE_MASK 0x0000ff

#define MAX_BUFFERS 10
#define QUEUE_TRIM_DURATION 100

/**
 * @brief Creates a new framebuffer object (Outbuf_Fb).
 *
 * This function allocates and initializes an Outbuf_Fb structure,
 * which includes creating an underlying ecore_drm2_fb.
 *
 * @param ob The parent Outbuf structure.
 * @param w The width of the framebuffer to create.
 * @param h The height of the framebuffer to create.
 * @return A pointer to the newly created Outbuf_Fb, or NULL on failure.
 */
static Outbuf_Fb *
_outbuf_fb_create(Outbuf *ob, int w, int h)
{
   Outbuf_Fb *out;

   out = calloc(1, sizeof(Outbuf_Fb));
   if (!out) return NULL;

   out->fb =
     ecore_drm2_fb_create(ob->dev, w, h,
                          ob->depth, ob->bpp, ob->format);
   if (!out->fb)
     {
        WRN("Failed To Create FB: %d %d", w, h);
        free(out);
        return NULL;
     }

   out->age = 0;
   out->drawn = EINA_FALSE;
   out->valid = EINA_TRUE;

   return out;
}

/**
 * @brief Destroys an Outbuf_Fb object.
 *
 * This function discards the underlying ecore_drm2_fb and frees
 * the memory associated with the Outbuf_Fb structure.
 *
 * @param ofb The Outbuf_Fb object to destroy.
 */
static void
_outbuf_fb_destroy(Outbuf_Fb *ofb)
{
   ecore_drm2_fb_discard(ofb->fb);

   memset(ofb, 0, sizeof(*ofb));
   ofb->valid = EINA_FALSE;
   ofb->drawn = EINA_FALSE;
   ofb->age = 0;
   free(ofb);
}

/**
 * @brief Waits for an available framebuffer or trims unused ones.
 *
 * This function searches for an available (not busy) framebuffer from the
 * list of framebuffers associated with the Outbuf. It prioritizes the
 * oldest available buffer. If unused buffers have persisted for a certain
 * duration (QUEUE_TRIM_DURATION), the oldest one is destroyed to free
 * resources.
 *
 * @param ob The parent Outbuf structure.
 * @return A pointer to an available Outbuf_Fb, or NULL if none are
 *         immediately available (though it might trigger a destroy and recurse).
 */
static Outbuf_Fb *
_outbuf_fb_wait(Outbuf *ob)
{
   Eina_List *l;
   Outbuf_Fb *ofb, *best = NULL;
   int best_age = -1, num_required = 1, num_allocated = 0;

   /* We pick the oldest available buffer to avoid using the same two
    * repeatedly and then having the third be stale when we need it
    */
   EINA_LIST_FOREACH(ob->priv.fb_list, l, ofb)
     {
        num_allocated++;
        if (ecore_drm2_fb_busy_get(ofb->fb))
          {
             num_required++;
             continue;
          }
        if (ofb->valid && (ofb->age > best_age))
          {
             best = ofb;
             best_age = best->age;
          }
     }

   if (num_required < num_allocated)
      ob->priv.unused_duration++;
   else
      ob->priv.unused_duration = 0;

   /* If we've had unused buffers for longer than QUEUE_TRIM_DURATION, then
    * destroy the oldest buffer (currently in best) and recursively call
    * ourself to get the next oldest.
    */
   if (best && (ob->priv.unused_duration > QUEUE_TRIM_DURATION))
     {
        ob->priv.unused_duration = 0;
        ob->priv.fb_list = eina_list_remove(ob->priv.fb_list, best);
        _outbuf_fb_destroy(best);
        best = _outbuf_fb_wait(ob);
     }

   return best;
}

/**
 * @brief Assigns a framebuffer for drawing.
 *
 * This function attempts to get an available framebuffer using _outbuf_fb_wait.
 * If no buffer is available and the maximum number of buffers has not been
 * reached, a new framebuffer is created. If still no buffer is available,
 * it attempts to release the current output's framebuffer and tries again.
 * It also manages the age of existing framebuffers.
 *
 * @param ob The parent Outbuf structure.
 * @return A pointer to the assigned Outbuf_Fb for drawing, or NULL on failure.
 */
static Outbuf_Fb *
_outbuf_fb_assign(Outbuf *ob)
{
   int fw = 0, fh = 0;
   Outbuf_Fb *ofb;
   Eina_List *l;

   ob->priv.draw = _outbuf_fb_wait(ob);
   if (!ob->priv.draw)
     {
        EINA_SAFETY_ON_TRUE_RETURN_VAL(eina_list_count(ob->priv.fb_list) >= MAX_BUFFERS, NULL);

        if ((ob->rotation == 0) || (ob->rotation == 180))
          {
             fw = ob->w;
             fh = ob->h;
          }
        else if ((ob->rotation == 90) || (ob->rotation == 270))
          {
             fw = ob->h;
             fh = ob->w;
          }
        ob->priv.draw = _outbuf_fb_create(ob, fw, fh);
        if (ob->priv.draw)
          ob->priv.fb_list = eina_list_append(ob->priv.fb_list, ob->priv.draw);
     }

   while (!ob->priv.draw)
     {
        ecore_drm2_fb_release(ob->priv.output, EINA_TRUE);
        ob->priv.draw = _outbuf_fb_wait(ob);
     }

   EINA_LIST_FOREACH(ob->priv.fb_list, l, ofb)
     {
        if ((ofb->valid) && (ofb->drawn))
          {
             ofb->age++;
             if (ofb->age > 4)
               {
                  ofb->age = 0;
                  ofb->drawn = EINA_FALSE;
               }
          }
     }

   return ob->priv.draw;
}

/**
 * @brief Swaps the current drawing buffer to the display.
 *
 * This function ensures a valid drawing buffer is assigned (using
 * _outbuf_fb_assign if necessary). It then assigns the framebuffer to
 * a DRM plane (creating one if it doesn't exist) and flips the
 * framebuffer to the output, making it visible.
 *
 * @param ob The parent Outbuf structure.
 */
static void
_outbuf_buffer_swap(Outbuf *ob)
{
   Outbuf_Fb *ofb;

   ofb = ob->priv.draw;
   if (!ofb)
     {
        ecore_drm2_fb_release(ob->priv.output, EINA_TRUE);
        ofb = _outbuf_fb_assign(ob);
        if (!ofb)
          {
             ERR("Could not assign front buffer");
             return;
          }
     }

   if (!ob->priv.plane)
     ob->priv.plane = ecore_drm2_plane_assign(ob->priv.output, ofb->fb, 0, 0);
   else ecore_drm2_plane_fb_set(ob->priv.plane, ofb->fb);

   ecore_drm2_fb_flip(ofb->fb, ob->priv.output);
   ofb->drawn = EINA_TRUE;
   ofb->age = 0;
}

/**
 * @brief Sets up and initializes an Outbuf structure.
 *
 * @param info Pointer to Evas_Engine_Info_Drm containing DRM device
 *             information, output, depth, bpp, format, alpha, and rotation.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @return A pointer to the newly allocated and initialized Outbuf structure,
 *         or NULL on allocation failure.
 */
Outbuf *
_outbuf_setup(Evas_Engine_Info_Drm *info, int w, int h)
{
   Outbuf *ob;

   ob = calloc(1, sizeof(Outbuf));
   if (!ob) return NULL;

   ob->w = w;
   ob->h = h;
   ob->dev = info->info.dev;
   ob->alpha = info->info.alpha;
   ob->rotation = info->info.rotation;

   ob->bpp = info->info.bpp;
   ob->depth = info->info.depth;
   ob->format = info->info.format;

   ob->priv.output = info->info.output;

   return ob;
}

/**
 * @brief Frees an Outbuf structure and its associated resources.
 *
 * This function releases all pending image updates, flushes any remaining
 * operations, destroys all associated framebuffers, and frees the Outbuf
 * structure itself.
 *
 * @param ob The Outbuf structure to free.
 */
void
_outbuf_free(Outbuf *ob)
{
   Outbuf_Fb *ofb;

   while (ob->priv.pending)
     {
        RGBA_Image *img;
        Eina_Rectangle *rect;

        img = ob->priv.pending->data;
        ob->priv.pending =
          eina_list_remove_list(ob->priv.pending, ob->priv.pending);

        rect = img->extended_info;

        evas_cache_image_drop(&img->cache_entry);

        eina_rectangle_free(rect);
     }

   /* TODO: idle flush */

   _outbuf_flush(ob, NULL, NULL, EVAS_RENDER_MODE_UNDEF);

   EINA_LIST_FREE(ob->priv.fb_list, ofb)
     _outbuf_fb_destroy(ofb);

   free(ob);
}

/**
 * @brief Gets the current rotation of the output buffer.
 *
 * @param ob The Outbuf structure.
 * @return The current rotation angle (0, 90, 180, or 270).
 */
int
_outbuf_rotation_get(Outbuf *ob)
{
   return ob->rotation;
}

/**
 * @brief Reconfigures the output buffer with new dimensions, rotation, or depth.
 *
 * This function updates the Outbuf's properties. If the new configuration
 * differs from the current one, it updates the width, height, depth, pixel
 * format, and rotation. It also resets the unused buffer duration and
 * flushes any idle buffers.
 *
 * @param ob The Outbuf structure to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rotation The new rotation (0, 90, 180, 270).
 * @param depth The new color depth, which determines the pixel format.
 *              Example: OUTBUF_DEPTH_ARGB_32BPP_8888_8888 results in DRM_FORMAT_ARGB8888.
 */
void
_outbuf_reconfigure(Outbuf *ob, int w, int h, int rotation, Outbuf_Depth depth)
{
   unsigned int format = DRM_FORMAT_ARGB8888;

   switch (depth)
     {
      case OUTBUF_DEPTH_RGB_16BPP_565_565_DITHERED:
        format = DRM_FORMAT_RGB565;
        break;
      case OUTBUF_DEPTH_RGB_16BPP_555_555_DITHERED:
        format = DRM_FORMAT_RGBX5551;
        break;
      case OUTBUF_DEPTH_RGB_16BPP_444_444_DITHERED:
        format = DRM_FORMAT_RGBX4444;
        break;
      case OUTBUF_DEPTH_RGB_16BPP_565_444_DITHERED:
        format = DRM_FORMAT_RGB565;
        break;
      case OUTBUF_DEPTH_RGB_32BPP_888_8888:
        format = DRM_FORMAT_RGBX8888;
        break;
      case OUTBUF_DEPTH_ARGB_32BPP_8888_8888:
        format = DRM_FORMAT_ARGB8888;
        break;
      case OUTBUF_DEPTH_BGRA_32BPP_8888_8888:
        format = DRM_FORMAT_BGRA8888;
        break;
      case OUTBUF_DEPTH_BGR_32BPP_888_8888:
        format = DRM_FORMAT_BGRX8888;
        break;
      case OUTBUF_DEPTH_RGB_24BPP_888_888:
        format = DRM_FORMAT_RGB888;
        break;
      case OUTBUF_DEPTH_BGR_24BPP_888_888:
        format = DRM_FORMAT_BGR888;
        break;
      case OUTBUF_DEPTH_INHERIT:
      default:
        depth = ob->depth;
        format = ob->format;
        break;
     }

   if ((ob->w == w) && (ob->h == h) && (ob->rotation == rotation) &&
       (ob->depth == depth) && (ob->format == format))
     return;

   ob->w = w;
   ob->h = h;
   ob->depth = depth;
   ob->format = format;
   ob->rotation = rotation;
   ob->priv.unused_duration = 0;

   _outbuf_idle_flush(ob);
}

/**
 * @brief Gets the current swap mode state of the output buffer.
 *
 * The swap mode is determined by the age of the current drawing buffer.
 * This indicates how many frames the current buffer has been displayed,
 * suggesting different rendering strategies (e.g., full redraw, copy, double/triple/quadruple buffering).
 *
 * @param ob The Outbuf structure.
 * @return The current Render_Output_Swap_Mode.
 *         Example: MODE_FULL if the buffer is new or very old (age > 4),
 *                  MODE_COPY if age is 1, etc.
 */
Render_Output_Swap_Mode
_outbuf_state_get(Outbuf *ob)
{
   int age;

   if (!ob->priv.draw) return MODE_FULL;

   age = ob->priv.draw->age;
   if (age > 4) return MODE_FULL;
   else if (age == 1) return MODE_COPY;
   else if (age == 2) return MODE_DOUBLE;
   else if (age == 3) return MODE_TRIPLE;
   else if (age == 4) return MODE_QUADRUPLE;

   return MODE_FULL;
}

/**
 * @brief Creates a new image buffer for updating a region of the output.
 *
 * This function allocates an RGBA_Image from the cache to hold pixel data
 * for a specified region. The region is clipped to the Outbuf dimensions.
 * The allocated image is added to a list of pending updates.
 *
 * @param ob The Outbuf structure.
 * @param x The x-coordinate of the region to update.
 * @param y The y-coordinate of the region to update.
 * @param w The width of the region to update.
 * @param h The height of the region to update.
 * @param[out] cx Pointer to store the clipped x-coordinate (relative to the new buffer, usually 0).
 * @param[out] cy Pointer to store the clipped y-coordinate (relative to the new buffer, usually 0).
 * @param[out] cw Pointer to store the clipped width.
 * @param[out] ch Pointer to store the clipped height.
 * @return A pointer to the allocated RGBA_Image, or NULL on failure.
 *         The RGBA_Image contains:
 *         - cache_entry: Evas_Cache_Image for image data and properties.
 *         - extended_info: Eina_Rectangle storing the original x, y, w, h.
 */
void *
_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch)
{
   RGBA_Image *img = NULL;
   Eina_Rectangle *rect;

   if ((w <= 0) || (h <= 0)) return NULL;

   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, ob->w, ob->h);

   if (!(rect = eina_rectangle_new(x, y, w, h)))
     return NULL;

   img = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());

   if (!img)
     {
        eina_rectangle_free(rect);
        return NULL;
     }

   img->cache_entry.flags.alpha = ob->alpha;

   evas_cache_image_surface_alloc(&img->cache_entry, w, h);

   img->extended_info = rect;

   if (cx) *cx = 0;
   if (cy) *cy = 0;
   if (cw) *cw = w;
   if (ch) *ch = h;

   /* add this cached image data to pending writes */
   ob->priv.pending =
     eina_list_append(ob->priv.pending, img);

   return img;
}

/**
 * @brief Pushes pixel data from an RGBA_Image to the current drawing framebuffer.
 *
 * This function converts and copies pixel data from the source `update` image
 * to the corresponding region in the current drawing framebuffer (ob->priv.draw).
 * It handles rotation by adjusting coordinates and selecting the appropriate
 * conversion function. The destination region is clipped to the framebuffer
 * dimensions.
 *
 * @param ob The Outbuf structure.
 * @param update The RGBA_Image containing the source pixel data.
 * @param x The x-offset within the `update` image (typically 0).
 * @param y The y-offset within the `update` image (typically 0).
 * @param w The width of the sub-region in `update` to push.
 * @param h The height of the sub-region in `update` to push.
 */
void
_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h)
{
   Gfx_Func_Convert func = NULL;
   Eina_Rectangle rect = {0, 0, 0, 0}, pr;
   DATA32 *src;
   DATA8 *dst;
   Ecore_Drm2_Fb *buff;
   int bpp = 0, bpl = 0;
   int rx = 0, ry = 0;
   int bw = 0, bh = 0;

   /* check for valid output buffer */
   if (!ob) return;

   /* check for pending writes */
   if (!ob->priv.pending) return;

   /* check for valid source data */
   if (!(src = update->image.data)) return;

   /* check for valid desination data */
   if (!ob->priv.draw) return;
   buff = ob->priv.draw->fb;

   dst = ecore_drm2_fb_data_get(buff);
   if (!dst) return;

   if ((ob->rotation == 0) || (ob->rotation == 180))
     {
        func =
          evas_common_convert_func_get(0, w, h, ob->bpp,
                                       RED_MASK, GREEN_MASK, BLUE_MASK,
                                       PAL_MODE_NONE, ob->rotation);
     }
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     {
        func =
          evas_common_convert_func_get(0, h, w, ob->bpp,
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

   bpp = ob->bpp / 8;
   bw = ob->w;
   bh = ob->h;
   /* bpp = (ob->depth / 8); */
   /* if (bpp <= 0) return; */

   bpl = ecore_drm2_fb_stride_get(buff);

   if (ob->rotation == 0)
     {
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h,
                           0, 0, bw, bh);
        dst += (bpl * rect.y) + (rect.x * bpp);
        w -= rx;
     }
   else if (ob->rotation == 180)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h,
                           0, 0, bw, bh);
        rx = pr.w - rect.w;
        ry = pr.h - rect.h;
        src += (update->cache_entry.w * ry) + rx;
        w -= rx;
     }
   else if (ob->rotation == 90)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h,
                           0, 0, bw, bh);
        rx = pr.w - rect.w; ry = pr.h - rect.h;
        src += ry;
        w -= ry;
     }
   else if (ob->rotation == 270)
     {
        pr = rect;
        RECTS_CLIP_TO_RECT(rect.x, rect.y, rect.w, rect.h,
                           0, 0, bw, bh);
        rx = pr.w - rect.w; ry = pr.h - rect.h;
        src += (update->cache_entry.w * rx);
        w -= ry;
     }

   if ((rect.w <= 0) || (rect.h <= 0)) return;

   func(src, dst, (update->cache_entry.w - w), ((bpl / bpp) - rect.w),
        rect.w, rect.h, x + rx, y + ry, NULL);
}

/**
 * @brief Flushes pending updates to the screen.
 *
 * This function processes all RGBA_Image updates that were previously added
 * via _outbuf_update_region_new and (implicitly) written to by
 * _outbuf_update_region_push. It collects the damage rectangles,
 * transforms them according to rotation, and then swaps the buffer to make
 * the changes visible. Pending images are then released.
 *
 * The `ob->priv.rects` array will contain `Eina_Rectangle` elements,
 * where each rectangle describes a damaged region:
 *   typedef struct _Eina_Rectangle
 *   {
 *      int x, y, w, h;
 *   } Eina_Rectangle;
 *
 * @param ob The Outbuf structure.
 * @param surface_damage Unused parameter.
 * @param buffer_damage Unused parameter.
 * @param render_mode The current rendering mode. If EVAS_RENDER_MODE_ASYNC_INIT,
 *                    the function returns early.
 */
void
_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   Eina_Rectangle *r;
   RGBA_Image *img;
   unsigned int i = 0;

   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   if (ob->priv.rect_count) free(ob->priv.rects);

   /* get number of pending writes */
   ob->priv.rect_count = eina_list_count(ob->priv.pending);
   if (ob->priv.rect_count == 0) return;

   /* allocate rectangles */
   ob->priv.rects = malloc(ob->priv.rect_count * sizeof(Eina_Rectangle));
   if (!ob->priv.rects) return;
   r = ob->priv.rects;

   /* loop the pending writes */
   EINA_LIST_FREE(ob->priv.pending, img)
     {
        Eina_Rectangle *rect;
        int x = 0, y = 0, w = 0, h = 0;

        rect = img->extended_info;
        if (!rect) continue;

        x = rect->x; y = rect->y; w = rect->w; h = rect->h;

        /* based on rotation, set rectangle position */
        if (ob->rotation == 0)
          {
             r[i].x = x;
             r[i].y = y;
          }
        else if (ob->rotation == 90)
          {
             r[i].x = y;
             r[i].y = (ob->w - x - w);
          }
        else if (ob->rotation == 180)
          {
             r[i].x = (ob->w - x - w);
             r[i].y = (ob->h - y - h);
          }
        else if (ob->rotation == 270)
          {
             r[i].x = (ob->h - y - h);
             r[i].y = x;
          }

        /* based on rotation, set rectangle size */
        if ((ob->rotation == 0) || (ob->rotation == 180))
          {
             r[i].w = w;
             r[i].h = h;
          }
        else if ((ob->rotation == 90) || (ob->rotation == 270))
          {
             r[i].w = h;
             r[i].h = w;
          }

        eina_rectangle_free(rect);

        evas_cache_image_drop(&img->cache_entry);

        i++;
     }

   _outbuf_buffer_swap(ob);
}

/**
 * @brief Sets the dirty regions on the current drawing framebuffer.
 *
 * This function takes a list of Tilebuf_Rect structures, converts them
 * to Eina_Rectangle, and marks these regions as dirty on the
 * ecore_drm2_fb associated with the current drawing buffer. This is
 * typically used by the DRM backend to optimize screen updates.
 *
 * The `damage` parameter is an Eina_Inlist of `Tilebuf_Rect` structures:
 *   typedef struct _Tilebuf_Rect
 *   {
 *     EINA_INLIST;
 *     int x, y, w, h;
 *   } Tilebuf_Rect;
 *
 * @param ob The Outbuf structure.
 * @param damage An Eina_Inlist of Tilebuf_Rect structures representing
 *               the damaged regions.
 */
void
_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage)
{
   Tilebuf_Rect *tr;
   Eina_Rectangle *rects;
   Ecore_Drm2_Fb *fb;
   int count, i = 0;

   if (!ob->priv.draw) return;

   fb = ob->priv.draw->fb;

   count = eina_inlist_count(EINA_INLIST_GET(damage));
   rects = alloca(count * sizeof(Eina_Rectangle));

   EINA_INLIST_FOREACH(damage, tr)
     {
        rects[i].x = tr->x;
        rects[i].y = tr->y;
        rects[i].w = tr->w;
        rects[i].h = tr->h;
        i++;
     }

   ecore_drm2_fb_dirty(fb, rects, count);
}

/**
 * @brief Flushes pending image updates and releases unused framebuffers.
 *
 * This function discards all images currently in the pending update list
 * without actually drawing them. It then attempts to release any framebuffers
 * held by the output that are not currently busy, effectively cleaning up
 * resources during idle periods.
 *
 * @param ob The Outbuf structure.
 */
void
_outbuf_idle_flush(Outbuf *ob)
{
   while (ob->priv.pending)
     {
        RGBA_Image *img;
        Eina_Rectangle *rect;

        img = ob->priv.pending->data;
        ob->priv.pending =
          eina_list_remove_list(ob->priv.pending, ob->priv.pending);

        rect = img->extended_info;

        evas_cache_image_drop(&img->cache_entry);

        eina_rectangle_free(rect);
     }

   while (ecore_drm2_fb_release(ob->priv.output, EINA_TRUE));
}
