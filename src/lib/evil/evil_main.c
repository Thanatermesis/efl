#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include <windows.h>

#include "evil_private.h"

/** @internal
 * @brief Stores the minimum timer resolution supported by the system.
 *
 * This value is obtained from TIMECAPS.wPeriodMin and used with
 * timeBeginPeriod and timeEndPeriod.
 */
static UINT     _evil_time_period = 1;

/** @internal
 * @brief Initialization counter for the Evil library.
 *
 * Incremented by evil_init() and decremented by evil_shutdown().
 * The library is effectively initialized on the first call to evil_init()
 * and shut down when this counter reaches zero.
 */
static int      _evil_init_count = 0;

/** @internal
 * @brief Stores the frequency of the high-resolution performance counter.
 *
 * This value is obtained by QueryPerformanceFrequency().
 * It is declared as extern because it might be set or used in other
 * parts of the Evil library, specifically related to time functions.
 */
extern LONGLONG _evil_time_freq;
/** @internal
 * @brief Stores the initial value of the high-resolution performance counter.
 *
 * This value is obtained by QueryPerformanceCounter() during initialization.
 * It is declared as extern because it might be set or used in other
 * parts of the Evil library, specifically related to time functions.
 */
extern LONGLONG _evil_time_count;

/** @internal
 * @brief Index for Thread Local Storage (TLS).
 *
 * This index is allocated by TlsAlloc() during DLL_PROCESS_ATTACH
 * and used to store thread-specific data.
 * It is declared as extern as it's used across different parts of the
 * library that require thread-local data, potentially defined in evil_private.h
 * or another source file.
 */
extern DWORD    _evil_tls_index;

/**
 * @brief Initializes the Evil library.
 *
 * @return The current initialization count if successful, 0 on failure.
 *
 * This function performs essential setup for the Evil library, including:
 * - Setting up high-resolution timers.
 * - Querying performance counter frequency.
 * - Initializing socket functionalities.
 * It uses a counter to track nested calls, but performs actual initialization
 * only on the first call.
 */
int
evil_init(void)
{
   LARGE_INTEGER freq;
   LARGE_INTEGER count;
   TIMECAPS tc;
   MMRESULT res;

   if (++_evil_init_count != 1)
     return _evil_init_count; /* Already initialized, return current count */

   /* Retrieve device capabilities for the system timer. */
   res = timeGetDevCaps(&tc, sizeof(TIMECAPS));
   if (res  != MMSYSERR_NOERROR)
     return --_evil_init_count; /* Failed, decrement count and return 0 */

   /* Set the timer resolution to the minimum supported period. */
   _evil_time_period = tc.wPeriodMin;
   res = timeBeginPeriod(_evil_time_period);
   if (res  != TIMERR_NOERROR)
     return --_evil_init_count; /* Failed, decrement count and return 0 */

   /* Query the frequency of the high-resolution performance counter. */
   QueryPerformanceFrequency(&freq);

   _evil_time_freq = freq.QuadPart;

   /* Query the current value of the high-resolution performance counter. */
   QueryPerformanceCounter(&count);

   _evil_time_count = count.QuadPart;

   /* Initialize socket related functionalities. */
   if (!evil_sockets_init())
     return --_evil_init_count; /* Failed, decrement count and return 0 */

   return _evil_init_count;
}

/**
 * @brief Shuts down the Evil library.
 *
 * @return The remaining initialization count. Returns 0 if the library is fully shut down.
 *
 * This function decrements the initialization counter. If the counter reaches zero,
 * it performs the actual cleanup, including:
 * - Releasing socket resources.
 * - Restoring the system's timer period.
 * It also handles cases where shutdown is called without a prior successful initialization.
 */
int
evil_shutdown(void)
{
   /* _evil_init_count should not go below zero. */
   if (_evil_init_count < 1)
     {
        /* This indicates a programming error or misuse of the library. */
        printf("Evil shutdown called without calling evil init.\n");
        return 0;
     }

   if (--_evil_init_count != 0)
     return _evil_init_count; /* Still other users, return current count */

   /* Shut down socket related functionalities. */
   evil_sockets_shutdown();

   /* Restore the original timer period. */
   timeEndPeriod(_evil_time_period);

   return _evil_init_count; /* Should be 0 at this point */
}

/**
 * @brief Entry point for the DLL.
 *
 * @param inst Handle to the DLL module.
 * @param reason The reason code that indicates why the DLL entry-point function is being called.
 * @param reserved Reserved.
 * @return TRUE if initialization succeeded for DLL_PROCESS_ATTACH, FALSE otherwise.
 *         For other reasons, it generally returns TRUE.
 *
 * This function handles process and thread attachment/detachment notifications.
 * It is primarily used here to manage Thread Local Storage (TLS) for
 * thread-specific data required by the Evil library.
 * - On DLL_PROCESS_ATTACH: Allocates a TLS index.
 * - On DLL_THREAD_ATTACH: Allocates memory for the current thread's TLS data.
 * - On DLL_THREAD_DETACH: Frees the current thread's TLS data.
 * - On DLL_PROCESS_DETACH: Frees the main thread's TLS data and the TLS index.
 */
BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
   LPVOID data;

   switch (reason)
     {
     case DLL_PROCESS_ATTACH:
       /* Allocate a TLS index for this process. */
       _evil_tls_index = TlsAlloc();
       if (_evil_tls_index == TLS_OUT_OF_INDEXES)
         return FALSE; /* Failed to allocate TLS index */
       /* No break: Initialize the index for first thread (main thread). */
       /* fall through */
     case DLL_THREAD_ATTACH:
       /* Allocate memory for this thread's TLS data.
        * LPTR combines LMEM_FIXED and LMEM_ZEROINIT.
        * 4096 bytes is a common page size, chosen as a general buffer size.
        */
       data = (LPVOID)LocalAlloc(LPTR, 4096);
       if (!data)
         return FALSE; /* Failed to allocate memory */
       /* Store the allocated memory pointer in the TLS slot for the current thread. */
       if (!TlsSetValue(_evil_tls_index, data))
         return FALSE; /* Failed to set TLS value */
       break;
     case DLL_THREAD_DETACH:
       /* Retrieve this thread's TLS data. */
       data = TlsGetValue(_evil_tls_index);
       if (data)
         LocalFree((HLOCAL)data); /* Free the allocated memory. */
       break;
     case DLL_PROCESS_DETACH:
       /* Retrieve the main thread's TLS data (if any remaining). */
       data = TlsGetValue(_evil_tls_index);
       if (data)
         LocalFree((HLOCAL)data); /* Free the allocated memory. */
       /* Release the TLS index for the process. */
       TlsFree(_evil_tls_index);
       break;
     default:
       /* Other reasons are not handled. */
       break;
     }

   return TRUE;

   /* These parameters are unused in this specific DllMain implementation,
    * but are part of the standard DllMain signature.
    * (void) inst; silences compiler warnings about unused parameters.
    */
   (void)inst;
   (void)reserved;
}
