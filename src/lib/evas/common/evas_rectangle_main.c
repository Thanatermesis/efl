#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_blend_private.h"

/**
 * @internal
 * @brief Draws a rectangle internally, handling clipping and pixel operations.
 *
 * This function is the core drawing routine for rectangles. It takes into account
 * the draw context's clipping region and mask, and uses the appropriate graphics
 * function (either Pixman or a software fallback) to render the rectangle.
 *
 * @param dst The destination image.
 * @param dc The drawing context.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
static void rectangle_draw_internal(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h);

/**
 * @brief Initializes common rectangle drawing functionalities.
 *
 * This function is called to set up any necessary resources or states
 * for drawing rectangles. Currently, it's a no-op.
 */
EVAS_API void
evas_common_rectangle_init(void)
{
}

/**
 * @brief Draws a rectangle using a callback function, handling cutouts.
 *
 * This function manages the drawing of a rectangle, taking into account
 * cutouts defined in the draw context. It iterates through the cutouts
 * and calls the provided callback function for each resulting drawable area.
 *
 * @param dst The destination image.
 * @param dc The drawing context.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param cb The callback function to perform the actual drawing for each segment.
 *           The callback signature is: `void (*cb)(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)`
 */
EVAS_API void
evas_common_rectangle_draw_cb(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h, Evas_Common_Rectangle_Draw_Cb cb)
{
   Cutout_Rect  *r;
   int          c, cx, cy, cw, ch;
   int          i;
   /* handle cutouts here! */

   if ((w <= 0) || (h <= 0)) return;
   if (!(RECTS_INTERSECT(x, y, w, h, 0, 0, dst->cache_entry.w, dst->cache_entry.h)))
     return;
   /* save out clip info */
   c = dc->clip.use; cx = dc->clip.x; cy = dc->clip.y; cw = dc->clip.w; ch = dc->clip.h;
   evas_common_draw_context_clip_clip(dc, 0, 0, dst->cache_entry.w, dst->cache_entry.h);
   /* no cutouts - cut right to the chase */
   if (!dc->cutout.rects)
     {
        cb(dst, dc, x, y, w, h);
     }
   else
     {
        evas_common_draw_context_clip_clip(dc, x, y, w, h);
        /* our clip is 0 size.. abort */
        if ((dc->clip.w > 0) && (dc->clip.h > 0))
          {
             dc->cache.rects = evas_common_draw_context_apply_cutouts(dc, dc->cache.rects);
             for (i = 0; i < dc->cache.rects->active; ++i)
               {
                  r = dc->cache.rects->rects + i;
                  evas_common_draw_context_set_clip(dc, r->x, r->y, r->w, r->h);
                  cb(dst, dc, x, y, w, h);
               }
             evas_common_draw_context_cache_update(dc);
          }
     }
   /* restore clip info */
   dc->clip.use = c; dc->clip.x = cx; dc->clip.y = cy; dc->clip.w = cw; dc->clip.h = ch;
}

