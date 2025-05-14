#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* TODO: add no_display check, as we might want only displayable items */

#include <ctype.h>

#include <Ecore_File.h>

/* define macros and variable for using the eina logging system  */
#define EFREET_MODULE_LOG_DOM _efreet_utils_log_dom
static int _efreet_utils_log_dom = -1;

#include "Efreet.h"
#include "efreet_private.h"

/**
 * @internal
 * @brief Checks if a given path is within one of the Efreet default directories for a specific section.
 * @param section The FDO section (e.g., "applications").
 * @param path The absolute path to check.
 * @return The base directory string (owned by Eina_Stringshare) if the path is within a default directory, otherwise NULL.
 *         The caller receives a new reference to the stringshare string and is responsible for freeing it if it's not the one returned (meaning path was not in default).
 *         If the path is found within a default directory, the returned string is the matching base directory.
 */
static char *efreet_util_path_in_default(const char *section, const char *path);

/**
 * @internal
 * @brief Matches a string against a glob pattern.
 * @param str The string to match.
 * @param glob The glob pattern.
 * @return 1 if the string matches the pattern, 0 otherwise.
 */
static int  efreet_util_glob_match(const char *str, const char *glob);

/**
 * @internal
 * @brief Helper function to find .menu files in a specific configuration directory.
 * @param menus An existing Eina_List to append found menu file paths to, or NULL to create a new list.
 * @param config_dir The configuration directory to search within (e.g., "~/.config" or a system config dir).
 * @return An Eina_List of menu file paths (const char *). Each string is a duplicated path and must be freed by the caller.
 *         The list itself must be freed using eina_list_free().
 */
static Eina_List *efreet_util_menus_find_helper(Eina_List *menus, const char *config_dir);

/**
 * @internal
 * @brief Finds a single Efreet_Desktop entry from the cache based on a search key and one or two values.
 * @param search The base name for the cache files (e.g., "name", "generic_name", "startup_wm_class").
 *               This is used to construct cache hash filename like "name_hash".
 * @param what1 The primary value to search for in the cache.
 * @param what2 An optional secondary value to search for if the primary is not found or not applicable (e.g., for wm_class). Can be NULL.
 * @return A pointer to an Efreet_Desktop object if found, otherwise NULL. The desktop entry is refcounted; use efreet_desktop_unref() when done.
 */
static Efreet_Desktop *efreet_util_cache_find(const char *search, const char *what1, const char *what2);

/**
 * @internal
 * @brief Retrieves a list of Efreet_Desktop entries from the cache that match a specific value for a given search key.
 * @param search The base name for the cache files (e.g., "mime_types", "categories").
 *               This is used to construct cache hash filename like "mime_types_hash".
 * @param what The value to search for within the specified cache.
 * @return An Eina_List of Efreet_Desktop objects. The list itself and its contents (Efreet_Desktop pointers)
 *         must be freed by the caller (e.g., EINA_LIST_FREE(list, desktop) efreet_desktop_unref(desktop);).
 *         Desktop entries are refcounted.
 */
static Eina_List *efreet_util_cache_list(const char *search, const char *what);

/**
 * @internal
 * @brief Retrieves a list of Efreet_Desktop entries from the cache where a specific field matches a glob pattern.
 * @param search The base name for the cache files (e.g., "name", "generic_name", "comment").
 *               This is used to construct cache list filename like "name_list" and hash filename "name_hash".
 * @param what The glob pattern to match against the values in the cache. If "*" or NULL, can match all entries depending on implementation.
 * @return An Eina_List of Efreet_Desktop objects. The list itself and its contents (Efreet_Desktop pointers)
 *         must be freed by the caller (e.g., EINA_LIST_FREE(list, desktop) efreet_desktop_unref(desktop);).
 *         Desktop entries are refcounted.
 */
static Eina_List *efreet_util_cache_glob_list(const char *search, const char *what);

static Eina_Lock _lock;

static Eina_Hash *file_id_by_desktop_path = NULL;

static int init = 0;

