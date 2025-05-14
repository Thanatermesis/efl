/**
 * @file
 * @brief Evas canvas rectangle object internal implementation.
 *
 * This file contains the internal implementation of the Evas canvas
 * rectangle object. It handles the rendering, state management, and
 * other low-level details specific to rectangle objects.
 */

#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EFL_CANVAS_RECTANGLE_CLASS

/* private magic number for rectangle objects */
static const char o_type[] = "rectangle";

const char *o_rect_type = o_type;

/**
 * @internal
 * @brief Private data structure for Evas rectangle objects.
 *
 * This structure holds data specific to an instance of an Evas rectangle object.
 */
typedef struct _Efl_Canvas_Rectangle_Data Efl_Canvas_Rectangle_Data;

/**
 * @internal
 * @brief Private data structure for Evas rectangle objects.
 */
struct _Efl_Canvas_Rectangle_Data
{
   void             *engine_data; /**< Engine-specific data for this rectangle object. This is used by the rendering engine to store its own state or cached information related to this object. */
};

/* private methods for rectangle objects */
/**
 * @internal
 * @brief Initializes a new Evas rectangle object.
 * @param eo_obj The Evas object (rectangle) to initialize.
 */
static void evas_object_rectangle_init(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Renders the Evas rectangle object.
 * @param eo_obj The Evas object (rectangle) to render.
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to the rectangle type.
 * @param engine The rendering engine handle.
 * @param output The output buffer/surface for rendering.
 * @param context The rendering context.
 * @param surface The target surface for rendering.
 * @param x The x-offset for rendering.
 * @param y The y-offset for rendering.
 * @param do_async Flag indicating if rendering should be asynchronous.
 */
static void evas_object_rectangle_render(Evas_Object *eo_obj,
                                         Evas_Object_Protected_Data *obj,
                                         void *type_private_data,
                                         void *engine, void *output, void *context, void *surface,
                                         int x, int y, Eina_Bool do_async);
/**
 * @internal
 * @brief Performs pre-render calculations for the Evas rectangle object.
 *
 * This function is called before the actual rendering. It calculates changes,
 * updates clip information, and adds redraw rectangles to the Evas update queue.
 * @param eo_obj The Evas object (rectangle).
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to the rectangle type.
 */
static void evas_object_rectangle_render_pre(Evas_Object *eo_obj,
                                             Evas_Object_Protected_Data *obj,
                                             void *type_private_data);
/**
 * @internal
 * @brief Performs post-render operations for the Evas rectangle object.
 *
 * This function is called after rendering. It cleans up temporary changes
 * and moves current state to previous state.
 * @param eo_obj The Evas object (rectangle).
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to the rectangle type.
 */
static void evas_object_rectangle_render_post(Evas_Object *eo_obj,
                                              Evas_Object_Protected_Data *obj,
                                              void *type_private_data);

/**
 * @internal
 * @brief Retrieves the engine-specific data for the Evas rectangle object.
 * @param eo_obj The Evas object (rectangle).
 * @return A pointer to the engine-specific data.
 */
static void *evas_object_rectangle_engine_data_get(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Checks if the Evas rectangle object is currently opaque.
 * @param eo_obj The Evas object (rectangle).
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to the rectangle type.
 * @return 1 if opaque, 0 otherwise.
 */
static int evas_object_rectangle_is_opaque(Evas_Object *eo_obj,
                                           Evas_Object_Protected_Data *obj,
                                           void *type_private_data);
/**
 * @internal
 * @brief Checks if the Evas rectangle object was previously opaque.
 * @param eo_obj The Evas object (rectangle).
 * @param obj The protected data of the Evas object.
 * @param type_private_data The private data specific to the rectangle type.
 * @return 1 if it was opaque, 0 otherwise.
 */
static int evas_object_rectangle_was_opaque(Evas_Object *eo_obj,
                                            Evas_Object_Protected_Data *obj,
                                            void *type_private_data);


#if 0 /* usless calls for a rect object. much more useful for images etc. */
static void evas_object_rectangle_store(Evas_Object *eo_obj);
static void evas_object_rectangle_unstore(Evas_Object *eo_obj);
static int evas_object_rectangle_is_visible(Evas_Object *eo_obj);
static int evas_object_rectangle_was_visible(Evas_Object *eo_obj);
static int evas_object_rectangle_is_inside(Evas_Object *eo_obj, double x, double y);
static int evas_object_rectangle_was_inside(Evas_Object *eo_obj, double x, double y);
#endif

/**
 * @internal
 * @brief Structure defining the Evas object functions for rectangle objects.
 *
 * This structure maps internal Evas object operations (like rendering,
 * opacity checks, etc.) to the specific implementations for rectangle objects.
 */
static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   NULL, /**< evas_object_free (handled by Efl_Object lifecycle) */
   evas_object_rectangle_render, /**< evas_object_render */
   evas_object_rectangle_render_pre, /**< evas_object_render_pre */
   evas_object_rectangle_render_post, /**< evas_object_render_post */
   evas_object_rectangle_engine_data_get, /**< evas_object_engine_data_get */
   /* these are optional. NULL = nothing */
   NULL, /**< evas_object_store */
   NULL, /**< evas_object_unstore */
   evas_object_rectangle_is_opaque, /**< evas_object_is_opaque */
   evas_object_rectangle_was_opaque, /**< evas_object_was_opaque */
   NULL, /**< evas_object_is_inside */
   NULL, /**< evas_object_was_inside */
   NULL, /**< evas_object_coords_recalc */
   NULL, /**< evas_object_scale_update */
   NULL, /**< evas_object_image_video_surface_setup - Not applicable */
   NULL, /**< evas_object_suspend */
   NULL, /**< evas_object_resume */
   NULL, // render_prepare /**< evas_object_render_prepare - Called before render_pre to allow objects to prepare rendering data. */
};

/**
 * @brief Adds a new rectangle object to the given Evas canvas.
 *
 * This function creates a new rectangle object. By default, the rectangle
 * will be black and have no specific geometry (x=0, y=0, w=0, h=0).
 * Its properties can be modified using generic Evas object functions
 * like evas_object_color_set(), evas_object_geometry_set(), etc.
 *
 * @param e The Evas canvas to add the rectangle to.
 * @return A handle to the newly created rectangle object, or @c NULL on failure.
 *
 * @see evas_object_color_set()
 * @see evas_object_geometry_set()
 * @see evas_object_show()
 * @see evas_object_del()
 *
 * @ingroup Evas_Object_Rectangle
 */
EVAS_API Evas_Object *
evas_object_rectangle_add(Evas *e)
{
   e = evas_find(e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(EFL_CANVAS_RECTANGLE_CLASS, e, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @internal
 * @brief Efl_Object constructor for Evas rectangle objects.
 *
 * This function is called when a new Evas rectangle object is constructed
 * using the Efl object system. It performs basic initialization.
 *
 * @param eo_obj The Evas object (rectangle) being constructed.
 * @param class_data Private data for the rectangle class (unused here).
 * @return The constructed Evas object.
 */
EOLIAN static Eo *
_efl_canvas_rectangle_efl_object_constructor(Eo *eo_obj, Efl_Canvas_Rectangle_Data *class_data EINA_UNUSED)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   evas_object_rectangle_init(eo_obj);

   return eo_obj;
}

/**
 * @internal
 * @brief Initializes the core properties of an Evas rectangle object.
 *
 * Sets up the function pointers for object-specific operations and
 * assigns the object type. This is called after the object is constructed.
 *
 * @param eo_obj The Evas object (rectangle) to initialize.
 */
static void
evas_object_rectangle_init(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = efl_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;
}

/**
 * @internal
 * @brief Renders the rectangle object onto the target surface.
 *
 * This function is called by the Evas rendering pipeline when the rectangle
 * needs to be drawn. It configures the rendering context (color, anti-aliasing,
 * render operation) and then calls the engine's rectangle drawing function.
 *
 * @param eo_obj The Evas object (rectangle), unused in this specific function but part of the generic render signature.
 * @param obj The protected data of the Evas object, containing current state like color, geometry.
 * @param type_private_data Private data specific to the rectangle type (unused).
 * @param engine The rendering engine handle.
 * @param output The output buffer/surface for rendering.
 * @param context The rendering context provided by the engine.
 * @param surface The target surface for rendering.
 * @param x The horizontal offset to apply when drawing.
 * @param y The vertical offset to apply when drawing.
 * @param do_async Flag indicating if rendering can be performed asynchronously.
 */
static void
evas_object_rectangle_render(Evas_Object *eo_obj EINA_UNUSED,
                             Evas_Object_Protected_Data *obj,
                             void *type_private_data EINA_UNUSED,
                             void *engine, void *output, void *context, void *surface, int x, int y, Eina_Bool do_async)
{
   /* render object to surface with context, and offxet by x,y */
   obj->layer->evas->engine.func->context_color_set(engine,
                                                    context,
                                                    obj->cur->cache.clip.r,
                                                    obj->cur->cache.clip.g,
                                                    obj->cur->cache.clip.b,
                                                    obj->cur->cache.clip.a);
   obj->layer->evas->engine.func->context_anti_alias_set(engine, context,
                                                         obj->cur->anti_alias);
   obj->layer->evas->engine.func->context_multiplier_unset(engine, context);
   obj->layer->evas->engine.func->context_render_op_set(engine, context,
                                                        obj->cur->render_op);
   obj->layer->evas->engine.func->rectangle_draw(engine,
                                                 output,
                                                 context,
                                                 surface,
                                                 obj->cur->geometry.x + x,
                                                 obj->cur->geometry.y + y,
                                                 obj->cur->geometry.w,
                                                 obj->cur->geometry.h,
                                                 do_async);
}

/**
 * @internal
 * @brief Pre-render phase for a rectangle object.
 *
 * This function is responsible for determining if the object needs redrawing
 * and what areas are affected. It checks for changes in visibility, clipping,
 * color, geometry, and other properties. Based on these changes, it adds
 * update rectangles to the Evas canvas.
 *
 * @param eo_obj The Evas object (rectangle).
 * @param obj The protected data of the Evas object, containing current and previous states.
 * @param type_private_data Private data specific to the rectangle type (unused).
 */
static void
evas_object_rectangle_render_pre(Evas_Object *eo_obj,
                                 Evas_Object_Protected_Data *obj,
                                 void *type_private_data EINA_UNUSED)
{
   int is_v, was_v;

   /* dont pre-render the obj twice! */
   if (obj->pre_render_done) return;
   obj->pre_render_done = EINA_TRUE;
   /* pre-render phase. this does anything an object needs to do just before */
   /* rendering. this could mean loading the image data, retrieving it from */
   /* elsewhere, decoding video etc. */
   /* then when this is done the object needs to figure if it changed and */
   /* if so what and where and add the appropriate redraw rectangles */
   /* if someone is clipping this obj - go calculate the clipper */
   if (obj->cur->clipper)
     {
        if (obj->cur->cache.clip.dirty)
          evas_object_clip_recalc(obj->cur->clipper);
        obj->cur->clipper->func->render_pre(obj->cur->clipper->object,
                                            obj->cur->clipper,
                                            obj->cur->clipper->private_data);
     }
   /* now figure what changed and add draw rects */
   /* if it just became visible or invisible */
   is_v = evas_object_is_visible(obj);
   was_v = evas_object_was_visible(obj);
   if (!(is_v | was_v)) goto done;
   if (is_v != was_v)
     {
        evas_object_render_pre_visible_change(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
        goto done;
     }
   if (obj->changed_map || obj->changed_src_visible)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* it's not visible - we accounted for it appearing or not so just abort */
   if (!is_v) goto done;
   /* clipper changed this is in addition to anything else for obj */
   evas_object_render_pre_clipper_change(&obj->layer->evas->clip_changes, eo_obj);
   /* if we restacked (layer or just within a layer) and don't clip anyone */
   if ((obj->restack) && (!obj->clip.clipees))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed render op */
   if (obj->cur->render_op != obj->prev->render_op)
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed color */
   if ((obj->cur->color.r != obj->prev->color.r) ||
       (obj->cur->color.g != obj->prev->color.g) ||
       (obj->cur->color.b != obj->prev->color.b) ||
       (obj->cur->color.a != obj->prev->color.a) ||
       (obj->cur->cache.clip.r != obj->prev->cache.clip.r) ||
       (obj->cur->cache.clip.g != obj->prev->cache.clip.g) ||
       (obj->cur->cache.clip.b != obj->prev->cache.clip.b) ||
       (obj->cur->cache.clip.a != obj->prev->cache.clip.a))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* it obviously didn't change - add a NO obscure - this "unupdates"  this */
   /* area so if there were updates for it they get wiped. don't do it if we */
   /* arent fully opaque and we are visible */
   if (evas_object_is_visible(obj) &&
       evas_object_is_opaque(obj) &&
       (!obj->clip.clipees))
     {
        Evas_Coord x, y, w, h;

        x = obj->cur->cache.clip.x;
        y = obj->cur->cache.clip.y;
        w = obj->cur->cache.clip.w;
        h = obj->cur->cache.clip.h;
        if (obj->cur->clipper)
          {
             RECTS_CLIP_TO_RECT(x, y, w, h,
                                obj->cur->clipper->cur->cache.clip.x,
                                obj->cur->clipper->cur->cache.clip.y,
                                obj->cur->clipper->cur->cache.clip.w,
                                obj->cur->clipper->cur->cache.clip.h);
          }
        evas_render_update_del(obj->layer->evas,
                               x + obj->layer->evas->framespace.x,
                               y + obj->layer->evas->framespace.y,
                               w, h);
     }
   /* if it changed geometry - and obviously not visibility or color */
   /* calculate differences since we have a constant color fill */
   /* we really only need to update the differences */
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y) ||
       (obj->cur->geometry.w != obj->prev->geometry.w) ||
       (obj->cur->geometry.h != obj->prev->geometry.h))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);

//This Performance is not so good ...!
#if 0
        evas_rects_return_difference_rects(&obj->layer->evas->clip_changes,
                                           obj->cur->geometry.x,
                                           obj->cur->geometry.y,
                                           obj->cur->geometry.w,
                                           obj->cur->geometry.h,
                                           obj->prev->geometry.x,
                                           obj->prev->geometry.y,
                                           obj->prev->geometry.w,
                                           obj->prev->geometry.h);
#endif
     }
done:
   evas_object_render_pre_effect_updates(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
}

/**
 * @internal
 * @brief Post-render phase for a rectangle object.
 *
 * This function is called after the object (and its children, if any) has been
 * rendered. It cleans up any temporary state related to clipping changes and
 * updates the object's previous state to match its current state. This prepares
 * the object for the next rendering cycle.
 *
 * @param eo_obj The Evas object (rectangle), unused.
 * @param obj The protected data of the Evas object.
 * @param type_private_data Private data specific to the rectangle type (unused).
 */
static void
evas_object_rectangle_render_post(Evas_Object *eo_obj EINA_UNUSED,
                                  Evas_Object_Protected_Data *obj,
                                  void *type_private_data EINA_UNUSED)
{

   /* this moves the current data to the previous state parts of the object */
   /* in whatever way is safest for the object. also if we don't need object */
   /* data anymore we can free it if the object deems this is a good idea */
   /* remove those pesky changes */
   evas_object_clip_changes_clean(obj);
   /* move cur to prev safely for object data */
   evas_object_cur_prev(obj);
}

/**
 * @internal
 * @brief Determines if the rectangle object is currently opaque.
 *
 * An object is opaque if it completely obscures whatever is behind it.
 * For a rectangle, this depends on its color's alpha channel and render operation.
 * If a map is applied, it's generally not considered opaque.
 *
 * @param eo_obj The Evas object (rectangle), unused.
 * @param obj The protected data of the Evas object.
 * @param type_private_data Private data specific to the rectangle type (unused).
 * @return 1 if the object is opaque, 0 otherwise.
 */
static int
evas_object_rectangle_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                Evas_Object_Protected_Data *obj,
                                void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object is */
   /* currently fully opaque over the entire rectangle it occupies */
   if ((obj->map->cur.map) && (obj->map->cur.usemap)) return 0; // If a map is active, assume not opaque for simplicity.
   if (obj->cur->render_op == EVAS_RENDER_COPY)
     return 1;
   if (obj->cur->render_op != EVAS_RENDER_BLEND)
     return 0;
   return (obj->cur->cache.clip.a == 255) ? 1 : 0; // Opaque if alpha is 255 and blend render op.
}

