#include "ecore_drm2_private.h"

#define FLIP_TIMEOUT 1.0

/**
 * @internal
 * @brief Creates a DRM framebuffer object using drmModeAddFB2.
 *
 * This function attempts to create a framebuffer object using the
 * drmModeAddFB2 ioctl. This is the preferred method as it supports
 * modifiers and multi-planar formats.
 *
 * @param fb Pointer to the Ecore_Drm2_Fb structure to populate.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_fb2_create(Ecore_Drm2_Fb *fb)
{
   uint32_t offsets[4] = { 0 };
   int r;

   r = sym_drmModeAddFB2(fb->fd, fb->w, fb->h, fb->format, fb->handles,
                         fb->strides, offsets, &fb->id, 0);

   if (r)
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Creates a new dumb framebuffer.
 *
 * This function allocates a new framebuffer using the dumb buffer
 * mechanism. It creates a dumb buffer, maps it for CPU access,
 * and then creates a DRM framebuffer object for it.
 *
 * @param dev The Ecore_Drm2_Device to create the framebuffer on.
 * @param width The width of the framebuffer in pixels.
 * @param height The height of the framebuffer in pixels.
 * @param depth The color depth of the framebuffer (e.g., 24).
 * @param bpp The bits per pixel of the framebuffer (e.g., 32).
 * @param format The pixel format of the framebuffer (e.g., DRM_FORMAT_XRGB8888).
 * @return A pointer to the newly created Ecore_Drm2_Fb on success, NULL otherwise.
 */
EAPI Ecore_Drm2_Fb *
ecore_drm2_fb_create(Ecore_Drm2_Device *dev, int width, int height, int depth, int bpp, unsigned int format)
{
   Ecore_Drm2_Fb *fb;
   struct drm_mode_create_dumb carg;
   struct drm_mode_destroy_dumb darg;
   struct drm_mode_map_dumb marg;
   int ret;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);

   fb = calloc(1, sizeof(Ecore_Drm2_Fb));
   if (!fb) return NULL;

   fb->fd = dev->fd;
   fb->w = width;
   fb->h = height;
   fb->bpp = bpp;
   fb->depth = depth;
   fb->format = format;
   fb->ref = 1;

   memset(&carg, 0, sizeof(struct drm_mode_create_dumb));
   carg.bpp = bpp;
   carg.width = width;
   carg.height = height;

   ret = sym_drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &carg);
   if (ret) goto err;

   fb->handles[0] = carg.handle;
   fb->sizes[0] = carg.size;
   fb->strides[0] = carg.pitch;

   if (!_fb2_create(fb))
     {
        ret =
          sym_drmModeAddFB(dev->fd, width, height, depth, bpp,
                           fb->strides[0], fb->handles[0], &fb->id);
        if (ret)
          {
             ERR("Could not add framebuffer: %m");
             goto add_err;
          }
     }

   memset(&marg, 0, sizeof(struct drm_mode_map_dumb));
   marg.handle = fb->handles[0];
   ret = sym_drmIoctl(dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &marg);
   if (ret)
     {
        ERR("Could not map framebuffer: %m");
        goto map_err;
     }

   fb->mmap = mmap(NULL, fb->sizes[0], PROT_WRITE, MAP_SHARED, dev->fd, marg.offset);
   if (fb->mmap == MAP_FAILED)
     {
        ERR("Could not mmap framebuffer memory: %m");
        goto map_err;
     }

   return fb;

map_err:
   sym_drmModeRmFB(dev->fd, fb->id);
add_err:
   memset(&darg, 0, sizeof(struct drm_mode_destroy_dumb));
   darg.handle = fb->handles[0];
   sym_drmIoctl(dev->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
err:
   free(fb);
   return NULL;
}

/**
 * @brief Creates a new framebuffer from a GBM buffer object.
 *
 * This function creates a DRM framebuffer object from an existing
 * GBM (Generic Buffer Management) buffer object. This is typically
 * used for hardware-accelerated rendering.
 *
 * @param dev The Ecore_Drm2_Device to create the framebuffer on.
 * @param width The width of the framebuffer in pixels.
 * @param height The height of the framebuffer in pixels.
 * @param depth The color depth of the framebuffer.
 * @param bpp The bits per pixel of the framebuffer.
 * @param format The pixel format of the framebuffer.
 * @param handle The GBM buffer handle (gem handle).
 * @param stride The stride (pitch) of the framebuffer in bytes.
 * @param bo Pointer to the GBM buffer object (struct gbm_bo *).
 * @return A pointer to the newly created Ecore_Drm2_Fb on success, NULL otherwise.
 */
