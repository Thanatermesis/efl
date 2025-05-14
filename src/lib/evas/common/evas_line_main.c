#include "evas_common_private.h"
#include "evas_blend_private.h"

/**
 * @file evas_line_main.c
 * @brief This file contains the main line drawing functions for Evas.
 *
 * It includes functions for drawing points, simple lines, anti-aliased lines,
 * and handling clipping and masking.
 */

/**
 * @def IN_RANGE(x, y, w, h)
 * @brief Checks if a point (x, y) is within the bounds (0, 0) to (w-1, h-1).
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param w The width of the bounding area.
 * @param h The height of the bounding area.
 * @return True if the point is within the range, false otherwise.
 */
#define IN_RANGE(x, y, w, h) \
  ( x > 0 && y > 0 &&((unsigned)(x) < (unsigned)(w)) && ((unsigned)(y) < (unsigned)(h)) )

/**
 * @def IN_RECT(x, y, rx, ry, rw, rh)
 * @brief Checks if a point (x, y) is within a rectangle defined by (rx, ry, rw, rh).
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param rx The x-coordinate of the rectangle's top-left corner.
 * @param ry The y-coordinate of the rectangle's top-left corner.
 * @param rw The width of the rectangle.
 * @param rh The height of the rectangle.
 * @return True if the point is within the rectangle, false otherwise.
 */
#define IN_RECT(x, y, rx, ry, rw, rh)                   \
  ( ((unsigned)((x) - (rx)) < (unsigned)(rw)) &&        \
    ((unsigned)((y) - (ry)) < (unsigned)(rh)) )

/**
 * @def EXCHANGE_POINTS(x0, y0, x1, y1)
 * @brief Swaps the coordinates of two points (x0, y0) and (x1, y1).
 * @param x0 The x-coordinate of the first point.
 * @param y0 The y-coordinate of the first point.
 * @param x1 The x-coordinate of the second point.
 * @param y1 The y-coordinate of the second point.
 */
#define EXCHANGE_POINTS(x0, y0, x1, y1)         \
  {                                             \
     int _tmp = y0;                             \
                                                \
     y0 = y1;                                   \
     y1 = _tmp;                                 \
                                                \
     _tmp = x0;                                 \
     x0 = x1;                                   \
     x1 = _tmp;                                 \
  }

/**
 * @brief Initializes common line drawing functionalities.
 * @ingroup Evas_Common_Line
 *
 * This function is currently a placeholder and does not perform any operations.
 */
EVAS_API void
evas_common_line_init(void)
{
}

/**
 * @internal
 * @brief Draws a single point on the destination image.
 *
 * This function handles clipping and masking for the point.
 * It uses the drawing context for color, render operation, and clipping information.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context containing color, render operation, and clip settings.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 */
static void
_evas_draw_point(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y)
{
   RGBA_Gfx_Pt_Func pfunc;
   DATA8 *mask = NULL;

   if (!dst->image.data) return;
   if (!IN_RANGE(x, y, dst->cache_entry.w, dst->cache_entry.h))
	return;
   if ((dc->clip.use) && (!IN_RECT(x, y, dc->clip.x, dc->clip.y, dc->clip.w, dc->clip.h)))
	return;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
   pixman_op_t op = PIXMAN_OP_SRC;

   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;

   if ((dst->pixman.im) && (dc->col.pixman_color_image))
     pixman_image_composite(op, dc->col.pixman_color_image, NULL,
                            dst->pixman.im, x, y, 0, 0, x, y, 1, 1);
   else
# endif
#endif
     {
        if (dc->clip.mask)
          {
             RGBA_Image *im = dc->clip.mask;
             mask = im->image.data8
                + (y - dc->clip.mask_y) * im->cache_entry.w
                + (x - dc->clip.mask_x);
             pfunc = evas_common_gfx_func_composite_mask_color_pt_get(dc->col.col, dst->cache_entry.flags.alpha, dc->render_op);
             if (pfunc)
               pfunc(0, *mask, dc->col.col, dst->image.data + (dst->cache_entry.w * y) + x);
          }
        else
          {
             pfunc = evas_common_gfx_func_composite_color_pt_get(dc->col.col, dst->cache_entry.flags.alpha, dc->render_op);
             if (pfunc)
               pfunc(0, 255, dc->col.col, dst->image.data + (dst->cache_entry.w * y) + x);
          }
     }
}

/**
 * @brief Draws a single point with explicit clipping and masking.
 * @ingroup Evas_Common_Line
 *
 * This function provides a more direct way to draw a point, specifying
 * all parameters explicitly rather than using a draw context.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param clip_x The x-coordinate of the clip rectangle.
 * @param clip_y The y-coordinate of the clip rectangle.
 * @param clip_w The width of the clip rectangle.
 * @param clip_h The height of the clip rectangle.
 * @param color The color of the point (in DATA32 format, e.g., 0xAARRGGBB).
 * @param render_op The rendering operation (e.g., _EVAS_RENDER_COPY, _EVAS_RENDER_BLEND).
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param mask_ie Optional mask image. If NULL, no mask is applied.
 * @param mask_x The x-offset for the mask image.
 * @param mask_y The y-offset for the mask image.
 */
EVAS_API void
evas_common_line_point_draw(RGBA_Image *dst, int clip_x, int clip_y, int clip_w, int clip_h, DATA32 color, int render_op, int x, int y, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   Eina_Bool no_cuse;
   RGBA_Gfx_Pt_Func pfunc;
   DATA8 *mask = NULL;

   if (!dst->image.data) return;
   no_cuse = ((clip_x == 0) && (clip_y == 0) &&
              ((clip_w == (int)dst->cache_entry.w) &&
               (clip_h == (int)dst->cache_entry.h)));

   if (!IN_RANGE(x, y, dst->cache_entry.w, dst->cache_entry.h)) return;
   if ((!no_cuse) && (!IN_RECT(x, y, clip_x, clip_y, clip_w, clip_h)))
     return;

   if (mask_ie)
     {
        mask = mask_ie->image.data8
           + (y - mask_y) * mask_ie->cache_entry.w
           + (x - mask_x);
        pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
        if (pfunc)
          pfunc(0, *mask, color, dst->image.data + (dst->cache_entry.w * y) + x);
     }
   else
     {
        pfunc = evas_common_gfx_func_composite_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
        if (pfunc)
          pfunc(0, 255, color, dst->image.data + (dst->cache_entry.w * y) + x);
     }
}

/*
   these functions use the dc->clip data as bounding
   data. they assume that such data has already been cut
   back to lie in the dst image rect and the object's
   (line) bounding rect.
*/
/**
 * @internal
 * @brief Draws a simple horizontal, vertical, or 45-degree diagonal line.
 *
 * This function is optimized for these specific line types. It uses the
 * drawing context for color, clipping, and render operation.
 * It assumes clipping has been pre-applied to the dc->clip structure.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 */
