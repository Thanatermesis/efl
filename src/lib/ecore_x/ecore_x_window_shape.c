#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"

/**
 * @defgroup Ecore_X_Window_Shape X Window Shape Functions
 * @ingroup Ecore_X_Group
 *
 * These functions use the shape extension of the X server to change
 * shape of given windows.
 */

/**
 * Sets the shape of the given window to that given by the pixmap @p mask.
 * @param   win  The given window.
 * @param   mask A 2-bit depth pixmap that provides the new shape of the
 *               window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_mask_set(Ecore_X_Window win,
                              Ecore_X_Pixmap mask)
{
   LOGFN;
   XShapeCombineMask(_ecore_x_disp, win, ShapeBounding, 0, 0, mask, ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the input shape of the given window to that given by the pixmap @p mask.
 * @param   win  The given window.
 * @param   mask A 1-bit depth pixmap that provides the new input shape of the
 *               window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_mask_set(Ecore_X_Window win,
                                    Ecore_X_Pixmap mask)
{
   LOGFN;
#ifdef ShapeInput
   XShapeCombineMask(_ecore_x_disp, win, ShapeInput, 0, 0, mask, ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
#else /* ifdef ShapeInput */
   return;
   win = mask = 0;
#endif /* ifdef ShapeInput */
}

