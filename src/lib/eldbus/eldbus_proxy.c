#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eldbus_private.h"
#include "eldbus_private_types.h"

/* TODO: mempool of Eldbus_Proxy, Eldbus_Proxy_Context_Event_Cb and
 * Eldbus_Proxy_Context_Event
 */

/**
 * @internal
 * @brief Represents a callback registered for a proxy event.
 *
 * This structure holds information about a single callback function
 * registered to be invoked when a specific Eldbus_Proxy_Event_Type occurs.
 * It's part of an Eina_Inlist to allow multiple callbacks for the same event.
 */
typedef struct _Eldbus_Proxy_Context_Event_Cb
{
   EINA_INLIST; /**< Macro to make this struct usable with Eina_Inlist */
   Eldbus_Proxy_Event_Cb cb; /**< The user-provided callback function */
   const void          *cb_data; /**< User data to be passed to the callback */
   Eina_Bool            deleted : 1; /**< Flag to mark if the callback is scheduled for deletion */
} Eldbus_Proxy_Context_Event_Cb;

/**
 * @internal
 * @brief Manages event callbacks for a specific proxy event type.
 *
 * This structure holds a list of registered callbacks (Eldbus_Proxy_Context_Event_Cb)
 * for a particular Eldbus_Proxy_Event_Type. It also manages the state during
 * callback invocation to handle safe deletion of callbacks.
 */
typedef struct _Eldbus_Proxy_Context_Event
{
   Eina_Inlist *list; /**< Inlist of Eldbus_Proxy_Context_Event_Cb for this event type */
   int          walking; /**< Counter to detect if we are currently iterating (walking) the callback list. Used to prevent modification during iteration. */
   Eina_List   *to_delete; /**< List of callbacks to be deleted after the current iteration finishes. */
} Eldbus_Proxy_Context_Event;

/**
 * @internal
 * @brief Represents a D-Bus proxy object.
 *
 * This structure encapsulates all the necessary information for interacting
 * with a remote D-Bus object's interface. It manages references, pending calls,
 * signal handlers, and properties associated with the proxy.
 */
struct _Eldbus_Proxy
{
   EINA_MAGIC; /**< Magic number for type checking */
   int                       refcount;
   Eldbus_Object             *obj; /**< The parent Eldbus_Object this proxy belongs to */
   const char               *interface; /**< The D-Bus interface name this proxy represents */
   Eina_Inlist              *pendings; /**< Inlist of Eldbus_Pending calls associated with this proxy */
   Eina_List                *handlers; /**< List of Eldbus_Signal_Handler instances attached to this proxy */
   Eina_Inlist              *cbs_free; /**< Inlist of Eldbus_Free_Cb callbacks to be called when the proxy is freed */
   Eina_Inlist              *data; /**< Inlist for storing arbitrary user data associated with this proxy (key-value pairs) */
   Eldbus_Proxy_Context_Event event_handlers[ELDBUS_PROXY_EVENT_LAST]; /**< Array to store event handlers for different proxy event types */
   Eina_Hash *props; /**< Hash table for caching properties of this proxy, if monitoring is enabled or properties are fetched. Keys are property names (const char *), values are Eina_Value*. */
   Eldbus_Signal_Handler *properties_changed; /**< Signal handler for the standard org.freedesktop.DBus.Properties.PropertiesChanged signal */
   Eina_Bool monitor_enabled:1; /**< Flag indicating if property monitoring is active for this proxy */
};

#define ELDBUS_PROXY_CHECK(proxy)                         \
  do                                                     \
    {                                                    \
       EINA_SAFETY_ON_NULL_RETURN(proxy);                \
       if (!EINA_MAGIC_CHECK(proxy, ELDBUS_PROXY_MAGIC))  \
         {                                               \
            EINA_MAGIC_FAIL(proxy, ELDBUS_PROXY_MAGIC);   \
            return;                                      \
         }                                               \
       EINA_SAFETY_ON_TRUE_RETURN(proxy->refcount <= 0); \
    }                                                    \
  while (0)

#define ELDBUS_PROXY_CHECK_RETVAL(proxy, retval)                      \
  do                                                                 \
    {                                                                \
       EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, retval);                \
       if (!EINA_MAGIC_CHECK(proxy, ELDBUS_PROXY_MAGIC))              \
         {                                                           \
            EINA_MAGIC_FAIL(proxy, ELDBUS_PROXY_MAGIC);               \
            return retval;                                           \
         }                                                           \
       EINA_SAFETY_ON_TRUE_RETURN_VAL(proxy->refcount <= 0, retval); \
    }                                                                \
  while (0)

#define ELDBUS_PROXY_CHECK_GOTO(proxy, label)                  \
  do                                                          \
    {                                                         \
       EINA_SAFETY_ON_NULL_GOTO(proxy, label);                \
       if (!EINA_MAGIC_CHECK(proxy, ELDBUS_PROXY_MAGIC))       \
         {                                                    \
            EINA_MAGIC_FAIL(proxy, ELDBUS_PROXY_MAGIC);        \
            goto label;                                       \
         }                                                    \
       EINA_SAFETY_ON_TRUE_GOTO(proxy->refcount <= 0, label); \
    }                                                         \
  while (0)

Eina_Bool
eldbus_proxy_init(void)
{
   return EINA_TRUE;
}

void
eldbus_proxy_shutdown(void)
{
}

static void _eldbus_proxy_event_callback_call(Eldbus_Proxy *proxy, Eldbus_Proxy_Event_Type type, const void *event_info);
static void _eldbus_proxy_context_event_cb_del(Eldbus_Proxy_Context_Event *ce, Eldbus_Proxy_Context_Event_Cb *ctx);
static void _on_signal_handler_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Invokes ELDBUS_PROXY_EVENT_DEL callbacks and clears them.
 *
 * This function is called when a proxy is being deleted. It triggers any
 * registered ELDBUS_PROXY_EVENT_DEL callbacks and then removes them to
 * prevent double invocation during the final cleanup in _eldbus_proxy_clear.
 *
 * @param proxy The proxy object that is being deleted.
 */
