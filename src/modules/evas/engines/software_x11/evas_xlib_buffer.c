/**
 * @file
 * @brief Implementation of Xlib output buffer handling for Evas software engine.
 *
 * This file contains functions for creating, managing, and drawing with
 * Xlib output buffers, including support for the MIT Shared Memory Extension (SHM)
 * for improved performance. It also includes functions for converting Evas's
 * 32-bit RGBA data into 1-bit Xlib masks.
 */
#include "evas_common_private.h"

#include "evas_xlib_buffer.h"

/** @brief Global flag to detect X errors during SHM attach test. */
static int _x_err = 0;

/**
 * @brief Writes a horizontal line of mask data (1-bit alpha) to an X output buffer.
 *
 * Converts the alpha channel of the source DATA32 pixels into a 1-bit mask
 * line in the destination X_Output_Buffer (which must be configured for 1-bit depth).
 * Handles X server bit swapping if necessary. Processes pixels from left to right.
 *
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the source DATA32 pixel data (alpha channel is used).
 * @param w The width of the line in pixels.
 * @param y The y-coordinate of the line within the X output buffer.
 */
void
evas_software_xlib_x_write_mask_line(Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int w, int y)
{
   int x;
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int bpl = 0;

   src_ptr = src;
   dst_ptr = evas_software_xlib_x_output_buffer_data(xob, &bpl);
   dst_ptr = dst_ptr + (bpl * y);
   w -= 7;
   if (buf->priv.x11.xlib.bit_swap)
     {
        for (x = 0; x < w; x += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[0])) >> 7) << 7) |
               ((A_VAL(&(src_ptr[1])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[2])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[3])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[4])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[5])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[6])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[7])) >> 7) << 0);
             src_ptr += 8;
             dst_ptr++;
          }
     }
   else
     {
        for (x = 0; x < w; x += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[0])) >> 7) << 0) |
               ((A_VAL(&(src_ptr[1])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[2])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[3])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[4])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[5])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[6])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[7])) >> 7) << 7);
             src_ptr += 8;
             dst_ptr++;
          }
     }
   w += 7;
   for (; x < w; x++)
     {
        XPutPixel(xob->xim, x, y, A_VAL(src_ptr) >> 7);
        src_ptr++;
     }
}

/**
 * @brief Writes a horizontal line of mask data (1-bit alpha) to an X output buffer, processing source pixels in reverse order.
 *
 * Converts the alpha channel of the source DATA32 pixels into a 1-bit mask
 * line in the destination X_Output_Buffer (which must be configured for 1-bit depth).
 * Handles X server bit swapping if necessary. Processes pixels from right to left.
 *
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the start of the source DATA32 pixel data (alpha channel is used). Processing starts from src + w - 1.
 * @param w The width of the line in pixels.
 * @param y The y-coordinate of the line within the X output buffer.
 */
void
evas_software_xlib_x_write_mask_line_rev(Outbuf *buf, X_Output_Buffer *xob, DATA32 *src, int w, int y)
{
   int x;
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int bpl = 0;

   src_ptr = src + w - 1;
   dst_ptr = evas_software_xlib_x_output_buffer_data(xob, &bpl);
   dst_ptr = dst_ptr + (bpl * y);
   w -= 7;
   if (buf->priv.x11.xlib.bit_swap)
     {
        for (x = 0; x < w; x += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[ 0])) >> 7) << 7) |
               ((A_VAL(&(src_ptr[-1])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[-2])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[-3])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[-4])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[-5])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[-6])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[-7])) >> 7) << 0);
             src_ptr -= 8;
             dst_ptr++;
          }
     }
   else
     {
        for (x = 0; x < w; x += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[ 0])) >> 7) << 0) |
               ((A_VAL(&(src_ptr[-1])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[-2])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[-3])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[-4])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[-5])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[-6])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[-7])) >> 7) << 7);
             src_ptr -= 8;
             dst_ptr++;
          }
     }
   w += 7;
   for (; x < w; x++)
     {
        XPutPixel(xob->xim, x, y, A_VAL(src_ptr) >> 7);
        src_ptr--;
     }
}

