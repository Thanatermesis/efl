/* blend mask x color -> dst */

#ifdef BUILD_SSE3

/**
 * @brief Blends a mask with a color and applies it to the destination.
 *
 * This function takes a mask, a color, and a destination buffer. It blends
 * the mask with the color and then blends the result with the destination.
 * SSE3 optimizations are used for performance.
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8, alpha values).
 *          Example: [m1, m2, m3, m4, ...] where m_i is an alpha value (0-255).
 * @param c The color to blend with the mask (DATA32, ARGB format).
 *          Example: 0xAARRGGBB.
 * @param d Pointer to the destination data buffer (array of DATA32, ARGB format).
 *          Example: [d1, d2, d3, d4, ...] where d_i is a pixel color.
 * @param l The number of pixels to process.
 */
static void
_op_blend_mas_c_dp_sse3(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         DATA32 a = *m;
         DATA32 mc = MUL_SYM(a, c);
         a = 256 - (mc >> 24);
         *d = mc + MUL_256(a, *d);
         m++;  d++; l--;
      },
      { /* A4OP */

         if ((m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 4; d += 4; l -= 4;
            continue;
         }

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i mc0 = mul_sym_sse3(m0, c_packed);
         __m128i  a0 = sub4_alpha_sse3(mc0);
         __m128i mul0 = mul_256_sse3(a0, d0);

         mul0 = _mm_add_epi32(mul0, mc0);

         _mm_store_si128((__m128i *)d, mul0);

         m += 4; d += 4; l -= 4;
      },
      { /* A8OP */

         if((m[7] | m[6] | m[5] | m[4] | m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 8; d += 8; l -= 8;
            continue;
         }

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i mc0 = mul_sym_sse3(m0, c_packed);
         __m128i a0  = sub4_alpha_sse3(mc0);
         __m128i mul0 = mul_256_sse3(a0, d0);

         mul0 = _mm_add_epi32(mc0, mul0);

         __m128i mc1 = mul_sym_sse3(m1, c_packed);
         __m128i a1  = sub4_alpha_sse3(mc1);
         __m128i mul1 = mul_256_sse3(a1, d1);

         mul1 = _mm_add_epi32(mc1, mul1);

         _mm_store_si128((__m128i *)d, mul0);
         _mm_store_si128((__m128i *)(d+4), mul1);

         m += 8; d += 8; l -= 8;
      })
}

/**
 * @brief Blends a mask with a color (considering alpha) and applies it to the destination.
 *
 * This function is similar to _op_blend_mas_c_dp_sse3, but it handles
 * cases where the mask alpha is 0 or 255 specifically for optimization.
 * It uses SSE3 instructions for accelerated processing.
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8, alpha values).
 *          Example: [m1, m2, m3, m4, ...] where m_i is an alpha value (0-255).
 * @param c The color to blend (DATA32, ARGB format).
 *          Example: 0xAARRGGBB.
 * @param d Pointer to the destination data buffer (array of DATA32, ARGB format).
 *          Example: [d1, d2, d3, d4, ...] where d_i is a pixel color.
 * @param l The number of pixels to process.
 */
