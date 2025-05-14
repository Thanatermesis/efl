#include <windows.h>

#include <evil_private.h> /* dlsym */

#include "evas_common_private.h" /* Also includes international specific stuff */
#include "evas_engine.h"
#include "../gl_common/evas_gl_define.h"
#include "../software_generic/evas_native_common.h"

/*
 * FIXME
 * - native surface in Windows: EVAS_NATIVE_SURFACE_GDI or EVAS_NATIVE_SURFACE_WIN32 ?
 */

/** @brief Main engine structure holding generic GL data and Win32 specific info. */
typedef struct _Render_Engine Render_Engine;

struct _Render_Engine
{
   Render_Output_GL_Generic generic; /**< Inherited generic GL engine data */
   HINSTANCE instance;               /**< Win32 application instance handle */
   char *class_name;                 /**< Dynamically generated window class name */
};

/** @brief Generic function pointer type returning void*. */
typedef void           *(*glsym_func_void_ptr) (void);

/** @brief Generic function pointer type taking void* and returning void. */
typedef void            (*glsym_func_void_in_voidp) (void *);
/** @brief Generic function pointer type taking int and returning void. */
typedef void            (*glsym_func_void_in_int) (int);
/** @brief Generic function pointer type taking void* and returning int. */
typedef int             (*glsym_func_int_in_voidp) (void *);

const char *debug_dir;             /**< Directory for swap buffer debug dumps */
int swap_buffer_debug_mode = -1;   /**< Swap buffer debug mode status (-1: unset, 0: disabled, 1: enabled) */
int swap_buffer_debug = 0;         /**< Flag to enable swap buffer debugging (dumping frames) */
int partial_render_debug = -1;     /**< Flag to enable partial rendering debug messages (-1: unset, 0: disabled, 1: enabled) */
int extn_have_buffer_age = 1;      /**< Flag indicating if EGL_EXT_buffer_age or equivalent is supported */

static int initted = 0;            /**< Flag indicating if the GL engine module has been initialized */
static int gl_wins = 0;            /**< Counter for active GL windows/outputs */
static int extn_have_y_inverted = 1; /**< Flag indicating if EGL_NOK_texture_from_pixmap (Y-inversion) is supported */

/** @brief Function pointer for referencing an Evas GL common image. */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_ref = NULL;
/** @brief Function pointer for unreferencing an Evas GL common image. */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_unref = NULL;
/** @brief Function pointer for freeing an Evas GL common image. */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_free = NULL;
/** @brief Function pointer for disabling native surface usage for an image. */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_native_disable = NULL;
/** @brief Function pointer for enabling native surface usage for an image. */
Evas_GL_Common_Image_Call glsym_evas_gl_common_image_native_enable = NULL;
/** @brief Function pointer for creating a new Evas GL image from data. */
Evas_GL_Common_Image_New_From_Data glsym_evas_gl_common_image_new_from_data = NULL;
/** @brief Function pointer for unloading all Evas GL common images. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_image_all_unload = NULL;
/** @brief Function pointer for initializing the Evas GL preload mechanism. */
Evas_GL_Preload glsym_evas_gl_preload_init = NULL;
/** @brief Function pointer for shutting down the Evas GL preload mechanism. */
Evas_GL_Preload glsym_evas_gl_preload_shutdown = NULL;
/** @brief Function pointer for shutting down the EVGL engine interface. */
EVGL_Engine_Call glsym_evgl_engine_shutdown = NULL;
/** @brief Function pointer for getting the native buffer from an EVGL surface. */
EVGL_Native_Surface_Call glsym_evgl_native_surface_buffer_get = NULL;
/** @brief Function pointer for getting the Y-inversion status of an EVGL surface. */
EVGL_Native_Surface_Yinvert_Call glsym_evgl_native_surface_yinvert_get = NULL;
/** @brief Function pointer for getting the current native context associated with an EVGL context. */
EVGL_Current_Native_Context_Get_Call glsym_evgl_current_native_context_get = NULL;
/** @brief Function pointer for resolving GL symbols via Evas GL common. */
Evas_Gl_Symbols glsym_evas_gl_symbols = NULL;

/** @brief Function pointer for creating a new Evas GL common context. */
Evas_GL_Common_Context_New glsym_evas_gl_common_context_new = NULL;
/** @brief Function pointer for flushing the Evas GL common context. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush = NULL;
/** @brief Function pointer for freeing the Evas GL common context. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free = NULL;
/** @brief Function pointer for making the Evas GL common context current. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use = NULL;
/** @brief Function pointer for starting a new frame in the Evas GL common context. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe = NULL;
/** @brief Function pointer for finishing operations in the Evas GL common context. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done = NULL;
/** @brief Function pointer for resizing the Evas GL common context. */
Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize = NULL;
/** @brief Function pointer for dumping the Evas GL common context buffer. */
Evas_GL_Common_Buffer_Dump_Call glsym_evas_gl_common_buffer_dump = NULL;
/** @brief Function pointer for locking the preload renderer. */
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock = NULL;
/** @brief Function pointer for unlocking the preload renderer. */
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock = NULL;
/** @brief Function pointer for relaxing the preload renderer lock. */
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_relax = NULL;

/** @brief Function pointer for flushing Evas GL common shaders. */
glsym_func_void_in_voidp glsym_evas_gl_common_shaders_flush = NULL;
/** @brief Function pointer for setting the Evas GL common error code. */
glsym_func_void_in_int   glsym_evas_gl_common_error_set = NULL;
/** @brief Function pointer for getting the Evas GL common error code. */
glsym_func_int_in_voidp  glsym_evas_gl_common_error_get = NULL;
/** @brief Function pointer for getting the current Evas GL common context. */
glsym_func_void_ptr      glsym_evas_gl_common_current_context_get = NULL;

/** @brief Function pointer for eglGetProcAddress or equivalent. */
void        *(*glsym_eglGetProcAddress)            (const char *a) = NULL;
/** @brief Function pointer for Evas GL common's eglCreateImage wrapper. */
EGLImageKHR  (*glsym_evas_gl_common_eglCreateImage)(EGLDisplay a, EGLContext b, EGLenum c, EGLClientBuffer d, const EGLAttrib *e) = NULL;
/** @brief Function pointer for Evas GL common's eglDestroyImage wrapper. */
int          (*glsym_evas_gl_common_eglDestroyImage) (EGLDisplay a, void *b) = NULL;
/** @brief Function pointer for glEGLImageTargetTexture2DOES. */
void         (*glsym_glEGLImageTargetTexture2DOES) (int a, void *b)  = NULL;
/** @brief Function pointer for eglSwapBuffersWithDamageEXT/KHR. */
unsigned int (*glsym_eglSwapBuffersWithDamage) (EGLDisplay a, void *b, const EGLint *d, EGLint c) = NULL;
/** @brief Function pointer for eglSetDamageRegionKHR. */
unsigned int (*glsym_eglSetDamageRegionKHR)  (EGLDisplay a, EGLSurface b, EGLint *c, EGLint d) = NULL;
/** @brief Function pointer for eglQueryWaylandBufferWL (unused in Win32). */
unsigned int (*glsym_eglQueryWaylandBufferWL)(EGLDisplay a, /*struct wl_resource */void *b, EGLint c, EGLint *d) = NULL;

static inline Outbuf *
eng_get_ob(Render_Engine *re)
{
   return re->generic.software.ob;
}

/**
 * @brief Window procedure for the hidden helper window used by EVGL.
 *
 * This handles basic window messages, primarily WM_CLOSE. It doesn't
 * handle painting as the window is typically hidden and small.
 *
 * @param window The window handle.
 * @param message The window message identifier.
 * @param window_param Additional message information.
 * @param data_param Additional message information.
 * @return The result of the message processing.
 */
static LRESULT CALLBACK
_procedure(HWND   window,
           UINT   message,
           WPARAM window_param,
           LPARAM data_param)
{
   switch(message)
     {
        case WM_CLOSE:
          DestroyWindow(window);
          return 0;
        /* case WM_PAINT: */
        /* { */
        /*     RECT r; */

        /*     if (GetUpdateRect(window, &r, FALSE)) */
        /*     { */
        /*         PAINTSTRUCT ps; */

        /*         if (BeginPaint(window, &ps)) */
        /*         { */
        /*             LONG_PTR ptr = GetWindowLongPtr(window, GWLP_USERDATA); */
        /*             win *w = (win *)ptr; */

        /*             w->data_paste(); */

        /*             EndPaint(window, &ps); */
        /*         } */
        /*     } */
        /*     return 0; */
        /* } */
        default:
          return DefWindowProc(window, message, window_param, data_param);
    }
}

/************* Evas GL Engine Funtions Overloading *************/

static EGLDisplay main_dpy  = EGL_NO_DISPLAY;
static EGLSurface main_draw = EGL_NO_SURFACE;
static EGLSurface main_read = EGL_NO_SURFACE;
static EGLContext main_ctx  = EGL_NO_CONTEXT; /**< Cached EGL context for main thread */

/**
 * @brief Wrapper for eglGetCurrentContext that handles main loop integration.
 *
 * If called from the main thread (where Eina's main loop runs), it returns
 * a cached context value to avoid potentially expensive EGL calls if the
 * context hasn't changed. Otherwise, it calls the real eglGetCurrentContext.
 *
 * @return The current EGL context.
 */
EGLContext
evas_eglGetCurrentContext(void)
{
   if (eina_main_loop_is())
     return main_ctx;
   else
     return eglGetCurrentContext();
}

/**
 * @brief Wrapper for eglGetCurrentSurface that handles main loop integration.
 *
 * Similar to evas_eglGetCurrentContext, this returns cached surface values
 * (read or draw) if called from the main thread, otherwise calls the real
 * eglGetCurrentSurface.
 *
 * @param readdraw Specifies EGL_READ or EGL_DRAW.
 * @return The current EGL surface for the specified purpose.
 */
EGLSurface
evas_eglGetCurrentSurface(EGLint readdraw)
{
   if (eina_main_loop_is())
     return (readdraw == EGL_READ) ? main_read : main_draw;
   else
     return eglGetCurrentSurface(readdraw);
}

/**
 * @brief Wrapper for eglGetCurrentDisplay that handles main loop integration.
 *
 * Returns a cached display value if called from the main thread, otherwise
 * calls the real eglGetCurrentDisplay.
 *
 * @return The current EGL display.
 */
EGLDisplay
evas_eglGetCurrentDisplay(void)
{
   if (eina_main_loop_is())
     return main_dpy;
   else
     return eglGetCurrentDisplay();
}

/**
 * @brief Wrapper for eglMakeCurrent that handles main loop integration and state caching.
 *
 * If called from the main thread, it first checks if the requested state
 * (display, draw surface, read surface, context) matches the cached state.
 * If it matches, it returns immediately assuming success. Otherwise, it calls
 * the real eglMakeCurrent and updates the cache upon success.
 * If not called from the main thread, it directly calls eglMakeCurrent.
 *
 * @param dpy The EGL display.
 * @param draw The EGL draw surface.
 * @param read The EGL read surface.
 * @param ctx The EGL context.
 * @return EGL_TRUE on success, EGL_FALSE otherwise.
 */