/**
 * Sets the shape of a window from another window.
 * @param   win       The window whose shape is to be set.
 * @param   shape_win The window whose shape will be used.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_window_set(Ecore_X_Window win,
                                Ecore_X_Window shape_win)
{
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeBounding,
                      0,
                      0,
                      shape_win,
                      ShapeBounding,
                      ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the input shape of a window from another window's input shape.
 * @param   win       The window whose input shape is to be set.
 * @param   shape_win The window whose input shape will be used.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_window_set(Ecore_X_Window win,
                                      Ecore_X_Window shape_win)
{
#ifdef ShapeInput
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeInput,
                      0,
                      0,
                      shape_win,
                      ShapeInput,
                      ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = shape_win = 0;
#endif
}

/**
 * Sets the shape of a window from another window, with an offset.
 * @param   win       The window whose shape is to be set.
 * @param   shape_win The window whose shape will be used.
 * @param   x         The X offset of the shape window relative to the target window.
 * @param   y         The Y offset of the shape window relative to the target window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_window_set_xy(Ecore_X_Window win,
                                   Ecore_X_Window shape_win,
                                   int x,
                                   int y)
{
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeBounding,
                      x,
                      y,
                      shape_win,
                      ShapeBounding,
                      ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the input shape of a window from another window's input shape, with an offset.
 * @param   win       The window whose input shape is to be set.
 * @param   shape_win The window whose input shape will be used.
 * @param   x         The X offset of the shape window relative to the target window.
 * @param   y         The Y offset of the shape window relative to the target window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_window_set_xy(Ecore_X_Window win,
                                         Ecore_X_Window shape_win,
                                         int x,
                                         int y)
{
#ifdef ShapeInput
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeInput,
                      x,
                      y,
                      shape_win,
                      ShapeInput,
                      ShapeSet);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = shape_win = x = y = 0;
#endif
}

/**
 * Sets the shape of a window to a rectangle.
 * The coordinates are relative to the window's origin.
 * @param   win The window whose shape is to be set.
 * @param   x   The x-coordinate of the rectangle.
 * @param   y   The y-coordinate of the rectangle.
 * @param   w   The width of the rectangle.
 * @param   h   The height of the rectangle.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangle_set(Ecore_X_Window win,
                                   int x,
                                   int y,
                                   int w,
                                   int h)
{
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeSet,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the input shape of a window to a rectangle.
 * The coordinates are relative to the window's origin.
 * @param   win The window whose input shape is to be set.
 * @param   x   The x-coordinate of the rectangle.
 * @param   y   The y-coordinate of the rectangle.
 * @param   w   The width of the rectangle.
 * @param   h   The height of the rectangle.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangle_set(Ecore_X_Window win,
                                         int x,
                                         int y,
                                         int w,
                                         int h)
{
#ifdef ShapeInput
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeSet,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = x = y = w = h = 0;
#endif
}

/**
 * Sets the shape of a window to the union of multiple rectangles.
 * The coordinates of the rectangles are relative to the window's origin.
 * @param   win   The window whose shape is to be set.
 * @param   rects An array of Ecore_X_Rectangle structures.
 *                Example: Ecore_X_Rectangle r[] = {{0,0,10,10}, {20,0,10,10}};
 * @param   num   The number of rectangles in the @p rects array.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangles_set(Ecore_X_Window win,
                                    Ecore_X_Rectangle *rects,
                                    int num)
{
#ifdef ShapeInput
   XRectangle *rect = NULL;
   int i;

   LOGFN;
   if (!rects) return;
   if (num > 0)
     {
        rect = malloc(sizeof(XRectangle) * num);
        if (!rect) return;
        for (i = 0; i < num; i++)
          {
             rect[i].x = rects[i].x;
             rect[i].y = rects[i].y;
             rect[i].width = rects[i].width;
             rect[i].height = rects[i].height;
          }
     }
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           rect,
                           num,
                           ShapeSet,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect) free(rect);
#else
   return;
   win = rects = num = 0;
#endif
}

/**
 * Sets the input shape of a window to the union of multiple rectangles.
 * The coordinates of the rectangles are relative to the window's origin.
 * @param   win   The window whose input shape is to be set.
 * @param   rects An array of Ecore_X_Rectangle structures.
 *                Example: Ecore_X_Rectangle r[] = {{0,0,10,10}, {20,0,10,10}};
 * @param   num   The number of rectangles in the @p rects array.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangles_set(Ecore_X_Window win,
                                          Ecore_X_Rectangle *rects,
                                          int num)
{
#ifdef ShapeInput
   XRectangle *rect = NULL;
   int i;

   LOGFN;
   if (!rects) return;
   if (num > 0)
     {
        rect = malloc(sizeof(XRectangle) * num);
        if (!rect) return;
        for (i = 0; i < num; i++)
          {
             rect[i].x = rects[i].x;
             rect[i].y = rects[i].y;
             rect[i].width = rects[i].width;
             rect[i].height = rects[i].height;
          }
     }
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           rect,
                           num,
                           ShapeSet,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect) free(rect);
#else
   return;
   win = rects = num = 0;
#endif
}

/**
 * Subtracts a rectangle from the window's current shape.
 * The coordinates are relative to the window's origin.
 * @param   win The window whose shape is to be modified.
 * @param   x   The x-coordinate of the rectangle to subtract.
 * @param   y   The y-coordinate of the rectangle to subtract.
 * @param   w   The width of the rectangle to subtract.
 * @param   h   The height of the rectangle to subtract.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangle_subtract(Ecore_X_Window win,
                                        int x,
                                        int y,
                                        int w,
                                        int h)
{
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeSubtract,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Subtracts a rectangle from the window's current input shape.
 * The coordinates are relative to the window's origin.
 * @param   win The window whose input shape is to be modified.
 * @param   x   The x-coordinate of the rectangle to subtract.
 * @param   y   The y-coordinate of the rectangle to subtract.
 * @param   w   The width of the rectangle to subtract.
 * @param   h   The height of the rectangle to subtract.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangle_subtract(Ecore_X_Window win,
                                              int x,
                                              int y,
                                              int w,
                                              int h)
{
#ifdef ShapeInput
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeSubtract,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = x = y = w = h = 0;
#endif
}

/**
 * Adds the shape of another window to the current window's shape (union).
 * @param   win       The window whose shape is to be modified.
 * @param   shape_win The window whose shape will be added.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_window_add(Ecore_X_Window win,
                                Ecore_X_Window shape_win)
{
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeBounding,
                      0,
                      0,
                      shape_win,
                      ShapeBounding,
                      ShapeUnion);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Adds the shape of another window (with an offset) to the current window's shape (union).
 * @param   win       The window whose shape is to be modified.
 * @param   shape_win The window whose shape will be added.
 * @param   x         The X offset of the shape window relative to the target window.
 * @param   y         The Y offset of the shape window relative to the target window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_window_add_xy(Ecore_X_Window win,
                                   Ecore_X_Window shape_win,
                                   int x,
                                   int y)
{
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeBounding,
                      x,
                      y,
                      shape_win,
                      ShapeBounding,
                      ShapeUnion);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Adds the input shape of another window (with an offset) to the current window's input shape (union).
 * @param   win       The window whose input shape is to be modified.
 * @param   shape_win The window whose input shape will be added.
 * @param   x         The X offset of the shape window relative to the target window.
 * @param   y         The Y offset of the shape window relative to the target window.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_window_add_xy(Ecore_X_Window win,
                                         Ecore_X_Window shape_win,
                                         int x,
                                         int y)
{
#ifdef ShapeInput
   LOGFN;
   XShapeCombineShape(_ecore_x_disp,
                      win,
                      ShapeInput,
                      x,
                      y,
                      shape_win,
                      ShapeInput,
                      ShapeUnion);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = shape_win = x = y = 0;
#endif
}

/**
 * Adds a rectangle to the window's current shape (union).
 * The coordinates are relative to the window's origin.
 * @param   win The window whose shape is to be modified.
 * @param   x   The x-coordinate of the rectangle to add.
 * @param   y   The y-coordinate of the rectangle to add.
 * @param   w   The width of the rectangle to add.
 * @param   h   The height of the rectangle to add.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangle_add(Ecore_X_Window win,
                                   int x,
                                   int y,
                                   int w,
                                   int h)
{
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeUnion,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Adds a rectangle to the window's current input shape (union).
 * The coordinates are relative to the window's origin.
 * @param   win The window whose input shape is to be modified.
 * @param   x   The x-coordinate of the rectangle to add.
 * @param   y   The y-coordinate of the rectangle to add.
 * @param   w   The width of the rectangle to add.
 * @param   h   The height of the rectangle to add.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangle_add(Ecore_X_Window win,
                                         int x,
                                         int y,
                                         int w,
                                         int h)
{
#ifdef ShapeInput
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeUnion,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = x = y = w = h = 0;
#endif
}

/**
 * Clips the window's current shape by a rectangle (intersection).
 * The coordinates are relative to the window's origin.
 * @param   win The window whose shape is to be modified.
 * @param   x   The x-coordinate of the clipping rectangle.
 * @param   y   The y-coordinate of the clipping rectangle.
 * @param   w   The width of the clipping rectangle.
 * @param   h   The height of the clipping rectangle.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangle_clip(Ecore_X_Window win,
                                    int x,
                                    int y,
                                    int w,
                                    int h)
{
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeIntersect,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Clips the window's current input shape by a rectangle (intersection).
 * The coordinates are relative to the window's origin.
 * @param   win The window whose input shape is to be modified.
 * @param   x   The x-coordinate of the clipping rectangle.
 * @param   y   The y-coordinate of the clipping rectangle.
 * @param   w   The width of the clipping rectangle.
 * @param   h   The height of the clipping rectangle.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangle_clip(Ecore_X_Window win,
                                          int x,
                                          int y,
                                          int w,
                                          int h)
{
#ifdef ShapeInput
   XRectangle rect;

   LOGFN;
   rect.x = x;
   rect.y = y;
   rect.width = w;
   rect.height = h;
   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           &rect,
                           1,
                           ShapeIntersect,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   return;
   win = x = y = w = h = 0;
#endif
}

/**
 * Adds multiple rectangles to the window's current shape (union).
 * The coordinates of the rectangles are relative to the window's origin.
 * @param   win   The window whose shape is to be modified.
 * @param   rects An array of Ecore_X_Rectangle structures to add.
 *                Example: Ecore_X_Rectangle r[] = {{0,0,10,10}, {20,0,10,10}};
 * @param   num   The number of rectangles in the @p rects array.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_rectangles_add(Ecore_X_Window win,
                                    Ecore_X_Rectangle *rects,
                                    int num)
{
   XRectangle *rect = NULL;
   int i;

   LOGFN;
   if (num > 0)
     {
        rect = malloc(sizeof(XRectangle) * num);
        if (!rect) return;
        for (i = 0; i < num; i++)
          {
             rect[i].x = rects[i].x;
             rect[i].y = rects[i].y;
             rect[i].width = rects[i].width;
             rect[i].height = rects[i].height;
          }
     }

   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeBounding,
                           0,
                           0,
                           rect,
                           num,
                           ShapeUnion,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect) free(rect);
}

/**
 * Adds multiple rectangles to the window's current input shape (union).
 * The coordinates of the rectangles are relative to the window's origin.
 * @param   win   The window whose input shape is to be modified.
 * @param   rects An array of Ecore_X_Rectangle structures to add.
 *                Example: Ecore_X_Rectangle r[] = {{0,0,10,10}, {20,0,10,10}};
 * @param   num   The number of rectangles in the @p rects array.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_input_rectangles_add(Ecore_X_Window win,
                                          Ecore_X_Rectangle *rects,
                                          int num)
{
#ifdef ShapeInput
   XRectangle *rect = NULL;
   int i;

   LOGFN;
   if (num > 0)
     {
        rect = malloc(sizeof(XRectangle) * num);
        if (!rect) return;
        for (i = 0; i < num; i++)
          {
             rect[i].x = rects[i].x;
             rect[i].y = rects[i].y;
             rect[i].width = rects[i].width;
             rect[i].height = rects[i].height;
          }
     }

   XShapeCombineRectangles(_ecore_x_disp,
                           win,
                           ShapeInput,
                           0,
                           0,
                           rect,
                           num,
                           ShapeUnion,
                           Unsorted);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect) free(rect);
#else
   return;
   win = rects = num = 0;
#endif
}

/**
 * Retrieves the rectangles that define the window's current shape.
 * @param   win     The window whose shape rectangles are to be retrieved.
 * @param[out] num_ret Pointer to an integer where the number of returned
 *                     rectangles will be stored.
 * @return  A newly allocated array of Ecore_X_Rectangle structures
 *          representing the window's shape. The caller is responsible for
 *          freeing this array using @c free(). Returns @c NULL on failure or
 *          if the window has no shape.
 *          Example of returned array structure:
 *          Ecore_X_Rectangle rects[] = {
 *            {x1, y1, w1, h1},
 *            {x2, y2, w2, h2},
 *            ...
 *          };
 * @ingroup Ecore_X_Window_Shape
 */
