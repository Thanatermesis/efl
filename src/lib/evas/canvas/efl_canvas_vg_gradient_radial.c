#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#define MY_CLASS EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS

/**
 * @brief Private data for the Efl_Canvas_Vg_Gradient_Radial class.
 *
 * This structure holds the specific properties of a radial gradient,
 * including its center, focal point, and radius.
 */
typedef struct _Efl_Canvas_Vg_Gradient_Radial_Data Efl_Canvas_Vg_Gradient_Radial_Data;
struct _Efl_Canvas_Vg_Gradient_Radial_Data
{
   struct {
      double x, y; /**< Coordinates of the point. */
   } center, focal; /**< Center and focal points of the radial gradient. The focal point determines the origin of the gradient rays. */
   double radius; /**< Radius of the radial gradient. */
};

/**
 * @brief Sets the center of the radial gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in,out] pd The private data of the object.
 * @param[in] x The x-coordinate of the center.
 * @param[in] y The y-coordinate of the center.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_center_set(Eo *obj EINA_UNUSED,
                                                           Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                           double x, double y)
{
   pd->center.x = x;
   pd->center.y = y;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the center of the radial gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in] pd The private data of the object.
 * @param[out] x Pointer to store the x-coordinate of the center.
 * @param[out] y Pointer to store the y-coordinate of the center.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_center_get(const Eo *obj EINA_UNUSED,
                                                           Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                           double *x, double *y)
{
   if (x) *x = pd->center.x;
   if (y) *y = pd->center.y;
}

/**
 * @brief Sets the radius of the radial gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in,out] pd The private data of the object.
 * @param[in] r The radius value.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_radius_set(Eo *obj EINA_UNUSED,
                                                           Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                           double r)
{
   pd->radius = r;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the radius of the radial gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in] pd The private data of the object.
 * @return The radius value.
 */
static double
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_radius_get(const Eo *obj EINA_UNUSED,
                                                           Efl_Canvas_Vg_Gradient_Radial_Data *pd)
{
   return pd->radius;
}

/**
 * @brief Sets the focal point of the radial gradient.
 *
 * The focal point determines the origin of the gradient rays.
 * If the focal point is the same as the center, the gradient is circular.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in,out] pd The private data of the object.
 * @param[in] x The x-coordinate of the focal point.
 * @param[in] y The y-coordinate of the focal point.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_focal_set(Eo *obj EINA_UNUSED,
                                                          Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                          double x, double y)
{
   pd->focal.x = x;
   pd->focal.y = y;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the focal point of the radial gradient.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in] pd The private data of the object.
 * @param[out] x Pointer to store the x-coordinate of the focal point.
 * @param[out] y Pointer to store the y-coordinate of the focal point.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_gradient_radial_focal_get(const Eo *obj EINA_UNUSED,
                                                          Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                          double *x, double *y)
{
   if (x) *x = pd->focal.x;
   if (y) *y = pd->focal.y;
}

/**
 * @brief Pre-render setup for the radial gradient.
 *
 * This function is called before rendering the gradient. It sets up the
 * renderer with the gradient's properties like transformation, colors,
 * spread method, center, focal point, and radius.
 *
 * @param[in] vg_pd Evas object protected data (unused).
 * @param[in] obj The Efl_VG object (gradient itself).
 * @param[in,out] nd The node data for the Efl_Canvas_Vg_Node.
 * @param[in] engine The rendering engine (unused).
 * @param[in] output The rendering output (unused).
 * @param[in] context The rendering context (unused).
 * @param[in] surface The Ector surface to render on.
 * @param[in] ptransform The parent transformation matrix.
 * @param[in] p_opacity Parent opacity (unused).
 * @param[in] comp The Ector buffer for composition.
 * @param[in] comp_method The composition method.
 * @param[in] data The private data of the radial gradient (Efl_Canvas_Vg_Gradient_Radial_Data).
 */
