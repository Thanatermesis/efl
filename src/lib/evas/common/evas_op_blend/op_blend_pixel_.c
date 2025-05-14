/* blend pixel --> dst */

/**
 * @brief Blends a source pixel onto a destination pixel.
 *
 * This function performs a standard alpha blend (Porter-Duff "over" operator).
 * The source pixel's alpha determines its opacity.
 * dst = src + (1 - src_alpha) * dst
 *
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel arrays (number of pixels to process).
 */
static void
_op_blend_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = 256 - (*s >> 24);
                        *d = *s++ + MUL_256(alpha, *d);
                        d++;
                     });
}

/**
 * @brief Blends a source pixel onto a destination pixel, skipping if source is fully transparent.
 *
 * This function performs an alpha blend similar to _op_blend_p_dp, but with optimizations:
 * - If the source pixel is fully transparent (alpha = 0), the destination pixel is unchanged.
 * - If the source pixel is fully opaque (alpha = 255), the destination pixel is replaced by the source.
 * Otherwise, a standard alpha blend is performed.
 *
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel arrays (number of pixels to process).
 */
static void
_op_blend_pas_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        switch (*s & 0xff000000)
                          {
                          case 0:
                             break;
                          case 0xff000000:
                             *d = *s;
                             break;
                          default:
                             alpha = 256 - (*s >> 24);
                             *d = *s + MUL_256(alpha, *d);
                             break;
                          }
                        s++;  d++;
                     });
}

#define _op_blend_pan_dp NULL

#define _op_blend_p_dpan _op_blend_p_dp
#define _op_blend_pas_dpan _op_blend_pas_dp
#define _op_blend_pan_dpan _op_blend_pan_dp

/**
 * @brief Initializes the span blending functions for pixel operations.
 *
 * This function assigns the appropriate C-specific blending functions
 * to the global function pointer array `op_blend_span_funcs`.
 * These functions handle blending entire spans (lines) of pixels.
 * - SP: Source Pixel
 * - SM_N: No Source Mask
 * - SC_N: No Source Color
 * - DP: Destination Pixel
 * - DP_AN: Destination Pixel with Alpha (No pre-multiplication)
 * - CPU_C: C implementation
 */
static void
init_blend_pixel_span_funcs_c(void)
{
   op_blend_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_blend_p_dp;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_blend_pas_dp;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_blend_pan_dp;

   op_blend_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_p_dpan;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_pas_dpan;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_pan_dpan;
}

/**
 * @brief Blends a single source pixel onto a single destination pixel.
 *
 * This is a point (single pixel) version of _op_blend_p_dp.
 * dst = src + (1 - src_alpha) * dst
 *
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Color value (DATA32), used to calculate inverse alpha.
 * @param d Pointer to the destination pixel value (DATA32).
 */
static void
_op_blend_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   // c is re-purposed here to store (256 - source_alpha)
   c = 256 - (s >> 24);
   *d = s + MUL_256(c, *d);
}

#define _op_blend_pt_pas_dp _op_blend_pt_p_dp
#define _op_blend_pt_pan_dp NULL

#define _op_blend_pt_p_dpan _op_blend_pt_p_dp
#define _op_blend_pt_pan_dpan _op_blend_pt_pan_dp
#define _op_blend_pt_pas_dpan _op_blend_pt_pas_dp

/**
 * @brief Initializes the point blending functions for pixel operations.
 *
 * This function assigns the appropriate C-specific blending functions
 * to the global function pointer array `op_blend_pt_funcs`.
 * These functions handle blending single pixels.
 * Array indices have similar meanings as in init_blend_pixel_span_funcs_c.
 */
static void
init_blend_pixel_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_blend_pt_p_dp;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_blend_pt_pas_dp;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_blend_pt_pan_dp;

   op_blend_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_pt_p_dpan;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_pt_pas_dpan;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_pt_pan_dpan;
}

/*-----*/

/* blend_rel pixel -> dst */