/**
 * @internal
 * @brief Determines if the rectangle object was opaque in its previous state.
 *
 * This is similar to evas_object_rectangle_is_opaque() but checks the
 * object's state from the previous rendering frame.
 *
 * @param eo_obj The Evas object (rectangle), unused.
 * @param obj The protected data of the Evas object.
 * @param type_private_data Private data specific to the rectangle type (unused).
 * @return 1 if the object was opaque, 0 otherwise.
 */
static int
evas_object_rectangle_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
                                 Evas_Object_Protected_Data *obj,
                                 void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object was */
   /* previously fully opaque over the entire rectangle it occupies */
   if (obj->prev->render_op == EVAS_RENDER_COPY) // EVAS_RENDER_COPY is always opaque.
     return 1;
   if (obj->prev->render_op != EVAS_RENDER_BLEND) // Other ops (like ADD, SUBTRACT) are not opaque.
     return 0;
   return (obj->prev->cache.clip.a == 255) ? 1 : 0; // Opaque if alpha was 255 and blend render op.
}

/**
 * @internal
 * @brief Retrieves the engine-specific data associated with this rectangle object.
 *
 * The rendering engine might store its own private data per object for
 * optimization or state tracking. This function provides access to that data.
 *
 * @param eo_obj The Evas object (rectangle).
 * @return A pointer to the engine-specific data, or @c NULL if none.
 */
