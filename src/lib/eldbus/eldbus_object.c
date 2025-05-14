#include "eldbus_private.h"
#include "eldbus_private_types.h"
#include <dbus/dbus.h>

/* TODO: mempool of Eldbus_Object, Eldbus_Object_Context_Event_Cb and
 * Eldbus_Object_Context_Event
 */

#define ELDBUS_OBJECT_CHECK(obj)                        \
  do                                                   \
    {                                                  \
       EINA_SAFETY_ON_NULL_RETURN(obj);                \
       if (!EINA_MAGIC_CHECK(obj, ELDBUS_OBJECT_MAGIC)) \
         {                                             \
            EINA_MAGIC_FAIL(obj, ELDBUS_OBJECT_MAGIC);  \
            return;                                    \
         }                                             \
       EINA_SAFETY_ON_TRUE_RETURN(obj->refcount <= 0); \
    }                                                  \
  while (0)

#define ELDBUS_OBJECT_CHECK_RETVAL(obj, retval)                     \
  do                                                               \
    {                                                              \
       EINA_SAFETY_ON_NULL_RETURN_VAL(obj, retval);                \
       if (!EINA_MAGIC_CHECK(obj, ELDBUS_OBJECT_MAGIC))             \
         {                                                         \
            EINA_MAGIC_FAIL(obj, ELDBUS_OBJECT_MAGIC);              \
            return retval;                                         \
         }                                                         \
       EINA_SAFETY_ON_TRUE_RETURN_VAL(obj->refcount <= 0, retval); \
    }                                                              \
  while (0)

#define ELDBUS_OBJECT_CHECK_GOTO(obj, label)                 \
  do                                                        \
    {                                                       \
       EINA_SAFETY_ON_NULL_GOTO(obj, label);                \
       if (!EINA_MAGIC_CHECK(obj, ELDBUS_OBJECT_MAGIC))      \
         {                                                  \
            EINA_MAGIC_FAIL(obj, ELDBUS_OBJECT_MAGIC);       \
            goto label;                                     \
         }                                                  \
       EINA_SAFETY_ON_TRUE_GOTO(obj->refcount <= 0, label); \
    }                                                       \
  while (0)

Eina_Bool
eldbus_object_init(void)
{
   return EINA_TRUE;
}

void
eldbus_object_shutdown(void)
{
}

/**
 * @internal
 * @brief Calls all registered event callbacks for a given event type.
 *
 * This function iterates over the list of registered callbacks for the
 * specified event type and invokes each one. It handles a walking counter
 * to allow safe deletion of callbacks from within a callback.
 *
 * @param obj The Eldbus_Object emitting the event.
 * @param type The type of event that occurred.
 * @param event_info A pointer to event-specific data.
 */
static void _eldbus_object_event_callback_call(Eldbus_Object *obj, Eldbus_Object_Event_Type type, const void *event_info);

/**
 * @internal
 * @brief Deletes an object context event callback.
 *
 * Removes the given context callback @p ctx from the event context @p ce
 * and frees the memory associated with @p ctx.
 *
 * @param ce The object context event from which to remove the callback.
 * @param ctx The object context event callback to delete.
 */
static void _eldbus_object_context_event_cb_del(Eldbus_Object_Context_Event *ce, Eldbus_Object_Context_Event_Cb *ctx);

/**
 * @internal
 * @brief Callback executed when an Eldbus_Connection is freed.
 *
 * This function is registered with an Eldbus_Connection and is called when
 * the connection is about to be freed. It clears and frees the associated
 * Eldbus_Object.
 *
 * @param data The Eldbus_Object associated with the connection.
 * @param dead_pointer The Eldbus_Connection being freed (unused).
 */
static void _on_connection_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Callback executed when an Eldbus_Signal_Handler is freed.
 *
 * This function is registered with an Eldbus_Signal_Handler and is called
 * when the signal handler is about to be freed. It removes the handler
 * from the object's list of signal handlers.
 *
 * @param data The Eldbus_Object that owns the signal handler.
 * @param dead_pointer The Eldbus_Signal_Handler being freed.
 */
