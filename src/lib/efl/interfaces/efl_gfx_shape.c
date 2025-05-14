#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <math.h>
#include <float.h>
#include <ctype.h>

#include <Efl.h>

#define ERR(...) EINA_LOG_DOM_ERR(EINA_LOG_DOMAIN_DEFAULT, __VA_ARGS__)

#define MY_CLASS EFL_GFX_SHAPE_MIXIN

/**
 * @brief Private data structure for Efl_Gfx_Shape.
 *
 * This structure holds all the private data members for an Efl_Gfx_Shape object,
 * including its public properties and fill rule.
 */
typedef struct _Efl_Gfx_Shape_Data
{
   Efl_Gfx_Shape_Public public; /**< Public shape properties, like stroke and color. */
   Efl_Gfx_Fill_Rule fill_rule; /**< The fill rule used for rendering the shape. */
} Efl_Gfx_Shape_Data;

/**
 * @brief Interpolates between two double values.
 *
 * @param from The starting value.
 * @param to The ending value.
 * @param pos_map The position map, a value between 0.0 and 1.0.
 * @return The interpolated double value.
 */
static inline double
interpolate(double from, double to, double pos_map)
{
   return (from * (1.0 - pos_map)) + (to * pos_map);
}

/**
 * @brief Interpolates between two integer values.
 *
 * @param from The starting integer value.
 * @param to The ending integer value.
 * @param pos_map The position map, a value between 0.0 and 1.0.
 * @return The interpolated integer value.
 */
static inline int
interpolatei(int from, int to, double pos_map)
{
   return (from * (1.0 - pos_map)) + (to * pos_map);
}

/**
 * @brief Structure to hold a snapshot of graphics properties.
 *
 * This is used during interpolation to store the state of 'from' and 'to' objects.
 */
typedef struct _Efl_Gfx_Property Efl_Gfx_Property;
struct _Efl_Gfx_Property
{
   double scale;                 /**< Stroke scale factor. */
   double w;                     /**< Stroke width. */
   double centered;              /**< Stroke location (0.0 for outset, 0.5 for centered, 1.0 for inset). */
   double miterlimit;            /**< Stroke miter limit. */

   Efl_Gfx_Cap c;                 /**< Stroke cap style. */
   Efl_Gfx_Join j;                /**< Stroke join style. */

   const Efl_Gfx_Dash *dash;      /**< Pointer to an array of dash structures.
                                   * Example: Efl_Gfx_Dash dash[] = { { .length = 5.0, .gap = 2.0 }, { .length = 1.0, .gap = 1.0 } }; */
   unsigned int dash_length;     /**< Number of elements in the dash array. */

   int r, g, b, a;              /**< Stroke color components (red, green, blue, alpha). */
};

/**
 * @brief Retrieves all relevant graphics properties from an Eo object.
 *
 * @param obj The Efl_Gfx_Shape object to get properties from.
 * @param property Pointer to an Efl_Gfx_Property struct to fill.
 */
static inline void
_efl_gfx_property_get(const Eo *obj, Efl_Gfx_Property *property)
{
   property->scale = efl_gfx_shape_stroke_scale_get(obj);
   efl_gfx_shape_stroke_color_get(obj, &property->r, &property->g,
                                  &property->b, &property->a);
   property->w = efl_gfx_shape_stroke_width_get(obj);
   property->centered = efl_gfx_shape_stroke_location_get(obj);
   efl_gfx_shape_stroke_dash_get(obj, &property->dash, &property->dash_length);
   property->c = efl_gfx_shape_stroke_cap_get(obj);
   property->j = efl_gfx_shape_stroke_join_get(obj);
   property->miterlimit = efl_gfx_shape_stroke_miterlimit_get(obj);
}

/**
 * @brief Interpolates the graphical properties of a shape between two other shapes.
 *
 * This function handles the interpolation of stroke properties (scale, color, width,
 * location, dash pattern, cap, join, miter limit) and then calls the parent class's
 * interpolate function for path data.
 *
 * @param obj The target Efl_Gfx_Shape object to apply interpolated values to.
 * @param pd Private data of the target object.
 * @param from The source Efl_Gfx_Shape object to interpolate from.
 * @param to The destination Efl_Gfx_Shape object to interpolate to.
 * @param pos_map A value between 0.0 and 1.0 indicating the interpolation position.
 *                0.0 means 'from' state, 1.0 means 'to' state.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., incompatible types,
 *         different dash lengths, memory allocation failure).
 */
