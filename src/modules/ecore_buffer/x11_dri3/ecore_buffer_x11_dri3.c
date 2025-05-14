#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/xshmfence.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/sync.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Buffer.h>

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>

#include "ecore_buffer_private.h"

typedef struct _Ecore_Buffer_Module_X11_Dri3_Data Ecore_Buffer_Module_X11_Dri3_Data;
typedef struct _Ecore_Buffer_X11_Dri3_Data Ecore_Buffer_X11_Dri3_Data;

/**
 * @brief Module-specific data for the X11 DRI3 Ecore_Buffer backend.
 * This structure holds data global to the DRI3 buffer module, primarily
 * the Tizen Buffer Manager (tbm_bufmgr) instance.
 */
struct _Ecore_Buffer_Module_X11_Dri3_Data {
     tbm_bufmgr tbm_mgr; /**< The Tizen Buffer Manager instance. */
};

/**
 * @brief Buffer-specific data for an X11 DRI3 Ecore_Buffer.
 * This structure holds data for an individual buffer, including its X Pixmap,
 * TBM surface, dimensions, format, and flags.
 */
struct _Ecore_Buffer_X11_Dri3_Data {
     Ecore_X_Pixmap pixmap; /**< The X Pixmap associated with this buffer, if created. */
     void *tbm_surface; /**< Pointer to the TBM surface. */
     int w;
     int h;
     int stride;
     unsigned int flags;
     Ecore_Buffer_Format format;
     Eina_Bool is_imported; /**< Flag indicating if the buffer was imported (EINA_TRUE) or allocated (EINA_FALSE). */
};

/**
 * @brief Get the number of planes for a given buffer format.
 *
 * @param format The Ecore_Buffer_Format to query.
 * @return The number of planes for the format, or 0 if unknown.
 */
static int
_buf_get_num_planes(Ecore_Buffer_Format format)
{
   int num_planes = 0;

   switch (format)
     {
      case ECORE_BUFFER_FORMAT_C8:
      case ECORE_BUFFER_FORMAT_RGB332:
      case ECORE_BUFFER_FORMAT_BGR233:
      case ECORE_BUFFER_FORMAT_XRGB4444:
      case ECORE_BUFFER_FORMAT_XBGR4444:
      case ECORE_BUFFER_FORMAT_RGBX4444:
      case ECORE_BUFFER_FORMAT_BGRX4444:
      case ECORE_BUFFER_FORMAT_ARGB4444:
      case ECORE_BUFFER_FORMAT_ABGR4444:
      case ECORE_BUFFER_FORMAT_RGBA4444:
      case ECORE_BUFFER_FORMAT_BGRA4444:
      case ECORE_BUFFER_FORMAT_XRGB1555:
      case ECORE_BUFFER_FORMAT_XBGR1555:
      case ECORE_BUFFER_FORMAT_RGBX5551:
      case ECORE_BUFFER_FORMAT_BGRX5551:
      case ECORE_BUFFER_FORMAT_ARGB1555:
      case ECORE_BUFFER_FORMAT_ABGR1555:
      case ECORE_BUFFER_FORMAT_RGBA5551:
      case ECORE_BUFFER_FORMAT_BGRA5551:
      case ECORE_BUFFER_FORMAT_RGB565:
      case ECORE_BUFFER_FORMAT_BGR565:
      case ECORE_BUFFER_FORMAT_RGB888:
      case ECORE_BUFFER_FORMAT_BGR888:
      case ECORE_BUFFER_FORMAT_XRGB8888:
      case ECORE_BUFFER_FORMAT_XBGR8888:
      case ECORE_BUFFER_FORMAT_RGBX8888:
      case ECORE_BUFFER_FORMAT_BGRX8888:
      case ECORE_BUFFER_FORMAT_ARGB8888:
      case ECORE_BUFFER_FORMAT_ABGR8888:
      case ECORE_BUFFER_FORMAT_RGBA8888:
      case ECORE_BUFFER_FORMAT_BGRA8888:
      case ECORE_BUFFER_FORMAT_XRGB2101010:
      case ECORE_BUFFER_FORMAT_XBGR2101010:
      case ECORE_BUFFER_FORMAT_RGBX1010102:
      case ECORE_BUFFER_FORMAT_BGRX1010102:
      case ECORE_BUFFER_FORMAT_ARGB2101010:
      case ECORE_BUFFER_FORMAT_ABGR2101010:
      case ECORE_BUFFER_FORMAT_RGBA1010102:
      case ECORE_BUFFER_FORMAT_BGRA1010102:
      case ECORE_BUFFER_FORMAT_YUYV:
      case ECORE_BUFFER_FORMAT_YVYU:
      case ECORE_BUFFER_FORMAT_UYVY:
      case ECORE_BUFFER_FORMAT_VYUY:
      case ECORE_BUFFER_FORMAT_AYUV:
         num_planes = 1;
         break;
      case ECORE_BUFFER_FORMAT_NV12:
      case ECORE_BUFFER_FORMAT_NV21:
      case ECORE_BUFFER_FORMAT_NV16:
      case ECORE_BUFFER_FORMAT_NV61:
         num_planes = 2;
         break;
      case ECORE_BUFFER_FORMAT_YUV410:
      case ECORE_BUFFER_FORMAT_YVU410:
      case ECORE_BUFFER_FORMAT_YUV411:
      case ECORE_BUFFER_FORMAT_YVU411:
      case ECORE_BUFFER_FORMAT_YUV420:
      case ECORE_BUFFER_FORMAT_YVU420:
      case ECORE_BUFFER_FORMAT_YUV422:
      case ECORE_BUFFER_FORMAT_YVU422:
      case ECORE_BUFFER_FORMAT_YUV444:
      case ECORE_BUFFER_FORMAT_YVU444:
         num_planes = 3;
         break;

      default :
         break;
     }

   return num_planes;
}

