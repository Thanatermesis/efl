/**
 * @file
 * @brief This file implements the Evas filter mixin, providing functionalities
 * for applying graphical filters to Evas objects.
 *
 * It handles filter program parsing, state management, rendering,
 * and interactions with Evas' rendering pipeline.
 */

#define EFL_CANVAS_FILTER_INTERNAL_PROTECTED

#include <Evas.h>

#include "evas_filter.h"

#define MY_CLASS EFL_CANVAS_FILTER_INTERNAL_MIXIN

#define FCOW_BEGIN(_pd) ({ Evas_Object_Filter_Data *_fcow = eina_cow_write(evas_object_filter_cow, (const Eina_Cow_Data**)&(_pd->data)); _state_check(_fcow); _fcow; })
#define FCOW_END(_fcow, _pd) eina_cow_done(evas_object_filter_cow, (const Eina_Cow_Data**)&(_pd->data), _fcow, EINA_TRUE)
#define FCOW_WRITE(pd, name, value) do { \
   if (pd->data->name != (value)) { \
     fcow = FCOW_BEGIN(pd); \
     fcow->name = (value); \
     FCOW_END(fcow, pd); \
   }} while (0)

typedef struct _Evas_Filter_Data Evas_Filter_Data;
typedef struct _Evas_Filter_Post_Render_Data Evas_Filter_Post_Render_Data;

/**
 * @brief Internal data structure for managing filter properties of an Evas object.
 * This structure is managed by an Eina_Cow (Copy-On-Write) mechanism.
 */
struct _Evas_Object_Filter_Data
{
   Evas_Object_Protected_Data *obj; /**< Pointer to the protected data of the Evas object. */
   Eina_Stringshare    *name; /**< Name of the filter program. */
   Eina_Stringshare    *code; /**< Lua script code for the filter. */
   Evas_Filter_Program *chain; /**< Parsed filter program chain. */
   Evas_Filter_Context *context; /**< Rendering context for the filter. */
   Eina_Hash           *sources; /**< Hash table of source objects for the filter (Evas_Filter_Proxy_Binding).
                                   * Example: `sources["source_name"] = Evas_Filter_Proxy_Binding_ptr;` */
   Eina_Inlist         *data; /**< Inlist of data bindings for the filter (Evas_Filter_Data_Binding).
                                * Example: `data = [{name="var1", value="1.0", execute=EINA_FALSE}, ...];` */
   Eina_Rectangle       prev_obscured, obscured; /**< Previous and current obscured regions. */
   Evas_Filter_Padding  prev_padding, padding; /**< Previous and current filter padding. */
   void                *output; /**< Output buffer of the filter rendering. */
   struct {
      struct {
         Eina_Stringshare *name; /**< Name of the current filter state. e.g., "default", "blur_strong" */
         double            value; /**< Value associated with the current state. */
      } cur; /**< Current filter state. */
      struct {
         Eina_Stringshare *name; /**< Name of the next filter state for transitions. */
         double            value; /**< Value associated with the next state. */
      } next; /**< Next filter state for transitions. */
      double               pos; /**< Position (0.0 to 1.0) for interpolation between current and next states. */
   } state; /**< Filter state information for animations and transitions. */
   int                  obscured_changes; /**< Counter for changes in the obscured region, used for heuristics. */
   Eina_Bool            changed : 1; /**< Flag indicating if the filter properties have changed. */
   Eina_Bool            invalid : 1; /**< Flag indicating if the filter code parsing failed. */
   Eina_Bool            async : 1; /**< Flag indicating if the filter rendering is asynchronous. */
   Eina_Bool            reuse : 1; /**< Flag indicating if the filter context should be reused. */
};

/**
 * @brief Wrapper structure for filter data, primarily used for passing to EOLIAN methods.
 */
struct _Evas_Filter_Data
{
   const Evas_Object_Filter_Data *data; /**< Const pointer to the actual filter data. */
};

/**
 * @brief Data structure for post-render tasks, used in asynchronous filter rendering.
 */
struct _Evas_Filter_Post_Render_Data
{
   Evas_Filter_Data *pd; /**< Pointer to the filter data. */
   Evas_Filter_Context *ctx; /**< Filter context used for rendering. */
   Eina_Bool success; /**< Flag indicating if the rendering was successful. */
};

// FIXME: This should be enabled (with proper heuristics)
#define FILTER_CONTEXT_REUSE EINA_FALSE

static const Evas_Object_Filter_Data evas_filter_data_cow_default = {
   .reuse = FILTER_CONTEXT_REUSE
};
Eina_Cow *evas_object_filter_cow = NULL; /**< Eina_Cow instance for managing Evas_Object_Filter_Data. */

/**
 * @brief Initializes the Evas filter mixin.
 * This function sets up the Eina_Cow for Evas_Object_Filter_Data.
 * It should be called once during Evas initialization.
 */
void
evas_filter_mixin_init(void)
{
   evas_object_filter_cow = eina_cow_add
         ("Evas Filter Data", sizeof(Evas_Object_Filter_Data), 8,
          &evas_filter_data_cow_default, EINA_TRUE);
}

