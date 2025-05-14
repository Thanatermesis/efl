#ifndef _EVAS_GL_CORE_H
#define _EVAS_GL_CORE_H
#define EVAS_GL_NO_GL_H_CHECK 1
#include "Evas_GL.h"

#ifdef EMODAPI
# undef EMODAPI
#endif

#ifdef _WIN32
# ifndef EFL_MODULE_STATIC
#  define EMODAPI __declspec(dllexport)
# else
#  define EMODAPI
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EMODAPI __attribute__ ((visibility("default")))
#  else
#   define EMODAPI
#  endif
# else
#  define EMODAPI
# endif
#endif

typedef void *EVGLNative_Display;
typedef void *EVGLNative_Window;
typedef void *EVGLNative_Surface;
typedef void *EVGLNative_Context;
typedef struct _EVGL_Engine         EVGL_Engine;
typedef struct _EVGL_Interface      EVGL_Interface;
typedef struct _EVGL_Surface        EVGL_Surface;
typedef struct _EVGL_Native_Window  EVGL_Native_Window;
typedef struct _EVGL_Context        EVGL_Context;
typedef struct _EVGL_Resource       EVGL_Resource;
typedef struct _EVGL_Cap            EVGL_Cap;
typedef struct _EVGL_Surface_Cap    EVGL_Surface_Cap;
typedef struct _EVGL_Surface_Format EVGL_Surface_Format;

/**
 * @brief Shuts down the Evas GL engine.
 *
 * This function cleans up all resources allocated by the Evas GL engine,
 * including surfaces, contexts, and internal resources. It should be called
 * when the Evas GL engine is no longer needed.
 *
 * @param eng_data A pointer to the engine-specific data.
 */
EMODAPI void         evgl_engine_shutdown(void *eng_data);
/**
 * @brief Retrieves the native surface buffer.
 *
 * This function returns a handle to the native buffer of a given Evas GL surface.
 * The buffer can be either an EGL image or a GL texture ID.
 *
 * @param sfc A pointer to the Evas GL surface.
 * @param is_egl_image A pointer to a boolean that will be set to EINA_TRUE if
 *        the returned buffer is an EGL image, EINA_FALSE otherwise.
 * @return A handle to the native surface buffer, or NULL on failure.
 */
EMODAPI void        *evgl_native_surface_buffer_get(EVGL_Surface *sfc, Eina_Bool *is_egl_image);
/**
 * @brief Gets the Y-inversion property of a native surface.
 *
 * This function returns whether the native surface is Y-inverted.
 * This is relevant for indirect rendering scenarios.
 *
 * @param sfc A pointer to the Evas GL surface.
 * @return An integer indicating if Y-inversion is enabled (1) or not (0).
 */
EMODAPI int          evgl_native_surface_yinvert_get(EVGL_Surface *sfc);
/**
 * @brief Gets the current native context.
 *
 * This function retrieves the underlying native context handle from an
 * Evas GL context. This can be an EGL context or another platform-specific
 * context handle.
 *
 * @param ctx A pointer to the Evas GL context.
 * @return A handle to the native context, or NULL if no context is current.
 */
EMODAPI void        *evgl_current_native_context_get(EVGL_Context *ctx);
/**
 * @brief Enables or disables the context restore mechanism.
 *
 * This function controls whether the GL context state should be restored
 * after certain operations. This is a global setting.
 *
 * @param enable EINA_TRUE to enable context restore, EINA_FALSE to disable.
 */
EMODAPI void         evas_gl_common_context_restore_set(Eina_Bool);

typedef void (*EVGL_Engine_Call)(void *eng_data);
typedef void *(*EVGL_Native_Surface_Call)(void *sfc, Eina_Bool *is_egl_image);
typedef int (*EVGL_Native_Surface_Yinvert_Call)(void *sfc);
typedef void *(*EVGL_Current_Native_Context_Get_Call)(void *ctx);

/**
 * @brief Initializes the Evas GL engine.
 *
 * This function sets up the Evas GL engine with a given set of interface
 * functions provided by the underlying graphics engine (e.g., EGL, GLX).
 * It must be called before any other Evas GL functions.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param efunc A pointer to the structure of engine interface functions.
 * @return A pointer to the initialized EVGL_Engine structure, or NULL on failure.
 */
EVGL_Engine *evgl_engine_init(void *eng_data, const EVGL_Interface *efunc);

