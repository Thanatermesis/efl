/**
 * @file op_blend_mask_color_i386.c
 * @brief MMX optimized functions for blending a color with a destination,
 *        using a mask.
 *
 * This file contains MMX implementations for various blending operations
 * where a source color is blended with a destination buffer, modulated by
 * a mask. It includes span and point operations for different blending modes.
 */

/* blend mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a color with destination pixels using a mask (MMX).
 *
 * This function blends a solid color @p c onto a destination buffer @p d,
 * with the blending amount for each pixel determined by the corresponding
 * alpha value in the mask @p m. The operation is performed for @p l pixels.
 * The source color @p c is pre-multiplied by its alpha.
 * The destination is also assumed to be pre-multiplied alpha.
 *
 * dst = (c * mask_alpha) + (dst * (1 - (c_alpha * mask_alpha)))
 * More precisely, since c is pre-multiplied:
 * c' = c * mask_alpha (where c is already c_val * c_alpha)
 * dst = c' + (dst * (1 - c'_alpha))
 * where c'_alpha = (c_alpha / 255) * mask_alpha
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8 alpha values).
 *          Example: [m1, m2, ..., ml] where m_i is alpha for pixel i.
 * @param c The color to blend (DATA32, ARGB format, pre-multiplied).
 *          Example: 0x80FF0000 for 50% transparent red.
 * @param d Pointer to the destination data (array of DATA32 pixels, ARGB, pre-multiplied).
 *          Example: [d1, d2, ..., dl] where d_i is a pixel.
 * @param l The number of pixels to process.
 */
static void
_op_blend_mas_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_P2R(c, mm2, mm0)
   c = 256 - (c >> 24);
   MOV_A2R(c, mm4)
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		MOV_P2R(*d, mm1, mm0)
		MUL4_256_R2R(mm4, mm1)
		paddw_r2r(mm2, mm1);
		MOV_R2P(mm1, *d, mm0)
		break;
	    default:
		l++;
		MOV_A2R(l, mm3)
		MUL4_256_R2R(mm2, mm3)

		MOV_RA2R(mm3, mm1)
		movq_r2r(mm6, mm7);
		psubw_r2r(mm1, mm7);

		MOV_P2R(*d, mm1, mm0)
		MUL4_256_R2R(mm7, mm1)

		paddw_r2r(mm3, mm1);
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  d++;
     }
}

/**
 * @brief Blend a color with destination pixels using a mask, assuming color has no alpha (MMX).
 *
 * This function blends a solid color @p c (ignoring its alpha channel, treating it as opaque)
 * onto a destination buffer @p d. The blending amount for each pixel is determined by the
 * corresponding alpha value in the mask @p m. The operation is performed for @p l pixels.
 * Destination is assumed to be pre-multiplied alpha.
 *
 * dst = (c_rgb * mask_alpha) + (dst * (1 - mask_alpha))
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8 alpha values).
 *          Example: [m1, m2, ..., ml]
 * @param c The color to blend (DATA32, ARGB format, alpha channel is effectively ignored and treated as 255).
 *          Example: 0xA0FF0000 for red, mask will determine transparency.
 * @param d Pointer to the destination data (array of DATA32 pixels, ARGB, pre-multiplied).
 *          Example: [d1, d2, ..., dl]
 * @param l The number of pixels to process.
 */
static void
_op_blend_mas_can_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_P2R(c, mm2, mm0)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		*d = c;
		break;
	    default:
		l++;
		MOV_A2R(l, mm3)
		MOV_P2R(*d, mm1, mm0)
		movq_r2r(mm2, mm4);
		INTERP_256_R2R(mm3, mm4, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  d++;
     }
}

#define _op_blend_mas_cn_dp_mmx _op_blend_mas_can_dp_mmx
#define _op_blend_mas_caa_dp_mmx _op_blend_mas_c_dp_mmx

#define _op_blend_mas_c_dpan_mmx _op_blend_mas_c_dp_mmx
#define _op_blend_mas_cn_dpan_mmx _op_blend_mas_cn_dp_mmx
#define _op_blend_mas_can_dpan_mmx _op_blend_mas_can_dp_mmx
#define _op_blend_mas_caa_dpan_mmx _op_blend_mas_caa_dp_mmx

/**
 * @brief Initializes MMX span blending functions for mask and color operations.
 *
 * This function assigns the MMX-optimized span blending functions
 * (like _op_blend_mas_c_dp_mmx, _op_blend_mas_can_dp_mmx)
 * to the global function pointer table `op_blend_span_funcs`.
 * These functions are used when blending a span of pixels where a source color
 * is modulated by a mask before being blended onto a destination.
 *
 * The indices used (SP_N, SM_AS, SC, DP, etc.) correspond to different
 * blending scenarios:
 * - SP_N: Source Pixels are Not used (color is used instead).
 * - SM_AS: Source Mask is Alpha Solid (mask data is used).
 * - SC: Source Color is present.
 * - SC_N: Source Color, No alpha.
 * - SC_AN: Source Color, Alpha, No pre-multiplication (effectively same as SC_N for mask ops).
 * - SC_AA: Source Color, Alpha, Alpha pre-multiplied.
 * - DP: Destination Pixels are present.
 * - DP_AN: Destination Pixels, Alpha, No pre-multiplication.
 * - CPU_MMX: Specifies that these are MMX implementations.
 */
