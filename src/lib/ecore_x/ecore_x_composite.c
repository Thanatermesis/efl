#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "ecore_x_private.h"
#include "Ecore_X.h"

/**
 * @internal
 * @brief Flag indicating if the X Composite extension is available.
 *
 * This variable is set during initialization by _ecore_x_composite_init().
 * It should not be accessed directly by applications. Use
 * ecore_x_composite_query() instead.
 */
static Eina_Bool _composite_available = EINA_FALSE;

/**
 * @internal
 * @brief Initializes the X Composite extension interface.
 *
 * This function checks for the availability of the X Composite, X Render,
 * and X Fixes extensions. If all are present, it sets the internal
 * _composite_available flag to EINA_TRUE.
 * This function is called internally during Ecore_X initialization.
 */
void
_ecore_x_composite_init(void)
{
   _composite_available = EINA_FALSE;

#ifdef ECORE_XCOMPOSITE
   int major, minor;

   if (XCompositeQueryVersion(_ecore_x_disp, &major, &minor))
     {
        if (_ecore_xlib_sync) ecore_x_sync();
# ifdef ECORE_XRENDER
        if (XRenderQueryExtension(_ecore_x_disp, &major, &minor))
          {
             if (_ecore_xlib_sync) ecore_x_sync();
#  ifdef ECORE_XFIXES
             if (XFixesQueryVersion(_ecore_x_disp, &major, &minor))
               {
                  if (_ecore_xlib_sync) ecore_x_sync();
                  _composite_available = EINA_TRUE;
               }
#  endif
          }
# endif
     }
#endif
}

/**
 * @brief Checks if the X Composite extension is available.
 *
 * @return @c EINA_TRUE if the Composite extension is available,
 *         @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_composite_query(void)
{
   LOGFN;
   return _composite_available;
}

/**
 * @brief Redirects drawing of a window and its children to an off-screen buffer.
 *
 * This function tells the X server to redirect the rendering of the specified
 * window @p win to an off-screen pixmap. The content of this pixmap can then
 * be accessed using ecore_x_composite_name_window_pixmap_get().
 *
 * @param win The window to redirect.
 * @param type The update type for the redirection.
 *             - @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC: The server automatically
 *               updates the pixmap when the window content changes.
 *             - @c ECORE_X_COMPOSITE_UPDATE_MANUAL: The application is responsible
 *               for updating the pixmap (not typically used with this function).
 */