int
efreet_util_init(void)
{
    if (init++) return init;
    _efreet_utils_log_dom = eina_log_domain_register
      ("efreet_util", EFREET_DEFAULT_LOG_COLOR);
    if (_efreet_utils_log_dom < 0)
    {
        EINA_LOG_ERR("Efreet: Could not create a log domain for efreet_util");
        return 0;
    }

    if (!eina_lock_new(&_lock))
    {
        ERR("Could not create lock");
        goto error;
    }


    file_id_by_desktop_path = eina_hash_string_superfast_new(EINA_FREE_CB(eina_stringshare_del));

    return init;
error:
    eina_log_domain_unregister(_efreet_utils_log_dom);
    _efreet_utils_log_dom = -1;
    return 0;
}

int
efreet_util_shutdown(void)
{
    if (--init) return init;

    eina_lock_free(&_lock);

    eina_log_domain_unregister(_efreet_utils_log_dom);
    _efreet_utils_log_dom = -1;
    IF_FREE_HASH(file_id_by_desktop_path);

    return init;
}

/**
 * @internal
 * @brief Checks if a given path is within one of the Efreet default directories for a specific section.
 *
 * This function iterates through the standard FDO base directories (XDG_DATA_HOME, XDG_DATA_DIRS)
 * combined with the provided @p section (e.g., "applications") to see if the @p path
 * starts with any of these.
 *
 * @param section The FDO section (e.g., "applications", "icons").
 * @param path The absolute path to check.
 * @return If @p path is inside one of the default directories for @p section,
 *         a stringshared pointer to that base directory is returned. The caller
 *         receives a new reference to this stringshare if it's not the one that was matched.
 *         Otherwise, returns NULL.
 *         Example: if path is "/home/user/.local/share/applications/foo.desktop" and section is "applications",
 *         and "/home/user/.local/share/" is XDG_DATA_HOME, this might return "/home/user/.local/share/applications".
 */
static char *
efreet_util_path_in_default(const char *section, const char *path)
{
    Eina_List *dirs;
    char *ret = NULL;
    char *dir;

    dirs = efreet_default_dirs_get(efreet_data_home_get(), efreet_data_dirs_get(),
                                   section);

    EINA_LIST_FREE(dirs, dir)
    {
        if (!strncmp(path, dir, strlen(dir)))
            ret = dir;
        else
            eina_stringshare_del(dir);
    }

    return ret;
}

EAPI const char *
efreet_util_path_to_file_id(const char *path)
{
    size_t len, len2;
    char *tmp, *p;
    char *base;
    const char *file_id;

    EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

    file_id = eina_hash_find(file_id_by_desktop_path, path);
    if (file_id) return file_id;

    base = efreet_util_path_in_default("applications", path);
    if (!base) return NULL;

    len = strlen(base);
    if (strlen(path) <= len)
    {
        eina_stringshare_del(base);
        return NULL;
    }
    if (strncmp(path, base, len))
    {
        eina_stringshare_del(base);
        return NULL;
    }

    len2 = strlen(path + len + 1) + 1;
    tmp = alloca(len2);
    memcpy(tmp, path + len + 1, len2);
    p = tmp;
    while (*p)
    {
        if (*p == '/') *p = '-';
        p++;
    }
    eina_stringshare_del(base);
    file_id = eina_stringshare_add(tmp);
    eina_lock_take(&_lock);
    eina_hash_set(file_id_by_desktop_path, path, (void *)file_id);
    eina_lock_release(&_lock);
    return file_id;
}

EAPI Eina_List *
efreet_util_desktop_mime_list(const char *mime)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(mime, NULL);
    return efreet_util_cache_list("mime_types", mime);
}

EAPI Efreet_Desktop *
efreet_util_desktop_wm_class_find(const char *wmname, const char *wmclass)
{
    EINA_SAFETY_ON_TRUE_RETURN_VAL((!wmname) && (!wmclass), NULL);
    return efreet_util_cache_find("startup_wm_class", wmname, wmclass);
}

EAPI Efreet_Desktop *
efreet_util_desktop_file_id_find(const char *file_id)
{
    Efreet_Cache_Hash *hash;
    Efreet_Desktop *ret = NULL;
    const char *str;

    EINA_SAFETY_ON_NULL_RETURN_VAL(file_id, NULL);

    hash = efreet_cache_util_hash_string("file_id");
    if (!hash) return NULL;
    str = eina_hash_find(hash->hash, file_id);
    if (str)
        ret = efreet_desktop_get(str);
    return ret;
}

