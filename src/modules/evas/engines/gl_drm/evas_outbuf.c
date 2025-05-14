#include "evas_engine.h"

/* local variables */
/** @internal Pointer to the currently active Outbuf window for GL operations. */
static Outbuf *_evas_gl_drm_window = NULL;
/** @internal Shared EGL context, if contexts can be shared. */
static EGLContext context = EGL_NO_CONTEXT;
/** @internal Counter for the number of active Outbuf windows. */
static int win_count = 0;

#ifdef EGL_MESA_platform_gbm
/** @internal Function pointer for eglGetPlatformDisplayEXT, used for GBM platform. */
static PFNEGLGETPLATFORMDISPLAYEXTPROC dlsym_eglGetPlatformDisplayEXT = NULL;
/** @internal Function pointer for eglCreatePlatformWindowSurfaceEXT, used for GBM platform. */
static PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC dlsym_eglCreatePlatformWindowSurfaceEXT = NULL;
#endif

/**
 * @internal
 * @brief Destroys the GBM surface associated with the output buffer.
 *
 * This function releases the GBM surface if it exists.
 *
 * @param ob The output buffer (Outbuf) whose GBM surface is to be destroyed.
 */
static void
_evas_outbuf_gbm_surface_destroy(Outbuf *ob)
{
   if (!ob) return;
   if (ob->surface)
     {
        gbm_surface_destroy(ob->surface);
        ob->surface = NULL;
     }
}

/**
 * @internal
 * @brief Creates a GBM surface for the output buffer.
 *
 * This function creates a GBM surface with the specified width, height,
 * and format suitable for rendering and scanout.
 *
 * @param ob The output buffer (Outbuf) for which to create the GBM surface.
 * @param w The width of the surface.
 * @param h The height of the surface.
 */
static void
_evas_outbuf_gbm_surface_create(Outbuf *ob, int w, int h)
{
   unsigned int format = GBM_FORMAT_XRGB8888;
   unsigned int flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;

   if (!ob) return;

   ob->surface =
     gbm_surface_create(ob->info->info.gbm, w, h, format, flags);

   if (!ob->surface) ERR("Failed to create gbm surface");
}

/**
 * @internal
 * @brief Callback function for GBM buffer object destruction.
 *
 * This function is called when a GBM buffer object (BO) is no longer
 * needed and is being destroyed. It discards the associated Ecore_Drm2_Fb.
 *
 * @param bo The GBM buffer object being destroyed (unused in function).
 * @param data User data, expected to be an Ecore_Drm2_Fb pointer.
 */
static void
_evas_outbuf_fb_cb_destroy(struct gbm_bo *bo EINA_UNUSED, void *data)
{
   Ecore_Drm2_Fb *fb;

   fb = data;
   if (fb) ecore_drm2_fb_discard(fb);
}

/**
 * @internal
 * @brief Callback function for Ecore_Drm2_Fb release.
 *
 * This function is invoked when an Ecore_Drm2 framebuffer (Fb) is released
 * (e.g., after a page flip). It releases the corresponding GBM buffer.
 *
 * @param fb The Ecore_Drm2 framebuffer that was released.
 * @param status The status of the framebuffer release.
 *               Only ECORE_DRM2_FB_STATUS_RELEASE triggers action.
 * @param data User data, expected to be an Outbuf pointer.
 */
void
_evas_outbuf_release_fb(Ecore_Drm2_Fb *fb, Ecore_Drm2_Fb_Status status, void *data)
{
   struct gbm_bo *bo;
   Outbuf *ob;

   if (status != ECORE_DRM2_FB_STATUS_RELEASE) return;

   ob = data;
   bo = ecore_drm2_fb_bo_get(fb);
   if ((!ob->surface) || (!bo)) return;
   gbm_surface_release_buffer(ob->surface, bo);
}

/**
 * @internal
 * @brief Retrieves or creates an Ecore_Drm2_Fb for a GBM buffer object.
 *
 * If an Ecore_Drm2_Fb already exists for the given GBM BO (stored as user
 * data), it is returned. Otherwise, a new Ecore_Drm2_Fb is created from
 * the GBM BO's properties and associated with it.
 *
 * @param ob The output buffer (Outbuf) context.
 * @param bo The GBM buffer object for which to get/create the framebuffer.
 * @return A pointer to the Ecore_Drm2_Fb, or NULL on failure.
 */
static Ecore_Drm2_Fb *
_evas_outbuf_fb_get(Outbuf *ob, struct gbm_bo *bo)
{
   Ecore_Drm2_Fb *fb;
   uint32_t format, hdl, stride;
   int w, h;

   fb = gbm_bo_get_user_data(bo);
   if (fb) return fb;

   format = gbm_bo_get_format(bo);
   w = gbm_bo_get_width(bo);
   h = gbm_bo_get_height(bo);
   hdl = gbm_bo_get_handle(bo).u32;
   stride = gbm_bo_get_stride(bo);
   /* fb->size = fb->stride * fb->h; */

   fb =
     ecore_drm2_fb_gbm_create(ob->dev, w, h, ob->depth, ob->bpp,
                              format, hdl, stride, bo);
   if (!fb)
     {
        ERR("Failed to create FBO");
        return NULL;
     }

   ecore_drm2_fb_status_handler_set(fb, _evas_outbuf_release_fb, ob);

   gbm_bo_set_user_data(bo, fb, _evas_outbuf_fb_cb_destroy);

   return fb;
}

