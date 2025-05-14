#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/time.h>
#include <sys/utsname.h>

#include "evas_common_private.h"
#include "evas_macros.h"

#include "evas_xlib_swapbuf.h"
#include "evas_xlib_color.h"
#include "evas_xlib_swapper.h"

typedef struct _Outbuf_Region Outbuf_Region;

struct _Outbuf_Region
{
   RGBA_Image *im;
   int         x;
   int         y;
   int         w;
   int         h;
};

/**
 * @brief Initializes the Xlib swapbuffer system.
 * Currently, this function is a no-op but is kept for API consistency.
 */
void
evas_software_xlib_swapbuf_init(void)
{
}

/**
 * @brief Frees an Outbuf structure and all associated Xlib resources.
 *
 * This includes deallocating colors from the colormap if a palette was used,
 * freeing the Xlib swapper, flushing any pending regions, and freeing the
 * Outbuf structure itself.
 *
 * @param buf The Outbuf to be freed.
 */
void
evas_software_xlib_swapbuf_free(Outbuf *buf)
{
   evas_software_xlib_swapbuf_flush(buf, NULL, NULL, EVAS_RENDER_MODE_UNDEF);
   evas_software_xlib_swapbuf_idle_flush(buf);
   if (buf->priv.pal)
     evas_software_xlib_x_color_deallocate
       (buf->priv.x11.xlib.disp, buf->priv.x11.xlib.cmap,
       buf->priv.x11.xlib.vis, buf->priv.pal);
   evas_xlib_swapper_free(buf->priv.swapper);
   eina_array_flush(&buf->priv.onebuf_regions);
   free(buf);
}

