/**
 * @file
 * @brief Wayland Display handling for Ecore.
 *
 * This file contains the Ecore_Wl2_Display related functions,
 * which manage the connection to a Wayland compositor, handle
 * global Wayland objects, and dispatch events.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"

#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include "efl-hints-client-protocol.h"

static Eina_Hash *_server_displays = NULL; /**< Hash table for server-side displays, keyed by display name */
static Eina_Hash *_client_displays = NULL; /**< Hash table for client-side displays, keyed by display name */

static Eina_Bool _cb_connect_data(void *data, Ecore_Fd_Handler *hdl);
static Eina_Bool _ecore_wl2_display_connect(Ecore_Wl2_Display *ewd, Eina_Bool sync);

static void _ecore_wl2_display_sync_add(Ecore_Wl2_Display *ewd);

/**
 * @internal
 * @brief Frees an Ecore_Wl2_Event_Connect event.
 *
 * This function is called when an ECORE_WL2_EVENT_CONNECT or
 * ECORE_WL2_EVENT_DISCONNECT event is no longer needed. It decrements
 * the reference count of the display and frees the event structure.
 *
 * @param d The Ecore_Wl2_Display associated with the event.
 * @param event The Ecore_Wl2_Event_Connect to free.
 */
void
_display_event_free(void *d, void *event)
{
   ecore_wl2_display_disconnect(d);
   free(event);
}

/**
 * @internal
 * @brief Creates and adds a display connection event.
 *
 * This function allocates and populates an Ecore_Wl2_Event_Connect
 * structure and adds it to the Ecore event queue. This is used for
 * ECORE_WL2_EVENT_CONNECT and ECORE_WL2_EVENT_DISCONNECT events.
 *
 * @param ewd The Ecore_Wl2_Display that connected or disconnected.
 * @param event The type of event to create (ECORE_WL2_EVENT_CONNECT or ECORE_WL2_EVENT_DISCONNECT).
 */
static void
_ecore_wl2_display_event(Ecore_Wl2_Display *ewd, int event)
{
   Ecore_Wl2_Event_Connect *ev;

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Connect));
   EINA_SAFETY_ON_NULL_RETURN(ev);
   ev->display = ewd;
   ewd->refs++;
   ecore_event_add(event, ev, _display_event_free, ewd);
}

/**
 * @internal
 * @brief Signals Ecore to exit.
 *
 * This function creates and adds an ECORE_EVENT_SIGNAL_EXIT event to
 * the Ecore event queue, typically used when a fatal Wayland error occurs.
 */
static void
_ecore_wl2_display_signal_exit(void)
{
   Ecore_Event_Signal_Exit *ev;

   ev = calloc(1, sizeof(Ecore_Event_Signal_Exit));
   if (!ev) return;

   ev->quit = EINA_TRUE;
   ecore_event_add(ECORE_EVENT_SIGNAL_EXIT, ev, NULL, NULL);
}

/**
 * @internal
 * @brief Callback for zwp_linux_dmabuf_v1.format event.
 * @since 1.18
 *
 * This callback is supposed to be invoked by the compositor to announce
 * supported DMA-BUF formats. However, it's noted that this might not
 * always happen.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param dmabuf The zwp_linux_dmabuf_v1 object.
 * @param format The supported DMA-BUF format.
 */
static void
_dmabuf_cb_format(void *data EINA_UNUSED, struct zwp_linux_dmabuf_v1 *dmabuf EINA_UNUSED, uint32_t format EINA_UNUSED)
{
   /* It would be awfully nice if this actually happened */
};

static const struct zwp_linux_dmabuf_v1_listener _dmabuf_listener =
{
   _dmabuf_cb_format,
   NULL /* modifier */
};

/**
 * @internal
 * @brief Callback for xdg_wm_base.ping event.
 *
 * Responds to a ping event from the compositor for the xdg_wm_base interface.
 * This is used to check if the client is responsive.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param shell The xdg_wm_base object.
 * @param serial The serial of the ping event.
 */
static void
_xdg_shell_cb_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
   xdg_wm_base_pong(shell, serial);
   ecore_wl2_display_flush(data);
}

static const struct xdg_wm_base_listener _xdg_shell_listener =
{
   _xdg_shell_cb_ping,
};

/**
 * @internal
 * @brief Callback for zxdg_shell_v6.ping event.
 *
 * Responds to a ping event from the compositor for the zxdg_shell_v6 interface.
 * This is used to check if the client is responsive.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param shell The zxdg_shell_v6 object.
 * @param serial The serial of the ping event.
 */
static void
_zxdg_shell_cb_ping(void *data, struct zxdg_shell_v6 *shell, uint32_t serial)
{
   zxdg_shell_v6_pong(shell, serial);
   ecore_wl2_display_flush(data);
}

static const struct zxdg_shell_v6_listener _zxdg_shell_listener =
{
   _zxdg_shell_cb_ping,
};

/**
 * @internal
 * @brief Callback for zwp_e_session_recovery.create_uuid event.
 * @since 1.20
 *
 * Associates a UUID with a window surface for session recovery purposes.
 * The UUID is provided by the compositor.
 *
 * @param data User data (unused).
 * @param session_recovery The zwp_e_session_recovery object (unused).
 * @param surface The wl_surface to associate the UUID with.
 * @param uuid The UUID string.
 */
static void
_session_recovery_create_uuid(void *data EINA_UNUSED, struct zwp_e_session_recovery *session_recovery EINA_UNUSED, struct wl_surface *surface, const char *uuid)
{
   Ecore_Wl2_Window *win;

   /* surface may have been destroyed */
   if (!surface) return;
   win = wl_surface_get_user_data(surface);

   eina_stringshare_replace(&win->uuid, uuid);
}

static const struct zwp_e_session_recovery_listener _session_listener =
{
   _session_recovery_create_uuid,
};

/**
 * @internal
 * @brief Callback for efl_aux_hints.supported_aux_hints event.
 * @since 1.20
 *
 * Receives the list of supported auxiliary hints for a given surface
 * from the compositor. Stores these hints in the Ecore_Wl2_Window structure
 * and emits an ECORE_WL2_EVENT_AUX_HINT_SUPPORTED event.
 *
 * The `hints` wl_array contains a series of null-terminated strings.
 * Example: `hints->data` might be "hint1\0hint2\0hint3\0"
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param aux_hints The efl_aux_hints object (unused).
 * @param surface_resource The wl_surface for which hints are supported.
 * @param hints A wl_array containing the names of supported hints.
 * @param num_hints The number of hints in the array.
 */
static void
_aux_hints_supported_aux_hints(void *data, struct efl_aux_hints *aux_hints EINA_UNUSED, struct wl_surface *surface_resource, struct wl_array *hints, uint32_t num_hints)
{
   Ecore_Wl2_Display *ewd = data;
   struct wl_surface *surface = surface_resource;
   Ecore_Wl2_Window *win = NULL;
   char *p = NULL;
   char **str = NULL;
   const char *hint = NULL;
   unsigned int i = 0;
   Ecore_Wl2_Event_Aux_Hint_Supported *ev;

   if (!surface) return;
   win = _ecore_wl2_display_window_surface_find(ewd, surface_resource);
   if (!win) return;

   p = hints->data;
   str = calloc(num_hints, sizeof(char *));
   if (!str) return;

   while ((const char *)p < ((const char *)hints->data + hints->size))
     {
        str[i] = (char *)eina_stringshare_add(p);
        p += strlen(p) + 1;
        i++;
     }
   for (i = 0; i < num_hints; i++)
     {
        hint = eina_stringshare_add(str[i]);
        win->supported_aux_hints =
               eina_list_append(win->supported_aux_hints, hint);
     }
   if (str)
     {
        for (i = 0; i < num_hints; i++)
          {
             if (str[i])
               {
                  eina_stringshare_del(str[i]);
                  str[i] = NULL;
               }
          }
        free(str);
     }

   if (!(ev = calloc(1, sizeof(Ecore_Wl2_Event_Aux_Hint_Supported)))) return;
   ev->win = win;
   ev->display = ewd;
   ewd->refs++;
   ecore_event_add(ECORE_WL2_EVENT_AUX_HINT_SUPPORTED, ev,
                   _display_event_free, ewd);
}

