/* mask mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for mask-mask-color span operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-optimized routines that handle span-based drawing
 * or processing where two masks and a color are combined to produce a
 * destination output.
 *
 * @note This function is only compiled if BUILD_MMX is defined.
 *       Currently, it is a stub.
 */
static void
init_mask_mask_color_span_funcs_mmx(void)
{}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for mask-mask-color point operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-optimized routines that handle point-based drawing
 * or processing where two masks and a color are combined to produce a
 * destination output for individual pixels.
 *
 * @note This function is only compiled if BUILD_MMX is defined.
 *       Currently, it is a stub.
 */
static void
init_mask_mask_color_pt_funcs_mmx(void)
{}
#endif
