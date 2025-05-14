/**
 * @file
 * @brief Ecore Wayland2 library main routines
 *
 * This file contains the initialization and shutdown routines for the
 * Ecore Wayland2 library, as well as event type definitions and
 * module loading logic.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef NEED_RUN_IN_TREE
# include "../../static_libs/buildsystem/buildsystem.h"
#endif

#include "ecore_wl2_private.h"

/* local variables */
/** @internal Keep track of how many times the library has been initialized. */
static int _ecore_wl2_init_count = 0;

/* external variables */
/** @internal Global flag to disable session recovery. Set by environment variable or API call. */
Eina_Bool no_session_recovery;
/** @internal Log domain for Ecore_Wl2. */
int _ecore_wl2_log_dom = -1;

/* public API variables */
/** @brief Event type for a new connection to the Wayland display. */
EAPI int ECORE_WL2_EVENT_CONNECT = 0;
/** @brief Event type for a disconnection from the Wayland display. */
EAPI int ECORE_WL2_EVENT_DISCONNECT = 0;
/** @brief Event type when a new global object is announced by the compositor. */
EAPI int ECORE_WL2_EVENT_GLOBAL_ADDED = 0;
/** @brief Event type when a global object is removed by the compositor. */
EAPI int ECORE_WL2_EVENT_GLOBAL_REMOVED = 0;
/** @brief Event type when a window receives focus. Event data is an Ecore_Wl2_Event_Focus. */
EAPI int ECORE_WL2_EVENT_FOCUS_IN = 0;
/** @brief Event type when a window loses focus. Event data is an Ecore_Wl2_Event_Focus. */
EAPI int ECORE_WL2_EVENT_FOCUS_OUT = 0;
/** @brief Event type when a drag operation enters a window. Event data is an Ecore_Wl2_Event_Dnd_Enter. */
EAPI int ECORE_WL2_EVENT_DND_ENTER = 0;
/** @brief Event type when a drag operation leaves a window. Event data is an Ecore_Wl2_Event_Dnd_Leave. */
EAPI int ECORE_WL2_EVENT_DND_LEAVE = 0;
/** @brief Event type when a drag operation moves within a window. Event data is an Ecore_Wl2_Event_Dnd_Motion. */
EAPI int ECORE_WL2_EVENT_DND_MOTION = 0;
/** @brief Event type when data is dropped onto a window. Event data is an Ecore_Wl2_Event_Dnd_Drop. */
EAPI int ECORE_WL2_EVENT_DND_DROP = 0;
/** @brief Event type when a drag and drop operation finishes. */
EAPI int ECORE_WL2_EVENT_DND_END = 0;
/** @brief Event type when a data source operation ends. */
EAPI int ECORE_WL2_EVENT_DATA_SOURCE_END = 0;
/** @brief Event type when a data source is dropped (cancelled). */
EAPI int ECORE_WL2_EVENT_DATA_SOURCE_DROP = 0;
/** @brief Event type indicating the action performed by the drop target. Event data is an Ecore_Wl2_Event_Data_Source_Action. */
EAPI int ECORE_WL2_EVENT_DATA_SOURCE_ACTION = 0;
/** @brief Event type indicating the target MIME type for a data source. Event data is an Ecore_Wl2_Event_Data_Source_Target. */
EAPI int ECORE_WL2_EVENT_DATA_SOURCE_TARGET = 0;
/** @brief Event type to request sending data for a MIME type. Event data is an Ecore_Wl2_Event_Data_Source_Send. */
EAPI int ECORE_WL2_EVENT_DATA_SOURCE_SEND = 0;
/** @brief Event type when a window configuration changes. Event data is an Ecore_Wl2_Event_Window_Configure. */
EAPI int ECORE_WL2_EVENT_WINDOW_CONFIGURE = 0;
/** @brief Event type when a Wayland sync is done. */
EAPI int ECORE_WL2_EVENT_SYNC_DONE = 0;
/** @brief Event type when data for a drag-and-drop offer is ready. Event data is an Ecore_Wl2_Event_Offer_Data_Ready. */
EAPI int ECORE_WL2_EVENT_OFFER_DATA_READY = 0;
/** @brief Event type when a seat's name changes. Event data is an Ecore_Wl2_Event_Seat_Name. */
EAPI int ECORE_WL2_EVENT_SEAT_NAME_CHANGED = 0;
/** @brief Event type when a seat's capabilities change. Event data is an Ecore_Wl2_Event_Seat_Capabilities. */
EAPI int ECORE_WL2_EVENT_SEAT_CAPABILITIES_CHANGED = 0;
/** @brief Event type when an input device is added. Event data is an Ecore_Wl2_Event_Device_Added. */
EAPI int ECORE_WL2_EVENT_DEVICE_ADDED = 0;
/** @brief Event type when an input device is removed. Event data is an Ecore_Wl2_Event_Device_Removed. */
EAPI int ECORE_WL2_EVENT_DEVICE_REMOVED = 0;
/** @brief Event type when a window configure sequence is complete. Event data is an Ecore_Wl2_Event_Window_Configure_Complete. */
EAPI int ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE = 0;
/** @brief Event type when a seat's keymap changes. Event data is an Ecore_Wl2_Event_Seat_Keymap. */
EAPI int ECORE_WL2_EVENT_SEAT_KEYMAP_CHANGED = 0;
/** @brief Event type when a seat's keyboard repeat info changes. Event data is an Ecore_Wl2_Event_Seat_Keyboard_Repeat. */
EAPI int ECORE_WL2_EVENT_SEAT_KEYBOARD_REPEAT_CHANGED = 0;
/** @brief Event type for selection events on a seat. */
EAPI int ECORE_WL2_EVENT_SEAT_SELECTION = 0;
/** @brief Event type when an output's transform changes. Event data is an Ecore_Wl2_Event_Output_Transform. */
EAPI int ECORE_WL2_EVENT_OUTPUT_TRANSFORM = 0;
/** @brief Event type when a window is requested to rotate. Event data is an Ecore_Wl2_Event_Window_Rotation. */
EAPI int ECORE_WL2_EVENT_WINDOW_ROTATE = 0;
/** @brief Event type indicating a window rotation change is about to occur. Event data is an Ecore_Wl2_Event_Window_Rotation_Change_Prepare. */
EAPI int ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE = 0;
/** @brief Event type indicating the application is done preparing for a window rotation. */
EAPI int ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE_DONE = 0;
/** @brief Event type for a request to change window rotation. Event data is an Ecore_Wl2_Event_Window_Rotation_Change_Request. */
EAPI int ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_REQUEST = 0;
/** @brief Event type indicating a window rotation change has completed. Event data is an Ecore_Wl2_Event_Window_Rotation_Change_Done. */
EAPI int ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_DONE = 0;
/** @brief Event type indicating whether an auxiliary hint is allowed. Event data is an Ecore_Wl2_Event_Aux_Hint_Allowed. */
EAPI int ECORE_WL2_EVENT_AUX_HINT_ALLOWED = 0;
/** @brief Event type indicating whether an auxiliary hint is supported. Event data is an Ecore_Wl2_Event_Aux_Hint_Supported. */
EAPI int ECORE_WL2_EVENT_AUX_HINT_SUPPORTED = 0;
/** @brief Event type for an auxiliary message. Event data is an Ecore_Wl2_Event_Aux_Message. */
EAPI int ECORE_WL2_EVENT_AUX_MESSAGE = 0;
/** @brief Event type when a window is shown. Event data is an Ecore_Wl2_Event_Window_Show. */
EAPI int ECORE_WL2_EVENT_WINDOW_SHOW = 0;
/** @brief Event type when a window is hidden. Event data is an Ecore_Wl2_Event_Window_Hide. */
EAPI int ECORE_WL2_EVENT_WINDOW_HIDE = 0;
/** @brief Event type when a window is activated. Event data is an Ecore_Wl2_Event_Window_Activate. */
EAPI int ECORE_WL2_EVENT_WINDOW_ACTIVATE = 0;
/** @brief Event type when a window is deactivated. Event data is an Ecore_Wl2_Event_Window_Deactivate. */
EAPI int ECORE_WL2_EVENT_WINDOW_DEACTIVATE = 0;
/** @brief Event type when a window's iconify state changes. Event data is an Ecore_Wl2_Event_Window_Iconify_State_Change. */
EAPI int ECORE_WL2_EVENT_WINDOW_ICONIFY_STATE_CHANGE = 0;
/** @brief Event type when a window goes offscreen. Event data is an Ecore_Wl2_Event_Window_Offscreen. */
EAPI int ECORE_WL2_EVENT_WINDOW_OFFSCREEN = 0;
/** @brief Event type when a new Ecore_Wl2_Window is created. Event data is the Ecore_Wl2_Window itself. */
EAPI int ECORE_WL2_EVENT_WINDOW_CREATE = 0;
/** @brief Event type when an Ecore_Wl2_Window is destroyed. Event data is the Ecore_Wl2_Window itself. */
EAPI int ECORE_WL2_EVENT_WINDOW_DESTROY = 0;

