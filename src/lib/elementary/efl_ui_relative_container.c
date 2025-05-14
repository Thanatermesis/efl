#include "efl_ui_relative_container_private.h"

/**
 * @def MY_CLASS
 * @brief Alias for the Efl.Ui.Relative_Container class.
 */
#define MY_CLASS EFL_UI_RELATIVE_CONTAINER_CLASS
#define MY_CLASS_NAME "Efl.Ui.Relative_Container"

/** @brief Index for the left side of a child object. */
#define LEFT               0
/** @brief Index for the right side of a child object. */
#define RIGHT              1
/** @brief Index for the top side of a child object. */
#define TOP                2
/** @brief Index for the bottom side of a child object. */
#define BOTTOM             3

/** @brief Macro to determine the start side (TOP or LEFT) based on the axis. */
#define START              (axis ? TOP : LEFT)
/** @brief Macro to determine the end side (BOTTOM or RIGHT) based on the axis. */
#define END                (axis ? BOTTOM : RIGHT)

/**
 * @brief Calculates the position and size of a child object along a specific axis.
 * @param child The child object to calculate.
 * @param axis The axis (0 for horizontal, 1 for vertical) to calculate.
 */
static void _child_calc(Efl_Ui_Relative_Container_Child *child, Eina_Bool axis);

/**
 * @brief Comparison function for sorting child calculation data in a chain.
 *
 * This function is used to sort Efl_Ui_Relative_Container_Calc structures
 * based on their `comp_factor` in ascending order.
 *
 * @param l1 Pointer to the first Eina_Inlist node containing Efl_Ui_Relative_Container_Calc.
 * @param l2 Pointer to the second Eina_Inlist node containing Efl_Ui_Relative_Container_Calc.
 * @return -1 if calc1->comp_factor is less than or equal to calc2->comp_factor, 1 otherwise.
 */
static int
_chain_sort_cb(const void *l1, const void *l2)
{
   Efl_Ui_Relative_Container_Calc *calc1, *calc2;

   calc1 = EINA_INLIST_CONTAINER_GET(l1, Efl_Ui_Relative_Container_Calc);
   calc2 = EINA_INLIST_CONTAINER_GET(l2, Efl_Ui_Relative_Container_Calc);

   return calc2->comp_factor <= calc1->comp_factor ? -1 : 1;
}

/**
 * @brief Callback invoked when a child object's size changes.
 *
 * Triggers a layout request for the container.
 *
 * @param data The Efl_Ui_Relative_Container object.
 * @param event The Efl_Event details (unused).
 */
static void
_on_child_size_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Relative_Container *obj = data;

   efl_pack_layout_request(obj);
}

/**
 * @brief Callback invoked when a child object's hints change.
 *
 * Triggers a layout request for the container.
 *
 * @param data The Efl_Ui_Relative_Container object.
 * @param event The Efl_Event details (unused).
 */
static void
_on_child_hints_changed(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Relative_Container *obj = data;

   efl_pack_layout_request(obj);
}

/**
 * @brief Callback invoked when a child object is deleted.
 *
 * Unpacks the child from the container.
 *
 * @param data The Efl_Ui_Relative_Container object.
 * @param event The Efl_Event details, where event->object is the child being deleted.
 */
static void
_on_child_del(void *data, const Efl_Event *event)
{
   Efl_Ui_Relative_Container *obj = data;

   efl_pack_unpack(obj, event->object);
}

/**
 * @brief Defines an array of callbacks for child objects within the relative container.
 *
 * This array maps specific events from child objects (size changed, hints changed, deletion)
 * to their respective handler functions (_on_child_size_changed, _on_child_hints_changed, _on_child_del).
 */
EFL_CALLBACKS_ARRAY_DEFINE(efl_ui_relative_container_callbacks,
  { EFL_GFX_ENTITY_EVENT_SIZE_CHANGED, _on_child_size_changed },
  { EFL_GFX_ENTITY_EVENT_HINTS_CHANGED, _on_child_hints_changed },
  { EFL_EVENT_DEL, _on_child_del }
);

/**
 * @brief Registers a new child object with the relative container.
 *
 * Initializes the child's relative layout properties, sets up event callbacks,
 * and adds it to the container's internal tracking.
 *
 * @param pd The private data of the Efl_Ui_Relative_Container.
 * @param child The Eo object to be registered as a child.
 * @return A pointer to the newly created Efl_Ui_Relative_Container_Child structure
 *         for the registered child, or NULL on failure.
 */
static Efl_Ui_Relative_Container_Child *
_efl_ui_relative_container_register(Efl_Ui_Relative_Container_Data *pd, Eo *child)
{
   Efl_Ui_Relative_Container_Child *rc;

   if (!efl_ui_widget_sub_object_add(pd->obj, child))
     return NULL;

   rc = calloc(1, sizeof(Efl_Ui_Relative_Container_Child));
   if (!rc) return NULL;

   rc->obj = child;
   rc->layout = pd->obj;
   rc->rel[LEFT].to = rc->layout;
   rc->rel[LEFT].relative_position = 0.0;
   rc->rel[RIGHT].to = rc->layout;
   rc->rel[RIGHT].relative_position = 1.0;
   rc->rel[TOP].to = rc->layout;
   rc->rel[TOP].relative_position = 0.0;
   rc->rel[BOTTOM].to = rc->layout;
   rc->rel[BOTTOM].relative_position = 1.0;

   efl_key_data_set(child, "_elm_leaveme", pd->obj);
   efl_canvas_object_clipper_set(child, pd->clipper);
   efl_event_callback_array_add(child, efl_ui_relative_container_callbacks(), pd->obj);
   efl_canvas_group_member_add(pd->obj, child);
   efl_canvas_group_change(pd->obj);

   eina_hash_add(pd->children, &child, rc);

   return rc;
}

