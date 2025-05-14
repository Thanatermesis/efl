/**
 * @file op_copy_color_neon.c
 * @brief NEON-optimized color copy operations.
 *
 * This file implements functions for copying a solid color to a destination buffer,
 * potentially with alpha blending, using NEON intrinsics or assembly for performance.
 * It handles different scenarios like span operations (copying to multiple pixels)
 * and point operations (copying to a single pixel).
 */

/* copy color --> dst */

#ifdef BUILD_NEON

#ifndef BUILD_NEON_INTRINSICS
/**
 * @brief NEON assembly function for composing a source color onto a destination.
 *
 * This external function is used when NEON intrinsics are not enabled.
 * It performs a source (solid color) over destination operation.
 *
 * @param w Width of the area to fill.
 * @param h Height of the area to fill (typically 1 for span operations).
 * @param dst Pointer to the destination buffer (array of DATA32 pixels).
 * @param dst_stride Stride of the destination buffer in pixels (not bytes).
 * @param src The source color to fill (format: 0xAARRGGBB).
 */
extern void
pixman_composite_src_n_8888_asm_neon (int32_t   w,
                                      int32_t   h,
                                      uint32_t *dst,
                                      int32_t   dst_stride,
                                      uint32_t  src);
#endif

/**
 * @brief Copies a solid color to a span of destination pixels using NEON.
 *
 * This function fills a line of `l` pixels in the destination buffer `d`
 * with the color `c`. It uses NEON intrinsics if available, otherwise
 * it falls back to a NEON assembly implementation.
 * The source buffer `s` and mask `m` are unused in this copy operation.
 *
 * @param s Unused pointer to source pixel data.
 * @param m Unused pointer to mask data.
 * @param c The solid color to copy (format: 0xAARRGGBB or 0xRRGGBB if alpha is not used by dest).
 * @param d Pointer to the destination buffer (array of DATA32 pixels).
 *          Example: `d` could point to `[pix1, pix2, ..., pixL]`.
 * @param l The number of pixels to fill (length of the span).
 */
static void
_op_copy_c_dp_neon(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
#ifdef BUILD_NEON_INTRINSICS
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = c;
                        d++;
                     });
#else
   pixman_composite_src_n_8888_asm_neon(l,1,d,l,c);
#endif
}

#define _op_copy_cn_dp_neon _op_copy_c_dp_neon
#define _op_copy_can_dp_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color with alpha, no alpha in destination. */
#define _op_copy_caa_dp_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color with alpha, alpha in destination. */

#define _op_copy_cn_dpan_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color no alpha, destination has pre-multiplied alpha. */
#define _op_copy_c_dpan_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color, destination has pre-multiplied alpha. */
#define _op_copy_can_dpan_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color with alpha, destination has pre-multiplied alpha. */
#define _op_copy_caa_dpan_neon _op_copy_c_dp_neon /**< Alias for _op_copy_c_dp_neon, color with alpha and alpha, destination has pre-multiplied alpha. */

/**
 * @brief Initializes the NEON-optimized span copy color function pointers.
 *
 * This function assigns the appropriate NEON-accelerated span copy functions
 * to the global function pointer table `op_copy_span_funcs`. These functions
 * are used when copying a solid color to a span of pixels, under various
 * source color and destination pixel format combinations.
 *
 * `op_copy_span_funcs` is a multi-dimensional array where dimensions might represent:
 * - SP (Source Pre-multiplied): e.g., SP_N (Not pre-multiplied)
 * - SM (Source Mask): e.g., SM_N (No source mask)
 * - SC (Source Color type): e.g., SC_N (No Alpha), SC (Alpha), SC_AN (Alpha, No Alpha channel in color), SC_AA (Alpha, Alpha channel in color)
 * - DP (Destination Pre-multiplied/Format): e.g., DP (Direct Pixel), DP_AN (Direct Pixel, Alpha No pre-mult)
 * - CPU specific implementation: e.g., CPU_NEON
 *
 * Example `op_copy_span_funcs[SP_N][SM_N][SC_N][DP][CPU_NEON]` would be a function
 * that copies a non-premultiplied source color, with no source mask, where the source color
 * has no alpha (e.g. RGB), to a destination buffer with direct pixel access, using NEON.
 */
