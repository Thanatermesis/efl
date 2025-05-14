#ifndef _EVAS_SCALE_MAIN_H
#define _EVAS_SCALE_MAIN_H

/**
 * @brief Callback function type for scaled RGBA image operations with clipping.
 *
 * This callback is invoked by functions like evas_common_scale_rgba_in_to_out_clip_cb
 * for each rectangular segment after cutouts and clipping have been applied.
 * The implementer of this callback should perform the actual drawing or scaling
 * operation for the given segment.
 *
 * @param src The source RGBA_Image.
 * @param dst The destination RGBA_Image.
 * @param dc The RGBA_Draw_Context, potentially with an updated clip region
 *           specific to this callback invocation.
 * @param src_region_x The x-coordinate of the top-left corner of the source region.
 * @param src_region_y The y-coordinate of the top-left corner of the source region.
 * @param src_region_w The width of the source region.
 * @param src_region_h The height of the source region.
 * @param dst_region_x The x-coordinate of the top-left corner of the destination region.
 * @param dst_region_y The y-coordinate of the top-left corner of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 * @return EINA_TRUE on success, EINA_FALSE on failure. The calling function
 *         might aggregate these return values (e.g., using a bitwise OR).
 */
typedef Eina_Bool (*Evas_Common_Scale_In_To_Out_Clip_Cb)(RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

/**
 * @internal
 * @brief Initializes the common scaling subsystem.
 * @see evas_common_scale_init()
 */
EVAS_API void evas_common_scale_init                            (void);
EVAS_API void evas_common_scale_sample_init                     (void);
EVAS_API void evas_common_scale_sample_shutdown                 (void);

/**
 * @internal
 * @brief Performs a scaled blit with clipping, using a callback for each drawable region.
 *
 * This function acts as a high-level wrapper for scaling operations that
 * require handling of complex clipping and cutouts. It calculates the visible
 * rectangles and for each one, invokes the provided callback.
 *
 * @see evas_common_scale_rgba_in_to_out_clip_cb()
 * @see Evas_Common_Scale_In_To_Out_Clip_Cb
 */
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_cb          (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, Evas_Common_Scale_In_To_Out_Clip_Cb cb);
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_smooth      (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_sample      (RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

EVAS_API void evas_common_rgba_image_scalecache_dump(void);

EVAS_API void evas_common_scale_rgba_in_to_out_clip_sample_do   (const Cutout_Rects *reuse, const Eina_Rectangle *clip, RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);
EVAS_API void evas_common_scale_rgba_in_to_out_clip_smooth_do   (const Cutout_Rects *reuse, const Eina_Rectangle *clip, RGBA_Image *src, RGBA_Image *dst, RGBA_Draw_Context *dc, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);
EVAS_API void evas_common_scale_rgba_sample_draw                (RGBA_Image *src, RGBA_Image *dst, int dst_clip_x, int dst_clip_y, int dst_clip_w, int dst_clip_h, DATA32 mul_col, int render_op, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, RGBA_Image *mask, int mask_x, int mask_y);
EVAS_API void evas_common_scale_rgba_smooth_draw                (RGBA_Image *src, RGBA_Image *dst, int dst_clip_x, int dst_clip_y, int dst_clip_w, int dst_clip_h, DATA32 mul_col, int render_op, int src_region_x, int src_region_y, int src_region_w, int src_region_h, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h, RGBA_Image *mask_ie, int mask_x, int mask_y);
/**
 * @internal
 * @brief Prepares for a clipped scaling operation by calculating cutout rectangles.
 *
 * This is an optimization to compute the list of clipping rectangles to be
 * used by a subsequent drawing operation.
 *
 * @see evas_common_scale_rgba_in_to_out_clip_prepare()
 */
EVAS_API Eina_Bool evas_common_scale_rgba_in_to_out_clip_prepare     (Cutout_Rects **reuse, const RGBA_Image *src, const RGBA_Image *dst, RGBA_Draw_Context *dc, int dst_region_x, int dst_region_y, int dst_region_w, int dst_region_h);

#endif /* _EVAS_SCALE_MAIN_H */
