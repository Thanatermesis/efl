/* blend pixel x mask --> dst */

#ifdef BUILD_MMX

// FIXME: These functions most likely don't perform the correct operation.
// Test them with masks and images.
#if 0
/**
 * @brief Blend source pixels (with alpha) with a mask onto opaque destination pixels using MMX.
 * @param s Pointer to the source pixel data (32-bit RGBA).
 * @param m Pointer to the mask data (8-bit alpha).
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (32-bit RGBA).
 * @param l Number of pixels to process.
 * @note This function is currently disabled (#if 0) and likely needs review.
 *       It intends to perform: D = S * M + D * (1 - M) where S's alpha is pre-applied to M.
 *       The operation seems to be: D = (S_rgb * M_alpha) + (D_rgb * (1 - M_alpha_final))
 *       where M_alpha_final is derived from S_alpha and M_mask.
 */
static void
_op_blend_pas_mas_dp_mmx(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   while (d < e) {
	l = (*s >> 24);
	switch(*m & l)
	  {
	    case 0:
		break;
	    case 255:
		*d = *s;
		break;
	    default:
		l = 1 + *m;
		MOV_A2R(l, mm3)
		MOV_P2R(*s, mm2, mm0)
		MUL4_256_R2R(mm3, mm2)

		MOV_RA2R(mm2, mm1)
		movq_r2r(mm6, mm3);
		psubw_r2r(mm1, mm3);

		MOV_P2R(*d, mm1, mm0)
		MUL4_256_R2R(mm3, mm1)

		paddw_r2r(mm2, mm1);
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  s++;  d++;
     }
}

/**
 * @brief Blend opaque source pixels with a mask onto opaque destination pixels using MMX.
 * @param s Pointer to the source pixel data (32-bit RGB, alpha ignored or 255).
 * @param m Pointer to the mask data (8-bit alpha).
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (32-bit RGBA).
 * @param l Number of pixels to process.
 * @note This function is currently disabled (#if 0) and likely needs review.
 *       It intends to perform: D = S * M + D * (1 - M).
 */
static void
_op_blend_pan_mas_dp_mmx(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   MOV_A2R(ALPHA_255, mm5)
   pxor_r2r(mm0, mm0);
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		*d = *s;
		break;
	    default:
		l++;
		MOV_A2R(l, mm3)
		MOV_P2R(*s, mm2, mm0)
		MOV_P2R(*d, mm1, mm0)
		INTERP_256_R2R(mm3, mm2, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  s++;  d++;
     }
}
#else
// FIXME
#define _op_blend_p_mas_dp_mmx NULL
#define _op_blend_pas_mas_dp_mmx _op_blend_p_mas_dp_mmx /**< Alias for MMX source-alpha + mask over destination. Currently points to _op_blend_p_mas_dp_mmx (NULL). */
#endif

#define _op_blend_pan_mas_dp_mmx _op_blend_pas_mas_dp_mmx /**< Alias for MMX source-no-alpha + mask over destination. Points to _op_blend_pas_mas_dp_mmx. */

#define _op_blend_p_mas_dpan_mmx _op_blend_p_mas_dp_mmx /**< Alias for MMX source + mask over destination-no-alpha. Points to _op_blend_p_mas_dp_mmx (NULL). */
#define _op_blend_pan_mas_dpan_mmx _op_blend_pan_mas_dp_mmx /**< Alias for MMX source-no-alpha + mask over destination-no-alpha. Points to _op_blend_pan_mas_dp_mmx. */
#define _op_blend_pas_mas_dpan_mmx _op_blend_pas_mas_dp_mmx /**< Alias for MMX source-alpha + mask over destination-no-alpha. Points to _op_blend_pas_mas_dp_mmx. */

/**
 * @brief Initializes MMX span blending functions for pixel operations with a mask.
 * These functions blend a span of pixels from a source to a destination,
 * applying a mask. The destination is treated as opaque (DP) or
 * opaque with alpha preservation (DP_AN).
 * @note Current MMX implementations for span operations with mask are set to NULL
 *       due to the _op_blend_p_mas_dp_mmx definition.
 */
static void
init_blend_pixel_mask_span_funcs_mmx(void)
{
   op_blend_span_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_p_mas_dp_mmx;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pas_mas_dp_mmx;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pan_mas_dp_mmx;

   op_blend_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_p_mas_dpan_mmx;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pas_mas_dpan_mmx;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pan_mas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a single opaque source pixel with a mask onto an opaque destination pixel using MMX.
 * @param s Source pixel data (32-bit RGBA, alpha is effectively 255).
 * @param m Mask data (8-bit alpha). The mask value `m` is used to calculate `m + 1`.
 * @param c Unused color parameter (repurposed for mask value `m + 1`).
 * @param d Pointer to the destination pixel data (32-bit RGBA).
 *
 * Performs the operation: D = (S * M_adj) + (D * (1 - M_adj))
 * where M_adj = (m + 1) / 256.
 */
static void
_op_blend_pt_p_mas_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	c = m + 1; // Mask value is m (0-255), effective alpha is (m+1)/256.
	MOV_A2R(c, mm3)
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_P2R(s, mm2, mm0)
	MUL4_256_R2R(mm3, mm2)

	MOV_RA2R(mm2, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm6, mm1)

	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_pt_pan_mas_dp_mmx _op_blend_pt_p_mas_dp_mmx /**< Alias for MMX point blend: source-no-alpha + mask over opaque destination. */
#define _op_blend_pt_pas_mas_dp_mmx _op_blend_pt_p_mas_dp_mmx /**< Alias for MMX point blend: source-alpha + mask over opaque destination. */

#define _op_blend_pt_p_mas_dpan_mmx _op_blend_pt_p_mas_dp_mmx /**< Alias for MMX point blend: source + mask over destination-no-alpha. */
#define _op_blend_pt_pas_mas_dpan_mmx _op_blend_pt_pas_mas_dp_mmx /**< Alias for MMX point blend: source-alpha + mask over destination-no-alpha. */
#define _op_blend_pt_pan_mas_dpan_mmx _op_blend_pt_pan_mas_dp_mmx /**< Alias for MMX point blend: source-no-alpha + mask over destination-no-alpha. */

/**
 * @brief Initializes MMX point blending functions for pixel operations with a mask.
 * These functions blend a single pixel from a source to a destination,
 * applying a mask. The destination is treated as opaque (DP) or
 * opaque with alpha preservation (DP_AN).
 */
static void
init_blend_pixel_mask_pt_funcs_mmx(void)
{
   op_blend_pt_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pt_p_mas_dp_mmx;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pt_pas_mas_dp_mmx;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_blend_pt_pan_mas_dp_mmx;

   op_blend_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_p_mas_dpan_mmx;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pas_mas_dpan_mmx;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pan_mas_dpan_mmx;
}
#endif

/*-----*/

/* blend_rel pixel x mask -> dst */

#ifdef BUILD_MMX

#define _op_blend_rel_p_mas_dpan_mmx NULL /**< MMX relative blend: source + mask over destination-no-alpha (span). Currently NULL. */
#define _op_blend_rel_pas_mas_dpan_mmx _op_blend_rel_p_mas_dpan_mmx /**< Alias for MMX relative blend: source-alpha + mask over destination-no-alpha (span). */
#define _op_blend_rel_pan_mas_dpan_mmx _op_blend_rel_pas_mas_dpan_mmx /**< Alias for MMX relative blend: source-no-alpha + mask over destination-no-alpha (span). */

/**
 * @brief Initializes MMX relative span blending functions for pixel operations with a mask.
 * These functions perform a "relative" blend of a span of pixels, applying a mask.
 * The destination is treated as opaque with alpha preservation (DP_AN).
 * @note Current MMX implementations for relative span operations with mask are set to NULL.
 */
static void
init_blend_rel_pixel_mask_span_funcs_mmx(void)
{
   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_p_mas_dpan_mmx;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pas_mas_dpan_mmx;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pan_mas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX

#define _op_blend_rel_pt_p_mas_dpan_mmx _op_blend_pt_p_mas_dpan_mmx /**< Alias for MMX relative point blend: source + mask over destination-no-alpha. Points to MMX point blend. */
#define _op_blend_rel_pt_pas_mas_dpan_mmx _op_blend_pt_pas_mas_dpan_mmx /**< Alias for MMX relative point blend: source-alpha + mask over destination-no-alpha. Points to MMX point blend. */
#define _op_blend_rel_pt_pan_mas_dpan_mmx _op_blend_pt_pan_mas_dpan_mmx /**< Alias for MMX relative point blend: source-no-alpha + mask over destination-no-alpha. Points to MMX point blend. */

/**
 * @brief Initializes MMX relative point blending functions for pixel operations with a mask.
 * These functions perform a "relative" blend of a single pixel, applying a mask.
 * The destination is treated as opaque with alpha preservation (DP_AN).
 * @note These currently point to the standard MMX point blending functions.
 */
static void
init_blend_rel_pixel_mask_pt_funcs_mmx(void)
{
   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_p_mas_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pas_mas_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pan_mas_dpan_mmx;
}
#endif
