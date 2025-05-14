/* this file contains code copied from weston; the copyright notice is below */
/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "elput_private.h"
#include <libudev.h>

/**
 * @brief Callback to open a device file descriptor.
 *
 * This function is part of the libinput_interface. It handles opening a device
 * specified by @p path.
 * If an Ecore input thread is active in @p em, this function attempts to
 * perform the open operation asynchronously. It creates a pipe to communicate
 * the resulting file descriptor (or an error) back from the thread that
 * performs the actual open via em->interface->open_async.
 * If no thread is active or open_async is not supported, it falls back to a
 * synchronous open via em->interface->open.
 *
 * @param path The file system path to the device node.
 * @param flags The flags to use when opening the device (e.g., O_RDWR, O_NONBLOCK).
 * @param data User data, expected to be an Elput_Manager instance.
 * @return The opened file descriptor on success, or -1 on failure.
 */
static int
_cb_open_restricted(const char *path, int flags, void *data)
{
   Elput_Manager *em = data;
   int ret = -1;
   Elput_Async_Open *ao;
   int p[2];

   if (!em->input.thread)
     return em->interface->open(em, path, flags);
   if (!em->interface->open_async) return ret;
   if (ecore_thread_check(em->input.thread)) return ret;
   ao = calloc(1, sizeof(Elput_Async_Open));
   if (!ao) return ret;
   if (pipe2(p, O_CLOEXEC) < 0)
     {
        free(ao);
        return ret;
     }
   ao->manager = em;
   ao->path = strdup(path);
   ao->flags = flags;
   em->input.pipe = p[1];
   ecore_thread_feedback(em->input.thread, ao);
   while (!ecore_thread_check(em->input.thread))
     {
        int avail, fd;
        fd_set rfds, wfds, exfds;
        struct timeval tv, *t;

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        FD_ZERO(&exfds);
        FD_SET(p[0], &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 300;
        t = &tv;
        avail = select(p[0] + 1, &rfds, &wfds, &exfds, t);
        if (avail > 0)
          {
             if (read(p[0], &fd, sizeof(int)) < 1)
               ret = -1;
             else
               ret = fd;
             break;
          }
        if (avail < 0) break;
     }
   close(p[0]);
   return ret;
}

/**
 * @brief Callback to close a device file descriptor.
 *
 * This function is part of the libinput_interface. It handles closing a
 * device file descriptor previously opened by _cb_open_restricted.
 *
 * @param fd The file descriptor to close.
 * @param data User data, expected to be an Elput_Manager instance.
 */
static void
_cb_close_restricted(int fd, void *data)
{
   Elput_Manager *em;

   em = data;
   elput_manager_close(em, fd);
}

/**
 * @brief libinput interface implementation for elput.
 *
 * This structure provides libinput with functions to open and close
 * file descriptors for input devices.
 */
const struct libinput_interface _input_interface =
{
   _cb_open_restricted,
   _cb_close_restricted,
};

/**
 * @brief Creates and initializes a new Elput_Seat.
 *
 * A seat represents a collection of input devices that logically belong
 * together (e.g., a keyboard, mouse, and touchscreen for a single user).
 * The newly created seat is added to the manager's list of seats.
 *
 * @param em The Elput_Manager instance.
 * @param name The name for the new seat (e.g., "seat0").
 * @return A pointer to the newly created Elput_Seat, or NULL on failure.
 */
static Elput_Seat *
_udev_seat_create(Elput_Manager *em, const char *name)
{
   Elput_Seat *eseat;

   eseat = calloc(1, sizeof(Elput_Seat));
   if (!eseat) return NULL;

   eseat->manager = em;
   eseat->refs = 1;

   eseat->name = eina_stringshare_add(name);
   em->input.seats = eina_list_append(em->input.seats, eseat);

   return eseat;
}

/**
 * @brief Destroys an Elput_Seat and frees its associated resources.
 *
 * This function decrements the reference count of the seat. If the reference
 * count reaches zero, it proceeds to destroy all associated devices,
 * keyboard, pointer, and touch handlers, removes the seat from its manager,
 * and frees the seat structure itself.
 *
 * @param eseat The Elput_Seat to destroy.
 */
void
_udev_seat_destroy(Elput_Seat *eseat)
{
   Elput_Device *edev;

   eseat->refs--;
   if (eseat->refs) return;

   EINA_LIST_FREE(eseat->devices, edev)
     _evdev_device_destroy(edev);

   if (eseat->kbd) _evdev_keyboard_destroy(eseat->kbd);
   eseat->kbd = NULL;
   if (eseat->ptr) _evdev_pointer_destroy(eseat->ptr);
   eseat->ptr = NULL;
   if (eseat->touch) _evdev_touch_destroy(eseat->touch);
   eseat->touch = NULL;
   if (eseat->manager->input.seats)
     eseat->manager->input.seats = eina_list_remove(eseat->manager->input.seats, eseat);
   if (eseat->refs) return;

   eina_stringshare_del(eseat->name);
   free(eseat);
}

/**
 * @brief Retrieves an Elput_Seat by its name, creating it if it doesn't exist.
 *
 * If @p name is NULL, it defaults to "seat0".
 *
 * @param em The Elput_Manager instance.
 * @param name The name of the seat to retrieve. Defaults to "seat0" if NULL.
 * @return A pointer to the Elput_Seat, or NULL on failure to create.
 */
static Elput_Seat *
_udev_seat_named_get(Elput_Manager *em, const char *name)
{
   Elput_Seat *eseat;
   Eina_List *l;

   if (!name) name = "seat0";

   EINA_LIST_FOREACH(em->input.seats, l, eseat)
     if (!strcmp(eseat->name, name)) return eseat;

   eseat = _udev_seat_create(em, name);
   if (!eseat) return NULL;

   return eseat;
}

/**
 * @brief Retrieves or creates the Elput_Seat associated with a libinput_device.
 *
 * This function uses the physical name of the libinput seat associated with
 * the given libinput device to find or create a corresponding Elput_Seat.
 *
 * @param em The Elput_Manager instance.
 * @param device The libinput_device for which to get the seat.
 * @return A pointer to the Elput_Seat, or NULL on failure.
 */
static Elput_Seat *
_udev_seat_get(Elput_Manager *em, struct libinput_device *device)
{
   struct libinput_seat *lseat;
   const char *name;

   lseat = libinput_device_get_seat(device);
   name = libinput_seat_get_physical_name(lseat);

   return _udev_seat_named_get(em, name);
}

/**
 * @brief Callback function to free an Elput_Event_Device_Change event.
 *
 * This function is called by Ecore when an ELPUT_EVENT_DEVICE_CHANGE event
 * is no longer needed. It decrements the reference count of the associated
 * Elput_Device. If the event type is ELPUT_DEVICE_REMOVED and the device's
 * reference count drops to a point where it can be destroyed (implicitly,
 * though not explicitly checked here beyond the ref count), it removes the
 * device from its seat and destroys it.
 *
 * @param data User data associated with the event (unused).
 * @param event The Elput_Event_Device_Change event to free.
 */
static void
_device_event_cb_free(void *data EINA_UNUSED, void *event)
{
   Elput_Event_Device_Change *ev;

   ev = event;

   ev->device->refs--;
   if (ev->type == ELPUT_DEVICE_REMOVED)
     {
        Elput_Seat *seat;

        seat = ev->device->seat;
        if (seat)
          seat->devices = eina_list_remove(seat->devices, ev->device);

        _evdev_device_destroy(ev->device);
     }

   free(ev);
}

/**
 * @brief Allocates and sends an ELPUT_EVENT_DEVICE_CHANGE Ecore event.
 *
 * This function creates a new device change event, populates it with the
 * given device and type, increments the device's reference count, and
 * adds the event to the Ecore event queue.
 *
 * @param edev The Elput_Device that changed.
 * @param type The type of change (ELPUT_DEVICE_ADDED or ELPUT_DEVICE_REMOVED).
 */
static void
_device_event_send(Elput_Device *edev, Elput_Device_Change_Type type)
{
   Elput_Event_Device_Change *ev;

   ev = calloc(1, sizeof(Elput_Event_Device_Change));
   if (!ev) return;

   ev->device = edev;
   ev->type = type;
   edev->refs++;

   ecore_event_add(ELPUT_EVENT_DEVICE_CHANGE, ev, _device_event_cb_free, NULL);
}

/**
 * @brief Handles the addition of a new input device.
 *
 * This function is called when a LIBINPUT_EVENT_DEVICE_ADDED event occurs.
 * It retrieves or creates the appropriate Elput_Seat for the device,
 * creates an Elput_Device wrapper for the libinput_device, configures
 * properties like pointer rotation based on the manager's settings,
 * and sends an ELPUT_DEVICE_ADDED event.
 *
 * @param em The Elput_Manager instance.
 * @param dev The libinput_device that was added.
 */
static void
_device_add(Elput_Manager *em, struct libinput_device *dev)
{
   Elput_Seat *eseat;
   Elput_Device *edev;

   eseat = _udev_seat_get(em, dev);
   if (!eseat) return;

   edev = _evdev_device_create(eseat, dev);
   if (!edev) return;

   eseat->devices = eina_list_append(eseat->devices, edev);

   DBG("Input Device Added: %s", libinput_device_get_name(dev));

   if (edev->caps & ELPUT_DEVICE_CAPS_KEYBOARD)
     DBG("\tDevice added as Keyboard device");

   if (edev->caps & ELPUT_DEVICE_CAPS_POINTER)
     {
        DBG("\tDevice added as Pointer device");
        switch (em->input.rotation)
          {
           case 0:
             edev->swap = EINA_FALSE;
             edev->invert_x = EINA_FALSE;
             edev->invert_y = EINA_FALSE;
             break;
           case 90:
             edev->swap = EINA_TRUE;
             edev->invert_x = EINA_FALSE;
             edev->invert_y = EINA_TRUE;
             break;
           case 180:
             edev->swap = EINA_FALSE;
             edev->invert_x = EINA_TRUE;
             edev->invert_y = EINA_TRUE;
             break;
           case 270:
             edev->swap = EINA_TRUE;
             edev->invert_x = EINA_TRUE;
             edev->invert_y = EINA_FALSE;
             break;
           default:
             break;
          }
     }

   if (edev->caps & ELPUT_DEVICE_CAPS_TOUCH)
     {
        DBG("\tDevice added as Touch device");
     }

   _device_event_send(edev, ELPUT_DEVICE_ADDED);
}

/**
 * @brief Handles the removal of an input device.
 *
 * This function is called when a LIBINPUT_EVENT_DEVICE_REMOVED event occurs.
 * It retrieves the Elput_Device associated with the libinput_device
 * (stored as user data) and sends an ELPUT_DEVICE_REMOVED event. The actual
 * cleanup of the device happens in _device_event_cb_free.
 *
 * @param em The Elput_Manager instance (unused).
 * @param device The libinput_device that was removed.
 */
static void
_device_remove(Elput_Manager *em EINA_UNUSED, struct libinput_device *device)
{
   Elput_Device *edev;

   edev = libinput_device_get_user_data(device);
   if (!edev) return;

   DBG("Input Device Removed: %s", libinput_device_get_name(device));

   _device_event_send(edev, ELPUT_DEVICE_REMOVED);
}

/**
 * @brief Processes device addition or removal events from libinput.
 *
 * This function checks the type of the libinput_event. If it's a
 * LIBINPUT_EVENT_DEVICE_ADDED or LIBINPUT_EVENT_DEVICE_REMOVED event,
 * it calls the appropriate handler (_device_add or _device_remove).
 *
 * @param event The libinput_event to process.
 * @return 1 if the event was a device added/removed event and was handled,
 *         0 otherwise.
 */
static int
_udev_process_event(struct libinput_event *event)
{
   Elput_Manager *em;
   struct libinput *lib;
   struct libinput_device *dev;
   int ret = 1;

   lib = libinput_event_get_context(event);
   dev = libinput_event_get_device(event);
   em = libinput_get_user_data(lib);

   switch (libinput_event_get_type(event))
     {
      case LIBINPUT_EVENT_DEVICE_ADDED:
        _device_add(em, dev);
        break;
      case LIBINPUT_EVENT_DEVICE_REMOVED:
        _device_remove(em, dev);
        break;
      default:
        ret = 0;
        break;
     }

   return ret;
}

/**
 * @brief Processes a single libinput event.
 *
 * This function acts as a dispatcher for various types of libinput events.
 * It first attempts to process udev-related events (device added/removed).
 * If not handled, and if `only_gesture_events` is false, it tries to process
 * it as an evdev event. Finally, it attempts to process it as a gesture event.
 *
 * @param em The Elput_Manager instance.
 * @param event The libinput_event to process.
 */
static void
_process_event(Elput_Manager *em, struct libinput_event *event)
{
   if (_udev_process_event(event)) return;
   if (!em->only_gesture_events)
     {
        if (_evdev_event_process(event)) return;
     }
   if (_gesture_event_process(event)) return;
}

/**
 * @brief Processes all pending events from libinput.
 *
 * This function retrieves and processes events from the libinput context
 * in a loop until no more events are available. Each event is passed to
 * _process_event for further handling, and then destroyed.
 *
 * @param em The Elput_Manager instance.
 */
static void
_process_events(Elput_Manager *em)
{
   struct libinput_event *event;
   Elput_Input *ei;

   ei = &em->input;
   while ((ei->lib) && (event = libinput_get_event(ei->lib)))
     {
        _process_event(em, event);
        libinput_event_destroy(event);
     }
}

/**
 * @brief Ecore Fd Handler callback for libinput dispatching.
 *
 * This function is called by the Ecore main loop when there is data
 * available to read on the libinput file descriptor. It calls
 * libinput_dispatch() to allow libinput to read and queue events,
 * and then calls _process_events() to handle these queued events.
 *
 * @param data User data, expected to be an Elput_Manager instance.
 * @param hdlr The Ecore_Fd_Handler that triggered this callback (unused).
 * @return EINA_TRUE to keep the handler active, EINA_FALSE to remove it.
 */
static Eina_Bool
_cb_input_dispatch(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Elput_Manager *em;

   em = data;

   if ((em->input.lib) && (libinput_dispatch(em->input.lib) != 0))
     WRN("libinput failed to dispatch events");

   _process_events(em);

   return EINA_TRUE;
}

/**
 * @brief Cancellation handler for the input initialization thread.
 *
 * This function is called if the Ecore thread responsible for initializing
 * libinput is cancelled. It cleans up resources such as pending DBus calls
 * and the communication pipe. If the manager is marked for deletion,
 * it triggers a disconnect.
 *
 * @param data User data, expected to be an Elput_Manager instance.
 * @param eth The Ecore_Thread that was cancelled (unused).
 */
static void
_elput_input_init_cancel(void *data, Ecore_Thread *eth EINA_UNUSED)
{
   Elput_Manager *manager;

   manager = data;
   manager->input.thread = NULL;
   if (manager->input.current_pending)
     {
        eldbus_pending_cancel(manager->input.current_pending);
        if (manager->input.pipe >= 0)
          close(manager->input.pipe);
     }
   if (manager->del)
     elput_manager_disconnect(manager);
}

/**
 * @brief End handler for the input initialization thread.
 *
 * This function is called when the Ecore thread responsible for initializing
 * libinput finishes execution successfully. It sets up an Ecore_Fd_Handler
 * to monitor the libinput file descriptor for new events. If setup is
 * successful, it processes any initially available events. It also handles
 * any pending pointer position settings that were queued before initialization
 * completed.
 *
 * @param data User data, expected to be an Elput_Manager instance.
 * @param eth The Ecore_Thread that finished (unused).
 */
static void
_elput_input_init_end(void *data, Ecore_Thread *eth EINA_UNUSED)
{
   Elput_Manager *manager;

   manager = data;
   manager->input.thread = NULL;
   if (!manager->input.lib) return;

   manager->input.hdlr =
     ecore_main_fd_handler_add(libinput_get_fd(manager->input.lib),
                               ECORE_FD_READ, _cb_input_dispatch,
                               manager, NULL, NULL);

   if (manager->input.hdlr)
      _process_events(manager);
   else
     {
        ERR("Could not create input fd handler");
        libinput_unref(manager->input.lib);
        manager->input.lib = NULL;
     }

   if ((manager->pending_ptr_x) || (manager->pending_ptr_y))
     {
        elput_input_pointer_xy_set(manager, NULL, manager->pending_ptr_x,
                                   manager->pending_ptr_y);
        manager->pending_ptr_x = 0;
        manager->pending_ptr_y = 0;
     }
}

/**
 * @brief Notification handler for the input initialization thread.
 *
 * This function is called by the Ecore thread feedback mechanism when the
 * input initialization thread sends a message. It's used to handle
 * asynchronous open operations requested by _cb_open_restricted.
 * The message data @p msg_data is expected to be an Elput_Async_Open
 * structure containing details for the asynchronous open.
 *
 * @param data User data (unused).
 * @param eth The Ecore_Thread that sent the notification (unused).
 * @param msg_data The message data, an Elput_Async_Open pointer.
 */
static void
_elput_input_init_notify(void *data EINA_UNUSED, Ecore_Thread *eth EINA_UNUSED, void *msg_data)
{
   Elput_Async_Open *ao;

   ao = msg_data;
   ao->manager->interface->open_async(ao->manager, ao->path, ao->flags);
   free(ao->path);
   free(ao);
}

/**
 * @brief Main function for the input initialization Ecore thread.
 *
 * This function runs in a separate thread to initialize the libinput
 * udev context and assign the seat. This is done in a thread to avoid
 * blocking the main loop during potentially lengthy udev operations.
 *
 * @param data User data, expected to be an Elput_Manager instance.
 * @param eth The Ecore_Thread executing this function (unused).
 */
static void
_elput_input_init_thread(void *data, Ecore_Thread *eth EINA_UNUSED)
{
   Elput_Manager *manager;
   struct udev *udev;

   manager = data;
   udev = udev_new();

   manager->input.lib =
     libinput_udev_create_context(&_input_interface, manager, udev);
   if (!manager->input.lib)
     {
        ERR("libinput could not create udev context");
        return;
     }
   udev_unref(udev);

   if (libinput_udev_assign_seat(manager->input.lib, manager->seat))
     {
        ERR("libinput could not assign udev seat");
        libinput_unref(manager->input.lib);
        manager->input.lib = NULL;
     }
}

/**
 * @brief Enables input event processing for the given manager.
 *
 * If not already active, this function sets up an Ecore_Fd_Handler to listen
 * for input events from libinput. If input processing was previously suspended,
 * it resumes libinput and processes any queued events.
 *
 * @param manager The Elput_Manager instance for which to enable input.
 */
void
_elput_input_enable(Elput_Manager *manager)
{
   if (!manager->input.hdlr)
     {
        manager->input.hdlr =
          ecore_main_fd_handler_add(libinput_get_fd(manager->input.lib),
                                    ECORE_FD_READ, _cb_input_dispatch,
                                    &manager->input, NULL, NULL);
     }

   if (manager->input.suspended)
     {
        if (libinput_resume(manager->input.lib) != 0) return;
        manager->input.suspended = EINA_FALSE;
        _process_events(manager);
     }
}

/**
 * @brief Disables input event processing for the given manager.
 *
 * This function suspends libinput, effectively stopping new input events
 * from being processed. It also marks all seats as having pending motion,
 * which might be relevant for state updates when input is re-enabled.
 * Any currently queued events are processed before suspension.
 *
 * @param manager The Elput_Manager instance for which to disable input.
 */
void
_elput_input_disable(Elput_Manager *manager)
{
   Elput_Seat *seat;
   Eina_List *l;

   EINA_LIST_FOREACH(manager->input.seats, l, seat)
     seat->pending_motion = 1;
   if (manager->input.lib) libinput_suspend(manager->input.lib);
   _process_events(manager);
   manager->input.suspended = EINA_TRUE;
}

/**
 * @brief Initializes the elput input system for a given manager.
 *
 * This function sets up the necessary structures for input handling and
 * starts an Ecore thread to initialize the libinput context.
 * Initialization is asynchronous.
 *
 * @param manager The Elput_Manager instance to initialize.
 * @return EINA_TRUE if the initialization thread was successfully started,
 *         EINA_FALSE otherwise.
 * @see _elput_input_init_thread()
 * @see _elput_input_init_end()
 */
EAPI Eina_Bool
elput_input_init(Elput_Manager *manager)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!!manager->input.hdlr, EINA_TRUE);

   memset(&manager->input, 0, sizeof(Elput_Input));
   manager->input.thread =
     ecore_thread_feedback_run(_elput_input_init_thread,
                               _elput_input_init_notify,
                               _elput_input_init_end,
                               _elput_input_init_cancel, manager, 1);
   return !!manager->input.thread;
}

