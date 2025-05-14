#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#define LOGRT 1

#ifdef LOGRT
#include <dlfcn.h>
#endif /* ifdef LOGRT */

#include "Ecore.h"
#include "ecore_private.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"
#include "Ecore_Input.h"

static Ecore_X_Version _version = { VMAJ, VMIN, VMIC, VREV };
/**
 * @brief The version of the Ecore_X library.
 */
EAPI Ecore_X_Version *ecore_x_version = &_version;

static Eina_Bool _ecore_x_fd_handler(void *data,
                                     Ecore_Fd_Handler *fd_handler);
static Eina_Bool _ecore_x_fd_handler_buf(void *data,
                                         Ecore_Fd_Handler *fd_handler);
static int       _ecore_x_key_mask_get(XModifierKeymap *mod, KeySym sym);
static int       _ecore_x_event_modifier(unsigned int state);

/**< Handler for the X server file descriptor */
static Ecore_Fd_Handler *_ecore_x_fd_handler_handle = NULL;

static const int AnyXEvent = 0; /* 0 can be used as there are no event types
                                 * with index 0 and 1 as they are used for
                                 * errors
                                 */

static int _ecore_x_event_shape_id = 0; /**< Event ID for Shape extension */
static int _ecore_x_event_screensaver_id = 0; /**< Event ID for Screensaver extension */
static int _ecore_x_event_sync_id = 0; /**< Event ID for Sync extension */
int _ecore_xlib_log_dom = -1; /**< Log domain for Ecore_Xlib */

Eina_Bool _ecore_xlib_sync = EINA_FALSE; /**< Flag to enable/disable X synchronization after calls */

#ifdef ECORE_XRANDR
static int _ecore_x_event_randr_id = 0;
#endif /* ifdef ECORE_XRANDR */
#ifdef ECORE_XFIXES
static int _ecore_x_event_fixes_selection_id = 0;
#endif /* ifdef ECORE_XFIXES */
#ifdef ECORE_XDAMAGE
static int _ecore_x_event_damage_id = 0;
#endif /* ifdef ECORE_XDAMAGE */
#ifdef ECORE_XKB
static int _ecore_x_event_xkb_id = 0;
#endif /* ifdef ECORE_XKB */
static int _ecore_x_event_handlers_num = 0;
typedef void (*Ecore_X_Event_Handler) (XEvent *event);
static Ecore_X_Event_Handler *_ecore_x_event_handlers = NULL;

static int _ecore_x_init_count = 0;
static int _ecore_x_grab_count = 0;

Display *_ecore_x_disp = NULL;
double _ecore_x_double_click_time = 0.25;
Time _ecore_x_event_last_time = 0;
Window _ecore_x_event_last_win = 0;
int _ecore_x_event_last_root_x = 0;
int _ecore_x_event_last_root_y = 0;
Eina_Bool _ecore_x_xcursor = EINA_FALSE;

Ecore_X_Window _ecore_x_private_win = 0;

Ecore_X_Atom _ecore_x_atoms_wm_protocols[ECORE_X_WM_PROTOCOL_NUM];

/** @brief A generic Ecore_X event. */
EAPI int ECORE_X_EVENT_ANY = 0;
/** @brief Mouse enter event. */
EAPI int ECORE_X_EVENT_MOUSE_IN = 0;
/** @brief Mouse leave event. */
EAPI int ECORE_X_EVENT_MOUSE_OUT = 0;
/** @brief Window focus in event. */
EAPI int ECORE_X_EVENT_WINDOW_FOCUS_IN = 0;
/** @brief Window focus out event. */
EAPI int ECORE_X_EVENT_WINDOW_FOCUS_OUT = 0;
/** @brief Window keymap event. */
EAPI int ECORE_X_EVENT_WINDOW_KEYMAP = 0;
/** @brief Window damage event. */
EAPI int ECORE_X_EVENT_WINDOW_DAMAGE = 0;
/** @brief Window visibility change event. */
EAPI int ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE = 0;
/** @brief Window creation event. */
EAPI int ECORE_X_EVENT_WINDOW_CREATE = 0;
/** @brief Window destruction event. */
EAPI int ECORE_X_EVENT_WINDOW_DESTROY = 0;
/** @brief Window hide (unmap) event. */
EAPI int ECORE_X_EVENT_WINDOW_HIDE = 0;
/** @brief Window show (map) event. */
EAPI int ECORE_X_EVENT_WINDOW_SHOW = 0;
/** @brief Window show request (map request) event. */
EAPI int ECORE_X_EVENT_WINDOW_SHOW_REQUEST = 0;
/** @brief Window reparent event. */
EAPI int ECORE_X_EVENT_WINDOW_REPARENT = 0;
/** @brief Window configure (geometry change) event. */
EAPI int ECORE_X_EVENT_WINDOW_CONFIGURE = 0;
/** @brief Window configure request event. */
EAPI int ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST = 0;
/** @brief Window gravity change event. */
EAPI int ECORE_X_EVENT_WINDOW_GRAVITY = 0;
/** @brief Window resize request event. */
EAPI int ECORE_X_EVENT_WINDOW_RESIZE_REQUEST = 0;
/** @brief Window stack (raise/lower) event. */
EAPI int ECORE_X_EVENT_WINDOW_STACK = 0;
/** @brief Window stack request event. */
EAPI int ECORE_X_EVENT_WINDOW_STACK_REQUEST = 0;
/** @brief Window property change event. */
EAPI int ECORE_X_EVENT_WINDOW_PROPERTY = 0;
/** @brief Window colormap change event. */
EAPI int ECORE_X_EVENT_WINDOW_COLORMAP = 0;
/** @brief Window mapping event. */
EAPI int ECORE_X_EVENT_WINDOW_MAPPING = 0;
/** @brief Keyboard mapping change event. */
EAPI int ECORE_X_EVENT_MAPPING_CHANGE = 0;
/** @brief Selection clear event. */
EAPI int ECORE_X_EVENT_SELECTION_CLEAR = 0;
/** @brief Selection request event. */
EAPI int ECORE_X_EVENT_SELECTION_REQUEST = 0;
/** @brief Selection notify event. */
EAPI int ECORE_X_EVENT_SELECTION_NOTIFY = 0;
/** @brief XFixes selection notify event. */
EAPI int ECORE_X_EVENT_FIXES_SELECTION_NOTIFY = 0;
/** @brief Client message event. */
EAPI int ECORE_X_EVENT_CLIENT_MESSAGE = 0;
/** @brief Window shape change event. */
EAPI int ECORE_X_EVENT_WINDOW_SHAPE = 0;
/** @brief Screensaver notify event. */
EAPI int ECORE_X_EVENT_SCREENSAVER_NOTIFY = 0;
/** @brief Gesture flick notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_FLICK;
/** @brief Gesture pan notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_PAN;
/** @brief Gesture pinch/rotation notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION;
/** @brief Gesture tap notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_TAP;
/** @brief Gesture tap and hold notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_TAPNHOLD;
/** @brief Gesture hold notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_HOLD;
/** @brief Gesture group notify event. */
EAPI int ECORE_X_EVENT_GESTURE_NOTIFY_GROUP;
/** @brief Sync counter event. */
EAPI int ECORE_X_EVENT_SYNC_COUNTER = 0;
/** @brief Sync alarm event. */
EAPI int ECORE_X_EVENT_SYNC_ALARM = 0;
/** @brief Screen change event (XRandr). */
EAPI int ECORE_X_EVENT_SCREEN_CHANGE = 0;
/** @brief Damage notify event (XDamage). */
EAPI int ECORE_X_EVENT_DAMAGE_NOTIFY = 0;
/** @brief RandR CRTC change event. */
EAPI int ECORE_X_EVENT_RANDR_CRTC_CHANGE = 0;
/** @brief RandR output change event. */
EAPI int ECORE_X_EVENT_RANDR_OUTPUT_CHANGE = 0;
/** @brief RandR output property notify event. */
EAPI int ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY = 0;
/** @brief Window delete request event (WM_DELETE_WINDOW). */
EAPI int ECORE_X_EVENT_WINDOW_DELETE_REQUEST = 0;
/** @brief Window move/resize request event (NETWM). */
EAPI int ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST = 0;
/** @brief Window state request event (NETWM). */
EAPI int ECORE_X_EVENT_WINDOW_STATE_REQUEST = 0;
/** @brief Frame extents request event (NETWM). */
EAPI int ECORE_X_EVENT_FRAME_EXTENTS_REQUEST = 0;
/** @brief Ping event (NETWM). */
EAPI int ECORE_X_EVENT_PING = 0;
/** @brief Desktop change event (NETWM). */
EAPI int ECORE_X_EVENT_DESKTOP_CHANGE = 0;

/** @brief Startup sequence new event. */
EAPI int ECORE_X_EVENT_STARTUP_SEQUENCE_NEW = 0;
/** @brief Startup sequence change event. */
EAPI int ECORE_X_EVENT_STARTUP_SEQUENCE_CHANGE = 0;
/** @brief Startup sequence remove event. */
EAPI int ECORE_X_EVENT_STARTUP_SEQUENCE_REMOVE = 0;

/** @brief XKB state notify event. */
EAPI int ECORE_X_EVENT_XKB_STATE_NOTIFY = 0;
/** @brief XKB new keyboard notify event. */
EAPI int ECORE_X_EVENT_XKB_NEWKBD_NOTIFY = 0;

/** @brief Generic X event (XInput2). */
EAPI int ECORE_X_EVENT_GENERIC = 0;

/** @brief Present configure notify event. */
EAPI int ECORE_X_EVENT_PRESENT_CONFIGURE = 0;
/** @brief Present complete notify event. */
EAPI int ECORE_X_EVENT_PRESENT_COMPLETE = 0;
/** @brief Present idle notify event. */
EAPI int ECORE_X_EVENT_PRESENT_IDLE = 0;

/** @brief Shift modifier mask. */
EAPI int ECORE_X_MODIFIER_SHIFT = 0;
/** @brief Control modifier mask. */
EAPI int ECORE_X_MODIFIER_CTRL = 0;
/** @brief Alt modifier mask. */
EAPI int ECORE_X_MODIFIER_ALT = 0;
/** @brief Windows/Super modifier mask. */
EAPI int ECORE_X_MODIFIER_WIN = 0;
/** @brief AltGr modifier mask. */
EAPI int ECORE_X_MODIFIER_ALTGR = 0;

/** @brief ScrollLock lock mask. */
EAPI int ECORE_X_LOCK_SCROLL = 0;
/** @brief NumLock lock mask. */
EAPI int ECORE_X_LOCK_NUM = 0;
/** @brief CapsLock lock mask. */
EAPI int ECORE_X_LOCK_CAPS = 0;
/** @brief ShiftLock lock mask. */
EAPI int ECORE_X_LOCK_SHIFT = 0;

/** @brief Raw button press event (XInput2). */
EAPI int ECORE_X_RAW_BUTTON_PRESS = 0;
/** @brief Raw button release event (XInput2). */
EAPI int ECORE_X_RAW_BUTTON_RELEASE = 0;
/** @brief Raw motion event (XInput2). */
EAPI int ECORE_X_RAW_MOTION = 0;

/** @brief Devices change event (XInput2). */
EAPI int ECORE_X_DEVICES_CHANGE = 0;

#ifdef LOGRT
static double t0 = 0.0;
static Status (*_logrt_real_reply)(Display *disp,
                                   void *rep,
                                   int extra,
                                   Bool discard) = NULL;
static void
_logrt_init(void)
{
   void *lib;

   lib = dlopen("libX11.so", RTLD_GLOBAL | RTLD_LAZY);
   if (!lib)
     lib = dlopen("libX11.so.6", RTLD_GLOBAL | RTLD_LAZY);

   if (!lib)
     lib = dlopen("libX11.so.6.3", RTLD_GLOBAL | RTLD_LAZY);

   if (!lib)
     lib = dlopen("libX11.so.6.3.0", RTLD_GLOBAL | RTLD_LAZY);

   _logrt_real_reply = dlsym(lib, "_XReply");
   t0 = ecore_time_get();
}

Status
_XReply(Display *disp,
        void *rep,
        int extra,
        Bool discard)
{
   void *bt[128];
   int i, n;
   char **sym;

   n = backtrace(bt, 128);
   if (n > 0)
     {
        sym = backtrace_symbols(bt, n);
        printf("ROUNDTRIP: %4.4f :", ecore_time_get() - t0);
        if (sym)
          {
             for (i = n - 1; i > 0; i--)
               {
                  char *fname = strchr(sym[i], '(');
                  if (fname)
                    {
                       char *tsym = alloca(strlen(fname) + 1);
                       char *end;
                       strcpy(tsym, fname + 1);
                       end = strchr(tsym, '+');
                       if (end)
                         {
                            *end = 0;
                            printf("%s", tsym);
                         }
                       else
                         printf("???");
                    }
                  else
                    printf("???");

                  if (i > 1)
                    printf(" > ");
               }
             printf("\n");
          }
     }

   // fixme: logme
   return _logrt_real_reply(disp, rep, extra, discard);
}

#endif /* ifdef LOGRT */

/**
 * @internal
 * @brief Wrapper to use XkbKeycodeToKeysym when ECORE_XKB is defined,
 *        otherwise uses XKeycodeToKeysym.
 *
 * This function abstracts the keycode-to-keysym conversion, allowing
 * the use of XKB extensions if available, which can provide more
 * accurate or extended key mappings.
 *
 * @param display The display connection.
 * @param keycode The hardware keycode to convert.
 * @param idx The index of the keysym to retrieve for the given keycode
 *            (e.g., 0 for the unshifted symbol, 1 for the shifted symbol).
 * @return The KeySym corresponding to the keycode and index, or NoSymbol
 *         if no such keysym exists.
 */
KeySym
_ecore_x_XKeycodeToKeysym(Display *display, KeyCode keycode, int idx)
{
#ifdef ECORE_XKB
   return XkbKeycodeToKeysym(display, keycode, 0, idx);
#else
   return XKeycodeToKeysym(display, keycode, idx);
#endif
}

/**
 * @internal
 * @brief Retrieves and sets the global Ecore_X modifier and lock masks.
 *
 * This function queries the X server for the current modifier key mapping
 * (Shift, Ctrl, Alt, Win, AltGr) and lock key states (ScrollLock, NumLock,
 * CapsLock, ShiftLock). It then populates the global ECORE_X_MODIFIER_*
 * and ECORE_X_LOCK_* variables with the corresponding X modifier masks.
 * It also handles potential conflicts between modifiers (e.g., if Alt and
 * Win map to the same X modifier).
 */
