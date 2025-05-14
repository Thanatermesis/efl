#ifdef BUILD_NEON

/**
 * @brief Apply one horizontal step of a box blur to an alpha channel using NEON intrinsics.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Array containing the blur radius for each pixel row/column.
 *              The structure is typically [radius_for_row_0, radius_for_row_1, ...].
 * @param region The rectangular region of the image to process.
 *
 * This function performs the horizontal pass of a box blur optimized with NEON.
 * Currently, it falls back to the generic C implementation.
 */
static inline void
_box_blur_alpha_horiz_step_neon(const uint8_t* restrict src, int src_stride,
                                uint8_t* restrict dst, int dst_stride,
                                const int* restrict const radii,
                                Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_horiz_step(src, src_stride, dst, dst_stride, radii, region);
}

/**
 * @brief Apply one vertical step of a box blur to an alpha channel using NEON intrinsics.
 * @param src Pointer to the source image data (alpha channel).
 * @param src_stride Stride of the source image data in bytes.
 * @param dst Pointer to the destination image data (alpha channel).
 * @param dst_stride Stride of the destination image data in bytes.
 * @param radii Array containing the blur radius for each pixel row/column.
 *              The structure is typically [radius_for_col_0, radius_for_col_1, ...].
 * @param region The rectangular region of the image to process.
 *
 * This function performs the vertical pass of a box blur optimized with NEON.
 * Currently, it falls back to the generic C implementation.
 */
static inline void
_box_blur_alpha_vert_step_neon(const uint8_t* restrict src, int src_stride,
                               uint8_t* restrict dst, int dst_stride,
                               const int* restrict const radii,
                               Eina_Rectangle region)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_alpha_vert_step(src, src_stride, dst, dst_stride, radii, region);
}

#endif