/**
 * @brief Shuts down the elput input system for a given manager.
 *
 * This function cleans up all resources associated with input handling for
 * the specified manager. It deletes the Ecore fd handler, destroys all seats,
 * cancels any running input thread, and unreferences the libinput context.
 *
 * @param manager The Elput_Manager instance to shut down.
 */
EAPI void
elput_input_shutdown(Elput_Manager *manager)
{
   Elput_Seat *seat;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   ecore_main_fd_handler_del(manager->input.hdlr);

   EINA_LIST_FOREACH_SAFE(manager->input.seats, l, ll, seat)
     _udev_seat_destroy(seat);

   if (manager->input.thread)
     ecore_thread_cancel(manager->input.thread);
   else
     {
        libinput_unref(manager->input.lib);
        manager->input.lib = NULL;
     }
}

/**
 * @brief Retrieves the current pointer coordinates for a specific seat.
 *
 * If @p seat is NULL, the default seat "seat0" is used.
 * The coordinates are written to the locations pointed to by @p x and @p y.
 * If @p x or @p y are NULL, the respective coordinate is not written.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param[out] x Pointer to an integer to store the X coordinate.
 * @param[out] y Pointer to an integer to store the Y coordinate.
 */
EAPI void
elput_input_pointer_xy_get(Elput_Manager *manager, const char *seat, int *x, int *y)
{
   Elput_Seat *eseat;
   Eina_List *l;

   if (x) *x = 0;
   if (y) *y = 0;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if (!eina_streq(eseat->name, seat)) continue;
        if (x) *x = eseat->pointer.x;
        if (y) *y = eseat->pointer.y;
        return;
     }
}