/**
 * @brief Shuts down the Evas filter mixin.
 * This function cleans up the Eina_Cow for Evas_Object_Filter_Data.
 * It should be called once during Evas shutdown.
 */
void
evas_filter_mixin_shutdown(void)
{
   eina_cow_del(evas_object_filter_cow);
   evas_object_filter_cow = NULL;
}

/**
 * @brief Ensures that the filter state names (current and next) are initialized.
 * If state names are NULL, they are set to "default". This prevents crashes
 * when accessing uninitialized state names.
 * @param fcow Pointer to the writable filter data (Evas_Object_Filter_Data).
 */
static inline void
_state_check(Evas_Object_Filter_Data *fcow)
{
   if (!fcow->state.cur.name)
     fcow->state.cur.name = eina_stringshare_add("default");
   if (!fcow->state.next.name)
     fcow->state.next.name = eina_stringshare_add("default");
}

/**
 * @brief Handles the completion of a synchronous or asynchronous filter rendering operation.
 * This function updates the filter's output buffer, cleans up resources,
 * and marks the filter as dirty if rendering failed.
 * @param ctx The filter context used for rendering.
 * @param obj The protected data of the Evas object being filtered.
 * @param pd The filter data associated with the object.
 * @param success EINA_TRUE if rendering was successful, EINA_FALSE otherwise.
 */
static void
_filter_end_sync(Evas_Filter_Context *ctx, Evas_Object_Protected_Data *obj,
                 Evas_Filter_Data *pd, Eina_Bool success)
{
   void *previous = pd->data->output;
   Eina_Bool destroy = !pd->data->reuse;
   Evas_Object_Filter_Data *fcow;
   Eo *eo_obj = obj->object;
   void *output = NULL;

   if (!success)
     {
        ERR("Filter failed at runtime!");
        evas_filter_invalid_set(eo_obj, EINA_TRUE);
        evas_filter_dirty(eo_obj);
        destroy = EINA_TRUE;
     }
   else
     {
        output = evas_filter_buffer_backing_get(ctx, EVAS_FILTER_BUFFER_OUTPUT_ID, EINA_FALSE);
        FCOW_WRITE(pd, output, output);
     }

   if (previous)
     ENFN->image_free(ENC, previous);

   if (destroy && (ctx == pd->data->context))
     {
        evas_filter_context_unref(ctx); // local ref
        FCOW_WRITE(pd, context, NULL);
     }

   evas_filter_context_unref(ctx); // run ref
   efl_unref(eo_obj);
}

/**
 * @brief Callback function executed after an asynchronous filter rendering job is completed.
 * This function is called from the main loop and then calls _filter_end_sync
 * to finalize the rendering process.
 * @param data Pointer to Evas_Filter_Post_Render_Data containing task details.
 */
static void
_filter_async_post_render_cb(void *data)
{
   Evas_Filter_Post_Render_Data *task = data;
   Evas_Filter_Data *pd = task->pd;

#ifdef FILTERS_DEBUG
   EINA_SAFETY_ON_FALSE_RETURN(eina_main_loop_is());
#endif

   _filter_end_sync(task->ctx, pd->data->obj, pd, task->success);
   free(task);
}

/**
 * @brief Callback function invoked when a filter rendering operation (either sync or async) finishes.
 * For synchronous operations, it directly calls _filter_end_sync.
 * For asynchronous operations, it schedules _filter_async_post_render_cb to be
 * executed in the main loop.
 * @param ctx The filter context used for rendering.
 * @param data Pointer to Evas_Filter_Data.
 * @param success EINA_TRUE if rendering was successful, EINA_FALSE otherwise.
 */
static void
_filter_cb(Evas_Filter_Context *ctx, void *data, Eina_Bool success)
{
   Evas_Filter_Post_Render_Data *post_data;
   Evas_Object_Protected_Data *obj;
   Evas_Filter_Data *pd = data;

   obj = pd->data->obj;
   EVAS_OBJECT_DATA_VALID_CHECK(obj);

   if (!pd->data->async)
     {
        _filter_end_sync(ctx, pd->data->obj, pd, success);
        return;
     }

#ifdef FILTERS_DEBUG
   EINA_SAFETY_ON_FALSE_RETURN(!eina_main_loop_is());
#endif

   post_data = calloc(1, sizeof(*post_data));
   post_data->success = success;
   post_data->ctx = ctx;
   post_data->pd = pd;
   evas_post_render_job_add(obj->layer->evas, _filter_async_post_render_cb, post_data);
}

/**
 * @brief Callback function used to free Evas_Filter_Proxy_Binding data when removed from the sources hash.
 * This function also updates the proxy lists in the source and proxy objects.
 * @param data Pointer to the Evas_Filter_Proxy_Binding to be freed.
 */
