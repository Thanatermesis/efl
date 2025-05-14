/* copy pixel x mask --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies pixels from source to destination, modulated by a mask.
 * @param s Pointer to the source pixel data (DATA32 array).
 *          Each element is a 32-bit pixel (e.g., ARGB).
 * @param m Pointer to the mask data (DATA8 array).
 *          Each element is an 8-bit alpha value (0-255).
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (DATA32 array).
 *          Each element is a 32-bit pixel (e.g., ARGB).
 * @param l Length of the pixel span to process.
 *
 * This function iterates over 'l' pixels. For each pixel:
 * - If mask value is 0, destination pixel is unchanged.
 * - If mask value is 255, source pixel is copied to destination.
 * - Otherwise, destination pixel is an interpolation between source and
 *   original destination, weighted by the mask value.
 * This is a NEON-optimized version, though the current implementation
 * is a C fallback (FIXME: neon-it).
 */
static void
_op_copy_p_mas_dp_neon(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   int color;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        color = *m;
                        switch(color)
                          {
                          case 0:
                             break;
                          case 255:
                             *d = *s;
                             break;
                          default:
                             color++;
                             *d = INTERP_256(color, *s, *d);
                             break;
                          }
                        m++;  s++;  d++;
                     });
}

#define _op_copy_pan_mas_dp_neon _op_copy_p_mas_dp_neon
#define _op_copy_pas_mas_dp_neon _op_copy_p_mas_dp_neon

#define _op_copy_p_mas_dpan_neon _op_copy_p_mas_dp_neon
#define _op_copy_pan_mas_dpan_neon _op_copy_p_mas_dpan_neon
#define _op_copy_pas_mas_dpan_neon _op_copy_p_mas_dpan_neon

/**
 * @brief Initializes function pointers for NEON-optimized span copy operations
 *        with pixel mask.
 *
 * This function assigns the `_op_copy_p_mas_dp_neon` (and its aliases)
 * to the appropriate entries in the `op_copy_span_funcs` table. This table
 * is likely used for dispatching to the correct copy routine based on
 * source, mask, destination, and CPU capabilities.
 *
 * - SP: Source Pixels
 * - SM_AS: Source Mask (Alpha Solid)
 * - SC_N: Source Color (Not used)
 * - DP: Destination Pixels
 * - DP_AN: Destination Pixels (Alpha Not relevant)
 * - CPU_NEON: NEON instruction set available
 *
 * Aliases like `_op_copy_pan_mas_dp_neon` handle variations in source
 * pixel properties (e.g., SP_AN for source pixels with alpha not relevant).
 */
static void
init_copy_pixel_mask_span_funcs_neon(void)
{
   op_copy_span_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_p_mas_dp_neon;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pan_mas_dp_neon;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pas_mas_dp_neon;

   op_copy_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_p_mas_dpan_neon;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pan_mas_dpan_neon;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pas_mas_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single source pixel to a destination pixel, modulated by a mask value.
 * @param s Source pixel data (DATA32).
 * @param m Mask value (DATA8, 0-255).
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (DATA32).
 *
 * The destination pixel is an interpolation between the source pixel and the
 * original destination pixel, weighted by the mask value `m`.
 * `m + 1` is used for the interpolation factor, likely to map 0-255 to 1-256 steps.
 * This is a NEON-optimized version.
 */
static void
_op_copy_pt_p_mas_dp_neon(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = INTERP_256(m + 1, s, *d);
}

#define _op_copy_pt_pan_mas_dp_neon _op_copy_pt_p_mas_dp_neon
#define _op_copy_pt_pas_mas_dp_neon _op_copy_pt_p_mas_dp_neon

#define _op_copy_pt_p_mas_dpan_neon _op_copy_pt_p_mas_dp_neon
#define _op_copy_pt_pan_mas_dpan_neon _op_copy_pt_p_mas_dpan_neon
#define _op_copy_pt_pas_mas_dpan_neon _op_copy_pt_p_mas_dpan_neon

/**
 * @brief Initializes function pointers for NEON-optimized point (single pixel)
 *        copy operations with pixel mask.
 *
 * This function assigns `_op_copy_pt_p_mas_dp_neon` (and its aliases)
 * to the `op_copy_pt_funcs` table. This table is used for dispatching
 * single-pixel copy operations based on various properties.
 *
 * See `init_copy_pixel_mask_span_funcs_neon` for an explanation of the
 * array indices like SP, SM_AS, etc.
 */
static void
init_copy_pixel_mask_pt_funcs_neon(void)
{
   op_copy_pt_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pt_p_mas_dp_neon;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pt_pan_mas_dp_neon;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_pt_pas_mas_dp_neon;

   op_copy_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_p_mas_dpan_neon;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_pan_mas_dpan_neon;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_pt_pas_mas_dpan_neon;
}
#endif

/*-----*/

/* copy_rel pixel x mask --> dst */

#ifdef BUILD_NEON
/**
 * @brief Copies pixels from source to destination, modulated by a mask,
 *        with relative (multiplicative) blending.
 * @param s Pointer to the source pixel data (DATA32 array).
 * @param m Pointer to the mask data (DATA8 array).
 * @param c Unused color parameter (reused locally).
 * @param d Pointer to the destination pixel data (DATA32 array).
 * @param l Length of the pixel span to process (reused locally).
 *
 * This function iterates over 'l_orig' pixels (parameter 'l' is shadowed).
 * For each pixel:
 * - If mask value is 0, destination pixel is unchanged.
 * - If mask value is 255, destination pixel is the source pixel multiplied
 *   symetrically by the destination's alpha channel (`*d >> 24`).
 * - Otherwise, an intermediate color `c_local` is calculated by multiplying
 *   source by destination alpha. Then, the destination pixel is an
 *   interpolation between `c_local` and original destination, weighted by
 *   the mask value (`l_local++`, which is `color + 1`).
 * This is a NEON-optimized version, though the current implementation
 * is a C fallback (FIXME: neon-it).
 * Note: The variable `l` (length) is reused as `l_local` for the interpolation factor,
 * and `c` (color) is reused as `c_local` for the intermediate color. This can be confusing.
 */
static void
_op_copy_rel_p_mas_dp_neon(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   // FIXME: neon-it
   DATA32 *e;
   int color;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        color = *m;
                        switch(color)
                          {
                          case 0:
                             break;
                          case 255:
                             *d = MUL_SYM(*d >> 24, *s);
                             break;
                          default:
                             c = MUL_SYM(*d >> 24, *s);
                             l++;
                             *d = INTERP_256(l, c, *d);
                             break;
                          }
                        m++;  s++;  d++;
                     });
}

