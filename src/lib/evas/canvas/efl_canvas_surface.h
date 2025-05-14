/**
 * @file
 * @brief These routines are used for Efl Canvas Surface.
 */

#define EFL_CANVAS_SURFACE_PROTECTED

#include "evas_image_private.h"
#include "efl_canvas_surface.eo.h"
#include "efl_canvas_surface_tbm.eo.h"
#include "efl_canvas_surface_x11.eo.h"
#include "efl_canvas_surface_wayland.eo.h"

/**
 * @brief Private data structure for Efl_Canvas_Surface.
 *
 * This structure holds the native surface information and a pointer
 * to the underlying buffer data.
 */
typedef struct _Efl_Canvas_Surface_Data
{
   Evas_Native_Surface surf; /**< Evas native surface information.
                               * This structure contains details about the native surface,
                               * such as its type, version, and specific data like visual,
                               * colormap, and drawable for X11, or the wl_buffer for Wayland.
                               * For example, for an X11 surface, it might look like:
                               * surf.type = EVAS_NATIVE_SURFACE_X11;
                               * surf.version = EVAS_NATIVE_SURFACE_VERSION;
                               * surf.data.x11.visual = xvisual;
                               * surf.data.x11.colormap = xcolormap;
                               * surf.data.x11.drawable = xdrawable;
                               */
   void *buffer;             /**< Pointer to the native buffer data.
                               * This is a generic pointer to the actual pixel data
                               * or a handle to the buffer, depending on the surface type.
                               * For instance, it could be a pointer to a shared memory segment
                               * or a TBM surface handle.
                               */
} Efl_Canvas_Surface_Data;
