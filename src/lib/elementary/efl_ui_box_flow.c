#include "efl_ui_box_private.h"
#include "efl_ui_container_layout.h"

#define MY_CLASS EFL_UI_BOX_FLOW_CLASS

/**
 * @brief Private data for the Efl_Ui_Box_Flow class.
 */
typedef struct _Efl_Ui_Box_Flow_Data Efl_Ui_Box_Flow_Data;

struct _Efl_Ui_Box_Flow_Data
{
   /* Currently empty, but reserved for future use. */
};

/**
 * @brief Internal structure for calculating item properties during layout.
 */
typedef struct _Item_Calc Item_Calc;
/**
 * @brief Internal structure for calculating row properties during layout.
 */
typedef struct _Row_Calc Row_Calc;

/**
 * @brief Represents an individual item within a row for layout calculations.
 *
 * This structure holds information about an Evas_Object (a child item),
 * its associated row, weight factor for distribution, and layout hints.
 */
struct _Item_Calc
{
   EINA_INLIST; /**< Macro for intrusive list node. */

   Evas_Object *obj; /**< The child Evas_Object this calculation item represents. */
   Row_Calc *row;    /**< Pointer to the Row_Calc this item belongs to. */
   double weight_factor; /**< Factor used for sorting items by weight during space distribution. */
   Efl_Ui_Container_Item_Hints hints[2]; /**< Layout hints for the item. Index 0 for x-axis, 1 for y-axis. */
};

/**
 * @brief Represents a row of items in the flow layout.
 *
 * This structure aggregates information for a collection of items that form a single row
 * in the flow layout. It includes counts, minimum sizes, weights, and current position.
 */
struct _Row_Calc
{
   EINA_INLIST; /**< Macro for intrusive list node. */

   Evas_Object *obj; /**< Not directly used for a row itself, potentially a placeholder or for future use. */
   int item_count;   /**< Number of items in this row. */
   int min_sum;      /**< Sum of minimum sizes of items in this row along the main axis. */
   int hgsize;       /**< Homogeneous size for items in this row if homogeneous layout is active. */
   double weight_sum;   /**< Sum of weights of items in this row along the main axis. */
   double cross_weight; /**< Maximum weight of items in this row along the cross axis. */
   double cross_space;  /**< Maximum space (size) of items in this row along the cross axis. */
   double cur_pos;      /**< Current position of this row along the main axis during calculation. */
   double weight_factor;/**< Factor used for sorting rows by weight during space distribution. */
   Efl_Ui_Container_Item_Hints hint; /**< Layout hints for the row itself (primarily for cross-axis calculations). */
};

/**
 * @brief Comparison function for sorting Item_Calc structures by weight_factor.
 *
 * Used with eina_inlist_sorted_insert to sort items for weighted space distribution.
 * Sorts in descending order of weight_factor.
 *
 * @param l1 Pointer to the first Eina_Inlist node (containing an Item_Calc).
 * @param l2 Pointer to the second Eina_Inlist node (containing an Item_Calc).
 * @return -1 if it1->weight_factor >= it2->weight_factor, 1 otherwise.
 */
static int
_item_weight_sort_cb(const void *l1, const void *l2)
{
   Item_Calc *it1, *it2;

   it1 = EINA_INLIST_CONTAINER_GET(l1, Item_Calc);
   it2 = EINA_INLIST_CONTAINER_GET(l2, Item_Calc);

   return it2->weight_factor <= it1->weight_factor ? -1 : 1;
}

/**
 * @brief Comparison function for sorting Row_Calc structures by weight_factor.
 *
 * Used with eina_inlist_sorted_insert to sort rows for weighted space distribution
 * along the cross axis. Sorts in descending order of weight_factor.
 *
 * @param l1 Pointer to the first Eina_Inlist node (containing a Row_Calc).
 * @param l2 Pointer to the second Eina_Inlist node (containing a Row_Calc).
 * @return -1 if it1->weight_factor >= it2->weight_factor, 1 otherwise.
 */
static int
_row_weight_sort_cb(const void *l1, const void *l2)
{
   Row_Calc *it1, *it2;

   it1 = EINA_INLIST_CONTAINER_GET(l1, Row_Calc);
   it2 = EINA_INLIST_CONTAINER_GET(l2, Row_Calc);

   return it2->weight_factor <= it1->weight_factor ? -1 : 1;
}

