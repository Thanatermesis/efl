#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"
#include "efl-hints-client-protocol.h"

static void _ecore_wl2_window_hide_send(Ecore_Wl2_Window *window);

/**
 * @internal
 * @brief Partially frees resources associated with a window.
 *
 * This function destroys various Wayland objects (xdg_popup, xdg_toplevel,
 * xdg_surface, zxdg equivalents, wl_surface, wl_callback) and frees
 * the list of outputs. It does not free the Ecore_Wl2_Window structure
 * itself, making it "semi" free. It's typically called as part of the
 * full window destruction process or when a window is hidden and its
 * surface-related resources can be released.
 *
 * @param window The window whose resources are to be partially freed.
 */
void
_ecore_wl2_window_semi_free(Ecore_Wl2_Window *window)
{
   if (window->xdg_popup) xdg_popup_destroy(window->xdg_popup);
   window->xdg_popup = NULL;

   if (window->xdg_toplevel) xdg_toplevel_destroy(window->xdg_toplevel);
   window->xdg_toplevel = NULL;

   if (window->xdg_surface) xdg_surface_destroy(window->xdg_surface);
   window->xdg_surface = NULL;

   if (window->zxdg_popup) zxdg_popup_v6_destroy(window->zxdg_popup);
   window->zxdg_popup = NULL;

   if (window->zxdg_toplevel) zxdg_toplevel_v6_destroy(window->zxdg_toplevel);
   window->zxdg_toplevel = NULL;

   if (window->zxdg_surface) zxdg_surface_v6_destroy(window->zxdg_surface);
   window->zxdg_surface = NULL;

   if (window->surface) wl_surface_destroy(window->surface);
   window->surface = NULL;
   window->surface_id = -1;

   if (window->callback) wl_callback_destroy(window->callback);
   window->callback = NULL;

   window->outputs = eina_list_free(window->outputs);

   ecore_wl2_window_surface_flush(window, EINA_TRUE);

   window->commit_pending = EINA_FALSE;
}

/**
 * @internal
 * @brief Sends a window activation event.
 *
 * This function allocates and populates an Ecore_Wl2_Event_Window_Activate
 * event structure and adds it to the Ecore event queue. This typically
 * signifies that the window has gained focus or has been brought to the
 * foreground.
 *
 * @param window The window that has been activated.
 */
static void
_ecore_wl2_window_activate_send(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Event_Window_Activate *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Activate));
   if (!ev) return;

   ev->win = window;
   if (window->parent)
     ev->parent_win = window->parent;
   ev->event_win = window;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_ACTIVATE, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Sends a window deactivation event.
 *
 * This function allocates and populates an Ecore_Wl2_Event_Window_Deactivate
 * event structure and adds it to the Ecore event queue. This typically
 * signifies that the window has lost focus.
 *
 * @param window The window that has been deactivated.
 */
static void
_ecore_wl2_window_deactivate_send(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Event_Window_Deactivate *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Deactivate));
   if (!ev) return;

   ev->win = window;
   if (window->parent)
     ev->parent_win = window->parent;
   ev->event_win = window;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_DEACTIVATE, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Sends a window configure event.
 *
 * This function is called when the compositor has configured the window
 * (e.g., resized, changed state). It creates an Ecore_Wl2_Event_Window_Configure
 * event. The event's width and height (ev->w, ev->h) are determined based on
 * whether the window's current dimensions match the "set" dimensions (requested by client)
 * or if the window is transitioning from/to fullscreen/maximized states without
 * explicit dimensions, in which case saved dimensions are used.
 * It also updates the window's requested configuration to match the
 * current definitive configuration and sends activate/deactivate events
 * based on the focused state.
 *
 * @param win The window that has been configured.
 */
static void
_ecore_wl2_window_configure_send(Ecore_Wl2_Window *win)
{
   Ecore_Wl2_Event_Window_Configure *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Configure));
   if (!ev) return;

   ev->win = win;
   ev->event_win = win;

   if ((win->set_config.geometry.w == win->def_config.geometry.w) &&
       (win->set_config.geometry.h == win->def_config.geometry.h))
     ev->w = ev->h = 0;
   else if ((!win->def_config.geometry.w) && (!win->def_config.geometry.h) &&
            (!win->def_config.fullscreen) &&
            (!win->def_config.maximized) &&
            ((win->def_config.fullscreen != win->req_config.fullscreen) ||
             (win->def_config.maximized != win->req_config.maximized)))
     ev->w = win->saved.w, ev->h = win->saved.h;
   else
     ev->w = win->def_config.geometry.w, ev->h = win->def_config.geometry.h;

   ev->edges = !!win->def_config.resizing;
   if (win->def_config.fullscreen)
     ev->states |= ECORE_WL2_WINDOW_STATE_FULLSCREEN;
   if (win->def_config.maximized)
     ev->states |= ECORE_WL2_WINDOW_STATE_MAXIMIZED;

   win->req_config = win->def_config;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE, ev, NULL, NULL);

   if (win->def_config.focused)
     _ecore_wl2_window_activate_send(win);
   else
     _ecore_wl2_window_deactivate_send(win);
}

/**
 * @internal
 * @brief Sends a window configure complete event.
 *
 * This function is called after a window configuration sequence is fully
 * processed, particularly after the initial configure. It marks the window's
 * pending configure state as false and sends an
 * ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE event.
 *
 * @param window The window for which configuration is complete.
 */
static void
_configure_complete(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Event_Window_Configure_Complete *ev;

   window->pending.configure = EINA_FALSE;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Configure_Complete));
   if (!ev) return;

   ev->win = window;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE, ev, NULL, NULL);

}

#include "window_v6.x"

/// @cond INTERNALS_DOXYGEN
/**
 * @internal
 * @brief Callback for xdg_surface.configure events.
 *
 * Triggered by the compositor to suggest a new configuration for the xdg_surface.
 * This function updates the window's definitive configuration serial.
 * If the window was pending a configure and is now updating (rendering),
 * it logs an error as rendering shouldn't happen before initial configure.
 * If the window is not currently updating its visuals, it sends an
 * Ecore_Wl2_Event_Window_Configure event. If the window was pending an
 * initial configure, it also sends a configure_complete event.
 *
 * @param data The Ecore_Wl2_Window associated with this xdg_surface.
 * @param xdg_surface The xdg_surface object.
 * @param serial The serial of this configure event, to be acked.
 */
static void
_xdg_surface_cb_configure(void *data, struct xdg_surface *xdg_surface EINA_UNUSED, uint32_t serial)
{
   Ecore_Wl2_Window *window;

   window = data;
   window->def_config.serial = serial;

   if (window->pending.configure)
     {
        window->saved.w = window->set_config.geometry.w;
        window->saved.h = window->set_config.geometry.h;
     }
   if (window->pending.configure && window->updating)
     ERR("Window shouldn't be rendering before initial configure");

   if (!window->updating)
     _ecore_wl2_window_configure_send(window);

   if (window->pending.configure)
     _configure_complete(window);
}

static const struct xdg_surface_listener _xdg_surface_listener =
{
   _xdg_surface_cb_configure,
};
/// @endcond

/// @cond INTERNALS_DOXYGEN
/**
 * @internal
 * @brief Callback for xdg_toplevel.configure events.
 *
 * Triggered by the compositor to suggest a new configuration for the xdg_toplevel,
 * including size and states (maximized, fullscreen, focused, resizing).
 * This function updates the window's definitive configuration based on the
 * provided width, height, and states.
 *
 * @param data The Ecore_Wl2_Window associated with this xdg_toplevel.
 * @param xdg_toplevel The xdg_toplevel object.
 * @param width The suggested new width. 0 means client chooses.
 * @param height The suggested new height. 0 means client chooses.
 * @param states A wl_array of xdg_toplevel_state enum values.
 *        Example states:
 *        - ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED
 *        - ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN
 *        - ZXDG_TOPLEVEL_V6_STATE_RESIZING
 *        - ZXDG_TOPLEVEL_V6_STATE_ACTIVATED
 */
static void
_xdg_toplevel_cb_configure(void *data, struct xdg_toplevel *xdg_toplevel EINA_UNUSED, int32_t width, int32_t height, struct wl_array *states)
{
   Ecore_Wl2_Window *win = data;
   uint32_t *s;

   win->def_config.maximized = EINA_FALSE;
   win->def_config.fullscreen = EINA_FALSE;
   win->def_config.focused = EINA_FALSE;
   win->def_config.resizing = EINA_FALSE;
   win->def_config.geometry.w = width;
   win->def_config.geometry.h = height;

   wl_array_for_each(s, states)
     {
        switch (*s)
          {
           case ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED:
             win->def_config.maximized = EINA_TRUE;
             break;
           case ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN:
             win->def_config.fullscreen = EINA_TRUE;
             break;
           case ZXDG_TOPLEVEL_V6_STATE_RESIZING:
             win->def_config.resizing = EINA_TRUE;
             break;
           case ZXDG_TOPLEVEL_V6_STATE_ACTIVATED:
             win->def_config.focused = EINA_TRUE;
           default:
             break;
          }
     }
}

/**
 * @internal
 * @brief Callback for xdg_toplevel.close events.
 *
 * Triggered by the compositor when it requests the toplevel window to be closed.
 * This function calls the user-provided close callback (if set) and then
 * frees the Ecore_Wl2_Window.
 *
 * @param data The Ecore_Wl2_Window associated with this xdg_toplevel.
 * @param xdg_toplevel The xdg_toplevel object.
 */
static void
_xdg_toplevel_cb_close(void *data, struct xdg_toplevel *xdg_toplevel EINA_UNUSED)
{
   Ecore_Wl2_Window *win;

   win = data;
   if (!win) return;
   if (win->cb_close)
     {
        win->cb_close(win->cb_close_data, win);
        win->cb_close = NULL;
     }
   ecore_wl2_window_free(win);
}

static const struct xdg_toplevel_listener _xdg_toplevel_listener =
{
   _xdg_toplevel_cb_configure,
   _xdg_toplevel_cb_close,
};
/// @endcond

/// @cond INTERNALS_DOXYGEN
/**
 * @internal
 * @brief Callback for xdg_popup.configure events.
 *
 * Triggered by the compositor to suggest a new configuration for the xdg_popup,
 * specifically its size. The position (x, y) is relative to the parent surface.
 *
 * @param data The Ecore_Wl2_Window (popup) associated with this xdg_popup.
 * @param xdg_popup The xdg_popup object.
 * @param x The suggested x coordinate relative to the parent surface geometry.
 * @param y The suggested y coordinate relative to the parent surface geometry.
 * @param width The suggested new width.
 * @param height The suggested new height.
 */
static void
_xdg_popup_cb_configure(void *data, struct xdg_popup *xdg_popup EINA_UNUSED, int32_t x EINA_UNUSED, int32_t y EINA_UNUSED, int32_t width, int32_t height)
{
   Ecore_Wl2_Window *win = data;
   win->def_config.geometry.w = width;
   win->def_config.geometry.h = height;
}

/**
 * @internal
 * @brief Callback for xdg_popup.popup_done events.
 *
 * Triggered by the compositor when the popup is dismissed (e.g., user clicked
 * outside, or an explicit grab was broken).
 * This function ungrabs the input if a grab was active for this popup and
 * sends a hide event for the window.
 *
 * @param data The Ecore_Wl2_Window (popup) associated with this xdg_popup.
 * @param xdg_popup The xdg_popup object.
 */
