#define EFL_CANVAS_GROUP_PROTECTED

#include "evas_common_private.h"
#include "evas_private.h"
#include <errno.h>

#define MY_CLASS EVAS_TABLE_CLASS

#define MY_CLASS_NAME "Evas_Table"
#define MY_CLASS_NAME_LEGACY "Evas_Object_Table"

typedef struct _Evas_Table_Data              Evas_Table_Data;
/**
 * @brief Represents the options for a child object within a table.
 *
 * This structure holds all the packing and layout properties for an
 * Evas_Object that has been added to an Evas_Table.
 */
typedef struct _Evas_Object_Table_Option     Evas_Object_Table_Option;
/**
 * @brief Caches layout calculation results for a table.
 *
 * This structure is used to store intermediate and final results of
 * table layout calculations, such as cell sizes, weights, and expansion flags.
 * This helps to optimize recalculations when the table or its children change.
 */
typedef struct _Evas_Object_Table_Cache      Evas_Object_Table_Cache;
typedef struct _Evas_Object_Table_Iterator   Evas_Object_Table_Iterator;
typedef struct _Evas_Object_Table_Accessor   Evas_Object_Table_Accessor;

struct _Evas_Object_Table_Option
{
   Evas_Object *obj; /**< The child object. */
   unsigned short col, row, colspan, rowspan, end_col, end_row; /**< Column, row, column span, row span, end column, and end row of the child. */
   struct {
      Evas_Coord w, h; /**< Minimum and maximum width/height hints for the child. */
   } min, max;
   struct {
      double h, v; /**< Horizontal and vertical alignment hints for the child. */
   } align;
   struct {
      Evas_Coord l, r, t, b; /**< Left, right, top, and bottom padding hints for the child. */
   } pad;
   Eina_Bool expand_h : 1; /**< Horizontal expansion flag. @deprecated XXX required? */
   Eina_Bool expand_v : 1; /**< Vertical expansion flag. @deprecated XXX required? */
   Eina_Bool fill_h : 1; /**< Horizontal fill flag. If true, the object will fill available horizontal space within its cell. */
   Eina_Bool fill_v : 1; /**< Vertical fill flag. If true, the object will fill available vertical space within its cell. */
};

struct _Evas_Object_Table_Cache
{
   int ref; /**< Reference count for the cache. */
   struct {
      struct {
         double h, v; /**< Total horizontal and vertical weights of all columns/rows. */
      } weights;
      struct {
         int h, v; /**< Total number of horizontally and vertically expanding columns/rows. */
      } expands;
      struct {
         Evas_Coord w, h; /**< Total minimum width and height of the table content. */
      } min;
   } total; /**< Aggregated values for the entire table. */
   struct {
      double *h, *v; /**< Array of horizontal (column) and vertical (row) weights. Example: `h = {0.2, 0.5, 0.3}` for 3 columns. */
   } weights;
   struct {
      Evas_Coord *h, *v; /**< Array of calculated horizontal (column) and vertical (row) sizes. Example: `h = {100, 200, 150}` for 3 columns with specific pixel widths. */
   } sizes;
   struct {
      Eina_Bool *h, *v; /**< Array of horizontal (column) and vertical (row) expansion flags. Example: `h = {EINA_TRUE, EINA_FALSE, EINA_TRUE}` for 3 columns. */
   } expands;
   double ___pad; // padding to make sure doubles at end can be aligned
};

/**
 * @brief Private data for an Evas_Table object.
 *
 * This structure holds all the internal state of an Evas_Table,
 * including its children, padding, alignment, size, layout cache,
 * and various mode flags.
 */
struct _Evas_Table_Data
{
   Eina_List *children; /**< List of Evas_Object_Table_Option, representing packed children. */
   struct {
      Evas_Coord h, v; /**< Horizontal and vertical padding between cells. */
   } pad;
   struct {
      double h, v; /**< Horizontal and vertical alignment of the entire table content within its allocated space. */
   } align;
   struct {
      int cols, rows; /**< Number of columns and rows in the table. */
   } size;
   Evas_Object_Table_Cache *cache; /**< Pointer to the layout cache. */
   Evas_Object_Table_Homogeneous_Mode homogeneous; /**< Homogeneous mode setting. */
   Eina_Bool hints_changed : 1; /**< Flag indicating if size hints of children have changed, requiring recalculation. */
   Eina_Bool expand_h : 1; /**< Flag indicating if the table has any horizontally expanding children (homogeneous mode). */
   Eina_Bool expand_v : 1; /**< Flag indicating if the table has any vertically expanding children (homogeneous mode). */
   Eina_Bool is_mirrored : 1; /**< Flag indicating if the table layout is mirrored (for RTL languages). */
};

/**
 * @brief Iterator for traversing children of an Evas_Table.
 */
struct _Evas_Object_Table_Iterator
{
   Eina_Iterator iterator; /**< Base Eina_Iterator structure. */

   Eina_Iterator *real_iterator; /**< The underlying list iterator for children. */
   const Evas_Object *table; /**< The table object being iterated. */
};

/**
 * @brief Accessor for children of an Evas_Table.
 */
struct _Evas_Object_Table_Accessor
{
   Eina_Accessor accessor; /**< Base Eina_Accessor structure. */

   Eina_Accessor *real_accessor; /**< The underlying list accessor for children. */
   const Evas_Object *table; /**< The table object being accessed. */
};

#define EVAS_OBJECT_TABLE_DATA_GET(o, ptr)                              \
   Evas_Table_Data *ptr = efl_data_scope_get(o, MY_CLASS)

#define EVAS_OBJECT_TABLE_DATA_GET_OR_RETURN(o, ptr)                    \
   EVAS_OBJECT_TABLE_DATA_GET(o, ptr);                                  \
if (!ptr)                                                               \
{                                                                       \
   ERR("No widget data for object %p (%s)",                            \
        o, evas_object_type_get(o));                                    \
   return;                                                              \
}

#define EVAS_OBJECT_TABLE_DATA_GET_OR_RETURN_VAL(o, ptr, val)           \
   EVAS_OBJECT_TABLE_DATA_GET(o, ptr);                                  \
if (!ptr)                                                               \
{                                                                       \
   ERR("No widget data for object %p (%s)",                            \
        o, evas_object_type_get(o));                                    \
   return val;                                                          \
}

static const char EVAS_OBJECT_TABLE_OPTION_KEY[] = "|EvTb"; /**< Key used to store Evas_Object_Table_Option data on a child object. */

/**
 * @brief Advances the table iterator to the next child object.
 * @param it The table iterator.
 * @param data Pointer to store the next child object.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_object_table_iterator_next(Evas_Object_Table_Iterator *it, void **data)
{
   Evas_Object_Table_Option *opt;

   if (!eina_iterator_next(it->real_iterator, (void **)&opt))
     return EINA_FALSE;
   if (data) *data = opt->obj;
   return EINA_TRUE;
}

/**
 * @brief Gets the container (table) of the iterator.
 * @param it The table iterator.
 * @return The table object.
 */
static Evas_Object *
_evas_object_table_iterator_get_container(Evas_Object_Table_Iterator *it)
{
   return (Evas_Object *)it->table;
}

/**
 * @brief Frees the table iterator.
 * @param it The table iterator to free.
 */
static void
_evas_object_table_iterator_free(Evas_Object_Table_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   free(it);
}

/**
 * @brief Gets the child object at a specific index using the accessor.
 * @param it The table accessor.
 * @param idx The index of the child object.
 * @param data Pointer to store the child object.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_object_table_accessor_get_at(Evas_Object_Table_Accessor *it, unsigned int idx, void **data)
{
   Evas_Object_Table_Option *opt = NULL;

   if (!eina_accessor_data_get(it->real_accessor, idx, (void **)&opt))
     return EINA_FALSE;
   if (data) *data = opt->obj;
   return EINA_TRUE;
}

/**
 * @brief Gets the container (table) of the accessor.
 * @param it The table accessor.
 * @return The table object.
 */