static void
init_blend_mask_color_span_funcs_mmx(void)
{
   op_blend_span_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_blend_mas_c_dp_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_mas_cn_dp_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_blend_mas_can_dp_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_blend_mas_caa_dp_mmx;

   op_blend_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_blend_mas_c_dpan_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_mas_cn_dpan_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_blend_mas_can_dpan_mmx;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_blend_mas_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a single point (pixel) of color with a destination pixel using a mask value (MMX).
 *
 * This function blends a solid color @p c onto a single destination pixel pointed to by @p d.
 * The blending amount is determined by the mask alpha value @p m.
 * The source color @p c is pre-multiplied by its alpha.
 * The destination is also assumed to be pre-multiplied alpha.
 * Formula is similar to _op_blend_mas_c_dp_mmx but for a single pixel.
 *
 * @param s Source data (unused, but often holds mask value in other contexts, here m is used).
 * @param m The mask alpha value (DATA8). Example: 0x80 for 50% transparency.
 * @param c The color to blend (DATA32, ARGB format, pre-multiplied).
 *          Example: 0x80FF0000 for 50% transparent red.
 * @param d Pointer to the destination pixel (DATA32, ARGB, pre-multiplied).
 */
static void
_op_blend_pt_mas_c_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	// s is effectively m + 1 (mask value scaled from 0-255 to 1-256 range for multiplication)
	s = m + 1;
	MOV_A2R(s, mm3)
	MOV_A2R(ALPHA_256, mm6)
	pxor_r2r(mm0, mm0);
	MOV_P2R(c, mm2, mm0)
	MUL4_256_R2R(mm2, mm3)

	MOV_RA2R(mm3, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm6, mm1)

	paddw_r2r(mm3, mm1);
	MOV_R2P(mm1, *d, mm0)
}


#define _op_blend_pt_mas_cn_dp_mmx _op_blend_pt_mas_c_dp_mmx
#define _op_blend_pt_mas_can_dp_mmx _op_blend_pt_mas_c_dp_mmx
#define _op_blend_pt_mas_caa_dp_mmx _op_blend_pt_mas_c_dp_mmx

#define _op_blend_pt_mas_c_dpan_mmx _op_blend_pt_mas_c_dp_mmx
#define _op_blend_pt_mas_cn_dpan_mmx _op_blend_pt_mas_cn_dp_mmx
#define _op_blend_pt_mas_can_dpan_mmx _op_blend_pt_mas_can_dp_mmx
#define _op_blend_pt_mas_caa_dpan_mmx _op_blend_pt_mas_caa_dp_mmx

/**
 * @brief Initializes MMX point blending functions for mask and color operations.
 *
 * This function assigns the MMX-optimized point (single pixel) blending functions
 * (like _op_blend_pt_mas_c_dp_mmx) to the global function pointer table
 * `op_blend_pt_funcs`. These functions are used for blending a single pixel
 * where a source color is modulated by a mask value.
 *
 * The indexing scheme is similar to `init_blend_mask_color_span_funcs_mmx`.
 */
static void
init_blend_mask_color_pt_funcs_mmx(void)
{
   op_blend_pt_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_blend_pt_mas_c_dp_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pt_mas_cn_dp_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_blend_pt_mas_can_dp_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_blend_pt_mas_caa_dp_mmx;

   op_blend_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_blend_pt_mas_c_dpan_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_mas_cn_dpan_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_blend_pt_mas_can_dpan_mmx;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_blend_pt_mas_caa_dpan_mmx;
}
#endif

/*-----*/

