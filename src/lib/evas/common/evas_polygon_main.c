#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>

#include "evas_common_private.h"
#include "evas_blend_private.h"

typedef struct _RGBA_Span RGBA_Span;
typedef struct _RGBA_Edge RGBA_Edge;
typedef struct _RGBA_Vertex RGBA_Vertex;

struct _RGBA_Span
{
   EINA_INLIST;
   int x, y, w;
};

struct _RGBA_Edge
{
   double x, dx;
   int i;
};

struct _RGBA_Vertex
{
   double x, y;
   int i;
};

#define POLY_EDGE_DEL(_i)                                               \
{                                                                       \
   int _j;                                                              \
                                                                        \
   for (_j = 0; (_j < num_active_edges) && (edges[_j].i != _i); _j++);  \
   if (_j < num_active_edges)                                           \
     {                                                                  \
	num_active_edges--;                                             \
	memmove(&(edges[_j]), &(edges[_j + 1]),                         \
	        (num_active_edges - _j) * sizeof(RGBA_Edge));           \
     }                                                                  \
}

#define POLY_EDGE_ADD(_i, _y)                                           \
{                                                                       \
   int _j;                                                              \
   float _dx;                                                           \
   RGBA_Vertex *_p, *_q;                                                \
   if (_i < (n - 1)) _j = _i + 1;                                       \
   else _j = 0;                                                         \
   if (point[_i].y < point[_j].y)                                       \
     {                                                                  \
	_p = &(point[_i]);                                              \
	_q = &(point[_j]);                                              \
     }                                                                  \
   else                                                                 \
     {                                                                  \
	_p = &(point[_j]);                                              \
	_q = &(point[_i]);                                              \
     }                                                                  \
   edges[num_active_edges].dx = _dx = (_q->x - _p->x) / (_q->y - _p->y); \
   edges[num_active_edges].x = (_dx * ((float)_y + 0.5 - _p->y)) + _p->x; \
   edges[num_active_edges].i = _i;                                      \
   num_active_edges++;                                                  \
}

EVAS_API void
evas_common_polygon_init(void)
{
}

/**
 * @brief Adds a point to a polygon.
 *
 * This function allocates a new RGBA_Polygon_Point, initializes it with the
 * given coordinates (x, y), and appends it to the linked list of points.
 *
 * @param points The existing list of polygon points (can be NULL for the first point).
 *               This list is an Eina_Inlist of RGBA_Polygon_Point.
 *               Example:
 *               RGBA_Polygon_Point *poly = NULL;
 *               poly = evas_common_polygon_point_add(poly, 10, 20);
 *               poly = evas_common_polygon_point_add(poly, 30, 40);
 *               // poly now contains two points: (10,20) -> (30,40)
 * @param x The x-coordinate of the point to add.
 * @param y The y-coordinate of the point to add.
 * @return The updated list of polygon points, with the new point appended.
 *         Returns the original list if memory allocation fails.
 */
EVAS_API RGBA_Polygon_Point *
evas_common_polygon_point_add(RGBA_Polygon_Point *points, int x, int y)
{
   RGBA_Polygon_Point *pt;

   pt = malloc(sizeof(RGBA_Polygon_Point));
   if (!pt) return points;
   pt->x = x;
   pt->y = y;
   points = (RGBA_Polygon_Point *)eina_inlist_append(EINA_INLIST_GET(points), EINA_INLIST_GET(pt));
   return points;
}

/**
 * @brief Clears all points from a polygon.
 *
 * This function iterates through the linked list of polygon points,
 * freeing each point. The list head is set to NULL.
 *
 * @param points The list of polygon points to clear.
 *               Example:
 *               poly = evas_common_polygon_points_clear(poly);
 *               // poly is now NULL
 * @return Always returns NULL, representing an empty list.
 */
EVAS_API RGBA_Polygon_Point *
evas_common_polygon_points_clear(RGBA_Polygon_Point *points)
{
   if (points)
     {
	while (points)
	  {
	     RGBA_Polygon_Point *old_p;

	     old_p = points;
	     points = (RGBA_Polygon_Point *)eina_inlist_remove(EINA_INLIST_GET(points), EINA_INLIST_GET(points));
	     free(old_p);
	  }
     }
   return NULL;
}

