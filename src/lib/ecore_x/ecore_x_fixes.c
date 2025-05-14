#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#include "ecore_x_private.h"
#include "Ecore_X.h"

#include <X11/Xregion.h>

static int _fixes_available; /**< Flag indicating if XFixes extension is available */
#ifdef ECORE_XFIXES
static int _fixes_major, _fixes_minor; /**< XFixes extension major and minor version */
static int _cursor_visible = 1; /**< Tracks the visibility state of the cursor */
#endif /* ifdef ECORE_XFIXES */

/**
 * @internal
 * @brief Initializes the XFixes extension interface.
 *
 * This function queries the X server for XFixes extension support and
 * its version. If available, it sets up the event type for selection
 * notifications.
 */
void
_ecore_x_fixes_init(void)
{
#ifdef ECORE_XFIXES
   _fixes_major = 3;
   _fixes_minor = 0;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);

   if (XFixesQueryVersion(_ecore_x_disp, &_fixes_major, &_fixes_minor))
     {
        _fixes_available = 1;

        ECORE_X_EVENT_FIXES_SELECTION_NOTIFY = ecore_event_type_new();
     }
   else
     _fixes_available = 0;

#else /* ifdef ECORE_XFIXES */
   _fixes_available = 0;
#endif /* ifdef ECORE_XFIXES */
}

#ifdef ECORE_XFIXES
/**
 * @internal
 * @brief Converts an array of Ecore_X_Rectangle to XRectangle.
 *
 * Allocates and populates an array of XRectangle structures from an
 * array of Ecore_X_Rectangle structures. The caller is responsible
 * for freeing the returned XRectangle array.
 *
 * @param rects Pointer to an array of Ecore_X_Rectangle structures.
 *              Example: `rects[0] = { .x=0, .y=0, .width=100, .height=100 };`
 * @param num The number of rectangles in the `rects` array.
 * @return A newly allocated array of XRectangle structures, or NULL on failure or if num is 0.
 */
static XRectangle *
_ecore_x_rectangle_ecore_to_x(Ecore_X_Rectangle *rects,
                              int num)
{
   XRectangle *xrect;
   int i;

   if (num == 0)
     return NULL;

   xrect = malloc(sizeof(XRectangle) * num);
   if (!xrect)
     return NULL;

   for (i = 0; i < num; i++)
     {
        xrect[i].x = rects[i].x;
        xrect[i].y = rects[i].y;
        xrect[i].width = rects[i].width;
        xrect[i].height = rects[i].height;
     }
   return xrect;
}

/**
 * @internal
 * @brief Converts an array of XRectangle to Ecore_X_Rectangle.
 *
 * Allocates and populates an array of Ecore_X_Rectangle structures from an
 * array of XRectangle structures. The caller is responsible for freeing
 * the returned Ecore_X_Rectangle array.
 *
 * @param xrect Pointer to an array of XRectangle structures.
 * @param num The number of rectangles in the `xrect` array.
 * @return A newly allocated array of Ecore_X_Rectangle structures, or NULL on failure or if num is 0.
 */
static Ecore_X_Rectangle *
_ecore_x_rectangle_x_to_ecore(XRectangle *xrect,
                              int num)
{
   Ecore_X_Rectangle *rects;
   int i;

   if (num == 0)
     return NULL;

   rects = malloc(sizeof(Ecore_X_Rectangle) * num);
   if (!rects)
     return NULL;

   for (i = 0; i < num; i++)
     {
        rects[i].x = xrect[i].x;
        rects[i].y = xrect[i].y;
        rects[i].width = xrect[i].width;
        rects[i].height = xrect[i].height;
     }
   return rects;
}

#endif /* ifdef ECORE_XFIXES */

