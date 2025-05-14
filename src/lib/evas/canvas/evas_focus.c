#define EFL_INTERNAL_UNSTABLE
#define EFL_INPUT_EVENT_PROTECTED

#include "evas_common_private.h"
#include "evas_private.h"

static int evas_focus_log_domain = -1;

#define F_CRI(...) EINA_LOG_DOM_CRIT(evas_focus_log_domain, __VA_ARGS__)
#define F_ERR(...) EINA_LOG_DOM_ERR(evas_focus_log_domain, __VA_ARGS__)
#define F_WRN(...) EINA_LOG_DOM_WARN(evas_focus_log_domain, __VA_ARGS__)
#define F_INF(...) EINA_LOG_DOM_INFO(evas_focus_log_domain, __VA_ARGS__)
#define F_DBG(...) EINA_LOG_DOM_DBG(evas_focus_log_domain, __VA_ARGS__)

/* private calls */

/* local calls */

/* public calls */

/**
 * @brief Initializes the Evas focus subsystem.
 *
 * This function registers a log domain for focus-related messages.
 * It should be called once during Evas initialization.
 */
void
evas_focus_init(void)
{
   evas_focus_log_domain = eina_log_domain_register("evas-focus", "red");
}

/**
 * @brief Shuts down the Evas focus subsystem.
 *
 * This function unregisters the log domain used for focus-related messages.
 * It should be called once during Evas shutdown.
 */
void
evas_focus_shutdown(void)
{
   eina_log_domain_unregister(evas_focus_log_domain);
   evas_focus_log_domain = -1;
}

/**
 * @internal
 * @brief Checks if a given seat is already in the list of focusing seats.
 *
 * @param seats The list of Efl_Input_Device pointers representing currently focusing seats.
 * @param seat The Efl_Input_Device to check.
 * @return @c EINA_TRUE if the seat is already in the list, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_already_focused(Eina_List *seats, Efl_Input_Device *seat)
{
   Eina_List *l;
   const Efl_Input_Device *s;

   EINA_LIST_FOREACH(seats, l, s)
     {
        if (s == seat)
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Retrieves the default seat associated with an Evas canvas.
 *
 * The default seat is typically the primary input device group (like keyboard/mouse)
 * for the canvas.
 *
 * @param evas_obj The Evas object (often the canvas itself or an object on it)
 *                 from which to derive the Evas canvas instance.
 * @return A pointer to the default Efl_Input_Device, or @c NULL if not found.
 */
static Efl_Input_Device *
_default_seat_get(const Eo *evas_obj)
{
   Evas_Public_Data *edata;
   Evas *evas = evas_object_evas_get((Evas_Object *)evas_obj);

   edata = efl_data_scope_get(evas, EVAS_CANVAS_CLASS);
   if (!edata) return NULL;
   return edata->default_seat;
}

#define DEBUG_TUPLE(v) v, (v ? efl_class_name_get(v) : "(null)")

/**
 * @internal
 * @brief Sets or unsets the focus state for a given object and input device (seat).
 *
 * This function updates the internal hash table mapping seats to focused objects
 * for a specific Evas canvas.
 *
 * @param evas_obj The Evas object to focus or unfocus.
 * @param key The Efl_Input_Device (seat) for which focus is being changed.
 * @param focus If @c EINA_TRUE, set focus; if @c EINA_FALSE, unset focus.
 */
static void
_evas_focus_set(Eo *evas_obj, Efl_Input_Device *key, Eina_Bool focus)
{
   Evas_Public_Data *edata;
   Evas *evas = evas_object_evas_get(evas_obj);

   EINA_SAFETY_ON_NULL_RETURN(evas);
   edata = efl_data_scope_get(evas, EVAS_CANVAS_CLASS);

   F_DBG("Focus moved in %d from (%p,%s) to (%p,%s)", efl_input_device_seat_id_get(key), DEBUG_TUPLE(eina_hash_find(edata->focused_objects, &key)), DEBUG_TUPLE(evas_obj));

   if (focus)
     {
        Eo *foc;

        foc = eina_hash_set(edata->focused_objects, &key, evas_obj);
        if (foc)
          {
             F_ERR("Element %p was focused while a other object was unfocused, this is not expected! No unfocus event will be sent to it", foc);
          }
     }
   else
     eina_hash_del_by_key(edata->focused_objects, &key);
}

