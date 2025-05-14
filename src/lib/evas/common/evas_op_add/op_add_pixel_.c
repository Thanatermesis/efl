/* add pixel --> dst */

/**
 * @brief Initializes span-based pixel addition functions.
 *
 * This function is responsible for setting up any necessary function pointers
 * or data structures for performing pixel addition operations on spans of pixels
 * directly to a destination buffer.
 */
static void
init_add_pixel_span_funcs_c(void)
{
}

/**
 * @brief Initializes point-based pixel addition functions.
 *
 * This function is responsible for setting up any necessary function pointers
 * or data structures for performing pixel addition operations on individual pixels
 * directly to a destination buffer.
 */
static void
init_add_pixel_pt_funcs_c(void)
{
}

/*-----*/

/* add_rel pixel --> dst */

/**
 * @brief Initializes span-based relative pixel addition functions.
 *
 * This function is responsible for setting up any necessary function pointers
 * or data structures for performing relative pixel addition operations on spans
 * of pixels to a destination buffer. "Relative" implies the operation might
 * modify existing pixel values based on the input, rather than a direct overwrite.
 */
static void
init_add_rel_pixel_span_funcs_c(void)
{
}

/**
 * @brief Initializes point-based relative pixel addition functions.
 *
 * This function is responsible for setting up any necessary function pointers
 * or data structures for performing relative pixel addition operations on
 * individual pixels to a destination buffer. "Relative" implies the operation
 * might modify existing pixel values based on the input.
 */
static void
init_add_rel_pixel_pt_funcs_c(void)
{
}