/**
 * @brief Sets up an Outbuf for Xlib rendering.
 *
 * Initializes an Outbuf structure with X11 display parameters, visual information,
 * colormap, and other settings required for rendering. It also sets up an
 * Xlib swapper for managing buffer swaps. Color palettes are allocated if
 * the visual class requires it (e.g., PseudoColor).
 *
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @param rot Rotation of the output (0, 90, 180, 270 degrees).
 * @param depth Output buffer depth.
 * @param disp X11 Display connection.
 * @param draw X11 Drawable (Window or Pixmap).
 * @param vis X11 Visual.
 * @param cmap X11 Colormap.
 * @param x_depth Depth of the X11 drawable.
 * @param grayscale Hint for grayscale palette generation.
 * @param max_colors Maximum colors for palette mode.
 * @param mask Mask Pixmap (currently unused).
 * @param shape_dither Flag to enable shape dithering.
 * @param destination_alpha Flag indicating if the destination has an alpha channel.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *
evas_software_xlib_swapbuf_setup_x(int w, int h, int rot, Outbuf_Depth depth,
                                   Display *disp, Drawable draw, Visual *vis,
                                   Colormap cmap, int x_depth,
                                   int grayscale, int max_colors,
                                   Pixmap mask EINA_UNUSED,
                                   int shape_dither, int destination_alpha)
{
   Outbuf *buf;
   Gfx_Func_Convert conv_func = NULL;
   int d;

   buf = calloc(1, sizeof(Outbuf));
   if (!buf) return NULL;

   if (x_depth < 15) rot = 0;

   buf->onebuf = 1;
   buf->w = w;
   buf->h = h;
   buf->depth = depth;
   buf->rot = rot;
   buf->priv.x11.xlib.disp = disp;
   buf->priv.x11.xlib.win = draw;
   buf->priv.x11.xlib.vis = vis;
   buf->priv.x11.xlib.cmap = cmap;
   buf->priv.x11.xlib.depth = x_depth;
   buf->priv.mask_dither = shape_dither;
   buf->priv.destination_alpha = destination_alpha;

   if ((buf->rot == 0) || (buf->rot == 180))
     buf->priv.swapper = evas_xlib_swapper_new(buf->priv.x11.xlib.disp,
                                               buf->priv.x11.xlib.win,
                                               buf->priv.x11.xlib.vis,
                                               buf->priv.x11.xlib.depth,
                                               buf->w, buf->h);
   else if ((buf->rot == 90) || (buf->rot == 270))
     buf->priv.swapper = evas_xlib_swapper_new(buf->priv.x11.xlib.disp,
                                               buf->priv.x11.xlib.win,
                                               buf->priv.x11.xlib.vis,
                                               buf->priv.x11.xlib.depth,
                                               buf->h, buf->w);
   if (!buf->priv.swapper)
     {
        free(buf);
        return NULL;
     }

   eina_array_step_set(&buf->priv.onebuf_regions, sizeof (Eina_Array), 8);

#ifdef WORDS_BIGENDIAN
   if (evas_xlib_swapper_byte_order_get(buf->priv.swapper) == LSBFirst)
     buf->priv.x11.xlib.swap = 1;
   if (evas_xlib_swapper_bit_order_get(buf->priv.swapper) == MSBFirst)
     buf->priv.x11.xlib.bit_swap = 1;
#else
   if (evas_xlib_swapper_byte_order_get(buf->priv.swapper) == MSBFirst)
     buf->priv.x11.xlib.swap = 1;
   if (evas_xlib_swapper_bit_order_get(buf->priv.swapper) == MSBFirst)
     buf->priv.x11.xlib.bit_swap = 1;
#endif
   if (((vis->class == TrueColor) || (vis->class == DirectColor)) &&
       (x_depth > 8))
     {
        buf->priv.mask.r = (DATA32)vis->red_mask;
        buf->priv.mask.g = (DATA32)vis->green_mask;
        buf->priv.mask.b = (DATA32)vis->blue_mask;
        if (buf->priv.x11.xlib.swap)
          {
             SWAP32(buf->priv.mask.r);
             SWAP32(buf->priv.mask.g);
             SWAP32(buf->priv.mask.b);
          }
     }
   else if ((vis->class == PseudoColor) || (vis->class == StaticColor) ||
            (vis->class == GrayScale) || (vis->class == StaticGray) ||
            (x_depth <= 8))
     {
        Convert_Pal_Mode pm = PAL_MODE_RGB332;

        if ((vis->class == GrayScale) || (vis->class == StaticGray))
          grayscale = 1;
        if (grayscale)
          {
             if (max_colors >= 256) pm = PAL_MODE_GRAY256;
             else if (max_colors >= 64)
               pm = PAL_MODE_GRAY64;
             else if (max_colors >= 16)
               pm = PAL_MODE_GRAY16;
             else if (max_colors >= 4)
               pm = PAL_MODE_GRAY4;
             else pm = PAL_MODE_MONO;
          }
        else
          {
             if (max_colors >= 256) pm = PAL_MODE_RGB332;
             else if (max_colors >= 216)
               pm = PAL_MODE_RGB666;
             else if (max_colors >= 128)
               pm = PAL_MODE_RGB232;
             else if (max_colors >= 64)
               pm = PAL_MODE_RGB222;
             else if (max_colors >= 32)
               pm = PAL_MODE_RGB221;
             else if (max_colors >= 16)
               pm = PAL_MODE_RGB121;
             else if (max_colors >= 8)
               pm = PAL_MODE_RGB111;
             else if (max_colors >= 4)
               pm = PAL_MODE_GRAY4;
             else pm = PAL_MODE_MONO;
          }
        /* FIXME: only alloc once per display+cmap */
        buf->priv.pal = evas_software_xlib_x_color_allocate
            (disp, cmap, vis, pm);
        if (!buf->priv.pal)
          {
             evas_xlib_swapper_free(buf->priv.swapper);
             free(buf);
             return NULL;
          }
     }
   d = evas_xlib_swapper_depth_get(buf->priv.swapper);
   if (buf->priv.pal)
     {
        if (buf->rot == 0 || buf->rot == 180)
          conv_func = evas_common_convert_func_get(0, buf->w, buf->h, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   buf->priv.pal->colors,
                                                   buf->rot);
        else if (buf->rot == 90 || buf->rot == 270)
          conv_func = evas_common_convert_func_get(0, buf->h, buf->w, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   buf->priv.pal->colors,
                                                   buf->rot);
     }
   else
     {
        if (buf->rot == 0 || buf->rot == 180)
          conv_func = evas_common_convert_func_get(0, buf->w, buf->h, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   PAL_MODE_NONE,
                                                   buf->rot);
        else if (buf->rot == 90 || buf->rot == 270)
          conv_func = evas_common_convert_func_get(0, buf->h, buf->w, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   PAL_MODE_NONE,
                                                   buf->rot);
     }
   if (!conv_func)
     {
        ERR("At depth: %i, RGB format mask: %08x %08x %08x, "
            "Palette mode: %i. "
            "Not supported by compiled in converters!",
            buf->priv.x11.xlib.depth,
            buf->priv.mask.r,
            buf->priv.mask.g,
            buf->priv.mask.b,
            buf->priv.pal ? (int)buf->priv.pal->colors : -1);
     }
   return buf;
}

