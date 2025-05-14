#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "ecore_x_private.h"
#include "Ecore_X.h"

#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>

static int _ecore_x_image_shm_can = -1; /**< Cache for SHM availability: -1 unk, 0 no, 1 yes */
static int _ecore_x_image_err = 0; /**< Flag to indicate if an X error occurred */

/**
 * @internal
 * @brief X error handler for image operations.
 *
 * This function is registered as a temporary X error handler during
 * certain image operations to detect failures. It sets the
 * global _ecore_x_image_err flag if an error occurs.
 *
 * @param d The display connection.
 * @param ev The XErrorEvent structure.
 * @return Always 0.
 */
static int
_ecore_x_image_error_handler(Display *d EINA_UNUSED, XErrorEvent *ev)
{
   _ecore_x_image_err = 1;
   switch (ev->error_code)
     {
      case BadRequest:	/* bad request code */
        ERR("BadRequest");
        break;
      case BadValue:	/* int parameter out of range */
        ERR("BadValue");
        break;
      case BadWindow:	/* parameter not a Window */
        ERR("BadWindow");
        break;
      case BadPixmap:	/* parameter not a Pixmap */
        ERR("BadPixmap");
        break;
      case BadAtom:	/* parameter not an Atom */
        ERR("BadAtom");
        break;
      case BadCursor:	/* parameter not a Cursor */
        ERR("BadCursor");
        break;
      case BadFont:	/* parameter not a Font */
        ERR("BadFont");
        break;
      case BadMatch:	/* parameter mismatch */
        ERR("BadMatch");
        break;
      case BadDrawable:	/* parameter not a Pixmap or Window */
        ERR("BadDrawable");
        break;
      case BadAccess:	/* depending on context */
        ERR("BadAccess");
        break;
      case BadAlloc:	/* insufficient resources */
        ERR("BadAlloc");
        break;
      case BadColor:	/* no such colormap */
        ERR("BadColor");
        break;
      case BadGC:	/* parameter not a GC */
        ERR("BadGC");
        break;
      case BadIDChoice:	/* choice not in range or already used */
        ERR("BadIDChoice");
        break;
      case BadName:	/* font or color name doesn't exist */
        ERR("BadName");
        break;
      case BadLength:	/* Request length incorrect */
        ERR("BadLength");
        break;
      case BadImplementation:	/* server is defective */
        ERR("BadImplementation");
        break;
     }
   return 0;
}

/**
 * @internal
 * @brief Checks if the X server supports MIT-SHM extension and it's usable.
 *
 * This function attempts to create a small SHM segment and attach it
 * to verify that SHM operations are working correctly. The result is
 * cached in _ecore_x_image_shm_can.
 *
 * @return 1 if SHM is available and usable, 0 otherwise.
 */
int
_ecore_x_image_shm_check(void)
{
   XErrorHandler ph;
   XShmSegmentInfo shminfo;
   XImage *xim;

   if (_ecore_x_image_shm_can != -1)
     return _ecore_x_image_shm_can;

   if (!XShmQueryExtension(_ecore_x_disp))
     {
        _ecore_x_image_shm_can = 0;
        return _ecore_x_image_shm_can;
     }

   XSync(_ecore_x_disp, False);
   _ecore_x_image_err = 0;

   xim = XShmCreateImage(_ecore_x_disp,
                         DefaultVisual(_ecore_x_disp,
                                       DefaultScreen(_ecore_x_disp)),
                         DefaultDepth(_ecore_x_disp,
                                      DefaultScreen(_ecore_x_disp)),
                         ZPixmap, NULL,
                         &shminfo, 1, 1);
   if (_ecore_xlib_sync) ecore_x_sync();
   if (!xim)
     {
        _ecore_x_image_shm_can = 0;
        return _ecore_x_image_shm_can;
     }

   shminfo.shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height,
                          IPC_CREAT | 0600);
   if (shminfo.shmid == -1)
     {
        ERR("%s", strerror(errno));
        XDestroyImage(xim);
        _ecore_x_image_shm_can = 0;
        return _ecore_x_image_shm_can;
     }

   shminfo.readOnly = False;
   shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
   xim->data = shminfo.shmaddr;

   if (xim->data == (char *)-1)
     {
        XDestroyImage(xim);
        _ecore_x_image_shm_can = 0;
        return _ecore_x_image_shm_can;
     }

   ph = XSetErrorHandler((XErrorHandler)_ecore_x_image_error_handler);
   XShmAttach(_ecore_x_disp, &shminfo);
   XShmGetImage(_ecore_x_disp, DefaultRootWindow(_ecore_x_disp),
                xim, 0, 0, 0xffffffff);
   XSync(_ecore_x_disp, False);
   XSetErrorHandler((XErrorHandler)ph);
   if (_ecore_x_image_err)
     {
        XShmDetach(_ecore_x_disp, &shminfo);
        XDestroyImage(xim);
        shmdt(shminfo.shmaddr);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        _ecore_x_image_shm_can = 0;
        return _ecore_x_image_shm_can;
     }

   XShmDetach(_ecore_x_disp, &shminfo);
   XDestroyImage(xim);
   shmdt(shminfo.shmaddr);
   shmctl(shminfo.shmid, IPC_RMID, 0);

   _ecore_x_image_shm_can = 1;
   return _ecore_x_image_shm_can;
}

