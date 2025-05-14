#ifdef BUILD_NEON

/**
 * @brief Apply a horizontal box blur step using NEON optimizations.
 * @param src Pointer to the source image data (RGBA format).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (RGBA format).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Array containing the blur radius for each pixel row.
 *              The structure is expected to be an array of integers,
 *              where each integer represents the blur radius for the corresponding row.
 *              Example: `radii = {10, 10, 11, ...}` for a region height.
 * @param region The rectangular region of the image to process.
 *               Example: `region = { .x = 0, .y = 0, .w = 100, .h = 50 }`
 *
 * This function performs the horizontal pass of a box blur algorithm,
 * optimized using NEON intrinsics. It reads from `src` and writes to `dst`.
 * Currently, this is a placeholder calling the non-optimized version.
 */
static inline void
_box_blur_rgba_horiz_step_neon(const uint32_t* restrict src, int src_stride,
                               uint32_t* restrict dst, int dst_stride,
                               const int* restrict const radii,
                               Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Apply a vertical box blur step using NEON optimizations.
 * @param src Pointer to the source image data (RGBA format, typically the output of the horizontal step).
 * @param src_stride Stride of the source image data in pixels.
 * @param dst Pointer to the destination image data (RGBA format).
 * @param dst_stride Stride of the destination image data in pixels.
 * @param radii Array containing the blur radius for each pixel column.
 *              The structure is expected to be an array of integers,
 *              where each integer represents the blur radius for the corresponding column.
 *              Example: `radii = {10, 10, 11, ...}` for a region width.
 * @param region The rectangular region of the image to process.
 *               Example: `region = { .x = 0, .y = 0, .w = 100, .h = 50 }`
 *
 * This function performs the vertical pass of a box blur algorithm,
 * optimized using NEON intrinsics. It reads from `src` and writes to `dst`.
 * Currently, this is a placeholder calling the non-optimized version.
 */
static inline void
_box_blur_rgba_vert_step_neon(const uint32_t* restrict src, int src_stride,
                              uint32_t* restrict dst, int dst_stride,
                              const int* restrict const radii,
                              Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
