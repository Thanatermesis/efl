#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eldbus_model_arguments_private.h"
#include "eldbus_model_private.h"

#include <Ecore.h>
#include <Eina.h>
#include <Eldbus.h>

#define MY_CLASS ELDBUS_MODEL_ARGUMENTS_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Arguments"

static void _eldbus_model_arguments_properties_load(Eldbus_Model_Arguments_Data *);
static void _eldbus_model_arguments_unload(Eldbus_Model_Arguments_Data *);
static Eina_Bool _eldbus_model_arguments_is_input_argument(Eldbus_Model_Arguments_Data *, const char *);
static Eina_Bool _eldbus_model_arguments_is_output_argument(Eldbus_Model_Arguments_Data *, const char *);
static Eina_Bool _eldbus_model_arguments_property_set(Eldbus_Model_Arguments_Data *, Eina_Value *, const char *); // TODO: This is a local helper, the public API is efl_model_property_set
static unsigned int _eldbus_model_arguments_argument_index_get(Eldbus_Model_Arguments_Data *, const char *);

/**
 * @brief Frees an Eina_Value stored in the properties hash.
 *
 * This function is used as a callback for eina_hash_string_superfast_new
 * to automatically free Eina_Value instances when they are removed from the hash.
 *
 * @param value The Eina_Value to be freed.
 */
static void
_eldbus_model_arguments_hash_free(Eina_Value *value)
{
   eina_value_free(value);
}

/**
 * @brief Constructor for the Eldbus_Model_Arguments object.
 *
 * Initializes the internal data structure, including the properties hash table.
 *
 * @param obj The Eo object being constructed.
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @return The constructed Eo object, or NULL on failure.
 */
