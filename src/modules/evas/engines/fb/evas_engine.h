#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H

#include "evas_fb.h"

#include "../software_generic/Evas_Engine_Software_Generic.h"

extern int _evas_engine_fb_log_dom;
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_fb_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_fb_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_fb_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_fb_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_fb_log_dom, __VA_ARGS__)

/**
 * @brief Structure representing an output buffer for the framebuffer engine.
 *
 * This structure holds information about the output buffer, including its
 * dimensions, depth, rotation, and private data related to the framebuffer
 * device and back buffer.
 */
struct _Outbuf
{
   Outbuf_Depth    depth; /**< Depth of the output buffer. */
   int             w, h;  /**< Width and height of the output buffer. */
   int             rot;   /**< Rotation of the output buffer (0, 90, 180, 270). */

   struct {
      struct {
         FB_Mode  *fb; /**< Pointer to the framebuffer mode information. */
      } fb;
      struct {
         DATA32    r, g, b; /**< Mask for red, green, and blue color components. */
      } mask;
      RGBA_Image  *back_buf; /**< Pointer to the back buffer image. */
   } priv; /**< Private data for the output buffer. */
};

/****/

/**
 * @brief Initializes the framebuffer output buffer system.
 * This function should be called once before any other evas_fb_outbuf_fb_* functions.
 */
void         evas_fb_outbuf_fb_init                   (void);
/**
 * @brief Frees an output buffer.
 * @param buf The output buffer to free.
 */
void         evas_fb_outbuf_fb_free                   (Outbuf *buf);

/**
 * @brief Sets up a new framebuffer output buffer.
 * @param w The width of the output buffer.
 * @param h The height of the output buffer.
 * @param rot The rotation of the output buffer.
 * @param depth The depth of the output buffer.
 * @param vt_no The virtual terminal number.
 * @param dev_no The device number.
 * @param refresh The refresh rate.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf      *evas_fb_outbuf_fb_setup_fb               (int w, int h, int rot, Outbuf_Depth depth, int vt_no, int dev_no, int refresh);

/**
 * @brief Blits a region from a source to the output buffer.
 * @param buf The output buffer.
 * @param src_x The source X coordinate.
 * @param src_y The source Y coordinate.
 * @param w The width of the region to blit.
 * @param h The height of the region to blit.
 * @param dst_x The destination X coordinate.
 * @param dst_y The destination Y coordinate.
 */
void         evas_fb_outbuf_fb_blit                   (Outbuf *buf, int src_x, int src_y, int w, int h, int dst_x, int dst_y);
/**
 * @brief Marks a region of the output buffer as updated.
 * This typically triggers a flush of the specified region to the display.
 * @param buf The output buffer.
 * @param x The X coordinate of the updated region.
 * @param y The Y coordinate of the updated region.
 * @param w The width of the updated region.
 * @param h The height of the updated region.
 */
void         evas_fb_outbuf_fb_update                 (Outbuf *buf, int x, int y, int w, int h);
/**
 * @brief Creates a new region for update.
 * This function is used to get a memory area that can be directly written to for updates.
 * @param buf The output buffer.
 * @param x The X coordinate of the region.
 * @param y The Y coordinate of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 * @param[out] cx Pointer to store the clipped X coordinate.
 * @param[out] cy Pointer to store the clipped Y coordinate.
 * @param[out] cw Pointer to store the clipped width.
 * @param[out] ch Pointer to store the clipped height.
 * @return A pointer to the image data for the update region, or NULL on failure.
 */
void        *evas_fb_outbuf_fb_new_region_for_update  (Outbuf *buf, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);
/**
 * @brief Frees a region previously obtained for update.
 * @param buf The output buffer.
 * @param update The RGBA_Image representing the update region to free.
 */
void         evas_fb_outbuf_fb_free_region_for_update (Outbuf *buf, RGBA_Image *update);
/**
 * @brief Pushes an updated region to the output buffer.
 * @param buf The output buffer.
 * @param update The RGBA_Image containing the updated data.
 * @param x The X coordinate of the region.
 * @param y The Y coordinate of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 */
void         evas_fb_outbuf_fb_push_updated_region    (Outbuf *buf, RGBA_Image *update, int x, int y, int w, int h);
/**
 * @brief Reconfigures an existing output buffer.
 * @param buf The output buffer to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation.
 * @param depth The new depth.
 */
void         evas_fb_outbuf_fb_reconfigure            (Outbuf *buf, int w, int h, int rot, Outbuf_Depth depth);
/**
 * @brief Gets the width of the output buffer.
 * @param buf The output buffer.
 * @return The width of the output buffer in pixels.
 */
int          evas_fb_outbuf_fb_get_width              (Outbuf *buf);
/**
 * @brief Gets the height of the output buffer.
 * @param buf The output buffer.
 * @return The height of the output buffer in pixels.
 */
int          evas_fb_outbuf_fb_get_height             (Outbuf *buf);
/**
 * @brief Gets the depth of the output buffer.
 * @param buf The output buffer.
 * @return The depth of the output buffer.
 */
Outbuf_Depth evas_fb_outbuf_fb_get_depth              (Outbuf *buf);
/**
 * @brief Gets the rotation of the output buffer.
 * @param buf The output buffer.
 * @return The rotation of the output buffer (0, 90, 180, 270).
 */
int          evas_fb_outbuf_fb_get_rot                (Outbuf *buf);
/**
 * @brief Checks if the output buffer has a back buffer.
 * @param buf The output buffer.
 * @return 1 if it has a back buffer, 0 otherwise.
 */
int          evas_fb_outbuf_fb_get_have_backbuf       (Outbuf *buf);
/**
 * @brief Sets whether the output buffer has a back buffer.
 * @param buf The output buffer.
 * @param have_backbuf 1 to enable back buffer, 0 to disable.
 */
void         evas_fb_outbuf_fb_set_have_backbuf       (Outbuf *buf, int have_backbuf);

#endif
