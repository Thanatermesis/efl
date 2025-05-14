#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Eina.h"
#include "Ecore.h"

#include "Ecore_Buffer.h"
#include "ecore_buffer_private.h"

typedef struct _Ecore_Buffer_Module Ecore_Buffer_Module;
typedef struct _Ecore_Buffer_Cb_Data Ecore_Buffer_Cb_Data;

/**
 * @brief Represents a loaded Ecore_Buffer backend module.
 *
 * This structure holds a pointer to the backend's function table and
 * any data associated with the initialized module.
 */
struct _Ecore_Buffer_Module
{
     Ecore_Buffer_Backend *be; /**< Pointer to the backend's function implementations. */
     Ecore_Buffer_Module_Data data; /**< Data specific to the initialized backend module. */
};

/**
 * @brief Represents an Ecore_Buffer object.
 *
 * This structure holds the properties of a buffer, such as its dimensions,
 * format, and flags, as well as backend-specific data and registered
 * free callbacks.
 */
struct _Ecore_Buffer
{
   unsigned int width; /**< The width of the buffer in pixels. */
   unsigned int height; /**< The height of the buffer in pixels. */
   int format; /**< The pixel format of the buffer (Ecore_Buffer_Format). */
   unsigned int flags; /**< Flags associated with the buffer. */

   Ecore_Buffer_Data buffer_data; /**< Backend-specific data for this buffer instance. */
   Ecore_Buffer_Module *bm; /**< Pointer to the backend module managing this buffer. */

   Eina_Hash *data; /**< Hash table for storing user-specific data associated with the buffer. */
   Eina_Inlist *free_callbacks; /**< List of callbacks to be invoked when the buffer is freed. */
};

/**
 * @brief Represents data for a buffer free callback.
 *
 * This structure is used to store a callback function and its associated
 * user data, to be called when an Ecore_Buffer is freed.
 */
struct _Ecore_Buffer_Cb_Data
{
   EINA_INLIST; /**< Macro to make this struct usable in an Eina_Inlist. */
   Ecore_Buffer_Cb cb; /**< The callback function to be called. */
   void *data; /**< User-supplied data to be passed to the callback. */
};

static Eina_Hash *_backends; /**< Hash table storing available buffer backends, keyed by name. */
static Eina_Array *_modules; /**< Array storing loaded Ecore_Buffer modules. */
static int _ecore_buffer_init_count = 0; /**< Initialization counter for the Ecore_Buffer library. */
static int _ecore_buffer_log_dom = -1; /**< Log domain for Ecore_Buffer messages. */

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_ecore_buffer_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_ecore_buffer_log_dom, __VA_ARGS__)

#ifndef PACKAGE_LIB_DIR
#define PACKAGE_LIB_DIR ""
#endif
#ifndef MODULE_ARCH
#define MODULE_ARCH ""
#endif

/**
 * @internal
 * @brief Retrieves and initializes a backend module.
 *
 * If @p name is NULL, it tries to get the backend name from the
 * ECORE_BUFFER_ENGINE environment variable. If that's not set,
 * it picks the first available backend.
 *
 * @param name The name of the backend to retrieve. Can be NULL.
 * @return A pointer to the initialized Ecore_Buffer_Module, or NULL on failure.
 */
static Ecore_Buffer_Module *
_ecore_buffer_get_backend(const char *name)
{
   Ecore_Buffer_Module *bm = NULL;
   Eina_Iterator *backend_name_itr;
   const char *backend_name = NULL;

   backend_name = name;

   if (backend_name == NULL)
     {
        backend_name = (const char*)getenv("ECORE_BUFFER_ENGINE");
        if (!backend_name)
          {
             backend_name_itr = eina_hash_iterator_data_new(_backends);
             while((!bm) &&
                   (eina_iterator_next(backend_name_itr, (void **)&bm)));
             eina_iterator_free(backend_name_itr);
          }
     }
   else
     bm = eina_hash_find(_backends, backend_name);

   if ((!bm) || (!bm->be))
     return NULL;

   if (bm->be->init)
     bm->data = bm->be->init(NULL, NULL);

   return bm;
}

