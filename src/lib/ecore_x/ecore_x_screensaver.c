/*
 * Screensaver code
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

/**
 * @internal
 * @brief Stores the availability of the XScreenSaver extension.
 * @details -1 means not checked yet, 0 means not available, 1 means available.
 */
static int _screensaver_available = -1;

/**
 * @brief Checks if the XScreenSaver extension is available.
 * @return @c EINA_TRUE if the extension is available, @c EINA_FALSE otherwise.
 * @note The result is cached for subsequent calls.
 */
EAPI Eina_Bool
ecore_x_screensaver_event_available_get(void)
{
   if (_screensaver_available >= 0)
     return _screensaver_available;

#ifdef ECORE_XSS
   int _screensaver_major, _screensaver_minor;

   LOGFN;
   _screensaver_major = 1;
   _screensaver_minor = 0;

   if (XScreenSaverQueryVersion(_ecore_x_disp, &_screensaver_major,
                                &_screensaver_minor))
     _screensaver_available = 1;
   else
     _screensaver_available = 0;

#else /* ifdef ECORE_XSS */
   _screensaver_available = 0;
#endif /* ifdef ECORE_XSS */
   return _screensaver_available;
}

/**
 * @brief Gets the current screensaver idle time in seconds.
 * @return The idle time in seconds, or 0 if the XScreenSaver extension is not available.
 * @note This function may also consider DPMS (Display Power Management Signaling)
 *       state to provide a more accurate idle time if DPMS is active.
 */
EAPI int
ecore_x_screensaver_idle_time_get(void)
{
#ifdef ECORE_XSS
   XScreenSaverInfo *xss;
   unsigned long _idle;
   int dummy;
   int idle;

   LOGFN;
   xss = XScreenSaverAllocInfo();
   XScreenSaverQueryInfo(_ecore_x_disp,
                         RootWindow(_ecore_x_disp, DefaultScreen(
                                      _ecore_x_disp)), xss);

   _idle = xss->idle;
   XFree(xss);
   if (DPMSQueryExtension(_ecore_x_disp, &dummy, &dummy))
     {
        CARD16 standby, suspend, off;
        CARD16 state;
        BOOL onoff;

        if (DPMSCapable(_ecore_x_disp))
          {
             DPMSGetTimeouts(_ecore_x_disp, &standby, &suspend, &off);
             DPMSInfo(_ecore_x_disp, &state, &onoff);

             if (onoff)
               {
                  switch (state)
                    {
                     case DPMSModeStandby:
                        /* this check is a littlebit paranoid, but be sure */
                        if (_idle < (unsigned) (standby * 1000))
                          _idle += (standby * 1000);
                        break;
                     case DPMSModeSuspend:
                        if (_idle < (unsigned) ((suspend + standby) * 1000))
                          _idle += ((suspend + standby) * 1000);
                        break;
                     case DPMSModeOff:
                        if (_idle < (unsigned) ((off + suspend + standby) * 1000))
                          _idle += ((off + suspend + standby) * 1000);
                        break;
                     case DPMSModeOn:
                     default:
                        break;
                    }
               }
          }
     }
   idle = _idle / 1000;

   return idle;
#else
   return 0;
#endif /* ifdef ECORE_XSS */
}

/**
 * @brief Sets the screensaver parameters.
 * @param timeout The screensaver timeout in seconds.
 *        Use -1 to restore the default timeout.
 *        Use 0 to disable the screensaver.
 * @param interval The screensaver cycle interval in seconds.
 *        Use -1 to restore the default interval.
 * @param prefer_blanking The blanking preference.
 *        Possible values are @c ECORE_X_SCREENSAVER_BLANKING_NOT_PREFERRED,
 *        @c ECORE_X_SCREENSAVER_BLANKING_PREFERRED, or
 *        @c ECORE_X_SCREENSAVER_BLANKING_DEFAULT.
 * @param allow_exposures Whether to allow exposures.
 *        Possible values are @c ECORE_X_SCREENSAVER_EXPOSURES_NOT_ALLOWED,
 *        @c ECORE_X_SCREENSAVER_EXPOSURES_ALLOWED, or
 *        @c ECORE_X_SCREENSAVER_EXPOSURES_DEFAULT.
 * @note This function requires the XScreenSaver extension.
 */
EAPI void
ecore_x_screensaver_set(int timeout,
                        int interval,
                        int prefer_blanking,
                        int allow_exposures)
{
   LOGFN;
   XSetScreenSaver(_ecore_x_disp,
                   timeout,
                   interval,
                   prefer_blanking,
                   allow_exposures);
}

