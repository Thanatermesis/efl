/* Portions of this code have been derived from Weston
 *
 * Copyright © 2008-2012 Kristian Høgsberg
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2010 Red Hat <mjg@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ecore_drm_private.h"

/** @file
 * @brief Ecore D-Bus integration for DRM session and device management.
 *
 * This file handles communication with system services like logind via D-Bus
 * for managing DRM device access, session control, and device pause/resume events.
 */

static int _dbus_init_count = 0; /**< Reference counter for D-Bus initialization. */

static Eldbus_Connection *dconn; /**< Eldbus system connection. */
static Eldbus_Object *dobj; /**< Eldbus object for the logind session. */

/**
 * @brief Sends a "PauseDeviceComplete" D-Bus signal.
 *
 * This function is called after a device pause operation has been completed
 * locally, to notify logind.
 *
 * @param major The major number of the paused device.
 * @param minor The minor number of the paused device.
 */
static void
_ecore_drm_dbus_device_pause_done(uint32_t major, uint32_t minor)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return;
     }

   if (!(msg = eldbus_proxy_method_call_new(proxy, "PauseDeviceComplete")))
     {
        ERR("Could not create method call for proxy");
        return;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);
   eldbus_proxy_send(proxy, msg, NULL, NULL, -1);
}

/**
 * @brief Callback for the "SessionRemoved" D-Bus signal.
 *
 * This function is invoked when logind signals that a session has been removed.
 * If the removed session is the current one, it triggers a restoration of
 * logind state for the Ecore_Drm_Device.
 *
 * @param data The Ecore_Drm_Device associated with the session.
 * @param msg The Eldbus_Message containing signal data.
 */
static void
_cb_session_removed(void *data, const Eldbus_Message *msg)
{
   Ecore_Drm_Device *dev;
   const char *errname, *errmsg;
   const char *sid;

   if (!(dev = data)) return;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "s", &sid))
     {
        if (!strcmp(sid, dev->session))
          {
             WRN("\tCurrent Session Removed!!");
             _ecore_drm_logind_restore(dev);
          }
     }
}

/**
 * @brief Callback for the "PauseDevice" D-Bus signal from logind.
 *
 * This function is invoked when logind requests to pause a device.
 * It checks if the device is a DRM device and if the type is "pause".
 * If so, it calls _ecore_drm_dbus_device_pause_done to acknowledge
 * and sends an activation event indicating the device is inactive.
 *
 * @param ctxt Unused context data.
 * @param msg The Eldbus_Message containing signal data (major, minor, type).
 */
static void
_cb_device_paused(void *ctxt EINA_UNUSED, const Eldbus_Message *msg)
{
   const char *errname, *errmsg;
   const char *type;
   uint32_t maj, min;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "uus", &maj, &min, &type))
     {
        if (!strcmp(type, "pause"))
          _ecore_drm_dbus_device_pause_done(maj, min);

        if (maj == DRM_MAJOR)
          _ecore_drm_event_activate_send(EINA_FALSE);
     }
}

/**
 * @brief Callback for the "ResumeDevice" D-Bus signal from logind.
 *
 * This function is invoked when logind signals that a device has been resumed.
 * If the resumed device is a DRM device, it sends an activation event
 * indicating the device is active.
 *
 * @param ctxt Unused context data.
 * @param msg The Eldbus_Message containing signal data (major, minor, fd).
 */
static void
_cb_device_resumed(void *ctxt EINA_UNUSED, const Eldbus_Message *msg)
{
   const char *errname, *errmsg;
   uint32_t maj, min;
   int fd;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "uuh", &maj, &min, &fd))
     {
        if (maj == DRM_MAJOR)
          _ecore_drm_event_activate_send(EINA_TRUE);
     }
}

/**
 * @brief Callback for D-Bus property set operations.
 *
 * Logs an error if the property set operation failed.
 *
 * @param data Unused context data.
 * @param msg The Eldbus_Message containing the response.
 * @param pending Unused Eldbus_Pending object.
 */
static void
_property_response_set(void *data EINA_UNUSED, const Eldbus_Message *msg, Eldbus_Pending *pending EINA_UNUSED)
{
   const char *errname, *errmsg;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     ERR("Eldbus Message error %s - %s\n\n", errname, errmsg);
}

/**
 * @brief Callback for D-Bus "PropertiesChanged" events on the session proxy.
 *
 * This function is invoked when properties of the logind session object change.
 * Specifically, it monitors the "Active" property. If "Active" changes (implicitly
 * to true, as we are taking control), it sets the "Active" property to true and
 * "State" to "active" on the session object via D-Bus.
 *
 * @param data Unused context data.
 * @param proxy The Eldbus_Proxy on which the property changed.
 * @param event The Eldbus_Proxy_Event_Property_Changed event data.
 */
static void
_cb_properties_changed(void *data EINA_UNUSED, Eldbus_Proxy *proxy EINA_UNUSED, void *event)
{
   Eldbus_Proxy_Event_Property_Changed *ev;

   ev = event;

   if (!strcmp(ev->name, "Active"))
     {
         eldbus_proxy_property_set(proxy, "Active", "b", (void *)EINA_TRUE,
                                   _property_response_set, NULL);
         eldbus_proxy_property_set(proxy, "State", "s", &"active",
                                   _property_response_set, NULL);
     }
}