/**
 * @brief Retrieves or registers a child object.
 *
 * If the child is already registered, its Efl_Ui_Relative_Container_Child data is returned.
 * Otherwise, the child is registered first.
 *
 * @param pd The private data of the Efl_Ui_Relative_Container.
 * @param child The Eo object to get or register.
 * @return A pointer to the Efl_Ui_Relative_Container_Child structure for the child.
 */
static Efl_Ui_Relative_Container_Child *
_relative_child_get(Efl_Ui_Relative_Container_Data *pd, Eo *child)
{
   Efl_Ui_Relative_Container_Child *rc;

   rc = eina_hash_find(pd->children, &child);
   if (!rc)
     rc = _efl_ui_relative_container_register(pd, child);

   return rc;
}

/**
 * @brief Finds a registered child object or the container itself.
 *
 * If the target is the container, returns the container's base child data.
 * If the target is a registered child, returns its data.
 * If the target is not found, logs an error and returns the container's base child data.
 *
 * @param pd The private data of the Efl_Ui_Relative_Container.
 * @param target The Eo object to find. This can be a child or the container itself.
 * @return A pointer to the Efl_Ui_Relative_Container_Child structure for the target.
 */
static Efl_Ui_Relative_Container_Child *
_relative_child_find(Efl_Ui_Relative_Container_Data *pd, Eo *target)
{
   Efl_Ui_Relative_Container_Child *child;

   if (pd->obj == target)
     return pd->base;

   child = eina_hash_find(pd->children, &target);
   if (!child)
     {
        ERR("target(%p(%s)) is not registered", target, efl_class_name_get(target));
        child = pd->base;
     }

   return child;
}

/**
 * @brief Calculates the child's size based on its aspect ratio hints.
 *
 * This function adjusts the `want` (desired) dimensions of the child
 * according to its aspect ratio settings (horizontal, vertical, both, or none)
 * and min/max size hints. It ensures the calculated size respects these constraints.
 *
 * @param child The child object whose aspect-related calculations are to be performed.
 * @param axis The primary axis (0 for horizontal, 1 for vertical) for which
 *             the calculation is being initiated. The function might recursively
 *             call _child_calc for the other axis if needed.
 */
static void
_child_aspect_calc(Efl_Ui_Relative_Container_Child *child, Eina_Bool axis)
{
   Efl_Ui_Relative_Container_Calc *calc = &child->calc;
   double temph;

   if ((calc->aspect[0] <= 0) || (calc->aspect[1] <= 0))
     {
        ERR("Invalid aspect parameter for obj(%p), aspect(%d, %d) ",
            child->obj, calc->aspect[0], calc->aspect[1]);
        return;
     }

   switch (calc->aspect_type)
     {
      case EFL_GFX_HINT_ASPECT_HORIZONTAL:
        if (axis) _child_calc(child, !axis);
        calc->want[1].length = calc->want[0].length * calc->aspect[1] / calc->aspect[0];
        break;
      case EFL_GFX_HINT_ASPECT_VERTICAL:
        if (!axis) _child_calc(child, !axis);
        calc->want[0].length = calc->want[1].length * calc->aspect[0] / calc->aspect[1];
        break;
      case EFL_GFX_HINT_ASPECT_BOTH:
        if (calc->state[!axis] != RELATIVE_CALC_ON)
          _child_calc(child, !axis);
        temph = calc->want[axis].length * calc->aspect[!axis] / calc->aspect[axis];
        if (temph > calc->want[!axis].length)
          {
             temph = calc->want[!axis].length;
             calc->want[axis].length = temph * calc->aspect[axis] / calc->aspect[!axis];
          }
        else
          calc->want[!axis].length = temph;
        break;
      default:
        if (calc->state[!axis] != RELATIVE_CALC_ON)
          _child_calc(child, !axis);
        temph = calc->want[axis].length * calc->aspect[!axis] / calc->aspect[axis];
        if (temph < calc->want[!axis].length)
          {
             temph = calc->want[!axis].length;
             calc->want[axis].length = temph * calc->aspect[axis] / calc->aspect[!axis];
          }
        else
          calc->want[!axis].length = temph;
     }

   //calculate max size
   if (calc->want[0].length > calc->max[0])
     {
        calc->want[0].length = calc->max[0];
        calc->want[1].length = calc->want[0].length * calc->aspect[1] / calc->aspect[0];
     }
   if (calc->want[1].length > calc->max[1])
     {
        calc->want[1].length = calc->max[1];
        calc->want[0].length = calc->want[1].length * calc->aspect[0] / calc->aspect[1];
     }
   //calculate min size
   if (calc->aspect[1] > calc->aspect[0])
     calc->min[1] = calc->min[0] * calc->aspect[1] / calc->aspect[0];
   else
     calc->min[0] = calc->min[1] * calc->aspect[0] / calc->aspect[1];

   if (calc->want[0].length < calc->min[0])
     {
        calc->want[0].length = calc->min[0];
        calc->want[1].length = calc->want[0].length * calc->aspect[1] / calc->aspect[0];
     }
   if (calc->want[1].length < calc->min[1])
     {
        calc->want[1].length = calc->min[1];
        calc->want[0].length = calc->want[1].length * calc->aspect[0] / calc->aspect[1];
     }

   //calculate align
   calc->want[!axis].position =
      calc->space[!axis].position +
      (calc->space[!axis].length - calc->want[!axis].length) * calc->align[!axis];
}

