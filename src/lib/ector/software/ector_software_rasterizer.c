#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"

#include "draw.h"

/**
 * @internal
 * @brief Blends a solid ARGB color onto the destination buffer.
 *
 * This function is called by the FreeType rasterizer for each set of spans
 * generated for a solid color fill. It applies the color to the target buffer
 * using the specified composition operator.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures. Each span describes a
 *              horizontal line segment to be filled.
 *              Example: `spans[0] = { .x = 10, .y = 5, .len = 20, .coverage = 255 }`
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters like color, composition operator, and target buffer.
 */
static void
_blend_argb(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   uint32_t color, *buffer, *target;
   const int pix_stride = sd->raster_buffer->stride / 4;

   // multiply the color with mul_col if any
   color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);

   // move to the offset location
   buffer = sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);

    while (count--)
      {
          target = buffer + ((pix_stride * spans->y) + spans->x);
          comp_func(target, spans->len, color, spans->coverage);
          ++spans;
      }
}

/**
 * @internal
 * @brief Composites a solid color with a matte (alpha mask) onto the destination buffer.
 *
 * This function first renders the solid color into a temporary buffer, then
 * composites this temporary buffer with the destination buffer using the alpha
 * values from a matte (composition) buffer. The matte's alpha determines how
 * much of the rendered color is blended.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the composition buffer (matte).
 */
static void
_comp_matte_alpha(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   const int pix_stride = sd->raster_buffer->stride / 4;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   // multiply the color with mul_col if any
   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);

   // move to the offset location
   uint32_t *buffer =
         sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);
   uint32_t *mbuffer = comp->pixels.u32;

   //Temp buffer for intermediate processing
   int tsize = sd->raster_buffer->generic->w;
   uint32_t *tbuffer = alloca(sizeof(uint32_t) * tsize);

   while (count--)
     {
        uint32_t *target = buffer + ((pix_stride * spans->y) + spans->x);
        uint32_t *mtarget =
              mbuffer + ((comp_stride * spans->y) + spans->x);
        uint32_t *temp = tbuffer;
        memset(temp, 0x00, sizeof(uint32_t) * spans->len);
        comp_func(temp, spans->len, color, spans->coverage);

        //composite
        for (int i = 0; i < spans->len; i++)
          {
             *temp = draw_mul_256(((*mtarget)>>24), *temp);
             int alpha = 255 - ((*temp) >> 24);
             *target = *temp + draw_mul_256(alpha, *target);
             ++temp;
             ++mtarget;
             ++target;
          }
        ++spans;
     }
}

/**
 * @internal
 * @brief Composites a solid color with an inverted matte (alpha mask) onto the destination buffer.
 *
 * Similar to _comp_matte_alpha, but uses the inverse of the matte's alpha values.
 * This means areas with low alpha in the matte will allow more of the rendered
 * color to be blended, and vice-versa.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the composition buffer (matte).
 */
static void
_comp_matte_alpha_inv(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   const int pix_stride = sd->raster_buffer->stride / 4;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   // multiply the color with mul_col if any
   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);

   // move to the offset location
   uint32_t *buffer =
         sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);
   uint32_t *mbuffer = comp->pixels.u32;

   //Temp buffer for intermediate processing
   int tsize = sd->raster_buffer->generic->w;
   uint32_t *tbuffer = alloca(sizeof(uint32_t) * tsize);

   while (count--)
     {
        uint32_t *target = buffer + ((pix_stride * spans->y) + spans->x);
        uint32_t *mtarget =
              mbuffer + ((comp_stride * spans->y) + spans->x);
        uint32_t *temp = tbuffer;
        memset(temp, 0x00, sizeof(uint32_t) * spans->len);
        comp_func(temp, spans->len, color, spans->coverage);

        //composite
        for (int i = 0; i < spans->len; i++)
          {
             if (*mtarget)
                *temp = draw_mul_256((255 - ((*mtarget)>>24)), *temp);
             int alpha = 255 - ((*temp) >> 24);
             *target = *temp + draw_mul_256(alpha, *target);
             ++temp;
             ++mtarget;
             ++target;
          }
        ++spans;
     }
}

/**
 * @internal
 * @brief Adds the rendered solid color (as a mask) to the composition buffer.
 *
 * This function renders the solid color into a temporary buffer and then adds
 * its alpha channel to the alpha channel of the composition buffer (mask).
 * The color components of the composition buffer are also blended.
 * This is typically used for operations like `EFL_GFX_VG_COMPOSITE_METHOD_MASK_ADD`.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the target composition buffer (mask).
 */
static void
_comp_mask_add(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);
   uint32_t *mbuffer = comp->pixels.u32;

   int tsize = sd->raster_buffer->generic->w;
   uint32_t *ttarget = alloca(sizeof(uint32_t) * tsize);

   while (count--)
     {
        uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
        memset(ttarget, 0x00, sizeof(uint32_t) * spans->len);
        comp_func(ttarget, spans->len, color, spans->coverage);
        for (int i = 0; i < spans->len; i++)
          mtarget[i] = draw_mul_256(0xFF - (ttarget[i]>>24), mtarget[i]) + ttarget[i];
        ++spans;
     }
}

/**
 * @internal
 * @brief Subtracts the rendered solid color (as a mask) from the composition buffer.
 *
 * This function renders the solid color into a temporary buffer. The alpha of this
 * temporary buffer is then used to modulate the alpha of the composition buffer (mask),
 * effectively subtracting the mask.
 * This is typically used for operations like `EFL_GFX_VG_COMPOSITE_METHOD_MASK_SUBSTRACT`.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the target composition buffer (mask).
 */