EOLIAN static Eina_Bool
_efl_gfx_shape_efl_gfx_path_interpolate(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                        const Eo *from, const Eo *to,
                                        double pos_map)
{
   Efl_Gfx_Shape_Data *from_pd, *to_pd;
   Efl_Gfx_Property property_from, property_to;
   Efl_Gfx_Dash *dash = NULL;
   double interv;    //interpolated value
   unsigned int i;

   if (!efl_isa(from, EFL_GFX_SHAPE_MIXIN) || !efl_isa(to, EFL_GFX_SHAPE_MIXIN))
     return EINA_FALSE;

   from_pd = efl_data_scope_get(from, EFL_GFX_SHAPE_MIXIN);
   to_pd = efl_data_scope_get(to, EFL_GFX_SHAPE_MIXIN);

   if ((pd == from_pd) || (pd == to_pd)) return EINA_FALSE;

   _efl_gfx_property_get(from, &property_from);
   _efl_gfx_property_get(to, &property_to);

   //Can be interpolated!
   if (property_from.dash_length != property_to.dash_length)
     return EINA_FALSE;

   if (property_to.dash_length)
     {
        dash = malloc(sizeof (Efl_Gfx_Dash) * property_to.dash_length);
        if (!dash) return EINA_FALSE;

        for (i = 0; i < property_to.dash_length; i++)
          {
             dash[i].length = interpolate(property_from.dash[i].length,
                                          property_to.dash[i].length, pos_map);
             dash[i].gap = interpolate(property_from.dash[i].gap,
                                       property_to.dash[i].gap, pos_map);
          }
     }

   interv = interpolate(property_from.scale, property_to.scale, pos_map);
   efl_gfx_shape_stroke_scale_set(obj, interv);

   efl_gfx_shape_stroke_color_set(obj,
                                  interpolatei(property_from.r, property_to.r,
                                               pos_map),
                                  interpolatei(property_from.g, property_to.g,
                                               pos_map),
                                  interpolatei(property_from.b, property_to.b,
                                               pos_map),
                                  interpolatei(property_from.a, property_to.a,
                                               pos_map));
   interv = interpolate(property_from.w, property_to.w, pos_map);
   efl_gfx_shape_stroke_width_set(obj, interv);

   interv = interpolate(property_from.centered, property_to.centered, pos_map);
   efl_gfx_shape_stroke_location_set(obj, interv);

   interv = interpolate(property_from.miterlimit, property_to.miterlimit, pos_map);
   efl_gfx_shape_stroke_miterlimit_set(obj, interv);

   efl_gfx_shape_stroke_dash_set(obj, dash, property_to.dash_length);
   efl_gfx_shape_stroke_cap_set(obj, (pos_map < 0.5) ?
                                property_from.c : property_to.c);
   efl_gfx_shape_stroke_join_set(obj, (pos_map < 0.5) ?
                                 property_from.j : property_to.j);

   return efl_gfx_path_interpolate(efl_cast(obj, EFL_GFX_PATH_MIXIN),
                                   from, to, pos_map);
}

/**
 * @brief Sets the stroke scale factor for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param s The scale factor to set.
 */
EOLIAN static void
_efl_gfx_shape_stroke_scale_set(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                                double s)
{
   pd->public.stroke.scale = s;
}

/**
 * @brief Gets the stroke scale factor of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke scale factor.
 */
EOLIAN static double
_efl_gfx_shape_stroke_scale_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.scale;
}

/**
 * @brief Sets the stroke color for the shape.
 *
 * Colors are premultiplied (0 <= R,G,B <= A <= 255).
 * Values outside this range will be clamped and an error logged.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
EOLIAN static void
_efl_gfx_shape_stroke_color_set(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                                int r, int g, int b, int a)
{
   Eina_Bool err = EINA_FALSE;
   if (a > 255) { a = 255; err = EINA_TRUE; }
   if (a < 0) { a = 0; err = EINA_TRUE; }
   if (r > a) { r = a; err = EINA_TRUE; }
   if (r < 0) { r = 0; err = EINA_TRUE; }
   if (g > a) { g = a; err = EINA_TRUE; }
   if (g < 0) { g = 0; err = EINA_TRUE; }
   if (b > a) { b = a; err = EINA_TRUE; }
   if (b < 0) { b = 0; err = EINA_TRUE; }
   if (err)
     ERR("Only handles premultiplied colors (0 <= R,G,B <= A <= 255)");
   pd->public.stroke.color.r = r;
   pd->public.stroke.color.g = g;
   pd->public.stroke.color.b = b;
   pd->public.stroke.color.a = a;
}

/**
 * @brief Gets the stroke color of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param r Pointer to store the red component.
 * @param g Pointer to store the green component.
 * @param b Pointer to store the blue component.
 * @param a Pointer to store the alpha component.
 */