void
_ecore_x_modifiers_get(void)
{
   XModifierKeymap *mod;
   ECORE_X_MODIFIER_SHIFT = 0;
   ECORE_X_MODIFIER_CTRL = 0;
   ECORE_X_MODIFIER_ALT = 0;
   ECORE_X_MODIFIER_WIN = 0;
   ECORE_X_MODIFIER_ALTGR = 0;
   ECORE_X_LOCK_SCROLL = 0;
   ECORE_X_LOCK_NUM = 0;
   ECORE_X_LOCK_CAPS = 0;
   ECORE_X_LOCK_SHIFT = 0;

   mod = XGetModifierMapping(_ecore_x_disp);
   if ((!mod) || (mod->max_keypermod <= 0)) goto clean_up;

   /* everything has these... unless its like a pda... :) */
   ECORE_X_MODIFIER_SHIFT = _ecore_x_key_mask_get(mod, XK_Shift_L);
   ECORE_X_MODIFIER_CTRL = _ecore_x_key_mask_get(mod, XK_Control_L);

   /* apple's xdarwin has no alt!!!! */
   ECORE_X_MODIFIER_ALT = _ecore_x_key_mask_get(mod, XK_Alt_L);
   if (!ECORE_X_MODIFIER_ALT)
     ECORE_X_MODIFIER_ALT = _ecore_x_key_mask_get(mod, XK_Meta_L);

   if (!ECORE_X_MODIFIER_ALT)
     ECORE_X_MODIFIER_ALT = _ecore_x_key_mask_get(mod, XK_Super_L);

   /* the windows key... a valid modifier :) */
   ECORE_X_MODIFIER_WIN = _ecore_x_key_mask_get(mod, XK_Super_L);
   if (!ECORE_X_MODIFIER_WIN)
     ECORE_X_MODIFIER_WIN = _ecore_x_key_mask_get(mod, XK_Meta_L);

   ECORE_X_MODIFIER_ALTGR = _ecore_x_key_mask_get(mod, XK_Mode_switch);

   if (ECORE_X_MODIFIER_WIN == ECORE_X_MODIFIER_ALT)
     ECORE_X_MODIFIER_WIN = 0;

   if (ECORE_X_MODIFIER_ALT == ECORE_X_MODIFIER_CTRL)
     ECORE_X_MODIFIER_ALT = 0;

   if (ECORE_X_MODIFIER_ALTGR)
     {
        if ((ECORE_X_MODIFIER_ALTGR == ECORE_X_MODIFIER_SHIFT) ||
            (ECORE_X_MODIFIER_ALTGR == ECORE_X_MODIFIER_CTRL) ||
            (ECORE_X_MODIFIER_ALTGR == ECORE_X_MODIFIER_ALT) ||
            (ECORE_X_MODIFIER_ALTGR == ECORE_X_MODIFIER_WIN))
          {
             ERR("ALTGR conflicts with other modifiers. IGNORE ALTGR");
             ECORE_X_MODIFIER_ALTGR = 0;
          }
     }

   if (ECORE_X_MODIFIER_ALT)
     {
        if ((ECORE_X_MODIFIER_ALT == ECORE_X_MODIFIER_SHIFT) ||
            (ECORE_X_MODIFIER_ALT == ECORE_X_MODIFIER_CTRL) ||
            (ECORE_X_MODIFIER_ALT == ECORE_X_MODIFIER_WIN))
          {
             ERR("ALT conflicts with other modifiers. IGNORE ALT");
             ECORE_X_MODIFIER_ALT = 0;
          }
     }

   if (ECORE_X_MODIFIER_WIN)
     {
        if ((ECORE_X_MODIFIER_WIN == ECORE_X_MODIFIER_SHIFT) ||
            (ECORE_X_MODIFIER_WIN == ECORE_X_MODIFIER_CTRL))
          {
             ERR("WIN conflicts with other modifiers. IGNORE WIN");
             ECORE_X_MODIFIER_WIN = 0;
          }
     }

   if (ECORE_X_MODIFIER_SHIFT)
     {
        if (ECORE_X_MODIFIER_SHIFT == ECORE_X_MODIFIER_CTRL)
          {
             ERR("CTRL conflicts with other modifiers. IGNORE CTRL");
             ECORE_X_MODIFIER_CTRL = 0;
          }
     }

   ECORE_X_LOCK_SCROLL = _ecore_x_key_mask_get(mod, XK_Scroll_Lock);
   ECORE_X_LOCK_NUM = _ecore_x_key_mask_get(mod, XK_Num_Lock);
   ECORE_X_LOCK_CAPS = _ecore_x_key_mask_get(mod, XK_Caps_Lock);
   ECORE_X_LOCK_SHIFT = _ecore_x_key_mask_get(mod, XK_Shift_Lock);

clean_up:
   if (mod)
     {
        if (mod->modifiermap) XFree(mod->modifiermap);
        XFree(mod);
     }
}

/**
 * @internal
 * @brief First stage of Ecore_X initialization.
 *
 * This function initializes Eina and Ecore core components, and registers
 * the Ecore_X log domain. It's the first part of the two-stage
 * initialization process for Ecore_X.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_x_init1(void)
{
   LOGFN;
#ifdef LOGRT
   _logrt_init();
#endif /* ifdef LOGRT */

   eina_init();
   _ecore_xlib_log_dom = eina_log_domain_register
       ("ecore_x", ECORE_XLIB_DEFAULT_LOG_COLOR);
   if (_ecore_xlib_log_dom < 0)
     {
        EINA_LOG_ERR(
          "Impossible to create a log domain for the Ecore Xlib module.");
        return EINA_FALSE;
     }

   if (!ecore_init())
     goto shutdown_eina;
   if (!ecore_event_init())
     goto shutdown_ecore;

   return EINA_TRUE;
shutdown_ecore:
   ecore_shutdown();
shutdown_eina:
   eina_log_domain_unregister(_ecore_xlib_log_dom);
   _ecore_xlib_log_dom = -1;
   eina_shutdown();
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Second stage of Ecore_X initialization.
 *
 * This function performs the X-specific parts of initialization after a
 * display connection has been established. This includes:
 * - Initializing error handlers.
 * - Querying for X extension support (Shape, Screensaver, Sync, RandR, etc.)
 *   and setting up their event IDs.
 * - Allocating and populating the X event handler array.
 * - Initializing Ecore_X event types.
 * - Getting initial modifier key mappings.
 * - Initializing various Ecore_X subsystems (atoms, ICCCM, NetWM, DND, etc.).
 * - Adding the X connection's file descriptor to the Ecore main loop.
 * - Creating a private helper window.
 *
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_x_init2(void)
{
   int shape_base = 0;
   int shape_err_base = 0;
#ifdef ECORE_XSS
   int screensaver_base = 0;
   int screensaver_err_base = 0;
#endif /* ifdef ECORE_XSS */
   int sync_base = 0;
   int sync_err_base = 0;
#ifdef ECORE_XRANDR
   int randr_base = 0;
   int randr_err_base = 0;
#endif /* ifdef ECORE_XRANDR */
#ifdef ECORE_XFIXES
   int fixes_base = 0;
   int fixes_err_base = 0;
#endif /* ifdef ECORE_XFIXES */
#ifdef ECORE_XDAMAGE
   int damage_base = 0;
   int damage_err_base = 0;
#endif /* ifdef ECORE_XDAMAGE */
#ifdef ECORE_XKB
   int xkb_base = 0;
#endif /* ifdef ECORE_XKB */

   _ecore_x_error_handler_init();
   _ecore_x_event_handlers_num = LASTEvent;

#define ECORE_X_EVENT_HANDLERS_GROW(ext_base, ext_num_events)            \
  do {                                                                   \
       if (_ecore_x_event_handlers_num < (ext_base + ext_num_events)) {  \
            _ecore_x_event_handlers_num = (ext_base + ext_num_events); } \
    } while (0)

   if (XShapeQueryExtension(_ecore_x_disp, &shape_base, &shape_err_base))
     _ecore_x_event_shape_id = shape_base;

   ECORE_X_EVENT_HANDLERS_GROW(shape_base, ShapeNumberEvents);

#ifdef ECORE_XSS
   if (XScreenSaverQueryExtension(_ecore_x_disp, &screensaver_base,
                                  &screensaver_err_base))
     _ecore_x_event_screensaver_id = screensaver_base;

   ECORE_X_EVENT_HANDLERS_GROW(screensaver_base, ScreenSaverNumberEvents);
#endif /* ifdef ECORE_XSS */

   if (XSyncQueryExtension(_ecore_x_disp, &sync_base, &sync_err_base))
     {
        int major, minor;

        _ecore_x_event_sync_id = sync_base;
        if (!XSyncInitialize(_ecore_x_disp, &major, &minor))
          _ecore_x_event_sync_id = 0;
     }

   ECORE_X_EVENT_HANDLERS_GROW(sync_base, XSyncNumberEvents);

#ifdef ECORE_XRANDR
   if (XRRQueryExtension(_ecore_x_disp, &randr_base, &randr_err_base))
     _ecore_x_event_randr_id = randr_base;

   ECORE_X_EVENT_HANDLERS_GROW(randr_base, RRNumberEvents);
#endif /* ifdef ECORE_XRANDR */

#ifdef ECORE_XFIXES
   if (XFixesQueryExtension(_ecore_x_disp, &fixes_base, &fixes_err_base))
     _ecore_x_event_fixes_selection_id = fixes_base;

   ECORE_X_EVENT_HANDLERS_GROW(fixes_base, XFixesNumberEvents);
#endif /* ifdef ECORE_XFIXES */

#ifdef ECORE_XDAMAGE
   if (XDamageQueryExtension(_ecore_x_disp, &damage_base, &damage_err_base))
     _ecore_x_event_damage_id = damage_base;

   ECORE_X_EVENT_HANDLERS_GROW(damage_base, XDamageNumberEvents);
#endif /* ifdef ECORE_XDAMAGE */

#ifdef ECORE_XKB
     {
        int dummy;

        if (XkbQueryExtension(_ecore_x_disp, &dummy, &xkb_base,
                              &dummy, &dummy, &dummy))
          _ecore_x_event_xkb_id = xkb_base;
        XkbSelectEventDetails(_ecore_x_disp, XkbUseCoreKbd, XkbStateNotify,
                              XkbAllStateComponentsMask, XkbGroupStateMask);
        XkbSelectEventDetails(_ecore_x_disp, XkbUseCoreKbd, XkbNewKeyboardNotify,
                              XkbNewKeyboardNotifyMask, XkbNewKeyboardNotifyMask);
     }
   ECORE_X_EVENT_HANDLERS_GROW(xkb_base, XkbNumberEvents);
#endif

   _ecore_x_event_handlers = calloc(_ecore_x_event_handlers_num, sizeof(Ecore_X_Event_Handler));
   if (!_ecore_x_event_handlers)
     goto close_display;

#ifdef ECORE_XCURSOR
   _ecore_x_xcursor = XcursorSupportsARGB(_ecore_x_disp) ? EINA_TRUE : EINA_FALSE;
#endif /* ifdef ECORE_XCURSOR */
   _ecore_x_event_handlers[AnyXEvent] = _ecore_x_event_handle_any_event;
   _ecore_x_event_handlers[KeyPress] = _ecore_x_event_handle_key_press;
   _ecore_x_event_handlers[KeyRelease] = _ecore_x_event_handle_key_release;
   _ecore_x_event_handlers[ButtonPress] = _ecore_x_event_handle_button_press;
   _ecore_x_event_handlers[ButtonRelease] = _ecore_x_event_handle_button_release;
   _ecore_x_event_handlers[MotionNotify] = _ecore_x_event_handle_motion_notify;
   _ecore_x_event_handlers[EnterNotify] = _ecore_x_event_handle_enter_notify;
   _ecore_x_event_handlers[LeaveNotify] = _ecore_x_event_handle_leave_notify;
   _ecore_x_event_handlers[FocusIn] = _ecore_x_event_handle_focus_in;
   _ecore_x_event_handlers[FocusOut] = _ecore_x_event_handle_focus_out;
   _ecore_x_event_handlers[KeymapNotify] = _ecore_x_event_handle_keymap_notify;
   _ecore_x_event_handlers[Expose] = _ecore_x_event_handle_expose;
   _ecore_x_event_handlers[GraphicsExpose] = _ecore_x_event_handle_graphics_expose;
   _ecore_x_event_handlers[VisibilityNotify] = _ecore_x_event_handle_visibility_notify;
   _ecore_x_event_handlers[CreateNotify] = _ecore_x_event_handle_create_notify;
   _ecore_x_event_handlers[DestroyNotify] = _ecore_x_event_handle_destroy_notify;
   _ecore_x_event_handlers[UnmapNotify] = _ecore_x_event_handle_unmap_notify;
   _ecore_x_event_handlers[MapNotify] = _ecore_x_event_handle_map_notify;
   _ecore_x_event_handlers[MapRequest] = _ecore_x_event_handle_map_request;
   _ecore_x_event_handlers[ReparentNotify] = _ecore_x_event_handle_reparent_notify;
   _ecore_x_event_handlers[ConfigureNotify] = _ecore_x_event_handle_configure_notify;
   _ecore_x_event_handlers[ConfigureRequest] = _ecore_x_event_handle_configure_request;
   _ecore_x_event_handlers[GravityNotify] = _ecore_x_event_handle_gravity_notify;
   _ecore_x_event_handlers[ResizeRequest] = _ecore_x_event_handle_resize_request;
   _ecore_x_event_handlers[CirculateNotify] = _ecore_x_event_handle_circulate_notify;
   _ecore_x_event_handlers[CirculateRequest] = _ecore_x_event_handle_circulate_request;
   _ecore_x_event_handlers[PropertyNotify] = _ecore_x_event_handle_property_notify;
   _ecore_x_event_handlers[SelectionClear] = _ecore_x_event_handle_selection_clear;
   _ecore_x_event_handlers[SelectionRequest] = _ecore_x_event_handle_selection_request;
   _ecore_x_event_handlers[SelectionNotify] = _ecore_x_event_handle_selection_notify;
   _ecore_x_event_handlers[ColormapNotify] = _ecore_x_event_handle_colormap_notify;
   _ecore_x_event_handlers[ClientMessage] = _ecore_x_event_handle_client_message;
   _ecore_x_event_handlers[MappingNotify] = _ecore_x_event_handle_mapping_notify;
#ifdef GenericEvent
   _ecore_x_event_handlers[GenericEvent] = _ecore_x_event_handle_generic_event;
#endif /* ifdef GenericEvent */

   if (_ecore_x_event_shape_id)
     _ecore_x_event_handlers[_ecore_x_event_shape_id] = _ecore_x_event_handle_shape_change;
   if (_ecore_x_event_screensaver_id)
     _ecore_x_event_handlers[_ecore_x_event_screensaver_id] = _ecore_x_event_handle_screensaver_notify;
   if (_ecore_x_event_sync_id)
     {
        _ecore_x_event_handlers[_ecore_x_event_sync_id + XSyncCounterNotify] = _ecore_x_event_handle_sync_counter;
        _ecore_x_event_handlers[_ecore_x_event_sync_id + XSyncAlarmNotify] = _ecore_x_event_handle_sync_alarm;
     }

#ifdef ECORE_XRANDR
   if (_ecore_x_event_randr_id)
     {
        _ecore_x_event_handlers[_ecore_x_event_randr_id + RRScreenChangeNotify] = _ecore_x_event_handle_randr_change;
        _ecore_x_event_handlers[_ecore_x_event_randr_id + RRNotify] = _ecore_x_event_handle_randr_notify;
     }
#endif /* ifdef ECORE_XRANDR */
#ifdef ECORE_XFIXES
   if (_ecore_x_event_fixes_selection_id)
     _ecore_x_event_handlers[_ecore_x_event_fixes_selection_id] = _ecore_x_event_handle_fixes_selection_notify;

#endif /* ifdef ECORE_XFIXES */
#ifdef ECORE_XDAMAGE
   if (_ecore_x_event_damage_id)
     _ecore_x_event_handlers[_ecore_x_event_damage_id] = _ecore_x_event_handle_damage_notify;

#endif /* ifdef ECORE_XDAMAGE */
#ifdef ECORE_XKB
   // set x autorepeat detection to on. that means instead of
   //   press-release-press-release-press-release
   // you get
   //   press-press-press-press-press-release
   do
     {
        Bool works = 0;
        XkbSetDetectableAutoRepeat(_ecore_x_disp, 1, &works);
     }
     while (0);
   if (_ecore_x_event_xkb_id)
   _ecore_x_event_handlers[_ecore_x_event_xkb_id] = _ecore_x_event_handle_xkb;
#endif /* ifdef ECORE_XKB */

   ECORE_X_EVENT_ANY = ecore_event_type_new();
   ECORE_X_EVENT_MOUSE_IN = ecore_event_type_new();
   ECORE_X_EVENT_MOUSE_OUT = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_FOCUS_IN = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_FOCUS_OUT = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_KEYMAP = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_DAMAGE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_CREATE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_DESTROY = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_HIDE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_SHOW = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_SHOW_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_REPARENT = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_CONFIGURE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_GRAVITY = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_RESIZE_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_STACK = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_STACK_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_PROPERTY = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_COLORMAP = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_MAPPING = ecore_event_type_new();
   ECORE_X_EVENT_MAPPING_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_SELECTION_CLEAR = ecore_event_type_new();
   ECORE_X_EVENT_SELECTION_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_SELECTION_NOTIFY = ecore_event_type_new();
   ECORE_X_EVENT_CLIENT_MESSAGE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_SHAPE = ecore_event_type_new();
   ECORE_X_EVENT_SCREENSAVER_NOTIFY = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_FLICK = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_PAN = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_TAP = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_TAPNHOLD = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_HOLD = ecore_event_type_new();
   ECORE_X_EVENT_GESTURE_NOTIFY_GROUP = ecore_event_type_new();
   ECORE_X_EVENT_SYNC_COUNTER = ecore_event_type_new();
   ECORE_X_EVENT_SYNC_ALARM = ecore_event_type_new();
   ECORE_X_EVENT_SCREEN_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_RANDR_CRTC_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_RANDR_OUTPUT_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY = ecore_event_type_new();
   ECORE_X_EVENT_DAMAGE_NOTIFY = ecore_event_type_new();

   ECORE_X_EVENT_WINDOW_DELETE_REQUEST = ecore_event_type_new();

   ECORE_X_EVENT_DESKTOP_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_WINDOW_STATE_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_FRAME_EXTENTS_REQUEST = ecore_event_type_new();
   ECORE_X_EVENT_PING = ecore_event_type_new();

   ECORE_X_EVENT_STARTUP_SEQUENCE_NEW = ecore_event_type_new();
   ECORE_X_EVENT_STARTUP_SEQUENCE_CHANGE = ecore_event_type_new();
   ECORE_X_EVENT_STARTUP_SEQUENCE_REMOVE = ecore_event_type_new();

   ECORE_X_EVENT_XKB_STATE_NOTIFY = ecore_event_type_new();
   ECORE_X_EVENT_XKB_NEWKBD_NOTIFY = ecore_event_type_new();

   ECORE_X_EVENT_GENERIC = ecore_event_type_new();

   ECORE_X_RAW_BUTTON_PRESS = ecore_event_type_new();
   ECORE_X_RAW_BUTTON_RELEASE = ecore_event_type_new();
   ECORE_X_RAW_MOTION = ecore_event_type_new();

   ECORE_X_DEVICES_CHANGE = ecore_event_type_new();

   _ecore_x_modifiers_get();

   _ecore_x_atoms_init();

   /* Set up the ICCCM hints */
   ecore_x_icccm_init();

   /* Set up the _NET_... hints */
   ecore_x_netwm_init();

   /* old e hints init */
   ecore_x_e_init();

   /* This is just to be anal about naming conventions */

   _ecore_x_atoms_wm_protocols[ECORE_X_WM_PROTOCOL_DELETE_REQUEST] =
     ECORE_X_ATOM_WM_DELETE_WINDOW;
   _ecore_x_atoms_wm_protocols[ECORE_X_WM_PROTOCOL_TAKE_FOCUS] =
     ECORE_X_ATOM_WM_TAKE_FOCUS;
   _ecore_x_atoms_wm_protocols[ECORE_X_NET_WM_PROTOCOL_PING] =
     ECORE_X_ATOM_NET_WM_PING;
   _ecore_x_atoms_wm_protocols[ECORE_X_NET_WM_PROTOCOL_SYNC_REQUEST] =
     ECORE_X_ATOM_NET_WM_SYNC_REQUEST;

   _ecore_x_selection_data_init();
   _ecore_x_dnd_init();
   _ecore_x_fixes_init();
   _ecore_x_damage_init();
   _ecore_x_composite_init();
   _ecore_x_present_init();
   _ecore_x_dpms_init();
   _ecore_x_randr_init();
   _ecore_x_input_init();
   _ecore_x_events_init();

   _ecore_x_fd_handler_handle =
     ecore_main_fd_handler_add(ConnectionNumber(_ecore_x_disp),
                               ECORE_FD_READ,
                               _ecore_x_fd_handler, _ecore_x_disp,
                               _ecore_x_fd_handler_buf, _ecore_x_disp);
   if (!_ecore_x_fd_handler_handle)
     goto free_event_handlers;

   _ecore_x_private_win = ecore_x_window_override_new(0, -77, -777, 123, 456);
   _ecore_xlib_sync = !!getenv("ECORE_X_SYNC");

   return EINA_TRUE;

free_event_handlers:
   free(_ecore_x_event_handlers);
   _ecore_x_event_handlers = NULL;
close_display:
   XCloseDisplay(_ecore_x_disp);
   _ecore_x_fd_handler_handle = NULL;
   _ecore_x_disp = NULL;
   ecore_event_shutdown();
   ecore_shutdown();
   eina_log_domain_unregister(_ecore_xlib_log_dom);
   _ecore_xlib_log_dom = -1;
   eina_shutdown();
   return EINA_FALSE;
}

