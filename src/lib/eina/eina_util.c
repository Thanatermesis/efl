/* EINA - EFL data type library
 * Copyright (C) 2015 Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#ifdef _WIN32
# include <string.h>
#else
# include <sys/types.h>
# include <pwd.h>
# include <string.h>
#endif

#include "eina_config.h"
#include "eina_private.h"
#include "eina_tmpstr.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EINA_API const char *
eina_environment_home_get(void)
{
   static char *home = NULL; // Cache for the home directory path.

   if (home) return home; // Return cached path if already resolved.
#ifdef _WIN32
   // On Windows, attempt to find home directory using common environment variables.
   // Priority: USERPROFILE, WINDIR, then HOMEDRIVE+HOMEPATH.
   home = getenv("USERPROFILE");
   if (!home || !*home) home = getenv("WINDIR");
   if ((!home  || !*home) &&
       (getenv("HOMEDRIVE") && getenv("HOMEPATH")))
     {
        // Concatenate HOMEDRIVE and HOMEPATH if both are set.
        char buf[PATH_MAX];

        snprintf(buf, sizeof(buf), "%s%s",
                 getenv("HOMEDRIVE"), getenv("HOMEPATH"));
        home = strdup(buf); // Store the combined path.
        // Note: strdup allocates memory which will be pointed to by the static 'home'
        // variable for the lifetime of the application after this function returns.
        return home;
     }
   if (!home) home = "C:\\"; // Default fallback for Windows.
#else
   // On POSIX-like systems.
# if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   // If process is not running setuid/setgid, trust HOME environment variable.
   if (getuid() == geteuid()) home = getenv("HOME");
# endif
   if (!home || !*home)
     {
        // If HOME is not set/empty or process is setuid/setgid, query user database.
# ifdef HAVE_GETPWENT
        struct passwd pwent, *pwent2 = NULL;
        char pwbuf[8129]; // Buffer for reentrant getpwuid_r.

        // Retrieve home directory from passwd entry for the effective user ID.
        if (!getpwuid_r(geteuid(), &pwent, pwbuf, sizeof(pwbuf), &pwent2))
          {
             if ((pwent2) && (pwent.pw_dir))
               {
                  home = strdup(pwent.pw_dir); // Store the retrieved path.
                  // Similar to Windows HOMEDRIVE/HOMEPATH case, return early
                  // as strdup allocates memory for the static 'home' variable.
                  return home;
               }
          }
# endif
        home = "/tmp"; // Default fallback for POSIX systems if other methods fail.
     }
#endif
   home = strdup(home); // Duplicate the path to ensure it's stored in writable memory owned by this cache.
                       // This handles cases where 'home' pointed to getenv's internal buffer or a string literal.
#ifdef _WIN32
   // For consistency within EFL, convert path separators to Unix style (/) on Windows.
   EINA_PATH_TO_UNIX(home);
#endif
   return home;
}

EINA_API const char *
eina_environment_tmp_get(void)
{
   static char *tmp = NULL; // Cache for the temporary directory path.

   if (tmp) return tmp; // Return cached path if already resolved.
#ifdef _WIN32
   // On Windows, attempt to find temp directory using common environment variables.
   // Priority: TMP, TEMP, USERPROFILE, WINDIR.
   tmp = getenv("TMP");
   if (!tmp || !*tmp) tmp = getenv("TEMP");
   if (!tmp || !*tmp) tmp = getenv("USERPROFILE");
   if (!tmp || !*tmp) tmp = getenv("WINDIR");
   if (!tmp || !*tmp) tmp = "C:\\"; // Default fallback for Windows.
#else
   // On POSIX-like systems.
# if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   // If process is not running setuid/setgid, trust standard temp environment variables.
   if (getuid() == geteuid())
# endif
     {
        // Check standard environment variables for temporary directory.
        // Priority: TMPDIR, TMP, TEMPDIR, TEMP.
        tmp = getenv("TMPDIR");
        if (!tmp || !*tmp) tmp = getenv("TMP");
        if (!tmp || !*tmp) tmp = getenv("TEMPDIR");
        if (!tmp || !*tmp) tmp = getenv("TEMP");
     }
   if (!tmp || !*tmp) tmp = "/tmp"; // Default fallback for POSIX systems.
#endif

#if defined(__MACH__) && defined(__APPLE__)
   // On macOS, normalize the path by removing a trailing slash if present.
   // This ensures consistency as some system APIs might be sensitive to it.
   if (tmp && tmp[strlen(tmp) -1] == '/')
     {
        char *tmp2 = strdup(tmp); // Duplicate to allow modification.
        tmp2[strlen(tmp2) - 1] = 0x0; // Remove trailing slash.
        tmp = tmp2; // Update tmp to point to the modified (and newly allocated) string.
        // This path is returned directly; the strdup at the end of the function is skipped.
        return tmp;
     }
#endif

   tmp = strdup(tmp); // Duplicate the path to ensure it's stored in writable memory owned by this cache.
                       // This handles cases where 'tmp' pointed to getenv's internal buffer or a string literal.
#ifdef _WIN32
   // For consistency within EFL, convert path separators to Unix style (/) on Windows.
   EINA_PATH_TO_UNIX(tmp);
#endif
   return tmp;
}
