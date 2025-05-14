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
 * @brief Private data for the Ector_Renderer_GL class.
 *
 * This structure holds data specific to the OpenGL renderer implementation.
 */
typedef struct _Ector_Renderer_GL_Data Ector_Renderer_GL_Data;
struct _Ector_Renderer_GL_Data
{
   Ector_Renderer_Data *base; /**< Pointer to the base renderer data. */
};

/**
 * @internal
 * @brief Prepares the OpenGL renderer for drawing operations.
 *
 * This function is called before any drawing operations are performed.
 * It can be used to set up any necessary OpenGL state.
 *
 * @param obj The Ector_Renderer_GL object.
 * @param pd The private data for the Ector_Renderer_GL object.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_gl_ector_renderer_prepare(Eo *obj EINA_UNUSED,
                                                            Ector_Renderer_GL_Data *pd EINA_UNUSED)
{
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Performs drawing operations using the OpenGL renderer.
 *
 * This function is responsible for rendering graphical elements to the target surface.
 * It sets the surface state based on the provided rendering operation and clips.
 *
 * @param obj The Ector_Renderer_GL object.
 * @param pd The private data for the Ector_Renderer_GL object.
 * @param op The rendering operation to perform (e.g., EFL_GFX_RENDER_OP_BLEND, EFL_GFX_RENDER_OP_COPY).
 * @param clips An array of Eina_Rect structures defining the clipping regions.
 *              Each Eina_Rect has x, y, w, h integer members.
 *              Example:
 *              @code
 *              Eina_Rect clip1 = { .x = 0, .y = 0, .w = 100, .h = 100 };
 *              Eina_Rect clip2 = { .x = 50, .y = 50, .w = 100, .h = 100 };
 *              Eina_Array *clips_array = eina_array_new(2);
 *              eina_array_push(clips_array, &clip1);
 *              eina_array_push(clips_array, &clip2);
 *              @endcode
 * @param mul_col The multiplication color (unused in this implementation).
 * @return @c EINA_TRUE if the drawing operation was successful, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ector_renderer_gl_ector_renderer_draw(Eo *obj EINA_UNUSED,
                                                         Ector_Renderer_GL_Data *pd,
                                                         Efl_Gfx_Render_Op op,
                                                         Eina_Array *clips,
                                                         unsigned int mul_col EINA_UNUSED)
{
   Eina_Bool r;

   r = ector_gl_surface_state_define(pd->base->surface, op, clips);

   return r;
}

/**
 * @internal
 * @brief Constructor for the Ector_Renderer_GL object.
 *
 * Initializes the Ector_Renderer_GL object and its private data.
 * It calls the parent class constructor and sets up a cross-reference
 * to the base Ector_Renderer_Data.
 *
 * @param obj The Ector_Renderer_GL object being constructed.
 * @param pd The private data for the Ector_Renderer_GL object.
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
static Efl_Object *
_ector_renderer_gl_efl_object_constructor(Eo *obj, Ector_Renderer_GL_Data *pd)
{
   obj = efl_constructor(efl_super(obj, ECTOR_RENDERER_GL_CLASS));
   if (!obj) return NULL;

   pd->base = efl_data_xref(obj, ECTOR_RENDERER_CLASS, obj);
   return obj;
}

/**
 * @internal
 * @brief Destructor for the Ector_Renderer_GL object.
 *
 * Cleans up resources used by the Ector_Renderer_GL object.
 * This primarily involves removing the cross-reference to the base
 * Ector_Renderer_Data.
 *
 * @param obj The Ector_Renderer_GL object being destructed.
 * @param pd The private data for the Ector_Renderer_GL object.
 */
static void
_ector_renderer_gl_efl_object_destructor(Eo *obj, Ector_Renderer_GL_Data *pd)
{
   efl_data_xunref(obj, pd->base, obj);
}

#include "ector_renderer_gl.eo.c"