/**
 * @brief Draws a rectangle using the internal drawing function.
 *
 * This is a convenience function that calls evas_common_rectangle_draw_cb
 * with the default internal rectangle drawing implementation.
 *
 * @param dst The destination image.
 * @param dc The drawing context.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
EVAS_API void
evas_common_rectangle_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   evas_common_rectangle_draw_cb(dst, dc, x, y, w, h, rectangle_draw_internal);
}

/**
 * @brief Prepares for drawing a rectangle by calculating cutout regions.
 *
 * This function checks if the rectangle is valid and visible. If cutouts
 * are present in the draw context, it calculates the resulting drawable
 * regions and stores them in the `reuse` parameter. This allows for
 * optimizing subsequent drawing operations by pre-calculating cutouts.
 *
 * @param reuse Pointer to a Cutout_Rects structure that will be populated
 *              with the calculated cutout regions. If no cutouts are applied,
 *              this might remain unchanged or be set to reflect the original rectangle.
 *              Example of Cutout_Rects structure:
 *              `typedef struct _Cutout_Rects Cutout_Rects;`
 *              `struct _Cutout_Rects {`
 *              `  Cutout_Rect *rects; // Array of Cutout_Rect`
 *              `  int active;         // Number of active rectangles in rects`
 *              `  int total;          // Total allocated size of rects`
 *              `};`
 *              Example of Cutout_Rect structure:
 *              `typedef struct _Cutout_Rect Cutout_Rect;`
 *              `struct _Cutout_Rect {`
 *              `  int x, y, w, h; // Geometry of the cutout rectangle`
 *              `};`
 * @param dst The destination image (const, as it's only used for dimensions).
 * @param dc The drawing context.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return EINA_TRUE if drawing is possible (rectangle is valid and visible),
 *         EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_common_rectangle_draw_prepare(Cutout_Rects **reuse, const RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   if ((w <= 0) || (h <= 0)) return EINA_FALSE;
   if (!(RECTS_INTERSECT(x, y, w, h, 0, 0, dst->cache_entry.w, dst->cache_entry.h)))
     return EINA_FALSE;
   /* save out clip info */
   evas_common_draw_context_clip_clip(dc, 0, 0, dst->cache_entry.w, dst->cache_entry.h);
   /* no cutouts - cut right to the chase */
   if (dc->cutout.rects)
     {
       evas_common_draw_context_clip_clip(dc, x, y, w, h);
       /* our clip is 0 size.. abort */
       if ((dc->clip.w > 0) && (dc->clip.h > 0))
         *reuse = evas_common_draw_context_apply_cutouts(dc, *reuse);
     }

   return EINA_TRUE;
}

/**
 * @brief Performs the actual drawing of a rectangle, using pre-calculated cutouts.
 *
 * This function draws a rectangle, potentially using a set of pre-calculated
 * cutout regions. If `reuse` is NULL, it draws the rectangle directly, clipped
 * by the `clip` rectangle. Otherwise, it iterates through the `reuse` cutouts,
 * intersects each with the `clip` rectangle, and draws the resulting segment.
 *
 * @param reuse Pre-calculated cutout regions from evas_common_rectangle_draw_prepare.
 *              If NULL, the rectangle is drawn without considering these pre-calculated cutouts.
 *              See evas_common_rectangle_draw_prepare for Cutout_Rects structure.
 * @param clip An Eina_Rectangle defining the overall clipping area for this draw operation.
 *             Example: `Eina_Rectangle clip_rect = { .x = 10, .y = 10, .w = 100, .h = 100 };`
 * @param dst The destination image.
 * @param dc The drawing context.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
EVAS_API void
evas_common_rectangle_draw_do(const Cutout_Rects *reuse,
                              const Eina_Rectangle *clip,
                              RGBA_Image *dst, RGBA_Draw_Context *dc,
                              int x, int y, int w, int h)
{
   Eina_Rectangle area;
   Cutout_Rect *r;
   int i;

   if (!reuse)
     {
        evas_common_draw_context_clip_clip(dc,
                                           clip->x, clip->y,
                                           clip->w, clip->h);
        rectangle_draw_internal(dst, dc, x, y, w, h);
        return;
     }

   for (i = 0; i < reuse->active; ++i)
     {
        r = reuse->rects + i;

        EINA_RECTANGLE_SET(&area, r->x, r->y, r->w, r->h);
        if (!eina_rectangle_intersection(&area, clip)) continue ;
        evas_common_draw_context_set_clip(dc, area.x, area.y, area.w, area.h);
        rectangle_draw_internal(dst, dc, x, y, w, h);
     }
}

static void
rectangle_draw_internal(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   RGBA_Gfx_Func func;
   int yy;
   DATA32 *ptr;
   DATA8 *mask;
   RGBA_Image *mask_ie = dc->clip.mask;

   if (!dst->image.data) return;
   RECTS_CLIP_TO_RECT(x, y, w, h, dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h);
   if ((w <= 0) || (h <= 0)) return;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_RECT
   pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY

   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;

   if ((dst->pixman.im) && (dc->col.pixman_color_image))
     {
        pixman_image_composite(op, dc->col.pixman_color_image, NULL,
                               dst->pixman.im, x, y, 0, 0,
                               x, y, w, h);
     }
   else
# endif
#endif
     {
        if (mask_ie)
          {
             func = evas_common_gfx_func_composite_mask_color_span_get(dc->col.col, dst->cache_entry.flags.alpha, w, dc->render_op);
             // Adjust clipping info
             if (EINA_UNLIKELY((x - dc->clip.mask_x) < 0))
               x = dc->clip.mask_x;
             if (EINA_UNLIKELY((y - dc->clip.mask_y) < 0))
               y = dc->clip.mask_y;
             if (EINA_UNLIKELY((x - dc->clip.mask_x + w) > (int)mask_ie->cache_entry.w))
               w = mask_ie->cache_entry.w + dc->clip.mask_x - x;
             if (EINA_UNLIKELY((y - dc->clip.mask_y + h) > (int)mask_ie->cache_entry.h))
               h = mask_ie->cache_entry.h + dc->clip.mask_y - y;
          }
        else
          func = evas_common_gfx_func_composite_color_span_get(dc->col.col, dst->cache_entry.flags.alpha, w, dc->render_op);
        ptr = dst->image.data + (y * dst->cache_entry.w) + x;
        for (yy = 0; yy < h; yy++)
          {
             if (mask_ie)
               {
                  mask = mask_ie->image.data8
                     + (y + yy - dc->clip.mask_y) * mask_ie->cache_entry.w
                     + (x - dc->clip.mask_x);
                  func(NULL, mask, dc->col.col, ptr, w);
               }
             else
               func(NULL, NULL, dc->col.col, ptr, w);

             ptr += dst->cache_entry.w;
          }
     }
}

/**
 * @brief Draws a rectangle with a specific RGBA color and render operation.
 *
 * This function provides a more direct way to draw a colored rectangle,
 * bypassing some of the higher-level context setup. It allows specifying
 * the color, render operation, and an optional mask directly.
 *
 * @param dst The destination image.
 * @param color The RGBA color to draw the rectangle with (e.g., 0xAARRGGBB).
 * @param render_op The rendering operation (e.g., _EVAS_RENDER_BLEND, _EVAS_RENDER_COPY).
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param mask_ie Optional mask image. If NULL, no mask is applied.
 * @param mask_x The x-offset of the mask image relative to the destination.
 * @param mask_y The y-offset of the mask image relative to the destination.
 */