/**
 * @internal
 * @brief Retrieves the currently focused object for a given Evas instance and seat.
 *
 * @param evas_obj An object on the Evas canvas, used to get the canvas instance.
 * @param key The Efl_Input_Device (seat) to query for focus.
 * @return The focused Evas_Object (Eo *) or @c NULL if no object is focused for that seat.
 */
static Eo *
_current_focus_get(Eo *evas_obj, Efl_Input_Device *key)
{
   Evas_Public_Data *edata;
   Evas *evas = evas_object_evas_get(evas_obj);

   EINA_SAFETY_ON_NULL_RETURN_VAL(evas, NULL);
   edata = efl_data_scope_get(evas, EVAS_CANVAS_CLASS);

   return eina_hash_find(edata->focused_objects, &key);
}

/**
 * @internal
 * @brief Dispatches focus_in or focus_out events to an object.
 *
 * This function handles the creation and emission of focus events,
 * including legacy Evas callbacks and new EFL events.
 *
 * @param obj The protected data of the Evas object receiving/losing focus.
 * @param seat The Efl_Input_Device (seat) associated with this focus change.
 * @param in @c EINA_TRUE if it's a focus_in event, @c EINA_FALSE for focus_out.
 */
void
_evas_focus_dispatch_event(Evas_Object_Protected_Data *obj, Efl_Input_Device *seat, Eina_Bool in)
{
   Efl_Input_Focus_Data *ev_data;
   Efl_Input_Focus *evt;
   Evas_Callback_Type cb_evas, cb_obj_evas;
   const Efl_Event_Description *efl_object_focus_event;

   EVAS_OBJECT_DATA_VALID_CHECK(obj);
   evt = efl_input_focus_instance_get(
                                efl_provider_find(obj->object, EVAS_CANVAS_CLASS),
                                (void **) &ev_data);
   if (!evt) return;

   ev_data->device = efl_ref(seat);
   efl_wref_add(obj->object, &ev_data->object_wref);
   ev_data->timestamp = time(NULL);

   if (in)
     {
        cb_obj_evas = EVAS_CALLBACK_FOCUS_IN;
        cb_evas = EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN;
        efl_object_focus_event = EFL_EVENT_FOCUS_IN;
     }
   else
     {
        cb_obj_evas = EVAS_CALLBACK_FOCUS_OUT;
        cb_evas = EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT;
        efl_object_focus_event = EFL_EVENT_FOCUS_OUT;
     }

   evas_object_event_callback_call(obj->object, obj,
                                   cb_obj_evas,
                                   evt, _evas_object_event_new(),
                                   efl_object_focus_event);
   evas_event_callback_call(obj->layer->evas->evas, cb_evas, evt);
   efl_unref(evt);
}

/**
 * @internal
 * @brief Unfocuses an object for a specific seat.
 *
 * This function removes the seat from the object's list of focusing seats,
 * updates the global focus state, dispatches a focus_out event, and
 * triggers post-event callbacks.
 *
 * @param obj The protected data of the Evas object to unfocus.
 * @param seat The Efl_Input_Device (seat) from which the object is losing focus.
 */
static void
_evas_object_unfocus(Evas_Object_Protected_Data *obj, Efl_Input_Device *seat)
{
   int event_id = _evas_event_counter;

   EVAS_OBJECT_DATA_VALID_CHECK(obj);
   EINA_COW_WRITE_BEGIN(evas_object_events_cow, obj->events, Evas_Object_Events_Data, events)
     events->focused_by_seats = eina_list_remove(events->focused_by_seats, seat);
   EINA_COW_WRITE_END(evas_object_events_cow, obj->events, events);

   _evas_focus_set(obj->object, seat, EINA_FALSE);
   _evas_focus_dispatch_event(obj, seat, EINA_FALSE);
   _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
}

/**
 * @internal
 * @brief Callback triggered when an input device (seat) that was focusing an object is invalidated.
 *
 * This ensures that if a seat is removed or becomes invalid, any object it was
 * focusing is properly unfocused.
 *
 * @param data Pointer to the Evas_Object_Protected_Data of the focused object.
 * @param ev The Efl_Event details, where ev->object is the invalidated Efl_Input_Device.
 */
void
_evas_focus_device_invalidate_cb(void *data, const Efl_Event *ev)
{
   _evas_object_unfocus(data, ev->object);
}

/**
 * @internal
 * @brief Efl.Canvas.Object.seat_focus_del EOLIAN method.
 *
 * Removes focus from the object for a specific seat. If seat is NULL,
 * it uses the default seat.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param seat The seat to remove focus from. Can be NULL for default seat.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if not focused by the seat.
 */
