/*
 * \@file evas_filter.c
 *
 * Infrastructure for simple filters applied to RGBA and Alpha buffers.
 * Originally used by font effects.
 *
 * Filters include:
 * - Blur (Gaussian, Box, Motion) and Shadows
 * - Bump maps (light effects)
 * - Displacement maps
 * - Color curves
 * - Blending and masking
 *
 * The reference documentation can be found in evas_filter_parser.c
 */

#include "evas_filter.h"

#include "evas_filter_private.h"
#include <Ector.h>
#include <software/Ector_Software.h>
#include "evas_ector_buffer.eo.h"

#define _assert(a) if (!(a)) CRI("Failed on %s", #a);

/**
 * @internal
 * @brief Frees an Evas_Filter_Buffer and its associated resources.
 * @param fb The filter buffer to free.
 */
static void _buffer_free(Evas_Filter_Buffer *fb);

/**
 * @internal
 * @brief Deletes an Evas_Filter_Command and frees its resources.
 * @param ctx The filter context.
 * @param cmd The filter command to delete.
 */
static void _command_del(Evas_Filter_Context *ctx, Evas_Filter_Command *cmd);

/**
 * @internal
 * @brief Allocates a new Evas_Filter_Buffer and its underlying Ector_Buffer.
 * @param ctx The filter context.
 * @param w The width of the buffer.
 * @param h The height of the buffer.
 * @param alpha_only EINA_TRUE if the buffer is alpha-only, EINA_FALSE otherwise.
 * @param render EINA_TRUE if the buffer is renderable.
 * @param draw EINA_TRUE if the buffer is drawable.
 * @return A pointer to the newly allocated Evas_Filter_Buffer, or NULL on failure.
 */
static Evas_Filter_Buffer *_buffer_alloc_new(Evas_Filter_Context *ctx, int w, int h, Eina_Bool alpha_only, Eina_Bool render, Eina_Bool draw);

/**
 * @internal
 * @brief Unlocks all transient buffers in the filter context.
 *        Transient buffers are locked when retrieved by evas_filter_temporary_buffer_get().
 * @param ctx The filter context.
 */
static void _filter_buffer_unlock_all(Evas_Filter_Context *ctx);

#define DRAW_COLOR_SET(r, g, b, a) do { cmd->draw.R = r; cmd->draw.G = g; cmd->draw.B = b; cmd->draw.A = a; } while (0)
#define DRAW_CLIP_SET(_x, _y, _w, _h) do { cmd->draw.clip.x = _x; cmd->draw.clip.y = _y; cmd->draw.clip.w = _w; cmd->draw.clip.h = _h; } while (0)
#define DRAW_FILL_SET(fmode) do { cmd->draw.fillmode = fmode; } while (0)

/* Main functions */

#define _free(ptr) free(ptr)
//eina_freeq_ptr_main_add(ptr, NULL, sizeof(*ptr))


/* FIXME: This code is come from a log by
   CRI<14853>:eo lib/eo/eo.c:1894 efl_unref() Calling efl_unref instead of efl_del or efl_parent_set(NULL).
   Temporary fallback in place triggered." When u get correct method, please fix this. */
/**
 * @internal
 * @brief Deletes an Ector buffer (Eo object).
 *        This function handles whether to call efl_del or efl_unref
 *        based on whether the buffer has a parent.
 * @param buffer The Ector buffer to delete.
 */
static void
_buffer_del(Eo *buffer)
{
   if (!buffer) return;

   if (efl_parent_get(buffer))
     efl_del(buffer);
   else
     efl_unref(buffer);
}

/**
 * @brief Creates a new Evas_Filter_Context.
 *
 * This context holds all the information needed to define and run a filter chain,
 * including buffers, commands, and target information.
 *
 * @param evas The Evas public data.
 * @param async If EINA_TRUE, the filter chain will be run asynchronously in a separate thread.
 * @param user_data Custom data to associate with this context.
 * @return A pointer to the newly created Evas_Filter_Context, or NULL on failure.
 *         The caller is responsible for freeing the context using evas_filter_context_unref().
 */
Evas_Filter_Context *
evas_filter_context_new(Evas_Public_Data *evas, Eina_Bool async, void *user_data)
{
   Evas_Filter_Context *ctx;

   EINA_SAFETY_ON_NULL_RETURN_VAL(evas, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(evas->engine.func->gfx_filter_supports, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(evas->engine.func->gfx_filter_process, NULL);

   ctx = calloc(1, sizeof(Evas_Filter_Context));
   if (!ctx) return NULL;

   ctx->evas = evas;
   ctx->async = async;
   ctx->user_data = user_data;
   ctx->buffer_scaled_get = &evas_filter_buffer_scaled_get;
   ctx->gl = (ENFN->gl_surface_read_pixels != NULL);
   ctx->refcount = 1;

   return ctx;
}

/**
 * @brief Retrieves the user data associated with an Evas_Filter_Context.
 * @param ctx The filter context.
 * @return A pointer to the user data, or NULL if no data is set or ctx is NULL.
 */
void *
evas_filter_context_data_get(Evas_Filter_Context *ctx)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   return ctx->user_data;
}

/**
 * @brief Checks if the filter context is set to run asynchronously.
 * @param ctx The filter context.
 * @return EINA_TRUE if the context is asynchronous, EINA_FALSE otherwise.
 */
Eina_Bool
evas_filter_context_async_get(Evas_Filter_Context *ctx)
{
   return ctx->async;
}

/**
 * @brief Gets the base width and height of the filter context.
 *        This size is typically the size of the target output area for the filter.
 * @param ctx The filter context.
 * @param w Pointer to store the width. Can be NULL.
 * @param h Pointer to store the height. Can be NULL.
 */
void
evas_filter_context_size_get(Evas_Filter_Context *ctx, int *w, int *h)
{
   if (w) *w = ctx->w;
   if (h) *h = ctx->h;
}

/**
 * @internal
 * @brief Clears the filter context, removing commands and optionally buffers.
 *        This function is primarily used by the filter parser to reset the context.
 * @param ctx The filter context to clear.
 * @param keep_buffers If EINA_TRUE, buffers are not freed. If EINA_FALSE, all buffers are freed.
 */
void
evas_filter_context_clear(Evas_Filter_Context *ctx, Eina_Bool keep_buffers)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *fb;

   if (!ctx) return;

   if (ctx->target.surface) ENFN->image_free(ENC, ctx->target.surface);
   if (ctx->target.mask) ENFN->image_free(ENC, ctx->target.mask);
   ctx->target.surface = NULL;
   ctx->target.mask = NULL;

   if (!keep_buffers)
     {
        ctx->last_buffer_id = 0;
        EINA_LIST_FREE(ctx->buffers, fb)
          _buffer_free(fb);
     }

   ctx->last_command_id = 0;
   EINA_INLIST_FREE(ctx->commands, cmd)
     _command_del(ctx, cmd);

   // Note: don't reset post_run, as it it set by the client
}

/**
 * @internal
 * @brief Frees the backing Ector_Buffer of an Evas_Filter_Buffer.
 * @param fb The filter buffer whose backing store to free.
 */
static void
_filter_buffer_backing_free(Evas_Filter_Buffer *fb)
{
   if (!fb || !fb->buffer) return;
   _buffer_del((Eo *)fb->buffer);
   fb->buffer = NULL;
}

/**
 * @internal
 * @brief Renders all proxy source objects associated with buffers in the filter context.
 *
 * This function iterates through buffers that are configured as proxy sources.
 * If a proxy source needs rendering (e.g., it's marked for redraw or has no surface),
 * it triggers a sub-render operation for that source. The rendered surface is then
 * wrapped or updated in the corresponding Evas_Filter_Buffer.
 *
 * @param ctx The filter context.
 * @param eo_obj The Evas object to which the filter is being applied.
 * @param output The rendering output target (e.g., a canvas surface).
 * @param do_async EINA_TRUE if rendering should be performed asynchronously.
 */
void
evas_filter_context_proxy_render_all(Evas_Filter_Context *ctx, Eo *eo_obj, void *output,
                                     Eina_Bool do_async)
{
   Evas_Object_Protected_Data *source;
   Evas_Object_Protected_Data *obj;
   void *proxy_surface;
   Evas_Filter_Buffer *fb;
   Eina_List *li;

   if (!ctx->has_proxies) return;
   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     if (fb->source)
       {
          // TODO: Lock current object as proxyrendering (see image obj)
          source = efl_data_scope_get(fb->source, EFL_CANVAS_OBJECT_CLASS);
          _assert(fb->w == source->cur->geometry.w);
          _assert(fb->h == source->cur->geometry.h);
          proxy_surface = source->proxy->surface;
          if (source->proxy->surface && !source->proxy->redraw)
            {
               XDBG("Source already rendered: '%s' of type '%s'",
                   fb->source_name, efl_class_name_get(efl_class_get(fb->source)));
            }
          else
            {
               XDBG("Source needs to be rendered: '%s' of type '%s' (%s)",
                   fb->source_name, efl_class_name_get(efl_class_get(fb->source)),
                   source->proxy->redraw ? "redraw" : "no surface");
               evas_render_proxy_subrender(ctx->evas->evas, output, fb->source, eo_obj, obj, EINA_FALSE, do_async);
            }
          if (fb->buffer)
            {
               void *old_surface;

               old_surface = evas_ector_buffer_drawable_image_get(fb->buffer);
               if (old_surface)
                 {
                    evas_ector_buffer_engine_image_release(fb->buffer, old_surface);
                    if (old_surface && (old_surface != proxy_surface))
                      _filter_buffer_backing_free(fb);
                 }
            }
          XDBG("Source #%d '%s' has dimensions %dx%d", fb->id, fb->source_name, fb->w, fb->h);
          if (!fb->buffer) fb->buffer = ENFN->ector_buffer_wrap(ENC, obj->layer->evas->evas, source->proxy->surface);
          fb->alpha_only = EINA_FALSE;
       }
}

/**
 * @internal
 * @brief Reinitializes render buffers for reuse in a filter program.
 *
 * This function is called when a filter program (a sequence of filter commands)
 * is to be reused. It iterates through the buffers in the context.
 * For buffers marked as `is_render` and not being a proxy source,
 * it clears them by drawing a transparent rectangle if they have a valid surface.
 * This ensures that render buffers are in a clean state before the filter
 * program runs again. It also resets `used`, `locked`, and `dirty` flags.
 *
 * @param engine The graphics engine context.
 * @param output The rendering output target.
 * @param ctx The filter context.
 */
void
_evas_filter_context_program_reuse(void *engine, void *output, Evas_Filter_Context *ctx)
{
   Evas_Filter_Buffer *fb;
   Eina_List *li;

   _filter_buffer_unlock_all(ctx);

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     {
        void *dc, *surface;

        fb->used = EINA_FALSE;
        fb->locked = EINA_FALSE;

        if (!fb->is_render) continue;
        if (fb->source) continue;

        surface = evas_ector_buffer_render_image_get(fb->buffer);
        if (!surface) continue;

        dc = ENFN->context_new(engine);
        ENFN->context_color_set(engine, dc, 0, 0, 0, 0);
        ENFN->context_render_op_set(engine, dc, EVAS_RENDER_COPY);
        ENFN->rectangle_draw(engine, output, dc, surface, 0, 0, fb->w, fb->h, ctx->async);
        ENFN->context_free(engine, dc);
        fb->dirty = EINA_FALSE;

        evas_ector_buffer_engine_image_release(fb->buffer, surface);
     }
}

/**
 * @internal
 * @brief Destroys an Evas_Filter_Context and frees all its resources.
 *        This is typically called when the refcount of the context reaches zero
 *        and the context is not currently running a filter chain.
 * @param data A pointer to the Evas_Filter_Context to destroy.
 */
static void
_context_destroy(void *data)
{
   Evas_Filter_Context *ctx = data;

   EINA_SAFETY_ON_FALSE_RETURN(ctx->refcount == 0);
   evas_filter_context_clear(ctx, EINA_FALSE);
   _free(ctx);
}