/**
 * @internal
 * @brief Handles the buffer swapping logic for DRM/GBM.
 *
 * This function locks the front buffer of the GBM surface,
 * gets/creates an Ecore_Drm2_Fb for it, assigns it to a DRM plane
 * (if not already assigned or if it needs reassignment), and
 * schedules a page flip.
 *
 * @param ob The output buffer (Outbuf) for which to swap buffers.
 */
static void
_evas_outbuf_buffer_swap(Outbuf *ob)
{
   struct gbm_bo *bo;
   Ecore_Drm2_Fb *fb = NULL;

   bo = gbm_surface_lock_front_buffer(ob->surface);
   if (!bo)
     {
        ecore_drm2_fb_release(ob->priv.output, EINA_TRUE);
        bo = gbm_surface_lock_front_buffer(ob->surface);
     }
   if (bo) fb = _evas_outbuf_fb_get(ob, bo);

   if (fb)
     {
        if (!ob->priv.plane)
          ob->priv.plane = ecore_drm2_plane_assign(ob->priv.output, fb, 0, 0);
        else ecore_drm2_plane_fb_set(ob->priv.plane, fb);

        ecore_drm2_fb_flip(fb, ob->priv.output);
     }
   else
     WRN("Could not get FBO from Bo");
}

/**
 * @internal
 * @brief Makes the EGL context of the Outbuf current or not current.
 *
 * This function is typically used as a callback for
 * glsym_evas_gl_preload_render_lock to manage EGL context activation.
 *
 * @param data User data, expected to be an Outbuf pointer.
 * @param doit If EINA_TRUE, makes the context current.
 *             If EINA_FALSE, makes no context current.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_evas_outbuf_make_current(void *data, void *doit)
{
   Outbuf *ob;

   if (!(ob = data)) return EINA_FALSE;

   if (doit)
     {
        if (!eglMakeCurrent(ob->egl.disp, ob->egl.surface,
                            ob->egl.surface, ob->egl.context))
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
 * @internal
 * @brief Initializes EGL extensions required by the GBM platform.
 *
 * This function checks for and retrieves function pointers for
 * EGL_EXT_platform_base and related extensions if EGL_MESA_platform_gbm
 * is defined. It ensures it's only run once.
 *
 * @return EINA_TRUE if initialization was successful or already done,
 *         EINA_FALSE otherwise (though currently always returns EINA_TRUE
 *         after the first successful call).
 */
static Eina_Bool
_evas_outbuf_init(void)
{
   static int _init = 0;

   if (_init) return EINA_TRUE;
#ifdef EGL_MESA_platform_gbm
   {
     const char *exts;

     exts = eglQueryString(NULL, EGL_EXTENSIONS);
     if (_ckext(exts, "EGL_EXT_platform_base"))
       {
          dlsym_eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
                eglGetProcAddress("eglGetPlatformDisplayEXT");
          dlsym_eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXTPROC)
                eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
       }
   }
#endif
   _init = 1;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets up EGL for the given output buffer.
 *
 * This involves:
 * - Initializing EGL display connection (trying platform GBM first, then default).
 * - Choosing an EGL configuration matching GBM format.
 * - Creating an EGL window surface using the GBM surface.
 * - Creating an EGL context (sharing with a global context if possible).
 * - Making the context current.
 * - Loading GL symbols.
 * - Performing driver blacklisting checks.
 * - Creating an Evas GL common context.
 *
 * @param ob The output buffer (Outbuf) to set up EGL for.
 * @return EINA_TRUE on successful EGL setup, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_outbuf_egl_setup(Outbuf *ob)
{
   const char *exts;
   int ctx_attr[5];
   int cfg_attr[40];
   int maj = 0, min = 0, n = 0, i = 0, cn = 0;
   EGLint ncfg = 0;
   EGLConfig *cfgs;
   const GLubyte *vendor, *renderer, *version, *glslversion;
   Eina_Bool blacklist = EINA_FALSE;

   if (!_evas_outbuf_init())
     {
        ERR("Could not initialize engine!");
        return EINA_FALSE;
     }

   /* setup gbm egl surface */
   ctx_attr[cn++] = EGL_CONTEXT_CLIENT_VERSION;
   ctx_attr[cn++] = 2;

   if (_extn_have_context_priority)
     {
        ctx_attr[cn++] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
        ctx_attr[cn++] = EGL_CONTEXT_PRIORITY_HIGH_IMG;
     }
   ctx_attr[cn++] = EGL_NONE;

   cfg_attr[n++] = EGL_RENDERABLE_TYPE;
   cfg_attr[n++] = EGL_OPENGL_ES2_BIT;
   cfg_attr[n++] = EGL_SURFACE_TYPE;
   cfg_attr[n++] = EGL_WINDOW_BIT;

   cfg_attr[n++] = EGL_RED_SIZE;
   cfg_attr[n++] = 1;
   cfg_attr[n++] = EGL_GREEN_SIZE;
   cfg_attr[n++] = 1;
   cfg_attr[n++] = EGL_BLUE_SIZE;
   cfg_attr[n++] = 1;


   cfg_attr[n++] = EGL_ALPHA_SIZE;
   if (ob->destination_alpha) cfg_attr[n++] = 1;
   else cfg_attr[n++] = 0;
   cfg_attr[n++] = EGL_NONE;

   ob->egl.disp = EGL_NO_DISPLAY;
#ifdef EGL_MESA_platform_gbm
   if (dlsym_eglGetPlatformDisplayEXT)
     ob->egl.disp = dlsym_eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA,
                                                   ob->info->info.gbm,
                                                   NULL);
