#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"
#include "Evas_Engine_Software_DDraw.h"

/** @brief Legacy log domain identifier, potentially unused. */
int _evas_engine_soft_ddraw_log_dom = -1;
/** @brief Function table for this engine, inheriting from software_generic. */
static Evas_Func func;
/** @brief Function table of the parent engine (software_generic). */
static Evas_Func pfunc;

/* engine struct data */
typedef struct _Render_Engine Render_Engine;

/**
 * @brief Structure holding the private data for the Software DirectDraw render engine instance.
 *
 * Contains the generic software rendering data structure, which in turn holds
 * the specific DirectDraw output buffer (`Outbuf`).
 */
struct _Render_Engine
{
   Render_Output_Software_Generic generic; /**< Inherited generic software rendering data, including the Outbuf. */
};

/* log domain variable */
/** @brief Log domain identifier used by the DBG, INF, WRN, ERR, CRT macros. */
int _evas_log_dom_module = -1;

/* engine api this module provides */

/**
 * @brief Sets up the output rendering resources (DirectDraw surfaces) for a given canvas.
 *
 * This function is called by Evas core when creating or resizing the output window/surface.
 * It allocates the engine-specific data (`Render_Engine`), initializes the DirectDraw
 * output buffer (`Outbuf`) using information from `in`, and sets up the generic
 * software rendering callbacks.
 *
 * @param engine The Evas engine pointer (passed to generic init).
 * @param in Pointer to Evas_Engine_Info_Software_DDraw containing setup details
 *           like the target window handle (HWND) and fullscreen flag.
 * @param w The initial width of the canvas/output in pixels.
 * @param h The initial height of the canvas/output in pixels.
 * @return A pointer to the allocated and initialized Render_Engine structure on success, NULL on failure.
 *         This pointer is stored by Evas core and passed back in subsequent engine calls.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Software_DDraw *info = in;
   Render_Engine *re;
   Outbuf *ob;

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   evas_software_ddraw_outbuf_init();

   ob = evas_software_ddraw_outbuf_setup(w, h,
                                         info->info.rotation,
                                         info->info.window,
                                         info->info.fullscreen);
   if (!ob) goto on_error;

   if (!evas_render_engine_software_generic_init(engine, &re->generic, ob, NULL,
                                                 evas_software_ddraw_outbuf_rot_get,
                                                 evas_software_ddraw_outbuf_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_software_ddraw_outbuf_new_region_for_update,
                                                 evas_software_ddraw_outbuf_push_updated_region,
                                                 NULL,
                                                 evas_software_ddraw_outbuf_idle_flush,
                                                 evas_software_ddraw_outbuf_flush,
                                                 NULL,
                                                 evas_software_ddraw_outbuf_free,
                                                 w, h))
     goto on_error;

   return re;

 on_error:
   if (ob) evas_software_ddraw_outbuf_free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Configures engine-specific information based on the provided Evas_Engine_Info structure.
 *
 * This function is called by Evas core to allow the engine to specify its
 * capabilities or default settings. Here, it sets the render mode to blocking.
 *
 * @param info Pointer to Evas_Engine_Info_Software_DDraw. This function modifies
 *             fields within this structure.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Software_DDraw *einfo = info;

   /* This engine operates in blocking mode */
   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Frees the rendering engine output resources allocated in eng_output_setup.
 *
 * This function is called by Evas core when the canvas associated with this
 * engine instance is destroyed. It cleans up the generic software rendering
 * resources (which in turn frees the DirectDraw Outbuf) and frees the
 * Render_Engine structure itself.
 *
 * @param engine The Evas engine pointer (passed to generic clean).
 * @param data The private Render_Engine data pointer previously returned by eng_output_setup.
 */
static void
eng_output_free(void *engine, void *data)
{
   Render_Engine *re;

   if (!data) return;

   re = (Render_Engine *)data;
   evas_render_engine_software_generic_clean(engine, &re->generic);
   free(re);
}

/**
 * @brief Reports whether the canvas associated with this engine instance supports an alpha channel.
 *
 * This determines if the window itself can be semi-transparent. The DirectDraw
 * engine, as implemented here, does not support alpha blending at the window level.
 *
 * @param engine The Evas engine pointer (unused in this implementation).
 * @return EINA_TRUE if the canvas supports alpha, EINA_FALSE otherwise. Always returns EINA_FALSE here.
 */
static Eina_Bool
eng_canvas_alpha_get(void *engine EINA_UNUSED)
{
#warning "We need to handle window with alpha channel."
   return EINA_FALSE;
}

/* module advertising code */

/**
 * @brief Opens (initializes) the Software DirectDraw engine module.
 *
 * This function is called by the Evas module system when loading this engine.
 * It performs the following steps:
 * 1. Inherits the function table from the "software_generic" engine.
 * 2. Registers a specific log domain ("evas-software_ddraw") for this module.
 * 3. Overrides specific functions in the inherited table with the
 *    DirectDraw-specific implementations (eng_output_info_setup, eng_output_setup, etc.).
 * 4. Assigns the final function table to the Evas_Module structure.
 *
 * @param em Pointer to the Evas_Module structure representing this engine module.
 *           The `functions` member will be set by this function.
 * @return 1 on successful initialization, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic", sizeof (Evas_Engine_Info_Software_DDraw))) return 0;
   _evas_log_dom_module = eina_log_domain_register
     ("evas-software_ddraw", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_log_dom_module < 0)
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
 * @brief Closes (shuts down) the Software DirectDraw engine module.
 *
 * This function is called by the Evas module system when unloading this engine.
 * It unregisters the log domain that was registered in module_open.
 *
 * @param em Pointer to the Evas_Module structure (unused in this function).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_log_dom_module >= 0)
     {
        eina_log_domain_unregister(_evas_log_dom_module);
        _evas_log_dom_module = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "software_ddraw",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, software_ddraw);

#ifndef EVAS_STATIC_BUILD_SOFTWARE_DDRAW
EVAS_EINA_MODULE_DEFINE(engine, software_ddraw);
#endif
