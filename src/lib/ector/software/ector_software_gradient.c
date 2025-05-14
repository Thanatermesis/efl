/**
 * @file
 * @brief Software gradient rendering implementation for Ector.
 *
 * This file contains the core logic for rendering linear and radial gradients
 * using software-based methods. It includes generic C implementations as well
 * as SSE3 optimized versions where available. It manages color table generation
 * for gradients and provides functions to fetch gradient pixels for spans.
 */

#include "ector_software_gradient.h"

#ifdef BUILD_SSE3
/**
 * @brief SSE3 optimized helper function for radial gradient calculation.
 * @param buffer The destination buffer for pixel data.
 * @param length The number of pixels to calculate.
 * @param g_data Pointer to the gradient data.
 * @param det Initial determinant value.
 * @param delta_det Change in determinant per pixel.
 * @param delta_delta_det Change in delta_det per pixel.
 * @param b Initial 'b' term in the quadratic equation.
 * @param delta_b Change in 'b' term per pixel.
 */
void _radial_helper_sse3(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data, float det, float delta_det, float delta_delta_det, float b, float delta_b);
/**
 * @brief SSE3 optimized helper function for linear gradient calculation.
 * @param buffer The destination buffer for pixel data.
 * @param length The number of pixels to calculate.
 * @param g_data Pointer to the gradient data.
 * @param t_fixed Initial fixed-point position value.
 * @param inc_fixed Fixed-point increment value per pixel.
 */
void _linear_helper_sse3(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data, int t, int inc);
#endif

#define GRADIENT_STOPTABLE_SIZE 1024 /**< Size of the pre-calculated gradient color table. */
#define FIXPT_BITS 8 /**< Number of bits for fixed-point arithmetic precision. */
#define FIXPT_SIZE (1<<FIXPT_BITS) /**< Scaling factor for fixed-point arithmetic (2^FIXPT_BITS). */

/**
 * @brief Function pointer type for radial gradient helper functions.
 * These functions calculate and fill a buffer with radial gradient pixels.
 */
typedef void (*Ector_Radial_Helper_Func)(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data,
                                          float det, float delta_det, float delta_delta_det, float b, float delta_b);
/**
 * @brief Function pointer type for linear gradient helper functions.
 * These functions calculate and fill a buffer with linear gradient pixels using fixed-point arithmetic.
 */
typedef void (*Ector_Linear_Helper_Func)(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data,
                                          int t_fixed, int inc_fixed);

static Ector_Radial_Helper_Func _ector_radial_helper; /**< Pointer to the current radial gradient helper function (generic or SSE3). */
static Ector_Linear_Helper_Func _ector_linear_helper; /**< Pointer to the current linear gradient helper function (generic or SSE3). */

/**
 * @brief Updates the gradient color table.
 * This function is typically scheduled to run in a software rendering thread.
 * It generates a table of colors based on the gradient stops.
 *
 * @param data Pointer to Ector_Renderer_Software_Gradient_Data.
 * @param t Pointer to Ector_Software_Thread (unused).
 */
static void
_update_color_table(void *data, Ector_Software_Thread *t EINA_UNUSED)
{
   Ector_Renderer_Software_Gradient_Data *gdata = data;
   gdata->alpha = efl_draw_generate_gradient_color_table(gdata->gd->colors, gdata->gd->colors_count,
                                                         gdata->color_table, GRADIENT_STOPTABLE_SIZE);
}

/**
 * @brief Marks the color table generation as complete.
 * This function is called after _update_color_table finishes.
 *
 * @param data Pointer to Ector_Renderer_Software_Gradient_Data.
 */
static void
_done_color_table(void *data)
{
   Ector_Renderer_Software_Gradient_Data *gdata = data;
   gdata->ctable_status = CTABLE_READY_DONE;
}

/**
 * @brief Ensures the gradient color table is up-to-date.
 * If the color table is not ready or is being processed, this function
 * will either schedule its generation or wait for its completion.
 *
 * @param gdata Pointer to the gradient data.
 */
void
ector_software_gradient_color_update(Ector_Renderer_Software_Gradient_Data *gdata)
{
   if (gdata->ctable_status == CTABLE_READY_DONE) return;

   //OPTIMIZE: This color can be updated only when gradient properties are changed.

   //Alloc only one time.
   if (!gdata->color_table)
     gdata->color_table = malloc(GRADIENT_STOPTABLE_SIZE * 4);

   if (gdata->ctable_status == CTABLE_NOT_READY)
     {
        gdata->ctable_status = CTABLE_PROCESSING;
        ector_software_schedule(_update_color_table, _done_color_table, gdata);
     }
   else if (gdata->ctable_status == CTABLE_PROCESSING)
        ector_software_wait(_update_color_table, _done_color_table, gdata);
}

