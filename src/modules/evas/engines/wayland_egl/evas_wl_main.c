#include "evas_engine.h"

/* local function prototypes */

/* local variables */
static Outbuf *_evas_gl_wl_window = NULL;
static EGLContext context = EGL_NO_CONTEXT;
static struct wl_display *display = NULL;
static int win_count = 0;

/**
 * @brief Creates a new Wayland EGL window buffer (Outbuf).
 *
 * Initializes EGL for the Wayland display, chooses an appropriate EGL
 * configuration based on the requested parameters (depth, stencil, MSAA, alpha),
 * creates a shared EGL context if one doesn't exist, and sets up the
 * Evas GL common context. It also performs driver blacklisting checks.
 *
 * @param einfo Pointer to the Wayland engine info structure.
 * @param w Initial width of the window buffer.
 * @param h Initial height of the window buffer.
 * @param swap_mode The requested swap mode for rendering output.
 * @return A pointer to the newly created Outbuf structure, or NULL on failure.
 */
Outbuf *
eng_window_new(Evas_Engine_Info_Wayland *einfo, int w, int h, Render_Output_Swap_Mode swap_mode)
{
   Outbuf *gw;
   int context_attrs[3];
   int config_attrs[40];
   int major_version, minor_version;
   int num_config, n;
   const GLubyte *vendor, *renderer, *version;
   Eina_Bool blacklist = EINA_FALSE;
   struct wl_display *wl_disp;
   int val = 0;

   /* try to allocate space for our window */
   if (!(gw = calloc(1, sizeof(Outbuf))))
     return NULL;

   win_count++;
   gw->info = einfo;
   gw->w = w;
   gw->h = h;
   gw->swap_mode = swap_mode;
   gw->wl2_disp = ecore_wl2_window_display_get(einfo->info.wl2_win);
   gw->wl2_win = einfo->info.wl2_win;
   if (display && (display != ecore_wl2_display_get(gw->wl2_disp)))
     context = EGL_NO_CONTEXT;
   display = ecore_wl2_display_get(gw->wl2_disp);
   gw->depth = einfo->info.depth;
   gw->alpha = einfo->info.destination_alpha;
   gw->rot = einfo->info.rotation;
   gw->depth_bits = einfo->depth_bits;
   gw->stencil_bits = einfo->stencil_bits;
   gw->msaa_bits = einfo->msaa_bits;

   context_attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   context_attrs[1] = 2;
   context_attrs[2] = EGL_NONE;

   wl_disp = ecore_wl2_display_get(gw->wl2_disp);
   const char *s = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
   if (s && strstr(s, "EXT_platform_base"))
     {
        EGLDisplay (*func) (EGLenum platform, void *native_display, const EGLint *attrib_list);
        func = (void *)eglGetProcAddress("eglGetPlatformDisplayEXT");
        if (!func) goto noext;
        gw->egl_disp = func(EGL_PLATFORM_WAYLAND_EXT, wl_disp, NULL);
     }
   else
     {
noext:
        putenv("EGL_PLATFORM=wayland");
        gw->egl_disp = eglGetDisplay((EGLNativeDisplayType)wl_disp);
     }
   if (!gw->egl_disp)
     {
        ERR("eglGetDisplay() fail. code=%#x", eglGetError());
        eng_window_free(gw);
        return NULL;
     }
   if (!eglInitialize(gw->egl_disp, &major_version, &minor_version))
     {
        ERR("eglInitialize() fail. code=%#x", eglGetError());
        eng_window_free(gw);
        return NULL;
     }
   if (!eglBindAPI(EGL_OPENGL_ES_API))
     {
        ERR("eglBindAPI() fail. code=%#x", eglGetError());
        eng_window_free(gw);
        return NULL;
     }

try_again:
   n = 0;
   config_attrs[n++] = EGL_SURFACE_TYPE;
   config_attrs[n++] = EGL_WINDOW_BIT;
   config_attrs[n++] = EGL_RENDERABLE_TYPE;
   config_attrs[n++] = EGL_OPENGL_ES2_BIT;

   config_attrs[n++] = EGL_RED_SIZE;
   config_attrs[n++] = 1;
   config_attrs[n++] = EGL_GREEN_SIZE;
   config_attrs[n++] = 1;
   config_attrs[n++] = EGL_BLUE_SIZE;
   config_attrs[n++] = 1;

   config_attrs[n++] = EGL_ALPHA_SIZE;
   config_attrs[n++] = 8 * !!gw->alpha;
   config_attrs[n++] = EGL_DEPTH_SIZE;
   config_attrs[n++] = gw->depth_bits;
   config_attrs[n++] = EGL_STENCIL_SIZE;
   config_attrs[n++] = gw->stencil_bits;
   if (gw->msaa_bits > 0)
     {
        config_attrs[n++] = EGL_SAMPLE_BUFFERS;
        config_attrs[n++] = 1;
        config_attrs[n++] = EGL_SAMPLES;
        config_attrs[n++] = gw->msaa_bits;
     }
   config_attrs[n++] = EGL_NONE;

   num_config = 0;
   if (!eglChooseConfig(gw->egl_disp, config_attrs, &gw->egl_config,
                        1, &num_config) || (num_config != 1))
     {
        ERR("eglChooseConfig() fail. code=%#x", eglGetError());

        if ((gw->depth_bits > 24) || (gw->stencil_bits > 8))
          {
             WRN("Please note that your driver might not support 32-bit depth or "
                 "16-bit stencil buffers, so depth24, stencil8 are the maximum "
                 "recommended values.");
             if (gw->depth_bits > 24) gw->depth_bits = 24;
             if (gw->stencil_bits > 8) gw->stencil_bits = 8;
             DBG("Trying again with depth:%d, stencil:%d", gw->depth_bits, gw->stencil_bits);
             goto try_again;
          }
        else if (gw->msaa_bits)
          {
             gw->msaa_bits /= 2;
             DBG("Trying again with msaa_samples: %d", gw->msaa_bits);
             goto try_again;
          }
        else if (gw->depth_bits || gw->stencil_bits)
          {
             gw->depth_bits = 0;
             gw->stencil_bits = 0;
             DBG("Trying again without any depth or stencil buffer");
             goto try_again;
          }

        eng_window_free(gw);
        return NULL;
     }

   gw->egl_context =
     eglCreateContext(gw->egl_disp, gw->egl_config, context, context_attrs);
   if (gw->egl_context == EGL_NO_CONTEXT)
     {
        ERR("eglCreateContext() fail. code=%#x", eglGetError());
        eng_window_free(gw);
        return NULL;
     }

   if (context == EGL_NO_CONTEXT) context = gw->egl_context;
   if (eglMakeCurrent(gw->egl_disp, EGL_NO_SURFACE,
                 EGL_NO_SURFACE, gw->egl_context) == EGL_FALSE)
     {
        ERR("eglMakeCurrent() fail. code=%#x", eglGetError());
        eng_window_free(gw);
        return NULL;
     }
   vendor = glGetString(GL_VENDOR);
   renderer = glGetString(GL_RENDERER);
   version = glGetString(GL_VERSION);
   if (!vendor) vendor   = (unsigned char *)"-UNKNOWN-";
   if (!renderer) renderer = (unsigned char *)"-UNKNOWN-";
   if (!version) version  = (unsigned char *)"-UNKNOWN-";
   if (getenv("EVAS_GL_INFO"))
     {
        fprintf(stderr, "vendor: %s\n", vendor);
        fprintf(stderr, "renderer: %s\n", renderer);
        fprintf(stderr, "version: %s\n", version);
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
        eng_window_free(gw);
        return NULL;
     }

   eglGetConfigAttrib(gw->egl_disp, gw->egl_config, EGL_DEPTH_SIZE, &val);
   gw->detected.depth_buffer_size = val;
   DBG("Detected depth size %d", val);
   eglGetConfigAttrib(gw->egl_disp, gw->egl_config, EGL_STENCIL_SIZE, &val);
   gw->detected.stencil_buffer_size = val;
   DBG("Detected stencil size %d", val);
   eglGetConfigAttrib(gw->egl_disp, gw->egl_config, EGL_SAMPLES, &val);
   gw->detected.msaa = val;
   DBG("Detected msaa %d", val);

   if (!gw->gl_context)
     {
        eng_gl_symbols(gw->egl_disp);

        if (!(gw->gl_context = glsym_evas_gl_common_context_new()))
          {
             eng_window_free(gw);
             return NULL;
          }
        gw->gl_context->egldisp = gw->egl_disp;
        gw->gl_context->eglctxt = gw->egl_context;
        eng_window_use(gw);
     }
   if (w && h)
     eng_window_resurf(gw);
   gw->gl_context->msaa = (Eina_Bool) !!gw->msaa_bits;
   return gw;
}

