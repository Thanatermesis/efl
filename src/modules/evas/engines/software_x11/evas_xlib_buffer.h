/**
 * @file
 * @brief Functions and structures for handling Xlib output buffers, including SHM support.
 */

#ifndef EVAS_XLIB_BUFFER_H
#define EVAS_XLIB_BUFFER_H


#include "evas_engine.h"

/** @brief Opaque type for an Xlib output buffer. */
typedef struct _X_Output_Buffer X_Output_Buffer;

/**
 * @brief Structure representing an Xlib output buffer.
 *
 * This structure holds information about an XImage, potentially using
 * shared memory (SHM) for efficient data transfer.
 */
struct _X_Output_Buffer
{
   Display         *display;    /**< The X display connection. */
   XImage          *xim;        /**< The XImage structure used for pixel data. */
   XShmSegmentInfo *shm_info;   /**< Shared memory segment information (if SHM is used). NULL otherwise. */
   Visual          *visual;     /**< The X visual associated with the buffer. */
   void            *data;       /**< Pointer to external pixel data if not using SHM and not allocating internally. */
   int              w;          /**< Width of the buffer in pixels. */
   int              h;          /**< Height of the buffer in pixels. */
   int              bpl;        /**< Bytes per line (stride) of the pixel data. */
   int              psize;      /**< Total size of the pixel data in bytes (bpl * h). */
   unsigned int     refcount;   /**< Reference count for managing the buffer's lifetime. */
};

/**
 * @brief Writes a horizontal line of mask data (1-bit alpha) to an X output buffer.
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the source DATA32 pixel data (alpha channel is used).
 * @param w The width of the line in pixels.
 * @param y The y-coordinate of the line within the X output buffer.
 */
void evas_software_xlib_x_write_mask_line               (Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int w, int y);

/**
 * @brief Writes a horizontal line of mask data (1-bit alpha) to an X output buffer, processing source pixels in reverse order.
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the start of the source DATA32 pixel data (alpha channel is used). Processing starts from src + w - 1.
 * @param w The width of the line in pixels.
 * @param y The y-coordinate of the line within the X output buffer.
 */
void evas_software_xlib_x_write_mask_line_rev           (Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int w, int y);

/**
 * @brief Writes a vertical line of mask data (1-bit alpha) to an X output buffer.
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the source DATA32 pixel data (alpha channel is used).
 * @param h The height of the vertical line in pixels.
 * @param ym The y-coordinate within the X output buffer where the vertical line starts (acts as the x-coordinate in the 1-bit mask image).
 * @param w The width (stride) of the source image data in pixels.
 */
void evas_software_xlib_x_write_mask_line_vert          (Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int h, int ym, int w);

/**
 * @brief Writes a vertical line of mask data (1-bit alpha) to an X output buffer, processing source pixels in reverse vertical order.
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the start of the source DATA32 pixel data (alpha channel is used). Processing starts from src + (h - 1) * w.
 * @param h The height of the vertical line in pixels.
 * @param ym The y-coordinate within the X output buffer where the vertical line starts (acts as the x-coordinate in the 1-bit mask image).
 * @param w The width (stride) of the source image data in pixels.
 */
void evas_software_xlib_x_write_mask_line_vert_rev      (Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int h, int ym, int w);

/**
 * @brief Checks if the X server connection supports the MIT-SHM extension and if it's usable.
 * @param d The X display connection.
 * @return 1 if SHM is available and usable, 0 otherwise.
 */
int evas_software_xlib_x_can_do_shm                     (Display *d);

/**
 * @brief Creates a new X output buffer.
 * @param d The X display connection.
 * @param v The X visual to use.
 * @param depth The color depth of the buffer.
 * @param w The width of the buffer in pixels.
 * @param h The height of the buffer in pixels.
 * @param try_shm 0: Don't try SHM, 1: Try SHM, fallback to XImage, 2: Try SHM, fail if SHM attach fails (used for testing).
 * @param data Optional pointer to pre-allocated image data. If NULL, data is allocated internally (unless SHM is used). If SHM is used, this parameter is ignored.
 * @return A pointer to the newly created X_Output_Buffer, or NULL on failure.
 */
X_Output_Buffer *evas_software_xlib_x_output_buffer_new (Display *d, Visual *v, int depth, int w, int h, int try_shm, void *data);

/**
 * @brief Decrements the reference count of an X output buffer and frees it if the count reaches zero.
 * @param xob The X output buffer to unreference.
 * @param sync If non-zero, performs an XSync before detaching SHM or destroying the image.
 */
void evas_software_xlib_x_output_buffer_unref            (X_Output_Buffer *xob, int sync);

/**
 * @brief Increments the reference count of an X output buffer.
 * @param xob The X output buffer to reference.
 * @return The same X_Output_Buffer pointer passed in, or NULL if refcount would overflow.
 */
X_Output_Buffer *evas_software_xlib_x_output_buffer_ref(X_Output_Buffer *xob);

/**
 * @brief Pastes the content of an X output buffer onto an X drawable.
 * @param xob The source X output buffer.
 * @param d The target X drawable (Window or Pixmap).
 * @param gc The graphics context to use for the operation.
 * @param x The destination x-coordinate on the drawable.
 * @param y The destination y-coordinate on the drawable.
 * @param sync If non-zero, performs an XSync after putting the image.
 */
void evas_software_xlib_x_output_buffer_paste           (X_Output_Buffer *xob, Drawable d, GC gc, int x, int y, int sync);

/**
 * @brief Retrieves a pointer to the pixel data of an X output buffer.
 * @param xob The X output buffer.
 * @param[out] bytes_per_line_ret Optional pointer to store the bytes per line (stride) of the image data.
 * @return A pointer to the raw pixel data (DATA8*).
 */
DATA8 *evas_software_xlib_x_output_buffer_data          (X_Output_Buffer *xob, int *bytes_per_line_ret);

/**
 * @brief Gets the color depth (bits per pixel) of the X output buffer.
 * @param xob The X output buffer.
 * @return The depth in bits per pixel.
 */
int evas_software_xlib_x_output_buffer_depth            (X_Output_Buffer *xob);

/**
 * @brief Gets the byte order (LSBFirst or MSBFirst) of the X output buffer's image data.
 * @param xob The X output buffer.
 * @return The byte order constant (e.g., LSBFirst, MSBFirst).
 */
int evas_software_xlib_x_output_buffer_byte_order       (X_Output_Buffer *xob);

/**
 * @brief Gets the bit order (LSBFirst or MSBFirst) of the X output buffer's image data (relevant for bitmap formats).
 * @param xob The X output buffer.
 * @return The bitmap bit order constant (e.g., LSBFirst, MSBFirst).
 */
int evas_software_xlib_x_output_buffer_bit_order        (X_Output_Buffer *xob);


#endif