/**
 * @internal
 * @brief Extracts the arguments string from a command line.
 *
 * This function parses a command string and returns everything after the first
 * whitespace sequence that is not within quotes. It handles single and double
 * quotes, as well as escaped characters.
 *
 * @param cmd The command string (e.g., "gedit %F", "mycommand -o file --with-space \"spaced arg\"").
 * @return A newly allocated string containing the arguments. If no arguments are found,
 *         an empty string is returned. The caller must free this string.
 *         Returns NULL on allocation failure.
 *         Example: for "gedit %F", it returns "%F". For "mycommand", it returns "".
 */
static char *
efreet_util_cmd_args_get(const char *cmd)
{
   Eina_Strbuf *buf;
   char *args;
   const char *p;
   Eina_Bool in_qout_double = EINA_FALSE;
   Eina_Bool in_qout_single = EINA_FALSE;
   Eina_Bool atargs = EINA_FALSE;
   Eina_Bool ingap = EINA_FALSE;

   buf = eina_strbuf_new();
   if (!buf) return NULL;

   // get the arguments to the command as a string on its own
   for (p = cmd; *p; p++)
     {
        if (!atargs)
          {
             if (in_qout_double)
               {
                  if (*p == '\\')
                    {
                       if (p[1]) p++;
                    }
                  else if (*p == '"') in_qout_double = EINA_FALSE;
               }
             else if (in_qout_single)
               {
                  if (*p == '\\')
                    {
                       if (p[1]) p++;
                    }
                  else if (*p == '\'') in_qout_single = EINA_FALSE;
               }
             else
               {
                  if (*p == '\\')
                    {
                       if (p[1]) p++;
                    }
                  else if (*p == '"') in_qout_double = EINA_TRUE;
                  else if (*p == '\'') in_qout_single = EINA_TRUE;
                  else
                    {
                       if (isspace((unsigned char)(*p)))
                         {
                            atargs = EINA_TRUE;
                            ingap = EINA_TRUE;
                         }
                    }
               }
          }
        else
          {
             if (ingap)
               {
                  if (!isspace((unsigned char)(*p))) ingap = EINA_FALSE;
               }
             if (!ingap) eina_strbuf_append_char(buf, *p);
          }
     }

   args = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   if (!args) return strdup("");
   return args;
}

EAPI Efreet_Desktop *
efreet_util_desktop_exec_find(const char *exec)
{
    Efreet_Cache_Hash *hash = NULL;
    Efreet_Desktop *ret = NULL, *bestret = NULL;
    Efreet_Cache_Array_String *names = NULL;
    unsigned int i;

    EINA_SAFETY_ON_NULL_RETURN_VAL(exec, NULL);

    names = efreet_cache_util_names("exec_list");
    if (!names) return NULL;
    for (i = 0; i < names->array_count; i++)
    {
        const char *file;
        char *exe;
        unsigned int j;
        Efreet_Cache_Array_String *array;

        exe = ecore_file_app_exe_get(names->array[i]);
        if (!exe) continue;
        file = ecore_file_file_get(exe);
        if ((!file) || (strcmp(exec, exe) && strcmp(exec, file)))
        {
            free(exe);
            continue;
        }
        free(exe);

        if (!hash)
            hash = efreet_cache_util_hash_array_string("exec_hash");
        if (!hash) return NULL;
        array = eina_hash_find(hash->hash, names->array[i]);
        if (!array) continue;
        for (j = 0; j < array->array_count; j++)
        {
            ret = efreet_desktop_get(array->array[j]);
            if (ret)
            {
               if (!bestret)
               {
                  bestret = ret;
                  if (bestret->exec && !strcmp(bestret->exec, exec))
                    goto done;
               }
               else
               {
                  if (ret->exec)
                  {
                     // perfect match - best
                     if (!strcmp(ret->exec, exec))
                     {
                        bestret = ret;
                        goto done;
                     }
                     else if (bestret->exec)
                     {
                        char *f1, *f2;

                        f1 = efreet_util_cmd_args_get(ret->exec);
                        f2 = efreet_util_cmd_args_get(bestret->exec);
                        if ((f1) && (f2))
                          {
                             // if this is shorter (less arguments) than best
                             // match so far, thewn this is the best match
                             if (strlen(f1) < strlen(f2))
                               {
                                  bestret = ret;
                               }
                          }
                        free(f1);
                        free(f2);
                     }
                  }
               }
            }
        }
    }
done:
    return bestret;
}