static void
_eldbus_proxy_call_del(Eldbus_Proxy *proxy)
{
   Eldbus_Proxy_Context_Event *ce;

   _eldbus_proxy_event_callback_call(proxy, ELDBUS_PROXY_EVENT_DEL, NULL);

   /* clear all del callbacks so we don't call them twice at
    * _eldbus_proxy_clear()
    */
   ce = proxy->event_handlers + ELDBUS_PROXY_EVENT_DEL;
   while (ce->list)
     {
        Eldbus_Proxy_Context_Event_Cb *ctx;

        ctx = EINA_INLIST_CONTAINER_GET(ce->list,
                                        Eldbus_Proxy_Context_Event_Cb);
        _eldbus_proxy_context_event_cb_del(ce, ctx);
     }
}

/**
 * @internal
 * @brief Cleans up resources associated with an Eldbus_Proxy.
 *
 * This function is responsible for releasing resources held by the proxy,
 * such as signal handlers, pending calls, and free callbacks. It's typically
 * called when the proxy's reference count drops to zero or when its parent
 * object is freed. It also calls _eldbus_proxy_call_del to handle DEL event
 * callbacks.
 *
 * @param proxy The proxy object to clear.
 */
static void
_eldbus_proxy_clear(Eldbus_Proxy *proxy)
{
   Eldbus_Signal_Handler *h;
   Eldbus_Pending *p;
   Eina_List *iter, *iter_next;
   Eina_Inlist *in_l;
   DBG("proxy=%p, refcount=%d, interface=%s, obj=%p",
       proxy, proxy->refcount, proxy->interface, proxy->obj);
   proxy->refcount = 1;
   eldbus_object_proxy_del(proxy->obj, proxy, proxy->interface);
   _eldbus_proxy_call_del(proxy);

   EINA_LIST_FOREACH_SAFE(proxy->handlers, iter, iter_next, h)
     {
        DBG("proxy=%p delete owned signal handler %p %s",
            proxy, h, eldbus_signal_handler_match_get(h));
        eldbus_signal_handler_del(h);
     }

   EINA_INLIST_FOREACH_SAFE(proxy->pendings, in_l, p)
     {
        DBG("proxy=%p delete owned pending call=%p dest=%s path=%s %s.%s()",
            proxy, p,
            eldbus_pending_destination_get(p),
            eldbus_pending_path_get(p),
            eldbus_pending_interface_get(p),
            eldbus_pending_method_get(p));
        eldbus_pending_cancel(p);
     }

   eldbus_cbs_free_dispatch(&(proxy->cbs_free), proxy);
   if (proxy->props)
     {
        eina_hash_free(proxy->props);
        proxy->props = NULL;
     }
   proxy->refcount = 0;
}

/**
 * @internal
 * @brief Frees the memory allocated for an Eldbus_Proxy structure.
 *
 * This function performs the final cleanup and deallocation of the proxy
 * object itself. It iterates through event handlers, frees associated lists,
 * unreferences the interface string, and finally frees the proxy structure.
 * This is called after _eldbus_proxy_clear has run.
 *
 * @param proxy The proxy object to free.
 */
static void
_eldbus_proxy_free(Eldbus_Proxy *proxy)
{
   unsigned int i;
   Eldbus_Signal_Handler *h;

   DBG("freeing proxy=%p", proxy);
   EINA_LIST_FREE(proxy->handlers, h)
     {
        if (h->dangling)
	  eldbus_signal_handler_free_cb_del(h, _on_signal_handler_free, proxy);
        else
           ERR("proxy=%p alive handler=%p %s", proxy, h,
               eldbus_signal_handler_match_get(h));
     }

   if (proxy->pendings)
     CRI("Proxy %p released with live pending calls!", proxy);

   for (i = 0; i < ELDBUS_PROXY_EVENT_LAST; i++)
     {
        Eldbus_Proxy_Context_Event *ce = proxy->event_handlers + i;
        while (ce->list)
          {
             Eldbus_Proxy_Context_Event_Cb *ctx;
             ctx = EINA_INLIST_CONTAINER_GET(ce->list,
                                             Eldbus_Proxy_Context_Event_Cb);
             _eldbus_proxy_context_event_cb_del(ce, ctx);
          }
        eina_list_free(ce->to_delete);
     }

   eina_stringshare_del(proxy->interface);
   EINA_MAGIC_SET(proxy, EINA_MAGIC_NONE);
   free(proxy);
}

/**
 * @internal
 * @brief Callback invoked when the parent Eldbus_Object of a proxy is freed.
 *
 * This function is registered as a free callback with the parent Eldbus_Object.
 * When the parent object is freed, this callback ensures that the associated
 * proxy is also properly cleaned up and freed. It first deletes all associated
 * user data, then calls _eldbus_proxy_clear to release resources, and finally
 * _eldbus_proxy_free to deallocate the proxy structure.
 *
 * @param data The Eldbus_Proxy object associated with the freed Eldbus_Object.
 * @param dead_pointer The Eldbus_Object that is being freed (unused).
 */
static void
_on_object_free(void *data, const void *dead_pointer EINA_UNUSED)
{
   Eldbus_Proxy *proxy = data;
   ELDBUS_PROXY_CHECK(proxy);
   DBG("proxy=%p, refcount=%d, interface=%s, obj=%p",
       proxy, proxy->refcount, proxy->interface, proxy->obj);
   eldbus_data_del_all(&(proxy->data));
   _eldbus_proxy_clear(proxy);
   _eldbus_proxy_free(proxy);
}

