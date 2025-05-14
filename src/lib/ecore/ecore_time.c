#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_LOOP_PROTECTED

#include <stdlib.h>
#include <sys/time.h>

#if defined(__APPLE__) && defined(__MACH__)
# include <mach/mach_time.h>
#endif

#include <time.h>

#ifdef _WIN32
# include <evil_private.h> /* gettimeofday */
#endif

#include "Ecore.h"
#include "ecore_private.h"

#if defined(_WIN32)
/** @internal
 * @brief Stores the frequency of the performance counter on Windows.
 * Used to convert raw counter values to seconds.
 */
static LONGLONG _ecore_time_freq;
#elif defined (HAVE_CLOCK_GETTIME)
/** @internal
 * @brief Stores the clock ID to be used with clock_gettime().
 * This is determined during initialization.
 */
static clockid_t _ecore_time_clock_id;
/** @internal
 * @brief Flag indicating whether a valid clock ID has been obtained.
 */
static Eina_Bool _ecore_time_got_clock_id = EINA_FALSE;
#elif defined(__APPLE__) && defined(__MACH__)
/** @internal
 * @brief Conversion factor for mach_absolute_time() to seconds on macOS.
 * Calculated from mach_timebase_info.
 */
static double _ecore_time_clock_conversion = 1e-9;
#endif

/**
 * @brief Retrieves the current time in seconds.
 *
 * This function returns the current time in seconds since an arbitrary
 * fixed point in the past. It aims to provide a monotonic clock where
 * available, otherwise it may fall back to a system clock that can
 * be adjusted (e.g., CLOCK_REALTIME or gettimeofday).
 *
 * The precision of the returned time depends on the underlying system
 * clock source.
 *
 * @return The current time in seconds. Returns 0.0 on critical error.
 *
 * @see ecore_time_unix_get()
 * @see ecore_loop_time_get()
 */
EAPI double
ecore_time_get(void)
{
#ifdef _WIN32
   LARGE_INTEGER count;

   QueryPerformanceCounter(&count);
   return (double)count.QuadPart/ (double)_ecore_time_freq;
#elif defined (HAVE_CLOCK_GETTIME)
   struct timespec t;

   if (EINA_UNLIKELY(!_ecore_time_got_clock_id))
     return ecore_time_unix_get();

   if (EINA_UNLIKELY(clock_gettime(_ecore_time_clock_id, &t)))
     {
        CRI("Cannot get current time");
        return 0.0;
     }

   return (double)t.tv_sec + (((double)t.tv_nsec) / 1000000000.0);
#elif defined(__APPLE__) && defined(__MACH__)
   return _ecore_time_clock_conversion * (double)mach_absolute_time();
#else
   return ecore_time_unix_get();
#endif
}

/**
 * @brief Retrieves the current time in seconds using gettimeofday().
 *
 * This function provides the current time based on the gettimeofday()
 * system call, which typically returns time since the Unix epoch
 * (00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970).
 * This clock is not necessarily monotonic and can be subject to system
 * time adjustments.
 *
 * This function serves as a fallback for ecore_time_get() on systems
 * where higher-resolution or monotonic clocks are not available or
 * could not be initialized.
 *
 * @return The current time in seconds.
 *
 * @note This function will cause a compile-time error if gettimeofday()
 *       is not available on the target platform.
 *
 * @see ecore_time_get()
 */
EAPI double
ecore_time_unix_get(void)
{
#ifdef HAVE_GETTIMEOFDAY
   struct timeval timev;

   gettimeofday(&timev, NULL);
   return (double)timev.tv_sec + (((double)timev.tv_usec) / 1000000);
#else
# error "Your platform isn't supported yet"
#endif
}

/**
 * @brief Retrieves the time of the last iteration of the main loop.
 *
 * This function returns the time, in seconds, at which the last iteration
 * of the Ecore main loop started. This time is updated by the main loop
 * itself.
 *
 * @return The time of the last main loop iteration in seconds.
 *
 * @see efl_loop_time_get()
 * @see ecore_loop_time_set()
 */
