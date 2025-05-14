/* sub mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for subtracting a color from a destination, masked, for spans.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated operations that subtract a color value
 * from destination pixels over a horizontal span, applying a mask.
 * The operation can be conceptualized as: dst = (dst - color) * mask.
 */
static void
init_sub_mask_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for subtracting a color from a destination, masked, for single points.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated operations that subtract a color value
 * from a single destination pixel, applying a mask.
 * The operation can be conceptualized as: dst = (dst - color) * mask.
 */
static void
init_sub_mask_color_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* sub_rel mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for relative subtraction of a color from a destination, masked, for spans.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated operations that perform a relative
 * subtraction of a color value from destination pixels over a horizontal span,
 * applying a mask. The "rel" might imply a specific type of subtraction,
 * possibly involving alpha blending or other relative adjustments.
 */
static void
init_sub_rel_mask_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for relative subtraction of a color from a destination, masked, for single points.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated operations that perform a relative
 * subtraction of a color value from a single destination pixel,
 * applying a mask. The "rel" might imply a specific type of subtraction,
 * possibly involving alpha blending or other relative adjustments.
 */
static void
init_sub_rel_mask_color_pt_funcs_mmx(void)
{
}
#endif
