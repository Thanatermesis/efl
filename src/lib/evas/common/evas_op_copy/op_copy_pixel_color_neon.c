/* copy pixel x color --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a span of pixels, modulating each source pixel by a color. NEON optimized.
 * @param s Pointer to the source pixel data array (e.g., [p1, p2, ...]). Each pixel is a DATA32.
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color (DATA32, e.g., 0xAARRGGBB) to modulate source pixels with.
 * @param d Pointer to the destination pixel data array.
 * @param l The number of pixels to process.
 */
static void
_op_copy_p_c_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL4_SYM(c, *s);
                        d++;
                        s++;
                     });
}

/**
 * @brief Copies a span of pixels, modulating each source pixel by a factor derived from the alpha component of a color. NEON optimized.
 * 'caa' likely means Color Alpha Alpha, suggesting the color's alpha component is primary.
 * The operation effectively is `*d = *s * (1 + (c_alpha / 255)) / 256`.
 * @param s Pointer to the source pixel data array.
 * @param m Pointer to the mask data array (unused).
 * @param c The color (DATA32) whose alpha component is used for modulation.
 * @param d Pointer to the destination pixel data array.
 * @param l The number of pixels to process.
 */
static void
_op_copy_p_caa_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {   // FIXME: neon-it
   // FIXME: neon-it
   DATA32 *e;
   c = 1 + (c >> 24);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL_256(c, *s);
                        d++;
                        s++;
                     });
}

/**
 * @brief Aliases for NEON-optimized pixel copy span functions.
 * These macros map various source pixel (SP), source color (SC), and destination pixel (DP)
 * characteristic combinations to the appropriate NEON implementation.
 * - `pas`: Pixel Alpha Source (source has alpha)
 * - `pan`: Pixel Alpha None (source has no alpha or it's ignored)
 * - `can`: Color Alpha None (color has no alpha or it's ignored)
 * - `caa`: Color Alpha Alpha (color's alpha is used significantly, e.g., as a multiplier)
 * - `dpan`: Destination Pixel Alpha None (destination alpha is not written or preserved)
 *
 * For example, `_op_copy_pas_c_dp_neon` is an alias for copying pixels
 * where the source has alpha (`pas`), modulated by a color (`c`),
 * to a destination pixel (`dp`).
 */
#define _op_copy_pas_c_dp_neon _op_copy_p_c_dp_neon
#define _op_copy_pan_c_dp_neon _op_copy_p_c_dp_neon
#define _op_copy_p_can_dp_neon _op_copy_p_c_dp_neon
#define _op_copy_pas_can_dp_neon _op_copy_pas_c_dp_neon
#define _op_copy_pan_can_dp_neon _op_copy_pan_c_dp_neon
#define _op_copy_pas_caa_dp_neon _op_copy_p_caa_dp_neon
#define _op_copy_pan_caa_dp_neon _op_copy_p_caa_dp_neon

#define _op_copy_p_c_dpan_neon _op_copy_p_c_dp_neon
#define _op_copy_pas_c_dpan_neon _op_copy_pas_c_dp_neon
#define _op_copy_pan_c_dpan_neon _op_copy_pan_c_dp_neon
#define _op_copy_p_can_dpan_neon _op_copy_p_can_dp_neon
#define _op_copy_pas_can_dpan_neon _op_copy_pas_can_dp_neon
#define _op_copy_pan_can_dpan_neon _op_copy_pan_can_dp_neon
#define _op_copy_p_caa_dpan_neon _op_copy_p_caa_dp_neon
#define _op_copy_pas_caa_dpan_neon _op_copy_pas_caa_dp_neon
#define _op_copy_pan_caa_dpan_neon _op_copy_pan_caa_dp_neon

/**
 * @brief Initializes the function pointer table for NEON-optimized span copy operations.
 * This function populates the `op_copy_span_funcs` array with pointers to the
 * NEON implementations for various combinations of source pixel properties (SP),
 * source mask (SM_N - no mask), source color properties (SC),
 * destination pixel properties (DP), for the NEON CPU.
 */
static void
init_copy_pixel_color_span_funcs_neon(void)
{
   op_copy_span_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_copy_p_c_dp_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_copy_pas_c_dp_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_copy_pan_c_dp_neon;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_p_can_dp_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pas_can_dp_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pan_can_dp_neon;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_p_caa_dp_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pas_caa_dp_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pan_caa_dp_neon;

   op_copy_span_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_p_c_dpan_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pas_c_dpan_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pan_c_dpan_neon;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_p_can_dpan_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pas_can_dpan_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pan_can_dpan_neon;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_p_caa_dpan_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pas_caa_dpan_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pan_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single pixel, modulating the source pixel by a color. NEON optimized (point operation).
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused).
 * @param c The color (DATA32) to modulate the source pixel with.
 * @param d Pointer to the destination pixel data (DATA32).
 */
