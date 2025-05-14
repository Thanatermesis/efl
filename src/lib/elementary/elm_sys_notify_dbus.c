#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"

#include "elm_sys_notify_interface_eo.h"
#include "elm_sys_notify_eo.h"
#include "elm_sys_notify_dbus_eo.h"
#include "elm_sys_notify_dbus_eo.legacy.h"

#define MY_CLASS ELM_SYS_NOTIFY_DBUS_CLASS

#define OBJ       "/org/freedesktop/Notifications" /**< D-Bus object path for notifications */
#define BUS       "org.freedesktop.Notifications" /**< D-Bus bus name for notifications */
#define INTERFACE "org.freedesktop.Notifications" /**< D-Bus interface name for notifications */

static Eldbus_Connection *_elm_sysnotif_conn  = NULL; /**< Eldbus connection handle */
static Eldbus_Object     *_elm_sysnotif_obj   = NULL; /**< Eldbus object for notifications */
static Eldbus_Proxy      *_elm_sysnotif_proxy = NULL; /**< Eldbus proxy for notifications */

static Eina_Bool _has_markup = EINA_FALSE; /**< Flag indicating if the server supports markup for notification body */

/**
 * @brief Structure to hold data for asynchronous notification sending.
 */
typedef struct _Elm_Sys_Notify_Send_Data
{
   Elm_Sys_Notify_Send_Cb cb; /**< Callback function to be called after sending the notification. */
   const void *data;          /**< User data to be passed to the callback function. */
} Elm_Sys_Notify_Send_Data;

/**
 * @brief Marshals a key-value pair where the value is a byte into a D-Bus message iterator.
 *
 * This function is used to add entries to a dictionary of variants (a{sv})
 * in a D-Bus message, specifically for hint values.
 *
 * @param array The D-Bus message iterator for the array (dictionary).
 * @param key The string key for the dictionary entry.
 * @param value The byte value for the dictionary entry.
 */
static void
_elm_sys_notify_marshal_dict_byte(Eldbus_Message_Iter *array,
                                  const char *key,
                                  const char value)
{
   Eldbus_Message_Iter *var, *entry;

   eldbus_message_iter_arguments_append(array, "{sv}", &entry);
   eldbus_message_iter_basic_append(entry, 's', key);

   var = eldbus_message_iter_container_new(entry, 'v', "y");
   eldbus_message_iter_basic_append(var, 'y', value);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(array, entry);
}

/**
 * @brief Marshals a key-value pair where the value is a string into a D-Bus message iterator.
 *
 * This function is used to add entries to a dictionary of variants (a{sv})
 * in a D-Bus message, specifically for hint values.
 *
 * @param array The D-Bus message iterator for the array (dictionary).
 * @param key The string key for the dictionary entry.
 * @param value The string value for the dictionary entry.
 */
static void
_elm_sys_notify_marshal_dict_string(Eldbus_Message_Iter *array,
                                   const char *key,
                                   const char *value)
{
   Eldbus_Message_Iter *var, *entry;

   eldbus_message_iter_arguments_append(array, "{sv}", &entry);
   eldbus_message_iter_basic_append(entry, 's', key);

   var = eldbus_message_iter_container_new(entry, 'v', "s");
   eldbus_message_iter_basic_append(var, 's', value);
   eldbus_message_iter_container_close(entry, var);
   eldbus_message_iter_container_close(array, entry);
}

/**
 * @brief Callback function for the GetCapabilities D-Bus method call.
 *
 * Parses the reply from the notification server to check for supported
 * capabilities, specifically "body-markup".
 *
 * @param data User data (unused).
 * @param msg The D-Bus message reply.
 * @param pending The Eldbus pending object (unused).
 */
static void
_get_capabilities_cb(void *data EINA_UNUSED,
                     const Eldbus_Message *msg,
                     Eldbus_Pending *pending EINA_UNUSED)
{
   char *val;
   Eldbus_Message_Iter *arr;

   if (eldbus_message_error_get(msg, NULL, NULL) ||
       !eldbus_message_arguments_get(msg, "as", &arr)) goto end;

   while (eldbus_message_iter_get_and_next(arr, 's', &val))
     if (!strcmp(val, "body-markup"))
       {
          _has_markup = EINA_TRUE;
          return;
       }

end:
   _has_markup = EINA_FALSE;
}

