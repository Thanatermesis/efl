#include "ecore_evas_wayland_private.h"

#ifdef BUILD_ECORE_EVAS_WAYLAND_SHM
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

/* external functions */

/**
 * @internal
 * @brief Creates a new Ecore_Evas Wayland SHM window.
 *
 * This function is an internal helper to create a Wayland Ecore_Evas
 * instance utilizing shared memory (SHM) for buffer sharing.
 *
 * @param disp_name The name of the Wayland display to connect to.
 *                  If NULL, the default display will be used.
 * @param parent The parent window, if any. For Wayland, this is often 0.
 * @param x The x-coordinate of the window's top-left corner.
 * @param y The y-coordinate of the window's top-left corner.
 * @param w The width of the window in pixels.
 * @param h The height of the window in pixels.
 * @param frame EINA_TRUE if the window should have a frame (decorations),
 *              EINA_FALSE otherwise.
 * @return A pointer to the newly created Ecore_Evas object on success,
 *         NULL on failure.
 */
EMODAPI Ecore_Evas *
ecore_evas_wayland_shm_new_internal(const char *disp_name, Ecore_Window parent, int x, int y, int w, int h, Eina_Bool frame)
{
   LOGFN;

   return _ecore_evas_wl_common_new_internal(disp_name, parent, x, y, w, h,
                                             frame, NULL, "wayland_shm");
}

#endif
