/* mask pixel x color --> dst */

/**
 * @brief Applies a color mask to a span of pixels.
 *
 * This function multiplies the destination pixels by a factor derived from
 * the source pixel's alpha and a given color's alpha.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Each DATA32 represents a pixel, typically in ARGB format.
 *          Example: `s[0] = 0xFFRRGGBB` (Alpha, Red, Green, Blue).
 * @param m Pointer to the mask data (array of DATA8). This parameter is unused.
 * @param c The color to use for masking (DATA32). The alpha component of this
 *          color is used in the calculation.
 *          Example: `c = 0xAARRGGBB`.
 * @param d Pointer to the destination pixel data (array of DATA32), which will be modified.
 *          Example: `d[0] = 0xFFRRGGBB`.
 * @param l The number of pixels to process in the span.
 */
static void
_op_mask_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   l = 1 + (c >> 24); // Pre-calculate factor from color's alpha
   while (d < e) {
	c = 1 + ((l * (*s >> 24)) >> 8);
	*d = MUL_256(c, *d);
	s++;  d++;
     }
}

#define _op_mask_pas_c_dp _op_mask_p_c_dp
#define _op_mask_pan_c_dp _op_mask_p_c_dp
#define _op_mask_p_can_dp _op_mask_p_c_dp
#define _op_mask_pas_can_dp _op_mask_p_c_dp
#define _op_mask_p_caa_dp _op_mask_p_c_dp
#define _op_mask_pas_caa_dp _op_mask_p_c_dp
#define _op_mask_pan_caa_dp _op_mask_p_c_dp

#define _op_mask_p_c_dpan _op_mask_p_c_dp
#define _op_mask_pas_c_dpan _op_mask_p_c_dp
#define _op_mask_pan_c_dpan _op_mask_p_c_dp
#define _op_mask_p_can_dpan _op_mask_p_c_dp
#define _op_mask_pas_can_dpan _op_mask_p_c_dp
#define _op_mask_p_caa_dpan _op_mask_p_c_dp
#define _op_mask_pas_caa_dpan _op_mask_p_c_dp
#define _op_mask_pan_caa_dpan _op_mask_p_c_dp

/**
 * @brief Initializes the span-based mask pixel color functions.
 *
 * This function assigns the appropriate pixel processing function
 * (e.g., _op_mask_p_c_dp and its variants) to a global array
 * `op_mask_span_funcs` based on various source, mask, and destination
 * properties. These properties are represented by enums like SP (Solid Pixel),
 * SM_N (No Mask), SC (Solid Color), DP (Destination Pixel), etc.
 * This allows for optimized function dispatch at runtime.
 */
static void
init_mask_pixel_color_span_funcs_c(void)
{
   op_mask_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_mask_p_c_dp;
   op_mask_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_mask_pas_c_dp;
   op_mask_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_mask_pan_c_dp;
   op_mask_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_mask_p_can_dp;
   op_mask_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_mask_pas_can_dp;
   op_mask_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_mask_p_caa_dp;
   op_mask_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_mask_pas_caa_dp;
   op_mask_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_mask_pan_caa_dp;

   op_mask_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_mask_p_c_dpan;
   op_mask_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_mask_pas_c_dpan;
   op_mask_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_mask_pan_c_dpan;
   op_mask_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_mask_p_can_dpan;
   op_mask_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_mask_pas_can_dpan;
   op_mask_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_p_caa_dpan;
   op_mask_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pas_caa_dpan;
   op_mask_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pan_caa_dpan;
}

/**
 * @brief Applies a color mask to a single pixel.
 *
 * This function multiplies the destination pixel by a factor derived from
 * the source pixel's alpha and a given color's alpha. This is a point
 * operation (operates on a single pixel).
 *
 * @param s The source pixel data (DATA32).
 *          Example: `s = 0xFFRRGGBB`.
 * @param m The mask data (DATA8). This parameter is unused.
 * @param c The color to use for masking (DATA32).
 *          Example: `c = 0xAARRGGBB`.
 * @param d Pointer to the destination pixel data (DATA32), which will be modified.
 *          Example: `*d = 0xFFRRGGBB`.
 */
static void
_op_mask_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	// Calculate a combined alpha factor from source alpha and color alpha
	c = 1 + ((((c >> 24) * (s >> 24)) + 255) >> 8);
	*d = MUL_256(c, *d); // Multiply destination pixel by the factor
}

#define _op_mask_pt_pas_c_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_pan_c_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_p_can_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_pas_can_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_p_caa_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_pas_caa_dp _op_mask_pt_p_c_dp
#define _op_mask_pt_pan_caa_dp _op_mask_pt_p_c_dp

#define _op_mask_pt_p_c_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_pas_c_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_pan_c_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_p_can_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_pas_can_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_p_caa_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_pas_caa_dpan _op_mask_pt_p_c_dp
#define _op_mask_pt_pan_caa_dpan _op_mask_pt_p_c_dp

/**
 * @brief Initializes the point-based mask pixel color functions.
 *
 * This function assigns the appropriate single-pixel processing function
 * (e.g., _op_mask_pt_p_c_dp and its variants) to a global array
 * `op_mask_pt_funcs` based on various source, mask, and destination
 * properties. Similar to `init_mask_pixel_color_span_funcs_c`, this
 * allows for optimized function dispatch for single-pixel operations.
 */
static void
init_mask_pixel_color_pt_funcs_c(void)
{
   op_mask_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_mask_pt_p_c_dp;
   op_mask_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_mask_pt_pas_c_dp;
   op_mask_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_mask_pt_pan_c_dp;
   op_mask_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_mask_pt_p_can_dp;
   op_mask_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_mask_pt_pas_can_dp;
   op_mask_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_mask_pt_p_caa_dp;
   op_mask_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_mask_pt_pas_caa_dp;
   op_mask_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_mask_pt_pan_caa_dp;

   op_mask_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_mask_pt_p_c_dpan;
   op_mask_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_mask_pt_pas_c_dpan;
   op_mask_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_mask_pt_pan_c_dpan;
   op_mask_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_mask_pt_p_can_dpan;
   op_mask_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_mask_pt_pas_can_dpan;
   op_mask_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pt_p_caa_dpan;
   op_mask_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pt_pas_caa_dpan;
   op_mask_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pt_pan_caa_dpan;
}