/**
 * @brief Takes control of the current logind session.
 *
 * This function sends a "TakeControl" D-Bus message to logind for the
 * current session. This is typically done to gain exclusive access to
 * DRM devices.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_ecore_drm_dbus_session_take(void)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg, *reply;
   const char *errname, *errmsg;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return EINA_FALSE;
     }

   /* send call to take control */
   if (!(msg = eldbus_proxy_method_call_new(proxy, "TakeControl")))
     {
        ERR("Could not create method call for proxy");
        return EINA_FALSE;
     }

   eldbus_message_arguments_append(msg, "b", EINA_FALSE);

   reply = eldbus_proxy_send_and_block(proxy, msg, -1);
   if (eldbus_message_error_get(reply, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        eldbus_message_unref(reply);
        return EINA_FALSE;
     }

   eldbus_message_unref(reply);
   return EINA_TRUE;
}

/**
 * @brief Releases control of the current logind session.
 *
 * This function sends a "ReleaseControl" D-Bus message to logind for the
 * current session. This is done when exclusive access to DRM devices
 * is no longer needed.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_ecore_drm_dbus_session_release(void)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg, *reply;
   const char *errname, *errmsg;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return EINA_FALSE;
     }

   /* send call to release control */
   if (!(msg = eldbus_proxy_method_call_new(proxy, "ReleaseControl")))
     {
        ERR("Could not create method call for proxy");
        return EINA_FALSE;
     }

   reply = eldbus_proxy_send_and_block(proxy, msg, -1);
   if (eldbus_message_error_get(reply, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        eldbus_message_unref(reply);
        return EINA_FALSE;
     }

   eldbus_message_unref(reply);
   return EINA_TRUE;
}

/**
 * @brief Releases a specific device through logind.
 *
 * Sends a "ReleaseDevice" D-Bus message to logind for the device
 * specified by its major and minor numbers.
 *
 * @param major The major number of the device to release.
 * @param minor The minor number of the device to release.
 */
void
_ecore_drm_dbus_device_release(uint32_t major, uint32_t minor)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return;
     }

   if (!(msg = eldbus_proxy_method_call_new(proxy, "ReleaseDevice")))
     {
        ERR("Could not create method call for proxy");
        return;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);

   eldbus_proxy_send(proxy, msg, NULL, NULL, -1);
}

/**
 * @brief Callback for the asynchronous "TakeDevice" D-Bus method call.
 *
 * This function is invoked when logind responds to a "TakeDevice" request.
 * It retrieves the file descriptor (fd) for the opened device and a boolean
 * indicating if the device was paused. It then calls the original callback
 * provided to _ecore_drm_dbus_device_take.
 *
 * @param data User data, typically the Ecore_Drm_Device.
 * @param msg The Eldbus_Message containing the reply (fd, paused_status).
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_cb_device_taken(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *errname, *errmsg;
   Ecore_Drm_Open_Cb callback = NULL;
   Eina_Bool b = EINA_FALSE;
   int fd = -1;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        goto eldbus_err;
     }

   /* DBUS_TYPE_UNIX_FD == 'h' */
   if (!eldbus_message_arguments_get(msg, "hb", &fd, &b))
     ERR("\tCould not get UNIX_FD from eldbus message: %d %d", fd, b);

eldbus_err:
   callback = (Ecore_Drm_Open_Cb)eldbus_pending_data_del(pending, "callback");
   if (callback) callback(data, fd, b);
}

/**
 * @brief Asynchronously takes a device through logind.
 *
 * Sends a "TakeDevice" D-Bus message to logind for the device specified by
 * its major and minor numbers. The result (file descriptor and paused state)
 * is delivered via the provided callback.
 *
 * @param major The major number of the device to take.
 * @param minor The minor number of the device to take.
 * @param callback The function to call upon completion.
 *                 Prototype: `void (*Ecore_Drm_Open_Cb)(void *data, int fd, Eina_Bool paused)`
 * @param data User data to pass to the callback.
 * @return 1 on success (request sent), -1 on failure.
 */
int
_ecore_drm_dbus_device_take(uint32_t major, uint32_t minor, Ecore_Drm_Open_Cb callback, void *data)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg;
   Eldbus_Pending *pending;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return -1;
     }

   if (!(msg = eldbus_proxy_method_call_new(proxy, "TakeDevice")))
     {
        ERR("Could not create method call for proxy");
        return -1;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);
   pending = eldbus_proxy_send(proxy, msg, _cb_device_taken, data, -1);
   if (callback) eldbus_pending_data_set(pending, "callback", callback);

   return 1;
}