EAPI Ecore_Drm2_Fb *
ecore_drm2_fb_gbm_create(Ecore_Drm2_Device *dev, int width, int height, int depth, int bpp, unsigned int format, unsigned int handle, unsigned int stride, void *bo)
{
   Ecore_Drm2_Fb *fb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);

   fb = calloc(1, sizeof(Ecore_Drm2_Fb));
   if (!fb) return NULL;

   fb->gbm = EINA_TRUE;
   fb->gbm_bo = bo;

   fb->fd = dev->fd;
   fb->w = width;
   fb->h = height;
   fb->bpp = bpp;
   fb->depth = depth;
   fb->format = format;
   fb->strides[0] = stride;
   fb->sizes[0] = fb->strides[0] * fb->h;
   fb->handles[0] = handle;
   fb->ref = 1;

   if (!_fb2_create(fb))
     {
        if (sym_drmModeAddFB(dev->fd, width, height, depth, bpp,
                             fb->strides[0], fb->handles[0], &fb->id))
          {
             ERR("Could not add framebuffer: %m");
             goto err;
          }
     }
   return fb;

err:
   free(fb);
   return NULL;
}

/**
 * @internal
 * @brief Destroys a framebuffer object.
 *
 * This function performs the actual cleanup of a framebuffer,
 * including unmapping memory, removing the DRM framebuffer object,
 * and freeing associated resources. It's called when the
 * framebuffer's reference count drops to zero.
 *
 * @param fb The Ecore_Drm2_Fb to destroy.
 */
static void
_ecore_drm2_fb_destroy(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);

   if (!fb->dead) WRN("Destroying an fb that hasn't been discarded");

   if (fb->scanout_count)
     WRN("Destroyed fb on scanout %d times.", fb->scanout_count);

   if (fb->mmap) munmap(fb->mmap, fb->sizes[0]);

   if (fb->id) sym_drmModeRmFB(fb->fd, fb->id);

   if (!fb->gbm && !fb->dmabuf)
     {
        struct drm_mode_destroy_dumb darg;

        memset(&darg, 0, sizeof(struct drm_mode_destroy_dumb));
        darg.handle = fb->handles[0];
        sym_drmIoctl(fb->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
     }

   free(fb);
}

/**
 * @internal
 * @brief Increments the reference count of a framebuffer.
 * @param fb The Ecore_Drm2_Fb to reference.
 */
void
_ecore_drm2_fb_ref(Ecore_Drm2_Fb *fb)
{
   fb->ref++;
}

/**
 * @internal
 * @brief Decrements the reference count of a framebuffer.
 *
 * If the reference count drops to zero, and a status handler is set,
 * it calls the handler with ECORE_DRM2_FB_STATUS_DELETED.
 * Then, it calls _ecore_drm2_fb_destroy to free the framebuffer.
 *
 * @param fb The Ecore_Drm2_Fb to dereference.
 */
void
_ecore_drm2_fb_deref(Ecore_Drm2_Fb *fb)
{
   fb->ref--;
   if (fb->ref) return;

   if (fb->status_handler)
     fb->status_handler(fb, ECORE_DRM2_FB_STATUS_DELETED, fb->status_data);

   _ecore_drm2_fb_destroy(fb);
}

/**
 * @brief Marks a framebuffer as discarded and decrements its reference count.
 *
 * This function should be called when a framebuffer is no longer needed
 * by the application. It marks the framebuffer as "dead", meaning it's
 * scheduled for destruction, and then decrements its reference count.
 * The actual destruction happens when the reference count reaches zero.
 *
 * @param fb The Ecore_Drm2_Fb to discard.
 */
EAPI void
ecore_drm2_fb_discard(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);
   EINA_SAFETY_ON_TRUE_RETURN(fb->ref < 1);

   fb->dead = EINA_TRUE;
   _ecore_drm2_fb_deref(fb);
}

