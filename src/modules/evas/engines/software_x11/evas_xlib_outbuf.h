#ifndef EVAS_XLIB_OUTBUF_H
#define EVAS_XLIB_OUTBUF_H


#include "evas_engine.h"

/**
 * @brief Initializes the Xlib output buffer subsystem.
 *
 * Sets up global resources needed for managing output buffers,
 * particularly the SHM pool spinlock.
 */
void         evas_software_xlib_outbuf_init (void);

/**
 * @brief Frees an Xlib output buffer and associated resources.
 * @param buf The Outbuf structure to free.
 *
 * Releases pending writes, GCs, color palettes, and clears the SHM pool.
 */
void         evas_software_xlib_outbuf_free (Outbuf *buf);

/**
 * @brief Sets up an Xlib output buffer for rendering.
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @param rot Rotation angle (0, 90, 180, 270).
 * @param depth Output buffer depth (usually OUTBUF_DEPTH_INHERIT).
 * @param disp The X Display connection.
 * @param draw The target X Drawable (Window or Pixmap).
 * @param vis The X Visual to use.
 * @param cmap The X Colormap to use.
 * @param x_depth The depth of the target X Drawable.
 * @param grayscale Hint for palette allocation if using PseudoColor.
 * @param max_colors Hint for palette allocation size.
 * @param mask Optional Pixmap used as a shape mask.
 * @param shape_dither Boolean, enable dithering for shape mask.
 * @param destination_alpha Boolean, indicates if the destination drawable supports alpha.
 * @return A newly allocated Outbuf structure, or NULL on failure.
 *
 * Configures the output buffer based on the target X11 drawable properties.
 * Detects SHM availability, determines color masks or allocates palettes,
 * selects appropriate image conversion functions based on depth, rotation,
 * and palette usage. Initializes GCs for drawing.
 */
Outbuf      *evas_software_xlib_outbuf_setup_x (int          w,
                                                int          h,
                                                int          rot,
                                                Outbuf_Depth depth,
                                                Display     *disp,
                                                Drawable     draw,
                                                Visual      *vis,
                                                Colormap     cmap,
                                                int          x_depth,
                                                int          grayscale,
                                                int          max_colors,
                                                Pixmap       mask,
                                                int          shape_dither,
                                                int          destination_alpha);

/**
 * @brief Provides a buffer region for rendering an update area.
 * @param buf The Outbuf structure.
 * @param x X coordinate of the update region on the target drawable.
 * @param y Y coordinate of the update region on the target drawable.
 * @param w Width of the update region.
 * @param h Height of the update region.
 * @param[out] cx Pointer to store the X coordinate within the returned buffer.
 * @param[out] cy Pointer to store the Y coordinate within the returned buffer.
 * @param[out] cw Pointer to store the width within the returned buffer.
 * @param[out] ch Pointer to store the height within the returned buffer.
 * @return An RGBA_Image* pointer representing the image buffer to draw into, or NULL on failure.
 *
 * This function allocates or retrieves an appropriate image buffer (potentially
 * using SHM via _find_xob) where the engine can render the updated pixels.
 * If the "onebuf" optimization is active (a single large buffer is used),
 * it simply returns the existing buffer and records the rectangle.
 * Otherwise, it creates a new buffer (RGBA_Image) potentially backed by
 * an XOB (and mask XOB if needed) for the specific region requested.
 * The `cx, cy, cw, ch` parameters indicate the sub-area within the returned
 * RGBA_Image* that corresponds to the requested `x, y, w, h` area on the
 * target drawable. For non-"onebuf" mode, this is usually (0, 0, w, h).
 */
void  *evas_software_xlib_outbuf_new_region_for_update (Outbuf *buf,
                                                        int     x,
                                                        int     y,
                                                        int     w,
                                                        int     h,
                                                        int    *cx,
                                                        int    *cy,
                                                        int    *cw,
                                                        int    *ch);

/**
 * @brief Flushes updated regions to the target X Drawable.
 * @param buf The Outbuf structure.
 * @param surface_damage Damage regions on the surface (unused).
 * @param buffer_damage Damage regions in the buffer (unused).
 * @param render_mode Render mode hint (unused).
 *
 * Pushes the rendered data to the X server.
 * If "onebuf" mode is active, it combines all recorded update rectangles into
 * an X Region and performs a single XCopyArea (or equivalent SHM put) using
 * that region as a clip mask.
 * Otherwise, it iterates through the list of `pending_writes` (regions prepared
 * by `evas_software_xlib_outbuf_new_region_for_update` and populated by
 * `evas_software_xlib_outbuf_push_updated_region`) and pushes each one
 * individually using XCopyArea or SHM equivalent.
 * It manages the lifecycle of pending write buffers.
 */
