/* copy pixel x color --> dst */

/**
 * @brief Copies a span of pixels, multiplying each source pixel by a color.
 *
 * This function takes a source span of pixels, a color, and a destination span.
 * It multiplies each source pixel by the given color and stores the result in
 * the corresponding destination pixel. The operation is `*d = MUL4_SYM(c, *s)`.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Example: `s[0] = 0xAARRGGBB` (Alpha, Red, Green, Blue)
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color to multiply with (DATA32).
 *          Example: `c = 0xFFFFFFFF` (White, full opacity)
 * @param d Pointer to the destination pixel data (array of DATA32).
 *          Example: `d[0]` will store the result of `MUL4_SYM(c, s[0])`
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_copy_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL4_SYM(c, *s);
                        d++;
                        s++;
                     });
}

/**
 * @brief Copies a span of pixels, multiplying each source pixel by the alpha component of a color.
 *
 * This function is similar to _op_copy_p_c_dp, but it uses only the alpha
 * component of the color `c` for multiplication. The operation is `*d = MUL_256( (1 + (c >> 24)), *s)`.
 * This is typically used when the color `c` itself is an alpha mask.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color whose alpha component is used for multiplication (DATA32).
 *          The alpha component is extracted as `1 + (c >> 24)`.
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_copy_p_caa_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   c = 1 + (c >> 24); /* Pre-calculate (1 + alpha_of_c) for MUL_256 */
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL_256(c, *s);
                        d++;
                        s++;
                     });
}

/* Aliases for span copy functions. These macros map various source/destination
 * pixel format combinations and color alpha modes to the core implementation
 * functions (_op_copy_p_c_dp and _op_copy_p_caa_dp).
 * For example, _op_copy_pas_c_dp (source pixel has alpha, color has no alpha)
 * is an alias for _op_copy_p_c_dp.
 */
#define _op_copy_pas_c_dp _op_copy_p_c_dp
#define _op_copy_pan_c_dp _op_copy_p_c_dp
#define _op_copy_p_can_dp _op_copy_p_c_dp
#define _op_copy_pas_can_dp _op_copy_p_can_dp
#define _op_copy_pan_can_dp _op_copy_p_c_dp
#define _op_copy_pas_caa_dp _op_copy_p_caa_dp
#define _op_copy_pan_caa_dp _op_copy_p_caa_dp

/* Aliases for span copy functions where destination has no alpha (dpan). */
#define _op_copy_p_c_dpan _op_copy_p_c_dp
#define _op_copy_pas_c_dpan _op_copy_pas_c_dp
#define _op_copy_pan_c_dpan _op_copy_pan_c_dp
#define _op_copy_p_can_dpan _op_copy_p_can_dp
#define _op_copy_pas_can_dpan _op_copy_pas_can_dp
#define _op_copy_pan_can_dpan _op_copy_pan_can_dp
#define _op_copy_p_caa_dpan _op_copy_p_caa_dp
#define _op_copy_pas_caa_dpan _op_copy_pas_caa_dp
#define _op_copy_pan_caa_dpan _op_copy_pan_caa_dp

/**
 * @brief Initializes the function pointers for span copy operations with color multiplication.
 *
 * This function populates the `op_copy_span_funcs` array with the appropriate
 * span copy functions (_op_copy_p_c_dp, _op_copy_p_caa_dp, and their aliases)
 * for different combinations of source/destination pixel formats and color alpha modes.
 * These are for CPU-specific (CPU_C) implementations.
 */
static void
init_copy_pixel_color_span_funcs_c(void)
{
   op_copy_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_copy_p_c_dp;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_copy_pas_c_dp;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_copy_pan_c_dp;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_copy_p_can_dp;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_copy_pas_can_dp;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_copy_pan_can_dp;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_copy_p_caa_dp;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_copy_pas_caa_dp;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_copy_pan_caa_dp;

   op_copy_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_copy_p_c_dpan;
   op_copy_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_copy_pas_c_dpan;
   op_copy_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_copy_pan_c_dpan;
   op_copy_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_p_can_dpan;
   op_copy_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pas_can_dpan;
   op_copy_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pan_can_dpan;
   op_copy_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_p_caa_dpan;
   op_copy_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pas_caa_dpan;
   op_copy_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pan_caa_dpan;
}

