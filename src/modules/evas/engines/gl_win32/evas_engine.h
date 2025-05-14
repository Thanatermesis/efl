#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H

#include "config.h"
#include "evas_common_private.h"
#include "evas_private.h"
#include "Evas.h"
#include "Evas_Engine_GL_Win32.h"

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "../gl_generic/Evas_Engine_GL_Generic.h"

extern int _evas_engine_gl_win32_log_dom ;
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_gl_win32_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_gl_win32_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_gl_win32_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_gl_win32_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_gl_win32_log_dom, __VA_ARGS__)

/**
 * @brief Structure representing an output buffer (window or pixmap surface).
 *
 * This structure holds all the necessary information for rendering to a
 * specific output, including EGL context, surface, configuration, and
 * various rendering parameters.
 */
struct _Outbuf
{
   EGLContext       egl_context; /**< EGL rendering context */
   EGLSurface       egl_surface; /**< EGL window or pixmap surface */
   EGLConfig        egl_config;  /**< EGL configuration used for the surface */
   EGLDisplay       egl_disp;    /**< EGL display connection */
   /** @brief Detected capabilities of the EGL configuration/surface. */
   struct {
      unsigned char depth_buffer_size;   /**< Detected depth buffer size in bits */
      unsigned char stencil_buffer_size; /**< Detected stencil buffer size in bits */
      unsigned char msaa;                /**< Detected MSAA samples (0 if none) */
      Eina_Bool     loose_binding : 1;   /**< Flag indicating if loose binding is detected/used */
   } detected;


   Evas            *evas;         /**< Pointer back to the Evas canvas */
   HWND             window;       /**< Win32 window handle */
   HDC              dc;           /**< Win32 device context handle */
   int              depth_bits;   /**< Requested depth buffer bits */
   int              stencil_bits; /**< Requested stencil buffer bits */
   int              msaa_bits;    /**< Requested MSAA bits */
   Evas_Engine_GL_Context *gl_context; /**< Pointer to the shared GL context data */
   Evas_Engine_Info_GL_Win32 *info; /**< Pointer to the engine setup information */

   Render_Output_Swap_Mode swap_mode; /**< Buffer swap mode (auto, copy, etc.) */
   unsigned int     w, h;         /**< Width and height of the output buffer */
   int              screen;       /**< Screen number (unused in Win32?) */
   int              depth;        /**< Color depth of the output buffer */
   int              alpha;        /**< Alpha channel enabled flag */
   int              rotation;     /**< Current rotation angle (0, 90, 180, 270) */
   int              prev_age;     /**< Previous buffer age (for damage tracking) */
   int              frame_cnt;    /**< Frame counter */
   int              vsync;        /**< VSync enabled flag */

   unsigned char    lost_back : 1; /**< Flag indicating if the back buffer was lost */
   unsigned char    surf : 1;      /**< Flag indicating if the EGL surface is valid */

   /** @brief Drawing related flags. */
   struct {
      unsigned char drew : 1;      /**< Flag indicating if anything was drawn in the current frame */
   } draw;
};

/**
 * @brief Structure representing a 3D rendering context for client applications (EVGL).
 *
 * This holds the necessary EGL handles for a separate context that can share
 * resources with the main Evas EGL context.
 */
struct _Context_3D
{
   EGLDisplay      display; /**< EGL display connection (usually same as Outbuf's) */
   EGLContext      context; /**< The client's EGL rendering context */
   EGLSurface      surface; /**< The client's EGL drawing surface (can be window, pbuffer, etc.) */
};

extern int extn_have_buffer_age; /**< Flag indicating if EGL_EXT_buffer_age or equivalent is supported */
extern int partial_render_debug; /**< Flag to enable partial rendering debug messages */
extern int swap_buffer_debug_mode; /**< Swap buffer debug mode status (-1: unset, 0: disabled, 1: enabled) */
extern int swap_buffer_debug;      /**< Flag to enable swap buffer debugging (dumping frames) */
extern const char *debug_dir;      /**< Directory for swap buffer debug dumps */

