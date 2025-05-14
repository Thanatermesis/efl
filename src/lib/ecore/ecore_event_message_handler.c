#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS ECORE_EVENT_MESSAGE_HANDLER_CLASS

//////////////////////////////////////////////////////////////////////////

typedef struct _Handler Handler;
typedef struct _Filter Filter;

/**
 * @internal
 * @brief Structure to hold event handler information.
 */
struct _Handler
{
   EINA_INLIST; /**< Macro to make this struct an element of an Eina_Inlist */
   Ecore_Event_Handler_Cb  func; /**< Callback function for the event handler */
   void                   *data; /**< User data to be passed to the callback */
   int                     type; /**< Event type this handler is for */
   Eina_Bool               delete_me : 1; /**< Flag indicating if the handler should be deleted */
   Eina_Bool               to_add : 1; /**< Flag indicating if the handler is pending addition */
   Eina_Bool               prepend : 1; /**< Flag indicating if the handler should be prepended */
};

/**
 * @internal
 * @brief Structure to hold event filter information.
 */
struct _Filter
{
   EINA_INLIST; /**< Macro to make this struct an element of an Eina_Inlist */
   Ecore_Data_Cb    func_start;  /**< Callback function called when the filter loop starts */
   Ecore_Filter_Cb  func_filter; /**< Callback function to filter events */
   Ecore_End_Cb     func_end;    /**< Callback function called when the filter loop ends */
   void            *data;        /**< User data to be passed to the callbacks */
   void            *loop_data;   /**< Data specific to a filter loop, returned by func_start */
   Eina_Bool        delete_me : 1; /**< Flag indicating if the filter should be deleted */
};

typedef struct _Ecore_Event_Message_Handler_Data Ecore_Event_Message_Handler_Data;

/**
 * @internal
 * @brief Private data for the Ecore_Event_Message_Handler class.
 *
 * This structure holds all the necessary information for managing event handlers
 * and filters within an Ecore event message handler instance.
 */
struct _Ecore_Event_Message_Handler_Data
{
   int           event_type_count; /**< Current number of registered event types */
   Eina_Inlist **handlers; /**< Array of Eina_Inlist, where each inlist contains handlers for a specific event type. The array is indexed by the event type. Example: handlers[ECORE_EVENT_MOUSE_BUTTON_DOWN] would be an Eina_Inlist of Handler structs for mouse button down events. */
   Eina_Inlist  *filters; /**< Eina_Inlist of Filter structs */
   Eina_List    *handlers_delete; /**< List of handlers marked for deletion */
   Eina_List    *handlers_add; /**< List of handlers pending addition */
   Eina_List    *filters_delete; /**< List of filters marked for deletion */
   Eina_List    *filters_add; /**< List of filters pending addition (currently unused) */
   void         *current_event_data; /**< Data of the event currently being processed */
   int           current_event_type; /**< Type of the event currently being processed */
   int           handlers_walking; /**< Counter to track if we are currently iterating over handlers (prevents modification issues) */
   int           filters_walking; /**< Counter to track if we are currently iterating over filters (prevents modification issues) */
};

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Applies registered event filters to a given message.
 *
 * This function is called by the Efl_Loop_Message infrastructure to filter
 * messages before they are dispatched. If a filter function returns EINA_FALSE,
 * the message is unsent (discarded).
 *
 * @param handler_pd Pointer to Ecore_Event_Message_Handler_Data.
 * @param msg_handler The message handler object (unused in current logic but kept for signature).
 * @param msg The message to be filtered.
 * @return EINA_TRUE if processing should continue, EINA_FALSE otherwise (though always returns EINA_TRUE currently).
 */