/**
 * @brief Find a desktop by name
 *
 * return value must be freed by efreet_desktop_free
 *
 * @param name the name
 * @return a desktop
 */
EAPI Efreet_Desktop *
efreet_util_desktop_name_find(const char *name)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
    return efreet_util_cache_find("name", name, NULL);
}

EAPI Efreet_Desktop *
efreet_util_desktop_generic_name_find(const char *generic_name)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(generic_name, NULL);
    return efreet_util_cache_find("generic_name", generic_name, NULL);
}

EAPI Eina_List *
efreet_util_desktop_name_glob_list(const char *glob)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(glob, NULL);
    return efreet_util_cache_glob_list("name", glob);
}

EAPI Eina_List *
efreet_util_desktop_exec_glob_list(const char *glob)
{
    Efreet_Cache_Hash *hash = NULL;
    Eina_List *ret = NULL;
    Efreet_Cache_Array_String *names = NULL;
    unsigned int i;

    EINA_SAFETY_ON_NULL_RETURN_VAL(glob, NULL);

    if (!strcmp(glob, "*"))
        glob = NULL;

    names = efreet_cache_util_names("exec_list");
    if (!names) return NULL;
    for (i = 0; i < names->array_count; i++)
    {
        Efreet_Cache_Array_String *array;
        unsigned int j;
        char *exe;
        Efreet_Desktop *desk;

        exe = ecore_file_app_exe_get(names->array[i]);
        if (!exe) continue;
        if (glob && !efreet_util_glob_match(exe, glob))
        {
            free(exe);
            continue;
        }
        free(exe);

        if (!hash)
            hash = efreet_cache_util_hash_array_string("exec_hash");
        if (!hash) return NULL;

        array = eina_hash_find(hash->hash, names->array[i]);
        if (!array) continue;
        for (j = 0; j < array->array_count; j++)
        {
            desk = efreet_desktop_get(array->array[j]);
            if (desk)
                ret = eina_list_append(ret, desk);
        }
    }
    return ret;
}

EAPI Eina_List *
efreet_util_desktop_generic_name_glob_list(const char *glob)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(glob, NULL);
    return efreet_util_cache_glob_list("generic_name", glob);
}

EAPI Eina_List *
efreet_util_desktop_comment_glob_list(const char *glob)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(glob, NULL);
    return efreet_util_cache_glob_list("comment", glob);
}

EAPI Eina_List *
efreet_util_desktop_categories_list(void)
{
    Efreet_Cache_Array_String *array;
    Eina_List *ret = NULL;
    unsigned int i;

    array = efreet_cache_util_names("categories_list");
    if (!array) return NULL;
    for (i = 0; i < array->array_count; i++)
        ret = eina_list_append(ret, array->array[i]);
    return ret;
}

EAPI Eina_List *
efreet_util_desktop_category_list(const char *category)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(category, NULL);
    return efreet_util_cache_list("categories", category);
}

EAPI Eina_List *
efreet_util_desktop_environments_list(void)
{
    Efreet_Cache_Array_String *array;
    Eina_List *ret = NULL;
    unsigned int i;

    array = efreet_cache_util_names("environments_list");
    if (!array) return NULL;
    for (i = 0; i < array->array_count; i++)
        ret = eina_list_append(ret, array->array[i]);
    return ret;
}

/**
 * @internal
 * @brief Matches a string against a glob pattern.
 *
 * This function provides a simple glob matching capability.
 * It handles the '*' wildcard.
 *
 * @param str The string to test.
 * @param glob The glob pattern to match against @p str.
 * @return 1 if @p str matches @p glob, 0 otherwise.
 *         Returns 0 if either @p str or @p glob is NULL.
 *         If @p glob is an empty string, it only matches an empty @p str.
 *         If @p glob is "*", it matches any @p str (including empty).
 */
static int
efreet_util_glob_match(const char *str, const char *glob)
{
    if (!str || !glob)
        return 0;
    if (glob[0] == '\0')
    {
        if (str[0] == '\0') return 1;
        return 0;
    }
    if (!strcmp(glob, "*")) return 1;
    if (eina_fnmatch(glob, str, 0)) return 1;
    return 0;
}

