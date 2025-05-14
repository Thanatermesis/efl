#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h> // For malloc, free
#include <stdio.h> // For snprintf
#include <sys/stat.h> // For stat (used in NEED_RUN_IN_TREE)

#include "evas_common_private.h"
#include "evas_private.h"
#include "../../static_libs/buildsystem/buildsystem.h"


#ifndef EVAS_MODULE_NO_ENGINES
#define EVAS_MODULE_NO_ENGINES 0
#endif

#ifndef EVAS_MODULE_NO_IMAGE_LOADERS
#define EVAS_MODULE_NO_IMAGE_LOADERS 0
#endif

#ifndef EVAS_MODULE_NO_IMAGE_SAVERS
#define EVAS_MODULE_NO_IMAGE_SAVERS 0
#endif

#ifndef EVAS_MODULE_NO_VG_LOADERS
#define EVAS_MODULE_NO_VG_LOADERS 0
#endif

#ifndef EVAS_MODULE_NO_VG_SAVERS
#define EVAS_MODULE_NO_VG_SAVERS 0
#endif

/**
 * @internal
 * @struct _Evas_Module_Task
 * @brief Structure to hold task cancellation information for a module.
 *
 * This structure is used with thread-local storage to allow modules
 * to check if their current operation/task has been cancelled.
 */
typedef struct _Evas_Module_Task Evas_Module_Task;
struct _Evas_Module_Task
{
   Eina_Bool (*cancelled)(void *data); /**< Callback function to check if the task is cancelled. */
   void *data; /**< User data to be passed to the cancelled callback. */
};

/**
 * @internal
 * @brief Thread-local storage key for Evas_Module_Task.
 *
 * This TLS key holds a pointer to an Evas_Module_Task structure,
 * allowing different threads to have their own task cancellation contexts.
 */
static Eina_TLS task = 0;

/**
 * @brief Checks if the current module task has been cancelled.
 *
 * This function retrieves the task cancellation status from thread-local storage.
 * Modules performing long operations can periodically call this to check
 * if they should abort.
 *
 * @return @c EINA_TRUE if the task has been cancelled, @c EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_module_task_cancelled(void)
{
   Evas_Module_Task *t;

   t = eina_tls_get(task);
   if (!t) return EINA_FALSE;

   return t->cancelled(t->data);
}

/**
 * @brief Registers a task cancellation callback for the current thread.
 *
 * Stores the provided cancellation callback and data in thread-local storage.
 * This allows a module to define how its cancellation status is determined.
 *
 * @param cancelled The callback function that will be called to check for cancellation.
 *                  It should return EINA_TRUE if cancelled, EINA_FALSE otherwise.
 * @param data      User data to be passed to the `cancelled` callback.
 */
EVAS_API void
evas_module_task_register(Eina_Bool (*cancelled)(void *data), void *data)
{
   Evas_Module_Task *t;

   t = malloc(sizeof (Evas_Module_Task));
   if (!t) return ;

   t->cancelled = cancelled;
   t->data = data;

   eina_tls_set(task, t);
}

/**
 * @brief Unregisters the task cancellation callback for the current thread.
 *
 * Removes the task cancellation information from thread-local storage and
 * frees the associated Evas_Module_Task structure. This is also used as
 * the delete callback for the TLS key `task`.
 */
EVAS_API void
evas_module_task_unregister(void)
{
    Evas_Module_Task *t;

    t = eina_tls_get(task);
    if (!t) return ;

    eina_tls_set(task, NULL);
    free(t);
}

/**
 * @internal
 * @brief Array of hash tables for different module types.
 *
 * Each Evas_Module_Type (engine, image_loader, etc.) has its own hash table
 * where modules are stored, keyed by their name.
 * The array is indexed by Evas_Module_Type enum values.
 * For example:
 * evas_modules[EVAS_MODULE_TYPE_ENGINE] stores engine modules.
 * evas_modules[EVAS_MODULE_TYPE_IMAGE_LOADER] stores image loader modules.
 */
static Eina_Hash *evas_modules[6] = {
  NULL, /**< EVAS_MODULE_TYPE_ENGINE */
  NULL, /**< EVAS_MODULE_TYPE_IMAGE_LOADER */
  NULL, /**< EVAS_MODULE_TYPE_IMAGE_SAVER */
  NULL, /**< EVAS_MODULE_TYPE_OBJECT */
  NULL, /**< EVAS_MODULE_TYPE_VG_LOADER */
  NULL  /**< EVAS_MODULE_TYPE_VG_SAVER */
};

/**
 * @internal
 * @brief List of paths where Evas modules are searched.
 *
 * This list contains strings, each representing a directory path.
 * Example: ["/usr/local/lib/evas/modules", "~/.evas/modules"]
 */
static Eina_List *evas_module_paths = NULL;

