
#include "evas_gl_api_ext.h"

/* dlsym */
#ifdef _WIN32
# include <evil_private.h>
#else
# include <dlfcn.h>
#endif

#define EVGL_FUNC_BEGIN() if (UNLIKELY(_need_context_restore)) _context_restore()

// list of egl extensions
#ifdef GL_GLES
static char *_egl_ext_string = NULL;
#endif
// list of gles 2.0 exts by official name
static char *_gl_ext_string = NULL;
// list of exts by official name only like "GL_EXT_discard_framebuffer GL_EXT_multi_draw_arrays"
static char *_gl_ext_string_official = NULL;
// list of gles 1.1 exts by official name
static char *_gles1_ext_string = NULL;
static char *_gles1_ext_string_official = NULL;
// list of gles 3.1 exts by official name
static char *_gles3_ext_string = NULL;
static char *_gles3_ext_string_official = NULL;
// indexed pointer list of each extension of gles 3
Eina_Array *_gles3_ext_plist = NULL;

typedef void (*_getproc_fn) (void);
typedef _getproc_fn (*fp_getproc)(const char *);

#ifndef EGL_NATIVE_PIXMAP_KHR
# define EGL_NATIVE_PIXMAP_KHR 0x30b0
#endif

#ifndef EGL_WL_bind_wayland_display
struct wl_display;
struct wl_resource;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Extension HEADER
/////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name)
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
   ret (*egl_ext_sym_##name) param1 = NULL; \
   ret (*gl_ext_sym_##name) param1 = NULL; \
   ret (*gles1_ext_sym_##name) param1 = NULL; \
   ret (*gles3_ext_sym_##name) param1 = NULL;
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
/////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Extension HEADER
/////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name) \
   int _egl_ext_support_##name = 0; \
   int _gl_ext_support_##name = 0; \
   int _gles1_ext_support_##name = 0; \
   int _gles3_ext_support_##name = 0;
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   int _egl_ext_support_func_##name = 0; \
   int _gl_ext_support_func_##name = 0; \
   int _gles1_ext_support_func_##name = 0; \
   int _gles3_ext_support_func_##name = 0;
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define EVGL_FUNC_BEGIN() if (UNLIKELY(_need_context_restore)) _context_restore()

// Evas extensions from EGL extensions
#ifdef GL_GLES
#define EGLDISPLAY_GET(a) _evgl_egl_display_get(__func__, a)

// this struct defines an EvasGLImage when using EGL
typedef struct _EvasGLImage {
   EGLDisplay  dpy;
   EGLImageKHR img;
} EvasGLImage_EGL;

/**
 * @brief Retrieves the EGLDisplay for the current context or a given Evas_GL instance.
 *
 * This function is a helper to get the EGLDisplay. It first tries to get it from
 * thread-local storage (current context). If that fails, and an `evgl` instance
 * is provided, it falls back to getting the display from the engine data associated
 * with that instance. This is crucial for EGL operations that require a display handle.
 *
 * @param function The name of the calling function, for logging purposes.
 * @param evgl A specific Evas_GL instance (optional, used as a fallback).
 * @return The EGLDisplay handle, or EGL_NO_DISPLAY on failure.
 */
static EGLDisplay
_evgl_egl_display_get(const char *function, Evas_GL *evgl)
{
   EGLDisplay dpy = EGL_NO_DISPLAY;
   EVGL_Resource *rsc;

   if (!evgl_engine || !evgl_engine->funcs || !evgl_engine->funcs->display_get)
     {
        ERR("%s: Invalid Engine... (Can't acccess EGL Display)\n", function);
        evas_gl_common_error_set(EVAS_GL_BAD_DISPLAY);
        return EGL_NO_DISPLAY;
     }

   if (!(rsc=_evgl_tls_resource_get()))
     {
        if (evgl) goto fallback;
        ERR("%s: Unable to execute GL command. Error retrieving tls", function);
        evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return EGL_NO_DISPLAY;
     }

   if (!rsc->current_eng)
     {
        if (evgl) goto fallback;
        ERR("%s: no current engine set; ensure you've called evas_gl_make_current()", function);
        evas_gl_common_error_set(EVAS_GL_NOT_INITIALIZED);
        return EGL_NO_DISPLAY;
     }

   dpy = (EGLDisplay) evgl_engine->funcs->display_get(rsc->current_eng);
   return dpy;

fallback:
   dpy = (EGLDisplay) evgl_engine->funcs->display_get(_evgl_engine_data_get(evgl));
   return dpy;
}

/**
 * @brief A wrapper around eglCreateImageKHR that handles EvasGLImage creation.
 *
 * This function takes Evas-specific arguments, translates them for EGL,
 * calls the real `eglCreateImage` function, and wraps the resulting
 * EGLImageKHR in an `EvasGLImage_EGL` struct. It also handles the
 * conversion of the attribute list from a 0-terminated list to an
 * EGL_NONE-terminated list.
 *
 * @param dpy The EGLDisplay.
 * @param ctx The EGLContext. Can be EGL_NO_CONTEXT for some targets.
 * @param target The EGL image target type (e.g., EVAS_GL_TEXTURE_2D).
 * @param buffer The source buffer for the image (e.g., a native pixmap).
 * @param attrib_list A 0-terminated list of attributes.
 * @return A pointer to an `EvasGLImage_EGL` struct on success, or NULL on failure.
 */
static void *
_evgl_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx,
                        int target, void* buffer, const int *attrib_list)
{
   EvasGLImage_EGL *img;
   int *attribs = NULL;
   void *eglimg;

   /* Convert 0 terminator into a EGL_NONE terminator */
   if (attrib_list)
     {
        int cnt = 0;
        int *a;

        for (a = (int *) attrib_list; (*a) && (*a != EGL_NONE); a += 2)
          {
             /* TODO: Verify supported attributes */
             cnt += 2;
          }

        attribs = alloca(sizeof(int) * (cnt + 1));
        for (a = attribs; (*attrib_list) && (*attrib_list != EGL_NONE);
             a += 2, attrib_list += 2)
          {
             a[0] = attrib_list[0];
             a[1] = attrib_list[1];
          }
        *a = EGL_NONE;
     }

   eglimg = EXT_FUNC_EGL(eglCreateImage)(dpy, ctx, target, buffer, attribs);
   if (!eglimg) return NULL;

   img = calloc(1, sizeof(EvasGLImage_EGL));
   img->dpy = dpy;
   img->img = eglimg;
   return img;
}

/**
 * @brief Creates an EvasGLImage from the current context.
 *
 * This function is the public-facing API implementation for creating an EvasGLImage.
 * It automatically retrieves the current EGLDisplay. If the target is a 2D texture,
 * it also retrieves the current EGLContext, as this is required by the EGL spec.
 *
 * @param target The image target type (e.g., EVAS_GL_TEXTURE_2D).
 * @param buffer The source buffer.
 * @param attrib_list A 0-terminated list of attributes.
 * @return An opaque EvasGLImage handle, or NULL on failure.
 */
static void *
_evgl_evasglCreateImage(int target, void* buffer, const int *attrib_list)
{
   EGLDisplay dpy = EGLDISPLAY_GET(NULL);
   EGLContext ctx = EGL_NO_CONTEXT;

   if (!dpy)
     {
        WRN("No display found, use evasglCreateImageForContext instead.");
        return NULL;
     }

   /* EGL_NO_CONTEXT will always fail for TEXTURE_2D */
   if (target == EVAS_GL_TEXTURE_2D)
     {
        ctx = eglGetCurrentContext();
        DBG("Creating EGL image based on the current context: %p", ctx);
     }

   return _evgl_eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

/**
 * @brief Creates an EvasGLImage for a specific Evas_GL context.
 *
 * This is an alternative to `_evgl_evasglCreateImage` that allows specifying
 * the exact Evas_GL instance and context to use, rather than relying on the
 * current thread's context.
 *
 * @param evasgl The Evas_GL instance.
 * @param evasctx The Evas_GL_Context.
 * @param target The image target type.
 * @param buffer The source buffer.
 * @param attrib_list A 0-terminated list of attributes.
 * @return An opaque EvasGLImage handle, or NULL on failure.
 */
static void *
_evgl_evasglCreateImageForContext(Evas_GL *evasgl, Evas_GL_Context *evasctx,
                                 int target, void* buffer, const int *attrib_list)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evasgl);
   EGLContext ctx = EGL_NO_CONTEXT;

   if (!dpy || !evasgl)
     {
        ERR("Evas_GL can not be NULL here.");
        evas_gl_common_error_set(EVAS_GL_BAD_DISPLAY);
        return NULL;
     }

   ctx = _evgl_native_context_get(evasctx);
   return _evgl_eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
}

/**
 * @brief Destroys an EvasGLImage.
 *
 * This function frees the resources associated with an EvasGLImage. It calls
 * `eglDestroyImage` and then frees the `EvasGLImage_EGL` wrapper struct.
 *
 * @param image The EvasGLImage handle to destroy.
 */
static void
_evgl_evasglDestroyImage(EvasGLImage image)
{
   EvasGLImage_EGL *img = image;

   if (!img)
     {
        ERR("EvasGLImage is NULL.");
        evas_gl_common_error_set(EVAS_GL_BAD_PARAMETER);
        return;
     }

   EXT_FUNC_EGL(eglDestroyImage)(img->dpy, img->img);
   free(img);
}

/**
 * @brief Binds an EvasGLImage to a 2D texture target.
 *
 * This is a wrapper around `glEGLImageTargetTexture2DOES`. It retrieves the
 * underlying EGLImageKHR from the EvasGLImage handle and passes it to the
 * driver function. It ensures that the current context is valid before
 * proceeding.
 *
 * @param target The texture target (e.g., GL_TEXTURE_2D).
 * @param image The EvasGLImage to bind.
 */
static void
_evgl_glEvasGLImageTargetTexture2D(GLenum target, EvasGLImage image)
{
   EvasGLImage_EGL *img = image;
   EVGL_Resource *rsc;
   EVGL_Context *ctx;

   if (!(rsc=_evgl_tls_resource_get()))
     {
        ERR("Unable to execute GL command. Error retrieving tls");
        return;
     }

   if (!rsc->current_eng)
     {
        ERR("Unable to retrieve Current Engine");
        return;
     }

   ctx = rsc->current_ctx;
   if (!ctx)
     {
        ERR("Unable to retrieve Current Context");
        return;
     }

  if (!img)
    {
       ERR("EvasGLImage is NULL");
       EXT_FUNC(glEGLImageTargetTexture2DOES)(target, NULL);
       return;
    }

   EXT_FUNC(glEGLImageTargetTexture2DOES)(target, img->img);
}

/**
 * @brief Binds an EvasGLImage to a renderbuffer storage target.
 *
 * This is a wrapper around `glEGLImageTargetRenderbufferStorageOES`. It behaves
 * similarly to `_evgl_glEvasGLImageTargetTexture2D` but for renderbuffers.
 *
 * @param target The renderbuffer target (must be GL_RENDERBUFFER).
 * @param image The EvasGLImage to bind.
 */
static void
_evgl_glEvasGLImageTargetRenderbufferStorage(GLenum target, EvasGLImage image)
{
   EvasGLImage_EGL *img = image;
   EVGL_Resource *rsc;
   EVGL_Context *ctx;

   if (!(rsc=_evgl_tls_resource_get()))
     {
        ERR("Unable to execute GL command. Error retrieving tls");
        return;
     }

   if (!rsc->current_eng)
     {
        ERR("Unable to retrieve Current Engine");
        return;
     }

   ctx = rsc->current_ctx;
   if (!ctx)
     {
        ERR("Unable to retrieve Current Context");
        return;
     }

  if (!img)
    {
       ERR("EvasGLImage is NULL");
       EXT_FUNC(glEGLImageTargetRenderbufferStorageOES)(target, NULL);
       return;
    }

   EXT_FUNC(glEGLImageTargetRenderbufferStorageOES)(target, img->img);
}

/**
 * @brief Creates a sync object for a given Evas_GL instance.
 *
 * This is a wrapper for `eglCreateSyncKHR`. It retrieves the EGLDisplay
 * associated with the Evas_GL instance and creates a fence sync object.
 *
 * @param evas_gl The Evas_GL instance.
 * @param type The type of sync object to create (e.g., EGL_SYNC_FENCE_KHR).
 * @param attrib_list A list of attributes for the sync object.
 * @return An `EvasGLSync` handle on success, NULL on failure.
 */
static EvasGLSync
_evgl_evasglCreateSync(Evas_GL *evas_gl,
                      unsigned int type, const int *attrib_list)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return NULL;
   return EXT_FUNC_EGL(eglCreateSyncKHR)(dpy, type, attrib_list);
}

/**
 * @brief Destroys a sync object.
 *
 * Wrapper for `eglDestroySyncKHR`.
 *
 * @param evas_gl The Evas_GL instance.
 * @param sync The sync object to destroy.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglDestroySync(Evas_GL *evas_gl, EvasGLSync sync)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglDestroySyncKHR)(dpy, sync);
}

/**
 * @brief Blocks the client until a sync object is signaled.
 *
 * Wrapper for `eglClientWaitSyncKHR`. This is a client-side wait.
 *
 * @param evas_gl The Evas_GL instance.
 * @param sync The sync object to wait on.
 * @param flags Flags controlling the wait behavior (e.g., EGL_SYNC_FLUSH_COMMANDS_BIT_KHR).
 * @param timeout The timeout value in nanoseconds.
 * @return The wait status (e.g., EGL_CONDITION_SATISFIED_KHR).
 */
static int
_evgl_evasglClientWaitSync(Evas_GL *evas_gl,
                          EvasGLSync sync, int flags, EvasGLTime timeout)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglClientWaitSyncKHR)(dpy, sync, flags, timeout);
}

