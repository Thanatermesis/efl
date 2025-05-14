#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "ecore_private.h"

#include "Ecore_Evas.h"
#include "ecore_evas_private.h"
#include <unistd.h>

#include "../../static_libs/buildsystem/buildsystem.h"

/** @internal
 * @brief Hash table of registered Ecore_Evas engines.
 * Key: engine name (const char *), Value: Eina_Module *
 */
static Eina_Hash *_registered_engines = NULL;
/** @internal
 * @brief List of paths (char *) where Ecore_Evas engines are searched.
 */
static Eina_List *_engines_paths = NULL;
/** @internal
 * @brief List of available Ecore_Evas engine names (const char * from eina_stringshare).
 * This list is populated on demand.
 */
static Eina_List *_engines_available = NULL;
/** @internal
 * @brief Pointer to the loaded VNC server Ecore_Evas module.
 */
static Eina_Module *_ecore_evas_vnc = NULL;

#ifdef _WIN32
# define ECORE_EVAS_ENGINE_NAME "module.dll"
#else
# define ECORE_EVAS_ENGINE_NAME "module.so"
#endif

/**
 * @internal
 * @brief Tries to load the VNC server module.
 *
 * This function attempts to load the VNC server module either directly
 * from the given @p prefix if @p use_prefix_only is EINA_TRUE, or by
 * constructing a path relative to the @p prefix, module architecture,
 * and standard engine name.
 *
 * @param prefix The base path or full path to the module.
 * @param use_prefix_only If EINA_TRUE, @p prefix is treated as the full path.
 *                        If EINA_FALSE, a path is constructed using @p prefix.
 * @return A pointer to the loaded Eina_Module on success, or NULL on failure.
 */
static Eina_Module *
_ecore_evas_vnc_server_module_try_load(const char *prefix,
                                       Eina_Bool use_prefix_only)
{
   Eina_Module *m;

   if (use_prefix_only)
     m = eina_module_new(prefix);
   else
     {
        char path[PATH_MAX];

        snprintf(path, sizeof(path), "%s/vnc_server/%s/%s", prefix,
                 MODULE_ARCH, ECORE_EVAS_ENGINE_NAME);
        m = eina_module_new(path);
     }

   if (!m)
     return NULL;
   if (!eina_module_load(m))
     {
        eina_module_free(m);
        _ecore_evas_vnc = NULL;
        return NULL;
     }

   return m;
}

/**
 * @internal
 * @brief Loads the VNC server Ecore_Evas module.
 *
 * This function attempts to load the VNC server module from various
 * potential locations:
 * 1. Using `bs_mod_get` if the user is the same as the effective user (non-setuid).
 * 2. Relative to the path of this function's symbol (`_ecore_evas_vnc_server_module_load`).
 * 3. From `PACKAGE_LIB_DIR/ecore_evas`.
 *
 * It caches the loaded module in `_ecore_evas_vnc`.
 *
 * @return A pointer to the loaded Eina_Module for the VNC server,
 *         or NULL if loading fails.
 */
Eina_Module *
_ecore_evas_vnc_server_module_load(void)
{
   char *prefix;
   char buf[PATH_MAX];

   if (_ecore_evas_vnc)
     return _ecore_evas_vnc;

   if (bs_mod_get(buf, sizeof(buf), "ecore_evas", "vnc_server"))
     {
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
        if (getuid() == geteuid())
#endif
          {
             _ecore_evas_vnc = _ecore_evas_vnc_server_module_try_load(buf,
                                                                      EINA_TRUE);
             if (_ecore_evas_vnc)
               return _ecore_evas_vnc;
          }
     }

   prefix = eina_module_symbol_path_get(_ecore_evas_vnc_server_module_load,
                                        "/ecore_evas");
   _ecore_evas_vnc = _ecore_evas_vnc_server_module_try_load(prefix, EINA_FALSE);
   free(prefix);
   //Last try...
   if (!_ecore_evas_vnc)
     {
        _ecore_evas_vnc = _ecore_evas_vnc_server_module_try_load(PACKAGE_LIB_DIR"/ecore_evas",
                                                                 EINA_FALSE);
        if (!_ecore_evas_vnc)
          ERR("Could not find a valid VNC module to load!");
     }
   return _ecore_evas_vnc;
}

