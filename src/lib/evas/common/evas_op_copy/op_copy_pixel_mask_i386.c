/**
 * @file op_copy_pixel_mask_i386.c
 * @brief MMX optimized functions for copying pixels with a mask.
 *
 * This file contains implementations for copying pixel data from a source
 * to a destination, applying a mask. Operations include direct copy
 * and relative copy, for both span and point operations.
 * These functions are specific to the i386 architecture and utilize MMX instructions.
 */

/* copy pixel x mask --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination, applying a mask (MMX version).
 *
 * Processes a line of pixels. For each pixel:
 * - If mask is 0, destination is unchanged.
 * - If mask is 255, source pixel is copied directly to destination.
 * - Otherwise, destination pixel is interpolated with source pixel based on mask value.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Example: `s[0]` is the first source pixel.
 * @param m Pointer to the mask data (array of DATA8).
 *          Example: `m[0]` is the mask for the first pixel. Mask values range from 0 (transparent) to 255 (opaque).
 * @param c Color value (unused in this function).
 * @param d Pointer to the destination pixel data (array of DATA32).
 *          Example: `d[0]` is the first destination pixel. This array is modified.
 * @param l Length of the pixel span to process.
 */
static void
_op_copy_p_mas_dp_mmx(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking pixels
   MOV_A2R(ALPHA_255, mm5)
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
		INTERP_256_R2R(mm3, mm2, mm1, mm5);
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  s++;  d++;
     }
}

#define _op_copy_pan_mas_dp_mmx _op_copy_p_mas_dp_mmx
#define _op_copy_pas_mas_dp_mmx _op_copy_p_mas_dp_mmx

#define _op_copy_p_mas_dpan_mmx _op_copy_p_mas_dp_mmx
#define _op_copy_pan_mas_dpan_mmx _op_copy_p_mas_dpan_mmx
#define _op_copy_pas_mas_dpan_mmx _op_copy_p_mas_dpan_mmx

/**
 * @brief Initializes function pointers for MMX span copy operations with mask.
 *
 * Assigns the MMX-optimized span copy functions to the global function pointer table
 * for various source, mask, and destination pixel format combinations.
 */
static void
init_copy_pixel_mask_span_funcs_mmx(void)
{
   op_copy_span_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_p_mas_dp_mmx;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pan_mas_dp_mmx;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pas_mas_dp_mmx;

   op_copy_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_p_mas_dpan_mmx;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pan_mas_dpan_mmx;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pas_mas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single pixel from source to destination, applying a mask (MMX version).
 *
 * Interpolates a single destination pixel with a source pixel based on a mask value.
 *
 * @param s Source pixel data (DATA32).
 * @param m Mask value (DATA8). Mask value ranges from 0 (transparent) to 255 (opaque).
 * @param c Temporary variable, effectively `m + 1` for interpolation.
 * @param d Pointer to the destination pixel data (DATA32). This value is modified.
 */
static void
_op_copy_pt_p_mas_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	c = m + 1; // Mask value is adjusted for 256-step interpolation
	MOV_A2R(ALPHA_255, mm5) // Load 255 into mm5 for alpha calculations
	MOV_A2R(c, mm3)
	pxor_r2r(mm0, mm0);
	MOV_P2R(s, mm2, mm0)
	MOV_P2R(*d, mm1, mm0)
	INTERP_256_R2R(mm3, mm2, mm1, mm5);
	MOV_R2P(mm1, *d, mm0)
}

#define _op_copy_pt_pan_mas_dp_mmx _op_copy_pt_p_mas_dp_mmx
#define _op_copy_pt_pas_mas_dp_mmx _op_copy_pt_p_mas_dp_mmx

#define _op_copy_pt_p_mas_dpan_mmx _op_copy_pt_p_mas_dp_mmx
#define _op_copy_pt_pan_mas_dpan_mmx _op_copy_pt_p_mas_dpan_mmx
#define _op_copy_pt_pas_mas_dpan_mmx _op_copy_pt_p_mas_dpan_mmx

/**
 * @brief Initializes function pointers for MMX point copy operations with mask.
 *
 * Assigns the MMX-optimized point copy functions to the global function pointer table
 * for various source, mask, and destination pixel format combinations.
 */
static void
init_copy_pixel_mask_pt_funcs_mmx(void)
{
   op_copy_pt_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pt_p_mas_dp_mmx;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pt_pan_mas_dp_mmx;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_pt_pas_mas_dp_mmx;

   op_copy_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_p_mas_dpan_mmx;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_pan_mas_dpan_mmx;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_pas_mas_dpan_mmx;
}
#endif

/*-----*/

