#include "evas_common_private.h"
#include "evas_engine.h"

/**
 * @brief Initializes the Evas software GDI engine.
 *
 * This function sets up the necessary resources for rendering using GDI.
 * It gets the Device Context (DC) for the given window, checks for
 * compatible color depth, and initializes a BITMAPINFO structure for
 * bitmap operations.
 *
 * @param window The handle to the window where rendering will occur.
 * @param borderless A flag indicating if the window is borderless.
 * @param fullscreen A flag indicating if the window is fullscreen (currently unused).
 * @param region A flag indicating if the window has a custom region.
 * @param buf Pointer to the Outbuf structure to be initialized.
 * @return 1 on success, 0 on failure.
 */
int
evas_software_gdi_init (HWND         window,
                        unsigned int borderless,
                        unsigned int fullscreen EINA_UNUSED,
                        unsigned int region,
                        Outbuf *buf)
{
   if (!window)
     {
        ERR("[Engine] [GDI] Window is NULL");
        return 0;
     }

   buf->priv.gdi.window = window;
   buf->priv.gdi.dc = GetDC(window);
   buf->priv.gdi.borderless = borderless;
   buf->priv.gdi.region = region;
   if (!buf->priv.gdi.dc)
     {
        ERR("[Engine] [GDI] Can not get DC");
        return 0;
     }

   if (GetDeviceCaps(buf->priv.gdi.dc, BITSPIXEL) != 32)
     {
        ERR("[Engine] [GDI] no compatible depth");
        ReleaseDC(window, buf->priv.gdi.dc);
        return 0;
     }

   /* FIXME: support fullscreen */

   buf->priv.gdi.bitmap_info = (BITMAPINFO_GDI *)malloc(sizeof(BITMAPINFO_GDI));
   if (!buf->priv.gdi.bitmap_info)
     {
        ERR("[Engine] [GDI] Can not allocate bitmap info");
        ReleaseDC(window, buf->priv.gdi.dc);
        return 0;
     }

   buf->priv.gdi.bitmap_info->bih.biSize = sizeof(BITMAPINFOHEADER);
   buf->priv.gdi.bitmap_info->bih.biWidth = buf->width;
   buf->priv.gdi.bitmap_info->bih.biHeight = -buf->height;
   buf->priv.gdi.bitmap_info->bih.biPlanes = 1;
   buf->priv.gdi.bitmap_info->bih.biSizeImage = 4 * buf->width * buf->height;
   buf->priv.gdi.bitmap_info->bih.biXPelsPerMeter = 0;
   buf->priv.gdi.bitmap_info->bih.biYPelsPerMeter = 0;
   buf->priv.gdi.bitmap_info->bih.biClrUsed = 0;
   buf->priv.gdi.bitmap_info->bih.biClrImportant = 0;
   buf->priv.gdi.bitmap_info->bih.biBitCount = 32;
   buf->priv.gdi.bitmap_info->bih.biCompression = BI_BITFIELDS;
   buf->priv.gdi.bitmap_info->masks[0] = 0x00ff0000;
   buf->priv.gdi.bitmap_info->masks[1] = 0x0000ff00;
   buf->priv.gdi.bitmap_info->masks[2] = 0x000000ff;

   return 1;
}

/**
 * @brief Shuts down the Evas software GDI engine.
 *
 * This function releases all resources allocated by evas_software_gdi_init(),
 * including the bitmap information, the Device Context, and any GDI regions.
 *
 * @param buf Pointer to the Outbuf structure containing engine data.
 */
void
evas_software_gdi_shutdown(Outbuf *buf)
{
   if (!buf)
     return;

   free(buf->priv.gdi.bitmap_info);
   ReleaseDC(buf->priv.gdi.window, buf->priv.gdi.dc);
   if (buf->priv.gdi.regions)
     DeleteObject(buf->priv.gdi.regions);
}

/**
 * @brief Resizes the GDI bitmap information.
 *
 * This function updates the width, height, and image size in the
 * BITMAPINFOHEADER structure when the output buffer's dimensions change.
 *
 * @param buf Pointer to the Outbuf structure containing the bitmap information
 *            and new dimensions.
 */
void
evas_software_gdi_bitmap_resize(Outbuf *buf)
{
   buf->priv.gdi.bitmap_info->bih.biWidth = buf->width;
   buf->priv.gdi.bitmap_info->bih.biHeight = -buf->height;
   buf->priv.gdi.bitmap_info->bih.biSizeImage = 4 * buf->width * buf->height;
}
