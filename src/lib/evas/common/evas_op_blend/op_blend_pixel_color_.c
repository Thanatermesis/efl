/* blend pixel x color --> dst */

/**
 * @brief Blend source pixels with a color, then blend with destination pixels.
 * @param s Pointer to the source pixel data array (ARGB).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color to blend with the source pixels (ARGB).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * For each pixel, computes:
 *   sc = MUL4_SYM(c, *s)  (source pixel modulated by color c)
 *   alpha = 256 - (sc >> 24) (alpha of the modulated source)
 *   *d = sc + MUL_256(alpha, *d) (blends sc over d using sc's alpha)
 */
static void
_op_blend_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL4_SYM(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = sc + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/**
 * @brief Blend source pixels (no alpha) with a color, then blend with destination pixels.
 * @param s Pointer to the source pixel data array (RGB, alpha ignored).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color to blend with the source pixels (ARGB).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * Assumes source pixels (*s) have no alpha component (or it's ignored).
 * The alpha for blending with the destination comes from the color c.
 * For each pixel, computes:
 *   alpha_c = 256 - (c >> 24) (alpha of color c)
 *   *d = ((c & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha_c, *d)
 *   (blends (c_alpha | c_rgb * s_rgb) over d using c's alpha)
 */
static void
_op_blend_pan_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha = 256 - (c >> 24);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = ((c & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/**
 * @brief Blend source pixels with a color (no alpha), then blend with destination pixels.
 * @param s Pointer to the source pixel data array (ARGB).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color to blend with the source pixels (RGB, alpha ignored).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * Assumes the color c has no alpha component (or it's ignored).
 * The alpha for blending with the destination comes from the source pixels *s.
 * For each pixel, computes:
 *   alpha_s = 256 - (*s >> 24) (alpha of source pixel *s)
 *   *d = ((*s & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha_s, *d)
 *   (blends (s_alpha | c_rgb * s_rgb) over d using s's alpha)
 */
static void
_op_blend_p_can_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = 256 - (*s >> 24);
                        *d = ((*s & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/**
 * @brief Blend source pixels (no alpha) with a color (no alpha), then overwrite destination pixels.
 * @param s Pointer to the source pixel data array (RGB, alpha ignored).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color to blend with the source pixels (RGB, alpha ignored).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * Assumes source pixels (*s) and color c have no alpha (or it's ignored).
 * The result is fully opaque.
 * For each pixel, computes:
 *   *d = 0xff000000 + MUL3_SYM(c, *s) (c_rgb * s_rgb with full alpha)
 */
static void
_op_blend_pan_can_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d++ = 0xff000000 + MUL3_SYM(c, *s);
                        s++;
                     });
}

/**
 * @brief Blend source pixels with the alpha channel of a color, then blend with destination.
 * @param s Pointer to the source pixel data array (ARGB).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color whose alpha channel is used for blending (alpha in [0-255]).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * Uses only the alpha component of 'c', scaled to [1-256].
 * For each pixel, computes:
 *   c_alpha_scaled = 1 + (c & 0xff)
 *   sc = MUL_256(c_alpha_scaled, *s) (source pixel modulated by c's scaled alpha)
 *   alpha_sc = 256 - (sc >> 24) (alpha of the modulated source)
 *   *d = sc + MUL_256(alpha_sc, *d) (blends sc over d using sc's alpha)
 */
static void
_op_blend_p_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   c = 1 + (c & 0xff);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL_256(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = sc + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/**
 * @brief Interpolate source pixels (no alpha) with destination pixels using alpha of a color.
 * @param s Pointer to the source pixel data array (RGB, alpha ignored).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color whose alpha channel is used for interpolation (alpha in [0-255]).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * Uses only the alpha component of 'c', scaled to [1-256].
 * For each pixel, computes:
 *   c_alpha_scaled = 1 + (c & 0xff)
 *   *d = INTERP_256(c_alpha_scaled, *s, *d)
 *      (i.e. c_alpha_scaled * *s + (256 - c_alpha_scaled) * *d)
 */
static void
_op_blend_pan_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   c = 1 + (c & 0xff);
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = INTERP_256(c, *s, *d);
                        d++;
                        s++;
                     });
}

