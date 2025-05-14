/**
 * @file
 * @brief Evas DRM engine implementation.
 *
 * This file contains the core implementation of the Evas DRM engine,
 * integrating with Ecore_Drm2 for hardware interaction and leveraging
 * the software_generic engine for rendering operations.
 */
#include "evas_engine.h"
#include "../software_generic/evas_native_common.h"

/**
 * @brief Internal structure for the DRM render engine.
 *
 * This structure encapsulates the state of the DRM render engine,
 * including a generic software rendering component and a pointer
 * to the Ecore_Drm2_Device.
 */
typedef struct _Render_Engine
{
   Render_Output_Software_Generic generic; /**< Generic software rendering output information. */

   Ecore_Drm2_Device *dev; /**< Pointer to the Ecore DRM device. */
} Render_Engine;

/**
 * @brief Structure to hold native scanout handler and its data.
 *
 * This is used to manage callbacks for native surface scanout status changes.
 */
struct scanout_handle
{
   Evas_Native_Scanout_Handler handler; /**< The callback function for scanout events. */
   void *data; /**< User data to be passed to the scanout handler. */
};

static Evas_Func func, pfunc;

int _evas_engine_drm_log_dom; /**< Log domain for the Evas DRM engine. */

/**
 * @brief Sets up the output for the DRM engine.
 *
 * This function initializes the rendering engine, creates an output buffer (_outbuf),
 * and sets up the software_generic rendering pipeline.
 *
 * @param engine The Evas generic engine pointer.
 * @param einfo Pointer to Evas_Engine_Info_Drm containing DRM specific settings.
 * @param w Initial width of the output.
 * @param h Initial height of the output.
 * @return A pointer to the configured Render_Engine, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *einfo, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Drm *info = einfo;
   Render_Engine *re;
   Outbuf *ob;

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   ob = _outbuf_setup(info, w, h);
   if (!ob) goto err;

   re->dev = info->info.dev;

   if (!evas_render_engine_software_generic_init(engine, &re->generic, ob,
                                                 _outbuf_state_get,
                                                 _outbuf_rotation_get,
                                                 _outbuf_reconfigure,
                                                 NULL,
                                                 _outbuf_damage_region_set,
                                                 _outbuf_update_region_new,
                                                 _outbuf_update_region_push,
                                                 NULL,
                                                 NULL,
                                                 _outbuf_flush,
                                                 NULL,
                                                 _outbuf_free,
                                                 ob->w, ob->h))
     goto init_err;

   evas_render_engine_software_generic_merge_mode_set(&re->generic);

   re->generic.ob->info = einfo;

   return re;

init_err:
   evas_render_engine_software_generic_clean(engine, &re->generic);
err:
   free(re);
   return NULL;
}

/**
 * @brief Sets up engine-specific information.
 *
 * This function is called to allow the engine to configure
 * Evas_Engine_Info_Drm with default or required settings.
 * Currently, it sets the render_mode to EVAS_RENDER_MODE_BLOCKING.
 *
 * @param info Pointer to Evas_Engine_Info_Drm to be configured.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Drm *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Updates the output configuration.
 *
 * This function is called when the output parameters (like width, height,
 * rotation, or depth) change. It reconfigures the output buffer and
 * updates the software_generic engine.
 *
 * @param engine The Evas generic engine pointer (unused).
 * @param data Pointer to the Render_Engine.
 * @param einfo Pointer to Evas_Engine_Info_Drm containing new settings.
 * @param w New width of the output.
 * @param h New height of the output.
 * @return Always returns 1 to indicate success.
 */
static int
eng_output_update(void *engine EINA_UNUSED, void *data, void *einfo, unsigned int w, unsigned int h)
{
   Render_Engine *re = data;
   Evas_Engine_Info_Drm *info;

   info = (Evas_Engine_Info_Drm *)einfo;
   _outbuf_reconfigure(re->generic.ob, w, h,
                       info->info.rotation, info->info.depth);

   evas_render_engine_software_generic_update(&re->generic,
                                              re->generic.ob, w, h);

   return 1;
}

