#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @file
 * @brief Evas line object internal implementation.
 */

#define MY_CLASS EVAS_LINE_CLASS

/* private magic number for line objects */
static const char o_type[] = "line"; /**< Identifier for Evas Line objects. */

/**
 * @internal
 * @brief Private structure for Evas Line object internal data.
 *
 * This structure holds all the private data for an Evas Line object,
 * including its current and previous states, coordinates, and cached values.
 */
typedef struct _Evas_Line_Data      Evas_Line_Data;

/**
 * @internal
 * @brief Private data for an Evas Line object.
 */
struct _Evas_Line_Data
{
   struct {
      /** @brief Cached values for the current state. */
      struct {
         Evas_Coord    x1, y1, x2, y2; /**< Cached absolute line coordinates. */
         /** @brief Cached object geometry. */
         struct {
            Evas_Coord w, h; /**< Cached width and height of the object's bounding box. */
         } object;
      } cache;
      Evas_Coord     x1, y1, x2, y2; /**< Line coordinates relative to the object's top-left. */
   } cur, /**< Current state of the line. */
     prev; /**< Previous state of the line, used for calculating changes. */

   void             *engine_data; /**< Engine-specific data for this line object. */
   Eina_Bool         changed : 1; /**< Flag indicating if the line's specific properties (x1,y1,x2,y2) changed. */
};

/* private methods for line objects */

/**
 * @internal
 * @brief Initializes a new Evas Line object.
 * @param eo_obj The Evas Object to initialize.
 */
static void evas_object_line_init(Evas_Object *eo_obj);
/**
 * @internal
 * @brief Renders the line object.
 * @param eo_obj The Evas Object to render.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 * @param engine The rendering engine.
 * @param output The output buffer.
 * @param context The rendering context.
 * @param surface The surface to render on.
 * @param x The x offset for rendering.
 * @param y The y offset for rendering.
 * @param do_async If EINA_TRUE, rendering can be asynchronous.
 */
static void evas_object_line_render(Evas_Object *eo_obj,
                                    Evas_Object_Protected_Data *obj,
                                    void *type_private_data,
                                    void *engine, void *output, void *context, void *surface,
                                    int x, int y, Eina_Bool do_async);
/**
 * @internal
 * @brief Performs pre-render calculations for the line object.
 * This function is called before rendering to update caches and determine damage.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 */
static void evas_object_line_render_pre(Evas_Object *eo_obj,
					Evas_Object_Protected_Data *obj,
					void *type_private_data);
/**
 * @internal
 * @brief Performs post-render operations for the line object.
 * This function is called after rendering to update the previous state.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 */
static void evas_object_line_render_post(Evas_Object *eo_obj,
					 Evas_Object_Protected_Data *obj,
					 void *type_private_data);

/**
 * @internal
 * @brief Retrieves engine-specific data for the line object.
 * @param eo_obj The Evas Object.
 * @return A pointer to the engine-specific data.
 */
static void *evas_object_line_engine_data_get(Evas_Object *eo_obj);

/**
 * @internal
 * @brief Checks if the line object is currently opaque.
 * Lines are generally not considered opaque.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 * @return 1 if opaque, 0 otherwise. Always returns 0 for lines.
 */
static int evas_object_line_is_opaque(Evas_Object *eo_obj,
				      Evas_Object_Protected_Data *obj,
				      void *type_private_data);
/**
 * @internal
 * @brief Checks if the line object was previously opaque.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 * @return 1 if it was opaque, 0 otherwise. Always returns 0 for lines.
 */
static int evas_object_line_was_opaque(Evas_Object *eo_obj,
				       Evas_Object_Protected_Data *obj,
				       void *type_private_data);
/**
 * @internal
 * @brief Checks if the given coordinates are inside the line object.
 * For lines, this typically means within its bounding box.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 * @param x The x-coordinate to check.
 * @param y The y-coordinate to check.
 * @return 1 if inside, 0 otherwise.
 */
static int evas_object_line_is_inside(Evas_Object *eo_obj,
				      Evas_Object_Protected_Data *obj,
				      void *type_private_data,
				      Evas_Coord x, Evas_Coord y);
/**
 * @internal
 * @brief Checks if the given coordinates were inside the line object in its previous state.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 * @param x The x-coordinate to check.
 * @param y The y-coordinate to check.
 * @return 1 if it was inside, 0 otherwise.
 */
