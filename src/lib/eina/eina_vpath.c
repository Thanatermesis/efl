#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#else
# include <pwd.h>
#endif

#include <Eina.h>

#include "eina_internal.h"
#include "eina_private.h"

/**
 * @internal
 * @brief Hash table to store vpath key-value pairs.
 * Keys are vpath identifiers like "home", "tmp", "app.dir", etc.
 * Values are the corresponding real paths (Eina_Stringshare *).
 */
static Eina_Hash *vpath_data = NULL;

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_eina_vpath_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_vpath_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eina_vpath_log_dom, __VA_ARGS__)

/**
 * @internal
 * @brief Log domain for eina_vpath.
 * Used for logging messages related to vpath operations.
 */
static int _eina_vpath_log_dom = -1;

/**
 * @internal
 * @brief Adds a key-value pair to the vpath_data hash.
 * The value is added as an Eina_Stringshare.
 * @param key The vpath key (e.g., "home", "tmp").
 * @param value The real path corresponding to the key.
 */
static inline void
_eina_vpath_data_add(const char *key, const char *value)
{
   eina_hash_add(vpath_data, key, eina_stringshare_add(value));
}

/**
 * @internal
 * @brief Retrieves a value (real path) from the vpath_data hash.
 * @param key The vpath key to look up.
 * @return The Eina_Stringshare* for the real path if found, otherwise NULL.
 */
static inline Eina_Stringshare*
_eina_vpath_data_get(const char *key)
{
   return eina_hash_find(vpath_data, key);
}

/**
 * @internal
 * @brief Creates a fallback runtime directory if XDG_RUNTIME_DIR is not set.
 * This typically creates a directory like "$HOME/.run".
 * It performs several checks for existence, type (must be a directory),
 * and ownership. If any check fails or creation fails, it aborts.
 * @param home The user's home directory path.
 * @return A newly allocated string containing the path to the runtime directory.
 *         The caller is responsible for freeing this string.
 */
static char *
_fallback_runtime_dir(const char *home)
{
   char buf[PATH_MAX];
   struct stat st;
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   uid_t uid = getuid();

   if (setuid(geteuid()) != 0)
      {
         fprintf(stderr,
                 "FATAL: Cannot setuid - errno=%i\n",
                 errno);
         abort();
      }
#endif
   // fallback - make ~/.run
   snprintf(buf, sizeof(buf), "%s/.run", home);
   if (!!mkdir(buf,  S_IRUSR | S_IWUSR | S_IXUSR))
     {
        if (errno == EEXIST)
          {
             if (stat(buf, &st) == 0)
               {
                  // some sanity checks - but not for security
                  if (!(st.st_mode & S_IFDIR))
                    {
                       // fatal - exists but is not a dir
                       fprintf(stderr,
                               "FATAL: run dir '%s' exists but not a dir\n",
                               buf);
                       abort();
                    }
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
                  if (st.st_uid != geteuid())
                    {
                       // fatal - run dir doesn't belong to user
                       fprintf(stderr,
                               "FATAL: run dir '%s' not owned by uid %i\n",
                               buf, (int)geteuid());
                       abort();
                    }
#endif
               }
             else
               {
                  // fatal - we cant create our run dir in ~/
                  fprintf(stderr,
                          "FATAL: Cannot verify run dir '%s' errno=%i\n",
                          buf, errno);
                  abort();
               }
          }
        else
          {
             // fatal - we cant create our run dir in ~/
             fprintf(stderr,
                     "FATAL: Cannot create run dir '%s' - errno=%i\n",
                     buf, errno);
             abort();
          }
     }
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
  if (setreuid(uid, geteuid()) != 0)
     {
        fprintf(stderr,
                "FATAL: Cannot setreuid - errno=%i\n",
                errno);
        abort();
     }
#endif

   return strdup(buf);
}

/**
 * @internal
 * @brief Provides a fallback home directory path if the standard one cannot be determined.
 * On systems with geteuid(), it tries to create/use "/tmp/<uid>".
 * If that fails or geteuid() is not available, it falls back to "/tmp" or ultimately "/".
 * @return A newly allocated string containing the fallback home directory path.
 *         The caller is responsible for freeing this string.
 */
