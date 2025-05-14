#ifdef BUILD_SSE3

/**
 * @brief Apply horizontal box blur step using SSE3 optimizations (currently falls back to generic).
 * @param src Pointer to the source image data (RGBA).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (RGBA).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Array containing the blur radius for each color channel (R, G, B, A).
 *              Example: {radius_r, radius_g, radius_b, radius_a}
 * @param region The rectangular region of the image to process.
 */
static inline void
_box_blur_rgba_horiz_step_sse3(const uint32_t* restrict src, int src_stride,
                               uint32_t* restrict dst, int dst_stride,
                               const int* restrict const radii,
                               Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Apply vertical box blur step using SSE3 optimizations (currently falls back to generic).
 * @param src Pointer to the source image data (RGBA).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (RGBA).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Array containing the blur radius for each color channel (R, G, B, A).
 *              Example: {radius_r, radius_g, radius_b, radius_a}
 * @param region The rectangular region of the image to process.
 */
static inline void
_box_blur_rgba_vert_step_sse3(const uint32_t* restrict src, int src_stride,
                              uint32_t* restrict dst, int dst_stride,
                              const int* restrict const radii,
                              Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