/**
 * @brief Frees the resources associated with a Wayland EGL window buffer.
 *
 * Destroys the EGL surface, the Wayland EGL window, and the EGL context
 * (if it's not the shared context or if it's the last reference). It also
 * terminates the EGL display connection if this is the last window buffer
 * being freed.
 *
 * @param gw The Outbuf structure to free.
 */
void
eng_window_free(Outbuf *gw)
{
   int ref = 0;

   if (!gw) return;
   win_count--;
   eng_window_use(gw);

   if (gw == _evas_gl_wl_window) _evas_gl_wl_window = NULL;

   if (gw->gl_context)
     {
        ref = gw->gl_context->references - 1;
        glsym_evas_gl_common_context_free(gw->gl_context);
     }

   eglMakeCurrent(gw->egl_disp, EGL_NO_SURFACE,
                  EGL_NO_SURFACE, EGL_NO_CONTEXT);

   if (gw->egl_context != context)
     eglDestroyContext(gw->egl_disp, gw->egl_context);

   if (gw->egl_surface != EGL_NO_SURFACE)
     eglDestroySurface(gw->egl_disp, gw->egl_surface);

   if (gw->win) wl_egl_window_destroy(gw->win);

   if (ref == 0)
     {
        if (context) eglDestroyContext(gw->egl_disp, context);
        eglTerminate(gw->egl_disp);
        eglReleaseThread();
        context = EGL_NO_CONTEXT;
     }

   free(gw);
}