EAPI double
ecore_loop_time_get(void)
{
   return efl_loop_time_get(ML_OBJ);
}

/**
 * @brief Sets the current time of the main loop.
 *
 * This function is typically used internally to update the main loop's
 * concept of the current time. It includes a check to prevent setting
 * the loop time to a point significantly in the future, as this could
 * indicate a problem or cause unexpected behavior in timed events.
 *
 * @param t The time in seconds to set as the current loop time.
 *          Example: `1678886400.5` (representing a specific point in time).
 *
 * @warning Setting the loop time far into the future will print an error
 *          and the operation will be ignored.
 *
 * @see efl_loop_time_set()
 * @see ecore_loop_time_get()
 */
EAPI void
ecore_loop_time_set(double t)
{
   double tnow = ecore_time_get();
   double tdelta = t - tnow;

   if (tdelta > 0.0)
     {
        fprintf(stderr,
                "Eccore: Trying to set loop time (%1.8f) %1.8fs too far in the future\n",
                t, tdelta);
        return;
     }
   efl_loop_time_set(ML_OBJ, t);
}

/*-********************   Internal methods   ********************************/

/* TODO: Documentation says "All  implementations  support  the  system-wide
 * real-time clock, which is identified by CLOCK_REALTIME. Check if the fallback
 * to unix time (without specifying the resolution) might be removed
 */
/**
 * @internal
 * @brief Initializes the Ecore time subsystem.
 *
 * This function is called internally to set up the necessary resources
 * for time measurement. It attempts to find the best available clock
 * source on the system:
 * - On Windows, it queries the performance counter frequency.
 * - On systems with `clock_gettime`, it tries to use `CLOCK_MONOTONIC`,
 *   falling back to `CLOCK_REALTIME` if `CLOCK_MONOTONIC` is unavailable.
 * - On macOS, it initializes the conversion factor for `mach_absolute_time`.
 *
 * If a preferred clock source cannot be initialized, it logs an error and
 * may rely on `ecore_time_unix_get()` (i.e., `gettimeofday`) as a final
 * fallback.
 *
 * After setting up the clock source, it initializes the main loop time
 * to the current time.
 *
 * @see _ecore_time_freq
 * @see _ecore_time_clock_id
 * @see _ecore_time_got_clock_id
 * @see _ecore_time_clock_conversion
 */
void
_ecore_time_init(void)
{
#if defined(_WIN32)
   LARGE_INTEGER freq;

   QueryPerformanceFrequency(&freq);
   _ecore_time_freq = freq.QuadPart;
#elif defined(HAVE_CLOCK_GETTIME)
   struct timespec t;

   if (_ecore_time_got_clock_id) return;

   if (!clock_gettime(CLOCK_MONOTONIC, &t))
     {
        _ecore_time_clock_id = CLOCK_MONOTONIC;
        _ecore_time_got_clock_id = EINA_TRUE;
        DBG("using CLOCK_MONOTONIC");
     }
   else if (!clock_gettime(CLOCK_REALTIME, &t))
     {
        // may go backwards
        _ecore_time_clock_id = CLOCK_REALTIME;
        _ecore_time_got_clock_id = EINA_TRUE;
        WRN("CLOCK_MONOTONIC not available. Fallback to CLOCK_REALTIME");
     }
   else
     CRI("Cannot get a valid clock_gettime() clock id! Fallback to unix time");
#else
# if defined(__APPLE__) && defined(__MACH__)
   mach_timebase_info_data_t info;
   kern_return_t err = mach_timebase_info(&info);
   if (err == 0)
     _ecore_time_clock_conversion = 1e-9 * (double)info.numer / (double)info.denom;
   else
     WRN("Unable to get timebase info. Fallback to nanoseconds");
# else
#  warning "Your platform isn't supported yet"
   CRI("Platform does not support clock_gettime. Fallback to unix time");
# endif
#endif
   ecore_loop_time_set(ecore_time_get());
}

