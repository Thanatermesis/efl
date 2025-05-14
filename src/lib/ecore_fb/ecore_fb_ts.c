#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_TSLIB
# include <tslib.h>
# include <errno.h>
#endif

#include "Ecore_Fb.h"
#include "ecore_fb_private.h"

/**
 * @internal
 * @brief Structure to hold raw touchscreen event data.
 */
typedef struct _Ecore_Fb_Ts_Event Ecore_Fb_Ts_Event;
/**
 * @internal
 * @brief Structure to hold touchscreen calibration data.
 */
typedef struct _Ecore_Fb_Ts_Calibrate Ecore_Fb_Ts_Calibrate;
/**
 * @internal
 * @brief Structure to control touchscreen backlight.
 */
typedef struct _Ecore_Fb_Ts_Backlight Ecore_Fb_Ts_Backlight;
/**
 * @internal
 * @brief Structure to control touchscreen contrast.
 */
typedef struct _Ecore_Fb_Ts_Contrast Ecore_Fb_Ts_Contrast;
/**
 * @internal
 * @brief Structure to control touchscreen LED.
 */
typedef struct _Ecore_Fb_Ts_Led Ecore_Fb_Ts_Led;
/**
 * @internal
 * @brief Structure for flite (front light) control.
 */
typedef struct _Ecore_Fb_Ts_Flite Ecore_Fb_Ts_Flite;

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Event
 * @brief Raw touchscreen event data.
 */
struct _Ecore_Fb_Ts_Event
{
   unsigned short pressure; /**< Pressure value of the touch event. */
   unsigned short x;        /**< X coordinate of the touch event. */
   unsigned short y;        /**< Y coordinate of the touch event. */
   unsigned short _unused;  /**< Unused field. */
};

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Calibrate
 * @brief Touchscreen calibration parameters.
 */
struct _Ecore_Fb_Ts_Calibrate
{
   int xscale; /**< Scaling factor for the X-axis. */
   int xtrans; /**< Translation offset for the X-axis. */
   int yscale; /**< Scaling factor for the Y-axis. */
   int ytrans; /**< Translation offset for the Y-axis. */
   int xyswap; /**< Flag to indicate if X and Y axes should be swapped. */
};

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Backlight
 * @brief Touchscreen backlight control.
 */
struct _Ecore_Fb_Ts_Backlight
{
   int           on;         /**< Backlight state (1 for on, 0 for off). */
   unsigned char brightness; /**< Backlight brightness level. */
};

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Contrast
 * @brief Touchscreen contrast control.
 */
struct _Ecore_Fb_Ts_Contrast
{
   unsigned char contrast; /**< Contrast level. */
};

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Led
 * @brief Touchscreen LED control.
 */
struct _Ecore_Fb_Ts_Led
{
   unsigned char on;         /**< LED state (1 for on, 0 for off). */
   unsigned char blink_time; /**< LED blink time duration. */
   unsigned char on_time;    /**< Duration LED stays on during a blink cycle. */
   unsigned char off_time;   /**< Duration LED stays off during a blink cycle. */
};

/**
 * @internal
 * @struct _Ecore_Fb_Ts_Flite
 * @brief Touchscreen front light (flite) control.
 */
struct _Ecore_Fb_Ts_Flite
{
   unsigned char mode;       /**< Front light mode. */
   unsigned char pwr;        /**< Front light power state. */
   unsigned char brightness; /**< Front light brightness level. */
};

static Eina_Bool _ecore_fb_ts_fd_handler(void *data, Ecore_Fd_Handler *fd_handler);
/**< File descriptor for the touchscreen device. Initialized to -1. */
static int _ecore_fb_ts_fd = -1;
/**< Flag indicating whether to apply calibration data manually. 0 by default. */
static int _ecore_fb_ts_apply_cal = 0;
#ifndef HAVE_TSLIB
/**< Counter for bytes read for a touchscreen event when not using tslib. */
static int _ecore_fb_ts_event_byte_count = 0;
/**< Buffer for a single touchscreen event when not using tslib. */
static Ecore_Fb_Ts_Event _ecore_fb_ts_event;
#endif
/**< Stores the current calibration data. Initialized with default values. */
static Ecore_Fb_Ts_Calibrate _ecore_fb_ts_cal = {1,1,0,0,0};
/**< Handle for the Ecore file descriptor handler. Initialized to NULL. */
static Ecore_Fd_Handler *_ecore_fb_ts_fd_handler_handle = NULL;

