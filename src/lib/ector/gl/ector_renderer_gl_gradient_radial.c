#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "gl/Ector_GL.h"
#include "ector_private.h"
#include "ector_gl_private.h"

/**
 * @internal
 * @brief Private data for the Ector GL radial gradient renderer.
 *
 * This structure holds all the private data necessary for rendering
 * radial gradients using OpenGL. It includes references to the
 * generic radial gradient data, generic gradient data, and base renderer data.
 */
typedef struct _Ector_Renderer_GL_Gradient_Radial_Data Ector_Renderer_GL_Gradient_Radial_Data;
struct _Ector_Renderer_GL_Gradient_Radial_Data
{
   Ector_Renderer_Gradient_Radial_Data *radial; /**< Pointer to the radial gradient specific data. This includes center point and radius. */
   Ector_Renderer_Gradient_Data *gradient;      /**< Pointer to the generic gradient data. This includes color stops and spread method. */
   Ector_Renderer_Data *base;                   /**< Pointer to the base renderer data. This includes origin and transformation matrix. */
};

/**
 * @internal
 * @brief Prepares the GL radial gradient renderer for drawing operations.
 *
 * This function is called before any drawing operations occur. It can be
 * used to set up GL states, shaders, or other resources needed for rendering
 * the radial gradient.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL radial gradient renderer.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_prepare(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   // FIXME: prepare something
   (void) obj;
   (void) pd;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Performs the drawing operation for the GL radial gradient.
 *
 * This function is responsible for actually rendering the radial gradient
 * to the target surface. It delegates to the parent class's draw function.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL radial gradient renderer.
 * @param op The rendering operation to perform (e.g., fill, stroke).
 * @param clips An array of Eina_Rect structures defining the clipping region.
 * @param mul_col The multiplication color to apply during rendering.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_draw(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips, unsigned int mul_col)
{
   ector_renderer_draw(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS), op, clips, mul_col);

   // FIXME: draw something !
   (void) pd;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Handles the GL fill operation for the radial gradient.
 *
 * This function is intended to select the appropriate GL shader for
 * radial gradients and pass the necessary parameters (e.g., center, radius,
 * color stops) to it, along with vertex data.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL radial gradient renderer.
 * @param flags Flags for the fill operation.
 * @param vertex An array of GLshort representing the vertices of the shape to fill.
 *               Example: {x1, y1, x2, y2, x3, y3, ...}
 * @param vertex_count The number of vertices in the @p vertex array.
 * @param mul_col The multiplication color to apply during rendering.
 * @return @c EINA_TRUE on success or if the operation is handled, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_gl_op_fill(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd, uint64_t flags, GLshort *vertex, unsigned int vertex_count, unsigned int mul_col)
{
   // FIXME: The idea here is to select the right shader and push the needed parameter for it
   // along with the other value
   (void) obj;
   (void) pd;
   (void) flags;
   (void) vertex;
   (void) vertex_count;
   (void) mul_col;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Calculates the bounding box of the radial gradient.
 *
 * The bounding box is determined by the gradient's center point and radius,
 * adjusted by the renderer's origin.
 *
 * @param obj The Ector renderer object (unused).
 * @param pd The private data for the GL radial gradient renderer.
 * @param r Pointer to an Eina_Rect structure to store the calculated bounds.
 *          The structure will be filled like: {x, y, w, h}
 *          where (x,y) is the top-left corner and (w,h) are width and height.
 */
static void
_ector_renderer_gl_gradient_radial_efl_gfx_path_bounds_get(const Eo *obj EINA_UNUSED,
                                                             Ector_Renderer_GL_Gradient_Radial_Data *pd,
                                                             Eina_Rect *r)
{
   EINA_RECTANGLE_SET(r,
                      pd->base->origin.x + pd->radial->radial.x - pd->radial->radius,
                      pd->base->origin.y + pd->radial->radial.y - pd->radial->radius,
                      pd->radial->radius * 2, pd->radial->radius * 2 );
}

/**
 * @internal
 * @brief Calculates a CRC (Cyclic Redundancy Check) for the radial gradient's properties.
 *
 * This CRC can be used to quickly check if the gradient's definition has changed,
 * which is useful for caching or invalidation. It incorporates the CRC of the
 * parent class and adds CRCs for the spread method, color stops, and radial
 * gradient specific data.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL radial gradient renderer.
 * @return The calculated CRC value.
 */
static unsigned int
_ector_renderer_gl_gradient_radial_ector_renderer_crc_get(const Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   unsigned int crc;

   crc = ector_renderer_crc_get(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS));

   crc = eina_crc((void*) pd->gradient->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (pd->gradient->colors_count)
     crc = eina_crc((void*) pd->gradient->colors, sizeof (Efl_Gfx_Gradient_Stop) * pd->gradient->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) pd->radial, sizeof (Ector_Renderer_Gradient_Radial_Data), crc, EINA_FALSE);

   return crc;
}

/**
 * @internal
 * @brief Constructor for the Ector GL radial gradient renderer object.
 *
 * Initializes the object by calling the parent class's constructor and
 * then sets up references to its private data members by cross-referencing
 * data from related Efl objects (ECTOR_RENDERER_CLASS,
 * ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN, ECTOR_RENDERER_GRADIENT_MIXIN).
 *
 * @param obj The Ector renderer object being constructed.
 * @param pd The private data for the GL radial gradient renderer.
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
static Efl_Object *
_ector_renderer_gl_gradient_radial_efl_object_constructor(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS));

   if (!obj) return NULL;

   pd->base = efl_data_xref(obj, ECTOR_RENDERER_CLASS, obj);
   pd->radial = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN, obj);
   pd->gradient = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_MIXIN, obj);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ector GL radial gradient renderer object.
 *
 * Cleans up resources by unreferencing the private data members that were
 * cross-referenced during construction.
 *
 * @param obj The Ector renderer object being destructed.
 * @param pd The private data for the GL radial gradient renderer.
 */
static void
_ector_renderer_gl_gradient_radial_efl_object_destructor(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   efl_data_xunref(obj, pd->base, obj);
   efl_data_xunref(obj, pd->radial, obj);
   efl_data_xunref(obj, pd->gradient, obj);
}

/**
 * @internal
 * @brief Sets the color stops for the radial gradient.
 *
 * This function delegates the setting of color stops to the parent class.
 * Color stops define the colors and their positions along the gradient.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL radial gradient renderer (unused).
 * @param colors An array of Efl_Gfx_Gradient_Stop structures.
 *               Each Efl_Gfx_Gradient_Stop has:
 *               - Efl_Gfx_Color c: The color (r, g, b, a components).
 *               - double offset: The position of the stop (0.0 to 1.0).
 *               - double opacity: The opacity at this stop.
 *               Example:
 *               Efl_Gfx_Gradient_Stop stops[] = {
 *                 {{255, 0, 0, 255}, 0.0, 1.0}, // Red at the start
 *                 {{0, 0, 255, 255}, 1.0, 1.0}  // Blue at the end
 *               };
 * @param length The number of color stops in the @p colors array.
 */
static void
_ector_renderer_gl_gradient_radial_efl_gfx_gradient_stop_set(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd EINA_UNUSED, const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   efl_gfx_gradient_stop_set(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS), colors, length);
}

#include "ector_renderer_gl_gradient_radial.eo.c"
