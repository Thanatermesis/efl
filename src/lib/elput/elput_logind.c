/**
 * @file
 * @brief Functions for interacting with systemd-logind or elogind.
 *
 * This file contains the implementation for managing input devices
 * and sessions through the logind D-Bus interface. It handles
 * operations like opening/closing devices, session activation,
 * and VT switching.
 */

#include "elput_private.h"

#ifdef HAVE_SYSTEMD

# ifdef major
#  define MAJOR(x) major(x)
# else
#  define MAJOR(x) ((((x) >> 8) & 0xfff) | (((x) >> 32) & ~0xfff))
# endif

# ifdef minor
#  define MINOR(x) minor(x)
# else
#  define MINOR(x) (((x) & 0xff) | (((x) >> 12) & ~0xff))
# endif

static Eina_Module *_libsystemd = NULL; /**< Pointer to the loaded libsystemd/libelogind module. */
static Eina_Bool _libsystemd_broken = EINA_FALSE; /**< Flag indicating if loading libsystemd/libelogind failed permanently. */

static int (*_elput_sd_session_get_vt) (const char *session, unsigned *vtnr) = NULL; /**< Function pointer for sd_session_get_vt. */
static int (*_elput_sd_session_get_tty) (const char *session, char **display) = NULL; /**< Function pointer for sd_session_get_tty. */
static int (*_elput_sd_pid_get_session) (pid_t pid, char **session) = NULL; /**< Function pointer for sd_pid_get_session. */
static int (*_elput_sd_session_get_seat) (const char *session, char **seat) = NULL; /**< Function pointer for sd_session_get_seat. */
static int (*_elput_sd_seat_can_tty) (const char *seat) = NULL; /**< Function pointer for sd_seat_can_tty. */

/**
 * @brief Initializes libsystemd/libelogind function pointers.
 *
 * This function attempts to load libsystemd.so.0 or libelogind-shared.so.0
 * (or libelogind.so.0) and resolve necessary symbols. It tries multiple
 * library names to support both systemd and elogind.
 * If EFL_ELOGIND_LIB environment variable is set, it will try that path first.
 * Sets _libsystemd_broken to EINA_TRUE if all attempts fail.
 */
void
_elput_sd_init(void)
{
   if (_libsystemd_broken) return;
   _libsystemd = eina_module_new("libsystemd.so.0");
   if (_libsystemd)
     {
        if (!eina_module_load(_libsystemd))
          {
             eina_module_free(_libsystemd);
             _libsystemd = NULL;
          }
     }
   if (!_libsystemd)
     {
        const char *s = getenv("EFL_ELOGIND_LIB");

        if (s)
          {
             _libsystemd = eina_module_new(s);
             if (_libsystemd)
               {
                  if (!eina_module_load(_libsystemd))
                    {
                       eina_module_free(_libsystemd);
                       _libsystemd = NULL;
                    }
               }
          }
        if (!_libsystemd)
          {
             _libsystemd = eina_module_new("libelogind-shared.so.0");
             if (_libsystemd)
               {
                  if (!eina_module_load(_libsystemd))
                    {
                       eina_module_free(_libsystemd);
                       _libsystemd = NULL;
                    }
               }
          }
        if (!_libsystemd)
          {
             _libsystemd = eina_module_new("libelogind.so.0");
             if (_libsystemd)
               {
                  if (!eina_module_load(_libsystemd))
                    {
                       eina_module_free(_libsystemd);
                       _libsystemd = NULL;
                    }
               }
          }
     }
   if (!_libsystemd)
     {
        _libsystemd_broken = EINA_TRUE;
        return;
     }
   // sd_session_get_vt == newer in systemd 207
   _elput_sd_session_get_vt =
     eina_module_symbol_get(_libsystemd, "sd_session_get_vt");
   // sd_session_get_tty == older api in ssystemd 198
   _elput_sd_session_get_tty =
     eina_module_symbol_get(_libsystemd, "sd_session_get_tty");
   _elput_sd_pid_get_session =
     eina_module_symbol_get(_libsystemd, "sd_pid_get_session");
   _elput_sd_session_get_seat =
     eina_module_symbol_get(_libsystemd, "sd_session_get_seat");
   _elput_sd_seat_can_tty =
     eina_module_symbol_get(_libsystemd, "sd_seat_can_tty");
   if (((!_elput_sd_session_get_vt) && (!_elput_sd_session_get_tty)) ||
       (!_elput_sd_pid_get_session) ||
       (!_elput_sd_session_get_seat) || (!_elput_sd_seat_can_tty))
     {
        _elput_sd_session_get_vt = NULL;
        _elput_sd_session_get_tty = NULL;
        _elput_sd_pid_get_session = NULL;
        _elput_sd_session_get_seat = NULL;
       _elput_sd_seat_can_tty = NULL;
        eina_module_free(_libsystemd);
        _libsystemd = NULL;
        _libsystemd_broken = EINA_TRUE;
     }
}

