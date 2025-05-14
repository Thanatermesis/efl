#include "evas_image_private.h"
#include "efl_canvas_proxy.eo.h"

#define MY_CLASS EFL_CANVAS_PROXY_CLASS

/**
 * @internal
 * @brief Sets the source object for a proxy image.
 *
 * This function establishes a link between a proxy image object and its source object.
 * The proxy image will then render the content of the source object.
 * It performs several checks to ensure the validity of both the proxy and source objects,
 * their layers, and prevents self-referencing or setting a source from a different Evas canvas.
 * If a previous source or image file was set, it cleans them up.
 *
 * @param eo_obj The proxy image object.
 * @param eo_src The Evas object to be used as the source. If NULL, the current source is unset.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool
_evas_image_proxy_source_set(Eo *eo_obj, Evas_Object *eo_src)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!obj)
     {
        WRN("Image source %p not valid", eo_obj);
        return EINA_FALSE;
     }
   if (obj->delete_me && eo_src)
     {
        WRN("Setting deleted object %p as image source %p", eo_src, eo_obj);
        return EINA_FALSE;
     }
   if (eo_src)
     {
        Evas_Object_Protected_Data *src = efl_data_scope_get(eo_src, EFL_CANVAS_OBJECT_CLASS);
        if (!src)
          {
             WRN("Setting object %p not valid", eo_src);
             return EINA_FALSE;
          }
        if (src->delete_me)
          {
             WRN("Setting object %p to deleted image source %p", eo_src, eo_obj);
             return EINA_FALSE;
          }
        if (!src->layer)
          {
             CRI("No evas surface associated with source object (%p)", eo_src);
             return EINA_FALSE;
          }
        if (!obj->layer)
          {
             CRI("No evas surface associated with destination object (%p)", eo_obj);
             return EINA_FALSE;
          }
        if ((obj->layer && src->layer) &&
            (obj->layer->evas != src->layer->evas))
          {
             CRI("Setting object %p from Evas (%p) from another Evas (%p)", eo_src, src->layer->evas, obj->layer->evas);
             return EINA_FALSE;
          }
        if (eo_src == eo_obj)
          {
             CRI("Setting object %p as a source for itself", obj);
             return EINA_FALSE;
          }
     }
   if (o->cur->source == eo_src) return EINA_TRUE;
   evas_object_async_block(obj);
   _evas_image_cleanup(eo_obj, obj, o);
   /* Kill the image if any */
   if (o->cur->f || o->cur->key)
     evas_object_image_mmap_set(eo_obj, NULL, NULL);

   if (eo_src) _evas_image_proxy_set(eo_obj, eo_src);
   else _evas_image_proxy_unset(eo_obj, obj, o);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Set EOLIAN wrapper.
 * @see _evas_image_proxy_source_set
 */
EOLIAN static Eina_Bool
_efl_canvas_proxy_source_set(Eo *eo_obj, void *_pd EINA_UNUSED, Evas_Object *eo_src)
{
   return _evas_image_proxy_source_set(eo_obj, eo_src);
}

/**
 * @internal
 * @brief Gets the source object of a proxy image.
 *
 * @param eo_obj The proxy image object.
 * @return The source Evas object, or @c NULL if no source is set or on error.
 */
Evas_Object *
_evas_image_proxy_source_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return o->cur->source;
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Get EOLIAN wrapper.
 * @see _evas_image_proxy_source_get
 */
EOLIAN static Evas_Object *
_efl_canvas_proxy_source_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_proxy_source_get(eo_obj);
}

/**
 * @internal
 * @brief Sets whether the proxy image should be clipped by its source's clipper.
 *
 * If @p source_clip is @c EINA_TRUE, the proxy image will be clipped by the
 * clipper object set on its source object.
 *
 * @param eo_obj The proxy image object.
 * @param source_clip @c EINA_TRUE to enable source clipping, @c EINA_FALSE otherwise.
 */
