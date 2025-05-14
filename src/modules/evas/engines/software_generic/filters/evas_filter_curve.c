#include "evas_engine_filter.h"

/**
 * @file
 * @brief CPU implementation for the curve filter in Evas.
 *
 * This file contains the software (CPU) rendering functions for applying
 * color channel curve adjustments to image buffers. It supports RGBA and
 * Alpha-only formats.
 */

/**
 * @brief Applies a color curve filter to an RGBA buffer using the CPU.
 * @param cmd The filter command containing input/output buffers and curve data.
 *        - cmd->input->buffer: Source Ector_Buffer.
 *        - cmd->output->buffer: Destination Ector_Buffer.
 *        - cmd->curve.data: A 256-byte array mapping input intensity to output intensity.
 *                           Example: curve[128] = 200 means input intensity 128 maps to 200.
 *        - cmd->curve.channel: The channel(s) to apply the curve to (RED, GREEN, BLUE, ALPHA, RGB).
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * This function maps the input and output buffers, applies the curve transformation
 * based on the specified channel(s), handling byte order differences, and manages
 * pre-multiplied alpha conversions.
 */
static Eina_Bool
_filter_curve_cpu_rgba(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, dst_len, dst_stride;
   void *src_map = NULL, *dst_map;
   Eina_Bool ret = EINA_FALSE;
   uint32_t *src, *dst, *d, *s;
   uint8_t *curve;
   int k, offset = -1, len;

   /** Macro to access the byte corresponding to the selected channel within a 32-bit pixel. */
#define C_VAL(p) (((uint8_t *)(p))[offset])

   // FIXME: support src_stride != dst_stride
   // Note: potentially mapping the same region twice (read then write)
   src_map = src = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ARGB, &src_stride);
   dst_map = dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ARGB, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src && dst && (src_len == dst_len), end);

   curve = cmd->curve.data;
   len = dst_len / sizeof(uint32_t); // Number of pixels

   // Determine the byte offset within a 32-bit pixel (uint32_t) for the target channel(s).
   // This depends on the system's endianness (WORDS_BIGENDIAN).
   // For ARGB format:
   // Little Endian (memory): B G R A  (offsets 0 1 2 3)
   // Big Endian (memory):    A R G B  (offsets 0 1 2 3)
   switch (cmd->curve.channel)
     {
#ifndef WORDS_BIGENDIAN // Little Endian (e.g., x86)
      case EVAS_FILTER_CHANNEL_RED:   offset = 2; break; // 0xAARRGGBB -> R is at byte 2
      case EVAS_FILTER_CHANNEL_GREEN: offset = 1; break; // 0xAARRGGBB -> G is at byte 1
      case EVAS_FILTER_CHANNEL_BLUE:  offset = 0; break; // 0xAARRGGBB -> B is at byte 0
#else // Big Endian (e.g., PowerPC)
      case EVAS_FILTER_CHANNEL_RED:   offset = 1; break; // 0xAARRGGBB -> R is at byte 1
      case EVAS_FILTER_CHANNEL_GREEN: offset = 2; break; // 0xAARRGGBB -> G is at byte 2
      case EVAS_FILTER_CHANNEL_BLUE:  offset = 3; break; // 0xAARRGGBB -> B is at byte 3
#endif
      case EVAS_FILTER_CHANNEL_ALPHA: break; // Handled separately later
      case EVAS_FILTER_CHANNEL_RGB: break;   // Handled separately later
      default:
        ERR("Invalid color channel %d", (int) cmd->curve.channel);
        goto end;
     }

   // If input and output buffers are different, copy data first.
   if (src != dst)
     memcpy(dst, src, dst_len);

   // Unpremultiply alpha for accurate color channel manipulation.
   efl_draw_argb_unpremul(dst, len);

   // Apply curve to a single color channel (R, G, or B) if offset was set.
   if (offset >= 0)
     {
        // Iterate through pixels and apply curve using the calculated offset.
        for (k = len, s = src, d = dst; k; k--, d++, s++)
          C_VAL(d) = curve[C_VAL(s)];

        goto premul; // Skip other channel processing
     }

   // Apply curve to all RGB channels.
   if (cmd->curve.channel == EVAS_FILTER_CHANNEL_RGB)
     {
        // Loop through R, G, B offsets based on endianness.
#ifndef WORDS_BIGENDIAN // Little Endian: B=0, G=1, R=2
        for (offset = 0; offset <= 2; offset++)
#else // Big Endian: R=1, G=2, B=3
        for (offset = 1; offset <= 3; offset++)
#endif
          {
             // Apply curve to the current channel (offset) for all pixels.
             for (k = len, s = src, d = dst; k; k--, d++, s++)
               C_VAL(d) = curve[C_VAL(s)];
          }

        goto premul; // Skip alpha processing
     }

   // Apply curve to the Alpha channel.
   // Calculate alpha offset based on endianness.
