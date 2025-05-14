/* mul mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Multiplies a mask by a color and blends the result with the destination. (MMX optimized)
 *
 * This function processes a span of pixels. For each pixel, it takes a mask value,
 * multiplies it by a given color, and then blends this result with the
 * corresponding destination pixel.
 *
 * @param s Pointer to the source data (unused in this function).
 * @param m Pointer to the mask data (array of DATA8). Each element represents the alpha/mask value for a pixel.
 * @param c The color (DATA32) to multiply the mask with.
 * @param d Pointer to the destination data (array of DATA32). Results are written here.
 *          Each DATA32 is an ARGB pixel, e.g., 0xAARRGGBB.
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_mul_mas_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   MOV_P2R(c, mm2, mm0)
   c = ~c;
   MOV_P2R(c, mm3, mm0)
   MOV_A2R(ALPHA_255, mm5)
   pxor_r2r(mm0, mm0);
   while (d < e) {
	DATA32 a = *m;
	switch(a)
	  {
	    case 0:
		break;
	    case 255:
		MOV_P2R(*d, mm1, mm0)
		MUL4_SYM_R2R(mm2, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	    default:
		a++;
		MOV_A2R(a, mm1)
		MUL4_256_R2R(mm3, mm1)
		movq_r2r(mm5, mm4);
		psubw_r2r(mm1, mm4);
		MOV_P2R(*d, mm1, mm0)
		MUL4_SYM_R2R(mm4, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  d++;
     }
}

#define _op_mul_mas_can_dp_mmx _op_mul_mas_c_dp_mmx
#define _op_mul_mas_caa_dp_mmx _op_mul_mas_c_dp_mmx

#define _op_mul_mas_c_dpan_mmx _op_mul_mas_c_dp_mmx
#define _op_mul_mas_can_dpan_mmx _op_mul_mas_can_dp_mmx
#define _op_mul_mas_caa_dpan_mmx _op_mul_mas_caa_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for mask multiplication span operations.
 *
 * This function assigns the MMX-optimized version of the mask multiplication
 * span operation to the appropriate entries in the global function pointer table
 * `op_mul_span_funcs`. This allows the rendering engine to dynamically select
 * the MMX version when available and appropriate for the current operation
 * parameters (source, mask, color, destination properties).
 */
static void
init_mul_mask_color_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_mul_mas_c_dp_mmx;
   op_mul_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_mul_mas_can_dp_mmx;
   op_mul_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_mul_mas_caa_dp_mmx;

   op_mul_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_mul_mas_c_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_mul_mas_can_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_mul_mas_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Multiplies a mask by a color for a single point and blends with the destination. (MMX optimized)
 *
 * This function processes a single pixel. It takes a mask value,
 * multiplies it by a given color, and then blends this result with the
 * destination pixel.
 *
 * @param s Source data (unused, but value is modified based on mask `m`).
 * @param m The mask value (DATA8) for the pixel.
 * @param c The color (DATA32) to multiply the mask with.
 * @param d Pointer to the destination pixel (DATA32). Result is written here.
 *          A DATA32 is an ARGB pixel, e.g., 0xAARRGGBB.
 */
static void
_op_mul_pt_mas_c_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	s = m + 1;
	c = ~c;
	MOV_P2R(c, mm3, mm0)
	MOV_A2R(ALPHA_255, mm4)
	pxor_r2r(mm0, mm0);
	MOV_A2R(s, mm1)
	MUL4_256_R2R(mm3, mm1)
	psubw_r2r(mm1, mm4);
	MOV_P2R(*d, mm1, mm0)
	MUL4_SYM_R2R(mm4, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
}

#define _op_mul_pt_mas_can_dp_mmx _op_mul_pt_mas_c_dp_mmx
#define _op_mul_pt_mas_caa_dp_mmx _op_mul_pt_mas_c_dp_mmx

#define _op_mul_pt_mas_c_dpan_mmx _op_mul_pt_mas_c_dp_mmx
#define _op_mul_pt_mas_can_dpan_mmx _op_mul_pt_mas_can_dp_mmx
#define _op_mul_pt_mas_caa_dpan_mmx _op_mul_pt_mas_caa_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for mask multiplication point operations.
 *
 * This function assigns the MMX-optimized version of the mask multiplication
 * point operation to the appropriate entries in the global function pointer table
 * `op_mul_pt_funcs`. This allows the rendering engine to dynamically select
 * the MMX version when available and appropriate for the current operation
 * parameters.
 */
static void
init_mul_mask_color_pt_funcs_mmx(void)
{
   op_mul_pt_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_mul_pt_mas_c_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_mul_pt_mas_can_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_mul_pt_mas_caa_dp_mmx;

   op_mul_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_mul_pt_mas_c_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_mas_can_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_mas_caa_dpan_mmx;
}
#endif
