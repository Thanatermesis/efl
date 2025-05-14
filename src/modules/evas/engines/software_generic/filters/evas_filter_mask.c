/* Implementation of some masking functions for the software engine */

#include "evas_engine_filter.h"

/** @internal
 * @brief Mask function for Alpha input, Alpha mask, Alpha output. */
static Eina_Bool _mask_cpu_alpha_alpha_alpha(Evas_Filter_Command *cmd);
/** @internal
 * @brief Mask function for Alpha input, RGBA mask, RGBA output. */
static Eina_Bool _mask_cpu_alpha_rgba_rgba(Evas_Filter_Command *cmd);
/** @internal
 * @brief Mask function for Alpha input, Alpha mask, RGBA output. */
static Eina_Bool _mask_cpu_alpha_alpha_rgba(Evas_Filter_Command *cmd);
/** @internal
 * @brief Mask function for RGBA input, Alpha mask, RGBA output. */
static Eina_Bool _mask_cpu_rgba_alpha_rgba(Evas_Filter_Command *cmd);
/** @internal
 * @brief Mask function for RGBA input, RGBA mask, RGBA output. */
static Eina_Bool _mask_cpu_rgba_rgba_rgba(Evas_Filter_Command *cmd);

/**
 * @brief Selects the appropriate CPU-based mask function based on buffer formats.
 *
 * This function examines the input, output, and mask buffers specified in the
 * filter command @p cmd. Based on whether these buffers are alpha-only or RGBA,
 * it returns a pointer to the specialized CPU function that can handle that
 * specific combination efficiently.
 *
 * @param cmd The filter command containing buffer information and parameters.
 * @return A function pointer to the appropriate mask implementation, or NULL on error
 *         or if no suitable function is found.
 */