/**
 * @brief Manually signals or unsignals a reusable sync object.
 *
 * Wrapper for `eglSignalSyncKHR`.
 *
 * @param evas_gl The Evas_GL instance.
 * @param sync The reusable sync object.
 * @param mode The new state (EGL_SIGNALED_KHR or EGL_UNSIGNALED_KHR).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglSignalSync(Evas_GL *evas_gl,
                      EvasGLSync sync, unsigned mode)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglSignalSyncKHR)(dpy, sync, mode);
}

/**
 * @brief Retrieves an attribute of a sync object.
 *
 * Wrapper for `eglGetSyncAttribKHR`.
 *
 * @param evas_gl The Evas_GL instance.
 * @param sync The sync object.
 * @param attribute The attribute to query (e.g., EGL_SYNC_STATUS_KHR).
 * @param value A pointer to store the result.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglGetSyncAttrib(Evas_GL *evas_gl,
                         EvasGLSync sync, int attribute, int *value)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglGetSyncAttribKHR)(dpy, sync, attribute, value);
}

/**
 * @brief Blocks the server (GPU) until a sync object is signaled.
 *
 * Wrapper for `eglWaitSyncKHR`. This is a server-side wait. The `flags` parameter
 * is currently unused in the EGL specification and must be 0.
 *
 * @param evas_gl The Evas_GL instance.
 * @param sync The sync object to wait on.
 * @param flags Must be 0.
 * @return EGL_TRUE on success, EGL_FALSE on failure.
 */
