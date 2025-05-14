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
 * @brief Event type for when a new seat is added.
 * This event is triggered when a new input seat is created and added to the system.
 * The event_info parameter of the Ecore_Event_Handler will be NULL for this event.
 */
EAPI int ECORE_DRM_EVENT_SEAT_ADD = -1;

/**
 * @brief Hash table to store file descriptors associated with device paths.
 * This is used to keep track of opened device file descriptors, mapping the
 * device path (char *) to the file descriptor (int).
 */
static Eina_Hash *_fd_hash = NULL;

/* local functions */
/**
 * @brief Callback function to open a device file in a restricted manner.
 * This function is part of the libinput_interface and is called by libinput
 * when it needs to open a device file. It uses the ecore_drm_launcher
 * to perform the open operation, which may involve special privileges.
 *
 * @param path The path to the device file to open.
 * @param flags The flags to use when opening the file (e.g., O_RDONLY, O_RDWR).
 * @param data User data, expected to be an Ecore_Drm_Input pointer.
 * @return The file descriptor of the opened device, or -1 on error.
 */
static int
_cb_open_restricted(const char *path, int flags, void *data)
{
   Ecore_Drm_Input *input;
   int fd = -1;

   if (!(input = data)) return -1;

   /* try to open the device */
   fd = _ecore_drm_launcher_device_open_no_pending(path, flags);
   if (fd < 0) ERR("Could not open device");
   if (_fd_hash)
     eina_hash_add(_fd_hash, path, (void *)(intptr_t)fd);

   return fd;
}

/**
 * @brief Callback function to close a device file that was opened by _cb_open_restricted.
 * This function is part of the libinput_interface and is called by libinput
 * when it needs to close a device file. It uses the ecore_drm_launcher
 * to perform the close operation.
 *
 * @param fd The file descriptor of the device to close.
 * @param data User data, expected to be an Ecore_Drm_Input pointer.
 */
static void
_cb_close_restricted(int fd, void *data)
{
   Ecore_Drm_Input *input;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;
   Eina_List *l, *ll;

   if (!(input = data)) return;

   EINA_LIST_FOREACH(input->dev->seats, l, seat)
     {
        EINA_LIST_FOREACH(seat->devices, ll, edev)
          {
             if (edev->fd == fd)
               {
                  _ecore_drm_launcher_device_close(edev->path, fd);

                  /* re-initialize fd after closing */
                  edev->fd = -1;
                  return;
               }
          }
     }
}

/**
 * @brief Creates a new Ecore_Drm_Seat structure.
 * A seat represents a collection of input devices (keyboard, mouse, touch, etc.)
 * that belong to a single user. This function allocates and initializes a new
 * seat, associates it with the given input, and adds it to the list of seats
 * for the Ecore_Drm_Device. It also triggers an ECORE_DRM_EVENT_SEAT_ADD event.
 *
 * @param input The Ecore_Drm_Input this seat will be associated with.
 * @param seat The name of the seat to create (e.g., "seat0").
 * @return A pointer to the newly created Ecore_Drm_Seat, or NULL on failure.
 */
static Ecore_Drm_Seat *
_seat_create(Ecore_Drm_Input *input, const char *seat)
{
   Ecore_Drm_Seat *s;

   /* try to allocate space for new seat */
   if (!(s = calloc(1, sizeof(Ecore_Drm_Seat))))
     return NULL;

   s->input = input;
   s->name = eina_stringshare_add(seat);

   /* add this new seat to list */
   input->dev->seats = eina_list_append(input->dev->seats, s);

   ecore_event_add(ECORE_DRM_EVENT_SEAT_ADD, NULL, NULL, NULL);

   return s;
}

