/* sub pixel x mask --> dst */

/**
 * @brief Initializes C-specific functions for span-based sub-pixel mask operations.
 * @note This function is currently not used.
 *
 * This function would typically set up function pointers or tables for
 * processing horizontal spans of pixels with sub-pixel accuracy using a mask.
 * "Sub-pixel" implies that the mask's position or influence is calculated
 * at a finer resolution than the pixel grid.
 */
/* XXX: not used
static void
init_sub_pixel_mask_span_funcs_c(void)
{
}
*/

/**
 * @brief Initializes C-specific functions for point-based sub-pixel mask operations.
 * @note This function is currently not used.
 *
 * This function would typically set up function pointers or tables for
 * processing individual pixels with sub-pixel accuracy using a mask.
 * "Point-based" refers to operations on single, discrete pixel locations.
 */
/* XXX: not used
static void
init_sub_pixel_mask_pt_funcs_c(void)
{
}
*/

/*-----*/

/* sub_rel pixel x mask --> dst */

/**
 * @brief Initializes C-specific functions for span-based sub-relative pixel mask operations.
 *
 * This function sets up function pointers or tables for processing
 * horizontal spans of pixels. "Sub-relative pixel" suggests that the mask
 * operations are relative to a sub-pixel position, potentially for effects
 * like anti-aliasing or precise texture mapping where the mask's influence
 * shifts at a sub-pixel level relative to a reference point.
 */
static void
init_sub_rel_pixel_mask_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific functions for point-based sub-relative pixel mask operations.
 *
 * This function sets up function pointers or tables for processing
 * individual pixels. "Sub-relative pixel" in a point-based context implies
 * that calculations for a single pixel take into account a mask positioned
 * or defined with sub-pixel precision relative to some reference.
 */
static void
init_sub_rel_pixel_mask_pt_funcs_c(void)
{
}