/**
 * @brief Increments the reference count of an Evas_Filter_Context.
 * @param ctx The filter context to reference.
 * @return The new reference count, or -1 on error.
 */
int
evas_filter_context_ref(Evas_Filter_Context *ctx)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, -1);

#ifdef FILTERS_DEBUG
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_main_loop_is(), -1);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ctx->refcount > 0, -1);
#endif

   return (++ctx->refcount);
}

/**
 * @brief Decrements the reference count of an Evas_Filter_Context.
 *
 * If the reference count drops to zero and the context is not currently
 * running a filter chain, the context is destroyed. If it is running,
 * destruction is deferred until the post_run_cb is called.
 *
 * @param ctx The filter context to unreference.
 */
void
evas_filter_context_unref(Evas_Filter_Context *ctx)
{
   if (!ctx) return;

#ifdef FILTERS_DEBUG
   EINA_SAFETY_ON_FALSE_RETURN(eina_main_loop_is());
   EINA_SAFETY_ON_FALSE_RETURN(ctx->refcount > 0);
#endif

   if ((--ctx->refcount) != 0) return;

   if (!ctx->running)
     _context_destroy(ctx);
   // else: post_run_cb will be called
}

/**
 * @brief Sets a callback function to be invoked after a filter chain run completes.
 *
 * This callback is useful for cleanup or notification when an asynchronous
 * filter operation finishes, or when a context is unreferenced while running.
 *
 * @param ctx The filter context.
 * @param cb The callback function.
 * @param data User data to be passed to the callback function.
 */
void
evas_filter_context_post_run_callback_set(Evas_Filter_Context *ctx,
                                          Evas_Filter_Cb cb, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(ctx);
   ctx->post_run.cb = cb;
   ctx->post_run.data = data;
}

/**
 * @internal
 * @brief Creates a new, empty Evas_Filter_Buffer structure without an Ector_Buffer.
 *        The Ector_Buffer (backing store) is allocated later.
 * @param ctx The filter context.
 * @param w The width of the buffer.
 * @param h The height of the buffer.
 * @param alpha_only EINA_TRUE if the buffer is alpha-only, EINA_FALSE for ARGB.
 * @param transient EINA_TRUE if this is a temporary buffer that can be reused.
 * @return A pointer to the newly created Evas_Filter_Buffer, or NULL on failure.
 */
static Evas_Filter_Buffer *
_buffer_empty_new(Evas_Filter_Context *ctx, int w, int h, Eina_Bool alpha_only,
                  Eina_Bool transient)
{
   Evas_Filter_Buffer *fb;

   fb = calloc(1, sizeof(Evas_Filter_Buffer));
   if (!fb) return NULL;

   fb->id = ++(ctx->last_buffer_id);
   fb->ctx = ctx;
   fb->alpha_only = alpha_only;
   fb->transient = transient;
   fb->w = w;
   fb->h = h;

   ctx->buffers = eina_list_append(ctx->buffers, fb);
   return fb;
}

/**
 * @internal
 * @brief Creates an Ector_Buffer with specified properties.
 *        This is the actual pixel data storage for an Evas_Filter_Buffer.
 * @param fb Constant pointer to the Evas_Filter_Buffer metadata (width, height, alpha_only).
 * @param render EINA_TRUE if the buffer should be renderable (target for drawing operations).
 * @param draw EINA_TRUE if the buffer should be drawable (source for drawing operations).
 * @return A pointer to the newly created Ector_Buffer, or NULL on failure.
 */
static Ector_Buffer *
_ector_buffer_create(Evas_Filter_Buffer const *fb, Eina_Bool render, Eina_Bool draw)
{
   Efl_Gfx_Colorspace cspace = EFL_GFX_COLORSPACE_ARGB8888;
   Ector_Buffer_Flag flags;

   // FIXME: Once all filters are GL buffers need not be CPU accessible
   flags = ECTOR_BUFFER_FLAG_CPU_READABLE | ECTOR_BUFFER_FLAG_CPU_WRITABLE;
   if (render) flags |= ECTOR_BUFFER_FLAG_RENDERABLE;
   if (draw) flags |= ECTOR_BUFFER_FLAG_DRAWABLE;
   if (fb->alpha_only) cspace = EFL_GFX_COLORSPACE_GRY8;

   return fb->ENFN->ector_buffer_new(FB_ENC, fb->ctx->evas->evas,
                                     fb->w, fb->h, cspace, flags);
}

/**
 * @brief Allocates backing Ector_Buffers for all necessary Evas_Filter_Buffers in the context.
 *
 * This function iterates through the commands in the filter chain to determine
 * which buffers are used and need allocation. It also handles allocation of
 * temporary buffers required for certain operations like stretching.
 * Unused buffers might be cleaned up.
 *
 * @param ctx The filter context.
 * @return EINA_TRUE on success, EINA_FALSE if any buffer allocation fails.
 */
Eina_Bool
evas_filter_context_buffers_allocate_all(Evas_Filter_Context *ctx)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *fb;
   Eina_List *li, *li2;
   unsigned w, h;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_FALSE);
   w = ctx->w;
   h = ctx->h;

   XDBG("Allocating all buffers based on output size %ux%u", w, h);

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     fb->cleanup = EINA_TRUE;

   EINA_INLIST_FOREACH(ctx->commands, cmd)
     {
        Evas_Filter_Fill_Mode fillmode = cmd->draw.fillmode;
        Evas_Filter_Buffer *in, *out;

        in = cmd->input;
        EINA_SAFETY_ON_NULL_GOTO(in, alloc_fail);

        in->cleanup = EINA_FALSE;
        if (!in->w && !in->h)
          {
             in->w = w;
             in->h = h;
          }

        if (cmd->mask)
          cmd->mask->cleanup = EINA_FALSE;

        // FIXME: No need for stretch buffers with GL!
        if (fillmode & EVAS_FILTER_FILL_MODE_STRETCH_XY)
          {
             unsigned sw = w, sh = h;

             switch (cmd->mode)
               {
                case EVAS_FILTER_MODE_BLEND:
                  in = cmd->input;
                  break;
                case EVAS_FILTER_MODE_BUMP:
                case EVAS_FILTER_MODE_DISPLACE:
                case EVAS_FILTER_MODE_MASK:
                  in = cmd->mask;
                  break;
                default:
                  CRI("Invalid fillmode set for command %d", cmd->mode);
                  return EINA_FALSE;
               }

             EINA_SAFETY_ON_NULL_GOTO(in, alloc_fail);
             if (in->w) sw = in->w;
             if (in->h) sh = in->h;

             if ((sw != w) || (sh != h))
               {
                  if (fillmode & EVAS_FILTER_FILL_MODE_STRETCH_X)
                    sw = w;
                  if (fillmode & EVAS_FILTER_FILL_MODE_STRETCH_Y)
                    sh = h;

                  fb = _buffer_alloc_new(ctx, sw, sh, in->alpha_only, 1, 1);
                  XDBG("Allocated temporary buffer #%d of size %ux%u %s",
                       fb ? fb->id : -1, sw, sh, in->alpha_only ? "alpha" : "rgba");
                  if (!fb) goto alloc_fail;
                  fb->transient = EINA_TRUE;
                  fb->cleanup = EINA_FALSE;
               }
          }

        if (cmd->draw.need_temp_buffer)
          {
             unsigned sw = w, sh = h;

             in = cmd->input;
             if (in->w) sw = in->w;
             if (in->h) sh = in->h;

             fb = _buffer_alloc_new(ctx, sw, sh, in->alpha_only, 1, 1);
             XDBG("Allocated temporary buffer #%d of size %ux%u %s",
                  fb ? fb->id : -1, sw, sh, in->alpha_only ? "alpha" : "rgba");
             if (!fb) goto alloc_fail;
             fb->transient = EINA_TRUE;
             fb->cleanup = EINA_FALSE;
          }

        out = cmd->output;
        out->cleanup = EINA_FALSE;
        if (!out->w && !out->h)
          {
             out->w = w;
             out->h = h;
          }
     }

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     {
        Eina_Bool render = EINA_FALSE, draw = EINA_FALSE;

        if (fb->source)
          {
             fb->cleanup = EINA_FALSE;
             continue;
          }
        if (fb->buffer || fb->cleanup)
          continue;

        if (!fb->w && !fb->h)
          {
             ERR("Size of buffer %d should be known at this point. Is this a dangling buffer?", fb->id);
             continue;
          }

        // Skip input buffer, allocate it in input render phase
        if (fb->id == EVAS_FILTER_BUFFER_INPUT_ID)
          continue;

        render = fb->is_render || fb->transient;
        draw |= (fb->id == EVAS_FILTER_BUFFER_OUTPUT_ID);

        fb->buffer = _ector_buffer_create(fb, render, draw);
        XDBG("Allocated buffer #%d of size %ux%u %s: %p",
             fb->id, fb->w, fb->h, fb->alpha_only ? "alpha" : "rgba", fb->buffer);
        if (!fb->buffer) goto alloc_fail;
     }

   EINA_LIST_FOREACH_SAFE(ctx->buffers, li, li2, fb)
     {
        if (fb->cleanup)
          {
             XDBG("Cleanup buffer #%d %dx%d %s", fb->id, fb->w, fb->h, fb->alpha_only ? "alpha" : "rgba");
             ctx->buffers = eina_list_remove_list(ctx->buffers, li);
             _buffer_free(fb);
          }
     }

   return EINA_TRUE;

alloc_fail:
   ERR("Buffer allocation failed! Context size: %dx%d", w, h);
   return EINA_FALSE;
}

/**
 * @brief Creates or retrieves an empty buffer with specified dimensions and format.
 *
 * If a suitable, unused buffer already exists in the context, it is reused.
 * Otherwise, a new Evas_Filter_Buffer structure is created (without allocating
 * the Ector_Buffer yet). The buffer is marked as used.
 *
 * @param ctx The filter context.
 * @param w The desired width of the buffer.
 * @param h The desired height of thebuffer.
 * @param alpha_only EINA_TRUE for an alpha-only buffer, EINA_FALSE for ARGB.
 * @return The ID of the (newly created or reused) buffer, or -1 on failure.
 */
int
evas_filter_buffer_empty_new(Evas_Filter_Context *ctx, int w, int h, Eina_Bool alpha_only)
{
   Evas_Filter_Buffer *fb;
   Eina_List *li;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, -1);

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     {
        if ((fb->alpha_only == alpha_only) &&
            (fb->w == w) && (fb->h == h) && !fb->dirty && !fb->used)
          {
             fb->used = EINA_TRUE;
             return fb->id;
          }
     }

   fb = _buffer_empty_new(ctx, w, h, alpha_only, EINA_FALSE);
   if (!fb) return -1;

   fb->used = EINA_TRUE;
   return fb->id;
}

/**
 * @brief Creates or retrieves a buffer that acts as a proxy for an Evas_Object.
 *
 * This allows an Evas_Object (e.g., another image, text) to be used as an input
 * source within the filter chain. If a proxy buffer for the same Evas_Object
 * with the same name and dimensions already exists and is unused, it's reused.
 * Otherwise, a new proxy buffer is created. The dimensions of the source object
 * are returned via the w and h parameters.
 *
 * @param ctx The filter context.
 * @param pb Pointer to an Evas_Filter_Proxy_Binding structure containing the
 *           Evas_Object source (eo_source) and its name.
 *           Example:
 *           Evas_Filter_Proxy_Binding binding = { .eo_source = source_object, .name = "my_source" };
 * @param w Pointer to store the width of the proxy source.
 * @param h Pointer to store the height of the proxy source.
 * @return The ID of the (newly created or reused) proxy buffer, or -1 on failure
 *         (e.g., source object is invalid, or a conflicting buffer exists).
 */