static void _on_signal_handler_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Calls ELDBUS_OBJECT_EVENT_DEL callbacks and clears them.
 *
 * This function is called as part of the object destruction process.
 * It first triggers all ELDBUS_OBJECT_EVENT_DEL event callbacks.
 * Then, it removes all ELDBUS_OBJECT_EVENT_DEL callbacks to prevent
 * them from being called again during the final cleanup in
 * _eldbus_object_clear().
 *
 * @param obj The Eldbus_Object being deleted.
 */
static void
_eldbus_object_call_del(Eldbus_Object *obj)
{
   Eldbus_Object_Context_Event *ce;

   _eldbus_object_event_callback_call(obj, ELDBUS_OBJECT_EVENT_DEL, NULL);

   /* clear all del callbacks so we don't call them twice at
    * _eldbus_object_clear()
    */
   ce = obj->event_handlers + ELDBUS_OBJECT_EVENT_DEL;
   while (ce->list)
     {
        Eldbus_Object_Context_Event_Cb *ctx;

        ctx = EINA_INLIST_CONTAINER_GET(ce->list,
                                        Eldbus_Object_Context_Event_Cb);
        _eldbus_object_context_event_cb_del(ce, ctx);
     }
}

/**
 * @internal
 * @brief Clears resources associated with an Eldbus_Object.
 *
 * This function is responsible for cleaning up an Eldbus_Object before it is
 * freed. It performs the following actions:
 * - Calls _eldbus_object_call_del() to notify ELDBUS_OBJECT_EVENT_DEL listeners.
 * - Removes the object from its connection's name-to-object mapping.
 * - Deletes all owned signal handlers.
 * - Cancels all owned pending calls.
 * - Dispatches all registered free callbacks (cbs_free).
 * - Sets the refcount to 0.
 *
 * Note: obj->proxies are expected to be cleared via cbs_free mechanism
 * by Eldbus_Proxy.
 *
 * @param obj The Eldbus_Object to clear.
 */
static void
_eldbus_object_clear(Eldbus_Object *obj)
{
   Eldbus_Signal_Handler *h;
   Eldbus_Pending *p;
   Eina_List *iter, *iter_next;
   Eina_Inlist *in_l;
   DBG("obj=%p, refcount=%d, name=%s, path=%s",
       obj, obj->refcount, obj->name, obj->path);

   obj->refcount = 1;
   _eldbus_object_call_del(obj);
   eldbus_connection_name_object_del(obj->conn, obj);

   /* NOTE: obj->proxies is deleted from obj->cbs_free. */

   EINA_LIST_FOREACH_SAFE(obj->signal_handlers, iter, iter_next, h)
     {
        DBG("obj=%p delete owned signal handler %p %s",
            obj, h, eldbus_signal_handler_match_get(h));
        eldbus_signal_handler_del(h);
     }
   EINA_INLIST_FOREACH_SAFE(obj->pendings, in_l, p)
     {
        DBG("obj=%p delete owned pending call=%p dest=%s path=%s %s.%s()",
            obj, p,
            eldbus_pending_destination_get(p),
            eldbus_pending_path_get(p),
            eldbus_pending_interface_get(p),
            eldbus_pending_method_get(p));
        eldbus_pending_cancel(p);
     }

   eldbus_cbs_free_dispatch(&(obj->cbs_free), obj);
   obj->refcount = 0;
}

/**
 * @internal
 * @brief Frees the memory allocated for an Eldbus_Object.
 *
 * This function is the final step in destroying an Eldbus_Object. It should
 * only be called after _eldbus_object_clear() has been successfully executed
 * and the object's refcount is 0.
 * It performs the following actions:
 * - Frees the hash table of proxies if any remain (logs errors for alive proxies).
 * - Frees the list of signal handlers if any remain (logs errors for alive handlers).
 * - Logs a critical error if any pendings still exist.
 * - Frees all event handlers and their associated callback contexts.
 * - Deletes specific signal handlers for InterfacesAdded, InterfacesRemoved, and PropertiesChanged.
 * - Frees the stringshared name and path.
 * - Sets the magic to EINA_MAGIC_NONE.
 * - Frees the Eldbus_Object structure itself.
 *
 * @param obj The Eldbus_Object to free.
 */
