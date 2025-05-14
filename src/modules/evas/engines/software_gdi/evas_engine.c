#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"
#include "Evas_Engine_Software_Gdi.h"

/** Log domain for the Evas Software GDI engine module. */
int _evas_engine_soft_gdi_log_dom = -1;
/* function tables - filled in later (func and parent func) */
/** Function table for this engine, inheriting from software_generic. */
static Evas_Func func;
/** Function table of the parent engine (software_generic). */
static Evas_Func pfunc;

/* engine struct data */
typedef struct _Render_Engine Render_Engine;

/**
 * @brief Structure holding the state for the Software GDI rendering engine instance.
 *
 * This primarily wraps the generic software rendering engine's data.
 */
struct _Render_Engine
{
   Render_Output_Software_Generic generic; /**< Generic software rendering engine data. */
};

/* engine api this module provides */
/**
 * @brief Sets up the engine info structure.
 * @param info Pointer to the Evas_Engine_Info_Software_Gdi structure to be filled.
 *
 * This function is called by Evas core to get information about the engine's
 * capabilities and requirements before setting up an output. It sets the
 * default render mode.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Software_Gdi *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Sets up the rendering engine for a specific output window.
 * @param engine The Evas engine pointer (unused in this function).
 * @param in Pointer to the Evas_Engine_Info_Software_Gdi structure containing setup parameters.
 * @param w Initial width of the output surface.
 * @param h Initial height of the output surface.
 * @return A pointer to the newly allocated Render_Engine structure on success, NULL on failure.
 *
 * This function initializes the GDI output buffer (`Outbuf`) and the generic
 * software rendering engine, linking them together.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Software_Gdi *info = in;
   Render_Engine *re;
   Outbuf *ob;

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   evas_software_gdi_outbuf_init();

   if (w <= 0)
     w = 1;
   if (h <= 0)
     h = 1;

   ob = evas_software_gdi_outbuf_setup(w, h,
                                       info->info.rotation,
                                       info->info.window,
                                       info->info.borderless,
                                       info->info.fullscreen,
                                       info->info.region,
                                       0, 0);
   if (!ob) goto on_error;

   if (!evas_render_engine_software_generic_init(engine, &re->generic, ob, NULL,
                                                 evas_software_gdi_outbuf_rot_get,
                                                 evas_software_gdi_outbuf_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_software_gdi_outbuf_new_region_for_update,
                                                 evas_software_gdi_outbuf_push_updated_region,
                                                 NULL,
                                                 evas_software_gdi_outbuf_idle_flush,
                                                 evas_software_gdi_outbuf_flush,
                                                 NULL,
                                                 evas_software_gdi_outbuf_free,
                                                 w, h))
     goto on_error;

   return re;

 on_error:
   if (ob) evas_software_gdi_outbuf_free(ob);
   free(re);
   return NULL;
}

static int
eng_output_update(void *engine EINA_UNUSED, void *data, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Software_Gdi *info;
   Render_Engine *re = data;
   Outbuf *ob;
   int ponebuf = 0;

   info = (Evas_Engine_Info_Software_Gdi *)in;
   ponebuf = re->generic.ob->onebuf;

   ob = evas_software_gdi_outbuf_setup(w,
                                       h,
                                       info->info.rotation,
                                       info->info.window,
                                       info->info.borderless,
                                       info->info.fullscreen,
                                       info->info.region,
                                       0, 0);
   if (!ob) return 0;

   evas_render_engine_software_generic_update(&re->generic, ob, w, h);
   re->generic.ob->onebuf = ponebuf;

   return 1;
}

static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   if (!data) return;

   re = (Render_Engine *)data;
   evas_render_engine_software_generic_clean(engine, &re->generic);
   free(re);
}

static Eina_Bool
eng_canvas_alpha_get(void *engine EINA_UNUSED)
{
#warning "We need to handle window with alpha channel."
   return EINA_FALSE;
}

/* module advertising code */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic", sizeof (Evas_Engine_Info_Software_Gdi))) return 0;

   _evas_engine_soft_gdi_log_dom = eina_log_domain_register
     ("evas-software_gdi", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_soft_gdi_log_dom < 0)
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

   /* now advertise out own api */
   em->functions = (void *)(&func);
   return 1;
}

static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_soft_gdi_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_soft_gdi_log_dom);
        _evas_engine_soft_gdi_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "software_gdi",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, software_gdi);

#ifndef EVAS_STATIC_BUILD_SOFTWARE_GDI
EVAS_EINA_MODULE_DEFINE(engine, software_gdi);
#endif