EAPI Eldbus_Proxy *
eldbus_proxy_get(Eldbus_Object *obj, const char *interface)
{
   Eldbus_Proxy *proxy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(interface, NULL);

   proxy = eldbus_object_proxy_get(obj, interface);
   if (proxy)
     return eldbus_proxy_ref(proxy);

   proxy = calloc(1, sizeof(Eldbus_Proxy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy, NULL);

   proxy->refcount = 1;
   proxy->obj = obj;
   proxy->interface = eina_stringshare_add(interface);
   EINA_MAGIC_SET(proxy, ELDBUS_PROXY_MAGIC);
   if (!eldbus_object_proxy_add(obj, proxy))
     goto cleanup;
   eldbus_object_free_cb_add(obj, _on_object_free, proxy);

   return proxy;

cleanup:
   eina_stringshare_del(proxy->interface);
   free(proxy);
   return NULL;
}

// Forward declaration, actual definition is later in the file.
static void _on_signal_handler_free(void *data, const void *dead_pointer);

/**
 * @internal
 * @brief Decrements the reference count of an Eldbus_Proxy and frees it if count reaches zero.
 *
 * This is the internal implementation for unreferencing a proxy. It decrements
 * the refcount. If the refcount becomes zero, it removes the object free callback,
 * deletes all associated user data, clears proxy resources via _eldbus_proxy_clear,
 * and finally frees the proxy structure via _eldbus_proxy_free.
 *
 * @param proxy The proxy object to unreference.
 */
static void
_eldbus_proxy_unref(Eldbus_Proxy *proxy)
{
   proxy->refcount--;
   if (proxy->refcount > 0) return;

   eldbus_object_free_cb_del(proxy->obj, _on_object_free, proxy);
   eldbus_data_del_all(&(proxy->data));
   _eldbus_proxy_clear(proxy);
   _eldbus_proxy_free(proxy);
}

EAPI Eldbus_Proxy *
eldbus_proxy_ref(Eldbus_Proxy *proxy)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   DBG("proxy=%p, pre-refcount=%d, interface=%s, obj=%p",
       proxy, proxy->refcount, proxy->interface, proxy->obj);
   proxy->refcount++;
   return proxy;
}

EAPI void
eldbus_proxy_unref(Eldbus_Proxy *proxy)
{
   ELDBUS_PROXY_CHECK(proxy);
   DBG("proxy=%p, pre-refcount=%d, interface=%s, obj=%p",
       proxy, proxy->refcount, proxy->interface, proxy->obj);
   _eldbus_proxy_unref(proxy);
}

EAPI void
eldbus_proxy_free_cb_add(Eldbus_Proxy *proxy, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_PROXY_CHECK(proxy);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   proxy->cbs_free = eldbus_cbs_free_add(proxy->cbs_free, cb, data);
}

EAPI void
eldbus_proxy_free_cb_del(Eldbus_Proxy *proxy, Eldbus_Free_Cb cb, const void *data)
{
   ELDBUS_PROXY_CHECK(proxy);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   proxy->cbs_free = eldbus_cbs_free_del(proxy->cbs_free, cb, data);
}

EAPI void
eldbus_proxy_data_set(Eldbus_Proxy *proxy, const char *key, const void *data)
{
   ELDBUS_PROXY_CHECK(proxy);
   EINA_SAFETY_ON_NULL_RETURN(key);
   EINA_SAFETY_ON_NULL_RETURN(data);
   eldbus_data_set(&(proxy->data), key, data);
}

EAPI void *
eldbus_proxy_data_get(const Eldbus_Proxy *proxy, const char *key)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, NULL);
   return eldbus_data_get(&(((Eldbus_Proxy *)proxy)->data), key);
}

EAPI void *
eldbus_proxy_data_del(Eldbus_Proxy *proxy, const char *key)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, NULL);
   return eldbus_data_del(&(((Eldbus_Proxy *)proxy)->data), key);
}

/**
 * @internal
 * @brief Converts a D-Bus variant (from a message iterator) to an Eina_Value and stores it in a hash.
 *
 * This function takes a D-Bus message iterator pointing to a variant,
 * extracts its value, converts it into an Eina_Value, and then sets or updates
 * this Eina_Value in the provided Eina_Hash (props) using the given key.
 * If the key doesn't exist in the hash, a new Eina_Value is created and added.
 * If it exists, the existing Eina_Value is updated.
 *
 * The D-Bus variant is expected to be a struct containing a single element,
 * which is the actual value of the property. This is typical for how properties
 * are represented in D-Bus (e.g., in PropertiesChanged signals or GetAll results).
 *
 * @param props The Eina_Hash (property cache) where the Eina_Value will be stored.
 * @param key The property name (key for the hash).
 * @param var An Eldbus_Message_Iter pointing to the D-Bus variant (typically 'v').
 * @return The Eina_Value that was set or updated in the hash.
 */
static Eina_Value *
_iter_hash_value_set(Eina_Hash *props, const char *key, Eldbus_Message_Iter *var) EINA_ARG_NONNULL(1, 2, 3)
{
   // Convert the D-Bus variant (expected to be a struct with one member) to an Eina_Value.
   Eina_Value *st_value = _message_iter_struct_to_eina_value(var);
   Eina_Value *value;
   Eina_Value stack_value;

   eina_value_struct_value_get(st_value, "arg0", &stack_value);

   value = eina_hash_find(props, key);
   if (!value)
     {
        value = eina_value_new(eina_value_type_get(&stack_value));
        eina_hash_add(props, key, value);
     }

   eina_value_flush(value);
   eina_value_copy(&stack_value, value);

   eina_value_flush(&stack_value);
   eina_value_free(st_value);
   return value;
}

/**
 * @internal
 * @brief Callback function for iterating over changed properties in a D-Bus PropertiesChanged signal.
 *
 * This function is used with eldbus_message_iter_dict_iterate to process each
 * property in the "properties changed" dictionary of a PropertiesChanged signal.
 * For each property, it updates the local property cache (proxy->props) using
 * _iter_hash_value_set and then triggers the ELDBUS_PROXY_EVENT_PROPERTY_CHANGED
 * event for that specific property.
 *
 * @param data The Eldbus_Proxy instance.
 * @param key The name of the property that changed (const char *).
 * @param var An Eldbus_Message_Iter pointing to the new D-Bus variant value of the property.
 */
static void
_property_changed_iter(void *data, const void *key, Eldbus_Message_Iter *var)
{
   Eldbus_Proxy *proxy = data;
   const char *skey = key;

   Eina_Value *value = _iter_hash_value_set(proxy->props, skey, var);

   Eldbus_Proxy_Event_Property_Changed event = {.name = skey,
                                                .value = value,
                                                .proxy = proxy};
   _eldbus_proxy_event_callback_call(proxy, ELDBUS_PROXY_EVENT_PROPERTY_CHANGED,
                                     &event);
}

