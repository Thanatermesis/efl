#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "efl_net-connman.h"

/**
 * @internal
 * @brief Private data structure for Efl_Net_Session instances using ConnMan.
 *
 * This structure holds all the necessary information for managing a network
 * session through ConnMan, including DBus proxies, pending operations,
 * and cached session properties.
 */
typedef struct
{
   Eldbus_Proxy *proxy; /**< DBus proxy for the net.connman.Session interface. */
   Eldbus_Service_Interface *notifier; /**< DBus service interface for net.connman.Notification. */
   Eldbus_Pending *mgr_pending; /**< Pending call for efl_net_connman_manager_get(). */

   struct {
      Eldbus_Pending *pending; /**< Pending DBus call related to connect/change operations. */
      Eina_Bool connected; /**< Flag indicating if the session should be connected. */
      Eina_Bool online_required; /**< Flag indicating if an online (internet) connection is required. */
      Efl_Net_Session_Technology technologies_allowed; /**< Bitmask of allowed network technologies. */
   } connect; /**< State related to connection requests. */

   /* properties notified by session, local cache */
   Eina_Stringshare *name; /**< Name of the network service (e.g., SSID for Wi-Fi). */
   Eina_Stringshare *interface; /**< Network interface name (e.g., "eth0", "wlan0"). */
   struct {
      Eina_Stringshare *address; /**< IPv4 address. */
      Eina_Stringshare *netmask; /**< IPv4 netmask. */
      Eina_Stringshare *gateway; /**< IPv4 gateway. */
   } ipv4; /**< IPv4 configuration details. */
   struct {
      Eina_Stringshare *address; /**< IPv6 address. */
      Eina_Stringshare *netmask; /**< IPv6 netmask (often represented by prefix length). */
      Eina_Stringshare *gateway; /**< IPv6 gateway. */
      uint8_t prefix_length; /**< IPv6 prefix length. */
   } ipv6; /**< IPv6 configuration details. */
   Efl_Net_Session_State state; /**< Current state of the network session. */
   Efl_Net_Session_Technology technology; /**< Current technology used by the session. */
} Efl_Net_Session_Data;

#define MY_CLASS EFL_NET_SESSION_CLASS

/**
 * @internal
 * @brief Converts a technology string from ConnMan to an Efl_Net_Session_Technology bitmask.
 *
 * This function parses common technology strings (e.g., "ethernet", "wifi")
 * and sets the corresponding bit in the `tech` parameter.
 * It supports accumulating multiple technologies if `*tech` is initialized to 0
 * and the function is called multiple times.
 *
 * @param str The technology string (e.g., "ethernet", "wifi", "*").
 * @param[out] tech Pointer to an Efl_Net_Session_Technology variable to store the result.
 *                  The corresponding bit for the technology will be set.
 * @return EINA_TRUE on success, EINA_FALSE if the string is unknown (and not empty).
 */