void
_evas_image_proxy_source_clip_set(Eo *eo_obj, Eina_Bool source_clip)
{
   Evas_Object_Protected_Data *src_obj;
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   source_clip = !!source_clip;
   if (o->proxy_src_clip == source_clip) return;
   evas_object_async_block(obj);
   o->proxy_src_clip = source_clip;

   if (!o->cur->source) return;

   src_obj = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
   evas_object_change(o->cur->source, src_obj);
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Clip_Set EOLIAN wrapper.
 * @see _evas_image_proxy_source_clip_set
 */
EOLIAN static void
_efl_canvas_proxy_source_clip_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Bool source_clip)
{
   return _evas_image_proxy_source_clip_set(eo_obj, source_clip);
}

/**
 * @internal
 * @brief Gets whether the proxy image is clipped by its source's clipper.
 *
 * @param eo_obj The proxy image object.
 * @return @c EINA_TRUE if source clipping is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_proxy_source_clip_get(const Eo *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return o->proxy_src_clip;
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Clip_Get EOLIAN wrapper.
 * @see _evas_image_proxy_source_clip_get
 */
EOLIAN static Eina_Bool
_efl_canvas_proxy_source_clip_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_proxy_source_clip_get(eo_obj);
}

/**
 * @internal
 * @brief Sets whether mouse events on the proxy object are passed to its source.
 *
 * If @p source_events is @c EINA_TRUE, mouse events occurring on the proxy
 * object will be propagated to its source object.
 *
 * @param eo_obj The proxy image object.
 * @param source_events @c EINA_TRUE to enable event propagation, @c EINA_FALSE otherwise.
 */
void
_evas_image_proxy_source_events_set(Eo *eo_obj, Eina_Bool source_events)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   source_events = !!source_events;
   if (obj->proxy->src_events == source_events) return;

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, obj->proxy, Evas_Object_Proxy_Data, proxy_write)
     proxy_write->src_events = source_events;
   EINA_COW_WRITE_END(evas_object_proxy_cow, obj->proxy, proxy_write);

   if (!o->cur->source) return;
   if ((obj->proxy->src_invisible) || (!source_events)) return;
   //FIXME: Feed mouse events here.
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Events_Set EOLIAN wrapper.
 * @see _evas_image_proxy_source_events_set
 */
EOLIAN static void
_efl_canvas_proxy_source_events_set(Eo *eo_obj, void *_pd EINA_UNUSED, Eina_Bool repeat)
{
   return _evas_image_proxy_source_events_set(eo_obj, repeat);
}

/**
 * @internal
 * @brief Gets whether mouse events on the proxy object are passed to its source.
 *
 * @param eo_obj The proxy image object.
 * @return @c EINA_TRUE if event propagation is enabled, @c EINA_FALSE otherwise.
 */
Eina_Bool
_evas_image_proxy_source_events_get(const Eo *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   return obj->proxy->src_events;
}

/**
 * @internal
 * @brief Efl_Canvas_Proxy_Source_Events_Get EOLIAN wrapper.
 * @see _evas_image_proxy_source_events_get
 */
EOLIAN static Eina_Bool
_efl_canvas_proxy_source_events_get(const Eo *eo_obj, void *_pd EINA_UNUSED)
{
   return _evas_image_proxy_source_events_get(eo_obj);
}

/**
 * @internal
 * @brief Gets the scaling factor between the proxy object and its source.
 *
 * This function calculates the ratio of the source object's dimensions to the
 * proxy object's dimensions.
 *
 * @param eo_obj The proxy image object.
 * @param[out] sw Pointer to a double to store the width scaling factor.
 *                Set to 1.0 if source is not set or proxy width is 0.
 * @param[out] sh Pointer to a double to store the height scaling factor.
 *                Set to 1.0 if source is not set or proxy height is 0.
 */
void
_evas_image_proxy_source_scale_get(const Eo *eo_obj, double *sw, double *sh)
{
   *sw = 1.0;
   *sh = 1.0;
   Evas_Object_Protected_Data *source = NULL;

   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   if (!o->cur->source) return;

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   source = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);

   if (obj->cur->geometry.w > 0)
     *sw = (double) source->cur->geometry.w / obj->cur->geometry.w;

   if (obj->cur->geometry.h > 0)
     *sh = (double) source->cur->geometry.h / obj->cur->geometry.h;
}

