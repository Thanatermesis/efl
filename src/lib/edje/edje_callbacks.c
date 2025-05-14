#include "edje_private.h"

/**
 * @file
 * @brief Callbacks for Edje object events.
 *
 * This file contains the callback functions that Edje uses to handle
 * various input and timer events. These callbacks are responsible for
 * translating low-level Evas events into Edje signals and managing
 * part-specific event handling like drag operations.
 */

/**
 * @brief Callback for EFL_EVENT_HOLD events.
 *
 * Emits "hold,on" or "hold,off" signals for the Edje part
 * associated with the event object.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_HOLD event information.
 */
static void
_edje_hold_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Hold *ev;
   Edje *ed;
   Edje_Real_Part *rp;

   ev = event->info;
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (!rp) return;
   if (efl_input_hold_get(ev))
     _edje_seat_emit(ed, efl_input_device_get(ev),
                     "hold,on", rp->part->name);
   else
     _edje_seat_emit(ed, efl_input_device_get(ev),
                     "hold,off", rp->part->name);
}

/**
 * @brief Callback for EFL_EVENT_FOCUS_IN events.
 *
 * Emits a "focus,part,in" signal for the Edje part
 * associated with the event object when it gains focus.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_FOCUS_IN event information.
 */
static void
_edje_focus_in_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Focus *ev;
   Edje *ed;
   Edje_Real_Part *rp;

   ev = event->info;
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if ((!rp) || (!ed))
     return;

   _edje_seat_emit(ed, efl_input_device_get(ev),
                   "focus,part,in", rp->part->name);
}

/**
 * @brief Callback for EFL_EVENT_FOCUS_OUT events.
 *
 * Emits a "focus,part,out" signal for the Edje part
 * associated with the event object when it loses focus.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_FOCUS_OUT event information.
 */
static void
_edje_focus_out_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Focus *ev;
   Edje *ed;
   Edje_Real_Part *rp;

   ev = event->info;
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if ((!rp) || (!ed))
     return;

   _edje_seat_emit(ed, efl_input_device_get(ev),
                   "focus,part,out", rp->part->name);
}

/**
 * @brief Callback for EFL_EVENT_POINTER_IN (mouse in) events.
 *
 * Emits a "mouse,in" signal for the Edje part if the event is not ignored.
 * Updates event flags based on the part's mask flags.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_IN event information.
 */
static void
_edje_mouse_in_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (rp)
     {
        if (!(ev->event_flags) || !(rp->ignore_flags & ev->event_flags))
          _edje_seat_emit(ed, ev->device, "mouse,in", rp->part->name);

        ev->event_flags |= rp->mask_flags;
     }
}

/**
 * @brief Callback for EFL_EVENT_POINTER_OUT (mouse out) events.
 *
 * Emits a "mouse,out" signal for the Edje part if the event is not ignored.
 * Updates event flags based on the part's mask flags.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_OUT event information.
 */
static void
_edje_mouse_out_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (rp)
     {
        if (!(ev->event_flags) || !(rp->ignore_flags & ev->event_flags))
          _edje_seat_emit(ed, ev->device, "mouse,out", rp->part->name);

        ev->event_flags |= rp->mask_flags;
     }
}

/**
 * @brief Callback for EFL_EVENT_POINTER_DOWN (mouse down) events.
 *
 * Handles mouse button down events. Emits "mouse,down,BUTTON" signals,
 * potentially with ",double" or ",triple" suffixes for multi-clicks.
 * Initializes drag operations if the part is draggable.
 * Manages `clicked_button` and `still_in` states for click detection.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_DOWN event information.
 */
