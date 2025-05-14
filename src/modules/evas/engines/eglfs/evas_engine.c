/**
 * @file
 * @brief Evas EGLFS engine implementation.
 *
 * This engine provides Evas rendering capabilities using EGL on a framebuffer device.
 * It leverages Evas_GL_Generic for common GL operations and extends it with
 * EGLFS specific functionalities.
 */

#include "config.h"
#include "evas_engine.h"
#include <wayland-client.h>

#ifdef HAVE_DLSYM
# include <dlfcn.h>      /* dlopen,dlclose,etc */
#else
# error eglfs should not get compiled if dlsym is not found on the system!
#endif

#define EVAS_GL_NO_GL_H_CHECK 1
#include "Evas_GL.h"

#define EVAS_GL_UPDATE_TILE_SIZE 16

#ifndef EGL_NATIVE_PIXMAP_KHR
# define EGL_NATIVE_PIXMAP_KHR 0x30b0
#endif

/* external variables */
int _evas_engine_eglfs_log_dom = -1;
int _extn_have_buffer_age = 1;

/* local variables */
static Eina_Bool initted = EINA_FALSE;
static int gl_wins = 0;

/* local structures */
/**
 * @brief Structure representing the EGLFS rendering engine.
 *
 * This structure holds the generic GL rendering output information.
 */
typedef struct _Render_Engine Render_Engine;
struct _Render_Engine
{
   Render_Output_GL_Generic generic; /**< Generic GL rendering output data. */
};

/**
 * @brief Structure representing a native surface.
 *
 * This structure holds information about a native surface, which can be
 * a Wayland buffer or an EGL surface.
 */
typedef struct _Native Native;
struct _Native
{
   Evas_Native_Surface ns;      /**< Evas native surface information. */
   struct wl_buffer *wl_buf;  /**< Wayland buffer, if applicable. */
   void *egl_surface;         /**< EGL surface, if applicable. */
};

/* local function prototype types */
typedef void (*_eng_fn)(void);
typedef _eng_fn (*glsym_func_eng_fn)();
typedef void (*glsym_func_void)();
typedef void *(*glsym_func_void_ptr)();
typedef int (*glsym_func_int)();
typedef unsigned int (*glsym_func_uint)();
typedef const char *(*glsym_func_const_char_ptr)();

/* external dynamic loaded Evas_GL function pointers */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_ref = NULL;
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_unref = NULL;
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_free = NULL;
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_native_disable = NULL;
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_native_enable = NULL;
Evas_GL_Common_Image_New_From_Data glsym_evas_gl_common_image_new_from_data = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_image_all_unload = NULL;
Evas_GL_Preload glsym_evas_gl_preload_init = NULL;
Evas_GL_Preload glsym_evas_gl_preload_shutdown = NULL;
EVGL_Engine_Call glsym_evgl_engine_shutdown = NULL;
EVGL_Current_Native_Context_Get_Call glsym_evgl_current_native_context_get = NULL;
Evas_Gl_Symbols glsym_evas_gl_symbols = NULL;

Evas_GL_Common_Context_New glsym_evas_gl_common_context_new = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe = NULL;
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done = NULL;
Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize = NULL;
Evas_GL_Common_Buffer_Dump_Call glsym_evas_gl_common_buffer_dump = NULL;
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock = NULL;
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock = NULL;
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_relax = NULL;

glsym_func_void_ptr glsym_evas_gl_common_current_context_get = NULL;

/* dynamic loaded local egl function pointers */
_eng_fn (*glsym_eglGetProcAddress)(const char *a) = NULL;
void *(*glsym_eglCreateImage)(EGLDisplay a, EGLContext b, EGLenum c, EGLClientBuffer d, const int *e) = NULL;
void (*glsym_eglDestroyImage)(EGLDisplay a, void *b) = NULL;
void (*glsym_glEGLImageTargetTexture2DOES)(int a, void *b)  = NULL;
unsigned int (*glsym_eglSwapBuffersWithDamage)(EGLDisplay a, void *b, const EGLint *d, EGLint c) = NULL;
unsigned int (*glsym_eglQueryWaylandBufferWL)(EGLDisplay a, struct wl_resource *b, EGLint c, EGLint *d) = NULL;

/* local function prototypes */
/**
 * @brief Loads necessary EGL and GLES symbols.
 *
 * This function dynamically loads EGL and GLES function pointers required
 * by the engine. It ensures that all necessary symbols are available before
 * they are used.
 */
static void gl_symbols(void);

/**
 * @brief Checks and disables certain EGL extensions based on environment or availability.
 *
 * This function queries available EGL extensions and may disable features like
 * partial rendering (buffer age) or swap-with-damage if explicitly disabled
 * via environment variables or if the extensions are not supported.
 *
 * @param re Pointer to the Render_Engine structure.
 */
static void gl_extn_veto(Render_Engine *re);

/**
 * @brief Retrieves the EGL display associated with the render engine.
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the EGLDisplay, or NULL on error.
 */
static void *evgl_eng_display_get(void *data);

/**
 * @brief Retrieves the EGL surface associated with the Evas canvas.
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the EGLSurface, or NULL on error.
 */
static void *evgl_eng_evas_surface_get(void *data);

/**
 * @brief Makes the given EGL context and surface current.
 * @param data Pointer to the Render_Engine structure.
 * @param surface The EGLSurface to make current. Can be EGL_NO_SURFACE.
 * @param context The EGLContext to make current. Can be EGL_NO_CONTEXT.
 * @param flush If non-zero, flushes the previous context before making the new one current.
 * @return 1 on success, 0 on failure.
 */
static int evgl_eng_make_current(void *data, void *surface, void *context, int flush);

/**
 * @brief Creates a native window.
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the native window (EGLNativeWindowType), or NULL on error.
 * @note This function currently creates an HWC (Hardware Composer) native window.
 */
static void *evgl_eng_native_window_create(void *data);

/**
 * @brief Destroys a native window.
 * @param data Pointer to the Render_Engine structure.
 * @param native_window The native window to destroy.
 * @return 1 on success, 0 on failure.
 */
static int evgl_eng_native_window_destroy(void *data, void *native_window);

/**
 * @brief Creates an EGL window surface from a native window.
 * @param data Pointer to the Render_Engine structure.
 * @param native_window The native window (EGLNativeWindowType) to create the surface from.
 * @return A void pointer to the EGLSurface, or NULL on error.
 */
static void *evgl_eng_window_surface_create(void *data, void *native_window);

