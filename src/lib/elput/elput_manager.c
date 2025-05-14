#include "elput_private.h"

static Elput_Interface *_ifaces[] =
{
#ifdef HAVE_SYSTEMD
   &_logind_interface,
#endif
   &_root_interface,
   NULL,
};

/**
 * @internal
 * @brief Callback function for key down events.
 *
 * This function is triggered when a key is pressed. It checks for
 * Ctrl+Alt+F[1-8] key combinations to initiate a virtual terminal (VT) switch.
 *
 * @param data The Elput_Manager instance.
 * @param type The type of the event (unused).
 * @param event The Ecore_Event_Key event data.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_cb_key_down(void *data, int type EINA_UNUSED, void *event)
{
   Elput_Manager *em;
   Ecore_Event_Key *ev;
   int code = 0, vt = 0;

   em = data;
   ev = event;
   code = (ev->keycode - 8);

   if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) &&
       (code >= KEY_F1) && (code <= KEY_F8))
     {
        vt = (code - KEY_F1 + 1);
        if (em->interface->vt_set)
          {
             if (!em->interface->vt_set(em, vt))
               ERR("Failed to switch to virtual terminal %d", vt);
          }
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Connects to an elput manager.
 *
 * This function attempts to connect to an elput manager using the available
 * interfaces. It iterates through the registered interfaces and tries to
 * establish a connection.
 *
 * @param seat The seat identifier (e.g., "seat0"). Can be NULL to use default.
 * @param tty The TTY number to use. If 0, it may be auto-detected or unused
 *            depending on the interface.
 * @return A pointer to the Elput_Manager instance on success, or NULL on failure.
 *
 * @see elput_manager_disconnect()
 */
EAPI Elput_Manager *
elput_manager_connect(const char *seat, unsigned int tty)
{
   Elput_Interface **it;

   for (it = _ifaces; *it != NULL; it++)
     {
        Elput_Interface *iface;
        Elput_Manager *em;

        iface = *it;
        if (iface->connect(&em, seat, tty))
          return em;
     }

   return NULL;
}

/**
 * @brief Disconnects from an elput manager.
 *
 * This function disconnects from the specified elput manager and releases
 * associated resources. If an input thread is running, it will be cancelled.
 *
 * @param manager The Elput_Manager instance to disconnect.
 *
 * @see elput_manager_connect()
 */
EAPI void
elput_manager_disconnect(Elput_Manager *manager)
{
   EINA_SAFETY_ON_NULL_RETURN(manager);
   EINA_SAFETY_ON_NULL_RETURN(manager->interface);

   if (manager->input.thread)
     {
        ecore_thread_cancel(manager->input.thread);
        manager->del = 1;
        return;
     }

   if (manager->interface->disconnect)
     manager->interface->disconnect(manager);
}

/**
 * @brief Opens a device node through the elput manager.
 *
 * This function requests the underlying interface to open a device specified
 * by its path. If successful and the opened device is a TTY, it sets up
 * a key event handler for VT switching.
 *
 * @param manager The Elput_Manager instance.
 * @param path The path to the device node to open (e.g., "/dev/input/event0").
 * @param flags The flags to use when opening the device (e.g., O_RDWR | O_NONBLOCK).
 *              If flags < 0, O_RDWR will be used by default.
 * @return The file descriptor of the opened device on success, or -1 on failure.
 *
 * @see elput_manager_close()
 */
EAPI int
elput_manager_open(Elput_Manager *manager, const char *path, int flags)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(manager->interface, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, -1);

   if (flags < 0) flags = O_RDWR;

   if (manager->interface->open)
     {
        ret = manager->interface->open(manager, path, flags);
        if (ret)
          {
             manager->vt_hdlr =
               ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                       _cb_key_down, manager);
             manager->vt_fd = ret;
          }
     }

   return ret;
}

/**
 * @brief Closes a device node previously opened by the elput manager.
 *
 * This function requests the underlying interface to close a device specified
 * by its file descriptor. If the closed fd was used for VT switching,
 * the associated event handler is removed.
 *
 * @param manager The Elput_Manager instance.
 * @param fd The file descriptor of the device to close.
 *
 * @see elput_manager_open()
 */
EAPI void
elput_manager_close(Elput_Manager *manager, int fd)
{
   EINA_SAFETY_ON_NULL_RETURN(manager);
   EINA_SAFETY_ON_NULL_RETURN(manager->interface);

   if (fd == manager->vt_fd)
     {
        if (manager->vt_hdlr) ecore_event_handler_del(manager->vt_hdlr);
        manager->vt_hdlr = NULL;
     }

   if (manager->interface->close)
     manager->interface->close(manager, fd);
}

/**
 * @brief Sets the active virtual terminal (VT).
 *
 * This function requests the underlying interface to switch to the specified
 * virtual terminal.
 *
 * @param manager The Elput_Manager instance.
 * @param vt The virtual terminal number to switch to (e.g., 1 for VT1, 2 for VT2).
 *           Must be a non-negative value.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if the operation
 *         is not supported by the current interface.
 */
EAPI Eina_Bool
elput_manager_vt_set(Elput_Manager *manager, int vt)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(manager->interface, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((vt < 0), EINA_FALSE);

   if (manager->interface->vt_set)
     return manager->interface->vt_set(manager, vt);

   return EINA_FALSE;
}

/**
 * @brief Sets the window context for input events.
 *
 * This function informs the elput manager about the window that should
 * receive input events. The interpretation of 'window' depends on the
 * graphics system in use (e.g., X11 Window ID, Wayland surface ID).
 *
 * @param manager The Elput_Manager instance.
 * @param window The identifier of the target window.
 */
EAPI void
elput_manager_window_set(Elput_Manager *manager, unsigned int window)
{
   EINA_SAFETY_ON_NULL_RETURN(manager);

   manager->window = window;
}

/**
 * @brief Gets the list of available input seats.
 *
 * This function retrieves the list of seats currently managed or detected
 * by the elput manager. Each element in the list typically represents
 * a collection of input devices (keyboard, mouse, touch, etc.) that
 * belong to a single user. The exact structure of the seat data
 * depends on the underlying elput implementation and interfaces.
 *
 * @note The returned list is owned by the Elput_Manager and should not be
 *       modified or freed by the caller. It is valid as long as the
 *       Elput_Manager instance is valid and has not been reconfigured.
 *
 * @param manager The Elput_Manager instance.
 * @return A const Eina_List pointer containing seat information, or NULL
 *         if no seats are available or an error occurred.
 *         Example of list elements (conceptual, actual structure may vary):
 *         - Seat 1: { name: "seat0", devices: ["/dev/input/event0", "/dev/input/mouse1"] }
 *         - Seat 2: { name: "seat1", devices: ["/dev/input/event2"] }
 */
EAPI const Eina_List *
elput_manager_seats_get(Elput_Manager *manager)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(manager, NULL);
   return manager->input.seats;
}
