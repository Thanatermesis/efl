#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ecore_file_private.h"

/**
 * @internal
 * @brief List of directories from the PATH environment variable.
 * This list stores stringshared directory paths.
 */
static Eina_List *__ecore_file_path_bin = NULL;

/**
 * @internal
 * @brief Parses a colon-separated list of paths from an environment variable.
 *
 * This function takes an environment variable name (e.g., "PATH"),
 * retrieves its value, and splits it into a list of individual directory paths.
 * Each path is added to the list as a stringshared string.
 * It filters out directories that do not exist using ecore_file_path_dir_exists,
 * which seems counter-intuitive here as ecore_file_path_dir_exists checks against
 * __ecore_file_path_bin which is what this function is trying to populate.
 * This might be a bug or a misunderstanding of ecore_file_path_dir_exists's role
 * in this specific context. Assuming it's meant to check physical existence,
 * but the current implementation checks against the global list.
 *
 * @param env The name of the environment variable (e.g., "PATH").
 * @return A new Eina_List containing stringshared paths, or NULL if the
 *         environment variable is not set or is empty. The caller is
 *         responsible for freeing this list (e.g., using EINA_LIST_FREE
 *         and eina_stringshare_del for each item).
 */
static Eina_List *_ecore_file_path_from_env(const char *env);

/**
 * @brief Initializes the ecore_file_path module.
 *
 * This function populates the internal list of binary paths (__ecore_file_path_bin)
 * by parsing the "PATH" environment variable. It should be called once at
 * application startup before any other ecore_file_path functions are used.
 */
void
ecore_file_path_init(void)
{
   __ecore_file_path_bin = _ecore_file_path_from_env("PATH");
}

/**
 * @brief Shuts down the ecore_file_path module.
 *
 * This function frees the internal list of binary paths (__ecore_file_path_bin)
 * and releases the associated stringshared directory names. It should be called
 * once at application shutdown.
 */
void
ecore_file_path_shutdown(void)
{
   char *dir;

   EINA_LIST_FREE(__ecore_file_path_bin, dir)
     eina_stringshare_del(dir);
}

Eina_List *
_ecore_file_path_from_env(const char *env)
{
   Eina_List *path = NULL;
   char *env_tmp, *env_path, *p, *last;

   env_tmp = getenv(env);
   if (!env_tmp)
     return path;

   env_path = alloca(sizeof(char) * strlen(env_tmp) + 1);
   memset(env_path, 0, strlen(env_tmp));
   strcpy(env_path, env_tmp);
   last = env_path;
   for (p = env_path; *p; p++)
     {
        if (*p == ':')
          *p = '\0';

        if (!*p)
          {
             if (!ecore_file_path_dir_exists(last))
               path = eina_list_append(path, eina_stringshare_add(last));
             last = p + 1;
          }
     }
   if (p > last)
     path = eina_list_append(path, eina_stringshare_add(last));

   return path;
}

/**
 * @brief Checks if a directory exists in the cached PATH.
 *
 * This function iterates through the list of directories obtained from the
 * PATH environment variable (cached in __ecore_file_path_bin) and checks
 * if the given @p in_dir matches any of them.
 *
 * @param in_dir The directory path to check. For example, "/usr/bin".
 * @return @c EINA_TRUE if the directory is found in the cached PATH,
 *         @c EINA_FALSE otherwise, or if @p in_dir is @c NULL.
 */
EAPI Eina_Bool
ecore_file_path_dir_exists(const char *in_dir)
{
   Eina_List *l;
   char *dir;

   if (!in_dir)
     return EINA_FALSE;

   if (!__ecore_file_path_bin) return EINA_FALSE;
   EINA_LIST_FOREACH(__ecore_file_path_bin, l, dir)
     {
        if (!strcmp(dir, in_dir))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Checks if an executable application is installed and accessible.
 *
 * This function determines if a given executable name corresponds to an
 * installed application. It first checks if @p exe is an absolute or
 * relative path and directly executable. If not, it searches for the
 * executable in the directories listed in the PATH environment variable
 * (cached in __ecore_file_path_bin).
 *
 * @param exe The name or path of the executable.
 *            Examples: "ls", "/bin/ls", "./my_app".
 * @return @c EINA_TRUE if the application is found and executable,
 *         @c EINA_FALSE otherwise, or if @p exe is @c NULL.
 */
EAPI Eina_Bool
ecore_file_app_installed(const char *exe)
{
   Eina_List *l;
   char *dir;
   char  buf[PATH_MAX];

   if (!exe) return EINA_FALSE;
   if (((!strncmp(exe, "/", 1)) ||
        (!strncmp(exe, "./", 2)) ||
        (!strncmp(exe, "../", 3))) &&
       ecore_file_can_exec(exe)) return EINA_TRUE;

   EINA_LIST_FOREACH(__ecore_file_path_bin, l, dir)
     {
        snprintf(buf, sizeof(buf), "%s/%s", dir, exe);
        if (ecore_file_can_exec(buf))
          return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Lists all executable applications found in the PATH directories.
 *
 * This function scans all directories specified in the PATH environment
 * variable (cached in __ecore_file_path_bin). For each directory, it lists
 * all files, checks if each file is executable and not a directory, and if so,
 * adds its full path to a list.
 *
 * @return A new Eina_List containing full paths (char *) to all found
 *         executable applications. Each string in the list is allocated
 *         with strdup() and must be freed by the caller, along with the
 *         list itself (e.g., using EINA_LIST_FREE and free() for each item).
 *         Returns @c NULL if no executables are found or if __ecore_file_path_bin
 *         is not initialized.
 *
 * @note The returned list contains dynamically allocated strings.
 *       Example of freeing the list:
 *       <pre>
 *       Eina_List *apps = ecore_file_app_list();
 *       char *app_path;
 *       EINA_LIST_FREE(apps, app_path)
 *         {
 *            free(app_path);
 *         }
 *       </pre>
 */
EAPI Eina_List *
ecore_file_app_list(void)
{
   Eina_List *list = NULL;
   Eina_List *files;
   Eina_List *l;
   char  buf[PATH_MAX], *dir, *exe;

   EINA_LIST_FOREACH(__ecore_file_path_bin, l, dir)
     {
        files = ecore_file_ls(dir);
        EINA_LIST_FREE(files, exe)
               {
                  snprintf(buf, sizeof(buf), "%s/%s", dir, exe);
                  if ((ecore_file_can_exec(buf)) &&
                      (!ecore_file_is_dir(buf)))
               list = eina_list_append(list, strdup(buf));
             free(exe);
          }
     }

   return list;
}