static void
_evas_draw_simple_line(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1)
{
   int     dx, dy, len, lx, ty, rx, by;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p, color;
   DATA8   *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;
   RGBA_Gfx_Func    sfunc;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
   pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY
   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;
# endif
#endif

   dstw = dst->cache_entry.w;
   color = dc->col.col;

   if (y0 > y1)
      EXCHANGE_POINTS(x0, y0, x1, y1)
   if (x0 > x1)
      EXCHANGE_POINTS(x0, y0, x1, y1)

   dx = x1 - x0;
   dy = y1 - y0;

   clx = dc->clip.x;
   cly = dc->clip.y;
   clw = dc->clip.w;
   clh = dc->clip.h;

   lx = clx;
   rx = clx + clw - 1;
   ty = cly;
   by = cly + clh - 1;

   if (dy == 0)
     {
        if ((y0 >= ty) && (y0 <= by))
          {
             if (dx < 0)
               {
                  int  tmp = x1;

                  x1 = x0;
                  x0 = tmp;
               }

             if (x0 < lx) x0 = lx;
             if (x1 > rx) x1 = rx;

             len = x1 - x0 + 1;
             p = dst->image.data + (dstw * y0) + x0;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             if ((dst->pixman.im) && (dc->col.pixman_color_image))
               pixman_image_composite(op, dc->col.pixman_color_image,
                                      NULL, dst->pixman.im,
                                      x0, y0, 0, 0, x0, y0, len, 1);
             else
# endif
#endif
               {
                  if (dc->clip.mask)
                    {
                       RGBA_Image *im = dc->clip.mask;
                       mask = im->image.data8
                          + ((y0 - dc->clip.mask_y) * im->cache_entry.w)
                          + (x0 - dc->clip.mask_x);
                       sfunc = evas_common_gfx_func_composite_mask_color_span_get(color, dst->cache_entry.flags.alpha, len, dc->render_op);
                       if (sfunc) sfunc(NULL, mask, color, p, len);
                    }
                  else
                    {
                       sfunc = evas_common_gfx_func_composite_color_span_get(color, dst->cache_entry.flags.alpha, len, dc->render_op);
                       if (sfunc)
                         sfunc(NULL, NULL, color, p, len);
                    }
               }
          }
        return;
     }

   if (dc->clip.mask)
     pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, dc->render_op);
   else
     pfunc = evas_common_gfx_func_composite_color_pt_get(color, dst->cache_entry.flags.alpha, dc->render_op);
   if (!pfunc) return;

   if (dx == 0)
     {
        if ((x0 >= lx) && (x0 <= rx))
          {
             if (y0 < ty) y0 = ty;
             if (y1 > by) y1 = by;

             len = y1 - y0 + 1;
             p = dst->image.data + (dstw * y0) + x0;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             if ((dst->pixman.im) && (dc->col.pixman_color_image))
               pixman_image_composite(op, dc->col.pixman_color_image,
                                      NULL, dst->pixman.im,
                                      x0, y0, 0, 0, x0, y0, 1, len);
             else
# endif
#endif
               {
                  if (dc->clip.mask)
                    {
                       RGBA_Image *im = dc->clip.mask;
                       mask_w = im->cache_entry.w;
                       mask = im->image.data8
                          + ((y0 - dc->clip.mask_y) * mask_w)
                          + (x0 - dc->clip.mask_x);
                       while (len--)
                         {
                            pfunc(0, *mask, color, p);
                            p += dstw;
                            mask += mask_w;
                         }
                    }
                  else
                    {
                       while (len--)
                         {
                            pfunc(0, 255, color, p);
                            p += dstw;
                         }
                    }
               }
          }
        return;
     }

   if ((dy == dx) || (dy == -dx))
     {
        int   p0_in, p1_in;

        p0_in = (IN_RECT(x0, y0, clx, cly, clw, clh) ? 1 : 0);
        p1_in = (IN_RECT(x1, y1, clx, cly, clw, clh) ? 1 : 0);

        if (dy > 0)
          {
             if (!p0_in)
               {
                  x0 = x0 + (ty - y0);
                  y0 = ty;
                  if (x0 > rx) return;
                  if (x0 < lx)
                    {
                       y0 = y0 + (lx - x0);
                       x0 = lx;
                       if ((y0 < ty) || (y0 > by)) return;
                    }
               }
             if (!p1_in)
               {
                  x1 = x0 + (by - y0);
                  y1 = by;
                  if (x1 < lx) return;
                  if (x1 > rx)
                    {
                       y1 = y0 + (rx - x0);
                       x1 = rx;
                       if ((y1 < ty) || (y1 > by)) return;
                    }
               }
          }
        else
          {
             if (!p0_in)
               {
                  x0 = x0 - (by - y0);
                  y0 = by;
                  if (x0 > rx) return;
                  if (x0 < lx)
                    {
                       y0 = y0 - (lx - x0);
                       x0 = lx;
                       if ((y0 < ty) || (y0 > by)) return;
                    }
               }
             if (!p1_in)
               {
                  x1 = x0 - (ty - y0);
                  y1 = ty;
                  if (x1 < lx) return;
                  if (x1 > rx)
                    {
                       y1 = y0 - (rx - x0);
                       x1 = rx;
                       if ((y1 < ty) || (y1 > by)) return;
                    }
               }
          }
        if (y1 > y0)
          {
             p = dst->image.data + (dstw * y0) + x0;
             len = y1 - y0 + 1;
             if (dx > 0)  dstw++;
             else  dstw--;
             if (dc->clip.mask)
               {
                  RGBA_Image *im = dc->clip.mask;
                  mask_w = im->cache_entry.w;
                  mask = im->image.data8
                     + ((y0 - dc->clip.mask_y) * mask_w)
                     + (x0 - dc->clip.mask_x);
                  if (dx > 0) mask_w++;
                  else mask_w--;
               }
          }
        else
          {
             len = y0 - y1 + 1;
             p = dst->image.data + (dstw * y1) + x1;
             if (dx > 0)  dstw--;
             else  dstw++;
             if (dc->clip.mask)
               {
                  RGBA_Image *im = dc->clip.mask;
                  mask_w = im->cache_entry.w;
                  mask = im->image.data8
                     + ((y1 - dc->clip.mask_y) * mask_w)
                     + (x1 - dc->clip.mask_x);
                  if (dx > 0) mask_w--;
                  else mask_w++;
               }
          }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
        int pixman_x_position = x0;
        int pixman_y_position = y0;
        int x_unit = dstw - dst->cache_entry.w;
# endif
#endif
        while (len--)
          {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             if ((dst->pixman.im) && (dc->col.pixman_color_image))
               pixman_image_composite(op, dc->col.pixman_color_image,
                                      NULL, dst->pixman.im,
                                      pixman_x_position,
                                      pixman_y_position,
                                      0, 0, pixman_x_position,
                                      pixman_y_position, 1, 1);
             else
# endif
#endif
               {
                  if (mask)
                    {
                       pfunc(0, *mask, color, p);
                       mask += mask_w;
                    }
                  else
                    pfunc(0, 255, color, p);
               }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             pixman_x_position += x_unit;
             pixman_y_position += 1;
# endif
#endif
             p += dstw;
          }
     }
}