static void
_eldbus_object_free(Eldbus_Object *obj)
{
   unsigned int i;
   Eldbus_Signal_Handler *h;

   if (obj->proxies)
     {
        Eina_Iterator *iterator = eina_hash_iterator_data_new(obj->proxies);
        Eldbus_Proxy *proxy;
        EINA_ITERATOR_FOREACH(iterator, proxy)
          ERR("obj=%p alive proxy=%p %s", obj, proxy,
              eldbus_proxy_interface_get(proxy));
        eina_iterator_free(iterator);
        eina_hash_free(obj->proxies);
     }

   EINA_LIST_FREE(obj->signal_handlers, h)
     {
        if (h->dangling)
          eldbus_signal_handler_free_cb_del(h, _on_signal_handler_free, obj);
        else
          ERR("obj=%p alive handler=%p %s", obj, h,
              eldbus_signal_handler_match_get(h));
     }

   if (obj->pendings)
     CRI("Object %p released with live pending calls!", obj);

   for (i = 0; i < ELDBUS_OBJECT_EVENT_LAST; i++)
     {
        Eldbus_Object_Context_Event *ce = obj->event_handlers + i;
        while (ce->list)
          {
             Eldbus_Object_Context_Event_Cb *ctx;

             ctx = EINA_INLIST_CONTAINER_GET(ce->list,
                                             Eldbus_Object_Context_Event_Cb);
             _eldbus_object_context_event_cb_del(ce, ctx);
          }
        eina_list_free(ce->to_delete);
     }

   if (obj->interfaces_added)
     eldbus_signal_handler_del(obj->interfaces_added);
   if (obj->interfaces_removed)
     eldbus_signal_handler_del(obj->interfaces_removed);
   if (obj->properties_changed)
     eldbus_signal_handler_del(obj->properties_changed);
   eina_stringshare_del(obj->name);
   eina_stringshare_del(obj->path);
   EINA_MAGIC_SET(obj, EINA_MAGIC_NONE);

   free(obj);
}

/*
 * For _on_connection_free Doxygen documentation, see its forward declaration
 * earlier in this file.
 */
static void
_on_connection_free(void *data, const void *dead_pointer EINA_UNUSED)
{
   Eldbus_Object *obj = data;
   ELDBUS_OBJECT_CHECK(obj);
   _eldbus_object_clear(obj);
   _eldbus_object_free(obj);
}

EAPI Eldbus_Object *
eldbus_object_get(Eldbus_Connection *conn, const char *bus, const char *path)
{
   Eldbus_Object *obj;

   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(bus, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   obj = eldbus_connection_name_object_get(conn, bus, path);
   if (obj)
     return eldbus_object_ref(obj);

   obj = calloc(1, sizeof(Eldbus_Object));
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);

   obj->conn = conn;
   obj->refcount = 1;
   obj->path = eina_stringshare_add(path);
   obj->name = eina_stringshare_add(bus);
   obj->proxies = eina_hash_string_small_new(NULL);
   EINA_SAFETY_ON_NULL_GOTO(obj->proxies, cleanup);
   EINA_MAGIC_SET(obj, ELDBUS_OBJECT_MAGIC);

   eldbus_connection_name_object_set(conn, obj);
   eldbus_connection_free_cb_add(obj->conn, _on_connection_free, obj);

   obj->properties = eldbus_proxy_get(obj, ELDBUS_FDO_INTERFACE_PROPERTIES);

   return obj;

cleanup:
   eina_stringshare_del(obj->path);
   eina_stringshare_del(obj->name);
   free(obj);

   return NULL;
}

/*
 * For _on_signal_handler_free Doxygen documentation, see its forward declaration
 * earlier in this file.
 */
static void _on_signal_handler_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Decrements the reference count of an Eldbus_Object and frees it if the count reaches zero.
 *
 * This is the internal implementation for unreferencing an object.
 * If the reference count drops to zero, it removes the connection free callback,
 * clears the object's resources, and then frees the object structure.
 *
 * @param obj The Eldbus_Object to unreference.
 */
static void
_eldbus_object_unref(Eldbus_Object *obj)
{
   obj->refcount--;
   if (obj->refcount > 0) return;

   eldbus_connection_free_cb_del(obj->conn, _on_connection_free, obj);
   _eldbus_object_clear(obj);
   _eldbus_object_free(obj);
}