/**
 * @brief Gets a pointer to the memory-mapped data of a framebuffer.
 *
 * This function returns a direct pointer to the framebuffer's pixel data,
 * allowing for direct CPU access. This is only valid for framebuffers
 * created with ecore_drm2_fb_create (dumb buffers) and not for GBM
 * or dmabuf-imported framebuffers that are not mappable.
 *
 * @param fb The Ecore_Drm2_Fb.
 * @return A pointer to the framebuffer data, or NULL if not available or on error.
 */
EAPI void *
ecore_drm2_fb_data_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(fb->dead, NULL);
   return fb->mmap;
}

/**
 * @brief Gets the total size in bytes of the framebuffer's data.
 *
 * @param fb The Ecore_Drm2_Fb.
 * @return The size of the framebuffer data in bytes, or 0 on error.
 */
EAPI unsigned int
ecore_drm2_fb_size_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, 0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(fb->dead, 0);
   return fb->sizes[0];
}

/**
 * @brief Gets the stride (pitch) of the framebuffer.
 *
 * The stride is the number of bytes from the start of one row of pixels
 * to the start of the next row.
 *
 * @param fb The Ecore_Drm2_Fb.
 * @return The stride of the framebuffer in bytes, or 0 on error.
 */
EAPI unsigned int
ecore_drm2_fb_stride_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, 0);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(fb->dead, 0);

   return fb->strides[0];
}

/**
 * @brief Marks regions of a framebuffer as dirty.
 *
 * This function informs the DRM subsystem that specified rectangular
 * regions of the framebuffer have been updated and need to be redrawn.
 * This is typically used with the dirty framebuffer (DIRTYFB) feature
 * to optimize updates by only re-scanning changed portions.
 *
 * @param fb The Ecore_Drm2_Fb to mark dirty.
 * @param rects An array of Eina_Rectangle structs defining the dirty regions.
 *              Example: `Eina_Rectangle rects[] = {{0, 0, 100, 100}, {200, 200, 50, 50}};`
 * @param count The number of rectangles in the `rects` array.
 */
EAPI void
ecore_drm2_fb_dirty(Ecore_Drm2_Fb *fb, Eina_Rectangle *rects, unsigned int count)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);
   EINA_SAFETY_ON_NULL_RETURN(rects);
   EINA_SAFETY_ON_TRUE_RETURN(fb->dead);

#ifdef DRM_MODE_FEATURE_DIRTYFB
   drmModeClip *clip;
   unsigned int i = 0;
   int ret;

   clip = alloca(count * sizeof(drmModeClip));
   for (i = 0; i < count; i++)
     {
        clip[i].x1 = rects[i].x;
        clip[i].y1 = rects[i].y;
        clip[i].x2 = rects[i].w;
        clip[i].y2 = rects[i].h;
     }

   ret = sym_drmModeDirtyFB(fb->fd, fb->id, clip, count);
   if ((ret) && (ret == -EINVAL))
     WRN("Could not mark framebuffer as dirty: %m");
#endif
}

/**
 * @internal
 * @brief Releases a framebuffer associated with an output state.
 *
 * This function is called when a framebuffer is no longer actively
 * being displayed or pending display on an output. It decrements the
 * framebuffer's reference count. If a status handler is registered,
 * it's called with ECORE_DRM2_FB_STATUS_RELEASE.
 * It also frees any associated atomic request if atomic modesetting is used.
 *
 * @param output The Ecore_Drm2_Output (currently unused, but kept for API stability).
 * @param s The Ecore_Drm2_Output_State whose framebuffer is to be released.
 */
void
_ecore_drm2_fb_buffer_release(Ecore_Drm2_Output *output EINA_UNUSED, Ecore_Drm2_Output_State *s)
{
   Ecore_Drm2_Fb *fb = s->fb;

   if (fb->status_handler)
     fb->status_handler(fb, ECORE_DRM2_FB_STATUS_RELEASE, fb->status_data);

   _ecore_drm2_fb_deref(fb);
   s->fb = NULL;
   if (_ecore_drm2_use_atomic)
     {
        if (s->atomic_req)
          sym_drmModeAtomicFree(s->atomic_req);
        s->atomic_req = NULL;
     }
}

/**
 * @internal
 * @brief Callback to delete the flip timeout timer from the main loop thread.
 *
 * This function is called asynchronously from a potentially different thread
 * (e.g., DRM event handler thread) to safely delete an Ecore_Timer
 * in the main loop.
 *
 * @param data Pointer to the Ecore_Drm2_Output whose flip_timeout timer needs deletion.
 */
