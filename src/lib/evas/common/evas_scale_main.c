#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Initializes the common scaling subsystem.
 *
 * This function is called to set up the necessary resources for the scaling
 * operations. It currently does nothing but is kept for future extensions.
 */
EVAS_API void
evas_common_scale_init(void)
{
}

/**
 * @internal
 * @brief Prepares for a clipped scaling operation by calculating cutout rectangles.
 *
 * This function computes the final set of rectangles to be rendered by taking into
 * account the destination image boundaries, the destination region, and the
 * cutouts specified in the draw context. It is an optimization to avoid
 * recomputing clip lists inside a rendering loop.
 *
 * If there are no cutouts in the draw context, it performs simple intersection
 * checks and returns quickly. Otherwise, it calculates the intersection of all
 * clip regions and applies the cutouts.
 *
 * The resulting list of rectangles is stored in @p reuse, which may be a
 * pre-allocated list to avoid repeated memory allocations.
 *
 * @param[out] reuse A pointer to a Cutout_Rects pointer. The resulting list of
 *                   cutout rectangles will be stored here. This can be used
 *                   to reuse a previously allocated Cutout_Rects structure.
 * @param src The source image (currently unused).
 * @param dst The destination image.
 * @param dc The drawing context, containing clipping and cutout information.
 * @param dst_region_x The X-coordinate of the destination region.
 * @param dst_region_y The Y-coordinate of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 * @return EINA_TRUE if there is a visible area to be drawn, EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_common_scale_rgba_in_to_out_clip_prepare(Cutout_Rects **reuse, const RGBA_Image *src EINA_UNUSED,
					      const RGBA_Image *dst,
					      RGBA_Draw_Context *dc,
					      int dst_region_x, int dst_region_y,
					      int dst_region_w, int dst_region_h)
{
   /* handle cutouts here! */
   if ((dst_region_w <= 0) || (dst_region_h <= 0)) return EINA_FALSE;
   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h,
			 0, 0, dst->cache_entry.w, dst->cache_entry.h)))
     return EINA_FALSE;
   /* no cutouts - cut right to the chase */
   if (!dc->cutout.rects) return EINA_TRUE;

   evas_common_draw_context_clip_clip(dc, 0, 0, dst->cache_entry.w, dst->cache_entry.h);
   evas_common_draw_context_clip_clip(dc, dst_region_x, dst_region_y, dst_region_w, dst_region_h);
   /* our clip is 0 size.. abort */
   if ((dc->clip.w <= 0) || (dc->clip.h <= 0))
     return EINA_FALSE;
   *reuse = evas_common_draw_context_apply_cutouts(dc, *reuse);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Performs a scaled blit with clipping, using a callback for each drawable region.
 *
 * This function acts as a high-level wrapper for scaling operations that
 * require handling of complex clipping and cutouts. It calculates the visible
 * rectangles resulting from the intersection of the destination region and
 * any cutouts defined in the draw context.
 *
 * If no cutouts are present, it invokes the callback @p cb once for the entire
 * clipped destination region.
 *
 * If cutouts exist, it iterates through each resulting rectangular region,
 * setting the clip for that region in the draw context, and then invokes
 * the @p cb for that region. The original clip state of the draw context is
 * restored before the function returns.
 *
 * This approach allows the actual scaling logic (implemented in @p cb) to be
 * unaware of the complexities of cutout handling.
 *
 * @param src The source image.
 * @param dst The destination image.
 * @param dc The drawing context, which will be modified with new clip rectangles
 *           for each callback invocation.
 * @param src_region_x X-coordinate of the source region.
 * @param src_region_y Y-coordinate of the source region.
 * @param src_region_w Width of the source region.
 * @param src_region_h Height of the source region.
 * @param dst_region_x X-coordinate of the destination region.
 * @param dst_region_y Y-coordinate of the destination region.
 * @param dst_region_w Width of the destination region.
 * @param dst_region_h Height of the destination region.
 * @param cb The callback function to be executed for each clipped rectangle.
 * @return The bitwise OR of the return values from all calls to @p cb. Returns
 *         EINA_FALSE if the destination region is invalid or fully clipped.
 */
EVAS_API Eina_Bool
evas_common_scale_rgba_in_to_out_clip_cb(RGBA_Image *src, RGBA_Image *dst,
                                         RGBA_Draw_Context *dc,
                                         int src_region_x, int src_region_y,
                                         int src_region_w, int src_region_h,
                                         int dst_region_x, int dst_region_y,
                                         int dst_region_w, int dst_region_h,
                                         Evas_Common_Scale_In_To_Out_Clip_Cb cb)
{
   Cutout_Rect  *r;
   int          c, cx, cy, cw, ch;
   int          i;
   Eina_Bool ret = EINA_FALSE;

   /* handle cutouts here! */
   if ((dst_region_w <= 0) || (dst_region_h <= 0)) return EINA_FALSE;
   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h, 0, 0, dst->cache_entry.w, dst->cache_entry.h)))
     return EINA_FALSE;

   /* no cutouts - cut right to the chase */
   if (!dc->cutout.rects)
     {
        return cb(src, dst, dc,
                  src_region_x, src_region_y, src_region_w, src_region_h,
                  dst_region_x, dst_region_y, dst_region_w, dst_region_h);
     }

   /* save out clip info */
   c = dc->clip.use; cx = dc->clip.x; cy = dc->clip.y; cw = dc->clip.w; ch = dc->clip.h;
   evas_common_draw_context_clip_clip(dc, 0, 0, dst->cache_entry.w, dst->cache_entry.h);
   evas_common_draw_context_clip_clip(dc, dst_region_x, dst_region_y, dst_region_w, dst_region_h);

   /* our clip is 0 size.. abort */
   if ((dc->clip.w <= 0) || (dc->clip.h <= 0))
     {
        dc->clip.use = c; dc->clip.x = cx; dc->clip.y = cy; dc->clip.w = cw; dc->clip.h = ch;
        return EINA_FALSE;
     }

   dc->cache.rects = evas_common_draw_context_apply_cutouts(dc, dc->cache.rects);
   for (i = 0; i < dc->cache.rects->active; ++i)
     {
        r = dc->cache.rects->rects + i;
        evas_common_draw_context_set_clip(dc, r->x, r->y, r->w, r->h);
        ret |= cb(src, dst, dc,
                  src_region_x, src_region_y, src_region_w, src_region_h,
                  dst_region_x, dst_region_y, dst_region_w, dst_region_h);
     }
   evas_common_draw_context_cache_update(dc);
   /* restore clip info */
   dc->clip.use = c; dc->clip.x = cx; dc->clip.y = cy; dc->clip.w = cw; dc->clip.h = ch;

   return ret;
}