EGLBoolean
evas_eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
   if (eina_main_loop_is())
     {
        EGLBoolean ret;

        if ((dpy == main_dpy) && (draw == main_draw) &&
            (read == main_read) && (ctx == main_ctx))
          return 1;

        ret = eglMakeCurrent(dpy, draw, read, ctx);
        if (ret)
          {
             main_dpy  = dpy;
             main_draw = draw;
             main_read = read;
             main_ctx  = ctx;
          }
        return ret;
     }
   else
     return eglMakeCurrent(dpy, draw, read, ctx);
}

/************* EVGL interface *************/
/**
 * @defgroup EVGL_Win32 Evas GL Engine Win32 EVGL Interface
 * @brief Functions implementing the EVGL interface for the Win32 GL engine.
 *
 * These functions allow external code (like the Evas GL library) to interact
 * with the Win32 EGL context, create surfaces, and manage resources.
 * @{
 */

/**
 * @brief Gets the EGL display handle used by the engine.
 * @param data The Render_Engine instance.
 * @return The EGLDisplay handle or NULL on error.
 */
static void *
evgl_eng_display_get(void *data)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   if (eng_get_ob(re))
      return (void*)eng_get_ob(re)->egl_disp;
   else
      return NULL;
}

/**
 * @brief Gets the main EGL surface handle used by Evas for rendering.
 * @param data The Render_Engine instance.
 * @return The EGLSurface handle or NULL on error.
 */
static void *
evgl_eng_evas_surface_get(void *data)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        return NULL;
     }

   if (eng_get_ob(re))
      return (void*)eng_get_ob(re)->egl_surface;
   else
      return NULL;
}

/**
 * @brief Creates a native Win32 window for use with EVGL (e.g., for offscreen rendering).
 *
 * Creates a small, hidden, popup window using the engine's instance and class.
 *
 * @param data The Render_Engine instance.
 * @return The HWND handle of the created window or NULL on error.
 */
static void *
evgl_eng_native_window_create(void *data)
{
   Render_Engine *re = (Render_Engine *)data;
   HWND win;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   win = CreateWindowEx(WS_EX_TOOLWINDOW,
                        re->class_name, NULL,
                        WS_VISIBLE | WS_POPUP,
                        -20, -20, 1, 1,
                        eng_get_ob(re)->window, NULL,
                        re->instance, NULL);
   if (!win)
     {
        ERR("Creating native X window failed.");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_DISPLAY);
        return NULL;
     }

   return (void*)win;
}

/**
 * @brief Destroys a native Win32 window previously created by evgl_eng_native_window_create.
 * @param data The Render_Engine instance.
 * @param native_window The HWND handle of the window to destroy.
 * @return 1 on success, 0 on failure.
 */
static int
evgl_eng_native_window_destroy(void *data, void *native_window)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, 0); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   if (!native_window)
     {
        ERR("Inavlid native surface.");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_NATIVE_WINDOW);
        return 0;
     }

   DestroyWindow((HWND)native_window);

   native_window = NULL;

   return 1;
}

/**
 * @brief Creates an EGL window surface for a given native Win32 window.
 *
 * This is needed to associate an EGL rendering surface with a native window.
 * Theoretically, this might not be needed if surfaceless contexts are fully supported
 * and used everywhere.
 *
 * @param data The Render_Engine instance.
 * @param native_window The HWND handle of the native window.
 * @return The EGLSurface handle or EGL_NO_SURFACE on error.
 */
static void *
evgl_eng_window_surface_create(void *data, void *native_window EINA_UNUSED)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   EGLSurface surface = EGL_NO_SURFACE;

   // Create resource surface for EGL
   surface = eglCreateWindowSurface(eng_get_ob(re)->egl_disp,
                                    eng_get_ob(re)->egl_config,
                                    (EGLNativeWindowType)native_window,
                                    NULL);
   if (!surface)
     {
        ERR("Creating window surface failed. Error: %#x.", eglGetError());
        abort();
        return NULL;
     }

   return (void*)surface;
}

/**
 * @brief Destroys an EGL window surface.
 * @param data The Render_Engine instance.
 * @param surface The EGLSurface handle to destroy.
 * @return 1 on success, 0 on failure.
 */
static int
evgl_eng_window_surface_destroy(void *data, void *surface)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, 0); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   if (!surface)
     {
        ERR("Invalid surface.");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_SURFACE);
        return 0;
     }

   eglDestroySurface(eng_get_ob(re)->egl_disp, (EGLSurface)surface);

   return 1;
   if (surface) return 0;
}

/**
 * @brief Creates a new EGL context, potentially sharing resources with another context.
 *
 * Allows creating GLES 1.x, 2.x, or 3.x contexts. If GLES 3 is supported by the
 * main Evas context and GLES 2 or 3 is requested, it attempts to create a GLES 3 context.
 *
 * @param data The Render_Engine instance.
 * @param share_ctx The EGLContext to share resources with (can be NULL or Evas' main context).
 * @param version The requested GLES version (EVAS_GL_GLES_1_X, EVAS_GL_GLES_2_X, EVAS_GL_GLES_3_X).
 * @return The EGLContext handle or EGL_NO_CONTEXT on error.
 */
static void *
evgl_eng_context_create(void *data, void *share_ctx, Evas_GL_Context_Version version)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   if ((version < EVAS_GL_GLES_1_X) || (version > EVAS_GL_GLES_3_X))
     {
        ERR("Invalid context version number %d", version);
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   if ((version == EVAS_GL_GLES_3_X) &&
       ((!eng_get_ob(re)->gl_context) || (eng_get_ob(re)->gl_context->gles_version != EVAS_GL_GLES_3_X)))
     {
        ERR("GLES 3 version not supported!");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_ATTRIBUTE);
        return NULL;
     }
   EGLContext context = EGL_NO_CONTEXT;
   int context_attrs[3];

   /* Upgrade GLES 2 to GLES 3.
    *
    * FIXME: Maybe we don't want to do this, unless we have no choice.
    * An alternative would be to use eglCreateImage() to share the indirect
    * rendering FBO between two contexts of incompatible version. For now,
    * we always upgrade the real context version to GLES 3 when it's available.
    * But this leads to some issues, namely that the list of extensions is
    * different, and MSAA surfaces also work differently.
    */
   if (gles3_supported && (version >= EVAS_GL_GLES_2_X))
     version = 3;

   context_attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   context_attrs[1] = version;
   context_attrs[2] = EGL_NONE;

   // Share context already assumes that it's sharing with evas' context
   if (share_ctx)
     {
        context = eglCreateContext(eng_get_ob(re)->egl_disp,
                                   eng_get_ob(re)->egl_config,
                                   (EGLContext)share_ctx,
                                   context_attrs);
     }
   else if ((version == EVAS_GL_GLES_1_X) || (version == EVAS_GL_GLES_3_X))
     {
        context = eglCreateContext(eng_get_ob(re)->egl_disp,
                                   eng_get_ob(re)->egl_config,
                                   NULL,
                                   context_attrs);
     }
   else
     {
        context = eglCreateContext(eng_get_ob(re)->egl_disp,
                                   eng_get_ob(re)->egl_config,
                                   eng_get_ob(re)->egl_context, // Evas' GL Context
                                   context_attrs);
     }

   if (!context)
     {
        int err = eglGetError();
        ERR("Engine Context Creations Failed. Error: %#x.", err);
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        return NULL;
     }

   return (void*)context;
}

/**
 * @brief Destroys an EGL context.
 * @param data The Render_Engine instance.
 * @param context The EGLContext handle to destroy.
 * @return 1 on success, 0 on failure.
 */
static int
evgl_eng_context_destroy(void *data, void *context)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, 0); */
   if ((!re) || (!context))
     {
        ERR("Invalid Render Input Data. Engine: %p, Context: %p", data, context);
        if (!re) glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        if (!context) glsym_evas_gl_common_error_set(EVAS_GL_BAD_CONTEXT);
        return 0;
     }

   eglDestroyContext(eng_get_ob(re)->egl_disp, (EGLContext)context);

   return 1;
}

/**
 * @brief Makes a given EGL context and surface current for the calling thread.
 *
 * Uses the evas_eglMakeCurrent wrapper to handle main loop integration and caching.
 * Optionally flushes the Evas rendering pipeline before making the new context current.
 * Passing NULL for both surface and context makes no context current.
 *
 * @param data The Render_Engine instance.
 * @param surface The EGLSurface handle for drawing (and reading).
 * @param context The EGLContext handle.
 * @param flush If non-zero, flush the Evas pipeline before making current.
 * @return 1 on success, 0 on failure.
 */
static int
evgl_eng_make_current(void *data, void *surface, void *context, int flush)
{
   Render_Engine *re = (Render_Engine *)data;
   int ret = 0;

   /* EVGLINIT(re, 0); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   EGLContext ctx = (EGLContext)context;
   EGLSurface sfc = (EGLSurface)surface;
   EGLDisplay dpy = eng_get_ob(re)->egl_disp; //eglGetCurrentDisplay();

   if ((!context) && (!surface))
     {
        ret = evas_eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (!ret)
          {
             int err = eglGetError();
             glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
             ERR("evas_eglMakeCurrent() failed! Error Code=%#x", err);
             return 0;
          }
        return 1;
     }

   // FIXME: Check (eglGetCurrentDisplay() != dpy) ?
   if ((evas_eglGetCurrentContext() != ctx) ||
       (evas_eglGetCurrentSurface(EGL_READ) != sfc) ||
       (evas_eglGetCurrentSurface(EGL_DRAW) != sfc) )
     {

        //!!!! Does it need to be flushed with it's set to NULL above??
        // Flush remainder of what's in Evas' pipeline
        if (flush) eng_window_use(NULL);

        // Do a make current
        ret = evas_eglMakeCurrent(dpy, sfc, sfc, ctx);

        if (!ret)
          {
             int err = eglGetError();
             glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
             ERR("evas_eglMakeCurrent() failed! Error Code=%#x", err);
             return 0;
          }
     }

   return 1;
}

/**
 * @brief Gets the address of an EGL or GL extension function.
 *
 * Uses eglGetProcAddress if available, otherwise falls back to dlsym.
 *
 * @param name The name of the function to retrieve.
 * @return A pointer to the function, or NULL if not found.
 */
static void *
evgl_eng_proc_address_get(const char *name)
{
   if (glsym_eglGetProcAddress) return glsym_eglGetProcAddress(name);
   return dlsym(RTLD_DEFAULT, name); // Note: dlsym might not be ideal on Windows
}

/**
 * @brief Gets the EGL extension string for the display.
 * @param data The Render_Engine instance.
 * @return The EGL extension string or NULL on error.
 */
static const char *
evgl_eng_string_get(void *data)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, NULL); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return NULL;
     }

   return eglQueryString(eng_get_ob(re)->egl_disp, EGL_EXTENSIONS);
}

/**
 * @brief Gets the current rotation angle of the Evas canvas.
 * @param data The Render_Engine instance.
 * @return The rotation angle (0, 90, 180, 270) or 0 on error.
 */
static int
evgl_eng_rotation_angle_get(void *data)
{
   Render_Engine *re = (Render_Engine *)data;

   /* EVGLINIT(re, 0); */
   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   if ((eng_get_ob(re)) && (eng_get_ob(re)->gl_context))
     return eng_get_ob(re)->gl_context->rot;
   else
     {
        ERR("Unable to retrieve rotation angle.");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_CONTEXT);
        return 0;
     }
}