EOLIAN Eina_Bool
_efl_canvas_object_seat_focus_del(Eo *eo_obj,
                                  Evas_Object_Protected_Data *obj,
                                  Efl_Input_Device *seat)
{
   Eina_List *l;
   Efl_Input_Device *dev;
   Eo *default_seat;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if (seat) default_seat = _default_seat_get(eo_obj);
   else default_seat = seat = _default_seat_get(eo_obj);

   if ((!seat) && obj->layer)
     {
        if (obj->layer->evas->pending_default_focus_obj == eo_obj)
          obj->layer->evas->pending_default_focus_obj = NULL;
     }

   EINA_LIST_FOREACH(obj->events->focused_by_seats, l, dev)
     {
        if (dev != seat)
          continue;
        if (obj->interceptors && obj->interceptors->focus_set.func && obj->interceptors->device_focus_set.func)
          {
             CRI("Your object is trying to use both focus_set and device_focus_set intercept! Sad!");
             return EINA_FALSE;
          }
        if (obj->interceptors && obj->interceptors->focus_set.func && (seat == default_seat))
          {
             if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_FOCUS_SET,
                                                  1, EINA_FALSE))
               {
                  return EINA_FALSE;
               }
          }
        else if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_DEVICE_FOCUS_SET,
                                             1, EINA_FALSE, seat))
          {
             return EINA_FALSE;
          }

        efl_event_callback_del(dev, EFL_EVENT_INVALIDATE,
                               _evas_focus_device_invalidate_cb, obj);
        _evas_object_unfocus(obj, dev);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Efl.Canvas.Object.seat_focus_add EOLIAN method.
 *
 * Sets focus to the object for a specific seat. If seat is NULL,
 * it uses the default seat. This involves handling focus interceptors,
 * unfocusing any previously focused object for that seat, and dispatching
 * focus events.
 *
 * @param eo_obj The Evas object to focus.
 * @param obj The protected data of the Evas object.
 * @param seat The seat to grant focus for. Can be NULL for default seat.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., object is invalid,
 *         seat is not a seat type, focus intercepted and denied).
 */
EOLIAN Eina_Bool
_efl_canvas_object_seat_focus_add(Eo *eo_obj,
                                  Evas_Object_Protected_Data *obj,
                                  Efl_Input_Device *seat)
{
   Eo *current_focus;
   int event_id;
   Eo *default_seat;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   EINA_SAFETY_ON_FALSE_RETURN_VAL(!efl_invalidating_get(eo_obj) && !efl_invalidated_get(eo_obj), EINA_FALSE);

   event_id = _evas_event_counter;
   if (seat) default_seat = _default_seat_get(eo_obj);
   else default_seat = seat = _default_seat_get(eo_obj);

   if (seat && (efl_input_device_type_get(seat) != EFL_INPUT_DEVICE_TYPE_SEAT))
     return EINA_FALSE;

   if (obj->layer && (!seat))
     {
        obj->layer->evas->pending_default_focus_obj = eo_obj;
        return EINA_TRUE; //questionable return
     }

   if (!efl_input_seat_event_filter_get(eo_obj, seat))
     return EINA_FALSE;

   if (_already_focused(obj->events->focused_by_seats, seat))
     goto end;

   if (obj->interceptors && obj->interceptors->focus_set.func && obj->interceptors->device_focus_set.func)
     {
        CRI("Your object is trying to use both focus_set and device_focus_set intercept! Sad!");
        return EINA_FALSE;
     }
   if (obj->interceptors && obj->interceptors->focus_set.func && (seat == default_seat))
     {
        if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_FOCUS_SET,
                                             1, EINA_TRUE))
          {
             return EINA_FALSE;
          }
     }
   else if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_DEVICE_FOCUS_SET,
                                        1, EINA_TRUE, seat))
     {
        return EINA_FALSE;
     }
   current_focus = _current_focus_get(eo_obj, seat);
   if (current_focus)
     efl_canvas_object_seat_focus_del(current_focus, seat);

   //In case intercept focus callback focused object we should return.
   if (_current_focus_get(eo_obj, seat)) goto end;

   efl_event_callback_add(seat, EFL_EVENT_INVALIDATE, _evas_focus_device_invalidate_cb, obj);

   EINA_COW_WRITE_BEGIN(evas_object_events_cow, obj->events, Evas_Object_Events_Data, events)
     events->focused_by_seats = eina_list_append(events->focused_by_seats, seat);
   EINA_COW_WRITE_END(evas_object_events_cow, obj->events, events);

   _evas_focus_set(eo_obj, seat, EINA_TRUE);

   _evas_focus_dispatch_event(obj, seat, EINA_TRUE);
 end:
   if (obj->layer)
     _evas_post_event_callback_call(obj->layer->evas->evas, obj->layer->evas, event_id);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Efl.Canvas.Object.seat_focus_check EOLIAN method.
 *
 * Checks if the object is focused by a specific seat.
 * If seat is NULL, it uses the default seat.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param seat The seat to check focus against. Can be NULL for default seat.
 * @return @c EINA_TRUE if the object is focused by the specified seat, @c EINA_FALSE otherwise.
 */
