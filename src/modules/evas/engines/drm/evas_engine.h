/**
 * @file
 * @brief Evas DRM engine internal header.
 *
 * This header defines the internal structures and function prototypes
 * used by the Evas DRM engine.
 */

#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include "evas_common_private.h"
# include "evas_macros.h"
# include "evas_private.h"
# include "Evas.h"
# include "Evas_Engine_Drm.h"
# include <Ecore.h>
# include <Ecore_Drm2.h>
# include <drm_fourcc.h>
# include <xf86drm.h>
# include <xf86drmMode.h>

# include "../software_generic/Evas_Engine_Software_Generic.h"

extern int _evas_engine_drm_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_drm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_drm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_drm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_drm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_drm_log_dom, __VA_ARGS__)

/**
 * @brief Structure representing a framebuffer within an output buffer.
 *
 * This structure holds information about a single framebuffer, including its
 * age (for buffer management), a pointer to the Ecore_Drm2_Fb, and status flags.
 */
typedef struct _Outbuf_Fb
{
   int age; /**< Age of the framebuffer, used for buffer eviction strategies. */
   Ecore_Drm2_Fb *fb; /**< Pointer to the Ecore DRM framebuffer object. */

   Eina_Bool valid : 1; /**< Flag indicating if the framebuffer is currently valid. */
   Eina_Bool drawn : 1; /**< Flag indicating if the framebuffer has been drawn to. */
} Outbuf_Fb;

/**
 * @brief Structure representing an output buffer for the DRM engine.
 *
 * This structure manages the output for a specific DRM device, including
 * its dimensions, format, and associated framebuffers.
 */
struct _Outbuf
{
   Ecore_Drm2_Device *dev; /**< Pointer to the Ecore DRM device. */
   int w; /**< Width of the output buffer. */
   int h; /**< Height of the output buffer. */
   int bpp; /**< Bits per pixel of the output buffer. */
   int rotation; /**< Rotation angle of the output buffer (0, 90, 180, 270). */
   unsigned int depth; /**< Color depth of the output buffer. */
   unsigned int format; /**< Pixel format of the output buffer (e.g., DRM_FORMAT_XRGB8888). */

   Evas_Engine_Info_Drm *info; /**< Pointer to the Evas DRM engine information. */

   /** @brief Private data for the output buffer. */
   struct
     {
        Eina_List *fb_list; /**< List of Outbuf_Fb structures managing framebuffers. */
        Outbuf_Fb *draw; /**< The current framebuffer being drawn to. */
        Ecore_Drm2_Output *output; /**< The Ecore DRM output associated with this buffer. */
        Ecore_Drm2_Plane *plane; /**< The Ecore DRM plane used for this buffer (if any). */
        Eina_List *pending; /**< List of pending operations or events. */
        Eina_Rectangle *rects; /**< Array of rectangles representing damaged regions. */
        unsigned int rect_count; /**< Number of rectangles in the 'rects' array. */
        int unused_duration; /**< Duration for which the buffer has been unused. */
     } priv;

   Eina_Bool alpha : 1; /**< Flag indicating if the output buffer supports alpha. */
   Eina_Bool vsync : 1; /**< Flag indicating if vsync is enabled for this output. */
};

/**
 * @brief Sets up an output buffer.
 * @param info Pointer to the Evas DRM engine information.
 * @param w Width of the output buffer.
 * @param h Height of the output buffer.
 * @return A pointer to the newly created Outbuf, or NULL on failure.
 */
Outbuf *_outbuf_setup(Evas_Engine_Info_Drm *info, int w, int h);

/**
 * @brief Frees an output buffer.
 * @param ob Pointer to the Outbuf to free.
 */
void _outbuf_free(Outbuf *ob);

/**
 * @brief Gets the rotation of an output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The rotation angle (0, 90, 180, 270).
 */
int _outbuf_rotation_get(Outbuf *ob);

/**
 * @brief Reconfigures an output buffer.
 * @param ob Pointer to the Outbuf.
 * @param w New width.
 * @param h New height.
 * @param rotation New rotation.
 * @param depth New depth (as Outbuf_Depth, though type seems to be an Evas internal).
 */
void _outbuf_reconfigure(Outbuf *ob, int w, int h, int rotation, Outbuf_Depth depth);

/**
 * @brief Gets the swap mode of the output buffer.
 * @param ob Pointer to the Outbuf.
 * @return The current render output swap mode.
 */
Render_Output_Swap_Mode _outbuf_state_get(Outbuf *ob);

/**
 * @brief Creates a new update region for the output buffer.
 * This function prepares a region of the output buffer for updating.
 * @param ob Pointer to the Outbuf.
 * @param x X-coordinate of the region.
 * @param y Y-coordinate of the region.
 * @param w Width of the region.
 * @param h Height of the region.
 * @param[out] cx Pointer to store the clipped X-coordinate.
 * @param[out] cy Pointer to store the clipped Y-coordinate.
 * @param[out] cw Pointer to store the clipped width.
 * @param[out] ch Pointer to store the clipped height.
 * @return A pointer to the data of the update region, or NULL on failure.
 */
void *_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an updated region to the output buffer.
 * @param ob Pointer to the Outbuf.
 * @param update Pointer to the RGBA_Image containing the update.
 * @param x X-coordinate of the update.
 * @param y Y-coordinate of the update.
 * @param w Width of the update.
 * @param h Height of the update.
 */
void _outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Flushes the output buffer.
 * This function commits the changes to the display.
 * @param ob Pointer to the Outbuf.
 * @param surface_damage Pointer to Tilebuf_Rect representing surface damage.
 * @param buffer_damage Pointer to Tilebuf_Rect representing buffer damage.
 * @param render_mode The Evas render mode.
 */
void _outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Sets the damage region for the output buffer.
 * @param ob Pointer to the Outbuf.
 * @param damage Pointer to Tilebuf_Rect representing the damage region.
 */
void _outbuf_damage_region_set(Outbuf *ob, Tilebuf_Rect *damage);

/**
 * @brief Flushes the output buffer when idle.
 * @param ob Pointer to the Outbuf.
 */
void _outbuf_idle_flush(Outbuf *ob);

#endif