static void
_edje_mouse_down_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;
   char buf[256];
   int ignored;

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (!rp) return;

   ignored = rp->ignore_flags & ev->event_flags;

   _edje_ref(ed);
   _edje_util_freeze(ed);

   if ((!ev->event_flags) || (!ignored))
     {
        if (ev->button_flags & EVAS_BUTTON_TRIPLE_CLICK)
          snprintf(buf, sizeof(buf), "mouse,down,%i,triple", ev->button);
        else if (ev->button_flags & EVAS_BUTTON_DOUBLE_CLICK)
          snprintf(buf, sizeof(buf), "mouse,down,%i,double", ev->button);
        else
          snprintf(buf, sizeof(buf), "mouse,down,%i", ev->button);
        _edje_seat_emit(ed, ev->device, buf, rp->part->name);
     }

   if (rp->part->dragable.event_id >= 0)
     {
        rp = ed->table_parts[rp->part->dragable.event_id % ed->table_parts_size];
        if (!ignored)
          {
             snprintf(buf, sizeof(buf), "mouse,down,%i", ev->button);
             _edje_seat_emit(ed, ev->device, buf, rp->part->name);
          }
     }

   if (rp->drag)
     {
        if (rp->drag->down.count == 0)
          {
             if (rp->part->dragable.x)
               rp->drag->down.x = ev->cur.x;
             if (rp->part->dragable.y)
               rp->drag->down.y = ev->cur.y;
             rp->drag->threshold_x = EINA_FALSE;
             rp->drag->threshold_y = EINA_FALSE;
             rp->drag->threshold_started_x = EINA_TRUE;
             rp->drag->threshold_started_y = EINA_TRUE;
          }
        rp->drag->down.count++;
     }

   if (rp->clicked_button == 0)
     {
        rp->clicked_button = ev->button;
        if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
          rp->still_in = EINA_TRUE;
     }
//   _edje_recalc_do(ed);
   _edje_util_thaw(ed);
   _edje_unref(ed);

   ev->event_flags |= rp->mask_flags;
}

/**
 * @brief Callback for EFL_EVENT_POINTER_UP (mouse up) events.
 *
 * Handles mouse button up events. Emits "mouse,up,BUTTON" signals.
 * Finalizes drag operations if active, emitting "drag,stop".
 * Emits "mouse,clicked,BUTTON" if a click is completed (down and up on the same part).
 * Resets `clicked_button` and `still_in` states.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_UP event information.
 */
static void
_edje_mouse_up_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;
   char buf[256];
   int ignored;

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (!rp) return;

   ignored = rp->ignore_flags & ev->event_flags;

   _edje_ref(ed);
   _edje_util_freeze(ed);

   if ((!ev->event_flags) || (!ignored))
     {
        snprintf(buf, sizeof(buf), "mouse,up,%i", ev->button);
        _edje_seat_emit(ed, ev->device, buf, rp->part->name);
     }

   if (rp->part->dragable.event_id >= 0)
     {
        rp = ed->table_parts[rp->part->dragable.event_id % ed->table_parts_size];
        if (!ignored)
          {
             snprintf(buf, sizeof(buf), "mouse,up,%i", ev->button);
             _edje_seat_emit(ed, ev->device, buf, rp->part->name);
          }
     }

   if (rp->drag)
     {
        if (rp->drag->down.count > 0)
          {
             rp->drag->down.count--;
             if (rp->drag->down.count == 0)
               {
                  rp->drag->threshold_started_x = EINA_FALSE;
                  rp->drag->threshold_started_y = EINA_FALSE;
                  rp->drag->need_reset = 1;
                  ed->recalc_call = EINA_TRUE;
                  ed->dirty = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
                  rp->invalidate = EINA_TRUE;
#endif
                  if (!ignored && rp->drag->started)
                    _edje_seat_emit(ed, ev->device, "drag,stop",
                                    rp->part->name);
                  rp->drag->started = EINA_FALSE;
                  _edje_recalc_do(ed);
               }
          }
     }

   if ((rp->still_in) && (rp->clicked_button == ev->button) && (!ev->event_flags))
     {
        snprintf(buf, sizeof(buf), "mouse,clicked,%i", ev->button);
        _edje_seat_emit(ed, ev->device, buf, rp->part->name);
     }
   rp->clicked_button = 0;
   rp->still_in = EINA_FALSE;

//   _edje_recalc_do(ed);
   _edje_util_thaw(ed);
   _edje_unref(ed);

   ev->event_flags |= rp->mask_flags;
}

/**
 * @brief Callback for EFL_EVENT_POINTER_MOVE (mouse move) events.
 *
 * Handles mouse movement. Emits "mouse,move" signals.
 * Updates `still_in` state based on pointer position relative to the part.
 * Emits "mouse,pressed,in" or "mouse,pressed,out" if a button is pressed
 * and the mouse moves into or out of the part.
 * Manages drag operations: updates drag values, emits "drag,start" and "drag" signals.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_MOVE event information.
 */
