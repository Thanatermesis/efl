#include "evas_engine_filter.h"

#include <math.h>
#include <time.h>

// FIXME: Add proper stride support

/**
 * @brief Calculates the radii for multiple box blur passes to approximate a larger radius.
 *
 * This function splits a given blur radius 'r' into up to three smaller radii
 * for consecutive box blur passes. This technique approximates a Gaussian blur
 * more closely than a single large box blur, especially for larger radii.
 * The resulting radii are stored in the `radii` array, terminated by a 0.
 *
 * @param[out] radii An array to store the calculated radii for each pass.
 *                   Must be large enough to hold up to 4 integers (3 radii + terminator).
 *                   Example for r=10: radii = {4, 3, 3, 0}
 * @param[in] r The target blur radius.
 * @return The number of blur passes (radii) calculated (1, 2, or 3).
 */
static int
_box_blur_auto_radius(int *radii, int r)
{
   if (r <= 2)
     {
        radii[0] = r;
        radii[1] = 0;
        WRN("Radius is too small for auto box blur: %d", r);
        return 1;
     }
   else if (r <= 6)
     {
        radii[0] = r / 2;
        radii[1] = r - radii[0] - 1;
        radii[2] = 0;
        XDBG("Using auto radius for %d: %d %d", r, radii[0], radii[1]);
        return 2;
     }
   else
     {
        radii[0] = (r + 3) / 3;
        radii[1] = (r + 2) / 3;
        radii[2] = r - radii[0] - radii[1];
        radii[3] = 0;
        XDBG("Using auto radius for %d: %d %d %d", r, radii[0], radii[1], radii[2]);
        return 3;
     }
}

#include "./blur/blur_box_rgba_.c"
#ifdef BUILD_MMX
#include "./blur/blur_box_rgba_i386.c"
#endif
#ifdef BUILD_SSE3
#include "./blur/blur_box_rgba_sse3.c"
#endif
#ifdef BUILD_NEON
#include "./blur/blur_box_rgba_neon.c"
#endif

/**
 * @brief Dispatches horizontal box blur for RGBA data to the best available implementation.
 *
 * Selects the appropriate horizontal box blur function (SSE3, MMX, NEON, or C)
 * based on runtime CPU feature detection.
 *
 * @param src Source buffer pointer (RGBA).
 * @param src_stride Source buffer stride in pixels.
 * @param dst Destination buffer pointer (RGBA).
 * @param dst_stride Destination buffer stride in pixels.
 * @param radii Array of radii for multiple passes, terminated by 0.
 *              Example: {5, 5, 4, 0} for radius 14 split into 3 passes.
 * @param region The rectangular region within the buffers to process.
 */