/**
 * @brief Calculates the layout for a chain of interconnected child objects along a specific axis.
 *
 * A chain is a sequence of children where each child's end is connected to the next child's start.
 * This function determines the available space for the chain and distributes it among the
 * children based on their weights, minimum sizes, and aspect ratios.
 *
 * @param child A child object within the chain.
 * @param axis The axis (0 for horizontal, 1 for vertical) along which the chain is being calculated.
 * @return EINA_TRUE if the chain calculation was completed or already done, EINA_FALSE if the
 *         provided child is not part of a valid chain structure for calculation at this moment
 *         (i.e., it's not a bidirectional link in the chain).
 */
static Eina_Bool
_child_chain_calc(Efl_Ui_Relative_Container_Child *child, Eina_Bool axis)
{
   Efl_Ui_Relative_Container_Child *head, *tail, *o;
   Efl_Gfx_Hint_Aspect aspect_type;
   int space, min_sum = 0;
   double weight_sum = 0, cur_pos;
   Eina_Inlist *chain = NULL;

   if (child->calc.chain_state[axis] == RELATIVE_CALC_DONE)
     return EINA_TRUE;

   if ((child != child->calc.to[START]->calc.to[END]) &&
       (child != child->calc.to[END]->calc.to[START]))
     return EINA_FALSE;

   // find head
   head = child;
   while (head == head->calc.to[START]->calc.to[END])
     {
        head = head->calc.to[START];
        if (head == child)
          {
             ERR("%c-axis circular dependency when calculating \"%s\"(%p).",
                 axis ? 'Y' : 'X', efl_class_name_get(child->obj), child->obj);
             return EINA_TRUE;
          }
     }

   //calculate weight_sum
   aspect_type = !axis ? EFL_GFX_HINT_ASPECT_VERTICAL : EFL_GFX_HINT_ASPECT_HORIZONTAL;
   o = head;
   do
     {
        if ((o->calc.aspect[0] > 0) && (o->calc.aspect[1] > 0) &&
            (o->calc.aspect_type == aspect_type))
          {
             _child_calc(o, !axis);
             if (o->calc.want[axis].length > o->calc.min[axis])
               o->calc.min[axis] = o->calc.want[axis].length;
          }
        else if ((o->calc.aspect[0] <= 0) ^ (o->calc.aspect[1] <= 0))
          {
             ERR("Invalid aspect parameter for obj(%p), aspect(%d, %d) ",
                 o->obj, o->calc.aspect[0], o->calc.aspect[1]);
          }

        o->calc.space[axis].length = o->calc.min[axis] +
                                     o->calc.margin[START] + o->calc.margin[END];
        min_sum += o->calc.space[axis].length;
        weight_sum += o->calc.weight[axis];

        tail = o;
        o = o->calc.to[END];
     }
   while (o->calc.to[START] == tail);

   _child_calc(head->calc.to[START], axis);
   _child_calc(tail->calc.to[END], axis);

   cur_pos = head->calc.to[START]->calc.want[axis].position +
             (head->calc.to[START]->calc.want[axis].length * head->rel[START].relative_position);
   space = tail->calc.to[END]->calc.want[axis].position +
           (tail->calc.to[END]->calc.want[axis].length * tail->rel[END].relative_position) - cur_pos;

   if ((space <= min_sum) || EINA_DBL_EQ(weight_sum, 0.0))
     cur_pos += (space - min_sum) * head->calc.align[axis];
   else
     {
        Efl_Ui_Relative_Container_Calc *calc;
        double weight_len, orig_space = space, orig_weight = weight_sum;

        // Calculate compare factor
        for (o = head; o != tail->calc.to[END]; o = o->calc.to[END])
          {
             double denom;

             calc = &o->calc;
             denom = (calc->weight[axis] * orig_space) - (orig_weight * calc->min[axis]);
             if (denom > 0)
               {
                  calc->comp_factor = (calc->weight[axis] * orig_space) / denom;
                  chain = eina_inlist_sorted_insert(chain, EINA_INLIST_GET(calc),
                                                    _chain_sort_cb);
               }
             else
               {
                  space -= calc->space[axis].length;
                  weight_sum -= calc->weight[axis];
               }
          }

        EINA_INLIST_FOREACH(chain, calc)
          {
             weight_len = (space * calc->weight[axis]) / weight_sum;

             if (calc->space[axis].length < weight_len)
               calc->space[axis].length = weight_len;
             else
               {
                  weight_sum -= calc->weight[axis];
                  space -= calc->space[axis].length;
               }
          }
     }

   for (o = head; o != tail->calc.to[END]; o = o->calc.to[END])
     {
        o->calc.space[axis].position = cur_pos + o->calc.margin[START] + 0.5;
        cur_pos += o->calc.space[axis].length;
        o->calc.space[axis].length -= o->calc.margin[START] + o->calc.margin[END];
        o->calc.chain_state[axis] = RELATIVE_CALC_DONE;
        child->calc.m0[axis] += o->calc.min[axis];
     }

   child->calc.mi[axis] = head->rel[START].relative_position * (head->calc.to[START]->calc.mj[axis] -
                    head->calc.to[START]->calc.mi[axis]) + head->calc.to[START]->calc.mi[axis];
   child->calc.mj[axis] = tail->rel[END].relative_position * (tail->calc.to[END]->calc.mj[axis] -
                    tail->calc.to[END]->calc.mi[axis]) + tail->calc.to[END]->calc.mi[axis];
   child->calc.m0[axis] += -child->calc.min[axis] +
            (head->calc.to[START]->calc.m0[axis] * head->rel[START].relative_position) +
            (tail->calc.to[END]->calc.m0[axis] * (1 - tail->rel[END].relative_position));

   return EINA_TRUE;
}

