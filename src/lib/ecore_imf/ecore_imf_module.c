#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <Ecore.h>
#include <ecore_private.h>

#include "Ecore_IMF.h"
#include "ecore_imf_private.h"

#include "../../static_libs/buildsystem/buildsystem.h"

static void _ecore_imf_module_free(Ecore_IMF_Module *module);
static int _ecore_imf_modules_exists(const char *ctx_id);

/**
 * @internal
 * @brief Structure used to select IMF modules based on a criteria.
 * This structure is used internally for filtering modules.
 */
typedef struct _Ecore_IMF_Selector
{
   const char *toselect; /**< The string criteria to select by (e.g., canvas type). */
   void       *selected; /**< A pointer to an Eina_List where selected items (e.g. module IDs) are stored. */
} Ecore_IMF_Selector;

static Eina_Hash *modules = NULL; /**< Hash table storing registered IMF modules, keyed by context ID. */
static Eina_Array *module_list = NULL; /**< Array of loaded Eina_Module instances. */
static Eina_Prefix *pfx = NULL; /**< Prefix for locating module files. */

/**
 * @internal
 * @brief Initializes the Ecore IMF module system.
 * This function discovers and loads available IMF modules.
 * It checks for modules in the build directory if running in tree,
 * otherwise it loads modules based on environment variables or
 * a default order.
 */
void
ecore_imf_module_init(void)
{
   const char *built_modules[] = {
#ifdef BUILD_ECORE_IMF_XIM
      "xim",
#endif
#ifdef BUILD_ECORE_IMF_IBUS
      "ibus",
#endif
#ifdef BUILD_ECORE_IMF_SCIM
      "scim",
#endif
#ifdef BUILD_ECORE_IMF_WAYLAND
      "wayland",
#endif
      NULL
   };
   const char *env;
   char buf[PATH_MAX] = "";

   pfx = eina_prefix_new(NULL, ecore_imf_init,
                         "ECORE_IMF", "ecore_imf", "checkme",
                         PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
#ifdef NEED_RUN_IN_TREE
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        if (getenv("EFL_RUN_IN_TREE"))
          {
             struct stat st;
             snprintf(buf, sizeof(buf), "%s/src/modules/ecore_imf",
                      PACKAGE_BUILD_DIR);
             if (stat(buf, &st) == 0)
               {
                  const char **itr;
                  const char **modules_load;
                  const char *modules_one[2] = { NULL, NULL };

                  modules_load = built_modules;
                  env = getenv("ECORE_IMF_MODULE");
                  if ((env) && (env[0]))
                    {
                       modules_one[0] = env;
                       modules_load = modules_one;
                    }
                  for (itr = modules_load; *itr != NULL; itr++)
                    {
                       bs_mod_dir_get(buf, sizeof(buf), "ecore_imf", *itr);
                       module_list = eina_module_list_get
                         (module_list, buf, EINA_FALSE, NULL, NULL);
                    }

                  if (module_list) eina_module_list_load(module_list);
                  return;
               }
          }
     }
#endif

   env = getenv("ECORE_IMF_MODULE");
#ifdef BUILD_ECORE_IMF_WAYLAND
   // if not set and we are sure we're on wayland....
   if ((!env) && (getenv("WAYLAND_DISPLAY")) && (!getenv("DISPLAY")))
     env = "wayland";
#endif
#ifdef BUILD_ECORE_IMF_XIM
   if ((!env) && (!getenv("WAYLAND_DISPLAY")) && (getenv("DISPLAY")))
     env = "xim";
#endif
   if ((env) && (env[0]))
     {
        const char **itr;
        Eina_Bool ok = EINA_FALSE;

        for (itr = built_modules; *itr != NULL; itr++)
          {
             if (!strcmp(env, *itr))
               {
                  ok = EINA_TRUE;
                  break;
               }
          }
        if (ok)
          {
             Eina_Module *m;

             snprintf(buf, sizeof(buf),
                      "%s/ecore_imf/modules/%s/%s/module" SHARED_LIB_SUFFIX,
                      eina_prefix_lib_get(pfx), env, MODULE_ARCH);
             m = eina_module_new(buf);
             if (m)
               {
                  module_list = eina_array_new(1);
                  if (module_list) eina_array_push(module_list, m);
                  else eina_module_free(m);
               }
          }
     }
   else
     {
        Eina_Module *m;
        const char **itr;

        for (itr = built_modules; *itr != NULL; itr++)
          {
             snprintf(buf, sizeof(buf),
                      "%s/ecore_imf/modules/%s/%s/module" SHARED_LIB_SUFFIX,
                      eina_prefix_lib_get(pfx), *itr, MODULE_ARCH);

             m = eina_module_new(buf);
             if (m)
               {
                  module_list = eina_array_new(1);
                  if (module_list)
                    {
                       eina_array_push(module_list, m);
                       break;
                    }
                  else
                    eina_module_free(m);
               }
          }
     }

   if (module_list) eina_module_list_load(module_list);
}