static Evas_Object *
_evas_object_table_accessor_get_container(Evas_Object_Table_Accessor *it)
{
   return (Evas_Object *)it->table;
}

/**
 * @brief Frees the table accessor.
 * @param it The table accessor to free.
 */
static void
_evas_object_table_accessor_free(Evas_Object_Table_Accessor *it)
{
   eina_accessor_free(it->real_accessor);
   free(it);
}

/**
 * @brief Allocates and initializes a new table cache.
 * @param cols Number of columns.
 * @param rows Number of rows.
 * @return A new Evas_Object_Table_Cache instance or NULL on failure.
 *
 * The cache structure is allocated as a single block of memory.
 * Pointers within the cache (weights, sizes, expands) are set up to
 * point to appropriate offsets within this block.
 */
static Evas_Object_Table_Cache *
_evas_object_table_cache_alloc(int cols, int rows)
{
   Evas_Object_Table_Cache *cache;
   int size;

   size = sizeof(Evas_Object_Table_Cache) +
      ((cols + rows) *
          (sizeof(double) + sizeof(Evas_Coord) + sizeof(Eina_Bool)));
   cache = malloc(size);
   if (!cache)
     {
        ERR("Could not allocate table cache %dx%d (%d bytes): %s",
            cols, rows, size, strerror(errno));
        return NULL;
     }

   cache->ref = 1;
   cache->weights.h = (double *)(cache + 1);
   cache->weights.v = (double *)(cache->weights.h + cols);
   cache->sizes.h = (Evas_Coord *)(cache->weights.v + rows);
   cache->sizes.v = (Evas_Coord *)(cache->sizes.h + cols);
   cache->expands.h = (Eina_Bool *)(cache->sizes.v + rows);
   cache->expands.v = (Eina_Bool *)(cache->expands.h + cols);

   return cache;
}

/**
 * @brief Decrements the reference count of a table cache and frees it if the count reaches zero.
 * @param cache The table cache to release.
 */
static void
_evas_object_table_cache_free(Evas_Object_Table_Cache *cache)
{
   cache->ref--;
   if (cache->ref == 0) free(cache);
}

/**
 * @brief Resets the values in an existing table cache to their defaults.
 * @param priv The private data of the table.
 *
 * This function clears calculated totals and zeros out the arrays for
 * weights, sizes, and expands within the cache.
 */
static void
_evas_object_table_cache_reset(Evas_Table_Data *priv)
{
   Evas_Object_Table_Cache *c = priv->cache;
   int size;

   if (!c) return;
   c->total.expands.v = 0;
   c->total.expands.h = 0;
   c->total.min.w = 0;
   c->total.min.h = 0;

   size = ((priv->size.rows + priv->size.cols) *
           (sizeof(double) + sizeof(Evas_Coord) + sizeof(Eina_Bool)));
   memset(c + 1, 0, size);
}

/**
 * @brief Invalidates the table cache.
 * @param priv The private data of the table.
 *
 * This marks hints as changed and frees the existing cache,
 * forcing a recalculation on the next layout pass.
 */
static void
_evas_object_table_cache_invalidate(Evas_Table_Data *priv)
{
   priv->hints_changed = 1;
   if (priv->cache)
     {
        _evas_object_table_cache_free(priv->cache);
        priv->cache = NULL;
     }
}

/**
 * @brief Retrieves the Evas_Object_Table_Option associated with a child object.
 * @param o The child object.
 * @return The Evas_Object_Table_Option for the child, or NULL if not found.
 */
static Evas_Object_Table_Option *
_evas_object_table_option_get(Evas_Object *o)
{
   return evas_object_data_get(o, EVAS_OBJECT_TABLE_OPTION_KEY);
}

/**
 * @brief Associates an Evas_Object_Table_Option with a child object.
 * @param o The child object.
 * @param opt The Evas_Object_Table_Option to set.
 */
static void
_evas_object_table_option_set(Evas_Object *o, const Evas_Object_Table_Option *opt)
{
   evas_object_data_set(o, EVAS_OBJECT_TABLE_OPTION_KEY, opt);
}

/**
 * @brief Removes and returns the Evas_Object_Table_Option associated with a child object.
 * @param o The child object.
 * @return The removed Evas_Object_Table_Option, or NULL if not found.
 */
static Evas_Object_Table_Option *
_evas_object_table_option_del(Evas_Object *o)
{
   return evas_object_data_del(o, EVAS_OBJECT_TABLE_OPTION_KEY);
}

/**
 * @brief Callback for when a child object is invalidated (e.g., deleted).
 * @param data The table object (user data).
 * @param event The Efl_Event details.
 *
 * This function unpacks the child from the table.
 */
static void
_on_child_invalidate(void *data, const Efl_Event *event)
{
   Evas_Object *table = data;
   evas_object_table_unpack(table, event->object);
}

/**
 * @brief Callback for when a child object's size hints change.
 * @param data The table object (user data).
 * @param event The Efl_Event details (unused).
 *
 * This function invalidates the table's layout cache and triggers a smart recalculation.
 */
static void
_on_child_hints_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Evas_Object *table = data;
   EVAS_OBJECT_TABLE_DATA_GET_OR_RETURN(table, priv);
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(table);
}

EFL_CALLBACKS_ARRAY_DEFINE(evas_object_table_callbacks,
  { EFL_EVENT_INVALIDATE, _on_child_invalidate },
  { EFL_GFX_ENTITY_EVENT_HINTS_CHANGED, _on_child_hints_changed }
);

/**
 * @brief Connects event callbacks to a child object when it's added to the table.
 * @param o The table object.
 * @param child The child object being added.
 */
static void
_evas_object_table_child_connect(Evas_Object *o, Evas_Object *child)
{
   efl_event_callback_array_add(child, evas_object_table_callbacks(), o);
}

/**
 * @brief Disconnects event callbacks from a child object when it's removed from the table.
 * @param o The table object.
 * @param child The child object being removed.
 */
static void
_evas_object_table_child_disconnect(Evas_Object *o, Evas_Object *child)
{
   efl_event_callback_array_del(child, evas_object_table_callbacks(), o);
}

/**
 * @brief Calculates the final geometry of a child object within its allocated cell space.
 * @param opt The packing options for the child.
 * @param x Pointer to the cell's X coordinate (input/output).
 * @param y Pointer to the cell's Y coordinate (input/output).
 * @param w Pointer to the cell's width (input/output).
 * @param h Pointer to the cell's height (input/output).
 *
 * This function applies padding, alignment, min/max constraints, and fill flags
 * to determine the child's actual position and size.
 */
static void
_evas_object_table_calculate_cell(const Evas_Object_Table_Option *opt, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord cw, ch;

   *w -= opt->pad.l + opt->pad.r;
   if (*w < opt->min.w)
     cw = opt->min.w;
   else if ((opt->max.w > -1) && (*w > opt->max.w))
     cw = opt->max.w;
   else if (opt->fill_h)
     cw = *w;
   else
     cw = opt->min.w;

   *h -= opt->pad.t + opt->pad.b;
   if (*h < opt->min.h)
     ch = opt->min.h;
   else if ((opt->max.h > -1) && (*h > opt->max.h))
     ch = opt->max.h;
   else if (opt->fill_v)
     ch = *h;
   else
     ch = opt->min.h;

   *x += opt->pad.l;
   if (cw != *w)
     {
        *x += (*w - cw) * opt->align.h;
        *w = cw;
     }

   *y += opt->pad.t;
   if (ch != *h)
     {
        *y += (*h - ch) * opt->align.v;
        *h = ch;
     }
}

/**
 * @brief Calculates size hints for the table when in homogeneous mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * In homogeneous mode, all cells are assumed to have the same size.
 * This function iterates over children to determine the maximum minimum cell
 * size required and whether the table should expand.
 */
