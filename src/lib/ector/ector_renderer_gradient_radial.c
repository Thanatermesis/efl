#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

/**
 * @internal
 * @brief Sets the center point of the radial gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in,out] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @param[in] x The x-coordinate of the center point.
 * @param[in] y The y-coordinate of the center point.
 */
static void
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_center_set(Eo *obj EINA_UNUSED,
                                                                           Ector_Renderer_Gradient_Radial_Data *pd,
                                                                           double x, double y)
{
   pd->radial.x = x;
   pd->radial.y = y;
}

/**
 * @internal
 * @brief Gets the center point of the radial gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @param[out] x Pointer to store the x-coordinate of the center point.
 * @param[out] y Pointer to store the y-coordinate of the center point.
 */
static void
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_center_get(const Eo *obj EINA_UNUSED,
                                                                           Ector_Renderer_Gradient_Radial_Data *pd,
                                                                           double *x, double *y)
{
   if (x) *x = pd->radial.x;
   if (y) *y = pd->radial.y;
}

/**
 * @internal
 * @brief Sets the radius of the radial gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in,out] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @param[in] r The radius value.
 */
static void
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_radius_set(Eo *obj EINA_UNUSED,
                                                                           Ector_Renderer_Gradient_Radial_Data *pd,
                                                                           double r)
{
   pd->radius = r;
}

/**
 * @internal
 * @brief Gets the radius of the radial gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @return The radius value.
 */
static double
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_radius_get(const Eo *obj EINA_UNUSED,
                                                                           Ector_Renderer_Gradient_Radial_Data *pd)
{
   return pd->radius;
}

/**
 * @internal
 * @brief Sets the focal point of the radial gradient.
 *
 * The focal point determines the origin of the gradient rays.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in,out] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @param[in] x The x-coordinate of the focal point.
 * @param[in] y The y-coordinate of the focal point.
 */
static void
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_focal_set(Eo *obj EINA_UNUSED,
                                                                          Ector_Renderer_Gradient_Radial_Data *pd,
                                                                          double x, double y)
{
   pd->focal.x = x;
   pd->focal.y = y;
}

/**
 * @internal
 * @brief Gets the focal point of the radial gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Radial object.
 * @param[in] pd The private data of the Ector_Renderer_Gradient_Radial object.
 * @param[out] x Pointer to store the x-coordinate of the focal point.
 * @param[out] y Pointer to store the y-coordinate of the focal point.
 */
static void
_ector_renderer_gradient_radial_efl_gfx_gradient_radial_focal_get(const Eo *obj EINA_UNUSED,
                                                                          Ector_Renderer_Gradient_Radial_Data *pd,
                                                                          double *x, double *y)
{
   if (x) *x = pd->focal.x;
   if (y) *y = pd->focal.y;
}

#include "ector_renderer_gradient_radial.eo.c"