#ifdef HAVE_TSLIB
/**< Pointer to the tslib device structure. Initialized to NULL. */
struct tsdev *_ecore_fb_tslib_tsdev = NULL;
/**< Buffer for a single tslib sample event. */
struct ts_sample _ecore_fb_tslib_event;
#endif

/**< Time window in seconds to detect a double click. Default is 0.25s. */
static double _ecore_fb_double_click_time = 0.25;
/**< Window associated with touchscreen events. Initialized to NULL. */
static void *_ecore_fb_ts_event_window = NULL;

/**
 * @brief Initializes the framebuffer touchscreen input system.
 *
 * This function attempts to open and configure the touchscreen device.
 * If TSLIB is available, it will be used. Otherwise, it falls back to
 * reading directly from "/dev/touchscreen/0".
 * An Ecore_Fd_Handler is set up to listen for touchscreen events.
 *
 * @return 1 on success, 0 on failure.
 */
EAPI int
ecore_fb_ts_init(void)
{
#ifdef HAVE_TSLIB
   char *tslib_tsdevice = NULL;
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if ((tslib_tsdevice = getenv("TSLIB_TSDEVICE")) == NULL)
          tslib_tsdevice = "/dev/input/event0";
        printf( "ECORE_FB: TSLIB_TSDEVICE = '%s'\n", tslib_tsdevice );
        _ecore_fb_tslib_tsdev = ts_open( tslib_tsdevice, 1 ); /* 1 = nonblocking, 0 = blocking */

        if ( !_ecore_fb_tslib_tsdev )
          {
             printf( "ECORE_FB: Can't ts_open (%s)\n", strerror( errno ) );
             return 0;
          }

        if ( ts_config( _ecore_fb_tslib_tsdev ) )
          {
             printf( "ECORE_FB: Can't ts_config (%s)\n", strerror( errno ) );
             return 0;
          }
        _ecore_fb_ts_fd = ts_fd( _ecore_fb_tslib_tsdev );
        if ( _ecore_fb_ts_fd < 0 )
          {
             printf( "ECORE_FB: Can't open touchscreen (%s)\n", strerror( errno ) );
             return 0;
          }
     }
#else
   _ecore_fb_ts_fd = open("/dev/touchscreen/0", O_RDONLY);
#endif
   if (_ecore_fb_ts_fd >= 0)
     {
        _ecore_fb_ts_fd_handler_handle = ecore_main_fd_handler_add(_ecore_fb_ts_fd,
                                                                   ECORE_FD_READ,
                                                                   _ecore_fb_ts_fd_handler, NULL,
                                                                   NULL, NULL);
        if (!_ecore_fb_ts_fd_handler_handle)
          {
             close(_ecore_fb_ts_fd);
             return 0;
          }
        // FIXME _ecore_fb_kbd_fd = open("/dev/touchscreen/key", O_RDONLY);
        return 1;
     }
   return 0;
}

/**
 * @brief Shuts down the framebuffer touchscreen input system.
 *
 * This function closes the touchscreen device file descriptor and
 * removes the Ecore_Fd_Handler.
 */
EAPI void
ecore_fb_ts_shutdown(void)
{
   if (_ecore_fb_ts_fd_handler_handle)
      ecore_main_fd_handler_del(_ecore_fb_ts_fd_handler_handle);
   if (_ecore_fb_ts_fd >= 0) close(_ecore_fb_ts_fd);
   _ecore_fb_ts_fd = -1;
   _ecore_fb_ts_fd_handler_handle = NULL;
   _ecore_fb_ts_event_window = NULL;
}

/**
 * @brief Sets the window to be associated with touchscreen events.
 *
 * This function is currently not used to set window properties on
 * the events themselves, but stores the window pointer for later retrieval.
 *
 * @param window A pointer to the window.
 */
EAPI void
ecore_fb_ts_event_window_set(void *window)
{
   _ecore_fb_ts_event_window = window;
}

/**
 * @brief Gets the window associated with touchscreen events.
 *
 * @return A pointer to the window previously set by ecore_fb_ts_event_window_set().
 */
