/**
 * @brief Scales an RGBA image from a source region to a destination region with clipping and optional masking.
 *
 * This function implements image scaling (both upscaling and downscaling)
 * between a source image (`src`) and a destination image (`dst`). It handles
 * specific regions within these images and applies clipping to the destination.
 * An optional mask image (`mask_ie`) can be provided to further constrain
 * the drawing area.
 *
 * @param src Pointer to the source RGBA_Image structure.
 *            Example: `src->image.data` contains pixel data, `src->cache_entry.w` is width.
 * @param dst Pointer to the destination RGBA_Image structure.
 *            Example: `dst->image.data` contains pixel data, `dst->cache_entry.w` is width.
 * @param dst_clip_x The x-coordinate of the destination clipping rectangle.
 * @param dst_clip_y The y-coordinate of the destination clipping rectangle.
 * @param dst_clip_w The width of the destination clipping rectangle.
 * @param dst_clip_h The height of the destination clipping rectangle.
 * @param mul_col A color multiplier (DATA32 format) applied during rendering.
 * @param render_op The rendering operation to perform (e.g., copy, blend).
 * @param src_region_x The x-coordinate of the source region.
 * @param src_region_y The y-coordinate of the source region.
 * @param src_region_w The width of the source region.
 * @param src_region_h The height of the source region.
 * @param dst_region_x The x-coordinate of the destination region.
 * @param dst_region_y The y-coordinate of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 * @param mask_ie Optional pointer to an RGBA_Image structure to be used as a mask.
 *                If NULL, no mask is applied.
 * @param mask_x The x-offset for the mask image relative to the destination.
 * @param mask_y The y-offset for the mask image relative to the destination.
 */
