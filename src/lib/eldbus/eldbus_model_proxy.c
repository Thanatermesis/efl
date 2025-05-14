#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>

#include "eldbus_model_proxy_private.h"
#include "eldbus_model_private.h"

#define MY_CLASS ELDBUS_MODEL_PROXY_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Proxy"

static void _eldbus_model_proxy_property_get_all_cb(void *, const Eldbus_Message *, Eldbus_Pending *);
static void _eldbus_model_proxy_property_set_cb(void *, const Eldbus_Message *, Eldbus_Pending *);
static void _eldbus_model_proxy_property_set_load_cb(void *, const Eldbus_Message *, Eldbus_Pending *);
static void _eldbus_model_proxy_start_monitor(Eldbus_Model_Proxy_Data *);
static void _eldbus_model_proxy_property_changed_cb(void *, Eldbus_Proxy *, void *);
static void _eldbus_model_proxy_property_invalidated_cb(void *, Eldbus_Proxy *, void *);
static const char *_eldbus_model_proxy_property_type_get(Eldbus_Model_Proxy_Data *, const char *);
static void _eldbus_model_proxy_create_methods_children(Eldbus_Model_Proxy_Data *);
static void _eldbus_model_proxy_create_signals_children(Eldbus_Model_Proxy_Data *);


typedef struct _Eldbus_Model_Proxy_Property_Set_Data Eldbus_Model_Proxy_Property_Set_Data;
typedef struct _Eldbus_Property_Promise Eldbus_Property_Promise;

/**
 * @brief Structure to hold data for a property set operation.
 * This is used to pass context through asynchronous D-Bus calls.
 */
struct _Eldbus_Model_Proxy_Property_Set_Data
{
   Eldbus_Model_Proxy_Data *pd; /**< Pointer to the proxy private data. */

   Eina_Stringshare *property; /**< The name of the property being set. */
   Eina_Promise *promise; /**< The promise associated with the set operation. */
   Eina_Value *value; /**< The value to set the property to. */
};

/**
 * @brief Structure to associate a promise with a property name.
 * Used when multiple properties might be involved in an operation, like GetAl.
 */
struct _Eldbus_Property_Promise
{
  Eina_Promise *promise; /**< The promise to be resolved or rejected. */
  Eina_Stringshare *property; /**< The name of the property this promise is for. Can be NULL if the promise is for a "GetAll" operation. */
};

/**
 * @brief Loads the D-Bus proxy and initializes its properties.
 * If the proxy is already loaded, this function does nothing.
 * It populates the internal properties hash table based on introspection data.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_eldbus_model_proxy_load(Eldbus_Model_Proxy_Data *pd)
{
   Eldbus_Introspection_Property *property;
   Eina_List *it;

   if (pd->proxy)
     return EINA_TRUE;

   pd->proxy = eldbus_proxy_get(pd->object, pd->name);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->proxy, EINA_FALSE);

   EINA_LIST_FOREACH(pd->interface->properties, it, property)
     {
        const Eina_Value_Type *type;
        Eina_Stringshare *name;
        Eina_Value *value;

        type = _dbus_type_to_eina_value_type(property->type[0]);
        name = eina_stringshare_add(property->name);
        value = eina_value_new(type);

        eina_hash_direct_add(pd->properties, name, value);
     }

   return EINA_TRUE;
}

/**
 * @brief Unloads the D-Bus proxy and cleans up associated resources.
 * Cancels any pending D-Bus operations and stops monitoring for property changes.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_unload(Eldbus_Model_Proxy_Data *pd)
{
   Eldbus_Pending *pending;

   EINA_LIST_FREE(pd->pendings, pending)
     eldbus_pending_cancel(pending);

   if (pd->monitoring)
     {
        eldbus_proxy_event_callback_del(pd->proxy,
                                        ELDBUS_PROXY_EVENT_PROPERTY_CHANGED,
                                        _eldbus_model_proxy_property_changed_cb,
                                        pd);
        eldbus_proxy_event_callback_del(pd->proxy,
                                        ELDBUS_PROXY_EVENT_PROPERTY_REMOVED,
                                        _eldbus_model_proxy_property_invalidated_cb,
                                        pd);
     }
   pd->monitoring = EINA_FALSE;

   if (pd->proxy) eldbus_proxy_unref(pd->proxy);
   pd->proxy = NULL;
}

/**
 * @brief Callback invoked when the associated Eldbus_Object is deleted.
 * Sets the internal object pointer to NULL to prevent use-after-free.
 *
 * @param data The Eldbus_Model_Proxy_Data.
 * @param object The Eldbus_Object that was deleted (unused).
 * @param event_info Event specific information (unused).
 */
static void
_eldbus_model_proxy_object_del(void *data, Eldbus_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eldbus_Model_Proxy_Data *pd = data;

   pd->object = NULL;
}

