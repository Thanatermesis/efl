/* blend pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a span of pixels (source) onto a destination span using MMX.
 *
 * This function performs a pixel-wise blend operation:
 *   d = s + d * (1 - sa)
 * where 's' is the source pixel, 'd' is the destination pixel, and 'sa' is
 * the alpha component of the source pixel.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Each DATA32 is an ARGB pixel. Example: [0xAARRGGBB, 0xAARRGGBB, ...]
 * @param m Pointer to the mask data (unused in this function).
 * @param c Color value (unused in this function).
 * @param d Pointer to the destination pixel data (array of DATA32).
 *          Each DATA32 is an ARGB pixel. This array is modified in place.
 *          Example: [0xAARRGGBB, 0xAARRGGBB, ...]
 * @param l Length of the pixel span to process.
 */
static void
_op_blend_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)

	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

/**
 * @brief Blend a span of pixels (source alpha solid) onto a destination span using MMX.
 *
 * This function checks the source alpha. If fully opaque, it copies the source
 * to destination. If fully transparent, it does nothing. Otherwise, it performs
 * the same blend operation as _op_blend_p_dp_mmx.
 * Currently, it's implemented by directly calling _op_blend_p_dp_mmx,
 * with the more complex conditional logic commented out.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data (array of DATA32), modified in place.
 * @param l Length of the pixel span.
 */
static void
_op_blend_pas_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   _op_blend_p_dp_mmx(s, m, c, d, l);
   return;
/*
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   while (d < e)
     {
	switch (*s & 0xff000000)
	  {
	    case 0:
	      break;
	    case 0xff000000:
		*d = *s;
	      break;
	    default :
		MOV_P2R(*s, mm2, mm0)
		MOV_RA2R(mm2, mm1)
		movq_r2r(mm6, mm3);
		psubw_r2r(mm1, mm3);

		MOV_P2R(*d, mm1, mm0)
		MUL4_256_R2R(mm3, mm1)

		paddw_r2r(mm2, mm1);
		MOV_R2P(mm1, *d, mm0)
	      break;
	  }
	s++;  d++;
     }
 */
}

#define _op_blend_pan_dp_mmx NULL

#define _op_blend_p_dpan_mmx _op_blend_p_dp_mmx
#define _op_blend_pas_dpan_mmx _op_blend_pas_dp_mmx
#define _op_blend_pan_dpan_mmx _op_blend_pan_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for pixel span blending operations.
 *
 * This function assigns the appropriate MMX-accelerated blending functions
 * to a global table `op_blend_span_funcs`. This table is likely indexed by
 * various parameters like source/destination properties and CPU capabilities.
 */