int
evas_filter_buffer_proxy_new(Evas_Filter_Context *ctx, Evas_Filter_Proxy_Binding *pb,
                             int *w, int *h)
{
   Evas_Object_Protected_Data *source;
   Evas_Filter_Buffer *fb;
   Eina_List *li;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pb, -1);

   source = efl_data_scope_get(pb->eo_source, EFL_CANVAS_OBJECT_CLASS);
   if (!source) return -1;

   // FIXME: This is not true if the source is an evas image
   *w = source->cur->geometry.w;
   *h = source->cur->geometry.h;

   EINA_LIST_FOREACH(ctx->buffers, li, fb)
     {
        if (pb->eo_source == fb->source)
          {
             if (fb->used) return -1;
             if (fb->alpha_only) return -1;
             if (!eina_streq(pb->name, fb->source_name)) return -1;
             if ((*w != fb->w) || (*h != fb->h)) return -1;

             fb->used = EINA_TRUE;
             return fb->id;
          }
     }

   fb = _buffer_empty_new(ctx, *w, *h, EINA_FALSE, EINA_FALSE);
   if (!fb) return -1;

   fb->source = efl_ref(pb->eo_source);
   fb->source_name = eina_stringshare_add(pb->name);

   fb->used = EINA_TRUE;
   return fb->id;
}

static Evas_Filter_Buffer *
_buffer_alloc_new(Evas_Filter_Context *ctx, int w, int h, Eina_Bool alpha_only,
                  Eina_Bool render, Eina_Bool draw)
{
   Evas_Filter_Buffer *fb;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(w > 0 && h > 0, NULL);

   fb = calloc(1, sizeof(Evas_Filter_Buffer));
   if (!fb) return NULL;

   fb->id = ++(ctx->last_buffer_id);
   fb->ctx = ctx;
   fb->w = w;
   fb->h = h;
   fb->alpha_only = alpha_only;
   fb->is_render = render;
   fb->buffer = _ector_buffer_create(fb, render, draw);
   if (!fb->buffer)
     {
        ERR("Failed to create ector buffer!");
        _free(fb);
        return NULL;
     }

   ctx->buffers = eina_list_append(ctx->buffers, fb);
   return fb;
}

static void
_buffer_free(Evas_Filter_Buffer *fb)
{
   _filter_buffer_backing_free(fb);
   eina_stringshare_del(fb->source_name);
   efl_unref(fb->source);
   _free(fb);
}

/**
 * @internal
 * @brief Retrieves an Evas_Filter_Buffer from the context by its ID.
 * @param ctx The filter context.
 * @param bufid The ID of the buffer to retrieve.
 * @return A pointer to the Evas_Filter_Buffer, or NULL if not found.
 */
Evas_Filter_Buffer *
_filter_buffer_get(Evas_Filter_Context *ctx, int bufid)
{
   Evas_Filter_Buffer *buffer;
   Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   EINA_LIST_FOREACH(ctx->buffers, l, buffer)
     if (buffer->id == bufid) return buffer;

   return NULL;
}

/**
 * @brief Gets the engine-specific backing surface/image of a filter buffer.
 *
 * This function retrieves the underlying graphics data (e.g., a software pixel buffer
 * or a GL texture ID) associated with an Evas_Filter_Buffer.
 * If the buffer's Ector_Buffer hasn't been allocated yet, this function might
 * trigger its allocation via evas_filter_buffer_backing_set(..., NULL).
 * The caller receives a new reference to the image and is responsible for
 * releasing it using evas_ector_buffer_engine_image_release().
 *
 * @param ctx The filter context.
 * @param bufid The ID of the buffer.
 * @param render If EINA_TRUE, requests the renderable image (e.g., for use as a texture).
 *               If EINA_FALSE, requests the drawable image (e.g., for CPU access).
 * @return A pointer to the engine-specific image data, or NULL on failure or if the
 *         buffer doesn't exist. The interpretation of this pointer depends on the
 *         Evas engine in use.
 */
void *
evas_filter_buffer_backing_get(Evas_Filter_Context *ctx, int bufid, Eina_Bool render)
{
   Evas_Filter_Buffer *fb;

   fb = _filter_buffer_get(ctx, bufid);
   if (!fb) return NULL;

   if (!fb->buffer)
     evas_filter_buffer_backing_set(ctx, bufid, NULL);

   if (render)
     return evas_ector_buffer_render_image_get(fb->buffer); // ref++
   else
     return evas_ector_buffer_drawable_image_get(fb->buffer); // ref++
}

/**
 * @brief Sets or allocates the engine-specific backing for a filter buffer.
 *
 * If `engine_buffer` is NULL, a new Ector_Buffer is created based on the
 * Evas_Filter_Buffer's properties (size, format, render/draw flags).
 * If `engine_buffer` is provided, it's wrapped by an Ector_Buffer. This is
 * typically used for input buffers that are pre-existing engine surfaces.
 * The function updates the `fb->buffer` pointer.
 *
 * @param ctx The filter context.
 * @param bufid The ID of the buffer.
 * @param engine_buffer An optional, pre-existing engine-specific buffer to wrap.
 *                      If NULL, a new buffer is allocated.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
evas_filter_buffer_backing_set(Evas_Filter_Context *ctx, int bufid,
                               void *engine_buffer)
{
   Evas_Filter_Buffer *fb;
   Eina_Bool ret = EINA_FALSE;
   Eo *buffer = NULL;

   fb = _filter_buffer_get(ctx, bufid);
   if (!fb) return EINA_FALSE;

   if (!engine_buffer)
     {
        buffer = _ector_buffer_create(fb, fb->is_render, EINA_FALSE);
        XDBG("Allocated buffer #%d of size %ux%u %s: %p",
             fb->id, fb->w, fb->h, fb->alpha_only ? "alpha" : "rgba", fb->buffer);
        ret = buffer ? EINA_TRUE : EINA_FALSE;
        goto end;
     }

   if (fb->is_render) goto end;

   buffer = ENFN->ector_buffer_wrap(ENC, ctx->evas->evas, engine_buffer);
   if (!buffer) return EINA_FALSE;

   ret = EINA_TRUE;

end:
   if (fb->buffer != buffer) _buffer_del((Eo *)fb->buffer);
   fb->buffer = buffer;
   return ret;
}

/**
 * @internal
 * @brief Creates a new Evas_Filter_Command structure.
 *
 * Initializes a filter command with the given mode, input/mask/output buffers,
 * and default draw parameters. The new command is added to the context's
 * command list. If an output buffer is specified, it's marked as `is_render`
 * and `dirty`.
 *
 * @param ctx The filter context.
 * @param mode The filter operation mode (e.g., EVAS_FILTER_MODE_BLUR).
 * @param input The input buffer for the command.
 * @param mask An optional mask buffer (used by modes like MASK, BUMP, DISPLACE).
 * @param output The output buffer for the command.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 */
static Evas_Filter_Command *
_command_new(Evas_Filter_Context *ctx, Evas_Filter_Mode mode,
             Evas_Filter_Buffer *input, Evas_Filter_Buffer *mask,
             Evas_Filter_Buffer *output)
{
   Evas_Filter_Command *cmd;

   cmd = calloc(1, sizeof(Evas_Filter_Command));
   if (!cmd) return NULL;

   cmd->id = ++(ctx->last_command_id);
   cmd->ctx = ctx;
   cmd->mode = mode;
   cmd->input = input;
   cmd->mask = mask;
   cmd->output = output;
   cmd->draw.R = 255;
   cmd->draw.G = 255;
   cmd->draw.B = 255;
   cmd->draw.A = 255;
   cmd->draw.rop = EFL_GFX_RENDER_OP_BLEND;
   if (output)
     {
        cmd->draw.output_was_dirty = output->dirty;
        output->is_render = EINA_TRUE;
        output->dirty = EINA_TRUE;
     }

   ctx->commands = eina_inlist_append(ctx->commands, EINA_INLIST_GET(cmd));
   return cmd;
}

static void
_command_del(Evas_Filter_Context *ctx, Evas_Filter_Command *cmd)
{
   if (!ctx || !cmd) return;
   ctx->commands = eina_inlist_remove(ctx->commands, EINA_INLIST_GET(cmd));
   switch (cmd->mode)
     {
      case EVAS_FILTER_MODE_CURVE: _free(cmd->curve.data); break;
      default: break;
     }
   _free(cmd);
}

Evas_Filter_Buffer *
evas_filter_temporary_buffer_get(Evas_Filter_Context *ctx, int w, int h,
                                 Eina_Bool alpha_only, Eina_Bool clean)
{
   Evas_Filter_Buffer *fb = NULL;
   Eina_List *l;

   EINA_LIST_FOREACH(ctx->buffers, l, fb)
     {
        if (fb->transient && !fb->locked && (fb->alpha_only == alpha_only)
            && (!clean || !fb->dirty))
          {
             if ((!w || (w == fb->w)) && (!h || (h == fb->h)))
               {
                  fb->locked = EINA_TRUE;
                  return fb;
               }
          }
     }

   if (ctx->running)
     {
        ERR("Can not create a new buffer while filter is running!");
        return NULL;
     }

   fb = _buffer_empty_new(ctx, w, h, alpha_only, EINA_TRUE);
   if (!fb) return NULL; // Added safety check
   fb->locked = EINA_TRUE;
   fb->is_render = EINA_TRUE;
   XDBG("Created temporary buffer %d %s", fb->id, alpha_only ? "alpha" : "rgba");

   return fb;
}

static void
_filter_buffer_unlock_all(Evas_Filter_Context *ctx)
{
   Evas_Filter_Buffer *buf = NULL;
   Eina_List *l;

   EINA_LIST_FOREACH(ctx->buffers, l, buf)
     buf->locked = EINA_FALSE;
}

/**
 * @brief Adds a fill command to the filter chain.
 *
 * This command fills the specified buffer with a color defined in the
 * draw_context. The fill operation respects the clip region set in the
 * draw_context.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context from which color and clip are retrieved.
 * @param bufid The ID of the buffer to fill. This buffer acts as both input and output.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 *         Example:
 *         // Assuming draw_context is set up with a red color and a clip rectangle
 *         evas_filter_command_fill_add(filter_ctx, evas_draw_context, buffer_id);
 */
Evas_Filter_Command *
evas_filter_command_fill_add(Evas_Filter_Context *ctx, void *draw_context,
                             int bufid)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *buf = NULL;
   int R, G, B, A, cx, cy, cw, ch;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(draw_context, NULL);

   buf = _filter_buffer_get(ctx, bufid);
   if (!buf)
     {
        ERR("Buffer %d does not exist.", bufid);
        return NULL;
     }

   cmd = _command_new(ctx, EVAS_FILTER_MODE_FILL, buf, NULL, buf);
   if (!cmd) return NULL;

   ENFN->context_color_get(ENC, draw_context, &R, &G, &B, &A);
   DRAW_COLOR_SET(R, G, B, A);

   ENFN->context_clip_get(ENC, draw_context, &cx, &cy, &cw, &ch);
   DRAW_CLIP_SET(cx, cy, cw, ch);

   XDBG("Add fill %d with color(%d,%d,%d,%d)", buf->id, R, G, B, A);

   if (!R && !G && !B && !A)
     buf->dirty = EINA_FALSE;

   return cmd;
}

/**
 * @internal
 * @brief Adds a GL-specific blur command or sequence of commands.
 *
 * This function implements blur using a multi-pass approach suitable for GL:
 * 1. Optional downscaling of the input to a temporary buffer (T1) if blur radius is large.
 * 2. Apply X-axis blur from T1 to another temporary buffer (T2) (or directly to output if no Y-blur).
 * 3. Apply Y-axis blur from T2 to the output buffer (or from input if no X-blur).
 * 4. Optional upscaling from the final blur stage to the output buffer if downscaling was used.
 *
 * It handles padding to align pixels for downscaling to avoid artifacts.
 *
 * @param ctx The filter context.
 * @param in The input buffer.
 * @param out The output buffer.
 * @param type The type of blur (e.g., EVAS_FILTER_BLUR_DEFAULT, EVAS_FILTER_BLUR_GAUSSIAN).
 * @param rx Horizontal blur radius.
 * @param ry Vertical blur radius.
 * @param ox Horizontal offset for the final blended result.
 * @param oy Vertical offset for the final blended result.
 * @param count Blur iterations (usually for box blur, Gaussian is single pass effectively).
 * @param R, G, B, A Color multiplier for the blur effect.
 * @param alphaonly EINA_TRUE if only the alpha channel should be affected/produced.
 * @return A pointer to the last Evas_Filter_Command added in the sequence (typically the final blend/upscale),
 *         or NULL on failure (e.g., temporary buffer allocation fails).
 */