static int
_evgl_evasglWaitSync(Evas_GL *evas_gl,
                    EvasGLSync sync, int flags)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglWaitSyncKHR)(dpy, sync, flags);
}

/**
 * @brief Binds a Wayland display to an EGL display.
 *
 * Wrapper for `eglBindWaylandDisplayWL`. This allows Evas GL to work
 * with Wayland surfaces.
 *
 * @param evas_gl The Evas_GL instance.
 * @param wl_display A pointer to the `wl_display` object.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglBindWaylandDisplay(Evas_GL *evas_gl,
                              void *wl_display)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglBindWaylandDisplayWL)(dpy, wl_display);
}

/**
 * @brief Unbinds a Wayland display from an EGL display.
 *
 * Wrapper for `eglUnbindWaylandDisplayWL`.
 *
 * @param evas_gl The Evas_GL instance.
 * @param wl_display A pointer to the `wl_display` object.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglUnbindWaylandDisplay(Evas_GL *evas_gl,
                                void *wl_display)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglUnbindWaylandDisplayWL)(dpy, wl_display);
}

/**
 * @brief Queries attributes of a Wayland buffer.
 *
 * Wrapper for `eglQueryWaylandBufferWL`. This can be used to get information
 * like texture format or dimensions from a `wl_buffer`.
 *
 * @param evas_gl The Evas_GL instance.
 * @param buffer The `wl_resource` for the buffer.
 * @param attribute The attribute to query (e.g., EGL_TEXTURE_FORMAT).
 * @param value A pointer to store the result.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglQueryWaylandBuffer(Evas_GL *evas_gl,
                              void *buffer, int attribute, int *value)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglQueryWaylandBufferWL)(dpy, buffer, attribute, value);
}

/**
 * @brief Queries the DMA-BUF formats supported by the EGL implementation.
 *
 * Wrapper for `eglQueryDmaBufFormatsEXT`. This is used for creating EGLImages
 * from DMA-BUF file descriptors.
 *
 * @param evas_gl The Evas_GL instance.
 * @param max_formats The maximum number of formats `formats` can hold.
 * @param formats An array to store the supported FOURCC format codes.
 * @param num_formats A pointer to store the number of formats returned.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglQueryDmaBufFormats(Evas_GL *evas_gl,
                               int max_formats, int *formats, int *num_formats)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglQueryDmaBufFormatsEXT)(dpy, max_formats, formats, num_formats);
}

/**
 * @brief Queries the DMA-BUF modifiers supported for a given format.
 *
 * Wrapper for `eglQueryDmaBufModifiersEXT`. Modifiers describe memory layout
 * properties like tiling.
 *
 * @param evas_gl The Evas_GL instance.
 * @param format The FOURCC format code.
 * @param max_modifiers The maximum number of modifiers `modifiers` can hold.
 * @param modifiers An array to store the supported modifier codes.
 * @param external_only If not NULL, will be set to EINA_TRUE if only external
 *                      consumers can use the modifiers.
 * @param num_modifiers A pointer to store the number of modifiers returned.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evgl_evasglQueryDmaBufModifiers(Evas_GL *evas_gl,
                                 int format, int max_modifiers, uint64_t *modifiers, Eina_Bool *external_only, int *num_modifiers)
{
   EGLDisplay dpy = EGLDISPLAY_GET(evas_gl);
   if (!dpy) return EINA_FALSE;
   return EXT_FUNC_EGL(eglQueryDmaBufModifiersEXT)(dpy, format, max_modifiers, modifiers, external_only, num_modifiers);
}

#else
#endif

/**
 * @brief Discards the contents of framebuffer attachments.
 *
 * This is a wrapper for `glDiscardFramebufferEXT`. Its main purpose is to
 * handle a translation of Evas-specific attachment enums (`GL_COLOR_EXT`, etc.)
 * to standard OpenGL `GL_COLOR_ATTACHMENT0`, etc. when not in direct rendering
 * mode and the default FBO is being used. This allows Evas code to be a bit
 * more generic.
 *
 * @param target The framebuffer target (e.g., GL_FRAMEBUFFER).
 * @param numAttachments The number of attachments in the `attachments` array.
 * @param attachments An array of enums specifying the attachments to discard.
 */