/**
 * @brief Destroys an EGL window surface.
 * @param data Pointer to the Render_Engine structure.
 * @param surface The EGLSurface to destroy.
 * @return 1 on success, 0 on failure.
 */
static int evgl_eng_window_surface_destroy(void *data, void *surface);

/**
 * @brief Creates an EGL context.
 * @param data Pointer to the Render_Engine structure.
 * @param share_ctx An EGLContext to share resources with, or NULL for no sharing (shares with Evas' main GL context by default).
 * @param version The GLES version for the context (currently only EVAS_GL_GLES_2_X is supported).
 * @return A void pointer to the EGLContext, or NULL on error.
 */
static void *evgl_eng_context_create(void *data, void *share_ctx, Evas_GL_Context_Version version);

/**
 * @brief Destroys an EGL context.
 * @param data Pointer to the Render_Engine structure.
 * @param context The EGLContext to destroy.
 * @return 1 on success, 0 on failure.
 */
static int evgl_eng_context_destroy(void *data, void *context);

/**
 * @brief Retrieves the EGL extension string.
 * @param data Pointer to the Render_Engine structure.
 * @return A const char pointer to the EGL extension string, or NULL on error.
 */
static const char *evgl_eng_string_get(void *data);

/**
 * @brief Retrieves the address of an EGL or GLES extension function.
 * @param name The name of the function to retrieve.
 * @return A void pointer to the function, or NULL if not found.
 */
static void *evgl_eng_proc_address_get(const char *name);

/**
 * @brief Retrieves the current rotation angle of the Evas canvas.
 * @param data Pointer to the Render_Engine structure.
 * @return The rotation angle in degrees (0, 90, 180, 270), or 0 on error.
 */
static int evgl_eng_rotation_angle_get(void *data);

/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;
static const EVGL_Interface evgl_funcs =
{
   evgl_eng_display_get,
   evgl_eng_evas_surface_get,
   evgl_eng_native_window_create,
   evgl_eng_native_window_destroy,
   evgl_eng_window_surface_create,
   evgl_eng_window_surface_destroy,
   evgl_eng_context_create,
   evgl_eng_context_destroy,
   evgl_eng_make_current,
   evgl_eng_proc_address_get,
   evgl_eng_string_get,
   evgl_eng_rotation_angle_get,
   NULL, // PBuffer
   NULL, // PBuffer
   NULL, // OpenGL-ES 1
   NULL, // OpenGL-ES 1
   NULL, // OpenGL-ES 1
   NULL, // native_win_surface_config_get
};


/* local inline functions */
static inline Outbuf *
eng_get_ob(Render_Engine *re)
{
   return re->generic.software.ob;
}

/* local functions */
/**
 * @brief Loads necessary EGL and GLES symbols.
 *
 * This function dynamically loads EGL and GLES function pointers required
 * by the engine. It ensures that all necessary symbols are available before
 * they are used. It uses dlsym and eglGetProcAddress to find the symbols.
 * This function is called only once.
 */
static void
gl_symbols(void)
{
   static Eina_Bool done = EINA_FALSE;
   const char *exts = NULL;

   if (done) return;

#define LINK2GENERIC(sym) \
   glsym_##sym = dlsym(RTLD_DEFAULT, #sym);

   // Get function pointer to evas_gl_common that is now provided through the link of GL_Generic.
   LINK2GENERIC(evas_gl_common_image_all_unload);
   LINK2GENERIC(evas_gl_common_image_ref);
   LINK2GENERIC(evas_gl_common_image_unref);
   LINK2GENERIC(evas_gl_common_image_new_from_data);
   LINK2GENERIC(evas_gl_common_image_native_disable);
   LINK2GENERIC(evas_gl_common_image_free);
   LINK2GENERIC(evas_gl_common_image_native_enable);
   LINK2GENERIC(evas_gl_common_context_new);
   LINK2GENERIC(evas_gl_common_context_flush);
   LINK2GENERIC(evas_gl_common_context_free);
   LINK2GENERIC(evas_gl_common_context_use);
   LINK2GENERIC(evas_gl_common_context_newframe);
   LINK2GENERIC(evas_gl_common_context_done);
   LINK2GENERIC(evas_gl_common_context_resize);
   LINK2GENERIC(evas_gl_common_buffer_dump);
   LINK2GENERIC(evas_gl_preload_render_lock);
   LINK2GENERIC(evas_gl_preload_render_unlock);
   LINK2GENERIC(evas_gl_preload_render_relax);
   LINK2GENERIC(evas_gl_preload_init);
   LINK2GENERIC(evas_gl_preload_shutdown);
   LINK2GENERIC(evgl_engine_shutdown);
   LINK2GENERIC(evas_gl_symbols);

#define FINDSYM(dst, sym, typ) \
   if (glsym_eglGetProcAddress) { \
      if (!dst) dst = (typ)glsym_eglGetProcAddress(sym); \
   } else { \
      if (!dst) dst = (typ)dlsym(RTLD_DEFAULT, sym); \
   }

   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressKHR", glsym_func_eng_fn);
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressEXT", glsym_func_eng_fn);
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressARB", glsym_func_eng_fn);
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddress", glsym_func_eng_fn);

   // Find EGL extensions
   // FIXME: whgen above eglGetDisplay() is fixed... fix the below...
//   exts = eglQueryString(ob->egl_disp, EGL_EXTENSIONS);
   glsym_evas_gl_symbols((void*)glsym_eglGetProcAddress, exts);

   FINDSYM(glsym_eglCreateImage, "eglCreateImageKHR", glsym_func_void_ptr);
   FINDSYM(glsym_eglCreateImage, "eglCreateImageEXT", glsym_func_void_ptr);
   FINDSYM(glsym_eglCreateImage, "eglCreateImageARB", glsym_func_void_ptr);
   FINDSYM(glsym_eglCreateImage, "eglCreateImage", glsym_func_void_ptr);

   FINDSYM(glsym_eglDestroyImage, "eglDestroyImageKHR", glsym_func_void);
   FINDSYM(glsym_eglDestroyImage, "eglDestroyImageEXT", glsym_func_void);
   FINDSYM(glsym_eglDestroyImage, "eglDestroyImageARB", glsym_func_void);
   FINDSYM(glsym_eglDestroyImage, "eglDestroyImage", glsym_func_void);

   FINDSYM(glsym_glEGLImageTargetTexture2DOES,
           "glEGLImageTargetTexture2DOES", glsym_func_void);

   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamageEXT",
           glsym_func_uint);
   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamageINTEL",
           glsym_func_uint);
   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamage",
           glsym_func_uint);

   FINDSYM(glsym_eglQueryWaylandBufferWL, "eglQueryWaylandBufferWL",
           glsym_func_uint);

   done = EINA_TRUE;
}