/**
 * @brief Requests selection notifications for a given selection on the root window.
 *
 * This function uses the XFixes extension to request notifications when
 * properties of the specified selection change (e.g., owner, window destruction,
 * client close).
 *
 * @param selection The X atom identifying the selection (e.g., `ECORE_X_ATOM_PRIMARY`, `ECORE_X_ATOM_CLIPBOARD`).
 * @return @c EINA_TRUE if the request was successfully made, @c EINA_FALSE otherwise (e.g., XFixes not available).
 */
EAPI Eina_Bool
ecore_x_fixes_selection_notification_request(Ecore_X_Atom selection)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);

#ifdef ECORE_XFIXES
   if (_fixes_available)
     {
        XFixesSelectSelectionInput (_ecore_x_disp,
                                    DefaultRootWindow(_ecore_x_disp),
                                    selection,
                                    XFixesSetSelectionOwnerNotifyMask |
                                    XFixesSelectionWindowDestroyNotifyMask |
                                    XFixesSelectionClientCloseNotifyMask);
        return EINA_TRUE;
     }
#endif
   return EINA_FALSE;
}

/**
 * @brief Requests selection notifications for a given selection on a specific window.
 *
 * Similar to ecore_x_fixes_selection_notification_request(), but targets a specific
 * window instead of the root window.
 *
 * @param window The Ecore_X_Window on which to monitor the selection.
 * @param selection The X atom identifying the selection.
 * @return @c EINA_TRUE if the request was successfully made, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_fixes_window_selection_notification_request(Ecore_X_Window window, Ecore_X_Atom selection)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);

#ifdef ECORE_XFIXES
   if (_fixes_available)
     {
        XFixesSelectSelectionInput (_ecore_x_disp,
                                    window,
                                    selection,
                                    XFixesSetSelectionOwnerNotifyMask |
                                    XFixesSelectionWindowDestroyNotifyMask |
                                    XFixesSelectionClientCloseNotifyMask);
        return EINA_TRUE;
     }
#endif
   return EINA_FALSE;
}

/**
 * @brief Creates a new XFixes region from an array of rectangles.
 *
 * @param rects An array of Ecore_X_Rectangle structures.
 *              Each element defines a rectangle:
 *              `rects[i].x` (int): X-coordinate of the top-left corner.
 *              `rects[i].y` (int): Y-coordinate of the top-left corner.
 *              `rects[i].width` (unsigned int): Width of the rectangle.
 *              `rects[i].height` (unsigned int): Height of the rectangle.
 *              Example: `Ecore_X_Rectangle my_rects[] = {{0,0,100,50}, {10,20,30,30}};`
 * @param num The number of rectangles in the `rects` array.
 * @return A new Ecore_X_Region handle, or 0 on failure or if XFixes is not available.
 *         The returned region must be freed using ecore_x_region_free().
 */
EAPI Ecore_X_Region
ecore_x_region_new(Ecore_X_Rectangle *rects,
                   int num)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);

#ifdef ECORE_XFIXES
   Ecore_X_Region region;
   XRectangle *xrect;

   LOGFN;
   xrect = _ecore_x_rectangle_ecore_to_x(rects, num);
   region = XFixesCreateRegion(_ecore_x_disp, xrect, num);
   free(xrect);
   return region;
#else /* ifdef ECORE_XFIXES */
   return 0;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Creates a new XFixes region from a bitmap (pixmap).
 *
 * The region will correspond to the set bits in the bitmap.
 *
 * @param bitmap The Ecore_X_Pixmap (must be of depth 1).
 * @return A new Ecore_X_Region handle, or 0 on failure or if XFixes is not available.
 *         The returned region must be freed using ecore_x_region_free().
 */
EAPI Ecore_X_Region
ecore_x_region_new_from_bitmap(Ecore_X_Pixmap bitmap)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
#ifdef ECORE_XFIXES
   Ecore_X_Region region;

   LOGFN;
   region = XFixesCreateRegionFromBitmap(_ecore_x_disp, bitmap);
   if (_ecore_xlib_sync) ecore_x_sync();
   return region;