#endif
   if (ob->egl.disp == EGL_NO_DISPLAY)
     ob->egl.disp = eglGetDisplay((EGLNativeDisplayType)ob->info->info.gbm);
   if (ob->egl.disp == EGL_NO_DISPLAY)
     {
        ERR("eglGetDisplay() fail. code=%#x", eglGetError());
        return EINA_FALSE;
     }

   if (!eglInitialize(ob->egl.disp, &maj, &min))
     {
        ERR("eglInitialize() fail. code=%#x", eglGetError());
        return EINA_FALSE;
     }

   eglBindAPI(EGL_OPENGL_ES_API);
   if (eglGetError() != EGL_SUCCESS)
     {
        ERR("eglBindAPI() fail. code=%#x", eglGetError());
        return EINA_FALSE;
     }

   eng_egl_symbols(ob->egl.disp);

   if (!eglGetConfigs(ob->egl.disp, NULL, 0, &ncfg) || (ncfg == 0))
     {
        ERR("eglGetConfigs() fail. code=%#x", eglGetError());
        return EINA_FALSE;
     }

   cfgs = malloc(ncfg * sizeof(EGLConfig));
   if (!cfgs)
     {
        ERR("Failed to malloc space for egl configs");
        return EINA_FALSE;
     }

   if (!eglChooseConfig(ob->egl.disp, cfg_attr, cfgs,
                        ncfg, &ncfg) || (ncfg == 0))
     {
        ERR("eglChooseConfig() fail. code=%#x", eglGetError());
        goto err;
     }

   for (; i < ncfg; ++i)
     {
        EGLint format = 0;

        if (!eglGetConfigAttrib(ob->egl.disp, cfgs[i], EGL_NATIVE_VISUAL_ID,
                                &format))
          {
             ERR("eglGetConfigAttrib() fail. code=%#x", eglGetError());
             goto err;
          }

        if (format == (int)ob->info->info.format)
          {
             ob->egl.config = cfgs[i];
             break;
          }
     }

   if (ob->egl.surface != EGL_NO_SURFACE)
     eglDestroySurface(ob->egl.disp, ob->egl.surface);
   ob->egl.surface = EGL_NO_SURFACE;
#ifdef EGL_MESA_platform_gbm
   if (dlsym_eglCreatePlatformWindowSurfaceEXT)
     ob->egl.surface =
       dlsym_eglCreatePlatformWindowSurfaceEXT(ob->egl.disp, ob->egl.config,
                                               ob->surface, NULL);
