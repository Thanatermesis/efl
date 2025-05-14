#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl_private.h"
#include "xdg-shell-client-protocol.h"
#include "session-recovery-client-protocol.h"

/* local function prototypes */
static void _ecore_wl_window_cb_ping(void *data EINA_UNUSED, struct wl_shell_surface *shell_surface, unsigned int serial);
static void _ecore_wl_window_cb_configure(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED, unsigned int edges, int w, int h);
static void _ecore_wl_window_cb_popup_done(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED);
static void _ecore_wl_window_configure_send(Ecore_Wl_Window *win, int w, int h, int edges);
static char *_ecore_wl_window_id_str_get(unsigned int win_id);
static void _ecore_xdg_handle_surface_configure(void *data, struct xdg_surface *xdg_surface, int32_t width, int32_t height,struct wl_array *states, uint32_t serial);
static void _ecore_xdg_handle_surface_delete(void *data, struct xdg_surface *xdg_surface);
static void _ecore_xdg_handle_popup_done(void *data, struct xdg_popup *xdg_popup);
static void _ecore_session_recovery_uuid(void *data, struct zwp_e_session_recovery *session_recovery, const char *uuid);

/* local variables */
/**
 * @internal
 * @brief Hash table storing all Ecore_Wl_Window instances, keyed by a string representation of their ID.
 */
static Eina_Hash *_windows = NULL;

/* wayland listeners */
static const struct wl_shell_surface_listener _ecore_wl_shell_surface_listener =
{
   _ecore_wl_window_cb_ping,
   _ecore_wl_window_cb_configure,
   _ecore_wl_window_cb_popup_done
};

static const struct xdg_surface_listener _ecore_xdg_surface_listener =
{
   _ecore_xdg_handle_surface_configure,
   _ecore_xdg_handle_surface_delete,
};

static const struct xdg_popup_listener _ecore_xdg_popup_listener =
{
   _ecore_xdg_handle_popup_done,
};

static const struct zwp_e_session_recovery_listener _ecore_session_recovery_listener =
{
   _ecore_session_recovery_uuid,
};

/* internal functions */
/**
 * @internal
 * @brief Initializes the Ecore Wayland window subsystem.
 *
 * This function creates the hash table used to store Ecore_Wl_Window instances.
 * It should be called once during Ecore_Wl initialization.
 */
void
_ecore_wl_window_init(void)
{
   if (!_windows)
     _windows = eina_hash_string_superfast_new(NULL);
}

/**
 * @internal
 * @brief Shuts down the Ecore Wayland window subsystem.
 *
 * This function frees the hash table used to store Ecore_Wl_Window instances.
 * It should be called once during Ecore_Wl shutdown.
 */
void
_ecore_wl_window_shutdown(void)
{
   eina_hash_free(_windows);
   _windows = NULL;
}

/**
 * @internal
 * @brief Retrieves the hash table of Ecore Wayland windows.
 *
 * @return A pointer to the Eina_Hash containing all Ecore_Wl_Window instances.
 */
Eina_Hash *
_ecore_wl_window_hash_get(void)
{
   return _windows;
}

/**
 * @internal
 * @brief Initializes the shell surface for a given Ecore Wayland window.
 *
 * This function creates and configures the appropriate shell surface
 * (xdg_surface, wl_shell_surface, or ivi_surface) based on the
 * window type and available shell extensions.
 *
 * @param win The Ecore_Wl_Window to initialize the shell surface for.
 */