struct _Ecore_X_Image
{
   XShmSegmentInfo shminfo;
   Ecore_X_Visual  vis;
   XImage         *xim;
   int             depth;
   int             w, h;
   int             bpl, bpp, rows;
   unsigned char  *data;
   Eina_Bool       shm : 1; /**< EINA_TRUE if SHM is used, EINA_FALSE otherwise */
};

/**
 * @brief Creates a new Ecore_X_Image.
 *
 * This function allocates and initializes an Ecore_X_Image structure.
 * It determines whether to use SHM based on server capabilities.
 * The actual XImage is not created until data is accessed or the image is used.
 *
 * @param w The width of the image.
 * @param h The height of the image.
 * @param vis The Ecore_X_Visual to use for the image. If NULL, the default visual is used.
 * @param depth The depth of the image. If 0, the default depth is used.
 * @return A pointer to the newly created Ecore_X_Image, or NULL on failure.
 *
 * @see ecore_x_image_free()
 */
EAPI Ecore_X_Image *
ecore_x_image_new(int w,
                  int h,
                  Ecore_X_Visual vis,
                  int depth)
{
   Ecore_X_Image *im;

   im = calloc(1, sizeof(Ecore_X_Image));
   if (!im)
     return NULL;

   LOGFN;
   im->w = w;
   im->h = h;
   im->vis = vis;
   im->depth = depth;
   if (depth <= 8) im->bpp = 1;
   else if (depth <= 16) im->bpp = 2;
   else if (depth <= 24) im->bpp = 3;
   else im->bpp = 4;
   _ecore_x_image_shm_check();
   im->shm = _ecore_x_image_shm_can;
   return im;
}

/**
 * @brief Frees an Ecore_X_Image.
 *
 * This function releases all resources associated with an Ecore_X_Image,
 * including any XImage data and SHM segments if used.
 *
 * @param im The Ecore_X_Image to free.
 *
 * @see ecore_x_image_new()
 */
EAPI void
ecore_x_image_free(Ecore_X_Image *im)
{
   LOGFN;
   if (im->shm)
     {
        if (im->xim)
          {
             XShmDetach(_ecore_x_disp, &(im->shminfo));
             XDestroyImage(im->xim);
             shmdt(im->shminfo.shmaddr);
             shmctl(im->shminfo.shmid, IPC_RMID, 0);
          }
     }
   else if (im->xim)
     {
        free(im->xim->data);
        im->xim->data = NULL;
        XDestroyImage(im->xim);
     }

   free(im);
}

/**
 * @internal
 * @brief Finalizes Ecore_X_Image properties after XImage creation.
 *
 * This function sets up internal fields like data pointer, bytes per line,
 * rows, and bytes per pixel based on the created XImage.
 *
 * @param im The Ecore_X_Image to finalize.
 */
static void
_ecore_x_image_finalize(Ecore_X_Image *im)
{
   im->data = (unsigned char *)im->xim->data;
   im->bpl = im->xim->bytes_per_line;
   im->rows = im->xim->height;
   if (im->xim->bits_per_pixel <= 8) im->bpp = 1;
   else if (im->xim->bits_per_pixel <= 16) im->bpp = 2;
   else if (im->xim->bits_per_pixel <= 24) im->bpp = 3;
   else im->bpp = 4;
}

