#include "gl_engine_filter.h"

/**
 * @internal
 * @brief Applies a grayscale filter to an image buffer using OpenGL.
 *
 * This function is the core implementation of the grayscale filter. It sets up
 * the OpenGL context, binds the input and output buffers, and invokes the
 * specific GL routine to perform the grayscale conversion.
 *
 * It handles both in-place (input buffer is the same as output buffer) and
 * out-of-place filtering. For in-place operations, EVAS_RENDER_COPY is used.
 *
 * @param[in] re The GL render engine instance.
 * @param[in] cmd The filter command containing all necessary data like
 *                input/output buffers, dimensions, and drawing parameters.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_gl_filter_grayscale(Render_Engine_GL_Generic *re, Evas_Filter_Command *cmd)
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

   evas_gl_common_filter_grayscale_push(gc, image->tex, 0, 0, w, h);

   evas_common_draw_context_free(gc->dc);
   gc->dc = dc_save;

   evas_ector_buffer_engine_image_release(cmd->input->buffer, image);
   evas_ector_buffer_engine_image_release(cmd->output->buffer, surface);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Retrieves the function pointer for the grayscale filter.
 *
 * This function acts as a factory for the grayscale filter. Based on the
 * provided command, it can potentially return different filter implementations.
 * In this case, it always returns _gl_filter_grayscale.
 *
 * It performs basic validation on the filter command to ensure that input
 * and output buffers are valid before returning the filter function.
 *
 * @param[in] re The GL render engine instance (unused).
 * @param[in] cmd The filter command to be executed.
 * @return A function pointer to the correct grayscale filter implementation
 *         (GL_Filter_Apply_Func), or @c NULL if the command is invalid.
 */
GL_Filter_Apply_Func
gl_filter_grayscale_func_get(Render_Engine_GL_Generic *re EINA_UNUSED, Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);

   return _gl_filter_grayscale;
}