void
_ecore_wl_window_shell_surface_init(Ecore_Wl_Window *win)
{
#ifdef USE_IVI_SHELL
   char *env;
#endif

   if ((win->type == ECORE_WL_WINDOW_TYPE_DND) ||
       (win->type == ECORE_WL_WINDOW_TYPE_NONE)) return;
#ifdef USE_IVI_SHELL
   if ((!win->ivi_surface) && (_ecore_wl_disp->wl.ivi_application))
     {
        if (win->parent && win->parent->ivi_surface)
          win->ivi_surface_id = win->parent->ivi_surface_id + 1;
        else if ((env = getenv("ECORE_IVI_SURFACE_ID")))
          win->ivi_surface_id = atoi(env);
        else
          win->ivi_surface_id = IVI_SURFACE_ID + getpid();

        win->ivi_surface =
          ivi_application_surface_create(_ecore_wl_disp->wl.ivi_application,
                                         win->ivi_surface_id, win->surface);
     }

   if (!win->ivi_surface)
     {
#endif
        if (_ecore_wl_disp->wl.xdg_shell)
          {
             if (win->xdg_surface) return;
             win->xdg_surface =
               xdg_shell_get_xdg_surface(_ecore_wl_disp->wl.xdg_shell,
                                         win->surface);
             if (!win->xdg_surface) return;
             if (win->title)
               xdg_surface_set_title(win->xdg_surface, win->title);
             if (win->class_name)
               xdg_surface_set_app_id(win->xdg_surface, win->class_name);
             xdg_surface_set_user_data(win->xdg_surface, win);
             xdg_surface_add_listener(win->xdg_surface,
                                      &_ecore_xdg_surface_listener, win);
          }
        else if (_ecore_wl_disp->wl.shell)
          {
             if (win->shell_surface) return;
             win->shell_surface =
               wl_shell_get_shell_surface(_ecore_wl_disp->wl.shell,
                                          win->surface);
             if (!win->shell_surface) return;

             if (win->title)
               wl_shell_surface_set_title(win->shell_surface, win->title);

             if (win->class_name)
               wl_shell_surface_set_class(win->shell_surface, win->class_name);
          }

        if (win->shell_surface)
          wl_shell_surface_add_listener(win->shell_surface,
                                        &_ecore_wl_shell_surface_listener, win);
#ifdef USE_IVI_SHELL
     }
#endif

   /* trap for valid shell surface */
   if ((!win->xdg_surface) && (!win->shell_surface)) return;

   switch (win->type)
     {
      case ECORE_WL_WINDOW_TYPE_FULLSCREEN:
        if (win->xdg_surface)
          xdg_surface_set_fullscreen(win->xdg_surface, NULL);
        else if (win->shell_surface)
          wl_shell_surface_set_fullscreen(win->shell_surface,
                                          WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                          0, NULL);
        break;
      case ECORE_WL_WINDOW_TYPE_MAXIMIZED:
        if (win->xdg_surface)
          xdg_surface_set_maximized(win->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_maximized(win->shell_surface, NULL);
        break;
      case ECORE_WL_WINDOW_TYPE_TRANSIENT:
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, win->parent->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_transient(win->shell_surface,
                                         win->parent->surface,
                                         win->allocation.x,
                                         win->allocation.y, 0);
        break;
      case ECORE_WL_WINDOW_TYPE_MENU:
        if (win->xdg_surface)
          {
             win->xdg_popup =
               xdg_shell_get_xdg_popup(_ecore_wl_disp->wl.xdg_shell,
                                       win->surface,
                                       win->parent->surface,
                                       _ecore_wl_disp->input->seat,
                                       _ecore_wl_disp->serial,
                                       win->allocation.x, win->allocation.y);
             if (!win->xdg_popup) return;
             xdg_popup_set_user_data(win->xdg_popup, win);
             xdg_popup_add_listener(win->xdg_popup,
                                    &_ecore_xdg_popup_listener, win);
          }
        else if (win->shell_surface)
          wl_shell_surface_set_popup(win->shell_surface,
                                     _ecore_wl_disp->input->seat,
                                     _ecore_wl_disp->serial,
                                     win->parent->surface,
                                     win->allocation.x, win->allocation.y, 0);
        break;
      case ECORE_WL_WINDOW_TYPE_TOPLEVEL:
        if (win->xdg_surface)
          xdg_surface_set_parent(win->xdg_surface, NULL);
        else if (win->shell_surface)
          wl_shell_surface_set_toplevel(win->shell_surface);
        break;
      default:
        break;
     }
}

EAPI Ecore_Wl_Window *
ecore_wl_window_new(Ecore_Wl_Window *parent, int x, int y, int w, int h, int buffer_type)
{
   Ecore_Wl_Window *win;
   static int _win_id = 1; /**< @internal Static counter for generating unique window IDs. */

   LOGFN;

   if (!(win = calloc(1, sizeof(Ecore_Wl_Window))))
     {
        ERR("Failed to allocate an Ecore Wayland Window");
        return NULL;
     }

   win->display = _ecore_wl_disp;
   win->parent = parent;
   win->allocation.x = x;
   win->allocation.y = y;
   win->allocation.w = w;
   win->allocation.h = h;
   win->saved.w = w;
   win->saved.h = h;
   win->transparent = EINA_FALSE;
   win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
   win->buffer_type = buffer_type;
   win->id = _win_id++;
   win->rotation = 0;

   win->opaque.x = x;
   win->opaque.y = y;
   win->opaque.w = w;
   win->opaque.h = h;

   win->title = NULL;
   win->class_name = NULL;

   eina_hash_add(_windows, _ecore_wl_window_id_str_get(win->id), win);

   return win;
}

/**
 * @brief Frees an Ecore Wayland window.
 *
 * This function destroys all resources associated with the given
 * Ecore_Wl_Window, including its Wayland surfaces (wl_surface,
 * shell_surface, xdg_surface, xdg_popup), and removes it from
 * internal tracking.
 *
 * @param win The Ecore_Wl_Window to free.
 */
EAPI void
ecore_wl_window_free(Ecore_Wl_Window *win)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   eina_hash_del(_windows, _ecore_wl_window_id_str_get(win->id), win);

   EINA_INLIST_FOREACH(_ecore_wl_disp->inputs, input)
     {
        if ((input->pointer_focus) && (input->pointer_focus == win))
          input->pointer_focus = NULL;
        if ((input->keyboard_focus) && (input->keyboard_focus == win))
          {
             input->keyboard_focus = NULL;
             ecore_timer_del(input->repeat.tmr);
             input->repeat.tmr = NULL;
          }
     }

   if (win->anim_callback) wl_callback_destroy(win->anim_callback);
   win->anim_callback = NULL;

   if (win->subsurfs) _ecore_wl_subsurfs_del_all(win);

#ifdef USE_IVI_SHELL
   if (win->ivi_surface) ivi_surface_destroy(win->ivi_surface);
   win->ivi_surface = NULL;
#endif
   if (win->xdg_surface) xdg_surface_destroy(win->xdg_surface);
   win->xdg_surface = NULL;
   if (win->xdg_popup) xdg_popup_destroy(win->xdg_popup);
   win->xdg_popup = NULL;

   if (win->shell_surface) wl_shell_surface_destroy(win->shell_surface);
   win->shell_surface = NULL;
   if (win->surface) wl_surface_destroy(win->surface);
   win->surface = NULL;

   if (win->title) eina_stringshare_del(win->title);
   if (win->class_name) eina_stringshare_del(win->class_name);

   /* HMMM, why was this disabled ? */
   free(win);
}

EAPI void
ecore_wl_window_move(Ecore_Wl_Window *win, int x, int y)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   // Store the current location before potentially changing it via shell move.
   // The actual move is requested from the compositor.
   input = win->keyboard_device;
   ecore_wl_window_update_location(win, x, y);

   if ((!input) && (win->parent))
     {
        if (!(input = win->parent->keyboard_device))
          input = win->parent->pointer_device;
     }

   if ((!input) || (!input->seat)) return;

   _ecore_wl_input_grab_release(input, win);

   if (win->xdg_surface)
     xdg_surface_move(win->xdg_surface, input->seat, input->display->serial);
   else if (win->shell_surface)
     wl_shell_surface_move(win->shell_surface, input->seat,
                           input->display->serial);
}