/**
 * @brief Get the bits per pixel (bpp) for a given buffer format.
 *
 * This function returns the total bits per pixel for a format, which might
 * differ from the depth (bits used for color information). For planar
 * formats, this typically represents the bpp of the combined planes if
 * they were interleaved, or a representative value.
 *
 * @param format The Ecore_Buffer_Format to query.
 * @return The bits per pixel for the format, or 0 if unknown.
 */
static int
_buf_get_bpp(Ecore_Buffer_Format format)
{
   int bpp = 0;

   switch (format)
     {
      case ECORE_BUFFER_FORMAT_C8:
      case ECORE_BUFFER_FORMAT_RGB332:
      case ECORE_BUFFER_FORMAT_BGR233:
         bpp = 8;
         break;
      case ECORE_BUFFER_FORMAT_XRGB4444:
      case ECORE_BUFFER_FORMAT_XBGR4444:
      case ECORE_BUFFER_FORMAT_RGBX4444:
      case ECORE_BUFFER_FORMAT_BGRX4444:
      case ECORE_BUFFER_FORMAT_ARGB4444:
      case ECORE_BUFFER_FORMAT_ABGR4444:
      case ECORE_BUFFER_FORMAT_RGBA4444:
      case ECORE_BUFFER_FORMAT_BGRA4444:
      case ECORE_BUFFER_FORMAT_XRGB1555:
      case ECORE_BUFFER_FORMAT_XBGR1555:
      case ECORE_BUFFER_FORMAT_RGBX5551:
      case ECORE_BUFFER_FORMAT_BGRX5551:
      case ECORE_BUFFER_FORMAT_ARGB1555:
      case ECORE_BUFFER_FORMAT_ABGR1555:
      case ECORE_BUFFER_FORMAT_RGBA5551:
      case ECORE_BUFFER_FORMAT_BGRA5551:
      case ECORE_BUFFER_FORMAT_RGB565:
      case ECORE_BUFFER_FORMAT_BGR565:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_RGB888:
      case ECORE_BUFFER_FORMAT_BGR888:
         bpp = 24;
         break;
      case ECORE_BUFFER_FORMAT_XRGB8888:
      case ECORE_BUFFER_FORMAT_XBGR8888:
      case ECORE_BUFFER_FORMAT_RGBX8888:
      case ECORE_BUFFER_FORMAT_BGRX8888:
      case ECORE_BUFFER_FORMAT_ARGB8888:
      case ECORE_BUFFER_FORMAT_ABGR8888:
      case ECORE_BUFFER_FORMAT_RGBA8888:
      case ECORE_BUFFER_FORMAT_BGRA8888:
      case ECORE_BUFFER_FORMAT_XRGB2101010:
      case ECORE_BUFFER_FORMAT_XBGR2101010:
      case ECORE_BUFFER_FORMAT_RGBX1010102:
      case ECORE_BUFFER_FORMAT_BGRX1010102:
      case ECORE_BUFFER_FORMAT_ARGB2101010:
      case ECORE_BUFFER_FORMAT_ABGR2101010:
      case ECORE_BUFFER_FORMAT_RGBA1010102:
      case ECORE_BUFFER_FORMAT_BGRA1010102:
      case ECORE_BUFFER_FORMAT_YUYV:
      case ECORE_BUFFER_FORMAT_YVYU:
      case ECORE_BUFFER_FORMAT_UYVY:
      case ECORE_BUFFER_FORMAT_VYUY:
      case ECORE_BUFFER_FORMAT_AYUV:
         bpp = 32;
         break;
      case ECORE_BUFFER_FORMAT_NV12:
      case ECORE_BUFFER_FORMAT_NV21:
         bpp = 12;
         break;
      case ECORE_BUFFER_FORMAT_NV16:
      case ECORE_BUFFER_FORMAT_NV61:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_YUV410:
      case ECORE_BUFFER_FORMAT_YVU410:
         bpp = 9;
         break;
      case ECORE_BUFFER_FORMAT_YUV411:
      case ECORE_BUFFER_FORMAT_YVU411:
      case ECORE_BUFFER_FORMAT_YUV420:
      case ECORE_BUFFER_FORMAT_YVU420:
         bpp = 12;
         break;
      case ECORE_BUFFER_FORMAT_YUV422:
      case ECORE_BUFFER_FORMAT_YVU422:
         bpp = 16;
         break;
      case ECORE_BUFFER_FORMAT_YUV444:
      case ECORE_BUFFER_FORMAT_YVU444:
         bpp = 24;
         break;
      default :
         break;
     }

   return bpp;
}

