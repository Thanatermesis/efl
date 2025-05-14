/* mask color --> dst */

/**
 * @brief Masks a color onto a destination buffer.
 *
 * This function applies a color `c` to a destination buffer `d` of length `l`.
 * The source `s` and mask `m` parameters are unused in this specific implementation.
 * The operation effectively scales the destination pixels by the alpha component of `c`.
 *
 * @param s Unused source data pointer.
 * @param m Unused mask data pointer.
 * @param c The color to apply (typically in ARGB format). The alpha component is used for masking.
 * @param d Pointer to the destination buffer (array of DATA32).
 * @param l The number of pixels in the destination buffer to process.
 */
static void
_op_mask_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   c = 1 + (c >> 24); // Extract alpha component and prepare for MUL_256
   for (; d < e; d++) {
	*d = MUL_256(c, *d);
   }
}

#define _op_mask_caa_dp _op_mask_c_dp

#define _op_mask_c_dpan _op_mask_c_dp
#define _op_mask_caa_dpan _op_mask_caa_dp

/**
 * @brief Initializes the span operation function pointers for mask color operations.
 *
 * This function assigns the appropriate C-specific implementation (_op_mask_c_dp and its aliases)
 * to the global function pointer array `op_mask_span_funcs`. These pointers are used
 * for span-based masking operations with different source, mask, and destination properties.
 * - SP_N: Source Pixels are Not present (unused).
 * - SM_N: Source Mask is Not present (unused).
 * - SC: Source Color is present.
 * - SC_AA: Source Color with Alpha is present.
 * - DP: Destination Pixels are present.
 * - DP_AN: Destination Pixels with Alpha are Not present (or ignored).
 * - CPU_C: C language implementation.
 */
static void
init_mask_color_span_funcs_c(void)
{
   op_mask_span_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_mask_c_dp;
   op_mask_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_mask_caa_dp;

   op_mask_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_mask_c_dpan;
   op_mask_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_caa_dpan;
}

/**
 * @brief Masks a color onto a single destination pixel.
 *
 * This function applies a color `c` to a single destination pixel `d`.
 * The source `s` and mask `m` parameters are unused in this specific implementation.
 * The operation multiplies the destination pixel by the alpha component of `c` symmetrically.
 *
 * @param s Unused source data.
 * @param m Unused mask data.
 * @param c The color to apply (typically in ARGB format). The alpha component is used for masking.
 * @param d Pointer to the destination pixel.
 */
static void
_op_mask_pt_c_dp(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	*d = MUL_SYM(c >> 24, *d); // Apply color using symmetric multiplication with alpha
}

#define _op_mask_pt_caa_dp _op_mask_pt_c_dp

#define _op_mask_pt_c_dpan _op_mask_pt_c_dp
#define _op_mask_pt_caa_dpan _op_mask_pt_caa_dp

/**
 * @brief Initializes the point operation function pointers for mask color operations.
 *
 * This function assigns the appropriate C-specific implementation (_op_mask_pt_c_dp and its aliases)
 * to the global function pointer array `op_mask_pt_funcs`. These pointers are used
 * for point-based (single pixel) masking operations with different source, mask, and destination properties.
 * - SP_N: Source Pixels are Not present (unused).
 * - SM_N: Source Mask is Not present (unused).
 * - SC: Source Color is present.
 * - SC_AA: Source Color with Alpha is present.
 * - DP: Destination Pixels are present.
 * - DP_AN: Destination Pixels with Alpha are Not present (or ignored).
 * - CPU_C: C language implementation.
 */
static void
init_mask_color_pt_funcs_c(void)
{
   op_mask_pt_funcs[SP_N][SM_N][SC][DP][CPU_C] = _op_mask_pt_c_dp;
   op_mask_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_C] = _op_mask_pt_caa_dp;

   op_mask_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_C] = _op_mask_pt_c_dpan;
   op_mask_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_C] = _op_mask_pt_caa_dpan;
}