/**
 * @brief Frees an Elput_Event_Session_Active event.
 *
 * Callback for ecore_event_add to free the event data.
 *
 * @param data Unused.
 * @param event The event data to free.
 */
static void
_logind_session_active_cb_free(void *data EINA_UNUSED, void *event)
{
   Elput_Event_Session_Active *ev;

   ev = event;
   eina_stringshare_del(ev->session);
   free(ev);
}

/**
 * @brief Sends an ELPUT_EVENT_SESSION_ACTIVE event.
 *
 * This function enables or disables input based on the active state
 * and then creates and sends an ELPUT_EVENT_SESSION_ACTIVE event.
 *
 * @param em The Elput_Manager instance.
 * @param active EINA_TRUE if the session is now active, EINA_FALSE otherwise.
 */
static void
_logind_session_active_send(Elput_Manager *em, Eina_Bool active)
{
   Elput_Event_Session_Active *ev;

   if (active)
     _elput_input_enable(em);
   else
     _elput_input_disable(em);

   ev = calloc(1, sizeof(Elput_Event_Session_Active));
   if (!ev) return;

   ev->session = eina_stringshare_add(em->sid);
   ev->active = active;

   ecore_event_add(ELPUT_EVENT_SESSION_ACTIVE, ev,
                   _logind_session_active_cb_free, NULL);
}

/**
 * @brief Sends a PauseDeviceComplete D-Bus signal.
 *
 * This is called after a device has been paused to notify logind.
 *
 * @param em The Elput_Manager instance.
 * @param major The major number of the paused device.
 * @param minor The minor number of the paused device.
 */
static void
_logind_device_pause_complete(Elput_Manager *em, uint32_t major, uint32_t minor)
{
   Eldbus_Message *msg;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "PauseDeviceComplete");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);

   eldbus_proxy_send(em->dbus.session, msg, NULL, NULL, -1);
}

/**
 * @brief Callback for the SessionRemoved D-Bus signal.
 *
 * Handles the SessionRemoved signal from logind. If the removed session
 * is the current one, it logs a warning.
 *
 * @param data The Elput_Manager instance.
 * @param msg The D-Bus message.
 */
static void
_cb_session_removed(void *data, const Eldbus_Message *msg)
{
   Elput_Manager *em;
   const char *errname, *errmsg;
   const char *sid;

   em = data;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "s", &sid))
     {
        if (!strcmp(sid, em->sid))
          {
             WRN("Logind session removed");
             /* TODO: call manager restore function */
          }
     }
}

/**
 * @brief Callback for the PauseDevice D-Bus signal.
 *
 * Handles the PauseDevice signal from logind. If the device is a DRM device
 * (major 226), it sends a session active event with active=EINA_FALSE.
 * It also calls _logind_device_pause_complete if the type is "pause".
 * Includes a workaround for multiple DRM device opens/closes during probing.
 *
 * @param data The Elput_Manager instance.
 * @param msg The D-Bus message.
 */
static void
_cb_device_paused(void *data, const Eldbus_Message *msg)
{
   Elput_Manager *em;
   const char *errname, *errmsg;
   const char *type;
   uint32_t maj, min;

   em = data;
   if (!em->drm_opens) return;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "uus", &maj, &min, &type))
     {
        /* If we opened a device during probing then we're still going
         * to get a "gone" callback when we release it, so we'd better
         * eat that instead of treating it like losing the drm device
         * we currently have open, and crapping up libinput's internals
         * for a nice deferred explosion at shutdown...
         *
         * FIXME: do this better?
         */
        if ((em->drm_opens > 1) && (maj == 226) && !strcmp(type, "gone"))
          {
             em->drm_opens--;
             return;
          }

        if (!strcmp(type, "pause"))
          _logind_device_pause_complete(em, maj, min);

        if (maj == 226) // DRM_MAJOR
          _logind_session_active_send(em, EINA_FALSE);
     }
}