static void
_cb_mainloop_async_timer_del(void *data)
{
   Ecore_Drm2_Output *output = data;

   ecore_timer_del(output->flip_timeout);
   output->flip_timeout = NULL;
}

/**
 * @brief Handles the completion of a page flip operation.
 *
 * This function is called when a page flip event is received from the kernel,
 * indicating that a previously requested flip has completed. It updates
 * the output's state, releasing the old framebuffer and making the pending
 * framebuffer current. It also manages plane scanout states and notifies
 * status handlers.
 *
 * @param output The Ecore_Drm2_Output on which the flip completed.
 * @return EINA_TRUE if there is a next framebuffer queued for display (output->next.fb is set),
 *         EINA_FALSE otherwise. This indicates if another flip should be scheduled immediately.
 */
EAPI Eina_Bool
ecore_drm2_fb_flip_complete(Ecore_Drm2_Output *output)
{
   Eina_Bool plane_scanout;
   Ecore_Drm2_Fb *fb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   if (output->flip_timeout)
     {
        // XXX: output ref++
        ecore_main_loop_thread_safe_call_async
          (_cb_mainloop_async_timer_del, output);
     }
   if (!output->pending.fb) fprintf(stderr, "XXX--XXX eeeeek pending fb is NULL so current would become null ----------------------------------\n");
   if (output->current.fb && (output->current.fb != output->pending.fb))
     _ecore_drm2_fb_buffer_release(output, &output->current);

   output->current.fb = output->pending.fb;
   output->pending.fb = NULL;

   if (_ecore_drm2_use_atomic)
     {
        Eina_List *l, *ll;
        Ecore_Drm2_Plane *plane;

        output->current.atomic_req = output->pending.atomic_req;
        output->pending.atomic_req = NULL;

        EINA_LIST_FOREACH_SAFE(output->planes, l, ll, plane)
          {
             fb = plane->fb;
             plane_scanout = plane->scanout;
             if (!plane->dead)
               {
                  /* First time this plane is scanned out */
                  if (!plane->scanout)
                    fb->scanout_count++;

                  plane->scanout = EINA_TRUE;
                  if (fb->status_handler && (fb->scanout_count == 1) &&
                      (plane_scanout != plane->scanout))
                    fb->status_handler(fb, ECORE_DRM2_FB_STATUS_SCANOUT_ON,
                                       fb->status_data);
                  continue;
               }
             output->planes = eina_list_remove_list(output->planes, l);
             free(plane);
             if (!plane_scanout) continue;

             fb->scanout_count--;
             if (fb->status_handler && (fb->scanout_count == 0))
               fb->status_handler(fb, ECORE_DRM2_FB_STATUS_SCANOUT_OFF,
                                  fb->status_data);
          }
     }

   EINA_LIST_FREE(output->fbs, fb)
     _ecore_drm2_fb_deref(fb);
   output->fbs = NULL;

   return !!output->next.fb;
}

/**
 * @internal
 * @brief Tests an atomic modesetting configuration without applying it.
 *
 * This function constructs an atomic request based on the current desired
 * state of the output (CRTC and planes) and performs a "test-only" commit.
 * This allows checking if the desired configuration is valid before
 * actually attempting to apply it.
 *
 * @param output The Ecore_Drm2_Output for which to test the atomic flip.
 * @return EINA_TRUE if the atomic test commit is successful, EINA_FALSE otherwise.
 *         On success, output->prep.atomic_req will hold the prepared request.
 */
