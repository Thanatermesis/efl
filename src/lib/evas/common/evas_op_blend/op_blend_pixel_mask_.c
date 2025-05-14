/* blend pixel x mask --> dst */

/**
 * @brief Blends a source pixel array with a destination pixel array using a mask.
 *
 * This function performs a pixel-wise blend operation. The blending is
 * controlled by a mask. If the mask value is 0, the destination pixel
 * remains unchanged. If the mask value is 255, a standard alpha blend
 * is performed. Otherwise, the source pixel is first multiplied by the
 * mask value (scaled appropriately), and then blended with the destination.
 *
 * @param s Pointer to the source pixel data array (DATA32).
 *          Each element is a 32-bit pixel (e.g., ARGB).
 * @param m Pointer to the mask data array (DATA8).
 *          Each element is an 8-bit alpha value (0-255).
 * @param c Temporary variable for color calculations (DATA32). Unused if alpha is 0 or 255.
 * @param d Pointer to the destination pixel data array (DATA32).
 *          This array is modified in place.
 * @param l The number of pixels to process.
 */
static void
_op_blend_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             break;
                          case 255:
                             alpha = 256 - (*s >> 24);
                             *d = *s + MUL_256(alpha, *d);
                             break;
                          default:
                             c = MUL_SYM(alpha, *s);
                             alpha = 256 - (c >> 24);
                             *d = c + MUL_256(alpha, *d);
                             break;
                          }
                        m++;  s++;  d++;
                     });
}

#define _op_blend_pas_mas_dp _op_blend_p_mas_dp
#define _op_blend_pan_mas_dp _op_blend_pas_mas_dp

#define _op_blend_p_mas_dpan _op_blend_p_mas_dp
#define _op_blend_pas_mas_dpan _op_blend_pas_mas_dp
#define _op_blend_pan_mas_dpan _op_blend_pan_mas_dp

/**
 * @brief Initializes the span blending functions for pixel mask operations.
 *
 * This function assigns the appropriate blending functions (like _op_blend_p_mas_dp)
 * to the op_blend_span_funcs array based on various source, mask, and destination
 * properties (e.g., presence of alpha, solid mask).
 * It covers cases for normal pixels (SP), alpha-premultiplied source (SP_AS),
 * and alpha-not-premultiplied source (SP_AN), all with solid mask (SM_AS),
 * no source color key (SC_N), and destination with or without alpha (DP, DP_AN).
 */
static void
init_blend_pixel_mask_span_funcs_c(void)
{
   op_blend_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_blend_p_mas_dp;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_blend_pas_mas_dp;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_blend_pan_mas_dp;

   op_blend_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_p_mas_dpan;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pas_mas_dpan;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pan_mas_dpan;
}

/**
 * @brief Blends a single source pixel with a single destination pixel using a mask.
 *
 * This function applies a mask to a source pixel and then blends it
 * with a destination pixel. The source pixel is first multiplied by the
 * mask value (scaled). The resulting alpha determines the blending factor.
 *
 * @param s The source pixel (DATA32).
 * @param m The mask value (DATA8, 0-255).
 * @param c Temporary variable for alpha calculation (DATA32).
 * @param d Pointer to the destination pixel (DATA32). This is modified in place.
 */
static void
_op_blend_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   s = MUL_SYM(m, s);
   c = 256 - (s >> 24);
   *d = s + MUL_256(c, *d);
}

/**
 * @brief Blends a single non-alpha-premultiplied source pixel with a destination pixel using a mask.
 *
 * This function performs an interpolation between the source and destination
 * pixels based on the mask value. It's typically used when the source pixel
 * is not alpha-premultiplied.
 *
 * @param s The source pixel (DATA32).
 * @param m The mask value (DATA8, 0-255).
 * @param c Unused parameter (DATA32).
 * @param d Pointer to the destination pixel (DATA32). This is modified in place.
 */
static void
_op_blend_pt_pan_mas_dp(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = INTERP_256(m + 1, s, *d);
}

#define _op_blend_pt_pas_mas_dp _op_blend_pt_p_mas_dp

#define _op_blend_pt_p_mas_dpan _op_blend_pt_p_mas_dp
#define _op_blend_pt_pas_mas_dpan _op_blend_pt_pas_mas_dp
#define _op_blend_pt_pan_mas_dpan _op_blend_pt_pan_mas_dp

