/* blend mask x color -> dst */

/**
 * @brief Blends a color with the destination buffer using a mask.
 *
 * This function applies a color to a destination buffer, where the blending
 * is controlled by a mask. The source data (s) is unused.
 * The operation is d = (c * m) + (d * (1 - (c * m)a)).
 *
 * @param s Source data (unused).
 * @param m Pointer to the mask data (array of DATA8). Each value represents opacity (0-255).
 * @param c The color to blend (DATA32, ARGB format).
 * @param d Pointer to the destination buffer (array of DATA32, ARGB format).
 * @param l Length of the data arrays (number of pixels).
 */
static void
_op_blend_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha = 256 - (c >> 24); // Pre-calculate 256 - alpha of the color
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 a = *m;
                        switch(a)
                          {
                          case 0:
                             break;
                          case 255:
                             *d = c + MUL_256(alpha, *d);
                             break;
                          default:
                               {
                                  DATA32 mc = MUL_SYM(a, c);
                                  a = 256 - (mc >> 24);
                                  *d = mc + MUL_256(a, *d);
                               }
                             break;
                          }
                        m++;  d++;
                     });
}

/**
 * @brief Blends a color with the destination buffer using a mask, assuming color has alpha.
 *
 * This function applies a color (which may have an alpha component) to a
 * destination buffer, controlled by a mask. The source data (s) is unused.
 * If mask is 255, d = c. Otherwise, d = INTERP_256(m+1, c, d).
 *
 * @param s Source data (unused).
 * @param m Pointer to the mask data (array of DATA8). Each value represents opacity (0-255).
 * @param c The color to blend (DATA32, ARGB format).
 * @param d Pointer to the destination buffer (array of DATA32, ARGB format).
 * @param l Length of the data arrays (number of pixels).
 */
static void
_op_blend_mas_can_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha; // Mask value used as alpha
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             break;
                          case 255:
                             *d = c;
                             break;
                          default:
                             alpha++;
                             *d = INTERP_256(alpha, c, *d);
                             break;
                          }
                        m++;  d++;
                     });
}

#define _op_blend_mas_cn_dp _op_blend_mas_can_dp
#define _op_blend_mas_caa_dp _op_blend_mas_c_dp

#define _op_blend_mas_c_dpan _op_blend_mas_c_dp
#define _op_blend_mas_cn_dpan _op_blend_mas_cn_dp
#define _op_blend_mas_can_dpan _op_blend_mas_can_dp
#define _op_blend_mas_caa_dpan _op_blend_mas_caa_dp

/**
 * @brief Initializes span blending functions for mask and color operations.
 *
 * This function assigns the appropriate blending functions to the
 * op_blend_span_funcs array for various combinations of source, mask,
 * and destination properties. These functions operate on spans (lines) of pixels.
 */
static void
init_blend_mask_color_span_funcs_c(void)
{
   op_blend_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_blend_mas_c_dp;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_blend_mas_cn_dp;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_blend_mas_can_dp;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_blend_mas_caa_dp;

   op_blend_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_blend_mas_c_dpan;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_mas_cn_dpan;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_blend_mas_can_dpan;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_blend_mas_caa_dpan;
}

/**
 * @brief Blends a single point (pixel) of color with the destination using a mask.
 *
 * This function applies a color to a single destination pixel, where the
 * blending is controlled by a mask value.
 * The operation is d = (c * m) + (d * (1 - (c * m)a)).
 *
 * @param s Source data (unused, but passed for API consistency).
 * @param m Mask value (DATA8, 0-255).
 * @param c The color to blend (DATA32, ARGB format).
 * @param d Pointer to the destination pixel (DATA32, ARGB format).
 */
static void
_op_blend_pt_mas_c_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   s = MUL_SYM(m, c); // Modulated color by mask
   m = 255 - (s >> 24); // Alpha for destination blending (255 - resulting alpha of s)
   *d = s + MUL_256(m, *d); // Blend modulated color with destination
}

/**
 * @brief Blends a single point (pixel) of color (with alpha) with the destination using a mask.
 *
 * This function applies a color (which may have an alpha component) to a single
 * destination pixel, controlled by a mask value.
 * The operation is d = INTERP_256(m+1, c, d).
 *
 * @param s Source data (unused).
 * @param m Mask value (DATA8, 0-255).
 * @param c The color to blend (DATA32, ARGB format).
 * @param d Pointer to the destination pixel (DATA32, ARGB format).
 */
static void
_op_blend_pt_mas_can_dp(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
   *d = INTERP_256(m + 1, c, *d); // Interpolate color with destination based on mask
}

#define _op_blend_pt_mas_cn_dp _op_blend_pt_mas_can_dp
#define _op_blend_pt_mas_caa_dp _op_blend_pt_mas_c_dp

#define _op_blend_pt_mas_c_dpan _op_blend_pt_mas_c_dp
#define _op_blend_pt_mas_cn_dpan _op_blend_pt_mas_cn_dp
#define _op_blend_pt_mas_can_dpan _op_blend_pt_mas_can_dp
#define _op_blend_pt_mas_caa_dpan _op_blend_pt_mas_caa_dp