/**
 * @brief Recursively calculates the position and size of a child object along a specific axis.
 *
 * This is the core layout calculation function. It handles dependencies between children,
 * resolves chains, applies aspect ratios, and respects fill/alignment hints.
 * It uses a state machine (RELATIVE_CALC_NONE, RELATIVE_CALC_ON, RELATIVE_CALC_DONE)
 * to detect and report circular dependencies.
 *
 * @param child The child object to calculate.
 * @param axis The axis (0 for horizontal, 1 for vertical) to calculate.
 */
static void
_child_calc(Efl_Ui_Relative_Container_Child *child, Eina_Bool axis)
{
   Efl_Ui_Relative_Container_Calc *calc = &child->calc;

   if (calc->state[axis] == RELATIVE_CALC_DONE)
     return;

   if (calc->state[axis] == RELATIVE_CALC_ON)
     {
        ERR("%c-axis circular dependency when calculating part \"%s\"(%p).",
            axis ? 'Y' : 'X', efl_class_name_get(child->obj), child->obj);
        return;
     }

   calc->state[axis] = RELATIVE_CALC_ON;

   if (!_child_chain_calc(child, axis))
     {
        _child_calc(calc->to[START], axis);
        _child_calc(calc->to[END], axis);

        calc->space[axis].position = calc->to[START]->calc.want[axis].position
                        + (calc->to[START]->calc.want[axis].length * child->rel[START].relative_position)
                        + calc->margin[START];
        calc->space[axis].length = calc->to[END]->calc.want[axis].position
                        + (calc->to[END]->calc.want[axis].length * child->rel[END].relative_position)
                        - calc->margin[END] - calc->space[axis].position;
     }

   if (calc->fill[axis] && (calc->weight[axis] > 0))
     calc->want[axis].length = calc->space[axis].length;

   if (!calc->aspect[0] && !calc->aspect[1])
     {
        if (calc->want[axis].length > calc->max[axis])
          calc->want[axis].length = calc->max[axis];

        if (calc->want[axis].length < calc->min[axis])
          calc->want[axis].length = calc->min[axis];
     }
   else
     {
        _child_aspect_calc(child, axis);
     }

   //calculate align
   calc->want[axis].position =
      calc->space[axis].position +
      (calc->space[axis].length - calc->want[axis].length) * calc->align[axis];

   child->calc.state[axis] = RELATIVE_CALC_DONE;
   if (child->calc.chain_state[axis] == RELATIVE_CALC_DONE)
     return;

   //calculate relative layout min
   calc->mi[axis] = child->rel[START].relative_position * (calc->to[START]->calc.mj[axis] -
                    calc->to[START]->calc.mi[axis]) + calc->to[START]->calc.mi[axis];
   calc->mj[axis] = child->rel[END].relative_position * (calc->to[END]->calc.mj[axis] -
                    calc->to[END]->calc.mi[axis]) + calc->to[END]->calc.mi[axis];
   calc->m0[axis] = calc->to[START]->calc.m0[axis] * child->rel[START].relative_position;

   if ((calc->to[START] == calc->to[END]) &&
       EINA_DBL_EQ(child->rel[START].relative_position, child->rel[END].relative_position))
     {
        double r, a; // relative, align
        r = calc->mi[axis] +
           (child->rel[START].relative_position * (calc->mj[axis] - calc->mi[axis]));
        a = calc->align[axis];
        calc->m0[axis] += (calc->min[axis] + calc->margin[START] + calc->margin[END]) *
           ((EINA_DBL_EQ(r, 0.0) || (!EINA_DBL_EQ(r, 1.0) && (a < r))) ?
            ((1 - a) / (1 - r)) : (a / r));
     }
   else
     {
        calc->m0[axis] += calc->to[END]->calc.m0[axis] * (1 - child->rel[END].relative_position);
     }

}

/**
 * @brief Callback function used by eina_hash to free a Efl_Ui_Relative_Container_Child.
 *
 * This function is called when a child is removed from the `pd->children` hash.
 * It performs necessary cleanup, such as removing the child from the canvas group,
 * resetting its clipper, removing event callbacks, and finally freeing the
 * Efl_Ui_Relative_Container_Child structure itself.
 *
 * @param data Pointer to the Efl_Ui_Relative_Container_Child to be freed.
 */
