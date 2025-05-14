#include "edje_private.h"

/**
 * @internal
 * @brief Hash table of loaded Edje modules.
 * Key: module name (const char *)
 * Value: Eina_Module *
 */
Eina_Hash *_registered_modules = NULL;

/**
 * @internal
 * @brief List of paths to search for Edje modules.
 * Each element is a (char *).
 */
Eina_List *_modules_paths = NULL;

/**
 * @internal
 * @brief List of found available Edje module names.
 * Each element is a (const char *) (stringshared).
 */
Eina_List *_modules_found = NULL;

#if _WIN32
# define EDJE_MODULE_NAME "module.dll"
#else
# define EDJE_MODULE_NAME "module.so"
#endif

/**
 * @brief Loads an Edje module.
 *
 * This function attempts to load the specified Edje module.
 * It's a wrapper around _edje_module_handle_load().
 *
 * @param module The name of the module to load.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
edje_module_load(const char *module)
{
   if (_edje_module_handle_load(module)) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Loads and registers an Edje module.
 *
 * This function searches for the specified module in the configured module
 * paths. If found, it loads the module and adds it to the
 * _registered_modules hash. It also handles "run-in-tree" scenarios
 * for development.
 *
 * @param module The name of the module to load.
 * @return A pointer to the loaded Eina_Module on success, or NULL on failure.
 */
Eina_Module *
_edje_module_handle_load(const char *module)
{
   const char *path;
   Eina_List *l;
   Eina_Module *em = NULL;
#ifdef NEED_RUN_IN_TREE
   Eina_Bool run_in_tree;
#endif

   EINA_SAFETY_ON_NULL_RETURN_VAL(module, NULL);

   em = (Eina_Module *)eina_hash_find(_registered_modules, module);
   if (em) return em;

#ifdef NEED_RUN_IN_TREE
   run_in_tree = !!getenv("EFL_RUN_IN_TREE");
#endif

   EINA_LIST_FOREACH(_modules_paths, l, path)
     {
        char tmp[PATH_MAX] = "";

#ifdef NEED_RUN_IN_TREE
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
        if (getuid() == geteuid())
#endif
        {
           if (run_in_tree)
             {
                struct stat st;
                snprintf(tmp, sizeof(tmp), "%s/%s/.libs/%s",
                         path, module, EDJE_MODULE_NAME);
                if (stat(tmp, &st) != 0)
                  tmp[0] = '\0';
             }
        }
#endif

        if (tmp[0] == '\0')
          snprintf(tmp, sizeof(tmp), "%s/%s/%s/%s",
                   path, module, MODULE_ARCH, EDJE_MODULE_NAME);

        em = eina_module_new(tmp);
        if (!em) continue;

        if (!eina_module_load(em))
          {
             eina_module_free(em);
             continue;
          }
        if (eina_hash_add(_registered_modules, module, em))
          return em;
     }

   return NULL;
}

/**
 * @internal
 * @brief Frees an Eina_Module.
 *
 * This function is used as a callback for eina_hash_free_cb to free
 * Eina_Module structures stored in the _registered_modules hash.
 *
 * @param mod Pointer to the Eina_Module to free.
 */
static void
module_free(void *mod)
{
   eina_module_free(mod);
}

/**
 * @internal
 * @brief Initializes the Edje module system.
 *
 * This function initializes the _registered_modules hash and populates
 * the _modules_paths list with default search paths for Edje modules.
 * It considers "run-in-tree" development environments and standard
 * installation paths.
 */
void
_edje_module_init(void)
{
   char *paths[2] = { NULL, NULL };
   unsigned int i;
   unsigned int j;

   _registered_modules = eina_hash_string_small_new(module_free);

#ifdef NEED_RUN_IN_TREE
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
   {
      if (getenv("EFL_RUN_IN_TREE"))
        {
           struct stat st;
           const char mp[] = PACKAGE_BUILD_DIR "/src/modules/edje";
           if (stat(mp, &st) == 0)
             {
                _modules_paths = eina_list_append(_modules_paths, strdup(mp));
                return;
             }
        }
   }
#endif

   /* 1. libedje.so/../edje/modules/ */
   paths[0] = eina_module_symbol_path_get(_edje_module_init, "/edje/modules");
   /* 2. PREFIX/edje/modules/ */
   paths[1] = strdup(PACKAGE_LIB_DIR "/edje/modules");

   for (j = 0; j < ((sizeof (paths) / sizeof (char *)) - 1); ++j)
     for (i = j + 1; i < sizeof (paths) / sizeof (char *); ++i)
       if (paths[i] && paths[j] && !strcmp(paths[i], paths[j]))
         {
            free(paths[i]);
            paths[i] = NULL;
         }

   for (i = 0; i < sizeof (paths) / sizeof (char *); ++i)
     if (paths[i])
       _modules_paths = eina_list_append(_modules_paths, paths[i]);
}

/**
 * @internal
 * @brief Shuts down the Edje module system.
 *
 * This function frees all resources associated with the Edje module system,
 * including the _registered_modules hash, the _modules_paths list, and
 * the _modules_found list.
 */
void
_edje_module_shutdown(void)
{
   char *path;

   if (_registered_modules)
     {
        eina_hash_free(_registered_modules);
        _registered_modules = NULL;
     }

   EINA_LIST_FREE(_modules_paths, path)
     free(path);

   EINA_LIST_FREE(_modules_found, path)
     eina_stringshare_del(path);
}

/**
 * @brief Gets a list of available Edje modules.
 *
 * This function scans the configured module search paths for available
 * Edje modules. It checks for the existence of "module.so" (or "module.dll")
 * within subdirectories corresponding to module names and architecture.
 *
 * @note The returned list and its stringshared contents should not be modified
 *       by the caller. The list is owned by the Edje module system and is
 *       valid until the next call to this function or _edje_module_shutdown().
 *
 * @return A const Eina_List of available module names (const char *).
 *         Each string is stringshared. Returns NULL on failure or if no
 *         modules are found.
 *         Example list structure:
 *         ("module_name1", "module_name2", ...)
 */
EAPI const Eina_List *
edje_available_modules_get(void)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *l;
   const char *path;
   Eina_Strbuf *buf;
   Eina_List *result = NULL;

   /* FIXME: Stat each possible dir and check if they did change, before starting a huge round of readdir/stat */
   if (_modules_found)
     {
        EINA_LIST_FREE(_modules_found, path)
          eina_stringshare_del(path);
     }

   buf = eina_strbuf_new();
   EINA_LIST_FOREACH(_modules_paths, l, path)
     {
        it = eina_file_direct_ls(path);

        EINA_ITERATOR_FOREACH(it, info)
          {
             eina_strbuf_append_printf(buf, "%s/%s/" EDJE_MODULE_NAME, info->path, MODULE_ARCH);

             if (ecore_file_exists(eina_strbuf_string_get(buf)))
               result = eina_list_append(result, eina_stringshare_add(info->path + info->name_start));
             eina_strbuf_reset(buf);
          }

        eina_iterator_free(it);
     }
   eina_strbuf_free(buf);

   _modules_found = result;

   return result;
}