static Evas_Filter_Command *
evas_filter_command_blur_add_gl(Evas_Filter_Context *ctx,
                                Evas_Filter_Buffer *in, Evas_Filter_Buffer *out,
                                Evas_Filter_Blur_Type type,
                                int rx, int ry, int ox, int oy, int count,
                                int R, int G, int B, int A, Eina_Bool alphaonly)
{
   Evas_Filter_Command *cmd = NULL;
   Evas_Filter_Buffer *dx_in, *dx_out, *dy_in, *dy_out, *tmp = NULL;
   int down_x = 1, down_y = 1;
   int pad_x = 0, pad_y = 0;
   double dx, dy;

   /* GL blur implementation:
    *
    * - Create intermediate buffer T1, T2
    * - Downscale input to buffer T1
    * - Apply X blur kernel from T1 to T2
    * - Apply Y blur kernel from T2 back to output
    *
    * In order to avoid sampling artifacts when moving or resizing a filtered
    * snapshot, we make sure that we always sample and scale based on the same
    * original pixels positions:
    * - Input pixels must be aligned to down_x,down_y boundaries
    * - T1/T2 buffer size is up to 1px larger than [input / scale_x,y]
    */

   dx = rx;
   dy = ry;
   dx_in = in;
   dy_out = out;

#if 1
   if (type == EVAS_FILTER_BLUR_DEFAULT)
     {
        // Apply downscaling for large enough radii only.
        down_x = 1 << evas_filter_smallest_pow2_larger_than(dx / 2) / 2;
        down_y = 1 << evas_filter_smallest_pow2_larger_than(dy / 2) / 2;

        // Downscaling to max 4 times for perfect picture quality (with
        // the standard scaling fragment shader and SHD_SAM22).
        if (down_x > 4) down_x = 4;
        if (down_y > 4) down_y = 4;

        if (down_x > 1 && down_y > 1)
          {
             int ww, hh;

             pad_x = ctx->x % down_x;
             pad_y = ctx->y % down_y;

             ww = ceil((double) ctx->w / down_x) + 1;
             hh = ceil((double) ctx->h / down_y) + 1;

             tmp = evas_filter_temporary_buffer_get(ctx, ww, hh, in->alpha_only, EINA_TRUE);
             if (!tmp) goto fail;

             dx /= (double) down_x;
             dy /= (double) down_y;

             XDBG("Add GL downscale %d (%dx%d) -> %d (%dx%d)", in->id, in->w, in->h, tmp->id, tmp->w, tmp->h);
             cmd = _command_new(ctx, EVAS_FILTER_MODE_BLEND, in, NULL, tmp);
             if (!cmd) goto fail;
             cmd->draw.fillmode = EVAS_FILTER_FILL_MODE_STRETCH_XY;
             cmd->draw.scale.down = EINA_TRUE;
             cmd->draw.scale.pad_x = pad_x;
             cmd->draw.scale.pad_y = pad_y;
             cmd->draw.scale.factor_x = down_x;
             cmd->draw.scale.factor_y = down_y;
             cmd->draw.alphaonly = alphaonly;
             dx_in = tmp;

             tmp = evas_filter_temporary_buffer_get(ctx, ww, hh, in->alpha_only, EINA_TRUE);
             if (!tmp) goto fail;
             dy_out = tmp;
          }
     }
#endif

   if (EINA_DBL_NONZERO(dx) && EINA_DBL_NONZERO(dy))
     {
        tmp = evas_filter_temporary_buffer_get(ctx, dx_in->w, dx_in->h, in->alpha_only, 1);
        if (!tmp) goto fail;
        dy_in = dx_out = tmp;
     }
   else
     {
        dx_out = out;
        dy_in = in;
     }

   if (EINA_DBL_NONZERO(dx))
     {
        XDBG("Add GL blur %d -> %d (%.2fx%.2f px)", dx_in->id, dx_out->id, dx, 0.0);
        cmd = _command_new(ctx, EVAS_FILTER_MODE_BLUR, dx_in, NULL, dx_out);
        if (!cmd) goto fail;
        cmd->blur.type = type;
        cmd->blur.dx = dx;
        cmd->blur.count = count;
        cmd->draw.alphaonly = alphaonly;
     }

   if (EINA_DBL_NONZERO(dy))
     {
        XDBG("Add GL blur %d -> %d (%.2fx%.2f px)", dy_in->id, dy_out->id, 0.0, dy);
        cmd = _command_new(ctx, EVAS_FILTER_MODE_BLUR, dy_in, NULL, dy_out);
        if (!cmd) goto fail;
        cmd->blur.type = type;
        cmd->blur.dy = dy;
        cmd->blur.count = count;
        cmd->draw.alphaonly = alphaonly;
     }

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   if (cmd->output != out)
     {
        XDBG("Add GL upscale %d (%dx%d) -> %d (%dx%d)",
             cmd->output->id, cmd->output->w, cmd->output->h, out->id, out->w, out->h);
        cmd = _command_new(ctx, EVAS_FILTER_MODE_BLEND, cmd->output, NULL, out);
        if (!cmd) goto fail;
        cmd->draw.fillmode = EVAS_FILTER_FILL_MODE_STRETCH_XY;
        cmd->draw.scale.down = EINA_FALSE;
        cmd->draw.scale.pad_x = pad_x;
        cmd->draw.scale.pad_y = pad_y;
        cmd->draw.scale.factor_x = down_x;
        cmd->draw.scale.factor_y = down_y;
        cmd->draw.alphaonly = alphaonly;
     }

   cmd->draw.ox = ox;
   cmd->draw.oy = oy;
   DRAW_COLOR_SET(R, G, B, A);
   cmd->draw.rop = (in == out) ? EFL_GFX_RENDER_OP_COPY : EFL_GFX_RENDER_OP_BLEND;

   _filter_buffer_unlock_all(ctx);
   return cmd;

fail:
   ERR("Failed to add blur");
   _filter_buffer_unlock_all(ctx);
   return NULL;
}

/**
 * @internal
 * @brief Checks if the current engine supports GL-accelerated blur for the given buffers.
 *
 * It creates a dummy blur command and queries the engine's filter support capabilities.
 *
 * @param ctx The filter context.
 * @param in The input buffer for the hypothetical blur.
 * @param out The output buffer for the hypothetical blur.
 * @return EINA_TRUE if GL blur is supported, EINA_FALSE otherwise.
 */
static Eina_Bool
_blur_support_gl(Evas_Filter_Context *ctx, Evas_Filter_Buffer *in, Evas_Filter_Buffer *out)
{
   Evas_Filter_Command cmd = {};

   cmd.input = in;
   cmd.output = out;
   cmd.mode = EVAS_FILTER_MODE_BLUR;
   cmd.ctx = ctx;
   cmd.blur.type = EVAS_FILTER_BLUR_GAUSSIAN;
   cmd.blur.dx = 5;

   return cmd.ENFN->gfx_filter_supports(_evas_engine_context(cmd.ctx->evas), &cmd) == EVAS_FILTER_SUPPORT_GL;
}

/**
 * @brief Adds a blur command to the filter chain.
 *
 * This function adds a blur effect. It can be a 1D (horizontal or vertical) or 2D blur.
 * If the engine supports GL blur (_blur_support_gl returns true), it delegates to
 * evas_filter_command_blur_add_gl. Otherwise, it sets up a sequence of commands
 * for software blur, potentially using temporary buffers for multi-pass operations
 * (e.g., X-blur then Y-blur, or handling in-place blurs).
 *
 * For EVAS_FILTER_BLUR_DEFAULT, it adaptively chooses blur type (Gaussian or Box)
 * and iterations based on radius for a balance of quality and performance in software.
 *
 * @param ctx The filter context.
 * @param drawctx The Evas draw context (used for color multiplier and render operation).
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @param type The type of blur (e.g., EVAS_FILTER_BLUR_GAUSSIAN, EVAS_FILTER_BLUR_BOX, EVAS_FILTER_BLUR_DEFAULT).
 * @param dx Horizontal blur radius.
 * @param dy Vertical blur radius.
 * @param ox Horizontal offset for the final result.
 * @param oy Vertical offset for the final result.
 * @param count Number of blur iterations (primarily for EVAS_FILTER_BLUR_BOX).
 *              If 0 and type is EVAS_FILTER_BLUR_DEFAULT, count is determined automatically.
 * @param alphaonly EINA_TRUE to blur only the alpha channel, EINA_FALSE for all channels.
 * @return A pointer to the (last) Evas_Filter_Command added, or NULL on failure.
 *         Example:
 *         // Blur buffer_A into buffer_B with a 5px radius Gaussian blur
 *         evas_filter_command_blur_add(filter_ctx, evas_draw_context, buffer_A_id, buffer_B_id,
 *                                      EVAS_FILTER_BLUR_GAUSSIAN, 5, 5, 0, 0, 1, EINA_FALSE);
 */