static Efl_Object*
_eldbus_model_arguments_efl_object_constructor(Eo *obj, Eldbus_Model_Arguments_Data *pd)
{
   pd->obj = obj;
   // We do keep strings here as some of our API are looking for arg%u as a key instead of just indexes.
   pd->properties = eina_hash_string_superfast_new(EINA_FREE_CB(_eldbus_model_arguments_hash_free));
   pd->pending_list = NULL;
   pd->proxy = NULL;
   pd->arguments = NULL;
   pd->name = NULL;

   return efl_constructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Callback function invoked when the associated Eldbus_Proxy is freed.
 *
 * This function nullifies the proxy pointer in the private data to prevent
 * use-after-free errors.
 *
 * @param data The private data (Eldbus_Model_Arguments_Data *) of the arguments model.
 * @param deadptr The pointer to the Eldbus_Proxy object that is being freed.
 */
static void
_cleanup_proxy_cb(void *data, const void *deadptr)
{
   Eldbus_Model_Arguments_Data *pd = data;

   if (pd->proxy == deadptr) pd->proxy = NULL;
}

/**
 * @brief Finalizes the Eldbus_Model_Arguments object.
 *
 * Sets up the connection if not already set and registers a cleanup callback
 * for the associated Eldbus_Proxy.
 *
 * @param obj The Eo object being finalized.
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @return The finalized Eo object, or NULL if the proxy is not set.
 */
static Efl_Object *
_eldbus_model_arguments_efl_object_finalize(Eo *obj, Eldbus_Model_Arguments_Data *pd)
{
   if (!pd->proxy) return NULL;
   if (!eldbus_model_connection_get(obj))
     eldbus_model_connection_set(obj,
                                 eldbus_object_connection_get(eldbus_proxy_object_get(pd->proxy)));

   eldbus_proxy_free_cb_add(pd->proxy, _cleanup_proxy_cb, pd);

   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Custom constructor for initializing Eldbus_Model_Arguments with specific proxy, name, and arguments.
 *
 * This function is called to set up the model with the necessary D-Bus introspection data.
 *
 * @param obj The Eo object (unused in this function).
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param proxy The Eldbus_Proxy associated with these arguments.
 * @param name The name of the method or signal these arguments belong to.
 * @param arguments A list of Eldbus_Introspection_Argument structures describing the arguments.
 */
static void
_eldbus_model_arguments_custom_constructor(Eo *obj EINA_UNUSED,
                                    Eldbus_Model_Arguments_Data *pd,
                                    Eldbus_Proxy *proxy,
                                    const char *name,
                                    const Eina_List *arguments)
{
   EINA_SAFETY_ON_NULL_RETURN(proxy);
   EINA_SAFETY_ON_NULL_RETURN(name);

   pd->proxy = eldbus_proxy_ref(proxy);
   pd->arguments = arguments;
   pd->name = eina_stringshare_add(name);
}

/**
 * @brief Invalidates the Eldbus_Model_Arguments object, cleaning up resources.
 *
 * This function unloads argument data, frees the properties hash,
 * unreferences the proxy, and performs other cleanup tasks.
 *
 * @param obj The Eo object being invalidated.
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 */
static void
_eldbus_model_arguments_efl_object_invalidate(Eo *obj, Eldbus_Model_Arguments_Data *pd)
{
   _eldbus_model_arguments_unload(pd);

   eina_hash_free(pd->properties);

   eina_stringshare_del(pd->name);
   if (pd->proxy)
     {
        eldbus_proxy_free_cb_del(pd->proxy, _cleanup_proxy_cb, pd);
        eldbus_proxy_unref(pd->proxy);
        pd->proxy = NULL;
     }

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets an iterator for the properties (arguments) of the model.
 *
 * The properties are named "arg0", "arg1", etc.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @return An Eina_Iterator over the property names (strings), or NULL on failure.
 *         The caller is responsible for freeing the iterator.
 */
static Eina_Iterator *
_eldbus_model_arguments_efl_model_properties_get(const Eo *obj EINA_UNUSED,
                                                 Eldbus_Model_Arguments_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);

   _eldbus_model_arguments_properties_load(pd);
   return eina_hash_iterator_key_new(pd->properties);
}

/**
 * @brief Loads argument properties into the internal hash table if not already loaded.
 *
 * Iterates over the list of Eldbus_Introspection_Argument structures and populates
 * the `pd->properties` hash. Each property is named "arg%u" (e.g., "arg0", "arg1")
 * and its value is an Eina_Value of the appropriate D-Bus type.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 */
static void
_eldbus_model_arguments_properties_load(Eldbus_Model_Arguments_Data *pd)
{
   unsigned int arguments_count;
   unsigned int i;

   if (eina_hash_population(pd->properties) > 0)
     return ;

   arguments_count = eina_list_count(pd->arguments);

   for (i = 0; i < arguments_count; ++i)
     {
        Eldbus_Introspection_Argument *arg;
        const Eina_Value_Type *type;
        Eina_Slstr *name;
        Eina_Value *value;

        name = eina_slstr_printf(ARGUMENT_FORMAT, i);
        if (!name) continue;

        arg = eina_list_nth(pd->arguments, i);
        type = _dbus_type_to_eina_value_type(arg->type[0]);
        value = eina_value_new(type);
        eina_hash_add(pd->properties, name, value);
     }
}

/**
 * @brief Sets the value of an input argument property.
 *
 * This function allows setting the value of an argument that is an input
 * to a D-Bus method call.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param property The name of the property to set (e.g., "arg0").
 * @param value The Eina_Value to set for the property.
 * @return An Eina_Future that resolves with a copy of the set value on success,
 *         or rejects with an Efl_Model_Error on failure.
 *         Possible errors:
 *         - EFL_MODEL_ERROR_NOT_FOUND: If the property does not exist or is not an input argument.
 *         - EFL_MODEL_ERROR_READ_ONLY: If the property is not an input argument (e.g., an output argument).
 *         - EFL_MODEL_ERROR_INCORRECT_VALUE: If `property` or `value` is NULL.
 */
static Eina_Future *
_eldbus_model_arguments_efl_model_property_set(Eo *obj,
                                               Eldbus_Model_Arguments_Data *pd,
                                               const char *property, Eina_Value *value)
{
   Eina_Value *prop_value;
   Eina_Error err = 0;
   Eina_Bool ret;

   DBG("(%p): property=%s", obj, property);

   err = EFL_MODEL_ERROR_NOT_FOUND;
   if (!property || !value) goto on_error;

   _eldbus_model_arguments_properties_load(pd);

   err = EFL_MODEL_ERROR_READ_ONLY;
   ret = _eldbus_model_arguments_is_input_argument(pd, property);
   if (!ret) goto on_error;

   err = EFL_MODEL_ERROR_NOT_FOUND;
   prop_value = eina_hash_find(pd->properties, property);
   if (!prop_value) goto on_error;

   eina_value_flush(prop_value);
   eina_value_copy(value, prop_value);

   return efl_loop_future_resolved(obj,
                               eina_value_reference_copy(value));

 on_error:
   return efl_loop_future_rejected(obj, err);
}

/**
 * @brief Gets the value of an output argument property.
 *
 * This function retrieves the value of an argument that is an output
 * from a D-Bus method call or signal.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param property The name of the property to get (e.g., "arg0").
 * @return A new Eina_Value containing the property's value on success.
 *         The caller is responsible for freeing this value.
 *         Returns an Eina_Value of type EINA_VALUE_TYPE_ERROR on failure.
 *         Possible errors:
 *         - EFL_MODEL_ERROR_NOT_FOUND: If the property does not exist.
 *         - EFL_MODEL_ERROR_PERMISSION_DENIED: If the property is not an output argument.
 *         - EFL_MODEL_ERROR_INCORRECT_VALUE: If `property` is NULL.
 */
static Eina_Value *
_eldbus_model_arguments_efl_model_property_get(const Eo *obj, Eldbus_Model_Arguments_Data *pd, const char *property)
{
   Eina_Value *value;
   Eina_Bool ret;

   DBG("(%p): property=%s", obj, property);
   if (!property) return eina_value_error_new(EFL_MODEL_ERROR_INCORRECT_VALUE);

   _eldbus_model_arguments_properties_load(pd);

   value = eina_hash_find(pd->properties, property);
   if (!value) return eina_value_error_new(EFL_MODEL_ERROR_NOT_FOUND);

   ret = _eldbus_model_arguments_is_output_argument(pd, property);
   if (!ret) return eina_value_error_new(EFL_MODEL_ERROR_PERMISSION_DENIED);

   return eina_value_dup(value);
}

/**
 * @brief Gets the name of the D-Bus method or signal these arguments belong to.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @return A stringshared pointer to the name.
 */
static const char *
_eldbus_model_arguments_arg_name_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Arguments_Data *pd)
{
   return pd->name;
}

/**
 * @brief Unloads argument data, cancelling pending operations and clearing properties.
 *
 * This function is called during invalidation or when a D-Bus operation
 * needs to be reset. It cancels all pending D-Bus calls associated with this
 * model and clears the cached property values.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 */
static void
_eldbus_model_arguments_unload(Eldbus_Model_Arguments_Data *pd)
{
   Eldbus_Pending *pending;

   EINA_SAFETY_ON_NULL_RETURN(pd);

   EINA_LIST_FREE(pd->pending_list, pending)
     eldbus_pending_cancel(pending);

   eina_hash_free_buckets(pd->properties);
}

/**
 * @brief Processes arguments received from a D-Bus message (typically a method reply or signal).
 *
 * This function is called when a D-Bus message arrives that contains argument values.
 * It updates the model's properties with these new values and notifies listeners
 * about the changes.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param msg The Eldbus_Message containing the argument data.
 * @param pending The Eldbus_Pending object associated with the D-Bus call, if any.
 *                This is used to remove it from the list of pending operations.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., D-Bus error in message,
 *         failed to parse arguments).
 */
Eina_Bool
eldbus_model_arguments_process_arguments(Eldbus_Model_Arguments_Data *pd,
                                         const Eldbus_Message *msg,
                                         Eldbus_Pending *pending)
{
   const Eldbus_Introspection_Argument *argument;
   const char *error_name, *error_text;
   const Eina_List *it;
   Eina_Value *value_struct;
   Eina_Array *changed_properties;
   unsigned int i = 0;
   Eina_Stringshare *property;
   Eina_Bool result = EINA_FALSE;

   _eldbus_model_arguments_properties_load(pd);

   pd->pending_list = eina_list_remove(pd->pending_list, pending);
   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        //efl_model_error_notify(pd->obj);
        return EINA_FALSE;
     }

   value_struct = eldbus_message_to_eina_value(msg);
   if (value_struct == NULL)
     {
        INF("%s", "No output arguments");
        return EINA_TRUE;
     }

   changed_properties = eina_array_new(1);

   EINA_LIST_FOREACH(pd->arguments, it, argument)
     {
        // Process only output arguments (or arguments with unspecified direction, treated as output)
        if (ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_IN != argument->direction)
          {
             Eina_Bool ret;

             property = eina_stringshare_printf(ARGUMENT_FORMAT, i);

             // Add property name to the list of changed properties for the event
             // Example: changed_properties might become ["arg0", "arg1"]
             ret = eina_array_push(changed_properties, property);
             EINA_SAFETY_ON_FALSE_GOTO(ret, on_error);

             // Set the actual property value in the model
             ret = _eldbus_model_arguments_property_set(pd, value_struct, property);
             EINA_SAFETY_ON_FALSE_GOTO(ret, on_error);
          }

        ++i;
     }

   // If any properties were changed, emit the EFL_MODEL_EVENT_PROPERTIES_CHANGED event
   // The event data `evt.changed_properties` is an Eina_Array of Eina_Stringshare*
   // indicating which properties (e.g., "arg0", "arg1") have changed.
   if (eina_array_count(changed_properties))
     {
        Efl_Model_Property_Event evt = {.changed_properties = changed_properties};
        efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &evt);
     }

   result = EINA_TRUE;