/**
 * @internal
 * @brief Callback for efl_aux_hints.allowed_aux_hint event.
 * @since 1.20
 *
 * Notifies that a specific auxiliary hint has been allowed (or processed)
 * by the compositor for a given surface. Emits an
 * ECORE_WL2_EVENT_AUX_HINT_ALLOWED event.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param aux_hints The efl_aux_hints object (unused).
 * @param surface_resource The wl_surface for which the hint was allowed.
 * @param id The ID of the allowed hint.
 */
static void
_aux_hints_allowed_aux_hint(void *data, struct efl_aux_hints *aux_hints  EINA_UNUSED, struct wl_surface *surface_resource, int id)
{
   struct wl_surface *surface = surface_resource;
   Ecore_Wl2_Window *win = NULL;
   Ecore_Wl2_Display *ewd = data;
   Ecore_Wl2_Event_Aux_Hint_Allowed *ev;

   if (!surface) return;
   win = _ecore_wl2_display_window_surface_find(ewd, surface_resource);
   if (!win) return;

   if (!(ev = calloc(1, sizeof(Ecore_Wl2_Event_Aux_Hint_Allowed)))) return;
   ev->win = win;
   ev->id = id;
   ev->display = ewd;
   ewd->refs++;
   ecore_event_add(ECORE_WL2_EVENT_AUX_HINT_ALLOWED, ev,
                   _display_event_free, ewd);
}

/**
 * @internal
 * @brief Frees an Ecore_Wl2_Event_Aux_Message event.
 * @since 1.20
 *
 * This function is called when an ECORE_WL2_EVENT_AUX_MESSAGE event
 * is no longer needed. It decrements the display reference count and
 * frees the event structure, including stringshared key, value, and options.
 *
 * @param data User data (unused).
 * @param event The Ecore_Wl2_Event_Aux_Message to free.
 */
 static void
_cb_aux_message_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Aux_Message *ev;
   char *str;

   ev = event;
   ecore_wl2_display_disconnect(ev->display);
   eina_stringshare_del(ev->key);
   eina_stringshare_del(ev->val);
   EINA_LIST_FREE(ev->options, str)
     eina_stringshare_del(str);
   free(ev);
}

/**
 * @internal
 * @brief Callback for efl_aux_hints.aux_message event.
 * @since 1.20
 *
 * Receives an auxiliary message (key-value pair with optional parameters)
 * from the compositor for a given surface. Emits an
 * ECORE_WL2_EVENT_AUX_MESSAGE event.
 *
 * The `options` wl_array contains a series of null-terminated strings.
 * Example: `options->data` might be "opt1\0opt2\0opt3\0"
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param aux_hints The efl_aux_hints object (unused).
 * @param surface_resource The wl_surface the message is for.
 * @param key The key of the message.
 * @param val The value of the message.
 * @param options A wl_array containing optional parameters for the message.
 */
 static void
_aux_hints_aux_message(void *data, struct efl_aux_hints *aux_hints EINA_UNUSED, struct wl_surface *surface_resource, const char *key, const char *val, struct wl_array *options)
{
   Ecore_Wl2_Window *win = NULL;
   Ecore_Wl2_Event_Aux_Message *ev;
   char *p = NULL, *str = NULL;
   Eina_List *opt_list = NULL;
   Ecore_Wl2_Display *ewd = data;

   if (!surface_resource) return;
   win = _ecore_wl2_display_window_surface_find(ewd, surface_resource);
   if (!win) return;

   if (!(ev = calloc(1, sizeof(Ecore_Wl2_Event_Aux_Message)))) return;

   if ((options) && (options->size))
     {
        p = options->data;
        while ((const char *)p < ((const char *)options->data + options->size))
          {
             str = (char *)eina_stringshare_add(p);
             opt_list = eina_list_append(opt_list, str);
             p += strlen(p) + 1;
          }
     }

   ev->win = win;
   ev->key = eina_stringshare_add(key);
   ev->val = eina_stringshare_add(val);
   ev->options = opt_list;
   ev->display = ewd;
   ewd->refs++;

   ecore_event_add(ECORE_WL2_EVENT_AUX_MESSAGE, ev, _cb_aux_message_free, NULL);
}

static const struct efl_aux_hints_listener _aux_hints_listener =
{
   _aux_hints_supported_aux_hints,
   _aux_hints_allowed_aux_hint,
   _aux_hints_aux_message,
};

/**
 * @internal
 * @brief Frees an Ecore_Wl2_Event_Global event.
 *
 * This function is called when an ECORE_WL2_EVENT_GLOBAL_ADDED or
 * ECORE_WL2_EVENT_GLOBAL_REMOVED event is no longer needed.
 * It decrements the display reference count and frees the event structure,
 * including the stringshared interface name.
 *
 * @param data User data (unused).
 * @param event The Ecore_Wl2_Event_Global to free.
 */
static void
_cb_global_event_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Global *ev;

   ev = event;
   eina_stringshare_del(ev->interface);
   ecore_wl2_display_disconnect(ev->display);
   free(ev);
}

/**
 * @internal
 * @brief Callback for wl_registry.global event (global object added).
 *
 * This function is called by the Wayland library when the compositor
 * announces a new global object. It binds to known interfaces
 * (like wl_compositor, wl_shm, xdg_wm_base, etc.) and stores information
 * about the global in the display's `globals` hash.
 * It also emits an ECORE_WL2_EVENT_GLOBAL_ADDED event.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param registry The wl_registry object.
 * @param id The unique ID of the global object.
 * @param interface The interface name of the global object (e.g., "wl_compositor").
 * @param version The version of the interface.
 */