#else /* ifdef ECORE_XFIXES */
   return 0;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Creates a new XFixes region from the shape of a window.
 *
 * @param win The Ecore_X_Window from which to create the region.
 * @param type The type of region to create (e.g., `ECORE_X_SHAPE_BOUNDING` for bounding box,
 *             `ECORE_X_SHAPE_CLIP` for clip region). This corresponds to X Shape kinds.
 * @return A new Ecore_X_Region handle, or 0 on failure or if XFixes is not available.
 *         The returned region must be freed using ecore_x_region_free().
 */
EAPI Ecore_X_Region
ecore_x_region_new_from_window(Ecore_X_Window win,
                               Ecore_X_Region_Type type)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
#ifdef ECORE_XFIXES
   Ecore_X_Region region;

   LOGFN;
   region = XFixesCreateRegionFromWindow(_ecore_x_disp, win, type);
   if (_ecore_xlib_sync) ecore_x_sync();
   return region;
#else /* ifdef ECORE_XFIXES */
   return 0;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Creates a new XFixes region from the clip region of a Graphics Context (GC).
 *
 * @param gc The Ecore_X_GC from which to extract the clip region.
 * @return A new Ecore_X_Region handle, or 0 on failure or if XFixes is not available.
 *         The returned region must be freed using ecore_x_region_free().
 */
EAPI Ecore_X_Region
ecore_x_region_new_from_gc(Ecore_X_GC gc)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
#ifdef ECORE_XFIXES
   Ecore_X_Region region;

   LOGFN;
   region = XFixesCreateRegionFromGC(_ecore_x_disp, gc);
   if (_ecore_xlib_sync) ecore_x_sync();
   return region;
#else /* ifdef ECORE_XFIXES */
   return 0;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Creates a new XFixes region from the clip region of a Picture.
 *
 * This is typically used with XRender.
 *
 * @param picture The Ecore_X_Picture from which to extract the clip region.
 * @return A new Ecore_X_Region handle, or 0 on failure or if XFixes is not available.
 *         The returned region must be freed using ecore_x_region_free().
 */
EAPI Ecore_X_Region
ecore_x_region_new_from_picture(Ecore_X_Picture picture)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
#ifdef ECORE_XFIXES
   Ecore_X_Region region;

   LOGFN;
   region = XFixesCreateRegionFromPicture(_ecore_x_disp, picture);
   if (_ecore_xlib_sync) ecore_x_sync();
   return region;
#else /* ifdef ECORE_XFIXES */
   return 0;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Frees an XFixes region.
 *
 * @param region The Ecore_X_Region to free.
 */
EAPI void
ecore_x_region_free(Ecore_X_Region region)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesDestroyRegion(_ecore_x_disp, region);
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Sets (replaces) the content of an existing XFixes region with a new set of rectangles.
 *
 * @param region The Ecore_X_Region to modify.
 * @param rects An array of Ecore_X_Rectangle structures defining the new region.
 *              See ecore_x_region_new() for `rects` structure.
 * @param num The number of rectangles in the `rects` array.
 */
EAPI void
ecore_x_region_set(Ecore_X_Region region,
                   Ecore_X_Rectangle *rects,
                   int num)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   XRectangle *xrect = _ecore_x_rectangle_ecore_to_x(rects, num);
   LOGFN;
   XFixesSetRegion(_ecore_x_disp, region, xrect, num);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Copies one XFixes region to another.
 *
 * The content of `dest` region will be replaced by the content of `source` region.
 *
 * @param dest The destination Ecore_X_Region.
 * @param source The source Ecore_X_Region.
 */
EAPI void
ecore_x_region_copy(Ecore_X_Region dest,
                    Ecore_X_Region source)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesCopyRegion(_ecore_x_disp, dest, source);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Computes the union of two XFixes regions.
 *
 * The `dest` region will contain all areas present in `source1` or `source2` (or both).
 * `dest` can be the same as `source1` or `source2`.
 *
 * @param dest The Ecore_X_Region where the result is stored.
 * @param source1 The first Ecore_X_Region.
 * @param source2 The second Ecore_X_Region.
 */
