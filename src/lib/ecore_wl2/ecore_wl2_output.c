#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * @file ecore_wl2_output.c
 * @brief Wayland Output Abstraction
 *
 * This file implements the Ecore_Wl2_Output abstraction, which
 * represents a display output (monitor) in a Wayland environment.
 * It handles events related to output geometry, mode, scale, and
 * transformation.
 */

#include "ecore_wl2_private.h"

/**
 * @internal
 * @brief Callback for wl_output geometry events.
 *
 * Handles updates to the output's position, physical dimensions,
 * make, model, and transform.
 *
 * @param data The Ecore_Wl2_Output structure.
 * @param wl_output The Wayland output object (unused).
 * @param x The x-coordinate of the output's position.
 * @param y The y-coordinate of the output's position.
 * @param w The physical width of the output in millimeters.
 * @param h The physical height of the output in millimeters.
 * @param subpixel The subpixel geometry (unused).
 * @param make The make of the output device.
 * @param model The model of the output device.
 * @param transform The output transformation (e.g., rotation).
 */
static void
_cb_geometry(void *data, struct wl_output *wl_output EINA_UNUSED, int x, int y, int w, int h, int subpixel EINA_UNUSED, const char *make, const char *model, int transform)
{
   Ecore_Wl2_Output *output;
   int ot;

   output = data;
   if (!output) return;

   eina_stringshare_replace(&output->make, make);
   eina_stringshare_replace(&output->model, model);

   output->mw = w;
   output->mh = h;
   output->geometry.x = x;
   output->geometry.y = y;

   ot = output->transform;

   if (transform & 0x4)
     ERR("Cannot support output transformation");

   transform &= 0x3;
   if (output->transform != transform)
     {
        Ecore_Wl2_Event_Output_Transform *ev;

        output->transform = transform;

        ev = calloc(1, sizeof(Ecore_Wl2_Event_Output_Transform));
        if (ev)
          {
             ev->output = output;
             ev->old_transform = ot;
             ev->transform = transform;
             ecore_event_add(ECORE_WL2_EVENT_OUTPUT_TRANSFORM, ev, NULL, NULL);
          }
     }
}

/**
 * @internal
 * @brief Callback for wl_output mode events.
 *
 * Handles updates to the output's current mode (resolution).
 *
 * @param data The Ecore_Wl2_Output structure.
 * @param wl_output The Wayland output object (unused).
 * @param flags Flags indicating the mode status (e.g., current, preferred).
 * @param w The width of the mode in pixels.
 * @param h The height of the mode in pixels.
 * @param refresh The refresh rate in mHz (unused).
 */
static void
_cb_mode(void *data, struct wl_output *wl_output EINA_UNUSED, unsigned int flags, int w, int h, int refresh EINA_UNUSED)
{
   Ecore_Wl2_Output *output;

   output = data;
   if (!output) return;

   if (flags & WL_OUTPUT_MODE_CURRENT)
     {
        output->geometry.w = w;
        output->geometry.h = h;
     }
}

/**
 * @internal
 * @brief Callback for wl_output done events.
 *
 * Indicates that all output properties have been sent. This can be used
 * to trigger events for output (re)configuration.
 *
 * @param data The Ecore_Wl2_Output structure (unused).
 * @param output The Wayland output object (unused).
 */
static void
_cb_done(void *data EINA_UNUSED, struct wl_output *output EINA_UNUSED)
{
   /* NB: Use this event to raise any "output (re)configured events" */
}

/**
 * @internal
 * @brief Callback for wl_output scale events.
 *
 * Handles updates to the output's scale factor.
 *
 * @param data The Ecore_Wl2_Output structure (unused).
 * @param output The Wayland output object (unused).
 * @param scale The new scale factor (unused in current implementation).
 */
static void
_cb_scale(void *data EINA_UNUSED, struct wl_output *output EINA_UNUSED, int scale EINA_UNUSED)
{

}

/**
 * @internal
 * @brief Listener for wl_output events.
 *
 * This structure maps Wayland output events to their respective
 * callback functions.
 */
static const struct wl_output_listener _output_listener =
{
   _cb_geometry,
   _cb_mode,
   _cb_done,
   _cb_scale
};

/**
 * @internal
 * @brief Adds a new Ecore_Wl2_Output to the display.
 *
 * This function is called when a new Wayland output is announced by the
 * compositor. It creates an Ecore_Wl2_Output structure, binds to the
 * wl_output interface, and sets up listeners.
 *
 * @param display The Ecore_Wl2_Display to which the output belongs.
 * @param id The Wayland global ID for this output.
 */