/**
 * @brief EFL object constructor for Eldbus_Model_Proxy.
 * Initializes basic fields in the private data structure.
 *
 * @param obj The Eo object being constructed.
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return The constructed Eo object.
 */
static Efl_Object*
_eldbus_model_proxy_efl_object_constructor(Eo *obj, Eldbus_Model_Proxy_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;
   pd->properties = eina_hash_stringshared_new(NULL);

   return obj;
}

/**
 * @brief EFL object finalizer for Eldbus_Model_Proxy.
 * Ensures that the necessary D-Bus object, name, and interface are set.
 * Loads the proxy and sets up monitoring for object deletion.
 *
 * @param obj The Eo object being finalized.
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return The finalized Eo object, or NULL on failure.
 */
static Efl_Object*
_eldbus_model_proxy_efl_object_finalize(Eo *obj, Eldbus_Model_Proxy_Data *pd)
{
   if (!pd->object ||
       !pd->name ||
       !pd->interface)
     return NULL;

   if (!_eldbus_model_proxy_load(pd)) return NULL;

   if (!eldbus_model_connection_get(obj))
     eldbus_model_connection_set(obj, eldbus_object_connection_get(pd->object));

   eldbus_object_event_callback_add(pd->object, ELDBUS_OBJECT_EVENT_DEL, _eldbus_model_proxy_object_del, pd);

   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the D-Bus object for this proxy model.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param object The Eldbus_Object to associate with this proxy.
 */
static void
_eldbus_model_proxy_object_set(Eo *obj EINA_UNUSED,
                               Eldbus_Model_Proxy_Data *pd,
                               Eldbus_Object *object)
{
   pd->object = eldbus_object_ref(object);
}

/**
 * @brief Sets the D-Bus interface introspection data for this proxy model.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param interface The Eldbus_Introspection_Interface to use.
 */
static void
_eldbus_model_proxy_interface_set(Eo *obj EINA_UNUSED,
                                  Eldbus_Model_Proxy_Data *pd,
                                  const Eldbus_Introspection_Interface *interface)
{
   pd->name = eina_stringshare_add(interface->name);
   pd->interface = interface;
}

/**
 * @brief EFL object invalidation handler for Eldbus_Model_Proxy.
 * Cleans up resources, unloads the proxy, and removes event callbacks.
 *
 * @param obj The Eo object being invalidated.
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_efl_object_invalidate(Eo *obj, Eldbus_Model_Proxy_Data *pd)
{
   Eo *child;

   EINA_LIST_FREE(pd->childrens, child)
     efl_unref(child);

   _eldbus_model_proxy_unload(pd);

   if (pd->object)
     {
        eldbus_object_event_callback_del(pd->object, ELDBUS_OBJECT_EVENT_DEL, _eldbus_model_proxy_object_del, pd);
        eldbus_object_unref(pd->object);
     }

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @brief EFL object destructor for Eldbus_Model_Proxy.
 * Frees all allocated resources, including the properties hash and interface name.
 *
 * @param obj The Eo object being destructed.
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_efl_object_destructor(Eo *obj, Eldbus_Model_Proxy_Data *pd)
{
   Eina_Hash_Tuple *tuple;
   Eina_Iterator *it;

   it = eina_hash_iterator_tuple_new(pd->properties);
   EINA_ITERATOR_FOREACH(it, tuple)
     {
        Eina_Stringshare *property = tuple->key;
        Eina_Value *value = tuple->data;

        eina_stringshare_del(property);
        eina_value_free(value);
     }
   eina_iterator_free(it);
   eina_hash_free(pd->properties);

   eina_stringshare_del(pd->name);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets an iterator over the names of the properties of this D-Bus proxy.
 * Implements Efl.Model.properties_get.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return An Eina_Iterator over property names (Eina_Stringshare *).
 *         The caller is responsible for freeing the iterator.
 */
static Eina_Iterator *
_eldbus_model_proxy_efl_model_properties_get(const Eo *obj EINA_UNUSED,
                                             Eldbus_Model_Proxy_Data *pd)
{
   return eina_hash_iterator_key_new(pd->properties);
}

#define PROPERTY_EXIST 1 /**< Flag indicating the property exists. */
#define PROPERTY_READ  2 /**< Flag indicating the property is readable. */
#define PROPERTY_WRITE 4 /**< Flag indicating the property is writable. */

/**
 * @brief Checks the existence and access rights of a D-Bus property.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param property The name of the property to check.
 * @return An unsigned char with flags indicating existence (PROPERTY_EXIST),
 *         read access (PROPERTY_READ), and write access (PROPERTY_WRITE).
 *         Returns 0 if the property is not found.
 */
static unsigned char
eldbus_model_proxy_property_check(Eldbus_Model_Proxy_Data *pd,
                                  const char *property)
{
    Eldbus_Introspection_Property *property_introspection =
      eldbus_introspection_property_find(pd->interface->properties, property);
    unsigned char r = 0;

    if (property_introspection == NULL)
       {
          WRN("Property not found: %s", property);
          return 0;
       }

    r = PROPERTY_EXIST;
    // Check read access
    if (property_introspection->access == ELDBUS_INTROSPECTION_PROPERTY_ACCESS_READ ||
        property_introspection->access == ELDBUS_INTROSPECTION_PROPERTY_ACCESS_READWRITE)
      r |= PROPERTY_READ;
    // Check write access
    if (property_introspection->access == ELDBUS_INTROSPECTION_PROPERTY_ACCESS_WRITE ||
        property_introspection->access == ELDBUS_INTROSPECTION_PROPERTY_ACCESS_READWRITE)
      r |= PROPERTY_WRITE;

    return r;
}

/**
 * @brief Callback for when a property set future is cancelled.
 * Cleans up the associated Eldbus_Model_Proxy_Property_Set_Data.
 *
 * @param consumer The Efl_Loop_Consumer (unused).
 * @param data The Eldbus_Model_Proxy_Property_Set_Data associated with the cancelled operation.
 * @param dead_future The Eina_Future that was cancelled (unused).
 */
static void
_eldbus_model_proxy_cancel_cb(Efl_Loop_Consumer *consumer EINA_UNUSED,
                              void *data,
                              const Eina_Future *dead_future EINA_UNUSED)
{
   Eldbus_Model_Proxy_Property_Set_Data *sd = data;

   sd->promise = NULL;
   eina_stringshare_del(sd->property);
   eina_value_free(sd->value);
   free(sd);
}

/**
 * @brief Initiates a D-Bus "GetAll" properties operation if not already pending.
 * This function is used when a property get or set is requested and the local cache
 * (pd->is_loaded) is not yet populated. It queues the original promise and property name.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param promise The Eina_Promise associated with the original property get/set request.
 *                This promise will be resolved/rejected after "GetAll" completes and
 *                the specific property operation is attempted. Can be NULL for a simple load.
 * @param property The name of the property that triggered this load. Can be NULL.
 * @param callback The callback function to invoke when the "GetAll" operation completes.
 * @param data User data for the callback.
 * @return The Eldbus_Pending object for the "GetAll" operation if initiated,
 *         otherwise NULL (e.g., if "GetAll" is already pending or memory allocation fails).
 */
static Eldbus_Pending *
_eldbus_model_proxy_load_all(Eldbus_Model_Proxy_Data *pd,
                             Eina_Promise *promise, const char *property,
                             Eldbus_Message_Cb callback,
                             void *data)
{
   Eldbus_Property_Promise *p;
   Eldbus_Pending *pending = NULL;

   p = calloc(1, sizeof(Eldbus_Property_Promise));
   if (!p)
     {
        if (promise) eina_promise_reject(promise, ENOMEM);
        return NULL;
     }

   p->promise = promise;
   p->property = eina_stringshare_add(property);
   pd->promises = eina_list_append(pd->promises, p);

   // Only initiate a new GetAll if there isn't one already pending.
   // pd->pendings stores all active D-Bus requests. If a GetAll is already
   // in flight, its completion will handle any queued promises.
   if (!pd->pendings) // This logic might be too simple if pd->pendings can contain non-GetAll pendings.
                     // However, given the context, it seems GetAll is the primary "pending" state for loading.
     {
        pending = eldbus_proxy_property_get_all(pd->proxy, callback, data);
     }
   return pending;
}

/**
 * @brief Sets the value of a D-Bus property.
 * Implements Efl.Model.property_set.
 * If properties are not yet loaded, it first triggers a "GetAll" operation.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set the property to.
 * @return An Eina_Future that resolves with the (potentially updated) Eina_Value of the property
 *         on success, or rejects with an Eina_Error on failure.
 *         Possible errors:
 *         - EFL_MODEL_ERROR_NOT_FOUND: Property does not exist.
 *         - EFL_MODEL_ERROR_READ_ONLY: Property is not writable.
 *         - EFL_MODEL_ERROR_UNKNOWN: D-Bus error or type mismatch.
 *         - ENOMEM: Memory allocation failure.
 */
static Eina_Future *
_eldbus_model_proxy_efl_model_property_set(Eo *obj EINA_UNUSED,
                                           Eldbus_Model_Proxy_Data *pd,
                                           const char *property,
                                           Eina_Value *value)
{
   Eldbus_Model_Proxy_Property_Set_Data *data = NULL;
   const char *signature;
   Eldbus_Pending *pending;
   unsigned char access;
   Eina_Error err = 0;

   DBG("(%p): property=%s", obj, property);

   access = eldbus_model_proxy_property_check(pd, property);
   err = EFL_MODEL_ERROR_NOT_FOUND;
   if (!access) goto on_error;
   err = EFL_MODEL_ERROR_READ_ONLY;
   if (!(access & PROPERTY_WRITE)) goto on_error;

   err = EFL_MODEL_ERROR_UNKNOWN;
   signature = _eldbus_model_proxy_property_type_get(pd, property);
   if (!signature) goto on_error;

   err = ENOMEM;
   data = calloc(1, sizeof (Eldbus_Model_Proxy_Property_Set_Data));
   if (!data) goto on_error;

   data->pd = pd;
   data->promise = efl_loop_promise_new(obj);
   data->property = eina_stringshare_add(property);
   if (!(data->value = eina_value_dup(value))) goto on_error;

   if (!pd->is_loaded)
     {
        pending = _eldbus_model_proxy_load_all(pd, data->promise, property,
                                               _eldbus_model_proxy_property_set_load_cb, data);
     }
   else
     {
        pending = eldbus_proxy_property_value_set(pd->proxy, property, signature, (Eina_Value*)value,
                                                  _eldbus_model_proxy_property_set_cb, data);
     }

   if (pending) pd->pendings = eina_list_append(pd->pendings, pending);
   return efl_future_then(obj, eina_future_new(data->promise),
                          .data = data, .free = _eldbus_model_proxy_cancel_cb);

 on_error:
   free(data);
   return efl_loop_future_rejected(obj, err);
}

/**
 * @brief Gets the value of a D-Bus property.
 * Implements Efl.Model.property_get.
 * If properties are loaded, it returns the cached value. Otherwise, it initiates
 * a "GetAll" operation and returns an Eina_Value of type ERROR with code EAGAIN,
 * indicating the operation is in progress and the caller should try again later
 * (typically by listening to PROPERTIES_CHANGED event).
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value on success.
 *         If properties are not loaded, returns an Eina_Value of type ERROR with code EAGAIN.
 *         On other errors (e.g., property not found, not readable), returns an Eina_Value
 *         of type ERROR with a relevant Eina_Error code (e.g., EFL_MODEL_ERROR_NOT_FOUND).
 *         The caller owns the returned Eina_Value and must free it.
 */
static Eina_Value *
_eldbus_model_proxy_efl_model_property_get(const Eo *obj EINA_UNUSED,
                                           Eldbus_Model_Proxy_Data *pd,
                                           const char *property)
{
   Eldbus_Pending *pending;
   unsigned char access;
   Eina_Error err = 0;

   access = eldbus_model_proxy_property_check(pd, property);
   err = EFL_MODEL_ERROR_NOT_FOUND;
   if (!access) goto on_error;
   if (!(access & PROPERTY_READ)) goto on_error;

   if (pd->is_loaded)
     {
        Eina_Stringshare *tmp;
        Eina_Value *value;

        err = EFL_MODEL_ERROR_NOT_FOUND;
        tmp = eina_stringshare_add(property);
        value = eina_hash_find(pd->properties, tmp);
        eina_stringshare_del(tmp);
        if (!value) goto on_error;

        return eina_value_dup(value);
     }

   err = ENOMEM;

   pending = _eldbus_model_proxy_load_all(pd, NULL, property,
                                          _eldbus_model_proxy_property_get_all_cb, pd);
   if (pending) pd->pendings = eina_list_append(pd->pendings, pending);
   else goto on_error;

   return eina_value_error_new(EAGAIN);

 on_error:
   return eina_value_error_new(err);
}

/**
 * @brief Ensures that children (methods and signals) of the proxy are created and listed.
 * This function is called when children information is first requested.
 * It populates the pd->childrens list and emits EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_listed(Eldbus_Model_Proxy_Data *pd)
{
   if (!pd->is_listed)
     {
        _eldbus_model_proxy_create_methods_children(pd);
        _eldbus_model_proxy_create_signals_children(pd);

        efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);
        pd->is_listed = EINA_TRUE;
     }
}

/**
 * @brief Gets a slice of children (methods and signals) of this D-Bus proxy.
 * Implements Efl.Model.children_slice_get.
 * The children are Eldbus_Model_Method or Eldbus_Model_Signal objects.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param start The starting index of the slice.
 * @param count The number of children to retrieve.
 * @return An Eina_Future that resolves with an Eina_Value array containing the children.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_ARRAY, with subtype
 *         EFL_MODEL_TYPE_OBJECT. Each object in the array is an Eo pointer
 *         to a child model (Eldbus_Model_Method or Eldbus_Model_Signal).
 *         Example of resolved Eina_Value structure:
 *         Eina_Value (type=EINA_VALUE_TYPE_ARRAY, subtype=EFL_MODEL_TYPE_OBJECT)
 *           -> [ Eo* (child1), Eo* (child2), ... ]
 */
static Eina_Future*
_eldbus_model_proxy_efl_model_children_slice_get(Eo *obj EINA_UNUSED,
                                                 Eldbus_Model_Proxy_Data *pd,
                                                 unsigned start,
                                                 unsigned count)
{
   Eina_Value v;

   _eldbus_model_proxy_listed(pd);

   v = efl_model_list_value_get(pd->childrens, start, count);
   return efl_loop_future_resolved(obj, v);
}

/**
 * @brief Gets the total count of children (methods and signals) of this D-Bus proxy.
 * Implements Efl.Model.children_count_get.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return The total number of children.
 */
static unsigned int
_eldbus_model_proxy_efl_model_children_count_get(const Eo *obj EINA_UNUSED,
                                                 Eldbus_Model_Proxy_Data *pd)
{
   _eldbus_model_proxy_listed(pd);
   return eina_list_count(pd->childrens);
}

/**
 * @brief Creates child models for each method in the D-Bus interface.
 * Adds Eldbus_Model_Method objects to the pd->childrens list.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_create_methods_children(Eldbus_Model_Proxy_Data *pd)
{
   Eldbus_Introspection_Method *method;
   Eina_List *it;

   EINA_LIST_FOREACH(pd->interface->methods, it, method)
     {
        const char *bus;
        const char *path;
        const char *interface_name;
        const char *method_name;
        Eo *child;

        bus = eldbus_object_bus_name_get(pd->object);
        if (!bus) continue;

        path = eldbus_object_path_get(pd->object);
        if (!path) continue;

        interface_name = pd->interface->name;
        if (!interface_name) continue;

        method_name = method->name;
        if (!method_name) continue;

        INF("(%p) Creating method child: bus = %s, path = %s, method = %s::%s",
                       pd->obj, bus, path, interface_name, method_name);

        child = efl_add_ref(ELDBUS_MODEL_METHOD_CLASS, pd->obj,
                            eldbus_model_method_proxy_set(efl_added, pd->proxy),
                            eldbus_model_method_set(efl_added, method));

        if (child) pd->childrens = eina_list_append(pd->childrens, child);
        else ERR("Could not create method child: bus = %s, path = %s method = %s::%s.",
                 bus, path, interface_name, method_name);
     }
}

/**
 * @brief Creates child models for each signal in the D-Bus interface.
 * Adds Eldbus_Model_Signal objects to the pd->childrens list.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_create_signals_children(Eldbus_Model_Proxy_Data *pd)
{
   Eina_List *it;
   Eldbus_Introspection_Signal *signal;

   EINA_LIST_FOREACH(pd->interface->signals, it, signal)
     {
        const char *bus;
        const char *path;
        const char *interface_name;
        const char *signal_name;
        Eo *child;

        bus = eldbus_object_bus_name_get(pd->object);
        if (!bus) continue;

        path = eldbus_object_path_get(pd->object);
        if (!path) continue;

        interface_name = pd->interface->name;
        if (!interface_name) continue;

        signal_name = signal->name;
        if (!signal_name) continue;

        DBG("(%p) Creating signal child: bus = %s, path = %s, signal = %s::%s",
                       pd->obj, bus, path, interface_name, signal_name);

        child = efl_add_ref(ELDBUS_MODEL_SIGNAL_CLASS, pd->obj, eldbus_model_signal_constructor(efl_added, pd->proxy, signal));

        if (child) pd->childrens = eina_list_append(pd->childrens, child);
        else ERR("Could not create signal child: bus = %s, path = %s signal = %s::%s.",
                 bus, path, interface_name, signal_name);
     }
}

/**
 * @brief Gets the D-Bus interface name this proxy represents.
 *
 * @param obj The Eldbus_Model_Proxy object (unused).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return The D-Bus interface name (e.g., "org.freedesktop.DBus.Properties").
 */
static const char *
_eldbus_model_proxy_proxy_name_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Proxy_Data *pd)
{
   return pd->name;
}