/**
 * @brief Sets the current pointer coordinates for a specific seat.
 *
 * This function attempts to update the logical pointer position for the
 * specified seat. If @p seat is NULL, the default seat "seat0" is used.
 * If input initialization is not yet complete or no seats exist, the
 * coordinates are stored as pending and applied later.
 * For existing seats, it updates the seat's pointer coordinates and
 * triggers a motion event on the first pointer device found for that seat.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param x The new X coordinate.
 * @param y The new Y coordinate.
 */
EAPI void
elput_input_pointer_xy_set(Elput_Manager *manager, const char *seat, int x, int y)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   if (eina_list_count(manager->input.seats) < 1)
     {
        manager->pending_ptr_x = x;
        manager->pending_ptr_y = y;
        return;
     }

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if (!eseat->ptr) continue;
        if ((eseat->name) && (strcmp(eseat->name, seat)))
          continue;

        eseat->pointer.x = x;
        eseat->pointer.y = y;
        eseat->ptr->timestamp = ecore_loop_time_get();

        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!libinput_device_has_capability(edev->device,
                                                 LIBINPUT_DEVICE_CAP_POINTER))
               continue;

             _evdev_pointer_motion_send(edev);
             break;
          }
     }
}

/**
 * @brief Sets the left-handed mode for pointer devices on a specific seat.
 *
 * If @p seat is NULL, the default seat "seat0" is used.
 * This function iterates through all pointer devices on the specified seat
 * and attempts to configure their left-handed mode via libinput.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param left EINA_TRUE to enable left-handed mode, EINA_FALSE for right-handed.
 * @return EINA_TRUE if the operation was attempted for all relevant devices
 *         (individual device configuration may still fail, check logs for WRN).
 *         EINA_FALSE if @p manager is NULL.
 */