/**
 * @internal
 * @brief Array of loaded engine modules.
 *
 * This array stores pointers to Evas_Module structures for engine modules.
 * The index in this array (plus one) serves as the engine's ID.
 * Example: If evas_engines contains [module_A, module_B], then
 * module_A has ID 1, and module_B has ID 2.
 */
static Eina_Array *evas_engines = NULL;

/**
 * @internal
 * @brief Appends a path to a list if the path exists.
 *
 * If the given path string is not NULL and the path exists in the filesystem,
 * it is appended to the list. If the path does not exist or the path string is NULL,
 * the path string is freed (if not NULL).
 *
 * @param list The list to append to.
 * @param path The path string to append. This function takes ownership of the string if appended,
 *             or frees it if not.
 * @return The (potentially modified) list.
 */
static Eina_List *
_evas_module_append(Eina_List *list, char *path)
{
   if (path)
     {
        if (evas_file_path_exists(path))
          list = eina_list_append(list, path);
        else
          free(path);
     }
   return list;
}

/**
 * @internal
 * @brief Initializes the search paths for Evas modules.
 *
 * This function populates `evas_module_paths` with directories where Evas
 * will look for modules. The search order is generally:
 * 1. Build directory (if `EFL_RUN_IN_TREE` is set and applicable).
 * 2. Path relative to the Evas library (`libevas.so/../evas/modules/`).
 * 3. Standard system library path (`PACKAGE_LIB_DIR/evas/modules/`).
 *
 * Paths are only added if they exist and are not already in the list.
 */
void
evas_module_paths_init(void)
{
   char *libdir, *path;

#ifdef NEED_RUN_IN_TREE
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
   if (getuid() == geteuid())
#endif
     {
        static signed char run_in_tree = -1;

        if (run_in_tree == -1)
          {
             if (getenv("EFL_RUN_IN_TREE")) run_in_tree = 1;
             else run_in_tree = 0;
          }
        if (run_in_tree == 1)
          {
             struct stat st;
             const char mp[] = PACKAGE_BUILD_DIR"/src/modules/evas";
             if (stat(mp, &st) == 0)
               {
                  evas_module_paths = _evas_module_append(evas_module_paths, strdup(mp));
                  return;
               }
          }
     }
#endif

   /* 1. libevas.so/../evas/modules/ */
   libdir = (char *)_evas_module_libdir_get();
   if (!libdir)
     path = eina_module_symbol_path_get(evas_module_paths_init, "/evas/modules");
   else
     {
        path = malloc(strlen(libdir) + strlen("/evas/modules") + 1);
        if (path)
          {
             strcpy(path, libdir);
             strcat(path, "/evas/modules");
          }
     }
   if (eina_list_search_unsorted(evas_module_paths, (Eina_Compare_Cb) strcmp, path))
     free(path);
   else
     evas_module_paths = _evas_module_append(evas_module_paths, path);

   /* 2. PREFIX/lib/evas/modules/ */
   path = PACKAGE_LIB_DIR "/evas/modules";
   if (!eina_list_search_unsorted(evas_module_paths, (Eina_Compare_Cb) strcmp, path))
     {
        path = strdup(path);
        if (path)
          evas_module_paths = _evas_module_append(evas_module_paths, path);
     }
}

#define EVAS_EINA_STATIC_MODULE_DEFINE(Tn, Name) \
  Eina_Bool evas_##Tn##_##Name##_init(void); \
  void evas_##Tn##_##Name##_shutdown(void);

#define EVAS_EINA_STATIC_MODULE_USE(Tn, Name) \
  { evas_##Tn##_##Name##_init, evas_##Tn##_##Name##_shutdown }

#if !EVAS_MODULE_NO_ENGINES
EVAS_EINA_STATIC_MODULE_DEFINE(engine, buffer);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, drm);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, fb);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, gl_generic);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, gl_drm);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, gl_x11);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, gl_sdl);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, gl_win32);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_8);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_8_x11);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_ddraw);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_gdi);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_generic);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, software_x11);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, wayland_shm);
EVAS_EINA_STATIC_MODULE_DEFINE(engine, wayland_egl);
#endif

#if !EVAS_MODULE_NO_VG_LOADERS
EVAS_EINA_STATIC_MODULE_DEFINE(vg_loader, eet);
EVAS_EINA_STATIC_MODULE_DEFINE(vg_loader, svg);
EVAS_EINA_STATIC_MODULE_DEFINE(vg_loader, json);
#endif

#if !EVAS_MODULE_NO_IMAGE_LOADERS
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, avif);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, bmp);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, dds);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, eet);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, generic);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, gif);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, heif);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, ico);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, jpeg);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, jp2k);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, jxl);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, pmaps);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, png);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, psd);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, qoi);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, svg);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, tga);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, tiff);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, wbmp);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, webp);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, xpm);
EVAS_EINA_STATIC_MODULE_DEFINE(image_loader, tgv);
#endif

#if !EVAS_MODULE_NO_VG_SAVERS
EVAS_EINA_STATIC_MODULE_DEFINE(vg_saver, eet);
EVAS_EINA_STATIC_MODULE_DEFINE(vg_saver, svg);
#endif

