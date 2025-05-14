/* mul pixel x color --> dst */

/**
 * @brief Multiplies source pixels by a color and then by destination pixels.
 *
 * This function iterates over a span of `l` pixels. For each pixel, it first
 * multiplies the source pixel `s` by the color `c` using `MUL4_SYM`.
 * The result `cs` is then multiplied by the destination pixel `d` using `MUL4_SYM`,
 * and the final result is stored back in `d`.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Each DATA32 represents a pixel, typically in ARGB format.
 *          Example: `s[0] = 0xFFRRGGBB`
 * @param m Pointer to the mask data (array of DATA8). This parameter is unused.
 * @param c The color to multiply with the source pixels (DATA32).
 *          Typically in ARGB format. Example: `c = 0xFFRRGGBB`
 * @param d Pointer to the destination pixel data (array of DATA32), also used as output.
 *          Each DATA32 represents a pixel, typically in ARGB format.
 *          Example: `d[0] = 0xFFRRGGBB`
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_mul_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   while (d < e) {
	DATA32 cs = MUL4_SYM(c, *s);
	*d = MUL4_SYM(cs, *d);
	s++;  d++;
     }
}

/**
 * @brief Multiplies source pixels by a color's alpha component and then by destination pixels.
 *
 * This function iterates over a span of `l` pixels. It first extracts and
 * normalizes the alpha component of the color `c`. For each pixel, it multiplies
 * the source pixel `s` by this normalized alpha using `MUL_256`. The result `cs`
 * is then multiplied by the destination pixel `d` using `MUL4_SYM`, and the
 * final result is stored back in `d`. This is typically used for operations
 * where only the alpha of the color `c` affects the source pixel before
 * blending with the destination.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Example: `s[0] = 0xFFRRGGBB`
 * @param m Pointer to the mask data (array of DATA8). This parameter is unused.
 * @param c The color whose alpha component is used for multiplication (DATA32).
 *          Example: `c = 0xAARRGGBB` (alpha `AA` is used)
 * @param d Pointer to the destination pixel data (array of DATA32), also used as output.
 *          Example: `d[0] = 0xFFRRGGBB`
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_mul_p_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e = d + l;
   c = 1 + (c >> 24); // Extract and normalize alpha: (alpha_value / 255) * 256, effectively (alpha_value + 1)
   while (d < e)
     {
	DATA32 cs = MUL_256(c, *s); // Multiply source by color's alpha
	*d = MUL4_SYM(cs, *d);    // Multiply result by destination
	s++;  d++;
     }
}

/** @name Function aliases for span operations
 *  These macros define aliases for different combinations of source, color,
 *  and destination pixel properties (e.g., alpha presence).
 *  They all point to either `_op_mul_p_c_dp` or `_op_mul_p_caa_dp`.
 * @{
 */
#define _op_mul_pas_c_dp _op_mul_p_c_dp
#define _op_mul_pan_c_dp _op_mul_p_c_dp
#define _op_mul_p_can_dp _op_mul_p_c_dp
#define _op_mul_pas_can_dp _op_mul_p_c_dp
#define _op_mul_pan_can_dp _op_mul_p_c_dp
#define _op_mul_pas_caa_dp _op_mul_p_caa_dp
#define _op_mul_pan_caa_dp _op_mul_p_caa_dp

/** @} */

/** @name Function aliases for span operations with destination alpha no-op
 *  Similar to the above, but for cases where destination alpha is handled
 *  as a no-op (dpan).
 * @{
 */
#define _op_mul_p_c_dpan _op_mul_p_c_dp
#define _op_mul_pas_c_dpan _op_mul_pas_c_dp
#define _op_mul_pan_c_dpan _op_mul_pan_c_dp
#define _op_mul_p_can_dpan _op_mul_p_can_dp
#define _op_mul_pas_can_dpan _op_mul_pas_can_dp
#define _op_mul_pan_can_dpan _op_mul_pan_can_dp
#define _op_mul_p_caa_dpan _op_mul_p_caa_dp
#define _op_mul_pas_caa_dpan _op_mul_pas_caa_dp
#define _op_mul_pan_caa_dpan _op_mul_pan_caa_dp
/** @} */

/**
 * @brief Initializes the function pointers for span-based multiplication operations.
 *
 * This function populates the `op_mul_span_funcs` array with the appropriate
 * function pointers for various combinations of source, mask, color, and
 * destination properties. These are used for C-based CPU implementations.
 * The array indices (SP, SM_N, SC, DP, CPU_C, etc.) represent different
 * pixel format and operation mode combinations.
 *
 * Example of array structure:
 * `op_mul_span_funcs[source_props][mask_mode][source_color_props][dest_props][cpu_type]`
 * Each element is a function pointer of type `op_mul_span_func`.
 */
static void
init_mul_pixel_color_span_funcs_c(void)
{
   op_mul_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_mul_p_c_dp;
   op_mul_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_mul_pas_c_dp;
   op_mul_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_mul_pan_c_dp;
   op_mul_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_mul_p_can_dp;
   op_mul_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_mul_pas_can_dp;
   op_mul_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_mul_pan_can_dp;
   op_mul_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_mul_p_caa_dp;
   op_mul_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_mul_pas_caa_dp;
   op_mul_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_mul_pan_caa_dp;

   op_mul_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_mul_p_c_dpan;
   op_mul_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_mul_pas_c_dpan;
   op_mul_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_mul_pan_c_dpan;
   op_mul_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_p_can_dpan;
   op_mul_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pas_can_dpan;
   op_mul_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pan_can_dpan;
   op_mul_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_p_caa_dpan;
   op_mul_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pas_caa_dpan;
   op_mul_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pan_caa_dpan;
}