/**
 * @brief Comparator function for sorting RGBA_Vertex structures by y-coordinate.
 *
 * Used with qsort to sort an array of RGBA_Vertex pointers based on their
 * y-coordinate in ascending order. If y-coordinates are equal, the original
 * order is effectively maintained (though not strictly guaranteed by qsort
 * for equal elements unless a stable sort is used).
 *
 * @param a Pointer to the first RGBA_Vertex.
 * @param b Pointer to the second RGBA_Vertex.
 * @return -1 if a->y <= b->y, 1 otherwise.
 */
static int
polygon_point_sorter(const void *a, const void *b)
{
   RGBA_Vertex *p, *q;

   p = (RGBA_Vertex *)a;
   q = (RGBA_Vertex *)b;
   if (p->y <= q->y) return -1;
   return 1;
}

/**
 * @brief Comparator function for sorting RGBA_Edge structures by x-coordinate.
 *
 * Used with qsort to sort an array of RGBA_Edge structures based on their
 * current x-coordinate in ascending order. This is crucial for the scanline
 * algorithm to correctly identify span start and end points.
 *
 * @param a Pointer to the first RGBA_Edge.
 * @param b Pointer to the second RGBA_Edge.
 * @return -1 if p->x <= q->x, 1 otherwise.
 */
static int
polygon_edge_sorter(const void *a, const void *b)
{
   RGBA_Edge *p, *q;

   p = (RGBA_Edge *)a;
   q = (RGBA_Edge *)b;
   if (p->x <= q->x) return -1;
   return 1;
}

/**
 * @brief Draws a filled polygon onto an RGBA_Image using a scanline algorithm.
 *
 * This function implements a standard scanline polygon filling algorithm.
 * It processes the polygon's vertices, builds an active edge table (AET),
 * sorts edges by their x-intersections, and then fills horizontal spans
 * for each scanline. Clipping is applied based on the draw context.
 *
 * @param dst The destination RGBA_Image to draw onto.
 * @param dc The draw context, containing color, clipping, and render operation information.
 *           - dc->col.col: The color to fill the polygon with.
 *           - dc->clip: Clipping parameters.
 *           - dc->render_op: The rendering operation (e.g., copy, blend).
 *           - dc->clip.mask: Optional mask image for masked drawing.
 * @param points A linked list of RGBA_Polygon_Point defining the vertices of the polygon.
 *               The points should define a closed polygon. The order matters for winding.
 *               Example:
 *               RGBA_Polygon_Point *poly = NULL;
 *               poly = evas_common_polygon_point_add(poly, 0, 0);
 *               poly = evas_common_polygon_point_add(poly, 100, 0);
 *               poly = evas_common_polygon_point_add(poly, 50, 100);
 *               // This defines a triangle.
 * @param x The x-offset to apply to all polygon points.
 * @param y The y-offset to apply to all polygon points.
 */