static void
_evgl_glDiscardFramebufferEXT(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
   EVGL_Resource *rsc;
   EVGL_Context *ctx;
   Eina_Bool target_is_fbo = EINA_FALSE;

   if (!(rsc=_evgl_tls_resource_get()))
     {
        ERR("Unable to execute GL command. Error retrieving tls");
        return;
     }

   if (!rsc->current_eng)
     {
        ERR("Unable to retrieve Current Engine");
        return;
     }

   ctx = rsc->current_ctx;
   if (!ctx)
     {
        ERR("Unable to retrieve Current Context");
        return;
     }

   if (!_evgl_direct_enabled())
     {
        if (ctx->current_fbo == 0)
          target_is_fbo = EINA_TRUE;
     }

   if (target_is_fbo && numAttachments)
     {
        GLenum *att;
        int i = 0;
        att = (GLenum *)calloc(1, numAttachments * sizeof(GLenum));
        if (!att)
          return;

        memcpy(att, attachments, numAttachments * sizeof(GLenum));
        while (i < numAttachments)
          {
             if (att[i] == GL_COLOR_EXT)
               att[i] = GL_COLOR_ATTACHMENT0;
             else if (att[i] == GL_DEPTH_EXT)
               att[i] = GL_DEPTH_ATTACHMENT;
             else if (att[i] == GL_STENCIL_EXT)
               att[i] = GL_STENCIL_ATTACHMENT;
             i++;
          }
        EXT_FUNC(glDiscardFramebuffer)(target, numAttachments, att);
        free(att);
     }
   else
     {
        EXT_FUNC(glDiscardFramebuffer)(target, numAttachments, attachments);
     }
}

//2.0 ext bodies
/**
 * @brief This set of macros defines static wrapper functions for GLES2/GL
 * extension functions.
 *
 * For each function in `evas_gl_api_ext_def.h` enabled for GLES2/GL, a static
 * function `evgl_name` is created. This wrapper calls the actual extension
 * function pointer (e.g., `gl_ext_sym_name`) after performing a context restore
 * check. These wrappers are then assigned to the `Evas_GL_API` structure.
 */
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name)
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
    static ret evgl_##name param1 { EVGL_FUNC_BEGIN(); return EXT_FUNC(name) param2; }
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

//1.1 ext bodies
/**
 * @brief This set of macros defines static wrapper functions for GLES1
 * extension functions.
 *
 * For each function in `evas_gl_api_ext_def.h` enabled for GLES1, a static
 * function `evgl_gles1_name` is created. This wrapper calls the actual extension
 * function pointer (e.g., `gles1_ext_sym_name`) after a context restore check.
 * These wrappers are then assigned to the `Evas_GL_API` structure for GLES1.
 */
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name)
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
    static ret evgl_gles1_##name param1 { EVGL_FUNC_BEGIN(); return EXT_FUNC_GLES1(name) param2; }
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

//3.X ext bodies
/**
 * @brief This set of macros defines static wrapper functions for GLES3
 * extension functions.
 *
 * For each function in `evas_gl_api_ext_def.h` enabled for GLES3, a static
 * function `evgl_gles3_name` is created. This wrapper calls the actual extension
 * function pointer (e.g., `gles3_ext_sym_name`) after a context restore check.
 * These wrappers are then assigned to the `Evas_GL_API` structure for GLES3.
 */
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name)
#define _EVASGL_EXT_END()
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
    static ret evgl_gles3_##name param1 { EVGL_FUNC_BEGIN(); return EXT_FUNC_GLES3(name) param2; }
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

/**
 * @brief Bitmask tracking the initialization status of different extension APIs.
 *
 * This variable uses a bitfield to track which API extensions (EGL, GLES1,
 * GLES2, GLES3) have been initialized. This prevents redundant initialization.
 * The individual bits are defined by the `EVASGL_API_*_EXT_INITIALIZED` macros.
 */
static int _evgl_api_ext_status = 0;
#define EVASGL_API_GLES2_EXT_INITIALIZED 0x1
#define EVASGL_API_GLES1_EXT_INITIALIZED 0x2
#define EVASGL_API_GLES3_EXT_INITIALIZED 0x4
#define EVASGL_API_EGL_EXT_INITIALIZED   0x8

#ifdef GL_GLES
Eina_Bool
evgl_api_egl_ext_init(void *getproc, const char *glueexts)
{
   fp_getproc gp = (fp_getproc)getproc;
   int _curext_supported = 0;
   Eina_Strbuf *sb = NULL;

   if (_evgl_api_ext_status & EVASGL_API_EGL_EXT_INITIALIZED)
     return EINA_TRUE;

   sb = eina_strbuf_new();

   // Always supported by Evas GL (faked with internal pbuffer if needed)
   // See also GL_OES_surfaceless_context (needs some more work to be actually
   // supported).
   eina_strbuf_append(sb, "EGL_KHR_surfaceless_context ");

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define GETPROCADDR(sym) \
   (((!(*drvfunc)) && (gp)) ? (__typeof__((*drvfunc)))gp(sym) : (__typeof__((*drvfunc)))dlsym(RTLD_DEFAULT, sym))

#define _EVASGL_EXT_BEGIN(name) \
     { \
        int *ext_support = &_egl_ext_support_##name; \
        *ext_support = 0;

#define _EVASGL_EXT_END() \
     }

#define _EVASGL_EXT_CHECK_SUPPORT(name) \
   (strstr(glueexts, name) != NULL)

#define _EVASGL_EXT_DISCARD_SUPPORT() \
   *ext_support = 0;

#define _EVASGL_EXT_DRVNAME(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) *ext_support = 1;

#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) { *ext_support = 1; _egl_ext_support_func_##name = 1; }

#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname) \
   if (_EVASGL_EXT_CHECK_SUPPORT(deskname)) *ext_support = 1;

#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
     { \
        ret (**drvfunc)param1 = &egl_ext_sym_##name; \
        if (*ext_support == 1) \
          {

#define _EVASGL_EXT_FUNCTION_END() \
          } \
        if ((*drvfunc) == NULL) _EVASGL_EXT_DISCARD_SUPPORT(); \
     }
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name) \
   if ((*drvfunc) == NULL) *drvfunc = name;