Evas_Filter_Command *
evas_filter_command_blur_add(Evas_Filter_Context *ctx, void *drawctx,
                             int inbuf, int outbuf, Evas_Filter_Blur_Type type,
                             int dx, int dy, int ox, int oy, int count,
                             Eina_Bool alphaonly)
{
   Evas_Filter_Buffer *in = NULL, *out = NULL, *tmp = NULL, *in_dy = NULL;
   Evas_Filter_Buffer *out_dy = NULL, *out_dx = NULL;
   Evas_Filter_Buffer *copybuf = NULL, *blendbuf = NULL;
   Evas_Filter_Command *cmd = NULL;
   int R, G, B, A, render_op;
   Eina_Bool override;
   DATA32 color;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(drawctx, NULL);

   if (dx < 0) dx = 0;
   if (dy < 0) dy = 0;
   if (!dx && !dy)
     {
        XDBG("Changing 0px blur into simple blend");
        return evas_filter_command_blend_add(ctx, drawctx, inbuf, outbuf, ox, oy, EVAS_FILTER_FILL_MODE_NONE, alphaonly);
     }

   in = _filter_buffer_get(ctx, inbuf);
   EINA_SAFETY_ON_FALSE_GOTO(in, fail);

   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_FALSE_GOTO(out, fail);

   ENFN->context_color_get(ENC, drawctx, &R, &G, &B, &A);
   color = ARGB_JOIN(A, R, G, B);
   if (!color)
     {
        DBG("Blur with transparent color. Nothing to do.");
        return _command_new(ctx, EVAS_FILTER_MODE_SKIP, NULL, NULL, NULL);
     }

   if (_blur_support_gl(ctx, in, out))
     return evas_filter_command_blur_add_gl(ctx, in, out, type, dx, dy, ox, oy,
                                            count, R, G, B, A, alphaonly);

   // Note (SW engine):
   // The basic blur operation overrides the pixels in the target buffer,
   // only supports one direction (X or Y) and no offset. As a consequence
   // most cases require intermediate work buffers.

   if (in == out) out->dirty = EINA_FALSE;

   render_op = ENFN->context_render_op_get(ENC, drawctx);
   override = (render_op == EVAS_RENDER_COPY);

   switch (type)
     {
      case EVAS_FILTER_BLUR_GAUSSIAN:
        count = 1;
        break;

      case EVAS_FILTER_BLUR_BOX:
        count = MIN(MAX(1, count), 6);
        break;

      case EVAS_FILTER_BLUR_DEFAULT:
        {
           /* In DEFAULT mode we cheat, depending on the size of the kernel:
            * For 1px to 2px, use true Gaussian blur.
            * For 3px to 6px, use two Box blurs.
            * For more than 6px, use three Box blurs.
            * This will give both nicer and MUCH faster results than Gaussian.
            *
            * NOTE: This step should be avoided in GL.
            */

           int tmp_out = outbuf;
           int tmp_in = inbuf;
           int tmp_ox = ox;
           int tmp_oy = oy;

           // For 2D blur: create intermediate buffer
           if (dx && dy)
             {
                tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 1);
                if (!tmp) goto fail;
                tmp_in = tmp_out = tmp->id;
                tmp_ox = tmp_oy = 0;
             }

           // X box blur
           if (dx)
             {
                if (dx <= 2)
                  type = EVAS_FILTER_BLUR_GAUSSIAN;
                else
                  type = EVAS_FILTER_BLUR_BOX;

                if (dy) ENFN->context_color_set(ENC, drawctx, 255, 255, 255, 255);
                cmd = evas_filter_command_blur_add(ctx, drawctx, inbuf, tmp_out,
                                                   type, dx, 0, tmp_ox, tmp_oy, 0,
                                                   alphaonly);
                if (!cmd) goto fail;
                cmd->blur.auto_count = EINA_TRUE;
                if (dy) ENFN->context_color_set(ENC, drawctx, R, G, B, A);
             }

           // Y box blur
           if (dy)
             {
                if (dy <= 2)
                  type = EVAS_FILTER_BLUR_GAUSSIAN;
                else
                  type = EVAS_FILTER_BLUR_BOX;

                if (dx && (inbuf == outbuf))
                  ENFN->context_render_op_set(ENC, drawctx, EVAS_RENDER_COPY);
                cmd = evas_filter_command_blur_add(ctx, drawctx, tmp_in, outbuf,
                                                   type, 0, dy, ox, oy, 0,
                                                   alphaonly);
                if (dx && (inbuf == outbuf))
                  ENFN->context_render_op_set(ENC, drawctx, render_op);
                if (!cmd) goto fail;
                cmd->blur.auto_count = EINA_TRUE;
             }

           return cmd;
        }

      default:
        CRI("Not implemented yet!");
        goto fail;
     }

   // For 2D blur: create intermediate buffer between X and Y passes
   if (dx && dy)
     {
        // If there's an offset: create intermediate buffer before offset blend
        if (ox || oy)
          {
             copybuf = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!copybuf) goto fail;
          }

        // Intermediate buffer between X and Y passes
        tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
        if (!tmp) goto fail;

        if (in == out)
          {
             // IN = OUT and 2-D blur. IN -blur-> TMP -blur-> IN.
             out_dx = tmp;
             in_dy = tmp;
             out_dy = copybuf ? copybuf : in;
          }
        else
          {
             // IN != OUT and 2-D blur. IN -blur-> TMP -blur-> OUT.
             out_dx = tmp;
             in_dy = tmp;
             out_dy = copybuf ? copybuf : out;
          }
     }
   else if (dx)
     {
        // X blur only
        if (in == out)
          {
             // IN = OUT and 1-D blur. IN -blur-> TMP -copy-> IN.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             copybuf = tmp;
             out_dx = tmp;
          }
        else if (ox || oy || (color != 0xFFFFFFFF))
          {
             // IN != OUT and 1-D blur. IN -blur-> TMP -blend-> OUT.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             blendbuf = tmp;
             out_dx = tmp;
          }
        else if (out->dirty)
          {
             // IN != OUT and 1-D blur. IN -blur-> TMP -blend-> OUT.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             blendbuf = tmp;
             out_dx = tmp;
          }
        else
          {
             // IN != OUT and 1-D blur. IN -blur-> OUT.
             out_dx = out;
          }
     }
   else
     {
        // Y blur only
        if (in == out)
          {
             // IN = OUT and 1-D blur. IN -blur-> TMP -copy-> IN.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             copybuf = tmp;
             in_dy = in;
             out_dy = tmp;
          }
        else if (ox || oy || (color != 0xFFFFFFFF))
          {
             // IN != OUT and 1-D blur. IN -blur-> TMP -blend-> IN.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             if (override)
               copybuf = tmp;
             else
               blendbuf = tmp;
             in_dy = in;
             out_dy = tmp;
          }
        else if (out->dirty && !override)
          {
             // IN != OUT and 1-D blur. IN -blur-> TMP -blend-> OUT.
             tmp = evas_filter_temporary_buffer_get(ctx, 0, 0, in->alpha_only, 0);
             if (!tmp) goto fail;
             blendbuf = tmp;
             in_dy = in;
             out_dy = tmp;
          }
        else
          {
             // IN != OUT and 1-D blur. IN -blur-> OUT.
             in_dy = in;
             out_dy = out;
          }
     }

   if (dx)
     {
        XDBG("Add horizontal blur %d -> %d (%dpx)", in->id, out_dx->id, dx);
        cmd = _command_new(ctx, EVAS_FILTER_MODE_BLUR, in, NULL, out_dx);
        if (!cmd) goto fail;
        cmd->blur.type = type;
        cmd->blur.dx = dx;
        cmd->blur.dy = 0;
        cmd->blur.count = count;
        if (!dy) DRAW_COLOR_SET(R, G, B, A);
     }

   if (dy)
     {
        XDBG("Add vertical blur %d -> %d (%dpx)", in_dy->id, out_dy->id, dy);
        cmd = _command_new(ctx, EVAS_FILTER_MODE_BLUR, in_dy, NULL, out_dy);
        if (!cmd) goto fail;
        cmd->blur.type = type;
        cmd->blur.dx = 0;
        cmd->blur.dy = dy;
        cmd->blur.count = count;
        DRAW_COLOR_SET(R, G, B, A);
     }

   if (blendbuf)
     {
        Evas_Filter_Command *blendcmd;

        XDBG("Add extra blend %d -> %d", blendbuf->id, out->id);
        blendcmd = evas_filter_command_blend_add(ctx, drawctx,
                                                 blendbuf->id, out->id, ox, oy,
                                                 EVAS_FILTER_FILL_MODE_NONE,
                                                 alphaonly);
        if (!blendcmd) goto fail;
        ox = oy = 0;
     }
   else if (copybuf)
     {
        Evas_Filter_Command *copycmd;

        XDBG("Add extra copy %d -> %d: offset: %d,%d", copybuf->id, out->id, ox, oy);
        ENFN->context_color_set(ENC, drawctx, 255, 255, 255, 255);
        ENFN->context_render_op_set(ENC, drawctx, EVAS_RENDER_COPY);
        copycmd = evas_filter_command_blend_add(ctx, drawctx,
                                                copybuf->id, out->id, ox, oy,
                                                EVAS_FILTER_FILL_MODE_NONE,
                                                alphaonly);
        ENFN->context_color_set(ENC, drawctx, R, G, B, A);
        ENFN->context_render_op_set(ENC, drawctx, render_op);
        if (!copycmd) goto fail;
        ox = oy = 0;
     }

   out->dirty = EINA_TRUE;
   _filter_buffer_unlock_all(ctx);
   return cmd;

fail:
   ERR("Failed to add blur");
   _filter_buffer_unlock_all(ctx);
   return NULL;
}

/**
 * @brief Adds a blend (or copy) command to the filter chain.
 *
 * This command blends (or copies) the input buffer onto the output buffer.
 * It respects the color multiplier, render operation (copy or blend), and
 * clip region from the provided draw context.
 *
 * @param ctx The filter context.
 * @param drawctx The Evas draw context.
 * @param inbuf The ID of the input (source) buffer.
 * @param outbuf The ID of the output (destination) buffer.
 * @param ox Horizontal offset for drawing the input onto the output.
 * @param oy Vertical offset for drawing the input onto the output.
 * @param fillmode Fill mode if the input and output sizes differ (e.g., stretch, tile).
 *                 See Evas_Filter_Fill_Mode.
 * @param alphaonly EINA_TRUE to operate on the alpha channel only (if supported by operation).
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure or if inbuf equals outbuf.
 *         Example:
 *         // Blend buffer_A onto buffer_B at offset (10,10)
 *         evas_filter_command_blend_add(filter_ctx, evas_draw_context, buffer_A_id, buffer_B_id,
 *                                       10, 10, EVAS_FILTER_FILL_MODE_NONE, EINA_FALSE);
 */
Evas_Filter_Command *
evas_filter_command_blend_add(Evas_Filter_Context *ctx, void *drawctx,
                              int inbuf, int outbuf, int ox, int oy,
                              Evas_Filter_Fill_Mode fillmode,
                              Eina_Bool alphaonly)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out;
   Eina_Bool copy;
   int R, G, B, A;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   if (inbuf == outbuf)
     {
        XDBG("Skipping NOP blend operation %d --> %d", inbuf, outbuf);
        return NULL;
     }

   in = _filter_buffer_get(ctx, inbuf);
   if (!in)
     {
        ERR("Buffer %d does not exist [input].", inbuf);
        return NULL;
     }

   out = _filter_buffer_get(ctx, outbuf);
   if (!out)
     {
        ERR("Buffer %d does not exist [output].", outbuf);
        return NULL;
     }

   cmd = _command_new(ctx, EVAS_FILTER_MODE_BLEND, in, NULL, out);
   if (!cmd) return NULL;

   if (ENFN->context_render_op_get(ENC, drawctx) == EVAS_RENDER_COPY)
     copy = EINA_TRUE;
   else
     copy = EINA_FALSE;

   ENFN->context_color_get(ENC, drawctx, &R, &G, &B, &A);
   DRAW_COLOR_SET(R, G, B, A);
   DRAW_FILL_SET(fillmode);
   cmd->draw.ox = ox;
   cmd->draw.oy = oy;
   cmd->draw.rop = copy ? EFL_GFX_RENDER_OP_COPY : EFL_GFX_RENDER_OP_BLEND;
   cmd->draw.alphaonly = alphaonly;
   cmd->draw.clip_use =
         !!ENFN->context_clip_get(ENC, drawctx,
                                  &cmd->draw.clip.x, &cmd->draw.clip.y,
                                  &cmd->draw.clip.w, &cmd->draw.clip.h);

   XDBG("Add %s %d -> %d: offset %d,%d, color: %d,%d,%d,%d",
        copy ? "copy" : "blend", in->id, out->id, ox, oy, R, G, B, A);
   if (cmd->draw.clip_use)
     XDBG("Draw clip: %d,%d,%d,%d", cmd->draw.clip.x, cmd->draw.clip.y,
         cmd->draw.clip.w, cmd->draw.clip.h);

   out->dirty = EINA_TRUE;
   return cmd;
}