#if !EVAS_MODULE_NO_IMAGE_SAVERS
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, avif);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, eet);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, jpeg);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, jxl);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, png);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, qoi);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, tiff);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, webp);
EVAS_EINA_STATIC_MODULE_DEFINE(image_saver, tgv);
#endif

static const struct {
   Eina_Bool (*init)(void);
   void (*shutdown)(void);
} evas_static_module[] = {
#if !EVAS_MODULE_NO_ENGINES
#ifdef EVAS_STATIC_BUILD_BUFFER
  EVAS_EINA_STATIC_MODULE_USE(engine, buffer),
#endif
#ifdef EVAS_STATIC_BUILD_DRM
  EVAS_EINA_STATIC_MODULE_USE(engine, drm),
#endif
#ifdef EVAS_STATIC_BUILD_FB
  EVAS_EINA_STATIC_MODULE_USE(engine, fb),
#endif
#ifdef EVAS_STATIC_BUILD_GL_COMMON
  EVAS_EINA_STATIC_MODULE_USE(engine, gl_generic),
#endif
#ifdef EVAS_STATIC_BUILD_GL_DRM
  EVAS_EINA_STATIC_MODULE_USE(engine, gl_drm),
#endif
#ifdef EVAS_STATIC_BUILD_GL_X11
  EVAS_EINA_STATIC_MODULE_USE(engine, gl_x11),
#endif
#ifdef EVAS_STATIC_BUILD_GL_SDL
  EVAS_EINA_STATIC_MODULE_USE(engine, gl_sdl),
#endif
#ifdef EVAS_STATIC_BUILD_GL_WIN32
  EVAS_EINA_STATIC_MODULE_USE(engine, gl_win32),
#endif
#ifdef EVAS_STATIC_BUILD_SOFTWARE_DDRAW
  EVAS_EINA_STATIC_MODULE_USE(engine, software_ddraw),
#endif
#ifdef EVAS_STATIC_BUILD_SOFTWARE_GDI
  EVAS_EINA_STATIC_MODULE_USE(engine, software_gdi),
#endif
#ifdef EVAS_STATIC_BUILD_SOFTWARE_GENERIC
  EVAS_EINA_STATIC_MODULE_USE(engine, software_generic),
#endif
#ifdef EVAS_STATIC_BUILD_SOFTWARE_X11
  EVAS_EINA_STATIC_MODULE_USE(engine, software_x11),
#endif
#ifdef EVAS_STATIC_BUILD_WAYLAND_EGL
  EVAS_EINA_STATIC_MODULE_USE(engine, wayland_egl),
#endif
#ifdef EVAS_STATIC_BUILD_WAYLAND_SHM
  EVAS_EINA_STATIC_MODULE_USE(engine, wayland_shm),
#endif
#endif
#if !EVAS_MODULE_NO_VG_LOADERS
#ifdef EVAS_STATIC_BUILD_VG_SVG
  EVAS_EINA_STATIC_MODULE_USE(vg_loader, svg),
#endif
#ifdef EVAS_STATIC_BUILD_VG_EET
  EVAS_EINA_STATIC_MODULE_USE(vg_loader, eet),
#endif
#ifdef EVAS_STATIC_BUILD_VG_JSON
  EVAS_EINA_STATIC_MODULE_USE(vg_loader, json),
#endif
#endif
#if !EVAS_MODULE_NO_IMAGE_LOADERS
#ifdef EVAS_STATIC_BUILD_AVIF
  EVAS_EINA_STATIC_MODULE_USE(image_loader, avif),
#endif
#ifdef EVAS_STATIC_BUILD_BMP
  EVAS_EINA_STATIC_MODULE_USE(image_loader, bmp),
#endif
#ifdef EVAS_STATIC_BUILD_DDS
  EVAS_EINA_STATIC_MODULE_USE(image_loader, dds),
#endif
#ifdef EVAS_STATIC_BUILD_EET
  EVAS_EINA_STATIC_MODULE_USE(image_loader, eet),
#endif
#ifdef EVAS_STATIC_BUILD_GENERIC
  EVAS_EINA_STATIC_MODULE_USE(image_loader, generic),
#endif
#ifdef EVAS_STATIC_BUILD_GIF
  EVAS_EINA_STATIC_MODULE_USE(image_loader, gif),
#endif
#ifdef EVAS_STATIC_BUILD_HEIF
  EVAS_EINA_STATIC_MODULE_USE(image_loader, heif),
#endif
#ifdef EVAS_STATIC_BUILD_ICO
  EVAS_EINA_STATIC_MODULE_USE(image_loader, ico),
#endif
#ifdef EVAS_STATIC_BUILD_JPEG
  EVAS_EINA_STATIC_MODULE_USE(image_loader, jpeg),
#endif
#ifdef EVAS_STATIC_BUILD_JP2K
  EVAS_EINA_STATIC_MODULE_USE(image_loader, jp2k),
#endif
#ifdef EVAS_STATIC_BUILD_JXL
  EVAS_EINA_STATIC_MODULE_USE(image_loader, jxl),
#endif
#ifdef EVAS_STATIC_BUILD_PMAPS
  EVAS_EINA_STATIC_MODULE_USE(image_loader, pmaps),
#endif
#ifdef EVAS_STATIC_BUILD_PNG
  EVAS_EINA_STATIC_MODULE_USE(image_loader, png),
#endif
#ifdef EVAS_STATIC_BUILD_PSD
  EVAS_EINA_STATIC_MODULE_USE(image_loader, psd),
#endif
#ifdef EVAS_STATIC_BUILD_QOI
  EVAS_EINA_STATIC_MODULE_USE(image_loader, qoi),
#endif
#ifdef EVAS_STATIC_BUILD_SVG
  EVAS_EINA_STATIC_MODULE_USE(image_loader, svg),
#endif
#ifdef EVAS_STATIC_BUILD_TGA
  EVAS_EINA_STATIC_MODULE_USE(image_loader, tga),
#endif
#ifdef EVAS_STATIC_BUILD_TIFF
  EVAS_EINA_STATIC_MODULE_USE(image_loader, tiff),
#endif
#ifdef EVAS_STATIC_BUILD_WBMP
  EVAS_EINA_STATIC_MODULE_USE(image_loader, wbmp),
#endif
#ifdef EVAS_STATIC_BUILD_WEBP
  EVAS_EINA_STATIC_MODULE_USE(image_loader, webp),
#endif
#ifdef EVAS_STATIC_BUILD_XPM
  EVAS_EINA_STATIC_MODULE_USE(image_loader, xpm),
#endif
#ifdef EVAS_STATIC_BUILD_TGV
  EVAS_EINA_STATIC_MODULE_USE(image_loader, tgv),
#endif
#endif
#if !EVAS_MODULE_NO_VG_SAVERS
#ifdef EVAS_STATIC_BUILD_VG_EET
  EVAS_EINA_STATIC_MODULE_USE(vg_saver, eet),
#endif
#ifdef EVAS_STATIC_BUILD_VG_SVG
  EVAS_EINA_STATIC_MODULE_USE(vg_saver, svg),
#endif
#endif
#if !EVAS_MODULE_NO_IMAGE_SAVERS
#ifdef EVAS_STATIC_BUILD_AVIF
  EVAS_EINA_STATIC_MODULE_USE(image_saver, avif),
#endif
#ifdef EVAS_STATIC_BUILD_EET
  EVAS_EINA_STATIC_MODULE_USE(image_saver, eet),
#endif
#ifdef EVAS_STATIC_BUILD_JPEG
  EVAS_EINA_STATIC_MODULE_USE(image_saver, jpeg),
#endif
#ifdef EVAS_STATIC_BUILD_JXL
  EVAS_EINA_STATIC_MODULE_USE(image_saver, jxl),
#endif
#ifdef EVAS_STATIC_BUILD_PNG
  EVAS_EINA_STATIC_MODULE_USE(image_saver, png),
#endif
#ifdef EVAS_STATIC_BUILD_QOI
  EVAS_EINA_STATIC_MODULE_USE(image_saver, qoi),
#endif
#ifdef EVAS_STATIC_BUILD_TIFF
  EVAS_EINA_STATIC_MODULE_USE(image_saver, tiff),
#endif
#ifdef EVAS_STATIC_BUILD_WEBP
  EVAS_EINA_STATIC_MODULE_USE(image_saver, webp),
#endif
#ifdef EVAS_STATIC_BUILD_TGV
  EVAS_EINA_STATIC_MODULE_USE(image_saver, tgv),
#endif
#endif
  { NULL, NULL }
};

