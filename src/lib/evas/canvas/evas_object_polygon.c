#include "evas_common_private.h"
#include "evas_private.h"
#include "evas_polygon_private.h"

#define MY_CLASS EFL_CANVAS_POLYGON_CLASS

/* private magic number for polygon objects */
static const char o_type[] = "polygon";

/**
 * @internal
 * @brief Private structure for storing a point of a polygon object.
 *
 * This structure holds the x and y coordinates of a single vertex
 * in a polygon.
 */
typedef struct _Efl_Canvas_Polygon_Point       Efl_Canvas_Polygon_Point;

/**
 * @internal
 * @brief Defines the coordinates of a polygon point.
 */
struct _Efl_Canvas_Polygon_Point
{
   Evas_Coord x, y; /**< The x-coordinate of the point. */
};

/* private methods for polygon objects */

/**
 * @internal
 * @brief Initializes a new polygon object.
 * @param eo_obj The Evas_Object (polygon) to initialize.
 */
static void evas_object_polygon_init(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Renders the polygon object.
 * @param eo_obj The Evas_Object (polygon) to render.
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 * @param engine The rendering engine.
 * @param output The rendering output context.
 * @param context The rendering context.
 * @param surface The surface to render on.
 * @param x The x-offset for rendering.
 * @param y The y-offset for rendering.
 * @param do_async Flag for asynchronous rendering.
 */
static void evas_object_polygon_render(Evas_Object *eo_obj,
                                       Evas_Object_Protected_Data *obj,
                                       void *type_private_data,
                                       void *engine, void *output, void *context, void *surface,
                                       int x, int y, Eina_Bool do_async);

/**
 * @internal
 * @brief Frees the resources used by a polygon object.
 * @param eo_obj The Evas_Object (polygon) to free.
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 */
static void evas_object_polygon_free(Evas_Object *eo_obj,
                                     Evas_Object_Protected_Data *obj,
                                     void *type_private_data);

/**
 * @internal
 * @brief Performs pre-render operations for the polygon object.
 *
 * This function is called before the actual rendering of the object. It
 * handles tasks like recalculating clipping and determining if the object
 * needs to be redrawn.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 */
static void evas_object_polygon_render_pre(Evas_Object *eo_obj,
                                           Evas_Object_Protected_Data *obj,
                                           void *type_private_data);

/**
 * @internal
 * @brief Performs post-render operations for the polygon object.
 *
 * This function is called after the object has been rendered. It updates
 * the object's state from current to previous.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 */
static void evas_object_polygon_render_post(Evas_Object *eo_obj,
                                            Evas_Object_Protected_Data *obj,
                                            void *type_private_data);

/**
 * @internal
 * @brief Retrieves the engine-specific data for the polygon object.
 * @param eo_obj The Evas_Object (polygon).
 * @return A pointer to the engine-specific data.
 */
static void *evas_object_polygon_engine_data_get(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Checks if the polygon object is currently opaque.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 * @return 1 if opaque, 0 otherwise. Polygons are generally not opaque.
 */
static int evas_object_polygon_is_opaque(Evas_Object *eo_obj,
                                         Evas_Object_Protected_Data *obj,
                                         void *type_private_data);
/**
 * @internal
 * @brief Checks if the polygon object was previously opaque.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 * @return 1 if it was opaque, 0 otherwise. Polygons are generally not opaque.
 */
static int evas_object_polygon_was_opaque(Evas_Object *eo_obj,
                                          Evas_Object_Protected_Data *obj,
                                          void *type_private_data);
/**
 * @internal
 * @brief Checks if a given point is inside the polygon.
 *
 * Implements the ray casting algorithm (even-odd rule) to determine
 * if a point is inside the polygon.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 * @param x The x-coordinate of the point to check.
 * @param y The y-coordinate of the point to check.
 * @return 1 if the point is inside, 0 otherwise.
 */
static int evas_object_polygon_is_inside(Evas_Object *eo_obj,
                                         Evas_Object_Protected_Data *obj,
                                         void *type_private_data,
                                         Evas_Coord x, Evas_Coord y);
/**
 * @internal
 * @brief Checks if a given point was inside the polygon in its previous state.
 * @param eo_obj The Evas_Object (polygon).
 * @param obj The protected data of the Evas_Object.
 * @param type_private_data The private data of the polygon object.
 * @param x The x-coordinate of the point to check.
 * @param y The y-coordinate of the point to check.
 * @return 1 if the point was inside, 0 otherwise. (Currently always returns 1)
 */
static int evas_object_polygon_was_inside(Evas_Object *eo_obj,
                                          Evas_Object_Protected_Data *obj,
                                          void *type_private_data,
                                          Evas_Coord x, Evas_Coord y);

/**
 * @internal
 * @brief Structure defining the Evas_Object_Func interface for polygon objects.
 *
 * This structure maps internal Evas object operations to the specific
 * implementations for polygon objects.
 */
static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   evas_object_polygon_free,
     evas_object_polygon_render,
     evas_object_polygon_render_pre,
     evas_object_polygon_render_post,
     evas_object_polygon_engine_data_get,
   /* these are optional. NULL = nothing */
     NULL,
     NULL,
     evas_object_polygon_is_opaque,
     evas_object_polygon_was_opaque,
     evas_object_polygon_is_inside,
     evas_object_polygon_was_inside,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL    // render_prepare
};

/* the actual api call to add a rect */
/* it has no other api calls as all properties are standard */

/**
 * @brief Adds a new polygon object to the given Evas canvas.
 *
 * @param e The Evas canvas to add the polygon to.
 * @return A handle to the new polygon object, or @c NULL on failure.
 *
 * @ingroup Evas_Object_Polygon
 */
EVAS_API Evas_Object *
evas_object_polygon_add(Evas *e)
{
   e = evas_find(e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, e, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @internal
 * @brief Constructor for Efl_Canvas_Polygon objects.
 *
 * This function is called when a new polygon object is created using
 * efl_add. It initializes the Evas-specific parts of the polygon.
 *
 * @param eo_obj The Efl_Canvas_Polygon object being constructed.
 * @param class_data The private data for the Efl_Canvas_Polygon class (unused).
 * @return The constructed Efl_Canvas_Polygon object.
 */
EOLIAN static Eo *
_efl_canvas_polygon_efl_object_constructor(Eo *eo_obj, Efl_Canvas_Polygon_Data *class_data EINA_UNUSED)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   evas_object_polygon_init(eo_obj);

   return eo_obj;
}

/**
 * @internal
 * @brief Adds a point to the polygon object.
 *
 * Points are added relative to the polygon's current position. The polygon's
 * geometry is updated to encompass all added points.
 *
 * @param eo_obj The Efl_Canvas_Polygon object.
 * @param _pd The private data for the Efl_Canvas_Polygon.
 * @param pos The 2D position (Eina_Position2D) of the point to add.
 *            Example: EINA_POSITION2D(10, 20)
 */
EOLIAN static void
_efl_canvas_polygon_point_add(Eo *eo_obj, Efl_Canvas_Polygon_Data *_pd, Eina_Position2D pos)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Efl_Canvas_Polygon_Data *o = _pd;
   Efl_Canvas_Polygon_Point *p;
   Evas_Coord min_x, max_x, min_y, max_y;
   Eina_List *was = NULL;

   evas_object_async_block(obj);
   if (!obj->layer->evas->is_frozen)
     {
        if (!evas_event_passes_through(eo_obj, obj) &&
            !evas_event_freezes_through(eo_obj, obj) &&
            !evas_object_is_source_invisible(eo_obj, obj))
          was = _evas_pointer_list_in_rect_get(obj->layer->evas, eo_obj, obj,
                                               1, 1);
     }
   if (!o->points)
     {
        o->offset.x = obj->cur->geometry.x;
        o->offset.y = obj->cur->geometry.y;
     }
   else
     {
        /* Update all points and take offset into account. */
        Eina_List *over;

        EINA_LIST_FOREACH(o->points, over, p)
          {
             p->x += o->offset.x;
             p->y += o->offset.y;
          }
     }

   p = malloc(sizeof(Efl_Canvas_Polygon_Point));
   if (!p) return;
   p->x = pos.x + o->offset.x;
   p->y = pos.y + o->offset.y;

   if (!o->points)
     {
        EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
          {
             state_write->geometry.x = p->x;
             state_write->geometry.y = p->y;
             state_write->geometry.w = 2;
             state_write->geometry.h = 2;
          }
        EINA_COW_STATE_WRITE_END(obj, state_write, cur);
     }
   else
     {
        if (p->x < obj->cur->geometry.x) min_x = p->x;
        else min_x = obj->cur->geometry.x;
        if (p->x > (obj->cur->geometry.x + obj->cur->geometry.w - 2))
          max_x = p->x;
        else max_x = obj->cur->geometry.x + obj->cur->geometry.w - 2;
        if (p->y < obj->cur->geometry.y) min_y = p->y;
        else min_y = obj->cur->geometry.y;
        if (p->y > (obj->cur->geometry.y + obj->cur->geometry.h - 2))
          max_y = p->y;
        else max_y = obj->cur->geometry.y + obj->cur->geometry.h - 2;

        EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
          {
             state_write->geometry.x = min_x;
             state_write->geometry.y = min_y;
             state_write->geometry.w = max_x - min_x + 2;
             state_write->geometry.h = max_y - min_y + 2;
          }
        EINA_COW_STATE_WRITE_END(obj, state_write, cur);
     }
   o->points = eina_list_append(o->points, p);

   o->geometry = obj->cur->geometry;
   o->offset.x = 0;
   o->offset.y = 0;

   ////   obj->cur->cache.geometry.validity = 0;
   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_coords_recalc(eo_obj, obj);
   if (!obj->layer->evas->is_frozen &&
       !evas_event_passes_through(eo_obj, obj) &&
       !evas_event_freezes_through(eo_obj, obj) &&
       !evas_object_is_source_invisible(eo_obj, obj) &&
       obj->cur->visible)
     _evas_canvas_event_pointer_in_list_mouse_move_feed(obj->layer->evas, was, eo_obj, obj, 1, 1, EINA_TRUE, NULL);
   eina_list_free(was);
   evas_object_inform_call_move(eo_obj, obj);
   evas_object_inform_call_resize(eo_obj, obj);
}

/**
 * @internal
 * @brief Clears all points from the polygon object.
 *
 * This function removes all vertices from the polygon, effectively making it
 * an empty polygon. Its geometry is reset to (0,0,0,0).
 *
 * @param eo_obj The Efl_Canvas_Polygon object.
 * @param _pd The private data for the Efl_Canvas_Polygon.
 */
EOLIAN static void
_efl_canvas_polygon_points_clear(Eo *eo_obj, Efl_Canvas_Polygon_Data *_pd)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Efl_Canvas_Polygon_Data *o = _pd;
   void *list_data;
   Eina_List *was;

   evas_object_async_block(obj);
   was = _evas_pointer_list_in_rect_get(obj->layer->evas, eo_obj, obj, 1, 1);

   EINA_LIST_FREE(o->points, list_data)
     {
        free(list_data);
     }

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     {
        state_write->geometry.x = 0;
        state_write->geometry.y = 0;
        state_write->geometry.w = 0;
        state_write->geometry.h = 0;
     }
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);

   ////   obj->cur->cache.geometry.validity = 0;
   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_coords_recalc(eo_obj, obj);
   if (obj->cur->visible)
     _evas_canvas_event_pointer_in_list_mouse_move_feed(obj->layer->evas, was, eo_obj, obj, 1, 1, EINA_FALSE, NULL);
   eina_list_free(was);
   evas_object_inform_call_move(eo_obj, obj);
   evas_object_inform_call_resize(eo_obj, obj);
}