EVAS_API void
evas_common_polygon_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, RGBA_Polygon_Point *points, int x, int y)
{
   RGBA_Gfx_Func      func;
   RGBA_Polygon_Point *pt;
   RGBA_Vertex       *point;
   RGBA_Edge         *edges;
   Eina_Inlist  *spans;
   int                num_active_edges;
   int                n;
   int                i, j, k;
   int                yy0, yy1, yi;
   int                ext_x, ext_y, ext_w, ext_h;
   int               *sorted_index;

   if (!dst->image.data) return;
   // Optional: Use Pixman for polygon rendering if available and configured.
#ifdef HAVE_PIXMAN
# ifdef PIXMAN_POLY
   pixman_op_t op = PIXMAN_OP_SRC; // _EVAS_RENDER_COPY
   if (dc->render_op == _EVAS_RENDER_BLEND)
     op = PIXMAN_OP_OVER;
# endif
#endif

   ext_x = 0;
   ext_y = 0;
   ext_w = dst->cache_entry.w;
   ext_h = dst->cache_entry.h;
   if (dc->clip.use)
     {
	if (dc->clip.x > ext_x)
	  {
	     ext_w += ext_x - dc->clip.x;
	     ext_x = dc->clip.x;
	  }
	if ((ext_x + ext_w) > (dc->clip.x + dc->clip.w))
	  {
	     ext_w = (dc->clip.x + dc->clip.w) - ext_x;
	  }
	if (dc->clip.y > ext_y)
	  {
	     ext_h += ext_y - dc->clip.y;
	     ext_y = dc->clip.y;
	  }
	if ((ext_y + ext_h) > (dc->clip.y + dc->clip.h))
	  {
	     ext_h = (dc->clip.y + dc->clip.h) - ext_y;
	  }
     }
   if ((ext_w <= 0) || (ext_h <= 0)) return;

   evas_common_cpu_end_opt(); // Ensure CPU-specific optimizations are finalized before drawing.

   n = 0; EINA_INLIST_FOREACH(points, pt) n++; // Count number of points.
   if (n < 3) return; // A polygon needs at least 3 vertices.
   edges = malloc(sizeof(RGBA_Edge) * n); // Active Edge Table (AET)
   if (!edges) return;
   point = malloc(sizeof(RGBA_Vertex) * n); // Array of vertices for sorting.
   if (!point)
     {
	free(edges);
	return;
     }
   sorted_index = malloc(sizeof(int) * n); // Stores original indices after sorting.
   if (!sorted_index)
     {
	free(edges);
	free(point);
	return;
     }

   // Initialize vertex array from the input points list, applying offsets.
   k = 0;
   EINA_INLIST_FOREACH(points, pt)
     {
	point[k].x = pt->x + x;
	point[k].y = pt->y + y;
	point[k].i = k; // Store original index.
	k++;
     }
   // Sort vertices by y-coordinate to process scanlines in order.
   // The sorted_index array keeps track of the original vertex indices
   // after the 'point' array is sorted by y-coordinate. This is needed
   // because the POLY_EDGE_ADD/DEL macros operate on the original point array.
   qsort(point, n, sizeof(RGBA_Vertex), polygon_point_sorter);
   for (k = 0; k < n; k++) sorted_index[k] = point[k].i;

   // Re-populate the point array with original points but now sorted_index
   // maps the scanline processing order to original vertex indices.
   // This step seems redundant or potentially misordered with the previous qsort.
   // The original point array (not the 'point' variable here, but the one implicitly
   // used by POLY_EDGE_ADD/DEL via indices) is what matters for edge calculations.
   // The 'point' array here is re-filled, but its sorted order from qsort is lost
   // if the EINA_INLIST_FOREACH order is different from the qsort order.
   // However, point[sorted_index[k]] is used later, which accesses the y-sorted vertices.
   k = 0;
   EINA_INLIST_FOREACH(points, pt)
     {
	point[k].x = pt->x + x; // This re-populates point[0]...point[n-1] in original order
	point[k].y = pt->y + y; // effectively overwriting the y-sorted version.
	point[k].i = k;         // This seems to be an error, as point[] should remain y-sorted.
                                // The POLY_EDGE_ADD/DEL macros use 'point[]' with original indices.
                                // The loop `for (; (k < n) && (point[sorted_index[k]].y <= ...); k++)`
                                // correctly uses `point[sorted_index[k]]` which refers to the y-sorted vertices.
                                // The `point` array itself is used by `POLY_EDGE_ADD` and `POLY_EDGE_DEL`
                                // which expect `point[i]` to be the i-th vertex in the original polygon definition.
                                // This second loop re-initializes `point` to the original vertex data,
                                // which is correct for `POLY_EDGE_ADD/DEL`. The `sorted_index` array
                                // is then used to iterate through vertices in their y-sorted order.
	k++;
     }

   // Determine the start and end scanlines for polygon processing.
   // point[sorted_index[0]] is the vertex with the minimum y.
   // point[sorted_index[n-1]] is the vertex with the maximum y.
   yy0 = MAX(ext_y, ceil(point[sorted_index[0]].y - 0.5));
   yy1 = MIN(ext_y + ext_h - 1, floor(point[sorted_index[n - 1]].y - 0.5));

   k = 0; // Index for iterating through sorted vertices.
   num_active_edges = 0; // Current number of edges in the AET.
   spans = NULL; // List to store horizontal spans to be drawn.

   // Main scanline loop: iterate from the top-most to bottom-most scanline of the polygon.
   for (yi = yy0; yi <= yy1; yi++)
     {
        // Update Active Edge Table (AET) for the current scanline yi.
        // Iterate through vertices sorted by y-coordinate.
	for (; (k < n) && (point[sorted_index[k]].y <= ((double)yi + 0.5)); k++)
	  {
	     i = sorted_index[k]; // Original index of the current vertex.

             // Consider edges connected to the current vertex point[i].
             // An edge is (point[j], point[i]).
	     if (i > 0) j = i - 1; // Previous vertex in original list.
	     else j = n - 1;       // Wrap around for the first vertex.

             // If the other end of the edge (point[j]) is above the current scanline,
             // this edge is no longer active or its activity changes.
	     if (point[j].y <= ((double)yi - 0.5))
	       {
		  POLY_EDGE_DEL(j) // Remove edge ending at point[j].
	       }
             // Else if the other end point[j] is below or on the current scanline,
             // this edge might become active.
	     else if (point[j].y > ((double)yi + 0.5))
	       {
		  POLY_EDGE_ADD(j, yi) // Add edge starting from point[j].
	       }

             // Consider the other edge connected to point[i].
             // An edge is (point[i], point[j]).
	     if (i < (n - 1)) j = i + 1; // Next vertex in original list.
	     else j = 0;                 // Wrap around for the last vertex.

	     if (point[j].y <= ((double)yi - 0.5))
	       {
		  POLY_EDGE_DEL(i) // Remove edge ending at point[i].
	       }
	     else if (point[j].y > ((double)yi + 0.5))
	       {
		  POLY_EDGE_ADD(i, yi) // Add edge starting from point[i].
	       }
	  }

        // Sort active edges by their current x-intersection point.
	qsort(edges, num_active_edges, sizeof(RGBA_Edge), polygon_edge_sorter);

        // Fill spans between pairs of active edges.
	for (j = 0; j < num_active_edges; j += 2)
	  {
	     int x0, x1;

	     x0 = ceil(edges[j].x - 0.5);
	     if (j < (num_active_edges - 1))
	       x1 = floor(edges[j + 1].x - 0.5);
	     else
	       x1 = x0;
	     if ((x1 >= ext_x) && (x0 < (ext_x + ext_w)) && (x0 <= x1))
	       {
		  RGBA_Span *span;

		  if (x0 < ext_x) x0 = ext_x;
		  if (x1 >= (ext_x + ext_w)) x1 = ext_x + ext_w - 1;
		  span = malloc(sizeof(RGBA_Span));
		  spans = eina_inlist_append(spans, EINA_INLIST_GET(span));
		  span->y = yi;
		  span->x = x0;
		  span->w = (x1 - x0) + 1;
	       }
             // Update x-coordinate for the next scanline using the edge's slope.
	     edges[j].x += edges[j].dx;
	     edges[j + 1].x += edges[j + 1].dx;
	  }
     }

   free(edges);
   free(point);
   free(sorted_index);

   // Select the appropriate drawing function based on whether a clip mask is used.
   if(dc->clip.mask)
     func = evas_common_gfx_func_composite_mask_color_span_get(dc->col.col, dst->cache_entry.flags.alpha, 1, dc->render_op);
   else
     func = evas_common_gfx_func_composite_color_span_get(dc->col.col, dst->cache_entry.flags.alpha, 1, dc->render_op);
   if (spans)
     {
	RGBA_Span *span;

	EINA_INLIST_FOREACH(spans, span)
	  {
	     DATA32 *ptr;
             DATA8 *mask;
             RGBA_Image *mask_ie;

#ifdef HAVE_PIXMAN
# ifdef PIXMAN_POLY
             // If pixman is available and enabled, use it for compositing the span.
	     if ((dst->pixman.im) && (dc->col.pixman_color_image))
	       pixman_image_composite(op, dc->col.pixman_color_image,
				      NULL, dst->pixman.im,
				      span->x, span->y, 0, 0,
				      span->x, span->y, span->w, 1);
	     else // Fallback to software rendering if pixman resources are not set up.
# endif
#endif
	       {
		 ptr = dst->image.data + (span->y * (dst->cache_entry.w)) + span->x;
                  if (dc->clip.mask)
                    {
                       mask_ie = dc->clip.mask;
                       mask = mask_ie->image.data8
                          + ((span->y - dc->clip.mask_y) * mask_ie->cache_entry.w)
                          + (span->x - dc->clip.mask_x);
                       func(NULL, mask, dc->col.col, ptr, span->w);
                    }
                  else // No clip mask, draw directly.
                    func(NULL, NULL, dc->col.col, ptr, span->w);
	       }
          }
        // Free the memory allocated for spans.
	while (spans)
	  {
	     span = (RGBA_Span *)spans;
	     spans = eina_inlist_remove(spans, spans);
	     free(span);
	  }
     }
}

