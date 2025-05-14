/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
/**
 * @file
 * @brief Evas GL API
 *
 * This file contains the Evas GL API implementation.
 * It provides functions for creating and managing Evas GL contexts,
 * surfaces, and configurations.
 */
#include "evas_common_private.h"
#include "evas_private.h"
#include "Evas_GL.h"

/**
 * @internal
 * @struct _Evas_GL_TLS_data
 * @brief Thread-local storage data for Evas GL.
 *
 * This structure holds thread-specific data for Evas GL, primarily
 * the error state.
 */
typedef struct _Evas_GL_TLS_data Evas_GL_TLS_data;

/* since 1.16: store current evas gl - this TLS is never destroyed */
static Eina_TLS _current_evas_gl_key = 0;

/**
 * @internal
 * @struct _Evas_GL
 * @brief Main Evas GL context structure.
 *
 * This structure holds all data related to an Evas GL instance,
 * including associated Evas canvas, lists of contexts and surfaces,
 * locks, and thread-local storage keys.
 */
struct _Evas_GL
{
   DATA32      magic; /**< Magic number for type checking. */
   Evas_Public_Data *evas; /**< Pointer to the associated Evas public data. */

   Eina_List  *contexts; /**< List of created Evas_GL_Context objects. */
   Eina_List  *surfaces; /**< List of created Evas_GL_Surface objects. */
   Eina_Lock   lck; /**< Lock for thread-safe access to shared resources. */
   Eina_TLS    resource_key; /**< Thread-local storage key for Evas_GL_TLS_data. */
   Eina_List  *resource_list; /**< List of allocated Evas_GL_TLS_data instances for cleanup. */
};

/**
 * @internal
 * @struct _Evas_GL_Context
 * @brief Evas GL context structure.
 *
 * This structure represents an Evas GL rendering context.
 */
struct _Evas_GL_Context
{
   void    *data; /**< Pointer to the underlying engine-specific context data. */
   Evas_GL_Context_Version version; /**< The OpenGL ES version of this context. */
};

/**
 * @internal
 * @struct _Evas_GL_Surface
 * @brief Evas GL surface structure.
 *
 * This structure represents an Evas GL rendering surface.
 */
struct _Evas_GL_Surface
{
   void    *data; /**< Pointer to the underlying engine-specific surface data. */
   void    *output; /**< Pointer to the output buffer or window system drawable (engine-specific). */
};

/**
 * @internal
 * @struct _Evas_GL_TLS_data
 * @brief Thread-local storage data for Evas GL.
 *
 * This structure holds thread-specific data for Evas GL, primarily
 * the error state.
 */
struct _Evas_GL_TLS_data
{
   int error_state; /**< The last error code set for the current thread. See @ref Evas_GL_Error_Codes. */
};

/**
 * @internal
 * @brief Retrieves or creates thread-local storage data for Evas GL.
 *
 * This function gets the Evas_GL_TLS_data associated with the current thread
 * for the given Evas_GL instance. If it doesn't exist, it's created and
 * initialized.
 *
 * @param evas_gl The Evas_GL instance.
 * @return A pointer to the Evas_GL_TLS_data for the current thread, or NULL on failure.
 */
Evas_GL_TLS_data *
_evas_gl_internal_tls_get(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return NULL;

   if (!(tls_data = eina_tls_get(evas_gl->resource_key)))
     {
        tls_data = (Evas_GL_TLS_data*) calloc(1, sizeof(Evas_GL_TLS_data));
        if (!tls_data)
          {
             ERR("Evas_GL: Could not set error!");
             return NULL;
          }
        tls_data->error_state = EVAS_GL_SUCCESS;

        if (eina_tls_set(evas_gl->resource_key, (void*)tls_data) == EINA_TRUE)
          {
             LKL(evas_gl->lck);
             evas_gl->resource_list = eina_list_prepend(evas_gl->resource_list, tls_data);
             LKU(evas_gl->lck);
             return tls_data;
          }
        else
          {
             ERR("Evas_GL: Failed setting TLS data!");
             free(tls_data);
             return NULL;
          }
     }

   return tls_data;
}

/**
 * @internal
 * @brief Destroys all thread-local storage data associated with an Evas_GL instance.
 *
 * This function iterates through all created Evas_GL_TLS_data instances
 * for the given Evas_GL and frees them. It also frees the TLS key itself.
 * This is typically called when the Evas_GL instance is being freed.
 *
 * @param evas_gl The Evas_GL instance.
 */
void
_evas_gl_internal_tls_destroy(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return;

   if (!eina_tls_get(evas_gl->resource_key))
     {
        WRN("Destructor: TLS data was never set!");
        return;
     }

   LKL(evas_gl->lck);
   EINA_LIST_FREE(evas_gl->resource_list, tls_data)
     free(tls_data);

   if (evas_gl->resource_key)
     eina_tls_free(evas_gl->resource_key);
   evas_gl->resource_key = 0;
   LKU(evas_gl->lck);
}

