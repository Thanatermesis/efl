#include "evas_common_private.h"
#include "region.h"


#ifdef NEWTILER
#define MAXREG 24

/**
 * @brief Initializes the tilebuffer system.
 * @note Currently a no-op in this implementation.
 */
EVAS_API void
evas_common_tilebuf_init(void)
{
}

/**
 * @brief Creates a new tilebuffer.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @return A pointer to the newly created Tilebuf, or NULL on failure.
 */
EVAS_API Tilebuf *
evas_common_tilebuf_new(int w, int h)
{
   Tilebuf *tb = malloc(sizeof(Tilebuf));
   tb->outbuf_w = w;
   tb->outbuf_h = h;
   tb->region = region_new(tb->outbuf_w, tb->outbuf_h);
   return tb;
}

/**
 * @brief Frees a tilebuffer.
 * @param tb The tilebuffer to free.
 */
EVAS_API void
evas_common_tilebuf_free(Tilebuf *tb)
{
   region_free(tb->region);
   free(tb);
}

/**
 * @brief Sets the tile size for the tilebuffer.
 * @note Currently a no-op in this implementation.
 * @param tb The tilebuffer.
 * @param tw The tile width.
 * @param th The tile height.
 */
EVAS_API void
evas_common_tilebuf_set_tile_size(Tilebuf *tb EINA_UNUSED, int tw EINA_UNUSED, int th EINA_UNUSED)
{
}

/**
 * @brief Gets the tile size of the tilebuffer.
 * @note Currently returns a fixed tile size of 1x1 in this implementation.
 * @param tb The tilebuffer.
 * @param tw Pointer to store the tile width.
 * @param th Pointer to store the tile height.
 */
EVAS_API void
evas_common_tilebuf_get_tile_size(Tilebuf *tb EINA_UNUSED, int *tw, int *th)
{
   if (tw) *tw = 1;
   if (th) *th = 1;
}

/**
 * @brief Sets the strict tiling mode.
 * @note Currently a no-op in this implementation.
 * @param tb The tilebuffer.
 * @param strict EINA_TRUE for strict tiling, EINA_FALSE otherwise.
 */
EVAS_API void
evas_common_tilebuf_tile_strict_set(Tilebuf *tb EINA_UNUSED, Eina_Bool strict EINA_UNUSED)
{
}

/**
 * @brief Adds a rectangle to the redraw region of the tilebuffer.
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return 1 on success, 0 on failure (though current implementation always returns 1).
 */
EVAS_API int
evas_common_tilebuf_add_redraw(Tilebuf *tb, int x, int y, int w, int h)
{
   region_rect_add(tb->region, x, y, w, h);
   return 1;
}

/**
 * @brief Deletes a rectangle from the redraw region of the tilebuffer.
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return 1 on success, 0 on failure (though current implementation always returns 1).
 */
EVAS_API int
evas_common_tilebuf_del_redraw(Tilebuf *tb, int x, int y, int w, int h)
{
   region_rect_del(tb->region, x, y, w, h);
   return 1;
}

/**
 * @brief Adds a motion vector to the tilebuffer.
 * @note Currently a no-op and always returns 0 in this implementation.
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the motion area.
 * @param y The y-coordinate of the motion area.
 * @param w The width of the motion area.
 * @param h The height of the motion area.
 * @param dx The horizontal displacement.
 * @param dy The vertical displacement.
 * @param alpha The alpha value for blending (unused).
 * @return 0, indicating no motion vector was added.
 */