#endif
   if (ob->egl.surface == EGL_NO_SURFACE)
     ob->egl.surface = eglCreateWindowSurface(ob->egl.disp, ob->egl.config,
                                              (EGLNativeWindowType)ob->surface,
                                              NULL);
   if (ob->egl.surface == EGL_NO_SURFACE)
     {
        ERR("eglCreateWindowSurface() fail for %p. code=%#x",
            ob->surface, eglGetError());
        goto err;
     }

   ob->egl.context =
     eglCreateContext(ob->egl.disp, ob->egl.config, context, ctx_attr);
   if (ob->egl.context == EGL_NO_CONTEXT)
     {
        ERR("eglCreateContext() fail. code=%#x", eglGetError());
        goto err;
     }

   if (context == EGL_NO_CONTEXT) context = ob->egl.context;

   if (eglMakeCurrent(ob->egl.disp, ob->egl.surface,
                      ob->egl.surface, ob->egl.context) == EGL_FALSE)
     {
        ERR("eglMakeCurrent() fail. code=%#x", eglGetError());
        goto err;
     }

   exts = eglQueryString(ob->egl.disp, EGL_EXTENSIONS);
   glsym_evas_gl_symbols(glsym_eglGetProcAddress, exts);

   vendor = glGetString(GL_VENDOR);
   renderer = glGetString(GL_RENDERER);
   version = glGetString(GL_VERSION);
   glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
   if (!vendor)   vendor   = (unsigned char *)"-UNKNOWN-";
   if (!renderer) renderer = (unsigned char *)"-UNKNOWN-";
   if (!version)  version  = (unsigned char *)"-UNKNOWN-";
   if (!glslversion) glslversion = (unsigned char *)"-UNKNOWN-";
   if (getenv("EVAS_GL_INFO"))
     {
        fprintf(stderr, "vendor  : %s\n", vendor);
        fprintf(stderr, "renderer: %s\n", renderer);
        fprintf(stderr, "version : %s\n", version);
        fprintf(stderr, "glsl ver: %s\n", glslversion);
     }

   if (strstr((const char *)vendor, "Mesa Project"))
     {
        if (strstr((const char *)renderer, "Software Rasterizer"))
          blacklist = EINA_TRUE;
     }
   if (strstr((const char *)renderer, "softpipe"))
     blacklist = EINA_TRUE;
   if (strstr((const char *)renderer, "llvmpipe"))
     blacklist = EINA_TRUE;

   if ((blacklist) && (!getenv("EVAS_GL_NO_BLACKLIST")))
     {
        ERR("OpenGL Driver blacklisted:");
        ERR("Vendor: %s", (const char *)vendor);
        ERR("Renderer: %s", (const char *)renderer);
        ERR("Version: %s", (const char *)version);
        goto err;
     }

   ob->gl_context = glsym_evas_gl_common_context_new();
   if (!ob->gl_context) goto err;

#ifdef GL_GLES
   ob->gl_context->egldisp = ob->egl.disp;
   ob->gl_context->eglctxt = ob->egl.context;
#endif

   evas_outbuf_use(ob);
   glsym_evas_gl_common_context_resize(ob->gl_context,
                                       ob->w, ob->h, ob->rotation);

   ob->surf = EINA_TRUE;

   free(cfgs);
   return EINA_TRUE;

err:
   free(cfgs);
   return EINA_FALSE;
}

/**
 * @brief Creates a new output buffer (Outbuf) for GL rendering on DRM.
 *
 * This function allocates and initializes an Outbuf structure, which
 * represents a render target (window or surface) for Evas GL operations
 * using DRM/GBM. It sets up the GBM surface and EGL resources.
 *
 * @param info Pointer to Evas_Engine_Info_GL_Drm containing DRM and display properties.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @param swap_mode The desired buffer swap mode.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *
evas_outbuf_new(Evas_Engine_Info_GL_Drm *info, int w, int h, Render_Output_Swap_Mode swap_mode)
{
   Outbuf *ob;

   if (!info) return NULL;

   /* try to allocate space for outbuf */
   if (!(ob = calloc(1, sizeof(Outbuf)))) return NULL;

   win_count++;

   ob->w = w;
   ob->h = h;
   ob->info = info;
   ob->depth = info->info.depth;
   ob->rotation = info->info.rotation;
   ob->destination_alpha = info->info.destination_alpha;
   /* ob->vsync = info->info.vsync; */
   ob->swap_mode = swap_mode;

   ob->dev = info->info.dev;
   ob->bpp = info->info.bpp;
   ob->format = info->info.format;
   ob->priv.output = info->info.output;

   /* if ((num = getenv("EVAS_GL_DRM_VSYNC"))) */
   /*   ob->vsync = atoi(num); */

   if ((ob->rotation == 0) || (ob->rotation == 180))
     _evas_outbuf_gbm_surface_create(ob, w, h);
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     _evas_outbuf_gbm_surface_create(ob, h, w);

   if (!_evas_outbuf_egl_setup(ob))
     {
        evas_outbuf_free(ob);
        return NULL;
     }

   return ob;
}

/**
 * @brief Frees an output buffer (Outbuf) and its associated resources.
 *
 * This function cleans up all resources tied to the Outbuf, including
 * the Evas GL context, EGL context, EGL surface, and GBM surface.
 * It also handles global EGL termination if this is the last Outbuf.
 *
 * @param ob The output buffer (Outbuf) to free.
 */
