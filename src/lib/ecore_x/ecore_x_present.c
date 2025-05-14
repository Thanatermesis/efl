#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#include "ecore_x_private.h"
#include "Ecore_X.h"

/** @internal
 *  Stores the major opcode for the XPresent extension.
 *  Initialized by _ecore_x_present_init.
 */
int _ecore_x_present_major = 0;

/** @internal
 *  Indicates if the XPresent extension is available.
 *  Set by _ecore_x_present_init.
 */
static Eina_Bool _ecore_x_present_exists = EINA_FALSE;

/**
 * @internal
 * @brief Initializes the Ecore_X Present extension support.
 *
 * This function queries the X server for the presence of the XPresent
 * extension and initializes event types related to it.
 */
void
_ecore_x_present_init(void)
{
   ECORE_X_EVENT_PRESENT_CONFIGURE = ecore_event_type_new();
   ECORE_X_EVENT_PRESENT_COMPLETE = ecore_event_type_new();
   ECORE_X_EVENT_PRESENT_IDLE = ecore_event_type_new();
#ifdef ECORE_XPRESENT
   LOGFN;
   _ecore_x_present_exists = XPresentQueryExtension(_ecore_x_disp, &_ecore_x_present_major, NULL, NULL);
#endif
}

#ifdef ECORE_XPRESENT
#define SET(X) e->X = ev->X

/**
 * @internal
 * @brief Handles XPresentConfigureNotify events.
 *
 * Creates and populates an Ecore_X_Event_Present_Configure event
 * and adds it to the Ecore event queue.
 *
 * @param ev The XPresentConfigureNotifyEvent from the X server.
 */
static void
_present_configure(XPresentConfigureNotifyEvent *ev)
{
   Ecore_X_Event_Present_Configure *e;

   e = calloc(1, sizeof(Ecore_X_Event_Present_Configure));
   if (!e) return;

   e->win = ev->window;
   SET(x), SET(y);
   SET(width), SET(height);
   SET(off_x), SET(off_y);
   SET(pixmap_width), SET(pixmap_height);
   SET(pixmap_flags);

   ecore_event_add(ECORE_X_EVENT_PRESENT_CONFIGURE, e, NULL, NULL);
}

/**
 * @internal
 * @brief Handles XPresentCompleteNotify events.
 *
 * Creates and populates an Ecore_X_Event_Present_Complete event
 * and adds it to the Ecore event queue.
 *
 * @param ev The XPresentCompleteNotifyEvent from the X server.
 */
static void
_present_complete(XPresentCompleteNotifyEvent *ev)
{
   unsigned int mode[] =
   {
    [PresentCompleteModeCopy] = ECORE_X_PRESENT_COMPLETE_MODE_COPY,
    [PresentCompleteModeFlip] = ECORE_X_PRESENT_COMPLETE_MODE_FLIP,
    [PresentCompleteModeSkip] = ECORE_X_PRESENT_COMPLETE_MODE_SKIP,
   };
   Ecore_X_Event_Present_Complete *e;

   e = calloc(1, sizeof(Ecore_X_Event_Present_Complete));
   if (!e) return;

   e->win = ev->window;
   e->serial = ev->serial_number;
   SET(ust), SET(msc);
   e->kind = (ev->kind == 1); //libXpresent doesn't expose this...
   e->mode = mode[ev->mode];
   ecore_event_add(ECORE_X_EVENT_PRESENT_COMPLETE, e, NULL, NULL);
}

/**
 * @internal
 * @brief Handles XPresentIdleNotify events.
 *
 * Creates and populates an Ecore_X_Event_Present_Idle event
 * and adds it to the Ecore event queue.
 *
 * @param ev The XPresentIdleNotifyEvent from the X server.
 */
static void
_present_idle(XPresentIdleNotifyEvent *ev)
{
   Ecore_X_Event_Present_Idle *e;

   e = calloc(1, sizeof(Ecore_X_Event_Present_Idle));
   if (!e) return;

   e->win = ev->window;
   e->serial = ev->serial_number;
   SET(pixmap);
   SET(idle_fence);
   ecore_event_add(ECORE_X_EVENT_PRESENT_IDLE, e, NULL, NULL);
}
#undef SET

/**
 * @internal
 * @brief Generic event handler for XPresent extension events.
 *
 * Dispatches XPresent extension events (ConfigureNotify, CompleteNotify, IdleNotify)
 * to their respective handlers.
 *
 * @param ge The XGenericEvent received from the X server.
 */
void
_ecore_x_present_handler(XGenericEvent *ge)
{
   XGenericEventCookie *gec = (XGenericEventCookie*)ge;

   if (XGetEventData(_ecore_x_disp, gec))
     {
        switch (gec->evtype)
          {
           case PresentConfigureNotify:
             _present_configure(gec->data);
             break;
           case PresentCompleteNotify:
             _present_complete(gec->data);
             break;
           case PresentIdleNotify:
             _present_idle(gec->data);
             break;
           default: break;
          }
     }
   XFreeEventData(_ecore_x_disp, gec);
}
#endif

/**
 * @brief Selects Present events for a window.
 *
 * This function requests the X server to send Present events for the
 * specified window.
 *
 * @param win The window to select events for.
 * @param events A bitmask of Present events to select.
 *               Possible values include:
 *               - @c XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY
 *               - @c XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY
 *               - @c XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY
 *               (Note: These are XCB constants, map to Ecore_X equivalents if available or use raw values)
 *               For example, to select all events:
 *               `XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY | XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY | XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY`
 */
