/* add pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for "add_pixel" span operations.
 *
 * This function is intended to set up optimized MMX routines for operations
 * that add pixel data across a horizontal span of pixels.
 * It is called during initialization if MMX support is compiled and available.
 */
static void
init_add_pixel_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for "add_pixel" point operations.
 *
 * This function is intended to set up optimized MMX routines for operations
 * that add pixel data at a single point (individual pixel).
 * It is called during initialization if MMX support is compiled and available.
 */
static void
init_add_pixel_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* add_rel pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for "add_rel_pixel" span operations.
 *
 * This function is intended to set up optimized MMX routines for operations
 * that add relative pixel data across a horizontal span of pixels.
 * "add_rel" typically implies a blending operation where the source pixel
 * modifies the destination pixel relative to its current value.
 * It is called during initialization if MMX support is compiled and available.
 */
static void
init_add_rel_pixel_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific function pointers for "add_rel_pixel" point operations.
 *
 * This function is intended to set up optimized MMX routines for operations
 * that add relative pixel data at a single point (individual pixel).
 * "add_rel" typically implies a blending operation where the source pixel
 * modifies the destination pixel relative to its current value.
 * It is called during initialization if MMX support is compiled and available.
 */
static void
init_add_rel_pixel_pt_funcs_mmx(void)
{
}
#endif
