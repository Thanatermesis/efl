/**
 * @file op_copy_mask_color_.c
 * @brief Functions for copying a color to a destination, modulated by a mask.
 *
 * These functions handle operations where a source color is multiplied
 * by a mask and then written to a destination buffer. Variations handle
 * different pixel formats and alpha blending scenarios.
 */

/* copy mask x color -> dst */

/**
 * @brief Copies a color multiplied by a mask to a destination buffer (span operation).
 *
 * This function iterates over a span of `l` pixels. For each pixel,
 * it takes a mask value `m`, a color `c`, and applies `m * c` to the
 * destination pixel `d`.
 *
 * The source `s` is unused in this specific operation.
 *
 * The core logic is:
 * - If mask alpha is 0, destination is 0.
 * - If mask alpha is 255, destination is `c`.
 * - Otherwise, destination is `(mask_alpha + 1) * c / 256`.
 *
 * @param s Pointer to source data (unused).
 * @param m Pointer to mask data (array of DATA8, one byte per pixel).
 *          Example: `m[0]` is the mask for the first pixel.
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to destination data (array of DATA32, one dword per pixel).
 *          Example: `d[0]` is the destination for the first pixel.
 * @param l Length of the span in pixels.
 */
static void
_op_copy_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   int alpha;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = m*c */
                        alpha = *m;
                        switch(alpha)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             *d = c;
                             break;
                          default:
                             alpha++;
                             *d = MUL_256(alpha, c);
                             break;
                          }
                        m++;  d++;
                     });
}

#define _op_copy_mas_cn_dp _op_copy_mas_c_dp
#define _op_copy_mas_can_dp _op_copy_mas_c_dp
#define _op_copy_mas_caa_dp _op_copy_mas_c_dp

#define _op_copy_mas_c_dpan _op_copy_mas_c_dp
#define _op_copy_mas_cn_dpan _op_copy_mas_c_dpan
#define _op_copy_mas_can_dpan _op_copy_mas_c_dpan
#define _op_copy_mas_caa_dpan _op_copy_mas_c_dpan

/**
 * @brief Initializes the span-based copy_mask_color function pointers for C CPU.
 *
 * This function assigns the appropriate `_op_copy_mas_c_dp` (and its aliases)
 * to the global `op_copy_span_funcs` array for various source, mask,
 * and destination color/alpha configurations.
 * - `SP_N`: Source Pixels are Not used.
 * - `SM_AS`: Source Mask is Alpha Solid (meaning mask is a simple alpha channel).
 * - `SC_N`, `SC`, `SC_AN`, `SC_AA`: Source Color configurations (None, Color, Alpha Not Premultiplied, Alpha Premultiplied).
 * - `DP`, `DP_AN`: Destination Pixel configurations (Premultiplied, Alpha Not Premultiplied).
 * - `CPU_C`: C language implementation.
 */
static void
init_copy_mask_color_span_funcs_c(void)
{
   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_mas_cn_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_mas_c_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_mas_can_dp;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_mas_caa_dp;

   op_copy_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_mas_cn_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_mas_c_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_mas_can_dpan;
   op_copy_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_mas_caa_dpan;
}

/**
 * @brief Copies a color multiplied by a mask to a single destination pixel (point operation).
 *
 * This function applies the operation `d = INTERP_256(m + 1, c, *d)` which means
 * `*d = ( (m+1) * c + (256 - (m+1)) * (*d) ) / 256`.
 * It blends the color `c` (modulated by mask `m`) with the existing destination pixel `*d`.
 *
 * The source `s` is unused in this specific operation.
 *
 * @param s Source data (unused).
 * @param m Mask value (DATA8).
 * @param c The color to apply (DATA32, typically ARGB).
 * @param d Pointer to the destination pixel (DATA32).
 */
static void
_op_copy_pt_mas_c_dp(DATA32 s EINA_UNUSED, DATA8 m, DATA32 c, DATA32 *d) {
   *d = INTERP_256(m + 1, c, *d);
}


#define _op_copy_pt_mas_cn_dp _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_can_dp _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_caa_dp _op_copy_pt_mas_c_dp

#define _op_copy_pt_mas_c_dpan _op_copy_pt_mas_c_dp
#define _op_copy_pt_mas_cn_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_pt_mas_can_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_pt_mas_caa_dpan _op_copy_pt_mas_c_dpan

/**
 * @brief Initializes the point-based copy_mask_color function pointers for C CPU.
 *
 * This function assigns the appropriate `_op_copy_pt_mas_c_dp` (and its aliases)
 * to the global `op_copy_pt_funcs` array for various source, mask,
 * and destination color/alpha configurations.
 * - `SP_N`: Source Pixels are Not used.
 * - `SM_AS`: Source Mask is Alpha Solid.
 * - `SC_N`, `SC`, `SC_AN`, `SC_AA`: Source Color configurations.
 * - `DP`, `DP_AN`: Destination Pixel configurations.
 * - `CPU_C`: C language implementation.
 */
static void
init_copy_mask_color_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_pt_mas_cn_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_pt_mas_c_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_pt_mas_can_dp;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_pt_mas_caa_dp;

   op_copy_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pt_mas_cn_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_pt_mas_c_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_pt_mas_can_dpan;
   op_copy_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_pt_mas_caa_dpan;
}

/*-----*/

/* copy_rel mask x color -> dst */

