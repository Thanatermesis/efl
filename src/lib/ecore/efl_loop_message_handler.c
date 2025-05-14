#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_MESSAGE_HANDLER_CLASS

#include "ecore_main_common.h"

typedef struct _Efl_Loop_Message_Handler_Data Efl_Loop_Message_Handler_Data;

struct _Efl_Loop_Message_Handler_Data
{
   Eo *loop;
   Efl_Loop_Data *loop_data;
};

/**
 * @brief Adds a new message to be processed by the message handler.
 *
 * This function creates a new message object associated with the handler.
 * The message is not yet sent; it's prepared for sending.
 *
 * @param obj The Efl_Loop_Message_Handler object.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 * @return A new Efl_Loop_Message object, or NULL on failure.
 */
EOLIAN static Efl_Loop_Message *
_efl_loop_message_handler_message_add(Eo *obj, Efl_Loop_Message_Handler_Data *pd EINA_UNUSED)
{
   // XXX: implement message object cache
   Efl_Loop_Message *message = efl_add(EFL_LOOP_MESSAGE_CLASS, obj);
   if (!message) return NULL;
   // XXX: track added messages not sent yet ...
   return message;
}

/**
 * @brief Sends a message to the associated event loop for processing.
 *
 * This function queues the given message to be handled by the main loop.
 * It initializes the loop and loop_data if they haven't been already.
 * Messages are added to a pending queue if the main loop is currently
 * iterating through messages (`message_walking > 0`), otherwise they are
 * added to the main message queue.
 *
 * @param obj The Efl_Loop_Message_Handler object.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 * @param message The Efl_Loop_Message object to send.
 */
EOLIAN static void
_efl_loop_message_handler_message_send(Eo *obj, Efl_Loop_Message_Handler_Data *pd, Efl_Loop_Message *message)
{
   Message *msg;

   if (EINA_UNLIKELY(!pd->loop))
     {
        pd->loop = efl_provider_find(obj, EFL_LOOP_CLASS);
        if (!pd->loop) return;
        pd->loop_data = efl_data_scope_get(pd->loop, EFL_LOOP_CLASS);
        if (!pd->loop_data)
          {
             pd->loop = NULL;
             return;
          }
     }
   msg = calloc(1, sizeof(Message));
   if (msg)
     {
        msg->handler = obj;
        msg->message = message;
        if (pd->loop_data->message_walking > 0)
          {
             pd->loop_data->message_pending_queue = eina_inlist_append
                (pd->loop_data->message_pending_queue, EINA_INLIST_GET(msg));
          }
        else
          {
             pd->loop_data->message_queue = eina_inlist_append
                (pd->loop_data->message_queue, EINA_INLIST_GET(msg));
          }
        _efl_loop_message_send_info_set(message, EINA_INLIST_GET(msg),
                                       pd->loop, pd->loop_data);
        return;
     }
   efl_del(message);
}

/**
 * @brief Processes a message that has been received by the loop.
 *
 * This function is called by the event loop when it's time to handle a message.
 * It finds the message in the queue, removes it, and triggers the
 * EFL_LOOP_MESSAGE_EVENT_MESSAGE event on the message object itself,
 * and the EFL_LOOP_MESSAGE_HANDLER_EVENT_MESSAGE event on the handler.
 * If the loop is iterating messages (`message_walking > 0`) and the
 * message is not the first one, it's marked for deletion (`delete_me`)
 * to be cleaned up later, otherwise it's freed immediately.
 *
 * @param obj The Efl_Loop_Message_Handler object.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 * @param message The Efl_Loop_Message object to process.
 */
EOLIAN static void
_efl_loop_message_handler_message_call(Eo *obj, Efl_Loop_Message_Handler_Data *pd, Efl_Loop_Message *message)
{
   Message *msg;
   unsigned int n = 0;
   Eina_Bool found = EINA_FALSE;

   if (!pd->loop) return;
   EINA_INLIST_FOREACH(pd->loop_data->message_queue, msg)
     {
        n++;
        if (msg->message != message) continue;
        found = EINA_TRUE;
        msg->message = NULL;
        msg->handler = NULL;
        _efl_loop_message_send_info_set(message, NULL, NULL, NULL);
        if ((pd->loop_data->message_walking == 0) || (n == 1))
          {
             pd->loop_data->message_queue =
               eina_inlist_remove(pd->loop_data->message_queue,
                                  EINA_INLIST_GET(msg));
             free(msg);
          }
        else
          msg->delete_me = EINA_TRUE;
        break;
     }
   efl_event_callback_call(message, EFL_LOOP_MESSAGE_EVENT_MESSAGE,
                           message);
   efl_event_callback_call(obj, EFL_LOOP_MESSAGE_HANDLER_EVENT_MESSAGE,
                           message);
   // XXX: implement message object cache...
   if (message) efl_del(message);
   if (found) return;
   ERR("Cannot find message called object %p on message queue", message);
}

/**
 * @brief Clears all pending messages associated with this handler.
 *
 * This function iterates through the message queue and removes all messages
 * that were sent by this specific handler. Messages are marked for deletion
 * and either freed immediately or during a later cleanup phase if the
 * loop is currently iterating messages (`message_walking > 0`).
 *
 * @param obj The Efl_Loop_Message_Handler object.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 * @return EINA_FALSE if there was no loop or no messages to clear,
 *         otherwise EINA_FALSE (the return value seems to be consistently EINA_FALSE,
 *         perhaps indicating it doesn't signify success/failure of clearing itself
 *         but rather a state).
 */
EOLIAN static Eina_Bool
_efl_loop_message_handler_message_clear(Eo *obj, Efl_Loop_Message_Handler_Data *pd)
{
   Eina_Inlist *tmp;
   Message *msg;

   if (!pd->loop) return EINA_FALSE;
   if (!pd->loop_data->message_queue) return EINA_FALSE;
   EINA_INLIST_FOREACH_SAFE(pd->loop_data->message_queue, tmp, msg)
     {
        if (msg->handler == obj)
          {
             Eo *message = msg->message;

             msg->delete_me = EINA_TRUE;
             msg->handler = NULL;
             msg->message = NULL;
             _efl_loop_message_send_info_set(message, NULL, NULL, NULL);
             if (pd->loop_data->message_walking == 0)
               {
                  pd->loop_data->message_queue =
                    eina_inlist_remove(pd->loop_data->message_queue,
                                       EINA_INLIST_GET(msg));
                  free(msg);
               }
             if (message) efl_del(message);
          }
     }
   return EINA_FALSE;
}

/**
 * @brief Constructor for the Efl_Loop_Message_Handler object.
 *
 * Standard EFL object constructor.
 *
 * @param obj The Efl_Loop_Message_Handler object being constructed.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 * @return The constructed Efl_Loop_Message_Handler object.
 */
EOLIAN static Efl_Object *
_efl_loop_message_handler_efl_object_constructor(Eo *obj, Efl_Loop_Message_Handler_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @brief Destructor for the Efl_Loop_Message_Handler object.
 *
 * Standard EFL object destructor. It's important that message_clear
 * has been called or that messages are otherwise handled to prevent leaks,
 * though this destructor itself doesn't explicitly clear messages.
 *
 * @param obj The Efl_Loop_Message_Handler object being destructed.
 * @param pd The private data of the Efl_Loop_Message_Handler object.
 */
EOLIAN static void
_efl_loop_message_handler_efl_object_destructor(Eo *obj, Efl_Loop_Message_Handler_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "efl_loop_message_handler.eo.c"