/**
 * @brief Starts monitoring D-Bus property change and removal events.
 * If monitoring is already active, this function does nothing.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 */
static void
_eldbus_model_proxy_start_monitor(Eldbus_Model_Proxy_Data *pd)
{
   if (pd->monitoring)
     return;

   pd->monitoring = EINA_TRUE;

   eldbus_proxy_event_callback_add(pd->proxy,
                                   ELDBUS_PROXY_EVENT_PROPERTY_CHANGED,
                                   _eldbus_model_proxy_property_changed_cb,
                                   pd);

   eldbus_proxy_event_callback_add(pd->proxy,
                                   ELDBUS_PROXY_EVENT_PROPERTY_REMOVED,
                                   _eldbus_model_proxy_property_invalidated_cb,
                                   pd);
}

/**
 * @brief Callback for D-Bus ELDBUS_PROXY_EVENT_PROPERTY_CHANGED events.
 * Updates the local cache of the property and emits efl_model_properties_changed.
 *
 * @param data The Eldbus_Model_Proxy_Data.
 * @param proxy The Eldbus_Proxy that emitted the event (unused).
 * @param event_info The Eldbus_Object_Event_Property_Changed data containing the
 *                   property name and its new value.
 */
static void
_eldbus_model_proxy_property_changed_cb(void *data,
                                        Eldbus_Proxy *proxy EINA_UNUSED,
                                        void *event_info)
{
   Eldbus_Model_Proxy_Data *pd = (Eldbus_Model_Proxy_Data*)data;
   Eldbus_Object_Event_Property_Changed *event = (Eldbus_Object_Event_Property_Changed*)event_info;
   Eina_Value *prop_value;
   Eina_Bool ret;

   prop_value = eina_hash_find(pd->properties, event->name);
   if (!prop_value) return ; // Property not tracked by this model proxy

   ret = eina_value_copy(event->value, prop_value);
   if (!ret) return ; // Failed to copy value

   efl_model_properties_changed(pd->obj, event->name);
}

