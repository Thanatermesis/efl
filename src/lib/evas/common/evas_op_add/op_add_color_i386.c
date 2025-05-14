/* add color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for adding color to a span of pixels.
 *
 * This function is responsible for setting up pointers or dispatch mechanisms
 * to use MMX-optimized routines for operations that add a specific color
 * to a horizontal line (span) of destination pixels.
 */
static void
init_add_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for adding color to individual pixels.
 *
 * This function sets up MMX-optimized routines for operations that add a
 * specific color to individual destination pixels (points).
 */
static void
init_add_color_pt_funcs_mmx(void)
{
}
#endif

/*-----*/

/* add_rel color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative color addition to a span of pixels.
 *
 * This function is responsible for setting up pointers or dispatch mechanisms
 * to use MMX-optimized routines for operations that add a color relative
 * to the existing color of a horizontal line (span) of destination pixels.
 * "Relative" might imply operations like blending or conditional addition.
 */
static void
init_add_rel_color_span_funcs_mmx(void)
{
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-specific functions for relative color addition to individual pixels.
 *
 * This function sets up MMX-optimized routines for operations that add a
 * color relative to the existing color of individual destination pixels (points).
 * "Relative" might imply operations like blending or conditional addition.
 */
static void
init_add_rel_color_pt_funcs_mmx(void)
{
}
#endif