/**
 * @def SETUP_LINE_SHALLOW
 * @internal
 * @brief Macro to set up parameters for drawing a shallow line (more horizontal than vertical).
 *
 * This macro initializes variables for Bresenham-like line drawing algorithms
 * specifically tailored for lines where the change in x is greater than the change in y.
 * It handles clipping to the destination image boundaries.
 *
 * Variables used and set by this macro (assumed to be in scope):
 * - `x0`, `y0`, `x1`, `y1`: Line endpoints (modified if x0 > x1).
 * - `dx`, `dy`: Differences in x and y (modified if x0 > x1).
 * - `px`, `py`: Current pixel coordinates.
 * - `p0_in`, `p1_in`: Flags indicating if endpoints are within clip bounds.
 * - `clw`, `clh`: Clip width and height.
 * - `dely`: Step direction for y (1 or -1).
 * - `dh`: Destination image data step for y (dstw or -dstw).
 * - `dstw`: Destination image width.
 * - `dyy`: (dy << 16) / dx, scaled slope for y.
 * - `dxx`: (dx << 16) / dy, scaled slope for x (used for clipping).
 * - `a_a`: Anti-aliasing flag (0 for non-AA, 1 for AA).
 * - `x`, `y`: Temporary coordinate variables.
 * - `xx`, `yy`: Fixed-point accumulators for coordinates.
 * - `p`: Pointer to current pixel in destination image data.
 * - `data`: Pointer to the start of the clipped destination image data.
 * - `prev_y`: Previous y-coordinate, for AA line drawing.
 * - `rx`, `by`: Right and bottom clip boundaries.
 */
#define SETUP_LINE_SHALLOW                                              \
  if (x0 > x1)                                                          \
    {									\
       EXCHANGE_POINTS(x0, y0, x1, y1);                                 \
       dx = -dx;							\
       dy = -dy;							\
    }									\
                                                                        \
  px = x0;								\
  py = y0;								\
                                                                        \
  p0_in = (IN_RANGE(x0 , y0 , clw, clh) ? 1 : 0);                       \
  p1_in = (IN_RANGE(x1 , y1 , clw, clh) ? 1 : 0);                       \
                                                                        \
  dely = 1;								\
  dh = dstw;								\
  if (dy < 0)								\
    {									\
       dely = -1;							\
       dh = -dstw;							\
    }									\
                                                                        \
  dyy = ((dy) << 16) / (dx);						\
                                                                        \
  if (!p0_in)								\
    {									\
       dxx = ((dx) << 16) / (dy);					\
       if (px < 0)							\
         {								\
            x = -px;  px = 0;						\
            yy = x * dyy;						\
            y = yy >> 16;						\
            if (!a_a)							\
              y += (yy - (y << 16)) >> 15;                              \
            py += y;							\
            if ((dely > 0) && (py >= clh))                              \
              return;							\
            else if ((dely < 0) && (py < -1))                           \
              return;							\
         }								\
                                                                        \
       y = 0;								\
       if ((dely > 0) && (py < 0))                                      \
         y = (-1 - py);                                                 \
       else if ((dely < 0) && (py >= clh))                              \
         y = (clh - 1 - py);                                            \
                                                                        \
       xx = y * dxx;							\
       x = xx >> 16;							\
       if (!a_a)							\
         x += (xx - (x << 16)) >> 15;                                   \
       px += x;								\
       if (px >= clw) return;                                           \
                                                                        \
       yy = x * dyy;							\
       y = yy >> 16;							\
       if (!a_a)							\
         y += (yy - (y << 16)) >> 15;                                   \
       py += y;                                                         \
       if ((dely > 0) && (py >= clh))                                   \
         return;							\
       else if ((dely < 0) && (py < -1))                                \
         return;							\
    }									\
                                                                        \
  p = data + (dstw * py) + px;                                          \
                                                                        \
  x = px - x0;                                                          \
  yy = x * dyy;                                                         \
  prev_y = (yy >> 16);                                                  \
                                                                        \
  rx = MIN(x1 + 1, clw);						\
  by = clh - 1;

/**
 * @def SETUP_LINE_STEEP
 * @internal
 * @brief Macro to set up parameters for drawing a steep line (more vertical than horizontal).
 *
 * This macro initializes variables for Bresenham-like line drawing algorithms
 * specifically tailored for lines where the change in y is greater than or equal to the change in x.
 * It handles clipping to the destination image boundaries.
 *
 * Variables used and set by this macro (assumed to be in scope):
 * - `x0`, `y0`, `x1`, `y1`: Line endpoints (modified if y0 > y1).
 * - `dx`, `dy`: Differences in x and y (modified if y0 > y1).
 * - `px`, `py`: Current pixel coordinates.
 * - `p0_in`, `p1_in`: Flags indicating if endpoints are within clip bounds.
 * - `clw`, `clh`: Clip width and height.
 * - `delx`: Step direction for x (1 or -1).
 * - `dstw`: Destination image width.
 * - `dxx`: (dx << 16) / dy, scaled slope for x.
 * - `dyy`: (dy << 16) / dx, scaled slope for y (used for clipping).
 * - `a_a`: Anti-aliasing flag (0 for non-AA, 1 for AA).
 * - `x`, `y`: Temporary coordinate variables.
 * - `xx`, `yy`: Fixed-point accumulators for coordinates.
 * - `p`: Pointer to current pixel in destination image data.
 * - `data`: Pointer to the start of the clipped destination image data.
 * - `prev_x`: Previous x-coordinate, for AA line drawing.
 * - `rx`, `by`: Right and bottom clip boundaries.
 */
