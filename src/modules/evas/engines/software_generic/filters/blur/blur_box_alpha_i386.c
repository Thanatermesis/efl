#ifdef BUILD_MMX

/**
 * @brief Performs a horizontal box blur step on alpha channel data using MMX optimization.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Pointer to an array of integers representing blur radii.
 *              The exact interpretation depends on the calling context,
 *              typically related to the size of the box kernel.
 * @param region The rectangular region of the image to process.
 *               Example: { .x = 0, .y = 0, .w = 100, .h = 100 }
 *
 * This function is intended to be an MMX-optimized version of
 * _box_blur_alpha_horiz_step.
 */
static inline void
_box_blur_alpha_horiz_step_mmx(const uint8_t* restrict src, int src_stride,
                               uint8_t* restrict dst, int dst_stride,
                               const int* restrict const radii,
                               Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Performs a vertical box blur step on alpha channel data using MMX optimization.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Pointer to an array of integers representing blur radii.
 *              The exact interpretation depends on the calling context,
 *              typically related to the size of the box kernel.
 * @param region The rectangular region of the image to process.
 *               Example: { .x = 0, .y = 0, .w = 100, .h = 100 }
 *
 * This function is intended to be an MMX-optimized version of
 * _box_blur_alpha_vert_step.
 */
static inline void
_box_blur_alpha_vert_step_mmx(const uint8_t* restrict src, int src_stride,
                              uint8_t* restrict dst, int dst_stride,
                              const int* restrict const radii,
                              Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
