#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_LOOP_MESSAGE_FUTURE_HANDLER_CLASS

/**
 * @brief Private data structure for Efl_Loop_Message_Future_Handler.
 *
 * This structure holds any private data specific to instances of
 * Efl_Loop_Message_Future_Handler. Currently, it only contains a dummy
 * data field, which might be used for future extensions or specific
 * instance data.
 */
typedef struct _Efl_Loop_Message_Future_Handler_Data Efl_Loop_Message_Future_Handler_Data;

struct _Efl_Loop_Message_Future_Handler_Data
{
   void *data; /**< Placeholder for instance-specific data. Currently unused. */
};

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Adds a new message future type.
 *
 * This function creates and returns a new Efl_Loop_Message_Future object,
 * which is associated with the current handler object. This future object
 * can then be used to wait for a specific message.
 *
 * @param obj The Efl_Loop_Message_Future_Handler object.
 * @param pd Private data for the Efl_Loop_Message_Future_Handler object (unused).
 * @return A new Efl_Loop_Message_Future object, or NULL on failure.
 */
EOLIAN static Efl_Loop_Message_Future *
_efl_loop_message_future_handler_message_type_add(Eo *obj, Efl_Loop_Message_Future_Handler_Data *pd EINA_UNUSED)
{
   // XXX: implemented event obj cache
   return efl_add(EFL_LOOP_MESSAGE_FUTURE_CLASS, obj);
}

/**
 * @brief Constructor for Efl_Loop_Message_Future_Handler.
 *
 * Initializes a new Efl_Loop_Message_Future_Handler object. This function
 * calls the constructor of the parent class.
 *
 * @param obj The Efl_Loop_Message_Future_Handler object to construct.
 * @param pd Private data for the Efl_Loop_Message_Future_Handler object (unused).
 * @return The constructed Efl_Object, or NULL on failure.
 */
EOLIAN static Efl_Object *
_efl_loop_message_future_handler_efl_object_constructor(Eo *obj, Efl_Loop_Message_Future_Handler_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

/**
 * @brief Destructor for Efl_Loop_Message_Future_Handler.
 *
 * Cleans up resources used by the Efl_Loop_Message_Future_Handler object.
 * This function calls the destructor of the parent class.
 *
 * @param obj The Efl_Loop_Message_Future_Handler object to destruct.
 * @param pd Private data for the Efl_Loop_Message_Future_Handler object (unused).
 */
EOLIAN static void
_efl_loop_message_future_handler_efl_object_destructor(Eo *obj, Efl_Loop_Message_Future_Handler_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Handles an incoming message.
 *
 * This function is called when a message is dispatched to this handler.
 * It triggers the EFL_LOOP_MESSAGE_FUTURE_HANDLER_EVENT_MESSAGE_FUTURE event,
 * passing the received message as event data. It then calls the parent class's
 * message_call implementation.
 *
 * @param obj The Efl_Loop_Message_Future_Handler object.
 * @param pd Private data for the Efl_Loop_Message_Future_Handler object (unused).
 * @param message The Efl_Loop_Message object that was received.
 */
EOLIAN static void
_efl_loop_message_future_handler_efl_loop_message_handler_message_call(Eo *obj, Efl_Loop_Message_Future_Handler_Data *pd EINA_UNUSED, Efl_Loop_Message *message)
{
   efl_event_callback_call
     (obj, EFL_LOOP_MESSAGE_FUTURE_HANDLER_EVENT_MESSAGE_FUTURE, message);
   efl_loop_message_handler_message_call
     (efl_super(obj, MY_CLASS), message);
}

//////////////////////////////////////////////////////////////////////////

#include "efl_loop_message_future_handler.eo.c"
