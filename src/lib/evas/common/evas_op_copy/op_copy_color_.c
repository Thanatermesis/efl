/* copy color --> dst */

/**
 * @brief Copies a solid color to a destination buffer.
 * @param s Source data pointer (unused).
 * @param m Mask data pointer (unused).
 * @param c The color to copy (in DATA32 format, e.g., 0xAARRGGBB).
 * @param d Destination data pointer.
 * @param l Length of the destination buffer in pixels.
 *
 * This function fills a span of pixels in the destination buffer with the given color 'c'.
 * It uses UNROLL8_PLD_WHILE for optimized copying.
 */
static void
_op_copy_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = c;
                        d++;
                     });
}

#define _op_copy_cn_dp _op_copy_c_dp
#define _op_copy_can_dp _op_copy_c_dp
#define _op_copy_caa_dp _op_copy_c_dp

#define _op_copy_c_dpan _op_copy_c_dp
#define _op_copy_cn_dpan _op_copy_c_dp
#define _op_copy_can_dpan _op_copy_c_dp
#define _op_copy_caa_dpan _op_copy_c_dp

/**
 * @brief Initializes the span copy function pointers for solid color operations.
 *
 * This function assigns the appropriate span copy functions (like _op_copy_c_dp)
 * to the op_copy_span_funcs array for various source color types (SC_N, SC, SC_AN, SC_AA)
 * and destination pixel formats (DP, DP_AN), specifically for CPU_C implementation.
 * SP_N indicates no source pixmap, SM_N indicates no source mask.
 */
static void
init_copy_color_span_funcs_c(void)
{
   op_copy_span_funcs[SP_N][SM_N][SC_N][DP][CPU_C] = _op_copy_cn_dp;
   op_copy_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_copy_c_dp;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_copy_can_dp;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_copy_caa_dp;

   op_copy_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_cn_dpan;
   op_copy_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_copy_c_dpan;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_can_dpan;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_caa_dpan;
}

/**
 * @brief Copies a solid color to a single destination pixel.
 * @param s Source data (unused).
 * @param m Mask data (unused).
 * @param c The color to copy (in DATA32 format, e.g., 0xAARRGGBB).
 * @param d Destination pixel pointer.
 *
 * This function sets a single pixel in the destination to the given color 'c'.
 */
static void
_op_copy_pt_c_dp(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   *d = c;
}

#define _op_copy_pt_cn_dp _op_copy_pt_c_dp
#define _op_copy_pt_can_dp _op_copy_pt_c_dp
#define _op_copy_pt_caa_dp _op_copy_pt_c_dp

#define _op_copy_pt_c_dpan _op_copy_pt_c_dp
#define _op_copy_pt_cn_dpan _op_copy_pt_c_dp
#define _op_copy_pt_can_dpan _op_copy_pt_c_dp
#define _op_copy_pt_caa_dpan _op_copy_pt_c_dp

/**
 * @brief Initializes the point copy function pointers for solid color operations.
 *
 * This function assigns the appropriate point copy functions (like _op_copy_pt_c_dp)
 * to the op_copy_pt_funcs array for various source color types (SC_N, SC, SC_AN, SC_AA)
 * and destination pixel formats (DP, DP_AN), specifically for CPU_C implementation.
 * SP_N indicates no source pixmap, SM_N indicates no source mask.
 */
static void
init_copy_color_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_C] = _op_copy_pt_cn_dp;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_copy_pt_c_dp;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_copy_pt_can_dp;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_copy_pt_caa_dp;

   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pt_cn_dpan;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_copy_pt_c_dpan;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pt_can_dpan;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pt_caa_dpan;
}

/*-----*/

/* copy_rel color --> dst */

/**
 * @brief Copies a color to a destination buffer, modulating with destination alpha.
 * @param s Source data pointer (unused).
 * @param m Mask data pointer (unused).
 * @param c The color to copy (in DATA32 format, e.g., 0xAARRGGBB). The alpha component of this color is used.
 * @param d Destination data pointer. The alpha component of each destination pixel is used for modulation.
 * @param l Length of the destination buffer in pixels.
 *
 * This function fills a span of pixels in the destination buffer. The copied color 'c'
 * is modulated by the alpha component of the *original* destination pixel color.
 * The operation is `*d = MUL_SYM(*d >> 24, c)`, where `*d >> 24` extracts the alpha
 * from the destination pixel, and `c` is the source color.
 * It uses UNROLL8_PLD_WHILE for optimized copying.
 */