EVAS_API void
evas_common_rectangle_rgba_draw(RGBA_Image *dst, DATA32 color, int render_op, int x, int y, int w, int h, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   RGBA_Gfx_Func func;
   DATA32 *ptr;
   DATA8 *mask;
   int yy;

   if (mask_ie)
     {
        func = evas_common_gfx_func_composite_mask_color_span_get(color, dst->cache_entry.flags.alpha, w, render_op);
        // Adjust clipping info
        if (EINA_UNLIKELY((x - mask_x) < 0))
          x = mask_x;
        if (EINA_UNLIKELY((y - mask_y) < 0))
          y = mask_y;
        if (EINA_UNLIKELY((x - mask_x + w) > (int)mask_ie->cache_entry.w))
          w = mask_ie->cache_entry.w + mask_x - x;
        if (EINA_UNLIKELY((y - mask_y + h) > (int)mask_ie->cache_entry.h))
          h = mask_ie->cache_entry.h + mask_y - y;
     }
   else
     func = evas_common_gfx_func_composite_color_span_get(color, dst->cache_entry.flags.alpha, w, render_op);

   ptr = dst->image.data + (y * dst->cache_entry.w) + x;
   for (yy = 0; yy < h; yy++)
     {
        if (mask_ie)
          {
             mask = mask_ie->image.data8
                + (y + yy - mask_y) * mask_ie->cache_entry.w
                + (x - mask_x);
             func(NULL, mask, color, ptr, w);
          }
        else
          func(NULL, NULL, color, ptr, w);

        ptr += dst->cache_entry.w;
     }
}