void
evas_outbuf_free(Outbuf *ob)
{
   int ref = 0;

   win_count--;
   evas_outbuf_use(ob);

   if (win_count == 0) evas_common_font_ext_clear();

   if (ob == _evas_gl_drm_window) _evas_gl_drm_window = NULL;

   if (ob->gl_context)
     {
        ref = ob->gl_context->references - 1;
        glsym_evas_gl_common_context_free(ob->gl_context);
     }

   eglMakeCurrent(ob->egl.disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

   if (ob->egl.context != context)
     eglDestroyContext(ob->egl.disp, ob->egl.context);

   if (ob->egl.surface != EGL_NO_SURFACE)
     eglDestroySurface(ob->egl.disp, ob->egl.surface);

   _evas_outbuf_gbm_surface_destroy(ob);

   if (ref == 0)
     {
        if (context) eglDestroyContext(ob->egl.disp, context);
        eglTerminate(ob->egl.disp);
        eglReleaseThread();
        context = EGL_NO_CONTEXT;
     }

   free(ob);
}

/**
 * @brief Sets the specified output buffer as the current target for GL operations.
 *
 * This function ensures that subsequent GL commands are directed to the
 * EGL context and surface associated with the given Outbuf. It handles
 * flushing the previous context if a switch occurs.
 *
 * @param ob The output buffer (Outbuf) to make current.
 *           If NULL, it might imply releasing the current context,
 *           though the primary use is to set a new current Outbuf.
 */
void
evas_outbuf_use(Outbuf *ob)
{
   Eina_Bool force = EINA_FALSE;

   glsym_evas_gl_preload_render_lock(_evas_outbuf_make_current, ob);

   if (_evas_gl_drm_window)
     {
        if (eglGetCurrentContext() != _evas_gl_drm_window->egl.context)
          force = EINA_TRUE;
     }

   if ((_evas_gl_drm_window != ob) || (force))
     {
        if (_evas_gl_drm_window)
          {
             glsym_evas_gl_common_context_use(_evas_gl_drm_window->gl_context);
             glsym_evas_gl_common_context_flush(_evas_gl_drm_window->gl_context);
          }

        _evas_gl_drm_window = ob;

        if (ob)
          {
             if (ob->egl.surface != EGL_NO_SURFACE)
               {
                  if (eglMakeCurrent(ob->egl.disp, ob->egl.surface,
                                     ob->egl.surface,
                                     ob->egl.context) == EGL_FALSE)
                    ERR("eglMakeCurrent() failed!");
               }
          }
     }

   if (ob) glsym_evas_gl_common_context_use(ob->gl_context);
}

/**
 * @brief Recreates the EGL surface for an output buffer.
 *
 * This is used if the EGL surface was previously destroyed (e.g., by
 * evas_outbuf_unsurf) and needs to be made available again.
 * It makes the new surface current.
 *
 * @param ob The output buffer (Outbuf) whose EGL surface needs recreation.
 */
void
evas_outbuf_resurf(Outbuf *ob)
{
   if (ob->surf) return;
   if (getenv("EVAS_GL_INFO")) printf("resurf %p\n", ob);

   if (ob->egl.surface != EGL_NO_SURFACE)
     eglDestroySurface(ob->egl.disp, ob->egl.surface);
   ob->egl.surface =
     eglCreateWindowSurface(ob->egl.disp, ob->egl.config,
                            (EGLNativeWindowType)ob->surface, NULL);

   if (ob->egl.surface == EGL_NO_SURFACE)
     {
        ERR("eglCreateWindowSurface() fail for %p. code=%#x",
            ob->surface, eglGetError());
        return;
     }

   if (eglMakeCurrent(ob->egl.disp, ob->egl.surface, ob->egl.surface,
                      ob->egl.context) == EGL_FALSE)
     ERR("eglMakeCurrent() failed!");

   ob->surf = EINA_TRUE;
}

/**
 * @brief Destroys the EGL surface of an output buffer.
 *
 * This function is called to release the EGL surface, typically when
 * the window is hidden or no longer needs to be rendered to directly.
 * It also clears the current EGL context if it was associated with this Outbuf.
 * The `EVAS_GL_WIN_RESURF` environment variable can prevent this.
 *
 * @param ob The output buffer (Outbuf) whose EGL surface is to be destroyed.
 */
void
evas_outbuf_unsurf(Outbuf *ob)
{
   if (!ob->surf) return;
   if (!getenv("EVAS_GL_WIN_RESURF")) return;
   if (getenv("EVAS_GL_INFO")) printf("unsurf %p\n", ob);

   if (_evas_gl_drm_window)
      glsym_evas_gl_common_context_flush(_evas_gl_drm_window->gl_context);
   if (_evas_gl_drm_window == ob)
     {
        eglMakeCurrent(ob->egl.disp, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (ob->egl.surface != EGL_NO_SURFACE)
           eglDestroySurface(ob->egl.disp, ob->egl.surface);
        ob->egl.surface = EGL_NO_SURFACE;

        _evas_gl_drm_window = NULL;
     }

   ob->surf = EINA_FALSE;
}

/**
 * @brief Reconfigures an existing output buffer.
 *
 * This function changes the dimensions, rotation, or depth of an Outbuf.
 * It involves releasing pending framebuffers, destroying and recreating
 * the GBM surface with new parameters, and then re-setting up EGL.
 *
 * @param ob The output buffer (Outbuf) to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation (0, 90, 180, 270 degrees).
 * @param depth The new color depth (or OUTBUF_DEPTH_INHERIT to keep current).
 */
void
evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth)
{
   if (depth == OUTBUF_DEPTH_INHERIT) depth = ob->depth;

   while (ecore_drm2_fb_release(ob->priv.output, EINA_TRUE));

   ob->w = w;
   ob->h = h;
   ob->rotation = rot;

   _evas_outbuf_gbm_surface_destroy(ob);

   if ((ob->rotation == 0) || (ob->rotation == 180))
     _evas_outbuf_gbm_surface_create(ob, w, h);
   else if ((ob->rotation == 90) || (ob->rotation == 270))
     _evas_outbuf_gbm_surface_create(ob, h, w);

   _evas_outbuf_egl_setup(ob);
}

/**
 * @brief Gets the current buffer swap mode, potentially based on EGL_BUFFER_AGE_EXT.
 *
 * If the swap mode is set to MODE_AUTO and the EGL_BUFFER_AGE_EXT extension
 * is available, this function queries the age of the current front buffer.
 * The age determines the most efficient swap mode (e.g., MODE_COPY, MODE_DOUBLE).
 * If the age changes unexpectedly, it defaults to MODE_FULL.
 *
 * @param ob The output buffer (Outbuf) to query.
 * @return The determined Render_Output_Swap_Mode. Returns MODE_FULL if
 *         auto-detection is not enabled/available or on error.
 */
Render_Output_Swap_Mode
evas_outbuf_buffer_state_get(Outbuf *ob)
{
   /* check for valid output buffer */
   if (!ob) return MODE_FULL;

   ecore_drm2_fb_release(ob->priv.output, EINA_FALSE);

   if (ob->swap_mode == MODE_AUTO && _extn_have_buffer_age)
     {
        Render_Output_Swap_Mode swap_mode;
        EGLint age = 0;

        eina_evlog("+gl_query_surf_swap_mode", ob, 0.0, NULL);
        if (!eglQuerySurface(ob->egl.disp, ob->egl.surface,
                             EGL_BUFFER_AGE_EXT, &age))
          age = 0;

        if (age == 1) swap_mode = MODE_COPY;
        else if (age == 2) swap_mode = MODE_DOUBLE;
        else if (age == 3) swap_mode = MODE_TRIPLE;
        else if (age == 4) swap_mode = MODE_QUADRUPLE;
        else swap_mode = MODE_FULL;
        if ((int)age != ob->prev_age)
          {
             char buf[16];
             snprintf(buf, sizeof(buf), "! %i", (int)age);
             eina_evlog("!gl_buffer_age", ob, 0.0, buf);
             swap_mode = MODE_FULL;
          }
        else
          {
             char buf[16];
             snprintf(buf, sizeof(buf), "%i", (int)age);
             eina_evlog("!gl_buffer_age", ob, 0.0, buf);
          }
        ob->prev_age = age;

        eina_evlog("-gl_query_surf_swap_mode", ob, 0.0, NULL);
        return swap_mode;
     }
   else
     return MODE_FULL;
}

/**
 * @brief Gets the rotation of the output buffer.
 *
 * @param ob The output buffer (Outbuf).
 * @return The rotation angle in degrees (0, 90, 180, or 270).
 */
int
evas_outbuf_rot_get(Outbuf *ob)
{
   return ob->rotation;
}

/**
 * @brief Prepares the GL context for rendering the first rectangle of an update.
 *
 * This function ensures the Outbuf's GL context is current, resizes it if
 * necessary, flushes any pending operations from previous frames, and starts
 * a new frame.
 *
 * @param ob The output buffer (Outbuf) being updated.
 * @return EINA_TRUE if window check (_re_wincheck) passes and setup is done,
 *         EINA_FALSE if preparation should skip (e.g. window not ready).
 *         Note: The return value seems inverted in its current usage logic;
 *         it returns EINA_TRUE if _re_wincheck fails, effectively skipping.
 */
Eina_Bool
evas_outbuf_update_region_first_rect(Outbuf *ob)
{
   /* ob->gl_context->preserve_bit = GL_COLOR_BUFFER_BIT0_QCOM; */

   glsym_evas_gl_preload_render_lock(_evas_outbuf_make_current, ob);
   evas_outbuf_use(ob);

   if (!_re_wincheck(ob)) return EINA_TRUE;

   glsym_evas_gl_common_context_resize(ob->gl_context, ob->w, ob->h, ob->rotation);
   glsym_evas_gl_common_context_flush(ob->gl_context);
   glsym_evas_gl_common_context_newframe(ob->gl_context);

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Converts Evas coordinates to OpenGL coordinates based on rotation.
 *
 * OpenGL typically has its origin at the bottom-left, while Evas might use
 * top-left. This function also accounts for canvas rotation.
 *
 * @param result Pointer to an integer array of size 4, which will be filled
 *               with `[gl_x, gl_y, gl_w, gl_h]`.
 *               Example: `int coords[4]; _glcoords_convert(coords, ...);`
 *                        `coords` will contain `[0, 0, 100, 100]` for a
 *                        100x100 region at (0,0) in a non-rotated 100x100 surface.
 * @param ob The output buffer (Outbuf) providing rotation and height context.
 * @param x The Evas x-coordinate of the rectangle.
 * @param y The Evas y-coordinate of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 */
static void
_glcoords_convert(int *result, Outbuf *ob, int x, int y, int w, int h)
{
   switch (ob->rotation)
     {
      case 0:
        result[0] = x;
        result[1] = ob->gl_context->h - (y + h);
        result[2] = w;
        result[3] = h;
        break;
      case 90:
        result[0] = y;
        result[1] = x;
        result[2] = h;
        result[3] = w;
        break;
      case 180:
        result[0] = ob->gl_context->w - (x + w);
        result[1] = y;
        result[2] = w;
        result[3] = h;
        break;
      case 270:
        result[0] = ob->gl_context->h - (y + h);
        result[1] = ob->gl_context->w - (x + w);
        result[2] = h;
        result[3] = w;
        break;
      default:
        result[0] = x;
        result[1] = ob->gl_context->h - (y + h);
        result[2] = w;
        result[3] = h;
        break;
     }
}

/**
 * @brief Sets the damage region for the EGL surface, if supported.
 *
 * This function informs EGL about which parts of the surface have changed,
 * potentially allowing for more efficient partial updates. It converts
 * Evas coordinates from the `damage` list to GL coordinates.
 *
 * @param ob The output buffer (Outbuf).
 * @param damage A linked list of Tilebuf_Rect structures defining the
 *               damaged areas in Evas coordinates.
 *               Example: A Tilebuf_Rect might be `{ .x=10, .y=20, .w=100, .h=50 }`.
 *                        The list is traversed using EINA_INLIST_FOREACH.
 */
void
evas_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage)
{
   if (glsym_eglSetDamageRegionKHR)
     {
        Tilebuf_Rect *tr;
        int *rect, *rects, count;

        count = eina_inlist_count(EINA_INLIST_GET(damage));
        rects = alloca(sizeof(int) * 4 * count);
        rect = rects;
        EINA_INLIST_FOREACH(damage, tr)
          {
             _glcoords_convert(rect, ob, tr->x, tr->y, tr->w, tr->h);
             rect += 4;
          }
        glsym_eglSetDamageRegionKHR(ob->egl.disp, ob->egl.surface, rects, count);
     }
}

/**
 * @brief Sets up a new region for update, configuring the master clip.
 *
 * This function defines the area that will be drawn to. If the update
 * region is smaller than the full output buffer, it enables and sets
 * a master clip in the GL context.
 *
 * @param ob The output buffer (Outbuf).
 * @param x The x-coordinate of the update region.
 * @param y The y-coordinate of the update region.
 * @param w The width of the update region.
 * @param h The height of the update region.
 * @param cx Unused parameter (intended for clip x).
 * @param cy Unused parameter (intended for clip y).
 * @param cw Unused parameter (intended for clip width).
 * @param ch Unused parameter (intended for clip height).
 * @return A pointer to the default surface of the GL context.
 *         This is typically `ob->gl_context->def_surface`.
 */
void *
evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx EINA_UNUSED, int *cy EINA_UNUSED, int *cw EINA_UNUSED, int *ch EINA_UNUSED)
{
   if ((w == ob->w) && (h == ob->h))
     ob->gl_context->master_clip.enabled = EINA_FALSE;
   else
     {
        ob->gl_context->master_clip.enabled = EINA_TRUE;
        ob->gl_context->master_clip.x = x;
        ob->gl_context->master_clip.y = y;
        ob->gl_context->master_clip.w = w;
        ob->gl_context->master_clip.h = h;
     }

   return ob->gl_context->def_surface;
}