/**
 * @internal
 * @brief Shuts down the Ecore IMF module system.
 * This function unloads all loaded IMF modules and frees associated resources.
 */
void
ecore_imf_module_shutdown(void)
{
   if (modules)
     {
        eina_hash_free(modules);
        modules = NULL;
     }
   if (module_list)
     {
        eina_module_list_free(module_list);
        eina_array_free(module_list);
        module_list = NULL;
     }

   eina_prefix_free(pfx);
   pfx = NULL;
}

/**
 * @internal
 * @brief Callback function for eina_hash_foreach to collect module data.
 * This function is used to iterate over the `modules` hash and append
 * each Ecore_IMF_Module (cast to int* for historical reasons, but it's a pointer)
 * to the provided Eina_List.
 *
 * @param hash The hash being iterated.
 * @param data The Ecore_IMF_Module pointer (as int*).
 * @param list A pointer to an Eina_List to append the module to.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_hash_module_available_get(const Eina_Hash *hash EINA_UNUSED, int *data, void *list)
{
   *(Eina_List**)list = eina_list_append(*(Eina_List**)list, data);
   return EINA_TRUE;
}

/**
 * @brief Retrieves a list of available (loaded) IMF modules.
 *
 * @return A new Eina_List containing pointers to Ecore_IMF_Module structures
 *         for each available module. The caller is responsible for freeing
 *         this list with eina_list_free() when no longer needed.
 *         The data pointers within the list are owned by the module system
 *         and should not be freed. Returns NULL if no modules are loaded or
 *         an error occurs.
 *
 * Example:
 * @code
 * Eina_List *available_modules, *l;
 * Ecore_IMF_Module *module;
 *
 * available_modules = ecore_imf_module_available_get();
 * EINA_LIST_FOREACH(available_modules, l, module)
 *   {
 *      printf("Available module ID: %s\n", module->info->id);
 *   }
 * eina_list_free(available_modules);
 * @endcode
 */
Eina_List *
ecore_imf_module_available_get(void)
{
   Eina_List *values = NULL;
   Eina_Iterator *it = NULL;

   if (!modules) return NULL;

   it = eina_hash_iterator_data_new(modules);
   if (!it)
     return NULL;

   eina_iterator_foreach(it, EINA_EACH_CB(_hash_module_available_get), &values);
   eina_iterator_free(it);

   return values;
}

/**
 * @brief Retrieves a specific IMF module by its context ID.
 *
 * @param ctx_id The context ID of the module to retrieve (e.g., "xim", "ibus").
 * @return A pointer to the Ecore_IMF_Module structure if found, otherwise NULL.
 *         The returned pointer is owned by the module system and should not be freed.
 */
Ecore_IMF_Module *
ecore_imf_module_get(const char *ctx_id)
{
   if (!modules) return NULL;
   return eina_hash_find(modules, ctx_id);
}

/**
 * @brief Creates an IMF context instance from a module specified by its context ID.
 *
 * @param ctx_id The context ID of the module from which to create a context
 *               (e.g., "xim", "ibus").
 * @return A pointer to the newly created Ecore_IMF_Context if successful,
 *         otherwise NULL. The caller is responsible for freeing this context
 *         using ecore_imf_context_del() when no longer needed.
 */
Ecore_IMF_Context *
ecore_imf_module_context_create(const char *ctx_id)
{
   Ecore_IMF_Module *module;
   Ecore_IMF_Context *ctx = NULL;

   if (!modules) return NULL;
   module = eina_hash_find(modules, ctx_id);
   if (module)
     {
        if (!(ctx = module->create())) return NULL;
        if (!ECORE_MAGIC_CHECK(ctx, ECORE_MAGIC_CONTEXT))
          {
             ECORE_MAGIC_FAIL(ctx, ECORE_MAGIC_CONTEXT,
                              "ecore_imf_module_context_create");
             return NULL;
          }
        ctx->module = module;
     }
   return ctx;
}

