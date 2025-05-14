/* copy pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination, multiplying by a color. (MMX optimized)
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color to multiply with (DATA32, format: 0xAARRGGBB).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l The number of pixels to process (length of the span).
 *
 * This function processes 'l' pixels. For each pixel, it takes a source pixel pointed to by 's',
 * multiplies it by the color 'c', and stores the result in the destination pixel pointed to by 'd'.
 * It uses MMX instructions for optimized performance. Alpha component of 'c' is used.
 */
static void
_op_copy_p_c_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_P2R(c, mm3, mm0)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e) {
	MOV_P2R(*s, mm2, mm0)
	MUL4_SYM_R2R(mm3, mm2, mm5);
	MOV_R2P(mm2, *d, mm0)
	s++;  d++;
     }
}


/**
 * @brief Copies a span of pixels from source to destination, multiplying by color's alpha. (MMX optimized)
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color whose alpha component is used for multiplication (DATA32, format: 0xAARRGGBB).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l The number of pixels to process (length of the span).
 *
 * This function processes 'l' pixels. For each pixel, it takes a source pixel pointed to by 's',
 * multiplies it by the alpha component of color 'c' (adjusted to 0-256 range),
 * and stores the result in the destination pixel pointed to by 'd'.
 * It uses MMX instructions for optimized performance. Only alpha of 'c' is used.
 */
static void
_op_copy_p_caa_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   c = 1 + (c >> 24); /* Extract alpha from 'c' and scale to 1-256 range for multiplication */
   MOV_A2R(c, mm3)
   pxor_r2r(mm0, mm0);
   while (d < e) {
	MOV_P2R(*s, mm2, mm0)
	MUL4_256_R2R(mm3, mm2);
	MOV_R2P(mm2, *d, mm0)
	s++;  d++;
     }
}

/** @name Function aliases for different source/color alpha modes
 *  These macros define aliases for the MMX span copy operations.
 *  They map various combinations of source alpha (SP_AS, SP_AN) and
 *  color alpha (SC_AN, SC_AA) to the appropriate underlying MMX implementations.
 *  - _op_copy_p_c_dp_mmx: Source pixel alpha, Color alpha used.
 *  - _op_copy_p_caa_dp_mmx: Source pixel alpha, Color alpha only (from 'c') used.
 *  @{
 */
#define _op_copy_pas_c_dp_mmx _op_copy_p_c_dp_mmx
#define _op_copy_pan_c_dp_mmx _op_copy_p_c_dp_mmx
#define _op_copy_p_can_dp_mmx _op_copy_p_c_dp_mmx
#define _op_copy_pas_can_dp_mmx _op_copy_pas_c_dp_mmx
#define _op_copy_pan_can_dp_mmx _op_copy_pan_c_dp_mmx
#define _op_copy_pas_caa_dp_mmx _op_copy_p_caa_dp_mmx
#define _op_copy_pan_caa_dp_mmx _op_copy_p_caa_dp_mmx

/** @} */

/** @name Function aliases for destination alpha non-opaque modes
 *  Similar to the above, but for cases where the destination is not opaque (DP_AN).
 *  @{
 */
#define _op_copy_p_c_dpan_mmx _op_copy_p_c_dp_mmx
#define _op_copy_pas_c_dpan_mmx _op_copy_pas_c_dp_mmx
#define _op_copy_pan_c_dpan_mmx _op_copy_pan_c_dp_mmx
#define _op_copy_p_can_dpan_mmx _op_copy_p_can_dp_mmx
#define _op_copy_pas_can_dpan_mmx _op_copy_pas_can_dp_mmx
#define _op_copy_pan_can_dpan_mmx _op_copy_pan_can_dp_mmx
#define _op_copy_p_caa_dpan_mmx _op_copy_p_caa_dp_mmx
#define _op_copy_pas_caa_dpan_mmx _op_copy_pas_caa_dp_mmx
#define _op_copy_pan_caa_dpan_mmx _op_copy_pan_caa_dp_mmx
/** @} */

