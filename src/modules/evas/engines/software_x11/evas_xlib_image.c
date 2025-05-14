#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_xlib_image.h"

/**
 * @brief Updates a region of the Evas image from its native Xlib source.
 *
 * This function is called to synchronize a portion of the Evas image
 * with the content of the underlying Xlib Pixmap. It fetches the pixel
 * data from the X server for the specified rectangle and updates the
 * Evas image buffer. If the XImage format is not ARGB32, a conversion
 * is performed.
 *
 * @param image Pointer to the RGBA_Image whose data needs to be updated.
 * @param x The x-coordinate of the top-left corner of the region to update.
 * @param y The y-coordinate of the top-left corner of the region to update.
 * @param w The width of the region to update.
 * @param h The height of the region to update.
 */
static void
evas_xlib_image_update(void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;
   char *pix;
   int bpl, rows, bpp;

   if (ecore_x_image_get(n->ns_data.x11.exim, n->ns_data.x11.pixmap, 0, 0, x, y, w, h))
     {
        pix = ecore_x_image_data_get(n->ns_data.x11.exim, &bpl, &rows, &bpp);
        if (!ecore_x_image_is_argb32_get(n->ns_data.x11.exim))
          {
             if (!im->image.data)
               im->image.data = (DATA32 *)malloc(im->cache_entry.w * im->cache_entry.h * sizeof(DATA32));
             Ecore_X_Colormap colormap = ecore_x_default_colormap_get(ecore_x_display_get(), ecore_x_default_screen_get());
             ecore_x_image_to_argb_convert(pix, bpp, bpl, colormap, n->ns_data.x11.visual,
                                           x, y, w, h,
                                           im->image.data, (w * sizeof(int)), 0, 0);
          }
        else
          {
             im->image.data = (DATA32 *)pix;
          }
     }
}

/**
 * @brief Callback function invoked when a native surface is bound to an image.
 *
 * This function is registered as the `bind` callback for an Evas image
 * that uses a native Xlib surface. It triggers an update of the image
 * data from the Xlib Pixmap for the specified region.
 *
 * @param image Pointer to the RGBA_Image.
 * @param x The x-coordinate of the region to update upon binding.
 * @param y The y-coordinate of the region to update upon binding.
 * @param w The width of the region to update upon binding.
 * @param h The height of the region to update upon binding.
 */
static void
_native_bind_cb(void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;

   if ((n) && (n->ns.type == EVAS_NATIVE_SURFACE_X11))
     {
        evas_xlib_image_update(image, x, y, w, h);
     }
}

/**
 * @brief Callback function invoked when a native surface is freed.
 *
 * This function is registered as the `free` callback for an Evas image
 * that uses a native Xlib surface. It is responsible for cleaning up
 * resources associated with the native surface, such as the XImage
 * and the internal Native data structure.
 *
 * @param image Pointer to the RGBA_Image whose native surface is being freed.
 */
static void
_native_free_cb(void *image)
{
   RGBA_Image *im = image;
   Native *n = im->native.data;

   if (n->ns_data.x11.exim)
     {
        ecore_x_image_free(n->ns_data.x11.exim);
        n->ns_data.x11.exim = NULL;
     }
   n->ns_data.x11.visual = NULL;

   im->native.data = NULL;
   im->native.func.bind = NULL;
   im->native.func.unbind = NULL;
   im->native.func.free = NULL;
   im->image.data = NULL;
   free(n);
}

/**
 * @brief Sets a native Xlib surface for an Evas image.
 *
 * This function configures an Evas image object to use an existing Xlib
 * Pixmap as its underlying data source. It allocates necessary internal
 * structures, creates an XImage for data transfer, and sets up callbacks
 * for binding and freeing the native surface.
 *
 * @param data The Evas engine data. Marked as EINA_UNUSED, indicating it's
 *             not directly used in this Xlib-specific implementation.
 * @param image Pointer to the Evas RGBA_Image object to be configured.
 *              This image's `native` member will be populated.
 * @param native Pointer to an Evas_Native_Surface structure describing the
 *               Xlib Pixmap and Visual to use.
 *               Example of Evas_Native_Surface structure for X11:
 *               typedef struct _Evas_Native_Surface_X11
 *               {
 *                 Pixmap pixmap;    // The X11 Pixmap ID
 *                 Visual *visual;  // The X11 Visual
 *                 Display *display; // The X11 Display (optional, can be NULL)
 *                 int screen_num;   // The X11 screen number (optional)
 *               } Evas_Native_Surface_X11;
 *
 *               Evas_Native_Surface ns;
 *               ns.type = EVAS_NATIVE_SURFACE_X11;
 *               ns.version = EVAS_NATIVE_SURFACE_VERSION;
 *               ns.data.x11.visual = xlib_visual_ptr;
 *               ns.data.x11.pixmap = (Pixmap)xlib_pixmap_id;
 * @return Returns the `image` pointer on success, or NULL on failure.
 *         On success, the `image`'s `native.data`, `native.func.bind`,
 *         and `native.func.free` members are set.
 */
void *
evas_xlib_image_native_set(void *data EINA_UNUSED, void *image, void *native)
{
   RGBA_Image *im = image;
   Evas_Native_Surface *ns = native;
   Native *n = NULL;
   Ecore_X_Image *exim = NULL;
   Visual *vis = NULL;
   Pixmap pm = 0;
   int w, h, depth;

   if ((ns) && (ns->type == EVAS_NATIVE_SURFACE_X11))
     {
        vis = ns->data.x11.visual;
        pm = ns->data.x11.pixmap;

        depth = ecore_x_drawable_depth_get(pm);

        w = im->cache_entry.w;
        h = im->cache_entry.h;

        n = calloc(1, sizeof(Native));
        if (!n) return NULL;

        exim = ecore_x_image_new(w, h, vis, depth);
        if (!exim)
          {
             ERR("ecore_x_image_new failed.");
             free(n);
             return NULL;
          }

        memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
        n->ns_data.x11.pixmap = pm;
        n->ns_data.x11.visual = vis;
        n->ns_data.x11.exim = exim;
        im->native.data = n;
        im->native.func.bind = _native_bind_cb;
        im->native.func.free = _native_free_cb;

        evas_xlib_image_update(image, 0, 0, w, h);
     }
   return im;
}