/**
 * @brief Initiates a D-Bus call to get capabilities of the notification server.
 *
 * This function calls the "GetCapabilities" method on the
 * org.freedesktop.Notifications interface. The result is handled by
 * _get_capabilities_cb, which checks for "body-markup" support.
 */
void
_elm_sys_notify_capabilities_get(void)
{
   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   if (!eldbus_proxy_call(_elm_sysnotif_proxy, "GetCapabilities",
                         _get_capabilities_cb, NULL, -1, ""))
     ERR("Error sending message: "INTERFACE".GetCapabilities.");
}

/**
 * @brief Callback function for the CloseNotification D-Bus method call.
 *
 * Logs any errors encountered during the CloseNotification call.
 *
 * @param data User data (unused).
 * @param msg The D-Bus message reply.
 * @param pending The Eldbus pending object (unused).
 */
static void
_close_notification_cb(void *data EINA_UNUSED,
                       const Eldbus_Message *msg,
                       Eldbus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        if (errmsg && errmsg[0] == '\0')
          INF("Notification no longer exists.");
        else
          ERR("Eldbus Error: %s %s", errname, errmsg);
     }
}

EOLIAN static void
_elm_sys_notify_dbus_elm_sys_notify_interface_close(const Eo     *obj EINA_UNUSED,
                                                    void         *sd  EINA_UNUSED,
                                                    unsigned int  id)
{
   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   // Call the D-Bus method "CloseNotification"
   // The signature is "u" for the notification ID.
   if (!eldbus_proxy_call(_elm_sysnotif_proxy, "CloseNotification",
                          _close_notification_cb, NULL, -1, "u", id))
     ERR("Error sending message: "INTERFACE".CloseNotification.");
}

/**
 * @brief Callback function for the Notify D-Bus method call.
 *
 * Handles the reply from the notification server after a notification is sent.
 * It extracts the notification ID and calls the user-provided callback.
 *
 * @param data Pointer to Elm_Sys_Notify_Send_Data containing the user callback and data.
 * @param msg The D-Bus message reply.
 * @param pending The Eldbus pending object (unused).
 */
static void
_notify_cb(void *data,
           const Eldbus_Message *msg,
           Eldbus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;
   Elm_Sys_Notify_Send_Data *d = data;
   unsigned int id = 0;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     ERR("Error: %s %s", errname, errmsg);
   else if (!eldbus_message_arguments_get(msg, "u", &id))
     {
        ERR("Error getting return values of "INTERFACE".Notify.");
        id = 0;
     }

   if (d->cb) d->cb((void *)d->data, id);
   free(d);
}