Eina_Bool
_fb_atomic_flip_test(Ecore_Drm2_Output *output)
{
   int ret = 0;
   Eina_List *l;
   Ecore_Drm2_Crtc_State *cstate;
   Ecore_Drm2_Plane_State *pstate;
   Ecore_Drm2_Plane *plane;
   drmModeAtomicReq *req = NULL;
   uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_ATOMIC_ALLOW_MODESET |
     DRM_MODE_ATOMIC_TEST_ONLY;

   if (!_ecore_drm2_use_atomic) return EINA_FALSE;

   req = sym_drmModeAtomicAlloc();
   if (!req) return EINA_FALSE;

   sym_drmModeAtomicSetCursor(req, 0);

   cstate = output->crtc_state;

   ret =
     sym_drmModeAtomicAddProperty(req, cstate->obj_id, cstate->mode.id,
                                  cstate->mode.value);
   if (ret < 0) goto err;

   ret =
     sym_drmModeAtomicAddProperty(req, cstate->obj_id, cstate->active.id,
                                  cstate->active.value);
   if (ret < 0) goto err;

   if (cstate->background.id)
     {
        ret =
          sym_drmModeAtomicAddProperty(req, cstate->obj_id,
                                       cstate->background.id,
                                       cstate->background.value);
        if (ret < 0) goto err;
     }

   EINA_LIST_FOREACH(output->planes, l, plane)
     {
        pstate = plane->state;

        if (!pstate->in_use)
          {
             pstate->cid.value = 0;
             pstate->fid.value = 0;
             pstate->sw.value = 0;
             pstate->sh.value = 0;
             pstate->cx.value = 0;
             pstate->cy.value = 0;
             pstate->cw.value = 0;
             pstate->ch.value = 0;
          }

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->cid.id, pstate->cid.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->fid.id, pstate->fid.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->sx.id, pstate->sx.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->sy.id, pstate->sy.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->sw.id, pstate->sw.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->sh.id, pstate->sh.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->cx.id, pstate->cx.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->cy.id, pstate->cy.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->cw.id, pstate->cw.value);
        if (ret < 0) goto err;

        ret =
          sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                       pstate->ch.id, pstate->ch.value);
        if (ret < 0) goto err;

#if 0
        /* XXX: Disable hardware plane rotation for now as this has broken
         * recently. The break happens because of an invalid argument,
         * ie: the value being sent from pstate->rotation_map ends up being
         * incorrect for some reason. I suspect the breakage to be from
         * kernel drivers (linux 4.20.0) but have not confirmed that version */
        if ((pstate->rotation.id) &&
            (pstate->type.value == DRM_PLANE_TYPE_PRIMARY))
          {
             DBG("Plane %d Atomic Rotation: %lu",
                 pstate->obj_id, pstate->rotation.value);
             ret =
               sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                            pstate->rotation.id,
                                            pstate->rotation_map[pstate->rotation.value]);
             if (ret < 0) goto err;
          }
#endif
     }

   ret =
     sym_drmModeAtomicCommit(output->fd, req, flags, output);
   if (ret < 0) goto err;

   /* clear any previous request */
   if (output->prep.atomic_req)
     sym_drmModeAtomicFree(output->prep.atomic_req);

   output->prep.atomic_req = req;
   return EINA_TRUE;

err:
   DBG("Failed Atomic Test: %m");
   sym_drmModeAtomicFree(req);

   return EINA_FALSE;
}

static int _fb_atomic_flip(Ecore_Drm2_Output *output);
static int _fb_flip(Ecore_Drm2_Output *output);

/**
 * @internal
 * @brief Callback function for flip timeout.
 *
 * This function is called if a page flip event is not received within
 * the FLIP_TIMEOUT duration. It logs an error and attempts to re-issue
 * the flip request.
 *
 * @param data Pointer to the Ecore_Drm2_Output that timed out.
 * @return EINA_FALSE to ensure the timer is removed after firing.
 */
static Eina_Bool
_cb_flip_timeout(void *data)
{
   Ecore_Drm2_Output *output = data;

   output->flip_timeout = NULL;
   ERR("flip event callback timout %0.2fsec - try again", FLIP_TIMEOUT);
   if (_ecore_drm2_use_atomic) _fb_atomic_flip(output);
   else _fb_flip(output);
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Resets (or creates) the flip timeout timer in the main loop thread.
 *
 * This function is called asynchronously to ensure the flip timeout timer
 * is managed safely from the main loop. It deletes any existing timer
 * and adds a new one.
 *
 * @param data Pointer to the Ecore_Drm2_Output for which to reset the timer.
 */
static void
_cb_mainloop_async_timer_reset(void *data)
{
   Ecore_Drm2_Output *output = data;
   if (output->flip_timeout) ecore_timer_del(output->flip_timeout);
   output->flip_timeout = ecore_timer_add(FLIP_TIMEOUT, _cb_flip_timeout, output);
}

/**
 * @internal
 * @brief Performs a page flip using atomic modesetting.
 *
 * This function commits an atomic request to perform a page flip.
 * It uses the request previously prepared by `_fb_atomic_flip_test`
 * (stored in `output->prep.atomic_req`). If no request is prepared,
 * it attempts to build one based on the current state.
 * It includes retry logic for EBUSY errors.
 *
 * @param output The Ecore_Drm2_Output on which to flip.
 * @return 0 on success, -1 on failure.
 */
static int
_fb_atomic_flip(Ecore_Drm2_Output *output)
{
   int res = 0;
   uint32_t flags =
     DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT |
     DRM_MODE_ATOMIC_ALLOW_MODESET;

   if (!_ecore_drm2_use_atomic) return -1;

   /* If we have no req yet, we're flipping to current state.
    * rebuild the current state in the prep state */
   if (!output->prep.atomic_req) _fb_atomic_flip_test(output);

   /* Still no req is a bad situation */
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->prep.atomic_req, -1);

   // sometimes we get a EBUSY ... so try again a few times.
   int i;
   for (i = 0; i < 10; i++)
     {
        res =
          sym_drmModeAtomicCommit(output->fd, output->prep.atomic_req, flags,
                                  output);
        if (res == 0) break;
        else ERR("DRM atomic commit failed - retry #%i", i + 1);
        usleep(100);
     }

   if (res < 0)
     {
        ERR("Failed Atomic Commit: %m");
        return -1;
     }
   else
     {
        // XXX: output ref++
        ecore_main_loop_thread_safe_call_async
          (_cb_mainloop_async_timer_reset, output);
     }

   return 0;
}