#define SETUP_LINE_STEEP                                                \
  if (y0 > y1)								\
    {									\
       EXCHANGE_POINTS(x0, y0, x1, y1);                                 \
       dx = -dx;                                                        \
       dy = -dy;                                                        \
    }									\
                                                                        \
  px = x0;                                                              \
  py = y0;                                                              \
                                                                        \
  p0_in = (IN_RANGE(x0 , y0 , clw, clh) ? 1 : 0);                       \
  p1_in = (IN_RANGE(x1 , y1 , clw, clh) ? 1 : 0);                       \
                                                                        \
  delx = 1;								\
  if (dx < 0)								\
    delx = -1;								\
                                                                        \
  dxx = ((dx) << 16) / (dy);						\
                                                                        \
  if (!p0_in)								\
    {									\
       dyy = ((dy) << 16) / (dx);                                       \
                                                                        \
       if (py < 0)                                                      \
         {                                                              \
	   y = -py;  py = 0;						\
	   xx = y * dxx;                                                \
	   x = xx >> 16;                                                \
	   if (!a_a)							\
             x += (xx - (x << 16)) >> 15;                               \
	   px += x;                                                     \
	   if ((delx > 0) && (px >= clw))				\
             return;							\
	   else if ((delx < 0) && (px < -1))                            \
             return;							\
         }                                                              \
                                                                        \
       x = 0;								\
       if ((delx > 0) && (px < -1))					\
         x = (-1 - px);							\
       else if ((delx < 0) && (px >= clw))				\
         x = (clw - 1 - px);						\
                                                                        \
       yy = x * dyy;							\
       y = yy >> 16;							\
       if (!a_a)                                                        \
         y += (yy - (y << 16)) >> 15;                                   \
       py += y;								\
       if (py >= clh) return;						\
                                                                        \
       xx = y * dxx;							\
       x = xx >> 16;							\
       if (!a_a)                                                        \
         x += (xx - (x << 16)) >> 15;                                   \
       px += x;								\
       if ((delx > 0) && (px >= clw))                                   \
         return;                                                        \
       else if ((delx < 0) && (px < -1))				\
         return;                                                        \
    }									\
                                                                        \
  p = data + (dstw * py) + px;                                          \
                                                                        \
  y = py - y0;								\
  xx = y * dxx;								\
  prev_x = (xx >> 16);							\
                                                                        \
  by = MIN(y1 + 1, clh);						\
  rx = clw - 1;

/**
 * @internal
 * @brief Draws a simple horizontal, vertical, or 45-degree diagonal line with explicit parameters.
 *
 * This function is similar to _evas_draw_simple_line but takes all parameters
 * explicitly, including clipping and masking information. This version is
 * suitable for use in rendering threads where a full draw context might not be
 * available or convenient.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param clip_x The x-coordinate of the clip rectangle.
 * @param clip_y The y-coordinate of the clip rectangle.
 * @param clip_w The width of the clip rectangle.
 * @param clip_h The height of the clip rectangle.
 * @param color The color of the line (in DATA32 format).
 * @param render_op The rendering operation.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @param mask_ie Optional mask image. If NULL, no mask is applied.
 * @param mask_x The x-offset for the mask image.
 * @param mask_y The y-offset for the mask image.
 */
static void
_draw_render_thread_simple_line(RGBA_Image *dst, int clip_x, int clip_y, int clip_w, int clip_h, DATA32 color, int render_op, int x0, int y0, int x1, int y1, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   int     dx, dy, len, lx, ty, rx, by;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p;
   DATA8   *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;
   RGBA_Gfx_Func    sfunc;

   dstw = dst->cache_entry.w;

   if (y0 > y1)
      EXCHANGE_POINTS(x0, y0, x1, y1)
   if (x0 > x1)
      EXCHANGE_POINTS(x0, y0, x1, y1)

   dx = x1 - x0;
   dy = y1 - y0;

   clx = clip_x;
   cly = clip_y;
   clw = clip_w;
   clh = clip_h;

   lx = clx;
   rx = clx + clw - 1;
   ty = cly;
   by = cly + clh - 1;

   if (dy == 0)
     {
        if ((y0 >= ty) && (y0 <= by))
          {
             if (dx < 0)
               {
                  int  tmp = x1;

                  x1 = x0;
                  x0 = tmp;
               }

             if (x0 < lx) x0 = lx;
             if (x1 > rx) x1 = rx;

             len = x1 - x0 + 1;
             p = dst->image.data + (dstw * y0) + x0;
             if (mask_ie)
               {
                  mask = mask_ie->image.data8
                     + ((y0 - mask_y) * mask_ie->cache_entry.w)
                     + (x0 - mask_x);
                  sfunc = evas_common_gfx_func_composite_mask_color_span_get(color, dst->cache_entry.flags.alpha, len, render_op);
                  if (sfunc) sfunc(NULL, mask, color, p, len);
               }
             else
               {
                  sfunc = evas_common_gfx_func_composite_color_span_get(color, dst->cache_entry.flags.alpha, len, render_op);
                  if (sfunc) sfunc(NULL, NULL, color, p, len);
               }
          }
        return;
     }

   if (mask_ie)
     pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
   else
     pfunc = evas_common_gfx_func_composite_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
   if (!pfunc) return;

   if (dx == 0)
     {
        if ((x0 >= lx) && (x0 <= rx))
          {
             if (y0 < ty) y0 = ty;
             if (y1 > by) y1 = by;

             len = y1 - y0 + 1;
             p = dst->image.data + (dstw * y0) + x0;
             if (mask_ie)
               {
                  mask_w = mask_ie->cache_entry.w;
                  mask = mask_ie->image.data8
                     + ((y0 - mask_y) * mask_w)
                     + (x0 - mask_x);
                  while (len--)
                    {
                       pfunc(0, *mask, color, p);
                       p += dstw;
                       mask += mask_w;
                    }
               }
             else
               {
                  while (len--)
                    {
                       pfunc(0, 255, color, p);
                       p += dstw;
                    }
               }
          }
        return;
     }

   if ((dy == dx) || (dy == -dx))
     {
        int   p0_in, p1_in;

        p0_in = (IN_RECT(x0, y0, clx, cly, clw, clh) ? 1 : 0);
        p1_in = (IN_RECT(x1, y1, clx, cly, clw, clh) ? 1 : 0);

        if (dy > 0)
          {
             if (!p0_in)
               {
                  x0 = x0 + (ty - y0);
                  y0 = ty;
                  if (x0 > rx) return;
                  if (x0 < lx)
                    {
                       y0 = y0 + (lx - x0);
                       x0 = lx;
                       if ((y0 < ty) || (y0 > by)) return;
                    }
               }
             if (!p1_in)
               {
                  x1 = x0 + (by - y0);
                  y1 = by;
                  if (x1 < lx) return;
                  if (x1 > rx)
                    {
                       y1 = y0 + (rx - x0);
                       x1 = rx;
                       if ((y1 < ty) || (y1 > by)) return;
                    }
               }
          }
        else
          {
             if (!p0_in)
               {
                  x0 = x0 - (by - y0);
                  y0 = by;
                  if (x0 > rx) return;
                  if (x0 < lx)
                    {
                       y0 = y0 - (lx - x0);
                       x0 = lx;
                       if ((y0 < ty) || (y0 > by)) return;
                    }
               }
             if (!p1_in)
               {
                  x1 = x0 - (ty - y0);
                  y1 = ty;
                  if (x1 < lx) return;
                  if (x1 > rx)
                    {
                       y1 = y0 - (rx - x0);
                       x1 = rx;
                       if ((y1 < ty) || (y1 > by)) return;
                    }
               }
          }
        if (y1 > y0)
          {
             p = dst->image.data + (dstw * y0) + x0;
             len = y1 - y0 + 1;
             if (dx > 0)  dstw++;
             else  dstw--;
             if (mask_ie)
               {
                  mask_w = mask_ie->cache_entry.w;
                  mask = mask_ie->image.data8
                     + ((y0 - mask_y) * mask_w)
                     + (x0 - mask_x);
                  if (dx > 0) mask_w++;
                  else mask_w--;
               }
          }
        else
          {
             len = y0 - y1 + 1;
             p = dst->image.data + (dstw * y1) + x1;
             if (dx > 0)  dstw--;
             else  dstw++;
             if (mask_ie)
               {
                  mask_w = mask_ie->cache_entry.w;
                  mask = mask_ie->image.data8
                     + ((y1 - mask_y) * mask_w)
                     + (x1 - mask_x);
                  if (dx > 0) mask_w--;
                  else mask_w++;
               }
          }
        if (mask)
          {
             while (len--)
               {
                  pfunc(0, *mask, color, p);
                  p += dstw;
                  mask += mask_w;
               }
          }
        else
          {
             while (len--)
               {
                  pfunc(0, 255, color, p);
                  p += dstw;
               }
          }
     }
}