static void
_op_blend_mas_can_dp_sse3(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {

   DATA32 alpha;

   const __m128i one = _mm_set_epi32(1, 1, 1, 1);
   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         alpha = *m;
         switch(alpha)
           {
           case 0:
              break;
           case 255:
              *d = c;
              break;
           default:
              alpha++;
              *d = INTERP_256(alpha, c, *d);
              break;
           }
         m++;  d++; l--;
      },
      { /* A4OP */

         if ((m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 4; d += 4; l -= 4;
            continue;
         }

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i zm0 = _mm_cmpeq_epi32(m0, _mm_setzero_si128());

         m0 = _mm_add_epi32(one, m0);

         __m128i r0 = interp4_256_sse3(m0, c_packed, d0);

         r0 = _mm_and_si128(~zm0, r0);
         d0 = _mm_and_si128(zm0, d0);

         d0 = _mm_add_epi32(r0, d0);

         _mm_store_si128((__m128i *)d, d0);

         m += 4; d += 4; l -= 4;
      },
      { /* A8OP */

         if ((m[7] | m[6] | m[5] | m[4] | m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 8; d += 8; l -= 8;
            continue;
         }

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i zm0 = _mm_cmpeq_epi32(m0, _mm_setzero_si128());
         __m128i zm1 = _mm_cmpeq_epi32(m1, _mm_setzero_si128());

         m0 = _mm_add_epi32(one, m0);
         m1 = _mm_add_epi32(one, m1);

         __m128i r0 = interp4_256_sse3(m0, c_packed, d0);
         __m128i r1 = interp4_256_sse3(m1, c_packed, d1);

         r0 = _mm_and_si128(~zm0, r0);
         d0 = _mm_and_si128(zm0, d0);

         r1 = _mm_and_si128(~zm1, r1);
         d1 = _mm_and_si128(zm1, d1);

         d0 = _mm_add_epi32(d0, r0);
         d1 = _mm_add_epi32(d1, r1);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         m += 8; d += 8; l -= 8;
      })
}

/** @brief Alias for _op_blend_mas_can_dp_sse3. Used when source color has no alpha. */
#define _op_blend_mas_cn_dp_sse3 _op_blend_mas_can_dp_sse3
/** @brief Alias for _op_blend_mas_c_dp_sse3. Used when source color has alpha. */
#define _op_blend_mas_caa_dp_sse3 _op_blend_mas_c_dp_sse3

/** @brief Alias for _op_blend_mas_c_dp_sse3. Used when destination has no alpha. */
#define _op_blend_mas_c_dpan_sse3 _op_blend_mas_c_dp_sse3
/** @brief Alias for _op_blend_mas_cn_dp_sse3. Used when destination has no alpha and source color has no alpha. */
#define _op_blend_mas_cn_dpan_sse3 _op_blend_mas_cn_dp_sse3
/** @brief Alias for _op_blend_mas_can_dp_sse3. Used when destination has no alpha and source color has alpha. */
#define _op_blend_mas_can_dpan_sse3 _op_blend_mas_can_dp_sse3
/** @brief Alias for _op_blend_mas_caa_dp_sse3. Used when destination has no alpha and source color has alpha and alpha. */
#define _op_blend_mas_caa_dpan_sse3 _op_blend_mas_caa_dp_sse3

/**
 * @brief Initializes the span blending functions for mask and color operations using SSE3.
 *
 * This function assigns the appropriate SSE3-optimized blending functions
 * to the global function pointer array `op_blend_span_funcs`. These functions
 * are used for blending a span of pixels.
 */
static void
init_blend_mask_color_span_funcs_sse3(void)
{
// FIXME: BUGGY BUGGY Core i5 750 (32bit), 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4), ello (text and rectangle)
//   op_blend_span_funcs[SP_N][SM_AS][SC][DP][CPU_SSE3] = _op_blend_mas_c_dp_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_mas_cn_dp_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_SSE3] = _op_blend_mas_can_dp_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_SSE3] = _op_blend_mas_caa_dp_sse3;

// FIXME: BUGGY BUGGY Core i5 2500 (64bit), gcc version 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4), ello (text)
//   op_blend_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_SSE3] = _op_blend_mas_c_dpan_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_mas_cn_dpan_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_SSE3] = _op_blend_mas_can_dpan_sse3;
   op_blend_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_SSE3] = _op_blend_mas_caa_dpan_sse3;
}

/** @brief Placeholder for point blending (mask x color -> dest), currently NULL. */
#define _op_blend_pt_mas_c_dp_sse3 NULL
/** @brief Placeholder for point blending (mask x color (alpha no) -> dest), currently NULL. */
#define _op_blend_pt_mas_can_dp_sse3 NULL

/** @brief Alias for _op_blend_pt_mas_can_dp_sse3 for point blending. */
#define _op_blend_pt_mas_cn_dp_sse3 _op_blend_pt_mas_can_dp_sse3
/** @brief Alias for _op_blend_pt_mas_c_dp_sse3 for point blending. */
#define _op_blend_pt_mas_caa_dp_sse3 _op_blend_pt_mas_c_dp_sse3

/** @brief Alias for _op_blend_pt_mas_c_dp_sse3 for point blending with destination no alpha. */
#define _op_blend_pt_mas_c_dpan_sse3 _op_blend_pt_mas_c_dp_sse3
/** @brief Alias for _op_blend_pt_mas_cn_dp_sse3 for point blending with destination no alpha. */
#define _op_blend_pt_mas_cn_dpan_sse3 _op_blend_pt_mas_cn_dp_sse3
/** @brief Alias for _op_blend_pt_mas_can_dp_sse3 for point blending with destination no alpha. */
#define _op_blend_pt_mas_can_dpan_sse3 _op_blend_pt_mas_can_dp_sse3
/** @brief Alias for _op_blend_pt_mas_caa_dp_sse3 for point blending with destination no alpha. */
#define _op_blend_pt_mas_caa_dpan_sse3 _op_blend_pt_mas_caa_dp_sse3