/**
 * @brief Frees the memory allocated for the gradient color table.
 *
 * @param gdata Pointer to the gradient data.
 */
void
destroy_color_table(Ector_Renderer_Software_Gradient_Data *gdata)
{
   if (gdata->color_table)
     {
        free(gdata->color_table);
        gdata->color_table = NULL;
     }
}

/**
 * @brief Generic C implementation for linear gradient calculation.
 * Fills a buffer with pixels for a linear gradient using fixed-point arithmetic.
 *
 * @param buffer The destination buffer for pixel data.
 *               Example: `uint32_t scanline[width];`
 * @param length The number of pixels to calculate (width of the span).
 * @param g_data Pointer to the gradient data, containing color table and spread mode.
 * @param t_fixed Initial fixed-point position value along the gradient axis.
 *                This value is scaled by `FIXPT_SIZE`.
 * @param inc_fixed Fixed-point increment value per pixel along the gradient axis.
 *                  This value is also scaled by `FIXPT_SIZE`.
 */
static void
_linear_helper_generic(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data,
                       int t_fixed, int inc_fixed)
{
   int i;

   for (i = 0 ; i < length ; i++)
     {
        *buffer++ = _gradient_pixel_fixed(g_data, t_fixed);
        t_fixed += inc_fixed;
     }
}

/**
 * @brief Fetches a span of pixels for a linear gradient.
 * Calculates the gradient parameters for the given span and then fills the
 * buffer with the corresponding gradient colors. It may use fixed-point
 * arithmetic for optimization if the values are within range, otherwise
 * it falls back to floating-point math.
 *
 * @param buffer The destination buffer for pixel data.
 *               Example: `uint32_t scanline_segment[segment_length];`
 * @param data Pointer to Span_Data containing transformation matrix and gradient data.
 * @param y The y-coordinate of the start of the span.
 * @param x The x-coordinate of the start of the span.
 * @param length The number of pixels in the span.
 */
void
fetch_linear_gradient(uint32_t *buffer, Span_Data *data, int y, int x, int length)
{
   Ector_Renderer_Software_Gradient_Data *g_data = data->gradient;
   float t, inc, rx=0, ry=0;
   uint32_t *end;
   int t_fixed, inc_fixed;

   if (EINA_DBL_EQ(g_data->linear.l, 0.0))
     {
        t = inc = 0;
     }
   else
     {
        rx = data->inv.xy * (y + (float)0.5) + data->inv.xz + data->inv.xx * (x + (float)0.5);
        ry = data->inv.yy * (y + (float)0.5) + data->inv.yz + data->inv.yx * (x + (float)0.5);
        t = g_data->linear.dx*rx + g_data->linear.dy*ry + g_data->linear.off;
        inc = g_data->linear.dx * data->inv.xx + g_data->linear.dx * data->inv.yx;

        t *= (GRADIENT_STOPTABLE_SIZE - 1);
        inc *= (GRADIENT_STOPTABLE_SIZE - 1);
     }

    end = buffer + length;
    if (inc > (float)(-1e-5) && inc < (float)(1e-5))
      {
         draw_memset32(buffer, _gradient_pixel_fixed(g_data, (int)(t * FIXPT_SIZE)), length);
      }
    else
      {
         const int vmax = INT_MAX >> (FIXPT_BITS + 1);
         const int vmin = -vmax;
         float v = t + (inc *length);

         if ((v < (float)vmax) && (v > (float)(vmin)))
           {
              // we can use fixed point math
              t_fixed = (int)(t * FIXPT_SIZE);
              inc_fixed = (int)(inc * FIXPT_SIZE);
              _ector_linear_helper(buffer, length, g_data, t_fixed, inc_fixed);
           }
         else
           {
              // we have to fall back to float math
              while (buffer < end)
                {
                   *buffer++ = _gradient_pixel(g_data, t/GRADIENT_STOPTABLE_SIZE);
                   t += inc;
                }
           }
      }
}