/**
 * @brief Prepares a region of the Outbuf for updating.
 *
 * This function is called before rendering to a part of the canvas.
 * It clips the requested update region (x, y, w, h) to the Outbuf dimensions.
 *
 * If the Outbuf is configured for direct rendering (0 rotation, 32-bit ARGB,
 * specific masks), it attempts to map the Xlib swapper's buffer directly and
 * returns a pointer to an RGBA_Image representing this mapped region.
 * The coordinates (cx, cy, cw, ch) will be the same as the input (x, y, w, h)
 * relative to the Outbuf. Regions are stored in `buf->priv.onebuf_regions`.
 * `buf->priv.onebuf_regions` is an Eina_Array of Eina_Rectangle pointers,
 * where each rectangle defines a region that has been prepared for direct update.
 * Example:
 *   buf->priv.onebuf_regions = [
 *     Eina_Rectangle{x=10, y=10, w=100, h=50},
 *     Eina_Rectangle{x=150, y=60, w=80, h=80}
 *   ]
 *
 * Otherwise (if direct rendering is not possible due to rotation, format, etc.),
 * it allocates a new temporary RGBA_Image for the update. This image is stored
 * in `buf->priv.pending_writes`. The coordinates (cx, cy, cw, ch) will be (0, 0, w, h)
 * relative to this new temporary image.
 * `buf->priv.pending_writes` is an Eina_List of RGBA_Image pointers, where each
 * RGBA_Image has its `extended_info` field pointing to an Eina_Rectangle
 * that stores the original (x, y, w, h) of the update region.
 * Example:
 *   buf->priv.pending_writes = [
 *     RGBA_Image1 (extended_info: Eina_Rectangle{x=20, y=20, w=50, h=50}),
 *     RGBA_Image2 (extended_info: Eina_Rectangle{x=80, y=80, w=30, h=30})
 *   ]
 *
 * @param buf The Outbuf structure.
 * @param x The x-coordinate of the top-left corner of the region to update.
 * @param y The y-coordinate of the top-left corner of the region to update.
 * @param w The width of the region to update.
 * @param h The height of the region to update.
 * @param[out] cx Pointer to store the clipped x-coordinate of the usable update area.
 * @param[out] cy Pointer to store the clipped y-coordinate of the usable update area.
 * @param[out] cw Pointer to store the clipped width of the usable update area.
 * @param[out] ch Pointer to store the clipped height of the usable update area.
 * @return A pointer to an RGBA_Image that can be rendered into, or NULL on failure.
 *         This image is either a direct view into the X shared memory buffer
 *         or a temporary buffer that will be blitted later.
 */
void *
evas_software_xlib_swapbuf_new_region_for_update(Outbuf *buf, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch)
{
   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, buf->w, buf->h);
   if ((w <= 0) || (h <= 0)) return NULL;
   // if rotation is 0, and 32bit argb, we can use the buffer directly
   if ((buf->rot == 0) &&
       (buf->priv.mask.r == 0xff0000) &&
       (buf->priv.mask.g == 0x00ff00) &&
       (buf->priv.mask.b == 0x0000ff))
     {
        RGBA_Image *im;
        void *data;
        int bpl = 0;
        Eina_Rectangle *rect;

        im = buf->priv.onebuf;
        if (!im)
          {
             int ww = 0, hh = 0;
             int d, bpp;

             d = evas_xlib_swapper_depth_get(buf->priv.swapper);
             bpp = d / 8;

             data = evas_xlib_swapper_buffer_map(buf->priv.swapper, &bpl,
                                                 &(ww), &(hh));
             // To take stride into account, we do use bpl as the real image width, but return the real useful one.
             im = (RGBA_Image *)evas_cache_image_data(evas_common_image_cache_get(),
                                                      bpl / bpp, hh, data,
                                                      buf->priv.destination_alpha,
                                                      EVAS_COLORSPACE_ARGB8888);
             buf->priv.onebuf = im;
             if (!im) return NULL;
          }
        rect = eina_rectangle_new(x, y, w, h);
        if (!eina_array_push(&buf->priv.onebuf_regions, rect))
          {
             evas_cache_image_drop(&im->cache_entry);
             eina_rectangle_free(rect);
             return NULL;
          }

        // the clip region of the onebuf to render
        *cx = x;
        *cy = y;
        *cw = w;
        *ch = h;
        return im;
     }
   else
     {
        RGBA_Image *im;
        Eina_Rectangle *rect;

        rect = eina_rectangle_new(x, y, w, h);
        if (!rect) return NULL;
        im = (RGBA_Image *)evas_cache_image_empty(evas_common_image_cache_get());
        if (!im)
          {
             eina_rectangle_free(rect);
             return NULL;
          }
        im->cache_entry.flags.alpha |= buf->priv.destination_alpha ? 1 : 0;
        evas_cache_image_surface_alloc(&im->cache_entry, w, h);
        im->extended_info = rect;
        buf->priv.pending_writes = eina_list_append(buf->priv.pending_writes, im);

        // the region is the update image
        *cx = 0;
        *cy = 0;
        *cw = w;
        *ch = h;
        return im;
     }
   return NULL;
}

