#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "efl_net-connman.h"

/**
 * @brief Private data structure for Efl_Net_Control_Access_Point instances.
 *
 * This structure holds all the internal state and properties of a network
 * access point as managed by ConnMan. It includes DBus proxy information,
 * pending operations, signal handlers, and cached properties like SSID,
 * security settings, IP configurations, etc.
 */
typedef struct
{
   /* Eldbus_Proxy/Eldbus_Object keeps a list of pending calls, but
    * they are reference counted singletons and will only cancel
    * pending calls when everything is gone.  However we operate on
    * our private data, that may be gone before other refs. So
    * keep the pending list.
    */
   Eina_List *pending;
   Eina_List *signal_handlers;
   Eldbus_Proxy *proxy;
   Eina_Stringshare *path;
   Eina_Stringshare *name;
   Eina_Stringshare *techname;
   struct {
      Eina_List *name_servers;
      Eina_List *time_servers;
      Eina_List *domains;
      struct {
         Efl_Net_Control_Access_Point_Ipv4_Method method;
         Eina_Stringshare *address;
         Eina_Stringshare *netmask;
         Eina_Stringshare *gateway;
      } ipv4;
      struct {
         Efl_Net_Control_Access_Point_Ipv6_Method method;
         Eina_Stringshare *address;
         Eina_Stringshare *netmask;
         Eina_Stringshare *gateway;
         uint8_t prefix_length;
      } ipv6;
      struct {
         Efl_Net_Control_Access_Point_Proxy_Method method;
         Eina_Stringshare *url;
         Eina_List *servers;
         Eina_List *excludes;
      } proxy;
   } actual, configured;
   unsigned int priority;
   Efl_Net_Control_Access_Point_State state;
   Efl_Net_Control_Access_Point_Error error;
   Efl_Net_Control_Access_Point_Security security;
   uint8_t strength;
   Eina_Bool roaming;
   Eina_Bool auto_connect;
   Eina_Bool remembered;
   Eina_Bool immutable;
} Efl_Net_Control_Access_Point_Data;

#define MY_CLASS EFL_NET_CONTROL_ACCESS_POINT_CLASS

/**
 * @brief Callback for DBus SetProperty method calls.
 *
 * This function is invoked when a DBus SetProperty call, initiated to change
 * an access point's property, completes. It removes the pending operation
 * from the list and logs any errors.
 *
 * @param data The Eo object associated with the access point.
 * @param msg The Eldbus_Message reply.
 * @param pending The Eldbus_Pending object for the call.
 */
static void
_efl_net_control_access_point_property_set_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   pd->pending = eina_list_remove(pd->pending, pending);
   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not set property %p: %s=%s", o, err_name, err_msg);
        return;
     }
}

/**
 * @brief Sets a ConnMan property that is an array of strings.
 *
 * This helper function constructs and sends a DBus message to ConnMan's
 * SetProperty method for properties that are string arrays (e.g., Nameservers).
 *
 * @param o The Eo object for the access point.
 * @param pd The private data of the access point.
 * @param name The name of the property to set (e.g., "Nameservers.Configuration").
 * @param it An Eina_Iterator yielding const char * strings for the array.
 *           The iterator will be freed by this function.
 */
static void
_efl_net_control_access_point_property_set_string_array(const Eo *o, Efl_Net_Control_Access_Point_Data *pd, const char *name, Eina_Iterator *it)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *var, *array;
   Eldbus_Pending *p;
   const char *str;

   msg = eldbus_proxy_method_call_new(pd->proxy, "SetProperty");
   EINA_SAFETY_ON_NULL_GOTO(msg, error_msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   eldbus_message_iter_basic_append(msg_itr, 's', name);
   var = eldbus_message_iter_container_new(msg_itr, 'v', "as");

   eldbus_message_iter_arguments_append(var, "as", &array);
   EINA_ITERATOR_FOREACH(it, str)
     eldbus_message_iter_basic_append(array, 's', str);
   eldbus_message_iter_container_close(var, array);
   eldbus_message_iter_container_close(msg_itr, var);
   eina_iterator_free(it);

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_access_point_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property %s", name);
   return;

 error_send:
   eldbus_message_unref(msg);
 error_msg:
   eina_iterator_free(it);
}

/**
 * @brief Sets a ConnMan property with a variant value.
 *
 * This is a generic helper function to set a ConnMan property using DBus.
 * It constructs a SetProperty call with the property name and a variant
 * value whose type and content are specified by @p signature and variadic
 * arguments.
 *
 * @param o The Eo object for the access point.
 * @param pd The private data of the access point.
 * @param name The name of the property to set (e.g., "AutoConnect").
 * @param signature The DBus signature of the variant's content (e.g., "b" for boolean).
 * @param ... The value(s) for the property, matching the @p signature.
 */
static void
_efl_net_control_access_point_property_set(const Eo *o, Efl_Net_Control_Access_Point_Data *pd, const char *name, const char *signature, ...)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *var;
   Eldbus_Pending *p;
   va_list ap;

   msg = eldbus_proxy_method_call_new(pd->proxy, "SetProperty");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   eldbus_message_iter_basic_append(msg_itr, 's', name);
   var = eldbus_message_iter_container_new(msg_itr, 'v', signature);

   va_start(ap, signature);
   eldbus_message_iter_arguments_vappend(var, signature, ap);
   va_end(ap);
   eldbus_message_iter_container_close(msg_itr, var);

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_access_point_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property %s", name);
   return;

 error_send:
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_control_access_point_efl_object_destructor(Eo *o, Efl_Net_Control_Access_Point_Data *pd)
{
   Eldbus_Pending *p;
   Eldbus_Signal_Handler *sh;
   const char *str;

   EINA_LIST_FREE(pd->pending, p)
     eldbus_pending_cancel(p);

   EINA_LIST_FREE(pd->signal_handlers, sh)
     eldbus_signal_handler_del(sh);

   if (pd->proxy)
     {
        Eldbus_Object *obj = eldbus_proxy_object_get(pd->proxy);
        eldbus_proxy_unref(pd->proxy);
        pd->proxy = NULL;
        eldbus_object_unref(obj);
     }

   efl_destructor(efl_super(o, MY_CLASS));
   eina_stringshare_replace(&pd->path, NULL);
   eina_stringshare_replace(&pd->name, NULL);

   /* actual values */
   EINA_LIST_FREE(pd->actual.name_servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->actual.time_servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->actual.domains, str) eina_stringshare_del(str);

   eina_stringshare_replace(&pd->actual.ipv4.address, NULL);
   eina_stringshare_replace(&pd->actual.ipv4.netmask, NULL);
   eina_stringshare_replace(&pd->actual.ipv4.gateway, NULL);

   eina_stringshare_replace(&pd->actual.ipv6.address, NULL);
   eina_stringshare_replace(&pd->actual.ipv6.netmask, NULL);
   eina_stringshare_replace(&pd->actual.ipv6.gateway, NULL);

   eina_stringshare_replace(&pd->actual.proxy.url, NULL);
   EINA_LIST_FREE(pd->actual.proxy.servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->actual.proxy.excludes, str) eina_stringshare_del(str);

   /* configured values */
   EINA_LIST_FREE(pd->configured.name_servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->configured.time_servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->configured.domains, str) eina_stringshare_del(str);

   eina_stringshare_replace(&pd->configured.ipv4.address, NULL);
   eina_stringshare_replace(&pd->configured.ipv4.netmask, NULL);
   eina_stringshare_replace(&pd->configured.ipv4.gateway, NULL);

   eina_stringshare_replace(&pd->configured.ipv6.address, NULL);
   eina_stringshare_replace(&pd->configured.ipv6.netmask, NULL);
   eina_stringshare_replace(&pd->configured.ipv6.gateway, NULL);

   eina_stringshare_replace(&pd->configured.proxy.url, NULL);
   EINA_LIST_FREE(pd->configured.proxy.servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->configured.proxy.excludes, str) eina_stringshare_del(str);
}