static void
_hash_free_cb(void *data)
{
   Efl_Ui_Relative_Container_Child *child = data;

   efl_canvas_group_member_remove(child->layout, child->obj);
   efl_canvas_object_clipper_set(child->obj, NULL);
   efl_key_data_set(child->obj, "_elm_leaveme", NULL);
   efl_event_callback_array_del(child->obj, efl_ui_relative_container_callbacks(),
                                child->layout);

   if (!efl_invalidated_get(child->obj))
     _elm_widget_sub_object_redirect_to_top(child->layout, child->obj);

   free(child);
}

/**
 * @brief Callback function used by eina_hash when clearing all children (pack_clear).
 *
 * This function is similar to _hash_free_cb but is used specifically during
 * a `pack_clear` operation. It removes event callbacks and then deletes the
 * child object itself using efl_del(). The Efl_Ui_Relative_Container_Child
 * structure will be freed by the regular _hash_free_cb when eina_hash iterates.
 *
 * @param data Pointer to the Efl_Ui_Relative_Container_Child whose Eo object is to be deleted.
 */
static void
_hash_clear_cb(void *data)
{
   Efl_Ui_Relative_Container_Child *child = data;

   efl_event_callback_array_del(child->obj, efl_ui_relative_container_callbacks(),
                                child->layout);
   efl_del(child->obj);
}

/**
 * @brief Callback function for eina_hash_foreach to perform layout calculations for each child.
 *
 * This function is called for every child in the container during the layout update process.
 * It triggers the core `_child_calc` for both axes for the current child.
 * After calculation, it updates the container's overall minimum required size based on
 * the child's calculated minimum dimensions and relative positioning. Finally, it applies
 * the calculated geometry (position and size) to the child object.
 *
 * @param hash The Eina_Hash being iterated (unused).
 * @param key The key of the hash item (unused).
 * @param data Pointer to the Efl_Ui_Relative_Container_Child for the current child.
 * @param fdata Pointer to the Efl_Ui_Relative_Container_Data of the container.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_hash_child_calc_foreach_cb(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED,
                            void *data, void *fdata)
{
   Efl_Ui_Relative_Container_Child *child = data;
   Efl_Ui_Relative_Container_Calc *calc = &(child->calc);
   Efl_Ui_Relative_Container_Data *pd = fdata;
   Eina_Rect want;
   int axis, layout_min;
   double min_len;

   _child_calc(child, 0);
   _child_calc(child, 1);

   want.x = calc->want[0].position;
   want.w = calc->want[0].length;
   want.y = calc->want[1].position;
   want.h = calc->want[1].length;

   for (axis = 0; axis < 2; axis++)
     {
        layout_min = 0;
        min_len = calc->mj[axis] - calc->mi[axis];
        if (EINA_DBL_EQ(min_len, 0.0))
          layout_min = calc->m0[axis];
        else
          layout_min = ((calc->min[axis] + calc->margin[START] +
                   calc->margin[END] + calc->m0[axis]) / fabs(min_len)) + 0.5;

        if (pd->base->calc.min[axis] < layout_min)
          pd->base->calc.min[axis] = layout_min;
     }

   efl_gfx_entity_geometry_set(child->obj, want);
   return EINA_TRUE;
}

/**
 * @brief Callback function for eina_hash_foreach to initialize calculation data for each child.
 *
 * This function is called for every child before the main layout calculation pass.
 * It resets calculation states, resolves relative target objects (e.g., `calc->to[LEFT]`),
 * and fetches the latest hint values (weight, align, fill, aspect, margin, min/max sizes)
 * from the child object, storing them in its Efl_Ui_Relative_Container_Calc structure.
 *
 * @param hash The Eina_Hash being iterated (unused).
 * @param key The key of the hash item (unused).
 * @param data Pointer to the Efl_Ui_Relative_Container_Child for the current child.
 * @param fdata Pointer to the Efl_Ui_Relative_Container_Data of the container.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_hash_child_init_foreach_cb(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED,
                            void *data, void *fdata)
{
   Eina_Size2D max, min, aspect;
   Efl_Ui_Relative_Container_Child *child = data;
   Efl_Ui_Relative_Container_Calc *calc = &(child->calc);
   Efl_Ui_Relative_Container_Data *pd = fdata;

   calc->to[LEFT] = _relative_child_find(pd, child->rel[LEFT].to);
   calc->to[RIGHT] = _relative_child_find(pd, child->rel[RIGHT].to);
   calc->to[TOP] = _relative_child_find(pd, child->rel[TOP].to);
   calc->to[BOTTOM] = _relative_child_find(pd, child->rel[BOTTOM].to);

   calc->state[0] = RELATIVE_CALC_NONE;
   calc->state[1] = RELATIVE_CALC_NONE;
   calc->chain_state[0] = RELATIVE_CALC_NONE;
   calc->chain_state[1] = RELATIVE_CALC_NONE;

   efl_gfx_hint_weight_get(child->obj, &calc->weight[0], &calc->weight[1]);
   efl_gfx_hint_align_get(child->obj, &calc->align[0], &calc->align[1]);
   efl_gfx_hint_fill_get(child->obj, &calc->fill[0], &calc->fill[1]);
   efl_gfx_hint_aspect_get(child->obj, &calc->aspect_type, &aspect);
   calc->aspect[0] = aspect.w;
   calc->aspect[1] = aspect.h;
   efl_gfx_hint_margin_get(child->obj, &calc->margin[LEFT], &calc->margin[RIGHT],
                           &calc->margin[TOP], &calc->margin[BOTTOM]);
   max = efl_gfx_hint_size_combined_max_get(child->obj);
   min = efl_gfx_hint_size_combined_min_get(child->obj);
   calc->max[0] = max.w;
   calc->max[1] = max.h;
   calc->min[0] = min.w;
   calc->min[1] = min.h;
   calc->m0[0] = 0.0;
   calc->m0[1] = 0.0;

   calc->want[0].position = 0;
   calc->want[0].length = 0;
   calc->want[1].position = 0;
   calc->want[1].length = 0;
   calc->space[0].position = 0;
   calc->space[0].length = 0;
   calc->space[1].position = 0;
   calc->space[1].length = 0;

   if (calc->weight[0] < 0) calc->weight[0] = 0;
   if (calc->weight[1] < 0) calc->weight[1] = 0;

   if (calc->align[0] < 0) calc->align[0] = 0;
   if (calc->align[1] < 0) calc->align[1] = 0;
   if (calc->align[0] > 1) calc->align[0] = 1;
   if (calc->align[1] > 1) calc->align[1] = 1;

   if (calc->max[0] < 0) calc->max[0] = INT_MAX;
   if (calc->max[1] < 0) calc->max[1] = INT_MAX;
   if (calc->aspect[0] < 0) calc->aspect[0] = 0;
   if (calc->aspect[1] < 0) calc->aspect[1] = 0;

   return EINA_TRUE;
}

/**
 * @brief Callback invoked when the container's own hints change.
 *
 * Triggers a layout request for the container itself.
 *
 * @param data User data associated with the callback (unused).
 * @param ev The Efl_Event details, where ev->object is the container.
 */
