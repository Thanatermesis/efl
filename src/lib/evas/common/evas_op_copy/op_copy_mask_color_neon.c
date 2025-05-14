/* copy mask x color -> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a color value to a destination buffer, modulated by a mask. (NEON version)
 *
 * This function iterates over 'l' pixels. For each pixel, it takes a mask value
 * from 'm', a constant color 'c', and writes the result to the destination 'd'.
 * The source 's' is unused in this specific operation.
 * If the mask value is 0, the destination is set to 0.
 * If the mask value is 255, the destination is set to 'c'.
 * Otherwise, the destination is 'c' multiplied by (mask + 1) / 256.
 *
 * @param s Unused source data pointer.
 * @param m Pointer to the mask data (array of DATA8). Each element is an alpha value (0-255).
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to the destination data buffer (array of DATA32).
 * @param l The number of pixels to process.
 */
static void
_op_copy_mas_c_dp_neon(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = m*c */
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             *d = c;
                             break;
                          default:
                             alpha++;
                             *d = MUL_256(alpha, c);
                             break;
                          }
                        m++;  d++;
                     });
}

#define _op_copy_mas_cn_dp_neon _op_copy_mas_c_dp_neon
#define _op_copy_mas_can_dp_neon _op_copy_mas_c_dp_neon
#define _op_copy_mas_caa_dp_neon _op_copy_mas_c_dp_neon

#define _op_copy_mas_c_dpan_neon _op_copy_mas_c_dp_neon
#define _op_copy_mas_cn_dpan_neon _op_copy_mas_c_dpan_neon
#define _op_copy_mas_can_dpan_neon _op_copy_mas_c_dpan_neon
#define _op_copy_mas_caa_dpan_neon _op_copy_mas_c_dpan_neon

/**
 * @brief Initializes the span operation function pointers for copy_mask_color operations using NEON.
 *
 * This function assigns the NEON-optimized version of the copy_mask_color operation
 * to the appropriate entries in the `op_copy_span_funcs` table. This table is
 * likely used for dispatching to the correct rasterizer function based on
 * source, mask, color, destination properties, and CPU capabilities.
 *
 * Variants assigned:
 * - No source, Alpha mask, No color channel, Destination opaque, NEON CPU
 * - No source, Alpha mask, Color channel, Destination opaque, NEON CPU
 * - No source, Alpha mask, Alpha in color channel, Destination opaque, NEON CPU
 * - No source, Alpha mask, Alpha and Alpha in color channel, Destination opaque, NEON CPU
 * - Similar variants for Destination with Alpha (DP_AN).
 */
static void
init_copy_mask_color_span_funcs_neon(void)
{
   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_mas_cn_dp_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP][CPU_NEON] = _op_copy_mas_c_dp_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_NEON] = _op_copy_mas_can_dp_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_NEON] = _op_copy_mas_caa_dp_neon;

   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_mas_cn_dpan_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_NEON] = _op_copy_mas_c_dpan_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_NEON] = _op_copy_mas_can_dpan_neon;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_NEON] = _op_copy_mas_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a color value to a single destination pixel, modulated by a mask and blended with the original destination. (NEON version)
 *
 * This function applies a color 'c' to a single destination pixel 'd',
 * where the mask 'm' controls the blending. The source 's' is unused.
 * The operation is `*d = INTERP_256(m + 1, c, *d)`, which typically means:
 * `*d = (c * (m + 1) + *d * (256 - (m + 1))) / 256`.
 *
 * @param s Unused source data.
 * @param m The mask value (DATA8, 0-255).
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to the destination pixel (DATA32).
 */
static void
_op_copy_pt_mas_c_dp_neon(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
   *d = INTERP_256(m + 1, c, *d);
}

#define _op_copy_pt_mas_cn_dp_neon _op_copy_pt_mas_c_dp_neon
#define _op_copy_pt_mas_can_dp_neon _op_copy_pt_mas_c_dp_neon
#define _op_copy_pt_mas_caa_dp_neon _op_copy_pt_mas_c_dp_neon

