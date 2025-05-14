/* mask pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Apply a source pixel span to a destination pixel span using MMX.
 *
 * This function processes a line of pixels, blending the source pixels (`s`)
 * onto the destination pixels (`d`). The operation effectively masks the
 * destination with the source.
 *
 * @param s Pointer to the source pixel data array. Each element is a DATA32 pixel.
 * @param m Pointer to the mask data array (unused in this MMX version).
 * @param c Color value (unused in this MMX version).
 * @param d Pointer to the destination pixel data array. Results are written here.
 *          Each element is a DATA32 pixel.
 * @param l The number of pixels to process in the span.
 */
static void
_op_mask_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   MOV_A2R(ALPHA_255, mm5) // Load 255 (opaque alpha) into mm5
   pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking pixels
   for (; d < e; d++) {
	MOV_P2R(*d, mm1, mm0) // Load destination pixel into mm1
	MOV_PA2R(*s, mm2) // Load source pixel (alpha premultiplied) into mm2
	MUL4_SYM_R2R(mm2, mm1, mm5) // Multiply source by destination, effectively blending
	MOV_R2P(mm1, *d, mm0) // Store result back to destination
   }
}

#define _op_mask_pas_dp_mmx _op_mask_p_dp_mmx /**< Alias for MMX span operation with source alpha. */

#define _op_mask_p_dpan_mmx _op_mask_p_dp_mmx /**< Alias for MMX span operation with destination alpha. */
#define _op_mask_pas_dpan_mmx _op_mask_pas_dp_mmx /**< Alias for MMX span operation with source and destination alpha. */

/**
 * @brief Initializes MMX-specific function pointers for mask span operations.
 *
 * This function assigns the MMX-optimized pixel span masking functions
 * to the global function pointer array `op_mask_span_funcs`. This allows
 * the rendering engine to dynamically select the MMX version when available
 * and appropriate for the operation type (e.g., simple copy, alpha source).
 */
static void
init_mask_pixel_span_funcs_mmx(void)
{
   op_mask_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_mask_p_dp_mmx;
   op_mask_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_mask_pas_dp_mmx;

   op_mask_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mask_p_dpan_mmx;
   op_mask_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mask_pas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Apply a source pixel to a destination pixel using MMX (point operation).
 *
 * This function processes a single pixel, blending the source pixel (`s`)
 * onto the destination pixel (`d`). The operation effectively masks the
 * destination with the source.
 *
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this MMX version).
 * @param c Color value (unused in this MMX version).
 * @param d Pointer to the destination pixel data (DATA32). Result is written here.
 */
static void
_op_mask_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	MOV_A2R(ALPHA_255, mm5) // Load 255 (opaque alpha) into mm5
	pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking pixels
	MOV_P2R(*d, mm1, mm0) // Load destination pixel into mm1
	MOV_PA2R(s, mm2) // Load source pixel (alpha premultiplied) into mm2
	MUL4_SYM_R2R(mm2, mm1, mm5) // Multiply source by destination, effectively blending
	MOV_R2P(mm1, *d, mm0) // Store result back to destination
}

#define _op_mask_pt_pas_dp_mmx _op_mask_pt_p_dp_mmx /**< Alias for MMX point operation with source alpha. */

#define _op_mask_pt_p_dpan_mmx _op_mask_pt_p_dp_mmx /**< Alias for MMX point operation with destination alpha. */
#define _op_mask_pt_pas_dpan_mmx _op_mask_pt_pas_dp_mmx /**< Alias for MMX point operation with source and destination alpha. */

/**
 * @brief Initializes MMX-specific function pointers for mask point operations.
 *
 * This function assigns the MMX-optimized single pixel masking functions
 * to the global function pointer array `op_mask_pt_funcs`. This allows
 * the rendering engine to dynamically select the MMX version for point
 * operations when available and appropriate.
 */
static void
init_mask_pixel_pt_funcs_mmx(void)
{
   op_mask_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_mask_pt_p_dp_mmx;
   op_mask_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_mask_pt_pas_dp_mmx;

   op_mask_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mask_pt_p_dpan_mmx;
   op_mask_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mask_pt_pas_dpan_mmx;
}
#endif
