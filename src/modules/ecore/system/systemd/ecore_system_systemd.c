#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eldbus.h>
#include <Ecore.h>
#include <locale.h>

/** @private Log domain for this module */
static int _log_dom = -1;
/** @private Eldbus system connection */
static Eldbus_Connection *_conn = NULL;

/** @private List of Eldbus_Object instances */
static Eina_List *_objs = NULL;
/** @private List of Eldbus_Proxy instances */
static Eina_List *_proxies = NULL;
/** @private List of Eldbus_Pending calls */
static Eina_List *_eldbus_pending = NULL;

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

/**
 * @brief Callback for D-Bus PropertiesChanged signal on org.freedesktop.hostname1.
 *
 * This function is triggered when properties of the hostname1 service change.
 * It checks if the "Hostname" property was among the changed or invalidated
 * properties and, if so, emits an ECORE_EVENT_HOSTNAME_CHANGED event.
 *
 * @param data Unused user data.
 * @param msg The Eldbus_Message containing signal data.
 *            Expected arguments: "sa{sv}as" (interface_name, changed_properties, invalidated_properties)
 *            - changed_properties: dict of property name (string) to new value (variant)
 *            - invalidated_properties: array of invalidated property names (string)
 */
static void
_props_changed_hostname(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
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
        if (strcmp(key, "Hostname") == 0)
          goto changed_hostname;
     }

   while (eldbus_message_iter_get_and_next(invalidated, 's', &prop))
     {
        if (strcmp(prop, "Hostname") == 0)
          goto changed_hostname;
     }

   return;

 changed_hostname:
   ecore_event_add(ECORE_EVENT_HOSTNAME_CHANGED, NULL, NULL, NULL);
}

/**
 * @brief Callback for D-Bus PropertiesChanged signal on org.freedesktop.timedate1.
 *
 * This function is triggered when properties of the timedate1 service change.
 * It checks if the "Timezone" property was among the changed or invalidated
 * properties and, if so, emits an ECORE_EVENT_SYSTEM_TIMEDATE_CHANGED event.
 *
 * @param data Unused user data.
 * @param msg The Eldbus_Message containing signal data.
 *            Expected arguments: "sa{sv}as" (interface_name, changed_properties, invalidated_properties)
 */
static void
_props_changed_timedate(void *data EINA_UNUSED, const Eldbus_Message *msg)
{
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
        if (strcmp(key, "Timezone") == 0)
          goto changed_timedate;
     }

   while (eldbus_message_iter_get_and_next(invalidated, 's', &prop))
     {
        if (strcmp(prop, "Timezone") == 0)
          goto changed_timedate;
     }

   return;

 changed_timedate:
   ecore_event_add(ECORE_EVENT_SYSTEM_TIMEDATE_CHANGED, NULL, NULL, NULL);
}

/**
 * @brief Unsets all LC_* environment variables.
 *
 * This is a helper function to clear existing locale settings before
 * applying new ones fetched from systemd.
 */
static void _locale_envs_unset(void)
{
   unsetenv("LC_CTYPE");
   unsetenv("LC_NUMERIC");
   unsetenv("LC_TIME");
   unsetenv("LC_COLLATE");
   unsetenv("LC_MONETARY");
   unsetenv("LC_MESSAGES");
   unsetenv("LC_ALL");
   unsetenv("LC_PAPER");
   unsetenv("LC_NAME");
   unsetenv("LC_ADDRESS");
   unsetenv("LC_TELEPHONE");
   unsetenv("LC_MEASUREMENT");
   unsetenv("LC_IDENTIFICATION");
}

/**
 * @brief Callback for eldbus_proxy_property_get for the "Locale" property.
 *
 * This function is called when the "Locale" property is successfully fetched
 * from org.freedesktop.locale1. It parses the array of locale strings
 * (e.g., "LANG=en_US.UTF-8"), unsets existing LC_* environment variables,
 * sets the new ones, updates the C library locale via setlocale(), and
 * finally emits an ECORE_EVENT_LOCALE_CHANGED event.
 *
 * @param data Unused user data.
 * @param msg The Eldbus_Message containing the property value.
 *            Expected arguments: "v" (variant), where the variant contains an array of strings "as".
 *            Each string is typically in "VARIABLE=value" format (e.g., "LANG=en_US.UTF-8").
 * @param pending The Eldbus_Pending object for this call.
 */