/**
 * @brief Creates an Evas GL surface.
 *
 * This function creates an off-screen rendering surface (typically an FBO)
 * with the specified dimensions and configuration.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param cfg A pointer to the Evas_GL_Config structure defining the surface properties.
 * @param w The width of the surface.
 * @param h The height of the surface.
 * @return A pointer to the created EVGL_Surface, or NULL on failure.
 */
void        *evgl_surface_create(void *eng_data, Evas_GL_Config *cfg, int w, int h);
/**
 * @brief Creates an Evas GL PBuffer surface.
 *
 * This function creates a pixel buffer (PBuffer) surface. PBuffers are
 * off-screen rendering surfaces that are part of the native platform's windowing system interface.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param cfg A pointer to the Evas_GL_Config structure defining the surface properties.
 * @param w The width of the surface.
 * @param h The height of the surface.
 * @param attrib_list A list of attributes for the PBuffer, terminated by a specific value (like EGL_NONE).
 * @return A pointer to the created EVGL_Surface for the PBuffer, or NULL on failure.
 */
void        *evgl_pbuffer_surface_create(void *eng_data, Evas_GL_Config *cfg, int w, int h, const int *attrib_list);
/**
 * @brief Destroys an Evas GL surface.
 *
 * This function frees all resources associated with an Evas GL surface,
 * including any allocated buffers.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param sfc A pointer to the Evas GL surface to destroy.
 * @return 1 on success, 0 on failure.
 */
int          evgl_surface_destroy(void *eng_data, EVGL_Surface *sfc);
/**
 * @brief Creates an Evas GL context.
 *
 * This function creates a new GL rendering context.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param share_ctx A pointer to another Evas GL context with which to share resources. Can be NULL.
 * @param version The GLES version for this context (e.g., EVAS_GL_GLES_2_X).
 * @param native_context_get A function pointer to retrieve the native context.
 * @param engine_data_get A function pointer to retrieve engine data.
 * @return A pointer to the created EVGL_Context, or NULL on failure.
 */
void        *evgl_context_create(void *eng_data, EVGL_Context *share_ctx, Evas_GL_Context_Version version, void *(*native_context_get)(void *), void *(*engine_data_get)(void *));
/**
 * @brief Destroys an Evas GL context.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param ctx A pointer to the Evas GL context to destroy.
 * @return 1 on success, 0 on failure.
 */
int          evgl_context_destroy(void *eng_data, EVGL_Context *ctx);
/**
 * @brief Makes an Evas GL context current with a surface.
 *
 * This function binds a rendering context to a drawing surface, making it the
 * target for all subsequent GL rendering commands.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param sfc A pointer to the Evas GL surface. Can be NULL for surfaceless context.
 * @param ctx A pointer to the Evas GL context.
 * @return 1 on success, 0 on failure.
 */
int          evgl_make_current(void *eng_data, EVGL_Surface *sfc, EVGL_Context *ctx);

/**
 * @brief Queries Evas GL strings.
 *
 * This function retrieves information strings from the GL implementation,
 * such as the list of supported extensions.
 *
 * @param name The name of the string to query (e.g., EVAS_GL_EXTENSIONS).
 * @return A pointer to the requested string. The caller must not free this string.
 */
const char  *evgl_string_query(int name);
/**
 * @brief Gets an Evas_Native_Surface from an EVGL_Surface.
 *
 * This function populates an Evas_Native_Surface structure, which is a generic
 * way to pass native surface information within Evas.
 *
 * @param sfc A pointer to the Evas GL surface.
 * @param ns A pointer to the Evas_Native_Surface structure to populate.
 * @return 1 on success, 0 on failure.
 */
int          evgl_native_surface_get(EVGL_Surface *sfc, Evas_Native_Surface *ns);
/**
 * @brief Gets the Evas_GL_API structure for a given GLES version.
 *
 * This function returns a structure containing function pointers to the
 * GL API functions for a specific GLES version. It handles loading these
 * functions on the first call.
 *
 * @param eng_data A pointer to the engine-specific data.
 * @param version The GLES version for which to get the API.
 * @param alloc_only If EINA_TRUE, only allocates the structure without resolving function pointers.
 * @return A pointer to the Evas_GL_API structure.
 */
