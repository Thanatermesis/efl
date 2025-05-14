#ifdef BUILD_NEON
#include <arm_neon.h>
#endif
/* blend pixel x mask --> dst */

#ifdef BUILD_NEON

/**
 * @brief Blend source pixels (with alpha) onto destination pixels using a mask, optimized with NEON.
 *
 * This function implements the "source-over" Porter-Duff compositing operation:
 * Dst = Src_blended + (1 - Src_blended_alpha) * Dst_orig
 * where Src_blended is the source pixel modulated by the mask.
 * The source pixels are assumed to have their own alpha channel (pas - Pixel Alpha Source).
 * The mask is an alpha mask (mas - Mask Alpha Source).
 * The destination pixels are assumed to be in premultiplied alpha format (dp - Destination Premultiplied).
 *
 * @param s Pointer to the source pixel data array. Each DATA32 is a pixel, e.g., 0xAARRGGBB.
 *          Example: s[0] = 0x80FF0000 (semi-transparent red).
 * @param m Pointer to the mask data array. Each DATA8 is an alpha value (0-255).
 *          Example: m[0] = 0xFF (opaque mask for pixel 0), m[1] = 0x80 (semi-transparent for pixel 1).
 * @param c Unused color value (part of a generic function signature).
 * @param d Pointer to the destination pixel data array (premualtiplied alpha). Modified in place.
 *          Example: d[0] = 0xFF0000FF (opaque blue).
 * @param l Number of pixels to process.
 */
