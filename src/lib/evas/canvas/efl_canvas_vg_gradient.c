#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#include <strings.h>

#define MY_CLASS EFL_CANVAS_VG_GRADIENT_CLASS

/**
 * @brief Sets the color stops for the gradient.
 *
 * This function updates the gradient's color stops. Color stops define the colors
 * and their positions along the gradient.
 *
 * @param obj The Evas object.
 * @param pd Private data for the gradient.
 * @param colors An array of Efl_Gfx_Gradient_Stop structures.
 *               Each Efl_Gfx_Gradient_Stop consists of:
 *               - offset: double (0.0 to 1.0), position of the stop.
 *               - r, g, b, a: unsigned char (0-255), color components.
 *                 Example: { {0.0, 255, 0, 0, 255}, {1.0, 0, 0, 255, 255} }
 *                 This defines a gradient from red to blue.
 * @param length The number of color stops in the colors array.
 */
static void
_efl_canvas_vg_gradient_efl_gfx_gradient_stop_set(Eo *obj EINA_UNUSED,
                                                Efl_Canvas_Vg_Gradient_Data *pd,
                                                const Efl_Gfx_Gradient_Stop *colors,
                                                unsigned int length)
{
   pd->colors = realloc(pd->colors, length * sizeof(Efl_Gfx_Gradient_Stop));
   if (!pd->colors)
     {
        pd->colors_count = 0;
        return ;
     }

   memcpy(pd->colors, colors, length * sizeof(Efl_Gfx_Gradient_Stop));
   pd->colors_count = length;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the color stops for the gradient.
 *
 * Retrieves the array of color stops currently defined for the gradient.
 * The caller should not modify the returned array.
 *
 * @param obj The Evas object.
 * @param pd Private data for the gradient.
 * @param[out] colors Pointer to store the address of the Efl_Gfx_Gradient_Stop array.
 * @param[out] length Pointer to store the number of color stops.
 */
static void
_efl_canvas_vg_gradient_efl_gfx_gradient_stop_get(const Eo *obj EINA_UNUSED,
                                                Efl_Canvas_Vg_Gradient_Data *pd,
                                                const Efl_Gfx_Gradient_Stop **colors,
                                                unsigned int *length)
{
   if (colors) *colors = pd->colors;
   if (length) *length = pd->colors_count;
}

/**
 * @brief Sets the spread method for the gradient.
 *
 * The spread method defines how the gradient is rendered outside the
 * defined gradient area (e.g., repeat, reflect).
 *
 * @param obj The Evas object.
 * @param pd Private data for the gradient.
 * @param spread The Efl_Gfx_Gradient_Spread enum value.
 */
static void
_efl_canvas_vg_gradient_efl_gfx_gradient_spread_set(Eo *obj EINA_UNUSED,
                                                  Efl_Canvas_Vg_Gradient_Data *pd,
                                                  Efl_Gfx_Gradient_Spread spread)
{
   pd->spread = spread;

   efl_canvas_vg_node_change(obj);
}

/**
 * @brief Gets the spread method for the gradient.
 *
 * @param obj The Evas object.
 * @param pd Private data for the gradient.
 * @return The current Efl_Gfx_Gradient_Spread enum value.
 */
static Efl_Gfx_Gradient_Spread
_efl_canvas_vg_gradient_efl_gfx_gradient_spread_get(const Eo *obj EINA_UNUSED,
                                                  Efl_Canvas_Vg_Gradient_Data *pd)
{
   return pd->spread;
}

/**
 * @brief Interpolates between two gradients.
 *
 * This function calculates an intermediate gradient state between a 'from'
 * gradient and a 'to' gradient, based on the 'pos_map' value.
 * The result is stored in the 'obj' gradient.
 * It interpolates both the path properties (via superclass) and the
 * color stops.
 *
 * @param obj The target Evas object where the interpolated gradient is stored.
 * @param pd Private data for the target gradient.
 * @param from The source gradient to interpolate from.
 * @param to The source gradient to interpolate to.
 * @param pos_map The interpolation position (0.0 for 'from', 1.0 for 'to').
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., color stop counts differ).
 */
static Eina_Bool
_efl_canvas_vg_gradient_efl_gfx_path_interpolate(Eo *obj,
                                         Efl_Canvas_Vg_Gradient_Data *pd,
                                         const Efl_VG *from, const Efl_VG *to,
                                         double pos_map)
{
   Efl_Canvas_Vg_Gradient_Data *fromd, *tod;
   Efl_Gfx_Gradient_Stop *colors;
   unsigned int i;
   double from_map;
   Eina_Bool r;

   r = efl_gfx_path_interpolate(efl_super(obj, EFL_CANVAS_VG_GRADIENT_CLASS), from, to, pos_map);

   fromd = efl_data_scope_get(from, EFL_CANVAS_VG_GRADIENT_CLASS);
   tod = efl_data_scope_get(to, EFL_CANVAS_VG_GRADIENT_CLASS);
   from_map = 1.0 - pos_map;

   if (!r) return EINA_FALSE;
   if (fromd->colors_count != tod->colors_count) return EINA_FALSE;

   colors = realloc(pd->colors, sizeof (Efl_Gfx_Gradient_Stop) * tod->colors_count);
   if (!colors) return EINA_FALSE;

   pd->colors = colors;

   // Interpolate each color stop's offset and RGBA values.
#define INTP(Pd, From, To, I, Member, From_Map, Pos_Map)                \
   Pd->colors[I].Member = From->colors[I].Member * From_Map + To->colors[I].Member * Pos_Map

   for (i = 0; i < fromd->colors_count; i++)
     {
        INTP(pd, fromd, tod, i, offset, from_map, pos_map);
        INTP(pd, fromd, tod, i, r, from_map, pos_map);
        INTP(pd, fromd, tod, i, g, from_map, pos_map);
        INTP(pd, fromd, tod, i, b, from_map, pos_map);
        INTP(pd, fromd, tod, i, a, from_map, pos_map);
     }

#undef INTP

   return EINA_TRUE;
}

/**
 * @brief Destructor for the Efl_Canvas_Vg_Gradient object.
 *
 * Frees allocated resources, specifically the color stops array,
 * and then calls the superclass destructor.
 *
 * @param obj The Evas object being destroyed.
 * @param pd Private data for the gradient.
 */
static void
_efl_canvas_vg_gradient_efl_object_destructor(Eo *obj, Efl_Canvas_Vg_Gradient_Data *pd)
{
   if (pd->colors) free(pd->colors);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Duplicates the gradient object.
 *
 * Creates a new Efl_VG object that is a copy of the current gradient,
 * including its color stops and spread method.
 *
 * @param obj The Evas object to duplicate.
 * @param pd Private data for the gradient.
 * @return A new Efl_VG object which is a duplicate of 'obj', or NULL on failure.
 */
EOLIAN static Efl_VG *
_efl_canvas_vg_gradient_efl_duplicate_duplicate(const Eo *obj, Efl_Canvas_Vg_Gradient_Data *pd)

{
   Efl_VG *cn = NULL;

   cn = efl_duplicate(efl_super(obj, MY_CLASS));
   efl_gfx_gradient_stop_set(cn, pd->colors, pd->colors_count);
   efl_gfx_gradient_spread_set(cn, pd->spread);
   return cn;
}

/**
 * @brief Sets the color stops for a gradient object (legacy C API).
 * @param obj The gradient object.
 * @param colors An array of Evas_Vg_Gradient_Stop structures.
 * @param length The number of color stops.
 * @see _efl_canvas_vg_gradient_efl_gfx_gradient_stop_set
 */
EVAS_API void
evas_vg_gradient_stop_set(Evas_Vg_Gradient *obj, const Evas_Vg_Gradient_Stop *colors, unsigned int length)
{
   efl_gfx_gradient_stop_set(obj, (const Efl_Gfx_Gradient_Stop *)colors, length);
}

/**
 * @brief Gets the color stops for a gradient object (legacy C API).
 * @param obj The gradient object.
 * @param[out] colors Pointer to store the address of the Evas_Vg_Gradient_Stop array.
 * @param[out] length Pointer to store the number of color stops.
 * @see _efl_canvas_vg_gradient_efl_gfx_gradient_stop_get
 */
EVAS_API void
evas_vg_gradient_stop_get(Evas_Vg_Gradient *obj, const Evas_Vg_Gradient_Stop **colors, unsigned int *length)
{
   efl_gfx_gradient_stop_get(obj, (const Efl_Gfx_Gradient_Stop **)colors, length);
}

/**
 * @brief Sets the spread method for a gradient object (legacy C API).
 * @param obj The gradient object.
 * @param s The Evas_Vg_Gradient_Spread value.
 * @see _efl_canvas_vg_gradient_efl_gfx_gradient_spread_set
 */
EVAS_API void
evas_vg_gradient_spread_set(Evas_Vg_Gradient *obj, Evas_Vg_Gradient_Spread s)
{
   efl_gfx_gradient_spread_set(obj, (Efl_Gfx_Gradient_Spread)s);
}

/**
 * @brief Gets the spread method for a gradient object (legacy C API).
 * @param obj The gradient object.
 * @return The current Evas_Vg_Gradient_Spread value.
 * @see _efl_canvas_vg_gradient_efl_gfx_gradient_spread_get
 */
EVAS_API Evas_Vg_Gradient_Spread
evas_vg_gradient_spread_get(Evas_Vg_Gradient *obj)
{
   return (Evas_Vg_Gradient_Spread)efl_gfx_gradient_spread_get(obj);
}

#include "efl_canvas_vg_gradient.eo.c"