/**
 * @brief Copies a color multiplied by a mask to a destination buffer, relative to destination alpha (span operation).
 *
 * This function iterates over a span of `l` pixels. For each pixel,
 * it takes a mask value `m`, a color `c`, and the destination pixel `d`.
 * The operation is `d = (m*c) * destination_alpha`.
 *
 * The source `s` is unused.
 *
 * The core logic is:
 * - If mask value (`color` variable in code) is 0, destination is 0.
 * - If mask value is 255, `d = ( (1 + (*d >> 24)) * c ) / 256`.
 * - Otherwise, `d = INTERP_256(mask_value + 1, ( (1 + (*d >> 24)) * c ) / 256, *d)`.
 *
 * @note This function has a "FIXME" comment indicating it might not have been thoroughly tested.
 *
 * @param s Pointer to source data (unused).
 * @param m Pointer to mask data (array of DATA8).
 * @param c The color to apply (DATA32).
 * @param d Pointer to destination data (array of DATA32).
 * @param l Length of the span in pixels.
 */
static void
_op_copy_rel_mas_c_dp(DATA32 *s EINA_UNUSED, DATA8 *m, DATA32 c, DATA32 *d, int l) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   DATA32 *e;
   int color;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        /* d = (m*c)*da */
                        color = *m;
                        switch(color)
                          {
                          case 0:
                             *d = 0;
                             break;
                          case 255:
                             color = 1 + (*d >> 24);
                             *d = MUL_256(color, c);
                             break;
                          default:
                               {
                                  DATA32 da = 1 + (*d >> 24);
                                  da = MUL_256(da, c);
                                  color++;
                                  *d = INTERP_256(color, da, *d);
                               }
                             break;
                          }
                        m++;  d++;
                     });
}


#define _op_copy_rel_mas_cn_dp _op_copy_rel_mas_c_dp
#define _op_copy_rel_mas_can_dp _op_copy_rel_mas_c_dp
#define _op_copy_rel_mas_caa_dp _op_copy_rel_mas_c_dp

#define _op_copy_rel_mas_c_dpan _op_copy_mas_c_dpan
#define _op_copy_rel_mas_cn_dpan _op_copy_mas_cn_dpan
#define _op_copy_rel_mas_can_dpan _op_copy_mas_can_dpan
#define _op_copy_rel_mas_caa_dpan _op_copy_mas_caa_dpan

/**
 * @brief Initializes the span-based copy_rel_mask_color function pointers for C CPU.
 *
 * This function assigns `_op_copy_rel_mas_c_dp` (and its aliases)
 * to `op_copy_rel_span_funcs` for various configurations.
 * "rel" signifies that the operation is relative to the destination alpha.
 * See init_copy_mask_color_span_funcs_c() for parameter meanings.
 */
static void
init_copy_rel_mask_color_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_mas_cn_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_rel_mas_c_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_rel_mas_can_dp;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_rel_mas_caa_dp;

   op_copy_rel_span_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_mas_cn_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_rel_mas_c_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_rel_mas_can_dpan;
   op_copy_rel_span_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_rel_mas_caa_dpan;
}

/**
 * @brief Copies a color multiplied by a mask to a single destination pixel, relative to destination alpha (point operation).
 *
 * This function applies an operation where the color `c` is first modulated by
 * the destination's alpha, then blended with the destination pixel `*d` using the mask `m`.
 * The logic is: `s_intermediate = ( (1 + (*d >> 24)) * c ) / 256;`
 * Then: `*d = INTERP_256(m + 1, s_intermediate, *d);`
 *
 * @note This function has a "FIXME" comment indicating it might not have been thoroughly tested.
 * @param s Source data (actually used here to store intermediate destination alpha `1 + (*d >> 24)`).
 * @param m Mask value (DATA8).
 * @param c The color to apply (DATA32).
 * @param d Pointer to the destination pixel (DATA32).
 */
static void
_op_copy_rel_pt_mas_c_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   /* FIXME: THIS FUNCTION HAS PROBABLY NEVER BEEN TESTED */
   s = 1 + (*d >> 24);
   s = MUL_256(s, c);
   *d = INTERP_256(m + 1, s, *d);
}

#define _op_copy_rel_pt_mas_cn_dp _op_copy_rel_pt_mas_c_dp
#define _op_copy_rel_pt_mas_can_dp _op_copy_rel_pt_mas_c_dp
#define _op_copy_rel_pt_mas_caa_dp _op_copy_rel_pt_mas_c_dp

#define _op_copy_rel_pt_mas_c_dpan _op_copy_pt_mas_c_dpan
#define _op_copy_rel_pt_mas_cn_dpan _op_copy_pt_mas_cn_dpan
#define _op_copy_rel_pt_mas_can_dpan _op_copy_pt_mas_can_dpan
#define _op_copy_rel_pt_mas_caa_dpan _op_copy_pt_mas_caa_dpan

/**
 * @brief Initializes the point-based copy_rel_mask_color function pointers for C CPU.
 *
 * This function assigns `_op_copy_rel_pt_mas_c_dp` (and its aliases)
 * to `op_copy_rel_pt_funcs` for various configurations.
 * "rel" signifies that the operation is relative to the destination alpha.
 * See init_copy_mask_color_pt_funcs_c() for parameter meanings.
 */
static void
init_copy_rel_mask_color_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pt_mas_cn_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP][CPU_C] = _op_copy_rel_pt_mas_c_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP][CPU_C] = _op_copy_rel_pt_mas_can_dp;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP][CPU_C] = _op_copy_rel_pt_mas_caa_dp;

   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_mas_cn_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC][DP_AN][CPU_C] = _op_copy_rel_pt_mas_c_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AN][DP_AN][CPU_C] = _op_copy_rel_pt_mas_can_dpan;
   op_copy_rel_pt_funcs[SP_N][SM_AS][SC_AA][DP_AN][CPU_C] = _op_copy_rel_pt_mas_caa_dpan;
}