#define _op_copy_rel_pan_mas_dp_neon _op_copy_rel_p_mas_dp_neon
#define _op_copy_rel_pas_mas_dp_neon _op_copy_rel_p_mas_dp_neon

#define _op_copy_rel_p_mas_dpan_neon _op_copy_p_mas_dpan_neon
#define _op_copy_rel_pan_mas_dpan_neon _op_copy_pan_mas_dpan_neon
#define _op_copy_rel_pas_mas_dpan_neon _op_copy_pas_mas_dpan_neon

/**
 * @brief Initializes function pointers for NEON-optimized relative span copy
 *        operations with pixel mask.
 *
 * This function assigns `_op_copy_rel_p_mas_dp_neon` (and its aliases)
 * to the `op_copy_rel_span_funcs` table for dispatching relative copy routines.
 * "Relative" typically implies a multiplicative blend mode.
 *
 * See `init_copy_pixel_mask_span_funcs_neon` for an explanation of the
 * array indices like SP, SM_AS, etc.
 */
static void
init_copy_rel_pixel_mask_span_funcs_neon(void)
{
   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_p_mas_dp_neon;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pan_mas_dp_neon;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pas_mas_dp_neon;

   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_p_mas_dpan_neon;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pan_mas_dpan_neon;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pas_mas_dpan_neon;
}
#endif

#ifdef BUILD_NEON
/**
 * @brief Copies a single source pixel to a destination pixel, modulated by a mask,
 *        with relative (multiplicative) blending for a single point.
 * @param s Source pixel data (DATA32).
 * @param m Mask value (DATA8, 0-255).
 * @param c Intermediate color value (DATA32), calculated based on source and
 *          destination alpha. Note: This parameter is effectively an output of
 *          the first operation and input to the second.
 * @param d Pointer to the destination pixel data (DATA32).
 *
 * First, an intermediate color `c` is calculated by multiplying the source
 * pixel `s` symetrically with the destination's alpha channel (`*d >> 24`).
 * Then, the destination pixel `*d` is an interpolation between this
 * intermediate color `c` and the original destination pixel `*d`, weighted
 * by the mask value `m + 1`.
 * This is a NEON-optimized version.
 */
static void
_op_copy_rel_pt_p_mas_dp_neon(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   c = MUL_SYM(*d >> 24, s);
   *d = INTERP_256(m + 1, c, *d);
}


#define _op_copy_rel_pt_pan_mas_dp_neon _op_copy_rel_pt_p_mas_dp_neon
#define _op_copy_rel_pt_pas_mas_dp_neon _op_copy_rel_pt_p_mas_dp_neon

#define _op_copy_rel_pt_p_mas_dpan_neon _op_copy_pt_p_mas_dpan_neon
#define _op_copy_rel_pt_pan_mas_dpan_neon _op_copy_pt_pan_mas_dpan_neon
#define _op_copy_rel_pt_pas_mas_dpan_neon _op_copy_pt_pas_mas_dpan_neon

/**
 * @brief Initializes function pointers for NEON-optimized relative point (single pixel)
 *        copy operations with pixel mask.
 *
 * This function assigns `_op_copy_rel_pt_p_mas_dp_neon` (and its aliases)
 * to the `op_copy_rel_pt_funcs` table for dispatching single-pixel relative
 * copy operations.
 *
 * See `init_copy_pixel_mask_span_funcs_neon` for an explanation of the
 * array indices like SP, SM_AS, etc.
 */
static void
init_copy_rel_pixel_mask_pt_funcs_neon(void)
{
   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_p_mas_dp_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_pan_mas_dp_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_NEON] = _op_copy_rel_pt_pas_mas_dp_neon;

   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_p_mas_dpan_neon;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_pan_mas_dpan_neon;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_NEON] = _op_copy_rel_pt_pas_mas_dpan_neon;
}
#endif