void
_evas_filter_source_hash_free_cb(void *data)
{
   Evas_Filter_Proxy_Binding *pb = data;
   Evas_Object_Protected_Data *proxy, *source;
   Evas_Filter_Data *pd;

   proxy = efl_data_scope_get(pb->eo_proxy, EFL_CANVAS_OBJECT_CLASS);
   source = efl_data_scope_get(pb->eo_source, EFL_CANVAS_OBJECT_CLASS);

   if (source)
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, source->proxy,
                             Evas_Object_Proxy_Data, source_write)
          source_write->proxies = eina_list_remove(source_write->proxies, pb->eo_proxy);
        EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, source_write)
     }

   pd = efl_data_scope_get(pb->eo_proxy, MY_CLASS);

   if (pd && proxy)
     {
        if (!eina_hash_population(pd->data->sources))
          {
             EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, proxy->proxy,
                                  Evas_Object_Proxy_Data, proxy_write)
               proxy_write->is_proxy = EINA_FALSE;
             EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, proxy_write)
          }
     }

   eina_stringshare_del(pb->name);
   free(pb);
}

/**
 * @brief Sets the current state (current, next, position) on a given filter program.
 * It prepares an Efl_Canvas_Filter_State structure based on the object's
 * filter data and applies it to the program.
 * @param pgm The filter program to update.
 * @param pd The filter data containing the state information.
 * @return EINA_TRUE if the state was successfully set, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_evas_filter_state_set_internal(Evas_Filter_Program *pgm, Evas_Filter_Data *pd)
{
   Efl_Canvas_Filter_State state = EFL_CANVAS_FILTER_STATE_DEFAULT;

   evas_filter_state_prepare(pd->data->obj->object, &state, NULL);
   state.cur.name = pd->data->state.cur.name;
   state.cur.value = pd->data->state.cur.value;
   state.next.name = pd->data->state.next.name;
   state.next.value = pd->data->state.next.value;
   state.pos = pd->data->state.pos;

   return evas_filter_program_state_set(pgm, &state);
}

/**
 * @brief Checks if the obscured region of a filtered object has significantly changed.
 * This is used to determine if a redraw is necessary.
 * @param pd The filter data containing current and previous obscured regions.
 * @return EINA_TRUE if the obscured region has changed, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_evas_filter_obscured_region_changed(Evas_Filter_Data *pd)
{
   Eina_Rectangle inter;

   inter = pd->data->prev_obscured;
   if (eina_rectangle_is_empty(&pd->data->obscured) &&
       eina_rectangle_is_empty(&inter))
     return EINA_FALSE;
   if (!eina_rectangle_intersection(&inter, &pd->data->obscured))
     return EINA_TRUE;
   if ((inter.w != pd->data->prev_obscured.w) ||
       (inter.h != pd->data->prev_obscured.h))
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Renders an Evas object with an applied filter.
 * This is the core function for filter rendering. It handles:
 * - Filter program compilation (if not already compiled).
 * - Checking for changes that necessitate a re-render (object content, state, sources).
 * - Setting up the filter context.
 * - Rendering proxy sources.
 * - Rendering the input for the filter (the object itself).
 * - Executing the filter program.
 * - Managing output buffers and asynchronous rendering.
 *
 * @param eo_obj The Evas object (Eo pointer) to render.
 * @param obj The protected data of the Evas object.
 * @param engine The Evas engine pointer.
 * @param output The Evas engine's output structure.
 * @param context The Evas engine's context for drawing.
 * @param surface The target surface for rendering.
 * @param x The x-coordinate for rendering.
 * @param y The y-coordinate for rendering.
 * @param do_async EINA_TRUE to perform rendering asynchronously, EINA_FALSE for synchronous.
 * @param alpha EINA_TRUE if the object has an alpha channel.
 * @return EINA_TRUE if rendering was successful or is in progress (async), EINA_FALSE on failure.
 */