// This adds all the function names to the "safe" list but only one pointer
// will be stored in the hash table.
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name) \
   if ((*drvfunc) == NULL) \
     { \
        *drvfunc = GETPROCADDR(name); \
        evgl_safe_extension_add(name, (void *) (*drvfunc)); \
     } \
   else evgl_safe_extension_add(name, NULL);

#ifdef _EVASGL_EXT_FUNCTION_WHITELIST
# undef _EVASGL_EXT_FUNCTION_WHITELIST
#endif
#define _EVASGL_EXT_FUNCTION_WHITELIST(name) evgl_safe_extension_add(name, NULL);

#define _EVASGL_EXT_ENABLE_GL_GLES 0
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_FUNCTION_WHITELIST
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

#undef GETPROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_BEGIN(name) \
     if (_egl_ext_support_##name != 0) \
       { \
          eina_strbuf_append(sb, #name" "); \
          _curext_supported = 1; \
       } \
     else _curext_supported = 0;

#define _EVASGL_EXT_END()
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_DRVNAME_PRINT(name) \
       { \
          if ((strncmp(name, "EGL_", 4) == 0) && (strstr(eina_strbuf_string_get(sb), name) == NULL)) \
            eina_strbuf_append(sb, name" "); \
       }
#define _EVASGL_EXT_DRVNAME(name) \
     if (_curext_supported) \
       _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
     if (_curext_supported && _egl_ext_support_func_##name) \
       _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#define _EVASGL_EXT_ENABLE_GL_GLES 0
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME_PRINT
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   if (_egl_ext_string) free(_egl_ext_string);
   _egl_ext_string = eina_strbuf_string_steal(sb);
   eina_strbuf_free(sb);

  INF("EGL extension string: %s", _egl_ext_string);

  _evgl_api_ext_status |= EVASGL_API_EGL_EXT_INITIALIZED;
   return EINA_TRUE;
}
#endif

/**
 * @brief Initializes GLES2 extension support.
 *
 * This function is called once to query and load all supported GLES2 and
 * desktop GL extensions. It populates the internal extension support flags
 * and function pointers. It uses the X-macro pattern with `evas_gl_api_ext_def.h`.
 *
 * @param getproc A function pointer to `eglGetProcAddress` or similar.
 * @param glueexts A space-separated string of additional extensions provided
 *                 by the Evas GL glue layer, which may not be reported by the
 *                 driver (e.g., for emulated extensions).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_evgl_api_gles2_ext_init(void *getproc, const char *glueexts)
{
   const char *glexts;
   fp_getproc gp = (fp_getproc)getproc;
   int _curext_supported = 0;
   Eina_Strbuf *sb = eina_strbuf_new();
   Eina_Strbuf *sboff = eina_strbuf_new();

#ifndef GL_GLES
   /* Add some extension strings that are always working on desktop GL */
   static const char *desktop_exts =
         "GL_EXT_read_format_bgra "
         "GL_EXT_texture_format_BGRA8888 "
         "GL_EXT_texture_type_2_10_10_10_REV ";
   eina_strbuf_append(sb, desktop_exts);
   eina_strbuf_append(sboff, desktop_exts);
#endif

   // GLES Extensions
   glexts = (const char*)glGetString(GL_EXTENSIONS);
   if (!glexts)
     {
        ERR("glGetString returned NULL! Something is very wrong...");
        eina_strbuf_free(sb);
        eina_strbuf_free(sboff);
        return EINA_FALSE;
     }

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define GETPROCADDR(sym) \
   (((!(*drvfunc)) && (gp)) ? (__typeof__((*drvfunc)))gp(sym) : (__typeof__((*drvfunc)))dlsym(RTLD_DEFAULT, sym))

#define _EVASGL_EXT_BEGIN(name) \
     { \
        int *ext_support = &_gl_ext_support_##name; \
        *ext_support = 0;

#define _EVASGL_EXT_END() \
     }

#define _EVASGL_EXT_CHECK_SUPPORT(name) \
   (strstr(glexts, name) != NULL || strstr(glueexts, name) != NULL)

#define _EVASGL_EXT_DISCARD_SUPPORT() \
   *ext_support = 0;

#define _EVASGL_EXT_DRVNAME(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) *ext_support = 1;

#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) { *ext_support = 1; _gl_ext_support_func_##name = 1; }

#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname) \
   if (_EVASGL_EXT_CHECK_SUPPORT(deskname)) *ext_support = 1;

#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
     { \
        ret (**drvfunc)param1 = &gl_ext_sym_##name; \
        if (*ext_support == 1) \
          {

#define _EVASGL_EXT_FUNCTION_END() \
          } \
        if ((*drvfunc) == NULL) _EVASGL_EXT_DISCARD_SUPPORT(); \
     }
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name) \
   if ((*drvfunc) == NULL) *drvfunc = name;

// This adds all the function names to the "safe" list but only one pointer
// will be stored in the hash table.
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name) \
   if ((*drvfunc) == NULL) \
     { \
        *drvfunc = GETPROCADDR(name); \
        evgl_safe_extension_add(name, (void *) (*drvfunc)); \
     } \
   else evgl_safe_extension_add(name, NULL);

#ifdef _EVASGL_EXT_FUNCTION_WHITELIST
# undef _EVASGL_EXT_FUNCTION_WHITELIST
#endif
#define _EVASGL_EXT_FUNCTION_WHITELIST(name) evgl_safe_extension_add(name, NULL);

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_FUNCTION_WHITELIST
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