/**
 * @brief Makes the specified Wayland EGL window buffer current for rendering.
 *
 * Ensures that the EGL context and surface associated with the given Outbuf
 * are bound for subsequent GL operations. It handles context switching
 * if a different Outbuf was previously current. It also updates the
 * Evas GL common context state (viewport, etc.).
 *
 * @param gw The Outbuf structure to make current. If NULL, the current
 *           context is unbound.
 */
void
eng_window_use(Outbuf *gw)
{
   Eina_Bool force = EINA_FALSE;

   glsym_evas_gl_preload_render_lock(eng_preload_make_current, gw);
   if ((gw) && (!gw->gl_context)) return;

   if (_evas_gl_wl_window)
     {
        if (eglGetCurrentContext() != _evas_gl_wl_window->egl_context)
          force = EINA_TRUE;
     }

   if ((_evas_gl_wl_window != gw) || (force))
     {
        if (_evas_gl_wl_window)
          {
             glsym_evas_gl_common_context_use(_evas_gl_wl_window->gl_context);
             glsym_evas_gl_common_context_flush(_evas_gl_wl_window->gl_context);
          }

        _evas_gl_wl_window = gw;

        if (gw)
          {
             if (gw->egl_surface != EGL_NO_SURFACE)
               {
                  if (eglMakeCurrent(gw->egl_disp, gw->egl_surface,
                                     gw->egl_surface,
                                     gw->egl_context) == EGL_FALSE)
                    ERR("eglMakeCurrent() failed!");
               }
          }
     }

   if (gw)
     {
        glsym_evas_gl_common_context_use(gw->gl_context);
        glsym_evas_gl_common_context_resize(gw->gl_context, gw->w, gw->h, gw->rot);
     }
}

/**
 * @brief Destroys the EGL surface associated with the Outbuf.
 *
 * This function is typically called to release surface resources when the
 * window is hidden or not actively rendering, potentially saving memory.
 * The surface can be recreated later using eng_window_resurf().
 * This behavior is controlled by the EVAS_GL_WIN_RESURF environment variable.
 *
 * @param gw The Outbuf whose EGL surface should be destroyed.
 */