/**
 * @brief Callback for the ResumeDevice D-Bus signal.
 *
 * Handles the ResumeDevice signal from logind. If the device is a DRM device
 * (major 226), it sends a session active event with active=EINA_TRUE.
 *
 * @param data The Elput_Manager instance.
 * @param msg The D-Bus message.
 */
static void
_cb_device_resumed(void *data, const Eldbus_Message *msg)
{
   Elput_Manager *em;
   const char *errname, *errmsg;
   uint32_t maj;

   em = data;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        return;
     }

   if (eldbus_message_arguments_get(msg, "u", &maj))
     {
        if (maj == 226) // DRM_MAJOR
          _logind_session_active_send(em, EINA_TRUE);
     }
}

/**
 * @brief Gets the VT number for a given session ID.
 *
 * Uses either sd_session_get_vt (newer systemd) or sd_session_get_tty
 * (older systemd/elogind) to retrieve the TTY and parse the VT number.
 *
 * @param sid The session ID string.
 * @param[out] vt Pointer to store the retrieved VT number.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_session_vt_get(const char *sid, unsigned int *vt)
{
   int ret = 0;
   char *tty;

   _elput_sd_init();
   if ((!_elput_sd_session_get_vt) && (!_elput_sd_session_get_tty))
     return EINA_FALSE;
   if (_elput_sd_session_get_vt)
     return (_elput_sd_session_get_vt(sid, vt) >= 0);

   ret = _elput_sd_session_get_tty(sid, &tty);
   if (ret < 0) return ret;

   ret = sscanf(tty, "tty%u", vt);
   free(tty);

   if (ret != 1) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Opens a connection to the system D-Bus.
 *
 * Initializes Eldbus if not already initialized and gets a connection
 * to the system bus.
 *
 * @param[out] conn Pointer to store the Eldbus_Connection.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_dbus_open(Eldbus_Connection **conn)
{
   if (!eldbus_init()) return EINA_FALSE;

   *conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SYSTEM);
   if (!*conn) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Closes the D-Bus connection.
 *
 * Unreferences the Eldbus_Connection and shuts down Eldbus.
 *
 * @param conn The Eldbus_Connection to close.
 */
static void
_logind_dbus_close(Eldbus_Connection *conn)
{
   if (conn) eldbus_connection_unref(conn);
   eldbus_shutdown();
}