EVAS_API int
evas_common_tilebuf_add_motion_vector(Tilebuf *tb EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dx EINA_UNUSED, int dy EINA_UNUSED, int alpha EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Clears all redraw regions from the tilebuffer.
 * @param tb The tilebuffer to clear.
 */
EVAS_API void
evas_common_tilebuf_clear(Tilebuf *tb)
{
   region_free(tb->region);
   tb->region = region_new(tb->outbuf_w, tb->outbuf_h);
}

/**
 * @internal
 * @brief Rounds the rectangles in a region to align with a given tile size.
 * This function takes an input region and creates a new region where all
 * rectangles are expanded or adjusted to be multiples of tsize.
 * The x1, y1 coordinates are rounded down to the nearest multiple of tsize.
 * The x2, y2 coordinates are rounded up to the nearest multiple of tsize.
 * @param region The input region.
 * @param tsize The tile size to align to.
 * @return A new Region object with rounded rectangles, or NULL on failure.
 *         The caller is responsible for freeing the returned region.
 */
static Region *
_region_round(Region *region, int tsize)
{
   Region *region2;
   Box *rects;
   int num, i, w, h;

   region_size_get(region, &w, &h);
   region2 = region_new(w, h);
   rects = region_rects(region);
   num = region_rects_num(region);
   for (i = 0; i < num; i++)
     {
        int x1, y1, x2, y2;

        x1 = (rects[i].x1 / tsize) * tsize;
        y1 = (rects[i].y1 / tsize) * tsize;
        x2 = ((rects[i].x2 + tsize - 1) / tsize) * tsize;
        y2 = ((rects[i].y2 + tsize - 1) / tsize) * tsize;
        region_rect_add(region2, x1, y1, x2 - x1, y2 - y1);
     }
   return region2;
}

/**
 * @brief Retrieves the rectangles that need to be rendered.
 *
 * This function processes the current redraw region in the tilebuffer,
 * rounds the rectangles to a tile size (hardcoded to 16 here),
 * and prepares a list of Tilebuf_Rect structures for rendering.
 * If the number of distinct rectangles exceeds MAXREG (24),
 * a single bounding box encompassing all rectangles is returned instead.
 *
 * @param tb The tilebuffer.
 * @return A pointer to an Eina_Inlist of Tilebuf_Rect structures.
 *         Each Tilebuf_Rect represents an area to be rendered.
 *         The format of the list is an Eina_Inlist, where each node
 *         is a Tilebuf_Rect. Example:
 *         rects -> Tilebuf_Rect{x,y,w,h, EINA_INLIST}
 *                  -> Tilebuf_Rect{x,y,w,h, EINA_INLIST}
 *                  -> ...
 *         The caller is responsible for freeing this list using
 *         evas_common_tilebuf_free_render_rects().
 *         Returns NULL if there are no rectangles to render or on allocation failure.
 */
EVAS_API Tilebuf_Rect *
evas_common_tilebuf_get_render_rects(Tilebuf *tb)
{
   Tilebuf_Rect *rects = NULL, *r, *rend, *rbuf;
   Region *region2;
   Box *rects2, *rs;
   int n, num, minx, miny, maxx, maxy;

   region2 = _region_round(tb->region, 16);
   if (!region2) return NULL;

   rects2 = region_rects(region2);
   if (!rects2)
     {
        region_free(region2);
        return NULL;
     }
   n = region_rects_num(region2);
   if (n <= 0)
     {
        region_free(region2);
        return NULL;
     }

   rbuf = malloc(n * sizeof(Tilebuf_Rect));
   if (!rbuf)
     {
        region_free(region2);
        return NULL;
     }

   rend = rbuf + n;
   rs = rects2;
   num = 0;

   minx = rs->x1;
   miny = rs->y1;
   maxx = rs->x2;
   maxy = rs->y2;
   for (r = rbuf; r < rend; r++)
     {
        if (rs->x1 < minx) minx = rs->x1;
        if (rs->y1 < miny) miny = rs->y1;
        if (rs->x2 > maxx) maxx = rs->x2;
        if (rs->y2 > maxy) maxy = rs->y2;
        EINA_INLIST_GET(r)->next = NULL;
        EINA_INLIST_GET(r)->prev = NULL;
        EINA_INLIST_GET(r)->last = NULL;
        r->x = rs->x1;
        r->y = rs->y1;
        r->w = rs->x2 - rs->x1;
        r->h = rs->y2 - rs->y1;
        rs++;
        rects = (Tilebuf_Rect *)
          eina_inlist_append(EINA_INLIST_GET(rects),
                             EINA_INLIST_GET(r));
        num++;
     }
   // if > max, then bounding box
   if (num > MAXREG)
     {
        r = rects;
        EINA_INLIST_GET(r)->next = NULL;
        EINA_INLIST_GET(r)->prev = NULL;
        EINA_INLIST_GET(r)->last = NULL;
        r->x = minx;
        r->y = miny;
        r->w = maxx - minx;
        r->h = maxy - miny;
     }
   region_free(region2);
   return rects;
}

/**
 * @brief Frees the list of render rectangles.
 * @param rects The list of Tilebuf_Rect structures to free. This list
 *              is typically obtained from evas_common_tilebuf_get_render_rects().
 *              It's a contiguous block of memory allocated for the rectangles,
 *              even though it's used as an Eina_Inlist.
 */
EVAS_API void
evas_common_tilebuf_free_render_rects(Tilebuf_Rect *rects)
{
   free(rects);
}

#else

#define FUZZ 32
#define MAXREG 24
#define MAX_NODES 1024

static inline void rect_list_node_pool_flush(void);
static inline list_node_t *rect_list_node_pool_get(void);
static inline void rect_list_node_pool_put(list_node_t *node);
static inline void rect_init(rect_t *r, int x, int y, int w, int h);
static inline void rect_list_append_node(list_t *rects, list_node_t *node);
static inline void rect_list_append(list_t *rects, const rect_t r);
static inline void rect_list_append_xywh(list_t *rects, int x, int y, int w, int h);
static inline void rect_list_concat(list_t *rects, list_t *other);
static inline list_node_t *rect_list_unlink_next(list_t *rects, list_node_t *parent_node);
static inline void rect_list_del_next(list_t *rects, list_node_t *parent_node);
static inline void rect_list_clear(list_t *rects);
static inline void rect_list_del_split_strict(list_t *rects, const rect_t del_r);
static inline list_node_t *rect_list_add_split_fuzzy(list_t *rects, list_node_t *node, int accepted_error);
static inline void rect_list_merge_rects(list_t *rects, list_t *to_merge, int accepted_error);
static inline void rect_list_add_split_fuzzy_and_merge(list_t *rects, list_node_t *node, int split_accepted_error, int merge_accepted_error);

static const list_node_t list_node_zeroed = { NULL };
static const list_t list_zeroed = { NULL, NULL };

/** @internal Structure for managing a pool of list_node_t objects. */
typedef struct list_node_pool
{
   list_node_t *node; /**< Pointer to the head of the free list. */
   int len;           /**< Current number of nodes in the pool. */
   int max;           /**< Maximum number of nodes to keep in the pool. */
} list_node_pool_t;

/** @internal Global instance of the list node pool. */
static list_node_pool_t list_node_pool = { NULL, 0, MAX_NODES };

/**
 * @internal
 * @brief Flushes the list node pool, freeing all cached nodes.
 * This is typically called during cleanup or when memory needs to be reclaimed.
 */
static inline void
rect_list_node_pool_flush(void)
{
   while (list_node_pool.node)
     {
        list_node_t *node = list_node_pool.node;
        list_node_pool.node = node->next;
        list_node_pool.len--;
        free(node);
     }
}

/**
 * @internal
 * @brief Retrieves a list_node_t from the pool or allocates a new one.
 * If the pool has available nodes, one is returned. Otherwise, a new node
 * is allocated using malloc.
 * @return A pointer to a list_node_t.
 */
static inline list_node_t *
rect_list_node_pool_get(void)
{
   if (list_node_pool.node)
     {
        list_node_t *node = list_node_pool.node;
        list_node_pool.node = node->next;
        list_node_pool.len--;
        return node;
     }
   else return (list_node_t *)malloc(sizeof(rect_node_t));
}

/**
 * @internal
 * @brief Returns a list_node_t to the pool if space is available, otherwise frees it.
 * If the pool's current size is less than its maximum capacity, the node is
 * added to the pool's free list. Otherwise, the node is freed directly.
 * @param node The list_node_t to return to the pool or free.
 */
static inline void
rect_list_node_pool_put(list_node_t *node)
{
   if (list_node_pool.len < list_node_pool.max)
     {
        node->next = list_node_pool.node;
        list_node_pool.node = node;
        list_node_pool.len++;
     }
   else free(node);
}

/**
 * @internal
 * @brief Initializes a rectangle structure.
 * Calculates and sets the area, left, top, right, bottom, width, and height
 * properties of the given rect_t structure.
 * @param r Pointer to the rect_t structure to initialize.
 * @param x The x-coordinate of the top-left corner.
 * @param y The y-coordinate of the top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
static inline void
rect_init(rect_t *r, int x, int y, int w, int h)
{
   r->area = w * h;
   r->left = x;
   r->top = y;
   r->right = x + w;
   r->bottom = y + h;
   r->width = w;
   r->height = h;
}

/**
 * @internal
 * @brief Appends a pre-allocated node to the end of a rectangle list.
 * @param rects Pointer to the list_t structure representing the rectangle list.
 * @param node Pointer to the list_node_t to append. The node should already
 *             contain the rectangle data.
 */
static inline void
rect_list_append_node(list_t *rects, list_node_t *node)
{
   if (rects->tail)
     {
	rects->tail->next = node;
	rects->tail = node;
     }
   else
     {
	rects->head = node;
	rects->tail = node;
     }
}

/**
 * @internal
 * @brief Appends a new rectangle to the end of a rectangle list.
 * A new node is obtained from the pool, initialized with the given rectangle data,
 * and then appended to the list.
 * @param rects Pointer to the list_t structure.
 * @param r The rect_t data to append.
 */
static inline void
rect_list_append(list_t *rects, const rect_t r)
{
   rect_node_t *rect_node = (rect_node_t *)rect_list_node_pool_get();
   rect_node->rect = r;
   rect_node->_lst = list_node_zeroed;
   rect_list_append_node(rects, (list_node_t *)rect_node);
}

/**
 * @internal
 * @brief Appends a new rectangle, defined by x, y, w, h, to a list.
 * Convenience function that initializes a rect_t and then appends it.
 * @param rects Pointer to the list_t structure.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param w The width.
 * @param h The height.
 */
static inline void
rect_list_append_xywh(list_t *rects, int x, int y, int w, int h)
{
   rect_t r;
   rect_init(&r, x, y, w, h);
   rect_list_append(rects, r);
}

/**
 * @internal
 * @brief Concatenates one rectangle list to another.
 * Appends all nodes from the 'other' list to the end of the 'rects' list.
 * The 'other' list becomes empty after this operation.
 * @param rects Pointer to the destination list_t.
 * @param other Pointer to the source list_t to be concatenated.
 */
static inline void
rect_list_concat(list_t *rects, list_t *other)
{
   if (!other->head) return;
   if (rects->tail)
     {
	rects->tail->next = other->head;
	rects->tail = other->tail;
     }
   else
     {
	rects->head = other->head;
	rects->tail = other->tail;
     }
   *other = list_zeroed;
}

/**
 * @internal
 * @brief Unlinks the node following parent_node from the list.
 * If parent_node is NULL, unlinks the head of the list.
 * Adjusts head/tail pointers of the list as necessary.
 * The unlinked node's next/prev pointers are zeroed.
 * @param rects Pointer to the list_t structure.
 * @param parent_node The node preceding the one to be unlinked, or NULL
 *                    to unlink the head.
 * @return The unlinked list_node_t. The caller is responsible for
 *         either reusing or freeing this node (e.g., via rect_list_node_pool_put).
 */
static inline list_node_t *
rect_list_unlink_next(list_t *rects, list_node_t *parent_node)
{
   list_node_t *node;

   if (parent_node)
     {
        node = parent_node->next;
        parent_node->next = node->next;
     }
   else
     {
        node = rects->head;
        rects->head = node->next;
     }
   if (rects->tail == node) rects->tail = parent_node;
   *node = list_node_zeroed;
   return node;
}

/**
 * @internal
 * @brief Deletes the node following parent_node from the list.
 * If parent_node is NULL, deletes the head of the list.
 * The deleted node is returned to the node pool.
 * @param rects Pointer to the list_t structure.
 * @param parent_node The node preceding the one to be deleted, or NULL
 *                    to delete the head.
 */
static inline void
rect_list_del_next(list_t *rects, list_node_t *parent_node)
{
    list_node_t *node = rect_list_unlink_next(rects, parent_node);
    rect_list_node_pool_put(node);
}

/**
 * @internal
 * @brief Clears all rectangles from a list.
 * All nodes in the list are returned to the node pool, and the list
 * is reset to an empty state.
 * @param rects Pointer to the list_t structure to clear.
 */
static inline void
rect_list_clear(list_t *rects)
{
   list_node_t *node = rects->head;
   while (node)
     {
        list_node_t *aux;

        aux = node->next;
        rect_list_node_pool_put(node);
        node = aux;
     }
   *rects = list_zeroed;
}

/**
 * @internal
 * @brief Calculates the dimensions of the intersection of two rectangles.
 * @param a The first rectangle.
 * @param b The second rectangle.
 * @param[out] width Pointer to store the width of the intersection.
 *                   Set to 0 or negative if no intersection on the x-axis.
 * @param[out] height Pointer to store the height of the intersection.
 *                    Set to 0 or negative if no intersection on the y-axis.
 */
static inline void
_calc_intra_rect_area(const rect_t a, const rect_t b, int *width, int *height)
{
   int max_left, min_right, max_top, min_bottom;

   if (a.left < b.left) max_left = b.left;
   else max_left = a.left;
   if (a.right < b.right) min_right = a.right;
   else min_right = b.right;
   *width = min_right - max_left;

   if (a.top < b.top) max_top = b.top;
   else max_top = a.top;
   if (a.bottom < b.bottom) min_bottom = a.bottom;
   else min_bottom = b.bottom;
   *height = min_bottom - max_top;
}

/**
 * @internal
 * @brief Splits rectangle 'r' by subtracting 'current' rectangle from it.
 * This function is used when a rectangle 'current' (which is being deleted or
 * processed) overlaps with another rectangle 'r'. The parts of 'r' that do
 * not overlap with 'current' are added to the 'dirty' list as new rectangles.
 * 'r' itself is effectively consumed or modified in this process conceptually,
 * though its input value isn't directly changed for w_1, w_2 calculations.
 *
 * Example:
 * If 'r' is a large rectangle and 'current' is a smaller rectangle inside 'r',
 * _split_strict will add up to four new rectangles to 'dirty' representing
 * the parts of 'r' surrounding 'current'.
 *
 *   Initial state:
 *   +-----------------+
 *   | r               |
 *   |   +---------+   |
 *   |   | current |   |
 *   |   +---------+   |
 *   |                 |
 *   +-----------------+
 *
 *   After _split_strict (conceptual, r is split into r_top, r_bottom, r_left, r_right):
 *   +-----------------+ dirty gets:
 *   | r_top           |   r_top
 *   +-----+-----+-----+   r_left, r_right
 *   |r_left|current|r_right| r_bottom
 *   +-----+-----+-----+
 *   | r_bottom        |
 *   +-----------------+
 *
 * @param dirty List to add the resulting split rectangles to.
 * @param current The rectangle that is causing the split (e.g., a deletion area).
 * @param r The rectangle to be split.
 */
static inline void
_split_strict(list_t *dirty, const rect_t current, rect_t r)
{
   int h_1, h_2, w_1, w_2;

   h_1 = current.top - r.top;
   h_2 = r.bottom - current.bottom;
   w_1 = current.left - r.left;
   w_2 = r.right - current.right;
   if (h_1 > 0)
     {
	/*    .--.r (b)                .---.r2
         *    |  |                     |   |
         *  .-------.cur (a) .---.r    '---'
         *  | |  |  |     -> |   |   +
         *  | `--'  |        `---'
         *  `-------'
         */
        rect_list_append_xywh(dirty, r.left, r.top, r.width, h_1);
        r.height -= h_1;
        r.top = current.top;
     }
   if (h_2 > 0)
     {
        /*  .-------.cur (a)
         *  | .---. |        .---.r
         *  | |   | |    ->  |   |
         *  `-------'        `---'   +  .---.r2
         *    |   |                     |   |
         *    `---'r (b)                `---'
         */
        rect_list_append_xywh(dirty, r.left, current.bottom, r.width, h_2);
        r.height -= h_2;
     }
   if (w_1 > 0)
     {
        /* (b) r  .----.cur (a)
         *     .--|-.  |      .--.r2   .-.r
         *     |  | |  |  ->  |  |   + | |
         *     `--|-'  |      `--'     `-'
         *        `----'
         */
        rect_list_append_xywh(dirty, r.left, r.top, w_1, r.height);
        /* not necessary to keep these, r (b) will be destroyed */
        /* r.width -= w_1; */
        /* r.left = current.left; */
     }
   if (w_2 > 0)
     {
        /*  .----.cur (a)
         *  |    |
         *  |  .-|--.r (b)  .-.r   .--.r2
         *  |  | |  |    -> | |  + |  |
         *  |  `-|--'       `-'    `--'
         *  `----'
         */
        rect_list_append_xywh(dirty, current.right, r.top, w_2, r.height);
        /* not necessary to keep this, r (b) will be destroyed */
        /* r.width -= w_2; */
     }
}

/**
 * @internal
 * @brief Deletes a rectangle 'del_r' from a list of rectangles 'rects'.
 * This function iterates through the 'rects' list. If 'del_r' completely
 * contains a rectangle in the list, that rectangle is removed. If 'del_r'
 * partially overlaps a rectangle, that rectangle is split into smaller pieces
 * using _split_strict, and the original overlapping rectangle is removed.
 * Non-overlapping rectangles are untouched.
 *
 * @param rects The list of rectangles to modify.
 * @param del_r The rectangle to delete from the list.
 */
static inline void
rect_list_del_split_strict(list_t *rects, const rect_t del_r)
{
   list_t modified = list_zeroed;
   list_node_t *cur_node, *prev_node;
   int intra_width, intra_height;
   rect_t current;

   prev_node = NULL;
   cur_node = rects->head;
   while (cur_node)
     {
        current = ((rect_node_t*)cur_node)->rect;
        _calc_intra_rect_area(del_r, current, &intra_width, &intra_height);
        if ((intra_width <= 0) || (intra_height <= 0))
          {
             /*  .---.current      .---.del_r
              *  |   |             |   |
              *  `---+---.del_r    `---+---.current
              *      |   |             |   |
              *      `---'             `---'
              * no interception, nothing to do
              */
              prev_node = cur_node;
              cur_node = cur_node->next;
          }
        else if ((intra_width == current.width) &&
                 (intra_height == current.height))
          {
             /*  .-------.del_r
              *  | .---. |
              *  | |   | |
              *  | `---'current
              *  `-------'
              * current is contained, remove from rects
              */
              cur_node = cur_node->next;
              rect_list_del_next(rects, prev_node);
          }
        else
          {
              _split_strict(&modified, del_r, current);
              cur_node = cur_node->next;
              rect_list_del_next(rects, prev_node);
          }
     }

   rect_list_concat(rects, &modified);
}

/**
 * @internal
 * @brief Calculates both the intersection (intra) and bounding box (outer) of two rectangles.
 *
 * @param a The first rectangle.
 * @param b The second rectangle.
 * @param[out] intra Pointer to a rect_t structure to store the intersection rectangle.
 *                   If there's no intersection, intra->area will be 0.
 * @param[out] outer Pointer to a rect_t structure to store the bounding box rectangle.
 */
static inline void
_calc_intra_outer_rect_area(const rect_t a, const rect_t b,
                            rect_t *intra, rect_t *outer)
{
   int min_left, max_left, min_right, max_right;
   int min_top, max_top, min_bottom, max_bottom;

   if (a.left < b.left)
     {
        max_left = b.left;
        min_left = a.left;
     }
   else
     {
        max_left = a.left;
        min_left = b.left;
     }
   if (a.right < b.right)
     {
        min_right = a.right;
        max_right = b.right;
     }
   else
     {
        min_right = b.right;
        max_right = a.right;
     }
   intra->left = max_left;
   intra->right = min_right;
   intra->width = min_right - max_left;
   outer->left = min_left;
   outer->right = max_right;
   outer->width = max_right - min_left;
   if (a.top < b.top)
     {
	max_top = b.top;
        min_top = a.top;
     }
   else
     {
        max_top = a.top;
        min_top = b.top;
     }
   if (a.bottom < b.bottom)
     {
        min_bottom = a.bottom;
        max_bottom = b.bottom;
     }
   else
     {
        min_bottom = b.bottom;
        max_bottom = a.bottom;
     }
   intra->top = max_top;
   intra->bottom = min_bottom;
   intra->height = min_bottom - max_top;
   if ((intra->width > 0) && (intra->height > 0))
     intra->area = intra->width * intra->height;
   else
     intra->area = 0;
   outer->top = min_top;
   outer->bottom = max_bottom;
   outer->height = max_bottom - min_top;
   outer->area = outer->width * outer->height;
}

enum
{
   SPLIT_FUZZY_ACTION_NONE,
   SPLIT_FUZZY_ACTION_SPLIT,
   SPLIT_FUZZY_ACTION_MERGE /**< Indicates that rectangles a and b can be merged horizontally. */
};

/**
 * @internal
 * @brief Splits rectangle 'b' based on its overlap with rectangle 'a', with fuzzy logic.
 * This is similar to _split_strict but allows for slight imperfections or specific
 * merge conditions. It's used in a context where rectangles might be merged if
 * they are "close enough" or align in a particular way.
 *
 * - If 'b' is partially outside 'a', the non-overlapping parts of 'b' are added to 'dirty'.
 * - 'b' is modified to represent the overlapping part or a part that might be merged.
 * - Returns an action code:
 *   - SPLIT_FUZZY_ACTION_NONE: No split or specific merge action taken for 'b' relative to 'a'.
 *   - SPLIT_FUZZY_ACTION_SPLIT: 'b' was split, and parts were added to 'dirty'.
 *   - SPLIT_FUZZY_ACTION_MERGE: 'b' and 'a' are candidates for a horizontal merge
 *     (same height, adjacent or overlapping horizontally).
 *
 * @param dirty List to add newly created rectangle fragments to.
 * @param a The reference rectangle.
 * @param b Pointer to the rectangle to be split or modified. This rectangle's
 *          properties (top, height) might be changed by this function.
 * @return An integer indicating the action taken (SPLIT_FUZZY_ACTION_NONE,
 *         SPLIT_FUZZY_ACTION_SPLIT, or SPLIT_FUZZY_ACTION_MERGE).
 */
static inline int
_split_fuzzy(list_t *dirty, const rect_t a, rect_t *b)
{
   int h_1, h_2, w_1, w_2, action;

   h_1 = a.top - b->top;
   h_2 = b->bottom - a.bottom;
   w_1 = a.left - b->left;
   w_2 = b->right - a.right;

   action = SPLIT_FUZZY_ACTION_NONE;
   if (h_1 > 0)
     {
        /*    .--.r (b)                .---.r2
         *    |  |                     |   |
         *  .-------.cur (a) .---.r    '---'
         *  | |  |  |     -> |   |   +
         *  | `--'  |        `---'
         *  `-------'
         */
        rect_list_append_xywh(dirty, b->left, b->top, b->width, h_1);
        b->height -= h_1;
        b->top = a.top;
        action = SPLIT_FUZZY_ACTION_SPLIT;
     }
   if (h_2 > 0)
     {
        /*  .-------.cur (a)
         *  | .---. |        .---.r
         *  | |   | |    ->  |   |
         *  `-------'        `---'   +  .---.r2
         *    |   |                     |   |
         *    `---'r (b)                `---'
         */
        rect_list_append_xywh(dirty, b->left, a.bottom, b->width, h_2);
        b->height -= h_2;
        action = SPLIT_FUZZY_ACTION_SPLIT;
     }
   if (((w_1 > 0) || (w_2 > 0)) && (a.height == b->height))
     return SPLIT_FUZZY_ACTION_MERGE;
   if (w_1 > 0)
     {
        /* (b)  r  .----.cur (a)
         *      .--|-.  |      .--.r2   .-.r
         *      |  | |  |  ->  |  |   + | |
         *      `--|-'  |      `--'     `-'
         *         `----'
         */
        rect_list_append_xywh(dirty, b->left, b->top, w_1, b->height);
        /* not necessary to keep these, r (b) will be destroyed */
        /* b->width -= w_1; */
        /* b->left = a.left; */
        action = SPLIT_FUZZY_ACTION_SPLIT;
     }
   if (w_2 > 0)
     {
        /* .----.cur (a)
         * |    |
         * |  .-|--.r (b)  .-.r   .--.r2
         * |  | |  |    -> | |  + |  |
         * |  `-|--'       `-'    `--'
         * `----'
         */
        rect_list_append_xywh(dirty, a.right, b->top, w_2, b->height);
        /* not necessary to keep these, r (b) will be destroyed */
        /* b->width -= w_2; */
        action = SPLIT_FUZZY_ACTION_SPLIT;
     }
   return action;
}

/**
 * @internal
 * @brief Adds a new rectangle node to a list, performing fuzzy splitting and merging.
 * This function attempts to integrate the new rectangle ('node') into the existing
 * 'rects' list. It processes the 'node' and potentially other rectangles from
 * 'rects' by:
 * 1. Checking for containment: If 'node' is already contained, it's discarded.
 *    If 'node' contains an existing rect, that rect is removed.
 * 2. Merging: If 'node' and an existing rect can form a new bounding box whose
 *    area is not much larger than their combined areas (within 'accepted_error'),
 *    they are merged.
 * 3. Splitting: If 'node' partially overlaps an existing rect and merging isn't
 *    optimal, the existing rect might be split using _split_fuzzy.
 *
 * The function maintains a 'dirty' list of rectangles that need further processing.
 * The goal is to keep the 'rects' list optimized by merging and splitting
 * rectangles to reduce redundancy and fragmentation, within the fuzzy tolerance.
 *
 * @param rects The main list of rectangles to which 'node' is being added.
 * @param node The new rectangle node to add. This node will be consumed (either
 *             added to 'rects', or its data used for merging/splitting and then pooled).
 * @param accepted_error The tolerance for merging. If (bounding_box_area - (area1 + area2 - intersection_area))
 *                       is less than or equal to this, rectangles may be merged.
 *                       Also used for intersection_area - combined_area_without_overlap.
 * @return Returns the 'old_last' node of the 'rects' list before any modifications
 *         due to processing 'node' and subsequent items from the 'dirty' list that
 *         were originally from 'rects'. This is used by the caller
 *         (rect_list_add_split_fuzzy_and_merge) to determine which part of the
 *         list needs further merging.
 */
static inline list_node_t *
rect_list_add_split_fuzzy(list_t *rects, list_node_t *node, int accepted_error)
{
   list_t dirty = list_zeroed;
   list_node_t *old_last = rects->tail;

   if (!rects->head)
     {
        rect_list_append_node(rects, node);
        return old_last;
     }
   rect_list_append_node(&dirty, node);
   while (dirty.head)
     {
	list_node_t *d_node, *cur_node, *prev_cur_node;
        int keep_dirty;
        rect_t r;

        d_node = rect_list_unlink_next(&dirty, NULL);
        r = ((rect_node_t *)d_node)->rect;
        prev_cur_node = NULL;
        cur_node = rects->head;
        keep_dirty = 1;
        while (cur_node)
	  {
	     int area, action;
	     rect_t current, intra, outer;

	     current = ((rect_node_t *)cur_node)->rect;
	     _calc_intra_outer_rect_area(r, current, &intra, &outer);
	     area = current.area + r.area - intra.area;
	     if ((intra.width == r.width) && (intra.height == r.height))
	       {
		  /*  .-------.cur
		   *  | .---.r|
		   *  | |   | |
		   *  | `---' |
		   *  `-------'
		   */
		  keep_dirty = 0;
		  break;
	       }
	     else if ((intra.width == current.width) &&
		      (intra.height == current.height))
	       {
		  /* .-------.r
		   * | .---.cur
		   * | |   | |
		   * | `---' |
		   * `-------'
		   */
		  if (old_last == cur_node)
                    old_last = prev_cur_node;
		  cur_node = cur_node->next;
		  rect_list_del_next(rects, prev_cur_node);
	       }
	     else if ((outer.area - area) <= accepted_error)
	       {
		  /* .-----------. bounding box (outer)
		   * |.---. .---.|
		   * ||cur| |r  ||
		   * ||   | |   ||
		   * |`---' `---'|
		   * `-----------'
		   * merge them, remove both and add merged
		   */
		  rect_node_t *n;

		  if (old_last == cur_node)
                    old_last = prev_cur_node;

		  n = (rect_node_t *)rect_list_unlink_next(rects, prev_cur_node);
		  n->rect = outer;
		  rect_list_append_node(&dirty, (list_node_t *)n);

		  keep_dirty = 0;
		  break;
	       }
	     else if ((intra.area - area) <= accepted_error)
	       {
		  /*  .---.cur     .---.r
		   *  |   |        |   |
		   *  `---+---.r   `---+---.cur
		   *      |   |        |   |
		   *      `---'        `---'
		   *  no split, no merge
		   */
		  prev_cur_node = cur_node;
		  cur_node = cur_node->next;
	       }
	     else
	       {
		  /* split is required */
		  action = _split_fuzzy(&dirty, current, &r);
		  if (action == SPLIT_FUZZY_ACTION_MERGE)
		    {
		       /* horizontal merge is possible: remove both, add merged */
		       rect_node_t *n;

		       if (old_last == cur_node)
			 old_last = prev_cur_node;

		       n = (rect_node_t *)
			 rect_list_unlink_next(rects, prev_cur_node);

		       n->rect.left = outer.left;
		       n->rect.width = outer.width;
		       n->rect.right = outer.right;
		       n->rect.area = outer.width * r.height;
		       rect_list_append_node(&dirty, (list_node_t *)n);
		    }
		  else if (action == SPLIT_FUZZY_ACTION_NONE)
		    {
		       /* this rect check was totally useless,
			* should never happen */
		       /* prev_cur_node = cur_node; */
		       /* cur_node = cur_node->next; */
		       WRN("Should not get here!");
		       abort();
		    }
		  keep_dirty = 0;
		  break;
	       }
	  }
        if (UNLIKELY(keep_dirty)) rect_list_append_node(rects, d_node);
        else rect_list_node_pool_put(d_node);
     }
    return old_last;
}

/**
 * @internal
 * @brief Calculates the bounding box (outer rectangle) of two rectangles.
 * @param a The first rectangle.
 * @param b The second rectangle.
 * @param[out] outer Pointer to a rect_t structure to store the resulting
 *                   bounding box.
 */
static inline void
_calc_outer_rect_area(const rect_t a, const rect_t b, rect_t *outer)
{
   int min_left, max_right;
   int min_top, max_bottom;

   if (a.left < b.left) min_left = a.left;
   else min_left = b.left;
   if (a.right < b.right) max_right = b.right;
   else max_right = a.right;
   outer->left = min_left;
   outer->right = max_right;
   outer->width = max_right - min_left;
   if (a.top < b.top) min_top = a.top;
   else min_top = b.top;
   if (a.bottom < b.bottom) max_bottom = b.bottom;
   else max_bottom = a.bottom;
   outer->top = min_top;
   outer->bottom = max_bottom;
   outer->height = max_bottom - min_top;
   outer->area = outer->width * outer->height;
}

/**
 * @internal
 * @brief Merges rectangles from the 'to_merge' list into the 'rects' list.
 * This function iterates through each rectangle in 'to_merge'. For each such
 * rectangle (r1), it searches 'rects' for a rectangle (r2) that can be merged
 * with r1. Merging occurs if their combined bounding box area is not significantly
 * larger (within 'accepted_error') than the sum of their individual areas.
 * If a merge occurs, r1 and r2 are replaced by their merged version, which is
 * then added back to 'to_merge' for further processing. If r1 cannot be merged
 * with any rectangle in 'rects', it's moved from 'to_merge' to 'rects'.
 *
 * @param rects The target list where merged rectangles are accumulated.
 * @param to_merge List of rectangles to be merged into 'rects'. This list
 *                 will be empty after the function completes.
 * @param accepted_error Tolerance for merging, similar to its use in
 *                       rect_list_add_split_fuzzy. (outer.area - (r1.area + r2.area)) <= accepted_error
 */
static inline void
rect_list_merge_rects(list_t *rects, list_t *to_merge, int accepted_error)
{
   while (to_merge->head)
     {
        list_node_t *node, *parent_node;
        rect_t r1;
        int merged;

        r1 = ((rect_node_t *)to_merge->head)->rect;
        merged = 0;
        parent_node = NULL;
        node = rects->head;
        while (node)
	  {
	     rect_t r2, outer;
	     int area;

	     r2 = ((rect_node_t *)node)->rect;
	     _calc_outer_rect_area(r1, r2, &outer);
	     area = r1.area + r2.area; /* intra area is taken as 0 */
	     if (outer.area - area <= accepted_error)
	       {
		  /* remove both r1 and r2, create r3
		   * actually r3 uses r2 instance, saves memory */
		  rect_node_t *n;

		  n = (rect_node_t *)rect_list_unlink_next(rects, parent_node);
		  n->rect = outer;
		  rect_list_append_node(to_merge, (list_node_t *)n);
		  merged = 1;
		  break;
	       }
	     parent_node = node;
	     node = node->next;
	  }
        if (!merged)
	  {
	     list_node_t *n;
	     n = rect_list_unlink_next(to_merge, NULL);
	     rect_list_append_node(rects, n);
	  }
	else
	  rect_list_del_next(to_merge, NULL);
    }
}

static inline void
rect_list_add_split_fuzzy_and_merge(list_t *rects,
                                    list_node_t *node,
                                    int split_accepted_error,
                                    int merge_accepted_error)
{
   list_node_t *n;

   /**
    * First, add the new 'node' to the 'rects' list. This process might
    * split existing rectangles in 'rects' or merge 'node' with some of them.
    * 'rect_list_add_split_fuzzy' returns a pointer to the node in 'rects'
    * that was the tail of the list *before* any new nodes (resulting from
    * splits of existing rects or the addition of 'node' itself if it wasn't merged)
    * were added *after* it during its processing.
    *
    * Essentially, 'n' marks a boundary: nodes up to 'n' (inclusive) are
    * considered relatively stable or already processed against each other
    * to some extent. Nodes after 'n' are newer additions or results of
    * recent splits/merges triggered by 'node'.
    */
   n = rect_list_add_split_fuzzy(rects, node, split_accepted_error);

   /**
    * If 'n' exists and has a 'next' node, it means that rect_list_add_split_fuzzy
    * potentially added new rectangles after 'n' (or 'n' itself was placed such
    * that there are subsequent nodes). These "newer" rectangles (from n->next
    * to the current tail of 'rects') might be candidates for further merging
    * amongst themselves or with the "older" part of the list (up to 'n').
    */
   if (n && n->next)
     {
        list_t to_merge;
        /* split list into 2 segments, already merged and to merge */
        to_merge.head = n->next;
        to_merge.tail = rects->tail;
        rects->tail = n;
        n->next = NULL;
        // Now, merge the 'to_merge' list (which contains all nodes originally after 'n')
        // back into the 'rects' list (which now ends at 'n').
        // This allows further consolidation.
        rect_list_merge_rects(rects, &to_merge, merge_accepted_error);
     }
}

/**
 * @internal
 * @brief Adds a redraw rectangle to the list, performing fuzzy splitting and merging.
 * This is a convenience wrapper around rect_list_add_split_fuzzy_and_merge.
 * It allocates a new rectangle node, initializes it, and then calls the
 * main fuzzy add/merge logic.
 *
 * @param rects The list of rectangles.
 * @param x The x-coordinate of the redraw rectangle.
 * @param y The y-coordinate of the redraw rectangle.
 * @param w The width of the redraw rectangle.
 * @param h The height of the redraw rectangle.
 * @param fuzz The accepted error/tolerance for fuzzy splitting and merging.
 *             This value is passed as both split_accepted_error and
 *             merge_accepted_error to rect_list_add_split_fuzzy_and_merge.
 * @return Always returns 1 (historically, perhaps to indicate success).
 */
static inline int
_add_redraw(list_t *rects, int x, int y, int w, int h, int fuzz)
{
   rect_node_t *rn;
   rn = (rect_node_t *)rect_list_node_pool_get();
   rn->_lst = list_node_zeroed;
   rect_init(&rn->rect, x, y, w, h);
   rect_list_add_split_fuzzy_and_merge(rects, (list_node_t *)rn, fuzz, fuzz);
   return 1;
}

/////////////////////////////////////////////////////////////////

/**
 * @brief Initializes the tilebuffer system.
 * @note Currently a no-op in this implementation.
 */
EVAS_API void
evas_common_tilebuf_init(void)
{
}

/**
 * @brief Creates a new tilebuffer.
 * @param w The width of the output buffer associated with this tilebuffer.
 * @param h The height of the output buffer.
 * @return A pointer to the newly created Tilebuf, or NULL on allocation failure.
 *         The tilebuffer is initialized with a default tile size (e.g., 8x8)
 *         and dimensions for the output buffer.
 */
EVAS_API Tilebuf *
evas_common_tilebuf_new(int w, int h)
{
   Tilebuf *tb;

   tb = calloc(1, sizeof(Tilebuf));
   if (!tb) return NULL;
   tb->tile_size.w = 8;
   tb->tile_size.h = 8;
   tb->outbuf_w = w;
   tb->outbuf_h = h;
   return tb;
}

/**
 * @brief Frees a tilebuffer and its associated resources.
 * This includes clearing any stored rectangle lists and flushing the node pool.
 * @param tb The tilebuffer to free.
 */
EVAS_API void
evas_common_tilebuf_free(Tilebuf *tb)
{
   rect_list_clear(&tb->rects);
   rect_list_node_pool_flush();
   free(tb);
}

/**
 * @brief Sets the tile size for the tilebuffer.
 * This size is used when generating render rectangles, to align them to tile boundaries.
 * @param tb The tilebuffer.
 * @param tw The desired tile width.
 * @param th The desired tile height.
 */
EVAS_API void
evas_common_tilebuf_set_tile_size(Tilebuf *tb, int tw, int th)
{
   tb->tile_size.w = tw;
   tb->tile_size.h = th;
}

/**
 * @brief Gets the current tile size of the tilebuffer.
 * @param tb The tilebuffer.
 * @param[out] tw Pointer to store the tile width. Can be NULL.
 * @param[out] th Pointer to store the tile height. Can be NULL.
 */
EVAS_API void
evas_common_tilebuf_get_tile_size(Tilebuf *tb, int *tw, int *th)
{
   if (tw) *tw = tb->tile_size.w;
   if (th) *th = tb->tile_size.h;
}

/**
 * @brief Sets whether strict tile alignment is used.
 * @param tb The tilebuffer.
 * @param strict If EINA_TRUE, strict tile alignment is enforced.
 *               If EINA_FALSE, fuzzy merging might be used.
 *               (Note: current get_render_rects seems to always use fuzzy merging logic).
 */
EVAS_API void
evas_common_tilebuf_tile_strict_set(Tilebuf *tb, Eina_Bool strict)
{
   tb->strict_tiles = strict;
}

/**
 * @brief Adds a rectangle to the set of redraw areas for the tilebuffer.
 * The rectangle is clipped to the tilebuffer's output dimensions.
 * An optimization prevents re-adding the exact same rectangle consecutively.
 * The actual addition uses a fuzzy logic (_add_redraw) to merge or split
 * rectangles to maintain an optimized list.
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the rectangle.
 * @param y The y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return 1 if the rectangle was added (or considered, even if optimized out),
 *         0 if the input rectangle had non-positive width or height before or after clipping.
 */
EVAS_API int
evas_common_tilebuf_add_redraw(Tilebuf *tb, int x, int y, int w, int h)
{
   if ((w <= 0) || (h <= 0)) return 0;
   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, tb->outbuf_w, tb->outbuf_h);
   if ((w <= 0) || (h <= 0)) return 0;
   // optimize a common case -> adding the exact same rect 2x in a row
   if ((tb->prev_add.x == x) && (tb->prev_add.y == y) &&
       (tb->prev_add.w == w) && (tb->prev_add.h == h)) return 1;
   tb->prev_add.x = x; tb->prev_add.y = y;
   tb->prev_add.w = w; tb->prev_add.h = h;
   tb->prev_del.w = 0; tb->prev_del.h = 0;
   return _add_redraw(&tb->rects, x, y, w, h, FUZZ * FUZZ);
}

