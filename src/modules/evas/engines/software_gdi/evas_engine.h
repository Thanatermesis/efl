#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "../software_generic/Evas_Engine_Software_Generic.h"

extern int _evas_engine_soft_gdi_log_dom;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_soft_gdi_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_soft_gdi_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_soft_gdi_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_soft_gdi_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_soft_gdi_log_dom, __VA_ARGS__)

typedef struct BITMAPINFO_GDI     BITMAPINFO_GDI;
typedef struct _Outbuf_Region     Outbuf_Region;
typedef struct _Gdi_Output_Buffer Gdi_Output_Buffer;

/**
 * @brief Structure extending BITMAPINFOHEADER with color masks for GDI operations.
 *
 * This structure holds the standard GDI bitmap information header along with
 * the color masks required for certain bitmap formats (like BI_BITFIELDS).
 */
struct BITMAPINFO_GDI
{
   BITMAPINFOHEADER bih;    /**< Standard GDI bitmap information header. */
   DWORD            masks[3]; /**< Color masks for Red, Green, and Blue channels. */
};

/**
 * @brief Represents the main output buffer for the Evas Software GDI engine.
 *
 * This structure holds all necessary information for rendering to a GDI window,
 * including dimensions, rotation, GDI handles, internal buffering details,
 * and configuration flags.
 */
struct _Outbuf
{
   int                   width;  /**< Width of the output buffer in pixels. */
   int                   height; /**< Height of the output buffer in pixels. */
   int                   rot;    /**< Rotation angle (0, 90, 180, 270). */
   int                   onebuf; /**< Flag indicating if single-buffer rendering is used. */

   /**
    * @brief Private data specific to the GDI backend.
    */
   struct {
      Convert_Pal       *pal; /**< Palette information (if applicable). */
      /**
       * @brief GDI specific handles and flags.
       */
      struct {
         BITMAPINFO_GDI *bitmap_info; /**< GDI bitmap information structure. */
         HWND            window;      /**< Handle to the target GDI window. */
         HDC             dc;          /**< Device context handle for the window. */
         HRGN            regions;     /**< GDI region handle for complex window shapes. */
         unsigned char   borderless : 1; /**< Flag indicating if the window is borderless. */
         unsigned char   fullscreen : 1; /**< Flag indicating if the window is fullscreen. */
         unsigned char   region     : 1; /**< Flag indicating if a custom region is used. */
      } gdi;

      /* 1 big buffer for updates - flush on idle_flush */
      RGBA_Image        *onebuf;         /**< Single buffer for rendering updates (if onebuf is set). */
      Eina_List         *onebuf_regions; /**< List of regions updated in the single buffer. */

      /* a list of pending regions to write to the target */
      Eina_List         *pending_writes; /**< List of Evas_Rectangle updates waiting to be flushed. */
      /* a list of previous frame pending regions to write to the target */
      Eina_List         *prev_pending_writes; /**< List of updates from the previous frame, kept for synchronization. */

      unsigned char      mask_dither       : 1; /**< Flag to enable mask dithering. */
      unsigned char      destination_alpha : 1; /**< Flag indicating if the destination surface has an alpha channel. */
      unsigned char      debug             : 1; /**< Flag to enable debugging features. */
      unsigned char      synced            : 1; /**< Flag indicating if rendering is synchronized. */

      unsigned char      region_built      : 1; /**< Flag indicating if the GDI region has been built. */
   } priv;
};

/**
 * @brief Represents a rectangular region within a GDI output buffer.
 *
 * Used internally to manage updates to specific parts of the GDI buffer.
 */
struct _Outbuf_Region
{
   Gdi_Output_Buffer *gdiob; /**< Pointer to the associated GDI output buffer. */
   int                x;      /**< X-coordinate of the region's top-left corner. */
   int                y;      /**< Y-coordinate of the region's top-left corner. */
   int                width;  /**< Width of the region in pixels. */
   int                height; /**< Height of the region in pixels. */
};

/**
 * @brief Represents a GDI Device Independent Bitmap (DIB) used for rendering.
 *
 * This structure encapsulates a GDI bitmap, its device context, dimensions,
 * and a pointer to its pixel data.
 */