/**
 * @internal
 * @brief Creates an XImage using SHM.
 *
 * This function allocates an XImage and associated SHM segment.
 * It handles SHM setup, attachment, and error checking.
 *
 * @param im The Ecore_X_Image for which to create the SHM XImage.
 *           The im->xim will be set on success.
 */
static void
_ecore_x_image_shm_create(Ecore_X_Image *im)
{
   im->xim = XShmCreateImage(_ecore_x_disp, im->vis, im->depth,
                             ZPixmap, NULL, &(im->shminfo),
                             im->w, im->h);
   if (!im->xim)
     return;

   im->shminfo.shmid = shmget(IPC_PRIVATE,
                              im->xim->bytes_per_line * im->xim->height,
                              IPC_CREAT | 0600);
   if (im->shminfo.shmid == -1)
     {
        ERR("shmget failed: %s", strerror(errno));
        XDestroyImage(im->xim);
        im->xim = NULL;
        return;
     }

   im->shminfo.readOnly = False;
   im->shminfo.shmaddr = shmat(im->shminfo.shmid, 0, 0);
   im->xim->data = im->shminfo.shmaddr;
   if ((im->xim->data == (char *)-1) ||
       (!im->xim->data))
     {
        ERR("shmat failed: %s", strerror(errno));
        shmdt(im->shminfo.shmaddr);
        shmctl(im->shminfo.shmid, IPC_RMID, 0);
        XDestroyImage(im->xim);
        im->xim = NULL;
        return;
     }

   XShmAttach(_ecore_x_disp, &im->shminfo);
   _ecore_x_image_finalize(im);
}

/**
 * @internal
 * @brief Creates an XImage without using SHM.
 *
 * This function allocates a standard XImage and its data buffer in client memory.
 *
 * @param im The Ecore_X_Image for which to create the XImage.
 *           The im->xim will be set on success.
 */
static void
_ecore_x_image_create(Ecore_X_Image *im)
{
   im->xim = XCreateImage(_ecore_x_disp, im->vis, im->depth,
                          ZPixmap, 0, NULL, im->w, im->h, 32, 0);
   if (!im->xim) return;
   im->xim->data = malloc(im->xim->bytes_per_line * im->h);
   if (!im->xim->data)
     {
        XDestroyImage(im->xim);
        im->xim = NULL;
        return;
     }
   _ecore_x_image_finalize(im);
}