static void
init_copy_color_span_funcs_neon(void)
{
   op_copy_span_funcs[SP_N][SM_N][SC_N][DP][CPU_NEON] = _op_copy_cn_dp_neon;
   op_copy_span_funcs[SP_N][SM_N][SC][DP][CPU_NEON] = _op_copy_c_dp_neon;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_can_dp_neon;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_caa_dp_neon;

   op_copy_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_cn_dpan_neon;
   op_copy_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_c_dpan_neon;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_can_dpan_neon;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a solid color to a single destination pixel using NEON (conceptually).
 *
 * This function sets a single pixel at destination `d` to the color `c`.
 * While NEON is typically for SIMD operations over multiple data elements,
 * this function represents the point operation equivalent. For a single pixel,
 * direct assignment is often as efficient.
 * The source pixel `s` and mask `m` are unused.
 *
 * @param s Unused source pixel value.
 * @param m Unused mask value.
 * @param c The solid color to copy (format: 0xAARRGGBB or 0xRRGGBB).
 * @param d Pointer to the single destination pixel (DATA32).
 *          Example: `d` points to `the_pixel_to_change`.
 */
static void
_op_copy_pt_c_dp_neon(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   *d = c;
}

#define _op_copy_pt_cn_dp_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color no alpha. */
#define _op_copy_pt_can_dp_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color with alpha, no alpha in dest. */
#define _op_copy_pt_caa_dp_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color with alpha, alpha in dest. */

#define _op_copy_pt_cn_dpan_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color no alpha, dest pre-multiplied alpha. */
#define _op_copy_pt_c_dpan_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color, dest pre-multiplied alpha. */
#define _op_copy_pt_can_dpan_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color with alpha, dest pre-multiplied alpha. */
#define _op_copy_pt_caa_dpan_neon _op_copy_pt_c_dp_neon /**< Alias for point copy, color with alpha and alpha, dest pre-multiplied alpha. */

/**
 * @brief Initializes the NEON-optimized point copy color function pointers.
 *
 * This function assigns the appropriate NEON-accelerated (or equivalent) point
 * copy functions to the global function pointer table `op_copy_pt_funcs`.
 * These functions are used when copying a solid color to a single pixel,
 * under various source color and destination pixel format combinations.
 *
 * `op_copy_pt_funcs` structure is similar to `op_copy_span_funcs`.
 * See @ref init_copy_color_span_funcs_neon for details on the array indices.
 */
static void
init_copy_color_pt_funcs_neon(void)
{
   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pt_cn_dp_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP][CPU_NEON] = _op_copy_pt_c_dp_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pt_can_dp_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pt_caa_dp_neon;

   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_cn_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pt_c_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pt_can_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pt_caa_dpan_neon;
}
#endif

/*-----*/

/* copy_rel color --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a color to a span of destination pixels, modulating by destination alpha (relative copy).
 *
 * This function fills a line of `l` pixels in the destination buffer `d`.
 * The copied color `c` is first multiplied by the alpha component of each
 * destination pixel (`*d >> 24`). This is a "relative" copy because the final
 * color depends on the existing destination pixel's alpha.
 * The source buffer `s` and mask `m` are unused.
 *
 * @note FIXME: This function is marked for NEON optimization but currently uses a C loop.
 *
 * @param s Unused pointer to source pixel data.
 * @param m Unused pointer to mask data.
 * @param c The solid color to copy (format: 0xAARRGGBB or 0xRRGGBB).
 * @param d Pointer to the destination buffer (array of DATA32 pixels).
 *          Example: `d` could point to `[pix1, pix2, ..., pixL]`.
 * @param l The number of pixels to fill (length of the span).
 */
static void
_op_copy_rel_c_dp_neon(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e = d + l;
   for (; d < e; d++) {
	*d = MUL_SYM(*d >> 24, c);
   }
}

#define _op_copy_rel_cn_dp_neon _op_copy_rel_c_dp_neon
#define _op_copy_rel_can_dp_neon _op_copy_rel_c_dp_neon /**< Alias for relative color copy, color with alpha. */
#define _op_copy_rel_caa_dp_neon _op_copy_rel_c_dp_neon /**< Alias for relative color copy, color with alpha and alpha. */