struct _Gdi_Output_Buffer
{
   BITMAPINFO_GDI *bitmap_info; /**< Pointer to the bitmap information structure. */
   HBITMAP         bitmap;      /**< Handle to the GDI bitmap object. */
   HDC             dc;          /**< Memory device context compatible with the bitmap. */
   int             width;       /**< Width of the bitmap in pixels. */
   int             height;      /**< Height of the bitmap in pixels. */
   void           *data;        /**< Pointer to the raw pixel data of the bitmap. */
   int             pitch;       /**< Number of bytes per scanline (row) of the bitmap. */
   int             psize;       /**< Total size of the pixel data buffer in bytes. */
};

/* evas_gdi_main.c */

/**
 * @brief Initializes the GDI-specific parts of the output buffer.
 * @param window The target window handle.
 * @param borderless Non-zero if the window is borderless.
 * @param fullscreen Non-zero if the window is fullscreen.
 * @param region Non-zero if the window uses a custom region shape.
 * @param buf The output buffer structure to initialize.
 * @return 1 on success, 0 on failure.
 */
int evas_software_gdi_init (HWND         window,
                            unsigned int borderless,
                            unsigned int fullscreen,
                            unsigned int region,
                            Outbuf      *buf);

/**
 * @brief Shuts down and cleans up GDI resources associated with the output buffer.
 * @param buf The output buffer to shut down.
 */
void evas_software_gdi_shutdown(Outbuf *buf);

/**
 * @brief Resizes the GDI bitmap associated with the output buffer.
 * @param buf The output buffer whose bitmap needs resizing.
 */
void evas_software_gdi_bitmap_resize(Outbuf *buf);

/* evas_gdi_buffer.c */

/**
 * @brief Creates a new GDI output buffer (DIB section).
 * @param dc A compatible device context (e.g., from the target window).
 * @param bitmap_info The bitmap information structure defining the DIB format.
 * @param width The desired width of the buffer.
 * @param height The desired height of the buffer.
 * @param data Optional pre-allocated data pointer (usually NULL).
 * @return A pointer to the newly created Gdi_Output_Buffer, or NULL on failure.
 */
Gdi_Output_Buffer *evas_software_gdi_output_buffer_new(HDC             dc,
                                                       BITMAPINFO_GDI *bitmap_info,
                                                       int             width,
                                                       int             height,
                                                       void           *data);

/**
 * @brief Frees the resources associated with a GDI output buffer.
 * @param gdiob The GDI output buffer to free.
 */
void evas_software_gdi_output_buffer_free(Gdi_Output_Buffer *gdiob);

/**
 * @brief Pastes (blits) the GDI output buffer to the target window's DC.
 * @param gdiob The GDI output buffer to paste.
 * @param x The destination X-coordinate on the target DC.
 * @param y The destination Y-coordinate on the target DC.
 */
void evas_software_gdi_output_buffer_paste(Gdi_Output_Buffer *gdiob,
                                           int                x,
                                           int                y);

/**
 * @brief Gets a pointer to the pixel data of the GDI output buffer.
 * @param gdiob The GDI output buffer.
 * @param[out] pitch Pointer to an integer where the buffer's pitch (bytes per scanline) will be stored.
 * @return A pointer to the raw pixel data (DATA8*), or NULL if not available.
 */
DATA8 *evas_software_gdi_output_buffer_data(Gdi_Output_Buffer *gdiob,
                                            int               *pitch);

/* evas_outbuf.c */

/**
 * @brief Performs one-time initialization for the GDI outbuf module.
 */
void evas_software_gdi_outbuf_init(void);

/**
 * @brief Frees the resources associated with an Outbuf structure.
 * @param buf The Outbuf structure to free.
 */
void evas_software_gdi_outbuf_free(Outbuf *buf);

/**
 * @brief Sets up and initializes a new Outbuf structure for GDI rendering.
 * @param width The desired width of the output buffer.
 * @param height The desired height of the output buffer.
 * @param rotation The desired rotation (0, 90, 180, 270).
 * @param window The target window handle.
 * @param borderless Non-zero if the window should be borderless.
 * @param fullscreen Non-zero if the window should be fullscreen.
 * @param region Non-zero if the window uses a custom region shape.
 * @param mask_dither Non-zero to enable mask dithering.
 * @param destination_alpha Non-zero if the destination surface has alpha.
 * @return A pointer to the newly configured Outbuf, or NULL on failure.
 */