Eina_Bool
evas_filter_object_render(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                          void *engine, void *output, void *context, void *surface,
                          int x, int y, Eina_Bool do_async, Eina_Bool alpha)
{
   Evas_Filter_Data *pd = efl_data_scope_get(eo_obj, MY_CLASS);
   int X, Y, W, H;
   Evas_Filter_Context *filter;
   void *drawctx;
   Eina_Bool ok;
   void *previous = pd->data->output;
   Evas_Object_Filter_Data *fcow;
   Eina_Bool use_map = EINA_FALSE;
   Evas_Filter_Padding pad;

   if (pd->data->invalid || (!pd->data->chain && !pd->data->code))
     return EINA_FALSE;

   W = obj->cur->geometry.w;
   H = obj->cur->geometry.h;
   X = obj->cur->geometry.x;
   Y = obj->cur->geometry.y;

   // Prepare color multiplier
   ENFN->context_color_set(engine, context,
                           obj->cur->cache.clip.r,
                           obj->cur->cache.clip.g,
                           obj->cur->cache.clip.b,
                           obj->cur->cache.clip.a);
   if (obj->cur->clipper)
     ENFN->context_multiplier_set(engine, context,
                                  obj->cur->clipper->cur->cache.clip.r,
                                  obj->cur->clipper->cur->cache.clip.g,
                                  obj->cur->clipper->cur->cache.clip.b,
                                  obj->cur->clipper->cur->cache.clip.a);
   else
      ENFN->context_multiplier_unset(engine, context);

   if (obj->map->cur.usemap && obj->map->cur.map && (obj->map->cur.map->count >= 4))
     {
        int iw, ih;

        use_map = EINA_TRUE;
        ENFN->image_size_get(engine, previous, &iw, &ih);
        evas_object_map_update(eo_obj, x, y, iw, ih, iw, ih);
     }

   if (!pd->data->chain)
     {
        Evas_Filter_Program *pgm;
        Eina_Bool invalid;

        pgm = evas_filter_program_new(pd->data->name, alpha);
        evas_filter_program_source_set_all(pgm, pd->data->sources);
        evas_filter_program_data_set_all(pgm, pd->data->data);
        _evas_filter_state_set_internal(pgm, pd);
        invalid = !evas_filter_program_parse(pgm, pd->data->code);
        if (invalid)
          {
             ERR("Filter program parsing failed");
             evas_filter_program_del(pgm);
             pgm = NULL;
          }
        fcow = FCOW_BEGIN(pd);
        if (!invalid) evas_filter_program_padding_get(pgm, NULL, &fcow->padding);
        fcow->chain = pgm;
        fcow->invalid = invalid;
        FCOW_END(fcow, pd);
        if (invalid) return EINA_FALSE;
     }
   else if (previous && !pd->data->changed)
     {
        Eina_Bool redraw = EINA_TRUE;

        if (_evas_filter_state_set_internal(pd->data->chain, pd))
          DBG("Filter redraw by state change!");
        else if (obj->changed)
          DBG("Filter redraw by object content change!");
        else if (obj->snapshot_needs_redraw)
          DBG("Filter redraw by snapshot change!");
        else if (_evas_filter_obscured_region_changed(pd))
          DBG("Filter redraw by obscure regions change!");
        else redraw = EINA_FALSE;

        // Scan proxies to find if any changed
        if (!redraw && pd->data->sources)
          {
             Evas_Filter_Proxy_Binding *pb;
             Evas_Object_Protected_Data *source;
             Eina_Iterator *iter;

             iter = eina_hash_iterator_data_new(pd->data->sources);
             EINA_ITERATOR_FOREACH(iter, pb)
               {
                  source = efl_data_scope_get(pb->eo_source, EFL_CANVAS_OBJECT_CLASS);
                  if (source->changed)
                    {
                       redraw = EINA_TRUE;
                       break;
                    }
               }
             eina_iterator_free(iter);
          }

        if (!redraw)
          {
             // Render this image only
             if (use_map)
               {
                  ENFN->image_map_draw(engine, output, context, surface, previous,
                                       obj->map->spans, EINA_TRUE, 0, do_async);
               }
             else
               {
                  ENFN->image_draw(engine, output, context,
                                   surface, previous,
                                   0, 0, W, H,         // src
                                   X + x, Y + y, W, H, // dst
                                   EINA_FALSE,         // smooth
                                   do_async);
               }
             return EINA_TRUE;
          }
     }
   else
     {
        _evas_filter_state_set_internal(pd->data->chain, pd);
     }

   filter = pd->data->context;
   if (filter)
     {
        int prev_w, prev_h;
        Eina_Bool was_async;

        was_async = evas_filter_context_async_get(filter);
        evas_filter_context_size_get(filter, &prev_w, &prev_h);
        if ((!pd->data->reuse) || (was_async != do_async) ||
            (prev_w != W) || (prev_h != H))
          {
             evas_filter_context_unref(filter);
             FCOW_WRITE(pd, context, NULL);
             filter = NULL;
          }
     }

   if (filter)
     {
        ok = evas_filter_context_program_use(engine, output, filter, pd->data->chain, EINA_TRUE, X, Y);
        if (!ok)
          {
             evas_filter_context_unref(filter);
             FCOW_WRITE(pd, context, NULL);
             filter = NULL;
          }
     }

   if (!filter)
     {
        filter = evas_filter_context_new(obj->layer->evas, do_async, 0);

        // Run script
        ok = evas_filter_context_program_use(engine, output, filter, pd->data->chain, EINA_FALSE, X, Y);
        if (!filter || !ok)
          {
             ERR("Parsing failed?");
             evas_filter_context_unref(filter);
             FCOW_WRITE(pd, invalid, EINA_TRUE);
             FCOW_WRITE(pd, context, NULL);
             return EINA_FALSE;
          }
     }

   // Proxies
   evas_filter_context_proxy_render_all(filter, eo_obj, output, EINA_FALSE);

   // Draw Context
   drawctx = ENFN->context_new(engine);
   ENFN->context_color_set(engine, drawctx,
                           obj->cur->cache.clip.r,
                           obj->cur->cache.clip.g,
                           obj->cur->cache.clip.b,
                           obj->cur->cache.clip.a);

   // Set obscured region
   evas_filter_context_obscured_region_set(filter, pd->data->obscured);

   // Allocate all buffers now
   evas_filter_context_buffers_allocate_all(filter);
   evas_filter_target_set(filter, context, surface, X + x, Y + y,
                          use_map ? obj->map->spans : NULL);

   // Request rendering from the object itself (child class)
   evas_filter_program_padding_get(pd->data->chain, &pad, NULL);
   ok = evas_filter_input_render(eo_obj, filter, engine, output, drawctx, NULL,
                                 pad.l, pad.r, pad.t, pad.b, 0, 0, do_async);
   if (!ok) ERR("Filter input render failed.");

   ENFN->context_free(engine, drawctx);

   // Add post-run callback and run filter
   evas_filter_context_post_run_callback_set(filter, _filter_cb, pd);

   fcow = FCOW_BEGIN(pd);
   fcow->context = filter;
   fcow->changed = EINA_FALSE;
   fcow->async = do_async;
   fcow->prev_obscured = fcow->obscured;
   fcow->prev_padding = fcow->padding;
   fcow->padding = pad;
   fcow->invalid = EINA_FALSE;
   FCOW_END(fcow, pd);

   // Run the filter now (maybe async)
   efl_ref(eo_obj);
   ok = evas_filter_context_run(engine, output, filter);
   if (!ok) ERR("Filter program failed to run!");

   return ok;
}