EAPI void
ecore_wl_window_resize(Ecore_Wl_Window *win, int w EINA_UNUSED, int h EINA_UNUSED, int location)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   // The actual resize is requested from the compositor.
   // `location` parameter usually refers to wl_shell_surface_resize's `edges`
   // or xdg_surface_resize's `edges`.
   input = win->keyboard_device;

   if ((!input) && (win->parent))
     {
        if (!(input = win->parent->keyboard_device))
          input = win->parent->pointer_device;
     }

   if ((!input) || (!input->seat)) return;

   _ecore_wl_input_grab_release(input, win);

   if (win->xdg_surface)
     xdg_surface_resize(win->xdg_surface, input->seat,
                        input->display->serial, location);
   else if (win->shell_surface)
     wl_shell_surface_resize(win->shell_surface, input->seat,
                             input->display->serial, location);
}

EAPI void
ecore_wl_window_damage(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   // Damage must be applied before wl_surface_commit.
   if (win->surface) wl_surface_damage(win->surface, x, y, w, h);
}

/**
 * @brief Commits pending changes to a Wayland surface.
 *
 * This function calls wl_surface_commit on the window's surface, making
 * any pending state changes (like damage, attached buffer) visible.
 *
 * @param win The Ecore_Wl_Window whose surface to commit.
 */
EAPI void
ecore_wl_window_commit(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   // The commented out win->has_buffer check might be relevant if
   // committing without a buffer has unintended side effects or is invalid.
   if ((win->surface))// && (win->has_buffer))
     wl_surface_commit(win->surface);
}

/**
 * @brief Attaches a Wayland buffer to an Ecore Wayland window.
 *
 * This function attaches the given wl_buffer to the window's surface.
 * For SHM and EGL_IMAGE buffer types, it also damages the entire surface
 * and commits the changes.
 *
 * @param win The Ecore_Wl_Window to attach the buffer to.
 * @param buffer The wl_buffer to attach. Can be NULL to detach.
 * @param x The x-coordinate offset for attaching the buffer.
 * @param y The y-coordinate offset for attaching the buffer.
 */
EAPI void
ecore_wl_window_buffer_attach(Ecore_Wl_Window *win, struct wl_buffer *buffer, int x, int y)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   switch (win->buffer_type)
     {
      case ECORE_WL_WINDOW_BUFFER_TYPE_EGL_WINDOW:
        break;
      case ECORE_WL_WINDOW_BUFFER_TYPE_EGL_IMAGE:
      case ECORE_WL_WINDOW_BUFFER_TYPE_SHM:
        if (win->surface)
          {
             win->has_buffer = (buffer != NULL);

             /* if (buffer) */
             wl_surface_attach(win->surface, buffer, x, y);
             wl_surface_damage(win->surface, 0, 0,
                               win->allocation.w, win->allocation.h);
             ecore_wl_window_commit(win);
          }
        break;
      default:
        return;
     }
}

EAPI struct wl_surface *
ecore_wl_window_surface_create(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, NULL);

   char uuid[37]; // Buffer for string representation of UUID.

   if (win->surface) return win->surface;
   win->surface = wl_compositor_create_surface(_ecore_wl_compositor_get());
   if (!win->surface) return NULL;

   if (_ecore_wl_disp->wl.session_recovery && getenv("EFL_WAYLAND_SESSION_RECOVERY"))
     {
        zwp_e_session_recovery_add_listener(_ecore_wl_disp->wl.session_recovery,
                                      &_ecore_session_recovery_listener, win);
        if (!uuid_is_null(win->uuid))
          {
             uuid_unparse(win->uuid, uuid);
             zwp_e_session_recovery_provide_uuid(_ecore_wl_disp->wl.session_recovery, uuid);
          }
     }
   win->surface_id = wl_proxy_get_id((struct wl_proxy *)win->surface);
   return win->surface;
}

EAPI void
ecore_wl_window_show(Ecore_Wl_Window *win)
{
   LOGFN;

   if (!win) return;

   // Ensure the underlying wl_surface is created.
   ecore_wl_window_surface_create(win);

   // Initialize the shell-specific surface (xdg, wl_shell, etc.).
   _ecore_wl_window_shell_surface_init(win);
}

/**
 * @brief Hides an Ecore Wayland window.
 *
 * This function effectively unmaps the window by destroying its
 * associated shell surfaces (xdg_surface, xdg_popup, wl_shell_surface)
 * and the underlying wl_surface.
 *
 * @param win The Ecore_Wl_Window to hide.
 */
EAPI void
ecore_wl_window_hide(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   // Destroy shell-specific surfaces first.
   if (win->xdg_surface) xdg_surface_destroy(win->xdg_surface);
   win->xdg_surface = NULL;

   if (win->xdg_popup) xdg_popup_destroy(win->xdg_popup);
   win->xdg_popup = NULL;

   if (win->shell_surface) wl_shell_surface_destroy(win->shell_surface);
   win->shell_surface = NULL;

   if (win->surface) wl_surface_destroy(win->surface);
   win->surface = NULL;
}

EAPI void
ecore_wl_window_raise(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   /* FIXME: This should raise the xdg surface also */
   if (win->shell_surface)
     wl_shell_surface_set_toplevel(win->shell_surface);
}

EAPI void
ecore_wl_window_maximized_set(Ecore_Wl_Window *win, Eina_Bool maximized)
{
   Eina_Bool prev;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   prev = win->maximized;
   maximized = !!maximized;
   if (prev == maximized) return;

   if (maximized)
     {
        if (win->xdg_surface)
          xdg_surface_set_maximized(win->xdg_surface);
        else if (win->shell_surface)
          wl_shell_surface_set_maximized(win->shell_surface, NULL);
        win->type = ECORE_WL_WINDOW_TYPE_MAXIMIZED;
     }
   else
     {
        if (win->xdg_surface)
          xdg_surface_unset_maximized(win->xdg_surface);
        else if (win->shell_surface)
          // For wl_shell_surface, unmaximizing means setting it to toplevel.
          wl_shell_surface_set_toplevel(win->shell_surface);
        win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
     }
   win->maximized = maximized;
}