EAPI void
ecore_x_region_combine(Ecore_X_Region dest,
                       Ecore_X_Region source1,
                       Ecore_X_Region source2)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesUnionRegion(_ecore_x_disp, dest, source1, source2);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Computes the intersection of two XFixes regions.
 *
 * The `dest` region will contain only the areas present in both `source1` and `source2`.
 * `dest` can be the same as `source1` or `source2`.
 *
 * @param dest The Ecore_X_Region where the result is stored.
 * @param source1 The first Ecore_X_Region.
 * @param source2 The second Ecore_X_Region.
 */
EAPI void
ecore_x_region_intersect(Ecore_X_Region dest,
                         Ecore_X_Region source1,
                         Ecore_X_Region source2)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesIntersectRegion(_ecore_x_disp, dest, source1, source2);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Subtracts one XFixes region from another.
 *
 * The `dest` region will contain areas present in `source1` but not in `source2`.
 * `dest` can be the same as `source1` or `source2`.
 *
 * @param dest The Ecore_X_Region where the result (source1 - source2) is stored.
 * @param source1 The Ecore_X_Region from which to subtract.
 * @param source2 The Ecore_X_Region to subtract.
 */
EAPI void
ecore_x_region_subtract(Ecore_X_Region dest,
                        Ecore_X_Region source1,
                        Ecore_X_Region source2)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesSubtractRegion(_ecore_x_disp, dest, source1, source2);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Inverts an XFixes region within specified bounds.
 *
 * The `dest` region will contain areas within the `bounds` rectangle that are
 * not in the `source` region.
 *
 * @param dest The Ecore_X_Region where the inverted region is stored.
 * @param bounds A pointer to an Ecore_X_Rectangle defining the bounding box for the inversion.
 *               Example: `Ecore_X_Rectangle b = {0,0,800,600};`
 * @param source The Ecore_X_Region to invert.
 */
EAPI void
ecore_x_region_invert(Ecore_X_Region dest,
                      Ecore_X_Rectangle *bounds,
                      Ecore_X_Region source)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   XRectangle *xbound;
   int num = 1;

   LOGFN;
   xbound = _ecore_x_rectangle_ecore_to_x(bounds, num);

   XFixesInvertRegion(_ecore_x_disp, dest, xbound, source);
   if (_ecore_xlib_sync) ecore_x_sync();
   free(xbound);
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Translates (moves) an XFixes region.
 *
 * All coordinates within the region are offset by (dx, dy).
 *
 * @param region The Ecore_X_Region to translate.
 * @param dx The horizontal offset.
 * @param dy The vertical offset.
 */
EAPI void
ecore_x_region_translate(Ecore_X_Region region,
                         int dx,
                         int dy)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesTranslateRegion(_ecore_x_disp, region, dx, dy);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Computes the smallest rectangle enclosing an XFixes region (extents).
 *
 * The `dest` region will be set to a single rectangle representing the
 * bounding box of the `source` region.
 *
 * @param dest The Ecore_X_Region where the extents rectangle is stored.
 * @param source The Ecore_X_Region whose extents are to be computed.
 */
EAPI void
ecore_x_region_extents(Ecore_X_Region dest,
                       Ecore_X_Region source)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesRegionExtents(_ecore_x_disp, dest, source);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Fetches the rectangles and bounding box of an XFixes region.
 *
 * Retrieves the constituent rectangles of a region and its overall bounding box.
 * The caller is responsible for freeing the returned array of Ecore_X_Rectangle
 * using `free()`.
 *
 * @param region The Ecore_X_Region to fetch.
 * @param[out] num Pointer to an integer where the number of rectangles will be stored.
 * @param[out] bounds Pointer to an Ecore_X_Rectangle where the bounding box of the region will be stored.
 *                    Example of `bounds` structure after call:
 *                    `bounds->x`, `bounds->y`, `bounds->width`, `bounds->height`.
 * @return A newly allocated array of Ecore_X_Rectangle structures representing the region,
 *         or NULL on failure. The structure of each element in the array is:
 *         `rects[i].x`, `rects[i].y`, `rects[i].width`, `rects[i].height`.
 */
