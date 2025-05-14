/**
 * @file op_copy_pixel_.c
 * @brief Pixel copy operations.
 *
 * This file contains functions for copying pixel data.
 * It includes basic copy operations as well as relative copy operations.
 * These functions are optimized for different scenarios like span and point operations.
 */

/* copy pixel --> dst */

/**
 * @brief Copies a span of pixels from source to destination.
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (array of DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l Length of the pixel span to copy.
 *
 * This function directly copies 'l' pixels from the source buffer 's'
 * to the destination buffer 'd'.
 */
static void
_op_copy_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   memcpy(d, s, l * sizeof(DATA32));
}

#define _op_copy_pan_dp _op_copy_p_dp
#define _op_copy_pas_dp _op_copy_p_dp

#define _op_copy_p_dpan _op_copy_p_dp
#define _op_copy_pan_dpan _op_copy_pan_dp
#define _op_copy_pas_dpan _op_copy_pas_dp

/**
 * @brief Initializes the span copy function pointers for C CPU.
 *
 * This function assigns the appropriate span copy functions
 * (e.g., _op_copy_p_dp, _op_copy_pan_dp) to the
 * op_copy_span_funcs array for different source, mask,
 * and destination pixel properties.
 */
static void
init_copy_pixel_span_funcs_c(void)
{
   op_copy_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_copy_p_dp;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_copy_pan_dp;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_copy_pas_dp;

   op_copy_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_p_dpan;
   op_copy_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pan_dpan;
   op_copy_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pas_dpan;
}

/**
 * @brief Copies a single pixel from source to destination.
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel (DATA32).
 *
 * This function copies the source pixel 's' to the destination pixel
 * pointed to by 'd'.
 */
static void
_op_copy_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d) {
   *d = s;
}

#define _op_copy_pt_pan_dp _op_copy_pt_p_dp
#define _op_copy_pt_pas_dp _op_copy_pt_p_dp

#define _op_copy_pt_p_dpan _op_copy_pt_p_dp
#define _op_copy_pt_pan_dpan _op_copy_pt_pan_dp
#define _op_copy_pt_pas_dpan _op_copy_pt_pas_dp

/**
 * @brief Initializes the point copy function pointers for C CPU.
 *
 * This function assigns the appropriate point copy functions
 * (e.g., _op_copy_pt_p_dp, _op_copy_pt_pan_dp) to the
 * op_copy_pt_funcs array for different source, mask,
 * and destination pixel properties.
 */
static void
init_copy_pixel_pt_funcs_c(void)
{
   op_copy_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_copy_pt_p_dp;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_copy_pt_pan_dp;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_copy_pt_pas_dp;

   op_copy_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pt_p_dpan;
   op_copy_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pt_pan_dpan;
   op_copy_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_pt_pas_dpan;
}

/*-----*/

/* copy_rel pixel --> dst */

/**
 * @brief Copies a span of pixels from source to destination, modulating by destination alpha.
 * @param s Pointer to the source pixel data (array of DATA32).
 * @param m Pointer to the mask data (array of DATA8, unused).
 * @param c Color value (DATA32, unused).
 * @param d Pointer to the destination pixel data (array of DATA32).
 * @param l Length of the pixel span to copy.
 *
 * This function copies 'l' pixels from 's' to 'd'.
 * The operation is a "relative" copy, meaning the source pixel's color
 * components are multiplied by the destination pixel's alpha component
 * (symbiotically).
 * Example: *d = MUL_SYM(*d >> 24, *s);
 *   - *d >> 24: extracts the alpha component of the destination pixel.
 *   - *s: the source pixel.
 *   - MUL_SYM: a macro that likely performs a multiplication and scaling,
 *     effectively blending the source pixel with the destination alpha.
 */
static void
_op_copy_rel_p_dp(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d = MUL_SYM(*d >> 24, *s);
                        d++; s++;
                     });
}


#define _op_copy_rel_pas_dp _op_copy_rel_p_dp
#define _op_copy_rel_pan_dp _op_copy_rel_p_dp

#define _op_copy_rel_p_dpan _op_copy_p_dpan
#define _op_copy_rel_pan_dpan _op_copy_pan_dpan
#define _op_copy_rel_pas_dpan _op_copy_pas_dpan

/**
 * @brief Initializes the relative span copy function pointers for C CPU.
 *
 * This function assigns the appropriate relative span copy functions
 * (e.g., _op_copy_rel_p_dp, _op_copy_rel_pan_dp) to the
 * op_copy_rel_span_funcs array for different source, mask,
 * and destination pixel properties.
 */
static void
init_copy_rel_pixel_span_funcs_c(void)
{
   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_p_dp;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pan_dp;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pas_dp;

   op_copy_rel_span_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_p_dpan;
   op_copy_rel_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pan_dpan;
   op_copy_rel_span_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pas_dpan;
}

/**
 * @brief Copies a single pixel from source to destination, modulating by destination alpha.
 * @param s Source pixel value (DATA32).
 * @param m Mask value (DATA8, unused).
 * @param c Temporary color value (DATA32), used to store (1 + destination_alpha).
 * @param d Pointer to the destination pixel (DATA32).
 *
 * This function copies the source pixel 's' to the destination pixel
 * pointed to by 'd'. The operation is "relative", meaning the source
 * pixel's color components are multiplied by (1 + destination_alpha).
 * Example:
 *   c = 1 + (*d >> 24);  // c = 1 + destination_alpha
 *   *d = MUL_256(c, s); // destination = ( (1 + destination_alpha) * source_color ) / 256
 */
static void
_op_copy_rel_pt_p_dp(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   c = 1 + (*d >> 24);
   *d = MUL_256(c, s);
}


#define _op_copy_rel_pt_pan_dp _op_copy_rel_pt_p_dp
#define _op_copy_rel_pt_pas_dp _op_copy_rel_pt_p_dp

#define _op_copy_rel_pt_p_dpan _op_copy_pt_p_dpan
#define _op_copy_rel_pt_pan_dpan _op_copy_pt_pan_dpan
#define _op_copy_rel_pt_pas_dpan _op_copy_pt_pas_dpan

/**
 * @brief Initializes the relative point copy function pointers for C CPU.
 *
 * This function assigns the appropriate relative point copy functions
 * (e.g., _op_copy_rel_pt_p_dp, _op_copy_rel_pt_pan_dp) to the
 * op_copy_rel_pt_funcs array for different source, mask,
 * and destination pixel properties.
 */
static void
init_copy_rel_pixel_pt_funcs_c(void)
{
   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pt_p_dp;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pt_pan_dp;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP][CPU_C] = _op_copy_rel_pt_pas_dp;

   op_copy_rel_pt_funcs[SP][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_p_dpan;
   op_copy_rel_pt_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_pan_dpan;
   op_copy_rel_pt_funcs[SP_AS][SM_N][SC_N][DP_AN][CPU_C] = _op_copy_rel_pt_pas_dpan;
}