static void
_cb_global_add(void *data, struct wl_registry *registry, unsigned int id, const char *interface, unsigned int version)
{
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Event_Global *ev;

   ewd = data;

   /* test to see if we have already added this global to our hash */
   if (!eina_hash_find(ewd->globals, &id))
     {
        Ecore_Wl2_Global *global;

        /* allocate space for new global */
        global = calloc(1, sizeof(Ecore_Wl2_Global));
        if (!global) return;

        global->id = id;
        global->interface = eina_stringshare_add(interface);
        global->version = version;

        /* add this global to our hash */
        if (!eina_hash_add(ewd->globals, &global->id, global))
          {
             eina_stringshare_del(global->interface);
             free(global);
          }
     }
   else
     goto event;

   if (!strcmp(interface, "wl_compositor"))
     {
        Ecore_Wl2_Window *window;
        ewd->wl.compositor_version = MIN(version, 4);
        ewd->wl.compositor =
          wl_registry_bind(registry, id, &wl_compositor_interface,
                           ewd->wl.compositor_version);
        EINA_INLIST_FOREACH(ewd->windows, window)
          _ecore_wl2_window_surface_create(window);
     }
   else if (!strcmp(interface, "wl_subcompositor"))
     {
        ewd->wl.subcompositor =
          wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
     }
   else if (!strcmp(interface, "wl_shm"))
     {
        ewd->wl.shm =
          wl_registry_bind(registry, id, &wl_shm_interface, 1);
     }
   else if (!strcmp(interface, "zwp_linux_dmabuf_v1") && (version >= 2))
     {
        ewd->wl.dmabuf =
          wl_registry_bind(registry, id, &zwp_linux_dmabuf_v1_interface, 2);
        zwp_linux_dmabuf_v1_add_listener(ewd->wl.dmabuf, &_dmabuf_listener, ewd);
        _ecore_wl2_buffer_test(ewd);
        _ecore_wl2_display_sync_add(ewd);
     }
   else if (!strcmp(interface, "wl_data_device_manager"))
     {
        ewd->wl.data_device_manager_version = MIN(version, 3);
        ewd->wl.data_device_manager =
          wl_registry_bind(registry, id, &wl_data_device_manager_interface, ewd->wl.data_device_manager_version);
     }
   else if ((!strcmp(interface, "zwp_e_session_recovery")) &&
            (!no_session_recovery))
     {
        ewd->wl.session_recovery =
          wl_registry_bind(registry, id,
                           &zwp_e_session_recovery_interface, 1);
        zwp_e_session_recovery_add_listener(ewd->wl.session_recovery,
                                            &_session_listener, ewd);
     }
   else if (!strcmp(interface, "efl_aux_hints"))
     {
        Ecore_Wl2_Window *window;
        ewd->wl.efl_aux_hints =
          wl_registry_bind(registry, id,
                           &efl_aux_hints_interface, 1);
        efl_aux_hints_add_listener(ewd->wl.efl_aux_hints, &_aux_hints_listener, ewd);
        EINA_INLIST_FOREACH(ewd->windows, window)
          if (window->surface) efl_aux_hints_get_supported_aux_hints(ewd->wl.efl_aux_hints, window->surface);
     }
   else if (!strcmp(interface, "wl_output"))
     _ecore_wl2_output_add(ewd, id);
   else if (!strcmp(interface, "wl_seat"))
     _ecore_wl2_input_add(ewd, id, version);
   else if (!strcmp(interface, "efl_hints"))
     {
        Ecore_Wl2_Window *window;

        ewd->wl.efl_hints = wl_registry_bind(registry, id, &efl_hints_interface, MIN(version, 2));
        EINA_INLIST_FOREACH(ewd->windows, window)
          {
             if (!window->xdg_surface) continue;
             if (window->aspect.set)
               efl_hints_set_aspect(window->display->wl.efl_hints, window->xdg_surface,
                 window->aspect.w, window->aspect.h, window->aspect.aspect);
             if (window->weight.set)
               efl_hints_set_weight(window->display->wl.efl_hints,
                 window->xdg_surface, window->weight.w, window->weight.h);
          }
     }

event:
   /* allocate space for event structure */
   ev = calloc(1, sizeof(Ecore_Wl2_Event_Global));
   if (!ev) return;

   ev->id = id;
   ev->display = ewd;
   ewd->refs++;
   ev->version = version;
   ev->interface = eina_stringshare_add(interface);

   /* raise an event saying a new global has been added */
   ecore_event_add(ECORE_WL2_EVENT_GLOBAL_ADDED, ev,
                   _cb_global_event_free, NULL);
}

/**
 * @internal
 * @brief Callback for wl_registry.global_remove event (global object removed).
 *
 * This function is called by the Wayland library when the compositor
 * announces that a global object has been removed. It removes the
 * global's information from the display's `globals` hash and emits an
 * ECORE_WL2_EVENT_GLOBAL_REMOVED event.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param registry The wl_registry object (unused).
 * @param id The unique ID of the global object that was removed.
 */
static void
_cb_global_remove(void *data, struct wl_registry *registry EINA_UNUSED, unsigned int id)
{
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Global *global;
   Ecore_Wl2_Event_Global *ev;

   ewd = data;

   /* try to find this global in our hash */
   global = eina_hash_find(ewd->globals, &id);
   if (!global) return;

   /* allocate space for event structure */
   ev = calloc(1, sizeof(Ecore_Wl2_Event_Global));
   if (!ev) return;

   ev->id = id;
   ev->display = ewd;
   ewd->refs++;
   ev->version = global->version;
   ev->interface = eina_stringshare_add(global->interface);

   /* raise an event saying a global has been removed */
   ecore_event_add(ECORE_WL2_EVENT_GLOBAL_REMOVED, ev,
                   _cb_global_event_free, NULL);

   /* delete this global from our hash */
   if (ewd->globals) eina_hash_del_by_key(ewd->globals, &id);
}

static const struct wl_registry_listener _registry_listener =
{
   _cb_global_add,
   _cb_global_remove
};

/**
 * @internal
 * @brief Ecore_Fd_Handler callback for server-side display event loop.
 *
 * This function is called when there is data to be read on the Wayland
 * display's file descriptor for a server-side display (created with
 * ecore_wl2_display_create()). It dispatches events from the Wayland
 * event loop.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param hdl The Ecore_Fd_Handler (unused).
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_cb_create_data(void *data, Ecore_Fd_Handler *hdl EINA_UNUSED)
{
   Ecore_Wl2_Display *ewd = data;
   struct wl_event_loop *loop;

   loop = wl_display_get_event_loop(ewd->wl.display);
   wl_event_loop_dispatch(loop, 0);

   /* wl_display_flush_clients(ewd->wl.display); */

   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Ecore_Fd_Handler prepare callback for server-side display.
 *
 * This function is called before polling the file descriptors. For a
 * server-side display, it flushes any pending client requests.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param hdlr The Ecore_Fd_Handler (unused).
 */
static void
_cb_create_prepare(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Wl2_Display *ewd = data;

   wl_display_flush_clients(ewd->wl.display);
}

/**
 * @internal
 * @brief Timer callback for attempting session recovery.
 * @since 1.20
 *
 * This function is called by an Ecore_Timer when a display connection
 * was lost and session recovery is being attempted. It tries to reconnect
 * to the Wayland display.
 *
 * @param ewd The Ecore_Wl2_Display attempting to recover.
 * @return EINA_TRUE to reschedule the timer if connection fails,
 *         EINA_FALSE if connection succeeds or recovery is abandoned.
 */
