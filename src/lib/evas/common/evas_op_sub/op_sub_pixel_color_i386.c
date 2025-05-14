/* sub pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for sub-pixel color span operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated routines that handle rendering or
 * processing of horizontal spans of pixels with sub-pixel precision and
 * a given color, writing to a destination buffer.
 */
static void
init_sub_pixel_color_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for sub-pixel color point operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated routines that handle rendering or
 * processing of individual pixels with sub-pixel precision and a given color,
 * writing to a destination buffer.
 */
static void
init_sub_pixel_color_pt_funcs_mmx(void)
{ }
#endif

/*-----*/

/* sub_rel pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for sub-pixel relative color span operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated routines that handle rendering or
 * processing of horizontal spans of pixels. These operations likely involve
 * sub-pixel precision, relative pixel addressing, and a given color,
 * writing to a destination buffer.
 */
static void
init_sub_rel_pixel_color_span_funcs_mmx(void)
{ }
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes MMX-optimized functions for sub-pixel relative color point operations.
 *
 * This function is intended to set up function pointers or perform other
 * initialization for MMX-accelerated routines that handle rendering or
 * processing of individual pixels. These operations likely involve
 * sub-pixel precision, relative pixel addressing, and a given color,
 * writing to a destination buffer.
 */
static void
init_sub_rel_pixel_color_pt_funcs_mmx(void)
{ }
#endif
