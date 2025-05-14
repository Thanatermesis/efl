#define EFL_GFX_HINT_PROTECTED

#include "efl_ui_box_private.h"
#include "efl_ui_container_layout.h"

// FIXME: handle RTL? just invert the horizontal order?

typedef struct _Item_Calc Item_Calc;

/**
 * @internal
 * @brief Internal structure to hold calculation data for each item in the box.
 *
 * This structure is used during the layout process to store properties
 * and intermediate calculation results for each child object.
 */
struct _Item_Calc
{
   EINA_INLIST; /**< Macro for Eina_Inlist node integration. */

   Evas_Object *obj; /**< The child Evas_Object. */
   double weight_factor; /**< Factor used for sorting items by weight during space distribution. */
   Efl_Ui_Container_Item_Hints hints[2]; /**< Layout hints for x-axis (index 0) and y-axis (index 1).
                                         *   Each Efl_Ui_Container_Item_Hints contains:
                                         *   - double weight: The weight of the item.
                                         *   - int space: The minimum space required by the item.
                                         *   - int margin[2]: Margins (start, end).
                                         *   - double align: Alignment of the item.
                                         *   - Eina_Bool fill: Whether the item should fill available space.
                                         *   - double aspect: Aspect ratio hint.
                                         */
};

/**
 * @internal
 * @brief Comparison function for sorting Item_Calc structures by weight_factor.
 *
 * Used with eina_inlist_sorted_insert to sort items in descending order
 * of their weight_factor. This helps in distributing extra space proportionally.
 *
 * @param l1 Pointer to the first Eina_Inlist node (containing an Item_Calc).
 * @param l2 Pointer to the second Eina_Inlist node (containing an Item_Calc).
 * @return -1 if it1->weight_factor >= it2->weight_factor, 1 otherwise.
 */
static int
_weight_sort_cb(const void *l1, const void *l2)
{
   Item_Calc *it1, *it2;

   it1 = EINA_INLIST_CONTAINER_GET(l1, Item_Calc);
   it2 = EINA_INLIST_CONTAINER_GET(l2, Item_Calc);

   return it2->weight_factor <= it1->weight_factor ? -1 : 1;
}

/* this function performs a simplified layout when the box has changed position
 * but no other changes have occurred, e.g., when a box is being scrolled
 */
/**
 * @internal
 * @brief Performs a simplified layout update.
 *
 * This function is called when the box's position changes, but no other
 * layout-affecting properties (like child sizes, weights, or orientation)
 * have changed. It simply translates the child items by the same delta
 * as the box itself. This is an optimization, for example, during scrolling.
 *
 * @param ui_box The Efl_Ui_Box object.
 * @param pd The private data of the Efl_Ui_Box object.
 */