EAPI Ecore_X_Rectangle *
ecore_x_region_fetch(Ecore_X_Region region,
                     int *num,
                     Ecore_X_Rectangle *bounds)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
#ifdef ECORE_XFIXES
   Ecore_X_Rectangle *rects;
   XRectangle *xrect, xbound;

   LOGFN;
   xrect = XFixesFetchRegionAndBounds(_ecore_x_disp, region, num, &xbound);
   if (_ecore_xlib_sync) ecore_x_sync();
   rects = _ecore_x_rectangle_x_to_ecore(xrect, *num);
   (*bounds).x = xbound.x;
   (*bounds).y = xbound.y;
   (*bounds).width = xbound.width;
   (*bounds).height = xbound.height;
   XFree(xrect);
   return rects;
#else /* ifdef ECORE_XFIXES */
   return NULL;
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Expands or shrinks an XFixes region.
 *
 * The `dest` region is formed by taking the `source` region and expanding
 * (if values are positive) or shrinking (if values are negative, though
 * XFixesExpandRegion expects unsigned, so shrinking is typically done via
 * other means or careful construction) its boundaries.
 *
 * @param dest The Ecore_X_Region where the result is stored.
 * @param source The Ecore_X_Region to expand.
 * @param left Amount to expand to the left.
 * @param right Amount to expand to the right.
 * @param top Amount to expand upwards.
 * @param bottom Amount to expand downwards.
 */
EAPI void
ecore_x_region_expand(Ecore_X_Region dest,
                      Ecore_X_Region source,
                      unsigned int left,
                      unsigned int right,
                      unsigned int top,
                      unsigned int bottom)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesExpandRegion(_ecore_x_disp, dest, source, left, right, top, bottom);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Sets the clip mask of a Graphics Context (GC) to an XFixes region.
 *
 * @param region The Ecore_X_Region to use as the clip mask.
 * @param gc The Ecore_X_GC whose clip mask is to be set.
 * @param x_origin The X offset for the region relative to the GC's origin.
 * @param y_origin The Y offset for the region relative to the GC's origin.
 */
EAPI void
ecore_x_region_gc_clip_set(Ecore_X_Region region,
                           Ecore_X_GC gc,
                           int x_origin,
                           int y_origin)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesSetGCClipRegion(_ecore_x_disp, gc, x_origin, y_origin, region);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Sets the shape of a window using an XFixes region.
 *
 * This function uses the X Shape extension, via XFixes, to define the
 * visible shape of a window.
 *
 * @param region The Ecore_X_Region defining the new shape.
 * @param win The Ecore_X_Window whose shape is to be set.
 * @param type The kind of shape to set (e.g., `ECORE_X_SHAPE_BOUNDING`, `ECORE_X_SHAPE_CLIP`).
 *             This corresponds to X Shape kinds (ShapeBounding, ShapeClip).
 * @param x_offset The X offset of the region relative to the window's origin.
 * @param y_offset The Y offset of the region relative to the window's origin.
 */
EAPI void
ecore_x_region_window_shape_set(Ecore_X_Region region,
                                Ecore_X_Window win,
                                Ecore_X_Shape_Type type,
                                int x_offset,
                                int y_offset)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesSetWindowShapeRegion(_ecore_x_disp,
                              win,
                              type,
                              x_offset,
                              y_offset,
                              region);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Sets the clip region of an XRender Picture.
 *
 * @param region The Ecore_X_Region to use as the clip mask.
 * @param picture The Ecore_X_Picture whose clip region is to be set.
 * @param x_origin The X offset for the region relative to the Picture's origin.
 * @param y_origin The Y offset for the region relative to the Picture's origin.
 */