/**
 * @brief Initializes the point blending functions for mask and color operations using SSE3.
 *
 * This function assigns the appropriate SSE3-optimized blending functions
 * (or NULL if not implemented) to the global function pointer array `op_blend_pt_funcs`.
 * These functions are used for blending a single point (pixel).
 */
static void
init_blend_mask_color_pt_funcs_sse3(void)
{
   op_blend_pt_funcs[SP_N][SM_AS][SC][DP][CPU_SSE3] = _op_blend_pt_mas_c_dp_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pt_mas_cn_dp_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_SSE3] = _op_blend_pt_mas_can_dp_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_SSE3] = _op_blend_pt_mas_caa_dp_sse3;

   op_blend_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_SSE3] = _op_blend_pt_mas_c_dpan_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pt_mas_cn_dpan_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pt_mas_can_dpan_sse3;
   op_blend_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pt_mas_caa_dpan_sse3;
}

/*-----*/

/* blend_rel mask x color --> dst */

/**
 * @brief Blends a mask with a color and applies it to the destination, relative to destination alpha.
 *
 * This function performs a blend operation where the source (mask * color)
 * is blended with the destination, and the operation is relative to the
 * destination's alpha. SSE3 optimizations are used.
 *
 * @param s Source data pointer (unused).
 * @param m Pointer to the mask data (array of DATA8, alpha values).
 *          Example: [m1, m2, m3, m4, ...] where m_i is an alpha value (0-255).
 * @param c The color to blend with the mask (DATA32, ARGB format).
 *          Example: 0xAARRGGBB.
 * @param d Pointer to the destination data buffer (array of DATA32, ARGB format).
 *          Example: [d1, d2, d3, d4, ...] where d_i is a pixel color.
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_mas_c_dp_sse3(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         DATA32 mc = MUL_SYM(*m, c);
         int alpha = 256 - (mc >> 24);
         *d = MUL_SYM(*d >> 24, mc) + MUL_256(alpha, *d);
         d++; m++; l--;
      },
      { /* A4OP */

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *) d);

         __m128i mc0 = mul_sym_sse3(m0, c_packed);
         __m128i a0 = sub4_alpha_sse3(mc0);

         __m128i d0_sym = mul_sym_sse3(_mm_srli_epi32(d0, 24), mc0);
         d0 = mul_256_sse3(a0, d0);

         d0 = _mm_add_epi32(d0, d0_sym);

         _mm_store_si128((__m128i *)d, d0);

         d += 4; m += 4; l -= 4;
      },
      { /* A8OP */

         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i mc0 = mul_sym_sse3(m0, c_packed);
         __m128i mc1 = mul_sym_sse3(m1, c_packed);

         __m128i a0 = sub4_alpha_sse3(mc0);
         __m128i a1 = sub4_alpha_sse3(mc1);

         __m128i d0_sym = mul_sym_sse3(_mm_srli_epi32(d0, 24), mc0);
         __m128i d1_sym = mul_sym_sse3(_mm_srli_epi32(d1, 24), mc1);

         d0 = mul_256_sse3(a0, d0);
         d1 = mul_256_sse3(a1, d1);

         d0 = _mm_add_epi32(d0, d0_sym);
         d1 = _mm_add_epi32(d1, d1_sym);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         d += 8; m += 8; l -= 8;
      })
}

/** @brief Alias for _op_blend_rel_mas_c_dp_sse3. Used when source color has no alpha. */
#define _op_blend_rel_mas_cn_dp_sse3 _op_blend_rel_mas_c_dp_sse3
/** @brief Alias for _op_blend_rel_mas_c_dp_sse3. Used when source color has alpha. */
#define _op_blend_rel_mas_can_dp_sse3 _op_blend_rel_mas_c_dp_sse3
/** @brief Alias for _op_blend_rel_mas_c_dp_sse3. Used when source color has alpha and alpha. */
#define _op_blend_rel_mas_caa_dp_sse3 _op_blend_rel_mas_c_dp_sse3