/**
 * @brief Pushes an updated region to the display by flushing the GL context.
 *
 * This function is called after drawing operations for a specific region
 * are complete. It flushes the GL command buffer to ensure drawing commands
 * are processed.
 *
 * @param ob The output buffer (Outbuf).
 * @param update The RGBA_Image containing the updated pixel data (unused).
 * @param x The x-coordinate of the pushed region (unused).
 * @param y The y-coordinate of the pushed region (unused).
 * @param w The width of the pushed region (unused).
 * @param h The height of the pushed region (unused).
 */
void
evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   /* Is it really necessary to flush per region ? Shouldn't we be able to
      still do that for the full canvas when doing partial update */
   if (!_re_wincheck(ob)) return;
   ob->drew = EINA_TRUE;
   glsym_evas_gl_common_context_flush(ob->gl_context);
}

/**
 * @brief Flushes all rendering updates to the screen.
 *
 * This function finalizes the rendering for the current frame. It ensures
 * the GL context is current, marks the GL operations as done for the frame,
 * sets EGL swap interval (vsync), and then swaps the EGL buffers.
 * If partial updates are supported (via eglSwapBuffersWithDamage) and
 * `surface_damage` is provided, it attempts a partial swap.
 * Finally, it calls the internal DRM buffer swap logic.
 *
 * @param ob The output buffer (Outbuf) to flush.
 * @param surface_damage A list of Tilebuf_Rects defining areas damaged on the
 *                       surface, used for partial swaps if available.
 *                       May be NULL if no specific damage or for full swap.
 * @param buffer_damage A list of Tilebuf_Rects defining areas damaged in the
 *                      backbuffer (unused in this function).
 * @param render_mode The current Evas render mode. If EVAS_RENDER_MODE_ASYNC_INIT,
 *                    the function exits early.
 */