static void
_xdg_popup_cb_done(void *data, struct xdg_popup *xdg_popup EINA_UNUSED)
{
   Ecore_Wl2_Window *win;

   win = data;
   if (!win) return;

   if (win->grab) _ecore_wl2_input_ungrab(win->grab);

   _ecore_wl2_window_hide_send(win);
}

static const struct xdg_popup_listener _xdg_popup_listener =
{
   _xdg_popup_cb_configure,
   _xdg_popup_cb_done,
};
/// @endcond

/**
 * @internal
 * @brief Creates an xdg_popup for a given window.
 *
 * This function is called when a window of type ECORE_WL2_WINDOW_TYPE_MENU
 * (or similar popup types) needs its xdg_popup Wayland object. It uses an
 * xdg_positioner to define the popup's placement relative to its parent.
 * If the window has an associated input grab, it attempts to grab the popup.
 *
 * @param win The Ecore_Wl2_Window for which to create the xdg_popup.
 *            Must have a valid parent window.
 */
static void
_ecore_wl2_window_xdg_popup_create(Ecore_Wl2_Window *win)
{
   int gw, gh;
   struct xdg_positioner *pos;

   EINA_SAFETY_ON_NULL_RETURN(win->parent);
   pos = xdg_wm_base_create_positioner(win->display->wl.xdg_wm_base);
   if (!pos) return;

   ecore_wl2_window_geometry_get(win, NULL, NULL, &gw, &gh);
   xdg_positioner_set_anchor_rect(pos, 0, 0, 1, 1);
   xdg_positioner_set_size(pos, gw, gh);
   xdg_positioner_set_anchor(pos, XDG_POSITIONER_ANCHOR_TOP_LEFT);
   xdg_positioner_set_gravity(pos, ZXDG_POSITIONER_V6_ANCHOR_BOTTOM |
                                  ZXDG_POSITIONER_V6_ANCHOR_RIGHT);

   win->xdg_popup = xdg_surface_get_popup(win->xdg_surface,
                               win->parent->xdg_surface, pos);

   xdg_positioner_destroy(pos);
   if (win->grab)
     xdg_popup_grab(win->xdg_popup, win->grab->wl.seat,
                        wl_display_get_serial(win->display->wl.display));
   xdg_popup_set_user_data(win->xdg_popup, win);
   xdg_popup_add_listener(win->xdg_popup, &_xdg_popup_listener, win);

   win->pending.configure = EINA_TRUE;

   ecore_wl2_window_commit(win, EINA_TRUE);
}

/**
 * @internal
 * @brief Creates the appropriate XDG shell surface (toplevel or popup).
 *
 * This function creates an xdg_surface for the window's wl_surface.
 * Depending on the window type, it then creates either an xdg_toplevel
 * (for regular windows, dialogs, etc.) or an xdg_popup (for menus, tooltips).
 * It sets up listeners for configure and close events, and applies
 * initial properties like title, app_id, parent, minimized/maximized/fullscreen
 * states, and aspect/weight hints if available.
 *
 * @param window The Ecore_Wl2_Window for which to create the shell surface.
 */
static void
_window_shell_surface_create(Ecore_Wl2_Window *window)
{
   if (window->xdg_surface) return;
   window->xdg_surface =
     xdg_wm_base_get_xdg_surface(window->display->wl.xdg_wm_base,
                                   window->surface);
   xdg_surface_set_user_data(window->xdg_surface, window);
   xdg_surface_add_listener(window->xdg_surface,
                                &_xdg_surface_listener, window);

   window->xdg_configure_ack = xdg_surface_ack_configure;
   window->pending.configure = EINA_TRUE;
   if (window->display->wl.efl_hints)
     {
        if (window->aspect.set)
          efl_hints_set_aspect(window->display->wl.efl_hints, window->xdg_surface,
            window->aspect.w, window->aspect.h, window->aspect.aspect);
        if (window->weight.set)
          efl_hints_set_weight(window->display->wl.efl_hints, window->xdg_surface,
            window->weight.w, window->weight.h);
     }

   if (window->type == ECORE_WL2_WINDOW_TYPE_MENU)
     _ecore_wl2_window_xdg_popup_create(window);
   else
     {
        struct xdg_toplevel *ptop = NULL;

        window->xdg_toplevel =
          xdg_surface_get_toplevel(window->xdg_surface);
        xdg_toplevel_set_user_data(window->xdg_toplevel, window);
        xdg_toplevel_add_listener(window->xdg_toplevel,
                                      &_xdg_toplevel_listener, window);

        if (window->deferred_minimize)
          xdg_toplevel_set_minimized(window->xdg_toplevel);
        window->deferred_minimize = EINA_FALSE;

        if (window->title)
          xdg_toplevel_set_title(window->xdg_toplevel, window->title);
        if (window->class)
          xdg_toplevel_set_app_id(window->xdg_toplevel, window->class);

        window->xdg_set_min_size = xdg_toplevel_set_min_size;
        window->xdg_set_max_size = xdg_toplevel_set_max_size;

        if (window->parent)
          ptop = window->parent->xdg_toplevel;

        if (ptop)
          xdg_toplevel_set_parent(window->xdg_toplevel, ptop);

        if (window->set_config.maximized)
          xdg_toplevel_set_maximized(window->xdg_toplevel);

        if (window->set_config.fullscreen)
          xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
     }

   ecore_wl2_window_commit(window, EINA_TRUE);
}

/**
 * @internal
 * @brief Initializes the shell surface for a window.
 *
 * This function ensures that a Wayland shell surface (either XDG or ZXDG_V6)
 * is created for the given window if one doesn't already exist and the
 * display supports the necessary shell protocols. It also handles session
 * recovery UUID setup if supported and applicable.
 *
 * @param window The window for which to initialize the shell surface.
 */
void
_ecore_wl2_window_shell_surface_init(Ecore_Wl2_Window *window)
{
   if (!window->surface) return;
   if (window->display->wl.xdg_wm_base) _window_shell_surface_create(window);
   if (window->display->wl.zxdg_shell) _window_v6_shell_surface_create(window);

   if (window->display->wl.session_recovery)
     {
        if (window->uuid)
          {
             int gx, gy, gw, gh;

             zwp_e_session_recovery_set_uuid(window->display->wl.session_recovery,
                                             window->surface, window->uuid);

             ecore_wl2_window_geometry_get(window, &gx, &gy, &gw, &gh);
             if (window->xdg_surface)
               xdg_surface_set_window_geometry(window->xdg_surface,
                                               gx, gy, gw, gh);
             if (window->zxdg_surface)
               zxdg_surface_v6_set_window_geometry(window->zxdg_surface,
                                                   gx, gy, gw, gh);

             ecore_wl2_window_opaque_region_set(window,
                                                window->opaque.x,
                                                window->opaque.y,
                                                window->opaque.w,
                                                window->opaque.h);
          }
        else
          zwp_e_session_recovery_get_uuid(window->display->wl.session_recovery, window->surface);
     }
}

/// @cond INTERNALS_DOXYGEN
/**
 * @internal
 * @brief Callback for wl_surface.enter events.
 *
 * Triggered when the wl_surface enters a wl_output (i.e., becomes visible
 * on a specific display/monitor). This function finds the corresponding
 * Ecore_Wl2_Output and adds it to the window's list of outputs.
 *
 * @param data The Ecore_Wl2_Window associated with this wl_surface.
 * @param surf The wl_surface object.
 * @param op The wl_output object that the surface entered.
 */
static void
_surface_enter(void *data, struct wl_surface *surf EINA_UNUSED, struct wl_output *op)
{
   Ecore_Wl2_Window *win;
   Ecore_Wl2_Output *output;

   win = data;

   output = _ecore_wl2_output_find(win->display, op);
   EINA_SAFETY_ON_NULL_RETURN(output);

   win->outputs = eina_list_append(win->outputs, output);
}

/**
 * @internal
 * @brief Callback for wl_surface.leave events.
 *
 * Triggered when the wl_surface leaves a wl_output (i.e., is no longer
 * visible on a specific display/monitor). This function finds the
 * corresponding Ecore_Wl2_Output and removes it from the window's list
 * of outputs. If the list becomes empty, it sends an
 * ECORE_WL2_EVENT_WINDOW_OFFSCREEN event.
 *
 * @param data The Ecore_Wl2_Window associated with this wl_surface.
 * @param surf The wl_surface object.
 * @param op The wl_output object that the surface left.
 */
static void
_surface_leave(void *data, struct wl_surface *surf EINA_UNUSED, struct wl_output *op)
{
   Ecore_Wl2_Window *win;
   Ecore_Wl2_Output *output;

   win = data;
   output = _ecore_wl2_output_find(win->display, op);
   EINA_SAFETY_ON_NULL_RETURN(output);

   win->outputs = eina_list_remove(win->outputs, output);
   if (!win->outputs)
     {
        Ecore_Wl2_Event_Window_Offscreen *ev;
        ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Offscreen));
        if (ev)
          {
             ev->win = win;
             ecore_event_add(ECORE_WL2_EVENT_WINDOW_OFFSCREEN, ev, NULL, NULL);
          }
     }
}

static const struct wl_surface_listener _surface_listener =
{
   _surface_enter,
   _surface_leave,
};
/// @endcond

/**
 * @internal
 * @brief Creates a wl_surface for the given window if it doesn't exist.
 *
 * This function ensures that a wl_surface is created via the wl_compositor.
 * It sets the window as user data for the surface, retrieves its ID,
 * and adds listeners for surface events (enter, leave). If efl_aux_hints
 * are supported, it also fetches supported auxiliary hints.
 *
 * @param window The window for which to create the wl_surface.
 */
void
_ecore_wl2_window_surface_create(Ecore_Wl2_Window *window)
{
   if (!window->display->wl.compositor) return;

   if (!window->surface)
     {
        window->surface =
          wl_compositor_create_surface(window->display->wl.compositor);
        if (!window->surface)
          {
             ERR("Failed to create surface for window");
             return;
          }
        wl_surface_set_user_data(window->surface, window);

        window->surface_id =
          wl_proxy_get_id((struct wl_proxy *)window->surface);

        wl_surface_add_listener(window->surface, &_surface_listener, window);
        if (window->display->wl.efl_aux_hints)
          {
             efl_aux_hints_get_supported_aux_hints(window->display->wl.efl_aux_hints, window->surface);
             if (_ecore_wl2_display_sync_get())
               wl_display_roundtrip(window->display->wl.display);
          }
     }
}

/**
 * @internal
 * @brief Sends a window show event.
 *
 * Allocates and populates an Ecore_Wl2_Event_Window_Show event,
 * sets the window's visible flag to EINA_TRUE, and adds the event
 * to the Ecore event queue.
 *
 * @param window The window that is being shown.
 */
static void
_ecore_wl2_window_show_send(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Event_Window_Show *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Show));
   if (!ev) return;

   ev->win = window;
   if (window->parent)
     ev->parent_win = window->parent;
   ev->event_win = window;
   window->visible = EINA_TRUE;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_SHOW, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Sends a window hide event.
 *
 * Allocates and populates an Ecore_Wl2_Event_Window_Hide event,
 * sets the window's visible flag to EINA_FALSE, and adds the event
 * to the Ecore event queue.
 *
 * @param window The window that is being hidden.
 */
