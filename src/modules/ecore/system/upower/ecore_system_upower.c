/**
 * @file
 * @brief Ecore module for UPower integration.
 *
 * This module interfaces with the UPower D-Bus service to monitor power
 * status, such as battery level and AC power connection. It updates
 * Ecore's power state accordingly.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eldbus.h>
#include <Ecore.h>
#include <locale.h>

static int _log_dom = -1; /**< Log domain for this module. */
static Eldbus_Connection *_conn = NULL; /**< Eldbus connection to the system bus. */

static Eldbus_Object *_obj = NULL; /**< Eldbus object for org.freedesktop.UPower. */
static Eldbus_Proxy *_proxy = NULL; /**< Eldbus proxy for org.freedesktop.UPower interface. */

static Eldbus_Object *_disp_obj = NULL; /**< Eldbus object for the DisplayDevice. */
static Eldbus_Proxy *_disp_proxy = NULL; /**< Eldbus proxy for the DisplayDevice interface. */

/**
 * @brief Specifies the UPower version compatibility mode.
 *
 * UPower versions >= 0.99.0 use "WarningLevel" property, while older
 * versions use "OnLowBattery". This enum helps manage these differences.
 */
typedef enum {
     VERSION_ON_LOW_BATTERY,    /**< UPower version uses OnLowBattery property. */
     VERSION_WARNING_LEVEL      /**< UPower version uses WarningLevel property. */
}Ecore_System_Upower_Version;
static Ecore_System_Upower_Version _version = 0; /**< Detected UPower version compatibility. */

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)

static Eina_Bool _ecore_on_battery = EINA_FALSE; /**< EINA_TRUE if the system is on battery power. */
static Eina_Bool _ecore_low_battery = EINA_FALSE; /**< EINA_TRUE if the battery is low (used by older UPower). */
static int _ecore_battery_level = -1; /**< Battery warning level (0-4, used by newer UPower). */

static Eina_List *_eldbus_pending = NULL; /**< List of pending Eldbus calls. */

static Eina_Bool _ecore_system_upower_display_device_init(void);
static void _ecore_system_upower_shutdown(void);

/**
 * @brief Evaluates the current battery status and updates Ecore's power state.
 *
 * This function determines the overall power state (mains, battery, low battery)
 * based on the `_ecore_on_battery`, `_ecore_low_battery`, and
 * `_ecore_battery_level` global variables. It then calls
 * `ecore_power_state_set()` to notify the Ecore system.
 */
static void
_battery_eval(void)
{
   Ecore_Power_State power_state = ECORE_POWER_STATE_MAINS;

   if (_ecore_low_battery)
     {
        power_state = ECORE_POWER_STATE_LOW;
     }
   else if (_ecore_on_battery)
     {
        power_state = ECORE_POWER_STATE_BATTERY;

        /* FIXME: get level value from libupower? */
        if (_ecore_battery_level >= 3)
          {
             power_state = ECORE_POWER_STATE_LOW;
          }
     }

   ecore_power_state_set(power_state);
}

/**
 * @brief Extracts the battery warning level from an Eldbus variant.
 *
 * This function is called when the "WarningLevel" property is received
 * from UPower. It updates `_ecore_battery_level` and calls `_battery_eval`.
 * The warning level is an unsigned integer typically ranging from 0 (none)
 * to 4 (critically low), though UPower defines specific meanings.
 *
 * @param variant Pointer to the Eldbus message iterator containing the variant.
 *                The variant is expected to be of type 'u' (unsigned int).
 */
static void
_warning_level_from_variant(Eldbus_Message_Iter *variant)
{
   unsigned int val;

   if (!eldbus_message_iter_get_and_next(variant, 'u', &val))
     {
        ERR("Error getting WarningLevel.");
        return;
     }

   _ecore_battery_level = val;
   _battery_eval();
}

/**
 * @brief Callback for the asynchronous retrieval of the "WarningLevel" property.
 *
 * Handles the D-Bus reply for the "WarningLevel" property. On success,
 * it extracts the value using `_warning_level_from_variant`.
 *
 * @param data User data (unused).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object associated with this call.
 */
static void
_warning_level_get_cb(void *data EINA_UNUSED,
                      const Eldbus_Message *msg,
                      Eldbus_Pending *pending)
{
   Eldbus_Message_Iter *variant;
   const char *errname, *errmsg;

   _eldbus_pending = eina_list_remove(_eldbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
// don't print errors because this results in complaints about upower not
// existing and it's OK if it doesn't exist. just no feature enabled then
//        ERR("Message error %s - %s", errname, errmsg);
        return;
     }
   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        return;
     }

   _warning_level_from_variant(variant);
}