/**
 * @brief Initializes the point blending functions for pixel mask operations.
 *
 * This function assigns the appropriate single-pixel blending functions
 * (like _op_blend_pt_p_mas_dp, _op_blend_pt_pan_mas_dp) to the op_blend_pt_funcs
 * array. The assignments are based on various source, mask, and destination
 * properties, similar to init_blend_pixel_mask_span_funcs_c, but for
 * single point operations.
 */
static void
init_blend_pixel_mask_pt_funcs_c(void)
{
   op_blend_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_blend_pt_p_mas_dp;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_blend_pt_pas_mas_dp;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_blend_pt_pan_mas_dp;

   op_blend_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pt_p_mas_dpan;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pt_pas_mas_dpan;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_pt_pan_mas_dpan;
}

/*-----*/

/* blend_rel pixel x mask -> dst */

/**
 * @brief Blends a source pixel array with a destination pixel array using a mask, with relative alpha.
 *
 * This function performs a pixel-wise blend operation where the source pixel's
 * alpha is modulated by the destination pixel's alpha. The mask further
 * modulates the source pixel before blending.
 *
 * @param s Pointer to the source pixel data array (DATA32).
 * @param m Pointer to the mask data array (DATA8).
 * @param c Temporary variable for color calculations (DATA32).
 * @param d Pointer to the destination pixel data array (DATA32). Modified in place.
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
           {
            c = MUL_SYM(*m, *s);
            alpha = 256 - (c >> 24);
            *d = MUL_SYM(*d >> 24, c) + MUL_256(alpha, *d);
            d++; m++; s++;
           });
}

#define _op_blend_rel_pas_mas_dp _op_blend_rel_p_mas_dp
#define _op_blend_rel_pan_mas_dp _op_blend_rel_p_mas_dp

#define _op_blend_rel_p_mas_dpan _op_blend_p_mas_dpan
#define _op_blend_rel_pas_mas_dpan _op_blend_pas_mas_dpan
#define _op_blend_rel_pan_mas_dpan _op_blend_pan_mas_dpan

/**
 * @brief Initializes the span blending functions for relative pixel mask operations.
 *
 * This function assigns relative blending functions (like _op_blend_rel_p_mas_dp)
 * to the op_blend_rel_span_funcs array. "Relative" implies that the source
 * alpha is modulated by the destination's alpha during the blend.
 * The assignments depend on source, mask, and destination properties.
 */
static void
init_blend_rel_pixel_mask_span_funcs_c(void)
{
   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_p_mas_dp;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pas_mas_dp;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pan_mas_dp;

   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_p_mas_dpan;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pas_mas_dpan;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pan_mas_dpan;
}

/**
 * @brief Blends a single source pixel with a destination pixel using a mask, with relative alpha.
 *
 * This function applies a mask to a source pixel and then blends it with a
 * destination pixel. The blending takes into account the destination pixel's
 * alpha, making it a "relative" blend.
 *
 * @param s The source pixel (DATA32).
 * @param m The mask value (DATA8, 0-255).
 * @param c Temporary variable for alpha calculation (DATA32).
 * @param d Pointer to the destination pixel (DATA32). Modified in place.
 */
static void
_op_blend_rel_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   s = MUL_SYM(m, s);
   c = 256 - (s >> 24);
   *d = MUL_SYM(*d >> 24, s) + MUL_256(c, *d);
}

#define _op_blend_rel_pt_pas_mas_dp _op_blend_rel_pt_p_mas_dp
#define _op_blend_rel_pt_pan_mas_dp _op_blend_rel_pt_p_mas_dp

#define _op_blend_rel_pt_p_mas_dpan _op_blend_pt_p_mas_dpan
#define _op_blend_rel_pt_pas_mas_dpan _op_blend_pt_pas_mas_dpan
#define _op_blend_rel_pt_pan_mas_dpan _op_blend_pt_pan_mas_dpan

/**
 * @brief Initializes the point blending functions for relative pixel mask operations.
 *
 * This function assigns relative single-pixel blending functions
 * (like _op_blend_rel_pt_p_mas_dp) to the op_blend_rel_pt_funcs array.
 * These functions handle blending where source alpha is modulated by
 * destination alpha, for various source, mask, and destination properties.
 */
static void
init_blend_rel_pixel_mask_pt_funcs_c(void)
{
   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pt_p_mas_dp;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pt_pas_mas_dp;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_blend_rel_pt_pan_mas_dp;

   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_p_mas_dpan;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_pas_mas_dpan;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_blend_rel_pt_pan_mas_dpan;
}
