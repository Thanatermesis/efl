/* sub pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub-pixel span operations.
 *
 * This function is responsible for setting up function pointers or other
 * initializations required for MMX-accelerated operations on spans of pixels
 * where the source is a sub-pixel value and the destination is a pixel.
 */
static void
init_sub_pixel_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub-pixel point operations.
 *
 * This function handles the setup for MMX-accelerated operations on individual
 * pixels (points) where the source is a sub-pixel value and the destination
 * is a pixel.
 */
static void
init_sub_pixel_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* sub_rel pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub_rel-pixel span operations.
 *
 * This function is responsible for setting up function pointers or other
 * initializations required for MMX-accelerated operations on spans of pixels
 * where the source is a sub_rel-pixel value (subtractive, relative) and the
 * destination is a pixel.
 */
static void
init_sub_rel_pixel_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for sub_rel-pixel point operations.
 *
 * This function handles the setup for MMX-accelerated operations on individual
 * pixels (points) where the source is a sub_rel-pixel value (subtractive,
 * relative) and the destination is a pixel.
 */
static void
init_sub_rel_pixel_pt_funcs_mmx(void)
{
}
#endif
