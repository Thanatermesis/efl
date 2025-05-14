/* blend color -> dst */

/**
 * @brief Blends a solid color onto a destination buffer.
 *
 * This function takes a solid color @p c and blends it over @p l pixels
 * in the destination buffer @p d. The source buffer @p s and mask @p m are
 * unused in this specific operation.
 * The blending formula used is: (*d) = c + ( (256 - (c >> 24)) * (*d) ) / 256
 * which simplifies to standard alpha blending where the source is a solid color.
 *
 * @param s Pointer to the source data (unused).
 * @param m Pointer to the mask data (unused).
 * @param c The solid color to blend (in ARGB format, e.g., 0xAARRGGBB).
 *          The alpha component of c (c >> 24) is used to determine transparency.
 * @param d Pointer to the destination buffer. Each element is a DATA32 pixel.
 *          Example: {0xFF112233, 0xFF445566, ...}
 * @param l The number of pixels to process.
 */
static void
_op_blend_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
    DATA32 *e, a = 256 - (c >> 24); // Pre-calculate (256 - source_alpha) for efficiency
    UNROLL8_PLD_WHILE(d, l, e,
                      {
                         // Apply the blend operation: Dst = Src + (1 - Src_alpha) * Dst
                         // Where Src is the solid color 'c'
                         // and Src_alpha is (c >> 24) / 256.
                         // MUL_256(a, *d) is equivalent to (a * (*d)) / 256
                         *d = c + MUL_256(a, *d);
                         d++;
                      });
}

#define _op_blend_caa_dp _op_blend_c_dp

#define _op_blend_c_dpan _op_blend_c_dp
#define _op_blend_caa_dpan _op_blend_c_dpan

/**
 * @brief Initializes the span blending functions for solid color operations.
 *
 * This function assigns the appropriate blending functions (e.g., _op_blend_c_dp)
 * to the global function pointer array `op_blend_span_funcs`. These functions
 * are used for blending a solid color over a span of pixels.
 * It covers cases with and without anti-aliasing (SC vs SC_AA) and
 * with and without destination alpha (DP vs DP_AN).
 */
static void
init_blend_color_span_funcs_c(void)
{
   op_blend_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_blend_c_dp;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_blend_caa_dp;

   op_blend_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_blend_c_dpan;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_caa_dpan;
}

/**
 * @brief Blends a solid color onto a single destination pixel.
 *
 * This function takes a solid color @p c and blends it onto a single pixel
 * pointed to by @p d. The source pixel value @p s and mask @p m are unused.
 * The blending formula is: *d = c + ( (256 - (c >> 24)) * (*d) ) / 256.
 *
 * @param s The source pixel data (unused in this function, but kept for API consistency).
 * @param m The mask data (unused).
 * @param c The solid color to blend (in ARGB format, e.g., 0xAARRGGBB).
 *          The alpha component of c (c >> 24) is used for blending.
 * @param d Pointer to the destination pixel (DATA32).
 *          Example: A pointer to a single 0xFF112233 value.
 */
static void
_op_blend_pt_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   // 's' here is reused to store 256 - source_alpha for the MUL_256 macro
   s = 256 - (c >> 24);
   // Apply the blend operation: Dst = Src + (1 - Src_alpha) * Dst
   // Where Src is the solid color 'c'
   *d = c + MUL_256(s, *d);
}

#define _op_blend_pt_caa_dp _op_blend_pt_c_dp

#define _op_blend_pt_c_dpan _op_blend_pt_c_dp
#define _op_blend_pt_caa_dpan _op_blend_pt_c_dpan

#define _op_blend_pt_c_dpas _op_blend_pt_c_dp
#define _op_blend_pt_caa_dpas _op_blend_pt_c_dp

/**
 * @brief Initializes the point blending functions for solid color operations.
 *
 * This function assigns the appropriate blending functions (e.g., _op_blend_pt_c_dp)
 * to the global function pointer array `op_blend_pt_funcs`. These functions
 * are used for blending a solid color onto a single pixel.
 * It covers cases with and without anti-aliasing (SC vs SC_AA) and
 * with and without destination alpha (DP vs DP_AN).
 */
static void
init_blend_color_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_blend_pt_c_dp;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_blend_pt_caa_dp;

   op_blend_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_blend_pt_c_dpan;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_pt_caa_dpan;
}

/*-----*/

/* blend_rel color -> dst */