/**
 * @internal
 * @brief Retrieves the source object of an Evas image object, if it's acting as a proxy.
 *
 * This is a legacy accessor. For Eo-based objects, prefer _evas_image_proxy_source_get.
 *
 * @param eo_obj The Evas image object.
 * @return The source Evas object if @p eo_obj is a proxy and has a source, otherwise @c NULL.
 */
Evas_Object *
_evas_object_image_source_get(Evas_Object *eo_obj)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   return o->cur->source;
}

/**
 * @internal
 * @brief Unsets the source from a proxy object.
 *
 * This function cleans up the proxy relationship. It removes the proxy from its
 * source's list of proxies. If the source has no more proxies, its proxy surface
 * is freed. It also updates visibility and invalidates caches if necessary.
 * The proxy's internal source reference and definition map are cleared.
 *
 * @param proxy The proxy Evas_Object.
 * @param cur_proxy The protected data of the proxy object.
 * @param o The image data of the proxy object.
 */
void
_evas_image_proxy_unset(Evas_Object *proxy, Evas_Object_Protected_Data *cur_proxy, Evas_Image_Data *o)
{
   Evas_Object_Protected_Data *cur_source;

   if (!o->cur->source) return;

   cur_source = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, cur_source->proxy, Evas_Object_Proxy_Data, proxy_source_write)
     {
       proxy_source_write->proxies = eina_list_remove(proxy_source_write->proxies,
                                                      proxy);

       if (eina_list_count(proxy_source_write->proxies) == 0)
          {
             if (proxy_source_write->surface)
               cur_proxy->layer->evas->engine.func->image_free(_evas_engine_context(cur_proxy->layer->evas),
                                                               proxy_source_write->surface);
             proxy_source_write->surface = NULL;
             proxy_source_write->redraw = EINA_FALSE;
          }

       if (proxy_source_write->src_invisible)
         {
           proxy_source_write->src_invisible = EINA_FALSE;
           cur_source->changed_src_visible = EINA_TRUE;
           evas_object_change(o->cur->source, cur_source);
           evas_object_smart_member_cache_invalidate(o->cur->source, EINA_FALSE,
                                                     EINA_FALSE, EINA_TRUE);
         }
     }
   EINA_COW_WRITE_END(evas_object_proxy_cow, cur_source->proxy, proxy_source_write);

   if (o->cur->source)
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          state_write->source = NULL;
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }

   if (o->cur->defmap)
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             evas_map_free(state_write->defmap);
             state_write->defmap = NULL;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, cur_proxy->proxy, Evas_Object_Proxy_Data, proxy_write)
     proxy_write->is_proxy = EINA_FALSE;
   EINA_COW_WRITE_END(evas_object_proxy_cow, cur_proxy->proxy, proxy_write);
}

/**
 * @internal
 * @brief Sets up the proxy relationship between two Evas objects.
 *
 * This function establishes @p eo_src as the source for @p eo_proxy.
 * It marks @p eo_proxy as a proxy, sets its source, and adds @p eo_proxy
 * to the list of proxies managed by @p eo_src. It also triggers a redraw
 * for the source. If the proxy object had a legacy file/key set, it's cleared.
 *
 * @param eo_proxy The Evas object that will act as a proxy.
 * @param eo_src The Evas object that will be the source for the proxy.
 */