/**
 * @internal
 * @brief Handles the org.freedesktop.DBus.Properties.PropertiesChanged signal.
 *
 * This function is registered as a callback for the PropertiesChanged signal.
 * It parses the signal message, which contains the interface name, a dictionary
 * of changed properties (name to new variant value), and an array of invalidated
 * property names.
 *
 * It iterates through the changed properties, updating the local cache and
 * emitting ELDBUS_PROXY_EVENT_PROPERTY_CHANGED events.
 * It then iterates through the invalidated properties, removing them from the
 * cache and emitting ELDBUS_PROXY_EVENT_PROPERTY_REMOVED events.
 *
 * @param data The Eldbus_Proxy instance.
 * @param msg The Eldbus_Message containing the PropertiesChanged signal data.
 */
static void
_properties_changed(void *data, const Eldbus_Message *msg)
{
   Eldbus_Proxy *proxy = data;
   Eldbus_Message_Iter *array, *invalidate; // Iterators for changed properties dictionary and invalidated properties array
   const char *iface;
   const char *invalidate_prop;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as", &iface, &array, &invalidate))
     {
        ERR("Error getting data from properties changed signal.");
        return;
     }
   if (proxy->props)
     eldbus_message_iter_dict_iterate(array, "sv", _property_changed_iter,
                                     proxy);

   while (eldbus_message_iter_get_and_next(invalidate, 's', &invalidate_prop))
     {
        Eldbus_Proxy_Event_Property_Removed event;
        event.interface = proxy->interface;
        event.name = invalidate_prop;
        event.proxy = proxy;
        if (proxy->props)
          eina_hash_del(proxy->props, event.name, NULL);
        _eldbus_proxy_event_callback_call(proxy, ELDBUS_PROXY_EVENT_PROPERTY_REMOVED,
                                         &event);
     }
}

/**
 * @internal
 * @brief Frees an Eina_Value stored in the properties cache.
 *
 * This function is used as a callback for eina_hash_string_superfast_new
 * to automatically free Eina_Value objects when they are removed from the
 * proxy's properties cache (proxy->props).
 *
 * @param data The Eina_Value to be freed.
 */
static void
_props_cache_free(void *data)
{
   Eina_Value *value = data;
   eina_value_free(value);
}

EAPI void
eldbus_proxy_event_callback_add(Eldbus_Proxy *proxy, Eldbus_Proxy_Event_Type type, Eldbus_Proxy_Event_Cb cb, const void *cb_data)
{
   Eldbus_Proxy_Context_Event *ce;
   Eldbus_Proxy_Context_Event_Cb *ctx;

   ELDBUS_PROXY_CHECK(proxy);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   EINA_SAFETY_ON_TRUE_RETURN(type >= ELDBUS_PROXY_EVENT_LAST);

   ce = proxy->event_handlers + type;

   ctx = calloc(1, sizeof(Eldbus_Proxy_Context_Event_Cb));
   EINA_SAFETY_ON_NULL_RETURN(ctx);
   ctx->cb = cb;
   ctx->cb_data = cb_data;

   ce->list = eina_inlist_append(ce->list, EINA_INLIST_GET(ctx));

   if (type == ELDBUS_PROXY_EVENT_PROPERTY_CHANGED)
     {
        if (proxy->properties_changed) return;
        if (!proxy->props)
          proxy->props = eina_hash_string_superfast_new(_props_cache_free);
        proxy->properties_changed =
                 eldbus_proxy_properties_changed_callback_add(proxy,
                                                             _properties_changed,
                                                             proxy);
     }
   else if (type == ELDBUS_PROXY_EVENT_PROPERTY_REMOVED)
     {
        if (proxy->properties_changed) return;
        proxy->properties_changed =
                 eldbus_proxy_properties_changed_callback_add(proxy,
                                                             _properties_changed,
                                                             proxy);
     }
}

/**
 * @internal
 * @brief Deletes a specific event callback context.
 *
 * Removes the given Eldbus_Proxy_Context_Event_Cb (ctx) from the event
 * context's (ce) list of callbacks and frees the memory associated with ctx.
 *
 * @param ce The Eldbus_Proxy_Context_Event from which to remove the callback.
 * @param ctx The Eldbus_Proxy_Context_Event_Cb to remove and free.
 */
static void
_eldbus_proxy_context_event_cb_del(Eldbus_Proxy_Context_Event *ce, Eldbus_Proxy_Context_Event_Cb *ctx)
{
   ce->list = eina_inlist_remove(ce->list, EINA_INLIST_GET(ctx));
   free(ctx);
}

EAPI void
eldbus_proxy_event_callback_del(Eldbus_Proxy *proxy, Eldbus_Proxy_Event_Type type, Eldbus_Proxy_Event_Cb cb, const void *cb_data)
{
   Eldbus_Proxy_Context_Event *ce;
   Eldbus_Proxy_Context_Event_Cb *iter, *found = NULL;

   ELDBUS_PROXY_CHECK(proxy);
   EINA_SAFETY_ON_NULL_RETURN(cb);
   EINA_SAFETY_ON_TRUE_RETURN(type >= ELDBUS_PROXY_EVENT_LAST);

   ce = proxy->event_handlers + type;

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

   _eldbus_proxy_context_event_cb_del(ce, found);

   if (type == ELDBUS_PROXY_EVENT_PROPERTY_CHANGED)
     {
        Eldbus_Proxy_Context_Event *ce_prop_remove;
        ce_prop_remove = proxy->event_handlers +
                 ELDBUS_PROXY_EVENT_PROPERTY_REMOVED;
        if (!ce->list && !proxy->monitor_enabled)
          {
             eina_hash_free(proxy->props);
             proxy->props = NULL;
          }

        if (!ce_prop_remove->list && !ce->list && !proxy->monitor_enabled)
          {
             eldbus_signal_handler_unref(proxy->properties_changed);
             proxy->properties_changed = NULL;
          }
     }
   else if (type == ELDBUS_PROXY_EVENT_PROPERTY_REMOVED)
     {
        Eldbus_Proxy_Context_Event *ce_prop_changed;
        ce_prop_changed = proxy->event_handlers +
                 ELDBUS_PROXY_EVENT_PROPERTY_CHANGED;

        if (!ce_prop_changed->list && !ce->list && !proxy->monitor_enabled)
          {
             eldbus_signal_handler_unref(proxy->properties_changed);
             proxy->properties_changed = NULL;
          }
     }
}

