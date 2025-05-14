/**
 * @file op_copy_mask_color_i386.c
 * @brief MMX optimized functions for copying a color to a destination, modulated by a mask.
 *
 * These functions handle pixel operations where a source color is applied to
 * a destination buffer, with the intensity of the application controlled by a
 * mask. Operations are provided for both span (multiple pixels) and point
 * (single pixel) transfers. Both direct copy and relative copy (blending
 * with destination alpha) versions are included.
 */

/* copy mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels, applying a mask and a solid color (MMX optimized).
 *
 * This function takes a solid color @p c, applies a mask @p m to it,
 * and writes the result to the destination @p d for @p l pixels.
 * The source data @p s is unused in this operation.
 *
 * The operation is effectively: `*d = (*m * c) / 255` for each pixel,
 * with special handling for mask values 0 (output is 0) and 255 (output is c).
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8, one byte per pixel).
 *          Mask values range from 0 (transparent) to 255 (opaque).
 * @param c The solid color to apply (DATA32, ARGB format).
 * @param d Pointer to the destination buffer (array of DATA32, ARGB format).
 * @param l The number of pixels to process.
 */
static void
_op_copy_mas_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
#if 1
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = m*c */
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             *d = c;
                             break;
                          default:
                             alpha++;
                             *d = MUL_256(alpha, c);
                             break;
                          }
                        m++;  d++;
                     });
#else
#warning This MMX function looks broken. Please fixme.
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
	      {
		l++;
		MOV_A2R(l, mm3)
		MOV_P2R(*d, mm1, mm0)
		movq_r2r(mm2, mm4);
		INTERP_256_R2R(mm3, mm4, mm1, mm5);
		MOV_R2P(mm1, *d, mm0)
	      }
		break;
	  }
	m++;  d++;
     }
#endif
}

#define _op_copy_mas_cn_dp_mmx _op_copy_mas_c_dp_mmx
#define _op_copy_mas_can_dp_mmx _op_copy_mas_c_dp_mmx
#define _op_copy_mas_caa_dp_mmx _op_copy_mas_c_dp_mmx

#define _op_copy_mas_c_dpan_mmx _op_copy_mas_c_dp_mmx
#define _op_copy_mas_cn_dpan_mmx _op_copy_mas_c_dpan_mmx
#define _op_copy_mas_can_dpan_mmx _op_copy_mas_c_dpan_mmx
#define _op_copy_mas_caa_dpan_mmx _op_copy_mas_c_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for span copy operations with mask and color.
 *
 * This function assigns the appropriate MMX-optimized span copy routines
 * to the global function pointer table `op_copy_span_funcs`. These functions
 * are used when the source is effectively a solid color modulated by a mask.
 * It covers various combinations of source, mask, and destination properties.
 * - SP_N: Source is not used (solid color is provided directly).
 * - SM_AS: Source mask is present (alpha mask).
 * - SC_N, SC, SC_AN, SC_AA: Different color source types (though 'c' parameter is primary).
 * - DP, DP_AN: Destination pixel types (opaque or with alpha).
 * - CPU_MMX: Specifies that these are MMX implementations.
 */