/**
 * @brief Draws a filled polygon onto an RGBA_Image using a scanline algorithm with explicit parameters.
 *
 * This function is similar to evas_common_polygon_draw but takes explicit parameters for
 * clipping extent, color, render operation, and mask, instead of an RGBA_Draw_Context.
 * It's a lower-level version, potentially used when a full draw context is not available
 * or necessary. The core polygon rasterization logic is identical.
 *
 * @param dst The destination RGBA_Image to draw onto.
 * @param ext_x The x-coordinate of the clipping rectangle's top-left corner.
 * @param ext_y The y-coordinate of the clipping rectangle's top-left corner.
 * @param ext_w The width of the clipping rectangle.
 * @param ext_h The height of the clipping rectangle.
 * @param col The color (DATA32) to fill the polygon with.
 * @param render_op The rendering operation (e.g., _EVAS_RENDER_COPY, _EVAS_RENDER_BLEND).
 * @param points A linked list of RGBA_Polygon_Point defining the vertices of the polygon.
 * @param x The x-offset to apply to all polygon points.
 * @param y The y-offset to apply to all polygon points.
 * @param mask_ie Optional mask image (RGBA_Image) for masked drawing. Can be NULL.
 * @param mask_x The x-offset for the mask image relative to the destination.
 * @param mask_y The y-offset for the mask image relative to the destination.
 */