static Eina_Bool
_recovery_timer(Ecore_Wl2_Display *ewd)
{
   if (!_ecore_wl2_display_connect(ewd, 1))
     return EINA_TRUE;

   ewd->recovery_timer = NULL;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Cleans up global Wayland objects associated with a display.
 *
 * Destroys all global Wayland proxy objects (compositor, shm, shell, etc.)
 * that were bound for the given Ecore_Wl2_Display. This is typically
 * called during display disconnection or session recovery.
 *
 * @param ewd The Ecore_Wl2_Display whose globals are to be cleaned up.
 */
static void
_ecore_wl2_display_globals_cleanup(Ecore_Wl2_Display *ewd)
{
   if (ewd->wl.session_recovery)
     zwp_e_session_recovery_destroy(ewd->wl.session_recovery);
   if (ewd->wl.xdg_wm_base) xdg_wm_base_destroy(ewd->wl.xdg_wm_base);
   if (ewd->wl.zxdg_shell) zxdg_shell_v6_destroy(ewd->wl.zxdg_shell);
   if (ewd->wl.shm) wl_shm_destroy(ewd->wl.shm);
   if (ewd->wl.data_device_manager)
     wl_data_device_manager_destroy(ewd->wl.data_device_manager);
   if (ewd->wl.compositor) wl_compositor_destroy(ewd->wl.compositor);
   if (ewd->wl.subcompositor) wl_subcompositor_destroy(ewd->wl.subcompositor);
   if (ewd->wl.dmabuf) zwp_linux_dmabuf_v1_destroy(ewd->wl.dmabuf);
   if (ewd->wl.efl_aux_hints) efl_aux_hints_destroy(ewd->wl.efl_aux_hints);
   if (ewd->wl.efl_hints) efl_hints_destroy(ewd->wl.efl_hints);

   if (ewd->wl.registry) wl_registry_destroy(ewd->wl.registry);
}

/**
 * @internal
 * @brief Initiates the session recovery process for a display.
 * @since 1.20
 *
 * This function is called when a connection error occurs and session
 * recovery is possible. It cleans up existing Wayland resources,
 * resets display state, and schedules a timer to attempt reconnection.
 * An ECORE_WL2_EVENT_DISCONNECT event is emitted.
 *
 * @param ewd The Ecore_Wl2_Display for which to start recovery.
 */
static void
_recovery_timer_add(Ecore_Wl2_Display *ewd)
{
   Eina_Inlist *tmp, *tmp2;
   Ecore_Wl2_Output *output;
   Ecore_Wl2_Input *input;
   Ecore_Wl2_Window *window;

   eina_hash_free_buckets(ewd->globals);

   ecore_main_fd_handler_del(ewd->fd_hdl);
   ewd->fd_hdl = NULL;

   ewd->shell_done = EINA_FALSE;
   ewd->sync_done = EINA_FALSE;
   ewd->recovering = EINA_TRUE;

   _ecore_wl2_display_globals_cleanup(ewd);

   memset(&ewd->wl, 0, sizeof(ewd->wl));
   EINA_INLIST_FOREACH_SAFE(ewd->inputs, tmp, input)
     _ecore_wl2_input_del(input);

   EINA_INLIST_FOREACH_SAFE(ewd->outputs, tmp, output)
     _ecore_wl2_output_del(output);

   EINA_INLIST_FOREACH_SAFE(ewd->windows, tmp, window)
     {
        Ecore_Wl2_Subsurface *subsurf;

        EINA_INLIST_FOREACH_SAFE(window->subsurfs, tmp2, subsurf)
          _ecore_wl2_subsurf_unmap(subsurf);
        _ecore_wl2_window_semi_free(window);
        window->set_config.serial = 0;
        window->req_config.serial = 0;
        window->xdg_configure_ack = NULL;
        window->xdg_set_min_size = NULL;
        window->xdg_set_max_size = NULL;
        window->zxdg_configure_ack = NULL;
        window->zxdg_set_min_size = NULL;
        window->zxdg_set_max_size = NULL;
     }

   ewd->recovery_timer =
     ecore_timer_add(0.5, (Ecore_Task_Cb)_recovery_timer, ewd);
   _ecore_wl2_display_event(ewd, ECORE_WL2_EVENT_DISCONNECT);
}

/**
 * @internal
 * @brief Decides whether to initiate session recovery or signal exit.
 * @since 1.20
 *
 * Called when a Wayland display error occurs. If session recovery is
 * enabled and applicable for the error, `_recovery_timer_add` is called.
 * Otherwise, if it's a client-side display error that's not recoverable,
 * `_ecore_wl2_display_signal_exit` is called to terminate the application.
 *
 * @param ewd The Ecore_Wl2_Display experiencing the error.
 * @param code The errno code from the display error.
 */
static void
_begin_recovery_maybe(Ecore_Wl2_Display *ewd, int code)
{
   if ((_server_displays || (code != EPROTO)) && ewd->wl.session_recovery)// && (errno == EPIPE))
     _recovery_timer_add(ewd);
   else if (!_server_displays)
     {
        ERR("Wayland Socket Error: %s", eina_error_msg_get(errno));
        _ecore_wl2_display_signal_exit();
     }
}

/**
 * @internal
 * @brief Ecore_Fd_Handler prepare callback for client-side display connection.
 *
 * This function is called before polling the file descriptors for a
 * client-side display. It attempts to dispatch any pending Wayland events
 * and checks for display errors. If a non-recoverable error occurs,
 * it may trigger session recovery or application exit.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param hdl The Ecore_Fd_Handler (unused).
 */
static void
_cb_connect_pre(void *data, Ecore_Fd_Handler *hdl EINA_UNUSED)
{
   Ecore_Wl2_Display *ewd = data;
   int ret = 0, code;

   while ((wl_display_prepare_read(ewd->wl.display) != 0) && (ret >= 0))
     ret = wl_display_dispatch_pending(ewd->wl.display);

   if (ret < 0) goto err;

   ret = wl_display_get_error(ewd->wl.display);
   if (ret < 0) goto err;

   return;

err:
   code = errno;
   if ((ret < 0) && (code != EAGAIN))
     {
        _begin_recovery_maybe(ewd, code);
     }
}

/**
 * @internal
 * @brief Ecore_Fd_Handler callback for client-side display connection events.
 *
 * This function is called when there is data to be read or written on the
 * Wayland display's file descriptor for a client-side display (connected
 * with ecore_wl2_display_connect()). It reads incoming Wayland events,
 * dispatches pending events, and flushes outgoing requests.
 * If a display error occurs, it may trigger session recovery or
 * application exit.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param hdl The Ecore_Fd_Handler.
 * @return ECORE_CALLBACK_RENEW to keep the handler active, or
 *         ECORE_CALLBACK_CANCEL if a fatal error occurs and the handler
 *         is removed.
 */
static Eina_Bool
_cb_connect_data(void *data, Ecore_Fd_Handler *hdl)
{
   Ecore_Wl2_Display *ewd = data;
   int ret = 0, code;

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_READ))
     {
        ret = wl_display_read_events(ewd->wl.display);
        code = errno;
        if ((ret < 0) && (code != EAGAIN)) goto err;
     }
   else
     wl_display_cancel_read(ewd->wl.display);

   wl_display_dispatch_pending(ewd->wl.display);
   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_WRITE))
     {
        ret = wl_display_flush(ewd->wl.display);
        code = errno;
        if (ret >= 0)
          ecore_main_fd_handler_active_set(hdl, ECORE_FD_READ | ECORE_FD_ALWAYS);

        if ((ret < 0) && (code != EAGAIN)) goto err;
     }

   return ECORE_CALLBACK_RENEW;

err:
   ewd->fd_hdl = NULL;
   _begin_recovery_maybe(ewd, code);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Frees an Ecore_Wl2_Global structure.
 *
 * This function is used as a callback when an Ecore_Wl2_Global entry
 * is removed from the `ewd->globals` hash table. It frees the
 * stringshared interface name and the structure itself.
 *
 * @param data Pointer to the Ecore_Wl2_Global structure to free.
 */
static void
_cb_globals_hash_del(void *data)
{
   Ecore_Wl2_Global *global;

   global = data;

   eina_stringshare_del(global->interface);

   free(global);
}

/**
 * @internal
 * @brief Finds a global object by interface name.
 *
 * Iterates through the display's known global objects to find one
 * that matches the given interface name.
 *
 * @param ewd The Ecore_Wl2_Display to search within.
 * @param interface The interface name to search for (e.g., "xdg_wm_base").
 * @return A pointer to the Ecore_Wl2_Global if found, otherwise NULL.
 */