/**
 * @brief Draws a general non-anti-aliased line with explicit parameters.
 * @ingroup Evas_Common_Line
 *
 * This function draws a line between (x0, y0) and (x1, y1) using a
 * Bresenham-like algorithm. It handles clipping and optional masking.
 * If the line is horizontal, vertical, or a 45-degree diagonal, it delegates
 * to the optimized _draw_render_thread_simple_line function.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param clip_x The x-coordinate of the clip rectangle.
 * @param clip_y The y-coordinate of the clip rectangle.
 * @param clip_w The width of the clip rectangle.
 * @param clip_h The height of the clip rectangle.
 * @param color The color of the line (in DATA32 format).
 * @param render_op The rendering operation.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @param mask_ie Optional mask image. If NULL, no mask is applied.
 * @param mask_x The x-offset for the mask image.
 * @param mask_y The y-offset for the mask image.
 */
EVAS_API void
evas_common_line_draw_line(RGBA_Image *dst, int clip_x, int clip_y, int clip_w, int clip_h, DATA32 color, int render_op, int x0, int y0, int x1, int y1, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   int     px, py, x, y, prev_x, prev_y;
   int     dx, dy, rx, by, p0_in, p1_in, dh, a_a = 0;
   int     delx, dely, xx, yy, dxx, dyy;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p, *data;
   DATA8   *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;

   dx = x1 - x0;
   dy = y1 - y0;

   if ( (dx == 0) || (dy == 0) || (dx == dy) || (dx == -dy) )
     {
	_draw_render_thread_simple_line
          (dst, clip_x, clip_y, clip_w, clip_h,
           color, render_op,
           x0, y0, x1, y1,
           mask_ie, mask_x, mask_y);
	return;
     }

   if (mask_ie)
     pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
   else
     pfunc = evas_common_gfx_func_composite_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
   if (!pfunc) return;

   clx = clip_x;
   cly = clip_y;
   clw = clip_w;
   clh = clip_h;

   data = dst->image.data;
   dstw = dst->cache_entry.w;

   data += (dstw * cly) + clx;
   x0 -= clx;
   y0 -= cly;
   x1 -= clx;
   y1 -= cly;

   if (mask_ie)
     {
        mask_w = mask_ie->cache_entry.w;
        mask = mask_ie->image.data8
           + ((cly - mask_y) * mask_w) + (clx - mask_x);
     }

   /* shallow: x-parametric */
   if ((dy < dx) || (dy < -dx))
     {
	SETUP_LINE_SHALLOW;

        if (mask) mask += (py * mask_w) + px;
	while (px < rx)
	  {
	    y = (yy >> 16);
	    y += ((yy - (y << 16)) >> 15);
	    if (prev_y != y)
	      {
		prev_y = y;
		p += dh;
                if (mask) mask += mask_w;
		py += dely;
	      }
	    if (!p1_in)
	      {
		if ((py < 0) && (dely < 0)) return;
		if ((py > by) && (dely > 0)) return;
	      }
            if (!p0_in)
              {
                 if (py < 0) goto next_x;
              }
            if (IN_RANGE(px, py, clw, clh))
              {
                 if (mask) pfunc(0, *mask, color, p);
                 else pfunc(0, 255, color, p);
              }

next_x:
            yy += dyy;
            px++;
            p++;
            if (mask) mask++;
	  }
	return;
     }

   /* steep: y-parametric */

   SETUP_LINE_STEEP;
   if (mask) mask += (py * mask_w) + px;

   while (py < by)
     {
	x = (xx >> 16);
	x += ((xx - (x << 16)) >> 15);
	if (prev_x != x)
	  {
             prev_x = x;
             px += delx;
             p += delx;
             if (mask) mask += delx;
	  }
	if (!p1_in)
	  {
	    if ((px < 0) && (delx < 0)) return;
	    if ((px > rx) && (delx > 0)) return;
	  }
        if (!p0_in)
          {
             if (px < 0) goto next_y;
          }
        if (IN_RANGE(px, py, clw, clh))
          {
             if (mask) pfunc(0, *mask, color, p);
             else pfunc(0, 255, color, p);
          }

next_y:
	xx += dxx;
	py++;
	p += dstw;
        if (mask) mask += mask_w;
     }
}

/**
 * @internal
 * @brief Draws a general non-anti-aliased line using a draw context.
 *
 * This function draws a line between (x0, y0) and (x1, y1) using a
 * Bresenham-like algorithm. It uses the drawing context for color, clipping,
 * and render operation. If the line is horizontal, vertical, or a 45-degree
 * diagonal, it delegates to the optimized _evas_draw_simple_line function.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 */