/**
 * @internal
 * @brief Sets the error state for the current thread.
 *
 * This function updates the error_state in the thread-local storage
 * for the given Evas_GL instance.
 *
 * @param evas_gl The Evas_GL instance.
 * @param error_enum The error code to set (from @ref Evas_GL_Error_Codes).
 */
void
_evas_gl_internal_error_set(Evas_GL *evas_gl, int error_enum)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return;

   tls_data = _evas_gl_internal_tls_get(evas_gl);
   if (!tls_data) return;

   tls_data->error_state = error_enum;
}

/**
 * @internal
 * @brief Gets the error state for the current thread.
 *
 * This function retrieves the error_state from the thread-local storage
 * for the given Evas_GL instance.
 *
 * @param evas_gl The Evas_GL instance.
 * @return The current error code for the thread (from @ref Evas_GL_Error_Codes).
 *         Returns EVAS_GL_NOT_INITIALIZED if evas_gl is NULL or TLS data cannot be retrieved.
 */
int
_evas_gl_internal_error_get(Evas_GL *evas_gl)
{
   Evas_GL_TLS_data *tls_data;

   if (!evas_gl) return EVAS_GL_NOT_INITIALIZED;

   tls_data = _evas_gl_internal_tls_get(evas_gl);
   if (!tls_data) return EVAS_GL_NOT_INITIALIZED;

   return tls_data->error_state;
}

