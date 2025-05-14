#define NEED_SSE3 1

#include "Eina.h"
#include "Evas.h"
#include "evas_common_types.h"

EXPORTAPI void evas_common_cpu_end_opt(void);

#include "config.h"
#include "evas_blend_ops.h"

#ifdef BUILD_SSE3
/**
 * @internal
 * @brief SSE3 mask to isolate the Alpha channel of a 32-bit RGBA pixel.
 *
 * This mask is used in SSE3 operations to extract or manipulate the alpha component
 * from four 32-bit pixels packed into an __m128i register.
 * Each 32-bit segment of the mask is 0xFF000000, targeting the most significant byte (alpha).
 */
static __m128i A_MASK_SSE3;
#endif

/**
 * @internal
 * @brief External declarations for global lookup tables of blending functions.
 *
 * These multi-dimensional arrays store pointers to specific blending functions
 * optimized for different pixel formats, color properties, mask usage, and CPU features.
 * They are categorized into span operations (processing multiple pixels) and point
 * operations (processing single pixels), as well as standard and "relative" blending modes.
 *
 * The specific functions for SSE3 are assigned in
 * evas_common_op_blend_init_sse3() and evas_common_op_blend_rel_init_sse3().
 *
 * The dimensions typically follow a pattern like [SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST],
 * representing various source (S) and destination (D) properties (P=Pixel, M=Mask, C=Color)
 * and CPU capabilities.
 */
extern RGBA_Gfx_Func     op_blend_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
extern RGBA_Gfx_Pt_Func  op_blend_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

extern RGBA_Gfx_Func     op_blend_rel_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
extern RGBA_Gfx_Pt_Func  op_blend_rel_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

# include "op_blend_pixel_sse3.c"
# include "op_blend_color_sse3.c"
# include "op_blend_pixel_color_sse3.c"
# include "op_blend_pixel_mask_sse3.c"
# include "op_blend_mask_color_sse3.c"

/**
 * @internal
 * @brief Initializes SSE3 optimized blending functions.
 *
 * This function populates the global blending function lookup tables
 * (e.g., op_blend_span_funcs, op_blend_pt_funcs) with pointers to
 * SSE3-specific implementations for various blending operations.
 * It also initializes SSE3 specific masks used by these operations, such as
 * GA_MASK_SSE3, RB_MASK_SSE3, A_MASK_SSE3, etc., which are used to isolate
 * specific color channels or alpha components within __m128i registers.
 * This function should be called once at startup if SSE3 support is detected and enabled.
 *
 * The included files (e.g., op_blend_pixel_sse3.c) contain the actual
 * initialization routines (e.g., init_blend_pixel_span_funcs_sse3()) for
 * different categories of blending operations.
 */
void
evas_common_op_blend_init_sse3(void)
{
#ifdef BUILD_SSE3
   GA_MASK_SSE3 = _mm_set_epi32(0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF);
   RB_MASK_SSE3 = _mm_set_epi32(0xFF00FF00, 0xFF00FF00, 0xFF00FF00, 0xFF00FF00);
   SYM4_MASK_SSE3 = _mm_set_epi32(0x00FF00FF, 0x000000FF, 0x00FF00FF, 0x000000FF);
   RGB_MASK_SSE3 = _mm_set_epi32(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF);
   A_MASK_SSE3 = _mm_set_epi32(0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000);
   ALPHA_SSE3 = _mm_set_epi32(256, 256, 256, 256);

   init_blend_pixel_span_funcs_sse3();
   init_blend_pixel_color_span_funcs_sse3();
   init_blend_pixel_mask_span_funcs_sse3(); // FIXME
   init_blend_color_span_funcs_sse3();
   init_blend_mask_color_span_funcs_sse3();

   init_blend_pixel_pt_funcs_sse3();
   init_blend_pixel_color_pt_funcs_sse3();
   init_blend_pixel_mask_pt_funcs_sse3();
   init_blend_color_pt_funcs_sse3();
   init_blend_mask_color_pt_funcs_sse3();
#endif
}

/**
 * @internal
 * @brief Initializes SSE3 optimized "relative" blending functions.
 *
 * Similar to evas_common_op_blend_init_sse3(), but this function initializes
 * the lookup tables for "relative" blending operations (e.g., op_blend_rel_span_funcs,
 * op_blend_rel_pt_funcs). Relative blending modes might involve different
 * calculations or interpretations of color and alpha values compared to standard blending.
 * This function should be called once at startup if SSE3 support is detected and enabled.
 *
 * The included files (e.g., op_blend_pixel_sse3.c) contain the actual
 * initialization routines (e.g., init_blend_rel_pixel_span_funcs_sse3()) for
 * different categories of relative blending operations.
 */
void
evas_common_op_blend_rel_init_sse3(void)
{
#ifdef BUILD_SSE3
   init_blend_rel_pixel_span_funcs_sse3();
   init_blend_rel_pixel_color_span_funcs_sse3();
   init_blend_rel_pixel_mask_span_funcs_sse3(); // FIXME
   init_blend_rel_color_span_funcs_sse3();
   init_blend_rel_mask_color_span_funcs_sse3();

   init_blend_rel_pixel_pt_funcs_sse3();
   init_blend_rel_pixel_color_pt_funcs_sse3();
   init_blend_rel_pixel_mask_pt_funcs_sse3();
   init_blend_rel_color_pt_funcs_sse3();
   init_blend_rel_mask_color_pt_funcs_sse3();
#endif
}

//#pragma GCC push_options
//#pragma GCC optimize ("O0")
/**
 * @internal
 * @brief A simple test function for SSE3 blending operations.
 *
 * This function is intended for debugging or verification purposes.
 * It sets up a small amount of sample data (source `s` and destination `d` arrays)
 * and calls a specific SSE3 blending function (_op_blend_pas_dp_sse3)
 * to operate on this data.
 * The `evas_common_cpu_end_opt()` call might be used to flush any CPU
 * instruction pipelines or ensure completion of operations, specific to Evas's
 * optimization handling.
 *
 * @note This function is typically conditionally compiled with BUILD_SSE3.
 * The pragmas for disabling optimization (`#pragma GCC optimize ("O0")`) are
 * commented out but suggest that this function might have been used for
 * step-by-step debugging where optimizations could obscure the behavior.
 */
void
evas_common_op_sse3_test(void)
{
#ifdef BUILD_SSE3
   DATA32 s[64] = {0x11883399}, d[64] = {0xff88cc33};

   s[0] = rand(); d[1] = rand();
   _op_blend_pas_dp_sse3(s, NULL, 0, d, 64);
   evas_common_cpu_end_opt();
#endif
}
//#pragma GCC pop_options