EOLIAN static void
_elm_sys_notify_dbus_elm_sys_notify_interface_send(const Eo *obj EINA_UNUSED,
                                                   void *sd EINA_UNUSED,
                                                   unsigned int replaces_id,
                                                   const char *icon,
                                                   const char *summary,
                                                   const char *body,
                                                   Elm_Sys_Notify_Urgency urgency,
                                                   int timeout,
                                                   Elm_Sys_Notify_Send_Cb cb,
                                                   const void *cb_data)
{
   Eldbus_Message *msg;
   Eldbus_Message_Iter *iter, *actions, *hints; // D-Bus message iterators
   Elm_Sys_Notify_Send_Data *data;
   char *body_free = NULL;
   char *desk_free = NULL;
   const char *deskentry = elm_app_desktop_entry_get();
   const char *appname = elm_app_name_get();

   EINA_SAFETY_ON_NULL_RETURN(_elm_sysnotif_proxy);

   data = malloc(sizeof(Elm_Sys_Notify_Send_Data));
   EINA_SAFETY_ON_NULL_GOTO(data, error);
   data->cb = cb;       // Store user callback
   data->data = cb_data; // Store user data for the callback

   // Ensure strings are not NULL, use empty strings instead.
   if (!icon) icon = "";
   if (!summary) summary = "";
   if (!body)
     body = "";
   else if (!_has_markup)
     body = body_free = elm_entry_markup_to_utf8(body);

   msg = eldbus_proxy_method_call_new(_elm_sysnotif_proxy, "Notify");

   iter = eldbus_message_iter_get(msg);
   // Append arguments for Notify method:
   // s: app_name (string)
   // u: replaces_id (uint32)
   // s: app_icon (string)
   // s: summary (string)
   // s: body (string)
   // as: actions (array of strings) - currently empty
   eldbus_message_iter_arguments_append(iter, "susssas", appname, replaces_id,
                                        icon, summary, body, &actions);
   /* actions: an array of pairs of strings.
    * Each pair is (action_key, human_readable_name).
    * Example: ["close", "Close", "open", "Open Link"]
    * For now, we send an empty array.
    */
   eldbus_message_iter_container_close(iter, actions);

   /* hints: a dictionary of (string, variant)
    * Example: {"urgency": <byte 0>, "desktop-entry": <string "myapp">}
    */
   eldbus_message_iter_arguments_append(iter, "a{sv}", &hints);
   _elm_sys_notify_marshal_dict_byte(hints, "urgency", (char) urgency);

   // Add "desktop-entry" hint if available
   if (strcmp(deskentry, ""))
     {
        // Get the base name of the desktop entry file (e.g., "myapp" from "/path/to/myapp.desktop")
        deskentry = ecore_file_file_get(deskentry);
        deskentry = desk_free = ecore_file_strip_ext(deskentry);
        _elm_sys_notify_marshal_dict_string(hints, "desktop_entry", deskentry);
     }
   eldbus_message_iter_container_close(iter, hints);

   /* timeout: integer (milliseconds) or -1 for server default, 0 for persistent */
   eldbus_message_iter_arguments_append(iter, "i", timeout);

   // Send the D-Bus message asynchronously
   eldbus_proxy_send(_elm_sysnotif_proxy, msg, _notify_cb, data, -1);
   free(desk_free);
   free(body_free);
   return;

error:
   if (cb) cb((void *)cb_data, 0);
}

EOLIAN static void
_elm_sys_notify_dbus_elm_sys_notify_interface_simple_send(const Eo   *obj,
                                                          void       *sd,
                                                          const char *icon,
                                                          const char *summary,
                                                          const char *body)
{
   // Call the more general send function with default values:
   // replaces_id = 0 (new notification)
   // urgency = ELM_SYS_NOTIFY_URGENCY_NORMAL
   // timeout = -1 (server default)
   // cb = NULL (no callback)
   // cb_data = NULL
   _elm_sys_notify_dbus_elm_sys_notify_interface_send(obj, sd,
                                                      0, icon, summary, body,
                                                      ELM_SYS_NOTIFY_URGENCY_NORMAL,
                                                      -1, NULL, NULL);
}

/**
 * @brief Callback for the "NotificationClosed" D-Bus signal.
 *
 * This function is invoked when the notification server emits the
 * "NotificationClosed" signal. It parses the signal arguments (notification ID
 * and reason) and emits an Ecore event ELM_EVENT_SYS_NOTIFY_NOTIFICATION_CLOSED.
 *
 * @param data User data (unused).
 * @param msg The D-Bus signal message.
 */
static void
_on_notification_closed(void *data EINA_UNUSED,
                        const Eldbus_Message *msg)
{
   const char *errname;
   const char *errmsg;
   Elm_Sys_Notify_Notification_Closed *d;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Error: %s %s", errname, errmsg);
        return;
     }

   d = malloc(sizeof(*d));

   if (!eldbus_message_arguments_get(msg, "uu", &(d->id), &(d->reason)))
     {
        ERR("Error processing signal: "INTERFACE".NotificationClosed.");
        goto cleanup;
     }

   if (!ecore_event_add(ELM_EVENT_SYS_NOTIFY_NOTIFICATION_CLOSED, d,
                        NULL, NULL)) goto cleanup;

   return;