static void
_efl_ui_relative_container_hints_changed_cb(void *data EINA_UNUSED, const Efl_Event *ev)
{
   efl_pack_layout_request(ev->object);
}

/**
 * @internal
 * @brief Updates the layout of the relative container and its children.
 *
 * This function is called to perform the actual layout calculations. It initializes
 * the base container's calculation parameters, then iterates through all children
 * to initialize their calculation data (_hash_child_init_foreach_cb) and then
 * to perform the actual calculations (_hash_child_calc_foreach_cb).
 * Finally, it sets the container's restricted minimum size based on the layout
 * and emits the EFL_PACK_EVENT_LAYOUT_UPDATED event.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 */
EOLIAN static void
_efl_ui_relative_container_efl_pack_layout_layout_update(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   Eina_Rect want = efl_gfx_entity_geometry_get(obj);
   pd->base->calc.want[0].position = want.x;
   pd->base->calc.want[0].length = want.w;
   pd->base->calc.want[1].position = want.y;
   pd->base->calc.want[1].length = want.h;
   pd->base->calc.min[0] = 0;
   pd->base->calc.min[1] = 0;

   eina_hash_foreach(pd->children, _hash_child_init_foreach_cb, pd);
   eina_hash_foreach(pd->children, _hash_child_calc_foreach_cb, pd);

   efl_gfx_hint_size_restricted_min_set(obj, EINA_SIZE2D(pd->base->calc.min[0], pd->base->calc.min[1]));

   efl_event_callback_call(obj, EFL_PACK_EVENT_LAYOUT_UPDATED, NULL);
}

/**
 * @internal
 * @brief Requests a layout update for the container.
 *
 * This typically marks the container as needing recalculation, which will
 * trigger `efl_canvas_group_group_calculate` at a later point.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container (unused).
 */
EOLIAN static void
_efl_ui_relative_container_efl_pack_layout_layout_request(Eo *obj, Efl_Ui_Relative_Container_Data *pd EINA_UNUSED)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Performs the layout calculation if needed.
 *
 * This is part of the Efl.Canvas.Group interface. If the group needs
 * recalculation, it calls `efl_pack_layout_update` to perform the actual layout.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container (unused).
 */
EOLIAN static void
_efl_ui_relative_container_efl_canvas_group_group_calculate(Eo *obj, Efl_Ui_Relative_Container_Data *pd EINA_UNUSED)
{
   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   efl_pack_layout_update(obj);
}

/**
 * @internal
 * @brief Sets the size of the container.
 *
 * Overrides the default Efl.Gfx.Entity behavior to also mark the
 * canvas group as changed, triggering a layout recalculation if necessary.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container (unused).
 * @param sz The new size.
 */
EOLIAN static void
_efl_ui_relative_container_efl_gfx_entity_size_set(Eo *obj, Efl_Ui_Relative_Container_Data *pd EINA_UNUSED, Eina_Size2D sz)
{
   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);
   efl_canvas_group_change(obj);
}

/**
 * @internal
 * @brief Sets the position of the container.
 *
 * Overrides the default Efl.Gfx.Entity behavior to also mark the
 * canvas group as changed, potentially affecting child positions if they
 * are relative to the container's origin.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container (unused).
 * @param pos The new position.
 */
