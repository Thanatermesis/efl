/**
 * @file
 * @brief NEON optimized pixel copy operations.
 *
 * This file contains NEON-specific implementations for various pixel
 * copying operations, including direct copy and relative copy, for
 * both span and point operations.
 */

/* copy pixel --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a span of pixels from source to destination using NEON.
 *
 * This function performs a direct memory copy of pixel data.
 * If USENEON is defined, it uses NEON assembly for optimized copying
 * of blocks of 24 pixels (3 * 8 pixels, where 8 pixels = 32 bytes = 256 bits).
 * Remaining pixels are copied using a simple loop.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l Number of pixels to copy.
 */
static void
_op_copy_p_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
//#define USENEON 1
#ifndef USENEON
   memcpy(d, s, l * sizeof(DATA32));
   return;
#else
   DATA32 *e;
   e = d + l - 23;
   if (e > d)
     {
        int dl;

        asm volatile
        (".fpu neon \n\t"
            "_op_copy_p_dp_neon_asmloop: \n\t"
            "pld     [%[s], #192]      \n\t" // preload 256 bytes ahead
            "pld     [%[s], #320]      \n\t" // preload 320 bytes ahead
            "vld1.32 {d0-d3},  [%[s]]! \n\t" // load 256bits (32 bytes 8 pix), 32bit aligned
            "vld1.32 {d4-d7} , [%[s]]! \n\t" // load 256bits (32 bytes 8 pix), 32bit aligned
            "vld1.32 {d8-d11}, [%[s]]! \n\t" // load 256bits (32 bytes 8 pix), 32bit aligned
            "vst1.32 {d0-d3},  [%[d]]! \n\t" // store 256bits (32 bytes 8 pix), 32bit aligned
            "vst1.32 {d4-d7},  [%[d]]! \n\t" // store 256bits (32 bytes 8 pix), 32bit aligned
            "vst1.32 {d8-d11}, [%[d]]! \n\t" // store 256bits (32 bytes 8 pix), 32bit aligned
            "cmp     %[e], %[d]        \n\t" // compare current and end ptr
            "bgt     _op_copy_p_dp_neon_asmloop \n\t"
          : /*out*/
          : /*in */ [s] "r" (s), [e] "r" (e), [d] "r" (d)
          : /*clobber*/
            "q0", "q1", "q2","q3", "q4", "q5", "q6",
            "d0", "d1", "d2", "d3",
            "d4", "d5", "d6", "d7",
            "d8", "d9", "d10", "d11",
            "memory" // clobbered
        );
        dl = l % 24; // dl is how many pixels at end that is not a multiple of 24
        l = l - dl; // jump to there at the end of the run?
        s = s + l;
        d = d + l;
     }
   e += 23;
   for (;d < e; d++, s++) *d = *s;
#endif
}

// Defines for variants of the copy operation (e.g., with alpha)
#define _op_copy_pan_dp_neon _op_copy_p_dp_neon
#define _op_copy_pas_dp_neon _op_copy_p_dp_neon

#define _op_copy_p_dpan_neon _op_copy_p_dp_neon
#define _op_copy_pan_dpan_neon _op_copy_pan_dp_neon
#define _op_copy_pas_dpan_neon _op_copy_pas_dp_neon

/**
 * @brief Initializes the function pointers for NEON-optimized pixel span copy operations.
 *
 * This function assigns the NEON-specific copy functions to the
 * global function pointer array `op_copy_span_funcs`. This allows
 * the graphics engine to dynamically select the optimized NEON
 * implementations at runtime when available.
 *
 * `op_copy_span_funcs` is expected to be a multi-dimensional array where
 * dimensions might represent:
 * - Source pixel properties (e.g., SP, SP_AN, SP_AS for solid, alpha no-mul, alpha-solid)
 * - Source mask mode (e.g., SM_N for no mask)
 * - Source color/alpha (e.g., SC_N for no specific color)
 * - Destination pixel properties (e.g., DP, DP_AN for solid, alpha no-mul)
 * - CPU capabilities (e.g., CPU_NEON)
 *
 * Example element structure for `op_copy_span_funcs`:
 * `op_copy_span_funcs[source_prop][mask_mode][source_color_alpha][dest_prop][cpu_type]`
 */
static void
init_copy_pixel_span_funcs_neon(void)
{
   op_copy_span_funcs[SP][SM_N][SC_N][DP][CPU_NEON] = _op_copy_p_dp_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pan_dp_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pas_dp_neon;

   op_copy_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_p_dpan_neon;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pan_dpan_neon;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pas_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single pixel from source to destination.
 *
 * This is a point operation, copying one DATA32 pixel.
 *
 * @param s Source pixel data (DATA32).
 * @param m Mask value (unused).
 * @param c Color value (unused).
 * @param d Pointer to the destination pixel data (DATA32).
 */
static void
_op_copy_pt_p_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = s;
}