EAPI Eina_Bool
elput_input_pointer_left_handed_set(Elput_Manager *manager, const char *seat, Eina_Bool left)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if ((eseat->name) && (strcmp(eseat->name, seat)))
          continue;

        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!libinput_device_has_capability(edev->device,
                                                 LIBINPUT_DEVICE_CAP_POINTER))
               continue;

             if (edev->left_handed == left) continue;

             if (libinput_device_config_left_handed_set(edev->device,
                                                        (int)left) !=
                 LIBINPUT_CONFIG_STATUS_SUCCESS)
               {
                  WRN("Failed to set left handed mode for device: %s",
                      libinput_device_get_name(edev->device));
                  continue;
               }
             else
               edev->left_handed = !!left;
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Sets the maximum dimensions for pointer movement.
 *
 * This typically corresponds to the screen or output resolution. These values
 * might be used for scaling or constraining pointer coordinates.
 *
 * @param manager The Elput_Manager instance.
 * @param maxw The maximum width for pointer coordinates.
 * @param maxh The maximum height for pointer coordinates.
 */
EAPI void
elput_input_pointer_max_set(Elput_Manager *manager, int maxw, int maxh)
{
   EINA_SAFETY_ON_NULL_RETURN(manager);
   manager->input.pointer_w = maxw;
   manager->input.pointer_h = maxh;
}

