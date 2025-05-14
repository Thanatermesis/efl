#include "efl_ui_table_private.h"
#include "efl_ui_container_layout.h"

typedef struct _Item_Calc Item_Calc;
typedef struct _Cell_Calc Cell_Calc;
typedef struct _Table_Calc Table_Calc;

/**
 * @brief Structure to hold calculation data for a single item in the table.
 */
struct _Item_Calc
{
   Evas_Object *obj; /**< The Evas object representing the item. */
   int cell_span[2]; /**< How many cells this item spans in [col, row] dimensions. Example: `{2, 1}` means it spans 2 columns and 1 row. */
   int cell_index[2]; /**< The starting [col, row] index of this item. Example: `{0, 0}` is the top-left cell. */
   Efl_Ui_Container_Item_Hints hints[2]; /**< Layout hints for [x-axis, y-axis]. */
};

/**
 * @brief Structure to hold calculation data for a single cell (column or row).
 */
struct _Cell_Calc
{
   EINA_INLIST; /**< Macro to make this struct usable in an Eina_Inlist. */

   int       index; /**< The logical index of this cell (0 to N-1 occupied cells). */
   int       next; /**< The actual index in the `cell_calc` array of the next occupied cell. */
   double    acc; /**< Accumulated size of cells up to this one (excluding this one's space). */
   double    space; /**< The calculated space (width or height) this cell should occupy. */
   double    weight; /**< The sum of weights of items in this cell. */
   double    weight_factor; /**< Factor used for sorting cells by weight distribution priority. */
   Eina_Bool occupied : 1; /**< Flag indicating if this cell is occupied by any item. */
};

/**
 * @brief Structure to hold overall table calculation data.
 */
struct _Table_Calc
{
   /* 0 is x-axis (columns), 1 is y-axis (rows) */

   int                           rows; /**< Total number of rows in the table. */
   int                           cols; /**< Total number of columns in the table. */
   int                           want[2]; /**< The total desired size [width, height] of the table content. */
   int                           hgsize[2]; /**< The size [width, height] of a single cell in homogeneous mode. */
   double                        weight_sum[2]; /**< Sum of all cell weights for [x-axis, y-axis]. */
   Cell_Calc                    *cell_calc[2]; /**< Array of Cell_Calc structures for [columns, rows]. Example `cell_calc[0]` is for columns, `cell_calc[1]` for rows. `cell_calc[0][i]` would be the i-th column's calculation data. */
   Efl_Ui_Container_Layout_Calc  layout_calc[2]; /**< General container layout calculation data for [x-axis, y-axis]. */
};

/**
 * @brief Comparison function for sorting Cell_Calc structures by weight_factor.
 *
 * Sorts in descending order of weight_factor.
 *
 * @param l1 Pointer to the first Eina_Inlist node (Cell_Calc).
 * @param l2 Pointer to the second Eina_Inlist node (Cell_Calc).
 * @return -1 if cc1->weight_factor >= cc2->weight_factor, 1 otherwise.
 */
static int
_weight_sort_cb(const void *l1, const void *l2)
{
   Cell_Calc *cc1, *cc2;

   cc1 = EINA_INLIST_CONTAINER_GET(l1, Cell_Calc);
   cc2 = EINA_INLIST_CONTAINER_GET(l2, Cell_Calc);

   return cc2->weight_factor <= cc1->weight_factor ? -1 : 1;
}

/**
 * @brief Calculates cell sizes based on their weights when there's extra space.
 *
 * This function distributes remaining space among cells proportionally to their
 * weights, ensuring that cells with higher weights get more of the available
 * extra space. It handles cases where distributing space based on weight might
 * cause a cell to shrink below its minimum required space.
 *
 * @param table_calc Pointer to the main table calculation data.
 * @param axis EINA_TRUE for y-axis (rows), EINA_FALSE for x-axis (columns).
 */