/**
 * @brief Flushes updated regions to the X server.
 *
 * This function takes all the regions prepared by
 * evas_software_xlib_swapbuf_new_region_for_update() and makes them visible.
 *
 * If `buf->priv.onebuf_regions` (direct rendering) has entries:
 *   It unmaps the shared memory buffer and calls the swapper to swap/copy
 *   the specified rectangular regions to the X drawable.
 *   The `onebuf_regions` array (Eina_Array of Eina_Rectangle pointers) is cleared.
 *
 * If `buf->priv.pending_writes` (indirect rendering) has entries:
 *   It iterates through the list of RGBA_Images. For each image:
 *     - The image data has already been pushed by evas_software_xlib_swapbuf_push_updated_region().
 *     - It calculates the destination rectangle on the X drawable, accounting for rotation.
 *     - These destination rectangles are collected.
 *   It then unmaps the shared memory buffer (if it was mapped by push_updated_region)
 *   and calls the swapper to swap/copy these collected rectangles to the X drawable.
 *   The `pending_writes` list (Eina_List of RGBA_Image pointers) is cleared, and
 *   associated image data and rectangles are freed.
 *
 * @param buf The Outbuf structure.
 * @param surface_damage Unused parameter.
 * @param buffer_damage Unused parameter.
 * @param render_mode The render mode; if EVAS_RENDER_MODE_ASYNC_INIT, the function returns early.
 */
void
evas_software_xlib_swapbuf_flush(Outbuf *buf, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) return;

   if (!buf->priv.pending_writes)
     {
        Eina_Rectangle *result, *rect;
        Eina_Array_Iterator it;
        unsigned int n, i;
        RGBA_Image *im;

        n = eina_array_count_get(&buf->priv.onebuf_regions);
        if (n == 0) return;
        result = alloca(n * sizeof(Eina_Rectangle));
        EINA_ARRAY_ITER_NEXT(&buf->priv.onebuf_regions, i, rect, it)
        {
           result[i] = *rect;
           eina_rectangle_free(rect);
        }
        evas_xlib_swapper_buffer_unmap(buf->priv.swapper);
        evas_xlib_swapper_swap(buf->priv.swapper, result, n);
        eina_array_clean(&buf->priv.onebuf_regions);
        im = buf->priv.onebuf;
        buf->priv.onebuf = NULL;
        if (im)
          evas_cache_image_drop(&im->cache_entry);
     }
   else
     {
        RGBA_Image *im;
        Eina_Rectangle *result;
        unsigned int n, i = 0;

        n = eina_list_count(buf->priv.pending_writes);
        if (n == 0) return;
        result = alloca(n * sizeof(Eina_Rectangle));
        EINA_LIST_FREE(buf->priv.pending_writes, im)
          {
             Eina_Rectangle *rect = im->extended_info;
             int x, y, w, h;

             x = rect->x; y = rect->y; w = rect->w; h = rect->h;
             if (buf->rot == 0)
               {
                  result[i].x = x;
                  result[i].y = y;
               }
             else if (buf->rot == 90)
               {
                  result[i].x = y;
                  result[i].y = buf->w - x - w;
               }
             else if (buf->rot == 180)
               {
                  result[i].x = buf->w - x - w;
                  result[i].y = buf->h - y - h;
               }
             else if (buf->rot == 270)
               {
                  result[i].x = buf->h - y - h;
                  result[i].y = x;
               }
             if ((buf->rot == 0) || (buf->rot == 180))
               {
                  result[i].w = w;
                  result[i].h = h;
               }
             else if ((buf->rot == 90) || (buf->rot == 270))
               {
                  result[i].w = h;
                  result[i].h = w;
               }
             eina_rectangle_free(rect);
             evas_cache_image_drop(&im->cache_entry);
             i++;
          }
        evas_xlib_swapper_buffer_unmap(buf->priv.swapper);
        evas_xlib_swapper_swap(buf->priv.swapper, result, n);
//        evas_xlib_swapper_swap(buf->priv.swapper, NULL, 0);
     }
}