/**
 * @internal
 * @brief Frees resources associated with a backend module.
 *
 * This function is used as a callback for eina_hash_foreach to shut down
 * and free backend modules stored in the _backends hash.
 *
 * @param hash The hash table being iterated (unused).
 * @param key The key of the hash entry (unused).
 * @param data The Ecore_Buffer_Module to free.
 * @param fdata User data passed to eina_hash_foreach (unused).
 * @return EINA_TRUE to continue iteration, EINA_FALSE to stop.
 */
static Eina_Bool
_ecore_buffer_backends_free(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   Ecore_Buffer_Module *bm = data;

   if (!bm)
     return EINA_FALSE;

   if (bm->data)
     bm->be->shutdown(bm->data);

   return EINA_TRUE;
}

/**
 * @brief Registers a new buffer backend.
 *
 * This function allows external modules to register their backend implementations
 * with the Ecore_Buffer system.
 *
 * @param be A pointer to an Ecore_Buffer_Backend structure describing the backend.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., if @p be is NULL or memory allocation fails).
 */
EAPI Eina_Bool
ecore_buffer_register(Ecore_Buffer_Backend *be)
{
   Ecore_Buffer_Module *bm;

   EINA_SAFETY_ON_NULL_RETURN_VAL(be, 0);

   bm = calloc(1, sizeof(Ecore_Buffer_Module));
   if (!bm)
     return EINA_FALSE;

   bm->be = be;
   bm->data = NULL;

   return eina_hash_add(_backends, be->name, bm);
}

/**
 * @brief Unregisters a buffer backend.
 *
 * This function removes a previously registered backend from the Ecore_Buffer system.
 *
 * @param be A pointer to the Ecore_Buffer_Backend structure of the backend to unregister.
 */
EAPI void
ecore_buffer_unregister(Ecore_Buffer_Backend *be)
{
   Ecore_Buffer_Module *bm;

   EINA_SAFETY_ON_NULL_RETURN(be);

   bm = eina_hash_find(_backends, be->name);
   if (!bm)
     return;

   eina_hash_del(_backends, be->name, bm);
   free(bm);
}

/**
 * @brief Initializes the Ecore_Buffer library.
 *
 * This function sets up the Ecore_Buffer system, including registering a log domain
 * and loading available backend modules. It maintains an init count, so it can be
 * called multiple times, but the actual initialization only happens on the first call.
 *
 * @return EINA_TRUE on successful initialization or if already initialized.
 *         EINA_FALSE if initialization fails (e.g., log domain registration fails,
 *         no modules found).
 */
EAPI Eina_Bool
ecore_buffer_init(void)
{
   char *path;

   if (++_ecore_buffer_init_count > 1)
     return EINA_TRUE;

   _ecore_buffer_log_dom = eina_log_domain_register("ecore_buffer", EINA_COLOR_BLUE);
   if (_ecore_buffer_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ecore_buffer");
        goto err;
     }

   _backends = eina_hash_string_superfast_new(NULL);

   /* dynamic backends */
   _modules = eina_module_arch_list_get(NULL,
                                        PACKAGE_LIB_DIR "/ecore_buffer/modules",
                                        MODULE_ARCH);

   path = eina_module_symbol_path_get((const void *)ecore_buffer_init,
                                      "/ecore_buffer/modules");

   _modules = eina_module_arch_list_get(_modules, path, MODULE_ARCH);
   if (path)
     free(path);

   /* fallback using module where in build directory */
   if ((!_modules) ||
       (eina_array_count(_modules) == 0))
     {
        ERR("No available module in library directy: %s",
            PACKAGE_LIB_DIR "/ecore_buffer/modules");
        ERR("Fallback to load module where in build directory :%s",
            PACKAGE_BUILD_DIR "/src/modules/");
        _modules = eina_module_list_get(NULL,
                                        PACKAGE_BUILD_DIR "/src/modules/",
                                        EINA_TRUE, NULL, NULL);
     }

   if ((!_modules) ||
       (eina_array_count(_modules) == 0))
     {
        ERR("no ecore_buffer modules able to be loaded.");
        eina_hash_free(_backends);
        eina_log_domain_unregister(_ecore_buffer_log_dom);
        _ecore_buffer_log_dom = -1;
        goto err;
     }

   // XXX: MODFIX: do not list ALL modules and load them ALL! this is
   // wrong. load the module we need WHEN we need it (by name etc. etc.
   // from api).
   eina_module_list_load(_modules);

   return EINA_TRUE;

err:
   _ecore_buffer_init_count--;
   return EINA_FALSE;
}