/**
 * @internal
 * @brief Sets the filter program (Lua script) and its name for the object.
 * This function parses the provided Lua code and prepares the filter program.
 * If the code or name changes, the existing filter program and context are invalidated.
 *
 * @param eo_obj The Evas object.
 * @param pd The filter data for the object.
 * @param code The Lua script string for the filter. Example: "blur { radius = 5 }"
 * @param name A descriptive name for the filter program. Example: "my_blur_filter"
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_program_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                                              const char *code, const char *name)
{
   Evas_Object_Protected_Data *obj;
   Evas_Filter_Program *pgm = NULL;
   Evas_Object_Filter_Data *fcow;
   Eina_Bool invalid = pd->data->invalid;
   Eina_Bool alpha;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (eina_streq(pd->data->code, code) && eina_streq(pd->data->name, name))
     return;

   evas_object_async_block(obj);
   fcow = FCOW_BEGIN(pd);
   {
      fcow->obj = obj;

      evas_filter_context_unref(fcow->context);
      fcow->context = NULL;

      // Parse filter program
      evas_filter_program_del(fcow->chain);
      eina_stringshare_replace(&fcow->name, name);
      if (code)
        {
           alpha = evas_filter_input_alpha(eo_obj);
           pgm = evas_filter_program_new(fcow->name, alpha);
           evas_filter_program_source_set_all(pgm, fcow->sources);
           evas_filter_program_data_set_all(pgm, fcow->data);
           _evas_filter_state_set_internal(pgm, pd);
           invalid = !evas_filter_program_parse(pgm, code);
           if (invalid)
             {
                ERR("Parsing failed!");
                evas_filter_program_del(pgm);
                pgm = NULL;
             }
           else
             {
                evas_filter_program_padding_get(pgm, NULL, &fcow->padding);
             }
        }
      fcow->chain = pgm;
      fcow->changed = EINA_TRUE;
      fcow->invalid = invalid;
      eina_stringshare_replace(&fcow->code, code);
   }
   FCOW_END(fcow, pd);

   evas_filter_dirty(eo_obj);
}

/**
 * @internal
 * @brief Retrieves the filter program code and name for the object.
 *
 * @param eo_obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param code Pointer to store the filter program code string.
 * @param name Pointer to store the filter program name string.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_program_get(const Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, const char **code, const char **name)
{
   if (code) *code = pd->data->code;
   if (name) *name = pd->data->name;
}

/**
 * @internal
 * @brief Sets a source object for the filter program.
 * Source objects can be referenced within the filter script to use their content as input.
 *
 * @param eo_obj The Evas object (the filter proxy).
 * @param pd The filter data for the object.
 * @param name The name by which the source will be identified in the filter script. Example: "background_image"
 * @param eo_source The Efl_Gfx_Entity to be used as a source.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_source_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                                             const char *name, Efl_Gfx_Entity *eo_source)
{
   Evas_Object_Protected_Data *obj;
   Evas_Filter_Proxy_Binding *pb, *pb_old = NULL;
   Evas_Object_Protected_Data *source = NULL;
   Evas_Object_Filter_Data *fcow = NULL;
   Eina_Bool invalid = pd->data->invalid;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (eo_source)
     source = efl_data_scope_get(eo_source, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if (!name)
     {
        if (!eo_source || !pd->data->sources) return;
        if (eina_hash_del_by_data(pd->data->sources, eo_source))
          goto update;
        return;
     }

   if (!source && !pd->data->sources)
     return;

   if (pd->data->sources)
     {
        pb_old = eina_hash_find(pd->data->sources, name);
        if (pb_old && (pb_old->eo_source == eo_source)) return;
     }

   fcow = FCOW_BEGIN(pd);
   if (!fcow->sources)
     fcow->sources = eina_hash_string_small_new(_evas_filter_source_hash_free_cb);
   else if (pb_old)
     eina_hash_del(fcow->sources, name, pb_old);

   if (!source)
     {
        pb_old = eina_hash_find(fcow->sources, name);
        if (!pb_old)
          {
             FCOW_END(fcow, pd);
             return;
          }
        eina_hash_del_by_key(fcow->sources, name);
        goto update;
     }

   pb = calloc(1, sizeof(*pb));
   pb->eo_proxy = eo_obj;
   pb->eo_source = eo_source;
   pb->name = eina_stringshare_add(name);

   if (!eina_list_data_find(source->proxy->proxies, eo_obj))
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, source->proxy, Evas_Object_Proxy_Data, source_write)
          source_write->proxies = eina_list_append(source_write->proxies, eo_obj);
        EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, source_write)
     }

   if (!obj->proxy->is_proxy)
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, obj->proxy, Evas_Object_Proxy_Data, proxy_write)
          proxy_write->is_proxy = EINA_TRUE;
        EINA_COW_WRITE_END(evas_object_proxy_cow, obj->proxy, proxy_write)
     }

   eina_hash_add(fcow->sources, pb->name, pb);
   evas_filter_program_source_set_all(fcow->chain, fcow->sources);
   evas_filter_program_data_set_all(fcow->chain, fcow->data);
   invalid = !evas_filter_program_parse(fcow->chain, fcow->code);
   if (!invalid) evas_filter_program_padding_get(fcow->chain, NULL, &fcow->padding);

   // Update object
update:
   if (fcow)
     {
        fcow->changed = EINA_TRUE;
        fcow->invalid = invalid;
        FCOW_END(fcow, pd);
     }

   evas_filter_dirty(eo_obj);
}

/**
 * @internal
 * @brief Retrieves a source object previously set for the filter program.
 *
 * @param obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param name The name of the source to retrieve.
 * @return The Efl_Gfx_Entity associated with the name, or NULL if not found.
 */
