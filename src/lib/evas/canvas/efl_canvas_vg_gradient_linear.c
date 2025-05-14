#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#include <strings.h>

#define MY_CLASS EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS

/**
 * @brief Private data structure for Efl_Canvas_Vg_Gradient_Linear objects.
 *
 * This structure holds the specific data for a linear gradient,
 * primarily its start and end points.
 */
typedef struct _Efl_Canvas_Vg_Gradient_Linear_Data Efl_Canvas_Vg_Gradient_Linear_Data;
struct _Efl_Canvas_Vg_Gradient_Linear_Data
{
   struct {
      double x, y; /**< Coordinates of the point. */
   } start, /**< The start point (x, y) of the linear gradient. */
     end;   /**< The end point (x, y) of the linear gradient. */
};

/**
 * @brief Sets the start point of the linear gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object.
 * @param[in,out] pd The private data of the object.
 * @param[in] x The x-coordinate of the start point.
 * @param[in] y The y-coordinate of the start point.
 */
static void
_efl_canvas_vg_gradient_linear_efl_gfx_gradient_linear_start_set(Eo *obj EINA_UNUSED,
                                                          Efl_Canvas_Vg_Gradient_Linear_Data *pd,
                                                          double x, double y)
{
   pd->start.x = x;
   pd->start.y = y;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the start point of the linear gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object.
 * @param[in] pd The private data of the object.
 * @param[out] x Pointer to store the x-coordinate of the start point. Can be NULL.
 * @param[out] y Pointer to store the y-coordinate of the start point. Can be NULL.
 */
static void
_efl_canvas_vg_gradient_linear_efl_gfx_gradient_linear_start_get(const Eo *obj EINA_UNUSED,
                                                          Efl_Canvas_Vg_Gradient_Linear_Data *pd,
                                                          double *x, double *y)
{
   if (x) *x = pd->start.x;
   if (y) *y = pd->start.y;
}

/**
 * @brief Sets the end point of the linear gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object.
 * @param[in,out] pd The private data of the object.
 * @param[in] x The x-coordinate of the end point.
 * @param[in] y The y-coordinate of the end point.
 */
static void
_efl_canvas_vg_gradient_linear_efl_gfx_gradient_linear_end_set(Eo *obj EINA_UNUSED,
                                                        Efl_Canvas_Vg_Gradient_Linear_Data *pd,
                                                        double x, double y)
{
   pd->end.x = x;
   pd->end.y = y;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the end point of the linear gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object.
 * @param[in] pd The private data of the object.
 * @param[out] x Pointer to store the x-coordinate of the end point. Can be NULL.
 * @param[out] y Pointer to store the y-coordinate of the end point. Can be NULL.
 */
static void
_efl_canvas_vg_gradient_linear_efl_gfx_gradient_linear_end_get(const Eo *obj EINA_UNUSED,
                                                        Efl_Canvas_Vg_Gradient_Linear_Data *pd,
                                                        double *x, double *y)
{
   if (x) *x = pd->end.x;
   if (y) *y = pd->end.y;
}

/**
 * @brief Pre-render setup for the linear gradient.
 *
 * This function is called before rendering the VG node. It sets up the
 * Ector renderer with the gradient's properties, including its
 * transformation, colors, spread method, and linear gradient specific
 * start and end points.
 *
 * @param[in] vg_pd Evas object protected data (unused).
 * @param[in] obj The Efl_VG object being rendered.
 * @param[in,out] nd The node data for the VG object.
 * @param[in] engine Rendering engine (unused).
 * @param[in] output Output target (unused).
 * @param[in] context Rendering context (unused).
 * @param[in] surface The Ector surface to render on.
 * @param[in] ptransform The parent's transformation matrix.
 * @param[in] p_opacity Parent's opacity (unused).
 * @param[in] comp Composition buffer for Ector.
 * @param[in] comp_method Composition method.
 * @param[in] data Custom data, which is Efl_Canvas_Vg_Gradient_Linear_Data.
 */
static void
_efl_canvas_vg_gradient_linear_render_pre(Evas_Object_Protected_Data *vg_pd EINA_UNUSED,
                                          Efl_VG *obj,
                                          Efl_Canvas_Vg_Node_Data *nd,
                                          void *engine EINA_UNUSED,
                                          void *output EINA_UNUSED,
                                          void *context EINA_UNUSED,
                                          Ector_Surface *surface,
                                          Eina_Matrix3 *ptransform,
                                          int p_opacity EINA_UNUSED,
                                          Ector_Buffer *comp,
                                          Efl_Gfx_Vg_Composite_Method comp_method,
                                          void *data)
{
   Efl_Canvas_Vg_Gradient_Linear_Data *pd = data;
   Efl_Canvas_Vg_Gradient_Data *gd;

   if (nd->flags == EFL_GFX_CHANGE_FLAG_NONE) return;

   nd->flags = EFL_GFX_CHANGE_FLAG_NONE;

   gd = efl_data_scope_get(obj, EFL_CANVAS_VG_GRADIENT_CLASS);
   EFL_CANVAS_VG_COMPUTE_MATRIX(ctransform, ptransform, nd);

   if (!nd->renderer)
     {
        efl_domain_current_push(EFL_ID_DOMAIN_SHARED);
        nd->renderer = ector_surface_renderer_factory_new(surface, ECTOR_RENDERER_GRADIENT_LINEAR_MIXIN);
        efl_domain_current_pop();
     }

   ector_renderer_transformation_set(nd->renderer, ctransform);
   ector_renderer_origin_set(nd->renderer, nd->x, nd->y);
   ector_renderer_visibility_set(nd->renderer, nd->visibility);
   efl_gfx_gradient_stop_set(nd->renderer, gd->colors, gd->colors_count);
   efl_gfx_gradient_spread_set(nd->renderer, gd->spread);
   efl_gfx_gradient_linear_start_set(nd->renderer, pd->start.x, pd->start.y);
   efl_gfx_gradient_linear_end_set(nd->renderer, pd->end.x, pd->end.y);
   ector_renderer_prepare(nd->renderer);
   ector_renderer_comp_method_set(nd->renderer, comp, comp_method);
}

/**
 * @brief Constructor for Efl_Canvas_Vg_Gradient_Linear objects.
 *
 * Initializes the object, sets up its private data, and assigns the
 * pre-render function.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object being constructed.
 * @param[in] pd The private data for the object.
 * @return The constructed Eo object.
 */
static Eo *
_efl_canvas_vg_gradient_linear_efl_object_constructor(Eo *obj,
                                            Efl_Canvas_Vg_Gradient_Linear_Data *pd)
{
   Efl_Canvas_Vg_Node_Data *nd;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   nd = efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS);
   nd->render_pre = _efl_canvas_vg_gradient_linear_render_pre;
   nd->data = pd;

   return obj;
}

/**
 * @brief Destructor for Efl_Canvas_Vg_Gradient_Linear objects.
 *
 * Cleans up resources used by the object.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object being destructed.
 * @param[in] pd The private data of the object (unused).
 */
static void
_efl_canvas_vg_gradient_linear_efl_object_destructor(Eo *obj, Efl_Canvas_Vg_Gradient_Linear_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets the bounding box of the linear gradient.
 *
 * The bounds are defined by the start and end points of the gradient.
 * Note: This calculates a rectangle where the width and height are
 * (end.x - start.x) and (end.y - start.y) respectively. This might not
 * represent the visual extent if the gradient line is not axis-aligned
 * or if spread methods extend the gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object.
 * @param[in] pd The private data of the object.
 * @param[out] r The Eina_Rect structure to store the bounds.
 */
static void
_efl_canvas_vg_gradient_linear_efl_gfx_path_bounds_get(const Eo *obj, Efl_Canvas_Vg_Gradient_Linear_Data *pd, Eina_Rect *r)
{
   Efl_Canvas_Vg_Node_Data *nd;

   nd = efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS);
   EINA_RECTANGLE_SET(r,
                      nd->x + pd->start.x, nd->y + pd->start.y,
                      pd->end.x - pd->start.x, pd->end.y - pd->start.x);
}

/**
 * @brief Interpolates between two linear gradients.
 *
 * This function linearly interpolates the start and end points of the gradient
 * based on the `pos_map` value. It also calls the superclass's interpolate
 * function to handle common gradient properties (like colors).
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object to store the interpolated result.
 * @param[in,out] pd The private data of the target object.
 * @param[in] from The source Efl_VG gradient to interpolate from.
 * @param[in] to The destination Efl_VG gradient to interpolate to.
 * @param[in] pos_map The interpolation factor (0.0 to 1.0).
 *                    0.0 means `from` gradient, 1.0 means `to` gradient.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_canvas_vg_gradient_linear_efl_gfx_path_interpolate(Eo *obj,
                                                Efl_Canvas_Vg_Gradient_Linear_Data *pd,
                                                const Efl_VG *from, const Efl_VG *to,
                                                double pos_map)
{
   Efl_Canvas_Vg_Gradient_Linear_Data *fromd, *tod;
   double from_map;
   Eina_Bool r;

   r = efl_gfx_path_interpolate(efl_super(obj, EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS), from, to, pos_map);

   if (!r) return EINA_FALSE;

   fromd = efl_data_scope_get(from, EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS);
   tod = efl_data_scope_get(to, EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS);
   from_map = 1.0 - pos_map;

#define INTP(Pd, From, To, Member, From_Map, Pos_Map)   \
   Pd->Member = From->Member * From_Map + To->Member * Pos_Map

   INTP(pd, fromd, tod, start.x, from_map, pos_map);
   INTP(pd, fromd, tod, start.y, from_map, pos_map);
   INTP(pd, fromd, tod, end.x, from_map, pos_map);
   INTP(pd, fromd, tod, end.y, from_map, pos_map);

#undef INTP

   return EINA_TRUE;
}

/**
 * @brief Duplicates an Efl_Canvas_Vg_Gradient_Linear object.
 *
 * Creates a new linear gradient object that is a copy of the original,
 * including its start and end points.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Linear object to duplicate.
 * @param[in] pd The private data of the object being duplicated.
 * @return A new Efl_VG object that is a duplicate of @p obj, or NULL on failure.
 */
EOLIAN static Efl_VG *
_efl_canvas_vg_gradient_linear_efl_duplicate_duplicate(const Eo *obj, Efl_Canvas_Vg_Gradient_Linear_Data *pd)
{
   Efl_VG *cn = NULL;

   cn = efl_duplicate(efl_super(obj, MY_CLASS));
   efl_gfx_gradient_linear_start_set(cn, pd->start.x, pd->start.y);
   efl_gfx_gradient_linear_end_set(cn, pd->end.x, pd->end.y);
   return cn;
}

/**
 * @brief Sets the start point of a linear gradient (legacy Evas API).
 * @param obj The gradient object.
 * @param x The x-coordinate of the start point.
 * @param y The y-coordinate of the start point.
 * @ingroup Evas_Vg_Gradient_Linear_Group
 */
EVAS_API void
evas_vg_gradient_linear_start_set(Evas_Vg_Gradient_Linear *obj, double x, double y)
{
   efl_gfx_gradient_linear_start_set(obj, x, y);
}

/**
 * @brief Gets the start point of a linear gradient (legacy Evas API).
 * @param obj The gradient object.
 * @param x Pointer to store the x-coordinate.
 * @param y Pointer to store the y-coordinate.
 * @ingroup Evas_Vg_Gradient_Linear_Group
 */
EVAS_API void
evas_vg_gradient_linear_start_get(Evas_Vg_Gradient_Linear *obj, double *x, double *y)
{
   efl_gfx_gradient_linear_start_get(obj, x, y);
}

/**
 * @brief Sets the end point of a linear gradient (legacy Evas API).
 * @param obj The gradient object.
 * @param x The x-coordinate of the end point.
 * @param y The y-coordinate of the end point.
 * @ingroup Evas_Vg_Gradient_Linear_Group
 */
EVAS_API void
evas_vg_gradient_linear_end_set(Evas_Vg_Gradient_Linear *obj, double x, double y)
{
   efl_gfx_gradient_linear_end_set(obj, x, y);
}

/**
 * @brief Gets the end point of a linear gradient (legacy Evas API).
 * @param obj The gradient object.
 * @param x Pointer to store the x-coordinate.
 * @param y Pointer to store the y-coordinate.
 * @ingroup Evas_Vg_Gradient_Linear_Group
 */
EVAS_API void
evas_vg_gradient_linear_end_get(Evas_Vg_Gradient_Linear *obj, double *x, double *y)
{
   efl_gfx_gradient_linear_end_get(obj, x, y);
}

/**
 * @brief Adds a new linear gradient object as a child of a VG container (legacy Evas API).
 * @param parent The parent VG container.
 * @return The new linear gradient object, or NULL on failure.
 * @ingroup Evas_Vg_Gradient_Linear_Group
 */
EVAS_API Evas_Vg_Gradient_Linear *
evas_vg_gradient_linear_add(Evas_Vg_Container *parent)
{
   return efl_add(EFL_CANVAS_VG_GRADIENT_LINEAR_CLASS, parent);
}

#include "efl_canvas_vg_gradient_linear.eo.c"
