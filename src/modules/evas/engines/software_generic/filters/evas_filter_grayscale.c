#include "evas_engine_filter.h"

/**
 * @internal
 * @brief Applies a grayscale filter to an input buffer and writes the result to an output buffer.
 *
 * This function reads pixel data from the input buffer specified in the command,
 * converts each pixel to grayscale using a standard luminance formula, and writes
 * the resulting grayscale pixel (with the original alpha) to the output buffer.
 *
 * The grayscale conversion formula used is:
 * gry = ((R * 19596) + (G * 38470) + (B * 7472)) >> 16
 * This is equivalent to Y = 0.299*R + 0.587*G + 0.114*B, scaled for integer arithmetic.
 *
 * Buffers are mapped for reading (input) and writing (output). Proper error handling
 * ensures buffers are unmapped even if processing fails.
 *
 * @param cmd Pointer to the Evas_Filter_Command structure containing input/output buffers
 *            and filter parameters. The input buffer contains the source ARGB image data.
 *            The output buffer will store the resulting ARGB grayscale image data.
 *            Example `cmd->input->buffer`: Contains pixels like [A1 R1 G1 B1, A2 R2 G2 B2, ...]
 *            Example `cmd->output->buffer`: Will contain pixels like [A1 GY1 GY1 GY1, A2 GY2 GY2 GY2, ...]
 *            where GY is the calculated grayscale value.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid buffer dimensions,
 *         failed buffer mapping).
 */
static Eina_Bool
_evas_filter_grayscale(Evas_Filter_Command *cmd)
{
   int sw, sh, dw, dh, x, y, slen, dlen;
   unsigned int src_len, src_stride, dst_len, dst_stride;
   Eina_Bool ret = EINA_FALSE;
   DATA32 *ts, *td, *src = NULL, *dst = NULL;
   DATA8 r, g, b, gry;

   ector_buffer_size_get(cmd->input->buffer, &sw, &sh);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((sw > 0) && (sh > 0), ret);

   ector_buffer_size_get(cmd->output->buffer, &dw, &dh);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((dw > 0) && (dh > 0), ret);

   src = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ARGB, &src_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src, end);

   dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ARGB, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(dst, end);

   slen = src_stride / sizeof(*src);
   dlen = dst_stride / sizeof(*dst);

   ts = src;
   td = dst;
   for (y = 0; y < sh ; y++)
     {
        for (x = 0; x < sw; x++)
          {
             A_VAL(td + x) = A_VAL(ts + x);
             r = R_VAL(ts + x);
             g = G_VAL(ts + x);
             b = B_VAL(ts + x);

             /* formula from evas_common_convert_rgba_to_8bpp_gry_256_dith */
             gry = ((r * 19596) + (g * 38470) + (b * 7472)) >> 16;
             R_VAL(td + x) = G_VAL(td + x) = B_VAL(td + x) = gry;
          }
          ts += slen;
          td += dlen;
     }

   ret = EINA_TRUE;

end:
   if (src) ector_buffer_unmap(cmd->input->buffer, src, src_len);
   if (dst) ector_buffer_unmap(cmd->output->buffer, dst, dst_len);
   return ret;
}

Software_Filter_Func
eng_filter_grayscale_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);

   return _evas_filter_grayscale;
}