static void
_comp_mask_sub(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);
   uint32_t *mbuffer = comp->pixels.u32;

   int tsize = sd->raster_buffer->generic->w;
   uint32_t *ttarget = alloca(sizeof(uint32_t) * tsize);

   while (count--)
     {
        uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
        memset(ttarget, 0x00, sizeof(uint32_t) * spans->len);
        comp_func(ttarget, spans->len, color, spans->coverage);
        for (int i = 0; i < spans->len; i++)
          mtarget[i] = draw_mul_256(0xFF - (ttarget[i]>>24), mtarget[i]);
        ++spans;
     }
}

/**
 * @internal
 * @brief Intersects the rendered solid color (as a mask) with the composition buffer.
 *
 * This function iterates over the entire composition buffer. For areas covered by
 * the input spans, it renders the solid color into a temporary buffer and then
 * multiplies the alpha of the composition buffer (mask) by the alpha of the
 * temporary buffer. Areas not covered by the spans have their alpha in the
 * composition buffer set to 0.
 * This is typically used for `EFL_GFX_VG_COMPOSITE_METHOD_MASK_INTERSECT`.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures. These spans define the area
 *              where the intersection will occur.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the target composition buffer (mask).
 */
static void
_comp_mask_ins(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);
   uint32_t *mbuffer = comp->pixels.u32;

   int tsize = sd->raster_buffer->generic->w;
   uint32_t *ttarget = alloca(sizeof(uint32_t) * tsize);

   for(unsigned int y = 0; y < comp->generic->h; y++)
     {
        for(unsigned int x = 0; x < comp->generic->w; x++)
          {
             if (x == (unsigned int)spans->x && x + spans->len <= comp->generic->w &&
                 y == (unsigned int)spans->y && count > 0)
               {
                  memset(ttarget, 0x00, sizeof(uint32_t) * spans->len);
                  uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
                  comp_func(ttarget, spans->len, color, spans->coverage);
                  for (int c = 0; c < spans->len; c++)
                    mtarget[c] = draw_mul_256(ttarget[c]>>24, mtarget[c]);
                  x += spans->len - 1;
                  ++spans;
                  --count;
               }
             else
               {
                  mbuffer[x + (comp_stride * y)] = (0x00FFFFFF & mbuffer[x + (comp_stride * y)]);
               }
          }
     }
}

/**
 * @internal
 * @brief Computes the difference between the composition buffer and the rendered solid color mask.
 *
 * This function renders the solid color into a temporary buffer. The resulting mask
 * (from the temporary buffer) and the existing composition buffer (mask) are
 * combined using a difference operation. Specifically, it's `dest = src * (1-dest_alpha) + dest * (1-src_alpha)`.
 * This is typically used for `EFL_GFX_VG_COMPOSITE_METHOD_MASK_DIFFERENCE`.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing rendering
 *                  parameters, including the target composition buffer (mask).
 */
static void
_comp_mask_diff(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = user_data;
   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   if (!comp || !comp->pixels.u32) return;
   const int comp_stride = comp->stride / 4;

   uint32_t color = DRAW_MUL4_SYM(sd->color, sd->mul_col);
   RGBA_Comp_Func_Solid comp_func = efl_draw_func_solid_span_get(sd->op, color);
   uint32_t *mbuffer = comp->pixels.u32;

   int tsize = sd->raster_buffer->generic->w;
   uint32_t *ttarget = alloca(sizeof(uint32_t) * tsize);

   while (count--)
     {
        memset(ttarget, 0x00, sizeof(uint32_t) * spans->len);
        uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
        comp_func(ttarget, spans->len, color, spans->coverage);
        for (int i = 0; i < spans->len; i++)
          mtarget[i] = draw_mul_256(0xFF - (mtarget[i]>>24), ttarget[i]) + draw_mul_256(0xFF - (ttarget[i]>>24), mtarget[i]);
        ++spans;
     }
}

#define BLEND_GRADIENT_BUFFER_SIZE 2048

/**
 * @internal
 * @brief Function pointer type for fetching source pixels for gradients.
 *
 * @param buffer Output buffer where fetched pixel data (ARGB) is stored.
 * @param data Span_Data containing gradient information and transformation.
 * @param y The y-coordinate of the start of the span in destination space.
 * @param x The x-coordinate of the start of the span in destination space.
 * @param length The number of pixels to fetch.
 */
typedef void (*src_fetch) (unsigned int *buffer, Span_Data *data, int y, int x, int length);

/**
 * @internal
 * @brief Blends a gradient onto the destination buffer.
 *
 * This function is called by the FreeType rasterizer for each set of spans
 * generated for a gradient fill. It fetches gradient colors using a `src_fetch`
 * function (either for linear or radial gradients) and then composites them
 * onto the target buffer.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing gradient
 *                  parameters, composition operator, and target buffer.
 */
static void
_blend_gradient(int count, const SW_FT_Span *spans, void *user_data)
{
   RGBA_Comp_Func comp_func;
   Span_Data *sd = (Span_Data *)(user_data);
   src_fetch fetchfunc = NULL;
   unsigned int buffer[BLEND_GRADIENT_BUFFER_SIZE], *target, *destbuffer;
   int length, l;
   const int pix_stride = sd->raster_buffer->stride / 4;

   // FIXME: Get the proper composition function using ,color, ECTOR_OP etc.
   if (sd->type == LinearGradient) fetchfunc = &fetch_linear_gradient;
   if (sd->type == RadialGradient) fetchfunc = &fetch_radial_gradient;

   if (!fetchfunc || !sd->raster_buffer->pixels.u32) return;

   comp_func = efl_draw_func_span_get(sd->op, sd->mul_col, sd->gradient->alpha);

   // move to the offset location
   destbuffer = sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);

   while (count--)
     {
        target = destbuffer + ((pix_stride * spans->y) + spans->x);
        length = spans->len;
        while (length)
          {
             l = MIN(length, BLEND_GRADIENT_BUFFER_SIZE);
             //FIXME: span->x must have adding an offset as much as subtracted length...
             fetchfunc(buffer, sd, spans->y, spans->x, l);
             comp_func(target, buffer, l, sd->mul_col, spans->coverage);
             target += l;
             length -= l;
          }
        ++spans;
     }
}