void
eng_window_unsurf(Outbuf *gw)
{
   if (!gw->surf) return;
   if (!getenv("EVAS_GL_WIN_RESURF")) return;
   if (getenv("EVAS_GL_INFO")) printf("unsurf %p\n", gw);

   if (_evas_gl_wl_window)
     glsym_evas_gl_common_context_flush(_evas_gl_wl_window->gl_context);

   if (_evas_gl_wl_window == gw)
     {
        eglMakeCurrent(gw->egl_disp, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (gw->egl_surface != EGL_NO_SURFACE)
          eglDestroySurface(gw->egl_disp, gw->egl_surface);
        gw->egl_surface = EGL_NO_SURFACE;

        _evas_gl_wl_window = NULL;
     }

   gw->surf = EINA_FALSE;
}

/**
 * @brief Recreates the EGL surface for the Outbuf.
 *
 * This function creates or recreates the Wayland EGL window (`wl_egl_window`)
 * if necessary, and then creates the EGL surface associated with it using the
 * current EGL configuration. It makes the new surface current. This is often
 * called after eng_window_unsurf() or during initial setup.
 *
 * @param gw The Outbuf for which to recreate the EGL surface.
 */
void
eng_window_resurf(Outbuf *gw)
{
   struct wl_surface *wls;

   if (gw->surf) return;
   if (getenv("EVAS_GL_INFO")) printf("resurf %p\n", gw);
   if ((!gw->w) || (!gw->h)) return;
   if (!gw->win)
     {
        wls = ecore_wl2_window_surface_get(gw->wl2_win);
        if ((gw->rot == 0) || (gw->rot == 180))
          gw->win = wl_egl_window_create(wls, gw->w, gw->h);
        else if ((gw->rot == 90) || (gw->rot == 270))
          gw->win = wl_egl_window_create(wls, gw->h, gw->w);
     }

   if (gw->egl_surface != EGL_NO_SURFACE)
     eglDestroySurface(gw->egl_disp, gw->egl_surface);
   gw->egl_surface =
     eglCreateWindowSurface(gw->egl_disp, gw->egl_config,
                            (EGLNativeWindowType)gw->win, NULL);
   if (gw->egl_surface == EGL_NO_SURFACE)
     {
        ERR("eglCreateWindowSurface() fail for %p. code=%#x",
            gw->win, eglGetError());
        return;
     }

   if (eglMakeCurrent(gw->egl_disp, gw->egl_surface,
                      gw->egl_surface, gw->egl_context) == EGL_FALSE)
     {
        ERR("eglMakeCurrent() fail. code=%#x", eglGetError());
        return;
     }

   gw->surf = EINA_TRUE;
}

/**
 * @brief Reconfigures the dimensions and rotation of an Outbuf.
 *
 * Updates the internal width, height, and rotation state of the Outbuf.
 * Resizes the underlying Wayland EGL window (`wl_egl_window`) and updates
 * the GL viewport via the Evas GL common context. Ensures the surface exists.
 *
 * @param ob The Outbuf to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation (0, 90, 180, 270).
 * @param depth Unused parameter.
 */
void
eng_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth EINA_UNUSED)
{
   ob->w = w;
   ob->h = h;
   ob->rot = rot;

   if (!ob->win)
     eng_window_resurf(ob);
   eng_window_use(ob);
   glsym_evas_gl_common_context_resize(ob->gl_context, w, h, rot);

   if (ob->win)
     {
        if ((ob->rot == 90) || (ob->rot == 270))
          wl_egl_window_resize(ob->win, h, w, 0, 0);
        else
          wl_egl_window_resize(ob->win, w, h, 0, 0);
     }
}

/**
 * @brief Gets the current rotation of the Outbuf.
 *
 * @param ob The Outbuf to query.
 * @return The rotation angle (0, 90, 180, or 270).
 */
int
eng_outbuf_rotation_get(Outbuf *ob)
{
   return ob->rot;
}

/**
 * @brief Determines the effective swap mode for the Outbuf.
 *
 * If the configured swap mode is MODE_AUTO and the EGL_EXT_buffer_age
 * extension is available, this function queries the buffer age to determine
 * the most appropriate swap mode (copy, double, triple, etc.). Otherwise,
 * it returns the configured swap mode.
 *
 * @param ob The Outbuf to query.
 * @return The effective Render_Output_Swap_Mode.
 */
