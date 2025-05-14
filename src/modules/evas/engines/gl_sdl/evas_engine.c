/**
 * @file evas_engine.c
 * @brief Evas GL SDL engine implementation.
 *
 * This file contains the Evas engine implementation for rendering using OpenGL
 * via SDL. It handles window creation, context management, and drawing
 * operations.
 */

#include "evas_common_private.h" /* Also includes international specific stuff */
#include "evas_private.h"
#include "evas_engine.h"

#ifndef _WIN32
# include <dlfcn.h>
#endif

#include <SDL2/SDL_opengl.h>

/** @brief Pointer to the Evas GL common context creation function. */
Evas_GL_Common_Context_New glsym_evas_gl_common_context_new = NULL;
/** @brief Pointer to the Evas GL common context free function. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free = NULL;
/** @brief Pointer to the Evas GL common context use function. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use = NULL;
/** @brief Pointer to the Evas GL common context flush function. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush = NULL;
/** @brief Pointer to the Evas GL common image unload function. */
Evas_GL_Common_Context_Call glsym_evas_gl_common_image_all_unload = NULL;
/** @brief Pointer to the Evas GL common context resize function. */
Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize = NULL;
/** @brief Pointer to the Evas GL preload render lock function. */
Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock = NULL;
/** @brief Pointer to the Evas GL symbols function. */
Evas_Gl_Symbols glsym_evas_gl_symbols = NULL;

/**
 * @brief Sets up the SDL output buffer.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @param fullscreen Non-zero if fullscreen, 0 otherwise.
 * @param noframe Non-zero if no frame, 0 otherwise.
 * @param info Pointer to Evas_Engine_Info_GL_SDL structure.
 * @return A pointer to the initialized Outbuf structure, or NULL on failure.
 */
static Outbuf *_sdl_output_setup(int w, int h, int fullscreen, int noframe, Evas_Engine_Info_GL_SDL *info);

/** @brief Log domain for the Evas GL SDL engine. */
int _evas_engine_GL_SDL_log_dom = -1;
/* function tables - filled in later (func and parent func) */
/** @brief Current engine functions. */
static Evas_Func func;
/** @brief Parent engine functions (gl_generic). */
static Evas_Func pfunc;

/**
 * @brief Reconfigures the output buffer.
 * @param ob The output buffer.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation.
 * @param depth The new depth.
 * @note This function is currently a no-op for this engine.
 */
static void
_outbuf_reconfigure(Outbuf *ob EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int rot EINA_UNUSED, Outbuf_Depth depth EINA_UNUSED)
{
}

/**
 * @brief Checks if there's a first rectangle in the output buffer's region.
 * @param ob The output buffer.
 * @return EINA_FALSE as this engine does not support this.
 */
