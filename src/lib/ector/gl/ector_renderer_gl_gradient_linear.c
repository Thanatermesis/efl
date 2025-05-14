#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "gl/Ector_GL.h"
#include "ector_private.h"
#include "ector_gl_private.h"

typedef struct _Ector_Renderer_GL_Gradient_Linear_Data Ector_Renderer_GL_Gradient_Linear_Data;
/**
 * @brief Private data for the Ector GL linear gradient renderer.
 *
 * This structure holds all the necessary data for rendering a linear gradient
 * using OpenGL. It includes references to the generic gradient data,
 * linear-specific gradient data, and base renderer data.
 */
struct _Ector_Renderer_GL_Gradient_Linear_Data
{
   Ector_Renderer_Gradient_Linear_Data *linear; /**< Data specific to linear gradients (start/end points). */
   Ector_Renderer_Gradient_Data *gradient; /**< Common gradient data (colors, spread method). */
   Ector_Renderer_Data *base; /**< Base renderer data (origin, transformation). */
};

/**
 * @brief Prepares the renderer for drawing operations.
 *
 * This function is called before any drawing operations occur. It can be used
 * to set up shaders, buffers, or other OpenGL states required for rendering
 * the linear gradient.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL linear gradient renderer.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ector_renderer_gl_gradient_linear_ector_renderer_prepare(Eo *obj,
                                                                       Ector_Renderer_GL_Gradient_Linear_Data *pd)
{
   // FIXME: prepare something
   (void) obj;
   (void) pd;

   return EINA_TRUE;
}

/**
 * @brief Performs the actual drawing of the linear gradient.
 *
 * This function is responsible for issuing the OpenGL draw calls to render
 * the gradient. It should respect the current rendering operation, clipping
 * regions, and color multiplication.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL linear gradient renderer.
 * @param op The rendering operation (e.g., EFL_GFX_RENDER_OP_BLEND).
 * @param clips An array of Eina_Rect structures defining the clipping regions.
 *              Example:
 *              @code
 *              Eina_Array *clips_array = eina_array_new(1);
 *              Eina_Rect clip_rect = { .x = 0, .y = 0, .w = 100, .h = 100 };
 *              eina_array_push(clips_array, &clip_rect);
 *              // ... pass clips_array to this function ...
 *              eina_array_free(clips_array);
 *              @endcode
 * @param mul_col The color to multiply with the gradient (ARGB format).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ector_renderer_gl_gradient_linear_ector_renderer_draw(Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips, unsigned int mul_col)
{
   ector_renderer_draw(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_LINEAR_CLASS), op, clips, mul_col);

   // FIXME: draw something !
   (void) pd;

   return EINA_TRUE;
}

/**
 * @brief Gets the bounding box of the linear gradient.
 *
 * This function calculates the rectangle that encompasses the linear gradient,
 * defined by its start and end points, relative to the renderer's origin.
 *
 * @param obj The Ector renderer object (unused).
 * @param pd The private data for the GL linear gradient renderer.
 * @param r Pointer to an Eina_Rect structure to store the calculated bounds.
 *          Example of output:
 *          If origin is (10, 20), start is (5, 5) and end is (50, 30),
 *          r will be { .x = 10 + 5, .y = 20 + 5, .w = 50 - 5, .h = 30 - 5 }.
 *          r will be { .x = 15, .y = 25, .w = 45, .h = 25 }.
 */
static void
_ector_renderer_gl_gradient_linear_efl_gfx_path_bounds_get(const Eo *obj EINA_UNUSED,
                                                             Ector_Renderer_GL_Gradient_Linear_Data *pd,
                                                             Eina_Rect *r)
{
   EINA_RECTANGLE_SET(r,
                      pd->base->origin.x + pd->linear->start.x,
                      pd->base->origin.y + pd->linear->start.y,
                      pd->linear->end.x - pd->linear->start.x,
                      pd->linear->end.y - pd->linear->start.y);
}