/**
 * @brief Initiates an asynchronous D-Bus call to get the "WarningLevel" property.
 *
 * @param proxy The Eldbus_Proxy for the UPower device.
 */
static void
_warning_level_get(Eldbus_Proxy *proxy)
{
   Eldbus_Pending *pend;

   pend = eldbus_proxy_property_get(proxy, "WarningLevel",
                                    _warning_level_get_cb, NULL);
   _eldbus_pending = eina_list_append(_eldbus_pending, pend);
}

/**
 * @brief Extracts the "OnLowBattery" status from an Eldbus variant.
 *
 * This function is called when the "OnLowBattery" property is received
 * from UPower (typically older versions). It updates `_ecore_low_battery`
 * and calls `_battery_eval`.
 *
 * @param variant Pointer to the Eldbus message iterator containing the variant.
 *                The variant is expected to be of type 'b' (boolean).
 */
static void
_on_low_battery_from_variant(Eldbus_Message_Iter *variant)
{
   Eina_Bool val;

   if (!eldbus_message_iter_get_and_next(variant, 'b', &val))
     {
        ERR("Error getting OnLowBattery.");
        return;
     }

   DBG("OnLowBattery=%hhu", val);
   _ecore_low_battery = val;
   _battery_eval();
}

/**
 * @brief Callback for the asynchronous retrieval of the "OnLowBattery" property.
 *
 * Handles the D-Bus reply for the "OnLowBattery" property. On success,
 * it extracts the value using `_on_low_battery_from_variant`.
 *
 * @param data User data (unused).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object associated with this call.
 */
static void
_on_low_battery_get_cb(void *data EINA_UNUSED, const Eldbus_Message *msg,
                        Eldbus_Pending *pending)
{
   Eldbus_Message_Iter *variant;
   const char *errname, *errmsg;

   _eldbus_pending = eina_list_remove(_eldbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        if (strcmp(errname, "org.enlightenment.DBus.Canceled"))
          ERR("Message error %s - %s", errname, errmsg);
        return;
     }
   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        return;
     }

   _on_low_battery_from_variant(variant);
}

/**
 * @brief Initiates an asynchronous D-Bus call to get the "OnLowBattery" property.
 *
 * @param proxy The Eldbus_Proxy for the UPower device.
 */
static void
_on_low_battery_get(Eldbus_Proxy *proxy)
{
   Eldbus_Pending *pend;

   pend = eldbus_proxy_property_get(proxy, "OnLowBattery",
                                    _on_low_battery_get_cb, NULL);
   _eldbus_pending = eina_list_append(_eldbus_pending, pend);
}

/**
 * @brief Extracts the "OnBattery" status from an Eldbus variant.
 *
 * This function is called when the "OnBattery" property is received
 * from UPower. It updates `_ecore_on_battery` and calls `_battery_eval`.
 *
 * @param variant Pointer to the Eldbus message iterator containing the variant.
 *                The variant is expected to be of type 'b' (boolean).
 */
static void
_on_battery_from_variant(Eldbus_Message_Iter *variant)
{
   Eina_Bool val;

   if (!eldbus_message_iter_get_and_next(variant, 'b', &val))
     {
        ERR("Error getting OnBattery.");
        return;
     }

   DBG("OnBattery=%hhu", val);
   _ecore_on_battery = val;
   _battery_eval();
}

/**
 * @brief Callback for the asynchronous retrieval of the "OnBattery" property.
 *
 * Handles the D-Bus reply for the "OnBattery" property. On success,
 * it extracts the value using `_on_battery_from_variant`.
 *
 * @param data User data (unused).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object associated with this call.
 */
static void
_on_battery_get_cb(void *data EINA_UNUSED, const Eldbus_Message *msg,
                        Eldbus_Pending *pending)
{
   Eldbus_Message_Iter *variant;
   const char *errname, *errmsg;

   _eldbus_pending = eina_list_remove(_eldbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        if (strcmp(errname, "org.enlightenment.DBus.Canceled"))
          ERR("Message error %s - %s", errname, errmsg);
        return;
     }
   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        return;
     }

   _on_battery_from_variant(variant);
}

/**
 * @brief Initiates an asynchronous D-Bus call to get the "OnBattery" property.
 *
 * @param proxy The Eldbus_Proxy for the UPower device.
 */
