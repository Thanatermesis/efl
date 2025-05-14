#ifndef EVAS_XLIB_COLOR_H
#define EVAS_XLIB_COLOR_H

/**
 * @brief Initializes the Xlib color allocation system.
 *
 * Sets up the function pointers and color counts for different palette modes.
 * This function must be called before any color allocation attempts.
 */
void evas_software_xlib_x_color_init (void);

/**
 * @brief Allocates or retrieves a shared color palette for a given X display,
 * colormap, and visual.
 *
 * Attempts to find an existing compatible palette first. If not found, it tries
 * to allocate a new palette, starting from the requested `colors` mode and
 * falling back to simpler modes if allocation fails.
 *
 * @param disp The X display connection.
 * @param cmap The colormap to allocate colors from.
 * @param vis The visual associated with the colormap.
 * @param colors The desired palette mode (e.g., PAL_MODE_RGB332).
 * @return A pointer to the allocated or retrieved Convert_Pal structure on
 *         success, or NULL on failure. The structure contains the color
 *         lookup table and metadata. The caller should not free this directly
 *         but use evas_software_xlib_x_color_deallocate().
 */
Convert_Pal *evas_software_xlib_x_color_allocate (Display         *disp,
                                                  Colormap         cmap,
                                                  Visual          *vis,
                                                  Convert_Pal_Mode colors);

/**
 * @brief Deallocates a reference to a color palette.
 *
 * Decrements the reference count of the palette. If the count reaches zero,
 * it frees the allocated X colors and the palette structure itself.
 *
 * @param disp The X display connection (must match the one used for allocation).
 * @param cmap The colormap (must match the one used for allocation).
 * @param vis The visual (must match the one used for allocation).
 * @param pal The palette structure previously obtained from
 *            evas_software_xlib_x_color_allocate().
 */
void evas_software_xlib_x_color_deallocate (Display     *disp,
                                            Colormap     cmap,
                                            Visual      *vis,
                                            Convert_Pal *pal);

#endif