#define _op_copy_rel_cn_dpan_neon _op_copy_cn_dpan_neon /**< Alias for relative color copy, color no alpha, dest pre-multiplied alpha. Refers to _op_copy_cn_dpan_neon. */
#define _op_copy_rel_c_dpan_neon _op_copy_c_dpan_neon   /**< Alias for relative color copy, color, dest pre-multiplied alpha. Refers to _op_copy_c_dpan_neon. */
#define _op_copy_rel_can_dpan_neon _op_copy_can_dpan_neon /**< Alias for relative color copy, color with alpha, dest pre-multiplied alpha. Refers to _op_copy_can_dpan_neon. */
#define _op_copy_rel_caa_dpan_neon _op_copy_caa_dpan_neon /**< Alias for relative color copy, color with alpha and alpha, dest pre-multiplied alpha. Refers to _op_copy_caa_dpan_neon. */

/**
 * @brief Initializes the NEON-optimized relative span copy color function pointers.
 *
 * This function assigns the appropriate NEON-accelerated (or C fallback)
 * relative span copy functions to the global function pointer table
 * `op_copy_rel_span_funcs`. These functions are used when copying a solid
 * color to a span of pixels, modulated by the destination alpha.
 *
 * `op_copy_rel_span_funcs` structure is similar to `op_copy_span_funcs`.
 * See @ref init_copy_color_span_funcs_neon for details on the array indices.
 */
static void
init_copy_rel_color_span_funcs_neon(void)
{
   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_cn_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_c_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_can_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_caa_dp_neon;

   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_cn_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_c_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_can_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a color to a single destination pixel, modulating by destination alpha (relative copy).
 *
 * This function sets a single pixel at destination `d`. The color `c` is
 * multiplied by `1 + (destination_alpha / 255)`, effectively scaling `c`
 * by the destination's opacity. The source pixel value `s` is overwritten
 * to store this scale factor before the multiplication.
 * The mask `m` is unused.
 *
 * @param s Source pixel value (used as a temporary for scale factor, its initial value is ignored).
 * @param m Unused mask value.
 * @param c The solid color to copy (format: 0xAARRGGBB or 0xRRGGBB).
 * @param d Pointer to the single destination pixel (DATA32).
 *          Example: `d` points to `the_pixel_to_change`.
 */
static void
_op_copy_rel_pt_c_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = 1 + (*d >> 24); // Calculate scale factor from destination alpha
   *d = MUL_256(s, c); // Modulate color c by the scale factor
}


#define _op_copy_rel_pt_cn_dp_neon _op_copy_rel_pt_c_dp_neon /**< Alias for relative point copy, color no alpha. */
#define _op_copy_rel_pt_can_dp_neon _op_copy_rel_pt_c_dp_neon /**< Alias for relative point copy, color with alpha. */
#define _op_copy_rel_pt_caa_dp_neon _op_copy_rel_pt_c_dp_neon /**< Alias for relative point copy, color with alpha and alpha. */

#define _op_copy_rel_pt_cn_dpan_neon _op_copy_pt_cn_dpan_neon /**< Alias for relative point copy, color no alpha, dest pre-multiplied alpha. Refers to _op_copy_pt_cn_dpan_neon. */
#define _op_copy_rel_pt_c_dpan_neon _op_copy_pt_c_dpan_neon   /**< Alias for relative point copy, color, dest pre-multiplied alpha. Refers to _op_copy_pt_c_dpan_neon. */
#define _op_copy_rel_pt_can_dpan_neon _op_copy_pt_can_dpan_neon /**< Alias for relative point copy, color with alpha, dest pre-multiplied alpha. Refers to _op_copy_pt_can_dpan_neon. */
#define _op_copy_rel_pt_caa_dpan_neon _op_copy_pt_caa_dpan_neon /**< Alias for relative point copy, color with alpha and alpha, dest pre-multiplied alpha. Refers to _op_copy_pt_caa_dpan_neon. */

/**
 * @brief Initializes the NEON-optimized relative point copy color function pointers.
 *
 * This function assigns the appropriate NEON-accelerated (or C fallback)
 * relative point copy functions to the global function pointer table
 * `op_copy_rel_pt_funcs`. These functions are used when copying a solid
 * color to a single pixel, modulated by the destination alpha.
 *
 * `op_copy_rel_pt_funcs` structure is similar to `op_copy_span_funcs`.
 * See @ref init_copy_color_span_funcs_neon for details on the array indices.
 */
static void
init_copy_rel_color_pt_funcs_neon(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_cn_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pt_c_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pt_can_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pt_caa_dp_neon;

   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_cn_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pt_c_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pt_can_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pt_caa_dpan_neon;
}
#endif