/**
 * @brief Retrieves an existing Ecore_Drm_Seat by name, or creates it if not found.
 * This function first searches for a seat with the given name. If found, it is
 * returned. Otherwise, a new seat with that name is created using _seat_create().
 *
 * @param input The Ecore_Drm_Input to search within or associate a new seat with.
 * @param seat The name of the seat to find or create.
 * @return A pointer to the Ecore_Drm_Seat, or NULL if creation fails.
 */
static Ecore_Drm_Seat *
_seat_get(Ecore_Drm_Input *input, const char *seat)
{
   Ecore_Drm_Seat *s;
   Eina_List *l;

   /* search for this name in existing seats */
   EINA_LIST_FOREACH(input->dev->seats, l, s)
     if (!strcmp(s->name, seat)) return s;

   return _seat_create(input, seat);
}

/**
 * @brief Handles the addition of a new input device by libinput.
 * This function is called when libinput detects a new input device. It retrieves
 * the seat associated with the device, creates an Ecore_Drm_Evdev representation
 * for the device, and adds it to the seat's device list.
 *
 * @param input The Ecore_Drm_Input context.
 * @param device The libinput_device structure for the newly added device.
 */
static void
_device_added(Ecore_Drm_Input *input, struct libinput_device *device)
{
   struct libinput_seat *libinput_seat;
   const char *seat_name;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;

   libinput_seat = libinput_device_get_seat(device);
   seat_name = libinput_seat_get_logical_name(libinput_seat);

   /* try to get a seat */
   if (!(seat = _seat_get(input, seat_name)))
     {
        ERR("Could not get matching seat: %s", seat_name);
        return;
     }

   /* try to create a new evdev device */
   if (!(edev = _ecore_drm_evdev_device_create(seat, device)))
     {
        ERR("Failed to create new evdev device");
        return;
     }

   edev->fd = (int)(intptr_t)eina_hash_find(_fd_hash, edev->path);

   /* append this device to the seat */
   seat->devices = eina_list_append(seat->devices, edev);
}

/**
 * @brief Handles the removal of an input device by libinput.
 * This function is called when libinput detects that an input device has been
 * removed. It retrieves the Ecore_Drm_Evdev structure associated with the
 * libinput_device, removes it from its seat's device list, closes the device
 * via the launcher, and frees the Ecore_Drm_Evdev structure.
 *
 * @param input The Ecore_Drm_Input context (unused in this function).
 * @param device The libinput_device structure for the removed device.
 */
static void
_device_removed(Ecore_Drm_Input *input EINA_UNUSED, struct libinput_device *device)
{
   Ecore_Drm_Evdev *edev;

   /* try to get the evdev structure */
   if (!(edev = libinput_device_get_user_data(device)))
     return;

   /* remove this evdev from the seat's list of devices */
   edev->seat->devices = eina_list_remove(edev->seat->devices, edev);

   if (_fd_hash)
     eina_hash_del_by_key(_fd_hash, edev->path);

   /* tell launcher to release device */
   _ecore_drm_launcher_device_close(edev->path, edev->fd);

   /* destroy this evdev */
   _ecore_drm_evdev_device_destroy(edev);
}

/**
 * @brief Processes udev-related events from libinput.
 * This function specifically handles LIBINPUT_EVENT_DEVICE_ADDED and
 * LIBINPUT_EVENT_DEVICE_REMOVED events. Other event types are ignored by
 * this function.
 *
 * @param event The libinput_event to process.
 * @return EINA_TRUE if the event was a device added/removed event and was handled,
 *         EINA_FALSE otherwise.
 */
static int
_udev_event_process(struct libinput_event *event)
{
   struct libinput *libinput;
   struct libinput_device *device;
   Ecore_Drm_Input *input;
   Eina_Bool ret = EINA_TRUE;

   libinput = libinput_event_get_context(event);
   input = libinput_get_user_data(libinput);
   device = libinput_event_get_device(event);

   switch (libinput_event_get_type(event))
     {
      case LIBINPUT_EVENT_DEVICE_ADDED:
        _device_added(input, device);
        break;
      case LIBINPUT_EVENT_DEVICE_REMOVED:
        _device_removed(input, device);
        break;
      default:
        ret = EINA_FALSE;
     }

   return ret;
}