static void
_edje_mouse_move_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;
   int ignored;

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (!rp) return;
   if (rp->part->dragable.event_id >= 0)
     {
        rp = ed->table_parts[rp->part->dragable.event_id % ed->table_parts_size];
     }

   ignored = rp->ignore_flags & ev->event_flags;

   _edje_ref(ed);
   if ((!ev->event_flags) || (!ignored))
     _edje_seat_emit(ed, ev->device, "mouse,move", rp->part->name);

   if (rp->still_in)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
          rp->still_in = EINA_FALSE;
        else
          {
             Evas_Coord x, y, w, h;

             evas_object_geometry_get(event->object, &x, &y, &w, &h);
             if ((ev->cur.x < x) || (ev->cur.y < y) ||
                 (ev->cur.x >= (x + w)) || (ev->cur.y >= (y + h)))
               {
                  if ((ev->pressed_buttons) && ((!ev->event_flags) || (!ignored)))
                    _edje_seat_emit(ed, ev->device, "mouse,pressed,out",
                                    rp->part->name);

                  rp->still_in = EINA_FALSE;
               }
          }
     }
   else
     {
        if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
          {
             Evas_Coord x, y, w, h;

             evas_object_geometry_get(event->object, &x, &y, &w, &h);
             if ((ev->cur.x >= x) && (ev->cur.y >= y) &&
                 (ev->cur.x < (x + w)) && (ev->cur.y < (y + h)))
               {
                  if ((ev->pressed_buttons) && ((!ev->event_flags) || (!ignored)))
                    _edje_seat_emit(ed, ev->device, "mouse,pressed,in",
                                    rp->part->name);

                  rp->still_in = EINA_TRUE;
               }
          }
     }
   _edje_util_freeze(ed);
   if (rp->drag)
     {
        if (rp->drag->down.count > 0)
          {
             if (rp->part->dragable.x)
               rp->drag->tmp.x = ev->cur.x - rp->drag->down.x;
             if (rp->part->dragable.y)
               rp->drag->tmp.y = ev->cur.y - rp->drag->down.y;
             ed->recalc_call = EINA_TRUE;
             ed->dirty = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
             rp->invalidate = EINA_TRUE;
#endif
          }
        _edje_recalc_do(ed);

        if (rp->drag->down.count > 0)
          {
             FLOAT_T dx, dy;

             _edje_part_dragable_calc(ed, rp, &dx, &dy);
             if ((NEQ(dx, rp->drag->val.x)) || (NEQ(dy, rp->drag->val.y)))
               {
                  rp->drag->val.x = dx;
                  rp->drag->val.y = dy;
                  if (!ignored)
                    {
                       if (!rp->drag->started)
                         _edje_seat_emit(ed, ev->device, "drag,start",
                                         rp->part->name);
                       _edje_seat_emit(ed, ev->device, "drag", rp->part->name);
                       rp->drag->started = EINA_TRUE;
                    }
                  ed->recalc_call = EINA_TRUE;
                  ed->dirty = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
                  rp->invalidate = EINA_TRUE;
#endif
                  _edje_recalc_do(ed);
               }
          }
     }
   _edje_unref(ed);
   _edje_util_thaw(ed);

   ev->event_flags |= rp->mask_flags;
}

/**
 * @brief Callback for EFL_EVENT_POINTER_WHEEL (mouse wheel) events.
 *
 * Emits "mouse,wheel,DIRECTION,VALUE" signals for the Edje part
 * if the event is not ignored. DIRECTION is 0 for vertical, 1 for horizontal.
 * VALUE is -1 or 1 indicating wheel direction.
 *
 * @param data The Edje object.
 * @param event The EFL_EVENT_POINTER_WHEEL event information.
 */