/**
 * @brief Blends a source pixel onto a destination pixel using relative alpha.
 *
 * This function performs a blend where the source pixel's contribution
 * is scaled by the destination pixel's alpha. This is often used for effects
 * where the source "reveals" or "colors" the destination based on the destination's existing opacity.
 * dst = (dst_alpha / 255) * src + (1 - src_alpha) * dst
 *
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32), used to store (1 + destination_alpha).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel arrays (number of pixels to process).
 */
static void
_op_blend_rel_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = 256 - (*s >> 24);
                        c = 1 + (*d >> 24);
                        *d = MUL_256(c, *s) + MUL_256(alpha, *d);
                        d++;
                        s++;
                     });
}

/**
 * @brief Blends a source pixel onto a destination pixel using relative alpha, source alpha is ignored (assumed opaque).
 *
 * This function is similar to _op_blend_rel_p_dp but assumes the source pixel is fully opaque,
 * or its alpha is not part of the blending calculation for the second term.
 * The destination pixel is effectively scaled by the source pixel, modulated by the destination's alpha.
 * dst = (dst_alpha / 255) * src
 *
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32), used to store (1 + destination_alpha).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel arrays (number of pixels to process).
 */
static void
_op_blend_rel_pan_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        c = 1 + (*d >> 24);
                        *d++ = MUL_256(c, *s);
                        s++;
                     });
}

#define _op_blend_rel_pas_dp _op_blend_rel_p_dp

#define _op_blend_rel_p_dpan _op_blend_p_dpan
#define _op_blend_rel_pan_dpan _op_blend_pan_dpan
#define _op_blend_rel_pas_dpan _op_blend_pas_dpan

/**
 * @brief Initializes the span blending functions for relative pixel operations.
 *
 * This function assigns the appropriate C-specific blending functions
 * to the global function pointer array `op_blend_rel_span_funcs`.
 * These functions handle blending entire spans (lines) of pixels with relative alpha.
 * Array indices have similar meanings as in init_blend_pixel_span_funcs_c.
 */
static void
init_blend_rel_pixel_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_p_dp;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_pas_dp;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_pan_dp;

   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_p_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_pas_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_pan_dpan;
}

/**
 * @brief Blends a single source pixel onto a single destination pixel using relative alpha.
 *
 * This is a point (single pixel) version of _op_blend_rel_p_dp.
 * dst = (dst_alpha / 255) * src + (1 - src_alpha) * dst
 * Note: MUL_SYM(da, s) is equivalent to (da * s) / 255.
 *
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Color value (DATA32), used to store (256 - source_alpha).
 * @param d Pointer to the destination pixel value (DATA32).
 */
static void
_op_blend_rel_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   // c is re-purposed here to store (256 - source_alpha)
   c = 256 - (s >> 24);
   *d = MUL_SYM(*d >> 24, s) + MUL_256(c, *d);
}

/**
 * @brief Blends a single source pixel onto a single destination pixel using relative alpha, source alpha ignored.
 *
 * This is a point (single pixel) version of _op_blend_rel_pan_dp.
 * dst = (dst_alpha / 255) * src
 * Note: MUL_SYM(da, s) is equivalent to (da * s) / 255.
 *
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel value (DATA32).
 */
static void
_op_blend_rel_pt_pan_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = MUL_SYM(*d >> 24, s);
}

#define _op_blend_rel_pt_pas_dp _op_blend_rel_pt_p_dp

#define _op_blend_rel_pt_p_dpan _op_blend_pt_p_dpan
#define _op_blend_rel_pt_pan_dpan _op_blend_pt_pan_dpan
#define _op_blend_rel_pt_pas_dpan _op_blend_pt_pas_dpan

/**
 * @brief Initializes the point blending functions for relative pixel operations.
 *
 * This function assigns the appropriate C-specific blending functions
 * to the global function pointer array `op_blend_rel_pt_funcs`.
 * These functions handle blending single pixels with relative alpha.
 * Array indices have similar meanings as in init_blend_pixel_span_funcs_c.
 */
static void
init_blend_rel_pixel_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_pt_p_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_pt_pas_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_blend_rel_pt_pan_dp;

   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_p_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_pas_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_pan_dpan;
}