/**
 * @brief Creates an EGL pbuffer surface.
 *
 * Pbuffers are offscreen rendering surfaces. This function attempts to create
 * one matching the requested dimensions and format specified in the EVGL_Surface.
 * Currently, extra attributes in `attrib_list` are ignored.
 * It tries to use an EGL config compatible with the main Evas context.
 *
 * @param data The Render_Engine instance (cast to Render_Output_GL_Generic).
 * @param sfc Pointer to the EVGL_Surface structure describing the desired pbuffer.
 *            Relevant fields: w, h, pbuffer.color_fmt, depth_fmt, stencil_fmt.
 * @param attrib_list Additional EGL attributes for surface creation (currently ignored).
 *                    Example: `{EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA, EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_NONE}`
 * @return The EGLSurface handle for the pbuffer or EGL_NO_SURFACE on error.
 */
static void *
evgl_eng_pbuffer_surface_create(void *data, EVGL_Surface *sfc,
                                const int *attrib_list)
{
   Render_Output_GL_Generic *re = data;

   // TODO: Add support for surfaceless pbuffers (EGL_NO_TEXTURE)
   // TODO: Add support for EGL_MIPMAP_TEXTURE??? (GLX doesn't support them)

   if (attrib_list)
     WRN("This PBuffer implementation does not support extra attributes yet");

   Evas_Engine_GL_Context *evasglctx;
   int config_attrs[20];
   int surface_attrs[20];
   EGLSurface egl_sfc;
   EGLConfig egl_cfg;
   int num_config, i = 0;
   EGLDisplay disp;
   EGLContext ctx;

   disp = re->window_egl_display_get(re->software.ob);
   evasglctx = re->window_gl_context_get(re->software.ob);
   ctx = evasglctx->eglctxt;

#if 0
   // Choose framebuffer configuration
   // DISABLED FOR NOW
   if (sfc->pbuffer.color_fmt != EVAS_GL_NO_FBO)
     {
        config_attrs[i++] = EGL_RED_SIZE;
        config_attrs[i++] = 1;
        config_attrs[i++] = EGL_GREEN_SIZE;
        config_attrs[i++] = 1;
        config_attrs[i++] = EGL_BLUE_SIZE;
        config_attrs[i++] = 1;

        if (sfc->pbuffer.color_fmt == EVAS_GL_RGBA_8888)
          {
             config_attrs[i++] = EGL_ALPHA_SIZE;
             config_attrs[i++] = 1;
             //config_attrs[i++] = EGL_BIND_TO_TEXTURE_RGBA;
             //config_attrs[i++] = EGL_TRUE;
          }
        else
          {
             //config_attrs[i++] = EGL_BIND_TO_TEXTURE_RGB;
             //config_attrs[i++] = EGL_TRUE;
          }
     }

   if (sfc->depth_fmt || sfc->depth_stencil_fmt)
     {
        config_attrs[i++] = EGL_DEPTH_SIZE;
        config_attrs[i++] = 1;
     }

   if (sfc->stencil_fmt || sfc->depth_stencil_fmt)
     {
        config_attrs[i++] = EGL_STENCIL_SIZE;
        config_attrs[i++] = 1;
     }

   config_attrs[i++] = EGL_RENDERABLE_TYPE;
   if (gles3_supported)
     config_attrs[i++] = EGL_OPENGL_ES3_BIT_KHR;
   else
     config_attrs[i++] = EGL_OPENGL_ES2_BIT;
   config_attrs[i++] = EGL_SURFACE_TYPE;
   config_attrs[i++] = EGL_PBUFFER_BIT;
   config_attrs[i++] = EGL_NONE;
#else
   // It looks like evas_eglMakeCurrent might fail if we use a different config from
   // the actual display surface. This is weird.
   i = 0;
   config_attrs[i++] = EGL_CONFIG_ID;
   config_attrs[i++] = 0;
   config_attrs[i++] = EGL_NONE;
   eglQueryContext(disp, ctx, EGL_CONFIG_ID, &config_attrs[1]);
#endif

   if (!eglChooseConfig(disp, config_attrs, &egl_cfg, 1, &num_config)
       || (num_config < 1))
     {
        int err = eglGetError();
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        ERR("eglChooseConfig failed with error %x", err);
        return NULL;
     }

   // Now, choose the config for the PBuffer
   i = 0;
   surface_attrs[i++] = EGL_WIDTH;
   surface_attrs[i++] = sfc->w;
   surface_attrs[i++] = EGL_HEIGHT;
   surface_attrs[i++] = sfc->h;
#if 0
   // Adding these parameters will trigger EGL_BAD_ATTRIBUTE because
   // the config also requires EGL_BIND_TO_TEXTURE_RGB[A]. But some drivers
   // don't support those configs (eg. nvidia)
   surface_attrs[i++] = EGL_TEXTURE_FORMAT;
   if (sfc->pbuffer.color_fmt == EVAS_GL_RGB_888)
     surface_attrs[i++] = EGL_TEXTURE_RGB;
   else
     surface_attrs[i++] = EGL_TEXTURE_RGBA;
   surface_attrs[i++] = EGL_TEXTURE_TARGET;
   surface_attrs[i++] = EGL_TEXTURE_2D;
   surface_attrs[i++] = EGL_MIPMAP_TEXTURE;
   surface_attrs[i++] = EINA_TRUE;
#endif
   surface_attrs[i++] = EGL_NONE;

   egl_sfc = eglCreatePbufferSurface(disp, egl_cfg, surface_attrs);
   if (!egl_sfc)
     {
        int err = eglGetError();
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        ERR("eglCreatePbufferSurface failed with error %x", err);
        return NULL;
     }

   return egl_sfc;
}

/**
 * @brief Destroys an EGL pbuffer surface.
 * @param data The Render_Engine instance.
 * @param surface The EGLSurface handle of the pbuffer to destroy.
 * @return 1 on success, 0 on failure.
 */
static int
evgl_eng_pbuffer_surface_destroy(void *data, void *surface)
{
   Render_Engine *re = data;

   /* EVGLINIT(re, 0); */
   if (!data)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   if (!surface)
     {
        ERR("Invalid surface.");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_SURFACE);
        return 0;
     }

   eglDestroySurface(eng_get_ob(re)->egl_disp, (EGLSurface)surface);

   return 1;
}

/**
 * @brief Creates an indirect rendering surface (Not implemented for Win32).
 *
 * This function is intended to create an offscreen surface (like a Pixmap on X11)
 * that can be rendered to using a separate context and then efficiently used as a
 * texture source in the main Evas GL context, typically via EGLImage.
 * This Win32 implementation is currently a stub.
 *
 * @param evgl The EVGL_Engine instance (unused).
 * @param data The Render_Engine instance.
 * @param evgl_sfc Pointer to the EVGL_Surface structure to be filled.
 * @param cfg Desired configuration for the surface (color format, depth, stencil, etc.).
 * @param w Width of the surface.
 * @param h Height of the surface.
 * @return A handle to the created surface (e.g., the EVGL_Surface pointer) or NULL on failure.
 *         Currently always returns 0.
 */
static void *
evgl_eng_indirect_surface_create(EVGL_Engine *evgl EINA_UNUSED, void *data,
                              EVGL_Surface *evgl_sfc,
                              Evas_GL_Config *cfg, int w, int h)
{
  // FIXME: Implement indirect surface creation for Win32 (e.g., using Pbuffers or FBOs with EGLImage)
#if 0 // Original X11 implementation stub
   Render_Engine *re = data;
   Eina_Bool alpha = EINA_FALSE;
   int colordepth;
   Pixmap px;

   if (!re || !evgl_sfc || !cfg)
     {
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   if ((w < 1) || (h < 1))
     {
        ERR("Inconsistent parameters, not creating any surface!");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_PARAMETER);
        return NULL;
     }

   /* Choose appropriate pixmap depth */
   if (cfg->color_format == EVAS_GL_RGBA_8888)
     {
        alpha = EINA_TRUE;
        colordepth = 32;
     }
   else if (cfg->color_format == EVAS_GL_RGB_888)
     colordepth = 24;
   else // this could also be XDefaultDepth but this case shouldn't happen
     colordepth = 24;

   px = XCreatePixmap(eng_get_ob(re)->disp, eng_get_ob(re)->win, w, h, colordepth);
   if (!px)
     {
        ERR("Failed to create XPixmap!");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_ALLOC);
        return NULL;
     }

   EGLSurface egl_sfc;
   EGLConfig egl_cfg;
   int i, num = 0, best = -1;
   EGLConfig configs[200];
   int config_attrs[40];
   Eina_Bool found = EINA_FALSE;
   int msaa = 0, depth = 0, stencil = 0;
   Visual *visual = NULL;
   Eina_Bool retried = EINA_FALSE;
   EGLint val = 0;

   /* Now we need to iterate over all EGL configurations to check the compatible
    * ones and finally check their visual ID. */

   if ((cfg->depth_bits > EVAS_GL_DEPTH_NONE) &&
       (cfg->depth_bits <= EVAS_GL_DEPTH_BIT_32))
     depth = 8 * ((int) cfg->depth_bits);

   if ((cfg->stencil_bits > EVAS_GL_STENCIL_NONE) &&
       (cfg->stencil_bits <= EVAS_GL_STENCIL_BIT_16))
     stencil = 1 << ((int) cfg->stencil_bits - 1);

   if ((cfg->multisample_bits > EVAS_GL_MULTISAMPLE_NONE) &&
       (cfg->multisample_bits <= EVAS_GL_MULTISAMPLE_HIGH))
     msaa = evgl->caps.msaa_samples[(int) cfg->multisample_bits - 1];

