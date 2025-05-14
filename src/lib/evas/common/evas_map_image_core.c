//#undef SCALE_USING_MMX
{
   /**
    * @brief Renders textured spans for a mapped image, with or without smoothing.
    *
    * This code processes a list of horizontal spans (`spans`) for each scanline
    * (`y` from `ystart` to `yend`). It supports smooth (perspective-correct)
    * and non-smooth (affine) texture mapping, color interpolation, and
    * optional MMX optimizations.
    *
    * The `spans` array is expected to contain `Line` structures, where each
    * `Line` can have up to two `Span` segments. A `Span` defines the
    * x-range, texture coordinates (u,v), perspective correction values (o1,o2),
    * and color values for a segment of a scanline.
    *
    * Example `spans` structure (conceptual):
    * spans[scanline_idx] = {
    *   .span[0] = { .x = {x0_start, x0_end}, .u = {u0_s, u0_e}, .v = {v0_s, v0_e}, .o1=o0_s, .o2=o0_e, .col={c0_s,c0_e} },
    *   .span[1] = { .x = {x1_start, x1_end}, .u = {u1_s, u1_e}, .v = {v1_s, v1_e}, .o1=o1_s, .o2=o1_e, .col={c1_s,c1_e} }
    *   // span[1] might be unused if x1_start < 0
    * }
    */
   if (smooth)
     {
        /**
         * Smooth rendering path:
         * Applies perspective-correct texture mapping and color interpolation.
         */
        for (y = ystart; y <= yend; y++)
          {
             /**
              * Process each scanline from ystart to yend.
              */
             int x, w, ww;
             FPc u, v, u2, v2, ud, vd, dv;
             DATA32 *d, *s;
#ifdef COLMUL
             FPc cv, cd; // col
# ifdef SCALE_USING_MMX
             FPc cc;
#endif //SCALE_USING_MMX
             DATA32 c1, c2; // col
#endif //COLMUL
             Line *line;
#ifdef SCALE_USING_MMX
             /**
              * MMX initialization (if SCALE_USING_MMX is defined):
              * - pxor_r2r(mm0, mm0): Clears mm0 register (sets to zero).
              * - MOV_A2R(ALPHA_255, mm5): Loads 0xff000000 (ALPHA_255) into mm5.
              *   mm0 is often used for unpacking operations.
              *   mm5 could be used as an alpha mask or for opaque alpha values.
              */
             pxor_r2r(mm0, mm0);
             MOV_A2R(ALPHA_255, mm5)
#endif //SCALE_USING_MMX
             line = &(spans[y - ystart]);
             /**
              * A scanline can be composed of up to two horizontal spans
              * (e.g., for a non-rectangular quadrilateral or a clipped polygon).
              * Loop through each potential span segment for the current scanline.
              */
             for (i = 0; i < 2; i++)
               {
                  Span *span;
                  span = &(line->span[i]);

                  //The polygon shape won't be completed type
                  /**
                   * If span->x[0] is negative, it indicates that this span
                   * (and any subsequent span for this scanline, due to 'break')
                   * is not part of the rendered polygon. This is a common way
                   * to mark unused or clipped-away span slots.
                   */
                  if (span->x[0] < 0) break;

                  x = span->x[0];

                  w = (span->x[1] - x);
                  if (w <= 0) continue;

                  /**
                   * dv represents the difference in the perspective correction factor
                   * (likely 1/z) between the end and start of the span.
                   * This value is crucial for perspective-correct interpolation of
                   * texture coordinates. If dv is zero or negative, perspective
                   * interpolation might be ill-defined or lead to division by zero,
                   * so the span is skipped.
                   */
                  dv = (span->o2 - span->o1);
                  if (dv <= 0) continue;

                  ww = w;

                  //correct elaborate u point
                  /**
                   * Perspective-correct calculation of the initial texture coordinate 'u'
                   * and its per-pixel increment 'ud'.
                   * 'FPI' is likely the number of fractional bits for fixed-point coordinates.
                   * 'swp' is source width in pixels, possibly scaled for fixed-point.
                   * 1. Initial u/u2 are span endpoint texture coords, clamped to source bounds.
                   * 2. Basic ud = (u2 - u) / w.
                   * 3. Perspective correction: ud_corrected = (ud * w_fixedpoint) / dv.
                   *    This scales ud by the ratio of screen space width to perspective depth change.
                   * 4. Adjust starting u: u_start -= (ud_corrected * (o1 - x0_fixedpoint_perspective_offset)).
                   *    This refines the starting 'u' based on the exact sub-pixel starting
                   *    point and its perspective value 'o1'.
                   * 5. Final clamping of 'u'.
                   */
                  u = span->u[0] << FPI;
                  if (u < 0) u = 0;
                  else if (u > swp) u = swp;
                  u2 = span->u[1] << FPI;
                  if (u2 < 0) u2 = 0;
                  else if (u2 > swp) u2 = swp;
                  ud = (u2 - u) / w;
                  ud = ((long long)ud * (w << FP)) / dv;
                  u -= (ud * (span->o1 - (span->x[0] << FP))) / FP1;
                  if (ud < 0) u += ud; // Ensure u is at the start of the pixel center
                  if (u < 0) u = 0;
                  else if (u >= swp) u = swp - 1;

                  //correct elaborate v point
                  /**
                   * Perspective-correct calculation for texture coordinate 'v' and its
                   * increment 'vd'. The logic mirrors the calculation for 'u'.
                   * 'shp' is source height in pixels, possibly scaled for fixed-point.
                   */
                  v = span->v[0] << FPI;
                  if (v < 0) v = 0;
                  else if (v > shp) v = shp;
                  v2 = span->v[1] << FPI;
                  if (v2 < 0) v2 = 0;
                  else if (v2 > shp) v2 = shp;
                  vd = (v2 - v) / w;
                  vd = ((long long)vd * (w << FP)) / dv;
                  v -= (vd * (span->o1 - (span->x[0] << FP))) / FP1;
                  if (vd < 0) v += vd; // Ensure v is at the start of the pixel center
                  if (v < 0) v = 0;
                  else if (v >= shp) v = shp - 1;

                  /**
                   * Select the destination buffer pointer 'd'.
                   * If 'direct' is true, pixels are written directly to the final
                   * destination image ('dst->image.data').
                   * Otherwise, pixels are written to an intermediate buffer ('buf'),
                   * which is later blended or copied to the destination. This is
                   * often used for effects like masking.
                   */
                  if (direct)
                    d = dst->image.data + (y * dst->cache_entry.w) + x;
                  else
                    d = buf;
#define SMOOTH 1
#ifdef COLMUL
                  /**
                   * If COLMUL (color multiplication/interpolation) is enabled:
                   * - c1, c2: Start and end colors of the span.
                   * - cv: Current interpolated color value (fixed-point, likely alpha and/or components).
                   * - cd: Per-pixel increment for the color interpolation.
                   */
                  c1 = span->col[0]; // col
                  c2 = span->col[1]; // col
                  cv = 0; // col
                  cd = (255 << 16) / w; // col

                  if (c1 == c2)
                    {
                       if (c1 == 0xffffffff)
                         {
#endif //COLMUL
#define COLSAME 1
                          /**
                           * Include the core pixel processing loop from "evas_map_image_loop.c".
                           * This external file likely contains optimized routines for rendering
                           * pixels, parameterized by preprocessor defines like:
                           * - SMOOTH: Indicates if smooth (bilinear/perspective) filtering is active.
                           * - COLSAME: Indicates if start and end colors of the span are identical (c1 == c2).
                           * - COLBLACK: Potentially a special case for black spans.
                           * This pattern allows reusing the inner loop logic for different configurations.
                           */
#include "evas_map_image_loop.c"
#undef COLSAME
#ifdef COLMUL
                         }
                       else if ((c1 == 0x0000ff) && (!src->cache_entry.flags.alpha))
                         {
                            // all black line
# define COLBLACK 1
# define COLSAME 1
# include "evas_map_image_loop.c"
# undef COLSAME
# undef COLBLACK
                         }
                       else if (c1 == 0x000000)
                         {
                            // skip span
                         }
                       else
                         {
                            // generic loop
# define COLSAME 1
# include "evas_map_image_loop.c"
# undef COLSAME
                         }
                    }
                  else
                    {
# include "evas_map_image_loop.c"
                    }
#endif //COLMUL
                  /**
                   * If rendering was not direct (i.e., pixels were written to 'buf'):
                   * This block handles transferring the pixels from 'buf' to the
                   * final destination 'dst->image.data'.
                   * - 'func' is a generic blitting/blending function.
                   * - If 'mask_ie' (mask image entry) is present, it's used as an
                   *   alpha mask for the blit.
                   * - 'func2' might be a pre-multiplication step if 'mul_col' is used.
                   */
                  if (!direct)
                    {
                       d = dst->image.data;
                       d += (y * dst->cache_entry.w) + x;
                       if (!mask_ie)
                         func(buf, NULL, mul_col, d, w);
                       else
                         {
                            DATA8 *mask = mask_ie->image.data8
                               + (y - mask_y) * mask_ie->cache_entry.w
                               + (x - mask_x);
                            if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                            func(buf, mask, 0, d, w);
                         }
                    }
               }
          }
     }
   else
     {
        /**
         * Non-smooth (affine) rendering path:
         * Uses simpler affine texture mapping where texture coordinate increments
         * (ud, vd) are constant across the span (no perspective correction for increments).
         * Color interpolation is still performed if COLMUL is set.
         */
        for (y = ystart; y <= yend; y++)
          {
             /**
              * Process each scanline from ystart to yend for non-smooth rendering.
              */
             int x, w, ww;
             FPc u, v, u2, v2, ud, vd;
             DATA32 *d, *s;
#ifdef COLMUL
             FPc cv, cd; // col
             DATA32 c1, c2; // col
#endif //COLMUL
             Line *line;
             line = &(spans[y - ystart]);
             /**
              * Process up to two horizontal spans per scanline.
              */
             for (i = 0; i < 2; i++)
               {
                  Span *span;
                  span = &(line->span[i]);

                  //The polygon shape won't be completed type
                  /**
                   * Skip invalid or unused spans.
                   */
                  if (span->x[0] < 0) break;

                  x = span->x[0];

                  w = (span->x[1] - x);
                  if (w <= 0) continue;

                  ww = w;

                  //correct elaborate u point
                  /**
                   * Affine calculation of initial texture coordinate 'u' and its
                   * per-pixel increment 'ud'.
                   * 'ud' is constant across the span (no perspective correction).
                   * The starting 'u' is adjusted if 'ud' is negative to ensure it aligns
                   * correctly with the first pixel of the span.
                   */
                  u = span->u[0] << FPI;
                  if (u < 0) u = 0;
                  else if (u > swp) u = swp;
                  u2 = span->u[1] << FPI;
                  if (u2 < 0) u2 = 0;
                  else if (u2 > swp) u2 = swp;
                  ud = (u2 - u) / w;
                  if (ud < 0) u += ud; // Ensure u is at the start of the pixel center
                  if (u < 0) u = 0;
                  else if (u >= swp) u = swp - 1;

                  //correct elaborate v point
                  /**
                   * Affine calculation for texture coordinate 'v' and its increment 'vd'.
                   * Similar to 'u', 'vd' is constant.
                   */
                  v = span->v[0] << FPI;
                  if (v < 0) v = 0;
                  else if (v > shp) v = shp;
                  v2 = span->v[1] << FPI;
                  if (v2 < 0) v2 = 0;
                  else if (v2 > shp) v2 = shp;
                  vd = (v2 - v) / w;
                  if (vd < 0) v += vd; // Ensure v is at the start of the pixel center
                  if (v < 0) v = 0;
                  else if (v >= shp) v = shp - 1;

                  /**
                   * Select destination buffer pointer 'd' (direct or intermediate 'buf').
                   * Same logic as in the smooth path.
                   */
                  if (direct)
                    d = dst->image.data + (y * dst->cache_entry.w) + x;
                  else
                    d = buf;
#undef SMOOTH
#ifdef COLMUL
                  /**
                   * Color interpolation setup if COLMUL is defined.
                   * Same logic as in the smooth path.
                   */
                  c1 = span->col[0]; // col
                  c2 = span->col[1]; // col
                  cv = 0; // col
                  cd = (255 << 16) / w; // col

                  if (c1 == c2)
                    {
                       if (c1 == 0xffffffff)
                         {
#endif //COLMUL
#define COLSAME 1
                          /**
                           * Include the core pixel processing loop ("evas_map_image_loop.c").
                           * SMOOTH is now undefined for this non-smooth path.
                           * COLSAME and other potential macros still apply.
                           */
#include "evas_map_image_loop.c"
#undef COLSAME
#ifdef COLMUL
                         }
                       else if ((c1 == 0x0000ff) && (!src->cache_entry.flags.alpha))
                         {
                            // all black line
# define COLBLACK 1
# define COLSAME 1
# include "evas_map_image_loop.c"
# undef COLSAME
# undef COLBLACK
                         }
                       else if (c1 == 0x000000)
                         {
                            // skip span
                         }
                       else
                         {
                            // generic loop
# define COLSAME 1
# include "evas_map_image_loop.c"
# undef COLSAME
                         }
                    }
                  else
                    {
                       // generic loop
# include "evas_map_image_loop.c"
                    }
#endif //COLMUL
                  /**
                   * If rendering was not direct (i.e., used 'buf'), copy/blend
                   * from 'buf' to the final destination, potentially with masking.
                   * Same logic as in the smooth path.
                   */
                  if (!direct)
                    {
                       d = dst->image.data;
                       d += (y * dst->cache_entry.w) + x;
                       if (!mask_ie)
                         func(buf, NULL, mul_col, d, w);
                       else
                         {
                            DATA8 *mask = mask_ie->image.data8
                               + (y - mask_y) * mask_ie->cache_entry.w
                               + (x - mask_x);
                            if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                            func(buf, mask, 0, d, w);
                         }
                    }
               }
          }
     }
}