static char *
_fallback_home_dir()
{
   char buf[PATH_MAX];
   /* Windows does not have getuid(), but home can't be NULL */
#ifdef HAVE_GETEUID
   uid_t uid = geteuid();
   struct stat st;

   snprintf(buf, sizeof(buf), "/tmp/%i", (int)uid);
   if (mkdir(buf,  S_IRUSR | S_IWUSR | S_IXUSR) < 0)
     {
        if (errno != EEXIST)
          {
             if (stat("/tmp", &st) == 0)
               snprintf(buf, sizeof(buf), "/tmp");
             else
               snprintf(buf, sizeof(buf), "/");
          }
     }
   if (stat(buf, &st) != 0)
     {
        if (stat("/tmp", &st) == 0)
          snprintf(buf, sizeof(buf), "/tmp");
        else
          snprintf(buf, sizeof(buf), "/");
     }
#else
   snprintf(buf, sizeof(buf), "/");
#endif
   return strdup(buf);
}

/**
 * @internal
 * @brief Initializes system-level vpath entries like "home" and "tmp".
 * It retrieves these paths from the environment or uses fallbacks if necessary,
 * then adds them to the vpath_data hash.
 */
static void
_eina_vpath_interface_sys_init(void)
{
   const char *home, *tmp;

   // $HOME / ~/ etc.
   home = eina_environment_home_get();
   if (!home)
     {
        char *home2 = _fallback_home_dir();
        _eina_vpath_data_add("home", home2);
        free(home2);
     }
   else
     _eina_vpath_data_add("home", home);

   // tmp dir - system wide
   tmp = eina_environment_tmp_get();
   _eina_vpath_data_add("tmp", tmp);
}

/**
 * @internal
 * @brief Initializes the Eina vpath system.
 * This function sets up the internal hash table for vpath storage,
 * initializes system-specific vpath entries (like home, tmp),
 * initializes XDG environment variables, and registers a log domain.
 * @return EINA_TRUE on success, EINA_FALSE on failure (though currently always returns EINA_TRUE).
 */