static void _evas_module_hash_free_cb(void *data);

/* this will alloc an Evas_Module struct for each module
 * it finds on the paths */
void
evas_module_init(void)
{
   int i;

   evas_module_paths_init();

   evas_modules[EVAS_MODULE_TYPE_ENGINE] = eina_hash_string_small_new(_evas_module_hash_free_cb);
   evas_modules[EVAS_MODULE_TYPE_IMAGE_LOADER] = eina_hash_string_small_new(_evas_module_hash_free_cb);
   evas_modules[EVAS_MODULE_TYPE_IMAGE_SAVER] = eina_hash_string_small_new(_evas_module_hash_free_cb);
   evas_modules[EVAS_MODULE_TYPE_OBJECT] = eina_hash_string_small_new(_evas_module_hash_free_cb);
   evas_modules[EVAS_MODULE_TYPE_VG_LOADER] = eina_hash_string_small_new(_evas_module_hash_free_cb);
   evas_modules[EVAS_MODULE_TYPE_VG_SAVER] = eina_hash_string_small_new(_evas_module_hash_free_cb);

   evas_engines = eina_array_new(4);

   eina_tls_cb_new(&task, (Eina_TLS_Delete_Cb) evas_module_task_unregister);

   for (i = 0; evas_static_module[i].init; ++i)
     evas_static_module[i].init();
}