/**
 * @brief Checks and disables certain EGL extensions based on environment or availability.
 *
 * This function queries available EGL extensions and may disable features like
 * partial rendering (buffer age) or swap-with-damage if explicitly disabled
 * via environment variables (EVAS_GL_PARTIAL_DISABLE) or if the extensions
 * (EGL_EXT_buffer_age, EGL_EXT_swap_buffers_with_damage, EGL_KHR_swap_buffers_with_damage)
 * are not supported by the EGL implementation.
 *
 * @param re Pointer to the Render_Engine structure, used to get the EGL display.
 */
static void
gl_extn_veto(Render_Engine *re)
{
   const char *str = NULL;

   str = eglQueryString(eng_get_ob(re)->egl.disp, EGL_EXTENSIONS);
   if (str)
     {
        const char *s = NULL;

        if (getenv("EVAS_GL_INFO")) printf("EGL EXTN:\n%s\n", str);

        // Disable Partial Rendering
        s = getenv("EVAS_GL_PARTIAL_DISABLE");
        if ((s) && (atoi(s)))
          {
             _extn_have_buffer_age = 0;
             glsym_eglSwapBuffersWithDamage = NULL;
          }
        if (!strstr(str, "EGL_EXT_buffer_age")) _extn_have_buffer_age = 0;
        if ((!strstr(str, "EGL_EXT_swap_buffers_with_damage")) &&
            (!strstr(str, "EGL_KHR_swap_buffers_with_damage")))
          glsym_eglSwapBuffersWithDamage = NULL;
     }
   else
     {
        if (getenv("EVAS_GL_INFO")) printf("NO EGL EXTN!\n");
        _extn_have_buffer_age = 0;
     }
}

/**
 * @brief Retrieves the EGL display associated with the render engine.
 * Implements the EVGL_Interface::display_get function.
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the EGLDisplay, or NULL if data is invalid or
 *         the output buffer or EGL display is not available.
 */
static void *
evgl_eng_display_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   if (eng_get_ob(re))
     return (void *)eng_get_ob(re)->egl.disp;
   else
     return NULL;
}

/**
 * @brief Retrieves the EGL surface associated with the Evas canvas.
 * Implements the EVGL_Interface::evas_surface_get function.
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the EGLSurface (specifically the first surface
 *         in the output buffer's EGL surface array), or NULL if data is invalid
 *         or the output buffer or EGL surface is not available.
 */
static void *
evgl_eng_evas_surface_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   if (eng_get_ob(re))
     return (void *)eng_get_ob(re)->egl.surface[0];
   else
     return NULL;
}

/**
 * @brief Makes the given EGL context and surface current for the calling thread.
 * Implements the EVGL_Interface::make_current function.
 *
 * If both context and surface are NULL, it makes no context/surface current.
 * Otherwise, it makes the specified context and surface current.
 * If `flush` is true, it ensures that any pending operations on the
 * previous Evas output buffer are completed.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param surface The EGLSurface to make current. Can be EGL_NO_SURFACE.
 *                Example: (EGLSurface)my_egl_surface
 * @param context The EGLContext to make current. Can be EGL_NO_CONTEXT.
 *                Example: (EGLContext)my_egl_context
 * @param flush If non-zero (true), flushes the previous context (evas_outbuf_use(NULL))
 *              before making the new one current.
 * @return 1 on success, 0 on failure (e.g., eglMakeCurrent fails or invalid data).
 */
static int
evgl_eng_make_current(void *data, void *surface, void *context, int flush)
{
   Render_Engine *re;
   EGLContext ctx;
   EGLSurface sfc;
   EGLDisplay dpy;
   int ret = 0;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return 0;
     }

   dpy = eng_get_ob(re)->egl.disp;
   ctx = (EGLContext)context;
   sfc = (EGLSurface)surface;

   if ((!context) && (!surface))
     {
        ret = eglMakeCurrent(dpy, EGL_NO_SURFACE,
                             EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (!ret)
          {
             ERR("eglMakeCurrent() failed! Error Code=%#x", eglGetError());
             return 0;
          }

        return 1;
     }

   if ((eglGetCurrentContext() != ctx) ||
       (eglGetCurrentSurface(EGL_READ) != sfc) ||
       (eglGetCurrentSurface(EGL_DRAW) != sfc) )
     {
        if (flush) evas_outbuf_use(NULL);

        ret = eglMakeCurrent(dpy, sfc, sfc, ctx);
        if (!ret)
          {
             ERR("eglMakeCurrent() failed! Error Code=%#x", eglGetError());
             return 0;
          }
     }

   return 1;
}

/**
 * @brief Callback for HWC (Hardware Composer) present operations.
 *
 * This function is intended as a callback for ANativeWindow present operations,
 * but it is currently empty and does not perform any actions.
 *
 * @param user_data User-defined data (unused).
 * @param window The ANativeWindow associated with the event (unused).
 * @param buffer The ANativeWindowBuffer being presented (unused).
 */
static void _hwc_present_cb(void *user_data, struct ANativeWindow *window,
                            struct ANativeWindowBuffer *buffer)
{

}

/**
 * @brief Creates a native window suitable for EGL rendering.
 * Implements the EVGL_Interface::native_window_create function.
 *
 * This function currently creates an HWC (Hardware Composer) native window
 * using `create_hwcomposernativewindow()`.
 *
 * @param data Pointer to the Render_Engine structure.
 * @return A void pointer to the created native window (EGLNativeWindowType),
 *         or NULL if data is invalid, Evas engine info is missing, or
 *         native window creation fails.
 */
static void *
evgl_eng_native_window_create(void *data)
{
   Render_Engine *re;
   Evas_Engine_Info_Eglfs *info;
   struct ANativeWindow *native_window;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   info = eng_get_ob(re)->info;
   if (!info)
     {
        ERR("Invalid Evas Engine Eglfs Info!");
        return NULL;
     }
   EGLNativeWindowType win;
   win = create_hwcomposernativewindow();
   return (void *)win;
}

