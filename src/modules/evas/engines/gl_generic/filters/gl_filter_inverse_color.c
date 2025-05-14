#include "gl_engine_filter.h"

/**
 * @internal
 * @brief Applies the inverse color filter to a buffer.
 *
 * This function performs the actual rendering of the inverse color effect.
 * It sets up the OpenGL context, binds the input texture and the output
 * framebuffer, and then executes a drawing operation that applies the
 * color inversion logic, likely through a shader.
 *
 * A temporary draw context is created to apply the filter-specific
 * color multiplier and render operation, without affecting the global
 * draw context. This is crucial for correct rendering when filters are
 * chained or used alongside other drawing operations.
 *
 * The function handles both in-place (input buffer is the same as output
 * buffer) and out-of-place filtering. For in-place operations, it uses
 * EVAS_RENDER_COPY to ensure the source is not blended with itself.
 *
 * @param[in] re The render engine data.
 * @param[in] cmd The filter command containing input/output buffers and
 *                drawing parameters.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_gl_filter_inverse_color(Render_Engine_GL_Generic *re, Evas_Filter_Command *cmd)
{
   Evas_Engine_GL_Context *gc;
   Evas_GL_Image *image, *surface;
   RGBA_Draw_Context *dc_save;
   int w, h;

   w = cmd->input->w;
   h = cmd->input->h;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(w == cmd->output->w, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(h == cmd->output->h, EINA_FALSE);

   image = evas_ector_buffer_drawable_image_get(cmd->input->buffer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(image, EINA_FALSE);

   surface = evas_ector_buffer_render_image_get(cmd->output->buffer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, EINA_FALSE);

   gc = gl_generic_context_find(re, 1);
   evas_gl_common_context_target_surface_set(gc, surface);

   dc_save = gc->dc;
   gc->dc = evas_common_draw_context_new();
   evas_common_draw_context_set_multiplier(gc->dc, cmd->draw.R, cmd->draw.G, cmd->draw.B, cmd->draw.A);

   if (cmd->input == cmd->output)
     gc->dc->render_op = EVAS_RENDER_COPY;
   else
     gc->dc->render_op = _gfx_to_evas_render_op(cmd->draw.rop);

   evas_gl_common_filter_inverse_color_push(gc, image->tex, 0, 0, w, h);

   evas_common_draw_context_free(gc->dc);
   gc->dc = dc_save;

   evas_ector_buffer_engine_image_release(cmd->input->buffer, image);
   evas_ector_buffer_engine_image_release(cmd->output->buffer, surface);

   return EINA_TRUE;
}

/**
 * @brief Get the inverse color filter function.
 *
 * @param re The render engine data (unused).
 * @param cmd The filter command.
 * @return The function pointer to apply the filter, or NULL on failure.
 *
 * This function acts as a factory for the inverse color filter.
 * It validates the provided filter command and, if valid, returns
 * a pointer to the internal function _gl_filter_inverse_color(),
 * which performs the actual filtering operation.
 */
GL_Filter_Apply_Func
gl_filter_inverse_color_func_get(Render_Engine_GL_Generic *re EINA_UNUSED, Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);

   return _gl_filter_inverse_color;
}