Eina_Bool
_ecore_event_do_filter(void *handler_pd, Eo *msg_handler, Eo *msg)
{
   Filter *f;
   void *ev;
   int type;
   Ecore_Event_Message_Handler_Data *eemhd = handler_pd;

   if (!eemhd->filters) return EINA_TRUE;
   if (!efl_isa(msg_handler, MY_CLASS)) return EINA_TRUE;
   eemhd->filters_walking++;
   EINA_INLIST_FOREACH(eemhd->filters, f)
     {
        if (f->delete_me) continue;
        type = -1;
        ev = NULL;
        ecore_event_message_data_get(msg, &type, &ev, NULL, NULL);
        if (type >= 0)
          {
             if (!f->func_filter(f->data, f->loop_data, type, ev))
               _efl_loop_message_unsend(msg);
          }
     }
   eemhd->filters_walking--;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Calls the start and end functions for all registered event filters.
 *
 * This function is typically invoked by the main loop before and after
 * processing a batch of messages. It calls `func_start` for each filter
 * before message processing and `func_end` after. It also triggers
 * the actual filtering of messages via `_efl_loop_messages_filter`.
 *
 * @param obj The Efl_Loop object.
 * @param pd The Efl_Loop_Data associated with the loop.
 */
void
_ecore_event_filters_call(Eo *obj, Efl_Loop_Data *pd)
{
   Filter *f;
   Ecore_Event_Message_Handler_Data *eemhd;
   Eo *ecore_event_handler = efl_provider_find(obj, ECORE_EVENT_MESSAGE_HANDLER_CLASS);
   /* If this is not != NULL, then _ecore_event_init was not called yet, which means,
      there cannot be any registered events yet
    */
   if (!ecore_event_handler) return;

   eemhd = efl_data_scope_get(ecore_event_handler, MY_CLASS);
   if (!eemhd) return;
   if (!eemhd->filters) return;
   eemhd->filters_walking++;
   EINA_INLIST_FOREACH(eemhd->filters, f)
     {
        if (f->delete_me) continue;
        if (f->func_start) f->loop_data = f->func_start(f->data);
     }
   _efl_loop_messages_filter(obj, pd, eemhd);
   EINA_INLIST_FOREACH(eemhd->filters, f)
     {
        if (f->delete_me) continue;
        if (f->func_end) f->func_end(f->data, f->loop_data);
     }
   eemhd->filters_walking--;
   if (eemhd->filters_walking == 0)
     {
        EINA_LIST_FREE(eemhd->filters_delete, f)
          {
             free(f);
          }
     }
}

//////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Creates a new Ecore_Event_Message object.
 *
 * This function is an EOLIAN method implementation.
 * It currently acts as a simple wrapper to `efl_add` for creating
 * ECORE_EVENT_MESSAGE_CLASS instances. The comment "XXX: implemented event obj cache"
 * suggests a potential future optimization for caching these objects.
 *
 * @param obj The Ecore_Event_Message_Handler object.
 * @param pd Private data of the Ecore_Event_Message_Handler object (unused).
 * @return A new Ecore_Event_Message object, or NULL on failure.
 */
EOLIAN static Ecore_Event_Message *
_ecore_event_message_handler_message_type_add(Eo *obj, Ecore_Event_Message_Handler_Data *pd EINA_UNUSED)
{
   // XXX: implemented event obj cache
   return efl_add(ECORE_EVENT_MESSAGE_CLASS, obj);
}

/**
 * @internal
 * @brief Allocates a new event type identifier.
 *
 * This function is an EOLIAN method implementation.
 * It increases the internal count of event types and resizes the `handlers`
 * array to accommodate the new type.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @return The new event type identifier (an integer > 0), or 0 on failure.
 *         Example: If current event_type_count is 5, this returns 6.
 */
EOLIAN static int
_ecore_event_message_handler_type_new(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd)
{
   Eina_Inlist **tmp;
   int evnum;

   evnum = pd->event_type_count + 1;
   tmp = realloc(pd->handlers, sizeof(Eina_Inlist *) * (evnum + 1));
   if (!tmp) return 0;
   pd->handlers = tmp;
   pd->handlers[ECORE_EVENT_NONE] = NULL;
   pd->handlers[evnum] = NULL;
   pd->event_type_count = evnum;
   return evnum;
}

/**
 * @internal
 * @brief Adds an event handler for a specific event type.
 *
 * This function is an EOLIAN method implementation.
 * The handler is appended to the list of handlers for the given type.
 * If handlers for this type are currently being processed (walked),
 * the new handler is queued and added later to avoid modification issues.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param type The event type to handle. Must be > 0 and <= pd->event_type_count.
 * @param func The callback function to execute when the event occurs.
 *             Signature: Eina_Bool (*Ecore_Event_Handler_Cb)(void *data, int type, void *event)
 * @param data User-provided data to pass to the callback function.
 * @return A handle to the added event handler, or NULL on failure.
 *         This handle is used for later deletion or modification.
 */
EOLIAN static void *
_ecore_event_message_handler_handler_add(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd, int type, void *func, void *data)
{
   Handler *h;

   EINA_SAFETY_ON_TRUE_RETURN_VAL((type <= 0) || (type > pd->event_type_count) || (!func), NULL);
   h = calloc(1, sizeof(Handler));
   if (!h) return NULL;
   h->func = func;
   h->data = data;
   h->type = type;
   if (pd->current_event_type == type)
     {
        h->to_add = EINA_TRUE;
        pd->handlers_add = eina_list_append(pd->handlers_add, h);
     }
   else
     pd->handlers[type] = eina_inlist_append(pd->handlers[type],
                                             EINA_INLIST_GET(h));
   return h;
}

/**
 * @internal
 * @brief Prepends an event handler for a specific event type.
 *
 * This function is an EOLIAN method implementation.
 * The handler is prepended to the list of handlers for the given type.
 * If handlers for this type are currently being processed (walked),
 * the new handler is queued and prepended later.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param type The event type to handle. Must be >= 0 and <= pd->event_type_count.
 *             (Note: type 0, ECORE_EVENT_NONE, is technically allowed by check but handlers[0] is NULL).
 * @param func The callback function to execute when the event occurs.
 *             Signature: Eina_Bool (*Ecore_Event_Handler_Cb)(void *data, int type, void *event)
 * @param data User-provided data to pass to the callback function.
 * @return A handle to the prepended event handler, or NULL on failure.
 */
EOLIAN static void *
_ecore_event_message_handler_handler_prepend(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd, int type, void *func, void *data)
{
   Handler *h;

   if ((type < 0) || (type > pd->event_type_count) || (!func)) return NULL;
   h = calloc(1, sizeof(Handler));
   if (!h) return NULL;
   h->func = func;
   h->data = data;
   h->type = type;
   if (pd->current_event_type == type)
     {
        h->to_add = EINA_TRUE;
        h->prepend = EINA_TRUE;
        pd->handlers_add = eina_list_append(pd->handlers_add, h);
     }
   else
     pd->handlers[type] = eina_inlist_prepend(pd->handlers[type],
                                             EINA_INLIST_GET(h));
   return h;
}

/**
 * @internal
 * @brief Deletes an event handler.
 *
 * This function is an EOLIAN method implementation.
 * If handlers are currently being processed, the handler is marked for
 * deletion and removed later. Otherwise, it's removed immediately.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param handler The handle of the event handler to delete (as returned by add/prepend).
 * @return The user data associated with the deleted handler, or NULL if handler is invalid.
 */
EOLIAN static void *
_ecore_event_message_handler_handler_del(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd, void *handler)
{
   Handler *h = handler;
   void *data;

   if (!h) return NULL;
   if ((h->type < 0) || (h->type > pd->event_type_count)) return NULL;
   data = h->data;
   if (pd->handlers_walking > 0)
     {
        if (h->to_add)
          {
             h->to_add = EINA_FALSE;
             pd->handlers_add = eina_list_remove(pd->handlers_add, h);
          }

        h->delete_me = EINA_TRUE;
        pd->handlers_delete = eina_list_append(pd->handlers_delete, h);
     }
   else
     {
        if (h->to_add)
          pd->handlers_add = eina_list_remove(pd->handlers_add, h);
        else
          pd->handlers[h->type] = eina_inlist_remove(pd->handlers[h->type],
                                                     EINA_INLIST_GET(h));
        free(h);
     }
   return data;
}

/**
 * @internal
 * @brief Gets the user data associated with an event handler.
 *
 * This function is an EOLIAN method implementation.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object (unused).
 * @param handler The handle of the event handler.
 * @return The user data associated with the handler, or NULL if handler is invalid.
 */
EOLIAN static void *
_ecore_event_message_handler_handler_data_get(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd EINA_UNUSED, void *handler)
{
   Handler *h = handler;

   if (!h) return NULL;
   return h->data;
}

/**
 * @internal
 * @brief Sets the user data associated with an event handler.
 *
 * This function is an EOLIAN method implementation.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object (unused).
 * @param handler The handle of the event handler.
 * @param data The new user data to associate with the handler.
 * @return The previous user data associated with the handler, or NULL if handler is invalid.
 */
EOLIAN static void *
_ecore_event_message_handler_handler_data_set(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd EINA_UNUSED, void *handler, void *data)
{
   Handler *h = handler;
   void *prev_data;

   if (!h) return NULL;
   prev_data = h->data;
   h->data = data;
   return prev_data;
}

/**
 * @internal
 * @brief Adds an event filter.
 *
 * This function is an EOLIAN method implementation.
 * Filters are processed in the order they are added.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param func_start Callback function executed when the filter loop begins.
 *                   Signature: void *(*Ecore_Data_Cb)(void *data)
 *                   Can be NULL. Its return value is passed as loop_data to func_filter and func_end.
 * @param func_filter The main filter callback function.
 *                    Signature: Eina_Bool (*Ecore_Filter_Cb)(void *data, void *loop_data, int type, void *event)
 *                    Must not be NULL. Return EINA_FALSE to stop/drop the event.
 * @param func_end Callback function executed when the filter loop ends.
 *                 Signature: void (*Ecore_End_Cb)(void *data, void *loop_data)
 *                 Can be NULL.
 * @param data User-provided data to pass to the callback functions.
 * @return A handle to the added filter, or NULL on failure (e.g., func_filter is NULL).
 */
EOLIAN static void *
_ecore_event_message_handler_filter_add(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd, void *func_start, void *func_filter, void *func_end, void *data)
{
   Filter *f;

   if (!func_filter) return NULL;
   f = calloc(1, sizeof(Filter));
   if (!f) return NULL;
   f->func_start = func_start;
   f->func_filter = func_filter;
   f->func_end = func_end;
   f->data = data;
   pd->filters = eina_inlist_append(pd->filters, EINA_INLIST_GET(f));
   return f;
}

/**
 * @internal
 * @brief Deletes an event filter.
 *
 * This function is an EOLIAN method implementation.
 * If filters are currently being processed, the filter is marked for
 * deletion and removed later. Otherwise, it's removed immediately.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param filter The handle of the filter to delete (as returned by filter_add).
 * @return The user data associated with the deleted filter, or NULL if filter is invalid.
 */
EOLIAN static void *
_ecore_event_message_handler_filter_del(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd, void *filter)
{
   Filter *f = filter;
   void *data;

   if (!f) return NULL;
   data = f->data;
   if (pd->filters_walking > 0)
     {
        f->delete_me = EINA_TRUE;
        pd->filters_delete = eina_list_append(pd->filters_delete, f);
     }
   else
     {
        pd->filters = eina_inlist_remove(pd->filters, EINA_INLIST_GET(f));
        free(f);
     }
   return data;
}

/**
 * @internal
 * @brief Gets the type of the event currently being processed.
 *
 * This function is an EOLIAN method implementation.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @return The type of the event currently being processed, or -1 if no event is being processed.
 */
EOLIAN static int
_ecore_event_message_handler_current_type_get(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd)
{
   return pd->current_event_type;
}

/**
 * @internal
 * @brief Gets the data of the event currently being processed.
 *
 * This function is an EOLIAN method implementation.
 *
 * @param obj The Ecore_Event_Message_Handler object (unused).
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @return The data of the event currently being processed, or NULL if no event is being processed.
 */
EOLIAN static void *
_ecore_event_message_handler_current_event_get(Eo *obj EINA_UNUSED, Ecore_Event_Message_Handler_Data *pd)
{
   return pd->current_event_data;
}

/**
 * @internal
 * @brief Constructor for the Ecore_Event_Message_Handler object.
 *
 * This function is an EOLIAN method implementation.
 * It initializes the private data structure for the event handler.
 *
 * @param obj The Ecore_Event_Message_Handler object being constructed.
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @return The constructed object.
 */
EOLIAN static Efl_Object *
_ecore_event_message_handler_efl_object_constructor(Eo *obj, Ecore_Event_Message_Handler_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->event_type_count = 0;
   pd->current_event_type = -1;
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ecore_Event_Message_Handler object.
 *
 * This function is an EOLIAN method implementation.
 * It cleans up all allocated resources, including handlers, filters,
 * and internal lists. It warns if destruction occurs while handlers are
 * being walked, as this can lead to instability.
 *
 * @param obj The Ecore_Event_Message_Handler object being destructed.
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 */
EOLIAN static void
_ecore_event_message_handler_efl_object_destructor(Eo *obj, Ecore_Event_Message_Handler_Data *pd)
{
   Handler *h;
   int i;

   if (pd->handlers_walking == 0)
     {
        EINA_LIST_FREE(pd->handlers_delete, h)
          {
             pd->handlers[h->type] =
               eina_inlist_remove(pd->handlers[h->type],
                                  EINA_INLIST_GET(h));
             free(h);
          }
        EINA_LIST_FREE(pd->handlers_add, h)
          {
             free(h);
          }
        for (i = 1; i <= pd->event_type_count; i++)
          {
             EINA_INLIST_FREE(pd->handlers[i], h)
               {
                  pd->handlers[i] = eina_inlist_remove(pd->handlers[i],
                                                       EINA_INLIST_GET(h));
                  free(h);
               }
          }
        free(pd->handlers);
        pd->handlers = NULL;
     }
   else
     {
        ERR("Destruction of ecore_event_message_handler while walking events");
     }
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Processes an incoming message (event).
 *
 * This function is an EOLIAN method implementation for Efl.Loop.Message_Handler.message_call.
 * It is the core of event dispatching for this handler.
 * It retrieves event data from the message, iterates through registered handlers
 * for the event type, and calls their callback functions.
 * It also handles deferred addition/deletion of handlers and manages the
 * lifecycle of event data (freeing it after processing).
 * If an ECORE_EVENT_SIGNAL_EXIT is received and not handled, it quits the main loop.
 * Finally, it calls the ECORE_EVENT_MESSAGE_HANDLER_EVENT_MESSAGE_ECORE_EVENT Eo event
 * and then upcalls to the parent class's message_call.
 *
 * @param obj The Ecore_Event_Message_Handler object.
 * @param pd Private data of the Ecore_Event_Message_Handler object.
 * @param message The Efl_Loop_Message (event) to process.
 */
EOLIAN static void
_ecore_event_message_handler_efl_loop_message_handler_message_call(Eo *obj, Ecore_Event_Message_Handler_Data *pd, Efl_Loop_Message *message)
{
   Handler *h;
   int type = -1;
   void *data = NULL, *free_func = NULL, *free_data = NULL;
   Ecore_End_Cb fn_free = NULL;
   Eina_List *l, *l2;
   int handled = 0;

   // call legacy handlers which are controled by this class' custom api
   ecore_event_message_data_steal
     (message, &type, &data, &free_func, &free_data);
   if ((type >= 0) && (type <= pd->event_type_count))
     {
        if (free_func) fn_free = free_func;
        pd->current_event_data = data;
        pd->current_event_type = type;
        pd->handlers_walking++;
        EINA_INLIST_FOREACH(pd->handlers[type], h)
          {
             if (h->delete_me) continue;
             handled++;
             if (!h->func(h->data, h->type, data)) break;
          }
        pd->handlers_walking--;
        pd->current_event_data = NULL;
        pd->current_event_type = -1;
        EINA_LIST_FOREACH_SAFE(pd->handlers_add, l, l2, h)
          {
             if (h->type == type)
               {
                  h->to_add = EINA_FALSE;
                  pd->handlers_add =
                    eina_list_remove_list(pd->handlers_add, l);
                  if (h->prepend)
                    pd->handlers[type] =
                      eina_inlist_prepend(pd->handlers[type], EINA_INLIST_GET(h));
                  else
                    pd->handlers[type] =
                      eina_inlist_append(pd->handlers[type], EINA_INLIST_GET(h));
               }
          }
        if (pd->handlers_walking == 0)
          {
             EINA_LIST_FREE(pd->handlers_delete, h)
               {
                  if (h->to_add)
                    pd->handlers_add = eina_list_remove(pd->handlers_add, h);
                  else
                    pd->handlers[h->type] =
                      eina_inlist_remove(pd->handlers[h->type],
                                         EINA_INLIST_GET(h));
                  free(h);
               }
          }
        if ((type == ECORE_EVENT_SIGNAL_EXIT) && (handled == 0))
          {
             Eo *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
             if (loop) efl_loop_quit(loop, eina_value_int_init(0));
          }
     }

   efl_event_callback_call
     (obj, ECORE_EVENT_MESSAGE_HANDLER_EVENT_MESSAGE_ECORE_EVENT, message);
   efl_loop_message_handler_message_call
     (efl_super(obj, MY_CLASS), message);

   if (data)
     {
        if (fn_free) fn_free(free_data, data);
        else free(data);
     }
}

/**
 * @internal
 * @brief Callback used by `_ecore_event_message_handler_type_flush` to filter messages.
 *
 * This function is passed to `_efl_loop_messages_call`. It checks if the
 * type of the given `message` matches the `type` provided in `data`.
 * It returns EINA_TRUE if the types do *not* match (i.e., keep the message),
 * and EINA_FALSE if they *do* match (i.e., process/remove this message as part of flush).
 *
 * @param data A pointer to an integer representing the event type to flush.
 * @param handler Unused.
 * @param message The Ecore_Event_Message to check.
 * @return EINA_TRUE if the message type is different from the type to flush, EINA_FALSE otherwise.
 */
static Eina_Bool
_flush_cb(void *data, void *handler EINA_UNUSED, void *message)
{
   int *type = data;
   int evtype = -1;

   if (!efl_isa(message, ECORE_EVENT_MESSAGE_CLASS)) return EINA_TRUE;
   ecore_event_message_data_get(message, &evtype, NULL, NULL, NULL);
   return *type != evtype;
}

/**
 * @internal
 * @brief Flushes all pending events of a specific type from the event queue.
 *
 * This function is an EOLIAN method implementation.
 * It iterates through the message queue of the associated loop and processes
 * (effectively removing) messages that match the given `type`.
 *
 * @param obj The Ecore_Event_Message_Handler object.
 * @param pd Private data of the Ecore_Event_Message_Handler object (unused).
 * @param type The event type to flush.
 *             Example: ECORE_EVENT_MOUSE_MOVE. All pending mouse move events will be processed.
 */
EOLIAN static void
_ecore_event_message_handler_type_flush(Eo *obj, Ecore_Event_Message_Handler_Data *pd EINA_UNUSED, int type)
{
   Eo *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
   Efl_Loop_Data *loop_data = efl_data_scope_get(loop, EFL_LOOP_CLASS);

   if (loop && loop_data)
     {
        _efl_loop_messages_call(loop, loop_data, _flush_cb, &type);
     }
}

//////////////////////////////////////////////////////////////////////////

#include "ecore_event_message_handler.eo.c"