EAPI void *
ecore_fb_ts_event_window_get(void)
{
    return _ecore_fb_ts_event_window;
}

/**
 * @defgroup Ecore_FB_Calibrate_Group Framebuffer Calibration Functions
 * @ingroup Ecore_FB_Group
 *
 * Functions that calibrate the screen.
 */


/**
 * Calibrates the touschreen using the given parameters.
 * @param   xscale X scaling, where 256 = 1.0
 * @param   xtrans X translation.
 * @param   yscale Y scaling.
 * @param   ytrans Y translation.
 * @param   xyswap Swap X & Y flag.
 * @ingroup Ecore_FB_Calibrate_Group
 */
EAPI void
ecore_fb_touch_screen_calibrate_set(int xscale, int xtrans, int yscale, int ytrans, int xyswap)
{
   Ecore_Fb_Ts_Calibrate cal;

   if (_ecore_fb_ts_fd < 0) return;
   cal.xscale = xscale;
   cal.xtrans = xtrans;
   cal.yscale = yscale;
   cal.ytrans = ytrans;
   cal.xyswap = xyswap;
   if (ioctl(_ecore_fb_ts_fd, TS_SET_CAL, (void *)&cal))
     {
        _ecore_fb_ts_cal = cal;
        _ecore_fb_ts_apply_cal = 1;
     }
}

/**
 * Retrieves the calibration parameters of the touchscreen.
 * @param   xscale Pointer to an integer in which to store the X scaling.
 *                 Note that 256 = 1.0.
 * @param   xtrans Pointer to an integer in which to store the X translation.
 * @param   yscale Pointer to an integer in which to store the Y scaling.
 * @param   ytrans Pointer to an integer in which to store the Y translation.
 * @param   xyswap Pointer to an integer in which to store the Swap X & Y flag.
 * @ingroup Ecore_FB_Calibrate_Group
 */
EAPI void
ecore_fb_touch_screen_calibrate_get(int *xscale, int *xtrans, int *yscale, int *ytrans, int *xyswap)
{
   Ecore_Fb_Ts_Calibrate cal;

   if (_ecore_fb_ts_fd < 0) return;
   if (!_ecore_fb_ts_apply_cal)
     {
        if (ioctl(_ecore_fb_ts_fd, TS_GET_CAL, (void *)&cal))
           _ecore_fb_ts_cal = cal;
     }
   else
      cal = _ecore_fb_ts_cal;
   if (xscale) *xscale = cal.xscale;
   if (xtrans) *xtrans = cal.xtrans;
   if (yscale) *yscale = cal.yscale;
   if (ytrans) *ytrans = cal.ytrans;
   if (xyswap) *xyswap = cal.xyswap;
}

/**
 * @internal
 * @brief Callback function for handling touchscreen file descriptor events.
 *
 * This function is called by the Ecore main loop when there is data
 * available to read from the touchscreen file descriptor. It reads the
 * raw touch data, applies calibration if necessary, and generates
 * Ecore_Event_Mouse_Move, Ecore_Event_Mouse_Button_Down, and
 * Ecore_Event_Mouse_Button_Up events. It also handles double and
 * triple click detection.
 *
 * @param data User data associated with the fd_handler (unused).
 * @param fd_handler The Ecore_Fd_Handler that triggered this callback (unused).
 * @return EINA_TRUE to keep the handler active, EINA_FALSE to remove it.
 *         Always returns EINA_TRUE in this implementation.
 */