/**
 * @defgroup Ecore_X_Init_Group X Library Initialization and Shutdown Functions
 * @ingroup Ecore_X_Group
 * Functions that start and shut down the Ecore X Library.
 */

/**
 * Initialize the X display connection to the given display.
 *
 * @param   name Display target name.  If @c NULL, the default display is
 *               assumed.
 * @return  The number of times the library has been initialized without
 *          being shut down.  0 is returned if an error occurs.
 * @ingroup Ecore_X_Init_Group
 */
EAPI int
ecore_x_init(const char *name)
{
   if (++_ecore_x_init_count != 1)
     return _ecore_x_init_count;

   if (!_ecore_x_init1())
     return --_ecore_x_init_count;

#ifdef EVAS_FRAME_QUEUING
   XInitThreads();
#endif /* ifdef EVAS_FRAME_QUEUING */
   _ecore_x_disp = XOpenDisplay((char *)name);
   if (!_ecore_x_disp)
     goto shutdown_ecore_event;
   if (_ecore_x_init2())
     return _ecore_x_init_count;
shutdown_ecore_event:
   ecore_event_shutdown();
   ecore_shutdown();
   eina_log_domain_unregister(_ecore_xlib_log_dom);
   _ecore_xlib_log_dom = -1;
   eina_shutdown();
   return --_ecore_x_init_count;
}

/**
 * Initialize the X display connection using an existing X Display.
 *
 * @param   display An existing X Display connection.
 * @return  The number of times the library has been initialized without
 *          being shut down.  0 is returned if an error occurs.
 * @ingroup Ecore_X_Init_Group
 */
EAPI int
ecore_x_init_from_display(Ecore_X_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, 0);
   if (++_ecore_x_init_count != 1)
     return _ecore_x_init_count;

   if (!_ecore_x_init1())
     return --_ecore_x_init_count;
   _ecore_x_disp = display;
   if (_ecore_x_init2())
     return _ecore_x_init_count;
   ecore_event_shutdown();
   ecore_shutdown();
   eina_log_domain_unregister(_ecore_xlib_log_dom);
   _ecore_xlib_log_dom = -1;
   eina_shutdown();
   return --_ecore_x_init_count;
}
static Eina_Bool _ecore_x_window_manage_succeeded = EINA_FALSE; /**< Flag to track if the last XSelectInput for managing a window succeeded */

/**
 * @internal
 * @brief Internal part of the Ecore_X shutdown process.
 *
 * This function performs the core cleanup tasks when Ecore_X is shut down,
 * assuming a display connection exists. It flushes event types, removes
 * the X FD handler, frees event handler structures, and shuts down
 * various Ecore_X subsystems.
 *
 * @return Always returns 0.
 */
int
_ecore_x_shutdown(void)
{
   if (!_ecore_x_disp)
     return 0;

   LOGFN;

   ecore_event_type_flush(ECORE_X_EVENT_ANY,
                          ECORE_X_EVENT_MOUSE_IN,
                          ECORE_X_EVENT_MOUSE_OUT,
                          ECORE_X_EVENT_WINDOW_FOCUS_IN,
                          ECORE_X_EVENT_WINDOW_FOCUS_OUT,
                          ECORE_X_EVENT_WINDOW_KEYMAP,
                          ECORE_X_EVENT_WINDOW_DAMAGE,
                          ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE,
                          ECORE_X_EVENT_WINDOW_CREATE,
                          ECORE_X_EVENT_WINDOW_DESTROY,
                          ECORE_X_EVENT_WINDOW_HIDE,
                          ECORE_X_EVENT_WINDOW_SHOW,
                          ECORE_X_EVENT_WINDOW_SHOW_REQUEST,
                          ECORE_X_EVENT_WINDOW_REPARENT,
                          ECORE_X_EVENT_WINDOW_CONFIGURE,
                          ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST,
                          ECORE_X_EVENT_WINDOW_GRAVITY,
                          ECORE_X_EVENT_WINDOW_RESIZE_REQUEST,
                          ECORE_X_EVENT_WINDOW_STACK,
                          ECORE_X_EVENT_WINDOW_STACK_REQUEST,
                          ECORE_X_EVENT_WINDOW_PROPERTY,
                          ECORE_X_EVENT_WINDOW_COLORMAP,
                          ECORE_X_EVENT_WINDOW_MAPPING,
                          ECORE_X_EVENT_MAPPING_CHANGE,
                          ECORE_X_EVENT_SELECTION_CLEAR,
                          ECORE_X_EVENT_SELECTION_REQUEST,
                          ECORE_X_EVENT_SELECTION_NOTIFY,
                          ECORE_X_EVENT_CLIENT_MESSAGE,
                          ECORE_X_EVENT_WINDOW_SHAPE,
                          ECORE_X_EVENT_SCREENSAVER_NOTIFY,
                          ECORE_X_EVENT_GESTURE_NOTIFY_FLICK,
                          ECORE_X_EVENT_GESTURE_NOTIFY_PAN,
                          ECORE_X_EVENT_GESTURE_NOTIFY_PINCHROTATION,
                          ECORE_X_EVENT_GESTURE_NOTIFY_TAP,
                          ECORE_X_EVENT_GESTURE_NOTIFY_TAPNHOLD,
                          ECORE_X_EVENT_GESTURE_NOTIFY_HOLD,
                          ECORE_X_EVENT_GESTURE_NOTIFY_GROUP,
                          ECORE_X_EVENT_SYNC_COUNTER,
                          ECORE_X_EVENT_SYNC_ALARM,
                          ECORE_X_EVENT_SCREEN_CHANGE,
                          ECORE_X_EVENT_RANDR_CRTC_CHANGE,
                          ECORE_X_EVENT_RANDR_OUTPUT_CHANGE,
                          ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY,
                          ECORE_X_EVENT_DAMAGE_NOTIFY,
                          ECORE_X_EVENT_WINDOW_DELETE_REQUEST,
                          ECORE_X_EVENT_DESKTOP_CHANGE,
                          ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST,
                          ECORE_X_EVENT_WINDOW_STATE_REQUEST,
                          ECORE_X_EVENT_FRAME_EXTENTS_REQUEST,
                          ECORE_X_EVENT_PING,
                          ECORE_X_EVENT_STARTUP_SEQUENCE_NEW,
                          ECORE_X_EVENT_STARTUP_SEQUENCE_CHANGE,
                          ECORE_X_EVENT_STARTUP_SEQUENCE_REMOVE,
                          ECORE_X_EVENT_XKB_STATE_NOTIFY,
                          ECORE_X_EVENT_XKB_NEWKBD_NOTIFY,
                          ECORE_X_EVENT_GENERIC,
                          ECORE_X_RAW_BUTTON_PRESS,
                          ECORE_X_RAW_BUTTON_RELEASE,
                          ECORE_X_RAW_MOTION,
                          ECORE_X_EVENT_PRESENT_CONFIGURE,
                          ECORE_X_EVENT_PRESENT_COMPLETE,
                          ECORE_X_EVENT_PRESENT_IDLE);
   ecore_main_fd_handler_del(_ecore_x_fd_handler_handle);

   free(_ecore_x_event_handlers);
   _ecore_x_fd_handler_handle = NULL;
   _ecore_x_event_handlers = NULL;
   _ecore_x_window_manage_succeeded = EINA_FALSE;
   _ecore_x_events_shutdown();
   _ecore_x_input_shutdown();
   _ecore_x_selection_shutdown();
   _ecore_x_dnd_shutdown();
   _ecore_x_resource_shutdown();
   ecore_x_netwm_shutdown();

   return 0;
}