static void
_evas_object_table_calculate_hints_homogeneous(Evas_Object *o, Evas_Table_Data *priv)
{
   Eina_List *l;
   Evas_Object_Table_Option *opt;
   Evas_Coord minw, minh, o_minw, o_minh;
   Eina_Bool expand_h, expand_v;

   o_minw = 0;
   o_minh = 0;
   minw = 0;
   minh = 0;
   expand_h = 0;
   expand_v = 0;

   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Object *child = opt->obj;
        Evas_Coord child_minw, child_minh, cell_minw, cell_minh;
        double weightw, weighth;

        evas_object_size_hint_combined_min_get(child, &opt->min.w, &opt->min.h);
        evas_object_size_hint_max_get(child, &opt->max.w, &opt->max.h);
        evas_object_size_hint_padding_get
           (child, &opt->pad.l, &opt->pad.r, &opt->pad.t, &opt->pad.b);
        evas_object_size_hint_align_get(child, &opt->align.h, &opt->align.v);
        evas_object_size_hint_weight_get(child, &weightw, &weighth);

        child_minw = opt->min.w + opt->pad.l + opt->pad.r;
        child_minh = opt->min.h + opt->pad.t + opt->pad.b;

        cell_minw = (child_minw + opt->colspan - 1) / opt->colspan;
        cell_minh = (child_minh + opt->rowspan - 1) / opt->rowspan;

        opt->expand_h = 0;
        if ((weightw > 0.0) &&
            ((opt->max.w < 0) ||
             ((opt->max.w > -1) && (opt->min.w < opt->max.w))))
          {
             opt->expand_h = 1;
             expand_h = 1;
          }

        opt->expand_v = 0;
        if ((weighth > 0.0) &&
            ((opt->max.h < 0) ||
             ((opt->max.h > -1) && (opt->min.h < opt->max.h))))
          {
             opt->expand_v = 1;
             expand_v = 1;
          }

        opt->fill_h = 0;
        if (opt->align.h < 0.0)
          {
             opt->align.h = 0.5;
             opt->fill_h = 1;
          }
        opt->fill_v = 0;
        if (opt->align.v < 0.0)
          {
             opt->align.v = 0.5;
             opt->fill_v = 1;
          }

        /* greatest mininum values, with paddings */
        if (minw < cell_minw)
          minw = cell_minw;
        if (minh < cell_minh)
          minh = cell_minh;
        /* greatest mininum values, without paddings */
        if (o_minw < opt->min.w)
          o_minw = opt->min.w;
        if (o_minh < opt->min.h)
          o_minh = opt->min.h;
     }

   if (priv->homogeneous == EVAS_OBJECT_TABLE_HOMOGENEOUS_ITEM)
     {
        if (o_minw < 1)
          {
             ERR("homogeneous table based on item size but no "
                 "horizontal minimum size specified! Using expand.");
             expand_h = 1;
          }
        if (o_minh < 1)
          {
             ERR("homogeneous table based on item size but no "
                 "vertical minimum size specified! Using expand.");
             expand_v = 1;
          }
     }

   minw = priv->size.cols * (minw + priv->pad.h) - priv->pad.h;
   minh = priv->size.rows * (minh + priv->pad.v) - priv->pad.v;

   priv->hints_changed = 0;
   priv->expand_h = expand_h;
   priv->expand_v = expand_v;

   if ((minw > 0 ) || (minh > 0))
     evas_object_size_hint_min_set(o, minw, minh);

   // XXX hint max?
}

/**
 * @brief Adjusts the table's overall size for homogeneous layout based on item hints.
 * @param o The table object.
 * @param priv The private data of the table.
 * @param x Pointer to the table's X coordinate (input/output).
 * @param y Pointer to the table's Y coordinate (input/output).
 * @param w Pointer to the table's width (input/output).
 * @param h Pointer to the table's height (input/output).
 *
 * If the table is smaller than its minimum hinted size and not set to expand,
 * its content area is shrunk and aligned according to the table's alignment properties.
 * This is specifically for EVAS_OBJECT_TABLE_HOMOGENEOUS_ITEM mode.
 */
static void
_evas_object_table_calculate_layout_homogeneous_sizes_item(const Evas_Object *o, const Evas_Table_Data *priv, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord minw, minh;
   Eina_Bool expand_h, expand_v;

   evas_object_size_hint_combined_min_get(o, &minw, &minh);
   expand_h = priv->expand_h;
   expand_v = priv->expand_v;

   if (*w < minw)
     expand_h = 0;
   if (!expand_h)
     {
        *x += (*w - minw) * priv->align.h;
        *w = minw;
     }

   if (*h < minh)
     expand_v = 0;
   if (!expand_v)
     {
        *y += (*h - minh) * priv->align.v;
        *h = minh;
     }
}

/**
 * @brief Calculates the overall content area and individual cell size for homogeneous layout.
 * @param o The table object.
 * @param priv The private data of the table.
 * @param x Pointer to store the content area's X coordinate.
 * @param y Pointer to store the content area's Y coordinate.
 * @param w Pointer to store the content area's width.
 * @param h Pointer to store the content area's height.
 * @param cellw Pointer to store the calculated cell width.
 * @param cellh Pointer to store the calculated cell height.
 *
 * This function gets the table's geometry and, if in HOMOGENEOUS_ITEM mode,
 * adjusts the content area size. Then, it calculates the uniform cell width and height.
 */
static void
_evas_object_table_calculate_layout_homogeneous_sizes(const Evas_Object *o, const Evas_Table_Data *priv, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h, Evas_Coord *cellw, Evas_Coord *cellh)
{
   evas_object_geometry_get(o, x, y, w, h);
   if (priv->homogeneous == EVAS_OBJECT_TABLE_HOMOGENEOUS_ITEM)
     _evas_object_table_calculate_layout_homogeneous_sizes_item
       (o, priv, x, y, w, h);

   *cellw = (*w + priv->size.cols - 1) / priv->size.cols;
   *cellh = (*h + priv->size.rows - 1) / priv->size.rows;
}

/**
 * @brief Calculates and applies the layout for all children in homogeneous mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * This function determines the size of each cell (which is uniform in homogeneous mode)
 * and then positions and sizes each child within its respective cell(s),
 * taking into account colspans, rowspans, padding, and alignment.
 */
static void
_evas_object_table_calculate_layout_homogeneous(Evas_Object *o, Evas_Table_Data *priv)
{
   Evas_Coord x = 0, y = 0, w = 0, h = 0, ww, hh, cellw = 0, cellh = 0;
   Eina_List *l;
   Evas_Object_Table_Option *opt;

   _evas_object_table_calculate_layout_homogeneous_sizes
      (o, priv, &x, &y, &w, &h, &cellw, &cellh);

   ww = w - ((priv->size.cols - 1) * priv->pad.h);
   hh = h - ((priv->size.rows - 1) * priv->pad.v);

   if (ww < 0) ww = 0;
   if (hh < 0) hh = 0;

   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Object *child = opt->obj;
        Evas_Coord cx, cy, cw, ch, cox, coy, cow, coh;

        cx = x + ((opt->col * ww) / priv->size.cols);
        cw = x + (((opt->col + opt->colspan) * ww) / priv->size.cols) - cx;
        cw += (opt->colspan - 1) * priv->pad.v;
        cy = y + ((opt->row * hh) / priv->size.rows);
        ch = y + (((opt->row + opt->rowspan) * hh) / priv->size.rows) - cy;
        ch += (opt->rowspan - 1) * priv->pad.h;

        cx += (opt->col) * priv->pad.h;
        cy += (opt->row) * priv->pad.v;

        cox = cx;
        coy = cy;
        cow = cw;
        coh = ch;

        _evas_object_table_calculate_cell(opt, &cx, &cy, &cw, &ch);
        if (cw > cow)
          {
             cx = cox;
             cw = cow;
          }
        if (ch > coh)
          {
             cy = coy;
             ch = coh;
          }

        if (priv->is_mirrored)
          evas_object_geometry_set(opt->obj, x + w - (cx - x + cw), cy, cw, ch);
        else
          evas_object_geometry_set(child, cx, cy, cw, ch);
     }
}