EAPI Eldbus_Object *
eldbus_object_ref(Eldbus_Object *obj)
{
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   DBG("obj=%p, pre-refcount=%d, name=%s, path=%s",
       obj, obj->refcount, obj->name, obj->path);
   obj->refcount++;
   return obj;
}

EAPI void
eldbus_object_unref(Eldbus_Object *obj)
{
   ELDBUS_OBJECT_CHECK(obj);
   DBG("obj=%p, pre-refcount=%d, name=%s, path=%s",
       obj, obj->refcount, obj->name, obj->path);
   _eldbus_object_unref(obj);
}

EAPI void
eldbus_object_free_cb_add(Eldbus_Object *obj, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_OBJECT_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   obj->cbs_free = eldbus_cbs_free_add(obj->cbs_free, cb, data);
}

EAPI void
eldbus_object_free_cb_del(Eldbus_Object *obj, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_OBJECT_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   obj->cbs_free = eldbus_cbs_free_del(obj->cbs_free, cb, data);
}

/**
 * @internal
 * @brief Callback for the "InterfacesAdded" D-Bus signal from org.freedesktop.DBus.ObjectManager.
 *
 * This function is invoked when an "InterfacesAdded" signal is received for
 * the monitored object path. It parses the signal message, which contains the
 * object path and a dictionary of interfaces added along with their properties.
 * For each interface added, it retrieves the corresponding Eldbus_Proxy
 * and triggers the ELDBUS_OBJECT_EVENT_IFACE_ADDED event.
 *
 * @param data The Eldbus_Object associated with this signal handler.
 * @param msg The received Eldbus_Message containing the signal data.
 *        The message arguments are expected to be:
 *        - "o": object_path (string)
 *        - "a{sa{sv}}": interfaces_and_properties (array of dictionary entries)
 *          - Each dictionary entry:
 *            - key: interface_name (string)
 *            - value: properties (dictionary of string to variant)
 */
static void
_cb_interfaces_added(void *data, const Eldbus_Message *msg)
{
   Eldbus_Object *obj = data;
   const char *obj_path;
   Eldbus_Message_Iter *array_ifaces, *entry_iface;

   if (!eldbus_message_arguments_get(msg, "oa{sa{sv}}", &obj_path, &array_ifaces))
     return;

   while (eldbus_message_iter_get_and_next(array_ifaces, 'e', &entry_iface))
     {
        const char *iface_name;
        Eldbus_Object_Event_Interface_Added event;

        eldbus_message_iter_basic_get(entry_iface, &iface_name);
        event.proxy = eldbus_proxy_get(obj, iface_name);
        EINA_SAFETY_ON_NULL_RETURN(event.proxy);
        event.interface = iface_name;
        _eldbus_object_event_callback_call(obj, ELDBUS_OBJECT_EVENT_IFACE_ADDED,
                                          &event);
        eldbus_proxy_unref(event.proxy);
     }
}

/**
 * @internal
 * @brief Callback for the "InterfacesRemoved" D-Bus signal from org.freedesktop.DBus.ObjectManager.
 *
 * This function is invoked when an "InterfacesRemoved" signal is received for
 * the monitored object path. It parses the signal message, which contains the
 * object path and an array of interface names that were removed.
 * For each interface removed, it triggers the ELDBUS_OBJECT_EVENT_IFACE_REMOVED event.
 *
 * @param data The Eldbus_Object associated with this signal handler.
 * @param msg The received Eldbus_Message containing the signal data.
 *        The message arguments are expected to be:
 *        - "o": object_path (string)
 *        - "as": interfaces (array of strings)
 *          - Each string is an interface_name.
 */
static void
_cb_interfaces_removed(void *data, const Eldbus_Message *msg)
{
   Eldbus_Object *obj = data;
   const char *obj_path, *iface;
   Eldbus_Message_Iter *array_ifaces;

   if (!eldbus_message_arguments_get(msg, "oas", &obj_path, &array_ifaces))
     return;

   while (eldbus_message_iter_get_and_next(array_ifaces, 's', &iface))
     {
        Eldbus_Object_Event_Interface_Removed event;
        event.interface = iface;
        _eldbus_object_event_callback_call(obj, ELDBUS_OBJECT_EVENT_IFACE_REMOVED,
                                          &event);
     }
}