/**
 * @brief Blends a solid color onto a destination buffer using relative alpha.
 *
 * This function blends a solid color @p c over @p l pixels in the destination
 * buffer @p d. The blending is "relative", meaning the source color's alpha
 * is modulated by the destination's alpha.
 * The formula is: *d = ( (*d >> 24) * c ) / 256 + ( (256 - (c >> 24)) * (*d) ) / 256
 * This can be interpreted as: Dst = Dst_alpha * Src + (1 - Src_alpha) * Dst.
 *
 * @param s Pointer to the source data (unused).
 * @param m Pointer to the mask data (unused).
 * @param c The solid color to blend (in ARGB format, e.g., 0xAARRGGBB).
 *          The alpha component of c (c >> 24) is used.
 * @param d Pointer to the destination buffer. Each element is a DATA32 pixel.
 *          Example: {0xFF112233, 0xFF445566, ...}
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha = 256 - (c >> 24); // Pre-calculate (256 - source_color_alpha)
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        // Apply relative blend: Dst = (Dst_alpha/256) * Src_color + (1 - Src_color_alpha/256) * Dst_color
                        // MUL_SYM(dst_alpha, src_color) is (dst_alpha * src_color) / 255 (approximately / 256)
                        // MUL_256( (1-src_alpha), dst_color) is ( (1-src_alpha) * dst_color) / 256
                        *d = MUL_SYM(*d >> 24, c) + MUL_256(alpha, *d);
                        d++;
                     });
}

#define _op_blend_rel_caa_dp _op_blend_rel_c_dp

#define _op_blend_rel_c_dpan _op_blend_c_dpan
#define _op_blend_rel_caa_dpan _op_blend_caa_dpan

/**
 * @brief Initializes the span blending functions for relative solid color operations.
 *
 * This function assigns the appropriate "relative" blending functions
 * (e.g., _op_blend_rel_c_dp) to the global function pointer array
 * `op_blend_rel_span_funcs`. These functions are used for blending a solid
 * color over a span of pixels, where the source color's alpha is modulated
 * by the destination's alpha.
 * It covers cases with and without anti-aliasing (SC vs SC_AA) and
 * with and without destination alpha (DP vs DP_AN).
 */
static void
init_blend_rel_color_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_blend_rel_c_dp;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_caa_dp;

   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_c_dpan;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_caa_dpan;
}

/**
 * @brief Blends a solid color onto a single destination pixel using relative alpha.
 *
 * This function blends a solid color @p c onto a single pixel pointed to by @p d.
 * The blending is "relative", meaning the source color's alpha is modulated by
 * the destination's alpha. The source pixel value @p s is reused to store
 * destination alpha. Mask @p m is unused.
 * The formula is: *d = ( (*d >> 24) * c ) / 256 + ( (256 - (c >> 24)) * (*d) ) / 256.
 *
 * @param s The source pixel data (reused to store destination alpha for the MUL_SYM macro).
 * @param m The mask data (unused).
 * @param c The solid color to blend (in ARGB format, e.g., 0xAARRGGBB).
 *          The alpha component of c (c >> 24) is used.
 * @param d Pointer to the destination pixel (DATA32).
 *          Example: A pointer to a single 0xFF112233 value.
 */
static void
_op_blend_rel_pt_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   // 's' here is reused to store destination_alpha for the MUL_SYM macro
   s = *d >> 24; // Extract destination alpha
   // Apply relative blend: Dst = (Dst_alpha/256) * Src_color + (1 - Src_color_alpha/256) * Dst_color
   *d = MUL_SYM(s, c) + MUL_256(256 - (c >> 24), *d);
}

#define _op_blend_rel_pt_caa_dp _op_blend_rel_pt_c_dp

#define _op_blend_rel_pt_c_dpan _op_blend_pt_c_dpan
#define _op_blend_rel_pt_caa_dpan _op_blend_pt_caa_dpan

/**
 * @brief Initializes the point blending functions for relative solid color operations.
 *
 * This function assigns the appropriate "relative" blending functions
 * (e.g., _op_blend_rel_pt_c_dp) to the global function pointer array
 * `op_blend_rel_pt_funcs`. These functions are used for blending a solid
 * color onto a single pixel, where the source color's alpha is modulated
 * by the destination's alpha.
 * It covers cases with and without anti-aliasing (SC vs SC_AA) and
 * with and without destination alpha (DP vs DP_AN).
 */
static void
init_blend_rel_color_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_blend_rel_pt_c_dp;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_blend_rel_pt_caa_dp;

   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_blend_rel_pt_c_dpan;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_caa_dpan;
}