/**
 * @brief Gets image data from a drawable into an Ecore_X_Image.
 *
 * This function copies a rectangular region from the given drawable
 * (Window or Pixmap) into the Ecore_X_Image. If SHM is used and the
 * source region matches the image dimensions and starts at (0,0) in the image,
 * an optimized path using XShmGetImage directly into the image buffer is taken.
 * Otherwise, if SHM is used but the regions don't align perfectly, a temporary
 * SHM image is created for the XShmGetImage call, and then the data is copied
 * to the target Ecore_X_Image. If SHM is not used, XGetSubImage is used.
 *
 * The Ecore_X_Image's internal XImage is created if it doesn't exist.
 *
 * @param im The Ecore_X_Image to store the data in.
 * @param draw The source Ecore_X_Drawable (Window or Pixmap).
 * @param x The x-coordinate in the drawable from which to get the image data.
 * @param y The y-coordinate in the drawable from which to get the image data.
 * @param sx The x-coordinate in the Ecore_X_Image where the retrieved data will be placed.
 * @param sy The y-coordinate in the Ecore_X_Image where the retrieved data will be placed.
 * @param w The width of the rectangular region to get.
 * @param h The height of the rectangular region to get.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_x_image_get(Ecore_X_Image *im,
                  Ecore_X_Drawable draw,
                  int x,
                  int y,
                  int sx,
                  int sy,
                  int w,
                  int h)
{
   Eina_Bool ret = EINA_TRUE;
   XErrorHandler ph;

   LOGFN;
   if (im->shm)
     {
        if (!im->xim) _ecore_x_image_shm_create(im);

        if (!im->xim)
          return EINA_FALSE;

        _ecore_x_image_err = 0;

        ecore_x_sync();
        // optimised path
        ph = XSetErrorHandler((XErrorHandler)_ecore_x_image_error_handler);
        if ((sx == 0) && (w == im->w))
          {
             im->xim->data = (char *)
               im->data + (im->xim->bytes_per_line * sy) + (sx * im->bpp);
             im->xim->width = MIN(w, im->w);
             im->xim->height = MIN(h, im->h);
             XGrabServer(_ecore_x_disp);
             if (!XShmGetImage(_ecore_x_disp, draw, im->xim, x, y, 0xffffffff))
               ret = EINA_FALSE;
             XUngrabServer(_ecore_x_disp);
             ecore_x_sync();
          }
        // unavoidable thanks to mit-shm get api - tmp shm buf + copy into it
        else
          {
             Ecore_X_Image *tim;
             unsigned char *spixels, *sp, *pixels, *p;
             int bpp, bpl, rows, sbpp, sbpl, srows;
             int r;

             tim = ecore_x_image_new(w, h, im->vis, im->depth);
             if (tim)
               {
                  ret = ecore_x_image_get(tim, draw, x, y, 0, 0, w, h);
                  if (ret)
                    {
                       spixels = ecore_x_image_data_get(tim,
                                                        &sbpl,
                                                        &srows,
                                                        &sbpp);
                       pixels = ecore_x_image_data_get(im, &bpl, &rows, &bpp);
                       if ((pixels) && (spixels))
                         {
                            p = pixels + (sy * bpl) + (sx * bpp);
                            sp = spixels;
                            for (r = srows; r > 0; r--)
                              {
                                 memcpy(p, sp, sbpl);
                                 p += bpl;
                                 sp += sbpl;
                              }
                         }
                    }

                  ecore_x_image_free(tim);
               }
          }

        XSetErrorHandler((XErrorHandler)ph);
        if (_ecore_x_image_err)
          ret = EINA_FALSE;
     }
   else
     {
        if (!im->xim)
          _ecore_x_image_create(im);

        if (!im->xim)
          return EINA_FALSE;

        if (XGetSubImage(_ecore_x_disp, draw, sx, sy, w, h,
                         0xffffffff, ZPixmap, im->xim, x, y) != im->xim)
          ret = EINA_FALSE;
     }

   return ret;
}

/**
 * @brief Puts image data from an Ecore_X_Image to a drawable.
 *
 * This function copies a rectangular region from the Ecore_X_Image
 * to the specified drawable (Window or Pixmap) using the given
 * graphics context (GC). If SHM is used, XShmPutImage is called,
 * otherwise XPutImage is used.
 *
 * The Ecore_X_Image's internal XImage is created if it doesn't exist.
 * If no GC is provided, a temporary one is created.
 *
 * @param im The source Ecore_X_Image.
 * @param draw The destination Ecore_X_Drawable (Window or Pixmap).
 * @param gc The Ecore_X_GC (Graphics Context) to use for the operation.
 *           If 0, a default GC is created and used.
 * @param x The x-coordinate in the drawable where the image data will be placed.
 * @param y The y-coordinate in the drawable where the image data will be placed.
 * @param sx The x-coordinate in the Ecore_X_Image from which to get the data.
 * @param sy The y-coordinate in the Ecore_X_Image from which to get the data.
 * @param w The width of the rectangular region to put.
 * @param h The height of the rectangular region to put.
 */
EAPI void
ecore_x_image_put(Ecore_X_Image *im,
                  Ecore_X_Drawable draw,
                  Ecore_X_GC gc,
                  int x,
                  int y,
                  int sx,
                  int sy,
                  int w,
                  int h)
{
   Ecore_X_GC tgc = 0;

   LOGFN;
   if (!gc)
     {
        XGCValues gcv;
        memset(&gcv, 0, sizeof(gcv));
        gcv.subwindow_mode = IncludeInferiors;
        tgc = XCreateGC(_ecore_x_disp, draw, GCSubwindowMode, &gcv);
        if (_ecore_xlib_sync) ecore_x_sync();
        gc = tgc;
     }
   if (!im->xim)
     {
        if (im->shm) _ecore_x_image_shm_create(im);
        else _ecore_x_image_create(im);
     }
   if (im->xim)
     {
        if (im->shm)
          XShmPutImage(_ecore_x_disp, draw, gc, im->xim,
                       sx, sy, x, y, w, h, False);
        else
          XPutImage(_ecore_x_disp, draw, gc, im->xim,
                    sx, sy, x, y, w, h);
        if (_ecore_xlib_sync) ecore_x_sync();
     }
   if (tgc) ecore_x_gc_free(tgc);
}

