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
 * @brief Private data for the Ector GL Shape Renderer.
 *
 * This structure holds all the private data necessary for rendering shapes
 * using OpenGL. It includes references to public shape data, internal shape
 * data, base renderer data, and vertex information for GL rendering.
 */
typedef struct _Ector_Renderer_GL_Shape_Data Ector_Renderer_GL_Shape_Data;
struct _Ector_Renderer_GL_Shape_Data
{
   Efl_Gfx_Shape_Public *public_shape; /**< Public shape data, provides stroke, dash, etc. */

   Ector_Renderer_Shape_Data *shape; /**< Internal shape data. */
   Ector_Renderer_Data *base; /**< Base renderer data. */

   GLshort *vertex; /**< Vertex array for GL rendering. Stores (x, y, z) tuples. */
};

/**
 * @internal
 * @brief Commits the path data for the GL shape renderer.
 *
 * This function is called when the path data for the shape has been finalized.
 * It frees any previously allocated vertex data, as it will need to be
 * regenerated based on the new path.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 */
EOLIAN static void
_ector_renderer_gl_shape_efl_gfx_path_commit(Eo *obj EINA_UNUSED,
                                             Ector_Renderer_GL_Shape_Data *pd)
{
   if (pd->vertex)
     {
        free(pd->vertex);
        pd->vertex = NULL;
     }
}

/**
 * @internal
 * @brief Prepares the GL shape renderer for rendering.
 *
 * This function ensures that the renderer is ready to draw. It calls the
 * parent class's prepare function and then generates the vertex data for
 * the shape if it hasn't been created yet. The vertex data currently
 * represents the shape's bounding box as two triangles.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 * @return @c EINA_TRUE if preparation was successful, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_gl_shape_ector_renderer_prepare(Eo *obj, Ector_Renderer_GL_Shape_Data *pd)
{
   Eina_Rect bounding_box;
   Eina_Bool r;

   if (pd->vertex) return EINA_TRUE;

   r = ector_renderer_prepare(efl_super(obj, ECTOR_RENDERER_GL_SHAPE_CLASS));

   efl_gfx_path_bounds_get(obj, &bounding_box);

   pd->vertex = malloc(sizeof (GLshort) * 6 * 3);

   // Pushing 2 triangles
   // Triangle 1: (x,y), (x+w,y), (x,y+h)
   // Triangle 2: (x,y+h), (x+w,y+h), (x+w,y)
   pd->vertex[0] = bounding_box.x;
   pd->vertex[1] = bounding_box.y;
   pd->vertex[2] = 0;
   pd->vertex[3] = bounding_box.x + bounding_box.w;
   pd->vertex[4] = bounding_box.y;
   pd->vertex[5] = 0;
   pd->vertex[6] = bounding_box.x;
   pd->vertex[7] = bounding_box.y + bounding_box.h;
   pd->vertex[8] = 0;

   pd->vertex[9] = bounding_box.x;
   pd->vertex[10] = bounding_box.y + bounding_box.h;
   pd->vertex[11] = 0;
   pd->vertex[12] = bounding_box.x + bounding_box.w;
   pd->vertex[13] = bounding_box.y + bounding_box.h;
   pd->vertex[14] = 0;
   pd->vertex[15] = bounding_box.x + bounding_box.w;
   pd->vertex[16] = bounding_box.y;
   pd->vertex[17] = 0;

   return r;
}

/**
 * @internal
 * @brief Draws the shape using the GL renderer.
 *
 * This function performs the actual drawing of the shape. It calls the parent
 * class's draw function and then, if a fill is defined, uses the GL fill
 * operation. Otherwise, it pushes the vertex data to the GL surface.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 * @param op The rendering operation (e.g., EFL_GFX_RENDER_OP_BLEND).
 * @param clips An array of clipping rectangles.
 * @param mul_col The multiplication color.
 * @return @c EINA_TRUE if drawing was successful, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_gl_shape_ector_renderer_draw(Eo *obj, Ector_Renderer_GL_Shape_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips, unsigned int mul_col)
{
   uint64_t flags = 0; // FIXME: flags should be set based on rendering parameters.

   ector_renderer_draw(efl_super(obj, ECTOR_RENDERER_GL_SHAPE_CLASS), op, clips, mul_col);

   // FIXME: adjust flags content correctly
   // FIXME: should not ignore clips (idea is that the geometry will be cliped here and the
   // context will just look up clips for match with current pipe to render)...

   if (pd->shape->fill)
     {
        ector_renderer_gl_op_fill(pd->shape->fill, flags, pd->vertex, 6, mul_col);
     }
   else
     {
        ector_gl_surface_push(pd->base->surface, flags, pd->vertex, 6, mul_col);
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Performs a GL fill operation for a shape.
 *
 * This function is intended to fill a shape with another shape. However,
 * it is currently not implemented.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 * @param flags Rendering flags.
 * @param vertex Vertex data for the shape to be filled.
 * @param vertex_count The number of vertices.
 * @param mul_col The multiplication color.
 * @return @c EINA_FALSE as it's not implemented.
 */