static void
_on_battery_get(Eldbus_Proxy *proxy)
{
   Eldbus_Pending *pend;

   pend = eldbus_proxy_property_get(proxy, "OnBattery",
                                    _on_battery_get_cb, NULL);
   _eldbus_pending = eina_list_append(_eldbus_pending, pend);
}

/**
 * @brief Retrieves the appropriate battery state property based on UPower version.
 *
 * If UPower version is `VERSION_ON_LOW_BATTERY`, it calls `_on_low_battery_get`.
 * If UPower version is `VERSION_WARNING_LEVEL`, it initializes the display
 * device if needed and then calls `_warning_level_get`.
 * It also always attempts to get the "OnBattery" status.
 */
static void
_battery_state_get()
{
   // Always get OnBattery status
   _on_battery_get(_proxy);

   switch (_version)
     {
      case VERSION_ON_LOW_BATTERY:
         _on_low_battery_get(_proxy);
         break;
      case VERSION_WARNING_LEVEL:
         if (_ecore_system_upower_display_device_init())
           _warning_level_get(_disp_proxy);
         break;
      default:
         break;
     }
}

/**
 * @brief Extracts the UPower daemon version from an Eldbus variant and determines compatibility.
 *
 * This function parses the "DaemonVersion" string (e.g., "0.99.7").
 * It compares the version against "0.99.0" to set the `_version`
 * global, which dictates whether to use "OnLowBattery" or "WarningLevel".
 * After determining the version, it calls `_battery_state_get` to fetch
 * initial battery status.
 *
 * @param variant Pointer to the Eldbus message iterator containing the variant.
 *                The variant is expected to be of type 's' (string).
 */
static void
_daemon_version_from_variant(Eldbus_Message_Iter *variant)
{
   const char *val;
   char **version;
   int standard[3] = {0, 99, 0}; // upower >= 0.99.0 provides WarningLevel instead of OnLowBattery
   int i;

   if (!eldbus_message_iter_get_and_next(variant, 's', &val))
     {
        ERR("Error getting DaemonVersion.");
        return;
     }
   version = eina_str_split(val, ".", 3);

   for (i = 0; i < 3; i ++)
     {
        if (atoi(version[i]) > standard[i])
          {
             _version = VERSION_WARNING_LEVEL;
             break;
          }
        else if (atoi(version[i]) < standard[i])
          {
             _version = VERSION_ON_LOW_BATTERY;
             break;
          }
        else if (i == 2)
          {
             _version = VERSION_WARNING_LEVEL;
             break;
          }
     }
   free(version[0]);
   free(version);

   _battery_state_get();
}

/**
 * @brief Callback for the asynchronous retrieval of the "DaemonVersion" property.
 *
 * Handles the D-Bus reply for the "DaemonVersion" property. On success,
 * it extracts the value using `_daemon_version_from_variant`.
 *
 * @param data User data (unused).
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object associated with this call.
 */
static void
_daemon_version_get_cb(void *data EINA_UNUSED, const Eldbus_Message *msg,
                          Eldbus_Pending *pending)
{
   Eldbus_Message_Iter *variant;
   const char *errname, *errmsg;

   _eldbus_pending = eina_list_remove(_eldbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        if (strcmp(errname, "org.enlightenment.DBus.Canceled"))
          ERR("Message error %s - %s", errname, errmsg);
        return;
     }
   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        return;
     }

   _daemon_version_from_variant(variant);
}

/**
 * @brief Initiates an asynchronous D-Bus call to get the "DaemonVersion" property.
 *
 * @param proxy The Eldbus_Proxy for the UPower service.
 */
static void
_daemon_version_get(Eldbus_Proxy *proxy)
{
   Eldbus_Pending *pend;

   pend = eldbus_proxy_property_get(proxy, "DaemonVersion",
                                    _daemon_version_get_cb, NULL);
   _eldbus_pending = eina_list_append(_eldbus_pending, pend);
}

/**
 * @brief Handles the "PropertiesChanged" D-Bus signal from UPower.
 *
 * This function is called when UPower properties (like "OnBattery",
 * "OnLowBattery", "WarningLevel") change. It parses the signal arguments
 * to identify which properties changed and updates the local state accordingly.
 * If a property is invalidated, it re-fetches that property.
 *
 * @param data The Eldbus_Proxy associated with the signal.
 * @param msg The D-Bus signal message. The message arguments are expected to be:
 *            - "s": interface_name (string)
 *            - "a{sv}": changed_properties (array of dictionary entries string to variant)
 *              - Example: {"OnBattery": <true>}, {"WarningLevel": <1>}
 *            - "as": invalidated_properties (array of strings)
 *              - Example: ["OnBattery"]
 */