#define _op_copy_pt_mas_c_dpan_neon _op_copy_pt_mas_c_dp_neon
#define _op_copy_pt_mas_cn_dpan_neon _op_copy_pt_mas_c_dpan_neon
#define _op_copy_pt_mas_can_dpan_neon _op_copy_pt_mas_c_dpan_neon
#define _op_copy_pt_mas_caa_dpan_neon _op_copy_pt_mas_c_dpan_neon

/**
 * @brief Initializes the point operation function pointers for copy_mask_color operations using NEON.
 *
 * This function assigns the NEON-optimized version of the point copy_mask_color operation
 * to the appropriate entries in the `op_copy_pt_funcs` table. This table is
 * used for dispatching to the correct rasterizer function for single pixel operations
 * based on source, mask, color, destination properties, and CPU capabilities.
 *
 * Variants assigned are similar to `init_copy_mask_color_span_funcs_neon` but for point operations.
 */
static void
init_copy_mask_color_pt_funcs_neon(void)
{
   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pt_mas_cn_dp_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP][CPU_NEON] = _op_copy_pt_mas_c_dp_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_NEON] = _op_copy_pt_mas_can_dp_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_NEON] = _op_copy_pt_mas_caa_dp_neon;

   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_mas_cn_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_NEON] = _op_copy_pt_mas_c_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_NEON] = _op_copy_pt_mas_can_dpan_neon;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_NEON] = _op_copy_pt_mas_caa_dpan_neon;
}
#endif

/*-----*/

/* copy_rel mask x color -> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a color value to a destination buffer, modulated by a mask and relative to destination alpha. (NEON version)
 *
 * This function iterates over 'l' pixels. For each pixel, it takes a mask value
 * from 'm', a constant color 'c', and modifies the destination 'd'.
 * The source 's' is unused.
 * The operation depends on the mask value:
 * - If mask is 0, destination is unchanged.
 * - If mask is 255, destination alpha is effectively increased, and then 'c' is multiplied by this new alpha.
 *   `color = 1 + (*d >> 24); *d = MUL_256(color, c);`
 * - Otherwise, it performs an interpolation based on the mask value, destination alpha, and 'c'.
 *   `DATA32 da = 1 + (*d >> 24); da = MUL_256(da, c); color = *m + 1; *d = INTERP_256(color, da, *d);`
 *
 * @note This function is marked with "FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED".
 *
 * @param s Unused source data pointer.
 * @param m Pointer to the mask data (array of DATA8). Each element is an alpha value (0-255).
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to the destination data buffer (array of DATA32).
 * @param l The number of pixels to process.
 */
static void
_op_copy_rel_mas_c_dp_neon(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   // FIXME: neon-it
   DATA32 *e;
   int color;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        color = *m;
                        switch(color)
                          {
                          case 0:
                             break;
                          case 255:
                             color = 1 + (*d >> 24);
                             *d = MUL_256(color, c);
                             break;
                          default:
                               {
                                  DATA32 da = 1 + (*d >> 24);
                                  da = MUL_256(da, c);
                                  color++;
                                  *d = INTERP_256(color, da, *d);
                               }
                             break;
                          }
                        m++;  d++;
                     });
}

#define _op_copy_rel_mas_cn_dp_neon _op_copy_rel_mas_c_dp_neon
#define _op_copy_rel_mas_can_dp_neon _op_copy_rel_mas_c_dp_neon
#define _op_copy_rel_mas_caa_dp_neon _op_copy_rel_mas_c_dp_neon

#define _op_copy_rel_mas_c_dpan_neon _op_copy_mas_c_dpan_neon
#define _op_copy_rel_mas_cn_dpan_neon _op_copy_mas_cn_dpan_neon
#define _op_copy_rel_mas_can_dpan_neon _op_copy_mas_can_dpan_neon
#define _op_copy_rel_mas_caa_dpan_neon _op_copy_mas_caa_dpan_neon