Software_Filter_Func
eng_filter_mask_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->mask, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((cmd->input->w > 0) && (cmd->input->h > 0), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((cmd->mask->w > 0) && (cmd->mask->h > 0), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(cmd->input->w == cmd->output->w, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(cmd->input->h == cmd->output->h, NULL);

   if (cmd->input->alpha_only)
     {
        if (cmd->output->alpha_only)
          {
             if (!cmd->mask->alpha_only)
               {
                  DBG("Input and output are Alpha but mask is RGBA. This is not "
                      "optimal (implicit conversion and loss of color).");
               }
             return _mask_cpu_alpha_alpha_alpha;
          }
        else if (!cmd->mask->alpha_only)
          return _mask_cpu_alpha_rgba_rgba;
        else
          return _mask_cpu_alpha_alpha_rgba;
     }
   else
     {
        if (!cmd->output->alpha_only)
          {
             // rgba -> rgba
             if (cmd->mask->alpha_only)
               return _mask_cpu_rgba_alpha_rgba;
             else
               return _mask_cpu_rgba_rgba_rgba;
          }
        else
          {
             // rgba -> alpha
             DBG("Input is RGBA but output is Alpha, losing colors.");
             return _mask_cpu_alpha_alpha_alpha;
          }
     }
}

/**
 * @internal
 * @brief Applies an alpha mask to an alpha input buffer, producing an alpha output.
 *
 * This function implements masking where input, mask, and output are all
 * single-channel alpha buffers (uint8_t arrays).
 *
 * Mechanism:
 * 1. Stretch mask if required by fillmode.
 * 2. Copy source alpha buffer to destination alpha buffer (if they are different).
 * 3. Apply the mask alpha buffer to the destination buffer using the specified
 *    render operation (@ref Efl_Gfx_Render_Op) via @ref efl_draw_alpha_func_get.
 *    The mask is tiled horizontally if its width is smaller than the input/output width.
 *    The mask is tiled vertically if its height is smaller than the input/output height.
 *
 * @param cmd The filter command containing buffer information (input, mask, output),
 *            dimensions, render operation, and fill mode.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_mask_cpu_alpha_alpha_alpha(Evas_Filter_Command *cmd)
{
   unsigned int src_len = 0, src_stride, msk_len = 0, msk_stride, dst_len = 0, dst_stride;
   Efl_Gfx_Render_Op render_op = cmd->draw.rop;
   Evas_Filter_Buffer *msk_fb;
   Draw_Func_Alpha func;
   uint8_t *src_map = NULL, *dst, *dst_map = NULL, *msk, *msk_map = NULL;
   int w, h, mw, mh, x, y, my;
   int stepsize, stepcount, step;
   Eina_Bool ret = EINA_FALSE;

   /* Mechanism:
    * 1. Stretch mask as requested in fillmode
    * 2. Copy source to destination
    * 3. Render mask into destination using alpha function
    *
    * FIXME: Could probably be optimized into a single op :)
    */

   w = cmd->input->w;
   h = cmd->input->h;
   mw = cmd->mask->w;
   mh = cmd->mask->h;
   stepsize  = MIN(mw, w);
   stepcount = w / stepsize;

   // Stretch if necessary.
   if ((mw != w || mh != h) && (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_XY))
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->ctx->buffer_scaled_get, EINA_FALSE);

        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_X)
          mw = w;
        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_Y)
          mh = h;

        BUFFERS_LOCK();
        msk_fb = cmd->ctx->buffer_scaled_get(cmd->ctx, cmd->mask, mw, mh);
        BUFFERS_UNLOCK();

        EINA_SAFETY_ON_NULL_RETURN_VAL(msk_fb, EINA_FALSE);
        msk_fb->locked = EINA_FALSE;
     }
   else msk_fb = cmd->mask;

   msk_map = msk = _buffer_map_all(msk_fb->buffer, &msk_len, E_READ, E_ALPHA, &msk_stride);
   dst_map = dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ALPHA, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(dst_map && msk_map, end);

   // First pass: copy to dest
   if (cmd->input->buffer != cmd->output->buffer)
     {
        src_map = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ALPHA, &src_stride);
        EINA_SAFETY_ON_FALSE_GOTO(src_map, end);
        if (dst_stride == src_stride)
          memcpy(dst_map, src_map, dst_stride * h * sizeof(uint8_t));
        else
          {
             for (y = 0; y < h; y++)
               memcpy(dst_map + (y * dst_stride), src_map + (y * src_stride),
                      MIN(dst_stride, src_stride) * h * sizeof(uint8_t));
          }
     }

   // Second pass: apply render op
   func = efl_draw_alpha_func_get(render_op, EINA_FALSE);
   for (y = 0, my = 0; y < h; y++, my++)
     {
        if (my >= mh) my = 0;

        msk = msk_map + (my * msk_stride);
        dst = dst_map + (y * dst_stride);

        for (step = 0; step < stepcount; step++, dst += stepsize)
          func(dst, msk, stepsize);

        x = stepsize * stepcount;
        if (x < w)
          {
             func(dst, msk, w - x);
          }
     }

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(msk_fb->buffer, msk_map, msk_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}

/**
 * @internal
 * @brief Applies an alpha mask to an RGBA input buffer, producing an RGBA output.
 *
 * This function handles masking where the input and output are RGBA (uint32_t arrays),
 * but the mask is alpha-only (uint8_t array). It cleverly reuses the
 * `_mask_cpu_alpha_rgba_rgba` function by temporarily swapping the roles
 * of the input and mask buffers within the command structure.
 *
 * Mechanism:
 * 1. Temporarily swap `cmd->input` (RGBA) and `cmd->mask` (Alpha).
 * 2. Call `_mask_cpu_alpha_rgba_rgba()`. This function now sees an Alpha "input"
 *    (originally the mask) and an RGBA "mask" (originally the input).
 * 3. Swap `cmd->input` and `cmd->mask` back to restore the original configuration.
 *
 * @param cmd The filter command containing buffer information (input, mask, output),
 *            dimensions, render operation, and fill mode.
 * @return EINA_TRUE on success, EINA_FALSE otherwise (inherited from the called function).
 */
static Eina_Bool
_mask_cpu_rgba_alpha_rgba(Evas_Filter_Command *cmd)
{
   Evas_Filter_Buffer *fb;
   Eina_Bool ok;

   /* Mechanism:
    * 1. Swap input and mask
    * 2. Apply mask operation for alpha+rgba+rgba
    * 3. Swap input and mask
    */

   fb = cmd->input;
   cmd->input = cmd->mask;
   cmd->mask = fb;

   ok = _mask_cpu_alpha_rgba_rgba(cmd);

   fb = cmd->input;
   cmd->input = cmd->mask;
   cmd->mask = fb;

   return ok;
}