/**
 * @internal
 * @brief Composites a gradient with a matte (alpha mask) onto the destination buffer.
 *
 * This function fetches gradient colors and then composites them with the
 * destination buffer, using alpha values from a matte (composition) buffer.
 * The matte's alpha determines how much of the gradient is blended.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing gradient
 *                  parameters and the composition buffer (matte).
 */
static void
_blend_gradient_alpha(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = (Span_Data *)(user_data);
   src_fetch fetchfunc = NULL;
   uint32_t *buffer;
   const int pix_stride = sd->raster_buffer->stride / 4;
   uint32_t gbuffer[BLEND_GRADIENT_BUFFER_SIZE];  //gradient buffer

   // FIXME: Get the proper composition function using ,color, ECTOR_OP etc.
   if (sd->type == LinearGradient) fetchfunc = &fetch_linear_gradient;
   if (sd->type == RadialGradient) fetchfunc = &fetch_radial_gradient;

   if (!fetchfunc) return;

   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   uint32_t *mbuffer = comp->pixels.u32;
   const int comp_stride = comp->stride / 4;

   // move to the offset location
   buffer = sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);

   while (count--)
     {
        uint32_t *target = buffer + ((pix_stride * spans->y) + spans->x);
        uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
        int length = spans->len;

        while (length)
          {
             int l = MIN(length, BLEND_GRADIENT_BUFFER_SIZE);
             //FIXME: span->x must have adding an offset as much as subtracted length...
             fetchfunc(gbuffer, sd, spans->y, spans->x, l);
             uint32_t *temp = gbuffer;

             for (int i = 0; i < l; i++)
               {
                  *temp = draw_mul_256(((*mtarget)>>24), *temp);
                  int alpha = 255 - ((*temp) >> 24);
                  *target = *temp + draw_mul_256(alpha, *target);
                  ++temp;
                  ++mtarget;
                  ++target;
               }
             length -= l;
          }
        ++spans;
     }
}

/**
 * @internal
 * @brief Composites a gradient with an inverted matte (alpha mask) onto the destination buffer.
 *
 * Similar to _blend_gradient_alpha, but uses the inverse of the matte's alpha
 * values. Areas with low alpha in the matte will allow more of the gradient
 * to be blended.
 *
 * @param count The number of spans to process.
 * @param spans An array of SW_FT_Span structures.
 * @param user_data A pointer to a Span_Data structure containing gradient
 *                  parameters and the composition buffer (matte).
 */
static void
_blend_gradient_alpha_inv(int count, const SW_FT_Span *spans, void *user_data)
{
   Span_Data *sd = (Span_Data *)(user_data);
   src_fetch fetchfunc = NULL;
   uint32_t *buffer;
   const int pix_stride = sd->raster_buffer->stride / 4;
   uint32_t gbuffer[BLEND_GRADIENT_BUFFER_SIZE];  //gradient buffer

   // FIXME: Get the proper composition function using ,color, ECTOR_OP etc.
   if (sd->type == LinearGradient) fetchfunc = &fetch_linear_gradient;
   if (sd->type == RadialGradient) fetchfunc = &fetch_radial_gradient;

   if (!fetchfunc) return;

   Ector_Software_Buffer_Base_Data *comp = sd->comp;
   uint32_t *mbuffer = comp->pixels.u32;
   const int comp_stride = comp->stride / 4;

   // move to the offset location
   buffer = sd->raster_buffer->pixels.u32 + ((pix_stride * sd->offy) + sd->offx);

   while (count--)
     {
        uint32_t *target = buffer + ((pix_stride * spans->y) + spans->x);
        uint32_t *mtarget = mbuffer + ((comp_stride * spans->y) + spans->x);
        int length = spans->len;

        while (length)
          {
             int l = MIN(length, BLEND_GRADIENT_BUFFER_SIZE);
             //FIXME: span->x must have adding an offset as much as subtracted length...
             fetchfunc(gbuffer, sd, spans->y, spans->x, l);
             uint32_t *temp = gbuffer;

             for (int i = 0; i < l; i++)
               {
                  if (*mtarget)
                    *temp = draw_mul_256((255 - ((*mtarget)>>24)), *temp);
                  int alpha = 255 - ((*temp) >> 24);
                  *target = *temp + draw_mul_256(alpha, *target);
                  ++temp;
                  ++mtarget;
                  ++target;
               }
             length -= l;
          }
        ++spans;
     }
}
/**
 * @internal
 * @brief Intersects a list of spans with a clipping rectangle.
 *
 * Takes an array of spans and clips them against a given rectangle.
 * The input spans are assumed to be sorted by their y-coordinate.
 *
 * @param clip The clipping rectangle.
 *           Example: `clip = { .x = 0, .y = 0, .w = 100, .h = 100 }`
 * @param spans Pointer to the first span in the input array.
 * @param end Pointer to one past the last span in the input array.
 * @param out_spans Pointer to a pointer where the clipped spans will be written.
 *                  The caller must ensure `*out_spans` points to a buffer large
 *                  enough to hold `available` spans.
 *                  On exit, `*out_spans` is updated to point past the last written span.
 *                  Example (after processing):
 *                  `(*out_spans)[0] = { .x = 10, .y = 5, .len = 15, .coverage = 255 }`
 * @param available The maximum number of spans that can be written to `*out_spans`.
 * @return A pointer to the next input span that was not processed. If all spans
 *         were processed, this will be equal to `end`.
 */