/**
 * @internal
 * @brief Second part of the internal Ecore_X shutdown process.
 *
 * This function shuts down Ecore event and core systems, unregisters the
 * Ecore_X log domain, and shuts down Eina. It also resets the
 * _ecore_xlib_sync flag.
 */
static void
_ecore_x_shutdown2(void)
{
   ecore_event_shutdown();
   ecore_shutdown();

   eina_log_domain_unregister(_ecore_xlib_log_dom);
   _ecore_xlib_log_dom = -1;
   eina_shutdown();
   _ecore_xlib_sync = EINA_FALSE;
}

/**
 * Shuts down the Ecore X library.
 *
 * In shutting down the library, the X display connection is terminated
 * and any event handlers for it are removed.
 *
 * @return  The number of times the library has been initialized without
 *          being shut down. 0 is returned if an error occurs.
 * @ingroup Ecore_X_Init_Group
 */
EAPI int
ecore_x_shutdown(void)
{
   if (!_ecore_x_init_count)
     {
        CRI("Calling ecore_x_shutdown without init! BUG!");
        return 0;
     }
   if (--_ecore_x_init_count != 0)
     return _ecore_x_init_count;
   if (_ecore_x_shutdown()) return _ecore_x_init_count;
   if (_ecore_x_disp)
     XCloseDisplay(_ecore_x_disp);
   _ecore_x_disp = NULL;
   _ecore_x_shutdown2();
   return 0;
}

/**
 * Shuts down the Ecore X library.
 *
 * As ecore_x_shutdown, except do not close Display, only connection.
 *
 * @ingroup Ecore_X_Init_Group
 */
EAPI int
ecore_x_disconnect(void)
{
   if (--_ecore_x_init_count != 0)
     return _ecore_x_init_count;
   if (_ecore_x_shutdown()) return _ecore_x_init_count;
   close(ConnectionNumber(_ecore_x_disp));
    // FIXME: may have to clean up x display internal here
// getting segv here? hmmm. odd. disable
//        XFree(_ecore_x_disp);
   _ecore_x_disp = NULL;
   _ecore_x_shutdown2();
   return 0;
}

/**
 * @defgroup Ecore_X_Display_Attr_Group X Display Attributes
 * @ingroup Ecore_X_Group
 *
 * Functions that set and retrieve X display attributes.
 */

/**
 * Retrieves the Ecore_X_Display handle used for the current X connection.
 * @return  The current X display.
 * @ingroup Ecore_X_Display_Attr_Group
 */
EAPI Ecore_X_Display *
ecore_x_display_get(void)
{
   return (Ecore_X_Display *)_ecore_x_disp;
}

/**
 * Retrieves the X display file descriptor.
 * @return  The current X display file descriptor.
 * @ingroup Ecore_X_Display_Attr_Group
 */
EAPI int
ecore_x_fd_get(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   return ConnectionNumber(_ecore_x_disp);
}

/**
 * Retrieves the Ecore_X_Screen handle used for the current X connection.
 * @return  The current default screen.
 * @ingroup Ecore_X_Display_Attr_Group
 */
EAPI Ecore_X_Screen *
ecore_x_default_screen_get(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
   return (Ecore_X_Screen *)DefaultScreenOfDisplay(_ecore_x_disp);
}

/**
 * Retrieves the size of an Ecore_X_Screen.
 * @param screen the handle to the screen to query.
 * @param w where to return the width. May be NULL. Returns 0 on errors.
 * @param h where to return the height. May be NULL. Returns 0 on errors.
 * @ingroup Ecore_X_Display_Attr_Group
 * @see ecore_x_default_screen_get()
 *
 * @since 1.1
 */
EAPI void
ecore_x_screen_size_get(const Ecore_X_Screen *screen,
                        int *w,
                        int *h)
{
   Screen *s = (Screen *)screen;
   LOGFN;
   if (w) *w = 0;
   if (h) *h = 0;
   EINA_SAFETY_ON_NULL_RETURN(screen);
   if (w) *w = s->width;
   if (h) *h = s->height;
}

/**
 * Retrieves the number of screens.
 *
 * @return  The count of the number of screens.
 * @ingroup Ecore_X_Display_Attr_Group
 *
 * @since 1.1
 */
EAPI int
ecore_x_screen_count_get(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   return ScreenCount(_ecore_x_disp);
}

/**
 * Retrieves the index number of the given screen.
 *
 * @param screen The screen for which the index will be retrieved.
 * @return  The index number of the screen.
 * @ingroup Ecore_X_Display_Attr_Group
 *
 * @since 1.1
 */
EAPI int
ecore_x_screen_index_get(const Ecore_X_Screen *screen)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(screen, -1);
   return XScreenNumberOfScreen((Screen *)screen);
}

/**
 * Retrieves the screen based on index number.
 *
 * @param idx The index that will be used to retrieve the screen.
 * @return  The Ecore_X_Screen at this index.
 * @ingroup Ecore_X_Display_Attr_Group
 *
 * @since 1.1
 */
EAPI Ecore_X_Screen *
ecore_x_screen_get(int idx)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
   return XScreenOfDisplay(_ecore_x_disp, idx);
}

/**
 * Sets the timeout for a double and triple clicks to be flagged.
 *
 * This sets the time between clicks before the double_click flag is
 * set in a button down event. If 3 clicks occur within double this
 * time, the triple_click flag is also set.
 *
 * @param   t The time in seconds
 * @ingroup Ecore_X_Display_Attr_Group
 */
EAPI void
ecore_x_double_click_time_set(double t)
{
   if (t < 0.0)
     t = 0.0;

   _ecore_x_double_click_time = t;
}

/**
 * Retrieves the double and triple click flag timeout.
 *
 * See @ref ecore_x_double_click_time_set for more information.
 *
 * @return  The timeout for double clicks in seconds.
 * @ingroup Ecore_X_Display_Attr_Group
 */
EAPI double
ecore_x_double_click_time_get(void)
{
   return _ecore_x_double_click_time;
}

/**
 * @defgroup Ecore_X_Flush_Group X Synchronization Functions
 * @ingroup Ecore_X_Group
 *
 * Functions that ensure that all commands that have been issued by the
 * Ecore X library have been sent to the server.
 */

/**
 * Sends all X commands in the X Display buffer.
 * @ingroup Ecore_X_Flush_Group
 */
EAPI void
ecore_x_flush(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XFlush(_ecore_x_disp);
}

/**
 * Flushes the command buffer and waits until all requests have been
 * processed by the server.
 * @ingroup Ecore_X_Flush_Group
 */
EAPI void
ecore_x_sync(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XSync(_ecore_x_disp, False);
}

/**
 * Kill all clients with subwindows under a given window.
 *
 * You can kill all clients connected to the X server by using
 * @ref ecore_x_window_root_list to get a list of root windows, and
 * then passing each root window to this function.
 *
 * @param root The window whose children will be killed.
 */
EAPI void
ecore_x_killall(Ecore_X_Window root)
{
   unsigned int j;
   Window root_r;
   Window parent_r;
   Window *children_r = NULL;
   unsigned int num_children = 0;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XGrabServer(_ecore_x_disp);
   /* Tranverse window tree starting from root, and drag each
    * before the firing squad */
   while (XQueryTree(_ecore_x_disp, root, &root_r, &parent_r,
                     &children_r, &num_children) && (num_children > 0))
     {
        for (j = 0; j < num_children; ++j)
          {
             XKillClient(_ecore_x_disp, children_r[j]);
          }

        XFree(children_r);
     }
   XUngrabServer(_ecore_x_disp);
   XSync(_ecore_x_disp, False);
}

/**
 * Kill a specific client
 *
 * You can kill a specific client owning window @p win
 *
 * @param win Window of the client to be killed
 */
EAPI void
ecore_x_kill(Ecore_X_Window win)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XKillClient(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * Return the last event time recorded by Ecore_X.
 *
 * This time is typically the timestamp from the last processed X event.
 * It can be used when a timestamp is required for X operations (e.g.,
 * setting input focus, grabbing the pointer/keyboard). Using @c CurrentTime
 * is often acceptable, but using the last known event time can sometimes
 * avoid race conditions.
 *
 * @return The timestamp of the last X event.
 */
EAPI Ecore_X_Time
ecore_x_current_time_get(void)
{
   return _ecore_x_event_last_time;
}

/**
 * Return the screen DPI
 *
 * This is a simplistic call to get DPI. It does not account for differing
 * DPI in the x amd y axes nor does it account for multihead or xinerama and
 * xrander where different parts of the screen may have different DPI etc.
 *
 * @return the general screen DPI (dots/pixels per inch).
 */
EAPI int
ecore_x_dpi_get(void)
{
   Screen *s;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   s = DefaultScreenOfDisplay(_ecore_x_disp);
   if (s->mwidth <= 0)
     return 75;

   return (((s->width * 254) / s->mwidth) + 5) / 10;
}

/**
 * Invoke the standard system beep to alert users
 *
 * @param percent The volume at which the bell rings. Must be in the range
 * [-100,+100]. If percent >= 0, the final volume will be:
 *       base - [(base * percent) / 100] + percent
 * Otherwise, it's calculated as:
 *       base + [(base * percent) / 100]
 * where @c base is the bell's base volume as set by XChangeKeyboardControl(3).
 *
 * @returns @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_bell(int percent)
{
   int ret;

   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = XBell(_ecore_x_disp, percent);
   if (ret == BadValue)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Ecore Fd_Handler callback for the X connection.
 *
 * This function is called by the Ecore main loop when there is data
 * available to be read from the X server's file descriptor. It reads
 * all pending X events, filters them if XIM is active, and dispatches
 * them to registered Ecore_X event handlers.
 *
 * @param data The X Display pointer.
 * @param fd_handler The Ecore_Fd_Handler that triggered this callback.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_x_fd_handler(void *data,
                    Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Display *d;

   d = data;
   while (XPending(d))
     {
        XEvent ev;

        XNextEvent(d, &ev);
#ifdef BUILD_ECORE_IMF_XIM
        /* Filter event for XIM */
        if (XFilterEvent(&ev, ev.xkey.window))
          continue;

#endif /* ifdef BUILD_ECORE_IMF_XIM */
        if ((ev.type >= 0) && (ev.type < _ecore_x_event_handlers_num))
          {
             if (_ecore_x_event_handlers[AnyXEvent])
               _ecore_x_event_handlers[AnyXEvent] (&ev);

             if (_ecore_x_event_handlers[ev.type])
               _ecore_x_event_handlers[ev.type] (&ev);
          }
     }
   return ECORE_CALLBACK_RENEW;
}

/**
 * @internal
 * @brief Ecore Fd_Handler buffer check callback for the X connection.
 *
 * This function is called by the Ecore main loop before polling to check
 * if there's already data in the X connection's buffer. If data is pending,
 * it means an immediate call to _ecore_x_fd_handler is warranted without
 * needing to poll.
 *
 * @param data The X Display pointer.
 * @param fd_handler The Ecore_Fd_Handler.
 * @return ECORE_CALLBACK_RENEW if data is pending, ECORE_CALLBACK_CANCEL otherwise.
 */
static Eina_Bool
_ecore_x_fd_handler_buf(void *data,
                        Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   Display *d;

   d = data;
   if (XPending(d))
     return ECORE_CALLBACK_RENEW;

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @internal
 * @brief Get the X modifier mask for a given KeySym.
 *
 * This function iterates through the X server's modifier map to find
 * which modifier mask (e.g., ShiftMask, ControlMask) corresponds to the
 * provided KeySym (e.g., XK_Shift_L, XK_Control_L).
 *
 * @param mod The XModifierKeymap obtained from XGetModifierMapping.
 * @param sym The KeySym to find the mask for (e.g., XK_Shift_L).
 * @return The X modifier mask (e.g., ShiftMask) if found, otherwise 0.
 */
static int
_ecore_x_key_mask_get(XModifierKeymap *mod, KeySym sym)
{
   KeySym sym2;
   int i, j, mask = 0;
   const int masks[8] =
     {
        ShiftMask, LockMask, ControlMask,
        Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
     };

   for (i = 0; i < (8 * mod->max_keypermod); i++)
     {
        for (j = 0; j < 8; j++)
          {
             sym2 = _ecore_x_XKeycodeToKeysym(_ecore_x_disp,
                                              mod->modifiermap[i], j);
             if (sym2 != 0)
             break;
          }
        if (sym2 == sym) mask = masks[i / mod->max_keypermod];
     }
   return mask;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/**
 * @defgroup Ecore_X_Window_Root_Group Root Window Functions
 * @ingroup Ecore_X_Window_Group
 *
 * Functions specifically dealing with X root windows.
 */
/*****************************************************************************/

/**
 * Get a list of all the root windows on the server.
 * @ingroup Ecore_X_Window_Root_Group
 * @note   The returned array will need to be freed after use.
 * @param  num_ret Pointer to integer to put number of windows returned in.
 * @return An array of all the root windows.  @c NULL is returned if memory
 *         could not be allocated for the list, or if @p num_ret is @c NULL.
 */
EAPI Ecore_X_Window *
ecore_x_window_root_list(int *num_ret)
{
   int num, i;
   Ecore_X_Window *roots;

   if (!num_ret)
     return NULL;

   *num_ret = 0;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
   num = ScreenCount(_ecore_x_disp);
   roots = malloc(num * sizeof(Ecore_X_Window));
   if (!roots)
     return NULL;

   *num_ret = num;
   for (i = 0; i < num; i++)
     roots[i] = RootWindow(_ecore_x_disp, i);
   return roots;
}

/**
 * Get the first root window of the display.
 *
 * This is typically the root window of screen 0.
 *
 * @return The Ecore_X_Window ID of the first root window, or 0 on error.
 * @ingroup Ecore_X_Window_Root_Group
 */
EAPI Ecore_X_Window
ecore_x_window_root_first_get(void)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, 0);
   return RootWindow(_ecore_x_disp, 0);
/*
   int num;
   Ecore_X_Window root, *roots = NULL;

   LOGFN;
   roots = ecore_x_window_root_list(&num);
   if (!(roots)) return 0;

   if (num > 0)
     root = roots[0];
   else
     root = 0;

   free(roots);
   return root;
 */
}

