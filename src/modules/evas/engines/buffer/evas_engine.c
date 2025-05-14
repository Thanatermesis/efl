/**
 * @file
 * @brief Evas engine for rendering to a memory buffer.
 *
 * This engine allows Evas to render its canvas content directly into a
 * pre-allocated memory buffer. It's useful for scenarios where Evas is
 * used as an off-screen rendering tool or for integration with custom
 * display mechanisms.
 */

#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_engine.h"
#include "Evas_Engine_Buffer.h"

/* domain for eina_log */
/* the log macros are defined in evas_common_private.h */
/* theirs names are EVAS_ERR, EVAS_DBG, EVAS_CRIT, EVAS_WRN and EVAS_INF */
/* although we can use the EVAS_ERROR, etc... macros it will not work
   when the -fvisibility=hidden option is passed to gcc */

int _evas_engine_buffer_log_dom = -1;

/* function tables - filled in later (func and parent func) */

static Evas_Func func, pfunc;


/* engine struct data */
typedef Render_Output_Software_Generic Render_Engine;

/* prototypes we will use here */
static void eng_output_free(void *engine EINA_UNUSED, void *data);

/* engine api this module provides */

/**
 * @brief Sets up the output buffer for rendering.
 *
 * This function initializes the rendering engine with the provided buffer
 * information. It configures the output buffer (Outbuf) based on the
 * destination buffer details, color depth, and other parameters.
 *
 * @param engine The Evas engine instance.
 * @param in Pointer to Evas_Engine_Info_Buffer containing setup details.
 *        Example:
 *        info->info.dest_buffer = my_pixel_data;
 *        info->info.dest_buffer_row_bytes = my_width * 4; // For ARGB32
 *        info->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
 *        info->info.use_color_key = 0;
 *        info->info.alpha_threshold = 0;
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @return A pointer to the configured Render_Engine, or NULL on failure.
 */
static void *
eng_output_setup(void *engine, void *in, unsigned int w, unsigned int h)
{
   Evas_Engine_Info_Buffer *info = in;
   Outbuf *ob;
   Render_Engine *re;
   Outbuf_Depth dep;
   DATA32 color_key = 0;
   void *dest_buffer = info->info.dest_buffer;
   int dest_buffer_row_bytes = info->info.dest_buffer_row_bytes;
   int depth_type = info->info.depth_type;
   int use_color_key = info->info.use_color_key;
   int alpha_threshold = info->info.alpha_threshold;
   int color_key_r = info->info.color_key_r;
   int color_key_g = info->info.color_key_g;
   int color_key_b = info->info.color_key_b;

   re = calloc(1, sizeof(Render_Engine));
   if (!re) return NULL;

   evas_buffer_outbuf_buf_init();

   dep = OUTBUF_DEPTH_BGR_24BPP_888_888;
   if      (depth_type == EVAS_ENGINE_BUFFER_DEPTH_ARGB32)
     dep = OUTBUF_DEPTH_ARGB_32BPP_8888_8888;
   else if (depth_type == EVAS_ENGINE_BUFFER_DEPTH_RGB32)
     dep = OUTBUF_DEPTH_RGB_32BPP_888_8888;
   else if (depth_type == EVAS_ENGINE_BUFFER_DEPTH_BGRA32)
     dep = OUTBUF_DEPTH_BGRA_32BPP_8888_8888;
   else if (depth_type == EVAS_ENGINE_BUFFER_DEPTH_RGB24)
     dep = OUTBUF_DEPTH_RGB_24BPP_888_888;
   else if (depth_type == EVAS_ENGINE_BUFFER_DEPTH_BGR24)
     dep = OUTBUF_DEPTH_BGR_24BPP_888_888;
   R_VAL(&color_key) = color_key_r;
   G_VAL(&color_key) = color_key_g;
   B_VAL(&color_key) = color_key_b;
   A_VAL(&color_key) = 0;
   ob = evas_buffer_outbuf_buf_setup_fb(w,
                                        h,
                                        dep,
                                        dest_buffer,
                                        dest_buffer_row_bytes,
                                        use_color_key,
                                        color_key,
                                        alpha_threshold,
                                        info->info.func.new_update_region,
                                        info->info.func.free_update_region,
                                        info->info.func.switch_buffer,
                                        info->info.switch_data);
   if (!ob) goto on_error;

   if (!evas_render_engine_software_generic_init(engine, re, ob,
                                                 evas_buffer_outbuf_buf_swap_mode_get,
                                                 evas_buffer_outbuf_buf_rot_get,
                                                 evas_buffer_outbuf_reconfigure,
                                                 NULL,
                                                 NULL,
                                                 evas_buffer_outbuf_buf_new_region_for_update,
                                                 evas_buffer_outbuf_buf_push_updated_region,
                                                 evas_buffer_outbuf_buf_free_region_for_update,
                                                 NULL,
                                                 evas_buffer_outbuf_buf_switch_buffer,
                                                 NULL,
                                                 evas_buffer_outbuf_buf_free,
                                                 w, h))
     goto on_error;
   return re;

 on_error:
   if (ob) evas_buffer_outbuf_buf_free(ob);
   free(re);
   return NULL;
}