static void
init_blend_pixel_span_funcs_mmx(void)
{
   op_blend_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_p_dp_mmx;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pas_dp_mmx;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pan_dp_mmx;

   op_blend_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_p_dpan_mmx;
   op_blend_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pas_dpan_mmx;
   op_blend_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pan_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a single source pixel onto a single destination pixel using MMX.
 *
 * This function performs the blend operation for a single point:
 *   d = s_pixel + d_pixel * (1 - s_pixel_alpha)
 *
 * @param s The source pixel (DATA32 ARGB). Example: 0xAARRGGBB
 * @param m Mask value (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel (DATA32 ARGB), modified in place.
 *          Example: *d = 0xAARRGGBB
 */
static void
_op_blend_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_P2R(s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MUL4_256_R2R(mm3, mm1)

	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}


#define _op_blend_pt_pan_dp_mmx NULL
#define _op_blend_pt_pas_dp_mmx _op_blend_pt_p_dp_mmx

#define _op_blend_pt_p_dpan_mmx _op_blend_pt_p_dp_mmx
#define _op_blend_pt_pan_dpan_mmx _op_blend_pt_pan_dp_mmx
#define _op_blend_pt_pas_dpan_mmx _op_blend_pt_pas_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for single pixel blending operations.
 *
 * This function assigns MMX-accelerated single pixel blending functions
 * to the `op_blend_pt_funcs` table, similar to `init_blend_pixel_span_funcs_mmx`
 * but for point operations.
 */
static void
init_blend_pixel_pt_funcs_mmx(void)
{
   op_blend_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_p_dp_mmx;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_pas_dp_mmx;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_pt_pan_dp_mmx;

   op_blend_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_p_dpan_mmx;
   op_blend_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pas_dpan_mmx;
   op_blend_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_pt_pan_dpan_mmx;
}
#endif

/*-----*/

/* blend_rel pixel -> dst */

#ifdef BUILD_MMX
/**
 * @brief Blend a span of pixels (source) onto a destination span with relative alpha using MMX.
 *
 * This function performs a pixel-wise blend operation considering the destination alpha:
 *   d = s * da + d * (1 - sa)
 * where 's' is source, 'd' is destination, 'sa' is source alpha, 'da' is destination alpha.
 * The MMX code implements:
 *   d' = s * (alpha_of_d) + d * (1 - alpha_of_s)
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data (array of DATA32), modified in place.
 * @param l Length of the pixel span.
 */
static void
_op_blend_rel_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	movq_r2r(mm6, mm3);
	psubw_r2r(mm1, mm3);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm3, mm1)

	MUL4_SYM_R2R(mm4, mm2, mm5)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

/**
 * @brief Blend a span of pixels (source alpha not solid) onto a destination span with relative alpha using MMX.
 *
 * This function performs a pixel-wise blend operation:
 *   d = s * da
 * where 's' is source, 'd' is destination, and 'da' is destination alpha.
 * The MMX code implements:
 *   d' = s * (alpha_of_d)
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data (array of DATA32), modified in place.
 * @param l Length of the pixel span.
 */
static void
_op_blend_rel_pan_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_256, mm6)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e)
     {
	MOV_P2R(*s, mm2, mm0)
	MOV_PA2R(*d, mm1)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

#define _op_blend_rel_pas_dp_mmx _op_blend_rel_p_dp_mmx

#define _op_blend_rel_p_dpan_mmx _op_blend_p_dpan_mmx
#define _op_blend_rel_pan_dpan_mmx _op_blend_pan_dpan_mmx
#define _op_blend_rel_pas_dpan_mmx _op_blend_pas_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative pixel span blending operations.
 *
 * Assigns MMX-accelerated relative blending functions for spans to `op_blend_rel_span_funcs`.
 */
static void
init_blend_rel_pixel_span_funcs_mmx(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_p_dp_mmx;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pas_dp_mmx;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pan_dp_mmx;

   op_blend_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_p_dpan_mmx;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pas_dpan_mmx;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pan_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Blend a single source pixel onto a single destination pixel with relative alpha using MMX.
 *
 * This function performs the relative blend operation for a single point:
 *   d = s * da + d * (1 - sa)
 * The MMX code implements:
 *   d_pixel' = s_pixel * (alpha_of_d_pixel) + d_pixel * (1 - alpha_of_s_pixel)
 *
 * @param s The source pixel (DATA32 ARGB).
 * @param m Mask value (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel (DATA32 ARGB), modified in place.
 */
static void
_op_blend_rel_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_256, mm6)
	MOV_A2R(ALPHA_255, mm5)

	MOV_P2R(s, mm2, mm0)
	MOV_RA2R(mm2, mm1)
	psubw_r2r(mm1, mm6);

	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm4)
	MUL4_256_R2R(mm6, mm1)

	MUL4_SYM_R2R(mm4, mm2, mm5)
	paddw_r2r(mm2, mm1);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_blend_rel_pt_pas_dp_mmx _op_blend_rel_pt_p_dp_mmx
#define _op_blend_rel_pt_pan_dp_mmx _op_blend_rel_pt_p_dp_mmx

#define _op_blend_rel_pt_p_dpan_mmx _op_blend_pt_p_dpan_mmx
#define _op_blend_rel_pt_pas_dpan_mmx _op_blend_pt_pas_dpan_mmx
#define _op_blend_rel_pt_pan_dpan_mmx _op_blend_pt_pan_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative single pixel blending operations.
 *
 * Assigns MMX-accelerated relative blending functions for single points
 * to `op_blend_rel_pt_funcs`.
 */
static void
init_blend_rel_pixel_pt_funcs_mmx(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_p_dp_mmx;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_pas_dp_mmx;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_blend_rel_pt_pan_dp_mmx;

   op_blend_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_p_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pas_dpan_mmx;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_blend_rel_pt_pan_dpan_mmx;
}
#endif
