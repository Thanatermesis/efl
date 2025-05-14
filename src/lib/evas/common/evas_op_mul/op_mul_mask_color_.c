/**
 * @file op_mul_mask_color_.c
 * @brief Operations for multiplying a color by a mask and then by destination.
 *
 * These functions implement pixel blending operations where a source color
 * is first modulated by a mask, and the result is then multiplied with the
 * destination pixel. This is typically used for effects like tinting or
 * applying a color filter through a mask.
 */

/* mul mask x color -> dst */

/**
 * @brief Multiplies a color by a mask and then by a span of destination pixels.
 *
 * This function iterates over a span of `l` pixels. For each pixel,
 * it takes a mask value `m`, a constant color `c`, and a destination
 * pixel `d`. The operation performed is `*d = (*d * (c * (*m / 255))) / 255`.
 *
 * The source `s` is unused in this specific implementation.
 *
 * @param s Pointer to source pixel data (unused).
 * @param m Pointer to mask data (array of DATA8, one per pixel).
 *          Each value is in the range [0, 255].
 * @param c The color to multiply (DATA32, typically ARGB).
 * @param d Pointer to destination pixel data (array of DATA32, one per pixel).
 *          These pixels are read and then overwritten.
 * @param l The number of pixels to process.
 */
static void
_op_mul_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l, nc = ~c;
   while (d < e)
     {
	DATA32 a = *m;
	switch(a)
	  {
	    case 0:
		break;
	    case 255:
		*d = MUL4_SYM(c, *d);
		break;
	    default:
		a = ~MUL_SYM(a, nc);
		*d = MUL4_SYM(a, *d);
		break;
	  }
	m++;  d++;
     }
}

#define _op_mul_mas_can_dp _op_mul_mas_c_dp
#define _op_mul_mas_caa_dp _op_mul_mas_c_dp

#define _op_mul_mas_c_dpan _op_mul_mas_c_dp
#define _op_mul_mas_can_dpan _op_mul_mas_can_dp
#define _op_mul_mas_caa_dpan _op_mul_mas_caa_dp

/**
 * @brief Initializes the span operation function pointers for mul_mask_color.
 *
 * This function assigns the `_op_mul_mas_c_dp` function (and its aliases)
 * to the appropriate entries in the `op_mul_span_funcs` table. This table
 * is used to dispatch to the correct span processing function based on
 * various parameters like source/mask/destination properties and CPU capabilities.
 *
 * The indices used (SP_N, SM_AS, SC, DP, CPU_C, etc.) represent different
 * configurations for the operation (e.g., source format, mask type,
 * destination alpha handling).
 */
static void
init_mul_mask_color_span_funcs_c(void)
{
   op_mul_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_mul_mas_c_dp;
   op_mul_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_mul_mas_can_dp;
   op_mul_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_mul_mas_caa_dp;

   op_mul_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_mul_mas_c_dpan;
   op_mul_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_mul_mas_can_dpan;
   op_mul_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_mul_mas_caa_dpan;
}

/**
 * @brief Multiplies a color by a mask and then by a single destination pixel.
 *
 * This function processes a single pixel. It takes a mask value `m`,
 * a constant color `c`, and a destination pixel `d`.
 * The operation performed is `*d = (*d * (c * (m / 255))) / 255`.
 * The specific implementation uses bitwise NOT and MUL_SYM/MUL4_SYM macros
 * which likely handle component-wise multiplication for ARGB pixels.
 *
 * The source `s` is unused in this specific implementation.
 *
 * @param s Source pixel data (unused).
 * @param m Mask value (DATA8, range [0, 255]).
 * @param c The color to multiply (DATA32, typically ARGB).
 * @param d Pointer to the destination pixel (DATA32).
 *          This pixel is read and then overwritten.
 */
static void
_op_mul_pt_mas_c_dp(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
	c = ~c;
	c = ~MUL_SYM(m, c);
	*d = MUL4_SYM(c, *d);
}

#define _op_mul_pt_mas_can_dp _op_mul_pt_mas_c_dp
#define _op_mul_pt_mas_caa_dp _op_mul_pt_mas_c_dp

#define _op_mul_pt_mas_c_dpan _op_mul_pt_mas_c_dp
#define _op_mul_pt_mas_can_dpan _op_mul_pt_mas_can_dp
#define _op_mul_pt_mas_caa_dpan _op_mul_pt_mas_caa_dp

/**
 * @brief Initializes the point operation function pointers for mul_mask_color.
 *
 * This function assigns the `_op_mul_pt_mas_c_dp` function (and its aliases)
 * to the appropriate entries in the `op_mul_pt_funcs` table. This table
 * is used to dispatch to the correct single-pixel processing function based
 * on various parameters.
 *
 * The indices used (SP_N, SM_AS, SC, DP, CPU_C, etc.) represent different
 * configurations for the operation.
 */
static void
init_mul_mask_color_pt_funcs_c(void)
{
   op_mul_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_mul_pt_mas_c_dp;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_mul_pt_mas_can_dp;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_mul_pt_mas_caa_dp;

   op_mul_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_mul_pt_mas_c_dpan;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_mul_pt_mas_can_dpan;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_mul_pt_mas_caa_dpan;
}