/**
 * @brief Processes a single input event from libinput.
 * This function acts as a dispatcher. It first attempts to process the event
 * as a udev event (device added/removed). If not handled, it then attempts
 * to process it as an evdev event (actual input like key presses, pointer motion).
 *
 * @param event The libinput_event to process.
 */
static void
_input_event_process(struct libinput_event *event)
{
   if (_udev_event_process(event)) return;
   if (_ecore_drm_evdev_event_process(event)) return;
}

/**
 * @brief Processes all pending input events from libinput.
 * This function retrieves and processes events from the libinput context
 * in a loop until no more events are pending. Each event is passed to
 * _input_event_process() for handling.
 *
 * @param input The Ecore_Drm_Input context from which to fetch events.
 */
static void
_input_events_process(Ecore_Drm_Input *input)
{
   struct libinput_event *event;

   while ((event = libinput_get_event(input->libinput)))
     {
        _input_event_process(event);
        libinput_event_destroy(event);
     }
}

/**
 * @brief Callback function for handling input events from the libinput file descriptor.
 * This function is registered as an Ecore_Fd_Handler callback. When data is
 * available on the libinput file descriptor, this function is called. It
 * dispatches pending libinput events and then processes them.
 *
 * @param data User data, expected to be an Ecore_Drm_Input pointer.
 * @param hdlr The Ecore_Fd_Handler that triggered this callback (unused).
 * @return EINA_TRUE to keep the handler active, EINA_FALSE to remove it.
 *         Currently always returns EINA_TRUE.
 */
static Eina_Bool
_cb_input_dispatch(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Drm_Input *input;

   if (!(input = data)) return EINA_TRUE;

   if (libinput_dispatch(input->libinput) != 0)
     ERR("Failed to dispatch libinput events");

   /* process pending events */
   _input_events_process(input);

   return EINA_TRUE;
}

/**
 * @brief libinput_interface implementation for Ecore_Drm.
 * This structure provides libinput with functions to open and close
 * device files in a restricted (potentially privileged) manner, typically
 * through a launcher mechanism.
 */
const struct libinput_interface _input_interface =
{
   _cb_open_restricted,
   _cb_close_restricted,
};

/* public functions */
/**
 * @brief Creates and initializes input handling for an Ecore_Drm_Device.
 * This function sets up libinput for the given DRM device. It creates an
 * Ecore_Drm_Input structure, initializes a libinput context using udev,
 * assigns the seat, processes any initial pending events, and enables input.
 *
 * @param dev The Ecore_Drm_Device to create input handling for.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_drm_inputs_create(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Input *input;

   /* check for valid device */
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);

   /* try to allocate space for new input structure */
   if (!(input = calloc(1, sizeof(Ecore_Drm_Input))))
     return EINA_FALSE;

   /* set reference for parent device */
   input->dev = dev;

   /* try to create libinput context */
   input->libinput =
     libinput_udev_create_context(&_input_interface, input, eeze_udev_get());
   if (!input->libinput)
     {
        ERR("Could not create libinput context");
        goto err;
     }

   /* set libinput log priority */
   libinput_log_set_priority(input->libinput, LIBINPUT_LOG_PRIORITY_INFO);

   /* assign udev seat */
   if (libinput_udev_assign_seat(input->libinput, dev->seat) != 0)
     {
        ERR("Failed to assign seat");
        goto err;
     }

   /* process pending events */
   _input_events_process(input);

   /* enable this input */
   if (!ecore_drm_inputs_enable(input))
     {
        ERR("Failed to enable input");
        goto err;
     }

   /* append this input */
   dev->inputs = eina_list_append(dev->inputs, input);

   return EINA_TRUE;

err:
   if (input->libinput) libinput_unref(input->libinput);
   free(input);
   return EINA_FALSE;
}