cleanup:
   free(d);
}

/**
 * @brief Frees the data associated with an ELM_EVENT_SYS_NOTIFY_ACTION_INVOKED event.
 *
 * This function is used as a callback by Ecore when the event data
 * for ELM_EVENT_SYS_NOTIFY_ACTION_INVOKED is no longer needed.
 *
 * @param data User data (unused).
 * @param ev_data Pointer to the Elm_Sys_Notify_Action_Invoked structure to be freed.
 */
static void
_ev_action_invoked_free(void *data EINA_UNUSED,
                        void *ev_data)
{
   Elm_Sys_Notify_Action_Invoked *d = ev_data;

   free(d->action_key); // Free the duplicated action key string
   free(d);             // Free the event structure itself
}

/**
 * @brief Callback for the "ActionInvoked" D-Bus signal.
 *
 * This function is invoked when the notification server emits the
 * "ActionInvoked" signal (e.g., user clicks an action button on a notification).
 * It parses the signal arguments (notification ID and action key) and emits an
 * Ecore event ELM_EVENT_SYS_NOTIFY_ACTION_INVOKED.
 *
 * @param data User data (unused).
 * @param msg The D-Bus signal message.
 */
static void
_on_action_invoked(void *data EINA_UNUSED,
                   const Eldbus_Message *msg)
{
   const char *errname;
   const char *aux;

   Elm_Sys_Notify_Action_Invoked *d;

   if (eldbus_message_error_get(msg, &errname, &aux))
     {
        ERR("Eldbus Error: %s %s", errname, aux);
        return;
     }

   d = calloc(1, sizeof(*d));
   if (!d)
     {
        ERR("Fail to allocate memory");
        return;
     }

   if (!eldbus_message_arguments_get(msg, "us", &(d->id), &aux))
     {
        ERR("Error processing signal: "INTERFACE".ActionInvoked.");
        goto cleanup;
     }

   d->action_key = strdup(aux);

   if (!ecore_event_add(ELM_EVENT_SYS_NOTIFY_ACTION_INVOKED, d,
                        _ev_action_invoked_free, NULL)) goto cleanup;

   return;

cleanup:
   free(d->action_key);
   free(d);
}

/**
 * @brief Releases Eldbus proxy and object resources.
 *
 * This function unrefs the Eldbus proxy and object if they exist,
 * effectively disconnecting from the notification service for these specific
 * handles. It does not unref the connection itself.
 */
static void
_release(void)
{
   if (_elm_sysnotif_proxy)
     {
        eldbus_proxy_unref(_elm_sysnotif_proxy);
        _elm_sysnotif_proxy = NULL;
     }

   if (_elm_sysnotif_obj)
     {
        eldbus_object_unref(_elm_sysnotif_obj);
        _elm_sysnotif_obj = NULL;
     }
}

/**
 * @brief Updates Eldbus object, proxy, and signal handlers.
 *
 * This function is called when the notification service becomes available
 * or its owner changes. It releases existing resources, re-acquires the
 * Eldbus object and proxy, fetches server capabilities, and re-registers
 * signal handlers for "NotificationClosed" and "ActionInvoked".
 */
static void
_update(void)
{
   _release(); // Release old proxy and object first
   _elm_sysnotif_obj = eldbus_object_get(_elm_sysnotif_conn, BUS, OBJ);
   _elm_sysnotif_proxy = eldbus_proxy_get(_elm_sysnotif_obj, INTERFACE);
   _elm_sys_notify_capabilities_get(); // Query server capabilities

   // Register for "NotificationClosed" signal
   eldbus_proxy_signal_handler_add(_elm_sysnotif_proxy, "NotificationClosed",
                                   _on_notification_closed, NULL);

   // Register for "ActionInvoked" signal
   eldbus_proxy_signal_handler_add(_elm_sysnotif_proxy, "ActionInvoked",
                                   _on_action_invoked, NULL);
}