static Eina_Bool
_efl_net_session_technology_from_str(const char *str, Efl_Net_Session_Technology *tech)
{
   if (0) { }
#define MAP(X, s) \
   else if (strcmp(str, s) == 0) *tech |= EFL_NET_SESSION_TECHNOLOGY_ ## X
   MAP(ALL, "*");
   MAP(ETHERNET, "ethernet");
   MAP(WIFI, "wifi");
   MAP(BLUETOOTH, "bluetooth");
   MAP(CELLULAR, "cellular");
   MAP(VPN, "vpn");
   MAP(GADGET, "gadget");
#undef MAP
   else if (str[0]) /* empty bearer = no technology */
     {
        WRN("Unknown technology name: %s", str);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Initiates or re-applies the connection settings to an existing ConnMan session.
 *
 * This function is called when a connection is requested or when the ConnMan
 * service becomes available and there's a pending connection request.
 * It sends "Change" DBus messages to ConnMan to set "AllowedBearers" and
 * "ConnectionType" before (or as part of) establishing the connection.
 *
 * @param o The Efl_Net_Session Eo object.
 * @param pd The private data of the Efl_Net_Session object.
 */
static void _efl_net_session_connect_do(Eo *o, Efl_Net_Session_Data *pd);

/* NOTE: unlike most DBus servers where you create paths using
 * ObjectManager and monitor properties on them using PropertyManager,
 * ConnMan doesn't use any of those and their API for Session is
 * different from all others in ConnMan itself:
 *
 * 1 - create a local service 'notifier' implementing
 *     net.connman.Notification, with method Release() and
 *     Update(dict settings).
 *
 * 2 - call / CreateSession(dict settings, path notifier), get an
 *     object path representing your session
 *
 * 3 - call Change(setting, value), Connect(), Disconnect() on
 *     object path returned on #2
 *
 * 4 - get Update() to be called on your local service notifier specified
 *     on step #1.
 */

/**
 * @internal
 * @brief DBus callback for the net.connman.Notification.Release method.
 *
 * This method is called by ConnMan when the session is being released
 * (e.g., ConnMan is shutting down or the session is explicitly destroyed).
 *
 * @param service The Eldbus_Service_Interface that received the call.
 * @param msg The incoming Eldbus_Message.
 * @return A new Eldbus_Message for the method return.
 */
static Eldbus_Message *
_efl_net_session_notifier_release(const Eldbus_Service_Interface *service, const Eldbus_Message *msg)
{
   Eo *o = eldbus_service_object_data_get(service, "efl_net");
   DBG("Session %p is released %s", o, eldbus_message_path_get(msg));

   return eldbus_message_method_return_new(msg);
}

/**
 * @internal
 * @brief Updates the session state based on a "State" property string from ConnMan.
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "State" property.
 *            Expected to be a string (e.g., "disconnected", "connected", "online").
 * @return 0 on success, EINVAL if the variant type is wrong or the state string is unknown.
 */
static Eina_Error
_efl_net_session_notifier_update_state(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   const char *str;

   if (!eldbus_message_iter_arguments_get(var, "s", &str))
     return EINVAL;

   if (strcmp(str, "disconnected") == 0)
     pd->state = EFL_NET_SESSION_STATE_OFFLINE;
   else if (strcmp(str, "connected") == 0)
     pd->state = EFL_NET_SESSION_STATE_LOCAL;
   else if (strcmp(str, "online") == 0)
     pd->state = EFL_NET_SESSION_STATE_ONLINE;
   else
     return EINVAL;

   return 0;
}

/**
 * @internal
 * @brief Updates the session name based on a "Name" property string from ConnMan.
 *
 * The name usually refers to the service name, like an SSID for Wi-Fi.
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "Name" property.
 *            Expected to be a string.
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_name(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   const char *str;

   if (!eldbus_message_iter_arguments_get(var, "s", &str))
     return EINVAL;

   eina_stringshare_replace(&pd->name, str);
   return 0;
}

/**
 * @internal
 * @brief Updates the session technology based on a "Bearer" property string from ConnMan.
 *
 * Note: ConnMan uses "Bearer" to refer to the technology type.
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "Bearer" property.
 *            Expected to be a string (e.g., "ethernet", "wifi").
 * @return 0 on success, EINVAL if the variant type is wrong or the technology string is unknown.
 */
static Eina_Error
_efl_net_session_notifier_update_technology(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   const char *str;

   if (!eldbus_message_iter_arguments_get(var, "s", &str))
     return EINVAL;

   pd->technology = 0;
   if (!_efl_net_session_technology_from_str(str, &pd->technology))
     return EINVAL;

   return 0;
}

/**
 * @internal
 * @brief Updates the session network interface name based on an "Interface" property string from ConnMan.
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "Interface" property.
 *            Expected to be a string (e.g., "eth0", "wlan0").
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_interface(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   const char *str;

   if (!eldbus_message_iter_arguments_get(var, "s", &str))
     return EINVAL;

   eina_stringshare_replace(&pd->interface, str);
   return 0;
}

/**
 * @internal
 * @brief Updates IPv4 configuration details from an "IPv4" property dictionary from ConnMan.
 *
 * The "IPv4" property is a dictionary (a{sv}) containing keys like "Address",
 * "Netmask", "Gateway", and "Method".
 * Example: {"Address": "192.168.1.100", "Netmask": "255.255.255.0", "Gateway": "192.168.1.1"}
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "IPv4" property.
 *            Expected to be a dictionary of string to variant (a{sv}).
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_ipv4(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   Eldbus_Message_Iter *sub, *entry;

   if (!eldbus_message_iter_arguments_get(var, "a{sv}", &sub))
     return EINVAL;

   while (eldbus_message_iter_get_and_next(sub, 'e', &entry))
     {
        const char *key;
        Eldbus_Message_Iter *value;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &value))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(key, "Method") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               DBG("configuration method %s", str);
          }
        else if (strcmp(key, "Address") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("address %s", str);
                  eina_stringshare_replace(&pd->ipv4.address, str);
               }
          }
        else if (strcmp(key, "Gateway") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("gateway %s", str);
                  eina_stringshare_replace(&pd->ipv4.gateway, str);
               }
          }
        else if (strcmp(key, "Netmask") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("netmask %s", str);
                  eina_stringshare_replace(&pd->ipv4.netmask, str);
               }
          }
        else
          {
             WRN("Unsupported field %s (signature=%s)", key, eldbus_message_iter_signature_get(value));
             continue;
          }
     }

   return 0;
}

/**
 * @internal
 * @brief Updates IPv6 configuration details from an "IPv6" property dictionary from ConnMan.
 *
 * The "IPv6" property is a dictionary (a{sv}) containing keys like "Address",
 * "PrefixLength", "Gateway", "Method", and "Privacy".
 * Example: {"Address": "2001:db8::100", "PrefixLength": 64, "Gateway": "2001:db8::1"}
 *
 * @param pd The private data of the Efl_Net_Session object.
 * @param var Eldbus_Message_Iter pointing to the variant value of the "IPv6" property.
 *            Expected to be a dictionary of string to variant (a{sv}).
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_ipv6(Efl_Net_Session_Data *pd, Eldbus_Message_Iter *var)
{
   Eldbus_Message_Iter *sub, *entry;

   if (!eldbus_message_iter_arguments_get(var, "a{sv}", &sub))
     return EINVAL;

   while (eldbus_message_iter_get_and_next(sub, 'e', &entry))
     {
        const char *key;
        Eldbus_Message_Iter *value;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &value))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(key, "Method") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               DBG("configuration method %s", str);
          }
        else if (strcmp(key, "Address") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("address %s", str);
                  eina_stringshare_replace(&pd->ipv6.address, str);
               }
          }
        else if (strcmp(key, "Gateway") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("gateway %s", str);
                  eina_stringshare_replace(&pd->ipv6.gateway, str);
               }
          }
        else if (strcmp(key, "Netmask") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               {
                  DBG("netmask %s", str);
                  eina_stringshare_replace(&pd->ipv6.netmask, str);
               }
          }
        else if (strcmp(key, "PrefixLength") == 0)
          {
             if (!eldbus_message_iter_arguments_get(value, "y", &pd->ipv6.prefix_length))
               ERR("expected unsigned byte, property=%s", key);
             else
               DBG("prefix_length %hhu", pd->ipv6.prefix_length);
          }
        else if (strcmp(key, "Privacy") == 0)
          {
             const char *str;
             if (!eldbus_message_iter_arguments_get(value, "s", &str))
               ERR("expected string, property=%s", key);
             else
               DBG("privacy %s (unused)", str);
          }
        else
          {
             WRN("Unsupported field %s (signature=%s)", key, eldbus_message_iter_signature_get(value));
             continue;
          }
     }

   return 0;
}

/**
 * @internal
 * @brief Processes "AllowedBearers" property from ConnMan.
 *
 * This property is an array of strings indicating which bearer types
 * (technologies) are allowed for the session. Currently, this function
 * only logs the bearers and does not store them.
 * Example: ["ethernet", "wifi"]
 *
 * @param pd The private data of the Efl_Net_Session object (unused).
 * @param var Eldbus_Message_Iter pointing to the variant value of the "AllowedBearers" property.
 *            Expected to be an array of strings (as).
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_bearers(Efl_Net_Session_Data *pd EINA_UNUSED, Eldbus_Message_Iter *var)
{
   Eldbus_Message_Iter *sub;
   const char *str;

   if (!eldbus_message_iter_arguments_get(var, "as", &sub))
     {
        ERR("Expected array of strings, got %s", eldbus_message_iter_signature_get(var));
        return EINVAL;
     }
   while (eldbus_message_iter_get_and_next(sub, 's', &str))
     DBG("allowed bearer '%s'", str);

   return 0;
}

/**
 * @internal
 * @brief Processes "ConnectionType" property from ConnMan.
 *
 * This property is a string indicating the type of connection (e.g., "internet", "local").
 * Currently, this function only logs the connection type and does not store it directly,
 * as the session state (online/local) is derived from the "State" property.
 *
 * @param pd The private data of the Efl_Net_Session object (unused).
 * @param var Eldbus_Message_Iter pointing to the variant value of the "ConnectionType" property.
 *            Expected to be a string.
 * @return 0 on success, EINVAL if the variant type is wrong.
 */
static Eina_Error
_efl_net_session_notifier_update_connection_type(Efl_Net_Session_Data *pd EINA_UNUSED, Eldbus_Message_Iter *var)
{
   const char *str;
   if (!eldbus_message_iter_arguments_get(var, "s", &str))
     return EINVAL;

   DBG("connection type '%s'", str);
   return 0;
}

/**
 * @internal
 * @brief DBus callback for the net.connman.Notification.Update method.
 *
 * This method is called by ConnMan to provide initial session properties
 * or to notify of property changes. It receives a dictionary of
 * properties that have been updated.
 *
 * The `settings` dictionary (a{sv}) can contain various keys, including:
 * - "State": (s) e.g., "disconnected", "connected", "online"
 * - "Name": (s) e.g., "MyWiFiNetwork"
 * - "Bearer": (s) e.g., "wifi", "ethernet" (maps to technology)
 * - "Interface": (s) e.g., "wlan0", "eth0"
 * - "IPv4": (a{sv}) Dictionary of IPv4 settings.
 *   - "Address": (s)
 *   - "Netmask": (s)
 *   - "Gateway": (s)
 *   - "Method": (s) e.g., "dhcp", "manual"
 * - "IPv6": (a{sv}) Dictionary of IPv6 settings.
 *   - "Address": (s)
 *   - "PrefixLength": (y)
 *   - "Gateway": (s)
 *   - "Method": (s) e.g., "auto", "manual"
 *   - "Privacy": (s) e.g., "disabled", "enabled"
 * - "AllowedBearers": (as) e.g., ["wifi", "ethernet"]
 * - "ConnectionType": (s) e.g., "internet", "local"
 *
 * @param service The Eldbus_Service_Interface that received the call.
 * @param msg The incoming Eldbus_Message containing the updated settings.
 * @return A new Eldbus_Message for the method return.
 */
static Eldbus_Message *
_efl_net_session_notifier_update(const Eldbus_Service_Interface *service, const Eldbus_Message *msg)
{
   Eo *o = eldbus_service_object_data_get(service, "efl_net");
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Message_Iter *array, *entry;
   Eina_Bool updated = EINA_FALSE;

   DBG("Session %p is updated %s", o, eldbus_message_path_get(msg));

   EINA_SAFETY_ON_NULL_GOTO(o, end);

   if (!eldbus_message_arguments_get(msg, "a{sv}", &array))
     {
        ERR("Unexpected net.connman.Notifier.Update() signature %s", eldbus_message_signature_get(msg));
        goto end;
     }

   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const char *key;
        Eldbus_Message_Iter *var;
        Eina_Error err;

        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &var))
          {
             ERR("Unexpected dict entry signature: %s", eldbus_message_iter_signature_get(entry));
             continue;
          }

        if (strcmp(key, "State") == 0)
          err = _efl_net_session_notifier_update_state(pd, var);
        else if (strcmp(key, "Name") == 0)
          err = _efl_net_session_notifier_update_name(pd, var);
        else if (strcmp(key, "Bearer") == 0)
          err = _efl_net_session_notifier_update_technology(pd, var);
        else if (strcmp(key, "Interface") == 0)
          err = _efl_net_session_notifier_update_interface(pd, var);
        else if (strcmp(key, "IPv4") == 0)
          err = _efl_net_session_notifier_update_ipv4(pd, var);
        else if (strcmp(key, "IPv6") == 0)
          err = _efl_net_session_notifier_update_ipv6(pd, var);
        else if (strcmp(key, "AllowedBearers") == 0)
          err = _efl_net_session_notifier_update_bearers(pd, var);
        else if (strcmp(key, "ConnectionType") == 0)
          err = _efl_net_session_notifier_update_connection_type(pd, var);
        else
          {
             WRN("Unsupported setting %s (signature=%s)", key, eldbus_message_iter_signature_get(var));
             continue;
          }

        if (err)
          {
             ERR("Could not handle property %s: %s", key, eina_error_msg_get(err));
             continue;
          }

        updated |= EINA_TRUE;
     }

   if (updated) efl_event_callback_call(o, EFL_NET_SESSION_EVENT_CHANGED, NULL);

 end:
   return eldbus_message_method_return_new(msg);
}