void         evas_software_xlib_outbuf_flush (Outbuf *buf, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Frees resources during idle time.
 * @param buf The Outbuf structure.
 *
 * Releases buffers held for pending writes or the "onebuf" buffer.
 * Returns SHM buffers to the pool or frees non-SHM buffers.
 * Can clear the entire SHM pool.
 */
void         evas_software_xlib_outbuf_idle_flush (Outbuf *buf);

/**
 * @brief Pushes pixel data from an RGBA_Image update buffer to the corresponding XOB.
 * @param buf The Outbuf structure.
 * @param update The RGBA_Image containing the rendered pixel data for a region.
 *               This is the buffer returned by `evas_software_xlib_outbuf_new_region_for_update`.
 * @param x The original X coordinate of the update region (relative to the update buffer).
 * @param y The original Y coordinate of the update region (relative to the update buffer).
 * @param w The width of the update region.
 * @param h The height of the update region.
 *
 * Converts the RGBA pixel data from the `update` image into the format required
 * by the target X Drawable (using the appropriate conversion function selected
 * during setup) and writes it into the X Output Buffer (XOB) associated with
 * this update region (stored in `update->extended_info`). Also handles writing
 * mask data to the mask XOB if necessary, considering rotation.
 */
void         evas_software_xlib_outbuf_push_updated_region (Outbuf     *buf,
                                                            RGBA_Image *update,
                                                            int         x,
                                                            int         y,
                                                            int         w,
                                                            int         h);

/**
 * @brief Reconfigures the output buffer dimensions or rotation.
 * @param buf The Outbuf structure.
 * @param w New width.
 * @param h New height.
 * @param rot New rotation.
 * @param depth New depth (ignored, depth cannot change).
 *
 * Updates the buffer's internal dimensions and rotation settings. Adjusts
 * the SHM memory limit based on the new size and flushes idle resources.
 */
void         evas_software_xlib_outbuf_reconfigure (Outbuf      *buf,
                                                    int          w,
                                                    int          h,
                                                    int          rot,
                                                    Outbuf_Depth depth);

/** @brief Gets the current width of the output buffer. */
int          evas_software_xlib_outbuf_get_width (Outbuf *buf);

/** @brief Gets the current height of the output buffer. */
int          evas_software_xlib_outbuf_get_height (Outbuf *buf);

/** @brief Gets the depth setting of the output buffer (usually OUTBUF_DEPTH_INHERIT). */
Outbuf_Depth evas_software_xlib_outbuf_get_depth (Outbuf *buf);

/** @brief Gets the current rotation of the output buffer. */
int          evas_software_xlib_outbuf_get_rot (Outbuf *buf);

/**
 * @brief Sets the target X Drawable for the output buffer.
 * @param buf The Outbuf structure.
 * @param draw The new target X Drawable.
 *
 * Updates the drawable and recreates the main GC if the drawable changes.
 */
void         evas_software_xlib_outbuf_drawable_set (Outbuf  *buf,
                                                     Drawable draw);

/**
 * @brief Sets the target X Pixmap mask for the output buffer.
 * @param buf The Outbuf structure.
 * @param mask The new target X Pixmap mask (or None).
 *
 * Updates the mask pixmap and recreates the mask GC if the mask changes.
 */
void         evas_software_xlib_outbuf_mask_set (Outbuf *buf,
                                                 Pixmap mask);

/**
 * @brief Sets the rotation for the output buffer. (Deprecated/Internal?)
 * @param buf The Outbuf structure.
 * @param rot The new rotation value.
 *
 * Note: Reconfiguration should typically be done via evas_software_xlib_outbuf_reconfigure.
 */
void         evas_software_xlib_outbuf_rotation_set (Outbuf *buf,
                                                     int     rot);

/**
 * @brief Enables or disables debug visualization for updated regions.
 * @param buf The Outbuf structure.
 * @param debug Non-zero to enable debug, zero to disable.
 */
void         evas_software_xlib_outbuf_debug_set (Outbuf *buf,
                                                  int     debug);

/**
 * @brief Shows a flashing rectangle for debugging purposes.
 * @param buf The Outbuf structure.
 * @param draw The target drawable.
 * @param x X coordinate of the debug rectangle.
 * @param y Y coordinate of the debug rectangle.
 * @param w Width of the debug rectangle.
 * @param h Height of the debug rectangle.
 *
 * Flashes a rectangle on the specified drawable to visualize an area,
 * typically called when debug mode is enabled during flush operations.
 */
void         evas_software_xlib_outbuf_debug_show (Outbuf  *buf,
                                                   Drawable draw,
                                                   int      x,
                                                   int      y,
                                                   int      w,
                                                   int      h);

/**
 * @brief Checks if the output buffer is configured to use an alpha mask.
 * @param buf The Outbuf structure.
 * @return EINA_TRUE if a mask pixmap is set, EINA_FALSE otherwise.
 */
Eina_Bool    evas_software_xlib_outbuf_alpha_get (Outbuf *buf);

#endif
