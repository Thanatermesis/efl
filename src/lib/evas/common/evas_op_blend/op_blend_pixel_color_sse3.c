/* blend pixel x color --> dst */

#ifdef BUILD_SSE3

/**
 * @brief Blends source pixels ('s') multiplied by a color ('c') onto destination pixels ('d').
 *
 * This function implements the operation: d = s * c + d * (1 - (s * c)_alpha).
 * It processes 'l' pixels. Source pixel 's' is first multiplied by color 'c'.
 * The resulting alpha component is used to blend with the destination 'd'.
 * SSE3 optimizations are used for aligned data.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel, e.g., 0xAARRGGBB).
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value to blend with the source pixels (DATA32, e.g., 0xAARRGGBB).
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 * @param l The number of pixels to process.
 */
static void
_op_blend_p_c_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   DATA32 alpha;

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         DATA32 sc = MUL4_SYM(c, *s);
         alpha = 256 - (sc >> 24);
         *d = sc + MUL_256(alpha, *d);
         d++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i sc0 = mul4_sym_sse3(c_packed, s0);
         __m128i a0  = sub4_alpha_sse3(sc0);
         __m128i mul0 = mul_256_sse3(a0, d0);

         d0 = _mm_add_epi32(sc0, mul0);

         _mm_store_si128((__m128i *)d, d0);

         d += 4; s += 4; l -= 4;
      },
      { /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i sc0 = mul4_sym_sse3(c_packed, s0);
         __m128i sc1 = mul4_sym_sse3(c_packed, s1);

         __m128i a0 = sub4_alpha_sse3(sc0);
         __m128i a1 = sub4_alpha_sse3(sc1);

         __m128i mul0 = mul_256_sse3(a0, d0);
         __m128i mul1 = mul_256_sse3(a1, d1);

         d0 = _mm_add_epi32(sc0, mul0);
         d1 = _mm_add_epi32(sc1, mul1);

         _mm_store_si128((__m128i *)d, d0);
         _mm_store_si128((__m128i *)(d+4), d1);

         d += 8; s += 8; l -= 8;
      })
}

/**
 * @brief Blends source pixels ('s') multiplied by a color ('c') onto destination pixels ('d'),
 *        where the source pixel's alpha is ignored (treated as opaque) and the color 'c' provides its own alpha.
 *
 * This function implements the operation: d = c_alpha + s * c_rgb + d * (1 - c_alpha).
 * It processes 'l' pixels. The RGB components of 's' are multiplied by the RGB components of 'c'.
 * The alpha of 'c' is added, and then blended with 'd' using the alpha of 'c'.
 * SSE3 optimizations are used for aligned data.
 * 'pan' typically means "pixel alpha none" - source alpha is not used in the s*c part,
 * but the color 'c' itself has an alpha component that dictates the blend with 'd'.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel). Alpha component of 's' is ignored.
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value to blend with the source pixels (DATA32). Its alpha component (c_a) is used for blending with 'd'.
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 * @param l The number of pixels to process.
 */