static void
_cell_weight_calc(Table_Calc *table_calc, Eina_Bool axis)
{
   int i, count, layout_size, calc_size;
   double denom, weight_sum, calc_weight;
   Eina_Inlist *inlist = NULL;
   Cell_Calc *cell_calc, *cc;

   layout_size = calc_size = table_calc->layout_calc[axis].size;
   weight_sum = calc_weight = table_calc->weight_sum[axis];
   cell_calc = table_calc->cell_calc[axis];
   count = axis ? table_calc->rows : table_calc->cols;

   for (i = 0; i < count; i = cell_calc[i].next)
     {
        denom = (cell_calc[i].weight * layout_size) -
                (weight_sum * cell_calc[i].space);
        if (denom > 0)
          {
             cell_calc[i].weight_factor = (cell_calc[i].weight * layout_size) / denom;
             inlist =  eina_inlist_sorted_insert(inlist,
                                                 EINA_INLIST_GET(&cell_calc[i]),
                                                 _weight_sort_cb);
          }
        else
          {
             calc_size -= cell_calc[i].space;
             calc_weight -= cell_calc[i].weight;
          }
     }

   EINA_INLIST_FOREACH(inlist, cc)
     {
        double weight_len;

        weight_len = (calc_size * cc->weight) / calc_weight;
        if (cc->space < weight_len)
          {
             cc->space = weight_len;
          }
        else
          {
             calc_size -= cc->space;
             calc_weight -= cc->weight;
          }
     }
}

/**
 * @brief Initializes cell calculation data for a homogeneous table layout.
 *
 * In a homogeneous layout, all occupied cells along the specified axis
 * will have the same size. This function determines that size.
 *
 * @param table_calc Pointer to the main table calculation data.
 * @param axis EINA_TRUE for y-axis (rows), EINA_FALSE for x-axis (columns).
 */
static void
_efl_ui_table_homogeneous_cell_init(Table_Calc *table_calc, Eina_Bool axis)
{
   int i, index = 0, mmin = 0, count;
   Cell_Calc *prev_cell = NULL, *cell_calc;

   cell_calc = table_calc->cell_calc[axis];
   count = axis ? table_calc->rows : table_calc->cols;

   for (i = 0; i < count; i++)
     {
        if (!cell_calc[i].occupied) continue;

        cell_calc[i].index = index++;
        if (cell_calc[i].space > mmin)
          mmin = cell_calc[i].space;

        if (prev_cell)
          prev_cell->next = i;

        prev_cell = &cell_calc[i];
     }
   if (!index)
     {
        memset(table_calc, 0, sizeof(Table_Calc));
        return;
     }
   if (prev_cell)
     prev_cell->next = count;

   table_calc->layout_calc[axis].size -= (table_calc->layout_calc[axis].pad
                                          * (index - 1));

   table_calc->want[axis] = mmin * index;
   table_calc->weight_sum[axis] = index;

   if (table_calc->want[axis] > table_calc->layout_calc[axis].size)
     table_calc->hgsize[axis] = table_calc->want[axis] / index;
   else
     table_calc->hgsize[axis] = table_calc->layout_calc[axis].size / index;

   table_calc->hgsize[axis] += table_calc->layout_calc[axis].pad;
}

/**
 * @brief Initializes cell calculation data for a regular (non-homogeneous) table layout.
 *
 * In a regular layout, cells can have different sizes based on the items they contain
 * and their weights. This function calculates initial cell properties like accumulated
 * size and total weight.
 *
 * @param table_calc Pointer to the main table calculation data.
 * @param axis EINA_TRUE for y-axis (rows), EINA_FALSE for x-axis (columns).
 */
static void
_efl_ui_table_regular_cell_init(Table_Calc *table_calc, Eina_Bool axis)
{
   int i, index = 0, acc, want = 0, count;
   double weight_sum = 0;
   Cell_Calc *prev_cell = NULL, *cell_calc;
   Efl_Ui_Container_Layout_Calc *layout_calc;

   layout_calc = &(table_calc->layout_calc[axis]);
   cell_calc = table_calc->cell_calc[axis];
   count = axis ? table_calc->rows : table_calc->cols;

   for (i = 0; i < count; i++)
     {
        if (!cell_calc[i].occupied) continue;
        else if (i && cell_calc[0].next == 0) cell_calc[0].next = i;

        cell_calc[i].index = index++;
        want += cell_calc[i].space;
        weight_sum += cell_calc[i].weight;

        if (prev_cell)
          prev_cell->next = i;

        prev_cell = &cell_calc[i];
     }
   if (prev_cell)
     prev_cell->next = count;

   table_calc->want[axis] = want;
   table_calc->weight_sum[axis] = weight_sum;
   table_calc->layout_calc[axis].size -= (table_calc->layout_calc[axis].pad
                                          * (index - 1));

   if ((layout_calc->size > want) && (weight_sum > 0))
     _cell_weight_calc(table_calc, axis);
   if (EINA_DBL_EQ(weight_sum, 0.0))
     layout_calc->pos += (layout_calc->size - want) * layout_calc->align;

   for (i = 0, acc = 0; i < count; acc += cell_calc[i].space, i = cell_calc[i].next)
     cell_calc[i].acc = acc;
}