EAPI Ecore_X_Rectangle *
ecore_x_window_shape_rectangles_get(Ecore_X_Window win,
                                    int *num_ret)
{
   XRectangle *rect;
   Ecore_X_Rectangle *rects = NULL;
   int i, num = 0, ord;

   LOGFN;
   rect = XShapeGetRectangles(_ecore_x_disp, win, ShapeBounding, &num, &ord);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect)
     {
        if (num < 1)
          {
             XFree(rect);
             if (num_ret) *num_ret = 0;
             return NULL;
          }
        rects = malloc(sizeof(Ecore_X_Rectangle) * num);
        if (!rects)
          {
             XFree(rect);
             if (num_ret) *num_ret = 0;
             return NULL;
          }
        for (i = 0; i < num; i++)
          {
             rects[i].x = rect[i].x;
             rects[i].y = rect[i].y;
             rects[i].width = rect[i].width;
             rects[i].height = rect[i].height;
          }
        XFree(rect);
     }
   if (num_ret) *num_ret = num;
   return rects;
}

/**
 * Retrieves the rectangles that define the window's current input shape.
 * If the Shape extension is not available, this function attempts to return
 * a single rectangle representing the window's geometry.
 * @param   win     The window whose input shape rectangles are to be retrieved.
 * @param[out] num_ret Pointer to an integer where the number of returned
 *                     rectangles will be stored.
 * @return  A newly allocated array of Ecore_X_Rectangle structures
 *          representing the window's input shape. The caller is responsible for
 *          freeing this array using @c free(). Returns @c NULL on failure or
 *          if the window has no input shape.
 *          Example of returned array structure:
 *          Ecore_X_Rectangle rects[] = {
 *            {x1, y1, w1, h1},
 *            {x2, y2, w2, h2},
 *            ...
 *          };
 * @ingroup Ecore_X_Window_Shape
 */