EOLIAN static Efl_Net_Control_Access_Point_State
_efl_net_control_access_point_state_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->state;
}

EOLIAN static Efl_Net_Control_Access_Point_Error
_efl_net_control_access_point_error_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->error;
}

EOLIAN static const char *
_efl_net_control_access_point_ssid_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->name;
}

/**
 * @brief Callback for DBus MoveBefore/MoveAfter method calls (priority change).
 *
 * This function is invoked when a DBus call to reorder services (change priority)
 * completes. It removes the pending operation and reloads access points as ConnMan
 * might not emit ServicesChanged signal reliably.
 *
 * @param data The Eo object associated with the access point.
 * @param msg The Eldbus_Message reply.
 * @param pending The Eldbus_Pending object for the call.
 */
static void
_efl_net_control_access_point_priority_set_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   pd->pending = eina_list_remove(pd->pending, pending);
   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not reorder %p: %s=%s", o, err_name, err_msg);
        return;
     }

   DBG("finished reordering %p", o);
   /* NOTE: it seems connman is not emiting ServicesChanged as expected */
   efl_net_connman_control_access_points_reload(efl_parent_get(o));
}

EOLIAN static void
_efl_net_control_access_point_priority_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, unsigned int priority)
{
   Efl_Net_Control_Access_Point_Data *other_pd;
   Eldbus_Pending *p;
   Eina_Iterator *it;
   Eo *ap = NULL, *last_ap = NULL, *sibling = NULL;
   int direction = 0;

   if (pd->priority == priority)
     {
        DBG("same priority %u, nothing to do for %s", priority, pd->name);
        return;
     }
   else if (!pd->remembered)
     {
        ERR("cannot change priority of non-remembered access point '%s'", pd->name);
        return;
     }

   it = efl_net_control_manager_access_points_get(efl_parent_get(o));
   EINA_ITERATOR_FOREACH(it, ap)
     {
        unsigned other_prio;
        if (ap == o) continue;
        else if (!efl_net_control_access_point_remembered_get(ap)) break;
        else if (priority == 0)
          {
             sibling = ap;
             direction = -1;
             break;
          }

        other_prio = efl_net_control_access_point_priority_get(ap);
        if (priority < other_prio) break;
        last_ap = ap;
     }
   eina_iterator_free(it);

   if ((!sibling) && (last_ap))
     {
        sibling = last_ap;
        if (priority <= efl_net_control_access_point_priority_get(last_ap))
          direction = -1;
        else
          direction = 1;
     }

   if (!sibling)
     {
        DBG("nothing to reorder priority %u for %s", priority, pd->name);
        return;
     }

   other_pd = efl_data_scope_get(sibling, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(other_pd);

   p = eldbus_proxy_call(pd->proxy,
                         (direction < 0) ? "MoveBefore" : "MoveAfter",
                         _efl_net_control_access_point_priority_set_cb, o, DEFAULT_TIMEOUT,
                         "o", other_pd->path);
   EINA_SAFETY_ON_NULL_RETURN(p);
   pd->pending = eina_list_append(pd->pending, p);
   DBG("Moving %s (%s) %s %s (%s)",
       pd->name, pd->path,
       (direction < 0) ? "before" : "after",
       other_pd->name, other_pd->path);
   return;
}

EOLIAN static unsigned int
_efl_net_control_access_point_priority_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->priority;
}

EOLIAN static Efl_Net_Control_Technology *
_efl_net_control_access_point_technology_get(const Eo *o, Efl_Net_Control_Access_Point_Data *pd)
{
   return efl_net_connman_control_find_technology_by_type(efl_parent_get(o), pd->techname);
}

EOLIAN static uint8_t
_efl_net_control_access_point_strength_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->strength;
}

EOLIAN static Eina_Bool
_efl_net_control_access_point_roaming_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->roaming;
}

EOLIAN static void
_efl_net_control_access_point_auto_connect_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Eina_Bool auto_connect)
{
   _efl_net_control_access_point_property_set(o, pd, "AutoConnect", "b", auto_connect);
}

EOLIAN static Eina_Bool
_efl_net_control_access_point_auto_connect_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->auto_connect;
}

EOLIAN static Eina_Bool
_efl_net_control_access_point_remembered_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->remembered;
}

EOLIAN static Eina_Bool
_efl_net_control_access_point_immutable_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->immutable;
}

EOLIAN static Efl_Net_Control_Access_Point_Security
_efl_net_control_access_point_security_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return pd->security;
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_name_servers_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->actual.name_servers);
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_time_servers_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->actual.time_servers);
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_domains_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->actual.domains);
}

EOLIAN static void
_efl_net_control_access_point_ipv4_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv4_Method *method, const char **address, const char **netmask, const char **gateway)
{
   if (method) *method = pd->actual.ipv4.method;
   if (address) *address = pd->actual.ipv4.address;
   if (netmask) *netmask = pd->actual.ipv4.netmask;
   if (gateway) *gateway = pd->actual.ipv4.gateway;
}

EOLIAN static void
_efl_net_control_access_point_ipv6_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv6_Method *method, const char **address, uint8_t *prefix_length, const char **netmask, const char **gateway)
{
   if (method) *method = pd->actual.ipv6.method;
   if (address) *address = pd->actual.ipv6.address;
   if (netmask) *netmask = pd->actual.ipv6.netmask;
   if (gateway) *gateway = pd->actual.ipv6.gateway;
   if (prefix_length) *prefix_length = pd->actual.ipv6.prefix_length;
}

EOLIAN static void
_efl_net_control_access_point_proxy_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Proxy_Method *method, const char **url, Eina_Iterator **servers, Eina_Iterator **excludes)
{
   if (method) *method = pd->actual.proxy.method;
   if (url) *url = pd->actual.proxy.url;
   if (servers) *servers = eina_list_iterator_new(pd->actual.proxy.servers);
   if (excludes) *excludes = eina_list_iterator_new(pd->actual.proxy.excludes);
}

EOLIAN static void
_efl_net_control_access_point_configuration_name_servers_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Eina_Iterator *name_servers)
{
   _efl_net_control_access_point_property_set_string_array(o, pd, "Nameservers.Configuration", name_servers);
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_name_servers_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->configured.name_servers);
}