/**
 * @brief Sets the screensaver timeout.
 * @param timeout The screensaver timeout in seconds.
 *        Use -1 to restore the default timeout.
 *        Use 0 to disable the screensaver.
 * @note This function requires the XScreenSaver extension.
 *       It retrieves the current interval, blanking, and exposure settings
 *       and only modifies the timeout.
 */
EAPI void
ecore_x_screensaver_timeout_set(int timeout)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   XSetScreenSaver(_ecore_x_disp, timeout, pint, pblank, pexpo);
}

/**
 * @brief Gets the current screensaver timeout.
 * @return The screensaver timeout in seconds.
 * @note This function requires the XScreenSaver extension.
 */
EAPI int
ecore_x_screensaver_timeout_get(void)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   return pto;
}

/**
 * @brief Sets the screensaver blanking preference.
 * @param blank The blanking preference.
 *        Possible values are @c ECORE_X_SCREENSAVER_BLANKING_NOT_PREFERRED,
 *        @c ECORE_X_SCREENSAVER_BLANKING_PREFERRED, or
 *        @c ECORE_X_SCREENSAVER_BLANKING_DEFAULT.
 * @note This function requires the XScreenSaver extension.
 *       It retrieves the current timeout, interval, and exposure settings
 *       and only modifies the blanking preference.
 */
EAPI void
ecore_x_screensaver_blank_set(int blank)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   XSetScreenSaver(_ecore_x_disp, pto, pint, blank, pexpo);
}

/**
 * @brief Gets the current screensaver blanking preference.
 * @return The blanking preference.
 *         Possible values are @c ECORE_X_SCREENSAVER_BLANKING_NOT_PREFERRED,
 *         @c ECORE_X_SCREENSAVER_BLANKING_PREFERRED, or
 *         @c ECORE_X_SCREENSAVER_BLANKING_DEFAULT.
 * @note This function requires the XScreenSaver extension.
 */
EAPI int
ecore_x_screensaver_blank_get(void)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   return pblank;
}

/**
 * @brief Sets whether the screensaver allows exposures.
 * @param expose Whether to allow exposures.
 *        Possible values are @c ECORE_X_SCREENSAVER_EXPOSURES_NOT_ALLOWED,
 *        @c ECORE_X_SCREENSAVER_EXPOSURES_ALLOWED, or
 *        @c ECORE_X_SCREENSAVER_EXPOSURES_DEFAULT.
 * @note This function requires the XScreenSaver extension.
 *       It retrieves the current timeout, interval, and blanking settings
 *       and only modifies the exposure setting.
 */
EAPI void
ecore_x_screensaver_expose_set(int expose)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   XSetScreenSaver(_ecore_x_disp, pto, pint, pblank, expose);
}

/**
 * @brief Gets whether the screensaver allows exposures.
 * @return The exposure setting.
 *         Possible values are @c ECORE_X_SCREENSAVER_EXPOSURES_NOT_ALLOWED,
 *         @c ECORE_X_SCREENSAVER_EXPOSURES_ALLOWED, or
 *         @c ECORE_X_SCREENSAVER_EXPOSURES_DEFAULT.
 * @note This function requires the XScreenSaver extension.
 */
EAPI int
ecore_x_screensaver_expose_get(void)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   return pexpo;
}

/**
 * @brief Sets the screensaver cycle interval.
 * @param interval The screensaver cycle interval in seconds.
 *        Use -1 to restore the default interval.
 * @note This function requires the XScreenSaver extension.
 *       It retrieves the current timeout, blanking, and exposure settings
 *       and only modifies the interval.
 */
EAPI void
ecore_x_screensaver_interval_set(int interval)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   XSetScreenSaver(_ecore_x_disp, pto, interval, pblank, pexpo);
}

/**
 * @brief Gets the current screensaver cycle interval.
 * @return The screensaver cycle interval in seconds.
 * @note This function requires the XScreenSaver extension.
 */
EAPI int
ecore_x_screensaver_interval_get(void)
{
   int pto, pint, pblank, pexpo;

   LOGFN;
   XGetScreenSaver(_ecore_x_disp, &pto, &pint, &pblank, &pexpo);
   return pint;
}

/**
 * @brief Enables or disables listening for screensaver events.
 * @param on If @c EINA_TRUE, listen for screensaver events;
 *           if @c EINA_FALSE, stop listening.
 * @note This function requires the XScreenSaver extension.
 *       When enabled, Ecore will emit @c ECORE_X_EVENT_SCREENSAVER_NOTIFY events.
 *       The events include whether the screensaver activated or deactivated,
 *       and if it was forced or not.
 */
