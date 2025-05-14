/**
 * @file
 * @brief Evas EGLFS engine header file.
 *
 * This header defines structures, function prototypes, and macros
 * used by the Evas EGLFS engine.
 */
#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include "evas_common_private.h"
# include "evas_macros.h"
# include "evas_private.h"
# include "Evas.h"
# include "Evas_Engine_Eglfs.h"

# define EGL_EGLEXT_PROTOTYPES
# define GL_GLEXT_PROTOTYPES

# include <EGL/egl.h>
# include <EGL/eglext.h>
# include <EGL/eglmesaext.h>
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>
# include <hwcomposer.h>
# include <hardware/hardware.h>
# include <hardware/hwcomposer.h>
# include "../gl_generic/Evas_Engine_GL_Generic.h"

extern int _evas_engine_eglfs_log_dom;
extern int _extn_have_buffer_age;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_eglfs_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_eglfs_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_eglfs_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_eglfs_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_eglfs_log_dom, __VA_ARGS__)

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

/**
 * @brief Represents a 3D rendering context (EGL).
 *
 * This structure holds the EGL display, context, and surface
 * required for 3D rendering operations.
 */
struct _Context_3D
{
   EGLDisplay display; /**< The EGL display connection. */
   EGLContext context; /**< The EGL rendering context. */
   EGLSurface surface; /**< The EGL drawing surface. */
};

/**
 * @brief Represents an output buffer for the EGLFS engine.
 *
 * This structure encapsulates all necessary information for managing
 * an EGL rendering target, including EGL context, surface, configuration,
 * dimensions, rotation, and swap behavior.
 */
struct _Outbuf
{
   Evas_Engine_Info_Eglfs *info; /**< Pointer to EGLFS engine specific information. */
   Evas_Engine_GL_Context *gl_context; /**< Pointer to the common Evas GL context. */

   Evas *evas; /**< Pointer to the Evas canvas; used for pre_swap, post_swap callbacks. */

   int w, h; /**< Width and height of the output buffer. */
   unsigned int rotation; /**< Rotation angle of the output (0, 90, 180, 270). */
   unsigned int depth;    /**< Color depth of the output buffer. */
   Render_Output_Swap_Mode swap_mode; /**< Swap mode (auto, full, copy, etc.). */

   struct
     {
        EGLContext context[1]; /**< Array holding the EGL context (typically one). */
        EGLSurface surface[1]; /**< Array holding the EGL surface (typically one). */
        EGLConfig config;      /**< The EGL configuration used for the context and surface. */
        EGLDisplay disp;       /**< The EGL display connection. */
     } egl; /**< EGL specific data. */

   struct
     {
        int prev_age;              /**< Previous buffer age for partial updates. */
        int frame_cnt;             /**< Frame counter. */
        // FIXME: Document curr, last, num if their purpose becomes clear or is standard.
        int curr, last, num;       /**< Buffer indices for multi-buffering schemes (e.g. triple buffering). */
        Eina_List *pending_writes; /**< List of pending write operations/damage rects. */
     } priv; /**< Private data for buffer management. */

   Eina_Bool destination_alpha : 1; /**< Flag: EINA_TRUE if the destination surface has an alpha channel. */
   Eina_Bool vsync : 1;             /**< Flag: EINA_TRUE if vsync is enabled. */
   Eina_Bool lost_back : 1;         /**< Flag: EINA_TRUE if the backbuffer content was lost and needs redraw. */
   Eina_Bool surf : 1;              /**< Flag: EINA_TRUE if the EGL surface is valid/created. */
   Eina_Bool drew : 1;              /**< Flag: EINA_TRUE if any drawing occurred in the current frame. */
};

/**
 * @brief Creates a new output buffer.
 * @param info Pointer to EGLFS engine specific information.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param swap_mode The desired swap mode.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *evas_outbuf_new(Evas_Engine_Info_Eglfs *info, int w, int h, Render_Output_Swap_Mode swap_mode);

/**
 * @brief Frees an output buffer and its associated resources.
 * @param ob Pointer to the Outbuf to free.
 */
void evas_outbuf_free(Outbuf *ob);

/**
 * @brief Makes the EGL context of the output buffer current.
 * @param ob Pointer to the Outbuf.
 */
