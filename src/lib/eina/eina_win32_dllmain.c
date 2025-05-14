/**
 * @file
 * @brief This file contains the DllMain function for the Eina library on Windows.
 *
 * It handles thread-specific cleanup when a thread detaches from the DLL.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eina_config.h"
#include "eina_types.h"
#include <evil_private.h>

void free_thread(void);

/**
 * @brief Entry point for the DLL.
 *
 * This function is called by the system when the DLL is loaded or unloaded,
 * or when a thread is created or terminated.
 *
 * @param inst Handle to the DLL module. This parameter is unused.
 * @param reason The reason code that indicates why the DLL entry-point function is being called.
 *               This function specifically handles the `DLL_THREAD_DETACH` reason.
 * @param reserved Reserved. This parameter is unused.
 * @return Always returns `TRUE`.
 */
BOOL WINAPI
DllMain(HINSTANCE inst EINA_UNUSED, WORD reason, PVOID reserved EINA_UNUSED)
{
   // When a thread is detaching, perform thread-specific cleanup.
   if (DLL_THREAD_DETACH == reason)
      free_thread();

   return TRUE;
}