/**
 * @brief Gets the position of an item in a homogeneous layout.
 * @param table_calc Pointer to the main table calculation data.
 * @param item Pointer to the item's calculation data.
 * @param axis EINA_TRUE for y-axis, EINA_FALSE for x-axis.
 * @return The calculated position (x or y coordinate) of the item.
 */
static inline int
_efl_ui_table_homogeneous_item_pos_get(Table_Calc *table_calc, Item_Calc *item, Eina_Bool axis)
{
   return 0.5 + table_calc->layout_calc[axis].pos + (table_calc->hgsize[axis]
           * table_calc->cell_calc[axis][item->cell_index[axis]].index);
}

/**
 * @brief Gets the size of an item in a homogeneous layout.
 * @param table_calc Pointer to the main table calculation data.
 * @param item Pointer to the item's calculation data.
 * @param axis EINA_TRUE for y-axis, EINA_FALSE for x-axis.
 * @return The calculated size (width or height) of the item.
 */
static inline int
_efl_ui_table_homogeneous_item_size_get(Table_Calc *table_calc, Item_Calc *item, Eina_Bool axis)
{
   return (table_calc->hgsize[axis] * item->cell_span[axis])
          - table_calc->layout_calc[axis].pad;
}

/**
 * @brief Gets the position of an item in a regular layout.
 * @param table_calc Pointer to the main table calculation data.
 * @param item Pointer to the item's calculation data.
 * @param axis EINA_TRUE for y-axis, EINA_FALSE for x-axis.
 * @return The calculated position (x or y coordinate) of the item.
 */
static inline int
_efl_ui_table_regular_item_pos_get(Table_Calc *table_calc, Item_Calc *item, Eina_Bool axis)
{
   return 0.5 + table_calc->layout_calc[axis].pos
          + table_calc->cell_calc[axis][item->cell_index[axis]].acc
          + (table_calc->cell_calc[axis][item->cell_index[axis]].index *
             table_calc->layout_calc[axis].pad);
}

/**
 * @brief Gets the size of an item in a regular layout.
 * @param table_calc Pointer to the main table calculation data.
 * @param item Pointer to the item's calculation data.
 * @param axis EINA_TRUE for y-axis, EINA_FALSE for x-axis.
 * @return The calculated size (width or height) of the item.
 */
static inline int
_efl_ui_table_regular_item_size_get(Table_Calc *table_calc, Item_Calc *item, Eina_Bool axis)
{
   int start, end;

   start = item->cell_index[axis];
   end = start + item->cell_span[axis] - 1;

   return table_calc->cell_calc[axis][end].acc
          - table_calc->cell_calc[axis][start].acc
          + table_calc->cell_calc[axis][end].space
          + ((item->cell_span[axis] - 1) * table_calc->layout_calc[axis].pad)
          - item->hints[axis].margin[0] - item->hints[axis].margin[1];
}

/* this function performs a simplified layout when the table has changed position
 * but no other changes have occurred, e.g., when a table is being scrolled
 */
/**
 * @brief Performs a simplified layout update when only the table's position changes.
 *
 * This is an optimization for scenarios like scrolling, where item sizes and
 * relative positions remain the same, and only their absolute screen positions
 * need updating.
 *
 * @param ui_table The Efl_Ui_Table object.
 * @param pd The private data of the Efl_Ui_Table.
 */
