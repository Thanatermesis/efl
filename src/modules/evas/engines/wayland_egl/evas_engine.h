/**
 * @file
 * @brief Evas Wayland EGL engine header definitions.
 *
 * This header defines structures, macros, and function prototypes used
 * internally by the Evas Wayland EGL rendering engine.
 */
#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include "config.h"
# include "evas_common_private.h"
# include "evas_private.h"
# include "Evas.h"
# include "Evas_Engine_Wayland.h"

/* NB: This already includes wayland-client.h */
# include <wayland-egl.h>

# define GL_GLEXT_PROTOTYPES

# if !defined(HAVE_ECORE_X_XLIB) && !defined(EGL_NO_X11)
#  define EGL_NO_X11
# endif

# include <EGL/egl.h>
# include <GLES2/gl2.h>
# include <GLES2/gl2ext.h>

# include "../gl_generic/Evas_Engine_GL_Generic.h"

extern int _evas_engine_wl_egl_log_dom;

# ifdef ERR
#  undef ERR
# endif
/** @brief Log an error message using the engine's log domain. */
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
/** @brief Log a debug message using the engine's log domain. */
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
/** @brief Log an informational message using the engine's log domain. */
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
/** @brief Log a warning message using the engine's log domain. */
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
/** @brief Log a critical message using the engine's log domain. */
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifndef EGL_BUFFER_AGE_EXT
/** @brief EGL extension definition for buffer age, if not already defined. */
#  define EGL_BUFFER_AGE_EXT 0x313d
# endif

/**
 * @brief Structure representing the output buffer and associated Wayland/EGL resources.
 *
 * This structure holds all the necessary information for rendering to a Wayland
 * surface using EGL, including display connections, window handles, EGL context,
 * surface dimensions, and rendering state.
 */
struct _Outbuf
{
   Ecore_Wl2_Display *wl2_disp; /**< Ecore Wayland display connection. */
   struct wl_egl_window *win; /**< Wayland EGL window wrapper. */
   Ecore_Wl2_Window *wl2_win; /**< Ecore Wayland window handle. */
   int w, h; /**< Width and height of the output buffer. */
   int depth, screen, rot, alpha; /**< Output properties: depth, screen (unused?), rotation, alpha enabled. */

   Evas_Engine_Info_Wayland *info; /**< Pointer to the engine setup information. */
   Evas_Engine_GL_Context *gl_context; /**< Evas GL context associated with this output buffer. */

   int prev_age; /**< Previous buffer age for partial updates (if supported). */
   Render_Output_Swap_Mode swap_mode; /**< Buffer swapping strategy (e.g., MODE_FULL, MODE_COPY). */
   int vsync; /**< Vertical synchronization state (unused?). */
   int frame_cnt; /**< Frame counter (potentially for debugging or profiling). */

   struct
     {
        Eina_Bool drew : 1; /**< Flag indicating if any drawing occurred in the current frame. */
     } draw;

   EGLContext egl_context; /**< EGL rendering context. */
   EGLSurface egl_surface; /**< EGL window surface. */
   EGLConfig egl_config; /**< EGL configuration used for the context and surface. */
   EGLDisplay egl_disp; /**< EGL display connection. */

   struct {
      unsigned char depth_buffer_size; /**< Detected depth buffer size (bits). */
      unsigned char stencil_buffer_size; /**< Detected stencil buffer size (bits). */
      unsigned char msaa; /**< Detected multisample anti-aliasing level. */
   } detected;

   int depth_bits; /**< Requested depth buffer size (bits). */
   int stencil_bits; /**< Requested stencil buffer size (bits). */
   int msaa_bits; /**< Requested multisample anti-aliasing level. */

   Eina_Bool lost_back : 1; /**< Flag indicating if the back buffer content was lost (e.g., due to resize). */
   Eina_Bool surf : 1; /**< Flag indicating if the EGL surface is currently valid. */
};

struct _Context_3D
{
   EGLDisplay display; /**< EGL display connection. */
   EGLContext context; /**< EGL rendering context. */
   EGLSurface surface; /**< EGL surface (likely for offscreen rendering, if used). */
};

extern Eina_Bool extn_have_buffer_age; /**< Flag indicating if EGL_EXT_buffer_age or EGL_KHR_partial_update is available. */
extern Eina_Bool extn_have_y_inverted; /**< Flag indicating if EGL_NOK_texture_from_pixmap (for Y-inversion) is available and working. */

/* Function pointers to GL_Generic common functions */
extern Evas_GL_Common_Context_New glsym_evas_gl_common_context_new; /**< Pointer to evas_gl_common_context_new. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush; /**< Pointer to evas_gl_common_context_flush. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free; /**< Pointer to evas_gl_common_context_free. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use; /**< Pointer to evas_gl_common_context_use. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe; /**< Pointer to evas_gl_common_context_newframe. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done; /**< Pointer to evas_gl_common_context_done. */
extern Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize; /**< Pointer to evas_gl_common_context_resize. */
extern Evas_GL_Common_Buffer_Dump_Call glsym_evas_gl_common_buffer_dump; /**< Pointer to evas_gl_common_buffer_dump. */
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock; /**< Pointer to evas_gl_preload_render_lock. */
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock; /**< Pointer to evas_gl_preload_render_unlock. */

/* Function pointers for EGL extensions */
extern unsigned int (*glsym_eglSwapBuffersWithDamage) (EGLDisplay a, void *b, const EGLint *d, EGLint c); /**< Pointer to eglSwapBuffersWithDamageEXT/INTEL/KHR. */
extern unsigned int (*glsym_eglSetDamageRegionKHR) (EGLDisplay a, EGLSurface b, EGLint *c, EGLint d); /**< Pointer to eglSetDamageRegionKHR. */