/**
 * @brief Sets the rotation for pointer input devices.
 *
 * This affects how pointer movements are interpreted (e.g., swapping axes,
 * inverting axes) to match screen rotation.
 * The rotation value must be a multiple of 90 degrees (0, 90, 180, 270).
 *
 * @param manager The Elput_Manager instance.
 * @param rotation The rotation angle in degrees. Valid values are 0, 90, 180, 270.
 * @return EINA_TRUE if the rotation value is valid and applied,
 *         EINA_FALSE otherwise (e.g., invalid rotation value, NULL manager).
 */
EAPI Eina_Bool
elput_input_pointer_rotation_set(Elput_Manager *manager, int rotation)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);

   if ((rotation % 90 != 0) || (rotation / 90 > 3) || (rotation < 0))
     return EINA_FALSE;

   manager->input.rotation = rotation;

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!(edev->caps & ELPUT_DEVICE_CAPS_POINTER)) continue;

             switch (rotation)
               {
                case 0:
                  edev->swap = EINA_FALSE;
                  edev->invert_x = EINA_FALSE;
                  edev->invert_y = EINA_FALSE;
                  break;
                case 90:
                  edev->swap = EINA_TRUE;
                  edev->invert_x = EINA_FALSE;
                  edev->invert_y = EINA_TRUE;
                  break;
                case 180:
                  edev->swap = EINA_FALSE;
                  edev->invert_x = EINA_TRUE;
                  edev->invert_y = EINA_TRUE;
                  break;
                case 270:
                  edev->swap = EINA_TRUE;
                  edev->invert_x = EINA_TRUE;
                  edev->invert_y = EINA_FALSE;
                  break;
                default:
                  break;
               }
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Calibrates all input devices based on output dimensions.
 *
 * This function sets the output dimensions (width @p w, height @p h) for the
 * manager and then iterates through all devices on all seats, triggering
 * their individual calibration logic (_evdev_device_calibrate). This is
 * typically used for touch devices to map their input area to the screen area.
 *
 * @param manager The Elput_Manager instance.
 * @param w The width of the output/screen.
 * @param h The height of the output/screen.
 */