/**
 * @internal
 * @brief Applies an RGBA mask to an alpha input buffer, producing an RGBA output.
 *
 * This function handles masking where the input is alpha-only (uint8_t array),
 * but the mask and output are RGBA (uint32_t arrays).
 *
 * Mechanism:
 * 1. Stretch the RGBA mask if required by fillmode.
 * 2. Process line by line, potentially in horizontal steps (`stepsize`):
 *    a. Use `efl_draw_func_mask_span_get` to render the alpha input line segment
 *       into a temporary RGBA `span` buffer. This effectively converts the alpha
 *       value into an RGBA pixel (likely modulating white: 0xFFFFFF).
 *    b. Use `efl_draw_func_argb_mix3_get` to mix three sources into the
 *       destination buffer segment: the generated `span`, the corresponding
 *       RGBA mask segment, and the original destination content (implicitly,
 *       as the function modifies `dst` in place). The command's draw color
 *       (`cmd->draw.{A,R,G,B}`) is also used in the mix operation.
 *    c. The mask is tiled vertically if its height is smaller than the input/output height.
 *
 * @param cmd The filter command containing buffer information (input, mask, output),
 *            dimensions, render operation, fill mode, and draw color.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_mask_cpu_alpha_rgba_rgba(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, msk_len, msk_stride, dst_len, dst_stride;
   Efl_Gfx_Render_Op op = cmd->draw.rop;
   Evas_Filter_Buffer *msk_fb;
   RGBA_Comp_Func_Mask func1;
   Draw_Func_ARGB_Mix3 func2;
   uint8_t *src, *src_map = NULL, *msk_map = NULL, *dst_map = NULL;
   uint32_t *dst, *msk, *span;
   int w, h, mw, mh, x, y, my;
   int stepsize, stepcount, step;
   Eina_Bool ret = EINA_FALSE;
   uint32_t color;

   /* Mechanism:
    * 1. Stretch mask as requested in fillmode
    * 2. Render input to span using (using "mask" function)
    * 3. Mix 3 RGBA images together into dest: span, mask, dest
    *
    * FIXME: Could probably be optimized into a single op :)
    */

   w = cmd->input->w;
   h = cmd->input->h;
   mw = cmd->mask->w;
   mh = cmd->mask->h;
   color = ARGB_JOIN(cmd->draw.A, cmd->draw.R, cmd->draw.G, cmd->draw.B);
   stepsize  = MIN(mw, w);
   stepcount = w / stepsize;
   span = alloca(stepsize * sizeof(uint32_t));

   // Stretch if necessary.
   if ((mw != w || mh != h) && (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_XY))
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->ctx->buffer_scaled_get, EINA_FALSE);

        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_X)
          mw = w;
        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_Y)
          mh = h;

        BUFFERS_LOCK();
        msk_fb = cmd->ctx->buffer_scaled_get(cmd->ctx, cmd->mask, mw, mh);
        BUFFERS_UNLOCK();

        EINA_SAFETY_ON_NULL_RETURN_VAL(msk_fb, EINA_FALSE);
        msk_fb->locked = EINA_FALSE;
     }
   else msk_fb = cmd->mask;

   src_map = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ALPHA, &src_stride);
   msk_map = _buffer_map_all(msk_fb->buffer, &msk_len, E_READ, E_ARGB, &msk_stride);
   dst_map = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ARGB, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src_map && dst_map && msk_map, end);

   func1 = efl_draw_func_mask_span_get(op, 0xFFFFFFFF);
   func2 = efl_draw_func_argb_mix3_get(op, color);
   EINA_SAFETY_ON_FALSE_GOTO(func1 && func2, end);

   // Apply mask using Gfx functions
   for (y = 0, my = 0; y < h; y++, my++)
     {
        int len;

        if (my >= mh) my = 0;

        len = stepsize;
        src = src_map + (y * src_stride);
        msk = (uint32_t *) (msk_map + (my * msk_stride));
        dst = (uint32_t *) (dst_map + (y * dst_stride));

        for (step = 0; step < stepcount; step++, dst += stepsize, src += stepsize)
          {
             memset(span, 0, len * sizeof(uint32_t));
             func1(span, src, len, 0xFFFFFFFF);
             func2(dst, span, msk, len, color);
          }

        x = stepsize * stepcount;
        if (x < w)
          {
             len = w - x;
             memset(span, 0, len * sizeof(uint32_t));
             func1(span, src, len, 0xFFFFFFFF);
             func2(dst, span, msk, len, color);
          }
     }

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(msk_fb->buffer, msk_map, msk_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}