/**
 * @internal
 * @brief Performs a page flip using the legacy drmModePageFlip ioctl.
 *
 * This function handles page flipping for systems that do not support
 * atomic modesetting. It first ensures the CRTC is set correctly if
 * the framebuffer changes or is being set for the first time.
 * It then calls drmModePageFlip and includes robust retry logic
 * for EBUSY errors, which can occur on some drivers.
 *
 * @param output The Ecore_Drm2_Output on which to flip.
 * @return 0 on success or if the flip is queued, a negative error code on failure.
 */
static int
_fb_flip(Ecore_Drm2_Output *output)
{
   Ecore_Drm2_Fb *fb;
   Eina_Bool repeat;
   int count = 0;
   int ret = 0;

   fb = output->prep.fb;
   if (!fb)
     {
        fb =  output->pending.fb;
        ERR("Trying to flip NULL fb - fallback to pending fb");
     }
   if (!fb)
     {
        ERR("Pending fb is also NULL, give up flipping");
        return ret;
     }

   if ((!output->current.fb) ||
       (output->current.fb->strides[0] != fb->strides[0]))
     {
        ret =
          sym_drmModeSetCrtc(fb->fd, output->crtc_id, fb->id,
                             0, 0, &output->conn_id, 1,
                             &output->current_mode->info);
        if (ret)
          {
             ERR("Failed to set Mode %dx%d for Output %s: %m",
                 output->current_mode->width, output->current_mode->height,
                 output->name);
             return ret;
          }

        if (output->current.fb)
          _ecore_drm2_fb_buffer_release(output, &output->current);
        output->current.fb = fb;
        _ecore_drm2_fb_ref(output->current.fb);
        output->next.fb = NULL;
        /* We used to return here, but now that the ticker is fixed this
         * can leave us hanging waiting for a tick to happen forever.
         * Instead, we now fall through to the flip path to make sure
         * even this first set can cause a flip callback.
         */
     }

   do
     {
        static Eina_Bool bugged_about_bug = EINA_FALSE;
        repeat = EINA_FALSE;
        ret = sym_drmModePageFlip(fb->fd, output->crtc_id, fb->id,
                                  DRM_MODE_PAGE_FLIP_EVENT, output);
        /* Some drivers (RPI - looking at you) are broken and produce
         * flip events before they are ready for another flip, so be
         * a little robust in the face of badness and try a few times
         * until we can flip or we give up (100 tries with a yield
         * between each try). We can't expect everyone to run the
         * latest bleeding edge kernel IF a workaround is possible
         * in userspace, so do this.
         * We only report this as an ERR once since if it will
         * generate a huge amount of spam otherwise. */
        if ((ret < 0) && (errno == EBUSY))
          {
             repeat = EINA_TRUE;
             if (count == 0 && !bugged_about_bug)
               {
                  ERR("Pageflip fail - EBUSY from drmModePageFlip - "
                      "This is either a kernel bug or an EFL one.");
                  bugged_about_bug = EINA_TRUE;
               }
             count++;
             if (count > 500)
               {
                  ERR("Pageflip EBUSY for %i tries - give up", count);
                  break;
               }
             usleep(100);
          }
        else
          {
             // XXX: output ref++
             ecore_main_loop_thread_safe_call_async
               (_cb_mainloop_async_timer_reset, output);
          }
     }
   while (repeat);

   if ((ret == 0) && (count > 0))
     DBG("Pageflip finally succeeded after %i tries due to EBUSY", count);

   if ((ret < 0) && (errno != EBUSY))
     {
        ERR("Pageflip Failed for Crtc %u on Connector %u: %m",
            output->crtc_id, output->conn_id);
        return ret;
     }
   else if (ret < 0)
     {
        output->next.fb = fb;
        _ecore_drm2_fb_ref(output->next.fb);
     }

   return 0;
}