static Ecore_Wl2_Global *
_ecore_wl2_global_find(Ecore_Wl2_Display *ewd, const char *interface)
{
   Eina_Iterator *itr;
   Ecore_Wl2_Global *global = NULL, *g = NULL;

   itr = eina_hash_iterator_data_new(ewd->globals);
   if (!itr) return NULL;

   EINA_ITERATOR_FOREACH(itr, g)
     {
        if (!strcmp(g->interface, interface))
          {
             global = g;
             break;
          }
     }

   eina_iterator_free(itr);
   return global;
}

/**
 * @internal
 * @brief Binds to a Wayland shell interface (xdg_wm_base or zxdg_shell_v6).
 *
 * This function attempts to find and bind to a supported shell interface.
 * It prioritizes "xdg_wm_base" and falls back to "zxdg_shell_v6".
 * Once a shell is successfully bound, `ewd->shell_done` is set to EINA_TRUE.
 * This is typically called after the initial display sync is complete.
 *
 * @param ewd The Ecore_Wl2_Display for which to bind a shell.
 */
static void
_ecore_wl2_shell_bind(Ecore_Wl2_Display *ewd)
{
   Ecore_Wl2_Global *global = NULL;
   const char **itr;
   const char *shells[] =
     {
        "xdg_wm_base",
        "zxdg_shell_v6",
        NULL
     };

   if (ewd->shell_done) return;

   for (itr = shells; *itr != NULL; itr++)
     {
        global = _ecore_wl2_global_find(ewd, *itr);
        if (!global) continue;
        break;
     }

   if (!global) return;

   if (!strcmp(global->interface, "xdg_wm_base"))
     {
        ewd->wl.xdg_wm_base =
          wl_registry_bind(ewd->wl.registry, global->id,
                           &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(ewd->wl.xdg_wm_base,
                                   &_xdg_shell_listener, ewd);
        ewd->shell_done = EINA_TRUE;
     }
   else if (!strcmp(global->interface, "zxdg_shell_v6"))
     {
        ewd->wl.zxdg_shell =
          wl_registry_bind(ewd->wl.registry, global->id,
                           &zxdg_shell_v6_interface, 1);
        zxdg_shell_v6_add_listener(ewd->wl.zxdg_shell,
                                   &_zxdg_shell_listener, ewd);
        ewd->shell_done = EINA_TRUE;
     }
}

/**
 * @internal
 * @brief Callback for wl_display.sync completion.
 *
 * This function is invoked when a `wl_display_sync` operation completes.
 * It decrements the `ewd->syncs` counter. If this is the last pending sync
 * and `ewd->sync_done` is not yet true, it sets `ewd->sync_done` to EINA_TRUE,
 * attempts to bind the shell interface, destroys the wl_callback, flushes
 * the display, and emits an ECORE_WL2_EVENT_SYNC_DONE event.
 *
 * @param data User data (Ecore_Wl2_Display).
 * @param cb The wl_callback object for the completed sync.
 * @param serial The serial associated with the sync callback (unused).
 */
static void
_cb_sync_done(void *data, struct wl_callback *cb, uint32_t serial EINA_UNUSED)
{
   Ecore_Wl2_Event_Sync_Done *ev;
   Ecore_Wl2_Display *ewd;

   ewd = data;
   if (--ewd->syncs) return;
   if (ewd->sync_done) return;

   ewd->sync_done = EINA_TRUE;

   _ecore_wl2_shell_bind(ewd);

   wl_callback_destroy(cb);
   ecore_wl2_display_flush(ewd);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Sync_Done));
   if (!ev) return;

   ev->display = ewd;
   ewd->refs++;
   ecore_event_add(ECORE_WL2_EVENT_SYNC_DONE, ev, _display_event_free, ewd);
}

static const struct wl_callback_listener _sync_listener =
{
   _cb_sync_done
};

/**
 * @internal
 * @brief Initiates a wl_display_sync operation.
 *
 * Increments the `ewd->syncs` counter and requests a sync callback from
 * the Wayland display. This is used to ensure all pending requests have
 * been processed by the compositor.
 *
 * @param ewd The Ecore_Wl2_Display to sync.
 */
static void
_ecore_wl2_display_sync_add(Ecore_Wl2_Display *ewd)
{
   struct wl_callback *cb;

   ewd->syncs++;
   cb = wl_display_sync(ewd->wl.display);
   wl_callback_add_listener(cb, &_sync_listener, ewd);
}

/**
 * @internal
 * @brief Connects to a Wayland display and sets up initial resources.
 *
 * This function attempts to connect to the Wayland display specified by
 * `ewd->name`. If successful, it retrieves the wl_registry, adds a listener
 * for global events, and initiates a display sync.
 * If `sync` is EINA_TRUE, it will block until the initial sync is complete.
 * It then sets up an Ecore_Fd_Handler to manage events on the display's
 * file descriptor and emits an ECORE_WL2_EVENT_CONNECT event.
 *
 * @param ewd The Ecore_Wl2_Display to connect.
 * @param sync If EINA_TRUE, block until the initial display sync completes.
 * @return EINA_TRUE on successful connection, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_wl2_display_connect(Ecore_Wl2_Display *ewd, Eina_Bool sync)
{
   /* try to connect to wayland display with this name */
   ewd->wl.display = wl_display_connect(ewd->name);
   if (!ewd->wl.display) return EINA_FALSE;

   ewd->recovering = EINA_FALSE;

   ewd->wl.registry = wl_display_get_registry(ewd->wl.display);
   wl_registry_add_listener(ewd->wl.registry, &_registry_listener, ewd);

   _ecore_wl2_display_sync_add(ewd);

   if (sync)
     {
        /* NB: If we are connecting (as a client), then we will need to setup
         * a callback for display_sync and wait for it to complete. There is no
         * other option here as we need the compositor, shell, etc, to be setup
         * before we can allow a user to make use of the API functions */
        while (!ewd->sync_done)
          {
             int ret;

             ret = wl_display_dispatch(ewd->wl.display);
             if ((ret < 0) && (errno != EAGAIN))
               {
                  ERR("Received Fatal Error on Wayland Display");

                  wl_registry_destroy(ewd->wl.registry);
                  return EINA_FALSE;
               }
          }
     }

   ewd->fd_hdl =
     ecore_main_fd_handler_add(wl_display_get_fd(ewd->wl.display),
                               ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ERROR | ECORE_FD_ALWAYS,
                               _cb_connect_data, ewd, NULL, ewd);

   ecore_main_fd_handler_prepare_callback_set
     (ewd->fd_hdl, _cb_connect_pre, ewd);

   _ecore_wl2_display_event(ewd, ECORE_WL2_EVENT_CONNECT);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Cleans up resources associated with an Ecore_Wl2_Display.
 *
 * This function is called when a display is being disconnected or destroyed.
 * It unrefs the XKB context, frees all inputs and outputs, removes the
 * Ecore_Fd_Handler, frees the globals hash, and cleans up global Wayland
 * proxy objects.
 *
 * @param ewd The Ecore_Wl2_Display to clean up.
 */
static void
_ecore_wl2_display_cleanup(Ecore_Wl2_Display *ewd)
{
   Ecore_Wl2_Output *output;
   Ecore_Wl2_Input *input;
   Eina_Inlist *tmp;

   if (ewd->xkb_context) xkb_context_unref(ewd->xkb_context);

   /* free each input */
   EINA_INLIST_FOREACH_SAFE(ewd->inputs, tmp, input)
     _ecore_wl2_input_del(input);

   /* free each output */
   EINA_INLIST_FOREACH_SAFE(ewd->outputs, tmp, output)
     _ecore_wl2_output_del(output);

   if (ewd->fd_hdl) ecore_main_fd_handler_del(ewd->fd_hdl);

   eina_hash_free(ewd->globals);

   _ecore_wl2_display_globals_cleanup(ewd);
}

