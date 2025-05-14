/* copy pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination using MMX.
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel span to copy.
 *
 * This function copies 'l' pixels from 's' to 'd'.
 * It processes 16 pixels (DATA32 words) at a time using MMX instructions
 * for optimized performance, then handles any remaining pixels individually.
 */
static void
_op_copy_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l - 15;
   for (; d < e; d+=16, s+=16) {
      MOVE_16DWORDS_MMX(s, d);
   }
   e+=15;
   for (; d < e; d++, s++) {
      *d = *s;
   }
}

#define _op_copy_pan_dp_mmx _op_copy_p_dp_mmx
#define _op_copy_pas_dp_mmx _op_copy_p_dp_mmx

#define _op_copy_p_dpan_mmx _op_copy_p_dp_mmx
#define _op_copy_pan_dpan_mmx _op_copy_pan_dp_mmx
#define _op_copy_pas_dpan_mmx _op_copy_pas_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for span copy operations.
 *
 * This function assigns the MMX-optimized pixel span copy functions
 * to the global function pointer array `op_copy_span_funcs`.
 * It covers different source and destination pixel properties (e.g., alpha).
 */
static void
init_copy_pixel_span_funcs_mmx(void)
{
   op_copy_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_copy_p_dp_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pan_dp_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pas_dp_mmx;

   op_copy_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_p_dpan_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pan_dpan_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single source pixel to a destination pixel using MMX.
 * @param s Source pixel data (DATA32).
 * @param m Mask data (DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (DATA32).
 *
 * This function copies the single source pixel 's' to the destination '*d'.
 */
static void
_op_copy_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
      *d = s;
}

#define _op_copy_pt_pan_dp_mmx _op_copy_pt_p_dp_mmx
#define _op_copy_pt_pas_dp_mmx _op_copy_pt_p_dp_mmx

#define _op_copy_pt_p_dpan_mmx _op_copy_pt_p_dp_mmx
#define _op_copy_pt_pan_dpan_mmx _op_copy_pt_pan_dp_mmx
#define _op_copy_pt_pas_dpan_mmx _op_copy_pt_pas_dp_mmx

/**
 * @brief Initializes the MMX-specific function pointers for single pixel copy operations.
 *
 * This function assigns the MMX-optimized single pixel copy functions
 * to the global function pointer array `op_copy_pt_funcs`.
 * It covers different source and destination pixel properties.
 */
static void
init_copy_pixel_pt_funcs_mmx(void)
{
   op_copy_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pt_p_dp_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pt_pan_dp_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pt_pas_dp_mmx;

   op_copy_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_p_dpan_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_pan_dpan_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_pas_dpan_mmx;
}
#endif

/*-----*/

/* copy_rel pixel --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination, relative to destination alpha, using MMX.
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel span to copy.
 *
 * This function copies 'l' pixels from 's' to 'd'. The operation is "relative"
 * meaning the source pixel is blended with the destination pixel based on the
 * destination's alpha channel.
 * MMX instructions are used for per-pixel operations.
 * mm0 is zeroed.
 * mm5 is loaded with ALPHA_255 (solid alpha).
 * For each pixel:
 *   - Destination pixel is loaded into mm1 (MOV_PA2R preserves alpha).
 *   - Source pixel is loaded into mm2 (MOV_P2R unpacks to 16-bit components, alpha is zeroed by mm0).
 *   - mm2 (source) is multiplied by mm1 (destination alpha) and then by mm5 (255), effectively scaling source by destination alpha.
 *   - Result is packed back into *d.
 */
static void
_op_copy_rel_p_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_255, mm5)
   for (; d < e; d++, s++) {
	MOV_PA2R(*d, mm1)
	MOV_P2R(*s, mm2, mm0)
	MUL4_SYM_R2R(mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
   }
}


#define _op_copy_rel_pas_dp_mmx _op_copy_rel_p_dp_mmx
#define _op_copy_rel_pan_dp_mmx _op_copy_rel_p_dp_mmx

#define _op_copy_rel_p_dpan_mmx _op_copy_p_dpan_mmx
#define _op_copy_rel_pan_dpan_mmx _op_copy_pan_dpan_mmx
#define _op_copy_rel_pas_dpan_mmx _op_copy_pas_dpan_mmx

/**
 * @brief Initializes the MMX-specific function pointers for relative span copy operations.
 *
 * This function assigns the MMX-optimized relative pixel span copy functions
 * to the global function pointer array `op_copy_rel_span_funcs`.
 * "Relative" copy implies blending based on destination alpha.
 * It covers different source and destination pixel properties.
 */
static void
init_copy_rel_pixel_span_funcs_mmx(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_p_dp_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pan_dp_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pas_dp_mmx;

   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_p_dpan_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pan_dpan_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single source pixel to a destination pixel, relative to destination alpha, using MMX.
 * @param s Source pixel data (DATA32).
 * @param m Mask data (DATA8, unused).
 * @param c Initially unused, then stores 1 + destination alpha.
 * @param d Pointer to the destination pixel data (DATA32).
 *
 * This function copies the single source pixel 's' to the destination '*d'.
 * The operation is "relative", blending the source with the destination
 * based on the destination's alpha.
 * 'c' is calculated as 1 + destination_alpha.
 * mm1 is loaded with 'c'.
 * mm0 is zeroed.
 * Source pixel 's' is loaded into mm2.
 * mm2 (source) is multiplied by mm1 (1 + dest alpha), effectively scaling source.
 * Result is packed back to *d.
 */
static void
_op_copy_rel_pt_p_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	c = 1 + (*d >> 24); // Calculate 1 + destination alpha
	MOV_A2R(c, mm1)
	pxor_r2r(mm0, mm0);
	MOV_P2R(s, mm2, mm0)
	MUL4_256_R2R(mm2, mm1)
	MOV_R2P(mm1, *d, mm0)
}


#define _op_copy_rel_pt_pan_dp_mmx _op_copy_rel_pt_p_dp_mmx
#define _op_copy_rel_pt_pas_dp_mmx _op_copy_rel_pt_p_dp_mmx

#define _op_copy_rel_pt_p_dpan_mmx _op_copy_pt_p_dpan_mmx
#define _op_copy_rel_pt_pan_dpan_mmx _op_copy_pt_pan_dpan_mmx
#define _op_copy_rel_pt_pas_dpan_mmx _op_copy_pt_pas_dpan_mmx

/**
 * @brief Initializes the MMX-specific function pointers for relative single pixel copy operations.
 *
 * This function assigns the MMX-optimized relative single pixel copy functions
 * to the global function pointer array `op_copy_rel_pt_funcs`.
 * "Relative" copy implies blending based on destination alpha.
 * It covers different source and destination pixel properties.
 */
static void
init_copy_rel_pixel_pt_funcs_mmx(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_p_dp_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_pan_dp_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_pas_dp_mmx;

   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_p_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_pan_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_pas_dpan_mmx;
}
#endif