/* all nice and private */
static void
evas_object_polygon_init(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = efl_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;
}

/**
 * @internal
 * @brief Destructor for Efl_Canvas_Polygon objects.
 *
 * This function is called when a polygon object is being destroyed.
 * It frees resources associated with the Evas-specific parts of the polygon.
 *
 * @param eo_obj The Efl_Canvas_Polygon object being destroyed.
 * @param _pd The private data for the Efl_Canvas_Polygon (unused).
 */
EOLIAN static void
_efl_canvas_polygon_efl_object_destructor(Eo *eo_obj, Efl_Canvas_Polygon_Data *_pd EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   evas_object_polygon_free(eo_obj, obj, obj->private_data);
   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

static void
evas_object_polygon_free(Evas_Object *eo_obj EINA_UNUSED,
                         Evas_Object_Protected_Data *obj,
                         void *type_private_data)
{
   Efl_Canvas_Polygon_Data *o = type_private_data;
   void *list_data;
   /* free obj */
   EINA_LIST_FREE(o->points, list_data)
     {
        free(list_data);
     }
   o->engine_data = obj->layer->evas->engine.func->polygon_points_clear(ENC, o->engine_data);
}

static void
evas_object_polygon_render(Evas_Object *eo_obj EINA_UNUSED,
                           Evas_Object_Protected_Data *obj,
                           void *type_private_data,
                           void *engine, void *output, void *context, void *surface, int x, int y, Eina_Bool do_async)
{
   Efl_Canvas_Polygon_Data *o = type_private_data;
   Eina_List *l;
   Efl_Canvas_Polygon_Point *p;

   /* render object to surface with context, and offxet by x,y */
   obj->layer->evas->engine.func->context_color_set(engine,
                                                    context,
                                                    obj->cur->cache.clip.r,
                                                    obj->cur->cache.clip.g,
                                                    obj->cur->cache.clip.b,
                                                    obj->cur->cache.clip.a);
   obj->layer->evas->engine.func->context_multiplier_unset(engine, context);
   obj->layer->evas->engine.func->context_render_op_set(engine, context,
                                                        obj->cur->render_op);
   if (o->changed)
     {
        o->engine_data = obj->layer->evas->engine.func->polygon_points_clear(engine, o->engine_data);
        EINA_LIST_FOREACH(o->points, l, p)
          {
             //px = evas_coord_world_x_to_screen(obj->layer->evas, p->x);
             //py = evas_coord_world_y_to_screen(obj->layer->evas, p->y);
             o->engine_data = obj->layer->evas->engine.func->polygon_point_add(engine,
                                                                               o->engine_data,
                                                                               p->x, p->y);
          }
     }

   if (o->engine_data)
     obj->layer->evas->engine.func->polygon_draw(engine,
                                                 output,
                                                 context,
                                                 surface,
                                                 o->engine_data,
                                                 o->offset.x + x, o->offset.y + y,
                                                 do_async);
}

static void
evas_object_polygon_render_pre(Evas_Object *eo_obj,
			       Evas_Object_Protected_Data *obj,
			       void *type_private_data)
{
   Efl_Canvas_Polygon_Data *o = type_private_data;
   int is_v, was_v;

   /* dont pre-render the obj twice! */
   if (obj->pre_render_done) return;
   obj->pre_render_done = EINA_TRUE;
   /* pre-render phase. this does anything an object needs to do just before */
   /* rendering. this could mean loading the image data, retrieving it from */
   /* elsewhere, decoding video etc. */
   /* then when this is done the object needs to figure if it changed and */
   /* if so what and where and add the appropriate redraw lines */
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
   /* if we restacked (layer or just within a layer) */
   if (obj->restack)
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
       (obj->cur->color.a != obj->prev->color.a))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed geometry - and obviously not visibility or color */
   /* calculate differences since we have a constant color fill */
   /* we really only need to update the differences */
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y) ||
       (obj->cur->geometry.w != obj->prev->geometry.w) ||
       (obj->cur->geometry.h != obj->prev->geometry.h) ||
       (o->changed))
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
        goto done;
     }
   done:
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y))
     {
        if (!o->changed)
          {
             o->offset.x = obj->cur->geometry.x - obj->prev->geometry.x;
             o->offset.y = obj->cur->geometry.y - obj->prev->geometry.y;
          }
        else
          {
             o->offset.x = obj->cur->geometry.x - o->geometry.x;
             o->offset.y = obj->cur->geometry.y - o->geometry.y;
          }
     }
   evas_object_render_pre_effect_updates(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
}