/**
 * @brief Registers a module with the Evas module system.
 *
 * This function is typically called by a module itself during its initialization
 * (e.g., from its `module_open` function if it's a dynamic module, or from
 * a static initializer). It adds the module to the appropriate hash table
 * based on its type.
 *
 * @param module Pointer to the module's API structure. This structure contains
 *               metadata about the module (name, version) and function pointers.
 * @param type The type of the module (e.g., EVAS_MODULE_TYPE_ENGINE).
 * @return @c EINA_TRUE if the module was registered successfully, @c EINA_FALSE otherwise
 *         (e.g., invalid type, NULL module, version mismatch, or module already registered).
 */
Eina_Bool
evas_module_register(const Evas_Module_Api *module, Evas_Module_Type type)
{
   Evas_Module *em;

   if ((unsigned int)type > 5) return EINA_FALSE; /* EVAS_MODULE_TYPE_LAST is 5 */
   if (!module) return EINA_FALSE;
   if (module->version != EVAS_MODULE_API_VERSION) return EINA_FALSE;

   em = eina_hash_find(evas_modules[type], module->name);
   if (em) return EINA_FALSE;

   em = calloc(1, sizeof (Evas_Module));
   if (!em) return EINA_FALSE;

   LKI(em->lock);
   em->definition = module;

   if (type == EVAS_MODULE_TYPE_ENGINE)
     {
        eina_array_push(evas_engines, em);
        em->id_engine = eina_array_count(evas_engines);
     }

   eina_hash_direct_add(evas_modules[type], module->name, em);

   return EINA_TRUE;
}

/**
 * @brief Retrieves a list of available engine module names.
 *
 * This function scans the configured module paths for engine subdirectories
 * and also includes the names of statically linked engines.
 * The returned list contains stringshared names of the engines.
 *
 * @return A new Eina_List containing engine names (const char *).
 *         The caller is responsible for freeing this list and its contents
 *         (e.g., using EINA_LIST_FREE and eina_stringshare_del).
 *         Example of list elements: ["software_x11", "gl_generic", "buffer"]
 */
Eina_List *
evas_module_engine_list(void)
{
   Evas_Module *em;
   Eina_List *r = NULL, *l, *ll;
   Eina_Array_Iterator iterator;
   Eina_Iterator *it, *it2;
   unsigned int i;
   const char *s, *s2;
   char buf[PATH_MAX];
#ifdef NEED_RUN_IN_TREE
   static signed char run_in_tree = -1;

   if (run_in_tree == -1)
     {
        if (getenv("EFL_RUN_IN_TREE")) run_in_tree = 1;
        else run_in_tree = 0;
     }
#endif

   EINA_LIST_FOREACH(evas_module_paths, l, s)
     {
        snprintf(buf, sizeof(buf), "%s/engines", s);
        it = eina_file_direct_ls(buf);
        if (it)
          {
             Eina_File_Direct_Info *fi;

             EINA_ITERATOR_FOREACH(it, fi)
               {
                  const char *fname = fi->path + fi->name_start;

#ifdef NEED_RUN_IN_TREE
                  buf[0] = '\0';
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
                  if (getuid() == geteuid())
#endif
                    {
                       if (run_in_tree == 1)
                         {
                            bs_mod_dir_get(buf, sizeof(buf), "evas/engines", fname);
                            if (!evas_file_path_exists(buf))
                            buf[0] = '\0';
                         }
                    }

                  if (buf[0] == '\0')
#endif
                    snprintf(buf, sizeof(buf), "%s/engines/%s/%s",
                             s, fname, MODULE_ARCH);

                  it2 = eina_file_ls(buf);
                  if (it2)
                    {
                       EINA_LIST_FOREACH(r, ll, s2)
                         {
                            if (!strcmp(fname, s2)) break;
                         }
                       if (!ll)
                         r = eina_list_append(r, eina_stringshare_add(fname));
                       eina_iterator_free(it2);
                    }
               }
             eina_iterator_free(it);
          }
     }

   EINA_ARRAY_ITER_NEXT(evas_engines, i, em, iterator)
     {
        EINA_LIST_FOREACH(r, ll, s2)
          {
             if (!strcmp(em->definition->name, s2)) break;
          }
        if (!ll)
          r = eina_list_append(r, eina_stringshare_add(em->definition->name));
     }

   return r;
}

/**
 * @brief Unregisters a module from the Evas module system.
 *
 * This function removes a module from the Evas module system. It's typically
 * called by a module during its shutdown sequence.
 *
 * @param module Pointer to the module's API structure that was used for registration.
 * @param type The type of the module.
 * @return @c EINA_TRUE if the module was successfully unregistered, @c EINA_FALSE otherwise
 *         (e.g., module not found, or the provided `module` pointer doesn't match
 *         the registered one).
 */
