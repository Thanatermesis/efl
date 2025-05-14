
#include "evas_common_private.h"
#include "evas_private.h"

EVAS_MEMPOOL(_mp_pc);

extern Eina_Hash* signals_hash_table;

/* Legacy events, do not use anywhere */
void _evas_object_smart_callback_call_internal(Evas_Object *eo_obj, const Efl_Event_Description *efl_event_desc);
static const Efl_Event_Description _EVAS_OBJECT_EVENT_FREE = EFL_EVENT_DESCRIPTION("free");
static const Efl_Event_Description _EVAS_OBJECT_EVENT_DEL = EFL_EVENT_DESCRIPTION("del");
#define EVAS_OBJECT_EVENT_FREE (&(_EVAS_OBJECT_EVENT_FREE))
#define EVAS_OBJECT_EVENT_DEL (&(_EVAS_OBJECT_EVENT_DEL))

/**
 * Evas events descriptions for Eo.
 */

/**
 * @brief Macro to define a static array of Efl_Event_Description pointers.
 *
 * This macro generates a function that returns an Efl_Event_Description
 * from a statically initialized array. The array is initialized on the
 * first call to the generated function.
 *
 * @param FUNC The name of the function to generate.
 * @param LAST The size of the array (number of event descriptions).
 * @param ... A list of Efl_Event_Description pointers to populate the array.
 *            Example: EFL_EVENT_POINTER_IN, EFL_EVENT_POINTER_OUT
 */
#define DEFINE_EVAS_CALLBACKS(FUNC, LAST, ...)                          \
  static const Efl_Event_Description *FUNC(unsigned int index)          \
  {                                                                     \
     static const Efl_Event_Description *internals[LAST] = { NULL };    \
                                                                        \
     if (index >= LAST) return NULL;                                    \
     if (internals[0] == NULL)                                          \
       {                                                                \
          memcpy(internals,                                             \
                 ((const Efl_Event_Description*[]) { __VA_ARGS__ }),    \
                 sizeof ((const Efl_Event_Description *[]) { __VA_ARGS__ })); \
       }                                                                \
     return internals[index];                                           \
  }

DEFINE_EVAS_CALLBACKS(_legacy_evas_callback_table, EVAS_CALLBACK_LAST,
                      EFL_EVENT_POINTER_IN,
                      EFL_EVENT_POINTER_OUT,
                      EFL_EVENT_POINTER_DOWN,
                      EFL_EVENT_POINTER_UP,
                      EFL_EVENT_POINTER_MOVE,
                      EFL_EVENT_POINTER_WHEEL,
                      EFL_EVENT_FINGER_DOWN,
                      EFL_EVENT_FINGER_UP,
                      EFL_EVENT_FINGER_MOVE,
                      EVAS_OBJECT_EVENT_FREE,
                      EFL_EVENT_KEY_DOWN,
                      EFL_EVENT_KEY_UP,
                      EFL_EVENT_FOCUS_IN,
                      EFL_EVENT_FOCUS_OUT,
                      EFL_GFX_ENTITY_EVENT_SHOW,
                      EFL_GFX_ENTITY_EVENT_HIDE,
                      EFL_GFX_ENTITY_EVENT_POSITION_CHANGED,
                      EFL_GFX_ENTITY_EVENT_SIZE_CHANGED,
                      EFL_GFX_ENTITY_EVENT_STACKING_CHANGED,
                      EVAS_OBJECT_EVENT_DEL,
                      EFL_EVENT_HOLD,
                      EFL_GFX_ENTITY_EVENT_HINTS_CHANGED,
                      EFL_GFX_IMAGE_EVENT_IMAGE_PRELOAD,
                      EFL_CANVAS_SCENE_EVENT_SCENE_FOCUS_IN,
                      EFL_CANVAS_SCENE_EVENT_SCENE_FOCUS_OUT,
                      EVAS_CANVAS_EVENT_RENDER_FLUSH_PRE,
                      EVAS_CANVAS_EVENT_RENDER_FLUSH_POST,
                      EFL_CANVAS_SCENE_EVENT_OBJECT_FOCUS_IN,
                      EFL_CANVAS_SCENE_EVENT_OBJECT_FOCUS_OUT,
                      EFL_GFX_IMAGE_EVENT_IMAGE_UNLOAD,
                      EFL_CANVAS_SCENE_EVENT_RENDER_PRE,
                      EFL_CANVAS_SCENE_EVENT_RENDER_POST,
                      EFL_GFX_IMAGE_EVENT_IMAGE_RESIZED,
                      EFL_CANVAS_SCENE_EVENT_DEVICE_CHANGED,
                      EFL_EVENT_POINTER_AXIS,
                      EVAS_CANVAS_EVENT_VIEWPORT_RESIZE );

/**
 * @brief Converts an Efl_Event_Description to a legacy Evas_Callback_Type.
 *
 * This function iterates through the known legacy Evas callback types and
 * compares their corresponding Efl_Event_Description with the provided one.
 *
 * @param desc The Efl_Event_Description to convert.
 * @return The corresponding Evas_Callback_Type, or EVAS_CALLBACK_LAST if not found.
 */
static inline Evas_Callback_Type
_legacy_evas_callback_type(const Efl_Event_Description *desc)
{
   Evas_Callback_Type type;

   for (type = 0; type < EVAS_CALLBACK_LAST; type++)
     {
        if (_legacy_evas_callback_table(type) == desc)
          return type;
     }

   return EVAS_CALLBACK_LAST;
}

/**
 * @brief Enumerates the types of event information that can be associated with an Efl_Event.
 *
 * This enum is used internally to determine how to process and interpret the
 * `event->info` field of an Efl_Event, particularly when converting to legacy
 * Evas event information.
 */
