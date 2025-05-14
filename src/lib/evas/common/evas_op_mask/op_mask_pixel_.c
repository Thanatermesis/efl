/* mask pixel --> dst */

/**
 * @brief Masks a span of pixels from source to destination.
 *
 * This function applies a mask operation to a line of pixels. The operation
 * multiplies the destination pixel by the alpha component of the source pixel.
 * The source color, mask, and color value parameters are not directly used in
 * this specific implementation, but are part of a generic function signature.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Each DATA32 represents a pixel, e.g., 0xAARRGGBB.
 *          The alpha component (*s >> 24) is used for multiplication.
 * @param m Pointer to the mask data (array of DATA8, unused in this function).
 * @param c Color value (DATA32, unused in this function).
 * @param d Pointer to the destination pixel data (array of DATA32).
 *          This is where the result of the operation is stored.
 * @param l Length of the pixel span to process.
 */
static void
_op_mask_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   for (; d < e; d++, s++) {
	*d = MUL_SYM(*s >> 24, *d);
   }
}

/** @brief Alias for _op_mask_p_dp, typically for 'source alpha solid' cases. */
#define _op_mask_pas_dp _op_mask_p_dp

/** @brief Alias for _op_mask_p_dp, typically for 'destination alpha no alpha' cases. */
#define _op_mask_p_dpan _op_mask_p_dp
/** @brief Alias for _op_mask_pas_dp, typically for 'source alpha solid, destination alpha no alpha' cases. */
#define _op_mask_pas_dpan _op_mask_pas_dp

/**
 * @brief Initializes C-specific span masking function pointers.
 *
 * This function assigns the C implementation of span masking operations
 * to the global function pointer array `op_mask_span_funcs`.
 * It sets up handlers for different combinations of source, mask,
 * scale, and destination properties.
 */
static void
init_mask_pixel_span_funcs_c(void)
{
   op_mask_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_mask_p_dp;
   op_mask_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_mask_pas_dp;

   op_mask_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_mask_p_dpan;
   op_mask_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_mask_pas_dpan;
}

/**
 * @brief Masks a single pixel from source to destination.
 *
 * This function applies a mask operation to a single pixel. The operation
 * multiplies the destination pixel by the alpha component of the source pixel.
 * The mask and color value parameters are not directly used in this specific
 * implementation but are part of a generic function signature.
 *
 * @param s Source pixel data (DATA32), e.g., 0xAARRGGBB.
 *          The alpha component (s >> 24) is used for multiplication.
 * @param m Mask data (DATA8, unused in this function).
 * @param c Color value (DATA32, unused in this function).
 * @param d Pointer to the destination pixel data (DATA32).
 *          This is where the result of the operation is stored.
 */
static void
_op_mask_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	*d = MUL_SYM(s >> 24, *d);
}

/** @brief Alias for _op_mask_pt_p_dp, typically for 'source alpha solid' point cases. */
#define _op_mask_pt_pas_dp _op_mask_pt_p_dp

/** @brief Alias for _op_mask_pt_p_dp, typically for 'destination alpha no alpha' point cases. */
#define _op_mask_pt_p_dpan _op_mask_pt_p_dp
/** @brief Alias for _op_mask_pt_pas_dp, typically for 'source alpha solid, destination alpha no alpha' point cases. */
#define _op_mask_pt_pas_dpan _op_mask_pt_pas_dp

/**
 * @brief Initializes C-specific point masking function pointers.
 *
 * This function assigns the C implementation of point masking operations
 * to the global function pointer array `op_mask_pt_funcs`.
 * It sets up handlers for different combinations of source, mask,
 * scale, and destination properties for single pixel operations.
 */
static void
init_mask_pixel_pt_funcs_c(void)
{
   op_mask_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_mask_pt_p_dp;
   op_mask_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_mask_pt_pas_dp;

   op_mask_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_mask_pt_p_dpan;
   op_mask_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_mask_pt_pas_dpan;
}