/**
 * @internal
 * @brief Iterator function for changed properties in a "PropertiesChanged" signal.
 *
 * This function is called for each property that has changed, as part of
 * processing the "PropertiesChanged" signal from org.freedesktop.DBus.Properties.
 * It extracts the property name (key) and its new value (var), then constructs
 * an Eldbus_Object_Event_Property_Changed event structure and triggers the
 * ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED event.
 *
 * @param data The Eldbus_Proxy associated with the interface whose property changed.
 * @param key The name of the property that changed (const char *).
 * @param var An Eldbus_Message_Iter pointing to the new value of the property (variant).
 */
static void
_property_changed_iter(void *data, const void *key, Eldbus_Message_Iter *var)
{
   Eldbus_Proxy *proxy = data;
   const char *skey = key;
   Eina_Value *st_value, stack_value;
   Eldbus_Object_Event_Property_Changed event;

   st_value = _message_iter_struct_to_eina_value(var);
   eina_value_struct_value_get(st_value, "arg0", &stack_value);

   event.interface = eldbus_proxy_interface_get(proxy);
   event.proxy = proxy;
   event.name = skey;
   event.value = &stack_value;
   _eldbus_object_event_callback_call(eldbus_proxy_object_get(proxy),
                                     ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED,
                                     &event);
   eina_value_free(st_value);
   eina_value_flush(&stack_value);
}

/**
 * @internal
 * @brief Callback for the "PropertiesChanged" D-Bus signal from org.freedesktop.DBus.Properties.
 *
 * This function is invoked when a "PropertiesChanged" signal is received.
 * The signal indicates that one or more properties of an interface have changed,
 * or that some properties have been invalidated (removed).
 *
 * It parses the signal message which contains:
 * - The interface name whose properties changed.
 * - A dictionary of changed properties (name to new value).
 * - An array of property names that were invalidated.
 *
 * For each changed property, it calls _property_changed_iter if there are listeners
 * for ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED.
 * For each invalidated property, it triggers an ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED
 * event if there are listeners for it.
 *
 * @param data The Eldbus_Object associated with this signal handler.
 * @param msg The received Eldbus_Message containing the signal data.
 *        The message arguments are expected to be:
 *        - "s": interface_name (string)
 *        - "a{sv}": changed_properties (dictionary of string to variant)
 *          - Each entry: property_name (string) to new_value (variant)
 *        - "as": invalidated_properties (array of strings)
 *          - Each string is a property_name.
 */
static void
_cb_properties_changed(void *data, const Eldbus_Message *msg)
{
   Eldbus_Object *obj = data;
   Eldbus_Proxy *proxy;
   Eldbus_Message_Iter *array, *invalidate;
   const char *iface;
   const char *invalidate_prop;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &array, &invalidate))
     {
        ERR("Error getting data from properties changed signal.");
        return;
     }

   proxy = eldbus_proxy_get(obj, iface);
   EINA_SAFETY_ON_NULL_RETURN(proxy);

   if (obj->event_handlers[ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED].list)
     eldbus_message_iter_dict_iterate(array, "sv", _property_changed_iter,
                                     proxy);

   if (!obj->event_handlers[ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED].list)
     goto end;

   while (eldbus_message_iter_get_and_next(invalidate, 's', &invalidate_prop))
     {
        Eldbus_Object_Event_Property_Removed event;
        event.interface = iface;
        event.name = invalidate_prop;
        event.proxy = proxy;
        _eldbus_object_event_callback_call(obj,
                                          ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED,
                                          &event);
     }
end:
   eldbus_proxy_unref(proxy);
}

/*
 * Doxygen documentation for eldbus_object_event_callback_add is in eldbus_object.h
 */