/**
 * @brief Callback for D-Bus ELDBUS_PROXY_EVENT_PROPERTY_REMOVED events.
 * Emits efl_model_property_invalidated_notify. Note that D-Bus properties
 * are typically defined by introspection and don't get "removed" dynamically
 * in the same way as object manager interfaces. This might be for completeness
 * or specific D-Bus services.
 *
 * @param data The Eldbus_Model_Proxy_Data.
 * @param proxy The Eldbus_Proxy that emitted the event (unused).
 * @param event_info The Eldbus_Proxy_Event_Property_Changed data (note: this seems to be
 *                   reused struct name from eldbus, typically contains property name).
 */
static void
_eldbus_model_proxy_property_invalidated_cb(void *data,
                                            Eldbus_Proxy *proxy EINA_UNUSED,
                                            void *event_info)
{
   Eldbus_Model_Proxy_Data *pd = (Eldbus_Model_Proxy_Data*)data;
   Eldbus_Proxy_Event_Property_Changed *event = (Eldbus_Proxy_Event_Property_Changed*)event_info;

   efl_model_property_invalidated_notify(pd->obj, event->name);
}

/**
 * @brief Processes the reply from a D-Bus "GetAll" properties call.
 * Updates the local property cache (pd->properties) with the received values.
 *
 * @param msg The Eldbus_Message containing the reply from "GetAll".
 *            Expected arguments: "a{sv}" (an array of string-variant dictionaries).
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @return An Eina_Array of Eina_Stringshare* for property names that were successfully
 *         loaded and updated. The caller is responsible for freeing the array and its contents.
 *         Returns NULL on error (e.g., D-Bus error, message format mismatch).
 *         Example of "a{sv}" structure in D-Bus message:
 *         [ {"PropertyName1", Variant(Value1)}, {"PropertyName2", Variant(Value2)}, ... ]
 *         The returned Eina_Array would contain:
 *         [ "PropertyName1", "PropertyName2", ... ] (as Eina_Stringshare*)
 */