static void
_ecore_wl2_window_hide_send(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Event_Window_Hide *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Hide));
   if (!ev) return;

   ev->win = window;
   if (window->parent)
     ev->parent_win = window->parent;
   ev->event_win = window;
   window->visible = EINA_FALSE;
   ecore_event_add(ECORE_WL2_EVENT_WINDOW_HIDE, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Sends a window create or destroy event.
 *
 * Allocates and populates an Ecore_Wl2_Event_Window_Common event.
 * Depending on the 'create' flag, it adds either an
 * ECORE_WL2_EVENT_WINDOW_CREATE or ECORE_WL2_EVENT_WINDOW_DESTROY
 * event to the Ecore event queue.
 *
 * @param window The window being created or destroyed.
 * @param create EINA_TRUE if a create event should be sent,
 *               EINA_FALSE for a destroy event.
 */
static void
_ecore_wl2_window_create_destroy_send(Ecore_Wl2_Window *window, Eina_Bool create)
{
   Ecore_Wl2_Event_Window_Common *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Common));
   if (!ev) return;

   ev->win = window;
   if (window->parent)
     ev->parent_win = window->parent;
   ev->event_win = window;

   if (create) ecore_event_add(ECORE_WL2_EVENT_WINDOW_CREATE, ev, NULL, NULL);
   else ecore_event_add(ECORE_WL2_EVENT_WINDOW_DESTROY, ev, NULL, NULL);
}

/**
 * @brief Creates a new Wayland window.
 *
 * This function allocates and initializes a new Ecore_Wl2_Window structure.
 * It associates the window with the given display and parent (if any),
 * sets its initial geometry, and adds it to the display's list of windows.
 * A wl_surface is created for the window, and an ECORE_WL2_EVENT_WINDOW_CREATE
 * event is sent.
 *
 * @param display The Ecore_Wl2_Display to create the window on.
 * @param parent The parent Ecore_Wl2_Window, or NULL for a top-level window.
 * @param x The initial x coordinate of the window.
 * @param y The initial y coordinate of the window.
 * @param w The initial width of the window.
 * @param h The initial height of the window.
 * @return A new Ecore_Wl2_Window instance, or NULL on failure.
 */
EAPI Ecore_Wl2_Window *
ecore_wl2_window_new(Ecore_Wl2_Display *display, Ecore_Wl2_Window *parent, int x, int y, int w, int h)
{
   Ecore_Wl2_Window *win;

   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   if (display->pid) CRI("CANNOT CREATE WINDOW WITH SERVER DISPLAY");

   /* try to allocate space for window structure */
   win = calloc(1, sizeof(Ecore_Wl2_Window));
   if (!win) return NULL;
   display->refs++;

   win->display = display;
   win->parent = parent;

   win->set_config.geometry.x = x;
   win->set_config.geometry.y = y;
   win->set_config.geometry.w = w;
   win->set_config.geometry.h = h;

   win->opaque.x = x;
   win->opaque.y = y;
   win->opaque.w = w;
   win->opaque.h = h;

   win->pending.configure = EINA_TRUE;
   display->windows =
     eina_inlist_append(display->windows, EINA_INLIST_GET(win));

   _ecore_wl2_window_surface_create(win);

   _ecore_wl2_window_create_destroy_send(win, EINA_TRUE);

   return win;
}

/**
 * @brief Sets a callback function to be invoked when the window is requested to close.
 *
 * This callback is typically triggered by the compositor (e.g., when the user
 * clicks the close button on a window decoration).
 *
 * @param window The window for which to set the close callback.
 * @param cb The callback function.
 * @param data User data to be passed to the callback function.
 */
EAPI void
ecore_wl2_window_close_callback_set(Ecore_Wl2_Window *window, void (*cb) (void *data, Ecore_Wl2_Window *win), void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->cb_close = cb;
   window->cb_close_data = data;
}

/**
 * @brief Gets the Wayland surface (wl_surface) associated with an Ecore_Wl2_Window.
 *
 * If the wl_surface has not yet been created for this window, this function
 * will attempt to create it.
 *
 * @param window The window whose wl_surface is to be retrieved.
 * @return The wl_surface, or NULL on failure or if the window is invalid.
 */
EAPI struct wl_surface *
ecore_wl2_window_surface_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   _ecore_wl2_window_surface_create(window);

   return window->surface;
}

/**
 * @brief Gets the Wayland proxy ID of the wl_surface associated with an Ecore_Wl2_Window.
 *
 * The proxy ID is a unique identifier for the wl_surface object within the
 * Wayland connection.
 *
 * @param window The window whose wl_surface ID is to be retrieved.
 * @return The wl_surface proxy ID, or -1 if the window or surface is invalid.
 */
EAPI int
ecore_wl2_window_surface_id_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, -1);
   return window->surface_id;
}

/**
 * @brief Makes an Ecore_Wl2_Window visible.
 *
 * This function ensures the window's wl_surface is created, applies any
 * pending input or opaque regions, initializes the shell surface (xdg_toplevel,
 * xdg_popup, etc.) if not already done for relevant window types, and sends
 * an ECORE_WL2_EVENT_WINDOW_SHOW event. For certain window types like DND or
 * NONE, it directly marks configuration as complete.
 *
 * @param window The window to show.
 */
EAPI void
ecore_wl2_window_show(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   _ecore_wl2_window_surface_create(window);

   if (window->input_set)
     ecore_wl2_window_input_region_set(window, window->input_rect.x,
                                       window->input_rect.y,
                                       window->input_rect.w,
                                       window->input_rect.h);
   if (window->opaque_set)
     ecore_wl2_window_opaque_region_set(window, window->opaque.x,
                                        window->opaque.y, window->opaque.w,
                                        window->opaque.h);

   if ((window->type != ECORE_WL2_WINDOW_TYPE_DND) &&
       (window->type != ECORE_WL2_WINDOW_TYPE_NONE))
     {
        _ecore_wl2_window_shell_surface_init(window);
        _ecore_wl2_window_show_send(window);
     }
   else
     _configure_complete(window);
}

/**
 * @brief Hides an Ecore_Wl2_Window.
 *
 * This function sends an ECORE_WL2_EVENT_WINDOW_HIDE event. If there's a
 * pending commit (e.g., due to an animator), the associated frame callback
 * is cancelled. It then detaches any buffer from the window's wl_surface
 * and commits the change. Any existing frame callback is destroyed.
 * It also resets various configuration serials and Wayland interface pointers
 * related to shell surfaces.
 *
 * @param window The window to hide.
 */
EAPI void
ecore_wl2_window_hide(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   _ecore_wl2_window_hide_send(window);

   if (window->commit_pending)
     {
        /* We've probably been hidden while an animator
         * is ticking.  Cancel the callback.
         */
        window->commit_pending = EINA_FALSE;
        if (window->callback)
          {
             wl_callback_destroy(window->callback);
             window->callback = NULL;
          }
     }

   if (window->surface)
     {
        wl_surface_attach(window->surface, NULL, 0, 0);
        ecore_wl2_window_commit(window, EINA_TRUE);
        window->commit_pending = EINA_FALSE;
     }

   /* The commit added a callback, disconnect it */
   if (window->callback)
     {
        wl_callback_destroy(window->callback);
        window->callback = NULL;
     }

   window->set_config.serial = 0;
   window->req_config.serial = 0;
   window->def_config.serial = 0;
   window->zxdg_configure_ack = NULL;
   window->xdg_configure_ack = NULL;
   window->xdg_set_min_size = NULL;
   window->xdg_set_max_size = NULL;
   window->zxdg_set_min_size = NULL;
   window->zxdg_set_max_size = NULL;
}

/**
 * @internal
 * @brief Frees the list of supported auxiliary hints for a window.
 *
 * Iterates through the window's list of supported auxiliary hint strings
 * and deletes each stringshare reference.
 *
 * @param win The window whose auxiliary hints are to be freed.
 */
static void
_ecore_wl2_window_aux_hint_free(Ecore_Wl2_Window *win)
{
   const char *supported;

   EINA_LIST_FREE(win->supported_aux_hints, supported)
     if (supported) eina_stringshare_del(supported);
}

/**
 * @brief Frees an Ecore_Wl2_Window and its associated resources.
 *
 * This function performs a comprehensive cleanup of the window:
 * - Sends a hide event if the window is visible.
 * - Sends a destroy event.
 * - Removes the window from all input devices' focus lists.
 * - Frees all associated subsurfaces.
 * - Frees auxiliary hints.
 * - Destroys any pending frame callback.
 * - If session recovery is active, destroys the UUID associated with the surface.
 * - Calls _ecore_wl2_window_semi_free() to release Wayland objects.
 * - Frees stringshared title, class, and role.
 * - Frees memory allocated for available rotations.
 * - Removes the window from the display's list of windows.
 * - Decrements the display's reference count.
 * - Frees the Ecore_Wl2_Window structure itself.
 *
 * @param window The window to free.
 */
EAPI void
ecore_wl2_window_free(Ecore_Wl2_Window *window)
{
   Ecore_Wl2_Display *display;
   Ecore_Wl2_Input *input;
   Ecore_Wl2_Subsurface *subsurf;
   Eina_Inlist *tmp;

   EINA_SAFETY_ON_NULL_RETURN(window);

   if (window->visible) _ecore_wl2_window_hide_send(window);

   _ecore_wl2_window_create_destroy_send(window, EINA_FALSE);

   display = window->display;

   EINA_INLIST_FOREACH(display->inputs, input)
      _ecore_wl2_input_window_remove(input, window);

   EINA_INLIST_FOREACH_SAFE(window->subsurfs, tmp, subsurf)
     _ecore_wl2_subsurf_free(subsurf);

   _ecore_wl2_window_aux_hint_free(window);

   if (window->callback) wl_callback_destroy(window->callback);
   window->callback = NULL;

   if (window->uuid && window->surface && window->display->wl.session_recovery)
     zwp_e_session_recovery_destroy_uuid(window->display->wl.session_recovery,
                                         window->surface, window->uuid);

   _ecore_wl2_window_semi_free(window);

   eina_stringshare_replace(&window->uuid, NULL);

   if (window->title) eina_stringshare_del(window->title);
   if (window->class) eina_stringshare_del(window->class);
   if (window->role) eina_stringshare_del(window->role);

   if (window->wm_rot.available_rots) free(window->wm_rot.available_rots);
   window->wm_rot.available_rots = NULL;

   display->windows =
     eina_inlist_remove(display->windows, EINA_INLIST_GET(window));

   ecore_wl2_display_disconnect(window->display);
   free(window);
}

/**
 * @brief Initiates an interactive move operation for the window.
 *
 * This function requests the compositor to start an interactive move session
 * for the window, typically allowing the user to drag the window to a new
 * position using the specified input device (e.g., mouse).
 * If `input` is NULL (deprecated usage), it defaults to the first available input.
 * After initiating the move, any existing grab on the input device is released.
 *
 * @param window The window to move.
 * @param input The Ecore_Wl2_Input device to be used for the move operation.
 *              If NULL, the first available input is used (deprecated).
 */
EAPI void
ecore_wl2_window_move(Ecore_Wl2_Window *window, Ecore_Wl2_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(window->display->inputs);

   if (!input)
     {
        ERR("NULL input parameter is deprecated");
        input = EINA_INLIST_CONTAINER_GET(window->display->inputs, Ecore_Wl2_Input);
     }
   if (window->xdg_toplevel)
     xdg_toplevel_move(window->xdg_toplevel, input->wl.seat,
                           window->display->serial);
   if (window->zxdg_toplevel)
     zxdg_toplevel_v6_move(window->zxdg_toplevel, input->wl.seat,
                           window->display->serial);
   ecore_wl2_display_flush(window->display);

   _ecore_wl2_input_ungrab(input);
}