/**
 * @internal
 * @brief Finds an Ecore_Wl2_Window associated with a given wl_surface.
 *
 * Iterates through the windows managed by the display to find the one
 * whose `surface` member matches the provided `wl_surface`.
 *
 * @param display The Ecore_Wl2_Display to search within.
 * @param wl_surface The wl_surface to find the Ecore_Wl2_Window for.
 * @return A pointer to the Ecore_Wl2_Window if found, otherwise NULL.
 */
Ecore_Wl2_Window *
_ecore_wl2_display_window_surface_find(Ecore_Wl2_Display *display, struct wl_surface *wl_surface)
{
   Ecore_Wl2_Window *window;

   if ((!display) || (!wl_surface)) return NULL;

   EINA_INLIST_FOREACH(display->windows, window)
     {
        if ((window->surface) &&
            (window->surface == wl_surface))
          return window;
     }

   return NULL;
}

/**
 * @brief Creates a new Wayland display (server-side).
 *
 * This function creates a new Wayland display that other clients can connect to.
 * If @p name is NULL, a default socket name will be automatically chosen
 * (e.g., "wayland-0", "wayland-1", etc.). If @p name is provided, that
 * specific socket name will be used.
 *
 * The created display is added to a global hash of server displays.
 * Subsequent calls with the same name will return the existing display
 * and increment its reference count.
 *
 * An Ecore_Fd_Handler is set up to manage events on the display's
 * event loop. The WAYLAND_DISPLAY environment variable is set to the
 * name of the created display socket.
 *
 * @param name The desired socket name for the display, or NULL for auto.
 * @return A pointer to the newly created Ecore_Wl2_Display on success,
 *         NULL on failure.
 *
 * @see ecore_wl2_display_destroy()
 * @see ecore_wl2_display_connect()
 */
EAPI Ecore_Wl2_Display *
ecore_wl2_display_create(const char *name)
{
   Ecore_Wl2_Display *ewd;
   struct wl_event_loop *loop;

   if (!_server_displays)
     _server_displays = eina_hash_string_superfast_new(NULL);

   if (name)
     {
        /* someone wants to create a server with a specific display */

        /* check hash of cached server displays for this name */
        ewd = eina_hash_find(_server_displays, name);
        if (ewd) goto found;
     }

   /* allocate space for display structure */
   ewd = calloc(1, sizeof(Ecore_Wl2_Display));
   if (!ewd) return NULL;

   ewd->refs++;
   ewd->pid = getpid();

   /* try to create new wayland display */
   ewd->wl.display = wl_display_create();
   if (!ewd->wl.display)
     {
        ERR("Could not create wayland display");
        goto create_err;
     }

   if (!name)
     {
        const char *n;

        n = wl_display_add_socket_auto(ewd->wl.display);
        if (!n)
          {
             ERR("Failed to add display socket");
             goto socket_err;
          }

        ewd->name = strdup(n);
     }
   else
     {
        if (wl_display_add_socket(ewd->wl.display, name))
          {
             ERR("Failed to add display socket");
             goto socket_err;
          }

        ewd->name = strdup(name);
     }

   setenv("WAYLAND_DISPLAY", ewd->name, 1);
   DBG("WAYLAND_DISPLAY: %s", ewd->name);

   loop = wl_display_get_event_loop(ewd->wl.display);

   ewd->fd_hdl =
     ecore_main_fd_handler_add(wl_event_loop_get_fd(loop),
                               ECORE_FD_READ | ECORE_FD_ERROR,
                               _cb_create_data, ewd, NULL, NULL);

   ecore_main_fd_handler_prepare_callback_set(ewd->fd_hdl,
                                              _cb_create_prepare, ewd);

   /* add this new server display to hash */
   eina_hash_add(_server_displays, ewd->name, ewd);

   return ewd;

socket_err:
   wl_display_destroy(ewd->wl.display);

create_err:
   free(ewd);
   return NULL;

found:
   ewd->refs++;
   return ewd;
}

/**
 * @internal
 * @brief Determines if a synchronous connection is needed.
 *
 * This function checks if there are any active server-side displays.
 * If there are no server displays (meaning we are likely a standalone client),
 * or if the server display hash hasn't been initialized, it returns EINA_TRUE,
 * indicating that a synchronous connection (waiting for initial globals)
 * is appropriate. Otherwise, it returns EINA_FALSE.
 *
 * @return EINA_TRUE if a synchronous connection is recommended, EINA_FALSE otherwise.
 */
Eina_Bool
_ecore_wl2_display_sync_get(void)
{
   return !_server_displays || !eina_hash_population(_server_displays);
}

/**
 * @brief Connects to an existing Wayland display (client-side).
 *
 * This function establishes a connection to a Wayland compositor.
 * If @p name is NULL, it attempts to connect to the display specified by
 * the WAYLAND_DISPLAY environment variable. If WAYLAND_DISPLAY is not set,
 * it defaults to "wayland-0".
 * If @p name is provided, it attempts to connect to that specific display.
 *
 * The connected display is added to a global hash of client displays.
 * Subsequent calls to connect to the same display name will return the
 * existing Ecore_Wl2_Display object and increment its reference count.
 *
 * An XKB context is initialized for keyboard handling.
 * The connection process involves retrieving the Wayland registry, listening
 * for global objects, and performing an initial synchronization with the
 * compositor.
 *
 * @param name The name of the Wayland display to connect to (e.g., "wayland-0"),
 *             or NULL to use the default.
 * @return A pointer to the Ecore_Wl2_Display on successful connection,
 *         NULL on failure.
 *
 * @see ecore_wl2_display_disconnect()
 * @see ecore_wl2_display_create()
 */
EAPI Ecore_Wl2_Display *
ecore_wl2_display_connect(const char *name)
{
   Ecore_Wl2_Display *ewd;
   const char *n;
   Eina_Bool hash_create = !_client_displays;

   if (!_client_displays)
     _client_displays = eina_hash_string_superfast_new(NULL);

   if (!name)
     {
        /* client wants to connect to default display */
        n = getenv("WAYLAND_DISPLAY");
        if (!n) n = "wayland-0";

        /* we have a default wayland display */

        /* check hash of cached client displays for this name */
        ewd = eina_hash_find(_client_displays, n);
        if (ewd) goto found;
     }
   else
     {
        /* client wants to connect to specific display */

        /* check hash of cached client displays for this name */
        ewd = eina_hash_find(_client_displays, name);
        if (ewd) goto found;
     }

   /* allocate space for display structure */
   ewd = calloc(1, sizeof(Ecore_Wl2_Display));
   if (!ewd) return NULL;

   ewd->refs++;

   if (name)
     ewd->name = strdup(name);
   else if (n)
     ewd->name = strdup(n);

   ewd->globals = eina_hash_int32_new(_cb_globals_hash_del);

   ewd->xkb_context = xkb_context_new(0);
   if (!ewd->xkb_context) goto context_err;

   /* check server display hash and match on pid. If match, skip sync */
   if (!_ecore_wl2_display_connect(ewd, _ecore_wl2_display_sync_get()))
     goto connect_err;

   /* add this new client display to hash */
   eina_hash_add(_client_displays, ewd->name, ewd);

   return ewd;

connect_err:
   xkb_context_unref(ewd->xkb_context);
   ewd->xkb_context = NULL;

context_err:
   eina_hash_free(ewd->globals);
   free(ewd->name);
   free(ewd);

   if (hash_create)
     {
        eina_hash_free(_client_displays);
        _client_displays = NULL;
     }
   return NULL;

found:
   ewd->refs++;
   return ewd;
}

