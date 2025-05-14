/* mul pixel x mask --> dst */

#ifdef BUILD_MMX
/**
 * @brief Multiplies source pixels by a mask and blends with destination pixels using MMX.
 *
 * This function processes a span of pixels. For each pixel, it applies a mask
 * value. If the mask is 0, the destination pixel is unchanged. If the mask is
 * 255, the source pixel is blended with the destination. For intermediate mask
 * values, a more complex blending operation is performed.
 *
 * @param s Pointer to the source pixel data array. Each DATA32 is a pixel (e.g., ARGB).
 *          Example: s[0] = 0xAARRGGBB
 * @param m Pointer to the mask data array. Each DATA8 is a mask value (0-255).
 *          Example: m[0] = 0 (transparent), m[0] = 255 (opaque)
 * @param c This parameter is unused as its value is immediately overwritten by `*m`.
 * @param d Pointer to the destination pixel data array, which is read and overwritten.
 *          Example: d[0] = 0xAARRGGBB
 * @param l The number of pixels to process.
 */
static void
_op_mul_p_mas_dp_mmx(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   MOV_A2R(ALPHA_255, mm5)
   pxor_r2r(mm0, mm0);
   while (d < e) {
	c = *m;
	switch(c)
	  {
	    case 0:
		break;
	    case 255:
		MOV_P2R(*d, mm1, mm0)
		MOV_P2R(*s, mm2, mm0)
		MUL4_SYM_R2R(mm2, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	    default:
		c++;
		MOV_A2R(c, mm1)
		c = ~(*s);
		MOV_P2R(c, mm3, mm0)
		MUL4_256_R2R(mm3, mm1)
		movq_r2r(mm5, mm4);
		psubw_r2r(mm1, mm4);
		MOV_P2R(*d, mm1, mm0)
		MUL4_SYM_R2R(mm4, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	s++;  m++;  d++;
     }
}

#define _op_mul_pas_mas_dp_mmx _op_mul_p_mas_dp_mmx
#define _op_mul_pan_mas_dp_mmx _op_mul_p_mas_dp_mmx

#define _op_mul_p_mas_dpan_mmx _op_mul_p_mas_dp_mmx
#define _op_mul_pas_mas_dpan_mmx _op_mul_pas_mas_dp_mmx
#define _op_mul_pan_mas_dpan_mmx _op_mul_pan_mas_dp_mmx

/**
 * @brief Initializes function pointers for MMX-optimized span multiplication operations.
 *
 * This function populates the `op_mul_span_funcs` dispatch table with pointers
 * to MMX-specific implementations for various combinations of source, mask,
 * and destination properties. These define how pixel spans are multiplied
 * when a mask is involved.
 */
static void
init_mul_pixel_mask_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_mul_p_mas_dp_mmx;
   op_mul_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_mul_pas_mas_dp_mmx;
   op_mul_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_mul_pan_mas_dp_mmx;

   op_mul_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_mul_p_mas_dpan_mmx;
   op_mul_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_mul_pas_mas_dpan_mmx;
   op_mul_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_mul_pan_mas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Initializes function pointers for MMX-optimized point multiplication operations.
 *
 * This function is intended to populate a dispatch table (similar to
 * `init_mul_pixel_mask_span_funcs_mmx`) for point-based (single pixel)
 * multiplication operations.
 * @note This function is currently a stub and does not initialize any operations.
 */
static void
init_mul_pixel_mask_pt_funcs_mmx(void)
{
}
#endif