/** @brief Function pointer for creating a new Evas GL common context. */
extern Evas_GL_Common_Context_New glsym_evas_gl_common_context_new;
/** @brief Function pointer for flushing the Evas GL common context. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush;
/** @brief Function pointer for freeing the Evas GL common context. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free;
/** @brief Function pointer for making the Evas GL common context current. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use;
/** @brief Function pointer for starting a new frame in the Evas GL common context. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe;
/** @brief Function pointer for finishing operations in the Evas GL common context. */
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done;
/** @brief Function pointer for resizing the Evas GL common context. */
extern Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize;
/** @brief Function pointer for dumping the Evas GL common context buffer. */
extern Evas_GL_Common_Buffer_Dump_Call glsym_evas_gl_common_buffer_dump;
/** @brief Function pointer for locking the preload renderer. */
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock;
/** @brief Function pointer for unlocking the preload renderer. */
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock;

#ifndef EGL_BUFFER_AGE_EXT
/** @brief EGL extension definition for buffer age, if not already defined. */
# define EGL_BUFFER_AGE_EXT 0x313d
#endif

/** @brief Function pointer for eglSwapBuffersWithDamageEXT/KHR. */
extern unsigned int   (*glsym_eglSwapBuffersWithDamage) (EGLDisplay a, void *b, const EGLint *d, EGLint c);
/** @brief Function pointer for eglSetDamageRegionKHR. */
extern unsigned int   (*glsym_eglSetDamageRegionKHR)  (EGLDisplay a, EGLSurface b, EGLint *c, EGLint d);

/**
 * @brief Creates a new output buffer (window surface).
 * @param info Engine setup information.
 * @param w Initial width.
 * @param h Initial height.
 * @param swap_mode Requested buffer swap mode.
 * @return A new Outbuf structure or NULL on failure.
 */
Outbuf *eng_window_new(Evas_Engine_Info_GL_Win32 *info,
                       unsigned int w,
                       unsigned int h,
                       Render_Output_Swap_Mode swap_mode);
/**
 * @brief Frees an output buffer.
 * @param gw The Outbuf to free.
 */
void    eng_window_free(Outbuf *gw);
/**
 * @brief Makes the output buffer's GL context current.
 * @param gw The Outbuf to use.
 */
void    eng_window_use(Outbuf *gw);
/**
 * @brief Destroys the EGL surface associated with the output buffer.
 * @param gw The Outbuf whose surface should be destroyed.
 */
void    eng_window_unsurf(Outbuf *gw);
/**
 * @brief Recreates the EGL surface for the output buffer if it was lost.
 * @param gw The Outbuf whose surface should be recreated.
 */
void    eng_window_resurf(Outbuf *gw);

/**
 * @brief Creates a new 3D context for client applications (EVGL).
 * @param win The Outbuf whose context should be shared.
 * @return A new Context_3D structure or NULL on failure.
 */
Context_3D *eng_gl_context_new(Outbuf *win);
/**
 * @brief Frees a 3D context.
 * @param context The Context_3D to free.
 */
void        eng_gl_context_free(Context_3D *context);
/**
 * @brief Makes the 3D context current.
 * @param context The Context_3D to use.
 */
void        eng_gl_context_use(Context_3D *context);

/**
 * @brief Wrapper for eglGetCurrentContext that handles main loop integration.
 * @return The current EGL context.
 */
EGLContext evas_eglGetCurrentContext(void);
/**
 * @brief Wrapper for eglGetCurrentSurface that handles main loop integration.
 * @param readdraw Specifies EGL_READ or EGL_DRAW.
 * @return The current EGL surface for the specified purpose.
 */
EGLSurface evas_eglGetCurrentSurface(EGLint readdraw);
/**
 * @brief Wrapper for eglGetCurrentDisplay that handles main loop integration.
 * @return The current EGL display.
 */
EGLDisplay evas_eglGetCurrentDisplay(void);
/**
 * @brief Wrapper for eglMakeCurrent that handles main loop integration and state caching.
 * @param dpy The EGL display.
 * @param draw The EGL draw surface.
 * @param read The EGL read surface.
 * @param ctx The EGL context.
 * @return EGL_TRUE on success, EGL_FALSE otherwise.
 */
EGLBoolean evas_eglMakeCurrent(EGLDisplay dpy,
                               EGLSurface draw,
                               EGLSurface read,
                               EGLContext ctx);

/**
 * @brief Reconfigures an output buffer (resizes, rotates).
 * @param ob The Outbuf to reconfigure.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation angle.
 * @param depth New depth (unused?).
 */