/**
 * @brief Destroys input handling resources for an Ecore_Drm_Device.
 * This function cleans up all resources associated with input handling for the
 * given DRM device. It iterates through all seats and their devices, closing
 * them via the launcher and destroying the Ecore_Drm_Evdev structures. It also
 * frees all Ecore_Drm_Seat and Ecore_Drm_Input structures.
 *
 * @param dev The Ecore_Drm_Device whose input resources are to be destroyed.
 */
EAPI void
ecore_drm_inputs_destroy(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Input *input;
   Ecore_Drm_Seat *seat;
   Ecore_Drm_Evdev *edev;

   EINA_SAFETY_ON_NULL_RETURN(dev);
   EINA_LIST_FREE(dev->seats, seat)
     {
        EINA_LIST_FREE(seat->devices, edev)
          {
             _ecore_drm_launcher_device_close(edev->path, edev->fd);
             _ecore_drm_evdev_device_destroy(edev);
          }

        if (seat->name) eina_stringshare_del(seat->name);
        free(seat);
     }

   EINA_LIST_FREE(dev->inputs, input)
     {
        if (input->hdlr) ecore_main_fd_handler_del(input->hdlr);
        if (input->libinput) libinput_unref(input->libinput);
        free(input);
     }
}

/**
 * @brief Enables input processing for a given Ecore_Drm_Input.
 * This function sets up an Ecore_Fd_Handler to listen for events on the
 * libinput file descriptor. If input was previously suspended, it resumes
 * libinput and processes any pending events.
 *
 * @param input The Ecore_Drm_Input to enable.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_drm_inputs_enable(Ecore_Drm_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(input, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(input->libinput, EINA_FALSE);

   input->fd = libinput_get_fd(input->libinput);

   if (!input->hdlr)
     {
        input->hdlr =
          ecore_main_fd_handler_add(input->fd, ECORE_FD_READ,
                                    _cb_input_dispatch, input, NULL, NULL);
     }

   if (input->suspended)
     {
        if (libinput_resume(input->libinput) != 0)
          goto err;

        input->suspended = EINA_FALSE;

        /* process pending events */
        _input_events_process(input);
     }

   input->enabled = EINA_TRUE;
   input->suspended = EINA_FALSE;

   return EINA_TRUE;

err:
   input->enabled = EINA_FALSE;
   if (input->hdlr) ecore_main_fd_handler_del(input->hdlr);
   input->hdlr = NULL;
   return EINA_FALSE;
}

/**
 * @brief Disables input processing for a given Ecore_Drm_Input.
 * This function suspends libinput, effectively stopping it from generating
 * new input events. It also processes any events that were pending before
 * suspension. The Ecore_Fd_Handler is not removed here, but libinput
 * will stop signaling activity on its fd.
 *
 * @param input The Ecore_Drm_Input to disable.
 */
EAPI void
ecore_drm_inputs_disable(Ecore_Drm_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN(input);
   EINA_SAFETY_ON_TRUE_RETURN(input->suspended);

   /* suspend this input */
   libinput_suspend(input->libinput);

   /* process pending events */
   _input_events_process(input);

   input->suspended = EINA_TRUE;
}

/**
 * @brief Initializes the Ecore_Drm input subsystem.
 * This function is called, usually at application startup, to prepare the
 * input handling components. Currently, it initializes the _fd_hash.
 * @internal
 */
void
_ecore_drm_inputs_init(void)
{
   _fd_hash = eina_hash_string_superfast_new(NULL);
}

/**
 * @brief Shuts down the Ecore_Drm input subsystem.
 * This function is called, usually at application shutdown, to clean up
 * resources used by the input handling components. Currently, it frees the _fd_hash.
 * @internal
 */
void
_ecore_drm_inputs_shutdown(void)
{
   eina_hash_free(_fd_hash);
   _fd_hash = NULL;
}
