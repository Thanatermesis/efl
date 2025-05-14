/* mask pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for mask pixel color span operations.
 *
 * This function is called to set up pointers or perform any necessary
 * initialization for MMX-accelerated routines that apply a color to a
 * span of pixels based on a mask.
 */
static void
init_mask_pixel_color_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for mask pixel color point operations.
 *
 * This function is called to set up pointers or perform any necessary
 * initialization for MMX-accelerated routines that apply a color to
 * individual pixels based on a mask.
 */
static void
init_mask_pixel_color_pt_funcs_mmx(void)
{ }
#endif