EAPI void
eldbus_object_event_callback_add(Eldbus_Object *obj, Eldbus_Object_Event_Type type, Eldbus_Object_Event_Cb cb, const void *cb_data)
{
   Eldbus_Object_Context_Event *ce;
   Eldbus_Object_Context_Event_Cb *ctx;

   ELDBUS_OBJECT_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   EINA_SAFETY_ON_TRUE_RETURN(type >= ELDBUS_OBJECT_EVENT_LAST);

   ce = obj->event_handlers + type;

   ctx = calloc(1, sizeof(Eldbus_Object_Context_Event_Cb));
   EINA_SAFETY_ON_NULL_RETURN(ctx);

   ctx->cb = cb;
   ctx->cb_data = cb_data;

   ce->list = eina_inlist_append(ce->list, EINA_INLIST_GET(ctx));

   switch (type)
     {
      case ELDBUS_OBJECT_EVENT_IFACE_ADDED:
         {
            if (obj->interfaces_added)
              break;
            obj->interfaces_added =
                     _eldbus_signal_handler_add(obj->conn, obj->name, NULL,
                                               ELDBUS_FDO_INTERFACE_OBJECT_MANAGER,
                                               "InterfacesAdded",
                                               _cb_interfaces_added, obj);
            EINA_SAFETY_ON_NULL_RETURN(obj->interfaces_added);
            eldbus_signal_handler_match_extra_set(obj->interfaces_added, "arg0",
                                                 obj->path, NULL);
            break;
         }
      case ELDBUS_OBJECT_EVENT_IFACE_REMOVED:
        {
           if (obj->interfaces_removed)
             break;
           obj->interfaces_removed =
                    _eldbus_signal_handler_add(obj->conn, obj->name, NULL,
                                              ELDBUS_FDO_INTERFACE_OBJECT_MANAGER,
                                              "InterfacesRemoved",
                                              _cb_interfaces_removed, obj);
           EINA_SAFETY_ON_NULL_RETURN(obj->interfaces_removed);
           eldbus_signal_handler_match_extra_set(obj->interfaces_removed,
                                                "arg0", obj->path, NULL);
           break;
        }
      case ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED:
      case ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED:
        {
           if (obj->properties_changed)
             break;
           obj->properties_changed =
                    eldbus_object_signal_handler_add(obj,
                                                    ELDBUS_FDO_INTERFACE_PROPERTIES,
                                                    "PropertiesChanged",
                                                    _cb_properties_changed, obj);
           EINA_SAFETY_ON_NULL_RETURN(obj->properties_changed);
           break;
        }
      default:
        break;
     }
}

/*
 * For _eldbus_object_context_event_cb_del Doxygen documentation, see its
 * forward declaration earlier in this file.
 */
static void
_eldbus_object_context_event_cb_del(Eldbus_Object_Context_Event *ce, Eldbus_Object_Context_Event_Cb *ctx)
{
   ce->list = eina_inlist_remove(ce->list, EINA_INLIST_GET(ctx));
   free(ctx);
}

EAPI void
eldbus_object_event_callback_del(Eldbus_Object *obj, Eldbus_Object_Event_Type type, Eldbus_Object_Event_Cb cb, const void *cb_data)
{
   Eldbus_Object_Context_Event *ce;
   Eldbus_Object_Context_Event_Cb *iter, *found = NULL;

   ELDBUS_OBJECT_CHECK(obj);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   EINA_SAFETY_ON_TRUE_RETURN(type >= ELDBUS_OBJECT_EVENT_LAST);

   ce = obj->event_handlers + type;

   EINA_INLIST_FOREACH(ce->list, iter)
     {
        if (cb != iter->cb) continue;
        if ((cb_data) && (cb_data != iter->cb_data)) continue;

        found = iter;
        break;
     }

   EINA_SAFETY_ON_NULL_RETURN(found);
   EINA_SAFETY_ON_TRUE_RETURN(found->deleted);

   if (ce->walking)
     {
        found->deleted = EINA_TRUE;
        ce->to_delete = eina_list_append(ce->to_delete, found);
        return;
     }

   _eldbus_object_context_event_cb_del(ce, found);

   switch (type)
     {
      case ELDBUS_OBJECT_EVENT_IFACE_ADDED:
         {
            if (obj->event_handlers[ELDBUS_OBJECT_EVENT_IFACE_ADDED].list)
              break;
            eldbus_signal_handler_del(obj->interfaces_added);
            obj->interfaces_added = NULL;
            break;
         }
      case ELDBUS_OBJECT_EVENT_IFACE_REMOVED:
        {
           if (obj->event_handlers[ELDBUS_OBJECT_EVENT_IFACE_REMOVED].list)
             break;
           eldbus_signal_handler_del(obj->interfaces_removed);
           obj->interfaces_removed = NULL;
           break;
        }
      case ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED:
      case ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED:
        {
           if (obj->event_handlers[ELDBUS_OBJECT_EVENT_PROPERTY_CHANGED].list ||
               obj->event_handlers[ELDBUS_OBJECT_EVENT_PROPERTY_REMOVED].list)
             break;
           eldbus_signal_handler_del(obj->properties_changed);
           obj->properties_changed = NULL;
           break;
        }
      default:
        break;
     }
}