/**
 * @brief Gets the D-Bus object path for the current session.
 *
 * Calls the GetSession method on org.freedesktop.login1.Manager
 * to retrieve the object path for the session specified by em->sid.
 * The path is stored in em->dbus.path.
 *
 * @param em The Elput_Manager instance.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_session_object_path_get(Elput_Manager *em)
{
   Eldbus_Message *msg, *reply;
   Eldbus_Object *obj;
   Eldbus_Proxy *proxy;
   const char *errname, *errmsg;
   char *str;
   Eina_Bool ret = EINA_FALSE;

   obj =
     eldbus_object_get(em->dbus.conn, "org.freedesktop.login1",
                       "/org/freedesktop/login1");
   if (!obj) return EINA_FALSE;

   proxy = eldbus_proxy_get(obj, "org.freedesktop.login1.Manager");
   if (!proxy) goto proxy_fail;

   msg = eldbus_proxy_method_call_new(proxy, "GetSession");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        goto message_fail;
     }

   eldbus_message_arguments_append(msg, "s", em->sid);

   reply = eldbus_proxy_send_and_block(proxy, msg, -1);
   if (eldbus_message_error_get(reply, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        eldbus_message_unref(reply);
        goto message_fail;
     }
   if (!eldbus_message_arguments_get(reply, "o", &str))
     {
        eldbus_message_unref(reply);
        goto message_fail;
     }

   em->dbus.path = strdup(str);
   eldbus_message_unref(reply);
   ret = EINA_TRUE;

message_fail:
   eldbus_proxy_unref(proxy);
proxy_fail:
   eldbus_object_unref(obj);
   return ret;
}

/**
 * @brief Sets up D-Bus proxies and signal handlers for the session.
 *
 * After obtaining the session object path, this function gets D-Bus objects
 * and proxies for org.freedesktop.login1.Manager and
 * org.freedesktop.login1.Session. It then registers signal handlers for
 * SessionRemoved, PauseDevice, and ResumeDevice.
 *
 * @param em The Elput_Manager instance.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_dbus_setup(Elput_Manager *em)
{
   Eldbus_Proxy *proxy;

   if (!_logind_session_object_path_get(em)) return EINA_FALSE;

   em->dbus.obj =
     eldbus_object_get(em->dbus.conn, "org.freedesktop.login1",
                       em->dbus.path);
   if (!em->dbus.obj)
     {
        ERR("Could not get dbus object");
        goto obj_err;
     }

   proxy =
     eldbus_proxy_get(em->dbus.obj, "org.freedesktop.login1.Manager");
   if (!proxy)
     {
        ERR("Could not get dbus proxy");
        goto proxy_err;
     }
   em->dbus.manager = proxy;

   eldbus_proxy_signal_handler_add(proxy, "SessionRemoved",
                                   _cb_session_removed, em);

   proxy =
     eldbus_proxy_get(em->dbus.obj, "org.freedesktop.login1.Session");
   if (!proxy)
     {
        ERR("Could not get dbus proxy");
        goto proxy_err;
     }
   em->dbus.session = proxy;

   eldbus_proxy_signal_handler_add(proxy, "PauseDevice",
                                   _cb_device_paused, em);
   eldbus_proxy_signal_handler_add(proxy, "ResumeDevice",
                                   _cb_device_resumed, em);
   return EINA_TRUE;

proxy_err:
   eldbus_object_unref(em->dbus.obj);
obj_err:
   free(em->dbus.path);
   return EINA_FALSE;
}

/**
 * @brief Takes control of the logind session.
 *
 * Calls the TakeControl method on the session D-Bus object.
 *
 * @param em The Elput_Manager instance.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_control_take(Elput_Manager *em)
{
   Eldbus_Message *msg, *reply;
   const char *errname, *errmsg;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "TakeControl");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return EINA_FALSE;
     }

   eldbus_message_arguments_append(msg, "b", EINA_FALSE);

   reply = eldbus_proxy_send_and_block(em->dbus.session, msg, -1);
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
 * @brief Releases control of the logind session.
 *
 * Calls the ReleaseControl method on the session D-Bus object.
 *
 * @param em The Elput_Manager instance.
 */
static void
_logind_control_release(Elput_Manager *em)
{
   Eldbus_Message *msg;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "ReleaseControl");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return;
     }

   eldbus_proxy_send(em->dbus.session, msg, NULL, NULL, -1);
}

/**
 * @brief Releases a device previously taken via logind.
 *
 * Calls the ReleaseDevice method on the session D-Bus object.
 *
 * @param em The Elput_Manager instance.
 * @param major The major number of the device.
 * @param minor The minor number of the device.
 */
static void
_logind_device_release(Elput_Manager *em, uint32_t major, uint32_t minor)
{
   Eldbus_Message *msg;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "ReleaseDevice");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);

   eldbus_proxy_send(em->dbus.session, msg, NULL, NULL, -1);
}

/**
 * @brief Writes a file descriptor to the input pipe.
 *
 * This is used to pass the opened device FD from the D-Bus callback thread
 * (or synchronous call) back to the main thread waiting on the pipe.
 * Closes the write end of the pipe after writing.
 *
 * @param em The Elput_Manager instance.
 * @param fd The file descriptor to write (or -1 on error).
 */
static void
_logind_pipe_write_fd(Elput_Manager *em, int fd)
{
   int ret;

   while (1)
     {
        ret = write(em->input.pipe, &fd, sizeof(int));
        if (ret < 0)
          {
             if ((errno == EAGAIN) || (errno == EINTR))
               continue;
             WRN("Failed to write to input pipe");
          }
        break;
     }
   close(em->input.pipe);
   em->input.pipe = -1;
}

/**
 * @brief Callback for asynchronous TakeDevice D-Bus method call.
 *
 * Handles the reply from the TakeDevice method. Extracts the file descriptor,
 * sets its flags (e.g., O_NONBLOCK), and writes it to the pipe using
 * _logind_pipe_write_fd. If an error occurs or fd is invalid, -1 is written.
 * Releases the device via logind if an error occurs after obtaining the fd.
 *
 * @param data The Elput_Manager instance.
 * @param msg The D-Bus reply message.
 * @param pending The Eldbus_Pending object for this call.
 */