static void
_op_copy_rel_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL_SYM(*d >> 24, c);
                        d++;
                     });
}


#define _op_copy_rel_cn_dp _op_copy_rel_c_dp
#define _op_copy_rel_can_dp _op_copy_rel_c_dp
#define _op_copy_rel_caa_dp _op_copy_rel_c_dp

#define _op_copy_rel_c_dpan _op_copy_c_dp
#define _op_copy_rel_cn_dpan _op_copy_cn_dp
#define _op_copy_rel_can_dpan _op_copy_can_dp
#define _op_copy_rel_caa_dpan _op_copy_caa_dp

/**
 * @brief Initializes the span copy_rel function pointers for color operations.
 *
 * This function assigns the appropriate relative span copy functions
 * (like _op_copy_rel_c_dp or aliased direct copy functions like _op_copy_c_dp for dpan variants)
 * to the op_copy_rel_span_funcs array. These functions handle operations where the source color
 * is modulated by the destination alpha.
 * It covers various source color types (SC_N, SC, SC_AN, SC_AA)
 * and destination pixel formats (DP, DP_AN), for CPU_C implementation.
 * SP_N indicates no source pixmap, SM_N indicates no source mask.
 *
 * Note: For DP_AN (destination has no alpha) variants, it falls back to direct copy operations
 * as relative alpha modulation is not applicable.
 */
static void
init_copy_rel_color_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_cn_dp;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_copy_rel_c_dp;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_can_dp;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_caa_dp;

   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_cn_dpan;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_c_dpan;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_can_dpan;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_caa_dpan;
}

/**
 * @brief Copies a color to a single destination pixel, modulating with destination alpha.
 * @param s Source data (used to store temporary alpha calculation: 1 + destination_alpha).
 * @param m Mask data (unused).
 * @param c The color to copy (in DATA32 format, e.g., 0xAARRGGBB).
 * @param d Destination pixel pointer. The alpha component of this pixel is used for modulation.
 *
 * This function sets a single pixel in the destination. The copied color 'c'
 * is modulated by the alpha component of the *original* destination pixel color.
 * The operation is `*d = MUL_256((1 + (*d >> 24)), c)`.
 * `(*d >> 24)` extracts the alpha from the destination pixel.
 */
static void
_op_copy_rel_pt_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = 1 + (*d >> 24); /* s becomes 1 + destination_alpha (0-255 range, so s is 1-256) */
   *d = MUL_256(s, c); /* Modulates color 'c' by 's' (effectively (1+dest_alpha)/256.0 * c) */
}


#define _op_copy_rel_pt_cn_dp _op_copy_rel_pt_c_dp
#define _op_copy_rel_pt_can_dp _op_copy_rel_pt_c_dp
#define _op_copy_rel_pt_caa_dp _op_copy_rel_pt_c_dp

#define _op_copy_rel_pt_c_dpan _op_copy_pt_c_dp
#define _op_copy_rel_pt_cn_dpan _op_copy_pt_cn_dp
#define _op_copy_rel_pt_can_dpan _op_copy_pt_can_dp
#define _op_copy_rel_pt_caa_dpan _op_copy_pt_caa_dp

/**
 * @brief Initializes the point copy_rel function pointers for color operations.
 *
 * This function assigns the appropriate relative point copy functions
 * (like _op_copy_rel_pt_c_dp or aliased direct point copy functions like _op_copy_pt_c_dp for dpan variants)
 * to the op_copy_rel_pt_funcs array. These functions handle operations where the source color
 * is modulated by the destination alpha for a single pixel.
 * It covers various source color types (SC_N, SC, SC_AN, SC_AA)
 * and destination pixel formats (DP, DP_AN), for CPU_C implementation.
 * SP_N indicates no source pixmap, SM_N indicates no source mask.
 *
 * Note: For DP_AN (destination has no alpha) variants, it falls back to direct point copy operations
 * as relative alpha modulation is not applicable.
 */
static void
init_copy_rel_color_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pt_cn_dp;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_copy_rel_pt_c_dp;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pt_can_dp;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pt_caa_dp;

   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_cn_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pt_c_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_can_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_caa_dpan;
}