/**
 * @brief Top-level function for calculating layout in homogeneous mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * If hints have changed, it first recalculates them. Then, it proceeds
 * to calculate and apply the layout for children.
 */
static void
_evas_object_table_smart_calculate_homogeneous(Evas_Object *o, Evas_Table_Data *priv)
{
   if (priv->hints_changed)
     _evas_object_table_calculate_hints_homogeneous(o, priv);
   _evas_object_table_calculate_layout_homogeneous(o, priv);
}

/**
 * @brief Counts the number of true (expanding) flags in a range of an Eina_Bool array.
 * @param expands Array of boolean flags. Example: `{EINA_TRUE, EINA_FALSE, EINA_TRUE}`.
 * @param start Starting index of the range (inclusive).
 * @param end Ending index of the range (exclusive).
 * @return The number of true flags in the specified range.
 */
static int
_evas_object_table_count_expands(const Eina_Bool *expands, int start, int end)
{
   const Eina_Bool *itr = expands + start, *itr_end = expands + end;
   int count = 0;

   for (; itr < itr_end; itr++)
     {
        if (*itr)
           count++;
     }

   return count;
}

/**
 * @brief Sums the values in a range of an Evas_Coord array.
 * @param sizes Array of Evas_Coord values. Example: `{10, 20, 30}`.
 * @param start Starting index of the range (inclusive).
 * @param end Ending index of the range (exclusive).
 * @return The sum of Evas_Coord values in the specified range.
 */
static Evas_Coord
_evas_object_table_sum_sizes(const Evas_Coord *sizes, int start, int end)
{
   const Evas_Coord *itr = sizes + start, *itr_end = sizes + end;
   Evas_Coord sum = 0;

   for (; itr < itr_end; itr++)
     sum += *itr;

   return sum;
}

/**
 * @brief Distributes extra space evenly among a range of cells that do not expand.
 * @param sizes Array of cell sizes (input/output). Example: `sizes = {10, 10, 10}`.
 * @param start Starting index of the range.
 * @param end Ending index of the range (exclusive).
 * @param space The total extra space to distribute.
 *
 * This function is used when cells need to accommodate extra space, but none of them
 * are marked to expand (e.g., due to minimum size constraints). The space is
 * divided equally.
 */
static void
_evas_object_table_sizes_calc_noexpand(Evas_Coord *sizes, int start, int end, Evas_Coord space)
{
   Evas_Coord *itr = sizes + start, *itr_end = sizes + end - 1;
   Evas_Coord step;
   int units;

   /* XXX move to fixed point math and spread errors among cells */
   units = end - start;
   step = space / units;
   for (; itr < itr_end; itr++)
     *itr += step;

   *itr += space - step * (units - 1);
}

/**
 * @brief Distributes extra space among a range of cells that are marked to expand.
 * @param sizes Array of cell sizes (input/output). Example: `sizes = {10, 20, 10}`.
 * @param start Starting index of the range.
 * @param end Ending index of the range (exclusive).
 * @param space The total extra space to distribute.
 * @param expands Array of boolean flags indicating if a cell expands. Example: `expands = {EINA_TRUE, EINA_FALSE, EINA_TRUE}`.
 * @param expand_count The number of expanding cells in the range.
 * @param weights Array of cell weights. Example: `weights = {0.5, 0.0, 0.5}`.
 * @param weighttot Total weight of expanding cells in the range.
 *
 * This function distributes `space` among the cells in the given range
 * that are marked by `expands`. If `weighttot` is positive, space is
 * distributed proportionally to `weights`. Otherwise, it's distributed evenly
 * among expanding cells.
 */
static void
_evas_object_table_sizes_calc_expand(Evas_Coord *sizes, int start, int end, Evas_Coord space, const Eina_Bool *expands, int expand_count, double *weights, double weighttot)
{
   Evas_Coord *itr = sizes + start, *itr_end = sizes + end;
   const Eina_Bool *itr_expand = expands + start;
   Evas_Coord step = 0, last_space = 0;
   int total = 0, i = start;

   /* XXX move to fixed point math and spread errors among cells */
   if (weighttot > 0.0)
     {
        step = space / expand_count;
        last_space = space - step * (expand_count - 1);
     }

   for (; itr < itr_end; itr++, itr_expand++, i++)
     {
        if (weighttot <= 0.0)
          {
             if (*itr_expand)
               {
                  expand_count--;
                  if (expand_count > 0)
                     *itr += step;
                  else
                    {
                       *itr += last_space;
                       break;
                    }
               }
          }
        else
          {
             if (*itr_expand)
               {
                  expand_count--;
                  if (expand_count > 0)
                    {
                       step = (weights[i] / weighttot) * space;
                       *itr += step;
                       total += step;
                    }
                  else
                    {
                       *itr += space - total;
                       break;
                    }
               }
          }
     }
}

/**
 * @brief Calculates size hints for the table when in regular (non-homogeneous) mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * This is the core logic for determining the minimum size of the table based on
 * its children's hints. It involves:
 * 1. Caching child hints (min/max sizes, padding, alignment, weight, expand flags).
 * 2. Determining which columns/rows can expand based on children.
 * 3. Iteratively calculating minimum column widths and row heights:
 *    - For each child, if its minimum required span (considering its own min size and padding)
 *      is greater than the current sum of min sizes of columns/rows it spans,
 *      the extra space needed is distributed among those columns/rows.
 *      - If any of those columns/rows can expand, space is distributed according to weights.
 *      - Otherwise, space is distributed evenly.
 * 4. Summing up all column/row minimums and inter-cell padding to get the table's total minimum size.
 * 5. Storing these calculated values in the layout cache.
 */