static void
_op_copy_pt_p_c_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   *d = MUL4_SYM(c, s);
}

/**
 * @brief Aliases for NEON-optimized single pixel (point) copy functions.
 * Similar to the span function aliases, these map various pixel and color
 * characteristics to the appropriate NEON point operation implementation.
 * The `_pt_` prefix indicates a point (single pixel) operation.
 */
#define _op_copy_pt_pas_c_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pan_c_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_p_can_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pas_can_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pan_can_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_p_caa_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pas_caa_dp_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pan_caa_dp_neon _op_copy_pt_p_c_dp_neon

#define _op_copy_pt_p_c_dpan_neon _op_copy_pt_p_c_dp_neon
#define _op_copy_pt_pas_c_dpan_neon _op_copy_pt_pas_c_dp_neon
#define _op_copy_pt_pan_c_dpan_neon _op_copy_pt_pan_c_dp_neon
#define _op_copy_pt_p_can_dpan_neon _op_copy_pt_p_can_dp_neon
#define _op_copy_pt_pas_can_dpan_neon _op_copy_pt_pas_can_dp_neon
#define _op_copy_pt_pan_can_dpan_neon _op_copy_pt_pan_can_dp_neon
#define _op_copy_pt_p_caa_dpan_neon _op_copy_pt_p_caa_dp_neon
#define _op_copy_pt_pas_caa_dpan_neon _op_copy_pt_pas_caa_dp_neon
#define _op_copy_pt_pan_caa_dpan_neon _op_copy_pt_pan_caa_dp_neon

/**
 * @brief Initializes the function pointer table for NEON-optimized point (single pixel) copy operations.
 * This function populates the `op_copy_pt_funcs` array with pointers to the
 * NEON implementations for various single-pixel operations.
 */
static void
init_copy_pixel_color_pt_funcs_neon(void)
{
   op_copy_pt_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_copy_pt_p_c_dp_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_copy_pt_pas_c_dp_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_copy_pt_pan_c_dp_neon;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pt_p_can_dp_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pt_pas_can_dp_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_pt_pan_can_dp_neon;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pt_p_caa_dp_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pt_pas_caa_dp_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_pt_pan_caa_dp_neon;

   op_copy_pt_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pt_p_c_dpan_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pt_pas_c_dpan_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_pt_pan_c_dpan_neon;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pt_p_can_dpan_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pt_pas_can_dpan_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_pt_pan_can_dpan_neon;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pt_p_caa_dpan_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pt_pas_caa_dpan_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_pt_pan_caa_dpan_neon;
}
#endif

/*-----*/

/* copy_rel pixel x color --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a span of pixels, modulating each source pixel by a color,
 *        and then modulating the result by the destination pixel's alpha. NEON optimized.
 *        The "rel" likely signifies an operation relative to the destination's properties.
 *        Operation: `*d = (*d_alpha / 255.0) * (*s * c)`
 * @param s Pointer to the source pixel data array.
 * @param m Pointer to the mask data array (unused).
 * @param c The color (DATA32) to modulate source pixels with.
 * @param d Pointer to the destination pixel data array (also read for alpha).
 * @param l The number of pixels to process.
 */
static void
_op_copy_rel_p_c_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 cs = MUL4_SYM(c, *s);
                        *d = MUL_SYM(*d >> 24, cs);
                        d++;
                        s++;
                     });
}

/**
 * @brief Aliases for NEON-optimized "copy_rel" span functions.
 * These macros map various source pixel, color, and destination characteristics
 * to the appropriate NEON "copy_rel" implementation. The "rel" suffix suggests
 * that the operation is relative to some property of the destination pixel,
 * often its alpha component.
 */
#define _op_copy_rel_pas_c_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_pan_c_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_p_can_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_pas_can_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_pan_can_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_p_caa_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_pas_caa_dp_neon _op_copy_rel_p_c_dp_neon
#define _op_copy_rel_pan_caa_dp_neon _op_copy_rel_p_c_dp_neon

