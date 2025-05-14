/*
 * Various ICCCM related functions.
 *
 * This is ALL the code involving anything ICCCM related. for both WM and
 * client.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

/**
 * @brief Initializes the ICCCM module.
 * @since 1.2
 *
 * This function currently does not perform any operations but is
 * reserved for future initialization needs of the ICCCM handling
 * within Ecore_X.
 */
EAPI void
ecore_x_icccm_init(void)
{
   LOGFN;
}

/**
 * @brief Sets the WM_STATE property of a window.
 * @param win The window whose state is to be set.
 * @param state The desired state for the window.
 * @since 1.2
 *
 * This function changes the WM_STATE property on the given window.
 * The state can be one of:
 * - ECORE_X_WINDOW_STATE_HINT_WITHDRAWN: The window is withdrawn.
 * - ECORE_X_WINDOW_STATE_HINT_NORMAL: The window is in its normal state.
 * - ECORE_X_WINDOW_STATE_HINT_ICONIC: The window is iconified.
 */
EAPI void
ecore_x_icccm_state_set(Ecore_X_Window win,
                        Ecore_X_Window_State_Hint state)
{
   unsigned long c[2];

   LOGFN;
   if (state == ECORE_X_WINDOW_STATE_HINT_WITHDRAWN)
     c[0] = WithdrawnState;
   else if (state == ECORE_X_WINDOW_STATE_HINT_NORMAL)
     c[0] = NormalState;
   else if (state == ECORE_X_WINDOW_STATE_HINT_ICONIC)
     c[0] = IconicState;

   c[1] = None;
   XChangeProperty(_ecore_x_disp, win, ECORE_X_ATOM_WM_STATE,
                   ECORE_X_ATOM_WM_STATE, 32, PropModeReplace,
                   (unsigned char *)c, 2);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the WM_STATE property of a window.
 * @param win The window whose state is to be retrieved.
 * @return The current state hint of the window.
 *         Returns ECORE_X_WINDOW_STATE_HINT_NONE if the property
 *         is not set or cannot be read.
 * @since 1.2
 */
EAPI Ecore_X_Window_State_Hint
ecore_x_icccm_state_get(Ecore_X_Window win)
{
   unsigned char *prop_ret = NULL;
   Atom type_ret;
   unsigned long bytes_after, num_ret;
   int format_ret;
   Ecore_X_Window_State_Hint hint;

   LOGFN;
   hint = ECORE_X_WINDOW_STATE_HINT_NONE;
   XGetWindowProperty(_ecore_x_disp, win, ECORE_X_ATOM_WM_STATE,
                      0, 0x7fffffff, False, ECORE_X_ATOM_WM_STATE,
                      &type_ret, &format_ret, &num_ret, &bytes_after,
                      &prop_ret);
   if (_ecore_xlib_sync) ecore_x_sync();
   if ((prop_ret) && (num_ret == 2))
     {
        if (prop_ret[0] == WithdrawnState)
          hint = ECORE_X_WINDOW_STATE_HINT_WITHDRAWN;
        else if (prop_ret[0] == NormalState)
          hint = ECORE_X_WINDOW_STATE_HINT_NORMAL;
        else if (prop_ret[0] == IconicState)
          hint = ECORE_X_WINDOW_STATE_HINT_ICONIC;
     }

   if (prop_ret)
     XFree(prop_ret);

   return hint;
}

/**
 * @brief Sends a WM_DELETE_WINDOW client message to a window.
 * @param win The window to which the message will be sent.
 * @param t The timestamp for the event.
 * @since 1.2
 *
 * This function is typically used by a window manager to request
 * that a client window close itself.
 */
EAPI void
ecore_x_icccm_delete_window_send(Ecore_X_Window win,
                                 Ecore_X_Time t)
{
   LOGFN;
   ecore_x_client_message32_send(win, ECORE_X_ATOM_WM_PROTOCOLS,
                                 ECORE_X_EVENT_MASK_NONE,
                                 ECORE_X_ATOM_WM_DELETE_WINDOW,
                                 t, 0, 0, 0);
}

/**
 * @brief Sends a WM_TAKE_FOCUS client message to a window.
 * @param win The window to which the message will be sent.
 * @param t The timestamp for the event.
 * @since 1.2
 *
 * This function is used to offer the input focus to a window.
 * A client should respond to this message by setting the input
 * focus to one of its windows if it wants focus.
 */
EAPI void
ecore_x_icccm_take_focus_send(Ecore_X_Window win,
                              Ecore_X_Time t)
{
   LOGFN;
   ecore_x_client_message32_send(win, ECORE_X_ATOM_WM_PROTOCOLS,
                                 ECORE_X_EVENT_MASK_NONE,
                                 ECORE_X_ATOM_WM_TAKE_FOCUS,
                                 t, 0, 0, 0);
}

/**
 * @brief Sends a WM_SAVE_YOURSELF client message to a window.
 * @param win The window to which the message will be sent.
 * @param t The timestamp for the event.
 * @since 1.2
 *
 * This function is used by a session manager to request that a client
 * save its state before termination.
 */
EAPI void
ecore_x_icccm_save_yourself_send(Ecore_X_Window win,
                                 Ecore_X_Time t)
{
   LOGFN;
   ecore_x_client_message32_send(win, ECORE_X_ATOM_WM_PROTOCOLS,
                                 ECORE_X_EVENT_MASK_NONE,
                                 ECORE_X_ATOM_WM_SAVE_YOURSELF,
                                 t, 0, 0, 0);
}

/**
 * @brief Sends a synthetic ConfigureNotify event to a window.
 * @param win The window to configure.
 * @param x The new x-coordinate of the window.
 * @param y The new y-coordinate of the window.
 * @param w The new width of the window.
 * @param h The new height of the window.
 * @since 1.2
 *
 * This function sends a ConfigureNotify event to the specified window,
 * effectively instructing it (or informing it, if it's a client window
 * being managed) about a change in its geometry. This is often used
 * by window managers.
 */
EAPI void
ecore_x_icccm_move_resize_send(Ecore_X_Window win,
                               int x,
                               int y,
                               int w,
                               int h)
{
   XEvent ev;

   LOGFN;
   ev.type = ConfigureNotify;
   ev.xconfigure.display = _ecore_x_disp;
   ev.xconfigure.event = win;
   ev.xconfigure.window = win;
   ev.xconfigure.x = x;
   ev.xconfigure.y = y;
   ev.xconfigure.width = w;
   ev.xconfigure.height = h;
   ev.xconfigure.border_width = 0;
   ev.xconfigure.above = None;
   ev.xconfigure.override_redirect = False;
   XSendEvent(_ecore_x_disp, win, False, StructureNotifyMask, &ev);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Sets the WM_HINTS property for a window.
 * @param win The window for which to set the hints.
 * @param accepts_focus EINA_TRUE if the window accepts input focus, EINA_FALSE otherwise.
 * @param initial_state The initial state of the window (e.g., normal, iconic, withdrawn).
 * @param icon_pixmap The pixmap to be used as an icon. Can be 0.
 * @param icon_mask The mask for the icon pixmap. Can be 0.
 * @param icon_window A window to be used as an icon. Can be 0.
 * @param window_group The leader of a group of windows. Can be 0.
 * @param is_urgent EINA_TRUE if the window requires urgent attention, EINA_FALSE otherwise.
 * @since 1.2
 *
 * This function sets various hints for the window manager regarding
 * the window's behavior and appearance.
 */
EAPI void
ecore_x_icccm_hints_set(Ecore_X_Window win,
                        Eina_Bool accepts_focus,
                        Ecore_X_Window_State_Hint initial_state,
                        Ecore_X_Pixmap icon_pixmap,
                        Ecore_X_Pixmap icon_mask,
                        Ecore_X_Window icon_window,
                        Ecore_X_Window window_group,
                        Eina_Bool is_urgent)
{
   XWMHints *hints;

   hints = XAllocWMHints();
   if (!hints)
     return;

   LOGFN;
   hints->flags = InputHint | StateHint;
   hints->input = accepts_focus;
   if (initial_state == ECORE_X_WINDOW_STATE_HINT_WITHDRAWN)
     hints->initial_state = WithdrawnState;
   else if (initial_state == ECORE_X_WINDOW_STATE_HINT_NORMAL)
     hints->initial_state = NormalState;
   else if (initial_state == ECORE_X_WINDOW_STATE_HINT_ICONIC)
     hints->initial_state = IconicState;

   if (icon_pixmap != 0)
     {
        hints->icon_pixmap = icon_pixmap;
        hints->flags |= IconPixmapHint;
     }

   if (icon_mask != 0)
     {
        hints->icon_mask = icon_mask;
        hints->flags |= IconMaskHint;
     }

   if (icon_window != 0)
     {
        hints->icon_window = icon_window;
        hints->flags |= IconWindowHint;
     }

   if (window_group != 0)
     {
        hints->window_group = window_group;
        hints->flags |= WindowGroupHint;
     }

   if (is_urgent)
     hints->flags |= XUrgencyHint;

   XSetWMHints(_ecore_x_disp, win, hints);
   if (_ecore_xlib_sync) ecore_x_sync();
   XFree(hints);
}

/**
 * @brief Retrieves the WM_HINTS property for a window.
 * @param win The window from which to get the hints.
 * @param accepts_focus Pointer to store whether the window accepts input focus. Can be NULL.
 * @param initial_state Pointer to store the initial state of the window. Can be NULL.
 * @param icon_pixmap Pointer to store the icon pixmap. Can be NULL.
 * @param icon_mask Pointer to store the icon mask. Can be NULL.
 * @param icon_window Pointer to store the icon window. Can be NULL.
 * @param window_group Pointer to store the window group leader. Can be NULL.
 * @param is_urgent Pointer to store the urgency hint. Can be NULL.
 * @return EINA_TRUE if hints were successfully retrieved, EINA_FALSE otherwise.
 * @since 1.2
 *
 * This function retrieves various hints set for the window.
 * Output parameters are initialized to default values before attempting to read.
 */
EAPI Eina_Bool
ecore_x_icccm_hints_get(Ecore_X_Window win,
                        Eina_Bool *accepts_focus,
                        Ecore_X_Window_State_Hint *initial_state,
                        Ecore_X_Pixmap *icon_pixmap,
                        Ecore_X_Pixmap *icon_mask,
                        Ecore_X_Window *icon_window,
                        Ecore_X_Window *window_group,
                        Eina_Bool *is_urgent)
{
   XWMHints *hints;

   LOGFN;
   if (accepts_focus)
     *accepts_focus = EINA_TRUE;

   if (initial_state)
     *initial_state = ECORE_X_WINDOW_STATE_HINT_NORMAL;

   if (icon_pixmap)
     *icon_pixmap = 0;

   if (icon_mask)
     *icon_mask = 0;

   if (icon_window)
     *icon_window = 0;

   if (window_group)
     *window_group = 0;

   if (is_urgent)
     *is_urgent = EINA_FALSE;

   hints = XGetWMHints(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (hints)
     {
        if ((hints->flags & InputHint) && (accepts_focus))
          {
             if (hints->input)
               *accepts_focus = EINA_TRUE;
             else
               *accepts_focus = EINA_FALSE;
          }

        if ((hints->flags & StateHint) && (initial_state))
          {
             if (hints->initial_state == WithdrawnState)
               *initial_state = ECORE_X_WINDOW_STATE_HINT_WITHDRAWN;
             else if (hints->initial_state == NormalState)
               *initial_state = ECORE_X_WINDOW_STATE_HINT_NORMAL;
             else if (hints->initial_state == IconicState)
               *initial_state = ECORE_X_WINDOW_STATE_HINT_ICONIC;
          }

        if ((hints->flags & IconPixmapHint) && (icon_pixmap))
          *icon_pixmap = hints->icon_pixmap;

        if ((hints->flags & IconMaskHint) && (icon_mask))
          *icon_mask = hints->icon_mask;

        if ((hints->flags & IconWindowHint) && (icon_window))
          *icon_window = hints->icon_window;

        if ((hints->flags & WindowGroupHint) && (window_group))
          *window_group = hints->window_group;

        if ((hints->flags & XUrgencyHint) && (is_urgent))
          *is_urgent = EINA_TRUE;

        XFree(hints);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Sets the WM_NORMAL_HINTS property for a window.
 * @param win The window for which to set the size/position hints.
 * @param request_pos EINA_TRUE if the window's initial position (if set in hints) is a request, not an absolute.
 * @param gravity The window gravity (e.g., ECORE_X_GRAVITY_NW for NorthWest).
 * @param min_w Minimum width. Set to 0 or less if no preference.
 * @param min_h Minimum height. Set to 0 or less if no preference.
 * @param max_w Maximum width. Set to 0 or less if no preference.
 * @param max_h Maximum height. Set to 0 or less if no preference.
 * @param base_w Base width for size increments. Set to 0 or less if no preference.
 * @param base_h Base height for size increments. Set to 0 or less if no preference.
 * @param step_x Width increment step. Set to 1 or less if no preference.
 * @param step_y Height increment step. Set to 1 or less if no preference.
 * @param min_aspect Minimum aspect ratio (width/height). Set to 0.0 or less if no preference.
 * @param max_aspect Maximum aspect ratio (width/height). Set to 0.0 or less if no preference.
 * @since 1.2
 *
 * This function sets hints related to the window's size, position,
 * and resizing behavior.
 */
EAPI void
ecore_x_icccm_size_pos_hints_set(Ecore_X_Window win,
                                 Eina_Bool request_pos,
                                 Ecore_X_Gravity gravity,
                                 int min_w,
                                 int min_h,
                                 int max_w,
                                 int max_h,
                                 int base_w,
                                 int base_h,
                                 int step_x,
                                 int step_y,
                                 double min_aspect,
                                 double max_aspect)
{
   XSizeHints hint;
   long mask;

   LOGFN;
   if (!XGetWMNormalHints(_ecore_x_disp, win, &hint, &mask))
     memset(&hint, 0, sizeof(XSizeHints));
   if (_ecore_xlib_sync) ecore_x_sync();

   hint.flags = 0;
   if (request_pos)
     hint.flags |= USPosition;

   if (gravity != ECORE_X_GRAVITY_NW)
     {
        hint.flags |= PWinGravity;
        hint.win_gravity = gravity;
     }

   if ((min_w > 0) || (min_h > 0))
     {
        hint.flags |= PMinSize;
        hint.min_width = min_w;
        hint.min_height = min_h;
     }

   if ((max_w > 0) || (max_h > 0))
     {
        hint.flags |= PMaxSize;
        hint.max_width = max_w;
        hint.max_height = max_h;
     }

   if ((base_w > 0) || (base_h > 0))
     {
        hint.flags |= PBaseSize;
        hint.base_width = base_w;
        hint.base_height = base_h;
     }

   if ((step_x > 1) || (step_y > 1))
     {
        hint.flags |= PResizeInc;
        hint.width_inc = step_x;
        hint.height_inc = step_y;
     }

   if ((min_aspect > 0.0) || (max_aspect > 0.0))
     {
        hint.flags |= PAspect;
        hint.min_aspect.x = min_aspect * 10000;
        hint.min_aspect.y = 10000;
        hint.max_aspect.x = max_aspect * 10000;
        hint.max_aspect.y = 10000;
     }

   XSetWMNormalHints(_ecore_x_disp, win, &hint);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the WM_NORMAL_HINTS property for a window.
 * @param win The window from which to get the size/position hints.
 * @param request_pos Pointer to store whether the window's position is a request. Can be NULL.
 * @param gravity Pointer to store the window gravity. Can be NULL.
 * @param min_w Pointer to store the minimum width. Can be NULL.
 * @param min_h Pointer to store the minimum height. Can be NULL.
 * @param max_w Pointer to store the maximum width. Can be NULL.
 * @param max_h Pointer to store the maximum height. Can be NULL.
 * @param base_w Pointer to store the base width. Can be NULL.
 * @param base_h Pointer to store the base height. Can be NULL.
 * @param step_x Pointer to store the width increment step. Can be NULL.
 * @param step_y Pointer to store the height increment step. Can be NULL.
 * @param min_aspect Pointer to store the minimum aspect ratio. Can be NULL.
 * @param max_aspect Pointer to store the maximum aspect ratio. Can be NULL.
 * @return EINA_TRUE if hints were successfully retrieved, EINA_FALSE otherwise.
 * @since 1.2
 *
 * This function retrieves hints related to the window's size, position,
 * and resizing behavior. Output parameters are initialized to default/sensible
 * values before attempting to read.
 */
EAPI Eina_Bool
ecore_x_icccm_size_pos_hints_get(Ecore_X_Window win,
                                 Eina_Bool *request_pos,
                                 Ecore_X_Gravity *gravity,
                                 int *min_w,
                                 int *min_h,
                                 int *max_w,
                                 int *max_h,
                                 int *base_w,
                                 int *base_h,
                                 int *step_x,
                                 int *step_y,
                                 double *min_aspect,
                                 double *max_aspect)
{
   XSizeHints hint;
   long mask;

   int minw = 0, minh = 0;
   int maxw = 32767, maxh = 32767;
   int basew = -1, baseh = -1;
   int stepx = -1, stepy = -1;
   double mina = 0.0, maxa = 0.0;

   LOGFN;
   if (!XGetWMNormalHints(_ecore_x_disp, win, &hint, &mask))
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        return EINA_FALSE;
     }

   if ((hint.flags & USPosition) || ((hint.flags & PPosition)))
     {
        if (request_pos)
          *request_pos = EINA_TRUE;
     }
   else if (request_pos)
     *request_pos = EINA_FALSE;

   if (hint.flags & PWinGravity)
     {
        if (gravity)
          *gravity = hint.win_gravity;
     }
   else if (gravity)
     *gravity = ECORE_X_GRAVITY_NW;

   if (hint.flags & PMinSize)
     {
        minw = hint.min_width;
        minh = hint.min_height;
     }

   if (hint.flags & PMaxSize)
     {
        maxw = hint.max_width;
        maxh = hint.max_height;
        if (maxw < minw)
          maxw = minw;

        if (maxh < minh)
          maxh = minh;
     }

   if (hint.flags & PBaseSize)
     {
        basew = hint.base_width;
        baseh = hint.base_height;
        if (basew > minw)
          minw = basew;

        if (baseh > minh)
          minh = baseh;
     }

   if (hint.flags & PResizeInc)
     {
        stepx = hint.width_inc;
        stepy = hint.height_inc;
        if (stepx < 1)
          stepx = 1;

        if (stepy < 1)
          stepy = 1;
     }

   if (hint.flags & PAspect)
     {
        if (hint.min_aspect.y > 0)
          mina = ((double)hint.min_aspect.x) / ((double)hint.min_aspect.y);

        if (hint.max_aspect.y > 0)
          maxa = ((double)hint.max_aspect.x) / ((double)hint.max_aspect.y);
     }

   if (min_w)
     *min_w = minw;

   if (min_h)
     *min_h = minh;

   if (max_w)
     *max_w = maxw;

   if (max_h)
     *max_h = maxh;

   if (base_w)
     *base_w = basew;

   if (base_h)
     *base_h = baseh;

   if (step_x)
     *step_x = stepx;

   if (step_y)
     *step_y = stepy;

   if (min_aspect)
     *min_aspect = mina;

   if (max_aspect)
     *max_aspect = maxa;

   return EINA_TRUE;
}

/**
 * @brief Sets the WM_NAME property (window title) for a window.
 * @param win The window for which to set the title.
 * @param t The title string. Must be UTF-8 encoded.
 * @since 1.2
 *
 * This function sets the title of the window, which is typically
 * displayed in the window's title bar by the window manager.
 * It attempts to set the title using UTF-8 encoding first,
 * then falls back to standard ICCCM text style if needed.
 */
EAPI void
ecore_x_icccm_title_set(Ecore_X_Window win,
                        const char *t)
{
   char *list[1];
   XTextProperty xprop;
   int ret;

   if (!t)
     return;

   LOGFN;
   xprop.value = NULL;
#ifdef X_HAVE_UTF8_STRING
   list[0] = strdup(t);
   ret =
     Xutf8TextListToTextProperty(_ecore_x_disp, list, 1, XUTF8StringStyle,
                                 &xprop);
#else /* ifdef X_HAVE_UTF8_STRING */
   list[0] = strdup(t);
   ret =
     XmbTextListToTextProperty(_ecore_x_disp, list, 1, XStdICCTextStyle,
                               &xprop);
#endif /* ifdef X_HAVE_UTF8_STRING */
   if (_ecore_xlib_sync) ecore_x_sync();
   if (ret >= Success)
     {
        XSetWMName(_ecore_x_disp, win, &xprop);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          XFree(xprop.value);
     }
   else if (XStringListToTextProperty(list, 1, &xprop) >= Success)
     {
        XSetWMName(_ecore_x_disp, win, &xprop);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          XFree(xprop.value);
     }

   free(list[0]);
}

/**
 * @brief Retrieves the WM_NAME property (window title) for a window.
 * @param win The window from which to get the title.
 * @return A newly allocated string containing the window title,
 *         or NULL if the title cannot be retrieved. The caller
 *         is responsible for freeing this string.
 *         The returned string is UTF-8 encoded.
 * @since 1.2
 */
EAPI char *
ecore_x_icccm_title_get(Ecore_X_Window win)
{
   XTextProperty xprop;

   LOGFN;
   xprop.value = NULL;
   if (XGetWMName(_ecore_x_disp, win, &xprop) >= Success)
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          {
             char **list = NULL;
             char *t = NULL;
             int num = 0;
             int ret;

             if (xprop.encoding == ECORE_X_ATOM_UTF8_STRING)
               t = strdup((char *)xprop.value);
             else
               {
                  /* convert to utf8 */
#ifdef X_HAVE_UTF8_STRING
                  ret = Xutf8TextPropertyToTextList(_ecore_x_disp, &xprop,
                                                    &list, &num);
#else /* ifdef X_HAVE_UTF8_STRING */
                  ret = XmbTextPropertyToTextList(_ecore_x_disp, &xprop,
                                                  &list, &num);
#endif /* ifdef X_HAVE_UTF8_STRING */
                  if (_ecore_xlib_sync) ecore_x_sync();

                  if ((ret == XLocaleNotSupported) ||
                      (ret == XNoMemory) || (ret == XConverterNotFound))
                    t = strdup((char *)xprop.value);
                  else if ((ret >= Success) && (num > 0))
                    t = strdup(list[0]);

                  if (list)
                    XFreeStringList(list);
               }

             if (xprop.value)
               XFree(xprop.value);

             return t;
          }
     }
   else
     {
        if (_ecore_xlib_sync) ecore_x_sync();
     }

   return NULL;
}

/**
 * @brief Sets the WM_PROTOCOLS property for a window using an explicit list of atoms.
 * @param win The window for which to set the protocols.
 * @param protos An array of Ecore_X_Atom representing the protocols.
 *               Example: `{ ECORE_X_ATOM_WM_DELETE_WINDOW, ECORE_X_ATOM_WM_TAKE_FOCUS }`
 * @param num The number of atoms in the @p protos array.
 * @since 1.2
 *
 * This function directly sets the WM_PROTOCOLS property with the provided
 * list of atoms. If @p num is 0, the property is deleted.
 */
EAPI void
ecore_x_icccm_protocol_atoms_set(Ecore_X_Window win,
                                 Ecore_X_Atom *protos,
                                 int num)
{
   Atom *protos2 = alloca(sizeof(Atom) * num);
   int i;

   for (i = 0; i < num; i++) protos2[i] = protos[i];
   LOGFN;
   if (num > 0)
     XSetWMProtocols(_ecore_x_disp, win, protos2, num);
   else
     XDeleteProperty(_ecore_x_disp, win, ECORE_X_ATOM_WM_PROTOCOLS);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Sets or unsets a specific WM protocol for a window.
 * @param win The window for which to modify the protocol.
 * @param protocol The Ecore_X_WM_Protocol to set or unset.
 *                 Example: ECORE_X_WM_PROTOCOL_DELETE_WINDOW
 * @param on EINA_TRUE to enable the protocol, EINA_FALSE to disable it.
 * @since 1.2
 *
 * This function modifies the WM_PROTOCOLS property by adding or removing
 * the specified protocol. It handles existing protocols correctly.
 */
EAPI void
ecore_x_icccm_protocol_set(Ecore_X_Window win,
                           Ecore_X_WM_Protocol protocol,
                           Eina_Bool on)
{
   Atom *protos = NULL;
   Atom proto;
   int protos_count = 0;
   int already_set = 0;
   int i;

   /* Check for invalid values */
   if (protocol >= ECORE_X_WM_PROTOCOL_NUM)
     return;

   LOGFN;
   proto = _ecore_x_atoms_wm_protocols[protocol];

   if (!XGetWMProtocols(_ecore_x_disp, win, &protos, &protos_count))
     {
        protos = NULL;
        protos_count = 0;
     }
   if (_ecore_xlib_sync) ecore_x_sync();
   for (i = 0; i < protos_count; i++)
     {
        if (protos[i] == proto)
          {
             already_set = 1;
             break;
          }
     }

   if (on)
     {
        Atom *new_protos = NULL;

        if (already_set)
          goto leave;

        new_protos = malloc((protos_count + 1) * sizeof(Atom));
        if (!new_protos)
          goto leave;

        for (i = 0; i < protos_count; i++)
          new_protos[i] = protos[i];
        new_protos[protos_count] = proto;
        XSetWMProtocols(_ecore_x_disp, win, new_protos, protos_count + 1);
        if (_ecore_xlib_sync) ecore_x_sync();
        free(new_protos);
     }
   else
     {
        if (!already_set)
          goto leave;

        for (i = 0; i < protos_count; i++)
          {
             if (protos[i] == proto)
               {
                  int j;

                  for (j = i + 1; j < protos_count; j++)
                    protos[j - 1] = protos[j];
                  if (protos_count > 1)
                    XSetWMProtocols(_ecore_x_disp, win, protos,
                                    protos_count - 1);
                  else
                    XDeleteProperty(_ecore_x_disp, win,
                                    ECORE_X_ATOM_WM_PROTOCOLS);
                  if (_ecore_xlib_sync) ecore_x_sync();

                  goto leave;
               }
          }
     }

leave:
   if (protos)
     XFree(protos);
}

/**
 * @brief Checks if a specific WM protocol is set for a window.
 * @param win The window to check.
 * @param protocol The Ecore_X_WM_Protocol to query.
 *                 Example: ECORE_X_WM_PROTOCOL_TAKE_FOCUS
 * @return EINA_TRUE if the protocol is set for the window, EINA_FALSE otherwise.
 * @since 1.2
 */
EAPI Eina_Bool
ecore_x_icccm_protocol_isset(Ecore_X_Window win,
                             Ecore_X_WM_Protocol protocol)
{
   Atom proto, *protos = NULL;
   int i, protos_count = 0;
   Eina_Bool ret = EINA_FALSE;

   /* check for invalid values */
   if (protocol >= ECORE_X_WM_PROTOCOL_NUM)
     return EINA_FALSE;

   LOGFN;
   proto = _ecore_x_atoms_wm_protocols[protocol];

   if (!XGetWMProtocols(_ecore_x_disp, win, &protos, &protos_count))
     return EINA_FALSE;
   if (_ecore_xlib_sync) ecore_x_sync();

   for (i = 0; i < protos_count; i++)
     if (protos[i] == proto)
       {
          ret = EINA_TRUE;
          break;
       }

   if (protos)
     XFree(protos);

   return ret;
}

/**
 * @brief Sets the WM_CLASS property for a window.
 * @param win The window for which to set the name and class.
 * @param n The resource name string (e.g., "myApp").
 * @param c The resource class string (e.g., "MyAppClass").
 * @since 1.2
 *
 * The WM_CLASS property contains two strings: the resource name
 * and the resource class. These are used by window managers and
 * session managers to identify and group windows.
 */
EAPI void
ecore_x_icccm_name_class_set(Ecore_X_Window win,
                             const char *n,
                             const char *c)
{
   XClassHint *xch;

   xch = XAllocClassHint();
   if (!xch)
     return;

   LOGFN;
   xch->res_name = (char *)n;
   xch->res_class = (char *)c;
   XSetClassHint(_ecore_x_disp, win, xch);
   if (_ecore_xlib_sync) ecore_x_sync();
   XFree(xch);
}

/**
 * @brief Retrieves the WM_CLASS property for a window.
 * @param win The window from which to get the name and class.
 * @param n Pointer to store the resource name string. The caller
 *          is responsible for freeing this string if not NULL.
 * @param c Pointer to store the resource class string. The caller
 *          is responsible for freeing this string if not NULL.
 * @since 1.2
 *
 * Retrieves the resource name and class for the window.
 * If @p n or @p c are not NULL, they will be set to newly allocated
 * strings containing the respective values, or NULL if not set.
 */
EAPI void
ecore_x_icccm_name_class_get(Ecore_X_Window win,
                             char **n,
                             char **c)
{
   XClassHint xch;

   LOGFN;
   if (n)
     *n = NULL;

   if (c)
     *c = NULL;

   xch.res_name = NULL;
   xch.res_class = NULL;
   if (XGetClassHint(_ecore_x_disp, win, &xch))
     {
        if (n)
          if (xch.res_name)
            *n = strdup(xch.res_name);

        if (c)
          if (xch.res_class)
            *c = strdup(xch.res_class);

        XFree(xch.res_name);
        XFree(xch.res_class);
     }
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the WM_CLIENT_MACHINE property for a window.
 * @param win The window from which to get the client machine string.
 * @return A newly allocated string containing the client machine name,
 *         or NULL if the property is not set or cannot be retrieved.
 *         The caller is responsible for freeing this string.
 * @since 1.2
 *
 * This property stores the hostname of the machine where the client
 * application is running.
 */
EAPI char *
ecore_x_icccm_client_machine_get(Ecore_X_Window win)
{
   char *name;

   LOGFN;
   name = ecore_x_window_prop_string_get(win, ECORE_X_ATOM_WM_CLIENT_MACHINE);
   return name;
}

/**
 * @brief Sets the WM_COMMAND property for a window.
 * @param win The window for which to set the command.
 * @param argc The number of arguments in @p argv.
 * @param argv An array of strings representing the command and its arguments.
 *             Example: `argv` could be `{"my_app", "--file", "/path/to/file.txt"}`
 *             with `argc = 3`. The strings in `argv` are copied.
 * @since 1.2
 *
 * This property stores the command used to start the application,
 * which can be used by session managers to restart the application.
 */
EAPI void
ecore_x_icccm_command_set(Ecore_X_Window win,
                          int argc,
                          char **argv)
{
   LOGFN;
   XSetCommand(_ecore_x_disp, win, argv, argc);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the WM_COMMAND property for a window.
 * @param win The window from which to get the command.
 * @param argc Pointer to store the number of arguments. Can be NULL.
 * @param argv Pointer to store the array of argument strings.
 *             If not NULL, `*argv` will be set to a newly allocated array
 *             of newly allocated strings. The caller is responsible for
 *             freeing each string in `*argv` and then `*argv` itself.
 *             Example: `*argv` might be `{"my_app", "-o", "output.log"}`.
 * @since 1.2
 *
 * This function retrieves the command and arguments used to start the
 * application associated with the window.
 */
EAPI void
ecore_x_icccm_command_get(Ecore_X_Window win,
                          int *argc,
                          char ***argv)
{
   int i, c;
   char **v;
   Eina_Bool success;

   if (argc)
     *argc = 0;

   if (argv)
     *argv = NULL;

   LOGFN;
   success = XGetCommand(_ecore_x_disp, win, &v, &c);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success) return;

   if (c < 1)
     {
        if (v)
          XFreeStringList(v);

        return;
     }

   if (argc)
     *argc = c;

   if (argv)
     {
        (*argv) = malloc(c * sizeof(char *));
        if (!*argv)
          {
             XFreeStringList(v);
             if (argc)
               *argc = 0;

             return;
          }

        for (i = 0; i < c; i++)
          {
             if (v[i])
               (*argv)[i] = strdup(v[i]);
             else
               (*argv)[i] = strdup("");
          }
     }

   XFreeStringList(v);
}

/**
 * @brief Sets the WM_ICON_NAME property for a window.
 * @param win The window for which to set the icon name.
 * @param t The icon name string. Must be UTF-8 encoded.
 * @since 1.2
 *
 * This function sets the name that should be displayed with the
 * window's icon. It attempts to use UTF-8 encoding.
 */
EAPI void
ecore_x_icccm_icon_name_set(Ecore_X_Window win,
                            const char *t)
{
   char *list[1];
   XTextProperty xprop;
   int ret;

   LOGFN;
   xprop.value = NULL;
#ifdef X_HAVE_UTF8_STRING
   list[0] = strdup(t);
   ret = Xutf8TextListToTextProperty(_ecore_x_disp, list, 1,
                                     XUTF8StringStyle, &xprop);
#else /* ifdef X_HAVE_UTF8_STRING */
   list[0] = strdup(t);
   ret = XmbTextListToTextProperty(_ecore_x_disp, list, 1,
                                   XStdICCTextStyle, &xprop);
#endif /* ifdef X_HAVE_UTF8_STRING */
   if (_ecore_xlib_sync) ecore_x_sync();
   if (ret >= Success)
     {
        XSetWMIconName(_ecore_x_disp, win, &xprop);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          XFree(xprop.value);
     }
   else if (XStringListToTextProperty(list, 1, &xprop) >= Success)
     {
        XSetWMIconName(_ecore_x_disp, win, &xprop);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          XFree(xprop.value);
     }

   free(list[0]);
}

/**
 * @brief Retrieves the WM_ICON_NAME property for a window.
 * @param win The window from which to get the icon name.
 * @return A newly allocated string containing the icon name,
 *         or NULL if the property is not set or cannot be retrieved.
 *         The caller is responsible for freeing this string.
 *         The returned string is UTF-8 encoded.
 * @since 1.2
 */
EAPI char *
ecore_x_icccm_icon_name_get(Ecore_X_Window win)
{
   XTextProperty xprop;

   LOGFN;
   xprop.value = NULL;
   if (XGetWMIconName(_ecore_x_disp, win, &xprop) >= Success)
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        if (xprop.value)
          {
             char **list = NULL;
             char *t = NULL;
             int num = 0;
             int ret;

             if (xprop.encoding == ECORE_X_ATOM_UTF8_STRING)
               t = strdup((char *)xprop.value);
             else
               {
                  /* convert to utf8 */
#ifdef X_HAVE_UTF8_STRING
                  ret = Xutf8TextPropertyToTextList(_ecore_x_disp, &xprop,
                                                    &list, &num);
#else /* ifdef X_HAVE_UTF8_STRING */
                  ret = XmbTextPropertyToTextList(_ecore_x_disp, &xprop,
                                                  &list, &num);
#endif /* ifdef X_HAVE_UTF8_STRING */
                  if (_ecore_xlib_sync) ecore_x_sync();

                  if ((ret == XLocaleNotSupported) ||
                      (ret == XNoMemory) || (ret == XConverterNotFound))
                    t = strdup((char *)xprop.value);
                  else if (ret >= Success)
                    {
                       if ((num >= 1) && (list))
                         t = strdup(list[0]);

                       if (list)
                         XFreeStringList(list);
                    }
               }

             if (xprop.value)
               XFree(xprop.value);

             return t;
          }
     }
   else
     {
        if (_ecore_xlib_sync) ecore_x_sync();
     }

   return NULL;
}

/**
 * @brief Adds a subwindow to the WM_COLORMAP_WINDOWS property of a top-level window.
 * @param win The top-level window whose WM_COLORMAP_WINDOWS property will be modified.
 * @param subwin The subwindow that requires its own colormap to be installed.
 * @since 1.2
 *
 * This function informs the window manager that @p subwin (which should be
 * a child of @p win) has a colormap different from @p win and may need
 * special handling when @p win gets focus. The list of such subwindows
 * is stored in the WM_COLORMAP_WINDOWS property on @p win.
 */
EAPI void
ecore_x_icccm_colormap_window_set(Ecore_X_Window win,
                                  Ecore_X_Window subwin)
{
   int num = 0, i;
   unsigned char *old_data = NULL;
   unsigned char *data = NULL;
   Window *oldset = NULL;
   Window *newset = NULL;

   LOGFN;
   if (!ecore_x_window_prop_property_get(win,
                                         ECORE_X_ATOM_WM_COLORMAP_WINDOWS,
                                         XA_WINDOW, 32, &old_data, &num))
     {
        newset = calloc(1, sizeof(Window));
        if (!newset)
          {
             if (old_data) free(old_data);
             return;
          }

        newset[0] = subwin;
        num = 1;
        data = (unsigned char *)newset;
     }
   else
     {
        newset = calloc(num + 1, sizeof(Window));
        oldset = (Window *)old_data;
        if (!newset)
          {
             if (old_data) free(old_data);
             return;
          }

        for (i = 0; i < num; ++i)
          {
             if (oldset[i] == subwin)
               {
                  free(old_data);
                  free(newset);
                  return;
               }

             newset[i] = oldset[i];
          }

        newset[num++] = subwin;
        data = (unsigned char *)newset;
     }

   ecore_x_window_prop_property_set(win,
                                    ECORE_X_ATOM_WM_COLORMAP_WINDOWS,
                                    XA_WINDOW, 32, data, num);
   free(newset);
   free(old_data);
}

/**
 * @brief Removes a subwindow from the WM_COLORMAP_WINDOWS property of a top-level window.
 * @param win The top-level window whose WM_COLORMAP_WINDOWS property will be modified.
 * @param subwin The subwindow to be removed from the list.
 * @since 1.2
 *
 * This function removes @p subwin from the list of windows in the
 * WM_COLORMAP_WINDOWS property of @p win.
 */
EAPI void
ecore_x_icccm_colormap_window_unset(Ecore_X_Window win,
                                    Ecore_X_Window subwin)
{
   int num = 0, i, j, k = 0;
   unsigned char *old_data = NULL;
   unsigned char *data = NULL;
   Window *oldset = NULL;
   Window *newset = NULL;

   LOGFN;
   if (!ecore_x_window_prop_property_get(win,
                                         ECORE_X_ATOM_WM_COLORMAP_WINDOWS,
                                         XA_WINDOW, 32, &old_data, &num))
     {
        if (old_data) free(old_data);
        return;
     }

   oldset = (Window *)old_data;
   for (i = 0; i < num; i++)
     {
        if (oldset[i] == subwin)
          {
             if (num == 1)
               {
                  XDeleteProperty(_ecore_x_disp,
                                  win, ECORE_X_ATOM_WM_COLORMAP_WINDOWS);
                  if (_ecore_xlib_sync) ecore_x_sync();
                  free(old_data);

                  old_data = NULL;
                  return;
               }
             else
               {
                  newset = calloc(num - 1, sizeof(Window));
                  data = (unsigned char *)newset;
                  for (j = 0; j < num; ++j)
                    if (oldset[j] != subwin)
                      newset[k++] = oldset[j];

                  ecore_x_window_prop_property_set(
                    win,
                    ECORE_X_ATOM_WM_COLORMAP_WINDOWS,
                    XA_WINDOW,
                    32,
                    data,
                    k);
                  free(old_data);

                  old_data = NULL;
                  free(newset);
                  return;
               }
          }
     }

   if (old_data)
     free(old_data);
}

/**
 * @brief Sets the WM_TRANSIENT_FOR hint for a window.
 * @param win The window that is transient (e.g., a dialog box).
 * @param forwin The main top-level window for which @p win is transient.
 * @since 1.2
 *
 * This hint indicates to the window manager that @p win is a temporary
 * window (like a dialog) associated with @p forwin. Window managers
 * may use this to keep transient windows above their main windows,
 * or to close them when the main window is closed.
 */
EAPI void
ecore_x_icccm_transient_for_set(Ecore_X_Window win,
                                Ecore_X_Window forwin)
{
   LOGFN;
   XSetTransientForHint(_ecore_x_disp, win, forwin);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Removes the WM_TRANSIENT_FOR hint from a window.
 * @param win The window from which to remove the hint.
 * @since 1.2
 */
EAPI void
ecore_x_icccm_transient_for_unset(Ecore_X_Window win)
{
   LOGFN;
   XDeleteProperty(_ecore_x_disp, win, ECORE_X_ATOM_WM_TRANSIENT_FOR);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Retrieves the WM_TRANSIENT_FOR hint for a window.
 * @param win The window to check.
 * @return The Ecore_X_Window ID of the window for which @p win is transient,
 *         or 0 if the hint is not set or cannot be retrieved.
 * @since 1.2
 */
EAPI Ecore_X_Window
ecore_x_icccm_transient_for_get(Ecore_X_Window win)
{
   Window forwin;
   Eina_Bool success;

   LOGFN;
   success = XGetTransientForHint(_ecore_x_disp, win, &forwin);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (success)
     return (Ecore_X_Window)forwin;
   else
     return 0;
}

/**
 * @brief Sets the WM_WINDOW_ROLE hint for a window.
 * @param win The window for which to set the role.
 * @param role A string defining the role of the window (e.g., "browser", "editor").
 * @since 1.2
 *
 * The WM_WINDOW_ROLE hint can be used by session managers or window managers
 * to identify windows with specific functionalities, aiding in session restoration
 * or specialized window handling.
 */
EAPI void
ecore_x_icccm_window_role_set(Ecore_X_Window win,
                              const char *role)
{
   LOGFN;
   ecore_x_window_prop_string_set(win, ECORE_X_ATOM_WM_WINDOW_ROLE,
                                  (char *)role);
}

/**
 * @brief Retrieves the WM_WINDOW_ROLE hint for a window.
 * @param win The window from which to get the role.
 * @return A newly allocated string containing the window's role,
 *         or NULL if the hint is not set or cannot be retrieved.
 *         The caller is responsible for freeing this string.
 * @since 1.2
 */
EAPI char *
ecore_x_icccm_window_role_get(Ecore_X_Window win)
{
   LOGFN;
   return ecore_x_window_prop_string_get(win, ECORE_X_ATOM_WM_WINDOW_ROLE);
}

/**
 * @brief Sets the WM_CLIENT_LEADER hint for a window.
 * @param win The window for which to set the client leader. This is typically
 *            a secondary top-level window of an application.
 * @param l The Ecore_X_Window ID of the main window of the application,
 *          which acts as the client leader.
 * @since 1.2
 *
 * The WM_CLIENT_LEADER hint is used to group related top-level windows
 * of an application. All non-transient, top-level windows created by an
 * application, other than its main window, should have this property set
 * to point to the application's main window.
 */
EAPI void
ecore_x_icccm_client_leader_set(Ecore_X_Window win,
                                Ecore_X_Window l)
{
   LOGFN;
   ecore_x_window_prop_window_set(win, ECORE_X_ATOM_WM_CLIENT_LEADER,
                                  &l, 1);
}

/**
 * @brief Retrieves the WM_CLIENT_LEADER hint for a window.
 * @param win The window to check.
 * @return The Ecore_X_Window ID of the client leader window,
 *         or 0 if the hint is not set or cannot be retrieved.
 * @since 1.2
 */
EAPI Ecore_X_Window
ecore_x_icccm_client_leader_get(Ecore_X_Window win)
{
   Ecore_X_Window l;

   LOGFN;
   if (ecore_x_window_prop_window_get(win, ECORE_X_ATOM_WM_CLIENT_LEADER,
                                      &l, 1) > 0)
     return l;

   return 0;
}

/**
 * @brief Sends a client message to request that a window be iconified.
 * @param win The window to be iconified.
 * @param root The root window. If 0, the default root window of the
 *             display is used.
 * @since 1.2
 *
 * This function sends a WM_CHANGE_STATE client message to the root window,
 * requesting that the specified window (@p win) be changed to the IconicState.
 * This is typically used by applications to request their own iconification.
 */
EAPI void
ecore_x_icccm_iconic_request_send(Ecore_X_Window win,
                                  Ecore_X_Window root)
{
   XEvent xev = { 0 };

   if (!win)
     return;

   LOGFN;
   if (!root)
     root = DefaultRootWindow(_ecore_x_disp);

   xev.xclient.type = ClientMessage;
   xev.xclient.serial = 0;
   xev.xclient.send_event = True;
   xev.xclient.display = _ecore_x_disp;
   xev.xclient.window = win;
   xev.xclient.format = 32;
   xev.xclient.message_type = ECORE_X_ATOM_WM_CHANGE_STATE;
   xev.xclient.data.l[0] = IconicState;

   XSendEvent(_ecore_x_disp, root, False,
              SubstructureNotifyMask | SubstructureRedirectMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/* FIXME: there are older E hints, gnome hints and mwm hints and new netwm */
/*        hints. each should go in their own file/section so we know which */
/*        is which. also older kde hints too. we should try support as much */
/*        as makese sense to support */
