#include "evas_engine.h"

/**
 * @file evas_xlib_swapper.h
 * @brief Functions for managing Xlib drawable swapping.
 *
 * This header defines the API for creating, managing, and using an X_Swapper
 * object, which facilitates buffer swapping for Xlib drawables. This can be
 * used for techniques like double or triple buffering.
 */

/**
 * @brief Opaque structure representing an Xlib swapper instance.
 *
 * This structure holds all the necessary information for managing
 * buffers associated with an Xlib drawable, including display connection,
 * drawable ID, visual information, and buffer details.
 * The actual definition is internal to evas_xlib_swapper.c.
 */
typedef struct _X_Swapper X_Swapper;

/**
 * @brief Creates a new Xlib swapper instance.
 *
 * Initializes a swapper for the given X display, drawable, visual,
 * depth, width, and height. This may involve allocating buffers
 * using XImage or DRI2, depending on availability and configuration.
 *
 * @param disp The X display connection.
 * @param draw The X drawable (e.g., a Window or Pixmap).
 * @param vis The X visual to be used.
 * @param depth The color depth of the drawable.
 * @param w The width of the drawable.
 * @param h The height of the drawable.
 * @return A pointer to the newly created X_Swapper instance, or NULL on failure.
 */
X_Swapper *evas_xlib_swapper_new(Display *disp, Drawable draw, Visual *vis,
                                 int depth, int w, int h);

/**
 * @brief Frees an Xlib swapper instance.
 *
 * Releases all resources associated with the swapper, including
 * any allocated buffers and X resources.
 *
 * @param swp The X_Swapper instance to free.
 */
void evas_xlib_swapper_free(X_Swapper *swp);

/**
 * @brief Maps the current drawing buffer for access.
 *
 * Provides a direct pointer to the pixel data of the current buffer
 * that can be drawn into.
 *
 * @param swp The X_Swapper instance.
 * @param[out] bpl Pointer to an integer to store the bytes per line of the buffer. Can be NULL.
 * @param[out] w Pointer to an integer to store the width of the buffer. Can be NULL.
 * @param[out] h Pointer to an integer to store the height of the buffer. Can be NULL.
 * @return A pointer to the raw pixel data of the buffer, or NULL on failure.
 *         The format of this data depends on the visual and depth.
 */
void *evas_xlib_swapper_buffer_map(X_Swapper *swp, int *bpl, int *w, int *h);

/**
 * @brief Unmaps the current drawing buffer.
 *
 * Call this after finishing drawing operations on the buffer obtained
 * from evas_xlib_swapper_buffer_map().
 *
 * @param swp The X_Swapper instance.
 */
void evas_xlib_swapper_buffer_unmap(X_Swapper *swp);

/**
 * @brief Swaps the buffers.
 *
 * Presents the content of the currently mapped buffer (which was drawn upon)
 * to the drawable. This typically involves an XPutImage, XShmPutImage, or
 * DRI2SwapBuffers operation. The specific regions to update can be specified.
 *
 * @param swp The X_Swapper instance.
 * @param rects An array of Eina_Rectangle structures defining the updated regions.
 *              Example: Eina_Rectangle rects[] = {{0, 0, 100, 100}, {200, 200, 50, 50}};
 * @param nrects The number of rectangles in the `rects` array.
 */
void evas_xlib_swapper_swap(X_Swapper *swp, Eina_Rectangle *rects, int nrects);

/**
 * @brief Gets the current buffer state for rendering optimization.
 *
 * Determines the optimal rendering strategy based on the state of
 * available buffers (e.g., if full redraw is needed, or if partial
 * updates like copy, double, triple, or quadruple buffering are possible).
 *
 * @param swp The X_Swapper instance.
 * @return A Render_Output_Swap_Mode enum value indicating the current buffer state.
 *         Example: MODE_DOUBLE, MODE_TRIPLE, MODE_FULL.
 */
Render_Output_Swap_Mode evas_xlib_swapper_buffer_state_get(X_Swapper *swp);

/**
 * @brief Gets the depth of the swapper's buffers.
 *
 * @param swp The X_Swapper instance.
 * @return The color depth (bits per pixel) of the buffers.
 */
int evas_xlib_swapper_depth_get(X_Swapper *swp);

/**
 * @brief Gets the byte order of the swapper's buffers.
 *
 * @param swp The X_Swapper instance.
 * @return The byte order (e.g., LSBFirst, MSBFirst).
 */
int evas_xlib_swapper_byte_order_get(X_Swapper *swp);

/**
 * @brief Gets the bit order of the swapper's buffers.
 *
 * @param swp The X_Swapper instance.
 * @return The bit order (e.g., LSBFirst, MSBFirst).
 */
int evas_xlib_swapper_bit_order_get(X_Swapper *swp);