/**
 * @internal
 * @brief Applies an alpha mask to an alpha input buffer, producing an RGBA output.
 *
 * This function handles masking where both input and mask are alpha-only
 * (uint8_t arrays), but the output is RGBA (uint32_t array).
 *
 * Mechanism:
 * 1. Stretch the alpha mask if required by fillmode.
 * 2. Process line by line, potentially in horizontal steps (`stepsize`):
 *    a. Copy the alpha mask line segment into a temporary alpha `span` buffer.
 *    b. Use `efl_draw_alpha_func_get` (with `mul = EINA_TRUE`) to multiply the
 *       source alpha line segment with the `span` buffer. The `span` now holds
 *       the product of the source alpha and mask alpha for that segment.
 *    c. Use `efl_draw_func_mask_span_get` to render the resulting alpha `span`
 *       into the RGBA destination buffer segment, using the command's draw
 *       color (`cmd->draw.{A,R,G,B}`).
 *    d. The mask is tiled vertically if its height is smaller than the input/output height.
 *
 * @param cmd The filter command containing buffer information (input, mask, output),
 *            dimensions, render operation, fill mode, and draw color.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_mask_cpu_alpha_alpha_rgba(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, msk_len, msk_stride, dst_len, dst_stride;
   uint8_t *src, *msk, *span, *src_map = NULL, *msk_map = NULL, *dst_map = NULL;
   Evas_Filter_Buffer *msk_fb;
   Draw_Func_Alpha func1;
   RGBA_Comp_Func_Mask func2;
   uint32_t *dst;
   uint32_t color;
   Efl_Gfx_Render_Op op = cmd->draw.rop;
   int w, h, mw, mh, x, y, my;
   int stepsize, stepcount, step;
   Eina_Bool ret = EINA_FALSE;

   /* Mechanism:
    * 1. Copy mask to span buffer (1 line)
    * 2. Multiply source by span (so that: span = mask * source)
    * 3. Render span to destination using color (blend)
    *
    * FIXME: Could probably be optimized into a single op :)
    */

   w = cmd->input->w;
   h = cmd->input->h;
   mw = cmd->mask->w;
   mh = cmd->mask->h;
   color = ARGB_JOIN(cmd->draw.A, cmd->draw.R, cmd->draw.G, cmd->draw.B);
   stepsize  = MIN(mw, w);
   stepcount = w / stepsize;
   span = alloca(stepsize * sizeof(uint32_t));

   // Stretch if necessary.
   if ((mw != w || mh != h) && (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_XY))
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->ctx->buffer_scaled_get, EINA_FALSE);

        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_X)
          mw = w;
        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_Y)
          mh = h;

        BUFFERS_LOCK();
        msk_fb = cmd->ctx->buffer_scaled_get(cmd->ctx, cmd->mask, mw, mh);
        BUFFERS_UNLOCK();

        EINA_SAFETY_ON_NULL_RETURN_VAL(msk_fb, EINA_FALSE);
        msk_fb->locked = EINA_FALSE;
     }
   else msk_fb = cmd->mask;

   src_map = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ALPHA, &src_stride);
   msk_map = _buffer_map_all(msk_fb->buffer, &msk_len, E_READ, E_ALPHA, &msk_stride);
   dst_map = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ARGB, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src_map && dst_map && msk_map, end);

   func1 = efl_draw_alpha_func_get(op, EINA_TRUE);
   func2 = efl_draw_func_mask_span_get(op, color);
   EINA_SAFETY_ON_FALSE_GOTO(func1 && func2, end);

   for (y = 0, my = 0; y < h; y++, my++)
     {
        int len;

        if (my >= mh) my = 0;

        len = stepsize;
        src = src_map + (y * src_stride);
        msk = msk_map + (my * msk_stride);
        dst = (uint32_t *) (dst_map + (y * dst_stride));

        for (step = 0; step < stepcount; step++, dst += stepsize, src += stepsize)
          {
             memcpy(span, msk, len * sizeof(uint8_t));
             func1(span, src, len);
             func2(dst, span, len, color);
          }

        x = stepsize * stepcount;
        if (x < w)
          {
             len = w - x;
             memcpy(span, msk, len * sizeof(uint8_t));
             func1(span, src, len);
             func2(dst, span, len, color);
          }
     }

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(msk_fb->buffer, msk_map, msk_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}

