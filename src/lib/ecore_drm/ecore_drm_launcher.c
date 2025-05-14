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

/**
 * @internal
 * @brief Flag indicating if logind is being used for device management.
 *
 * This variable is EINA_TRUE if the system is using logind and ecore_drm
 * has successfully connected to it. Otherwise, it's EINA_FALSE, implying
 * direct TTY/VT management or root privileges are required.
 */
static Eina_Bool logind = EINA_FALSE;

/**
 * @internal
 * @brief Callback function to handle VT (Virtual Terminal) switch events.
 *
 * This function is triggered on key down events. It checks for
 * CTRL+ALT+F[1-8] key combinations to initiate a VT switch.
 *
 * @param data The Ecore_Drm_Device associated with this event.
 * @param type The type of the event (unused).
 * @param event The Ecore_Event_Key structure containing key press details.
 * @return ECORE_CALLBACK_PASS_ON always, to allow other handlers to process the event.
 */
static Eina_Bool
_ecore_drm_launcher_cb_vt_switch(void *data, int type EINA_UNUSED, void *event)
{
   Ecore_Drm_Device *dev;
   Ecore_Event_Key *ev;
   int keycode;
   int vt;

   dev = data;
   ev = event;
   keycode = ev->keycode - 8;

   if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) &&
       (keycode >= KEY_F1) && (keycode <= KEY_F8))
     {
        vt = (keycode - KEY_F1 + 1);

        if (!_ecore_drm_tty_switch(dev, vt))
          ERR("Failed to activate vt");
     }

   return ECORE_CALLBACK_PASS_ON;
}

/**
 * @internal
 * @brief Sets file descriptor flags for a device.
 *
 * This function modifies the flags of an open file descriptor, typically
 * to set it to non-blocking mode and control the close-on-exec flag.
 *
 * @param fd The file descriptor to modify.
 * @param flags The flags to apply (e.g., O_NONBLOCK, O_CLOEXEC).
 *              If O_NONBLOCK is set in `flags`, F_GETFL/F_SETFL will be used to add it.
 *              If O_CLOEXEC is *not* set in `flags`, FD_CLOEXEC will be cleared using F_GETFD/F_SETFD.
 * @return The file descriptor `fd` on success, or -1 on failure.
 */
int
_ecore_drm_launcher_device_flags_set(int fd, int flags)
{
   int fl;

   fl = fcntl(fd, F_GETFL);
   if (fl < 0) return -1;

   if (flags & O_NONBLOCK)
     fl |= O_NONBLOCK;

   if (fcntl(fd, F_SETFL, fl) < 0)
     return -1;

   fl = fcntl(fd, F_GETFD);
   if (fl < 0) return -1;

   if (!(flags & O_CLOEXEC))
     fl &= ~FD_CLOEXEC;

   if (fcntl(fd, F_SETFD, fl) < 0)
     return -1;

   return fd;
}

/**
 * @brief Connects to the system's session manager (logind) or sets up direct TTY access.
 *
 * This function attempts to establish a connection with logind. If logind is
 * not available or the connection fails, it falls back to direct TTY management
 * if the process has root privileges. It also sets up a handler for VT switch
 * key combinations.
 *
 * @param dev The Ecore_Drm_Device to associate with the launcher.
 * @return EINA_TRUE on successful connection or setup, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm_launcher_connect(Ecore_Drm_Device *dev)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   /* try to connect to logind */
   if (!(logind = _ecore_drm_logind_connect(dev)))
     {
        DBG("Launcher: Logind not supported");
        if (geteuid() == 0)
          {
             DBG("Launcher: Trying to continue with root privileges");
             if (!ecore_drm_tty_open(dev, NULL))
               {
                  ERR("Launcher: Could not setup tty");
                  return EINA_FALSE;
               }
          }
        else
          {
             ERR("Launcher: Root privileges needed");
             return EINA_FALSE;
          }
     }

   dev->tty.switch_hdlr =
     ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                             _ecore_drm_launcher_cb_vt_switch, dev);

   return EINA_TRUE;
}

