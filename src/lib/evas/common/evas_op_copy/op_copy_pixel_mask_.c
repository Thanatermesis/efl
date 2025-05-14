/**
 * @file op_copy_pixel_mask_.c
 * @brief Functions for copying pixel data with a mask.
 *
 * These functions handle pixel copying operations where a mask is applied.
 * Different functions cater to span and point operations, as well as
 * normal and relative copy modes.
 */

/* copy pixel x mask --> dst */

/**
 * @brief Copies a span of pixels from source to destination, applying a mask.
 *
 * This function iterates over a length 'l', processing each pixel.
 * The mask value determines how the source pixel is blended with the
 * destination pixel.
 * - If mask is 0, destination pixel is unchanged.
 * - If mask is 255, source pixel overwrites destination.
 * - Otherwise, pixels are interpolated based on the mask value.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 *          Example: `s[0]` is the first source pixel.
 * @param m Pointer to the mask data (array of DATA8).
 *          Example: `m[0]` is the mask for `s[0]`. Mask values range from 0 (transparent) to 255 (opaque).
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (array of DATA32).
 *          Example: `d[0]` is the first destination pixel. This array is modified.
 * @param l The number of pixels to process.
 */
static void
_op_copy_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
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


#define _op_copy_pan_mas_dp _op_copy_p_mas_dp
#define _op_copy_pas_mas_dp _op_copy_p_mas_dp

#define _op_copy_p_mas_dpan _op_copy_p_mas_dp
#define _op_copy_pan_mas_dpan _op_copy_p_mas_dpan
#define _op_copy_pas_mas_dpan _op_copy_p_mas_dpan

/**
 * @brief Initializes function pointers for span copy operations with pixel mask.
 *
 * This function assigns the appropriate version of the span copy function
 * (e.g., _op_copy_p_mas_dp) to an array of function pointers (op_copy_span_funcs).
 * The indices (SP, SM_AS, SC_N, DP, CPU_C, etc.) represent different
 * configurations or states for the copy operation, such as source/destination
 * properties or CPU capabilities.
 */
static void
init_copy_pixel_mask_span_funcs_c(void)
{
   op_copy_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_copy_p_mas_dp;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_copy_pan_mas_dp;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_copy_pas_mas_dp;

   op_copy_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_p_mas_dpan;
   op_copy_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pan_mas_dpan;
   op_copy_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pas_mas_dpan;
}

/**
 * @brief Copies a single pixel from source to destination, applying a mask.
 *
 * This function processes a single pixel. The source pixel 's' is
 * interpolated with the destination pixel '*d' based on the mask value 'm'.
 * The mask value 'm' is incremented by 1 before being used in INTERP_256,
 * likely to scale it correctly for the interpolation macro.
 *
 * @param s The source pixel data (DATA32).
 * @param m The mask value (DATA8). Ranges from 0 to 255.
 * @param c Unused color parameter.
 * @param d Pointer to the destination pixel data (DATA32). This value is modified.
 */
static void
_op_copy_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = INTERP_256(m + 1, s, *d);
}

#define _op_copy_pt_pan_mas_dp _op_copy_pt_p_mas_dp
#define _op_copy_pt_pas_mas_dp _op_copy_pt_p_mas_dp

#define _op_copy_pt_p_mas_dpan _op_copy_pt_p_mas_dp
#define _op_copy_pt_pan_mas_dpan _op_copy_pt_p_mas_dpan
#define _op_copy_pt_pas_mas_dpan _op_copy_pt_p_mas_dpan

/**
 * @brief Initializes function pointers for single point copy operations with pixel mask.
 *
 * Similar to init_copy_pixel_mask_span_funcs_c, this function assigns
 * the appropriate version of the point copy function (e.g., _op_copy_pt_p_mas_dp)
 * to an array of function pointers (op_copy_pt_funcs) for various configurations.
 */
static void
init_copy_pixel_mask_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_copy_pt_p_mas_dp;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_copy_pt_pan_mas_dp;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_copy_pt_pas_mas_dp;

   op_copy_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pt_p_mas_dpan;
   op_copy_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pt_pan_mas_dpan;
   op_copy_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_pt_pas_mas_dpan;
}

/*-----*/

/* copy_rel pixel x mask --> dst */

