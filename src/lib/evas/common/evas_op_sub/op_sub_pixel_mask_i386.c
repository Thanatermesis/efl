/* sub pixel x mask -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub-pixel mask span operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for span operations (e.g., drawing a horizontal line of pixels)
 * that apply a mask at sub-pixel precision, using MMX optimizations.
 * The mask is applied to a destination buffer.
 */
static void
init_sub_pixel_mask_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub-pixel mask point operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for point operations (e.g., drawing a single pixel)
 * that apply a mask at sub-pixel precision, using MMX optimizations.
 * The mask is applied to a destination buffer.
 */
static void
init_sub_pixel_mask_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* sub_rel pixel x mask -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative sub-pixel mask span operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for span operations that apply a mask at a relative sub-pixel
 * position, using MMX optimizations. The mask is applied to a destination buffer.
 * "sub_rel" likely implies that the sub-pixel offset is relative to a pixel boundary.
 */
static void
init_sub_rel_pixel_mask_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative sub-pixel mask point operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for point operations that apply a mask at a relative sub-pixel
 * position, using MMX optimizations. The mask is applied to a destination buffer.
 * "sub_rel" likely implies that the sub-pixel offset is relative to a pixel boundary.
 */
static void
init_sub_rel_pixel_mask_pt_funcs_mmx(void)
{
}
#endif
