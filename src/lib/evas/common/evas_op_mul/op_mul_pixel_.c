/* mul pixel --> dst */

/**
 * @brief Multiplies a span of pixels with destination pixels.
 *
 * This function iterates over a line of pixels, multiplying each source
 * pixel with the corresponding destination pixel and storing the result
 * in the destination.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (array of DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l Length of the pixel span to process.
 */
static void
_op_mul_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   for (; d < e; d++, s++) {
      *d = MUL4_SYM(*s, *d);
   }
}

#define _op_mul_pas_dp _op_mul_p_dp
#define _op_mul_pan_dp _op_mul_p_dp

#define _op_mul_p_dpan _op_mul_p_dp
#define _op_mul_pas_dpan _op_mul_pas_dp
#define _op_mul_pan_dpan _op_mul_pan_dp

/**
 * @brief Initializes the span-based multiplication functions for C CPU.
 *
 * This function assigns the appropriate C-specific pixel multiplication
 * functions to the global function pointer array `op_mul_span_funcs`.
 * It covers different source and destination pixel properties (e.g., alpha).
 */
static void
init_mul_pixel_span_funcs_c(void)
{
   op_mul_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_mul_p_dp;
   op_mul_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_mul_pas_dp;
   op_mul_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_mul_pan_dp;

   op_mul_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_p_dpan;
   op_mul_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_pas_dpan;
   op_mul_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_pan_dpan;
}

/**
 * @brief Multiplies a single source pixel with a destination pixel.
 *
 * This function takes a single source pixel `s` and multiplies it with
 * the destination pixel pointed to by `d`. The result is stored back in `*d`.
 *
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel (DATA32).
 */
static void
_op_mul_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
      *d = MUL4_SYM(s, *d);
}

#define _op_mul_pt_pas_dp _op_mul_pt_p_dp
#define _op_mul_pt_pan_dp _op_mul_pt_p_dp

#define _op_mul_pt_p_dpan _op_mul_pt_p_dp
#define _op_mul_pt_pan_dpan _op_mul_pt_pan_dp
#define _op_mul_pt_pas_dpan _op_mul_pt_pas_dp

/**
 * @brief Initializes the point-based multiplication functions for C CPU.
 *
 * This function assigns the appropriate C-specific single pixel multiplication
 * functions to the global function pointer array `op_mul_pt_funcs`.
 * It covers different source and destination pixel properties.
 */
static void
init_mul_pixel_pt_funcs_c(void)
{
   op_mul_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_mul_pt_p_dp;
   op_mul_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_mul_pt_pas_dp;
   op_mul_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_mul_pt_pan_dp;

   op_mul_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_pt_p_dpan;
   op_mul_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_pt_pas_dpan;
   op_mul_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_mul_pt_pan_dpan;
}