/**
 * @brief Disconnects from the session manager or releases TTY resources.
 *
 * This function cleans up resources acquired by ecore_drm_launcher_connect().
 * If connected via logind, it disconnects. Otherwise, it closes the TTY.
 * It also removes the VT switch event handler.
 *
 * @param dev The Ecore_Drm_Device associated with the launcher.
 */
EAPI void
ecore_drm_launcher_disconnect(Ecore_Drm_Device *dev)
{
   EINA_SAFETY_ON_NULL_RETURN(dev);

   if (dev->tty.switch_hdlr) ecore_event_handler_del(dev->tty.switch_hdlr);
   dev->tty.switch_hdlr = NULL;

   if (!logind)
     {
        if (!ecore_drm_tty_close(dev))
          ERR("Launcher: Could not close tty");
     }
   else
     {
        _ecore_drm_logind_disconnect(dev);
     }
}

/**
 * @internal
 * @brief Opens a DRM device, either via logind or directly.
 *
 * This function handles opening a specified DRM device. If logind is active,
 * it requests logind to open the device. Otherwise, it attempts a direct
 * `open()` call. The result (file descriptor) is passed to the provided
 * callback.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0").
 * @param callback The function to call upon successful or failed opening.
 *                 The callback will receive `data`, the opened file descriptor (or -1 on error),
 *                 and a boolean indicating if the open operation is pending (EINA_TRUE if logind is used
 *                 and the open is asynchronous, EINA_FALSE otherwise).
 * @param data User data to pass to the callback function.
 * @param flags Flags to use when opening the device directly (e.g., O_RDWR).
 *              These flags are ORed with O_CLOEXEC.
 * @return EINA_TRUE if the open request was successfully initiated, EINA_FALSE on immediate failure.
 *         Note that for logind-based opens, success here doesn't mean the device is open yet,
 *         the callback will indicate the final status.
 */
Eina_Bool
_ecore_drm_launcher_device_open(const char *device, Ecore_Drm_Open_Cb callback, void *data, int flags)
{
   int fd = -1;
   struct stat s;

   if (logind)
     {
        if (!_ecore_drm_logind_device_open(device, callback, data))
          return EINA_FALSE;
     }
   else
     {
        fd = open(device, flags | O_CLOEXEC);
        if (fd < 0) return EINA_FALSE;
        if (fstat(fd, &s) == -1)
          {
             close(fd);
             fd = -1;
             return EINA_FALSE;
          }

        callback(data, fd, EINA_FALSE);
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Opens a DRM device synchronously, either via logind or directly.
 *
 * This function is similar to _ecore_drm_launcher_device_open but operates
 * synchronously. If using logind, it waits for logind to provide the
 * file descriptor.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0").
 * @param flags Flags to use when opening the device (e.g., O_RDWR, O_NONBLOCK).
 *              These flags are ORed with O_CLOEXEC.
 * @return The opened file descriptor on success, or -1 on failure.
 */
int
_ecore_drm_launcher_device_open_no_pending(const char *device, int flags)
{
   int fd = -1;
   struct stat s;

   if (logind)
     {
        fd = _ecore_drm_logind_device_open_no_pending(device);
        if (fd < 0) return -1;
        if (_ecore_drm_launcher_device_flags_set(fd, flags | O_CLOEXEC) < 0)
          {
             close(fd);
             _ecore_drm_logind_device_close(device);
             return -1;
          }
     }
   else
     {
        fd = open(device, flags | O_CLOEXEC);
        if (fd < 0) return fd;
        if (fstat(fd, &s) == -1)
          {
             close(fd);
             return -1;
          }
     }

   return fd;
}

/**
 * @internal
 * @brief Closes a DRM device.
 *
 * This function closes a file descriptor associated with a DRM device.
 * If logind was used to open the device, it also informs logind that the
 * device is being released.
 *
 * @param device The path to the DRM device (e.g., "/dev/dri/card0").
 *               This is used to notify logind if applicable. Can be NULL
 *               if logind is not active or if the device path is not known,
 *               but it's best to provide it if logind might be in use.
 * @param fd The file descriptor of the device to close.
 */
void
_ecore_drm_launcher_device_close(const char *device, int fd)
{
   if ((logind) && (device)) _ecore_drm_logind_device_close(device);

   if (fd < 0) return;
   close(fd);
}
