#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

#define MY_CLASS ECTOR_RENDERER_GRADIENT_MIXIN

/**
 * @brief Sets the color stops for the gradient.
 *
 * @param obj The Evas object.
 * @param pd Private data for the Ector_Renderer_Gradient.
 * @param colors An array of Efl_Gfx_Gradient_Stop structures.
 *               Each structure defines a color and its offset in the gradient.
 *               Example:
 *               Efl_Gfx_Gradient_Stop stops[] = {
 *                 { { 0, 0, 0, 255 }, 0.0 }, // Black at the start
 *                 { { 255, 255, 255, 255 }, 1.0 }  // White at the end
 *               };
 * @param length The number of color stops in the @p colors array.
 */
static void
_ector_renderer_gradient_efl_gfx_gradient_stop_set(Eo *obj EINA_UNUSED,
                                                                Ector_Renderer_Gradient_Data *pd,
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
}

/**
 * @brief Gets the color stops for the gradient.
 *
 * @param obj The Evas object.
 * @param pd Private data for the Ector_Renderer_Gradient.
 * @param[out] colors Pointer to store the array of Efl_Gfx_Gradient_Stop structures.
 * @param[out] length Pointer to store the number of color stops.
 */
static void
_ector_renderer_gradient_efl_gfx_gradient_stop_get(const Eo *obj EINA_UNUSED,
                                                                Ector_Renderer_Gradient_Data *pd,
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
 * [0, 1] offset range.
 *
 * @param obj The Evas object.
 * @param pd Private data for the Ector_Renderer_Gradient.
 * @param s The Efl_Gfx_Gradient_Spread value.
 *          Example: EFL_GFX_GRADIENT_SPREAD_REPEAT
 */
static void
_ector_renderer_gradient_efl_gfx_gradient_spread_set(Eo *obj EINA_UNUSED,
                                                                  Ector_Renderer_Gradient_Data *pd,
                                                                  Efl_Gfx_Gradient_Spread s)
{
   pd->s = s;
}

/**
 * @brief Gets the spread method for the gradient.
 *
 * @param obj The Evas object.
 * @param pd Private data for the Ector_Renderer_Gradient.
 * @return The Efl_Gfx_Gradient_Spread value.
 */
static Efl_Gfx_Gradient_Spread
_ector_renderer_gradient_efl_gfx_gradient_spread_get(const Eo *obj EINA_UNUSED,
                                                                  Ector_Renderer_Gradient_Data *pd)
{
   return pd->s;
}

/**
 * @brief Invalidates the gradient renderer, freeing associated resources.
 *
 * This function is called when the Evas object is being invalidated.
 * It frees the memory allocated for color stops.
 *
 * @param obj The Evas object.
 * @param pd Private data for the Ector_Renderer_Gradient.
 */
static void
_ector_renderer_gradient_efl_object_invalidate(Eo *obj EINA_UNUSED,
                                               Ector_Renderer_Gradient_Data *pd)
{
   if (pd->colors) free(pd->colors);
}

#include "ector_renderer_gradient.eo.c"