static const Eldbus_Service_Interface_Desc _efl_net_session_notifier_desc = {
  .interface = "net.connman.Notification",
  .methods = (const Eldbus_Method []){
     {
        .member = "Release",
        .in = NULL,
        .out = NULL,
        .cb = _efl_net_session_notifier_release,
        .flags = 0
     },
     {
        .member = "Update",
        .in = ELDBUS_ARGS({"a{sv}", "settings"}),
        .out = NULL,
        .cb = _efl_net_session_notifier_update,
        .flags = 0
     },
     { NULL, NULL, NULL, NULL, 0 }
   },
   .signals = NULL,
   .properties = NULL,
   .default_get = NULL,
   .default_set = NULL
};

/**
 * @internal
 * @brief Callback for the ConnMan Manager.CreateSession DBus method call.
 *
 * This function is invoked when ConnMan responds to the CreateSession request.
 * If successful, it retrieves the object path of the newly created session and
 * creates a DBus proxy for `net.connman.Session` on that path.
 * If a connection was requested before the session was created, it triggers
 * `_efl_net_session_connect_do` to apply those settings.
 *
 * @param data The Eo *o (Efl_Net_Session object) passed as user data.
 * @param msg The reply message from DBus.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_efl_net_session_create_session_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Connection *conn;
   Eldbus_Object *obj;
   const char *err_name, *err_msg, *path;

   if (pd->mgr_pending == pending)
     pd->mgr_pending = NULL;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not create session %p: %s=%s", o, err_name, err_msg);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "o", &path))
     {
        ERR("Could not get session %p DBus path!", o);
        return;
     }

   conn = efl_net_connman_connection_get();
   obj = eldbus_object_get(conn, "net.connman", path);
   pd->proxy = eldbus_proxy_get(obj, "net.connman.Session");
   if (!pd->proxy)
     {
        ERR("could not create DBus proxy for interface='net.connman.Session', name='net.connman', path='%s' o=%p", path, o);
        eldbus_object_unref(obj);
        return;
     }

   DBG("Created session %p (session=%s) with ConnMan...", o, path);

   if (pd->connect.connected)
     {
        DBG("apply pending connect request: online_required=%d, technologies_allowed=%#x", pd->connect.online_required, pd->connect.technologies_allowed);
        _efl_net_session_connect_do(o, pd);
     }
}

/**
 * @internal
 * @brief Clears all cached session properties in the private data structure.
 *
 * Resets stringshares to NULL and numeric values to their defaults (e.g.,
 * state to OFFLINE, technology to UNKNOWN). This is typically called when
 * the session is destroyed or when ConnMan disappears.
 *
 * @param pd The private data of the Efl_Net_Session object.
 */
