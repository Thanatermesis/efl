/* sub pixel x color --> dst */

/**
 * @brief Initializes C-specific functions for span operations with sub-pixel precision and color.
 *
 * This function is responsible for setting up the necessary function pointers
 * or configurations for drawing horizontal spans of pixels where the start
 * position has sub-pixel accuracy and a specific color is applied to the
 * destination.
 */
static void
init_sub_pixel_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific functions for point operations with sub-pixel precision and color.
 *
 * This function sets up the mechanisms for drawing individual pixels (points)
 * where the position has sub-pixel accuracy and a specific color is applied
 * to the destination.
 */
static void
init_sub_pixel_color_pt_funcs_c(void)
{
}

/*-----*/

/* sub_rel pixel x color --> dst */

/**
 * @brief Initializes C-specific functions for span operations with sub-pixel relative positioning and color.
 *
 * This function configures drawing operations for horizontal spans of pixels.
 * 'sub_rel' implies that the sub-pixel positioning might be relative to a
 * previously established pixel grid or coordinate system, and a specific color
 * is applied to the destination.
 */
static void
init_sub_rel_pixel_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific functions for point operations with sub-pixel relative positioning and color.
 *
 * This function sets up the drawing of individual pixels (points) where
 * 'sub_rel' indicates sub-pixel positioning relative to some reference,
 * and a specific color is applied to the destination.
 */
static void
init_sub_rel_pixel_color_pt_funcs_c(void)
{
}