/** @name Blend function aliases
 *  These macros define aliases for blend functions.
 *  - `_pas_` (Pixel Alpha Source) variants are typically the same as `_p_` (Pixel)
 *    if source alpha is inherently used or correctly handled by the base function.
 *  - `_dpan_` (Destination Pixel Alpha No) variants imply that the destination alpha
 *    is not considered or is overwritten. If the base function already does this
 *    or if its alpha blending naturally handles opaque destinations correctly,
 *    it can be aliased.
 */
///@{
#define _op_blend_pas_c_dp _op_blend_p_c_dp
#define _op_blend_pas_can_dp _op_blend_p_can_dp
#define _op_blend_pas_caa_dp _op_blend_p_caa_dp

#define _op_blend_p_c_dpan _op_blend_p_c_dp
#define _op_blend_pas_c_dpan _op_blend_pas_c_dp
#define _op_blend_pan_c_dpan _op_blend_pan_c_dp
#define _op_blend_p_can_dpan _op_blend_p_can_dp
#define _op_blend_pas_can_dpan _op_blend_pas_can_dp
#define _op_blend_pan_can_dpan _op_blend_pan_can_dp
#define _op_blend_p_caa_dpan _op_blend_p_caa_dp
#define _op_blend_pas_caa_dpan _op_blend_pas_caa_dp
#define _op_blend_pan_caa_dpan _op_blend_pan_caa_dp
///@}

/**
 * @brief Initializes the span-based pixel-color blend function pointers for C CPU.
 * This function populates the `op_blend_span_funcs` array with the appropriate
 * C implementations for various blending operations involving a source image,
 * a color, and a destination image.
 */
static void
init_blend_pixel_color_span_funcs_c(void)
{
   op_blend_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_p_c_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_pas_c_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_pan_c_dp;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_p_can_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_pas_can_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_pan_can_dp;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_p_caa_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_pas_caa_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_pan_caa_dp;

   op_blend_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_p_c_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_pas_c_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_pan_c_dpan;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_p_can_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pas_can_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pan_can_dpan;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_p_caa_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pas_caa_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pan_caa_dpan;
}

/**
 * @brief Blend a single source pixel with a color, then blend with a destination pixel.
 * @param s The source pixel (ARGB).
 * @param m The mask value (alpha, unused).
 * @param c The color to blend with the source pixel (ARGB).
 * @param d Pointer to the destination pixel (ARGB).
 *
 * This is a point (single pixel) version of _op_blend_p_c_dp.
 * Computes:
 *   s_modulated = MUL4_SYM(c, s)
 *   alpha_sm = 256 - (s_modulated >> 24)
 *   *d = s_modulated + MUL_256(alpha_sm, *d)
 */
static void
_op_blend_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   c = 256 - (s >> 24); // Note: c is reused here to store alpha
   *d = s + MUL_256(c, *d);
}

/** @name Point blend function aliases
 *  These macros define aliases for point (single-pixel) blend functions.
 *  The logic for aliasing (e.g., `_pas_`, `_pan_`, `_can_`, `_caa_`, `_dpan_`)
 *  is often simplified for point operations as the base `_op_blend_pt_p_c_dp`
 *  might be general enough or the specific variations are not needed/optimized
 *  differently at the point level. Here, many variations alias to the base
 *  `_op_blend_pt_p_c_dp`, implying its logic covers these cases or that
 *  specific optimized versions for these cases are not provided.
 */
///@{
#define _op_blend_pt_pas_c_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_c_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_p_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_can_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_p_caa_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_caa_dp _op_blend_pt_p_c_dp
#define _op_blend_pt_pan_caa_dp _op_blend_pt_p_c_dp