static void
_evas_draw_line(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1)
{
   int     px, py, x, y, prev_x, prev_y;
   int     dx, dy, rx, by, p0_in, p1_in, dh, a_a = 0;
   int     delx, dely, xx, yy, dxx, dyy;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p, *data, color;
   DATA8   *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;

   dx = x1 - x0;
   dy = y1 - y0;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
   int pix_x;
   int pix_y;
   int pix_x_unit;
   int pix_y_unit;

   pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY
   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;
   pix_x = x0;
   pix_y = y0;

   if (dx < 0)
     pix_x_unit = -1;
   else
     pix_x_unit = 1;

   if (dy < 0)
     pix_y_unit = -1;
   else
     pix_y_unit = 1;
# endif
#endif

   if ( (dx == 0) || (dy == 0) || (dx == dy) || (dx == -dy) )
     {
	_evas_draw_simple_line(dst, dc, x0, y0, x1, y1);
	return;
     }

   color = dc->col.col;
   if (dc->clip.mask)
     pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, dc->render_op);
   else
     pfunc = evas_common_gfx_func_composite_color_pt_get(color, dst->cache_entry.flags.alpha, dc->render_op);
   if (!pfunc) return;

   clx = dc->clip.x;
   cly = dc->clip.y;
   clw = dc->clip.w;
   clh = dc->clip.h;

   data = dst->image.data;
   dstw = dst->cache_entry.w;

   data += (dstw * cly) + clx;
   x0 -= clx;
   y0 -= cly;
   x1 -= clx;
   y1 -= cly;

   if (dc->clip.mask)
     {
        RGBA_Image *im = dc->clip.mask;
        mask_w = im->cache_entry.w;
        mask = im->image.data8
           + ((cly - dc->clip.mask_y) * mask_w) + (clx - dc->clip.mask_x);
     }

   /* shallow: x-parametric */
   if ((dy < dx) || (dy < -dx))
     {
	SETUP_LINE_SHALLOW;

        if (mask) mask += (py * mask_w) + px;
	while (px < rx)
	  {
	    y = (yy >> 16);
	    y += ((yy - (y << 16)) >> 15);
	    if (prev_y != y)
	      {
		prev_y = y;
		p += dh;
                if (mask) mask += mask_w;
		py += dely;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                 pix_y += pix_y_unit;
# endif
#endif
	      }
	    if (!p1_in)
	      {
		if ((py < 0) && (dely < 0)) return;
		if ((py > by) && (dely > 0)) return;
	      }
            if (!p0_in)
              {
                 if (py < 0) goto next_x;
              }
            if (IN_RANGE(px, py, clw, clh))
              {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                 if ((dst->pixman.im) && (dc->col.pixman_color_image))
                   pixman_image_composite(op, dc->col.pixman_color_image,
                                          NULL, dst->pixman.im,
                                          pix_x, pix_y, 0, 0,
                                          pix_x, pix_y, 1, 1);
                 else
# endif
#endif
                   {
                      if (mask) pfunc(0, *mask, color, p);
                      else pfunc(0, 255, color, p);
                   }
              }

next_x:
            yy += dyy;
            px++;
            p++;
            if (mask) mask++;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
            pix_x += pix_x_unit;
# endif
#endif
	  }
	return;
     }

   /* steep: y-parametric */

   SETUP_LINE_STEEP;
   if (mask) mask += (py * mask_w) + px;

   while (py < by)
     {
	x = (xx >> 16);
	x += ((xx - (x << 16)) >> 15);
	if (prev_x != x)
	  {
             prev_x = x;
             px += delx;
             p += delx;
             if (mask) mask += delx;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             pix_x += pix_x_unit;
# endif
#endif
	  }
	if (!p1_in)
	  {
	    if ((px < 0) && (delx < 0)) return;
	    if ((px > rx) && (delx > 0)) return;
	  }
        if (!p0_in)
          {
             if (px < 0) goto next_y;
          }
        if (IN_RANGE(px, py, clw, clh))
          {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             if ((dst->pixman.im) && (dc->col.pixman_color_image))
               pixman_image_composite(op, dc->col.pixman_color_image,
                                      NULL, dst->pixman.im,
                                      pix_x, pix_y, 0, 0,
                                      pix_x, pix_y, 1, 1);
             else
# endif
#endif
               {
                  if (mask) pfunc(0, *mask, color, p);
                  else pfunc(0, 255, color, p);
               }
          }
next_y:
	xx += dxx;
	py++;
	p += dstw;
        if (mask) mask += mask_w;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
        pix_y += pix_y_unit;
# endif
#endif
     }
}

/**
 * @brief Draws a general anti-aliased line with explicit parameters.
 * @ingroup Evas_Common_Line
 *
 * This function draws an anti-aliased line between (x0, y0) and (x1, y1)
 * using a Xiaolin Wu-like algorithm. It handles clipping and optional masking.
 * If the line is horizontal, vertical, or a 45-degree diagonal, it delegates
 * to the non-anti-aliased _draw_render_thread_simple_line function, as AA
 * is not typically needed or well-defined for such integer-aligned lines.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param clip_x The x-coordinate of the clip rectangle.
 * @param clip_y The y-coordinate of the clip rectangle.
 * @param clip_w The width of the clip rectangle.
 * @param clip_h The height of the clip rectangle.
 * @param color The color of the line (in DATA32 format).
 * @param render_op The rendering operation.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @param mask_ie Optional mask image. If NULL, no mask is applied.
 * @param mask_x The x-offset for the mask image.
 * @param mask_y The y-offset for the mask image.
 */
