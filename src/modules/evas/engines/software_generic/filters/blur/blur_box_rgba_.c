/* @file blur_box_rgba_.c
 * Should define the functions:
 * - _box_blur_horiz_rgba_step
 * - _box_blur_vert_rgba_step
 */

#include "evas_filter_private.h"

/**
 * @brief Applies a horizontal box blur to an RGBA image region.
 *
 * This function performs one or more horizontal box blur passes on a specified
 * region of an RGBA image. The blur is applied iteratively if multiple radii
 * are provided.
 *
 * @param srcdata Pointer to the source image data (array of uint32_t RGBA pixels).
 * @param src_stride Stride of the source image data in pixels (width of the source image).
 * @param dstdata Pointer to the destination image data (array of uint32_t RGBA pixels).
 * @param dst_stride Stride of the destination image data in pixels (width of the destination image).
 * @param radii An array of integers representing the radius for each blur pass.
 *              The array is terminated by a 0. Example: `{10, 5, 0}` for two passes.
 * @param region The rectangular region of the image to process.
 */
static inline void
_box_blur_rgba_horiz_step(const uint32_t* restrict srcdata, int src_stride,
                          uint32_t* restrict dstdata, int dst_stride,
                          const int* restrict const radii,
                          Eina_Rectangle region)
{
   const int len = region.w;
   const int loops = region.h;

   const DATA32* restrict src;
   DATA32* restrict dst;
   DATA32* restrict span1;
   DATA32* restrict span2;

#if DIV_USING_BITSHIFT
   int pow2_shifts[6] = {0};
   int numerators[6] = {0};
   for (int run = 0; radii[run]; run++)
     {
        const int div = radii[run] * 2 + 1;
        pow2_shifts[run] = evas_filter_smallest_pow2_larger_than(div << 10);
        numerators[run] = (1 << pow2_shifts[run]) / (div);
     }
#endif

   srcdata += region.x + src_stride * region.y;
   dstdata += region.x + dst_stride * region.y;

   span1 = alloca(len * sizeof(DATA32));
   span2 = alloca(len * sizeof(DATA32));
   memset(span1, 0, len * sizeof(DATA32));
   memset(span2, 0, len * sizeof(DATA32));

   // For each line, apply as many blurs as requested
   for (int l = 0; l < loops; l++) // Iterate over each row in the region
     {
        int run;

        // New line: reset source & destination pointers
        src = srcdata + src_stride * l;
        if (!radii[1]) // Only one blur run requested
          dst = dstdata + dst_stride * l; // Write directly to destination
        else
          dst = span1; // Use temporary span for intermediate results

        // Apply blur with current radius
        for (run = 0; radii[run]; run++) // Iterate over each requested blur radius
          {
             const int radius = radii[run];
             const int left = MIN(radius, len);

#if DIV_USING_BITSHIFT
             const int pow2 = pow2_shifts[run];
             const int numerator = numerators[run];
#else
             const int divider = 2 * radius + 1;
#endif

             const DATA8* restrict sl = (DATA8 *) src;
             const DATA8* restrict sr = (DATA8 *) src;
             const DATA8* restrict sre = (DATA8 *) (src + len);
             const DATA8* restrict sle = (DATA8 *) (src + len - radius);
             DATA8* restrict d = (DATA8 *) dst;
             int acc[4] = {0};
             int count = 0;

             // Read-ahead
             for (int x = left; x > 0; x--)
               {
                  for (int k = 0; k < 4; k++)
                    acc[k] += sr[k];
                  sr += sizeof(DATA32);
                  count++;
               }

             // Left
             for (int x = left; x > 0; x--)
               {
                  if (sr < sre)
                    {
                       for (int k = 0; k < 4; k++)
                         acc[k] += sr[k];
                       sr += sizeof(DATA32);
                       count++;
                    }

                  d[ALPHA] = acc[ALPHA] / count;
                  d[RED]   = acc[RED]   / count;
                  d[GREEN] = acc[GREEN] / count;
                  d[BLUE]  = acc[BLUE]  / count;
                  d += sizeof(DATA32);
               }

             // Main part
             for (; sr < sre; sr += sizeof(DATA32), sl += sizeof(DATA32))
               {
                  for (int k = 0; k < 4; k++)
                    acc[k] += sr[k];

                  d[ALPHA] = DIVIDE(acc[ALPHA]);
                  d[RED]   = DIVIDE(acc[RED]);
                  d[GREEN] = DIVIDE(acc[GREEN]);
                  d[BLUE]  = DIVIDE(acc[BLUE]);
                  d += sizeof(DATA32);

                  for (int k = 0; k < 4; k++)
                    acc[k] -= sl[k];
               }

             // Right part
             count = 2 * radius + 1;
             for (; sl < sle; sl += sizeof(DATA32))
               {
                  const int divider = --count;
                  d[ALPHA] = acc[ALPHA] / divider;
                  d[RED]   = acc[RED]   / divider;
                  d[GREEN] = acc[GREEN] / divider;
                  d[BLUE]  = acc[BLUE]  / divider;
                  d += sizeof(DATA32);

                  for (int k = 0; k < 4; k++)
                    acc[k] -= sl[k];
               }

             // More runs to go: swap spans and prepare for next pass
             if (radii[run + 1])
               {
                  src = dst; // Current destination becomes source for next pass
                  if (radii[run + 2]) // If there are at least two more runs
                    {
                       // Swap span1 and span2 to reuse them
                       DATA32* swap = span1;
                       span1 = span2;
                       span2 = swap;
                       dst = span1; // Next destination is the (now new) span1
                    }
                  else // This was the second to last run
                    {
                       // Next run is the last one: write directly to final destination
                       dst = dstdata + dst_stride * l;
                    }
               }
          }
     }
}