/**
 * @brief Gets a pointer to the raw image data of an Ecore_X_Image.
 *
 * This function provides direct access to the image pixel data.
 * The Ecore_X_Image's internal XImage is created if it doesn't exist.
 *
 * @param im The Ecore_X_Image.
 * @param bpl If not NULL, stores the number of bytes per line of the image data.
 * @param rows If not NULL, stores the number of rows (height) of the image data.
 * @param bpp If not NULL, stores the number of bytes per pixel of the image data.
 * @return A pointer to the raw image data, or NULL on failure or if the image
 *         has not been populated yet. The data format depends on the image's
 *         depth and visual.
 */
EAPI void *
ecore_x_image_data_get(Ecore_X_Image *im,
                       int *bpl,
                       int *rows,
                       int *bpp)
{
   LOGFN;
   if (!im->xim)
     {
        if (im->shm) _ecore_x_image_shm_create(im);
        else _ecore_x_image_create(im);
        if (!im->xim) return NULL;
     }
   if (bpl) *bpl = im->bpl;
   if (rows) *rows = im->rows;
   if (bpp) *bpp = im->bpp;
   return im->data;
}

/**
 * @brief Checks if the Ecore_X_Image is in a format compatible with ARGB32.
 *
 * This function determines if the image's visual and depth correspond to
 * a 32-bit ARGB format with specific masks (0xff0000 for red, 0x00ff00 for green,
 * 0x0000ff for blue) and considers the system's byte order and X server's
 * bitmap bit order.
 *
 * @param im The Ecore_X_Image to check.
 * @return EINA_TRUE if the image is ARGB32 compatible, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_image_is_argb32_get(Ecore_X_Image *im)
{
   Visual *vis = im->vis;
   if (((vis->class == TrueColor) ||
        (vis->class == DirectColor)) &&
       (im->bpp == 4) &&
       (vis->red_mask == 0xff0000) &&
       (vis->green_mask == 0x00ff00) &&
       (vis->blue_mask == 0x0000ff))
     {
#ifdef WORDS_BIGENDIAN
        if (BitmapBitOrder(_ecore_x_disp) == MSBFirst) return EINA_TRUE;
#else
        if (BitmapBitOrder(_ecore_x_disp) == LSBFirst) return EINA_TRUE;
#endif
     }
   return EINA_FALSE;
}

/**
 * @brief Converts image data from a source format to ARGB32 format.
 *
 * This function takes raw image data in a format described by the source
 * bits per pixel (sbpp), source bytes per line (sbpl), colormap (c), and
 * visual (v), and converts a specified rectangular region of it into a
 * 32-bit ARGB format (0xAARRGGBB, where alpha is always 0xFF).
 *
 * The supported source formats include:
 * - Indexed color (8-bit) if colormap and visual allow palette lookup.
 * - TrueColor/DirectColor:
 *   - 24-bit RGB (RR GG BB) or BGR (BB GG RR).
 *   - 32-bit ARGB (AA RR GG BB), ABGR (AA BB GG RR), RGBA (RR GG BB AA), BGRA (BB GG RR AA).
 *   - 18-bit (6 bits per channel) ARGBX666.
 *   - 16-bit RGB565, BGR565, RGBX555.
 *
 * @param src Pointer to the source image data.
 * @param sbpp Source bits per pixel (e.g., 8, 16, 24, 32). Note: this is converted to bytes internally.
 * @param sbpl Source bytes per line (stride).
 * @param c Source Ecore_X_Colormap (used for indexed color). Can be 0 for default.
 * @param v Source Ecore_X_Visual, describes the pixel format of @p src.
 * @param x X-coordinate of the top-left corner of the region in the source data.
 * @param y Y-coordinate of the top-left corner of the region in the source data.
 * @param w Width of the region to convert.
 * @param h Height of the region to convert.
 * @param dst Pointer to the destination buffer for ARGB32 data.
 *            The ARGB32 format is 0xffRRGGBB (alpha is opaque).
 *            Example: `unsigned int *pixels = malloc(w * h * sizeof(unsigned int));`
 * @param dbpl Destination bytes per line (stride) for @p dst.
 * @param dx X-coordinate of the top-left corner in the destination buffer.
 * @param dy Y-coordinate of the top-left corner in the destination buffer.
 * @return EINA_TRUE on successful conversion, EINA_FALSE otherwise (e.g., unsupported format).
 */