static void
_efl_ui_table_layout_simple(Efl_Ui_Table *ui_table, Efl_Ui_Table_Data *pd)
{
   Table_Item *ti;
   Eina_Position2D pos = efl_gfx_entity_position_get(ui_table);

   EINA_INLIST_FOREACH(EINA_INLIST_GET(pd->items), ti)
     {
        Eina_Position2D child_pos = efl_gfx_entity_position_get(ti->object);

        efl_gfx_entity_position_set(ti->object,
          EINA_POSITION2D(pos.x - pd->last_pos.x + child_pos.x,
                          pos.y - pd->last_pos.y + child_pos.y));
     }
   pd->last_pos = pos;
   efl_event_callback_call(ui_table, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
}

/**
 * @brief Performs the full custom layout calculation for the table.
 *
 * This function is called when a full recalculation of item positions and sizes
 * is necessary. It handles both homogeneous and regular layouts, considers item
 * spans, weights, hints, and applies padding and alignment.
 *
 * @param ui_table The Efl_Ui_Table object.
 * @param pd The private data of the Efl_Ui_Table.
 */
void
_efl_ui_table_custom_layout(Efl_Ui_Table *ui_table, Efl_Ui_Table_Data *pd)
{
   Table_Item *ti;
   Item_Calc *items, *item;
   Efl_Ui_Container_Item_Hints *hints;
   int id = 0, i = 0, count, rows, cols;
   int (*_efl_ui_table_item_pos_get[2])(Table_Calc *, Item_Calc *, Eina_Bool);
   int (*_efl_ui_table_item_size_get[2])(Table_Calc *, Item_Calc *, Eina_Bool);
   Table_Calc table_calc;
   Eina_Bool do_free;

   count = pd->count;
   if (!count)
     {
        efl_gfx_hint_size_restricted_min_set(ui_table, EINA_SIZE2D(0, 0));
        return;
     }
   if (!pd->full_recalc)
     {
        _efl_ui_table_layout_simple(ui_table, pd);
        return;
     }
   _efl_ui_container_layout_init(ui_table, table_calc.layout_calc);
   pd->last_pos.x = table_calc.layout_calc[0].pos - table_calc.layout_calc[0].margin[0];
   pd->last_pos.y = table_calc.layout_calc[1].pos - table_calc.layout_calc[1].margin[0];

   table_calc.want[0] = table_calc.want[1] = 0;
   table_calc.weight_sum[0] = table_calc.weight_sum[1] = 0;

   efl_pack_table_size_get(ui_table, &cols, &rows);

   table_calc.cell_calc[0] = alloca(cols * sizeof(Cell_Calc));
   table_calc.cell_calc[1] = alloca(rows * sizeof(Cell_Calc));

   memset(table_calc.cell_calc[0], 0, cols * sizeof(Cell_Calc));
   memset(table_calc.cell_calc[1], 0, rows * sizeof(Cell_Calc));

   /* Item_Calc struct is currently 152 bytes.
    * this is pretty big to be allocating a huge number of, and we don't want to explode the stack
    */
   do_free = count >= 500;
   if (do_free)
     {
        items = malloc(count * sizeof(*items));
        EINA_SAFETY_ON_NULL_RETURN(items);
     }
   else
     items = alloca(count * sizeof(*items));
#ifdef DEBUG
   memset(items, 0, count * sizeof(*items));
#endif

   table_calc.cols = cols;
   table_calc.rows = rows;
   // scan all items, get their properties, calculate total weight & min size
   EINA_INLIST_FOREACH(EINA_INLIST_GET(pd->items), ti)
     {
        if (((ti->col + ti->col_span) > cols) ||
            ((ti->row + ti->row_span) > rows))
          {
             efl_gfx_entity_geometry_set(ti->object, EINA_RECT(9999, 9999, 0, 0));
             count--;
             continue;
          }

        item = &items[id++];
        item->obj = ti->object;
        hints = item->hints;

        _efl_ui_container_layout_item_init(item->obj, hints);

        if (table_calc.layout_calc[0].fill || pd->homogeneoush)
          hints[0].weight = 1;
        else if (hints[0].weight < 0)
          hints[0].weight = 0;

        if (table_calc.layout_calc[1].fill || pd->homogeneousv)
          hints[1].weight = 1;
        else if (hints[1].weight < 0)
          hints[1].weight = 0;

        item->cell_index[0] = ti->col;
        item->cell_index[1] = ti->row;
        item->cell_span[0] = ti->col_span;
        item->cell_span[1] = ti->row_span;

        int end;
        double ispace, iweight;

        end = ti->col + ti->col_span;
        ispace = hints[0].space / ti->col_span;
        iweight = hints[0].weight / ti->col_span;
        for (i = ti->col; i < end; i++)
          {
             table_calc.cell_calc[0][i].occupied = EINA_TRUE;

             if (table_calc.cell_calc[0][i].space < ispace)
               table_calc.cell_calc[0][i].space = ispace;
             if (table_calc.cell_calc[0][i].weight < iweight)
               table_calc.cell_calc[0][i].weight = iweight;
          }

        end = ti->row + ti->row_span;
        ispace = hints[1].space / ti->row_span;
        iweight = hints[1].weight / ti->row_span;
        for (i = ti->row; i < end; i++)
          {
             table_calc.cell_calc[1][i].occupied = EINA_TRUE;

             if (table_calc.cell_calc[1][i].space < ispace)
               table_calc.cell_calc[1][i].space = ispace;
             if (table_calc.cell_calc[1][i].weight < iweight)
               table_calc.cell_calc[1][i].weight = iweight;
          }
     }

   if (pd->homogeneoush)
     {
        _efl_ui_table_homogeneous_cell_init(&table_calc, 0);
        _efl_ui_table_item_pos_get[0] = _efl_ui_table_homogeneous_item_pos_get;
        _efl_ui_table_item_size_get[0] = _efl_ui_table_homogeneous_item_size_get;
     }
   else
     {
        _efl_ui_table_regular_cell_init(&table_calc, 0);
        _efl_ui_table_item_pos_get[0] = _efl_ui_table_regular_item_pos_get;
        _efl_ui_table_item_size_get[0] = _efl_ui_table_regular_item_size_get;
     }

   if (pd->homogeneousv)
     {
        _efl_ui_table_homogeneous_cell_init(&table_calc, 1);
        _efl_ui_table_item_pos_get[1] = _efl_ui_table_homogeneous_item_pos_get;
        _efl_ui_table_item_size_get[1] = _efl_ui_table_homogeneous_item_size_get;
     }
   else
     {
        _efl_ui_table_regular_cell_init(&table_calc, 1);
        _efl_ui_table_item_pos_get[1] = _efl_ui_table_regular_item_pos_get;
        _efl_ui_table_item_size_get[1] = _efl_ui_table_regular_item_size_get;
     }

   for (i = 0; i < count; i++)
     {
        Eina_Rect space, item_geom;
        item = &items[i];
        hints = items[i].hints;

        space.x = _efl_ui_table_item_pos_get[0](&table_calc, item, 0);
        space.y = _efl_ui_table_item_pos_get[1](&table_calc, item, 1);
        space.w = _efl_ui_table_item_size_get[0](&table_calc, item, 0);
        space.h = _efl_ui_table_item_size_get[1](&table_calc, item, 1);

        item_geom.w = hints[0].fill ? space.w : hints[0].min;
        item_geom.h = hints[1].fill ? space.h : hints[1].min;

        _efl_ui_container_layout_min_max_calc(hints, &item_geom.w, &item_geom.h,
                                (hints[0].aspect > 0) && (hints[1].aspect > 0));

        item_geom.x = space.x + ((space.w - item_geom.w) * hints[0].align)
                      + hints[0].margin[0];
        item_geom.y = space.y + ((space.h - item_geom.h) * hints[1].align)
                      + hints[1].margin[0];

        efl_gfx_entity_geometry_set(item->obj, item_geom);
     }

   table_calc.want[0] += table_calc.layout_calc[0].margin[0]
                         + table_calc.layout_calc[0].margin[1]
                         + (table_calc.layout_calc[0].pad *
                            table_calc.cell_calc[0][cols - 1].index);

   table_calc.want[1] += table_calc.layout_calc[1].margin[0]
                         + table_calc.layout_calc[1].margin[1]
                         + (table_calc.layout_calc[1].pad *
                            table_calc.cell_calc[1][rows - 1].index);

   pd->full_recalc = EINA_FALSE;
   efl_gfx_hint_size_restricted_min_set(ui_table,
                                        EINA_SIZE2D(table_calc.want[0],
                                                    table_calc.want[1]));
   efl_event_callback_call(ui_table, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
   if (do_free) free(items);
}