/**
 * @brief Creates a new Evas_GL object.
 *
 * This function initializes a new Evas_GL context associated with the
 * given Evas canvas. It checks if the underlying Evas engine supports
 * Evas GL operations.
 *
 * @param e The Evas canvas to associate with this Evas_GL object.
 * @return A new Evas_GL object on success, or @c NULL on failure.
 *         Possible failures include the Evas engine not supporting Evas GL,
 *         or memory allocation errors.
 *
 * @see evas_gl_free()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Evas_GL *
evas_gl_new(Evas *e)
{
   Evas_GL *evas_gl;

   MAGIC_CHECK(e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();

   if (!_current_evas_gl_key)
     {
        if (!eina_tls_new(&_current_evas_gl_key))
          {
             ERR("Error creating tls key for current Evas GL");
             return NULL;
          }
        eina_tls_set(_current_evas_gl_key, NULL);
     }

   evas_gl = calloc(1, sizeof(Evas_GL));
   if (!evas_gl) return NULL;

   evas_gl->magic = MAGIC_EVAS_GL;
   evas_gl->evas = efl_data_ref(e, EVAS_CANVAS_CLASS);
   LKI(evas_gl->lck);

   if (!evas_gl->evas->engine.func->gl_context_create ||
       !evas_gl->evas->engine.func->gl_supports_evas_gl ||
       !evas_gl->evas->engine.func->gl_supports_evas_gl(
          _evas_engine_context(evas_gl->evas)))
     {
        ERR("Evas GL engine not available.");
        efl_data_unref(e, evas_gl->evas);
        free(evas_gl);
        return NULL;
     }

   // Initialize tls resource key
   if (eina_tls_new(&(evas_gl->resource_key)) == EINA_FALSE)
     {
        ERR("Error creating tls key");
        efl_data_unref(e, evas_gl->evas);
        free(evas_gl);
        return NULL;
     }

   _evas_gl_internal_error_set(evas_gl, EVAS_GL_SUCCESS);
   return evas_gl;
}

/**
 * @brief Frees an Evas_GL object.
 *
 * This function releases all resources associated with the given Evas_GL
 * object. This includes destroying any remaining surfaces and contexts,
 * freeing thread-local storage, and unreferencing the associated Evas canvas.
 *
 * @param evas_gl The Evas_GL object to free.
 *
 * @see evas_gl_new()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API void
evas_gl_free(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   // Delete undeleted surfaces
   while (evas_gl->surfaces)
     evas_gl_surface_destroy(evas_gl, evas_gl->surfaces->data);

   // Delete undeleted contexts
   while (evas_gl->contexts)
     evas_gl_context_destroy(evas_gl, evas_gl->contexts->data);

   // Destroy private tls
   _evas_gl_internal_tls_destroy(evas_gl);

   // Reset current evas gl tls
   if (_current_evas_gl_key && (evas_gl == eina_tls_get(_current_evas_gl_key)))
     eina_tls_set(_current_evas_gl_key, NULL);

   efl_data_unref(evas_gl->evas->evas, evas_gl->evas);
   evas_gl->magic = 0;
   LKD(evas_gl->lck);
   free(evas_gl);
}

/**
 * @brief Creates a new Evas_GL_Config object.
 *
 * This function allocates and initializes a new Evas_GL_Config object
 * with default values. The caller is responsible for freeing this object
 * using evas_gl_config_free().
 *
 * @return A new Evas_GL_Config object on success, or @c NULL on allocation failure.
 *
 * @see evas_gl_config_free()
 * @see Evas_GL_Config
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Evas_GL_Config *
evas_gl_config_new(void)
{
   Evas_GL_Config *cfg;

   cfg = calloc(1, sizeof(Evas_GL_Config));

   if (!cfg) return NULL;

   return cfg;
}

/**
 * @brief Frees an Evas_GL_Config object.
 *
 * @param cfg The Evas_GL_Config object to free.
 *
 * @see evas_gl_config_new()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API void
evas_gl_config_free(Evas_GL_Config *cfg)
{
   if (cfg) free(cfg);
}

/**
 * @brief Creates a new Evas_GL_Surface.
 *
 * This function creates an offscreen rendering surface (typically a PBuffer or FBO)
 * with the specified configuration, width, and height.
 *
 * @param evas_gl The Evas_GL object.
 * @param config The configuration for the surface. Must not be @c NULL.
 *               See @ref Evas_GL_Config for details on configuration options.
 *               Example:
 *               @code
 *               Evas_GL_Config *cfg = evas_gl_config_new();
 *               cfg->color_format = EVAS_GL_RGB_888;
 *               cfg->depth_bits = EVAS_GL_DEPTH_BIT_24;
 *               // ... set other config options ...
 *               @endcode
 * @param width The width of the surface in pixels. Must be greater than 0.
 * @param height The height of the surface in pixels. Must be greater than 0.
 * @return A new Evas_GL_Surface object on success, or @c NULL on failure.
 *         Errors can include invalid parameters, bad configuration, or
 *         engine-level failures. Use evas_gl_error_get() to retrieve
 *         the specific error.
 *
 * @see evas_gl_surface_destroy()
 * @see evas_gl_config_new()
 * @see evas_gl_error_get()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Evas_GL_Surface *
evas_gl_surface_create(Evas_GL *evas_gl, Evas_GL_Config *config, int width, int height)
{
   Evas_GL_Surface *surf;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!config)
     {
        ERR("Invalid Config Pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONFIG);
        return NULL;
     }

   if ((width <= 0) || (height <= 0))
     {
        ERR("Invalid surface dimensions: %d, %d", width, height);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   surf = calloc(1, sizeof(Evas_GL_Surface));

   if (!surf)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   surf->data = evas_gl->evas->engine.func->gl_surface_create(_evas_engine_context(evas_gl->evas), config, width, height);

   if (!surf->data)
     {
        ERR("Failed creating a surface from the engine.");
        free(surf);
        return NULL;
     }

   // Keep track of the surface creations
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_prepend(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   return surf;
}

/**
 * @brief Creates a new PBuffer Evas_GL_Surface with extended attributes.
 *
 * This function creates an offscreen PBuffer rendering surface with the
 * specified configuration, dimensions, and additional attributes.
 * This is often used for more fine-grained control over PBuffer creation
 * when supported by the underlying engine.
 *
 * @param evas_gl The Evas_GL object.
 * @param cfg The configuration for the surface. Must not be @c NULL.
 *            See @ref Evas_GL_Config.
 * @param w The width of the PBuffer surface in pixels. Must be > 0.
 * @param h The height of the PBuffer surface in pixels. Must be > 0.
 * @param attrib_list A NULL-terminated list of attribute-value pairs for
 *                    PBuffer creation. The specific attributes are
 *                    engine-dependent.
 *                    Example for EGL-like attributes (conceptual):
 *                    @code
 *                    const int attribs[] = {
 *                        EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGB, // Example attribute
 *                        EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,  // Example attribute
 *                        EGL_NONE // Terminator
 *                    };
 *                    @endcode
 *                    If no extra attributes are needed, this can be @c NULL.
 * @return A new Evas_GL_Surface object on success, or @c NULL on failure.
 *         Errors can include invalid parameters, bad configuration, engine
 *         not supporting PBuffers, or engine-level failures. Use
 *         evas_gl_error_get() to retrieve the specific error.
 *
 * @see evas_gl_surface_destroy()
 * @see evas_gl_config_new()
 * @see evas_gl_error_get()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Evas_GL_Surface *
evas_gl_pbuffer_surface_create(Evas_GL *evas_gl, Evas_GL_Config *cfg,
                               int w, int h, const int *attrib_list)
{
   Evas_GL_Surface *surf;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!cfg)
     {
        ERR("Invalid Config Pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONFIG);
        return NULL;
     }

   if ((w <= 0) || (h <= 0))
     {
        ERR("Invalid surface dimensions: %d, %d", w, h);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   if (!evas_gl->evas->engine.func->gl_pbuffer_surface_create)
     {
        ERR("Engine does not support PBuffer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   surf = calloc(1, sizeof(Evas_GL_Surface));
   if (!surf)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   surf->data = evas_gl->evas->engine.func->gl_pbuffer_surface_create
     (_evas_engine_context(evas_gl->evas), cfg, w, h, attrib_list);
   if (!surf->data)
     {
        ERR("Engine failed to create a PBuffer!");
        free(surf);
        return NULL;
     }

   // Keep track of the surface creations
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_prepend(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   return surf;
}

/**
 * @brief Destroys an Evas_GL_Surface.
 *
 * This function releases all resources associated with the given
 * Evas_GL_Surface.
 *
 * @param evas_gl The Evas_GL object.
 * @param surf The Evas_GL_Surface to destroy. If @c NULL, an error
 *             EVAS_GL_BAD_SURFACE is set.
 *
 * @see evas_gl_surface_create()
 * @see evas_gl_pbuffer_surface_create()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API void
evas_gl_surface_destroy(Evas_GL *evas_gl, Evas_GL_Surface *surf)
{
   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   if (!surf)
     {
        ERR("Trying to destroy a NULL surface pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return;
     }

   // Call Engine's Surface Destroy
   evas_gl->evas->engine.func->gl_surface_destroy(_evas_engine_context(evas_gl->evas), surf->data);

   // Remove it from the list
   LKL(evas_gl->lck);
   evas_gl->surfaces = eina_list_remove(evas_gl->surfaces, surf);
   LKU(evas_gl->lck);

   // Delete the object
   free(surf);
   surf = NULL;
}

// Internal functions - called from evas_gl_core.c
/**
 * @internal
 * @brief Retrieves the native (engine-specific) context handle.
 *
 * This function is a callback used by the underlying graphics engine
 * to obtain the native context handle from an Evas_GL_Context wrapper.
 *
 * @param context A pointer to an Evas_GL_Context object.
 * @return The native context handle (e.g., EGLContext) or @c NULL if input is invalid.
 */