static void
_evas_object_table_calculate_hints_regular(Evas_Object *o, Evas_Table_Data *priv)
{
   Evas_Object_Table_Option *opt;
   Evas_Object_Table_Cache *c;
   Eina_List *l;
   double totweightw = 0.0, totweighth = 0.0;
   int i;

   if (!priv->cache)
     {
        priv->cache = _evas_object_table_cache_alloc
           (priv->size.cols, priv->size.rows);
        if (!priv->cache)
          return;
     }
   c = priv->cache;
   c->ref++;
   _evas_object_table_cache_reset(priv);

   /* cache interesting data */
   memset(c->expands.h, 1, priv->size.cols * sizeof(Eina_Bool));
   memset(c->expands.v, 1, priv->size.rows * sizeof(Eina_Bool));
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Object *child = opt->obj;
        double weightw, weighth;

        evas_object_size_hint_combined_min_get(child, &opt->min.w, &opt->min.h);
        evas_object_size_hint_max_get(child, &opt->max.w, &opt->max.h);
        evas_object_size_hint_padding_get
           (child, &opt->pad.l, &opt->pad.r, &opt->pad.t, &opt->pad.b);
        evas_object_size_hint_align_get(child, &opt->align.h, &opt->align.v);
        evas_object_size_hint_weight_get(child, &weightw, &weighth);

        opt->expand_h = 0;
        if ((weightw > 0.0) &&
            ((opt->max.w < 0) ||
             ((opt->max.w > -1) && (opt->min.w < opt->max.w))))
          opt->expand_h = 1;

        opt->expand_v = 0;
        if ((weighth > 0.0) &&
            ((opt->max.h < 0) ||
             ((opt->max.h > -1) && (opt->min.h < opt->max.h))))
          opt->expand_v = 1;

        opt->fill_h = 0;
        if (opt->align.h < 0.0)
          {
             opt->align.h = 0.5;
             opt->fill_h = 1;
          }
        opt->fill_v = 0;
        if (opt->align.v < 0.0)
          {
             opt->align.v = 0.5;
             opt->fill_v = 1;
          }

        if (!opt->expand_h)
          memset(c->expands.h + opt->col, 0, opt->colspan * sizeof(Eina_Bool));
        else
          {
             for (i = opt->col; i < opt->col + opt->colspan; i++)
               c->weights.h[i] += (weightw / (double)opt->colspan);
          }
        if (!opt->expand_v)
          memset(c->expands.v + opt->row, 0, opt->rowspan * sizeof(Eina_Bool));
        else
          {
             for (i = opt->row; i < opt->row + opt->rowspan; i++)
                c->weights.v[i] += (weighth / (double)opt->rowspan);
          }
     }
   for (i = 0; i < priv->size.cols; i++) totweightw += c->weights.h[i];
   for (i = 0; i < priv->size.rows; i++) totweighth += c->weights.v[i];

   /* calculate sizes for each row and column */
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Coord tot, need;

        /* handle horizontal */
        tot = _evas_object_table_sum_sizes(c->sizes.h, opt->col, opt->end_col);
        need = opt->min.w + opt->pad.l + opt->pad.r;
        if (tot < need)
          {
             Evas_Coord space = need - tot;
             int count;

             count = _evas_object_table_count_expands
                (c->expands.h, opt->col, opt->end_col);

             if (count > 0)
               _evas_object_table_sizes_calc_expand
                  (c->sizes.h, opt->col, opt->end_col, space,
                   c->expands.h, count, c->weights.h, totweightw);
             else
               _evas_object_table_sizes_calc_noexpand
                  (c->sizes.h, opt->col, opt->end_col, space);
          }

        /* handle vertical */
        tot = _evas_object_table_sum_sizes(c->sizes.v, opt->row, opt->end_row);
        need = opt->min.h + opt->pad.t + opt->pad.b;
        if (tot < opt->min.h)
          {
             Evas_Coord space = need - tot;
             int count;

             count = _evas_object_table_count_expands
                (c->expands.v, opt->row, opt->end_row);

             if (count > 0)
               _evas_object_table_sizes_calc_expand
                  (c->sizes.v, opt->row, opt->end_row, space,
                   c->expands.v, count, c->weights.v, totweighth);
             else
               _evas_object_table_sizes_calc_noexpand
                  (c->sizes.v, opt->row, opt->end_row, space);
          }
     }

   c->total.weights.h = totweightw;
   c->total.weights.v = totweighth;

   c->total.expands.h = _evas_object_table_count_expands
      (c->expands.h, 0, priv->size.cols);
   c->total.expands.v = _evas_object_table_count_expands
     (c->expands.v, 0, priv->size.rows);

   c->total.min.w = _evas_object_table_sum_sizes
     (c->sizes.h, 0, priv->size.cols);
   c->total.min.h = _evas_object_table_sum_sizes
     (c->sizes.v, 0, priv->size.rows);

   c->total.min.w += priv->pad.h * (priv->size.cols - 1);
   c->total.min.h += priv->pad.v * (priv->size.rows - 1);

   if ((c->total.min.w > 0) || (c->total.min.h > 0))
     evas_object_size_hint_min_set(o, c->total.min.w, c->total.min.h);
   _evas_object_table_cache_free(c);
   // XXX hint max?
}

/**
 * @brief Calculates and applies the layout for all children in regular (non-homogeneous) mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * This function uses the pre-calculated (or cached) column widths and row heights.
 * 1. It gets the table's actual geometry.
 * 2. It determines the final sizes for each column and row:
 *    - If the table's available space is less than its calculated minimum,
 *      the content is aligned within the available space using the table's
 *      alignment hints, and the cached minimum column/row sizes are used.
 *    - If there's extra space and there are expanding columns/rows,
 *      this extra space is distributed among them according to their weights
 *      (or evenly if weights are zero).
 * 3. It then iterates through each child, calculates its cell's geometry based on
 *    the final column/row sizes and its col/row span, and then calls
 *    `_evas_object_table_calculate_cell()` to position the child within that cell.
 * 4. Handles mirroring if enabled.
 */
static void
_evas_object_table_calculate_layout_regular(Evas_Object *o, Evas_Table_Data *priv)
{
   Evas_Object_Table_Option *opt;
   Evas_Object_Table_Cache *c;
   Eina_List *l;
   Evas_Coord *cols = NULL, *rows = NULL;
   Evas_Coord x, y, w, h;
   Evas_Coord totw;

   c = priv->cache;
   if (!c) return;

   c->ref++;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   totw = w;

   /* handle horizontal */
   if ((c->total.expands.h <= 0) || (c->total.min.w >= w))
     {
        x += (w - c->total.min.w) * priv->align.h;
        w = c->total.min.w;
        cols = c->sizes.h;
     }
   else
     {
        int size = priv->size.cols * sizeof(Evas_Coord);
        cols = malloc(size);
        if (!cols)
          {
             ERR("Could not allocate temp columns (%d bytes): %s",
                 size, strerror(errno));
             goto end;
          }
        memcpy(cols, c->sizes.h, size);
        _evas_object_table_sizes_calc_expand
           (cols, 0, priv->size.cols, w - c->total.min.w,
            c->expands.h, c->total.expands.h, c->weights.h, c->total.weights.h);
     }

   /* handle vertical */
   if ((c->total.expands.v <= 0) || (c->total.min.h >= h))
     {
        y += (h - c->total.min.h) * priv->align.v;
        h = c->total.min.h;
        rows = c->sizes.v;
     }
   else
     {
        int size = priv->size.rows * sizeof(Evas_Coord);
        rows = malloc(size);
        if (!rows)
          {
             ERR("could not allocate temp rows (%d bytes): %s",
                 size, strerror(errno));
             goto end;
          }
        memcpy(rows, c->sizes.v, size);
        _evas_object_table_sizes_calc_expand
           (rows, 0, priv->size.rows, h - c->total.min.h,
            c->expands.v, c->total.expands.v, c->weights.v, c->total.weights.v);
     }

   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        Evas_Object *child = opt->obj;
        Evas_Coord cx, cy, cw, ch;

        cx = x + opt->col * (priv->pad.h);
        cx += _evas_object_table_sum_sizes(cols, 0, opt->col);
        cw = (opt->colspan - 1) * priv->pad.h;
        cw += _evas_object_table_sum_sizes(cols, opt->col, opt->end_col);

        cy = y + opt->row * (priv->pad.v);
        cy += _evas_object_table_sum_sizes(rows, 0, opt->row);
        ch = (opt->rowspan - 1) * priv->pad.v;
        ch += _evas_object_table_sum_sizes(rows, opt->row, opt->end_row);

        _evas_object_table_calculate_cell(opt, &cx, &cy, &cw, &ch);

        if (priv->is_mirrored)
          evas_object_geometry_set(opt->obj, x + w + 2 * (0.5 - priv->align.h) * (totw - w) - (cx - x + cw), cy, cw, ch);
        else
          evas_object_geometry_set(child, cx, cy, cw, ch);
     }

 end:
   if (priv->cache)
     {
        if (cols != c->sizes.h)
          {
             if (cols) free(cols);
          }
        if (rows != c->sizes.v)
          {
             if (rows) free(rows);
          }
     }
   _evas_object_table_cache_free(c);
}

/**
 * @brief Top-level function for calculating layout in regular (non-homogeneous) mode.
 * @param o The table object.
 * @param priv The private data of the table.
 *
 * If hints have changed (or cache is invalid), it first recalculates them.
 * Then, it proceeds to calculate and apply the layout for children.
 */