/**
 * @brief Updates the layout of children in a flow manner.
 *
 * This function calculates the positions and sizes of all child objects
 * within the Efl_Ui_Box_Flow container. It arranges items in rows,
 * wrapping to new rows when the current row is full, based on available
 * space and item hints.
 *
 * The layout process involves several stages:
 * 1. Initialization: Gathers properties of all child items (min size, weight).
 * 2. Row Formation: Determines how items group into rows based on available width/height.
 *    - For homogeneous layout, items are distributed evenly.
 *    - For non-homogeneous, items are placed until the row is full, then a new row starts.
 * 3. Main Axis Sizing: Distributes space within each row along the main layout axis (horizontal or vertical).
 *    - Considers item weights and minimum sizes.
 *    - Uses a sorted list approach for weighted distribution if necessary.
 * 4. Cross Axis Sizing: Determines the size of each row along the cross axis.
 *    - Considers the maximum item size/weight in each row for the cross dimension.
 *    - Distributes available cross-axis space among rows, potentially weighted.
 * 5. Final Geometry: Calculates the final (x, y, width, height) for each child item.
 *    - Applies margins, alignment, and fill hints.
 *    - Sets the restricted minimum size for the container itself.
 *
 * @param obj The Efl_Ui_Box_Flow object.
 * @param pd Private data for Efl_Ui_Box_Flow (unused in this function).
 */
EOLIAN static void
_efl_ui_box_flow_efl_pack_layout_layout_update(Eo *obj, Efl_Ui_Box_Flow_Data *pd EINA_UNUSED)
{
   Efl_Ui_Box_Data *bd = efl_data_scope_get(obj, EFL_UI_BOX_CLASS);
   Eo *child;
   Eina_List *li;
   Eina_Inlist *inlist = NULL;
   Item_Calc *items, *item;
   Row_Calc *rows, *row;
   Efl_Ui_Container_Item_Hints *hints, *hint;
   Eina_Bool axis = !efl_ui_layout_orientation_is_horizontal(bd->dir, EINA_FALSE);
   Eina_Bool c_axis = !axis;
   int want[2] = { 0, 0 };
   int rc = 0, count, i = 0, id, item_last = 0;
   double cur_pos, cross_weight_sum = 0, cross_min_sum = 0, min_sum = 0;
   Efl_Ui_Container_Layout_Calc box_calc[2]; /* 0 is x-axis, 1 is y-axis */

   count = eina_list_count(bd->children);
   if (!count)
     {
        efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(0, 0));
        return;
     }

   _efl_ui_container_layout_init(obj, box_calc);

   items = alloca(count * sizeof(*items));
   rows = alloca(count * sizeof(*rows));
   memset(rows, 0, count * sizeof(*rows));

#ifdef DEBUG
   memset(items, 0, count * sizeof(*items));