static void
_logind_device_take_cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   Eina_Bool p = EINA_FALSE;
   const char *errname, *errmsg;
   int ret, fd = -1;
   int fl, flags;
   Elput_Manager *em = data;

   if (em->input.current_pending == pending)
     em->input.current_pending = NULL;

   if (eldbus_message_error_get(msg, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        goto err;
     }

   if (!eldbus_message_arguments_get(msg, "hb", &fd, &p))
     ERR("Could not get UNIX_FD from dbus message");

   if (fd < 0) goto err;

   fl = fcntl(fd, F_GETFL);
   if (fl < 0) goto err;

   flags = (intptr_t)eldbus_pending_data_get(pending, "flags");

   if (flags & O_NONBLOCK)
     fl |= O_NONBLOCK;

   ret = fcntl(fd, F_SETFL, fl);
   if (ret < 0) goto err;

   _logind_pipe_write_fd(em, fd);
   return;

err:
   if (fd >= 0)
     {
        uintptr_t majo, mino;

        close(fd);
        majo = (uintptr_t)eldbus_pending_data_get(pending, "major");
        mino = (uintptr_t)eldbus_pending_data_get(pending, "minor");
        _logind_device_release(em, majo, mino);
     }
   fd = -1;
   _logind_pipe_write_fd(em, fd);
}

/**
 * @brief Asynchronously takes a device via logind.
 *
 * Calls the TakeDevice method on the session D-Bus object asynchronously.
 * The result (file descriptor or error) is handled by _logind_device_take_cb.
 * Stores major, minor, and flags in the Eldbus_Pending object to be
 * retrieved in the callback.
 *
 * @param em The Elput_Manager instance.
 * @param flags Flags for opening the device (e.g., O_NONBLOCK).
 * @param major The major number of the device.
 * @param minor The minor number of the device.
 */
static void
_logind_device_take_async(Elput_Manager *em, int flags, uint32_t major, uint32_t minor)
{
   Eldbus_Message *msg;
   intptr_t fd = -1;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "TakeDevice");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        _logind_pipe_write_fd(em, fd);
        return;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);

   em->input.current_pending =
     eldbus_proxy_send(em->dbus.session, msg, _logind_device_take_cb, em, -1);
   if (!em->input.current_pending) CRIT("FAIL!");
   eldbus_pending_data_set(em->input.current_pending, "major",
                           (uintptr_t*)(uintptr_t)major);
   eldbus_pending_data_set(em->input.current_pending, "minor",
                           (uintptr_t*)(uintptr_t)minor);
   eldbus_pending_data_set(em->input.current_pending, "flags",
                           (intptr_t*)(intptr_t)flags);
}

/**
 * @brief Synchronously takes a device via logind.
 *
 * Calls the TakeDevice method on the session D-Bus object and blocks
 * until a reply is received.
 *
 * @param em The Elput_Manager instance.
 * @param major The major number of the device.
 * @param minor The minor number of the device.
 * @return The opened file descriptor on success, or -1 on error.
 */
static int
_logind_device_take(Elput_Manager *em, uint32_t major, uint32_t minor)
{
   Eldbus_Message *msg, *reply;
   Eina_Bool p = EINA_FALSE;
   const char *errname, *errmsg;
   int fd = -1;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "TakeDevice");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return -1;
     }

   eldbus_message_arguments_append(msg, "uu", major, minor);

   reply = eldbus_proxy_send_and_block(em->dbus.session, msg, -1);
   if (eldbus_message_error_get(reply, &errname, &errmsg))
     {
        ERR("Eldbus Message Error: %s %s", errname, errmsg);
        eldbus_message_unref(reply);
        return -1;
     }

   if (!eldbus_message_arguments_get(reply, "hb", &fd, &p))
     ERR("Could not get UNIX_FD from dbus message");

   eldbus_message_unref(reply);
   return fd;
}

/**
 * @brief Activates the logind session.
 *
 * Calls the Activate method on the session D-Bus object. This is typically
 * done to bring the session to the foreground.
 *
 * @param em The Elput_Manager instance.
 * @return EINA_TRUE on success or if message sending is initiated, EINA_FALSE on error creating message.
 */
static Eina_Bool
_logind_activate(Elput_Manager *em)
{
   Eldbus_Message *msg;

   msg = eldbus_proxy_method_call_new(em->dbus.session, "Activate");
   if (!msg)
     {
        ERR("Could not create method call for proxy");
        return EINA_FALSE;
     }

   eldbus_proxy_send(em->dbus.session, msg, NULL, NULL, -1);
   return EINA_TRUE;
}

