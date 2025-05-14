/* add pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for adding pixel color over a span.
 *
 * This function is intended to set up optimized routines for operations that
 * add a color to a horizontal span of pixels, if MMX is available.
 */
static void
init_add_pixel_color_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for adding pixel color at a point.
 *
 * This function is intended to set up optimized routines for operations that
 * add a color to a single pixel, if MMX is available.
 */
static void
init_add_pixel_color_pt_funcs_mmx(void)
{ }
#endif

/*-----*/

/* add_rel pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for relative addition of pixel color over a span.
 *
 * This function is intended to set up optimized routines for operations that
 * perform a relative addition of a color to a horizontal span of pixels,
 * if MMX is available. "Relative" might imply a blending operation or
 * addition with clamping.
 */
static void
init_add_rel_pixel_color_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for relative addition of pixel color at a point.
 *
 * This function is intended to set up optimized routines for operations that
 * perform a relative addition of a color to a single pixel, if MMX is available.
 * "Relative" might imply a blending operation or addition with clamping.
 */
static void
init_add_rel_pixel_color_pt_funcs_mmx(void)
{ }
#endif