#define _op_blend_pt_p_c_dpan _op_blend_pt_p_c_dp
#define _op_blend_pt_pas_c_dpan _op_blend_pt_pas_c_dp
#define _op_blend_pt_pan_c_dpan _op_blend_pt_pan_c_dp
#define _op_blend_pt_p_can_dpan _op_blend_pt_p_can_dp
#define _op_blend_pt_pas_can_dpan _op_blend_pt_pas_can_dp
#define _op_blend_pt_pan_can_dpan _op_blend_pt_pan_can_dp
#define _op_blend_pt_p_caa_dpan _op_blend_pt_p_caa_dp
#define _op_blend_pt_pas_caa_dpan _op_blend_pt_pas_caa_dp
#define _op_blend_pt_pan_caa_dpan _op_blend_pt_pan_caa_dp
///@}

/**
 * @brief Initializes the point-based pixel-color blend function pointers for C CPU.
 * This function populates the `op_blend_pt_funcs` array with the appropriate
 * C implementations for various single-pixel blending operations.
 */
static void
init_blend_pixel_color_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_pt_p_c_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_pt_pas_c_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_pt_pan_c_dp;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_p_can_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_pas_can_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_pt_pan_can_dp;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_p_caa_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_pas_caa_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_pan_caa_dp;

   op_blend_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_p_c_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_pas_c_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_pan_c_dpan;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_p_can_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_pas_can_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_pt_pan_can_dpan;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_p_caa_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_pas_caa_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_pan_caa_dpan;
}

/*-----*/

/* blend_rel pixel x color -> dst */

/**
 * @brief Relative blend of source pixels with a color, then blend with destination pixels.
 * @param s Pointer to the source pixel data array (ARGB).
 * @param m Pointer to the mask data array (alpha, unused).
 * @param c The color to blend with the source pixels (ARGB).
 * @param d Pointer to the destination pixel data array (ARGB).
 * @param l The number of pixels to process.
 *
 * "Relative" blend implies that the source contribution is modulated by
 * the destination's alpha.
 * For each pixel, computes:
 *   sc = MUL4_SYM(c, *s) (source pixel modulated by color c)
 *   alpha_sc = 256 - (sc >> 24) (alpha of the modulated source)
 *   *d = MUL_SYM(*d >> 24, sc) + MUL_256(alpha_sc, *d)
 *      (blends (d_alpha * sc) over d using sc's alpha for the second term)
 */
static void
_op_blend_rel_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 sc = MUL4_SYM(c, *s);
                        alpha = 256 - (sc >> 24);
                        *d = MUL_SYM(*d >> 24, sc) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/** @name Relative blend function aliases
 *  These macros define aliases for relative blend functions.
 *  For relative blending, many specific cases (`_pan_`, `_can_`, etc.)
 *  are aliased to the base `_op_blend_rel_p_c_dp`. This suggests that
 *  the base function is general enough to handle these scenarios, or
 *  specific optimized versions are not provided for these combinations
 *  in the relative blending mode.
 */
///@{
#define _op_blend_rel_pas_c_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_c_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_p_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pas_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_can_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_p_caa_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pas_caa_dp _op_blend_rel_p_c_dp
#define _op_blend_rel_pan_caa_dp _op_blend_rel_p_c_dp

#define _op_blend_rel_p_c_dpan _op_blend_p_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_p_c_dp */
#define _op_blend_rel_pas_c_dpan _op_blend_pas_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pas_c_dp */
#define _op_blend_rel_pan_c_dpan _op_blend_pan_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pan_c_dp */
#define _op_blend_rel_p_can_dpan _op_blend_p_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_p_can_dp */
#define _op_blend_rel_pas_can_dpan _op_blend_pas_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pas_can_dp */
#define _op_blend_rel_pan_can_dpan _op_blend_pan_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pan_can_dp */
#define _op_blend_rel_p_caa_dpan _op_blend_p_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_p_caa_dp */
#define _op_blend_rel_pas_caa_dpan _op_blend_pas_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pas_caa_dp */
#define _op_blend_rel_pan_caa_dpan _op_blend_pan_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pan_caa_dp */
///@}

