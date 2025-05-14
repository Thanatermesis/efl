/* mul pixel x mask --> dst */

/**
 * @brief Multiplies source pixels by a mask, then blends with destination pixels.
 *
 * This function processes a span of pixels. For each pixel:
 * If mask is 0, destination is unchanged.
 * If mask is 255, destination is updated by MUL4_SYM(source, destination).
 * Otherwise, source is blended with the mask, and the result is then
 * blended with the destination using MUL4_SYM.
 * Alpha components are processed.
 *
 * @param s Pointer to the source pixel data array (e.g., [0xAARRGGBB, ...]).
 * @param m Pointer to the mask data array (e.g., [0x00-0xFF, ...]).
 * @param c Temporary variable for calculations (initial value unused in loop).
 * @param d Pointer to the destination pixel data array, also used for output.
 * @param l Number of pixels to process.
 */
static void
_op_mul_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   while (d < e)
     {
	c = *m;
	switch(c)
	  {
	    case 0:
		break;
	    case 255:
		*d = MUL4_SYM(*s, *d);
		break;
	    default:
		c = ~(*s);
		c = ~MUL_SYM(*m, c);
		*d = MUL4_SYM(c, *d);
		break;
	  }
	m++;  s++;  d++;
     }
}

/**
 * @brief Multiplies source pixels (ignoring source alpha) by a mask, then blends with destination pixels, preserving destination alpha.
 *
 * This function processes a span of pixels. For each pixel:
 * If mask is 0, destination is unchanged.
 * If mask is 255, destination RGB is updated by MUL3_SYM(source_RGB, destination_RGB), destination alpha is preserved.
 * Otherwise, source RGB is blended with the mask, and the result is then
 * blended with the destination RGB using MUL3_SYM. Destination alpha is preserved.
 *
 * @param s Pointer to the source pixel data array. Source alpha is not used in blending.
 * @param m Pointer to the mask data array.
 * @param c Temporary variable for calculations (initial value unused in loop).
 * @param d Pointer to the destination pixel data array, also used for output. Destination alpha is preserved.
 * @param l Number of pixels to process.
 */
static void
_op_mul_pan_mas_dp(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   while (d < e)
     {
	c = *m;
	switch(c)
	  {
	    case 0:
		break;
	    case 255:
		*d = (*d & 0xff000000) + MUL3_SYM(*s, *d);
		break;
	    default:
		c = ~(*s);
		c = ~MUL_SYM(*m, c);
		*d = (*d & 0xff000000) + MUL3_SYM(c, *d);
		break;
	  }
	m++;  s++;  d++;
     }
}

/**
 * @brief Multiplies a single source color by a mask, then blends with destination pixels. Output alpha comes from the (blended) source.
 *
 * This function processes a span of pixels, using a single source color `*s`
 * for all operations. For each pixel in the destination:
 * If mask is 0, destination is unchanged.
 * If mask is 255, destination is updated. Alpha comes from `*s`, RGB from MUL3_SYM(*s, destination_RGB).
 * Otherwise, `*s` is blended with the mask. The resulting color (including its alpha)
 * is then blended with the destination RGB using MUL3_SYM.
 * Note: The source pointer `s` is NOT incremented in the loop, meaning `*s` is constant.
 *
 * @param s Pointer to a single source pixel data (used as a constant color for the span).
 * @param m Pointer to the mask data array.
 * @param c Temporary variable for calculations (initial value unused in loop).
 * @param d Pointer to the destination pixel data array, also used for output.
 * @param l Number of pixels to process.
 */
static void
_op_mul_p_mas_dpan(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   while (d < e)
     {
	c = *m;
	switch(c)
	  {
	    case 0:
		break;
	    case 255:
		*d = (*s & 0xff000000) + MUL3_SYM(*s, *d);
		break;
	    default:
		c = ~(*s);
		c = ~MUL_SYM(*m, c);
		*d = (c & 0xff000000) + MUL3_SYM(c, *d);
		break;
	  }
	m++;  d++;
     }
}

#define _op_mul_pas_mas_dp _op_mul_p_mas_dp

#define _op_mul_pan_mas_dpan _op_mul_p_mas_dpan
#define _op_mul_pas_mas_dpan _op_mul_p_mas_dpan

/**
 * @brief Initializes span operation functions for multiplying pixels with a mask.
 *
 * This function populates the `op_mul_span_funcs` table with pointers
 * to specific implementations of masked pixel multiplication for spans of pixels.
 * The indices (e.g., SP, SM_AS) determine the exact operation variant.
 */
static void
init_mul_pixel_mask_span_funcs_c(void)
{
   op_mul_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_mul_p_mas_dp;
   op_mul_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_mul_pas_mas_dp;
   op_mul_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_mul_pan_mas_dp;

   op_mul_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_p_mas_dpan;
   op_mul_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_pas_mas_dpan;
   op_mul_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_pan_mas_dpan;
}

/**
 * @brief Multiplies a single source pixel by a mask, then blends with a destination pixel.
 *
 * This is a point operation (operates on a single pixel).
 * The source pixel `s` is first blended with the mask `m`.
 * The result of this blend is then multiplied with the destination pixel `*d`
 * using `MUL4_SYM` (presumably a 4-component symmetric multiplication).
 *
 * @param s The source pixel value (e.g., 0xAARRGGBB).
 * @param m The mask value (0-255).
 * @param c Unused parameter.
 * @param d Pointer to the destination pixel, which will be updated.
 */
static void
_op_mul_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
	s = ~s;
	s = ~MUL_SYM(m, s);
	*d = MUL4_SYM(s, *d);
}

#define _op_mul_pt_pas_mas_dp _op_mul_pt_p_mas_dp
#define _op_mul_pt_pan_mas_dp _op_mul_pt_p_mas_dp

#define _op_mul_pt_p_mas_dpan _op_mul_pt_p_mas_dp
#define _op_mul_pt_pas_mas_dpan _op_mul_pt_p_mas_dp
#define _op_mul_pt_pan_mas_dpan _op_mul_pt_p_mas_dp

/**
 * @brief Initializes point operation functions for multiplying a pixel with a mask.
 *
 * This function populates the `op_mul_pt_funcs` table with pointers
 * to specific implementations of masked pixel multiplication for single pixels (points).
 * The indices (e.g., SP, SM_AS) determine the exact operation variant.
 */
static void
init_mul_pixel_mask_pt_funcs_c(void)
{
   op_mul_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_mul_pt_p_mas_dp;
   op_mul_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_mul_pt_pas_mas_dp;
   op_mul_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_mul_pt_pan_mas_dp;

   op_mul_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_pt_p_mas_dpan;
   op_mul_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_pt_pas_mas_dpan;
   op_mul_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_mul_pt_pan_mas_dpan;
}