/**
 * @brief Destroys a previously created native window.
 * Implements the EVGL_Interface::native_window_destroy function.
 *
 * This function destroys an HWC (Hardware Composer) native window
 * using `HWCNativeWindowDestroy()`.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param native_window The native window (EGLNativeWindowType) to destroy.
 *                      Example: (EGLNativeWindowType)my_native_window
 * @return 1 on success, 0 if data or native_window is invalid.
 */
static int
evgl_eng_native_window_destroy(void *data, void *native_window)
{
   Render_Engine *re = (Render_Engine *)data;

   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return 0;
     }

   if (!native_window)
     {
        ERR("Invalid native surface.");
        return 0;
     }

   HWCNativeWindowDestroy(native_window);

   return 1;
}

/**
 * @brief Creates an EGL window surface from a given native window.
 * Implements the EVGL_Interface::window_surface_create function.
 *
 * Uses `eglCreateWindowSurface` to create the EGL surface.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param native_window The native window (EGLNativeWindowType) from which to create the surface.
 *                      Example: (EGLNativeWindowType)my_native_window
 * @return A void pointer to the created EGLSurface, or NULL if data is invalid,
 *         or if `eglCreateWindowSurface` fails.
 */
static void *
evgl_eng_window_surface_create(void *data, void *native_window)
{
   Render_Engine *re;
   EGLSurface surface = EGL_NO_SURFACE;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   // Create resource surface for EGL
   surface = eglCreateWindowSurface(eng_get_ob(re)->egl.disp,
                                    eng_get_ob(re)->egl.config,
                                    (EGLNativeWindowType)native_window,
                                    NULL);
   if (!surface)
     {
        ERR("Creating window surface failed. Error: %#x.", eglGetError());
        return NULL;
     }

   return (void *)surface;
}

/**
 * @brief Destroys an EGL window surface.
 * Implements the EVGL_Interface::window_surface_destroy function.
 *
 * Uses `eglDestroySurface` to destroy the EGL surface.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param surface The EGLSurface to destroy.
 *                Example: (EGLSurface)my_egl_surface
 * @return 1 on success (EGL_TRUE from eglDestroySurface), 0 if data or surface is invalid,
 *         or if `eglDestroySurface` fails.
 */
static int
evgl_eng_window_surface_destroy(void *data, void *surface)
{
   Render_Engine *re;
   EGLBoolean ret = EGL_FALSE;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return 0;
     }

   if (!surface)
     {
        ERR("Invalid surface.");
        return 0;
     }

   ret = eglDestroySurface(eng_get_ob(re)->egl.disp, (EGLSurface)surface);
   if (ret == EGL_TRUE) return 1;

   return 0;
}

/**
 * @brief Creates an EGL context.
 * Implements the EVGL_Interface::context_create function.
 *
 * This engine currently only supports creating OpenGL ES 2.0 contexts.
 * The created context will share resources with `share_ctx`. If `share_ctx` is NULL,
 * it shares with the main Evas GL context.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param share_ctx An EGLContext to share resources with. If NULL, shares with
 *                  the Evas' main GL context (`eng_get_ob(re)->egl.context[0]`).
 *                  Example: (EGLContext)my_shared_context
 * @param version The desired OpenGL ES version. Currently, only EVAS_GL_GLES_2_X is supported.
 *                Example: EVAS_GL_GLES_2_X
 * @return A void pointer to the created EGLContext, or NULL if data is invalid,
 *         an unsupported version is requested, or `eglCreateContext` fails.
 */
static void *
evgl_eng_context_create(void *data, void *share_ctx, Evas_GL_Context_Version version)
{
   Render_Engine *re;
   EGLContext context = EGL_NO_CONTEXT;
   int context_attrs[3];

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   if (version != EVAS_GL_GLES_2_X)
     {
        ERR("This engine only supports OpenGL-ES 2.0 contexts for now!");
        return NULL;
     }

   context_attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   context_attrs[1] = 2;
   context_attrs[2] = EGL_NONE;

   // Share context already assumes that it's sharing with evas' context
   if (share_ctx)
     {
        context = eglCreateContext(eng_get_ob(re)->egl.disp,
                                   eng_get_ob(re)->egl.config,
                                   (EGLContext)share_ctx,
                                   context_attrs);
     }
   else
     {
        context = eglCreateContext(eng_get_ob(re)->egl.disp,
                                   eng_get_ob(re)->egl.config,
                                   eng_get_ob(re)->egl.context[0], // Evas' GL Context
                                   context_attrs);
     }

   if (!context)
     {
        ERR("eglMakeCurrent() failed! Error Code=%#x", eglGetError());
        return NULL;
     }

   return (void *)context;
}

/**
 * @brief Destroys an EGL context.
 * Implements the EVGL_Interface::context_destroy function.
 *
 * Uses `eglDestroyContext` to destroy the EGL context.
 *
 * @param data Pointer to the Render_Engine structure.
 * @param context The EGLContext to destroy.
 *                Example: (EGLContext)my_egl_context
 * @return 1 on success (EGL_TRUE from eglDestroyContext), 0 if data or context is invalid,
 *         or if `eglDestroyContext` fails.
 */
static int
evgl_eng_context_destroy(void *data, void *context)
{
   Render_Engine *re;
   EGLBoolean ret = EGL_FALSE;

   re = (Render_Engine *)data;
   if ((!re) || (!context))
     {
        ERR("Invalid Render Input Data. Engine: %p, Context: %p",
            data, context);
        return 0;
     }

   ret = eglDestroyContext(eng_get_ob(re)->egl.disp, (EGLContext)context);
   if (ret == EGL_TRUE) return 1;

   return 0;
}

/**
 * @brief Retrieves the EGL extensions string for the current display.
 * Implements the EVGL_Interface::string_get function.
 *
 * Uses `eglQueryString` with `EGL_EXTENSIONS`.
 *
 * @param data Pointer to the Render_Engine structure.
 * @return A const char pointer to the EGL extensions string, or NULL if
 *         data is invalid or the EGL display is not available.
 */
static const char *
evgl_eng_string_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   return eglQueryString(eng_get_ob(re)->egl.disp, EGL_EXTENSIONS);
}

/**
 * @brief Retrieves the address of an EGL or GLES extension function.
 * Implements the EVGL_Interface::proc_address_get function.
 *
 * It first tries to use the loaded `eglGetProcAddress` symbol (`glsym_eglGetProcAddress`).
 * If that's not available or fails, it falls back to `dlsym(RTLD_DEFAULT, name)`.
 *
 * @param name The name of the function to retrieve.
 *             Example: "glEGLImageTargetTexture2DOES"
 * @return A void pointer to the function, or NULL if not found by either method.
 */