/**
 * @brief Gets the maximized state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return @c EINA_TRUE if the window is maximized, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl_window_maximized_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, EINA_FALSE);

   return win->maximized;
}

/**
 * @brief Sets the fullscreen state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param fullscreen @c EINA_TRUE to set fullscreen, @c EINA_FALSE to unset.
 */
EAPI void
ecore_wl_window_fullscreen_set(Ecore_Wl_Window *win, Eina_Bool fullscreen)
{
   Eina_Bool prev;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   prev = win->fullscreen;
   fullscreen = !!fullscreen;
   if (prev == fullscreen) return;

   if (fullscreen)
     {
        win->type = ECORE_WL_WINDOW_TYPE_FULLSCREEN;

        if (win->xdg_surface)
          xdg_surface_set_fullscreen(win->xdg_surface, NULL); // NULL output means compositor picks.
        else if (win->shell_surface)
          wl_shell_surface_set_fullscreen(win->shell_surface,
                                          WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                          0, // refresh rate, 0 for default
                                          NULL); // NULL output means compositor picks.
     }
   else
     {
        if (win->xdg_surface)
          xdg_surface_unset_fullscreen(win->xdg_surface);
        else if (win->shell_surface)
          // For wl_shell_surface, unfullscreening means setting it to toplevel.
          wl_shell_surface_set_toplevel(win->shell_surface);

        win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
     }
   win->fullscreen = fullscreen;
}

/**
 * @brief Gets the fullscreen state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return @c EINA_TRUE if the window is fullscreen, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl_window_fullscreen_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, EINA_FALSE);

   return win->fullscreen;
}

/**
 * @brief Sets the transparent state of an Ecore Wayland window.
 *
 * If set to transparent, the opaque region is cleared. Otherwise,
 * the previously set opaque region is restored.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param transparent @c EINA_TRUE to set transparent, @c EINA_FALSE otherwise.
 */
EAPI void
ecore_wl_window_transparent_set(Ecore_Wl_Window *win, Eina_Bool transparent)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->transparent = transparent;
   if (!win->transparent)
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y,
                                       win->opaque.w, win->opaque.h);
   else // Clear opaque region if transparent
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y, 0, 0);
}

/**
 * @brief Gets the alpha state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return @c EINA_TRUE if the window has an alpha channel, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl_window_alpha_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, EINA_FALSE);

   return win->alpha;
}

/**
 * @brief Sets the alpha state of an Ecore Wayland window.
 *
 * If set to use alpha, the opaque region is cleared. Otherwise,
 * the previously set opaque region is restored. This is similar to
 * transparency but might be handled differently by the compositor
 * or imply per-pixel alpha.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param alpha @c EINA_TRUE to enable alpha, @c EINA_FALSE otherwise.
 */
EAPI void
ecore_wl_window_alpha_set(Ecore_Wl_Window *win, Eina_Bool alpha)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->alpha = alpha;
   if (!win->alpha)
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y,
                                       win->opaque.w, win->opaque.h);
   else // Clear opaque region if alpha is enabled
     ecore_wl_window_opaque_region_set(win, win->opaque.x, win->opaque.y, 0, 0);
}

/**
 * @brief Gets the transparent state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return @c EINA_TRUE if the window is transparent, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl_window_transparent_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, EINA_FALSE);

   return win->transparent;
}

/**
 * @brief Updates the size of an Ecore Wayland window.
 *
 * This function updates the internal allocation size of the window.
 * If the window is not maximized or fullscreen, it also updates the
 * saved size (used for restoring from maximized/fullscreen).
 * For xdg_surface, it also informs the compositor about the new
 * window geometry.
 *
 * @param win The Ecore_Wl_Window to update.
 * @param w The new width.
 * @param h The new height.
 */
EAPI void
ecore_wl_window_update_size(Ecore_Wl_Window *win, int w, int h)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->allocation.w = w;
   win->allocation.h = h;

   // Save dimensions if not maximized or fullscreen for later restoration.
   if ((!ecore_wl_window_maximized_get(win)) && (!win->fullscreen))
     {
        win->saved.w = w;
        win->saved.h = h;
     }

   // For xdg_surface, the client can suggest a new window geometry.
   if (win->xdg_surface)
     xdg_surface_set_window_geometry(win->xdg_surface,
                                     win->allocation.x, win->allocation.y,
                                     win->allocation.w, win->allocation.h);
}

/**
 * @brief Updates the location (position) of an Ecore Wayland window.
 *
 * This function updates the internal allocation position of the window.
 * For xdg_surface, it also informs the compositor about the new
 * window geometry. Note that moving a window is typically initiated
 * by a move request (see ecore_wl_window_move), this function updates
 * the client-side state.
 *
 * @param win The Ecore_Wl_Window to update.
 * @param x The new x-coordinate.
 * @param y The new y-coordinate.
 */
EAPI void
ecore_wl_window_update_location(Ecore_Wl_Window *win, int x, int y)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->allocation.x = x;
   win->allocation.y = y;

   // For xdg_surface, the client can suggest a new window geometry.
   if (win->xdg_surface)
     xdg_surface_set_window_geometry(win->xdg_surface,
                                     win->allocation.x, win->allocation.y,
                                     win->allocation.w, win->allocation.h);
}

/**
 * @brief Gets the underlying wl_surface of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window.
 * @return The associated wl_surface, or @c NULL if none.
 */
EAPI struct wl_surface *
ecore_wl_window_surface_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, NULL);

   return win->surface;
}

/* @since 1.2 */
/**
 * @brief Gets the wl_shell_surface of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window.
 * @return The associated wl_shell_surface, or @c NULL if not using wl_shell or not created.
 * @since 1.2
 */
EAPI struct wl_shell_surface *
ecore_wl_window_shell_surface_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, NULL);

   return win->shell_surface;
}