on_error:
   // Cleanup in case of error
   while ((property = eina_array_pop(changed_properties)))
     eina_stringshare_del(property);
   eina_array_free(changed_properties);
   eina_value_free(value_struct);

   return result;
}

/**
 * @brief Internal helper to set a property value from a D-Bus message structure.
 *
 * This function extracts the relevant value for a given property (e.g., "arg0")
 * from the `value_struct` (which represents the entire set of arguments from
 * the D-Bus message) and updates the corresponding property in `pd->properties`.
 *
 * @note This function assumes `value_struct` contains a D-Bus struct where the
 *       first member (named "arg0" in the Eina_Value struct representation)
 *       corresponds to the value for the property being set. This might need
 *       adjustment if D-Bus messages can return multiple output arguments
 *       not wrapped in a single struct.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param value_struct An Eina_Value of type struct, holding the D-Bus message arguments.
 *                     Example: If D-Bus returns (s,i) "hello", 123
 *                     value_struct would be a struct with fields "arg0" (string "hello")
 *                     and "arg1" (int32 123). This function currently expects to find
 *                     the relevant value under the key "arg0" within this struct.
 * @param property The name of the property to set (e.g., "arg0").
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_eldbus_model_arguments_property_set(Eldbus_Model_Arguments_Data *pd,
                                     Eina_Value *value_struct,
                                     const char *property)
{
   Eina_Value *prop_value;
   Eina_Value value;
   Eina_Bool ret;

   _eldbus_model_arguments_properties_load(pd);

   prop_value = eina_hash_find(pd->properties, property);
   EINA_SAFETY_ON_NULL_RETURN_VAL(prop_value, EINA_FALSE);

   ret = eina_value_struct_value_get(value_struct, "arg0", &value);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ret, EINA_FALSE);

   eina_value_flush(prop_value);
   ret = eina_value_copy(&value, prop_value);
   eina_value_flush(&value);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ret, EINA_FALSE);

   return ret;
}

/**
 * @brief Checks if a given argument has a specific D-Bus direction (in, out, or none).
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param argument The name of the argument (e.g., "arg0").
 * @param direction The Eldbus_Introspection_Argument_Direction to check against.
 * @return EINA_TRUE if the argument has the specified direction, EINA_FALSE otherwise
 *         or if the argument is not found.
 */