static Eina_Array *
_eldbus_model_proxy_property_get_all_load(const Eldbus_Message *msg, Eldbus_Model_Proxy_Data *pd)
{
   Eldbus_Message_Iter *values = NULL;
   Eldbus_Message_Iter *entry;
   Eina_Array *changed_properties;
   Eina_Stringshare *tmp = NULL;
   const char *error_name, *error_text;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        return NULL;
     }

   if (!eldbus_message_arguments_get(msg, "a{sv}", &values))
     {
        ERR("%s", "Error getting arguments.");
        return NULL;
     }

   changed_properties = eina_array_new(1);
   while (eldbus_message_iter_get_and_next(values, 'e', &entry))
     {
        const char *property;
        Eldbus_Message_Iter *variant;
        Eina_Value *struct_value;
        Eina_Value *prop_value;
        Eina_Value arg0;
        Eina_Bool ret;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &property, &variant))
          continue;

        struct_value = eldbus_message_iter_struct_like_to_eina_value(variant);
        if (!struct_value) goto on_error;

        ret = eina_value_struct_value_get(struct_value, "arg0", &arg0);
        eina_value_free(struct_value);
        if (!ret) goto on_error;

        tmp = eina_stringshare_add(property);
        prop_value = eina_hash_find(pd->properties, tmp);
        if (!prop_value) goto on_error;

        ret = eina_value_copy(&arg0, prop_value);
        if (!ret) goto on_error;

        eina_value_flush(&arg0);

        ret = eina_array_push(changed_properties, tmp);
        if (!ret) goto on_error;

        // Reset tmp to NULL to avoid double free.
        tmp = NULL;
     }

   pd->is_loaded = EINA_TRUE;
   return changed_properties;

 on_error:
   eina_stringshare_del(tmp);
   while ((tmp = eina_array_pop(changed_properties)))
     eina_stringshare_del(tmp);
   eina_array_free(changed_properties);
   return NULL;
}