EAPI Eina_List *
efreet_util_menus_find(void)
{
    Eina_List *menus = NULL;
    Eina_List *dirs, *l;
    const char *dir;

    menus = efreet_util_menus_find_helper(menus, efreet_config_home_get());

    dirs = efreet_config_dirs_get();
    EINA_LIST_FOREACH(dirs, l, dir)
        menus = efreet_util_menus_find_helper(menus, dir);

    return menus;
}

/**
 * @internal
 * @brief Helper function to find .menu files in a specific configuration directory.
 *
 * This function scans the "menus" subdirectory of @p config_dir for files
 * ending with the ".menu" extension. Found paths are duplicated and added to
 * the @p menus list.
 *
 * @param menus An Eina_List to which found menu file paths (char *) will be appended.
 *              If NULL, a new list will be created (though current usage passes an existing list).
 * @param config_dir The base configuration directory to search within (e.g., "/home/user/.config", "/etc/xdg").
 *                   The function will look into a "menus" subdirectory of this path.
 * @return The @p menus list with appended menu file paths. Each path string is
 *         newly allocated with strdup() and must be freed by the caller when the list is cleared.
 *         The list itself is managed by the caller.
 */
static Eina_List *
efreet_util_menus_find_helper(Eina_List *menus, const char *config_dir)
{
    Eina_Iterator *it;
    Eina_File_Direct_Info *info;
    char dbuf[PATH_MAX];

    snprintf(dbuf, sizeof(dbuf), "%s/menus", config_dir);
    it = eina_file_stat_ls(dbuf);
    if (!it) return menus;
    EINA_ITERATOR_FOREACH(it, info)
    {
        const char *exten;

        if (info->type == EINA_FILE_DIR) continue;

        exten = strrchr(info->path + info->name_start, '.');
        if (!exten) continue;

        if (strcmp(".menu", exten)) continue;

        menus = eina_list_append(menus, strdup(info->path));
    }
    eina_iterator_free(it);
    return menus;
}

/**
 * @internal
 * @brief Finds a single Efreet_Desktop entry from the cache.
 *
 * This function queries the Efreet cache for a desktop entry. It constructs
 * a cache key (e.g., "name_hash") from @p search and looks up @p what1 (and
 * optionally @p what2) in this hash. The first matching desktop entry found is returned.
 *
 * @param search A string prefix used to determine the cache file, e.g., "name", "generic_name", "startup_wm_class".
 *               This will be used to look for a hash named, for example, "name_hash".
 * @param what1 The primary key to search for in the hash.
 * @param what2 An optional secondary key to search for if @p what1 is not found or if applicable (e.g. for WM_CLASS where name and class can be used). Can be NULL.
 * @return An Efreet_Desktop* if a match is found, otherwise NULL. The returned desktop
 *         is refcounted; use efreet_desktop_unref() when no longer needed.
 */
static Efreet_Desktop *
efreet_util_cache_find(const char *search, const char *what1, const char *what2)
{
    Efreet_Cache_Hash *hash;
    Efreet_Desktop *ret = NULL;
    Efreet_Cache_Array_String *array = NULL;
    char key[256];

    if ((!what1) && (!what2)) return NULL;

    snprintf(key, sizeof(key), "%s_hash", search);
    hash = efreet_cache_util_hash_array_string(key);
    if (!hash) return NULL;
    if (what1)
        array = eina_hash_find(hash->hash, what1);
    if (!array && what2) array = eina_hash_find(hash->hash, what2);
    if (array)
    {
        unsigned int i;

        for (i = 0; i < array->array_count; i++)
        {
            ret = efreet_desktop_get(array->array[i]);
            if (ret) break;
        }
    }
    return ret;
}

/**
 * @internal
 * @brief Retrieves a list of Efreet_Desktop entries from the cache matching a key.
 *
 * This function queries the Efreet cache. It constructs a cache key (e.g., "mime_types_hash")
 * from @p search and looks up @p what in this hash. All desktop entries associated
 * with this key are returned in a list.
 *
 * @param search A string prefix used to determine the cache file, e.g., "mime_types", "categories".
 *               This will be used to look for a hash named, for example, "mime_types_hash".
 * @param what The key to search for in the hash (e.g., a specific MIME type or category name).
 * @return An Eina_List* of Efreet_Desktop* entries. Returns NULL if @p what is NULL or
 *         if the cache cannot be accessed. The caller is responsible for freeing the list
 *         (e.g., using EINA_LIST_FREE and efreet_desktop_unref() for each item).
 *         Each Efreet_Desktop in the list is refcounted.
 *         Example of returned list structure for `search="categories", what="AudioVideo"`:
 *         Eina_List [
 *           Efreet_Desktop* (for app1.desktop that is in AudioVideo),
 *           Efreet_Desktop* (for app2.desktop that is in AudioVideo),
 *           ...
 *         ]
 */