#ifndef WORDS_BIGENDIAN // Little Endian: A=3
   offset = 3;
#else // Big Endian: A=0
   offset = 0;
#endif

   // Iterate through pixels and apply curve to the alpha channel.
   // Note: src is incremented here, but it's okay as it's the last use before premul/end.
   for (k = len, d = dst; k; k--, d++, src++)
     C_VAL(d) = curve[C_VAL(src)];

premul:
   // Premultiply alpha back after modifications.
   efl_draw_argb_premul(dst, len);
   ret = EINA_TRUE;

end:
   // Unmap buffers.
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}

/**
 * @brief Applies a curve filter to an Alpha-only buffer using the CPU.
 * @param cmd The filter command containing input/output buffers and curve data.
 *        - cmd->input->buffer: Source Ector_Buffer (Alpha format).
 *        - cmd->output->buffer: Destination Ector_Buffer (Alpha format).
 *        - cmd->curve.data: A 256-byte array mapping input alpha to output alpha.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 *
 * This function maps the input and output alpha buffers and applies the curve
 * transformation directly byte by byte.
 */
static Eina_Bool
_filter_curve_cpu_alpha(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, dst_len, dst_stride;
   uint8_t *src, *dst, *curve;
   void *src_map, *dst_map;
   Eina_Bool ret = EINA_FALSE;
   int k;

   // FIXME: support src_stride != dst_stride
   // Note: potentially mapping the same region twice (read then write)
   src_map = src = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, E_ALPHA, &src_stride);
   dst_map = dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, E_ALPHA, &dst_stride);
   EINA_SAFETY_ON_FALSE_GOTO(src && dst && (src_len == dst_len), end);
   curve = cmd->curve.data;

   for (k = src_len; k; k--)
     *dst++ = curve[*src++];

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, src_map, src_len);
   ector_buffer_unmap(cmd->output->buffer, dst_map, dst_len);
   return ret;
}

/**
 * @brief Gets the appropriate CPU function for the curve filter based on buffer formats.
 * @param cmd The filter command containing input and output buffer information.
 * @return A function pointer to the correct CPU implementation (_filter_curve_cpu_rgba or
 *         _filter_curve_cpu_alpha), or NULL if inputs are invalid or formats incompatible
 *         in a way that can't be handled by implicit conversion.
 */
Software_Filter_Func
eng_filter_curve_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((cmd->input->w == cmd->output->w)
                                   && (cmd->input->h == cmd->output->h), NULL);

   // Both buffers are RGBA (or compatible).
   if (!cmd->input->alpha_only && !cmd->output->alpha_only)
     return _filter_curve_cpu_rgba;

   // Both buffers are Alpha-only.
   if (cmd->input->alpha_only && cmd->output->alpha_only)
     return _filter_curve_cpu_alpha;

   // Mismatched formats (e.g., RGBA -> Alpha or Alpha -> RGBA).
   // The curve filter inherently expects matching channel counts for meaningful operation.
   // While Ector might handle the buffer conversion, the filter's effect might be
   // unexpected. We proceed with the RGBA function, relying on Ector's conversion,
   // but issue a warning.
   WRN("Incompatible image formats for curve filter (input alpha: %d, output alpha: %d). "
       "Relying on implicit buffer conversion.",
       cmd->input->alpha_only, cmd->output->alpha_only);
   return _filter_curve_cpu_rgba;
}