static void
_evas_object_table_smart_calculate_regular(Evas_Object *o, Evas_Table_Data *priv)
{
   if (priv->hints_changed)
     _evas_object_table_calculate_hints_regular(o, priv);
   _evas_object_table_calculate_layout_regular(o, priv);
}

/**
 * @brief Initializes the Evas_Table_Data when the object is added to a canvas group.
 * @param obj The Evas_Table object.
 * @param priv The private data of the table.
 *
 * Sets default values for padding, alignment, size, cache, homogeneous mode,
 * and flags. Calls the superclass's group_add function.
 */
EOLIAN static void
_evas_table_efl_canvas_group_group_add(Eo *obj, Evas_Table_Data *priv)
{
   priv->pad.h = 0;
   priv->pad.v = 0;
   priv->align.h = 0.5;
   priv->align.v = 0.5;
   priv->size.cols = 0;
   priv->size.rows = 0;
   priv->cache = NULL;
   priv->homogeneous = EVAS_OBJECT_TABLE_HOMOGENEOUS_NONE;
   priv->hints_changed = 1;
   priv->expand_h = 0;
   priv->expand_v = 0;

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
}

/**
 * @brief Cleans up the Evas_Table_Data when the object is removed from a canvas group.
 * @param obj The Evas_Table object.
 * @param priv The private data of the table.
 *
 * Frees all child packing options, disconnects callbacks from children,
 * releases the layout cache, and calls the superclass's group_del function.
 */
EOLIAN static void
_evas_table_efl_canvas_group_group_del(Eo *obj, Evas_Table_Data *priv)
{
   Eina_List *l;

   l = priv->children;
   while (l)
     {
        Evas_Object_Table_Option *opt = l->data;
        _evas_object_table_child_disconnect(obj, opt->obj);
        _evas_object_table_option_del(opt->obj);
        free(opt);
        l = eina_list_remove_list(l, l);
     }

   if (priv->cache)
     {
        _evas_object_table_cache_free(priv->cache);
        priv->cache = NULL;
     }

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @brief Handles size setting for the table.
 * @param obj The Evas_Table object.
 * @param _pd Private data (unused).
 * @param sz The new size.
 *
 * Calls interceptors, then the superclass's size_set, and finally
 * marks the smart object as changed to trigger recalculation.
 */
EOLIAN static void
_evas_table_efl_gfx_entity_size_set(Eo *obj, Evas_Table_Data *_pd EINA_UNUSED, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);
   evas_object_smart_changed(obj);
}

/**
 * @brief Handles position setting for the table.
 * @param obj The Evas_Table object.
 * @param _pd Private data (unused).
 * @param pos The new position.
 *
 * Calls interceptors, then the superclass's position_set, and finally
 * marks the smart object as changed (though position change doesn't directly
 * affect table's internal layout, it's good practice for smart objects).
 */
EOLIAN static void
_evas_table_efl_gfx_entity_position_set(Eo *obj, Evas_Table_Data *_pd EINA_UNUSED, Eina_Position2D pos)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_MOVE, 0, pos.x, pos.y))
     return;

   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);
   evas_object_smart_changed(obj);
}

/**
 * @brief Performs the layout calculation for the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 *
 * This is the main entry point for table layout. It freezes Evas events,
 * calls the appropriate calculation function based on homogeneous mode,
 * and then thaws Evas events.
 */
EOLIAN static void
_evas_table_efl_canvas_group_group_calculate(Eo *o, Evas_Table_Data *priv)
{
   Evas *e;

   if ((priv->size.cols < 1) || (priv->size.rows < 1))
     {
        DBG("Nothing to do: cols=%d, rows=%d",
            priv->size.cols, priv->size.rows);
        return;
     }

   e = evas_object_evas_get(o);
   evas_event_freeze(e);

   if (priv->homogeneous)
     _evas_object_table_smart_calculate_homogeneous(o, priv);
   else
     _evas_object_table_smart_calculate_regular(o, priv);

   evas_event_thaw(e);
   evas_event_thaw_eval(e);
}

/**
 * @brief Creates a new Evas_Object_Table.
 * @param evas The Evas canvas to add the table to.
 * @return The new Evas_Object_Table, or NULL on failure.
 * @ingroup Evas_Object_Table
 */
EVAS_API Evas_Object *
evas_object_table_add(Evas *evas)
{
   evas = evas_find(evas);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(evas, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, evas, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @brief Constructor for the Evas_Table EFL object.
 * @param obj The Evas_Table object being constructed.
 * @param class_data Private class data (unused).
 * @return The constructed object.
 *
 * Sets the object to be clipped by default, calls the superclass constructor,
 * and sets the legacy type name.
 */
EOLIAN static Eo *
_evas_table_efl_object_constructor(Eo *obj, Evas_Table_Data *class_data EINA_UNUSED)
{
   efl_canvas_group_clipped_set(obj, EINA_TRUE);
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);

   return obj;
}

/**
 * @brief Adds a new table as a smart member of a parent object.
 * @param parent The parent Evas_Object.
 * @param _pd Private data (unused).
 * @return The new Evas_Object_Table, or NULL on failure.
 *
 * This is typically used by container widgets that want to provide a table
 * as part of their internal implementation.
 */
EOLIAN static Evas_Object*
_evas_table_add_to(Eo *parent, Evas_Table_Data *_pd EINA_UNUSED)
{
   Evas_Object *ret;
   Evas *evas;

   evas = evas_object_evas_get(parent);
   ret = evas_object_table_add(evas);
   evas_object_smart_member_add(ret, parent);

   return ret;
}

/**
 * @brief Sets the homogeneous mode of the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param homogeneous The new homogeneous mode.
 *        Example: `EVAS_OBJECT_TABLE_HOMOGENEOUS_NONE`, `EVAS_OBJECT_TABLE_HOMOGENEOUS_TABLE`, `EVAS_OBJECT_TABLE_HOMOGENEOUS_ITEM`.
 *
 * If the mode changes, the cache is invalidated and the table is marked for recalculation.
 */
EOLIAN static void
_evas_table_homogeneous_set(Eo *o, Evas_Table_Data *priv, Evas_Object_Table_Homogeneous_Mode homogeneous)
{
   if (priv->homogeneous == homogeneous)
     return;
   priv->homogeneous = homogeneous;
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(o);
}

/**
 * @brief Gets the homogeneous mode of the table.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @return The current homogeneous mode.
 */
EOLIAN static Evas_Object_Table_Homogeneous_Mode
_evas_table_homogeneous_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv)
{
   return priv->homogeneous;
}

/**
 * @brief Sets the overall alignment of the table content within its allocated space.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param horizontal Horizontal alignment (0.0 to 1.0, or -1.0 for fill).
 * @param vertical Vertical alignment (0.0 to 1.0, or -1.0 for fill).
 *
 * If alignment changes, the table is marked for recalculation.
 */
EOLIAN static void
_evas_table_align_set(Eo *o, Evas_Table_Data *priv, double horizontal, double vertical)
{
   if ((EINA_DBL_EQ(priv->align.h, horizontal)) &&
       (EINA_DBL_EQ(priv->align.v, vertical)))
     return;
   priv->align.h = horizontal;
   priv->align.v = vertical;
   evas_object_smart_changed(o);
}

/**
 * @brief Gets the overall alignment of the table content.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @param horizontal Pointer to store horizontal alignment.
 * @param vertical Pointer to store vertical alignment.
 */
EOLIAN static void
_evas_table_align_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv, double *horizontal, double *vertical)
{
   if (priv)
     {
        if (horizontal) *horizontal = priv->align.h;
        if (vertical) *vertical = priv->align.v;
     }
   else
     {
        if (horizontal) *horizontal = 0.5; // Default alignment
        if (vertical) *vertical = 0.5;   // Default alignment
     }
}

/**
 * @brief Sets the padding between cells in the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param horizontal Horizontal padding between columns.
 * @param vertical Vertical padding between rows.
 *
 * If padding changes, the cache is invalidated and the table is marked for recalculation.
 */