EOLIAN static Efl_Gfx_Entity *
_efl_canvas_filter_internal_efl_gfx_filter_filter_source_get(const Eo *obj EINA_UNUSED, Evas_Filter_Data *pd,
                                                             const char * name)
{
   Evas_Filter_Proxy_Binding *pb = eina_hash_find(pd->data->sources, name);
   if (!pb) return NULL;
   return pb->eo_source;
}

/**
 * @internal
 * @brief Sets the filter state, including current and next states for transitions, and the transition position.
 * This allows for animating filter parameters over time.
 *
 * @param eo_obj The Evas object.
 * @param pd The filter data for the object.
 * @param cur_state Name of the current state. Example: "default"
 * @param cur_val Value associated with the current state.
 * @param next_state Name of the next state for transition. Example: "focused"
 * @param next_val Value associated with the next state.
 * @param pos Transition position (0.0 to 1.0). 0.0 means fully `cur_state`, 1.0 means fully `next_state`.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_state_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                                            const char *cur_state, double cur_val,
                                                            const char *next_state, double next_val,
                                                            double pos)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if ((cur_state != pd->data->state.cur.name) ||
       (!EINA_DBL_EQ(cur_val, pd->data->state.cur.value)) ||
       (next_state != pd->data->state.next.name) ||
       (!EINA_DBL_EQ(next_val, pd->data->state.next.value)) ||
       (!EINA_DBL_EQ(pos, pd->data->state.pos)))
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->changed = 1;
        eina_stringshare_replace(&fcow->state.cur.name, cur_state);
        fcow->state.cur.value = cur_val;
        eina_stringshare_replace(&fcow->state.next.name, next_state);
        fcow->state.next.value = next_val;
        fcow->state.pos = pos;
        FCOW_END(fcow, pd);

        if (pd->data->chain)
          {
             _evas_filter_state_set_internal(pd->data->chain, pd);
          }

        evas_filter_dirty(eo_obj);
     }
}

/**
 * @internal
 * @brief Retrieves the current filter state information.
 *
 * @param obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param cur_state Pointer to store the name of the current state.
 * @param cur_val Pointer to store the value of the current state.
 * @param next_state Pointer to store the name of the next state.
 * @param next_val Pointer to store the value of the next state.
 * @param pos Pointer to store the current transition position.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_state_get(const Eo *obj EINA_UNUSED, Evas_Filter_Data *pd,
                                                            const char **cur_state, double *cur_val,
                                                            const char **next_state, double *next_val,
                                                            double *pos)
{
   if (cur_state) *cur_state = pd->data->state.cur.name;
   if (cur_val) *cur_val = pd->data->state.cur.value;
   if (next_state) *next_state = pd->data->state.next.name;
   if (next_val) *next_val = pd->data->state.next.value;
   if (pos) *pos = pd->data->state.pos;
}

/**
 * @internal
 * @brief Retrieves the padding required by the filter.
 * Padding indicates how much extra space the filter might need around the object
 * to render correctly (e.g., for blurs or shadows).
 *
 * @param eo_obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param l Pointer to store the left padding.
 * @param r Pointer to store the right padding.
 * @param t Pointer to store the top padding.
 * @param b Pointer to store the bottom padding.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_padding_get(const Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd,
                                                              int *l, int *r, int *t, int *b)
{
   Evas_Filter_Padding pad = { 0, 0, 0, 0 };

   if (pd->data->chain)
     evas_filter_program_padding_get(pd->data->chain, &pad, NULL);

   if (l) *l = pad.l;
   if (r) *r = pad.r;
   if (t) *t = pad.t;
   if (b) *b = pad.b;
}

/**
 * @internal
 * @brief Sets the 'changed' flag for the filter.
 * This flag is used internally to indicate that the filter's properties or inputs
 * have changed and a re-render might be necessary.
 *
 * @param eo_obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param val The new value for the 'changed' flag.
 */
EOLIAN static void
_efl_canvas_filter_internal_filter_changed_set(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, Eina_Bool val)
{
   if ((&evas_filter_data_cow_default != pd->data) && (pd->data->changed != val))
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->changed = val;
        FCOW_END(fcow, pd);
     }
}