/**
 * @brief Copies a single pixel, multiplying the source pixel by a color.
 *
 * This function takes a source pixel, a color, and a destination pixel pointer.
 * It multiplies the source pixel by the given color and stores the result in
 * the destination pixel. The operation is `*d = MUL4_SYM(c, s)`.
 *
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this function).
 * @param c The color to multiply with (DATA32).
 * @param d Pointer to the destination pixel data (DATA32).
 */
static void
_op_copy_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   *d = MUL4_SYM(c, s);
}

/**
 * @brief Copies a single pixel, multiplying the source pixel by the alpha component of a color.
 *
 * Similar to _op_copy_pt_p_c_dp, but uses only the alpha component of `c`
 * for multiplication. The operation is `*d = MUL_SYM((c >> 24), s)`.
 * Note: Unlike _op_copy_p_caa_dp, this uses `(c >> 24)` directly, not `1 + (c >> 24)`.
 *
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this function).
 * @param c The color whose alpha component is used for multiplication (DATA32).
 * @param d Pointer to the destination pixel data (DATA32).
 */
static void
_op_copy_pt_p_caa_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   *d = MUL_SYM(c >> 24, s); /* Multiply by alpha of c */
}

/* Aliases for point copy functions. These macros map various source/destination
 * pixel format combinations and color alpha modes to the core implementation
 * functions (_op_copy_pt_p_c_dp and _op_copy_pt_p_caa_dp).
 */
#define _op_copy_pt_p_can_dp _op_copy_pt_p_c_dp
#define _op_copy_pt_pan_c_dp _op_copy_pt_p_c_dp
#define _op_copy_pt_pan_can_dp _op_copy_pt_p_c_dp
#define _op_copy_pt_pan_caa_dp _op_copy_pt_p_caa_dp
#define _op_copy_pt_pas_c_dp _op_copy_pt_p_c_dp
#define _op_copy_pt_pas_can_dp _op_copy_pt_p_can_dp
#define _op_copy_pt_pas_caa_dp _op_copy_pt_p_caa_dp

/* Aliases for point copy functions where destination has no alpha (dpan). */
#define _op_copy_pt_p_c_dpan _op_copy_pt_p_c_dp
#define _op_copy_pt_pas_c_dpan _op_copy_pt_pas_c_dp
#define _op_copy_pt_pan_c_dpan _op_copy_pt_pan_c_dp
#define _op_copy_pt_p_can_dpan _op_copy_pt_p_can_dp
#define _op_copy_pt_pas_can_dpan _op_copy_pt_pas_can_dp
#define _op_copy_pt_pan_can_dpan _op_copy_pt_pan_can_dp
#define _op_copy_pt_p_caa_dpan _op_copy_pt_p_caa_dp
#define _op_copy_pt_pas_caa_dpan _op_copy_pt_pas_caa_dp
#define _op_copy_pt_pan_caa_dpan _op_copy_pt_pan_caa_dp

/**
 * @brief Initializes the function pointers for single point copy operations with color multiplication.
 *
 * This function populates the `op_copy_pt_funcs` array with the appropriate
 * point copy functions for different combinations of pixel formats and color modes.
 * These are for CPU-specific (CPU_C) implementations.
 */
static void
init_copy_pixel_color_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_copy_pt_p_c_dp;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_copy_pt_pas_c_dp;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_copy_pt_pan_c_dp;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_copy_pt_p_can_dp;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_copy_pt_pas_can_dp;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_copy_pt_pan_can_dp;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_copy_pt_p_caa_dp;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_copy_pt_pas_caa_dp;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_copy_pt_pan_caa_dp;

   op_copy_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_copy_pt_p_c_dpan;
   op_copy_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_copy_pt_pas_c_dpan;
   op_copy_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_copy_pt_pan_c_dpan;
   op_copy_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pt_p_can_dpan;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pt_pas_can_dpan;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_pt_pan_can_dpan;
   op_copy_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pt_p_caa_dpan;
   op_copy_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pt_pas_caa_dpan;
   op_copy_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_pt_pan_caa_dpan;
}