static void
_props_changed(void *data, const Eldbus_Message *msg)
{
   Eldbus_Proxy *proxy = data;
   Eldbus_Message_Iter *changed, *entry, *invalidated;
   const char *iface, *prop;

   if (!eldbus_message_arguments_get(msg, "sa{sv}as",
                                     &iface, &changed, &invalidated))
     {
        ERR("Error getting data from properties changed signal.");
        return;
     }

   while (eldbus_message_iter_get_and_next(changed, 'e', &entry))
     {
        const void *key;
        Eldbus_Message_Iter *var;
        if (!eldbus_message_iter_arguments_get(entry, "sv", &key, &var))
          continue;
        if (strcmp(key, "OnBattery") == 0)
          _on_battery_from_variant(var);
        if (strcmp(key, "OnLowBattery") == 0)
          _on_low_battery_from_variant(var);
        if (strcmp(key, "WarningLevel") == 0)
          _warning_level_from_variant(var);
     }

   while (eldbus_message_iter_get_and_next(invalidated, 's', &prop))
     {
        if (strcmp(prop, "OnBattery") == 0)
          _on_battery_get(proxy);
        if (strcmp(prop, "OnLowBattery") == 0)
          _on_low_battery_get(proxy);
        if (strcmp(prop, "WarningLevel") == 0)
          _warning_level_get(proxy);
     }
}

/**
 * @brief Callback for UPower D-Bus service name owner changes.
 *
 * This function is invoked when the `org.freedesktop.UPower` D-Bus name
 * owner changes (e.g., UPower service starts or restarts). If a new owner
 * appears, it re-fetches the daemon version to initialize or update
 * the UPower integration.
 *
 * @param data The Eldbus_Proxy for the UPower service.
 * @param bus The D-Bus name (unused, expected to be "org.freedesktop.UPower").
 * @param old_id The old unique name of the owner, or empty if none.
 * @param new_id The new unique name of the owner, or empty if none.
 */
static void _upower_name_owner_cb(void *data,
                                  const char *bus EINA_UNUSED,
                                  const char *old_id,
                                  const char *new_id)
{
   Eldbus_Proxy *proxy = data;

   DBG("org.freedesktop.UPower name owner changed from '%s' to '%s'",
       old_id, new_id);

   if ((new_id) && (new_id[0]))
     {
        _daemon_version_get(proxy);
     }
}

/**
 * @brief Initializes the D-Bus connection for the UPower DisplayDevice.
 *
 * This function sets up the Eldbus object and proxy for
 * `/org/freedesktop/UPower/devices/DisplayDevice`. This device is typically
 * used by newer UPower versions to report "WarningLevel". It also registers
 * a "PropertiesChanged" signal handler for this device.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_system_upower_display_device_init(void)
{
   Eldbus_Signal_Handler *s;

   _disp_obj =
      eldbus_object_get(_conn, "org.freedesktop.UPower",
                        "/org/freedesktop/UPower/devices/DisplayDevice");
   if (!_disp_obj)
     {
        ERR("could not get object name=org.freedesktop.UPower, "
            "path=/org/freedesktop/UPower/devices/DisplayDevice");
        goto disp_error;
     }

   _disp_proxy = eldbus_proxy_get(_disp_obj, "org.freedesktop.UPower");
   if (!_disp_proxy)
     {
        ERR("could not get proxy interface=org.freedesktop.UPower, "
            "name=org.freedesktop.UPower, "
            "path=/org/freedesktop/UPower/devices/DisplayDevice");
        goto disp_error;
     }

   s = eldbus_proxy_properties_changed_callback_add(_disp_proxy,
                                                    _props_changed,
                                                    _disp_proxy);
   if (!s)
     {
        ERR("could not add signal handler for properties changed for proxy "
            "interface=org.freedesktop.UPower, "
            "name=org.freedesktop.UPower, "
            "path=/org/freedesktop/UPower/devices/DisplayDevice");
        goto disp_error;
     }

   return EINA_TRUE;

disp_error:
   _ecore_system_upower_shutdown();
   return EINA_FALSE;
}

static Eina_Bool _ecore_system_upower_init(void);
static void _ecore_system_upower_shutdown(void);
static unsigned int reseting; /**< Flag to indicate if a reset is in progress, to avoid re-entry issues. */

/**
 * @brief Resets the UPower module, typically after a fork.
 *
 * This function shuts down and re-initializes the UPower D-Bus connections
 * and state. It's registered as an ecore_fork_reset_callback.
 * The `reseting` flag prevents recursive calls or issues during the
 * shutdown/init sequence.
 *
 * @param data User data (unused).
 */
