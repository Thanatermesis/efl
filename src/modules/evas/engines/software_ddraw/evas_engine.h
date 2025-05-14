#ifndef __EVAS_ENGINE_H__
#define __EVAS_ENGINE_H__


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <ddraw.h>

#include "../software_generic/Evas_Engine_Software_Generic.h"

typedef struct _Outbuf_Region         Outbuf_Region;
typedef struct _DD_Output_Buffer      DD_Output_Buffer;

/**
 * @brief Structure representing the output buffer for DirectDraw rendering.
 *
 * This structure holds all the necessary information for managing the
 * DirectDraw surfaces and related rendering properties.
 */
struct _Outbuf
{
   int                       width;  /**< The width of the output buffer in pixels. */
   int                       height; /**< The height of the output buffer in pixels. */
   int                       rot;    /**< The rotation angle (0, 90, 180, 270). */
   int                       onebuf; /**< Flag indicating if a single buffer is used for updates (deprecated?). */

   /** @brief Private data specific to the DirectDraw output buffer implementation. */
   struct {
      Convert_Pal           *pal; /**< Color palette information, if applicable. */
      /** @brief DirectDraw specific handles and surfaces. */
      struct {
         HWND                window;          /**< Handle to the target window. */
         LPDIRECTDRAW        object;          /**< Pointer to the DirectDraw object. */
         LPDIRECTDRAWSURFACE surface_primary; /**< Pointer to the primary DirectDraw surface. */
         LPDIRECTDRAWSURFACE surface_back;    /**< Pointer to the back buffer DirectDraw surface. */
         LPDIRECTDRAWCLIPPER clipper;         /**< Pointer to the DirectDraw clipper object. */
         unsigned char       fullscreen : 1;  /**< Flag indicating if running in fullscreen mode. */
         unsigned char       swap       : 1;  /**< Internal flag related to buffer swapping. */
         unsigned char       bit_swap   : 1;  /**< Internal flag related to bit swapping (endianness?). */
      } dd;
      /** @brief Color mask information for the pixel format. */
      struct {
         DATA32              r, g, b; /**< Color masks for the display format (e.g., 0xff0000 for red in RGB32). */
      } mask;

      /** @brief Single buffer used for accumulating updates if onebuf is enabled. */
      RGBA_Image            *onebuf;
      /** @brief List of regions updated in the single buffer. Elements are likely Tilebuf_Rect or similar. */
      Eina_List             *onebuf_regions;

      /** @brief List of Outbuf_Region structures waiting to be written to the DirectDraw surface. */
      Eina_List             *pending_writes;
      /** @brief List of Outbuf_Region structures from the previous frame, potentially used for synchronization or double buffering logic. */
      Eina_List             *prev_pending_writes;

      unsigned char          mask_dither       : 1; /**< Flag indicating if mask dithering is enabled. */
      unsigned char          destination_alpha : 1; /**< Flag indicating if the destination surface has an alpha channel. */
      unsigned char          debug             : 1; /**< Flag indicating if debug mode is enabled. */
      unsigned char          synced            : 1; /**< Flag indicating if rendering is synchronized (e.g., vsync). */
   } priv;
};

/**
 * @brief Represents a rectangular region within an output buffer, typically queued for writing.
 *
 * Used to track areas that need updating or have been updated, linking to the actual pixel data.
 */
struct _Outbuf_Region
{
   DD_Output_Buffer *ddob; /**< Pointer to the DirectDraw output buffer containing the pixel data for this region. */
   int               x;      /**< The x-coordinate of the top-left corner of the region relative to the Outbuf. */
   int               y;      /**< The y-coordinate of the top-left corner of the region relative to the Outbuf. */
   int               width;  /**< The width of the region in pixels. */
   int               height; /**< The height of the region in pixels. */
};

/**
 * @brief Wrapper structure for raw pixel data used in DirectDraw operations.
 *
 * This often represents a temporary system memory buffer holding pixel data
 * before it's transferred to a DirectDraw surface, or data read back.
 */