void
_ecore_wl2_output_add(Ecore_Wl2_Display *display, unsigned int id)
{
   Ecore_Wl2_Output *output;

   output = calloc(1, sizeof(Ecore_Wl2_Output));
   if (!output) return;

   output->display = display;

   output->wl_output =
     wl_registry_bind(display->wl.registry, id, &wl_output_interface, 2);

   display->outputs =
     eina_inlist_append(display->outputs, EINA_INLIST_GET(output));

   wl_output_add_listener(output->wl_output, &_output_listener, output);
}

/**
 * @internal
 * @brief Deletes an Ecore_Wl2_Output from the display.
 *
 * This function is called when a Wayland output is removed. It cleans up
 * resources associated with the Ecore_Wl2_Output, including destroying
 * the wl_output proxy and freeing memory.
 *
 * @param output The Ecore_Wl2_Output to delete.
 */
void
_ecore_wl2_output_del(Ecore_Wl2_Output *output)
{
   Ecore_Wl2_Display *display;

   if (!output) return;

   display = output->display;

   if (output->wl_output) wl_output_destroy(output->wl_output);
   if (output->make) eina_stringshare_del(output->make);
   if (output->model) eina_stringshare_del(output->model);

   display->outputs =
     eina_inlist_remove(display->outputs, EINA_INLIST_GET(output));

   free(output);
}

/**
 * @internal
 * @brief Finds an Ecore_Wl2_Output associated with a wl_output.
 *
 * Iterates through the list of outputs for a given display to find the
 * Ecore_Wl2_Output that corresponds to the provided wl_output proxy.
 *
 * @param display The Ecore_Wl2_Display to search within.
 * @param op The wl_output proxy to find.
 * @return The matching Ecore_Wl2_Output, or NULL if not found.
 */
Ecore_Wl2_Output *
_ecore_wl2_output_find(Ecore_Wl2_Display *display, struct wl_output *op)
{
   Ecore_Wl2_Output *wl2op;

   EINA_INLIST_FOREACH(display->outputs, wl2op)
     if (wl2op->wl_output == op) return wl2op;

   return NULL;
}

/**
 * @brief Gets the DPI (dots per inch) of the specified output.
 *
 * Calculates the DPI based on the physical dimensions (in millimeters)
 * and the current resolution (in pixels) of the output.
 *
 * @param output The Ecore_Wl2_Output to query.
 * @return The calculated DPI of the output. Returns 75 if physical
 *         dimensions are not available or invalid.
 *
 * @note The calculation assumes square pixels.
 */
EAPI int
ecore_wl2_output_dpi_get(Ecore_Wl2_Output *output)
{
   int w, h, mw, mh, dpi;
   double target;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 75);

   mw = output->mw;
   if (mw <= 0) return 75;

   mh = output->mh;
   if (mh <= 0) return 75;

   w = output->geometry.w;
   h = output->geometry.h;

   target = (round((sqrt(mw * mw + mh * mh) / 25.4) * 10) / 10);
   dpi = (round((sqrt(w * w + h * h) / target) * 10) / 10);

   return dpi;
}

/**
 * @brief Gets the current transformation of the specified output.
 *
 * The transformation indicates how the output's image is rotated or flipped.
 * Possible values are defined by `wl_output_transform`:
 * - WL_OUTPUT_TRANSFORM_NORMAL (0)
 * - WL_OUTPUT_TRANSFORM_90 (1)
 * - WL_OUTPUT_TRANSFORM_180 (2)
 * - WL_OUTPUT_TRANSFORM_270 (3)
 * - WL_OUTPUT_TRANSFORM_FLIPPED (4) - Note: Currently reported as an error.
 * - WL_OUTPUT_TRANSFORM_FLIPPED_90 (5) - Note: Currently reported as an error.
 * - WL_OUTPUT_TRANSFORM_FLIPPED_180 (6) - Note: Currently reported as an error.
 * - WL_OUTPUT_TRANSFORM_FLIPPED_270 (7) - Note: Currently reported as an error.
 *
 * This function currently only supports normal rotations (0-3).
 *
 * @param output The Ecore_Wl2_Output to query.
 * @return The current transformation value (0-3), or 0 if output is NULL.
 */
EAPI int
ecore_wl2_output_transform_get(Ecore_Wl2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   return output->transform;
}