static void
init_copy_mask_color_span_funcs_mmx(void)
{
   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_mas_cn_dp_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_copy_mas_c_dp_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_copy_mas_can_dp_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_copy_mas_caa_dp_mmx;

   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_mas_cn_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_copy_mas_c_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_copy_mas_can_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_copy_mas_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single pixel, applying a mask and a solid color (MMX optimized).
 *
 * This function takes a solid color @p c, applies a mask value @p m to it,
 * and writes the result to the destination pixel @p d.
 * The source data @p s is used to derive an alpha value (m + 1).
 *
 * The operation involves MMX instructions for interpolation.
 * `*d = INTERP_256(m + 1, c, *d)`
 *
 * @param s Source data (used to derive alpha for interpolation, effectively `m + 1`).
 * @param m The mask value (DATA8, 0-255).
 * @param c The solid color to apply (DATA32, ARGB format).
 * @param d Pointer to the destination pixel (DATA32, ARGB format).
 */
static void
_op_copy_pt_mas_c_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	s = m + 1; /* s becomes the alpha for interpolation, ranging from 1 to 256 */
	MOV_A2R(ALPHA_255, mm5) /* mm5 = 0x00ff00ff (used for 256-level scaling) */
	pxor_r2r(mm0, mm0);
	MOV_P2R(c, mm2, mm0)
	MOV_A2R(s, mm3)
	MOV_P2R(*d, mm1, mm0)
	INTERP_256_R2R(mm3, mm2, mm1, mm5);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_copy_pt_mas_cn_dp_mmx _op_copy_pt_mas_c_dp_mmx
#define _op_copy_pt_mas_can_dp_mmx _op_copy_pt_mas_c_dp_mmx
#define _op_copy_pt_mas_caa_dp_mmx _op_copy_pt_mas_c_dp_mmx

#define _op_copy_pt_mas_c_dpan_mmx _op_copy_pt_mas_c_dp_mmx
#define _op_copy_pt_mas_cn_dpan_mmx _op_copy_pt_mas_c_dpan_mmx
#define _op_copy_pt_mas_can_dpan_mmx _op_copy_pt_mas_c_dpan_mmx
#define _op_copy_pt_mas_caa_dpan_mmx _op_copy_pt_mas_c_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for single pixel copy operations with mask and color.
 *
 * This function assigns the appropriate MMX-optimized point copy routines
 * to the global function pointer table `op_copy_pt_funcs`. These functions
 * are used for single pixel operations where a solid color is modulated by a mask.
 * It covers various combinations similar to `init_copy_mask_color_span_funcs_mmx`.
 */
static void
init_copy_mask_color_pt_funcs_mmx(void)
{
   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pt_mas_cn_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_copy_pt_mas_c_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_copy_pt_mas_can_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_copy_pt_mas_caa_dp_mmx;

   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_mas_cn_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_copy_pt_mas_c_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_copy_pt_mas_can_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_copy_pt_mas_caa_dpan_mmx;
}
#endif

/*-----*/

/* copy_rel mask x color -> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels relative to destination alpha, applying a mask and a solid color (MMX optimized).
 *
 * This function blends a solid color @p c (modulated by mask @p m) with the
 * destination @p d, taking into account the destination's alpha channel.
 * The operation is relative, meaning the source color's alpha is multiplied
 * by the destination's alpha.
 *
 * @note Contains a "FIXME" comment indicating it might not have been fully tested.
 *
 * For each pixel:
 * If mask is 0, destination is unchanged.
 * If mask is 255:
 *   `effective_alpha = 1 + (*d >> 24)` (destination alpha + 1 for scaling)
 *   `*d = MUL4_256(c, effective_alpha)` (color `c` scaled by `effective_alpha`)
 * Else (mask is 1-254):
 *   `mask_alpha = *m + 1`
 *   `dest_alpha_component = (*d >> 24) + 1` (alpha component of destination for scaling)
 *   `scaled_color = MUL4_256(c, dest_alpha_component)`
 *   `*d = INTERP_256(mask_alpha, scaled_color, *d)`
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8).
 * @param c The solid color to apply (DATA32, ARGB format).
 * @param d Pointer to the destination buffer (array of DATA32, ARGB format).
 * @param l The number of pixels to process.
 */
static void
_op_copy_rel_mas_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
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
		l = 1 + (*d >> 24);
		MOV_A2R(l, mm1)
		MUL4_256_R2R(mm2, mm1)
		MOV_R2P(mm1, *d, mm0)
		break;
	    default:
		l++;
		MOV_A2R(l, mm3)
		MOV_P2R(*d, mm1, mm0)
		MOV_RA2R(mm1, mm4)
		MUL4_256_R2R(mm2, mm4)
		INTERP_256_R2R(mm3, mm4, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  d++;
     }
}

#define _op_copy_rel_mas_cn_dp_mmx _op_copy_rel_mas_c_dp_mmx
#define _op_copy_rel_mas_can_dp_mmx _op_copy_rel_mas_c_dp_mmx
#define _op_copy_rel_mas_caa_dp_mmx _op_copy_rel_mas_c_dp_mmx