/* blend_rel mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a color with destination pixels using a mask, relative alpha (MMX).
 *
 * This function performs a "relative" or "additive" blend of a color @p c
 * onto a destination buffer @p d, modulated by a mask @p m.
 * This is often used for effects like lighting or glows where the source color
 * adds to the destination, scaled by its own alpha and the mask.
 * The source color @p c is pre-multiplied. Destination is pre-multiplied.
 *
 * dst = (c * mask_alpha) + (dst * (1 - mask_alpha)) + (dst * c_alpha * mask_alpha) -> simplified from a more complex formula
 * More accurately, for "rel" (Porter-Duff "atop" like behavior or additive depending on interpretation):
 * Assuming c is (Cr, Cg, Cb, Ca) and d is (Dr, Dg, Db, Da), all pre-multiplied.
 * Mask alpha is Ma.
 * Result_rgb = (Cr * Ma) + (Dr * (1 - Ma)) + (Dr * Ca/255 * Ma)  -- this is an approximation of typical "rel" ops.
 * Result_alpha = Da + (Ca * Ma / 255) * (1 - Da/255) -- or simply Da if not changing dst alpha
 * The MMX code implements a specific type of "rel" blend.
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8 alpha values).
 * @param c The color to blend (DATA32, ARGB format, pre-multiplied).
 * @param d Pointer to the destination data (array of DATA32 pixels, ARGB, pre-multiplied).
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_mas_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_A2R(ALPHA_255, mm5)
   MOV_P2R(c, mm2, mm0)
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    default:
		l++;
		MOV_A2R(l, mm3)
		MUL4_256_R2R(mm2, mm3)

		MOV_RA2R(mm3, mm1)
		movq_r2r(mm6, mm7);
		psubw_r2r(mm1, mm7);

		MOV_P2R(*d, mm1, mm0)
		MOV_RA2R(mm1, mm4)
		MUL4_256_R2R(mm7, mm1)

		MUL4_SYM_R2R(mm4, mm3, mm5)

		paddw_r2r(mm3, mm1);
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  d++;
     }
}

#define _op_blend_rel_mas_cn_dp_mmx _op_blend_rel_mas_c_dp_mmx
#define _op_blend_rel_mas_can_dp_mmx _op_blend_rel_mas_c_dp_mmx
#define _op_blend_rel_mas_caa_dp_mmx _op_blend_rel_mas_c_dp_mmx

#define _op_blend_rel_mas_c_dpan_mmx _op_blend_mas_c_dpan_mmx
#define _op_blend_rel_mas_cn_dpan_mmx _op_blend_mas_cn_dpan_mmx
#define _op_blend_rel_mas_can_dpan_mmx _op_blend_mas_can_dpan_mmx
#define _op_blend_rel_mas_caa_dpan_mmx _op_blend_mas_caa_dpan_mmx

/**
 * @brief Initializes MMX relative span blending functions for mask and color operations.
 *
 * This function assigns MMX-optimized "relative" span blending functions
 * (like _op_blend_rel_mas_c_dp_mmx) to `op_blend_rel_span_funcs`.
 * "Relative" blending typically implies an additive or "atop" like effect.
 *
 * The indexing scheme is similar to `init_blend_mask_color_span_funcs_mmx`.
 */
static void
init_blend_rel_mask_color_span_funcs_mmx(void)
{
   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_blend_rel_mas_c_dp_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_rel_mas_cn_dp_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_blend_rel_mas_can_dp_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_blend_rel_mas_caa_dp_mmx;

   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_blend_rel_mas_c_dpan_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_mas_cn_dpan_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_blend_rel_mas_can_dpan_mmx;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_blend_rel_mas_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a single point (pixel) of color with a destination pixel using a mask, relative alpha (MMX).
 *
 * This function performs a "relative" blend of a solid color @p c onto a single
 * destination pixel @p d, modulated by mask alpha @p m.
 * Similar to _op_blend_rel_mas_c_dp_mmx but for a single pixel.
 *
 * @param s Source data (unused, but often holds mask value in other contexts).
 * @param m The mask alpha value (DATA8).
 * @param c The color to blend (DATA32, ARGB format, pre-multiplied).
 * @param d Pointer to the destination pixel (DATA32, ARGB, pre-multiplied).
 */
static void
_op_blend_rel_pt_mas_c_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_A2R(ALPHA_255, mm5)
	s = m + 1;
	MOV_A2R(s, mm3)
	MOV_P2R(c, mm2, mm0)
	MUL4_256_R2R(mm2, mm3)

	MOV_RA2R(mm3, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm6, mm1)

	MUL4_SYM_R2R(mm4, mm3, mm5)

	paddw_r2r(mm3, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_rel_pt_mas_cn_dp_mmx _op_blend_rel_pt_mas_c_dp_mmx
#define _op_blend_rel_pt_mas_can_dp_mmx _op_blend_rel_pt_mas_c_dp_mmx
#define _op_blend_rel_pt_mas_caa_dp_mmx _op_blend_rel_pt_mas_c_dp_mmx

#define _op_blend_rel_pt_mas_c_dpan_mmx _op_blend_pt_mas_c_dpan_mmx
#define _op_blend_rel_pt_mas_cn_dpan_mmx _op_blend_pt_mas_cn_dpan_mmx
#define _op_blend_rel_pt_mas_can_dpan_mmx _op_blend_pt_mas_can_dpan_mmx
#define _op_blend_rel_pt_mas_caa_dpan_mmx _op_blend_pt_mas_caa_dpan_mmx

/**
 * @brief Initializes MMX relative point blending functions for mask and color operations.
 *
 * This function assigns MMX-optimized "relative" point blending functions
 * (like _op_blend_rel_pt_mas_c_dp_mmx) to `op_blend_rel_pt_funcs`.
 *
 * The indexing scheme is similar to `init_blend_mask_color_pt_funcs_mmx`.
 */
static void
init_blend_rel_mask_color_pt_funcs_mmx(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_blend_rel_pt_mas_c_dp_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_mas_cn_dp_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_blend_rel_pt_mas_can_dp_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_blend_rel_pt_mas_caa_dp_mmx;

   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_blend_rel_pt_mas_c_dpan_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_mas_cn_dpan_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_blend_rel_pt_mas_can_dpan_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_blend_rel_pt_mas_caa_dpan_mmx;
}
#endif