EAPI void
ecore_x_region_picture_clip_set(Ecore_X_Region region,
                                Ecore_X_Picture picture,
                                int x_origin,
                                int y_origin)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   LOGFN;
   XFixesSetPictureClipRegion(_ecore_x_disp,
                              picture,
                              x_origin,
                              y_origin,
                              region);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XFIXES */
}

/**
 * @brief Shows the mouse cursor.
 *
 * Uses XFixesShowCursor to make the cursor visible on the root window
 * if it was previously hidden by ecore_x_cursor_hide().
 * This function tracks visibility state to avoid redundant X calls.
 */
EAPI void
ecore_x_cursor_show(void)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   if (!_cursor_visible)
     {
        XFixesShowCursor(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp));
        XFlush(_ecore_x_disp);
        _cursor_visible = 1;
     }
#endif  /* ifdef ECORE_XFIXES */
}

/**
 * @brief Hides the mouse cursor.
 *
 * Uses XFixesHideCursor to make the cursor invisible on the root window.
 * This function tracks visibility state to avoid redundant X calls.
 */
EAPI void
ecore_x_cursor_hide(void)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
#ifdef ECORE_XFIXES
   if (_cursor_visible)
     {
        XFixesHideCursor(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp));
        XFlush(_ecore_x_disp);
        _cursor_visible = 0;
     }
#endif  /* ifdef ECORE_XFIXES */
}

/**
 * @brief Sets pointer barriers on the root window to confine the mouse pointer to a set of screen areas.
 *
 * This function creates XFixes pointer barriers along the boundaries of the
 * specified screen rectangles. The effect is to prevent the mouse pointer
 * from leaving the combined area of these screens.
 *
 * If the mouse pointer is currently outside all specified screen areas, it will
 * be warped to the center of the closest screen.
 *
 * Old barriers are cleared before new ones are set.
 *
 * @param screens An array of Ecore_X_Rectangle structures defining the screen areas.
 *                Each element defines a screen:
 *                `screens[i].x` (int): X-coordinate of the screen.
 *                `screens[i].y` (int): Y-coordinate of the screen.
 *                `screens[i].width` (unsigned int): Width of the screen.
 *                `screens[i].height` (unsigned int): Height of the screen.
 *                Example: `Ecore_X_Rectangle my_screens[] = {{0,0,1920,1080}, {1920,0,1024,768}};`
 * @param num The number of screen rectangles in the `screens` array. If 0 or `screens` is NULL,
 *            existing barriers are cleared and no new ones are created.
 */
