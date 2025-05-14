/* mul pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Multiplies a span of source pixels by a color, then multiplies the result with the destination pixels, using MMX.
 *
 * Operation: d[i] = d[i] * (s[i] * c) for each pixel i in the span.
 * This function processes 'l' pixels.
 *
 * @param s Pointer to the source pixel array (e.g., [0xAARRGGBB, ...]).
 * @param m Pointer to mask data (unused in this MMX implementation).
 * @param c The color value (e.g., 0xAARRGGBB) to multiply with source pixels.
 * @param d Pointer to the destination pixel array, also used as an operand.
 * @param l The number of pixels to process.
 */
static void
_op_mul_p_c_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l; // End pointer for the loop
   MOV_A2R(ALPHA_255, mm5) // Load 255 into alpha components of mm5 for scaling
   pxor_r2r(mm0, mm0);     // Zero out mm0, used for unpacking bytes to words
   MOV_P2R(c, mm2, mm0)     // Unpack color 'c' into MMX register mm2

   while (d < e)
     {
	// Load source pixel s[i] into mm3
	MOV_P2R(*s, mm3, mm0)
	// Multiply s[i] by color c: mm3 = (s[i] * c) (component-wise, scaled)
	MUL4_SYM_R2R(mm2, mm3, mm5)
	// Load destination pixel d[i] into mm1
	MOV_P2R(*d, mm1, mm0)
	// Multiply d_orig[i] by (s[i] * c): mm1 = d_orig[i] * mm3 (component-wise, scaled)
	MUL4_SYM_R2R(mm3, mm1, mm5)
	// Store the result back to d[i]
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

#define _op_mul_pas_c_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pan_c_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_p_can_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pas_can_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pan_can_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_p_caa_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pas_caa_dp_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pan_caa_dp_mmx _op_mul_p_c_dp_mmx

#define _op_mul_p_c_dpan_mmx _op_mul_p_c_dp_mmx
#define _op_mul_pan_c_dpan_mmx _op_mul_pan_c_dp_mmx
#define _op_mul_pas_c_dpan_mmx _op_mul_pas_c_dp_mmx
#define _op_mul_p_can_dpan_mmx _op_mul_p_can_dp_mmx
#define _op_mul_pan_can_dpan_mmx _op_mul_pan_can_dp_mmx
#define _op_mul_pas_can_dpan_mmx _op_mul_pas_can_dp_mmx
#define _op_mul_p_caa_dpan_mmx _op_mul_p_caa_dp_mmx
#define _op_mul_pan_caa_dpan_mmx _op_mul_pan_caa_dp_mmx
#define _op_mul_pas_caa_dpan_mmx _op_mul_pas_caa_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for span-based pixel multiplication operations.
 *
 * This function populates the `op_mul_span_funcs` table with MMX implementations
 * for various combinations of source, mask, color, and destination properties.
 * Many combinations alias to the same core MMX function `_op_mul_p_c_dp_mmx`.
 */
static void
init_mul_pixel_color_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_mul_p_c_dp_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_mul_pas_c_dp_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_mul_pan_c_dp_mmx;
   op_mul_span_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_p_can_dp_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pas_can_dp_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pan_can_dp_mmx;
   op_mul_span_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_p_caa_dp_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pas_caa_dp_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pan_caa_dp_mmx;

   op_mul_span_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_p_c_dpan_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pas_c_dpan_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pan_c_dpan_mmx;
   op_mul_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_p_can_dpan_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pas_can_dpan_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pan_can_dpan_mmx;
   op_mul_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_p_caa_dpan_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pas_caa_dpan_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pan_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Multiplies a single source pixel value by a color, then multiplies the result with a destination pixel, using MMX.
 *
 * Operation: *d = *d * (s * c).
 * This is a point operation (processes a single pixel).
 *
 * @param s The source pixel value (e.g., 0xAARRGGBB).
 * @param m Mask data (unused in this MMX implementation).
 * @param c The color value (e.g., 0xAARRGGBB) to multiply with the source pixel.
 * @param d Pointer to the destination pixel, also used as an operand.
 */
static void
_op_mul_pt_p_c_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	MOV_A2R(ALPHA_255, mm5) // Load 255 into alpha components of mm5 for scaling
	pxor_r2r(mm0, mm0);     // Zero out mm0, used for unpacking bytes to words
	MOV_P2R(c, mm2, mm0)     // Unpack color 'c' into MMX register mm2
	MOV_P2R(s, mm3, mm0)     // Unpack source pixel 's' into MMX register mm3
	// Multiply s by color c: mm3 = (s * c) (component-wise, scaled)
	MUL4_SYM_R2R(mm2, mm3, mm5)
	// Load destination pixel *d into mm1
	MOV_P2R(*d, mm1, mm0)
	// Multiply d_orig by (s * c): mm1 = d_orig * mm3 (component-wise, scaled)
	MUL4_SYM_R2R(mm3, mm1, mm5)
	// Store the result back to *d
	MOV_R2P(mm1, *d, mm0)
}

#define _op_mul_pt_pas_c_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pan_c_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_p_can_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pas_can_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pan_can_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_p_caa_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pas_caa_dp_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pan_caa_dp_mmx _op_mul_pt_p_c_dp_mmx

#define _op_mul_pt_p_c_dpan_mmx _op_mul_pt_p_c_dp_mmx
#define _op_mul_pt_pan_c_dpan_mmx _op_mul_pt_pan_c_dp_mmx
#define _op_mul_pt_pas_c_dpan_mmx _op_mul_pt_pas_c_dp_mmx
#define _op_mul_pt_p_can_dpan_mmx _op_mul_pt_p_can_dp_mmx
#define _op_mul_pt_pan_can_dpan_mmx _op_mul_pt_pan_can_dp_mmx
#define _op_mul_pt_pas_can_dpan_mmx _op_mul_pt_pas_can_dp_mmx
#define _op_mul_pt_p_caa_dpan_mmx _op_mul_pt_p_caa_dp_mmx
#define _op_mul_pt_pan_caa_dpan_mmx _op_mul_pt_pan_caa_dp_mmx
#define _op_mul_pt_pas_caa_dpan_mmx _op_mul_pt_pas_caa_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for point-based (single pixel) multiplication operations.
 *
 * This function populates the `op_mul_pt_funcs` table with MMX implementations
 * for various combinations of source, mask, color, and destination properties.
 * Many combinations alias to the same core MMX function `_op_mul_pt_p_c_dp_mmx`.
 */
static void
init_mul_pixel_color_pt_funcs_mmx(void)
{
   op_mul_pt_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_mul_pt_p_c_dp_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_mul_pt_pas_c_dp_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_mul_pt_pan_c_dp_mmx;
   op_mul_pt_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pt_p_can_dp_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pt_pas_can_dp_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pt_pan_can_dp_mmx;
   op_mul_pt_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pt_p_caa_dp_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pt_pas_caa_dp_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pt_pan_caa_dp_mmx;

   op_mul_pt_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pt_p_c_dpan_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pt_pas_c_dpan_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pt_pan_c_dpan_mmx;
   op_mul_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_p_can_dpan_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_pas_can_dpan_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_pan_can_dpan_mmx;
   op_mul_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_p_caa_dpan_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_pas_caa_dpan_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_pan_caa_dpan_mmx;
}
#endif