/*
 * For _eldbus_object_event_callback_call Doxygen documentation, see its
 * forward declaration earlier in this file.
 */
static void
_eldbus_object_event_callback_call(Eldbus_Object *obj, Eldbus_Object_Event_Type type, const void *event_info)
{
   Eldbus_Object_Context_Event *ce;
   Eldbus_Object_Context_Event_Cb *iter;

   ce = obj->event_handlers + type;

   ce->walking++;
   EINA_INLIST_FOREACH(ce->list, iter)
     {
        if (iter->deleted) continue;
        iter->cb((void *)iter->cb_data, obj, (void *)event_info);
     }
   ce->walking--;
   if (ce->walking > 0) return;

   EINA_LIST_FREE(ce->to_delete, iter)
     _eldbus_object_context_event_cb_del(ce, iter);
}

EAPI Eldbus_Connection *
eldbus_object_connection_get(const Eldbus_Object *obj)
{
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   return obj->conn;
}

EAPI const char *
eldbus_object_bus_name_get(const Eldbus_Object *obj)
{
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   return obj->name;
}

EAPI const char *
eldbus_object_path_get(const Eldbus_Object *obj)
{
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   return obj->path;
}

/**
 * @internal
 * @brief Internal callback wrapper for messages sent via an Eldbus_Object.
 *
 * This function is used as the callback for eldbus_connection_send when
 * a message is sent using eldbus_object_send. It retrieves the user-provided
 * callback and object from the pending's data, removes the pending from the
 * object's list of pendings, and then calls the user's callback.
 *
 * @param data The user-provided callback data.
 * @param msg The reply message (or error message).
 * @param pending The Eldbus_Pending object associated with this message exchange.
 */
static void
_on_object_message_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eldbus_Message_Cb cb = eldbus_pending_data_del(pending, "__user_cb");
   Eldbus_Object *obj = eldbus_pending_data_del(pending, "__object");

   ELDBUS_OBJECT_CHECK(obj);
   obj->pendings = eina_inlist_remove(obj->pendings, EINA_INLIST_GET(pending));

   cb(data, msg, pending);
}

EAPI Eldbus_Pending *
eldbus_object_send(Eldbus_Object *obj, Eldbus_Message *msg, Eldbus_Message_Cb cb, const void *cb_data, double timeout)
{
   Eldbus_Pending *pending;

   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);

   if (!cb)
     {
        _eldbus_connection_send(obj->conn, msg, NULL, NULL, timeout);
        return NULL;
     }
   pending = _eldbus_connection_send(obj->conn, msg, _on_object_message_cb,
                                     cb_data, timeout);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pending, NULL);

   eldbus_pending_data_set(pending, "__user_cb", cb);
   eldbus_pending_data_set(pending, "__object", obj);
   obj->pendings = eina_inlist_append(obj->pendings, EINA_INLIST_GET(pending));

   return pending;
}

/*
 * For _on_signal_handler_free Doxygen documentation, see its forward declaration
 * earlier in this file.
 */
static void
_on_signal_handler_free(void *data, const void *dead_pointer)
{
   Eldbus_Object *obj = data;
   ELDBUS_OBJECT_CHECK(obj);
   obj->signal_handlers = eina_list_remove(obj->signal_handlers, dead_pointer);
}

/*
 * Doxygen documentation for eldbus_object_signal_handler_add is in eldbus_object.h
 */
