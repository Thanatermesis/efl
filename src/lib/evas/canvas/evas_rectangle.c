#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Calculates the symmetric difference of two rectangles.
 *
 * This function computes the regions that are part of either the first rectangle
 * or the second rectangle, but not part of both. The resulting rectangles
 * representing this difference are added to the provided Eina_Array.
 *
 * If the two input rectangles do not intersect, both rectangles are added
 * to the `rects` array as they form the difference.
 *
 * If they do intersect, the space covered by the union of the two rectangles
 * is subdivided. Each subdivision is then tested: if it lies within exactly
 * one of the original rectangles (i.e., in R1 XOR R2), it is added to the
 * `rects` array.
 *
 * The `rects` array will be populated with `Evas_Rectangle`-like structures,
 * where each element effectively represents a rectangle defined by x, y, w, h.
 * For example, if `evas_add_rect(rects, r_x, r_y, r_w, r_h)` is called,
 * a conceptual `Evas_Rectangle { .x=r_x, .y=r_y, .w=r_w, .h=r_h }` is added.
 *
 * @param rects An initialized Eina_Array to which the difference rectangles will be added.
 *              Example: Eina_Array *my_rects = eina_array_new(10);
 * @param x The x-coordinate of the first rectangle.
 * @param y The y-coordinate of the first rectangle.
 * @param w The width of the first rectangle.
 * @param h The height of the first rectangle.
 * @param xx The x-coordinate of the second rectangle.
 * @param yy The y-coordinate of the second rectangle.
 * @param ww The width of the second rectangle.
 * @param hh The height of the second rectangle.
 */
void
evas_rects_return_difference_rects(Eina_Array *rects, int x, int y, int w, int h, int xx, int yy, int ww, int hh)
{
   // If the rectangles do not intersect, the difference is simply both rectangles.
   if (!RECTS_INTERSECT(x, y, w, h, xx, yy, ww, hh))
     {
	evas_add_rect(rects, x, y, w, h);
	evas_add_rect(rects, xx, yy, ww, hh);
     }
   else
     {
	// pt_x and pt_y arrays will store the sorted unique x and y coordinates
	// that define the boundaries of the sub-rectangles.
	// There are at most 4 distinct x-coordinates (x, x+w, xx, xx+ww)
	// and 4 distinct y-coordinates (y, y+h, yy, yy+hh).
	int pt_x[4], pt_y[4], i, j;

	// Sort and store the x-coordinates
	if (x < xx)
	  {
	     pt_x[0] = x;
	     pt_x[1] = xx;
	  }
	else
	  {
	     pt_x[0] = xx;
	     pt_x[1] = x;
	  }
	if ((x + w) < (xx + ww))
	  {
	     pt_x[2] = x + w;
	     pt_x[3] = xx + ww;
	  }
	else
	  {
	     pt_x[2] = xx + ww;
	     pt_x[3] = x + w;
	  }
	// Sort and store the y-coordinates
	if (y < yy)
	  {
	     pt_y[0] = y;
	     pt_y[1] = yy;
	  }
	else
	  {
	     pt_y[0] = yy;
	     pt_y[1] = y;
	  }
	if ((y + h) < (yy + hh))
	  {
	     pt_y[2] = y + h;
	     pt_y[3] = yy + hh;
	  }
	else
	  {
	     pt_y[2] = yy + hh;
	     pt_y[3] = y + h;
	  }
	// Iterate over the grid formed by pt_x and pt_y coordinates.
	// This creates up to 3x3 = 9 sub-rectangles.
	for (j = 0; j < 3; j++) // Iterate through y-intervals
	  {
	     for (i = 0; i < 3; i++) // Iterate through x-intervals
	       {
		  int intsec1, intsec2;
		  int tx, ty, tw, th; // Temporary rectangle coordinates and dimensions

		  // Define the current sub-rectangle
		  tx = pt_x[i];
		  ty = pt_y[j];
		  tw = pt_x[i + 1] - pt_x[i];
		  th = pt_y[j + 1] - pt_y[j];

		  // Check if the sub-rectangle has a valid positive area
		  if (tw <= 0 || th <= 0) continue;

		  // Check if the current sub-rectangle intersects with the first input rectangle
		  intsec1 = (RECTS_INTERSECT(tx, ty, tw, th, x, y, w, h));
		  // Check if the current sub-rectangle intersects with the second input rectangle
		  intsec2 = (RECTS_INTERSECT(tx, ty, tw, th, xx, yy, ww, hh));

		  // If the sub-rectangle is in one of the input rectangles but not both (XOR),
		  // it's part of the symmetric difference.
		  if (intsec1 ^ intsec2)
		    {
		       evas_add_rect(rects, tx, ty, tw, th);
		    }
	       }
	  }
/*	if (tmp.count > 0) */
/*	  { */
/*	     unsigned int i; */

/*	     for (i = 0; i < tmp.count; ++i) */
/*	       { */
/*		  if ((tmp.array[i].w > 0) && (tmp.array[i].h > 0)) */
/*		    { */
/*		       int intsec1, intsec2; */

/*		       intsec1 = (RECTS_INTERSECT(tmp.array[i].x, tmp.array[i].y, tmp.array[i].w, tmp.array[i].h, x, y, w, h)); */
/*		       intsec2 = (RECTS_INTERSECT(tmp.array[i].x, tmp.array[i].y, tmp.array[i].w, tmp.array[i].h, xx, yy, ww, hh)); */
/*		       if (intsec1 ^ intsec2) */
/*			 { */
/*			    evas_add_rect(rects, tmp.array[i].x, tmp.array[i].y, tmp.array[i].w, tmp.array[i].h); */
/*			 } */
/*		    } */
/*	       } */
/*	     free(tmp.array); */
/*	  } */

     }
}