/* copy_rel pixel x mask --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a span of pixels from source to destination, applying a mask and performing a relative (multiplicative) blend (MMX version).
 *
 * Processes a line of pixels. For each pixel:
 * - If mask is 0, destination is unchanged.
 * - If mask is 255, source pixel is multiplied with destination alpha and then blended into destination.
 * - Otherwise, source pixel is multiplied with destination alpha, then interpolated with destination pixel based on mask value.
 * This operation is typically used for effects like lighting or shading where the source modifies the destination relatively.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (array of DATA8). Mask values range from 0 to 255.
 * @param c Color value (unused in this function).
 * @param d Pointer to the destination pixel data (array of DATA32). This array is modified.
 * @param l Length of the pixel span to process.
 */
static void
_op_copy_rel_p_mas_dp_mmx(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0); // Zero out mm0
   MOV_A2R(ALPHA_255, mm5)
   while (d < e) {
	l = *m;
	switch(l)
	  {
	    case 0:
		break;
	    case 255:
		MOV_P2R(*s, mm2, mm0)
		MOV_PA2R(*d, mm1)
		MUL4_SYM_R2R(mm2, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	    default:
		l++;
		MOV_P2R(*s, mm3, mm0)
		MOV_P2R(*d, mm1, mm0)
		MOV_RA2R(mm1, mm2)
		MUL4_SYM_R2R(mm3, mm2, mm5)
		MOV_A2R(l, mm3)
		INTERP_256_R2R(mm3, mm2, mm1, mm5)
		MOV_R2P(mm1, *d, mm0)
		break;
	  }
	m++;  s++;  d++;
     }
}

#define _op_copy_rel_pan_mas_dp_mmx _op_copy_rel_p_mas_dp_mmx
#define _op_copy_rel_pas_mas_dp_mmx _op_copy_rel_p_mas_dp_mmx

#define _op_copy_rel_p_mas_dpan_mmx _op_copy_p_mas_dpan_mmx
#define _op_copy_rel_pan_mas_dpan_mmx _op_copy_pan_mas_dpan_mmx
#define _op_copy_rel_pas_mas_dpan_mmx _op_copy_pas_mas_dpan_mmx

/**
 * @brief Initializes function pointers for MMX relative span copy operations with mask.
 *
 * Assigns the MMX-optimized relative span copy functions to the global function pointer table
 * for various source, mask, and destination pixel format combinations.
 */
static void
init_copy_rel_pixel_mask_span_funcs_mmx(void)
{
   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_p_mas_dp_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pan_mas_dp_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pas_mas_dp_mmx;

   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_p_mas_dpan_mmx;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pan_mas_dpan_mmx;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pas_mas_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a single pixel from source to destination, applying a mask and performing a relative (multiplicative) blend (MMX version).
 *
 * The source pixel is first multiplied by the destination's alpha channel.
 * Then, this result is interpolated with the original destination pixel based on the mask value.
 *
 * @param s Source pixel data (DATA32).
 * @param m Mask value (DATA8). Mask value ranges from 0 to 255.
 * @param c Temporary variable, effectively `m + 1` for interpolation.
 * @param d Pointer to the destination pixel data (DATA32). This value is modified.
 */
static void
_op_copy_rel_pt_p_mas_dp_mmx(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
	c = m + 1; // Mask value is adjusted for 256-step interpolation
	pxor_r2r(mm0, mm0); // Zero out mm0
	MOV_A2R(ALPHA_255, mm5)
	MOV_P2R(s, mm3, mm0)
	MOV_P2R(*d, mm1, mm0)
	MOV_RA2R(mm1, mm2)
	MUL4_SYM_R2R(mm3, mm2, mm5)
	MOV_A2R(c, mm3)
	INTERP_256_R2R(mm3, mm2, mm1, mm5)
	MOV_R2P(mm1, *d, mm0)
}


#define _op_copy_rel_pt_pan_mas_dp_mmx _op_copy_rel_pt_p_mas_dp_mmx
#define _op_copy_rel_pt_pas_mas_dp_mmx _op_copy_rel_pt_p_mas_dp_mmx

#define _op_copy_rel_pt_p_mas_dpan_mmx _op_copy_pt_p_mas_dpan_mmx
#define _op_copy_rel_pt_pan_mas_dpan_mmx _op_copy_pt_pan_mas_dpan_mmx
#define _op_copy_rel_pt_pas_mas_dpan_mmx _op_copy_pt_pas_mas_dpan_mmx

/**
 * @brief Initializes function pointers for MMX relative point copy operations with mask.
 *
 * Assigns the MMX-optimized relative point copy functions to the global function pointer table
 * for various source, mask, and destination pixel format combinations.
 */
static void
init_copy_rel_pixel_mask_pt_funcs_mmx(void)
{
   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_p_mas_dp_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_pan_mas_dp_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_pas_mas_dp_mmx;

   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_p_mas_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_pan_mas_dpan_mmx;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_pas_mas_dpan_mmx;
}
#endif

