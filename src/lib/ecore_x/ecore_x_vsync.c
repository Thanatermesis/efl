#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"

static double _ecore_x_vsync_animator_tick_delay = 0.0;

#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>

#ifdef HAVE_PRCTL
# include <sys/prctl.h>
#endif

#define ECORE_X_VSYNC_DRM 1

static Ecore_X_Window vsync_root = 0;

int _ecore_x_image_shm_check(void);

static int _vsync_log_dom = -1;

#undef ERR
#define ERR(...) EINA_LOG_DOM_ERR(_vsync_log_dom, __VA_ARGS__)

#undef DBG
#define DBG(...) EINA_LOG_DOM_DBG(_vsync_log_dom, __VA_ARGS__)

#undef INF
#define INF(...) EINA_LOG_DOM_INFO(_vsync_log_dom, __VA_ARGS__)

#undef WRN
#define WRN(...) EINA_LOG_DOM_WARN(_vsync_log_dom, __VA_ARGS__)

#undef CRI
#define CRI(...) EINA_LOG_DOM_CRIT(_vsync_log_dom, __VA_ARGS__)



#ifdef ECORE_X_VSYNC_DRM
// relevant header bits of dri/drm inlined here to avoid needing external
// headers to build
/// drm
/**
 * @brief Specifies the type of vblank request or reply.
 *
 * These flags control the behavior of DRM vblank operations, such as
 * whether the sequence number is absolute or relative, and whether
 * an event should be generated.
 */
typedef enum
{
   DRM_VBLANK_ABSOLUTE = 0x00000000, /**< Request an absolute sequence number. */
   DRM_VBLANK_RELATIVE = 0x00000001, /**< Request a relative sequence number. */
   DRM_VBLANK_EVENT = 0x04000000,    /**< Request an event to be generated. */
   DRM_VBLANK_FLIP = 0x08000000,     /**< Request a page flip. */
   DRM_VBLANK_NEXTONMISS = 0x10000000,/**< If missed, wait for the next vblank. */
   DRM_VBLANK_SECONDARY = 0x20000000,/**< Use secondary CRTC. */
   DRM_VBLANK_SIGNAL = 0x40000000   /**< Send a signal when vblank occurs. */
}
drmVBlankSeqType;

/**
 * @brief DRM VBlank Request structure.
 *
 * This structure is used to request a vblank wait.
 */
typedef struct _drmVBlankReq
{
   drmVBlankSeqType type;      /**< Type of vblank request (absolute, relative, event). */
   unsigned int     sequence;  /**< Vblank sequence number. */
   unsigned long    signal;    /**< Signal to be sent (if DRM_VBLANK_SIGNAL is set). */
} drmVBlankReq;

/**
 * @brief DRM VBlank Reply structure.
 *
 * This structure is returned after a vblank wait, containing timing information.
 */
typedef struct _drmVBlankReply
{
   drmVBlankSeqType type;      /**< Type of vblank reply. */
   unsigned int     sequence;  /**< Vblank sequence number. */
   long             tval_sec;  /**< Seconds part of the timestamp. */
   long             tval_usec; /**< Microseconds part of the timestamp. */
} drmVBlankReply;

/**
 * @brief Union for DRM VBlank request and reply.
 *
 * This union allows handling both vblank requests and replies
 * using the same memory space.
 */
typedef union _drmVBlank
{
   drmVBlankReq   request; /**< VBlank request data. */
   drmVBlankReply reply;   /**< VBlank reply data. */
} drmVBlank;

#define DRM_EVENT_CONTEXT_VERSION 2

/**
 * @brief DRM Event Context structure.
 *
 * This structure holds callbacks for handling DRM events like vblank
 * and page flips.
 */
typedef struct _drmEventContext
{
   int version; /**< Version of the event context structure (should be DRM_EVENT_CONTEXT_VERSION). */
   /**
    * @brief Callback for vblank events.
    * @param fd File descriptor for the DRM device.
    * @param sequence The vblank sequence number.
    * @param tv_sec Seconds part of the event timestamp.
    * @param tv_usec Microseconds part of the event timestamp.
    * @param user_data User-provided data.
    */
   void (*vblank_handler)(int fd,
                          unsigned int sequence,
                          unsigned int tv_sec,
                          unsigned int tv_usec,
                          void *user_data);
   /**
    * @brief Callback for page flip events.
    * @param fd File descriptor for the DRM device.
    * @param sequence The page flip sequence number.
    * @param tv_sec Seconds part of the event timestamp.
    * @param tv_usec Microseconds part of the event timestamp.
    * @param user_data User-provided data.
    */
   void (*page_flip_handler)(int fd,
                             unsigned int sequence,
                             unsigned int tv_sec,
                             unsigned int tv_usec,
                             void *user_data);
} drmEventContext;

/**
 * @brief DRM Version structure (for older, ABI-broken versions).
 *
 * This structure represents DRM version information. It is specifically
 * defined to handle cases where the system's drm.h might have an ABI
 * incompatible version of this struct.
 */
typedef struct _drmVersionBroken
{
   int version_major; /**< Major version number. */
   int version_minor; /**< Minor version number. */
//   int version_patchlevel; /**< Patch level (commented out as per original). */
   size_t name_len;   /**< Length of the driver name string. */
   // WARNING! this does NOT match the system drm.h headers because
   // literally drm.h is wrong. the below is correct. drm hapily
   // broke its ABI at some point.
   char *name;         /**< Driver name. */
   size_t date_len;   /**< Length of the driver date string. */
   char *date;         /**< Driver date string. */
   size_t desc_len;   /**< Length of the driver description string. */
   char *desc;         /**< Driver description string. */
} drmVersionBroken;

/**
 * @brief DRM Version structure (intended to be correct).
 *
 * This structure represents DRM version information, aiming for
 * correctness despite potential ABI issues in system headers.
 */
typedef struct _drmVersion
{
   int version_major;      /**< Major version number. */
   int version_minor;      /**< Minor version number. */
   int version_patchlevel; /**< Patch level. */
   size_t name_len;        /**< Length of the driver name string. */
   // WARNING! this does NOT match the system drm.h headers because
   // literally drm.h is wrong. the below is correct. drm hapily
   // broke its ABI at some point.
   char *name;              /**< Driver name. */
   size_t date_len;        /**< Length of the driver date string. */
   char *date;              /**< Driver date string. */
   size_t desc_len;        /**< Length of the driver description string. */
   char *desc;              /**< Driver description string. */
} drmVersion;

