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
#include <ctype.h>

#ifndef KDSKBMUTE
# define KDSKBMUTE 0x4B51
#endif

/**
 * @internal
 * @brief Global event handler for ECORE_DRM_EVENT_ACTIVATE events.
 *
 * This handler is responsible for processing activation/deactivation events,
 * typically by calling _ecore_drm_logind_cb_activate.
 */
static Ecore_Event_Handler *active_hdlr;

#ifdef HAVE_SYSTEMD
static Eina_Module *_libsystemd = NULL;
static Eina_Bool _libsystemd_broken = EINA_FALSE;

static int (*_ecore_sd_session_get_vt) (const char *session, unsigned *vtnr) = NULL;
static int (*_ecore_sd_pid_get_session) (pid_t pid, char **session) = NULL;
static int (*_ecore_sd_session_get_seat) (const char *session, char **seat) = NULL;

/**
 * @internal
 * @brief Initializes the connection to libsystemd and resolves symbols.
 *
 * This function attempts to load libsystemd.so.0 and get pointers to
 * necessary functions like sd_session_get_vt, sd_pid_get_session, and
 * sd_session_get_seat. If loading or symbol resolution fails, it marks
 * libsystemd support as broken.
 */
void
_ecore_drm_sd_init(void)
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
        _libsystemd_broken = EINA_TRUE;
        return;
     }
   _ecore_sd_session_get_vt =
     eina_module_symbol_get(_libsystemd, "sd_session_get_vt");
   _ecore_sd_pid_get_session =
     eina_module_symbol_get(_libsystemd, "sd_pid_get_session");
   _ecore_sd_session_get_seat =
     eina_module_symbol_get(_libsystemd, "sd_session_get_seat");
   if ((!_ecore_sd_session_get_vt) ||
       (!_ecore_sd_pid_get_session) ||
       (!_ecore_sd_session_get_seat))
     {
        _ecore_sd_session_get_vt = NULL;
        _ecore_sd_pid_get_session = NULL;
        _ecore_sd_session_get_seat = NULL;
        eina_module_free(_libsystemd);
        _libsystemd = NULL;
        _libsystemd_broken = EINA_TRUE;
     }
}

/**
 * @internal
 * @brief Retrieves the virtual terminal (VT) number for the current session using systemd.
 *
 * This function uses the systemd function sd_session_get_vt (obtained via
 * _ecore_drm_sd_init) to determine the VT number associated with the
 * Ecore_Drm_Device's session.
 *
 * @param dev The Ecore_Drm_Device structure. Its 'session' field is used to
 *            query systemd, and its 'vt' field is updated with the result.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., systemd functions
 *         not available or sd_session_get_vt fails).
 */