EAPI void
elput_input_devices_calibrate(Elput_Manager *manager, int w, int h)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   manager->output_w = w;
   manager->output_h = h;

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             edev->ow = w;
             edev->oh = h;
             _evdev_device_calibrate(edev);
          }
     }
}

/**
 * @brief Enables or disables key remapping for keyboard devices.
 *
 * When enabled, keyboard devices can use a hash table for remapping keys.
 * When disabled, if a remapping hash table exists for a device, it is freed.
 *
 * @param manager The Elput_Manager instance.
 * @param enable EINA_TRUE to enable key remapping, EINA_FALSE to disable.
 * @return EINA_TRUE on success, EINA_FALSE if @p manager is NULL.
 */
EAPI Eina_Bool
elput_input_key_remap_enable(Elput_Manager *manager, Eina_Bool enable)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!(edev->caps & ELPUT_DEVICE_CAPS_KEYBOARD)) continue;

             edev->key_remap = enable;
             if ((!enable) && (edev->key_remap_hash))
               {
                  eina_hash_free(edev->key_remap_hash);
                  edev->key_remap_hash = NULL;
               }
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Sets key remappings for keyboard devices.
 *
 * This function applies a set of key remappings. For each keyboard device
 * that has key remapping enabled, it adds entries to its remapping hash table.
 * The `from_keys` and `to_keys` arrays define the mapping: `from_keys[i]`
 * will be remapped to `to_keys[i]`.
 *
 * @param manager The Elput_Manager instance.
 * @param from_keys An array of key codes to be remapped.
 *                  Example: `int from_keys[] = {KEY_A, KEY_B};`
 * @param to_keys An array of key codes that `from_keys` will be mapped to.
 *                Must be the same size as `from_keys`.
 *                Example: `int to_keys[] = {KEY_X, KEY_Y};` (A becomes X, B becomes Y)
 * @param num The number of elements in `from_keys` and `to_keys` arrays.
 * @return EINA_TRUE on success or if no applicable devices found.
 *         EINA_FALSE if @p manager, @p from_keys, or @p to_keys are NULL,
 *         or if @p num is not positive.
 */
