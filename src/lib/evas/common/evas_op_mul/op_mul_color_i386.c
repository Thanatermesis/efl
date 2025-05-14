/**
 * @file op_mul_color_i386.c
 * @brief MMX optimized routines for multiplying a color with destination pixels.
 *
 * This file contains functions for multiplying a solid color onto destination
 * pixels, using MMX instructions for acceleration. It handles both span-based
 * and point-based operations.
 */

/* mul color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Multiplies a color with a span of destination pixels using MMX.
 *
 * This function takes a solid color @p c and multiplies it with each pixel
 * in the destination buffer @p d for a length @p l.
 * The source buffer @p s and mask @p m are unused in this operation.
 *
 * @param s Pointer to source pixel data (unused).
 * @param m Pointer to mask data (unused).
 * @param c The color (DATA32) to multiply with the destination pixels.
 *          The color format is typically 0xAARRGGBB.
 * @param d Pointer to the destination pixel data (DATA32 array).
 *          Each pixel is modified in place.
 * @param l The number of pixels in the span.
 */
static void
_op_mul_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking bytes to words.
   MOV_A2R(ALPHA_255, mm5) // Load 255 into mm5 for alpha scaling.
   MOV_P2R(c, mm2, mm0)    // Load color c into mm2.
   for (; d < e; d++) {
	MOV_P2R(*d, mm1, mm0)      // Load destination pixel *d into mm1.
	MUL4_SYM_R2R(mm2, mm1, mm5) // Multiply mm2 (color) with mm1 (pixel), result in mm1.
	                            // mm5 (255) is used for scaling during multiplication.
	MOV_R2P(mm1, *d, mm0)      // Store result from mm1 back to *d.
   }
}

/** @name MMX span multiplication function aliases
 *  These macros define aliases for _op_mul_c_dp_mmx, mapping it to various
 *  source and destination alpha handling scenarios. This indicates that for
 *  MMX color multiplication, the core logic is the same regardless of
 *  these specific alpha properties (e.g., SC_AN for source color with alpha,
 *  DP_AN for destination with alpha).
 *  @{
 */
#define _op_mul_can_dp_mmx _op_mul_c_dp_mmx
#define _op_mul_caa_dp_mmx _op_mul_c_dp_mmx

#define _op_mul_c_dpan_mmx _op_mul_c_dp_mmx
#define _op_mul_can_dpan_mmx _op_mul_can_dp_mmx
#define _op_mul_caa_dpan_mmx _op_mul_caa_dp_mmx
/** @} */

/**
 * @brief Initializes MMX-specific span color multiplication functions.
 *
 * This function populates the `op_mul_span_funcs` table with pointers
 * to MMX-optimized span multiplication functions for various drawing contexts.
 * The indices (e.g., SP_N, SM_N, SC, DP, CPU_MMX) represent different
 * configurations like source pixmap, source mask, source color,
 * destination properties, and CPU capabilities.
 */
static void
init_mul_color_span_funcs_mmx(void)
{
   op_mul_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mul_c_dp_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_can_dp_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_caa_dp_mmx;

   op_mul_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_c_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_can_dpan_mmx;
   op_mul_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Multiplies a color with a single destination pixel using MMX.
 *
 * This function takes a solid color @p c and multiplies it with the single
 * destination pixel pointed to by @p d.
 * The source pixel @p s and mask @p m are unused in this operation.
 *
 * @param s Source pixel data (DATA32, unused).
 * @param m Mask data (DATA8, unused).
 * @param c The color (DATA32) to multiply with the destination pixel.
 *          The color format is typically 0xAARRGGBB.
 * @param d Pointer to the single destination pixel (DATA32).
 *          The pixel is modified in place.
 */
static void
_op_mul_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	pxor_r2r(mm0, mm0);         // Zero out mm0, used for unpacking bytes to words.
	MOV_A2R(ALPHA_255, mm5)     // Load 255 into mm5 for alpha scaling.
	MOV_P2R(c, mm2, mm0)        // Load color c into mm2.
	MOV_P2R(*d, mm1, mm0)       // Load destination pixel *d into mm1.
	MUL4_SYM_R2R(mm2, mm1, mm5) // Multiply mm2 (color) with mm1 (pixel), result in mm1.
	                            // mm5 (255) is used for scaling during multiplication.
	MOV_R2P(mm1, *d, mm0)       // Store result from mm1 back to *d.
}

/** @name MMX point multiplication function aliases
 *  These macros define aliases for _op_mul_pt_c_dp_mmx, similar to the span
 *  aliases. They map the core MMX point color multiplication function to
 *  various source and destination alpha handling scenarios.
 *  @{
 */
#define _op_mul_pt_caa_dp_mmx _op_mul_pt_c_dp_mmx
#define _op_mul_pt_can_dp_mmx _op_mul_pt_c_dp_mmx

#define _op_mul_pt_c_dpan_mmx _op_mul_pt_c_dp_mmx
#define _op_mul_pt_can_dpan_mmx _op_mul_pt_can_dp_mmx
#define _op_mul_pt_caa_dpan_mmx _op_mul_pt_caa_dp_mmx
/** @} */

/**
 * @brief Initializes MMX-specific point color multiplication functions.
 *
 * This function populates the `op_mul_pt_funcs` table with pointers
 * to MMX-optimized point multiplication functions for various drawing contexts.
 * The indices (e.g., SP_N, SM_N, SC, DP, CPU_MMX) represent different
 * configurations.
 */
static void
init_mul_color_pt_funcs_mmx(void)
{
   op_mul_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mul_pt_c_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_mul_pt_can_dp_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mul_pt_caa_dp_mmx;

   op_mul_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mul_pt_c_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_mul_pt_can_dpan_mmx;
   op_mul_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mul_pt_caa_dpan_mmx;
}
#endif
