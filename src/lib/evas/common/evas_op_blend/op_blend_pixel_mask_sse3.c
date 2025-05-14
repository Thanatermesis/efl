/**
 * @file
 * @brief SSE3 optimized pixel blending operations with a mask.
 *
 * This file contains functions for blending a source pixel array with a
 * destination pixel array, using a mask. The operations are optimized
 * using SSE3 intrinsics.
 *
 * The naming convention for functions generally follows:
 * _op_blend_[src_type]_[mask_type]_[dst_type]_[arch]
 * - p: pixel (source is data)
 * - pas: pixel + alpha (source has alpha)
 * - pan: pixel + alpha, no alpha in result (source has alpha, but alpha channel not written)
 * - mas: mask (mask is alpha source)
 * - dp: destination pixel
 * - dpan: destination pixel + alpha
 * - sse3: SSE3 optimized version
 *
 * Point (pt) functions operate on single pixels, while span functions operate on arrays.
 */

/* blend pixel x mask --> dst */

#ifdef BUILD_SSE3

// FIXME: These functions most likely don't perform the correct operation.
// Test them with masks and images.
#if 0
/**
 * @brief Blend source pixels with destination pixels using a mask (SSE3).
 * s + (d * (1 - m))
 * @param s Pointer to the source pixel data array (DATA32: ARGB).
 * @param m Pointer to the mask data array (DATA8: alpha values, 0-255).
 * @param c Color value (not directly used in this variant, but part of signature).
 * @param d Pointer to the destination pixel data array (DATA32: ARGB), modified in place.
 * @param l Number of pixels to process.
 */
static void
_op_blend_p_mas_dp_sse3(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {

   int alpha;

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         alpha = *m;
         c = MUL_SYM(alpha, *s);
         alpha = 256 - (c >> 24);
         *d = c + MUL_256(alpha, *d);
         m++;  s++;  d++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);

         __m128i c0 = mul_sym_sse3(m0, s0);
         __m128i a0 = sub4_alpha_sse3(c0);
         __m128i r0 = mul_256_sse3(a0, d0);

         r0 = _mm_add_epi32(r0, c0);

         _mm_store_si128((__m128i *)d, r0);

         m += 4; s += 4; d += 4; l -= 4;
      },
      { /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));
         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);

         __m128i c0 = mul_sym_sse3(m0, s0);
         __m128i c1 = mul_sym_sse3(m1, s1);

         __m128i a0 = sub4_alpha_sse3(c0);
         __m128i a1 = sub4_alpha_sse3(c1);

         __m128i r0 = mul_256_sse3(a0, d0);
         __m128i r1 = mul_256_sse3(a1, d1);

         r0 = _mm_add_epi32(r0, c0);
         r1 = _mm_add_epi32(r1, c1);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         m += 8; s += 8; d += 8; l -= 8;
      })
}

/**
 * @brief Blend source pixels (with alpha) with destination pixels using a mask (SSE3).
 * This function implements the Porter-Duff 'source-over' operation: (s * m) + (d * (1 - m))
 * where m is the mask value. If mask is 0, dst is unchanged. If mask is 255, dst = src.
 * Otherwise, dst = INTERP_256(mask+1, src, dst).
 * @param s Pointer to the source pixel data array (DATA32: ARGB).
 * @param m Pointer to the mask data array (DATA8: alpha values, 0-255).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data array (DATA32: ARGB), modified in place.
 * @param l Number of pixels to process.
 */