EAPI Eina_Bool
elput_input_key_remap_set(Elput_Manager *manager, int *from_keys, int *to_keys, int num)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;
   int i = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(from_keys, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(to_keys, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((num <= 0), EINA_FALSE);

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!(edev->caps & ELPUT_DEVICE_CAPS_KEYBOARD)) continue;

             if (!edev->key_remap) continue;
             if (!edev->key_remap_hash)
               edev->key_remap_hash = eina_hash_int32_new(NULL);
             if (!edev->key_remap_hash) continue;

             for (i = 0; i < num; i++)
               {
                  if ((!from_keys[i]) || (!to_keys[i]))
                    continue;
               }

             for (i = 0; i < num; i++)
               eina_hash_add(edev->key_remap_hash, &from_keys[i],
                             (void *)(intptr_t)to_keys[i]);
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Sets the XKB context, keymap, and initial group for keyboard handling.
 *
 * This function updates the cached XKB information within the manager.
 * If the provided context or keymap differs from the cached ones,
 * the old ones are unreferenced, and the new ones are referenced.
 * After updating the cache, it triggers a keymap update for all seats.
 *
 * @param manager The Elput_Manager instance.
 * @param context The XKB context (e.g., `struct xkb_context *`).
 *                Both @p context and @p keymap must be either both NULL or both non-NULL.
 * @param keymap The XKB keymap (e.g., `struct xkb_keymap *`).
 * @param group The initial keyboard layout group index.
 */
EAPI void
elput_input_keyboard_info_set(Elput_Manager *manager, void *context, void *keymap, int group)
{
   Eina_List *l;
   Elput_Seat *seat;

   EINA_SAFETY_ON_NULL_RETURN(manager);
   EINA_SAFETY_ON_FALSE_RETURN((!!context) == (!!keymap));

   if ((manager->cached.context == context) &&
       (manager->cached.keymap == keymap))
     return;
   if (context) xkb_context_ref(context);
   if (keymap) xkb_keymap_ref(keymap);
   if (manager->cached.context) xkb_context_unref(manager->cached.context);
   if (manager->cached.keymap) xkb_keymap_unref(manager->cached.keymap);
   manager->cached.context = context;
   manager->cached.keymap = keymap;
   manager->cached.group = group;
   EINA_LIST_FOREACH(manager->input.seats, l, seat)
     _keyboard_keymap_update(seat);
}

/**
 * @brief Sets the active keyboard layout group.
 *
 * If the new @p group is different from the currently cached group,
 * this function updates the cached group in the manager and then
 * triggers a group update for all seats, causing them to switch
 * to the new layout group.
 *
 * @param manager The Elput_Manager instance.
 * @param group The new keyboard layout group index.
 */
EAPI void
elput_input_keyboard_group_set(Elput_Manager *manager, int group)
{
   Eina_List *l;
   Elput_Seat *seat;
   EINA_SAFETY_ON_NULL_RETURN(manager);

   if (manager->cached.group == group) return;
   manager->cached.group = group;
   EINA_LIST_FOREACH(manager->input.seats, l, seat)
     _keyboard_group_update(seat);
}

/**
 * @brief Sets the pointer acceleration profile for devices on a specific seat.
 *
 * If @p seat is NULL, the default seat "seat0" is used.
 * This function iterates through all pointer devices on the specified seat
 * and attempts to set their acceleration profile using libinput.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param profile The acceleration profile to set (e.g.,
 *                LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT,
 *                LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE).
 */
EAPI void
elput_input_pointer_accel_profile_set(Elput_Manager *manager, const char *seat, uint32_t profile)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if ((eseat->name) && (strcmp(eseat->name, seat)))
          continue;

        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!libinput_device_has_capability(edev->device,
                                                 LIBINPUT_DEVICE_CAP_POINTER))
               continue;

             if (libinput_device_config_accel_set_profile(edev->device,
                                                          profile) !=
                 LIBINPUT_CONFIG_STATUS_SUCCESS)
               {
                  WRN("Failed to set acceleration profile for device: %s",
                      libinput_device_get_name(edev->device));
                  continue;
               }
          }
     }
}

/**
 * @brief Sets the pointer acceleration speed for devices on a specific seat.
 *
 * If @p seat is NULL, the default seat "seat0" is used.
 * This function iterates through all pointer devices on the specified seat
 * that support acceleration speed configuration and attempts to set their speed.
 * The @p speed value typically ranges from -1.0 to 1.0.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param speed The acceleration speed to set.
 */
EAPI void
elput_input_pointer_accel_speed_set(Elput_Manager *manager, const char *seat, double speed)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if ((eseat->name) && (strcmp(eseat->name, seat)))
          continue;

        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!libinput_device_has_capability(edev->device,
                                                 LIBINPUT_DEVICE_CAP_POINTER))
               continue;

             if (!libinput_device_config_accel_is_available(edev->device))
               continue;

             if (libinput_device_config_accel_set_speed(edev->device,
                                                        speed) !=
                 LIBINPUT_CONFIG_STATUS_SUCCESS)
               {
                  WRN("Failed to set acceleration speed for device: %s",
                      libinput_device_get_name(edev->device));
                  continue;
               }
          }
     }
}