/**
 * @brief Synchronously takes a device through logind.
 *
 * Sends a "TakeDevice" D-Bus message to logind and blocks until a reply
 * is received or a timeout occurs.
 *
 * @param major The major number of the device to take.
 * @param minor The minor number of the device to take.
 * @param[out] paused_out Pointer to store the paused state of the device.
 *                        Set to EINA_TRUE if the device is paused, EINA_FALSE otherwise.
 * @param timeout The timeout in seconds for the D-Bus call.
 * @return The file descriptor for the opened device on success, -1 on failure or timeout.
 */
int
_ecore_drm_dbus_device_take_no_pending(uint32_t major, uint32_t minor, Eina_Bool *paused_out, double timeout)
{
   Eldbus_Proxy *proxy;
   Eldbus_Message *msg, *reply;
   Eina_Bool b;
   const char *errname, *errmsg;
   int fd;

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus session proxy");
        return -1;
     }

   if (!(msg = eldbus_proxy_method_call_new(proxy, "TakeDevice")))
     {
        ERR("Could not create method call for proxy");
        return -1;
     }

   if (!eldbus_message_arguments_append(msg, "uu", major, minor))
     {
        eldbus_message_unref(msg);
        return -1;
     }

   reply = eldbus_proxy_send_and_block(proxy, msg, timeout);
   if (eldbus_message_error_get(reply, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        eldbus_message_unref(reply);
        return -1;
     }

   if (!eldbus_message_arguments_get(reply, "hb", &fd, &b))
     {
        eldbus_message_unref(reply);
        return -1;
     }

   eldbus_message_unref(reply);
   if (paused_out) *paused_out = b;
   return fd;
}

/**
 * @brief Initializes D-Bus communication for Ecore_Drm.
 *
 * Sets up the Eldbus connection to the system bus, gets the logind session
 * object, and registers signal handlers for session and device events.
 * This function uses a reference counter; D-Bus is fully initialized only
 * on the first call and shut down on the last corresponding call to
 * _ecore_drm_dbus_shutdown.
 *
 * @param dev The Ecore_Drm_Device containing session information.
 * @return The current D-Bus initialization count, or 0 on failure.
 */
int
_ecore_drm_dbus_init(Ecore_Drm_Device *dev)
{
   Eldbus_Proxy *proxy;
   int ret = 0;
   char *dpath;

   if (++_dbus_init_count != 1) return _dbus_init_count;

   if (!dev->session) return --_dbus_init_count;

   /* try to init eldbus */
   if (!eldbus_init())
     {
        ERR("Could not init eldbus library");
        return --_dbus_init_count;
     }

   /* try to get the dbus connection */
   if (!(dconn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM)))
     {
        ERR("Failed to get eldbus system connection");
        goto conn_err;
     }

   /* assemble dbus path */
   ret = asprintf(&dpath, "/org/freedesktop/login1/session/%s", dev->session);
   if (ret < 0)
     {
        ERR("Could not assemble dbus path");
        goto path_err;
     }

   /* try to get the eldbus object */
   if (!(dobj = eldbus_object_get(dconn, "org.freedesktop.login1", dpath)))
     {
        ERR("Could not get eldbus object: %s", dpath);
        goto obj_err;
     }

   /* try to get the Manager proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Manager")))
     {
        ERR("Could not get eldbus proxy");
        goto proxy_err;
     }

   eldbus_proxy_signal_handler_add(proxy, "SessionRemoved",
                                   _cb_session_removed, dev);

   /* try to get the Session proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.login1.Session")))
     {
        ERR("Could not get eldbus proxy");
        goto proxy_err;
     }

   eldbus_proxy_signal_handler_add(proxy, "PauseDevice",
                                   _cb_device_paused, NULL);
   eldbus_proxy_signal_handler_add(proxy, "ResumeDevice",
                                   _cb_device_resumed, NULL);

   /* try to get the Properties proxy */
   if (!(proxy = eldbus_proxy_get(dobj, "org.freedesktop.DBus.Properties")))
     {
        ERR("Could not get eldbus proxy");
        goto proxy_err;
     }

   eldbus_proxy_properties_monitor(proxy, EINA_TRUE);
   eldbus_proxy_event_callback_add(proxy, ELDBUS_PROXY_EVENT_PROPERTY_CHANGED,
                                   _cb_properties_changed, NULL);

   return _dbus_init_count;

proxy_err:
   eldbus_object_unref(dobj);
obj_err:
   free(dpath);
path_err:
   eldbus_connection_unref(dconn);
conn_err:
   eldbus_shutdown();
   return --_dbus_init_count;
}

/**
 * @brief Shuts down D-Bus communication for Ecore_Drm.
 *
 * Decrements the D-Bus initialization reference counter. If the counter
 * reaches zero, it unrefs D-Bus objects, closes the connection, and
 * shuts down Eldbus.
 *
 * @return The current D-Bus initialization count.
 */
int
_ecore_drm_dbus_shutdown(void)
{
   if (--_dbus_init_count != 0) return _dbus_init_count;

   /* release dbus object */
   if (dobj) eldbus_object_unref(dobj);

   /* release the dbus connection */
   if (dconn) eldbus_connection_unref(dconn);

   /* shutdown eldbus library */
   eldbus_shutdown();

   return _dbus_init_count;
}