EOLIAN static void
_efl_gfx_shape_stroke_color_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                                int *r, int *g, int *b, int *a)
{
   if (r) *r = pd->public.stroke.color.r;
   if (g) *g = pd->public.stroke.color.g;
   if (b) *b = pd->public.stroke.color.b;
   if (a) *a = pd->public.stroke.color.a;
}

/**
 * @brief Sets the stroke width for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param w The stroke width.
 */
EOLIAN static void
_efl_gfx_shape_stroke_width_set(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd, double w)
{
   pd->public.stroke.width = w;
}

/**
 * @brief Gets the stroke width of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke width.
 */
EOLIAN static double
_efl_gfx_shape_stroke_width_get(const Eo *obj EINA_UNUSED,
                                Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.width;
}

/**
 * @brief Sets the stroke location for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param centered The stroke location (0.0 for outset, 0.5 for centered, 1.0 for inset).
 */
EOLIAN static void
_efl_gfx_shape_stroke_location_set(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                                   double centered)
{
   pd->public.stroke.centered = centered;
}

/**
 * @brief Gets the stroke location of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke location.
 */
EOLIAN static double
_efl_gfx_shape_stroke_location_get(const Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.centered;
}

/**
 * @brief Sets the stroke dash pattern for the shape.
 *
 * The dash pattern is defined by an array of Efl_Gfx_Dash structures,
 * where each structure specifies a length and a gap.
 * Example: Efl_Gfx_Dash dash_pattern[] = { { .length = 10.0, .gap = 5.0 }, { .length = 2.0, .gap = 2.0 } };
 * This would create a pattern of a 10-unit dash, a 5-unit gap, a 2-unit dash, and a 2-unit gap.
 *
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param dash Pointer to an array of Efl_Gfx_Dash structures. If NULL, the dash pattern is cleared.
 * @param length The number of elements in the dash array.
 */
EOLIAN static void
_efl_gfx_shape_stroke_dash_set(Eo *obj EINA_UNUSED, Efl_Gfx_Shape_Data *pd,
                               const Efl_Gfx_Dash *dash, unsigned int length)
{
   Efl_Gfx_Dash *tmp;

   if (!dash)
     {
        free(pd->public.stroke.dash);
        pd->public.stroke.dash = NULL;
        pd->public.stroke.dash_length = 0;
        return;
     }

   tmp = realloc(pd->public.stroke.dash, length * sizeof (Efl_Gfx_Dash));
   if (!tmp && length) return;
   memcpy(tmp, dash, length * sizeof (Efl_Gfx_Dash));

   pd->public.stroke.dash = tmp;
   pd->public.stroke.dash_length = length;
}

/**
 * @brief Gets the stroke dash pattern of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param dash Pointer to store the address of the internal dash array.
 *             The returned array should not be modified by the caller.
 *             Example: const Efl_Gfx_Dash *my_dash; unsigned int len;
 *                      _efl_gfx_shape_stroke_dash_get(obj, pd, &my_dash, &len);
 *                      // my_dash[0].length, my_dash[0].gap etc.
 * @param length Pointer to store the number of elements in the dash array.
 */
EOLIAN static void
_efl_gfx_shape_stroke_dash_get(const Eo *obj EINA_UNUSED,
                               Efl_Gfx_Shape_Data *pd,
                               const Efl_Gfx_Dash **dash, unsigned int *length)
{
   if (dash) *dash = pd->public.stroke.dash;
   if (length) *length = pd->public.stroke.dash_length;
}

/**
 * @brief Sets the stroke cap style for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param c The cap style (e.g., EFL_GFX_CAP_BUTT, EFL_GFX_CAP_ROUND, EFL_GFX_CAP_SQUARE).
 */
EOLIAN static void
_efl_gfx_shape_stroke_cap_set(Eo *obj EINA_UNUSED,
                              Efl_Gfx_Shape_Data *pd,
                              Efl_Gfx_Cap c)
{
   pd->public.stroke.cap = c;
}

/**
 * @brief Gets the stroke cap style of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke cap style.
 */