EAPI void
ecore_x_screensaver_event_listen_set(Eina_Bool on)
{
#ifdef ECORE_XSS
   Ecore_X_Window root;

   LOGFN;
   root = DefaultRootWindow(_ecore_x_disp);
   if (on)
     XScreenSaverSelectInput(_ecore_x_disp, root,
                             ScreenSaverNotifyMask | ScreenSaverCycle);
   else
     XScreenSaverSelectInput(_ecore_x_disp, root, 0);
#else
   return;
   on = EINA_FALSE;
#endif /* ifdef ECORE_XSS */
}


/**
 * @brief Enables custom blanking for the screensaver.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if
 *         the XScreenSaver extension is not available.
 * @note This function allows an application to take over the screen blanking
 *       mechanism, typically by drawing its own content when the screensaver
 *       would normally activate. It sets specific window attributes on the
 *       root window to achieve this. The exact values (-9999, 1, 1, 0) are
 *       specific to how XScreenSaver interprets these attributes for custom
 *       blanking.
 */
EAPI Eina_Bool
ecore_x_screensaver_custom_blanking_enable(void)
{
#ifdef ECORE_XSS
   XSetWindowAttributes attr;

   // The values -9999 for x and y, and 1 for width and height,
   // along with specific flags, signal to XScreenSaver that
   // custom blanking attributes are being set.
   XScreenSaverSetAttributes(_ecore_x_disp,
                             DefaultRootWindow(_ecore_x_disp),
                             -9999, -9999, 1, 1, 0,
                             CopyFromParent, InputOnly, CopyFromParent,
                             0, &attr);
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif /* ifdef ECORE_XSS */
}

/**
 * @brief Disables custom blanking for the screensaver.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure or if
 *         the XScreenSaver extension is not available.
 * @note This function reverts the changes made by
 *       ecore_x_screensaver_custom_blanking_enable(), restoring default
 *       screensaver behavior.
 */
EAPI Eina_Bool
ecore_x_screensaver_custom_blanking_disable(void)
{
#ifdef ECORE_XSS
   XScreenSaverUnsetAttributes(_ecore_x_disp,
                               DefaultRootWindow(_ecore_x_disp));
   return EINA_TRUE;
#else
   return EINA_FALSE;
#endif /* ifdef ECORE_XSS */
}

/**
 * @brief Suspends the screensaver. (Deprecated)
 * @deprecated Use ecore_x_screensaver_suspend() instead.
 * @note This function requires the XScreenSaver extension.
 */
EINA_DEPRECATED EAPI void
ecore_x_screensaver_supend(void)
{
   ecore_x_screensaver_suspend();
}

/**
 * @brief Suspends or resumes the screensaver.
 * @param suspend If non-zero, suspend the screensaver. If zero, resume.
 * @note This function requires the XScreenSaver extension.
 *       This is the underlying Xlib function call.
 *       ecore_x_screensaver_suspend() and ecore_x_screensaver_resume()
 *       are wrappers for this.
 */
/*
 * Internally, XScreenSaverSuspend(_ecore_x_disp, suspend_state) is called.
 * suspend_state = 1 means suspend.
 * suspend_state = 0 means resume.
 */

/**
 * @brief Suspends the screensaver.
 * @note This function requires the XScreenSaver extension.
 *       It prevents the screensaver from activating.
 */
EAPI void
ecore_x_screensaver_suspend(void)
{
#ifdef ECORE_XSS
   XScreenSaverSuspend(_ecore_x_disp, 1); // 1 to suspend
#endif /* ifdef ECORE_XSS */
}

/**
 * @brief Resumes the screensaver.
 * @note This function requires the XScreenSaver extension.
 *       It allows the screensaver to activate again after being suspended.
 */
EAPI void
ecore_x_screensaver_resume(void)
{
#ifdef ECORE_XSS
   XScreenSaverSuspend(_ecore_x_disp, 0); // 0 to resume
#endif /* ifdef ECORE_XSS */
}

/**
 * @brief Resets the screensaver.
 * @note This function requires the XScreenSaver extension.
 *       If the screensaver is active, it deactivates it.
 *       If the screensaver is inactive, it resets the idle timer.
 */
EAPI void
ecore_x_screensaver_reset(void)
{
   XResetScreenSaver(_ecore_x_disp);
}

/**
 * @brief Activates the screensaver immediately.
 * @note This function requires the XScreenSaver extension.
 */
EAPI void
ecore_x_screensaver_activate(void)
{
   XActivateScreenSaver(_ecore_x_disp);
}