static void _locale_get(void *data EINA_UNUSED, const Eldbus_Message *msg,
                        Eldbus_Pending *pending)
{
   Eldbus_Message_Iter *variant, *array;
   const char *errname, *errmsg, *val;

   _eldbus_pending = eina_list_remove(_eldbus_pending, pending);
   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Message error %s - %s", errname, errmsg);
        goto end;
     }
   if (!eldbus_message_arguments_get(msg, "v", &variant))
     {
        ERR("Error getting arguments.");
        goto end;
     }

   if (!eldbus_message_iter_get_and_next(variant, 'a', &array))
     {
        ERR("Error getting array.");
        goto end;
     }

   _locale_envs_unset();

   while (eldbus_message_iter_get_and_next(array, 's', &val))
     {
        char buf[1024], *value, *type;

        snprintf(buf, sizeof(buf), "%s", val);

        type = buf;

        value = strchr(buf, '=');
        if (!value)
          continue;
        *value = 0;
        value++;

        setenv(type, value, 1);
     }
   setlocale(LC_ALL, "");

 end:
   ecore_event_add(ECORE_EVENT_LOCALE_CHANGED, NULL, NULL, NULL);
}

/**
 * @brief Callback for D-Bus PropertiesChanged signal on org.freedesktop.locale1.
 *
 * This function is triggered when properties of the locale1 service change.
 * It checks if the "Locale" property was among the changed or invalidated
 * properties. If so, it initiates an asynchronous request to get the current
 * value of the "Locale" property.
 *
 * @param data The Eldbus_Proxy for org.freedesktop.locale1, passed as user data.
 * @param msg The Eldbus_Message containing signal data.
 *            Expected arguments: "sa{sv}as" (interface_name, changed_properties, invalidated_properties)
 */
static void
_props_changed_locale(void *data, const Eldbus_Message *msg)
{
   Eldbus_Proxy *proxy = data;
   Eldbus_Message_Iter *changed, *entry, *invalidated;
   const char *iface, *prop;
   Eldbus_Pending *pend;

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
        if (strcmp(key, "Locale") == 0)
          goto changed_locale;
     }

   while (eldbus_message_iter_get_and_next(invalidated, 's', &prop))
     {
        if (strcmp(prop, "Locale") == 0)
          goto changed_locale;
     }

   return;

 changed_locale:
   pend = eldbus_proxy_property_get(proxy, "Locale", _locale_get, NULL);
   _eldbus_pending = eina_list_append(_eldbus_pending, pend);
}

/**
 * @brief Sets up monitoring for property changes on a D-Bus interface.
 *
 * This function gets a D-Bus object and proxy for the given service name,
 * object path, and interface. It then registers a callback to be invoked
 * when the PropertiesChanged signal is emitted for that interface.
 *
 * @param name The D-Bus service name (e.g., "org.freedesktop.hostname1").
 * @param path The D-Bus object path (e.g., "/org/freedesktop/hostname1").
 * @param iface The D-Bus interface name (e.g., "org.freedesktop.hostname1").
 * @param cb The callback function to invoke on PropertiesChanged signal.
 *           The Eldbus_Proxy for this interface will be passed as user data to the callback.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_property_change_monitor(const char *name,
                         const char *path,
                         const char *iface,
                         Eldbus_Signal_Cb cb)
{
   Eldbus_Object *o;
   Eldbus_Proxy *p;
   Eldbus_Signal_Handler *s;

   o = eldbus_object_get(_conn, name, path);
   if (!o)
     {
        ERR("could not get object name=%s, path=%s", name, path);
        return EINA_FALSE;
     }

   p = eldbus_proxy_get(o, iface);
   if (!p)
     {
        ERR("could not get proxy interface=%s, name=%s, path=%s",
            iface, name, path);
        eldbus_object_unref(o);
        return EINA_FALSE;
     }

   s = eldbus_proxy_properties_changed_callback_add(p, cb, p);
   if (!s)
     {
        ERR("could not add signal handler for properties changed for proxy "
            "interface=%s, name=%s, path=%s", iface, name, path);
        eldbus_proxy_unref(p);
        eldbus_object_unref(o);
        return EINA_FALSE;
     }

   _objs = eina_list_append(_objs, o);
   _proxies = eina_list_append(_proxies, p);
   return EINA_TRUE;
}

static void _ecore_system_systemd_shutdown(void);
static Eina_Bool _ecore_system_systemd_init(void);
/** @private Flag to indicate if the module is currently resetting. */
static unsigned int reseting = 0;

