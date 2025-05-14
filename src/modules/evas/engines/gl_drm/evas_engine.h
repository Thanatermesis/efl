#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include "evas_common_private.h"
# include "evas_macros.h"
# include "evas_private.h"
# include "Evas.h"
# include "Evas_Engine_GL_Drm.h"

# include <Ecore.h>
# include <drm_fourcc.h>
# include <xf86drm.h>
# include <xf86drmMode.h>

# define EGL_EGLEXT_PROTOTYPES
# define GL_GLEXT_PROTOTYPES

# if !defined(HAVE_ECORE_X_XLIB) && !defined(EGL_NO_X11)
#  define EGL_NO_X11
# endif

# include <EGL/egl.h>
# include <EGL/eglext.h>
# include <EGL/eglmesaext.h>
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# include "../gl_generic/Evas_Engine_GL_Generic.h"

/** @brief Logging domain for the Evas GL DRM engine. */
extern int _evas_engine_gl_drm_log_dom;
/** @brief Flag indicating if the EGL_EXT_buffer_age extension is available and enabled.
 *  Set to 1 if available, 0 otherwise. Used for partial updates.
 */
extern int _extn_have_buffer_age;
/** @brief Flag indicating if the EGL_IMG_context_priority extension is available.
 *  Set to 1 if available, 0 otherwise.
 */
extern int _extn_have_context_priority;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_gl_drm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_gl_drm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_gl_drm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_gl_drm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_gl_drm_log_dom, __VA_ARGS__)

extern Evas_GL_Common_Context_New glsym_evas_gl_common_context_new;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done;
extern Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize;
extern Evas_GL_Common_Buffer_Dump_Call glsym_evas_gl_common_buffer_dump;
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock;
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock;
extern Evas_Gl_Symbols glsym_evas_gl_symbols;
extern void * (*glsym_eglGetProcAddress) (const char *a);
extern Evas_Gl_Extension_String_Check _ckext;

/**
 * @brief Main structure for the GL DRM rendering engine.
 * This structure holds generic GL rendering data and DRM device specific information.
 */
typedef struct _Render_Engine Render_Engine;
struct _Render_Engine
{
   Render_Output_GL_Generic generic; /**< Generic GL rendering data. */
   Ecore_Drm2_Device *dev; /**< Pointer to the Ecore DRM device. */
};

/**
 * @brief Structure representing a 3D rendering context (EGL).
 */
struct _Context_3D
{
   EGLDisplay display; /**< EGL display. */
   EGLContext context; /**< EGL context. */
   EGLSurface surface; /**< EGL surface. */
};

/**
 * @brief Structure representing an output buffer for rendering.
 * This structure holds all necessary information for managing and rendering to a DRM output.
 */
struct _Outbuf
{
   Evas_Engine_Info_GL_Drm *info; /**< Pointer to the Evas GL DRM engine information. */
   Evas_Engine_GL_Context *gl_context; /**< Pointer to the Evas GL context. */

   Ecore_Drm2_Device *dev; /**< Pointer to the Ecore DRM device. */

   int w, h, bpp; /**< Width, height, and bits per pixel of the output buffer. */
   unsigned int rotation, depth, format; /**< Rotation, depth, and format of the output buffer. */
   int prev_age; /**< Previous buffer age, used for damage tracking. */
   Render_Output_Swap_Mode swap_mode; /**< Swap mode for rendering (e.g., auto, copy, double, triple). */

   struct gbm_surface *surface; /**< Pointer to the GBM surface. */

   struct
     {
        EGLContext context; /**< EGL context for this output buffer. */
        EGLSurface surface; /**< EGL surface for this output buffer. */
        EGLConfig config;   /**< EGL configuration used. */
        EGLDisplay disp;    /**< EGL display used. */
     } egl;

   struct
     {
        Ecore_Drm2_Output *output; /**< Ecore DRM output associated with this buffer. */
        Ecore_Drm2_Plane *plane;   /**< Ecore DRM plane used by this buffer, if any. */
     } priv;

   Eina_Bool destination_alpha : 1; /**< Flag indicating if destination alpha is enabled. */
   Eina_Bool vsync : 1;             /**< Flag indicating if vsync is enabled. */
   Eina_Bool lost_back : 1;         /**< Flag indicating if the back buffer was lost. */
   Eina_Bool surf : 1;              /**< Flag indicating if the surface is valid. */
   Eina_Bool drew : 1;              /**< Flag indicating if drawing has occurred. */
};

/**
 * @brief Initializes the GBM (Generic Buffer Management) device.
 * @param info Pointer to the Evas GL DRM engine information.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool eng_gbm_init(Evas_Engine_Info_GL_Drm *info);

/**
 * @brief Shuts down the GBM device.
 * @param info Pointer to the Evas GL DRM engine information.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool eng_gbm_shutdown(Evas_Engine_Info_GL_Drm *info);

/**
 * @brief Creates a new output buffer.
 * @param info Pointer to the Evas GL DRM engine information.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param swap_mode The swap mode to use for rendering.
 * @return A pointer to the new Outbuf structure, or NULL on failure.
 */