static void
_ecore_system_upower_reset(void *data EINA_UNUSED)
{
   reseting = 1;
   _ecore_system_upower_shutdown();
   _ecore_system_upower_init();
   reseting = 0;
}

/**
 * @brief Initializes the Ecore UPower module.
 *
 * This function performs the main initialization for UPower integration:
 * - Initializes Eldbus.
 * - Registers a fork reset callback (`_ecore_system_upower_reset`).
 * - Registers a log domain.
 * - Establishes a D-Bus connection to the system bus.
 * - Gets the D-Bus object and proxy for `org.freedesktop.UPower`.
 * - Registers a "PropertiesChanged" signal handler for the main UPower proxy.
 * - Registers a name owner changed callback to detect UPower service restarts.
 * - Initially, it will trigger `_daemon_version_get` via the name owner callback
 *   if UPower is already running, or when it starts.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_system_upower_init(void)
{
   Eldbus_Signal_Handler *s;

   eldbus_init();
   if (!reseting)
     ecore_fork_reset_callback_add(_ecore_system_upower_reset, NULL);

   _log_dom = eina_log_domain_register("ecore_system_upower", NULL);
   if (_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ecore_system_upower");
        goto error;
     }

   _conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);

   _obj = eldbus_object_get(_conn, "org.freedesktop.UPower",
                            "/org/freedesktop/UPower");
   if (!_obj)
     {
        ERR("could not get object name=org.freedesktop.UPower, "
            "path=/org/freedesktop/UPower");
        goto error;
     }

   _proxy = eldbus_proxy_get(_obj, "org.freedesktop.UPower");
   if (!_proxy)
     {
        ERR("could not get proxy interface=org.freedesktop.UPower, "
            "name=org.freedesktop.UPower, path=/org/freedesktop/UPower");
        goto error;
     }

   s = eldbus_proxy_properties_changed_callback_add(_proxy, _props_changed,
                                                    _proxy);
   if (!s)
     {
        ERR("could not add signal handler for properties changed for proxy "
            "interface=org.freedesktop.UPower, name=org.freedesktop.UPower, "
            "path=/org/freedesktop/UPower");
        goto error;
     }

   eldbus_name_owner_changed_callback_add(_conn, "org.freedesktop.UPower",
                                          _upower_name_owner_cb,
                                          _proxy, EINA_TRUE);

   DBG("ecore system 'upower' loaded");
   return EINA_TRUE;

 error:
   _ecore_system_upower_shutdown();
   return EINA_FALSE;
}

/**
 * @brief Shuts down the Ecore UPower module.
 *
 * This function cleans up all resources used by the UPower integration:
 * - Unregisters the fork reset callback.
 * - Deletes D-Bus name owner changed callback.
 * - Unrefs D-Bus proxies and objects for both the main UPower service
 *   and the DisplayDevice.
 * - Cancels any pending Eldbus calls.
 * - Unrefs the D-Bus connection.
 * - Unregisters the log domain.
 * - Shuts down Eldbus.
 *
 * This is called during module unload or when resetting.
 */
static void
_ecore_system_upower_shutdown(void)
{
   Eldbus_Pending *pend;

   DBG("ecore system 'upower' unloaded");
   if (!reseting)
     ecore_fork_reset_callback_del(_ecore_system_upower_reset, NULL);

   eldbus_name_owner_changed_callback_del(_conn, "org.freedesktop.UPower",
                                          _upower_name_owner_cb,
                                          NULL);
  if (_disp_proxy)
     {
        eldbus_proxy_unref(_disp_proxy);
        _disp_proxy = NULL;
     }

   if (_disp_obj)
     {
        eldbus_object_unref(_disp_obj);
        _disp_obj = NULL;
     }

   if (_proxy)
     {
        eldbus_proxy_unref(_proxy);
        _proxy = NULL;
     }

   if (_obj)
     {
        eldbus_object_unref(_obj);
        _obj = NULL;
     }

   EINA_LIST_FREE(_eldbus_pending, pend)
     {
        eldbus_pending_cancel(pend);
     }

   if (_conn)
     {
        eldbus_connection_unref(_conn);
        _conn = NULL;
     }

   if (_log_dom > 0)
     {
        eina_log_domain_unregister(_log_dom);
        _log_dom = -1;
     }

   eldbus_shutdown();
}

EINA_MODULE_INIT(_ecore_system_upower_init);
EINA_MODULE_SHUTDOWN(_ecore_system_upower_shutdown);
