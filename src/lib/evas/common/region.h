#ifndef _REGION_H_
#define _REGION_H_

/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Id: pixman.h,v 1.21 2005/06/25 01:21:16 jrmuizel Exp $ */

/* pixregion.h */

////////////////////////////////////////////////////////////////////////////
#include "Eina.h"
typedef struct _Region Region;
typedef struct _Box    Box;

typedef enum _Region_State
{
   REGION_STATE_OUT,
   REGION_STATE_IN,
   REGION_STATE_PARTIAL
} Region_State;

struct _Box
{
   int x1, y1, x2, y2;
};

/* creation/destruction */

/**
 * @brief Creates a new empty region.
 * @param w The width of the region's coordinate space.
 * @param h The height of the region's coordinate space.
 * @return A new region object, or NULL on allocation failure.
 * The new region is initially empty, defined by an empty box {0, 0, 0, 0}.
 */
Region       *region_new          (int w, int h);
/**
 * @brief Frees the memory associated with a region.
 * @param region The region to free.
 */
void          region_free         (Region *region);
/**
 * @brief Gets the coordinate space dimensions of the region.
 * @param region The region to query.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
void          region_size_get     (Region *region, int *w, int *h);

/**
 * @brief Moves a region by a given offset.
 * @param region The region to move.
 * @param x The horizontal offset.
 * @param y The vertical offset.
 * This function translates all rectangles within the region by (x, y).
 */
void          region_move         (Region *region, int x, int y);
/**
 * @brief Copies one region to another.
 * @param dest The destination region.
 * @param source The source region.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The destination region will be a complete copy of the source.
 */
Eina_Bool     region_copy         (Region *dest, Region *source);
/**
 * @brief Computes the intersection of two regions.
 * @param dest The destination region, which is also the first operand.
 * @param source The second source region.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The result is stored in 'dest'. 'dest' is modified in place.
 * The resulting region contains only the areas present in both 'dest' and 'source'.
 */
Eina_Bool     region_intersect    (Region *dest, Region *source);
/**
 * @brief Computes the union of two regions.
 * @param dest The destination region, which is also the first operand.
 * @param source The second source region.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The result is stored in 'dest'. 'dest' is modified in place.
 * The resulting region contains areas present in either 'dest' or 'source'.
 * This is also known as a union operation.
 */
Eina_Bool     region_add          (Region *dest, Region *source);
/**
 * @brief Adds a rectangle to a region.
 * @param dest The region to add the rectangle to.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The rectangle is clipped to the region's bounds before being added.
 */
Eina_Bool     region_rect_add     (Region *dest, int x, int y, unsigned int w, unsigned int h);
/**
 * @brief Subtracts one region from another.
 * @param dest The destination region, from which 'source' is subtracted.
 * @param source The region to subtract.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The result is stored in 'dest'. 'dest' is modified in place.
 * The resulting region contains areas of 'dest' that are not in 'source'.
 * This is also known as a subtraction or difference operation.
 */
Eina_Bool     region_del          (Region *dest, Region *source);
/**
 * @brief Deletes a rectangle from a region.
 * @param dest The region to delete the rectangle from.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * The rectangle is clipped to the region's bounds before being subtracted.
 */
Eina_Bool     region_rect_del     (Region *dest, int x, int y, unsigned int w, unsigned int h);

/**
 * @brief Gets the number of rectangles that compose the region.
 * @param region The region.
 * @return The number of rectangles.
 * A region is represented as a list of non-overlapping rectangles.
 */
int           region_rects_num    (Region *region);
/**
 * @brief Gets the array of rectangles that compose the region.
 * @param region The region.
 * @return A pointer to the array of rectangles (Box).
 * The array is sorted by y1, then x1.
 * The rectangles are guaranteed to be non-overlapping.
 *
 * Example of array structure:
 * [
 *   {x1: 10, y1: 10, x2: 20, y2: 20}, // First rectangle
 *   {x1: 30, y1: 10, x2: 40, y2: 20}, // Second rectangle in same band
 *   {x1: 10, y1: 30, x2: 50, y2: 40}  // Rectangle in a new band
 * ]
 */
Box          *region_rects        (Region *region);

/**
 * @brief Checks if a point is inside a region.
 * @param region The region.
 * @param x The x-coordinate of the point.
 * @param y The y-coordinate of the point.
 * @param[out] bx If not NULL and the point is inside, this will be filled
 *                with the bounding box of the rectangle containing the point.
 * @return EINA_TRUE if the point is in the region, EINA_FALSE otherwise.
 */
Eina_Bool     region_point_inside (Region *region, int x, int y, Box *bx);
/**
 * @brief Checks if a rectangle is inside, outside, or partially inside a region.
 * @param region The region.
 * @param bx The rectangle to check.
 * @return A Region_State value:
 *         - REGION_STATE_IN: The rectangle is completely inside the region.
 *         - REGION_STATE_OUT: The rectangle is completely outside the region.
 *         - REGION_STATE_PARTIAL: The rectangle partially overlaps with the region.
 */
Region_State  region_rect_inside  (Region *region, Box *bx);
/**
 * @brief Checks if a region is not empty.
 * @param region The region to check.
 * @return EINA_TRUE if the region is not empty, EINA_FALSE otherwise.
 */
Eina_Bool     region_exists       (Region *region);
/**
 * @brief Gets the bounding box of the entire region.
 * @param region The region.
 * @return A pointer to the region's bounding box.
 * This is the smallest rectangle that contains all rectangles in the region.
 */
Box          *region_bounds       (Region *region);

/**
 * @brief Appends rectangles from one region to another without checking for overlaps.
 * @param dest The destination region.
 * @param region The source region.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @warning This function assumes that the rectangles from 'region' are
 * correctly positioned relative to the existing rectangles in 'dest' to
 * maintain the sorted, non-overlapping property of a valid region.
 * Use with caution. It is faster than region_add but can lead to an
 * invalid region if used incorrectly.
 */
Eina_Bool     region_append       (Region *dest, Region *region);
/**
 * @brief Validates a region, merging overlapping rectangles.
 * @param region The region to validate.
 * @param[out] overlap_ret Set to EINA_TRUE if any overlapping rectangles were merged.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * This function sorts the rectangles and merges any that overlap or are adjacent,
 * ensuring the region is represented by a minimal set of non-overlapping rectangles.
 * This can be useful after operations like region_append that do not guarantee validity.
 */
Eina_Bool     region_validate     (Region *region, Eina_Bool *overlap_ret);
/**
 * @brief Resets a region to a single rectangle.
 * @param region The region to reset.
 * @param bx The box to set as the new region content.
 * Any existing data in the region is discarded.
 */
void          region_reset        (Region *region, Box *bx);
/**
 * @brief Empties a region.
 * @param region The region to empty.
 * This makes the region contain no rectangles.
 */
void          region_empty        (Region *region);

#endif