/**
 * @brief Deletes a rectangle from the set of redraw areas.
 * The specified rectangle is subtracted from the existing redraw regions.
 * This may involve splitting existing rectangles.
 * An optimization prevents re-deleting the exact same rectangle consecutively.
 * Sets a flag indicating that the rectangle list might need merging.
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the rectangle to delete.
 * @param y The y-coordinate of the rectangle to delete.
 * @param w The width of the rectangle to delete.
 * @param h The height of the rectangle to delete.
 * @return 0 if the operation was processed. (Note: The return value might not
 *         clearly indicate success/failure in all cases, e.g. if list was empty).
 *         Returns 1 if the exact same rectangle was just deleted (optimization).
 */
EVAS_API int
evas_common_tilebuf_del_redraw(Tilebuf *tb, int x, int y, int w, int h)
{
   rect_t r;

   if (!tb->rects.head) return 0;
   if ((w <= 0) || (h <= 0)) return 0;
   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, tb->outbuf_w, tb->outbuf_h);
   if ((w <= 0) || (h <= 0)) return 0;
   // optimize a common case -> deleting the exact same rect 2x in a row
   if ((tb->prev_del.x == x) && (tb->prev_del.y == y) &&
       (tb->prev_del.w == w) && (tb->prev_del.h == h)) return 1;
   tb->prev_del.x = x; tb->prev_del.y = y;
   tb->prev_del.w = w; tb->prev_del.h = h;
   tb->prev_add.w = 0; tb->prev_add.h = 0;
   rect_init(&r, x, y, w, h);
   rect_list_del_split_strict(&tb->rects, r);
   tb->need_merge = 1;
   return 0;
}

