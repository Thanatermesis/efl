/* sub mask x color -> dst */

/**
 * @brief Initializes C-specific function pointers for span operations.
 *
 * This function sets up the necessary C implementations for drawing spans
 * where the operation is defined as: destination = color - mask.
 * It is typically called during Evas image object setup.
 */
static void
init_sub_mask_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific function pointers for point operations.
 *
 * This function sets up the necessary C implementations for drawing points
 * where the operation is defined as: destination = color - mask.
 * It is typically called during Evas image object setup.
 */
static void
init_sub_mask_color_pt_funcs_c(void)
{
}

/*-----*/

/* sub_rel mask x color -> dst */

/**
 * @brief Initializes C-specific function pointers for relative span operations.
 *
 * This function sets up the necessary C implementations for drawing spans
 * where the operation is defined as: destination = color - (relative mask).
 * "Relative" typically implies that the mask's alpha value modulates the
 * subtraction. It is typically called during Evas image object setup.
 */
static void
init_sub_rel_mask_color_span_funcs_c(void)
{
}

/**
 * @brief Initializes C-specific function pointers for relative point operations.
 *
 * This function sets up the necessary C implementations for drawing points
 * where the operation is defined as: destination = color - (relative mask).
 * "Relative" typically implies that the mask's alpha value modulates the
 * subtraction. It is typically called during Evas image object setup.
 */
static void
init_sub_rel_mask_color_pt_funcs_c(void)
{
}
