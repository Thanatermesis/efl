#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "Ecore_Con.h"
#include "ecore_con_private.h"
#include "efl_net-connman.h"

/**
 * @brief Private data structure for Efl_Net_Control_Technology.
 *
 * This structure holds all the internal data associated with a ConnMan
 * technology object, such as its D-Bus proxy, properties, and pending
 * operations.
 */
typedef struct
{
   /* Eldbus_Proxy/Eldbus_Object keeps a list of pending calls, but
    * they are reference counted singletons and will only cancel
    * pending calls when everything is gone.  However we operate on
    * our private data, that may be gone before other refs. So
    * keep the pending list.
    */
   Eina_List *pending; /**< List of pending D-Bus calls. */
   Eina_List *signal_handlers; /**< List of D-Bus signal handlers. */
   Eldbus_Proxy *proxy; /**< D-Bus proxy for net.connman.Technology. */
   Eina_Stringshare *path; /**< D-Bus object path for this technology. */
   Eina_Stringshare *name; /**< Name of the technology (e.g., "WiFi", "Ethernet"). */
   struct {
      Eina_Stringshare *identifier; /**< Tethering identifier (SSID for WiFi). */
      Eina_Stringshare *passphrase; /**< Tethering passphrase. */
      Eina_Bool enabled; /**< Tethering enabled state. */
   } tethering; /**< Tethering specific properties. */
   Efl_Net_Control_Technology_Type type; /**< Type of the technology. */
   Eina_Bool powered; /**< Powered state of the technology. */
   Eina_Bool connected; /**< Connected state of the technology. */
} Efl_Net_Control_Technology_Data;

#define MY_CLASS EFL_NET_CONTROL_TECHNOLOGY_CLASS

/**
 * @brief Handles the "Powered" property change from D-Bus.
 *
 * Updates the internal powered state and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_powered_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool powered;

   if (!eldbus_message_iter_arguments_get(value, "b", &powered))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->powered == powered) return;
   pd->powered = powered;
   DBG("powered=%hhu", powered);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Handles the "Connected" property change from D-Bus.
 *
 * Updates the internal connected state and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_connected_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool connected;

   if (!eldbus_message_iter_arguments_get(value, "b", &connected))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->connected == connected) return;
   pd->connected = connected;
   DBG("connected=%hhu", connected);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Handles the "Name" property change from D-Bus.
 *
 * Updates the internal name and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_name_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   const char *name;

   if (!eldbus_message_iter_arguments_get(value, "s", &name))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (!eina_stringshare_replace(&pd->name, name)) return;
   DBG("name=%s", name);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Converts a ConnMan technology type string to an Efl_Net_Control_Technology_Type enum.
 *
 * @param str The technology type string (e.g., "wifi", "ethernet").
 * @return The corresponding Efl_Net_Control_Technology_Type enum value,
 *         or EFL_NET_CONTROL_TECHNOLOGY_TYPE_UNKNOWN if not found.
 */
Efl_Net_Control_Technology_Type
efl_net_connman_technology_type_from_str(const char *str)
{
   const struct {
      Efl_Net_Control_Technology_Type val;
      const char *str;
   } *itr, map[] = {
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_UNKNOWN, "unknown"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_SYSTEM, "system"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_ETHERNET, "ethernet"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_WIFI, "wifi"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_BLUETOOTH, "bluetooth"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_CELLULAR, "cellular"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_GPS, "gps"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_VPN, "vpn"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_GADGET, "gadget"},
     {EFL_NET_CONTROL_TECHNOLOGY_TYPE_P2P, "p2p"},
     { }
   };

   for (itr = map; itr->str != NULL; itr++)
     if (strcmp(itr->str, str) == 0) return itr->val;

   ERR("Unknown type '%s'", str);
   return EFL_NET_CONTROL_TECHNOLOGY_TYPE_UNKNOWN;
}

