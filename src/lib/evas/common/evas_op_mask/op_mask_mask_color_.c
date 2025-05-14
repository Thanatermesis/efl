/* mask mask x color -> dst */

/**
 * @brief Apply a color to a destination buffer, modulated by a mask.
 *
 * This function iterates over a span of pixels. For each pixel, it uses
 * the mask value to determine how much of the input color `c` to blend
 * with the destination pixel `d`. The source data `s` is unused.
 *
 * @param s Pointer to the source data (unused).
 * @param m Pointer to the mask data (array of DATA8). Each value is 0-255.
 * @param c The color to apply (DATA32). The alpha channel of this color is
 *          used to modulate the effect.
 * @param d Pointer to the destination buffer (array of DATA32).
 *          Example: `d` could be `{0xAAFF00FF, 0xBB00FF00, ...}` where each
 *          element is an ARGB pixel.
 * @param l The number of pixels to process.
 */
static void
_op_mask_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   // Extract and pre-adjust alpha from color c for efficiency in the loop.
   // c becomes a multiplier based on its original alpha.
   c = 1 + (c >> 24);
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		*d = MUL_256(c, *d);
		break;
	    default:
		l = 256 - (((257 - c) * l) >> 8);
		*d = MUL_256(l, *d);
		break;
	  }
	m++;  d++;
     }
}

#define _op_mask_mas_caa_dp _op_mask_mas_c_dp

#define _op_mask_mas_c_dpan _op_mask_mas_c_dp
#define _op_mask_mas_caa_dpan _op_mask_mas_caa_dp

/**
 * @brief Initializes the span processing functions for mask operations.
 *
 * This function assigns the appropriate span processing functions
 * (like _op_mask_mas_c_dp) to a global table `op_mask_span_funcs`.
 * These functions are selected based on various parameters like source,
 * mask, and destination properties.
 */
static void
init_mask_mask_color_span_funcs_c(void)
{
   op_mask_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_mask_mas_c_dp;
   op_mask_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_mask_mas_caa_dp;

   op_mask_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_mask_mas_c_dpan;
   op_mask_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_mask_mas_caa_dpan;
}

/**
 * @brief Apply a color to a single destination pixel, modulated by a mask value.
 *
 * This function processes a single pixel. It uses the mask value `m` and
 * the alpha channel of color `c` to determine the blending factor.
 * The source data `s` is unused.
 *
 * @param s The source data (unused).
 * @param m The mask value (DATA8, 0-255).
 * @param c The color to apply (DATA32). The alpha channel of this color is
 *          used. Example: `c` could be `0x80FF00FF` (50% transparent red).
 * @param d Pointer to the destination pixel (DATA32).
 *          Example: `*d` could be `0xFF00FF00` (opaque green).
 */
static void
_op_mask_pt_mas_c_dp(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
	// Calculate the blending factor.
	// (c >> 24) extracts the alpha from the color c.
	// The formula effectively combines the mask `m` and color's alpha.
	c = 256 - (((256 - (c >> 24)) * m) >> 8);
	// Apply the blended color to the destination.
	*d = MUL_256(c, *d);
}

#define _op_mask_pt_mas_caa_dp _op_mask_pt_mas_c_dp

#define _op_mask_pt_mas_c_dpan _op_mask_pt_mas_c_dp
#define _op_mask_pt_mas_caa_dpan _op_mask_pt_mas_caa_dp

/**
 * @brief Initializes the point processing functions for mask operations.
 *
 * This function assigns the appropriate point processing functions
 * (like _op_mask_pt_mas_c_dp) to a global table `op_mask_pt_funcs`.
 * These functions are used for single pixel operations and are selected
 * based on various parameters.
 */
static void
init_mask_mask_color_pt_funcs_c(void)
{
   op_mask_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_mask_pt_mas_c_dp;
   op_mask_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_mask_pt_mas_caa_dp;

   op_mask_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_mask_pt_mas_c_dpan;
   op_mask_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_mask_pt_mas_caa_dpan;
}