static void *
evas_gl_native_context_get(void *context)
{
   Evas_GL_Context *ctx = context;
   if (!ctx) return NULL;
   return ctx->data;
}

/**
 * @internal
 * @brief Retrieves the engine-specific data associated with an Evas_GL instance.
 *
 * This function is a callback used by the underlying graphics engine
 * to obtain its private data structure associated with the Evas canvas.
 *
 * @param evgl A pointer to an Evas_GL object.
 * @return The engine-specific context data (e.g., GL_Engine_Data*) or @c NULL if input is invalid.
 */
static void *
evas_gl_engine_data_get(void *evgl)
{
   Evas_GL *evasgl = evgl;

   if (!evasgl) return NULL;
   if (!evasgl->evas) return NULL;

   return _evas_engine_context(evasgl->evas);
}

/**
 * @brief Creates a new Evas_GL_Context with a specific GLES version.
 *
 * This function creates a new OpenGL ES rendering context with the specified
 * version. The new context can optionally share resources with an existing
 * context.
 *
 * @param evas_gl The Evas_GL object.
 * @param share_ctx An optional existing Evas_GL_Context with which to share
 *                  resources (e.g., textures, buffers). Can be @c NULL if no
 *                  sharing is desired.
 * @param version The desired OpenGL ES version for the new context.
 *                Supported versions are typically EVAS_GL_GLES_1_X,
 *                EVAS_GL_GLES_2_X, EVAS_GL_GLES_3_X.
 *                Example: `EVAS_GL_GLES_2_X`
 * @return A new Evas_GL_Context object on success, or @c NULL on failure.
 *         Errors can include invalid parameters, unsupported GLES version,
 *         or engine-level failures. Use evas_gl_error_get() to retrieve
 *         the specific error.
 *
 * @see evas_gl_context_destroy()
 * @see evas_gl_context_create() (for default GLES 2.X context)
 * @see evas_gl_error_get()
 * @see Evas_GL_Context_Version
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Evas_GL_Context *
evas_gl_context_version_create(Evas_GL *evas_gl, Evas_GL_Context *share_ctx,
                               Evas_GL_Context_Version version)
{
   Evas_GL_Context *ctx;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if ((version < EVAS_GL_GLES_1_X) || (version > EVAS_GL_GLES_3_X))
     {
        ERR("Can not create an OpenGL-ES %d.x context (not supported).",
            (int) version);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   // Allocate a context object
   ctx = calloc(1, sizeof(Evas_GL_Context));
   if (!ctx)
     {
        ERR("Unable to create a Evas_GL_Context object");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   // Call engine->gl_create_context
   ctx->version = version;
   ctx->data = evas_gl->evas->engine.func->gl_context_create
     (_evas_engine_context(evas_gl->evas), share_ctx ? share_ctx->data : NULL,
      version, &evas_gl_native_context_get, &evas_gl_engine_data_get);

   // Set a few variables
   if (!ctx->data)
     {
        ERR("Failed creating a context from the engine.");
        free(ctx);
        return NULL;
     }

   // Keep track of the context creations
   LKL(evas_gl->lck);
   evas_gl->contexts = eina_list_prepend(evas_gl->contexts, ctx);
   LKU(evas_gl->lck);

   return ctx;
}

/**
 * @brief Creates a new Evas_GL_Context (defaults to GLES 2.X).
 *
 * This function is a convenience wrapper around evas_gl_context_version_create()
 * that creates an OpenGL ES 2.X context.
 *
 * @param evas_gl The Evas_GL object.
 * @param share_ctx An optional existing Evas_GL_Context with which to share
 *                  resources. Can be @c NULL.
 * @return A new Evas_GL_Context object on success, or @c NULL on failure.
 *
 * @see evas_gl_context_version_create()
 * @see evas_gl_context_destroy()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Evas_GL_Context *
evas_gl_context_create(Evas_GL *evas_gl, Evas_GL_Context *share_ctx)
{
   return evas_gl_context_version_create(evas_gl, share_ctx, EVAS_GL_GLES_2_X);
}

/**
 * @brief Destroys an Evas_GL_Context.
 *
 * This function releases all resources associated with the given
 * Evas_GL_Context.
 *
 * @param evas_gl The Evas_GL object.
 * @param ctx The Evas_GL_Context to destroy. If @c NULL, an error
 *            EVAS_GL_BAD_CONTEXT is set.
 *
 * @see evas_gl_context_create()
 * @see evas_gl_context_version_create()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API void
evas_gl_context_destroy(Evas_GL *evas_gl, Evas_GL_Context *ctx)
{

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return;
   MAGIC_CHECK_END();

   if (!ctx)
     {
        ERR("Trying to destroy a NULL context pointer!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONTEXT);
        return;
     }

   // Call Engine's destroy
   evas_gl->evas->engine.func->gl_context_destroy(_evas_engine_context(evas_gl->evas), ctx->data);

   // Remove it from the list
   LKL(evas_gl->lck);
   evas_gl->contexts = eina_list_remove(evas_gl->contexts, ctx);
   LKU(evas_gl->lck);

   // Delete the object
   free(ctx);
   ctx = NULL;
}

/**
 * @brief Makes a GLES context current with a surface.
 *
 * This function binds a GLES context (@p ctx) to a drawing surface (@p surf),
 * making it the current target for rendering operations for the calling thread.
 *
 * @param evas_gl The Evas_GL object.
 * @param surf The Evas_GL_Surface to bind for drawing. Can be @c NULL
 *             for surfaceless contexts if supported by the platform and engine.
 * @param ctx The Evas_GL_Context to make current. Can be @c NULL to release
 *            the current context and surface (making no context current).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *         If @p surf is not @c NULL and @p ctx is @c NULL, or vice-versa in some
 *         engine implementations, it might be an error (EVAS_GL_BAD_MATCH).
 *         Check evas_gl_error_get() for specific errors.
 *
 * @note If both @p surf and @p ctx are @c NULL, this effectively releases the
 *       current context from the calling thread.
 * @note If @p surf is @c NULL and @p ctx is not @c NULL, this attempts to make
 *       the context current without a default draw/read surface (surfaceless context).
 *       This is useful for offscreen rendering to frame buffer objects (FBOs)
 *       that are not Evas_GL_Surface objects.
 *
 * @see evas_gl_current_context_get()
 * @see evas_gl_current_surface_get()
 * @see evas_gl_error_get()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Eina_Bool
evas_gl_make_current(Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_GL_Context *ctx)
{
   Eina_Bool ret;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if ((surf) && (ctx))
     ret = (Eina_Bool)evas_gl->evas->engine.func->gl_make_current(_evas_engine_context(evas_gl->evas), surf->data, ctx->data);
   else if ((!surf) && (!ctx))
     ret = (Eina_Bool)evas_gl->evas->engine.func->gl_make_current(_evas_engine_context(evas_gl->evas), NULL, NULL);
   else if ((!surf) && (ctx)) // surfaceless make current
     ret = (Eina_Bool)evas_gl->evas->engine.func->gl_make_current(_evas_engine_context(evas_gl->evas), NULL, ctx->data);
   else
     {
        ERR("Bad match between surface: %p and context: %p", surf, ctx);
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_MATCH);
        return EINA_FALSE;
     }

   if (_current_evas_gl_key)
     eina_tls_set(_current_evas_gl_key, evas_gl);

   return ret;
}

/**
 * @brief Gets the Evas_GL_Context currently bound to the calling thread.
 *
 * This function retrieves the Evas_GL_Context that was made current by
 * evas_gl_make_current() for the given Evas_GL instance.
 *
 * @param evas_gl The Evas_GL object.
 * @return The current Evas_GL_Context, or @c NULL if no context is current
 *         for this Evas_GL instance, or if the engine does not support
 *         retrieving the current context.
 *
 * @see evas_gl_make_current()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Evas_GL_Context *
evas_gl_current_context_get(Evas_GL *evas_gl)
{
   Evas_GL_Context *comp;
   void *internal_ctx;
   Eina_List *li;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_current_context_get)
     {
        CRI("Can not get current context with this engine: %s",
            evas_gl->evas->engine.module->definition->name);
        return NULL;
     }

   internal_ctx = evas_gl->evas->engine.func->gl_current_context_get(_evas_engine_context(evas_gl->evas));
   if (!internal_ctx)
     return NULL;

   LKL(evas_gl->lck);
   EINA_LIST_FOREACH(evas_gl->contexts, li, comp)
     {
        if (comp->data == internal_ctx)
          {
             LKU(evas_gl->lck);
             return comp;
          }
     }

   ERR("The currently bound context could not be found.");
   LKU(evas_gl->lck);
   return NULL;
}

/**
 * @brief Gets the Evas_GL_Surface currently bound to the calling thread.
 *
 * This function retrieves the Evas_GL_Surface that was made current by
 * evas_gl_make_current() for the given Evas_GL instance.
 *
 * @param evas_gl The Evas_GL object.
 * @return The current Evas_GL_Surface, or @c NULL if no surface is current
 *         for this Evas_GL instance, or if the engine does not support
 *         retrieving the current surface.
 *
 * @see evas_gl_make_current()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Evas_GL_Surface *
evas_gl_current_surface_get(Evas_GL *evas_gl)
{
   Evas_GL_Surface *comp;
   void *internal_sfc;
   Eina_List *li;

   // Magic
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_current_surface_get)
     {
        CRI("Can not get current surface with this engine: %s",
            evas_gl->evas->engine.module->definition->name);
        return NULL;
     }

   internal_sfc = evas_gl->evas->engine.func->gl_current_surface_get(_evas_engine_context(evas_gl->evas));
   if (!internal_sfc)
     return NULL;

   LKL(evas_gl->lck);
   EINA_LIST_FOREACH(evas_gl->surfaces, li, comp)
     {
        if (comp->data == internal_sfc)
          {
             LKU(evas_gl->lck);
             return comp;
          }
     }

   ERR("The currently bound surface could not be found.");
   LKU(evas_gl->lck);
   return NULL;
}

/**
 * @brief Gets the current Evas_GL instance, context, and surface for the calling thread.
 *
 * This function retrieves the Evas_GL instance that is currently active
 * on the calling thread (set via evas_gl_make_current). It can also
 * optionally return the current Evas_GL_Context and Evas_GL_Surface
 * associated with that Evas_GL instance.
 *
 * This is useful when an application has multiple Evas_GL instances and
 * needs to determine which one is currently active for GL operations.
 *
 * @param[out] context If not @c NULL, this will be filled with the current
 *                     Evas_GL_Context for the returned Evas_GL instance.
 *                     Set to @c NULL if no context is current.
 * @param[out] surface If not @c NULL, this will be filled with the current
 *                     Evas_GL_Surface for the returned Evas_GL instance.
 *                     Set to @c NULL if no surface is current.
 * @return The current Evas_GL instance for the calling thread, or @c NULL
 *         if no Evas_GL instance is currently active.
 *
 * @see evas_gl_make_current()
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API Evas_GL *
evas_gl_current_evas_gl_get(Evas_GL_Context **context, Evas_GL_Surface **surface)
{
   Evas_GL *evasgl = NULL;

   if (_current_evas_gl_key)
     evasgl = eina_tls_get(_current_evas_gl_key);

   if (!evasgl)
     {
        if (context) *context = NULL;
        if (surface) *surface = NULL;
        return NULL;
     }

   if (context) *context = evas_gl_current_context_get(evasgl);
   if (surface) *surface = evas_gl_current_surface_get(evasgl);
   return evasgl;
}

/**
 * @brief Queries GLES strings like GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS.
 *
 * This function retrieves GLES string information, similar to glGetString().
 * The actual strings returned depend on the underlying GLES implementation
 * and the current context.
 *
 * @param evas_gl The Evas_GL object.
 * @param name The string to query. This should be one of the Evas_GL_String_Query
 *             enums, which typically map to GLES constants like GL_VENDOR,
 *             GL_RENDERER, GL_VERSION, or GL_EXTENSIONS.
 *             Example: `EVAS_GL_EXTENSIONS` (maps to `GL_EXTENSIONS`)
 * @return A pointer to a null-terminated string containing the requested
 *         information. The string is owned by the GLES driver and should not
 *         be modified or freed. Returns an empty string "" or @c NULL on error
 *         or if the name is not supported.
 *
 * @see Evas_GL_String_Query
 *
 * @ingroup Evas_GL_Group_Info
 */
