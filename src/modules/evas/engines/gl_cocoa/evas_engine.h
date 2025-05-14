#ifndef __EVAS_ENGINE_H__
#define __EVAS_ENGINE_H__

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <OpenGL/gl.h>

#include "../gl_common/evas_gl_common.h"
#include "Evas_Engine_GL_Cocoa.h"
#include "../gl_generic/Evas_Engine_GL_Generic.h"


extern int _evas_engine_gl_cocoa_log_dom;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_gl_cocoa_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_gl_cocoa_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_gl_cocoa_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_gl_cocoa_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_gl_cocoa_log_dom, __VA_ARGS__)

typedef struct _Render_Engine Render_Engine;

/**
 * @brief Structure representing the rendering engine.
 *
 * This structure holds the generic GL rendering output and a pointer to the
 * output buffer associated with the window.
 */
struct _Render_Engine
{
   Render_Output_GL_Generic generic; /**< Generic GL rendering output data. */

   Outbuf *win; /**< Pointer to the output buffer for the window. */
};

/**
 * @brief Structure representing a 3D rendering context.
 *
 * This structure holds a pointer to the output buffer and the native
 * NSOpenGLContext.
 */
struct _Context_3D
{
   Outbuf *ob;             /**< Pointer to the associated output buffer. */
   void   *ns_gl_context;  /**< Pointer to the native NSOpenGLContext. */
};

/**
 * @brief Structure representing an output buffer.
 *
 * This structure contains information about the Cocoa GL engine's output,
 * including the native view and window, dimensions, and rendering context.
 */
struct _Outbuf
{
   Evas_Engine_Info_GL_Cocoa *info; /**< Evas Cocoa GL engine specific information. */
   Evas_Engine_GL_Context *gl_context; /**< Evas GL context. */

   void *ns_gl_view; /**< Pointer to the native NSOpenGLView. */
   void *ns_window;  /**< Pointer to the native NSWindow. */

   int w; /**< Width of the output buffer. */
   int h; /**< Height of the output buffer. */
   int rot; /**< Rotation angle of the output buffer. */
   Render_Output_Swap_Mode swap_mode; /**< Swap mode for rendering. */

   Eina_Bool drew; /**< Flag indicating if drawing occurred. */
};


/**
 * @brief Get the output buffer from the render engine.
 * @param re Pointer to the Render_Engine.
 * @return Pointer to the Outbuf.
 */
static inline Outbuf *
eng_get_ob(const Render_Engine *re)
{
   return re->generic.software.ob;
}

extern Evas_GL_Common_Context_New glsym_evas_gl_common_context_new;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_free;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_flush;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_use;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_done;
extern Evas_GL_Common_Context_Resize_Call glsym_evas_gl_common_context_resize;
extern Evas_GL_Common_Context_Call glsym_evas_gl_common_context_newframe;
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_lock;
extern Evas_GL_Preload_Render_Call glsym_evas_gl_preload_render_unlock;
extern Evas_Gl_Symbols glsym_evas_gl_symbols;

/**
 * @brief Creates a new output buffer.
 * @param info Pointer to Evas_Engine_Info_GL_Cocoa.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @return A new Outbuf instance or NULL on failure.
 */
Outbuf *evas_outbuf_new(Evas_Engine_Info_GL_Cocoa *info, int w, int h);

/**
 * @brief Frees an output buffer.
 * @param ob Pointer to the Outbuf to free.
 */
void evas_outbuf_free(Outbuf *ob);

/**
 * @brief Sets the given output buffer as the current one for rendering.
 * @param ob Pointer to the Outbuf to use.
 */
void evas_outbuf_use(Outbuf *ob);

/**
 * @brief Gets the rotation of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The rotation angle.
 */
int evas_outbuf_rot_get(Outbuf *ob);

/**
 * @brief Gets the buffer state (swap mode) of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The current Render_Output_Swap_Mode.
 */
Render_Output_Swap_Mode evas_outbuf_buffer_state_get(Outbuf *ob);

/**
 * @brief Makes the GL context of the given 3D context current.
 * @param ctx Pointer to the Context_3D.
 */
void evas_outbuf_gl_context_use(Context_3D *ctx);

/**
 * @brief Checks if this is the first rectangle in an update region.
 * @param ob Pointer to the Outbuf.
 * @return EINA_TRUE if it's the first rectangle, EINA_FALSE otherwise.
 */
Eina_Bool evas_outbuf_update_region_first_rect(Outbuf *ob);

/**
 * @brief Creates a new update region.
 * @param ob Pointer to the Outbuf.
 * @param x X coordinate of the region.
 * @param y Y coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param[out] cx Clipped X coordinate.
 * @param[out] cy Clipped Y coordinate.
 * @param[out] cw Clipped width.
 * @param[out] ch Clipped height.
 * @return A pointer to the update region data, or NULL on failure.
 */
void *evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an updated RGBA image to the specified region of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @param update Pointer to the RGBA_Image containing the update.
 * @param x X coordinate of the update.
 * @param y Y coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Flushes the output buffer, handling surface and buffer damage.
 * @param ob Pointer to the Outbuf.
 * @param surface_damage Pointer to Tilebuf_Rect for surface damage.
 * @param buffer_damage Pointer to Tilebuf_Rect for buffer damage.
 * @param render_mode The Evas_Render_Mode.
 */
void evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Gets the Evas GL context from the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return Pointer to Evas_Engine_GL_Context.
 */
Evas_Engine_GL_Context *evas_outbuf_gl_context_get(Outbuf *ob);

/**
 * @brief Creates a new 3D context for the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return A new Context_3D instance or NULL on failure.
 */
Context_3D *evas_outbuf_gl_context_new(Outbuf *ob);

/**
 * @brief Reconfigures the output buffer.
 * @param ob Pointer to the Outbuf.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation.
 * @param depth New depth (unused in this backend).
 */
void evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth);

/**
 * @brief Gets the EGL display associated with the output buffer.
 * @note This is likely a misnomer in a Cocoa context and might return a native display handle.
 * @param ob Pointer to the Outbuf.
 * @return Pointer to the display (e.g., EGLDisplay or native equivalent).
 */
void *evas_outbuf_egl_display_get(Outbuf *ob);

#define GL_COCOA_UNIMPLEMENTED_CALL_SO_RETURN(...) \
   do { \
      CRI("[%s] IS AN UNIMPLEMENTED CALL. PLEASE REPORT!!", __func__); \
      return __VA_ARGS__; \
   } while (0)

#endif /* __EVAS_ENGINE_H__ */