EAPI void
ecore_x_composite_redirect_window(Ecore_X_Window win,
                                  Ecore_X_Composite_Update_Type type)
{
#ifdef ECORE_XCOMPOSITE
   int update = CompositeRedirectAutomatic;

   LOGFN;
   switch (type)
     {
      case ECORE_X_COMPOSITE_UPDATE_AUTOMATIC:
        update = CompositeRedirectAutomatic;
        break;

      case ECORE_X_COMPOSITE_UPDATE_MANUAL:
        update = CompositeRedirectManual;
        break;
     }
   XCompositeRedirectWindow(_ecore_x_disp, win, update);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Redirects drawing of all subwindows of a given window to off-screen buffers.
 *
 * This function tells the X server to redirect the rendering of all child
 * windows of the specified window @p win to their respective off-screen pixmaps.
 *
 * @param win The parent window whose subwindows are to be redirected.
 * @param type The update type for the redirection.
 *             - @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC: The server automatically
 *               updates the pixmaps when the subwindows' content changes.
 *             - @c ECORE_X_COMPOSITE_UPDATE_MANUAL: The application is responsible
 *               for updating the pixmaps.
 */
EAPI void
ecore_x_composite_redirect_subwindows(Ecore_X_Window win,
                                      Ecore_X_Composite_Update_Type type)
{
#ifdef ECORE_XCOMPOSITE
   int update = CompositeRedirectAutomatic;

   LOGFN;
   switch (type)
     {
      case ECORE_X_COMPOSITE_UPDATE_AUTOMATIC:
        update = CompositeRedirectAutomatic;
        break;

      case ECORE_X_COMPOSITE_UPDATE_MANUAL:
        update = CompositeRedirectManual;
        break;
     }
   XCompositeRedirectSubwindows(_ecore_x_disp, win, update);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Stops redirecting the drawing of a window to an off-screen buffer.
 *
 * This function tells the X server to stop redirecting the rendering of the
 * specified window @p win. The window will resume drawing directly to the screen.
 *
 * @param win The window to unredirect.
 * @param type The update type (parameter is largely historical for this call,
 *             typically @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC is used).
 *             - @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC
 *             - @c ECORE_X_COMPOSITE_UPDATE_MANUAL
 */
EAPI void
ecore_x_composite_unredirect_window(Ecore_X_Window win,
                                    Ecore_X_Composite_Update_Type type)
{
#ifdef ECORE_XCOMPOSITE
   int update = CompositeRedirectAutomatic;

   LOGFN;
   switch (type)
     {
      case ECORE_X_COMPOSITE_UPDATE_AUTOMATIC:
        update = CompositeRedirectAutomatic;
        break;

      case ECORE_X_COMPOSITE_UPDATE_MANUAL:
        update = CompositeRedirectManual;
        break;
     }
   XCompositeUnredirectWindow(_ecore_x_disp, win, update);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Stops redirecting the drawing of subwindows of a given window.
 *
 * This function tells the X server to stop redirecting the rendering of all
 * child windows of the specified window @p win. The subwindows will resume
 * drawing directly.
 *
 * @param win The parent window whose subwindows are to be unredirected.
 * @param type The update type (parameter is largely historical for this call,
 *             typically @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC is used).
 *             - @c ECORE_X_COMPOSITE_UPDATE_AUTOMATIC
 *             - @c ECORE_X_COMPOSITE_UPDATE_MANUAL
 */
EAPI void
ecore_x_composite_unredirect_subwindows(Ecore_X_Window win,
                                        Ecore_X_Composite_Update_Type type)
{
#ifdef ECORE_XCOMPOSITE
   int update = CompositeRedirectAutomatic;

   LOGFN;
   switch (type)
     {
      case ECORE_X_COMPOSITE_UPDATE_AUTOMATIC:
        update = CompositeRedirectAutomatic;
        break;

      case ECORE_X_COMPOSITE_UPDATE_MANUAL:
        update = CompositeRedirectManual;
        break;
     }
   XCompositeUnredirectSubwindows(_ecore_x_disp, win, update);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Gets the off-screen pixmap for a redirected window.
 *
 * After a window has been redirected using ecore_x_composite_redirect_window(),
 * this function retrieves the X Pixmap ID of the off-screen buffer containing
 * the window's contents.
 *
 * @param win The redirected window.
 * @return The X Pixmap ID, or @c None if the window is not redirected or
 *         an error occurs. The caller does not own this pixmap and should not
 *         free it directly with XFreePixmap; it is managed by the X server
 *         as part of the redirection.
 */
EAPI Ecore_X_Pixmap
ecore_x_composite_name_window_pixmap_get(Ecore_X_Window win)
{
   Ecore_X_Pixmap pixmap = None;
#ifdef ECORE_XCOMPOSITE
   LOGFN;
   pixmap = XCompositeNameWindowPixmap(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
   return pixmap;
}

/**
 * @brief Disables input events for a composite overlay window.
 *
 * This function makes the specified window @p win (typically an overlay window
 * obtained from ecore_x_composite_render_window_enable()) transparent to
 * mouse and keyboard events. It does this by setting a 1x1 input shape
 * at an off-screen coordinate.
 *
 * @param win The composite overlay window.
 */
EAPI void
ecore_x_composite_window_events_disable(Ecore_X_Window win)
{
#ifdef ECORE_XCOMPOSITE
   ecore_x_window_shape_input_rectangle_set(win, -1, -1, 1, 1);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Enables input events for a composite overlay window.
 *
 * This function makes the specified window @p win (typically an overlay window)
 * receive mouse and keyboard events. It does this by setting a large input
 * shape covering the typical screen area.
 *
 * @param win The composite overlay window.
 */
EAPI void
ecore_x_composite_window_events_enable(Ecore_X_Window win)
{
#ifdef ECORE_XCOMPOSITE
   ecore_x_window_shape_input_rectangle_set(win, 0, 0, 65535, 65535);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

/**
 * @brief Enables the composite overlay window for a given root window.
 *
 * This function retrieves or creates the X Composite overlay window associated
 * with the given @p root window. This overlay window can be used by compositing
 * managers to draw effects without interfering with the actual window contents.
 * Input events are typically disabled on this window after creation using
 * ecore_x_composite_window_events_disable().
 *
 * @param root The root window for which to get the overlay window.
 * @return The Ecore_X_Window ID of the overlay window, or 0 on failure.
 */
EAPI Ecore_X_Window
ecore_x_composite_render_window_enable(Ecore_X_Window root)
{
   Ecore_X_Window win = 0;
#ifdef ECORE_XCOMPOSITE
   win = XCompositeGetOverlayWindow(_ecore_x_disp, root);
   if (_ecore_xlib_sync) ecore_x_sync();
   ecore_x_composite_window_events_disable(win);
#endif /* ifdef ECORE_XCOMPOSITE */
   return win;
}

/**
 * @brief Disables (releases) the composite overlay window for a given root window.
 *
 * This function releases the X Composite overlay window associated with the
 * given @p root window. This should be called when a compositing manager
 * no longer needs the overlay window.
 *
 * @param root The root window for which to release the overlay window.
 */
EAPI void
ecore_x_composite_render_window_disable(Ecore_X_Window root)
{
#ifdef ECORE_XCOMPOSITE
   LOGFN;
   XCompositeReleaseOverlayWindow(_ecore_x_disp, root);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif /* ifdef ECORE_XCOMPOSITE */
}