/**
 * @internal
 * @brief Callback function for eina_hash_foreach to collect module context IDs.
 * This function is used to iterate over the `modules` hash and append
 * each module's context ID (the key of the hash entry) to the provided Eina_List.
 *
 * @param hash The hash being iterated.
 * @param key The context ID string (key of the hash entry).
 * @param list A pointer to an Eina_List to append the context ID to.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_hash_ids_get(const Eina_Hash *hash EINA_UNUSED, const char *key, void *list)
{
   *(Eina_List**)list = eina_list_append(*(Eina_List**)list, key);
   return EINA_TRUE;
}

/**
 * @brief Retrieves a list of all available IMF context IDs.
 *
 * These IDs can be used with ecore_imf_context_new() or
 * ecore_imf_module_context_create().
 *
 * @return A new Eina_List containing const char* strings of the context IDs.
 *         The caller is responsible for freeing this list with eina_list_free()
 *         when no longer needed. The strings themselves are owned by the
 *         module system and should not be freed or modified.
 *         Returns NULL if no modules are loaded or an error occurs.
 *
 * Example:
 * @code
 * Eina_List *context_ids, *l;
 * const char *id;
 *
 * context_ids = ecore_imf_module_context_ids_get();
 * EINA_LIST_FOREACH(context_ids, l, id)
 *   {
 *      printf("Available context ID: %s\n", id);
 *   }
 * eina_list_free(context_ids);
 * @endcode
 */
Eina_List *
ecore_imf_module_context_ids_get(void)
{
   Eina_List *l = NULL;
   Eina_Iterator *it = NULL;

   if (!modules) return NULL;

   it = eina_hash_iterator_key_new(modules);
   if (!it)
     return NULL;

   eina_iterator_foreach(it, EINA_EACH_CB(_hash_ids_get), &l);
   eina_iterator_free(it);

   return l;
}

/**
 * @internal
 * @brief Callback function for eina_hash_foreach to collect module context IDs
 *        that match a specific canvas type.
 *
 * This function iterates over the `modules` hash. For each module, it checks
 * if its `canvas_type` (from `module->info->canvas_type`) matches the
 * `toselect` field in the `Ecore_IMF_Selector` structure passed via `fdata`.
 * If they match, the module's ID (`module->info->id`) is appended to the
 * `selected` list within the `Ecore_IMF_Selector`.
 *
 * @param hash The hash being iterated.
 * @param data A pointer to an Ecore_IMF_Module (the value of the hash entry).
 * @param fdata A pointer to an Ecore_IMF_Selector structure.
 *              `fdata->toselect` contains the canvas type string to match.
 *              `fdata->selected` is a pointer to an Eina_List where matching
 *              module IDs (const char *) will be appended.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_hash_ids_by_canvas_type_get(const Eina_Hash *hash EINA_UNUSED, void *data, void *fdata)
{
   Ecore_IMF_Module *module = data;
   Ecore_IMF_Selector *selector = fdata;

   if (!strcmp(module->info->canvas_type, selector->toselect))
     selector->selected = eina_list_append(selector->selected, (void *)module->info->id);

   return EINA_TRUE;
}

/**
 * @brief Retrieves a list of IMF context IDs that support a specific canvas type.
 *
 * @param canvas_type The canvas type string to filter by (e.g., "evas", "wayland").
 *                    If NULL, this function behaves like ecore_imf_module_context_ids_get()
 *                    and returns all available context IDs.
 * @return A new Eina_List containing const char* strings of the context IDs
 *         that support the specified canvas type. The caller is responsible
 *         for freeing this list with eina_list_free() when no longer needed.
 *         The strings themselves are owned by the module system and should not
 *         be freed or modified. Returns NULL if no modules are loaded, no modules
 *         match the canvas type, or an error occurs.
 *
 * Example:
 * @code
 * Eina_List *wayland_context_ids, *l;
 * const char *id;
 *
 * wayland_context_ids = ecore_imf_module_context_ids_by_canvas_type_get("wayland");
 * if (wayland_context_ids)
 *   {
 *      EINA_LIST_FOREACH(wayland_context_ids, l, id)
 *        {
 *           printf("Wayland-compatible context ID: %s\n", id);
 *        }
 *      eina_list_free(wayland_context_ids);
 *   }
 * @endcode
 */