/**
 * @brief Disconnects from a Wayland display (client-side).
 *
 * Decrements the reference count of the Ecore_Wl2_Display. If the reference
 * count reaches zero, it cleans up all associated resources, disconnects
 * from the Wayland display, removes it from the client display cache,
 * and frees the Ecore_Wl2_Display structure.
 *
 * Before disconnecting, it dispatches any pending Wayland events.
 *
 * @param display The Ecore_Wl2_Display to disconnect.
 *
 * @see ecore_wl2_display_connect()
 */
EAPI void
ecore_wl2_display_disconnect(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);
   int ret;

   do
     {
        ret = wl_display_dispatch_pending(display->wl.display);
     } while (ret > 0);

   --display->refs;
   if (display->refs == 0)
     {
        _ecore_wl2_display_cleanup(display);

        wl_display_disconnect(display->wl.display);

        /* remove this client display from hash */
        eina_hash_del_by_key(_client_displays, display->name);

        free(display->name);
        free(display);
     }
}

/**
 * @brief Destroys a Wayland display (server-side).
 *
 * Decrements the reference count of the Ecore_Wl2_Display. If the reference
 * count reaches zero, it cleans up all associated resources (including
 * destroying the underlying wl_display), removes it from the server
 * display cache, cancels any recovery timer, and frees the
 * Ecore_Wl2_Display structure.
 *
 * @param display The Ecore_Wl2_Display to destroy (must have been created
 *                with ecore_wl2_display_create()).
 *
 * @see ecore_wl2_display_create()
 */
EAPI void
ecore_wl2_display_destroy(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);

   --display->refs;
   if (display->refs == 0)
     {
        /* this ensures that things like wl_registry are destroyed
         * before we destroy the actual wl_display */
        _ecore_wl2_display_cleanup(display);

        wl_display_destroy(display->wl.display);

        /* remove this client display from hash */
        eina_hash_del_by_key(_server_displays, display->name);
        ecore_timer_del(display->recovery_timer);

        free(display->name);
        free(display);
     }
}

/**
 * @brief Terminates a Wayland display server.
 *
 * This function calls `wl_display_terminate()` on the underlying
 * Wayland display. This will cause the display server to stop accepting
 * new connections and to eventually shut down once all clients have
 * disconnected.
 *
 * @param display The Ecore_Wl2_Display (server-side) to terminate.
 * @since 1.8
 */
EAPI void
ecore_wl2_display_terminate(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);
   wl_display_terminate(display->wl.display);
}

/**
 * @brief Retrieves the native `struct wl_display` from an Ecore_Wl2_Display.
 *
 * This allows direct interaction with the Wayland library's display object
 * if needed, though most operations should be possible through Ecore_Wl2 APIs.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The underlying `struct wl_display`, or NULL if @p display is NULL.
 */
EAPI struct wl_display *
ecore_wl2_display_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.display;
}

/**
 * @brief Retrieves the `wl_shm` global object for the display.
 *
 * The `wl_shm` interface is used for creating shared memory buffers
 * that can be used by clients to draw pixel data and share it with
 * the compositor.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The `struct wl_shm` proxy, or NULL if not available or @p display is NULL.
 */
EAPI struct wl_shm *
ecore_wl2_display_shm_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.shm;
}

/**
 * @brief Retrieves the `zwp_linux_dmabuf_v1` global object for the display.
 *
 * The `zwp_linux_dmabuf_v1` interface is used for creating buffers from
 * DMA-BUF file descriptors, allowing for zero-copy buffer sharing between
 * clients (like Mesa, GStreamer) and the compositor.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The `struct zwp_linux_dmabuf_v1` proxy (cast to void*),
 *         or NULL if not available or @p display is NULL.
 * @since 1.18
 */
EAPI void *
ecore_wl2_display_dmabuf_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.dmabuf;
}

/**
 * @brief Retrieves an iterator for the global Wayland objects of a display.
 *
 * This allows iterating over all global objects (Ecore_Wl2_Global)
 * that the compositor has announced and Ecore_Wl2 is aware of.
 * Each item in the iterator is an Ecore_Wl2_Global structure.
 *
 * Example:
 * @code
 * Eina_Iterator *it;
 * Ecore_Wl2_Global *global;
 * it = ecore_wl2_display_globals_get(display);
 * EINA_ITERATOR_FOREACH(it, global)
 *   printf("Global: %s, ID: %u, Version: %u\n",
 *          global->interface, global->id, global->version);
 * eina_iterator_free(it);
 * @endcode
 *
 * @param display The Ecore_Wl2_Display.
 * @return An Eina_Iterator over Ecore_Wl2_Global structures,
 *         or NULL if @p display or its globals hash is NULL.
 *         The iterator must be freed using eina_iterator_free().
 */
EAPI Eina_Iterator *
ecore_wl2_display_globals_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(display->globals, NULL);

   return eina_hash_iterator_data_new(display->globals);
}

/**
 * @brief Gets the total screen size aggregated from all outputs.
 *
 * This function calculates the total dimensions of the screen space by summing
 * up the widths and heights of all connected outputs, considering their
 * transformations (rotations).
 *
 * Note: This provides a logical sum of dimensions and might not represent
 * a single contiguous rectangular area if outputs are arranged in a complex layout.
 * For individual output details, iterate through ecore_wl2_display_outputs_get().
 *
 * @param display The Ecore_Wl2_Display.
 * @param[out] w Pointer to store the total width. Can be NULL.
 * @param[out] h Pointer to store the total height. Can be NULL.
 */
EAPI void
ecore_wl2_display_screen_size_get(Ecore_Wl2_Display *display, int *w, int *h)
{
   Ecore_Wl2_Output *output;
   int ow = 0, oh = 0;

   EINA_SAFETY_ON_NULL_RETURN(display);

   if (w) *w = 0;
   if (h) *h = 0;

   EINA_INLIST_FOREACH(display->outputs, output)
     {
        switch (output->transform)
          {
           case WL_OUTPUT_TRANSFORM_90:
           case WL_OUTPUT_TRANSFORM_270:
           case WL_OUTPUT_TRANSFORM_FLIPPED_90:
           case WL_OUTPUT_TRANSFORM_FLIPPED_270:
             ow += output->geometry.h;
             oh += output->geometry.w;
             break;
           default:
             ow += output->geometry.w;
             oh += output->geometry.h;
             break;
          }
     }

   if (w) *w = ow;
   if (h) *h = oh;
}

/**
 * @brief Retrieves the `wl_registry` for the display.
 *
 * The `wl_registry` is the Wayland object used to bind to global interfaces
 * announced by the compositor.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The `struct wl_registry` proxy, or NULL if @p display is NULL.
 */
EAPI struct wl_registry *
ecore_wl2_display_registry_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);

   return display->wl.registry;
}

/**
 * @brief Gets the version of the `wl_compositor` interface supported by the display.
 *
 * The `wl_compositor` is a fundamental Wayland interface used for creating
 * surfaces (`wl_surface`). The version indicates which features of the
 * compositor interface are available. Ecore_Wl2 will bind up to version 4.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The version of the `wl_compositor` interface, or 0 if @p display is NULL.
 */