/**
 * @brief Requests a page flip on an output to display the given framebuffer.
 *
 * This function schedules the provided framebuffer (`fb`) to be displayed
 * on the specified `output`. If a flip is already pending, the new `fb`
 * is queued. If `fb` is NULL, it attempts to flip to a previously queued
 * framebuffer or, as a last resort, to the current framebuffer (to generate a tick).
 *
 * The actual flip is performed using either atomic modesetting or the legacy
 * page flip ioctl, depending on availability.
 *
 * @param fb The Ecore_Drm2_Fb to flip to. Can be NULL to re-flip current or next.
 * @param output The Ecore_Drm2_Output to flip on.
 * @return 0 on success (flip initiated or queued), -1 on error.
 */
EAPI int
ecore_drm2_fb_flip(Ecore_Drm2_Fb *fb, Ecore_Drm2_Output *output)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->current_mode, -1);

   if (!output->enabled) return -1;

   if (fb) _ecore_drm2_fb_ref(fb);

   if (output->pending.fb)
     {
        if (output->next.fb)
          _ecore_drm2_fb_buffer_release(output, &output->next);
        output->next.fb = fb;
        return 0;
     }
   if (!fb)
     {
        fb = output->next.fb;
        output->next.fb = NULL;
     }

   /* So we can generate a tick by flipping to the current fb */
   if (!fb) fb = output->current.fb;

   if (output->next.fb)
     _ecore_drm2_fb_buffer_release(output, &output->next);

   /* If we don't have an fb to set by now, BAIL! */
   if (!fb) return -1;

   output->prep.fb = fb;

   if (_ecore_drm2_use_atomic)
     ret = _fb_atomic_flip(output);
   else
     ret = _fb_flip(output);

   if (ret)
     {
        if (output->prep.fb != output->current.fb)
          _ecore_drm2_fb_buffer_release(output, &output->prep);
        return ret;
     }
   output->pending.fb = output->prep.fb;
   output->prep.fb = NULL;

   if (_ecore_drm2_use_atomic)
     {
        output->pending.atomic_req = output->prep.atomic_req;
        output->prep.atomic_req = NULL;
     }

   return 0;
}

/**
 * @brief Checks if a framebuffer is currently busy.
 *
 * A framebuffer is considered busy if its reference count is greater than 1.
 * This typically means it's either currently being scanned out, pending a flip,
 * or queued for a future flip.
 *
 * @param fb The Ecore_Drm2_Fb to check.
 * @return EINA_TRUE if the framebuffer is busy, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm2_fb_busy_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(fb->dead, EINA_FALSE);

   return !!(fb->ref - 1);
}

/**
 * @brief Releases framebuffers associated with an output.
 *
 * This function attempts to release framebuffers queued or displayed on an output.
 * It prioritizes releasing the "next" framebuffer (one queued by a recent
 * `ecore_drm2_fb_flip` call but not yet processed due to a pending flip).
 *
 * If `panic` is EINA_TRUE, it will also attempt to release the "current"
 * and then "pending" framebuffers if no "next" framebuffer exists. This
 * is a more aggressive release, typically used in situations like surface
 * resize where all buffers need to be reclaimed, potentially causing visual
 * artifacts like tearing.
 *
 * @param o The Ecore_Drm2_Output from which to release framebuffers.
 * @param panic If EINA_TRUE, attempt to release current/pending buffers as well.
 * @return EINA_TRUE if a buffer was successfully released, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm2_fb_release(Ecore_Drm2_Output *o, Eina_Bool panic)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(o, EINA_FALSE);

   if (o->next.fb)
     {
        _ecore_drm2_fb_buffer_release(o, &o->next);
        return EINA_TRUE;
     }
   if (!panic) return EINA_FALSE;

   /* This has been demoted to DBG from WRN because we
    * call this function to reclaim all buffers on a
    * surface resize.
    */
   DBG("Buffer release request when no next buffer");
   /* If we have to release these we're going to see tearing.
    * Try to reclaim in decreasing order of visual awfulness
    */
   if (o->current.fb)
     {
        _ecore_drm2_fb_buffer_release(o, &o->current);
        return EINA_TRUE;
     }

   if (o->pending.fb)
     {
        _ecore_drm2_fb_buffer_release(o, &o->pending);
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

/**
 * @brief Gets the underlying GBM buffer object (struct gbm_bo) of a framebuffer.
 *
 * This function is only relevant for framebuffers created using
 * `ecore_drm2_fb_gbm_create`.
 *
 * @param fb The Ecore_Drm2_Fb.
 * @return A pointer to the GBM buffer object, or NULL if not a GBM framebuffer or on error.
 */
EAPI void *
ecore_drm2_fb_bo_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(fb->dead, NULL);

   return fb->gbm_bo;
}

