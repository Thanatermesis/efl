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

#define MY_CLASS EFL_LOOP_MESSAGE_CLASS

#include "ecore_main_common.h"

/**
 * @internal
 * @brief Private data for Efl_Loop_Message objects.
 *
 * This structure holds data specific to an instance of Efl_Loop_Message,
 * managing its state and relationship with the Ecore loop.
 */
typedef struct _Efl_Loop_Message_Data Efl_Loop_Message_Data;

struct _Efl_Loop_Message_Data
{
   Eina_Inlist   *send_list_node; /**< Node in the loop's message queue, representing this message. */
   Eo            *loop;           /**< The Efl_Loop instance this message is associated with. */
   Efl_Loop_Data *loop_data;      /**< Private data of the associated Efl_Loop. */
};

/////////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Sets information about the message when it's sent.
 *
 * This function is called internally when a message is added to the loop's
 * message queue. It stores references to the message's node in the queue,
 * the loop it belongs to, and the loop's private data.
 *
 * @param obj The Efl_Loop_Message object.
 * @param node The Eina_Inlist node representing the message in the queue.
 * @param loop The Efl_Loop object this message is sent to.
 * @param loop_data The private data of the Efl_Loop.
 */
void
_efl_loop_message_send_info_set(Eo *obj, Eina_Inlist *node, Eo *loop, Efl_Loop_Data *loop_data)
{
   Efl_Loop_Message_Data *pd = efl_data_scope_get(obj, MY_CLASS);
   if (!pd) return;
   pd->send_list_node = node;
   pd->loop = loop;
   pd->loop_data = loop_data;
}

/**
 * @internal
 * @brief Marks a message to be unsent (removed from the queue).
 *
 * This function flags a message for deletion. The actual removal might be
 * deferred, for example, if the message queue is currently being processed.
 * It effectively cancels a pending message.
 *
 * @param obj The Efl_Loop_Message object to unsend.
 */
void
_efl_loop_message_unsend(Eo *obj)
{
   Efl_Loop_Message_Data *pd = efl_data_scope_get(obj, MY_CLASS);
   if (!pd) return;
   if ((!pd->send_list_node) || (!pd->loop)) return;

   Message *msg = (Message *)pd->send_list_node;
   msg->delete_me = EINA_TRUE;
}

/////////////////////////////////////////////////////////////////////////////

/**
 * @internal
 * @brief Constructor for Efl_Loop_Message.
 *
 * Initializes a new Efl_Loop_Message object.
 *
 * @param obj The Efl_Loop_Message object to construct.
 * @param pd Private data for the Efl_Loop_Message object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_loop_message_efl_object_constructor(Eo *obj, Efl_Loop_Message_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @internal
 * @brief Destructor for Efl_Loop_Message.
 *
 * Cleans up resources associated with an Efl_Loop_Message object.
 * This includes marking the message for deletion from the loop's queue
 * and freeing associated data if it's safe to do so.
 *
 * @param obj The Efl_Loop_Message object to destruct.
 * @param pd Private data of the Efl_Loop_Message object.
 */
EOLIAN static void
_efl_loop_message_efl_object_destructor(Eo *obj, Efl_Loop_Message_Data *pd)
{
   if ((pd->send_list_node) && (pd->loop_data))
     {
        Message *msg = (Message *)pd->send_list_node;

        msg->delete_me = EINA_TRUE;
        msg->message = NULL;
        msg->handler = NULL;
        if (pd->loop_data->message_walking == 0)
          {
             pd->loop_data->message_queue =
               eina_inlist_remove(pd->loop_data->message_queue,
                                  pd->send_list_node);
          }
        pd->send_list_node = NULL;
        pd->loop = NULL;
        pd->loop_data = NULL;
        free(pd->send_list_node);
     }
   efl_destructor(efl_super(obj, MY_CLASS));
}

#include "efl_loop_message.eo.c"