EAPI int
ecore_wl2_display_compositor_version_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, 0);

   return display->wl.compositor_version;
}

/**
 * @brief Retrieves an iterator for the input devices (seats) of a display.
 *
 * This function is only relevant for client-side displays. Server-side
 * displays (created with ecore_wl2_display_create()) do not manage inputs
 * in this way and will return NULL.
 * Each item in the iterator is an Ecore_Wl2_Input structure.
 *
 * Example:
 * @code
 * Eina_Iterator *it;
 * Ecore_Wl2_Input *input;
 * it = ecore_wl2_display_inputs_get(display);
 * EINA_ITERATOR_FOREACH(it, input)
 *   printf("Input ID: %u, Name: %s\n", input->id, ecore_wl2_input_name_get(input));
 * eina_iterator_free(it);
 * @endcode
 *
 * @param display The Ecore_Wl2_Display.
 * @return An Eina_Iterator over Ecore_Wl2_Input structures, or NULL if
 *         @p display is NULL or it's a server-side display.
 *         The iterator must be freed using eina_iterator_free().
 */
EAPI Eina_Iterator *
ecore_wl2_display_inputs_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(display->pid, NULL);
   return eina_inlist_iterator_new(display->inputs);
}

/**
 * @brief Finds an input device (seat) by its ID.
 *
 * This function is only relevant for client-side displays.
 *
 * @param display The Ecore_Wl2_Display.
 * @param id The ID of the input device to find.
 * @return The Ecore_Wl2_Input if found, otherwise NULL.
 *         Returns NULL if @p display is NULL or it's a server-side display.
 */
EAPI Ecore_Wl2_Input *
ecore_wl2_display_input_find(const Ecore_Wl2_Display *display, unsigned int id)
{
   Ecore_Wl2_Input *input;

   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(display->pid, NULL);
   EINA_INLIST_FOREACH(display->inputs, input)
     if (input->id == id) return input;
   return NULL;
}

/**
 * @brief Finds an input device (seat) by its name.
 *
 * This function is only relevant for client-side displays.
 * The name of an input device is typically provided by the compositor
 * via the `wl_seat.name` event (if supported and sent).
 *
 * @param display The Ecore_Wl2_Display.
 * @param name The name of the input device to find.
 * @return The Ecore_Wl2_Input if found, otherwise NULL.
 *         Returns NULL if @p display is NULL, @p name is NULL, or it's a server-side display.
 * @since 1.20
 */
EAPI Ecore_Wl2_Input *
ecore_wl2_display_input_find_by_name(const Ecore_Wl2_Display *display, const char *name)
{
   Ecore_Wl2_Input *input;

   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(display->pid, NULL);
   EINA_INLIST_FOREACH(display->inputs, input)
     if (eina_streq(input->name, name)) return input;
   return NULL;
}

/**
 * @brief Checks if the initial display synchronization is complete.
 *
 * After connecting to a Wayland display, Ecore_Wl2 performs an initial
 * synchronization to receive all initial global objects and other setup
 * information from the compositor. This function returns whether that
 * initial sync process has finished.
 *
 * @param display The Ecore_Wl2_Display.
 * @return EINA_TRUE if the initial sync is done, EINA_FALSE otherwise
 *         (or if @p display is NULL).
 */
EAPI Eina_Bool
ecore_wl2_display_sync_is_done(const Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, EINA_FALSE);
   return display->sync_done;
}

/**
 * @brief Gets the name of the Wayland display.
 *
 * For client connections, this is the name used to connect (e.g., "wayland-0"
 * or the value of $WAYLAND_DISPLAY). For server-side displays, this is the
 * socket name the server is listening on.
 *
 * @param display The Ecore_Wl2_Display.
 * @return The name of the display, or NULL if @p display is NULL.
 *         The returned string is an internal string and should not be modified or freed.
 */
EAPI const char *
ecore_wl2_display_name_get(const Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->name;
}

/**
 * @brief Flushes pending Wayland requests to the compositor.
 *
 * Most Wayland requests are buffered client-side and sent to the compositor
 * at appropriate times (e.g., before blocking for events). This function
 * explicitly flushes any buffered requests.
 *
 * If flushing results in an EAGAIN error, it means the write would block,
 * so the fd_handler is set to monitor for write readiness.
 * If another error occurs, it may trigger session recovery.
 *
 * @param display The Ecore_Wl2_Display.
 */
EAPI void
ecore_wl2_display_flush(Ecore_Wl2_Display *display)
{
   int ret, code;

   EINA_SAFETY_ON_NULL_RETURN(display);

   ret = wl_display_flush(display->wl.display);
   if (ret >= 0) return;

   code = errno;
   if (code == EAGAIN)
     {
        ecore_main_fd_handler_active_set(display->fd_hdl,
                                         (ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ALWAYS));
        return;
     }

   _begin_recovery_maybe(display, code);
}

/**
 * @brief Finds an Ecore_Wl2_Window associated with a given wl_surface.
 *
 * This is a public wrapper around the internal
 * `_ecore_wl2_display_window_surface_find` function.
 *
 * @param display The Ecore_Wl2_Display to search within.
 * @param surface The wl_surface to find the Ecore_Wl2_Window for.
 * @return A pointer to the Ecore_Wl2_Window if found, otherwise NULL.
 * @since 1.8
 */
EAPI Ecore_Wl2_Window *
ecore_wl2_display_window_find_by_surface(Ecore_Wl2_Display *display, struct wl_surface *surface)
{
   return _ecore_wl2_display_window_surface_find(display, surface);
}

/**
 * @brief Retrieves a pointer to an already connected Ecore_Wl2_Display.
 *
 * This function checks the cache of connected client displays. If a display
 * matching @p name (or the default if @p name is NULL) is found in the cache,
 * a pointer to it is returned. This function does not increment the display's
 * reference count.
 *
 * This is useful for obtaining a handle to an Ecore_Wl2_Display that was
 * connected elsewhere in the application, without establishing a new connection.
 *
 * @param name The name of the Wayland display (e.g., "wayland-0"), or NULL
 *             to use the default (checks $WAYLAND_DISPLAY, then "wayland-0").
 * @return A pointer to the cached Ecore_Wl2_Display if connected,
 *         otherwise NULL.
 * @since 1.8
 */
EAPI Ecore_Wl2_Display *
ecore_wl2_connected_display_get(const char *name)
{
   Ecore_Wl2_Display *ewd;

   EINA_SAFETY_ON_NULL_RETURN_VAL(_client_displays, NULL);

   if (!name)
     {
        const char *n;

        /* client wants to connected to default display */
        n = getenv("WAYLAND_DISPLAY");
        if (!n) n = "wayland-0";

        /* we have a default wayland display */

        /* check hash of cached client displays for this name */
        ewd = eina_hash_find(_client_displays, n);
     }
   else
     {
        /* client wants to connect to specific display */

        /* check hash of cached client displays for this name */
        ewd = eina_hash_find(_client_displays, name);
     }

   return ewd;
}

/**
 * @brief Retrieves the `wl_compositor` global object for the display.
 *
 * The `wl_compositor` interface is used for creating surfaces (`wl_surface`).
 *
 * @param display The Ecore_Wl2_Display.
 * @return The `struct wl_compositor` proxy, or NULL if not available or @p display is NULL.
 * @since 1.8
 */
EAPI struct wl_compositor *
ecore_wl2_display_compositor_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.compositor;
}