/**
 * @brief Sets up engine-specific information.
 *
 * This function configures properties of the Evas_Engine_Info_Buffer
 * structure that are specific to this buffer engine.
 *
 * @param info Pointer to Evas_Engine_Info_Buffer to be configured.
 *             The render_mode field will be set.
 */
static void
eng_output_info_setup(void *info)
{
   Evas_Engine_Info_Buffer *einfo = info;

   einfo->render_mode = EVAS_RENDER_MODE_BLOCKING;
}

/**
 * @brief Frees the resources associated with the output engine.
 *
 * Cleans up and deallocates the Render_Engine structure and any
 * associated resources, including the underlying generic software
 * rendering engine components.
 *
 * @param engine The Evas engine instance (unused in this specific function
 *               but part of the Evas_Func signature).
 * @param data Pointer to the Render_Engine data to be freed.
 */
static void
eng_output_free(void *engine EINA_UNUSED, void *data)
{
   Render_Engine *re;

   if ((re = (Render_Engine *)data))
     {
        evas_render_engine_software_generic_clean(engine, re);
        free(re);
     }
}

/**
 * @brief Gets the alpha channel state of the canvas.
 *
 * Determines if the canvas (specifically its back buffer) uses an
 * alpha channel.
 *
 * @param data Pointer to the Render_Engine data.
 * @return EINA_TRUE if the canvas has an alpha channel, EINA_FALSE otherwise.
 *         Returns EINA_TRUE if Render_Engine or its back buffer is NULL.
 */
static Eina_Bool
eng_canvas_alpha_get(void *data)
{
   Render_Engine *re;

   if ((re = (Render_Engine *)data))
     if (re->ob->priv.back_buf)
       return re->ob->priv.back_buf->cache_entry.flags.alpha;
   return EINA_TRUE;
}

/* module advertising code */

/**
 * @brief Opens and initializes the buffer engine module.
 *
 * This function is called when the Evas module is loaded. It inherits
 * functions from a parent "software_generic" engine, registers a log
 * domain, and overrides specific engine functions with implementations
 * from this buffer engine.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic", sizeof (Evas_Engine_Info_Buffer))) return 0;

   _evas_engine_buffer_log_dom = eina_log_domain_register
     ("evas-buffer", EINA_COLOR_BLUE);
   if (_evas_engine_buffer_log_dom < 0)
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
 * @brief Closes the buffer engine module.
 *
 * This function is called when the Evas module is unloaded.
 * It unregisters the log domain used by this engine.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_engine_buffer_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_engine_buffer_log_dom);
        _evas_engine_buffer_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "buffer",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, buffer);

#ifndef EVAS_STATIC_BUILD_BUFFER
EVAS_EINA_MODULE_DEFINE(engine, buffer);
#endif