static void
_op_blend_pas_mas_dp_neon(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   uint16x8_t m_16x8;
   uint16x8_t ms0_16x8;
   uint16x8_t ms1_16x8;
   uint16x8_t temp0_16x8;
   uint16x8_t temp1_16x8;
   uint16x8_t x255_16x8;
   uint32_t m_32;
   uint32x2_t m_32x2 = {0, 0};
   uint32x4_t a_32x4;
   uint32x4_t ad_32x4;
   uint32x4_t cond_32x4;
   uint32x4_t d_32x4;
   uint32x4_t m_32x4;
   uint32x4_t ms_32x4;
   uint32x4_t s_32x4;
   uint32x4_t temp_32x4;
   uint32x4_t x0_32x4;
   uint32x4_t x1_32x4;
   uint8x16_t a_8x16;
   uint8x16_t d_8x16;
   uint8x16_t m_8x16;
   uint8x16_t ms_8x16;
   uint8x16_t s_8x16;
   uint8x16_t temp_8x16;
   uint8x16_t x0_8x16;
   uint8x16_t x1_8x16;
   uint8x8_t a0_8x8;
   uint8x8_t a1_8x8;
   uint8x8_t d0_8x8;
   uint8x8_t d1_8x8;
   uint8x8_t m0_8x8;
   uint8x8_t m1_8x8;
   uint8x8_t m_8x8;
   uint8x8_t ms0_8x8;
   uint8x8_t ms1_8x8;
   uint8x8_t s0_8x8;
   uint8x8_t s1_8x8;
   uint8x8_t temp0_8x8;
   uint8x8_t temp1_8x8;

   x1_8x16 = vdupq_n_u8(0x1);
   x1_32x4 = vreinterpretq_u32_u8(x1_8x16);
   x255_16x8 = vdupq_n_u16(0xff);
   x0_8x16 = vdupq_n_u8(0x0);
   x0_32x4 = vreinterpretq_u32_u8(x0_8x16);

   DATA32 *end = d + (l & ~3);
   while (d < end)
   {
      unsigned int k = *((unsigned int *)m); // Read 4 mask values at once
      // Shortcut: if all 4 mask alpha values are 0, skip blending for these 4 pixels.
      if (k == 0)
      {
         m+=4;
         d+=4;
         s+=4;
         continue;
      }
      // Shortcut: if all 4 mask alpha values are 0xff (fully opaque).
      // In this case, the mask effectively has no impact, and the blend is
      // equivalent to blending the source directly onto the destination.
      // Dst = Src + (1 - Src_alpha) * Dst_orig
      if (~k == 0)
      {
         // load 4 elements from s
         s_32x4 = vld1q_u32(s);
         s_8x16 = vreinterpretq_u8_u32(s_32x4);

         // load 4 elements from d (destination)
         d_32x4 = vld1q_u32(d);
         d_8x16 = vreinterpretq_u8_u32(d_32x4);
         d0_8x8 = vget_low_u8(d_8x16);
         d1_8x8 = vget_high_u8(d_8x16);

         // Calculate (256 - SourceAlpha) for each pixel.
         // SourceAlpha is in the highest byte of each 32-bit pixel.
         // x0_8x16 is a vector of zeros. vsubq(0, s) gives -s.
         // The alpha component effectively becomes (256 - s_alpha) due to unsigned arithmetic.
         a_8x16 = vsubq_u8(x0_8x16, s_8x16);
         a_32x4 = vreinterpretq_u32_u8(a_8x16);

         // Broadcast the calculated (256 - SourceAlpha) to all channels of the pixel.
         // This value will be used to multiply with the destination pixel components.
         a_32x4 = vshrq_n_u32(a_32x4, 24); // Extract alpha (now 256 - s_alpha)
         a_32x4 = vmulq_u32(a_32x4, x1_32x4); // Broadcast to all 4 bytes
         a_8x16 = vreinterpretq_u8_u32(a_32x4);
         a0_8x8 = vget_low_u8(a_8x16);
         a1_8x8 = vget_high_u8(a_8x16);

         // Calculate MUL_256( (256 - SourceAlpha), DestinationPixel )
         // This is equivalent to ( (256 - SourceAlpha) * DestinationPixel ) / 256
         temp0_16x8 = vmull_u8(a0_8x8, d0_8x8); // Multiply low bytes
         temp1_16x8 = vmull_u8(a1_8x8, d1_8x8); // Multiply high bytes
         temp0_8x8 = vshrn_n_u16(temp0_16x8,8); // Shift right by 8 (divide by 256)
         temp1_8x8 = vshrn_n_u16(temp1_16x8,8);
         temp_8x16 = vcombine_u8(temp0_8x8, temp1_8x8);
         temp_32x4 = vreinterpretq_u32_u8(temp_8x16);

         // If SourceAlpha was 255, then (256 - SourceAlpha) would be 1 (approximately).
         // If SourceAlpha was 0, then (256 - SourceAlpha) would be 0 (effectively, from 256 becoming 0 in u8).
         // The vbslq instruction selects d_32x4 if (256-SourceAlpha) was 0, otherwise temp_32x4.
         // This handles the case where source alpha is 0, so Dst = Dst_orig.
         cond_32x4 = vceqq_u32(a_32x4, x0_32x4); // Check if (256-SourceAlpha) broadcasted is all zeros
         ad_32x4 = vbslq_u32(cond_32x4, d_32x4, temp_32x4);

         // Final result: Dst = Src + (1 - Src_alpha) * Dst_orig
         d_32x4 = vaddq_u32(s_32x4, ad_32x4);

         // Store result
         vst1q_u32(d, d_32x4);
         m+=4;
         d+=4;
         s+=4;
         continue;
      }
      // General case: mask is partially transparent.
      // Load 4 mask values (already in k, now put into a vector)
      m_32 = k;
      m_32x2 = vset_lane_u32(m_32, m_32x2, 0);

      // Load 4 source pixels
      s_32x4 = vld1q_u32(s);
      s_8x16 = vreinterpretq_u8_u32(s_32x4);
      s0_8x8 = vget_low_u8(s_8x16);
      s1_8x8 = vget_high_u8(s_8x16);

      // Load 4 destination pixels
      d_32x4 = vld1q_u32(d);
      d_8x16 = vreinterpretq_u8_u32(d_32x4);
      d0_8x8 = vget_low_u8(d_8x16);
      d1_8x8 = vget_high_u8(d_8x16);

      // Expand mask from 4xDATA8 to 4x(DATA32 with mask alpha in each byte)
      // See similar comments in _op_blend_pas_mas_dp_neon for this expansion logic.
      m_8x8 = vreinterpret_u8_u32(m_32x2);
      m_16x8 = vmovl_u8(m_8x8);
      m_8x16 = vreinterpretq_u8_u16(m_16x8);
      m_8x8 = vget_low_u8(m_8x16);
      m_16x8 = vmovl_u8(m_8x8);
      m_32x4 = vreinterpretq_u32_u16(m_16x8);

      // Broadcast each mask byte to all 4 bytes of its corresponding 32-bit word.
      m_32x4 = vmulq_u32(m_32x4, x1_32x4);
      m_8x16 = vreinterpretq_u8_u32(m_32x4);
      m0_8x8 = vget_low_u8(m_8x16);
      m1_8x8 = vget_high_u8(m_8x16);

      // Calculate MUL_SYM(MaskAlpha, SourcePixel) -> Src_blended
      // This is (MaskAlpha * SourcePixel + 255) / 256 for each channel.
      ms0_16x8 = vmull_u8(m0_8x8, s0_8x8);
      ms1_16x8 = vmull_u8(m1_8x8, s1_8x8);
      ms0_16x8 = vaddq_u16(ms0_16x8, x255_16x8); // Add 255 for rounding
      ms1_16x8 = vaddq_u16(ms1_16x8, x255_16x8);
      ms0_8x8 = vshrn_n_u16(ms0_16x8, 8);    // Shift right by 8
      ms1_8x8 = vshrn_n_u16(ms1_16x8, 8);
      ms_8x16 = vcombine_u8(ms0_8x8, ms1_8x8); // Result is Src_blended

      // Calculate (256 - Src_blended_alpha)
      // Src_blended_alpha is in the highest byte of each 32-bit element of ms_8x16.
      a_8x16 = vsubq_u8(x0_8x16, ms_8x16); // (0 - Src_blended)
      a_32x4 = vreinterpretq_u32_u8(a_8x16);

      // Broadcast (256 - Src_blended_alpha) to all channels of the pixel.
      a_32x4 = vshrq_n_u32(a_32x4, 24); // Extract alpha (now 256 - Src_blended_alpha)
      a_32x4 = vmulq_u32(a_32x4, x1_32x4); // Broadcast
      a_8x16 = vreinterpretq_u8_u32(a_32x4);
      a0_8x8 = vget_low_u8(a_8x16);
      a1_8x8 = vget_high_u8(a_8x16);

      // Calculate MUL_256( (256 - Src_blended_alpha), DestinationPixel )
      // This is ( (256 - Src_blended_alpha) * DestinationPixel ) / 256
      temp0_16x8 = vmull_u8(a0_8x8, d0_8x8);
      temp1_16x8 = vmull_u8(a1_8x8, d1_8x8);
      temp0_8x8 = vshrn_n_u16(temp0_16x8,8); // Shift right by 8
      temp1_8x8 = vshrn_n_u16(temp1_16x8,8);
      temp_8x16 = vcombine_u8(temp0_8x8, temp1_8x8);
      temp_32x4 = vreinterpretq_u32_u8(temp_8x16);

      // If (256 - Src_blended_alpha) was 0, select original destination component.
      cond_32x4 = vceqq_u32(a_32x4, x0_32x4);
      ad_32x4 = vbslq_u32(cond_32x4, d_32x4, temp_32x4);

      // Final result: Dst = Src_blended + (1 - Src_blended_alpha) * Dst_orig
      ms_32x4 = vreinterpretq_u32_u8(ms_8x16); // Src_blended
      d_32x4 = vaddq_u32(ms_32x4, ad_32x4);    // Add the two components

      // Store result
      vst1q_u32(d, d_32x4);

      d+=4;
      s+=4;
      m+=4;
   }

   int alpha;
   DATA32 temp;

   // Process remaining pixels (0 to 3) using scalar operations
   end += (l & 3);
   while (d < end)
   {
      alpha = *m; // Current mask alpha
      switch(alpha)
        {
        case 0: // Mask is fully transparent, destination unchanged
           break;
        case 255: // Mask is fully opaque
           // Dst = Src + (1 - Src_alpha) * Dst_orig
           alpha = 256 - (*s >> 24); // (1 - Src_alpha), effectively (256-SrcAlpha)/256
           *d = *s + MUL_256(alpha, *d);
           break;
        default: // Mask is partially transparent
           // Src_blended = MUL_SYM(MaskAlpha, Src)
           temp = MUL_SYM(alpha, *s);
           // alpha_blend_factor = (1 - Src_blended_alpha)
           alpha = 256 - (temp >> 24);
           // Dst = Src_blended + alpha_blend_factor * Dst_orig
           *d = temp + MUL_256(alpha, *d);
           break;
        }
      m++;  s++;  d++;
   }
}