/**
 * @brief Applies a vertical box blur to an RGBA image region.
 *
 * This function performs one or more vertical box blur passes on a specified
 * region of an RGBA image. The blur is applied iteratively if multiple radii
 * are provided.
 * It optimizes for cache hits by processing data in rotated horizontal spans.
 *
 * @param srcdata Pointer to the source image data (array of uint32_t RGBA pixels).
 * @param src_stride Stride of the source image data in pixels (width of the source image).
 * @param dstdata Pointer to the destination image data (array of uint32_t RGBA pixels).
 * @param dst_stride Stride of the destination image data in pixels (width of the destination image).
 * @param radii An array of integers representing the radius for each blur pass.
 *              The array is terminated by a 0. Example: `{10, 5, 0}` for two passes.
 * @param region The rectangular region of the image to process.
 */
static inline void
_box_blur_rgba_vert_step(const uint32_t* restrict srcdata, int src_stride,
                         uint32_t* restrict dstdata, int dst_stride,
                         const int* restrict const radii,
                         Eina_Rectangle region)
{
   /* Note: This function tries to optimize cache hits by working on
    * contiguous horizontal spans.
    */

   const int len = region.h;
   const int loops = region.w;

   DATA32* restrict src;
   DATA32* restrict dst;
   DATA32* restrict span1;
   DATA32* restrict span2;

#if DIV_USING_BITSHIFT
   int pow2_shifts[6] = {0};
   int numerators[6] = {0};
   for (int run = 0; radii[run]; run++)
     {
        const int div = radii[run] * 2 + 1;
        pow2_shifts[run] = evas_filter_smallest_pow2_larger_than(div << 10);
        numerators[run] = (1 << pow2_shifts[run]) / (div);
     }
#endif

   srcdata += region.x + src_stride * region.y;
   dstdata += region.x + dst_stride * region.y;

   span1 = alloca(len * sizeof(DATA32));
   span2 = alloca(len * sizeof(DATA32));
   memset(span1, 0, len * sizeof(DATA32));
   memset(span2, 0, len * sizeof(DATA32));

   // For each column (processed as a horizontal span after rotation), apply blurs
   for (int l = 0; l < loops; l++) // Iterate over each column in the region
     {
        int run;

        // Rotate input column into a horizontal work span (span1) for cache efficiency
        const DATA32* srcptr = srcdata + l; // Start of current column in source
        DATA32* s = span1;
        for (int k = len; k; --k) // 'len' is region.h (height of the column)
          {
             *s++ = *srcptr;
             srcptr += src_stride; // Move to next row in the same column
          }

        src = span1; // Source for the first blur pass is the rotated column
        dst = span2; // Destination for the first blur pass (or intermediate)

        // Apply blur with current radius
        for (run = 0; radii[run]; run++) // Iterate over each requested blur radius
          {
             const int radius = radii[run];
             const int left = MIN(radius, len);

#if DIV_USING_BITSHIFT
             const int pow2 = pow2_shifts[run];
             const int numerator = numerators[run];
#else
             const int divider = 2 * radius + 1;
#endif

             const DATA8* restrict sl = (DATA8 *) src;
             const DATA8* restrict sr = (DATA8 *) src;
             const DATA8* restrict sre = (DATA8 *) (src + len);
             const DATA8* restrict sle = (DATA8 *) (src + len - radius);
             DATA8* restrict d = (DATA8 *) dst;
             int acc[4] = {0};
             int count = 0;

             // Read-ahead
             for (int x = left; x > 0; x--)
               {
                  for (int k = 0; k < 4; k++)
                    acc[k] += sr[k];
                  sr += sizeof(DATA32);
                  count++;
               }

             // Left
             for (int x = left; x > 0; x--)
               {
                  if (sr < sre)
                    {
                       for (int k = 0; k < 4; k++)
                         acc[k] += sr[k];
                       sr += sizeof(DATA32);
                       count++;
                    }

                  d[ALPHA] = acc[ALPHA] / count;
                  d[RED]   = acc[RED]   / count;
                  d[GREEN] = acc[GREEN] / count;
                  d[BLUE]  = acc[BLUE]  / count;
                  d += sizeof(DATA32);
               }

             // Main part
             for (; sr < sre; sr += sizeof(DATA32), sl += sizeof(DATA32))
               {
                  for (int k = 0; k < 4; k++)
                    acc[k] += sr[k];

                  d[ALPHA] = DIVIDE(acc[ALPHA]);
                  d[RED]   = DIVIDE(acc[RED]);
                  d[GREEN] = DIVIDE(acc[GREEN]);
                  d[BLUE]  = DIVIDE(acc[BLUE]);
                  d += sizeof(DATA32);

                  for (int k = 0; k < 4; k++)
                    acc[k] -= sl[k];
               }

             // Right part
             count = 2 * radius + 1;
             for (; sl < sle; sl += sizeof(DATA32))
               {
                  const int divider = --count;
                  d[ALPHA] = acc[ALPHA] / divider;
                  d[RED]   = acc[RED]   / divider;
                  d[GREEN] = acc[GREEN] / divider;
                  d[BLUE]  = acc[BLUE]  / divider;
                  d += sizeof(DATA32);

                  for (int k = 0; k < 4; k++)
                    acc[k] -= sl[k];
               }

             // More runs to go: swap source and destination spans for the next pass
             if (radii[run + 1])
               {
                  DATA32* swap = src;
                  src = dst; // Current destination becomes source for the next pass
                  dst = swap; // Reuse the old source span as the new destination
               }
          }

        // After all blur passes for this column are done,
        // rotate the final blurred span (dst) back into the destination image column.
        DATA32* restrict dstptr = dstdata + l; // Start of current column in destination
        for (int k = len; k; --k) // 'len' is region.h (height of the column)
          {
             *dstptr = *dst++; // Copy pixel from processed horizontal span
             dstptr += dst_stride; // Move to next row in the same column
          }
     }
}
