#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "ector_private.h"

#define MY_CLASS ECTOR_RENDERER_SHAPE_MIXIN

/**
 * @internal
 * @brief Sets the fill renderer for the shape.
 *
 * This function replaces the current fill renderer with the provided renderer.
 * If @p r is NULL, the fill renderer is cleared.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @param r The Ector_Renderer to use for filling the shape.
 */
static void
_ector_renderer_shape_fill_set(Eo *obj EINA_UNUSED,
                                       Ector_Renderer_Shape_Data *pd,
                                       const Ector_Renderer *r)
{
   efl_replace(&pd->fill, r);
}

/**
 * @internal
 * @brief Gets the fill renderer for the shape.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @return The current fill Ector_Renderer, or NULL if not set.
 */
static const Ector_Renderer *
_ector_renderer_shape_fill_get(const Eo *obj EINA_UNUSED,
                                       Ector_Renderer_Shape_Data *pd)
{
   return pd->fill;
}

/**
 * @internal
 * @brief Sets the stroke fill renderer for the shape.
 *
 * This function replaces the current stroke fill renderer with the provided renderer.
 * If @p r is NULL, the stroke fill renderer is cleared.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @param r The Ector_Renderer to use for the stroke fill.
 */
static void
_ector_renderer_shape_stroke_fill_set(Eo *obj EINA_UNUSED,
                                              Ector_Renderer_Shape_Data *pd,
                                              const Ector_Renderer *r)
{
   efl_replace(&pd->stroke.fill, r);
}

/**
 * @internal
 * @brief Gets the stroke fill renderer for the shape.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @return The current stroke fill Ector_Renderer, or NULL if not set.
 */
static const Ector_Renderer *
_ector_renderer_shape_stroke_fill_get(const Eo *obj EINA_UNUSED,
                                              Ector_Renderer_Shape_Data *pd)
{
   return pd->stroke.fill;
}

/**
 * @internal
 * @brief Sets the stroke marker renderer for the shape.
 *
 * This function replaces the current stroke marker renderer with the provided renderer.
 * If @p r is NULL, the stroke marker renderer is cleared.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @param r The Ector_Renderer to use for the stroke marker.
 */
static void
_ector_renderer_shape_stroke_marker_set(Eo *obj EINA_UNUSED,
                                                Ector_Renderer_Shape_Data *pd,
                                                const Ector_Renderer *r)
{
   efl_replace(&pd->stroke.marker, r);
}

/**
 * @internal
 * @brief Gets the stroke marker renderer for the shape.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 * @return The current stroke marker Ector_Renderer, or NULL if not set.
 */
static const Ector_Renderer *
_ector_renderer_shape_stroke_marker_get(const Eo *obj EINA_UNUSED,
                                                Ector_Renderer_Shape_Data *pd)
{
   return pd->stroke.marker;
}

/**
 * @internal
 * @brief Invalidates the shape renderer by releasing its associated renderers.
 *
 * This function is called when the Ector_Renderer_Shape object is being invalidated.
 * It releases references to the fill, stroke fill, and stroke marker renderers.
 *
 * @param obj The Ector_Renderer_Shape object.
 * @param pd The private data of the Ector_Renderer_Shape object.
 */
static void
_ector_renderer_shape_efl_object_invalidate(Eo *obj EINA_UNUSED,
                                            Ector_Renderer_Shape_Data *pd)
{
   efl_replace(&pd->fill, NULL);
   efl_replace(&pd->stroke.fill, NULL);
   efl_replace(&pd->stroke.marker, NULL);
}


#include "ector_renderer_shape.eo.c"