/**
 * @brief Blend source pixels (opaque or alpha ignored) onto destination pixels using a mask, optimized with NEON.
 *
 * This function implements the "source-over" Porter-Duff compositing operation:
 * Dst = Src_blended + (1 - Src_blended_alpha) * Dst_orig
 * where Src_blended is the source pixel modulated by the mask.
 * The source pixels are treated as if their alpha is opaque or their alpha channel is not
 * directly used in the initial (1 - Src_alpha) calculation like in `_op_blend_pas_mas_dp_neon`.
 * Instead, the alpha of Src_blended (after modulation by mask) is used.
 * (p - Pixel, implies source is opaque or its alpha isn't used in a "pas" way).
 * The mask is an alpha mask (mas - Mask Alpha Source).
 * The destination pixels are assumed to be in premultiplied alpha format (dp - Destination Premultiplied).
 *
 * @param s Pointer to the source pixel data array. Each DATA32 is a pixel, e.g., 0xAARRGGBB.
 * @param m Pointer to the mask data array. Each DATA8 is an alpha value (0-255).
 * @param c Unused color value.
 * @param d Pointer to the destination pixel data array (premultiplied alpha). Modified in place.
 * @param l Number of pixels to process.
 */
static void
_op_blend_p_mas_dp_neon(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   uint16x8_t m_16x8;
   uint16x8_t ms0_16x8;
   uint16x8_t ms1_16x8;
   uint16x8_t temp0_16x8;
   uint16x8_t temp1_16x8;
   uint16x8_t x255_16x8;
   uint32x2_t m_32x2 = { 0,  0 };
   uint32x4_t a_32x4;
   uint32x4_t ad_32x4;
   uint32x4_t cond_32x4;
   uint32x4_t d_32x4;
   uint32x4_t m_32x4;
   uint32x4_t ms_32x4;
   uint32x4_t s_32x4;
   uint32x4_t temp_32x4;
   uint32x4_t x0_32x4;
   uint32x4_t x1_32x4;
   uint8x16_t a_8x16;
   uint8x16_t d_8x16;
   uint8x16_t m_8x16;
   uint8x16_t ms_8x16;
   uint8x16_t s_8x16;
   uint8x16_t temp_8x16;
   uint8x16_t x0_8x16;
   uint8x16_t x1_8x16;
   uint8x8_t a0_8x8;
   uint8x8_t a1_8x8;
   uint8x8_t d0_8x8;
   uint8x8_t d1_8x8;
   uint8x8_t m0_8x8;
   uint8x8_t m1_8x8;
   uint8x8_t m_8x8;
   uint8x8_t ms0_8x8;
   uint8x8_t ms1_8x8;
   uint8x8_t s0_8x8;
   uint8x8_t s1_8x8;
   uint8x8_t temp0_8x8;
   uint8x8_t temp1_8x8;

   x1_8x16 = vdupq_n_u8(0x1);
   x1_32x4 = vreinterpretq_u32_u8(x1_8x16);
   x255_16x8 = vdupq_n_u16(0xff);
   x0_8x16 = vdupq_n_u8(0x0);
   x0_32x4 = vreinterpretq_u32_u8(x0_8x16);

   DATA32 *end = d + (l & ~3); // End of 4-pixel aligned processing block
   while (d < end)
   {
      // Load 4 mask values (DATA8) as a single u32.
      m_32x2 = vld1_lane_u32((DATA32*)m, m_32x2, 0);

      // Load 4 source pixels
      s_32x4 = vld1q_u32(s);
      s_8x16 = vreinterpretq_u8_u32(s_32x4);
      s0_8x8 = vget_low_u8(s_8x16);
      s1_8x8 = vget_high_u8(s_8x16);

      // Load 4 destination pixels
      d_32x4 = vld1q_u32(d);
      d_8x16 = vreinterpretq_u8_u32(d_32x4);
      d0_8x8 = vget_low_u8(d_8x16);
      d1_8x8 = vget_high_u8(d_8x16);

      // make m 32 bit wide
      m_8x8 = vreinterpret_u8_u32(m_32x2);
      m_16x8 = vmovl_u8(m_8x8);
      m_8x16 = vreinterpretq_u8_u16(m_16x8);
      m_8x8 = vget_low_u8(m_8x16);
      m_16x8 = vmovl_u8(m_8x8);
      m_32x4 = vreinterpretq_u32_u16(m_16x8);

      // place m into every 8 bit element of vector
      m_32x4 = vmulq_u32(m_32x4, x1_32x4);
      m_8x16 = vreinterpretq_u8_u32(m_32x4);
      m0_8x8 = vget_low_u8(m_8x16);
      m1_8x8 = vget_high_u8(m_8x16);

      // multiply MUL_SYM(m, *s);
      ms0_16x8 = vmull_u8(m0_8x8, s0_8x8);
      ms1_16x8 = vmull_u8(m1_8x8, s1_8x8);
      ms0_16x8 = vaddq_u16(ms0_16x8, x255_16x8);
      ms1_16x8 = vaddq_u16(ms1_16x8, x255_16x8);
      ms0_8x8 = vshrn_n_u16(ms0_16x8, 8);
      ms1_8x8 = vshrn_n_u16(ms1_16x8, 8);
      ms_8x16 = vcombine_u8(ms0_8x8, ms1_8x8);

      // substract 256 - m*s
      a_8x16 = vsubq_u8(x0_8x16, ms_8x16);
      a_32x4 = vreinterpretq_u32_u8(a_8x16);

      // shift alpha>>24 and place it into every 8bit element
      a_32x4 = vshrq_n_u32(a_32x4, 24);
      a_32x4 = vmulq_u32(a_32x4, x1_32x4);
      a_8x16 = vreinterpretq_u8_u32(a_32x4);
      a0_8x8 = vget_low_u8(a_8x16);
      a1_8x8 = vget_high_u8(a_8x16);

      // multiply MUL_256(a, *d)
      temp0_16x8 = vmull_u8(a0_8x8, d0_8x8);
      temp1_16x8 = vmull_u8(a1_8x8, d1_8x8);
      temp0_8x8 = vshrn_n_u16(temp0_16x8,8);
      temp1_8x8 = vshrn_n_u16(temp1_16x8,8);
      temp_8x16 = vcombine_u8(temp0_8x8, temp1_8x8);
      temp_32x4 = vreinterpretq_u32_u8(temp_8x16);

      // if alpha is 0, replace a*d with d
      cond_32x4 = vceqq_u32(a_32x4, x0_32x4);
      ad_32x4 = vbslq_u32(cond_32x4, d_32x4, temp_32x4);

      // add m*s
      ms_32x4 = vreinterpretq_u32_u8(ms_8x16);
      d_32x4 = vaddq_u32(ms_32x4, ad_32x4);

      // save result
      vst1q_u32(d, d_32x4);

      d+=4;
      s+=4;
      m+=4;
   }

   int alpha;
   DATA32 temp;

   // Process remaining pixels (0 to 3) using scalar operations
   end += (l & 3);
   while (d < end)
   {
      alpha = *m; // Current mask alpha
      // Src_blended = MUL_SYM(MaskAlpha, Src)
      temp = MUL_SYM(alpha, *s);
      // alpha_blend_factor = (1 - Src_blended_alpha)
      alpha = 256 - (temp >> 24);
      // Dst = Src_blended + alpha_blend_factor * Dst_orig
      *d = temp + MUL_256(alpha, *d);
      m++;  s++;  d++;
   }
}