/**
 * @internal
 * @brief Loads a specific Ecore_Evas engine module by name.
 *
 * This function attempts to load an Ecore_Evas engine module.
 * It first checks if the engine is already registered. If not, it tries
 * to load it using `bs_mod_get` for a path like "ecore_evas/engines/<engine_name>".
 * If that fails, it iterates through the paths in `_engines_paths` and
 * attempts to load the module from "<path>/<engine_name>/<MODULE_ARCH>/<ECORE_EVAS_ENGINE_NAME>".
 *
 * Successfully loaded modules are registered in `_registered_engines`.
 *
 * @param engine The name of the engine to load (e.g., "software_x11", "opengl_sdl").
 * @return A pointer to the loaded Eina_Module for the engine,
 *         or NULL if the engine cannot be found or loaded.
 */
Eina_Module *
_ecore_evas_engine_load(const char *engine)
{
   const char *path;
   Eina_List *l;
   Eina_Module *em = NULL;
   char tmp[PATH_MAX] = "";

   EINA_SAFETY_ON_NULL_RETURN_VAL(engine, NULL);

   em =  (Eina_Module *)eina_hash_find(_registered_engines, engine);
   if (em) return em;

   if (bs_mod_get(tmp, sizeof(tmp), "ecore_evas/engines", engine))
     {
        em = eina_module_new(tmp);
        if (!em) return NULL;

        if (!eina_module_load(em))
          {
             eina_module_free(em);
             return NULL;
          }
        if (eina_hash_add(_registered_engines, engine, em))
          return em;
     }

   EINA_LIST_FOREACH(_engines_paths, l, path)
     {
        snprintf(tmp, sizeof(tmp), "%s/%s/%s/%s",
                 path, engine, MODULE_ARCH, ECORE_EVAS_ENGINE_NAME);

        em = eina_module_new(tmp);
        if (!em) continue;

        if (!eina_module_load(em))
          {
             eina_module_free(em);
             continue;
          }
        if (eina_hash_add(_registered_engines, engine, em))
          return em;
     }

   return NULL;
}

/**
 * @internal
 * @brief Initializes the Ecore_Evas engine loading system.
 *
 * This function sets up the necessary structures for loading Ecore_Evas engines.
 * It initializes the `_registered_engines` hash table.
 * It determines potential search paths for engine modules:
 * 1. A path relative to this function's symbol (`_ecore_evas_engine_init`),
 *    typically `libecore_evas.so/../ecore_evas/engines/`.
 * 2. The system's standard library directory for Ecore_Evas engines,
 *    `PACKAGE_LIB_DIR/ecore_evas/engines/` (on non-Windows systems) or
 *    a path relative to this function for Windows.
 * These paths are stored in `_engines_paths`.
 */
void
_ecore_evas_engine_init(void)
{
   char *paths[2] = { NULL, NULL };
   unsigned int i;
   unsigned int j;

/* avoid freeing modules ever to avoid deferred cb symbol problems */
//   _registered_engines = eina_hash_string_small_new(EINA_FREE_CB(eina_module_free));
   _registered_engines = eina_hash_string_small_new(NULL);

   /* 1. libecore_evas.so/../ecore_evas/engines/ */
   paths[0] = eina_module_symbol_path_get(_ecore_evas_engine_init, "/ecore_evas/engines");
#ifndef _WIN32
   /* 3. PREFIX/ecore_evas/engines/ */
   paths[1] = strdup(PACKAGE_LIB_DIR "/ecore_evas/engines");
#else
   paths[1] = eina_module_symbol_path_get(_ecore_evas_engine_init, "/../lib/ecore_evas/engines");
#endif

   for (j = 0; j < ((sizeof (paths) / sizeof (char*)) - 1); ++j)
     for (i = j + 1; i < sizeof (paths) / sizeof (char*); ++i)
       if (paths[i] && paths[j] && !strcmp(paths[i], paths[j]))
         {
            free(paths[i]);
            paths[i] = NULL;
         }

   for (i = 0; i < sizeof (paths) / sizeof (char*); ++i)
     if (paths[i])
       _engines_paths = eina_list_append(_engines_paths, paths[i]);
}

/**
 * @internal
 * @brief Shuts down the Ecore_Evas engine loading system.
 *
 * This function cleans up resources used by the engine loading system.
 * It frees the list of engine search paths (`_engines_paths`) and
 * the list of available engine names (`_engines_available`).
 * Note: The `_registered_engines` hash table and the modules themselves
 * are intentionally not freed to avoid issues with deferred callbacks
 * that might still reference symbols from unloaded modules.
 */
void
_ecore_evas_engine_shutdown(void)
{
   char *path;

/* don't free modules to avoid fn callback deferred symbol problems
   if (_registered_engines)
     {
       eina_hash_free(_registered_engines);
       _registered_engines = NULL;
     }
 */

   EINA_LIST_FREE(_engines_paths, path)
     free(path);

   EINA_LIST_FREE(_engines_available, path)
     eina_stringshare_del(path);
}

