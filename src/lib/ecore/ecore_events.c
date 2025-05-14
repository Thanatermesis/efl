#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include "Ecore.h"
#include "ecore_private.h"

/**
 * @internal
 * @brief Global static variable holding the event message handler.
 * This handler is responsible for managing and dispatching all legacy Ecore events.
 */
static Ecore_Event_Message_Handler *_event_msg_handler = NULL;

EAPI Ecore_Event_Handler *
ecore_event_handler_add(int                    type,
                        Ecore_Event_Handler_Cb func,
                        const void            *data)
{
   // This function is a wrapper around ecore_event_message_handler_add.
   // It adds an event handler for a specific event type.
   return ecore_event_message_handler_add(_event_msg_handler,
                                          type, func, (void *)data);
}

EAPI Ecore_Event_Handler *
ecore_event_handler_prepend(int                    type,
                        Ecore_Event_Handler_Cb func,
                        const void            *data)
{
   // This function is a wrapper around ecore_event_message_handler_prepend.
   // It prepends an event handler for a specific event type, so it's called before others.
   return ecore_event_message_handler_prepend(_event_msg_handler,
                                          type, func, (void *)data);
}

EAPI void *
ecore_event_handler_del(Ecore_Event_Handler *event_handler)
{
   // This function is a wrapper around ecore_event_message_handler_del.
   // It deletes an event handler.
   return ecore_event_message_handler_del(_event_msg_handler,
                                          event_handler);
}

EAPI void *
ecore_event_handler_data_get(Ecore_Event_Handler *eh)
{
   // This function is a wrapper around ecore_event_message_handler_data_get.
   // It retrieves the data associated with an event handler.
   return ecore_event_message_handler_data_get(_event_msg_handler, eh);
}

EAPI void *
ecore_event_handler_data_set(Ecore_Event_Handler *eh,
                             const void          *data)
{
   // This function is a wrapper around ecore_event_message_handler_data_set.
   // It sets the data associated with an event handler.
   return ecore_event_message_handler_data_set(_event_msg_handler, eh,
                                               (void *)data);
}

EAPI Ecore_Event *
ecore_event_add(int          type,
                void        *ev,
                Ecore_End_Cb func_free,
                void        *data)
{
   // This function adds a new event to the event queue.
   // It creates a message, sets its data, and sends it via the message handler.
   Ecore_Event_Message *msg;
   if (type <= ECORE_EVENT_NONE) return NULL; // Basic validation for event type

   msg = ecore_event_message_handler_message_type_add(_event_msg_handler);
   if (msg)
     {
        ecore_event_message_data_set(msg, type, ev, func_free, data);
        efl_loop_message_handler_message_send(_event_msg_handler, msg);
     }
   return (Ecore_Event *)msg;
}

EAPI void *
ecore_event_del(Ecore_Event *event)
{
   // This function deletes an event from the queue.
   // It retrieves associated data before unsending the message.
   void *data = NULL;
   if (!event) return data;
   ecore_event_message_data_get((Eo *)event, NULL, NULL, NULL, &data);
   _efl_loop_message_unsend((Eo *)event); // Internal function to unsend/delete
   return data;
}

EAPI int
ecore_event_type_new(void)
{
   // This function is a wrapper around ecore_event_message_handler_type_new.
   // It registers a new event type and returns its ID.
   return ecore_event_message_handler_type_new(_event_msg_handler);
}

EAPI Ecore_Event_Filter *
ecore_event_filter_add(Ecore_Data_Cb   func_start,
                       Ecore_Filter_Cb func_filter,
                       Ecore_End_Cb    func_end,
                       const void     *data)
{
   // This function is a wrapper around ecore_event_message_handler_filter_add.
   // It adds an event filter that can intercept and potentially modify or drop events.
   return ecore_event_message_handler_filter_add(_event_msg_handler,
                                                 func_start, func_filter,
                                                 func_end, (void *)data);
}

EAPI void *
ecore_event_filter_del(Ecore_Event_Filter *ef)
{
   // This function is a wrapper around ecore_event_message_handler_filter_del.
   // It deletes an event filter.
   return ecore_event_message_handler_filter_del(_event_msg_handler, ef);
}

EAPI int
ecore_event_current_type_get(void)
{
   // This function is a wrapper around ecore_event_message_handler_current_type_get.
   // It gets the type of the event currently being processed.
   return ecore_event_message_handler_current_type_get(_event_msg_handler);
}