/**
 * @internal
 * @brief Invokes all registered callbacks for a given proxy event type.
 *
 * Iterates through the list of registered callbacks for the specified event type
 * and calls each one. It handles safe deletion of callbacks by marking them
 * as deleted if a deletion request occurs during the iteration, and then
 * actually removing them after the iteration is complete.
 *
 * @param proxy The Eldbus_Proxy instance.
 * @param type The Eldbus_Proxy_Event_Type for which to call callbacks.
 * @param event_info The event-specific data to pass to the callbacks.
 *                   For example, for ELDBUS_PROXY_EVENT_PROPERTY_CHANGED, this would be
 *                   an Eldbus_Proxy_Event_Property_Changed struct.
 */
static void
_eldbus_proxy_event_callback_call(Eldbus_Proxy *proxy, Eldbus_Proxy_Event_Type type, const void *event_info)
{
   Eldbus_Proxy_Context_Event *ce;
   Eldbus_Proxy_Context_Event_Cb *iter;

   ce = proxy->event_handlers + type;

   ce->walking++;
   EINA_INLIST_FOREACH(ce->list, iter)
     {
        if (iter->deleted) continue;
        iter->cb((void *)iter->cb_data, proxy, (void *)event_info);
     }
   ce->walking--;
   if (ce->walking > 0) return;

   EINA_LIST_FREE(ce->to_delete, iter)
     _eldbus_proxy_context_event_cb_del(ce, iter);
}

EAPI Eldbus_Object *
eldbus_proxy_object_get(const Eldbus_Proxy *proxy)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   return proxy->obj;
}

EAPI const char *
eldbus_proxy_interface_get(const Eldbus_Proxy *proxy)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   return proxy->interface;
}

static void
_on_proxy_message_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eldbus_Message_Cb cb = eldbus_pending_data_del(pending, "__user_cb");
   Eldbus_Proxy *proxy = eldbus_pending_data_del(pending, "__proxy");

   ELDBUS_PROXY_CHECK(proxy);
   proxy->pendings = eina_inlist_remove(proxy->pendings,
                                        EINA_INLIST_GET(pending));
   cb(data, msg, pending);
}

/**
 * @internal
 * @brief Internal implementation for sending a message via a proxy.
 *
 * This function sends a message through the connection associated with the proxy.
 * If a callback `cb` is provided, it wraps this callback with `_on_proxy_message_cb`
 * to manage the pending call's lifecycle within the proxy's list of pendings.
 * The original user callback and the proxy itself are stored as data on the
 * Eldbus_Pending object.
 *
 * @param proxy The proxy through which to send the message.
 * @param msg The message to send.
 * @param cb User callback for the reply or NULL if no reply/callback is needed.
 * @param cb_data User data for the callback.
 * @param timeout Timeout in milliseconds.
 * @return An Eldbus_Pending object if a callback is provided, otherwise NULL.
 */
static Eldbus_Pending *
_eldbus_proxy_send(Eldbus_Proxy *proxy, Eldbus_Message *msg, Eldbus_Message_Cb cb, const void *cb_data, double timeout)
{
   Eldbus_Pending *pending;

   if (!cb)
     {
        _eldbus_connection_send(proxy->obj->conn, msg, NULL, NULL, timeout);
        return NULL;
     }
   pending = _eldbus_connection_send(proxy->obj->conn, msg,
                                     _on_proxy_message_cb, cb_data, timeout);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pending, NULL);

   eldbus_pending_data_set(pending, "__user_cb", cb);
   eldbus_pending_data_set(pending, "__proxy", proxy);
   proxy->pendings = eina_inlist_append(proxy->pendings,
                                        EINA_INLIST_GET(pending));

   return pending;
}

/**
 * @internal
 * @brief Internal implementation for sending a message and blocking for a reply.
 *
 * This function simply forwards the call to the connection's send_and_block
 * function, using the connection associated with the proxy's object.
 *
 * @param proxy The proxy through which to send the message.
 * @param msg The message to send.
 * @param timeout Timeout in milliseconds.
 * @return The reply message, an error message, or NULL on timeout/error.
 */
static Eldbus_Message *
_eldbus_proxy_send_and_block(Eldbus_Proxy *proxy, Eldbus_Message *msg, double timeout)
{
   return _eldbus_connection_send_and_block(proxy->obj->conn, msg, timeout);
}

EAPI Eldbus_Pending *
eldbus_proxy_send(Eldbus_Proxy *proxy, Eldbus_Message *msg, Eldbus_Message_Cb cb, const void *cb_data, double timeout)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);

   return _eldbus_proxy_send(proxy, msg, cb, cb_data, timeout);
}

EAPI Eldbus_Message *
eldbus_proxy_send_and_block(Eldbus_Proxy *proxy, Eldbus_Message *msg, double timeout)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);

   return _eldbus_proxy_send_and_block(proxy, msg, timeout);
}

EAPI Eldbus_Message *
eldbus_proxy_method_call_new(Eldbus_Proxy *proxy, const char *member)
{
   Eldbus_Message *msg;
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);

   msg = eldbus_message_method_call_new(
                           eldbus_object_bus_name_get(proxy->obj),
                           eldbus_object_path_get(proxy->obj),
                           proxy->interface, member);
   return msg;
}

