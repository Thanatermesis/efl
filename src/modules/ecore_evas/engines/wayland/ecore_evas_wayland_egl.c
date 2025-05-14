#include "ecore_evas_wayland_private.h"

#ifdef BUILD_ECORE_EVAS_WAYLAND_EGL
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

/* external functions */

/**
 * @internal
 * @brief Creates a new Ecore_Evas backed by Wayland EGL.
 *
 * This function is an internal constructor for creating an Ecore_Evas instance
 * that uses Wayland for display and EGL for rendering. It serves as a wrapper
 * around _ecore_evas_wl_common_new_internal, specifically for EGL-based
 * Wayland Ecore_Evas instances.
 *
 * @param disp_name The Wayland display name (e.g., "wayland-0"). If NULL,
 *                  the default display will be used.
 * @param parent The parent window (Ecore_Window). For Wayland, this is
 *               typically 0 as Wayland doesn't have a concept of parent
 *               windows in the traditional X11 sense for top-level windows.
 * @param x The x-coordinate for the window's top-left corner.
 * @param y The y-coordinate for the window's top-left corner.
 * @param w The width of the window.
 * @param h The height of the window.
 * @param frame EINA_TRUE if the window should have a frame (decorations),
 *              EINA_FALSE otherwise. Note that frame support depends on the
 *              Wayland compositor.
 * @param opt An optional array of integers for additional engine-specific
 *            options. This is typically NULL if no specific options are needed.
 *            Example: const int opt[] = { ECORE_EVAS_OPT_VSYNC, 1, 0 }; // VSync on
 *                     (The last 0 terminates the options array)
 * @return A pointer to the newly created Ecore_Evas instance on success,
 *         or NULL on failure.
 */
EMODAPI Ecore_Evas *
ecore_evas_wayland_egl_new_internal(const char *disp_name, Ecore_Window parent, int x, int y, int w, int h, Eina_Bool frame, const int *opt)
{
   LOGFN;

   return _ecore_evas_wl_common_new_internal(disp_name, parent,
                                             x, y, w, h, frame,
                                             opt, "wayland_egl");
}

#endif