EOLIAN static void
_efl_net_control_access_point_configuration_time_servers_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Eina_Iterator *time_servers)
{
   _efl_net_control_access_point_property_set_string_array(o, pd, "Timeservers.Configuration", time_servers);
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_time_servers_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->configured.time_servers);
}

EOLIAN static void
_efl_net_control_access_point_configuration_domains_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Eina_Iterator *domains)
{
   _efl_net_control_access_point_property_set_string_array(o, pd, "Domains.Configuration", domains);
}

EOLIAN static Eina_Iterator *
_efl_net_control_access_point_configuration_domains_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd)
{
   return eina_list_iterator_new(pd->configured.domains);
}

/**
 * @brief Appends a dictionary entry (string to variant) to a DBus message iterator.
 *
 * This helper is used to construct `a{sv}` (array of dictionary entries)
 * type messages for ConnMan properties like IPv4.Configuration.
 *
 * @param array The DBus message iterator for the array `a{sv}`.
 * @param name The string key of the dictionary entry.
 * @param signature The DBus signature of the variant value (e.g., "s" for string).
 * @param ... The value for the variant, matching the @p signature.
 */
static void
_append_dict_entry(Eldbus_Message_Iter *array, const char *name, const char *signature, ...)
{
   Eldbus_Message_Iter *entry, *var;
   va_list ap;

   if (!eldbus_message_iter_arguments_append(array, "{sv}", &entry))
     {
        ERR("could not append dict entry");
        return;
     }

   eldbus_message_iter_basic_append(entry, 's', name);
   var = eldbus_message_iter_container_new(entry, 'v', signature);

   va_start(ap, signature);
   eldbus_message_iter_arguments_vappend(var, signature, ap);
   va_end(ap);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(array, entry);
}

/**
 * @brief Appends a dictionary entry (string to array of strings) to a DBus message iterator.
 *
 * This helper is used for `a{sv}` dictionary entries where the variant `v`
 * itself contains an array of strings `as`. Example: Proxy.Configuration's "Servers"
 * or "Excludes".
 *
 * @param array The DBus message iterator for the array `a{sv}`.
 * @param name The string key of the dictionary entry.
 * @param it An Eina_Iterator yielding const char * strings for the `as` value.
 *           The iterator will be freed by this function if not NULL.
 */
static void
_append_dict_entry_string_array(Eldbus_Message_Iter *array, const char *name, Eina_Iterator *it)
{
   Eldbus_Message_Iter *entry, *var, *sub;
   const char *str;

   if (!eldbus_message_iter_arguments_append(array, "{sv}", &entry))
     {
        ERR("could not append dict entry");
        return;
     }

   eldbus_message_iter_basic_append(entry, 's', name);
   var = eldbus_message_iter_container_new(entry, 'v', "as");

   eldbus_message_iter_arguments_append(var, "as", &sub);
   EINA_ITERATOR_FOREACH(it, str)
     eldbus_message_iter_basic_append(sub, 's', str);

   eldbus_message_iter_container_close(var, sub);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(array, entry);
}

EOLIAN static void
_efl_net_control_access_point_configuration_ipv4_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv4_Method method, const char *address, const char *netmask, const char *gateway)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *array, *var;
   Eldbus_Pending *p;

   if (method == EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_UNSET)
     {
        ERR("Invalid IPv4 Method (%d) EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_UNSET\n", method);
        return;
     }

   msg = eldbus_proxy_method_call_new(pd->proxy, "SetProperty");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   eldbus_message_iter_basic_append(msg_itr, 's', "IPv4.Configuration");
   var = eldbus_message_iter_container_new(msg_itr, 'v', "a{sv}");
   eldbus_message_iter_arguments_append(var, "a{sv}", &array);

   switch (method)
     {
      case EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_OFF:
         _append_dict_entry(array, "Method", "s", "off");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_DHCP:
         _append_dict_entry(array, "Method", "s", "dhcp");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_MANUAL:
         _append_dict_entry(array, "Method", "s", "manual");
         if (address)
           _append_dict_entry(array, "Address", "s", address);
         if (netmask)
           _append_dict_entry(array, "Netmask", "s", netmask);
         if (gateway)
           _append_dict_entry(array, "Gateway", "s", gateway);
         break;
      default:
         break;
     }

   eldbus_message_iter_container_close(var, array);
   eldbus_message_iter_container_close(msg_itr, var);

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_access_point_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property IPv4");
   return;

 error_send:
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_control_access_point_configuration_ipv4_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv4_Method *method, const char **address, const char **netmask, const char **gateway)
{
   if (method) *method = pd->configured.ipv4.method;
   if (address) *address = pd->configured.ipv4.address;
   if (netmask) *netmask = pd->configured.ipv4.netmask;
   if (gateway) *gateway = pd->configured.ipv4.gateway;
}

EOLIAN static void
_efl_net_control_access_point_configuration_ipv6_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv6_Method method, const char *address, uint8_t prefix_length, const char *netmask, const char *gateway)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *array, *var;
   Eldbus_Pending *p;

   if (method == EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET)
     {
        ERR("Invalid IPv6 Method (%d) EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET\n", method);
        return;
     }
   else if (method == EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_FIXED)
     {
        ERR("Invalid IPv6 Method (%d) EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_FIXED\n", method);
        return;
     }
   else if (method == EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_TUNNEL6TO4)
     {
        ERR("Invalid IPv6 Method (%d) EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_TUNNEL6TO4\n", method);
        return;
     }

   msg = eldbus_proxy_method_call_new(pd->proxy, "SetProperty");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   eldbus_message_iter_basic_append(msg_itr, 's', "IPv6.Configuration");
   var = eldbus_message_iter_container_new(msg_itr, 'v', "a{sv}");
   eldbus_message_iter_arguments_append(var, "a{sv}", &array);

   switch (method)
     {
      case EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_OFF:
         _append_dict_entry(array, "Method", "s", "off");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_MANUAL:
         _append_dict_entry(array, "Method", "s", "manual");
         if (address)
           _append_dict_entry(array, "Address", "s", address);
         if (netmask)
           _append_dict_entry(array, "Netmask", "s", netmask);
         if (gateway)
           _append_dict_entry(array, "Gateway", "s", gateway);
         if (prefix_length)
           _append_dict_entry(array, "PrefixLength", "y", prefix_length);
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE:
         _append_dict_entry(array, "Method", "s", "auto");
         _append_dict_entry(array, "Privacy", "s", "disabled");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_PUBLIC:
         _append_dict_entry(array, "Method", "s", "auto");
         _append_dict_entry(array, "Privacy", "s", "enabled");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_TEMPORARY:
         _append_dict_entry(array, "Method", "s", "auto");
         _append_dict_entry(array, "Privacy", "s", "preferred");
         break;
      default:
         break;
     }

   eldbus_message_iter_container_close(var, array);
   eldbus_message_iter_container_close(msg_itr, var);

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_access_point_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property IPv6");
   return;

 error_send:
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_control_access_point_configuration_ipv6_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Ipv6_Method *method, const char **address, uint8_t *prefix_length, const char **netmask, const char **gateway)
{
   if (method) *method = pd->configured.ipv6.method;
   if (address) *address = pd->configured.ipv6.address;
   if (netmask) *netmask = pd->configured.ipv6.netmask;
   if (gateway) *gateway = pd->configured.ipv6.gateway;
   if (prefix_length) *prefix_length = pd->configured.ipv6.prefix_length;
}

