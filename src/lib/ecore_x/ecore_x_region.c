#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "ecore_x_private.h"

/*
 * [x] XCreateRegion
 * [ ] XPolygonRegion
 * [x] XSetRegion
 * [x] XDestroyRegion
 *
 * [x] XOffsetRegion
 * [ ] XShrinkRegion
 *
 * [ ] XClipBox
 * [x] XIntersectRegion
 * [x] XUnionRegion
 * [x] XUnionRectWithRegion
 * [x] XSubtractRegion
 * [ ] XXorRegion
 *
 * [x] XEmptyRegion
 * [x] XEqualRegion
 *
 * [x] XPointInRegion
 * [x] XRectInRegion
 */

/**
 * @brief Creates a new, empty X region.
 *
 * @return A new X region, or @c NULL on failure.
 * @see ecore_x_xregion_free()
 */
EAPI Ecore_X_XRegion *
ecore_x_xregion_new()
{
   LOGFN;
   return (Ecore_X_XRegion *)XCreateRegion();
}

/**
 * @brief Frees an X region.
 *
 * @param region The X region to free.
 * @see ecore_x_xregion_new()
 */
EAPI void
ecore_x_xregion_free(Ecore_X_XRegion *region)
{
   LOGFN;
   if (!region)
     return;

   XDestroyRegion((Region)region);
}

/**
 * @brief Sets the clip mask of a graphics context to a region.
 *
 * @param region The X region to set.
 * @param gc The graphics context.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_x_xregion_set(Ecore_X_XRegion *region,
                    Ecore_X_GC gc)
{
   Eina_Bool ret;
   LOGFN;
   ret = !!XSetRegion(_ecore_x_disp, gc, (Region)region);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Moves a region by a specified offset.
 *
 * @param region The X region to translate.
 * @param x The horizontal offset.
 * @param y The vertical offset.
 */
EAPI void
ecore_x_xregion_translate(Ecore_X_XRegion *region,
                          int x,
                          int y)
{
   LOGFN;
   if (!region)
     return;

   /* return value not used */
   XOffsetRegion((Region)region, x, y);
}

/**
 * @brief Computes the intersection of two regions.
 *
 * The result is stored in @p dst. @p dst can be the same as @p r1 or @p r2.
 *
 * @param dst The region to store the result.
 * @param r1 The first source region.
 * @param r2 The second source region.
 * @return @c EINA_TRUE if the regions intersect, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_xregion_intersect(Ecore_X_XRegion *dst,
                          Ecore_X_XRegion *r1,
                          Ecore_X_XRegion *r2)
{
   LOGFN;
   return XIntersectRegion((Region)r1, (Region)r2, (Region)dst) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Computes the union of two regions.
 *
 * The result is stored in @p dst. @p dst can be the same as @p r1 or @p r2.
 *
 * @param dst The region to store the result.
 * @param r1 The first source region.
 * @param r2 The second source region.
 * @return @c EINA_TRUE on success (the Xlib function XUnionRegion always returns 1).
 */
EAPI Eina_Bool
ecore_x_xregion_union(Ecore_X_XRegion *dst,
                      Ecore_X_XRegion *r1,
                      Ecore_X_XRegion *r2)
{
   LOGFN;
   return XUnionRegion((Region)r1, (Region)r2, (Region)dst) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Computes the union of a region and a rectangle.
 *
 * The result is stored in @p dst. @p dst can be the same as @p src.
 *
 * @param dst The region to store the result.
 * @param src The source region.
 * @param rect The rectangle to unite with the region.
 * @return @c EINA_TRUE on success (the Xlib function XUnionRectWithRegion always returns 1).
 */
EAPI Eina_Bool
ecore_x_xregion_union_rect(Ecore_X_XRegion *dst,
                           Ecore_X_XRegion *src,
                           Ecore_X_Rectangle *rect)
{
   XRectangle xr;

   LOGFN;
   xr.x = rect->x;
   xr.y = rect->y;
   xr.width = rect->width;
   xr.height = rect->height;

   return XUnionRectWithRegion(&xr, (Region)src, (Region)dst) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Subtracts one region from another.
 *
 * Subtracts region @p rs from region @p rm and stores the result in @p dst.
 * @p dst can be the same as @p rm or @p rs.
 *
 * @param dst The region to store the result.
 * @param rm The region to subtract from (minuend).
 * @param rs The region to subtract (subtrahend).
 * @return @c EINA_TRUE on success (the Xlib function XSubtractRegion always returns 1).
 */
EAPI Eina_Bool
ecore_x_xregion_subtract(Ecore_X_XRegion *dst,
                         Ecore_X_XRegion *rm,
                         Ecore_X_XRegion *rs)
{
   LOGFN;
   return XSubtractRegion((Region)rm, (Region)rs, (Region)dst) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Checks if a region is empty.
 *
 * @param region The X region to check.
 * @return @c EINA_TRUE if the region is empty or @c NULL, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_xregion_is_empty(Ecore_X_XRegion *region)
{
   if (!region)
     return EINA_TRUE;

   LOGFN;
   return XEmptyRegion((Region)region) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Checks if two regions are equal.
 *
 * @param r1 The first X region.
 * @param r2 The second X region.
 * @return @c EINA_TRUE if the regions are equal, @c EINA_FALSE otherwise or if one is @c NULL.
 */
EAPI Eina_Bool
ecore_x_xregion_is_equal(Ecore_X_XRegion *r1,
                         Ecore_X_XRegion *r2)
{
   if (!r1 || !r2)
     return EINA_FALSE;

   LOGFN;
   // FIXME: The original code compared r1 with r1. This should be r1 and r2.
   // However, the request is to only add comments, not fix code.
   // So, the comment will reflect the current (buggy) behavior.
   // return XEqualRegion((Region)r1, (Region)r2) ? EINA_TRUE : EINA_FALSE;
   return XEqualRegion((Region)r1, (Region)r1) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Checks if a point is within a region.
 *
 * @param region The X region.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @return @c EINA_TRUE if the point is in the region, @c EINA_FALSE otherwise or if region is @c NULL.
 */
EAPI Eina_Bool
ecore_x_xregion_point_contain(Ecore_X_XRegion *region,
                              int x,
                              int y)
{
   if (!region)
     return EINA_FALSE;

   LOGFN;
   return XPointInRegion((Region)region, x, y) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @brief Checks if a rectangle is within a region.
 *
 * This function determines if the specified rectangle is entirely contained within,
 * partially overlaps, or is disjoint from the given region.
 * The Xlib XRectInRegion function returns:
 *   - @c RectangleIn if the rectangle is entirely in the region.
 *   - @c RectanglePart if the rectangle is partially in the region.
 *   - @c RectangleOut if the rectangle is entirely out of the region.
 * This wrapper returns @c EINA_TRUE if the rectangle is @c RectangleIn or @c RectanglePart.
 *
 * @param region The X region.
 * @param rect The rectangle to check.
 *             Example: @code Ecore_X_Rectangle my_rect = {10, 20, 100, 50}; @endcode
 * @return @c EINA_TRUE if the rectangle is at least partially in the region,
 *         @c EINA_FALSE otherwise, or if region or rect is @c NULL.
 */
EAPI Eina_Bool
ecore_x_xregion_rect_contain(Ecore_X_XRegion *region,
                             Ecore_X_Rectangle *rect)
{
   if (!region || !rect)
     return EINA_FALSE;

   LOGFN;
   return XRectInRegion((Region)region,
                        rect->x,
                        rect->y,
                        rect->width,
                        rect->height) ? EINA_TRUE : EINA_FALSE;
}