EOLIAN Eina_Bool
_efl_canvas_object_seat_focus_check(const Eo *eo_obj,
                                    Evas_Object_Protected_Data *obj,
                                    Efl_Input_Device *seat)
{
   Eina_List *l;
   Efl_Input_Device *s;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if (!seat) seat = _default_seat_get(eo_obj);

   EINA_LIST_FOREACH(obj->events->focused_by_seats, l, s)
     {
        if (s == seat)
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Efl.Canvas.Object.key_focus_set (legacy) EOLIAN method.
 *
 * Sets or unsets key focus for the object using the default seat.
 * This is a convenience function that maps to seat_focus_add/del with a NULL seat.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param focus @c EINA_TRUE to set focus, @c EINA_FALSE to unset.
 */
EOLIAN void
_efl_canvas_object_key_focus_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, Eina_Bool focus)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();

   if (_efl_canvas_object_seat_focus_check(eo_obj, obj, NULL) == focus) return;

   if (focus)
     _efl_canvas_object_seat_focus_add(eo_obj, obj, NULL);
   else
     _efl_canvas_object_seat_focus_del(eo_obj, obj, NULL);
}

/**
 * @internal
 * @brief Efl.Canvas.Object.seat_focus_get EOLIAN method.
 * (Note: The EOLIAN name might be `seat_focus_get` but this function seems to check if *any* seat has focus)
 *
 * Checks if the object is focused by *any* seat.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @return @c EINA_TRUE if the object is focused by at least one seat, @c EINA_FALSE otherwise.
 */
EOLIAN Eina_Bool
_efl_canvas_object_seat_focus_get(const Eo *eo_obj, Evas_Object_Protected_Data *obj)
{
   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   return eina_list_count(obj->events->focused_by_seats) ? EINA_TRUE : EINA_FALSE;
}

/**
 * @internal
 * @brief Efl.Canvas.Object.key_focus_get (legacy) EOLIAN method.
 *
 * Checks if the object has key focus (i.e., focused by the default seat).
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @return @c EINA_TRUE if the object is focused by the default seat, @c EINA_FALSE otherwise.
 */
EOLIAN Eina_Bool
_efl_canvas_object_key_focus_get(const Eo *eo_obj, Evas_Object_Protected_Data *obj)
{
   return _efl_canvas_object_seat_focus_check(eo_obj, obj, NULL);
}

/**
 * @internal
 * @brief Evas.Canvas.seat_focus_get EOLIAN method.
 *
 * Retrieves the object currently focused by a specific seat on the canvas.
 *
 * @param eo_obj The Evas canvas (unused in current implementation, context from `e`).
 * @param e The public data of the Evas canvas.
 * @param seat The seat to query. If NULL, uses the default seat.
 * @return The Evas_Object* that has focus for the given seat, or NULL.
 */
EOLIAN Evas_Object *
_evas_canvas_seat_focus_get(const Eo *eo_obj EINA_UNUSED, Evas_Public_Data *e,
                            Efl_Input_Device *seat)
{
   if (!seat)
     seat = e->default_seat;

   return eina_hash_find(e->focused_objects, &seat);
}

/**
 * @internal
 * @brief Evas.Canvas.focus_get (legacy) EOLIAN method.
 *
 * Retrieves the object currently focused by the default seat on the canvas.
 *
 * @param eo_obj The Evas canvas (unused in current implementation, context from `e`).
 * @param e The public data of the Evas canvas.
 * @return The Evas_Object* that has focus for the default seat, or NULL.
 */
EOLIAN Evas_Object*
_evas_canvas_focus_get(const Eo *eo_obj EINA_UNUSED, Evas_Public_Data *e)
{
   return _evas_canvas_seat_focus_get(eo_obj, e, NULL);
}