static void
_efl_net_session_clear(Efl_Net_Session_Data *pd)
{
   eina_stringshare_replace(&pd->name, NULL);
   eina_stringshare_replace(&pd->interface, NULL);
   eina_stringshare_replace(&pd->ipv4.address, NULL);
   eina_stringshare_replace(&pd->ipv4.netmask, NULL);
   eina_stringshare_replace(&pd->ipv4.gateway, NULL);
   eina_stringshare_replace(&pd->ipv6.address, NULL);
   eina_stringshare_replace(&pd->ipv6.netmask, NULL);
   eina_stringshare_replace(&pd->ipv6.gateway, NULL);
   pd->ipv6.prefix_length = 0;
   pd->state = EFL_NET_SESSION_STATE_OFFLINE;
   pd->technology = EFL_NET_SESSION_TECHNOLOGY_UNKNOWN;
}

/**
 * @internal
 * @brief Callback for DBus name owner changes for "net.connman".
 *
 * This function is invoked when the "net.connman" service appears on or
 * disappears from the system bus.
 *
 * If ConnMan appears (new_id is not NULL or empty):
 * It calls `CreateSession` on the ConnMan Manager to establish a new
 * network session context.
 *
 * If ConnMan disappears (new_id is NULL or empty):
 * It cleans up the existing session proxy and clears cached data, as the
 * session is no longer valid. It also emits an EFL_NET_SESSION_EVENT_CHANGED
 * event to notify listeners.
 *
 * @param data The Eo *o (Efl_Net_Session object) passed as user data.
 * @param bus The bus name that changed owner (expected to be "net.connman").
 * @param old_id The old unique connection ID of the owner, or empty if none.
 * @param new_id The new unique connection ID of the owner, or empty if none.
 */