// Pointers to dynamically loaded DRM functions
static int (*sym_drmClose)(int fd) = NULL;                     /**< Pointer to drmClose function. */
static int (*sym_drmWaitVBlank)(int fd, drmVBlank *vbl) = NULL; /**< Pointer to drmWaitVBlank function. */
static int (*sym_drmHandleEvent)(int fd, drmEventContext *evctx) = NULL; /**< Pointer to drmHandleEvent function. */
static void *(*sym_drmGetVersion)(int fd) = NULL;               /**< Pointer to drmGetVersion function. */
static void (*sym_drmFreeVersion)(void *drmver) = NULL;         /**< Pointer to drmFreeVersion function. */

static int drm_fd = -1; /**< File descriptor for the DRM device. */
static volatile int drm_event_is_busy = 0; /**< Flag indicating if DRM event handling is active. */
static int drm_animators_interval = 1;    /**< Interval for DRM animator ticks. */
static drmEventContext drm_evctx;         /**< DRM event context for handling vblank events. */
static double _drm_fail_time = 0.1;       /**< Timeout for DRM failure detection (initial). */
static double _drm_fail_time2 = 1.0 / 60.0; /**< Timeout for DRM failure detection (after multiple fails). */
static int _drm_fail_count = 0;           /**< Counter for consecutive DRM failures. */

static void *drm_lib = NULL; /**< Handle for the dynamically loaded DRM library. */

// Threading and synchronization for vsync
static Eina_Thread_Queue *thq = NULL;        /**< Thread queue for communication with the DRM thread. */
static Ecore_Thread *drm_thread = NULL;      /**< Ecore thread for handling DRM events. */
static Eina_Spinlock tick_queue_lock;      /**< Spinlock for protecting tick_queue_count. */
static int           tick_queue_count = 0; /**< Number of ticks currently queued. */
static Eina_Bool     tick_skip = EINA_FALSE; /**< Flag to indicate if animator ticks should be skipped. */
static Eina_Bool     threaded_vsync = EINA_TRUE; /**< Flag to enable/disable threaded vsync. */
static Ecore_Timer  *fail_timer = NULL;      /**< Timer to handle DRM operation failures. */
static Ecore_Timer  *fallback_timer = NULL;  /**< Timer for fallback mechanism when DRM fails repeatedly. */

/**
 * @brief Message structure for the thread queue.
 *
 * Used to send simple commands or data to the DRM handling thread.
 */
typedef struct
{
   Eina_Thread_Queue_Msg head; /**< Standard thread queue message header. */
   char val;                   /**< Value of the message (e.g., 0 for stop, 1 for start). */
} Msg;

#if 0
# define D(args...) fprintf(stderr, ##args)
#else
# define D(args...)
#endif

/**
 * @brief Sends a timestamp to the main loop for animator ticking.
 *
 * This function is called when a vsync event occurs (or is simulated).
 * If threaded vsync is enabled, it queues the timestamp to be processed
 * by the main thread. Otherwise, it directly sets the loop time and
 * triggers animator ticks.
 *
 * @param t The timestamp to send, typically from ecore_time_get() or vblank.
 */
static void _drm_send_time(double t);

/**
 * @brief Timer callback for fallback mechanism when DRM fails repeatedly.
 *
 * If DRM operations fail consistently, this timer provides a fallback
 * by sending a synthetic tick at a regular interval (e.g., 60Hz)
 * if `drm_event_is_busy` is still true.
 *
 * @param data User data (unused).
 * @return EINA_TRUE to reschedule the timer, EINA_FALSE otherwise.
 */
static Eina_Bool
_fallback_timeout(void *data EINA_UNUSED)
{
   if (drm_event_is_busy)
     {
        _drm_send_time(ecore_time_get());
        return EINA_TRUE;
     }
   fallback_timer = NULL;
   return EINA_FALSE;
}

/**
 * @brief Timer callback to handle DRM operation failures.
 *
 * This timer is started when a DRM operation is expected but doesn't complete
 * in time. It increments a failure counter. If failures persist, it may
 * trigger a fallback mechanism. If `drm_event_is_busy` is true, it sends
 * the current time as a tick.
 *
 * @param data User data (unused).
 * @return EINA_FALSE, as this timer is typically a one-shot or reset.
 */
static Eina_Bool
_fail_timeout(void *data EINA_UNUSED)
{
   fail_timer = NULL;
   _drm_fail_count++;
   if (_drm_fail_count >= 10)
     {
        _drm_fail_count = 10;
        if (!fallback_timer)
          fallback_timer = ecore_timer_add
            (1.0 / 60.0, _fallback_timeout, NULL);
     }
   if (drm_event_is_busy)
     _drm_send_time(ecore_time_get());
   return EINA_FALSE;
}

/**
 * @brief Schedules a DRM vblank wait operation.
 *
 * Requests the DRM to notify on the next vblank event.
 * The request is for a relative vblank event.
 *
 * @return EINA_TRUE if scheduling was successful, EINA_FALSE otherwise.
 */