/**
 * @brief Shuts down the Ecore_Buffer library.
 *
 * This function cleans up resources used by the Ecore_Buffer system,
 * such as loaded modules and the log domain. It decrements an init count
 * and performs cleanup only when the count reaches zero.
 *
 * @return EINA_TRUE on successful shutdown or if the library was not fully shut down yet.
 *         EINA_FALSE if shutdown is called without a corresponding init or if already fully shut down.
 */
EAPI Eina_Bool
ecore_buffer_shutdown(void)
{
   if (_ecore_buffer_init_count < 1)
     {
        WARN("Ecore_Buffer shut down called without init");
        return EINA_FALSE;
     }

   if (--_ecore_buffer_init_count != 0)
     return EINA_FALSE;

   /* dynamic backends */
   eina_hash_foreach(_backends, _ecore_buffer_backends_free, NULL);

   eina_module_list_free(_modules);
   if (_modules)
     eina_array_free(_modules);

   if (_backends)
     eina_hash_free(_backends);

   eina_log_domain_unregister(_ecore_buffer_log_dom);
   _ecore_buffer_log_dom = -1;

   return EINA_TRUE;
}

/**
 * @brief Creates a new Ecore_Buffer.
 *
 * This function allocates and initializes a new buffer using the specified backend engine.
 *
 * @param engine The name of the backend engine to use (e.g., "drm", "x11").
 *               If NULL, the default engine is chosen (see _ecore_buffer_get_backend()).
 * @param width The desired width of the buffer in pixels.
 * @param height The desired height of the buffer in pixels.
 * @param format The desired pixel format of the buffer (see Ecore_Buffer_Format).
 * @param flags Flags to control buffer creation (backend-specific).
 * @return A pointer to the newly created Ecore_Buffer, or NULL on failure.
 *         Example flags: ECORE_BUFFER_FLAG_NONE
 *         Example format: ECORE_BUFFER_FORMAT_ARGB8888
 */
EAPI Ecore_Buffer*
ecore_buffer_new(const char *engine, unsigned int width, unsigned int height, Ecore_Buffer_Format format, unsigned int flags)
{
   Ecore_Buffer_Module *bm;
   Ecore_Buffer *bo;
   void *bo_data;

   bm = _ecore_buffer_get_backend(engine);
   if (!bm)
     {
        ERR("Failed to get backend: %s", engine);
        return NULL;
     }

   EINA_SAFETY_ON_NULL_RETURN_VAL(bm->be, NULL);

   if (!bm->be->buffer_alloc)
     {
        ERR("Not supported create buffer");
        return NULL;
     }

   bo = calloc(1, sizeof(Ecore_Buffer));
   if (!bo)
     return NULL;

   bo_data = bm->be->buffer_alloc(bm->data, width, height, format, flags);
   if (!bo_data)
     {
        free(bo);
        return NULL;
     }

   bo->bm = bm;
   bo->width = width;
   bo->height = height;
   bo->format = format;
   bo->flags = flags;
   bo->buffer_data = bo_data;

   return bo;
}

/**
 * @brief Frees an Ecore_Buffer.
 *
 * This function releases all resources associated with the given buffer,
 * including calling any registered free callbacks and backend-specific
 * deallocation routines.
 *
 * @param buf The Ecore_Buffer to free.
 */
EAPI void
ecore_buffer_free(Ecore_Buffer *buf)
{
   Ecore_Buffer_Cb_Data *free_cb;

   EINA_SAFETY_ON_NULL_RETURN(buf);

   //Call free_cb
   while (buf->free_callbacks)
     {
        free_cb = EINA_INLIST_CONTAINER_GET(buf->free_callbacks, Ecore_Buffer_Cb_Data);
        buf->free_callbacks = eina_inlist_remove(buf->free_callbacks, buf->free_callbacks);

        free_cb->cb(buf, free_cb->data);
        free(free_cb);
     }

   EINA_SAFETY_ON_NULL_RETURN(buf->bm);
   EINA_SAFETY_ON_NULL_RETURN(buf->bm->be);
   EINA_SAFETY_ON_NULL_RETURN(buf->bm->be->buffer_free);

   buf->bm->be->buffer_free(buf->bm->data, buf->buffer_data);

   //Free User Data
   if (buf->data)
     eina_hash_free(buf->data);

   free(buf);
}

