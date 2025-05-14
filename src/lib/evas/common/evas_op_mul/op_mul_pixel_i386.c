/* mul pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Multiplies a span of source pixels by destination pixels using MMX.
 *
 * This function performs pixel-wise multiplication (s * d) / 255 for each
 * color channel, effectively blending the source pixels onto the destination
 * pixels. The alpha channel is also multiplied.
 *
 * @param s Pointer to the source pixel array (DATA32 per pixel, e.g., ARGB).
 *          Example: s points to [0xA1R1G1B1, 0xA2R2G2B2, ...]
 * @param m Unused mask pointer.
 * @param c Unused color value.
 * @param d Pointer to the destination pixel array (DATA32 per pixel, e.g., ARGB).
 *          This array is read from and written to.
 *          Example: d points to [0xA3R3G3B3, 0xA4R4G4B4, ...]
 * @param l The number of pixels in the span to process.
 */
static void
_op_mul_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = s + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_255, mm5)
   for (; s < e; s++, d++) {
	MOV_P2R(*d, mm1, mm0)
	MOV_P2R(*s, mm2, mm0)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
   }
}

#define _op_mul_pas_dp_mmx _op_mul_p_dp_mmx
#define _op_mul_pan_dp_mmx _op_mul_p_dp_mmx

#define _op_mul_p_dpan_mmx _op_mul_p_dp_mmx
#define _op_mul_pan_dpan_mmx _op_mul_pan_dp_mmx
#define _op_mul_pas_dpan_mmx _op_mul_pas_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for span multiplication operations.
 *
 * This function populates the `op_mul_span_funcs` array with pointers to
 * MMX-optimized functions for various source, mask, and destination pixel
 * format combinations. This allows for dynamic dispatch to the most
 * appropriate MMX routine at runtime.
 */
static void
init_mul_pixel_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_mul_p_dp_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_mul_pan_dp_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_mul_pas_dp_mmx;

   op_mul_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_p_dpan_mmx;
   op_mul_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_pan_dpan_mmx;
   op_mul_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_pas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Multiplies a single source pixel by a destination pixel using MMX.
 *
 * This function performs pixel-wise multiplication (s * d) / 255 for each
 * color channel of a single pixel, effectively blending the source pixel
 * onto the destination pixel. The alpha channel is also multiplied.
 *
 * @param s The source pixel value (DATA32, e.g., 0xAARRGGBB).
 * @param m Unused mask value.
 * @param c Unused color value.
 * @param d Pointer to the destination pixel (DATA32, e.g., 0xAARRGGBB).
 *          This pixel is read from and written to.
 */
static void
_op_mul_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_255, mm5)
	MOV_P2R(*d, mm1, mm0)
	MOV_P2R(s, mm2, mm0)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
}

#define _op_mul_pt_pan_dp_mmx _op_mul_pt_p_dp_mmx
#define _op_mul_pt_pas_dp_mmx _op_mul_pt_p_dp_mmx

#define _op_mul_pt_p_dpan_mmx _op_mul_pt_p_dp_mmx
#define _op_mul_pt_pan_dpan_mmx _op_mul_pt_pan_dp_mmx
#define _op_mul_pt_pas_dpan_mmx _op_mul_pt_pas_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for single-point multiplication operations.
 *
 * This function populates the `op_mul_pt_funcs` array with pointers to
 * MMX-optimized functions for various source, mask, and destination pixel
 * format combinations for single pixel operations. This allows for dynamic
 * dispatch to the most appropriate MMX routine at runtime.
 */
static void
init_mul_pixel_pt_funcs_mmx(void)
{
   op_mul_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_mul_pt_p_dp_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_mul_pt_pan_dp_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_mul_pt_pas_dp_mmx;

   op_mul_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_pt_p_dpan_mmx;
   op_mul_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_pt_pan_dpan_mmx;
   op_mul_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_mul_pt_pas_dpan_mmx;
}
#endif
