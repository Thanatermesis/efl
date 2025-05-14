#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#define MY_CLASS EFL_CANVAS_VG_SHAPE_CLASS

/**
 * @brief Private data structure for Efl_Canvas_Vg_Shape.
 *
 * This structure holds the fill and stroke properties of a vector graphics shape.
 */
typedef struct _Efl_Canvas_Vg_Shape_Data Efl_Canvas_Vg_Shape_Data;
struct _Efl_Canvas_Vg_Shape_Data
{
   Efl_Canvas_Vg_Node *fill; /**< The fill node for the shape. */

   struct {
      Efl_Canvas_Vg_Node *fill;   /**< The fill node for the stroke. */
      Efl_Canvas_Vg_Node *marker; /**< The marker node for the stroke. */
   } stroke; /**< Stroke specific properties. */
};

// FIXME: Use the renderer bounding box when it has been created instead of an estimation

/**
 * @brief Sets the fill node for the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @param f The Efl_Canvas_Vg_Node to use as fill.
 */
static void
_efl_canvas_vg_shape_fill_set(Eo *obj EINA_UNUSED,
                       Efl_Canvas_Vg_Shape_Data *pd,
                       Efl_Canvas_Vg_Node *f)
{
   if (pd->fill == f) return;

   Efl_Canvas_Vg_Node *tmp = pd->fill;

   pd->fill = efl_ref(f);
   efl_unref(tmp);
}

/**
 * @brief Gets the fill node for the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @return The Efl_Canvas_Vg_Node used as fill.
 */
static Efl_Canvas_Vg_Node *
_efl_canvas_vg_shape_fill_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Vg_Shape_Data *pd)
{
   return pd->fill;
}

/**
 * @brief Sets the fill node for the stroke of the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @param f The Efl_Canvas_Vg_Node to use as stroke fill.
 */
static void
_efl_canvas_vg_shape_stroke_fill_set(Eo *obj EINA_UNUSED,
                              Efl_Canvas_Vg_Shape_Data *pd,
                              Efl_Canvas_Vg_Node *f)
{
   if (pd->stroke.fill == f) return;

   Efl_Canvas_Vg_Node *tmp = pd->stroke.fill;
   pd->stroke.fill = efl_ref(f);
   efl_unref(tmp);
}

/**
 * @brief Gets the fill node for the stroke of the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @return The Efl_Canvas_Vg_Node used as stroke fill.
 */
static Efl_Canvas_Vg_Node *
_efl_canvas_vg_shape_stroke_fill_get(const Eo *obj EINA_UNUSED,
                              Efl_Canvas_Vg_Shape_Data *pd)
{
   return pd->stroke.fill;
}

/**
 * @brief Sets the marker shape for the stroke of the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @param m The Efl_Canvas_Vg_Shape to use as stroke marker.
 */
static void
_efl_canvas_vg_shape_stroke_marker_set(Eo *obj EINA_UNUSED,
                                Efl_Canvas_Vg_Shape_Data *pd,
                                Efl_Canvas_Vg_Shape *m)
{
   Efl_Canvas_Vg_Node *tmp = pd->stroke.marker;

   pd->stroke.marker = efl_ref(m);
   efl_unref(tmp);
}

/**
 * @brief Gets the marker shape for the stroke of the shape.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @return The Efl_Canvas_Vg_Shape used as stroke marker.
 */
static Efl_Canvas_Vg_Shape *
_efl_canvas_vg_shape_stroke_marker_get(const Eo *obj EINA_UNUSED,
                                Efl_Canvas_Vg_Shape_Data *pd)
{
   return pd->stroke.marker;
}