EAPI Eldbus_Signal_Handler *
eldbus_object_signal_handler_add(Eldbus_Object *obj, const char *interface, const char *member, Eldbus_Signal_Cb cb, const void *cb_data)
{
   Eldbus_Signal_Handler *handler;

   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   handler = _eldbus_signal_handler_add(obj->conn, obj->name, obj->path,
                                       interface, member, cb, cb_data);
   EINA_SAFETY_ON_NULL_RETURN_VAL(handler, NULL);

   eldbus_signal_handler_free_cb_add(handler, _on_signal_handler_free, obj);
   obj->signal_handlers = eina_list_append(obj->signal_handlers, handler);

   return handler;
}

/*
 * Doxygen documentation for eldbus_object_method_call_new is in eldbus_object.h
 */
EAPI Eldbus_Message *
eldbus_object_method_call_new(Eldbus_Object *obj, const char *interface, const char *member)
{
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(interface, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(member, NULL);

   return eldbus_message_method_call_new(obj->name, obj->path, interface, member);
}

/**
 * @internal
 * @brief Adds a proxy to the object's internal hash of proxies.
 * This is typically called by eldbus_proxy_get().
 *
 * @param obj The Eldbus_Object.
 * @param proxy The Eldbus_Proxy to add.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eldbus_object_proxy_add(Eldbus_Object *obj, Eldbus_Proxy *proxy)
{
   return eina_hash_add(obj->proxies, eldbus_proxy_interface_get(proxy), proxy);
}

/**
 * @internal
 * @brief Retrieves a proxy from the object's internal hash of proxies by interface name.
 * This is typically called by eldbus_proxy_get().
 *
 * @param obj The Eldbus_Object.
 * @param interface The interface name of the proxy to retrieve.
 * @return The Eldbus_Proxy if found, NULL otherwise.
 */
Eldbus_Proxy *
eldbus_object_proxy_get(Eldbus_Object *obj, const char *interface)
{
   return eina_hash_find(obj->proxies, interface);
}

/**
 * @internal
 * @brief Deletes a proxy from the object's internal hash of proxies.
 * This is typically called when an Eldbus_Proxy is unref'd to 0.
 *
 * @param obj The Eldbus_Object.
 * @param proxy The Eldbus_Proxy to delete (for verification).
 * @param interface The interface name of the proxy to delete.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eldbus_object_proxy_del(Eldbus_Object *obj, Eldbus_Proxy *proxy, const char *interface)
{
   return eina_hash_del(obj->proxies, interface, proxy);
}

/*
 * Doxygen documentation for eldbus_object_peer_ping is in eldbus_object.h
 * (Note: eldbus_object.h does not currently have this EAPI, but assuming it should based on pattern)
 * If it's not meant to be EAPI, this comment should be adjusted.
 * For now, assuming it's an EAPI function that might be missing from the header or is new.
 */
EAPI Eldbus_Pending *
eldbus_object_peer_ping(Eldbus_Object *obj, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Pending *p;
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   msg = eldbus_object_method_call_new(obj, ELDBUS_FDO_INTEFACE_PEER, "Ping");
   p = eldbus_object_send(obj, msg, cb, data, -1);
   return p;
}

/*
 * Doxygen documentation for eldbus_object_peer_machine_id_get is in eldbus_object.h
 * (Note: eldbus_object.h does not currently have this EAPI, but assuming it should based on pattern)
 */
EAPI Eldbus_Pending *
eldbus_object_peer_machine_id_get(Eldbus_Object *obj, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Pending *p;
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   msg = eldbus_object_method_call_new(obj, ELDBUS_FDO_INTEFACE_PEER,
                                      "GetMachineId");
   p = eldbus_object_send(obj, msg, cb, data, -1);
   return p;
}

/*
 * Doxygen documentation for eldbus_object_introspect is in eldbus_object.h
 * (Note: eldbus_object.h does not currently have this EAPI, but assuming it should based on pattern)
 */
EAPI Eldbus_Pending *
eldbus_object_introspect(Eldbus_Object *obj, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Pending *p;
   ELDBUS_OBJECT_CHECK_RETVAL(obj, NULL);
   msg = eldbus_object_method_call_new(obj, ELDBUS_FDO_INTERFACE_INTROSPECTABLE,
                                      "Introspect");
   p = eldbus_object_send(obj, msg, cb, data, -1);
   return p;
}