try_again:
   i = 0;
   config_attrs[i++] = EGL_SURFACE_TYPE;
   config_attrs[i++] = EGL_PIXMAP_BIT;
   config_attrs[i++] = EGL_RENDERABLE_TYPE;
   if (cfg->gles_version == EVAS_GL_GLES_3_X)
     config_attrs[i++] = EGL_OPENGL_ES3_BIT;
   else if (cfg->gles_version == EVAS_GL_GLES_2_X)
     config_attrs[i++] = EGL_OPENGL_ES2_BIT;
   else
     config_attrs[i++] = EGL_OPENGL_ES_BIT;
   if (alpha)
     {
        config_attrs[i++] = EGL_ALPHA_SIZE;
        config_attrs[i++] = 1; // should it be 8?
        DBG("Requesting RGBA pixmap");
     }
   else
     {
        config_attrs[i++] = EGL_ALPHA_SIZE;
        config_attrs[i++] = 0;
     }
   if (depth)
     {
        depth = 8 * ((int) cfg->depth_bits);
        config_attrs[i++] = EGL_DEPTH_SIZE;
        config_attrs[i++] = depth;
        DBG("Requesting depth buffer size %d", depth);
     }
   if (stencil)
     {
        stencil = 1 << ((int) cfg->stencil_bits - 1);
        config_attrs[i++] = EGL_STENCIL_SIZE;
        config_attrs[i++] = stencil;
        DBG("Requesting stencil buffer size %d", stencil);
     }
   if (msaa)
     {
        msaa = evgl->caps.msaa_samples[(int) cfg->multisample_bits - 1];
        config_attrs[i++] = EGL_SAMPLE_BUFFERS;
        config_attrs[i++] = 1;
        config_attrs[i++] = EGL_SAMPLES;
        config_attrs[i++] = msaa;
        DBG("Requesting MSAA buffer with %d samples", msaa);
     }
   config_attrs[i++] = EGL_NONE;
   config_attrs[i++] = 0;

   if (!eglChooseConfig(eng_get_ob(re)->egl_disp, config_attrs, configs, 200, &num) || !num)
     {
        int err = eglGetError();
        ERR("eglChooseConfig() can't find any configs, error: %x", err);
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        XFreePixmap(eng_get_ob(re)->disp, px);
        return NULL;
     }

   DBG("Found %d potential configurations", num);
   for (i = 0; (i < num) && !found; i++)
     {
        VisualID visid = 0;
        XVisualInfo *xvi, vi_in;
        XRenderPictFormat *fmt;
        int nvi = 0, j;

        if (!eglGetConfigAttrib(eng_get_ob(re)->egl_disp, configs[i],
                                EGL_NATIVE_VISUAL_ID, &val))
          continue;

        // Find matching visuals. Only alpha & depth are really valid here.
        visid = val;
        vi_in.screen = eng_get_ob(re)->screen;
        vi_in.visualid = visid;
        xvi = XGetVisualInfo(eng_get_ob(re)->disp,
                             VisualScreenMask | VisualIDMask,
                             &vi_in, &nvi);
        if (xvi)
          {
             for (j = 0; (j < nvi) && !found; j++)
               {
                  if (xvi[j].depth >= colordepth)
                    {
                       if (best < 0) best = i;
                       if (alpha)
                         {
                            fmt = XRenderFindVisualFormat(eng_get_ob(re)->disp, xvi[j].visual);
                            if (fmt && (fmt->direct.alphaMask))
                              found = EINA_TRUE;
                         }
                       else found = EINA_TRUE;
                    }
               }
             if (found)
               {
                  egl_cfg = configs[i];
                  visual = xvi[j].visual;
                  XFree(xvi);
                  break;
               }
             XFree(xvi);
          }
     }

   if (!found)
     {
        if (num && (best >= 0))
          {
             ERR("No matching config found. Trying with EGL config #%d", best);
             egl_cfg = configs[best];
          }
        else if (msaa && !retried)
          {
             ERR("Trying again without MSAA.");
             msaa = 0;
             retried = EINA_TRUE;
             goto try_again;
          }
        else
          {
             // This config will probably not work, but we try anyways.
             // NOTE: Maybe it would be safer to just return NULL here, leaving
             // the app responsible for changing its config.
             ERR("XGetVisualInfo failed. Trying with the window's EGL config.");
             egl_cfg = eng_get_ob(re)->egl_config;
          }
     }

   egl_sfc = eglCreatePixmapSurface(eng_get_ob(re)->egl_disp, egl_cfg, px, NULL);
   if (!egl_sfc)
     {
        int err = eglGetError();
        ERR("eglCreatePixmapSurface failed with error: %x", err);
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        XFreePixmap(eng_get_ob(re)->disp, px);
        return NULL;
     }

   if (extn_have_y_inverted &&
       eglGetConfigAttrib(eng_get_ob(re)->egl_disp, egl_cfg,
                          EGL_Y_INVERTED_NOK, &val))
     evgl_sfc->yinvert = val;
   else
     evgl_sfc->yinvert = 1;

   evgl_sfc->indirect = EINA_TRUE;
   evgl_sfc->indirect_sfc = egl_sfc;
   evgl_sfc->indirect_sfc_native = (void *)(intptr_t) px;
   evgl_sfc->indirect_sfc_visual = visual;
   evgl_sfc->indirect_sfc_config = egl_cfg;
   DBG("Successfully created indirect surface: Pixmap %lu EGLSurface %p", px, egl_sfc);
   return evgl_sfc;

#endif
   return 0;
   (void)data;
   (void)evgl_sfc;
   (void)cfg;
   (void)w;
   (void)h;
}

/**
 * @brief Destroys an indirect rendering surface (Not implemented for Win32).
 *
 * This should free the resources associated with the surface created by
 * evgl_eng_indirect_surface_create (e.g., EGLSurface, native Pixmap/Pbuffer).
 * This Win32 implementation is currently a stub.
 *
 * @param data The Render_Engine instance.
 * @param evgl_sfc The EVGL_Surface structure representing the surface to destroy.
 * @return 1 on success, 0 on failure. Currently always returns 0.
 */
static int
evgl_eng_indirect_surface_destroy(void *data, EVGL_Surface *evgl_sfc)
{
  // FIXME: Implement indirect surface destruction for Win32
#if 0 // Original X11 implementation stub
   Render_Engine *re = (Render_Engine *)data;

   if (!re)
     {
        ERR("Invalid Render Engine Data!");
        glsym_evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return 0;
     }

   if ((!evgl_sfc) || (!evgl_sfc->indirect_sfc))
     {
        ERR("Invalid surface");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_SURFACE);
        return 0;
     }

   eglDestroySurface(eng_get_ob(re)->egl_disp, (EGLSurface)evgl_sfc->indirect_sfc);

   if (!evgl_sfc->indirect_sfc_native)
     {
        ERR("Inconsistent parameters, not freeing XPixmap for indirect surface!");
        glsym_evas_gl_common_error_set(EVAS_GL_BAD_PARAMETER);
        return 0;
     }

   XFreePixmap(eng_get_ob(re)->disp, (Pixmap)evgl_sfc->indirect_sfc_native);

   return 1;
#endif
   return 0;
   (void)data;
   (void)evgl_sfc;
}

/**
 * @brief Creates a GLES context specifically for indirect rendering.
 *
 * This context is intended to be used with surfaces created by
 * `evgl_eng_indirect_surface_create`. It must share resources with the
 * provided `share_ctx` (usually the main Evas GL context or another EVGL context).
 * It attempts to match the GLES version of the share context.
 *
 * @param data The Render_Engine instance.
 * @param share_ctx The EVGL_Context to share resources with (required).
 * @param sfc The EVGL_Surface this context will primarily render to (used to find a compatible EGL config).
 * @return The EGLContext handle or EGL_NO_CONTEXT on error.
 */
static void *
evgl_eng_gles_context_create(void *data,
                              EVGL_Context *share_ctx, EVGL_Surface *sfc)
{
   Render_Engine *re = data;
   if (!re) return NULL;

   EGLContext context = EGL_NO_CONTEXT;
   int context_attrs[3];
   EGLConfig config;

   if (!share_ctx)
     {
        ERR("Share context not set, Unable to retrieve GLES version");
        return NULL;
     }

   context_attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   context_attrs[1] = share_ctx->version;
   context_attrs[2] = EGL_NONE;

   if (!sfc || !sfc->indirect_sfc_config)
     {
        ERR("Surface is not set! Creating context anyways but evas_eglMakeCurrent "
            "might very well fail with EGL_BAD_MATCH (0x3009)");
        config = eng_get_ob(re)->egl_config;
     }
   else config = sfc->indirect_sfc_config;

   context = eglCreateContext(eng_get_ob(re)->egl_disp, config,
                              share_ctx->context,
                              context_attrs);
   if (!context)
     {
        int err = eglGetError();
        ERR("eglCreateContext failed with error 0x%x", err);
        glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
        return NULL;
     }

   DBG("Successfully created context for indirect rendering.");
   return context;
}

/**
 * @brief Retrieves the detected configuration of the main Evas window surface.
 *
 * Provides the actual depth buffer size, stencil buffer size, and MSAA samples
 * used by the main Evas rendering surface, as detected during initialization.
 *
 * @param data The Render_Engine instance.
 * @param[out] win_depth Pointer to store the detected depth buffer size (in bits). Can be NULL.
 * @param[out] win_stencil Pointer to store the detected stencil buffer size (in bits). Can be NULL.
 * @param[out] win_msaa Pointer to store the detected MSAA samples (0 if none). Can be NULL.
 */
static void
evgl_eng_native_win_surface_config_get(void *data, int *win_depth,
                                         int *win_stencil, int *win_msaa)
{
   Render_Engine *re = data;
   if (!re) return;

   if (win_depth)
     *win_depth = eng_get_ob(re)->detected.depth_buffer_size;
   if (win_stencil)
     *win_stencil = eng_get_ob(re)->detected.stencil_buffer_size;
   if (win_msaa)
     *win_msaa = eng_get_ob(re)->detected.msaa;

   DBG("Window config(depth %d, stencil %d, msaa %d)",
       eng_get_ob(re)->detected.depth_buffer_size,
       eng_get_ob(re)->detected.stencil_buffer_size,
       eng_get_ob(re)->detected.msaa);
}

/** @} */ // End of EVGL_Win32 group

/** @brief Table of EVGL interface function pointers for this engine. */
static const EVGL_Interface evgl_funcs =
{
   evgl_eng_display_get,                 /**< Get EGL display */
   evgl_eng_evas_surface_get,            /**< Get main Evas EGL surface */
   evgl_eng_native_window_create,        /**< Create native Win32 window */
   evgl_eng_native_window_destroy,       /**< Destroy native Win32 window */
   evgl_eng_window_surface_create,       /**< Create EGL window surface */
   evgl_eng_window_surface_destroy,      /**< Destroy EGL window surface */
   evgl_eng_context_create,              /**< Create EGL context */
   evgl_eng_context_destroy,             /**< Destroy EGL context */
   evgl_eng_make_current,                /**< Make context/surface current */
   evgl_eng_proc_address_get,            /**< Get GL/EGL function address */
   evgl_eng_string_get,                  /**< Get EGL extension string */
   evgl_eng_rotation_angle_get,          /**< Get Evas rotation angle */
   evgl_eng_pbuffer_surface_create,      /**< Create EGL pbuffer surface */
   evgl_eng_pbuffer_surface_destroy,     /**< Destroy EGL pbuffer surface */
   evgl_eng_indirect_surface_create,     /**< Create indirect surface (stub) */
   evgl_eng_indirect_surface_destroy,    /**< Destroy indirect surface (stub) */
   evgl_eng_gles_context_create,         /**< Create GLES context for indirect rendering */
   evgl_eng_native_win_surface_config_get, /**< Get main window surface config */
};

//----------------------------------------------------------//

/**
 * @brief Resolves function pointers from the linked Evas GL generic module.
 *
 * This function uses dlsym (or equivalent) to find the addresses of functions
 * provided by the `gl_generic` Evas engine module, which this module inherits from.
 * It also finds a suitable `eglGetProcAddress` implementation.
 * This should be called once during module initialization.
 */
