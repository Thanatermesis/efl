#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"
#include "ector_software_gradient.h"

/**
 * @internal
 * @brief Prepares the linear gradient renderer.
 *
 * This function initializes the gradient parameters, such as start and end points,
 * calculates the gradient vector (dx, dy), its length (l), and the offset (off).
 * It also ensures the color table is updated if necessary.
 *
 * @param obj The Ector_Renderer object.
 * @param pd The private data of the software gradient renderer.
 * @return EINA_FALSE always, as this function doesn't indicate success/failure of an operation
 *         in the traditional sense but rather prepares the renderer.
 */
static Eina_Bool
_ector_renderer_software_gradient_linear_ector_renderer_prepare(Eo *obj,
                                                                Ector_Renderer_Software_Gradient_Data *pd)
{
   pd->ctable_status = CTABLE_NOT_READY;

   if (!pd->surface)
     {
        Ector_Renderer_Data *base = efl_data_scope_get(obj, ECTOR_RENDERER_CLASS);
        pd->surface = efl_data_xref(base->surface, ECTOR_SOFTWARE_SURFACE_CLASS, obj);
     }
   ector_software_gradient_color_update(pd);

   pd->linear.x1 = pd->gld->start.x;
   pd->linear.y1 = pd->gld->start.y;

   pd->linear.x2 = pd->gld->end.x;
   pd->linear.y2 = pd->gld->end.y;

   pd->linear.dx = pd->linear.x2 - pd->linear.x1;
   pd->linear.dy = pd->linear.y2 - pd->linear.y1;
   pd->linear.l = pd->linear.dx * pd->linear.dx + pd->linear.dy * pd->linear.dy;
   pd->linear.off = 0;

   if (!EINA_DBL_EQ(pd->linear.l, 0.0))
     {
        pd->linear.dx /= pd->linear.l;
        pd->linear.dy /= pd->linear.l;
        pd->linear.off = -pd->linear.dx * pd->linear.x1 - pd->linear.dy * pd->linear.y1;
     }

   return EINA_FALSE;
}

/**
 * @internal
 * @brief Draws using the linear gradient renderer.
 *
 * This function is currently a stub and returns EINA_TRUE, indicating
 * that the operation is handled, though no actual drawing operations are
 * performed here directly. The actual drawing is typically handled by
 * rasterizer operations.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the software gradient renderer (unused).
 * @param op The rendering operation (unused).
 * @param clips An array of clipping regions (unused).
 * @param mul_col The multiplication color (unused).
 * @return EINA_TRUE always.
 */
static Eina_Bool
_ector_renderer_software_gradient_linear_ector_renderer_draw(Eo *obj EINA_UNUSED,
                                                             Ector_Renderer_Software_Gradient_Data *pd EINA_UNUSED,
                                                             Efl_Gfx_Render_Op op EINA_UNUSED, Eina_Array *clips EINA_UNUSED,
                                                             unsigned int mul_col EINA_UNUSED)
{
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Performs a fill operation using the linear gradient.
 *
 * This function sets the linear gradient parameters in the software rasterizer
 * and ensures the gradient color table is up-to-date.
 *
 * @param obj The Ector_Renderer object (unused).
 * @param pd The private data of the software gradient renderer.
 * @return EINA_TRUE if the operation was successful.
 */
static Eina_Bool
_ector_renderer_software_gradient_linear_ector_renderer_software_op_fill(Eo *obj EINA_UNUSED,
                                                                         Ector_Renderer_Software_Gradient_Data *pd)
{
   ector_software_rasterizer_linear_gradient_set(pd->surface->rasterizer, pd);
   ector_software_gradient_color_update(pd);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Constructor for the linear gradient software renderer.
 *
 * Initializes the renderer object by calling the superclass constructor and
 * setting up references to gradient data (generic and linear specific).
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the software gradient renderer.
 * @return The constructed Eo object, or NULL on failure.
 */
static Eo *
_ector_renderer_software_gradient_linear_efl_object_constructor(Eo *obj,
                                                                Ector_Renderer_Software_Gradient_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS));
   if (!obj) return NULL;

   pd->gd  = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_MIXIN, obj);
   pd->gld = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_LINEAR_MIXIN, obj);
   pd->ctable_status = CTABLE_NOT_READY;

   return obj;
}

/**
 * @internal
 * @brief Destructor for the linear gradient software renderer.
 *
 * Cleans up resources used by the renderer, including the color table
 * and references to surface and gradient data.
 *
 * @param obj The Eo object being destructed.
 * @param pd The private data of the software gradient renderer.
 */
static void
_ector_renderer_software_gradient_linear_efl_object_destructor(Eo *obj,
                                                            Ector_Renderer_Software_Gradient_Data *pd)
{
   Ector_Renderer_Data *base;

   destroy_color_table(pd);

   base = efl_data_scope_get(obj, ECTOR_RENDERER_CLASS);
   efl_data_xunref(base->surface, pd->surface, obj);

   efl_data_xunref(obj, pd->gd, obj);
   efl_data_xunref(obj, pd->gld, obj);

   efl_destructor(efl_super(obj, ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS));
}

/**
 * @internal
 * @brief Sets the gradient stops for the linear gradient.
 *
 * This function forwards the call to the superclass to set the gradient stops.
 * The color table status is implicitly affected and will be updated when needed.
 *
 * @param obj The Ector_Renderer object.
 * @param pd The private data of the software gradient renderer (unused here, but part of the function signature).
 * @param colors An array of Efl_Gfx_Gradient_Stop structures.
 *               Example:
 *               Efl_Gfx_Gradient_Stop stops[] = {
 *                 { { 255, 0, 0, 255 }, 0.0 }, // Red at the start
 *                 { { 0, 0, 255, 255 }, 1.0 }  // Blue at the end
 *               };
 * @param length The number of Efl_Gfx_Gradient_Stop elements in the colors array.
 */
void
_ector_renderer_software_gradient_linear_efl_gfx_gradient_stop_set(Eo *obj, Ector_Renderer_Software_Gradient_Data *pd EINA_UNUSED,
                                                                   const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   efl_gfx_gradient_stop_set(efl_super(obj, ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS), colors, length);
}

/**
 * @internal
 * @brief Calculates a CRC checksum for the current state of the linear gradient renderer.
 *
 * This CRC is used to detect changes in the renderer's configuration,
 * which might necessitate re-caching or re-rendering. It includes the
 * spread method, color stops, and linear gradient specific data.
 *
 * @param obj The Ector_Renderer object.
 * @param pd The private data of the software gradient renderer.
 * @return The calculated CRC value.
 */
static unsigned int
_ector_renderer_software_gradient_linear_ector_renderer_crc_get(const Eo *obj, Ector_Renderer_Software_Gradient_Data *pd)
{
   unsigned int crc;

   crc = ector_renderer_crc_get(efl_super(obj, ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS));

   crc = eina_crc((void*) pd->gd->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (pd->gd->colors_count)
     crc = eina_crc((void*) pd->gd->colors, sizeof (Efl_Gfx_Gradient_Stop) * pd->gd->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) pd->gld, sizeof (Ector_Renderer_Gradient_Linear_Data), crc, EINA_FALSE);

   return crc;
}

#include "ector_renderer_software_gradient_linear.eo.c"