/**
 * @internal
 * @brief Internal implementation for calling a D-Bus method with va_list arguments.
 *
 * This function creates a new method call message, appends arguments from the
 * va_list based on the provided signature, and then sends the message using
 * _eldbus_proxy_send.
 *
 * @param proxy The proxy on which to call the method.
 * @param member The name of the method to call.
 * @param cb Callback for the reply.
 * @param cb_data User data for the callback.
 * @param timeout Timeout in milliseconds.
 * @param signature D-Bus signature string for the arguments.
 * @param ap va_list of arguments.
 * @return An Eldbus_Pending object for the method call.
 */
static Eldbus_Pending *
_eldbus_proxy_vcall(Eldbus_Proxy *proxy, const char *member, Eldbus_Message_Cb cb, const void *cb_data, double timeout, const char *signature, va_list ap)
{
   Eldbus_Message *msg = eldbus_proxy_method_call_new(proxy, member);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, NULL);

   if (!eldbus_message_arguments_vappend(msg, signature, ap))
     {
        eldbus_message_unref(msg);
        ERR("Error setting arguments");
        return NULL;
     }

   return _eldbus_proxy_send(proxy, msg, cb, cb_data, timeout);
}

EAPI Eldbus_Pending *
eldbus_proxy_call(Eldbus_Proxy *proxy, const char *member, Eldbus_Message_Cb cb, const void *cb_data, double timeout, const char *signature, ...)
{
   /**
    * @page eldbus_proxy_call_example Eldbus_proxy_call Example
    *
    * @code
    * // Example: Calling a method "TestMethod" on interface "com.example.Interface"
    * // which takes an integer (i) and a string (s) and returns a boolean (b).
    *
    * void on_method_reply(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
    * {
    *    Eldbus_Proxy *proxy = data;
    *    const char *errname, *errmsg;
    *    Eina_Bool result;
    *
    *    if (eldbus_message_error_get(msg, &errname, &errmsg))
    *    {
    *        fprintf(stderr, "Error: %s %s\n", errname, errmsg);
    *        return;
    *    }
    *
    *    if (!eldbus_message_arguments_get(msg, "b", &result))
    *    {
    *        fprintf(stderr, "Error getting arguments from reply\n");
    *        return;
    *    }
    *    printf("Method returned: %s\n", result ? "true" : "false");
    * }
    *
    * // ... assuming proxy is a valid Eldbus_Proxy* ...
    * int32_t my_int = 42;
    * const char *my_string = "hello world";
    *
    * eldbus_proxy_call(proxy, "TestMethod", on_method_reply, proxy, -1, "is", my_int, my_string);
    * @endcode
    */
   Eldbus_Pending *pending;
   va_list ap;

   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(member, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(signature, NULL);

   va_start(ap, signature);
   pending = _eldbus_proxy_vcall(proxy, member, cb, cb_data, timeout,
                                signature, ap);
   va_end(ap);

   return pending;
}

EAPI Eldbus_Pending *
eldbus_proxy_vcall(Eldbus_Proxy *proxy, const char *member, Eldbus_Message_Cb cb, const void *cb_data, double timeout, const char *signature, va_list ap)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(member, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(signature, NULL);

   return _eldbus_proxy_vcall(proxy, member, cb, cb_data, timeout,
                             signature, ap);
}

static void
_on_signal_handler_free(void *data, const void *dead_pointer)
{
   Eldbus_Proxy *proxy = data;
   ELDBUS_PROXY_CHECK(proxy);
   proxy->handlers = eina_list_remove(proxy->handlers, dead_pointer);
}

EAPI Eldbus_Signal_Handler *
eldbus_proxy_signal_handler_add(Eldbus_Proxy *proxy, const char *member, Eldbus_Signal_Cb cb, const void *cb_data)
{
   Eldbus_Signal_Handler *handler;
   const char *name, *path;

   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   /**
    * Example of cb_data usage:
    * @code
    * typedef struct {
    *     int id;
    *     const char *description;
    * } MySignalData;
    *
    * void my_signal_callback(void *data, const Eldbus_Message *message) {
    *     MySignalData *signal_info = data;
    *     // Process signal using signal_info->id and signal_info->description
    *     // ... extract arguments from message ...
    * }
    *
    * MySignalData *user_data = malloc(sizeof(MySignalData));
    * user_data->id = 123;
    * user_data->description = "Handler for MySignal";
    *
    * // Assuming 'proxy' is a valid Eldbus_Proxy* for the desired interface
    * // and 'MySignal' is the signal name.
    * eldbus_proxy_signal_handler_add(proxy, "MySignal", my_signal_callback, user_data);
    * // Remember to free user_data when the signal handler is removed or proxy is freed.
    * @endcode
    */
   name = eldbus_object_bus_name_get(proxy->obj);
   path = eldbus_object_path_get(proxy->obj);

   handler = _eldbus_signal_handler_add(proxy->obj->conn, name, path,
                                       proxy->interface, member, cb, cb_data);
   EINA_SAFETY_ON_NULL_RETURN_VAL(handler, NULL);
   DBG("signal handler added: proxy=%p handler=%p cb=%p", proxy, handler, cb);

   eldbus_signal_handler_free_cb_add(handler, _on_signal_handler_free, proxy);
   proxy->handlers = eina_list_append(proxy->handlers, handler);

   return handler;
}

EAPI Eldbus_Pending *
eldbus_proxy_property_get(Eldbus_Proxy *proxy, const char *name, Eldbus_Message_Cb cb, const void *data)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   return eldbus_proxy_call(proxy->obj->properties, "Get", cb, data, -1,
                           "ss", proxy->interface, name);
}

static inline Eina_Bool
_type_is_number(char sig)
{
   switch (sig)
     {
      case 'y': case 'b': case 'n': case 'q': case 'i':
      case 'u': case 'x': case 't': case 'd': case 'h':
        return EINA_TRUE;
      default:
        break;
     }
   return EINA_FALSE;
}

