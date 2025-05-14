/**
 * @file
 * @brief This file contains the main blending and compositing functions for Evas.
 * It handles the initialization, shutdown, and retrieval of appropriate
 * compositing functions based on operation type and source/destination properties.
 */

#include "evas_common_private.h"
#include "evas_blend_private.h"

#ifdef BUILD_MMX
#include "evas_mmx.h"
#endif

const DATA32 ALPHA_255 = 255; /**< Represents fully opaque alpha (255). Used in alpha calculations. */
const DATA32 ALPHA_256 = 256; /**< Represents 256, often used as a multiplier in alpha blending for fixed-point arithmetic (255 + 1). */

/**
 * @brief A no-operation function for span compositing.
 *
 * This function is used as a fallback when no specific compositing
 * operation is available or applicable. It does nothing.
 *
 * @param s Source pixel data (unused).
 * @param m Mask data (unused).
 * @param c Color value (unused).
 * @param d Destination pixel data (unused).
 * @param l Length of the span (unused).
 */
static void
_composite_span_nothing(DATA32 *s EINA_UNUSED, DATA8 *m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d EINA_UNUSED, int l EINA_UNUSED)
{
}

/**
 * @brief A no-operation function for point compositing.
 *
 * This function is used as a fallback when no specific compositing
 * operation is available or applicable for a single point. It does nothing.
 *
 * @param s Source pixel data (unused).
 * @param m Mask data (unused).
 * @param c Color value (unused).
 * @param d Destination pixel data (unused).
 */
static void
_composite_pt_nothing(DATA32 s EINA_UNUSED, DATA8 m EINA_UNUSED, DATA32 c EINA_UNUSED, DATA32 *d EINA_UNUSED)
{
}

/**
 * @brief Retrieves a graphics compositor based on the rendering operation.
 *
 * This function acts as a dispatcher, returning the appropriate compositor
 * (e.g., for blend, copy, mask, multiply operations) based on the `op` code.
 *
 * @param op The rendering operation code (e.g., _EVAS_RENDER_BLEND, _EVAS_RENDER_COPY).
 * @return A pointer to the RGBA_Gfx_Compositor structure for the requested operation,
 *         or the blend compositor if the operation is unknown.
 */
static RGBA_Gfx_Compositor  *
evas_gfx_compositor_get(unsigned char op)
{
   RGBA_Gfx_Compositor  *comp;

   switch (op)
     {
      case _EVAS_RENDER_BLEND:
        comp = evas_common_gfx_compositor_blend_get();
        break;
      case _EVAS_RENDER_BLEND_REL:
        comp = evas_common_gfx_compositor_blend_rel_get();
        break;
      case _EVAS_RENDER_COPY:
        comp = evas_common_gfx_compositor_copy_get();
        break;
      case _EVAS_RENDER_COPY_REL:
        comp = evas_common_gfx_compositor_copy_rel_get();
        break;
        /*
      case _EVAS_RENDER_ADD:
        comp = evas_common_gfx_compositor_add_get();
        break;
      case _EVAS_RENDER_ADD_REL:
        comp = evas_common_gfx_compositor_add_rel_get();
        break;
      case _EVAS_RENDER_SUB:
        comp = evas_common_gfx_compositor_sub_get();
        break;
      case _EVAS_RENDER_SUB_REL:
        comp = evas_common_gfx_compositor_sub_rel_get();
        break;
      */
      case _EVAS_RENDER_MASK:
        comp = evas_common_gfx_compositor_mask_get();
        break;
      case _EVAS_RENDER_MUL:
        comp = evas_common_gfx_compositor_mul_get();
        break;
      default:
        comp = evas_common_gfx_compositor_blend_get();
        break;
     }
   return comp;
}

/**
 * @brief Initializes the common blending subsystem.
 *
 * This function sets up the graphics compositors for various rendering
 * operations. It detects CPU capabilities (MMX, SSE, SSE2) and initializes
 * each compositor. This function should be called once at startup.
 */
EVAS_API void
evas_common_blend_init(void)
{
   static int gfx_initialised = 0;
   static int mmx = 0;
   static int sse = 0;
   static int sse2 = 0;
   RGBA_Gfx_Compositor  *comp;

   if (gfx_initialised) return;
   gfx_initialised = 1;

   evas_common_cpu_can_do(&mmx, &sse, &sse2);

   comp = evas_common_gfx_compositor_copy_get();
   if (comp) comp->init();
   comp = evas_common_gfx_compositor_copy_rel_get();
   if (comp) comp->init();

   comp = evas_common_gfx_compositor_blend_get();
   if (comp) comp->init();
   comp = evas_common_gfx_compositor_blend_rel_get();
   if (comp) comp->init();

   /*
   comp = evas_common_gfx_compositor_add_get();
   if (comp) comp->init();
   comp = evas_common_gfx_compositor_add_rel_get();
   if (comp) comp->init();
   comp = evas_common_gfx_compositor_sub_get();
   if (comp) comp->init();
   comp = evas_common_gfx_compositor_sub_rel_get();
   if (comp) comp->init();
*/
   comp = evas_common_gfx_compositor_mask_get();
   if (comp) comp->init();

   comp = evas_common_gfx_compositor_mul_get();
   if (comp) comp->init();
}