/* @since 1.11 */
/**
 * @brief Gets the xdg_surface of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window.
 * @return The associated xdg_surface, or @c NULL if not using xdg_shell or not created.
 * @since 1.11
 */
EAPI struct xdg_surface *
ecore_wl_window_xdg_surface_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, NULL);

   return win->xdg_surface;
}

/**
 * @brief Finds an Ecore Wayland window by its unique ID.
 *
 * @param id The unique ID of the window to find.
 * @return The Ecore_Wl_Window if found, otherwise @c NULL.
 */
EAPI Ecore_Wl_Window *
ecore_wl_window_find(unsigned int id)
{
   Ecore_Wl_Window *win = NULL;

   if (!_windows) return NULL;
   win = eina_hash_find(_windows, _ecore_wl_window_id_str_get(id));
   return win;
}

/**
 * @brief Sets the type of an Ecore Wayland window.
 *
 * This function sets the intended role or behavior of the window,
 * such as toplevel, fullscreen, menu, etc. This type is used when
 * initializing the shell surface.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param type The desired Ecore_Wl_Window_Type.
 */
EAPI void
ecore_wl_window_type_set(Ecore_Wl_Window *win, Ecore_Wl_Window_Type type)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->type = type;
}

/**
 * @brief Sets a custom pointer (cursor) surface for an Ecore Wayland window.
 *
 * When the pointer enters this window, the compositor will be asked to
 * display the given surface as the cursor image.
 *
 * @param win The Ecore_Wl_Window for which to set the pointer.
 * @param surface The wl_surface to use as the pointer image.
 * @param hot_x The x-coordinate of the pointer hotspot relative to the surface.
 * @param hot_y The y-coordinate of the pointer hotspot relative to the surface.
 */
EAPI void
ecore_wl_window_pointer_set(Ecore_Wl_Window *win, struct wl_surface *surface, int hot_x, int hot_y)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->pointer.surface = surface;
   win->pointer.hot_x = hot_x;
   win->pointer.hot_y = hot_y;
   win->pointer.set = EINA_TRUE; // Mark that a custom pointer is set

   // If there's an active pointer device associated with this window,
   // apply the cursor change immediately.
   if ((input = win->pointer_device))
     ecore_wl_input_pointer_set(input, surface, hot_x, hot_y);
}

/**
 * @brief Sets the pointer (cursor) for an Ecore Wayland window from a theme name.
 *
 * Requests the compositor to use a cursor from the current cursor theme.
 *
 * @param win The Ecore_Wl_Window for which to set the cursor.
 * @param cursor_name The name of the cursor to use (e.g., "left_ptr").
 */
EAPI void
ecore_wl_window_cursor_from_name_set(Ecore_Wl_Window *win, const char *cursor_name)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->pointer.set = EINA_FALSE; // Mark that a themed cursor is requested

   if (!(input = win->pointer_device))
     return;

   eina_stringshare_replace(&win->cursor_name, cursor_name);

   // Apply if the requested cursor name is different from the current one.
   if ((input->cursor_name) && (strcmp(input->cursor_name, win->cursor_name)))
     ecore_wl_input_cursor_from_name_set(input, cursor_name);
}

/**
 * @brief Restores the default pointer (cursor) for an Ecore Wayland window.
 *
 * Requests the compositor to revert to the default cursor image when the
 * pointer is over this window.
 *
 * @param win The Ecore_Wl_Window for which to restore the default cursor.
 */
EAPI void
ecore_wl_window_cursor_default_restore(Ecore_Wl_Window *win)
{
   Ecore_Wl_Input *input;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->pointer.set = EINA_FALSE; // No custom surface, no specific named cursor

   if ((input = win->pointer_device))
     ecore_wl_input_cursor_default_restore(input);
}

/* @since 1.2 */
/**
 * @brief Sets the parent of an Ecore Wayland window.
 *
 * This is used for transient windows (popups, menus) that should be
 * positioned relative to a parent window.
 *
 * @param win The Ecore_Wl_Window whose parent to set.
 * @param parent The parent Ecore_Wl_Window.
 * @since 1.2
 */
EAPI void
ecore_wl_window_parent_set(Ecore_Wl_Window *win, Ecore_Wl_Window *parent)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->parent = parent;
}

/* @since 1.12 */
/**
 * @brief Sets the iconified (minimized) state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param iconified @c EINA_TRUE to iconify, @c EINA_FALSE to deiconify.
 * @since 1.12
 */
EAPI void
ecore_wl_window_iconified_set(Ecore_Wl_Window *win, Eina_Bool iconified)
{
   Eina_Bool prev;
   struct wl_array states;
   uint32_t *s;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   prev = win->minimized;
   iconified = !!iconified;
   if (prev == iconified) return;

   if (iconified)
     {
        if (win->xdg_surface)
          xdg_surface_set_minimized(win->xdg_surface);
        else if (win->shell_surface)
          {
             /* TODO: handle case of iconifying a wl_shell surface.
              * wl_shell does not have a minimize request. This might involve
              * unmapping or other compositor-specific mechanisms if supported.
              */
          }
     }
   else // De-iconify
     {
        if (win->xdg_surface)
          {
             // To de-iconify an xdg_surface, we typically reconfigure it
             // to its previous state (e.g., activated).
             // Here, it's being configured with its saved size and an ACTIVATED state.
             wl_array_init(&states);
             s = wl_array_add(&states, sizeof(*s));
             *s = XDG_SURFACE_STATE_ACTIVATED;
             // This directly calls the configure handler, which might be
             // unexpected. Usually, the compositor sends configure events.
             // A more typical approach might be to ensure the window is mapped
             // and let the compositor manage its state transitions.
             _ecore_xdg_handle_surface_configure(win, win->xdg_surface, win->saved.w, win->saved.h, &states, 0);
             wl_array_release(&states);
          }
        else if (win->shell_surface)
          // For wl_shell_surface, de-iconifying means setting it to toplevel.
          wl_shell_surface_set_toplevel(win->shell_surface);

        win->type = ECORE_WL_WINDOW_TYPE_TOPLEVEL;
     }

   win->minimized = iconified;
}