EVAS_API void
evas_common_polygon_rgba_draw(RGBA_Image *dst, int ext_x, int ext_y, int ext_w, int ext_h, DATA32 col, int render_op, RGBA_Polygon_Point *points, int x, int y, RGBA_Image *mask_ie, int mask_x, int mask_y)
{
   RGBA_Gfx_Func      func;
   RGBA_Polygon_Point *pt;
   RGBA_Vertex       *point;
   RGBA_Edge         *edges;
   Eina_Inlist  *spans;
   int                num_active_edges;
   int                n;
   int                i, j, k;
   int                yy0, yy1, yi;
   int               *sorted_index;

   if (!dst->image.data) return;
   if ((ext_w <= 0) || (ext_h <= 0)) return; // Nothing to draw if clip area is empty.

   evas_common_cpu_end_opt(); // Ensure CPU-specific optimizations are finalized.

   n = 0; EINA_INLIST_FOREACH(points, pt) n++; // Count vertices.
   if (n < 3) return; // Polygon needs at least 3 vertices.
   edges = malloc(sizeof(RGBA_Edge) * n); // Active Edge Table (AET).
   if (!edges) return;
   point = malloc(sizeof(RGBA_Vertex) * n); // Array of vertices for sorting.
   if (!point)
     {
	free(edges);
	return;
     }
   sorted_index = malloc(sizeof(int) * n); // Stores original indices after sorting.
   if (!sorted_index)
     {
	free(edges);
	free(point);
	return;
     }

   // Initialize vertex array from the input points list, applying offsets.
   k = 0;
   EINA_INLIST_FOREACH(points, pt)
     {
	point[k].x = pt->x + x;
	point[k].y = pt->y + y;
	point[k].i = k; // Store original index.
	k++;
     }
   // Sort vertices by y-coordinate.
   qsort(point, n, sizeof(RGBA_Vertex), polygon_point_sorter);
   for (k = 0; k < n; k++) sorted_index[k] = point[k].i;

   // Re-initialize 'point' array to original vertex data.
   // 'sorted_index' is used to iterate through vertices in y-sorted order,
   // while 'point[original_index]' is used by POLY_EDGE_ADD/DEL macros.
   k = 0;
   EINA_INLIST_FOREACH(points, pt)
     {
	point[k].x = pt->x + x;
	point[k].y = pt->y + y;
	point[k].i = k;
	k++;
     }

   // Determine the start and end scanlines for polygon processing, clipped to ext_y, ext_h.
   yy0 = MAX(ext_y, ceil(point[sorted_index[0]].y - 0.5));
   yy1 = MIN(ext_y + ext_h - 1, floor(point[sorted_index[n - 1]].y - 0.5));

   k = 0; // Index for iterating through y-sorted vertices.
   num_active_edges = 0; // Current number of edges in AET.
   spans = NULL; // List to store horizontal spans.

   // Main scanline loop.
   for (yi = yy0; yi <= yy1; yi++)
     {
        // Update Active Edge Table (AET) for current scanline yi.
	for (; (k < n) && (point[sorted_index[k]].y <= ((double)yi + 0.5)); k++)
	  {
	     i = sorted_index[k]; // Original index of the current vertex.

             // Process edges connected to vertex point[i].
	     if (i > 0) j = i - 1; else j = n - 1; // Previous vertex.
	     if (point[j].y <= ((double)yi - 0.5)) { POLY_EDGE_DEL(j); }
	     else if (point[j].y > ((double)yi + 0.5)) { POLY_EDGE_ADD(j, yi); }

	     if (i < (n - 1)) j = i + 1; else j = 0; // Next vertex.
	     if (point[j].y <= ((double)yi - 0.5)) { POLY_EDGE_DEL(i); }
	     else if (point[j].y > ((double)yi + 0.5)) { POLY_EDGE_ADD(i, yi); }
	  }

        // Sort active edges by x-intersection.
	qsort(edges, num_active_edges, sizeof(RGBA_Edge), polygon_edge_sorter);

        // Fill spans between pairs of active edges.
	for (j = 0; j < num_active_edges; j += 2)
	  {
	     int x0, x1;

	     x0 = ceil(edges[j].x - 0.5);
	     if (j < (num_active_edges - 1))
	       x1 = floor(edges[j + 1].x - 0.5);
	     else
	       x1 = x0;
	     if ((x1 >= ext_x) && (x0 < (ext_x + ext_w)) && (x0 <= x1))
	       {
		  RGBA_Span *span;

		  if (x0 < ext_x) x0 = ext_x;
		  if (x1 >= (ext_x + ext_w)) x1 = ext_x + ext_w - 1;
		  span = malloc(sizeof(RGBA_Span));
		  spans = eina_inlist_append(spans, EINA_INLIST_GET(span));
		  span->y = yi;
		  span->x = x0;
		  span->w = (x1 - x0) + 1;
	       }
             // Update x-coordinates for the next scanline.
	     edges[j].x += edges[j].dx;
	     edges[j + 1].x += edges[j + 1].dx;
	  }
     }

   free(edges);
   free(point);
   free(sorted_index);

   // Select drawing function based on whether a mask is provided.
   if (mask_ie)
     func = evas_common_gfx_func_composite_mask_color_span_get(col, dst->cache_entry.flags.alpha, 1, render_op);
   else
     func = evas_common_gfx_func_composite_color_span_get(col, dst->cache_entry.flags.alpha, 1, render_op);
   if (spans)
     {
	RGBA_Span *span;
        DATA8 *mask;

	EINA_INLIST_FOREACH(spans, span)
	  {
	     DATA32 *ptr;

             ptr = dst->image.data + (span->y * (dst->cache_entry.w)) + span->x;
             if (mask_ie) // If there's a mask, apply it.
               {
                  mask = mask_ie->image.data8 // Get pointer to mask data for this span.
                     + ((span->y - mask_y) * mask_ie->cache_entry.w)
                     + (span->x - mask_x);
                  func(NULL, mask, col, ptr, span->w);
               }
             else // No mask, draw directly.
               func(NULL, NULL, col, ptr, span->w);
          }
        // Free the memory allocated for spans.
	while (spans)
	  {
	     span = (RGBA_Span *)spans;
	     spans = eina_inlist_remove(spans, spans);
	     free(span);
	  }
     }
}