/**
 * @brief Shuts down the common blending subsystem.
 *
 * This function calls the shutdown method for each initialized graphics
 * compositor, allowing them to release any resources. This function
 * should be called once during application termination.
 */
void
evas_common_blend_shutdown(void)
{
   RGBA_Gfx_Compositor  *comp;

   comp = evas_common_gfx_compositor_copy_get();
   if (comp) comp->shutdown();
   comp = evas_common_gfx_compositor_copy_rel_get();
   if (comp) comp->shutdown();

   comp = evas_common_gfx_compositor_blend_get();
   if (comp) comp->shutdown();
   comp = evas_common_gfx_compositor_blend_rel_get();
   if (comp) comp->shutdown();

   /*
   comp = evas_common_gfx_compositor_add_get();
   if (comp) comp->shutdown();
   comp = evas_common_gfx_compositor_add_rel_get();
   if (comp) comp->shutdown();
   comp = evas_common_gfx_compositor_sub_get();
   if (comp) comp->shutdown();
   comp = evas_common_gfx_compositor_sub_rel_get();
   if (comp) comp->shutdown();
   */

   comp = evas_common_gfx_compositor_mask_get();
   if (comp) comp->shutdown();

   comp = evas_common_gfx_compositor_mul_get();
   if (comp) comp->shutdown();
}

/**
 * @brief Gets a function for compositing a span of pixels from a source to a destination.
 *
 * Selects the appropriate compositing function based on source alpha properties,
 * destination alpha presence, number of pixels, and the rendering operation.
 * If the source has no alpha and the operation is blend, it's optimized to a copy.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param src_sparse_alpha EINA_TRUE if the source alpha is sparse (not all pixels have alpha).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param pixels The number of pixels in the span.
 * @param op The rendering operation code.
 * @return A pointer to the compositing function, or _composite_span_nothing if none is found.
 */
RGBA_Gfx_Func
evas_common_gfx_func_composite_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Func        func = NULL;

   if (!src_alpha)
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == _EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }

   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_span_get(src_alpha, src_sparse_alpha, dst_alpha, pixels);
   if (func)
     return func;
   return _composite_span_nothing;
}

/**
 * @brief Gets a function for compositing a solid color onto a span of destination pixels.
 *
 * Selects the appropriate compositing function based on the color's alpha,
 * destination alpha presence, number of pixels, and the rendering operation.
 * If the color is fully opaque and the operation is blend, it's optimized to a copy.
 *
 * @param col The color to composite (in ARGB format, e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param pixels The number of pixels in the span.
 * @param op The rendering operation code.
 * @return A pointer to the compositing function, or _composite_span_nothing if none is found.
 */
RGBA_Gfx_Func
evas_common_gfx_func_composite_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Func        func = NULL;

   if ((col & 0xff000000) == 0xff000000)
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }
   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_color_span_get(col, dst_alpha, pixels);
   if (func)
     return func;
   return _composite_span_nothing;
}

/**
 * @brief Gets a function for compositing a span of source pixels modulated by a color onto a destination.
 *
 * Selects the appropriate compositing function based on source alpha, color alpha,
 * destination alpha, number of pixels, and operation.
 * If the source has no alpha, the color is fully opaque, and the operation is blend,
 * it's optimized to a copy.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param src_sparse_alpha EINA_TRUE if the source alpha is sparse.
 * @param col The color to modulate the source pixels with (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param pixels The number of pixels in the span.
 * @param op The rendering operation code.
 * @return A pointer to the compositing function, or _composite_span_nothing if none is found.
 */
RGBA_Gfx_Func
evas_common_gfx_func_composite_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Func        func = NULL;

   if ((!src_alpha) && ((col & 0xff000000) == 0xff000000))
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == _EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }
   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_color_span_get(src_alpha, src_sparse_alpha, col, dst_alpha, pixels);
   if (func)
     return func;
   return _composite_span_nothing;
}

/**
 * @brief Gets a function for compositing a solid color onto a span of destination pixels, using a mask.
 *
 * Selects the appropriate compositing function based on the color, destination alpha,
 * number of pixels, and the rendering operation. The mask data is provided separately
 * to the returned function.
 *
 * @param col The color to composite (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param pixels The number of pixels in the span.
 * @param op The rendering operation code.
 * @return A pointer to the compositing function, or _composite_span_nothing if none is found.
 */
RGBA_Gfx_Func
evas_common_gfx_func_composite_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Func        func = NULL;

   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_mask_color_span_get(col, dst_alpha, pixels);
   if (func)
     return func;
   return _composite_span_nothing;
}