/**
 * @brief Gets the iconified (minimized) state of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return @c EINA_TRUE if the window is iconified, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_wl_window_iconified_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, EINA_FALSE);

   return win->minimized;
}

/**
 * @brief Finds an Ecore Wayland window associated with a given wl_surface.
 *
 * Iterates through all known Ecore_Wl_Window instances to find the one
 * that manages the specified wl_surface.
 *
 * @param surface The wl_surface to search for.
 * @return The Ecore_Wl_Window if found, otherwise @c NULL.
 */
EAPI Ecore_Wl_Window *
ecore_wl_window_surface_find(struct wl_surface *surface)
{
   Eina_Iterator *itr;
   Ecore_Wl_Window *win = NULL;
   void *data;

   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, NULL);

   itr = eina_hash_iterator_data_new(_windows);
   while (eina_iterator_next(itr, &data))
     {
        if (((Ecore_Wl_Window *)data)->surface == surface)
          {
             win = data;
             break;
          }
     }

   eina_iterator_free(itr);

   return win;
}

/* @since 1.8 */
/**
 * @brief Sets the input region of an Ecore Wayland window.
 *
 * The input region defines the area of the window that can receive
 * pointer and keyboard events. Pixels outside this region are transparent
 * to input. Coordinates are relative to the window.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param x The x-coordinate of the input region.
 * @param y The y-coordinate of the input region.
 * @param w The width of the input region.
 * @param h The height of the input region.
 * @since 1.8
 */
EAPI void
ecore_wl_window_input_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->input.x = x;
   win->input.y = y;
   win->input.w = w;
   win->input.h = h;

   // DND windows typically don't have input regions in the same way.
   if (win->type != ECORE_WL_WINDOW_TYPE_DND)
     {
        struct wl_region *region;

        region = wl_compositor_create_region(_ecore_wl_compositor_get());
        if (!region) return;

        // The region coordinates need to be adjusted based on window rotation.
        // The current implementation for 180, 90, 270 rotations seems unusual.
        // For example, for 180-degree rotation, (x, y) might become (W-x-w, H-y-h)
        // relative to the unrotated buffer, or the compositor handles this.
        // This needs careful review against Wayland expectations for rotated surfaces.
        // Assuming for now the coordinates are pre-transformed or this is a specific use case.
        switch (win->rotation)
          {
           case 0:
             wl_region_add(region, x, y, w, h);
             break;
           case 180: // This transformation (x, x+y, w, h) is suspicious.
             wl_region_add(region, x, x + y, w, h);
             break;
           case 90:  // This transformation (y, x, h, w) is also suspicious.
             wl_region_add(region, y, x, h, w);
             break;
           case 270: // This transformation (x+y, x, h, w) is suspicious.
             wl_region_add(region, x + y, x, h, w);
             break;
          }

        wl_surface_set_input_region(win->surface, region);
        wl_region_destroy(region);
     }
}

/* @since 1.8 */
/**
 * @brief Sets the opaque region of an Ecore Wayland window.
 *
 * The opaque region is a hint to the compositor about which parts of the
 * window are fully opaque. This can be used for optimization, as the
 * compositor may not need to blend content behind these areas.
 * Coordinates are relative to the window.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param x The x-coordinate of the opaque region.
 * @param y The y-coordinate of the opaque region.
 * @param w The width of the opaque region.
 * @param h The height of the opaque region.
 * @since 1.8
 */
EAPI void
ecore_wl_window_opaque_region_set(Ecore_Wl_Window *win, int x, int y, int w, int h)
{
   struct wl_region *region;

   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->opaque.x = x;
   win->opaque.y = y;
   win->opaque.w = w;
   win->opaque.h = h;

   // If window is marked as transparent or has alpha, opaque region is ignored or cleared.
   if ((win->transparent) || (win->alpha)) return;

   region = wl_compositor_create_region(_ecore_wl_compositor_get());
   if (!region) return;

   // Similar to input_region_set, the rotation transformations here are suspect
   // and need verification against Wayland's coordinate system for rotated surfaces.
   switch (win->rotation)
     {
      case 0:
        wl_region_add(region, x, y, w, h);
        break;
      case 180: // Suspicious transformation
        wl_region_add(region, x, x + y, w, h);
        break;
      case 90:  // Suspicious transformation
        wl_region_add(region, y, x, h, w);
        break;
      case 270: // Suspicious transformation
        wl_region_add(region, x + y, x, h, w);
        break;
     }

   wl_surface_set_opaque_region(win->surface, region);
   wl_region_destroy(region);
}

/* @since 1.8 */
/**
 * @brief Sets the rotation of an Ecore Wayland window.
 *
 * This informs how the window's content should be rotated by the compositor.
 * The actual rendering of rotated content might need to be handled by the client
 * by transforming buffer contents or by the compositor if it supports surface rotation.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param rotation The rotation angle (0, 90, 180, 270 degrees).
 * @since 1.8
 */
EAPI void
ecore_wl_window_rotation_set(Ecore_Wl_Window *win, int rotation)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   win->rotation = rotation;
   // Note: This function only sets the internal state. The client is responsible
   // for attaching buffers with correctly rotated content or ensuring the
   // compositor handles wl_surface.set_buffer_transform if that's the intended mechanism.
}

/* @since 1.8 */
/**
 * @brief Gets the rotation of an Ecore Wayland window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return The rotation angle (0, 90, 180, 270 degrees).
 * @since 1.8
 */
EAPI int
ecore_wl_window_rotation_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, 0);

   return win->rotation;
}

/* @since 1.8 */
/**
 * @brief Gets the unique ID of an Ecore Wayland window.
 *
 * This ID is assigned internally when the window is created.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return The unique ID of the window.
 * @since 1.8
 */
EAPI int
ecore_wl_window_id_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, 0);

   return win->id;
}

