/* add pixel x mask -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for adding pixel data with a mask over a span.
 *
 * This function is intended to set up pointers or perform other initialization
 * for MMX-optimized routines that handle operations on a horizontal line (span)
 * of pixels, where a source pixel is added to a destination pixel, modulated by a mask.
 *
 * @note This is a stub and would be implemented if MMX support is fully utilized.
 */
static void
init_add_pixel_mask_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for adding pixel data with a mask at a point.
 *
 * This function is intended to set up pointers or perform other initialization
 * for MMX-optimized routines that handle operations on a single pixel (point),
 * where a source pixel is added to a destination pixel, modulated by a mask.
 *
 * @note This is a stub and would be implemented if MMX support is fully utilized.
 */
static void
init_add_pixel_mask_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* add_rel pixel x mask -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative addition of pixel data with a mask over a span.
 *
 * This function is intended to set up pointers or perform other initialization
 * for MMX-optimized routines that handle operations on a horizontal line (span)
 * of pixels. The "rel" (relative) likely indicates that the addition operation
 * considers the destination pixel's current value in a specific way (e.g., clamping
 * or scaling relative to it) before adding the masked source pixel.
 *
 * @note This is a stub and would be implemented if MMX support is fully utilized.
 */
static void
init_add_rel_pixel_mask_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative addition of pixel data with a mask at a point.
 *
 * This function is intended to set up pointers or perform other initialization
 * for MMX-optimized routines that handle operations on a single pixel (point).
 * The "rel" (relative) likely indicates that the addition operation
 * considers the destination pixel's current value in a specific way (e.g., clamping
 * or scaling relative to it) before adding the masked source pixel.
 *
 * @note This is a stub and would be implemented if MMX support is fully utilized.
 */
static void
init_add_rel_pixel_mask_pt_funcs_mmx(void)
{
}
#endif