/**
 * @brief Initiates an interactive resize operation for the window.
 *
 * This function requests the compositor to start an interactive resize session
 * for the window from the specified edge or corner (`location`). This typically
 * allows the user to drag the window border/corner to resize it using the
 * specified input device.
 * If `input` is NULL (deprecated usage), it defaults to the first available input.
 * After initiating the resize, any existing grab on the input device is released.
 *
 * @param window The window to resize.
 * @param input The Ecore_Wl2_Input device to be used for the resize operation.
 *              If NULL, the first available input is used (deprecated).
 * @param location An enum value (e.g., XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT)
 *                 indicating which edge or corner to start resizing from.
 */
EAPI void
ecore_wl2_window_resize(Ecore_Wl2_Window *window, Ecore_Wl2_Input *input, int location)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(window->display->inputs);

   if (!input)
     {
        ERR("NULL input parameter is deprecated");
        input = EINA_INLIST_CONTAINER_GET(window->display->inputs, Ecore_Wl2_Input);
     }

   if (window->xdg_toplevel)
     xdg_toplevel_resize(window->xdg_toplevel, input->wl.seat,
                             window->display->serial, location);
   if (window->zxdg_toplevel)
     zxdg_toplevel_v6_resize(window->zxdg_toplevel, input->wl.seat,
                             window->display->serial, location);
   ecore_wl2_display_flush(window->display);

   _ecore_wl2_input_ungrab(input);
}

/**
 * @brief Gets the alpha state of the window.
 *
 * An alpha window typically means it supports per-pixel transparency.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window has alpha enabled, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_alpha_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   return window->alpha;
}

/**
 * @brief Sets the alpha state of the window.
 *
 * Enabling alpha allows the window to have transparent regions. If the window
 * has an associated Ecore_Wl2_Surface, this function will also reconfigure
 * that surface with the new alpha state.
 *
 * @param window The window to modify.
 * @param alpha EINA_TRUE to enable alpha, EINA_FALSE to disable.
 */
EAPI void
ecore_wl2_window_alpha_set(Ecore_Wl2_Window *window, Eina_Bool alpha)
{
   Ecore_Wl2_Surface *surf;

   EINA_SAFETY_ON_NULL_RETURN(window);

   if (window->alpha == alpha) return;

   window->alpha = alpha;
   surf = window->wl2_surface;
   if (surf)
     ecore_wl2_surface_reconfigure(surf, surf->w, surf->h, 0, alpha);
}

/**
 * @brief Sets the opaque region of the window.
 *
 * The opaque region is a hint to the compositor about which parts of the
 * window are fully opaque. This can allow for optimizations (e.g., not
 * rendering content behind these areas). The coordinates are relative to
 * the window itself. The function takes the window's current rotation
 * into account when transforming the input rectangle to the surface's
 * coordinate space.
 *
 * @param window The window to modify.
 * @param x The x coordinate of the opaque region.
 * @param y The y coordinate of the opaque region.
 * @param w The width of the opaque region.
 * @param h The height of the opaque region.
 */
EAPI void
ecore_wl2_window_opaque_region_set(Ecore_Wl2_Window *window, int x, int y, int w, int h)
{
   int nx = 0, ny = 0, nw = 0, nh = 0;

   EINA_SAFETY_ON_NULL_RETURN(window);

   switch (window->rotation)
     {
      case 0:
        nx = x;
        ny = y;
        nw = w;
        nh = h;
        break;
      case 90:
        nx = y;
        ny = x;
        nw = h;
        nh = w;
        break;
      case 180:
        nx = x;
        ny = x + y;
        nw = w;
        nh = h;
        break;
      case 270:
        nx = x + y;
        ny = x;
        nw = h;
        nh = w;
        break;
      default:
        break;
     }

   if ((window->opaque.x == nx) && (window->opaque.y == ny) &&
       (window->opaque.w == nw) && (window->opaque.h == nh))
     return;

   window->opaque.x = nx;
   window->opaque.y = ny;
   window->opaque.w = nw;
   window->opaque.h = nh;
   window->opaque_set = x || y || w || h;
   window->pending.opaque = EINA_TRUE;
}

/**
 * @brief Gets the opaque region of the window.
 *
 * Retrieves the currently set opaque region. The coordinates returned are
 * in the surface's coordinate space (after rotation adjustment).
 *
 * @param window The window to query.
 * @param[out] x Pointer to store the x coordinate of the opaque region.
 * @param[out] y Pointer to store the y coordinate of the opaque region.
 * @param[out] w Pointer to store the width of the opaque region.
 * @param[out] h Pointer to store the height of the opaque region.
 */
EAPI void
ecore_wl2_window_opaque_region_get(Ecore_Wl2_Window *window, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (x) *x = window->opaque.x;
   if (y) *y = window->opaque.y;
   if (w) *w = window->opaque.w;
   if (h) *h = window->opaque.h;
}

/**
 * @brief Sets the input region of the window.
 *
 * The input region defines the area of the window that can receive pointer
 * and touch events. Events outside this region may be passed to windows
 * underneath. The coordinates are relative to the window itself.
 * The function takes the window's current rotation into account when
 * transforming the input rectangle to the surface's coordinate space.
 *
 * @param window The window to modify.
 * @param x The x coordinate of the input region.
 * @param y The y coordinate of the input region.
 * @param w The width of the input region.
 * @param h The height of the input region.
 */
EAPI void
ecore_wl2_window_input_region_set(Ecore_Wl2_Window *window, int x, int y, int w, int h)
{
   int nx = 0, ny = 0, nw = 0, nh = 0;

   EINA_SAFETY_ON_NULL_RETURN(window);

   switch (window->rotation)
     {
      case 0:
        nx = x;
        ny = y;
        nw = w;
        nh = h;
        break;
      case 90:
        nx = y;
        ny = x;
        nw = h;
        nh = w;
        break;
      case 180:
        nx = x;
        ny = x + y;
        nw = w;
        nh = h;
        break;
      case 270:
        nx = x + y;
        ny = x;
        nw = h;
        nh = w;
        break;
      default:
        break;
     }

   if ((window->input_rect.x == nx) && (window->input_rect.y == ny) &&
       (window->input_rect.w == nw) && (window->input_rect.h == nh))
     return;

   window->input_rect.x = nx;
   window->input_rect.y = ny;
   window->input_rect.w = nw;
   window->input_rect.h = nh;
   window->input_set = x || y || w || h;
   window->pending.input = EINA_TRUE;
}

/**
 * @brief Gets the input region of the window.
 *
 * Retrieves the currently set input region. The coordinates returned are
 * in the surface's coordinate space (after rotation adjustment).
 *
 * @param window The window to query.
 * @param[out] x Pointer to store the x coordinate of the input region.
 * @param[out] y Pointer to store the y coordinate of the input region.
 * @param[out] w Pointer to store the width of the input region.
 * @param[out] h Pointer to store the height of the input region.
 */
EAPI void
ecore_wl2_window_input_region_get(Ecore_Wl2_Window *window, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (x) *x = window->input_rect.x;
   if (y) *y = window->input_rect.y;
   if (w) *w = window->input_rect.w;
   if (h) *h = window->input_rect.h;
}

/**
 * @brief Gets the maximized state of the window.
 *
 * This reflects the client's requested or current maximized state.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window is set to be maximized, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_maximized_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   return window->set_config.maximized;
}

/**
 * @brief Sets the maximized state of the window.
 *
 * Requests the compositor to maximize or unmaximize the window.
 * If the window is currently being updated (e.g., during a resize), the
 * request is pended. Otherwise, the appropriate xdg_toplevel or
 * zxdg_toplevel_v6 request is sent. When maximizing, the window's current
 * geometry is saved if it's not fullscreen, to be restored later.
 *
 * @param window The window to modify.
 * @param maximized EINA_TRUE to maximize, EINA_FALSE to unmaximize.
 */
EAPI void
ecore_wl2_window_maximized_set(Ecore_Wl2_Window *window, Eina_Bool maximized)
{
   Eina_Bool prev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   prev = window->set_config.maximized;
   maximized = !!maximized;
   if (prev == maximized) return;

   window->set_config.maximized = maximized;
   if (window->updating)
     {
        window->pending.maximized = EINA_TRUE;
        return;
     }

   if (maximized)
     {
        if (!window->set_config.fullscreen)
          window->saved = window->set_config.geometry;

        if (window->xdg_toplevel)
          xdg_toplevel_set_maximized(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_set_maximized(window->zxdg_toplevel);
     }
   else
     {
        if (window->xdg_toplevel)
          xdg_toplevel_unset_maximized(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_unset_maximized(window->zxdg_toplevel);
     }
   ecore_wl2_display_flush(window->display);
}

/**
 * @brief Gets the fullscreen state of the window.
 *
 * This reflects the client's requested or current fullscreen state.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window is set to be fullscreen, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_fullscreen_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   return window->set_config.fullscreen;
}

/**
 * @brief Sets the fullscreen state of the window.
 *
 * Requests the compositor to set the window to fullscreen or restore it.
 * If the window is currently being updated, the request is pended.
 * Otherwise, the appropriate xdg_toplevel or zxdg_toplevel_v6 request is sent.
 * When going fullscreen, the window's current geometry is saved if it's not
 * maximized, to be restored later.
 *
 * @param window The window to modify.
 * @param fullscreen EINA_TRUE to set fullscreen, EINA_FALSE to unset.
 */
EAPI void
ecore_wl2_window_fullscreen_set(Ecore_Wl2_Window *window, Eina_Bool fullscreen)
{
   Eina_Bool prev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   prev = window->set_config.fullscreen;
   fullscreen = !!fullscreen;
   if (prev == fullscreen) return;

   window->set_config.fullscreen = fullscreen;
   if (window->updating)
     {
        window->pending.fullscreen = EINA_TRUE;
        return;
     }

   if (fullscreen)
     {
        if (!window->set_config.maximized)
          window->saved = window->set_config.geometry;

        if (window->xdg_toplevel)
          xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_set_fullscreen(window->zxdg_toplevel, NULL);
     }
   else
     {
        if (window->xdg_toplevel)
          xdg_toplevel_unset_fullscreen(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_unset_fullscreen(window->zxdg_toplevel);
     }
   ecore_wl2_display_flush(window->display);
}

/**
 * @brief Gets the current rotation of the window content.
 *
 * This is the rotation applied by the client to its content, typically
 * 0, 90, 180, or 270 degrees.
 *
 * @param window The window to query.
 * @return The rotation angle in degrees, or -1 on error.
 */
EAPI int
ecore_wl2_window_rotation_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, -1);

   return window->rotation;
}

/**
 * @brief Sets the rotation of the window content.
 *
 * This informs Ecore_Wl2 about the rotation applied by the client to its
 * content. This value is used, for example, when calculating transformed
 * opaque or input regions. It does not directly cause a wl_surface
 * buffer transform; for that, see ecore_wl2_window_buffer_transform_set().
 *
 * @param window The window to modify.
 * @param rotation The rotation angle in degrees (e.g., 0, 90, 180, 270).
 */
EAPI void
ecore_wl2_window_rotation_set(Ecore_Wl2_Window *window, int rotation)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   window->rotation = rotation;
}