static void
_op_blend_pas_mas_dp_sse3(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {

   const __m128i ones = _mm_set_epi32(1, 1, 1, 1);
   int alpha;

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         alpha = *m;
         switch(alpha)
           {
           case 0:
              break;
           case 255:
              *d = *s;
              break;
           default:
              alpha++;
              *d = INTERP_256(alpha, *s, *d);
              break;
           }
         m++;  s++;  d++; l--;
      },
      {  /*A4OP */

         if ((m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 4; s += 4; d += 4; l -= 4;
            continue;
         }

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);

         __m128i zm0 = _mm_cmpeq_epi32(m0, _mm_setzero_si128());

          m0 = _mm_add_epi32(m0, ones);

         __m128i r0 = interp4_256_sse3(m0, s0, d0);

         r0 = _mm_and_si128(~zm0, r0);
         d0 = _mm_and_si128(zm0, d0);

         d0 = _mm_add_epi32(r0, d0);

         _mm_store_si128((__m128i *)d, d0);

         m += 4; s += 4; d += 4; l -= 4;
      },
      { /* A8OP */

         if ((m[7] | m[6] | m[5] | m[4] | m[3] | m[2] | m[1] | m[0]) == 0) {
            m += 8; s += 8; d += 8; l -= 8;
            continue;
         }

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));
         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);

         __m128i zm0 = _mm_cmpeq_epi32(m0, _mm_setzero_si128());
         __m128i zm1 = _mm_cmpeq_epi32(m1, _mm_setzero_si128());

         m0 = _mm_add_epi32(m0, ones);
         m1 = _mm_add_epi32(m1, ones);

         __m128i r0 = interp4_256_sse3(m0, s0, d0);
         __m128i r1 = interp4_256_sse3(m1, s1, d1);

         r0 = _mm_and_si128(~zm0, r0);
         d0 = _mm_and_si128(zm0, d0);

         r1 = _mm_and_si128(~zm1, r1);
         d1 = _mm_and_si128(zm1, d1);

         d0 = _mm_add_epi32(d0, r0);
         d1 = _mm_add_epi32(d1, r1);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         m += 8; s += 8; d += 8; l -= 8;
      })
}
#endif

// FIXME
#define _op_blend_p_mas_dp_sse3 NULL
#define _op_blend_pas_mas_dp_sse3 _op_blend_p_mas_dp_sse3
// -----

#define _op_blend_pan_mas_dp_sse3 _op_blend_pas_mas_dp_sse3

#define _op_blend_p_mas_dpan_sse3 _op_blend_p_mas_dp_sse3
#define _op_blend_pas_mas_dpan_sse3 _op_blend_pas_mas_dp_sse3
#define _op_blend_pan_mas_dpan_sse3 _op_blend_pan_mas_dp_sse3

/**
 * @brief Initializes the SSE3 span blending functions for pixel operations with a mask.
 * This function assigns the SSE3-optimized implementations to the global function
 * pointer table `op_blend_span_funcs`. These functions are used for blending
 * spans (arrays) of pixels.
 */
static void
init_blend_pixel_mask_span_funcs_sse3(void)
{
   op_blend_span_funcs[SP][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_p_mas_dp_sse3;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pas_mas_dp_sse3;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pan_mas_dp_sse3;

   op_blend_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_p_mas_dpan_sse3;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pas_mas_dpan_sse3;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pan_mas_dpan_sse3;
}

// FIXME
#define _op_blend_pt_p_mas_dp_sse3 NULL
#define _op_blend_pt_pan_mas_dp_sse3 NULL
// -----

#define _op_blend_pt_pas_mas_dp_sse3 _op_blend_pt_p_mas_dp_sse3

#define _op_blend_pt_p_mas_dpan_sse3 _op_blend_pt_p_mas_dp_sse3
#define _op_blend_pt_pas_mas_dpan_sse3 _op_blend_pt_pas_mas_dp_sse3
#define _op_blend_pt_pan_mas_dpan_sse3 _op_blend_pt_pan_mas_dp_sse3

/**
 * @brief Initializes the SSE3 point blending functions for pixel operations with a mask.
 * This function assigns the SSE3-optimized implementations (or NULL if not implemented)
 * to the global function pointer table `op_blend_pt_funcs`. These functions are
 * used for blending single pixels.
 */
static void
init_blend_pixel_mask_pt_funcs_sse3(void)
{
   op_blend_pt_funcs[SP][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pt_p_mas_dp_sse3;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pt_pas_mas_dp_sse3;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_pt_pan_mas_dp_sse3;

   op_blend_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pt_p_mas_dpan_sse3;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pt_pas_mas_dpan_sse3;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_pt_pan_mas_dpan_sse3;
}

/*-----*/

/* blend_rel pixel x mask -> dst */

#if 0
/**
 * @brief Relative blend of source pixels with destination pixels using a mask (SSE3).
 * This operation seems to be a variation of blending where the source contribution
 * is modulated by the destination's alpha before combining.
 * dst = (c * (d >> 24)) + (d * (1 - (c >> 24))) where c = (s * m)
 * @param s Pointer to the source pixel data array (DATA32: ARGB).
 * @param m Pointer to the mask data array (DATA8: alpha values, 0-255).
 * @param c Color value (not directly used in this variant, but part of signature).
 * @param d Pointer to the destination pixel data array (DATA32: ARGB), modified in place.
 * @param l Number of pixels to process.
 */
static void
_op_blend_rel_p_mas_dp_sse3(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {

   int alpha;

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         c = MUL_SYM(*m, *s);
         alpha = 256 - (c >> 24);
         *d = MUL_SYM(*d >> 24, c) + MUL_256(alpha, *d);
         d++; m++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i c0 = mul_sym_sse3(m0, s0);
         __m128i a0 = sub4_alpha_sse3(c0);

         __m128i l0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), c0);
         __m128i r0 = mul_256_sse3(a0, d0);

         d0 = _mm_add_epi32(l0, r0);

         _mm_store_si128((__m128i *)d, d0);

         d += 4; m += 4; s += 4; l -= 4;
      },
      { /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i m0 = _mm_set_epi32(m[3], m[2], m[1], m[0]);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i m1 = _mm_set_epi32(m[7], m[6], m[5], m[4]);
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i c0 = mul_sym_sse3(m0, s0);
         __m128i c1 = mul_sym_sse3(m1, s1);

         __m128i a0 = sub4_alpha_sse3(c0);
         __m128i a1 = sub4_alpha_sse3(c1);

         __m128i l0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), c0);
         __m128i r0 = mul_256_sse3(a0, d0);

         __m128i l1 = mul_sym_sse3(_mm_srli_epi32(d1, 24), c1);
         __m128i r1 = mul_256_sse3(a1, d1);

         d0 =  _mm_add_epi32(l0, r0);
         d1 = _mm_add_epi32(l1, r1);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         d += 8; m += 8; s += 8; l -= 8;
      })
}
#else
// FIXME
#define _op_blend_rel_p_mas_dp_sse3 NULL
#endif

