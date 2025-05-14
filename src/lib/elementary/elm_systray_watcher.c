/**
 * @file
 * @brief Handles D-Bus communication with the StatusNotifierWatcher service.
 *
 * This file implements the client side for interacting with a
 * StatusNotifierWatcher, as defined by the freedesktop.org
 * StatusNotifierItem specification. It allows Elementary applications
 * to register their StatusNotifierItems (system tray icons) with the
 * watcher service.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"

#include "elm_systray_watcher.h"

#define OBJ       "/StatusNotifierWatcher" /**< D-Bus object path for the StatusNotifierWatcher */
#define BUS       "org.kde.StatusNotifierWatcher" /**< D-Bus bus name for the StatusNotifierWatcher */
#define INTERFACE "org.kde.StatusNotifierWatcher" /**< D-Bus interface name for the StatusNotifierWatcher */

static Eina_Bool _elm_systray_watcher = EINA_FALSE; /**< Flag indicating if the watcher system is initialized */

static Eldbus_Connection *_watcher_conn  = NULL; /**< D-Bus connection to the session bus */
static Eldbus_Object     *_watcher_obj   = NULL; /**< D-Bus object for the StatusNotifierWatcher */
static Eldbus_Proxy      *_watcher_proxy = NULL; /**< D-Bus proxy for the StatusNotifierWatcher interface */

/**
 * @brief Callback for the RegisterStatusNotifierItem D-Bus method call.
 * @param data User data (unused).
 * @param msg The D-Bus message reply.
 * @param pending The Eldbus_Pending object (unused).
 *
 * Logs an error if the D-Bus call failed.
 */
static void
_status_notifier_item_register_cb(void *data EINA_UNUSED,
                                  const Eldbus_Message *msg,
                                  Eldbus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     ERR("Eldbus Error: %s %s", errname, errmsg);
}

Eina_Bool
_elm_systray_watcher_status_notifier_item_register(const char *obj)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(_watcher_proxy, EINA_FALSE);

   // Call the RegisterStatusNotifierItem D-Bus method
   if (!eldbus_proxy_call(_watcher_proxy, "RegisterStatusNotifierItem",
                         _status_notifier_item_register_cb,
                         NULL, -1, "s", obj))
     {
        ERR("Error sending message: "INTERFACE".RegisterStatusNotifierItem.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Releases D-Bus proxy and object resources.
 *
 * Unreferences the watcher proxy and object if they exist.
 */
static void
_release(void)
{
   if (_watcher_proxy)
     {
        eldbus_proxy_unref(_watcher_proxy);
        _watcher_proxy = NULL;
     }

   if (_watcher_obj)
     {
        eldbus_object_unref(_watcher_obj);
        _watcher_obj = NULL;
     }
}

/**
 * @brief Updates the D-Bus proxy and object for the StatusNotifierWatcher.
 *
 * This function is called when the StatusNotifierWatcher service becomes available.
 * It releases any existing proxy/object and creates new ones.
 * It also emits an ELM_EVENT_SYSTRAY_READY event.
 */
static void
_update(void)
{
   _release();

   _watcher_obj = eldbus_object_get(_watcher_conn, BUS, OBJ);
   _watcher_proxy = eldbus_proxy_get(_watcher_obj, INTERFACE);

   // Notify that the systray watcher is ready (or re-established)
   ecore_event_add(ELM_EVENT_SYSTRAY_READY, NULL, NULL, NULL);
}

/**
 * @brief Callback for D-Bus name owner changes of the StatusNotifierWatcher service.
 * @param data User data (unused).
 * @param bus The D-Bus name (unused, should be BUS).
 * @param old_id The old owner of the name (unused).
 * @param new_id The new owner of the name.
 *
 * If a new owner appears, it calls _update() to establish communication.
 * If the owner disappears (new_id is empty or NULL), it calls _release().
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

Eina_Bool
_elm_systray_watcher_init(void)
{
   if (_elm_systray_watcher) return EINA_TRUE; // Already initialized

   if (!elm_need_eldbus()) return EINA_FALSE; // Eldbus is not available

   _watcher_conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!_watcher_conn)
     {
        ERR("Could not get D-Bus session connection.");
        return EINA_FALSE;
     }

   // Watch for the StatusNotifierWatcher service appearing/disappearing
   eldbus_name_owner_changed_callback_add(_watcher_conn, BUS,
                                         _name_owner_changed_cb, NULL,
                                         EINA_TRUE); // EINA_TRUE for initial check

   // Attempt to set up the proxy immediately if the service is already running.
   // _name_owner_changed_cb will be called due to EINA_TRUE above,
   // which in turn calls _update() if the service exists.

   _elm_systray_watcher = EINA_TRUE;
   return EINA_TRUE;
}

void
_elm_systray_watcher_shutdown(void)
{
   if (!_elm_systray_watcher) return; // Not initialized or already shut down

   _elm_systray_watcher = EINA_FALSE;

   _release(); // Release D-Bus proxy and object

   // eldbus_name_owner_changed_callback_del can be added here if we stored the handler
   // but eldbus automatically cleans up handlers when the connection is unreffed.

   if (_watcher_conn) // Check if _watcher_conn was successfully acquired in init
     eldbus_connection_unref(_watcher_conn);
   _watcher_conn = NULL;
}