Eina_List *
ecore_imf_module_context_ids_by_canvas_type_get(const char *canvas_type)
{
   Ecore_IMF_Selector selector;
   Eina_List *values = NULL;
   Eina_Iterator *it = NULL;

   if (!modules) return NULL;

   if (!canvas_type)
     return ecore_imf_module_context_ids_get();

   it = eina_hash_iterator_data_new(modules);
   if (!it)
     return NULL;

   selector.toselect = canvas_type;
   selector.selected = values;
   eina_iterator_foreach(it, EINA_EACH_CB(_hash_ids_by_canvas_type_get), &selector);
   eina_iterator_free(it);

   return values;
}

/**
 * @brief Registers an IMF module with the Ecore IMF system.
 * @since 1.1
 *
 * This function is typically called by an IMF module itself during its
 * initialization (e.g., in its `module_open` function if it's a dynamic module,
 * or by a static linking mechanism).
 *
 * @param info A pointer to an Ecore_IMF_Context_Info structure describing the module.
 *             This structure contains the ID, name, and canvas type of the module.
 *             The `info` pointer must remain valid for the lifetime of the module registration.
 * @param imf_module_create A function pointer to the module's context creation function.
 *                          This function will be called to create new IMF context instances.
 *                          It should return a new Ecore_IMF_Context or NULL on failure.
 * @param imf_module_exit A function pointer to the module's exit/cleanup function.
 *                        This function is called when the module is being freed.
 *                        It can be NULL if no specific cleanup is needed by the module itself
 *                        beyond what _ecore_imf_module_free handles.
 *
 * Example (typically within a module's own code):
 * @code
 * // In my_imf_module.c
 * static const Ecore_IMF_Context_Info my_module_info = {
 *   "my_imf",    // id
 *   "My IMF",    // name
 *   "evas",      // canvas_type
 *   NULL,        // default_id (usually NULL, Ecore IMF handles selection)
 *   EINA_FALSE   // needs_preedit_set (module specific)
 * };
 *
 * static Ecore_IMF_Context *my_context_create(void) { ... return new_context; }
 * static Ecore_IMF_Context *my_context_exit(void) { ... cleanup; return NULL; } // Or just void
 *
 * Eina_Bool my_module_init(void) // Or module_open for dynamic modules
 * {
 *    ecore_imf_module_register(&my_module_info, my_context_create, my_context_exit);
 *    return EINA_TRUE;
 * }
 * @endcode
 */
EAPI void
ecore_imf_module_register(const Ecore_IMF_Context_Info *info,
                          Ecore_IMF_Context *(*imf_module_create)(void),
                          Ecore_IMF_Context *(*imf_module_exit)(void))
{
   Ecore_IMF_Module *module;

   if (!info || _ecore_imf_modules_exists(info->id)) return;

   if (!modules)
     modules = eina_hash_string_superfast_new(EINA_FREE_CB(_ecore_imf_module_free));

   module = malloc(sizeof(Ecore_IMF_Module));
   EINA_SAFETY_ON_NULL_RETURN(module);

   module->info = info;
   /* cache imf_module_create as it may be used several times */
   module->create = imf_module_create;
   module->exit = imf_module_exit;

   eina_hash_add(modules, info->id, module);
}

/**
 * @internal
 * @brief Frees an Ecore_IMF_Module structure.
 * This function is used as a callback for eina_hash_free when the `modules`
 * hash is destroyed, or when a module is explicitly unregistered (if such
 * functionality existed). It calls the module's exit function, if provided,
 * and then frees the module structure itself.
 *
 * @param module The Ecore_IMF_Module to free.
 */
static void
_ecore_imf_module_free(Ecore_IMF_Module *module)
{
   if (module->exit) module->exit();
   free(module);
}

/**
 * @internal
 * @brief Checks if an IMF module with the given context ID is already registered.
 *
 * @param ctx_id The context ID to check.
 * @return 1 if a module with the given ID exists, 0 otherwise.
 */
static int
_ecore_imf_modules_exists(const char *ctx_id)
{
   if (!modules) return 0;
   if (!ctx_id) return 0;

   if (eina_hash_find(modules, ctx_id))
     return 1;

   return 0;
}