static void
_box_blur_horiz_rgba(const uint32_t *src, int src_stride,
                     uint32_t *dst, int dst_stride,
                     int* radii, Eina_Rectangle region)
{
   DEBUG_TIME_BEGIN();

#ifdef BUILD_SSE3
   if (eina_cpu_features_get() & EINA_CPU_SSE3)
     {
        _box_blur_rgba_horiz_step_sse3(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_MMX
   if (eina_cpu_features_get() & EINA_CPU_MMX)
     {
        _box_blur_rgba_horiz_step_mmx(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_NEON
   if (eina_cpu_features_get() & EINA_CPU_NEON)
     {
        _box_blur_rgba_horiz_step_neon(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
   _box_blur_rgba_horiz_step(src, src_stride, dst, dst_stride, radii, region);

#if defined(BUILD_SSE3) || defined(BUILD_MMX) || defined(BUILD_NEON)
end:
#endif
   DEBUG_TIME_END();
}

/**
 * @brief Dispatches vertical box blur for RGBA data to the best available implementation.
 *
 * Selects the appropriate vertical box blur function (SSE3, MMX, NEON, or C)
 * based on runtime CPU feature detection.
 *
 * @param src Source buffer pointer (RGBA).
 * @param src_stride Source buffer stride in pixels.
 * @param dst Destination buffer pointer (RGBA).
 * @param dst_stride Destination buffer stride in pixels.
 * @param radii Array of radii for multiple passes, terminated by 0.
 *              Example: {5, 5, 4, 0} for radius 14 split into 3 passes.
 * @param region The rectangular region within the buffers to process.
 */
static void
_box_blur_vert_rgba(const uint32_t *src, int src_stride,
                    uint32_t *dst, int dst_stride,
                    int* radii, Eina_Rectangle region)
{
   DEBUG_TIME_BEGIN();

#ifdef BUILD_SSE3
   if (eina_cpu_features_get() & EINA_CPU_SSE3)
     {
        _box_blur_rgba_vert_step_sse3(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_MMX
   if (eina_cpu_features_get() & EINA_CPU_MMX)
     {
        _box_blur_rgba_vert_step_mmx(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_NEON
   if (eina_cpu_features_get() & EINA_CPU_NEON)
     {
        _box_blur_rgba_vert_step_neon(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
   _box_blur_rgba_vert_step(src, src_stride, dst, dst_stride, radii, region);

#if defined(BUILD_SSE3) || defined(BUILD_MMX) || defined(BUILD_NEON)
end:
#endif
   DEBUG_TIME_END();
}

#include "./blur/blur_box_alpha_.c"
#ifdef BUILD_MMX
#include "./blur/blur_box_alpha_i386.c"
#endif
#ifdef BUILD_SSE3
#include "./blur/blur_box_alpha_sse3.c"
#endif
#ifdef BUILD_NEON
#include "./blur/blur_box_alpha_neon.c"
#endif

/**
 * @brief Dispatches horizontal box blur for Alpha data to the best available implementation.
 *
 * Selects the appropriate horizontal box blur function (SSE3, MMX, NEON, or C)
 * based on runtime CPU feature detection.
 *
 * @param src Source buffer pointer (Alpha).
 * @param src_stride Source buffer stride in bytes.
 * @param dst Destination buffer pointer (Alpha).
 * @param dst_stride Destination buffer stride in bytes.
 * @param radii Array of radii for multiple passes, terminated by 0.
 *              Example: {5, 5, 4, 0} for radius 14 split into 3 passes.
 * @param region The rectangular region within the buffers to process.
 */
static void
_box_blur_horiz_alpha(const uint8_t *src, int src_stride,
                      uint8_t *dst, int dst_stride,
                      int* radii, Eina_Rectangle region)
{
   DEBUG_TIME_BEGIN();

#ifdef BUILD_SSE3
   if (eina_cpu_features_get() & EINA_CPU_SSE3)
     {
        _box_blur_alpha_horiz_step_sse3(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_MMX
   if (eina_cpu_features_get() & EINA_CPU_MMX)
     {
        _box_blur_alpha_horiz_step_mmx(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_NEON
   if (eina_cpu_features_get() & EINA_CPU_NEON)
     {
        _box_blur_alpha_horiz_step_neon(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
   _box_blur_alpha_horiz_step(src, src_stride, dst, dst_stride, radii, region);

#if defined(BUILD_SSE3) || defined(BUILD_MMX) || defined(BUILD_NEON)
end:
#endif
   DEBUG_TIME_END();
}

/**
 * @brief Dispatches vertical box blur for Alpha data to the best available implementation.
 *
 * Selects the appropriate vertical box blur function (SSE3, MMX, NEON, or C)
 * based on runtime CPU feature detection.
 *
 * @param src Source buffer pointer (Alpha).
 * @param src_stride Source buffer stride in bytes.
 * @param dst Destination buffer pointer (Alpha).
 * @param dst_stride Destination buffer stride in bytes.
 * @param radii Array of radii for multiple passes, terminated by 0.
 *              Example: {5, 5, 4, 0} for radius 14 split into 3 passes.
 * @param region The rectangular region within the buffers to process.
 */
static void
_box_blur_vert_alpha(const uint8_t *src, int src_stride,
                     uint8_t *dst, int dst_stride,
                     int* radii, Eina_Rectangle region)
{
   DEBUG_TIME_BEGIN();

#ifdef BUILD_SSE3
   if (eina_cpu_features_get() & EINA_CPU_SSE3)
     {
        _box_blur_alpha_vert_step_sse3(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_MMX
   if (eina_cpu_features_get() & EINA_CPU_MMX)
     {
        _box_blur_alpha_vert_step_mmx(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
#ifdef BUILD_NEON
   if (eina_cpu_features_get() & EINA_CPU_NEON)
     {
        _box_blur_alpha_vert_step_neon(src, src_stride, dst, dst_stride, radii, region);
        goto end;
     }
#endif
   _box_blur_alpha_vert_step(src, src_stride, dst, dst_stride, radii, region);

#if defined(BUILD_SSE3) || defined(BUILD_MMX) || defined(BUILD_NEON)
end:
#endif
   DEBUG_TIME_END();
}

/**
 * @brief Creates and clamps a rectangle to given boundaries.
 *
 * Ensures the resulting rectangle coordinates and dimensions are non-negative
 * and fit within the maximum width (maxw) and height (maxh).
 *
 * @param x The initial X coordinate.
 * @param y The initial Y coordinate.
 * @param w The initial width.
 * @param h The initial height.
 * @param maxw The maximum allowed width (exclusive bound for x + w).
 * @param maxh The maximum allowed height (exclusive bound for y + h).
 * @return The clamped Eina_Rectangle.
 */
static inline Eina_Rectangle
_rect(int x, int y, int w, int h, int maxw, int maxh)
{
   Eina_Rectangle rect;

   if (x < 0)
     {
        w -= (-x);
        x = 0;
     }
   if (y < 0)
     {
        h -= (-y);
        y = 0;
     }
   if ((x + w) > maxw) w = maxw - x;
   if ((y + h) > maxh) h = maxh - y;
   if (w < 0) w = 0;
   if (h < 0) h = 0;

   rect.x = x;
   rect.y = y;
   rect.w = w;
   rect.h = h;
   return rect;
}
/** @brief Macro to simplify calling _rect within the context of _box_blur_apply. */
#define RECT(_x, _y, _w, _h) _rect(_x, _y, _w, _h, w, h)

/**
 * @brief Applies a box blur (horizontal or vertical) to an image buffer.
 *
 * This function handles the core logic for applying a box blur. It maps the
 * input and output buffers, determines the regions to process (avoiding obscured
 * areas if necessary), calculates the radii for multiple passes if auto_count is set,
 * and calls the appropriate low-level blur function (_box_blur_horiz/vert_rgba/alpha)
 * for each region.
 *
 * @param cmd The filter command containing blur parameters and buffer information.
 * @param vert EINA_TRUE for vertical blur, EINA_FALSE for horizontal blur.
 * @param rgba EINA_TRUE for RGBA data, EINA_FALSE for alpha-only data.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., buffer mapping failed).
 */
static Eina_Bool
_box_blur_apply(Evas_Filter_Command *cmd, Eina_Bool vert, Eina_Bool rgba)
{
   unsigned int src_len, src_stride, dst_len, dst_stride;
   Eina_Bool ret = EINA_FALSE;
   Eina_Rectangle o, region[4];
   int radii[7] = {0};
   int radius, regions, w, h;
   void *src, *dst;

   radius = abs(vert ? (int) cmd->blur.dy : (int) cmd->blur.dx);
   src = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, rgba ? E_ARGB : E_ALPHA, &src_stride);
   dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, rgba ? E_ARGB : E_ALPHA, &dst_stride);
   if (!src || !dst) goto unmap;

   if (cmd->blur.auto_count)
     _box_blur_auto_radius(radii, radius);
   else for (int k = 0; k < cmd->blur.count; k++)
     radii[k] = radius;

   w = cmd->input->w;
   h = cmd->input->h;
   o = cmd->ctx->obscured.effective;
   if (!o.w || !o.h)
     {
        region[0] = RECT(0, 0, w, h);
        regions = 1;
     }
   else if (!vert)
     {
        // top (full), left, right, bottom (full)
        region[0] = RECT(0, 0, w, o.y);
        region[1] = RECT(0, o.y, o.x, o.h);
        region[2] = RECT(o.x + o.w, o.y, w - o.x - o.w, o.h);
        region[3] = RECT(0, o.y + o.h, w, h - o.y - o.h);
        regions = 4;
     }
   else
     {
        // left (full), top, bottom, right (full)
        region[0] = RECT(0, 0, o.x, h);
        region[1] = RECT(o.x, 0, o.w, o.y);
        region[2] = RECT(o.x, o.y + o.h, o.w, h - o.y - o.h);
        region[3] = RECT(o.x + o.w, 0, w - o.x - o.w, h);
        regions = 4;
     }

   XDBG("Box blur on image %dx%d obscured by %d,%d %dx%d", w, h, o.x, o.y, o.w, o.h);
   for (int k = 0; k < regions; k++)
     {
        XDBG("Box blur in region %d,%d %dx%d", region[k].x, region[k].y, region[k].w, region[k].h);
        if (rgba)
          {
             if (!vert)
               _box_blur_horiz_rgba(src, src_stride / 4, dst, dst_stride / 4, radii, region[k]);
             else
               _box_blur_vert_rgba(src, src_stride / 4, dst, dst_stride / 4, radii, region[k]);
          }
        else
          {
             if (!vert)
               _box_blur_horiz_alpha(src, src_stride, dst, dst_stride, radii, region[k]);
             else
               _box_blur_vert_alpha(src, src_stride, dst, dst_stride, radii, region[k]);
          }
     }

   ret = EINA_TRUE;

unmap:
   ector_buffer_unmap(cmd->input->buffer, src, src_len);
   ector_buffer_unmap(cmd->output->buffer, dst, dst_len);

   return ret;
}

/** @brief Wrapper for applying horizontal box blur to alpha channel. */
static Eina_Bool
_box_blur_horiz_apply_alpha(Evas_Filter_Command *cmd)
{
   return _box_blur_apply(cmd, 0, 0);
}

/** @brief Wrapper for applying vertical box blur to alpha channel. */
static Eina_Bool
_box_blur_vert_apply_alpha(Evas_Filter_Command *cmd)
{
   return _box_blur_apply(cmd, 1, 0);
}

/** @brief Wrapper for applying horizontal box blur to RGBA data. */
static Eina_Bool
_box_blur_horiz_apply_rgba(Evas_Filter_Command *cmd)
{
   return _box_blur_apply(cmd, 0, 1);
}

/** @brief Wrapper for applying vertical box blur to RGBA data. */
static Eina_Bool
_box_blur_vert_apply_rgba(Evas_Filter_Command *cmd)
{
   return _box_blur_apply(cmd, 1, 1);
}

/* Gaussian blur */

/**
 * @brief Calculates weights for a Gaussian-like blur using a sine approximation.
 *
 * Generates a kernel of weights based on a shifted and scaled sine curve,
 * approximating a Gaussian distribution. The weights are normalized so that
 * their sum is a power of 2, allowing for efficient division using bit shifts.
 *
 * @param[out] weights An array to store the calculated weights. Must be large
 *                     enough to hold `2 * radius + 1` integers.
 *                     The weights represent the contribution of neighboring pixels,
 *                     centered around the middle element (index `radius`).
 *                     Example (radius=1): weights = { W(-1), W(0), W(1) }
 * @param[out] pow2_divider Pointer to store the exponent for the power-of-2 divider (optional, can be NULL).
 *                          The sum of weights equals `1 << (*pow2_divider)`.
 * @param[in] radius The radius of the blur kernel. The total kernel size is `2 * radius + 1`.
 */
static void
_sin_blur_weights_get(int *weights, int *pow2_divider, int radius)
{
   const int diameter = 2 * radius + 1;
   double x, divider, sum = 0.0;
   double dweights[diameter];
   int k, nextpow2, isum = 0;
   const int FAKE_PI = 3.0;

   /* Base curve:
    * f(x) = sin(x+pi/2)/2+1/2
    */

   for (k = 0; k < diameter; k++)
     {
        x = ((double) k / (double) (diameter - 1)) * FAKE_PI * 2.0 - FAKE_PI;
        dweights[k] = ((sin(x + M_PI_2) + 1.0) / 2.0) * 1024.0;
        sum += dweights[k];
     }

   // Now we need to normalize to have a 2^N divider.
   nextpow2 = log2(2 * sum);
   divider = (double) (1 << nextpow2);

   for (k = 0; k < diameter; k++)
     {
        weights[k] = round(dweights[k] * divider / sum);
        isum += weights[k];
     }

   // Final correction. The difference SHOULD be small...
   weights[radius] += (int) divider - isum;

   if (pow2_divider)
     *pow2_divider = nextpow2;
}

/* Include implementations for Gaussian blur steps (Alpha) */
#define FUNCTION_NAME _gaussian_blur_horiz_alpha_step
#define STEP 1 /* Process pixel by pixel horizontally */
#include "./blur/blur_gaussian_alpha_.c"

#define FUNCTION_NAME _gaussian_blur_vert_alpha_step
#define STEP loops /* Process pixel by pixel vertically (step is image width) */
#include "./blur/blur_gaussian_alpha_.c"

/* Include implementations for Gaussian blur steps (RGBA) */
#define FUNCTION_NAME _gaussian_blur_horiz_rgba_step
#define STEP 1 /* Process pixel by pixel horizontally */
#include "./blur/blur_gaussian_rgba_.c"

#define FUNCTION_NAME _gaussian_blur_vert_rgba_step
#define STEP loops /* Process pixel by pixel vertically (step is image width) */
#include "./blur/blur_gaussian_rgba_.c"

/**
 * @brief Applies a Gaussian-like blur (horizontal or vertical) to an image buffer.
 *
 * This function handles the core logic for applying the sine-approximated
 * Gaussian blur. It maps the input and output buffers, calculates the blur
 * weights using _sin_blur_weights_get(), and calls the appropriate low-level
 * blur step function (_gaussian_blur_horiz/vert_rgba/alpha_step).
 * Unlike box blur, this currently processes the entire buffer without
 * considering obscured regions.
 *
 * @param cmd The filter command containing blur parameters and buffer information.
 * @param vert EINA_TRUE for vertical blur, EINA_FALSE for horizontal blur.
 * @param rgba EINA_TRUE for RGBA data, EINA_FALSE for alpha-only data.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., buffer mapping failed).
 */
static Eina_Bool
_gaussian_blur_apply(Evas_Filter_Command *cmd, Eina_Bool vert, Eina_Bool rgba)
{
   unsigned int src_len, src_stride, dst_len, dst_stride, radius;
   Eina_Bool ret = EINA_TRUE;
   int pow2_div = 0, w, h;
   void *src, *dst;
   int *weights;

   radius = abs(vert ? (int) cmd->blur.dy : (int) cmd->blur.dx);
   src = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, rgba ? E_ARGB : E_ALPHA, &src_stride);
   dst = _buffer_map_all(cmd->output->buffer, &dst_len, E_WRITE, rgba ? E_ARGB : E_ALPHA, &dst_stride);
   w = cmd->input->w;
   h = cmd->input->h;

   weights = alloca((2 * radius + 1) * sizeof(int));
   _sin_blur_weights_get(weights, &pow2_div, radius);

   if (src && dst)
     {
        DEBUG_TIME_BEGIN();
        if (rgba)
          {
             if (!vert)
               _gaussian_blur_horiz_rgba_step(src, dst, radius, w, h, w, weights, pow2_div);
             else
               _gaussian_blur_vert_rgba_step(src, dst, radius, h, w, 1, weights, pow2_div);
          }
        else
          {
             if (!vert)
               _gaussian_blur_horiz_alpha_step(src, dst, radius, w, h, w, weights, pow2_div);
             else
               _gaussian_blur_vert_alpha_step(src, dst, radius, h, w, 1, weights, pow2_div);
          }
        DEBUG_TIME_END();
     }
   else ret = EINA_FALSE;

   ector_buffer_unmap(cmd->input->buffer, src, src_len);
   ector_buffer_unmap(cmd->output->buffer, dst, dst_len);

   return ret;
}

/** @brief Wrapper for applying horizontal Gaussian blur to alpha channel. */
static Eina_Bool
_gaussian_blur_horiz_apply_alpha(Evas_Filter_Command *cmd)
{
   return _gaussian_blur_apply(cmd, 0, 0);
}

/** @brief Wrapper for applying vertical Gaussian blur to alpha channel. */
static Eina_Bool
_gaussian_blur_vert_apply_alpha(Evas_Filter_Command *cmd)
{
   return _gaussian_blur_apply(cmd, 1, 0);
}

/** @brief Wrapper for applying horizontal Gaussian blur to RGBA data. */
static Eina_Bool
_gaussian_blur_horiz_apply_rgba(Evas_Filter_Command *cmd)
{
   return _gaussian_blur_apply(cmd, 0, 1);
}

/** @brief Wrapper for applying vertical Gaussian blur to RGBA data. */
static Eina_Bool
_gaussian_blur_vert_apply_rgba(Evas_Filter_Command *cmd)
{
   return _gaussian_blur_apply(cmd, 1, 1);
}

/* Main entry point */

/**
 * @brief Gets the appropriate software filter function for a blur command.
 *
 * This function acts as the main dispatcher for blur operations. Based on the
 * blur type (Box, Gaussian), direction (dx/dy), and color format (RGBA/Alpha)
 * specified in the Evas_Filter_Command, it returns a pointer to the
 * corresponding specialized blur function (e.g., _box_blur_horiz_apply_rgba).
 *
 * @param cmd The filter command describing the desired blur operation.
 * @return A function pointer (Software_Filter_Func) to the appropriate blur
 *         implementation, or NULL if the command is invalid or unsupported.
 */
Software_Filter_Func
eng_filter_blur_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(cmd->mode == EVAS_FILTER_MODE_BLUR, NULL);

   switch (cmd->blur.type)
     {
      case EVAS_FILTER_BLUR_BOX:
        if (!cmd->output->alpha_only)
          {
             if (EINA_DBL_NONZERO(cmd->blur.dx))
               return _box_blur_horiz_apply_rgba;
             else if (EINA_DBL_NONZERO(cmd->blur.dy))
               return _box_blur_vert_apply_rgba;
          }
        else
          {
             if (EINA_DBL_NONZERO(cmd->blur.dx))
               return _box_blur_horiz_apply_alpha;
             else if (EINA_DBL_NONZERO(cmd->blur.dy))
               return _box_blur_vert_apply_alpha;
          }
        break;
      case EVAS_FILTER_BLUR_GAUSSIAN:
        if (!cmd->output->alpha_only)
          {
             if (EINA_DBL_NONZERO(cmd->blur.dx))
               return _gaussian_blur_horiz_apply_rgba;
             else if (EINA_DBL_NONZERO(cmd->blur.dy))
               return _gaussian_blur_vert_apply_rgba;
          }
        else
          {
             if (EINA_DBL_NONZERO(cmd->blur.dx))
               return _gaussian_blur_horiz_apply_alpha;
             else if (EINA_DBL_NONZERO(cmd->blur.dy))
               return _gaussian_blur_vert_apply_alpha;
          }
        break;
      default:
        ERR("Unsupported blur type %d", cmd->blur.type);
        return NULL;
     }

   return NULL;
}