typedef enum {
   EFL_EVENT_TYPE_NULL,    ///< No specific event info type, or event info is NULL.
   EFL_EVENT_TYPE_OBJECT,  ///< Event info is a pointer to an Efl_Object.
   EFL_EVENT_TYPE_STRUCT,  ///< Event info is a pointer to a generic struct (e.g., Efl_Canvas_Object_Render_Update).
   EFL_EVENT_TYPE_POINTER, ///< Event info is related to pointer events (mouse, touch).
   EFL_EVENT_TYPE_KEY,     ///< Event info is related to key events.
   EFL_EVENT_TYPE_HOLD,    ///< Event info is related to hold events.
   EFL_EVENT_TYPE_FOCUS    ///< Event info is related to focus events (but typically handled as NULL type for legacy).
} Efl_Event_Info_Type;

/**
 * @brief Wrapper structure for legacy Evas event callbacks.
 *
 * This structure holds information needed to manage and invoke legacy
 * Evas callbacks that are registered through the Efl event system.
 * It includes the callback function itself, user data, the legacy
 * Evas callback type, the corresponding Efl event info type, and
 * the callback priority.
 */
typedef struct
{
   EINA_INLIST; ///< Macro for intrusive list node.
   union {
      Evas_Event_Cb         evas_cb;   ///< Legacy Evas canvas event callback function.
      Evas_Object_Event_Cb  object_cb; ///< Legacy Evas object event callback function.
   } func; ///< Union of callback function pointers.
   void                    *data; ///< User data passed to the callback.
   Evas_Callback_Type       type; ///< The legacy Evas_Callback_Type.
   Efl_Event_Info_Type      efl_event_type; ///< The type of Efl event info associated.
   Evas_Callback_Priority   priority; ///< Priority of the callback.
} Evas_Event_Cb_Wrapper_Info;

/**
 * @brief Determines the Efl_Event_Info_Type based on a legacy Evas_Callback_Type.
 *
 * This function maps a legacy Evas callback type to an internal Efl_Event_Info_Type,
 * which helps in deciding how to process the event information when a legacy
 * callback is invoked.
 *
 * @param type The legacy Evas_Callback_Type.
 * @return The corresponding Efl_Event_Info_Type.
 */
static int
_evas_event_efl_event_info_type(Evas_Callback_Type type)
{
   switch (type)
     {
      case EVAS_CALLBACK_MOUSE_IN:
      case EVAS_CALLBACK_MOUSE_OUT:
      case EVAS_CALLBACK_MOUSE_DOWN:
      case EVAS_CALLBACK_MOUSE_UP:
      case EVAS_CALLBACK_MOUSE_MOVE:
      case EVAS_CALLBACK_MOUSE_WHEEL:
      case EVAS_CALLBACK_MULTI_DOWN:
      case EVAS_CALLBACK_MULTI_UP:
      case EVAS_CALLBACK_MULTI_MOVE:
      case EVAS_CALLBACK_AXIS_UPDATE:
        return EFL_EVENT_TYPE_POINTER;

      case EVAS_CALLBACK_KEY_DOWN:
      case EVAS_CALLBACK_KEY_UP:
        return EFL_EVENT_TYPE_KEY;

      case EVAS_CALLBACK_HOLD:
        return EFL_EVENT_TYPE_HOLD;

      case EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_IN:
      case EVAS_CALLBACK_CANVAS_OBJECT_FOCUS_OUT: /* Efl.Canvas.Object */
        return EFL_EVENT_TYPE_OBJECT;

      case EVAS_CALLBACK_RENDER_POST: /* Efl_Gfx_Event_Render_Post */
        return EFL_EVENT_TYPE_STRUCT;

      case EVAS_CALLBACK_DEVICE_CHANGED: /* Efl.Input.Device */
        return EFL_EVENT_TYPE_OBJECT;

      case EVAS_CALLBACK_FOCUS_IN:
      case EVAS_CALLBACK_FOCUS_OUT:
          return EFL_EVENT_TYPE_FOCUS;

      default:
        return EFL_EVENT_TYPE_NULL;
     }
}

/**
 * @brief EFL event callback handler for legacy Evas object events.
 *
 * This function is registered as an EFL event listener. When an EFL event
 * occurs on an Evas_Object, this handler is invoked. It translates the
 * EFL event data into the legacy Evas event format and calls the
 * appropriate legacy Evas_Object_Event_Cb.
 *
 * @param data Pointer to an Evas_Event_Cb_Wrapper_Info structure.
 * @param event The Efl_Event that occurred.
 */
static void
_eo_evas_object_cb(void *data, const Efl_Event *event)
{
   Evas_Event_Flags *event_flags = NULL, evflags = EVAS_EVENT_FLAG_NONE;
   Efl_Input_Event *efl_event_info = event->info;
   Evas_Event_Cb_Wrapper_Info *info = data;
   void *event_info;
   Evas *evas;

   if (!info->func.object_cb) return;
   evas = evas_object_evas_get(event->object);

   event_info = event->info;
   switch (info->efl_event_type)
     {
      case EFL_EVENT_TYPE_POINTER:
        event_info = efl_input_pointer_legacy_info_fill(evas, efl_event_info, info->type, &event_flags);
        break;

      case EFL_EVENT_TYPE_KEY:
        event_info = efl_input_key_legacy_info_fill(efl_event_info, &event_flags);
        break;

      case EFL_EVENT_TYPE_HOLD:
        event_info = efl_input_hold_legacy_info_fill(efl_event_info, &event_flags);
        break;

      case EFL_EVENT_TYPE_FOCUS:
      case EFL_EVENT_TYPE_NULL:
         info->func.object_cb(info->data, evas, event->object, NULL);
         return;

      case EFL_EVENT_TYPE_STRUCT:
      case EFL_EVENT_TYPE_OBJECT:
        info->func.object_cb(info->data, evas, event->object, event_info);
        return;

      default: return;
     }

   if (!event_info) return;
   if (event_flags) evflags = *event_flags;
   info->func.object_cb(info->data, evas, event->object, event_info);
   if (event_flags && (evflags != *event_flags))
     efl_input_event_flags_set(efl_event_info,
                               (Efl_Input_Flags)*event_flags);
}