static inline Eina_Bool
_ecore_drm_logind_vt_get(Ecore_Drm_Device *dev)
{
   int ret;

   _ecore_drm_sd_init();
   if (!_ecore_sd_session_get_vt)
     {
        ERR("Could not get systemd tty");
        return EINA_FALSE;
     }
   ret = _ecore_sd_session_get_vt(dev->session, &dev->vt);
   if (ret < 0)
     {
        ERR("Could not get systemd tty");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}
#endif

/**
 * @internal
 * @brief Sets up the virtual terminal (VT) for the Ecore_Drm_Device.
 *
 * This function constructs the TTY device path (e.g., /dev/ttyX) based on the
 * VT number stored in @p dev and then attempts to open it using
 * ecore_drm_tty_open().
 *
 * @param dev The Ecore_Drm_Device structure. Its 'vt' field is used to
 *            determine the TTY to open.
 * @return EINA_TRUE if the TTY was successfully opened, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_drm_logind_vt_setup(Ecore_Drm_Device *dev)
{
   char buff[64];

   snprintf(buff, sizeof(buff), "/dev/tty%d", dev->vt);
   buff[sizeof(buff) - 1] = 0;

   if (!ecore_drm_tty_open(dev, buff))
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function for handling VT switch signals (SIGUSR1, SIGUSR2).
 *
 * This function is registered as an Ecore_Event_Handler for ECORE_EVENT_SIGNAL_USER.
 * It processes signals that indicate a VT switch is requested or acknowledged.
 * - SIGUSR1 (ev->number == 1): Indicates a request to release the VT.
 *   It sends a deactivate event and calls ioctl VT_RELDISP to release display.
 * - SIGUSR2 (ev->number == 2): Indicates the VT has been acquired.
 *   It calls ioctl VT_RELDISP with VT_ACKACQ to acknowledge and sends an activate event.
 *
 * @param data Pointer to the Ecore_Drm_Device.
 * @param type The type of the event (unused).
 * @param event Pointer to the Ecore_Event_Signal_User event data.
 * @return ECORE_CALLBACK_RENEW to keep the handler registered.
 */
static Eina_Bool
_ecore_drm_logind_cb_vt_signal(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Drm_Device *dev;
   Ecore_Event_Signal_User *ev;
   siginfo_t sig;

   ev = event;
   sig = ev->data;

   if (sig.si_code != SI_KERNEL) return ECORE_CALLBACK_RENEW;
   if (!(dev = data)) return ECORE_CALLBACK_RENEW;

   switch (ev->number)
     {
      case 1:
        _ecore_drm_event_activate_send(EINA_FALSE);
        ioctl(dev->tty.fd, VT_RELDISP, 1);
        break;
      case 2:
        ioctl(dev->tty.fd, VT_RELDISP, VT_ACKACQ);
        _ecore_drm_event_activate_send(EINA_TRUE);
        break;
      default:
        break;
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Callback function for handling ECORE_DRM_EVENT_ACTIVATE events.
 *
 * This function is triggered when the DRM device's session becomes active or
 * inactive (e.g., due to a VT switch).
 * If activating:
 *  - Sets the mode for all outputs.
 *  - Enables all inputs.
 * If deactivating:
 *  - Disables all inputs.
 *  - Disables hardware cursors on all outputs.
 *  - Disables all sprites.
 *
 * @param data Pointer to the Ecore_Drm_Device.
 * @param type The type of the event (unused).
 * @param event Pointer to the Ecore_Drm_Event_Activate event data.
 * @return ECORE_CALLBACK_PASS_ON to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_drm_logind_cb_activate(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Drm_Event_Activate *ev;
   Ecore_Drm_Device *dev;
   Ecore_Drm_Output *output;
   Ecore_Drm_Input *input;
   Eina_List *l;

   if ((!event) || (!data)) return ECORE_CALLBACK_RENEW;

   ev = event;
   dev = data;

   dev->active = ev->active;

   if (ev->active)
     {
        /* set output mode */
        EINA_LIST_FOREACH(dev->outputs, l, output)
          _ecore_drm_output_render_enable(output);

        /* enable inputs */
        EINA_LIST_FOREACH(dev->inputs, l, input)
          ecore_drm_inputs_enable(input);
     }
   else
     {
        Ecore_Drm_Sprite *sprite;

        /* disable inputs */
        EINA_LIST_FOREACH(dev->inputs, l, input)
          ecore_drm_inputs_disable(input);

        /* disable hardware cursor */
        EINA_LIST_FOREACH(dev->outputs, l, output)
          _ecore_drm_output_render_disable(output);

        /* disable sprites */
        EINA_LIST_FOREACH(dev->sprites, l, sprite)
          ecore_drm_sprites_fb_set(sprite, 0, 0);
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @internal
 * @brief Connects to logind and sets up the DRM device for a session.
 *
 * This function performs several steps:
 * 1. If systemd is available, it retrieves the session ID and seat,
 *    verifying the seat matches the device's configured seat. It then
 *    gets the VT number.
 * 2. Initializes D-Bus communication for logind.
 * 3. Takes control of the logind session.
 * 4. Sets up the VT using _ecore_drm_logind_vt_setup().
 * 5. Registers an event handler for VT switch signals (_ecore_drm_logind_cb_vt_signal).
 * 6. Registers an event handler for activation events (_ecore_drm_logind_cb_activate).
 *
 * @param dev The Ecore_Drm_Device to connect and configure.
 * @return EINA_TRUE on successful connection and setup, EINA_FALSE otherwise.
 *         On failure, appropriate cleanup (like releasing session control or
 *         shutting down D-Bus) is attempted.
 */
Eina_Bool
_ecore_drm_logind_connect(Ecore_Drm_Device *dev)
{
#ifdef HAVE_SYSTEMD
   char *seat = NULL;

   _ecore_drm_sd_init();
   if ((!_ecore_sd_pid_get_session) ||
       (!_ecore_sd_session_get_seat))
     {
        ERR("Could not get systemd session");
        return EINA_FALSE;
     }
   /* get session id */
   if (_ecore_sd_pid_get_session(getpid(), &dev->session) < 0)
     {
        ERR("Could not get systemd session");
        return EINA_FALSE;
     }
   if (_ecore_sd_session_get_seat(dev->session, &seat) < 0)
     {
        ERR("Could not get systemd seat");
        return EINA_FALSE;
     }
   else if (strcmp(dev->seat, seat))
     {
        ERR("Session seat '%s' differs from device seat '%s'", seat, dev->seat);
        free(seat);
        return EINA_FALSE;
     }
   free(seat);
   if (!_ecore_drm_logind_vt_get(dev)) return EINA_FALSE;
#endif

   if (!_ecore_drm_dbus_init(dev)) return EINA_FALSE;

   /* take control of session */
   if (!_ecore_drm_dbus_session_take())
     {
        ERR("Could not take control of session");
        goto take_err;
     }

   /* setup vt */
   if (!_ecore_drm_logind_vt_setup(dev))
     {
        ERR("Could not setup vt '%d'", dev->vt);
        goto vt_err;
     }

   /* setup handler for vt signals */
   if (!dev->tty.event_hdlr)
     {
        dev->tty.event_hdlr =
          ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER,
                                  _ecore_drm_logind_cb_vt_signal, dev);
     }

   if (!active_hdlr)
     {
        active_hdlr =
          ecore_event_handler_add(ECORE_DRM_EVENT_ACTIVATE,
                                  _ecore_drm_logind_cb_activate, dev);
     }

   return EINA_TRUE;

vt_err:
   _ecore_drm_dbus_session_release();
take_err:
   _ecore_drm_dbus_shutdown();
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Disconnects from logind and cleans up resources for the DRM device.
 *
 * This function performs the following cleanup:
 * 1. Deletes the ECORE_DRM_EVENT_ACTIVATE event handler.
 * 2. Closes the TTY associated with the device.
 * 3. Releases control of the logind session via D-Bus.
 * 4. Shuts down D-Bus communication.
 *
 * @param dev The Ecore_Drm_Device to disconnect.
 */
void
_ecore_drm_logind_disconnect(Ecore_Drm_Device *dev)
{
   if (active_hdlr) ecore_event_handler_del(active_hdlr);
   active_hdlr = NULL;

   ecore_drm_tty_close(dev);
   _ecore_drm_dbus_session_release();
   _ecore_drm_dbus_shutdown();
}

/**
 * @internal
 * @brief Restores the TTY state for the given DRM device.
 *
 * This typically involves resetting terminal modes or other TTY-specific
 * configurations that might have been altered during the session.
 *
 * @param dev The Ecore_Drm_Device whose TTY needs to be restored.
 */
void
_ecore_drm_logind_restore(Ecore_Drm_Device *dev)
{
   _ecore_drm_tty_restore(dev);
}

/**
 * @internal
 * @brief Opens a DRM device through logind, requesting control.
 *
 * This function first stats the device file to ensure it's a character device.
 * Then, it uses D-Bus to ask logind to grant access to the DRM device
 * identified by its major and minor numbers. The result of this operation
 * (success or failure, and the opened file descriptor if successful) is
 * communicated asynchronously via the provided callback.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0").
 * @param callback The function to call when logind responds to the open request.
 *                 The callback will receive the opened fd (or -1 on error) and
 *                 a boolean indicating if the device is paused.
 * @param data User data to pass to the callback function.
 * @return EINA_TRUE if the request to logind was successfully made, EINA_FALSE
 *         otherwise (e.g., stat failed, device is not a character device, or
 *         D-Bus call failed immediately). Note that a EINA_TRUE return does
 *         not mean the device was successfully opened, only that the request
 *         was sent.
 */
Eina_Bool
_ecore_drm_logind_device_open(const char *device, Ecore_Drm_Open_Cb callback, void *data)
{
   struct stat st;

   if (stat(device, &st) < 0) return EINA_FALSE;
   if (!S_ISCHR(st.st_mode)) return EINA_FALSE;

   if (_ecore_drm_dbus_device_take(major(st.st_rdev), minor(st.st_rdev),
                                   callback, data) < 0)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Opens a DRM device through logind synchronously, without a pending callback.
 *
 * This function is similar to _ecore_drm_logind_device_open but operates
 * synchronously. It requests access to the DRM device from logind and
 * directly returns the file descriptor.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0").
 * @return The opened file descriptor for the DRM device on success.
 *         Returns -1 on failure (e.g., stat failed, not a character device,
 *         logind denied access, or D-Bus error).
 */
int
_ecore_drm_logind_device_open_no_pending(const char *device)
{
   struct stat st;

   if (stat(device, &st) < 0) return -1;
   if (!S_ISCHR(st.st_mode)) return -1;

   return _ecore_drm_dbus_device_take_no_pending(major(st.st_rdev), minor(st.st_rdev), NULL, -1);
}

/**
 * @internal
 * @brief Closes a DRM device through logind, releasing control.
 *
 * This function informs logind that the application is done with the specified
 * DRM device. It stats the device to get its major and minor numbers and then
 * makes a D-Bus call to logind to release the device.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0") that
 *               was previously opened via _ecore_drm_logind_device_open or
 *               _ecore_drm_logind_device_open_no_pending.
 */
void
_ecore_drm_logind_device_close(const char *device)
{
   struct stat st;

   if (stat(device, &st) < 0) return;
   if (!S_ISCHR(st.st_mode)) return;

   _ecore_drm_dbus_device_release(major(st.st_rdev), minor(st.st_rdev));
}