static Eina_Bool
_ector_renderer_gl_shape_ector_renderer_gl_op_fill(Eo *obj EINA_UNUSED,
                                                     Ector_Renderer_GL_Shape_Data *pd EINA_UNUSED,
                                                     uint64_t flags EINA_UNUSED,
                                                     GLshort *vertex EINA_UNUSED,
                                                     unsigned int vertex_count EINA_UNUSED,
                                                     unsigned int mul_col EINA_UNUSED)
{
   // FIXME: let's find out how to fill a shape with a shape later.
   // I need to read SVG specification and see what to do here.
   ERR("fill with shape not implemented\n");
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Gets the bounding box of the shape, adjusted by the renderer's origin.
 *
 * This function retrieves the bounding box of the shape and then adjusts its
 * position by the origin defined in the base renderer data.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 * @param r Pointer to an Eina_Rect to store the bounding box.
 */
static void
_ector_renderer_gl_shape_efl_gfx_path_bounds_get(const Eo *obj, Ector_Renderer_GL_Shape_Data *pd, Eina_Rect *r)
{
   efl_gfx_path_bounds_get(obj, r);

   r->x += pd->base->origin.x;
   r->y += pd->base->origin.y;
}

/**
 * @internal
 * @brief Calculates a CRC (Cyclic Redundancy Check) for the renderer's state.
 *
 * This function computes a CRC value based on the current state of the
 * renderer, including stroke properties, fill, and dash patterns. This CRC
 * can be used for caching or change detection.
 *
 * @param obj The Evas object.
 * @param pd The private data for the GL shape renderer.
 * @return The calculated CRC value.
 */
static unsigned int
_ector_renderer_gl_shape_ector_renderer_crc_get(const Eo *obj, Ector_Renderer_GL_Shape_Data *pd)
{
   unsigned int crc;

   crc = ector_renderer_crc_get(efl_super(obj, ECTOR_RENDERER_GL_SHAPE_CLASS));

   // This code should be shared with the other implementation
   crc = eina_crc((void*) &pd->shape->stroke.marker, sizeof (pd->shape->stroke.marker), crc, EINA_FALSE);
   crc = eina_crc((void*) &pd->public_shape->stroke.scale, sizeof (pd->public_shape->stroke.scale) * 3, crc, EINA_FALSE); // scale, width, centered
   crc = eina_crc((void*) &pd->public_shape->stroke.color, sizeof (pd->public_shape->stroke.color), crc, EINA_FALSE);
   crc = eina_crc((void*) &pd->public_shape->stroke.cap, sizeof (pd->public_shape->stroke.cap), crc, EINA_FALSE);
   crc = eina_crc((void*) &pd->public_shape->stroke.join, sizeof (pd->public_shape->stroke.join), crc, EINA_FALSE);

   if (pd->shape->fill) crc = _renderer_crc_get(pd->shape->fill, crc);
   if (pd->shape->stroke.fill) crc = _renderer_crc_get(pd->shape->stroke.fill, crc);
   if (pd->shape->stroke.marker) crc = _renderer_crc_get(pd->shape->stroke.marker, crc);
   if (pd->public_shape->stroke.dash_length)
     {
        crc = eina_crc((void*) pd->public_shape->stroke.dash, sizeof (Efl_Gfx_Dash) * pd->public_shape->stroke.dash_length, crc, EINA_FALSE);
     }

   return crc;
}

/**
 * @internal
 * @brief Constructor for the Ector GL Shape Renderer.
 *
 * This function initializes the Ector GL Shape Renderer object. It calls the
 * parent class's constructor and then sets up references to the public shape
 * data, internal shape data, and base renderer data.
 *
 * @param obj The Evas object being constructed.
 * @param pd The private data for the GL shape renderer.
 * @return The constructed Evas object, or @c NULL on failure.
 */
static Efl_Object *
_ector_renderer_gl_shape_efl_object_constructor(Eo *obj, Ector_Renderer_GL_Shape_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECTOR_RENDERER_GL_SHAPE_CLASS));

   if (!obj) return NULL;

   pd->public_shape = efl_data_xref(obj, EFL_GFX_SHAPE_MIXIN, obj);
   pd->shape = efl_data_xref(obj, ECTOR_RENDERER_SHAPE_MIXIN, obj);
   pd->base = efl_data_xref(obj, ECTOR_RENDERER_CLASS, obj);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ector GL Shape Renderer.
 *
 * This function cleans up resources used by the Ector GL Shape Renderer
 * object. It unreferences the public shape data, internal shape data, and
 * base renderer data. It also frees the vertex data if it was allocated.
 *
 * @param obj The Evas object being destructed.
 * @param pd The private data for the GL shape renderer.
 */
static void
_ector_renderer_gl_shape_efl_object_destructor(Eo *obj, Ector_Renderer_GL_Shape_Data *pd)
{
   // Note: pd->vertex is freed in _ector_renderer_gl_shape_efl_gfx_path_commit
   // or if the object is destroyed before commit, it should be handled by Eo's
   // generic data freeing mechanisms if it were directly part of the object's data.
   // However, as it's manually managed, it should ideally be freed here if not
   // already handled by a commit call (e.g., if commit was never called).
   // For now, relying on the commit logic.
   if (pd->vertex)
     {
        free(pd->vertex);
        pd->vertex = NULL;
     }

   efl_data_xunref(obj, pd->shape, obj);
   efl_data_xunref(obj, pd->base, obj);
   efl_data_xunref(obj, pd->public_shape, obj);
}

#include "ector_renderer_gl_shape.eo.c"