static void
_radial_helper_generic(uint32_t *buffer, int length, Ector_Renderer_Software_Gradient_Data *g_data, float det,
                       float delta_det, float delta_delta_det, float b, float delta_b)
{
   int i;

   // This loop calculates the color for each pixel in the span.
   // 'det' is related to the squared distance from the gradient center, adjusted by 'a'.
   // 'b' is related to the linear term in the radial gradient equation.
   // The gradient position is effectively sqrt(det) - b.
   for (i = 0 ; i < length ; i++)
     {
        *buffer++ = _gradient_pixel(g_data, sqrt(det) - b);
        det += delta_det; // Update det for the next pixel
        delta_det += delta_delta_det; // Update the rate of change of det
        b += delta_b; // Update b for the next pixel
     }
}

/**
 * @brief Fetches a span of pixels for a radial gradient.
 * Calculates the complex parameters for a radial gradient across a span
 * and then fills the buffer with the corresponding gradient colors.
 * This involves solving a quadratic equation for each pixel to determine
 * its distance from the gradient's focal point, considering the gradient's
 * transformation.
 *
 * @param buffer The destination buffer for pixel data.
 *               Example: `uint32_t scanline_segment[segment_length];`
 * @param data Pointer to Span_Data containing transformation matrix and gradient data.
 * @param y The y-coordinate of the start of the span.
 * @param x The x-coordinate of the start of the span.
 * @param length The number of pixels in the span.
 */
void
fetch_radial_gradient(uint32_t *buffer, Span_Data *data, int y, int x, int length)
{
   Ector_Renderer_Software_Gradient_Data *g_data = data->gradient;
   float rx, ry, inv_a, delta_rx, delta_ry, b, delta_b, b_delta_b, delta_b_delta_b,
         bb, delta_bb, rxrxryry, delta_rxrxryry, rx_plus_ry, delta_rx_plus_ry, det,
         delta_det, delta_delta_det;

   // avoid division by zero
   if (fabsf(g_data->radial.a) <= 0.00001f)
     {
        draw_memset32(buffer, 0, length);
        return;
     }

   rx = data->inv.xy * (y + (float)0.5) + data->inv.xz + data->inv.xx * (x + (float)0.5);
   ry = data->inv.yy * (y + (float)0.5) + data->inv.yz + data->inv.yx * (x + (float)0.5);

   rx -= g_data->radial.fx;
   ry -= g_data->radial.fy;

   inv_a = 1 / (float)(2 * g_data->radial.a);

   delta_rx = data->inv.xx;
   delta_ry = data->inv.yx;

   b = 2*(g_data->radial.dr*g_data->radial.fradius + rx * g_data->radial.dx + ry * g_data->radial.dy);
   delta_b = 2*(delta_rx * g_data->radial.dx + delta_ry * g_data->radial.dy);
   b_delta_b = 2 * b * delta_b;
   delta_b_delta_b = 2 * delta_b * delta_b;

   bb = b * b;
   delta_bb = delta_b * delta_b;
   b *= inv_a;
   delta_b *= inv_a;

   rxrxryry = rx * rx + ry * ry;
   delta_rxrxryry = delta_rx * delta_rx + delta_ry * delta_ry;
   rx_plus_ry = 2*(rx * delta_rx + ry * delta_ry);
   delta_rx_plus_ry = 2 * delta_rxrxryry;

   inv_a *= inv_a;

   det = (bb - 4 * g_data->radial.a * (g_data->radial.sqrfr - rxrxryry)) * inv_a;
   delta_det = (b_delta_b + delta_bb + 4 * g_data->radial.a * (rx_plus_ry + delta_rxrxryry)) * inv_a;
   delta_delta_det = (delta_b_delta_b + 4 * g_data->radial.a * delta_rx_plus_ry) * inv_a;

   _ector_radial_helper(buffer, length, g_data, det, delta_det, delta_delta_det, b, delta_b);
}

/**
 * @brief Initializes the software gradient module.
 * This function sets up the appropriate helper functions for linear and radial
 * gradients, choosing between generic C versions and SSE3 optimized versions
 * if available and supported by the CPU. It ensures this initialization
 * happens only once.
 *
 * @return The initialization count. Returns 1 on the first call, and increments
 *         on subsequent calls (though typically only called once effectively).
 */
int
ector_software_gradient_init(void)
{
   static int i = 0;
   if (!(i++)) // Ensure initialization runs only once
     {
        _ector_radial_helper = _radial_helper_generic;
        _ector_linear_helper = _linear_helper_generic;
#ifdef BUILD_SSE3
        // Check for SSE3 CPU support and use optimized versions if available
        if (eina_cpu_features_get() & EINA_CPU_SSE3)
          {
             _ector_radial_helper = _radial_helper_sse3;
             _ector_linear_helper = _linear_helper_sse3;
          }
#endif
     }
   return i;
}