/**
 * @brief Gets a function for compositing a span of source pixels onto a destination, using a mask.
 *
 * Selects the appropriate compositing function based on source alpha, destination alpha,
 * number of pixels, and the rendering operation. The mask data is provided separately
 * to the returned function.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param src_sparse_alpha EINA_TRUE if the source alpha is sparse.
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param pixels The number of pixels in the span.
 * @param op The rendering operation code.
 * @return A pointer to the compositing function, or _composite_span_nothing if none is found.
 */
RGBA_Gfx_Func
evas_common_gfx_func_composite_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Func        func = NULL;

   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_mask_span_get(src_alpha, src_sparse_alpha, dst_alpha, pixels);
   if (func)
     return func;
   return _composite_span_nothing;
}

/**
 * @brief Gets a function for compositing a single source pixel onto a destination pixel.
 *
 * Selects the appropriate compositing function based on source alpha,
 * destination alpha, and the rendering operation.
 * If the source has no alpha and the operation is blend, it's optimized to a copy.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param op The rendering operation code.
 * @return A pointer to the point compositing function, or _composite_pt_nothing if none is found.
 */
RGBA_Gfx_Pt_Func
evas_common_gfx_func_composite_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Pt_Func     func = NULL;

   if (!src_alpha)
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == _EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }
   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_pt_get(src_alpha, dst_alpha);
   if (func)
     return func;
   return _composite_pt_nothing;
}

/**
 * @brief Gets a function for compositing a single solid color pixel onto a destination pixel.
 *
 * Selects the appropriate compositing function based on the color's alpha,
 * destination alpha, and the rendering operation.
 * If the color is fully opaque and the operation is blend, it's optimized to a copy.
 *
 * @param col The color to composite (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param op The rendering operation code.
 * @return A pointer to the point compositing function, or _composite_pt_nothing if none is found.
 */
RGBA_Gfx_Pt_Func
evas_common_gfx_func_composite_color_pt_get(DATA32 col, Eina_Bool dst_alpha, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Pt_Func     func = NULL;

   if ((col & 0xff000000) == 0xff000000)
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }
   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_color_pt_get(col, dst_alpha);
   if (func)
     return func;
   return _composite_pt_nothing;
}

/**
 * @brief Gets a function for compositing a single source pixel modulated by a color onto a destination pixel.
 *
 * Selects the appropriate compositing function based on source alpha, color alpha,
 * destination alpha, and the rendering operation.
 * If the source has no alpha, the color is fully opaque, and the operation is blend,
 * it's optimized to a copy.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param col The color to modulate the source pixel with (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param op The rendering operation code.
 * @return A pointer to the point compositing function, or _composite_pt_nothing if none is found.
 */
RGBA_Gfx_Pt_Func
evas_common_gfx_func_composite_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Pt_Func     func = NULL;

   if ((!src_alpha) && ((col & 0xff000000) == 0xff000000))
     {
        if (op == _EVAS_RENDER_BLEND) op = _EVAS_RENDER_COPY;
        else if (op == _EVAS_RENDER_BLEND_REL) op = _EVAS_RENDER_COPY_REL;
     }
   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_color_pt_get(src_alpha, col, dst_alpha);
   if (func)
     return func;
   return _composite_pt_nothing;
}

/**
 * @brief Gets a function for compositing a single solid color pixel onto a destination pixel, using a mask.
 *
 * Selects the appropriate compositing function based on the color, destination alpha,
 * and the rendering operation. The mask data (a single DATA8 value) is provided
 * separately to the returned function.
 *
 * @param col The color to composite (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param op The rendering operation code.
 * @return A pointer to the point compositing function, or _composite_pt_nothing if none is found.
 */
RGBA_Gfx_Pt_Func
evas_common_gfx_func_composite_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Pt_Func     func = NULL;

   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_mask_color_pt_get(col, dst_alpha);
   if (func)
     return func;
   return _composite_pt_nothing;
}

/**
 * @brief Gets a function for compositing a single source pixel onto a destination pixel, using a mask.
 *
 * Selects the appropriate compositing function based on source alpha, destination alpha,
 * and the rendering operation. The mask data (a single DATA8 value) is provided
 * separately to the returned function.
 *
 * @param src_alpha EINA_TRUE if the source has an alpha channel.
 * @param dst_alpha EINA_TRUE if the destination has an alpha channel.
 * @param op The rendering operation code.
 * @return A pointer to the point compositing function, or _composite_pt_nothing if none is found.
 */
RGBA_Gfx_Pt_Func
evas_common_gfx_func_composite_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha, int op)
{
   RGBA_Gfx_Compositor  *comp;
   RGBA_Gfx_Pt_Func     func = NULL;

   comp = evas_gfx_compositor_get(op);
   if (comp)
     func = comp->composite_pixel_mask_pt_get(src_alpha, dst_alpha);
   if (func)
     return func;
   return _composite_pt_nothing;
}