/* @since 1.8 */
/**
 * @brief Gets the Wayland object ID of the window's wl_surface.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return The Wayland object ID of the wl_surface, or 0 if no surface.
 * @since 1.8
 */
EAPI int
ecore_wl_window_surface_id_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, 0);

   return win->surface_id;
}

/* @since 1.8 */
/**
 * @brief Sets the title of an Ecore Wayland window.
 *
 * The title is typically displayed in window decorations or task switchers.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param title The title string.
 * @since 1.8
 */
EAPI void
ecore_wl_window_title_set(Ecore_Wl_Window *win, const char *title)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   eina_stringshare_replace(&win->title, title);

   if ((win->xdg_surface) && (win->title))
     xdg_surface_set_title(win->xdg_surface, win->title);
   else if ((win->shell_surface) && (win->title))
     wl_shell_surface_set_title(win->shell_surface, win->title);
}

/* @since 1.8 */
/**
 * @brief Sets the class name (application ID) of an Ecore Wayland window.
 *
 * The class name or app_id is used by the compositor to identify the
 * application, often for grouping windows or applying specific rules.
 *
 * @param win The Ecore_Wl_Window to modify.
 * @param class_name The class name or application ID string.
 * @since 1.8
 */
EAPI void
ecore_wl_window_class_name_set(Ecore_Wl_Window *win, const char *class_name)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN(win);

   eina_stringshare_replace(&win->class_name, class_name);

   if ((win->xdg_surface) && (win->class_name))
     xdg_surface_set_app_id(win->xdg_surface, win->class_name);
   else if ((win->shell_surface) && (win->class_name))
     wl_shell_surface_set_class(win->shell_surface, win->class_name);
}

/* @since 1.8 */
/* Maybe we need an ecore_wl_window_pointer_get() too */
/**
 * @brief Gets the Ecore_Wl_Input device that currently has keyboard focus for this window.
 *
 * @param win The Ecore_Wl_Window to query.
 * @return The Ecore_Wl_Input device with keyboard focus, or @c NULL if none.
 * @since 1.8
 */
EAPI Ecore_Wl_Input *
ecore_wl_window_keyboard_get(Ecore_Wl_Window *win)
{
   LOGFN;

   EINA_SAFETY_ON_NULL_RETURN_VAL(win, 0);

   return win->keyboard_device;
}


/* local functions */
/**
 * @internal
 * @brief Callback for wl_shell_surface ping events.
 *
 * Responds to compositor pings with a pong to indicate liveness.
 *
 * @param data User data (Ecore_Wl_Window).
 * @param shell_surface The wl_shell_surface that received the ping.
 * @param serial The serial of the ping event.
 */
static void
_ecore_wl_window_cb_ping(void *data EINA_UNUSED, struct wl_shell_surface *shell_surface, unsigned int serial)
{
   if (!shell_surface) return;
   wl_shell_surface_pong(shell_surface, serial);
}

/**
 * @internal
 * @brief Callback for wl_shell_surface configure events.
 *
 * Handles requests from the compositor to configure the window's size and state.
 *
 * @param data User data (Ecore_Wl_Window).
 * @param shell_surface The wl_shell_surface being configured.
 * @param edges Hint about which edges are being resized (wl_shell_surface_resize_edge).
 * @param w The new width suggested by the compositor.
 * @param h The new height suggested by the compositor.
 */
static void
_ecore_wl_window_cb_configure(void *data, struct wl_shell_surface *shell_surface EINA_UNUSED, unsigned int edges, int w, int h)
{
   Ecore_Wl_Window *win;

   LOGFN;

   if (!(win = data)) return;

   // Ignore invalid dimensions.
   if ((w <= 0) || (h <= 0)) return;

   // If dimensions changed, send an Ecore event.
   if ((win->allocation.w != w) || (win->allocation.h != h))
     _ecore_wl_window_configure_send(win, w, h, edges);
}

/**
 * @internal
 * @brief Callback for xdg_surface configure events.
 *
 * Handles requests from the compositor to configure the window's size and state.
 * Updates window state flags (maximized, fullscreen, etc.) and sends an
 * Ecore configure event if dimensions change. Finally, acknowledges the
 * configuration.
 *
 * @param data User data (Ecore_Wl_Window).
 * @param xdg_surface The xdg_surface being configured.
 * @param width The new width suggested by the compositor.
 * @param height The new height suggested by the compositor.
 * @param states A wl_array of xdg_surface_state enum values.
 *               Each element is a uint32_t representing a state like
 *               XDG_SURFACE_STATE_MAXIMIZED, XDG_SURFACE_STATE_FULLSCREEN, etc.
 *               Example: If states contains {XDG_SURFACE_STATE_MAXIMIZED, XDG_SURFACE_STATE_ACTIVATED},
 *               the window is maximized and activated.
 * @param serial The serial of the configure event, to be used in ack_configure.
 */
static void
_ecore_xdg_handle_surface_configure(void *data, struct xdg_surface *xdg_surface EINA_UNUSED, int32_t width, int32_t height, struct wl_array *states, uint32_t serial)
{
   Ecore_Wl_Window *win;
   uint32_t *p;

   LOGFN;

   if (!(win = data)) return;

   // Reset states before processing the new ones.
   win->maximized = EINA_FALSE;
   win->fullscreen = EINA_FALSE;
   win->resizing = EINA_FALSE;
   win->focused = EINA_FALSE; // Note: 'activated' usually means focused.

   wl_array_for_each(p, states)
     {
        uint32_t state = *p;
        switch (state)
          {
           case XDG_SURFACE_STATE_MAXIMIZED:
             win->maximized = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_FULLSCREEN:
             win->fullscreen = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_RESIZING:
             win->resizing = EINA_TRUE;
             break;
           case XDG_SURFACE_STATE_ACTIVATED:
             win->focused = EINA_TRUE;
             win->minimized = EINA_FALSE; // Being activated implies not minimized.
             break;
           default:
             break;
          }
     }

   // If valid new dimensions are provided, send an Ecore event.
   // The `edges` parameter is 0 here as xdg_surface.configure doesn't provide edge info directly.
   if ((width > 0) && (height > 0))
     _ecore_wl_window_configure_send(win, width, height, 0);

   // Acknowledge the configuration.
   if (win->xdg_surface)
     xdg_surface_ack_configure(win->xdg_surface, serial);
}

