#ifdef BUILD_SSE3

/**
 * @brief Apply horizontal box blur to the alpha channel using SSE3 instructions.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Array containing the blur radius for each pixel row.
 *              The array structure is expected to be `radii[region.h]`.
 *              Example: `radii = {radius_row_0, radius_row_1, ...}`
 * @param region The rectangular region of the image to process.
 *               Example: `region = { .x = 0, .y = 0, .w = 100, .h = 50 }`
 *
 * This function is intended to be an SSE3-optimized version for the
 * horizontal step of the box blur algorithm applied only to the alpha channel.
 * Currently, it falls back to the non-optimized version.
 */
static inline void
_box_blur_alpha_horiz_step_sse3(const uint8_t* restrict src, int src_stride,
                                uint8_t* restrict dst, int dst_stride,
                                const int* restrict const radii,
                                Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Apply vertical box blur to the alpha channel using SSE3 instructions.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Array containing the blur radius for each pixel column.
 *              The array structure is expected to be `radii[region.w]`.
 *              Example: `radii = {radius_col_0, radius_col_1, ...}`
 * @param region The rectangular region of the image to process.
 *               Example: `region = { .x = 0, .y = 0, .w = 100, .h = 50 }`
 *
 * This function is intended to be an SSE3-optimized version for the
 * vertical step of the box blur algorithm applied only to the alpha channel.
 * Currently, it falls back to the non-optimized version.
 */
static inline void
_box_blur_alpha_vert_step_sse3(const uint8_t* restrict src, int src_stride,
                               uint8_t* restrict dst, int dst_stride,
                               const int* restrict const radii,
                               Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