EAPI Ecore_X_Rectangle *
ecore_x_window_shape_input_rectangles_get(Ecore_X_Window win,
                                          int *num_ret)
{
   Ecore_X_Rectangle *rects = NULL;
#ifdef ShapeInput
   XRectangle *rect;
   int i, num = 0, ord;

   LOGFN;
   rect = XShapeGetRectangles(_ecore_x_disp, win, ShapeInput, &num, &ord);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (rect)
     {
        if (num < 1)
          {
             XFree(rect);
             if (num_ret) *num_ret = 0;
             return NULL;
          }
        rects = malloc(sizeof(Ecore_X_Rectangle) * num);
        if (!rects)
          {
             XFree(rect);
             if (num_ret) *num_ret = 0;
             return NULL;
          }
        for (i = 0; i < num; i++)
          {
             rects[i].x = rect[i].x;
             rects[i].y = rect[i].y;
             rects[i].width = rect[i].width;
             rects[i].height = rect[i].height;
          }
        XFree(rect);
     }
   if (num_ret) *num_ret = num;
   return rects;
#else
   // have to return fake shape input rect of size of window
   Window dw;
   unsigned int di;

   if (num_ret) *num_ret = 0;
   rects = malloc(sizeof(Ecore_X_Rectangle));
   if (!rects) return NULL;
   if (!XGetGeometry(_ecore_x_disp, win, &dw,
                     &(rects[0].x), &(rects[0].y),
                     &(rects[0].width), &(rects[0].height),
                     &di, &di))
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        free(rects);
        return NULL;
     }
      if (_ecore_xlib_sync) ecore_x_sync();
   if (num_ret) *num_ret = 1;
   return rects;
#endif
}

/**
 * Selects whether ShapeNotify events are generated for the given window.
 * When a window's shape is changed, a ShapeNotify event can be sent.
 * @param   win The window for which to enable or disable ShapeNotify events.
 * @param   on  If @c EINA_TRUE, enable ShapeNotify events.
 *              If @c EINA_FALSE, disable ShapeNotify events.
 * @ingroup Ecore_X_Window_Shape
 */
EAPI void
ecore_x_window_shape_events_select(Ecore_X_Window win,
                                   Eina_Bool on)
{
   LOGFN;
   if (on)
     XShapeSelectInput(_ecore_x_disp, win, ShapeNotifyMask);
   else
     XShapeSelectInput(_ecore_x_disp, win, 0);
      if (_ecore_xlib_sync) ecore_x_sync();
}