/**
 * @brief Connects to logind and sets up an Elput_Manager.
 *
 * This is the main entry point for establishing a logind-managed input session.
 * It performs several steps:
 * 1. Initializes libsystemd/libelogind symbols.
 * 2. Gets the current process's session ID and seat.
 * 3. Validates the provided seat and TTY against the session's values.
 * 4. Opens a D-Bus connection.
 * 5. Sets up D-Bus proxies and signal handlers for the session.
 * 6. Takes control of the session.
 * 7. Activates the session.
 *
 * @param[out] manager Pointer to store the created Elput_Manager instance.
 * @param seat The requested seat ID (e.g., "seat0"). Can be NULL.
 * @param tty The requested TTY number. If 0, it's not checked against session TTY.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_logind_connect(Elput_Manager **manager, const char *seat, unsigned int tty)
{
   Elput_Manager *em;
   int ret = 0;
   char *s = NULL;

   _elput_sd_init();
   if (!_elput_sd_pid_get_session) return EINA_FALSE;

   em = calloc(1, sizeof(Elput_Manager));
   if (!em) return EINA_FALSE;

   em->interface = &_logind_interface;
   em->seat = eina_stringshare_add(seat);

   ret = _elput_sd_pid_get_session(getpid(), &em->sid);

   if (ret < 0)
     {
        if (ret == -ENODATA || ret == -ENXIO)
          ERR("Could not get systemd session, the pid is outside a session, check that you are running inside a logind-session.");
        else
          ERR("Could not get systemd session");
        goto session_err;
     }

   if (!em->sid) goto session_err;

   ret = _elput_sd_session_get_seat(em->sid, &s);
   if (ret < 0)
     {
        ERR("Failed to get session seat");
        free(s);
        goto seat_err;
     }
   else if ((seat) && (s) && (strcmp(seat, s)))
     {
        ERR("Seat '%s' differs from session seat '%s'", seat, s);
        free(s);
        goto seat_err;
     }

   if ((seat) && (_elput_sd_seat_can_tty(seat)))
     {
        if (!_logind_session_vt_get(em->sid, &em->vt_num))
          {
             ERR("Could not get session vt");
             goto vt_err;
          }
        else if ((tty > 0) && (em->vt_num != tty))
          {
             ERR("Requested VT %u differs from session VT %u", tty, em->vt_num);
             goto vt_err;
          }
     }

   free(s);

   if (!_logind_dbus_open(&em->dbus.conn))
     {
        ERR("Could not connect to dbus");
        goto vt_err;
     }

   if (!_logind_dbus_setup(em))
     {
        ERR("Could not setup dbus");
        goto dbus_err;
     }

   if (!_logind_control_take(em))
     {
        ERR("Could not take control of session");
        goto ctrl_err;
     }

   if (!_logind_activate(em))
     {
        ERR("Could not activate session");
        goto actv_err;
     }

   *(Elput_Manager **)manager = em;

   return EINA_TRUE;

actv_err:
   _logind_control_release(em);
ctrl_err:
   eldbus_object_unref(em->dbus.obj);
   free(em->dbus.path);
dbus_err:
   _logind_dbus_close(em->dbus.conn);
vt_err:
seat_err:
   free(em->sid);
session_err:
   free(em);
   return EINA_FALSE;
}

/**
 * @brief Disconnects from logind and cleans up resources.
 *
 * Releases control of the session, unrefs D-Bus objects and proxies,
 * closes the D-Bus connection, and frees the Elput_Manager instance
 * and associated data (seat, session ID, XKB context/keymap).
 *
 * @param em The Elput_Manager instance to disconnect and free.
 */
static void
_logind_disconnect(Elput_Manager *em)
{
   _logind_control_release(em);
   eldbus_proxy_unref(em->dbus.manager);
   eldbus_proxy_unref(em->dbus.session);
   eldbus_object_unref(em->dbus.obj);
   free(em->dbus.path);
   _logind_dbus_close(em->dbus.conn);
   eina_stringshare_del(em->seat);
   free(em->sid);
   if (em->cached.context) xkb_context_unref(em->cached.context);
   if (em->cached.keymap) xkb_keymap_unref(em->cached.keymap);
   free(em);
}