/**
 * @brief Handles the "Type" property change from D-Bus.
 *
 * Updates the internal technology type and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_type_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   const char *str;
   Efl_Net_Control_Technology_Type type;

   if (!eldbus_message_iter_arguments_get(value, "s", &str))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   type = efl_net_connman_technology_type_from_str(str);
   if (pd->type == type) return;
   pd->type = type;
   DBG("type=%d (%s)", type, str);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Handles the "Tethering" property change from D-Bus.
 *
 * Updates the internal tethering enabled state and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_tethering_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   Eina_Bool tethering;

   if (!eldbus_message_iter_arguments_get(value, "b", &tethering))
     {
        ERR("Expected boolean, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (pd->tethering.enabled == tethering) return;
   pd->tethering.enabled = tethering;
   DBG("tethering=%hhu", tethering);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Handles the "TetheringIdentifier" property change from D-Bus.
 *
 * Updates the internal tethering identifier and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_tethering_identifier_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   const char *tethering_identifier;

   if (!eldbus_message_iter_arguments_get(value, "s", &tethering_identifier))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (!eina_stringshare_replace(&pd->tethering.identifier, tethering_identifier)) return;
   DBG("tethering identifier=%s", tethering_identifier);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Handles the "TetheringPassphrase" property change from D-Bus.
 *
 * Updates the internal tethering passphrase and emits the 'changed' event.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param value The D-Bus message iterator containing the new property value.
 */
static void
_efl_net_control_technology_property_tethering_passphrase_changed(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *value)
{
   const char *tethering_passphrase;

   if (!eldbus_message_iter_arguments_get(value, "s", &tethering_passphrase))
     {
        ERR("Expected string, got %s", eldbus_message_iter_signature_get(value));
        return;
     }

   if (!eina_stringshare_replace(&pd->tethering.passphrase, tethering_passphrase)) return;
   DBG("tethering passphrase=%s", tethering_passphrase);
   efl_event_callback_call(o, EFL_NET_CONTROL_TECHNOLOGY_EVENT_CHANGED, NULL);
}

/**
 * @brief Internal handler for property changes.
 *
 * This function is called when a "PropertyChanged" D-Bus signal is received
 * or during initial property population. It dispatches to specific property
 * handlers based on the property name.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param itr The D-Bus message iterator containing the property name and value.
 *            Expected signature: "sv" (string name, variant value).
 */
static void
_efl_net_control_technology_property_changed_internal(Eo *o, Efl_Net_Control_Technology_Data *pd, Eldbus_Message_Iter *itr)
{
   Eldbus_Message_Iter *value;
   const char *name;

   if (!eldbus_message_iter_arguments_get(itr, "sv", &name, &value))
     {
        ERR("Unexpected signature: %s", eldbus_message_iter_signature_get(itr));
        return;
     }

   if (strcmp(name, "Powered") == 0)
     _efl_net_control_technology_property_powered_changed(o, pd, value);
   else if (strcmp(name, "Connected") == 0)
     _efl_net_control_technology_property_connected_changed(o, pd, value);
   else if (strcmp(name, "Name") == 0)
     _efl_net_control_technology_property_name_changed(o, pd, value);
   else if (strcmp(name, "Type") == 0)
     _efl_net_control_technology_property_type_changed(o, pd, value);
   else if (strcmp(name, "Tethering") == 0)
     _efl_net_control_technology_property_tethering_changed(o, pd, value);
   else if (strcmp(name, "TetheringIdentifier") == 0)
     _efl_net_control_technology_property_tethering_identifier_changed(o, pd, value);
   else if (strcmp(name, "TetheringPassphrase") == 0)
     _efl_net_control_technology_property_tethering_passphrase_changed(o, pd, value);
   else
     WRN("Unknown property name: %s", name);
}

/**
 * @brief D-Bus signal callback for "PropertyChanged" on net.connman.Technology.
 *
 * @param data The Efl_Net_Control_Technology object (user data).
 * @param msg The received D-Bus message.
 */
static void
_efl_net_control_technology_property_changed(void *data, const Eldbus_Message *msg)
{
   Eo *o = data;
   Efl_Net_Control_Technology_Data *pd = efl_data_scope_get(o, MY_CLASS);
   Eldbus_Message_Iter *itr;

   itr = eldbus_message_iter_get(msg);
   _efl_net_control_technology_property_changed_internal(o, pd, itr);
}

EOLIAN static void
_efl_net_control_technology_efl_object_destructor(Eo *o, Efl_Net_Control_Technology_Data *pd)
{
   Eldbus_Pending *p;
   Eldbus_Signal_Handler *sh;

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
   eina_stringshare_replace(&pd->tethering.identifier, NULL);
   eina_stringshare_replace(&pd->tethering.passphrase, NULL);
}