#undef GETPROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_BEGIN(name) \
     if (_gl_ext_support_##name != 0) \
       { \
          eina_strbuf_append(sb, #name" "); \
          _curext_supported = 1; \
       } \
     else _curext_supported = 0;

#define _EVASGL_EXT_END()
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_DRVNAME_PRINT(name) \
       { \
          eina_strbuf_append(sb, name" "); \
          if ((strncmp(name, "GL_", 3) == 0) && (strstr(eina_strbuf_string_get(sboff), name) == NULL)) \
            eina_strbuf_append(sboff, name" "); \
       }
#define _EVASGL_EXT_DRVNAME(name) \
     if (_curext_supported) \
       _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
     if (_curext_supported && _gl_ext_support_func_##name) \
       _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)
#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME_PRINT
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   if (_gl_ext_string) free(_gl_ext_string);
   if (_gl_ext_string_official) free(_gl_ext_string_official);
   _gl_ext_string = eina_strbuf_string_steal(sb);
   _gl_ext_string_official = eina_strbuf_string_steal(sboff);
   eina_strbuf_free(sb);
   eina_strbuf_free(sboff);

  _evgl_api_ext_status |= EVASGL_API_GLES2_EXT_INITIALIZED;
   return EINA_TRUE;
}

void
evgl_api_gles2_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts)
{
   if (!(_evgl_api_ext_status & EVASGL_API_GLES2_EXT_INITIALIZED))
     {
        DBG("Initializing GLESv2 extensions...");
        if (!_evgl_api_gles2_ext_init(getproc, glueexts))
          {
             ERR("GLESv2 extensions initialization failed");
             return;
          }
     }
#define ORD(f) EVAS_API_OVERRIDE(f, gl_funcs, evgl_)

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name) \
   if (_gl_ext_support_##name != 0) \
     {
#define _EVASGL_EXT_END() \
     }
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
   ORD(name);
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#undef _EVASGL_EXT_WHITELIST_ONLY
#define _EVASGL_EXT_WHITELIST_ONLY 0

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_WHITELIST_ONLY
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#undef ORD

}

/**
 * @brief Initializes GLES1 extension support.
 *
 * This function is called once to query and load all supported GLES1.1
 * extensions. It populates the internal extension support flags and function
 * pointers for GLES1. It uses the X-macro pattern with `evas_gl_api_ext_def.h`.
 * A GLES1 context must be current when this is called.
 *
 * @param getproc A function pointer to `eglGetProcAddress` or similar.
 * @param glueexts A space-separated string of additional extensions.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_evgl_api_gles1_ext_init(void *getproc, const char *glueexts)
{
   const char *glexts;
   fp_getproc gp = (fp_getproc)getproc;
   int _curext_supported = 0;
   Evas_GL_API *gles1_funcs;
   Eina_Strbuf *sb = eina_strbuf_new();
   Eina_Strbuf *sboff = eina_strbuf_new();

#ifdef GL_GLES
   EVGL_Resource *rsc;
   EGLint context_version;
   EGLDisplay dpy = EGLDISPLAY_GET(NULL);

   /* glGetString returns the information for the currently bound context
    * So, update glexts only if GLES1 context is currently bound.
    * Check here if GLESv1 is current
    */
   if (!(rsc=_evgl_tls_resource_get()))
     {
        ERR("Unable to initialize GLES1 extensions. Error retrieving tls");
        goto error;
     }

   if ((dpy == EGL_NO_DISPLAY) || !rsc->current_ctx)
     {
        DBG("Unable to initialize GLES1 extensions. Engine not initialized");
        goto error;
     }

   if (!eglQueryContext(dpy, rsc->current_ctx->context, EGL_CONTEXT_CLIENT_VERSION, &context_version))
     {
        ERR("Unable to initialize GLES1 extensions. eglQueryContext failed 0x%x", eglGetError());
        goto error;
     }

   if (context_version != EVAS_GL_GLES_1_X)
     {
        DBG("GLESv1 context not bound");
        goto error;
     }
#endif

   gles1_funcs = _evgl_api_gles1_internal_get();
   if (!gles1_funcs || !gles1_funcs->glGetString)
     {
        ERR("Could not get address of glGetString in GLESv1 library!");
        goto error;
     }

   glexts = (const char *) gles1_funcs->glGetString(GL_EXTENSIONS);
   if (!glexts)
     {
        ERR("GLESv1:glGetString(GL_EXTENSIONS) returned NULL!");
        goto error;
     }

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Scanning supported extensions, sets the variables
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   // Preparing all the magic macros
#define GETPROCADDR(sym) \
   (((!(*drvfunc)) && (gp)) ? (__typeof__((*drvfunc)))gp(sym) : (__typeof__((*drvfunc)))dlsym(RTLD_DEFAULT, sym))

#define _EVASGL_EXT_BEGIN(name) \
   { \
      int *ext_support = &_gles1_ext_support_##name; \
      *ext_support = 0;

#define _EVASGL_EXT_END() \
   }

#define _EVASGL_EXT_CHECK_SUPPORT(name) \
   ((strstr(glexts, name) != NULL) || (strstr(glueexts, name) != NULL))

#define _EVASGL_EXT_DISCARD_SUPPORT() \
   *ext_support = 0;

#define _EVASGL_EXT_DRVNAME(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) *ext_support = 1;

#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) { *ext_support = 1; _gles1_ext_support_func_##name = 1; }
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname) \
   if (_EVASGL_EXT_CHECK_SUPPORT(deskname)) *ext_support = 1;

#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
     { \
        ret (**drvfunc)param1 = &gles1_ext_sym_##name; \
        if (*ext_support == 1) \
          {

#define _EVASGL_EXT_FUNCTION_END() \
          } \
        if ((*drvfunc) == NULL) _EVASGL_EXT_DISCARD_SUPPORT(); \
     }
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN() \
   if (EINA_FALSE) \
     {
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END() \
     }
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name) \
   if ((*drvfunc) == NULL) *drvfunc = name;

#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name) \
   if ((*drvfunc) == NULL) \
     { \
        *drvfunc = GETPROCADDR(name); \
        evgl_safe_extension_add(name, (void *) (*drvfunc)); \
     } \
   else evgl_safe_extension_add(name, NULL);

#ifdef _EVASGL_EXT_FUNCTION_WHITELIST
# undef _EVASGL_EXT_FUNCTION_WHITELIST
#endif
#define _EVASGL_EXT_FUNCTION_WHITELIST(name) evgl_safe_extension_add(name, NULL);

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1
#define _EVASGL_EXT_GLES1_ONLY 1

   // Okay, now we are ready to scan.
#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_GLES1_ONLY
#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_FUNCTION_WHITELIST
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
#undef GETPROCADDR