/**
 * @brief Writes a vertical line of mask data (1-bit alpha) to an X output buffer.
 *
 * Converts the alpha channel of the source DATA32 pixels (arranged vertically
 * according to the source width `w`) into a 1-bit mask line in the destination
 * X_Output_Buffer (which must be configured for 1-bit depth). The vertical
 * line in the source becomes a horizontal line segment in the destination mask.
 * Handles X server bit swapping if necessary. Processes pixels from top to bottom.
 *
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the source DATA32 pixel data (alpha channel is used).
 * @param h The height of the vertical line in pixels.
 * @param ym The y-coordinate within the X output buffer where the horizontal mask line segment starts (acts as the x-coordinate in the 1-bit mask image).
 * @param w The width (stride) of the source image data in pixels.
 */
void
evas_software_xlib_x_write_mask_line_vert(Outbuf *buf, X_Output_Buffer *xob,
                                          DATA32 *src,
                                          int h, int ym, int w)
{
   int y;
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int bpl = 0;

   src_ptr = src;
   dst_ptr = evas_software_xlib_x_output_buffer_data(xob, &bpl);
   dst_ptr = dst_ptr + (bpl * ym);
   h -= 7;
   if (buf->priv.x11.xlib.bit_swap)
     {
        for (y = 0; y < h; y += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[0 * w])) >> 7) << 7) |
               ((A_VAL(&(src_ptr[1 * w])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[2 * w])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[3 * w])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[4 * w])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[5 * w])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[6 * w])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[7 * w])) >> 7) << 0);
             src_ptr += 8 * w;
             dst_ptr++;
          }
     }
   else
     {
        for (y = 0; y < h; y += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[0 * w])) >> 7) << 0) |
               ((A_VAL(&(src_ptr[1 * w])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[2 * w])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[3 * w])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[4 * w])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[5 * w])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[6 * w])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[7 * w])) >> 7) << 7);
             src_ptr += 8 * w;
             dst_ptr++;
          }
     }
   h += 7;
   for (; y < h; y++)
     {
        XPutPixel(xob->xim, y, ym, A_VAL(src_ptr) >> 7);
        src_ptr += w;
     }
}

/**
 * @brief Writes a vertical line of mask data (1-bit alpha) to an X output buffer, processing source pixels in reverse vertical order.
 *
 * Converts the alpha channel of the source DATA32 pixels (arranged vertically
 * according to the source width `w`) into a 1-bit mask line in the destination
 * X_Output_Buffer (which must be configured for 1-bit depth). The vertical
 * line in the source becomes a horizontal line segment in the destination mask.
 * Handles X server bit swapping if necessary. Processes pixels from bottom to top.
 *
 * @param buf The Evas Outbuf (contains X display info like bit swap).
 * @param xob The target X output buffer (must be 1-bit depth).
 * @param src Pointer to the start of the source DATA32 pixel data (alpha channel is used). Processing starts from src + (h - 1) * w.
 * @param h The height of the vertical line in pixels.
 * @param ym The y-coordinate within the X output buffer where the horizontal mask line segment starts (acts as the x-coordinate in the 1-bit mask image).
 * @param w The width (stride) of the source image data in pixels.
 */