/**
 * @internal
 * @brief Sets the 'invalid' flag for the filter.
 * This flag indicates whether the filter program code is invalid (e.g., due to a parsing error).
 *
 * @param eo_obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param val The new value for the 'invalid' flag.
 */
EOLIAN static void
_efl_canvas_filter_internal_filter_invalid_set(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, Eina_Bool val)
{
   if (pd->data->invalid != val)
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->invalid = val;
        FCOW_END(fcow, pd);
     }
}

/**
 * @internal
 * @brief Constructor for the Efl.Canvas.Filter.Internal mixin.
 * Initializes the filter data for the object using Eina_Cow.
 *
 * @param eo_obj The Evas object being constructed.
 * @param pd The filter data to initialize.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_canvas_filter_internal_efl_object_constructor(Eo *eo_obj, Evas_Filter_Data *pd)
{
   Eo *obj = NULL;

   obj = efl_constructor(efl_super(eo_obj, MY_CLASS));
   pd->data = eina_cow_alloc(evas_object_filter_cow);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl.Canvas.Filter.Internal mixin.
 * Cleans up all resources associated with the filter, including the filter program,
 * context, output buffers, sources, and data bindings.
 *
 * @param eo_obj The Evas object being destructed.
 * @param pd The filter data to clean up.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_object_destructor(Eo *eo_obj, Evas_Filter_Data *pd)
{
   Evas_Object_Protected_Data *obj;
   Evas_Object_Filter_Data *fcow;
   Evas_Filter_Data_Binding *db;
   Evas_Public_Data *e;
   Eina_Inlist *il;

   if (!pd->data || (&evas_filter_data_cow_default == pd->data))
     goto finish;

   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   e = obj->layer->evas;

   if (pd->data->context)
     {
        evas_filter_context_unref(pd->data->context);
        FCOW_WRITE(pd, context, NULL);
     }

   if (pd->data->output)
     {
        if (!pd->data->async)
          ENFN->image_free(ENC, pd->data->output);
        else
          evas_unref_queue_image_put(e, pd->data->output);
     }
   eina_hash_free(pd->data->sources);
   EINA_INLIST_FOREACH_SAFE(pd->data->data, il, db)
     {
        eina_stringshare_del(db->name);
        eina_stringshare_del(db->value);
        free(db);
     }
   evas_filter_program_del(pd->data->chain);
   eina_stringshare_del(pd->data->code);
   eina_stringshare_del(pd->data->state.cur.name);
   eina_stringshare_del(pd->data->state.next.name);

finish:
   eina_cow_free(evas_object_filter_cow, (const Eina_Cow_Data **) &pd->data);

   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

/**
 * @internal
 * @brief Sets a data binding for the filter program.
 * Data bindings allow passing named string values to the filter script.
 * The `execute` flag indicates if the value string should be treated as Lua code
 * to be executed to get the actual value.
 *
 * @param eo_obj The Evas object.
 * @param pd The filter data for the object.
 * @param name The name of the data binding. Example: "blur_radius_str"
 * @param value The string value for the binding. Example: "5.0" or "my_lua_function_returning_a_value()"
 * @param execute If EINA_TRUE, the `value` string is executed as Lua code.
 *                If EINA_FALSE, `value` is treated as a literal string.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_data_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                                           const char *name, const char *value,
                                                           Eina_Bool execute)
{
   Evas_Filter_Data_Binding *db, *found = NULL;
   Evas_Object_Filter_Data *fcow;
   Eina_Bool invalid = pd->data->invalid;

   EINA_SAFETY_ON_NULL_RETURN(pd->data);
   EINA_SAFETY_ON_NULL_RETURN(name);

   EINA_INLIST_FOREACH(pd->data->data, db)
     {
        if (!strcmp(name, db->name))
          {
             if (db->execute == execute)
               {
                  if ((value == db->value) || (value && db->value && !strcmp(value, db->value)))
                    return;
               }
             found = db;
             break;
          }
     }

   fcow = FCOW_BEGIN(pd);
   {
      if (found)
        {
           // Note: we are keeping references to NULL values here.
           eina_stringshare_replace(&found->value, value);
           found->execute = execute;
        }
      else if (value)
        {
           db = calloc(1, sizeof(Evas_Filter_Data_Binding));
           db->name = eina_stringshare_add(name);
           db->value = eina_stringshare_add(value);
           db->execute = execute;
           fcow->data = eina_inlist_append(fcow->data, EINA_INLIST_GET(db));
        }
      if (fcow->chain)
        {
           evas_filter_program_data_set_all(fcow->chain, fcow->data);
           invalid = !evas_filter_program_parse(fcow->chain, fcow->code);
           if (!invalid) evas_filter_program_padding_get(fcow->chain, NULL, &fcow->padding);
        }
      fcow->invalid = invalid;
      fcow->changed = 1;
   }
   FCOW_END(fcow, pd);

   evas_filter_dirty(eo_obj);
}

/**
 * @internal
 * @brief Retrieves a data binding previously set for the filter program.
 *
 * @param obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @param name The name of the data binding to retrieve.
 * @param value Pointer to store the string value of the binding.
 * @param execute Pointer to store the 'execute' flag of the binding.
 */