static void
_efl_net_session_connman_name_owner_changed(void *data, const char *bus, const char *old_id, const char *new_id)
{
   Eo *o = data;
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Message_Iter *msg_itr, *cont;
   Eldbus_Proxy *mgr;
   Eldbus_Message *msg;
   const char *path;

   DBG("Name Owner Changed %s: %s->%s", bus, old_id, new_id);
   if ((!new_id) || (new_id[0] == '\0'))
     {
        /* connman is gone, remove proxy as it became useless */
        if (pd->proxy)
          {
             Eldbus_Object *obj = eldbus_proxy_object_get(pd->proxy);
             eldbus_proxy_unref(pd->proxy);
             pd->proxy = NULL;
             eldbus_object_unref(obj);
          }
        _efl_net_session_clear(pd);
        efl_event_callback_call(o, EFL_NET_SESSION_EVENT_CHANGED, NULL);
        return;
     }

   path = eldbus_service_object_path_get(pd->notifier);
   EINA_SAFETY_ON_NULL_RETURN(path);

   INF("Create session %p notifier=%s with %s (%s)", o, path, bus, new_id);

   mgr = efl_net_connman_manager_get();
   EINA_SAFETY_ON_NULL_RETURN(mgr);

   msg = eldbus_proxy_method_call_new(mgr, "CreateSession");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_send);

   cont = eldbus_message_iter_container_new(msg_itr, 'a', "{sv}");
   eldbus_message_iter_container_close(msg_itr, cont); /* empty, use defaults */

   eldbus_message_iter_basic_append(msg_itr, 'o', path);

   if (pd->mgr_pending)
     eldbus_pending_cancel(pd->mgr_pending);

   pd->mgr_pending = eldbus_proxy_send(mgr, msg, _efl_net_session_create_session_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(pd->mgr_pending, error_send);
   return;

 error_send:
   eldbus_message_unref(msg);
}