/**
 * @internal
 * @brief Callback for xdg_surface close events (historically, delete request).
 *
 * Handles requests from the compositor to close the window.
 * This typically means the user tried to close the window via window manager controls.
 * The window is freed in response.
 *
 * @param data User data (Ecore_Wl_Window).
 * @param xdg_surface The xdg_surface that received the close request.
 */
static void
_ecore_xdg_handle_surface_delete(void *data, struct xdg_surface *xdg_surface EINA_UNUSED)
{
   Ecore_Wl_Window *win;

   LOGFN;

   if (!(win = data)) return;
   // TODO: This should probably emit an ECORE_WL_EVENT_WINDOW_DELETE_REQUEST
   // event first, to allow the application to intercept and potentially cancel
   // the close operation, rather than freeing directly.
   ecore_wl_window_free(win);
}

/**
 * @internal
 * @brief Callback for wl_shell_surface popup_done events.
 *
 * Indicates that a popup grab (typically for menus) has ended.
 * The input grab associated with the popup should be released.
 *
 * @param data User data (Ecore_Wl_Window, the popup window).
 * @param shell_surface The wl_shell_surface of the popup.
 */
static void
_ecore_wl_window_cb_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
   Ecore_Wl_Window *win;

   LOGFN;

   if (!shell_surface) return;
   if (!(win = data)) return;
   // Assuming win->pointer_device is the input device that initiated the grab.
   ecore_wl_input_ungrab(win->pointer_device);
}

/**
 * @internal
 * @brief Callback for xdg_popup popup_done events.
 *
 * Indicates that an xdg_popup grab has ended (e.g., user clicked outside the popup).
 * The input grab associated with the popup should be released.
 *
 * @param data User data (Ecore_Wl_Window, the popup window).
 * @param xdg_popup The xdg_popup that is done.
 */
static void
_ecore_xdg_handle_popup_done(void *data, struct xdg_popup *xdg_popup)
{
   Ecore_Wl_Window *win;

   LOGFN;

   if (!xdg_popup) return;
   if (!(win = data)) return;
   // Assuming win->pointer_device is the input device that initiated the grab.
   ecore_wl_input_ungrab(win->pointer_device);
}

/**
 * @internal
 * @brief Callback for session recovery UUID events.
 *
 * Receives a UUID from the compositor for session recovery purposes.
 * This UUID can be used to restore the window's state across sessions.
 *
 * @param data User data (Ecore_Wl_Window).
 * @param session_recovery The session recovery object.
 * @param uuid The UUID string provided by the compositor.
 */
static void
_ecore_session_recovery_uuid(void *data EINA_UNUSED, struct zwp_e_session_recovery *session_recovery, const char *uuid)
{
   Ecore_Wl_Window *win;
   char uuid_string[37]; // Standard UUID string length (36 chars + null terminator)

   LOGFN;

   if (!(win = data)) return;
   if (!session_recovery) return; // Should not happen if listener is set
   uuid_parse(uuid, win->uuid); // Parse string UUID into binary form

   uuid_unparse(win->uuid, uuid_string); // For debugging/logging
   DBG("UUID event received from compositor with UUID: %s\n", uuid_string);
}

/**
 * @internal
 * @brief Sends an ECORE_WL_EVENT_WINDOW_CONFIGURE event.
 *
 * This function is called when a window's configuration (size, position, edges)
 * changes, typically in response to a compositor configure event.
 *
 * @param win The Ecore_Wl_Window that was configured.
 * @param w The new width.
 * @param h The new height.
 * @param edges A bitmask indicating which edges were involved in a resize (wl_shell_surface_resize_edge).
 *              For xdg_surface, this might be 0 if not applicable.
 */
static void
_ecore_wl_window_configure_send(Ecore_Wl_Window *win, int w, int h, int edges)
{
   Ecore_Wl_Event_Window_Configure *ev;

   LOGFN;

   if (!(ev = calloc(1, sizeof(Ecore_Wl_Event_Window_Configure)))) return;
   ev->win = win->id;       // Ecore_Wl_Window ID
   ev->event_win = win->id; // Source window ID for the event (same as win->id here)
   ev->x = win->allocation.x; // Current x position
   ev->y = win->allocation.y; // Current y position
   ev->w = w;                 // New width
   ev->h = h;                 // New height
   ev->edges = edges;         // Edges involved in resize (if any)

   ecore_event_add(ECORE_WL_EVENT_WINDOW_CONFIGURE, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Generates a short, fixed-length string representation of a window ID.
 *
 * This is used as a key in the `_windows` hash table. The string is static
 * and will be overwritten on subsequent calls, so it should be used or copied
 * immediately.
 *
 * @param win_id The unsigned integer window ID.
 * @return A pointer to a static char array containing the string ID.
 */
static char *
_ecore_wl_window_id_str_get(unsigned int win_id)
{
   const char *vals = "qWeRtYuIoP5$&<~"; // Character set for encoding (15 chars)
   static char id[9]; // 8 characters + null terminator
   unsigned int val;

   val = win_id;
   id[0] = vals[(val >> 28) & 0xf];
   id[1] = vals[(val >> 24) & 0xf];
   id[2] = vals[(val >> 20) & 0xf];
   id[3] = vals[(val >> 16) & 0xf];
   id[4] = vals[(val >> 12) & 0xf];
   id[5] = vals[(val >> 8) & 0xf];
   id[6] = vals[(val >> 4) & 0xf];
   id[7] = vals[(val) & 0xf];
   id[8] = 0;

   return id;
}
