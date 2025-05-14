/* mask color --> dst */

/**
 * @file
 * @brief MMX optimized functions for masking color operations.
 *
 * This file contains MMX implementations for applying a color to a destination
 * buffer, optionally modulated by a mask. These functions are typically used
 * in graphics rendering pipelines.
 */

#ifdef BUILD_MMX
/**
 * @brief Apply a color to a span of destination pixels using MMX.
 *
 * This function takes a solid color `c` and applies it to a span of `l` pixels
 * in the destination buffer `d`. The source `s` and mask `m` are unused in this
 * specific version but are part of a generic function signature.
 * The color `c` is pre-multiplied by its alpha component.
 *
 * @param s Unused source pixel data.
 * @param m Unused mask data.
 * @param c The color to apply (ARGB format). Alpha is extracted and used for blending.
 * @param d Pointer to the destination pixel buffer (ARGB format).
 * @param l The number of pixels in the span.
 */
static void
_op_mask_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   // Extract alpha from color c, scale to 1-256 range for multiplication
   c = 1 + (c >> 24);
   MOV_A2R(c, mm2) // Move scaled alpha to mm2
   pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking bytes to words
   for (; d < e; d++) {
	// Load destination pixel into mm1, unpack to words
	MOV_P2R(*d, mm1, mm0)
	// Multiply destination pixel components by scaled alpha in mm2
	MUL4_256_R2R(mm2, mm1)
	// Pack words back to bytes and store in destination
	MOV_R2P(mm1, *d, mm0)
   }
}

/** @brief Alias for _op_mask_c_dp_mmx, treating color as alpha-aware. */
#define _op_mask_caa_dp_mmx _op_mask_c_dp_mmx

/** @brief Alias for _op_mask_c_dp_mmx, destination pixels are non-alpha. */
#define _op_mask_c_dpan_mmx _op_mask_c_dp_mmx
/** @brief Alias for _op_mask_caa_dp_mmx, destination pixels are non-alpha. */
#define _op_mask_caa_dpan_mmx _op_mask_caa_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for span-based mask color operations.
 *
 * This function assigns the MMX-optimized span operation functions to the
 * global function pointer table `op_mask_span_funcs`. This table is indexed by
 * various operation parameters like source/mask presence, color type,
 * destination properties, and CPU capabilities.
 *
 * `op_mask_span_funcs` likely has dimensions like:
 * [SourcePresent_e][SourceMask_e][ColorType_e][DestProperty_e][CpuFeature_e]
 * - SP_N: Source is not present or not used.
 * - SM_N: Source mask is not present or not used.
 * - SC: Solid Color.
 * - SC_AA: Solid Color, Alpha Aware.
 * - DP: Destination Pixels have alpha.
 * - DP_AN: Destination Pixels have no alpha (Alpha Not present).
 * - CPU_MMX: MMX instruction set is available.
 */
static void
init_mask_color_span_funcs_mmx(void)
{
   op_mask_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mask_c_dp_mmx;
   op_mask_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mask_caa_dp_mmx;

   op_mask_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mask_c_dpan_mmx;
   op_mask_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mask_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Apply a color to a single destination pixel using MMX.
 *
 * This function takes a solid color `c` and applies it to a single pixel
 * at the destination `d`. The source `s` and mask `m` are unused.
 * The color `c` is pre-multiplied by its alpha component. This is the
 * point (single pixel) version of _op_mask_c_dp_mmx.
 *
 * @param s Unused source pixel data.
 * @param m Unused mask data.
 * @param c The color to apply (ARGB format). Alpha is extracted and used for blending.
 * @param d Pointer to the destination pixel (ARGB format).
 */
static void
_op_mask_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	// Extract alpha from color c, scale to 1-256 range for multiplication
	c = 1 + (c >> 24);
	MOV_A2R(c, mm2) // Move scaled alpha to mm2
	pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking bytes to words
	// Load destination pixel into mm1, unpack to words
	MOV_P2R(*d, mm1, mm0)
	// Multiply destination pixel components by scaled alpha in mm2
	MUL4_256_R2R(mm2, mm1)
	// Pack words back to bytes and store in destination
	MOV_R2P(mm1, *d, mm0)
}

/** @brief Alias for _op_mask_pt_c_dp_mmx, treating color as alpha-aware. */
#define _op_mask_pt_caa_dp_mmx _op_mask_pt_c_dp_mmx

/** @brief Alias for _op_mask_pt_c_dp_mmx, destination pixel is non-alpha. */
#define _op_mask_pt_c_dpan_mmx _op_mask_pt_c_dp_mmx
/** @brief Alias for _op_mask_pt_caa_dp_mmx, destination pixel is non-alpha. */
#define _op_mask_pt_caa_dpan_mmx _op_mask_pt_caa_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for point-based (single pixel) mask color operations.
 *
 * This function assigns the MMX-optimized point operation functions to the
 * global function pointer table `op_mask_pt_funcs`. This table is indexed
 * similarly to `op_mask_span_funcs` but for single-pixel operations.
 *
 * `op_mask_pt_funcs` likely has dimensions like:
 * [SourcePresent_e][SourceMask_e][ColorType_e][DestProperty_e][CpuFeature_e]
 * (See init_mask_color_span_funcs_mmx for details on indices)
 */
static void
init_mask_color_pt_funcs_mmx(void)
{
   op_mask_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_mask_pt_c_dp_mmx;
   op_mask_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_mask_pt_caa_dp_mmx;

   op_mask_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_mask_pt_c_dpan_mmx;
   op_mask_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_mask_pt_caa_dpan_mmx;
}
#endif