Outbuf *evas_software_gdi_outbuf_setup(int          width,
                                       int          height,
                                       int          rotation,
                                       HWND         window,
                                       unsigned int borderless,
                                       unsigned int fullscreen,
                                       unsigned int region,
                                       int          mask_dither,
                                       int          destination_alpha);

/**
 * @brief Reconfigures an existing Outbuf, typically after a resize or rotation change.
 * @param buf The Outbuf structure to reconfigure.
 * @param width The new width.
 * @param height The new height.
 * @param rotation The new rotation.
 * @param depth The color depth (not directly used by GDI backend, but part of the generic API).
 */
void evas_software_gdi_outbuf_reconfigure(Outbuf      *buf,
                                          int          width,
                                          int          height,
                                          int          rotation,
                                          Outbuf_Depth depth);

/**
 * @brief Creates a new region for update within the output buffer.
 *
 * This function is called by the generic software engine to get a buffer
 * region where it can render updated pixels.
 *
 * @param buf The output buffer.
 * @param x The X-coordinate of the update region.
 * @param y The Y-coordinate of the update region.
 * @param w The width of the update region.
 * @param h The height of the update region.
 * @param[out] cx Pointer to store the clipped X-coordinate.
 * @param[out] cy Pointer to store the clipped Y-coordinate.
 * @param[out] cw Pointer to store the clipped width.
 * @param[out] ch Pointer to store the clipped height.
 * @return A pointer to the RGBA_Image representing the update region, or NULL.
 */
void *evas_software_gdi_outbuf_new_region_for_update(Outbuf *buf,
                                                     int     x,
                                                     int     y,
                                                     int     w,
                                                     int     h,
                                                     int    *cx,
                                                     int    *cy,
                                                     int    *cw,
                                                     int    *ch);

/**
 * @brief Pushes an updated region to the list of pending writes.
 *
 * This function is called by the generic software engine after rendering
 * to an update region obtained via evas_software_gdi_outbuf_new_region_for_update().
 *
 * @param buf The output buffer.
 * @param update The RGBA_Image containing the updated pixel data.
 * @param x The X-coordinate of the updated region.
 * @param y The Y-coordinate of the updated region.
 * @param w The width of the updated region.
 * @param h The height of the updated region.
 */
void evas_software_gdi_outbuf_push_updated_region(Outbuf     *buf,
                                                  RGBA_Image *update,
                                                  int         x,
                                                  int         y,
                                                  int         w,
                                                  int         h);

/**
 * @brief Flushes pending updates to the target window.
 *
 * This function takes the list of updated regions and blits them to the
 * GDI window's device context.
 *
 * @param buf The output buffer.
 * @param surface_damage Regions damaged on the surface (input).
 * @param buffer_damage Regions damaged in the buffer (input).
 * @param render_mode The current render mode (e.g., blocking, async).
 */
void evas_software_gdi_outbuf_flush(Outbuf *buf, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Flushes pending updates during an idle period.
 *
 * Similar to evas_software_gdi_outbuf_flush, but typically called when the
 * application is idle.
 *
 * @param buf The output buffer.
 */
void evas_software_gdi_outbuf_idle_flush(Outbuf *buf);

/**
 * @brief Gets the current width of the output buffer.
 * @param buf The output buffer.
 * @return The width in pixels.
 */
int evas_software_gdi_outbuf_width_get(Outbuf *buf);

/**
 * @brief Gets the current height of the output buffer.
 * @param buf The output buffer.
 * @return The height in pixels.
 */
int evas_software_gdi_outbuf_height_get(Outbuf *buf);

/**
 * @brief Gets the current rotation of the output buffer.
 * @param buf The output buffer.
 * @return The rotation angle (0, 90, 180, 270).
 */
int evas_software_gdi_outbuf_rot_get(Outbuf *buf);


#endif /* EVAS_ENGINE_H */
