/* mask pixel x mask --> dst */

/**
 * @brief Applies a mask to a span of pixels.
 *
 * This function processes a line of pixels, applying a mask to each.
 * The operation is d = (s_alpha * m) * d.
 * If the mask value is 0, the destination pixel is unchanged.
 * If the mask value is 255, the destination pixel is multiplied by the source alpha.
 * Otherwise, a blended alpha is calculated and applied.
 *
 * @param s Pointer to the source pixel data (DATA32 array). Each element is an ARGB pixel.
 * @param m Pointer to the mask data (DATA8 array). Each element is an alpha value (0-255).
 * @param c Unused color value.
 * @param d Pointer to the destination pixel data (DATA32 array), which is modified in place.
 * @param l Length of the pixel span to process.
 */
static void
_op_mask_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		*d = MUL_SYM(*s >> 24, *d);
		break;
	    default:
		l = 256 - (((256 - (*s >> 24)) * l) >> 8);
		*d = MUL_256(l, *d);
		break;
	  }
	m++;  s++;  d++;
     }
}

#define _op_mask_pas_mas_dp _op_mask_p_mas_dp

#define _op_mask_p_mas_dpan _op_mask_p_mas_dp
#define _op_mask_pas_mas_dpan _op_mask_pas_mas_dp

/**
 * @brief Initializes function pointers for mask pixel span operations.
 *
 * This function assigns the appropriate C-specific implementation for
 * various combinations of source, mask, and destination properties
 * for span operations.
 */
static void
init_mask_pixel_mask_span_funcs_c(void)
{
   op_mask_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_mask_p_mas_dp;
   op_mask_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_mask_pas_mas_dp;

   op_mask_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_mask_p_mas_dpan;
   op_mask_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_mask_pas_mas_dpan;
}

/**
 * @brief Applies a mask to a single pixel.
 *
 * This function processes a single pixel, applying a mask value.
 * The operation calculates a new alpha based on the source pixel's alpha and the mask value,
 * then multiplies the destination pixel by this new alpha.
 * s_new_alpha = 256 - (((256 - (s_alpha)) * m) >> 8);
 * d = (s_new_alpha * d) / 256;
 *
 * @param s Source pixel data (DATA32, ARGB).
 * @param m Mask data (DATA8, alpha value 0-255).
 * @param c Unused color value.
 * @param d Pointer to the destination pixel data (DATA32), which is modified in place.
 */
static void
_op_mask_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
	s = 256 - (((256 - (s >> 24)) * m) >> 8);
	*d = MUL_256(s, *d);
}

#define _op_mask_pt_pas_mas_dp _op_mask_pt_p_mas_dp

#define _op_mask_pt_p_mas_dpan _op_mask_pt_p_mas_dp
#define _op_mask_pt_pas_mas_dpan _op_mask_pt_pas_mas_dp

/**
 * @brief Initializes function pointers for mask pixel point operations.
 *
 * This function assigns the appropriate C-specific implementation for
 * various combinations of source, mask, and destination properties
 * for point (single pixel) operations.
 */
static void
init_mask_pixel_mask_pt_funcs_c(void)
{
   op_mask_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_mask_pt_p_mas_dp;
   op_mask_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_mask_pt_pas_mas_dp;

   op_mask_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_mask_pt_p_mas_dpan;
   op_mask_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_mask_pt_pas_mas_dpan;
}