static void
gl_symbols(void)
{
   static int done = 0;

   if (done) return;

#define LINK2GENERIC(sym) \
   glsym_##sym = dlsym(RTLD_DEFAULT, #sym); \
   if (!glsym_##sym) ERR("Could not find function '%s'", #sym);

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
   LINK2GENERIC(evgl_native_surface_buffer_get);
   LINK2GENERIC(evgl_native_surface_yinvert_get);
   LINK2GENERIC(evgl_current_native_context_get);
   LINK2GENERIC(evas_gl_symbols);
   LINK2GENERIC(evas_gl_common_error_get);
   LINK2GENERIC(evas_gl_common_error_set);
   LINK2GENERIC(evas_gl_common_current_context_get);
   LINK2GENERIC(evas_gl_common_shaders_flush);

#define FINDSYM(dst, sym) if (!dst) dst = dlsym(RTLD_DEFAULT, sym);

   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressKHR");
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressEXT");
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddressARB");
   FINDSYM(glsym_eglGetProcAddress, "eglGetProcAddress");

#undef FINDSYM

   done = 1;
}

/**
 * @brief Resolves EGL and GL extension function pointers based on available extensions.
 *
 * Queries the EGL and GL extension strings and uses `eglGetProcAddress` (obtained
 * via `gl_symbols`) or `dlsym` to find function pointers for specific extensions
 * like buffer damage, partial update, EGLImage, etc.
 * This should be called once per output buffer (`Outbuf`) after it's created.
 *
 * @param ob The output buffer providing the EGL display and context.
 */
void
eng_gl_symbols(Outbuf *ob)
{
   static int done = 0; // FIXME: This 'done' flag seems wrong, should probably be per-Outbuf or context
   const char *exts;

   if (done) return;

   /* GetProcAddress() may not return NULL, even if the extension is not
    * supported. Nvidia drivers since version 360 never return NULL, thus
    * we need to always match the function name with their full extension
    * name. Other drivers tend to return NULL for glX/egl prefixed names, but
    * this could change in the future.
    *
    * -- jpeg, 2016/08/04
    */

#define FINDSYM(dst, sym, ext) do { \
   if (!dst) { \
      if (eina_str_has_extension(exts, ext) && glsym_eglGetProcAddress) \
        dst =  glsym_eglGetProcAddress(sym); \
      if (!dst) \
        dst =  dlsym(RTLD_DEFAULT, sym); \
   }} while (0)

   // Find EGL extensions
   exts = eglQueryString(ob->egl_disp, EGL_EXTENSIONS);

   // Find GL extensions
   glsym_evas_gl_symbols((void*)glsym_eglGetProcAddress, exts);

   LINK2GENERIC(evas_gl_common_eglCreateImage);
   LINK2GENERIC(evas_gl_common_eglDestroyImage);

   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamage", NULL);
   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamageEXT", "EGL_EXT_swap_buffers_with_damage");
   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamageKHR", "EGL_KHR_swap_buffers_with_damage");
   FINDSYM(glsym_eglSwapBuffersWithDamage, "eglSwapBuffersWithDamageINTEL", "EGL_INTEL_swap_buffers_with_damage");

   FINDSYM(glsym_eglSetDamageRegionKHR, "eglSetDamageRegionKHR", "EGL_KHR_partial_update");

   FINDSYM(glsym_eglQueryWaylandBufferWL, "eglQueryWaylandBufferWL", "EGL_WL_bind_wayland_display");

   // This is a GL extension
   exts = (const char *) glGetString(GL_EXTENSIONS);
   FINDSYM(glsym_glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES", "GL_OES_EGL_image_external");
   FINDSYM(glsym_glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES", "GL_OES_EGL_image");

#undef FINDSYM

   done = 1;
}

/**
 * @brief Checks EGL extensions and disables features or applies workarounds.
 *
 * This function queries the EGL extension string and:
 * - Disables partial updates (buffer_age, damage) if the extensions are missing
 *   or if explicitly disabled via environment variables.
 * - Disables Y-inversion for texture_from_pixmap if the extension is missing
 *   or if specific buggy drivers (Mesa Intel) are detected.
 * - Disables swap_buffers_with_damage if the extension is missing.
 * - Detects Tizen-specific native surface extensions.
 *
 * @param re The Render_Engine instance.
 */
static void
gl_extn_veto(Render_Engine *re)
{
   const char *str = NULL;

   str = eglQueryString(eng_get_ob(re)->egl_disp, EGL_EXTENSIONS);
   if (str)
     {
        const char *s;
        if (getenv("EVAS_GL_INFO"))
          printf("EGL EXTN:\n%s\n", str);
        // Disable Partial Rendering
        if ((s = getenv("EVAS_GL_PARTIAL_DISABLE")) && atoi(s))
          {
             extn_have_buffer_age = 0;
             glsym_eglSwapBuffersWithDamage = NULL;
             glsym_eglSetDamageRegionKHR = NULL;
          }
        if (!strstr(str, "EGL_EXT_buffer_age"))
          {
             if (!strstr(str, "EGL_KHR_partial_update"))
               extn_have_buffer_age = 0;
          }
        if (!strstr(str, "EGL_KHR_partial_update"))
          {
             glsym_eglSetDamageRegionKHR = NULL;
          }
        if (!strstr(str, "EGL_NOK_texture_from_pixmap"))
          {
             extn_have_y_inverted = 0;
          }
        else
          {
             const GLubyte *vendor, *renderer;

             vendor = glGetString(GL_VENDOR);
             renderer = glGetString(GL_RENDERER);
             // XXX: workaround mesa bug!
             // looking for mesa and intel build which is known to
             // advertise the EGL_NOK_texture_from_pixmap extension
             // but not set it correctly. guessing vendor/renderer
             // strings will be like the following:
             // OpenGL vendor string: Intel Open Source Technology Center
             // OpenGL renderer string: Mesa DRI Intel(R) Sandybridge Desktop
             if (((vendor) && (strstr((const char *)vendor, "Intel"))) &&
                 ((renderer) && (strstr((const char *)renderer, "Mesa"))) &&
                 ((renderer) && (strstr((const char *)renderer, "Intel")))
                )
               extn_have_y_inverted = 0;
          }
        if ((!strstr(str, "EGL_EXT_swap_buffers_with_damage")) &&
            (!strstr(str, "EGL_KHR_swap_buffers_with_damage")))
          {
             glsym_eglSwapBuffersWithDamage = NULL;
          }
        if (strstr(str, "EGL_TIZEN_image_native_surface"))
          {
             eng_get_ob(re)->gl_context->shared->info.egl_tbm_ext = 1;
          }
     }
   else
     {
        if (getenv("EVAS_GL_INFO"))
          printf("NO EGL EXTN!\n");
        extn_have_buffer_age = 0;
     }
}

/**
 * @brief Callback function for making the GL context current during preload rendering.
 *
 * This is passed to the Evas GL common preload functions. It uses the
 * `evas_eglMakeCurrent` wrapper to manage context switching.
 *
 * @param data The Outbuf pointer associated with the context.
 * @param doit EINA_TRUE to make the Outbuf's context current, EINA_FALSE to release the context.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
eng_preload_make_current(void *data, void *doit)
{
   Outbuf *ob = data;

   if (doit)
     {
        if (!evas_eglMakeCurrent(ob->egl_disp, ob->egl_surface, ob->egl_surface, ob->egl_context))
          return EINA_FALSE;
     }
   else
     {
        if (!evas_eglMakeCurrent(ob->egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
          return EINA_FALSE;
     }
   return EINA_TRUE;
}

/************* Evas Module Engine Funtions Overloading *************/

int _evas_engine_gl_win32_log_dom = -1;
/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc; /**< Function tables for Evas module */

/**
 * @brief Releases resources associated with a window surface before freeing the Outbuf.
 *
 * Specifically, it relaxes the preload renderer lock and destroys the EGL surface.
 *
 * @param re The Render_Engine instance.
 */
static void
_re_winfree(Render_Engine *re)
{
   if (!eng_get_ob(re)->surf) return;
   // Release context before destroying surface
   glsym_evas_gl_preload_render_relax(eng_preload_make_current, eng_get_ob(re));
   eng_window_unsurf(eng_get_ob(re));
}

/**
 * @brief Flushes GL shaders during idle time for a specific output buffer.
 * @param ob The Outbuf whose context's shaders should be flushed.
 */
static void
eng_outbuf_idle_flush(Outbuf *ob)
{
   if (glsym_evas_gl_common_shaders_flush)
     glsym_evas_gl_common_shaders_flush(ob->gl_context->shared);
}

/**
 * @brief Sets up the engine info structure with default values.
 * @param info Pointer to the Evas_Engine_Info_GL_Win32 structure.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_GL_Win32 *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Sets up a new rendering output (window).
 *
 * This function initializes the Win32 GL engine for a specific output window.
 * It performs the following steps:
 * 1. Initializes Evas GL preload mechanism if not already done.
 * 2. Allocates the Render_Engine structure.
 * 3. Gets the application instance handle.
 * 4. Registers a simple window class for the hidden EVGL helper window.
 * 5. Creates the main output buffer (`Outbuf`) using `eng_window_new`.
 * 6. Initializes the generic GL engine parts using `evas_render_engine_gl_generic_init`.
 * 7. Increments the active window count.
 * 8. Sets up software generic merge mode.
 * 9. Performs extension vetting (`gl_extn_veto`) if this is the first window.
 * 10. Makes the new window's context current.
 *
 * @param engine The generic engine pointer (unused).
 * @param in Pointer to the Evas_Engine_Info_GL_Win32 structure containing setup details.
 * @param w Initial width of the output.
 * @param h Initial height of the output.
 * @return A pointer to the newly created Render_Engine structure, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   WNDCLASS wc;
   char buf[16];
   Evas_Engine_Info_GL_Win32 *info = in;
   Render_Engine *re = NULL;
   Outbuf *ob = NULL;
   Render_Output_Swap_Mode swap_mode;

   swap_mode = evas_render_engine_gl_swap_mode_get(info->swap_mode);

   // Set this env var to dump files every frame
   // Or set the global var in gdb to 1|0 to turn it on and off
   if (getenv("EVAS_GL_SWAP_BUFFER_DEBUG_ALWAYS"))
     swap_buffer_debug = 1;

   if (swap_buffer_debug_mode == -1)
     {
        if ((debug_dir = getenv("EVAS_GL_SWAP_BUFFER_DEBUG_DIR")))
          {
             int stat;

             stat = CreateDirectoryA(debug_dir, NULL);
             if ((!stat) || errno == EEXIST) swap_buffer_debug_mode = 1;
          }
        else
           swap_buffer_debug_mode = 0;
     }

   if (!initted)
     {
        glsym_evas_gl_preload_init();
     }

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   re->instance = GetModuleHandle(NULL);
   if (!re->instance) goto on_error;

   snprintf(buf, sizeof(buf), "%p", re);
   re->class_name = strdup(buf);
   if (!re->class_name) goto on_error;

   ZeroMemory(&wc, sizeof(wc));
   wc.lpfnWndProc = _procedure;
   wc.hInstance = re->instance;
   wc.lpszClassName = re->class_name;
   if (!RegisterClass(&wc)) goto on_error;

   ob = eng_window_new(info,
                       w, h,
                       swap_mode);
   if (!ob) goto on_error;

   if (!evas_render_engine_gl_generic_init(engine, &re->generic, ob,
                                           eng_outbuf_swap_mode,
                                           eng_outbuf_get_rot,
                                           eng_outbuf_reconfigure,
                                           eng_outbuf_region_first_rect,
                                           eng_outbuf_damage_region_set,
                                           eng_outbuf_new_region_for_update,
                                           eng_outbuf_push_updated_region,
                                           NULL,
                                           eng_outbuf_idle_flush,
                                           eng_outbuf_flush,
                                           NULL,
                                           eng_window_free,
                                           eng_window_use,
                                           eng_outbuf_gl_context_get,
                                           eng_outbuf_egl_display_get,
                                           eng_gl_context_new,
                                           eng_gl_context_use,
                                           &evgl_funcs,
                                           w, h))
     goto on_error;

   gl_wins++;

   evas_render_engine_software_generic_merge_mode_set(&re->generic.software);

   if (!initted)
     {
        gl_extn_veto(re);
        //             evgl_engine_init(re, &evgl_funcs);
        initted = 1;
     }

   eng_window_use(eng_get_ob(re));

   return re;

 on_error:
   if (ob) eng_window_free(ob);
   if (re->class_name) free(re->class_name);
   if (re->instance) FreeLibrary(re->instance);
   free(re);
   return NULL;
}

/**
 * @brief Updates an existing rendering output based on new information.
 *
 * Checks if the window handle, dimensions, rotation, or other parameters
 * in the provided `info` structure have changed compared to the existing
 * `Outbuf`. If necessary, it either creates a completely new `Outbuf`
 * (if window handle or fundamental properties changed) or reconfigures
 * the existing one (if only size/rotation changed).
 *
 * @param engine The generic engine pointer (unused).
 * @param data The Render_Engine instance for the output being updated.
 * @param in Pointer to the Evas_Engine_Info_GL_Win32 structure with potentially updated info.
 * @param w New width.
 * @param h New height.
 * @return 1 on success, 0 on failure (e.g., if creating a new Outbuf fails).
 */
static int
eng_output_update(void *engine EINA_UNUSED, void *data, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_GL_Win32 *info = in;
   Render_Engine *re = data;
   Render_Output_Swap_Mode swap_mode;

   swap_mode = evas_render_engine_gl_swap_mode_get(info->swap_mode);

   if (eng_get_ob(re) && _re_wincheck(eng_get_ob(re)))
     {
        if ((info->info.window != eng_get_ob(re)->window) ||
            (info->stencil_bits != eng_get_ob(re)->stencil_bits) ||
            (info->msaa_bits != eng_get_ob(re)->msaa_bits) ||
            (info->info.destination_alpha != eng_get_ob(re)->alpha))
          {
             Outbuf *ob;

             gl_wins--;

             ob = eng_window_new(info, w, h, swap_mode);
             if (!ob) return 0;

             eng_window_use(ob);
             evas_render_engine_software_generic_update(&re->generic.software,
                                                        ob, w, h);
             gl_wins++;
          }
        else if ((eng_get_ob(re)->w != w) ||
                 (eng_get_ob(re)->h != h) ||
                 (eng_get_ob(re)->info->info.rotation != eng_get_ob(re)->rotation))
          {
             eng_outbuf_reconfigure(eng_get_ob(re), w, h, eng_get_ob(re)->info->info.rotation, 0);
             evas_render_engine_software_generic_update(&re->generic.software,
                                                        re->generic.software.ob,
                                                        w, h);
          }
     }

   eng_window_use(eng_get_ob(re));

   return 1;
}

/**
 * @brief Gets whether the canvas associated with the engine has an alpha channel.
 * @param engine The Render_Engine instance.
 * @return EINA_TRUE if the canvas has alpha, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_canvas_alpha_get(void *engine)
{
   Render_Engine *re = (Render_Engine *)engine;
   return re->generic.software.ob->alpha;
}

/**
 * @brief Frees the resources associated with a rendering output.
 *
 * This function cleans up a Render_Engine instance created by `eng_output_setup`.
 * It performs the following steps:
 * 1. Relaxes the preload renderer lock.
 * 2. Shuts down the EVGL engine interface if this is the last window.
 * 3. Cleans up the generic software engine parts.
 * 4. Decrements the active window count.
 * 5. Unregisters the window class used for the helper window.
 * 6. Frees allocated memory (class name, Render_Engine structure).
 * 7. Shuts down the Evas GL preload mechanism if this was the last window.
 *
 * @param engine The generic engine pointer.
 * @param data The Render_Engine instance to free.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;

   if (re)
     {
        glsym_evas_gl_preload_render_relax(eng_preload_make_current, eng_get_ob(re));

#if 0
        // Destroy the resource surface
        // Only required for EGL case
        if (re->surface)
           eglDestroySurface(eng_get_ob(re)->egl_disp, re->surface);

        // Destroy the resource context
        _destroy_internal_context(re, context);
#endif

        if (gl_wins == 1) glsym_evgl_engine_shutdown(re);

        evas_render_engine_software_generic_clean(engine, &re->generic.software);

        gl_wins--;

        UnregisterClass(re->class_name, re->instance);
        free(re->class_name);
        FreeLibrary(re->instance);
        free(re);
     }
   if ((initted == 1) && (gl_wins == 0))
     {
        glsym_evas_gl_preload_shutdown();
        initted = 0;
     }
}

/**
 * @brief Dumps engine state and releases some resources (e.g., for debugging or state reset).
 *
 * Makes the output's context current, dumps cache information, unloads
 * images and fonts, and releases the window surface resources via `_re_winfree`.
 *
 * @param engine The generic engine pointer.
 * @param data The Render_Engine instance to dump.
 */
static void
eng_output_dump(void *engine, void *data)
{
   Render_Engine *re = data;
   Render_Engine_GL_Generic *e = engine;

   eng_window_use(eng_get_ob(re));
   generic_cache_dump(e->software.surface_cache);
   evas_common_image_image_all_unload();
   evas_common_font_font_all_unload();
   glsym_evas_gl_common_image_all_unload(eng_get_ob(re)->gl_context);
   _re_winfree(re);
}

/**
 * @brief Initializes native surface support for a given type.
 *
 * Currently supports TBM, OpenGL textures, and EvasGL surfaces.
 * Calls platform-specific initialization if needed (e.g., `_evas_native_tbm_init`).
 *
 * @param engine The generic engine pointer (unused).
 * @param type The type of native surface to initialize support for.
 * @return 1 on success or if type is supported, 0 otherwise.
 */
static int
eng_image_native_init(void *engine EINA_UNUSED, Evas_Native_Surface_Type type)
{
   switch (type)
     {
      case EVAS_NATIVE_SURFACE_TBM:
        return _evas_native_tbm_init();
      case EVAS_NATIVE_SURFACE_OPENGL:
      case EVAS_NATIVE_SURFACE_EVASGL:
        return 1;
      default:
        ERR("Native surface type %d not supported!", type);
        return 0;
     }
}

/**
 * @brief Shuts down native surface support for a given type.
 *
 * Calls platform-specific shutdown if needed (e.g., `_evas_native_tbm_shutdown`).
 *
 * @param engine The generic engine pointer (unused).
 * @param type The type of native surface to shut down support for.
 */
static void
eng_image_native_shutdown(void *engine EINA_UNUSED, Evas_Native_Surface_Type type)
{
   switch (type)
     {
      case EVAS_NATIVE_SURFACE_TBM:
        _evas_native_tbm_shutdown();
        return;
      case EVAS_NATIVE_SURFACE_OPENGL:
      case EVAS_NATIVE_SURFACE_EVASGL:
        return;
      default:
        ERR("Native surface type %d not supported!", type);
        return;
     }
}

/////////////////////////////////////////////////////////////////////////
//
//
// FIXME: this is enabled so updates happen - but its SLOOOOOOOOOOOOOOOW
// (i am sure this is the reason)  not to mention seemingly superfluous. but
// i need to enable it for it to work on fglrx at least. havent tried nvidia.
//
// why is this the case? does anyone know? has anyone tried it on other gfx
// drivers?
//
//#define GLX_TEX_PIXMAP_RECREATE 1 // Old X11 specific define

/**
 * @brief Callback function to bind a native surface to a texture target.
 *
 * This function is called by the Evas GL common image handling code when
 * a native image needs to be bound before rendering. It handles different
 * native surface types:
 * - X11 (Pixmap via EGLImage): Re-creates EGLImage if needed (multi-buffering) and calls glEGLImageTargetTexture2DOES. (Currently #if 0'd)
 * - OpenGL (Texture ID): Calls glBindTexture.
 * - TBM (Buffer via EGLImage): Calls glEGLImageTargetTexture2DOES.
 * - EvasGL (Surface): Gets the underlying buffer/texture and calls either glEGLImageTargetTexture2DOES or glBindTexture.
 *
 * @param image Pointer to the Evas_GL_Image structure.
 */
static void
_native_bind_cb(void *image)
{
#if 0 // Implementation details for various surface types (currently inactive/simplified)
   Evas_GL_Image *im = image;
   Native *n = im->native.data;

  if (n->ns.type == EVAS_NATIVE_SURFACE_X11)
    {
       if (n->ns_data.x11.surface)
         {
            if ((n->frame_cnt != im->gc->frame_cnt) &&
                (n->ns_data.x11.multiple_buffer))
              {
                 EGLint err;

                 if (!glsym_evas_gl_common_eglDestroyImage)
                   {
                      ERR("Try eglDestroyImage()/eglCreateImage() on EGL with no support");
                      return;
                   }
                 n->frame_cnt = im->gc->frame_cnt;
                 glsym_evas_gl_common_eglDestroyImage(im->native.disp,
                                                      n->ns_data.x11.surface);
                 if ((err = eglGetError()) != EGL_SUCCESS)
                   {
                      ERR("eglDestroyImage() failed.");
                      glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
                   }

                 n->ns_data.x11.surface = glsym_evas_gl_common_eglCreateImage(im->native.disp,
                                                          EGL_NO_CONTEXT,
                                                          EGL_NATIVE_PIXMAP_KHR,
                                                          (void *)n->ns_data.x11.pixmap,
                                                          NULL);
                 if (!n->ns_data.x11.surface)
                   WRN("eglCreateImage() for Pixmap 0x%#lx failed: %#x", n->ns_data.x11.pixmap, eglGetError());
              }
            if (glsym_glEGLImageTargetTexture2DOES)
              {
                 glsym_glEGLImageTargetTexture2DOES(im->native.target, n->ns_data.x11.surface);
                 GLERRV("glsym_glEGLImageTargetTexture2DOES");
              }
            else
              ERR("Try glEGLImageTargetTexture2DOES on EGL with no support");
         }
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
    {
       glBindTexture(im->native.target, n->ns.data.opengl.texture_id);
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_TBM)
    {
       if (n->ns_data.tbm.surface)
         {
            if (glsym_glEGLImageTargetTexture2DOES)
              {
                 glsym_glEGLImageTargetTexture2DOES(im->native.target, n->ns_data.tbm.surface);
                 GLERRV("glsym_glEGLImageTargetTexture2DOES");
              }
             else
               ERR("Try glEGLImageTargetTexture2DOES on EGL with no support");
         }
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_EVASGL)
    {
       if (n->ns_data.evasgl.surface)
         {
             Eina_Bool is_egl_image = EINA_FALSE;
             void *surface = NULL;

             if (glsym_evgl_native_surface_buffer_get)
               surface = glsym_evgl_native_surface_buffer_get(n->ns_data.evasgl.surface, &is_egl_image);
            if (is_egl_image)
              {
                 if (glsym_glEGLImageTargetTexture2DOES)
                   {
                      glsym_glEGLImageTargetTexture2DOES(im->native.target, surface);
                      GLERRV("glsym_glEGLImageTargetTexture2DOES");
                   }
                 else
                   ERR("Try glEGLImageTargetTexture2DOES on EGL with no support");
              }
            else
              {
                 glBindTexture(GL_TEXTURE_2D, (GLuint)(uintptr_t)surface);
              }
         }
    }

#endif
  (void)image;
}

/**
 * @brief Callback function to unbind a native surface from a texture target.
 *
 * Called after rendering with a native image. For OpenGL textures, it binds
 * texture 0 to the target. Other types might not require explicit unbinding.
 *
 * @param image Pointer to the Evas_GL_Image structure.
 */
static void
_native_unbind_cb(void *image)
{
   Evas_GL_Image *im = image;
   Native *n = im->native.data;

   if (n->ns.type == EVAS_NATIVE_SURFACE_X11)
     {
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
     {
        glBindTexture(im->native.target, 0);
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_TBM)
     {
        // nothing
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_EVASGL)
     {
     }
}

/**
 * @brief Callback function to free resources associated with a native surface image.
 *
 * Called when the Evas_GL_Image using a native surface is freed. It performs:
 * - Removes the image from the corresponding native hash table (Pixmap, Texture ID, TBM buffer, EvasGL surface).
 * - Destroys the associated EGLImageKHR if applicable (X11 Pixmap, TBM).
 * - Frees the internal Native structure.
 * - Clears the native function pointers in the Evas_GL_Image.
 *
 * @param image Pointer to the Evas_GL_Image structure.
 */
static void
_native_free_cb(void *image)
{
  Evas_GL_Image *im = image;
  Native *n = im->native.data;
  uint32_t pmid EINA_UNUSED, texid; // pmid only used in #if 0'd X11 code

  if (n->ns.type == EVAS_NATIVE_SURFACE_X11)
    {
       pmid = n->ns_data.x11.pixmap;
       eina_hash_del(im->native.shared->native_pm_hash, &pmid, im);
       if (n->ns_data.x11.surface)
         {
            int err;
            if (glsym_evas_gl_common_eglDestroyImage)
              {
                 glsym_evas_gl_common_eglDestroyImage(im->native.disp,
                                                      n->ns_data.x11.surface);
                 n->ns_data.x11.surface = 0;
                 if ((err = eglGetError()) != EGL_SUCCESS)
                   {
                      ERR("eglDestroyImage() failed.");
                      glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
                   }
              }
            else
              ERR("Try eglDestroyImage on EGL with no support");
         }
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
    {
       texid = n->ns.data.opengl.texture_id;
       eina_hash_del(im->native.shared->native_tex_hash, &texid, im);
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_TBM)
    {
       eina_hash_del(im->native.shared->native_tbm_hash, &n->ns_data.tbm.buffer, im);
       if (n->ns_data.tbm.surface)
         {
            int err;
            if (glsym_evas_gl_common_eglDestroyImage)
              {
                 glsym_evas_gl_common_eglDestroyImage(im->native.disp,
                                                      n->ns_data.tbm.surface);
                 n->ns_data.tbm.surface = 0;
                 if ((err = eglGetError()) != EGL_SUCCESS)
                   {
                      ERR("eglDestroyImage() failed.");
                      glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
                   }
              }
            else
              ERR("Try eglDestroyImage on EGL with no support");
         }
    }
  else if (n->ns.type == EVAS_NATIVE_SURFACE_EVASGL)
    {
       eina_hash_del(im->native.shared->native_evasgl_hash, &n->ns.data.evasgl.surface, im);
    }
  im->native.data        = NULL;
  im->native.func.bind   = NULL;
  im->native.func.unbind = NULL;
  im->native.func.free   = NULL;
  free(n);
}

/**
 * @brief Callback function to determine if a native surface requires Y-inversion.
 *
 * Checks the native surface type and returns the appropriate Y-inversion flag.
 * - X11: Checks EGL_Y_INVERTED_NOK attribute if `extn_have_y_inverted` is set.
 * - OpenGL: Assumes no inversion (0).
 * - TBM: Assumes inversion (1).
 * - EvasGL: Queries the EvasGL surface using `glsym_evgl_native_surface_yinvert_get`.
 *
 * @param image Pointer to the Evas_GL_Image structure.
 * @return 1 if Y-inversion is needed, 0 otherwise.
 */
static int
_native_yinvert_cb(void *image)
{
   Evas_GL_Image *im = image;
   Native *n = im->native.data;
   int yinvert = 0;
   int val EINA_UNUSED; // Only used in X11 path

   // Yinvert callback should only be used for EVAS_NATIVE_SURFACE_EVASGL type now,
   // as yinvert value is not changed for other types.
   if (n->ns.type == EVAS_NATIVE_SURFACE_X11)
     {
        if (extn_have_y_inverted &&
            eglGetConfigAttrib(im->native.disp, n->ns_data.x11.config,
                               EGL_Y_INVERTED_NOK, &val))
          yinvert = val;
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_OPENGL)
     {
        yinvert = 0;
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_TBM)
     {
        yinvert = 1;
     }
   else if (n->ns.type == EVAS_NATIVE_SURFACE_EVASGL)
     {
        yinvert = glsym_evgl_native_surface_yinvert_get(n->ns_data.evasgl.surface);
     }

   return yinvert;
}

/**
 * @brief Sets or updates the native surface associated with an Evas image.
 *
 * This function handles the logic for associating an Evas_GL_Image with a
 * native surface (like an external OpenGL texture, TBM buffer, or EvasGL surface).
 *
 * It performs the following:
 * 1. Finds the Evas GL context.
 * 2. If the input `image` is NULL, creates a new placeholder Evas_GL_Image if the
 *    native type is OpenGL (as size info is available).
 * 3. Checks if the provided `native` surface is the same as the one already
 *    associated with the `image`. If so, returns the existing `image`.
 * 4. If `native` is NULL, frees the existing `image` and returns NULL.
 * 5. Checks hash tables (texture ID, TBM buffer, EvasGL surface) to see if
 *    another Evas_GL_Image already uses this native surface. If found,
 *    references the existing image, frees the input `image`, and returns the found one.
 * 6. If no existing image is found, creates a new Evas_GL_Image placeholder.
 * 7. Allocates and initializes a `Native` struct to store the native surface info.
 * 8. Based on the `ns->type`:
 *    - **X11 (Pixmap):** (Currently #if 0'd) Creates EGLImage, sets callbacks.
 *    - **OpenGL (Texture ID):** Stores texture/FBO IDs, adds to hash, sets callbacks.
 *    - **TBM (Buffer):** Creates EGLImage, adds to hash, sets callbacks, sets target to GL_TEXTURE_EXTERNAL_OES.
 *    - **EvasGL (Surface):** Stores surface, adds to hash, sets callbacks (including yinvert).
 * 9. Enables native support on the Evas_GL_Image.
 * 10. Returns the (potentially new) Evas_GL_Image associated with the native surface.
 *
 * @param engine The generic engine pointer.
 * @param image The existing Evas_GL_Image to update, or NULL to create a new one.
 * @param native Pointer to the Evas_Native_Surface structure describing the native resource, or NULL to detach.
 * @return The Evas_GL_Image associated with the native surface, or NULL on failure or detachment.
 */
static void *
eng_image_native_set(void *engine, void *image, void *native)
{
  const Evas_Native_Surface *ns = native;
  Evas_Engine_GL_Context *gl_context;
  Evas_GL_Image *im = image, *im2 = NULL;
#if 0
  Visual *vis = NULL;
  Pixmap pm = 0;
  uint32_t pmid;
#endif
  Native *n = NULL;
  uint32_t texid;
  unsigned int tex = 0;
  unsigned int fbo = 0;
  void *buffer = NULL;
  Outbuf *ob;

  gl_context = gl_generic_context_find(engine, 1);
  ob = gl_generic_any_output_get(engine);
  if (!im)
    {
       if ((ns) && (ns->type == EVAS_NATIVE_SURFACE_OPENGL))
         {
            im = glsym_evas_gl_common_image_new_from_data(gl_context,
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
       if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
         {
            tex = ns->data.opengl.texture_id;
            fbo = ns->data.opengl.framebuffer_id;
            if (im->native.data)
              {
                 Evas_Native_Surface *ens = im->native.data;
                 if ((ens->data.opengl.texture_id == tex) &&
                     (ens->data.opengl.framebuffer_id == fbo))
                   return im;
              }
         }
       else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
         {
            buffer = ns->data.tbm.buffer;
            if (im->native.data)
              {
                 Evas_Native_Surface *ens = im->native.data;
                 if (ens->data.tbm.buffer == buffer)
                   return im;
              }
         }
       else if (ns->type == EVAS_NATIVE_SURFACE_EVASGL)
         {
            buffer = ns->data.evasgl.surface;
            if (im->native.data)
              {
                 Evas_Native_Surface *ens = im->native.data;
                 if (ens->data.evasgl.surface == buffer)
                   return im;
              }
         }
    }
   if (!ns)
     {
        glsym_evas_gl_common_image_free(im);
        return NULL;
     }


  if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
    {
       texid = tex;
       im2 = eina_hash_find(gl_context->shared->native_tex_hash, &texid);
       if (im2 == im) return im;
       if (im2)
         {
            n = im2->native.data;
            if (n)
              {
                 glsym_evas_gl_common_image_ref(im2);
                 glsym_evas_gl_common_image_free(im);
                 return im2;
              }
         }
    }
  else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
    {
       im2 = eina_hash_find(gl_context->shared->native_tbm_hash, &buffer);
       if (im2 == im) return im;
       if (im2)
         {
            n = im2->native.data;
            if (n)
             {
                glsym_evas_gl_common_image_ref(im2);
                glsym_evas_gl_common_image_free(im);
                return im2;
             }
         }
    }
  else if (ns->type == EVAS_NATIVE_SURFACE_EVASGL)
    {
       im2 = eina_hash_find(gl_context->shared->native_evasgl_hash, &buffer);
       if (im2 == im) return im;
       if (im2)
         {
            n = im2->native.data;
            if (n)
              {
                 glsym_evas_gl_common_image_ref(im2);
                 glsym_evas_gl_common_image_free(im);
                 return im2;
              }
         }
    }

  im2 = glsym_evas_gl_common_image_new_from_data(gl_context,
                                                 im->w, im->h, NULL, im->alpha,
                                                 EVAS_COLORSPACE_ARGB8888);
  glsym_evas_gl_common_image_free(im);
  im = im2;
  if (!im) return NULL;
  if (ns->type == EVAS_NATIVE_SURFACE_X11)
    {
      // FIXME to do
#if 0
       if (native)
         {
            if (!glsym_evas_gl_common_eglDestroyImage)
              {
                 ERR("Try eglCreateImage on EGL with no support");
                 return NULL;
              }
            n = calloc(1, sizeof(Native));
            if (n)
              {
                 EGLConfig egl_config;
                 int config_attrs[20];
                 int num_config, i = 0;
                 int yinvert = 1;

                 // assume 32bit pixmap! :)
                 config_attrs[i++] = EGL_RED_SIZE;
                 config_attrs[i++] = 8;
                 config_attrs[i++] = EGL_GREEN_SIZE;
                 config_attrs[i++] = 8;
                 config_attrs[i++] = EGL_BLUE_SIZE;
                 config_attrs[i++] = 8;
                 config_attrs[i++] = EGL_ALPHA_SIZE;
                 config_attrs[i++] = 8;
                 config_attrs[i++] = EGL_DEPTH_SIZE;
                 config_attrs[i++] = 0;
                 config_attrs[i++] = EGL_STENCIL_SIZE;
                 config_attrs[i++] = 0;
                 config_attrs[i++] = EGL_RENDERABLE_TYPE;
                 if (gles3_supported)
                   config_attrs[i++] = EGL_OPENGL_ES3_BIT_KHR;
                 else
                   config_attrs[i++] = EGL_OPENGL_ES2_BIT;
                 config_attrs[i++] = EGL_SURFACE_TYPE;
                 config_attrs[i++] = EGL_PIXMAP_BIT;
                 config_attrs[i++] = EGL_NONE;

                 if (!eglChooseConfig(ob->egl_disp, config_attrs,
                                      &egl_config, 1, &num_config))
                   {
                      int err = eglGetError();
                      ERR("eglChooseConfig() failed for pixmap %#lx, "
                          "num_config = %i with error %d", pm, num_config, err);
                      glsym_evas_gl_common_error_set(err - EGL_SUCCESS);
                      free(n);
                      return NULL;
                   }
                 else
                   {
                      int val;
                      if (extn_have_y_inverted &&
                          eglGetConfigAttrib(ob->egl_disp, egl_config,
                                             EGL_Y_INVERTED_NOK, &val))
                            yinvert = val;
                   }

                 memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
                 n->ns_data.x11.pixmap = pm;
                 n->ns_data.x11.visual = vis;
                 n->ns_data.x11.surface = glsym_evas_gl_common_eglCreateImage(ob->egl_disp,
                                                          EGL_NO_CONTEXT,
                                                          EGL_NATIVE_PIXMAP_KHR,
                                                          (void *)pm, NULL);

                 if ((ns->version < 4) ||
                     ((ns->version == 4) && !(ns->data.x11.multiple_buffer == 1)))
                   n->ns_data.x11.multiple_buffer = 0;
                 else
                   n->ns_data.x11.multiple_buffer = 1;
                 if (ob->detected.no_multi_buffer_native)
                   n->ns_data.x11.multiple_buffer = 0;

                 if (!n->ns_data.x11.surface)
                   {
                      WRN("eglCreateImage() for Pixmap %#lx failed: %#x", pm, eglGetError());
                      free(n);
                      return NULL;
                   }
                 n->ns_data.x11.config = (void *)egl_config;
                 im->native.yinvert     = yinvert;
                 im->native.loose       = 0;
                 im->native.disp        = ob->egl_disp;
                 im->native.shared      = gl_context->shared;
                 im->native.data        = n;
                 im->native.func.bind   = _native_bind_cb;
                 im->native.func.unbind = _native_unbind_cb;
                 im->native.func.free   = _native_free_cb;
                 im->native.target      = GL_TEXTURE_2D;
                 im->native.mipmap      = 0;
                 eina_hash_add(ob->gl_context->shared->native_pm_hash, &pmid, im);
                 glsym_evas_gl_common_image_native_enable(im);
             }
         }
#endif
    }
  else if (ns->type == EVAS_NATIVE_SURFACE_OPENGL)
    {
       if (native)
         {
            n = calloc(1, sizeof(Native));
            if (n)
              {
                 memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));

                 eina_hash_add(gl_context->shared->native_tex_hash, &texid, im);

                 n->ns_data.opengl.surface = 0;

                 im->native.yinvert     = 0;
                 im->native.loose       = 0;
                 im->native.disp        = ob->egl_disp;
                 im->native.shared      = gl_context->shared;
                 im->native.data        = n;
                 im->native.func.bind   = _native_bind_cb;
                 im->native.func.unbind = _native_unbind_cb;
                 im->native.func.free   = _native_free_cb;
                 im->native.target      = GL_TEXTURE_2D;
                 im->native.mipmap      = 0;

                 // FIXME: need to implement mapping sub texture regions
                 // x, y, w, h for possible texture atlasing

                 glsym_evas_gl_common_image_native_enable(im);
              }
         }
    }
  else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
    {
       if (native)
         {
            n = calloc(1, sizeof(Native));
            if (n)
              {
                 eina_hash_add(gl_context->shared->native_tbm_hash, &buffer, im);

                 memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
                 n->ns_data.tbm.buffer = buffer;
                 if (glsym_evas_gl_common_eglDestroyImage)
                   n->ns_data.tbm.surface =
                     glsym_evas_gl_common_eglCreateImage(ob->egl_disp,
                                                         EGL_NO_CONTEXT,
                                                         EGL_NATIVE_SURFACE_TIZEN,
                                                         (void *)buffer,
                                                         NULL);
                 else
                   ERR("Try eglCreateImage on EGL with no support");
                 if (!n->ns_data.tbm.surface)
                   WRN("eglCreateImage() for %p failed", buffer);
                 im->native.yinvert     = 1;
                 im->native.loose       = 0;
                 im->native.disp        = ob->egl_disp;
                 im->native.shared      = gl_context->shared;
                 im->native.data        = n;
                 im->native.func.bind   = _native_bind_cb;
                 im->native.func.unbind = _native_unbind_cb;
                 im->native.func.free   = _native_free_cb;
                 im->native.target      = GL_TEXTURE_EXTERNAL_OES;
                 im->native.mipmap      = 0;
                 glsym_evas_gl_common_image_native_enable(im);
              }
         }
    }
  else if (ns->type == EVAS_NATIVE_SURFACE_EVASGL)
    {
       if (native)
         {
            n = calloc(1, sizeof(Native));
            if (n)
              {
                 memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));

                 eina_hash_add(gl_context->shared->native_evasgl_hash, &buffer, im);

                 n->ns_data.evasgl.surface = ns->data.evasgl.surface;

                 im->native.yinvert      = 0;
                 im->native.loose        = 0;
#ifdef GL_GLES
                 im->native.disp        = ob->egl_disp;
#else
                 im->native.disp        = ob->disp;
#endif
                 im->native.shared      = gl_context->shared;
                 im->native.data         = n;
                 im->native.func.bind    = _native_bind_cb;
                 im->native.func.unbind  = _native_unbind_cb;
                 im->native.func.free    = _native_free_cb;
                 im->native.func.yinvert = _native_yinvert_cb;
                 im->native.target       = GL_TEXTURE_2D;
                 im->native.mipmap       = 0;

                 // FIXME: need to implement mapping sub texture regions
                 // x, y, w, h for possible texture atlasing

                 glsym_evas_gl_common_image_native_enable(im);
              }
         }
    }

   return im;
}

/**
 * @brief Gets the last EGL or Evas GL common error code.
 *
 * First checks the Evas GL common error status, then checks the EGL error status.
 * Resets the Evas GL common error status after checking.
 *
 * @param engine The generic engine pointer.
 * @return The combined error code (EGL errors are offset by -EGL_SUCCESS). Returns EVAS_GL_SUCCESS (0) if no error.
 */
static int
eng_gl_error_get(void *engine)
{
   int err;

   if ((err = glsym_evas_gl_common_error_get(engine)) != EVAS_GL_SUCCESS)
     goto end;

   err = eglGetError() - EGL_SUCCESS;

end:
   glsym_evas_gl_common_error_set(EVAS_GL_SUCCESS);
   return err;
}

/**
 * @brief Gets the currently active EVGL_Context, if it belongs to this engine instance.
 *
 * Retrieves the current EVGL_Context from Evas GL common and checks if its
 * underlying native EGL context matches the one returned by eglGetCurrentContext.
 *
 * @param engine The generic engine pointer (unused).
 * @return A pointer to the current EVGL_Context if it's active and belongs to this engine, NULL otherwise.
 */
static void *
eng_gl_current_context_get(void *engine EINA_UNUSED)
{
   EVGL_Context *ctx;
   EVGLNative_Context context;

   ctx = glsym_evas_gl_common_current_context_get();
   if (!ctx)
     return NULL;

   context = glsym_evgl_current_native_context_get(ctx);

   if (evas_eglGetCurrentContext() == context)
     return ctx;

   return NULL;
}

/**
 * @brief Evas module initialization function.
 *
 * Called by Evas core when loading the engine module.
 * - Inherits function table from "gl_generic" engine.
 * - Registers a log domain.
 * - Initializes partial render debug flag.
 * - Overrides specific engine functions with Win32 implementations.
 * - Temporarily sets EGL environment variables (EGL_PLATFORM=angle, EGL_LOG_LEVEL=debug) if not already set.
 * - Calls `gl_symbols` to resolve common function pointers.
 * - Restores original EGL environment variables.
 * - Sets the module's function table.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   const char *egl_platform_env = NULL;
   const char *egl_log_level_env = NULL;

   if (!em) return 0;

   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "gl_generic", sizeof (Evas_Engine_Info_GL_Win32))) return 0;

   if (_evas_engine_gl_win32_log_dom < 0)
     _evas_engine_gl_win32_log_dom = eina_log_domain_register
       ("evas-gl_win32", EVAS_DEFAULT_LOG_COLOR);

   if (_evas_engine_gl_win32_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   if (partial_render_debug == -1)
     {
        if (getenv("EVAS_GL_PARTIAL_DEBUG")) partial_render_debug = 1;
        else partial_render_debug = 0;
     }

   /* store it for later use */
   func = pfunc;
   /* now to override methods */
   #define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(output_info_setup);
   ORD(output_setup);
   ORD(output_update);
   ORD(canvas_alpha_get);
   ORD(output_free);
   ORD(output_dump);

   ORD(image_native_init);
   ORD(image_native_shutdown);
   ORD(image_native_set);

   ORD(gl_error_get);
   // gl_current_surface_get is in gl generic
   ORD(gl_current_context_get);

   /* FIXME: should be set to what on Windows ? */
   if (!(egl_platform_env = getenv("EGL_PLATFORM")))
     SetEnvironmentVariable("EGL_PLATFORM", "angle");

   /*
    * FIXME: set to something else when release, but used in Angle
    * possible values: debug, info, warning, and fatal
    */
   if (!(egl_log_level_env = getenv("EGL_LOG_LEVEL")))
     SetEnvironmentVariable("EGL_LOG_LEVEL", "debug");

   gl_symbols();

   if (!egl_log_level_env)
     SetEnvironmentVariable("EGL_LOG_LEVEL", NULL);

   if (!egl_platform_env)
     SetEnvironmentVariable("EGL_PLATFORM", NULL);

   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

/**
 * @brief Evas module shutdown function.
 *
 * Called by Evas core when unloading the engine module.
 * - Unregisters the log domain.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_gl_win32_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_gl_win32_log_dom);
        _evas_engine_gl_win32_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "gl_win32",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, gl_win32);

#ifndef EVAS_STATIC_BUILD_GL_WIN32
EVAS_EINA_MODULE_DEFINE(engine, gl_win32);
#endif