static void
_edje_mouse_wheel_signal_cb(void *data, const Efl_Event *event)
{
   Efl_Input_Pointer_Data *ev;
   Edje *ed;
   Edje_Real_Part *rp;
   char buf[256];

   ev = efl_data_scope_get(event->info, EFL_INPUT_POINTER_CLASS);
   ed = data;
   rp = evas_object_data_get(event->object, "real_part");
   if (rp)
     {
        if (!(ev->event_flags) || !(rp->ignore_flags & ev->event_flags))
          {
             snprintf(buf, sizeof(buf), "mouse,wheel,%i,%i",
                      ev->wheel.horizontal,
                      (ev->wheel.z < 0) ? (-1) : (1));
             _edje_seat_emit(ed, ev->device, buf, rp->part->name);
          }

        ev->event_flags |= rp->mask_flags;
     }
}

/**
 * @brief Timer callback for processing Edje animations and programs.
 *
 * This function is called periodically by a timer (likely ecore_animator or similar).
 * It iterates through active Edje programs (actions) and updates their states
 * based on the current time. Handles program execution, completion, and cleanup.
 *
 * @param data The Edje object.
 * @param event The Efl_Event (unused in this function).
 */
void
_edje_timer_cb(void *data, const Efl_Event *event EINA_UNUSED)
{
   double t;
   Eina_List *l;
   Eina_List *newl = NULL;
   Edje *ed = data;

   t = ecore_loop_time_get();
   _edje_ref(ed);

   _edje_block(ed);
   _edje_util_freeze(ed);
   if ((!ed->paused) && (!ed->delete_me))
     {
        Edje_Running_Program *tmp;

        ed->walking_actions = EINA_TRUE;
        EINA_LIST_FOREACH(ed->actions, l, tmp)
          {
             tmp->ref++;
             newl = eina_list_append(newl, tmp);
          }
        while (newl)
          {
             Edje_Running_Program *runp;

             runp = eina_list_data_get(newl);
             newl = eina_list_remove(newl, eina_list_data_get(newl));
             runp->ref--;
             if (!runp->delete_me)
               _edje_program_run_iterate(runp, t);
             if (_edje_block_break(ed))
               {
                  EINA_LIST_FREE(newl, tmp)
                    {
                       tmp->ref--;
                       if ((tmp->delete_me) && (tmp->ref == 0))
                         {
                            _edje_program_run_cleanup(ed, tmp);
                            free(tmp);
                         }
                    }
                  newl = NULL;
                  goto break_prog;
               }
          }
        EINA_LIST_FOREACH(ed->actions, l, tmp)
          {
             tmp->ref++;
             newl = eina_list_append(newl, tmp);
          }
        while (newl)
          {
             Edje_Running_Program *runp;

             runp = eina_list_data_get(newl);
             newl = eina_list_remove(newl, eina_list_data_get(newl));
             runp->ref--;
             if ((runp->delete_me) && (runp->ref == 0))
               {
                  _edje_program_run_cleanup(ed, runp);
                  free(runp);
               }
          }
        ed->walking_actions = EINA_FALSE;
     }
break_prog:
   _edje_unblock(ed);
   _edje_util_thaw(ed);
   _edje_unref(ed);
}

/**
 * @brief Callback for pending Edje programs.
 *
 * This function is typically scheduled by a timer (e.g., ecore_timer_add)
 * to execute an Edje program after a delay. It removes the program
 * from the pending list and runs it.
 *
 * @param data Pointer to an Edje_Pending_Program structure.
 * @return ECORE_CALLBACK_CANCEL to automatically remove the timer.
 */