#define _op_copy_rel_mas_c_dpan_mmx _op_copy_mas_c_dpan_mmx
#define _op_copy_rel_mas_cn_dpan_mmx _op_copy_mas_cn_dpan_mmx
#define _op_copy_rel_mas_can_dpan_mmx _op_copy_mas_can_dpan_mmx
#define _op_copy_rel_mas_caa_dpan_mmx _op_copy_mas_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative span copy operations with mask and color.
 *
 * This function assigns MMX-optimized "copy relative" span routines to
 * `op_copy_rel_span_funcs`. "Copy relative" implies that the operation
 * considers the destination alpha when blending.
 * The setup is similar to `init_copy_mask_color_span_funcs_mmx` but for
 * relative alpha blending operations.
 */
static void
init_copy_rel_mask_color_span_funcs_mmx(void)
{
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_mas_cn_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_copy_rel_mas_c_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_copy_rel_mas_can_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_copy_rel_mas_caa_dp_mmx;

   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_mas_cn_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_copy_rel_mas_c_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_mas_can_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_mas_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single pixel relative to destination alpha, applying a mask and a solid color (MMX optimized).
 *
 * This function blends a solid color @p c (modulated by mask value @p m)
 * with a single destination pixel @p d, taking into account the destination's
 * alpha channel. The source data @p s is used to derive an alpha value (m + 1).
 *
 * The operation is:
 * `mask_alpha = m + 1`
 * `dest_alpha_component = (*d >> 24) + 1` (alpha component of destination for scaling)
 * `scaled_color = MUL4_256(c, dest_alpha_component)`
 * `*d = INTERP_256(mask_alpha, scaled_color, *d)`
 *
 * @param s Source data (used to derive alpha for interpolation, effectively `m + 1`).
 * @param m The mask value (DATA8, 0-255).
 * @param c The solid color to apply (DATA32, ARGB format).
 * @param d Pointer to the destination pixel (DATA32, ARGB format).
 */
static void
_op_copy_rel_pt_mas_c_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	s = m + 1; /* s becomes the alpha for interpolation, ranging from 1 to 256 */
	MOV_A2R(ALPHA_255, mm5) /* mm5 = 0x00ff00ff (used for 256-level scaling) */
	pxor_r2r(mm0, mm0);
	MOV_A2R(s, mm3)
	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MOV_P2R(c, mm2, mm0)
	MUL4_256_R2R(mm2, mm4)
	INTERP_256_R2R(mm3, mm4, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
}

#define _op_copy_rel_pt_mas_cn_dp_mmx _op_copy_rel_pt_mas_c_dp_mmx
#define _op_copy_rel_pt_mas_can_dp_mmx _op_copy_rel_pt_mas_c_dp_mmx
#define _op_copy_rel_pt_mas_caa_dp_mmx _op_copy_rel_pt_mas_c_dp_mmx

#define _op_copy_rel_pt_mas_c_dpan_mmx _op_copy_pt_mas_c_dpan_mmx
#define _op_copy_rel_pt_mas_cn_dpan_mmx _op_copy_pt_mas_cn_dpan_mmx
#define _op_copy_rel_pt_mas_can_dpan_mmx _op_copy_pt_mas_can_dpan_mmx
#define _op_copy_rel_pt_mas_caa_dpan_mmx _op_copy_pt_mas_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative single pixel copy operations with mask and color.
 *
 * This function assigns MMX-optimized "copy relative" point routines to
 * `op_copy_rel_pt_funcs`. Similar to `init_copy_mask_color_pt_funcs_mmx`,
 * but for operations that consider destination alpha.
 */
static void
init_copy_rel_mask_color_pt_funcs_mmx(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_mas_cn_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_MMX] = _op_copy_rel_pt_mas_c_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_MMX] = _op_copy_rel_pt_mas_can_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_MMX] = _op_copy_rel_pt_mas_caa_dp_mmx;

   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_mas_cn_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_MMX] = _op_copy_rel_pt_mas_c_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pt_mas_can_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pt_mas_caa_dpan_mmx;
}
#endif
