#ifndef _EVAS_SCALE_SMOOTH_H
#define _EVAS_SCALE_SMOOTH_H

/**
 * @file evas_scale_smooth.h
 * @brief Header for Evas smooth scaling functions.
 *
 * This header declares functions related to smooth scaling of RGBA images,
 * including MMX and C implementations.
 */

/**
 * @brief Scales an RGBA image region to another with clipping using MMX.
 * @copydetails evas_common_scale_rgba_in_to_out_clip_smooth_mmx
 */
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_smooth_mmx  (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

/**
 * @brief Scales an RGBA image region to another with clipping using C.
 * @copydetails evas_common_scale_rgba_in_to_out_clip_smooth_c
 */
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_smooth_c    (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);
// Note: evas_common_scale_rgba_in_to_out_clip_smooth_neon is not declared here
// as it's typically conditionally compiled and used internally by the dispatcher
// evas_common_scale_rgba_in_to_out_clip_smooth.

#endif /* _EVAS_SCALE_SMOOTH_H */