/**
 * @internal
 * @brief Applies an RGBA mask to an RGBA input buffer, producing an RGBA output.
 *
 * This function handles masking where input, mask, and output are all RGBA
 * (uint32_t arrays).
 *
 * Mechanism:
 * 1. Stretch the RGBA mask if required by fillmode.
 * 2. Process line by line, potentially in horizontal steps (`stepsize`):
 *    a. Use `efl_draw_func_argb_mix3_get` to mix three sources into the
 *       destination buffer segment: the RGBA input segment, the RGBA mask
 *       segment, and the original destination content (implicitly, as the
 *       function modifies `dst` in place). The command's draw color
 *       (`cmd->draw.{A,R,G,B}`) is also used in the mix operation.
 *    b. The mask is tiled vertically if its height is smaller than the input/output height.
 *
 * @param cmd The filter command containing buffer information (input, mask, output),
 *            dimensions, render operation, fill mode, and draw color.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_mask_cpu_rgba_rgba_rgba(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, msk_len, msk_stride, dst_len, dst_stride;
   uint8_t *src_map = NULL, *msk_map = NULL, *dst_map = NULL;
   Draw_Func_ARGB_Mix3 func;
   Evas_Filter_Buffer *msk_fb;
   uint32_t *dst, *msk, *src;
   int w, h, mw, mh, x, y, my;
   int stepsize, stepcount, step;
   Eina_Bool ret = EINA_FALSE;
   uint32_t color;

   /* Mechanism:
    * 1. Stretch mask as requested in fillmode
    * 2. Mix 3 colors
    */

   w = cmd->input->w;
   h = cmd->input->h;
   mw = cmd->mask->w;
   mh = cmd->mask->h;
   color = ARGB_JOIN(cmd->draw.A, cmd->draw.R, cmd->draw.G, cmd->draw.B);
   stepsize  = MIN(mw, w);
   stepcount = w / stepsize;

   // Stretch if necessary.
   if ((mw != w || mh != h) && (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_XY))
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->ctx->buffer_scaled_get, EINA_FALSE);

        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_X)
          mw = w;
        if (cmd->draw.fillmode & EVAS_FILTER_FILL_MODE_STRETCH_Y)
          mh = h;

        BUFFERS_LOCK();
        msk_fb = cmd->ctx->buffer_scaled_get(cmd->ctx, cmd->mask, mw, mh);
        BUFFERS_UNLOCK();

        EINA_SAFETY_ON_NULL_RETURN_VAL(msk_fb, EINA_FALSE);
        msk_fb->locked = EINA_FALSE;
     }
   else msk_fb = cmd->mask;

   src_map = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ARGB, &src_stride);
   msk_map = _buffer_map_all(msk_fb->buffer, &msk_len, E_READ, E_ARGB, &msk_stride);
   dst_map = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ARGB, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src_map && dst_map && msk_map, end);

   func = efl_draw_func_argb_mix3_get(cmd->draw.rop, color);

   // Apply mask using Gfx functions
   for (y = 0, my = 0; y < h; y++, my++)
     {
        if (my >= mh) my = 0;

        src = (uint32_t *) (src_map + (y * src_stride));
        msk = (uint32_t *) (msk_map + (my * msk_stride));
        dst = (uint32_t *) (dst_map + (y * dst_stride));

        for (step = 0; step < stepcount; step++, dst += stepsize, src += stepsize)
          func(dst, src, msk, stepsize, color);

        x = stepsize * stepcount;
        if (x < w)
          {
             func(dst, src, msk, w - x, color);
          }
     }

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(msk_fb->buffer, msk_map, msk_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}