/**
 * @brief EFL event callback handler for legacy Evas canvas events.
 *
 * This function is registered as an EFL event listener. When an EFL event
 * occurs on an Evas canvas, this handler is invoked. It translates the
 * EFL event data into the legacy Evas event format and calls the
 * appropriate legacy Evas_Event_Cb.
 *
 * @param data Pointer to an Evas_Event_Cb_Wrapper_Info structure.
 * @param event The Efl_Event that occurred.
 */
static void
_eo_evas_cb(void *data, const Efl_Event *event)
{
   Evas_Event_Cb_Wrapper_Info *info = data;
   Efl_Input_Event *efl_event_info = event->info;
   Evas *evas = event->object;
   void *event_info;

   if (!info->func.evas_cb) return;

   if (event->desc == EFL_CANVAS_SCENE_EVENT_OBJECT_FOCUS_IN ||
       event->desc == EFL_CANVAS_SCENE_EVENT_OBJECT_FOCUS_OUT)
     {
        event_info = efl_input_focus_object_get(efl_event_info);
        goto emit;
     }

   event_info = event->info;
   switch (info->efl_event_type)
     {
      case EFL_EVENT_TYPE_POINTER:
        event_info = efl_input_pointer_legacy_info_fill(evas, efl_event_info, info->type, NULL);
        break;

      case EFL_EVENT_TYPE_KEY:
        event_info = efl_input_key_legacy_info_fill(efl_event_info, NULL);
        break;

      case EFL_EVENT_TYPE_HOLD:
        event_info = efl_input_hold_legacy_info_fill(efl_event_info, NULL);
        break;

      case EFL_EVENT_TYPE_FOCUS:
      case EFL_EVENT_TYPE_NULL:
        event_info = NULL;
        break;

      case EFL_EVENT_TYPE_STRUCT:
      case EFL_EVENT_TYPE_OBJECT:
        break;
     }

emit:
   info->func.evas_cb(info->data, event->object, event_info);
}

/**
 * @brief Processes and calls post-event callbacks for an Evas canvas.
 *
 * This function iterates through the list of registered post-event callbacks
 * for the given Evas canvas and invokes them if their event ID is greater
 * than or equal to `min_event_id`. It handles canvas deletion and allows
 * callbacks to stop further processing.
 *
 * @param eo_e The Evas canvas object.
 * @param e The public data of the Evas canvas.
 * @param min_event_id The minimum event ID for a callback to be processed.
 *                     If 0, all post-event callbacks are considered.
 */
void
_evas_post_event_callback_call_real(Evas *eo_e, Evas_Public_Data *e, int min_event_id)
{
   Evas_Post_Callback *pc;
   Eina_List *l, *l_next;
   int skip = 0;

   if (e->delete_me) return;

   _evas_walk(e);
   e->running_post_events++;
   EINA_LIST_FOREACH_SAFE(e->post_events, l, l_next, pc)
     {
        if ((unsigned int) pc->event_id < (unsigned int) min_event_id) break;
        e->post_events = eina_list_remove_list(e->post_events, l);
        if ((!skip) && (!e->delete_me) && (!pc->delete_me))
          {
             if (!pc->func((void*)pc->data, eo_e)) skip = 1;
          }
        EVAS_MEMPOOL_FREE(_mp_pc, pc);
     }
   e->running_post_events--;
   _evas_unwalk(e);

   if (!e->running_post_events && e->post_events
       && (e->current_event == EVAS_CALLBACK_LAST))
     {
        WRN("Not all post-event callbacks have been processed!");
        _evas_post_event_callback_call_real(eo_e, e, 0);
     }
}

/**
 * @brief Frees all post-event callbacks associated with an Evas canvas.
 *
 * This function is typically called during canvas cleanup to release
 * resources held by pending post-event callbacks.
 *
 * @param eo_e The Evas canvas object.
 */
void
_evas_post_event_callback_free(Evas *eo_e)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Evas_Post_Callback *pc;

   if (EINA_LIKELY(!e->post_events)) return;

   EINA_LIST_FREE(e->post_events, pc)
     {
        EVAS_MEMPOOL_FREE(_mp_pc, pc);
     }
}

/**
 * @brief Deletes all legacy event callbacks from an Evas object.
 *
 * This function iterates through all registered legacy event callbacks
 * for the given Evas object and removes them. It also frees the
 * associated wrapper information.
 *
 * @param eo_obj The Evas object.
 */
void
evas_object_event_callback_all_del(Evas_Object *eo_obj)
{
   Evas_Event_Cb_Wrapper_Info *info;
   Eina_Inlist *itr;
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return;
   if (!obj->callbacks) return;
   EINA_INLIST_FOREACH_SAFE(obj->callbacks, itr, info)
     {
        efl_event_callback_del(eo_obj, _legacy_evas_callback_table(info->type), _eo_evas_object_cb, info);

        obj->callbacks =
           eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
        free(info);
     }
}

/**
 * @brief Cleans up all event callbacks for an Evas object.
 *
 * This is a convenience function that simply calls
 * evas_object_event_callback_all_del().
 *
 * @param eo_obj The Evas object.
 */
