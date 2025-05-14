/* @file blur_gaussian_rgba_.c
 * Should define the functions:
 * - _gaussian_blur_horiz_rgba_step
 * - _gaussian_blur_vert_rgba_step
 */

#include "evas_filter_private.h"

#if !defined (FUNCTION_NAME) || !defined (STEP)
# error Must define FUNCTION_NAME and STEP
#endif

/**
 * @brief Applies a 1D Gaussian blur step (horizontal or vertical) to RGBA data.
 *
 * This function implements one pass of a separable Gaussian blur. It can operate
 * either horizontally or vertically based on the compile-time `STEP` macro.
 * It handles boundary conditions carefully by adjusting the kernel weights
 * and normalization factor near the edges.
 *
 * @param srcdata Pointer to the source image data (array of DATA32 RGBA pixels).
 *                Example: [R1G1B1A1, R2G2B2A2, ...]
 * @param dstdata Pointer to the destination buffer for the blurred data.
 *                Must be large enough to hold the results for `loops` lines/columns.
 * @param radius The radius of the Gaussian kernel (kernel size = 2 * radius + 1).
 * @param len The length of the line/column being processed (e.g., width for horizontal, height for vertical).
 * @param loops The number of lines/columns to process.
 * @param loopstep The offset (in pixels) between the start of consecutive lines/columns
 *                 in both `srcdata` and `dstdata`. (e.g., image width for vertical blur).
 * @param weights Pointer to the precomputed Gaussian kernel weights (an array of `2 * radius + 1` integers).
 *                Example (radius=1): [weight_neg1, weight_0, weight_pos1]
 * @param pow2_divider The normalization factor for the central part of the blur,
 *                     expressed as a power of 2 for fast division via bit shifting.
 *                     Should be equal to the sum of all weights in the `weights` array.
 */
static inline void
FUNCTION_NAME(const DATA32* restrict srcdata, DATA32* restrict dstdata,
              const int radius, const int len,
              const int loops, const int loopstep,
              const int* restrict weights, const int pow2_divider)
{
   const int diameter = 2 * radius + 1;
   const int left = MIN(radius, len);
   const int right = MIN(radius, (len - radius));
   const DATA32* restrict src;
   DATA32* restrict dst;
   int i, j, k;

   for (i = loops; i; --i)
     {
        src = srcdata;
        dst = dstdata;

        // --- Left Edge Handling ---
        // Process pixels where the kernel window extends *before* the start (index 0).
        // The number of contributing source pixels is less than the full kernel diameter.
        // We adjust the weights and the normalization divider accordingly.
        for (k = 0; k < left; k++, dst += STEP)
          {
             int acc[4] = {0}; // Accumulators for Alpha, Red, Green, Blue channels
             int divider = 0; // Sum of weights used for this pixel (normalization factor)
             const DATA32* restrict s = src;
             // Iterate over the source pixels covered by the kernel for this destination pixel.
             for (j = 0; j <= k + radius; j++, s += STEP)
               {
                  // Calculate the correct index into the weights array based on the
                  // current source pixel's position (j) relative to the destination pixel (k).
                  // For the leftmost destination pixel (k=0), the first source pixel (j=0)
                  // corresponds to the center weight (weights[radius]).
                  // The next source pixel (j=1) corresponds to weights[radius+1], etc.
                  const int weightidx = j + radius - k;
                  acc[ALPHA] += A_VAL(s) * weights[weightidx];
                  acc[RED]   += R_VAL(s) * weights[weightidx];
                  acc[GREEN] += G_VAL(s) * weights[weightidx];
                  acc[BLUE]  += B_VAL(s) * weights[weightidx];
                  divider += weights[weightidx]; // Accumulate the weights for normalization.
               }
            // Normalize using the sum of the weights actually used for this edge pixel.
            // Avoid division by zero, although this should not happen with valid weights.
            if (!divider) goto error;
            A_VAL(dst) = acc[ALPHA] / divider;
            R_VAL(dst) = acc[RED]   / divider;
             G_VAL(dst) = acc[GREEN] / divider;
             B_VAL(dst) = acc[BLUE]  / divider;
          }

        // middle
        for (k = len - (2 * radius); k > 0; k--, src += STEP, dst += STEP)
          {
             int acc[4] = {0};
             const DATA32* restrict s = src;
             for (j = 0; j < diameter; j++, s += STEP)
               {
                  acc[ALPHA] += A_VAL(s) * weights[j];
                  acc[RED]   += R_VAL(s) * weights[j];
                  acc[GREEN] += G_VAL(s) * weights[j];
                  acc[BLUE]  += B_VAL(s) * weights[j];
               }
            // Normalize using the precalculated power-of-2 divider for speed (using bit shift).
            // This assumes pow2_divider is the sum of all weights in the kernel.
            A_VAL(dst) = acc[ALPHA] >> pow2_divider;
            R_VAL(dst) = acc[RED]   >> pow2_divider;
            G_VAL(dst) = acc[GREEN] >> pow2_divider;
             B_VAL(dst) = acc[BLUE]  >> pow2_divider;
          }

        // right
        for (k = 0; k < right; k++, dst += STEP, src += STEP)
          {
             int acc[4] = {0};
             int divider = 0; // Sum of weights used for this pixel (normalization factor)
             const DATA32* restrict s = src;
             // Iterate over the source pixels covered by the kernel for this destination pixel.
             // The number of iterations decreases as k increases (approaching the right edge).
             for (j = 0; j < 2 * radius - k; j++, s += STEP)
               {
                  // Weights are used starting from index 0 up to the available source pixels.
                  acc[ALPHA] += A_VAL(s) * weights[j];
                  acc[RED]   += R_VAL(s) * weights[j];
                  acc[GREEN] += G_VAL(s) * weights[j];
                  acc[BLUE]  += B_VAL(s) * weights[j];
                  divider += weights[j]; // Accumulate the weights for normalization.
               }
            // Normalize using the sum of the weights actually used for this edge pixel.
            if (!divider) goto error;
            A_VAL(dst) = acc[ALPHA] / divider;
            R_VAL(dst) = acc[RED]   / divider;
             G_VAL(dst) = acc[GREEN] / divider;
             B_VAL(dst) = acc[BLUE]  / divider;
          }

        // Advance to the next line/column in source and destination buffers.
        dstdata += loopstep;
        srcdata += loopstep;
     }

   return;

error:
   CRI("Avoided division by 0.");
}

#undef FUNCTION_NAME
#undef STEP
