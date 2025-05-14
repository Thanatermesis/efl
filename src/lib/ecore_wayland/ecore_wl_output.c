#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl_private.h"

/**
 * @brief Wayland output listener callback for geometry changes.
 *
 * This function is called by the Wayland compositor when an output's
 * geometry properties (position, physical size, transform) are advertised.
 *
 * @param data User data, expected to be an Ecore_Wl_Output pointer.
 * @param wl_output The Wayland output object.
 * @param x The x-coordinate of the output's top-left corner in the global compositor space.
 * @param y The y-coordinate of the output's top-left corner in the global compositor space.
 * @param w The physical width of the output in millimeters.
 * @param h The physical height of the output in millimeters.
 * @param subpixel The subpixel geometry (e.g., WL_OUTPUT_SUBPIXEL_RGB, WL_OUTPUT_SUBPIXEL_UNKNOWN).
 * @param make The manufacturer name of the output.
 * @param model The model name of the output.
 * @param transform The output transformation (e.g., WL_OUTPUT_TRANSFORM_NORMAL, WL_OUTPUT_TRANSFORM_90).
 */
static void
_ecore_wl_output_cb_geometry(void *data, struct wl_output *wl_output EINA_UNUSED, int x, int y, int w, int h, int subpixel EINA_UNUSED, const char *make EINA_UNUSED, const char *model EINA_UNUSED, int transform)
{
   Ecore_Wl_Output *output;

   LOGFN;

   output = data;
   output->allocation.x = x;
   output->allocation.y = y;
   output->mw = w;
   output->mh = h;
   output->transform = transform;
}

/**
 * @brief Wayland output listener callback for mode changes.
 *
 * This function is called by the Wayland compositor when an output's
 * mode (resolution, refresh rate, flags) is advertised.
 *
 * @param data User data, expected to be an Ecore_Wl_Output pointer.
 * @param wl_output The Wayland output object.
 * @param flags Flags for the mode (e.g., WL_OUTPUT_MODE_CURRENT, WL_OUTPUT_MODE_PREFERRED).
 * @param w The width of the mode in pixels.
 * @param h The height of the mode in pixels.
 * @param refresh The refresh rate of the mode in mHz (millihertz).
 */
static void
_ecore_wl_output_cb_mode(void *data, struct wl_output *wl_output EINA_UNUSED, unsigned int flags, int w, int h, int refresh EINA_UNUSED)
{
   Ecore_Wl_Output *output;
   Ecore_Wl_Display *ewd;

   LOGFN;

   output = data;
   ewd = output->display;
   if (flags & WL_OUTPUT_MODE_CURRENT)
     {
        output->allocation.w = w;
        output->allocation.h = h;
        _ecore_wl_disp->output = output;
        if (ewd->output_configure) (*ewd->output_configure)(output, ewd->data);
     }
}

/**
 * @brief Wayland output listener callback for done event.
 *
 * This function is called by the Wayland compositor after all properties
 * of an output have been sent. This typically signals the end of a batch
 * of output configuration updates.
 *
 * @param data User data, not used in the current implementation.
 * @param output The Wayland output object.
 */
static void
_ecore_wl_output_cb_done(void *data EINA_UNUSED, struct wl_output *output EINA_UNUSED)
{
   // This event is sent after all other properties have been sent.
   // It can be used to trigger updates that depend on a complete set of output properties.
}

/**
 * @brief Wayland output listener callback for scale factor changes.
 *
 * This function is called by the Wayland compositor when an output's
 * scale factor is advertised.
 *
 * @param data User data, not used in the current implementation.
 * @param output The Wayland output object.
 * @param scale The new scale factor for the output.
 */
static void
_ecore_wl_output_cb_scale(void *data EINA_UNUSED, struct wl_output *output EINA_UNUSED, int scale EINA_UNUSED)
{
   // This event advertises the scale factor of the output.
   // It can be used to adjust UI rendering for high-DPI displays.
}

/**
 * @brief Listener structure for Wayland output events.
 *
 * This structure maps Wayland output events to their corresponding
 * callback functions.
 */
static const struct wl_output_listener _ecore_wl_output_listener =
{
   _ecore_wl_output_cb_geometry,
   _ecore_wl_output_cb_mode,
   _ecore_wl_output_cb_done,
   _ecore_wl_output_cb_scale
};

/**
 * @brief Get the list of available Wayland outputs.
 *
 * This function returns an Eina_Inlist containing all currently known
 * Ecore_Wl_Output structures. Each Ecore_Wl_Output represents a
 * display screen connected to the system.
 *
 * @return A pointer to the Eina_Inlist of Ecore_Wl_Output structures.
 *         The list is owned by Ecore_Wl and should not be modified
 *         or freed by the caller. Returns NULL if Ecore_Wl is not
 *         initialized or if there are no outputs.
 *
 * @since 1.2
 *
 * Example:
 * @code
 * Eina_Inlist *outputs;
 * Ecore_Wl_Output *output;
 *
 * outputs = ecore_wl_outputs_get();
 * EINA_INLIST_FOREACH(outputs, output)
 *   {
 *      printf("Output: %p, Resolution: %dx%d\n",
 *             output, output->allocation.w, output->allocation.h);
 *   }
 * @endcode
 */
EAPI Eina_Inlist *
ecore_wl_outputs_get(void)
{
   return _ecore_wl_disp->outputs;
}

/**
 * @internal
 * @brief Create a new Ecore_Wl_Output and add it to the display's list.
 *
 * This function is called internally when a new Wayland output (wl_output)
 * is advertised by the compositor via the wl_registry. It creates an
 * Ecore_Wl_Output wrapper, binds to the wl_output interface, and sets up
 * listeners for output events.
 *
 * @param ewd The Ecore_Wl_Display to which this output belongs.
 * @param id The Wayland global ID for the new output.
 */
void
_ecore_wl_output_add(Ecore_Wl_Display *ewd, unsigned int id)
{
   Ecore_Wl_Output *output;

   LOGFN;

   if (!(output = calloc(1, sizeof(Ecore_Wl_Output)))) return;

   output->display = ewd;

   output->output =
     wl_registry_bind(ewd->wl.registry, id, &wl_output_interface, 2);

   ewd->outputs = eina_inlist_append(ewd->outputs, EINA_INLIST_GET(output));
   wl_output_add_listener(output->output, &_ecore_wl_output_listener, output);
}

/**
 * @internal
 * @brief Destroy an Ecore_Wl_Output and remove it from the display's list.
 *
 * This function is called internally when a Wayland output is removed
 * (e.g., a monitor is disconnected). It cleans up resources associated
 * with the Ecore_Wl_Output, including destroying the wl_output proxy
 * and freeing allocated memory.
 *
 * @param output The Ecore_Wl_Output to be destroyed.
 */
void
_ecore_wl_output_del(Ecore_Wl_Output *output)
{
   if (!output) return;
   if (output->destroy) (*output->destroy)(output, output->data);
   if (output->output) wl_output_destroy(output->output);
   _ecore_wl_disp->outputs =
     eina_inlist_remove(_ecore_wl_disp->outputs, EINA_INLIST_GET(output));
   free(output);
}
