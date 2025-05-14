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

#include "ecore_drm_private.h"

/**
 * @defgroup Ecore_Drm_Sprites_Group Ecore DRM Sprites
 *
 * Functions for managing DRM sprites.
 *
 */

/* TODO: DOXY !! */

/**
 * @brief Creates and initializes sprites for a DRM device.
 *
 * This function queries the DRM device for available hardware planes (sprites)
 * and creates an Ecore_Drm_Sprite structure for each one. These structures
 * are then added to the device's list of sprites.
 *
 * @param dev The Ecore_Drm_Device to create sprites for.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @ingroup Ecore_Drm_Sprites_Group
 */
EAPI Eina_Bool
ecore_drm_sprites_create(Ecore_Drm_Device *dev)
{
   drmModePlaneRes *res;
   drmModePlane *p;
   unsigned int i = 0;

   /* check for valid device */
   EINA_SAFETY_ON_TRUE_RETURN_VAL((!dev) || (dev->drm.fd < 0), EINA_FALSE);

   /* get plane resources */
   if (!(res = drmModeGetPlaneResources(dev->drm.fd))) return EINA_FALSE;

   for (i = 0; i < res->count_planes; i++)
     {
        Ecore_Drm_Sprite *sprite;

        if (!(p = drmModeGetPlane(dev->drm.fd, res->planes[i])))
          continue;

        /* allocate space for sprite */
        if (!(sprite =
              malloc(sizeof(Ecore_Drm_Sprite) +
                     ((sizeof(unsigned int)) * p->count_formats))))
          {
             drmModeFreePlane(p);
             continue;
          }

        sprite->output = NULL;
        sprite->drm_fd = dev->drm.fd;
        sprite->crtcs = p->possible_crtcs;
        sprite->plane_id = p->plane_id;
        sprite->num_formats = p->count_formats;
        memcpy(sprite->formats, p->formats,
               p->count_formats * sizeof(p->formats[0]));
        drmModeFreePlane(p);

        dev->sprites = eina_list_append(dev->sprites, sprite);
     }

   /* free resources */
   drmModeFreePlaneResources(res);

   return EINA_TRUE;
}

/**
 * @brief Destroys all sprites associated with a DRM device.
 *
 * This function iterates through the list of sprites in the given
 * Ecore_Drm_Device, releases any associated framebuffer resources,
 * and frees the memory allocated for each Ecore_Drm_Sprite structure.
 *
 * @param dev The Ecore_Drm_Device whose sprites are to be destroyed.
 *
 * @ingroup Ecore_Drm_Sprites_Group
 */
EAPI void
ecore_drm_sprites_destroy(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Sprite *sprite;

   /* check for valid device */
   EINA_SAFETY_ON_NULL_RETURN(dev);

   EINA_LIST_FREE(dev->sprites, sprite)
     {
        if (sprite->output)
          {
             ecore_drm_sprites_fb_set(sprite, 0, 0);

             _ecore_drm_output_fb_release(sprite->output, sprite->current_fb);
             _ecore_drm_output_fb_release(sprite->output, sprite->next_fb);
          }

        free(sprite);
     }
}

/**
 * @brief Sets or removes a framebuffer for a sprite.
 *
 * This function configures a sprite plane to display a given framebuffer (fb_id)
 * or disables the sprite plane if fb_id is 0. It uses drmModeSetPlane to
 * apply the settings.
 *
 * @param sprite The Ecore_Drm_Sprite to configure.
 * @param fb_id The ID of the framebuffer to display, or 0 to disable the sprite.
 * @param flags DRM plane configuration flags (e.g., for synchronization).
 *              See drmModeSetPlane documentation for details.
 *
 * @ingroup Ecore_Drm_Sprites_Group
 */
EAPI void
ecore_drm_sprites_fb_set(Ecore_Drm_Sprite *sprite, int fb_id, int flags)
{
   EINA_SAFETY_ON_TRUE_RETURN((!sprite) || (!sprite->output));

   if (fb_id)
     {
        drmModeSetPlane(sprite->drm_fd, sprite->plane_id,
                        sprite->output->crtc_id, fb_id, flags,
                        sprite->dest.x, sprite->dest.y, sprite->dest.w,
                        sprite->dest.h, sprite->src.x, sprite->src.y,
                        sprite->src.w, sprite->src.h);
     }
   else
     {
        drmModeSetPlane(sprite->drm_fd, sprite->plane_id,
                        sprite->output->crtc_id, 0, 0,
                        0, 0, 0, 0, 0, 0, 0, 0);
     }
}

/**
 * @brief Checks if a CRTC associated with an output is supported by a sprite's capabilities.
 *
 * This function determines if the CRTC (Cathode Ray Tube Controller) used by
 * the given Ecore_Drm_Output is compatible with a sprite, based on the
 * sprite's `possible_crtcs` bitmask.
 *
 * @param output The Ecore_Drm_Output whose CRTC is to be checked.
 * @param supported A bitmask representing the CRTCs supported by a sprite.
 *                  Each bit corresponds to a CRTC index. If the bit is set,
 *                  the CRTC at that index is supported.
 *                  Example: if `supported` is `(1 << 0) | (1 << 2)`, it means
 *                  CRTC at index 0 and CRTC at index 2 are supported.
 * @return EINA_TRUE if the output's CRTC is supported, EINA_FALSE otherwise.
 *
 * @ingroup Ecore_Drm_Sprites_Group
 */
EAPI Eina_Bool
ecore_drm_sprites_crtc_supported(Ecore_Drm_Output *output, unsigned int supported)
{
   Ecore_Drm_Device *dev;
   unsigned int c = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, EINA_FALSE);
   dev = output->dev;

   for (c = 0; c < dev->crtc_count; c++)
     {
        if (dev->crtcs[c] != output->crtc_id) continue;
        if ((supported) && (1 << c)) return EINA_FALSE;
     }

   return EINA_TRUE;
}