void
evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) goto end;

   if (!_re_wincheck(ob)) goto end;
   if (!ob->drew) goto end;

   ob->drew = EINA_FALSE;
   evas_outbuf_use(ob);
   glsym_evas_gl_common_context_done(ob->gl_context);

   if (!ob->vsync)
     {
        if (ob->info->info.vsync) eglSwapInterval(ob->egl.disp, 1);
        else eglSwapInterval(ob->egl.disp, 0);
        ob->vsync = 1;
     }

   /* if (ob->info->callback.pre_swap) */
   /*   ob->info->callback.pre_swap(ob->info->callback.data, ob->evas); */

   if ((glsym_eglSwapBuffersWithDamage) && (surface_damage) &&
       (ob->swap_mode != MODE_FULL))
     {
        EGLint num = 0, *result = NULL, i = 0;
        Tilebuf_Rect *r;

        // if partial swaps can be done use surface_damage
        num = eina_inlist_count(EINA_INLIST_GET(surface_damage));
        if (num > 0)
          {
             result = alloca(sizeof(EGLint) * 4 * num);
             EINA_INLIST_FOREACH(EINA_INLIST_GET(surface_damage), r)
               {
                  _glcoords_convert(&result[i], ob, r->x, r->y, r->w, r->h);
                  i += 4;
               }
             glsym_eglSwapBuffersWithDamage(ob->egl.disp, ob->egl.surface,
                                            result, num);
          }
     }
   else
      eglSwapBuffers(ob->egl.disp, ob->egl.surface);

   /* if (ob->info->callback.post_swap) */
   /*   ob->info->callback.post_swap(ob->info->callback.data, ob->evas); */

   _evas_outbuf_buffer_swap(ob);

