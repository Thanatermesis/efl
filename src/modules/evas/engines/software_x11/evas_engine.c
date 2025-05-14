#include "evas_common_private.h"
#include "evas_private.h"

#include "Evas_Engine_Software_X11.h"
#include "evas_engine.h"

#include "evas_xlib_outbuf.h"
#include "evas_xlib_buffer.h"
#include "evas_xlib_swapbuf.h"
#include "evas_xlib_color.h"
#include "evas_xlib_image.h"
#include "evas_xlib_dri_image.h"

#include "../software_generic/evas_native_common.h"

#ifdef HAVE_DLSYM
# include <dlfcn.h>
#endif

#include <Ecore.h>
#include <Eina.h>

static Evas_Native_Tbm_Surface_Image_Set_Call glsym__evas_native_tbm_surface_image_set = NULL;
static Evas_Native_Tbm_Surface_Stride_Get_Call glsym__evas_native_tbm_surface_stride_get = NULL;
int _evas_engine_soft_x11_log_dom = -1;

/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;

/* engine struct data */
typedef struct _Render_Engine Render_Engine;

/**
 * @brief Structure holding the state for a software_x11 render engine instance.
 *
 * Contains the generic software rendering data and specific function pointers
 * or data relevant to the X11 backend (like EGL info, though potentially unused here).
 */
struct _Render_Engine
{
   Render_Output_Software_Generic generic; /**< Generic software rendering data. */
   Eina_Bool                      (*outbuf_alpha_get)(Outbuf *ob); /**< Function pointer to get alpha status of the output buffer. */

   struct
   {
      void *disp;    /**< EGL display connection (potentially unused). */
      void *config;  /**< EGL configuration (potentially unused). */
      void *surface; /**< EGL surface (potentially unused). */
   } egl;
};

/* prototypes we will use here */
static void        *_best_visual_get(void *connection, int screen);
static unsigned int _best_colormap_get(void *connection, int screen);
static int          _best_depth_get(void *connection, int screen);

/** @brief Global list tracking all active output buffers (_Outbuf). */
static Eina_List *_outbufs = NULL;

/* internal engine routines */
/**
 * @brief Sets up an output buffer using standard Xlib rendering.
 *
 * Initializes the necessary Xlib resources (GCs, etc.) and configures the
 * output buffer (`Outbuf`) for rendering directly via XPutImage or similar.
 * It also initializes the generic software rendering parts.
 *
 * @param engine The engine pointer (unused in generic init).
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @param rot Rotation angle (0, 90, 180, 270).
 * @param disp X11 Display connection.
 * @param draw Target X11 Drawable (Window or Pixmap).
 * @param vis X11 Visual to use.
 * @param cmap X11 Colormap to use.
 * @param depth Depth of the drawable.
 * @param debug Enable debugging features.
 * @param grayscale Hint for grayscale allocation (unused?).
 * @param max_colors Maximum colors for palette modes (unused?).
 * @param mask Optional shape mask Pixmap.
 * @param shape_dither Enable dithering for shape mask.
 * @param destination_alpha Indicates if the destination drawable supports alpha.
 * @return A pointer to the initialized Render_Engine structure, or NULL on failure.
 */