struct _DD_Output_Buffer
{
   void *data;   /**< Pointer to the raw pixel data (e.g., allocated with malloc). */
   int   width;  /**< The width of the buffer in pixels. */
   int   height; /**< The height of the buffer in pixels. */
   int   pitch;  /**< The number of bytes per scanline (stride). May be different from width * bytes_per_pixel. */
   int   psize;  /**< The total size of the pixel data in bytes (usually pitch * height). */
};

extern int _evas_log_dom_module; /**< Log domain specific to this Evas engine module. */

#ifdef EVAS_DEFAULT_LOG_COLOR
# undef EVAS_DEFAULT_LOG_COLOR
#endif
#define EVAS_DEFAULT_LOG_COLOR EINA_COLOR_CYAN
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_log_dom_module, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_log_dom_module, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_log_dom_module, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_log_dom_module, __VA_ARGS__)
#ifdef CRT
# undef CRT
#endif
#define CRT(...) EINA_LOG_DOM_CRIT(_evas_log_dom_module, __VA_ARGS__)

/* evas_outbuf.c */

/**
 * @brief Initializes the DirectDraw output buffer subsystem.
 *
 * Performs any necessary global setup for the DirectDraw output buffer management.
 * Should be called once before creating any Outbuf instances.
 */
void evas_software_ddraw_outbuf_init(void);

/**
 * @brief Frees the resources associated with an Outbuf structure.
 * @param buf The Outbuf structure to free. Releases DirectDraw objects and memory.
 */
void evas_software_ddraw_outbuf_free(Outbuf *buf);

/**
 * @brief Sets up and creates a new DirectDraw output buffer (Outbuf).
 *
 * Initializes DirectDraw, creates primary and back surfaces, and sets up
 * the Outbuf structure for rendering.
 * @param width The desired width of the rendering surface in pixels.
 * @param height The desired height of the rendering surface in pixels.
 * @param rotation The desired rotation angle (0, 90, 180, 270).
 * @param window The handle (HWND) of the window to render into.
 * @param fullscreen Non-zero (e.g., 1) if fullscreen mode is requested, 0 for windowed mode.
 * @return A pointer to the newly created Outbuf structure, or NULL on failure.
 */
Outbuf *evas_software_ddraw_outbuf_setup(int          width,
                                         int          height,
                                         int          rotation,
                                         HWND         window,
                                         int          fullscreen);

/**
 * @brief Reconfigures an existing output buffer, typically on resize or rotation change.
 * @param buf The Outbuf structure to reconfigure.
 * @param width The new width in pixels.
 * @param height The new height in pixels.
 * @param rotation The new rotation angle (0, 90, 180, 270).
 * @param depth The new color depth (Note: DirectDraw might use the existing surface depth).
 */
void evas_software_ddraw_outbuf_reconfigure(Outbuf      *buf,
                                            int          width,
                                            int          height,
                                            int          rotation,
                                            Outbuf_Depth depth);

/**
 * @brief Gets a buffer region intended for direct pixel data updates.
 *
 * This function likely prepares a region (possibly in system memory or by
 * locking a surface) where the engine can write pixel data. Clipping
 * might occur based on the buffer's dimensions.
 *
 * @param buf The target output buffer.
 * @param x The desired x-coordinate of the update region's top-left corner.
 * @param y The desired y-coordinate of the update region's top-left corner.
 * @param w The desired width of the update region.
 * @param h The desired height of the update region.
 * @param[out] cx Pointer to store the potentially clipped x-coordinate of the returned region.
 * @param[out] cy Pointer to store the potentially clipped y-coordinate of the returned region.
 * @param[out] cw Pointer to store the potentially clipped width of the returned region.
 * @param[out] ch Pointer to store the potentially clipped height of the returned region.
 * @return A pointer representing the update region. This could be an RGBA_Image,
 *         a DD_Output_Buffer, or another internal structure depending on the
 *         buffering strategy (e.g., onebuf). Returns NULL on failure.
 */