/**
 * @internal
 * @brief X error handler callback for ecore_x_window_manage.
 *
 * This function is temporarily set as the X error handler during
 * ecore_x_window_manage. It checks if a BadAccess error occurred during
 * an XChangeWindowAttributes request, which typically means the client
 * is not a window manager and cannot select for SubstructureRedirectMask.
 * If such an error occurs, it sets _ecore_x_window_manage_succeeded
 * to EINA_FALSE.
 *
 * @param data User data (unused).
 */
static void
_ecore_x_window_manage_error(void *data EINA_UNUSED)
{
   if ((ecore_x_error_request_get() == X_ChangeWindowAttributes) &&
       (ecore_x_error_code_get() == BadAccess))
     _ecore_x_window_manage_succeeded = EINA_FALSE;
}

/**
 * @brief Select input events for a window, typically for window management.
 *
 * This function attempts to select a comprehensive set of events on the given
 * window, suitable for a window manager. This includes events for window
 * geometry changes, substructure notifications (child window creation/destruction),
 * property changes, and input events (mouse and keyboard).
 *
 * It temporarily installs an error handler to detect if selecting for
 * SubstructureRedirectMask fails (which happens if another window manager
 * is already running).
 *
 * @param win The window to manage.
 * @return EINA_TRUE if event selection was successful (or seemed to be),
 *         EINA_FALSE otherwise (e.g., another WM is running, or the window
 *         does not exist).
 * @ingroup Ecore_X_Window_Group
 */
