/* add pixel x mask --> dst */

/**
 * @brief Initializes span functions for adding pixel values with a mask.
 *
 * This function is intended to set up dispatch tables or function pointers
 * for optimized span-based operations where a pixel's color is added to
 * a destination, modulated by a mask value.
 *
 * @note This function is currently not used.
 */
/* XXX: not used
static void
init_add_pixel_mask_span_funcs_c(void)
{
}
*/

/**
 * @brief Initializes point functions for adding pixel values with a mask.
 *
 * This function is intended to set up dispatch tables or function pointers
 * for optimized point-based (per-pixel) operations where a pixel's color
 * is added to a destination, modulated by a mask value.
 *
 * @note This function is currently not used.
 */
/* XXX: not used
static void
init_add_pixel_mask_pt_funcs_c(void)
{
}
*/

/*-----*/

/* add_rel pixel x mask --> dst */

/**
 * @brief Initializes span functions for relative addition of pixel values with a mask.
 *
 * This function is intended to set up dispatch tables or function pointers
 * for optimized span-based operations. "Relative addition" might imply
 * that the operation takes into account the existing destination pixel value
 * in a specific way beyond a simple sum, modulated by a mask.
 * For example, `dst = dst + (src * mask) / MAX_ALPHA`.
 */
static void
init_add_rel_pixel_mask_span_funcs_c(void)
{
}

/**
 * @brief Initializes point functions for relative addition of pixel values with a mask.
 *
 * This function is intended to set up dispatch tables or function pointers
 * for optimized point-based (per-pixel) operations. "Relative addition"
 * might imply that the operation takes into account the existing destination
 * pixel value in a specific way beyond a simple sum, modulated by a mask.
 * For example, `dst_pixel = dst_pixel + (src_pixel * mask_value) / MAX_ALPHA`.
 */
static void
init_add_rel_pixel_mask_pt_funcs_c(void)
{
}
