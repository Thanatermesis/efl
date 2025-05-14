/**
 * @file
 * This file contains the core loop for rendering scaled and transformed images,
 * with different paths for smooth (bilinear interpolation) and non-smooth
 * (nearest neighbor) scaling, and optimizations for MMX and NEON instruction sets.
 * It also handles color multiplication.
 */

#ifdef SMOOTH
/**
 * @brief Smooth scaling (bilinear interpolation) path.
 *
 * This block handles image rendering with bilinear interpolation for smoother
 * results when scaling or transforming. It has further specialized paths
 * for MMX and NEON optimizations.
 */
{
# ifdef SCALE_USING_MMX
/**
 * @brief MMX optimized smooth scaling.
 *
 * Variables and initial setup for MMX-accelerated bilinear interpolation.
 * This section is active if MMX optimizations are enabled.
 */
#  ifdef COLMUL
#   ifdef COLSAME
   MOV_P2R(c1, mm7, mm0); // col
#   endif //COLSAME
#  endif //COLMUL
# endif //SCALE_USING_MMX

# ifdef SCALE_USING_NEON
/**
 * @brief NEON optimized smooth scaling.
 *
 * Variables and initial setup for NEON-accelerated bilinear interpolation.
 * This section is active if NEON optimizations are enabled.
 */
#  ifndef COLBLACK
/**
 * @brief NEON smooth scaling with color processing.
 *
 * This sub-block handles NEON-optimized smooth scaling when not simply
 * filling with a black color. It involves color calculations and
 * potentially color multiplication.
 */
   uint16x4_t temp_16x4 = { 0, 0, 0, 0 };
   uint16x4_t rv_16x4;
   uint16x4_t val1_16x4;
   uint16x4_t val3_16x4;
   uint16x8_t ru_16x8;
   uint16x8_t val1_val3_16x8;
   uint16x8_t val2_val4_16x8;
   uint16x8_t x255_16x8;
   uint32x2_t res_32x2;
   uint32x2_t val1_val3_32x2 = { 0, 0 };
   uint32x2_t val2_val4_32x2 = { 0, 0 };
   uint8x8_t val1_val3_8x8;
   uint8x8_t val2_val4_8x8;

   x255_16x8 = vdupq_n_u16(0xff); // Vector of 0xff for masking/clamping.
#   ifdef COLMUL
/**
 * @brief NEON smooth scaling with color multiplication.
 *
 * Variables for NEON-optimized smooth scaling when color multiplication
 * is enabled.
 */
   uint16x4_t x255_16x4;
   x255_16x4 = vget_low_u16(x255_16x8);
   uint16x4_t c1_16x4;
#    ifdef COLSAME
/**
 * @brief NEON smooth scaling with a single color multiplier.
 *
 * Variables for NEON-optimized smooth scaling when a single color
 * multiplier (c1) is applied to all color channels.
 */
   uint16x4_t c1_val3_16x4;
   uint16x8_t c1_16x8;
   uint16x8_t c1_val3_16x8;
   uint32x2_t c1_32x2 = { 0, 0 };
   uint8x8_t c1_8x8;
   uint8x8_t c1_val3_8x8;

   c1_32x2 = vset_lane_u32(c1, c1_32x2, 0);
   c1_8x8 = vreinterpret_u8_u32(c1_32x2);
   c1_16x8 = vmovl_u8(c1_8x8);
   c1_16x4 = vget_low_u16(c1_16x8);
#    else //COLSAME
/**
 * @brief NEON smooth scaling with interpolated color multipliers.
 *
 * Variables for NEON-optimized smooth scaling when color multipliers
 * (c1, c2) are interpolated.
 */
   uint16x4_t c2_16x4;
   uint16x4_t c2_local_16x4;
   uint16x4_t cv_16x4;
   uint16x8_t c1_c2_16x8;
   uint16x8_t c1_val1_16x8;
   uint16x8_t c2_val3_16x8;
   uint16x8_t cv_rv_16x8;
   uint32x2_t c1_c2_32x2 = { 0, 0 };
   uint8x8_t c1_c2_8x8;
   uint8x8_t val3_8x8;
   uint16x8_t val3_16x8;

   c1_c2_32x2 = vset_lane_u32(c1, c1_c2_32x2, 0);
   c1_c2_32x2 = vset_lane_u32(c2, c1_c2_32x2, 1);
   c1_c2_8x8 = vreinterpret_u8_u32(c1_c2_32x2);
   c1_c2_16x8 = vmovl_u8(c1_c2_8x8);
   c1_16x4 = vget_low_u16(c1_c2_16x8);
   c2_16x4 = vget_high_u16(c1_c2_16x8);
#    endif //COLSAME
#   else //COLMUL
   /**
    * @brief NEON smooth scaling without color multiplication.
    *
    * Variables for NEON-optimized smooth scaling when color multiplication
    * is disabled but not filling with black.
    */
   uint8x8_t val3_8x8;
   uint16x8_t val3_16x8;
#   endif //COLMUL
#  endif //COLBLACK
# endif //SCALE_USING_NEON

   /**
    * @brief Main pixel processing loop.
    *
    * Iterates `ww` times, processing one output pixel per iteration.
    * `ww` is the width of the scanline segment to render.
    */
   while (ww > 0)
     {
# ifdef COLBLACK
        /**
         * @brief Fill with black color if COLBLACK is defined.
         * Output pixel `d` is set to opaque black (0xff000000).
         */
        *d = 0xff000000; // col
# else  //COLBLACK
        /**
         * @brief Bilinear interpolation and color processing for a single pixel.
         * This block is executed if not filling with black. It calculates the
         * source texture coordinates (u, v), fetches the four surrounding pixels
         * (val1, val2, val3, val4), performs bilinear interpolation, and then
         * applies color multiplication if enabled.
         */
        FPc uu1, vv1, uu2, vv2; // Fixed-point texture coordinates for the 4 neighbor pixels.
        FPc rv, ru;             // Fixed-point interpolation factors (0-255).
        DATA32 val1, val2, val3, val4;

        uu1 = u;
        if (uu1 < 0) uu1 = 0;
        else if (uu1 >= swp) uu1 = swp - 1;

        vv1 = v;
        if (vv1 < 0) vv1 = 0;
        else if (vv1 >= shp) vv1 = shp - 1;

        uu2 = uu1 + FPFPI1;      // next u point
        if (uu2 >= swp) uu2 = swp - 1;

        vv2 = vv1 + FPFPI1;      // next v point
        if (vv2 >= shp) vv2 = shp - 1;

        ru = (u >> (FP + FPI - 8)) & 0xff;
        rv = (v >> (FP + FPI - 8)) & 0xff;

        s = sp + ((vv1 >> (FP + FPI)) * sw) + (uu1 >> (FP + FPI));
        val1 = *s;             // current pixel

        s = sp + ((vv1 >> (FP + FPI)) * sw) + (uu2 >> (FP + FPI));
        val2 = *s;             // right pixel

        s = sp + ((vv2 >> (FP + FPI)) * sw) + (uu1 >> (FP + FPI));
        val3 = *s;             // bottom pixel

        s = sp + ((vv2 >> (FP + FPI)) * sw) + (uu2 >> (FP + FPI));
        val4 = *s;             // right bottom pixel

#  ifdef SCALE_USING_MMX
        /**
         * @brief MMX optimized bilinear interpolation and color multiplication.
         */
        MOV_A2R(rv, mm4); // Move rv (v-fraction) to mm4
        MOV_A2R(ru, mm6); // Move ru (u-fraction) to mm6
        MOV_P2R(val1, mm1, mm0);
        if (val1 | val2)
          {
             MOV_P2R(val2, mm2, mm0);
             INTERP_256_R2R(mm6, mm2, mm1, mm5);
          }
        MOV_P2R(val3, mm2, mm0);
        if (val3 | val4)
          {
             MOV_P2R(val4, mm3, mm0);
             INTERP_256_R2R(mm6, mm3, mm2, mm5);
          }
        INTERP_256_R2R(mm4, mm2, mm1, mm5); // Interpolate vertically between intermediate results
#   ifdef COLMUL
        /**
         * @brief MMX color multiplication.
         */
#    ifdef COLSAME
        /**
         * @brief MMX: Apply single color multiplier c1.
         */
//        MOV_P2R(c1, mm7, mm0); // col (c1 is already in mm7 from outer scope)
        MUL4_SYM_R2R(mm7, mm1, mm5); // col: Multiply interpolated color by c1
#    else //COLSAME
        /**
         * @brief MMX: Apply interpolated color multiplier.
         * The color multiplier itself is interpolated based on `cv`.
         */
        cc = cv >> 16; // col: Get integer part of current color value for interpolation
        cv += cd; // col: Increment current color value by delta
        MOV_A2R(cc, mm2); // col
        MOV_P2R(c1, mm3, mm0); // col
        MOV_P2R(c2, mm4, mm0); // col
        INTERP_256_R2R(mm2, mm4, mm3, mm5); // col
        MUL4_SYM_R2R(mm3, mm1, mm5); // col: Multiply interpolated color by interpolated c1/c2
#    endif //COLSAME
#   endif //COLMUL
        MOV_R2P(mm1, *d, mm0); // Store final pixel
#  elif defined SCALE_USING_NEON
        /**
         * @brief NEON optimized bilinear interpolation and color multiplication.
         *
         * This section performs bilinear interpolation using NEON intrinsics.
         * It first interpolates along the U axis for two pairs of pixels
         * (val1, val2) and (val3, val4), then interpolates the results
         * along the V axis. Color multiplication is applied afterwards if enabled.
         *
         * If all source pixels (val1, val2, val3, val4) are zero (transparent black),
         * the output pixel `d` is directly set to `val1` (which would be 0).
         * Otherwise, the full interpolation and color processing path is taken.
         */
        if (val1 | val2 | val3 | val4) // Optimization: if all samples are 0, skip calculations
          {
             // rv_16x4: [rv, rv, rv, rv] (u16)
             rv_16x4 = vdup_n_u16(rv);
             // ru_16x8: [ru, ru, ru, ru, ru, ru, ru, ru] (u16)
             ru_16x8 = vdupq_n_u16(ru);

             // val1_val3_32x2: [val1, val3] (u32)
             val1_val3_32x2 = vset_lane_u32(val1, val1_val3_32x2, 0);
             val1_val3_32x2 = vset_lane_u32(val3, val1_val3_32x2, 1);
             // val2_val4_32x2: [val2, val4] (u32)
             val2_val4_32x2 = vset_lane_u32(val2, val2_val4_32x2, 0);
             val2_val4_32x2 = vset_lane_u32(val4, val2_val4_32x2, 1);

             // Unpack 32-bit pixels to 8-bit components (RGBA)
             // val1_val3_8x8: [v1B, v1G, v1R, v1A, v3B, v3G, v3R, v3A] (u8)
             val1_val3_8x8 = vreinterpret_u8_u32(val1_val3_32x2);
             // val2_val4_8x8: [v2B, v2G, v2R, v2A, v4B, v4G, v4R, v4A] (u8)
             val2_val4_8x8 = vreinterpret_u8_u32(val2_val4_32x2);

             // Widen 8-bit components to 16-bit for calculation precision
             val2_val4_16x8 = vmovl_u8(val2_val4_8x8);
             val1_val3_16x8 = vmovl_u8(val1_val3_8x8);

             // Interpolate horizontally: res_u = val1_val3 + (val2_val4 - val1_val3) * ru / 256
             // val2_val4_16x8 effectively becomes [interp(v1,v2), interp(v3,v4)]
             val2_val4_16x8 = vsubq_u16(val2_val4_16x8, val1_val3_16x8); // (val2_val4 - val1_val3)
             val2_val4_16x8 = vmulq_u16(val2_val4_16x8, ru_16x8);        // * ru
             val2_val4_16x8 = vshrq_n_u16(val2_val4_16x8, 8);            // / 256
             val2_val4_16x8 = vaddq_u16(val2_val4_16x8, val1_val3_16x8); // + val1_val3
             val2_val4_16x8 = vandq_u16(val2_val4_16x8, x255_16x8);      // Clamp to 0-255 (effectively)

             // val1_16x4 is the result of U-interpolation between val1 and val2.
             // val3_16x4 is the result of U-interpolation between val3 and val4.
             val1_16x4 = vget_low_u16(val2_val4_16x8);  // Interpolated top row pixels
             val3_16x4 = vget_high_u16(val2_val4_16x8); // Interpolated bottom row pixels
#   ifdef COLMUL
            /**
             * @brief NEON color multiplication after bilinear interpolation.
             */
#    ifdef COLSAME
            /**
             * @brief NEON: Apply single color multiplier c1 after bilinear interpolation.
             * Interpolate vertically: res_v = val1_interp + (val3_interp - val1_interp) * rv / 256
             * Then multiply by c1.
             */
             // val3_16x4 effectively becomes the final bilinearly interpolated color
             val3_16x4 = vsub_u16(val3_16x4, val1_16x4); // (val3_interp - val1_interp)
             val3_16x4 = vmul_u16(val3_16x4, rv_16x4);   // * rv
             val3_16x4 = vshr_n_u16(val3_16x4, 8);       // / 256
             val3_16x4 = vadd_u16(val3_16x4, val1_16x4); // + val1_interp
             val3_16x4 = vand_u16(val3_16x4, x255_16x4); // Clamp

             // Multiply by color c1: (c1 * val3_interp + 255) >> 8
             c1_val3_16x4 = vmul_u16(c1_16x4, val3_16x4);
             c1_val3_16x4 = vadd_u16(c1_val3_16x4, x255_16x4); // + 255 (for rounding)

             // Combine into an 8-element vector (only lower 4 used for result)
             c1_val3_16x8 = vcombine_u16(c1_val3_16x4, temp_16x4);

             // Narrow, shift right by 8 (divide by 256)
             c1_val3_8x8 = vshrn_n_u16(c1_val3_16x8, 8);
             // Reinterpret as u32x2_t to store the first 32-bit pixel
             res_32x2 = vreinterpret_u32_u8(c1_val3_8x8);
#    else //COLSAME
            /**
             * @brief NEON: Apply interpolated color multiplier after bilinear interpolation.
             * This is a more complex case where the color multiplier itself is
             * interpolated, and then applied to the bilinearly interpolated pixel value.
             *
             * 1. Interpolate color multiplier: c_interp = c1 + (c2 - c1) * (cv>>16) / 256
             * 2. Interpolate pixel value: p_interp = val1_u_interp + (val3_u_interp - val1_u_interp) * rv / 256
             * 3. Final color: (c_interp * p_interp + 255) >> 8
             *
             * The NEON code interleaves these operations for efficiency.
             * It combines c1 with val1_u_interp and c2 with val3_u_interp,
             * then performs a combined interpolation.
             */
             // c1_val1_16x8: [c1B,c1G,c1R,c1A, v1uB,v1uG,v1uR,v1uA] (u16)
             c1_val1_16x8 = vcombine_u16(c1_16x4, val1_16x4);
             // c2_val3_16x8: [c2B,c2G,c2R,c2A, v3uB,v3uG,v3uR,v3uA] (u16)
             c2_val3_16x8 = vcombine_u16(c2_16x4, val3_16x4);

             // cv_16x4: [cv_int, cv_int, cv_int, cv_int] (u16), where cv_int = cv >> 16
             cv_16x4 = vdup_n_u16(cv>>16);
             cv += cd;
             cv_rv_16x8 = vcombine_u16(cv_16x4, rv_16x4);

             c2_val3_16x8 = vsubq_u16(c2_val3_16x8, c1_val1_16x8);
             c2_val3_16x8 = vmulq_u16(c2_val3_16x8, cv_rv_16x8);
             c2_val3_16x8 = vshrq_n_u16(c2_val3_16x8, 8);
             c2_val3_16x8 = vaddq_u16(c2_val3_16x8, c1_val1_16x8);
             c2_val3_16x8 = vandq_u16(c2_val3_16x8, x255_16x8);

             c2_local_16x4 = vget_low_u16(c2_val3_16x8);
             val3_16x4 = vget_high_u16(c2_val3_16x8);

             val3_16x4 = vmul_u16(c2_local_16x4, val3_16x4);
             val3_16x4 = vadd_u16(val3_16x4, x255_16x4);

             val3_16x8 = vcombine_u16(val3_16x4, temp_16x4);

             val3_8x8 = vshrn_n_u16(val3_16x8, 8);       // Narrow, shift right by 8
             res_32x2 = vreinterpret_u32_u8(val3_8x8); // Reinterpret as u32x2_t
#    endif //COLSAME
#   else //COLMUL
            /**
             * @brief NEON: Bilinear interpolation without color multiplication.
             * Interpolate vertically: res_v = val1_interp + (val3_interp - val1_interp) * rv / 256
             */
             val3_16x4 = vsub_u16(val3_16x4, val1_16x4); // (val3_interp - val1_interp)
             val3_16x4 = vmul_u16(val3_16x4, rv_16x4);   // * rv
             val3_16x4 = vshr_n_u16(val3_16x4, 8);       // / 256
             val3_16x4 = vadd_u16(val3_16x4, val1_16x4); // + val1_interp
                                                         // val3_16x4 now holds the final interpolated pixel RGBA components (u16)

             // Combine into an 8-element vector (only lower 4 used for result)
             val3_16x8 = vcombine_u16(val3_16x4, temp_16x4);

             // Narrow from u16 to u8 (effectively clamping to 0-255 if values were >255, though interpolation should keep them in range)
             val3_8x8 = vmovn_u16(val3_16x8);
             // Reinterpret as u32x2_t to store the first 32-bit pixel
             res_32x2 = vreinterpret_u32_u8(val3_8x8);
#   endif //COLMUL
             vst1_lane_u32(d, res_32x2, 0); // Store the resulting 32-bit pixel
          }
        else // All source samples (val1, val2, val3, val4) were 0
          *d = val1; // Output is 0
#  else // Not SCALE_USING_MMX and not SCALE_USING_NEON (Generic C implementation for SMOOTH)
        /**
         * @brief Generic C bilinear interpolation.
         * This is the fallback if no MMX/NEON optimization is available.
         */
        val1 = INTERP_256(ru, val2, val1); // Interpolate (val1, val2) by ru
        val3 = INTERP_256(ru, val4, val3); // Interpolate (val3, val4) by ru
        val1 = INTERP_256(rv, val3, val1); // Interpolate results by rv // col
#   ifdef COLMUL
        /**
         * @brief Generic C color multiplication.
         */
#    ifdef COLSAME
        /**
         * @brief Generic C: Apply single color multiplier c1.
         */
        *d = MUL4_SYM(c1, val1);
#    else //COLSAME
        /**
         * @brief Generic C: Apply interpolated color multiplier.
         */
        val2 = INTERP_256((cv >> 16), c2, c1); // col: Interpolate color multiplier
        *d  = MUL4_SYM(val2, val1); // col: Apply interpolated multiplier
        cv += cd; // col
#    endif //COLSAME
#   else // No COLMUL
        *d = val1; // Assign interpolated value directly
#   endif //COLMUL
#  endif //SCALE_USING_MMX (ends #elif defined SCALE_USING_NEON, #else)
        u += ud; // Increment source u coordinate
        v += vd; // Increment source v coordinate
# endif //COLBLACK
        d++;    // Move to next destination pixel
        ww--;   // Decrement remaining width
     }
}
#else //SMOOTH
/**
 * @brief Non-smooth scaling (nearest neighbor) path.
 *
 * This block handles image rendering using nearest neighbor sampling.
 * It's generally faster but results in blockier images when scaling.
 * It also has specialized paths for NEON optimizations.
 */
{
# ifdef SCALE_USING_NEON
/**
 * @brief NEON optimized non-smooth scaling.
 *
 * Variables and initial setup for NEON-accelerated nearest neighbor scaling.
 */
#  ifndef COLBLACK
/**
 * @brief NEON non-smooth scaling with color processing.
 */
#   ifdef COLMUL
/**
 * @brief NEON non-smooth scaling with color multiplication.
 */
   uint16x4_t x255_16x4;
   uint16x4_t temp_16x4 = { 0, 0, 0, 0 };
   uint16x8_t cval_16x8;
   uint32x2_t res_32x2;
   uint8x8_t cval_8x8;
   uint16x4_t c1_16x4;
   uint16x4_t cval_16x4;
   uint16x4_t val1_16x4;
   uint32x2_t val1_32x2 = { 0, 0 };
   uint8x8_t val1_8x8;

   x255_16x4 = vdup_n_u16(0xff); // Vector of 0xff for masking/clamping.
#    ifdef COLSAME
/**
 * @brief NEON non-smooth scaling with a single color multiplier.
 */
   uint16x8_t c1_16x8;
   uint16x8_t val1_16x8;
   uint32x2_t c1_32x2 = { 0, 0 };
   uint8x8_t c1_8x8;

   c1_32x2 = vset_lane_u32(c1, c1_32x2, 0);

   c1_8x8 = vreinterpret_u8_u32(c1_32x2);
   c1_16x8 = vmovl_u8(c1_8x8);

   c1_16x4 = vget_low_u16(c1_16x8);
#    else //COLSAME
/**
 * @brief NEON non-smooth scaling with interpolated color multipliers.
 */
   uint16x4_t c2_16x4;
   uint16x4_t c2_c1_16x4;
   uint16x4_t c2_c1_local_16x4;
   uint16x4_t cv_16x4;
   uint16x8_t c1_c2_16x8;
   uint16x8_t val1_16x8;
   uint32x2_t c1_c2_32x2 = { 0, 0 };
   uint8x8_t c1_c2_8x8;

   c1_c2_32x2 = vset_lane_u32(c1, c1_c2_32x2, 0);
   c1_c2_32x2 = vset_lane_u32(c2, c1_c2_32x2, 1);

   c1_c2_8x8 = vreinterpret_u8_u32(c1_c2_32x2);
   c1_c2_16x8 = vmovl_u8(c1_c2_8x8);

   c1_16x4 = vget_low_u16(c1_c2_16x8);
   c2_16x4 = vget_high_u16(c1_c2_16x8);

   c2_c1_16x4 = vsub_u16(c2_16x4, c1_16x4);
#    endif //COLSAME
#   endif //COLMUL
#  endif //COLBLACK
# endif //SCALE_USING_NEON

   /**
    * @brief Main pixel processing loop for non-smooth scaling.
    *
    * Iterates `ww` times, processing one output pixel per iteration.
    */
   while (ww > 0)
     {
# ifndef SCALE_USING_NEON
     /**
      * @brief Non-NEON path for non-smooth scaling (likely generic C or MMX if enabled elsewhere).
      * Variable setup for color multiplication if active.
      */
#  ifdef COLMUL
#   ifndef COLBLACK
        DATA32 val1; // Source pixel
#    ifndef COLSAME
        DATA32 cval; // col: Calculated color multiplier
#    endif //COLSAME
#   endif //COLBLACK
#  endif //COLMUL
# endif //SCALE_USING_NEON

# ifdef COLBLACK
        /**
         * @brief Fill with black color if COLBLACK is defined.
         */
        *d = 0xff000000; // col
# else //COLBLACK
        /**
         * @brief Nearest neighbor sampling and color processing.
         */
        s = sp + ((v >> (FP + FPI)) * sw) + (u >> (FP + FPI)); // Calculate source pixel address
#  ifdef COLMUL
#   ifdef SCALE_USING_NEON
        /**
         * @brief NEON non-smooth scaling with color multiplication.
         */
#    ifdef COLSAME
        /**
         * @brief NEON non-smooth: Apply single color multiplier c1.
         * Fetches source pixel *s, then multiplies by c1.
         */
        // Load source pixel *s into a NEON register
        val1_32x2 = vset_lane_u32(*s, val1_32x2, 0);
        val1_8x8 = vreinterpret_u8_u32(val1_32x2); // [sB, sG, sR, sA, 0,0,0,0] (u8)
        val1_16x8 = vmovl_u8(val1_8x8);            // Widen to u16
        val1_16x4 = vget_low_u16(val1_16x8);       // [sB, sG, sR, sA] (u16)
        cval_16x4 = c1_16x4;                       // Use c1 as the color multiplier
#    else //COLSAME
        /**
         * @brief NEON non-smooth: Apply interpolated color multiplier.
         * Fetches source pixel *s, interpolates color multiplier, then multiplies.
         */
        // Interpolate color: cval = c1 + (c2_c1_val * (cv>>16)) / 256
        cv_16x4 = vdup_n_u16(cv>>16); // Interpolation factor for color
        cv += cd; // col

        c2_c1_local_16x4 = vmul_u16(c2_c1_16x4, cv_16x4); // (c2-c1) * factor
        c2_c1_local_16x4 = vshr_n_u16(c2_c1_local_16x4, 8);
        c2_c1_local_16x4 = vadd_u16(c2_c1_local_16x4, c1_16x4);
        cval_16x4 = vand_u16(c2_c1_local_16x4, x255_16x4);
        val1_32x2 = vset_lane_u32(*s, val1_32x2, 0);
        val1_8x8 = vreinterpret_u8_u32(val1_32x2);
        val1_16x8 = vmovl_u8(val1_8x8);
        val1_16x4 = vget_low_u16(val1_16x8);
#    endif //COLSAME
        // Multiply result by source pixel: (cval * val1 + 255) >> 8
        cval_16x4 = vmul_u16(cval_16x4, val1_16x4);       // cval * val1
        cval_16x4 = vadd_u16(cval_16x4, x255_16x4);       // + 255 (for rounding)

        cval_16x8 = vcombine_u16(cval_16x4, temp_16x4); // Combine for narrowing

        cval_8x8 = vshrn_n_u16(cval_16x8, 8);           // Narrow and shift (divide by 256)
        res_32x2 = vreinterpret_u32_u8(cval_8x8);       // Reinterpret for storing

        vst1_lane_u32(d, res_32x2, 0);                  // Store result
#   else //SCALE_USING_NEON (Generic C or MMX for non-smooth with COLMUL)
        /**
         * @brief Generic C non-smooth scaling with color multiplication.
         */
        val1 = *s; // col: Fetch source pixel
#    ifdef COLSAME
        /**
         * @brief Generic C non-smooth: Apply single color multiplier c1.
         */
        *d = MUL4_SYM(c1, val1);
#    else //COLSAME
        /**
         * @brief Generic C non-smooth: Apply interpolated color multiplier.
         */
        cval = INTERP_256((cv >> 16), c2, c1); // col: Interpolate color multiplier
        *d = MUL4_SYM(cval, val1);             // Apply multiplier
        cv += cd; // col
#    endif //COLSAME
#   endif //SCALE_USING_NEON
#  else //COLMUL (No color multiplication for non-smooth)
        /**
         * @brief Non-smooth scaling without color multiplication.
         * Simply copy the source pixel.
         */
        *d = *s;
#  endif //COLMUL
        u += ud; // Increment source u coordinate
        v += vd; // Increment source v coordinate
# endif //COLBLACK
        d++;    // Move to next destination pixel
        ww--;   // Decrement remaining width
     }
}
#endif //SMOOTH