/**
 * @brief Resets the ecore_system_systemd module.
 *
 * This function is registered as an ecore_fork_reset_callback. It is called
 * after a fork to re-initialize the D-Bus connection and related resources
 * in the new process. It sets the `reseting` flag to prevent re-registering
 * the fork reset callback during re-initialization.
 *
 * @param data Unused user data.
 */
static void
_ecore_system_systemd_reset(void *data EINA_UNUSED)
{
   reseting = 1;
   _ecore_system_systemd_shutdown();
   _ecore_system_systemd_init();
   reseting = 0;
}

/**
 * @brief Initializes the ecore_system_systemd module.
 *
 * This function performs the following steps:
 * 1. Initializes Eldbus.
 * 2. Registers a fork reset callback (`_ecore_system_systemd_reset`) if not already resetting.
 * 3. Registers a log domain "ecore_system_systemd".
 * 4. Establishes a connection to the system D-Bus.
 * 5. Sets up property change monitoring for:
 *    - org.freedesktop.hostname1 (for hostname changes)
 *    - org.freedesktop.timedate1 (for timezone changes)
 *    - org.freedesktop.locale1 (for locale changes)
 * If any step fails, it performs a shutdown and returns EINA_FALSE.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_system_systemd_init(void)
{
   eldbus_init();
   if (!reseting)
     ecore_fork_reset_callback_add(_ecore_system_systemd_reset, NULL);

   _log_dom = eina_log_domain_register("ecore_system_systemd", NULL);
   if (_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ecore_system_systemd");
        goto error;
     }

   _conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);

   if (!_property_change_monitor("org.freedesktop.hostname1",
                                 "/org/freedesktop/hostname1",
                                 "org.freedesktop.hostname1",
                                 _props_changed_hostname))
     goto error;

   if (!_property_change_monitor("org.freedesktop.timedate1",
                                 "/org/freedesktop/timedate1",
                                 "org.freedesktop.timedate1",
                                 _props_changed_timedate))
     goto error;

   if (!_property_change_monitor("org.freedesktop.locale1",
                                 "/org/freedesktop/locale1",
                                 "org.freedesktop.locale1",
                                 _props_changed_locale))
     goto error;

   DBG("ecore system 'systemd' loaded");
   return EINA_TRUE;

 error:
   _ecore_system_systemd_shutdown();
   return EINA_FALSE;
}

/**
 * @brief Shuts down the ecore_system_systemd module.
 *
 * This function performs the following cleanup steps:
 * 1. Unregisters the fork reset callback if not currently resetting.
 * 2. Unrefs all stored Eldbus_Proxy instances.
 * 3. Unrefs all stored Eldbus_Object instances.
 * 4. Unrefs the Eldbus_Connection.
 * 5. Unregisters the log domain.
 * 6. Cancels any pending Eldbus calls.
 * 7. Shuts down Eldbus.
 */
static void
_ecore_system_systemd_shutdown(void)
{
   Eldbus_Pending *pend;

   DBG("ecore system 'systemd' unloaded");
   if (!reseting)
     ecore_fork_reset_callback_del(_ecore_system_systemd_reset, NULL);

   while (_proxies)
     {
        eldbus_proxy_unref(_proxies->data);
        _proxies = eina_list_remove_list(_proxies, _proxies);
     }

   while (_objs)
     {
        eldbus_object_unref(_objs->data);
        _objs = eina_list_remove_list(_objs, _objs);
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

   EINA_LIST_FREE(_eldbus_pending, pend)
     {
        eldbus_pending_cancel(pend);
     }

   eldbus_shutdown();
}

EINA_MODULE_INIT(_ecore_system_systemd_init);
EINA_MODULE_SHUTDOWN(_ecore_system_systemd_shutdown);