static const
SW_FT_Span *_intersect_spans_rect(const Eina_Rectangle *clip,
                                  const SW_FT_Span *spans,
                                  const SW_FT_Span *end,
                                  SW_FT_Span **out_spans,
                                  int available)
{
   SW_FT_Span *out = *out_spans;
   short minx, miny, maxx, maxy;
   minx = clip->x;
   miny = clip->y;
   maxx = minx + clip->w - 1;
   maxy = miny + clip->h - 1;

   while (available && spans < end )
     {
        if (spans->y > maxy)
          {
             spans = end;// update spans so that we can breakout
             break;
          }
        if (spans->y < miny
            || spans->x > maxx
            || spans->x + spans->len <= minx)
          {
             ++spans;
             continue;
          }
        if (spans->x < minx)
          {
             out->len = MIN(spans->len - (minx - spans->x), maxx - minx + 1);
             out->x = minx;
          }
        else
          {
             out->x = spans->x;
             out->len = MIN(spans->len, (maxx - spans->x + 1));
          }
        if (out->len != 0)
          {
             out->y = spans->y;
             out->coverage = spans->coverage;
             ++out;
          }
        ++spans;
        --available;
     }

   *out_spans = out;
   return spans;
}

/**
 * @internal
 * @brief Fast division by 255 for byte values.
 * @param x The integer value to divide (expected to be in a range like 0 to 255*255).
 * @return The result of `x / 255`.
 */
static inline int
_div_255(int x) { return (x + (x>>8) + 0x80) >> 8; }

/**
 * @internal
 * @brief Intersects a list of spans with a clipping region defined by RLE data.
 *
 * Clips an array of input spans against a clipping region represented by another
 * set of spans (RLE data). Both sets of spans are assumed to be sorted by their
 * y-coordinate.
 *
 * @param clip Pointer to the Shape_Rle_Data representing the clipping region.
 *             `clip->spans` contains an array of SW_FT_Span.
 *             Example: `clip->spans[0] = { .x = 5, .y = 2, .len = 10, .coverage = 255 }`
 * @param currentClip Pointer to an integer that tracks the current position
 *                    within the `clip->spans` array. This is updated by the function.
 * @param spans Pointer to the first span in the input array to be clipped.
 * @param end Pointer to one past the last span in the input array.
 * @param out_spans Pointer to a pointer where the clipped spans will be written.
 *                  The caller must ensure `*out_spans` points to a buffer large
 *                  enough to hold `available` spans.
 *                  On exit, `*out_spans` is updated to point past the last written span.
 * @param available The maximum number of spans that can be written to `*out_spans`.
 * @return A pointer to the next input span that was not processed. If all spans
 *         were processed, this will be equal to `end`.
 */
static const
SW_FT_Span *_intersect_spans_region(const Shape_Rle_Data *clip,
                                    int *currentClip,
                                    const SW_FT_Span *spans,
                                    const SW_FT_Span *end,
                                    SW_FT_Span **out_spans,
                                    int available)
{
   SW_FT_Span *out = *out_spans;
   int sx1, sx2, cx1, cx2, x, len;

   const SW_FT_Span *clipSpans = clip->spans + *currentClip;
   const SW_FT_Span *clipEnd = clip->spans + clip->size;

   while (available && spans < end )
     {
        if (clipSpans >= clipEnd)
          {
             spans = end;
             break;
          }
        if (clipSpans->y > spans->y)
          {
             ++spans;
             continue;
          }
        if (spans->y != clipSpans->y)
          {
             ++clipSpans;
             continue;
          }
        //assert(spans->y == clipSpans->y);
        sx1 = spans->x;
        sx2 = sx1 + spans->len;
        cx1 = clipSpans->x;
        cx2 = cx1 + clipSpans->len;

        if (cx1 < sx1 && cx2 < sx1)
          {
             ++clipSpans;
             continue;
          }
        else if (sx1 < cx1 && sx2 < cx1)
          {
             ++spans;
             continue;
          }
        x = MAX(sx1, cx1);
        len = MIN(sx2, cx2) - x;
        if (len)
          {
             out->x = MAX(sx1, cx1);
             out->len = MIN(sx2, cx2) - out->x;
             out->y = spans->y;
             out->coverage = _div_255(spans->coverage * clipSpans->coverage);
             ++out;
             --available;
          }
        if (sx2 < cx2)
          {
             ++spans;
          }
        else
          {
             ++clipSpans;
          }
     }

   *out_spans = out;
   *currentClip = clipSpans - clip->spans;
   return spans;
}

/**
 * @internal
 * @brief Fills spans after clipping them with a list of rectangles.
 *
 * This function iterates through a list of clipping rectangles. For each rectangle,
 * it intersects the input spans with that rectangle and then calls the
 * `unclipped_blend` function (from `fill_data`) to render the resulting clipped spans.
 *
 * @param span_count The number of input spans.
 * @param spans An array of SW_FT_Span structures to be filled.
 * @param user_data A pointer to a Span_Data structure. This contains the list
 *                  of clipping rectangles (`clip.clips`), the offset (`offx`, `offy`),
 *                  and the actual blending function (`unclipped_blend`).
 *                  `clip.clips` is an Eina_Array of Eina_Rectangle.
 *                  Example `clip.clips` element: `Eina_Rectangle { .x=0, .y=0, .w=10, .h=10 }`
 */