EAPI void *
ecore_event_current_event_get(void)
{
   // This function is a wrapper around ecore_event_message_handler_current_event_get.
   // It gets the event data of the event currently being processed.
   return ecore_event_message_handler_current_event_get(_event_msg_handler);
}

/**
 * @internal
 * @brief Initializes the Ecore event subsystem.
 * This function sets up the main event message handler and registers
 * some core legacy event types.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_ecore_event_init(void)
{
   const char *choice = getenv("EINA_MEMPOOL");
   if ((!choice) || (!choice[0])) choice = "chained_mempool";

   _event_msg_handler = efl_add(ECORE_EVENT_MESSAGE_HANDLER_CLASS, _mainloop_singleton);
   efl_provider_register(_mainloop_singleton, ECORE_EVENT_MESSAGE_HANDLER_CLASS, _event_msg_handler);

   if (!_event_msg_handler)
     {
        ERR("Cannot create legacy ecore event message handler");
        return EINA_FALSE;
     }
   // init some core legacy event types in t he same order and numbering as before
   // ECORE_EVENT_NONE                     0
   // no need to do as ev types start at 1

   // ECORE_EVENT_SIGNAL_USER              1
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_SIGNAL_HUP               2
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_SIGNAL_EXIT              3
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_SIGNAL_POWER             4
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_SIGNAL_REALTIME          5
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_MEMORY_STATE             6
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_POWER_STATE              7
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_LOCALE_CHANGED           8
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_HOSTNAME_CHANGED         9
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_SYSTEM_TIMEDATE_CHANGED  10
   ecore_event_message_handler_type_new(_event_msg_handler);
   // ECORE_EVENT_COUNT                    11
   // no need to do as it was a count, nto an event

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Ecore event subsystem.
 * This function clears any pending messages from the event handler
 * and releases the handler itself.
 */
void
_ecore_event_shutdown(void)
{
   efl_loop_message_handler_message_clear(_event_msg_handler);
   _event_msg_handler = NULL;
}

/**
 * @internal
 * @brief Allocates and zero-initializes a new Ecore_Event_Signal_User structure.
 * @return Pointer to the allocated structure, or NULL on failure.
 */
void *
_ecore_event_signal_user_new(void)
{
   return calloc(1, sizeof(Ecore_Event_Signal_User));
}

/**
 * @internal
 * @brief Allocates and zero-initializes a new Ecore_Event_Signal_Hup structure.
 * @return Pointer to the allocated structure, or NULL on failure.
 */
void *
_ecore_event_signal_hup_new(void)
{
   return calloc(1, sizeof(Ecore_Event_Signal_Hup));
}

/**
 * @internal
 * @brief Allocates and zero-initializes a new Ecore_Event_Signal_Exit structure.
 * @return Pointer to the allocated structure, or NULL on failure.
 */
void *
_ecore_event_signal_exit_new(void)
{
   return calloc(1, sizeof(Ecore_Event_Signal_Exit));
}

/**
 * @internal
 * @brief Allocates and zero-initializes a new Ecore_Event_Signal_Power structure.
 * @return Pointer to the allocated structure, or NULL on failure.
 */
void *
_ecore_event_signal_power_new(void)
{
   return calloc(1, sizeof(Ecore_Event_Signal_Power));
}

/**
 * @internal
 * @brief Allocates and zero-initializes a new Ecore_Event_Signal_Realtime structure.
 * @return Pointer to the allocated structure, or NULL on failure.
 */
void *
_ecore_event_signal_realtime_new(void)
{
   return calloc(1, sizeof(Ecore_Event_Signal_Realtime));
}

EAPI void
ecore_event_type_flush_internal(int type, ...)
{
   // This function flushes events of specified types from the event queue.
   // It takes a variable number of event types, terminated by ECORE_EVENT_NONE.
   va_list args;

   if (type == ECORE_EVENT_NONE) return;
   ecore_event_message_handler_type_flush(_event_msg_handler, type);

   va_start(args, type);
   for (;;)
     {
        type = va_arg(args, int);
        if (type == ECORE_EVENT_NONE) break;
        ecore_event_message_handler_type_flush(_event_msg_handler, type);
     }
   va_end(args);
}
