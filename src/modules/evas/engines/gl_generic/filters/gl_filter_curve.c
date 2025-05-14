#include "gl_engine_filter.h"

/**
 * @file
 * @brief This file implements the curve filter for the GL engine.
 */

/**
 * @internal
 * @brief Applies a curve filter to an image.
 *
 * This function takes an input image, applies a color curve transformation
 * based on the provided points and channel, and writes the result to the
 * output surface.
 *
 * @param re The GL generic render engine.
 * @param cmd The filter command containing input, output, and curve parameters.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_gl_filter_curve(Render_Engine_GL_Generic *re, Evas_Filter_Command *cmd)
{
   Evas_Engine_GL_Context *gc;
   Evas_GL_Image *image, *surface;
   RGBA_Draw_Context *dc_save;
   const uint8_t *points;
   int channel;
   int w, h;

   DEBUG_TIME_BEGIN();

   w = cmd->input->w;
   h = cmd->input->h;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(w == cmd->output->w, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(h == cmd->output->h, EINA_FALSE);

   image = evas_ector_buffer_drawable_image_get(cmd->input->buffer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(image, EINA_FALSE);

   surface = evas_ector_buffer_render_image_get(cmd->output->buffer);
   EINA_SAFETY_ON_NULL_RETURN_VAL(surface, EINA_FALSE);

   DBG("curve %d @%p -> %d @%p", cmd->input->id, cmd->input->buffer,
       cmd->output->id, cmd->output->buffer);

   gc = gl_generic_context_find(re, 1);
   evas_gl_common_context_target_surface_set(gc, surface);

   dc_save = gc->dc;
   gc->dc = evas_common_draw_context_new();
   evas_common_draw_context_set_multiplier(gc->dc, cmd->draw.R, cmd->draw.G, cmd->draw.B, cmd->draw.A);
   evas_common_draw_context_clip_clip(gc->dc, 0, 0, w, h);
   if (cmd->input == cmd->output)
     gc->dc->render_op = EVAS_RENDER_COPY;

   points = cmd->curve.data;
   channel = (int) cmd->curve.channel;
   if (cmd->input->alpha_only)
     channel = 5;

   evas_gl_common_filter_curve_push(gc, image->tex, 0, 0, w, h, points, channel);

   evas_common_draw_context_free(gc->dc);
   gc->dc = dc_save;

   evas_ector_buffer_engine_image_release(cmd->input->buffer, image);
   evas_ector_buffer_engine_image_release(cmd->output->buffer, surface);

   DEBUG_TIME_END();

   return EINA_TRUE;
}

/**
 * @brief Gets the function pointer for applying the curve filter.
 *
 * This function validates the filter command and its parameters. If valid,
 * it returns a pointer to the _gl_filter_curve function.
 *
 * @param re The GL generic render engine (unused).
 * @param cmd The filter command to validate.
 * @return A function pointer to _gl_filter_curve if the command is valid,
 *         otherwise @c NULL.
 *
 * @note The `cmd->curve.data` is an array of 256 `uint8_t` values representing
 *       the curve points for the selected channel. For example, if `cmd->curve.channel`
 *       is `EVAS_FILTER_CHANNEL_R`, then `cmd->curve.data` contains 256 values
 *       for the red channel curve.
 *       - `cmd->curve.data[0]` is the output value for input value 0.
 *       - `cmd->curve.data[128]` is the output value for input value 128.
 *       - `cmd->curve.data[255]` is the output value for input value 255.
 */
GL_Filter_Apply_Func
gl_filter_curve_func_get(Render_Engine_GL_Generic *re EINA_UNUSED, Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->curve.data, NULL);

   return _gl_filter_curve;
}