/**
 * @internal
 * @brief Gets a list of available Ecore_Evas engine names.
 *
 * This function scans the configured engine paths (`_engines_paths`)
 * to discover available Ecore_Evas engines. For each directory found in
 * `_engines_paths`, it looks for subdirectories. If a subdirectory
 * `<engine_dir_name>` contains a valid engine module file
 * (`<engine_dir_name>/<MODULE_ARCH>/ECORE_EVAS_ENGINE_NAME`),
 * the corresponding engine name(s) are added to a list.
 *
 * The engine names are determined based on `<engine_dir_name>` and
 * preprocessor definitions (e.g., `BUILD_ECORE_EVAS_FB`).
 * For example, if `<engine_dir_name>` is "x", it might add "opengl_x11"
 * and/or "software_x11" depending on build configurations.
 *
 * The list of available engines is cached in `_engines_available`
 * and returned. Subsequent calls will return the cached list.
 *
 * @return A const Eina_List * containing stringshared engine names.
 *         The caller should not modify or free this list.
 *         The strings are eina_stringshare instances.
 *         Example list elements: "fb", "opengl_x11", "software_x11", "buffer", etc.
 */
const Eina_List *
_ecore_evas_available_engines_get(void)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *l = NULL, *result = NULL;
   Eina_Strbuf *buf;
   const char *path;

   if (_engines_available) return _engines_available;

   buf = eina_strbuf_new();
   EINA_LIST_FOREACH(_engines_paths, l, path)
     {
        it = eina_file_direct_ls(path);

        EINA_ITERATOR_FOREACH(it, info)
          {
             eina_strbuf_append_printf(buf, "%s/%s/" ECORE_EVAS_ENGINE_NAME,
                                       info->path, MODULE_ARCH);

             if (eina_file_access(eina_strbuf_string_get(buf), EINA_FILE_ACCESS_MODE_EXIST))
               {
                  const char *name;

#ifdef _WIN32
                  name = strrchr(info->path, '\\');
                  if (name) name++;
#endif
                  name = strrchr(info->path, '/');
                  if (name) name++;
                  else name = info->path;
#define ADDENG(x) result = eina_list_append(result, eina_stringshare_add(x))
                  if (!strcmp(name, "fb"))
                    {
#ifdef BUILD_ECORE_EVAS_FB
                       ADDENG("fb");
#endif
                    }
                  else if (!strcmp(name, "x"))
                    {
#ifdef BUILD_ECORE_EVAS_OPENGL_X11
                       ADDENG("opengl_x11");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_XLIB
                       ADDENG("software_x11");
#endif
                    }
                  else if (!strcmp(name, "buffer"))
                    {
#ifdef BUILD_ECORE_EVAS_BUFFER
                       ADDENG("buffer");
#endif
                    }
                  else if (!strcmp(name, "cocoa"))
                    {
#ifdef BUILD_ECORE_EVAS_OPENGL_COCOA
                       ADDENG("opengl_cocoa");
#endif
                    }
                  else if (!strcmp(name, "sdl"))
                    {
#ifdef BUILD_ECORE_EVAS_OPENGL_SDL
                       ADDENG("opengl_sdl");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_SDL
                       ADDENG("sdl");
#endif
                    }
                  else if (!strcmp(name, "wayland"))
                    {
#ifdef BUILD_ECORE_EVAS_WAYLAND_SHM
                       ADDENG("wayland_shm");
#endif
#ifdef BUILD_ECORE_EVAS_WAYLAND_EGL
                       ADDENG("wayland_egl");
#endif
                    }
                  else if (!strcmp(name, "win32"))
                    {
#ifdef BUILD_ECORE_EVAS_SOFTWARE_GDI
                       ADDENG("software_gdi");
#endif
#ifdef BUILD_ECORE_EVAS_SOFTWARE_DDRAW
                       ADDENG("software_ddraw");
#endif
#ifdef BUILD_ECORE_EVAS_DIRECT3D
                       ADDENG("direct3d");
#endif
#ifdef BUILD_ECORE_EVAS_OPENGL_GLEW
                       ADDENG("opengl_glew");
#endif
#ifdef BUILD_ECORE_EVAS_OPENGL_WIN32
                       ADDENG("opengl_win32");
#endif
                    }
                  else if (!strcmp(name, "drm"))
                    {
#ifdef BUILD_ECORE_EVAS_DRM
                       ADDENG("drm");
#endif
#ifdef BUILD_ECORE_EVAS_GL_DRM
                       ADDENG("gl_drm");
#endif
                    }
               }
             eina_strbuf_reset(buf);
          }
        eina_iterator_free(it);
     }
   eina_strbuf_free(buf);

   _engines_available = result;
   return result;
}