/**
 * @brief Initializes point blending functions for mask and color operations.
 *
 * This function assigns the appropriate blending functions to the
 * op_blend_pt_funcs array for various combinations of source, mask,
 * and destination properties. These functions operate on single pixels.
 */
static void
init_blend_mask_color_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_blend_pt_mas_c_dp;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_blend_pt_mas_cn_dp;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_blend_pt_mas_can_dp;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_blend_pt_mas_caa_dp;

   op_blend_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_blend_pt_mas_c_dpan;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pt_mas_cn_dpan;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_blend_pt_mas_can_dpan;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_blend_pt_mas_caa_dpan;
}

/*-----*/

/* blend_rel mask x color --> dst */

/**
 * @brief Blends a color with the destination buffer using a mask, relative to destination alpha.
 *
 * This function applies a color to a destination buffer, controlled by a mask.
 * The blending is relative to the destination pixel's alpha.
 * The operation is d = (d_alpha * c * m) + (d * (1 - (c * m)a)).
 *
 * @param s Source data (unused).
 * @param m Pointer to the mask data (array of DATA8). Each value represents opacity (0-255).
 * @param c The color to blend (DATA32, ARGB format).
 * @param d Pointer to the destination buffer (array of DATA32, ARGB format).
 * @param l Length of the data arrays (number of pixels).
 */
static void
_op_blend_rel_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha; // Alpha for destination blending
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 mc = MUL_SYM(*m, c);
                        alpha = 256 - (mc >> 24);
                        *d = MUL_SYM(*d >> 24, mc) + MUL_256(alpha, *d);
                        d++;
                        m++;
                     });
}

#define _op_blend_rel_mas_cn_dp _op_blend_rel_mas_c_dp
#define _op_blend_rel_mas_can_dp _op_blend_rel_mas_c_dp
#define _op_blend_rel_mas_caa_dp _op_blend_rel_mas_c_dp

#define _op_blend_rel_mas_c_dpan _op_blend_mas_c_dpan
#define _op_blend_rel_mas_cn_dpan _op_blend_mas_cn_dpan
#define _op_blend_rel_mas_can_dpan _op_blend_mas_can_dpan
#define _op_blend_rel_mas_caa_dpan _op_blend_mas_caa_dpan

/**
 * @brief Initializes relative span blending functions for mask and color operations.
 *
 * This function assigns the appropriate relative blending functions to the
 * op_blend_rel_span_funcs array. "Relative" means the blending takes into
 * account the destination alpha. These functions operate on spans (lines) of pixels.
 */
static void
init_blend_rel_mask_color_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_blend_rel_mas_c_dp;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_mas_can_dp;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_blend_rel_mas_can_dp;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_blend_rel_mas_caa_dp;

   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_blend_rel_mas_c_dpan;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_mas_cn_dpan;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_blend_rel_mas_can_dpan;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_blend_rel_mas_caa_dpan;
}

/**
 * @brief Blends a single point (pixel) of color with the destination using a mask, relative to destination alpha.
 *
 * This function applies a color to a single destination pixel, controlled by a
 * mask value. The blending is relative to the destination pixel's alpha.
 * The operation is d = (d_alpha * c * m) + (d * (1 - (c * m)a)).
 *
 * @param s Source data (passed for API consistency, used to store intermediate modulated color).
 * @param m Mask value (DATA8, 0-255).
 * @param c The color to blend (DATA32, ARGB format, used to store intermediate alpha).
 * @param d Pointer to the destination pixel (DATA32, ARGB format).
 */
static void
_op_blend_rel_pt_mas_c_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   s = MUL_SYM(m, c); // Modulated color by mask
   c = 256 - (s >> 24); // Alpha for destination blending (255 - resulting alpha of s)
   *d = MUL_SYM(*d >> 24, s) + MUL_256(c, *d); // Blend modulated color with destination, relative to dest alpha
}

#define _op_blend_rel_pt_mas_cn_dp _op_blend_rel_pt_mas_c_dp
#define _op_blend_rel_pt_mas_can_dp _op_blend_rel_pt_mas_c_dp
#define _op_blend_rel_pt_mas_caa_dp _op_blend_rel_pt_mas_c_dp

#define _op_blend_rel_pt_mas_c_dpan _op_blend_pt_mas_c_dpan
#define _op_blend_rel_pt_mas_cn_dpan _op_blend_pt_mas_cn_dpan
#define _op_blend_rel_pt_mas_can_dpan _op_blend_pt_mas_can_dpan
#define _op_blend_rel_pt_mas_caa_dpan _op_blend_pt_mas_caa_dpan

/**
 * @brief Initializes relative point blending functions for mask and color operations.
 *
 * This function assigns the appropriate relative blending functions to the
 * op_blend_rel_pt_funcs array. "Relative" means the blending takes into
 * account the destination alpha. These functions operate on single pixels.
 */
static void
init_blend_rel_mask_color_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_blend_rel_pt_mas_c_dp;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pt_mas_cn_dp;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_blend_rel_pt_mas_can_dp;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_blend_rel_pt_mas_caa_dp;

   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_blend_rel_pt_mas_c_dpan;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_mas_cn_dpan;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_blend_rel_pt_mas_can_dpan;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_blend_rel_pt_mas_caa_dpan;
}
