/* add color -> dst */

/**
 * @brief Initializes C-specific functions for adding color to a span of pixels.
 *
 * This function is responsible for setting up the necessary function pointers
 * or other configurations for operations that add a specified color to a
 * contiguous horizontal line (span) of destination pixels.
 */
static void
init_add_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific functions for adding color to a single pixel.
 *
 * This function sets up the C implementations for operations that add a
 * specified color to an individual destination pixel.
 */
static void
init_add_color_pt_funcs_c(void)
{
}

/*-----*/

/* add_rel color -> dst */

/**
 * @brief Initializes C-specific functions for relatively adding color to a span of pixels.
 *
 * This function is responsible for setting up the necessary function pointers
 * or other configurations for operations that add a color to a contiguous
 * horizontal line (span) of destination pixels, where the addition is relative
 * (e.g., increasing brightness or saturation by a certain amount).
 */
static void
init_add_rel_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific functions for relatively adding color to a single pixel.
 *
 * This function sets up the C implementations for operations that add a
 * color to an individual destination pixel in a relative manner.
 */
static void
init_add_rel_color_pt_funcs_c(void)
{
}