/**
 * @brief Sets the title of the window.
 *
 * This title is typically displayed by the compositor in window decorations
 * or task switchers. If the window has an xdg_toplevel or zxdg_toplevel_v6,
 * the new title is sent to the compositor.
 *
 * @param window The window to modify.
 * @param title The new title string. The string is copied.
 */
EAPI void
ecore_wl2_window_title_set(Ecore_Wl2_Window *window, const char *title)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   eina_stringshare_replace(&window->title, title);
   if (!window->title) return;
   if (!window->xdg_toplevel && !window->zxdg_toplevel) return;

   if (window->xdg_toplevel)
     xdg_toplevel_set_title(window->xdg_toplevel, window->title);
   if (window->zxdg_toplevel)
     zxdg_toplevel_v6_set_title(window->zxdg_toplevel, window->title);
   ecore_wl2_display_flush(window->display);
}

/**
 * @brief Gets the title of the window.
 *
 * @param window The window to query.
 * @return A pointer to the window's title string (stringshared, do not free),
 *         or NULL if no title is set or on error.
 */
EAPI const char *
ecore_wl2_window_title_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   return window->title ? window->title : NULL;
}

/**
 * @brief Sets the application ID (class) of the window.
 *
 * The application ID is used by the compositor to group windows or apply
 * specific theming/rules. If the window has an xdg_toplevel or
 * zxdg_toplevel_v6, the new app_id is sent to the compositor.
 *
 * @param window The window to modify.
 * @param clas The new application ID string (often called class name).
 *             The string is copied.
 */
EAPI void
ecore_wl2_window_class_set(Ecore_Wl2_Window *window, const char *clas)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   eina_stringshare_replace(&window->class, clas);
   if (!window->class) return;
   if (!window->xdg_toplevel && !window->zxdg_toplevel) return;

   if (window->xdg_toplevel)
     xdg_toplevel_set_app_id(window->xdg_toplevel, window->class);
   if (window->zxdg_toplevel)
     zxdg_toplevel_v6_set_app_id(window->zxdg_toplevel, window->class);
   ecore_wl2_display_flush(window->display);
}

/**
 * @brief Gets the application ID (class) of the window.
 *
 * @param window The window to query.
 * @return A pointer to the window's application ID string (stringshared, do not free),
 *         or NULL if no class is set or on error.
 */
EAPI const char *
ecore_wl2_window_class_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   return window->class ? window->class : NULL;
}

/**
 * @brief Gets the current geometry of the window (client-side understanding).
 *
 * This returns the geometry that the client has set or believes the window
 * currently has. This might not always match the compositor's view immediately,
 * especially during resize operations.
 *
 * @param window The window to query.
 * @param[out] x Pointer to store the x coordinate. Can be NULL.
 * @param[out] y Pointer to store the y coordinate. Can be NULL.
 * @param[out] w Pointer to store the width. Can be NULL.
 * @param[out] h Pointer to store the height. Can be NULL.
 */
EAPI void
ecore_wl2_window_geometry_get(Ecore_Wl2_Window *window, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (x) *x = window->set_config.geometry.x;
   if (y) *y = window->set_config.geometry.y;
   if (w) *w = window->set_config.geometry.w;
   if (h) *h = window->set_config.geometry.h;
}

/**
 * @brief Sets the desired geometry of the window.
 *
 * This function updates the client-side understanding of the window's geometry
 * and marks it as pending a geometry change. The actual change is usually
 * communicated to the compositor during a commit operation (e.g., via
 * xdg_surface_set_window_geometry). This does not directly resize the window;
 * resizing is typically a negotiation with the compositor.
 *
 * @param window The window to modify.
 * @param x The desired x coordinate.
 * @param y The desired y coordinate.
 * @param w The desired width.
 * @param h The desired height.
 */
EAPI void
ecore_wl2_window_geometry_set(Ecore_Wl2_Window *window, int x, int y, int w, int h)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if ((window->set_config.geometry.x == x) &&
       (window->set_config.geometry.y == y) &&
       (window->set_config.geometry.w == w) &&
       (window->set_config.geometry.h == h))
     return;

   window->set_config.geometry.x = x;
   window->set_config.geometry.y = y;
   window->set_config.geometry.w = w;
   window->set_config.geometry.h = h;
   window->pending.geom = EINA_TRUE;
}

/**
 * @brief Sets the iconified (minimized) state of the window.
 *
 * Requests the compositor to minimize the window. If the window's shell
 * surface (xdg_toplevel or zxdg_toplevel_v6) hasn't been created yet,
 * the request is deferred until it is.
 *
 * @param window The window to modify.
 * @param iconified EINA_TRUE to iconify (minimize), EINA_FALSE is usually
 *                  not used to unminimize (compositor handles unminimizing).
 */
EAPI void
ecore_wl2_window_iconified_set(Ecore_Wl2_Window *window, Eina_Bool iconified)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   iconified = !!iconified;

   if (!window->xdg_toplevel && !window->zxdg_toplevel)
     {
        window->deferred_minimize = iconified;
        return;
     }

   if (iconified)
     {
        if (window->xdg_toplevel)
          xdg_toplevel_set_minimized(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_set_minimized(window->zxdg_toplevel);
        ecore_wl2_display_flush(window->display);
     }
}

/**
 * @brief Sets the type of the Ecore_Wl2_Window.
 *
 * The window type influences how it's handled by Ecore_Wl2 and potentially
 * by the compositor (e.g., ECORE_WL2_WINDOW_TYPE_MENU might become an
 * xdg_popup). This should generally be set before the window is shown.
 *
 * @param window The window to modify.
 * @param type The Ecore_Wl2_Window_Type to set.
 *        Example types:
 *        - ECORE_WL2_WINDOW_TYPE_TOPLEVEL
 *        - ECORE_WL2_WINDOW_TYPE_MENU
 *        - ECORE_WL2_WINDOW_TYPE_DND
 */
EAPI void
ecore_wl2_window_type_set(Ecore_Wl2_Window *window, Ecore_Wl2_Window_Type type)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->type = type;
}

/**
 * @brief Associates an input device with a popup window for grabbing.
 *
 * For windows of type ECORE_WL2_WINDOW_TYPE_MENU (popups), this function
 * sets the Ecore_Wl2_Input device that will be used to perform a grab
 * when the popup is shown (e.g., xdg_popup_grab).
 *
 * @param window The popup window (must be of type ECORE_WL2_WINDOW_TYPE_MENU).
 * @param input The Ecore_Wl2_Input device to associate with the popup for grabs.
 */
EAPI void
ecore_wl2_window_popup_input_set(Ecore_Wl2_Window *window, Ecore_Wl2_Input *input)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(input);
   EINA_SAFETY_ON_TRUE_RETURN(window->type != ECORE_WL2_WINDOW_TYPE_MENU);
   window->grab = input;
}

/**
 * @brief Gets the input device associated with a popup window for grabbing.
 *
 * @param window The popup window to query.
 * @return The Ecore_Wl2_Input device associated for grabs, or NULL if none.
 */
EAPI Ecore_Wl2_Input *
ecore_wl2_window_popup_input_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   return window->grab;
}

/**
 * @brief Gets the Ecore_Wl2_Display associated with a window.
 *
 * Returns NULL if the display is currently in a session recovery state.
 *
 * @param window The window to query.
 * @return The Ecore_Wl2_Display, or NULL if invalid or during recovery.
 */
EAPI Ecore_Wl2_Display *
ecore_wl2_window_display_get(const Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(window->display, NULL);

   if (window->display->recovering) return NULL;

   return window->display;
}

/**
 * @brief Checks if a Wayland shell surface (XDG or ZXDG_V6) exists for the window.
 *
 * A shell surface (like xdg_surface or zxdg_surface_v6) is necessary for
 * a window to be managed by the compositor as a toplevel or popup.
 *
 * @param window The window to check.
 * @return EINA_TRUE if a shell surface exists, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_shell_surface_exists(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return !!window->zxdg_surface || !!window->xdg_surface;
}

/**
 * @brief Gets the activation (focused) state of the window.
 *
 * This reflects the `focused` state received from the compositor in the
 * most recent configure event for the window's toplevel surface.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window is considered activated (focused),
 *         EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_activated_get(const Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return window->req_config.focused;
}

/**
 * @brief Finds an output (display/monitor) that the window is currently on.
 *
 * This function returns the first Ecore_Wl2_Output from the list of outputs
 * the window's surface has entered. If the window is on multiple outputs,
 * this will return one of them (typically the first one encountered or
 * the primary one, depending on compositor behavior and event order).
 *
 * @param window The window to query.
 * @return An Ecore_Wl2_Output the window is on, or NULL if it's not on any
 *         (e.g., offscreen or not yet mapped).
 */
EAPI Ecore_Wl2_Output *
ecore_wl2_window_output_find(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   return eina_list_data_get(window->outputs);
}

/**
 * @brief Sets the buffer transform for the window's surface.
 *
 * This informs the compositor how the content of attached buffers should be
 * transformed (e.g., rotated, flipped) before display. This is a direct
 * Wayland request (wl_surface_set_buffer_transform).
 *
 * @param window The window whose surface buffer transform is to be set.
 * @param transform A wl_output_transform enum value (e.g.,
 *                  WL_OUTPUT_TRANSFORM_NORMAL, WL_OUTPUT_TRANSFORM_90).
 */
EAPI void
ecore_wl2_window_buffer_transform_set(Ecore_Wl2_Window *window, int transform)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   wl_surface_set_buffer_transform(window->surface, transform);
}

/**
 * @brief Sets whether the window supports WM-managed rotation.
 *
 * This is a hint from the application to the window manager (compositor)
 * indicating whether it is capable of handling rotation changes initiated
 * by the WM.
 *
 * @param window The window to modify.
 * @param enabled EINA_TRUE if WM-managed rotation is supported, EINA_FALSE otherwise.
 */
EAPI void
ecore_wl2_window_wm_rotation_supported_set(Ecore_Wl2_Window *window, Eina_Bool enabled)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->wm_rot.supported = enabled;
}

/**
 * @brief Gets whether the window supports WM-managed rotation.
 *
 * @param window The window to query.
 * @return EINA_TRUE if WM-managed rotation is supported, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_wm_rotation_supported_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return window->wm_rot.supported;
}

/**
 * @brief Sets whether the application itself is managing the window's rotation.
 *
 * This flag indicates if the current rotation of the window content was set
 * by the application (e.g., in response to orientation sensors) rather than
 * by the window manager.
 *
 * @param window The window to modify.
 * @param set EINA_TRUE if the application set the current rotation, EINA_FALSE otherwise.
 */
EAPI void
ecore_wl2_window_rotation_app_set(Ecore_Wl2_Window *window, Eina_Bool set)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->wm_rot.app_set = set;
}

/**
 * @brief Gets whether the application itself is managing the window's rotation.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the application set the current rotation, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_rotation_app_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return window->wm_rot.app_set;
}

/**
 * @brief Sets the application's preferred rotation for the window.
 *
 * This is a hint to the window manager about the rotation angle (e.g., 0, 90,
 * 180, 270 degrees) that the application would prefer for this window.
 *
 * @param window The window to modify.
 * @param rot The preferred rotation angle in degrees.
 */
EAPI void
ecore_wl2_window_preferred_rotation_set(Ecore_Wl2_Window *window, int rot)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->wm_rot.preferred_rot = rot;
}

/**
 * @brief Gets the application's preferred rotation for the window.
 *
 * @param window The window to query.
 * @return The preferred rotation angle in degrees. Defaults to 0 if not set or on error.
 */