void
evas_object_event_callback_cleanup(Evas_Object *eo_obj)
{
   evas_object_event_callback_all_del(eo_obj);
}

/**
 * @brief Deletes all legacy event callbacks from an Evas canvas.
 *
 * This function iterates through all registered legacy event callbacks
 * for the given Evas canvas and removes them. It also frees the
 * associated wrapper information.
 *
 * @param eo_e The Evas canvas object.
 */
void
evas_event_callback_all_del(Evas *eo_e)
{
   Evas_Event_Cb_Wrapper_Info *info;
   Eina_Inlist *itr;
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (!e) return;
   if (!e->callbacks) return;

   EINA_INLIST_FOREACH_SAFE(e->callbacks, itr, info)
     {
        efl_event_callback_del(eo_e, _legacy_evas_callback_table(info->type), _eo_evas_cb, info);

        e->callbacks =
           eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
        free(info);
     }
}

/**
 * @brief Cleans up all event callbacks for an Evas canvas.
 *
 * This is a convenience function that simply calls
 * evas_event_callback_all_del().
 *
 * @param eo_e The Evas canvas object.
 */
void
evas_event_callback_cleanup(Evas *eo_e)
{
   evas_event_callback_all_del(eo_e);
}

/**
 * @brief Calls legacy Evas event callbacks for a given type on an Evas canvas.
 *
 * This function triggers the invocation of all legacy Evas event callbacks
 * registered for the specified type on the given Evas canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param type The Evas_Callback_Type of the event.
 * @param event_info The event-specific data.
 */
void
evas_event_callback_call(Evas *eo_e, Evas_Callback_Type type, void *event_info)
{
   efl_event_callback_legacy_call(eo_e, _legacy_evas_callback_table(type), event_info);
}

/**
 * @brief Handles compatibility for smart object legacy events.
 *
 * This function ensures that newer EFL events trigger corresponding legacy
 * smart object events for compatibility. For example, SHOW/HIDE events
 * also trigger VISIBILITY_CHANGED.
 *
 * @param eo_obj The Evas object.
 * @param efl_event_desc The EFL event description that occurred.
 * @param event_info The event-specific data.
 */
static void
_evas_callback_legacy_smart_compatibility_do_it(Evas_Object *eo_obj, const Efl_Event_Description *efl_event_desc, void *event_info)
{
   /* this is inverted: the base call is the legacy compat and this is the new event */
   if ((efl_event_desc == EFL_GFX_ENTITY_EVENT_SHOW) || (efl_event_desc == EFL_GFX_ENTITY_EVENT_HIDE))
     efl_event_callback_call(eo_obj, EFL_GFX_ENTITY_EVENT_VISIBILITY_CHANGED, event_info);
   else if ((efl_event_desc == EFL_GFX_IMAGE_EVENT_IMAGE_PRELOAD) || (efl_event_desc == EFL_GFX_IMAGE_EVENT_IMAGE_UNLOAD))
     efl_event_callback_call(eo_obj, EFL_GFX_IMAGE_EVENT_IMAGE_PRELOAD_STATE_CHANGED, event_info);
}

/**
 * @brief Calls legacy Evas event callbacks for a given type on an Evas object.
 *
 * This function is responsible for invoking legacy Evas event callbacks
 * on an Evas object. It handles event propagation to parent objects,
 * manages event IDs to prevent redundant calls, and deals with
 * special cases like multi-touch events derived from mouse events.
 * It also calls smart object callbacks and gesture filtering.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param type The Evas_Callback_Type of the event.
 * @param event_info The event-specific data.
 * @param event_id A unique ID for the event instance to prevent re-processing.
 * @param efl_event_desc The corresponding Efl_Event_Description for this event.
 */