#define _EVASGL_EXT_BEGIN(name) \
     if (_gles1_ext_support_##name != 0) \
       { \
          eina_strbuf_append(sb, #name" "); \
          _curext_supported = 1; \
       } \
     else _curext_supported = 0;


   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Scanning again to add to the gles1 ext string list
   /////////////////////////////////////////////////////////////////////////////////////////////////////

#define _EVASGL_EXT_END()
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_DRVNAME_PRINT(name) \
     { \
        eina_strbuf_append(sb, name" "); \
        if ((strncmp(name, "GL_", 3) == 0) && (strstr(eina_strbuf_string_get(sboff), name) == NULL)) \
          eina_strbuf_append(sboff, name" "); \
     }
#define _EVASGL_EXT_DRVNAME(name) \
   if (_curext_supported) \
      _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_curext_supported && _gles1_ext_support_func_##name) \
      _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)
#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME_PRINT
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

   if (_gles1_ext_string) free(_gles1_ext_string);
   if (_gles1_ext_string_official) free(_gles1_ext_string_official);
   _gles1_ext_string = eina_strbuf_string_steal(sb);
   _gles1_ext_string_official = eina_strbuf_string_steal(sboff);
   eina_strbuf_free(sb);
   eina_strbuf_free(sboff);

   if (evgl_engine->api_debug_mode)
     DBG("GLES1: List of supported extensions:\n%s", _gles1_ext_string);

   // GLESv1 version has been initialized!
   _evgl_api_ext_status |= EVASGL_API_GLES1_EXT_INITIALIZED;
   return EINA_TRUE;
error:
   eina_strbuf_free(sb);
   eina_strbuf_free(sboff);
   return EINA_FALSE;
}

void
evgl_api_gles1_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts)
{
   if (!(_evgl_api_ext_status & EVASGL_API_GLES1_EXT_INITIALIZED))
     {
        DBG("Initializing GLESv1 extensions...");
        if (!_evgl_api_gles1_ext_init(getproc, glueexts))
          {
             ERR("GLESv1 extensions initialization failed");
             return;
          }
     }

#define ORD(f) EVAS_API_OVERRIDE(f, gl_funcs, evgl_gles1_)

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name) \
   if (_gles1_ext_support_##name != 0) \
     {
#define _EVASGL_EXT_END() \
     }
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
   ORD(name);
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#undef _EVASGL_EXT_WHITELIST_ONLY
#define _EVASGL_EXT_WHITELIST_ONLY 0
#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#undef ORD

}

/**
 * @brief Initializes GLES3 extension support.
 *
 * This function is called once to query and load all supported GLES3
 * extensions. It populates the internal extension support flags and function
 * pointers for GLES3. It uses the X-macro pattern with `evas_gl_api_ext_def.h`.
 * A GLES3 context must be current when this is called.
 *
 * @param getproc A function pointer to `eglGetProcAddress` or similar.
 * @param glueexts A space-separated string of additional extensions.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_evgl_api_gles3_ext_init(void *getproc, const char *glueexts)
{
   const char *glexts;
   fp_getproc gp = (fp_getproc)getproc;
   int _curext_supported = 0;
   Evas_GL_API *gles3_funcs;
   Eina_Strbuf *sb = eina_strbuf_new();
   Eina_Strbuf *sboff = eina_strbuf_new();

#ifdef GL_GLES
   EVGL_Resource *rsc;
   EGLint context_version;
   EGLDisplay dpy = EGLDISPLAY_GET(NULL);

   /* glGetString returns the information for the currently bound context
    * So, update gles3_exts only if GLES3 context is currently bound.
    * Check here if GLESv3 is current
    */
   if (!(rsc=_evgl_tls_resource_get()))
     {
        ERR("Unable to initialize GLES3 extensions. Error retrieving tls");
        goto error;
     }

   if ((dpy == EGL_NO_DISPLAY) || !rsc->current_ctx)
     {
        DBG("Unable to initialize GLES3 extensions. Engine not initialized");
        goto error;
     }

   if (!eglQueryContext(dpy, rsc->current_ctx->context, EGL_CONTEXT_CLIENT_VERSION, &context_version))
     {
        ERR("Unable to initialize GLES3 extensions. eglQueryContext failed 0x%x", eglGetError());
        goto error;
     }

   if (context_version != EVAS_GL_GLES_3_X)
     {
        DBG("GLESv3 context not bound");
        goto error;
     }
#endif

   _gles3_ext_plist = eina_array_new(1);
   gles3_funcs = _evgl_api_gles3_internal_get();
   if (!gles3_funcs || !gles3_funcs->glGetString)
     {
        ERR("Could not get address of glGetString in GLESv3 library!");
        goto error;
     }

   glexts = (const char *) gles3_funcs->glGetString(GL_EXTENSIONS);
   if (!glexts)
     {
        ERR("GLESv3:glGetString(GL_EXTENSIONS) returned NULL!");
        goto error;
     }

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Scanning supported extensions, sets the variables
   /////////////////////////////////////////////////////////////////////////////////////////////////////

   // Preparing all the magic macros
#define GETPROCADDR(sym) \
      (((!(*drvfunc)) && (gp)) ? (__typeof__((*drvfunc)))gp(sym) : (__typeof__((*drvfunc)))dlsym(RTLD_DEFAULT, sym))

#define _EVASGL_EXT_BEGIN(name) \
   { \
      int *ext_support = &_gles3_ext_support_##name; \
      *ext_support = 0;

#define _EVASGL_EXT_END() \
   }

#define _EVASGL_EXT_CHECK_SUPPORT(name) \
   ((strstr(glexts, name) != NULL) || (strstr(glueexts, name) != NULL))

#define _EVASGL_EXT_DISCARD_SUPPORT() \
   *ext_support = 0;

#define _EVASGL_EXT_DRVNAME(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) *ext_support = 1;

#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_EVASGL_EXT_CHECK_SUPPORT(#name)) { *ext_support = 1; _gles3_ext_support_func_##name = 1; }
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname) \
   if (_EVASGL_EXT_CHECK_SUPPORT(deskname)) *ext_support = 1;

#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
     { \
        ret (**drvfunc)param1 = &gles3_ext_sym_##name; \
        if (*ext_support == 1) \
          {

#define _EVASGL_EXT_FUNCTION_END() \
          } \
        if ((*drvfunc) == NULL) _EVASGL_EXT_DISCARD_SUPPORT(); \
     }
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name) \
   if ((*drvfunc) == NULL) *drvfunc = name;

#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name) \
   if ((*drvfunc) == NULL) \
     { \
        *drvfunc = GETPROCADDR(name); \
        evgl_safe_extension_add(name, (void *) (*drvfunc)); \
     } \
   else evgl_safe_extension_add(name, NULL);

#ifdef _EVASGL_EXT_FUNCTION_WHITELIST
# undef _EVASGL_EXT_FUNCTION_WHITELIST
#endif
#define _EVASGL_EXT_FUNCTION_WHITELIST(name) evgl_safe_extension_add(name, NULL);

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

   // Okay, now we are ready to scan.
#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_FUNCTION_WHITELIST
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
#undef GETPROCADDR