EOLIAN static Efl_Gfx_Cap
_efl_gfx_shape_stroke_cap_get(const Eo *obj EINA_UNUSED,
                              Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.cap;
}

/**
 * @brief Sets the stroke join style for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param j The join style (e.g., EFL_GFX_JOIN_MITER, EFL_GFX_JOIN_ROUND, EFL_GFX_JOIN_BEVEL).
 */
EOLIAN static void
_efl_gfx_shape_stroke_join_set(Eo *obj EINA_UNUSED,
                               Efl_Gfx_Shape_Data *pd,
                               Efl_Gfx_Join j)
{
   pd->public.stroke.join = j;
}

/**
 * @brief Gets the stroke join style of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke join style.
 */
EOLIAN static Efl_Gfx_Join
_efl_gfx_shape_stroke_join_get(const Eo *obj EINA_UNUSED,
                               Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.join;
}

/**
 * @brief Sets the fill rule for the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param fill_rule The fill rule (e.g., EFL_GFX_FILL_RULE_WINDING, EFL_GFX_FILL_RULE_ODD_EVEN).
 */
EOLIAN static void
_efl_gfx_shape_fill_rule_set(Eo *obj EINA_UNUSED,
                             Efl_Gfx_Shape_Data *pd,
                             Efl_Gfx_Fill_Rule fill_rule)
{
   pd->fill_rule = fill_rule;
}

/**
 * @brief Gets the fill rule of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current fill rule.
 */
EOLIAN static Efl_Gfx_Fill_Rule
_efl_gfx_shape_fill_rule_get(const Eo *obj EINA_UNUSED,
                             Efl_Gfx_Shape_Data *pd)
{
   return pd->fill_rule;
}

/**
 * @brief Copies all graphical properties from another Efl_Gfx_Shape object.
 *
 * This includes stroke properties (scale, width, location, cap, join, color,
 * miter limit, dash pattern) and the fill rule. After copying these shape-specific
 * properties, it calls the parent class's copy_from function to handle path data.
 *
 * @param obj The target Efl_Gfx_Shape object.
 * @param pd Private data of the target object.
 * @param dup_from The source Efl_Gfx_Shape object to copy properties from.
 */
EOLIAN static void
_efl_gfx_shape_efl_gfx_path_copy_from(Eo *obj, Efl_Gfx_Shape_Data *pd,
                                      const Eo *dup_from)
{
   Efl_Gfx_Shape_Data *from;

   if (obj == dup_from) return;

   from = efl_data_scope_get(dup_from, MY_CLASS);
   if (!from) return;

   pd->public.stroke.scale = from->public.stroke.scale;
   pd->public.stroke.width = from->public.stroke.width;
   pd->public.stroke.centered = from->public.stroke.centered;
   pd->public.stroke.cap = from->public.stroke.cap;
   pd->public.stroke.join = from->public.stroke.join;
   pd->public.stroke.color.r = from->public.stroke.color.r;
   pd->public.stroke.color.g = from->public.stroke.color.g;
   pd->public.stroke.color.b = from->public.stroke.color.b;
   pd->public.stroke.color.a = from->public.stroke.color.a;
   pd->public.stroke.miterlimit = from->public.stroke.miterlimit;
   pd->fill_rule = from->fill_rule;

   _efl_gfx_shape_stroke_dash_set(obj, pd, from->public.stroke.dash,
                                  from->public.stroke.dash_length);

   efl_gfx_path_copy_from(efl_super(obj, MY_CLASS), dup_from);
}

/**
 * @brief Sets the stroke miter limit for the shape.
 *
 * The miter limit is the maximum ratio of miter length to stroke width.
 * If the miter length exceeds this limit, a bevel join will be used instead of a miter join.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @param miterlimit The miter limit value.
 */
EOLIAN static void
_efl_gfx_shape_stroke_miterlimit_set(Eo *obj EINA_UNUSED,
                                     Efl_Gfx_Shape_Data *pd,
                                     double miterlimit)
{
   pd->public.stroke.miterlimit = miterlimit;
}

/**
 * @brief Gets the stroke miter limit of the shape.
 * @param obj The Efl_Gfx_Shape object.
 * @param pd Private data of the object.
 * @return The current stroke miter limit.
 */
EOLIAN static double
_efl_gfx_shape_stroke_miterlimit_get(const Eo *obj EINA_UNUSED,
                                     Efl_Gfx_Shape_Data *pd)
{
   return pd->public.stroke.miterlimit;
}

#include "interfaces/efl_gfx_shape.eo.c"