void *evas_software_ddraw_outbuf_new_region_for_update(Outbuf *buf,
                                                       int     x,
                                                       int     y,
                                                       int     w,
                                                       int     h,
                                                       int    *cx,
                                                       int    *cy,
                                                       int    *cw,
                                                       int    *ch);

/**
 * @brief Pushes an updated image region to be queued for drawing onto the output buffer.
 *
 * Takes the pixel data from the `update` image and schedules it to be drawn
 * onto the DirectDraw back buffer at the specified coordinates during the next flush operation.
 *
 * @param buf The target output buffer.
 * @param update An RGBA_Image structure containing the pixel data for the updated region.
 *               This data will be converted and copied.
 * @param x The x-coordinate within the output buffer where the update should be placed.
 * @param y The y-coordinate within the output buffer where the update should be placed.
 * @param w The width of the updated region.
 * @param h The height of the updated region.
 */
void evas_software_ddraw_outbuf_push_updated_region(Outbuf     *buf,
                                                    RGBA_Image *update,
                                                    int        x,
                                                    int        y,
                                                    int        w,
                                                    int        h);

/**
 * @brief Flushes all queued updated regions to the display.
 *
 * This function takes all regions pushed since the last flush, performs
 * necessary color conversions and copies (e.g., Blt) them to the DirectDraw
 * back buffer, and then potentially flips the primary and back surfaces
 * to make the changes visible.
 *
 * @param buf The output buffer to flush.
 * @param[out] surface_damage Pointer to store regions damaged on the final surface (optional).
 * @param[out] buffer_damage Pointer to store regions damaged in the intermediate buffer (optional).
 * @param render_mode The rendering mode influencing flush behavior (e.g., sync, async).
 */