/**
 * @brief Initializes the MMX span copy function pointers.
 * This function populates the op_copy_span_funcs array with the
 * MMX-optimized span copy functions for various source, mask, color,
 * and destination alpha modes.
 */
static void
init_copy_pixel_color_span_funcs_mmx(void)
{
   op_copy_span_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_copy_p_c_dp_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_copy_pas_c_dp_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_copy_pan_c_dp_mmx;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_p_can_dp_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pas_can_dp_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pan_can_dp_mmx;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_p_caa_dp_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pas_caa_dp_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pan_caa_dp_mmx;

   op_copy_span_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_p_c_dpan_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pas_c_dpan_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pan_c_dpan_mmx;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_p_can_dpan_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pas_can_dpan_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pan_can_dpan_mmx;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_p_caa_dpan_mmx;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pas_caa_dpan_mmx;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pan_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single source pixel to a destination pixel, multiplying by a color. (MMX optimized point operation)
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this function).
 * @param c The color to multiply with (DATA32, format: 0xAARRGGBB).
 * @param d Pointer to the destination pixel data (DATA32).
 *
 * This function processes a single pixel. It takes the source pixel 's',
 * multiplies it by the color 'c', and stores the result in the destination pixel pointed to by 'd'.
 * It uses MMX instructions for optimized performance.
 */
static void
_op_copy_pt_p_c_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0); /* Zero out mm0 register */
	MOV_A2R(ALPHA_255, mm5) /* Load 255 (opaque alpha) into mm5 */
	MOV_P2R(c, mm2, mm0)
	MOV_P2R(s, mm2, mm0)
	MUL4_SYM_R2R(mm3, mm2, mm5);
	MOV_R2P(mm2, *d, mm0)
}

/** @name Function aliases for MMX point copy operations
 *  These macros define aliases for the MMX point copy operations,
 *  similar to the span copy aliases, for different alpha combinations.
 * @{
 */
#define _op_copy_pt_pas_c_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pan_c_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_p_can_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pas_can_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pan_can_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_p_caa_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pas_caa_dp_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pan_caa_dp_mmx _op_copy_pt_p_c_dp_mmx
/** @} */

/** @name Function aliases for MMX point copy operations with non-opaque destination
 * @{
 */
#define _op_copy_pt_p_c_dpan_mmx _op_copy_pt_p_c_dp_mmx
#define _op_copy_pt_pas_c_dpan_mmx _op_copy_pt_pas_c_dp_mmx
#define _op_copy_pt_pan_c_dpan_mmx _op_copy_pt_pan_c_dp_mmx
#define _op_copy_pt_p_can_dpan_mmx _op_copy_pt_p_can_dp_mmx
#define _op_copy_pt_pas_can_dpan_mmx _op_copy_pt_pas_can_dp_mmx
#define _op_copy_pt_pan_can_dpan_mmx _op_copy_pt_pan_can_dp_mmx
#define _op_copy_pt_p_caa_dpan_mmx _op_copy_pt_p_caa_dp_mmx
#define _op_copy_pt_pas_caa_dpan_mmx _op_copy_pt_pas_caa_dp_mmx
#define _op_copy_pt_pan_caa_dpan_mmx _op_copy_pt_pan_caa_dp_mmx
/** @} */

/**
 * @brief Initializes the MMX point copy function pointers.
 * This function populates the op_copy_pt_funcs array with the
 * MMX-optimized point copy functions for various source, mask, color,
 * and destination alpha modes.
 */
