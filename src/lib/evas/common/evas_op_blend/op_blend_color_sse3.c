/**
 * @file op_blend_color_sse3.c
 * @brief SSE3 optimized functions for blending a solid color onto a destination buffer.
 *
 * These functions handle blending operations where the source is a single color
 * and the destination is a pixel buffer. Operations include simple blending
 * and relative blending, with and without anti-aliasing considerations.
 */

/* blend color -> dst */

#ifdef BUILD_SSE3

/**
 * @brief Blends a solid color onto a destination buffer using SSE3.
 *
 * This function takes a solid color `c` and blends it over `l` pixels
 * in the destination buffer `d`. The source alpha component of `c`
 * determines the blend strength.
 *
 * @param s Source data pointer (unused for solid color operations).
 * @param m Mask data pointer (unused in this version).
 * @param c The solid color to blend (in ARGB8888 format).
 *          Example: 0xFFRRGGBB (opaque red).
 * @param d Pointer to the destination pixel buffer (ARGB8888 format).
 *          Each element is a DATA32 representing a pixel.
 * @param l The number of pixels to process.
 */
static void
_op_blend_c_dp_sse3(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   DATA32 a = 256 - (c >> 24);

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);
   const __m128i a_packed = _mm_set_epi32(a, a, a, a);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         *d = c + MUL_256(a, *d);
         d++; l--;
      },
      { /* A4OP */

         __m128i d0 = _mm_load_si128((__m128i *)d);

         d0 = mul_256_sse3(a_packed, d0);
         d0 = _mm_add_epi32(d0, c_packed);

         _mm_store_si128((__m128i *)d, d0);

         d += 4; l -= 4;
      },
      { /* A8OP */

         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         d0 = mul_256_sse3(a_packed, d0);
         d1 = mul_256_sse3(a_packed, d1);

         d0 = _mm_add_epi32(d0, c_packed);
         d1 = _mm_add_epi32(d1, c_packed);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         d += 8; l -= 8;
      })
}

/** @brief Alias for _op_blend_c_dp_sse3, typically used for color with alpha + alpha destination. */
#define _op_blend_caa_dp_sse3 _op_blend_c_dp_sse3

/** @brief Alias for _op_blend_c_dp_sse3, typically used for color with destination alpha + no alpha. */
#define _op_blend_c_dpan_sse3 _op_blend_c_dp_sse3
/** @brief Alias for _op_blend_c_dpan_sse3, typically used for color with alpha + alpha destination + no alpha. */
#define _op_blend_caa_dpan_sse3 _op_blend_c_dpan_sse3

/**
 * @brief Initializes function pointers for SSE3 optimized solid color blend span operations.
 *
 * This function assigns the appropriate SSE3-accelerated blend functions
 * to the global function pointer table `op_blend_span_funcs`.
 * It handles cases for solid color blending with and without anti-aliasing,
 * and for destinations with or without an alpha channel.
 */
static void
init_blend_color_span_funcs_sse3(void)
{
// FIXME: BUGGY BUGGY Core i5 750 (32bit), 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4), ello (text and rectangle)
//   op_blend_span_funcs[SP_N][SM_N][SC][DP][CPU_SSE3] = _op_blend_c_dp_sse3;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_caa_dp_sse3;

// FIXME: BUGGY BUGGY Core i5 750 (32bit), 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4), ello (text and rectangle)
//   op_blend_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_c_dpan_sse3;
   op_blend_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_caa_dpan_sse3;
}

/** @brief Placeholder for SSE3 point blend (color, destination has alpha). Currently NULL. */
#define _op_blend_pt_c_dp_sse3 NULL
/** @brief Alias for _op_blend_pt_c_dp_sse3 (color with alpha, destination has alpha). */
#define _op_blend_pt_caa_dp_sse3 _op_blend_pt_c_dp_sse3

/** @brief Alias for _op_blend_pt_c_dp_sse3 (color, destination has no alpha). */
#define _op_blend_pt_c_dpan_sse3 _op_blend_pt_c_dp_sse3
/** @brief Alias for _op_blend_pt_c_dpan_sse3 (color with alpha, destination has no alpha). */
#define _op_blend_pt_caa_dpan_sse3 _op_blend_pt_c_dpan_sse3

/** @brief Alias for _op_blend_pt_c_dp_sse3 (color, destination has alpha, source is solid). */
#define _op_blend_pt_c_dpas_sse3 _op_blend_pt_c_dp_sse3
/** @brief Alias for _op_blend_pt_c_dp_sse3 (color with alpha, destination has alpha, source is solid). */
#define _op_blend_pt_caa_dpas_sse3 _op_blend_pt_c_dp_sse3

/**
 * @brief Initializes function pointers for SSE3 optimized solid color blend point operations.
 *
 * This function assigns the appropriate SSE3-accelerated blend functions
 * (or NULL if not implemented) to the global function pointer table `op_blend_pt_funcs`.
 * It handles various combinations of color, anti-aliasing, and destination alpha.
 */