/*-----*/

/* copy_rel pixel x color --> dst */

/**
 * @brief Copies a span of pixels "relatively", multiplying source by color, then by destination alpha.
 *
 * This function performs a "relative" copy. First, the source pixel `*s` is
 * multiplied by the color `c`. Then, this intermediate result `cs` is
 * multiplied by the alpha component of the original destination pixel `*d`.
 * The final result is stored back in `*d`.
 * Operation: `cs = MUL4_SYM(c, *s); *d = MUL_SYM((*d >> 24), cs);`
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused in this function).
 * @param c The color to multiply with the source pixel (DATA32).
 * @param d Pointer to the destination pixel data (array of DATA32). This is read then overwritten.
 * @param l The number of pixels to process (length of the span).
 */
static void
_op_copy_rel_p_c_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        DATA32 cs = MUL4_SYM(c, *s);
                        *d = MUL_SYM(*d >> 24, cs);
                        d++;
                        s++;
                     });
}

/* Aliases for relative span copy functions. These macros map various
 * source/destination pixel format combinations and color alpha modes to the
 * core implementation function _op_copy_rel_p_c_dp.
 * Note that _op_copy_rel_p_caa_dp (color has alpha only) also maps to
 * _op_copy_rel_p_c_dp, implying the full color `c` is used in the first
 * multiplication step, and the "caa" (color alpha only) aspect might be
 * handled by the value of `c` itself or is not specialized here.
 */
#define _op_copy_rel_pas_c_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_pan_c_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_p_can_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_pas_can_dp _op_copy_rel_pas_c_dp
#define _op_copy_rel_pan_can_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_p_caa_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_pas_caa_dp _op_copy_rel_p_c_dp
#define _op_copy_rel_pan_caa_dp _op_copy_rel_p_c_dp

/* Aliases for relative span copy functions where destination has no alpha (dpan).
 * These map to the _op_copy_p_c_dpan family, which are themselves aliases
 * to _op_copy_p_c_dp. This suggests that for relative operations, the
 * destination alpha handling (`*d >> 24`) is always performed, and if the
 * destination format is _dpan (no alpha), this might result in multiplying by 0xFF.
 */
#define _op_copy_rel_p_c_dpan _op_copy_p_c_dpan
#define _op_copy_rel_pas_c_dpan _op_copy_pas_c_dpan
#define _op_copy_rel_pan_c_dpan _op_copy_pan_c_dpan
#define _op_copy_rel_p_can_dpan _op_copy_p_can_dpan
#define _op_copy_rel_pas_can_dpan _op_copy_pas_can_dpan
#define _op_copy_rel_pan_can_dpan _op_copy_pan_can_dpan
#define _op_copy_rel_p_caa_dpan _op_copy_p_caa_dpan
#define _op_copy_rel_pas_caa_dpan _op_copy_pas_caa_dpan
#define _op_copy_rel_pan_caa_dpan _op_copy_pan_caa_dpan

/**
 * @brief Initializes function pointers for "relative" span copy operations with color.
 *
 * Populates `op_copy_rel_span_funcs` with appropriate functions for relative
 * span copy operations, considering different pixel formats and color modes.
 * These are for CPU-specific (CPU_C) implementations.
 */
static void
init_copy_rel_pixel_color_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC][DP][CPU_C] = _op_copy_rel_p_c_dp;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_copy_rel_pas_c_dp;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_copy_rel_pan_c_dp;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_p_can_dp;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pas_can_dp;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pan_can_dp;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_p_caa_dp;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pas_caa_dp;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pan_caa_dp;

   op_copy_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_p_c_dpan;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pas_c_dpan;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pan_c_dpan;
   op_copy_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_p_can_dpan;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pas_can_dpan;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pan_can_dpan;
   op_copy_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_p_caa_dpan;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pas_caa_dpan;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pan_caa_dpan;
}