/**
 * @internal
 * @brief Efl_Object constructor for Efl_Net_Session (ConnMan backend).
 *
 * Initializes the ConnMan infrastructure.
 *
 * @param o The Eo object being constructed.
 * @param pd Private data for the object (unused in this function).
 * @return The constructed Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_net_session_efl_object_constructor(Eo *o, Efl_Net_Session_Data *pd EINA_UNUSED)
{
   if (!efl_net_connman_init())
     {
        ERR("could not initialize connman infrastructure");
        return NULL;
     }

   return efl_constructor(efl_super(o, MY_CLASS));
}

/**
 * @internal
 * @brief Efl_Object finalize method for Efl_Net_Session (ConnMan backend).
 *
 * This function is called after the object is constructed and fully initialized.
 * It performs the main setup for interacting with ConnMan:
 * 1. Registers a local DBus service (`net.connman.Notification`) which ConnMan
 *    will call to send updates about the session. The path for this service
 *    is dynamically generated (e.g., "/connman/notifier_0xADDRESS").
 * 2. Sets up a listener for DBus name owner changes of "net.connman". This
 *    allows the session to react when ConnMan becomes available or disappears.
 *
 * @param o The Eo object being finalized.
 * @param pd Private data for the object.
 * @return The finalized Eo object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_net_session_efl_object_finalize(Eo *o, Efl_Net_Session_Data *pd)
{
   Eldbus_Connection *conn;
   char path[128];

   o = efl_finalize(efl_super(o, MY_CLASS));
   if (!o) return NULL;

   conn = efl_net_connman_connection_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);

   /* step #1: create local notifier path */
   snprintf(path, sizeof(path), "/connman/notifier_%p", o);
   pd->notifier = eldbus_service_interface_register(conn, path, &_efl_net_session_notifier_desc);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->notifier, NULL);
   eldbus_service_object_data_set(pd->notifier, "efl_net", o);

   eldbus_name_owner_changed_callback_add(conn, "net.connman", _efl_net_session_connman_name_owner_changed, o, EINA_TRUE);
   DBG("waiting for net.connman to show on the DBus system bus...");

   return o;
}

/**
 * @internal
 * @brief Efl_Object destructor for Efl_Net_Session (ConnMan backend).
 *
 * Cleans up all resources associated with the ConnMan session:
 * - Removes the DBus name owner changed callback.
 * - Cancels any pending DBus operations.
 * - Unregisters the local `net.connman.Notification` service.
 * - If a session proxy exists, calls `DestroySession` on the ConnMan Manager
 *   to inform ConnMan that this session is ending.
 * - Unrefs the session proxy and its associated DBus object.
 * - Clears cached session data.
 * - Shuts down the ConnMan infrastructure.
 *
 * @param o The Eo object being destructed.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_net_session_efl_object_destructor(Eo *o, Efl_Net_Session_Data *pd)
{
   Eldbus_Connection *conn;

   conn = efl_net_connman_connection_get();
   eldbus_name_owner_changed_callback_del(conn, "net.connman", _efl_net_session_connman_name_owner_changed, o);

   if (pd->mgr_pending)
     {
        eldbus_pending_cancel(pd->mgr_pending);
        pd->mgr_pending = NULL;
     }

   if (pd->notifier)
     {
        eldbus_service_object_data_del(pd->notifier, "efl_net");
        eldbus_service_object_unregister(pd->notifier);
        pd->notifier = NULL;
     }

   if (pd->proxy)
     {
        Eldbus_Object *obj = eldbus_proxy_object_get(pd->proxy);
        Eldbus_Proxy *mgr = efl_net_connman_manager_get();

        /* DestroySession is required since the manager proxy and bus
         * may be alive, thus connman won't get any NameOwnerChanged
         * or related to know the object is gone
         */
        eldbus_proxy_call(mgr, "DestroySession", NULL, NULL, DEFAULT_TIMEOUT,
                          "o", eldbus_object_path_get(obj));

        eldbus_proxy_unref(pd->proxy);
        eldbus_object_unref(obj);
        pd->proxy = NULL;
     }

   _efl_net_session_clear(pd);

   efl_destructor(efl_super(o, MY_CLASS));
   efl_net_connman_shutdown();
}