static void
init_blend_color_pt_funcs_sse3(void)
{
   op_blend_pt_funcs[SP_N][SM_N][SC][DP][CPU_SSE3] = _op_blend_pt_c_dp_sse3;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pt_caa_dp_sse3;

   op_blend_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pt_c_dpan_sse3;
   op_blend_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pt_caa_dpan_sse3;
}


/*-----*/

/* blend_rel color -> dst */

/**
 * @brief Blends a solid color onto a destination buffer using relative alpha blending with SSE3.
 *
 * This function performs a "relative" blend, where the source color `c` is
 * blended with the destination `d` based on the destination's alpha.
 * The formula is effectively: `output = (c * Da) + (d * (1 - Sa))`,
 * where `Sa` is source alpha and `Da` is destination alpha.
 *
 * @param s Source data pointer (unused for solid color operations).
 * @param m Mask data pointer (unused in this version).
 * @param c The solid color to blend (in ARGB8888 format).
 *          Example: 0xFFRRGGBB (opaque red).
 * @param d Pointer to the destination pixel buffer (ARGB8888 format).
 *          Each element is a DATA32 representing a pixel.
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_c_dp_sse3(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   int alpha = 256 - (c >> 24);

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);
   const __m128i alpha_packed = _mm_set_epi32(alpha, alpha, alpha, alpha);

   LOOP_ALIGNED_U1_A48(d, l,
      {  /* UOP */

         *d = MUL_SYM(*d >> 24, c) + MUL_256(alpha, *d);
         d++; l--;
      },
      { /* A4OP */

         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i mul0 = mul_256_sse3(alpha_packed, d0);
         __m128i sym0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), c_packed);

         d0 = _mm_add_epi32(mul0, sym0);

         _mm_store_si128((__m128i *)d, d0);

         d += 4; l -= 4;
      },
      { /* A8OP */

         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i mul0 = mul_256_sse3(alpha_packed, d0);
         __m128i mul1 = mul_256_sse3(alpha_packed, d1);

         __m128i sym0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), c_packed);
         __m128i sym1 = mul_sym_sse3(_mm_srli_epi32(d1, 24), c_packed);

         d0 = _mm_add_epi32(mul0, sym0);
         d1 = _mm_add_epi32(mul1, sym1);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         d += 8; l -= 8;
      })
}

/** @brief Alias for _op_blend_rel_c_dp_sse3, for relative blend with color alpha + alpha destination. */
#define _op_blend_rel_caa_dp_sse3 _op_blend_rel_c_dp_sse3
/** @brief Alias for _op_blend_c_dpan_sse3, for relative blend with color + destination alpha + no alpha. Note: Uses non-relative base function. */
#define _op_blend_rel_c_dpan_sse3 _op_blend_c_dpan_sse3
/** @brief Alias for _op_blend_caa_dpan_sse3, for relative blend with color alpha + alpha destination + no alpha. Note: Uses non-relative base function. */
#define _op_blend_rel_caa_dpan_sse3 _op_blend_caa_dpan_sse3

/**
 * @brief Initializes function pointers for SSE3 optimized relative solid color blend span operations.
 *
 * This function assigns the appropriate SSE3-accelerated relative blend functions
 * to the global function pointer table `op_blend_rel_span_funcs`.
 * It covers cases for solid color blending with and without anti-aliasing,
 * and for destinations with or without an alpha channel, using relative blending logic.
 */
static void
init_blend_rel_color_span_funcs_sse3(void)
{
   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_c_dp_sse3;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_caa_dp_sse3;

   op_blend_rel_span_funcs[SP_N][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_c_dpan_sse3;
   op_blend_rel_span_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_caa_dpan_sse3;
}

/** @brief Placeholder for SSE3 relative point blend (color, destination has alpha). Currently NULL. */
#define _op_blend_rel_pt_c_dp_sse3 NULL
/** @brief Alias for _op_blend_rel_pt_c_dp_sse3 (relative, color with alpha, destination has alpha). */
#define _op_blend_rel_pt_caa_dp_sse3 _op_blend_rel_pt_c_dp_sse3

/** @brief Alias for _op_blend_pt_c_dpan_sse3 (relative, color, destination has no alpha). Note: Uses non-relative base macro. */
#define _op_blend_rel_pt_c_dpan_sse3 _op_blend_pt_c_dpan_sse3
/** @brief Alias for _op_blend_pt_caa_dpan_sse3 (relative, color with alpha, destination has no alpha). Note: Uses non-relative base macro. */
#define _op_blend_rel_pt_caa_dpan_sse3 _op_blend_pt_caa_dpan_sse3

/**
 * @brief Initializes function pointers for SSE3 optimized relative solid color blend point operations.
 *
 * This function assigns the appropriate SSE3-accelerated relative blend functions
 * (or NULL if not implemented) to the global function pointer table `op_blend_rel_pt_funcs`.
 * It handles various combinations of color, anti-aliasing, and destination alpha using relative blending.
 */
static void
init_blend_rel_color_pt_funcs_sse3(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pt_c_dp_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pt_caa_dp_sse3;

   op_blend_rel_pt_funcs[SP_N][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pt_c_dpan_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pt_caa_dpan_sse3;
}

#endif