/**
 * @brief Flushes idle buffers.
 * Currently, this function is a no-op.
 * @param buf The Outbuf (unused).
 */
void
evas_software_xlib_swapbuf_idle_flush(Outbuf *buf EINA_UNUSED)
{
   return;
}

/**
 * @brief Pushes an updated RGBA image region to the Outbuf's backing store.
 *
 * This function is typically called after rendering to a temporary buffer obtained
 * from evas_software_xlib_swapbuf_new_region_for_update() when direct rendering
 * was not possible. It converts the RGBA data from `update` into the native
 * format of the X server's drawable and writes it into the Xlib swapper's buffer.
 * It handles color conversion, palette lookup (if applicable), and rotation.
 *
 * The destination coordinates (x, y) and dimensions (w, h) are relative to the
 * original Outbuf, not the `update` image.
 *
 * @param buf The Outbuf structure.
 * @param update The RGBA_Image containing the pixel data to push.
 * @param x The target x-coordinate in the Outbuf.
 * @param y The target y-coordinate in the Outbuf.
 * @param w The width of the region to push.
 * @param h The height of the region to push.
 */
void
evas_software_xlib_swapbuf_push_updated_region(Outbuf *buf, RGBA_Image *update, int x, int y, int w, int h)
{
   Gfx_Func_Convert conv_func = NULL;
   Eina_Rectangle r = { 0, 0, 0, 0 }, pr;
   int d, bpl = 0, wid, bpp, rx = 0, ry = 0, ww = 0, hh = 0;
   DATA32 *src_data;
   DATA8 *dst_data;

   if (!buf->priv.pending_writes) return;
   d = evas_xlib_swapper_depth_get(buf->priv.swapper);
   bpp = d / 8;
   if (bpp <= 0) return;
   if (buf->priv.pal)
     {
        if ((buf->rot == 0) || (buf->rot == 180))
          conv_func = evas_common_convert_func_get(0, w, h, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   buf->priv.pal->colors,
                                                   buf->rot);
        else if ((buf->rot == 90) || (buf->rot == 270))
          conv_func = evas_common_convert_func_get(0, h, w, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   buf->priv.pal->colors,
                                                   buf->rot);
     }
   else
     {
        if ((buf->rot == 0) || (buf->rot == 180))
          conv_func = evas_common_convert_func_get(0, w, h, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   PAL_MODE_NONE,
                                                   buf->rot);
        else if ((buf->rot == 90) || (buf->rot == 270))
          conv_func = evas_common_convert_func_get(0, h, w, d,
                                                   buf->priv.mask.r,
                                                   buf->priv.mask.g,
                                                   buf->priv.mask.b,
                                                   PAL_MODE_NONE,
                                                   buf->rot);
     }
   if (!conv_func) return;
   if (buf->rot == 0)
     {
        r.x = x;
        r.y = y;
     }
   else if (buf->rot == 90)
     {
        r.x = y;
        r.y = buf->w - x - w;
     }
   else if (buf->rot == 180)
     {
        r.x = buf->w - x - w;
        r.y = buf->h - y - h;
     }
   else if (buf->rot == 270)
     {
        r.x = buf->h - y - h;
        r.y = x;
     }
   if ((buf->rot == 0) || (buf->rot == 180))
     {
        r.w = w;
        r.h = h;
     }
   else if ((buf->rot == 90) || (buf->rot == 270))
     {
        r.w = h;
        r.h = w;
     }
   src_data = update->image.data;
   if (!src_data) return;
   if ((buf->rot == 0) || (buf->rot == 180))
     {
        dst_data = evas_xlib_swapper_buffer_map(buf->priv.swapper, &bpl,
                                                &(ww), &(hh));
        if (!dst_data) return;
        if (buf->rot == 0)
          {
             RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h, 0, 0, ww, hh);
             dst_data += (bpl * r.y) + (r.x * bpp);
             w -= rx;
          }
        else if (buf->rot == 180)
          {
             pr = r;
             RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h, 0, 0, ww, hh);
             rx = pr.w - r.w; ry = pr.h - r.h;
             src_data += (update->cache_entry.w * ry) + rx;
             w -= rx;
          }
     }
   else
     {
        dst_data = evas_xlib_swapper_buffer_map(buf->priv.swapper, &bpl,
                                                &(ww), &(hh));
        if (!dst_data) return;
        if (buf->rot == 90)
          {
             pr = r;
             RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h, 0, 0, ww, hh);
             rx = pr.w - r.w; ry = pr.h - r.h;
             src_data += ry;
             w -= ry;
          }
        else if (buf->rot == 270)
          {
             pr = r;
             RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h, 0, 0, ww, hh);
             rx = pr.w - r.w; ry = pr.h - r.h;
             src_data += (update->cache_entry.w * rx);
             w -= ry;
          }
     }
   if ((r.w <= 0) || (r.h <= 0)) return;
   wid = bpl / bpp;
   dst_data += (bpl * r.y) + (r.x * bpp);
   if (buf->priv.pal)
     conv_func(src_data, dst_data,
               update->cache_entry.w - w,
               wid - r.w,
               r.w, r.h,
               x + rx, y + ry,
               buf->priv.pal->lookup);
   else
     conv_func(src_data, dst_data,
               update->cache_entry.w - w,
               wid - r.w,
               r.w, r.h,
               x + rx, y + ry,
               NULL);
}