Eina_Bool
eina_vpath_init(void)
{
   vpath_data = eina_hash_string_superfast_new((Eina_Free_Cb)eina_stringshare_del);

   _eina_vpath_interface_sys_init();
   eina_xdg_env_init();

   _eina_vpath_log_dom = eina_log_domain_register("vpath", "cyan");
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Eina vpath system.
 * This function frees the internal hash table used for vpath storage
 * and unregisters the log domain.
 * @return EINA_TRUE on success, EINA_FALSE on failure (though currently always returns EINA_TRUE).
 */
Eina_Bool
eina_vpath_shutdown(void)
{
   eina_hash_free(vpath_data);
   vpath_data = NULL;
   eina_log_domain_unregister(_eina_vpath_log_dom);
   _eina_vpath_log_dom = -1;
   return EINA_TRUE;
}

#ifdef HAVE_GETPWENT
/**
 * @internal
 * @brief Fetches the home directory of a specified user.
 * This function is only available if HAVE_GETPWENT is defined.
 * It uses getpwnam() to look up the user.
 * @param[out] str Pointer to a char* that will be set to the user's home directory (pw_dir).
 *                 This pointer is not allocated by this function but points into the passwd struct.
 * @param name The username to look up.
 * @param error A string to include in error messages if the user is not found.
 * @return EINA_TRUE if the user is found and home directory is retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
_fetch_user_homedir(char **str, const char *name, const char *error)
{
  *str = NULL;
  struct passwd *pwent;

  pwent = getpwnam(name);
  if (!pwent)
    {
       ERR("User %s not found\nThe string was: %s", name, error);
       return EINA_FALSE;
    }
  *str = pwent->pw_dir;

  return EINA_TRUE;
}
#endif

/**
 * @internal
 * @brief Core logic for resolving a vpath string into a real path.
 * This function handles different vpath syntaxes:
 * - `~/...`: Current user's home directory.
 * - `~username/...`: Specified user's home directory (if HAVE_GETPWENT).
 * - `(:key:)/...` or `${key}/...`: Vpath key lookup (e.g., "(:tmp:)/file").
 * - Absolute or relative paths: Passed through as is.
 *
 * @param path The vpath string to resolve.
 * @param[out] str Buffer to store the resolved path.
 * @param size Size of the output buffer `str`.
 * @return The number of characters written to `str` (excluding null terminator)
 *         on success, or 0 on failure or if the path is invalid.
 */
static int
_eina_vpath_resolve(const char *path, char *str, size_t size)
{
   if (path[0] == '~')
     {
        char *home = NULL;
        // ~/ <- home directory
        if (path[1] == '/')
          {
             home = eina_hash_find(vpath_data, "home");
             path ++;
          }
        // ~username/ <- homedir of user "username"
        else
          {
#ifndef HAVE_GETPWENT
             ERR("User fetching is disabled on this system\nThe string was: %s", path);
             return 0;
#else
             const char *p;
             char *name;

             for (p = path + 1; *p; p++)
               {
                  if (*p =='/') break;
               }
             name = alloca(p - path);
             strncpy(name, path + 1, p - path - 1);
             name[p - path - 1] = 0;

             if (!_fetch_user_homedir(&home, name, path))
               return 0;
             path = p;
#endif
           }
         if (home)
           {
              return snprintf(str, size, "%s%s", home, path);
           }
     }
   // (:xxx:)/* ... <- meta hash table
   else if (((path[0] == '(') && (path[1] == ':')) ||
            ((path[0] == '$') && (path[1] == '{')))
     {
        const char *p, *end, *meta;
        const char *msg_start, *msg_end;
        char *name;
        int offset;
        Eina_Bool found = EINA_FALSE;

        if (path[0] == '(')
          {
             end = p = strstr(path + 2, ":)");
             offset = 2;
             msg_start = "(:";
             msg_end = ":)";
          }
        else
          {
             end = p = strchr(path + 2, '}');
             offset = 1;
             msg_start = "${";
             msg_end = "}";
          }
        if (p) found = EINA_TRUE;
        p += offset;

        if (!found)
          {
             ERR("'%s' Needs to have a matching '%s'\nThe string was: %s", msg_start, msg_end, path);
             return 0;
          }

        if (*p != '/')
          {
             ERR("A / is expected after '%s'\nThe string was: %s", msg_end, path);
             return 0;
          }

        if (found)
          {
             name = alloca(end - path);
             strncpy(name, path + 2, end - path - offset);
             name[end - path - 2] = 0;
             meta = _eina_vpath_data_get(name);
             if (meta)
               {
                  return snprintf(str, size, "%s%s", meta, end + offset);
               }
             else
               {
                  ERR("Meta key '%s' was not registered!\nThe string was: %s", name, path);
                  return 0;
               }
          }
     }
   //just return the path, since we assume that this is a normal path
   else
     {
        return snprintf(str, size, "%s", path);
     }
   str[0] = '\0';
   return 0;
}

/**
 * @brief Resolves a virtual path to a real, absolute path.
 *
 * This function takes a vpath string (e.g., "(:home:)/file.txt", "~/docs")
 * and converts it into a canonical file system path. The returned string
 * is dynamically allocated and must be freed by the caller using free().
 *
 * @param path The virtual path string to resolve.
 * @return A newly allocated string containing the resolved path,
 *         or NULL if the path is NULL or resolution fails.
 */
EINA_API char *
eina_vpath_resolve(const char* path)
{
   char buf[PATH_MAX];
   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   if (_eina_vpath_resolve(path, buf, sizeof(buf)) > 0)
     return strdup(buf);
   return NULL;
}

/**
 * @brief Resolves a virtual path formatted string into a real path, storing it in a provided buffer.
 *
 * This function works like snprintf, but first resolves any vpath components
 * within the formatted string. For example, if format is "(:tmp:)/%s" and an argument is "myfile",
 * it will resolve "(:tmp:)/myfile" into something like "/tmp/myfile".
 *
 * @param str Output buffer to store the resolved path.
 * @param size Size of the output buffer `str`.
 * @param format A printf-style format string that may contain vpath specifiers.
 * @param ... Variable arguments for the format string.
 * @return The number of characters that would have been written if `size` were
 *         sufficiently large (excluding the null terminator), similar to snprintf.
 *         Returns 0 or a negative value on error (e.g., invalid vpath syntax).
 */
EINA_API int
eina_vpath_resolve_snprintf(char *str, size_t size, const char *format, ...)
{
   va_list args;
   char *path;
   int r;

   // XXX: implement parse of path then look up in hash if not just create
   // object where path and result are the same and return that with
   // path set and result set to resolved path - return obj handler calls
   // "do" on object to get the result inside fetched or failed callback.
   // if it's a url then we need a new classs that overrides the do and
   // begins a fetch and on finish calls the event cb or when wait is called
   /* FIXME: not working for WIndows */
   // /* <- full path

   path = alloca(size + 1);

   va_start(args, format);
   vsnprintf(path, size, format, args);
   va_end(args);

   r = _eina_vpath_resolve(path, str, size);
   if (r > 0) return r;

   ERR("The path has to start with either '~/' or '(:NAME:)/' or be a normal path \nThe string was: %s", path);

   return 0;
}

/**
 * @brief Sets application-specific vpath entries.
 *
 * This function registers several vpath keys related to application directories,
 * based on the application's domain and prefix. These keys include:
 * - "app.dir": Application installation directory (from eina_prefix_get()).
 * - "app.bin": Application binaries directory.
 * - "app.lib": Application libraries directory.
 * - "app.data": Application shared data directory.
 * - "app.locale": Application locale data directory.
 * - "app.config": Application-specific user configuration directory (e.g., ~/.config/app_domain).
 * - "app.cache": Application-specific user cache directory (e.g., ~/.cache/app_domain).
 * - "app.local": Application-specific user local data directory (e.g., ~/.local/share/app_domain).
 * - "app.tmp": Application-specific user temporary directory (e.g., ~/.local/tmp/app_domain).
 *
 * @param app_domain The application's domain name (e.g., "my_app"). Used to construct user-specific paths.
 * @param app_pfx An Eina_Prefix object representing the application's installation paths.
 */
EINA_API void
eina_vpath_interface_app_set(const char *app_domain, Eina_Prefix *app_pfx)
{
   char buf[PATH_MAX];

   EINA_SAFETY_ON_NULL_RETURN(app_domain);
   EINA_SAFETY_ON_NULL_RETURN(app_pfx);

   _eina_vpath_data_add("app.dir", eina_prefix_get(app_pfx));
   _eina_vpath_data_add("app.bin", eina_prefix_bin_get(app_pfx));
   _eina_vpath_data_add("app.lib", eina_prefix_lib_get(app_pfx));
   _eina_vpath_data_add("app.data", eina_prefix_data_get(app_pfx));
   _eina_vpath_data_add("app.locale", eina_prefix_locale_get(app_pfx));
   snprintf(buf, sizeof(buf), "%s/%s",
            _eina_vpath_data_get("usr.config"), app_domain);
   _eina_vpath_data_add("app.config", buf);
   snprintf(buf, sizeof(buf), "%s/%s",
            _eina_vpath_data_get("usr.cache"), app_domain);
   _eina_vpath_data_add("app.cache", buf);
   snprintf(buf, sizeof(buf), "%s/%s",
            _eina_vpath_data_get("usr.data"), app_domain);
   _eina_vpath_data_add("app.local", buf);
   snprintf(buf, sizeof(buf), "%s/%s",
            _eina_vpath_data_get("usr.tmp"), app_domain);
   _eina_vpath_data_add("app.tmp", buf);
}

/**
 * @brief Sets user-specific vpath entries based on XDG user directories or fallbacks.
 *
 * This function registers vpath keys for standard user directories like "desktop",
 * "documents", "downloads", "music", "pictures", "public", "templates", "videos",
 * as well as XDG base directories like "data", "config", "cache", "run", and "tmp".
 *
 * If `user->run` is NULL, it attempts to determine a fallback runtime directory.
 *
 * The paths are taken from the `user` struct, which should be populated with
 * paths to these standard user directories (e.g., from XDG lookups).
 *
 * Registered keys (prefixed with "usr."):
 * - "usr.desktop"
 * - "usr.documents"
 * - "usr.downloads"
 * - "usr.music"
 * - "usr.pictures"
 * - "usr.public" (from user->pub)
 * - "usr.templates"
 * - "usr.videos"
 * - "usr.data"
 * - "usr.config"
 * - "usr.cache"
 * - "usr.run"
 * - "usr.tmp"
 *
 * @param user Pointer to an Eina_Vpath_Interface_User struct containing paths to user directories.
 */
EINA_API void
eina_vpath_interface_user_set(Eina_Vpath_Interface_User *user)
{
   Eina_Bool free_run = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN(user);

   if (!user->run)
     {
        user->run = _fallback_runtime_dir(_eina_vpath_data_get("home"));
        free_run = EINA_TRUE;
     }

#define ADD(a) _eina_vpath_data_add("usr." #a , user->a)
   ADD(desktop);
   ADD(documents);
   ADD(downloads);
   ADD(music);
   ADD(pictures);
// should be public ain path string
//   ADD(pub);
   _eina_vpath_data_add("usr.public", user->pub);
   ADD(templates);
   ADD(videos);
   ADD(data);
   ADD(config);
   ADD(cache);
   ADD(run);
   ADD(tmp);
#undef ADD

   if (free_run)
     free((char *)user->run);
}
