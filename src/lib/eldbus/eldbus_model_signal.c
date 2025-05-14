#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eldbus_model_arguments_private.h"
#include "eldbus_model_signal_private.h"
#include "eldbus_model_private.h"

#include <Ecore.h>
#include <Eina.h>

#define MY_CLASS ELDBUS_MODEL_SIGNAL_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Signal"

static void _eldbus_model_signal_handler_cb(void *, const Eldbus_Message *);
static void _eldbus_model_signal_callback_add(Eldbus_Model_Signal_Data *);
static void _eldbus_model_signal_callback_del(Eldbus_Model_Signal_Data *);

/**
 * @internal
 * @brief EFL object constructor for Eldbus_Model_Signal.
 *
 * Initializes the Eldbus_Model_Signal_Data structure and calls the parent
 * class constructor.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Eldbus_Model_Signal instance.
 * @return The constructed Eo object.
 */
static Efl_Object*
_eldbus_model_signal_efl_object_constructor(Eo *obj, Eldbus_Model_Signal_Data *pd)
{
   efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;
   pd->handler = NULL;
   pd->signal = NULL;
   return obj;
}

/**
 * @internal
 * @brief Constructor for the signal model aspects.
 *
 * Initializes the signal-specific parts of the Eldbus_Model_Signal object.
 * It sets up the arguments based on the introspection data and registers
 * the signal handler.
 *
 * @param obj The Eo object (unused in this function but part of EFL constructor signature).
 * @param pd The private data for the Eldbus_Model_Signal instance.
 * @param proxy The Eldbus_Proxy to which this signal belongs.
 * @param signal The introspection data for the D-Bus signal.
 */
static void
_eldbus_model_signal_signal_constructor(Eo *obj EINA_UNUSED,
                                 Eldbus_Model_Signal_Data *pd,
                                 Eldbus_Proxy *proxy,
                                 const Eldbus_Introspection_Signal *signal)
{
   EINA_SAFETY_ON_NULL_RETURN(proxy);
   EINA_SAFETY_ON_NULL_RETURN(signal);

   eldbus_model_arguments_custom_constructor(efl_super(obj, MY_CLASS), proxy, signal->name, signal->arguments);

   pd->signal = signal;
   _eldbus_model_signal_callback_add(pd);
}

/**
 * @internal
 * @brief EFL object invalidation function for Eldbus_Model_Signal.
 *
 * Cleans up resources, specifically by removing the D-Bus signal handler,
 * before calling the parent class invalidation.
 *
 * @param obj The Eo object to invalidate.
 * @param pd The private data for the Eldbus_Model_Signal instance.
 */
static void
_eldbus_model_signal_efl_object_invalidate(Eo *obj, Eldbus_Model_Signal_Data *pd)
{
   _eldbus_model_signal_callback_del(pd);

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Adds a D-Bus signal handler for the configured signal.
 *
 * Registers a callback function (_eldbus_model_signal_handler_cb) to be
 * invoked when the D-Bus signal is received on the associated proxy.
 *
 * @param pd The private data for the Eldbus_Model_Signal instance,
 *           containing proxy and signal information.
 */
static void
_eldbus_model_signal_callback_add(Eldbus_Model_Signal_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN(pd);
   EINA_SAFETY_ON_FALSE_RETURN(NULL == pd->handler);

   Eldbus_Model_Arguments_Data *args_data = efl_data_scope_get(pd->obj, ELDBUS_MODEL_ARGUMENTS_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(args_data);

   pd->handler = eldbus_proxy_signal_handler_add(args_data->proxy, pd->signal->name, _eldbus_model_signal_handler_cb, pd);
}

/**
 * @internal
 * @brief Removes the D-Bus signal handler.
 *
 * Unregisters the previously added signal handler. This is typically called
 * during object invalidation or when the signal monitoring is no longer needed.
 *
 * @param pd The private data for the Eldbus_Model_Signal instance,
 *           which holds the reference to the Eldbus_Signal_Handler.
 */
static void
_eldbus_model_signal_callback_del(Eldbus_Model_Signal_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (pd->handler)
     {
        eldbus_signal_handler_unref(pd->handler);
        pd->handler = NULL;
     }
}

/**
 * @internal
 * @brief Callback executed when a D-Bus signal is received.
 *
 * This function is invoked by the Eldbus library when a matching D-Bus
 * signal arrives. It retrieves the arguments from the message and processes them.
 *
 * @param data User data provided when the handler was added (points to Eldbus_Model_Signal_Data).
 * @param msg The received Eldbus_Message containing the signal data.
 */
static void
_eldbus_model_signal_handler_cb(void *data, const Eldbus_Message *msg)
{
   Eldbus_Model_Signal_Data *pd = (Eldbus_Model_Signal_Data*)data;

   Eldbus_Model_Arguments_Data *args_data = efl_data_scope_get(pd->obj, ELDBUS_MODEL_ARGUMENTS_CLASS);

   eldbus_model_arguments_process_arguments(args_data, msg, NULL);
}

#include "eldbus_model_signal.eo.c"
