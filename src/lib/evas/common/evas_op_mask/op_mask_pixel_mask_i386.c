/* mask pixel x mask --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for mask pixel operations on spans.
 *
 * This function is responsible for setting up any necessary function pointers
 * or tables for MMX-optimized routines that operate on horizontal spans of pixels,
 * applying a mask to them.
 *
 * @note This function is only compiled if BUILD_MMX is defined.
 *       Currently, it is a stub.
 */
static void
init_mask_pixel_mask_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for mask pixel operations on individual points.
 *
 * This function is responsible for setting up any necessary function pointers
 * or tables for MMX-optimized routines that operate on single pixels (points),
 * applying a mask to them.
 *
 * @note This function is only compiled if BUILD_MMX is defined.
 *       Currently, it is a stub.
 */
static void
init_mask_pixel_mask_pt_funcs_mmx(void)
{ }
#endif
