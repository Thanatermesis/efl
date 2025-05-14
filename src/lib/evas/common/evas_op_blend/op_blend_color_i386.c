/* blend color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a solid color onto a destination buffer using MMX.
 *
 * This function blends a constant color 'c' with each pixel in the destination
 * buffer 'd'. The source buffer 's' and mask 'm' are unused in this operation.
 * The blending formula applied is effectively:
 *   d = c + d * (1 - c.alpha)
 *
 * @param s Source pixel data (unused).
 * @param m Mask data (unused).
 * @param c The solid color to blend (in ARGB format). Alpha of 'c' is used for blending.
 * @param d Destination pixel data buffer (in ARGB format). Pixels are modified in place.
 * @param l Length of the pixel span to process.
 */
static void
_op_blend_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_P2R(c, mm2, mm0)
   c = 256 - (c >> 24);
   MOV_A2R(c, mm3)
   while (d < e) {
	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
	d++;
     }
}

#define _op_blend_caa_dp_mmx _op_blend_c_dp_mmx

#define _op_blend_c_dpan_mmx _op_blend_c_dp_mmx
#define _op_blend_caa_dpan_mmx _op_blend_c_dpan_mmx

/**
 * @brief Initializes MMX-specific span blending functions for solid color.
 *
 * This function assigns the MMX-optimized span blending function
 * (_op_blend_c_dp_mmx) to the appropriate entries in the
 * op_blend_span_funcs table. It covers cases with and without
 * alpha in the source color, and with and without alpha in the destination.
 */
static void
init_blend_color_span_funcs_mmx(void)
{
   op_blend_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_blend_c_dp_mmx;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_blend_caa_dp_mmx;

   op_blend_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_blend_c_dpan_mmx;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_blend_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a solid color onto a single destination pixel using MMX.
 *
 * This function blends a constant color 'c' with a single pixel pointed to by 'd'.
 * The source pixel 's' and mask 'm' are unused.
 * The blending formula is similar to the span version:
 *   d = c + d * (1 - c.alpha)
 *
 * @param s Source pixel (unused).
 * @param m Mask value (unused).
 * @param c The solid color to blend (in ARGB format). Alpha of 'c' is used for blending.
 * @param d Pointer to the destination pixel (in ARGB format). The pixel is modified in place.
 */
static void
_op_blend_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_P2R(c, mm2, mm0)
	c = 256 - (c >> 24);
	MOV_A2R(c, mm3)
	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_pt_caa_dp_mmx _op_blend_pt_c_dp_mmx

#define _op_blend_pt_c_dpan_mmx _op_blend_pt_c_dp_mmx
#define _op_blend_pt_caa_dpan_mmx _op_blend_pt_c_dpan_mmx

/**
 * @brief Initializes MMX-specific point blending functions for solid color.
 *
 * This function assigns the MMX-optimized point blending function
 * (_op_blend_pt_c_dp_mmx) to the appropriate entries in the
 * op_blend_pt_funcs table. It covers cases with and without
 * alpha in the source color, and with and without alpha in the destination.
 */
static void
init_blend_color_pt_funcs_mmx(void)
{
   op_blend_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_blend_pt_c_dp_mmx;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_blend_pt_caa_dp_mmx;

   op_blend_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_blend_pt_c_dpan_mmx;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_blend_pt_caa_dpan_mmx;
}
#endif
/*-----*/

/* blend_rel color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a solid color (relative) onto a destination buffer using MMX.
 *
 * This function performs a "relative" blend of a constant color 'c' with
 * each pixel in the destination buffer 'd'. The term "relative" implies
 * that the destination alpha influences the blending of the color components.
 * The blending formula is effectively:
 *   d.rgb = c.rgb * c.alpha + d.rgb * (1 - c.alpha)
 *   d.alpha = d.alpha (remains unchanged by this specific RGB blend part,
 *             but the overall operation might involve separate alpha blending)
 * This is a common Porter-Duff "over" operation if c.alpha is premultiplied.
 *
 * @param s Source pixel data (unused).
 * @param m Mask data (unused).
 * @param c The solid color to blend (in ARGB format). Alpha of 'c' is used for blending.
 * @param d Destination pixel data buffer (in ARGB format). Pixels are modified in place.
 * @param l Length of the pixel span to process.
 */
static void
_op_blend_rel_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_P2R(c, mm2, mm0)
   c = 256 - (c >> 24);
   MOV_A2R(c, mm3)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e) {
	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm3, mm1)
	MUL4_SYM_R2R(mm2, mm4, mm5)
	paddw_r2r(mm4, mm1);
	MOV_R2P(mm1, *d, mm0)
	d++;
     }
}

#define _op_blend_rel_caa_dp_mmx _op_blend_rel_c_dp_mmx

#define _op_blend_rel_c_dpan_mmx _op_blend_c_dpan_mmx
#define _op_blend_rel_caa_dpan_mmx _op_blend_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific span blending functions for relative solid color.
 *
 * This function assigns the MMX-optimized relative span blending function
 * (_op_blend_rel_c_dp_mmx) to the appropriate entries in the
 * op_blend_rel_span_funcs table.
 */
static void
init_blend_rel_color_span_funcs_mmx(void)
{
   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_blend_rel_c_dp_mmx;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_blend_rel_caa_dp_mmx;

   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_blend_rel_c_dpan_mmx;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_blend_rel_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a solid color (relative) onto a single destination pixel using MMX.
 *
 * This function performs a "relative" blend of a constant color 'c' with
 * a single pixel pointed to by 'd'.
 * The blending formula is similar to the span version:
 *   d.rgb = c.rgb * c.alpha + d.rgb * (1 - c.alpha)
 *
 * @param s Source pixel (unused).
 * @param m Mask value (unused).
 * @param c The solid color to blend (in ARGB format). Alpha of 'c' is used for blending.
 * @param d Pointer to the destination pixel (in ARGB format). The pixel is modified in place.
 */
static void
_op_blend_rel_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_A2R(ALPHA_255, mm5)

	MOV_P2R(c, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm6, mm1)

	MUL4_SYM_R2R(mm4, mm2, mm5)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_rel_pt_caa_dp_mmx _op_blend_rel_pt_c_dp_mmx

#define _op_blend_rel_pt_c_dpan_mmx _op_blend_pt_c_dpan_mmx
#define _op_blend_rel_pt_caa_dpan_mmx _op_blend_pt_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific point blending functions for relative solid color.
 *
 * This function assigns the MMX-optimized relative point blending function
 * (_op_blend_rel_pt_c_dp_mmx) to the appropriate entries in the
 * op_blend_rel_pt_funcs table.
 */
static void
init_blend_rel_color_pt_funcs_mmx(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_blend_rel_pt_c_dp_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_blend_rel_pt_caa_dp_mmx;

   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_blend_rel_pt_c_dpan_mmx;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_blend_rel_pt_caa_dpan_mmx;
}
#endif