void
evas_object_event_callback_call(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj,
                                Evas_Callback_Type type, void *event_info, int event_id,
                                const Efl_Event_Description *efl_event_desc)
{
   /* MEM OK */
   const Evas_Button_Flags CLICK_MASK = EVAS_BUTTON_DOUBLE_CLICK | EVAS_BUTTON_TRIPLE_CLICK;
   Evas_Button_Flags flags = EVAS_BUTTON_NONE;
   Evas_Callback_Type prev_type;
   Evas_Public_Data *e;

   if (!obj) return;
   if ((obj->delete_me) || (!obj->layer)) return;
   if ((obj->last_event_id == event_id) &&
       (obj->last_event_type == type)) return;
   if (obj->last_event_id > event_id)
     {
        if ((obj->last_event_type == EVAS_CALLBACK_MOUSE_OUT) &&
            ((type >= EVAS_CALLBACK_MOUSE_DOWN) &&
             (type <= EVAS_CALLBACK_MULTI_MOVE)))
          {
             return;
          }
     }
   obj->last_event_id = event_id;
   obj->last_event_type = type;
   if (!(e = obj->layer->evas)) return;

   _evas_walk(e);

   // gesture hook
   if ( type == EVAS_CALLBACK_MOUSE_MOVE ||
        type == EVAS_CALLBACK_MULTI_MOVE ||
        type == EVAS_CALLBACK_MOUSE_DOWN ||
        type == EVAS_CALLBACK_MULTI_DOWN ||
        type == EVAS_CALLBACK_MOUSE_UP ||
        type == EVAS_CALLBACK_MULTI_UP)
     _efl_canvas_gesture_manager_filter_event(e->gmd, eo_obj, event_info);

   if (obj->is_smart)
     _evas_object_smart_callback_call_internal(eo_obj, efl_event_desc);

   if (!_evas_object_callback_has_by_type(obj, type))
     goto nothing_here;

   if ((type == EVAS_CALLBACK_MOUSE_DOWN) || (type == EVAS_CALLBACK_MOUSE_UP))
     {
        flags = (Evas_Button_Flags)efl_input_pointer_button_flags_get(event_info);
        if (flags & CLICK_MASK)
          {
             if (obj->last_mouse_down_counter < (e->last_mouse_down_counter - 1))
               efl_input_pointer_button_flags_set(event_info, flags & ~CLICK_MASK);
          }
        obj->last_mouse_down_counter = e->last_mouse_down_counter;
     }

   if (!efl_event_desc)
     {
        /* This can happen for DEL and FREE which are defined only in here */
        efl_event_desc = _legacy_evas_callback_table(type);
     }

   prev_type = e->current_event;
   e->current_event = type;

   efl_event_callback_legacy_call(eo_obj, efl_event_desc, event_info);
   _evas_callback_legacy_smart_compatibility_do_it(eo_obj, efl_event_desc, event_info);

   /* multi events with finger 0 - only for eo callbacks */
   if (type == EVAS_CALLBACK_MOUSE_DOWN)
     {
        if (_evas_object_callback_has_by_type(obj, EVAS_CALLBACK_MULTI_DOWN))
          {
             e->current_event = EVAS_CALLBACK_MULTI_DOWN;
             efl_event_callback_call(eo_obj, EFL_EVENT_FINGER_DOWN, event_info);
          }
        efl_input_pointer_button_flags_set(event_info, (Efl_Pointer_Flags) flags);
     }
   else if (type == EVAS_CALLBACK_MOUSE_UP)
     {
        if (_evas_object_callback_has_by_type(obj, EVAS_CALLBACK_MULTI_UP))
          {
             e->current_event = EVAS_CALLBACK_MULTI_UP;
             efl_event_callback_call(eo_obj, EFL_EVENT_FINGER_UP, event_info);
          }
        efl_input_pointer_button_flags_set(event_info, (Efl_Pointer_Flags)flags);
     }
   else if (type == EVAS_CALLBACK_MOUSE_MOVE)
     {
        if (_evas_object_callback_has_by_type(obj, EVAS_CALLBACK_MULTI_MOVE))
          {
             e->current_event = EVAS_CALLBACK_MULTI_MOVE;
             efl_event_callback_call(eo_obj, EFL_EVENT_FINGER_MOVE, event_info);
          }
     }

   e->current_event = prev_type;

nothing_here:
   if (!obj->no_propagate)
     {
        if ((obj->smart.parent || ((obj->events) && obj->events->parent)) &&
            (type != EVAS_CALLBACK_FREE) &&
            (type <= EVAS_CALLBACK_KEY_UP))
          {
             Evas_Object_Protected_Data *parent_obj;
             Eo *parent;

             parent = ((obj->events) && obj->events->parent) ?
               obj->events->parent: obj->smart.parent;
             parent_obj = efl_data_scope_get(parent, EFL_CANVAS_OBJECT_CLASS);
             evas_object_event_callback_call(parent, parent_obj, type, event_info, event_id, efl_event_desc);
          }
     }
   _evas_unwalk(e);
}

/**
 * @brief Adds an event callback for a specific event type to an Evas object.
 * @param eo_obj The object to attach a callback to.
 * @param type The event type to trigger this callback.
 * @param func The function to call when the event is triggered.
 * @param data The data pointer to pass to @p func.
 * @see evas_object_event_callback_priority_add()
 * @see evas_object_event_callback_del()
 */
EVAS_API void
evas_object_event_callback_add(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func, const void *data)
{
   evas_object_event_callback_priority_add(eo_obj, type,
                                           EVAS_CALLBACK_PRIORITY_DEFAULT, func, data);
}

/**
 * @brief Adds an event callback for a specific event type to an Evas object with a given priority.
 * @param eo_obj The object to attach a callback to.
 * @param type The event type to trigger this callback.
 * @param priority The priority of the callback. Lower values are called earlier.
 * @param func The function to call when the event is triggered.
 * @param data The data pointer to pass to @p func.
 * @see evas_object_event_callback_add()
 * @see evas_object_event_callback_del()
 */
EVAS_API void
evas_object_event_callback_priority_add(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Callback_Priority priority, Evas_Object_Event_Cb func, const void *data)
{
   Evas_Object_Protected_Data *obj;
   Evas_Event_Cb_Wrapper_Info *cb_info;
   const Efl_Event_Description *desc;

   EINA_SAFETY_ON_NULL_RETURN(eo_obj);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_SAFETY_ON_TRUE_RETURN(efl_invalidated_get(eo_obj));

   obj = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(obj);

   cb_info = calloc(1, sizeof(*cb_info));
   cb_info->func.object_cb = func;
   cb_info->data = (void *)data;
   cb_info->type = type;
   cb_info->efl_event_type = _evas_event_efl_event_info_type(type);

   desc = _legacy_evas_callback_table(type);
   efl_event_callback_priority_add(eo_obj, desc, priority, _eo_evas_object_cb, cb_info);

   obj->callbacks =
      eina_inlist_append(obj->callbacks, EINA_INLIST_GET(cb_info));
}

/**
 * @brief Deletes a callback that was added with evas_object_event_callback_add() or evas_object_event_callback_priority_add().
 * @param eo_obj The object to delete the callback from.
 * @param type The event type the callback is registered for.
 * @param func The function that was registered.
 * @return The data pointer that was passed to evas_object_event_callback_add() or evas_object_event_callback_priority_add() when the callback was registered.
 *         Returns @c NULL if the callback is not found.
 * @see evas_object_event_callback_add()
 * @see evas_object_event_callback_priority_add()
 * @see evas_object_event_callback_del_full()
 */