static Eina_Bool
_ecore_fb_ts_fd_handler(void *data EINA_UNUSED, Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   static int prev_x = 0, prev_y = 0, prev_pressure = 0; /**< Previous touch coordinates and pressure. */
   static double last_time = 0; /**< Timestamp of the last button event. */
   static double last_last_time = 0; /**< Timestamp of the event before the last button event, for triple click. */
   int v = 0; /**< Return value from read() or flag indicating more samples. */

   do
     {
        int x, y, pressure;
        int num;
        double t = 0.0;
        static int did_double = 0;
        static int did_triple = 0;

#ifdef HAVE_TSLIB
        if (_ecore_fb_ts_apply_cal)
           num = ts_read_raw(_ecore_fb_tslib_tsdev, &_ecore_fb_tslib_event, 1);
        else
           num = ts_read(_ecore_fb_tslib_tsdev, &_ecore_fb_tslib_event, 1);
        if (num != 1) return 1; /* no more samples at this time */
        x = _ecore_fb_tslib_event.x;
        y = _ecore_fb_tslib_event.y;
        pressure = _ecore_fb_tslib_event.pressure;
        v = 1; /* loop, there might be more samples */
#else
        char *ptr;
        ptr = (char *)&(_ecore_fb_ts_event);
        ptr += _ecore_fb_ts_event_byte_count;
        num = sizeof(Ecore_Fb_Ts_Event) - _ecore_fb_ts_event_byte_count;
        v = read(_ecore_fb_ts_fd, ptr, num);
        if (v < 0) return 1;
        _ecore_fb_ts_event_byte_count += v;
        if (v < num) return 1;
        _ecore_fb_ts_event_byte_count = 0;
        if (_ecore_fb_ts_apply_cal)
          {
             x = ((_ecore_fb_ts_cal.xscale * _ecore_fb_ts_event.x) >> 8) + _ecore_fb_ts_cal.xtrans;
             y = ((_ecore_fb_ts_cal.yscale * _ecore_fb_ts_event.y) >> 8) + _ecore_fb_ts_cal.ytrans;
          }
        else
          {
             x = _ecore_fb_ts_event.x;
             y = _ecore_fb_ts_event.y;
          }
        pressure = _ecore_fb_ts_event.pressure;
#endif
        t = ecore_loop_time_get();
        /* add event to queue */
        /* always add a move event */
        if ((pressure) || (prev_pressure))
          {
             /* MOVE: mouse is down and was */
             Ecore_Event_Mouse_Move *e;

             e = calloc(1, sizeof(Ecore_Event_Mouse_Move));
             if (!e) goto retry;
             e->x = x;
             e->y = y;
             e->root.x = e->x;
             e->root.y = e->y;
             e->window = 1;
             e->event_window = e->window;
             e->root_window = e->window;
             e->same_screen = 1;
             e->timestamp = ecore_loop_time_get() * 1000.0;
             ecore_event_add(ECORE_EVENT_MOUSE_MOVE, e, NULL, NULL);
          }
        if ((pressure) && (!prev_pressure))
          {
             /* DOWN: mouse is down, but was not now */
             Ecore_Event_Mouse_Button *e;

             e = calloc(1, sizeof(Ecore_Event_Mouse_Button));
             if (!e) goto retry;
             e->x = x;
             e->y = y;
             e->root.x = e->x;
             e->root.y = e->y;
             e->buttons = 1;
             if ((t - last_time) <= _ecore_fb_double_click_time)
               {
                  e->double_click = 1;
                  did_double = 1;
               }
             else
               {
                  did_double = 0;
                  did_triple = 0;
               }
             if ((t - last_last_time) <= (2 * _ecore_fb_double_click_time))
               {
                  did_triple = 1;
                  e->triple_click = 1;
               }
             else
               {
                  did_triple = 0;
               }
             e->window = 1;
             e->event_window = e->window;
             e->root_window = e->window;
             e->same_screen = 1;
             e->timestamp = ecore_loop_time_get() * 1000.0;
             ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, e, NULL, NULL);
          }
        else if ((!pressure) && (prev_pressure))
          {
             /* UP: mouse was down, but is not now */
             Ecore_Event_Mouse_Button *e;

             e = calloc(1, sizeof(Ecore_Event_Mouse_Button));
             if (!e) goto retry;
             e->x = prev_x;
             e->y = prev_y;
             e->root.x = e->x;
             e->root.y = e->y;
             e->buttons = 1;
             if (did_double)
                e->double_click = 1;
             if (did_triple)
                e->triple_click = 1;
             e->window = 1;
             e->event_window = e->window;
             e->root_window = e->window;
             e->same_screen = 1;
             e->timestamp = ecore_loop_time_get() * 1000.0;
             ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, e, NULL, NULL);
          }
        if (did_triple)
          {
             last_time = 0;
             last_last_time = 0;
          }
        else
          {
             last_last_time = last_time;
             last_time = t;
          }
retry:
        prev_x = x;
        prev_y = y;
        prev_pressure = pressure;
     }
   while (v > 0);
   return 1;
}