EOLIAN static void
_efl_net_control_access_point_configuration_proxy_set(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Proxy_Method method, const char *url, Eina_Iterator *servers, Eina_Iterator *excludes)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *array, *var;
   Eldbus_Pending *p;

   if (method == EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_UNSET)
     {
        ERR("Invalid Proxy Method (%d) EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_UNSET\n", method);
        if (servers) eina_iterator_free(servers);
        if (excludes) eina_iterator_free(excludes);
        return;
     }

   msg = eldbus_proxy_method_call_new(pd->proxy, "SetProperty");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   eldbus_message_iter_basic_append(msg_itr, 's', "Proxy.Configuration");
   var = eldbus_message_iter_container_new(msg_itr, 'v', "a{sv}");
   eldbus_message_iter_arguments_append(var, "a{sv}", &array);

   switch (method)
     {
      case EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_OFF:
         _append_dict_entry(array, "Method", "s", "direct");
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_AUTO:
         _append_dict_entry(array, "Method", "s", "auto");
         if (url)
           _append_dict_entry(array, "URL", "s", url);
         break;
      case EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_MANUAL:
         _append_dict_entry(array, "Method", "s", "manual");
         if (servers)
           _append_dict_entry_string_array(array, "Servers", servers);
         if (excludes)
           _append_dict_entry_string_array(array, "Excludes", excludes);
         break;
      default:
         break;
     }

   eldbus_message_iter_container_close(var, array);
   eldbus_message_iter_container_close(msg_itr, var);

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_access_point_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property Proxy");
   if (servers) eina_iterator_free(servers);
   if (excludes) eina_iterator_free(excludes);
   return;

 error_send:
   if (servers) eina_iterator_free(servers);
   if (excludes) eina_iterator_free(excludes);
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_control_access_point_configuration_proxy_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Efl_Net_Control_Access_Point_Proxy_Method *method, const char **url, Eina_Iterator **servers, Eina_Iterator **excludes)
{
   if (method) *method = pd->configured.proxy.method;
   if (url) *url = pd->configured.proxy.url;
   if (servers) *servers = eina_list_iterator_new(pd->configured.proxy.servers);
   if (excludes) *excludes = eina_list_iterator_new(pd->configured.proxy.excludes);
}

/**
 * @brief Callback for the DBus Connect method call.
 *
 * This function is invoked when ConnMan's Connect method call completes.
 * It resolves or rejects the associated Eina_Promise based on the outcome
 * of the connection attempt.
 *
 * @param data The Eina_Promise to be resolved or rejected.
 * @param msg The Eldbus_Message reply.
 * @param pending The Eldbus_Pending object for the call, used to retrieve private data.
 */
static void
_efl_net_control_access_point_connect_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eina_Promise *promise = data;
   Efl_Net_Control_Access_Point_Data *pd = eldbus_pending_data_get(pending, ".object");
   const char *err_name, *err_msg;

   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        Eina_Error err = EINVAL;

        if (strcmp(err_name, "net.connman.Error.InProgress") == 0)
          err = EINPROGRESS;
        else if (strcmp(err_name, "net.connman.Error.AlreadyConnected") == 0)
          err = EALREADY;
        WRN("Could not Connect: %s=%s", err_name, err_msg);

        eina_promise_reject(promise, err);
        return;
     }

   eina_promise_resolve(promise, EINA_VALUE_EMPTY);
}

/**
 * @brief Error callback for the connect future, handling cancellation.
 *
 * If the future associated with a connect operation is cancelled (e.g., object
 * destroyed before connection completes), this function is called. It cancels
 * the pending DBus "Connect" call.
 *
 * @param consumer The Efl_Loop_Consumer (unused).
 * @param data The Eldbus_Pending object for the "Connect" call.
 * @param error The error code, ECANCELED if the future was cancelled.
 * @return An Eina_Value containing the error.
 */
static Eina_Value
_efl_net_control_access_point_connect_promise_del(Efl_Loop_Consumer *consumer EINA_UNUSED, void *data, Eina_Error error)
{
   if (error == ECANCELED)
     {
        Eldbus_Pending *p = data;

        DBG("cancel pending connect %p", p);
        eldbus_pending_cancel(p);
     }

   return eina_value_error_init(error);
}

/**
 * @brief Cleanup callback for the connect future.
 *
 * This function is called when the future associated with a connect operation
 * is cleaned up (e.g., after resolution, rejection, or cancellation). It removes
 * the pending DBus operation from the access point's list of pending operations.
 *
 * @param o The Eo object (unused).
 * @param data The Eldbus_Pending object for the "Connect" call.
 * @param dead_future The Eina_Future that has completed (unused).
 */
static void
_efl_net_control_access_point_connect_promise_clean(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Net_Control_Access_Point_Data *pd;
   Eldbus_Pending *p = data;

   pd = eldbus_pending_data_get(p, ".object");

   pd->pending = eina_list_remove(pd->pending, p);
}

EOLIAN static Eina_Future *
_efl_net_control_access_point_connect(Eo *o, Efl_Net_Control_Access_Point_Data *pd)
{
   Eldbus_Pending *p;
   Eina_Promise *promise;
   Eina_Future *f = NULL;

   promise = efl_loop_promise_new(o);
   EINA_SAFETY_ON_NULL_RETURN_VAL(promise, NULL);

   f = eina_future_new(promise);

   p = eldbus_proxy_call(pd->proxy, "Connect",
                         _efl_net_control_access_point_connect_cb, promise, -1.0, "");
   EINA_SAFETY_ON_NULL_GOTO(p, error_dbus);

   pd->pending = eina_list_append(pd->pending, p);
   eldbus_pending_data_set(p, ".object", pd);

   return efl_future_then(o, f,
                          .data = p,
                          .error = _efl_net_control_access_point_connect_promise_del,
                          .free = _efl_net_control_access_point_connect_promise_clean);

 error_dbus:
   eina_promise_reject(promise, ENOSYS);
   return efl_future_then(o, f);
}

/**
 * @brief Callback for the DBus Disconnect method call.
 *
 * This function is invoked when ConnMan's Disconnect method call completes.
 * It removes the pending operation from the list and logs any errors.
 *
 * @param data The Eo object associated with the access point.
 * @param msg The Eldbus_Message reply.
 * @param pending The Eldbus_Pending object for the call.
 */
static void
_efl_net_control_access_point_disconnect_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   pd->pending = eina_list_remove(pd->pending, pending);
   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not disconnect %p: %s=%s", o, err_name, err_msg);
        return;
     }
}

EOLIAN static void
_efl_net_control_access_point_disconnect(Eo *o, Efl_Net_Control_Access_Point_Data *pd)
{
   Eldbus_Pending *p;
   p = eldbus_proxy_call(pd->proxy, "Disconnect",
                         _efl_net_control_access_point_disconnect_cb, o, DEFAULT_TIMEOUT, "");
   EINA_SAFETY_ON_NULL_RETURN(p);
   pd->pending = eina_list_append(pd->pending, p);
}