Eina_Bool
evas_module_unregister(const Evas_Module_Api *module, Evas_Module_Type type)
{
   Evas_Module *em;

   if ((unsigned int)type > 5) return EINA_FALSE; /* EVAS_MODULE_TYPE_LAST is 5 */
   if (!module) return EINA_FALSE;

   em = eina_hash_find(evas_modules[type], module->name);
   if (!em || em->definition != module) return EINA_FALSE;

   eina_hash_del(evas_modules[type], module->name, em);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Callback function to free Evas_Module structures stored in hash tables.
 *
 * This function is called when a module is removed from one of the `evas_modules`
 * hash tables (e.g., during shutdown or unregistration).
 * It calls the module's close function if it's loaded and then frees the
 * Evas_Module structure itself.
 *
 * @note This function intentionally does not call `dlclose()` on the module's
 *       Eina_Module handle. This is a design choice to avoid issues related to
 *       unloading shared libraries that might still be in use or have complex
 *       dependencies. The Eina_Module itself might be leaked.
 *
 * @param data Pointer to the Evas_Module to be freed.
 */
static void
_evas_module_hash_free_cb(void *data)
{
   Evas_Module *em = data;

   // Note: This free callback leaks the Eina_Module, and does not call
   // dlclose(). This is by choice as dlclose() leads to other issues.

   if (!em) return;
   if (em->id_engine > 0)
     eina_array_data_set(evas_engines, em->id_engine - 1, NULL);
   if (em->loaded)
     {
        em->definition->func.close(em);
        em->loaded = 0;
     }

   LKD(em->lock);
   free(em);
}

#if defined(_WIN32) || defined(__CYGWIN__)
/** @internal @brief Defines the standard module file extension for Windows systems. */
# define EVAS_MODULE_NAME "module.dll"
#else
/** @internal @brief Defines the standard module file extension for Unix-like systems. */
# define EVAS_MODULE_NAME "module.so"
#endif

/**
 * @internal
 * @brief Finds a module of a specific type by its name. (Implementation)
 *
 * This is the internal implementation for `evas_module_find_type`.
 * It first checks statically linked/already registered modules. If not found,
 * it iterates through `evas_module_paths`, constructing potential module file paths
 * (e.g., `<path>/<type_str>/<name>/<MODULE_ARCH>/module.so`) and attempts to
 * load them using `eina_module_new` and `eina_module_load`.
 * If a dynamic module loads successfully, it's expected to register itself via
 * `evas_module_register`, at which point it will be found in the hash.
 *
 * @param type The type of the module to find.
 * @param name The name of the module.
 * @return Pointer to the Evas_Module if found and loaded, NULL otherwise.
 */
Evas_Module *
evas_module_find_type(Evas_Module_Type type, const char *name)
{
   const char *path;
   char buffer[PATH_MAX];
   Evas_Module *em;
   Eina_Module *en;
   Eina_List *l;
#ifdef NEED_RUN_IN_TREE
   static signed char run_in_tree = -1;

   if (run_in_tree == -1)
     {
        if (getenv("EFL_RUN_IN_TREE")) run_in_tree = 1;
        else run_in_tree = 0;
     }
#endif

   if ((unsigned int)type > 5) return NULL;

   em = eina_hash_find(evas_modules[type], name);
   if (em)
     {
        if (evas_module_load(em)) return em;
        return NULL;
     }

   EINA_LIST_FOREACH(evas_module_paths, l, path)
     {
        const char *type_str = "unknown";
        switch (type)
          {
           case EVAS_MODULE_TYPE_ENGINE: type_str = "engines"; break;
           case EVAS_MODULE_TYPE_IMAGE_LOADER: type_str = "image_loaders"; break;
           case EVAS_MODULE_TYPE_IMAGE_SAVER: type_str = "image_savers"; break;
           case EVAS_MODULE_TYPE_OBJECT: type_str = "object"; break;
           case EVAS_MODULE_TYPE_VG_LOADER: type_str = "vg_loaders"; break;
           case EVAS_MODULE_TYPE_VG_SAVER: type_str = "vg_savers"; break;
          }

        buffer[0] = '\0';
#if NEED_RUN_IN_TREE
#if defined(HAVE_GETUID) && defined(HAVE_GETEUID)
        if (getuid() == geteuid())
#endif
          {
             if (run_in_tree == 1)
               {
                  char subsystem[PATH_MAX];

                  snprintf(subsystem, sizeof(subsystem), "evas/%s", type_str);
                  bs_mod_get(buffer, sizeof(buffer), subsystem, name);
                  if (!evas_file_path_exists(buffer))
                  buffer[0] = '\0';
               }
          }
#endif

        if (buffer[0] == '\0')
          snprintf(buffer, sizeof(buffer), "%s/%s/%s/%s/%s",
                   path, type_str, name, MODULE_ARCH, EVAS_MODULE_NAME);

        if (!evas_file_path_is_file(buffer)) continue;

        en = eina_module_new(buffer);
        if (!en) continue;

        if (type == EVAS_MODULE_TYPE_ENGINE)
          eina_module_symbol_global_set(en, EINA_TRUE);

        if (!eina_module_load(en))
          {
             eina_module_free(en);
             continue;
          }
        // this is intentional. the above module load if it succeeds
        // registers the evas module below in the table that we then
        // lookup in the hash... and then load that as a 2nd stage.
        // since we will never unload a module once used it doesnt matter
        // what happens to the handle anyway.
        em = eina_hash_find(evas_modules[type], name);
        if (em)
          {
             if (evas_module_load(em)) return em;
          }

        eina_module_free(en);
     }

   return NULL;
}

/**
 * @internal
 * @brief Retrieves an engine module by its render method ID. (Implementation)
 *
 * @param render_method The ID of the render method (1-based index).
 * @return Pointer to the Evas_Module if found, NULL otherwise.
 */
Evas_Module *
evas_module_engine_get(int render_method)
{
   if ((render_method <= 0) ||
       ((unsigned int)render_method > eina_array_count(evas_engines)))
     return NULL;
   return eina_array_data_get(evas_engines, render_method - 1);
}

/**
 * @internal
 * @brief Iterates over all registered image loader modules. (Implementation)
 *
 * @param cb The callback function.
 * @param fdata User data for the callback.
 */
void
evas_module_foreach_image_loader(Eina_Hash_Foreach cb, const void *fdata)
{
   eina_hash_foreach(evas_modules[EVAS_MODULE_TYPE_IMAGE_LOADER], cb, fdata);
}

/**
 * @internal
 * @brief Loads an Evas module. (Implementation)
 * Calls the module's `open` function if not already loaded.
 *
 * @param em The module to load.
 * @return 1 on success or if already loaded, 0 on failure.
 */
int
evas_module_load(Evas_Module *em)
{
   if (em->loaded) return 1;
   if (!em->definition) return 0;

   if (!em->definition->func.open(em)) return 0;
   em->loaded = 1;

   return 1;
}

/**
 * @internal
 * @brief Unloads an Evas module. (Implementation)
 *
 * @note Currently, this function is a no-op. Modules are not actually
 *       unloaded by calling their `close` function to prevent potential
 *       instability if they are still in use.
 *
 * @param em The module to unload.
 */
void
evas_module_unload(Evas_Module *em)
{
   if (!em->loaded)
     return;
   if (!em->definition)
     return;

// for now lets not unload modules - they may still be in use.
//   em->definition->func.close(em);
//   em->loaded = 0;

}

/**
 * @internal
 * @brief Increments the reference count of an Evas module. (Implementation)
 * Uses a lock to ensure thread safety.
 *
 * @param em The module to reference.
 */
void
evas_module_ref(Evas_Module *em)
{
   LKL(em->lock);
   em->ref++;
   LKU(em->lock);
}

/**
 * @internal
 * @brief Decrements the reference count of an Evas module. (Implementation)
 * Uses a lock to ensure thread safety.
 *
 * @param em The module to unreference.
 */
void
evas_module_unref(Evas_Module *em)
{
   LKL(em->lock);
   em->ref--;
   LKU(em->lock);
}

/**
 * @internal
 * @brief Global counter for module usage, used by `evas_module_clean`.
 * This counter increments periodically and helps determine how "long ago"
 * a module was last used.
 */
static int use_count = 0;

/**
 * @internal
 * @brief Marks an Evas module as recently used. (Implementation)
 * Sets the module's `last_used` field to the current `use_count`.
 *
 * @param em The module to mark.
 */
void
evas_module_use(Evas_Module *em)
{
   em->last_used = use_count;
}

/**
 * @internal
 * @brief Cleans up unused Evas modules. (Implementation)
 *
 * This function is intended to unload modules that are not referenced and
 * have not been used for a certain number of cleaning cycles.
 * It is called periodically (every 256 calls to this function).
 * Module cleaning can be disabled by setting the `EVAS_NOCLEAN` environment
 * variable.
 *
 * @note Currently, the actual unloading logic within this function is
 *       commented out, effectively disabling module cleaning even if
 *       `EVAS_NOCLEAN` is not set.
 */
void
evas_module_clean(void)
{
   static int call_count = 0;
/*    int ago; */
   static signed char noclean = -1;
/*    Eina_List *l; */
/*    Evas_Module *em; */

   /* only clean modules every 256 calls */
   call_count++;
   if (call_count <= 256) return;
   call_count = 0;

   if (noclean == -1)
     {
        if (getenv("EVAS_NOCLEAN")) noclean = 1;
        else noclean = 0;
     }
   if (noclean == 1) return;

   /* disable module cleaning for now - may cause instability with some modules */
   return;

   /* FIXME: Don't know what it is supposed to do. */
/*    /\* incriment use counter = 28bits *\/ */
/*    use_count++; */
/*    if (use_count > 0x0fffffff) use_count = 0; */

/*    /\* printf("CLEAN!\n"); *\/ */
/*    /\* go through all modules *\/ */
/*    EINA_LIST_FOREACH(evas_modules, l, em) */
/*      { */
/*         /\* printf("M %s %i %i\n", em->name, em->ref, em->loaded); *\/ */
/* 	/\* if the module is refernced - skip *\/ */
/* 	if ((em->ref > 0) || (!em->loaded)) continue; */
/* 	/\* how many clean cycles ago was this module last used *\/ */
/* 	ago = use_count - em->last_used; */
/* 	if (em->last_used > use_count) ago += 0x10000000; */
/* 	/\* if it was used last more than N clean cycles ago - unload *\/ */
/* 	if (ago > 5) */
/* 	  { */
/*             /\* printf("  UNLOAD %s\n", em->name); *\/ */
/* 	     evas_module_unload(em); */
/* 	  } */
/*      } */
}

/**
 * @internal
 * @brief Eina_Prefix object for Evas.
 * Used to determine library and data directories.
 */
static Eina_Prefix *pfx = NULL;

/**
 * @internal
 * @brief Shuts down the Evas module system. (Implementation)
 *
 * Calls shutdown for all static modules, frees all module hash tables,
 * the TLS key, module paths list, engines array, and the Eina_Prefix object.
 */
void
evas_module_shutdown(void)
{
   char *path;
   int i;

   for (i = 0; evas_static_module[i].shutdown; ++i)
     evas_static_module[i].shutdown();

   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_ENGINE]);
   evas_modules[EVAS_MODULE_TYPE_ENGINE] = NULL;
   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_IMAGE_LOADER]);
   evas_modules[EVAS_MODULE_TYPE_IMAGE_LOADER] = NULL;
   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_IMAGE_SAVER]);
   evas_modules[EVAS_MODULE_TYPE_IMAGE_SAVER] = NULL;
   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_OBJECT]);
   evas_modules[EVAS_MODULE_TYPE_OBJECT] = NULL;
   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_VG_LOADER]);
   evas_modules[EVAS_MODULE_TYPE_VG_LOADER] = NULL;
   eina_hash_free(evas_modules[EVAS_MODULE_TYPE_VG_SAVER]);
   evas_modules[EVAS_MODULE_TYPE_VG_SAVER] = NULL;

   eina_tls_free(task);

   EINA_LIST_FREE(evas_module_paths, path)
     free(path);

   eina_array_free(evas_engines);
   evas_engines = NULL;
   if (pfx)
     {
        eina_prefix_free(pfx);
        pfx = NULL;
     }
}

