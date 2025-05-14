/* sub color -> dst */

/**
 * @brief Initializes C-specific function pointers for span-based color subtraction operations.
 *
 * This function is intended to set up optimized C implementations for operations
 * that subtract a color from a destination span of pixels.
 */
static void
init_sub_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific function pointers for point-based color subtraction operations.
 *
 * This function is intended to set up optimized C implementations for operations
 * that subtract a color from individual destination pixels.
 */
static void
init_sub_color_pt_funcs_c(void)
{
}

/*-----*/

/* sub_rel color -> dst */

/**
 * @brief Initializes C-specific function pointers for span-based relative color subtraction operations.
 *
 * This function is intended to set up optimized C implementations for operations
 * that perform a relative subtraction of a color from a destination span of pixels.
 * "Relative" might imply a scaled or modulated subtraction.
 */
static void
init_sub_rel_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific function pointers for point-based relative color subtraction operations.
 *
 * This function is intended to set up optimized C implementations for operations
 * that perform a relative subtraction of a color from individual destination pixels.
 * "Relative" might imply a scaled or modulated subtraction.
 */
static void
init_sub_rel_color_pt_funcs_c(void)
{
}