/**
 * @brief Imports a framebuffer from dmabuf file descriptors.
 *
 * This function creates an Ecore_Drm2_Fb by importing pixel data
 * from one or more dmabuf file descriptors. This is used for
 * zero-copy buffer sharing between different processes or components.
 *
 * @param dev The Ecore_Drm2_Device to import the framebuffer to.
 * @param width The width of the framebuffer in pixels.
 * @param height The height of the framebuffer in pixels.
 * @param depth The color depth of the framebuffer.
 * @param bpp The bits per pixel of the framebuffer.
 * @param format The pixel format (e.g., DRM_FORMAT_XRGB8888).
 * @param strides Array of strides for each plane. For single-plane formats,
 *                only `strides[0]` is used.
 *                Example for a single plane: `unsigned int strides[4] = {1920 * 4, 0, 0, 0};`
 * @param dmabuf_fd Array of file descriptors for each plane's dmabuf.
 *                  Example for a single plane: `int dmabuf_fd[4] = {fd, -1, -1, -1};`
 * @param dmabuf_fd_count The number of valid file descriptors in `dmabuf_fd` (number of planes).
 * @return A pointer to the newly created Ecore_Drm2_Fb on success, NULL otherwise.
 */
EAPI Ecore_Drm2_Fb *
ecore_drm2_fb_dmabuf_import(Ecore_Drm2_Device *dev, int width, int height, int depth, int bpp, unsigned int format, unsigned int strides[4], int dmabuf_fd[4], int dmabuf_fd_count)
{
   int i;
   Ecore_Drm2_Fb *fb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);

   fb = calloc(1, sizeof(Ecore_Drm2_Fb));
   if (!fb) return NULL;

   for (i = 0; i < dmabuf_fd_count; i++)
     if (sym_drmPrimeFDToHandle(dev->fd, dmabuf_fd[i], &fb->handles[i]))
       goto fail;

   fb->dmabuf = EINA_TRUE;
   fb->fd = dev->fd;
   fb->w = width;
   fb->h = height;
   fb->bpp = bpp;
   fb->depth = depth;
   fb->format = format;
   fb->ref = 1;

   memcpy(&fb->strides, strides, sizeof(fb->strides));
   if (_fb2_create(fb)) return fb;

fail:
   free(fb);
   return NULL;
}

/**
 * @brief Sets a status handler function for a framebuffer.
 *
 * The status handler is a callback function that will be invoked when
 * certain events occur for the framebuffer, such as being released,
 * deleted, or when its scanout status changes.
 *
 * @param fb The Ecore_Drm2_Fb for which to set the handler.
 * @param handler The callback function to set.
 *                Example:
 *                ```c
 *                void my_fb_status_handler(Ecore_Drm2_Fb *fb, Ecore_Drm2_Fb_Status status, void *user_data)
 *                {
 *                    // Handle status change
 *                }
 *                ```
 * @param data User-defined data to be passed to the handler function.
 */
EAPI void
ecore_drm2_fb_status_handler_set(Ecore_Drm2_Fb *fb, Ecore_Drm2_Fb_Status_Handler handler, void *data)
{
   fb->status_handler = handler;
   fb->status_data = data;
}