EVAS_API int
_evas_module_engine_inherit(Evas_Func *funcs, char *name, size_t info)
{
   Evas_Module *em;

   em = evas_module_find_type(EVAS_MODULE_TYPE_ENGINE, name);
   if (em)
     {
        if (evas_module_load(em))
          {
             /* FIXME: no way to unref */
             evas_module_ref(em);
             evas_module_use(em);
             *funcs = *((Evas_Func *)(em->functions));
             funcs->info_size = info;
             return 1;
          }
     }
   return 0;
}

/**
 * @brief Gets the Evas library directory.
 *
 * Uses Eina_Prefix to determine the correct library directory for Evas.
 * This is used, for example, to find module paths relative to the library.
 *
 * @return A string containing the library directory path. Do not free.
 *         Returns an empty string if Eina_Prefix fails.
 */
EVAS_API const char *
_evas_module_libdir_get(void)
{
   if (!pfx) pfx = eina_prefix_new
      (NULL, _evas_module_libdir_get, "EVAS", "evas", "checkme", /* "checkme" is a placeholder file to find prefix */
       PACKAGE_BIN_DIR, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   if (!pfx) return "";
   return eina_prefix_lib_get(pfx);
}

/**
 * @internal
 * @brief Gets the Evas data directory.
 *
 * Uses Eina_Prefix to determine the correct data directory for Evas.
 *
 * @return A string containing the data directory path. Do not free.
 *         Returns NULL if Eina_Prefix fails.
 */
const char *
_evas_module_datadir_get(void)
{
   if (!pfx) pfx = eina_prefix_new
      (NULL, _evas_module_libdir_get, "EVAS", "evas", "checkme", /* "checkme" is a placeholder file to find prefix */
       PACKAGE_BIN_DIR, PACKAGE_LIB_DIR, PACKAGE_DATA_DIR, PACKAGE_DATA_DIR);
   if (!pfx) return NULL;
   return eina_prefix_data_get(pfx);
}

/**
 * @deprecated This function is deprecated and always returns NULL.
 * @brief Gets the CServe path.
 * @return Always NULL.
 */
EVAS_API const char *
evas_cserve_path_get(void)
{
   return NULL;
}