static Eina_Bool
_outbuf_region_first_rect(Outbuf *ob EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Creates a new region for update.
 * @param ob The output buffer.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param cx Pointer to store the context X-coordinate (unused).
 * @param cy Pointer to store the context Y-coordinate (unused).
 * @param cw Pointer to store the context width (unused).
 * @param ch Pointer to store the context height (unused).
 * @return A pointer to the default surface of the GL context.
 */
static void *
_outbuf_new_region_for_update(Outbuf *ob,
                              int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED,
                              int *cx EINA_UNUSED, int *cy EINA_UNUSED, int *cw EINA_UNUSED, int *ch EINA_UNUSED)
{
   return ob->gl_context->def_surface;
}

/**
 * @brief Pushes an updated region to the output buffer.
 * @param ob The output buffer.
 * @param update The RGBA image data for the update.
 * @param x X-coordinate of the update.
 * @param y Y-coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 * @note This function is currently a no-op for this engine.
 */
static void
_outbuf_push_updated_region(Outbuf *ob EINA_UNUSED,
                            RGBA_Image *update EINA_UNUSED,
                            int x EINA_UNUSED, int y EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED)
{
}

/**
 * @brief Frees the output buffer.
 * @param ob The output buffer to free.
 */
static void
_outbuf_free(Outbuf *ob)
{
   evas_common_font_ext_clear();
   glsym_evas_gl_common_context_free(ob->gl_context);
}

/**
 * @brief Gets the rotation of the output buffer.
 * @param ob The output buffer.
 * @return Always returns 0 as rotation is not supported.
 */
static int
_outbuf_get_rot(Outbuf *ob EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Flushes the output buffer.
 * @param ob The output buffer.
 * @param surface_damage Regions of the surface that were damaged (unused).
 * @param buffer_damage Regions of the buffer that were damaged (unused).
 * @param render_mode The render mode (unused).
 * @note This swaps the SDL GL window buffers.
 */
static void
_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage EINA_UNUSED, Tilebuf_Rect *buffer_damage EINA_UNUSED, Evas_Render_Mode render_mode EINA_UNUSED)
{
   SDL_GL_SwapWindow(ob->window);
}

/**
 * @brief Makes the engine's window current for GL operations.
 * @param data Pointer to the Outbuf structure.
 * @param doit Unused parameter.
 * @return EINA_TRUE on success, EINA_FALSE otherwise (though always TRUE here).
 */
static Eina_Bool
eng_window_make_current(void *data, void *doit EINA_UNUSED)
{
   Outbuf *ob = data;

   SDL_GL_MakeCurrent(ob->window, ob->context);
   return EINA_TRUE;
}

/**
 * @brief Prepares the window for use by locking and flushing the GL context.
 * @param ob The output buffer associated with the window.
 */
static void
_window_use(Outbuf *ob)
{
   /* With SDL 1.x, only one window, no issue here so only flush evas context */
   glsym_evas_gl_preload_render_lock(eng_window_make_current, ob);

   if (ob)
     {
        glsym_evas_gl_common_context_use(ob->gl_context);
        glsym_evas_gl_common_context_flush(ob->gl_context);
     }
}

/**
 * @brief Gets the Evas GL context associated with the window.
 * @param ob The output buffer.
 * @return A pointer to the Evas_Engine_GL_Context.
 */
static Evas_Engine_GL_Context *
_window_gl_context_get(Outbuf *ob)
{
   return ob->gl_context;
}

/**
 * @brief Gets the EGL display associated with the window.
 * @param ob The output buffer.
 * @return A pointer to the EGLDisplay if using GLES, otherwise NULL.
 */
static void *
_window_egl_display_get(Outbuf *ob)
{
#ifdef GL_GLES
   return ob->egl_disp;
#else
   (void) ob;
   return NULL;
#endif
}

/**
 * @brief Structure for a 3D context.
 * Holds the output buffer and the SDL GL context.
 */
struct _Context_3D
{
   Outbuf *ob; /**< Pointer to the output buffer. */
   SDL_GLContext sdl_context; /**< The SDL GL context. */
};

/**
 * @brief Creates a new 3D GL context.
 * @param ob The output buffer for which to create the context.
 * @return A pointer to the new Context_3D, or NULL on failure.
 */
static Context_3D *
_window_gl_context_new(Outbuf *ob)
{
   Context_3D *ctx;

   ctx = calloc(1, sizeof (Context_3D));
   if (!ctx) return NULL;

   ctx->ob = ob;
   ctx->sdl_context = SDL_GL_CreateContext(ob->window);

   return ctx;
}

/**
 * @brief Makes the specified 3D GL context current.
 * @param ctx The 3D context to make current.
 */
static void
_window_gl_context_use(Context_3D *ctx)
{
   SDL_GL_MakeCurrent(ctx->ob->window, ctx->sdl_context);
}

/* FIXME: noway to destroy Context_3D */

/**
 * @brief Gets the EGL display for the EvasGL engine.
 * @param data Pointer to the Render_Engine structure.
 * @return A pointer to the EGLDisplay if using GLES and output buffer exists, otherwise NULL.
 */
static void *
evgl_eng_display_get(void *data)
{
   Render_Engine *re = data;

   if (!re->generic.software.ob) return NULL;
#ifdef GL_GLES
   return re->generic.software.ob->egl_disp;
#else
   return NULL; /* FIXME: what should we do here ? */
#endif
}

/**
 * @brief Gets the Evas surface (SDL window) for the EvasGL engine.
 * @param data Pointer to the Render_Engine structure.
 * @return A pointer to the SDL_Window.
 */
static void *
evgl_eng_evas_surface_get(void *data)
{
   Render_Engine *re = data;

   return re->generic.software.ob->window;
}

/**
 * @brief Makes the given surface and context current for EvasGL.
 * @param data Unused.
 * @param surface The surface (SDL_Window) to make current.
 * @param context The GL context to make current.
 * @param flush If non-zero, flushes the window context.
 * @return EINA_TRUE on success.
 */
static int
evgl_eng_make_current(void *data EINA_UNUSED,
                      void *surface, void *context,
                      int flush)
{
   if (flush) _window_use(NULL);
   SDL_GL_MakeCurrent(surface, context);
   return EINA_TRUE;
}

/**
 * @brief Creates a native window for EvasGL.
 * @param data Unused.
 * @return NULL, as this is not fully implemented for SDL.
 * @note FIXME: Needs proper implementation for SDL.
 */
static void *
evgl_eng_native_window_create(void *data EINA_UNUSED)
{
   /* FIXME: Need to understand how to implement that with SDL */
   return NULL;
   /* return SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, */
   /*                         2, 2, SDL_WINDOW_OPENGL); */
}

/**
 * @brief Destroys a native window for EvasGL.
 * @param data Unused.
 * @param native_window The native window to destroy (unused).
 * @return Always returns 1 (success).
 * @note FIXME: Needs proper implementation if native_window_create is implemented.
 */
static int
evgl_eng_native_window_destroy(void *data EINA_UNUSED, void *native_window EINA_UNUSED)
{
   /* SDL_DestroyWindow(native_window); */
   return 1;
}

/**
 * @brief Creates a window surface for EvasGL from a native window.
 * @param data Unused.
 * @param native_window The native window.
 * @return The native_window itself, as SDL_Window acts as the surface.
 */
static void *
evgl_eng_window_surface_create(void *data EINA_UNUSED, void *native_window)
{
   return native_window;
}

/**
 * @brief Destroys a window surface for EvasGL.
 * @param data Unused.
 * @param surface The surface to destroy (unused).
 * @return Always returns 1 (success).
 */
static int
evgl_eng_window_surface_destroy(void *data EINA_UNUSED,
                                void *surface EINA_UNUSED)
{
   return 1;
}

/**
 * @brief Creates a GL context for EvasGL.
 * @param data Pointer to the Render_Engine structure.
 * @param share_ctx Context to share resources with (unused).
 * @param version The GLES version requested. Currently only GLES 2.0 is supported.
 * @return A pointer to the created SDL_GLContext, or NULL on failure or unsupported version.
 */
static void *
evgl_eng_context_create(void *data, void *share_ctx EINA_UNUSED, Evas_GL_Context_Version version)
{
   Render_Engine *re = data;

   if (version != EVAS_GL_GLES_2_X)
     {
        ERR("This engine only supports OpenGL-ES 2.0 contexts for now!");
        return NULL;
     }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
   return SDL_GL_CreateContext(re->generic.software.ob->window);
}

/**
 * @brief Destroys a GL context for EvasGL.
 * @param data Unused.
 * @param context The GL context to destroy.
 * @return Always returns 1 (success).
 */
static int
evgl_eng_context_destroy(void *data EINA_UNUSED, void *context)
{
   SDL_GL_DeleteContext(context);
   return 1;
}

/**
 * @brief Gets the GL extension string.
 * @param data Unused.
 * @return The GL_EXTENSIONS string, or NULL if glGetString is not found.
 */
static const char *
evgl_eng_string_get(void *data EINA_UNUSED)
{
   const char *(*glGetString)(GLenum n);

   glGetString = SDL_GL_GetProcAddress("glGetString");
   if (glGetString) return glGetString(GL_EXTENSIONS);
   return NULL;
}

/**
 * @brief Gets the address of a GL/EGL extension function.
 * @param name The name of the function.
 * @return A pointer to the function, or NULL if not found.
 */
static void *
evgl_eng_proc_address_get(const char *name)
{
   return SDL_GL_GetProcAddress(name);
}

/**
 * @brief Gets the rotation angle of the EvasGL surface.
 * @param data Unused.
 * @return Always returns 0, as rotation is not supported by this backend.
 */
static int
evgl_eng_rotation_angle_get(void *data EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Interface functions for EvasGL.
 * This structure provides callbacks for EvasGL to interact with the
 * underlying GL implementation (SDL in this case).
 */
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
   NULL, // PBuffer create
   NULL, // PBuffer destroy
   NULL, // OpenGL-ES 1 (pixmap_surface_create)
   NULL, // OpenGL-ES 1 (pixmap_surface_destroy)
   NULL, // OpenGL-ES 1 (image_target_render_surface_set)
   NULL, // native_win_surface_config_get
};

/**
 * @brief Sets up the output for the Evas engine.
 * @param engine Pointer to the Evas engine (Render_Engine_GL_Generic).
 * @param in Pointer to Evas_Engine_Info_GL_SDL structure.
 * @param w The width of the output.
 * @param h The height of the output.
 * @return A pointer to the initialized Render_Engine structure, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Render_Engine *re = NULL;
   Outbuf *ob = NULL;
   Evas_Engine_Info_GL_SDL *info = in;

   ob = _sdl_output_setup(w, h,
                          info->flags.fullscreen,
                          info->flags.noframe,
                          info);
   if (!ob) goto on_error;

   re = calloc(1, sizeof (Render_Engine));
   if (!re) goto on_error;

   if (!evas_render_engine_gl_generic_init(engine, &re->generic, ob, NULL,
                                           _outbuf_get_rot,
                                           _outbuf_reconfigure,
                                           _outbuf_region_first_rect,
                                           NULL, // outbuf_damage_region_set - not used by gl_generic
                                           _outbuf_new_region_for_update,
                                           _outbuf_push_updated_region,
                                           NULL, // outbuf_idle_flush - not used by gl_generic
                                           NULL, // outbuf_free_region_for_update - not used by gl_generic
                                           _outbuf_flush,
                                           NULL, // outbuf_drawable_add - not used by gl_generic
                                           _outbuf_free,
                                           _window_use,
                                           _window_gl_context_get,
                                           _window_egl_display_get,
                                           _window_gl_context_new,
                                           _window_gl_context_use,
                                           &evgl_funcs,
                                           w, h))
     goto on_error;

   return re;

 on_error:
   if (ob) _outbuf_free(ob);
   free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Frees the output data for the Evas engine.
 * @param engine Pointer to the Evas engine (Render_Engine_GL_Generic).
 * @param data Pointer to the Render_Engine structure to free.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re = data;

   evas_render_engine_software_generic_clean(engine, &re->generic.software);
}

/**
 * @brief Dumps information about the engine's current state.
 * This includes cache dumps and unloading of images and fonts.
 * @param engine Pointer to the Evas engine (Render_Engine_GL_Generic).
 * @param data Pointer to the Render_Engine structure.
 */
static void
eng_output_dump(void *engine, void *data)
{
   Render_Engine *re;
   Render_Engine_GL_Generic *e = engine;

   re = (Render_Engine *)data;
   generic_cache_dump(e->software.surface_cache);
   evas_common_image_image_all_unload();
   evas_common_font_font_all_unload();
   glsym_evas_gl_common_image_all_unload(re->generic.software.ob->gl_context);
}

/**
 * @brief Gets the alpha channel state of the canvas.
 * @param data Unused.
 * @return EINA_FALSE, as this engine does not support canvas alpha.
 */
static Eina_Bool
eng_canvas_alpha_get(void *data EINA_UNUSED)
{
   return 0;
}

/**
 * @brief Loads GL symbols required by the engine.
 * Uses dlsym to dynamically link to GL common functions.
 * Also initializes Evas GL symbols.
 */
static void
gl_symbols(void)
{
   const char *exts = NULL;

#define LINK2GENERIC(sym)                       \
   glsym_##sym = dlsym(RTLD_DEFAULT, #sym);

   LINK2GENERIC(evas_gl_symbols);
   LINK2GENERIC(evas_gl_common_context_new);
   LINK2GENERIC(evas_gl_common_context_free);
   LINK2GENERIC(evas_gl_common_context_use);
   LINK2GENERIC(evas_gl_common_context_flush);
   LINK2GENERIC(evas_gl_common_image_all_unload);
   LINK2GENERIC(evas_gl_common_context_resize);
   LINK2GENERIC(evas_gl_preload_render_lock);

   // Find EGL extensions
   // FIXME: whgen above eglGetDisplay() is fixed... fix the below...
//   exts = eglQueryString(ob->egl_disp, EGL_EXTENSIONS);

   // Find EGL extensions
   glsym_evas_gl_symbols((void*)SDL_GL_GetProcAddress, exts);
}

/**
 * @brief Opens/initializes the Evas GL SDL engine module.
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "gl_generic", sizeof (Evas_Engine_Info_GL_SDL))) return 0;
   if (_evas_engine_GL_SDL_log_dom < 0)
     _evas_engine_GL_SDL_log_dom = eina_log_domain_register
       ("evas-gl_sdl", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_GL_SDL_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   /* store it for later use */
   func = pfunc;
   /* now to override methods */
   #define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(output_setup);
   ORD(canvas_alpha_get);
   ORD(output_free);
   ORD(output_dump);

   gl_symbols();

   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

/**
 * @brief Closes/deinitializes the Evas GL SDL engine module.
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_GL_SDL_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_GL_SDL_log_dom);
        _evas_engine_GL_SDL_log_dom = -1;
     }
}

/**
 * @brief Evas module API structure for the GL SDL engine.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "gl_sdl",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, gl_sdl);

#ifndef EVAS_STATIC_BUILD_GL_SDL
EVAS_EINA_MODULE_DEFINE(engine, gl_sdl);
#endif

/**
 * @brief Sets up the SDL output buffer and GL context.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @param fullscreen EINA_UNUSED: Non-zero if fullscreen, 0 otherwise.
 * @param noframe EINA_UNUSED: Non-zero if no frame, 0 otherwise.
 * @param info Pointer to Evas_Engine_Info_GL_SDL structure containing window and other info.
 * @return A pointer to the initialized Outbuf structure, or NULL on failure.
 *
 * This function initializes SDL GL attributes for a GLES 2.0 context,
 * creates an SDL GL context, and initializes the Evas common GL context.
 */
static Outbuf *
_sdl_output_setup(int w, int h, int fullscreen EINA_UNUSED, int noframe EINA_UNUSED, Evas_Engine_Info_GL_SDL *info)
{
   Outbuf *ob = NULL;
   const char *(*glGetString)(GLenum n);

   if (!info->window) return NULL;
   if (w <= 0) w = 640;
   if (h <= 0) h = 480;

   /* GL Initialization */
#ifdef HAVE_SDL_GL_CONTEXT_VERSION
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   ob = calloc(1, sizeof(Outbuf));
   if (!ob) return NULL;

   ob->window = info->window;
   ob->w = w;
   ob->h = h;
   ob->info = info;
   ob->context = SDL_GL_CreateContext(ob->window);
   if (!ob->context)
     {
        ERR("Impossible to create a context for : %p", info->window);
        goto on_error;
     }

   glGetString = SDL_GL_GetProcAddress("glGetString");

   INF("Vendor: '%s', Renderer: '%s', Version: '%s'",
     glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));

   ob->gl_context = glsym_evas_gl_common_context_new();
   if (!ob->gl_context) goto on_error;

   glsym_evas_gl_common_context_use(ob->gl_context);
   glsym_evas_gl_common_context_resize(ob->gl_context, w, h, ob->gl_context->rot);

   /* End GL Initialization */
   return ob;

 on_error:
   if (ob && ob->window) SDL_DestroyWindow(ob->window);
   free(ob);
   return NULL;
}