/**
 * @brief Frees the output resources.
 *
 * Cleans up the software_generic engine resources and frees the
 * Render_Engine structure.
 *
 * @param engine The Evas generic engine pointer.
 * @param data Pointer to the Render_Engine to be freed.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re = data;

   evas_render_engine_software_generic_clean(engine, &re->generic);
   free(re);
}

/**
 * @brief Imports a dmabuf as an Ecore_Drm2_Fb.
 *
 * This function takes dmabuf attributes and imports them to create
 * an Ecore_Drm2 framebuffer. It assumes a 32-bit depth/bpp for the import.
 *
 * @param dev The Ecore_Drm2_Device to import the framebuffer into.
 * @param attributes Pointer to dmabuf_attributes describing the buffer.
 *        Example:
 *        attributes->n_planes = 1;
 *        attributes->fd[0] = dmabuf_fd;
 *        attributes->stride[0] = width * 4;
 *        attributes->width = width;
 *        attributes->height = height;
 *        attributes->format = DRM_FORMAT_XRGB8888; (or other compatible format)
 * @return A pointer to the imported Ecore_Drm2_Fb, or NULL on failure.
 */
static Ecore_Drm2_Fb *
drm_import_simple_dmabuf(Ecore_Drm2_Device *dev, struct dmabuf_attributes *attributes)
{
   unsigned int stride[4] = { 0 };
   int dmabuf_fd[4] = { 0 };
   int i;

   for (i = 0; i < attributes->n_planes; i++)
     {
        stride[i] = attributes->stride[i];
        dmabuf_fd[i] = attributes->fd[i];
     }

   return ecore_drm2_fb_dmabuf_import(dev, attributes->width,
                                      attributes->height, 32, 32,
                                      attributes->format, stride,
                                      dmabuf_fd, attributes->n_planes);
}

/**
 * @brief Callback for Ecore_Drm2_Fb status changes.
 *
 * This function is invoked when the status of an Ecore_Drm2_Fb changes
 * (e.g., scanout on/off, plane assignment). It forwards these status
 * updates to the Evas_Native_Scanout_Handler if one is registered.
 *
 * @param fb The Ecore_Drm2_Fb whose status changed (unused).
 * @param status The new Ecore_Drm2_Fb_Status.
 * @param data User data, expected to be a struct scanout_handle.
 */
static void
_eng_fb_release(Ecore_Drm2_Fb *fb EINA_UNUSED, Ecore_Drm2_Fb_Status status, void *data)
{
   struct scanout_handle *sh;

   sh = data;
   if (status == ECORE_DRM2_FB_STATUS_DELETED)
     {
        free(sh);
        return;
     }

   if (!sh->handler) return;

   switch (status)
     {
      case ECORE_DRM2_FB_STATUS_SCANOUT_ON:
        sh->handler(sh->data, EVAS_NATIVE_SURFACE_STATUS_SCANOUT_ON);
        break;
      case ECORE_DRM2_FB_STATUS_SCANOUT_OFF:
        sh->handler(sh->data, EVAS_NATIVE_SURFACE_STATUS_SCANOUT_OFF);
        break;
      case ECORE_DRM2_FB_STATUS_PLANE_ASSIGN:
        sh->handler(sh->data, EVAS_NATIVE_SURFACE_STATUS_PLANE_ASSIGN);
        break;
      case ECORE_DRM2_FB_STATUS_PLANE_RELEASE:
        sh->handler(sh->data, EVAS_NATIVE_SURFACE_STATUS_PLANE_RELEASE);
        break;
      default:
        ERR("Unhandled framebuffer status");
     }
}

/**
 * @brief Assigns an Evas image (specifically a dmabuf native surface) to a DRM plane.
 *
 * This function attempts to import a dmabuf-backed Evas image as a DRM
 * framebuffer and then assign it to an available DRM plane on the output.
 * It also sets up a handler for framebuffer status changes.
 *
 * @param data Pointer to the Render_Engine.
 * @param image Pointer to the Evas RGBA_Image. The image must be a native
 *              surface of type EVAS_NATIVE_SURFACE_WL_DMABUF.
 * @param x The X coordinate for plane placement.
 * @param y The Y coordinate for plane placement.
 * @return A pointer to the Ecore_Drm2_Plane if assignment is successful,
 *         otherwise NULL. The returned plane handle is an opaque type
 *         for the caller.
 */