/**
 * @brief Gets a pointer to the raw pixel data of a buffer.
 *
 * The interpretation of this data depends on the buffer's format.
 * This function relies on the backend's `data_get` implementation.
 *
 * @param buf The Ecore_Buffer.
 * @return A pointer to the buffer's pixel data, or NULL if not available or on error.
 */
EAPI void *
ecore_buffer_data_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm->be, NULL);

   if (!buf->bm->be->data_get)
     return NULL;

   return buf->bm->be->data_get(buf->bm->data, buf->buffer_data);
}

/**
 * @brief Gets a native pixmap handle for the buffer.
 *
 * This function is backend-dependent and returns a handle (e.g., X11 Pixmap ID)
 * that can be used with the native windowing system.
 * Relies on the backend's `pixmap_get` implementation.
 *
 * @param buf The Ecore_Buffer.
 * @return A native pixmap handle (Ecore_Pixmap), or 0 if not available or on error.
 */
EAPI Ecore_Pixmap
ecore_buffer_pixmap_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm->be, 0);

   if (!buf->bm->be->pixmap_get)
     return 0;

   return buf->bm->be->pixmap_get(buf->bm->data, buf->buffer_data);
}

/**
 * @brief Gets a TBM surface handle for the buffer.
 *
 * Tizen Buffer Manager (TBM) is a buffer management system. This function
 * retrieves a handle to the TBM surface associated with the Ecore_Buffer.
 * Relies on the backend's `tbm_surface_get` implementation.
 *
 * @param buf The Ecore_Buffer.
 * @return A pointer to the TBM surface, or NULL if TBM is not supported or on error.
 */
EAPI void *
ecore_buffer_tbm_surface_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm->be, NULL);

   if (!buf->bm->be->tbm_surface_get)
     {
        ERR("TBM is not supported\n");
        return NULL;
     }

   return buf->bm->be->tbm_surface_get(buf->bm->data, buf->buffer_data);
}

/**
 * @brief Retrieves the dimensions of an Ecore_Buffer.
 *
 * @param buf The Ecore_Buffer.
 * @param[out] width Pointer to store the width of the buffer. Can be NULL.
 * @param[out] height Pointer to store the height of the buffer. Can be NULL.
 * @return EINA_TRUE on success, EINA_FALSE if @p buf is NULL.
 */
EAPI Eina_Bool
ecore_buffer_size_get(Ecore_Buffer *buf, unsigned int *width, unsigned int *height)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, EINA_FALSE);

   if (width) *width = buf->width;
   if (height) *height = buf->height;

   return EINA_TRUE;
}

/**
 * @brief Retrieves the pixel format of an Ecore_Buffer.
 *
 * @param buf The Ecore_Buffer.
 * @return The Ecore_Buffer_Format of the buffer, or 0 if @p buf is NULL.
 *         Example return: ECORE_BUFFER_FORMAT_ARGB8888
 */
EAPI unsigned int
ecore_buffer_format_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, 0);

   return buf->format;
}

/**
 * @brief Retrieves the flags associated with an Ecore_Buffer.
 *
 * These flags are typically set during buffer creation and can be backend-specific.
 *
 * @param buf The Ecore_Buffer.
 * @return The flags of the buffer, or 0 if @p buf is NULL.
 */
EAPI unsigned int
ecore_buffer_flags_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, 0);

   return buf->flags;
}

/**
 * @brief Adds a callback function to be called when an Ecore_Buffer is freed.
 *
 * Multiple callbacks can be added. They will be called in the order they were added.
 *
 * @param buf The Ecore_Buffer to attach the callback to.
 * @param func The callback function to add.
 *             The function signature is: `void (*Ecore_Buffer_Cb)(Ecore_Buffer *buffer, void *user_data);`
 * @param data User-specific data to be passed to the callback function.
 */
EAPI void
ecore_buffer_free_callback_add(Ecore_Buffer *buf, Ecore_Buffer_Cb func, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(buf);
   EINA_SAFETY_ON_NULL_RETURN(func);

   Ecore_Buffer_Cb_Data *free_cb;

   free_cb = calloc(1, sizeof(Ecore_Buffer_Cb_Data));
   if (!free_cb)
     return;

   free_cb->cb = func;
   free_cb->data = data;
   buf->free_callbacks = eina_inlist_append(buf->free_callbacks, EINA_INLIST_GET(free_cb));
}