Outbuf *evas_outbuf_new(Evas_Engine_Info_GL_Drm *info, int w, int h, Render_Output_Swap_Mode swap_mode);

/**
 * @brief Frees an output buffer.
 * @param ob Pointer to the Outbuf structure to free.
 */
void evas_outbuf_free(Outbuf *ob);

/**
 * @brief Makes the EGL context of the output buffer current.
 * @param ob Pointer to the Outbuf structure.
 */
void evas_outbuf_use(Outbuf *ob);

/**
 * @brief Recreates the EGL surface for the output buffer.
 * @param ob Pointer to the Outbuf structure.
 */
void evas_outbuf_resurf(Outbuf *ob);

/**
 * @brief Destroys the EGL surface of the output buffer.
 * @param ob Pointer to the Outbuf structure.
 */
void evas_outbuf_unsurf(Outbuf *ob);

/**
 * @brief Reconfigures the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @param w New width of the buffer.
 * @param h New height of the buffer.
 * @param rot New rotation of the buffer.
 * @param depth New depth of the buffer.
 */
void evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);

/**
 * @brief Gets the buffer state (swap mode) of the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @return The current swap mode.
 */
Render_Output_Swap_Mode evas_outbuf_buffer_state_get(Outbuf *ob);

/**
 * @brief Gets the rotation of the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @return The current rotation angle.
 */
int evas_outbuf_rot_get(Outbuf *ob);

/**
 * @brief Checks if this is the first rectangle in an update region (for partial updates).
 * @param ob Pointer to the Outbuf structure.
 * @return EINA_TRUE if it's the first rectangle, EINA_FALSE otherwise.
 */
Eina_Bool evas_outbuf_update_region_first_rect(Outbuf *ob);

/**
 * @brief Creates a new update region for rendering.
 * @param ob Pointer to the Outbuf structure.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param[out] cx Pointer to store the clipped X-coordinate.
 * @param[out] cy Pointer to store the clipped Y-coordinate.
 * @param[out] cw Pointer to store the clipped width.
 * @param[out] ch Pointer to store the clipped height.
 * @return A pointer to the update region data, or NULL on failure.
 */
void *evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an RGBA image update to the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @param update Pointer to the RGBA_Image containing the update.
 * @param x X-coordinate of the update.
 * @param y Y-coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Flushes the output buffer, performing a swap.
 * @param ob Pointer to the Outbuf structure.
 * @param surface_damage Pointer to a Tilebuf_Rect representing surface damage.
 * @param buffer_damage Pointer to a Tilebuf_Rect representing buffer damage.
 * @param render_mode The rendering mode.
 */
void evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Callback for releasing a framebuffer.
 * @param data User data (typically Outbuf).
 * @param fb Framebuffer data (typically gbm_bo).
 */
void evas_outbuf_release_fb(void *, void *);

/**
 * @brief Sets the damage region for the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @param damage Pointer to a Tilebuf_Rect representing the damage region.
 */
void evas_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage);

/**
 * @brief Gets the Evas GL context associated with the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @return A pointer to the Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context* evas_outbuf_gl_context_get(Outbuf *ob);

/**
 * @brief Gets the EGL display associated with the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @return A void pointer to the EGLDisplay.
 */
void *evas_outbuf_egl_display_get(Outbuf *ob);

/**
 * @brief Creates a new 3D context (EGL) for the output buffer.
 * @param ob Pointer to the Outbuf structure.
 * @return A pointer to the new Context_3D structure, or NULL on failure.
 */
Context_3D *evas_outbuf_gl_context_new(Outbuf *ob);

/**
 * @brief Makes the given 3D context current.
 * @param ctx Pointer to the Context_3D structure.
 */
void evas_outbuf_gl_context_use(Context_3D *ctx);

/**
 * @brief Loads EGL symbols and checks for extensions.
 * @param edsp The EGLDisplay to query for extensions.
 */
void eng_egl_symbols(EGLDisplay edsp);

/**
 * @brief Checks if the window surface for the output buffer is valid, recreates if not.
 * @param ob Pointer to the Outbuf structure.
 * @return EINA_TRUE if the surface is valid or successfully recreated, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_re_wincheck(Outbuf *ob)
{
   if (ob->surf) return EINA_TRUE;
   evas_outbuf_resurf(ob);
   ob->lost_back = 1;
   if (!ob->surf) ERR("GL engine can't re-create window surface!");
   return EINA_FALSE;
}

/**
 * @brief Retrieves the Outbuf structure from a Render_Engine structure.
 * @param re Pointer to the Render_Engine structure.
 * @return A pointer to the Outbuf structure.
 */
static inline Outbuf *
eng_get_ob(Render_Engine *re)
{
   return re->generic.software.ob;
}

/** @brief Function pointer for eglSwapBuffersWithDamageEXT, eglSwapBuffersWithDamageINTEL or eglSwapBuffersWithDamage. */
extern unsigned int (*glsym_eglSwapBuffersWithDamage)(EGLDisplay a, void *b, const EGLint *d, EGLint c);
/** @brief Function pointer for eglSetDamageRegionKHR. */
extern unsigned int (*glsym_eglSetDamageRegionKHR)(EGLDisplay a, EGLSurface b, EGLint *c, EGLint d);

#endif