static Eina_List *
efreet_util_cache_list(const char *search, const char *what)
{
    Efreet_Cache_Hash *hash;
    Efreet_Cache_Array_String *array;
    Eina_List *ret = NULL;
    char key[256];

    if (!what) return NULL;

    snprintf(key, sizeof(key), "%s_hash", search);
    hash = efreet_cache_util_hash_array_string(key);
    if (!hash) return NULL;
    array = eina_hash_find(hash->hash, what);
    if (array)
    {
        unsigned int i;
        Efreet_Desktop *desk;

        for (i = 0; i < array->array_count; i++)
        {
            desk = efreet_desktop_get(array->array[i]);
            if (desk)
                ret = eina_list_append(ret, desk);
        }
    }
    return ret;
}

/**
 * @internal
 * @brief Retrieves a list of Efreet_Desktop entries from the cache where a field matches a glob pattern.
 *
 * This function iterates through a list of known values for a field (e.g., all known application names,
 * obtained from a cache like "name_list"). For each value that matches the @p what glob pattern,
 * it then retrieves all associated desktop files from a corresponding hash cache (e.g., "name_hash").
 *
 * @param search A string prefix used to determine the cache files, e.g., "name", "generic_name", "comment".
 *               This will be used to look for a list like "name_list" and a hash like "name_hash".
 * @param what The glob pattern to match against the field values. If "*" or NULL, it may match all entries,
 *             depending on the specific implementation (currently, "*" is treated as match-all by setting `what` to NULL).
 * @return An Eina_List* of Efreet_Desktop* entries. Returns NULL if @p what is NULL (unless it was "*")
 *         or if the cache cannot be accessed. The caller is responsible for freeing the list
 *         (e.g., using EINA_LIST_FREE and efreet_desktop_unref() for each item).
 *         Each Efreet_Desktop in the list is refcounted.
 *         Example of returned list structure for `search="name", what="G*"` (apps starting with G):
 *         Eina_List [
 *           Efreet_Desktop* (for gedit.desktop),
 *           Efreet_Desktop* (for gimp.desktop),
 *           ...
 *         ]
 */
static Eina_List *
efreet_util_cache_glob_list(const char *search, const char *what)
{
    Efreet_Cache_Hash *hash = NULL;
    Eina_List *ret = NULL;
    Efreet_Cache_Array_String *names = NULL;
    char key[256];
    unsigned int i;

    if (!what) return NULL;
    if (!strcmp(what, "*"))
        what = NULL;

    snprintf(key, sizeof(key), "%s_list", search);
    names = efreet_cache_util_names(key);
    if (!names) return NULL;
    for (i = 0; i < names->array_count; i++)
    {
        Efreet_Cache_Array_String *array;
        unsigned int j;
        Efreet_Desktop *desk;

        if (what && !efreet_util_glob_match(names->array[i], what)) continue;

        if (!hash)
        {
            snprintf(key, sizeof(key), "%s_hash", search);
            hash = efreet_cache_util_hash_array_string(key);
        }
        if (!hash) return NULL;

        array = eina_hash_find(hash->hash, names->array[i]);
        if (!array) continue;
        for (j = 0; j < array->array_count; j++)
        {
            desk = efreet_desktop_get(array->array[j]);
            if (desk)
                ret = eina_list_append(ret, desk);
        }
    }
    return ret;
}

/*
 * Needs EAPI because of helper binaries
 */
/**
 * @brief Frees an Eina_Hash table with a custom data free callback.
 * @param hash The Eina_Hash table to free.
 * @param free_cb The callback function to free the data stored in the hash.
 *
 * Needs EAPI because of helper binaries that might link against efreet
 * and use this utility for cleaning up hash tables they create, ensuring
 * they use the same memory management context if that were relevant, or simply
 * for convenience if they also use Eina.
 */
EAPI void
efreet_hash_free(Eina_Hash *hash, Eina_Free_Cb free_cb)
{
    eina_hash_free_cb_set(hash, free_cb);
    eina_hash_free(hash);
}