static void *
evgl_eng_proc_address_get(const char *name)
{
   if (glsym_eglGetProcAddress) return glsym_eglGetProcAddress(name);
   return dlsym(RTLD_DEFAULT, name);
}

/**
 * @brief Retrieves the current rotation angle of the Evas canvas.
 * Implements the EVGL_Interface::rotation_angle_get function.
 *
 * @param data Pointer to the Render_Engine structure.
 * @return The rotation angle in degrees (e.g., 0, 90, 180, 270) as stored in
 *         the output buffer's GL context, or 0 if data is invalid or the
 *         output buffer/GL context is not available.
 */
static int
evgl_eng_rotation_angle_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return 0;
     }

   if ((eng_get_ob(re)) && (eng_get_ob(re)->gl_context))
     return eng_get_ob(re)->gl_context->rot;
   else
     {
        ERR("Unable to retrieve rotation angle.");
        return 0;
     }
}

/**
 * @brief Makes the EGL context current for preloading operations.
 *
 * This function is used by the Evas GL preloading mechanism.
 * If `doit` is true, it makes the output buffer's EGL context and surface current.
 * If `doit` is false, it makes no context/surface current.
 *
 * @param data Pointer to an Outbuf structure.
 * @param doit A void pointer interpreted as a boolean. If non-NULL (true),
 *             makes the context current. If NULL (false), releases the current context.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid Outbuf or eglMakeCurrent fails).
 */