EVAS_API const char *
evas_gl_string_query(Evas_GL *evas_gl, int name)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return "";
   MAGIC_CHECK_END();

   return evas_gl->evas->engine.func->gl_string_query(_evas_engine_context(evas_gl->evas), name);
}

/**
 * @brief Retrieves the address of a GLES extension function.
 *
 * This function provides a means to get a pointer to a GLES extension
 * function, similar to eglGetProcAddress() or glXGetProcAddress().
 *
 * @param evas_gl The Evas_GL object.
 * @param name The name of the GLES extension function to retrieve.
 *             Example: `"glEGLImageTargetTexture2DOES"`
 * @return A pointer to the function if found, or @c NULL if the function
 *         is not available or an error occurred. The returned pointer
 *         should be cast to the appropriate function pointer type.
 *
 * @ingroup Evas_GL_Group_Info
 */
EVAS_API Evas_GL_Func
evas_gl_proc_address_get(Evas_GL *evas_gl, const char *name)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   return (Evas_GL_Func)evas_gl->evas->engine.func->gl_proc_address_get(_evas_engine_context(evas_gl->evas), name);
}

/**
 * @brief Retrieves native surface information.
 *
 * This function allows querying for underlying native surface details,
 * such as a native window handle or pixmap ID, if applicable and supported
 * by the Evas engine. The content of the Evas_Native_Surface structure
 * is highly platform and engine dependent.
 *
 * @param evas_gl The Evas_GL object.
 * @param surf The Evas_GL_Surface to query. Must not be @c NULL.
 * @param[out] ns A pointer to an Evas_Native_Surface structure to be filled
 *                with the native surface information. Must not be @c NULL.
 *                The `version` field of `ns` should be initialized to
 *                `EVAS_NATIVE_SURFACE_VERSION`.
 *                Example:
 *                @code
 *                Evas_Native_Surface ns;
 *                ns.version = EVAS_NATIVE_SURFACE_VERSION;
 *                if (evas_gl_native_surface_get(evgl, surface, &ns)) {
 *                    // Use ns.type, ns.data.x11.visualid, etc.
 *                }
 *                @endcode
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., invalid
 *         parameters, surface not native, or engine does not support this query).
 *         Use evas_gl_error_get() for specific errors.
 *
 * @see Evas_Native_Surface
 * @see evas_gl_error_get()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Eina_Bool
evas_gl_native_surface_get(Evas_GL *evas_gl, Evas_GL_Surface *surf, Evas_Native_Surface *ns)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EINA_FALSE;
   MAGIC_CHECK_END();

   if (!surf)
     {
        ERR("Invalid surface!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return EINA_FALSE;
     }

   if (!ns)
     {
        ERR("Invalid input parameters!");
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return EINA_FALSE;
     }

   return (Eina_Bool)evas_gl->evas->engine.func->gl_native_surface_get(_evas_engine_context(evas_gl->evas), surf->data, ns);
}

/**
 * @brief Retrieves the GLES 2.x API function table.
 *
 * This function returns a pointer to a structure (Evas_GL_API) containing
 * function pointers for GLES 2.x core functions. This allows direct calling
 * of GLES functions without needing to use evas_gl_proc_address_get() for
 * each one.
 *
 * @note This typically returns the API for GLES 2.X. For other versions,
 *       use evas_gl_context_api_get().
 *
 * @param evas_gl The Evas_GL object.
 * @return A pointer to the Evas_GL_API structure for GLES 2.x, or @c NULL
 *         if an error occurs or GLES 2.x is not supported. The returned
 *         structure is owned by Evas and should not be modified or freed.
 *
 * @see Evas_GL_API
 * @see evas_gl_context_api_get()
 *
 * @ingroup Evas_GL_Group_API
 */