EOLIAN static void
_efl_ui_relative_container_efl_gfx_entity_position_set(Eo *obj, Efl_Ui_Relative_Container_Data *pd EINA_UNUSED, Eina_Position2D pos)
{
   efl_gfx_entity_position_set(efl_super(obj, MY_CLASS), pos);
   efl_canvas_group_change(obj);
}

/**
 * @internal
 * @brief Adds the container to a canvas group.
 *
 * Initializes the clipper object used for children, sets up hints changed callback
 * for the container itself, and calls the superclass's group_add.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 */
EOLIAN static void
_efl_ui_relative_container_efl_canvas_group_group_add(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   pd->clipper = efl_add(EFL_CANVAS_RECTANGLE_CLASS, obj);
   evas_object_static_clip_set(pd->clipper, EINA_TRUE);
   efl_gfx_entity_geometry_set(pd->clipper, EINA_RECT(-49999, -49999, 99999, 99999));
   efl_canvas_group_member_add(obj, pd->clipper);
   efl_ui_widget_sub_object_add(obj, pd->clipper);

   efl_event_callback_add(obj, EFL_GFX_ENTITY_EVENT_HINTS_CHANGED,
                          _efl_ui_relative_container_hints_changed_cb, NULL);
   efl_canvas_group_add(efl_super(obj, MY_CLASS));

   elm_widget_highlight_ignore_set(obj, EINA_TRUE);
}

/**
 * @internal
 * @brief Constructor for the Efl_Ui_Relative_Container object.
 *
 * Initializes the object, sets its type and accessibility role.
 * Initializes the private data structure, including the children hash
 * and the 'base' child structure representing the container itself for
 * relative calculations.
 *
 * @param obj The Efl_Ui_Relative_Container object being constructed.
 * @param pd The private data to initialize.
 * @return The constructed object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_relative_container_efl_object_constructor(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME);
   efl_access_object_access_type_set(obj, EFL_ACCESS_TYPE_SKIPPED);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_FILLER);

   pd->obj = obj;
   pd->children = eina_hash_pointer_new(_hash_free_cb);

   pd->base = calloc(1, sizeof(Efl_Ui_Relative_Container_Child));
   if (!pd->base) return NULL;

   pd->base->obj = obj;
   pd->base->layout = obj;
   pd->base->rel[LEFT].to = obj;
   pd->base->rel[LEFT].relative_position = 0.0;
   pd->base->rel[RIGHT].to = obj;
   pd->base->rel[RIGHT].relative_position = 1.0;
   pd->base->rel[TOP].to = obj;
   pd->base->rel[TOP].relative_position = 0.0;
   pd->base->rel[BOTTOM].to = obj;
   pd->base->rel[BOTTOM].relative_position = 1.0;
   pd->base->calc.mi[0] = pd->base->calc.mi[1] = 0.0;
   pd->base->calc.mj[0] = pd->base->calc.mj[1] = 1.0;
   pd->base->calc.state[0] = RELATIVE_CALC_DONE;
   pd->base->calc.state[1] = RELATIVE_CALC_DONE;
   pd->base->calc.chain_state[0] = RELATIVE_CALC_DONE;
   pd->base->calc.chain_state[1] = RELATIVE_CALC_DONE;

   return obj;
}

/**
 * @internal
 * @brief Invalidates the Efl_Ui_Relative_Container object.
 *
 * Calls the superclass invalidate and then frees the buckets of the children hash.
 * The actual child data (Efl_Ui_Relative_Container_Child) and the Eo child objects
 * are typically freed/deleted when the hash is fully freed in the destructor or
 * via unpack operations, managed by _hash_free_cb.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 */
EOLIAN static void
_efl_ui_relative_container_efl_object_invalidate(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   efl_invalidate(efl_super(obj, MY_CLASS));

   eina_hash_free_buckets(pd->children);
}

/**
 * @internal
 * @brief Destructor for the Efl_Ui_Relative_Container object.
 *
 * Cleans up resources: removes event callbacks, frees the children hash
 * (which in turn frees/deletes child data and objects via _hash_free_cb),
 * frees the base child structure, and calls the superclass destructor.
 *
 * @param obj The Efl_Ui_Relative_Container object being destructed.
 * @param pd The private data of the container.
 */
EOLIAN static void
_efl_ui_relative_container_efl_object_destructor(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   efl_event_callback_del(obj, EFL_GFX_ENTITY_EVENT_HINTS_CHANGED,
                          _efl_ui_relative_container_hints_changed_cb, NULL);
   eina_hash_free(pd->children);
   if (pd->base) free(pd->base);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Packs a sub-object into the container.
 *
 * Registers the sub-object as a child of the relative container.
 *
 * @param obj The Efl_Ui_Relative_Container object (unused).
 * @param pd The private data of the container.
 * @param subobj The Efl_Gfx_Entity to pack.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., if subobj is NULL or already packed).
 */
EOLIAN static Eina_Bool
_efl_ui_relative_container_efl_pack_pack(Eo *obj EINA_UNUSED, Efl_Ui_Relative_Container_Data *pd, Efl_Gfx_Entity *subobj)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(subobj, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!!eina_hash_find(pd->children, &subobj), EINA_FALSE);

   return !!_efl_ui_relative_container_register(pd, subobj);
}

/**
 * @internal
 * @brief Unpacks (removes) a child object from the container.
 *
 * Deletes the child from the internal children hash. This triggers _hash_free_cb
 * for cleanup of the child's data and Eo object. Requests a layout update.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 * @param child The Efl_Object (child) to unpack.
 * @return EINA_TRUE on success, EINA_FALSE if the child was not registered.
 */