/**
 * @brief Asynchronously opens a device file via logind.
 *
 * Stats the given path to get its device major/minor numbers.
 * If it's a character device, calls _logind_device_take_async.
 * Otherwise, writes -1 to the pipe to indicate failure.
 * This function is part of the Elput_Interface and is called when elput
 * needs to open a device in a non-blocking way. The actual fd is passed
 * back via a pipe.
 *
 * @param em The Elput_Manager instance.
 * @param path The path to the device file (e.g., "/dev/input/event0").
 * @param flags Flags for opening the device (e.g., O_RDONLY | O_NONBLOCK).
 */
static void
_logind_open_async(Elput_Manager *em, const char *path, int flags)
{
   struct stat st;
   intptr_t fd = -1;

   if ((stat(path, &st) < 0) || (!S_ISCHR(st.st_mode)))
     _logind_pipe_write_fd(em, fd);
   else
     _logind_device_take_async(em, flags, MAJOR(st.st_rdev), MINOR(st.st_rdev));
}

/**
 * @brief Synchronously opens a device file via logind.
 *
 * Stats the given path to get its device major/minor numbers.
 * If it's a character device, calls _logind_device_take to get an fd.
 * Sets O_NONBLOCK if requested in flags. Increments drm_opens if it's a DRM device.
 * This function is part of the Elput_Interface.
 *
 * @param em The Elput_Manager instance.
 * @param path The path to the device file (e.g., "/dev/input/event0").
 * @param flags Flags for opening the device (e.g., O_RDONLY | O_NONBLOCK).
 * @return The opened file descriptor on success, or -1 on error.
 */
static int
_logind_open(Elput_Manager *em, const char *path, int flags)
{
   struct stat st;
   int ret, fd = -1;
   int fl;

   ret = stat(path, &st);
   if (ret < 0) return -1;

   if (!S_ISCHR(st.st_mode)) return -1;

   fd = _logind_device_take(em, MAJOR(st.st_rdev), MINOR(st.st_rdev));
   if (fd < 0) return fd;

   if (MAJOR(st.st_rdev) == 226) //DRM_MAJOR
     em->drm_opens++;

   fl = fcntl(fd, F_GETFL);
   if (fl < 0) goto err;

   if (flags & O_NONBLOCK)
     fl |= O_NONBLOCK;

   ret = fcntl(fd, F_SETFL, fl);
   if (ret < 0) goto err;

   return fd;

err:
   close(fd);
   _logind_device_release(em, MAJOR(st.st_rdev), MINOR(st.st_rdev));
   return -1;
}

/**
 * @brief Closes a device file previously opened via logind.
 *
 * Closes the file descriptor and then calls _logind_device_release
 * to inform logind that the device is no longer in use.
 * This function is part of the Elput_Interface.
 *
 * @param em The Elput_Manager instance.
 * @param fd The file descriptor to close.
 */
static void
_logind_close(Elput_Manager *em, int fd)
{
   struct stat st;
   int ret;

   ret = fstat(fd, &st);
   close(fd);
   if (ret < 0) return;

   if (!S_ISCHR(st.st_mode)) return;

   _logind_device_release(em, MAJOR(st.st_rdev), MINOR(st.st_rdev));
}

/**
 * @brief Switches to a different virtual terminal (VT).
 *
 * Calls the SwitchTo method on org.freedesktop.login1.Seat D-Bus interface
 * for the "self" seat (usually the current seat).
 * This function is part of the Elput_Interface.
 *
 * @param em The Elput_Manager instance.
 * @param vt The VT number to switch to.
 * @return EINA_TRUE if the D-Bus message was successfully created and sent (or queued),
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_logind_vt_set(Elput_Manager *em, int vt)
{
   Eldbus_Message *msg;

   msg =
     eldbus_message_method_call_new("org.freedesktop.login1",
                                    "/org/freedesktop/login1/seat/self",
                                    "org.freedesktop.login1.Seat", "SwitchTo");
   if (!msg) return EINA_FALSE;

   if (!eldbus_message_arguments_append(msg, "u", vt))
     {
        eldbus_message_unref(msg);
        return EINA_FALSE;
     }

   eldbus_connection_send(em->dbus.conn, msg, NULL, NULL, -1);

   return EINA_TRUE;
}

/**
 * @brief The Elput_Interface implementation for logind.
 *
 * This structure provides the function pointers that elput core uses
 * to interact with the session management system (logind in this case).
 */
Elput_Interface _logind_interface =
{
   _logind_connect,
   _logind_disconnect,
   _logind_open,
   _logind_open_async,
   _logind_close,
   _logind_vt_set,
};

#endif