static void
evas_object_polygon_render_post(Evas_Object *eo_obj EINA_UNUSED,
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

static void *evas_object_polygon_engine_data_get(Evas_Object *eo_obj)
{
   Efl_Canvas_Polygon_Data *o = efl_data_scope_get(eo_obj, MY_CLASS);
   return o->engine_data;
}

static int
evas_object_polygon_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
			      Evas_Object_Protected_Data *obj EINA_UNUSED,
			      void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object is */
   /* currently fully opaque over the entire line it occupies */
   return 0;
}

static int
evas_object_polygon_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
			       Evas_Object_Protected_Data *obj EINA_UNUSED,
			       void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object was */
   /* previously fully opaque over the entire line it occupies */
   return 0;
}

/* We count the number of edges a "ray" 90 degs upwards from our point
 * intersects with. If it's even, we are outside of the polygon, if it's odd,
 * we are inside of it. This is known as the Ray Casting algorithm or the
 * even-odd rule algorithm.
 */
static int
evas_object_polygon_is_inside(Evas_Object *eo_obj EINA_UNUSED,
			      Evas_Object_Protected_Data *obj EINA_UNUSED,
			      void *type_private_data,
			      Evas_Coord x, Evas_Coord y)
{
   Efl_Canvas_Polygon_Data *o = type_private_data;
   int num_edges = 0; /* Number of edges we crossed */
   Eina_List *itr;
   Efl_Canvas_Polygon_Point *p;

   if (!o->points) return 0;

   /* Adjust X and Y according to current geometry */
   x -= o->offset.x;
   y -= o->offset.y;

   if (eina_list_count(o->points) == 1)
     {
        p = eina_list_data_get(o->points);
        return ((p->x == x) && (p->y == y));
     }

   EINA_LIST_FOREACH(o->points, itr, p)
     {
        Evas_Coord line_y;
        Eina_List *next = eina_list_next(itr);
        Efl_Canvas_Polygon_Point *p_next;
        /* Get the next, or if there's no next, take the first */
        if (next)
          {
             p_next = eina_list_data_get(next);
          }
        else
          {
             p_next = eina_list_data_get(o->points);
          }

        /* Make sure that we are directly below the edge,
         * and that p->x != p_next->x */
        if (((p->x < p_next->x) && (p->x <= x) && (x < p_next->x)) ||
              ((p->x > p_next->x) && (p_next->x < x) && (x <= p->x)))
          {
             line_y = ((double) (p->y - p_next->y) /
                   (double) (p->x - p_next->x)) *
                (x - p_next->x) + p_next->y;
             /* We crossed that edge if the line is directly above us */
             if (line_y < y)
                num_edges++;
          }
     }

   /* Return true if num_edges is odd */
   return ((num_edges % 2) == 1);
}

static int
evas_object_polygon_was_inside(Evas_Object *eo_obj EINA_UNUSED,
                               Evas_Object_Protected_Data *obj EINA_UNUSED,
                               void *type_private_data EINA_UNUSED,
                               Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   /* this returns 1 if the canvas co-ordinates were inside the object based */
   /* on object private data. not much use for rects, but for polys, images */
   /* and other complex objects it might be */
   return 1;
}

/**
 * @brief Adds a point to the given polygon object.
 *
 * @param obj The polygon object.
 * @param x The x-coordinate of the point to add.
 * @param y The y-coordinate of the point to add.
 *
 * @ingroup Evas_Object_Polygon
 * @see efl_canvas_polygon_point_add()
 */
EVAS_API void
evas_object_polygon_point_add(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   efl_canvas_polygon_point_add(obj, EINA_POSITION2D(x, y));
}

/**
 * @brief Clears all points from the given polygon object.
 *
 * @param obj The polygon object.
 *
 * @ingroup Evas_Object_Polygon
 * @see efl_canvas_polygon_points_clear()
 */
EVAS_API void
evas_object_polygon_points_clear(Evas_Object *obj)
{
   efl_canvas_polygon_points_clear(obj);
}

#include "canvas/efl_canvas_polygon.eo.c"