/**
 * @brief Enables or disables tap-to-click for touch devices on a specific seat.
 *
 * If @p seat is NULL, the default seat "seat0" is used.
 * This function iterates through all pointer-capable devices (which often
 * include touchpads that handle tap-to-click) on the specified seat and
 * attempts to enable or disable their tap-to-click feature via libinput.
 *
 * @param manager The Elput_Manager instance.
 * @param seat The name of the seat (e.g., "seat0"). Can be NULL for default.
 * @param enabled EINA_TRUE to enable tap-to-click, EINA_FALSE to disable.
 */
EAPI void
elput_input_touch_tap_to_click_enabled_set(Elput_Manager *manager, const char *seat, Eina_Bool enabled)
{
   Elput_Seat *eseat;
   Elput_Device *edev;
   Eina_List *l, *ll;
   enum libinput_config_tap_state state;

   EINA_SAFETY_ON_NULL_RETURN(manager);

   state = enabled ? LIBINPUT_CONFIG_TAP_ENABLED : LIBINPUT_CONFIG_TAP_DISABLED;

   /* if no seat name is passed in, just use default seat name */
   if (!seat) seat = "seat0";

   EINA_LIST_FOREACH(manager->input.seats, l, eseat)
     {
        if ((eseat->name) && (strcmp(eseat->name, seat)))
          continue;

        EINA_LIST_FOREACH(eseat->devices, ll, edev)
          {
             if (!libinput_device_has_capability(edev->device,
                                                 LIBINPUT_DEVICE_CAP_POINTER))
               continue;

             if (libinput_device_config_tap_set_enabled(edev->device, state)
                 != LIBINPUT_CONFIG_STATUS_SUCCESS)
               {
                  WRN("Failed to %s tap-to-click on device: %s",
                      enabled ? "enable" : "disable",
                      libinput_device_get_name(edev->device));
                  continue;
               }
          }
     }
}

/**
 * @brief Retrieves the Elput_Seat associated with an Elput_Device.
 *
 * @param dev The Elput_Device.
 * @return A pointer to the Elput_Seat the device belongs to, or NULL if
 *         @p dev is NULL.
 */
EAPI Elput_Seat *
elput_device_seat_get(const Elput_Device *dev)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);
   return dev->seat;
}

/**
 * @brief Retrieves the capabilities of an Elput_Device.
 *
 * Capabilities indicate what kind of input the device can produce (e.g.,
 * keyboard, pointer, touch).
 *
 * @param dev The Elput_Device.
 * @return A bitmask of Elput_Device_Caps representing the device's
 *         capabilities, or 0 if @p dev is NULL.
 *         Example: `ELPUT_DEVICE_CAPS_KEYBOARD | ELPUT_DEVICE_CAPS_POINTER`
 */
EAPI Elput_Device_Caps
elput_device_caps_get(const Elput_Device *dev)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, 0);
   return dev->caps;
}

/**
 * @brief Retrieves the output name associated with an Elput_Device.
 *
 * The output name typically refers to the display or screen that the
 * input device is mapped to. This is often used for multi-monitor setups.
 *
 * @param device The Elput_Device.
 * @return An Eina_Stringshare containing the output name, or NULL if
 *         @p device is NULL or no output name is set. The caller should not
 *         free the returned stringshare; its lifetime is managed by elput.
 */
EAPI Eina_Stringshare *
elput_device_output_name_get(Elput_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, NULL);

   return device->output_name;
}

/**
 * @brief Retrieves the list of Elput_Devices associated with an Elput_Seat.
 *
 * @param seat The Elput_Seat.
 * @return A pointer to an Eina_List containing Elput_Device pointers.
 *         The list is owned by elput and should not be modified or freed
 *         by the caller. Returns NULL if @p seat is NULL.
 */
EAPI const Eina_List *
elput_seat_devices_get(const Elput_Seat *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, NULL);
   return seat->devices;
}

/**
 * @brief Retrieves the name of an Elput_Seat.
 *
 * @param seat The Elput_Seat.
 * @return An Eina_Stringshare containing the seat's name (e.g., "seat0").
 *         The stringshare is owned by elput and should not be freed by
 *         the caller. Returns NULL if @p seat is NULL.
 */
EAPI Eina_Stringshare *
elput_seat_name_get(const Elput_Seat *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, NULL);
   return seat->name;
}

/**
 * @brief Retrieves the Elput_Manager associated with an Elput_Seat.
 *
 * @param seat The Elput_Seat.
 * @return A pointer to the Elput_Manager that owns this seat, or NULL if
 *         @p seat is NULL.
 */
EAPI Elput_Manager *
elput_seat_manager_get(const Elput_Seat *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, NULL);
   return seat->manager;
}