/**
 * @brief Removes a previously added free callback.
 *
 * The callback function and its associated data must match those provided
 * during `ecore_buffer_free_callback_add`.
 *
 * @param buf The Ecore_Buffer from which to remove the callback.
 * @param func The callback function to remove.
 * @param data The user-specific data associated with the callback.
 */
EAPI void
ecore_buffer_free_callback_remove(Ecore_Buffer *buf, Ecore_Buffer_Cb func, void *data)
{
   Ecore_Buffer_Cb_Data *free_cb;

   EINA_SAFETY_ON_NULL_RETURN(buf);
   EINA_SAFETY_ON_NULL_RETURN(func);

   if (buf->free_callbacks)
     {
        Eina_Inlist *itrn;
        EINA_INLIST_FOREACH_SAFE(buf->free_callbacks, itrn, free_cb)
          {
             if (free_cb->cb == func && free_cb->data == data)
               {
                  buf->free_callbacks =
                     eina_inlist_remove(buf->free_callbacks,
                                        EINA_INLIST_GET(free_cb));
                  free(free_cb);
               }
          }
     }
}

/**
 * @internal
 * @brief Gets the name of the backend engine used by a buffer.
 *
 * @param buf The Ecore_Buffer.
 * @return The name of the backend engine, or NULL (0 cast to char*) on error.
 */
const char *
_ecore_buffer_engine_name_get(Ecore_Buffer *buf)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm->be, 0);

   return buf->bm->be->name;
}

/**
 * @internal
 * @brief Exports a buffer for sharing with other processes or APIs.
 *
 * This function relies on the backend's `buffer_export` implementation.
 * The type of export (e.g., DMABUF FD, TBM surface name) and the
 * associated ID are returned.
 *
 * @param buf The Ecore_Buffer to export.
 * @param[out] id Pointer to store the ID associated with the export (e.g., file descriptor).
 * @return The type of export (Ecore_Export_Type), or EXPORT_TYPE_INVALID on failure.
 *         Example return: EXPORT_TYPE_DMABUF
 */
Ecore_Export_Type
_ecore_buffer_export(Ecore_Buffer *buf, int *id)
{
   Ecore_Export_Type type = EXPORT_TYPE_INVALID;
   int ret_id;

   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, type);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm, type);
   EINA_SAFETY_ON_NULL_RETURN_VAL(buf->bm->be, type);

   if (!buf->bm->be->buffer_export)
     return type;

   type = buf->bm->be->buffer_export(buf->bm->data, buf->buffer_data, &ret_id);

   if (id) *id = ret_id;

   return type;
}

/**
 * @internal
 * @brief Imports a buffer that was previously exported.
 *
 * This function creates a new Ecore_Buffer from an exported buffer handle.
 * It relies on the backend's `buffer_import` implementation.
 *
 * @param engine The name of the backend engine to use for importing.
 * @param width The width of the buffer.
 * @param height The height of the buffer.
 * @param format The pixel format of the buffer.
 * @param type The type of the exported buffer (Ecore_Export_Type).
 *             Example: EXPORT_TYPE_DMABUF
 * @param export_id The ID of the exported buffer (e.g., file descriptor).
 * @param flags Flags for the imported buffer.
 * @return A new Ecore_Buffer wrapping the imported resource, or NULL on failure.
 */
Ecore_Buffer *
_ecore_buffer_import(const char *engine, int width, int height, Ecore_Buffer_Format format, Ecore_Export_Type type, int export_id, unsigned int flags)
{
   Ecore_Buffer_Module *bm;
   Ecore_Buffer *bo;
   void *bo_data;

   bm = _ecore_buffer_get_backend(engine);
   if (!bm)
     {
        ERR("Filed to get Backend: %s", engine);
        return NULL;
     }

   EINA_SAFETY_ON_NULL_RETURN_VAL(bm->be, NULL);

   if (!bm->be->buffer_import)
     {
        ERR("Not supported import buffer");
        return NULL;
     }

   bo = calloc(1, sizeof(Ecore_Buffer));
   if (!bo)
     return NULL;

   bo_data = bm->be->buffer_import(bm->data, width, height, format, type, export_id, flags);
   if (!bo_data)
     {
        free(bo);
        return NULL;
     }

   bo->bm = bm;
   bo->width = width;
   bo->height = height;
   bo->format = format;
   bo->flags = flags;
   bo->buffer_data = bo_data;

   return bo;
}