/**
 * @brief Multiplies a single source pixel by a color and then by a destination pixel.
 *
 * This function performs the multiplication for a single point (pixel).
 * It first multiplies the source pixel `s` by the color `c` using `MUL4_SYM`.
 * The result is then multiplied by the destination pixel pointed to by `d`
 * using `MUL4_SYM`, and the final result is stored in `*d`.
 *
 * @param s The source pixel (DATA32). Example: `s = 0xFFRRGGBB`
 * @param m The mask value (DATA8). This parameter is unused.
 * @param c The color to multiply with the source pixel (DATA32). Example: `c = 0xFFRRGGBB`
 * @param d Pointer to the destination pixel (DATA32), also used as output.
 *          Example: `*d = 0xFFRRGGBB`
 */
static void
_op_mul_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	s = MUL4_SYM(c, s);
	*d = MUL4_SYM(s, *d);
}

/**
 * @brief Multiplies a single source pixel by a color's alpha and then by a destination pixel.
 *
 * This function performs the multiplication for a single point (pixel) using
 * only the alpha component of the color `c`. The source pixel `s` is multiplied
 * by the alpha of `c` (extracted and used directly, unlike `_op_mul_p_caa_dp`
 * which normalizes it for `MUL_256`). The result is then multiplied by the
 * destination pixel `*d` using `MUL4_SYM`.
 *
 * @param s The source pixel (DATA32). Example: `s = 0xFFRRGGBB`
 * @param m The mask value (DATA8). This parameter is unused.
 * @param c The color whose alpha component is used (DATA32). Example: `c = 0xAARRGGBB`
 * @param d Pointer to the destination pixel (DATA32), also used as output.
 *          Example: `*d = 0xFFRRGGBB`
 */
static void
_op_mul_pt_p_caa_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
	s = MUL_SYM(c >> 24, s); // Multiply source by color's alpha (raw value)
	*d = MUL4_SYM(s, *d);    // Multiply result by destination
}

/** @name Function aliases for point operations
 *  These macros define aliases for different combinations of source, color,
 *  and destination pixel properties for single point operations.
 *  They all point to either `_op_mul_pt_p_c_dp` or `_op_mul_pt_p_caa_dp`.
 * @{
 */
#define _op_mul_pt_pas_c_dp _op_mul_pt_p_c_dp
#define _op_mul_pt_pan_c_dp _op_mul_pt_p_c_dp
#define _op_mul_pt_p_can_dp _op_mul_pt_p_c_dp
#define _op_mul_pt_pas_can_dp _op_mul_pt_p_c_dp
#define _op_mul_pt_pan_can_dp _op_mul_pt_p_c_dp
#define _op_mul_pt_pas_caa_dp _op_mul_pt_p_caa_dp
#define _op_mul_pt_pan_caa_dp _op_mul_pt_p_caa_dp
/** @} */

/** @name Function aliases for point operations with destination alpha no-op
 *  Similar to the above, but for point operations where destination alpha
 *  is handled as a no-op (dpan).
 * @{
 */
#define _op_mul_pt_p_c_dpan _op_mul_pt_p_c_dp
#define _op_mul_pt_pan_c_dpan _op_mul_pt_pan_c_dp
#define _op_mul_pt_pas_c_dpan _op_mul_pt_pas_c_dp
#define _op_mul_pt_p_can_dpan _op_mul_pt_p_can_dp
#define _op_mul_pt_pan_can_dpan _op_mul_pt_pan_can_dp
#define _op_mul_pt_pas_can_dpan _op_mul_pt_pas_can_dp
#define _op_mul_pt_p_caa_dpan _op_mul_pt_p_caa_dp
#define _op_mul_pt_pan_caa_dpan _op_mul_pt_pan_caa_dp
#define _op_mul_pt_pas_caa_dpan _op_mul_pt_pas_caa_dp
/** @} */

/**
 * @brief Initializes the function pointers for point-based multiplication operations.
 *
 * This function populates the `op_mul_pt_funcs` array with the appropriate
 * function pointers for various combinations of source, mask, color, and
 * destination properties for single pixel (point) operations. These are used
 * for C-based CPU implementations.
 * The array indices (SP, SM_N, SC, DP, CPU_C, etc.) represent different
 * pixel format and operation mode combinations.
 *
 * Example of array structure:
 * `op_mul_pt_funcs[source_props][mask_mode][source_color_props][dest_props][cpu_type]`
 * Each element is a function pointer of type `op_mul_pt_func`.
 */
static void
init_mul_pixel_color_pt_funcs_c(void)
{
   op_mul_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_mul_pt_p_c_dp;
   op_mul_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_mul_pt_pas_c_dp;
   op_mul_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_mul_pt_pan_c_dp;
   op_mul_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_mul_pt_p_can_dp;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_mul_pt_pas_can_dp;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_mul_pt_pan_can_dp;
   op_mul_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_mul_pt_p_caa_dp;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_mul_pt_pas_caa_dp;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_mul_pt_pan_caa_dp;

   op_mul_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_mul_pt_p_c_dpan;
   op_mul_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_mul_pt_pas_c_dpan;
   op_mul_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_mul_pt_pan_c_dpan;
   op_mul_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pt_p_can_dpan;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pt_pas_can_dpan;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_mul_pt_pan_can_dpan;
   op_mul_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pt_p_caa_dpan;
   op_mul_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pt_pas_caa_dpan;
   op_mul_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_mul_pt_pan_caa_dpan;
}