void
_evas_image_proxy_set(Evas_Object *eo_proxy, Evas_Object *eo_src)
{
   Evas_Object_Protected_Data *src = efl_data_scope_get(eo_src, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *proxy = efl_data_scope_get(eo_proxy, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_proxy, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (o->legacy_type)
     efl_file_simple_load(eo_proxy, NULL, NULL);

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, proxy->proxy, Evas_Object_Proxy_Data, proxy_write)
     proxy_write->is_proxy = EINA_TRUE;
   EINA_COW_WRITE_END(evas_object_proxy_cow, proxy->proxy, proxy_write);

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->source = eo_src;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
   o->load_error = EFL_GFX_IMAGE_LOAD_ERROR_NONE;;
   o->proxyerror = 0;

   EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, src->proxy, Evas_Object_Proxy_Data, proxy_src_write)
     {
        if (!eina_list_data_find(proxy_src_write->proxies, eo_proxy))
          proxy_src_write->proxies = eina_list_append(proxy_src_write->proxies,
                                                      eo_proxy);
        proxy_src_write->redraw = EINA_TRUE;
     }
   EINA_COW_WRITE_END(evas_object_proxy_cow, src->proxy, proxy_src_write);
}

/**
 * @internal
 * @brief Retrieves the underlying image data or surface of a proxy's source.
 *
 * This function determines if the source of the proxy is an image itself or another
 * object with a proxy surface, and returns the appropriate engine data or surface.
 *
 * @param o The image data of the proxy object.
 * @return A pointer to the engine data of the source image or the proxy surface
 *         of the source object. Returns @c NULL if the source is not set or invalid.
 */
static inline void *
_proxy_image_get(Evas_Image_Data *o)
{
   Evas_Object_Protected_Data *source = efl_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *source_img = NULL;

   if (!source)
     return NULL;

   if (efl_isa(o->cur->source, EFL_CANVAS_IMAGE_INTERNAL_CLASS))
     source_img = efl_data_scope_safe_get(o->cur->source, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (source_img)
     return source_img->engine_data;
   else
     return source->proxy->surface;
}

/**
 * @internal
 * @brief Implements efl_gfx_buffer_map for proxy objects.
 *
 * This function allows mapping the pixel data of the proxy's source.
 * It currently only supports read access and does not allow mapping specific planes
 * other than the main one. It retrieves the source's image data and then
 * attempts to map it using the engine's image_data_map function.
 *
 * @param eo_obj The proxy object.
 * @param _pd Private data (unused).
 * @param mode The access mode for mapping (only read is supported).
 * @param region The rectangular region to map. If NULL, maps the whole image.
 * @param cspace The desired colorspace for the mapped data.
 * @param plane The plane to map (only 0 is currently supported).
 * @param[out] stride Pointer to store the stride (bytes per line) of the mapped data.
 * @return An Eina_Rw_Slice representing the mapped buffer. The slice will have a NULL
 *         memory pointer and zero length on failure or if not implemented.
 *
 * @warning Writing to the returned slice is not supported and will result in an error.
 * @warning Mapping specific planes (plane != 0) is not supported.
 */
EOLIAN static Eina_Rw_Slice
_efl_canvas_proxy_efl_gfx_buffer_buffer_map(Eo *eo_obj, void *_pd EINA_UNUSED,
                                            Efl_Gfx_Buffer_Access_Mode mode,
                                            const Eina_Rect *region,
                                            Efl_Gfx_Colorspace cspace, int plane,
                                            int *stride)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   int s = 0, width = 0, height = 0;
   Eina_Rw_Slice slice = {};
   int x, y, w, h;
   void *image;

   if (!ENFN->image_data_map)
     goto end; // not implemented

   if (plane)
     {
        ERR("invalid plane id for proxy object");
        goto end;
     }

   if (mode & EFL_GFX_BUFFER_ACCESS_MODE_WRITE)
     {
        ERR("invalid map mode for proxy object");
        goto end;
     }

   image = _proxy_image_get(o);
   if (image)
     ENFN->image_size_get(ENC, image, &width, &height);

   if (!image || !width || !height)
     {
        // TODO: Create a map_surface and draw there. Maybe. This could
        // depend on the flags (eg. add a "force render" flag).
        WRN("This proxy image has no data available");
        goto end;
     }

   if (region)
     {
        x = region->x;
        y = region->y;
        w = region->w;
        h = region->h;
     }
   else
     {
        x = y = 0;
        w = width;
        h = height;
     }

   if ((x < 0) || (y < 0) || (w <= 0) || (h <= 0) || ((x + w) > width) || ((y + h) > height))
     {
        ERR("Invalid map dimensions: %dx%d +%d,%d. Image is %dx%d.",
            w, h, x, y, width, height);
        goto end;
     }

   if (ENFN->image_data_map(ENC, &o->engine_data, &slice, &s, x, y, w, h, (Evas_Colorspace)cspace, mode, plane))
     {
        DBG("map(%p, %d,%d %dx%d plane:%d) -> " EINA_SLICE_FMT,
            eo_obj, x, y, w, h, plane, EINA_SLICE_PRINT(slice));
     }
   else DBG("map(%p, %d,%d %dx%d plane:%d) -> (null)", eo_obj, x, y, w, h, plane);

end:
   if (stride) *stride = s;
   return slice;
}