/**
 * @brief Callback for the DBus Remove (Forget) method call.
 *
 * This function is invoked when ConnMan's Remove method call (to forget a service)
 * completes. It removes the pending operation from the list and logs any errors.
 *
 * @param data The Eo object associated with the access point.
 * @param msg The Eldbus_Message reply.
 * @param pending The Eldbus_Pending object for the call.
 */
static void
_efl_net_control_access_point_forget_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   pd->pending = eina_list_remove(pd->pending, pending);
   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not forget %p: %s=%s", o, err_name, err_msg);
        return;
     }
}

EOLIAN static void
_efl_net_control_access_point_forget(Eo *o, Efl_Net_Control_Access_Point_Data *pd)
{
   Eldbus_Pending *p;
   p = eldbus_proxy_call(pd->proxy, "Remove",
                         _efl_net_control_access_point_forget_cb, o, DEFAULT_TIMEOUT, "");
   EINA_SAFETY_ON_NULL_RETURN(p);
   pd->pending = eina_list_append(pd->pending, p);
}

static void
_efl_net_control_access_point_property_name_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   const char *name;

   if (!eldbus_message_iter_arguments_get(value, "s", &name))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (!eina_stringshare_replace(&pd->name, name)) return;
   DBG("name=%s", name);
}

static void
_efl_net_control_access_point_property_state_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   const char *str;
   const struct {
      Efl_Net_Control_Access_Point_State val;
      const char *str;
   } *itr, map[] = {
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_IDLE, "idle"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_ASSOCIATION, "association"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_CONFIGURATION, "configuration"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_LOCAL, "ready"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_ONLINE, "online"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_DISCONNECT, "disconnect"},
     {EFL_NET_CONTROL_ACCESS_POINT_STATE_FAILURE, "failure"},
     { }
   };

   if (!eldbus_message_iter_arguments_get(value, "s", &str))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   for (itr = map; itr->str != NULL; itr++)
     if (strcmp(itr->str, str) == 0) break;

   if (!itr->str)
     {
        ERR("Unknown state '%s'", str);
        return;
     }

   if (pd->state == itr->val) return;
   pd->state = itr->val;
   DBG("state=%d (%s)", pd->state, str);
}

static void
_efl_net_control_access_point_property_error_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   const char *str;
   const struct {
      Efl_Net_Control_Access_Point_Error val;
      const char *str;
   } *itr, map[] = {
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_NONE, "none"},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_NONE, ""},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_OUT_OF_RANGE, "range"},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_PIN_MISSING, "missing"},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_DHCP_FAILED, "failed"},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_CONNECT_FAILED, "failed"},
     {EFL_NET_CONTROL_ACCESS_POINT_ERROR_LOGIN_FAILED, "failed"},
     { }
   };

   if (!eldbus_message_iter_arguments_get(value, "s", &str))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   for (itr = map; itr->str != NULL; itr++)
     if (strcmp(itr->str, str) == 0) break;

   if (!itr->str)
     {
        ERR("Unknown error '%s'", str);
        return;
     }

   if (pd->error == itr->val) return;
   pd->error = itr->val;
   DBG("error=%d (%s)", pd->error, str);
}

static void
_efl_net_control_access_point_property_type_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   const char *name;

   if (!eldbus_message_iter_arguments_get(value, "s", &name))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (!eina_stringshare_replace(&pd->techname, name)) return;
   DBG("technology name=%s", name);
}

static void
_efl_net_control_access_point_property_security_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   const char *str;
   Eldbus_Message_Iter *array;
   const struct {
      Efl_Net_Control_Access_Point_Security val;
      const char *str;
   } *itr, map[] = {
     {EFL_NET_CONTROL_ACCESS_POINT_SECURITY_NONE, "none"},
     {EFL_NET_CONTROL_ACCESS_POINT_SECURITY_WEP, "wep"},
     {EFL_NET_CONTROL_ACCESS_POINT_SECURITY_PSK, "psk"},
     {EFL_NET_CONTROL_ACCESS_POINT_SECURITY_IEEE802_1X, "ieee8021x"},
     { }
   };
   Efl_Net_Control_Access_Point_Security security = 0;

   if (!eldbus_message_iter_arguments_get(value, "as", &array))
     {
        ERR("Expected array of string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   while (eldbus_message_iter_get_and_next(array, 's', &str))
     {
        for (itr = map; itr->str != NULL; itr++)
          if (strcmp(itr->str, str) == 0) break;

        if (!itr->str)
          {
             ERR("Unknown security '%s'", str);
             continue;
          }

        security |= itr->val;
     }

   if (pd->security == security) return;
   pd->security = security;
   DBG("security=%#x", pd->security);
}

static void
_efl_net_control_access_point_property_strength_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   uint8_t strength;

   if (!eldbus_message_iter_arguments_get(value, "y", &strength))
     {
        ERR("Expected unsigned byte, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->strength == strength) return;
   pd->strength = strength;
   DBG("strength=%hhu", pd->strength);
}

static void
_efl_net_control_access_point_property_remembered_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool remembered;

   if (!eldbus_message_iter_arguments_get(value, "b", &remembered))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->remembered == remembered) return;
   pd->remembered = remembered;
   if (!remembered)
     {
        /* force this flag to be cleared */
        pd->auto_connect = EINA_FALSE;
     }
   DBG("remembered=%hhu", remembered);
}

static void
_efl_net_control_access_point_property_immutable_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool immutable;

   if (!eldbus_message_iter_arguments_get(value, "b", &immutable))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->immutable == immutable) return;
   pd->immutable = immutable;
   DBG("immutable=%hhu", immutable);
}

static void
_efl_net_control_access_point_property_auto_connect_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool auto_connect;

   if (!eldbus_message_iter_arguments_get(value, "b", &auto_connect))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->auto_connect == auto_connect) return;
   pd->auto_connect = auto_connect;
   DBG("auto_connect=%hhu", auto_connect);
}

static void
_efl_net_control_access_point_property_roaming_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool roaming;

   if (!eldbus_message_iter_arguments_get(value, "b", &roaming))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->roaming == roaming) return;
   pd->roaming = roaming;
   DBG("roaming=%hhu", roaming);
}

static void
_efl_net_control_access_point_list_updated(const char *name, Eina_List **p_list, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array;
   Eina_List *old_list;
   const char *str;

   if (!eldbus_message_iter_arguments_get(value, "as", &array))
     {
        ERR("Expected array of strings for %s, got %s", name, eldbus_message_iter_signature_get(value));
        return;
     }

   old_list = *p_list;
   *p_list = NULL;
   while (eldbus_message_iter_get_and_next(array, 's', &str))
     *p_list = eina_list_append(*p_list, eina_stringshare_add(str));

   EINA_LIST_FREE(old_list, str) eina_stringshare_del(str);
}

/* Actual Values */