static Eina_Bool
_drm_tick_schedule(void)
{
   drmVBlank vbl;

   DBG("sched...");
   vbl.request.type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT;
   vbl.request.sequence = drm_animators_interval;
   vbl.request.signal = 0;
   if (sym_drmWaitVBlank(drm_fd, &vbl) < 0) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Sends a message to the DRM handling thread via a thread queue.
 *
 * This is used to communicate state changes (like starting/stopping
 * event processing) to the dedicated DRM thread.
 *
 * @param val The character value to send in the message.
 *            Typically 1 to start/continue, 0 to stop.
 */
static void
_tick_send(char val)
{
   Msg *msg;
   void *ref;
   DBG("_tick_send(%i)", val);
   msg = eina_thread_queue_send(thq, sizeof(Msg), &ref);
   msg->val = val;
   eina_thread_queue_send_done(thq, ref);
}

/**
 * @brief Callback function invoked when the custom animator source begins ticking.
 *
 * This function initializes or signals the DRM vsync mechanism to start
 * generating ticks. If using threaded vsync, it sends a message to the
 * DRM thread. Otherwise, it sets up timers and schedules the first DRM tick.
 *
 * @param data User data (unused).
 */
static void
_drm_tick_begin(void *data EINA_UNUSED)
{
   _drm_fail_count = 0;
   drm_event_is_busy = 1;
   if (threaded_vsync)
     {
        _tick_send(1);
     }
   else
     {
        if (fail_timer) ecore_timer_reset(fail_timer);
        else fail_timer = ecore_timer_add(1.0 / 15.0, _fail_timeout, NULL);
        if (_drm_fail_count < 10)
          {
             if (!_drm_tick_schedule())
               {
                  _drm_fail_count = 999999;
                  if (!fallback_timer)
                    fallback_timer = ecore_timer_add
                      (1.0 / 60.0, _fallback_timeout, NULL);
               }
          }
        else
          {
             if (!_drm_tick_schedule())
               _drm_fail_count = 999999;
             if (!fallback_timer)
               fallback_timer = ecore_timer_add
                 (1.0 / 60.0, _fallback_timeout, NULL);
          }
     }
}

/**
 * @brief Callback function invoked when the custom animator source stops ticking.
 *
 * This function signals the DRM vsync mechanism to stop generating ticks.
 * If using threaded vsync, it sends a message to the DRM thread.
 * Otherwise, it cleans up any active timers.
 *
 * @param data User data (unused).
 */
static void
_drm_tick_end(void *data EINA_UNUSED)
{
   _drm_fail_count = 0;
   drm_event_is_busy = 0;
   if (threaded_vsync)
     {
        _tick_send(0);
     }
   else
     {
        if (fail_timer)
          {
             ecore_timer_del(fail_timer);
             fail_timer = NULL;
          }
        if (fallback_timer)
          {
             ecore_timer_del(fallback_timer);
             fallback_timer = NULL;
          }
     }
}

static void
_drm_send_time(double t)
{
   if (threaded_vsync)
     {
        static double t_last = 0.0;
        double *tim = malloc(sizeof(*tim));

        // you won't believe this
        if (t <= t_last)
          {
             fprintf(stderr, "EEEEEEK! time went backwards! %1.5f -> %1.5f\n", t_last, t);
             t = ecore_time_get();
             if (t <= t_last) t = t_last + 0.001;
          }
        if (tim)
          {
             *tim = t;
             DBG("   ... send %1.8f", t);
             // if we are the wm/compositor we need to offset out vsync by 1/2
             // a frame ... we should never offset by more than
             // frame_time - render_time though ... but we don't know what
             // this is and this varies... so for now this will do.a
             if (_ecore_x_vsync_animator_tick_delay > 0.0)
               {
                  static double t_delta_hist[10] = { 0.0 };
                  double t_delta = t - t_last;
                  double t_delta_min = 0.0;
                  double t_sleep = 0.0;

                  // if time delta is sane like 1/20th of a sec or less..
                  if (t_delta < (1.0 / 20.0))
                    {
                       int i;

                       for (i = 0; i < 9; i++)
                         t_delta_hist[i] = t_delta_hist[i + 1];
                       t_delta_hist[9] = t_delta;
                       t_delta_min = t_delta_hist[0];
                       for (i = 1; i < 10; i++)
                         {
                            if (t_delta_hist[i] < t_delta_min)
                              t_delta_min = t_delta_hist[i];
                         }
                       t_sleep = t_delta_min * _ecore_x_vsync_animator_tick_delay;
                       // if w'ere sleeping too long - don't sleep at all.
                       if (t_sleep > (1.0 / 20.0)) t_sleep = 0.0;
                    }
                  if (t_sleep > 0.0) usleep(t_sleep * 1000000.0);
               }
             D("VSYNC:    @%1.5f   ... send %1.8f\n", ecore_time_get(), t);
             eina_spinlock_take(&tick_queue_lock);
             tick_queue_count++;
             eina_spinlock_release(&tick_queue_lock);
             ecore_thread_feedback(drm_thread, tim);
          }
        t_last = t;
     }
   else
     {
        if (drm_event_is_busy)
          {
             if (_drm_fail_count == 0)
               {
                  if (fallback_timer)
                    {
                       ecore_timer_del(fallback_timer);
                       fallback_timer = NULL;
                    }
               }
             ecore_loop_time_set(t);
             ecore_animator_custom_tick();
             if (drm_event_is_busy)
               {
                  if (fail_timer) ecore_timer_reset(fail_timer);
                  else fail_timer = ecore_timer_add(1.0 / 15.0, _fail_timeout, NULL);
                  if (!_drm_tick_schedule())
                    _drm_fail_count = 999999;
               }
          }
     }
}

/**
 * @brief DRM vblank event handler callback.
 *
 * This function is called by the DRM system when a vblank event occurs.
 * It processes the event, potentially adjusting the timestamp, and then
 * calls _drm_send_time() to propagate the tick to the Ecore animator system.
 *
 * @param fd The DRM file descriptor (unused in this function body).
 * @param frame The vblank frame counter.
 * @param sec Seconds part of the vblank timestamp.
 * @param usec Microseconds part of the vblank timestamp.
 * @param data User data (unused).
 */
static void
_drm_vblank_handler(int fd EINA_UNUSED,
                    unsigned int frame,
                    unsigned int sec,
                    unsigned int usec,
                    void *data EINA_UNUSED)
{
   if (drm_event_is_busy)
     {
        static unsigned int pframe = 0;

        DBG("vblank %i", frame);
        D("VSYNC:    @%1.5f vblank %i\n", ecore_time_get(), frame);
        if (pframe != frame)
          {
#if 0 // disable timestamp from vblank and use time event arrived
             double t = (double)sec + ((double)usec / 1000000);
             unsigned long long tusec, ptusec, tdelt = 0;
             static unsigned int psec = 0, pusec = 0;

             tusec = ((unsigned long long)sec) * 1000000 + usec;
             ptusec = ((unsigned long long)psec) * 1000000 + pusec;
             if (tusec <= ptusec)
               {
                  fprintf(stderr,
                          "EEEEEEK! drm time went backwards! %u.%06u -> %u.%06u\n",
                          psec, pusec, sec, usec);
               }
             else
               {
                  if (frame > pframe)
                    {
                       tdelt = (tusec - ptusec) / (frame - pframe);
                       // go back in time 1/8th of a frame to account for
                       // vlnbak gap - this should be enough for now.
                       // probably need to be a bit more accurate.
                       // 
                       // why do this? because the timestamp is the time
                       // the top-left pixel is first displayed which is
                       // after the vlbank gap time
                       t -= (double)(tdelt / 8) / 1000000.0;
                    }
               }
             _drm_fail_count = 0;
             pusec = usec;
             psec = sec;
#else
             double t = ecore_time_get();
             _drm_send_time(t);
             sec = 0;
             usec = 0;
#endif
             pframe = frame;
          }
     }
   else
     {
        D("VSYNC:    @%1.5f vblank drm event when not busy!\n", ecore_time_get());
     }
}

static double _ecore_x_vsync_wakeup_time = 0.0; /**< Stores the time when the vsync mechanism last woke up. */

/**
 * @brief Gets the last time the vsync mechanism woke up.
 *
 * This is primarily used for debugging or performance analysis to understand
 * the timing of vsync events.
 *
 * @return The timestamp of the last vsync wakeup.
 */
EAPI double _ecore_x_vsync_wakeup_time_get(void)
{
   return _ecore_x_vsync_wakeup_time;
}

/**
 * @brief Core function for the dedicated DRM event handling thread.
 *
 * This thread waits for messages on a queue (to start/stop) and for
 * DRM events. When a DRM event (vblank) occurs, or a timeout happens,
 * it notifies the main thread with a timestamp.
 *
 * @param data User data (unused).
 * @param thread The Ecore_Thread context for this thread.
 */
static void
_drm_tick_core(void *data EINA_UNUSED, Ecore_Thread *thread)
{
   Msg *msg;
   void *ref;
   int tick = 0;

   eina_thread_name_set(eina_thread_self(), "Eanimator-vsync");
#ifdef HAVE_PRCTL
   prctl(PR_SET_TIMERSLACK, 1, 0, 0, 0);
#endif
   while (!ecore_thread_check(thread))
     {
        DBG("------- drm_event_is_busy=%i", drm_event_is_busy);
        D("VSYNC:    @%1.5f ------- drm_event_is_busy=%i\n", ecore_time_get(), drm_event_is_busy);
        if (!drm_event_is_busy)
          {
             DBG("wait...");
             D("VSYNC:    @%1.5f wait...\n", ecore_time_get());
             msg = eina_thread_queue_wait(thq, &ref);
             if (msg)
               {
                  tick = msg->val;
                  eina_thread_queue_wait_done(thq, ref);
               }
          }
        else
          {
             do
               {
                  DBG("poll...");
                  D("VSYNC:    @%1.5f poll...\n", ecore_time_get());
                  msg = eina_thread_queue_poll(thq, &ref);
                  if (msg)
                    {
                       tick = msg->val;
                       eina_thread_queue_wait_done(thq, ref);
                    }
               }
             while (msg);
          }
        DBG("tick = %i", tick);
        D("VSYNC:    @%1.5f tick = %i\n", ecore_time_get(), tick);
        if (tick == -1)
          {
             drm_thread = NULL;
             eina_thread_queue_free(thq);
             thq = NULL;
             return;
          }
        else if (tick)
          {
             fd_set rfds, wfds, exfds;
             int max_fd;
             int ret;
             struct timeval tv;

             if (!_drm_tick_schedule())
               {
                  D("VSYNC:    @%1.5f schedule fail\n", ecore_time_get());
                  _drm_fail_count = 999999;
               }
             max_fd = 0;
             FD_ZERO(&rfds);
             FD_ZERO(&wfds);
             FD_ZERO(&exfds);
             FD_SET(drm_fd, &rfds);
             max_fd = drm_fd;
             tv.tv_sec = 0;
             if (_drm_fail_count >= 10)
               tv.tv_usec = _drm_fail_time2 * 1000000;
             else
               tv.tv_usec = _drm_fail_time * 1000000;
             D("VSYNC:    @%1.5f wait %ims\n", ecore_time_get(), (int)(tv.tv_usec /1000));
             ret = select(max_fd + 1, &rfds, &wfds, &exfds, &tv);
             _ecore_x_vsync_wakeup_time = ecore_time_get();
#if 0
             static double pt = 0.0;
             double t = ecore_time_get();
             double f = 1.0 / (t - pt);
             char buf[1024];
             int i, fps;
             fps = 30 + ((f - 60.0) * 10.0);
             if (fps > 1000) fps = 1000;
             for (i = 0; i < fps; i++) buf[i] = '#';
             buf[i] = 0;
             printf("WAKE %1.5f [%s>\n", 1.0 / (t - pt), buf);
             pt = t;
#endif
             if ((ret == 1) && (FD_ISSET(drm_fd, &rfds)))
               {
                  D("VSYNC:    @%1.5f have event\n", ecore_time_get());
                  sym_drmHandleEvent(drm_fd, &drm_evctx);
                  _drm_fail_count = 0;
               }
             else if (ret == 0)
               {
                  // timeout
                  _drm_send_time(ecore_time_get());
                  _drm_fail_count++;
                  D("VSYNC:    @%1.5f fail count %i\n", ecore_time_get(), _drm_fail_count);
               }
          }
     }
}

/**
 * @brief Notification callback executed in the main thread when the DRM thread sends data.
 *
 * This function receives timestamps from the DRM thread. It updates the
 * Ecore loop time and triggers a custom animator tick if vsync events are
 * active and not being skipped.
 *
 * @param data User data (unused).
 * @param thread The Ecore_Thread that sent the notification (unused).
 * @param msg The message data, which is a pointer to a double (timestamp).
 */
static void
_drm_tick_notify(void *data EINA_UNUSED, Ecore_Thread *thread EINA_UNUSED, void *msg)
{
   int tick_queued;

   eina_spinlock_take(&tick_queue_lock);
   tick_queued = tick_queue_count;
   tick_queue_count--;
   eina_spinlock_release(&tick_queue_lock);
   DBG("notify.... %3.3f %i", *((double *)msg), drm_event_is_busy);
   D("VSYNC: notify.... %3.3f %i\n", *((double *)msg), drm_event_is_busy);
   if (drm_event_is_busy)
     {
        double *t = msg, rt, lt;
        static double pt = 0.0, prt = 0.0, plt = 0.0;

        rt = ecore_time_get();
        lt = ecore_loop_time_get();
        DBG("VSYNC %1.8f = delt %1.8f | real = %1.8f | loop = %1.8f", *t, *t - pt, rt - prt, lt - plt);
        D("VSYNC: %1.8f = delt %1.8f | real = %1.8f | loop = %1.8f", *t, *t - pt, rt - prt, lt - plt);
        if ((!tick_skip) || (tick_queued == 1))
          {
             ecore_loop_time_set(*t);
             ecore_animator_custom_tick();
          }
        pt = *t;
        prt = rt;
        plt = lt;
     }
   free(msg);
}

/**
 * @brief Ecore file descriptor handler for DRM events (non-threaded mode).
 *
 * This function is called when there is activity on the DRM file descriptor,
 * indicating a DRM event (like vblank) is ready to be processed.
 * It calls sym_drmHandleEvent to process the event.
 *
 * @param data User data (unused).
 * @param fd_handler The Ecore_Fd_Handler that triggered this callback (unused).
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_ecore_vsync_fd_handler(void *data EINA_UNUSED,
                        Ecore_Fd_Handler *fd_handler EINA_UNUSED)
{
   _ecore_x_vsync_wakeup_time = ecore_time_get();
   sym_drmHandleEvent(drm_fd, &drm_evctx);
   return ECORE_CALLBACK_RENEW;
}

// yes. most evil. we dlopen libdrm and libGL etc. to manually find smbols
// so we can be as compatible as possible given the whole mess of the
// gl/dri/drm etc. world. and handle graceful failure at runtime not
// compile time
/**
 * @brief Dynamically loads the DRM library and resolves necessary symbols.
 *
 * This function attempts to dlopen a DRM library (e.g., libdrm.so.2) and
 * dlsym the required DRM functions (drmClose, drmWaitVBlank, etc.).
 * This allows Ecore to use DRM vsync without a hard build-time dependency
 * on libdrm development headers and to gracefully degrade if DRM is not
 * available or symbols cannot be found.
 *
 * @return 1 if linking was successful and all symbols were found, 0 otherwise.
 */
static int
_drm_link(void)
{
   const char *drm_libs[] =
   {
      "libdrm.so.2",
      "libdrm.so.1",
      "libdrm.so.0",
      "libdrm.so",
      NULL,
   };
   int i, fail;
#define SYM(lib, xx)                         \
   do {                                      \
      sym_ ## xx = dlsym(lib, #xx);          \
      if (!(sym_ ## xx)) {                   \
         fail = 1;                           \
      }                                      \
   } while (0)

   if (drm_lib) return 1;
   for (i = 0; drm_libs[i]; i++)
     {
        drm_lib = dlopen(drm_libs[i], RTLD_LOCAL | RTLD_LAZY);
        if (drm_lib)
          {
             fail = 0;
             SYM(drm_lib, drmClose);
             SYM(drm_lib, drmWaitVBlank);
             SYM(drm_lib, drmHandleEvent);
             SYM(drm_lib, drmGetVersion);
             SYM(drm_lib, drmFreeVersion);
             if (fail)
               {
                  dlclose(drm_lib);
                  drm_lib = NULL;
               }
             else break;
          }
     }
   if (!drm_lib) return 0;
   return 1;
}

#define DRM_HAVE_NVIDIA 1 /**< Flag indicating if an NVIDIA binary driver is detected, which might affect vsync behavior. */

/**
 * @brief Performs a glob pattern match.
 *
 * A simple wrapper around eina_fnmatch to check if a string matches a glob pattern.
 *
 * @param glob The glob pattern. If NULL, it's considered a match.
 * @param str The string to match against the pattern. If NULL, it's not a match (unless glob is also NULL).
 * @return EINA_TRUE if the string matches the glob pattern, EINA_FALSE otherwise.
 */
static Eina_Bool
glob_match(const char *glob, const char *str)
{
   if (!glob) return EINA_TRUE;
   if (!str) return EINA_FALSE;
   if (eina_fnmatch(glob, str, 0)) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @brief Initializes the DRM vsync mechanism.
 *
 * This function performs several checks:
 * - Verifies kernel version and presence of specific DRM-related modules (e.g., vboxvideo).
 * - Opens the DRM device (typically /dev/dri/card0 or card1).
 * - Retrieves and validates the DRM driver version and information.
 * - Checks the driver against a whitelist of known-to-work configurations.
 * - Sets up DRM event handling (vblank handlers).
 * - Schedules the first vblank tick.
 * - Initializes threading or fd_handler based on `threaded_vsync` setting.
 *
 * @param flags Pointer to an integer where flags like DRM_HAVE_NVIDIA can be set.
 * @return 1 on successful initialization, 0 on failure.
 */
static int
_drm_init(int *flags)
{
   // whitelist of known-to-work drivers
   struct whitelist_card
     {
        const char *name_glob;
        const char *desc_glob;
        const char *date_glob;
        int drm_ver_min_maj;
        int drm_ver_min_min;
        int kernel_ver_min_maj;
        int kernel_ver_min_min;
     };
   static const struct whitelist_card whitelist[] = {
      { "exynos",  "*Samsung*", NULL,                 1,  6,    3,  0 },
      { "i915",    "*Intel*",   NULL,                 1,  6,    3, 14 },
      { "radeon",  "*Radeon*",  NULL,                 2, 39,    3, 14 },
      { "amdgpu",  "*AMD*",     NULL,                 3,  0,    4,  9 },
      { "nouveau", "*nVidia*",  "201[23456789]*",     1,  3,    4,  9 },
      { "nouveau", "*nVidia*",  "202[0123456789]*",   1,  3,    4,  9 },
      { NULL, NULL, NULL, 0, 0, 0, 0 }
   };
   int i;
   struct stat st;
   char buf[512];
   Eina_Bool ok = EINA_FALSE;
   Eina_Bool card0, card1;
   int vmaj = 0, vmin = 0;
   FILE *fp;

   // vboxvideo 4.3.14 is crashing when calls drmWaitVBlank()
   // https://www.virtualbox.org/ticket/13265
   // also affects 4.3.12
   if (stat("/sys/module/vboxvideo", &st) == 0) return 0;

   // only do this on new kernels = let's say 3.14 and up. 3.16 definitely
   // works
   fp = fopen("/proc/sys/kernel/osrelease", "rb");
   if (fp)
     {
        if (fgets(buf, sizeof(buf), fp))
          {
             if (sscanf(buf, "%i.%i.%*s", &vmaj, &vmin) == 2)
               {
                  if (vmaj >= 3) ok = EINA_TRUE;
               }
          }
        fclose(fp);
     }
   if (!ok) return 0;
   ok = EINA_FALSE;

   D("VSYNC: init...\n");
   card0 = (stat("/dev/dri/card0", &st) == 0);
   card1 = (stat("/dev/dri/card1", &st) == 0);
   if      (!card0 && card1)
     snprintf(buf, sizeof(buf), "/dev/dri/card1");
   else if (card0 && !card1)
     snprintf(buf, sizeof(buf), "/dev/dri/card0");
   else
     {
        D("VSYNC: 2 cards - confused. can't do this.\n");
        // XXX: 2 dri cards - ambiguous. unknown device for screen
        if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
          fprintf(stderr, "You have 2 DRI cards. Don't know which to use for vsync\n");
        return 0;
     }
   D("VSYNC: open %s\n", buf);
   drm_fd = open(buf, O_RDWR | O_CLOEXEC);
   if (drm_fd < 0)
     {
        if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
          fprintf(stderr, "Cannot open device card 0 (/de/dri/card0)\n");
        return 0;
     }

   if (!getenv("ECORE_VSYNC_DRM_ALL"))
     {
        drmVersion *drmver;
        drmVersionBroken *drmverbroken;

        drmver = sym_drmGetVersion(drm_fd);
        drmverbroken = (drmVersionBroken *)drmver;
        if (!drmver)
          {
             if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
               fprintf(stderr, "Cannot get dri version info from drmGetVersion()\n");
             close(drm_fd);
             return 0;
          }
        // sanity check the drm version structure due to public versions
        // not matching the real memory layout, check drm version
        // is recent (1.6+) and name and sec ptrs exist AND their lengths are
        // not garbage (within a sensible range)
        if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
          {
             if ((drmverbroken->name > (char *)4000L) &&
                 (drmverbroken->date_len < 200))
              fprintf(stderr,
                      "!BROKEN DRM! Do FIXUP of ABI\n"
                      "DRM Version: %i.%i\n"
                      "Name:        '%s'\n"
                      "Date:        '%s'\n"
                      "Desc:        '%s'\n",
                      drmverbroken->version_major, drmverbroken->version_minor,
                      drmverbroken->name, drmverbroken->date, drmverbroken->desc);
             else
               fprintf(stderr,
                       "OK DRM\n"
                       "DRM Version: %i.%i\n"
                       "Name:        '%s'\n"
                       "Date:        '%s'\n"
                       "Desc:        '%s'\n",
                       drmver->version_major, drmver->version_minor,
                       drmver->name, drmver->date, drmver->desc);
          }

        if ((((drmver->version_major == 1) &&
              (drmver->version_minor >= 3)) ||
             (drmver->version_major > 1)) &&
            (drmver->name > (char *)4000L) &&
            (drmver->date_len < 200))
          {
             if ((!strcmp(drmver->name, "nvidia-drm")) &&
                 (strstr(drmver->desc, "NVIDIA DRM driver")))
               {
                  if (((vmaj >= 3) && (vmin >= 14)) || (vmaj >= 4))
                    {
                       if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
                         fprintf(stderr, "You have nVidia binary drivers - no vsync\n");
                       *flags |= DRM_HAVE_NVIDIA;
                       goto checkdone;
                    }
               }
             for (i = 0; whitelist[i].name_glob; i++)
               {
                  if ((glob_match(whitelist[i].name_glob, drmver->name)) &&
                      (glob_match(whitelist[i].desc_glob, drmver->desc)) &&
                      (glob_match(whitelist[i].date_glob, drmver->date)) &&
                      ((drmver->version_major > whitelist[i].drm_ver_min_maj) ||
                       ((drmver->version_major == whitelist[i].drm_ver_min_maj) &&
                        (drmver->version_minor >= whitelist[i].drm_ver_min_min))) &&
                      ((vmaj > whitelist[i].kernel_ver_min_maj) ||
                       ((vmaj == whitelist[i].kernel_ver_min_maj) &&
                        (vmin >= whitelist[i].kernel_ver_min_min))))
                    {
                       if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
                         fprintf(stderr, "Whitelisted %s OK\n",
                                 whitelist[i].name_glob);
                       ok = EINA_TRUE;
                       goto checkdone;
                    }
               }
          }
        else if ((((drmverbroken->version_major == 1) &&
                   (drmverbroken->version_minor >= 3)) ||
                  (drmverbroken->version_major > 1)) &&
                 (drmverbroken->name > (char *)4000L) &&
                 (drmverbroken->date_len < 200))
          {
             if ((!strcmp(drmverbroken->name, "nvidia-drm")) &&
                 (strstr(drmverbroken->desc, "NVIDIA DRM driver")))
               {
                  if (((vmaj >= 3) && (vmin >= 14)) || (vmaj >= 4))
                    {
                       if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
                         fprintf(stderr, "You have nVidia binary drivers - no vsync\n");
                       *flags |= DRM_HAVE_NVIDIA;
                       goto checkdone;
                    }
               }
             for (i = 0; whitelist[i].name_glob; i++)
               {
                  if ((glob_match(whitelist[i].name_glob, drmverbroken->name)) &&
                      (glob_match(whitelist[i].desc_glob, drmverbroken->desc)) &&
                      (glob_match(whitelist[i].date_glob, drmverbroken->date)) &&
                      ((drmverbroken->version_major > whitelist[i].drm_ver_min_maj) ||
                       ((drmverbroken->version_major == whitelist[i].drm_ver_min_maj) &&
                        (drmverbroken->version_minor >= whitelist[i].drm_ver_min_min))) &&
                      ((vmaj > whitelist[i].kernel_ver_min_maj) ||
                       ((vmaj == whitelist[i].kernel_ver_min_maj) &&
                        (vmin >= whitelist[i].kernel_ver_min_min))))
                    {
                       if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
                         fprintf(stderr, "Whitelisted %s OK\n",
                                 whitelist[i].name_glob);
                       ok = EINA_TRUE;
                       goto checkdone;
                    }
               }
          }
checkdone:
        sym_drmFreeVersion(drmver);
        if (!ok)
          {
             close(drm_fd);
             return 0;
          }
     }

   memset(&drm_evctx, 0, sizeof(drm_evctx));
   drm_evctx.version = DRM_EVENT_CONTEXT_VERSION;
   drm_evctx.vblank_handler = _drm_vblank_handler;
   drm_evctx.page_flip_handler = NULL;

   if (!_drm_tick_schedule())
     {
        if (getenv("ECORE_VSYNC_DRM_VERSION_DEBUG"))
          fprintf(stderr, "Cannot schedule vblank tick.event...\n");
        close(drm_fd);
        drm_fd = -1;
        return 0;
     }

   if (getenv("ECORE_ANIMATOR_SKIP")) tick_skip = EINA_TRUE;
   if (threaded_vsync)
     {
        tick_queue_count = 0;
        eina_spinlock_new(&tick_queue_lock);
        thq = eina_thread_queue_new();
        drm_thread = ecore_thread_feedback_run(_drm_tick_core, _drm_tick_notify,
                                               NULL, NULL, NULL, EINA_TRUE);
     }
   else
     {
        ecore_main_fd_handler_add(drm_fd, ECORE_FD_READ,
                                  _ecore_vsync_fd_handler, NULL,
                                  NULL, NULL);
     }
   return 1;
}

/**
 * @brief Sets or unsets the DRM-based custom animator tick source.
 *
 * If a `vsync_root` window is set (meaning vsync is desired for a specific
 * root window), this function configures Ecore's animator to use custom
 * tick callbacks (_drm_tick_begin, _drm_tick_end).
 * If `vsync_root` is 0, it reverts the animator source to the default timer
 * and cleans up DRM-related callbacks.
 *
 * @return EINA_TRUE always, though the success of setting the source depends
 *         on internal Ecore animator functions.
 */
static Eina_Bool
_drm_animator_tick_source_set(void)
{
   if (vsync_root)
     {
        ecore_animator_custom_source_tick_begin_callback_set
          (_drm_tick_begin, NULL);
        ecore_animator_custom_source_tick_end_callback_set
          (_drm_tick_end, NULL);
        ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_CUSTOM);
     }
   else
     {
        if (drm_fd >= 0)
          {
             _drm_tick_end(NULL);
             ecore_animator_custom_source_tick_begin_callback_set
               (NULL, NULL);
             ecore_animator_custom_source_tick_end_callback_set
               (NULL, NULL);
             ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_TIMER);
          }
     }
   return EINA_TRUE;
}
#endif







// XXX: missing mode 3 == separate x connection with compiled in dri2 proto
// handling ala mesa (taken from mesa likely)

static int mode = 0; /**< Vsync mode: 0 = none/unknown, 1 = DRM. Other modes were planned but not fully implemented. */

/**
 * @brief Initializes the vsync subsystem.
 *
 * This function is called once to set up the vsync mechanism.
 * It registers a log domain and attempts to initialize DRM-based vsync
 * if shared memory is available and a DRM device (/dev/dri/card0 or card1) exists.
 * Sets the `mode` variable based on successful initialization.
 */
static void
_vsync_init(void)
{
   static int done = 0;
   struct stat stb;
   int flags = 0;

   if (done) return;

   _vsync_log_dom = eina_log_domain_register("ecore_x_vsync", EINA_COLOR_LIGHTRED);
   if (_ecore_x_image_shm_check())
     {
#ifdef ECORE_X_VSYNC_DRM
        // preferred inline drm if possible
        if ((!stat("/dev/dri/card0", &stb)) ||
            (!stat("/dev/dri/card1", &stb)))
          {
             if (_drm_link())
               {
                  if (_drm_init(&flags)) mode = 1;
               }
          }
#endif
     }
   done = 1;
}

/**
 * @brief Sets the animator tick source to use DRM-based vsync for a given X window.
 *
 * This function determines if DRM-based vsync should be enabled. It checks:
 * - Environment variables (ECORE_VSYNC_THREAD, ECORE_VSYNC_NO_THREAD, ECORE_NO_VSYNC).
 * - Veto files (~/.ecore-no-vsync, /etc/.ecore-no-vsync).
 * If not vetoed, it gets the root window for the given `win`. If this root
 * is different from the currently active `vsync_root`, it calls `_vsync_init`
 * (if not already done) and then `_drm_animator_tick_source_set` to
 * configure Ecore's animator to use DRM vsync.
 *
 * @param win The Ecore_X_Window for which vsync is being configured.
 *            The root window of this window will be used.
 * @return EINA_TRUE if the tick source was successfully set or vsync is active,
 *         EINA_FALSE if vsync is vetoed or initialization fails.
 */
static Eina_Bool
_drm_ecore_x_vsync_animator_tick_source_set(Ecore_X_Window win)
{
   Ecore_X_Window root;
   static int vsync_veto = -1;

   if (vsync_veto == -1)
     {
        char buf[4096];
        const char *home;
        struct stat st;

        if (getenv("ECORE_VSYNC_THREAD")) threaded_vsync = EINA_TRUE;
        if (getenv("ECORE_VSYNC_NO_THREAD")) threaded_vsync = EINA_FALSE;
        home = eina_environment_home_get();
        if (!home) eina_environment_tmp_get();
        snprintf(buf, sizeof(buf), "%s/.ecore-no-vsync", home);
        if (getenv("ECORE_NO_VSYNC")) vsync_veto = 1;
        else if (stat(buf, &st) == 0) vsync_veto = 1;
        else if (stat("/etc/.ecore-no-vsync", &st) == 0) vsync_veto = 1;
        else vsync_veto = 0;
     }
   if (vsync_veto == 1) return EINA_FALSE;

   root = ecore_x_window_root_get(win);
   if (root != vsync_root)
     {
        _vsync_init();
        vsync_root = root;
#ifdef ECORE_X_VSYNC_DRM
        if (mode == 1) return _drm_animator_tick_source_set();
#endif
     }
   return EINA_TRUE;
}

static Ecore_X_Window _ecore_x_vsync_win = 0;
static unsigned long long _ecore_x_vsync_msc = 0;
static unsigned long long _ecore_x_vsync_prev_ust = 0;
static Eina_Bool _ecore_x_vsync_ticking = EINA_FALSE;
static Ecore_Timer *_ecore_x_vsync_delay_timer = NULL;
static double _ecore_x_vsync_delay_amount = 0.0;
static Ecore_Event_Handler *_ecore_x_vsync_complete_handler = NULL;
//#define PRESENTDBG 1
#ifdef PRESENTDBG
static double last_t = 0.0;
#endif

/**
 * @brief Timer callback to introduce a delay before ticking the animator.
 *
 * This function is used when `_ecore_x_vsync_animator_tick_delay` is positive.
 * After a Present Complete event, instead of ticking immediately, a timer
 * is started. This callback, when fired, calls `ecore_animator_custom_tick()`.
 * This helps in offsetting the animator tick from the actual vblank,
 * potentially for smoother animations or to align with compositor schedules.
 *
 * @param data User data (unused).
 * @return EINA_FALSE as the timer is one-shot.
 */
static Eina_Bool
_ecore_x_cb_vsync_delay(void *data EINA_UNUSED)
{
#ifdef PRESENTDBG
  double t = ecore_time_get();
#endif

  if (_ecore_x_vsync_ticking)
    {
#ifdef PRESENTDBG
      printf("tik delayed delta=%1.5f\n", t - last_t);
#endif
      ecore_animator_custom_tick();
    }
  _ecore_x_vsync_delay_timer = NULL;
  return EINA_FALSE;
}

/**
 * @brief Requests a Present extension NotifyMSC event.
 *
 * This function increments the Media Stream Counter (MSC) and sends a request
 * to the X server (via the Present extension) to be notified when the display
 * pipeline reaches this MSC. This is the core mechanism for Present-based vsync.
 */
static void
_ecore_x_vsync_req(void)
{
  _ecore_x_vsync_msc++;
  ecore_x_present_notify_msc(_ecore_x_vsync_win, 0, _ecore_x_vsync_msc, 1, 0);
}

/**
 * @brief Event handler for X Present Complete events.
 *
 * This function is called when the X server sends a Present Complete event,
 * indicating that a previously requested frame (identified by MSC) has been
 * displayed (or its vblank time has passed).
 * It updates timing information, potentially triggers a delayed animator tick
 * (if `_ecore_x_vsync_animator_tick_delay` is set), or ticks the animator
 * immediately. It then re-requests another vsync notification if ticking is active.
 *
 * @param data User data (unused).
 * @param type The type of the event (unused, expected to be ECORE_X_EVENT_PRESENT_COMPLETE).
 * @param info The event structure, cast to Ecore_X_Event_Present_Complete.
 * @return EINA_TRUE to continue handling events.
 */
static Eina_Bool
_ecore_x_cb_pres_complete(void *data EINA_UNUSED, int type EINA_UNUSED, void *info EINA_UNUSED)
{
  Ecore_X_Event_Present_Complete *ev = info;
#ifdef PRESENTDBG
  double t = ecore_time_get();
#endif

  if (_ecore_x_vsync_prev_ust != ev->ust)
    {
      if (_ecore_x_vsync_ticking)
        {
          if ((_ecore_x_vsync_animator_tick_delay > 0.0) &&
              (_ecore_x_vsync_delay_amount > 0.0))
            {
              if (!_ecore_x_vsync_delay_timer)
                _ecore_x_vsync_delay_timer = ecore_timer_add
                  (_ecore_x_vsync_delay_amount * _ecore_x_vsync_animator_tick_delay,
                   _ecore_x_cb_vsync_delay, NULL);
            }
          else ecore_animator_custom_tick();
          if (_ecore_x_vsync_prev_ust > 0)
            _ecore_x_vsync_delay_amount =
              (double)(ev->ust - _ecore_x_vsync_prev_ust) / 1000000.0;
        }
      _ecore_x_vsync_msc = ev->msc;
#ifdef PRESENTDBG
      printf("tik %i: msc=%llu | diff_ust=%llu | delta=%1.5fs\n", _ecore_x_vsync_ticking, _ecore_x_vsync_msc, ev->ust - _ecore_x_vsync_prev_ust, t - last_t);
      last_t = t;
#endif
      if (_ecore_x_vsync_ticking) _ecore_x_vsync_req();
      _ecore_x_vsync_prev_ust = ev->ust;
    }
#ifdef PRESENTDBG
  else
    printf("tik %i: msc=%llu | delta=%1.5fs skip\n", _ecore_x_vsync_ticking, _ecore_x_vsync_msc, t - last_t);
#endif
  return EINA_TRUE;
}

/**
 * @brief Callback function invoked when the custom animator source (Present-based) begins ticking.
 *
 * Sets the `_ecore_x_vsync_ticking` flag to true, resets previous timing info,
 * and makes the first request for a vsync notification using `_ecore_x_vsync_req()`.
 *
 * @param data User data (unused).
 */
static void
_ecore_x_vsync_present_tick_begin(void *data EINA_UNUSED)
{
  if (!_ecore_x_vsync_ticking)
    {
      _ecore_x_vsync_prev_ust = 0;
      _ecore_x_vsync_ticking = EINA_TRUE;
      _ecore_x_vsync_req();
    }
}

/**
 * @brief Callback function invoked when the custom animator source (Present-based) stops ticking.
 *
 * Sets the `_ecore_x_vsync_ticking` flag to false, which will prevent further
 * vsync requests in `_ecore_x_cb_pres_complete`.
 *
 * @param data User data (unused).
 */
static void
_ecore_x_vsync_present_tick_end(void *data EINA_UNUSED)
{
  if (_ecore_x_vsync_ticking)
    {
      _ecore_x_vsync_ticking = EINA_FALSE;
    }
}

/**
 * @brief Initializes or deinitializes the X Present extension based vsync mechanism.
 *
 * If `_ecore_x_vsync_win` is set (a valid window is targeted for vsync):
 *  - Sets up Ecore animator custom source tick callbacks for Present.
 *  - Selects Present Complete Notify events for the window.
 *  - Adds an event handler for ECORE_X_EVENT_PRESENT_COMPLETE if not already present.
 * If `_ecore_x_vsync_win` is 0 (vsync is being disabled or no target window):
 *  - Stops any ongoing Present ticking.
 *  - Resets Ecore animator custom source callbacks.
 *  - Reverts animator source to the default timer.
 *  - (Implicitly, the event handler for Present Complete remains, but won't be triggered
 *    if no events are selected or no window is targeted).
 */
static void
_ecore_x_vsync_init(void)
{
  if (_ecore_x_vsync_win)
    {
      ecore_animator_custom_source_tick_begin_callback_set(_ecore_x_vsync_present_tick_begin, NULL);
      ecore_animator_custom_source_tick_end_callback_set(_ecore_x_vsync_present_tick_end, NULL);
      ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_CUSTOM);
      ecore_x_present_select_events(_ecore_x_vsync_win,
                                    ECORE_X_PRESENT_EVENT_MASK_COMPLETE_NOTIFY);
      if (!_ecore_x_vsync_complete_handler)
        {
          _ecore_x_vsync_complete_handler =
            ecore_event_handler_add(ECORE_X_EVENT_PRESENT_COMPLETE,
                                    _ecore_x_cb_pres_complete, NULL);
        }
    }
  else
    {
      if (_ecore_x_vsync_ticking) _ecore_x_vsync_present_tick_end(NULL);
      ecore_animator_custom_source_tick_begin_callback_set(NULL, NULL);
      ecore_animator_custom_source_tick_end_callback_set(NULL, NULL);
      ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_TIMER);
    }
}