static void
_efl_canvas_vg_gradient_radial_render_pre(Evas_Object_Protected_Data *vg_pd EINA_UNUSED,
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
   Efl_Canvas_Vg_Gradient_Radial_Data *pd = data;
   Efl_Canvas_Vg_Gradient_Data *gd;

   if (nd->flags == EFL_GFX_CHANGE_FLAG_NONE) return;

   nd->flags = EFL_GFX_CHANGE_FLAG_NONE;

   gd = efl_data_scope_get(obj, EFL_CANVAS_VG_GRADIENT_CLASS);
   EFL_CANVAS_VG_COMPUTE_MATRIX(ctransform, ptransform, nd);

   if (!nd->renderer)
     {
        efl_domain_current_push(EFL_ID_DOMAIN_SHARED);
        nd->renderer = ector_surface_renderer_factory_new(surface, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN);
        efl_domain_current_pop();
     }

   ector_renderer_transformation_set(nd->renderer, ctransform);
   ector_renderer_origin_set(nd->renderer, nd->x, nd->y);
   ector_renderer_visibility_set(nd->renderer, nd->visibility);
   efl_gfx_gradient_stop_set(nd->renderer, gd->colors, gd->colors_count);
   efl_gfx_gradient_spread_set(nd->renderer, gd->spread);
   efl_gfx_gradient_radial_center_set(nd->renderer, pd->center.x, pd->center.y);
   efl_gfx_gradient_radial_focal_set(nd->renderer, pd->focal.x, pd->focal.y);
   efl_gfx_gradient_radial_radius_set(nd->renderer, pd->radius);
   ector_renderer_prepare(nd->renderer);
   ector_renderer_comp_method_set(nd->renderer, comp, comp_method);
}

/**
 * @brief Constructor for the Efl_Canvas_Vg_Gradient_Radial object.
 *
 * Initializes the radial gradient object, setting up its node data
 * and render_pre function.
 *
 * @param[in] obj The Eo object to construct.
 * @param[in] pd The private data for the radial gradient.
 * @return The constructed Eo object.
 */
static Eo *
_efl_canvas_vg_gradient_radial_efl_object_constructor(Eo *obj, Efl_Canvas_Vg_Gradient_Radial_Data *pd)
{
   Efl_Canvas_Vg_Node_Data *nd;

   obj = efl_constructor(efl_super(obj, MY_CLASS));

   nd = efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS);
   nd->render_pre = _efl_canvas_vg_gradient_radial_render_pre;
   nd->data = pd;

   return obj;
}

/**
 * @brief Destructor for the Efl_Canvas_Vg_Gradient_Radial object.
 *
 * Cleans up resources used by the radial gradient object.
 *
 * @param[in] obj The Eo object to destruct.
 * @param[in] pd The private data of the radial gradient (unused).
 */
