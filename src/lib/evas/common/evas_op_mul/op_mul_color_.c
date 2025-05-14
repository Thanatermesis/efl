/* mul color --> dst */

/**
 * @brief Multiplies each destination pixel by a color value.
 * @param s Source data (unused).
 * @param m Mask data (unused).
 * @param c The color value (DATA32) to multiply with.
 * @param d Pointer to the destination pixel data (DATA32 array).
 *          Each element is a pixel, e.g., 0xAARRGGBB.
 * @param l The number of pixels to process.
 */
static void
_op_mul_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   for (; d < e; d++) {
      *d = MUL4_SYM(c, *d);
   }
}

/**
 * @brief Multiplies each destination pixel by a color value, using the color's alpha.
 * This function scales the destination pixel by the alpha component of the color `c`.
 * @param s Source data (unused).
 * @param m Mask data (unused).
 * @param c The color value (DATA32) whose alpha component is used for multiplication.
 *          The alpha is extracted and normalized (1 + (alpha_value)).
 * @param d Pointer to the destination pixel data (DATA32 array).
 *          Each element is a pixel, e.g., 0xAARRGGBB.
 * @param l The number of pixels to process.
 */
static void
_op_mul_caa_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   c = 1 + (c >> 24); // Extract and normalize alpha: (alpha + 1)
   for (; d < e; d++) {
      *d = MUL_256(c, *d);
   }
}

#define _op_mul_can_dp _op_mul_c_dp

#define _op_mul_c_dpan _op_mul_c_dp
#define _op_mul_can_dpan _op_mul_can_dp
#define _op_mul_caa_dpan _op_mul_caa_dp

/**
 * @brief Initializes the span-based multiplication function pointers for color operations.
 * This function assigns the appropriate multiplication functions (e.g., _op_mul_c_dp)
 * to the op_mul_span_funcs array based on various operation flags
 * (source/mask presence, color alpha type, destination alpha presence).
 * It covers variants for normal (SC), alpha-no-alpha (SC_AN), and alpha-alpha (SC_AA)
 * color modes, for both destination-present (DP) and destination-present-no-alpha (DP_AN) cases.
 */
static void
init_mul_color_span_funcs_c(void)
{
   op_mul_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_mul_c_dp;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_mul_can_dp;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_mul_caa_dp;

   op_mul_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_mul_c_dpan;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_can_dpan;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_caa_dpan;
}

/**
 * @brief Multiplies a single destination pixel by a color value.
 * @param s Source data (unused).
 * @param m Mask data (unused).
 * @param c The color value (DATA32) to multiply with.
 * @param d Pointer to the single destination pixel (DATA32).
 *          The pixel is modified in place, e.g., *d = 0xAARRGGBB.
 */
static void
_op_mul_pt_c_dp(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	*d = MUL4_SYM(c, *d);
}

#define _op_mul_pt_can_dp _op_mul_pt_c_dp
#define _op_mul_pt_caa_dp _op_mul_pt_c_dp

#define _op_mul_pt_c_dpan _op_mul_pt_c_dp
#define _op_mul_pt_can_dpan _op_mul_pt_can_dp
#define _op_mul_pt_caa_dpan _op_mul_pt_caa_dp

/**
 * @brief Initializes the point-based multiplication function pointers for color operations.
 * This function assigns the appropriate single-pixel multiplication functions (e.g., _op_mul_pt_c_dp)
 * to the op_mul_pt_funcs array based on various operation flags.
 * Similar to init_mul_color_span_funcs_c, it covers different color modes and
 * destination alpha scenarios for single point operations.
 */
static void
init_mul_color_pt_funcs_c(void)
{
   op_mul_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_mul_pt_c_dp;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_C] = _op_mul_pt_can_dp;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_mul_pt_caa_dp;

   op_mul_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_mul_pt_c_dpan;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pt_can_dpan;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pt_caa_dpan;
}