/** @internal Event type for WWW window interactions. */
EAPI int _ecore_wl2_event_window_www = -1;
/** @internal Event type for WWW window drag interactions. */
EAPI int _ecore_wl2_event_window_www_drag = -1;

/** @internal Array of Eina_Module for supplied surface engine modules.
 * Example: `Eina_Module` instances for engines like "dmabuf".
 */
static Eina_Array *supplied_modules = NULL;
/** @internal Array of Eina_Module for locally specified surface engine modules.
 * Loaded from path specified in `ECORE_WL2_SURFACE_MODULE_DIR` environment variable.
 * Example: `Eina_Module` instances for custom or user-provided engines.
 */
static Eina_Array *local_modules = NULL;

/**
 * @internal
 * @brief Initializes and loads Wayland surface engine modules.
 *
 * This function attempts to load surface engine modules first from an
 * in-tree build location (if `NEED_RUN_IN_TREE` is defined and effective UID
 * matches real UID), then from the standard system path
 * (`PACKAGE_LIB_DIR"/ecore_wl2/engines"`), and finally from a custom path
 * specified by the `ECORE_WL2_SURFACE_MODULE_DIR` environment variable.
 *
 * @return EINA_TRUE on success or if at least one module is loaded,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_wl2_surface_modules_init(void)
{
   const char *mod_dir;

#ifdef NEED_RUN_IN_TREE
# if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
# endif
     {
        char path[PATH_MAX];
        //when running in tree we are ignoring all the settings
        //and just load the intree module that we have build
        if (bs_mod_get(path, sizeof(path), "ecore_wl2/engines","dmabuf"))
          {
             Eina_Module *local_module = eina_module_new(path);
             EINA_SAFETY_ON_NULL_RETURN_VAL(local_module, EINA_FALSE);

             if (!eina_module_load(local_module))
               {
                  ERR("Cannot load module %s", path);
                  eina_module_free(local_module);
                  local_module = NULL;
                  return EINA_FALSE;
               }
             return EINA_TRUE;
          }
     }
#endif
   supplied_modules =
     eina_module_arch_list_get(NULL, PACKAGE_LIB_DIR"/ecore_wl2/engines",
                               MODULE_ARCH);
   eina_module_list_load(supplied_modules);

   mod_dir = getenv("ECORE_WL2_SURFACE_MODULE_DIR");
   if (mod_dir)
     {
        local_modules =
          eina_module_list_get(NULL, mod_dir, EINA_TRUE, NULL, NULL);
        eina_module_list_load(local_modules);
     }

   if (!supplied_modules && !local_modules)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Unloads all loaded Wayland surface engine modules.
 *
 * This function unloads modules from both the supplied_modules list
 * (system/package modules) and the local_modules list (user-specified
 * modules).
 */