/** @brief Alias for _op_blend_mas_c_dpan_sse3 for relative blending. */
#define _op_blend_rel_mas_c_dpan_sse3 _op_blend_mas_c_dpan_sse3
/** @brief Alias for _op_blend_mas_cn_dpan_sse3 for relative blending. */
#define _op_blend_rel_mas_cn_dpan_sse3 _op_blend_mas_cn_dpan_sse3
/** @brief Alias for _op_blend_mas_can_dpan_sse3 for relative blending. */
#define _op_blend_rel_mas_can_dpan_sse3 _op_blend_mas_can_dpan_sse3
/** @brief Alias for _op_blend_mas_caa_dpan_sse3 for relative blending. */
#define _op_blend_rel_mas_caa_dpan_sse3 _op_blend_mas_caa_dpan_sse3

/**
 * @brief Initializes the relative span blending functions for mask and color operations using SSE3.
 *
 * This function assigns the appropriate SSE3-optimized relative blending functions
 * to the global function pointer array `op_blend_rel_span_funcs`. These functions
 * are used for blending a span of pixels with relative alpha consideration.
 */
static void
init_blend_rel_mask_color_span_funcs_sse3(void)
{
   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_SSE3] = _op_blend_rel_mas_c_dp_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_mas_can_dp_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_SSE3] = _op_blend_rel_mas_can_dp_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_SSE3] = _op_blend_rel_mas_caa_dp_sse3;

   op_blend_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_SSE3] = _op_blend_rel_mas_c_dpan_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_mas_cn_dpan_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_mas_can_dpan_sse3;
   op_blend_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_mas_caa_dpan_sse3;
}

/** @brief Placeholder for relative point blending (mask x color -> dest), currently NULL. */
#define _op_blend_rel_pt_mas_c_dp_sse3 NULL

/** @brief Alias for _op_blend_rel_pt_mas_c_dp_sse3 for relative point blending. */
#define _op_blend_rel_pt_mas_cn_dp_sse3 _op_blend_rel_pt_mas_c_dp_sse3
/** @brief Alias for _op_blend_rel_pt_mas_c_dp_sse3 for relative point blending. */
#define _op_blend_rel_pt_mas_can_dp_sse3 _op_blend_rel_pt_mas_c_dp_sse3
/** @brief Alias for _op_blend_rel_pt_mas_c_dp_sse3 for relative point blending. */
#define _op_blend_rel_pt_mas_caa_dp_sse3 _op_blend_rel_pt_mas_c_dp_sse3

/** @brief Alias for _op_blend_pt_mas_c_dpan_sse3 for relative point blending with destination no alpha. */
#define _op_blend_rel_pt_mas_c_dpan_sse3 _op_blend_pt_mas_c_dpan_sse3
/** @brief Alias for _op_blend_pt_mas_cn_dpan_sse3 for relative point blending with destination no alpha. */
#define _op_blend_rel_pt_mas_cn_dpan_sse3 _op_blend_pt_mas_cn_dpan_sse3
/** @brief Alias for _op_blend_pt_mas_can_dpan_sse3 for relative point blending with destination no alpha. */
#define _op_blend_rel_pt_mas_can_dpan_sse3 _op_blend_pt_mas_can_dpan_sse3
/** @brief Alias for _op_blend_pt_mas_caa_dpan_sse3 for relative point blending with destination no alpha. */
#define _op_blend_rel_pt_mas_caa_dpan_sse3 _op_blend_pt_mas_caa_dpan_sse3

/**
 * @brief Initializes the relative point blending functions for mask and color operations using SSE3.
 *
 * This function assigns the appropriate SSE3-optimized relative blending functions
 * (or NULL if not implemented) to the global function pointer array `op_blend_rel_pt_funcs`.
 * These functions are used for blending a single point (pixel) with relative alpha consideration.
 */
static void
init_blend_rel_mask_color_pt_funcs_sse3(void)
{
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_SSE3] = _op_blend_rel_pt_mas_c_dp_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pt_mas_cn_dp_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pt_mas_can_dp_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pt_mas_caa_dp_sse3;

   op_blend_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pt_mas_c_dpan_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pt_mas_cn_dpan_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pt_mas_can_dpan_sse3;
   op_blend_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pt_mas_caa_dpan_sse3;
}

#endif