static int evas_object_line_was_inside(Evas_Object *eo_obj,
				       Evas_Object_Protected_Data *obj,
				       void *type_private_data,
				       Evas_Coord x, Evas_Coord y);
/**
 * @internal
 * @brief Recalculates the cached coordinates of the line object.
 * This is called when the object's geometry or line points change.
 * @param eo_obj The Evas Object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object.
 */
static void evas_object_line_coords_recalc(Evas_Object *eo_obj,
					   Evas_Object_Protected_Data *obj,
					   void *type_private_data);

/**
 * @internal
 * @brief Structure defining the Evas Object functions for line objects.
 *
 * This structure maps standard Evas object operations to the line-specific
 * implementations.
 */
static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   NULL,
   evas_object_line_render,
   evas_object_line_render_pre,
   evas_object_line_render_post,
   evas_object_line_engine_data_get,
   /* these are optional. NULL = nothing */
   NULL,
   NULL,
   evas_object_line_is_opaque,
   evas_object_line_was_opaque,
   evas_object_line_is_inside,
   evas_object_line_was_inside,
   evas_object_line_coords_recalc,
   NULL,
   NULL,
   NULL,
   NULL // render_prepare
};

/* the actual api call to add a rect */
/* it has no other api calls as all properties are standard */

/**
 * @brief Adds a new line object to the Evas canvas.
 *
 * @param e The Evas canvas to add the line to.
 * @return A new Evas_Object handle for the created line, or NULL on failure.
 *
 * @ingroup Evas_Object_Line
 */