EVAS_API Evas_GL_API *
evas_gl_api_get(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   return (Evas_GL_API*)evas_gl->evas->engine.func->gl_api_get(_evas_engine_context(evas_gl->evas), EVAS_GL_GLES_2_X);
}

/**
 * @brief Retrieves the GLES API function table for a specific context.
 *
 * This function returns a pointer to a structure (Evas_GL_API) containing
 * function pointers for GLES core functions corresponding to the version
 * of the provided Evas_GL_Context.
 *
 * @param evas_gl The Evas_GL object.
 * @param ctx The Evas_GL_Context for which to get the API. Must not be @c NULL.
 *            The version of this context (e.g., GLES 1.x, 2.x, 3.x) determines
 *            which set of API functions is returned.
 * @return A pointer to the Evas_GL_API structure, or @c NULL if an error
 *         occurs (e.g., invalid context, unsupported version). The returned
 *         structure is owned by Evas and should not be modified or freed.
 *
 * @see Evas_GL_API
 * @see evas_gl_api_get() (for default GLES 2.X API)
 * @see Evas_GL_Context_Version
 *
 * @ingroup Evas_GL_Group_API
 */
EVAS_API Evas_GL_API *
evas_gl_context_api_get(Evas_GL *evas_gl, Evas_GL_Context *ctx)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return NULL;
   MAGIC_CHECK_END();

   if (!ctx)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_CONTEXT);
        return NULL;
     }

   return (Evas_GL_API*)evas_gl->evas->engine.func->gl_api_get(_evas_engine_context(evas_gl->evas), ctx->version);
}