static void
_span_fill_clipRect(int span_count, const SW_FT_Span *spans, void *user_data)
{
   const int NSPANS = 256;
   int clip_count, i;
   Span_Data *fill_data = (Span_Data *) user_data;
   Clip_Data clip = fill_data->clip;
   SW_FT_Span *clipped;
   Eina_Rectangle *rect;
   Eina_Rectangle tmp_rect;
   SW_FT_Span *cspans = NULL;
   Eina_Bool intersect = EINA_FALSE;

   //Note: Uses same span_count sized heap memory in intersect mask case.
   if (fill_data->comp_method == EFL_GFX_VG_COMPOSITE_METHOD_MASK_INTERSECT)
     {
        intersect = EINA_TRUE;
        cspans = malloc(sizeof(SW_FT_Span) * (span_count));
        if (!cspans)
          {
             ERR("OOM: Failed malloc()");
             return ;
          }
     }
   else
     {
        cspans = alloca(sizeof(SW_FT_Span) * (NSPANS));
     }

   clip_count = eina_array_count(clip.clips);

   for (i = 0; i < clip_count; i++)
     {
        rect = (Eina_Rectangle *)eina_array_data_get(clip.clips, i);

        // invert transform the offset
        tmp_rect.x = rect->x - fill_data->offx;
        tmp_rect.y = rect->y - fill_data->offy;
        tmp_rect.w = rect->w;
        tmp_rect.h = rect->h;
        const SW_FT_Span *end = spans + span_count;

        while (spans < end)
          {
             clipped = cspans;
             spans = _intersect_spans_rect(&tmp_rect, spans, end, &clipped, intersect ? span_count : NSPANS);
             if (clipped - cspans)
               fill_data->unclipped_blend(clipped - cspans, cspans, fill_data);
          }
     }
   if (intersect && cspans) free(cspans);
}

/**
 * @internal
 * @brief Fills spans after clipping them with a path (RLE data).
 *
 * This function intersects the input spans with a clipping path (represented
 * as RLE data) and then calls the `unclipped_blend` function (from `fill_data`)
 * to render the resulting clipped spans.
 *
 * @param span_count The number of input spans.
 * @param spans An array of SW_FT_Span structures to be filled.
 * @param user_data A pointer to a Span_Data structure. This contains the
 *                  clipping path (`clip.path`), and the actual blending
 *                  function (`unclipped_blend`). `clip.path` is a Shape_Rle_Data.
 */
static void
_span_fill_clipPath(int span_count, const SW_FT_Span *spans, void *user_data)
{
   const int NSPANS = 256;
   int current_clip = 0;
   Span_Data *fill_data = (Span_Data *) user_data;
   Clip_Data clip = fill_data->clip;
   SW_FT_Span *clipped;
   SW_FT_Span *cspans = NULL;
   Eina_Bool intersect = EINA_FALSE;

   //Note: Uses same span_count sized heap memory in intersect mask case.
   if (fill_data->comp_method == EFL_GFX_VG_COMPOSITE_METHOD_MASK_INTERSECT)
     {
        intersect = EINA_TRUE;
        cspans = malloc(sizeof(SW_FT_Span) * (span_count));
        if (!cspans)
          {
             ERR("OOM: Failed malloc()");
             return ;
          }
     }
   else
     {
        cspans = alloca(sizeof(SW_FT_Span) * (NSPANS));
     }

   // FIXME: Take clip path offset into account.
   const SW_FT_Span *end = spans + span_count;
   while (spans < end)
     {
        clipped = cspans;
        spans = _intersect_spans_region(clip.path, &current_clip, spans, end, &clipped, intersect ? span_count : NSPANS);
        if (clipped - cspans)
          fill_data->unclipped_blend(clipped - cspans, cspans, fill_data);
     }
   if (intersect && cspans) free(cspans);
}

/**
 * @internal
 * @brief Adjusts and selects the appropriate blending and clipping functions.
 *
 * Based on the properties set in `spdata` (like fill type, composition mode,
 * and clipping state), this function assigns the correct function pointers
 * to `spdata->unclipped_blend` and `spdata->blend`.
 * `spdata->unclipped_blend` is the function that performs the actual pixel blending
 * without considering clipping.
 * `spdata->blend` is the top-level function that will be called by the rasterizer;
 * it might be a clipping function (which then calls `unclipped_blend`) or
 * `unclipped_blend` directly if no clipping is enabled.
 *
 * @param spdata Pointer to the Span_Data structure to be configured.
 */
static void
_adjust_span_fill_methods(Span_Data *spdata)
{
   //Blending Function
   if (spdata->comp)
     {
        switch (spdata->comp_method)
          {
           default:
           case EFL_GFX_VG_COMPOSITE_METHOD_MATTE_ALPHA:
              if (spdata->type == Solid)
                spdata->unclipped_blend = &_comp_matte_alpha;
              else if (spdata->type == LinearGradient || spdata->type == RadialGradient)
                spdata->unclipped_blend = &_blend_gradient_alpha;
              else //None
                spdata->unclipped_blend = NULL;
              break;
           case EFL_GFX_VG_COMPOSITE_METHOD_MATTE_ALPHA_INVERSE:
              if (spdata->type == Solid)
                spdata->unclipped_blend = &_comp_matte_alpha_inv;
              else if (spdata->type == LinearGradient || spdata->type == RadialGradient)
                spdata->unclipped_blend = &_blend_gradient_alpha_inv;
              else //None
                spdata->unclipped_blend = NULL;
              break;
           case EFL_GFX_VG_COMPOSITE_METHOD_MASK_ADD:
              spdata->unclipped_blend = &_comp_mask_add;
              break;
           case EFL_GFX_VG_COMPOSITE_METHOD_MASK_SUBSTRACT:
              spdata->unclipped_blend = &_comp_mask_sub;
              break;
           case EFL_GFX_VG_COMPOSITE_METHOD_MASK_INTERSECT:
              spdata->unclipped_blend = &_comp_mask_ins;
              break;
           case EFL_GFX_VG_COMPOSITE_METHOD_MASK_DIFFERENCE:
              spdata->unclipped_blend = &_comp_mask_diff;
              break;
          }
     }
   else
     {
        if (spdata->type == Solid)
          spdata->unclipped_blend = &_blend_argb;
        else if (spdata->type == LinearGradient || spdata->type == RadialGradient)
          spdata->unclipped_blend = &_blend_gradient;
        else //None
          spdata->unclipped_blend = NULL;
     }

   // Clipping Function
   if (spdata->clip.enabled)
     {
        if (spdata->clip.type == 0)
          spdata->blend = &_span_fill_clipRect;
        else
          spdata->blend = &_span_fill_clipPath;
     }
   else
     spdata->blend = spdata->unclipped_blend;
}