EAPI Eina_Bool
ecore_x_window_manage(Ecore_X_Window win)
{
   XWindowAttributes att;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   if (XGetWindowAttributes(_ecore_x_disp, win, &att) != True)
     return EINA_FALSE;

   ecore_x_sync();
   _ecore_x_window_manage_succeeded = EINA_TRUE;
   ecore_x_error_handler_set(_ecore_x_window_manage_error, NULL);
   XSelectInput(_ecore_x_disp, win,
                EnterWindowMask |
                LeaveWindowMask |
                PropertyChangeMask |
                ResizeRedirectMask |
                SubstructureRedirectMask |
                SubstructureNotifyMask |
                StructureNotifyMask |
                KeyPressMask |
                KeyReleaseMask |
                att.your_event_mask);
   ecore_x_sync();
   ecore_x_error_handler_set(NULL, NULL);
   if (!_ecore_x_window_manage_succeeded)
     {
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Select input events for a container window.
 *
 * This function selects for SubstructureRedirectMask and SubstructureNotifyMask
 * on the given window. This is typically used for windows that will contain
 * other windows and need to intercept or be notified of requests to change
 * their children's geometry or state.
 *
 * @param win The container window.
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_container_manage(Ecore_X_Window win)
{
   LOGFN;
   if (_ecore_x_window_manage_succeeded && (win == ecore_x_window_root_first_get())) return;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XSelectInput(_ecore_x_disp, win,
                SubstructureRedirectMask |
                SubstructureNotifyMask);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Select input events for a client window (managed by a window manager).
 *
 * This function selects a set of events appropriate for a window that is
 * being managed by the current client (acting as a window manager).
 * This includes property changes, focus changes, colormap changes,
 * visibility changes, and structure/substructure notifications. It also
 * selects for ShapeNotifyMask if the Shape extension is available.
 *
 * @param win The client window to manage.
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_client_manage(Ecore_X_Window win)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   if (_ecore_x_window_manage_succeeded && (win == ecore_x_window_root_first_get())) return;
   XSelectInput(_ecore_x_disp, win,
                PropertyChangeMask |
//		ResizeRedirectMask |
                FocusChangeMask |
                ColormapChangeMask |
                VisibilityChangeMask |
                StructureNotifyMask |
                SubstructureNotifyMask
                );
   if (_ecore_xlib_sync) ecore_x_sync();
   XShapeSelectInput(_ecore_x_disp, win, ShapeNotifyMask);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Select input events for "sniffing" a window's properties and substructure.
 *
 * This function selects for PropertyChangeMask and SubstructureNotifyMask.
 * It's a lighter-weight way to monitor a window for property changes and
 * child window events, without attempting full management (which might
 * involve redirecting requests).
 *
 * @param win The window to sniff.
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_sniff(Ecore_X_Window win)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   if (_ecore_x_window_manage_succeeded && (win == ecore_x_window_root_first_get())) return;
   XSelectInput(_ecore_x_disp, win,
                PropertyChangeMask |
                SubstructureNotifyMask);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @internal
 * @brief Selects PropertyChangeMask on the first root window.
 *
 * This function is used to monitor property changes on the root window.
 * It only performs the selection if `_ecore_x_window_manage_succeeded` is false,
 * implying that the current client is not the window manager (as a WM would
 * typically select more events on the root).
 *
 * This is marked as internal-only.
 */
EAPI void
ecore_x_window_root_properties_select(void)
{
   LOGFN;
   if (_ecore_x_window_manage_succeeded) return;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XSelectInput(_ecore_x_disp, ecore_x_window_root_first_get(), PropertyChangeMask);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Select input events for "sniffing" a client window.
 *
 * This function selects a broader set of events than ecore_x_window_sniff(),
 * including focus changes, colormap changes, visibility changes, and
 * structure/substructure notifications, as well as property changes.
 * It also selects for ShapeNotifyMask if the Shape extension is available.
 * This is suitable for monitoring client windows more closely without
 * full management.
 *
 * @param win The client window to sniff.
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_client_sniff(Ecore_X_Window win)
{
   LOGFN;
   if (_ecore_x_window_manage_succeeded && (win == ecore_x_window_root_first_get())) return;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XSelectInput(_ecore_x_disp, win,
                PropertyChangeMask |
                FocusChangeMask |
                ColormapChangeMask |
                VisibilityChangeMask |
                StructureNotifyMask |
                SubstructureNotifyMask);
   if (_ecore_xlib_sync) ecore_x_sync();
   XShapeSelectInput(_ecore_x_disp, win, ShapeNotifyMask);
   if (_ecore_xlib_sync) ecore_x_sync();
}

EAPI Eina_Bool
ecore_x_window_attributes_get(Ecore_X_Window win,
                              Ecore_X_Window_Attributes *att_ret)
{
   XWindowAttributes att;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = XGetWindowAttributes(_ecore_x_disp, win, &att);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!ret) return EINA_FALSE;

   memset(att_ret, 0, sizeof(Ecore_X_Window_Attributes));
   att_ret->root = att.root;
   att_ret->x = att.x;
   att_ret->y = att.y;
   att_ret->w = att.width;
   att_ret->h = att.height;
   att_ret->border = att.border_width;
   att_ret->depth = att.depth;
   if (att.map_state != IsUnmapped)
     att_ret->visible = 1;

   if (att.map_state == IsViewable)
     att_ret->viewable = 1;

   if (att.override_redirect)
     att_ret->override = 1;

   if (att.class == InputOnly)
     att_ret->input_only = 1;

   if (att.save_under)
     att_ret->save_under = 1;

   att_ret->event_mask.mine = att.your_event_mask;
   att_ret->event_mask.all = att.all_event_masks;
   att_ret->event_mask.no_propagate = att.do_not_propagate_mask;
   att_ret->window_gravity = att.win_gravity;
   att_ret->pixel_gravity = att.bit_gravity;
   att_ret->colormap = att.colormap;
   att_ret->visual = att.visual;
   return EINA_TRUE;
}

/**
 * @brief Add a window to the client's save-set.
 *
 * When a client exits, windows in its save-set are reparented to the
 * closest surviving ancestor if they are children of the exiting client's
 * windows. This is primarily used by window managers to prevent client
 * windows from being destroyed when the window manager exits.
 *
 * @param win The window to add to the save-set.
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_save_set_add(Ecore_X_Window win)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XAddToSaveSet(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Remove a window from the client's save-set.
 *
 * @param win The window to remove from the save-set.
 * @see ecore_x_window_save_set_add()
 * @ingroup Ecore_X_Window_Group
 */
EAPI void
ecore_x_window_save_set_del(Ecore_X_Window win)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XRemoveFromSaveSet(_ecore_x_disp, win);
   if (_ecore_xlib_sync) ecore_x_sync();
}

EAPI Ecore_X_Window *
ecore_x_window_children_get(Ecore_X_Window win,
                            int *num)
{
   Ecore_X_Window *windows = NULL;
   Eina_Bool success;
   Window root_ret = 0, parent_ret = 0, *children_ret = NULL;
   unsigned int children_ret_num = 0;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
   success = XQueryTree(_ecore_x_disp, win, &root_ret, &parent_ret, &children_ret,
                   &children_ret_num);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!success) return NULL;

   if (children_ret)
     {
        windows = malloc(children_ret_num * sizeof(Ecore_X_Window));
        if (windows)
          {
             unsigned int i;

             for (i = 0; i < children_ret_num; i++)
               windows[i] = children_ret[i];
             *num = children_ret_num;
          }

        XFree(children_ret);
     }

   return windows;
}

/**
 * @brief Set pointer acceleration and threshold.
 *
 * @param accel_num The numerator for pointer acceleration.
 * @param accel_denom The denominator for pointer acceleration.
 *        Pointer acceleration is accel_num / accel_denom.
 * @param threshold The pointer movement threshold (in pixels) before
 *        acceleration takes effect.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_control_set(int accel_num,
                            int accel_denom,
                            int threshold)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = !!XChangePointerControl(_ecore_x_disp, 1, 1,
                                accel_num, accel_denom, threshold);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Get current pointer acceleration and threshold.
 *
 * @param[out] accel_num Pointer to store the numerator for acceleration.
 * @param[out] accel_denom Pointer to store the denominator for acceleration.
 * @param[out] threshold Pointer to store the threshold.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_control_get(int *accel_num,
                            int *accel_denom,
                            int *threshold)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = !!XGetPointerControl(_ecore_x_disp,
                             accel_num, accel_denom, threshold);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Set the pointer button mapping.
 *
 * The @p map array defines the mapping of physical pointer buttons
 * to logical button numbers. For example, `map[0] = 3, map[1] = 1, map[2] = 2`
 * would map physical button 1 to logical button 3, physical button 2 to
 * logical button 1, and physical button 3 to logical button 2.
 * A value of 0 in the map disables that physical button.
 *
 * @param map An array of @p nmap unsigned chars specifying the mapping.
 *            Example: `unsigned char map[] = {1, 2, 3};` (standard mapping for 3 buttons)
 *                     `unsigned char map[] = {3, 2, 1};` (buttons 1 and 3 swapped)
 * @param nmap The number of entries in the @p map array.
 * @return EINA_TRUE if the mapping was successfully set, EINA_FALSE otherwise.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_mapping_set(unsigned char *map,
                            int nmap)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = (XSetPointerMapping(_ecore_x_disp, map, nmap) == MappingSuccess);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Get the current pointer button mapping.
 *
 * @param[out] map A pre-allocated array of unsigned chars to store the mapping.
 *                 The size of this array should be at least @p nmap.
 * @param nmap The number of mapping entries to retrieve. This should typically
 *             be the value returned by ecore_x_pointer_mapping_count_get().
 * @return EINA_TRUE on success, EINA_FALSE on failure. The @p map array is
 *         filled with the current mapping on success.
 *         Example of returned map structure: `map[0]` is logical button for physical 1, etc.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_mapping_get(unsigned char *map,
                            int nmap)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = !!XGetPointerMapping(_ecore_x_disp, map, nmap);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Grab the pointer.
 *
 * This function actively grabs the pointer, directing all pointer events
 * to the specified window @p win. The grab is asynchronous for both
 * pointer and keyboard events.
 *
 * @param win The window to which pointer events will be reported.
 * @return EINA_TRUE if the grab was successful, EINA_FALSE otherwise.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_grab(Ecore_X_Window win)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = (XGrabPointer(_ecore_x_disp, win, False,
                    ButtonPressMask | ButtonReleaseMask |
                    EnterWindowMask | LeaveWindowMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync,
                    None, None, CurrentTime) == GrabSuccess);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Grab the pointer and confine it to a window.
 *
 * This function actively grabs the pointer, directing all pointer events
 * to the specified window @p win. Additionally, the pointer cursor is
 * confined to the boundaries of @p win. The grab is asynchronous for both
 * pointer and keyboard events.
 *
 * @param win The window to which pointer events will be reported and to
 *            which the cursor will be confined.
 * @return EINA_TRUE if the grab was successful, EINA_FALSE otherwise.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_confine_grab(Ecore_X_Window win)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = (XGrabPointer(_ecore_x_disp, win, False,
                    ButtonPressMask | ButtonReleaseMask |
                    EnterWindowMask | LeaveWindowMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync,
                    win, None, CurrentTime) == GrabSuccess);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Ungrab the pointer.
 *
 * Releases any active pointer grab made by this client.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_pointer_ungrab(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XUngrabPointer(_ecore_x_disp, CurrentTime);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Warp (move) the pointer to a specific location within a window.
 *
 * @param win The destination window. If None, coordinates are relative to the
 *            root window of the screen the pointer is currently on.
 * @param x The target x-coordinate within @p win.
 * @param y The target y-coordinate within @p win.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_pointer_warp(Ecore_X_Window win,
                     int x,
                     int y)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = !!XWarpPointer(_ecore_x_disp, None, win, 0, 0, 0, 0, x, y);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Grab the keyboard.
 *
 * This function actively grabs the keyboard, directing all keyboard events
 * to the specified window @p win. The grab is asynchronous for both
 * pointer and keyboard events.
 *
 * @param win The window to which keyboard events will be reported.
 * @return EINA_TRUE if the grab was successful, EINA_FALSE otherwise.
 * @ingroup Ecore_X_Input_Group
 */
EAPI Eina_Bool
ecore_x_keyboard_grab(Ecore_X_Window win)
{
   Eina_Bool ret;
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = (XGrabKeyboard(_ecore_x_disp, win, False,
                     GrabModeAsync, GrabModeAsync,
                     CurrentTime) == GrabSuccess);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Ungrab the keyboard.
 *
 * Releases any active keyboard grab made by this client.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_keyboard_ungrab(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XUngrabKeyboard(_ecore_x_disp, CurrentTime);
}

/**
 * @brief Grab the X server.
 *
 * This function increments a grab counter and, if the counter becomes 1,
 * issues an XGrabServer request. This prevents any other clients from
 * communicating with the X server until ecore_x_ungrab() is called a
 * corresponding number of times. This is a very heavyweight operation
 * and should be used sparingly and for very short durations.
 * @ingroup Ecore_X_Server_Group
 */
EAPI void
ecore_x_grab(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   _ecore_x_grab_count++;
   if (_ecore_x_grab_count == 1)
     XGrabServer(_ecore_x_disp);
}

/**
 * @brief Ungrab the X server.
 *
 * This function decrements the server grab counter. If the counter reaches 0,
 * an XUngrabServer request is issued, allowing other clients to communicate
 * with the X server again.
 * @see ecore_x_grab()
 * @ingroup Ecore_X_Server_Group
 */
EAPI void
ecore_x_ungrab(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   _ecore_x_grab_count--;
   if (_ecore_x_grab_count < 0)
     _ecore_x_grab_count = 0;

   if (_ecore_x_grab_count == 0)
     XUngrabServer(_ecore_x_disp);
}

/**
 * @brief Function pointer type for replaying events during a passive grab.
 * @param data User-supplied data.
 * @param event_type The type of the event being replayed.
 * @param event Pointer to the event structure.
 * @return EINA_TRUE to allow further processing or re-queueing, EINA_FALSE otherwise.
 */
Eina_Bool (*_ecore_window_grab_replay_func)(void *data,
                                            int event_type,
                                            void *event);
/** @brief User data for the passive grab replay function. */
void *_ecore_window_grab_replay_data;

/**
 * @brief Set the function to be called when replaying events from a passive grab.
 *
 * When a passive grab (e.g., via XGrabButton with GrabModeSync) is triggered,
 * events are queued by the server. This function allows specifying a callback
 * that can inspect or modify these events before they are replayed (e.g., by
 * XAllowEvents).
 *
 * @param func The function to call for replaying events.
 *             Example:
 *             @code
 *             Eina_Bool my_replay_handler(void *data, int type, void *event) {
 *                 XEvent *xev = event;
 *                 if (xev->type == ButtonPress) {
 *                     printf("Replaying button press on window %lx\n", xev->xbutton.window);
 *                 }
 *                 return EINA_TRUE; // Allow event to be replayed
 *             }
 *             ecore_x_passive_grab_replay_func_set(my_replay_handler, NULL);
 *             @endcode
 * @param data Custom data to be passed to the @p func.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_passive_grab_replay_func_set(Eina_Bool (*func)(void *data,
                                                       int event_type,
                                                       void *event),
                                     void *data)
{
   LOGFN;
   _ecore_window_grab_replay_func = func;
   _ecore_window_grab_replay_data = data;
}






//////////////////////////////////////////////////////////////////////////////
/** @internal @brief Number of active window button grabs. */
int _ecore_window_grabs_num = 0;
/**
 * @internal
 * @brief Array storing information about active window button grabs.
 * Each element is a Wingrab struct:
 * @code
 * typedef struct _Wingrab
 * {
 *    Ecore_X_Window     win;        // The window on which the grab is set
 *    int                button;     // The button number (0 for AnyButton)
 *    Ecore_X_Event_Mask event_mask; // Event mask for the grab
 *    int                mod;        // Modifier mask
 *    int                any_mod;    // Boolean, true if AnyModifier is used
 * } Wingrab;
 * @endcode
 */
Wingrab *_ecore_window_grabs = NULL;

/**
 * @internal
 * @brief Internal function to perform an XGrabButton.
 *
 * This function handles the low-level details of calling XGrabButton,
 * including translating Ecore_X modifiers to X modifiers and iterating
 * through all possible lock key combinations (Caps Lock, Num Lock, etc.)
 * to ensure the grab works regardless of their state.
 *
 * @param win The window to grab the button on.
 * @param button The button number (1-5, or 0 for AnyButton).
 * @param event_mask The event mask for the grab (e.g., ButtonPressMask).
 * @param mod An Ecore_X_Modifier mask.
 * @param any_mod If true, @p mod is ignored and AnyModifier is used.
 */
static void
_ecore_x_window_button_grab_internal(Ecore_X_Window win,
                                     int button,
                                     Ecore_X_Event_Mask event_mask,
                                     int mod,
                                     int any_mod)
{
   unsigned int b;
   unsigned int m;
   unsigned int locks[8];
   int i, ev;

   LOGFN;
   b = button;
   if (b == 0)
     b = AnyButton;

   m = _ecore_x_event_modifier(mod);
   if (any_mod)
     m = AnyModifier;

   locks[0] = 0;
   locks[1] = ECORE_X_LOCK_CAPS;
   locks[2] = ECORE_X_LOCK_NUM;
   locks[3] = ECORE_X_LOCK_SCROLL;
   locks[4] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM;
   locks[5] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_SCROLL;
   locks[6] = ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   locks[7] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   ev = event_mask;
   for (i = 0; i < 8; i++)
     XGrabButton(_ecore_x_disp, b, m | locks[i],
                 win, False, ev, GrabModeSync, GrabModeAsync, None, None);
}

/**
 * @brief Passively grab a mouse button on a window.
 *
 * This function sets up a passive grab for a specific mouse button and
 * modifier combination on the given window. When the button is pressed
 * with the specified modifiers, the grab becomes active.
 * The grab is set up with GrabModeSync for the pointer and GrabModeAsync
 * for the keyboard. This means pointer events are queued until
 * XAllowEvents is called, while keyboard events are processed as usual.
 *
 * This function also records the grab internally to allow re-establishing
 * grabs (e.g., after a window manager restarts or grabs are temporarily
 * suspended).
 *
 * @param win The window on which to grab the button.
 * @param button The button number to grab (1-5). Use 0 for AnyButton.
 * @param event_mask The event mask to activate on grab (e.g., ButtonPressMask | ButtonReleaseMask).
 *                   Example: `ECORE_X_EVENT_MASK_MOUSE_DOWN | ECORE_X_EVENT_MASK_MOUSE_UP | ECORE_X_EVENT_MASK_MOUSE_MOVE`
 * @param mod A combination of Ecore_X_Modifier flags (e.g., ECORE_X_MODIFIER_CTRL | ECORE_X_MODIFIER_SHIFT).
 *            See @ref ECORE_X_MODIFIER_SHIFT etc.
 * @param any_mod If non-zero, the @p mod parameter is ignored, and the grab
 *                applies regardless of modifiers (AnyModifier).
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_window_button_grab(Ecore_X_Window win,
                           int button,
                           Ecore_X_Event_Mask event_mask,
                           int mod,
                           int any_mod)
{
   Wingrab *t;

   _ecore_x_window_button_grab_internal(win, button, event_mask, mod, any_mod);
   _ecore_window_grabs_num++;
   t = realloc(_ecore_window_grabs,
               _ecore_window_grabs_num * sizeof(Wingrab));
   if (!t) return;
   _ecore_window_grabs = t;
   _ecore_window_grabs[_ecore_window_grabs_num - 1].win = win;
   _ecore_window_grabs[_ecore_window_grabs_num - 1].button = button;
   _ecore_window_grabs[_ecore_window_grabs_num - 1].event_mask = event_mask;
   _ecore_window_grabs[_ecore_window_grabs_num - 1].mod = mod;
   _ecore_window_grabs[_ecore_window_grabs_num - 1].any_mod = any_mod;
}

/**
 * @internal
 * @brief Sends a "magic" ClientMessage event to Ecore_X's private window.
 *
 * This is used internally to signal changes to grabs (button or key)
 * across different parts of Ecore_X or potentially to other cooperating
 * clients that might be listening on the private window for these specific
 * messages. The message type is 27777.
 *
 * The `val` parameter indicates the type of operation:
 * - 1: Button ungrab
 * - 2: Key ungrab
 *
 * @param val An integer indicating the operation type.
 * @param swin The subject window of the grab/ungrab operation.
 * @param b For button grabs, the button number. For key grabs, the KeySym.
 * @param mod The modifier mask.
 * @param anymod Non-zero if AnyModifier was used.
 */
static void
_ecore_x_sync_magic_send(int val, Ecore_X_Window swin, int b, int mod, int anymod)
{
   XEvent xev = { 0 };

   xev.xclient.type = ClientMessage;
   xev.xclient.serial = 0;
   xev.xclient.send_event = True;
   xev.xclient.display = _ecore_x_disp;
   xev.xclient.window = _ecore_x_private_win;
   xev.xclient.format = 32;
   xev.xclient.message_type = 27777;
   xev.xclient.data.l[0] = 0x7162534;
   xev.xclient.data.l[1] = val | (anymod << 8);
   xev.xclient.data.l[2] = swin;
   xev.xclient.data.l[3] = b;
   xev.xclient.data.l[4] = mod;
   XSendEvent(_ecore_x_disp, _ecore_x_private_win, False, NoEventMask, &xev);
}

/**
 * @internal
 * @brief Removes a window button grab from the internal tracking list.
 *
 * This function searches the `_ecore_window_grabs` array for a grab
 * matching the provided parameters and removes it.
 *
 * @param win The window of the grab to remove.
 * @param button The button number of the grab. If -1, all grabs for @p win are considered.
 * @param mod The modifier mask of the grab.
 * @param any_mod The any_mod flag of the grab.
 * @return 1 if a grab was found and removed, 0 otherwise.
 */
int
_ecore_x_window_grab_remove(Ecore_X_Window win, int button, int mod, int any_mod)
{
   int i, shuffle = 0;
   Wingrab *t;

   if (_ecore_window_grabs_num > 0)
     {
        for (i = 0; i < _ecore_window_grabs_num; i++)
          {
             if (shuffle)
               _ecore_window_grabs[i - 1] = _ecore_window_grabs[i];

             if ((!shuffle) && (_ecore_window_grabs[i].win == win) &&
                 (((button >= 0) && (_ecore_window_grabs[i].mod == mod) &&
                   (_ecore_window_grabs[i].any_mod == any_mod)) ||
                  (button < 0)))
               shuffle = 1;
          }
        if (shuffle)
          {
             _ecore_window_grabs_num--;
             if (_ecore_window_grabs_num <= 0)
               {
                  free(_ecore_window_grabs);
                  _ecore_window_grabs = NULL;
                  return shuffle;
               }
             t = realloc(_ecore_window_grabs,
                         _ecore_window_grabs_num *
                         sizeof(Wingrab));
             if (!t) return shuffle;
             _ecore_window_grabs = t;
          }
     }
   return shuffle;
}

/**
 * @internal
 * @brief Internal function to perform an XUngrabButton.
 *
 * This function handles the low-level details of calling XUngrabButton,
 * including translating Ecore_X modifiers to X modifiers and iterating
 * through all possible lock key combinations to ensure the ungrab
 * applies correctly.
 *
 * @param win The window to ungrab the button on.
 * @param button The button number (1-5, or 0 for AnyButton).
 * @param mod An Ecore_X_Modifier mask.
 * @param any_mod If true, @p mod is ignored and AnyModifier is used.
 */
static void
_ecore_x_window_button_ungrab_internal(Ecore_X_Window win,
                                       int button,
                                       int mod,
                                       int any_mod)
{
   unsigned int b;
   unsigned int m;
   unsigned int locks[8];
   int i;

   LOGFN;
   b = button;
   if (b == 0)
     b = AnyButton;

   m = _ecore_x_event_modifier(mod);
   if (any_mod)
     m = AnyModifier;

   locks[0] = 0;
   locks[1] = ECORE_X_LOCK_CAPS;
   locks[2] = ECORE_X_LOCK_NUM;
   locks[3] = ECORE_X_LOCK_SCROLL;
   locks[4] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM;
   locks[5] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_SCROLL;
   locks[6] = ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   locks[7] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   for (i = 0; i < 8; i++)
     {
        XUngrabButton(_ecore_x_disp, b, m | locks[i], win);
        if (_ecore_xlib_sync) ecore_x_sync();
     }
}

EAPI void
ecore_x_window_button_ungrab(Ecore_X_Window win,
                             int button,
                             int mod,
                             int any_mod)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   _ecore_x_window_button_ungrab_internal(win, button, mod, any_mod);
   _ecore_x_sync_magic_send(1, win, button, mod, any_mod); // Notify about ungrab
// _ecore_x_window_grab_remove(win, button, mod, any_mod); // This is now handled by client message or elsewhere
}

/**
 * @internal
 * @brief Temporarily suspends all active window button grabs.
 *
 * This function iterates through all internally tracked button grabs
 * (in `_ecore_window_grabs`) and ungrabs them using XUngrabButton.
 * This is typically used when, for example, a menu is popped up, and
 * existing application-level grabs need to be temporarily disabled.
 * The grabs can be restored later using _ecore_x_window_grab_resume().
 */
void _ecore_x_window_grab_suspend(void)
{
   int i;

   for (i = 0; i < _ecore_window_grabs_num; i++)
     {
        _ecore_x_window_button_ungrab_internal
        (_ecore_window_grabs[i].win, _ecore_window_grabs[i].button,
         _ecore_window_grabs[i].mod, _ecore_window_grabs[i].any_mod);
     }
}

/**
 * @internal
 * @brief Resumes all previously suspended window button grabs.
 *
 * This function iterates through all internally tracked button grabs
 * (in `_ecore_window_grabs`) and re-establishes them using XGrabButton.
 * This is used to restore grabs that were temporarily disabled by
 * _ecore_x_window_grab_suspend().
 */
void _ecore_x_window_grab_resume(void)
{
   int i;

   for (i = 0; i < _ecore_window_grabs_num; i++)
     {
        _ecore_x_window_button_grab_internal
        (_ecore_window_grabs[i].win, _ecore_window_grabs[i].button,
         _ecore_window_grabs[i].event_mask,
         _ecore_window_grabs[i].mod, _ecore_window_grabs[i].any_mod);
     }
}








//////////////////////////////////////////////////////////////////////////////

/** @internal @brief Number of active key grabs. */
int _ecore_key_grabs_num = 0;
/**
 * @internal
 * @brief Structure to store information about an active key grab.
 */
typedef struct _Keygrab Keygrab;
struct _Keygrab
{
   Window win;      /**< The window on which the key is grabbed. */
   char  *key;      /**< The string representation of the key (e.g., "Control_L", "a", "Keycode-65"). */
   int    mod;      /**< The Ecore_X_Modifier mask. */
   int    any_mod;  /**< Boolean, true if AnyModifier is used. */
};
/**
 * @internal
 * @brief Array storing information about active key grabs.
 * Each element is a Keygrab struct.
 */
Keygrab *_ecore_key_grabs = NULL;

/**
 * @internal
 * @brief Internal function to perform an XGrabKey.
 *
 * This function handles the low-level details of calling XGrabKey.
 * It converts the key string (which can be a KeySym name like "Return"
 * or a "Keycode-XXX" string) to a KeyCode. It also translates Ecore_X
 * modifiers to X modifiers and iterates through all possible lock key
 * combinations to ensure the grab works regardless of their state.
 *
 * @param win The window to grab the key on.
 * @param key The string name of the key (e.g., "space", "Control_L") or
 *            a keycode string (e.g., "Keycode-65").
 * @param mod An Ecore_X_Modifier mask.
 * @param any_mod If true, @p mod is ignored and AnyModifier is used.
 * @return The KeyCode that was grabbed, or 0 on failure (e.g., invalid key name).
 */
static KeyCode
_ecore_x_window_key_grab_internal(Ecore_X_Window win,
                                  const char *key,
                                  int mod,
                                  int any_mod)
{
   KeyCode keycode = 0;
   KeySym keysym;
   unsigned int m;
   unsigned int locks[8];
   int i;

   LOGFN;
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          return 0;

        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     return 0;

   m = _ecore_x_event_modifier(mod);
   if (any_mod)
     m = AnyModifier;

   locks[0] = 0;
   locks[1] = ECORE_X_LOCK_CAPS;
   locks[2] = ECORE_X_LOCK_NUM;
   locks[3] = ECORE_X_LOCK_SCROLL;
   locks[4] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM;
   locks[5] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_SCROLL;
   locks[6] = ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   locks[7] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   for (i = 0; i < 8; i++)
     {
        XGrabKey(_ecore_x_disp, keycode, m | locks[i],
                 win, False, GrabModeAsync, GrabModeAsync);
        if (_ecore_xlib_sync) ecore_x_sync();
     }
   return keycode;
}

/**
 * @brief Passively grab a key on a window.
 *
 * This function sets up a passive grab for a specific key and modifier
 * combination on the given window. When the key is pressed with the
 * specified modifiers, the grab becomes active.
 * The grab is set up with GrabModeAsync for both pointer and keyboard,
 * meaning events are processed as usual once the grab activates.
 *
 * This function also records the grab internally to allow re-establishing
 * grabs (e.g., after a window manager restarts or grabs are temporarily
 * suspended).
 *
 * @param win The window on which to grab the key.
 * @param key The name of the key to grab (e.g., "Control_L", "a", "F1").
 *            It can also be a string like "Keycode-37" for a specific keycode.
 *            Example key names: "Return", "Escape", "Shift_L", "a", "b", "1", "plus".
 * @param mod A combination of Ecore_X_Modifier flags (e.g., ECORE_X_MODIFIER_CTRL | ECORE_X_MODIFIER_SHIFT).
 *            See @ref ECORE_X_MODIFIER_SHIFT etc.
 * @param any_mod If non-zero, the @p mod parameter is ignored, and the grab
 *                applies regardless of modifiers (AnyModifier).
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_window_key_grab(Ecore_X_Window win,
                        const char *key,
                        int mod,
                        int any_mod)
{
   Keygrab *t;
   KeyCode keycode;

   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   if (!(keycode = _ecore_x_window_key_grab_internal(win, key, mod, any_mod)))
      return;
   _ecore_key_grabs_num++;
   t = realloc(_ecore_key_grabs,
               _ecore_key_grabs_num * sizeof(Keygrab));
   if (!t) return;
   _ecore_key_grabs = t;
   _ecore_key_grabs[_ecore_key_grabs_num - 1].win = win;
   _ecore_key_grabs[_ecore_key_grabs_num - 1].key = strdup(key);
   _ecore_key_grabs[_ecore_key_grabs_num - 1].mod = mod;
   _ecore_key_grabs[_ecore_key_grabs_num - 1].any_mod = any_mod;
}

/**
 * @internal
 * @brief Removes a key grab from the internal tracking list.
 *
 * This function searches the `_ecore_key_grabs` array for a grab
 * matching the provided parameters and removes it.
 *
 * @param win The window of the grab to remove.
 * @param key The key string of the grab. If NULL, all grabs for @p win are considered.
 * @param mod The modifier mask of the grab.
 * @param any_mod The any_mod flag of the grab.
 * @return 1 if a grab was found and removed, 0 otherwise.
 */
int
_ecore_x_key_grab_remove(Ecore_X_Window win,
                         const char *key,
                         int mod,
                         int any_mod)
{
   int i, shuffle = 0;
   Keygrab *t;

   if (_ecore_key_grabs_num > 0)
     {
        for (i = 0; i < _ecore_key_grabs_num; i++)
          {
             if (shuffle)
               _ecore_key_grabs[i - 1] = _ecore_key_grabs[i];

             if ((!shuffle) && (_ecore_key_grabs[i].win == win) &&
                 ((key && ((!strcmp(_ecore_key_grabs[i].key, key)) &&
                           (_ecore_key_grabs[i].mod == mod) &&
                           (_ecore_key_grabs[i].any_mod == any_mod))) ||
                  (!key)))
               {
                  free(_ecore_key_grabs[i].key);
                  _ecore_key_grabs[i].key = NULL;
                  shuffle = 1;
               }
          }
        if (shuffle)
          {
             _ecore_key_grabs_num--;
             if (_ecore_key_grabs_num <= 0)
               {
                  free(_ecore_key_grabs);
                  _ecore_key_grabs = NULL;
                  return shuffle;
               }
             t = realloc(_ecore_key_grabs,
                         _ecore_key_grabs_num * sizeof(Keygrab));
             if (!t) return shuffle;
             _ecore_key_grabs = t;
          }
     }
   return shuffle;
}

/**
 * @internal
 * @brief Internal function to perform an XUngrabKey.
 *
 * This function handles the low-level details of calling XUngrabKey.
 * It converts the key string to a KeyCode, translates Ecore_X modifiers
 * to X modifiers, and iterates through all possible lock key combinations
 * to ensure the ungrab applies correctly.
 *
 * @param win The window to ungrab the key on.
 * @param key The string name of the key or keycode string.
 * @param mod An Ecore_X_Modifier mask.
 * @param any_mod If true, @p mod is ignored and AnyModifier is used.
 * @return The KeyCode that was ungrabbed, or 0 on failure.
 */
static KeyCode
_ecore_x_window_key_ungrab_internal(Ecore_X_Window win,
                                    const char *key,
                                    int mod,
                                    int any_mod)
{
   KeyCode keycode = 0;
   KeySym keysym;
   unsigned int m;
   unsigned int locks[8];
   int i;

   LOGFN;
   if (!strncmp(key, "Keycode-", 8))
     keycode = atoi(key + 8);
   else
     {
        keysym = XStringToKeysym(key);
        if (keysym == NoSymbol)
          return 0;

        keycode = XKeysymToKeycode(_ecore_x_disp, keysym);
     }

   if (keycode == 0)
     return 0;

   m = _ecore_x_event_modifier(mod);
   if (any_mod)
     m = AnyModifier;

   locks[0] = 0;
   locks[1] = ECORE_X_LOCK_CAPS;
   locks[2] = ECORE_X_LOCK_NUM;
   locks[3] = ECORE_X_LOCK_SCROLL;
   locks[4] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM;
   locks[5] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_SCROLL;
   locks[6] = ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   locks[7] = ECORE_X_LOCK_CAPS | ECORE_X_LOCK_NUM | ECORE_X_LOCK_SCROLL;
   for (i = 0; i < 8; i++)
     XUngrabKey(_ecore_x_disp, keycode, m | locks[i], win);
   return keycode;
}

EAPI void
ecore_x_window_key_ungrab(Ecore_X_Window win,
                          const char *key,
                          int mod,
                          int any_mod)
{
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   _ecore_x_window_key_ungrab_internal(win, key, mod, any_mod);
   _ecore_x_sync_magic_send(2, win, XStringToKeysym(key), mod, any_mod); // Notify about ungrab
// _ecore_x_key_grab_remove(win, key, mod, any_mod); // This is now handled by client message or elsewhere
}

/**
 * @internal
 * @brief Temporarily suspends all active key grabs.
 *
 * This function iterates through all internally tracked key grabs
 * (in `_ecore_key_grabs`) and ungrabs them using XUngrabKey.
 * This is used to temporarily disable global or application-wide
 * key bindings. The grabs can be restored later using
 * _ecore_x_key_grab_resume().
 */
void
_ecore_x_key_grab_suspend(void)
{
   int i;

   for (i = 0; i < _ecore_key_grabs_num; i++)
     {
        _ecore_x_window_key_ungrab_internal
        (_ecore_key_grabs[i].win, _ecore_key_grabs[i].key,
         _ecore_key_grabs[i].mod, _ecore_key_grabs[i].any_mod);
     }
}

/**
 * @internal
 * @brief Resumes all previously suspended key grabs.
 *
 * This function iterates through all internally tracked key grabs
 * (in `_ecore_key_grabs`) and re-establishes them using XGrabKey.
 * This is used to restore key grabs that were temporarily disabled by
 * _ecore_x_key_grab_suspend().
 */
void
_ecore_x_key_grab_resume(void)
{
   int i;

   for (i = 0; i < _ecore_key_grabs_num; i++)
     {
        _ecore_x_window_key_grab_internal
        (_ecore_key_grabs[i].win, _ecore_key_grabs[i].key,
         _ecore_key_grabs[i].mod, _ecore_key_grabs[i].any_mod);
     }
}








/**
 * Send client message with given type and format 32.
 *
 * @param win     The window the message is sent to.
 * @param type    The client message type.
 * @param mask    The mask of the message to be sent.
 * @param d0      The client message data item 1
 * @param d1      The client message data item 2
 * @param d2      The client message data item 3
 * @param d3      The client message data item 4
 * @param d4      The client message data item 5
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_client_message32_send(Ecore_X_Window win,
                              Ecore_X_Atom type,
                              Ecore_X_Event_Mask mask,
                              long d0,
                              long d1,
                              long d2,
                              long d3,
                              long d4)
{
   XEvent xev = { 0 };
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   xev.xclient.window = win;
   xev.xclient.type = ClientMessage;
   xev.xclient.message_type = type;
   xev.xclient.format = 32;
   xev.xclient.data.l[0] = d0;
   xev.xclient.data.l[1] = d1;
   xev.xclient.data.l[2] = d2;
   xev.xclient.data.l[3] = d3;
   xev.xclient.data.l[4] = d4;

   ret = !!XSendEvent(_ecore_x_disp, win, False, mask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * Send client message with given type and format 8.
 *
 * @param win     The window the message is sent to.
 * @param type    The client message type.
 * @param data    Data to be sent.
 * @param len     Number of data bytes, max @c 20.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_client_message8_send(Ecore_X_Window win,
                             Ecore_X_Atom type,
                             const void *data,
                             int len)
{
   XEvent xev = { 0 };
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   xev.xclient.window = win;
   xev.xclient.type = ClientMessage;
   xev.xclient.message_type = type;
   xev.xclient.format = 8;
   if (len > 20)
     len = 20;

   if (data && len > 0)
     memcpy(xev.xclient.data.b, data, len);
   if (len < 20)
     memset(xev.xclient.data.b + len, 0, 20 - len);

   ret = !!XSendEvent(_ecore_x_disp, win, False, NoEventMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Send a synthetic mouse motion event to a window.
 *
 * This function creates and sends an X MotionNotify event to the specified
 * window. This can be used to simulate mouse movement.
 *
 * @param win The window to send the event to.
 * @param x The x-coordinate of the mouse pointer relative to the window.
 * @param y The y-coordinate of the mouse pointer relative to the window.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Event_Group
 */
EAPI Eina_Bool
ecore_x_mouse_move_send(Ecore_X_Window win,
                        int x,
                        int y)
{
   XEvent xev = { 0 };
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xmotion.type = MotionNotify;
   xev.xmotion.window = win;
   xev.xmotion.root = att.root;
   xev.xmotion.subwindow = win;
   xev.xmotion.time = _ecore_x_event_last_time;
   xev.xmotion.x = x;
   xev.xmotion.y = y;
   xev.xmotion.x_root = rx;
   xev.xmotion.y_root = ry;
   xev.xmotion.state = 0;
   xev.xmotion.is_hint = 0;
   xev.xmotion.same_screen = 1;
   ret = !!XSendEvent(_ecore_x_disp, win, True, PointerMotionMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Send a synthetic mouse button press event to a window.
 *
 * This function creates and sends an X ButtonPress event to the specified
 * window. This can be used to simulate a mouse button click.
 *
 * @param win The window to send the event to.
 * @param x The x-coordinate of the mouse pointer relative to the window.
 * @param y The y-coordinate of the mouse pointer relative to the window.
 * @param b The button number (1 for left, 2 for middle, 3 for right, etc.).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Event_Group
 */
EAPI Eina_Bool
ecore_x_mouse_down_send(Ecore_X_Window win,
                        int x,
                        int y,
                        int b)
{
   XEvent xev = { 0 };
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xbutton.type = ButtonPress;
   xev.xbutton.window = win;
   xev.xbutton.root = att.root;
   xev.xbutton.subwindow = win;
   xev.xbutton.time = _ecore_x_event_last_time;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = rx;
   xev.xbutton.y_root = ry;
   xev.xbutton.state = 1 << b;
   xev.xbutton.button = b;
   xev.xbutton.same_screen = 1;
   ret = !!XSendEvent(_ecore_x_disp, win, True, ButtonPressMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Send a synthetic mouse button release event to a window.
 *
 * This function creates and sends an X ButtonRelease event to the specified
 * window. This can be used to simulate releasing a mouse button.
 *
 * @param win The window to send the event to.
 * @param x The x-coordinate of the mouse pointer relative to the window.
 * @param y The y-coordinate of the mouse pointer relative to the window.
 * @param b The button number (1 for left, 2 for middle, 3 for right, etc.).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Event_Group
 */
EAPI Eina_Bool
ecore_x_mouse_up_send(Ecore_X_Window win,
                      int x,
                      int y,
                      int b)
{
   XEvent xev = { 0 };
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xbutton.type = ButtonRelease;
   xev.xbutton.window = win;
   xev.xbutton.root = att.root;
   xev.xbutton.subwindow = win;
   xev.xbutton.time = _ecore_x_event_last_time;
   xev.xbutton.x = x;
   xev.xbutton.y = y;
   xev.xbutton.x_root = rx;
   xev.xbutton.y_root = ry;
   xev.xbutton.state = 0;
   xev.xbutton.button = b;
   xev.xbutton.same_screen = 1;
   ret = !!XSendEvent(_ecore_x_disp, win, True, ButtonReleaseMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Send a synthetic mouse enter event to a window.
 *
 * This function creates and sends an X EnterNotify event to the specified
 * window. This can be used to simulate the mouse pointer entering a window.
 *
 * @param win The window to send the event to.
 * @param x The x-coordinate of the mouse pointer relative to the window.
 * @param y The y-coordinate of the mouse pointer relative to the window.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Event_Group
 */
EAPI Eina_Bool
ecore_x_mouse_in_send(Ecore_X_Window win,
                      int x,
                      int y)
{
   XEvent xev = { 0 };
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xcrossing.type = EnterNotify;
   xev.xcrossing.window = win;
   xev.xcrossing.root = att.root;
   xev.xcrossing.subwindow = win;
   xev.xcrossing.time = _ecore_x_event_last_time;
   xev.xcrossing.x = x;
   xev.xcrossing.y = y;
   xev.xcrossing.x_root = rx;
   xev.xcrossing.y_root = ry;
   xev.xcrossing.mode = NotifyNormal;
   xev.xcrossing.detail = NotifyNonlinear;
   xev.xcrossing.same_screen = 1;
   xev.xcrossing.focus = 0;
   xev.xcrossing.state = 0;
   ret = !!XSendEvent(_ecore_x_disp, win, True, EnterWindowMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Send a synthetic mouse leave event to a window.
 *
 * This function creates and sends an X LeaveNotify event to the specified
 * window. This can be used to simulate the mouse pointer leaving a window.
 *
 * @param win The window to send the event to.
 * @param x The x-coordinate of the mouse pointer relative to the window.
 * @param y The y-coordinate of the mouse pointer relative to the window.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 * @ingroup Ecore_X_Event_Group
 */
EAPI Eina_Bool
ecore_x_mouse_out_send(Ecore_X_Window win,
                      int x,
                      int y)
{
   XEvent xev = { 0 };
   XWindowAttributes att;
   Window tw;
   int rx, ry;
   Eina_Bool ret;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   XGetWindowAttributes(_ecore_x_disp, win, &att);
   XTranslateCoordinates(_ecore_x_disp, win, att.root, x, y, &rx, &ry, &tw);
   xev.xcrossing.type = LeaveNotify;
   xev.xcrossing.window = win;
   xev.xcrossing.root = att.root;
   xev.xcrossing.subwindow = win;
   xev.xcrossing.time = _ecore_x_event_last_time;
   xev.xcrossing.x = x;
   xev.xcrossing.y = y;
   xev.xcrossing.x_root = rx;
   xev.xcrossing.y_root = ry;
   xev.xcrossing.mode = NotifyNormal;
   xev.xcrossing.detail = NotifyNonlinear;
   xev.xcrossing.same_screen = 1;
   xev.xcrossing.focus = 0;
   xev.xcrossing.state = 0;
   ret = !!XSendEvent(_ecore_x_disp, win, True, LeaveWindowMask, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
   return ret;
}

/**
 * @brief Reset the input focus to PointerRoot.
 *
 * This function sets the X input focus to the root window of the screen
 * where the pointer is currently located (PointerRoot). The focus will
 * revert to this state (RevertToPointerRoot). This is often used to
 * clear the focus from any specific client window.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_focus_reset(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XSetInputFocus(_ecore_x_disp, PointerRoot, RevertToPointerRoot, CurrentTime);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Allow processing of all queued events after a synchronous grab.
 *
 * If a grab was made with GrabModeSync (e.g., via XGrabButton or XGrabKey
 * with such mode), the X server queues further events. This function calls
 * XAllowEvents with AsyncBoth, which releases events from both the keyboard
 * and pointer queues and allows normal event processing to resume.
 * @ingroup Ecore_X_Event_Group
 */
EAPI void
ecore_x_events_allow_all(void)
{
   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XAllowEvents(_ecore_x_disp, AsyncBoth, CurrentTime);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get the last known root coordinates of the pointer.
 *
 * This function retrieves the root X and Y coordinates of the pointer
 * as recorded from the last processed X event that contained pointer
 * position information (e.g., MotionNotify, ButtonPress).
 *
 * @param[out] x Pointer to store the last known root X-coordinate. Can be NULL.
 * @param[out] y Pointer to store the last known root Y-coordinate. Can be NULL.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_pointer_last_xy_get(int *x,
                            int *y)
{
   if (x)
     *x = _ecore_x_event_last_root_x;

   if (y)
     *y = _ecore_x_event_last_root_y;
}

/**
 * @brief Get the current pointer coordinates relative to a window.
 *
 * This function queries the X server for the current position of the mouse
 * pointer. The coordinates returned are relative to the specified window @p win.
 *
 * @param win The window to get pointer coordinates relative to.
 * @param[out] x Pointer to store the X-coordinate. Can be NULL.
 *               Returns -1 if the query fails.
 * @param[out] y Pointer to store the Y-coordinate. Can be NULL.
 *               Returns -1 if the query fails.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_pointer_xy_get(Ecore_X_Window win,
                       int *x,
                       int *y)
{
   Window rwin, cwin;
   int rx, ry, wx, wy, ret;
   unsigned int mask;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   ret = XQueryPointer(_ecore_x_disp, win, &rwin, &cwin,
                       &rx, &ry, &wx, &wy, &mask);
   if (!ret)
     wx = wy = -1;

   if (x) *x = wx;
   if (y) *y = wy;
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Get the current pointer coordinates relative to the root window.
 *
 * This function queries the X server for the current position of the mouse
 * pointer. The coordinates returned are relative to the root window of the
 * screen the pointer is currently on.
 *
 * @param[out] x Pointer to store the root X-coordinate. Can be NULL.
 *               Returns -1 if the query fails.
 * @param[out] y Pointer to store the root Y-coordinate. Can be NULL.
 *               Returns -1 if the query fails.
 * @ingroup Ecore_X_Input_Group
 */
EAPI void
ecore_x_pointer_root_xy_get(int *x, int *y)
{
   Ecore_X_Window *root;
   Window rwin, cwin;
   int rx, ry, wx, wy, ret = 0;
   int i, num;
   unsigned int mask;

   LOGFN;
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   root = ecore_x_window_root_list(&num);
   for (i = 0; i < num; i++)
     {
        ret = XQueryPointer(_ecore_x_disp, root[i], &rwin, &cwin,
                            &rx, &ry, &wx, &wy, &mask);
        if (_ecore_xlib_sync) ecore_x_sync();
        if (ret) break;
     }

   if (!ret)
     rx = ry = -1;

   if (x) *x = rx;
   if (y) *y = ry;
   free(root);
}

/**
 * Retrieve the Visual ID from a given Visual.
 *
 * @param visual  The Visual to get the ID for.
 *
 * @return The visual id.
 * @since 1.1.0
 */
EAPI unsigned int
ecore_x_visual_id_get(Ecore_X_Visual visual)
{
   unsigned int vis;
   vis = XVisualIDFromVisual(visual);
   if (_ecore_xlib_sync) ecore_x_sync();
   return vis;
}

/**
 * Retrieve the default Visual.
 *
 * @param disp  The Display to get the Default Visual from
 * @param screen The Screen.
 *
 * @return The default visual.
 * @since 1.1.0
 */
EAPI Ecore_X_Visual
ecore_x_default_visual_get(Ecore_X_Display *disp,
                           Ecore_X_Screen *screen)
{
   Ecore_X_Visual vis = DefaultVisual(disp, ecore_x_screen_index_get(screen));
   if (_ecore_xlib_sync) ecore_x_sync();
   return vis;
}

/**
 * Retrieve the default Colormap.
 *
 * @param disp  The Display to get the Default Colormap from
 * @param screen The Screen.
 *
 * @return The default colormap.
 * @since 1.1.0
 */
EAPI Ecore_X_Colormap
ecore_x_default_colormap_get(Ecore_X_Display *disp,
                             Ecore_X_Screen *screen)
{
   Ecore_X_Colormap col = DefaultColormap(disp, ecore_x_screen_index_get(screen));
   if (_ecore_xlib_sync) ecore_x_sync();
   return col;
}

/**
 * Retrieve the default depth.
 *
 * @param disp  The Display to get the Default Depth from
 * @param screen The Screen.
 *
 * @return The default depth.
 * @since 1.1.0
 */
EAPI int
ecore_x_default_depth_get(Ecore_X_Display *disp,
                          Ecore_X_Screen *screen)
{
   int depth = DefaultDepth(disp, ecore_x_screen_index_get(screen));
   if (_ecore_xlib_sync) ecore_x_sync();
   return depth;
}

EAPI Ecore_X_Connection *
ecore_x_connection_get(void)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, NULL);
   return XGetXCBConnection(_ecore_x_disp);
}

/**
 * @brief Selects (activates) a specific XKB keyboard group.
 *
 * This function attempts to switch the active keyboard layout group
 * if the XKB extension is available and enabled.
 *
 * @param group The 0-indexed group number to select.
 *              Example: `0` for the primary layout, `1` for the secondary, etc.
 * @ingroup Ecore_X_XKB_Group
 */
EAPI void
ecore_x_xkb_select_group(int group)
{
#ifdef ECORE_XKB
   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);
   XkbLockGroup(_ecore_x_disp, XkbUseCoreKbd, group);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif
}

/**
 * @brief Enables XKB event tracking for keyboard state changes.
 *
 * If the XKB extension is available, this function selects for XKB events
 * related to keyboard state, map, and new keyboard notifications. This allows
 * Ecore_X to receive events like ECORE_X_EVENT_XKB_STATE_NOTIFY and
 * ECORE_X_EVENT_XKB_NEWKBD_NOTIFY.
 *
 * @return EINA_TRUE if XKB event selection was successful, EINA_FALSE otherwise
 *         (or if XKB is not available).
 * @ingroup Ecore_X_XKB_Group
 */
EAPI Eina_Bool
ecore_x_xkb_track_state(void)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XKB
   unsigned mask = XkbNewKeyboardNotifyMask | XkbMapNotifyMask |
     XkbStateNotifyMask | XkbCompatMapNotifyMask;
   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = XkbSelectEvents(_ecore_x_disp, XkbUseCoreKbd, mask, mask);
   if (_ecore_xlib_sync) ecore_x_sync();
#endif
   return ret;
}

/**
 * @brief Retrieves the current XKB keyboard state.
 *
 * If the XKB extension is available, this function queries the server for the
 * current XKB state, including active group, base group, latched group,
 * locked group, and similar information for modifiers.
 *
 * @param[out] state Pointer to an Ecore_X_Xkb_State structure to be filled.
 *                   The structure contains fields like:
 *                   - `group`: Current effective group.
 *                   - `base_group`: Group set by core protocol.
 *                   - `latched_group`: Latched group (temporary).
 *                   - `locked_group`: Locked group (persistent).
 *                   - `mods`: Current effective modifiers.
 *                   - `base_mods`: Modifiers from core protocol.
 *                   - `latched_mods`: Latched modifiers.
 *                   - `locked_mods`: Locked modifiers.
 * @return EINA_TRUE if the state was successfully retrieved, EINA_FALSE
 *         otherwise (or if XKB is not available).
 * @ingroup Ecore_X_XKB_Group
 */
EAPI Eina_Bool
ecore_x_xkb_state_get(Ecore_X_Xkb_State *state)
{
   Eina_Bool ret = EINA_FALSE;
#ifdef ECORE_XKB
   XkbStateRec xkbstate;

   EINA_SAFETY_ON_NULL_RETURN_VAL(_ecore_x_disp, EINA_FALSE);
   ret = XkbGetState(_ecore_x_disp, XkbUseCoreKbd, &xkbstate);
   if (!ret) return ret;

   state->group = xkbstate.group;
   state->base_group = xkbstate.base_group;
   state->latched_group = xkbstate.latched_group;
   state->locked_group = xkbstate.locked_group;

   state->mods = xkbstate.mods;
   state->base_mods = xkbstate.base_mods;
   state->latched_mods = xkbstate.latched_mods;
   state->locked_mods = xkbstate.locked_mods;
#endif
   return ret;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/**
 * @internal
 * @brief Convert Ecore event modifier/lock state to X modifier/lock mask.
 *
 * This function takes an Ecore event state bitmask (which includes both
 * modifiers like ECORE_EVENT_MODIFIER_SHIFT and locks like
 * ECORE_EVENT_LOCK_CAPS) and converts it into an X11 modifier mask
 * (e.g., ShiftMask, LockMask, ControlMask).
 *
 * @param state An unsigned int bitmask representing Ecore modifier and lock states.
 *              Example: `ECORE_EVENT_MODIFIER_CTRL | ECORE_EVENT_LOCK_CAPS`
 * @return The corresponding X11 modifier mask.
 */
static int
_ecore_x_event_modifier(unsigned int state)
{
   int xmodifiers = 0;

   if (state & ECORE_EVENT_MODIFIER_SHIFT)
     xmodifiers |= ECORE_X_MODIFIER_SHIFT;

   if (state & ECORE_EVENT_MODIFIER_CTRL)
     xmodifiers |= ECORE_X_MODIFIER_CTRL;

   if (state & ECORE_EVENT_MODIFIER_ALT)
     xmodifiers |= ECORE_X_MODIFIER_ALT;

   if (state & ECORE_EVENT_MODIFIER_WIN)
     xmodifiers |= ECORE_X_MODIFIER_WIN;

   if (state & ECORE_EVENT_MODIFIER_ALTGR)
     xmodifiers |= ECORE_X_MODIFIER_ALTGR;

   if (state & ECORE_EVENT_LOCK_SCROLL)
     xmodifiers |= ECORE_X_LOCK_SCROLL;

   if (state & ECORE_EVENT_LOCK_NUM)
     xmodifiers |= ECORE_X_LOCK_NUM;

   if (state & ECORE_EVENT_LOCK_CAPS)
     xmodifiers |= ECORE_X_LOCK_CAPS;

   if (state & ECORE_EVENT_LOCK_SHIFT)
     xmodifiers |= ECORE_X_LOCK_SHIFT;

   return xmodifiers;
}