/**
 * @brief Get the color depth for a given buffer format.
 *
 * The depth refers to the number of bits used to represent the color
 * information of a single pixel, excluding padding bits. For YUV formats,
 * this function returns 0 as X11 typically doesn't have a direct
 * representation for their depth.
 *
 * @param format The Ecore_Buffer_Format to query.
 * @return The color depth for the format, or 0 if unknown or not applicable to X.
 */
static int
_buf_get_depth(Ecore_Buffer_Format format)
{
   int depth = 0;

   switch (format)
     {
      case ECORE_BUFFER_FORMAT_C8:
      case ECORE_BUFFER_FORMAT_RGB332:
      case ECORE_BUFFER_FORMAT_BGR233:
         depth = 8;
         break;
      case ECORE_BUFFER_FORMAT_XRGB4444:
      case ECORE_BUFFER_FORMAT_XBGR4444:
      case ECORE_BUFFER_FORMAT_RGBX4444:
      case ECORE_BUFFER_FORMAT_BGRX4444:
         depth = 12;
         break;
      case ECORE_BUFFER_FORMAT_ARGB4444:
      case ECORE_BUFFER_FORMAT_ABGR4444:
      case ECORE_BUFFER_FORMAT_RGBA4444:
      case ECORE_BUFFER_FORMAT_BGRA4444:
         depth = 16;
         break;
      case ECORE_BUFFER_FORMAT_XRGB1555:
      case ECORE_BUFFER_FORMAT_XBGR1555:
      case ECORE_BUFFER_FORMAT_RGBX5551:
      case ECORE_BUFFER_FORMAT_BGRX5551:
         depth = 15;
         break;
      case ECORE_BUFFER_FORMAT_ARGB1555:
      case ECORE_BUFFER_FORMAT_ABGR1555:
      case ECORE_BUFFER_FORMAT_RGBA5551:
      case ECORE_BUFFER_FORMAT_BGRA5551:
      case ECORE_BUFFER_FORMAT_RGB565:
      case ECORE_BUFFER_FORMAT_BGR565:
         depth = 16;
         break;
      case ECORE_BUFFER_FORMAT_RGB888:
      case ECORE_BUFFER_FORMAT_BGR888:
         depth = 24;
         break;
      case ECORE_BUFFER_FORMAT_XRGB8888:
      case ECORE_BUFFER_FORMAT_XBGR8888:
      case ECORE_BUFFER_FORMAT_RGBX8888:
      case ECORE_BUFFER_FORMAT_BGRX8888:
         depth = 24;
         break;
      case ECORE_BUFFER_FORMAT_ARGB8888:
      case ECORE_BUFFER_FORMAT_ABGR8888:
      case ECORE_BUFFER_FORMAT_RGBA8888:
      case ECORE_BUFFER_FORMAT_BGRA8888:
         depth = 32;
         break;
      case ECORE_BUFFER_FORMAT_XRGB2101010:
      case ECORE_BUFFER_FORMAT_XBGR2101010:
      case ECORE_BUFFER_FORMAT_RGBX1010102:
      case ECORE_BUFFER_FORMAT_BGRX1010102:
         depth = 30;
         break;
      case ECORE_BUFFER_FORMAT_ARGB2101010:
      case ECORE_BUFFER_FORMAT_ABGR2101010:
      case ECORE_BUFFER_FORMAT_RGBA1010102:
      case ECORE_BUFFER_FORMAT_BGRA1010102:
         depth = 32;
         break;
      case ECORE_BUFFER_FORMAT_YUYV:
      case ECORE_BUFFER_FORMAT_YVYU:
      case ECORE_BUFFER_FORMAT_UYVY:
      case ECORE_BUFFER_FORMAT_VYUY:
      case ECORE_BUFFER_FORMAT_AYUV:
      case ECORE_BUFFER_FORMAT_NV12:
      case ECORE_BUFFER_FORMAT_NV21:
      case ECORE_BUFFER_FORMAT_NV16:
      case ECORE_BUFFER_FORMAT_NV61:
      case ECORE_BUFFER_FORMAT_YUV410:
      case ECORE_BUFFER_FORMAT_YVU410:
      case ECORE_BUFFER_FORMAT_YUV411:
      case ECORE_BUFFER_FORMAT_YVU411:
      case ECORE_BUFFER_FORMAT_YUV420:
      case ECORE_BUFFER_FORMAT_YVU420:
      case ECORE_BUFFER_FORMAT_YUV422:
      case ECORE_BUFFER_FORMAT_YVU422:
      case ECORE_BUFFER_FORMAT_YUV444:
      case ECORE_BUFFER_FORMAT_YVU444:
      default :
         depth = 0; //unknown in X
         break;
     }

   return depth;
}