/**
 * @brief Handles changes to the "Nameservers" (actual) property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_actual_name_servers_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("name_servers", &pd->actual.name_servers, value);
}

/**
 * @brief Handles changes to the "Timeservers" (actual) property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_actual_time_servers_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("time_servers", &pd->actual.time_servers, value);
}

/**
 * @brief Handles changes to the "Domains" (actual) property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_actual_domains_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("domains", &pd->actual.domains, value);
}

/**
 * @brief Handles changes to the "IPv4" (actual) property.
 *
 * Parses a DBus dictionary `a{sv}` representing the IPv4 settings
 * (Method, Address, Netmask, Gateway) and updates the `pd->actual.ipv4` struct.
 * Example `a{sv}` structure:
 *   { "Method": Variant("dhcp"), "Address": Variant("192.168.1.100"), ... }
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_actual_ipv4_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->actual.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_UNSET;
   eina_stringshare_replace(&pd->actual.ipv4.address, NULL);
   eina_stringshare_replace(&pd->actual.ipv4.netmask, NULL);
   eina_stringshare_replace(&pd->actual.ipv4.gateway, NULL);

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name, *str;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "dhcp") == 0)
               pd->actual.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_DHCP;
             else if (strcmp(str, "manual") == 0)
               pd->actual.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_MANUAL;
             else if (strcmp(str, "off") == 0)
               pd->actual.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_OFF;
             else
               WRN("Unexpected IPV4 Method value '%s'", str);
          }
        else if (strcmp(name, "Address") == 0)
          eina_stringshare_replace(&pd->actual.ipv4.address, str);
        else if (strcmp(name, "Netmask") == 0)
          eina_stringshare_replace(&pd->actual.ipv4.netmask, str);
        else if (strcmp(name, "Gateway") == 0)
          eina_stringshare_replace(&pd->actual.ipv4.gateway, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/**
 * @brief Handles changes to the "IPv6" (actual) property.
 *
 * Parses a DBus dictionary `a{sv}` representing the IPv6 settings
 * (Method, Address, PrefixLength, Netmask, Gateway, Privacy) and updates
 * the `pd->actual.ipv6` struct.
 * Example `a{sv}` structure:
 *   { "Method": Variant("auto"), "Address": Variant("fe80::1"), "PrefixLength": Variant(byte 64), ... }
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_actual_ipv6_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET;
   eina_stringshare_replace(&pd->actual.ipv6.address, NULL);
   eina_stringshare_replace(&pd->actual.ipv6.netmask, NULL);
   eina_stringshare_replace(&pd->actual.ipv6.gateway, NULL);
   pd->actual.ipv6.prefix_length = 0;

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name, *str;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(name, "PrefixLength") == 0)
          {
             if (!eldbus_message_iter_arguments_get(var, "y", &pd->actual.ipv6.prefix_length))
               ERR("Expected unsigned byte value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "off") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_OFF;
             else if (strcmp(str, "fixed") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_FIXED;
             else if (strcmp(str, "manual") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_MANUAL;
             else if (strcmp(str, "6to4") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_TUNNEL6TO4;
             else if (strcmp(str, "auto") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE;
             else
               WRN("Unexpected IPV6 Method value '%s'", str);
          }
        else if (strcmp(name, "Privacy") == 0)
          {
             if ((pd->actual.ipv6.method != EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE) &&
                 (pd->actual.ipv6.method != EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET))
               {
                  DBG("Skip privacy %s, method already set to %d", str, pd->actual.ipv6.method);
                  continue;
               }
             if (strcmp(str, "disabled") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE;
             else if (strcmp(str, "enabled") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_PUBLIC;
             else if (strcmp(str, "preferred") == 0)
               pd->actual.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_TEMPORARY;
             else
               WRN("Unexpected IPV6 Privacy value '%s'", str);
          }
        else if (strcmp(name, "Address") == 0)
          eina_stringshare_replace(&pd->actual.ipv6.address, str);
        else if (strcmp(name, "Netmask") == 0)
          eina_stringshare_replace(&pd->actual.ipv6.netmask, str);
        else if (strcmp(name, "Gateway") == 0)
          eina_stringshare_replace(&pd->actual.ipv6.gateway, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/**
 * @brief Handles changes to the "Proxy" (actual) property.
 *
 * Parses a DBus dictionary `a{sv}` representing the proxy settings
 * (Method, URL, Servers, Excludes) and updates the `pd->actual.proxy` struct.
 * "Servers" and "Excludes" are arrays of strings.
 * Example `a{sv}` structure:
 *   { "Method": Variant("manual"), "URL": Variant("http://proxy.example.com/pac.js"),
 *     "Servers": Variant(["http://server1:8080", "socks://server2:1080"]), ... }
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_actual_proxy_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;
   const char *str;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->actual.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_UNSET;
   eina_stringshare_replace(&pd->actual.proxy.url, NULL);
   EINA_LIST_FREE(pd->actual.proxy.servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->actual.proxy.excludes, str) eina_stringshare_del(str);

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(name, "Servers") == 0)
          {
             _efl_net_control_access_point_list_updated("Proxy Servers", &pd->actual.proxy.servers, var);
             continue;
          }
        else if (strcmp(name, "Excludes") == 0)
          {
             _efl_net_control_access_point_list_updated("Proxy Excludes", &pd->actual.proxy.excludes, var);
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "auto") == 0)
               pd->actual.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_AUTO;
             else if (strcmp(str, "manual") == 0)
               pd->actual.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_MANUAL;
             else if (strcmp(str, "direct") == 0)
               pd->actual.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_OFF;
             else
               WRN("Unexpected PROXY Method value '%s'", str);
          }
        else if (strcmp(name, "URL") == 0)
          eina_stringshare_replace(&pd->actual.proxy.url, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/* Configured Values */

/**
 * @brief Handles changes to the "Nameservers.Configuration" property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_configured_name_servers_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("name_servers", &pd->configured.name_servers, value);
}

/**
 * @brief Handles changes to the "Timeservers.Configuration" property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_configured_time_servers_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("time_servers", &pd->configured.time_servers, value);
}

/**
 * @brief Handles changes to the "Domains.Configuration" property.
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value.
 */
static void
_efl_net_control_access_point_property_configured_domains_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   _efl_net_control_access_point_list_updated("domains", &pd->configured.domains, value);
}