Eina_Bool
_edje_pending_timer_cb(void *data)
{
   Edje_Pending_Program *pp;

   pp = data;
   pp->edje->pending_actions = eina_list_remove(pp->edje->pending_actions, pp);
   _edje_program_run(pp->edje, pp->program, 1, "", "", NULL);
   pp->timer = NULL;
   free(pp);
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Array defining standard Edje event callbacks.
 *
 * This array maps EFL input events to their respective Edje callback handlers.
 * It is used with efl_event_callback_array_add() to register these callbacks
 * on Evas objects that are part of an Edje layout.
 *
 * The structure of elements is { Efl_Event_Description, Efl_Event_Cb }.
 * Example:
 *   { EFL_EVENT_HOLD, _edje_hold_signal_cb } means that when an EFL_EVENT_HOLD
 *   occurs on an object where this array is registered, _edje_hold_signal_cb
 *   will be called.
 */
EFL_CALLBACKS_ARRAY_DEFINE(edje_callbacks,
                          { EFL_EVENT_HOLD, _edje_hold_signal_cb },
                          { EFL_EVENT_POINTER_IN, _edje_mouse_in_signal_cb },
                          { EFL_EVENT_POINTER_OUT, _edje_mouse_out_signal_cb },
                          { EFL_EVENT_POINTER_DOWN, _edje_mouse_down_signal_cb },
                          { EFL_EVENT_POINTER_UP, _edje_mouse_up_signal_cb },
                          { EFL_EVENT_POINTER_MOVE, _edje_mouse_move_signal_cb },
                          { EFL_EVENT_POINTER_WHEEL, _edje_mouse_wheel_signal_cb });

/**
 * @brief Array defining Edje focus event callbacks.
 *
 * This array maps EFL focus events (EFL_EVENT_FOCUS_IN, EFL_EVENT_FOCUS_OUT)
 * to their respective Edje callback handlers.
 * It is used with efl_event_callback_array_add() to register these callbacks
 * for focus handling.
 *
 * The structure of elements is { Efl_Event_Description, Efl_Event_Cb }.
 * Example:
 *   { EFL_EVENT_FOCUS_IN, _edje_focus_in_signal_cb } means that when an
 *   EFL_EVENT_FOCUS_IN occurs, _edje_focus_in_signal_cb will be called.
 */
EFL_CALLBACKS_ARRAY_DEFINE(edje_focus_callbacks,
                          { EFL_EVENT_FOCUS_IN, _edje_focus_in_signal_cb },
                          { EFL_EVENT_FOCUS_OUT, _edje_focus_out_signal_cb });

/**
 * @brief Adds standard Edje event callbacks to an Evas object.
 *
 * Registers the set of callbacks defined in `edje_callbacks` array
 * for the given Evas object. Also stores the Edje_Real_Part associated
 * with this object for use within the callbacks.
 *
 * @param obj The Evas object to add callbacks to.
 * @param ed The Edje object parent.
 * @param rp The Edje_Real_Part associated with obj.
 */
void
_edje_callbacks_add(Evas_Object *obj, Edje *ed, Edje_Real_Part *rp)
{
   efl_event_callback_array_add(obj, edje_callbacks(), ed);
   evas_object_data_set(obj, "real_part", rp);
}

/**
 * @brief Deletes standard Edje event callbacks from an Evas object.
 *
 * Unregisters the set of callbacks defined in `edje_callbacks` array
 * from the given Evas object and removes the associated Edje_Real_Part data.
 *
 * @param obj The Evas object to delete callbacks from.
 * @param ed The Edje object parent.
 */
void
_edje_callbacks_del(Evas_Object *obj, Edje *ed)
{
   efl_event_callback_array_del(obj, edje_callbacks(), ed);
   evas_object_data_del(obj, "real_part");
}

/**
 * @brief Adds Edje focus event callbacks to an Evas object.
 *
 * Registers the set of callbacks defined in `edje_focus_callbacks` array
 * for the given Evas object. Also stores the Edje_Real_Part associated
 * with this object for use within the focus callbacks.
 *
 * @param obj The Evas object to add focus callbacks to.
 * @param ed The Edje object parent.
 * @param rp The Edje_Real_Part associated with obj.
 */
void
_edje_callbacks_focus_add(Evas_Object *obj, Edje *ed, Edje_Real_Part *rp)
{
   efl_event_callback_array_add(obj, edje_focus_callbacks(), ed);
   evas_object_data_set(obj, "real_part", rp);
}

/**
 * @brief Deletes Edje focus event callbacks from an Evas object.
 *
 * Unregisters the set of callbacks defined in `edje_focus_callbacks` array
 * from the given Evas object and removes the associated Edje_Real_Part data.
 *
 * @param obj The Evas object to delete focus callbacks from.
 * @param ed The Edje object parent.
 */
void
_edje_callbacks_focus_del(Evas_Object *obj, Edje *ed)
{
   efl_event_callback_array_del(obj, edje_focus_callbacks(), ed);
   evas_object_data_del(obj, "real_part");
}