EOLIAN static Eina_Bool
_efl_ui_relative_container_efl_pack_unpack(Eo *obj, Efl_Ui_Relative_Container_Data *pd, Efl_Object *child)
{
   if (!eina_hash_del_by_key(pd->children, &child))
     {
        ERR("child(%p(%s)) is not registered", child, efl_class_name_get(child));
        return EINA_FALSE;
     }

   efl_pack_layout_request(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Unpacks all child objects from the container.
 *
 * Frees all buckets in the children hash, effectively removing all children.
 * This triggers _hash_free_cb for each child. Requests a layout update.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 * @return EINA_TRUE always.
 */
EOLIAN static Eina_Bool
_efl_ui_relative_container_efl_pack_unpack_all(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   eina_hash_free_buckets(pd->children);
   efl_pack_layout_request(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Clears all packed objects, deleting them.
 *
 * Temporarily sets a different hash free callback (_hash_clear_cb) that
 * explicitly deletes the child Eo objects. Then frees all buckets,
 * triggering this deletion. Restores the original hash free callback.
 * Requests a layout update.
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 * @return EINA_TRUE always.
 */
EOLIAN static Eina_Bool
_efl_ui_relative_container_efl_pack_pack_clear(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   eina_hash_free_cb_set(pd->children, _hash_clear_cb);
   eina_hash_free_buckets(pd->children);
   eina_hash_free_cb_set(pd->children, _hash_free_cb);

   efl_pack_layout_request(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Advances the content iterator to the next child object.
 *
 * @param it The content iterator.
 * @param data Pointer to store the next child Eo object.
 * @return EINA_TRUE if there is a next item, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_relative_container_content_iterator_next(Efl_Ui_Relative_Container_Content_Iterator *it, void **data)
{
   Efl_Ui_Relative_Container_Child *child;

   if (!eina_iterator_next(it->real_iterator, (void **) &child))
     return EINA_FALSE;

   if (data) *data = child->obj;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container associated with the content iterator.
 *
 * @param it The content iterator.
 * @return The Efl_Ui_Relative_Container object.
 */
static Eo *
_efl_ui_relative_container_content_iterator_get_container(Efl_Ui_Relative_Container_Content_Iterator *it)
{
   return it->relative_container;
}

/**
 * @internal
 * @brief Frees the content iterator.
 *
 * @param it The content iterator to free.
 */
static void
_efl_ui_relative_container_content_iterator_free(Efl_Ui_Relative_Container_Content_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   free(it);
}

/**
 * @internal
 * @brief Creates an iterator for the container's content (child objects).
 *
 * @param obj The Efl_Ui_Relative_Container object.
 * @param pd The private data of the container.
 * @return A new Eina_Iterator for the children, or NULL on failure.
 */
EOLIAN static Eina_Iterator *
_efl_ui_relative_container_efl_container_content_iterate(Eo *obj, Efl_Ui_Relative_Container_Data *pd)
{
   Efl_Ui_Relative_Container_Content_Iterator *it;

   it = calloc(1, sizeof(*it));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->relative_container = obj;
   it->real_iterator = eina_hash_iterator_data_new(pd->children);

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_efl_ui_relative_container_content_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
     _efl_ui_relative_container_content_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_efl_ui_relative_container_content_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @brief Gets the number of child objects in the container.
 *
 * @param obj The Efl_Ui_Relative_Container object (unused).
 * @param pd The private data of the container.
 * @return The count of children.
 */
EOLIAN static int
_efl_ui_relative_container_efl_container_content_count(Eo *obj EINA_UNUSED, Efl_Ui_Relative_Container_Data *pd)
{
   return eina_hash_population(pd->children);
}

/**
 * @brief Macro invocations to generate setter and getter functions for relative layout properties.
 *
 * These macros (defined in efl_ui_relative_container_private.h or similar)
 * generate functions like `efl_ui_relative_container_relation_left_set`,
 * `efl_ui_relative_container_relation_left_get`, etc., for each of the four sides.
 * These functions allow specifying how a child's side (e.g., its left edge)
 * is positioned relative to another object's side (e.g., the container's right edge
 * or another child's center).
 *
 * Example generated functions:
 * - `efl_ui_relative_container_relation_left_set(Eo *obj, Efl_Gfx_Entity *child, Efl_Gfx_Entity *relative_to, double position)`
 * - `efl_ui_relative_container_relation_left_get(Eo *obj, Efl_Gfx_Entity *child, Efl_Gfx_Entity **relative_to, double *position)`
 * (and similarly for right, top, bottom)
 */
EFL_UI_RELATIVE_CONTAINER_RELATION_SET_GET(left, LEFT);
EFL_UI_RELATIVE_CONTAINER_RELATION_SET_GET(right, RIGHT);
EFL_UI_RELATIVE_CONTAINER_RELATION_SET_GET(top, TOP);
EFL_UI_RELATIVE_CONTAINER_RELATION_SET_GET(bottom, BOTTOM);

/* Internal EO APIs and hidden overrides */

#define EFL_UI_RELATIVE_CONTAINER_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_OPS(efl_ui_relative_container)

#include "efl_ui_relative_container.eo.c"