void
SCALE_FUNC(RGBA_Image *src, RGBA_Image *dst, int dst_clip_x, int dst_clip_y, int dst_clip_w, int dst_clip_h, DATA32 mul_col, int render_op, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   DATA32  *dst_ptr;
   int      src_w, src_h, dst_w, dst_h;

   /* Ensure source and destination image data are valid. */
   if ((!src->image.data) || (!dst->image.data)) return;
   /* Check for intersection between destination region and destination image bounds. */
   if (!(RECTS_INTERSECT(dst_region_x, dst_region_y, dst_region_w, dst_region_h,
                         0, 0, dst->cache_entry.w, dst->cache_entry.h))) return;
   /* Check for intersection between source region and source image bounds. */
   if (!(RECTS_INTERSECT(src_region_x, src_region_y, src_region_w, src_region_h,
                         0, 0, src->cache_entry.w, src->cache_entry.h))) return;

   /* Cache source and destination image dimensions. */
   src_w = src->cache_entry.w;
   src_h = src->cache_entry.h;
   dst_w = dst->cache_entry.w;
   dst_h = dst->cache_entry.h;

   /* Adjust destination clipping rectangle to be within destination image bounds (0,0). */
   if (dst_clip_x < 0)
     {
        dst_clip_w += dst_clip_x;
        dst_clip_x = 0;
     }
   if (dst_clip_y < 0)
     {
        dst_clip_h += dst_clip_y;
        dst_clip_y = 0;
     }

   /* If clipping dimensions are non-positive, there's nothing to render. */
   if ((dst_clip_w <= 0) || (dst_clip_h <= 0)) return;
   /* Further adjust destination clipping rectangle to ensure it does not exceed destination image dimensions. */
   if ((dst_clip_x + dst_clip_w) > dst_w) dst_clip_w = dst_w - dst_clip_x;
   if ((dst_clip_y + dst_clip_h) > dst_h) dst_clip_h = dst_h - dst_clip_y;

   /* Clip the destination clipping rectangle against the destination region. */
   if (dst_clip_x < dst_region_x)
     {
	dst_clip_w += dst_clip_x - dst_region_x;
	dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > (dst_region_x + dst_region_w))
     dst_clip_w = dst_region_x + dst_region_w - dst_clip_x;
   if (dst_clip_y < dst_region_y)
     {
	dst_clip_h += dst_clip_y - dst_region_y;
	dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > (dst_region_y + dst_region_h))
     dst_clip_h = dst_region_y + dst_region_h - dst_clip_y;

   if ((src_region_w <= 0) || (src_region_h <= 0) ||
       (dst_region_w <= 0) || (dst_region_h <= 0) ||
       (dst_clip_w <= 0) || (dst_clip_h <= 0))
     return;

   /* Sanitize source and destination regions for X coordinate. */
   /* This adjusts the destination region if the source region starts outside the source image (negative x). */
   if (src_region_x < 0)
     {
        /* Adjust destination region x and width based on how much src_region_x is negative. */
	dst_region_x -= (src_region_x * dst_region_w) / src_region_w;
	dst_region_w += (src_region_x * dst_region_w) / src_region_w;
	src_region_w += src_region_x; /* Adjust source region width. */
	src_region_x = 0; /* Clamp source region x to 0. */
     }
   /* If source region is completely outside source image width, nothing to render. */
   if (src_region_x >= src_w) return;
   /* If source region extends beyond source image width, clip it and adjust destination width proportionally. */
   if ((src_region_x + src_region_w) > src_w)
     {
	dst_region_w = (dst_region_w * (src_w - src_region_x)) / (src_region_w);
	src_region_w = src_w - src_region_x;
     }
   /* If destination or source region width is non-positive, nothing to render. */
   if (dst_region_w <= 0) return;
   if (src_region_w <= 0) return;

   /* Re-apply clipping for X after source region sanitization, ensuring it's within destination bounds and region. */
   if (dst_clip_x < 0)
     {
	dst_clip_w += dst_clip_x;
	dst_clip_x = 0;
     }
   if (dst_clip_w <= 0) return;
   if (dst_clip_x >= dst_w) return;
   if (dst_clip_x < dst_region_x)
     {
	dst_clip_w += (dst_clip_x - dst_region_x);
	dst_clip_x = dst_region_x;
     }
   if ((dst_clip_x + dst_clip_w) > dst_w)
     {
	dst_clip_w = dst_w - dst_clip_x;
     }
   if (dst_clip_w <= 0) return;

   /* Sanitize source and destination regions for Y coordinate. */
   /* This adjusts the destination region if the source region starts outside the source image (negative y). */
   if (src_region_y < 0)
     {
        /* Adjust destination region y and height based on how much src_region_y is negative. */
	dst_region_y -= (src_region_y * dst_region_h) / src_region_h;
	dst_region_h += (src_region_y * dst_region_h) / src_region_h;
	src_region_h += src_region_y; /* Adjust source region height. */
	src_region_y = 0; /* Clamp source region y to 0. */
     }
   /* If source region is completely outside source image height, nothing to render. */
   if (src_region_y >= src_h) return;
   /* If source region extends beyond source image height, clip it and adjust destination height proportionally. */
   if ((src_region_y + src_region_h) > src_h)
     {
	dst_region_h = (dst_region_h * (src_h - src_region_y)) / (src_region_h);
	src_region_h = src_h - src_region_y;
     }
   /* If destination or source region height is non-positive, nothing to render. */
   if (dst_region_h <= 0) return;
   if (src_region_h <= 0) return;

   /* Re-apply clipping for Y after source region sanitization, ensuring it's within destination bounds and region. */
   if (dst_clip_y < 0)
     {
	dst_clip_h += dst_clip_y;
	dst_clip_y = 0;
     }
   if (dst_clip_h <= 0) return;
   if (dst_clip_y >= dst_h) return;
   if (dst_clip_y < dst_region_y)
     {
	dst_clip_h += (dst_clip_y - dst_region_y);
	dst_clip_y = dst_region_y;
     }
   if ((dst_clip_y + dst_clip_h) > dst_h)
     {
	dst_clip_h = dst_h - dst_clip_y;
     }
   if (dst_clip_h <= 0) return;

   /* Impose some maximum region sizes to prevent excessive memory allocation
    * or computation time for point tables in scaler implementations. */
   if (dst_clip_w > 65536) return;
   if (dst_clip_h > 65536) return;
   if (dst_region_w > (65536 * 1024)) return;
   if (dst_region_h > (65536 * 1024)) return;

   /* figure out dst jump
    * NB: Unused currently, so commented out */
//   dst_jump = dst_w - dst_clip_w;

   /* figure out dest start ptr */
   dst_ptr = dst->image.data + dst_clip_x + (dst_clip_y * dst_w);

   /* If a mask image is provided, adjust the destination clipping rectangle
    * to be within the bounds of the mask image, considering the mask's offset. */
   if (mask_ie)
     {
        // Adjust clipping info based on the mask
        /* Ensure clip start is not before mask start */
        if (EINA_UNLIKELY((dst_clip_x - mask_x) < 0))
          dst_clip_x = mask_x;
        if (EINA_UNLIKELY((dst_clip_y - mask_y) < 0))
          dst_clip_y = mask_y;
        /* Ensure clip end does not exceed mask dimensions */
        if (EINA_UNLIKELY((dst_clip_x - mask_x + dst_clip_w) > (int)mask_ie->cache_entry.w))
          dst_clip_w = mask_ie->cache_entry.w - dst_clip_x + mask_x;
        if (EINA_UNLIKELY((dst_clip_y - mask_y + dst_clip_h) > (int)mask_ie->cache_entry.h))
          dst_clip_h = mask_ie->cache_entry.h - dst_clip_y + mask_y;
     }

/* FIXME:
 *
 * things to do later for speedups:
 *
 * break upscale into 3 cases (as listed below - up:up, 1:up, up:1)
 *
 * break downscale into more cases (as listed below)
 *
 * roll func (blend/copy/cultiply/cmod) code into inner loop of scaler.
 * (578 fps vs 550 in mmx upscale in evas demo - this means probably
 *  a good 10-15% speedup over the func call, but means massively larger
 *  code)
 *
 * anything involving downscaling has no mmx equivalent code and maybe the
 * C could do with a little work.
 *
 * ---------------------------------------------------------------------------
 *
 * (1 = no scaling (1:1 ratio), + = scale up, - = scale down)
 * (* == fully optimised mmx, # = fully optimised C)
 *
 * h:v mmx C
 *
 * 1:1 *   #
 *
 * +:+ *   #
 * 1:+ *   #
 * +:1 *   #
 *
 * 1:-
 * -:1
 * +:-
 * -:+
 * -:-
 *
 */

   /* if 1:1 scale */
   /* Determine scaling strategy based on region sizes. */
   if ((dst_region_w == src_region_w) &&
       (dst_region_h == src_region_h))
     {
       /* Case 1: No scaling (1:1 ratio).
        * The source and destination regions have the same dimensions. */
#include "evas_scale_smooth_scaler_noscale.c"
     }
   else
     {
	/* Case 2: Scaling is required. */
	/* scaling up only - dont need anything except original */
//	if ((!dc->anti_alias) || ((dst_region_w >= src_region_w) && (dst_region_h >= src_region_h)))
	if (((dst_region_w >= src_region_w) && (dst_region_h >= src_region_h)))
	  {
            /* Subcase 2a: Upscaling or mixed scaling where both dimensions are scaled up or one is scaled up and the other is 1:1. */
#include "evas_scale_smooth_scaler_up.c"
	     return;
	  }
	else
	  /* scaling down... funkiness */
	  {
            /* Subcase 2b: Downscaling or mixed scaling involving at least one dimension being scaled down. */
#include "evas_scale_smooth_scaler_down.c"
	     return;
	  }
     }
}