Evas_GL_API *evgl_api_get(void *eng_data, Evas_GL_Context_Version version, Eina_Bool alloc_only);
/**
 * @brief Whitelists a GL extension function.
 *
 * This function adds a GL extension function to a list of "safe" extensions that
 * can be used. This is a security/stability feature.
 *
 * @param name The name of the extension function.
 * @param funcptr A pointer to the function.
 */
void         evgl_safe_extension_add(const char *name, void *funcptr);
/**
 * @brief Checks if a GL extension function is whitelisted.
 *
 * @param name The name of the extension function.
 * @param pfuncptr A pointer to a void pointer that will receive the function pointer if found.
 * @return EINA_TRUE if the function is on the safe list, EINA_FALSE otherwise.
 */
Eina_Bool    evgl_safe_extension_get(const char *name, void **pfuncptr);

/**
 * @brief Checks if rendering is currently direct.
 *
 * Direct rendering bypasses intermediate FBOs and renders directly to the
 * back buffer.
 *
 * @return 1 if direct rendering is active, 0 otherwise.
 */
int          evgl_direct_rendered(void);
/**
 * @brief Gets the direct rendering override settings.
 *
 * @param override A pointer to a boolean that will be set to EINA_TRUE if direct rendering is forced.
 * @param force_off A pointer to a boolean that will be set to EINA_TRUE if direct rendering is forcibly disabled.
 */
void         evgl_direct_override_get(Eina_Bool *override, Eina_Bool *force_off);
/**
 * @brief Sets information for direct rendering.
 *
 * This function provides the GL engine with the necessary geometry and
 * state information to perform direct rendering.
 *
 * @param win_w Window width.
 * @param win_h Window height.
 * @param rot Window rotation angle.
 * @param img_x Image X coordinate.
 * @param img_y Image Y coordinate.
 * @param img_w Image width.
 * @param img_h Image height.
 * @param clip_x Clip rectangle X.
 * @param clip_y Clip rectangle Y.
 * @param clip_w Clip rectangle width.
 * @param clip_h Clip rectangle height.
 * @param render_op The Evas render operation (e.g., EVAS_RENDER_COPY).
 * @param surface A pointer to the EVGL_Surface being rendered.
 */
void         evgl_direct_info_set(int win_w, int win_h, int rot,
                                  int img_x, int img_y, int img_w, int img_h,
                                  int clip_x, int clip_y, int clip_w, int clip_h,
                                  int render_op, void *surface);
/**
 * @brief Clears the direct rendering information.
 *
 * This disables direct rendering by clearing the information set by
 * evgl_direct_info_set().
 */
void         evgl_direct_info_clear(void);
/**
 * @brief Notifies the engine that a get_pixels operation is about to start.
 *
 * This is a hint to the engine to prepare for a glReadPixels-like operation,
 * which may require special handling in direct rendering scenarios.
 */
void         evgl_get_pixels_pre(void);
/**
 * @brief Notifies the engine that a get_pixels operation has finished.
 */
void         evgl_get_pixels_post(void);

/**
 * @brief Gets direct rendering options from a native surface.
 *
 * This function checks a native surface for properties that affect direct
 * rendering, such as client-side rotation support.
 *
 * @param ns The native surface.
 * @param direct_render Output: whether direct rendering is possible.
 * @param client_side_rotation Output: whether client-side rotation is supported.
 * @param direct_override Output: whether direct rendering is forced.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool    evgl_native_surface_direct_opts_get(Evas_Native_Surface *ns,
                                                 Eina_Bool *direct_render,
                                                 Eina_Bool *client_side_rotation,
                                                 Eina_Bool *direct_override);

/**
 * @brief Sets information for partial rendering (tiling).
 *
 * @param pres The preservation flag for partial rendering. For example, on
 *        Qualcomm SoCs, this might be `GL_COLOR_BUFFER_BIT0_QCOM`.
 */
void         evgl_direct_partial_info_set(int pres);
/**
 * @brief Clears information for partial rendering.
 */
void         evgl_direct_partial_info_clear(void);
/**
 * @brief Starts a partial rendering operation (tiling).
 *
 * This function sets up the GL state (viewport, scissor) for rendering a
 * tile of a larger surface. It is used to implement partial updates.
 */
void         evgl_direct_partial_render_start(void);
/**
 * @brief Ends a partial rendering operation.
 */
void         evgl_direct_partial_render_end(void);

#endif //_EVAS_GL_CORE_H