EAPI void
ecore_x_present_select_events(Ecore_X_Window win, unsigned int events)
{
#ifdef ECORE_XPRESENT
   XPresentSelectInput(_ecore_x_disp, win, events);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   (void)win;
   (void)events;
#endif
}

/**
 * @brief Requests a Present CompleteNotify event when a particular MSC (Media Stream Counter) is reached.
 *
 * This function allows synchronization with the display's refresh cycle.
 *
 * @param win The window associated with the Present operation.
 * @param serial The serial number of the Present request to which this notification pertains.
 * @param target_msc The target Media Stream Counter value.
 * @param divisor The divisor for the MSC calculation.
 * @param remainder The remainder for the MSC calculation.
 *                  The event will be generated when `(current_msc % divisor) == remainder`
 *                  and `current_msc >= target_msc`.
 */
EAPI void
ecore_x_present_notify_msc(Ecore_X_Window win, unsigned int serial, unsigned long long target_msc, unsigned long long divisor, unsigned long long remainder)
{
#ifdef ECORE_XPRESENT
   XPresentNotifyMSC(_ecore_x_disp, win, serial, target_msc, divisor, remainder);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   (void)win;
   (void)serial;
   (void)target_msc;
   (void)divisor;
   (void)remainder;
#endif
}

/**
 * @brief Presents a pixmap to a window.
 *
 * This function is the core of the Present extension, allowing an application
 * to request that a pixmap's contents be displayed on a window, typically
 * synchronized with vblank.
 *
 * @param win The target window for presentation.
 * @param pixmap The pixmap to present.
 * @param serial A serial number for this Present request, chosen by the client.
 *               This serial is used in PresentCompleteNotify events.
 * @param valid The region of the pixmap that contains valid data.
 *              An XID which can be 0 (None) if the entire pixmap is valid.
 * @param update The region of the window that needs to be updated.
 *               An XID which can be 0 (None) if the entire window is updated.
 * @param x_off X offset for the update region within the window.
 * @param y_off Y offset for the update region within the window.
 * @param target_crtc The CRTC (Ecore_X_Randr_Crtc which is an XID) to synchronize with.
 *                    Can be 0 (None).
 * @param wait_fence A sync fence (Ecore_X_Sync_Fence which is an XID) to wait for
 *                   before executing the Present operation. Can be 0 (None).
 * @param idle_fence A sync fence (Ecore_X_Sync_Fence which is an XID) that will be
 *                   signaled when the Present operation is complete and the pixmap
 *                   is idle. Can be 0 (None).
 * @param options A bitmask of Present options.
 *                Example: @c PresentOptionNone, @c PresentOptionAsync.
 *                (Note: These are Xlib constants. Refer to X11/extensions/Xpresent.h)
 * @param target_msc The target Media Stream Counter for presentation.
 * @param divisor Divisor for MSC-based presentation timing.
 * @param remainder Remainder for MSC-based presentation timing.
 * @param notifies An array of Ecore_X_Present_Notify structures (which should map to XPresentNotify).
 *                 Each structure specifies a window and a serial number for
 *                 which a PresentComplete event should be generated upon completion
 *                 of this Present operation on *that* window/serial.
 *                 This is used for chaining or synchronizing presentations.
 *                 Example:
 *                 @code
 *                 Ecore_X_Present_Notify my_notifies[1];
 *                 my_notifies[0].win = another_window_to_be_notified; // Ecore_X_Window
 *                 my_notifies[0].serial = serial_of_present_on_another_window; // unsigned int
 *                 // ... call ecore_x_present_pixmap with &my_notifies, 1 ...
 *                 @endcode
 *                 If no such notifications are needed, pass NULL for @p notifies and 0 for @p num_notifies.
 * @param num_notifies The number of elements in the @p notifies array.
 */
EAPI void
ecore_x_present_pixmap(Ecore_X_Window win, Ecore_X_Pixmap pixmap, unsigned int serial, Ecore_X_Region valid,
                       Ecore_X_Region update, int x_off, int y_off, Ecore_X_Randr_Crtc target_crtc,
                       Ecore_X_Sync_Fence wait_fence, Ecore_X_Sync_Fence idle_fence, unsigned int options,
                       unsigned long long target_msc, unsigned long long divisor, unsigned long long remainder,
                       Ecore_X_Present *notifies, int num_notifies)
{
#ifdef ECORE_XPRESENT
   XPresentPixmap(_ecore_x_disp, win, pixmap, serial, valid, update,
                  x_off, y_off, target_crtc, wait_fence, idle_fence, options, target_msc,
                  divisor, remainder, (XPresentNotify*)notifies, num_notifies);
   if (_ecore_xlib_sync) ecore_x_sync();
#else
   (void)win;
   (void)pixmap;
   (void)serial;
   (void)valid;
   (void)update,
   (void)x_off;
   (void)y_off;
   (void)target_crtc;
   (void)wait_fence;
   (void)idle_fence;
   (void)options;
   (void)target_msc,
   (void)divisor;
   (void)remainder;
   (void)notifies;
   (void)num_notifies;
#endif
}

/**
 * @brief Checks if the XPresent extension is available on the X server.
 *
 * @return @c EINA_TRUE if the XPresent extension is available,
 *         @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_present_exists(void)
{
   return _ecore_x_present_exists;
}