/**
 * @internal
 * @brief Initializes software rasterizer resources specific to a thread.
 *
 * This function sets up the FreeType rasterizer and stroker instances
 * that will be used by a particular Ector software rendering thread.
 *
 * @param thread Pointer to the Ector_Software_Thread structure to initialize.
 */
void ector_software_thread_init(Ector_Software_Thread *thread)
{
   // initialize the rasterizer and stroker
   sw_ft_grays_raster.raster_new(&thread->raster);

   SW_FT_Stroker_New(&thread->stroker);
   SW_FT_Stroker_Set(thread->stroker, 1 << 6,
                     SW_FT_STROKER_LINECAP_BUTT, SW_FT_STROKER_LINEJOIN_MITER_FIXED, 0x4<<16);
}

/**
 * @internal
 * @brief Initializes a Software_Rasterizer instance.
 *
 * Sets up initial values for the rasterizer's fill data, including disabling
 * clipping by default and initializing drawing and gradient systems.
 *
 * @param rasterizer Pointer to the Software_Rasterizer structure to initialize.
 */
void ector_software_rasterizer_init(Software_Rasterizer *rasterizer)
{
   //initialize the span data.
   rasterizer->fill_data.clip.enabled = EINA_FALSE;
   rasterizer->fill_data.unclipped_blend = 0;
   rasterizer->fill_data.blend = 0;
   efl_draw_init();
   ector_software_gradient_init();
}

/**
 * @internal
 * @brief Shuts down and cleans up thread-specific software rasterizer resources.
 *
 * Releases the FreeType rasterizer and stroker instances associated with
 * the given Ector software rendering thread.
 *
 * @param thread Pointer to the Ector_Software_Thread structure whose resources
 *               are to be released.
 */
void ector_software_thread_shutdown(Ector_Software_Thread *thread)
{
   sw_ft_grays_raster.raster_done(thread->raster);
   SW_FT_Stroker_Done(thread->stroker);
}

/**
 * @internal
 * @brief Configures the stroker for rendering outlines.
 *
 * Sets the stroke properties (width, cap style, join style, miter limit)
 * on the FreeType stroker associated with the given thread. The stroke width
 * might be adjusted based on the transformation matrix `m` to account for scaling.
 *
 * @param thread Pointer to the Ector_Software_Thread containing the stroker.
 * @param rasterizer The software rasterizer (currently unused in this function).
 * @param width The desired stroke width.
 * @param cap_style The style for line endings (e.g., butt, round, square).
 * @param join_style The style for line joins (e.g., miter, round, bevel).
 * @param m Optional transformation matrix. If provided, its scale factors are
 *          used to adjust the stroke width.
 *          Example: `m = { {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0} }` (identity)
 * @param miterlimit The miter limit for miter joins.
 */
void ector_software_rasterizer_stroke_set(Ector_Software_Thread *thread,
                                          Software_Rasterizer *rasterizer EINA_UNUSED, double width,
                                          Efl_Gfx_Cap cap_style, Efl_Gfx_Join join_style,
                                          Eina_Matrix3 *m, double miterlimit)
{
   SW_FT_Stroker_LineCap cap;
   SW_FT_Stroker_LineJoin join;
   int stroke_width;
   double scale_factor = 1.0;

   // convert to freetype co-ordinate
   SW_FT_Fixed miter_limit = miterlimit * (1<<16);

   if (m)
     {
        // get the minimum scale factor from matrix
        scale_factor =  m->xx < m->yy ? m->xx : m->yy;
     }
   width = width * scale_factor;
   width = width/2.0; // as free type uses it as the radius of the
                      // pen not the diameter.
   // convert to freetype co-ordinate
   stroke_width = (int)(width * 64);

   switch (cap_style)
     {
        case EFL_GFX_CAP_SQUARE:
          cap = SW_FT_STROKER_LINECAP_SQUARE;
          break;
        case EFL_GFX_CAP_ROUND:
          cap = SW_FT_STROKER_LINECAP_ROUND;
          break;
        default:
          cap = SW_FT_STROKER_LINECAP_BUTT;
          break;
     }

   switch (join_style)
     {
        case EFL_GFX_JOIN_BEVEL:
          join = SW_FT_STROKER_LINEJOIN_BEVEL;
          break;
        case EFL_GFX_JOIN_ROUND:
          join = SW_FT_STROKER_LINEJOIN_ROUND;
          break;
        default:
          join = SW_FT_STROKER_LINEJOIN_MITER_FIXED;
          break;
     }
   SW_FT_Stroker_Set(thread->stroker, stroke_width, cap, join, miter_limit);
}

/**
 * @internal
 * @brief Callback function used by FreeType rasterizer to collect generated spans into RLE data.
 *
 * This function is invoked by `sw_ft_grays_raster.raster_render` for each batch
 * of spans generated. It appends these spans to a dynamically growing
 * `Shape_Rle_Data` structure.
 *
 * @param count The number of spans in the current batch.
 * @param spans An array of SW_FT_Span structures generated by the rasterizer.
 * @param user A pointer to the Shape_Rle_Data structure where spans should be stored.
 */
static void
_rle_generation_cb( int count, const SW_FT_Span*  spans,void *user)
{
   Shape_Rle_Data *rle = (Shape_Rle_Data *) user;
   int newsize = rle->size + count;

   // allocate enough memory for new spans
   // alloc is required to prevent free and reallocation
   // when the rle needs to be regenerated because of attribute change.
   if (rle->alloc < newsize)
     {
        rle->spans = (SW_FT_Span *) realloc(rle->spans, newsize * sizeof(SW_FT_Span));
        rle->alloc = newsize;
     }

   // copy the new spans to the allocated memory
   SW_FT_Span *lastspan = (rle->spans + rle->size);
   memcpy(lastspan,spans, count * sizeof(SW_FT_Span));

   // update the size
   rle->size = newsize;
}