#define _op_copy_pt_pan_dp_neon _op_copy_pt_p_dp_neon
#define _op_copy_pt_pas_dp_neon _op_copy_pt_p_dp_neon

#define _op_copy_pt_p_dpan_neon _op_copy_pt_p_dp_neon
#define _op_copy_pt_pan_dpan_neon _op_copy_pt_pan_dp_neon
#define _op_copy_pt_pas_dpan_neon _op_copy_pt_pas_dp_neon

/**
 * @brief Initializes the function pointers for NEON-optimized single pixel (point) copy operations.
 *
 * This function assigns the NEON-specific point copy functions to the
 * global function pointer array `op_copy_pt_funcs`. This allows
 * the graphics engine to dynamically select the optimized NEON
 * implementations at runtime when available.
 *
 * `op_copy_pt_funcs` structure is similar to `op_copy_span_funcs` but for
 * point operations.
 */
static void
init_copy_pixel_pt_funcs_neon(void)
{
   op_copy_pt_funcs[SP][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pt_p_dp_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pt_pan_dp_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_NEON] = _op_copy_pt_pas_dp_neon;

   op_copy_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_p_dpan_neon;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_pan_dpan_neon;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_pas_dpan_neon;
}
#endif

/*-----*/

/* copy_rel pixel --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies a span of pixels relative to destination alpha using NEON (placeholder).
 *
 * This function is intended to copy pixel data, modulating the result
 * by the destination alpha. The current implementation is a placeholder
 * and marked with "FIXME: neon-it", indicating that a NEON-optimized
 * version is pending.
 * The operation performed is `*d = MUL_SYM(*d >> 24, c);` for each pixel.
 *
 * @param s Pointer to the source pixel data (unused).
 * @param m Pointer to the mask data (unused).
 * @param c Color value (used in the MUL_SYM operation, likely the source color).
 * @param d Pointer to the destination pixel data (array of DATA32), read and written.
 * @param l Number of pixels to process.
 */
static void
_op_copy_rel_p_dp_neon(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL_SYM(*d >> 24, c);
                        d++;
                     });
}


#define _op_copy_rel_pas_dp_neon _op_copy_rel_p_dp_neon
#define _op_copy_rel_pan_dp_neon _op_copy_rel_p_dp_neon

#define _op_copy_rel_p_dpan_neon _op_copy_p_dpan_neon
#define _op_copy_rel_pan_dpan_neon _op_copy_pan_dpan_neon
#define _op_copy_rel_pas_dpan_neon _op_copy_pas_dpan_neon

/**
 * @brief Initializes the function pointers for NEON-optimized relative pixel span copy operations.
 *
 * This function assigns the NEON-specific relative copy functions to the
 * global function pointer array `op_copy_rel_span_funcs`.
 *
 * `op_copy_rel_span_funcs` structure is similar to `op_copy_span_funcs`.
 */
static void
init_copy_rel_pixel_span_funcs_neon(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_p_dp_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pan_dp_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pas_dp_neon;

   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_p_dpan_neon;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pan_dpan_neon;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pas_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single pixel relative to destination alpha.
 *
 * This function calculates a new destination pixel value based on its
 * original alpha component and a source color `c`.
 * The operation is:
 *   `temp_alpha = 1 + (*d >> 24);` (destination alpha + 1)
 *   `*d = MUL_256(temp_alpha, c);` (multiply temp_alpha by color `c`, scaled by 256)
 *
 * @param s Source pixel data (unused, but typically would be the source color).
 *          In this specific implementation, `c` is used as the source color.
 * @param m Mask value (unused).
 * @param c Source color value (DATA32).
 * @param d Pointer to the destination pixel data (DATA32), read and written.
 */
static void
_op_copy_rel_pt_p_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = 1 + (*d >> 24); // s is reused here to store (destination_alpha + 1)
   *d = MUL_256(s, c);
}


#define _op_copy_rel_pt_pan_dp_neon _op_copy_rel_pt_p_dp_neon
#define _op_copy_rel_pt_pas_dp_neon _op_copy_rel_pt_p_dp_neon

#define _op_copy_rel_pt_p_dpan_neon _op_copy_pt_p_dpan_neon
#define _op_copy_rel_pt_pan_dpan_neon _op_copy_pt_pan_dpan_neon
#define _op_copy_rel_pt_pas_dpan_neon _op_copy_pt_pas_dpan_neon

/**
 * @brief Initializes the function pointers for NEON-optimized relative single pixel (point) copy operations.
 *
 * This function assigns the NEON-specific relative point copy functions to the
 * global function pointer array `op_copy_rel_pt_funcs`.
 *
 * `op_copy_rel_pt_funcs` structure is similar to `op_copy_pt_funcs`.
 */
static void
init_copy_rel_pixel_pt_funcs_neon(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_p_dp_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_pan_dp_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_pas_dp_neon;

   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_p_dpan_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_pan_dpan_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_pas_dpan_neon;
}
#endif