EVAS_API void *
evas_object_event_callback_del(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func)
{
   Evas_Object_Protected_Data *obj;
   Evas_Event_Cb_Wrapper_Info *info;

   if (!eo_obj) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   obj = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);

   if (!obj->callbacks) return NULL;

   EINA_INLIST_REVERSE_FOREACH(obj->callbacks, info)
     {
        if ((info->func.object_cb == func) && (info->type == type))
          {
             void *tmp = info->data;
             efl_event_callback_del(eo_obj, _legacy_evas_callback_table(type), _eo_evas_object_cb, info);

             obj->callbacks =
                eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

/**
 * @brief Deletes a callback that was added with evas_object_event_callback_add() or evas_object_event_callback_priority_add(), matching the data pointer as well.
 * @param eo_obj The object to delete the callback from.
 * @param type The event type the callback is registered for.
 * @param func The function that was registered.
 * @param data The data pointer that was passed when the callback was registered.
 * @return The data pointer that was passed to evas_object_event_callback_add() or evas_object_event_callback_priority_add() when the callback was registered.
 *         Returns @c NULL if the callback is not found.
 * @see evas_object_event_callback_add()
 * @see evas_object_event_callback_priority_add()
 * @see evas_object_event_callback_del()
 */
EVAS_API void *
evas_object_event_callback_del_full(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func, const void *data)
{
   Evas_Object_Protected_Data *obj;
   Evas_Event_Cb_Wrapper_Info *info;

   if (!eo_obj) return NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   obj = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);

   if (!obj->callbacks) return NULL;

   EINA_INLIST_FOREACH(obj->callbacks, info)
     {
        if ((info->func.object_cb == func) && (info->type == type) && info->data == data)
          {
             void *tmp = info->data;
             efl_event_callback_del(eo_obj, _legacy_evas_callback_table(type), _eo_evas_object_cb, info);

             obj->callbacks =
                eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

/**
 * @brief Adds an event callback for a specific event type to an Evas canvas.
 * @param eo_e The Evas canvas to attach a callback to.
 * @param type The event type to trigger this callback.
 * @param func The function to call when the event is triggered.
 * @param data The data pointer to pass to @p func.
 * @see evas_event_callback_priority_add()
 * @see evas_event_callback_del()
 */
EVAS_API void
evas_event_callback_add(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func, const void *data)
{
   evas_event_callback_priority_add(eo_e, type, EVAS_CALLBACK_PRIORITY_DEFAULT,
                                    func, data);
}

/**
 * @brief Processes deferred Evas canvas callbacks.
 *
 * Callbacks for certain events (like EVAS_CALLBACK_RENDER_POST) might be
 * deferred if they are added during rendering or post-render phases.
 * This function processes and adds such deferred callbacks to the main
 * callback list.
 *
 * @param eo_e The Evas canvas object.
 * @param e The public data of the Evas canvas.
 */
void
_deferred_callbacks_process(Evas *eo_e, Evas_Public_Data *e)
{
   Evas_Event_Cb_Wrapper_Info *cb_info;
   const Efl_Event_Description *desc;

   while (e->deferred_callbacks)
     {
        cb_info = EINA_INLIST_CONTAINER_GET(e->deferred_callbacks,
                                            Evas_Event_Cb_Wrapper_Info);
        e->deferred_callbacks = eina_inlist_remove(e->deferred_callbacks,
                                                   e->deferred_callbacks);
        desc = _legacy_evas_callback_table(cb_info->type);
        efl_event_callback_priority_add(eo_e, desc, cb_info->priority, _eo_evas_cb, cb_info);
        e->callbacks = eina_inlist_append(e->callbacks, EINA_INLIST_GET(cb_info));
     }
}

/**
 * @brief Adds an event callback for a specific event type to an Evas canvas with a given priority.
 * @param eo_e The Evas canvas to attach a callback to.
 * @param type The event type to trigger this callback.
 * @param priority The priority of the callback. Lower values are called earlier.
 * @param func The function to call when the event is triggered.
 * @param data The data pointer to pass to @p func.
 * @see evas_event_callback_add()
 * @see evas_event_callback_del()
 */
EVAS_API void
evas_event_callback_priority_add(Evas *eo_e, Evas_Callback_Type type, Evas_Callback_Priority priority, Evas_Event_Cb func, const void *data)
{
   Evas_Public_Data *e;
   Evas_Event_Cb_Wrapper_Info *cb_info;
   const Efl_Event_Description *desc;

   EINA_SAFETY_ON_NULL_RETURN(eo_e);
   EINA_SAFETY_ON_NULL_RETURN(func);

   EINA_SAFETY_ON_TRUE_RETURN(efl_invalidated_get(eo_e));

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(e);

   cb_info = calloc(1, sizeof(*cb_info));
   cb_info->func.evas_cb = func;
   cb_info->data = (void *)data;
   cb_info->priority = priority;
   cb_info->type = type;
   cb_info->efl_event_type = _evas_event_efl_event_info_type(type);

   if ((e->rendering || e->inside_post_render) && type == EVAS_CALLBACK_RENDER_POST)
     {
        e->deferred_callbacks = eina_inlist_append(e->deferred_callbacks,
                                                   EINA_INLIST_GET(cb_info));
     }
   else
     {
        desc = _legacy_evas_callback_table(type);
        efl_event_callback_priority_add(eo_e, desc, priority, _eo_evas_cb, cb_info);

        e->callbacks = eina_inlist_append(e->callbacks, EINA_INLIST_GET(cb_info));
     }
}

/**
 * @brief Deletes a callback that was added with evas_event_callback_add() or evas_event_callback_priority_add().
 * @param eo_e The Evas canvas to delete the callback from.
 * @param type The event type the callback is registered for.
 * @param func The function that was registered.
 * @return The data pointer that was passed to evas_event_callback_add() or evas_event_callback_priority_add() when the callback was registered.
 *         Returns @c NULL if the callback is not found.
 * @see evas_event_callback_add()
 * @see evas_event_callback_priority_add()
 * @see evas_event_callback_del_full()
 */
EVAS_API void *
evas_event_callback_del(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func)
{
   Evas_Public_Data *e;
   Evas_Event_Cb_Wrapper_Info *info;

   EINA_SAFETY_ON_NULL_RETURN_VAL(eo_e, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);

   if (!e->callbacks) return NULL;

   if (type == EVAS_CALLBACK_RENDER_POST)
     EINA_INLIST_REVERSE_FOREACH(e->deferred_callbacks, info)
       {
          if (info->func.evas_cb == func)
            {
               void *tmp = info->data;

               e->deferred_callbacks =
                  eina_inlist_remove(e->deferred_callbacks, EINA_INLIST_GET(info));
               free(info);
               return tmp;
            }
       }

   EINA_INLIST_REVERSE_FOREACH(e->callbacks, info)
     {
        if ((info->func.evas_cb == func) && (info->type == type))
          {
             void *tmp = info->data;
             efl_event_callback_del(eo_e, _legacy_evas_callback_table(type), _eo_evas_cb, info);

             e->callbacks =
                eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

/**
 * @brief Deletes a callback that was added with evas_event_callback_add() or evas_event_callback_priority_add(), matching the data pointer as well.
 * @param eo_e The Evas canvas to delete the callback from.
 * @param type The event type the callback is registered for.
 * @param func The function that was registered.
 * @param data The data pointer that was passed when the callback was registered.
 * @return The data pointer that was passed to evas_event_callback_add() or evas_event_callback_priority_add() when the callback was registered.
 *         Returns @c NULL if the callback is not found.
 * @see evas_event_callback_add()
 * @see evas_event_callback_priority_add()
 * @see evas_event_callback_del()
 */
EVAS_API void *
evas_event_callback_del_full(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func, const void *data)
{
   Evas_Public_Data *e;
   Evas_Event_Cb_Wrapper_Info *info;

   EINA_SAFETY_ON_NULL_RETURN_VAL(eo_e, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(func, NULL);

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e, NULL);

   if (!e->callbacks) return NULL;

   if (type == EVAS_CALLBACK_RENDER_POST)
     EINA_INLIST_REVERSE_FOREACH(e->deferred_callbacks, info)
       {
          if ((info->func.evas_cb == func) && (info->data == data))
            {
               void *tmp = info->data;

               e->deferred_callbacks =
                  eina_inlist_remove(e->deferred_callbacks, EINA_INLIST_GET(info));
               free(info);
               return tmp;
            }
       }

   EINA_INLIST_FOREACH(e->callbacks, info)
     {
        if ((info->func.evas_cb == func) && (info->type == type) && (info->data == data))
          {
             void *tmp = info->data;
             efl_event_callback_del(eo_e, _legacy_evas_callback_table(type), _eo_evas_cb, info);

             e->callbacks =
                eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

/**
 * @brief Pushes a callback to be called after the current event processing is finished.
 *
 * This function registers a callback that will be invoked after the current
 * input event handling cycle is complete for the Evas canvas. It can only be
 * called from within an input event callback.
 *
 * @param eo_e The Evas canvas.
 * @param func The post-event callback function to push.
 * @param data User data to be passed to the callback function.
 */
EVAS_API void
evas_post_event_callback_push(Evas *eo_e, Evas_Object_Event_Post_Cb func, const void *data)
{
   Evas_Public_Data *e;
   Evas_Post_Callback *pc;

   EINA_SAFETY_ON_NULL_RETURN(eo_e);
   EINA_SAFETY_ON_TRUE_RETURN(efl_invalidated_get(eo_e));

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(e);
   if (e->delete_me) return;
   if (e->current_event == EVAS_CALLBACK_LAST)
     {
        ERR("%s() can only be called from an input event callback!", __func__);
        return;
     }
   EVAS_MEMPOOL_INIT(_mp_pc, "evas_post_callback", Evas_Post_Callback, 64, );
   pc = EVAS_MEMPOOL_ALLOC(_mp_pc, Evas_Post_Callback);
   if (!pc) return;
   EVAS_MEMPOOL_PREP(_mp_pc, pc, Evas_Post_Callback);

   pc->func = func;
   pc->data = data;
   pc->type = e->current_event;
   pc->event_id = _evas_event_counter;
   e->post_events = eina_list_prepend(e->post_events, pc);
}

/**
 * @brief Removes a previously pushed post-event callback.
 *
 * This function marks a post-event callback (identified by its function pointer)
 * for deletion. The callback will not be invoked if it hasn't run yet.
 *
 * @param eo_e The Evas canvas.
 * @param func The post-event callback function to remove.
 */
EVAS_API void
evas_post_event_callback_remove(Evas *eo_e, Evas_Object_Event_Post_Cb func)
{
   Evas_Public_Data *e;
   Evas_Post_Callback *pc;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(eo_e);

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(e);
   EINA_LIST_FOREACH(e->post_events, l, pc)
     {
        if (pc->func == func)
          {
             pc->delete_me = 1;
             return;
          }
     }
}

/**
 * @brief Removes a previously pushed post-event callback, matching function and data.
 *
 * This function marks a post-event callback (identified by its function pointer
 * and data pointer) for deletion. The callback will not be invoked if it
 * hasn't run yet.
 *
 * @param eo_e The Evas canvas.
 * @param func The post-event callback function to remove.
 * @param data The user data associated with the callback to remove.
 */
EVAS_API void
evas_post_event_callback_remove_full(Evas *eo_e, Evas_Object_Event_Post_Cb func, const void *data)
{
   Evas_Public_Data *e;
   Evas_Post_Callback *pc;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN(eo_e);

   e = efl_data_scope_safe_get(eo_e, EVAS_CANVAS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(e);
   EINA_LIST_FOREACH(e->post_events, l, pc)
     {
        if ((pc->func == func) && (pc->data == data))
          {
             pc->delete_me = 1;
             return;
          }
     }
}

/**
 * @brief Repeats animator tick events from the canvas to an object.
 *
 * This function is a callback that listens for EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK
 * on the Evas canvas. When the event occurs, it forwards (repeats) it as a
 * legacy animator tick event to the specified Evas object. This is used to
 * implement animator functionality for objects that need per-frame updates.
 *
 * @param data Pointer to the Evas_Object_Protected_Data of the target object.
 * @param event The EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK event from the canvas.
 */
static void
_animator_repeater(void *data, const Efl_Event *event)
{
   Evas_Object_Protected_Data *obj = data;

   efl_event_callback_legacy_call(obj->object, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, event->info);
   DBG("Emitting animator tick on %p.", obj->object);
}

/**
 * @brief Finalizes animator callback registration for an Evas object.
 *
 * This function is called when an Evas object is finalized. If the object
 * has pending animator references (meaning it needs animator ticks), this
 * function registers the `_animator_repeater` callback on the canvas to
 * forward animator ticks to this object.
 *
 * @param eo_obj The Evas object being finalized.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_callbacks_finalized(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   EINA_SAFETY_ON_NULL_RETURN(obj);

   if (obj->animator_ref > 0)
     {
       if (obj->layer && obj->layer->evas)
         {
            efl_event_callback_add(obj->layer->evas->evas, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_repeater, obj);
            DBG("Registering an animator tick on canvas %p for object %p.",
                obj->layer->evas->evas, obj->object);
         }
     }
}

/**
 * @brief Adds event catchers for an Evas object based on an array of callback descriptions.
 *
 * This function processes an array of Efl_Callback_Array_Item. For each item,
 * it performs actions like hooking into the gesture manager or managing
 * animator tick registrations. It also updates the object's internal
 * `callback_mask` to reflect which legacy Evas event types are being listened to.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param array An array of Efl_Callback_Array_Item, terminated by an item with a NULL desc.
 *              Example of an array item:
 *              { EFL_EVENT_POINTER_DOWN, _my_pointer_down_cb_func }
 *              The func pointer in the array item is not directly used here for legacy Evas callbacks,
 *              but the `desc` field is crucial.
 */
void
evas_object_callbacks_event_catcher_add(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, const Efl_Callback_Array_Item *array)
{
   Evas_Callback_Type type = EVAS_CALLBACK_LAST;
   int i;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (obj->layer && obj->layer->evas && obj->layer->evas->gesture_manager)
          _efl_canvas_gesture_manager_callback_add_hook(obj->layer->evas->gmd, obj->object, array[i].desc);

        if (array[i].desc == EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK)
          {
             if (obj->animator_ref++ > 0) break;

             if (efl_finalized_get(eo_obj))
               {
                  efl_event_callback_add(obj->layer->evas->evas, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_repeater, obj);
                  DBG("Registering an animator tick on canvas %p for object %p.",
                      obj->layer->evas->evas, obj->object);
               }
          }
        else if ((type = _legacy_evas_callback_type(array[i].desc)) != EVAS_CALLBACK_LAST)
          {
             obj->callback_mask |= (((uint64_t)1) << type);
          }
        else if (array[i].desc == EFL_GFX_ENTITY_EVENT_VISIBILITY_CHANGED)
          {
             obj->callback_mask |= (((uint64_t)1) << EVAS_CALLBACK_SHOW);
             obj->callback_mask |= (((uint64_t)1) << EVAS_CALLBACK_HIDE);
          }
     }
}

/**
 * @brief Deletes event catchers for an Evas object.
 *
 * This function processes an array of Efl_Callback_Array_Item. For each item,
 * it performs cleanup actions like unhooking from the gesture manager or
 * decrementing animator tick registration counts.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param array An array of Efl_Callback_Array_Item, terminated by an item with a NULL desc.
 *              Example of an array item:
 *              { EFL_EVENT_POINTER_DOWN, _my_pointer_down_cb_func }
 *              The func pointer in the array item is not directly used here,
 *              but the `desc` field is crucial.
 */
void
evas_object_callbacks_event_catcher_del(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, const Efl_Callback_Array_Item *array)
{
   int i;

   if (!obj->layer ||
       !obj->layer->evas)
     return ;

   for (i = 0; array[i].desc != NULL; i++)
     {
        if (obj->layer->evas->gesture_manager)
          _efl_canvas_gesture_manager_callback_del_hook(obj->layer->evas->gmd, obj->object, array[i].desc);

        if (array[i].desc == EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK)
          {
             if ((--obj->animator_ref) > 0) break;

             efl_event_callback_del(obj->layer->evas->evas, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_repeater, obj);
             DBG("Unregistering an animator tick on canvas %p for object %p.",
                 obj->layer->evas->evas, obj->object);
          }
     }
}

/**
 * @brief Shuts down callbacks for an Evas object, particularly animator-related ones.
 *
 * This function is called during object destruction or when callbacks are being
 * generally torn down. It ensures that if the object was registered for animator
 * ticks, its `_animator_repeater` callback is removed from the canvas.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 */
void
evas_object_callbacks_shutdown(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   if (obj->animator_ref > 0)
     efl_event_callback_del(obj->layer->evas->evas, EFL_CANVAS_OBJECT_EVENT_ANIMATOR_TICK, _animator_repeater, obj);
   obj->animator_ref = 0;
}