/* Aliases for NEON span blending functions.
 * pan: Pixel Alpha No (source is opaque or alpha ignored for input, but output alpha is generated)
 * pas: Pixel Alpha Source (source has alpha)
 * dpan: Destination Premultiplied, Alpha No (destination alpha is not used in blend factor, or set to opaque)
 */
#define _op_blend_pan_mas_dp_neon _op_blend_pas_mas_dp_neon

#define _op_blend_p_mas_dpan_neon _op_blend_p_mas_dp_neon
#define _op_blend_pan_mas_dpan_neon _op_blend_pan_mas_dp_neon
#define _op_blend_pas_mas_dpan_neon _op_blend_pas_mas_dp_neon

/**
 * @brief Initializes NEON-optimized span blending functions for pixel operations with masks.
 * This function populates a table (op_blend_span_funcs) with pointers to NEON-specific
 * implementations for various combinations of source, mask, and destination properties.
 */
static void
init_blend_pixel_mask_span_funcs_neon(void)
{
   op_blend_span_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_p_mas_dp_neon;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_pas_mas_dp_neon;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_pan_mas_dp_neon;

   op_blend_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_p_mas_dpan_neon;
   op_blend_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_pas_mas_dpan_neon;
   op_blend_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_pan_mas_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Blend a single source pixel (opaque/alpha ignored) onto a destination pixel using a mask.
 *
 * This is a point (single pixel) operation.
 * Dst = Src_blended + (1 - Src_blended_alpha) * Dst_orig
 * where Src_blended = MUL_SYM(MaskAlpha, Src).
 * (pt - Point operation, p - Pixel, mas - Mask Alpha Source, dp - Destination Premultiplied)
 *
 * @param s Source pixel (0xAARRGGBB).
 * @param m Mask alpha value (0-255).
 * @param c Unused color value. Marked EINA_UNUSED as it is not used in this function.
 * @param d Pointer to the destination pixel (0xAARRGGBB, premultiplied alpha). Modified in place.
 */
static void
_op_blend_pt_p_mas_dp_neon(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
   DATA32 src_blended;
   int blend_factor_alpha;

   // Src_blended = MUL_SYM(MaskAlpha, Src)
   src_blended = MUL_SYM(m, s);
   // blend_factor_alpha = (1 - Src_blended_alpha)
   blend_factor_alpha = 256 - (src_blended >> 24);
   // Dst = Src_blended + blend_factor_alpha * Dst_orig
   *d = src_blended + MUL_256(blend_factor_alpha, *d);
}

/* Aliases for NEON point blending functions. */
#define _op_blend_pt_pan_mas_dp_neon _op_blend_pt_p_mas_dp_neon
#define _op_blend_pt_pas_mas_dp_neon _op_blend_pt_p_mas_dp_neon

#define _op_blend_pt_p_mas_dpan_neon _op_blend_pt_p_mas_dp_neon
#define _op_blend_pt_pas_mas_dpan_neon _op_blend_pt_pas_mas_dp_neon
#define _op_blend_pt_pan_mas_dpan_neon _op_blend_pt_pan_mas_dp_neon

/**
 * @brief Initializes NEON-optimized point blending functions for pixel operations with masks.
 * This function populates a table (op_blend_pt_funcs) with pointers to NEON-specific
 * implementations for various single-pixel blending scenarios.
 */
static void
init_blend_pixel_mask_pt_funcs_neon(void)
{
   op_blend_pt_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_pt_p_mas_dp_neon;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_pt_pas_mas_dp_neon;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_blend_pt_pan_mas_dp_neon;

   op_blend_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_pt_p_mas_dpan_neon;
   op_blend_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_pt_pas_mas_dpan_neon;
   op_blend_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_pt_pan_mas_dpan_neon;
}
#endif

/*-----*/

/* blend_rel pixel x mask -> dst */
/* 'rel' likely refers to "relative" blending operations, which might imply
 * specific interpretations of alpha or color values, often used in contexts
 * like evas textblock rendering where glyphs are blended additively or differently.
 * The actual implementation here aliases to standard blend functions, suggesting
 * the 'rel' distinction is handled by which function from the standard set is chosen,
 * or that for these specific NEON paths, the 'rel' variant is identical to a standard one.
 */

#ifdef BUILD_NEON

/* Aliases for 'relative' NEON span blending functions. */
#define _op_blend_rel_p_mas_dpan_neon _op_blend_p_mas_dpan_neon
#define _op_blend_rel_pas_mas_dpan_neon _op_blend_pas_mas_dpan_neon
#define _op_blend_rel_pan_mas_dpan_neon _op_blend_pan_mas_dpan_neon

/**
 * @brief Initializes NEON-optimized 'relative' span blending functions for pixel operations with masks.
 * Populates op_blend_rel_span_funcs table.
 */
static void
init_blend_rel_pixel_mask_span_funcs_neon(void)
{
   op_blend_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_p_mas_dpan_neon;
   op_blend_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_pas_mas_dpan_neon;
   op_blend_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_pan_mas_dpan_neon;
}
#endif

#ifdef BUILD_NEON

/* Aliases for 'relative' NEON point blending functions. */
#define _op_blend_rel_pt_p_mas_dpan_neon _op_blend_pt_p_mas_dpan_neon
#define _op_blend_rel_pt_pas_mas_dpan_neon _op_blend_pt_pas_mas_dpan_neon
#define _op_blend_rel_pt_pan_mas_dpan_neon _op_blend_pt_pan_mas_dpan_neon

/**
 * @brief Initializes NEON-optimized 'relative' point blending functions for pixel operations with masks.
 * Populates op_blend_rel_pt_funcs table.
 */
static void
init_blend_rel_pixel_mask_pt_funcs_neon(void)
{
   op_blend_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_pt_p_mas_dpan_neon;
   op_blend_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_pt_pas_mas_dpan_neon;
   op_blend_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_blend_rel_pt_pan_mas_dpan_neon;
}
#endif