void evas_outbuf_use(Outbuf *ob);

/**
 * @brief Recreates the EGL surface for the output buffer if it was lost or not created.
 * @param ob Pointer to the Outbuf.
 */
void evas_outbuf_resurf(Outbuf *ob);

/**
 * @brief Destroys the EGL surface of the output buffer.
 * @param ob Pointer to the Outbuf.
 */
void evas_outbuf_unsurf(Outbuf *ob);

/**
 * @brief Reconfigures an existing output buffer with new parameters.
 * @param ob Pointer to the Outbuf to reconfigure.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation.
 * @param depth New color depth. (Note: Outbuf_Depth type seems to be an internal detail or typedef not shown here)
 */
void evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);

/**
 * @brief Gets the current buffer swap state/mode of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The current Render_Output_Swap_Mode.
 */
Render_Output_Swap_Mode evas_outbuf_buffer_state_get(Outbuf *ob);

/**
 * @brief Gets the rotation of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The rotation angle (0, 90, 180, 270).
 */
int evas_outbuf_rot_get(Outbuf *ob);

/**
 * @brief Checks if this is the first rectangle in an update region.
 * @param ob Pointer to the Outbuf.
 * @return EINA_TRUE if it's the first rectangle, EINA_FALSE otherwise.
 */
Eina_Bool evas_outbuf_update_region_first_rect(Outbuf *ob);

/**
 * @brief Creates a new update region.
 * @param ob Pointer to the Outbuf.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param[out] cx Clipped X-coordinate (if applicable).
 * @param[out] cy Clipped Y-coordinate (if applicable).
 * @param[out] cw Clipped width (if applicable).
 * @param[out] ch Clipped height (if applicable).
 * @return A pointer to an internal structure representing the update region, or NULL.
 */
void *evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an updated RGBA image region to the output buffer.
 * @param ob Pointer to the Outbuf.
 * @param update Pointer to the RGBA_Image containing the updated pixel data.
 * @param x X-coordinate of the update.
 * @param y Y-coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Flushes pending drawing operations to the display.
 * @param ob Pointer to the Outbuf.
 * @param surface_damage Array of Tilebuf_Rect indicating damaged regions on the surface.
 *                       Example: Tilebuf_Rect damages[] = {{0,0,w,h}, {0,0,0,0}};
 * @param buffer_damage Array of Tilebuf_Rect indicating damaged regions in the buffer (for partial swap).
 *                      Example: Tilebuf_Rect damages[] = {{0,0,w,h}, {0,0,0,0}};
 * @param render_mode The Evas_Render_Mode for this flush.
 */
void evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Gets the Evas_Engine_GL_Context associated with the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return Pointer to the Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context* evas_outbuf_gl_context_get(Outbuf *ob);

/**
 * @brief Gets the EGLDisplay associated with the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return A void pointer to the EGLDisplay.
 */
void *evas_outbuf_egl_display_get(Outbuf *ob);

/**
 * @brief Creates a new 3D context (EGL context and surface) for the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return Pointer to the newly created Context_3D, or NULL on failure.
 */
Context_3D *evas_outbuf_gl_context_new(Outbuf *ob);

/**
 * @brief Makes a given 3D context current.
 * @param ctx Pointer to the Context_3D to make current.
 */
void evas_outbuf_gl_context_use(Context_3D *ctx);

/**
 * @brief Creates a native window for HWComposer.
 * @return The created EGLNativeWindowType, or equivalent.
 */
EGLNativeWindowType create_hwcomposernativewindow(void);

/**
 * @brief Inline function to check and ensure the EGL surface of an Outbuf is valid.
 *
 * If the surface (`ob->surf`) is not valid (e.g., lost), this function attempts
 * to recreate it using `evas_outbuf_resurf()`. If recreation fails, it logs an error.
 * It also sets `ob->lost_back = 1` if the surface had to be recreated.
 *
 * @param ob Pointer to the Outbuf to check.
 * @return EINA_TRUE if the surface was already valid.
 * @return EINA_FALSE if the surface had to be recreated (even if successful) or if recreation failed.
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

extern unsigned int (*glsym_eglSwapBuffersWithDamage)(EGLDisplay a, void *b, const EGLint *d, EGLint c);

#endif
