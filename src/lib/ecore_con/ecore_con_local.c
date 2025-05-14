#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#else
# include <pwd.h>
#endif
#include <libgen.h>

#ifdef _WIN32
# include <ws2tcpip.h>
#endif

#include <Ecore.h>
#include <ecore_private.h>

#include "Ecore_Con.h"
#include "ecore_con_private.h"

/**
 * @brief Generates a path for local Ecore_Con communication.
 *
 * This function constructs a platform-dependent path string that can be used
 * for local inter-process communication (e.g., Unix domain sockets or
 * Windows named pipes). The path varies based on whether it's a system-wide
 * or user-specific service, the service name, and an optional port number.
 *
 * @param is_system If EINA_TRUE, creates a path for a system-wide service.
 *                  If EINA_FALSE, creates a path for a user-specific service.
 *                  On Windows, user-specific paths include the username.
 *                  On Unix-like systems, user-specific paths are typically
 *                  placed in "(:usr.run:)/.ecore/". System paths on Unix-like
 *                  systems are placed in the directory returned by
 *                  eina_environment_tmp_get() (e.g., /tmp).
 * @param name The base name for the service or socket. Cannot be NULL.
 *             If `is_system` is EINA_TRUE and `name` starts with '/',
 *             it is treated as an absolute path on Unix-like systems.
 * @param port An optional port number. If negative, the port is not included
 *             in the path string.
 * @return A newly allocated string containing the generated path.
 *         The caller is responsible for freeing this string using free().
 *         Returns NULL if `name` is NULL.
 *
 * @note On Windows, the path format is typically:
 *       - User: "ecore!<username>!<name>[!<port>]"
 *       - System: "ecore_service!<name>[!<port>]"
 *       The '!' character is used as a separator.
 * @note On Unix-like systems, the path format is typically:
 *       - User: "(:usr.run:)/.ecore/<name>[/<port>]" (resolved by eina_vpath)
 *       - System (relative name): "<tmp_dir>/.ecore_service|<name>[|<port>]"
 *       - System (absolute name): "<name>[|<port>]"
 *       The '|' character is used as a separator for system paths with relative names.
 */
ECORE_CON_API char *
ecore_con_local_path_new(Eina_Bool is_system, const char *name, int port)
{
#if _WIN32
   char buf[256 - sizeof(PIPE_NS)] = "";

   /* note: using '!' instead of '|' since at least on wine '|' causes
    * ERROR_INVALID_NAME
    */

   if (!is_system)
     {
        TCHAR user[sizeof(buf) - sizeof("ecore!u!n!1")] = "unknown";
        DWORD userlen = sizeof(user);
        if (!GetUserName(user, &userlen))
          {
             char *msg = _efl_net_windows_error_msg_get(GetLastError());
             ERR("GetUserName(%p, %lu): %s", user, userlen, msg);
             free(msg);
          }
        if (port < 0)
          snprintf(buf, sizeof(buf), "ecore!%s!%s", user, name);
        else
          snprintf(buf, sizeof(buf), "ecore!%s!%s!%d", user, name, port);
     }
   else
     {
        if (port < 0)
          snprintf(buf, sizeof(buf), "ecore_service!%s", name);
        else
          snprintf(buf, sizeof(buf), "ecore_service!%s!%d", name, port);
     }
   return strdup(buf);
#else
   char buf[4096];
   const char *homedir;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   if (!is_system)
     {
        if (port < 0)
          eina_vpath_resolve_snprintf(buf, sizeof(buf), "(:usr.run:)/.ecore/%s", name);
        else
          eina_vpath_resolve_snprintf(buf, sizeof(buf), "(:usr.run:)/.ecore/%s/%i", name, port);

        return strdup(buf);
     }
   else
     {
        if (port < 0)
          {
             if (name[0] == '/')
               return strdup(name);
             else
               {
                  homedir = eina_environment_tmp_get();
                  snprintf(buf, sizeof(buf), "%s/.ecore_service|%s",
                           homedir, name);
                  return strdup(buf);
               }
          }
        else
          {
             if (name[0] == '/')
               snprintf(buf, sizeof(buf), "%s|%i", name, port);
             else
               {
                  homedir = eina_environment_tmp_get();
                  snprintf(buf, sizeof(buf), "%s/.ecore_service|%s|%i",
                           homedir, name, port);
               }
             return strdup(buf);
          }
     }
#endif
}

/**
 * @internal
 * @brief Creates all parent directories for a given path if they do not exist.
 *
 * This function is similar to `mkdir -p`. It takes a full path to a file or
 * directory and ensures that all leading directory components exist, creating
 * them with the specified mode if necessary.
 *
 * @param path The full path for which the directory structure should be created.
 *             Must be an absolute path (start with '/').
 *             If NULL, the function does nothing.
 * @param mode The file mode (permissions) to use when creating new directories.
 *             See `man 2 mkdir` for details on mode.
 *
 * @note This function is intended for internal use within Ecore_Con.
 * @warning If `path` is not an absolute path (does not start with '/'),
 *          a safety check will trigger, and the function will return early.
 *          Errors during directory creation (other than EEXIST for an existing
 *          directory) are logged using ERR().
 */
void
_ecore_con_local_mkpath(const char *path, mode_t mode)
{
   char *s, *d, *itr;

   if (!path) return;
   EINA_SAFETY_ON_TRUE_RETURN(path[0] != '/');

   s = strdup(path);
   EINA_SAFETY_ON_NULL_RETURN(s);
   d = dirname(s);
   EINA_SAFETY_ON_NULL_RETURN(d);

   for (itr = d + 1; *itr != '\0'; itr++)
     {
        if (*itr == '/')
          {
             *itr = '\0';
             if (mkdir(d, mode) != 0)
               {
                  if (errno != EEXIST)
                    {
                       ERR("could not create parent directory '%s' of path '%s': %s", d, path, eina_error_msg_get(errno));
                       goto end;
                    }
               }
             *itr = '/';
          }
     }

   if (mkdir(d, mode) != 0)
     {
        if (errno != EEXIST)
          ERR("could not create parent directory '%s' of path '%s': %s", d, path, eina_error_msg_get(errno));
        else
          {
             struct stat st;
             if ((stat(d, &st) != 0) || (!S_ISDIR(st.st_mode)))
               ERR("could not create parent directory '%s' of path '%s': exists but is not a directory", d, path);
          }
     }

 end:
   free(s);
}