/**
 * @brief Cleans up an Eldbus_Property_Promise structure.
 * Rejects the associated promise if it exists and frees allocated memory.
 *
 * @param p The Eldbus_Property_Promise to clean.
 * @param err The Eina_Error code to reject the promise with.
 */
static void
_eldbus_model_proxy_promise_clean(Eldbus_Property_Promise* p,
                                  Eina_Error err)
{
   if (p->promise) eina_promise_reject(p->promise, err);
   eina_stringshare_del(p->property);
   free(p);
}

/**
 * @brief Callback for the completion of a D-Bus "GetAll" properties operation.
 * This is typically used when a property get was requested before properties were loaded.
 * It processes the loaded properties, resolves/rejects any queued promises related to
 * specific property gets (though current logic seems to reject them with READ_ONLY
 * regardless of the actual property requested, which might need review),
 * starts property monitoring, and emits EFL_MODEL_EVENT_PROPERTIES_CHANGED.
 *
 * @param data The Eldbus_Model_Proxy_Data.
 * @param msg The Eldbus_Message reply from "GetAll".
 * @param pending The Eldbus_Pending object for the "GetAll" operation.
 */
static void
_eldbus_model_proxy_property_get_all_cb(void *data,
                                        const Eldbus_Message *msg,
                                        Eldbus_Pending *pending)
{
   Eldbus_Model_Proxy_Data *pd = (Eldbus_Model_Proxy_Data*)data;
   Eldbus_Property_Promise* p;
   Eina_Stringshare *sp;
   Eina_Array *properties;
   Efl_Model_Property_Event evt;

   pd->pendings = eina_list_remove(pd->pendings, pending);

   properties = _eldbus_model_proxy_property_get_all_load(msg, pd);
   if (!properties)
     {
        // If GetAll failed, reject all pending promises for properties.
        EINA_LIST_FREE(pd->promises, p)
          _eldbus_model_proxy_promise_clean(p, EFL_MODEL_ERROR_NOT_FOUND); // Or a more generic D-Bus error
        return ;
     }

   // If GetAll succeeded, the original promises were for individual property reads.
   // However, GetAll itself doesn't return individual values for promises.
   // The current logic rejects these promises with READ_ONLY. This might be a placeholder
   // or assumes that if a specific get was deferred to GetAll, it's treated as a bulk read.
   // A more refined approach might try to find the specific property from 'properties'
   // and resolve its promise, or simply let the PROPERTIES_CHANGED event signal data availability.
   EINA_LIST_FREE(pd->promises, p)
     _eldbus_model_proxy_promise_clean(p, EFL_MODEL_ERROR_READ_ONLY); // This seems odd, should it resolve?

   _eldbus_model_proxy_start_monitor(pd);

   evt.changed_properties = properties;
   efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &evt);

   // Clean up the list of property names returned by _eldbus_model_proxy_property_get_all_load
   while ((sp = eina_array_pop(properties)))
     eina_stringshare_del(sp);
   eina_array_free(properties);
}


