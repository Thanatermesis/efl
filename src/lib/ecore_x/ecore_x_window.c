#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

static int ignore_num = 0;
static Ecore_X_Window *ignore_list = NULL;

/**
 * @defgroup Ecore_X_Window_Create_Group X Window Creation Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that can be used to create an X window.
 */

/**
 * Creates a new X window with specified visual, colormap, depth, and override_redirect.
 *
 * This function provides more control over window creation than ecore_x_window_new().
 *
 * @param   parent   The parent window. If 0, the default root window is used.
 * @param   x        The X coordinate of the new window.
 * @param   y        The Y coordinate of the new window.
 * @param   w        The width of the new window.
 * @param   h        The height of the new window.
 * @param   visual   The visual to use for the window.
 * @param   colormap The colormap to use for the window.
 * @param   depth    The depth of the window.
 * @param   override If EINA_TRUE, the window manager will not manage this window (override_redirect).
 * @return  The ID of the newly created window, or 0 on failure.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_full_new(Ecore_X_Window parent,
                        int x,
                        int y,
                        int w,
                        int h,
                        Ecore_X_Visual *visual,
                        Ecore_X_Colormap colormap,
                        int depth,
                        Eina_Bool override)
{
   Window win;
   XSetWindowAttributes attr;

   LOGFN;
   if (parent == 0)
     parent = DefaultRootWindow(_ecore_x_disp);

   attr.backing_store = NotUseful;
   attr.override_redirect = override;
   attr.border_pixel = 0;
   attr.background_pixmap = None;
   attr.bit_gravity = NorthWestGravity;
   attr.win_gravity = NorthWestGravity;
   attr.save_under = False;
   attr.do_not_propagate_mask = NoEventMask;
   attr.colormap = (Colormap)colormap;
   attr.event_mask = KeyPressMask |
     KeyReleaseMask |
     ButtonPressMask |
     ButtonReleaseMask |
     EnterWindowMask |
     LeaveWindowMask |
     PointerMotionMask |
     ExposureMask |
     VisibilityChangeMask |
     StructureNotifyMask |
     FocusChangeMask |
     PropertyChangeMask |
     ColormapChangeMask;
   win = XCreateWindow(_ecore_x_disp, parent,
                       x, y, w, h, 0,
                       depth,
                       InputOutput,
                       (Visual *)visual,
                       CWBackingStore |
                       CWOverrideRedirect |
                       (colormap ? CWColormap : 0) |
                       CWBorderPixel |
                       CWBackPixmap |
                       CWSaveUnder |
                       CWDontPropagate |
                       CWEventMask |
                       CWBitGravity |
                       CWWinGravity,
                       &attr);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (parent == DefaultRootWindow(_ecore_x_disp))
     ecore_x_window_defaults_set(win);

   return win;
}

/**
 * Creates a new window.
 * @param   parent The parent window to use.  If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window handle.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_new(Ecore_X_Window parent,
                   int x,
                   int y,
                   int w,
                   int h)
{
   Window win;
   XSetWindowAttributes attr;

   LOGFN;
   if (parent == 0)
     parent = DefaultRootWindow(_ecore_x_disp);

   attr.backing_store = NotUseful;
   attr.override_redirect = False;
   attr.border_pixel = 0;
   attr.background_pixmap = None;
   attr.bit_gravity = NorthWestGravity;
   attr.win_gravity = NorthWestGravity;
   attr.save_under = False;
   attr.do_not_propagate_mask = NoEventMask;
   attr.event_mask = KeyPressMask |
     KeyReleaseMask |
     ButtonPressMask |
     ButtonReleaseMask |
     EnterWindowMask |
     LeaveWindowMask |
     PointerMotionMask |
     ExposureMask |
     VisibilityChangeMask |
     StructureNotifyMask |
     FocusChangeMask |
     PropertyChangeMask |
     ColormapChangeMask;
   win = XCreateWindow(_ecore_x_disp, parent,
                       x, y, w, h, 0,
                       CopyFromParent, /*DefaultDepth(_ecore_x_disp, DefaultScreen(_ecore_x_disp)),*/
                       InputOutput,
                       CopyFromParent, /*DefaultVisual(_ecore_x_disp, DefaultScreen(_ecore_x_disp)),*/
                       CWBackingStore |
                       CWOverrideRedirect |
/*		       CWColormap | */
                       CWBorderPixel |
                       CWBackPixmap |
                       CWSaveUnder |
                       CWDontPropagate |
                       CWEventMask |
                       CWBitGravity |
                       CWWinGravity,
                       &attr);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (parent == DefaultRootWindow(_ecore_x_disp))
     ecore_x_window_defaults_set(win);

   return win;
}

/**
 * Creates a window with the override redirect attribute set to @c True.
 * @param   parent The parent window to use.  If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window handle.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_override_new(Ecore_X_Window parent,
                            int x,
                            int y,
                            int w,
                            int h)
{
   Window win;
   XSetWindowAttributes attr;

   LOGFN;
   if (parent == 0)
     parent = DefaultRootWindow(_ecore_x_disp);

   attr.backing_store = NotUseful;
   attr.override_redirect = True;
   attr.border_pixel = 0;
   attr.background_pixmap = None;
   attr.bit_gravity = NorthWestGravity;
   attr.win_gravity = NorthWestGravity;
   attr.save_under = False;
   attr.do_not_propagate_mask = NoEventMask;
   attr.event_mask = KeyPressMask |
     KeyReleaseMask |
     ButtonPressMask |
     ButtonReleaseMask |
     EnterWindowMask |
     LeaveWindowMask |
     PointerMotionMask |
     ExposureMask |
     VisibilityChangeMask |
     StructureNotifyMask |
     FocusChangeMask |
     PropertyChangeMask |
     ColormapChangeMask;
   win = XCreateWindow(_ecore_x_disp, parent,
                       x, y, w, h, 0,
                       CopyFromParent, /*DefaultDepth(_ecore_x_disp, DefaultScreen(_ecore_x_disp)),*/
                       InputOutput,
                       CopyFromParent, /*DefaultVisual(_ecore_x_disp, DefaultScreen(_ecore_x_disp)),*/
                       CWBackingStore |
                       CWOverrideRedirect |
/*		       CWColormap | */
                       CWBorderPixel |
                       CWBackPixmap |
                       CWSaveUnder |
                       CWDontPropagate |
                       CWEventMask |
                       CWBitGravity |
                       CWWinGravity,
                       &attr);
   if (_ecore_xlib_sync) ecore_x_sync();
   return win;
}

/**
 * Creates a new input window.
 * @param   parent The parent window to use.    If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_input_new(Ecore_X_Window parent,
                         int x,
                         int y,
                         int w,
                         int h)
{
   Window win;
   XSetWindowAttributes attr;

   LOGFN;
   if (parent == 0)
     parent = DefaultRootWindow(_ecore_x_disp);

   attr.override_redirect = True;
   attr.do_not_propagate_mask = NoEventMask;
   attr.event_mask = KeyPressMask |
     KeyReleaseMask |
     ButtonPressMask |
     ButtonReleaseMask |
     EnterWindowMask |
     LeaveWindowMask |
     PointerMotionMask |
     ExposureMask |
     VisibilityChangeMask |
     StructureNotifyMask |
     FocusChangeMask |
     PropertyChangeMask |
     ColormapChangeMask;
   win = XCreateWindow(_ecore_x_disp, parent,
                       x, y, w, h, 0,
                       CopyFromParent,
                       InputOnly,
                       CopyFromParent, /*DefaultVisual(_ecore_x_disp, DefaultScreen(_ecore_x_disp)),*/
                       CWOverrideRedirect |
                       CWDontPropagate |
                       CWEventMask,
                       &attr);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (parent == DefaultRootWindow(_ecore_x_disp))
     {
     }

   return win;
}