void
evas_software_xlib_x_write_mask_line_vert_rev(Outbuf *buf, X_Output_Buffer *xob,
                                              DATA32 *src,
                                              int h, int ym, int w)
{
   int y;
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int bpl = 0;

   src_ptr = src + ((h - 1) * w);
   dst_ptr = evas_software_xlib_x_output_buffer_data(xob, &bpl);
   dst_ptr = dst_ptr + (bpl * ym);
   h -= 7;
   if (buf->priv.x11.xlib.bit_swap)
     {
        for (y = 0; y < h; y += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[ 0 * w])) >> 7) << 7) |
               ((A_VAL(&(src_ptr[-1 * w])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[-2 * w])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[-3 * w])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[-4 * w])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[-5 * w])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[-6 * w])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[-7 * w])) >> 7) << 0);
             src_ptr -= 8 * w;
             dst_ptr++;
          }
     }
   else
     {
        for (y = 0; y < h; y += 8)
          {
             *dst_ptr =
               ((A_VAL(&(src_ptr[ 0 * w])) >> 7) << 0) |
               ((A_VAL(&(src_ptr[-1 * w])) >> 7) << 1) |
               ((A_VAL(&(src_ptr[-2 * w])) >> 7) << 2) |
               ((A_VAL(&(src_ptr[-3 * w])) >> 7) << 3) |
               ((A_VAL(&(src_ptr[-4 * w])) >> 7) << 4) |
               ((A_VAL(&(src_ptr[-5 * w])) >> 7) << 5) |
               ((A_VAL(&(src_ptr[-6 * w])) >> 7) << 6) |
               ((A_VAL(&(src_ptr[-7 * w])) >> 7) << 7);
             src_ptr -= 8 * w;
             dst_ptr++;
          }
     }
   h += 7;
   for (; y < h; y++)
     {
        XPutPixel(xob->xim, y, ym, A_VAL(src_ptr) >> 7);
        src_ptr -= w;
     }
}

/**
 * @brief Checks if the X server connection supports the MIT-SHM extension and if it's usable.
 *
 * This function queries the X server for SHM support and performs a test
 * allocation and attachment to ensure it works correctly. Results are cached
 * per display connection.
 *
 * @param d The X display connection.
 * @return 1 if SHM is available and usable, 0 otherwise.
 */
int
evas_software_xlib_x_can_do_shm(Display *d)
{
   static Display *cached_d = NULL;
   static int cached_result = 0;

   if (d == cached_d) return cached_result;
   cached_d = d;
   if (XShmQueryExtension(d))
     {
        X_Output_Buffer *xob;

        xob = evas_software_xlib_x_output_buffer_new
            (d, DefaultVisual(d, DefaultScreen(d)),
            DefaultDepth(d, DefaultScreen(d)), 16, 16, 2, NULL);
        if (!xob)
          {
             cached_result = 0;
             return 0;
          }
        evas_software_xlib_x_output_buffer_unref(xob, 1);
        cached_result = 1;
        return 1;
     }
   cached_result = 0;
   return 0;
}

/**
 * @brief Temporary X error handler used during SHM attach testing.
 *
 * Sets the global `_x_err` flag if an X error occurs. This allows the
 * SHM creation code to detect if `XShmAttach` failed silently.
 *
 * @param d The X display connection (unused).
 * @param ev The X error event (unused).
 */
static void
x_output_tmp_x_err(Display *d EINA_UNUSED, XErrorEvent *ev EINA_UNUSED)
{
   _x_err = 1;
   return;
}

//static int creates = 0; // Potential leftover debug counter

/**
 * @brief Creates a new X output buffer, potentially using SHM.
 *
 * Allocates an X_Output_Buffer structure and associated resources (XImage, SHM segment).
 * It attempts to use SHM if requested (`try_shm` > 0) and falls back to a standard
 * client-side XImage if SHM fails or is not requested.
 *
 * @param d The X display connection.
 * @param v The X visual to use.
 * @param depth The color depth of the buffer.
 * @param w The width of the buffer in pixels.
 * @param h The height of the buffer in pixels.
 * @param try_shm 0: Don't try SHM, 1: Try SHM, fallback to XImage, 2: Try SHM, fail if SHM attach fails (used for testing).
 * @param data Optional pointer to pre-allocated image data. If NULL, data is allocated internally (unless SHM is used). If SHM is used, this parameter is ignored.
 * @return A pointer to the newly created X_Output_Buffer, or NULL on failure.
 */