EAPI void
ecore_x_root_screen_barriers_set(Ecore_X_Rectangle *screens, int num)
{
#ifdef ECORE_XFIXES
   static PointerBarrier *bar = NULL;
   static int bar_num = 0;
   static int bar_alloc = 0;
   Region reg, reg2, reg3;
   Window rwin, cwin;
   int rx, ry, wx, wy;
   int i;
   int closest_dist, dist;
   int sx, sy, dx, dy;
   unsigned int mask;
   Eina_Bool inside = EINA_FALSE;
   Ecore_X_Rectangle *closest_screen = NULL;

   // clear out old root screen barriers....
   if (bar)
     {
        for (i = 0; i < bar_num; i++)
          {
             XFixesDestroyPointerBarrier(_ecore_x_disp, bar[i]);
          }
        free(bar);
     }
   // ensure mouse pointer is insude the new set of screens if it is not
   // inside them right now
   XQueryPointer(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp),
                 &rwin, &cwin, &rx, &ry, &wx, &wy, &mask);
   for (i = 0; i < num; i++)
     {
        if ((rx >= screens[i].x) &&
            (rx < (screens[i].x + (int)screens[i].width)) &&
            (ry >= screens[i].y) &&
            (ry < (screens[i].y + (int)screens[i].height)))
          {
             inside = EINA_TRUE;
             break;
          }
        if (!closest_screen) closest_screen = &(screens[i]);
        else
          {
             // screen center
             sx = closest_screen->x + (closest_screen->width / 2);
             sy = closest_screen->y + (closest_screen->height / 2);
             dx = rx - sx;
             dy = ry - sy;
             // square dist to center
             closest_dist = ((dx * dx) + (dy * dy));
             // screen center
             sx = screens[i].x + (screens[i].width / 2);
             sy = screens[i].y + (screens[i].height / 2);
             dx = rx - sx;
             dy = ry - sy;
             // square dist to center
             dist = ((dx * dx) + (dy * dy));
             // if closer than previous closest, then this screen is closer
             if (dist < closest_dist) closest_screen = &(screens[i]);
          }
     }
   // if the pointer is not inside oneof the new screen areas then
   // move it to the center of the closest one to ensure it doesn't get
   // stuck outside
   if ((!inside) && (closest_screen))
     {
        // screen center
        sx = closest_screen->x + (closest_screen->width / 2);
        sy = closest_screen->y + (closest_screen->height / 2);
        // move pointer there
        XWarpPointer(_ecore_x_disp, None,
                     DefaultRootWindow(_ecore_x_disp),
                     0, 0, 0, 0, sx, sy);
     }

   bar = NULL;
   bar_num = 0;
   bar_alloc = 0;
   if ((!screens) || (num <= 0)) return;

   // new region
   reg = XCreateRegion();
   // add each screen rect + 1 pixel around it to reg
   for (i = 0; i < num; i++)
     {
        XRectangle xrect;

        reg2 = XCreateRegion();
        xrect.x      = screens[i].x - 1;
        xrect.y      = screens[i].y - 1;
        xrect.width  = screens[i].width + 2;
        xrect.height = screens[i].height + 2;
        XUnionRectWithRegion(&xrect, reg, reg2);
        XDestroyRegion(reg);
        reg = reg2;
     }
   // del the content of each screen from the above
   for (i = 0; i < num; i++)
     {
        XRectangle xrect;

        // create just a rect with the screen in it
        reg2 = XCreateRegion();
        reg3 = XCreateRegion();
        xrect.x      = screens[i].x;
        xrect.y      = screens[i].y;
        xrect.width  = screens[i].width;
        xrect.height = screens[i].height;
        XUnionRectWithRegion(&xrect, reg3, reg2);
        XDestroyRegion(reg3);

        // now subtract it
        reg3 = XCreateRegion();
        XSubtractRegion(reg, reg2, reg3);
        XDestroyRegion(reg);
        XDestroyRegion(reg2);
        reg = reg3;
     }
   if (reg)
     {
        // walk rects and create barriers
        for (i = 0; i < reg->numRects; i++)
          {
             int x1, y1, x2, y2;

             bar_num++;
             if (bar_num > bar_alloc)
               {
                  bar_alloc += 32;
                  PointerBarrier *t = realloc(bar, bar_alloc * sizeof(PointerBarrier));
                  if (!t)
                    {
                       bar_num--;
                       XDestroyRegion(reg);
                       return;
                    }
                  bar = t;
               }
             x1 = reg->rects[i].x1;
             y1 = reg->rects[i].y1;
             x2 = reg->rects[i].x2 - 1;
             y2 = reg->rects[i].y2 - 1;
             bar[bar_num - 1] =
               XFixesCreatePointerBarrier(_ecore_x_disp,
                                          DefaultRootWindow(_ecore_x_disp),
                                          x1, y1, x2, y2, 0, 0, NULL);
          }
        XDestroyRegion(reg);
     }
#endif
}