end:
   //TODO: Need render unlock after drm page flip?
   glsym_evas_gl_preload_render_unlock(_evas_outbuf_make_current, ob);
}

/**
 * @brief Retrieves the Evas GL context associated with an output buffer.
 *
 * @param ob The output buffer (Outbuf).
 * @return A pointer to the Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context *
evas_outbuf_gl_context_get(Outbuf *ob)
{
   return ob->gl_context;
}

/**
 * @brief Retrieves the EGL display handle associated with an output buffer.
 *
 * @param ob The output buffer (Outbuf).
 * @return A void pointer to the EGLDisplay.
 */
void *
evas_outbuf_egl_display_get(Outbuf *ob)
{
   return ob->egl.disp;
}

/**
 * @brief Creates a new 3D graphics context (EGL context).
 *
 * This function creates an EGL context that is compatible with the
 * EGL configuration of the provided Outbuf and shares with the
 * Outbuf's main EGL context.
 *
 * @param ob The output buffer (Outbuf) whose EGL display and config are used.
 * @return A pointer to the newly created Context_3D structure, or NULL on failure.
 *         The Context_3D structure contains the EGLDisplay, EGLSurface, and
 *         EGLContext.
 */
Context_3D *
evas_outbuf_gl_context_new(Outbuf *ob)
{
   Context_3D *ctx;
   int context_attrs[3] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

   if (!ob) return NULL;

   ctx = calloc(1, sizeof(Context_3D));
   if (!ctx) return NULL;

   ctx->context = eglCreateContext(ob->egl.disp, ob->egl.config,
                                   ob->egl.context, context_attrs);

   if (!ctx->context)
     {
        ERR("EGL context creation failed.");
        goto error;
     }

   ctx->display = ob->egl.disp;
   ctx->surface = ob->egl.surface;

   return ctx;

error:
   free(ctx);
   return NULL;
}

/**
 * @brief Frees a 3D graphics context.
 *
 * Destroys the EGL context and frees the memory allocated for the
 * Context_3D structure.
 *
 * @param ctx The Context_3D to free.
 */
void
evas_outbuf_gl_context_free(Context_3D *ctx)
{
   eglDestroyContext(ctx->display, ctx->context);
   free(ctx);
}

/**
 * @brief Makes a 3D graphics context current for rendering.
 *
 * @param ctx The Context_3D to make current.
 */
void
evas_outbuf_gl_context_use(Context_3D *ctx)
{
   if (eglMakeCurrent(ctx->display, ctx->surface,
                      ctx->surface, ctx->context) == EGL_FALSE)
     ERR("eglMakeCurrent() failed.");
}