/**
 * @brief Handles changes to the "IPv4.Configuration" property.
 *
 * Parses a DBus dictionary `a{sv}` representing the configured IPv4 settings
 * and updates the `pd->configured.ipv4` struct.
 * See _efl_net_control_access_point_property_actual_ipv4_changed() for structure.
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_configured_ipv4_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->configured.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_UNSET;
   eina_stringshare_replace(&pd->configured.ipv4.address, NULL);
   eina_stringshare_replace(&pd->configured.ipv4.netmask, NULL);
   eina_stringshare_replace(&pd->configured.ipv4.gateway, NULL);

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name, *str;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "dhcp") == 0)
               pd->configured.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_DHCP;
             else if (strcmp(str, "manual") == 0)
               pd->configured.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_MANUAL;
             else if (strcmp(str, "off") == 0)
               pd->configured.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_OFF;
             else
               WRN("Unexpected IPV4 Method value '%s'", str);
          }
        else if (strcmp(name, "Address") == 0)
          eina_stringshare_replace(&pd->configured.ipv4.address, str);
        else if (strcmp(name, "Netmask") == 0)
          eina_stringshare_replace(&pd->configured.ipv4.netmask, str);
        else if (strcmp(name, "Gateway") == 0)
          eina_stringshare_replace(&pd->configured.ipv4.gateway, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/**
 * @brief Handles changes to the "IPv6.Configuration" property.
 *
 * Parses a DBus dictionary `a{sv}` representing the configured IPv6 settings
 * and updates the `pd->configured.ipv6` struct.
 * See _efl_net_control_access_point_property_actual_ipv6_changed() for structure.
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_configured_ipv6_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET;
   eina_stringshare_replace(&pd->configured.ipv6.address, NULL);
   eina_stringshare_replace(&pd->configured.ipv6.netmask, NULL);
   eina_stringshare_replace(&pd->configured.ipv6.gateway, NULL);
   pd->configured.ipv6.prefix_length = 0;

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name, *str;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(name, "PrefixLength") == 0)
          {
             if (!eldbus_message_iter_arguments_get(var, "y", &pd->configured.ipv6.prefix_length))
               ERR("Expected unsigned byte value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "off") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_OFF;
             else if (strcmp(str, "fixed") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_FIXED;
             else if (strcmp(str, "manual") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_MANUAL;
             else if (strcmp(str, "6to4") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_TUNNEL6TO4;
             else if (strcmp(str, "auto") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE;
             else
               WRN("Unexpected IPV6 Method value '%s'", str);
          }
        else if (strcmp(name, "Privacy") == 0)
          {
             if ((pd->configured.ipv6.method != EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE) &&
                 (pd->configured.ipv6.method != EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET))
               {
                  DBG("Skip privacy %s, method already set to %d", str, pd->configured.ipv6.method);
                  continue;
               }
             if (strcmp(str, "disabled") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_NONE;
             else if (strcmp(str, "enabled") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_PUBLIC;
             else if (strcmp(str, "preferred") == 0)
               pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_AUTO_PRIVACY_TEMPORARY;
             else
               WRN("Unexpected IPV6 Privacy value '%s'", str);
          }
        else if (strcmp(name, "Address") == 0)
          eina_stringshare_replace(&pd->configured.ipv6.address, str);
        else if (strcmp(name, "Netmask") == 0)
          eina_stringshare_replace(&pd->configured.ipv6.netmask, str);
        else if (strcmp(name, "Gateway") == 0)
          eina_stringshare_replace(&pd->configured.ipv6.gateway, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/**
 * @brief Handles changes to the "Proxy.Configuration" property.
 *
 * Parses a DBus dictionary `a{sv}` representing the configured proxy settings
 * and updates the `pd->configured.proxy` struct.
 * See _efl_net_control_access_point_property_actual_proxy_changed() for structure.
 *
 * @param o The Eo object (unused).
 * @param pd The private data of the access point.
 * @param value The Eldbus_Message_Iter containing the new property value (a dictionary).
 */