static void *
eng_image_plane_assign(void *data, void *image, int x, int y)
{
   Render_Engine *re;
   Outbuf *ob;
   RGBA_Image *img;
   Native *n;
   Ecore_Drm2_Fb *fb = NULL;
   Ecore_Drm2_Plane *plane = NULL;
   struct scanout_handle *g;

   EINA_SAFETY_ON_NULL_RETURN_VAL(image, NULL);

   re = (Render_Engine *)data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(re, NULL);

   ob = re->generic.ob;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ob, NULL);

   img = image;
   n = img->native.data;

   /* Perhaps implementable on other surface types, but we're
    * sticking to this one for now */
   if (n->ns.type != EVAS_NATIVE_SURFACE_WL_DMABUF) return NULL;

   fb = drm_import_simple_dmabuf(re->dev, &n->ns_data.wl_surface_dmabuf.attr);
   if (!fb) return NULL;

   g = calloc(1, sizeof(struct scanout_handle));
   if (!g) goto out;

   g->handler = n->ns.data.wl_dmabuf.scanout.handler;
   g->data = n->ns.data.wl_dmabuf.scanout.data;
   ecore_drm2_fb_status_handler_set(fb, _eng_fb_release, g);

   /* Fail or not, we're going to drop that fb and let refcounting get rid of
    * it later
    */
   plane = ecore_drm2_plane_assign(ob->priv.output, fb, x, y);

out:
   ecore_drm2_fb_discard(fb);
   return plane;
}

/**
 * @brief Releases a previously assigned DRM plane.
 *
 * @param data Pointer to the Render_Engine (unused).
 * @param image Pointer to the Evas RGBA_Image (unused).
 * @param plin The opaque plane handle returned by eng_image_plane_assign.
 */
static void
eng_image_plane_release(void *data EINA_UNUSED, void *image EINA_UNUSED, void *plin)
{
   Ecore_Drm2_Plane *plane = plin;

   ecore_drm2_plane_release(plane);
}

/**
 * @brief Opens/initializes the Evas DRM engine module.
 *
 * This function is the entry point for loading the DRM engine. It inherits
 * functions from the "software_generic" engine, sets up logging, initializes
 * Ecore, and overrides specific engine functions with DRM implementations.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   /* check for valid evas module */
   if (!em) return 0;

   /* try to inherit functions from software_generic engine */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic",
                                    sizeof(Evas_Engine_Info_Drm)))
     return 0;

   /* try to create eina logging domain */
   _evas_engine_drm_log_dom =
     eina_log_domain_register("evas-drm", EVAS_DEFAULT_LOG_COLOR);

   /* if we could not create a logging domain, error out */
   if (_evas_engine_drm_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }

   ecore_init();

   /* store parent functions */
   func = pfunc;

   /* override the methods we provide */
#define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_)
   ORD(output_info_setup);
   ORD(output_setup);
   ORD(output_update);
   ORD(output_free);
   ORD(image_plane_assign);
   ORD(image_plane_release);

   /* advertise our engine functions */
   em->functions = (void *)(&func);

   return 1;
}

/**
 * @brief Closes/deinitializes the Evas DRM engine module.
 *
 * This function is called when the engine module is unloaded.
 * It unregisters the logging domain and shuts down Ecore.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   /* unregister the eina log domain for this engine */
   if (_evas_engine_drm_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_drm_log_dom);
        _evas_engine_drm_log_dom = -1;
     }

   ecore_shutdown();
}

/**
 * @brief Evas module API structure for the DRM engine.
 *
 * This structure provides metadata and entry points (module_open, module_close)
 * for the Evas module system.
 */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION, /**< Evas module API version. */
   "drm",                   /**< Module name. */
   "none",                  /**< Module author/licence (conventionally "none" for core Evas modules). */
   { module_open, module_close } /**< Module open and close function pointers. */
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, drm);

#ifndef EVAS_STATIC_BUILD_DRM
EVAS_EINA_MODULE_DEFINE(engine, drm);
#endif