static Eina_Bool
eng_preload_make_current(void *data, void *doit)
{
   Outbuf *ob;

   ob = (Outbuf *)data;
   if (!ob) return EINA_FALSE;

   if (doit)
     {
        if (!eglMakeCurrent(ob->egl.disp, ob->egl.surface[0],
                            ob->egl.surface[0], ob->egl.context[0]))
          return EINA_FALSE;
     }
   else
     {
        if (!eglMakeCurrent(ob->egl.disp, EGL_NO_SURFACE,
                            EGL_NO_SURFACE, EGL_NO_CONTEXT))
          return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Releases resources associated with the render engine's window/surface.
 *
 * This function is typically called when the window or surface is no longer needed
 * or needs to be recreated. It relaxes the GL preload rendering lock and
 * un-surfaces the output buffer.
 *
 * @param re Pointer to the Render_Engine structure.
 */
static void
_re_winfree(Render_Engine *re)
{
   if (!re) return;
   if (!eng_get_ob(re)->surf) return;
   glsym_evas_gl_preload_render_relax(eng_preload_make_current, eng_get_ob(re));
   evas_outbuf_unsurf(eng_get_ob(re));
}

/**
 * @brief Callback function to bind a native surface's texture.
 *
 * This function is set as the `bind` callback for Evas_GL_Image native surfaces.
 * It binds the appropriate texture based on the native surface type:
 * - For EVAS_NATIVE_SURFACE_WL: Calls `glEGLImageTargetTexture2DOES` if the EGL image surface exists.
 * - For EVAS_NATIVE_SURFACE_OPENGL: Calls `glBindTexture` with the texture ID.
 *
 * @param image Pointer to the Evas_GL_Image whose native texture is to be bound.
 */
static void
_native_cb_bind(void *image)
{
   Evas_GL_Image *img;
   Native *n;

   if (!(img = image)) return;
   if (!(n = img->native.data)) return;

   if (n->ns.type == EVAS_NATIVE_SURFACE_WL)
     {
        if (n->egl_surface)
          {
             if (glsym_glEGLImageTargetTexture2DOES)
               {
                  glsym_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, n->egl_surface);
                  if (eglGetError() != EGL_SUCCESS)
                    ERR("glEGLImageTargetTexture2DOES() failed.");
               }
             else
               ERR("Try glEGLImageTargetTexture2DOES on EGL with no support");
          }
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
     glBindTexture(GL_TEXTURE_2D, n->ns.data.opengl.texture_id);

   /* TODO: NATIVE_SURFACE_TBM and NATIVE_SURFACE_EVASGL */
}

/**
 * @brief Callback function to unbind a native surface's texture.
 *
 * This function is set as the `unbind` callback for Evas_GL_Image native surfaces.
 * It unbinds the texture:
 * - For EVAS_NATIVE_SURFACE_OPENGL: Calls `glBindTexture(GL_TEXTURE_2D, 0)`.
 * - Other types currently do not have specific unbind actions in this function.
 *
 * @param image Pointer to the Evas_GL_Image whose native texture is to be unbound.
 */
static void
_native_cb_unbind(void *image)
{
   Evas_GL_Image *img;
   Native *n;

   if (!(img = image)) return;
   if (!(n = img->native.data)) return;

   else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
     glBindTexture(GL_TEXTURE_2D, 0);

   /* TODO: NATIVE_SURFACE_TBM and NATIVE_SURFACE_EVASGL */
}

/**
 * @brief Callback function to free resources associated with a native surface.
 *
 * This function is set as the `free` callback for Evas_GL_Image native surfaces.
 * It performs cleanup based on the native surface type:
 * - For EVAS_NATIVE_SURFACE_WL: Removes the image from the Wayland native hash,
 *   and destroys the EGL image surface using `eglDestroyImage` if it exists.
 * - For EVAS_NATIVE_SURFACE_OPENGL: Removes the image from the native texture hash.
 *
 * It also clears the native data and function pointers in the Evas_GL_Image
 * and frees the associated Native structure.
 *
 * @param image Pointer to the Evas_GL_Image whose native resources are to be freed.
 */
static void
_native_cb_free(void *image)
{
   Evas_GL_Image *img;
   Native *n;
   uint32_t texid;
   void *wlid;

   if (!(img = image)) return;
   if (!(n = img->native.data)) return;
   if (!img->native.shared) return;

   if (n->ns.type == EVAS_NATIVE_SURFACE_WL)
     {
        wlid = n->wl_buf;
        eina_hash_del(img->native.shared->native_wl_hash, &wlid, img);
        if (n->egl_surface)
          {
             if (glsym_eglDestroyImage)
               {
                  glsym_eglDestroyImage(img->native.disp, n->egl_surface);
                  if (eglGetError() != EGL_SUCCESS)
                    ERR("eglDestroyImage() failed.");
               }
             else
               ERR("Try eglDestroyImage on EGL with  no support");
          }
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
     {
        texid = n->ns.data.opengl.texture_id;
        eina_hash_del(img->native.shared->native_tex_hash, &texid, img);
     }

   img->native.data = NULL;
   img->native.func.bind = NULL;
   img->native.func.unbind = NULL;
   img->native.func.free = NULL;

   free(n);
}

/* engine specific override functions */
/**
 * @brief Sets up engine-specific information.
 *
 * This function is an override for the Evas engine's `output_info_setup`
 * method. It configures the render mode for the EGLFS engine.
 * Currently, it sets the render mode to EVAS_RENDER_MODE_BLOCKING.
 *
 * @param info Pointer to an Evas_Engine_Info_Eglfs structure that will be
 *             populated with engine-specific settings.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Eglfs *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Sets up the rendering output for the EGLFS engine.
 *
 * This function is an override for the Evas engine's `output_setup` method.
 * It initializes the rendering environment, including:
 * - Initializing Evas GL preloading if not already done.
 * - Allocating a Render_Engine structure.
 * - Creating an Outbuf (output buffer) for rendering.
 * - Initializing the generic GL rendering engine components.
 * - Vetoing EGL extensions if this is the first window.
 *
 * @param engine The Evas engine pointer (unused in this function but part of the API).
 * @param in Pointer to an Evas_Engine_Info_Eglfs structure containing setup information.
 *           Example:
 *           Evas_Engine_Info_Eglfs my_info;
 *           // ... populate my_info ...
 *           eng_output_setup(evas_object_evas_get(canvas), &my_info, width, height);
 * @param w The width of the output surface.
 * @param h The height of the output surface.
 * @return A pointer to the initialized Render_Engine structure on success, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Eglfs *info = in;
   Render_Engine *re = NULL;
   Outbuf *ob = NULL;
   Render_Output_Swap_Mode swap_mode;

   swap_mode = evas_render_engine_gl_swap_mode_get(info->info.swap_mode);

   if (!initted)
     {
        glsym_evas_gl_preload_init();
     }

   if (!(re = calloc(1, sizeof(Render_Engine)))) return NULL;

   /* try to create new outbuf */
   ob = evas_outbuf_new(info, w, h, swap_mode);
   if (!ob) goto on_error;

   ob->evas = evas;

   if (!evas_render_engine_gl_generic_init(engine, &re->generic, ob,
                                           evas_outbuf_buffer_state_get,
                                           evas_outbuf_rot_get,
                                           evas_outbuf_reconfigure,
                                           evas_outbuf_update_region_first_rect,
                                           NULL,
                                           evas_outbuf_update_region_new,
                                           evas_outbuf_update_region_push,
                                           NULL,
                                           NULL,
                                           evas_outbuf_flush,
                                           NULL,
                                           evas_outbuf_free,
                                           evas_outbuf_use,
                                           evas_outbuf_gl_context_get,
                                           evas_outbuf_egl_display_get,
                                           evas_outbuf_gl_context_new,
                                           evas_outbuf_gl_context_use,
                                           &evgl_funcs, ob->w, ob->h))
     goto on_error;

   gl_wins++;

   evas_render_engine_software_generic_merge_mode_set(&re->generic.software);

   evas_outbuf_use(eng_get_ob(re));

   if (!initted)
     {
        gl_extn_veto(re);
        initted = EINA_TRUE;
     }

   return re;

 on_error:
   /* free outbuf */
   evas_outbuf_free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Updates the rendering output configuration.
 *
 * This function is an override for the Evas engine's `output_update` method.
 * It handles changes in output parameters like depth, alpha, size, or rotation.
 * If significant properties like depth or destination alpha change, it may
 * recreate the output buffer. If only size or rotation changes, it reconfigures
 * the existing output buffer.
 *
 * @param engine The Evas engine pointer (unused).
 * @param data Pointer to the Render_Engine structure.
 * @param info Pointer to an Evas_Engine_Info_Eglfs structure containing the new configuration.
 *             The `info->info` sub-structure contains fields like `depth`,
 *             `destination_alpha`, `rotation`.
 * @param w The new width of the output surface.
 * @param h The new height of the output surface.
 * @return 1 on success, 0 on failure (e.g., if recreating the output buffer fails).
 */
static int
eng_output_update(void *engine EINA_UNUSED, void *data, void *info, unsigned int w, unsigned int h)
{
   Render_Engine *re = data;

   if (eng_get_ob(re) && _re_wincheck(eng_get_ob(re)))
     {
        if ((info->info.depth != eng_get_ob(re)->depth) ||
            (info->info.destination_alpha != eng_get_ob(re)->destination_alpha))
          {
             Outbuf *ob, *ob_old;

             ob_old = re->generic.software.ob;
             re->generic.software.ob = NULL;
             gl_wins--;

             ob = evas_outbuf_new(info, w, h, swap_mode);
             if (!ob)
               {
                  if (ob_old) evas_outbuf_free(ob_old);
                  return 0;
               }

             evas_outbuf_use(ob);
             if (ob_old) evas_outbuf_free(ob_old);

             ob->evas = evas;

             evas_render_engine_software_generic_update(&re->generic.software, ob,
                                                        w, h);

             gl_wins++;
          }
        else if ((eng_get_ob(re)->w != w) ||
                 (eng_get_ob(re)->h != h) ||
                 (info->info.rotation != eng_get_ob(re)->rotation))
          {
             evas_outbuf_reconfigure(eng_get_ob(re),
                                     w, h,
                                     info->info.rotation,
                                     info->info.depth);
             evas_render_engine_software_generic_update(&re->generic.software,
                                                        re->generic.software.ob,
                                                        w, h);
          }
     }

   evas_outbuf_use(eng_get_ob(re));

   return 1;
}

/**
 * @brief Frees resources associated with the rendering output.
 *
 * This function is an override for the Evas engine's `output_free` method.
 * It cleans up resources used by the Render_Engine, including:
 * - Relaxing the GL preload render lock.
 * - Shutting down the EVGL engine if this is the last window.
 * - Cleaning the generic software rendering components (which also frees the Outbuf).
 * - Freeing the Render_Engine structure itself.
 * - Shutting down Evas GL preloading if this was the last window and preloading was initialized.
 *
 * @param engine The Evas engine pointer, used for cleaning generic software components.
 * @param data Pointer to the Render_Engine structure to be freed.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (re)
     {
        glsym_evas_gl_preload_render_relax(eng_preload_make_current, eng_get_ob(re));

        if (gl_wins == 1) glsym_evgl_engine_shutdown(re);

        /* NB: evas_render_engine_software_generic_clean() frees ob */
        evas_render_engine_software_generic_clean(engine, &re->generic.software);

        gl_wins--;

        free(re);
     }

   if ((initted == EINA_TRUE) && (gl_wins == 0))
     {
        glsym_evas_gl_preload_shutdown();
        initted = EINA_FALSE;
     }
}

/**
 * @brief Gets the alpha channel state of the canvas.
 *
 * This function is an override for the Evas engine's `canvas_alpha_get` method.
 * It returns whether the destination surface has an alpha channel.
 *
 * @param data Pointer to the Render_Engine structure.
 * @return EINA_TRUE if the destination surface has an alpha channel, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if `data` is NULL.
 */
static Eina_Bool
eng_canvas_alpha_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re) return EINA_FALSE;

   return eng_get_ob(re)->destination_alpha;
}

