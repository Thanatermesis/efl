/* add mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for span operations
 *        that add a color to a destination, masked by a given mask.
 *
 * This function is intended to set up pointers to MMX-optimized routines
 * for processing horizontal spans of pixels.
 */
static void
init_add_mask_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for point operations
 *        that add a color to a destination pixel, masked by a given mask.
 *
 * This function is intended to set up pointers to MMX-optimized routines
 * for processing individual pixels.
 */
static void
init_add_mask_color_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* add_rel mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for span operations
 *        that add a color to a destination (relative addition),
 *        masked by a given mask.
 *
 * This function is intended to set up pointers to MMX-optimized routines
 * for processing horizontal spans of pixels with relative color addition.
 */
static void
init_add_rel_mask_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for point operations
 *        that add a color to a destination pixel (relative addition),
 *        masked by a given mask.
 *
 * This function is intended to set up pointers to MMX-optimized routines
 * for processing individual pixels with relative color addition.
 */
static void
init_add_rel_mask_color_pt_funcs_mmx(void)
{
}
#endif
