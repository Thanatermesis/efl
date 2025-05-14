#include <string.h>

#include "evas_common_private.h"
#include "evas_engine.h"

/**
 * @brief Creates a new GDI output buffer.
 *
 * This function allocates and initializes a Gdi_Output_Buffer structure.
 * If `data` is NULL, it creates a new Device Independent Bitmap (DIB) section.
 * Otherwise, it uses the provided `data` pointer.
 *
 * @param dc The device context handle.
 * @param bitmap_info Pointer to the BITMAPINFO structure describing the bitmap format.
 *                    The structure's dimensions (biWidth, biHeight, biSizeImage)
 *                    might be updated if a new DIB section is created.
 * @param width The desired width of the buffer in pixels.
 * @param height The desired height of the buffer in pixels.
 * @param data Optional pointer to pre-allocated pixel data. If NULL, a new DIB
 *             section is created and its data pointer is used.
 * @return A pointer to the newly created Gdi_Output_Buffer, or NULL on failure.
 */
Gdi_Output_Buffer *
evas_software_gdi_output_buffer_new(HDC             dc,
                                    BITMAPINFO_GDI *bitmap_info,
                                    int             width,
                                    int             height,
                                    void           *data)
{
   Gdi_Output_Buffer *gdiob;

   gdiob = calloc(1, sizeof(Gdi_Output_Buffer));
   if (!gdiob) return NULL;

   if (!data)
     {
        bitmap_info->bih.biWidth = width;
        bitmap_info->bih.biHeight = -height;
        bitmap_info->bih.biSizeImage = 4 * width * height;
        gdiob->bitmap = CreateDIBSection(dc,
                                         (const BITMAPINFO *)bitmap_info,
                                         DIB_RGB_COLORS,
                                         (void **)(&data),
                                         NULL,
                                         0);
        if (!gdiob->bitmap)
          {
             free(gdiob);
             return NULL;
          }
     }

   gdiob->bitmap_info = bitmap_info;
   gdiob->dc = dc;
   gdiob->data = data;
   gdiob->width = width;
   gdiob->height = height;
   gdiob->pitch = width * 4;

   return gdiob;
}

/**
 * @brief Frees the resources associated with a GDI output buffer.
 *
 * This function deletes the GDI bitmap object and frees the memory
 * allocated for the Gdi_Output_Buffer structure.
 *
 * @param gdiob The GDI output buffer to free.
 */
void
evas_software_gdi_output_buffer_free(Gdi_Output_Buffer *gdiob)
{
   /* DeleteObject should be called only if we created the bitmap */
   /* Need to track ownership if pre-allocated data is used more often */
   DeleteObject(gdiob->bitmap);
   free(gdiob);
}

/**
 * @brief Pastes the GDI output buffer onto its associated device context.
 *
 * This function copies the contents of the GDI buffer's bitmap to the
 * target device context (`gdiob->dc`) at the specified coordinates (x, y).
 * It uses BitBlt with SRCCOPY for a direct pixel copy.
 *
 * @param gdiob The GDI output buffer containing the bitmap to paste.
 * @param x The x-coordinate of the top-left corner where the buffer should be pasted.
 * @param y The y-coordinate of the top-left corner where the buffer should be pasted.
 */
void
evas_software_gdi_output_buffer_paste(Gdi_Output_Buffer *gdiob,
                                      int                x,
                                      int                y)
{
   HDC     dc;
   HGDIOBJ obj;

   dc = CreateCompatibleDC(gdiob->dc);
   if (dc)
     {
        obj = SelectObject(dc, gdiob->bitmap);
        BitBlt(gdiob->dc,
               x, y,
               gdiob->width, gdiob->height,
               dc,
               0, 0,
               SRCCOPY);
        SelectObject(dc, obj);
        DeleteDC(dc);
     }
}

/**
 * @brief Retrieves the raw pixel data and pitch of the GDI output buffer.
 *
 * @param gdiob The GDI output buffer.
 * @param pitch Optional pointer to an integer where the buffer's pitch
 *              (bytes per scanline) will be stored.
 * @return A pointer to the raw pixel data (DATA8*).
 */
DATA8 *
evas_software_gdi_output_buffer_data(Gdi_Output_Buffer *gdiob,
                                     int               *pitch)
{
   if (pitch) *pitch = gdiob->pitch;
   return gdiob->data;
}