/**
 * @brief Callback for "GetAll" properties, specifically when triggered by a property set operation.
 * After properties are loaded via "GetAll", this function proceeds to issue the original
 * property set D-Bus call.
 *
 * @param data The Eldbus_Model_Proxy_Property_Set_Data containing information about the
 *             property set operation that was deferred.
 * @param msg The Eldbus_Message reply from "GetAll".
 * @param pending The Eldbus_Pending object for the "GetAll" operation.
 */
static void
_eldbus_model_proxy_property_set_load_cb(void *data,
                                         const Eldbus_Message *msg,
                                         Eldbus_Pending *pending)
{
   Eldbus_Model_Proxy_Property_Set_Data *set_data = (Eldbus_Model_Proxy_Property_Set_Data *)data;
   Eldbus_Model_Proxy_Data *pd = set_data->pd;
   Eina_Array *properties; // List of properties loaded by GetAll
   Eina_Stringshare *sp;
   const char *signature;

   pd->pendings = eina_list_remove(pd->pendings, pending);

   signature = _eldbus_model_proxy_property_type_get(pd, set_data->property);

   properties = _eldbus_model_proxy_property_get_all_load(msg, pd);
   if (!signature || !properties) // If signature is NULL (property not found after load) or GetAll failed
     {
        // Reject the original promise for the set operation
        eina_promise_reject(set_data->promise, EFL_MODEL_ERROR_UNKNOWN);
        goto end;
     }

   // Now that properties are loaded (pd->is_loaded is true), make the actual set call.
   pending = eldbus_proxy_property_value_set(pd->proxy, set_data->property,
                                             signature, set_data->value,
                                             _eldbus_model_proxy_property_set_cb, set_data);
   if (pending) pd->pendings = eina_list_append(pd->pendings, pending);
   // If eldbus_proxy_property_value_set fails to return a pending (e.g. immediate error),
   // the promise (set_data->promise) will be rejected in _eldbus_model_proxy_property_set_cb
   // if that callback is invoked with an error, or it might hang if the callback is never called.
   // Consider rejecting here if pending is NULL and no callback will fire.

end:
   // Clean up the list of property names from _eldbus_model_proxy_property_get_all_load
   if (!properties) return;
   while ((sp = eina_array_pop(properties)))
     eina_stringshare_del(sp);
   eina_array_free(properties);
}