/**
 * @brief Gets the rotation angle of the Evas canvas.
 *
 * This function retrieves the current rotation angle (0, 90, 180, or 270 degrees)
 * of the Evas canvas associated with the Evas_GL object. This can be
 * important for GL operations that need to be aware of the canvas orientation.
 *
 * @param evas_gl The Evas_GL object.
 * @return The rotation angle in degrees (0, 90, 180, 270), or 0 if the
 *         engine does not support this query or on error.
 *
 * @ingroup Evas_GL_Group_Info
 */
EVAS_API int
evas_gl_rotation_get(Evas_GL *evas_gl)
{
   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return 0;
   MAGIC_CHECK_END();

   if (!evas_gl->evas->engine.func->gl_rotation_angle_get)
     return 0;

   return evas_gl->evas->engine.func->gl_rotation_angle_get(_evas_engine_context(evas_gl->evas));
}

/**
 * @brief Retrieves the last Evas GL error code.
 *
 * This function returns the last error code set by an Evas GL API call
 * for the current thread and the given Evas_GL instance.
 * It first checks for Evas GL specific errors stored in thread-local storage,
 * and if none, queries the underlying engine for its last GL error.
 *
 * Calling this function resets the Evas GL specific error state for the
 * current thread to EVAS_GL_SUCCESS. However, it does not necessarily
 * clear the underlying GLES error state (e.g., from glGetError()).
 *
 * @param evas_gl The Evas_GL object.
 * @return The last error code. See @ref Evas_GL_Error_Codes for possible values.
 *         Returns EVAS_GL_NOT_INITIALIZED if evas_gl is invalid or the
 *         engine function for error checking is unavailable.
 *
 * @see Evas_GL_Error_Codes
 *
 * @ingroup Evas_GL_Group_Context
 */