/**
 * @brief Sets the animator tick source for Ecore X, choosing between Present extension or DRM.
 *
 * This is the main public API function to enable vsync for an Ecore X window.
 * It first checks if the X Present extension is available and if the
 * ECORE_X_VSYNC_PRESENT environment variable is set. If both are true,
 * it attempts to use Present extension-based vsync by calling `_ecore_x_vsync_init()`.
 * Otherwise, it falls back to trying DRM-based vsync via
 * `_drm_ecore_x_vsync_animator_tick_source_set()`.
 *
 * The `win` parameter is used to determine the root window, ensuring vsync is
 * tied to a specific display/screen.
 *
 * @param win The Ecore_X_Window for which to enable vsync.
 * @return EINA_TRUE if a vsync mechanism was successfully configured or is already active
 *         for the window's root, EINA_FALSE otherwise (e.g., neither Present nor DRM
 *         could be initialized, or vsync is vetoed).
 */
EAPI Eina_Bool
ecore_x_vsync_animator_tick_source_set(Ecore_X_Window win)
{
  if ((ecore_x_present_exists()) &&
      (getenv("ECORE_X_VSYNC_PRESENT")))
    {
      // get the root win because this win may be deleted
      win = ecore_x_window_root_get(win);
      if (_ecore_x_vsync_win == win) return EINA_TRUE;
      _ecore_x_vsync_win = win;
      _ecore_x_vsync_init();
      return EINA_TRUE;
    }
  else return _drm_ecore_x_vsync_animator_tick_source_set(win);
}

/**
 * @brief Sets a delay for animator ticks relative to the vsync event.
 *
 * This allows animator ticks to be offset from the actual vblank time.
 * The delay is a fraction of the detected frame time. For example, a delay
 * of 0.5 would attempt to tick halfway between vblanks.
 * This is used by both DRM and Present vsync mechanisms if they support it.
 *
 * @param delay The delay factor, typically between 0.0 (no delay) and 1.0.
 *              A value of 0.0 means the tick will happen as close as possible
 *              to the vsync event. A value of 0.5 means it will be delayed
 *              by half the frame duration.
 */
EAPI void
ecore_x_vsync_animator_tick_delay_set(double delay)
{
  _ecore_x_vsync_animator_tick_delay = delay;
}