static void
_op_blend_pan_c_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   DATA32 c_a = c & 0xFF000000;
   DATA32 alpha = 256 - (c >> 24);

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);
   const __m128i c_alpha = _mm_set_epi32(c_a, c_a, c_a, c_a);
   const __m128i a0 = _mm_set_epi32(alpha, alpha, alpha, alpha);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         *d = ((c & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
         d++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i r0 = _mm_add_epi32(mul3_sym_sse3(c_packed, s0),
            mul_256_sse3(a0, d0));

         r0 = _mm_add_epi32(r0, c_alpha);

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      { /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i r0 = _mm_add_epi32(mul3_sym_sse3(c_packed, s0),
            mul_256_sse3(a0, d0));

         __m128i r1 = _mm_add_epi32(mul3_sym_sse3(c_packed, s1),
            mul_256_sse3(a0, d1));

         r0 = _mm_add_epi32(r0, c_alpha);
         r1 = _mm_add_epi32(r1, c_alpha);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

/**
 * @brief Blends source pixels ('s') multiplied by a color ('c') onto destination pixels ('d'),
 *        where the color 'c's alpha is ignored (treated as opaque for the s*c multiplication)
 *        but the source pixel's alpha ('s'_alpha) is used for the final blend with 'd'.
 *
 * This function implements the operation: d = s_alpha + s_rgb * c_rgb + d * (1 - s_alpha).
 * It processes 'l' pixels. The RGB components of 's' are multiplied by RGB components of 'c'.
 * The alpha of 's' is added, and then blended with 'd' using the alpha of 's'.
 * SSE3 optimizations are used for aligned data.
 * 'can' typically means "color alpha none" - color 'c' is treated as if its alpha is full for the s*c part,
 * but the source pixel 's' alpha is used for the final blend.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel). Its alpha component is used for blending with 'd'.
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value to blend with the source pixels (DATA32). Its alpha component is ignored for the s*c multiplication.
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 * @param l The number of pixels to process.
 */
static void
_op_blend_p_can_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   int alpha;
   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         alpha = 256 - (*s >> 24);
         *d = ((*s & 0xff000000) + MUL3_SYM(c, *s)) + MUL_256(alpha, *d);
         d++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i a0 = sub4_alpha_sse3(s0);

         __m128i r0 = _mm_add_epi32(mul3_sym_sse3(c_packed, s0),
            mul_256_sse3(a0, d0));

         r0 = _mm_add_epi32(r0, _mm_and_si128(s0, A_MASK_SSE3));

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      {
         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i a0 = sub4_alpha_sse3(s0);
         __m128i a1 = sub4_alpha_sse3(s1);

         __m128i r0 = _mm_add_epi32(mul3_sym_sse3(c_packed, s0),
            mul_256_sse3(a0, d0));

         __m128i r1 = _mm_add_epi32(mul3_sym_sse3(c_packed, s1),
            mul_256_sse3(a1, d1));

         r0 = _mm_add_epi32(r0, _mm_and_si128(s0, A_MASK_SSE3));
         r1 = _mm_add_epi32(r1, _mm_and_si128(s1, A_MASK_SSE3));

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

/**
 * @brief Blends source pixels ('s') multiplied by a color ('c') onto destination pixels ('d'),
 *        where both the source pixel's alpha and the color's alpha are ignored (treated as opaque)
 *        for the s*c multiplication, resulting in a fully opaque result which replaces 'd'.
 *
 * This function implements the operation: d = 0xFF000000 + s_rgb * c_rgb.
 * It processes 'l' pixels. The RGB components of 's' are multiplied by RGB components of 'c',
 * and the result is made fully opaque. This effectively overwrites 'd'.
 * SSE3 optimizations are used for aligned data.
 * 'pan' (pixel alpha none) and 'can' (color alpha none) mean that alpha components from 's' and 'c'
 * are not used in the multiplication, and the result is forced to be opaque.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel). Alpha component is ignored.
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value to blend with the source pixels (DATA32). Alpha component is ignored.
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is overwritten.
 * @param l The number of pixels to process.
 */
static void
_op_blend_pan_can_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         *d++ = 0xff000000 + MUL3_SYM(c, *s);
         s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);

         __m128i r0 = mul3_sym_sse3(c_packed, s0);
         r0 = _mm_add_epi32(r0, A_MASK_SSE3);

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      { /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));

         __m128i r0 = mul3_sym_sse3(c_packed, s0);
         __m128i r1 = mul3_sym_sse3(c_packed, s1);

         r0 = _mm_add_epi32(r0, A_MASK_SSE3);
         r1 = _mm_add_epi32(r1, A_MASK_SSE3);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

/**
 * @brief Blends source pixels ('s') multiplied by a color factor ('c') onto destination pixels ('d'),
 *        using an additive alpha blending mode based on the color's alpha channel.
 *
 * This function implements an additive blend: d = s * (c_alpha/255) + d * (1 - (s * (c_alpha/255))_alpha).
 * The lower 8 bits of 'c' are used as an alpha factor (c_val = 1 + (c & 0xff)).
 * Source 's' is scaled by this factor. The resulting alpha is used to blend with 'd'.
 * SSE3 optimizations are used for aligned data.
 * 'caa' typically means "color alpha additive" - the color 'c' provides an alpha value that is used additively.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel).
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value, where (c & 0xff) is used as an alpha factor (0-255, treated as 1-256).
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 * @param l The number of pixels to process.
 */
static void
_op_blend_p_caa_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   int alpha;
   c = 1 + (c & 0xff); /* Extract alpha factor from color, scale to 1-256 range */
   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         DATA32 sc = MUL_256(c, *s);
         alpha = 256 - (sc >> 24);
         *d = sc + MUL_256(alpha, *d);
         d++;
         s++;
         l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128 ((__m128i *)d);

         __m128i sc0 = mul_256_sse3(c_packed, s0);
         __m128i a0 = sub4_alpha_sse3(sc0);

         __m128i r0 = _mm_add_epi32(mul_256_sse3(a0, d0), sc0);

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      {
         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i sc0 = mul_256_sse3(c_packed, s0);
         __m128i sc1 = mul_256_sse3(c_packed, s1);

         __m128i a0 = sub4_alpha_sse3(sc0);
         __m128i a1 = sub4_alpha_sse3(sc1);

         __m128i r0 = _mm_add_epi32(mul_256_sse3(a0, d0), sc0);
         __m128i r1 = _mm_add_epi32(mul_256_sse3(a1, d1), sc1);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

/**
 * @brief Interpolates source pixels ('s') and destination pixels ('d') using a color factor ('c').
 *        The source pixel's alpha is ignored (treated as opaque).
 *
 * This function implements the operation: d = INTERP_256(c_factor, s, d).
 * This is equivalent to: d = s * (c_factor/256) + d * (1 - c_factor/256).
 * The lower 8 bits of 'c' are used as an interpolation factor (c_factor = 1 + (c & 0xff)).
 * SSE3 optimizations are used for aligned data.
 * 'pan' (pixel alpha none) means source alpha is not used.
 * 'caa' (color alpha additive) means 'c' provides an alpha-like factor for interpolation.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel). Alpha component is ignored.
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value, where (c & 0xff) is used as an interpolation factor (0-255, treated as 1-256).
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 * @param l The number of pixels to process.
 */
static void
_op_blend_pan_caa_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   c = 1 + (c & 0xff); /* Extract interpolation factor from color, scale to 1-256 range */
   const __m128i c_packed = _mm_set_epi32(c, c, c,c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         *d = INTERP_256(c, *s, *d);
         d++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i r0 = interp4_256_sse3(c_packed, s0, d0);

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      {

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i r0 = interp4_256_sse3(c_packed, s0, d0);
         __m128i r1 = interp4_256_sse3(c_packed, s1, d1);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

#define _op_blend_pas_c_dp_sse3 _op_blend_p_c_dp_sse3
#define _op_blend_pas_can_dp_sse3 _op_blend_p_can_dp_sse3
#define _op_blend_pas_caa_dp_sse3 _op_blend_p_caa_dp_sse3

#define _op_blend_p_c_dpan_sse3 _op_blend_p_c_dp_sse3
#define _op_blend_pas_c_dpan_sse3 _op_blend_pas_c_dp_sse3
#define _op_blend_pan_c_dpan_sse3 _op_blend_pan_c_dp_sse3
#define _op_blend_p_can_dpan_sse3 _op_blend_p_can_dp_sse3
#define _op_blend_pas_can_dpan_sse3 _op_blend_pas_can_dp_sse3
#define _op_blend_pan_can_dpan_sse3 _op_blend_pan_can_dp_sse3
#define _op_blend_p_caa_dpan_sse3 _op_blend_p_caa_dp_sse3
#define _op_blend_pas_caa_dpan_sse3 _op_blend_pas_caa_dp_sse3
#define _op_blend_pan_caa_dpan_sse3 _op_blend_pan_caa_dp_sse3

/**
 * @brief Initializes the SSE3 optimized span blending functions for pixel x color operations.
 *
 * This function populates the `op_blend_span_funcs` table with pointers to the
 * SSE3-specific implementations for various combinations of source pixel properties (SP, SP_AS, SP_AN),
 * source mask (SM_N - none), source color properties (SC, SC_AN, SC_AA),
 * destination pixel properties (DP, DP_AN), and CPU features (CPU_SSE3).
 *
 * The defines above this function (e.g., _op_blend_pas_c_dp_sse3) often alias
 * more general functions when specific properties (like 'pas' - pixel alpha solid)
 * lead to simplifications handled by the general case or the SSE implementation.
 * Similarly, 'dpan' (destination pixel alpha none) variants often map to 'dp'
 * if the operation inherently makes the destination opaque or if destination alpha
 * isn't a factor in the SSE path.
 */
static void
init_blend_pixel_color_span_funcs_sse3(void)
{
   op_blend_span_funcs[SP][SM_N][SC][DP][CPU_SSE3] = _op_blend_p_c_dp_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP][CPU_SSE3] = _op_blend_pas_c_dp_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP][CPU_SSE3] = _op_blend_pan_c_dp_sse3;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_p_can_dp_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_pas_can_dp_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_pan_can_dp_sse3;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_p_caa_dp_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pas_caa_dp_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pan_caa_dp_sse3;

   op_blend_span_funcs[SP][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_p_c_dpan_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pas_c_dpan_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pan_c_dpan_sse3;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_p_can_dpan_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pas_can_dpan_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pan_can_dpan_sse3;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_p_caa_dpan_sse3;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pas_caa_dpan_sse3;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pan_caa_dpan_sse3;
}

#define _op_blend_pt_p_c_dp_sse3 NULL

#define _op_blend_pt_pas_c_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pan_c_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_p_can_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pas_can_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pan_can_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_p_caa_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pas_caa_dp_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pan_caa_dp_sse3 _op_blend_pt_p_c_dp_sse3

#define _op_blend_pt_p_c_dpan_sse3 _op_blend_pt_p_c_dp_sse3
#define _op_blend_pt_pas_c_dpan_sse3 _op_blend_pt_pas_c_dp_sse3
#define _op_blend_pt_pan_c_dpan_sse3 _op_blend_pt_pan_c_dp_sse3
#define _op_blend_pt_p_can_dpan_sse3 _op_blend_pt_p_can_dp_sse3
#define _op_blend_pt_pas_can_dpan_sse3 _op_blend_pt_pas_can_dp_sse3
#define _op_blend_pt_pan_can_dpan_sse3 _op_blend_pt_pan_can_dp_sse3
#define _op_blend_pt_p_caa_dpan_sse3 _op_blend_pt_p_caa_dp_sse3
#define _op_blend_pt_pas_caa_dpan_sse3 _op_blend_pt_pas_caa_dp_sse3
#define _op_blend_pt_pan_caa_dpan_sse3 _op_blend_pt_pan_caa_dp_sse3

/**
 * @brief Initializes the SSE3 optimized point blending functions for pixel x color operations.
 *
 * This function populates the `op_blend_pt_funcs` table with pointers to the
 * SSE3-specific implementations for point operations. In this specific SSE3 backend,
 * all point operations for pixel x color are currently set to NULL, indicating
 * they might not be implemented or are handled by generic (non-SSE3) versions.
 *
 * The defines above this function (e.g., _op_blend_pt_pas_c_dp_sse3) follow
 * a similar aliasing pattern as seen in span functions, mapping specialized cases
 * to a general (or in this case, NULL) point operation.
 */
static void
init_blend_pixel_color_pt_funcs_sse3(void)
{
   op_blend_pt_funcs[SP][SM_N][SC][DP][CPU_SSE3] = _op_blend_pt_p_c_dp_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP][CPU_SSE3] = _op_blend_pt_pas_c_dp_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP][CPU_SSE3] = _op_blend_pt_pan_c_dp_sse3;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_pt_p_can_dp_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_pt_pas_can_dp_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_pt_pan_can_dp_sse3;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pt_p_caa_dp_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pt_pas_caa_dp_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_pt_pan_caa_dp_sse3;

   op_blend_pt_funcs[SP][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pt_p_c_dpan_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pt_pas_c_dpan_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_pt_pan_c_dpan_sse3;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pt_p_can_dpan_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pt_pas_can_dpan_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_pt_pan_can_dpan_sse3;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pt_p_caa_dpan_sse3;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pt_pas_caa_dpan_sse3;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_pt_pan_caa_dpan_sse3;
}

/*-----*/

/* blend_rel pixel x color -> dst */

/**
 * @brief Blends source pixels ('s') multiplied by a color ('c') onto destination pixels ('d')
 *        relative to the destination's alpha.
 *
 * This function implements the operation: d = (s * c) * d_alpha/255 + d * (1 - (s * c)_alpha).
 * It processes 'l' pixels. Source pixel 's' is first multiplied by color 'c'.
 * The result is then scaled by the destination pixel's alpha.
 * Finally, this is blended with the original destination 'd' using the alpha of (s*c).
 * SSE3 optimizations are used for aligned data.
 * 'rel' signifies a "relative" blend, where the destination alpha influences the source contribution.
 *
 * @param s Pointer to the source pixel data array (DATA32 per pixel).
 * @param m Pointer to the mask data array (unused in this function).
 * @param c The color value to blend with the source pixels (DATA32).
 * @param d Pointer to the destination pixel data array (DATA32 per pixel). This array is read from and written to.
 *          Its alpha component is used to scale the (s*c) contribution.
 * @param l The number of pixels to process.
 */
static void
_op_blend_rel_p_c_dp_sse3(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {

   int alpha;

   const __m128i c_packed = _mm_set_epi32(c, c, c, c);

   LOOP_ALIGNED_U1_A48(d, l,
      { /* UOP */

         DATA32 sc = MUL4_SYM(c, *s);
         alpha = 256 - (sc >> 24);
         *d = MUL_SYM(*d >> 24, sc) + MUL_256(alpha, *d);
         d++; s++; l--;
      },
      { /* A4OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i sc0 = mul4_sym_sse3(c_packed, s0);
         __m128i a0 = sub4_alpha_sse3(sc0);

         __m128i l0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), sc0);
         __m128i r0 = mul_256_sse3(a0, d0);

         r0 = _mm_add_epi32(l0, r0);

         _mm_store_si128((__m128i *)d, r0);

         d += 4; s += 4; l -= 4;
      },
      {  /* A8OP */

         __m128i s0 = _mm_lddqu_si128((__m128i *)s);
         __m128i d0 = _mm_load_si128((__m128i *)d);

         __m128i s1 = _mm_lddqu_si128((__m128i *)(s+4));
         __m128i d1 = _mm_load_si128((__m128i *)(d+4));

         __m128i sc0 = mul4_sym_sse3(c_packed, s0);
         __m128i sc1 = mul4_sym_sse3(c_packed, s1);

         __m128i a0 = sub4_alpha_sse3(sc0);
         __m128i a1 = sub4_alpha_sse3(sc1);

         __m128i l0 = mul_sym_sse3(_mm_srli_epi32(d0, 24), sc0);
         __m128i r0 = mul_256_sse3(a0, d0);

         __m128i l1 = mul_sym_sse3(_mm_srli_epi32(d1, 24), sc1);
         __m128i r1 = mul_256_sse3(a1, d1);

         r0 = _mm_add_epi32(l0, r0);
         r1 = _mm_add_epi32(l1, r1);

         _mm_store_si128((__m128i *)d, r0);
         _mm_store_si128((__m128i *)(d+4), r1);

         d += 8; s += 8; l -= 8;
      })
}

#define _op_blend_rel_pas_c_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_pan_c_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_p_can_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_pas_can_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_pan_can_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_p_caa_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_pas_caa_dp_sse3 _op_blend_rel_p_c_dp_sse3
#define _op_blend_rel_pan_caa_dp_sse3 _op_blend_rel_p_c_dp_sse3

#define _op_blend_rel_p_c_dpan_sse3 _op_blend_p_c_dpan_sse3
#define _op_blend_rel_pas_c_dpan_sse3 _op_blend_pas_c_dpan_sse3
#define _op_blend_rel_pan_c_dpan_sse3 _op_blend_pan_c_dpan_sse3
#define _op_blend_rel_p_can_dpan_sse3 _op_blend_p_can_dpan_sse3
#define _op_blend_rel_pas_can_dpan_sse3 _op_blend_pas_can_dpan_sse3
#define _op_blend_rel_pan_can_dpan_sse3 _op_blend_pan_can_dpan_sse3
#define _op_blend_rel_p_caa_dpan_sse3 _op_blend_p_caa_dpan_sse3
#define _op_blend_rel_pas_caa_dpan_sse3 _op_blend_pas_caa_dpan_sse3
#define _op_blend_rel_pan_caa_dpan_sse3 _op_blend_pan_caa_dpan_sse3

/**
 * @brief Initializes the SSE3 optimized span blending functions for relative pixel x color operations.
 *
 * This function populates the `op_blend_rel_span_funcs` table with pointers to the
 * SSE3-specific implementations for various "relative" blending modes.
 *
 * For `_op_blend_rel_p_c_dp_sse3` and its aliases: these functions perform a blend
 * where the source (pixel * color) is modulated by the destination's alpha before
 * being combined with the destination.
 *
 * For aliases pointing to `_op_blend_p_c_dpan_sse3` (and similar `dpan` suffixed functions):
 * these are intended for destinations where alpha is not preserved or considered opaque.
 * The actual implementation aliased (`_op_blend_p_c_dpan_sse3` etc.) would handle this.
 * Note: The current defines map `_op_blend_rel_..._dpan_sse3` to `_op_blend_..._dpan_sse3` from the
 * non-relative section. This implies that for 'dpan' cases, the 'relative' aspect might be
 * handled differently or becomes equivalent to a non-relative 'dpan' operation.
 *
 * The indexing (SP, SC, DP etc.) follows the same pattern as `init_blend_pixel_color_span_funcs_sse3`.
 */
static void
init_blend_rel_pixel_color_span_funcs_sse3(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_p_c_dp_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pas_c_dp_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pan_c_dp_sse3;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_p_can_dp_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pas_can_dp_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pan_can_dp_sse3;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_p_caa_dp_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pas_caa_dp_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pan_caa_dp_sse3;

   op_blend_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_p_c_dpan_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pas_c_dpan_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pan_c_dpan_sse3;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_p_can_dpan_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pas_can_dpan_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pan_can_dpan_sse3;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_p_caa_dpan_sse3;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pas_caa_dpan_sse3;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pan_caa_dpan_sse3;
}

#define _op_blend_rel_pt_p_c_dp_sse3 NULL

#define _op_blend_rel_pt_pas_c_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_pan_c_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_p_can_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_pas_can_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_pan_can_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_p_caa_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_pas_caa_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3
#define _op_blend_rel_pt_pan_caa_dp_sse3 _op_blend_rel_pt_p_c_dp_sse3

#define _op_blend_rel_pt_p_c_dpan_sse3 _op_blend_pt_p_c_dpan_sse3
#define _op_blend_rel_pt_pas_c_dpan_sse3 _op_blend_pt_pas_c_dpan_sse3
#define _op_blend_rel_pt_pan_c_dpan_sse3 _op_blend_pt_pan_c_dpan_sse3
#define _op_blend_rel_pt_p_can_dpan_sse3 _op_blend_pt_p_can_dpan_sse3
#define _op_blend_rel_pt_pas_can_dpan_sse3 _op_blend_pt_pas_can_dpan_sse3
#define _op_blend_rel_pt_pan_can_dpan_sse3 _op_blend_pt_pan_can_dpan_sse3
#define _op_blend_rel_pt_p_caa_dpan_sse3 _op_blend_pt_p_caa_dpan_sse3
#define _op_blend_rel_pt_pas_caa_dpan_sse3 _op_blend_pt_pas_caa_dpan_sse3
#define _op_blend_rel_pt_pan_caa_dpan_sse3 _op_blend_pt_pan_caa_dpan_sse3

/**
 * @brief Initializes the SSE3 optimized point blending functions for relative pixel x color operations.
 *
 * This function populates the `op_blend_rel_pt_funcs` table with pointers to the
 * SSE3-specific implementations for relative point operations. Similar to
 * `init_blend_pixel_color_pt_funcs_sse3`, all point operations for relative
 * pixel x color blends are currently set to NULL in this SSE3 backend.
 * This suggests they are either not implemented with SSE3 or fall back to generic code.
 *
 * The defines and aliasing patterns mirror those in `init_blend_pixel_color_pt_funcs_sse3`
 * and `init_blend_rel_pixel_color_span_funcs_sse3`.
 */
static void
init_blend_rel_pixel_color_pt_funcs_sse3(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pt_p_c_dp_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pt_pas_c_dp_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP][CPU_SSE3] = _op_blend_rel_pt_pan_c_dp_sse3;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pt_p_can_dp_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pt_pas_can_dp_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_SSE3] = _op_blend_rel_pt_pan_can_dp_sse3;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pt_p_caa_dp_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pt_pas_caa_dp_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_SSE3] = _op_blend_rel_pt_pan_caa_dp_sse3;

   op_blend_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pt_p_c_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pas_c_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pan_c_dpan_sse3;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pt_p_can_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pas_can_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pan_can_dpan_sse3;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pt_p_caa_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pas_caa_dpan_sse3;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_SSE3] = _op_blend_rel_pt_pan_caa_dpan_sse3;
}

#endif