/**
 * @brief Dumps rendering resources, typically to free up memory.
 *
 * This function is an override for the Evas engine's `output_dump` method.
 * It unloads all common image and font data, unloads GL images specific
 * to the engine's GL context, and frees window-specific resources.
 * This is often called in low-memory situations.
 *
 * @param engine The Evas engine pointer (unused).
 * @param data Pointer to the Render_Engine structure.
 */
static void
eng_output_dump(void *engine EINA_UNUSED, void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (!re) return;

   evas_common_image_image_all_unload();
   evas_common_font_font_all_unload();
   glsym_evas_gl_common_image_all_unload(eng_get_ob(re)->gl_context);
   _re_winfree(re);
}

/**
 * @brief Sets or updates a native surface for an Evas image.
 *
 * This function is an override for the Evas engine's `image_native_set` method.
 * It allows Evas to use externally managed graphics resources (native surfaces)
 * as image sources.
 *
 * The function handles different types of native surfaces:
 * - EVAS_NATIVE_SURFACE_WL (Wayland buffer): Creates an EGLImage from the Wayland
 *   buffer and associates it with the Evas_GL_Image. It uses a hash to reuse
 *   existing Evas_GL_Images for the same Wayland buffer.
 * - EVAS_NATIVE_SURFACE_OPENGL (OpenGL texture): Associates an existing OpenGL
 *   texture with the Evas_GL_Image. It uses a hash to reuse existing
 *   Evas_GL_Images for the same texture ID.
 *
 * If `image` is NULL and `native` specifies an OpenGL surface, a new Evas_GL_Image
 * is created.
 * If `native` is NULL, any existing native surface association for `image` is cleared.
 *
 * @param engine The Evas engine pointer (unused).
 * @param data Pointer to the Render_Engine structure.
 * @param image Pointer to an Evas_GL_Image. If NULL and `native` is an OpenGL
 *              surface, a new image might be created.
 *              Example: (Evas_GL_Image *)my_evas_gl_image
 * @param native Pointer to an Evas_Native_Surface structure describing the native resource.
 *               Example:
 *               Evas_Native_Surface ns;
 *               ns.type = EVAS_NATIVE_SURFACE_WL;
 *               ns.version = EVAS_NATIVE_SURFACE_VERSION;
 *               ns.data.wl.legacy_buffer = my_wl_buffer;
 *               // or for OpenGL:
 *               // ns.type = EVAS_NATIVE_SURFACE_OPENGL;
 *               // ns.data.opengl.texture_id = my_texture_id;
 *               // ns.data.opengl.framebuffer_id = my_fbo_id;
 *               // ns.data.opengl.w = tex_width;
 *               // ns.data.opengl.h = tex_height;
 *               eng_image_native_set(engine, re, evas_image, &ns);
 *               If NULL, clears the native surface from the image.
 * @return Pointer to the Evas_GL_Image (possibly a new or reused one) associated
 *         with the native surface, or NULL on failure or if `native` is NULL.
 */