/**
 * @brief Opens a connection to a DRI3 provider.
 *
 * This function uses the XCB DRI3 extension to open a device file descriptor
 * for a given provider (typically the display driver).
 *
 * @param dpy The Ecore_X_Display connection.
 * @param root The root window of the screen.
 * @param provider The provider to open (usually 0 for the default).
 * @return The file descriptor on success, or -1 on error.
 */
static int
_dri3_open(Ecore_X_Display *dpy, Ecore_X_Window root, unsigned provider)
{
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_dri3_open_cookie_t cookie;
   xcb_dri3_open_reply_t *reply;

   cookie = xcb_dri3_open(c, root, provider);
   reply = xcb_dri3_open_reply(c, cookie, NULL);
   if ((!reply) || (reply->nfd != 1))
     return -1;

   return xcb_dri3_open_reply_fds(c, reply)[0];
}

/**
 * @brief Creates an X Pixmap from a file descriptor using DRI3.
 *
 * This function utilizes the xcb_dri3_pixmap_from_buffer request to create
 * an X Pixmap from a DMA-BUF file descriptor.
 *
 * @param dpy The Ecore_X_Display connection.
 * @param draw The X Drawable (often the root window) to associate with the pixmap.
 * @param width The width of the pixmap.
 * @param height The height of the pixmap.
 * @param depth The color depth of the pixmap.
 * @param fd The file descriptor representing the buffer.
 * @param bpp The bits per pixel of the buffer.
 * @param stride The stride (bytes per row) of the buffer.
 * @param size The total size in bytes of the buffer.
 * @return The Ecore_X_Pixmap on success, or 0 on error.
 */
static Ecore_X_Pixmap
_dri3_pixmap_from_fd(Ecore_X_Display *dpy, Ecore_X_Drawable draw, int width, int height, int depth, int fd, int bpp, int stride, int size)
{
   xcb_connection_t *c = XGetXCBConnection(dpy);
   Ecore_X_Pixmap pixmap = xcb_generate_id(c);

   if (!dpy)
     return 0;

   if (!c)
     return 0;

   if (!pixmap)
     return 0;

   xcb_dri3_pixmap_from_buffer(c, pixmap, draw, size, width, height, stride, depth, bpp, fd);

   return pixmap;
}