/**
 * @brief Callback for D-Bus SetProperty method calls.
 *
 * Handles the reply from ConnMan after attempting to set a property.
 *
 * @param data The Efl_Net_Control_Technology object (user data).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_efl_net_control_technology_property_set_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eo *o = data;
   Efl_Net_Control_Technology_Data *pd = efl_data_scope_get(o, MY_CLASS);
   const char *err_name, *err_msg;

   pd->pending = eina_list_remove(pd->pending, pending);
   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        ERR("Could not set property %p: %s=%s", o, err_name, err_msg);
        return;
     }
}

/**
 * @brief Sets a property on the ConnMan technology object via D-Bus.
 *
 * This is a generic helper function to call the SetProperty method on
 * net.connman.Technology.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param pd The private data of the technology object.
 * @param name The name of the property to set (e.g., "Powered").
 * @param signature The D-Bus signature of the property value (e.g., "b" for boolean).
 * @param ... The value(s) to set for the property, matching the signature.
 */
static void
_efl_net_control_technology_property_set(Eo *o, Efl_Net_Control_Technology_Data *pd, const char *name, const char *signature, ...)
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

   p = eldbus_proxy_send(pd->proxy, msg, _efl_net_control_technology_property_set_cb, o, DEFAULT_TIMEOUT);
   EINA_SAFETY_ON_NULL_GOTO(p, error_send);

   pd->pending = eina_list_append(pd->pending, p);
   DBG("Setting property %s", name);
   return;

 error_send:
   eldbus_message_unref(msg);
}

EOLIAN static void
_efl_net_control_technology_powered_set(Eo *o, Efl_Net_Control_Technology_Data *pd, Eina_Bool powered)
{
   _efl_net_control_technology_property_set(o, pd, "Powered", "b", powered);
}

EOLIAN static Eina_Bool
_efl_net_control_technology_powered_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Technology_Data *pd)
{
   return pd->powered;
}

EOLIAN static void
_efl_net_control_technology_tethering_set(Eo *o, Efl_Net_Control_Technology_Data *pd, Eina_Bool enabled, const char *identifier, const char *passphrase)
{
   if (passphrase)
     _efl_net_control_technology_property_set(o, pd, "TetheringPassphrase", "s", passphrase);

   if (identifier)
     _efl_net_control_technology_property_set(o, pd, "TetheringIdentifier", "s", identifier);

   _efl_net_control_technology_property_set(o, pd, "Tethering", "b", enabled);
}

EOLIAN static void
_efl_net_control_technology_tethering_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Technology_Data *pd, Eina_Bool *enabled, const char **identifier, const char **passphrase)
{
   if (enabled) *enabled = pd->tethering.enabled;
   if (identifier) *identifier = pd->tethering.identifier;
   if (passphrase) *passphrase = pd->tethering.passphrase;
}

EOLIAN static Eina_Bool
_efl_net_control_technology_connected_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Technology_Data *pd)
{
   return pd->connected;
}

EOLIAN static const char *
_efl_net_control_technology_efl_object_name_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Technology_Data *pd)
{
   return pd->name;
}

EOLIAN static Efl_Net_Control_Technology_Type
_efl_net_control_technology_type_get(const Eo *o EINA_UNUSED, Efl_Net_Control_Technology_Data *pd)
{
   return pd->type;
}

/**
 * @brief Callback for the D-Bus Scan method call.
 *
 * Handles the reply from ConnMan after a scan request. Resolves or rejects
 * the associated promise.
 *
 * @param data The Eina_Promise to be resolved/rejected.
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_efl_net_control_technology_scan_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eina_Promise *promise = data;
   const char *err_name, *err_msg;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
     {
        Eo *o = eldbus_pending_data_get(pending, ".object");
        Eina_Error err = EINVAL;

        if (strcmp(err_name, "net.connman.Error.NotSupported") == 0)
          err = ENOTSUP;

        WRN("Could not Scan %p: %s=%s", o, err_name, err_msg);
        eina_promise_reject(promise, err);
        return;
     }

   eina_promise_resolve(promise, EINA_VALUE_EMPTY);
}

/**
 * @brief Cancellation handler for the scan promise.
 *
 * Called when the future associated with a scan operation is cancelled.
 * It cancels the pending D-Bus call.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param data The Eldbus_Pending object associated with the scan.
 * @param error The cancellation error code.
 * @return An Eina_Value containing the error.
 */
static Eina_Value
_efl_net_control_technology_scan_promise_cancel(Eo *o EINA_UNUSED, void *data, Eina_Error error)
{
   Eldbus_Pending *p = data;

   if (error == ECANCELED)
     {
        DBG("cancel pending scan %p", p);
        eldbus_pending_cancel(p);
     }

   return eina_value_error_init(error);
}