/**
 * @brief Copies a span of pixels from source to destination using relative addressing, applying a mask.
 *
 * This function is similar to _op_copy_p_mas_dp but performs a "relative" copy.
 * The "relative" aspect likely involves modulating the source pixel by the
 * destination alpha ( `*d >> 24` ) before blending.
 * - If mask is 0, destination pixel is unchanged.
 * - If mask is 255, the source pixel, modulated by destination alpha, overwrites destination.
 * - Otherwise, the modulated source pixel is interpolated with the destination pixel
 *   based on the mask value. The parameter 'c' is reused to store the modulated source,
 *   and 'l' (length parameter) is incremented within the default case, which seems unusual
 *   and might be a bug or specific logic requiring careful review.
 *
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (array of DATA8).
 * @param c A color value (DATA32), reused internally. Its initial value might be relevant.
 * @param d Pointer to the destination pixel data (array of DATA32). This array is modified.
 * @param l The number of pixels to process. This parameter is modified in one of the switch cases.
 */
static void
_op_copy_rel_p_mas_dp(DATA32 *s, DATA8 *m, DATA32 c, DATA32 *d, int l) {
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


#define _op_copy_rel_pan_mas_dp _op_copy_rel_p_mas_dp
#define _op_copy_rel_pas_mas_dp _op_copy_rel_p_mas_dp

#define _op_copy_rel_p_mas_dpan _op_copy_p_mas_dpan
#define _op_copy_rel_pan_mas_dpan _op_copy_pan_mas_dpan
#define _op_copy_rel_pas_mas_dpan _op_copy_pas_mas_dpan

/**
 * @brief Initializes function pointers for relative span copy operations with pixel mask.
 *
 * Assigns the appropriate version of the relative span copy function
 * (e.g., _op_copy_rel_p_mas_dp) to the op_copy_rel_span_funcs array
 * for various configurations.
 */
static void
init_copy_rel_pixel_mask_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_p_mas_dp;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pan_mas_dp;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pas_mas_dp;

   op_copy_rel_span_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_p_mas_dpan;
   op_copy_rel_span_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pan_mas_dpan;
   op_copy_rel_span_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pas_mas_dpan;
}

/**
 * @brief Copies a single pixel from source to destination using relative addressing, applying a mask.
 *
 * This function processes a single pixel in a "relative" manner.
 * The source pixel 's' is first modulated by the destination alpha (`*d >> 24`)
 * and stored in 'c'. Then, this modulated color 'c' is interpolated with the
 * destination pixel '*d' based on the mask value 'm'.
 * The mask value 'm' is incremented by 1 for the interpolation.
 *
 * @param s The source pixel data (DATA32).
 * @param m The mask value (DATA8).
 * @param c A color value (DATA32), used to store the intermediate modulated color.
 *          Its initial value is overwritten.
 * @param d Pointer to the destination pixel data (DATA32). This value is modified.
 */
static void
_op_copy_rel_pt_p_mas_dp(DATA32 s, DATA8 m, DATA32 c, DATA32 *d) {
   c = MUL_SYM(*d >> 24, s);
   *d = INTERP_256(m + 1, c, *d);
}


#define _op_copy_rel_pt_pan_mas_dp _op_copy_rel_pt_p_mas_dp
#define _op_copy_rel_pt_pas_mas_dp _op_copy_rel_pt_p_mas_dp

#define _op_copy_rel_pt_p_mas_dpan _op_copy_pt_p_mas_dpan
#define _op_copy_rel_pt_pan_mas_dpan _op_copy_pt_pan_mas_dpan
#define _op_copy_rel_pt_pas_mas_dpan _op_copy_pt_pas_mas_dpan

/**
 * @brief Initializes function pointers for relative single point copy operations with pixel mask.
 *
 * Assigns the appropriate version of the relative point copy function
 * (e.g., _op_copy_rel_pt_p_mas_dp) to the op_copy_rel_pt_funcs array
 * for various configurations.
 */
static void
init_copy_rel_pixel_mask_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pt_p_mas_dp;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pt_pan_mas_dp;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP][CPU_C] = _op_copy_rel_pt_pas_mas_dp;

   op_copy_rel_pt_funcs[SP][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_p_mas_dpan;
   op_copy_rel_pt_funcs[SP_AN][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_pan_mas_dpan;
   op_copy_rel_pt_funcs[SP_AS][SM_AS][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_pas_mas_dpan;
}