/**
 * @brief Callback for eldbus_name_owner_get.
 *
 * This function is called after attempting to get the owner of the
 * notification service name on D-Bus. If successful (no error),
 * it calls _update() to set up the proxy and signal handlers.
 *
 * @param data User data (unused).
 * @param msg The D-Bus message reply.
 * @param pending The Eldbus pending object (unused).
 */
static void
_name_owner_get_cb(void *data EINA_UNUSED,
                   const Eldbus_Message *msg,
                   Eldbus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     ERR("Eldbus Error: %s %s", errname, errmsg);
   else
     _update();
}

/**
 * @brief Callback for D-Bus name owner changes.
 *
 * This function is invoked when the owner of the BUS
 * (org.freedesktop.Notifications) changes on D-Bus.
 * If a new owner appears, it calls _update() to re-establish communication.
 * If the owner disappears (new_id is NULL or empty), it calls _release()
 * to clean up resources.
 *
 * @param data User data (unused).
 * @param bus The bus name that changed owner (unused, should be BUS).
 * @param old_id The old owner ID (unused).
 * @param new_id The new owner ID. NULL or empty if the name has no owner.
 */
static void
_name_owner_changed_cb(void *data EINA_UNUSED,
                       const char *bus EINA_UNUSED,
                       const char *old_id EINA_UNUSED,
                       const char *new_id)
{
   if ((!new_id) || (*new_id == '\0'))
     _release();
   else
     _update();
}

/**
 * @brief Efl_Object constructor for Elm_Sys_Notify_Dbus.
 *
 * This constructor initializes the D-Bus connection for system notifications.
 * It ensures that only one instance of this object (singleton) is created.
 * It establishes a connection to the session bus, sets up a callback for
 * notification service owner changes, and attempts to get the current owner
 * to initialize the service proxy.
 *
 * @param obj The Efl_Object being constructed.
 * @param sd Private data for the object (unused).
 * @return The constructed Efl_Object, or NULL on failure or if already constructed.
 */
EOLIAN static Efl_Object *
_elm_sys_notify_dbus_efl_object_constructor(Eo   *obj,
                                         void *sd  EINA_UNUSED)
{
   /* Don't create the same object twice (singleton) */
   if (!_elm_sysnotif_conn) // Check if already initialized
     {
        if (!elm_need_eldbus()) return NULL; // Check if eldbus is available

        // Get a connection to the D-Bus session bus
        _elm_sysnotif_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
        if (!_elm_sysnotif_conn) return NULL; // Failed to get connection

        // Register a callback for when the owner of the notification service name changes
        eldbus_name_owner_changed_callback_add(_elm_sysnotif_conn, BUS,
                                               _name_owner_changed_cb, NULL,
                                               EINA_FALSE); // EINA_FALSE: do not allow pending_ jaane

        // Try to get the current owner of the notification service name
        // This will trigger _name_owner_get_cb, which then calls _update()
        eldbus_name_owner_get(_elm_sysnotif_conn, BUS, _name_owner_get_cb, NULL);

        obj = efl_constructor(efl_super(obj, MY_CLASS)); // Call parent constructor
        return obj;
     }

   ERR("Elm.Sys_Notify.Dbus is a singleton. It has already been created"); // Log error if already created
   return NULL;
}

/**
 * @brief Efl_Object destructor for Elm_Sys_Notify_Dbus.
 *
 * This destructor cleans up resources used by the D-Bus notification system.
 * It releases the Eldbus proxy and object, and unrefs the D-Bus connection.
 *
 * @param obj The Efl_Object being destructed.
 * @param sd Private data for the object (unused).
 */
EOLIAN static void
_elm_sys_notify_dbus_efl_object_destructor(Eo   *obj,
                                        void *sd  EINA_UNUSED)
{
   _release(); // Release proxy and object

   // Unreference the D-Bus connection
   if (_elm_sysnotif_conn)
     {
        eldbus_connection_unref(_elm_sysnotif_conn);
        _elm_sysnotif_conn = NULL;
     }
   efl_destructor(efl_super(obj, MY_CLASS)); // Call parent destructor
}


#include "elm_sys_notify_dbus_eo.c"

