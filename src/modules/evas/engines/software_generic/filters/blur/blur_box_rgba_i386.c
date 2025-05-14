#ifdef BUILD_MMX

/**
 * @brief Applies a horizontal box blur step to an RGBA image using MMX instructions.
 * @param src Pointer to the source image data (array of uint32_t RGBA pixels).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (array of uint32_t RGBA pixels).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Pointer to an array of 3 integers representing the blur radii for R, G, B channels.
 *              Example: {radius_r, radius_g, radius_b}
 * @param region The rectangular region of the image to process.
 *               Example: { .x = 0, .y = 0, .w = 100, .h = 100 }
 *
 * This function is intended to be an MMX-optimized version for the horizontal pass
 * of a box blur. Currently, it falls back to the generic C implementation.
 */
static inline void
_box_blur_rgba_horiz_step_mmx(const uint32_t* restrict src, int src_stride,
                              uint32_t* restrict dst, int dst_stride,
                              const int* restrict const radii,
                              Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Applies a vertical box blur step to an RGBA image using MMX instructions.
 * @param src Pointer to the source image data (array of uint32_t RGBA pixels).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (array of uint32_t RGBA pixels).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Pointer to an array of 3 integers representing the blur radii for R, G, B channels.
 *              Example: {radius_r, radius_g, radius_b}
 * @param region The rectangular region of the image to process.
 *               Example: { .x = 0, .y = 0, .w = 100, .h = 100 }
 *
 * This function is intended to be an MMX-optimized version for the vertical pass
 * of a box blur. Currently, it falls back to the generic C implementation.
 */
static inline void
_box_blur_rgba_vert_step_mmx(const uint32_t* restrict src, int src_stride,
                             uint32_t* restrict dst, int dst_stride,
                             const int* restrict const radii,
                             Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
