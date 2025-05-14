#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"
#include "Evas_Engine_FB.h"

#include <Ecore.h>
#include <Eina.h>

/** @internal Log domain for the Evas FB engine. */
int _evas_engine_fb_log_dom = -1;

/** @internal List of active output buffers. */
static Eina_List *_outbufs = NULL;

/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;

/* engine struct data */
/**
 * @internal
 * @brief Typedef for the rendering engine data.
 * This engine uses the software_generic rendering capabilities.
 */
typedef Render_Output_Software_Generic Render_Engine;

/* engine api this module provides */
/**
 * @internal
 * @brief Sets up the output for the Evas FB engine.
 *
 * This function initializes the rendering engine, sets up the output buffer
 * using framebuffer-specific functions, and configures the software_generic
 * rendering engine with appropriate callbacks.
 *
 * @param engine The Evas generic engine pointer.
 * @param in Pointer to Evas_Engine_Info_FB containing setup information.
 * @param w The width of the output.
 * @param h The height of the output.
 * @return A pointer to the configured Render_Engine, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_FB *info = in;
   Render_Engine *re;
   Outbuf *ob;

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   evas_fb_outbuf_fb_init();

   /* get any stored performance metrics from device */
   ob = evas_fb_outbuf_fb_setup_fb(w, h,
                                   info->info.rotation,
                                   OUTBUF_DEPTH_INHERIT,
                                   info->info.virtual_terminal,
                                   info->info.device_number,
                                   info->info.refresh);
   if (!ob) goto on_error;

   if (!evas_render_engine_software_generic_init(engine, re, ob, NULL,
                                                 evas_fb_outbuf_fb_get_rot,
                                                 evas_fb_outbuf_fb_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_fb_outbuf_fb_new_region_for_update,
                                                 evas_fb_outbuf_fb_push_updated_region,
                                                 evas_fb_outbuf_fb_free_region_for_update,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 evas_fb_outbuf_fb_free,
                                                 evas_fb_outbuf_fb_get_width(ob),
                                                 evas_fb_outbuf_fb_get_height(ob)))
     goto on_error;

   /* no backbuf! */
   evas_fb_outbuf_fb_set_have_backbuf(ob, 0);
   _outbufs = eina_list_append(_outbufs, ob);
   return re;

 on_error:
   if (ob) evas_fb_outbuf_fb_free(ob);
   free(re);
   return NULL;
}

/**
 * @internal
 * @brief Sets up engine-specific information.
 *
 * This function is called to allow the engine to populate the
 * Evas_Engine_Info_FB structure. It sets the rendering mode to blocking.
 *
 * @param info Pointer to Evas_Engine_Info_FB to be filled.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_FB *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @internal
 * @brief Frees the output data associated with the engine.
 *
 * Cleans up and frees the Render_Engine structure, including removing the
 * output buffer from the list and cleaning the software_generic engine.
 *
 * @param engine The Evas generic engine pointer (unused in this function).
 * @param data Pointer to the Render_Engine data to be freed.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   if (re)
     {
        _outbufs = eina_list_remove(_outbufs, re->ob);
        evas_render_engine_software_generic_clean(engine, re);
        free(re);
     }
}

/**
 * @internal
 * @brief Gets the alpha channel support status for the canvas.
 *
 * Checks if the underlying framebuffer device supports transparency.
 *
 * @param data Pointer to the Render_Engine data.
 * @return EINA_TRUE if alpha is supported, EINA_FALSE otherwise.
 */
static Eina_Bool
eng_canvas_alpha_get(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   return (re->ob->priv.fb.fb->fb_var.transp.length > 0);
}

/* module advertising code */
/**
 * @internal
 * @brief Opens the Evas FB engine module.
 *
 * This function is called when the module is loaded. It inherits functions
 * from the "software_generic" engine, registers a log domain, and overrides
 * specific engine functions with FB-specific implementations.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic", sizeof (Evas_Engine_Info_FB))) return 0;
   _evas_engine_fb_log_dom = eina_log_domain_register
     ("evas-fb", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_fb_log_dom < 0)
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
   ORD(canvas_alpha_get);
   ORD(output_free);

   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

/**
 * @internal
 * @brief Closes the Evas FB engine module.
 *
 * This function is called when the module is unloaded. It unregisters the
 * log domain used by the engine.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_fb_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_fb_log_dom);
        _evas_engine_fb_log_dom = -1;
     }
}

/**
 * @internal
 * @brief Module API structure for the Evas FB engine.
 *
 * Defines the module API version, name, and entry points (module_open, module_close).
 */
static Evas_Module_Api evas_modapi =
{
  EVAS_MODULE_API_VERSION, /**< Evas module API version. */
  "fb",                    /**< Module name. */
  "none",                  /**< Module author/licence (not strictly enforced). */
  {
    module_open,           /**< Function to open the module. */
    module_close           /**< Function to close the module. */
  }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, fb);

#ifndef EVAS_STATIC_BUILD_FB
EVAS_EINA_MODULE_DEFINE(engine, fb);
#endif