#endif

   /*
    * Stage 1: Scan all items.
    * - Initialize Item_Calc for each child.
    * - Get layout hints (min size, weight, fill, align, margin, aspect).
    * - Determine initial row breaks and populate Row_Calc structures.
    * - Calculate initial min_sum (sum of minimum sizes along main axis) for each row.
    * - Calculate cross_weight and cross_space for each row.
    */
   EINA_LIST_FOREACH(bd->children, li, child)
     {
        item = &items[i++];
        item->obj = child;
        hints = item->hints;

        _efl_ui_container_layout_item_init(item->obj, hints);

        if ((bd->homogeneous && !axis) || box_calc[0].fill)
          hints[0].weight = 1;
        else if (hints[0].weight < 0)
          hints[0].weight = 0;

        if ((bd->homogeneous && axis) || box_calc[1].fill)
          hints[1].weight = 1;
        else if (hints[1].weight < 0)
          hints[1].weight = 0;

        if (want[axis] < hints[axis].space)
          want[axis] = hints[axis].space;

        if (bd->homogeneous)
          continue;

        if (i == 1)
          {
             min_sum = hints[axis].space;
          }
        else if (box_calc[axis].size < (min_sum + hints[axis].space + box_calc[axis].pad))
          {
             min_sum = hints[axis].space;
             rc++;
          }
        else
          {
             min_sum += (hints[axis].space + box_calc[axis].pad);
          }

        row = &rows[rc];
        item->row = row;

        if (row->cross_weight < hints[c_axis].weight)
          row->cross_weight = hints[c_axis].weight;
        if (row->cross_space < hints[c_axis].space)
          row->cross_space = hints[c_axis].space;
        row->weight_sum += hints[axis].weight;
        row->min_sum += hints[axis].space;
        row->item_count++;
     }

   /*
    * Stage 1.1: Initialize homogeneous properties if applicable.
    * If homogeneous layout is enabled, recalculate row breaks and properties
    * assuming all items in a row have the same main-axis size (want[axis]).
    */
   if (bd->homogeneous)
     {
        min_sum = 0;
        for (i = 0; i < count; i++)
          {
             item = &items[i];
             hints = items[i].hints;

             if (i == 0)
               {
                  min_sum = want[axis];
               }
             else if (box_calc[axis].size < (min_sum + want[axis] + box_calc[axis].pad))
               {
                  min_sum = want[axis];
                  rc++;
               }
             else
               {
                  min_sum += (want[axis] + box_calc[axis].pad);
               }

             row = &rows[rc];
             item->row = row;

             if (row->cross_weight < hints[c_axis].weight)
               row->cross_weight = hints[c_axis].weight;
             if (row->cross_space < hints[c_axis].space)
               row->cross_space = hints[c_axis].space;
             row->item_count++;
          }
     }

   /*
    * Stage 2: Calculate item space within each row (Main Axis Sizing).
    * For each row:
    * - Determine available space for items (box_size) after accounting for padding.
    * - If homogeneous, calculate hgsize (homogeneous size per item).
    * - If not homogeneous and items have weights:
    *   - Distribute remaining space (box_size - sum of min_sizes) among items
    *     proportionally to their weights. This involves a sorted list approach
    *     to handle cases where an item's weighted share is less than its min_size.
    * - If not homogeneous and no items have weights:
    *   - Align items within the row based on box_calc[axis].align.
    * - Accumulate cross_min_sum and cross_weight_sum for later cross-axis calculation.
    */
   for (id = 0, i = 0; id <= rc; id++)
     {
        int box_size;

        row = &rows[id];
        row->cur_pos = box_calc[axis].pos;

        box_size = box_calc[axis].size -
                   (box_calc[axis].pad * (row->item_count - 1));
        row->hgsize = box_size / row->item_count;

        cross_min_sum += row->cross_space;
        cross_weight_sum += row->cross_weight;
        item_last += row->item_count;

        if (bd->homogeneous)
          continue;

        if (row->weight_sum > 0)
          {
             int calc_size;
             double orig_weight = row->weight_sum;

             calc_size = box_size;
             inlist = NULL;

             for (; i < item_last; i++)
               {
                  double denom;
                  hint = &items[i].hints[axis];

                  denom = (hint->weight * box_size) - (orig_weight * hint->space);
                  if (denom > 0)
                    {
                       items[i].weight_factor = (hint->weight * box_size) / denom;
                       inlist = eina_inlist_sorted_insert(inlist, EINA_INLIST_GET(&items[i]),
                                                          _item_weight_sort_cb);

                    }
                  else
                    {
                       calc_size -= hint->space;
                       row->weight_sum -= hint->weight;
                    }
               }

             EINA_INLIST_FOREACH(inlist, item)
               {
                  double weight_len;
                  hint = &item->hints[axis];

                  weight_len = (calc_size * hint->weight) / row->weight_sum;
                  if (hint->space < weight_len)
                    {
                       hint->space = weight_len;
                    }
                  else
                    {
                       row->weight_sum -= hint->weight;
                       calc_size -= hint->space;
                    }
               }
          }
        else if (EINA_DBL_EQ(row->weight_sum, 0))
          {
             row->cur_pos += (box_size - row->min_sum) * box_calc[axis].align;
             i += row->item_count;
          }
     }

   /*
    * Stage 3: Calculate row space (Cross Axis Sizing).
    * - Adjust available cross-axis space by subtracting inter-row padding.
    * - If available space > sum of minimum cross sizes (cross_min_sum)
    *   and rows have cross-axis weights:
    *   - Distribute remaining space among rows proportionally to their cross_weights.
    *     Similar to item sizing, this uses a sorted list approach.
    * - If no weights or not enough space for weighted distribution:
    *   - Align rows within the container based on box_calc[c_axis].align.
    */
   box_calc[c_axis].size -= (box_calc[c_axis].pad * rc);
   cur_pos = box_calc[c_axis].pos;
   if ((box_calc[c_axis].size > cross_min_sum))
     {
        if (cross_weight_sum > 0)
          {
             int orig_size, calc_size;
             double orig_weight = cross_weight_sum;

             calc_size = orig_size = box_calc[c_axis].size;
             inlist = NULL;

             for (i = 0; i <= rc; i++)
               {
                  double denom;
                  row = &rows[i];

                  denom = (row->cross_weight * orig_size) -
                          (orig_weight * row->cross_space);
                  if (denom > 0)
                    {
                       row->weight_factor = (row->cross_weight * orig_size) / denom;
                       inlist = eina_inlist_sorted_insert(inlist, EINA_INLIST_GET(row),
                                                          _row_weight_sort_cb);

                    }
                  else
                    {
                       calc_size -= row->cross_space;
                       cross_weight_sum -= row->cross_weight;
                    }
               }

             EINA_INLIST_FOREACH(inlist, row)
               {
                  double weight_len;

                  weight_len = (calc_size * row->cross_weight) / cross_weight_sum;
                  if (row->cross_space < weight_len)
                    {
                       row->cross_space = weight_len;
                    }
                  else
                    {
                       cross_weight_sum -= row->cross_weight;
                       calc_size -= row->cross_space;
                    }
               }
          }
        else if (EINA_DBL_EQ(cross_weight_sum, 0))
          {
             cur_pos += (box_calc[c_axis].size - cross_min_sum) * box_calc[c_axis].align;
          }
     }

   /*
    * Stage 4: Calculate final item geometry.
    * Iterate through all items again:
    * - Update current cross-axis position (cur_pos) when moving to a new row.
    * - Set item's main-axis size (hints[axis].space):
    *   - If homogeneous, use row->hgsize.
    *   - Otherwise, it was calculated in Stage 2.
    * - Set item's cross-axis size (hints[c_axis].space) to row->cross_space.
    * - Calculate effective space (sw, sh) after subtracting margins.
    * - Determine final item_size based on fill hints.
    * - Apply min/max constraints and aspect ratio.
    * - Calculate item_pos based on row's current main-axis position (row->cur_pos),
    *   the overall cross-axis position (cur_pos), margins, and alignment.
    * - Update row->cur_pos for the next item in the same row.
    * - Set the geometry of the child Evas_Object.
    */
   int item_size[2], item_pos[2], sw, sh;

   row = NULL;
   for (i = 0; i < count; i++)
     {
        item = &items[i];
        hints = items[i].hints;

        if (row && (row != item->row))
          cur_pos += row->cross_space + box_calc[c_axis].pad;

        row = item->row;

        if (bd->homogeneous)
          hints[axis].space = row->hgsize;
        hints[c_axis].space = row->cross_space;
        sw = hints[0].space - (hints[0].margin[0] + hints[0].margin[1]);
        sh = hints[1].space - (hints[1].margin[0] + hints[1].margin[1]);

        item_size[0] = ((hints[0].weight > 0) && hints[0].fill) ? sw : 0;
        item_size[1] = ((hints[1].weight > 0) && hints[1].fill) ? sh : 0;

        _efl_ui_container_layout_min_max_calc(hints, &item_size[0], &item_size[1],
                                (hints[0].aspect > 0) && (hints[1].aspect > 0));

        item_pos[axis] = row->cur_pos + 0.5;
        item_pos[c_axis] = cur_pos + 0.5;

        item_pos[0] += (hints[0].margin[0] +
                        ((sw - item_size[0]) * hints[0].align));
        item_pos[1] += (hints[1].margin[0] +
                        ((sh - item_size[1]) * hints[1].align));

        row->cur_pos += hints[axis].space + box_calc[axis].pad;

        efl_gfx_entity_geometry_set(items[i].obj,
                                    EINA_RECT(item_pos[0], item_pos[1],
                                              item_size[0], item_size[1]));
     }

   /*
    * Stage 5: Finalize container size.
    * - Calculate the total required size (want[0], want[1]) for the container,
    *   including margins, padding between rows, and the sum of row cross_spaces.
    * - Set the restricted minimum size of the container.
    * - Emit layout updated event.
    */
   want[axis] += (box_calc[axis].margin[0] + box_calc[axis].margin[1]);
   want[c_axis] = (box_calc[c_axis].margin[0] + box_calc[c_axis].margin[1]) +
                  (box_calc[c_axis].pad * rc) + cross_min_sum;

   efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(want[0], want[1]));

   efl_event_callback_call(obj, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
}

#include "efl_ui_box_flow.eo.c"