static void
_ecore_wl2_surface_modules_unload(void)
{
   eina_module_list_unload(supplied_modules);
   eina_module_list_unload(local_modules);
}

/* public API functions */
/**
 * @brief Initializes the Ecore_Wl2 library.
 *
 * This function initializes all the necessary sub-components for Ecore_Wl2
 * to operate, including Eina, Ecore, Ecore_Event, and loads surface modules.
 * It also registers a log domain for Ecore_Wl2 and creates event types.
 *
 * This function should be called before any other Ecore_Wl2 function.
 *
 * @return The new initialization count. 1 on the first successful
 *         initialization, >1 if already initialized, 0 on failure.
 *
 * @see ecore_wl2_shutdown()
 */
EAPI int
ecore_wl2_init(void)
{
   if (++_ecore_wl2_init_count != 1) return _ecore_wl2_init_count;

   /* try to initialize Eina */
   if (!eina_init()) return --_ecore_wl2_init_count;

   /* try to create Eina logging domain */
   _ecore_wl2_log_dom =
     eina_log_domain_register("ecore_wl2", ECORE_WL2_DEFAULT_LOG_COLOR);
   if (_ecore_wl2_log_dom < 0)
     {
        EINA_LOG_ERR("Cannot create a log domain for Ecore Wl2");
        goto eina_err;
     }

   /* try to initialize Ecore */
   if (!ecore_init())
     {
        ERR("Could not initialize Ecore");
        goto ecore_err;
     }

   /* try to initialize Ecore_Event */
   if (!ecore_event_init())
     {
        ERR("Could not initialize Ecore_Event");
        goto ecore_event_err;
     }

   if (!_ecore_wl2_surface_modules_init())
     {
        ERR("Could not load surface modules");
        goto module_load_err;
     }

   /* handle creating new Ecore_Wl2 event types */
   ECORE_WL2_EVENT_CONNECT = ecore_event_type_new();
   ECORE_WL2_EVENT_DISCONNECT = ecore_event_type_new();
   ECORE_WL2_EVENT_GLOBAL_ADDED = ecore_event_type_new();
   ECORE_WL2_EVENT_GLOBAL_REMOVED = ecore_event_type_new();
   ECORE_WL2_EVENT_FOCUS_IN = ecore_event_type_new();
   ECORE_WL2_EVENT_FOCUS_OUT = ecore_event_type_new();
   ECORE_WL2_EVENT_DND_ENTER = ecore_event_type_new();
   ECORE_WL2_EVENT_DND_LEAVE = ecore_event_type_new();
   ECORE_WL2_EVENT_DND_MOTION = ecore_event_type_new();
   ECORE_WL2_EVENT_DND_DROP = ecore_event_type_new();
   ECORE_WL2_EVENT_DND_END = ecore_event_type_new();
   ECORE_WL2_EVENT_DATA_SOURCE_END = ecore_event_type_new();
   ECORE_WL2_EVENT_DATA_SOURCE_DROP = ecore_event_type_new();
   ECORE_WL2_EVENT_DATA_SOURCE_ACTION = ecore_event_type_new();
   ECORE_WL2_EVENT_DATA_SOURCE_TARGET = ecore_event_type_new();
   ECORE_WL2_EVENT_DATA_SOURCE_SEND = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_CONFIGURE = ecore_event_type_new();
   ECORE_WL2_EVENT_SYNC_DONE = ecore_event_type_new();
   ECORE_WL2_EVENT_OFFER_DATA_READY = ecore_event_type_new();
   ECORE_WL2_EVENT_SEAT_NAME_CHANGED = ecore_event_type_new();
   ECORE_WL2_EVENT_SEAT_CAPABILITIES_CHANGED = ecore_event_type_new();
   ECORE_WL2_EVENT_DEVICE_ADDED = ecore_event_type_new();
   ECORE_WL2_EVENT_DEVICE_REMOVED = ecore_event_type_new();
   _ecore_wl2_event_window_www = ecore_event_type_new();
   _ecore_wl2_event_window_www_drag = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE = ecore_event_type_new();
   ECORE_WL2_EVENT_SEAT_KEYMAP_CHANGED = ecore_event_type_new();
   ECORE_WL2_EVENT_SEAT_KEYBOARD_REPEAT_CHANGED = ecore_event_type_new();
   ECORE_WL2_EVENT_SEAT_SELECTION = ecore_event_type_new();
   ECORE_WL2_EVENT_OUTPUT_TRANSFORM = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ROTATE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE_DONE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_REQUEST = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_DONE = ecore_event_type_new();
   ECORE_WL2_EVENT_AUX_HINT_ALLOWED = ecore_event_type_new();
   ECORE_WL2_EVENT_AUX_HINT_SUPPORTED = ecore_event_type_new();
   ECORE_WL2_EVENT_AUX_MESSAGE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_SHOW = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_HIDE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ACTIVATE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_DEACTIVATE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_ICONIFY_STATE_CHANGE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_OFFSCREEN = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_CREATE = ecore_event_type_new();
   ECORE_WL2_EVENT_WINDOW_DESTROY = ecore_event_type_new();

   if (!no_session_recovery)
     no_session_recovery = !!getenv("EFL_NO_WAYLAND_SESSION_RECOVERY");

   return _ecore_wl2_init_count;

module_load_err:
   ecore_event_shutdown();

ecore_event_err:
   ecore_shutdown();

ecore_err:
   eina_log_domain_unregister(_ecore_wl2_log_dom);
   _ecore_wl2_log_dom = -1;

eina_err:
   eina_shutdown();
   return --_ecore_wl2_init_count;
}