X_Output_Buffer *
evas_software_xlib_x_output_buffer_new(Display *d, Visual *v, int depth, int w, int h, int try_shm, void *data)
{
   X_Output_Buffer *xob;

   xob = calloc(1, sizeof(X_Output_Buffer));
   if (!xob) return NULL;

   xob->display = d;
   xob->visual = v;
   xob->xim = NULL;
   xob->shm_info = NULL;
   xob->w = w;
   xob->h = h;
   xob->refcount = 1;

   if (try_shm > 0)
     {
        xob->shm_info = malloc(sizeof(XShmSegmentInfo));
        if (xob->shm_info)
          {
             xob->xim = XShmCreateImage(d, v, depth, ZPixmap, NULL,
                                        xob->shm_info, w, h);
             if (xob->xim)
               {
                  xob->shm_info->shmid = shmget(IPC_PRIVATE,
                                                xob->xim->bytes_per_line *
                                                xob->xim->height,
                                                IPC_CREAT | 0600);
                  if (xob->shm_info->shmid >= 0)
                    {
                       xob->shm_info->readOnly = False;
                       xob->shm_info->shmaddr = xob->xim->data =
                           shmat(xob->shm_info->shmid, 0, 0);
                       if (xob->shm_info->shmaddr != ((void *)-1))
                         {
                            XErrorHandler ph;

                            if (try_shm == 2) // only needed during testing
                              {
                                 XSync(d, False);
                                 _x_err = 0;
                                 ph = XSetErrorHandler((void *)x_output_tmp_x_err);
                              }
#if defined(LIBXEXT_VERSION_LOW)
                            /* workaround for libXext of lower then 1.1.1 */
                            if (evas_common_frameq_enabled())
                              XLockDisplay(d);
#endif
                            XShmAttach(d, xob->shm_info);
#if defined(LIBXEXT_VERSION_LOW)
                            /* workaround for libXext of lower then 1.1.1 */
                            if (evas_common_frameq_enabled())
                              XUnlockDisplay(d);
#endif

                            if (try_shm == 2) // only needed during testing
                              {
                                 XSync(d, False);
                                 XSetErrorHandler((XErrorHandler)ph);
                              }
                            if (!_x_err)
                              {
                                 xob->bpl = xob->xim->bytes_per_line;
                                 xob->psize = xob->bpl * xob->h;
                                 return xob;
                              }
                         }
                       shmdt(xob->shm_info->shmaddr);
                       shmctl(xob->shm_info->shmid, IPC_RMID, 0);
                    }
                  if (xob->xim) XDestroyImage(xob->xim);
                  xob->xim = NULL;
               }
             if (xob->shm_info) free(xob->shm_info);
             xob->shm_info = NULL;
          }
     }

   if (try_shm > 1)
     {
        free(xob);
        return NULL;
     }

   xob->xim = XCreateImage(d, v, depth, ZPixmap, 0, data, w, h, 32, 0);
   if (!xob->xim)
     {
        free(xob);
        return NULL;
     }

   xob->data = data;

   if (!xob->xim->data)
     {
        xob->xim->data = malloc(xob->xim->bytes_per_line * xob->xim->height);
        if (!xob->xim->data)
          {
             XDestroyImage(xob->xim);
             free(xob);
             return NULL;
          }
     }
   xob->bpl = xob->xim->bytes_per_line;
   xob->psize = xob->bpl * xob->h;
   return xob;
}

/**
 * @brief Increments the reference count of an X output buffer.
 *
 * Used for managing the lifetime of the buffer when shared.
 *
 * @param xob The X output buffer to reference.
 * @return The same X_Output_Buffer pointer passed in, or NULL if refcount would overflow.
 */
X_Output_Buffer *
evas_software_xlib_x_output_buffer_ref(X_Output_Buffer *xob)
{
   if (xob->refcount == UINT_MAX)
     return NULL;
   xob->refcount++;
   return xob;
}

/**
 * @brief Decrements the reference count of an X output buffer and frees resources if the count reaches zero.
 *
 * Cleans up associated XImage, SHM segment (if used), and the buffer structure itself.
 *
 * @param xob The X output buffer to unreference.
 * @param psync If non-zero, performs an XSync before detaching SHM or destroying the image. This ensures X operations involving the buffer complete before its resources are released.
 */