void                    eng_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);
/**
 * @brief Gets the current rotation of the output buffer.
 * @param ob The Outbuf.
 * @return The rotation angle (0, 90, 180, 270).
 */
int                     eng_outbuf_get_rot(Outbuf *ob);
/**
 * @brief Gets the swap mode of the output buffer.
 * @param ob The Outbuf.
 * @return The current swap mode.
 */
Render_Output_Swap_Mode eng_outbuf_swap_mode(Outbuf *ob);
/**
 * @brief Checks if this is the first rectangle in the damage region.
 * @param ob The Outbuf.
 * @return EINA_TRUE if it's the first rectangle, EINA_FALSE otherwise.
 */
Eina_Bool               eng_outbuf_region_first_rect(Outbuf *ob);
/**
 * @brief Sets the damage region for the next buffer swap (if supported).
 * @param ob The Outbuf.
 * @param damage Pointer to the damage rectangles.
 */
void                    eng_outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage);
/**
 * @brief Gets a new region for update (allocates buffer space).
 * @param ob The Outbuf.
 * @param x X coordinate of the update region.
 * @param y Y coordinate of the update region.
 * @param w Width of the update region.
 * @param h Height of the update region.
 * @param cx Pointer to store the clipped X coordinate.
 * @param cy Pointer to store the clipped Y coordinate.
 * @param cw Pointer to store the clipped width.
 * @param ch Pointer to store the clipped height.
 * @return Pointer to the allocated buffer region or NULL.
 */
void                   *eng_outbuf_new_region_for_update(Outbuf *ob,
                                                         int x, int y,
                                                         int w, int h,
                                                         int *cx, int *cy,
                                                         int *cw, int *ch);
/**
 * @brief Pushes an updated region to the output buffer (copies data).
 * @param ob The Outbuf.
 * @param update The RGBA image containing the update data.
 * @param x X coordinate of the update region.
 * @param y Y coordinate of the update region.
 * @param w Width of the update region.
 * @param h Height of the update region.
 */
void                    eng_outbuf_push_updated_region(Outbuf *ob,
                                                       RGBA_Image *update,
                                                       int x, int y,
                                                       int w, int h);
/**
 * @brief Flushes the output buffer (performs buffer swap).
 * @param ob The Outbuf.
 * @param surface_damage Damage region for the surface (unused?).
 * @param buffer_damage Damage region for the buffer (used for partial swaps).
 * @param render_mode The rendering mode (full, copy, etc.).
 */
void                    eng_outbuf_flush(Outbuf *ob,
                                         Tilebuf_Rect *surface_damage,
                                         Tilebuf_Rect *buffer_damage,
                                         Evas_Render_Mode render_mode);
/**
 * @brief Gets the Evas GL context associated with the output buffer.
 * @param ob The Outbuf.
 * @return Pointer to the Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context *eng_outbuf_gl_context_get(Outbuf *ob);
/**
 * @brief Gets the EGL display associated with the output buffer.
 * @param ob The Outbuf.
 * @return The EGLDisplay handle.
 */
void                   *eng_outbuf_egl_display_get(Outbuf *ob);

/**
 * @brief Callback function for making the GL context current during preload rendering.
 * @param data The Outbuf pointer.
 * @param doit EINA_TRUE to make current, EINA_FALSE to release.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool eng_preload_make_current(void *data, void *doit);
/**
 * @brief Resolves required GL/EGL symbols.
 * @param ob The Outbuf (used to query extensions).
 */
void eng_gl_symbols(Outbuf *ob);

/**
 * @brief Checks if the window surface is valid and attempts to recreate it if not.
 *
 * This function is typically called before rendering operations to ensure
 * the EGL surface is available. If the surface was lost (e.g., due to context loss),
 * it calls eng_window_resurf() to try and recreate it.
 *
 * @param ob The output buffer to check.
 * @return 1 if the surface is valid or was successfully recreated, 0 otherwise.
 */
static inline int
_re_wincheck(Outbuf *ob)
{
   if (ob->surf) return 1;
   eng_window_resurf(ob);
   ob->lost_back = 1;
   if (!ob->surf)
     {
        ERR("GL engine can't re-create window surface!");
     }
   return 0;
}

extern Eina_Bool gles3_supported; /**< Flag indicating if GLES 3.x is supported by the driver/context */

#endif