static void
_efl_ui_box_layout_simple(Efl_Ui_Box *ui_box, Efl_Ui_Box_Data *pd)
{
   Eo *child;
   Eina_List *l;
   Eina_Position2D pos = efl_gfx_entity_position_get(ui_box);

   EINA_LIST_FOREACH(pd->children, l, child)
     {
        Eina_Position2D child_pos = efl_gfx_entity_position_get(child);

        efl_gfx_entity_position_set(child,
          EINA_POSITION2D(pos.x - pd->last_pos.x + child_pos.x,
                          pos.y - pd->last_pos.y + child_pos.y));
     }
   pd->last_pos = pos;
   efl_event_callback_call(ui_box, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
}

/**
 * @internal
 * @brief Performs a full custom layout calculation for the box.
 *
 * This function calculates the size and position of all child items within
 * the box based on their hints (weight, alignment, fill, etc.), the box's
 * orientation, padding, and available space. It handles homogeneous mode,
 * weight distribution, and aspect ratio constraints.
 *
 * @param ui_box The Efl_Ui_Box object.
 * @param pd The private data of the Efl_Ui_Box object.
 */
void
_efl_ui_box_custom_layout(Efl_Ui_Box *ui_box, Efl_Ui_Box_Data *pd)
{
   Eo *child;
   Eina_List *li;
   Eina_Inlist *inlist = NULL;
   Item_Calc *items, *item;
   Efl_Ui_Container_Item_Hints *hints, *hint;
   Eina_Bool axis = !efl_ui_layout_orientation_is_horizontal(pd->dir, EINA_FALSE);
   Eina_Bool r_axis = !axis;
   int want[2] = { 0, 0 };
   int count, i = 0;
   double cur_pos, mmin = 0, weight_sum = 0;
   Efl_Ui_Container_Layout_Calc box_calc[2]; /* 0 is x-axis, 1 is y-axis */


   count = eina_list_count(pd->children);
   if (!count)
     {
        efl_gfx_hint_size_restricted_min_set(ui_box, EINA_SIZE2D(0, 0));
        return;
     }
   if (!pd->full_recalc)
     {
        _efl_ui_box_layout_simple(ui_box, pd);
        return;
     }
   _efl_ui_container_layout_init(ui_box, box_calc);
   pd->last_pos.x = box_calc[0].pos - box_calc[0].margin[0];
   pd->last_pos.y = box_calc[1].pos - box_calc[1].margin[0];

   /* Item_Calc struct is currently 152 bytes.
    * this is pretty big to be allocating a huge number of, and we don't want to explode the stack
    */
   // Allocate Item_Calc array: use stack for small counts, heap for large counts
   // to avoid stack overflow while maintaining performance for common cases.
   if (count >= 500)
     {
        items = malloc(count * sizeof(*items));
        EINA_SAFETY_ON_NULL_RETURN(items);
     }
   else
     items = alloca(count * sizeof(*items));
#ifdef DEBUG
   memset(items, 0, count * sizeof(*items));
#endif

   // Phase 1: Scan all items, initialize their Item_Calc structures,
   // and calculate initial total weight and minimum required space.
   EINA_LIST_FOREACH(pd->children, li, child)
     {
        item = &items[i++];
        item->obj = child;
        hints = item->hints;

        _efl_ui_container_layout_item_init(item->obj, hints);

        if (pd->homogeneous || box_calc[0].fill)
          hints[0].weight = 1;
        else if (hints[0].weight < 0)
          hints[0].weight = 0;

        if (pd->homogeneous || box_calc[1].fill)
          hints[1].weight = 1;
        else if (hints[1].weight < 0)
          hints[1].weight = 0;

        weight_sum += hints[axis].weight;

        if (hints[r_axis].space > want[r_axis])
          want[r_axis] = hints[r_axis].space;

        if (pd->homogeneous)
          {
             if (hints[axis].space > mmin)
               mmin = hints[axis].space;
          }
        else
          {
             want[axis] += hints[axis].space;
          }
     }

   // Phase 2: Calculate total required space (`want`) and adjust available
   // space in `box_calc` based on padding and homogeneous mode.
   if (pd->homogeneous)
     want[axis] = mmin * count; // In homogeneous mode, all items get the size of the largest min item.

   // Ensure the cross-axis size of the box is at least the minimum required by items.
   if (box_calc[r_axis].size < want[r_axis])
     box_calc[r_axis].size = want[r_axis];

   // Subtract inter-item padding from the available size along the main axis.
   // Cross-axis padding is handled by item alignment and margins.
   box_calc[axis].size -= (box_calc[axis].pad * (count - 1));
   box_calc[r_axis].pad = 0; // No inter-item padding on the cross axis for box layout.
   cur_pos = box_calc[axis].pos; // Starting position for item placement.

   // Phase 3: Distribute extra space among items based on their weights.
   // This is done only if not in homogeneous mode, there's extra space, and total weight > 0.
   if (!pd->homogeneous && (box_calc[axis].size > want[axis]) && (weight_sum > 0))
     {
        int orig_size, calc_size;
        double orig_weight = weight_sum;

        calc_size = orig_size = box_calc[axis].size;

        for (i = 0; i < count; i++)
          {
             double denom;
             hint = &items[i].hints[axis];

             denom = (hint->weight * orig_size) - (orig_weight * hint->space);
             if (denom > 0)
               {
                  items[i].weight_factor = (hint->weight * orig_size) / denom;
                  inlist = eina_inlist_sorted_insert(inlist, EINA_INLIST_GET(&items[i]),
                                                     _weight_sort_cb);

               }
             else
               {
                  calc_size -= hint->space;
                  weight_sum -= hint->weight;
               }
          }

        EINA_INLIST_FOREACH(inlist, item)
          {
             double weight_len;
             hint = &item->hints[axis];

             weight_len = (calc_size * hint->weight) / weight_sum;
             if (hint->space < weight_len)
               {
                  hint->space = weight_len;
               }
             else
               {
                  weight_sum -= hint->weight;
                  calc_size -= hint->space;
               }
          }
     }

   // Phase 4: Calculate final item geometry (position and size).
     {
        int item_size[2], item_pos[2], sw, sh; // sw/sh: space for item after margins

        // Adjust starting position if there's extra space and no weights (or homogeneous)
        // to honor box alignment.
        if (box_calc[axis].size > want[axis])
          {
             if (pd->homogeneous)
               mmin = (double)box_calc[axis].size / count; // Homogeneous items share space equally.
             else if (EINA_DBL_EQ(weight_sum, 0)) // No weights, distribute extra space by alignment.
               cur_pos += (box_calc[axis].size - want[axis]) * box_calc[axis].align;
          }

        for (i = 0; i < count; i++)
          {
             hints = items[i].hints;

             if (pd->homogeneous)
               hints[axis].space = mmin; // Assign calculated homogeneous size.
             // Assign full cross-axis size, alignment will position the item within this.
             hints[r_axis].space = box_calc[r_axis].size;
             // Calculate available space for item content after subtracting its margins.
             sw = hints[0].space - (hints[0].margin[0] + hints[0].margin[1]);
             sh = hints[1].space - (hints[1].margin[0] + hints[1].margin[1]);

             // Initial item size based on fill hints. If not filling, size is 0 and
             // _efl_ui_container_layout_min_max_calc will use min/max hints.
             item_size[0] = ((hints[0].weight > 0) && hints[0].fill) ? sw : 0;
             item_size[1] = ((hints[1].weight > 0) && hints[1].fill) ? sh : 0;

             // Apply min/max constraints and aspect ratio.
             _efl_ui_container_layout_min_max_calc(hints, &item_size[0], &item_size[1],
                                (hints[0].aspect > 0) && (hints[1].aspect > 0));

             // Set initial position based on current packing position and box's cross-axis position.
             item_pos[axis] = cur_pos + 0.5; // +0.5 for rounding to nearest pixel.
             item_pos[r_axis] = box_calc[r_axis].pos;

             // Adjust position based on item margins and alignment within its allocated space.
             item_pos[0] += (hints[0].margin[0] +
                             ((sw - item_size[0]) * hints[0].align));
             item_pos[1] += (hints[1].margin[0] +
                             ((sh - item_size[1]) * hints[1].align));

             // Advance current packing position for the next item.
             cur_pos += hints[axis].space + box_calc[axis].pad;

             // Apply calculated geometry to the child object.
             efl_gfx_entity_geometry_set(items[i].obj,
                                         EINA_RECT(item_pos[0], item_pos[1],
                                                   item_size[0], item_size[1]));
          }
     }
   // Phase 5: Calculate the total minimum size required by the box,
   // including its own margins and padding.
   want[0] += (box_calc[0].margin[0] + box_calc[0].margin[1]) +
              (box_calc[0].pad * (count - 1));
   want[1] += (box_calc[1].margin[0] + box_calc[1].margin[1]) +
              (box_calc[1].pad * (count - 1));

   pd->full_recalc = EINA_FALSE; // Reset full recalculation flag.
   efl_gfx_hint_size_restricted_min_set(ui_box, EINA_SIZE2D(want[0], want[1]));

   efl_event_callback_call(ui_box, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
   if (count >= 500) free(items);
}