/**
 * @brief Reconfigures an Outbuf with new dimensions, rotation, or depth.
 *
 * If the new parameters are different from the current ones, this function
 * updates the Outbuf's internal state and recreates the Xlib swapper
 * with the new configuration.
 *
 * @param buf The Outbuf to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation (0, 90, 180, 270).
 * @param depth The new output depth.
 */
void
evas_software_xlib_swapbuf_reconfigure(Outbuf *buf, int w, int h, int rot,
                                       Outbuf_Depth depth)
{
   if ((w == buf->w) && (h == buf->h) && (rot == buf->rot) &&
       (depth == buf->depth))
     return;
   buf->w = w;
   buf->h = h;
   buf->rot = rot;
   evas_xlib_swapper_free(buf->priv.swapper);
   if ((buf->rot == 0) || (buf->rot == 180))
     buf->priv.swapper = evas_xlib_swapper_new(buf->priv.x11.xlib.disp,
                                               buf->priv.x11.xlib.win,
                                               buf->priv.x11.xlib.vis,
                                               buf->priv.x11.xlib.depth,
                                               buf->w, buf->h);
   else if ((buf->rot == 90) || (buf->rot == 270))
     buf->priv.swapper = evas_xlib_swapper_new(buf->priv.x11.xlib.disp,
                                               buf->priv.x11.xlib.win,
                                               buf->priv.x11.xlib.vis,
                                               buf->priv.x11.xlib.depth,
                                               buf->h, buf->w);
}

/**
 * @brief Retrieves the current rotation of the Outbuf.
 * @param buf The Outbuf.
 * @return The current rotation value (0, 90, 180, or 270).
 */
int
evas_software_xlib_swapbuf_get_rot(Outbuf *buf)
{
   return buf->rot;
}

/**
 * @brief Checks if the destination buffer is configured to use an alpha channel.
 * @param buf The Outbuf.
 * @return EINA_TRUE if the destination has an alpha channel, EINA_FALSE otherwise.
 */
Eina_Bool
evas_software_xlib_swapbuf_alpha_get(Outbuf *buf)
{
   return buf->priv.destination_alpha;
}

/**
 * @brief Gets the buffer state from the Xlib swapper.
 *
 * This indicates how the swapper handles updates (e.g., full repaint, copy).
 *
 * @param buf The Outbuf.
 * @return The current Render_Output_Swap_Mode of the swapper.
 *         Returns MODE_FULL if the swapper is not initialized.
 */
Render_Output_Swap_Mode
evas_software_xlib_swapbuf_buffer_state_get(Outbuf *buf)
{
   if (!buf->priv.swapper) return MODE_FULL;
   return evas_xlib_swapper_buffer_state_get(buf->priv.swapper);
}