/**
 * @brief Pre-render operations for the Efl_Canvas_Vg_Shape.
 *
 * This function is called before rendering the shape. It computes transformations,
 * alpha values, and prepares the renderers for fill, stroke fill, and stroke marker.
 *
 * @param vg_pd Evas object protected data.
 * @param obj The Efl_VG object (shape itself).
 * @param nd The node data for this shape.
 * @param engine The rendering engine.
 * @param output The rendering output target.
 * @param context The rendering context.
 * @param surface The Ector_Surface to render on.
 * @param ptransform The parent's transformation matrix.
 * @param p_opacity The parent's opacity.
 * @param comp The Ector_Buffer for compositing.
 * @param comp_method The Efl_Gfx_Vg_Composite_Method for compositing.
 * @param data The private data of the Efl_Canvas_Vg_Shape object (pd).
 */
static void
_efl_canvas_vg_shape_render_pre(Evas_Object_Protected_Data *vg_pd,
                                Efl_VG *obj,
                                Efl_Canvas_Vg_Node_Data *nd,
                                void *engine, void *output, void *context,
                                Ector_Surface *surface,
                                Eina_Matrix3 *ptransform,
                                int p_opacity,
                                Ector_Buffer *comp,
                                Efl_Gfx_Vg_Composite_Method comp_method,
                                void *data)
{
   Efl_Canvas_Vg_Shape_Data *pd = data;
   Efl_Canvas_Vg_Node_Data *fill, *stroke_fill, *stroke_marker;

   if (nd->flags == EFL_GFX_CHANGE_FLAG_NONE) return;

   nd->flags = EFL_GFX_CHANGE_FLAG_NONE;

   EFL_CANVAS_VG_COMPUTE_MATRIX(ctransform, ptransform, nd);
   EFL_CANVAS_VG_COMPUTE_ALPHA(c_r, c_g, c_b, c_a, p_opacity, nd);

   fill = _evas_vg_render_pre(vg_pd, pd->fill,
                              engine, output, context,
                              surface, ctransform, c_a, comp, comp_method);
   stroke_fill = _evas_vg_render_pre(vg_pd, pd->stroke.fill,
                                     engine, output, context,
                                     surface, ctransform, c_a, comp, comp_method);
   stroke_marker = _evas_vg_render_pre(vg_pd, pd->stroke.marker,
                                       engine, output, context,
                                       surface, ctransform, c_a, comp, comp_method);

   if (!nd->renderer)
     {
        efl_domain_current_push(EFL_ID_DOMAIN_SHARED);
        nd->renderer = ector_surface_renderer_factory_new(surface, ECTOR_RENDERER_SHAPE_MIXIN);
        efl_domain_current_pop();
     }
   ector_renderer_transformation_set(nd->renderer, ctransform);
   ector_renderer_origin_set(nd->renderer, nd->x, nd->y);
   ector_renderer_color_set(nd->renderer, c_r, c_g, c_b, c_a);
   ector_renderer_visibility_set(nd->renderer, nd->visibility);
   ector_renderer_shape_fill_set(nd->renderer, fill ? fill->renderer : NULL);
   ector_renderer_shape_stroke_fill_set(nd->renderer, stroke_fill ? stroke_fill->renderer : NULL);
   ector_renderer_shape_stroke_marker_set(nd->renderer, stroke_marker ? stroke_marker->renderer : NULL);
   efl_gfx_path_copy_from(nd->renderer, obj);
   efl_gfx_path_commit(nd->renderer);
   ector_renderer_prepare(nd->renderer);
   ector_renderer_comp_method_set(nd->renderer, comp, comp_method);
}

/**
 * @brief Constructor for Efl_Canvas_Vg_Shape.
 *
 * Initializes the shape with default stroke properties and sets up
 * its render_pre callback.
 *
 * @param obj The Efl_Canvas_Vg_Shape object being constructed.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 * @return The constructed Efl_Canvas_Vg_Shape object.
 */