/**
 * @brief Callback for the completion of a D-Bus property set operation.
 * Updates the local property cache, emits efl_model_properties_changed,
 * and resolves or rejects the promise associated with the set operation.
 *
 * @param data The Eldbus_Model_Proxy_Property_Set_Data.
 * @param msg The Eldbus_Message reply from the property set call.
 * @param pending The Eldbus_Pending object for the property set operation.
 */
static void
_eldbus_model_proxy_property_set_cb(void *data,
                                    const Eldbus_Message *msg,
                                    Eldbus_Pending *pending)
{
   Eldbus_Model_Proxy_Property_Set_Data *sd = (Eldbus_Model_Proxy_Property_Set_Data *)data;
   Eldbus_Model_Proxy_Data *pd = sd->pd;
   const char *error_name, *error_text;
   Eina_Value *value; // This is the cached value, not the one from msg.

   pd->pendings = eina_list_remove(pd->pendings, pending);

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
         ERR("%s: %s", error_name, error_text);
         if (sd->promise) eina_promise_reject(sd->promise, EFL_MODEL_ERROR_UNKNOWN); // Or map D-Bus error
         return;
     }

   // Assuming the set was successful on the bus, update our local cache
   // Note: The D-Bus SetProperty method usually doesn't return the new value.
   // We assume the value passed in sd->value is now the current value.
   // It's crucial that sd->value was correctly copied/referenced.
   value = eina_hash_find(pd->properties, sd->property);
   if (value)
     {
        // Before notifying, update the cached value.
        // The original sd->value is what we attempted to set.
        // We should copy sd->value into the cache (pd->properties[sd->property]).
        // This step seems to be missing. If the property changed event from the bus
        // is relied upon to update the cache, there might be a race or if events are off.
        // However, eldbus_proxy_property_value_set should have updated the proxy's internal cache
        // if it's a standard property, which then _eldbus_model_proxy_property_changed_cb handles.
        // For now, we assume the cache will be updated by a subsequent property_changed event.
        // Or, if the set implies success, we should update our model's value directly here.
        // Let's assume for now that a changed event will fire and update the cache.
        // If not, `eina_value_copy(sd->value, value);` would be needed here.

        efl_model_properties_changed(pd->obj, sd->property); // Notify that it *might* have changed.
        if (sd->promise)
          {
             // Resolve with a *copy* of the value we believe is now set.
             // This should ideally be the value from our cache after it's confirmed.
             Eina_Value *current_value = eina_hash_find(pd->properties, sd->property);
             eina_promise_resolve(sd->promise, eina_value_reference_copy(current_value ? current_value : sd->value));
          }
     }
   else
     {
        // This case (property not found in our cache after a successful set) should ideally not happen
        // if the property existed and was writable.
        if (sd->promise)
          eina_promise_reject(sd->promise, EFL_MODEL_ERROR_NOT_FOUND);
     }
}

/**
 * @brief Retrieves the D-Bus signature string for a given property.
 *
 * @param pd The private data of the Eldbus_Model_Proxy.
 * @param property The name of the property.
 * @return The D-Bus type signature string (e.g., "s" for string, "i" for int32)
 *         if the property is found, otherwise NULL.
 *         The returned string is owned by the introspection data and should not be freed.
 */
static const char *
_eldbus_model_proxy_property_type_get(Eldbus_Model_Proxy_Data *pd,
                                      const char *property)
{
   Eldbus_Introspection_Property *property_introspection =
     eldbus_introspection_property_find(pd->interface->properties, property);

   if (property_introspection == NULL)
     {
        WRN("Property not found: %s", property);
        return NULL;
     }

   return property_introspection->type;
}

#include "eldbus_model_proxy.eo.c"