EVAS_API Evas_Object *
evas_object_line_add(Evas *e)
{
   e = evas_find(e);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(EVAS_LINE_CLASS, e, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @internal
 * @brief Sets the start and end coordinates of the line object.
 *
 * This function updates the line's geometry based on the new (x1, y1) and (x2, y2)
 * points. It calculates the new bounding box for the line, marks the object as
 * changed, and triggers necessary updates for redrawing and event handling.
 *
 * @param eo_obj The Evas Line object.
 * @param _pd The private data of the line object.
 * @param x1 The x-coordinate of the starting point of the line.
 * @param y1 The y-coordinate of the starting point of the line.
 * @param x2 The x-coordinate of the ending point of the line.
 * @param y2 The y-coordinate of the ending point of the line.
 */
EOLIAN static void
_evas_line_xy_set(Eo *eo_obj, Evas_Line_Data *_pd, Evas_Coord x1, Evas_Coord y1, Evas_Coord x2, Evas_Coord y2)
{

   Evas_Line_Data *o = _pd;
   Evas_Coord min_x, max_x, min_y, max_y;
   Eina_List *was = NULL;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();

   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if ((x1 == (obj->cur->geometry.x + o->cur.x1)) &&
       (y1 == (obj->cur->geometry.y + o->cur.y1)) &&
       (x2 == (obj->cur->geometry.x + o->cur.x2)) &&
       (y2 == (obj->cur->geometry.y + o->cur.y2))) return;
   evas_object_async_block(obj);

   if (!(obj->layer->evas->is_frozen))
     {
        if (!evas_event_passes_through(eo_obj, obj) &&
            !evas_event_freezes_through(eo_obj, obj) &&
            !evas_object_is_source_invisible(eo_obj, obj))
          was = _evas_pointer_list_in_rect_get(obj->layer->evas, eo_obj, obj,
                                               1, 1);
     }
   if (x1 < x2)
     {
        min_x = x1;
        max_x = x2;
     }
   else
     {
        min_x = x2;
        max_x = x1;
     }
   if (y1 < y2)
     {
        min_y = y1;
        max_y = y2;
     }
   else
     {
        min_y = y2;
        max_y = y1;
     }

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     {
       state_write->geometry.x = min_x;
       state_write->geometry.y = min_y;
       state_write->geometry.w = max_x - min_x + 2;
       state_write->geometry.h = max_y - min_y + 2;
     }
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);

////   obj->cur->cache.geometry.validity = 0;
   o->cur.x1 = x1 - min_x;
   o->cur.y1 = y1 - min_y;
   o->cur.x2 = x2 - min_x;
   o->cur.y2 = y2 - min_y;
   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
   evas_object_coords_recalc(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   if (!(obj->layer->evas->is_frozen) &&
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
 * @brief Gets the start and end coordinates of the line object.
 *
 * Retrieves the current (x1, y1) and (x2, y2) coordinates of the line.
 * These coordinates are absolute canvas coordinates.
 *
 * @param eo_obj The Evas Line object.
 * @param _pd The private data of the line object.
 * @param x1 Pointer to store the x-coordinate of the starting point. Can be NULL.
 * @param y1 Pointer to store the y-coordinate of the starting point. Can be NULL.
 * @param x2 Pointer to store the x-coordinate of the ending point. Can be NULL.
 * @param y2 Pointer to store the y-coordinate of the ending point. Can be NULL.
 */
EOLIAN static void
_evas_line_xy_get(const Eo *eo_obj, Evas_Line_Data *_pd, Evas_Coord *x1, Evas_Coord *y1, Evas_Coord *x2, Evas_Coord *y2)
{
   const Evas_Line_Data *o = _pd;


   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (x1) *x1 = obj->cur->geometry.x + o->cur.x1;
   if (y1) *y1 = obj->cur->geometry.y + o->cur.y1;
   if (x2) *x2 = obj->cur->geometry.x + o->cur.x2;
   if (y2) *y2 = obj->cur->geometry.y + o->cur.y2;
}

/* all nice and private */
/**
 * @internal
 * @brief Initializes the Evas Line object after its Efl object part is constructed.
 *
 * Sets up the object function table, private data reference, and object type.
 *
 * @param eo_obj The Evas Line object to initialize.
 */
static void
evas_object_line_init(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = efl_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;
}

/**
 * @internal
 * @brief Constructor for the Evas Line Efl object.
 *
 * This function is called when a new Evas Line object is created.
 * It calls the parent constructor, initializes the Evas object part,
 * and sets default values for the line's private data.
 *
 * @param eo_obj The Evas Line object being constructed.
 * @param class_data The private data for the line object.
 * @return The constructed Evas Line object.
 */
EOLIAN static Eo *
_evas_line_efl_object_constructor(Eo *eo_obj, Evas_Line_Data *class_data EINA_UNUSED)
{
   Evas_Line_Data *o;

   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   evas_object_line_init(eo_obj);

   o = class_data;
   /* Initialize line specific private data */
   o->cur.x1 = 0;  // Default x1 relative to object's origin
   o->cur.y1 = 0;  // Default y1 relative to object's origin
   o->cur.x2 = 31; // Default x2 relative to object's origin, creating a 32x32 default bounding box
   o->cur.y2 = 31; // Default y2 relative to object's origin
   o->prev = o->cur;

   return eo_obj;
}

/**
 * @internal
 * @brief Renders the line object onto the canvas.
 *
 * This function sets up the drawing context (color, anti-aliasing, render operation)
 * and then calls the engine's line drawing function.
 *
 * @param eo_obj The Evas Line object (unused in this function but part of the signature).
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object (Evas_Line_Data).
 * @param engine The rendering engine handle.
 * @param output The output target for rendering.
 * @param context The rendering context.
 * @param surface The surface to draw upon.
 * @param x The x-offset for drawing (usually object's current x).
 * @param y The y-offset for drawing (usually object's current y).
 * @param do_async Flag for asynchronous rendering.
 */
static void
evas_object_line_render(Evas_Object *eo_obj EINA_UNUSED,
                        Evas_Object_Protected_Data *obj,
                        void *type_private_data,
                        void *engine, void *output, void *context, void *surface,
                        int x, int y, Eina_Bool do_async)
{
   Evas_Line_Data *o = type_private_data;

   /* render object to surface with context, and offset by x,y */

   obj->layer->evas->engine.func->context_color_set(engine,
                                                    context,
                                                    obj->cur->cache.clip.r,
                                                    obj->cur->cache.clip.g,
                                                    obj->cur->cache.clip.b,
                                                    obj->cur->cache.clip.a);
   obj->layer->evas->engine.func->context_multiplier_unset(engine, context);
   obj->layer->evas->engine.func->context_anti_alias_set(engine, context,
                                                         obj->cur->anti_alias);
   obj->layer->evas->engine.func->context_render_op_set(engine, context,
                                                        obj->cur->render_op);
   obj->layer->evas->engine.func->line_draw(engine, output,
                                            context,
                                            surface,
                                            o->cur.cache.x1 + x,
                                            o->cur.cache.y1 + y,
                                            o->cur.cache.x2 + x,
                                            o->cur.cache.y2 + y,
                                            do_async);
}

/**
 * @internal
 * @brief Pre-render calculations for the Evas Line object.
 *
 * This function is called before the actual rendering. It checks for changes
 * in visibility, geometry, color, clipping, or other properties. If any
 * relevant changes are detected, it adds damage rectangles to the canvas's
 * update list to ensure the correct areas are redrawn.
 *
 * @param eo_obj The Evas Line object.
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object (Evas_Line_Data).
 */
static void
evas_object_line_render_pre(Evas_Object *eo_obj,
                            Evas_Object_Protected_Data *obj,
                            void *type_private_data)
{
   Evas_Line_Data *o = type_private_data;
   int is_v, was_v;
   Eina_Bool changed_color = EINA_FALSE;

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
	evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj,
                                            obj);
        goto done;
     }
   /* it's not visible - we accounted for it appearing or not so just abort */
   if (!is_v) goto done;
   /* clipper changed this is in addition to anything else for obj */
   evas_object_render_pre_clipper_change(&obj->layer->evas->clip_changes, eo_obj);

   if ((obj->cur->color.r != obj->prev->color.r) ||
       (obj->cur->color.g != obj->prev->color.g) ||
       (obj->cur->color.b != obj->prev->color.b) ||
       (obj->cur->color.a != obj->prev->color.a))
     changed_color = EINA_TRUE;

   /* if we restacked (layer or just within a layer) */
   /* or if it changed anti_alias */
   /* or if ii changed render op */
   /* or if it changed color */
   if ((obj->restack) ||
       (obj->cur->anti_alias != obj->prev->anti_alias) ||
       (obj->cur->render_op != obj->prev->render_op) ||
       (changed_color)
      )
     {
        evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes,
                                            eo_obj, obj);
        goto done;
     }

   /* if it changed geometry - and obviously not visibility or color */
   /* calculate differences since we have a constant color fill */
   /* we really only need to update the differences */
   if ((obj->cur->geometry.x != obj->prev->geometry.x) ||
       (obj->cur->geometry.y != obj->prev->geometry.y) ||
       (obj->cur->geometry.w != obj->prev->geometry.w) ||
       (obj->cur->geometry.h != obj->prev->geometry.h) ||
       ((o->changed) &&
        ((o->cur.x1 != o->prev.x1) ||
         (o->cur.y1 != o->prev.y1) ||
         (o->cur.x2 != o->prev.x2) ||
         (o->cur.y2 != o->prev.y2)))
      )
     {
	evas_object_render_pre_prev_cur_add(&obj->layer->evas->clip_changes, eo_obj, obj);
	goto done;
     }
   done:
   evas_object_render_pre_effect_updates(&obj->layer->evas->clip_changes, eo_obj, is_v, was_v);
}

/**
 * @internal
 * @brief Post-render cleanup and state update for the Evas Line object.
 *
 * This function is called after the object (and its children, if any) has been rendered.
 * It cleans up any temporary changes made for rendering (like clip_changes)
 * and updates the 'previous' state of the object to match the 'current' state,
 * preparing it for the next rendering cycle.
 *
 * @param eo_obj The Evas Line object (unused).
 * @param obj The protected data of the Evas Object.
 * @param type_private_data The private data of the line object (Evas_Line_Data).
 */
static void
evas_object_line_render_post(Evas_Object *eo_obj EINA_UNUSED,
                             Evas_Object_Protected_Data *obj,
                             void *type_private_data)
{
   Evas_Line_Data *o = type_private_data;

   /* this moves the current data to the previous state parts of the object */
   /* in whatever way is safest for the object. also if we don't need object */
   /* data anymore we can free it if the object deems this is a good idea */
   /* remove those pesky changes */
   evas_object_clip_changes_clean(obj);
   /* move cur to prev safely for object data */
   evas_object_cur_prev(obj);
   o->prev = o->cur; // Update line-specific previous state
}

/**
 * @internal
 * @brief Retrieves the engine-specific data associated with the line object.
 *
 * @param eo_obj The Evas Line object.
 * @return A pointer to the engine-specific data, or NULL if none.
 */
static void *evas_object_line_engine_data_get(Evas_Object *eo_obj)
{
   Evas_Line_Data *o = efl_data_scope_get(eo_obj, MY_CLASS);
   return o->engine_data;
}

/**
 * @internal
 * @brief Determines if the line object is opaque.
 *
 * Lines are typically drawn with a single color and may have alpha,
 * but they don't cover an area in a way that would make them fully opaque
 * in the general sense (e.g., like a solid rectangle).
 *
 * @param eo_obj Unused.
 * @param obj Unused.
 * @param type_private_data Unused.
 * @return Always 0, indicating the line is not opaque.
 */
static int
evas_object_line_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
                           Evas_Object_Protected_Data *obj EINA_UNUSED,
                           void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object is */
   /* currently fully opaque over the entire line it occupies */
   // Lines are generally not considered opaque as they are 1-pixel wide paths.
   return 0;
}