static void
_efl_net_control_access_point_property_configured_proxy_changed(Eo *o EINA_UNUSED, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *value)
{
   Eldbus_Message_Iter *array, *entry;
   const char *str;

   if (!eldbus_message_iter_arguments_get(value, "a{sv}", &array))
     {
        ERR("Expected dict for, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   pd->configured.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_UNSET;
   eina_stringshare_replace(&pd->configured.proxy.url, NULL);
   EINA_LIST_FREE(pd->configured.proxy.servers, str) eina_stringshare_del(str);
   EINA_LIST_FREE(pd->configured.proxy.excludes, str) eina_stringshare_del(str);

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *name;
        Eldbus_Message_Iter *var;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &name, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(name, "Servers") == 0)
          {
             _efl_net_control_access_point_list_updated("Proxy Servers", &pd->configured.proxy.servers, var);
             continue;
          }
        else if (strcmp(name, "Excludes") == 0)
          {
             _efl_net_control_access_point_list_updated("Proxy Excludes", &pd->configured.proxy.excludes, var);
             continue;
          }

        if (!eldbus_message_iter_arguments_get(var, "s", &str))
          {
             ERR("Expected string value for %s, got %s", name, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (strcmp(name, "Method") == 0)
          {
             if (strcmp(str, "auto") == 0)
               pd->configured.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_AUTO;
             else if (strcmp(str, "manual") == 0)
               pd->configured.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_MANUAL;
             else if (strcmp(str, "direct") == 0)
               pd->configured.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_OFF;
             else
               WRN("Unexpected PROXY Method value '%s'", str);
          }
        else if (strcmp(name, "URL") == 0)
          eina_stringshare_replace(&pd->configured.proxy.url, str);
        else
          WRN("Unknown property name: %s", name);
     }
}

/**
 * @brief Internal dispatcher for property changes.
 *
 * This function takes a DBus message iterator pointing to a property name
 * and its new value (as a variant), and calls the appropriate specific
 * handler function based on the property name.
 *
 * @param o The Eo object for the access point.
 * @param pd The private data of the access point.
 * @param itr The Eldbus_Message_Iter positioned at the start of a "sv" pair
 *            (property name string, property value variant).
 */
static void
_efl_net_control_access_point_property_changed_internal(Eo *o, Efl_Net_Control_Access_Point_Data *pd, Eldbus_Message_Iter *itr)
{
   Eldbus_Message_Iter *value;
   const char *name;

   if (!eldbus_message_iter_arguments_get(itr, "sv", &name, &value))
     {
        ERR("Unexpected signature: %s", eldbus_message_iter_signature_get(itr));
        return;
     }

   if (strcmp(name, "Name") == 0)
     _efl_net_control_access_point_property_name_changed(o, pd, value);
   else if (strcmp(name, "State") == 0)
     _efl_net_control_access_point_property_state_changed(o, pd, value);
   else if (strcmp(name, "Error") == 0)
     _efl_net_control_access_point_property_error_changed(o, pd, value);
   else if (strcmp(name, "Type") == 0)
     _efl_net_control_access_point_property_type_changed(o, pd, value);
   else if (strcmp(name, "Security") == 0)
     _efl_net_control_access_point_property_security_changed(o, pd, value);
   else if (strcmp(name, "Strength") == 0)
     _efl_net_control_access_point_property_strength_changed(o, pd, value);
   else if (strcmp(name, "Favorite") == 0)
     _efl_net_control_access_point_property_remembered_changed(o, pd, value);
   else if (strcmp(name, "Immutable") == 0)
     _efl_net_control_access_point_property_immutable_changed(o, pd, value);
   else if (strcmp(name, "AutoConnect") == 0)
     _efl_net_control_access_point_property_auto_connect_changed(o, pd, value);
   else if (strcmp(name, "Roaming") == 0)
     _efl_net_control_access_point_property_roaming_changed(o, pd, value);
   else if (strcmp(name, "Nameservers") == 0)
     _efl_net_control_access_point_property_actual_name_servers_changed(o, pd, value);
   else if (strcmp(name, "Timeservers") == 0)
     _efl_net_control_access_point_property_actual_time_servers_changed(o, pd, value);
   else if (strcmp(name, "Domains") == 0)
     _efl_net_control_access_point_property_actual_domains_changed(o, pd, value);
   else if (strcmp(name, "IPv4") == 0)
     _efl_net_control_access_point_property_actual_ipv4_changed(o, pd, value);
   else if (strcmp(name, "IPv6") == 0)
     _efl_net_control_access_point_property_actual_ipv6_changed(o, pd, value);
   else if (strcmp(name, "Proxy") == 0)
     _efl_net_control_access_point_property_actual_proxy_changed(o, pd, value);
   else if (strcmp(name, "Nameservers.Configuration") == 0)
     _efl_net_control_access_point_property_configured_name_servers_changed(o, pd, value);
   else if (strcmp(name, "Timeservers.Configuration") == 0)
     _efl_net_control_access_point_property_configured_time_servers_changed(o, pd, value);
   else if (strcmp(name, "Domains.Configuration") == 0)
     _efl_net_control_access_point_property_configured_domains_changed(o, pd, value);
   else if (strcmp(name, "IPv4.Configuration") == 0)
     _efl_net_control_access_point_property_configured_ipv4_changed(o, pd, value);
   else if (strcmp(name, "IPv6.Configuration") == 0)
     _efl_net_control_access_point_property_configured_ipv6_changed(o, pd, value);
   else if (strcmp(name, "Proxy.Configuration") == 0)
     _efl_net_control_access_point_property_configured_proxy_changed(o, pd, value);
   else if ((strcmp(name, "Provider") == 0) ||
            (strcmp(name, "Ethernet") == 0))
     DBG("Ignored property name: %s", name);
   else
     WRN("Unknown property name: %s", name);
}

/**
 * @brief Callback for ConnMan's "PropertyChanged" DBus signal.
 *
 * This function is invoked when a property of the ConnMan service (access point)
 * changes. It extracts the property name and new value from the signal message
 * and calls the internal dispatcher to update the cached state. Finally, it
 * emits the EFL_NET_CONTROL_ACCESS_POINT_EVENT_CHANGED event.
 *
 * @param data The Eo object associated with the access point.
 * @param msg The Eldbus_Message for the PropertyChanged signal.
 *            The message iterator contains a string (property name) and a variant (new value).
 */
static void
_efl_net_control_access_point_property_changed(void *data, const Eldbus_Message *msg)
{
   Eo *o = data;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Message_Iter *itr;

   itr = eldbus_message_iter_get(msg);
   _efl_net_control_access_point_property_changed_internal(o, pd, itr);
   efl_event_callback_call(o, EFL_NET_CONTROL_ACCESS_POINT_EVENT_CHANGED, NULL);
}

/**
 * @brief Creates a new Efl_Net_Control_Access_Point instance from ConnMan data.
 *
 * This function is called by the Efl_Net_Control_Manager (ConnMan backend)
 * when a new service (access point) is discovered or when existing services
 * are enumerated. It initializes the object, sets up DBus proxy and signal
 * handlers, and populates its initial properties.
 *
 * @param ctl The parent Efl_Net_Control_Manager object.
 * @param path The DBus object path of the ConnMan service.
 * @param properties An Eldbus_Message_Iter containing the initial properties
 *                   of the service, typically from GetServices or a PropertiesChanged signal.
 *                   This iterator is expected to be an array of dictionary entries 'a{sv}'.
 *                   Example: [ {"Name": Variant("MyWiFi"), "State": Variant("idle"), ...}, ... ]
 * @param priority The initial priority of this access point among others.
 * @return A new Efl_Net_Control_Access_Point object, or NULL on failure.
 */
Efl_Net_Control_Access_Point *
efl_net_connman_access_point_new(Efl_Net_Control_Manager *ctl, const char *path, Eldbus_Message_Iter *properties, unsigned int priority)
{
   Eo *o;
   Efl_Net_Control_Access_Point_Data *pd;
   Eldbus_Connection *conn;
   Eldbus_Object *obj;

   conn = efl_net_connman_connection_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);

   o = efl_add(MY_CLASS, ctl);
   EINA_SAFETY_ON_NULL_RETURN_VAL(o, NULL);

   pd = efl_data_scope_get(o, MY_CLASS);
   EINA_SAFETY_ON_NULL_GOTO(pd, error);

   pd->path = eina_stringshare_add(path);
   EINA_SAFETY_ON_NULL_GOTO(pd->path, error);

   obj = eldbus_object_get(conn, "net.connman", pd->path);
   EINA_SAFETY_ON_NULL_GOTO(obj, error);
   pd->proxy = eldbus_proxy_get(obj, "net.connman.Service");
   EINA_SAFETY_ON_NULL_GOTO(pd->proxy, error);

   pd->configured.ipv4.method = EFL_NET_CONTROL_ACCESS_POINT_IPV4_METHOD_UNSET;
   pd->configured.ipv6.method = EFL_NET_CONTROL_ACCESS_POINT_IPV6_METHOD_UNSET;
   pd->configured.proxy.method = EFL_NET_CONTROL_ACCESS_POINT_PROXY_METHOD_UNSET;

#define SH(sig, cb) \
   do { \
     Eldbus_Signal_Handler *sh = eldbus_proxy_signal_handler_add(pd->proxy, sig, cb, o); \
     if (sh) pd->signal_handlers = eina_list_append(pd->signal_handlers, sh); \
     else ERR("could not add DBus signal handler %s", sig); \
   } while (0)

   SH("PropertyChanged", _efl_net_control_access_point_property_changed);
#undef SH

   efl_event_freeze(o);
   efl_net_connman_access_point_update(o, properties, priority);
   efl_event_thaw(o);

   return o;

 error:
   efl_del(o);
   return NULL;
}

/**
 * @brief Gets the DBus object path for a ConnMan access point.
 *
 * @param o The Efl_Net_Control_Access_Point object.
 * @return The DBus object path as a stringshare, or NULL on error.
 */
const char *
efl_net_connman_access_point_path_get(Efl_Net_Control_Access_Point *o)
{
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);
   return pd->path;
}

/**
 * @brief Updates an existing Efl_Net_Control_Access_Point instance with new properties.
 *
 * This function is typically called when ConnMan signals that properties of an
 * existing service have changed, or during an initial bulk update of services.
 *
 * @param o The Efl_Net_Control_Access_Point object to update. (Note: parameter type seems to be Efl_Net_Control_Manager in signature, but used as AccessPoint)
 * @param properties An Eldbus_Message_Iter containing the properties
 *                   of the service. This iterator is expected to be an array
 *                   of dictionary entries 'a{sv}'.
 *                   Example: [ {"State": Variant("online"), "Strength": Variant(byte 80), ...}, ... ]
 * @param priority The new priority of this access point.
 */
void
efl_net_connman_access_point_update(Efl_Net_Control_Manager *o, Eldbus_Message_Iter *properties, unsigned int priority)
{
   Eldbus_Message_Iter *entry;
   Efl_Net_Control_Access_Point_Data *pd = efl_data_scope_get(o, MY_CLASS);

   EINA_SAFETY_ON_NULL_RETURN(pd);

   pd->priority = priority;

   while (eldbus_message_iter_get_and_next(properties, 'e', &entry))
     _efl_net_control_access_point_property_changed_internal(o, pd, entry);

   efl_event_callback_call(o, EFL_NET_CONTROL_ACCESS_POINT_EVENT_CHANGED, NULL);
}

#include "efl_net_control_access_point.eo.c"