/**
 * @internal
 * @brief Implements efl_gfx_buffer_unmap for proxy objects.
 *
 * This function unmaps a previously mapped buffer slice obtained via
 * _efl_canvas_proxy_efl_gfx_buffer_buffer_map.
 *
 * @param eo_obj The proxy object.
 * @param _pd Private data (unused).
 * @param slice The Eina_Rw_Slice that was previously mapped.
 * @return @c EINA_TRUE on successful unmap, @c EINA_FALSE otherwise (e.g., if slice
 *         is invalid, unmap function is not available, or engine data is missing).
 */
EOLIAN static Eina_Bool
_efl_canvas_proxy_efl_gfx_buffer_buffer_unmap(Eo *eo_obj, void *_pd EINA_UNUSED,
                                              const Eina_Rw_Slice slice)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = efl_data_scope_get(eo_obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);

   if (!slice.mem || !ENFN->image_data_unmap || !o->engine_data)
     return EINA_FALSE;

   if (!ENFN->image_data_unmap(ENC, o->engine_data, &slice))
     return EINA_FALSE;

   return EINA_TRUE;
}

/* Some moron just set a proxy on a proxy.
 * Give them some pixels.  A random color
 */
/**
 * @internal
 * @brief Handles rendering for a proxy object when a recursive proxy situation is detected.
 *
 * This function is called when a proxy's source is another proxy, creating a
 * potential infinite loop or undefined behavior. To provide visual feedback for this
 * error state, it draws a solid black rectangle where the proxy would be rendered.
 * An error message is logged the first time this occurs for a given proxy object.
 *
 * @param eo_proxy The proxy object that is part of a recursive proxy chain.
 * @param engine The Evas engine pointer.
 * @param output The Evas engine output target.
 * @param context The Evas engine drawing context.
 * @param surface The surface to draw upon.
 * @param x The x-coordinate for drawing the error rectangle.
 * @param y The y-coordinate for drawing the error rectangle.
 * @param do_async Flag indicating if rendering should be asynchronous.
 */
void
_evas_image_proxy_error(Evas_Object *eo_proxy,
                        void *engine, void *output, void *context, void *surface,
                        int x, int y, Eina_Bool do_async)
{
   Evas_Image_Data *o = efl_data_scope_get(eo_proxy, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Evas_Object_Protected_Data *proxy;
   Evas_Func *func;

   if (!o->proxyerror)
     {
        ERR("Using recursive proxies! Don't set a proxy's source as another proxy!");
        o->proxyerror = 1;
     }

   proxy = efl_data_scope_get(eo_proxy, EFL_CANVAS_OBJECT_CLASS);
   func = proxy->layer->evas->engine.func;
   func->context_color_set(engine, context, 0, 0, 0, 255);
   func->context_multiplier_unset(engine, context);
   func->context_render_op_set(engine, context, proxy->cur->render_op);
   func->rectangle_draw(engine, output, context, surface, proxy->cur->geometry.x + x,
                        proxy->cur->geometry.y + y,
                        proxy->cur->geometry.w,
                        proxy->cur->geometry.h,
                        do_async);
}

#include "efl_canvas_proxy.eo.c"