EVAS_API int
evas_gl_error_get(Evas_GL *evas_gl)
{
   int err;

   MAGIC_CHECK(evas_gl, Evas_GL, MAGIC_EVAS_GL);
   return EVAS_GL_NOT_INITIALIZED;
   MAGIC_CHECK_END();

   if ((err = _evas_gl_internal_error_get(evas_gl)) != EVAS_GL_SUCCESS)
     goto end;

   if (!evas_gl->evas->engine.func->gl_error_get)
     err = EVAS_GL_NOT_INITIALIZED;
   else
     err = evas_gl->evas->engine.func->gl_error_get(_evas_engine_context(evas_gl->evas));

end:
   /* Call to evas_gl_error_get() should set error to EVAS_GL_SUCCESS */
   _evas_gl_internal_error_set(evas_gl, EVAS_GL_SUCCESS);
   return err;
}

/**
 * @brief Queries attributes of an Evas_GL_Surface.
 *
 * This function allows querying various attributes of a given Evas_GL_Surface,
 * such as its width, height, or pixel format. The available attributes
 * and the type of data returned in @p value depend on the @p attribute queried.
 *
 * @param evas_gl The Evas_GL object.
 * @param surface The Evas_GL_Surface to query. Must not be @c NULL.
 * @param attribute The attribute to query. This should be one of the
 *                  Evas_GL_ पृष्ठाAttribute enums (e.g., EVAS_GL_WIDTH,
 *                  EVAS_GL_HEIGHT, EVAS_GL_COLOR_FORMAT).
 *                  Example: `EVAS_GL_WIDTH`
 * @param[out] value A pointer to a variable where the attribute's value will be
 *                   stored. The type of this variable depends on the attribute.
 *                   For example, for EVAS_GL_WIDTH, `value` should be an `int*`.
 *                   Example:
 *                   @code
 *                   int width;
 *                   if (evas_gl_surface_query(evgl, surf, EVAS_GL_WIDTH, &width)) {
 *                       // use width
 *                   }
 *                   @endcode
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *         Failures can occur due to invalid parameters, an unsupported attribute,
 *         or if the engine does not support surface queries.
 *         Use evas_gl_error_get() for specific errors.
 *
 * @see Evas_GL_ पृष्ठाAttribute
 * @see evas_gl_error_get()
 *
 * @ingroup Evas_GL_Group_Surface
 */
EVAS_API Eina_Bool
evas_gl_surface_query(Evas_GL *evas_gl, Evas_GL_Surface *surface, int attribute, void *value)
{
   if (!evas_gl) return EINA_FALSE;
   if (!surface)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_SURFACE);
        return EINA_FALSE;
     }

   if (!evas_gl->evas->engine.func->gl_surface_query)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_NOT_INITIALIZED);
        return EINA_FALSE;
     }

   if (!value)
     {
        _evas_gl_internal_error_set(evas_gl, EVAS_GL_BAD_PARAMETER);
        return EINA_FALSE;
     }

   return evas_gl->evas->engine.func->gl_surface_query
         (_evas_engine_context(evas_gl->evas), surface->data, attribute, value);
}