static void
init_copy_pixel_color_pt_funcs_mmx(void)
{
   op_copy_pt_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_copy_pt_p_c_dp_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_copy_pt_pas_c_dp_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_copy_pt_pan_c_dp_mmx;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pt_p_can_dp_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pt_pas_can_dp_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pt_pan_can_dp_mmx;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pt_p_caa_dp_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pt_pas_caa_dp_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pt_pan_caa_dp_mmx;

   op_copy_pt_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pt_p_c_dpan_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pt_pas_c_dpan_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pt_pan_c_dpan_mmx;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pt_p_can_dpan_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pt_pas_can_dpan_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pt_pan_can_dpan_mmx;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pt_p_caa_dpan_mmx;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pt_pas_caa_dpan_mmx;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pt_pan_caa_dpan_mmx;
}
#endif

/*-----*/

/* copy_rel pixel x color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination, modulating with a color and existing destination. (MMX optimized)
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color to multiply with (DATA32, format: 0xAARRGGBB).
 * @param d Pointer to the destination pixel data (array of DATA32), also used as a multiplicand.
 * @param l The number of pixels to process (length of the span).
 *
 * This function processes 'l' pixels. For each pixel, it takes a source pixel 's',
 * multiplies it by color 'c', then multiplies this result by the original destination pixel 'd',
 * and finally stores the outcome back into 'd'. This is a "relative" copy operation.
 * It uses MMX instructions for optimized performance.
 */
static void
_op_copy_rel_p_c_dp_mmx(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0);
   MOV_P2R(c, mm3, mm0)
   MOV_A2R(ALPHA_255, mm5)
   while (d < e) {
	MOV_PA2R(*d, mm1)
	MOV_P2R(*s, mm2, mm0)
	MUL4_SYM_R2R(mm3, mm2, mm5);
	MUL4_SYM_R2R(mm2, mm1, mm5);
	MOV_R2P(mm1, *d, mm0)
	s++;  d++;
     }
}

/** @name Function aliases for MMX relative span copy operations
 *  These macros define aliases for the MMX relative span copy operations,
 *  mapping various alpha combinations to the base implementation.
 * @{
 */
#define _op_copy_rel_pas_c_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_pan_c_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_p_can_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_pas_can_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_pan_can_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_p_caa_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_pas_caa_dp_mmx _op_copy_rel_p_c_dp_mmx
#define _op_copy_rel_pan_caa_dp_mmx _op_copy_rel_p_c_dp_mmx
/** @} */

/** @name Function aliases for MMX relative span copy operations with non-opaque destination
 *  Note: Some of these aliases point to non-relative functions (_op_copy_p_c_dpan_mmx etc.).
 *  This might indicate that for certain non-opaque destination scenarios, the "relative" aspect
 *  is handled differently or simplified to a standard copy operation.
 * @{
 */
#define _op_copy_rel_p_c_dpan_mmx _op_copy_p_c_dpan_mmx
#define _op_copy_rel_pas_c_dpan_mmx _op_copy_pas_c_dpan_mmx
#define _op_copy_rel_pan_c_dpan_mmx _op_copy_pan_c_dpan_mmx
#define _op_copy_rel_p_can_dpan_mmx _op_copy_p_can_dpan_mmx
#define _op_copy_rel_pas_can_dpan_mmx _op_copy_pas_can_dpan_mmx
#define _op_copy_rel_pan_can_dpan_mmx _op_copy_pan_can_dpan_mmx
#define _op_copy_rel_p_caa_dpan_mmx _op_copy_p_caa_dpan_mmx
#define _op_copy_rel_pas_caa_dpan_mmx _op_copy_pas_caa_dpan_mmx
#define _op_copy_rel_pan_caa_dpan_mmx _op_copy_pan_caa_dpan_mmx
/** @} */

/**
 * @brief Initializes the MMX relative span copy function pointers.
 * This function populates the op_copy_rel_span_funcs array with the
 * MMX-optimized relative span copy functions for various alpha modes.
 */