/**
 * @internal
 * @brief Generates Run-Length Encoded (RLE) data from a FreeType outline.
 *
 * Rasterizes the given FreeType outline and collects the generated spans into
 * a `Shape_Rle_Data` structure. This RLE data can then be used for filling
 * or clipping. Also calculates the bounding box of the RLE data.
 *
 * @param thread Pointer to the Ector_Software_Thread containing the rasterizer.
 * @param rasterizer The software rasterizer (currently unused in this function).
 * @param outline Pointer to the SW_FT_Outline to be rasterized.
 *                Example: `outline.points`, `outline.tags`, `outline.contours`
 *                define the shape.
 * @return A newly allocated Shape_Rle_Data structure containing the spans and
 *         bounding box, or NULL on allocation failure. The caller is responsible
 *         for freeing this structure using `ector_software_rasterizer_destroy_rle_data`.
 *         Example `rle_data->spans`: array of `SW_FT_Span { .x, .y, .len, .coverage }`
 *         Example `rle_data->bbox`: `Eina_Rectangle { .x, .y, .w, .h }`
 */
Shape_Rle_Data *
ector_software_rasterizer_generate_rle_data(Ector_Software_Thread *thread,
                                            Software_Rasterizer *rasterizer EINA_UNUSED,
                                            SW_FT_Outline *outline)
{
   int i, rle_size;
   int l = 0, t = 0, r = 0, b = 0;
   Shape_Rle_Data *rle_data = (Shape_Rle_Data *) calloc(1, sizeof(Shape_Rle_Data));
   SW_FT_Raster_Params params;
   SW_FT_Span* span;

   params.flags = SW_FT_RASTER_FLAG_DIRECT | SW_FT_RASTER_FLAG_AA ;
   params.gray_spans = &_rle_generation_cb;
   params.user = rle_data;
   params.source = outline;

   sw_ft_grays_raster.raster_render(thread->raster, &params);

   // update RLE bounding box.
   span = rle_data->spans;
   rle_size = rle_data->size;
   if (rle_size)
     {
        t = span[0].y;
        b = span[rle_size-1].y;
        for (i = 0; i < rle_size; i++)
          {
             if (span[i].x < l) l = span[i].x;
             if (span[i].x + span[i].len > r) r = span[i].x + span[i].len;
          }
        rle_data->bbox.x = l;
        rle_data->bbox.y = t;
        rle_data->bbox.w = r - l;
        rle_data->bbox.h = b - t + 1;
     }
   return rle_data;
}

/**
 * @internal
 * @brief Generates Run-Length Encoded (RLE) data for the stroke of a FreeType outline.
 *
 * First, it uses the FreeType stroker (configured by
 * `ector_software_rasterizer_stroke_set`) to generate an outline representing
 * the stroke of the input `outline`. Then, it rasterizes this stroke outline
 * to produce RLE data.
 *
 * @param thread Pointer to the Ector_Software_Thread containing the stroker and rasterizer.
 * @param rasterizer The software rasterizer.
 * @param outline Pointer to the original SW_FT_Outline to be stroked.
 * @param closePath EINA_TRUE if the path should be closed before stroking, EINA_FALSE otherwise.
 * @return A newly allocated Shape_Rle_Data structure for the stroke, or NULL
 *         on allocation failure. The caller is responsible for freeing this.
 */
Shape_Rle_Data *
ector_software_rasterizer_generate_stroke_rle_data(Ector_Software_Thread *thread,
                                                   Software_Rasterizer *rasterizer,
                                                   SW_FT_Outline *outline,
                                                   Eina_Bool closePath)
{
   uint32_t points,contors;
   Shape_Rle_Data *rle_data;
   SW_FT_Outline strokeOutline = { 0, 0, NULL, NULL, NULL, 0 };

   SW_FT_Stroker_ParseOutline(thread->stroker, outline, !closePath);
   SW_FT_Stroker_GetCounts(thread->stroker,&points, &contors);

   strokeOutline.points = (SW_FT_Vector *) calloc(points, sizeof(SW_FT_Vector));
   strokeOutline.tags = (char *) calloc(points, sizeof(char));
   strokeOutline.contours = (short *) calloc(contors, sizeof(short));

   SW_FT_Stroker_Export(thread->stroker, &strokeOutline);

   rle_data = ector_software_rasterizer_generate_rle_data(thread, rasterizer, &strokeOutline);

   // cleanup the outline data.
   free(strokeOutline.points);
   free(strokeOutline.tags);
   free(strokeOutline.contours);

   return rle_data;
}

/**
 * @internal
 * @brief Frees the memory associated with a Shape_Rle_Data structure.
 *
 * Releases the `spans` array and the `Shape_Rle_Data` structure itself.
 *
 * @param rle Pointer to the Shape_Rle_Data structure to be destroyed.
 *            If NULL, the function does nothing.
 */
void
ector_software_rasterizer_destroy_rle_data(Shape_Rle_Data *rle)
{
   if (rle)
     {
        if (rle->spans)
          free(rle->spans);
        free(rle);
     }
}

/**
 * @internal
 * @brief Sets up the inverse transformation matrix for span filling.
 *
 * If a transformation matrix (`rasterizer->transform`) is set, this function
 * computes its inverse and stores it in `rasterizer->fill_data.inv`.
 * If no transform is set, `fill_data.inv` is set to the identity matrix.
 * This inverse matrix is used by gradient fetch functions to map destination
 * coordinates back to source gradient space.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 */
