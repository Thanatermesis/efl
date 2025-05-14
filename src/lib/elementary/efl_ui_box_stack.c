#define EFL_GFX_HINT_PROTECTED

#include "efl_ui_box_private.h"
#include "efl_ui_container_layout.h"

#define MY_CLASS EFL_UI_BOX_STACK_CLASS

typedef struct _Item_Calc Item_Calc;

/**
 * @brief Internal structure to hold calculation data for each item in the box layout.
 *
 * This structure stores a pointer to the Evas_Object representing the item
 * and its layout hints for both X and Y axes.
 */
struct _Item_Calc
{
   Evas_Object *obj; /**< The Evas_Object (child item) being processed. */
   Efl_Ui_Container_Item_Hints hints[2]; /**< Layout hints for the item. Index 0 for X-axis, 1 for Y-axis.
                                          * hints[0].space: desired width
                                          * hints[0].margin: left and right margins
                                          * hints[0].align: horizontal alignment
                                          * hints[0].weight: horizontal weight
                                          * hints[0].fill: horizontal fill policy
                                          * hints[0].aspect: aspect ratio control
                                          * hints[1] follows the same pattern for the Y-axis (height, top/bottom margins, etc.)
                                          */
};

/**
 * @brief Updates the layout of the box stack.
 *
 * This function is called when the layout of the box needs to be recalculated.
 * It iterates over all child items, calculates their desired sizes and positions
 * based on their hints (margins, alignment, weight, fill, aspect ratio),
 * and then arranges them in a stack. All items will occupy the same area,
 * stacked on top of each other, with the last child added being on top.
 * The overall minimum size of the box is determined by the largest child's
 * dimensions, considering padding.
 *
 * @param obj The Efl_Ui_Box_Stack object.
 * @param _pd Private data for the Efl_Ui_Box_Stack class (unused in this function).
 */
EOLIAN static void
_efl_ui_box_stack_efl_pack_layout_layout_update(Eo *obj, void *_pd EINA_UNUSED)
{
   Efl_Ui_Box_Data *bd = efl_data_scope_get(obj, EFL_UI_BOX_CLASS);
   Eo *child;
   Efl_Ui_Container_Layout_Calc box_calc[2];
   Efl_Ui_Container_Item_Hints *hints;
   Item_Calc *items, *item;
   Eina_List *l;
   Eina_Size2D want = { 0, 0 };
   Evas_Object *old_child = NULL;
   int i = 0, count;

   count = eina_list_count(bd->children);
   // If there are no children, set the minimum size to zero and return.
   if (!count)
     {
        efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(0, 0));
        return;
     }

   // Initialize the box calculation data (e.g., padding, alignment for the box itself).
   _efl_ui_container_layout_init(obj, box_calc);

   // Allocate space on the stack for storing per-item calculation data.
   items = alloca(count * sizeof(*items));
#ifdef DEBUG
   memset(items, 0, count * sizeof(*items));
#endif

   // First pass: Iterate over children to initialize their layout hints
   // and determine the maximum desired space (want.w, want.h) among all children.
   EINA_LIST_FOREACH(bd->children, l, child)
     {
        item = &items[i++];
        item->obj = child;
        hints = item->hints;

        _efl_ui_container_layout_item_init(child, hints);

        if (want.w < hints[0].space)
          want.w = hints[0].space;
        if (want.h < hints[1].space)
          want.h = hints[1].space;
     }

   // The box's content area size is at least the maximum space wanted by any child.
   if (box_calc[0].size < want.w)
     box_calc[0].size = want.w;
   if (box_calc[1].size < want.h)
     box_calc[1].size = want.h;

   // Second pass: Calculate and apply geometry for each item.
   // All items in a stack layout share the same conceptual space,
   // determined by box_calc.size, adjusted for individual margins.
   for (i = 0; i < count; i++)
     {
        hints = items[i].hints;
        Eina_Rect item_geom;

        hints[0].space = box_calc[0].size -
                         (hints[0].margin[0] + hints[0].margin[1]);
        hints[1].space = box_calc[1].size -
                         (hints[1].margin[0] + hints[1].margin[1]);

        item_geom.w = ((hints[0].weight > 0) && hints[0].fill) ? hints[0].space : 0;
        item_geom.h = ((hints[1].weight > 0) && hints[1].fill) ? hints[1].space : 0;

        _efl_ui_container_layout_min_max_calc(hints, &item_geom.w, &item_geom.h,
                                (hints[0].aspect > 0) && (hints[1].aspect > 0));

        item_geom.x = box_calc[0].pos + hints[0].margin[0] +
                      (hints[0].space - item_geom.w) * hints[0].align;
        item_geom.y = box_calc[1].pos + hints[1].margin[0] +
                      (hints[1].space - item_geom.h) * hints[1].align;

        efl_gfx_entity_geometry_set(items[i].obj, item_geom);

        // Stack items on top of each other. The current item is stacked above the previous one.
        // This ensures that the last item in the children list appears on top.
        if (old_child)
          efl_gfx_stack_above(items[i].obj, old_child);
        old_child = items[i].obj;
     }

   // The total minimum size required by the box includes its own margins/padding.
   want.w += (box_calc[0].margin[0] + box_calc[0].margin[1]);
   want.h += (box_calc[1].margin[0] + box_calc[1].margin[1]);

   // Set the calculated minimum size for the box widget.
   efl_gfx_hint_size_restricted_min_set(obj, want);

   // Notify that the layout has been updated.
   efl_event_callback_call(obj, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
}

#include "efl_ui_box_stack.eo.c"
