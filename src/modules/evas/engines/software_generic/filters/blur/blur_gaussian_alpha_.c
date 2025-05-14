/* @file blur_gaussian_alpha_.c
 * Should define the functions:
 * - _gaussian_blur_horiz_alpha_step
 * - _gaussian_blur_vert_alpha_step
 */

/* Datatypes and MIN macro */
#include "evas_filter_private.h"

#if !defined (FUNCTION_NAME) || !defined (STEP)
# error Must define FUNCTION_NAME and STEP
#endif

/**
 * @brief Apply one step of Gaussian blur (horizontal or vertical) to alpha channel data.
 *
 * This function applies a weighted average blur using precomputed Gaussian weights.
 * It handles edge cases separately for pixels near the borders. The direction
 * of the blur (horizontal or vertical) is determined by the STEP macro defined
 * during compilation.
 *
 * @param srcdata Pointer to the source alpha data array.
 *                Example: [a1, a2, a3, ...] for horizontal blur (STEP=1)
 *                         [a1, aN+1, a2N+1, ...] for vertical blur (STEP=width)
 * @param dstdata Pointer to the destination alpha data array where results are stored.
 *                Structure mirrors srcdata.
 * @param radius The radius of the Gaussian kernel (kernel size is 2*radius + 1).
 * @param len The length of the line/column being processed (width for horizontal, height for vertical).
 * @param loops The number of lines/columns to process.
 * @param loopstep The offset to move to the next line/column in srcdata/dstdata.
 *                 Example: width * sizeof(DATA8) for horizontal blur,
 *                          sizeof(DATA8) for vertical blur.
 * @param weights Pointer to the precomputed Gaussian weights array. Must have 2*radius + 1 elements.
 *                Example: [w0, w1, ..., w_radius, ..., w_2radius]
 * @param pow2_divider The power-of-2 divider for the weighted sum in the middle section, used for optimization (equivalent to right-shifting).
 */
static inline void
FUNCTION_NAME(const DATA8* restrict srcdata, DATA8* restrict dstdata,
              const int radius, const int len,
              const int loops, const int loopstep,
              const int* restrict weights, const int pow2_divider)
{
   int i, j, k, acc, divider;
   const int diameter = 2 * radius + 1;
   const int left = MIN(radius, len);
   const int right = MIN(radius, (len - radius));
   const DATA8* restrict s;
   const DATA8* restrict src;
   DATA8* restrict dst;

   for (i = loops; i; --i)
     {
        src = srcdata;
        dst = dstdata;

        // Process the left edge pixels where the kernel goes out of bounds.
        // The weights and divider are adjusted dynamically.
        for (k = 0; k < left; k++, dst += STEP)
          {
             acc = 0;
             divider = 0;
             s = src;
             for (j = 0; j <= k + radius; j++, s += STEP)
               {
                  acc += (*s) * weights[j + radius - k];
                  divider += weights[j + radius - k];
               }
             if (!divider) goto error;
             *dst = acc / divider;
          }

        // Process the middle pixels where the full kernel fits within bounds.
        // Uses optimized division via bit shifting (pow2_divider).
        // middle
        for (k = radius; k < (len - radius); k++, src += STEP, dst += STEP)
          {
             acc = 0;
             s = src;
             for (j = 0; j < diameter; j++, s += STEP)
               acc += (*s) * weights[j];
             *dst = acc >> pow2_divider;
          }

        // Process the right edge pixels where the kernel goes out of bounds.
        // Similar to the left edge, weights and divider are adjusted.
        // right
        for (k = 0; k < right; k++, dst += STEP, src += STEP)
          {
             acc = 0;
             divider = 0;
             s = src;
             for (j = 0; j < 2 * radius - k; j++, s += STEP)
               {
                  acc += (*s) * weights[j];
                  divider += weights[j];
               }
             if (!divider) goto error;
             *dst = acc / divider;
          }

        dstdata += loopstep;
        srcdata += loopstep;
     }

   return;

error:
   CRI("Avoided division by 0.");
}

#undef FUNCTION_NAME
#undef STEP
