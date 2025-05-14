/* copy color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a color value to a destination buffer using MMX.
 *
 * This function fills a block of memory (destination buffer 'd') with a given
 * color 'c'. It processes 'l' pixels. MMX instructions are used for
 * optimization, writing two pixels at a time where possible.
 *
 * @param s Source data pointer (unused).
 * @param m Mask data pointer (unused).
 * @param c The color to copy (DATA32).
 * @param d Destination buffer pointer.
 * @param l Length of the span in pixels.
 */
static void
_op_copy_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l - 1;
   movd_m2r(c, mm1);
   movq_r2r(mm1, mm2);
   psllq_i2r(32, mm1);
   por_r2r(mm2, mm1);
   for (; d < e; d+=2) {
      movq_r2m(mm1, d[0]);
   }
   e+=1;
   for (; d < e; d++) {
      *d = c;
   }
}

#define _op_copy_cn_dp_mmx _op_copy_c_dp_mmx
#define _op_copy_can_dp_mmx _op_copy_c_dp_mmx
#define _op_copy_caa_dp_mmx _op_copy_c_dp_mmx

#define _op_copy_cn_dpan_mmx _op_copy_c_dp_mmx
#define _op_copy_c_dpan_mmx _op_copy_c_dp_mmx
#define _op_copy_can_dpan_mmx _op_copy_c_dp_mmx
#define _op_copy_caa_dpan_mmx _op_copy_c_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for color copy span operations.
 *
 * This function assigns the MMX-optimized color copy span function
 * (_op_copy_c_dp_mmx and its aliases) to the appropriate entries in the
 * op_copy_span_funcs table. This table is likely used for dispatching
 * to the correct raster operation function based on source/destination
 * properties and CPU capabilities.
 *
 * The array op_copy_span_funcs might be structured as:
 * op_copy_span_funcs[source_present][source_mask][source_color_type][destination_packed_or_alpha][cpu_feature]
 * - SP_N: Source is not present (using a color value directly).
 * - SM_N: Source mask is not present.
 * - SC_N: Source color type is none (using a color value).
 * - SC: Source color type is solid color.
 * - SC_AN: Source color type is alpha + no alpha in color.
 * - SC_AA: Source color type is alpha + alpha in color.
 * - DP: Destination is packed pixels.
 * - DP_AN: Destination is packed pixels with alpha, but no alpha in source.
 * - CPU_MMX: MMX instruction set is available.
 */
static void
init_copy_color_span_funcs_mmx(void)
{
   op_copy_span_funcs[SP_N][SM_N][SC_N][DP][CPU_MMX] = _op_copy_cn_dp_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_copy_c_dp_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_can_dp_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_caa_dp_mmx;

   op_copy_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_cn_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_c_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_can_dpan_mmx;
   op_copy_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a color value to a single destination pixel using MMX (though MMX not strictly needed here).
 *
 * This function sets a single pixel at destination 'd' to the color 'c'.
 * While defined within MMX context, this specific operation is simple
 * and doesn't leverage MMX instructions directly for a single pixel.
 *
 * @param s Source data (unused).
 * @param m Mask data (unused).
 * @param c The color to copy (DATA32).
 * @param d Destination pixel pointer.
 */
static void
_op_copy_pt_c_dp_mmx(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
     *d = c;
}

#define _op_copy_pt_cn_dp_mmx _op_copy_pt_c_dp_mmx
#define _op_copy_pt_can_dp_mmx _op_copy_pt_c_dp_mmx
#define _op_copy_pt_caa_dp_mmx _op_copy_pt_c_dp_mmx

#define _op_copy_pt_cn_dpan_mmx _op_copy_pt_c_dp_mmx
#define _op_copy_pt_c_dpan_mmx _op_copy_pt_c_dp_mmx
#define _op_copy_pt_can_dpan_mmx _op_copy_pt_c_dp_mmx
#define _op_copy_pt_caa_dpan_mmx _op_copy_pt_c_dp_mmx

/**
 * @brief Initializes MMX-specific function pointers for color copy point operations.
 *
 * This function assigns the MMX-optimized color copy point function
 * (_op_copy_pt_c_dp_mmx and its aliases) to the appropriate entries in the
 * op_copy_pt_funcs table. This table is used for dispatching
 * to the correct raster operation function for single pixel operations.
 *
 * The array op_copy_pt_funcs structure is similar to op_copy_span_funcs:
 * op_copy_pt_funcs[source_present][source_mask][source_color_type][destination_packed_or_alpha][cpu_feature]
 */
static void
init_copy_color_pt_funcs_mmx(void)
{
   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_MMX] = _op_copy_pt_cn_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_copy_pt_c_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_pt_can_dp_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_pt_caa_dp_mmx;

   op_copy_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_pt_cn_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_pt_c_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_pt_can_dpan_mmx;
   op_copy_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_pt_caa_dpan_mmx;
}
#endif

/*-----*/

/* copy_rel color --> dst */

#ifdef BUILD_MMX
/**
 * @brief Copies a color value to a destination buffer, modulating with destination alpha (relative copy), using MMX.
 *
 * This function copies a color 'c' to a destination buffer 'd' of length 'l'.
 * The operation is "relative" meaning the source color 'c' is multiplied by the
 * destination pixel's alpha component before being written. This effectively
 * blends the color 'c' based on the existing transparency of the destination.
 * MMX instructions are used for pixel processing.
 *
 * @param s Source data pointer (unused).
 * @param m Mask data pointer (unused).
 * @param c The color to copy (DATA32).
 * @param d Destination buffer pointer.
 * @param l Length of the span in pixels.
 */