/**
 * @brief Shuts down the Ecore_Wl2 library.
 *
 * This function shuts down all the sub-components that were initialized by
 * ecore_wl2_init(). It also unloads surface modules, unregisters the log
 * domain, and flushes event types.
 *
 * This function should be called when Ecore_Wl2 is no longer needed,
 * typically at application exit. It should be called as many times as
 * ecore_wl2_init() was successfully called.
 *
 * @return The new initialization count. 0 when the library is fully shut down.
 *
 * @see ecore_wl2_init()
 */
EAPI int
ecore_wl2_shutdown(void)
{
   if (_ecore_wl2_init_count < 1)
     {
        EINA_LOG_ERR("Ecore_Wl2 shutdown called without Ecore_Wl2 Init");
        return 0;
     }

   if (--_ecore_wl2_init_count != 0) return _ecore_wl2_init_count;

   /* reset events */
   ecore_event_type_flush(ECORE_WL2_EVENT_CONNECT,
                          ECORE_WL2_EVENT_DISCONNECT,
                          ECORE_WL2_EVENT_GLOBAL_ADDED,
                          ECORE_WL2_EVENT_GLOBAL_REMOVED,
                          ECORE_WL2_EVENT_FOCUS_IN,
                          ECORE_WL2_EVENT_FOCUS_OUT,
                          ECORE_WL2_EVENT_DND_ENTER,
                          ECORE_WL2_EVENT_DND_LEAVE,
                          ECORE_WL2_EVENT_DND_MOTION,
                          ECORE_WL2_EVENT_DND_DROP,
                          ECORE_WL2_EVENT_DND_END,
                          ECORE_WL2_EVENT_DATA_SOURCE_END,
                          ECORE_WL2_EVENT_DATA_SOURCE_DROP,
                          ECORE_WL2_EVENT_DATA_SOURCE_ACTION,
                          ECORE_WL2_EVENT_DATA_SOURCE_TARGET,
                          ECORE_WL2_EVENT_DATA_SOURCE_SEND,
                          ECORE_WL2_EVENT_WINDOW_CONFIGURE,
                          ECORE_WL2_EVENT_SYNC_DONE,
                          ECORE_WL2_EVENT_OFFER_DATA_READY,
                          ECORE_WL2_EVENT_SEAT_NAME_CHANGED,
                          ECORE_WL2_EVENT_SEAT_CAPABILITIES_CHANGED,
                          ECORE_WL2_EVENT_DEVICE_ADDED,
                          ECORE_WL2_EVENT_DEVICE_REMOVED,
                          ECORE_WL2_EVENT_WINDOW_CONFIGURE_COMPLETE,
                          ECORE_WL2_EVENT_SEAT_KEYMAP_CHANGED,
                          ECORE_WL2_EVENT_SEAT_KEYBOARD_REPEAT_CHANGED,
                          ECORE_WL2_EVENT_SEAT_SELECTION,
                          ECORE_WL2_EVENT_OUTPUT_TRANSFORM,
                          ECORE_WL2_EVENT_WINDOW_ROTATE,
                          ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE,
                          ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_PREPARE_DONE,
                          ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_REQUEST,
                          ECORE_WL2_EVENT_WINDOW_ROTATION_CHANGE_DONE,
                          ECORE_WL2_EVENT_AUX_HINT_ALLOWED,
                          ECORE_WL2_EVENT_AUX_HINT_SUPPORTED,
                          ECORE_WL2_EVENT_AUX_MESSAGE,
                          ECORE_WL2_EVENT_WINDOW_SHOW,
                          ECORE_WL2_EVENT_WINDOW_HIDE,
                          ECORE_WL2_EVENT_WINDOW_ACTIVATE,
                          ECORE_WL2_EVENT_WINDOW_DEACTIVATE,
                          ECORE_WL2_EVENT_WINDOW_ICONIFY_STATE_CHANGE,
                          ECORE_WL2_EVENT_WINDOW_OFFSCREEN,
                          ECORE_WL2_EVENT_WINDOW_CREATE,
                          ECORE_WL2_EVENT_WINDOW_DESTROY);

   /* shutdown Ecore_Event */
   ecore_event_shutdown();

   /* shutdown Ecore */
   ecore_shutdown();

   /* unregister logging domain */
   eina_log_domain_unregister(_ecore_wl2_log_dom);
   _ecore_wl2_log_dom = -1;

   _ecore_wl2_surface_modules_unload();

   /* shutdown eina */
   eina_shutdown();

   return _ecore_wl2_init_count;
}

/**
 * @brief Disables Wayland session recovery.
 *
 * By default, Ecore_Wl2 may attempt to recover from Wayland compositor
 * crashes or restarts if the underlying platform supports it. Calling this
 * function disables this behavior.
 * This can also be disabled by setting the environment variable
 * `EFL_NO_WAYLAND_SESSION_RECOVERY`.
 */
EAPI void
ecore_wl2_session_recovery_disable(void)
{
   no_session_recovery = EINA_TRUE;
}