static Eo *
_efl_canvas_vg_shape_efl_object_constructor(Eo *obj, Efl_Canvas_Vg_Shape_Data *pd)
{
   Efl_Canvas_Vg_Node_Data *nd;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   efl_gfx_shape_stroke_scale_set(obj, 1);
   efl_gfx_shape_stroke_location_set(obj, 0.5);
   efl_gfx_shape_stroke_cap_set(obj, EFL_GFX_CAP_BUTT);
   efl_gfx_shape_stroke_join_set(obj, EFL_GFX_JOIN_MITER);

   //NOTE: The default value is 4. It only refers to the standard of web svg.
   //      https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/stroke-miterlimit
   efl_gfx_shape_stroke_miterlimit_set(obj, 4);

   nd = efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS);
   nd->render_pre = _efl_canvas_vg_shape_render_pre;
   nd->data = pd;

   return obj;
}

/**
 * @brief Destructor for Efl_Canvas_Vg_Shape.
 *
 * Cleans up resources used by the shape, including unreferencing
 * fill and stroke nodes, and resetting the path.
 *
 * @param obj The Efl_Canvas_Vg_Shape object being destructed.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object.
 */
static void
_efl_canvas_vg_shape_efl_object_destructor(Eo *obj, Efl_Canvas_Vg_Shape_Data *pd)
{
   if (pd->fill) efl_unref(pd->fill);
   if (pd->stroke.fill) efl_unref(pd->stroke.fill);
   if (pd->stroke.marker) efl_unref(pd->stroke.marker);

   efl_gfx_path_reset(obj);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Interpolates the properties of the Efl_Canvas_Vg_Shape.
 *
 * This function interpolates the path, fill, stroke fill, and stroke marker
 * between two shapes (`from` and `to`) based on a position map (`pos_map`).
 *
 * @param obj The Efl_Canvas_Vg_Shape object to store the interpolated result.
 * @param pd The private data of the target Efl_Canvas_Vg_Shape object.
 * @param from The starting Efl_Canvas_Vg_Node for interpolation. Must be an Efl_Canvas_Vg_Shape.
 * @param to The ending Efl_Canvas_Vg_Node for interpolation. Must be an Efl_Canvas_Vg_Shape.
 * @param pos_map The interpolation position (0.0 to 1.0).
 * @return EINA_TRUE on success, EINA_FALSE otherwise (e.g., if types don't match).
 */
static Eina_Bool
_efl_canvas_vg_shape_efl_gfx_path_interpolate(Eo *obj,
                                              Efl_Canvas_Vg_Shape_Data *pd,
                                              const Efl_Canvas_Vg_Node *from,
                                              const Efl_Canvas_Vg_Node *to,
                                              double pos_map)
{
   Efl_Canvas_Vg_Shape_Data *fromd, *tod;
   Eina_Bool r = EINA_TRUE;

   //Check if both objects have same type
   if (!(efl_isa(from, MY_CLASS) && efl_isa(to, MY_CLASS)))
     return EINA_FALSE;

   //Is this the best way?
   r &= efl_gfx_path_interpolate(efl_cast(obj, EFL_CANVAS_VG_NODE_CLASS),
                                 from, to, pos_map);
   r &= efl_gfx_path_interpolate(efl_super(obj, MY_CLASS), from, to, pos_map);

   fromd = efl_data_scope_get(from, MY_CLASS);
   tod = efl_data_scope_get(to, MY_CLASS);

   //Fill
   if (fromd->fill && tod->fill && pd->fill)
     r &= efl_gfx_path_interpolate(pd->fill, fromd->fill, tod->fill, pos_map);

   //Stroke Fill
   if (fromd->stroke.fill && tod->stroke.fill && pd->stroke.fill)
     r &= efl_gfx_path_interpolate(pd->stroke.fill, fromd->stroke.fill, tod->stroke.fill, pos_map);

   //Stroke Marker
   if (fromd->stroke.marker && tod->stroke.marker && pd->stroke.marker)
     r &= efl_gfx_path_interpolate(pd->stroke.marker, fromd->stroke.marker, tod->stroke.marker, pos_map);

   return r;
}

/**
 * @brief Commits path changes for the Efl_Canvas_Vg_Shape.
 *
 * This function is called when the path of the shape has been modified.
 * It triggers a node change notification.
 *
 * @param obj The Efl_Canvas_Vg_Shape object.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object (unused).
 */
EOLIAN static void
_efl_canvas_vg_shape_efl_gfx_path_commit(Eo *obj,
                                         Efl_Canvas_Vg_Shape_Data *pd EINA_UNUSED)
{
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Duplicates the Efl_Canvas_Vg_Shape.
 *
 * Creates a deep copy of the shape, including its fill, stroke fill,
 * stroke marker, and path data.
 *
 * @param obj The Efl_Canvas_Vg_Shape object to duplicate.
 * @param pd The private data of the Efl_Canvas_Vg_Shape object to duplicate.
 * @return A new Efl_Canvas_Vg_Node representing the duplicated shape, or NULL on failure.
 */
EOLIAN static Efl_Canvas_Vg_Node *
_efl_canvas_vg_shape_efl_duplicate_duplicate(const Eo *obj, Efl_Canvas_Vg_Shape_Data *pd)
{
   Efl_Canvas_Vg_Node *node;
   Efl_Canvas_Vg_Shape_Data *sd;

   node = efl_duplicate(efl_super(obj, MY_CLASS));
   sd = efl_data_scope_get(node, MY_CLASS);

   if (pd->fill)
     {
        sd->fill = efl_duplicate(pd->fill);
        efl_parent_set(sd->fill, efl_parent_get(node));
     }

   if (pd->stroke.fill)
     {
        sd->stroke.fill = efl_duplicate(pd->stroke.fill);
        efl_parent_set(sd->stroke.fill, efl_parent_get(node));
     }

   if (pd->stroke.marker)
     {
        sd->stroke.marker = efl_duplicate(pd->stroke.marker);
        efl_parent_set(sd->stroke.marker, efl_parent_get(node));
     }

   efl_gfx_path_copy_from(node, obj);

   return node;
}

/**
 * @brief Gets the stroke scale of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @return The stroke scale value.
 * @see efl_gfx_shape_stroke_scale_get()
 */
EVAS_API double
evas_vg_shape_stroke_scale_get(Evas_Vg_Shape *obj)
{
   return efl_gfx_shape_stroke_scale_get(obj);
}

/**
 * @brief Sets the stroke scale of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param s The stroke scale value to set.
 * @see efl_gfx_shape_stroke_scale_set()
 */
EVAS_API void
evas_vg_shape_stroke_scale_set(Evas_Vg_Shape *obj, double s)
{
   efl_gfx_shape_stroke_scale_set(obj, s);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke color of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param r Pointer to store the red component (0-255).
 * @param g Pointer to store the green component (0-255).
 * @param b Pointer to store the blue component (0-255).
 * @param a Pointer to store the alpha component (0-255).
 * @see efl_gfx_shape_stroke_color_get()
 */
EVAS_API void
evas_vg_shape_stroke_color_get(Evas_Vg_Shape *obj, int *r, int *g, int *b, int *a)
{
   efl_gfx_shape_stroke_color_get(obj, r, g, b, a);
}

/**
 * @brief Sets the stroke color of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 * @param a The alpha component (0-255).
 * @see efl_gfx_shape_stroke_color_set()
 */
EVAS_API void
evas_vg_shape_stroke_color_set(Evas_Vg_Shape *obj, int r, int g, int b, int a)
{
   efl_gfx_shape_stroke_color_set(obj, r, g, b, a);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke width of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @return The stroke width value.
 * @see efl_gfx_shape_stroke_width_get()
 */
EVAS_API double
evas_vg_shape_stroke_width_get(Evas_Vg_Shape *obj)
{
   return efl_gfx_shape_stroke_width_get(obj);
}

/**
 * @brief Sets the stroke width of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param w The stroke width value to set.
 * @see efl_gfx_shape_stroke_width_set()
 */
EVAS_API void
evas_vg_shape_stroke_width_set(Evas_Vg_Shape *obj, double w)
{
   efl_gfx_shape_stroke_width_set(obj, w);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke location (center) of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @return The stroke location value (0.0 for edge, 0.5 for center, 1.0 for other edge).
 * @see efl_gfx_shape_stroke_location_get()
 */
EVAS_API double
evas_vg_shape_stroke_location_get(Evas_Vg_Shape *obj)
{
   return efl_gfx_shape_stroke_location_get(obj);
}

/**
 * @brief Sets the stroke location (center) of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param centered The stroke location value to set.
 * @see efl_gfx_shape_stroke_location_set()
 */
EVAS_API void
evas_vg_shape_stroke_location_set(Evas_Vg_Shape *obj, double centered)
{
   efl_gfx_shape_stroke_location_set(obj, centered);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke dash pattern of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param dash Pointer to store the dash pattern array.
 *             Example: `dash[0].length`, `dash[0].gap`.
 * @param length Pointer to store the length of the dash pattern array.
 * @see efl_gfx_shape_stroke_dash_get()
 */
EVAS_API void
evas_vg_shape_stroke_dash_get(Evas_Vg_Shape *obj, const Evas_Vg_Dash **dash, unsigned int *length)
{
   efl_gfx_shape_stroke_dash_get(obj, (const Efl_Gfx_Dash **)dash, length);
}

/**
 * @brief Sets the stroke dash pattern of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param dash The dash pattern array.
 *             Example: `Evas_Vg_Dash dashes[] = {{.length = 5, .gap = 2}, {.length = 1, .gap = 1}};`
 * @param length The length of the dash pattern array.
 * @see efl_gfx_shape_stroke_dash_set()
 */
EVAS_API void
evas_vg_shape_stroke_dash_set(Evas_Vg_Shape *obj, const Evas_Vg_Dash *dash, unsigned int length)
{
   efl_gfx_shape_stroke_dash_set(obj, (const Efl_Gfx_Dash *)dash, length);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke cap style of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @return The stroke cap style (Evas_Vg_Cap).
 * @see efl_gfx_shape_stroke_cap_get()
 */
EVAS_API Evas_Vg_Cap
evas_vg_shape_stroke_cap_get(Evas_Vg_Shape *obj)
{
   return (Evas_Vg_Cap)efl_gfx_shape_stroke_cap_get(obj);
}

/**
 * @brief Sets the stroke cap style of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param c The stroke cap style (Evas_Vg_Cap) to set.
 * @see efl_gfx_shape_stroke_cap_set()
 */
EVAS_API void
evas_vg_shape_stroke_cap_set(Evas_Vg_Shape *obj, Evas_Vg_Cap c)
{
   efl_gfx_shape_stroke_cap_set(obj, (Efl_Gfx_Cap)c);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the stroke join style of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @return The stroke join style (Evas_Vg_Join).
 * @see efl_gfx_shape_stroke_join_get()
 */
EVAS_API Evas_Vg_Join
evas_vg_shape_stroke_join_get(Evas_Vg_Shape *obj)
{
   return (Evas_Vg_Join)efl_gfx_shape_stroke_join_get(obj);
}

/**
 * @brief Sets the stroke join style of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param j The stroke join style (Evas_Vg_Join) to set.
 * @see efl_gfx_shape_stroke_join_set()
 */
EVAS_API void
evas_vg_shape_stroke_join_set(Evas_Vg_Shape *obj, Evas_Vg_Join j)
{
   efl_gfx_shape_stroke_join_set(obj, (Efl_Gfx_Join)j);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Sets the path commands and points for a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param op Pointer to an array of Evas_Vg_Path_Command.
 * @param points Pointer to an array of doubles representing coordinates for the commands.
 * @see efl_gfx_path_set()
 */
EVAS_API void
evas_vg_shape_path_set(Evas_Vg_Shape *obj, const Evas_Vg_Path_Command *op, const double *points)
{
   efl_gfx_path_set(obj, (const Efl_Gfx_Path_Command *)op, points);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the path commands and points of a vector graphics shape.
 * @param obj The Evas_Vg_Shape object.
 * @param op Pointer to store the array of Evas_Vg_Path_Command.
 * @param points Pointer to store the array of doubles representing coordinates.
 * @see efl_gfx_path_get()
 */
EVAS_API void
evas_vg_shape_path_get(Evas_Vg_Shape *obj, const Evas_Vg_Path_Command **op, const double **points)
{
   efl_gfx_path_get(obj, (const Efl_Gfx_Path_Command **)op, points);
}

/**
 * @brief Gets the length of the path (number of commands and points).
 * @param obj The Evas_Vg_Shape object.
 * @param commands Pointer to store the number of path commands.
 * @param points Pointer to store the number of points.
 * @see efl_gfx_path_length_get()
 */
EVAS_API void
evas_vg_shape_path_length_get(Evas_Vg_Shape *obj, unsigned int *commands, unsigned int *points)
{
   efl_gfx_path_length_get(obj, commands, points);
}

/**
 * @brief Gets the current point of the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x Pointer to store the current x-coordinate.
 * @param y Pointer to store the current y-coordinate.
 * @see efl_gfx_path_current_get()
 */
EVAS_API void
evas_vg_shape_current_get(Evas_Vg_Shape *obj, double *x, double *y)
{
   efl_gfx_path_current_get(obj, x, y);
}

/**
 * @brief Gets the current control point of the path (used for smooth curves).
 * @param obj The Evas_Vg_Shape object.
 * @param x Pointer to store the current control x-coordinate.
 * @param y Pointer to store the current control y-coordinate.
 * @see efl_gfx_path_current_ctrl_get()
 */
EVAS_API void
evas_vg_shape_current_ctrl_get(Evas_Vg_Shape *obj, double *x, double *y)
{
   efl_gfx_path_current_ctrl_get(obj, x, y);
}

/**
 * @brief Duplicates the path data from one shape to another.
 * @param obj The destination Evas_Vg_Shape object.
 * @param dup_from The source Evas_Vg_Shape object.
 * @see efl_gfx_path_copy_from()
 */
EVAS_API void
evas_vg_shape_dup(Evas_Vg_Shape *obj, Evas_Vg_Shape *dup_from)
{
   efl_gfx_path_copy_from(obj, dup_from);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Resets the path of the shape, clearing all commands and points.
 * @param obj The Evas_Vg_Shape object.
 * @see efl_gfx_path_reset()
 */
EVAS_API void
evas_vg_shape_reset(Evas_Vg_Shape *obj)
{
   efl_gfx_path_reset(obj);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a "move to" command to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate to move to.
 * @param y The y-coordinate to move to.
 * @see efl_gfx_path_append_move_to()
 */
EVAS_API void
evas_vg_shape_append_move_to(Evas_Vg_Shape *obj, double x, double y)
{
   efl_gfx_path_append_move_to(obj, x, y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a "line to" command to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the line's end point.
 * @param y The y-coordinate of the line's end point.
 * @see efl_gfx_path_append_line_to()
 */
EVAS_API void
evas_vg_shape_append_line_to(Evas_Vg_Shape *obj, double x, double y)
{
   efl_gfx_path_append_line_to(obj, x, y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a quadratic Bezier curve command to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the curve's end point.
 * @param y The y-coordinate of the curve's end point.
 * @param ctrl_x The x-coordinate of the control point.
 * @param ctrl_y The y-coordinate of the control point.
 * @see efl_gfx_path_append_quadratic_to()
 */
EVAS_API void
evas_vg_shape_append_quadratic_to(Evas_Vg_Shape *obj, double x, double y, double ctrl_x, double ctrl_y)
{
   efl_gfx_path_append_quadratic_to(obj, x, y, ctrl_x, ctrl_y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a smooth quadratic Bezier curve command to the path.
 * The control point is a reflection of the previous command's control point.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the curve's end point.
 * @param y The y-coordinate of the curve's end point.
 * @see efl_gfx_path_append_squadratic_to()
 */
EVAS_API void
evas_vg_shape_append_squadratic_to(Evas_Vg_Shape *obj, double x, double y)
{
   efl_gfx_path_append_squadratic_to(obj, x, y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a cubic Bezier curve command to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the curve's end point.
 * @param y The y-coordinate of the curve's end point.
 * @param ctrl_x0 The x-coordinate of the first control point.
 * @param ctrl_y0 The y-coordinate of the first control point.
 * @param ctrl_x1 The x-coordinate of the second control point.
 * @param ctrl_y1 The y-coordinate of the second control point.
 * @see efl_gfx_path_append_cubic_to()
 */
EVAS_API void
evas_vg_shape_append_cubic_to(Evas_Vg_Shape *obj, double x, double y, double ctrl_x0, double ctrl_y0, double ctrl_x1, double ctrl_y1)
{
   efl_gfx_path_append_cubic_to(obj, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1, x, y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a smooth cubic Bezier curve command to the path.
 * The first control point is a reflection of the previous command's second control point.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the curve's end point.
 * @param y The y-coordinate of the curve's end point.
 * @param ctrl_x The x-coordinate of the second control point.
 * @param ctrl_y The y-coordinate of the second control point.
 * @see efl_gfx_path_append_scubic_to()
 */
EVAS_API void
evas_vg_shape_append_scubic_to(Evas_Vg_Shape *obj, double x, double y, double ctrl_x, double ctrl_y)
{
   efl_gfx_path_append_scubic_to(obj, x, y, ctrl_x, ctrl_y);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends an elliptical arc command to the path (SVG style).
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the arc's end point.
 * @param y The y-coordinate of the arc's end point.
 * @param rx The x-radius of the ellipse.
 * @param ry The y-radius of the ellipse.
 * @param angle The rotation angle of the ellipse in degrees.
 * @param large_arc EINA_TRUE if the large arc should be chosen, EINA_FALSE for the small arc.
 * @param sweep EINA_TRUE if the arc should be drawn in a "positive-angle" direction, EINA_FALSE for "negative-angle".
 * @see efl_gfx_path_append_arc_to()
 */
EVAS_API void
evas_vg_shape_append_arc_to(Evas_Vg_Shape *obj, double x, double y, double rx, double ry, double angle, Eina_Bool large_arc, Eina_Bool sweep)
{
   efl_gfx_path_append_arc_to(obj, x, y, rx, ry, angle, large_arc, sweep);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends an arc command to the path (center point, width, height, angles).
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the arc's bounding box top-left corner.
 * @param y The y-coordinate of the arc's bounding box top-left corner.
 * @param w The width of the arc's bounding box.
 * @param h The height of the arc's bounding box.
 * @param start_angle The starting angle of the arc in degrees.
 * @param sweep_length The sweep length of the arc in degrees (can be negative).
 * @see efl_gfx_path_append_arc()
 */
EVAS_API void
evas_vg_shape_append_arc(Evas_Vg_Shape *obj, double x, double y, double w, double h, double start_angle, double sweep_length)
{
   efl_gfx_path_append_arc(obj, x, y, w, h, start_angle, sweep_length);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a "close path" command to the path.
 * This draws a line from the current point to the start of the current sub-path.
 * @param obj The Evas_Vg_Shape object.
 * @see efl_gfx_path_append_close()
 */
EVAS_API void
evas_vg_shape_append_close(Evas_Vg_Shape *obj)
{
   efl_gfx_path_append_close(obj);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a circle to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the circle's center.
 * @param y The y-coordinate of the circle's center.
 * @param radius The radius of the circle.
 * @see efl_gfx_path_append_circle()
 */
EVAS_API void
evas_vg_shape_append_circle(Evas_Vg_Shape *obj, double x, double y, double radius)
{
   efl_gfx_path_append_circle(obj, x, y, radius);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends a rectangle (possibly with rounded corners) to the path.
 * @param obj The Evas_Vg_Shape object.
 * @param x The x-coordinate of the rectangle's top-left corner.
 * @param y The y-coordinate of the rectangle's top-left corner.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param rx The x-radius for rounded corners (0 for sharp corners).
 * @param ry The y-radius for rounded corners (0 for sharp corners).
 * @see efl_gfx_path_append_rect()
 */
EVAS_API void
evas_vg_shape_append_rect(Evas_Vg_Shape *obj, double x, double y, double w, double h, double rx, double ry)
{
   efl_gfx_path_append_rect(obj, x, y, w, h, rx, ry);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Appends path commands from an SVG path data string.
 * @param obj The Evas_Vg_Shape object.
 * @param svg_path_data A string containing SVG path data (e.g., "M 10 10 L 20 20 Z").
 * @see efl_gfx_path_append_svg_path()
 */
EVAS_API void
evas_vg_shape_append_svg_path(Evas_Vg_Shape *obj, const char *svg_path_data)
{
   efl_gfx_path_append_svg_path(obj, svg_path_data);
   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Interpolates the path data between two shapes.
 * The command lists of `from` and `to` shapes must be identical for successful interpolation.
 * @param obj The Evas_Vg_Shape object to store the interpolated path.
 * @param from The starting Evas_Vg_Shape for interpolation.
 * @param to The ending Evas_Vg_Shape for interpolation.
 * @param pos_map The interpolation position (0.0 to 1.0).
 * @return EINA_TRUE on success, EINA_FALSE otherwise (e.g., command lists differ).
 * @see efl_gfx_path_interpolate()
 */
EVAS_API Eina_Bool
evas_vg_shape_interpolate(Evas_Vg_Shape *obj, const Evas_Vg_Shape *from, const Evas_Vg_Shape *to, double pos_map)
{
   Eina_Bool ret = efl_gfx_path_interpolate(obj, from, to, pos_map);
   efl_canvas_vg_node_change(obj);
   return ret;
}

/**
 * @brief Checks if the path commands of two shapes are identical.
 * This does not compare the points, only the sequence of commands.
 * @param obj The first Evas_Vg_Shape object.
 * @param with The second Evas_Vg_Shape object to compare with.
 * @return EINA_TRUE if the command lists are identical, EINA_FALSE otherwise.
 * @see efl_gfx_path_equal_commands()
 */
EVAS_API Eina_Bool
evas_vg_shape_equal_commands(Evas_Vg_Shape *obj, const Evas_Vg_Shape *with)
{
   return efl_gfx_path_equal_commands(obj, with);
}

/**
 * @brief Adds a new Efl_Canvas_Vg_Shape to a parent Efl_Canvas_Vg_Node.
 *
 * @param parent The parent Efl_Canvas_Vg_Node. Must not be NULL.
 * @return A new Efl_Canvas_Vg_Shape object, or NULL on failure or if parent is NULL.
 * @warning The parent must be an Efl_Canvas_Vg_Node.
 */
EVAS_API Efl_Canvas_Vg_Shape*
evas_vg_shape_add(Efl_Canvas_Vg_Node *parent)
{
   /* Warn it because the usage has been changed.
      We can remove this message after v1.21. */
   if (!parent)
     {
        ERR("Efl_Canvas_Vg_Shape only allow Efl_Canvas_Vg_Node as the parent");
        return NULL;
     }

   return efl_add(MY_CLASS, parent);
}

#include "efl_canvas_vg_shape.eo.c"
#include "efl_canvas_vg_shape_eo.legacy.c"