static void
_op_copy_rel_c_dp_mmx(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   pxor_r2r(mm0, mm0); // Zero out mm0, used for unpacking bytes to words.
   MOV_P2R(c, mm2, mm0)
   for (; d < e; d++) {
	DATA32  da = 1 + (*d >> 24);
	MOV_A2R(da, mm1)
	MUL4_256_R2R(mm2, mm1)
	MOV_R2P(mm1, *d, mm0)
   }
}

#define _op_copy_rel_cn_dp_mmx _op_copy_rel_c_dp_mmx
#define _op_copy_rel_can_dp_mmx _op_copy_rel_c_dp_mmx
#define _op_copy_rel_caa_dp_mmx _op_copy_rel_c_dp_mmx

#define _op_copy_rel_cn_dpan_mmx _op_copy_cn_dpan_mmx
#define _op_copy_rel_c_dpan_mmx _op_copy_c_dpan_mmx
#define _op_copy_rel_can_dpan_mmx _op_copy_can_dpan_mmx
#define _op_copy_rel_caa_dpan_mmx _op_copy_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative color copy span operations.
 *
 * This function assigns the MMX-optimized relative color copy span function
 * (_op_copy_rel_c_dp_mmx and its aliases, plus other potentially distinct
 * dpan variants) to the appropriate entries in the op_copy_rel_span_funcs table.
 * This table is for dispatching raster operations that involve modulating
 * the source color with the destination alpha.
 *
 * The array op_copy_rel_span_funcs structure is similar to op_copy_span_funcs:
 * op_copy_rel_span_funcs[source_present][source_mask][source_color_type][destination_packed_or_alpha][cpu_feature]
 * Note: The dpan variants (_op_copy_cn_dpan_mmx etc.) are defined elsewhere and
 * likely handle cases where the destination has alpha but the source might not
 * explicitly provide it in the same way.
 */
static void
init_copy_rel_color_span_funcs_mmx(void)
{
   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_cn_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_c_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_can_dp_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_caa_dp_mmx;

   op_copy_rel_span_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_cn_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_c_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_can_dpan_mmx;
   op_copy_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_caa_dpan_mmx;
}
#endif

#ifdef BUILD_MMX
/**
 * @brief Copies a color value to a single destination pixel, modulating with destination alpha (relative copy), using MMX.
 *
 * This function sets a single pixel at destination 'd' to the color 'c',
 * after modulating 'c' with the alpha of the original pixel at 'd'.
 * The source parameter 's' is repurposed here to store the destination alpha.
 * MMX instructions are used for the color manipulation.
 *
 * @param s Repurposed to hold (1 + destination_alpha). DATA32, but only alpha byte is relevant.
 * @param m Mask data (unused).
 * @param c The color to copy (DATA32).
 * @param d Destination pixel pointer.
 */
static void
_op_copy_rel_pt_c_dp_mmx(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	s = 1 + (*d >> 24); // Extract destination alpha, add 1 for scaling (common 0-255 to 1-256 range for mul by 256)
	pxor_r2r(mm0, mm0); // Zero out mm0
	MOV_P2R(c, mm2, mm0) // Unpack color c into mm2 (e.g., 00R0G0B0)
	MOV_A2R(s, mm1)
	MUL4_256_R2R(mm2, mm1)
	MOV_R2P(mm1, *d, mm0)
}


#define _op_copy_rel_pt_cn_dp_mmx _op_copy_rel_pt_c_dp_mmx
#define _op_copy_rel_pt_can_dp_mmx _op_copy_rel_pt_c_dp_mmx
#define _op_copy_rel_pt_caa_dp_mmx _op_copy_rel_pt_c_dp_mmx

#define _op_copy_rel_pt_cn_dpan_mmx _op_copy_pt_cn_dpan_mmx
#define _op_copy_rel_pt_c_dpan_mmx _op_copy_pt_c_dpan_mmx
#define _op_copy_rel_pt_can_dpan_mmx _op_copy_pt_can_dpan_mmx
#define _op_copy_rel_pt_caa_dpan_mmx _op_copy_pt_caa_dpan_mmx

/**
 * @brief Initializes MMX-specific function pointers for relative color copy point operations.
 *
 * This function assigns the MMX-optimized relative color copy point function
 * (_op_copy_rel_pt_c_dp_mmx and its aliases, plus other potentially distinct
 * dpan variants) to the appropriate entries in the op_copy_rel_pt_funcs table.
 * This table is for dispatching single-pixel raster operations that involve
 * modulating the source color with the destination alpha.
 *
 * The array op_copy_rel_pt_funcs structure is similar to op_copy_span_funcs:
 * op_copy_rel_pt_funcs[source_present][source_mask][source_color_type][destination_packed_or_alpha][cpu_feature]
 * Note: The dpan variants (_op_copy_pt_cn_dpan_mmx etc.) are defined elsewhere and
 * likely handle cases where the destination has alpha but the source might not
 * explicitly provide it in the same way. These appear to be self-referential defines
 * in the provided code snippet, which might indicate they are placeholders or defined
 * in a different part of the codebase not shown.
 */
static void
init_copy_rel_color_pt_funcs_mmx(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP][CPU_MMX] = _op_copy_rel_pt_cn_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_MMX] = _op_copy_rel_pt_c_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP][CPU_MMX] = _op_copy_rel_pt_can_dp_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_MMX] = _op_copy_rel_pt_caa_dp_mmx;

   op_copy_rel_pt_funcs[SP_N][SM_N][SC_N][DP_AN][CPU_MMX] = _op_copy_rel_pt_cn_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_MMX] = _op_copy_rel_pt_c_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AN][DP_AN][CPU_MMX] = _op_copy_rel_pt_can_dpan_mmx;
   op_copy_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_MMX] = _op_copy_rel_pt_caa_dpan_mmx;
}
#endif