EOLIAN static void
_efl_canvas_filter_internal_efl_gfx_filter_filter_data_get(const Eo *obj EINA_UNUSED, Evas_Filter_Data *pd,
                                                           const char *name, const char **value,
                                                           Eina_Bool *execute)
{
   Evas_Filter_Data_Binding *db;

   if (!value && !execute) return;
   EINA_SAFETY_ON_NULL_RETURN(pd->data);

   EINA_INLIST_FOREACH(pd->data->data, db)
     {
        if (!strcmp(name, db->name))
          {
             if (value) *value = db->value;
             if (execute) *execute = db->execute;
             return;
          }
     }

   if (value) *value = NULL;
   if (execute) *execute = EINA_FALSE;
}

/**
 * @internal
 * @brief Retrieves the current output buffer of the filter.
 * This buffer contains the result of the last filter rendering operation.
 *
 * @param obj The Evas object (unused).
 * @param pd The filter data for the object.
 * @return Pointer to the output buffer, or NULL if not available.
 */
EOLIAN static void *
_efl_canvas_filter_internal_filter_output_buffer_get(const Eo *obj EINA_UNUSED, Evas_Filter_Data *pd)
{
   return pd->data->output;
}

/**
 * @brief Updates the obscured region for a filtered object based on a tiler.
 * This function identifies the largest opaque rectangle from the tiler and sets it
 * as the obscured region. It includes heuristics to prevent excessive redraws
 * if the obscured region changes too frequently.
 *
 * @param obj The protected data of the Evas object.
 * @param tiler The Eina_Tiler containing obscured regions.
 * @return EINA_TRUE if a redraw is needed due to changes in obscured regions or padding,
 *         EINA_FALSE otherwise.
 */
Eina_Bool
_evas_filter_obscured_regions_set(Evas_Object_Protected_Data *obj, const Eina_Tiler *tiler)
{
   Evas_Object_Filter_Data *fcow;
   Eina_Rectangle prev, rect = {};
   Eina_Rectangle *r;
   Evas_Filter_Data *pd;
   Eina_Iterator *it;
   Eina_Bool was_empty = EINA_FALSE, redraw = EINA_FALSE;
   int obscured_changes = 0;
   int area = 0;

   // TODO: Can we handle more than one opaque region?

   pd = efl_data_scope_get(obj->object, MY_CLASS);
   if (!pd->data) return EINA_FALSE;

   // Find largest opaque rect
   it = eina_tiler_iterator_new(tiler);
   EINA_ITERATOR_FOREACH(it, r)
     {
        int wh = r->w * r->h;
        if (wh > area)
          {
             area = wh;
             rect = *r;
          }
     }
   eina_iterator_free(it);

   prev = pd->data->prev_obscured;
   if (!pd->data->changed && (!prev.w || !prev.h))
     {
        was_empty = EINA_TRUE;
        obscured_changes = 0;
     }
   else if (memcmp(&rect, &prev, sizeof(rect)))
     {
        fcow = FCOW_BEGIN(pd);
        fcow->obscured = rect;
        obscured_changes = fcow->obscured_changes + 1;
        if (obscured_changes > 2)
          {
             // Reset obscure as it changes too much
             memset(&fcow->obscured, 0, sizeof(fcow->obscured));
             obscured_changes = 0;
             redraw = EINA_TRUE;
          }
        FCOW_END(fcow, pd);
     }

   FCOW_WRITE(pd, obscured_changes, obscured_changes);
   if (redraw) return EINA_TRUE;

   // Snapshot objects need to be redrawn if the padding has increased
   if ((pd->data->prev_padding.l < pd->data->padding.l) ||
       (pd->data->prev_padding.r < pd->data->padding.r) ||
       (pd->data->prev_padding.t < pd->data->padding.t) ||
       (pd->data->prev_padding.b < pd->data->padding.b))
     return EINA_TRUE;

   // Snapshot objects need to be redrawn if the obscured region has shrank
   if (!was_empty && !_evas_eina_rectangle_inside(&pd->data->obscured, &prev))
     return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Retrieves the effective radius (padding) of the filter applied to an object.
 * This is similar to `_efl_canvas_filter_internal_efl_gfx_filter_filter_padding_get`
 * but is intended for internal Evas use.
 *
 * @param obj The protected data of the Evas object.
 * @param l Pointer to store the left padding (radius).
 * @param r Pointer to store the right padding (radius).
 * @param t Pointer to store the top padding (radius).
 * @param b Pointer to store the bottom padding (radius).
 */
void
_evas_filter_radius_get(Evas_Object_Protected_Data *obj, int *l, int *r, int *t, int *b)
{
   Evas_Filter_Padding pad = {};
   Evas_Filter_Data *pd;

   pd = efl_data_scope_get(obj->object, MY_CLASS);
   if (!pd->data || !pd->data->chain) goto end;

   evas_filter_program_padding_get(pd->data->chain, NULL, &pad);

end:
   if (l) *l = pad.l;
   if (r) *r = pad.r;
   if (t) *t = pad.t;
   if (b) *b = pad.b;
}

#include "canvas/efl_canvas_filter_internal.eo.c"