/**
 * @brief Deletion handler for the scan promise data.
 *
 * Called when the future associated with a scan operation is freed.
 * It removes the pending D-Bus call from the internal list.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @param data The Eldbus_Pending object associated with the scan.
 * @param dead_future The future that is being freed.
 */
static void
_efl_net_control_technology_scan_promise_del(Eo *o, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Eldbus_Pending *p = data;
   Efl_Net_Control_Technology_Data *pd;

   pd = efl_data_scope_get(o, MY_CLASS);

   pd->pending = eina_list_remove(pd->pending, p);
}

EOLIAN static Eina_Future *
_efl_net_control_technology_scan(Eo *o, Efl_Net_Control_Technology_Data *pd)
{
   Eldbus_Pending *p;
   Eina_Promise *promise;
   Eina_Future *f = NULL;

   promise = efl_loop_promise_new(o);
   EINA_SAFETY_ON_NULL_RETURN_VAL(promise, NULL);

   f = eina_future_new(promise);

   p = eldbus_proxy_call(pd->proxy, "Scan",
                         _efl_net_control_technology_scan_cb, promise, -1.0, "");
   EINA_SAFETY_ON_NULL_GOTO(p, error_dbus);

   pd->pending = eina_list_append(pd->pending, p);
   eldbus_pending_data_set(p, ".object", o);

   return efl_future_then(o, f,
                          .error = _efl_net_control_technology_scan_promise_cancel,
                          .free = _efl_net_control_technology_scan_promise_del,
                          .data = p);

 error_dbus:
   eina_promise_reject(promise, ENOSYS);
   return efl_future_then(o, f);
}

/**
 * @brief Gets the D-Bus object path for a ConnMan technology.
 *
 * @param o The Efl_Net_Control_Technology object.
 * @return The D-Bus object path as a stringshare, or @c NULL on error.
 *         The returned stringshare should not be modified or freed by the caller.
 */
const char *
efl_net_connman_technology_path_get(Efl_Net_Control_Technology *o)
{
   Efl_Net_Control_Technology_Data *pd = efl_data_scope_get(o, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);
   return pd->path;
}

/**
 * @brief Creates a new Efl_Net_Control_Technology object for a ConnMan technology.
 *
 * This function is typically called by the Efl_Net_Control_Manager when a new
 * technology is discovered. It sets up the D-Bus proxy, signal handlers,
 * and populates initial properties.
 *
 * @param ctl The parent Efl_Net_Control_Manager object.
 * @param path The D-Bus object path of the technology (e.g., "/net/connman/technology/wifi").
 * @param itr A D-Bus message iterator positioned at the array of properties for this technology.
 *            Each entry in the array is a dictionary entry ('e') containing
 *            a property name (string 's') and its value (variant 'v').
 *            Example structure for `itr` pointing to properties:
 *            [
 *              { "Name", "WiFi" },
 *              { "Type", "wifi" },
 *              { "Powered", true },
 *              ...
 *            ]
 * @return A new Efl_Net_Control_Technology object, or @c NULL on failure.
 */
Efl_Net_Control_Technology *
efl_net_connman_technology_new(Efl_Net_Control_Manager *ctl, const char *path, Eldbus_Message_Iter *itr)
{
   Eo *o;
   Efl_Net_Control_Technology_Data *pd;
   Eldbus_Message_Iter *entry;
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
   pd->proxy = eldbus_proxy_get(obj, "net.connman.Technology");
   EINA_SAFETY_ON_NULL_GOTO(pd->proxy, error);

#define SH(sig, cb) \
   do { \
     Eldbus_Signal_Handler *sh = eldbus_proxy_signal_handler_add(pd->proxy, sig, cb, o); \
     if (sh) pd->signal_handlers = eina_list_append(pd->signal_handlers, sh); \
     else ERR("could not add DBus signal handler %s", sig); \
   } while (0)

   SH("PropertyChanged", _efl_net_control_technology_property_changed);
#undef SH

   efl_event_freeze(o);
   while (eldbus_message_iter_get_and_next(itr, 'e', &entry))
     _efl_net_control_technology_property_changed_internal(o, pd, entry);
   efl_event_thaw(o);

   return o;

 error:
   efl_del(o);
   return NULL;
}

#include "efl_net_control_technology.eo.c"