static void *
eng_image_native_set(void *engine EINA_UNUSED, void *data, void *image, void *native)
{
   Render_Engine *re;
   Outbuf *ob;
   Native *n;
   Evas_Native_Surface *ns;
   Evas_GL_Image *img, *img2;
   unsigned int tex = 0, fbo = 0;
   uint32_t texid;
   void *wlid, *wl_buf = NULL;

   re = (Render_Engine *)data;
   if (!re) return NULL;

   ob = eng_get_ob(re);
   if (!ob) return NULL;

   ns = native;

   if (!(img = image))
     {
        if ((ns) && (ns->type == EVAS_NATIVE_SURFACE_OPENGL))
          {
             img =
               glsym_evas_gl_common_image_new_from_data(ob->gl_context,
                                                        ns->data.opengl.w,
                                                        ns->data.opengl.h,
                                                        NULL, 1,
                                                        EVAS_COLORSPACE_ARGB8888);
          }
        else
          return NULL;
     }

   if (ns)
     {
        if (ns->type == EVAS_NATIVE_SURFACE_WL)
          {
             wl_buf = ns->data.wl.legacy_buffer;
             if (img->native.data)
               {
                  Evas_Native_Surface *ens;

                  ens = img->native.data;
                  if (ens->data.wl.legacy_buffer == wl_buf)
                    return img;
               }
          }
        else if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
          {
             tex = ns->data.opengl.texture_id;
             fbo = ns->data.opengl.framebuffer_id;
             if (img->native.data)
               {
                  Evas_Native_Surface *ens;

                  ens = img->native.data;
                  if ((ens->data.opengl.texture_id == tex) &&
                      (ens->data.opengl.framebuffer_id == fbo))
                     return img;
               }
          }
     }

   evas_outbuf_use(ob);

   if (!ns)
     {
        glsym_evas_gl_common_image_free(img);
        return NULL;
     }

   if (ns->type == EVAS_NATIVE_SURFACE_WL)
     {
        wlid = wl_buf;
        img2 = eina_hash_find(ob->gl_context->shared->native_wl_hash, &wlid);
        if (img2 == img) return img;
        if (img2)
          {
             if((n = img2->native.data))
               {
                  glsym_evas_gl_common_image_ref(img2);
                  glsym_evas_gl_common_image_free(img);
                  return img2;
               }
          }
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
     {
        texid = tex;
        img2 = eina_hash_find(ob->gl_context->shared->native_tex_hash, &texid);
        if (img2 == img) return img;
        if (img2)
          {
             if ((n = img2->native.data))
               {
                  glsym_evas_gl_common_image_ref(img2);
                  glsym_evas_gl_common_image_free(img);
                  return img2;
               }
          }
     }

   img2 = glsym_evas_gl_common_image_new_from_data(ob->gl_context, img->w,
                                                   img->h, NULL, img->alpha,
                                                   EVAS_COLORSPACE_ARGB8888);
   glsym_evas_gl_common_image_free(img);

   if (!(img = img2)) return NULL;

   if (ns->type == EVAS_NATIVE_SURFACE_WL)
     {
        if (native)
          {
             if ((n = calloc(1, sizeof(Native))))
               {
                  EGLint attribs[3];
                  int format, yinvert = 1;

                  glsym_eglQueryWaylandBufferWL(ob->egl.disp, wl_buf,
                                                EGL_TEXTURE_FORMAT, &format);
                  if ((format != EGL_TEXTURE_RGB) &&
                      (format != EGL_TEXTURE_RGBA))
                    {
                       ERR("eglQueryWaylandBufferWL() %d format is not supported ", format);
                       glsym_evas_gl_common_image_free(img);
                       free(n);
                       return NULL;
                    }

#ifndef EGL_WAYLAND_PLANE_WL
# define EGL_WAYLAND_PLANE_WL 0x31D6
#endif
#ifndef EGL_WAYLAND_BUFFER_WL
# define EGL_WAYLAND_BUFFER_WL 0x31D5
#endif
                  attribs[0] = EGL_WAYLAND_PLANE_WL;
                  attribs[1] = 0; //if plane is 1 then 0, if plane is 2 then 1
                  attribs[2] = EGL_NONE;

                  memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
                  glsym_eglQueryWaylandBufferWL(ob->egl.disp, wl_buf,
                                                EGL_WAYLAND_Y_INVERTED_WL,
                                                &yinvert);
                  eina_hash_add(ob->gl_context->shared->native_wl_hash,
                                &wlid, img);

                  n->wl_buf = wl_buf;
                  if (glsym_eglCreateImage)
                    n->egl_surface = glsym_eglCreateImage(ob->egl.disp,
                                                          NULL,
                                                          EGL_WAYLAND_BUFFER_WL,
                                                          wl_buf, attribs);
                  else
                    {
                       ERR("Try eglCreateImage on EGL with no support");
                       eina_hash_del(ob->gl_context->shared->native_wl_hash,
                                     &wlid, img);
                       glsym_evas_gl_common_image_free(img);
                       free(n);
                       return NULL;
                    }

                  if (!n->egl_surface)
                    {
                       ERR("eglCreatePixmapSurface() for %p failed", wl_buf);
                       eina_hash_del(ob->gl_context->shared->native_wl_hash,
                                     &wlid, img);
                       glsym_evas_gl_common_image_free(img);
                       free(n);
                       return NULL;
                    }

                  //XXX: workaround for mesa-10.2.8
                  // mesa's eglQueryWaylandBufferWL() with EGL_WAYLAND_Y_INVERTED_WL works incorrect.
                  //img->native.yinvert = yinvert;
                  img->native.yinvert = 1;
                  img->native.loose = 0;
                  img->native.data = n;
                  img->native.func.bind = _native_cb_bind;
                  img->native.func.unbind = _native_cb_unbind;
                  img->native.func.free = _native_cb_free;
                  img->native.target = GL_TEXTURE_2D;
                  img->native.mipmap = 0;

                  glsym_evas_gl_common_image_native_enable(img);
               }
          }
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
     {
        if (native)
          {
             if ((n = calloc(1, sizeof(Native))))
               {
                  memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
                  eina_hash_add(ob->gl_context->shared->native_tex_hash,
                                &texid, img);

                  n->egl_surface = 0;

                  img->native.yinvert = 0;
                  img->native.loose = 0;
                  img->native.data = n;
                  img->native.func.bind = _native_cb_bind;
                  img->native.func.unbind = _native_cb_unbind;
                  img->native.func.free = _native_cb_free;
                  img->native.target = GL_TEXTURE_2D;
                  img->native.mipmap = 0;

                  glsym_evas_gl_common_image_native_enable(img);
               }
          }
     }

   /* TODO: NATIVE_SURFACE_TBM and NATIVE_SURFACE_EVASGL */

   return img;
}

/* module api functions */
/**
 * @brief Opens and initializes the EGLFS engine module.
 *
 * This function is the entry point for the Evas module system to load the EGLFS engine.
 * It performs the following steps:
 * 1. Inherits functions from the "gl_generic" engine module.
 * 2. Registers an Eina log domain for the eglfs engine.
 * 3. Overrides specific engine functions with EGLFS implementations (e.g., output_setup, output_free).
 * 4. Sets the EGL_PLATFORM environment variable to "fbdev".
 * 5. Loads EGL/GLES symbols using `gl_symbols()`.
 * 6. Advertises the engine's API functions to the Evas module system.
 *
 * @param em Pointer to the Evas_Module structure for this engine.
 * @return 1 on successful initialization, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   /* check for valid evas module */
   if (!em) return 0;

   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "gl_generic", sizeof (Evas_Engine_Info_Eglfs))) return 0;

   /* try to create eina logging domain */
   if (_evas_engine_eglfs_log_dom < 0)
     {
        _evas_engine_eglfs_log_dom =
          eina_log_domain_register("evas-eglfs", EVAS_DEFAULT_LOG_COLOR);
     }

   /* if we could not create a logging domain, error out */
   if (_evas_engine_eglfs_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   /* store it for later use */
   func = pfunc;

   /* now to override methods */
#define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(output_info);
   ORD(output_setup);
   ORD(output_update);
   ORD(canvas_alpha_get);
   ORD(output_free);
   ORD(output_dump);
   ORD(image_native_set);

   setenv("EGL_PLATFORM", "fbdev", 1);

   gl_symbols();

   /* now advertise out own api */
   em->functions = (void *)(&func);

   return 1;
}

/**
 * @brief Closes and deinitializes the EGLFS engine module.
 *
 * This function is called by the Evas module system when the engine is unloaded.
 * It unregisters the Eina log domain previously registered by `module_open()`.
 *
 * @param em Pointer to the Evas_Module structure for this engine (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   /* unregister the eina log domain for this engine */
   if (_evas_engine_eglfs_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_eglfs_log_dom);
        _evas_engine_eglfs_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, "eglfs", "none", { module_open, module_close }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, eglfs);

#ifndef EVAS_STATIC_BUILD_EGLFS
EVAS_EINA_MODULE_DEFINE(engine, eglfs);
#endif
