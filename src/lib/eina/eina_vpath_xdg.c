#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#ifndef _WIN32
# include <pwd.h>
#endif

#include <Eina.h>

#include "eina_internal.h"
#include "eina_private.h"

/**
 * @internal
 * @brief Initializes the XDG environment variables for Eina's vpath system.
 *
 * This function sets up the paths for common user directories (Desktop,
 * Documents, Downloads, etc.) based on environment variables or defaults
 * to standard locations within the user's home directory. It handles
 * platform-specific differences for Windows and POSIX-like systems (using
 * XDG Base Directory Specification).
 *
 * The resolved paths are then registered with the Eina vpath user interface.
 */
void
eina_xdg_env_init(void)
{
   char home[PATH_MAX], *s;
   Eina_Vpath_Interface_User user;

   eina_vpath_resolve_snprintf(home, sizeof(home), "(:home:)/");
   // last char is / - we won't want it
   for (s = home; *s; s++)
     {
        if (s[1] == 0) s[0] = 0;
     }
   memset(&user, 0, sizeof(Eina_Vpath_Interface_User));

/**
 * @internal
 * @def FATAL_SNPRINTF
 * @brief A macro for snprintf that aborts if the output is truncated.
 *
 * This macro wraps snprintf and checks if the output string was truncated.
 * If truncation occurs, it prints an error message to stderr and calls abort().
 * This is used to prevent buffer overflows when constructing paths.
 *
 * @param _buf The buffer to write to.
 * @param _err The error message string to print if truncation occurs.
 * @param _fmt The format string for snprintf.
 * @param ... Additional arguments for the format string.
 */
#define FATAL_SNPRINTF(_buf, _err, _fmt, ...) \
   do { \
      if ((size_t)snprintf(_buf, sizeof(_buf), _fmt, ## __VA_ARGS__) >= (sizeof(_buf) - 1)) { \
         fprintf(stderr, _err"\n", _buf); \
         abort(); \
      } \
   } while (0)

#ifdef _WIN32
/**
 * @internal
 * @def ENV_DIR_SET
 * @brief Sets a user directory path on Windows.
 *
 * This macro constructs a path by appending a subdirectory to a base path
 * determined by an environment variable or the user's home directory.
 * The resulting path is stored in a provided variable and converted to
 * Unix-style slashes.
 *
 * @param _env The environment variable to check for the base path. If NULL or
 *             not set, the user's home directory is used.
 * @param _dir The subdirectory to append to the base path.
 * @param _meta The variable name (member of Eina_Vpath_Interface_User) to
 *              store the resulting path. A char array with this name will be
 *              declared by the macro.
 */
# define ENV_DIR_SET(_env, _dir, _meta) \
   char _meta[PATH_MAX + 128]; \
   if (_env) { \
      s = getenv(_env); \
      if (!s) s = home; \
   } else s = home; \
   FATAL_SNPRINTF(_meta, "vpath string '%s' truncated - fatal", "%s\\%s", s, (char *)_dir); \
   EINA_PATH_TO_UNIX(_meta); \
   (&user)->_meta = _meta;

/**
 * @internal
 * @def ENV_SET
 * @brief Sets a user path on Windows.
 *
 * This macro retrieves a path from an environment variable or defaults to the
 * user's home directory. The resulting path is stored in a provided variable
 * and converted to Unix-style slashes.
 *
 * @param _env The environment variable to check for the path. If NULL or
 *             not set, the user's home directory is used.
 * @param _meta The variable name (member of Eina_Vpath_Interface_User) to
 *              store the resulting path. A char array with this name will be
 *              declared by the macro.
 */
# define ENV_SET(_env, _meta) \
   char _meta[PATH_MAX + 128]; \
   if (_env) { \
      s = getenv(_env); \
      if (!s) s = home; \
   } else s = home; \
   FATAL_SNPRINTF(_meta, "vpath string '%s' truncated - fatal", "%s", s); \
   EINA_PATH_TO_UNIX(_meta); \
   (&user)->_meta = _meta;

   ENV_DIR_SET(NULL, "Desktop", desktop);
   ENV_DIR_SET(NULL, "Documents", documents);
   ENV_DIR_SET(NULL, "Downloads", downloads);
   ENV_DIR_SET(NULL, "Music", music);
   ENV_DIR_SET(NULL, "Pictures", pictures);
   ENV_SET("PUBLIC", pub);
   ENV_DIR_SET("APPDATA", "Microsoft\\Windows\\Templates", templates);
   ENV_DIR_SET(NULL, "Videos", videos);
   ENV_SET("LOCALAPPDATA", data);
   ENV_DIR_SET("LOCALAPPDATA", "Temp", tmp);
   ENV_SET("APPDATA", config);
   ENV_SET("LOCALAPPDATA", cache);
   if (!(s = getenv("APPDATA")))
     user.run = NULL;
   else
     {
        EINA_PATH_TO_UNIX(s);
        user.run = s;
     }
#else /* _WIN32 */
// For non-Windows systems, typically POSIX-like environments using XDG Base Directory Specification.
# if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
/**
 * @internal
 * @def ENV_HOME_SET
 * @brief Sets a user directory path based on XDG environment variables or defaults.
 *
 * This macro retrieves a path from a specified XDG environment variable.
 * If the variable is not set, or if the real and effective user IDs differ
 * (indicating a potential security concern like suid), it defaults to a
 * path constructed by appending `_dir` to the user's home directory.
 * The resulting path is stored in the `_meta` variable.
 *
 * @param _env The XDG environment variable to check (e.g., "XDG_DESKTOP_DIR").
 * @param _dir The default subdirectory name within the home directory if `_env`
 *             is not set or if UID/EUID mismatch.
 * @param _meta The variable name (member of Eina_Vpath_Interface_User) to
 *              store the resulting path. A char array with this name will be
 *              declared by the macro.
 */
#  define ENV_HOME_SET(_env, _dir, _meta) \
   char _meta [PATH_MAX + 128]; \
   if ((getuid() != geteuid()) || (!(s = getenv(_env)))) { \
      FATAL_SNPRINTF(_meta, "vpath string '%s' truncated - fatal", "%s/"_dir, home); \
      s = _meta; \
   } \
   (&user)->_meta = s;
# else
/**
 * @internal
 * @def ENV_HOME_SET
 * @brief Sets a user directory path based on XDG environment variables or defaults (fallback for systems without geteuid).
 *
 * This macro retrieves a path from a specified XDG environment variable.
 * If the variable is not set, it defaults to a path constructed by appending
 * `_dir` to the user's home directory.
 * The resulting path is stored in the `_meta` variable. This version is used
 * when `geteuid` is not available, so it doesn't check for UID/EUID mismatch.
 *
 * @param _env The XDG environment variable to check (e.g., "XDG_DESKTOP_DIR").
 * @param _dir The default subdirectory name within the home directory if `_env` is not set.
 * @param _meta The variable name (member of Eina_Vpath_Interface_User) to
 *              store the resulting path. A char array with this name will be
 *              declared by the macro.
 */
#  define ENV_HOME_SET(_env, _dir, _meta) \
   char _meta [PATH_MAX + 128]; \
   if (!(s = getenv(_env))) { \
      FATAL_SNPRINTF(_meta, "vpath string '%s' truncated - fatal", "%s/"_dir, home); \
      s = _meta; \
   } \
   (&user)->_meta = s;
# endif
   // $XDG_DESKTOP_DIR="$HOME/Desktop"
   ENV_HOME_SET("XDG_DESKTOP_DIR", "Desktop", desktop);
   // $XDG_DOCUMENTS_DIR="$HOME/Documents"
   ENV_HOME_SET("XDG_DOCUMENTS_DIR", "Documents", documents);
   // $XDG_DOWNLOAD_DIR="$HOME/Downloads"
   ENV_HOME_SET("XDG_DOWNLOAD_DIR", "Downloads", downloads);
   // $XDG_MUSIC_DIR="$HOME/Music"
   ENV_HOME_SET("XDG_MUSIC_DIR", "Music", music);
   // $XDG_PICTURES_DIR="$HOME/Pictures"
   ENV_HOME_SET("XDG_PICTURES_DIR", "Pictures", pictures);
   // $XDG_PUBLICSHARE_DIR="$HOME/Public"
   ENV_HOME_SET("XDG_PUBLICSHARE_DIR", "Public", pub);
   // $XDG_TEMPLATES_DIR="$HOME/.Templates"
   ENV_HOME_SET("XDG_TEMPLATES_DIR", "Templates", templates);
   // $XDG_VIDEOS_DIR="$HOME/Videos"
   ENV_HOME_SET("XDG_VIDEOS_DIR", "Videos", videos);
   // $XDG_DATA_HOME defines the base directory relative to which user
   //   specific data files should be stored. If $XDG_DATA_HOME is either
   //   not set or empty, a default equal to $HOME/.local/share should be
   //   used.
   ENV_HOME_SET("XDG_DATA_HOME", ".local/share", data);
   ENV_HOME_SET("XDG_TMP_HOME", ".local/tmp", tmp);
   // $XDG_CONFIG_HOME defines the base directory relative to which user
   //   specific configuration files should be stored. If $XDG_CONFIG_HOME
   //   is either not set or empty, a default equal to $HOME/.config should
   //   be used.
   ENV_HOME_SET("XDG_CONFIG_HOME", ".config", config);
   // $XDG_CACHE_HOME defines the base directory relative to which
   //   user specific non-essential data files should be stored. If
   //   $XDG_CACHE_HOME is either not set or empty, a default equal to
   //   $HOME/.cache should be used.
   ENV_HOME_SET("XDG_CACHE_HOME", ".cache", cache);

# if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if ((getuid() != geteuid()) || (!(s = getenv("XDG_RUNTIME_DIR"))))
# else
   if (!(s = getenv("XDG_RUNTIME_DIR")))
# endif
     user.run = NULL;
   else
     user.run = s;
#endif /* _WIN32 */

   eina_vpath_interface_user_set(&user);
}
