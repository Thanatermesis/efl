#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

/**
 * @brief Sets the start point of the linear gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Linear object.
 * @param[out] pd The Ector_Renderer_Gradient_Linear_Data object to be modified.
 * @param[in] x The x-coordinate of the start point.
 * @param[in] y The y-coordinate of the start point.
 */
static void
_ector_renderer_gradient_linear_efl_gfx_gradient_linear_start_set(Eo *obj EINA_UNUSED,
                                                                          Ector_Renderer_Gradient_Linear_Data *pd,
                                                                          double x, double y)
{
   pd->start.x = x;
   pd->start.y = y;
}

/**
 * @brief Gets the start point of the linear gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Linear object.
 * @param[in] pd The Ector_Renderer_Gradient_Linear_Data object.
 * @param[out] x Pointer to store the x-coordinate of the start point. Can be NULL.
 * @param[out] y Pointer to store the y-coordinate of the start point. Can be NULL.
 */
static void
_ector_renderer_gradient_linear_efl_gfx_gradient_linear_start_get(const Eo *obj EINA_UNUSED,
                                                                          Ector_Renderer_Gradient_Linear_Data *pd,
                                                                          double *x, double *y)
{
   if (x) *x = pd->start.x;
   if (y) *y = pd->start.y;
}

/**
 * @brief Sets the end point of the linear gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Linear object.
 * @param[out] pd The Ector_Renderer_Gradient_Linear_Data object to be modified.
 * @param[in] x The x-coordinate of the end point.
 * @param[in] y The y-coordinate of the end point.
 */
static void
_ector_renderer_gradient_linear_efl_gfx_gradient_linear_end_set(Eo *obj EINA_UNUSED,
                                                                        Ector_Renderer_Gradient_Linear_Data *pd,
                                                                        double x, double y)
{
   pd->end.x = x;
   pd->end.y = y;
}

/**
 * @brief Gets the end point of the linear gradient.
 *
 * @param[in] obj The Ector_Renderer_Gradient_Linear object.
 * @param[in] pd The Ector_Renderer_Gradient_Linear_Data object.
 * @param[out] x Pointer to store the x-coordinate of the end point. Can be NULL.
 * @param[out] y Pointer to store the y-coordinate of the end point. Can be NULL.
 */
static void
_ector_renderer_gradient_linear_efl_gfx_gradient_linear_end_get(const Eo *obj EINA_UNUSED,
                                                                        Ector_Renderer_Gradient_Linear_Data *pd,
                                                                        double *x, double *y)
{
   if (x) *x = pd->end.x;
   if (y) *y = pd->end.y;
}

#include "ector_renderer_gradient_linear.eo.c"