/**
 * @brief Initializes the span operation function pointers for copy_relative_mask_color operations using NEON.
 *
 * This function assigns the NEON-optimized version of the copy_relative_mask_color operation
 * to the appropriate entries in the `op_copy_rel_span_funcs` table. This table is
 * likely used for dispatching to the correct rasterizer function based on
 * source, mask, color, destination properties, and CPU capabilities, specifically for
 * operations where the color application is relative to the destination's existing alpha.
 *
 * Variants assigned are similar to `init_copy_mask_color_span_funcs_neon` but for relative copy operations.
 */
static void
init_copy_rel_mask_color_span_funcs_neon(void)
{
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_mas_cn_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_NEON] = _op_copy_rel_mas_c_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_NEON] = _op_copy_rel_mas_can_dp_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_NEON] = _op_copy_rel_mas_caa_dp_neon;

   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_mas_cn_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_NEON] = _op_copy_rel_mas_c_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_mas_can_dpan_neon;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_mas_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a color value to a single destination pixel, modulated by a mask and relative to destination alpha, blended with original destination. (NEON version)
 *
 * This function applies a color 'c' to a single destination pixel 'd'.
 * The mask 'm' controls the blending strength. The operation is relative to the
 * destination pixel's original alpha.
 * The source parameter 's' is reused here to store an intermediate calculation based on destination alpha and color 'c'.
 * `s_intermediate = MUL_256(1 + (*d >> 24), c);`
 * `*d = INTERP_256(m + 1, s_intermediate, *d);`
 *
 * @note This function is marked with "FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED".
 *
 * @param s Unused source data, but its variable is reused for intermediate calculation.
 * @param m The mask value (DATA8, 0-255).
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to the destination pixel (DATA32).
 */
static void
_op_copy_rel_pt_mas_c_dp_neon(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   s = 1 + (*d >> 24); // s is reused here, not the original source data. It calculates (1 + destination_alpha)
   s = MUL_256(s, c);   // s becomes ( (1 + destination_alpha)/256 * c )
   *d = INTERP_256(m + 1, s, *d); // Blends the calculated color with original *d based on mask m
}

#define _op_copy_rel_pt_mas_cn_dp_neon _op_copy_rel_pt_mas_c_dp_neon
#define _op_copy_rel_pt_mas_can_dp_neon _op_copy_rel_pt_mas_c_dp_neon
#define _op_copy_rel_pt_mas_caa_dp_neon _op_copy_rel_pt_mas_c_dp_neon

#define _op_copy_rel_pt_mas_c_dpan_neon _op_copy_pt_mas_c_dpan_neon
#define _op_copy_rel_pt_mas_cn_dpan_neon _op_copy_pt_mas_cn_dpan_neon
#define _op_copy_rel_pt_mas_can_dpan_neon _op_copy_pt_mas_can_dpan_neon
#define _op_copy_rel_pt_mas_caa_dpan_neon _op_copy_pt_mas_caa_dpan_neon

/**
 * @brief Initializes the point operation function pointers for copy_relative_mask_color operations using NEON.
 *
 * This function assigns the NEON-optimized version of the point copy_relative_mask_color operation
 * to the appropriate entries in the `op_copy_rel_pt_funcs` table. This table is
 * used for dispatching to the correct rasterizer function for single pixel operations
 * where the color application is relative to the destination's existing alpha.
 *
 * Variants assigned are similar to `init_copy_mask_color_pt_funcs_neon` but for relative copy point operations.
 */
static void
init_copy_rel_mask_color_pt_funcs_neon(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_mas_cn_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_NEON] = _op_copy_rel_pt_mas_c_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_NEON] = _op_copy_rel_pt_mas_can_dp_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_NEON] = _op_copy_rel_pt_mas_caa_dp_neon;

   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_mas_cn_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_NEON] = _op_copy_rel_pt_mas_c_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_NEON] = _op_copy_rel_pt_mas_can_dpan_neon;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_NEON] = _op_copy_rel_pt_mas_caa_dpan_neon;
}
#endif