/**
 * @brief Initializes the Ecore_Buffer X11 DRI3 backend.
 *
 * This function sets up the necessary X11 connection, opens the DRI3 device,
 * and initializes the Tizen Buffer Manager (TBM).
 *
 * @param context Optional context string (unused).
 * @param options Optional options string (unused).
 * @return A pointer to Ecore_Buffer_Module_Data on success, NULL on failure.
 *         The returned data is an Ecore_Buffer_Module_X11_Dri3_Data struct.
 */
static Ecore_Buffer_Module_Data
_ecore_buffer_x11_dri3_init(const char *context EINA_UNUSED, const char *options EINA_UNUSED)
{
   Ecore_X_Display *xdpy;
   Ecore_X_Window root;
   Ecore_Buffer_Module_X11_Dri3_Data *mdata = NULL;
   int fd = 0;

   if (!ecore_x_init(NULL))
     return NULL;

   xdpy = ecore_x_display_get();
   if (!xdpy)
     goto on_error;

   root = ecore_x_window_root_first_get();
   if (!root)
     goto on_error;

   mdata = calloc(1, sizeof(Ecore_Buffer_Module_X11_Dri3_Data));
   if (!mdata)
     goto on_error;

   //Init DRI3 and TBM
   fd = _dri3_open(xdpy, root, 0);
   if (fd < 0)
     goto on_error;

   mdata->tbm_mgr = tbm_bufmgr_init(fd);
   if (!mdata->tbm_mgr)
     goto on_error;

   close(fd);

   return mdata;

on_error:
   if (fd > 0) close(fd);
   if (mdata) free(mdata);
   ecore_x_shutdown();

   return NULL;
}

/**
 * @brief Shuts down the Ecore_Buffer X11 DRI3 backend.
 *
 * This function deinitializes the Tizen Buffer Manager and closes the X11
 * connection.
 *
 * @param bmdata The module data (Ecore_Buffer_Module_X11_Dri3_Data)
 *               obtained from _ecore_buffer_x11_dri3_init.
 */
static void
_ecore_buffer_x11_dri3_shutdown(Ecore_Buffer_Module_Data bmdata)
{
   Ecore_Buffer_Module_X11_Dri3_Data *bm = bmdata;

   if (!bm)
     return;

   if (bm->tbm_mgr)
     tbm_bufmgr_deinit(bm->tbm_mgr);

   ecore_x_shutdown();
}

/**
 * @brief Allocates a new buffer using the X11 DRI3 backend.
 *
 * This function creates a TBM surface for the buffer. The actual X Pixmap
 * is typically created on demand when _ecore_buffer_x11_dri3_pixmap_get is called.
 *
 * @param bmdata Module data (unused in this function but part of the interface).
 * @param width The desired width of the buffer.
 * @param height The desired height of the buffer.
 * @param format The desired Ecore_Buffer_Format of the buffer.
 * @param flags Creation flags for the buffer (e.g., TBM_BO_SCANOUT).
 * @return A pointer to Ecore_Buffer_Data on success, NULL on failure.
 *         The returned data is an Ecore_Buffer_X11_Dri3_Data struct.
 */
static Ecore_Buffer_Data
_ecore_buffer_x11_dri3_buffer_alloc(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, int width, int height, Ecore_Buffer_Format format, unsigned int flags)
{
   Ecore_Buffer_X11_Dri3_Data *buf;

   buf = calloc(1, sizeof(Ecore_Buffer_X11_Dri3_Data));
   if (!buf)
     return NULL;

   buf->w = width;
   buf->h = height;
   buf->format = format;
   buf->flags = flags;
   buf->is_imported = EINA_FALSE;
   buf->tbm_surface = tbm_surface_create(width,height,(tbm_format)format);
   if (!buf->tbm_surface)
     {
        free(buf);
        return NULL;
     }

   return buf;
}

/**
 * @brief Frees a buffer allocated by the X11 DRI3 backend.
 *
 * This function releases the associated X Pixmap (if any) and destroys
 * the TBM surface.
 *
 * @param bmdata Module data (unused in this function but part of the interface).
 * @param bdata The buffer data (Ecore_Buffer_X11_Dri3_Data) to free.
 */