EVAS_API void
evas_common_line_draw_line_aa(RGBA_Image *dst, int clip_x, int clip_y, int clip_w, int clip_h, DATA32 color, int render_op, int x0, int y0, int x1, int y1, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   int     px, py, x, y, prev_x, prev_y;
   int     dx, dy, rx, by, p0_in, p1_in, dh, a_a = 1;
   int     delx, dely, xx, yy, dxx, dyy;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p, *data;
   DATA8 *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;

   dx = x1 - x0;
   dy = y1 - y0;

   if (y0 > y1)
     EXCHANGE_POINTS(x0, y0, x1, y1);

   dx = x1 - x0;
   dy = y1 - y0;

   if ((dx == 0) || (dy == 0) || (dx == dy) || (dx == -dy))
     {
	_draw_render_thread_simple_line
          (dst, clip_x, clip_y, clip_w, clip_h,
           color, render_op,
           x0, y0, x1, y1,
           mask_ie, mask_x, mask_y);
	return;
     }

   pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, render_op);
   if (!pfunc) return;

   clx = clip_x;
   cly = clip_y;
   clw = clip_w;
   clh = clip_h;

   data = evas_cache_image_pixels(&dst->cache_entry);
   dstw = dst->cache_entry.w;

   data += (dstw * cly) + clx;
   x0 -= clx;
   y0 -= cly;
   x1 -= clx;
   y1 -= cly;

   if (mask_ie)
     {
        mask_w = mask_ie->cache_entry.w;
        mask = mask_ie->image.data8
           + (cly - mask_y) * mask_w + (clx - mask_x);
     }

   /* shallow: x-parametric */
   if ((dy < dx) || (dy < -dx))
     {
	SETUP_LINE_SHALLOW;

        if (mask) mask += (py * mask_w) + px;
	while (px < rx)
	  {
	    DATA8   aa;

	    y = (yy >> 16);
	    if (prev_y != y)
	      {
                 prev_y = y;
                 p += dh;
                 if (mask) mask += mask_w;
                 py += dely;
	      }
	    if (!p1_in)
	      {
		if ((py < 0) && (dely < 0)) return;
		if ((py > by) && (dely > 0)) return;
	      }
            if (!p0_in)
              {
                 if (py < 0) goto next_x;
              }
	    if (px < clw)
	      {
                 aa = ((yy - (y << 16)) >> 8);

                 if (mask)
                   {
                      if ((py) < clh) pfunc(0, (255 - aa) * (*mask) / 255, color, p);
                      if ((py + 1) < clh) pfunc(0, aa * (*(mask + mask_w)) / 255, color, p + dstw);
                   }
                 else
                   {
                      if ((py) < clh) pfunc(0, 255 - aa, color, p);
                      if ((py + 1) < clh) pfunc(0, aa, color, p + dstw);
                   }
              }

next_x:
             yy += dyy;
             px++;
             p++;
             if (mask) mask++;
	  }
	return;
     }

   /* steep: y-parametric */
   SETUP_LINE_STEEP;
   if (mask) mask += (py * mask_w) + px;

   while (py < by)
     {
	DATA8   aa;

	x = (xx >> 16);
	if (prev_x != x)
	  {
             prev_x = x;
             px += delx;
             p += delx;
             if (mask) mask += delx;
	  }
	if (!p1_in)
	  {
             if ((px < 0) && (delx < 0)) return;
             if ((px > rx) && (delx > 0)) return;
	  }
        if (!p0_in)
          {
             if (px < 0) goto next_y;
          }
	if (py < clh)
	  {
             aa = ((xx - (x << 16)) >> 8);

             if (mask)
               {
                  if ((px) < clw) pfunc(0, (255 - aa) * (*mask) / 255, color, p);
                  if ((px + 1) < clw) pfunc(0, aa * (*(mask + 1)) / 255, color, p + 1);
               }
             else
               {
                  if ((px) < clw) pfunc(0, 255 - aa, color, p);
                  if ((px + 1) < clw) pfunc(0, aa, color, p + 1);
               }
          }

     next_y:
	xx += dxx;
	py++;
	p += dstw;
        if (mask) mask += mask_w;
     }
}

/**
 * @internal
 * @brief Draws a general anti-aliased line using a draw context.
 *
 * This function draws an anti-aliased line between (x0, y0) and (x1, y1)
 * using a Xiaolin Wu-like algorithm. It uses the drawing context for color,
 * clipping, and render operation. If the line is horizontal, vertical, or
 * a 45-degree diagonal, it delegates to the non-anti-aliased
 * _evas_draw_simple_line function.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 */
static void
_evas_draw_line_aa(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1)
{
   int     px, py, x, y, prev_x, prev_y;
   int     dx, dy, rx, by, p0_in, p1_in, dh, a_a = 1;
   int     delx, dely, xx, yy, dxx, dyy;
   int     clx, cly, clw, clh;
   int     dstw, mask_w = 0;
   DATA32  *p, *data, color;
   DATA8   *mask = NULL;
   RGBA_Gfx_Pt_Func pfunc;

   dx = x1 - x0;
   dy = y1 - y0;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
   int pix_x;
   int pix_y;
   int pix_x_unit;
   int pix_y_unit;

   pixman_image_t *aa_mask_image;
   int alpha_data_buffer;

   pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY
   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;
   pix_x = x0;
   pix_y = y0;

   if (dx < 0)
     pix_x_unit = -1;
   else
     pix_x_unit = 1;

   if (dy < 0)
     pix_y_unit = -1;
   else
     pix_y_unit = 1;
# endif
#endif
   if (y0 > y1)
     EXCHANGE_POINTS(x0, y0, x1, y1);

   dx = x1 - x0;
   dy = y1 - y0;

   if ((dx == 0) || (dy == 0) || (dx == dy) || (dx == -dy))
     {
	_evas_draw_simple_line(dst, dc, x0, y0, x1, y1);
	return;
     }

   color = dc->col.col;
   pfunc = evas_common_gfx_func_composite_mask_color_pt_get(color, dst->cache_entry.flags.alpha, dc->render_op);
   if (!pfunc) return;

   clx = dc->clip.x;
   cly = dc->clip.y;
   clw = dc->clip.w;
   clh = dc->clip.h;

   data = evas_cache_image_pixels(&dst->cache_entry);
   dstw = dst->cache_entry.w;

   data += (dstw * cly) + clx;
   x0 -= clx;
   y0 -= cly;
   x1 -= clx;
   y1 -= cly;

   if (dc->clip.mask)
     {
        RGBA_Image *im = dc->clip.mask;
        mask_w = im->cache_entry.w;
        mask = im->image.data8
           + ((cly - dc->clip.mask_y) * mask_w) + (clx - dc->clip.mask_x);
     }

   /* shallow: x-parametric */
   if ((dy < dx) || (dy < -dx))
     {
	SETUP_LINE_SHALLOW;

        if (mask) mask += (py * mask_w) + px;
	while (px < rx)
	  {
	    DATA8   aa;

	    y = (yy >> 16);
	    if (prev_y != y)
	      {
                 prev_y = y;
                 p += dh;
                 if (mask) mask += mask_w;
                 py += dely;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                 pix_y += pix_y_unit;
# endif
#endif
	      }
	    if (!p1_in)
	      {
		if ((py < 0) && (dely < 0)) return;
		if ((py > by) && (dely > 0)) return;
	      }
            if (!p0_in)
              {
                 if (py < 0) goto next_x;
              }
	    if (px < clw)
	      {
                 aa = ((yy - (y << 16)) >> 8);
                 if ((py) < clh)
                   {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                      alpha_data_buffer = 255 - aa;
                      aa_mask_image = pixman_image_create_bits(PIXMAN_a8, 1, 1,
                                                               (uint32_t *)&alpha_data_buffer, 4);

                      if ((dst->pixman.im) && (dc->col.pixman_color_image))
                        pixman_image_composite(op, dc->col.pixman_color_image,
                                               aa_mask_image, dst->pixman.im,
                                               pix_x, pix_y, 0, 0,
                                               pix_x, pix_y, 1, 1);
                      else
# endif
#endif
                        {
                           if (mask) pfunc(0, (255 - aa) * (*mask) / 255, color, p);
                           else pfunc(0, 255 - aa, color, p);
                        }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                      pixman_image_unref(aa_mask_image);
# endif
#endif
                    }
                 if ((py + 1) < clh)
                   {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                      alpha_data_buffer = aa;
                      aa_mask_image = pixman_image_create_bits(PIXMAN_a8, 1, 1,
                                                               (uint32_t *)&alpha_data_buffer, 4);

                      if ((dst->pixman.im) && (dc->col.pixman_color_image))
                        pixman_image_composite(op, dc->col.pixman_color_image,
                                               aa_mask_image, dst->pixman.im,
                                               pix_x, pix_y + 1, 0, 0,
                                               pix_x, pix_y + 1, 1, 1);
                      else
# endif
#endif
                        {
                           if (mask) pfunc(0, aa * (*(mask + mask_w)) / 255, color, p + dstw);
                           else pfunc(0, aa, color, p + dstw);
                        }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                      pixman_image_unref(aa_mask_image);
# endif
#endif
                   }
              }

next_x:
             yy += dyy;
             px++;
             p++;
             if (mask) mask++;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             pix_x += pix_x_unit;
# endif
#endif
	  }
	return;
     }

   /* steep: y-parametric */
   SETUP_LINE_STEEP;
   if (mask) mask += (py * mask_w) + px;

   while (py < by)
     {
	DATA8   aa;

	x = (xx >> 16);
	if (prev_x != x)
	  {
             prev_x = x;
             px += delx;
             p += delx;
             if (mask) mask += delx;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
             pix_x += pix_x_unit;
# endif
#endif
	  }
	if (!p1_in)
	  {
             if ((px < 0) && (delx < 0)) return;
             if ((px > rx) && (delx > 0)) return;
	  }
        if (!p0_in)
          {
             if (px < 0) goto next_y;
          }
	if (py < clh)
	  {
             aa = ((xx - (x << 16)) >> 8);
             if ((px) < clw)
               {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                  alpha_data_buffer = 255 - aa;
                  aa_mask_image = pixman_image_create_bits(PIXMAN_a8, 1, 1, (uint32_t *)&alpha_data_buffer, 4);

                  if ((dst->pixman.im) && (dc->col.pixman_color_image))
                    pixman_image_composite(op, dc->col.pixman_color_image,
                                           aa_mask_image, dst->pixman.im,
                                           pix_x, pix_y, 0, 0,
                                           pix_x, pix_y, 1, 1);
                  else
# endif
#endif
                    {
                       if (mask) pfunc(0, (255 - aa) * (*mask) / 255, color, p);
                       else pfunc(0, 255 - aa, color, p);
                    }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                  pixman_image_unref(aa_mask_image);
# endif
#endif

               }
             if ((px + 1) < clw)
               {
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                  alpha_data_buffer = aa;
                  aa_mask_image = pixman_image_create_bits(PIXMAN_a8, 1, 1,
                                                           (uint32_t *)&alpha_data_buffer, 4);

                  if ((dst->pixman.im) && (dc->col.pixman_color_image))
                       pixman_image_composite(op, dc->col.pixman_color_image,
                                              aa_mask_image, dst->pixman.im,
                                              pix_x + 1, pix_y, 0, 0,
                                              pix_x + 1, pix_y, 1, 1);
                  else
# endif
#endif
                    {
                       if (mask) pfunc(0, aa * (*(mask + 1)) / 255, color, p + 1);
                       else pfunc(0, aa, color, p + 1);
                    }
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
                  pixman_image_unref(aa_mask_image);
# endif
#endif
               }
          }
     next_y:
	xx += dxx;
	py++;
	p += dstw;
        if (mask) mask += mask_w;
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_LINE
        pix_y += pix_y_unit;
# endif
#endif
     }
}

