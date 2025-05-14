/* Portions of this code have been derived from Weston
 *
 * Copyright © 2008-2012 Kristian Høgsberg
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2010 Red Hat <mjg@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _EVAS_ENGINE_H
# define _EVAS_ENGINE_H

//# define LOGFNS 1

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN printf("-EVAS-WL: %25s: %5i - %s\n", __FILE__, __LINE__, __func__)

# else
#  define LOGFN
# endif

extern int _evas_engine_way_shm_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# include <wayland-client.h>
# include "../software_generic/Evas_Engine_Software_Generic.h"
# include "Evas_Engine_Wayland.h"

/**
 * @brief Structure representing an output buffer for rendering.
 *
 * This structure holds information about the target rendering surface,
 * including its dimensions, rotation, depth, associated Wayland display
 * and surface, and internal buffer management details.
 */
struct _Outbuf
{
   int w, h; /**< Width and height of the output buffer in pixels. */
   int rotation; /**< Rotation angle of the output (0, 90, 180, 270). */
   int onebuf; /**< Flag indicating if a single buffer strategy is used. */
   Outbuf_Depth depth; /**< Color depth of the output buffer. */

   Ecore_Wl2_Display *ewd; /**< Pointer to the Ecore Wayland display connection. */
   Evas_Engine_Info_Wayland *info; /**< Pointer to Wayland specific engine information. */

   Ecore_Wl2_Surface *surface; /**< Pointer to the Ecore Wayland surface associated with this buffer. */

   /** @brief Private data for buffer management. */
   struct
     {
        /** @brief Single large buffer used for accumulating updates when onebuf strategy is active. */
        RGBA_Image *onebuf;
        /** @brief Array storing regions updated in the single buffer. Eina_Array<Eina_Rectangle *> */
        Eina_Array onebuf_regions;

        /** @brief List of regions (Tilebuf_Rect *) pending write-out for the current frame. */
        Eina_List *pending_writes;

        /** @brief List of regions (Tilebuf_Rect *) pending write-out from the previous frame. */
        Eina_List *prev_pending_writes;

        /** @brief Array of rectangles representing damaged areas. */
        Eina_Rectangle *rects;
        /** @brief Number of rectangles in the rects array. */
        unsigned int rect_count;

        /* Eina_Bool redraw : 1; */ /**< (Commented out) Flag indicating redraw needed. */
        Eina_Bool destination_alpha : 1; /**< Flag indicating if the destination surface has an alpha channel. */
     } priv; /**< Private buffer management data. */
};

/**
 * @brief Sets up and initializes a new output buffer.
 * @param w The initial width of the buffer.
 * @param h The initial height of the buffer.
 * @param info Wayland specific engine information.
 * @return A pointer to the newly created Outbuf structure, or NULL on failure.
 */
Outbuf *_evas_outbuf_setup(int w, int h, Evas_Engine_Info_Wayland *info);

/**
 * @brief Frees the resources associated with an output buffer.
 * @param ob The output buffer to free.
 */
void _evas_outbuf_free(Outbuf *ob);

/**
 * @brief Flushes the pending updates in the output buffer to the Wayland surface.
 * @param ob The output buffer to flush.
 * @param surface_damage Regions damaged on the surface (unused in this engine).
 * @param buffer_damage Regions damaged in the buffer.
 * @param render_mode The current render mode.
 */
void _evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *surface_damage, Tilebuf_Rect *buffer_damage, Evas_Render_Mode render_mode);

/**
 * @brief Flushes updates during an idle period. Typically used with the one-buffer strategy.
 * @param ob The output buffer to flush.
 */
void _evas_outbuf_idle_flush(Outbuf *ob);

/**
 * @brief Gets the buffer swap mode for the output buffer.
 * @param ob The output buffer.
 * @return The swap mode (e.g., full, copy, double, triple).
 */
Render_Output_Swap_Mode _evas_outbuf_swap_mode_get(Outbuf *ob);

/**
 * @brief Gets the current rotation of the output buffer.
 * @param ob The output buffer.
 * @return The rotation angle (0, 90, 180, 270).
 */
int _evas_outbuf_rotation_get(Outbuf *ob);

/**
 * @brief Reconfigures the output buffer with new dimensions, rotation, depth, etc.
 * @param ob The output buffer to reconfigure.
 * @param w The new width.
 * @param h The new height.
 * @param rot The new rotation.
 * @param depth The new color depth.
 * @param alpha Whether the destination has an alpha channel.
 * @param resize Indicates if this is part of a resize operation.
 */
void _evas_outbuf_reconfigure(Outbuf *ob, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize);

/**
 * @brief Creates a new update region within the output buffer.
 * @param ob The output buffer.
 * @param x The x-coordinate of the region.
 * @param y The y-coordinate of the region.
 * @param w The width of the region.
 * @param h The height of the region.
 * @param[out] cx Clipped x-coordinate (unused).
 * @param[out] cy Clipped y-coordinate (unused).
 * @param[out] cw Clipped width (unused).
 * @param[out] ch Clipped height (unused).
 * @return A pointer to the image data for the new region, or NULL on failure.
 */
void *_evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);

/**
 * @brief Pushes an updated region to the output buffer for later flushing.
 * @param ob The output buffer.
 * @param update The RGBA image data containing the update.
 * @param x The x-coordinate of the update.
 * @param y The y-coordinate of the update.
 * @param w The width of the update.
 * @param h The height of the update.
 */
void _evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);

/**
 * @brief Clears any pending redraw regions in the output buffer.
 * @param ob The output buffer.
 */
void _evas_outbuf_redraws_clear(Outbuf *ob);

#endif