/**
 * @brief Adds a "grow" or "shrink" effect command sequence to the filter chain.
 *
 * This effect expands or contracts the opaque regions of an image.
 * It's implemented as a sequence of:
 * 1. Blur: To soften the edges. The blur radius is based on the `radius` parameter.
 * 2. Curve (Threshold): To make the blurred areas opaque (for grow) or transparent (for shrink).
 * 3. Blend: To combine the result with the output buffer if necessary.
 *
 * A positive `radius` results in a grow effect, while a negative `radius`
 * results in a shrink effect.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context.
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @param radius The radius of the grow/shrink effect. Positive for grow, negative for shrink.
 *               Example: `radius = 5` grows by 5 pixels. `radius = -3` shrinks by 3 pixels.
 * @param smooth EINA_TRUE for a smoother transition at the edges, EINA_FALSE for a hard edge.
 * @param alphaonly EINA_TRUE to operate on the alpha channel only.
 * @return A pointer to the first command in the sequence (the blur command), or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_grow_add(Evas_Filter_Context *ctx, void *draw_context,
                             int inbuf, int outbuf, int radius, Eina_Bool smooth,
                             Eina_Bool alphaonly)
{
   Evas_Filter_Command *blurcmd = NULL, *threshcmd = NULL, *blendcmd;
   Evas_Filter_Buffer *tmp, *in, *out;
   int diam = abs(radius) * 2 + 1;
   DATA8 curve[256] = {0};
   int tmin = 0, growbuf;

   EINA_SAFETY_ON_NULL_GOTO(ctx, fail);

   if (!radius)
     {
        XDBG("Changing 0px grow into simple blend");
        return evas_filter_command_blend_add(ctx, draw_context, inbuf, outbuf, 0, 0,
                                             EVAS_FILTER_FILL_MODE_NONE, alphaonly);
     }

   in = _filter_buffer_get(ctx, inbuf);
   EINA_SAFETY_ON_NULL_GOTO(in, fail);

   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_NULL_GOTO(out, fail);

   if ((inbuf != outbuf) && out->dirty)
     {
        tmp = evas_filter_temporary_buffer_get(ctx, in->w, in->h, in->alpha_only, 1);
        EINA_SAFETY_ON_NULL_GOTO(tmp, fail);
        growbuf = tmp->id;
     }
   else
     growbuf = outbuf;

   blurcmd = evas_filter_command_blur_add(ctx, draw_context, inbuf, growbuf,
                                          EVAS_FILTER_BLUR_DEFAULT,
                                          abs(radius), abs(radius), 0, 0, 0,
                                          alphaonly);
   EINA_SAFETY_ON_NULL_GOTO(blurcmd, fail);

   if (diam > 255) diam = 255;
   if (radius > 0)
     tmin = 255 / diam;
   else if (radius < 0)
     tmin = 256 - (255 / diam);

   if (!smooth)
     memset(curve + tmin, 255, 256 - tmin);
   else
     {
        int k, start, end, range;

        // This is pretty experimental.
        range = MAX(2, 12 - radius);
        start = ((tmin > range) ? (tmin - range) : 0);
        end = ((tmin < (256 - range)) ? (tmin + range) : 256);

        for (k = start; k < end; k++)
          curve[k] = ((k - start) * 255) / (end - start);
        if (end < 256)
          memset(curve + end, 255, 256 - end);
     }

   /* Use a temp buffer here. Becuase curve_add is using a temp buffer as well
      if inbuf and outbuf are same and doing blend_add. Then grow_add will do
      blend_add twice. Using a temp buffer will save a calling blend_add */
   tmp = evas_filter_temporary_buffer_get(ctx, in->w, in->h, in->alpha_only, 1);
   EINA_SAFETY_ON_NULL_GOTO(tmp, fail);

   threshcmd = evas_filter_command_curve_add(ctx, draw_context, growbuf, tmp->id,
                                             curve, EVAS_FILTER_CHANNEL_ALPHA);
   EINA_SAFETY_ON_NULL_GOTO(threshcmd, fail);

   blendcmd = evas_filter_command_blend_add(ctx, draw_context, tmp->id,
                                            outbuf, 0, 0,
                                            EVAS_FILTER_FILL_MODE_NONE,
                                            alphaonly);
   EINA_SAFETY_ON_NULL_GOTO(blendcmd, fail);

   return blurcmd;

fail:
   ERR("Failed to add grow");
   if (threshcmd) _command_del(ctx, threshcmd);
   if (blurcmd) _command_del(ctx, blurcmd);
   return NULL;
}

/**
 * @brief Adds a color curve adjustment command to the filter chain.
 *
 * This command applies a transfer curve to one or more color channels of the input buffer.
 * The `curve` parameter is an array of 256 values, where `curve[input_value]`
 * defines the `output_value`.
 * If input and output buffers are the same, a temporary buffer is used internally.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (used if a blend is needed for in-place operation).
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @param curve A pointer to an array of 256 DATA8 values defining the curve.
 *              Example for inverting alpha:
 *              DATA8 invert_alpha_curve[256];
 *              for (int i = 0; i < 256; ++i) invert_alpha_curve[i] = 255 - i;
 * @param channel The channel(s) to apply the curve to (e.g., EVAS_FILTER_CHANNEL_ALPHA,
 *                EVAS_FILTER_CHANNEL_RGB, EVAS_FILTER_CHANNEL_LUMINANCE).
 * @return A pointer to the newly created Evas_Filter_Command (or the blend command if temp buffer used),
 *         or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_curve_add(Evas_Filter_Context *ctx,
                              void *draw_context EINA_UNUSED,
                              int inbuf, int outbuf, DATA8 *curve,
                              Evas_Filter_Channel channel)
{
   Evas_Filter_Command *cmd, *blendcmd;
   Evas_Filter_Buffer *in, *out, *tmp = NULL, *curve_out;
   DATA8 *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(curve, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   if (in->alpha_only != out->alpha_only)
     WRN("Incompatible formats for color curves, implicit conversion will be "
         "slow and may not produce the desired output.");

   if (in == out)
     {
        tmp = evas_filter_temporary_buffer_get(ctx, in->w, in->h, in->alpha_only, 1);
        if (!tmp) return NULL;
        curve_out = tmp;
     }
   else curve_out = out;

   XDBG("Add curve %d -> %d", in->id, curve_out->id);

   copy = malloc(256 * sizeof(DATA8));
   if (!copy) return NULL;

   cmd = _command_new(ctx, EVAS_FILTER_MODE_CURVE, in, NULL, curve_out);
   if (!cmd)
     {
        _free(copy);
        return NULL;
     }

   memcpy(copy, curve, 256 * sizeof(DATA8));
   cmd->curve.data = copy;
   if (cmd->input->alpha_only)
     cmd->curve.channel = EVAS_FILTER_CHANNEL_ALPHA;
   else
     cmd->curve.channel = channel;

   if (tmp)
     {
        blendcmd = evas_filter_command_blend_add(ctx, draw_context, curve_out->id,
                                                out->id, 0, 0,
                                                EVAS_FILTER_FILL_MODE_NONE,
                                                out->alpha_only);
        if (!blendcmd)
          {
             _command_del(ctx, cmd);
             return NULL;
          }
     }

   return cmd;
}

/**
 * @brief Adds a displacement map command to the filter chain.
 *
 * This command displaces pixels in the input buffer based on the color values
 * in a displacement map buffer. The red channel of the map typically controls
 * X displacement, and the green channel controls Y displacement.
 * If input and output buffers are the same, a temporary buffer is used.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (used for render op and if a blend is needed).
 * @param inbuf The ID of the input buffer (the image to be displaced).
 * @param outbuf The ID of the output buffer.
 * @param dispbuf The ID of the displacement map buffer. This buffer's R/G channels
 *                are typically used to control X/Y displacement.
 * @param flags Flags controlling the displacement behavior (e.g., EVAS_FILTER_DISPLACE_NEAREST).
 *              See Evas_Filter_Displacement_Flags.
 * @param intensity A factor scaling the displacement effect.
 * @param fillmode Fill mode for pixels displaced outside the original bounds.
 *                 See Evas_Filter_Fill_Mode.
 * @return A pointer to the newly created Evas_Filter_Command (or the blend command if temp buffer used),
 *         or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_displacement_map_add(Evas_Filter_Context *ctx,
                                         void *draw_context EINA_UNUSED,
                                         int inbuf, int outbuf, int dispbuf,
                                         Evas_Filter_Displacement_Flags flags,
                                         int intensity,
                                         Evas_Filter_Fill_Mode fillmode)
{
   Evas_Filter_Buffer *in, *out, *map, *tmp = NULL, *disp_out;
   Evas_Filter_Command *cmd = NULL;
   Eina_Bool alphaonly = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(intensity >= 0, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   out = _filter_buffer_get(ctx, outbuf);
   map = _filter_buffer_get(ctx, dispbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   if (in->alpha_only != out->alpha_only)
     DBG("Different color formats, implicit conversion may be slow");

   if (map->alpha_only)
     {
        WRN("Displacement map is not an RGBA buffer, X and Y axes will be "
            "displaced together.");
     }

   if (in == out)
     {
        tmp = evas_filter_temporary_buffer_get(ctx, in->w, in->h, in->alpha_only, 1);
        if (!tmp) return NULL;
        disp_out = tmp;
     }
   else disp_out = out;

   cmd = _command_new(ctx, EVAS_FILTER_MODE_DISPLACE, in, map, disp_out);
   if (!cmd) goto fail;

   DRAW_FILL_SET(fillmode);
   cmd->displacement.flags = flags & EVAS_FILTER_DISPLACE_BITMASK;
   cmd->displacement.intensity = intensity;
   cmd->draw.rop = _evas_to_gfx_render_op(ENFN->context_render_op_get(ENC, draw_context));

   if (tmp)
     {
        Evas_Filter_Command *fillcmd;

        fillcmd = evas_filter_command_blend_add(ctx, draw_context, disp_out->id,
                                                out->id, 0, 0,
                                                EVAS_FILTER_FILL_MODE_NONE,
                                                alphaonly);
        if (!fillcmd) goto fail;
     }

   _filter_buffer_unlock_all(ctx);
   return cmd;

fail:
   _filter_buffer_unlock_all(ctx);
   _command_del(ctx, cmd);
   return NULL;
}

/**
 * @brief Adds a mask command to the filter chain.
 *
 * This command combines the input buffer with the output buffer, using the
 * mask buffer to control the blending. Typically, the alpha channel of the
 * mask buffer determines the opacity of the input buffer when applied to the output.
 * The operation is similar to `output = output * (1-mask_alpha) + input * mask_alpha`.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (used for render op and color multiplier).
 * @param inbuf The ID of the input (source) buffer.
 * @param maskbuf The ID of the mask buffer.
 * @param outbuf The ID of the output (destination) buffer.
 * @param fillmode Fill mode if buffer sizes differ. See Evas_Filter_Fill_Mode.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_mask_add(Evas_Filter_Context *ctx, void *draw_context,
                             int inbuf, int maskbuf, int outbuf,
                             Evas_Filter_Fill_Mode fillmode)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out, *mask;
   Efl_Gfx_Render_Op render_op;
   int R, G, B, A;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   render_op = _evas_to_gfx_render_op(ENFN->context_render_op_get(ENC, draw_context));
   ENFN->context_color_get(ENC, draw_context, &R, &G, &B, &A);

   in = _filter_buffer_get(ctx, inbuf);
   out = _filter_buffer_get(ctx, outbuf);
   mask = _filter_buffer_get(ctx, maskbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(mask, NULL);

   cmd = _command_new(ctx, EVAS_FILTER_MODE_MASK, in, mask, out);
   if (!cmd) return NULL;

   cmd->draw.rop = render_op;
   DRAW_COLOR_SET(R, G, B, A);
   DRAW_FILL_SET(fillmode);

   return cmd;
}

/**
 * @brief Adds a bump map (or emboss) lighting effect command to the filter chain.
 *
 * This command applies a lighting effect to the input buffer using the bump (height)
 * map buffer to simulate surface relief. The lighting is defined by light source
 * angles, elevation, and material colors.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (currently unused by this command).
 * @param inbuf The ID of the input buffer (texture to apply lighting to, typically alpha).
 * @param bumpbuf The ID of the bump map buffer (height map, typically alpha).
 * @param outbuf The ID of the output buffer.
 * @param xyangle Light source angle in the XY plane (degrees).
 * @param zangle Light source angle in the Z plane (degrees, elevation from surface).
 * @param elevation Surface elevation factor.
 * @param sf Specular factor.
 * @param black Color for darkest shadowed areas (ARGB).
 * @param color Base color of the material (ARGB).
 * @param white Color for brightest highlighted areas (ARGB).
 * @param flags Flags for bump mapping (e.g., EVAS_FILTER_BUMP_COMPENSATE).
 *              See Evas_Filter_Bump_Flags.
 * @param fillmode Fill mode if buffer sizes differ. See Evas_Filter_Fill_Mode.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 *         Note: Currently, this function expects `inbuf != outbuf` and `mapbuf != outbuf`.
 */