static Eina_Bool
_eldbus_model_arguments_is(Eldbus_Model_Arguments_Data *pd,
                           const char *argument,
                           Eldbus_Introspection_Argument_Direction direction)
{
   Eldbus_Introspection_Argument *argument_introspection;
   unsigned int i;

   _eldbus_model_arguments_properties_load(pd);

   i = _eldbus_model_arguments_argument_index_get(pd, argument);
   if ((i > 0x7fffffff) || ((int)i >= eina_hash_population(pd->properties)))
     {
        WRN("Argument not found: %s", argument);
        return false;
     }

   argument_introspection = eina_list_nth(pd->arguments, i);
   EINA_SAFETY_ON_NULL_RETURN_VAL(argument_introspection, EINA_FALSE);

   return argument_introspection->direction == direction;
}

/**
 * @brief Checks if a given argument is an input argument.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param argument The name of the argument (e.g., "arg0").
 * @return EINA_TRUE if the argument is an input argument, EINA_FALSE otherwise.
 */
static Eina_Bool
_eldbus_model_arguments_is_input_argument(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   return _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_IN);
}

/**
 * @brief Checks if a given argument is an output argument.
 *
 * Arguments with direction ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_NONE are also
 * considered output arguments for the purpose of property getting.
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance.
 * @param argument The name of the argument (e.g., "arg0").
 * @return EINA_TRUE if the argument is an output argument (or direction "none"), EINA_FALSE otherwise.
 */
static Eina_Bool
_eldbus_model_arguments_is_output_argument(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   return _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_OUT) ||
     _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_NONE);
}

/**
 * @brief Gets the numerical index of an argument from its name (e.g., "arg0" -> 0).
 *
 * @param pd The private data for the Eldbus_Model_Arguments instance (used to get total count as fallback).
 * @param argument The name of the argument (e.g., "arg0", "arg1").
 * @return The numerical index of the argument if successfully parsed.
 *         If parsing fails (e.g., `argument` is not in "arg%u" format),
 *         it returns a value equal to `eina_hash_population(pd->properties)`,
 *         which typically indicates an invalid or out-of-bounds index.
 */
static unsigned int
_eldbus_model_arguments_argument_index_get(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   unsigned int i = 0;

   // Tries to parse "arg%u" from the argument string.
   // For example, if argument is "arg5", i will be 5.
   if (sscanf(argument, ARGUMENT_FORMAT, &i) > 0)
     return i;
   // Fallback: if sscanf fails, return a value that will likely be out of bounds.
   return eina_hash_population(pd->properties);
}

#include "eldbus_model_arguments.eo.c"