/**
 * @internal
 * @brief Determines if the line object was opaque in its previous state.
 *
 * @param eo_obj Unused.
 * @param obj Unused.
 * @param type_private_data Unused.
 * @return Always 0, indicating the line was not opaque.
 */
static int
evas_object_line_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
                            Evas_Object_Protected_Data *obj EINA_UNUSED,
                            void *type_private_data EINA_UNUSED)
{
   /* this returns 1 if the internal object data implies that the object was */
   /* previously fully opaque over the entire line it occupies */
   // Lines are generally not considered opaque.
   return 0;
}

/**
 * @internal
 * @brief Checks if a given point (x, y) is "inside" the line object.
 *
 * For a line, "inside" typically means within its bounding box.
 * A more precise check would involve proximity to the line segment itself,
 * but Evas often uses bounding box checks for simplicity in event handling.
 * This implementation returns 1, implying a basic bounding box check is sufficient
 * or that detailed point-on-line checks are handled elsewhere or not needed.
 *
 * @param eo_obj Unused.
 * @param obj Unused.
 * @param type_private_data Unused.
 * @param x The x-coordinate to check. Unused.
 * @param y The y-coordinate to check. Unused.
 * @return 1, indicating the point is considered inside (likely based on bounding box).
 */
static int
evas_object_line_is_inside(Evas_Object *eo_obj EINA_UNUSED,
                           Evas_Object_Protected_Data *obj EINA_UNUSED,
                           void *type_private_data EINA_UNUSED,
                           Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   /* this returns 1 if the canvas co-ordinates are inside the object based */
   /* on object private data. not much use for rects, but for polys, images */
   /* and other complex objects it might be */
   // For lines, this usually means inside the bounding box.
   return 1;
}