void evas_software_ddraw_outbuf_flush(Outbuf *buf, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Flushes updates during idle time, potentially optimized for specific scenarios.
 *
 * This might handle flushing the 'onebuf' if it's being used, or perform
 * other background flushing tasks.
 * @param buf The output buffer.
 */
void evas_software_ddraw_outbuf_idle_flush(Outbuf *buf);

/**
 * @brief Gets the current width of the output buffer.
 * @param buf The output buffer.
 * @return The width in pixels.
 */
int evas_software_ddraw_outbuf_width_get(Outbuf *buf);

/**
 * @brief Gets the current height of the output buffer.
 * @param buf The output buffer.
 * @return The height in pixels.
 */
int evas_software_ddraw_outbuf_height_get(Outbuf *buf);

/**
 * @brief Gets the current rotation angle of the output buffer.
 * @param buf The output buffer.
 * @return The rotation angle (0, 90, 180, or 270).
 */
int evas_software_ddraw_outbuf_rot_get(Outbuf *buf);

/* evas_ddraw_buffer.c */

/**
 * @brief Creates a new DD_Output_Buffer structure, potentially allocating memory.
 * @param width The width of the buffer in pixels.
 * @param height The height of the buffer in pixels.
 * @param data If not NULL, this pre-allocated memory is used for the buffer's data.
 *             If NULL, memory is allocated internally.
 * @return A pointer to the newly created DD_Output_Buffer, or NULL on failure.
 */
DD_Output_Buffer *evas_software_ddraw_output_buffer_new(int   width,
                                                        int   height,
                                                        void *data);

/**
 * @brief Frees a DD_Output_Buffer structure and its associated data if internally allocated.
 * @param ddob The DD_Output_Buffer to free.
 */
void evas_software_ddraw_output_buffer_free(DD_Output_Buffer *ddob);

/**
 * @brief Copies pixel data from a DD_Output_Buffer (source) to a raw memory region (destination),
 *        typically representing a locked DirectDraw surface.
 * @param ddob The source DD_Output_Buffer containing the pixel data to copy.
 * @param ddraw_data Pointer to the destination memory (e.g., locked surface bits).
 * @param ddraw_width Width of the destination surface/region (used for clipping).
 * @param ddraw_height Height of the destination surface/region (used for clipping).
 * @param ddraw_pitch Pitch (stride) of the destination surface in bytes.
 * @param x The target x-coordinate within the destination memory where the top-left of `ddob` data should be placed.
 * @param y The target y-coordinate within the destination memory where the top-left of `ddob` data should be placed.
 */
void evas_software_ddraw_output_buffer_paste(DD_Output_Buffer *ddob,
                                             void             *ddraw_data,
                                             int               ddraw_width,
                                             int               ddraw_height,
                                             int               ddraw_pitch,
                                             int               x,
                                             int               y);

/**
 * @brief Gets the raw pixel data pointer and pitch from a DD_Output_Buffer.
 * @param ddob The DD_Output_Buffer.
 * @param[out] bytes_per_line_ret Pointer to store the pitch (bytes per scanline) of the buffer data.
 * @return Pointer to the raw pixel data (as DATA8*).
 */
DATA8 *evas_software_ddraw_output_buffer_data(DD_Output_Buffer *ddob,
                                              int              *bytes_per_line_ret);

/* evas_ddraw_main.cpp */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the core DirectDraw components (object, surfaces, clipper) for a given window.
 * @param window The target window handle (HWND).
 * @param fullscreen Non-zero (e.g., 1) to request fullscreen mode, 0 for windowed mode.
 * @param buf The Outbuf structure to populate with initialized DirectDraw handles and surface pointers.
 * @return 1 on success, 0 on failure.
 */
int evas_software_ddraw_init (HWND    window,
                              int     fullscreen,
                              Outbuf *buf);

/**
 * @brief Shuts down and releases DirectDraw resources (surfaces, clipper, object) associated with an Outbuf.
 * @param buf The Outbuf whose DirectDraw resources should be released.
 */
void evas_software_ddraw_shutdown(Outbuf *buf);

/**
 * @brief Retrieves the color masks (R, G, B) for the primary display surface format.
 * @param buf The Outbuf associated with the display. The masks are stored in buf->priv.mask.
 * @return 1 on success (masks retrieved), 0 on failure (e.g., DirectDraw not initialized or couldn't get format).
 */
int evas_software_ddraw_masks_get(Outbuf *buf);

/**
 * @brief Locks the DirectDraw back buffer surface to allow direct memory access for drawing.
 * @param buf The Outbuf containing the back buffer surface to lock.
 * @param[out] ddraw_width Pointer to store the width of the locked surface in pixels.
 * @param[out] ddraw_height Pointer to store the height of the locked surface in pixels.
 * @param[out] ddraw_pitch Pointer to store the pitch (stride) of the locked surface in bytes.
 * @return Pointer to the beginning of the locked surface memory, or NULL if locking failed.
 */
void *evas_software_ddraw_lock(Outbuf *buf,
                               int    *ddraw_width,
                               int    *ddraw_height,
                               int    *ddraw_pitch);

/**
 * @brief Unlocks the previously locked DirectDraw back buffer and performs a surface flip
 *        to present the contents of the back buffer onto the primary surface (making it visible).
 * @param buf The Outbuf containing the surface that was locked and needs to be flipped.
 */
void evas_software_ddraw_unlock_and_flip(Outbuf *buf);

/**
 * @brief Handles resizing of the DirectDraw surfaces (primary and back buffer).
 *
 * This is typically called when the target window is resized. It may involve
 * releasing the existing surfaces and recreating them with the new dimensions
 * stored within the `buf` structure.
 * @param buf The Outbuf whose associated surfaces need resizing according to its current width/height.
 */
void evas_software_ddraw_surface_resize(Outbuf *buf);

#ifdef __cplusplus
}
#endif


#endif /* __EVAS_ENGINE_H__ */