/**
 * @brief Set a property value on a remote object.
 *
 * This function calls the "Set" method of the "org.freedesktop.DBus.Properties"
 * interface on the remote object to set the value of a property.
 *
 * @param proxy The proxy object.
 * @param name The name of the property to set.
 * @param sig The D-Bus signature string of the property's value.
 *            Example: "s" for string, "i" for int32, "b" for boolean.
 * @param value A pointer to the value to set. For basic types, this is a direct pointer
 *              to the value (e.g., `int *` for signature "i", `const char **` for "s").
 *              For complex types (structs, arrays), this should be an Eina_Value
 *              pointer representing the structure or array, and you should use
 *              eldbus_proxy_property_value_set instead, or construct the message manually.
 *              However, for basic types passed by pointer (like `const char *`),
 *              you pass `&my_string_ptr`.
 * @param cb Callback function to be invoked when the reply is received.
 * @param data User data to be passed to the callback function.
 * @return An #Eldbus_Pending object representing the asynchronous method call,
 *         or @c NULL on error.
 *
 * @note For setting properties with complex types (structs, arrays not of basic types),
 *       it's generally easier to use eldbus_proxy_property_value_set() or
 *       construct the Eldbus_Message manually and use eldbus_proxy_send().
 *
 * Example for basic types:
 * @code
 * int32_t new_int_val = 123;
 * const char *new_str_val = "hello";
 * Eina_Bool new_bool_val = EINA_TRUE;
 *
 * // Set an integer property "IntProperty"
 * eldbus_proxy_property_set(proxy, "IntProperty", "i", &new_int_val, on_set_reply_cb, NULL);
 *
 * // Set a string property "StringProperty"
 * eldbus_proxy_property_set(proxy, "StringProperty", "s", &new_str_val, on_set_reply_cb, NULL);
 *
 * // Set a boolean property "BoolProperty"
 * eldbus_proxy_property_set(proxy, "BoolProperty", "b", &new_bool_val, on_set_reply_cb, NULL);
 * @endcode
 */
EAPI Eldbus_Pending *
eldbus_proxy_property_set(Eldbus_Proxy *proxy, const char *name, const char *sig, const void *value, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *iter, *variant;

   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sig, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(dbus_signature_validate_single(sig, NULL), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((_type_is_number(sig[0]) || value), NULL);

   msg = eldbus_proxy_method_call_new(proxy->obj->properties, "Set");
   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_append(iter, 's', proxy->interface);
   eldbus_message_iter_basic_append(iter, 's', name);
   variant = eldbus_message_iter_container_new(iter, 'v', sig);
   if (dbus_type_is_basic(sig[0]))
     dbus_message_iter_append_basic(&variant->dbus_iterator, sig[0], &value);
   else
     {
        if (!_message_iter_from_eina_value_struct(sig, variant, value))
          {
             eldbus_message_unref(msg);
             return NULL;
          }
     }
   eldbus_message_iter_container_close(iter, variant);

   return eldbus_proxy_send(proxy->obj->properties, msg, cb, data, -1);
}

/**
 * @brief Set a property value on a remote object using an Eina_Value.
 *
 * This function calls the "Set" method of the "org.freedesktop.DBus.Properties"
 * interface on the remote object to set the value of a property.
 * The value is provided as an Eina_Value.
 *
 * @param proxy The proxy object.
 * @param name The name of the property to set.
 * @param sig The D-Bus signature string of the property's value.
 *            Example: "s" for string, "(is)" for a struct of int and string, "ai" for an array of integers.
 * @param value An Eina_Value containing the value to set. The type of the Eina_Value
 *              must be compatible with the D-Bus signature `sig`.
 *              For basic types, it's a simple Eina_Value.
 *              For structs, it's an Eina_Value of type EINA_VALUE_TYPE_STRUCT.
 *              For arrays, it's an Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 * @param cb Callback function to be invoked when the reply is received.
 * @param data User data to be passed to the callback function.
 * @return An #Eldbus_Pending object representing the asynchronous method call,
 *         or @c NULL on error.
 *
 * Example for an array of strings (as):
 * @code
 * Eina_Value array_val;
 * eina_value_array_setup(&array_val, EINA_VALUE_TYPE_STRING, 0);
 * eina_value_array_append(&array_val, "item1");
 * eina_value_array_append(&array_val, "item2");
 *
 * eldbus_proxy_property_value_set(proxy, "StringArrayProperty", "as", &array_val, on_set_reply_cb, NULL);
 * eina_value_flush(&array_val);
 * @endcode
 *
 * Example for a struct (is) - a struct with an int and a string:
 * @code
 * Eina_Value struct_val;
 * Eina_Value_Struct_Desc desc = EINA_VALUE_STRUCT_DESC_DEFAULT("MyStruct", NULL);
 * eina_value_struct_desc_member_add(&desc, "field_int", EINA_VALUE_TYPE_INT);
 * eina_value_struct_desc_member_add(&desc, "field_str", EINA_VALUE_TYPE_STRING);
 *
 * eina_value_struct_setup(&struct_val, &desc);
 * eina_value_struct_set(&struct_val, "field_int", 123);
 * eina_value_struct_set(&struct_val, "field_str", "hello struct");
 *
 * eldbus_proxy_property_value_set(proxy, "MyStructProperty", "(is)", &struct_val, on_set_reply_cb, NULL);
 * eina_value_flush(&struct_val);
 * @endcode
 */
EAPI Eldbus_Pending *
eldbus_proxy_property_value_set(Eldbus_Proxy *proxy, const char *name, const char *sig, const Eina_Value *value, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *iter, *variant;

   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sig, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(dbus_signature_validate_single(sig, NULL), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((_type_is_number(sig[0]) || value), NULL);

   msg = eldbus_proxy_method_call_new(proxy->obj->properties, "Set");
   iter = eldbus_message_iter_get(msg);
   eldbus_message_iter_basic_append(iter, 's', proxy->interface);
   eldbus_message_iter_basic_append(iter, 's', name);
   variant = eldbus_message_iter_container_new(iter, 'v', sig);
   if (dbus_type_is_basic(sig[0]))
     {
        if (!_message_iter_from_eina_value(sig, variant, value))
          goto error;
     }
   else
     {
        if (!_message_iter_from_eina_value_struct(sig, variant, value))
          goto error;
     }
   eldbus_message_iter_container_close(iter, variant);

   return eldbus_proxy_send(proxy->obj->properties, msg, cb, data, -1);

error:
   eldbus_message_unref(msg);
   return NULL;
}

EAPI Eldbus_Pending *
eldbus_proxy_property_get_all(Eldbus_Proxy *proxy, Eldbus_Message_Cb cb, const void *data)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   return eldbus_proxy_call(proxy->obj->properties, "GetAll", cb, data, -1,
                            "s", proxy->interface);
}

EAPI Eldbus_Signal_Handler *
eldbus_proxy_properties_changed_callback_add(Eldbus_Proxy *proxy, Eldbus_Signal_Cb cb, const void *data)
{
   Eldbus_Signal_Handler *sh;
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   sh = eldbus_proxy_signal_handler_add(proxy->obj->properties,
                                        "PropertiesChanged", cb, data);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sh, NULL);
   eldbus_signal_handler_match_extra_set(sh, "arg0", proxy->interface, NULL);
   return sh;
}