Render_Output_Swap_Mode
eng_outbuf_swap_mode_get(Outbuf *ob)
{
   if ((ob->swap_mode == MODE_AUTO) && (extn_have_buffer_age))
     {
        Render_Output_Swap_Mode swap_mode;
        EGLint age = 0;

        eina_evlog("+gl_query_surf_swap_mode", ob, 0.0, NULL);
        if (!eglQuerySurface(ob->egl_disp, ob->egl_surface,
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
             snprintf(buf, sizeof(buf), "!%i", (int)age);
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

   return ob->swap_mode;
}

/**
 * @brief Prepares the Outbuf for rendering the first rectangle of a frame.
 *
 * Ensures the Outbuf's context is current, checks if the window surface is
 * valid, flushes any pending GL commands, and signals the start of a new
 * frame to the Evas GL common context.
 *
 * @param ob The Outbuf to prepare.
 * @return EINA_FALSE if preparation is successful, EINA_TRUE if the window
 *         check fails (e.g., no valid surface).
 */
Eina_Bool
eng_outbuf_region_first_rect(Outbuf *ob)
{
   glsym_evas_gl_preload_render_lock(eng_preload_make_current, ob);

   eng_window_use(ob);
   if (!_re_wincheck(ob)) return EINA_TRUE;

   glsym_evas_gl_common_context_flush(ob->gl_context);
   glsym_evas_gl_common_context_newframe(ob->gl_context);

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Converts Evas coordinates (origin top-left) to OpenGL coordinates
 *        (origin bottom-left), taking into account buffer rotation.
 *
 * @param result Output array of 4 integers to store the GL coordinates [x, y, w, h].
 *               Example: `int gl_coords[4];`
 * @param ob The Outbuf containing rotation and dimension information.
 * @param x Input X coordinate (Evas).
 * @param y Input Y coordinate (Evas).
 * @param w Input width (Evas).
 * @param h Input height (Evas).
 */
static void
_convert_glcoords(int *result, Outbuf *ob, int x, int y, int w, int h)
{
   switch (ob->rot)
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
 * @brief Sets the damage region hint for EGL partial updates.
 *
 * If the EGL_KHR_swap_buffers_with_damage extension is supported, this function
 * converts the provided list of Evas damage rectangles (Tilebuf_Rect) into
 * OpenGL coordinates and passes them to EGL via eglSetDamageRegionKHR.
 * This allows the driver to optimize buffer swaps by only updating the
 * specified regions.
 *
 * @param ob The Outbuf associated with the EGL surface.
 * @param damage A list of Tilebuf_Rect structures representing the damaged areas
 *               in Evas coordinates.
 */
void
eng_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage)
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
             _convert_glcoords(rect, ob, tr->x, tr->y, tr->w, tr->h);
             rect += 4;
          }
        glsym_eglSetDamageRegionKHR(ob->egl_disp, ob->egl_surface, rects, count);
     }
}

/**
 * @brief Sets up the GL context for rendering within a specific region.
 *
 * Configures the master clip rectangle in the Evas GL common context based on
 * the provided update region (x, y, w, h). If the update region covers the
 * entire Outbuf, clipping is disabled.
 *
 * @param ob The Outbuf being updated.
 * @param x The X coordinate of the update region.
 * @param y The Y coordinate of the update region.
 * @param w The width of the update region.
 * @param h The height of the update region.
 * @param cx Unused output parameter.
 * @param cy Unused output parameter.
 * @param cw Unused output parameter.
 * @param ch Unused output parameter.
 * @return A pointer to the default surface associated with the GL context
 *         (ob->gl_context->def_surface).
 */
void *
eng_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx EINA_UNUSED, int *cy EINA_UNUSED, int *cw EINA_UNUSED, int *ch EINA_UNUSED)
{
   if ((ob->w == w) && (ob->h == h))
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
 * @brief Pushes an updated region to the Outbuf, flushing GL commands.
 *
 * This function marks that drawing has occurred within the current update
 * region and flushes the pending GL command buffer for the Outbuf's context.
 * The actual image data parameter (`update`) is currently unused by this
 * specific backend function.
 *
 * @param ob The Outbuf being updated.
 * @param update Unused: The image data for the update.
 * @param x Unused: The X coordinate of the update region.
 * @param y Unused: The Y coordinate of the update region.
 * @param w Unused: The width of the update region.
 * @param h Unused: The height of the update region.
 */
void
eng_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update EINA_UNUSED, int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
   if (!_re_wincheck(ob)) return;
   ob->draw.drew = EINA_TRUE;
   glsym_evas_gl_common_context_flush(ob->gl_context);
}