/**
 * @brief Creates a new output buffer (Outbuf) for a Wayland window.
 * @param einfo Engine setup information.
 * @param w Initial width.
 * @param h Initial height.
 * @param swap_mode Desired buffer swapping mode.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *eng_window_new(Evas_Engine_Info_Wayland *einfo, int w, int h, Render_Output_Swap_Mode swap_mode);

/**
 * @brief Frees the resources associated with an output buffer.
 * @param gw The Outbuf to free.
 */
void eng_window_free(Outbuf *gw);

/**
 * @brief Makes the EGL context and surface of the output buffer current.
 * @param gw The Outbuf to make current, or NULL to release the current context.
 */
void eng_window_use(Outbuf *gw);

/**
 * @brief Destroys the EGL surface associated with the output buffer.
 * @param gw The Outbuf whose surface should be destroyed.
 */
void eng_window_unsurf(Outbuf *gw);

/**
 * @brief Recreates the EGL surface for the output buffer if it was destroyed.
 * @param gw The Outbuf whose surface should be recreated.
 */
void eng_window_resurf(Outbuf *gw);

/**
 * @brief Reconfigures the output buffer dimensions and rotation.
 * @param ob The Outbuf to reconfigure.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation angle.
 * @param depth New depth (unused?).
 */
void eng_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);

/**
 * @brief Gets the current rotation angle of the output buffer.
 * @param ob The Outbuf.
 * @return The rotation angle (0, 90, 180, 270).
 */
int eng_outbuf_rotation_get(Outbuf *ob);

/**
 * @brief Gets the buffer swapping mode of the output buffer.
 * @param ob The Outbuf.
 * @return The current swap mode.
 */
Render_Output_Swap_Mode eng_outbuf_swap_mode_get(Outbuf *ob);

/**
 * @brief Checks if this is the first rectangle in the update region list.
 * @param ob The Outbuf.
 * @return EINA_TRUE if it's the first rectangle, EINA_FALSE otherwise.
 */
Eina_Bool eng_outbuf_region_first_rect(Outbuf *ob);

/**
 * @brief Sets the damage region for the EGL surface (if partial updates are supported).
 * @param ob The Outbuf.
 * @param damage Pointer to the damage rectangles.
 */
void eng_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage);

/**
 * @brief Creates a new update region (potentially for software rendering fallback).
 * @param ob The Outbuf.
 * @param x X coordinate of the update region.
 * @param y Y coordinate of the update region.
 * @param w Width of the update region.
 * @param h Height of the update region.
 * @param cx Pointer to store the clipped X coordinate (output).
 * @param cy Pointer to store the clipped Y coordinate (output).
 * @param cw Pointer to store the clipped width (output).
 * @param ch Pointer to store the clipped height (output).
 * @return A pointer related to the update region, or NULL.
 */
void *eng_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an updated RGBA image region to the output buffer (potentially for software rendering fallback).
 * @param ob The Outbuf.
 * @param update The RGBA image containing the updated pixels.
 * @param x X coordinate where the update should be placed.
 * @param y Y coordinate where the update should be placed.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void eng_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Flushes the rendered content to the Wayland surface, performing the buffer swap.
 * @param ob The Outbuf to flush.
 * @param surface_damage Rectangles defining the damaged region on the surface.
 * @param buffer_damage Rectangles defining the damaged region in the back buffer.
 * @param render_mode The current Evas render mode.
 */
void eng_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Gets the Evas GL context associated with the output buffer.
 * @param ob The Outbuf.
 * @return A pointer to the Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context *eng_outbuf_gl_context_get(Outbuf *ob);

/**
 * @brief Gets the EGL display connection associated with the output buffer.
 * @param ob The Outbuf.
 * @return A pointer to the EGLDisplay.
 */
void *eng_outbuf_egl_display_get(Outbuf *ob);

/**
 * @brief Callback function for Evas GL preloading to make the context current or release it.
 * @param data User data (expected to be an Outbuf pointer).
 * @param doit EINA_TRUE to make current, EINA_FALSE to release.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool eng_preload_make_current(void *data, void *doit);

/**
 * @brief Creates a new 3D context (currently unused or minimal).
 * @param win The associated Outbuf.
 * @return A pointer to the new Context_3D, or NULL on failure.
 */
Context_3D *eng_gl_context_new(Outbuf *win);

/**
 * @brief Frees a 3D context.
 * @param context The Context_3D to free.
 */
void eng_gl_context_free(Context_3D *context);

/**
 * @brief Makes a 3D context current (currently unused or minimal).
 * @param context The Context_3D to make current.
 */
void eng_gl_context_use(Context_3D *context);

/**
 * @brief Resolves symbols for required EGL/GL extensions.
 * @param disp The EGL display connection.
 */
void eng_gl_symbols(EGLDisplay disp);

/**
 * @brief Checks if the output buffer's surface is valid, recreating it if necessary.
 *
 * This inline function ensures that the EGL surface (`ob->surf`) is valid before
 * attempting to use it. If the surface was lost (e.g., due to context loss or
 * window state changes), it calls `eng_window_resurf()` to recreate it and sets
 * the `lost_back` flag.
 *
 * @param ob The output buffer (`Outbuf`) to check.
 * @return 1 if the surface is valid (or was successfully recreated), 0 on failure to recreate.
 */
static inline int
_re_wincheck(Outbuf *ob)
{
   if (ob->surf) return 1;
   eng_window_resurf(ob);
   ob->lost_back = EINA_TRUE;
   if (!ob->surf)
     ERR("Wayland EGL Engine cannot recreate window surface");
   return 0;
}

#endif