Evas_Filter_Command *
evas_filter_command_bump_map_add(Evas_Filter_Context *ctx,
                                 void *draw_context EINA_UNUSED,
                                 int inbuf, int bumpbuf, int outbuf,
                                 float xyangle, float zangle, float elevation,
                                 float sf,
                                 DATA32 black, DATA32 color, DATA32 white,
                                 Evas_Filter_Bump_Flags flags,
                                 Evas_Filter_Fill_Mode fillmode)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out, *map;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   out = _filter_buffer_get(ctx, outbuf);
   map = _filter_buffer_get(ctx, bumpbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   if (!map->alpha_only)
     DBG("Bump map is not an Alpha buffer, implicit conversion may be slow");

   // FIXME: Boo!
   if (!in->alpha_only)
     WRN("RGBA bump map support is not implemented! This will trigger conversion.");

   // FIXME: Must ensure in != out
   EINA_SAFETY_ON_FALSE_RETURN_VAL(in != out, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(map != out, NULL);

   cmd = _command_new(ctx, EVAS_FILTER_MODE_BUMP, in, map, out);
   if (!cmd) return NULL;

   DRAW_FILL_SET(fillmode);
   cmd->bump.xyangle = xyangle;
   cmd->bump.zangle = zangle;
   cmd->bump.specular_factor = sf;
   cmd->bump.dark = black;
   cmd->bump.color = color;
   cmd->bump.white = white;
   cmd->bump.elevation = elevation;
   cmd->bump.compensate = !!(flags & EVAS_FILTER_BUMP_COMPENSATE);

   return cmd;
}

/**
 * @brief Adds a geometric transformation command to the filter chain.
 *
 * This command can perform operations like flipping the input buffer horizontally
 * or vertically. The result is placed in the output buffer with an optional offset.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (currently unused by this command).
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @param flags Transformation flags (e.g., EVAS_FILTER_TRANSFORM_FLIP_X, EVAS_FILTER_TRANSFORM_FLIP_Y).
 *              See Evas_Filter_Transform_Flags.
 * @param ox Horizontal offset for placing the transformed result into the output buffer.
 * @param oy Vertical offset for placing the transformed result into the output buffer.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_transform_add(Evas_Filter_Context *ctx,
                                  void *draw_context EINA_UNUSED,
                                  int inbuf, int outbuf,
                                  Evas_Filter_Transform_Flags flags,
                                  int ox, int oy)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   cmd = _command_new(ctx, EVAS_FILTER_MODE_TRANSFORM, in, NULL, out);
   if (!cmd) return NULL;

   DRAW_COLOR_SET(255, 255, 255, 255);
   cmd->transform.flags = flags;
   cmd->draw.ox = ox;
   cmd->draw.oy = oy;

   if (in->alpha_only == out->alpha_only)
     {
        DBG("Incompatible buffer formats, will trigger implicit conversion.");
        cmd->draw.rop = EFL_GFX_RENDER_OP_COPY;
     }
   else
     cmd->draw.rop = EFL_GFX_RENDER_OP_BLEND;

   return cmd;
}

/**
 * @brief Adds a grayscale conversion command to the filter chain.
 *
 * This command converts the colors in the input buffer to shades of gray
 * and stores the result in the output buffer.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (currently unused by this command).
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_grayscale_add(Evas_Filter_Context *ctx,
                                  void *draw_context EINA_UNUSED,
                                  int inbuf, int outbuf)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);

   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   cmd = _command_new(ctx, EVAS_FILTER_MODE_GRAYSCALE, in, NULL, out);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   return cmd;
}

/**
 * @brief Adds an inverse color (negative) command to the filter chain.
 *
 * This command inverts the colors (RGB channels) of the input buffer
 * and stores the result in the output buffer. The alpha channel is typically unchanged.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context (currently unused by this command).
 * @param inbuf The ID of the input buffer.
 * @param outbuf The ID of the output buffer.
 * @return A pointer to the newly created Evas_Filter_Command, or NULL on failure.
 */
Evas_Filter_Command *
evas_filter_command_inverse_color_add(Evas_Filter_Context *ctx,
                                     void *draw_context EINA_UNUSED,
                                     int inbuf, int outbuf)
{
   Evas_Filter_Command *cmd;
   Evas_Filter_Buffer *in, *out;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);

   in = _filter_buffer_get(ctx, inbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(in, NULL);

   out = _filter_buffer_get(ctx, outbuf);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   cmd = _command_new(ctx, EVAS_FILTER_MODE_INVERSE_COLOR, in, NULL, out);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out, NULL);

   return cmd;
}

/**
 * @brief Sets the region of the target surface that is known to be obscured by other content.
 *
 * This information can potentially be used by the filter engine to optimize rendering
 * by skipping processing for obscured parts. The `_filter_obscured_region_calc` function
 * later adjusts this "real" obscured region by the filter's calculated padding
 * to determine the "effective" obscured region.
 *
 * @param ctx The filter context.
 * @param rect The rectangle defining the obscured region in target coordinates.
 *             Example: `Eina_Rectangle obscured_rect = { .x = 10, .y = 10, .w = 50, .h = 50 };`
 */
void
evas_filter_context_obscured_region_set(Evas_Filter_Context *ctx, Eina_Rectangle rect)
{
   ctx->obscured.real = rect;
}

/**
 * @brief Sets the final rendering target for the filter chain.
 *
 * This specifies where the content of the EVAS_FILTER_BUFFER_OUTPUT_ID buffer
 * will be drawn after all filter commands have been processed. It includes
 * the target surface, position, clipping, color modulation, render operation,
 * and an optional RGBA_Map for complex drawing.
 *
 * @param ctx The filter context.
 * @param draw_context The Evas draw context from which clip, color, and render op are taken.
 * @param surface The engine-specific target surface to draw onto.
 * @param x The X coordinate on the target surface.
 * @param y The Y coordinate on the target surface.
 * @param map An optional RGBA_Map for drawing (e.g., for perspective transforms). If NULL, simple image draw is used.
 *            The structure of RGBA_Map includes a count of points and an array of RGBA_Map_Point.
 *            Each RGBA_Map_Point has source (sx, sy, sz) and destination (dx, dy, dz) coordinates,
 *            and a color. Example:
 *            RGBA_Map_Point points[4] = {
 *              { {0,0,0}, {10,10,0}, {255,255,255,255} }, // Top-left
 *              { {W,0,0}, {W+10,10,0}, {255,255,255,255} }, // Top-right
 *              { {W,H,0}, {W+10,H+10,0}, {255,255,255,255} }, // Bottom-right
 *              { {0,H,0}, {10,H+10,0}, {255,255,255,255} }  // Bottom-left
 *            };
 *            RGBA_Map my_map;
 *            my_map.count = 4;
 *            my_map.points = points; // In reality, this needs to be a flexible array member or allocated together.
 *                                    // The code actually copies this structure.
 * @return EINA_TRUE on success, EINA_FALSE if ctx is NULL.
 */
Eina_Bool
evas_filter_target_set(Evas_Filter_Context *ctx, void *draw_context,
                       void *surface, int x, int y, const RGBA_Map *map)
{
   void *mask = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_FALSE);

   ctx->target.surface = ENFN->image_ref(ENC, surface);
   ctx->target.x = x;
   ctx->target.y = y;
   ctx->target.clip_use = ENFN->context_clip_get
         (ENC, draw_context, &ctx->target.cx, &ctx->target.cy,
          &ctx->target.cw, &ctx->target.ch);
   ctx->target.color_use = ENFN->context_multiplier_get
         (ENC, draw_context, &ctx->target.r, &ctx->target.g,
          &ctx->target.b, &ctx->target.a);
   if (ctx->target.r == 255 && ctx->target.g == 255 &&
       ctx->target.b == 255 && ctx->target.a == 255)
     ctx->target.color_use = EINA_FALSE;
   ctx->target.rop = ENFN->context_render_op_get(ENC, draw_context);

   _free(ctx->target.map);
   if (!map) ctx->target.map = NULL;
   else
     {
        size_t len = sizeof(RGBA_Map) + sizeof(RGBA_Map_Point) * (map->count - 1);
        ctx->target.map = malloc(len);
        memcpy(ctx->target.map, map, len);
     }

   ENFN->context_clip_image_get
      (ENC, draw_context, &mask, &ctx->target.mask_x, &ctx->target.mask_y);
   if (ctx->target.mask)
     ctx->evas->engine.func->image_free(_evas_engine_context(ctx->evas), ctx->target.mask);
   ctx->target.mask = mask; // FIXME: why no ref???

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Renders the final output buffer (EVAS_FILTER_BUFFER_OUTPUT_ID) to the target surface.
 *
 * This function is called after all filter commands in the chain have been executed.
 * It retrieves the image from the output buffer and draws it to the target surface
 * specified by `evas_filter_target_set`, applying clipping, color modulation,
 * render operation, and RGBA map if configured.
 *
 * @param engine The graphics engine context.
 * @param output The rendering output context (often same as engine for software).
 * @param ctx The filter context containing target information and the output buffer.
 * @return EINA_TRUE on successful rendering, EINA_FALSE on failure.
 */
static Eina_Bool
_filter_target_render(void *engine, void *output, Evas_Filter_Context *ctx)
{
   Evas_Filter_Buffer *src;
   void *drawctx, *image = NULL, *surface;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx->target.surface, EINA_FALSE);

   drawctx = ENFN->context_new(engine);
   surface = ctx->target.surface;

   src = _filter_buffer_get(ctx, EVAS_FILTER_BUFFER_OUTPUT_ID);
   EINA_SAFETY_ON_NULL_RETURN_VAL(src, EINA_FALSE);

   image = evas_ector_buffer_drawable_image_get(src->buffer);
   EINA_SAFETY_ON_NULL_GOTO(image, fail);

   // FIXME: Use ector buffer RENDERER here

   if (ctx->target.clip_use)
     {
        ENFN->context_clip_set(engine, drawctx, ctx->target.cx, ctx->target.cy,
                               ctx->target.cw, ctx->target.ch);
     }

   if (ctx->target.color_use)
     {
        ENFN->context_multiplier_set(engine, drawctx,
                                     ctx->target.r, ctx->target.g,
                                     ctx->target.b, ctx->target.a);
     }

   if (ctx->target.mask)
     {
        ENFN->context_clip_image_set(engine, drawctx, ctx->target.mask,
                                     ctx->target.mask_x, ctx->target.mask_y,
                                     ctx->evas, EINA_FALSE);
     }

   ENFN->context_render_op_set(engine, drawctx, ctx->target.rop);
   if (ctx->target.map)
     {
        ENFN->image_map_draw(engine, output, drawctx, surface, image,
                             ctx->target.map, EINA_TRUE, 0, EINA_FALSE);
     }
   else
     {
        ENFN->image_draw(engine, output, drawctx, surface, image,
                         0, 0, src->w, src->h,
                         ctx->target.x, ctx->target.y, src->w, src->h,
                         EINA_TRUE, EINA_FALSE);
     }

   ENFN->context_free(engine, drawctx);
   evas_ector_buffer_engine_image_release(src->buffer, image);

   ENFN->image_free(engine, surface);
   ctx->target.surface = NULL;

   return EINA_TRUE;

fail:
   ENFN->image_free(engine, surface);
   ctx->target.surface = NULL;

   ERR("Failed to render filter to target canvas!");
   return EINA_FALSE;
}

/**
 * @brief Draws text into a specified filter buffer.
 *
 * This function is used to render text directly into a filter buffer, which can
 * then be used as an input for subsequent filter operations (e.g., blurring text).
 * It handles asynchronous glyph unreferencing if `do_async` is true.
 *
 * @param ctx The filter context.
 * @param engine The graphics engine context.
 * @param output The rendering output context.
 * @param draw_context The Evas draw context for the text rendering.
 * @param bufid The ID of the target filter buffer to draw the text into.
 * @param font The Evas_Font_Set to use for rendering.
 * @param x The X coordinate within the buffer to start drawing the text.
 * @param y The Y coordinate within the buffer to start drawing the text.
 * @param text_props The Evas_Text_Props structure containing the text and its properties.
 *                   Example (simplified):
 *                   Evas_Text_Props props;
 *                   props.text = eina_stringshare_add("Hello");
 *                   props.glyphs = evas_common_text_props_to_glyphs(&props, font, ...); // Simplified
 * @param do_async EINA_TRUE if the font drawing operation might be asynchronous.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., buffer not found or backing get fails).
 */