static void
_ecore_buffer_x11_dri3_buffer_free(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata)
{
   Ecore_Buffer_X11_Dri3_Data *buf = bdata;

   if (!buf)
     return;

   if (buf->pixmap)
     ecore_x_pixmap_free(buf->pixmap);

   if (buf->tbm_surface)
     tbm_surface_destroy(buf->tbm_surface);

   free(buf);
}

/**
 * @brief Exports a buffer for sharing with other processes or APIs.
 *
 * This function exports the underlying TBM buffer object (bo) as a
 * file descriptor (DMA-BUF FD). It only supports single-plane formats.
 *
 * @param bmdata Module data (unused).
 * @param bdata The buffer data (Ecore_Buffer_X11_Dri3_Data) to export.
 * @param[out] id Pointer to store the exported file descriptor.
 * @return EXPORT_TYPE_FD on success, EXPORT_TYPE_INVALID on failure or
 *         if the format is not suitable for direct FD export (e.g., multi-planar).
 */
static Ecore_Export_Type
_ecore_buffer_x11_dri3_buffer_export(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata, int *id)
{
   Ecore_Buffer_X11_Dri3_Data *buf = bdata;
   tbm_bo bo;

   if (_buf_get_num_planes(buf->format) != 1)
     return EXPORT_TYPE_INVALID;

   bo = tbm_surface_internal_get_bo(buf->tbm_surface, 0);
   if (!bo)
     return EXPORT_TYPE_INVALID;

   if (id) *id = tbm_bo_export_fd(bo);

   return EXPORT_TYPE_FD;
}

/**
 * @brief Imports a buffer from an external source.
 *
 * This function imports a buffer from a file descriptor (DMA-BUF FD)
 * using the Tizen Buffer Manager. It creates a TBM surface from the
 * imported buffer object.
 *
 * @param bmdata The module data (Ecore_Buffer_Module_X11_Dri3_Data).
 * @param w The width of the buffer to import.
 * @param h The height of the buffer to import.
 * @param format The Ecore_Buffer_Format of the buffer.
 * @param type The type of export (must be EXPORT_TYPE_FD).
 * @param export_id The file descriptor to import.
 * @param flags Flags associated with the buffer (passed to TBM).
 * @return A pointer to Ecore_Buffer_Data (Ecore_Buffer_X11_Dri3_Data) on success,
 *         NULL on failure.
 */
static Ecore_Buffer_Data
_ecore_buffer_x11_dri3_buffer_import(Ecore_Buffer_Module_Data bmdata, int w, int h, Ecore_Buffer_Format format, Ecore_Export_Type type, int export_id, unsigned int flags)
{
   Ecore_Buffer_Module_X11_Dri3_Data *bm = bmdata;
   Ecore_Buffer_X11_Dri3_Data *buf;
   tbm_bo bo;
   tbm_surface_info_s info;
   int i, num_plane;

   if (!bm)
     return NULL;

   if (type != EXPORT_TYPE_FD)
     return NULL;

   if (export_id < 1)
     return NULL;

   buf = calloc(1, sizeof(Ecore_Buffer_X11_Dri3_Data));
   if (!buf)
     return NULL;

   buf->w = w;
   buf->h = h;
   buf->format = format;
   buf->flags = flags;
   buf->is_imported = EINA_TRUE;

   //Import tbm_surface
   bo = tbm_bo_import_fd(bm->tbm_mgr, export_id);
   if (!bo)
     {
        free(buf);
        return NULL;
     }

   num_plane = _buf_get_num_planes(format);
   info.width = w;
   info.height = h;
   info.format = format;
   info.bpp = _buf_get_bpp(format);
   info.size = w * h * info.bpp;
   for ( i = 0 ; i < num_plane ; i++)
   {
      info.planes[i].size = w * h * info.bpp;
      info.planes[i].stride = w * info.bpp;
      info.planes[i].offset = 0;
   }

   buf->tbm_surface = tbm_surface_internal_create_with_bos(&info, &bo, 1);
   if (!buf->tbm_surface)
     {
        tbm_bo_unref(bo);
        free(buf);
        return NULL;
     }

   tbm_bo_unref(bo);

   return buf;
}