static void
_property_iter(void *data, const void *key, Eldbus_Message_Iter *var)
{
   Eldbus_Proxy *proxy = data;
   const char *skey = key;

   _iter_hash_value_set(proxy->props, skey, var);
}

/**
 * @internal
 * @brief Callback invoked when a monitored proxy is deleted.
 *
 * This function is registered as an ELDBUS_PROXY_EVENT_DEL callback when
 * eldbus_proxy_properties_monitor is enabled. If the proxy is deleted while
 * the initial "GetAll" properties call is still pending, this callback
 * cancels that pending call to prevent issues.
 *
 * @param data The Eldbus_Pending object for the "GetAll" call.
 * @param proxy The proxy being deleted (unused).
 * @param event_info Event information (unused).
 */
static void
_on_monitored_proxy_del(void *data, Eldbus_Proxy *proxy EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eldbus_Pending *pending = data;
   eldbus_pending_cancel(pending);
}

/**
 * @internal
 * @brief Callback for the reply of the "GetAll" properties method.
 *
 * This function is invoked when the reply to the "GetAll" method call (initiated
 * by eldbus_proxy_properties_monitor) is received. It parses the dictionary of
 * properties from the message, populates the local property cache (proxy->props)
 * using _property_iter, and then triggers the ELDBUS_PROXY_EVENT_PROPERTY_LOADED
 * event. It also unregisters the _on_monitored_proxy_del callback as the
 * initial fetch is now complete or has failed.
 *
 * @param data The Eldbus_Proxy instance.
 * @param msg The reply message from the "GetAll" call.
 * @param pending The Eldbus_Pending object for the "GetAll" call.
 */
static void
_props_get_all(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eldbus_Proxy *proxy = data;
   Eldbus_Message_Iter *dict; // Iterator for the dictionary of properties a{sv}
   const char *name, *error_msg;
   Eldbus_Proxy_Event_Property_Loaded event;

   eldbus_proxy_event_callback_del(proxy, ELDBUS_PROXY_EVENT_DEL,
                                   _on_monitored_proxy_del, pending);

   if (eldbus_message_error_get(msg, &name, &error_msg))
     {
        /* don't print warnings for user-canceled calls */
        if (!eina_streq(name, "org.enlightenment.DBus.Canceled"))
          WRN("Error getting all properties of %s %s, error message: %s %s",
              proxy->obj->name, proxy->obj->path, name, error_msg);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "a{sv}", &dict))
     {
        char *txt;

        if (eldbus_message_arguments_get(msg, "s", &txt))
          WRN("Error getting data from properties getAll: %s", txt);
        return;
     }
   eldbus_message_iter_dict_iterate(dict, "sv", _property_iter, proxy);

   event.proxy = proxy;
   _eldbus_proxy_event_callback_call(proxy, ELDBUS_PROXY_EVENT_PROPERTY_LOADED,
                                     &event);
}

EAPI Eina_Bool
eldbus_proxy_properties_monitor(Eldbus_Proxy *proxy, Eina_Bool enable)
{
   Eldbus_Pending *pending;
   ELDBUS_PROXY_CHECK_RETVAL(proxy, EINA_FALSE);
   if (proxy->monitor_enabled == enable)
     return proxy->props ? !!eina_hash_population(proxy->props) : EINA_FALSE;

   proxy->monitor_enabled = enable;
   if (!enable)
     {
        Eldbus_Proxy_Context_Event *ce_prop_changed, *ce_prop_removed;
        ce_prop_changed = proxy->event_handlers + ELDBUS_PROXY_EVENT_PROPERTY_CHANGED;
        ce_prop_removed = proxy->event_handlers + ELDBUS_PROXY_EVENT_PROPERTY_REMOVED;

        if (!ce_prop_changed->list)
          {
             eina_hash_free(proxy->props);
             proxy->props = NULL;
          }
        if (!ce_prop_changed->list && !ce_prop_removed->list)
          {
             eldbus_signal_handler_unref(proxy->properties_changed);
             proxy->properties_changed = NULL;
          }
        return EINA_TRUE;
     }

   if (!proxy->props)
     proxy->props = eina_hash_string_superfast_new(_props_cache_free);

   pending = eldbus_proxy_property_get_all(proxy, _props_get_all, proxy);
   eldbus_proxy_event_callback_add(proxy, ELDBUS_PROXY_EVENT_DEL,
                                   _on_monitored_proxy_del, pending);

   if (proxy->properties_changed)
     return !!eina_hash_population(proxy->props);
   proxy->properties_changed =
            eldbus_proxy_properties_changed_callback_add(proxy,
                                                         _properties_changed,
                                                         proxy);
   return !!eina_hash_population(proxy->props);
}

EAPI Eina_Value *
eldbus_proxy_property_local_get(Eldbus_Proxy *proxy, const char *name)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy->props, NULL);
   return eina_hash_find(proxy->props, name);
}

EAPI const Eina_Hash *
eldbus_proxy_property_local_get_all(Eldbus_Proxy *proxy)
{
   ELDBUS_PROXY_CHECK_RETVAL(proxy, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(proxy->props, NULL);
   return proxy->props;
}