/**
 * @brief Initializes the span-based relative pixel-color blend function pointers for C CPU.
 * This function populates the `op_blend_rel_span_funcs` array with the appropriate
 * C implementations for various relative blending operations.
 */
static void
init_blend_rel_pixel_color_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_rel_p_c_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_rel_pas_c_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_rel_pan_c_dp;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_p_can_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pas_can_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pan_can_dp;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_p_caa_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pas_caa_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pan_caa_dp;

   op_blend_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_p_c_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pas_c_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pan_c_dpan;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_p_can_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pas_can_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pan_can_dpan;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_p_caa_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pas_caa_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pan_caa_dpan;
}

/**
 * @brief Relative blend of a single source pixel with a color, then blend with a destination pixel.
 * @param s The source pixel (ARGB).
 * @param m The mask value (alpha, unused).
 * @param c The color to blend with the source pixel (ARGB).
 * @param d Pointer to the destination pixel (ARGB).
 *
 * This is a point (single pixel) version of _op_blend_rel_p_c_dp.
 * Computes:
 *   s_modulated = MUL4_SYM(c, s)
 *   alpha_sm = 256 - (s_modulated >> 24) // Stored in c after this line
 *   *d = MUL_SYM(*d >> 24, s_modulated) + MUL_256(alpha_sm, *d)
 */
static void
_op_blend_rel_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   c = 256 - (s >> 24); // Note: c is reused here to store alpha
   *d = MUL_SYM(*d >> 24, s) + MUL_256(c, *d);
}

/** @name Relative point blend function aliases
 *  These macros define aliases for relative point (single-pixel) blend functions.
 *  Similar to non-relative point blends and relative span blends, many variations
 *  alias to the base `_op_blend_rel_pt_p_c_dp`.
 */
///@{
#define _op_blend_rel_pt_pas_c_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_c_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_p_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pas_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_can_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_p_caa_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pas_caa_dp _op_blend_rel_pt_p_c_dp
#define _op_blend_rel_pt_pan_caa_dp _op_blend_rel_pt_p_c_dp

#define _op_blend_rel_pt_p_c_dpan _op_blend_pt_p_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_p_c_dp */
#define _op_blend_rel_pt_pas_c_dpan _op_blend_pt_pas_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pas_c_dp */
#define _op_blend_rel_pt_pan_c_dpan _op_blend_pt_pan_c_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pan_c_dp */
#define _op_blend_rel_pt_p_can_dpan _op_blend_pt_p_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_p_can_dp */
#define _op_blend_rel_pt_pas_can_dpan _op_blend_pt_pas_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pas_can_dp */
#define _op_blend_rel_pt_pan_can_dpan _op_blend_pt_pan_can_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pan_can_dp */
#define _op_blend_rel_pt_p_caa_dpan _op_blend_pt_p_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_p_caa_dp */
#define _op_blend_rel_pt_pas_caa_dpan _op_blend_pt_pas_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pas_caa_dp */
#define _op_blend_rel_pt_pan_caa_dpan _op_blend_pt_pan_caa_dpan /* FIXME: This seems to be a typo, should be _op_blend_rel_pt_pan_caa_dp */
///@}

/**
 * @brief Initializes the point-based relative pixel-color blend function pointers for C CPU.
 * This function populates the `op_blend_rel_pt_funcs` array with the appropriate
 * C implementations for various single-pixel relative blending operations.
 */
static void
init_blend_rel_pixel_color_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_p_c_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_pas_c_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_pan_c_dp;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_p_can_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_pas_can_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_blend_rel_pt_pan_can_dp;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_p_caa_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_pas_caa_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_pan_caa_dp;

   op_blend_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_p_c_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_pas_c_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_pan_c_dpan;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_p_can_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_pas_can_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_pan_can_dpan;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_p_caa_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_pas_caa_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_pan_caa_dpan;
}