/**
 * @brief Draws a line using a specified callback function after setting up clipping.
 * @ingroup Evas_Common_Line
 *
 * This function prepares the drawing context's clip region based on the
 * destination image bounds, the current clip settings in `dc`, and the
 * bounding box of the line. It then calls the provided callback `cb`
 * to perform the actual line drawing. After the callback returns,
 * the original clip settings in `dc` are restored.
 *
 * This allows for different line drawing algorithms (e.g., anti-aliased or not)
 * to be used with a common clipping setup.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 * @param cb The callback function (Evas_Common_Line_Draw_Cb) to execute for drawing.
 *           This callback will be one of _evas_draw_line or _evas_draw_line_aa.
 */
EVAS_API void
evas_common_line_draw_cb(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1, Evas_Common_Line_Draw_Cb cb)
{
   int  x, y, w, h;
   int  clx, cly, clw, clh;
   int  cuse, cx, cy, cw, ch;

   /* No cutout ? FIXME ? */

   if ((x0 == x1) && (y0 == y1))
     {
	_evas_draw_point(dst, dc, x0, y0);
	return;
     }

   clx = cly = 0;
   clw = dst->cache_entry.w;
   clh = dst->cache_entry.h;

   /* save out clip info */
   cuse = dc->clip.use;
   cx = dc->clip.x;
   cy = dc->clip.y;
   cw = dc->clip.w;
   ch = dc->clip.h;

   if (cuse)
     {
	RECTS_CLIP_TO_RECT(clx, cly, clw, clh, cx, cy, cw, ch);
	if ((clw < 1) || (clh < 1))
	   return;
     }

   x = MIN(x0, x1);
   y = MIN(y0, y1);
   w = MAX(x0, x1) - x + 1;
   h = MAX(y0, y1) - y + 1;

   RECTS_CLIP_TO_RECT(clx, cly, clw, clh, x, y, w, h);
   if ((clw < 1) || (clh < 1))
	return;

   dc->clip.use = 1;
   dc->clip.x = clx;
   dc->clip.y = cly;
   dc->clip.w = clw;
   dc->clip.h = clh;

   cb(dst, dc, x0, y0, x1, y1);

   /* restore clip info */
   dc->clip.use = cuse;
   dc->clip.x = cx;
   dc->clip.y = cy;
   dc->clip.w = cw;
   dc->clip.h = ch;
}

/**
 * @brief Main entry point for drawing a line using a draw context.
 * @ingroup Evas_Common_Line
 *
 * This function selects the appropriate line drawing function (anti-aliased
 * or non-anti-aliased) based on the `anti_alias` flag in the drawing context `dc`.
 * It then calls evas_common_line_draw_cb() to set up clipping and execute
 * the selected drawing function.
 *
 * @param dst The destination RGBA_Image to draw on.
 * @param dc The drawing context, which specifies color, render operation,
 *           clipping, and whether anti-aliasing should be used.
 * @param x0 The x-coordinate of the start point of the line.
 * @param y0 The y-coordinate of the start point of the line.
 * @param x1 The x-coordinate of the end point of the line.
 * @param y1 The y-coordinate of the end point of the line.
 */
EVAS_API void
evas_common_line_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x0, int y0, int x1, int y1)
{
   Evas_Common_Line_Draw_Cb cb;

   if (dc->anti_alias) cb = _evas_draw_line_aa;
   else cb = _evas_draw_line;

   evas_common_line_draw_cb(dst, dc, x0, y0, x1, y1, cb);
}