static void
_efl_canvas_vg_gradient_radial_efl_object_destructor(Eo *obj,
                                           Efl_Canvas_Vg_Gradient_Radial_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets the bounding box of the radial gradient.
 *
 * The bounds are calculated based on the gradient's center and radius.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object.
 * @param[in] pd The private data of the radial gradient.
 * @param[out] r Pointer to an Eina_Rect to store the bounding box.
 */
static void
_efl_canvas_vg_gradient_radial_efl_gfx_path_bounds_get(const Eo *obj, Efl_Canvas_Vg_Gradient_Radial_Data *pd, Eina_Rect *r)
{
   Efl_Canvas_Vg_Node_Data *nd;

   nd = efl_data_scope_get(obj, EFL_CANVAS_VG_NODE_CLASS);
   EINA_RECTANGLE_SET(r,
                      nd->x + pd->center.x - pd->radius,
                      nd->y + pd->center.y - pd->radius,
                      pd->radius * 2, pd->radius * 2);
}

/**
 * @brief Interpolates the properties of the radial gradient.
 *
 * This function is used for animations, calculating intermediate states
 * of the gradient's focal point, center, and radius between a 'from'
 * and 'to' state based on a position map value (pos_map).
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object being interpolated.
 * @param[out] pd The private data of the object to store interpolated values.
 * @param[in] from The starting Efl_VG gradient state.
 * @param[in] to The ending Efl_VG gradient state.
 * @param[in] pos_map A value between 0.0 and 1.0 indicating the interpolation position.
 *                    0.0 means the state is identical to 'from', 1.0 means 'to'.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_canvas_vg_gradient_radial_efl_gfx_path_interpolate(Eo *obj,
                                                Efl_Canvas_Vg_Gradient_Radial_Data *pd,
                                                const Efl_VG *from, const Efl_VG *to,
                                                double pos_map)
{
   Efl_Canvas_Vg_Gradient_Radial_Data *fromd, *tod;
   double from_map;
   Eina_Bool r;

   r = efl_gfx_path_interpolate(efl_super(obj, EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS), from, to, pos_map);

   if (!r) return EINA_FALSE;

   fromd = efl_data_scope_get(from, EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS);
   tod = efl_data_scope_get(to, EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS);
   from_map = 1.0 - pos_map;

#define INTP(Pd, From, To, Member, From_Map, Pos_Map)   \
   Pd->Member = From->Member * From_Map + To->Member * Pos_Map

   INTP(pd, fromd, tod, focal.x, from_map, pos_map);
   INTP(pd, fromd, tod, focal.y, from_map, pos_map);
   INTP(pd, fromd, tod, center.x, from_map, pos_map);
   INTP(pd, fromd, tod, center.y, from_map, pos_map);
   INTP(pd, fromd, tod, radius, from_map, pos_map);

#undef INTP

   return EINA_TRUE;
}

/**
 * @brief Duplicates the Efl_Canvas_Vg_Gradient_Radial object.
 *
 * Creates a new radial gradient object that is a copy of the original.
 * The focal point, center, and radius are copied to the new object.
 *
 * @param[in] obj The Efl_Canvas_Vg_Gradient_Radial object to duplicate.
 * @param[in] pd The private data of the object to duplicate.
 * @return A new Efl_VG object that is a duplicate of obj, or NULL on failure.
 */
EOLIAN static Efl_VG *
_efl_canvas_vg_gradient_radial_efl_duplicate_duplicate(const Eo *obj, Efl_Canvas_Vg_Gradient_Radial_Data *pd)

{
   Efl_VG *cn = NULL;

   cn = efl_duplicate(efl_super(obj, MY_CLASS));
   efl_gfx_gradient_radial_focal_set(cn, pd->focal.x, pd->focal.y);
   efl_gfx_gradient_radial_center_set(cn, pd->center.x, pd->center.y);
   efl_gfx_gradient_radial_radius_set(cn, pd->radius);
   return cn;
}

/**
 * @brief Sets the center of a radial gradient.
 * @param obj The radial gradient object.
 * @param x The x-coordinate of the center.
 * @param y The y-coordinate of the center.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API void
evas_vg_gradient_radial_center_set(Evas_Vg_Gradient_Radial *obj, double x, double y)
{
   efl_gfx_gradient_radial_center_set(obj, x, y);
}

/**
 * @brief Gets the center of a radial gradient.
 * @param obj The radial gradient object.
 * @param x Pointer to store the x-coordinate of the center.
 * @param y Pointer to store the y-coordinate of the center.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API void
evas_vg_gradient_radial_center_get(Evas_Vg_Gradient_Radial *obj, double *x, double *y)
{
   efl_gfx_gradient_radial_center_get(obj, x, y);
}

/**
 * @brief Sets the radius of a radial gradient.
 * @param obj The radial gradient object.
 * @param r The radius value.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API void
evas_vg_gradient_radial_radius_set(Evas_Vg_Gradient_Radial *obj, double r)
{
   efl_gfx_gradient_radial_radius_set(obj, r);
}

/**
 * @brief Gets the radius of a radial gradient.
 * @param obj The radial gradient object.
 * @return The radius value.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API double
evas_vg_gradient_radial_radius_get(Evas_Vg_Gradient_Radial *obj)
{
   return efl_gfx_gradient_radial_radius_get(obj);
}

/**
 * @brief Sets the focal point of a radial gradient.
 * @param obj The radial gradient object.
 * @param x The x-coordinate of the focal point.
 * @param y The y-coordinate of the focal point.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API void
evas_vg_gradient_radial_focal_set(Evas_Vg_Gradient_Radial *obj, double x, double y)
{
   efl_gfx_gradient_radial_focal_set(obj, x, y);
}

/**
 * @brief Gets the focal point of a radial gradient.
 * @param obj The radial gradient object.
 * @param x Pointer to store the x-coordinate of the focal point.
 * @param y Pointer to store the y-coordinate of the focal point.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API void
evas_vg_gradient_radial_focal_get(Evas_Vg_Gradient_Radial *obj, double *x, double *y)
{
   efl_gfx_gradient_radial_focal_get(obj, x, y);
}

/**
 * @brief Adds a new radial gradient object as a child of the given Evas_Vg_Container.
 * @param parent The parent container object.
 * @return The new Evas_Vg_Gradient_Radial object, or NULL on failure.
 * @ingroup Evas_Vg_Gradient_Radial
 */
EVAS_API Evas_Vg_Gradient_Radial*
evas_vg_gradient_radial_add(Evas_Vg_Container *parent)
{
   return efl_add(EFL_CANVAS_VG_GRADIENT_RADIAL_CLASS, parent);
}

#include "efl_canvas_vg_gradient_radial.eo.c"