#define _op_copy_rel_p_c_dpan_neon _op_copy_p_c_dpan_neon
#define _op_copy_rel_pas_c_dpan_neon _op_copy_pas_c_dpan_neon
#define _op_copy_rel_pan_c_dpan_neon _op_copy_pan_c_dpan_neon
#define _op_copy_rel_p_can_dpan_neon _op_copy_p_can_dpan_neon
#define _op_copy_rel_pas_can_dpan_neon _op_copy_pas_can_dpan_neon
#define _op_copy_rel_pan_can_dpan_neon _op_copy_pan_can_dpan_neon
#define _op_copy_rel_p_caa_dpan_neon _op_copy_p_caa_dpan_neon
#define _op_copy_rel_pas_caa_dpan_neon _op_copy_pas_caa_dpan_neon
#define _op_copy_rel_pan_caa_dpan_neon _op_copy_pan_caa_dpan_neon

/**
 * @brief Initializes the function pointer table for NEON-optimized "copy_rel" span operations.
 * This function populates the `op_copy_rel_span_funcs` array with pointers to the
 * NEON implementations for "copy_rel" operations, considering various pixel and color properties.
 */
static void
init_copy_rel_pixel_color_span_funcs_neon(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_p_c_dp_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pas_c_dp_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pan_c_dp_neon;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_p_can_dp_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pas_can_dp_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pan_can_dp_neon;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_p_caa_dp_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pas_caa_dp_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pan_caa_dp_neon;

   op_copy_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_p_c_dpan_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pas_c_dpan_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pan_c_dpan_neon;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_p_can_dpan_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pas_can_dpan_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pan_can_dpan_neon;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_p_caa_dpan_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pas_caa_dpan_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pan_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single pixel ("point" operation), modulating the source pixel by a color,
 *        and then modulating the result by the destination pixel's alpha. NEON optimized.
 *        Operation: `*d = (*d_alpha / 255.0) * (s * c)`
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused).
 * @param c The color (DATA32) to modulate the source pixel with.
 * @param d Pointer to the destination pixel data (DATA32), also read for alpha.
 */
static void
_op_copy_rel_pt_p_c_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   *d = MUL_SYM(*d >> 24, s);
}

/**
 * @brief Aliases for NEON-optimized single pixel (point) "copy_rel" functions.
 * These macros map various pixel and color characteristics to the appropriate
 * NEON "copy_rel" point operation implementation.
 */
#define _op_copy_rel_pt_pas_c_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_pan_c_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_p_can_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_pas_can_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_pan_can_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_p_caa_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_pas_caa_dp_neon _op_copy_rel_pt_p_c_dp_neon
#define _op_copy_rel_pt_pan_caa_dp_neon _op_copy_rel_pt_p_c_dp_neon

#define _op_copy_rel_pt_p_c_dpan_neon _op_copy_pt_p_c_dpan_neon
#define _op_copy_rel_pt_pas_c_dpan_neon _op_copy_pt_pas_c_dpan_neon
#define _op_copy_rel_pt_pan_c_dpan_neon _op_copy_pt_pan_c_dpan_neon
#define _op_copy_rel_pt_p_can_dpan_neon _op_copy_pt_p_can_dpan_neon
#define _op_copy_rel_pt_pas_can_dpan_neon _op_copy_pt_pas_can_dpan_neon
#define _op_copy_rel_pt_pan_can_dpan_neon _op_copy_pt_pan_can_dpan_neon
#define _op_copy_rel_pt_p_caa_dpan_neon _op_copy_pt_p_caa_dpan_neon
#define _op_copy_rel_pt_pas_caa_dpan_neon _op_copy_pt_pas_caa_dpan_neon
#define _op_copy_rel_pt_pan_caa_dpan_neon _op_copy_pt_pan_caa_dpan_neon

/**
 * @brief Initializes the function pointer table for NEON-optimized "copy_rel" point operations.
 * This function populates the `op_copy_rel_pt_funcs` array with pointers to the
 * NEON implementations for single-pixel "copy_rel" operations.
 */
static void
init_copy_rel_pixel_color_pt_funcs_neon(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pt_p_c_dp_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pt_pas_c_dp_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_copy_rel_pt_pan_c_dp_neon;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pt_p_can_dp_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pt_pas_can_dp_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_copy_rel_pt_pan_can_dp_neon;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pt_p_caa_dp_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pt_pas_caa_dp_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_copy_rel_pt_pan_caa_dp_neon;

   op_copy_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pt_p_c_dpan_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pt_pas_c_dpan_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_copy_rel_pt_pan_c_dpan_neon;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pt_p_can_dpan_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pt_pas_can_dpan_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pt_pan_can_dpan_neon;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pt_p_caa_dpan_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pt_pas_caa_dpan_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pt_pan_caa_dpan_neon;
}
#endif
