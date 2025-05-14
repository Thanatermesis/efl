{
   /**
    * @file evas_scale_smooth_scaler_down.c
    * @brief Implements downscaling for images using a smooth scaling algorithm.
    *
    * This code block handles the specifics of scaling an image region down to a smaller
    * destination region. It calculates the necessary pixel and alpha mappings,
    * selects the appropriate graphics rendering functions, and then dispatches
    * to a specialized scaling routine based on whether the scaling is primarily
    * vertical, horizontal, or both.
    */
   DATA32  **ypoints; /**< Array of pointers to source image rows, pre-calculated for Y-axis scaling. Each element `ypoints[i]` points to the source data row corresponding to the i-th destination row. */
   int     *xpoints; /**< Array of source X coordinates, pre-calculated for X-axis scaling. `xpoints[i]` is the source X for the i-th destination X. */
   int     *xapoints, *xapp; /**< Array of source X coordinates for alpha blending, pre-calculated for X-axis scaling. Similar to xpoints but potentially for alpha channel calculations. `xapp` is likely a pointer used to iterate over `xapoints`. */
   int     *yapoints, *yapp; /**< Array of source Y coordinates for alpha blending, pre-calculated for Y-axis scaling. Similar to ypoints but potentially for alpha channel calculations. `yapp` is likely a pointer used to iterate over `yapoints`. */
   DATA32  *buf, *src_data; /**< `buf` is a temporary scanline buffer for destination pixels. `src_data` points to the beginning of the source image pixel data. */

   RGBA_Gfx_Func      func, func2 = NULL;

   src_data = src->image.data;

   /* some maximum region sizes to avoid insane calc point tables */
   /** Calculate the mapping from destination X coordinates to source X coordinates. */
   SCALE_CALC_X_POINTS(xpoints, src_region_w, dst_region_w, dst_clip_x - dst_region_x, dst_clip_w);
   /** Calculate the mapping from destination Y coordinates to source Y image row pointers. */
   SCALE_CALC_Y_POINTS(ypoints, src_data, src_w, src_region_h, dst_region_h, dst_clip_y - dst_region_y, dst_clip_h);
   /** Calculate the mapping for alpha values along the X-axis. */
   SCALE_CALC_A_POINTS(xapoints, src_region_w, dst_region_w, dst_clip_x - dst_region_x, dst_clip_w);
   /** Calculate the mapping for alpha values along the Y-axis. */
   SCALE_CALC_A_POINTS(yapoints, src_region_h, dst_region_h, dst_clip_y - dst_region_y, dst_clip_h);

   /* a scanline buffer */
   buf = alloca(dst_clip_w * sizeof(DATA32));

   /**
    * Select the appropriate graphics rendering function based on whether a mask (`mask_ie`)
    * is involved and whether a specific color (`mul_col`) needs to be multiplied.
    * `func` is the primary rendering function.
    * `func2` is an auxiliary function, used in masked color multiplication scenarios.
    */
   if (!mask_ie) // If no mask is used
     {
        if (mul_col != 0xffffffff) // If color multiplication is enabled
          func = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
        else // No color multiplication
          func = evas_common_gfx_func_composite_pixel_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
     }
   else // If a mask is used
     {
        if (mul_col != 0xffffffff) // If color multiplication is enabled with a mask
          {
             // Primary function handles masking
             func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
             // Secondary function handles color multiplication (typically as a copy operation after masking)
             func2 = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, EVAS_RENDER_COPY);
          }
        else // Masking without color multiplication
          func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
     }

   /* scaling down vertically */
   /**
    * Based on the relationship between source and destination region dimensions,
    * include the appropriate specialized scaling implementation.
    * This section handles cases where the image is scaled down.
    */
   if ((dst_region_w >= src_region_w) && // Width is same or larger (no X downscale or upscale)
       (dst_region_h <  src_region_h))   // Height is smaller (Y downscale)
     {
        /** Only scale down vertically. Width might be stretching or same. */
#include "evas_scale_smooth_scaler_downy.c"
     }
   /* scaling down horizontally */
   else if ((dst_region_w < src_region_w) &&   // Width is smaller (X downscale)
            (dst_region_h >=  src_region_h))  // Height is same or larger (no Y downscale or upscale)
     {
        /** Only scale down horizontally. Height might be stretching or same. */
#include "evas_scale_smooth_scaler_downx.c"
     }
   /* scaling down both vertically & horizontally */
   else if ((dst_region_w < src_region_w) && // Width is smaller (X downscale)
            (dst_region_h <  src_region_h))  // Height is smaller (Y downscale)
     {
        /** Scale down both horizontally and vertically. */
#include "evas_scale_smooth_scaler_downx_downy.c"
     }
}