/**
 * @internal
 * @brief Checks if a given point (x, y) was "inside" the line object in its previous state.
 *
 * Similar to evas_object_line_is_inside(), but for the previous state.
 *
 * @param eo_obj Unused.
 * @param obj Unused.
 * @param type_private_data Unused.
 * @param x The x-coordinate to check. Unused.
 * @param y The y-coordinate to check. Unused.
 * @return 1, indicating the point was considered inside.
 */
static int
evas_object_line_was_inside(Evas_Object *eo_obj EINA_UNUSED,
                            Evas_Object_Protected_Data *obj EINA_UNUSED,
                            void *type_private_data EINA_UNUSED,
                            Evas_Coord x EINA_UNUSED, Evas_Coord y EINA_UNUSED)
{
   /* this returns 1 if the canvas co-ordinates were inside the object based */
   /* on object private data. not much use for rects, but for polys, images */
   /* and other complex objects it might be */
   // For lines, this usually means inside the bounding box.
   return 1;
}

/**
 * @internal
 * @brief Recalculates cached coordinates for the line object.
 *
 * This function updates the `cache` part of the line's private data (`Evas_Line_Data`).
 * The cached coordinates (`x1`, `y1`, `x2`, `y2`) are absolute canvas coordinates,
 * derived from the object's current geometry (position `obj->cur->geometry.x, .y`)
 * and the line's relative coordinates (`o->cur.x1, .y1, .x2, .y2`).
 * It also caches the object's width and height.
 * This is typically called when the object moves or its line points are changed.
 *
 * @param eo_obj The Evas Line object (unused).
 * @param obj The protected data of the Evas Object, containing current geometry.
 * @param type_private_data The private data of the line object (Evas_Line_Data).
 */
static void
evas_object_line_coords_recalc(Evas_Object *eo_obj EINA_UNUSED,
                               Evas_Object_Protected_Data *obj,
                               void *type_private_data)
{
   Evas_Line_Data *o = type_private_data;

   o->cur.cache.x1 = obj->cur->geometry.x + o->cur.x1;
   o->cur.cache.y1 = obj->cur->geometry.y + o->cur.y1;
   o->cur.cache.x2 = obj->cur->geometry.x + o->cur.x2;
   o->cur.cache.y2 = obj->cur->geometry.y + o->cur.y2;
   o->cur.cache.object.w = obj->cur->geometry.w;
   o->cur.cache.object.h = obj->cur->geometry.h;
}

#include "canvas/evas_line_eo.c"