/**
 * @defgroup Ecore_X_Window_Properties_Group X Window Property Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that set window properties.
 */

/**
 * Sets the default properties for the given window.
 *
 * The default properties set for the window are @c WM_CLIENT_MACHINE and
 * @c _NET_WM_PID.
 *
 * @param   win The given window.
 * @ingroup Ecore_X_Window_Properties_Group
 */
EAPI void
ecore_x_window_defaults_set(Ecore_X_Window win)
{
   long pid;
   char buf[MAXHOSTNAMELEN];
   char *hostname[1];
   int argc;
   char **argv;
   XTextProperty xprop;

   LOGFN;
   /*
    * Set WM_CLIENT_MACHINE.
    */
   gethostname(buf, MAXHOSTNAMELEN);
   buf[MAXHOSTNAMELEN - 1] = '\0';
   hostname[0] = buf;
   /* The ecore function uses UTF8 which Xlib may not like (especially
    * with older clients) */
   /* ecore_x_window_prop_string_set(win, ECORE_X_ATOM_WM_CLIENT_MACHINE,
                                  (char *)buf); */
   if (XStringListToTextProperty(hostname, 1, &xprop))
     {
        XSetWMClientMachine(_ecore_x_disp, win, &xprop);
        XFree(xprop.value);
        if (_ecore_xlib_sync) ecore_x_sync();
     }
   if (_ecore_xlib_sync) ecore_x_sync();
   /*
    * Set _NET_WM_PID
    */
   pid = getpid();
   ecore_x_netwm_pid_set(win, pid);

   ecore_x_netwm_window_type_set(win, ECORE_X_WINDOW_TYPE_NORMAL);

   ecore_app_args_get(&argc, &argv);
   ecore_x_icccm_command_set(win, argc, argv);
}

/**
 * Configures an X window's geometry and stacking order.
 *
 * This function allows changing various attributes of a window, such as its
 * position, size, border width, and stacking relative to sibling windows.
 * The @p mask parameter specifies which attributes to change.
 *
 * @param win          The window to configure.
 * @param mask         A bitmask indicating which fields in the XWindowChanges
 *                     structure are to be modified. For example, (CWX | CWY)
 *                     to change x and y coordinates.
 * @param x            The new X coordinate of the window.
 * @param y            The new Y coordinate of the window.
 * @param w            The new width of the window.
 * @param h            The new height of the window.
 * @param border_width The new border width of the window.
 * @param sibling      A sibling window for stacking operations.
 * @param stack_mode   Specifies how the window is stacked relative to the sibling.
 *                     Possible values include: Above, Below, TopIf, BottomIf, Opposite.
 */