static void
init_copy_rel_pixel_color_span_funcs_mmx(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_p_c_dp_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pas_c_dp_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pan_c_dp_mmx;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_p_can_dp_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pas_can_dp_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pan_can_dp_mmx;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_p_caa_dp_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pas_caa_dp_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pan_caa_dp_mmx;

   op_copy_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_p_c_dpan_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pas_c_dpan_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pan_c_dpan_mmx;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_p_can_dpan_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pas_can_dpan_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pan_can_dpan_mmx;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_p_caa_dpan_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pas_caa_dpan_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pan_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single source pixel, modulating with a color and existing destination. (MMX optimized point operation)
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this function).
 * @param c The color to multiply with (DATA32, format: 0xAARRGGBB).
 * @param d Pointer to the destination pixel data (DATA32), also used as a multiplicand.
 *
 * This function processes a single pixel. It takes the source pixel 's', multiplies it by color 'c',
 * then multiplies this result by the original destination pixel 'd', and finally stores the outcome
 * back into 'd'. This is a "relative" point copy operation.
 * It uses MMX instructions for optimized performance.
 */
static void
_op_copy_rel_pt_p_c_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);
	MOV_P2R(c, mm3, mm0)
	MOV_A2R(ALPHA_255, mm5)
	MOV_PA2R(*d, mm1)
	MOV_P2R(s, mm2, mm0)
	MUL4_SYM_R2R(mm3, mm2, mm5);
	MUL4_SYM_R2R(mm2, mm1, mm5);
	MOV_R2P(mm1, *d, mm0)
}

/** @name Function aliases for MMX relative point copy operations
 *  These macros define aliases for the MMX relative point copy operations.
 * @{
 */
#define _op_copy_rel_pt_pas_c_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_pan_c_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_p_can_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_pas_can_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_pan_can_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_p_caa_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_pas_caa_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
#define _op_copy_rel_pt_pan_caa_dp_mmx _op_copy_rel_pt_p_c_dp_mmx
/** @} */

/** @name Function aliases for MMX relative point copy operations with non-opaque destination
 *  Note: These aliases point to non-relative point functions (_op_copy_pt_p_c_dpan_mmx etc.).
 *  This suggests that for non-opaque destination point operations, the "relative" aspect
 *  might be simplified to a standard point copy.
 * @{
 */
#define _op_copy_rel_pt_p_c_dpan_mmx _op_copy_pt_p_c_dpan_mmx
#define _op_copy_rel_pt_pas_c_dpan_mmx _op_copy_pt_pas_c_dpan_mmx
#define _op_copy_rel_pt_pan_c_dpan_mmx _op_copy_pt_pan_c_dpan_mmx
#define _op_copy_rel_pt_p_can_dpan_mmx _op_copy_pt_p_can_dpan_mmx
#define _op_copy_rel_pt_pas_can_dpan_mmx _op_copy_pt_pas_can_dpan_mmx
#define _op_copy_rel_pt_pan_can_dpan_mmx _op_copy_pt_pan_can_dpan_mmx
#define _op_copy_rel_pt_p_caa_dpan_mmx _op_copy_pt_p_caa_dpan_mmx
#define _op_copy_rel_pt_pas_caa_dpan_mmx _op_copy_pt_pas_caa_dpan_mmx
#define _op_copy_rel_pt_pan_caa_dpan_mmx _op_copy_pt_pan_caa_dpan_mmx
/** @} */

/**
 * @brief Initializes the MMX relative point copy function pointers.
 * This function populates the op_copy_rel_pt_funcs array with the
 * MMX-optimized relative point copy functions for various alpha modes.
 */
static void
init_copy_rel_pixel_color_pt_funcs_mmx(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pt_p_c_dp_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pt_pas_c_dp_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pt_pan_c_dp_mmx;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pt_p_can_dp_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pt_pas_can_dp_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pt_pan_can_dp_mmx;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pt_p_caa_dp_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pt_pas_caa_dp_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pt_pan_caa_dp_mmx;

   op_copy_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pt_p_c_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pt_pas_c_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pt_pan_c_dpan_mmx;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pt_p_can_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pt_pas_can_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pt_pan_can_dpan_mmx;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pt_p_caa_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pt_pas_caa_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pt_pan_caa_dpan_mmx;
}
#endif