/**
 * @brief Gets the X Pixmap associated with an Ecore_Buffer.
 *
 * If the Pixmap has not been created yet, this function will create it
 * using DRI3 from the TBM surface's file descriptor. It only supports
 * single-plane formats for Pixmap creation. The created Pixmap is cached
 * in the Ecore_Buffer_X11_Dri3_Data structure.
 *
 * @param bmdata Module data (unused).
 * @param bdata The buffer data (Ecore_Buffer_X11_Dri3_Data).
 * @return The Ecore_X_Pixmap (which is an XID) on success, or 0 on failure.
 */
static Ecore_Pixmap
_ecore_buffer_x11_dri3_pixmap_get(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata)
{
   Ecore_Buffer_X11_Dri3_Data *buf = bdata;
   Ecore_X_Display *xdpy;
   Ecore_X_Window root;
   tbm_surface_info_s info;
   tbm_bo bo;
   int ret;

   if (!buf)
     return 0;

   if (buf->pixmap)
     return buf->pixmap;

   ret = tbm_surface_get_info(buf->tbm_surface, &info);
   if (ret != 0)
     return 0;

   if (info.num_planes != 1)
     return 0;

   bo = tbm_surface_internal_get_bo(buf->tbm_surface, 0);
   if (!bo)
     return 0;

   xdpy = ecore_x_display_get();
   root = ecore_x_window_root_first_get();
   buf->pixmap = _dri3_pixmap_from_fd(xdpy, root,
                                      buf->w, buf->h,
                                      _buf_get_depth(buf->format),
                                      tbm_bo_export_fd(bo),
                                      _buf_get_bpp(buf->format),
                                      info.planes[0].stride,
                                      info.planes[0].size);

   return buf->pixmap;
}

/**
 * @brief Gets the underlying TBM surface associated with an Ecore_Buffer.
 *
 * @param bmdata Module data (unused).
 * @param bdata The buffer data (Ecore_Buffer_X11_Dri3_Data).
 * @return A pointer to the tbm_surface (void *) on success, or NULL if the
 *         buffer data is invalid.
 */
static void *
_ecore_buffer_x11_dri3_tbm_bo_get(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata)
{
   Ecore_Buffer_X11_Dri3_Data *buf = bdata;

   if (!buf)
     return NULL;

   return buf->tbm_surface;
}

/**
 * @brief Structure defining the X11 DRI3 Ecore_Buffer backend implementation.
 *
 * This structure provides function pointers for all operations supported by
 * the Ecore_Buffer interface, tailored for the X11 DRI3 mechanism using TBM.
 */
static Ecore_Buffer_Backend _ecore_buffer_x11_dri3_backend = {
     "x11_dri3", /**< Name of the backend. */
     &_ecore_buffer_x11_dri3_init, /**< Function to initialize the backend. */
     &_ecore_buffer_x11_dri3_shutdown, /**< Function to shutdown the backend. */
     &_ecore_buffer_x11_dri3_buffer_alloc, /**< Function to allocate a buffer. */
     &_ecore_buffer_x11_dri3_buffer_free, /**< Function to free a buffer. */
     &_ecore_buffer_x11_dri3_buffer_export, /**< Function to export a buffer. */
     &_ecore_buffer_x11_dri3_buffer_import, /**< Function to import a buffer. */
     NULL, /**< Function to map a buffer (not implemented for DRI3). */
     &_ecore_buffer_x11_dri3_pixmap_get, /**< Function to get an X Pixmap from a buffer. */
     &_ecore_buffer_x11_dri3_tbm_bo_get, /**< Function to get the TBM surface from a buffer. */
};

/**
 * @brief Registers the X11 DRI3 Ecore_Buffer backend.
 *
 * This function is typically called at module load time (EINA_MODULE_INIT).
 *
 * @return EINA_TRUE on successful registration, EINA_FALSE otherwise.
 */
Eina_Bool x11_dri3_init(void)
{
   return ecore_buffer_register(&_ecore_buffer_x11_dri3_backend);
}

/**
 * @brief Unregisters the X11 DRI3 Ecore_Buffer backend.
 *
 * This function is typically called at module unload time (EINA_MODULE_SHUTDOWN).
 */
void x11_dri3_shutdown(void)
{
   ecore_buffer_unregister(&_ecore_buffer_x11_dri3_backend);
}

EINA_MODULE_INIT(x11_dri3_init);
EINA_MODULE_SHUTDOWN(x11_dri3_shutdown);