EAPI int
ecore_wl2_window_preferred_rotation_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, 0);
   return window->wm_rot.preferred_rot;
}

/**
 * @brief Sets the list of available rotations that the window content can adapt to.
 *
 * This informs the window manager about the rotation angles (e.g., 0, 90, 180, 270)
 * that the application can handle for this window. The WM might use this
 * information when deciding on an optimal rotation for the window or screen.
 *
 * @param window The window to modify.
 * @param rots An array of integer rotation angles in degrees.
 *             Example: `const int rots[] = {0, 90, 180, 270};`
 * @param count The number of rotation angles in the `rots` array.
 */
EAPI void
ecore_wl2_window_available_rotations_set(Ecore_Wl2_Window *window, const int *rots, unsigned int count)
{
   unsigned int i = 0;
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (window->wm_rot.available_rots)
     {
        free(window->wm_rot.available_rots);
        window->wm_rot.available_rots = NULL;
     }
   window->wm_rot.count = count;

   if (count >= 1)
     {
        window->wm_rot.available_rots = calloc(count, sizeof(int));
        if (!window->wm_rot.available_rots) return;

        for (; i < count; i++)
          window->wm_rot.available_rots[i] = ((int *)rots)[i];
     }
}

/**
 * @brief Gets the list of available rotations that the window content can adapt to.
 *
 * Retrieves the rotation angles previously set by
 * ecore_wl2_window_available_rotations_set(). The caller is responsible
 * for freeing the `*rots` array if the function returns EINA_TRUE and `*count` is > 0.
 *
 * @param window The window to query.
 * @param[out] rots Pointer to store a newly allocated array of integer rotation angles.
 *                  The caller must free this array using `free()` if not NULL.
 *                  Example of returned array structure if `*count` is 4:
 *                  `*rots` will point to `[rot1, rot2, rot3, rot4]`
 * @param[out] count Pointer to store the number of rotation angles in the `*rots` array.
 * @return EINA_TRUE if successful (even if count is 0), EINA_FALSE on allocation failure.
 */
EAPI Eina_Bool
ecore_wl2_window_available_rotations_get(Ecore_Wl2_Window *window, int **rots, unsigned int *count)
{
   unsigned int i = 0;
   int *val = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   *rots = NULL;
   *count = window->wm_rot.count;

   if (window->wm_rot.count >= 1)
     {
        val = calloc(window->wm_rot.count, sizeof(int));
        if (!val) return EINA_FALSE;

        for (; i < window->wm_rot.count; i++)
          val[i] = ((int *)window->wm_rot.available_rots)[i];

        *rots = val;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Sends an event indicating the window is preparing for a rotation change.
 *
 * This is typically called by the application when it receives a hint from
 * the WM (or detects an orientation change) that a rotation is imminent.
 * It allows the application to signal that it's starting to adapt its content
 * to the new rotation `rot` and potentially new dimensions `w`, `h`.
 *
 * @param window The window undergoing rotation preparation.
 * @param rot The new target rotation angle in degrees.
 * @param w The new target width after rotation.
 * @param h The new target height after rotation.
 * @param resize EINA_TRUE if the dimensions are changing along with rotation.
 */
EAPI void
ecore_wl2_window_rotation_change_prepare_send(Ecore_Wl2_Window *window, int rot, int w, int h, Eina_Bool resize)
{
   Ecore_Wl2_Event_Window_Rotation_Change_Prepare *ev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Rotation_Change_Prepare));
   if (!ev) return;

   ev->win = window;
   ev->rotation = rot;
   ev->w = w;
   ev->h = h;
   ev->resize = resize;

   ecore_event_add(ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE, ev, NULL, NULL);
}

/**
 * @brief Sends an event indicating the window has finished preparing for a rotation change.
 *
 * This is called by the application after it has completed all necessary
 * adjustments (e.g., re-layout, buffer reallocation) for the new rotation `rot`.
 * The WM can then proceed with the actual rotation display.
 *
 * @param window The window that has finished rotation preparation.
 * @param rot The rotation angle for which preparation is done.
 */
EAPI void
ecore_wl2_window_rotation_change_prepare_done_send(Ecore_Wl2_Window *window, int rot)
{
   Ecore_Wl2_Event_Window_Rotation_Change_Prepare_Done *ev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Rotation_Change_Prepare_Done));
   if (!ev) return;

   ev->win = window;
   ev->rotation = rot;
   ev->w = 0;
   ev->h = 0;
   ev->resize = 0;

   ecore_event_add(ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE_DONE,
                   ev, NULL, NULL);
}

/**
 * @brief Sends an event requesting the window manager to change the window's rotation.
 *
 * This is typically called by the application if it wants to initiate a
 * rotation change (e.g., based on its own logic or sensor input).
 * The WM may or may not honor this request.
 *
 * @param window The window requesting a rotation change.
 * @param rot The desired new rotation angle in degrees.
 */
EAPI void
ecore_wl2_window_rotation_change_request_send(Ecore_Wl2_Window *window, int rot)
{
   Ecore_Wl2_Event_Window_Rotation_Change_Request *ev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Rotation_Change_Request));
   if (!ev) return;

   ev->win = window;
   ev->rotation = rot;
   ev->w = 0;
   ev->h = 0;
   ev->resize = 0;

   ecore_event_add(ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_REQUEST,
                   ev, NULL, NULL);
}

/**
 * @brief Sends an event indicating the window manager has completed a rotation change.
 *
 * This event is usually generated internally or by a part of Ecore that
 * interfaces more directly with WM rotation protocols, to inform the
 * application that the rotation to `rot` (and potentially new dimensions `w`, `h`)
 * is now complete from the WM's perspective.
 *
 * @param window The window whose rotation change is done.
 * @param rot The new rotation angle in degrees.
 * @param w The new width after rotation.
 * @param h The new height after rotation.
 */
EAPI void
ecore_wl2_window_rotation_change_done_send(Ecore_Wl2_Window *window, int rot, int w, int h)
{
   Ecore_Wl2_Event_Window_Rotation_Change_Done *ev;

   EINA_SAFETY_ON_NULL_RETURN(window);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Window_Rotation_Change_Done));
   if (!ev) return;

   ev->win = window;
   ev->rotation = rot;
   ev->w = w;
   ev->h = h;
   ev->resize = 0;

   ecore_event_add(ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_DONE,
                   ev, NULL, NULL);
}

/**
 * @brief Gets a list of supported auxiliary hint names for the window.
 *
 * Auxiliary hints are custom key-value pairs that can be communicated
 * between the client and compositor using the efl_aux_hints protocol.
 * This function retrieves the names of hints that the compositor has
 * advertised as supported for this window's surface.
 *
 * @param win The window to query.
 * @return A new Eina_List of stringshared hint names. The caller is
 *         responsible for freeing the list and its contents (e.g., using
 *         EINA_LIST_FREE and eina_stringshare_del).
 *         Example of returned list structure:
 *         `list -> ["hint_name1", "hint_name2", ...]`
 *         Returns NULL if the window or its surface is invalid.
 */
EAPI Eina_List *
ecore_wl2_window_aux_hints_supported_get(Ecore_Wl2_Window *win)
{
   Eina_List *res = NULL;
   Eina_List *ll;
   char *supported_hint = NULL;
   const char *hint = NULL;

   if (!win) return NULL;
   if (!win->surface) return NULL;

   EINA_LIST_FOREACH(win->supported_aux_hints, ll, supported_hint)
     {
        hint = eina_stringshare_add(supported_hint);
        res = eina_list_append(res, hint);
     }
   return res;
}

/**
 * @brief Adds an auxiliary hint to the window.
 *
 * This sends a request to the compositor to associate a new auxiliary hint
 * (key-value pair) with the window's surface, identified by a client-chosen ID.
 * Requires the efl_aux_hints protocol to be available.
 *
 * @param win The window to add the hint to.
 * @param id A client-defined integer ID for this hint instance. Must be unique
 *           among hints currently set by this client on this surface.
 * @param hint The name (key) of the auxiliary hint.
 * @param val The value of the auxiliary hint.
 */
EAPI void
ecore_wl2_window_aux_hint_add(Ecore_Wl2_Window *win, int id, const char *hint, const char *val)
{
   if (!win) return;
   if ((!win->surface) || (!win->display->wl.efl_aux_hints)) return;

   efl_aux_hints_add_aux_hint(win->display->wl.efl_aux_hints, win->surface, id, hint, val);
   ecore_wl2_display_flush(win->display);
}

/**
 * @brief Changes the value of an existing auxiliary hint on the window.
 *
 * This sends a request to the compositor to update the value of an auxiliary
 * hint previously added with the given `id`.
 * Requires the efl_aux_hints protocol to be available.
 *
 * @param win The window whose hint is to be changed.
 * @param id The ID of the hint instance to change.
 * @param val The new value for the auxiliary hint.
 */
EAPI void
ecore_wl2_window_aux_hint_change(Ecore_Wl2_Window *win, int id, const char *val)
{
   if (!win) return;
   if ((!win->surface) && (!win->display->wl.efl_aux_hints)) return;

   efl_aux_hints_change_aux_hint(win->display->wl.efl_aux_hints, win->surface, id, val);
   ecore_wl2_display_flush(win->display);
}

/**
 * @brief Deletes an auxiliary hint from the window.
 *
 * This sends a request to the compositor to remove an auxiliary hint
 * previously added with the given `id`.
 * Requires the efl_aux_hints protocol to be available.
 *
 * @param win The window from which to delete the hint.
 * @param id The ID of the hint instance to delete.
 */
EAPI void
ecore_wl2_window_aux_hint_del(Ecore_Wl2_Window *win, int id)
{
   if (!win) return;
   if ((!win->surface) || (!win->display->wl.efl_aux_hints)) return;

   efl_aux_hints_del_aux_hint(win->display->wl.efl_aux_hints, win->surface, id);
   ecore_wl2_display_flush(win->display);
}

/**
 * @brief Sets whether the window should be skipped for focus.
 *
 * This is a hint to the window manager. If true, the window manager
 * should try to avoid giving keyboard focus to this window (e.g., if it's
 * a purely informational popup or a utility window that doesn't need input).
 *
 * @param window The window to modify.
 * @param focus_skip EINA_TRUE to suggest skipping focus, EINA_FALSE otherwise.
 */
EAPI void
ecore_wl2_window_focus_skip_set(Ecore_Wl2_Window *window, Eina_Bool focus_skip)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->focus_skip = focus_skip;
}

/**
 * @brief Gets whether the window is set to be skipped for focus.
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window is set to skip focus, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_focus_skip_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return window->focus_skip;
}

/**
 * @brief Sets the role of the window.
 *
 * The role is a string that can provide additional semantic information
 * about the window's purpose (e.g., "dialog", "toolbar", "notification").
 * This is a hint that can be used by the window manager or accessibility tools.
 *
 * @param window The window to modify.
 * @param role The role string. The string is copied.
 */
EAPI void
ecore_wl2_window_role_set(Ecore_Wl2_Window *window, const char *role)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   eina_stringshare_replace(&window->role, role);
}

/**
 * @brief Gets the role of the window.
 *
 * @param window The window to query.
 * @return A pointer to the window's role string (stringshared, do not free),
 *         or NULL if no role is set or on error.
 */
EAPI const char *
ecore_wl2_window_role_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);

   return window->role ? window->role : NULL;
}