/**
 * @brief Adds a motion vector to the tilebuffer.
 * @note Currently a no-op and always returns 0 in this implementation.
 * This function is intended for hinting areas that have moved, potentially
 * for optimized rendering (e.g., blitting).
 * @param tb The tilebuffer.
 * @param x The x-coordinate of the source area.
 * @param y The y-coordinate of the source area.
 * @param w The width of the area.
 * @param h The height of the area.
 * @param dx The horizontal displacement (motion).
 * @param dy The vertical displacement (motion).
 * @param alpha Alpha value for blending (likely unused or for future use).
 * @return Always returns 0 in this implementation.
 */
EVAS_API int
evas_common_tilebuf_add_motion_vector(Tilebuf *tb EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dx EINA_UNUSED, int dy EINA_UNUSED, int alpha EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Clears all redraw rectangles from the tilebuffer.
 * Resets the list of redraw areas and clears previous add/delete caches.
 * @param tb The tilebuffer to clear.
 */
EVAS_API void
evas_common_tilebuf_clear(Tilebuf *tb)
{
   tb->prev_add.x = tb->prev_add.y = tb->prev_add.w = tb->prev_add.h = 0;
   tb->prev_del.x = tb->prev_del.y = tb->prev_del.w = tb->prev_del.h = 0;
   rect_list_clear(&tb->rects);
   tb->need_merge = 0;
}

/**
 * @brief Retrieves the list of rectangles that need to be rendered.
 *
 * This function performs several steps:
 * 1. Alignment: It iterates through the current redraw rectangles in `tb->rects`,
 *    aligns each to the tile boundaries defined by `tb->tile_size`.
 *    These aligned rectangles are added to a temporary `to_merge` list using
 *    `_add_redraw` with zero fuzz, which implies strict merging of adjacent/overlapping
 *    tile-aligned blocks.
 * 2. Consolidation: The original `tb->rects` is cleared, and the content of
 *    `to_merge` (now tile-aligned and somewhat merged) is merged back into
 *    `tb->rects` (again with zero fuzz for strict merging).
 * 3. Clipping and Bounding Box: The function then iterates through the consolidated
 *    `tb->rects`, clips each rectangle to the output buffer dimensions, and
 *    calculates the overall bounding box of all valid rectangles.
 * 4. Output Generation:
 *    - If the number of resulting rectangles (`num`) is greater than `MAXREG` (24),
 *      a single `Tilebuf_Rect` representing the calculated bounding box is returned.
 *    - Otherwise, an array of `Tilebuf_Rect` is allocated, and each valid, clipped
 *      rectangle from `tb->rects` is converted into a `Tilebuf_Rect` and added
 *      to an Eina_Inlist (using the allocated array as backing storage).
 *
 * The returned `Tilebuf_Rect*` is the head of an Eina_Inlist. Each element in the
 * list is a `Tilebuf_Rect`.
 * Example of returned structure (if num <= MAXREG):
 *   rects -> Tilebuf_Rect[0] {x, y, w, h, EINA_INLIST links to Tilebuf_Rect[1]}
 *            Tilebuf_Rect[1] {x, y, w, h, EINA_INLIST links to Tilebuf_Rect[2]}
 *            ...
 *            Tilebuf_Rect[num-1] {x, y, w, h, EINA_INLIST links to NULL}
 * The actual memory is a single block `rbuf = malloc(sizeof(Tilebuf_Rect) * num)`.
 * If num > MAXREG, then:
 *   rects -> dynamically allocated Tilebuf_Rect {bx1, by1, bx2-bx1, by2-by1, EINA_INLIST links to NULL}
 *
 * @param tb The tilebuffer.
 * @return A pointer to the head of an Eina_Inlist of Tilebuf_Rects to be rendered.
 *         The caller is responsible for freeing this list using
 *         evas_common_tilebuf_free_render_rects().
 *         Returns NULL if there are no rectangles to render after processing.
 */
EVAS_API Tilebuf_Rect *
evas_common_tilebuf_get_render_rects(Tilebuf *tb)
{
   list_node_t *n;
   list_t to_merge;
   Tilebuf_Rect *rects = NULL, *rbuf, *r;
   int bx1 = 0, bx2 = 0, by1 = 0, by2 = 0, num = 0, x1, x2, y1, y2, i;

/* don't need this since the below is now always on
   if (tb->need_merge)
     {
        to_merge = tb->rects;
        tb->rects = list_zeroed;
        rect_list_merge_rects(&tb->rects, &to_merge, 0);
        tb->need_merge = 0;
     }
 */
   if (1)
// always fuzz merge for optimal perf
//   if (!tb->strict_tiles)
     {
        // round up rects to tb->tile_size.w and tb->tile_size.h
        to_merge = list_zeroed;
        for (n = tb->rects.head; n; n = n->next)
          {
             x1 = ((rect_node_t *)n)->rect.left;
             x2 = x1 + ((rect_node_t *)n)->rect.width;
             y1 = ((rect_node_t *)n)->rect.top;
             y2 = y1 + ((rect_node_t *)n)->rect.height;
             x1 = tb->tile_size.w * (x1 / tb->tile_size.w);
             y1 = tb->tile_size.h * (y1 / tb->tile_size.h);
             x2 = tb->tile_size.w * ((x2 + tb->tile_size.w - 1) / tb->tile_size.w);
             y2 = tb->tile_size.h * ((y2 + tb->tile_size.h - 1) / tb->tile_size.h);
             _add_redraw(&to_merge, x1, y1, x2 - x1, y2 - y1, 0);
          }
        rect_list_clear(&tb->rects);
        rect_list_merge_rects(&tb->rects, &to_merge, 0);
     }
   n = tb->rects.head;
   if (n)
     {
        RECTS_CLIP_TO_RECT(((rect_node_t *)n)->rect.left,
                           ((rect_node_t *)n)->rect.top,
                           ((rect_node_t *)n)->rect.width,
                           ((rect_node_t *)n)->rect.height,
                           0, 0, tb->outbuf_w, tb->outbuf_h);
        num = 1;
        bx1 = ((rect_node_t *)n)->rect.left;
        bx2 = bx1 + ((rect_node_t *)n)->rect.width;
        by1 = ((rect_node_t *)n)->rect.top;
        by2 = by1 + ((rect_node_t *)n)->rect.height;
        n = n->next;
        for (; n; n = n->next)
          {
             RECTS_CLIP_TO_RECT(((rect_node_t *)n)->rect.left,
                                ((rect_node_t *)n)->rect.top,
                                ((rect_node_t *)n)->rect.width,
                                ((rect_node_t *)n)->rect.height,
                                0, 0, tb->outbuf_w, tb->outbuf_h);
             x1 = ((rect_node_t *)n)->rect.left;
             if (x1 < bx1) bx1 = x1;
             x2 = x1 + ((rect_node_t *)n)->rect.width;
             if (x2 > bx2) bx2 = x2;

             y1 = ((rect_node_t *)n)->rect.top;
             if (y1 < by1) by1 = y1;
             y2 = y1 + ((rect_node_t *)n)->rect.height;
             if (y2 > by2) by2 = y2;
             num++;
          }
     }
   else
     return NULL;

   /* magic number - if we have > MAXREG regions to update, take bounding */
   if (num > MAXREG)
     {
        r = malloc(sizeof(Tilebuf_Rect));
        if (r)
          {
             EINA_INLIST_GET(r)->next = NULL;
             EINA_INLIST_GET(r)->prev = NULL;
             EINA_INLIST_GET(r)->last = NULL;
             r->x = bx1;
             r->y = by1;
             r->w = bx2 - bx1;
             r->h = by2 - by1;
             rects = (Tilebuf_Rect *)
               eina_inlist_append(EINA_INLIST_GET(rects),
                                  EINA_INLIST_GET(r));
          }
        return rects;
     }

   rbuf = malloc(sizeof(Tilebuf_Rect) * num);
   if (!rbuf) return NULL;

   for (i = 0, n = tb->rects.head; n; n = n->next)
     {
        rect_t cur;

        cur = ((rect_node_t *)n)->rect;
        if ((cur.width > 0) && (cur.height > 0))
          {
             r = &(rbuf[i]);
             EINA_INLIST_GET(r)->next = NULL;
             EINA_INLIST_GET(r)->prev = NULL;
             EINA_INLIST_GET(r)->last = NULL;
             r->x = cur.left;
             r->y = cur.top;
             r->w = cur.width;
             r->h = cur.height;
             rects = (Tilebuf_Rect *)
               eina_inlist_append(EINA_INLIST_GET(rects),
                                  EINA_INLIST_GET(r));
             i++;
          }
     }

   // It is possible that due to the clipping we do not return any rectangle here.
   if (!rects) free(rbuf);

   return rects;
}

/**
 * @brief Frees the render rectangles list obtained from evas_common_tilebuf_get_render_rects().
 * @param rects The pointer to the Tilebuf_Rect list (which is the head of an Eina_Inlist,
 *              but the memory itself is a contiguous block or a single allocation).
 */
EVAS_API void
evas_common_tilebuf_free_render_rects(Tilebuf_Rect *rects)
{
   free(rects);
}
#endif
