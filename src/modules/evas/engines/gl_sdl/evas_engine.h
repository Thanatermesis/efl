#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H

/**
 * @file evas_engine.h
 * @brief Header file for the Evas GL SDL engine.
 *
 * This file defines the structures and constants used by the Evas GL SDL
 * engine. It includes necessary SDL and OpenGL headers, as well as Evas
 * internal headers.
 */

#define _EVAS_ENGINE_SDL_H /**< Guard to indicate SDL engine header inclusion. */

#include "config.h"
#include <SDL2/SDL.h>
#ifdef GL_GLES
# include <SDL2/SDL_opengles.h>
# ifdef HAVE_SDL_FLAG_OPENGLES
#  define EVAS_SDL_GL_FLAG SDL_OPENGLES
# else
#  define EVAS_SDL_GL_FLAG SDL_OPENGL /* This probably won't work? */
# endif
#else
# include <SDL2/SDL_opengl.h>
# define EVAS_SDL_GL_FLAG SDL_OPENGL
#endif
#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_gl_common.h"
#include "Evas.h"
#include "Evas_Engine_GL_SDL.h"

#include "../gl_generic/Evas_Engine_GL_Generic.h"

extern int _evas_engine_GL_SDL_log_dom; /**< Log domain for the Evas GL SDL engine. */

#ifdef ERR
# undef ERR
#endif
/** @brief Error logging macro for the Evas GL SDL engine. */
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_GL_SDL_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
/** @brief Debug logging macro for the Evas GL SDL engine. */
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_GL_SDL_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
/** @brief Info logging macro for the Evas GL SDL engine. */
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_GL_SDL_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
/** @brief Warning logging macro for the Evas GL SDL engine. */
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_GL_SDL_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
/** @brief Critical logging macro for the Evas GL SDL engine. */
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_GL_SDL_log_dom, __VA_ARGS__)

/** @brief Typedef for the main render engine structure. */
typedef struct _Render_Engine Render_Engine;

/**
 * @struct _Outbuf
 * @brief Structure representing the output buffer for the Evas GL SDL engine.
 *
 * This structure holds all necessary information for managing the SDL window,
 * GL context, Evas GL context, and EGL context (if applicable).
 */
struct _Outbuf
{
   Evas_Engine_Info_GL_SDL *info; /**< Pointer to Evas engine info specific to GL SDL. */
   SDL_Window *window;            /**< Pointer to the SDL window. */
   SDL_GLContext *context;        /**< Pointer to the SDL GL context. */

   Evas_Engine_GL_Context *gl_context; /**< Pointer to the Evas common GL context. */
   struct {
      int              redraw : 1; /**< Flag indicating if a redraw is needed. */
      int              drew : 1;   /**< Flag indicating if drawing occurred. */
      int              x1, y1, x2, y2; /**< Coordinates of the drawn area. */
   } draw; /**< Drawing status and area. */
#ifdef GL_GLES
   EGLContext       egl_context; /**< EGL context (for GLES). */
   EGLSurface       egl_surface; /**< EGL surface (for GLES). */
   EGLConfig        egl_config;  /**< EGL configuration (for GLES). */
   EGLDisplay       egl_disp;    /**< EGL display (for GLES). */
#endif

   int w, h; /**< Width and height of the output buffer. */
};

/**
 * @struct _Render_Engine
 * @brief Main structure for the Evas GL SDL render engine.
 *
 * This structure encapsulates the generic GL rendering output components.
 */
struct _Render_Engine
{
   Render_Output_GL_Generic generic; /**< Generic GL rendering output data. */
};

#endif
