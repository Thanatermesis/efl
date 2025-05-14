/* sub color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for subtracting a color from a span of pixels.
 *
 * This function is intended to set up function pointers or other mechanisms
 * to use MMX-accelerated routines for operations where a solid color is
 * subtracted from each pixel in a horizontal line (span) of an image.
 * The result is written to the destination.
 */
static void
init_sub_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for subtracting a color from a single pixel.
 *
 * This function is intended to set up function pointers or other mechanisms
 * to use MMX-accelerated routines for operations where a solid color is
 * subtracted from an individual pixel.
 * The result is written to the destination.
 */
static void
init_sub_color_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* sub_rel color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for relative color subtraction from a span of pixels.
 *
 * This function is intended to set up function pointers or other mechanisms
 * to use MMX-accelerated routines for operations where a color is subtracted
 * from each pixel in a horizontal line (span) of an image, potentially with
 * relative adjustments or clamping.
 * The result is written to the destination.
 */
static void
init_sub_rel_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for relative color subtraction from a single pixel.
 *
 * This function is intended to set up function pointers or other mechanisms
 * to use MMX-accelerated routines for operations where a color is subtracted
 * from an individual pixel, potentially with relative adjustments or clamping.
 * The result is written to the destination.
 */
static void
init_sub_rel_color_pt_funcs_mmx(void)
{
}
#endif
