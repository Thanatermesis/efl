#include "evas_engine_filter.h"

/**
 * @file
 * @brief Implements the fill filter operation for the software engine.
 */

/**
 * @brief Fills a rectangular area of the output buffer with a solid color using CPU.
 *
 * This function calculates the target rectangle based on the command's draw
 * context (clip rectangle or LRTB margins) and fills it with the specified
 * color (or alpha value if alpha_only).
 *
 * @param cmd The filter command containing parameters like output buffer,
 *            color, and clipping information.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., buffer mapping failed).
 */
static Eina_Bool
_fill_cpu(Evas_Filter_Command *cmd)
{
   Evas_Filter_Buffer *fb = cmd->output;
   int step = fb->alpha_only ? sizeof(uint8_t) : sizeof(uint32_t);
   int x = MAX(0, cmd->draw.clip.x);
   int y = MAX(0, cmd->draw.clip.y);
   uint32_t color = ARGB_JOIN(cmd->draw.A, cmd->draw.R, cmd->draw.G, cmd->draw.B);
   unsigned int stride, len;
   int w, h, k;
   uint8_t *map, *ptr;

   if (!cmd->draw.clip_mode_lrtb)
     {
        if (cmd->draw.clip.w)
          w = MIN(cmd->draw.clip.w, fb->w - x);
        else
          w = fb->w - x;
        if (cmd->draw.clip.h)
          h = MIN(cmd->draw.clip.h, fb->h - y);
        else
          h = fb->h - y;
     }
   else
     {
        x = MAX(0, cmd->draw.clip.l);
        y = MAX(0, cmd->draw.clip.t);
        w = CLAMP(0, fb->w - x - cmd->draw.clip.r, fb->w - x);
        h = CLAMP(0, fb->h - y - cmd->draw.clip.b, fb->h - y);
     }

   map = _buffer_map_all(fb->buffer, &len, E_WRITE, fb->alpha_only ? E_ALPHA : E_ARGB, &stride);
   if (!map) return EINA_FALSE;

   ptr = map + y * stride;
   if (fb->alpha_only)
     {
        for (k = 0; k < h; k++)
          {
             memset(ptr + (x * step), cmd->draw.A, step * w);
             ptr += stride;
          }
     }
   else
     {
        for (k = 0; k < h; k++)
          {
             uint32_t *dst = ((uint32_t *) (ptr + (y + k) * stride)) + x;
             draw_memset32(dst, color, w);
          }
     }

   ector_buffer_unmap(fb->buffer, map, len);
   return EINA_TRUE;
}

/**
 * @brief Gets the appropriate CPU function for the fill filter operation.
 *
 * Currently, this always returns the CPU implementation (_fill_cpu).
 * It performs safety checks on the command and its output buffer.
 *
 * @param cmd The filter command.
 * @return A pointer to the function _fill_cpu if the command is valid,
 *         otherwise NULL.
 */
Software_Filter_Func
eng_filter_fill_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   return _fill_cpu;
}