/**
 * @brief Finalizes rendering for the current frame and swaps buffers.
 *
 * Marks the end of GL rendering for the frame, swaps the EGL front and back
 * buffers to display the result. It attaches the Wayland buffer and commits
 * the Wayland surface state. If damage regions are supported and provided
 * (`surface_damage`), it attempts to perform a partial buffer swap using
 * eglSwapBuffersWithDamage. Otherwise, performs a full eglSwapBuffers.
 * Handles render synchronization and Wayland display flushing.
 *
 * @param ob The Outbuf to flush.
 * @param surface_damage List of rectangles defining the area updated on the
 *                       surface (used for partial swaps).
 * @param buffer_damage Unused: List of rectangles defining damage within the buffer.
 * @param render_mode The current Evas render mode. Asynchronous modes might
 *                    skip the actual swap.
 */
void
eng_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode)
{
   if (render_mode == EVAS_RENDER_MODE_ASYNC_INIT) goto end;

   if (!_re_wincheck(ob)) goto end;
   if (!ob->draw.drew) goto end;

   ob->draw.drew = EINA_FALSE;
   eng_window_use(ob);
   glsym_evas_gl_common_context_done(ob->gl_context);
   eglSwapInterval(ob->egl_disp, 0);

   ecore_wl2_window_buffer_attach(ob->wl2_win, NULL, 0, 0, EINA_TRUE);
   ecore_wl2_window_commit(ob->wl2_win, EINA_FALSE);

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
                  _convert_glcoords(&result[i], ob, r->x, r->y, r->w, r->h);
                  i += 4;
               }
             glsym_eglSwapBuffersWithDamage(ob->egl_disp, ob->egl_surface,
                                            result, num);
          }
     }
   else
      eglSwapBuffers(ob->egl_disp, ob->egl_surface);

   ob->frame_cnt++;

 end:
   glsym_evas_gl_preload_render_unlock(eng_preload_make_current, ob);
   ecore_wl2_display_flush(ob->wl2_disp);
}

/**
 * @brief Gets the Evas GL common context associated with the Outbuf.
 *
 * @param ob The Outbuf.
 * @return A pointer to the Evas_Engine_GL_Context, or NULL if ob is NULL.
 */
Evas_Engine_GL_Context *
eng_outbuf_gl_context_get(Outbuf *ob)
{
   if (!ob) return NULL;

   return ob->gl_context;
}

/**
 * @brief Gets the EGL display associated with the Outbuf.
 *
 * @param ob The Outbuf.
 * @return The EGLDisplay handle.
 */
void *
eng_outbuf_egl_display_get(Outbuf *ob)
{
   return ob->egl_disp;
}

/**
 * @brief Creates a new EGL context for 3D rendering, sharing resources.
 *
 * Creates an EGL context that shares resources (like textures, shaders)
 * with the main EGL context used by the Outbuf (`ob->egl_context`). This
 * allows integrating external 3D rendering with Evas GL.
 *
 * @param ob The Outbuf whose main context will be shared.
 * @return A pointer to the newly created Context_3D structure, or NULL
 *         on failure.
 */
Context_3D *
eng_gl_context_new(Outbuf *ob)
{
   Context_3D *ctx;
   int attrs[3];

   if (!ob) return NULL;

   attrs[0] = EGL_CONTEXT_CLIENT_VERSION;
   attrs[1] = 2;
   attrs[2] = EGL_NONE;

   if (!(ctx = calloc(1, sizeof(Context_3D)))) return NULL;

   ctx->context =
     eglCreateContext(ob->egl_disp, ob->egl_config, ob->egl_context, attrs);
   if (!ctx->context)
     {
        ERR("Could not create egl context %#x", eglGetError());
        goto err;
     }

   ctx->display = ob->egl_disp;
   ctx->surface = ob->egl_surface;

   return ctx;

err:
   free(ctx);
   return NULL;
}

/**
 * @brief Frees a 3D EGL context created by eng_gl_context_new.
 *
 * @param ctx The Context_3D structure to free.
 */
void
eng_gl_context_free(Context_3D *ctx)
{
   eglDestroyContext(ctx->display, ctx->context);
   free(ctx);
}

/**
 * @brief Makes the specified 3D EGL context current for rendering.
 *
 * Binds the given 3D context (`ctx->context`) and its associated surface
 * (`ctx->surface`, which is typically the same as the Outbuf's surface)
 * to the current thread for GL operations.
 *
 * @param ctx The Context_3D structure to make current.
 */
void
eng_gl_context_use(Context_3D *ctx)
{
   if (eglMakeCurrent(ctx->display, ctx->surface,
                      ctx->surface, ctx->context) == EGL_FALSE)
     {
        ERR("eglMakeCurrent Failed: %#x", eglGetError());
     }
}