/**
 * @brief Copies a single pixel "relatively", multiplying source by color, then by destination alpha.
 *
 * This function performs a "relative" copy for a single point.
 * The source pixel `s` is multiplied by color `c`. This intermediate result
 * is then multiplied by the alpha component of the original destination pixel `*d`.
 * The final result is stored back in `*d`.
 * Operation: `tmp_s = MUL4_SYM(c, s); *d = MUL_SYM((*d >> 24), tmp_s);`
 *
 * @param s The source pixel data (DATA32).
 * @param m Mask data (unused in this function).
 * @param c The color to multiply with the source pixel (DATA32).
 * @param d Pointer to the destination pixel data (DATA32). This is read then overwritten.
 */
static void
_op_copy_rel_pt_p_c_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   *d = MUL_SYM(*d >> 24, s);
}

/* Aliases for relative point copy functions. These macros map various
 * source/destination pixel format combinations and color alpha modes to the
 * core implementation function _op_copy_rel_pt_p_c_dp.
 */
#define _op_copy_rel_pt_pas_c_dp _op_copy_rel_pt_p_c_dp
#define _op_copy_rel_pt_pan_c_dp _op_copy_rel_pt_p_c_dp
#define _op_copy_rel_pt_p_can_dp _op_copy_rel_pt_p_c_dp
#define _op_copy_rel_pt_pas_can_dp _op_copy_rel_pt_p_can_dp
#define _op_copy_rel_pt_pan_can_dp _op_copy_rel_pt_p_c_dp
#define _op_copy_rel_pt_p_caa_dp _op_copy_rel_pt_p_c_dp
#define _op_copy_rel_pt_pas_caa_dp _op_copy_rel_pt_p_caa_dp
#define _op_copy_rel_pt_pan_caa_dp _op_copy_rel_pt_p_caa_dp

/* Aliases for relative point copy functions where destination has no alpha (dpan).
 * These map to the _op_copy_pt_p_c_dpan family, which are themselves aliases
 * to _op_copy_pt_p_c_dp.
 */
#define _op_copy_rel_pt_p_c_dpan _op_copy_pt_p_c_dpan
#define _op_copy_rel_pt_pas_c_dpan _op_copy_pt_pas_c_dpan
#define _op_copy_rel_pt_pan_c_dpan _op_copy_pt_pan_c_dpan
#define _op_copy_rel_pt_p_can_dpan _op_copy_pt_p_can_dpan
#define _op_copy_rel_pt_pas_can_dpan _op_copy_pt_pas_can_dpan
#define _op_copy_rel_pt_pan_can_dpan _op_copy_pt_pan_can_dpan
#define _op_copy_rel_pt_p_caa_dpan _op_copy_pt_p_caa_dpan
#define _op_copy_rel_pt_pas_caa_dpan _op_copy_pt_pas_caa_dpan
#define _op_copy_rel_pt_pan_caa_dpan _op_copy_pt_pan_caa_dpan

/**
 * @brief Initializes function pointers for "relative" single point copy operations with color.
 *
 * Populates `op_copy_rel_pt_funcs` with appropriate functions for relative
 * point copy operations, considering different pixel formats and color modes.
 * These are for CPU-specific (CPU_C) implementations.
 */
static void
init_copy_rel_pixel_color_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC][DP][CPU_C] = _op_copy_rel_pt_p_c_dp;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_C] = _op_copy_rel_pt_pas_c_dp;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_C] = _op_copy_rel_pt_pan_c_dp;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pt_p_can_dp;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pt_pas_can_dp;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_C] = _op_copy_rel_pt_pan_can_dp;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pt_p_caa_dp;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pt_pas_caa_dp;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_C] = _op_copy_rel_pt_pan_caa_dp;

   op_copy_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pt_p_c_dpan;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pt_pas_c_dpan;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_C] = _op_copy_rel_pt_pan_c_dpan;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_p_can_dpan;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_pas_can_dpan;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_pan_can_dpan;
   op_copy_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_p_caa_dpan;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_pas_caa_dpan;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_pan_caa_dpan;
}