EOLIAN static const char *
_efl_net_session_network_name_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd)
{
   return pd->name;
}

EOLIAN static Efl_Net_Session_State
_efl_net_session_state_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd)
{
   return pd->state;
}

EOLIAN static Efl_Net_Session_Technology
_efl_net_session_technology_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd)
{
   return pd->technology;
}

EOLIAN static const char *
_efl_net_session_interface_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd)
{
   return pd->interface;
}

EOLIAN static void
_efl_net_session_ipv4_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd, const char **address, const char **netmask, const char **gateway)
{
   if (address) *address = pd->ipv4.address;
   if (netmask) *netmask = pd->ipv4.netmask;
   if (gateway) *gateway = pd->ipv4.gateway;
}

EOLIAN static void
_efl_net_session_ipv6_get(const Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd, const char **address, uint8_t *prefix_length, const char **netmask, const char **gateway)
{
   if (address) *address = pd->ipv6.address;
   if (netmask) *netmask = pd->ipv6.netmask;
   if (gateway) *gateway = pd->ipv6.gateway;
   if (prefix_length) *prefix_length = pd->ipv6.prefix_length;
}

/**
 * @internal
 * @brief Callback for the `net.connman.Session.Connect` DBus method call.
 *
 * This function is invoked when ConnMan responds to the `Connect` request.
 * It primarily logs the outcome of the request.
 *
 * @param data The Eo *o (Efl_Net_Session object) passed as user data.
 * @param msg The reply message from DBus.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_efl_net_session_connect_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   if (pd->connect.pending == pending)
     pd->connect.pending = NULL;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        WRN("Could not Connect: %s=%s", err_name, err_msg);
        return;
     }
   DBG("Successfully requested a connection online_required=%hhu, technologies_allowed=%#x", pd->connect.online_required, pd->connect.technologies_allowed);
}

/**
 * @internal
 * @brief Issues the `net.connman.Session.Connect` DBus method call.
 *
 * This function is called after successfully changing the "ConnectionType"
 * (via `_efl_net_session_connect_change_online_required`). It makes the
 * actual `Connect` call to ConnMan.
 *
 * @param data The Eo *o (Efl_Net_Session object) passed as user data.
 * @param msg The reply message from the previous `Change` DBus call (for "ConnectionType").
 * @param pending The Eldbus_Pending object for the previous call.
 */
static void
_efl_net_session_connect_do_connect(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   if (pd->connect.pending == pending)
     pd->connect.pending = NULL;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        WRN("Could not Change('ConnectionType'): %s=%s", err_name, err_msg);
        return;
     }

   pd->connect.pending = eldbus_proxy_call(pd->proxy, "Connect",
                                           _efl_net_session_connect_cb, o,
                                           DEFAULT_TIMEOUT, "");
   EINA_SAFETY_ON_NULL_RETURN(pd->connect.pending);
}

/**
 * @internal
 * @brief Issues a `net.connman.Session.Change` DBus call to set "ConnectionType".
 *
 * This function is called after successfully changing "AllowedBearers".
 * It sets the "ConnectionType" property on the ConnMan session based on
 * `pd->connect.online_required` (to "internet" or "local").
 * Upon successful completion, it calls `_efl_net_session_connect_do_connect`
 * to proceed with the `Connect` call.
 *
 * @param data The Eo *o (Efl_Net_Session object) passed as user data.
 * @param msg_ret The reply message from the previous `Change` DBus call (for "AllowedBearers").
 * @param pending The Eldbus_Pending object for the previous call.
 */