#define _EVASGL_EXT_BEGIN(name) \
     if (_gles3_ext_support_##name != 0) \
       { \
          eina_strbuf_append(sb, #name" "); \
          _curext_supported = 1; \
       } \
     else _curext_supported = 0;


   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Scanning again to add to the gles3 ext string list
   /////////////////////////////////////////////////////////////////////////////////////////////////////

#define _EVASGL_EXT_END()
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_DRVNAME_PRINT(name) \
     { \
        eina_strbuf_append(sb, name" "); \
        if ((strncmp(name, "GL_", 3) == 0) && (strstr(eina_strbuf_string_get(sboff), name) == NULL)) \
          { \
             eina_strbuf_append(sboff, name" "); \
             eina_array_push(_gles3_ext_plist, name); \
          } \
     }
#define _EVASGL_EXT_DRVNAME(name) \
   if (_curext_supported) \
      _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name) \
   if (_curext_supported && _gles3_ext_support_func_##name) \
      _EVASGL_EXT_DRVNAME_PRINT(#name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME_PRINT
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR

   if (_gles3_ext_string) free(_gles3_ext_string);
   if (_gles3_ext_string_official) free(_gles3_ext_string_official);
   _gles3_ext_string = eina_strbuf_string_steal(sb);
   _gles3_ext_string_official = eina_strbuf_string_steal(sboff);
   eina_strbuf_free(sb);
   eina_strbuf_free(sboff);

   if (evgl_engine->api_debug_mode)
     DBG("GLES3: List of supported extensions:\n%s", _gles3_ext_string);

   // GLESv3 version has been initialized!
   _evgl_api_ext_status |= EVASGL_API_GLES3_EXT_INITIALIZED;
   return EINA_TRUE;
error:
   eina_strbuf_free(sb);
   eina_strbuf_free(sboff);
   return EINA_FALSE;
}

void
evgl_api_gles3_ext_get(Evas_GL_API *gl_funcs, void *getproc, const char *glueexts)
{
   if (!(_evgl_api_ext_status & EVASGL_API_GLES3_EXT_INITIALIZED))
     {
        DBG("Initializing GLESv3 extensions...");
        if (!_evgl_api_gles3_ext_init(getproc, glueexts))
          {
             ERR("GLESv3 extensions initialization failed");
             return;
          }
     }

#define ORD(f) EVAS_API_OVERRIDE(f, gl_funcs, evgl_gles3_)

   /////////////////////////////////////////////////////////////////////////////////////////////////////
   // Extension HEADER
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#define _EVASGL_EXT_CHECK_SUPPORT(name)
#define _EVASGL_EXT_DISCARD_SUPPORT()
#define _EVASGL_EXT_BEGIN(name) \
   if (_gles3_ext_support_##name != 0) \
     {
#define _EVASGL_EXT_END() \
     }
#define _EVASGL_EXT_DRVNAME(name)
#define _EVASGL_EXT_DRVNAME_PRIVATE(name)
#define _EVASGL_EXT_DRVNAME_DESKTOP(deskname)
#define _EVASGL_EXT_FUNCTION_BEGIN(ret, name, param1, param2) \
   ORD(name);
#define _EVASGL_EXT_FUNCTION_END()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN()
#define _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END()
#define _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN(ret, name, param1, param2)
#define _EVASGL_EXT_FUNCTION_PRIVATE_END()
#define _EVASGL_EXT_FUNCTION_DRVFUNC(name)
#define _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR(name)

#undef _EVASGL_EXT_WHITELIST_ONLY
#define _EVASGL_EXT_WHITELIST_ONLY 0

#define _EVASGL_EXT_ENABLE_GL_GLES 1
#define _EVASGL_EXT_ENABLE_EGL 1

#include "evas_gl_api_ext_def.h"

#undef _EVASGL_EXT_ENABLE_EGL
#undef _EVASGL_EXT_ENABLE_GL_GLES
#undef _EVASGL_EXT_CHECK_SUPPORT
#undef _EVASGL_EXT_DISCARD_SUPPORT
#undef _EVASGL_EXT_BEGIN
#undef _EVASGL_EXT_END
#undef _EVASGL_EXT_DRVNAME
#undef _EVASGL_EXT_DRVNAME_PRIVATE
#undef _EVASGL_EXT_DRVNAME_DESKTOP
#undef _EVASGL_EXT_FUNCTION_BEGIN
#undef _EVASGL_EXT_FUNCTION_END
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_BEGIN
#undef _EVASGL_EXT_FUNCTION_DISABLE_FOR_GLES1_END
#undef _EVASGL_EXT_FUNCTION_PRIVATE_BEGIN
#undef _EVASGL_EXT_FUNCTION_PRIVATE_END
#undef _EVASGL_EXT_FUNCTION_DRVFUNC
#undef _EVASGL_EXT_FUNCTION_DRVFUNC_PROCADDR
   /////////////////////////////////////////////////////////////////////////////////////////////////////
#undef ORD

}

const char *
evgl_api_ext_egl_string_get(void)
{
#ifdef GL_GLES
   if (!(_evgl_api_ext_status & EVASGL_API_EGL_EXT_INITIALIZED))
     {
        ERR("EVGL extension for egl is not yet initialized.");
        return NULL;
     }

   return _egl_ext_string;
#else
   return "";
#endif
}

const char *
evgl_api_ext_string_get(Eina_Bool official, int version)
{
   if (_evgl_api_ext_status < 1)
     {
        ERR("EVGL extension is not yet initialized.");
        return NULL;
     }

   if (version == EVAS_GL_GLES_1_X)
     return (official?_gles1_ext_string_official:_gles1_ext_string);

   if (version == EVAS_GL_GLES_3_X)
     return (official?_gles3_ext_string_official:_gles3_ext_string);

   return (official?_gl_ext_string_official:_gl_ext_string);
}

const char *
evgl_api_ext_stringi_get(GLuint index, int version)
{
   if (_evgl_api_ext_status < 1)
     {
        ERR("EVGL extension is not yet initialized.");
        return NULL;
     }

   if (version == EVAS_GL_GLES_3_X)
     {
        if (index < evgl_api_ext_num_extensions_get(version))
          {
             return eina_array_data_get(_gles3_ext_plist, index);
          }
     }

   return NULL;
}

GLuint
evgl_api_ext_num_extensions_get(int version)
{
   if (version == EVAS_GL_GLES_3_X)
     return eina_array_count_get(_gles3_ext_plist);

   return 0;
}