/**
 * @brief Sets whether the window requests to be in a "floating" mode.
 *
 * This is a hint to the window manager. A floating window is typically
 * displayed above tiled or normally managed windows and is not subject
 * to the same layout constraints. This is often used for dialogs or
 * utility windows that should remain on top.
 *
 * @param window The window to modify.
 * @param floating EINA_TRUE to request floating mode, EINA_FALSE otherwise.
 */
EAPI void
ecore_wl2_window_floating_mode_set(Ecore_Wl2_Window *window, Eina_Bool floating)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   window->floating = floating;
}

/**
 * @brief Gets whether the window is set to request "floating" mode.
 *
 * @param window The window to query.
 * @return EINA_TRUE if floating mode is requested, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_floating_mode_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);
   return window->floating;
}

/**
 * @brief Sets the preferred aspect ratio for the window.
 *
 * This informs the compositor about the window's desired aspect ratio (width / height).
 * The `aspect` parameter specifies how the aspect ratio should be maintained
 * (e.g., keep aspect, ignore aspect, etc., often defined by a specific
 * Wayland protocol extension like efl-hints).
 * If the efl_hints protocol is available and an xdg_surface exists, this
 * information is sent to the compositor.
 *
 * @param window The window to modify.
 * @param w The width component of the aspect ratio. Must be >= 1.
 * @param h The height component of the aspect ratio. Must be >= 1.
 * @param aspect An enum or bitmask defining the aspect policy (e.g.,
 *               EFL_HINTS_ASPECT_POLICY_KEEP from efl-hints-client-protocol.h).
 */
EAPI void
ecore_wl2_window_aspect_set(Ecore_Wl2_Window *window, int w, int h, unsigned int aspect)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_TRUE_RETURN(w < 1);
   EINA_SAFETY_ON_TRUE_RETURN(h < 1);

   if ((window->aspect.aspect == aspect) && (window->aspect.w == w) &&
       (window->aspect.h == h))
     return;

   window->aspect.w = w;
   window->aspect.h = h;
   window->aspect.aspect = aspect;
   window->aspect.set = 1;
   if (!window->display->wl.efl_hints) return;
   if (window->xdg_surface)
     efl_hints_set_aspect(window->display->wl.efl_hints,
                          window->xdg_surface, w, h, aspect);
   ecore_wl2_display_flush(window->display);
}

/**
 * @brief Gets the preferred aspect ratio settings for the window.
 *
 * @param window The window to query.
 * @param[out] w Pointer to store the width component of the aspect ratio. Can be NULL.
 * @param[out] h Pointer to store the height component of the aspect ratio. Can be NULL.
 * @param[out] aspect Pointer to store the aspect policy. Can be NULL.
 */
EAPI void
ecore_wl2_window_aspect_get(Ecore_Wl2_Window *window, int *w, int *h, unsigned int *aspect)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (w) *w = window->aspect.w;
   if (h) *h = window->aspect.h;
   if (aspect) *aspect = window->aspect.aspect;
}

/**
 * @brief Sets the preferred weight (sizing preference) for the window.
 *
 * Weight is a hint to the compositor, often used in tiling or layout managers,
 * to indicate how much space the window would like relative to others.
 * The double values `w` and `h` are typically in the range [0.0, 1.0] and
 * are converted to integer representations (multiplied by 100) for the protocol.
 * If the efl_hints protocol is available and an xdg_surface exists, this
 * information is sent to the compositor.
 *
 * @param window The window to modify.
 * @param w The horizontal weight component (e.g., 0.0 to 1.0).
 * @param h The vertical weight component (e.g., 0.0 to 1.0).
 */
EAPI void
ecore_wl2_window_weight_set(Ecore_Wl2_Window *window, double w, double h)
{
   int ww, hh;
   EINA_SAFETY_ON_NULL_RETURN(window);

   ww = lround(w * 100);
   hh = lround(h * 100);

   if ((window->weight.w == ww) && (window->weight.h == hh))
     return;

   window->weight.w = ww;
   window->weight.h = hh;
   window->weight.set = 1;
   if (!window->display->wl.efl_hints) return;
   if (window->xdg_surface)
     efl_hints_set_weight(window->display->wl.efl_hints,
                          window->xdg_surface, ww, hh);
   ecore_wl2_display_flush(window->display);
}

/// @cond INTERNALS_DOXYGEN
/**
 * @internal
 * @brief Callback for wl_surface.frame events.
 *
 * Triggered by the compositor when it's a good time for the client to
 * redraw the window content, typically synchronized with the display's
 * refresh cycle. This function marks the window's commit as no longer
 * pending, destroys the wl_callback object, and then invokes all
 * registered Ecore_Wl2_Frame_Cb callbacks for this window.
 *
 * @param data The Ecore_Wl2_Window associated with this frame callback.
 * @param callback The wl_callback object that was triggered.
 * @param timestamp The timestamp of the frame event (e.g., presentation time).
 */
static void
_frame_cb(void *data, struct wl_callback *callback, uint32_t timestamp)
{
   Ecore_Wl2_Frame_Cb_Handle *cb;
   Ecore_Wl2_Window *window;
   Eina_Inlist *l;

   window = data;
   window->commit_pending = EINA_FALSE;
   wl_callback_destroy(callback);
   window->callback = NULL;
   EINA_INLIST_FOREACH_SAFE(window->frame_callbacks, l, cb)
     cb->cb(window, timestamp, cb->data);
}

static struct wl_callback_listener _frame_listener =
{
   _frame_cb
};
/// @endcond

/**
 * @internal
 * @brief Applies the pending maximized state to the window's shell surface.
 *
 * This function is called, usually during a commit operation, if there's a
 * pending change to the window's maximized state. It sends the appropriate
 * xdg_toplevel_set_maximized/unset_maximized (or zxdg equivalent) request
 * to the compositor. It also saves/restores window geometry as needed when
 * transitioning to/from maximized state.
 *
 * @param window The window whose maximized state is to be applied.
 */