EAPI Eina_Bool
ecore_x_image_to_argb_convert(void *src,
                              int sbpp,
                              int sbpl,
                              Ecore_X_Colormap c,
                              Ecore_X_Visual v,
                              int x,
                              int y,
                              int w,
                              int h,
                              unsigned int *dst,
                              int dbpl,
                              int dx,
                              int dy)
{
   Visual *vis = v;
   XColor *cols = NULL;
   int n = 0, nret = 0, i, row;
   unsigned int pal[256], r, g, b;
   enum
   {
      rgbnone = 0,
      rgb565,
      bgr565,
      rgbx555,
      rgb888,
      bgr888,
      argbx888,
      abgrx888,
      rgba888x,
      bgra888x,
      argbx666
   };
   int mode = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(src, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);

   sbpp *= 8;

   n = vis->map_entries;
   if ((n <= 256) &&
       ((vis->class == PseudoColor) ||
        (vis->class == StaticColor) ||
        (vis->class == GrayScale) ||
        (vis->class == StaticGray)))
     {
        if (!c)
          c = DefaultColormap(_ecore_x_disp,
                              DefaultScreen(_ecore_x_disp));
        cols = alloca(n * sizeof(XColor));
        for (i = 0; i < n; i++)
          {
             cols[i].pixel = i;
             cols[i].flags = DoRed | DoGreen | DoBlue;
             cols[i].red = 0;
             cols[i].green = 0;
             cols[i].blue = 0;
          }
        XQueryColors(_ecore_x_disp, c, cols, n);
        for (i = 0; i < n; i++)
          {
             pal[i] = 0xff000000 |
               ((cols[i].red >> 8) << 16) |
               ((cols[i].green >> 8) << 8) |
               ((cols[i].blue >> 8));
          }
        nret = n;
     }
   else if ((vis->class == TrueColor) ||
            (vis->class == DirectColor))
     {
        if (sbpp == 24)
          {
             if ((vis->red_mask == 0x00ff0000) &&
                 (vis->green_mask == 0x0000ff00) &&
                 (vis->blue_mask == 0x000000ff))
               mode = rgb888;
             else if ((vis->red_mask == 0x000000ff) &&
                      (vis->green_mask == 0x0000ff00) &&
                      (vis->blue_mask == 0x00ff0000))
               mode = bgr888;
             else
               return EINA_FALSE;
          }
        else
          {
             if ((vis->red_mask == 0x00ff0000) &&
                 (vis->green_mask == 0x0000ff00) &&
                 (vis->blue_mask == 0x000000ff))
               mode = argbx888;
             else if ((vis->red_mask == 0x000000ff) &&
                      (vis->green_mask == 0x0000ff00) &&
                      (vis->blue_mask == 0x00ff0000))
               mode = abgrx888;
             else if ((vis->red_mask == 0xff000000) &&
                      (vis->green_mask == 0x00ff0000) &&
                      (vis->blue_mask == 0x0000ff00))
               mode = rgba888x;
             else if ((vis->red_mask == 0x0000ff00) &&
                      (vis->green_mask == 0x00ff0000) &&
                      (vis->blue_mask == 0xff000000))
               mode = bgra888x;
             else if ((vis->red_mask == 0x0003f000) &&
                      (vis->green_mask == 0x00000fc0) &&
                      (vis->blue_mask == 0x0000003f))
               mode = argbx666;
             else if ((vis->red_mask == 0x0000f800) &&
                      (vis->green_mask == 0x000007e0) &&
                      (vis->blue_mask == 0x0000001f))
               mode = rgb565;
             else if ((vis->red_mask == 0x0000001f) &&
                      (vis->green_mask == 0x000007e0) &&
                      (vis->blue_mask == 0x0000f800))
               mode = bgr565;
             else if ((vis->red_mask == 0x00007c00) &&
                      (vis->green_mask == 0x000003e0) &&
                      (vis->blue_mask == 0x0000001f))
               mode = rgbx555;
             else
               return EINA_FALSE;
          }
     }
   for (row = 0; row < h; row++)
     {
        unsigned char *s8;
        unsigned short *s16;
        unsigned int *s32;
        unsigned int *dp, *de;

        dp = ((unsigned int *)(((unsigned char *)dst) +
                               ((dy + row) * dbpl))) + dx;
        de = dp + w;
        switch (sbpp)
          {
           case 8:
             s8 = ((unsigned char *)(((unsigned char *)src) + ((y + row) * sbpl))) + x;
             if (nret > 0)
               {
                  while (dp < de)
                    {
                       *dp = pal[*s8];
                       s8++; dp++;
                    }
               }
             else
               return EINA_FALSE;
             break;

           case 16:
             s16 = ((unsigned short *)(((unsigned char *)src) + ((y + row) * sbpl))) + x;
             switch (mode)
               {
                case rgb565:
                  while (dp < de)
                    {
                       r = (*s16 & 0xf800) << 8;
                       g = (*s16 & 0x07e0) << 5;
                       b = (*s16 & 0x001f) << 3;
                       r |= (r >> 5) & 0xff0000;
                       g |= (g >> 6) & 0x00ff00;
                       b |= (b >> 5);
                       *dp = 0xff000000 | r | g | b;
                       s16++; dp++;
                    }
                  break;

                case bgr565:
                  while (dp < de)
                    {
                       r = (*s16 & 0x001f) << 19;
                       g = (*s16 & 0x07e0) << 5;
                       b = (*s16 & 0xf800) >> 8;
                       r |= (r >> 5) & 0xff0000;
                       g |= (g >> 6) & 0x00ff00;
                       b |= (b >> 5);
                       *dp = 0xff000000 | r | g | b;
                       s16++; dp++;
                    }
                  break;

                case rgbx555:
                  while (dp < de)
                    {
                       r = (*s16 & 0x7c00) << 9;
                       g = (*s16 & 0x03e0) << 6;
                       b = (*s16 & 0x001f) << 3;
                       r |= (r >> 5) & 0xff0000;
                       g |= (g >> 5) & 0x00ff00;
                       b |= (b >> 5);
                       *dp = 0xff000000 | r | g | b;
                       s16++; dp++;
                    }
                  break;

                default:
                  return EINA_FALSE;
                  break;
               }
             break;

           case 24:
             s8 = ((unsigned char *)(((unsigned char *)src) + ((y + row) * sbpl))) + (x * (sbpp / 8));
             switch (mode)
               {
                case rgb888:
                  while (dp < de)
                    {
                       *dp = 0xff000000 | (s8[2] << 16) | (s8[1] << 8) | s8[0];
                       s8 += 3; dp++;
                    }
                  break;
                case bgr888:
                  while (dp < de)
                    {
                       *dp = 0xff000000 | (s8[0] << 16) | (s8[1] << 8) | s8[2];
                       s8 += 3; dp++;
                    }
                  break;
                default:
                  return EINA_FALSE;
                  break;
               }
             break;

           case 32:
             s32 = ((unsigned int *)(((unsigned char *)src) + ((y + row) * sbpl))) + x;
             switch (mode)
               {
                case argbx888:
                  while (dp < de)
                    {
                       *dp = 0xff000000 | *s32;
                       s32++; dp++;
                    }
                  break;

                case abgrx888:
                  while (dp < de)
                    {
                       r = *s32 & 0x000000ff;
                       g = *s32 & 0x0000ff00;
                       b = *s32 & 0x00ff0000;
                       *dp = 0xff000000 | (r << 16) | (g) | (b >> 16);
                       s32++; dp++;
                    }
                  break;

                case rgba888x:
                  while (dp < de)
                    {
                       *dp = 0xff000000 | (*s32 >> 8);
                       s32++; dp++;
                    }
                  break;

                case bgra888x:
                  while (dp < de)
                    {
                       r = *s32 & 0x0000ff00;
                       g = *s32 & 0x00ff0000;
                       b = *s32 & 0xff000000;
                       *dp = 0xff000000 | (r << 8) | (g >> 8) | (b >> 24);
                       s32++; dp++;
                    }
                  break;

                case argbx666:
                  while (dp < de)
                    {
                       r = (*s32 & 0x3f000) << 6;
                       g = (*s32 & 0x00fc0) << 4;
                       b = (*s32 & 0x0003f) << 2;
                       r |= (r >> 6) & 0xff0000;
                       g |= (g >> 6) & 0x00ff00;
                       b |= (b >> 6);
                       *dp = 0xff000000 | r | g | b;
                       s32++; dp++;
                    }
                  break;

                default:
                  return EINA_FALSE;
                  break;
               }
             break;

           default:
             return EINA_FALSE;
             break;
          }
     }
   return EINA_TRUE;
}