/**
 * @brief Fills a shape with the linear gradient using OpenGL operations.
 *
 * This function is responsible for setting up the appropriate OpenGL shader
 * for linear gradients, passing the gradient parameters (colors, stops,
 * start/end points), and then drawing the provided vertices.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL linear gradient renderer.
 * @param flags Flags controlling the fill operation (e.g., anti-aliasing).
 * @param vertex An array of GLshort representing the vertices of the shape to fill.
 *               The structure of this array depends on the vertex format used by the renderer.
 *               Typically, it's pairs of (x, y) coordinates.
 *               Example for a triangle: { x1, y1, x2, y2, x3, y3 }
 * @param vertex_count The number of vertices in the @p vertex array.
 * @param mul_col The color to multiply with the gradient (ARGB format).
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ector_renderer_gl_gradient_linear_ector_renderer_gl_op_fill(Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd, uint64_t flags, GLshort *vertex, unsigned int vertex_count, unsigned int mul_col)
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
 * @brief Constructor for the Ector GL linear gradient renderer.
 *
 * Initializes the renderer object and its private data. It establishes
 * cross-references to data from parent/mixin classes.
 *
 * @param obj The Ector renderer object being constructed.
 * @param pd The private data for the GL linear gradient renderer.
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
static Efl_Object *
_ector_renderer_gl_gradient_linear_efl_object_constructor(Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_LINEAR_CLASS));

   if (!obj) return NULL;

   pd->base = efl_data_xref(obj, ECTOR_RENDERER_CLASS, obj);
   pd->linear = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_LINEAR_MIXIN, obj);
   pd->gradient = efl_data_xref(obj, ECTOR_RENDERER_GRADIENT_MIXIN, obj);

   return obj;
}

/**
 * @brief Destructor for the Ector GL linear gradient renderer.
 *
 * Cleans up resources used by the renderer, including unreferencing
 * data from parent/mixin classes.
 *
 * @param obj The Ector renderer object being destructed.
 * @param pd The private data for the GL linear gradient renderer.
 */
static void
_ector_renderer_gl_gradient_linear_efl_object_destructor(Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd)
{
   efl_data_xunref(obj, pd->base, obj);
   efl_data_xunref(obj, pd->linear, obj);
   efl_data_xunref(obj, pd->gradient, obj);
}

/**
 * @brief Sets the color stops for the gradient.
 *
 * This function updates the gradient's color stops. It calls the superclass
 * implementation to handle the actual storage of the color stops.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL linear gradient renderer (unused).
 * @param colors An array of Efl_Gfx_Gradient_Stop structures defining the color stops.
 *               Each Efl_Gfx_Gradient_Stop has:
 *               - Efl_Gfx_Color color: The color at this stop (r, g, b, a components).
 *               - double offset: The position of the stop (0.0 to 1.0).
 *               Example:
 *               @code
 *               Efl_Gfx_Gradient_Stop stops[] = {
 *                   { {255, 0, 0, 255}, 0.0 }, // Red at the beginning
 *                   { {0, 0, 255, 255}, 1.0 }  // Blue at the end
 *               };
 *               unsigned int length = sizeof(stops) / sizeof(stops[0]);
 *               // ... pass stops and length to this function ...
 *               @endcode
 * @param length The number of color stops in the @p colors array.
 */
static void
_ector_renderer_gl_gradient_linear_efl_gfx_gradient_stop_set(Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd EINA_UNUSED, const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   efl_gfx_gradient_stop_set(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_LINEAR_CLASS), colors, length);
}

/**
 * @brief Calculates a CRC (Cyclic Redundancy Check) for the renderer's current state.
 *
 * This CRC can be used to quickly check if the renderer's configuration
 * (gradient colors, spread method, linear gradient parameters) has changed.
 * It combines the CRC from the superclass with CRCs of the gradient-specific data.
 *
 * @param obj The Ector renderer object.
 * @param pd The private data for the GL linear gradient renderer.
 * @return The calculated CRC value.
 */
static unsigned int
_ector_renderer_gl_gradient_linear_ector_renderer_crc_get(const Eo *obj, Ector_Renderer_GL_Gradient_Linear_Data *pd)
{
   unsigned int crc;

   crc = ector_renderer_crc_get(efl_super(obj, ECTOR_RENDERER_GL_GRADIENT_LINEAR_CLASS));

   crc = eina_crc((void*) pd->gradient->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (pd->gradient->colors_count)
     crc = eina_crc((void*) pd->gradient->colors, sizeof (Efl_Gfx_Gradient_Stop) * pd->gradient->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) pd->linear, sizeof (Ector_Renderer_Gradient_Linear_Data), crc, EINA_FALSE);

   return crc;
}

#include "ector_renderer_gl_gradient_linear.eo.c"