EOLIAN static void
_evas_table_padding_set(Eo *o, Evas_Table_Data *priv, Evas_Coord horizontal, Evas_Coord vertical)
{
   if (priv->pad.h == horizontal && priv->pad.v == vertical)
     return;
   priv->pad.h = horizontal;
   priv->pad.v = vertical;
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(o);
}

/**
 * @brief Gets the padding between cells in the table.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @param horizontal Pointer to store horizontal padding.
 * @param vertical Pointer to store vertical padding.
 */
EOLIAN static void
_evas_table_padding_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv, Evas_Coord *horizontal, Evas_Coord *vertical)
{
   if (priv)
     {
        if (horizontal) *horizontal = priv->pad.h;
        if (vertical) *vertical = priv->pad.v;
     }
   else
     {
        if (horizontal) *horizontal = 0; // Default padding
        if (vertical) *vertical = 0;   // Default padding
     }
}

/**
 * @brief Gets the packing information for a child object.
 * @param o The Evas_Table object (unused).
 * @param _pd Private data (unused).
 * @param child The child object.
 * @param col Pointer to store the column.
 * @param row Pointer to store the row.
 * @param colspan Pointer to store the column span.
 * @param rowspan Pointer to store the row span.
 * @return EINA_TRUE if the child is packed and info is retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_evas_table_pack_get(const Eo *o EINA_UNUSED, Evas_Table_Data *_pd EINA_UNUSED, Evas_Object *child, unsigned short *col, unsigned short *row, unsigned short *colspan, unsigned short *rowspan)
{
   Evas_Object_Table_Option *opt;

   opt = _evas_object_table_option_get(child);
   if (!opt)
     {
        if (col) *col = 0;
        if (row) *row = 0;
        if (colspan) *colspan = 0;
        if (rowspan) *rowspan = 0;
        return EINA_FALSE;
     }
   if (col) *col = opt->col;
   if (row) *row = opt->row;
   if (colspan) *colspan = opt->colspan;
   if (rowspan) *rowspan = opt->rowspan;

   return EINA_TRUE;
}

/**
 * @brief Packs a child object into the table at a specified location and span.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param child The child object to pack.
 * @param col The column to pack the child into (0-indexed).
 * @param row The row to pack the child into (0-indexed).
 * @param colspan The number of columns the child should span. Must be >= 1.
 * @param rowspan The number of rows the child should span. Must be >= 1.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid parameters, memory allocation failure).
 *
 * If the child is already packed in this table, its packing options are updated.
 * If it's a new child, packing options are created, and it's added as a smart member.
 * The table's dimensions (cols, rows) are updated if necessary.
 * The cache is invalidated and the table is marked for recalculation.
 */
EOLIAN static Eina_Bool
_evas_table_pack(Eo *o, Evas_Table_Data *priv, Evas_Object *child, unsigned short col, unsigned short row, unsigned short colspan, unsigned short rowspan)
{
   Eina_Bool optalloc = EINA_FALSE;

   Evas_Object_Table_Option *opt;

   if (colspan < 1)
     {
        ERR("colspan < 1");
        return EINA_FALSE;
     }
   if ((0xffff - col) < colspan)
     {
        ERR("col + colspan > 0xffff");
        return EINA_FALSE;
     }
   if ((col + colspan) >= 0x7ffff)
     {
        WRN("col + colspan getting rather large (>32767)");
     }
   if (rowspan < 1)
     {
        ERR("rowspan < 1");
        return EINA_FALSE;
     }
   if ((0xffff - row) < rowspan)
     {
        ERR("row + rowspan > 0xffff");
        return EINA_FALSE;
     }
   if ((row + rowspan) >= 0x7ffff)
     {
        WRN("row + rowspan getting rather large (>32767)");
     }

   opt = _evas_object_table_option_get(child);
   if (opt)
     {
        if (evas_object_smart_parent_get(child) != o)
          {
             CRI("cannot pack child of one table into another table!");
             return EINA_FALSE;
          }
     }
   else
     {
        opt = malloc(sizeof(*opt));
        if (!opt)
          {
             ERR("could not allocate table option data.");
             return EINA_FALSE;
          }
        optalloc = EINA_TRUE;
     }

   opt->obj = child;
   opt->col = col;
   opt->row = row;
   opt->colspan = colspan;
   opt->rowspan = rowspan;
   opt->end_col = col + colspan;
   opt->end_row = row + rowspan;

   if (!optalloc)
     {
        Eina_Bool need_shrink = EINA_FALSE;

        if (priv->size.cols < opt->end_col)
          priv->size.cols = opt->end_col;
        else
          need_shrink = EINA_TRUE;
        if (priv->size.rows < opt->end_row)
          priv->size.rows = opt->end_row;
        else
          need_shrink = EINA_TRUE;

        if (need_shrink)
          {
             Eina_List *l;
             Evas_Object_Table_Option *opt2;
             int max_row = 0, max_col = 0;

             EINA_LIST_FOREACH(priv->children, l, opt2)
               {
                  if (max_col < opt2->end_col) max_col = opt2->end_col;
                  if (max_row < opt2->end_row) max_row = opt2->end_row;
               }
             priv->size.cols = max_col;
             priv->size.rows = max_row;
          }
     }
   else
     {
        opt->min.w = 0;
        opt->min.h = 0;
        opt->max.w = 0;
        opt->max.h = 0;
        opt->align.h = 0.5;
        opt->align.v = 0.5;
        opt->pad.l = 0;
        opt->pad.r = 0;
        opt->pad.t = 0;
        opt->pad.b = 0;
        opt->expand_h = 0;
        opt->expand_v = 0;

        priv->children = eina_list_append(priv->children, opt);

        if (priv->size.cols < opt->end_col)
          priv->size.cols = opt->end_col;
        if (priv->size.rows < opt->end_row)
          priv->size.rows = opt->end_row;

        _evas_object_table_option_set(child, opt);
        evas_object_smart_member_add(child, o);
        _evas_object_table_child_connect(o, child);
     }
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(o);

   return EINA_TRUE;
}

/**
 * @brief Removes a child's packing option from the internal list and updates table dimensions if needed.
 * @param priv The private data of the table.
 * @param opt The Evas_Object_Table_Option of the child to remove.
 *
 * This function is an internal helper for unpack operations. It removes the `opt`
 * from `priv->children`. If the removed child defined the maximum column or row
 * extent of the table, the table's `size.cols` and `size.rows` are recalculated
 * by iterating through the remaining children.
 */
static void
_evas_object_table_remove_opt(Evas_Table_Data *priv, Evas_Object_Table_Option *opt)
{
   Eina_List *l;
   int max_row, max_col, was_greatest;

   max_row = 0;
   max_col = 0;
   was_greatest = 0;
   l = priv->children;
   while (l)
     {
        Evas_Object_Table_Option *cur_opt = l->data;

        if (cur_opt != opt)
          {
             if (max_col < cur_opt->end_col)
               max_col = cur_opt->end_col;
             if (max_row < cur_opt->end_row)
               max_row = cur_opt->end_row;

             l = l->next;
          }
        else
          {
             Eina_List *tmp = l->next;
             priv->children = eina_list_remove_list(priv->children, l);

             if ((priv->size.cols > opt->end_col) &&
                 (priv->size.rows > opt->end_row))
               break;
             else
               {
                  was_greatest = 1;
                  l = tmp;
               }
          }
     }

   if (was_greatest)
     {
        priv->size.cols = max_col;
        priv->size.rows = max_row;
     }
}

/**
 * @brief Unpacks (removes) a child object from the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param child The child object to unpack.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., child not packed or not a child of this table).
 *
 * Removes the child as a smart member, disconnects callbacks, frees its packing options,
 * invalidates the cache, and marks the table for recalculation.
 */