Eina_Bool
evas_filter_font_draw(Evas_Filter_Context *ctx,
                      void *engine, void *output, void *draw_context, int bufid,
                      Evas_Font_Set *font, int x, int y,
                      Evas_Text_Props *text_props, Eina_Bool do_async)
{
   Eina_Bool async_unref;
   Evas_Filter_Buffer *fb;
   void *surface = NULL;

   fb = _filter_buffer_get(ctx, bufid);
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, EINA_FALSE);

   surface = evas_filter_buffer_backing_get(ctx, bufid, EINA_TRUE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, EINA_FALSE);

   // Copied from evas_font_draw_async_check
   async_unref = ENFN->font_draw(engine, output, draw_context, surface,
                                 font, x, y, fb->w, fb->h, fb->w, fb->h,
                                 text_props, do_async);
   if (do_async && async_unref)
     {
        evas_common_font_glyphs_ref(text_props->glyphs);
        evas_unref_queue_glyph_put(ctx->evas, text_props->glyphs);
     }

   evas_ector_buffer_engine_image_release(fb->buffer, surface);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Clips a source rectangle against a destination rectangle with an offset.
 *
 * Given a source rectangle of size (sw, sh) at (0,0) in its own coordinates,
 * and a destination rectangle of size (dw, dh) at (0,0) in its own coordinates,
 * this function calculates the overlapping region when the source is placed
 * at an offset (ox, oy) relative to the destination.
 *
 * It outputs:
 * - (sx, sy): The top-left corner of the valid (clipped) region in the source's coordinates.
 * - (dx, dy): The top-left corner of the valid (clipped) region in the destination's coordinates.
 * - (cols, rows): The width and height of this valid (clipped) overlapping region.
 *
 * This is useful for determining how much of a source image can be drawn onto a
 * destination surface and at what corresponding coordinates.
 *
 * @param[out] sx Pointer to store the clipped source X coordinate.
 * @param[out] sy Pointer to store the clipped source Y coordinate.
 * @param sw Width of the source rectangle.
 * @param sh Height of the source rectangle.
 * @param ox X offset of the source rectangle relative to the destination.
 * @param oy Y offset of the source rectangle relative to the destination.
 * @param dw Width of the destination rectangle.
 * @param dh Height of the destination rectangle.
 * @param[out] dx Pointer to store the clipped destination X coordinate.
 * @param[out] dy Pointer to store the clipped destination Y coordinate.
 * @param[out] rows Pointer to store the height of the clipped region (number of rows).
 * @param[out] cols Pointer to store the width of the clipped region (number of columns).
 */
void
_clip_to_target(int *sx /* OUT */, int *sy /* OUT */, int sw, int sh,
                int ox, int oy, int dw, int dh,
                int *dx /* OUT */, int *dy /* OUT */,
                int *rows /* OUT */, int *cols /* OUT */)
{
   if (ox > 0)
     {
        (*sx) = 0;
        (*dx) = ox;
        (*cols) = sw;
        if (((*dx) + (*cols)) > (dw))
          (*cols) = dw - (*dx);
     }
   else if (ox < 0)
     {
        (*dx) = 0;
        (*sx) = (-ox);
        (*cols) = sw - (*sx);
        if ((*cols) > dw) (*cols) = dw;
     }
   else
     {
        (*sx) = 0;
        (*dx) = 0;
        (*cols) = sw;
        if ((*cols) > dw) (*cols) = dw;
     }

   if (oy > 0)
     {
        (*sy) = 0;
        (*dy) = oy;
        (*rows) = sh;
        if (((*dy) + (*rows)) > (dh))
          (*rows) = dh - (*dy);
     }
   else if (oy < 0)
     {
        (*dy) = 0;
        (*sy) = (-oy);
        (*rows) = sh - (*sy);
        if ((*rows) > dh) (*rows) = dh;
     }
   else
     {
        (*sy) = 0;
        (*dy) = 0;
        (*rows) = sh;
        if ((*rows) > dh) (*rows) = dh;
     }
   if ((*cols) < 0) *cols = 0;
   if ((*rows) < 0) *rows = 0;
}

#ifdef FILTERS_DEBUG
/**
 * @internal
 * @brief Returns a string representation of an Evas_Filter_Mode enum value.
 *        Used for debugging purposes.
 * @param mode The filter mode enum value.
 * @return A string name for the filter mode, or "INVALID" if not recognized.
 */
static const char *
_filter_name_get(int mode)
{
#define FNAME(a) case EVAS_FILTER_MODE_ ## a: return "EVAS_FILTER_MODE_" #a
   switch (mode)
     {
      FNAME(SKIP);
      FNAME(BLEND);
      FNAME(BLUR);
      FNAME(CURVE);
      FNAME(DISPLACE);
      FNAME(MASK);
      FNAME(BUMP);
      FNAME(FILL);
      default: return "INVALID";
     }
#undef FNAME
}
#endif

/**
 * @internal
 * @brief Executes a single filter command.
 *
 * This function checks for basic validity (e.g., non-empty input/output buffers
 * for most operations). It then queries the engine for support of the given
 * filter command and, if supported, calls the engine's processing function.
 *
 * @param cmd The Evas_Filter_Command to run.
 * @return EINA_TRUE if the command was run successfully (or skipped appropriately),
 *         EINA_FALSE on failure (e.g., unsupported filter, invalid buffer sizes,
 *         or engine processing error).
 */
static Eina_Bool
_filter_command_run(Evas_Filter_Command *cmd)
{
   Evas_Filter_Support support = EVAS_FILTER_SUPPORT_NONE;

   if (cmd->mode == EVAS_FILTER_MODE_SKIP)
     return EINA_TRUE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, EINA_FALSE);

#ifdef FILTERS_DEBUG
   XDBG("Command %d (%s): %d [%d] --> %d",
       cmd->id, _filter_name_get(cmd->mode),
       cmd->input->id, cmd->mask ? cmd->mask->id : 0, cmd->output->id);
#endif

   if (!cmd->input->w && !cmd->input->h
       && (cmd->mode != EVAS_FILTER_MODE_FILL))
     {
        XDBG("Skipping processing of empty input buffer (size 0x0)");
        return EINA_TRUE;
     }

   if ((cmd->output->w <= 0) || (cmd->output->h <= 0))
     {
        ERR("Output size invalid: %dx%d", cmd->output->w, cmd->output->h);
        return EINA_FALSE;
     }

   support = cmd->ENFN->gfx_filter_supports(CMD_ENC, cmd);
   if (support == EVAS_FILTER_SUPPORT_NONE)
     {
        ERR("No function to process this filter (mode %d)", cmd->mode);
        return EINA_FALSE;
     }

   return cmd->ENFN->gfx_filter_process(CMD_ENC, cmd);
}

/**
 * @internal
 * @brief Executes the entire chain of filter commands and renders the result to the target.
 *
 * This function iterates through all commands in the Evas_Filter_Context,
 * executing each one using `_filter_command_run`. If all commands succeed,
 * it then calls `_filter_target_render` to draw the final output.
 * Finally, it invokes the post-run callback.
 *
 * @param engine The graphics engine context.
 * @param output The rendering output context.
 * @param ctx The filter context containing the command chain and target information.
 * @return EINA_TRUE if the entire chain and target rendering were successful,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_filter_chain_run(void *engine, void *output, Evas_Filter_Context *ctx)
{
   Evas_Filter_Command *cmd;
   Eina_Bool ok = EINA_FALSE;

   DEBUG_TIME_BEGIN();

   EINA_INLIST_FOREACH(ctx->commands, cmd)
     {
        ok = _filter_command_run(cmd);
        if (!ok)
          {
             ERR("Filter processing failed!");
             goto end;
          }
     }

   ok = _filter_target_render(engine, output, ctx);

end:
   ctx->running = EINA_FALSE;
   DEBUG_TIME_END();

   ctx->post_run.cb(ctx, ctx->post_run.data, ok);
   return ok;
}

typedef struct _Filter_Thread_Data Filter_Thread_Data;
struct _Filter_Thread_Data
{
   void *engine;
   void *output;
   Evas_Filter_Context *ctx;
};

static void
_filter_thread_run_cb(void *data)
{
   Filter_Thread_Data *ftd = data;

   _filter_chain_run(ftd->engine, ftd->output, ftd->ctx);
   _free(ftd);
}

/**
 * @internal
 * @brief Calculates the effective obscured region based on the "real" obscured region and filter padding.
 *
 * The "real" obscured region is set by `evas_filter_context_obscured_region_set`.
 * This function adjusts that rectangle by the calculated padding values of the filter
 * (how much the filter might expand beyond its nominal bounds). The result is stored
 * in `ctx->obscured.effective`. This effective region can be used by rendering
 * engines to optimize away drawing of fully obscured parts of the filtered output.
 *
 * @param ctx The filter context, containing `obscured.real` and `pad.calculated`.
 */
static void
_filter_obscured_region_calc(Evas_Filter_Context *ctx)
{
   Eina_Rectangle rect = ctx->obscured.real;

   // left
   if (rect.x > 0)
     {
        rect.x += ctx->pad.calculated.l;
        rect.w -= ctx->pad.calculated.l;
     }
   else
     {
        rect.w -= (-rect.x);
        rect.x = 0;
     }
   if (rect.w < 0) rect.w = 0;

   // right
   if ((rect.x + rect.w) <= ctx->w)
     rect.w -= ctx->pad.calculated.r;
   else
     rect.w = ctx->w - rect.x;

   // top
   if (rect.y > 0)
     {
        rect.y += ctx->pad.calculated.t;
        rect.h -= ctx->pad.calculated.t;
     }
   else
     {
        rect.h -= (-rect.y);
        rect.y = 0;
     }
   if (rect.h < 0) rect.h = 0;

   // bottom
   if ((rect.y + rect.h) <= ctx->h)
     rect.h -= ctx->pad.calculated.b;
   else
     rect.h = ctx->h - rect.y;

   if ((rect.w <= 0) || (rect.h <= 0))
     memset(&rect, 0, sizeof(rect));

   ctx->obscured.effective = rect;
}

/**
 * @brief Runs the configured filter chain in the given context.
 *
 * This function initiates the execution of all filter commands.
 * It first increments the context's reference count, calculates the effective
 * obscured region, and then either runs the filter chain synchronously via
 * `_filter_chain_run` or schedules it for asynchronous execution in a
 * separate thread if `ctx->async` is true.
 *
 * @param engine The graphics engine context.
 * @param output The rendering output context.
 * @param ctx The filter context to run.
 * @return EINA_TRUE if the filter chain was successfully started (or run synchronously).
 *         For asynchronous runs, this indicates successful queuing. The actual
 *         result of the async run is reported via the post-run callback.
 *         For synchronous runs, it returns the result of `_filter_chain_run`.
 */
Eina_Bool
evas_filter_context_run(void *engine, void *output, Evas_Filter_Context *ctx)
{
   evas_filter_context_ref(ctx);
   _filter_obscured_region_calc(ctx);

   ctx->run_count++;
   ctx->running = EINA_TRUE;
   if (ctx->async)
     {
        Filter_Thread_Data *ftd;

        ftd = calloc(1, sizeof(*ftd));
        ftd->engine = engine;
        ftd->output = output;
        ftd->ctx = ctx;

        evas_thread_queue_flush(_filter_thread_run_cb, ftd);
        return EINA_TRUE;
     }

   return _filter_chain_run(engine, output, ctx);
}


/* Logging */

static int init_cnt = 0;
int _evas_filter_log_dom = 0;

/**
 * @brief Initializes the Evas filter subsystem.
 *
 * This function sets up logging for filters and initializes any
 * associated components like mixins. It uses a static counter
 * to ensure initialization happens only once.
 */
void
evas_filter_init(void)
{
   if ((init_cnt++) > 0) return;
   _evas_filter_log_dom = eina_log_domain_register("evas_filter", EVAS_FILTER_LOG_COLOR);
   evas_filter_mixin_init();
}

/**
 * @brief Shuts down the Evas filter subsystem.
 *
 * This function cleans up resources used by the filter system,
 * including unregistering the log domain and shutting down
 * associated components. It uses a static counter to ensure
 * shutdown occurs only when the init count reaches zero.
 */
void
evas_filter_shutdown(void)
{
   if ((--init_cnt) > 0) return;
   evas_filter_parser_shutdown();
   evas_filter_mixin_shutdown();
   eina_log_domain_unregister(_evas_filter_log_dom);
   _evas_filter_log_dom = 0;
}