static void *
_output_xlib_setup(void *engine, int w, int h, int rot, Display *disp, Drawable draw,
                   Visual *vis, Colormap cmap, int depth, int debug,
                   int grayscale, int max_colors, Pixmap mask,
                   int shape_dither, int destination_alpha)
{
   Render_Engine *re;
   Outbuf *ob;

   if (!(re = calloc(1, sizeof(Render_Engine)))) return NULL;

   evas_software_xlib_x_init();
   evas_software_xlib_x_color_init();
   evas_software_xlib_outbuf_init();

   ob =
     evas_software_xlib_outbuf_setup_x(w, h, rot, OUTBUF_DEPTH_INHERIT, disp,
                                       draw, vis, cmap, depth, grayscale,
                                       max_colors, mask, shape_dither,
                                       destination_alpha);
   if (!ob) goto on_error;

   /* for updates return 1 big buffer, but only use portions of it, also cache
    * it and keepit around until an idle_flush */

   /* disable for now - i am hunting down why some expedite tests are slower,
    * as well as shaped stuff is broken and probable non-32bpp is broken as
    * convert funcs dont do the right thing
    *
    */
//   re->ob->onebuf = 1;

   evas_software_xlib_outbuf_debug_set(ob, debug);
   if (!evas_render_engine_software_generic_init(engine, &re->generic, ob, NULL,
                                                 evas_software_xlib_outbuf_get_rot,
                                                 evas_software_xlib_outbuf_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_software_xlib_outbuf_new_region_for_update,
                                                 evas_software_xlib_outbuf_push_updated_region,
                                                 NULL,
                                                 evas_software_xlib_outbuf_idle_flush,
                                                 evas_software_xlib_outbuf_flush,
                                                 NULL,
                                                 evas_software_xlib_outbuf_free,
                                                 w, h))
     goto on_error;

   evas_render_engine_software_generic_merge_mode_set(&re->generic);

   return re;

on_error:
   if (ob) evas_software_xlib_outbuf_free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Sets up an output buffer using DRI/SwapBuffers for rendering.
 *
 * Initializes the necessary Xlib resources and configures the output buffer
 * (`Outbuf`) for rendering using potentially hardware-accelerated buffer swapping
 * (e.g., via DRI). It also initializes the generic software rendering parts.
 *
 * @param engine The engine pointer (unused in generic init).
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @param rot Rotation angle (0, 90, 180, 270).
 * @param disp X11 Display connection.
 * @param draw Target X11 Drawable (Window or Pixmap).
 * @param vis X11 Visual to use.
 * @param cmap X11 Colormap to use.
 * @param depth Depth of the drawable.
 * @param debug Enable debugging features (unused by swapbuf setup itself).
 * @param grayscale Hint for grayscale allocation (unused?).
 * @param max_colors Maximum colors for palette modes (unused?).
 * @param mask Optional shape mask Pixmap.
 * @param shape_dither Enable dithering for shape mask.
 * @param destination_alpha Indicates if the destination drawable supports alpha.
 * @return A pointer to the initialized Render_Engine structure, or NULL on failure.
 */
static void *
_output_swapbuf_setup(void *engine, int w, int h, int rot, Display *disp, Drawable draw,
                      Visual *vis, Colormap cmap, int depth,
                      int debug EINA_UNUSED,
                      int grayscale, int max_colors, Pixmap mask,
                      int shape_dither, int destination_alpha)
{
   Render_Engine *re;
   Outbuf *ob;

   if (!(re = calloc(1, sizeof(Render_Engine)))) return NULL;

   evas_software_xlib_x_init();
   evas_software_xlib_x_color_init();
   evas_software_xlib_swapbuf_init();

   ob =
     evas_software_xlib_swapbuf_setup_x(w, h, rot, OUTBUF_DEPTH_INHERIT, disp,
                                        draw, vis, cmap, depth, grayscale,
                                        max_colors, mask, shape_dither,
                                        destination_alpha);
   if (!ob) goto on_error;

   if (!evas_render_engine_software_generic_init(engine, &re->generic, ob,
                                                 evas_software_xlib_swapbuf_buffer_state_get,
                                                 evas_software_xlib_swapbuf_get_rot,
                                                 evas_software_xlib_swapbuf_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_software_xlib_swapbuf_new_region_for_update,
                                                 evas_software_xlib_swapbuf_push_updated_region,
                                                 NULL,
                                                 evas_software_xlib_swapbuf_idle_flush,
                                                 evas_software_xlib_swapbuf_flush,
                                                 NULL,
                                                 evas_software_xlib_swapbuf_free,
                                                 w, h))
     goto on_error;
   return re;

on_error:
   if (ob) evas_software_xlib_swapbuf_free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Gets the default visual for a given screen.
 * @param connection The X11 display connection (Display *).
 * @param screen The screen number.
 * @return The default Visual * for the screen, or NULL if connection is invalid.
 */
static void *
_best_visual_get(void *connection, int screen)
{
   if (!connection) return NULL;

   return DefaultVisual((Display *)connection, screen);
}

/**
 * @brief Gets the default colormap for a given screen.
 * @param connection The X11 display connection (Display *).
 * @param screen The screen number.
 * @return The default Colormap for the screen, or 0 if connection is invalid.
 */
static unsigned int
_best_colormap_get(void *connection, int screen)
{
   if (!connection) return 0;

   return DefaultColormap((Display *)connection, screen);
}

/**
 * @brief Gets the default depth for a given screen.
 * @param connection The X11 display connection (Display *).
 * @param screen The screen number.
 * @return The default depth (int) for the screen, or 0 if connection is invalid.
 */
static int
_best_depth_get(void *connection, int screen)
{
   if (!connection) return 0;

   return DefaultDepth((Display *)connection, screen);
}

/**
 * @brief Dynamically links symbols needed from the generic software engine.
 *
 * This function uses dlsym to find symbols related to native surface handling
 * (specifically TBM surfaces) that are provided by the linked software_generic
 * engine module. It ensures these functions are available for use.
 * This is typically needed when Evas is not built statically.
 */
static void
_symbols(void)
{
   static int done = 0;

   if (done) return;

#define LINK2GENERIC(sym) \
  glsym_##sym = dlsym(RTLD_DEFAULT, #sym);

   // Get function pointer to native_common that is now provided through the link of SW_Generic.
   LINK2GENERIC(_evas_native_tbm_surface_image_set);
   LINK2GENERIC(_evas_native_tbm_surface_stride_get);

   done = 1;
}

/* engine api this module provides */
/**
 * @brief Sets up the engine info structure with software_x11 specific details.
 *
 * Populates the Evas_Engine_Info_Software_X11 structure with default values
 * and function pointers for querying display capabilities (visual, colormap, depth).
 *
 * @param info Pointer to the Evas_Engine_Info_Software_X11 structure to be filled.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Software_X11 *einfo = info;

   einfo->info.debug = 0;
   einfo->info.alloc_grayscale = 0;
   einfo->info.alloc_colors_max = 216;
   einfo->func.best_visual_get = _best_visual_get;
   einfo->func.best_colormap_get = _best_colormap_get;
   einfo->func.best_depth_get = _best_depth_get;
   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Sets up the rendering engine output for a given canvas configuration.
 *
 * Creates and initializes a Render_Engine instance based on the provided
 * Evas_Engine_Info_Software_X11 settings. It attempts to use the swapbuffer
 * (DRI) backend first, falling back to the standard Xlib backend if unavailable
 * or disabled via environment variable (EVAS_NO_DRI_SWAPBUF).
 *
 * @param engine The engine pointer (passed to backend setup).
 * @param in Pointer to the Evas_Engine_Info_Software_X11 structure containing setup parameters.
 * @param w Width of the canvas.
 * @param h Height of the canvas.
 * @return A pointer to the initialized Render_Engine structure, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Software_X11 *info = in;
   Render_Engine *re = NULL;
   static int try_swapbuf = -1;
   char *s;

   if (try_swapbuf == -1)
     {
        if ((s = getenv("EVAS_NO_DRI_SWAPBUF")) != NULL)
          {
             if (atoi(s) == 1) try_swapbuf = 0;
             else try_swapbuf = 1;
          }
        else try_swapbuf = 1;
     }

   if (try_swapbuf)
     re = _output_swapbuf_setup(engine, w, h,
                                info->info.rotation, info->info.connection,
                                info->info.drawable, info->info.visual,
                                info->info.colormap,
                                info->info.depth, info->info.debug,
                                info->info.alloc_grayscale,
                                info->info.alloc_colors_max,
                                info->info.mask, info->info.shape_dither,
                                info->info.destination_alpha);
   if (re) re->outbuf_alpha_get = evas_software_xlib_swapbuf_alpha_get;
   else
     {
        re = _output_xlib_setup(engine, w, h,
                                info->info.rotation, info->info.connection,
                                info->info.drawable, info->info.visual,
                                info->info.colormap,
                                info->info.depth, info->info.debug,
                                info->info.alloc_grayscale,
                                info->info.alloc_colors_max,
                                info->info.mask, info->info.shape_dither,
                                info->info.destination_alpha);
        re->outbuf_alpha_get = evas_software_xlib_outbuf_alpha_get;
     }

   _outbufs = eina_list_append(_outbufs, re->generic.ob);

   return re;
}

/**
 * @brief Updates the rendering engine output configuration.
 *
 * Reconfigures an existing Render_Engine instance, typically used when the
 * target window/drawable or its properties (size, rotation, etc.) change.
 * It determines the backend type (swapbuf or xlib) based on the existing
 * engine data and re-initializes the output buffer accordingly.
 *
 * @param engine The engine pointer (unused).
 * @param data Pointer to the existing Render_Engine structure.
 * @param in Pointer to the Evas_Engine_Info_Software_X11 structure containing the new setup parameters.
 * @param w New width of the canvas.
 * @param h New height of the canvas.
 * @return 1 on success, although the return value isn't strictly checked elsewhere. Failure might involve internal errors during setup.
 */
static int
eng_output_update(void *engine EINA_UNUSED, void *data, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Software_X11 *info = in;
   Render_Engine *re = data;
   Outbuf *ob = NULL;

   _outbufs = eina_list_remove(_outbufs, re->generic.ob);

   if (re->generic.outbuf_free == evas_software_xlib_swapbuf_free)
     {
        ob =
          evas_software_xlib_swapbuf_setup_x(w, h,
                                             info->info.rotation,
                                             OUTBUF_DEPTH_INHERIT,
                                             info->info.connection,
                                             info->info.drawable,
                                             info->info.visual,
                                             info->info.colormap,
                                             info->info.depth,
                                             info->info.alloc_grayscale,
                                             info->info.alloc_colors_max,
                                             info->info.mask,
                                             info->info.shape_dither,
                                             info->info.destination_alpha);
     }
   else
     {
        ob =
          evas_software_xlib_outbuf_setup_x(w, h,
                                            info->info.rotation,
                                            OUTBUF_DEPTH_INHERIT,
                                            info->info.connection,
                                            info->info.drawable,
                                            info->info.visual,
                                            info->info.colormap,
                                            info->info.depth,
                                            info->info.alloc_grayscale,
                                            info->info.alloc_colors_max,
                                            info->info.mask,
                                            info->info.shape_dither,
                                            info->info.destination_alpha);
        if (ob)
          evas_software_xlib_outbuf_debug_set(ob, info->info.debug);
     }

   if (ob)
     {
        evas_render_engine_software_generic_update(&re->generic, ob, w, h);
     }

   _outbufs = eina_list_append(_outbufs, re->generic.ob);

   return 1;
}

/**
 * @brief Frees the resources associated with a rendering engine instance.
 *
 * Cleans up the generic software engine parts and frees the Render_Engine structure.
 * It also removes the associated output buffer from the global list.
 *
 * @param engine The engine pointer (passed to generic clean).
 * @param data Pointer to the Render_Engine structure to be freed.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   if ((re = (Render_Engine *)data))
     {
        _outbufs = eina_list_remove(_outbufs, re->generic.ob);
        evas_render_engine_software_generic_clean(engine, &re->generic);
        free(re);
     }
}

/**
 * @brief Checks if the canvas associated with the engine supports an alpha channel.
 *
 * Determines alpha support based on the output buffer's properties
 * (destination_alpha flag or the result of the outbuf_alpha_get function).
 *
 * @param engine Pointer to the Render_Engine structure.
 * @return EINA_TRUE if alpha is supported, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_canvas_alpha_get(void *engine)
{
   Render_Engine *re;

   re = (Render_Engine *)engine;
   return (re->generic.ob->priv.destination_alpha) ||
          (re->outbuf_alpha_get(re->generic.ob));
}

/**
 * @brief Frees resources associated with an EvasGL native surface wrapper.
 *
 * Cleans up the Native structure allocated when setting an EvasGL surface
 * as the source for an RGBA_Image.
 *
 * @param image Pointer to the RGBA_Image whose native data should be freed.
 */
static void
_native_evasgl_free(void *image)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;

   im->native.data = NULL;
   im->native.func.bind = NULL;
   im->native.func.unbind = NULL;
   im->native.func.free = NULL;
   //im->image.data         = NULL;
   free(n);
}

/**
 * @brief Initializes native surface support for a given type.
 *
 * Checks if the specified native surface type (X11, TBM, EvasGL) is supported
 * by this engine backend. For TBM, it might perform specific initialization.
 *
 * @param engine The engine pointer (unused).
 * @param type The Evas_Native_Surface_Type to initialize.
 * @return 1 if supported/initialized successfully, 0 otherwise.
 */
static int
eng_image_native_init(void *engine EINA_UNUSED, Evas_Native_Surface_Type type)
{
   switch (type)
     {
#ifdef GL_GLES
      case EVAS_NATIVE_SURFACE_TBM:
        return _evas_native_tbm_init();

#endif
      case EVAS_NATIVE_SURFACE_X11:
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
 * Performs any necessary cleanup for the specified native surface type.
 * For TBM, it might call a specific shutdown function.
 *
 * @param engine The engine pointer (unused).
 * @param type The Evas_Native_Surface_Type to shut down.
 */
static void
eng_image_native_shutdown(void *engine EINA_UNUSED, Evas_Native_Surface_Type type)
{
   switch (type)
     {
#ifdef GL_GLES
      case EVAS_NATIVE_SURFACE_TBM:
        _evas_native_tbm_shutdown();
        return;

#endif
      case EVAS_NATIVE_SURFACE_X11:
      case EVAS_NATIVE_SURFACE_OPENGL:
        return;

      default:
        ERR("Native surface type %d not supported!", type);
        return;
     }
}

/**
 * @brief Sets a native surface as the data source for an Evas image.
 *
 * Associates an Evas image (RGBA_Image/Image_Entry) with a native surface
 * (X11 Pixmap, TBM buffer, EvasGL surface). This allows Evas to potentially
 * use the native surface directly for rendering or texture uploading, avoiding
 * copies where possible. It handles different surface types and may involve
 * cache lookups or specific backend functions (like DRI or TBM setters).
 *
 * @param engine Pointer to the Render_Engine structure.
 * @param image Pointer to the Evas image (Image_Entry/RGBA_Image).
 * @param native Pointer to the Evas_Native_Surface structure describing the native source. If NULL, disassociates any existing native surface.
 * @return A pointer to the (potentially new) Evas image (Image_Entry) associated with the native surface, or NULL on failure or if native is NULL.
 */
static void *
eng_image_native_set(void *engine, void *image, void *native)
{
   Render_Engine *re = (Render_Engine *)engine;
   Evas_Native_Surface *ns = native;
   Image_Entry *ie = image, *ie2 = NULL;
   RGBA_Image *im = image;
   int stride;

   if (!im) return NULL;
   if (!ns)
     {
        if (im->native.data && im->native.func.free)
          im->native.func.free(im);
        return NULL;
     }

   if (ns->type == EVAS_NATIVE_SURFACE_X11)
     {
        if (im->native.data)
          {
             //image have native surface already
             Evas_Native_Surface *ens = im->native.data;

             if ((ens->type == ns->type) &&
                 (ens->data.x11.visual == ns->data.x11.visual) &&
                 (ens->data.x11.pixmap == ns->data.x11.pixmap))
               return im;
          }
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
     {
        if (im->native.data)
          {
             //image have native surface already
             Evas_Native_Surface *ens = im->native.data;

             if ((ens->type == ns->type) &&
                 (ens->data.tbm.buffer == ns->data.tbm.buffer))
               return im;
          }
     }

   // Code from software_generic
   if ((ns->type == EVAS_NATIVE_SURFACE_EVASGL) &&
       (ns->version == EVAS_NATIVE_SURFACE_VERSION))
     ie2 = evas_cache_image_data(evas_common_image_cache_get(),
                                 ie->w, ie->h, ns->data.evasgl.surface, 1,
                                 EVAS_COLORSPACE_ARGB8888);
   else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
     {
        stride = glsym__evas_native_tbm_surface_stride_get(re->generic.ob, ns);
        ie2 = evas_cache_image_copied_data(evas_common_image_cache_get(),
                                           stride, ie->h, NULL, ie->flags.alpha,
                                           EVAS_COLORSPACE_ARGB8888);
     }
   else
     ie2 = evas_cache_image_data(evas_common_image_cache_get(),
                                 ie->w, ie->h, NULL, ie->flags.alpha,
                                 EVAS_COLORSPACE_ARGB8888);

   if (im->native.data)
     {
        if (im->native.func.free)
          im->native.func.free(im);
     }

   evas_cache_image_drop(ie);
   ie = ie2;

   if (ns->type == EVAS_NATIVE_SURFACE_X11)
     {
        RGBA_Image *ret_im = NULL;
        ret_im = evas_xlib_image_dri_native_set(re->generic.ob, ie, ns);
        if (!ret_im)
          ret_im = evas_xlib_image_native_set(re->generic.ob, ie, ns);
        return ret_im;
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_TBM)
     {
        return glsym__evas_native_tbm_surface_image_set(re->generic.ob, ie, ns);
     }
   else if (ns->type == EVAS_NATIVE_SURFACE_EVASGL)
     {
        /* Native contains Evas_Native_Surface. What a mess. */
        Native *n = calloc(1, sizeof(Native));
        if (n)
          {
             n->ns_data.evasgl.surface = ns->data.evasgl.surface;
             im = (RGBA_Image *)ie;
             n->ns.type = EVAS_NATIVE_SURFACE_EVASGL;
             n->ns.version = EVAS_NATIVE_SURFACE_VERSION;
             n->ns.data.evasgl.surface = ns->data.evasgl.surface;
             im->native.data = n;
             im->native.func.free = _native_evasgl_free;
             im->native.func.bind = NULL;
             im->native.func.unbind = NULL;
          }
     }

   return ie;
}

/**
 * @brief Retrieves the native surface information associated with an Evas image.
 *
 * If an Evas image has been associated with a native surface via
 * eng_image_native_set(), this function returns a pointer to the
 * Evas_Native_Surface structure describing that association.
 *
 * @param engine The engine pointer (unused).
 * @param image Pointer to the Evas image (RGBA_Image).
 * @return A pointer to the associated Evas_Native_Surface structure, or NULL if no native surface is associated.
 */
static void *
eng_image_native_get(void *engine EINA_UNUSED, void *image)
{
   RGBA_Image *im = image;
   Native *n;
   if (!im) return NULL;
   n = im->native.data;
   if (!n) return NULL;
   return &(n->ns);
}

/* module advertising code */
/**
 * @brief Opens/initializes the software_x11 engine module.
 *
 * Inherits function pointers from the "software_generic" engine, registers
 * a log domain, overrides specific engine functions with the software_x11
 * implementations, dynamically links necessary symbols, and advertises the
 * engine's API.
 *
 * @param em Pointer to the Evas_Module structure for this engine.
 * @return 1 on successful initialization, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic", sizeof (Evas_Engine_Info_Software_X11))) return 0;

   _evas_engine_soft_x11_log_dom =
     eina_log_domain_register("evas-software_x11", EVAS_DEFAULT_LOG_COLOR);

   if (_evas_engine_soft_x11_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
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
   ORD(image_native_init);
   ORD(image_native_shutdown);
   ORD(image_native_set);
   ORD(image_native_get);

   _symbols();
   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

/**
 * @brief Closes/shuts down the software_x11 engine module.
 *
 * Unregisters the log domain associated with this engine.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_soft_x11_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_soft_x11_log_dom);
        _evas_engine_soft_x11_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, "software_x11", "none",
   {
      module_open,
      module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, software_x11);

#ifndef EVAS_STATIC_BUILD_SOFTWARE_X11
EVAS_EINA_MODULE_DEFINE(engine, software_x11);
#endif