static void
_efl_net_session_connect_change_online_required(void *data, const Eldbus_Message *msg_ret, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Session_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Message *msg;
   Eldbus_Message_Iter *msg_itr, *itr;
   const char *err_name, *err_msg;
   Eina_Bool ret;

   if (pd->connect.pending == pending)
     pd->connect.pending = NULL;

   if (eldbus_message_error_get(msg_ret, &err_name, &err_msg))
     {
        WRN("Could not Change('AllowedBearers'): %s=%s", err_name, err_msg);
        return;
     }

   msg = eldbus_proxy_method_call_new(pd->proxy, "Change");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_msg);

   eldbus_message_iter_basic_append(msg_itr, 's', "ConnectionType");
   itr = eldbus_message_iter_container_new(msg_itr, 'v', "s");
   EINA_SAFETY_ON_NULL_GOTO(itr, error_msg);

   ret = eldbus_message_iter_basic_append(itr, 's', pd->connect.online_required ? "internet" : "local");
   EINA_SAFETY_ON_FALSE_GOTO(ret, error_msg);

   eldbus_message_iter_container_close(msg_itr, itr);

   pd->connect.pending = eldbus_proxy_send(pd->proxy, msg,
                                           _efl_net_session_connect_do_connect, o,
                                           DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_RETURN(pd->connect.pending);
   return;

 error_msg:
   eldbus_message_unref(msg);
}

/**
 * @internal
 * @brief Initiates the connection process by first setting "AllowedBearers".
 *
 * This is the main entry point for configuring a ConnMan session for connection.
 * It sends a `net.connman.Session.Change` DBus message to set the
 * "AllowedBearers" property based on `pd->connect.technologies_allowed`.
 * Upon successful completion, it calls
 * `_efl_net_session_connect_change_online_required` to set "ConnectionType".
 *
 * This function handles cancellation of previous pending connect operations.
 *
 * @param o The Efl_Net_Session Eo object.
 * @param pd The private data of the Efl_Net_Session object.
 */
static void
_efl_net_session_connect_do(Eo *o, Efl_Net_Session_Data *pd)
{
   Eldbus_Message_Iter *msg_itr, *itr, *array;
   Eldbus_Message *msg;

   if (!pd->connect.connected) return;

   if (pd->connect.pending)
     {
        eldbus_pending_cancel(pd->connect.pending);
        pd->connect.pending = NULL;
     }

   msg = eldbus_proxy_method_call_new(pd->proxy, "Change");
   EINA_SAFETY_ON_NULL_RETURN(msg);

   msg_itr = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_GOTO(msg_itr, error_msg);

   eldbus_message_iter_basic_append(msg_itr, 's', "AllowedBearers");
   itr = eldbus_message_iter_container_new(msg_itr, 'v', "as");
   EINA_SAFETY_ON_NULL_GOTO(itr, error_msg);

   array = eldbus_message_iter_container_new(itr, 'a', "s");

#define MAP(X, s) \
   if ((pd->connect.technologies_allowed & EFL_NET_SESSION_TECHNOLOGY_ ## X) == EFL_NET_SESSION_TECHNOLOGY_ ## X) \
        eldbus_message_iter_basic_append(array, 's', s)
   MAP(ALL, "*");
   if (pd->connect.technologies_allowed == EFL_NET_SESSION_TECHNOLOGY_ALL) goto end;
   MAP(ETHERNET, "ethernet");
   MAP(WIFI, "wifi");
   MAP(BLUETOOTH, "bluetooth");
   MAP(CELLULAR, "cellular");
   MAP(VPN, "vpn");
   MAP(GADGET, "gadget");
#undef MAP

 end:
   eldbus_message_iter_container_close(itr, array);
   eldbus_message_iter_container_close(msg_itr, itr);

   pd->connect.pending = eldbus_proxy_send(pd->proxy, msg,
                                           _efl_net_session_connect_change_online_required, o,
                                           DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_RETURN(pd->connect.pending);
   return;

 error_msg:
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_session_connect(Eo *o, Efl_Net_Session_Data *pd, Eina_Bool online_required, Efl_Net_Session_Technology technologies_allowed)
{
   pd->connect.connected = EINA_TRUE;
   pd->connect.online_required = online_required;
   pd->connect.technologies_allowed = technologies_allowed;

   if (pd->proxy) _efl_net_session_connect_do(o, pd);
}

/**
 * @internal
 * @brief Implements Efl.Net.Session.disconnect method.
 *
 * Cancels any pending connect-related operations and sends a
 * `net.connman.Session.Disconnect` DBus message to ConnMan.
 * Sets the internal `pd->connect.connected` flag to EINA_FALSE.
 *
 * @param o The Eo object (unused).
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_net_session_disconnect(Eo *o EINA_UNUSED, Efl_Net_Session_Data *pd)
{
   if (pd->connect.pending)
     {
        eldbus_pending_cancel(pd->connect.pending);
        pd->connect.pending = NULL;
     }
   pd->connect.connected = EINA_FALSE;

   eldbus_proxy_call(pd->proxy, "Disconnect", NULL, NULL, DEFAULT_TIMEOUT, "");
}

#include "efl_net_session.eo.c"