EOLIAN static Eina_Bool
_evas_table_unpack(Eo *o, Evas_Table_Data *priv, Evas_Object *child)
{
   Evas_Object_Table_Option *opt;

   if (o != evas_object_smart_parent_get(child))
     {
        ERR("cannot unpack child from incorrect table!");
        return EINA_FALSE;
     }

   opt = _evas_object_table_option_del(child);
   if (!opt)
     {
        ERR("cannot unpack child with no packing option!");
        return EINA_FALSE;
     }

   _evas_object_table_child_disconnect(o, child);
   _evas_object_table_remove_opt(priv, opt);
   evas_object_smart_member_del(child);
   free(opt);
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(o);

   return EINA_TRUE;
}

/**
 * @brief Removes all children from the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param clear If EINA_TRUE, also delete the child objects. If EINA_FALSE, only unpack them.
 *
 * Iterates through all children, unpacks them, and optionally deletes them.
 * Resets table dimensions, invalidates cache, and marks for recalculation.
 */
EOLIAN static void
_evas_table_clear(Eo *o, Evas_Table_Data *priv, Eina_Bool clear)
{
   Evas_Object_Table_Option *opt;
   Evas *e;

   e = evas_object_evas_get(o);
   evas_event_freeze(e);

   EINA_LIST_FREE(priv->children, opt)
     {
        _evas_object_table_child_disconnect(o, opt->obj);
        _evas_object_table_option_del(opt->obj);
        evas_object_smart_member_del(opt->obj);
        if (clear)
          evas_object_del(opt->obj);
        free(opt);
     }
   priv->size.cols = 0;
   priv->size.rows = 0;
   _evas_object_table_cache_invalidate(priv);
   evas_object_smart_changed(o);

   evas_event_thaw(e);
}

/**
 * @brief Gets the number of columns and rows currently in the table.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @param cols Pointer to store the number of columns.
 * @param rows Pointer to store the number of rows.
 *
 * These dimensions are determined by the maximum column/row + span of packed children.
 */
EOLIAN static void
_evas_table_col_row_size_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv, int *cols, int *rows)
{
   if (priv)
     {
        if (cols) *cols = priv->size.cols;
        if (rows) *rows = priv->size.rows;
     }
   else
     {
        if (cols) *cols = -1; // Indicate error or uninitialized state
        if (rows) *rows = -1; // Indicate error or uninitialized state
     }
}

/**
 * @brief Creates a new iterator for the children of the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @return A new Eina_Iterator, or NULL if there are no children or on allocation failure.
 *
 * The iterator will yield Evas_Object pointers (the children).
 */
EOLIAN static Eina_Iterator*
_evas_table_iterator_new(const Eo *o, Evas_Table_Data *priv)
{
   Evas_Object_Table_Iterator *it;

   if (!priv->children)
     {
        return NULL;
     }

   it = calloc(1, sizeof(Evas_Object_Table_Iterator));
   if (!it)
     {
        return NULL;
     }

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->real_iterator = eina_list_iterator_new(priv->children);
   it->table = o;

   it->iterator.next = FUNC_ITERATOR_NEXT(_evas_object_table_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_evas_object_table_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_evas_object_table_iterator_free);

   return &it->iterator;
}

/**
 * @brief Creates a new accessor for the children of the table.
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @return A new Eina_Accessor, or NULL if there are no children or on allocation failure.
 *
 * The accessor allows random access to children by index, yielding Evas_Object pointers.
 */
EOLIAN static Eina_Accessor*
_evas_table_accessor_new(const Eo *o, Evas_Table_Data *priv)
{
   Evas_Object_Table_Accessor *it;

   if (!priv->children) return NULL;

   it = calloc(1, sizeof(Evas_Object_Table_Accessor));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->accessor, EINA_MAGIC_ACCESSOR);

   it->real_accessor = eina_list_accessor_new(priv->children);
   it->table = o;

   it->accessor.get_at = FUNC_ACCESSOR_GET_AT(_evas_object_table_accessor_get_at);
   it->accessor.get_container = FUNC_ACCESSOR_GET_CONTAINER(_evas_object_table_accessor_get_container);
   it->accessor.free = FUNC_ACCESSOR_FREE(_evas_object_table_accessor_free);

   return &it->accessor;
}

/**
 * @brief Gets a new list containing all child objects of the table.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @return A new Eina_List containing Evas_Object pointers of the children.
 *         The caller is responsible for freeing this list (but not its contents).
 *         Returns NULL if there are no children.
 */
EOLIAN static Eina_List*
_evas_table_children_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv)
{
   Eina_List *new_list = NULL, *l;
   Evas_Object_Table_Option *opt;

   EINA_LIST_FOREACH(priv->children, l, opt)
      new_list = eina_list_append(new_list, opt->obj);

   return new_list;
}

/**
 * @brief Gets the number of children packed into the table.
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @return The count of child objects.
 */
EOLIAN static int
_evas_table_count(Eo *o EINA_UNUSED, Evas_Table_Data *priv)
{
   return eina_list_count(priv->children);
}

/**
 * @brief Gets the child object packed at a specific cell (top-left corner).
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @param col The column of the cell.
 * @param row The row of the cell.
 * @return The Evas_Object at the specified cell, or NULL if no child starts at that cell.
 *
 * Note: This finds a child whose packing `col` and `row` match the parameters.
 * It does not consider colspans or rowspans for cells other than the top-left.
 */
EOLIAN static Evas_Object *
_evas_table_child_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv, unsigned short col, unsigned short row)
{
   Eina_List *l;
   Evas_Object_Table_Option *opt;

   EINA_LIST_FOREACH(priv->children, l, opt)
      if (opt->col == col && opt->row == row)
         return opt->obj;
   return NULL;
}

/**
 * @brief Gets the mirrored mode of the table (for RTL language support).
 * @param o The Evas_Table object (unused).
 * @param priv The private data of the table.
 * @return EINA_TRUE if mirrored, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_evas_table_efl_ui_i18n_mirrored_get(const Eo *o EINA_UNUSED, Evas_Table_Data *priv)
{
   return priv->is_mirrored;
}

/**
 * @brief Gets the mirrored mode of the table.
 * @param obj The Evas_Table object.
 * @return EINA_TRUE if mirrored, EINA_FALSE otherwise.
 * @ingroup Evas_Object_Table
 */
EVAS_API Eina_Bool
evas_object_table_mirrored_get(const Eo *obj)
{
   return efl_ui_mirrored_get(obj);
}

/**
 * @brief Sets the mirrored mode of the table (for RTL language support).
 * @param o The Evas_Table object.
 * @param priv The private data of the table.
 * @param mirrored EINA_TRUE to enable mirrored mode, EINA_FALSE to disable.
 *
 * If the mode changes, triggers a recalculation of the table layout.
 */
EOLIAN static void
_evas_table_efl_ui_i18n_mirrored_set(Eo *o, Evas_Table_Data *priv, Eina_Bool mirrored)
{
   if (priv->is_mirrored != mirrored)
     {
        priv->is_mirrored = mirrored;
        efl_canvas_group_calculate(o);
     }
}

/**
 * @brief Sets the mirrored mode of the table.
 * @param obj The Evas_Table object.
 * @param mirrored EINA_TRUE to enable mirrored mode, EINA_FALSE to disable.
 * @ingroup Evas_Object_Table
 */
EVAS_API void
evas_object_table_mirrored_set(Eo *obj, Eina_Bool mirrored)
{
   efl_ui_mirrored_set(obj, mirrored);
}

/**
 * @brief Class constructor for Evas_Table.
 * @param klass The Efl_Class being constructed.
 *
 * Registers the legacy smart type name for this class.
 */
EOLIAN static void
_evas_table_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Internal EO APIs and hidden overrides */

#define EVAS_TABLE_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(evas_table)

#include "canvas/evas_table_eo.c"