static void
_maximized_set(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_FALSE_RETURN(window->zxdg_toplevel || window->xdg_toplevel);

   if (window->set_config.maximized)
     {
        window->saved = window->set_config.geometry;
        if (window->xdg_toplevel)
          xdg_toplevel_set_maximized(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_set_maximized(window->zxdg_toplevel);
     }
   else
     {
        if (window->xdg_toplevel)
          xdg_toplevel_unset_maximized(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_unset_maximized(window->zxdg_toplevel);
     }
}

/**
 * @internal
 * @brief Applies the pending fullscreen state to the window's shell surface.
 *
 * This function is called, usually during a commit operation, if there's a
 * pending change to the window's fullscreen state. It sends the appropriate
 * xdg_toplevel_set_fullscreen/unset_fullscreen (or zxdg equivalent) request
 * to the compositor. It also saves/restores window geometry as needed when
 * transitioning to/from fullscreen state.
 *
 * @param window The window whose fullscreen state is to be applied.
 */
static void
_fullscreen_set(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_FALSE_RETURN(window->zxdg_toplevel || window->xdg_toplevel);

   if (window->set_config.fullscreen)
     {
        window->saved = window->set_config.geometry;
        if (window->xdg_toplevel)
          xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_set_fullscreen(window->zxdg_toplevel, NULL);
     }
   else
     {
        if (window->xdg_toplevel)
          xdg_toplevel_unset_fullscreen(window->xdg_toplevel);
        if (window->zxdg_toplevel)
          zxdg_toplevel_v6_unset_fullscreen(window->zxdg_toplevel);
     }
}

/**
 * @internal
 * @brief Creates a Wayland region (wl_region) with a single rectangle.
 *
 * Utility function to simplify creating a wl_region object and adding
 * a rectangular area to it.
 *
 * @param comp The wl_compositor interface.
 * @param x The x coordinate of the rectangle.
 * @param y The y coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @return The newly created wl_region, or NULL on failure. The caller
 *         is responsible for destroying the region using wl_region_destroy().
 */
static struct wl_region *
_region_create(struct wl_compositor *comp, int x, int y, int w, int h)
{
   struct wl_region *out;

   out = wl_compositor_create_region(comp);
   if (!out)
     {
        ERR("Failed to create region");
        return NULL;
     }

   wl_region_add(out, x, y, w, h);

   return out;
}

/**
 * @internal
 * @brief Applies pending opaque and input regions to the window's surface.
 *
 * This function is called, usually during a commit operation, if there are
 * pending changes to the window's opaque or input regions. It creates
 * wl_region objects for these areas and sets them on the wl_surface using
 * wl_surface_set_opaque_region and wl_surface_set_input_region.
 * It attempts to reuse a created region if opaque and input regions are identical.
 *
 * @param window The window whose regions are to be set.
 */
static void
_regions_set(Ecore_Wl2_Window *window)
{
   struct wl_region *region = NULL;

   if (window->pending.opaque)
     {
        if (window->opaque_set)
          {
             region = _region_create(window->display->wl.compositor,
                                     window->opaque.x, window->opaque.y,
                                     window->opaque.w, window->opaque.h);
             if (!region) return;
          }
        wl_surface_set_opaque_region(window->surface, region);
     }

   if (!window->pending.input) goto out;
   if (window->type == ECORE_WL2_WINDOW_TYPE_DND) goto out;

   if (!window->input_set)
     {
        wl_surface_set_input_region(window->surface, NULL);
        goto out;
     }

   if (region && (window->opaque.x == window->input_rect.x) &&
       (window->opaque.y == window->input_rect.y) &&
       (window->opaque.w == window->input_rect.w) &&
       (window->opaque.h == window->input_rect.h))
     {
        wl_surface_set_input_region(window->surface, region);
        goto out;
     }
   if (region) wl_region_destroy(region);

   region = _region_create(window->display->wl.compositor,
                           window->input_rect.x, window->input_rect.y,
                           window->input_rect.w, window->input_rect.h);
   if (!region) return;
   wl_surface_set_input_region(window->surface, region);

out:
   if (region) wl_region_destroy(region);
}

/**
 * @brief Commits pending state changes for the window to its surface.
 *
 * This is a crucial function that applies various pending changes to the
 * window's wl_surface and then, if `flush` is true, commits the surface
 * state and flushes the display connection.
 *
 * Operations performed during commit (if changes are pending):
 * - Registers a frame callback for synchronization.
 * - Sets window geometry on the xdg_surface/zxdg_surface.
 * - Sets opaque and input regions on the wl_surface.
 * - Applies pending maximized or fullscreen states.
 * - Acknowledges compositor configure events (xdg_surface_ack_configure).
 *
 * If a commit is already pending (i.e., a frame callback is active), a warning
 * is issued, as this usually indicates redundant commits.
 *
 * @param window The window whose state is to be committed.
 * @param flush If EINA_TRUE, wl_surface_commit() is called and the display
 *              connection is flushed. If EINA_FALSE, only internal state
 *              updates and Wayland requests are queued, but not necessarily
 *              sent immediately (useful for batching).
 */
EAPI void
ecore_wl2_window_commit(Ecore_Wl2_Window *window, Eina_Bool flush)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(window->surface);

   if (window->commit_pending)
     {
        if (window->callback)
          wl_callback_destroy(window->callback);
        window->callback = NULL;
        /* The elm mouse cursor bits do some harmless but weird stuff that
         * can hit this, silence the warning for that case only. */
        if (window->type != ECORE_WL2_WINDOW_TYPE_NONE)
          WRN("Commit before previous commit processed");
     }
   if (!window->pending.configure)
     {
        if (window->has_buffer)
          window->commit_pending = EINA_TRUE;
        window->callback = wl_surface_frame(window->surface);
        wl_callback_add_listener(window->callback, &_frame_listener, window);
        /* Dispatch any state we've been saving along the way */
        if (window->pending.geom)
          {
             int gx, gy, gw, gh;

             ecore_wl2_window_geometry_get(window, &gx, &gy, &gw, &gh);
             if (window->xdg_toplevel)
               xdg_surface_set_window_geometry(window->xdg_surface,
                                               gx, gy, gw, gh);
             if (window->zxdg_surface)
               zxdg_surface_v6_set_window_geometry(window->zxdg_surface,
                                                   gx, gy, gw, gh);
          }
        if (window->pending.opaque || window->pending.input)
          _regions_set(window);

        if (window->pending.maximized)
          _maximized_set(window);

        if (window->pending.fullscreen)
          _fullscreen_set(window);

        window->pending.geom = EINA_FALSE;
        window->pending.opaque = EINA_FALSE;
        window->pending.input = EINA_FALSE;
        window->pending.maximized = EINA_FALSE;
        window->pending.fullscreen = EINA_FALSE;
     }

   if (window->req_config.serial != window->set_config.serial)
     {
        if (window->xdg_configure_ack)
           window->xdg_configure_ack(window->xdg_surface,
                                      window->req_config.serial);
        if (window->zxdg_configure_ack)
           window->zxdg_configure_ack(window->zxdg_surface,
                                      window->req_config.serial);
        window->set_config.serial = window->req_config.serial;
     }
   if (flush)
     {
        wl_surface_commit(window->surface);
        ecore_wl2_display_flush(window->display);
     }

   if (!window->updating) return;

   window->updating = EINA_FALSE;
   if (window->def_config.serial != window->set_config.serial)
      _ecore_wl2_window_configure_send(window);
}

/**
 * @brief Performs a "false" commit on the window's surface.
 *
 * A false commit involves registering a frame callback and committing the
 * surface, but it's typically used in scenarios where no new buffer content
 * is being attached, yet a frame callback is desired for timing or
 * synchronization purposes (e.g., to drive animations when the content
 * itself hasn't changed but needs to be re-evaluated at the next frame).
 * If the window `has_buffer`, `commit_pending` will be set.
 * This function should not be called if a configure is pending or a commit
 * is already pending.
 *
 * @param window The window for which to perform a false commit.
 */
EAPI void
ecore_wl2_window_false_commit(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_NULL_RETURN(window->surface);
   EINA_SAFETY_ON_TRUE_RETURN(window->pending.configure);
   EINA_SAFETY_ON_TRUE_RETURN(window->commit_pending);

   window->callback = wl_surface_frame(window->surface);
   wl_callback_add_listener(window->callback, &_frame_listener, window);
   wl_surface_commit(window->surface);
   ecore_wl2_display_flush(window->display);
   if (window->has_buffer)
     window->commit_pending = EINA_TRUE;
}

/**
 * @brief Checks if a commit is pending for the window.
 *
 * A commit is pending if a frame callback has been registered and is waiting
 * to be triggered by the compositor. This usually means the client has
 * updated the surface state and is waiting for the compositor to process it
 * and signal readiness for the next frame.
 *
 * @param window The window to query.
 * @return EINA_TRUE if a commit is pending, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_pending_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   return window->commit_pending;
}

/**
 * @brief Adds a frame callback to be invoked for the window.
 *
 * Frame callbacks are functions that get called when the compositor signals
 * it's time to draw a new frame (via wl_surface.frame). Multiple callbacks
 * can be registered per window. They are invoked after the main wl_callback
 * for the frame event is processed.
 *
 * @param window The window to add the callback to.
 * @param cb The Ecore_Wl2_Frame_Cb function to call.
 *           Prototype: `void (*Ecore_Wl2_Frame_Cb)(Ecore_Wl2_Window *window, uint32_t timestamp, void *data)`
 * @param data User data to be passed to the callback function.
 * @return A handle to the registered callback, or NULL on failure. This handle
 *         is used with ecore_wl2_window_frame_callback_del() to remove the callback.
 */
EAPI Ecore_Wl2_Frame_Cb_Handle *
ecore_wl2_window_frame_callback_add(Ecore_Wl2_Window *window, Ecore_Wl2_Frame_Cb cb, void *data)
{
   Ecore_Wl2_Frame_Cb_Handle *callback;

   EINA_SAFETY_ON_NULL_RETURN_VAL(window, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cb, NULL);

   callback = malloc(sizeof(*callback));
   EINA_SAFETY_ON_NULL_RETURN_VAL(callback, NULL);
   callback->cb = cb;
   callback->data = data;
   callback->win = window;
   window->frame_callbacks =
     eina_inlist_append(window->frame_callbacks, EINA_INLIST_GET(callback));
   return callback;
}

/**
 * @brief Deletes a previously added frame callback.
 *
 * @param handle The handle returned by ecore_wl2_window_frame_callback_add().
 */
EAPI void
ecore_wl2_window_frame_callback_del(Ecore_Wl2_Frame_Cb_Handle *handle)
{
   EINA_SAFETY_ON_NULL_RETURN(handle);

   handle->win->frame_callbacks =
     eina_inlist_remove(handle->win->frame_callbacks, EINA_INLIST_GET(handle));
   free(handle);
}

/**
 * @brief Attaches a buffer to the window's surface.
 *
 * This function associates a Wayland buffer (e.g., a wl_buffer created from
 * shared memory or a hardware buffer) with the window's wl_surface.
 * The `x` and `y` parameters specify the offset of the buffer relative to the
 * surface, though they are often 0.
 * If `implicit` is EINA_FALSE, `wl_surface_attach()` is called explicitly.
 * If `implicit` is EINA_TRUE, it's assumed the attach is handled elsewhere
 * (e.g., by a graphics library integrating with Wayland), and this function
 * mainly updates Ecore_Wl2's internal state (`win->buffer`, `win->has_buffer`).
 *
 * @param win The window whose surface the buffer will be attached to.
 * @param buffer The Wayland buffer (e.g., struct wl_buffer *) to attach.
 *               Can be NULL to detach the current buffer.
 * @param x The x offset for attaching the buffer.
 * @param y The y offset for attaching the buffer.
 * @param implicit If EINA_TRUE, wl_surface_attach is not called directly by this function.
 */
EAPI void
ecore_wl2_window_buffer_attach(Ecore_Wl2_Window *win, void *buffer, int x, int y, Eina_Bool implicit)
{
   EINA_SAFETY_ON_NULL_RETURN(win);
   EINA_SAFETY_ON_NULL_RETURN(win->surface);

   /* FIXME: Haven't given any thought to x and y since we always use 0... */
   if (!implicit) wl_surface_attach(win->surface, buffer, x, y);
   win->buffer = buffer;
   if (!implicit && !buffer)
     win->has_buffer = EINA_FALSE;
   else
     win->has_buffer = EINA_TRUE;
}

/**
 * @brief Gets the resizing state of the window.
 *
 * This reflects whether the compositor has indicated in the most recent
 * configure event that the window is currently being resized (e.g., by the user).
 *
 * @param window The window to query.
 * @return EINA_TRUE if the window is in a resizing state, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl2_window_resizing_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, EINA_FALSE);

   return window->req_config.resizing;
}

/**
 * @brief Marks the beginning of a window update sequence.
 *
 * This function sets an internal flag (`window->updating`) to EINA_TRUE.
 * This flag can be used to batch certain operations or defer actions until
 * the update sequence is complete (signaled by a commit or other means).
 * It helps prevent processing configure events while the client is in the
 * middle of preparing a new frame.
 *
 * @param window The window beginning an update.
 */
EAPI void
ecore_wl2_window_update_begin(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN(window);
   EINA_SAFETY_ON_TRUE_RETURN(window->updating);

   window->updating = EINA_TRUE;
}

/**
 * @brief Marks regions of the window's surface as damaged (needs redraw).
 *
 * This informs the compositor which parts of the surface have changed since
 * the last commit. The compositor can use this information to optimize
 * rendering by only recompositing the damaged areas.
 * Depending on the compositor's version, this will use either
 * `wl_surface_damage` (older) or `wl_surface_damage_buffer` (newer,
 * coordinates relative to the buffer).
 * If `rects` is NULL or `count` is 0, the entire surface is marked as damaged.
 *
 * @param window The window whose surface is to be damaged.
 * @param rects An array of Eina_Rectangle structs defining the damaged areas.
 *              Coordinates are relative to the surface or buffer.
 *              Example: `Eina_Rectangle rects[] = {{10, 10, 100, 50}, {0, 0, 20, 20}};`
 * @param count The number of rectangles in the `rects` array.
 */
EAPI void
ecore_wl2_window_damage(Ecore_Wl2_Window *window, Eina_Rectangle *rects, unsigned int count)
{
   void (*damage)(struct wl_surface *, int32_t, int32_t, int32_t, int32_t);
   unsigned int k;
   int compositor_version;

   EINA_SAFETY_ON_NULL_RETURN(window);

   compositor_version = window->display->wl.compositor_version;

   if (compositor_version >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
     damage = wl_surface_damage_buffer;
   else
     damage = wl_surface_damage;

   if ((rects) && (count > 0))
     for (k = 0; k < count; k++)
       damage(window->surface, rects[k].x, rects[k].y, rects[k].w, rects[k].h);
   else
     damage(window->surface, 0, 0, INT_MAX, INT_MAX);
}

/**
 * @brief Flushes (and optionally purges) the underlying Ecore_Wl2_Surface.
 *
 * If the Ecore_Wl2_Window is associated with an Ecore_Wl2_Surface (which
 * typically manages a shm buffer), this function calls ecore_wl2_surface_flush()
 * on that surface. This might involve operations like posting the buffer to
 * a display server or freeing resources if `purge` is true.
 *
 * @param window The window whose associated Ecore_Wl2_Surface should be flushed.
 * @param purge If EINA_TRUE, resources of the Ecore_Wl2_Surface might be released.
 */
EAPI void
ecore_wl2_window_surface_flush(Ecore_Wl2_Window *window, Eina_Bool purge)
{
   EINA_SAFETY_ON_NULL_RETURN(window);

   if (!window->wl2_surface) return;
   ecore_wl2_surface_flush(window->wl2_surface, purge);
}

/**
 * @brief Gets the type of the Ecore_Wl2_Window.
 *
 * @param window The window to query.
 * @return The Ecore_Wl2_Window_Type of the window.
 * @see ecore_wl2_window_type_set()
 */
EAPI Ecore_Wl2_Window_Type
ecore_wl2_window_type_get(Ecore_Wl2_Window *window)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(window, ECORE_WL2_WINDOW_TYPE_NONE);
   return window->type;
}

/**
 * @brief Finds an Ecore_Wl2_Window associated with a given wl_surface.
 *
 * This function iterates through the windows known to the current Ecore_Wl2
 * display connection and returns the Ecore_Wl2_Window whose wl_surface
 * matches the provided one.
 *
 * @param surface The wl_surface to find the Ecore_Wl2_Window for.
 * @return The Ecore_Wl2_Window associated with the surface, or NULL if not found
 *         or if the display connection is not available.
 */
EAPI Ecore_Wl2_Window *
ecore_wl2_window_surface_find(struct wl_surface *surface)
{
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Window *win;

   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, NULL);

   ewd = ecore_wl2_connected_display_get(NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ewd, NULL);

   win = ecore_wl2_display_window_find_by_surface(ewd, surface);
   return win;
}