EAPI void
ecore_x_window_configure(Ecore_X_Window win,
                         Ecore_X_Window_Configure_Mask mask,
                         int x,
                         int y,
                         int w,
                         int h,
                         int border_width,
                         Ecore_X_Window sibling,
                         int stack_mode)
{
   XWindowChanges xwc;

   if (!win)
     return;

   LOGFN;

   xwc.x = x;
   xwc.y = y;
   xwc.width = w;
   xwc.height = h;
   xwc.border_width = border_width;
   xwc.sibling = sibling;
   xwc.stack_mode = stack_mode;

   XConfigureWindow(_ecore_x_disp, win, mask, &xwc);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @defgroup Ecore_X_Window_Destroy_Group X Window Destroy Functions
 * @ingroup Ecore_X_Group
 *
 * Functions to destroy X windows.
 */

/**
 * Deletes the given window.
 * @param   win The given window.
 * @ingroup Ecore_X_Window_Destroy_Group
 */
EAPI void
ecore_x_window_free(Ecore_X_Window win)
{
   /* sorry sir, deleting the root window doesn't sound like
    * a smart idea.
    */
   LOGFN;
   if (!win) return;
   XDestroyWindow(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Adds or removes a window from the internal list of windows to be ignored by certain operations.
 *
 * This is typically used to prevent Ecore_X from processing events for specific
 * windows, often utility windows or windows managed by other parts of an
 * application.
 *
 * @param win    The window to add to or remove from the ignore list.
 * @param ignore If non-zero, the window is added to the ignore list.
 *               If zero, the window is removed from the ignore list.
 */
EAPI void
ecore_x_window_ignore_set(Ecore_X_Window win,
                          int ignore)
{
   int i, j, cnt;
   Ecore_X_Window *t;

   LOGFN;
   if (ignore)
     {
        if (ignore_list)
          {
             for (i = 0; i < ignore_num; i++)
               {
                  if (win == ignore_list[i])
                    return;
               }
             t = realloc(ignore_list, (ignore_num + 1) * sizeof(Ecore_X_Window));
             if (!t) return;
             ignore_list = t;
             ignore_list[ignore_num++] = win;
          }
        else
          {
             ignore_num = 0;
             ignore_list = malloc(sizeof(Ecore_X_Window));
             if (ignore_list)
               ignore_list[ignore_num++] = win;
          }
     }
   else
     {
        if (!ignore_list)
          return;

        for (cnt = ignore_num, i = 0, j = 0; i < cnt; i++)
          {
             if (win != ignore_list[i])
               ignore_list[j++] = ignore_list[i];
             else
               ignore_num--;
          }

        if (ignore_num <= 0)
          {
             free(ignore_list);
             ignore_list = NULL;
             return;
          }
        t = realloc(ignore_list, ignore_num * sizeof(Ecore_X_Window));
        if (t) ignore_list = t;
     }
}

/**
 * Retrieves the list of windows currently marked as ignored.
 *
 * The returned list is a direct pointer to the internal storage and should not
 * be modified or freed by the caller. The number of windows in the list is
 * returned via the @p num parameter.
 *
 * @param num Pointer to an integer where the number of ignored windows will be stored.
 *            Can be NULL if the count is not needed.
 * @return    A pointer to an array of Ecore_X_Window IDs.
 *            The array contains the windows that are currently ignored.
 *            Returns NULL if the ignore list is empty or not initialized.
 *            Example of returned array structure:
 *            If `ignore_list` contains `[win1, win2, win3]`, then `*num` will be 3,
 *            and the return value will point to this array.
 */
EAPI Ecore_X_Window *
ecore_x_window_ignore_list(int *num)
{
   if (num)
     *num = ignore_num;

   return ignore_list;
}

/**
 * Sends a delete request to the given window.
 * @param   win The given window.
 * @ingroup Ecore_X_Window_Destroy_Group
 */
EAPI void
ecore_x_window_delete_request_send(Ecore_X_Window win)
{
   XEvent xev = { 0 };

   /* sorry sir, deleting the root window doesn't sound like
    * a smart idea.
    */
   if (!win)
     return;

   LOGFN;
   xev.xclient.type = ClientMessage;
   xev.xclient.display = _ecore_x_disp;
   xev.xclient.window = win;
   xev.xclient.message_type = ECORE_X_ATOM_WM_PROTOCOLS;
   xev.xclient.format = 32;
   xev.xclient.data.l[0] = ECORE_X_ATOM_WM_DELETE_WINDOW;
   xev.xclient.data.l[1] = CurrentTime;

   XSendEvent(_ecore_x_disp, win, False, NoEventMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @defgroup Ecore_X_Window_Visibility_Group X Window Visibility Functions
 * @ingroup Ecore_X_Group
 *
 * Functions to access and change the visibility of X windows.
 */

/**
 * Shows a window.
 *
 * Synonymous to "mapping" a window in X Window System terminology.
 *
 * @param   win The window to show.
 * @ingroup Ecore_X_Window_Visibility
 */
EAPI void
ecore_x_window_show(Ecore_X_Window win)
{
   LOGFN;
   XMapWindow(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Hides a window.
 *
 * Synonymous to "unmapping" a window in X Window System terminology.
 *
 * @param   win The window to hide.
 * @ingroup Ecore_X_Window_Visibility
 */
EAPI void
ecore_x_window_hide(Ecore_X_Window win)
{
   XEvent xev = { 0 };
   Window root;
   int idum;
   unsigned int uidum;

   /* ICCCM: SEND unmap event... */
   LOGFN;
   root = win;
   if (ScreenCount(_ecore_x_disp) == 1)
     root = DefaultRootWindow(_ecore_x_disp);
   else
     XGetGeometry(_ecore_x_disp,
                  win,
                  &root,
                  &idum,
                  &idum,
                  &uidum,
                  &uidum,
                  &uidum,
                  &uidum);

   XUnmapWindow(_ecore_x_disp, win);
   xev.xunmap.type = UnmapNotify;
   xev.xunmap.serial = 0;
   xev.xunmap.send_event = True;
   xev.xunmap.display = _ecore_x_disp;
   xev.xunmap.event = root;
   xev.xunmap.window = win;
   xev.xunmap.from_configure = False;
   XSendEvent(_ecore_x_disp, xev.xunmap.event, False,
              SubstructureRedirectMask | SubstructureNotifyMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @defgroup Ecore_X_Window_Geometry_Group X Window Geometry Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that change or retrieve the geometry of X windows.
 */

/**
 * Moves a window to the position @p x, @p y.
 *
 * The position is relative to the upper left hand corner of the
 * parent window.
 *
 * @param   win The window to move.
 * @param   x   X position.
 * @param   y   Y position.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_move(Ecore_X_Window win,
                    int x,
                    int y)
{
   LOGFN;
   XMoveWindow(_ecore_x_disp, win, x, y);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Resizes a window.
 * @param   win The window to resize.
 * @param   w   New width of the window.
 * @param   h   New height of the window.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_resize(Ecore_X_Window win,
                      int w,
                      int h)
{
   LOGFN;
   if (w < 1)
     w = 1;

   if (h < 1)
     h = 1;

   XResizeWindow(_ecore_x_disp, win, w, h);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Moves and resizes a window.
 * @param   win The window to move and resize.
 * @param   x   New X position of the window.
 * @param   y   New Y position of the window.
 * @param   w   New width of the window.
 * @param   h   New height of the window.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_move_resize(Ecore_X_Window win,
                           int x,
                           int y,
                           int w,
                           int h)
{
   LOGFN;
   if (w < 1)
     w = 1;

   if (h < 1)
     h = 1;

   XMoveResizeWindow(_ecore_x_disp, win, x, y, w, h);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @defgroup Ecore_X_Window_Focus_Functions X Window Focus Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that give the focus to an X Window.
 */

/**
 * Sets the focus to the window @p win.
 * @param   win The window to focus.
 * @ingroup Ecore_X_Window_Focus_Functions
 */
EAPI void
ecore_x_window_focus(Ecore_X_Window win)
{
   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);  //   XSetInputFocus(_ecore_x_disp, win, RevertToNone, CurrentTime);

//   XSetInputFocus(_ecore_x_disp, win, RevertToPointerRoot, CurrentTime);
   XSetInputFocus(_ecore_x_disp, win, RevertToParent, CurrentTime);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the focus to the given window at a specific time.
 * @param   win The window to focus.
 * @param   t   When to set the focus to the window.
 * @ingroup Ecore_X_Window_Focus_Functions
 */
EAPI void
ecore_x_window_focus_at_time(Ecore_X_Window win,
                             Ecore_X_Time t)
{
   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);  //   XSetInputFocus(_ecore_x_disp, win, RevertToNone, t);

//   XSetInputFocus(_ecore_x_disp, win, PointerRoot, t);
   XSetInputFocus(_ecore_x_disp, win, RevertToParent, t);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * gets the window that has focus.
 * @return  The window that has focus.
 * @ingroup Ecore_X_Window_Focus_Functions
 */
EAPI Ecore_X_Window
ecore_x_window_focus_get(void)
{
   Window win;
   int revert_mode;

   LOGFN;
   win = 0;
   XGetInputFocus(_ecore_x_disp, &win, &revert_mode);
   return win;
}

/**
 * @defgroup Ecore_X_Window_Z_Order_Group X Window Z Order Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that change the Z order of X windows.
 */

/**
 * Raises the given window.
 * @param   win The window to raise.
 * @ingroup Ecore_X_Window_Z_Order_Group
 */
EAPI void
ecore_x_window_raise(Ecore_X_Window win)
{
   LOGFN;
   XRaiseWindow(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Lowers the given window.
 * @param   win The window to lower.
 * @ingroup Ecore_X_Window_Z_Order_Group
 */
EAPI void
ecore_x_window_lower(Ecore_X_Window win)
{
   LOGFN;
   XLowerWindow(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @defgroup Ecore_X_Window_Parent_Group X Window Parent Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that retrieve or changes the parent window of a window.
 */

/**
 * Moves a window to within another window at a given position.
 * @param   win        The window to reparent.
 * @param   new_parent The new parent window.
 * @param   x          X position within new parent window.
 * @param   y          Y position within new parent window.
 * @ingroup Ecore_X_Window_Parent_Group
 */
EAPI void
ecore_x_window_reparent(Ecore_X_Window win,
                        Ecore_X_Window new_parent,
                        int x,
                        int y)
{
   LOGFN;
   if (new_parent == 0)
     new_parent = DefaultRootWindow(_ecore_x_disp);

   XReparentWindow(_ecore_x_disp, win, new_parent, x, y);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Retrieves the size of the given window.
 * @param   win The given window.
 * @param   w   Pointer to an integer into which the width is to be stored.
 * @param   h   Pointer to an integer into which the height is to be stored.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_size_get(Ecore_X_Window win,
                        int *w,
                        int *h)
{
   int dummy_x, dummy_y;

   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);

   ecore_x_drawable_geometry_get(win, &dummy_x, &dummy_y, w, h);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Retrieves the geometry of the given window.
 *
 * Note that the x & y coordinates are relative to your parent.  In
 * particular for reparenting window managers - relative to you window border.
 * If you want screen coordinates either walk the window tree to the root,
 * else for ecore_evas applications see ecore_evas_geometry_get().  Elementary
 * applications can use elm_win_screen_position_get().
 *
 * @param   win The given window.
 * @param   x   Pointer to an integer in which the X position is to be stored.
 * @param   y   Pointer to an integer in which the Y position is to be stored.
 * @param   w   Pointer to an integer in which the width is to be stored.
 * @param   h   Pointer to an integer in which the height is to be stored.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_geometry_get(Ecore_X_Window win,
                            int *x,
                            int *y,
                            int *w,
                            int *h)
{
   LOGFN;
   if (!win)
     win = DefaultRootWindow(_ecore_x_disp);

   ecore_x_drawable_geometry_get(win, x, y, w, h);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Retrieves the width of the border of the given window.
 * @param   win The given window.
 * @return  Width of the border of @p win.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI int
ecore_x_window_border_width_get(Ecore_X_Window win)
{
   int w;
   LOGFN;
   /* doesn't make sense to call this on a root window */
   if (!win)
     return 0;

   w = ecore_x_drawable_border_width_get(win);
   if (_ecore_xlib_sync) ecore_x_sync();
   return w;
}

/**
 * Sets the width of the border of the given window.
 * @param   win The given window.
 * @param   width The new border width.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_border_width_set(Ecore_X_Window win,
                                int width)
{
   LOGFN;
   /* doesn't make sense to call this on a root window */
   if (!win)
     return;

   XSetWindowBorderWidth (_ecore_x_disp, win, width);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Retrieves the depth of the given window.
 * @param  win The given window.
 * @return Depth of the window.
 */
EAPI int
ecore_x_window_depth_get(Ecore_X_Window win)
{
   int d;
   LOGFN;
   d = ecore_x_drawable_depth_get(win);
   if (_ecore_xlib_sync) ecore_x_sync();
   return d;
}

/**
 * @brief Show the cursor on a window of type Ecore_X_Window.
 * @param win The window for which the cursor will be showed.
 * @param show Enables the show of the cursor on the window if equals EINA_TRUE, disables if equals EINA_FALSE.
 */
EAPI void
ecore_x_window_cursor_show(Ecore_X_Window win,
                           Eina_Bool show)
{
   LOGFN;
   if (win == 0)
     win = DefaultRootWindow(_ecore_x_disp);

   if (!show)
     {
        Cursor c;
        XColor cl;
        Pixmap p, m;
        GC gc;
        XGCValues gcv;

        p = XCreatePixmap(_ecore_x_disp, win, 1, 1, 1);
        if (_ecore_xlib_sync) ecore_x_sync();
        m = XCreatePixmap(_ecore_x_disp, win, 1, 1, 1);
        if (_ecore_xlib_sync) ecore_x_sync();
        gc = XCreateGC(_ecore_x_disp, m, 0, &gcv);
        if (_ecore_xlib_sync) ecore_x_sync();
        XSetForeground(_ecore_x_disp, gc, 0);
        if (_ecore_xlib_sync) ecore_x_sync();
        XDrawPoint(_ecore_x_disp, m, gc, 0, 0);
        if (_ecore_xlib_sync) ecore_x_sync();
        XFreeGC(_ecore_x_disp, gc);
        c = XCreatePixmapCursor(_ecore_x_disp, p, m, &cl, &cl, 0, 0);
        if (_ecore_xlib_sync) ecore_x_sync();
        XDefineCursor(_ecore_x_disp, win, c);
        XFreeCursor(_ecore_x_disp, c);
        XFreePixmap(_ecore_x_disp, p);
        XFreePixmap(_ecore_x_disp, m);
}
   else
     XDefineCursor(_ecore_x_disp, win, 0);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the cursor for a given window.
 *
 * This function changes the mouse pointer's appearance when it is over the
 * specified window.
 *
 * @param win The window for which to set the cursor.
 * @param c   The cursor to set. If 0 (None), the cursor is undefined
 *            (usually inheriting from the parent or root window).
 *            This can be a standard X cursor or a custom cursor created
 *            with ecore_x_cursor_new().
 */
EAPI void
ecore_x_window_cursor_set(Ecore_X_Window win,
                          Ecore_X_Cursor c)
{
#ifdef ECORE_XI2
   int devid;
#endif

   LOGFN;
#ifdef ECORE_XI2
   XIGetClientPointer(_ecore_x_disp, None, &devid);
   if (c == 0)
     XIUndefineCursor(_ecore_x_disp, devid, win);
   else
     XIDefineCursor(_ecore_x_disp, devid, win, c);
#else
   if (c == 0)
     XUndefineCursor(_ecore_x_disp, win);
   else
     XDefineCursor(_ecore_x_disp, win, c);
#endif
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Finds out whether the given window is currently visible.
 * @param   win The given window.
 * @return  1 if the window is visible, otherwise 0.
 * @ingroup Ecore_X_Window_Visibility_Group
 */
EAPI int
ecore_x_window_visible_get(Ecore_X_Window win)
{
   Eina_Bool ret;
   XWindowAttributes attr;

   LOGFN;
   ret = (XGetWindowAttributes(_ecore_x_disp, win, &attr) &&
          (attr.map_state == IsViewable));
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

typedef struct _Shadow Shadow;
struct _Shadow
{
   Shadow        *parent;
   Shadow       **children;
   Window         win;
   int            children_num;
   short          x, y;
   unsigned short w, h;
};

static Shadow **shadow_base = NULL;
static int shadow_num = 0;

/**
 * @internal
 * Recursively walks the X window tree starting from @p win and builds a
 * shadow tree representation.
 *
 * This function queries the X server for window attributes and children,
 * creating a tree of Shadow structures. It only includes mapped (visible)
 * windows in the shadow tree.
 *
 * @param  win The window to start walking from.
 * @return A pointer to the Shadow structure representing @p win and its
 *         descendants, or NULL if @p win is not mapped or an error occurs.
 */
static Shadow *
_ecore_x_window_tree_walk(Window win)
{
   Window *list = NULL;
   Window parent_win = 0, root_win = 0;
   unsigned int num;
   Shadow *s, **sl;
   XWindowAttributes att;
   Eina_Bool ret;

   ret = (XGetWindowAttributes(_ecore_x_disp, win, &att) && (att.map_state == IsViewable));
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!ret) return NULL; //   if (att.class == InputOnly) return NULL;

   s = calloc(1, sizeof(Shadow));
   if (!s)
     return NULL;

   s->win = win;
   s->x = att.x;
   s->y = att.y;
   s->w = att.width;
   s->h = att.height;
   if (XQueryTree(_ecore_x_disp, s->win, &root_win, &parent_win,
                  &list, &num))
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        s->children = calloc(1, sizeof(Shadow *) * num);
        if (s->children)
          {
             size_t i, j;
             s->children_num = num;
             for (i = 0; i < num; i++)
               {
                  s->children[i] = _ecore_x_window_tree_walk(list[i]);
                  if (s->children[i])
                    s->children[i]->parent = s;
               }
             /* compress list down */
             j = 0;
             for (i = 0; i < num; i++)
               {
                  if (s->children[i])
                    {
                       s->children[j] = s->children[i];
                       j++;
                    }
               }
             if (j == 0)
               {
                  free(s->children);
                  s->children = NULL;
                  s->children_num = 0;
               }
             else
               {
                  s->children_num = j;
                  sl = realloc(s->children, sizeof(Shadow *) * j);
                  if (sl)
                    s->children = sl;
               }
          }
     }

   if (list)
     XFree(list);

   return s;
}

/**
 * @internal
 * Recursively frees a Shadow structure and all its children.
 *
 * This is a helper function for _ecore_x_window_tree_shadow_free to deallocate
 * the memory used by the shadow window tree.
 *
 * @param s The Shadow node to free.
 */
static void
_ecore_x_window_tree_shadow_free1(Shadow *s)
{
   int i;

   if (!s)
     return;

   if (s->children)
     {
        for (i = 0; i < s->children_num; i++)
          {
             if (s->children[i])
               _ecore_x_window_tree_shadow_free1(s->children[i]);
          }
        free(s->children);
     }

   free(s);
}

/**
 * @internal
 * Frees the entire shadow window tree.
 *
 * This function iterates through the base of the shadow tree (typically roots
 * of screens) and calls _ecore_x_window_tree_shadow_free1 to deallocate
 * all Shadow structures.
 */
static void
_ecore_x_window_tree_shadow_free(void)
{
   int i;

   if (!shadow_base)
     return;

   for (i = 0; i < shadow_num; i++)
     {
        if (!shadow_base[i])
          continue;

        _ecore_x_window_tree_shadow_free1(shadow_base[i]);
     }
   free(shadow_base);
   shadow_base = NULL;
   shadow_num = 0;
}

/**
 * @internal
 * Populates the shadow window tree by walking the actual X window tree.
 *
 * This function gets the list of root windows for all screens and then
 * calls _ecore_x_window_tree_walk for each root to build the shadow
 * representation.
 */
static void
_ecore_x_window_tree_shadow_populate(void)
{
   Ecore_X_Window *roots;
   int i, num;

   roots = ecore_x_window_root_list(&num);
   if (roots)
     {
        shadow_base = calloc(1, sizeof(Shadow *) * num);
        if (shadow_base)
          {
             shadow_num = num;
             for (i = 0; i < num; i++)
               shadow_base[i] = _ecore_x_window_tree_walk(roots[i]);
          }

        free(roots);
     }
}

/*
   static int shadow_count = 0;

   static void
   _ecore_x_window_tree_shadow_start(void)
   {
   shadow_count++;
   if (shadow_count > 1) return;
   _ecore_x_window_tree_shadow_populate();
   }

   static void
   _ecore_x_window_tree_shadow_stop(void)
   {
   shadow_count--;
   if (shadow_count != 0) return;
   _ecore_x_window_tree_shadow_free();
   }
 */

/**
 * @internal
 * Recursively searches for a Shadow node corresponding to a given X Window ID
 * within a subtree of the shadow window tree.
 *
 * @param s   The root of the Shadow subtree to search.
 * @param win The X Window ID to find.
 * @return A pointer to the Shadow structure if found, otherwise NULL.
 */
static Shadow *
_ecore_x_window_shadow_tree_find_shadow(Shadow *s,
                                        Window win)
{
   Shadow *ss;
   int i;

   if (s->win == win)
     return s;

   if (s->children)
     for (i = 0; i < s->children_num; i++)
       {
          if (!s->children[i])
            continue;

          if ((ss =
                 _ecore_x_window_shadow_tree_find_shadow(s->children[i], win)))
            return ss;
       }

   return NULL;
}

/**
 * @internal
 * Finds a Shadow node corresponding to a given X Window ID in the global
 * shadow window tree.
 *
 * This function iterates through all root nodes of the shadow tree (one for
 * each screen) and calls _ecore_x_window_shadow_tree_find_shadow to search
 * within each screen's subtree.
 *
 * @param base The X Window ID to find.
 * @return A pointer to the Shadow structure if found, otherwise NULL.
 */
static Shadow *
_ecore_x_window_shadow_tree_find(Window base)
{
   Shadow *s;
   int i;

   for (i = 0; i < shadow_num; i++)
     {
        if (!shadow_base[i])
          continue;

        if ((s = _ecore_x_window_shadow_tree_find_shadow(shadow_base[i], base)))
          return s;
     }
   return NULL;
}

/**
 * @internal
 * Checks if a given point (x, y) is inside any of the provided rectangles.
 *
 * The point (x, y) is in global coordinates. The rectangles are relative to
 * the shadow window `s` whose top-left corner is at (`s->x + bx`, `s->y + by`).
 *
 * @param s     The Shadow window structure, used for its position.
 * @param x     The global X coordinate of the point to check.
 * @param y     The global Y coordinate of the point to check.
 * @param bx    The X offset of the parent window's coordinate system relative to global.
 * @param by    The Y offset of the parent window's coordinate system relative to global.
 * @param rects An array of Ecore_X_Rectangle structures.
 *              Example: `rects[0] = { .x=10, .y=10, .width=20, .height=20 }`
 * @param num   The number of rectangles in the @p rects array.
 * @return 1 if the point is inside any rectangle, 0 otherwise. The @p rects array is freed by this function.
 */
static int
_inside_rects(Shadow *s,
              int x,
              int y,
              int bx,
              int by,
              Ecore_X_Rectangle *rects,
              int num)
{
   int i, inside;

   if (!rects) return 0;
   inside = 0;
   for (i = 0; i < num; i++)
     {
        if ((x >= s->x + bx + rects[i].x) &&
            (y >= s->y + by + rects[i].y) &&
            (x < (int)(s->x + bx + rects[i].x + rects[i].width)) &&
            (y < (int)(s->y + by + rects[i].y + rects[i].height)))
          {
             inside = 1;
             break;
          }
     }
   free(rects);
   return inside;
}

/**
 * @internal
 * Recursively finds the topmost visible window in the shadow tree at global
 * coordinates (x, y), considering window shapes and a skip list.
 *
 * This function traverses the shadow tree. For each window, it checks if the
 * point (x, y) is within its bounds and shape. If so, it recursively checks
 * its children. If no child contains the point, the current window is returned.
 *
 * @param s        The current Shadow node being checked.
 * @param bx       The X offset of the parent of @p s relative to global coordinates.
 * @param by       The Y offset of the parent of @p s relative to global coordinates.
 * @param x        The global X coordinate.
 * @param y        The global Y coordinate.
 * @param skip     An array of Ecore_X_Window IDs to ignore during the search.
 *                 Example: `skip[0] = some_window_id_to_ignore`
 * @param skip_num The number of windows in the @p skip array.
 * @return The Ecore_X_Window ID of the topmost window at (x, y), or 0 if no
 *         window is found (or if the found window is shaped out).
 */
static Window
_ecore_x_window_shadow_tree_at_xy_get_shadow(Shadow *s,
                                             int bx,
                                             int by,
                                             int x,
                                             int y,
                                             Ecore_X_Window *skip,
                                             int skip_num)
{
   Window child;
   int i, j;
   int wx, wy;

   wx = s->x + bx;
   wy = s->y + by;
   if (!((x >= wx) && (y >= wy) && (x < (wx + s->w)) && (y < (wy + s->h))))
     return 0;

   /* FIXME: get shape */
   {
      int num;
      Ecore_X_Rectangle *rects;

      num = 0;
      rects = ecore_x_window_shape_rectangles_get(s->win, &num);
      if (!_inside_rects(s, x, y, bx, by, rects, num)) return 0;
      num = 0;
      rects = ecore_x_window_shape_input_rectangles_get(s->win, &num);
      if (!_inside_rects(s, x, y, bx, by, rects, num)) return 0;
   }

   if (s->children)
     {
        int skipit = 0;

        for (i = s->children_num - 1; i >= 0; --i)
          {
             if (!s->children[i])
               continue;

             skipit = 0;
             if (skip)
               for (j = 0; j < skip_num; j++)
                 {
                    if (s->children[i]->win == skip[j])
                      {
                         skipit = 1;
                         goto onward;
                      }
                 }

onward:
             if (!skipit)
               if ((child =
                      _ecore_x_window_shadow_tree_at_xy_get_shadow(s->
                                                                   children[i
                                                                   ], wx, wy,
                                                                   x, y, skip,
                                                                   skip_num)))
                 return child;
          }
     }

   return s->win;
}

/**
 * @internal
 * Finds the topmost visible window in the shadow tree starting from a @p base
 * window at global coordinates (x,y), considering a skip list.
 *
 * This function first ensures the shadow tree is populated. It then finds the
 * Shadow node for @p base and calls
 * _ecore_x_window_shadow_tree_at_xy_get_shadow to perform the search.
 *
 * @param base     The Ecore_X_Window ID from which to start the search (usually a root window).
 * @param bx       Initial X offset (usually 0 for root window searches).
 * @param by       Initial Y offset (usually 0 for root window searches).
 * @param x        The global X coordinate.
 * @param y        The global Y coordinate.
 * @param skip     An array of Ecore_X_Window IDs to ignore.
 * @param skip_num The number of windows in the @p skip array.
 * @return The Ecore_X_Window ID of the topmost window, or 0 if not found.
 */
static Window
_ecore_x_window_shadow_tree_at_xy_get(Window base,
                                      int bx,
                                      int by,
                                      int x,
                                      int y,
                                      Ecore_X_Window *skip,
                                      int skip_num)
{
   Shadow *s;

   if (!shadow_base)
     {
        _ecore_x_window_tree_shadow_populate();
        if (!shadow_base)
          return 0;
     }

   s = _ecore_x_window_shadow_tree_find(base);
   if (!s)
     return 0;

   return _ecore_x_window_shadow_tree_at_xy_get_shadow(s,
                                                       bx,
                                                       by,
                                                       x,
                                                       y,
                                                       skip,
                                                       skip_num);
}

/**
 * Retrieves the top, visible window at the given location,
 * but skips the windows in the list. This uses a shadow tree built from the
 * window tree that is only updated the first time
 * ecore_x_window_shadow_tree_at_xy_with_skip_get() is called, or the next time
 * it is called after a  ecore_x_window_shadow_tree_flush()
 * @param   base The base window to start searching from (normally root).
 * @param   x The given X position.
 * @param   y The given Y position.
 * @param   skip The list of windows to be skipped.
 * @param   skip_num The number of windows to be skipped.
 * @return  The window at that position.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI Ecore_X_Window
ecore_x_window_shadow_tree_at_xy_with_skip_get(Ecore_X_Window base,
                                               int x,
                                               int y,
                                               Ecore_X_Window *skip,
                                               int skip_num)
{
   LOGFN;
   return _ecore_x_window_shadow_tree_at_xy_get(base,
                                                0,
                                                0,
                                                x,
                                                y,
                                                skip,
                                                skip_num);
}

/**
 * Retrieves the parent window a given window has. This uses the shadow window
 * tree.
 * @param   root The root window of @p win - if 0, this will be automatically determined with extra processing overhead
 * @param   win The window to get the parent window of
 * @return  The parent window of @p win
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI Ecore_X_Window
ecore_x_window_shadow_parent_get(Ecore_X_Window root EINA_UNUSED,
                                 Ecore_X_Window win)
{
   Shadow *s;
   int i;

   LOGFN;
   if (!shadow_base)
     {
        _ecore_x_window_tree_shadow_populate();
        if (!shadow_base)
          return 0;
     }

   for (i = 0; i < shadow_num; i++)
     {
        if (!shadow_base[i])
          continue;

        s = _ecore_x_window_shadow_tree_find_shadow(shadow_base[i], win);
        if (s)
          {
             if (!s->parent)
               return 0;

             return s->parent->win;
          }
     }
   return 0;
}

/**
 * Flushes the window shadow tree so nothing is stored.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI void
ecore_x_window_shadow_tree_flush(void)
{
   LOGFN;
   _ecore_x_window_tree_shadow_free();
}

/**
 * Retrieves the root window a given window is on.
 * @param   win The window to get the root window of
 * @return  The root window of @p win
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI Ecore_X_Window
ecore_x_window_root_get(Ecore_X_Window win)
{
   XWindowAttributes att;

   LOGFN;
   if (!XGetWindowAttributes(_ecore_x_disp, win, &att))
     return 0;

   return att.root;
}

/**
 * @internal
 * Recursively finds the topmost visible window by querying the X server directly
 * (not using the shadow tree) at global coordinates (x, y).
 *
 * This function checks if the current @p base window is visible and contains
 * the point (x, y). If so, it queries its children and recursively calls
 * itself for each child. If no child contains the point, @p base is returned.
 * This version can be slower than the shadow tree version due to multiple
 * X server requests.
 *
 * @param base     The current window being checked.
 * @param bx       The X offset of the parent of @p base relative to global coordinates.
 * @param by       The Y offset of the parent of @p base relative to global coordinates.
 * @param x        The global X coordinate.
 * @param y        The global Y coordinate.
 * @param skip     An array of Ecore_X_Window IDs to ignore.
 *                 Example: `skip[0] = some_window_id_to_ignore`
 * @param skip_num The number of windows in the @p skip array.
 * @return The Ecore_X_Window ID of the topmost window at (x, y), or 0 if no
 *         window is found.
 */
static Window
_ecore_x_window_at_xy_get(Window base,
                          int bx,
                          int by,
                          int x,
                          int y,
                          Ecore_X_Window *skip,
                          int skip_num)
{
   Window *list = NULL;
   Window parent_win = 0, child = 0, root_win = 0;
   int i, j, wx, wy, ww, wh;
   unsigned int num;

   LOGFN;
   if (!ecore_x_window_visible_get(base))
     return 0;

   LOGFN;
   ecore_x_window_geometry_get(base, &wx, &wy, &ww, &wh);
   wx += bx;
   wy += by;

   if (!((x >= wx) && (y >= wy) && (x < (wx + ww)) && (y < (wy + wh))))
     return 0;

   LOGFN;
   if (!XQueryTree(_ecore_x_disp, base, &root_win, &parent_win, &list, &num))
     return base;

   if (list)
     {
        int skipit = 0;

        for (i = num - 1; i >= 0; --i)
          {
             skipit = 0;

             if (skip)
               for (j = 0; j < skip_num; j++)
                 {
                    if (list[i] == skip[j])
                      {
                         skipit = 1;
                         goto onward;
                      }
                 }

onward:
             if (!skipit)
               if ((child =
                      _ecore_x_window_at_xy_get(list[i], wx, wy, x, y, skip,
                                                skip_num)))
                 {
                    XFree(list);
                    return child;
                 }
          }
        XFree(list);
     }

   return base;
}

/**
 * Retrieves the top, visible window at the given location.
 * @param   x The given X position.
 * @param   y The given Y position.
 * @return  The window at that position.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI Ecore_X_Window
ecore_x_window_at_xy_get(int x,
                         int y)
{
   Ecore_X_Window win, root;

   LOGFN;
   /* FIXME: Proper function to determine current root/virtual root
    * window missing here */
   root = DefaultRootWindow(_ecore_x_disp);

   ecore_x_grab();
   win = _ecore_x_window_at_xy_get(root, 0, 0, x, y, NULL, 0);
   ecore_x_ungrab();

   return win ? win : root;
}

/**
 * Retrieves the top, visible window at the given location,
 * but skips the windows in the list.
 * @param   x The given X position.
 * @param   y The given Y position.
 * @param   skip The list of windows to be skipped.
 * @param   skip_num The number of windows to be skipped.
 * @return  The window at that position.
 * @ingroup Ecore_X_Window_Geometry_Group
 */
EAPI Ecore_X_Window
ecore_x_window_at_xy_with_skip_get(int x,
                                   int y,
                                   Ecore_X_Window *skip,
                                   int skip_num)
{
   Ecore_X_Window win, root;

   LOGFN;
   /* FIXME: Proper function to determine current root/virtual root
    * window missing here */
   root = DefaultRootWindow(_ecore_x_disp);

   ecore_x_grab();
   win = _ecore_x_window_at_xy_get(root, 0, 0, x, y, skip, skip_num);
   ecore_x_ungrab();

   return win ? win : root;
}

/**
 * Retrieves the top, visible window at the given location, starting the search from a specific window.
 *
 * This function performs a "live" query of the X server, similar to
 * ecore_x_window_at_xy_get(), but starts its traversal from the @p begin
 * window instead of the root window.
 *
 * @param   begin The window from which to start the search. The search will
 *                consider @p begin and its children.
 * @param   x     The global X coordinate.
 * @param   y     The global Y coordinate.
 * @return  The Ecore_X_Window ID of the topmost window at (x, y) within the
 *          subtree of @p begin, or @p begin itself if no child is found at
 *          that location but @p begin is. Returns 0 if @p begin is not at (x,y).
 */
EAPI Ecore_X_Window
ecore_x_window_at_xy_begin_get(Ecore_X_Window begin,
                               int x,
                               int y)
{
   Ecore_X_Window win;

   LOGFN;
   ecore_x_grab();
   win = _ecore_x_window_at_xy_get(begin, 0, 0, x, y, NULL, 0);
   ecore_x_ungrab();

   return win ? win : begin;
}

/**
 * Retrieves the parent window of the given window.
 * @param   win The given window.
 * @return  The parent window of @p win.
 * @ingroup Ecore_X_Window_Parent_Group
 */
EAPI Ecore_X_Window
ecore_x_window_parent_get(Ecore_X_Window win)
{
   Window root, parent, *children = NULL;
   unsigned int num;
   Eina_Bool success;

   LOGFN;
   success = XQueryTree(_ecore_x_disp, win, &root, &parent, &children, &num);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success) return 0;

   if (children)
     XFree(children);

   return parent;
}

/**
 * Sets the background color of the given window.
 * @param win   The given window
 * @param r     red value (0...65536, 16 bits)
 * @param g     green value (0...65536, 16 bits)
 * @param b     blue value (0...65536, 16 bits)
 */
EAPI void
ecore_x_window_background_color_set(Ecore_X_Window win,
                                    unsigned short r,
                                    unsigned short g,
                                    unsigned short b)
{
   XSetWindowAttributes attr;
   Colormap map;
   XColor col;

   LOGFN;
   col.red = r;
   col.green = g;
   col.blue = b;

   map = DefaultColormap(_ecore_x_disp, DefaultScreen(_ecore_x_disp));
   XAllocColor(_ecore_x_disp, map, &col);

   attr.background_pixel = col.pixel;
   XChangeWindowAttributes(_ecore_x_disp, win, CWBackPixel, &attr);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the window gravity for a given window.
 *
 * Window gravity determines how a window's position is affected when its
 * parent is resized. For example, NorthWestGravity means the window will
 * maintain its position relative to the top-left corner of its parent.
 *
 * @param win  The window for which to set the gravity.
 * @param grav The Ecore_X_Gravity value to set (e.g., ECORE_X_GRAVITY_NORTH_WEST).
 *             See X.h for Gravity constants (e.g., NorthWestGravity).
 */
EAPI void
ecore_x_window_gravity_set(Ecore_X_Window win,
                           Ecore_X_Gravity grav)
{
   XSetWindowAttributes att;

   LOGFN;
   att.win_gravity = grav;
   XChangeWindowAttributes(_ecore_x_disp, win, CWWinGravity, &att);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the bit gravity for a given window.
 *
 * Bit gravity determines how the contents of a window are preserved or
 * repositioned when the window is resized. For example, NorthWestGravity
 * means the top-left portion of the window's contents will be preserved.
 *
 * @param win  The window for which to set the bit gravity.
 * @param grav The Ecore_X_Gravity value to set (e.g., ECORE_X_GRAVITY_NORTH_WEST).
 *             See X.h for Gravity constants (e.g., NorthWestGravity).
 */
EAPI void
ecore_x_window_pixel_gravity_set(Ecore_X_Window win,
                                 Ecore_X_Gravity grav)
{
   XSetWindowAttributes att;

   LOGFN;
   att.bit_gravity = grav;
   XChangeWindowAttributes(_ecore_x_disp, win, CWBitGravity, &att);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets the background pixmap for a given window.
 *
 * This will tile the specified pixmap as the window's background.
 * If the pixmap is None, the window's background is not changed in this way,
 * and it might be inherited or use a solid color.
 *
 * @param win  The window for which to set the background pixmap.
 * @param pmap The Ecore_X_Pixmap to use as the background.
 *             Set to 0 (None) to remove a previously set background pixmap
 *             and revert to default background behavior.
 */
EAPI void
ecore_x_window_pixmap_set(Ecore_X_Window win,
                          Ecore_X_Pixmap pmap)
{
   LOGFN;
   XSetWindowBackgroundPixmap(_ecore_x_disp, win, pmap);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Clears a rectangular area within a window.
 *
 * This function fills the specified rectangle with the window's current
 * background color or pixmap. It does not generate Expose events.
 *
 * @param win The window in which to clear the area.
 * @param x   The X coordinate of the top-left corner of the area to clear,
 *            relative to the window's origin.
 * @param y   The Y coordinate of the top-left corner of the area to clear.
 * @param w   The width of the area to clear. If 0, it extends to the
 *            window's right edge from x.
 * @param h   The height of the area to clear. If 0, it extends to the
 *            window's bottom edge from y.
 */
EAPI void
ecore_x_window_area_clear(Ecore_X_Window win,
                          int x,
                          int y,
                          int w,
                          int h)
{
   LOGFN;
   XClearArea(_ecore_x_disp, win, x, y, w, h, False);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Clears a rectangular area within a window and generates Expose events.
 *
 * This function fills the specified rectangle with the window's current
 * background and then generates Expose events for that area. This signals
 * to the application that the area needs to be redrawn.
 *
 * @param win The window in which to expose the area.
 * @param x   The X coordinate of the top-left corner of the area,
 *            relative to the window's origin.
 * @param y   The Y coordinate of the top-left corner of the area.
 * @param w   The width of the area. If 0, it extends to the
 *            window's right edge from x.
 * @param h   The height of the area. If 0, it extends to the
 *            window's bottom edge from y.
 */
EAPI void
ecore_x_window_area_expose(Ecore_X_Window win,
                           int x,
                           int y,
                           int w,
                           int h)
{
   LOGFN;
   XClearArea(_ecore_x_disp, win, x, y, w, h, True);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Sets or clears the override-redirect attribute of a window.
 *
 * If @p override is EINA_TRUE, the window manager will typically not manage
 * (e.g., decorate, move, resize) this window. This is often used for menus,
 * tooltips, and other transient pop-up windows.
 *
 * @param win      The window for which to set the override-redirect attribute.
 * @param override EINA_TRUE to set override-redirect, EINA_FALSE to clear it.
 */
EAPI void
ecore_x_window_override_set(Ecore_X_Window win,
                            Eina_Bool override)
{
   XSetWindowAttributes att;

   LOGFN;
   att.override_redirect = override;
   XChangeWindowAttributes(_ecore_x_disp, win, CWOverrideRedirect, &att);
   if (_ecore_xlib_sync) ecore_x_sync();
}

#ifdef ECORE_XRENDER
/**
 * @internal
 * Creates a new window with an ARGB visual (if available).
 *
 * This is an internal helper function used by other ARGB window creation
 * functions. It attempts to find a 32-bit visual with an alpha channel.
 *
 * @param parent    The parent window. If 0, the default root window is used.
 * @param x         The X coordinate of the new window.
 * @param y         The Y coordinate of the new window.
 * @param w         The width of the new window.
 * @param h         The height of the new window.
 * @param override  If EINA_TRUE, set override-redirect for the window.
 * @param saveunder If EINA_TRUE, set save-under for the window (hint for server).
 * @return The ID of the newly created ARGB window, or 0 on failure (e.g.,
 *         if no suitable ARGB visual is found or XRender is not available).
 */
static Ecore_X_Window
_ecore_x_window_argb_internal_new(Ecore_X_Window parent,
                                  int x,
                                  int y,
                                  int w,
                                  int h,
                                  Eina_Bool override,
                                  Eina_Bool saveunder)
{
   Window win;
   XSetWindowAttributes attr;
   XWindowAttributes att;
   XVisualInfo *xvi;
   XVisualInfo vi_in;
   int nvi, i, scr = 0;
   XRenderPictFormat *fmt;
   Visual *vis;

   if (parent == 0)
     {
        parent = DefaultRootWindow(_ecore_x_disp);
        scr = DefaultScreen(_ecore_x_disp);
     }
   else
     {
        /* ewww - round trip */
        if (XGetWindowAttributes(_ecore_x_disp, parent, &att))
          {
             if (_ecore_xlib_sync) ecore_x_sync();
             for (i = 0; i < ScreenCount(_ecore_x_disp); i++)
               {
                  if (att.screen == ScreenOfDisplay(_ecore_x_disp, i))
                    {
                       scr = i;
                       break;
                    }
               }
          }
     }

   vi_in.screen = scr;
   vi_in.depth = 32;
   vi_in.class = TrueColor;
   xvi = XGetVisualInfo(_ecore_x_disp,
                        VisualScreenMask |
                        VisualDepthMask |
                        VisualClassMask,
                        &vi_in,
                        &nvi);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!xvi)
     return 0;

   vis = NULL;
   for (i = 0; i < nvi; i++)
     {
        fmt = XRenderFindVisualFormat(_ecore_x_disp, xvi[i].visual);
        if (_ecore_xlib_sync) ecore_x_sync();
        if ((fmt->type == PictTypeDirect) && (fmt->direct.alphaMask))
          {
             vis = xvi[i].visual;
             break;
          }
     }
   XFree (xvi);

   attr.backing_store = NotUseful;
   attr.override_redirect = override;
   attr.colormap = XCreateColormap(_ecore_x_disp, parent,
                                   vis, AllocNone);
   attr.border_pixel = 0;
   attr.background_pixmap = None;
   attr.bit_gravity = NorthWestGravity;
   attr.win_gravity = NorthWestGravity;
   attr.save_under = saveunder;
   attr.do_not_propagate_mask = NoEventMask;
   attr.event_mask = KeyPressMask |
     KeyReleaseMask |
     ButtonPressMask |
     ButtonReleaseMask |
     EnterWindowMask |
     LeaveWindowMask |
     PointerMotionMask |
     ExposureMask |
     VisibilityChangeMask |
     StructureNotifyMask |
     FocusChangeMask |
     PropertyChangeMask |
     ColormapChangeMask;
   win = XCreateWindow(_ecore_x_disp, parent,
                       x, y, w, h, 0,
                       32,
                       InputOutput,
                       vis,
                       CWBackingStore |
                       CWOverrideRedirect |
                       CWColormap |
                       CWBorderPixel |
                       CWBackPixmap |
                       CWSaveUnder |
                       CWDontPropagate |
                       CWEventMask |
                       CWBitGravity |
                       CWWinGravity,
                       &attr);
   if (_ecore_xlib_sync) ecore_x_sync();

   if (parent == DefaultRootWindow(_ecore_x_disp))
     ecore_x_window_defaults_set(win);

   return win;
}

#endif /* ifdef ECORE_XRENDER */

/**
 * Checks if a window is an ARGB window (i.e., uses a visual with an alpha channel).
 *
 * This function requires the XRender extension.
 *
 * @param  win The window to check.
 * @return 1 if the window is an ARGB window, 0 otherwise or if XRender is
 *         not available.
 */
EAPI int
ecore_x_window_argb_get(Ecore_X_Window win)
{
#ifdef ECORE_XRENDER
   XWindowAttributes att;
   XRenderPictFormat *fmt;
   Eina_Bool ret;

   att.visual = 0;
   ret = XGetWindowAttributes(_ecore_x_disp, win, &att);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!ret) return 0;

   fmt = XRenderFindVisualFormat(_ecore_x_disp, att.visual);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!fmt)
     return 0;

   if ((fmt->type == PictTypeDirect) && (fmt->direct.alphaMask))
     return 1;

   return 0;
#else /* ifdef ECORE_XRENDER */
   return 0;
#endif /* ifdef ECORE_XRENDER */
}

/**
 * Creates a new window.
 * @param   parent The parent window to use.  If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window handle.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_manager_argb_new(Ecore_X_Window parent,
                                int x,
                                int y,
                                int w,
                                int h)
{
#ifdef ECORE_XRENDER
   LOGFN;
   return _ecore_x_window_argb_internal_new(parent, x, y, w, h, 1, 0);
#else /* ifdef ECORE_XRENDER */
   return 0;
#endif /* ifdef ECORE_XRENDER */
}

/**
 * Creates a new window.
 * @param   parent The parent window to use.  If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window handle.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_argb_new(Ecore_X_Window parent,
                        int x,
                        int y,
                        int w,
                        int h)
{
#ifdef ECORE_XRENDER
   LOGFN;
   return _ecore_x_window_argb_internal_new(parent, x, y, w, h, 0, 0);
#else /* ifdef ECORE_XRENDER */
   return 0;
#endif /* ifdef ECORE_XRENDER */
}

/**
 * Creates a window with the override redirect attribute set to @c True.
 * @param   parent The parent window to use.  If @p parent is @c 0, the root
 *                 window of the default display is used.
 * @param   x      X position.
 * @param   y      Y position.
 * @param   w      Width.
 * @param   h      Height.
 * @return  The new window handle.
 * @ingroup Ecore_X_Window_Create_Group
 */
EAPI Ecore_X_Window
ecore_x_window_override_argb_new(Ecore_X_Window parent,
                                 int x,
                                 int y,
                                 int w,
                                 int h)
{
#ifdef ECORE_XRENDER
   LOGFN;
   return _ecore_x_window_argb_internal_new(parent, x, y, w, h, 1, 0);
#else /* ifdef ECORE_XRENDER */
   return 0;
#endif /* ifdef ECORE_XRENDER */
}

/**
 * Creates or retrieves a "permanent" window associated with a unique atom on a parent window.
 *
 * This function is used to create a helper window that can persist across
 * application restarts or client disconnections, provided the X server
 * supports it and the window is set to RetainPermanent.
 *
 * The function first tries to find an existing window by checking for a
 * property named @p unique_atom on the @p parent window. If this property
 * exists and points to a valid window, and that window also has the
 * @p unique_atom property pointing back to itself, that window is returned.
 *
 * If no such window is found, a new simple window is created as a child of
 * @p parent. The @p unique_atom property is then set on both the new window
 * (pointing to itself) and on the @p parent window (pointing to the new window).
 * The close-down mode for the new window is set to RetainPermanent.
 *
 * This mechanism is often used for tasks like selection ownership or other
 * roles where a persistent, uniquely identifiable window is needed.
 *
 * @param parent      The parent window under which to find or create the permanent window.
 * @param unique_atom An X atom that uniquely identifies this permanent window's role
 *                    or purpose. For example, `ECORE_X_ATOM_E_PERMANENT_WINDOW`.
 * @return The Ecore_X_Window ID of the existing or newly created permanent window.
 *         Returns 0 on failure (e.g., if unable to open a new display connection).
 */
EAPI Ecore_X_Window
ecore_x_window_permanent_new(Ecore_X_Window parent, Ecore_X_Atom unique_atom)
{
   Display *disp;
   Window win, win2, realwin = 0;
   Atom type_ret;
   int format_ret;
   unsigned long ldata, bytes_after, num_ret, *datap;
   unsigned char *prop_ret;

   LOGFN;
   disp = XOpenDisplay(DisplayString(_ecore_x_disp));
   if (!disp) return 0;

   XGrabServer(disp);
   if (XGetWindowProperty(disp, parent, unique_atom, 0, 0x7fffffff,
                          False, XA_WINDOW, &type_ret, &format_ret,
                          &num_ret, &bytes_after, &prop_ret) == Success)
     {
        if (prop_ret)
          {
             if ((format_ret == 32) && (type_ret == XA_WINDOW) &&
                 (num_ret == 1))
               {
                  datap = (unsigned long *)prop_ret;
                  win = (Window)(*datap);
                  XFree(prop_ret);
                  if (XGetWindowProperty(disp, win, unique_atom, 0, 0x7fffffff,
                                         False, XA_WINDOW, &type_ret, &format_ret,
                                         &num_ret, &bytes_after, &prop_ret) == Success)
                    {
                       if (prop_ret)
                         {
                            if ((format_ret == 32) && (type_ret == XA_WINDOW) &&
                                (num_ret == 1))
                              {
                                 datap = (unsigned long *)prop_ret;
                                 win2 = (Window)(*datap);
                                 XFree(prop_ret);
                                 if (win2 == win) realwin = win;
                              }
                            else XFree(prop_ret);
                         }
                    }
               }
             else XFree(prop_ret);
          }
     }
   if (realwin != 0)
     {
        XUngrabServer(disp);
        XFlush(disp);
        XCloseDisplay(disp);
        return realwin;
     }
   win = XCreateSimpleWindow(disp, parent, -77, -77, 7, 7, 0, 0, 0);
   ldata = (unsigned long)win;
   XChangeProperty(disp, win, unique_atom, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *)(&ldata), 1);
   XChangeProperty(disp, parent, unique_atom, XA_WINDOW, 32,
                   PropModeReplace, (unsigned char *)(&ldata), 1);
   XSetCloseDownMode(disp, RetainPermanent);
   XUngrabServer(disp);
   XFlush(disp);
   XCloseDisplay(disp);
   return win;
}