static
void _setup_span_fill_matrix(Software_Rasterizer *rasterizer)
{
   if (rasterizer->transform)
     eina_matrix3_inverse(rasterizer->transform, &rasterizer->fill_data.inv);
   else
     eina_matrix3_identity(&rasterizer->fill_data.inv);
}

/**
 * @internal
 * @brief Sets the transformation matrix for the rasterizer.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param t Pointer to an Eina_Matrix3 to be used for transformations.
 *          Can be NULL to clear the transform.
 *          Example: `t = eina_matrix3_identity_new();`
 */
void
ector_software_rasterizer_transform_set(Software_Rasterizer *rasterizer, Eina_Matrix3 *t)
{
   rasterizer->transform = t;
}

/**
 * @internal
 * @brief Sets rectangular clipping regions for the rasterizer.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param clips An Eina_Array containing Eina_Rectangle pointers that define
 *              the clipping regions. If NULL, clipping is disabled.
 *              Example: `clips_array` might contain `Eina_Rectangle {10,10,50,50}`.
 */
void
ector_software_rasterizer_clip_rect_set(Software_Rasterizer *rasterizer, Eina_Array *clips)
{
   if (clips)
     {
        rasterizer->fill_data.clip.clips = clips;
        rasterizer->fill_data.clip.type = 0;
        rasterizer->fill_data.clip.enabled = EINA_TRUE;
     }
   else
     {
        rasterizer->fill_data.clip.clips = NULL;
        rasterizer->fill_data.clip.type = 0;
        rasterizer->fill_data.clip.enabled = EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Sets a shape-based clipping region for the rasterizer.
 *
 * The clipping shape is defined by RLE data.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param clip Pointer to Shape_Rle_Data representing the clipping path.
 *             If NULL, clipping is disabled.
 */
void
ector_software_rasterizer_clip_shape_set(Software_Rasterizer *rasterizer, Shape_Rle_Data *clip)
{
   if (clip)
     {
        rasterizer->fill_data.clip.path = clip;
        rasterizer->fill_data.clip.type = 1;
        rasterizer->fill_data.clip.enabled = EINA_TRUE;
     }
   else
     {
        rasterizer->fill_data.clip.path = NULL;
        rasterizer->fill_data.clip.type = 0;
        rasterizer->fill_data.clip.enabled = EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Sets the solid fill color for the rasterizer.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param a Alpha component (0-255).
 */
void
ector_software_rasterizer_color_set(Software_Rasterizer *rasterizer, int r, int g, int b, int a)
{
   rasterizer->fill_data.color = DRAW_ARGB_JOIN(a, r, g, b);
   rasterizer->fill_data.type = Solid;
}

/**
 * @internal
 * @brief Sets a linear gradient fill for the rasterizer.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param linear Pointer to Ector_Renderer_Software_Gradient_Data containing
 *               the linear gradient's definition (stops, geometry).
 */
void ector_software_rasterizer_linear_gradient_set(Software_Rasterizer *rasterizer,
                                                   Ector_Renderer_Software_Gradient_Data *linear)
{
   rasterizer->fill_data.gradient = linear;
   rasterizer->fill_data.type = LinearGradient;
}

/**
 * @internal
 * @brief Sets a radial gradient fill for the rasterizer.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param radial Pointer to Ector_Renderer_Software_Gradient_Data containing
 *               the radial gradient's definition (stops, geometry).
 */
void
ector_software_rasterizer_radial_gradient_set(Software_Rasterizer *rasterizer,
                                              Ector_Renderer_Software_Gradient_Data *radial)
{
   rasterizer->fill_data.gradient = radial;
   rasterizer->fill_data.type = RadialGradient;
}

/**
 * @internal
 * @brief Draws RLE (Run-Length Encoded) data onto the rasterizer's target buffer.
 *
 * This is a central drawing function. It configures the `rasterizer->fill_data`
 * with drawing parameters (offset, color modulation, operator, composition settings),
 * sets up transformation matrices, adjusts fill methods (choosing appropriate
 * blending and clipping functions), and then invokes the selected blend function
 * to render the RLE spans.
 *
 * @param rasterizer Pointer to the Software_Rasterizer.
 * @param x The x-offset for drawing the RLE data.
 * @param y The y-offset for drawing the RLE data.
 * @param mul_col A color multiplier (ARGB) applied to the fill color or gradient.
 *                Example: `0xFFFFFFFF` for no change, `0x80FFFFFF` for 50% alpha.
 * @param op The rendering operation (e.g., EFL_GFX_RENDER_OP_BLEND).
 * @param rle Pointer to the Shape_Rle_Data to be drawn.
 * @param comp Optional Ector_Buffer used as a composition mask/matte.
 * @param comp_method The composition method to use if `comp` is provided
 *                    (e.g., EFL_GFX_VG_COMPOSITE_METHOD_MATTE_ALPHA).
 */
void
ector_software_rasterizer_draw_rle_data(Software_Rasterizer *rasterizer,
                                        int x, int y, uint32_t mul_col,
                                        Efl_Gfx_Render_Op op, Shape_Rle_Data* rle,
                                        Ector_Buffer *comp,
                                        Efl_Gfx_Vg_Composite_Method comp_method)
{
   if (!rle) return;
   if (!rasterizer->fill_data.raster_buffer->pixels.u32) return;

   rasterizer->fill_data.offx = x;
   rasterizer->fill_data.offy = y;
   rasterizer->fill_data.mul_col = mul_col;
   rasterizer->fill_data.op = op;
   rasterizer->fill_data.comp =
         comp ? efl_data_scope_get(comp, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN) : NULL;
   rasterizer->fill_data.comp_method = comp_method;

   _setup_span_fill_matrix(rasterizer);
   _adjust_span_fill_methods(&rasterizer->fill_data);

   if (rasterizer->fill_data.blend)
     rasterizer->fill_data.blend(rle->size, rle->spans, &rasterizer->fill_data);
}