#define _op_blend_rel_pas_mas_dp_sse3 _op_blend_rel_p_mas_dp_sse3
#define _op_blend_rel_pan_mas_dp_sse3 _op_blend_rel_p_mas_dp_sse3

#define _op_blend_rel_p_mas_dpan_sse3 _op_blend_p_mas_dpan_sse3
#define _op_blend_rel_pas_mas_dpan_sse3 _op_blend_pas_mas_dpan_sse3
#define _op_blend_rel_pan_mas_dpan_sse3 _op_blend_pan_mas_dpan_sse3

/**
 * @brief Initializes the SSE3 span relative blending functions for pixel operations with a mask.
 * This function assigns the SSE3-optimized implementations (or NULL if not implemented)
 * to the global function pointer table `op_blend_rel_span_funcs`. These functions
 * are used for relative blending of spans (arrays) of pixels.
 */
static void
init_blend_rel_pixel_mask_span_funcs_sse3(void)
{
   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_p_mas_dp_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pas_mas_dp_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pan_mas_dp_sse3;

   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_p_mas_dpan_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pas_mas_dpan_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pan_mas_dpan_sse3;
}

#define _op_blend_rel_pt_p_mas_dp_sse3 NULL

#define _op_blend_rel_pt_pas_mas_dp_sse3 _op_blend_rel_pt_p_mas_dp_sse3
#define _op_blend_rel_pt_pan_mas_dp_sse3 _op_blend_rel_pt_p_mas_dp_sse3

#define _op_blend_rel_pt_p_mas_dpan_sse3 _op_blend_pt_p_mas_dpan_sse3
#define _op_blend_rel_pt_pas_mas_dpan_sse3 _op_blend_pt_pas_mas_dpan_sse3
#define _op_blend_rel_pt_pan_mas_dpan_sse3 _op_blend_pt_pan_mas_dpan_sse3

/**
 * @brief Initializes the SSE3 point relative blending functions for pixel operations with a mask.
 * This function assigns the SSE3-optimized implementations (or NULL if not implemented)
 * to the global function pointer table `op_blend_rel_pt_funcs`. These functions
 * are used for relative blending of single pixels.
 */
static void
init_blend_rel_pixel_mask_pt_funcs_sse3(void)
{
   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pt_p_mas_dp_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pt_pas_mas_dp_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_SSE3] = _op_blend_rel_pt_pan_mas_dp_sse3;

   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pt_p_mas_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pas_mas_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pan_mas_dpan_sse3;
}

#endif