static void *evas_object_rectangle_engine_data_get(Evas_Object *eo_obj)
{
   Efl_Canvas_Rectangle_Data *o = efl_data_scope_get(eo_obj, MY_CLASS);
   return o->engine_data;
}

#if 0 /* usless calls for a rect object. much more useful for images etc. */
static void
evas_object_rectangle_store(Evas_Object *eo_obj)
{
   /* store... nothing for rectangle objects... it's a bit silly */
   /* but for others that may have expensive caluclations to do to */
   /* generate the object data, hint that they might want to be pre-calced */
   /* once and stored */
}

static void
evas_object_rectangle_unstore(Evas_Object *eo_obj)
{
   /* store... nothing for rectangle objects... it's a bit silly */
}

static int
evas_object_rectangle_is_visible(Evas_Object *eo_obj)
{
   /* this returns 1 if the internal object data would imply that it is */
   /* visible (ie drawing it draws something. this is not to do with events */
   return 1;
}

static int
evas_object_rectangle_was_visible(Evas_Object *eo_obj)
{
   /* this returns 1 if the internal object data would imply that it was */
   /* visible (ie drawing it draws something. this is not to do with events */
   return 1;
}

static int
evas_object_rectangle_is_inside(Evas_Object *eo_obj, double x, double y)
{
   /* this returns 1 if the canvas co-ordinates are inside the object based */
   /* on object private data. not much use for rects, but for polys, images */
   /* and other complex objects it might be */
   return 1;
}

static int
evas_object_rectangle_was_inside(Evas_Object *eo_obj, double x, double y)
{
   /* this returns 1 if the canvas co-ordinates were inside the object based */
   /* on object private data. not much use for rects, but for polys, images */
   /* and other complex objects it might be */
   return 1;
}
#endif

#include "canvas/efl_canvas_rectangle.eo.c"