void
evas_software_xlib_x_output_buffer_unref(X_Output_Buffer *xob, int psync)
{
   if (!xob->refcount)
     return;
   xob->refcount--;
   if (xob->refcount)
     return;
   if (xob->shm_info)
     {
        if (psync) XSync(xob->display, False);
        XShmDetach(xob->display, xob->shm_info);
        XDestroyImage(xob->xim);
        shmdt(xob->shm_info->shmaddr);
        shmctl(xob->shm_info->shmid, IPC_RMID, 0);
        free(xob->shm_info);
     }
   else
     {
        if (xob->data) xob->xim->data = NULL;
        XDestroyImage(xob->xim);
     }
   free(xob);
}

/**
 * @brief Pastes the content of an X output buffer onto an X drawable (Window or Pixmap).
 *
 * Uses `XShmPutImage` if the buffer uses SHM, otherwise uses `XPutImage`.
 *
 * @param xob The source X output buffer.
 * @param d The target X drawable.
 * @param gc The graphics context to use for the operation.
 * @param x The destination x-coordinate on the drawable.
 * @param y The destination y-coordinate on the drawable.
 * @param psync If non-zero, performs an XSync after putting the image to ensure the drawing operation completes immediately.
 */
void
evas_software_xlib_x_output_buffer_paste(X_Output_Buffer *xob, Drawable d, GC gc, int x, int y, int psync)
{
   if (xob->shm_info)
     {
        XShmPutImage(xob->display, d, gc, xob->xim, 0, 0, x, y,
                     xob->w, xob->h, False);
        if (psync) XSync(xob->display, False);
     }
   else
     {
        XPutImage(xob->display, d, gc, xob->xim, 0, 0, x, y,
                  xob->w, xob->h);
     }
}

/**
 * @brief Retrieves a pointer to the raw pixel data of an X output buffer.
 *
 * @param xob The X output buffer.
 * @param[out] bytes_per_line_ret Optional pointer to store the bytes per line (stride) of the image data.
 * @return A pointer to the raw pixel data (DATA8*), which is either the SHM segment address or the allocated buffer for a standard XImage.
 */
DATA8 *
evas_software_xlib_x_output_buffer_data(X_Output_Buffer *xob, int *bytes_per_line_ret)
{
   if (bytes_per_line_ret) *bytes_per_line_ret = xob->xim->bytes_per_line;
   return (DATA8 *)xob->xim->data;
}

/**
 * @brief Gets the color depth (bits per pixel) of the X output buffer.
 *
 * This corresponds to the `bits_per_pixel` field of the underlying XImage.
 *
 * @param xob The X output buffer.
 * @return The depth in bits per pixel.
 */
int
evas_software_xlib_x_output_buffer_depth(X_Output_Buffer *xob)
{
   return xob->xim->bits_per_pixel;
}

/**
 * @brief Gets the byte order (LSBFirst or MSBFirst) of the X output buffer's image data.
 *
 * This corresponds to the `byte_order` field of the underlying XImage.
 *
 * @param xob The X output buffer.
 * @return The byte order constant (e.g., LSBFirst, MSBFirst).
 */
int
evas_software_xlib_x_output_buffer_byte_order(X_Output_Buffer *xob)
{
   return xob->xim->byte_order;
}

/**
 * @brief Gets the bit order (LSBFirst or MSBFirst) of the X output buffer's image data.
 *
 * This corresponds to the `bitmap_bit_order` field of the underlying XImage,
 * primarily relevant for 1-bit depth images (bitmaps).
 *
 * @param xob The X output buffer.
 * @return The bitmap bit order constant (e.g., LSBFirst, MSBFirst).
 */
int
evas_software_xlib_x_output_buffer_bit_order(X_Output_Buffer *xob)
{
   return xob->xim->bitmap_bit_order;
}

