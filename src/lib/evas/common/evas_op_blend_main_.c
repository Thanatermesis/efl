#include "evas_common_private.h"

/**
 * @brief Array of function pointers for span blending operations.
 *
 * This 5-dimensional array stores function pointers for different blending
 * operations on spans of pixels. The dimensions represent:
 * - Source pixel properties (e.g., alpha, no alpha)
 * - Source mask properties (e.g., no mask, alpha mask)
 * - Source color properties (e.g., solid color, alpha color)
 * - Destination pixel properties (e.g., alpha, no alpha)
 * - CPU-specific implementations (e.g., C, MMX, SSE3, NEON)
 *
 * Example: op_blend_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] would
 * point to a C implementation for blending a source with no alpha, no mask,
 * no special color properties, onto a destination with no alpha.
 */
RGBA_Gfx_Func     op_blend_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Array of function pointers for point (single pixel) blending operations.
 *
 * Similar to op_blend_span_funcs, but for operations on individual pixels.
 * The dimensions have the same meaning.
 *
 * Example: op_blend_pt_funcs[SP][SM_N][SC_N][DP][CPU_SSE3] would
 * point to an SSE3 implementation for blending a source with alpha, no mask,
 * no special color properties, onto a destination with alpha.
 */
RGBA_Gfx_Pt_Func  op_blend_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the blend operation functions.
 *
 * Sets up the op_blend_span_funcs and op_blend_pt_funcs tables with
 * appropriate function pointers based on CPU capabilities (MMX, SSE3, NEON)
 * and generic C implementations.
 */
static void op_blend_init(void);
/**
 * @brief Shuts down the blend operation module.
 * @note Currently, this function is a no-op.
 */
static void op_blend_shutdown(void);

/**
 * @brief Retrieves a span blending function for pixel-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse (not used by this compositor).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate span blending routine.
 */
static RGBA_Gfx_Func op_blend_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span blending function for color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate span blending routine.
 */
static RGBA_Gfx_Func op_blend_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span blending function for pixel-and-color-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse.
 * @param col The color multiplier as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate span blending routine.
 */
static RGBA_Gfx_Func op_blend_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span blending function for mask-and-color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate span blending routine.
 */
static RGBA_Gfx_Func op_blend_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span blending function for pixel-and-mask-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse (not used by this compositor).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate span blending routine.
 */
static RGBA_Gfx_Func op_blend_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point blending function for pixel-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point blending function for color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point blending function for pixel-and-color-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param col The color multiplier as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point blending function for mask-and-color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point blending function for pixel-and-mask-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for standard blend operations.
 *
 * This structure defines the "blend" compositor, providing function pointers
 * for initialization, shutdown, and various span/point blending operations.
 */
static RGBA_Gfx_Compositor  _composite_blend = { "blend",
 op_blend_init, op_blend_shutdown,
 op_blend_pixel_span_get, op_blend_color_span_get,
 op_blend_pixel_color_span_get, op_blend_mask_color_span_get,
 op_blend_pixel_mask_span_get,
 op_blend_pixel_pt_get, op_blend_color_pt_get,
 op_blend_pixel_color_pt_get, op_blend_mask_color_pt_get,
 op_blend_pixel_mask_pt_get
 };

/**
 * @brief Gets the standard blend compositor.
 * @return A pointer to the _composite_blend structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_blend_get(void)
{
   return &(_composite_blend);
}

/**
 * @brief Array of function pointers for relative span blending operations.
 * @see op_blend_span_funcs for details on the array structure.
 * "Relative" blending typically means the alpha of the source is modulated
 * by the alpha of the destination.
 */
RGBA_Gfx_Func     op_blend_rel_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
/**
 * @brief Array of function pointers for relative point (single pixel) blending operations.
 * @see op_blend_pt_funcs for details on the array structure.
 * "Relative" blending typically means the alpha of the source is modulated
 * by the alpha of the destination.
 */
RGBA_Gfx_Pt_Func  op_blend_rel_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the relative blend operation functions.
 *
 * Sets up the op_blend_rel_span_funcs and op_blend_rel_pt_funcs tables with
 * appropriate function pointers based on CPU capabilities (MMX, SSE3, NEON)
 * and generic C implementations for relative blending.
 */
static void op_blend_rel_init(void);
/**
 * @brief Shuts down the relative blend operation module.
 * @note Currently, this function is a no-op.
 */
static void op_blend_rel_shutdown(void);

/**
 * @brief Retrieves a relative span blending function for pixel-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate relative span blending routine.
 */
static RGBA_Gfx_Func op_blend_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a relative span blending function for color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate relative span blending routine.
 */
static RGBA_Gfx_Func op_blend_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a relative span blending function for pixel-and-color-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse.
 * @param col The color multiplier as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate relative span blending routine.
 */
static RGBA_Gfx_Func op_blend_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a relative span blending function for mask-and-color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate relative span blending routine.
 */
static RGBA_Gfx_Func op_blend_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a relative span blending function for pixel-and-mask-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (not used by this compositor to select function).
 * @return A function pointer to the appropriate relative span blending routine.
 */
static RGBA_Gfx_Func op_blend_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a relative point blending function for pixel-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate relative point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a relative point blending function for color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate relative point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a relative point blending function for pixel-and-color-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param col The color multiplier as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate relative point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a relative point blending function for mask-and-color-to-pixel operations.
 * @param col The source color as a DATA32 (ARGB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate relative point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a relative point blending function for pixel-and-mask-to-pixel operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate relative point blending routine.
 */
static RGBA_Gfx_Pt_Func op_blend_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for relative blend operations.
 *
 * This structure defines the "blend_rel" compositor, providing function pointers
 * for initialization, shutdown, and various relative span/point blending operations.
 */
static RGBA_Gfx_Compositor  _composite_blend_rel = { "blend_rel",
 op_blend_rel_init, op_blend_rel_shutdown,
 op_blend_rel_pixel_span_get, op_blend_rel_color_span_get,
 op_blend_rel_pixel_color_span_get, op_blend_rel_mask_color_span_get,
 op_blend_rel_pixel_mask_span_get,
 op_blend_rel_pixel_pt_get, op_blend_rel_color_pt_get,
 op_blend_rel_pixel_color_pt_get, op_blend_rel_mask_color_pt_get,
 op_blend_rel_pixel_mask_pt_get
 };

/**
 * @brief Gets the relative blend compositor.
 * @return A pointer to the _composite_blend_rel structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_blend_rel_get(void)
{
   return &(_composite_blend_rel);
}


# include "./evas_op_blend/op_blend_pixel_.c"
# include "./evas_op_blend/op_blend_color_.c"
# include "./evas_op_blend/op_blend_pixel_color_.c"
# include "./evas_op_blend/op_blend_pixel_mask_.c"
# include "./evas_op_blend/op_blend_mask_color_.c"
//# include "./evas_op_blend/op_blend_pixel_mask_color_.c"

# include "./evas_op_blend/op_blend_pixel_i386.c"
# include "./evas_op_blend/op_blend_color_i386.c"
# include "./evas_op_blend/op_blend_pixel_color_i386.c"
# include "./evas_op_blend/op_blend_pixel_mask_i386.c"
# include "./evas_op_blend/op_blend_mask_color_i386.c"
//# include "./evas_op_blend/op_blend_pixel_mask_color_i386.c"

# include "./evas_op_blend/op_blend_pixel_neon.c"
# include "./evas_op_blend/op_blend_color_neon.c"
# include "./evas_op_blend/op_blend_pixel_color_neon.c"
# include "./evas_op_blend/op_blend_pixel_mask_neon.c"
# include "./evas_op_blend/op_blend_mask_color_neon.c"
//# include "./evas_op_blend/op_blend_pixel_mask_color_neon.c"

#ifdef BUILD_SSE3
void evas_common_op_blend_init_sse3(void);
#endif

/**
 * @brief Initializes SSE3 specific blend functions if available.
 * @note This function is conditionally compiled with BUILD_SSE3.
 */
#ifdef BUILD_SSE3
void evas_common_op_blend_init_sse3(void);
#endif

static void
op_blend_init(void)
{
   memset(op_blend_span_funcs, 0, sizeof(op_blend_span_funcs));
   memset(op_blend_pt_funcs, 0, sizeof(op_blend_pt_funcs));
#ifdef BUILD_SSE3
   if (evas_common_cpu_has_feature(CPU_FEATURE_SSE3))
     evas_common_op_blend_init_sse3();
#endif
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_blend_pixel_span_funcs_mmx();
        init_blend_pixel_color_span_funcs_mmx();
        init_blend_pixel_mask_span_funcs_mmx(); // FIXME
        init_blend_color_span_funcs_mmx();
        init_blend_mask_color_span_funcs_mmx();

        init_blend_pixel_pt_funcs_mmx();
        init_blend_pixel_color_pt_funcs_mmx();
        init_blend_pixel_mask_pt_funcs_mmx();
        init_blend_color_pt_funcs_mmx();
        init_blend_mask_color_pt_funcs_mmx();
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
        init_blend_pixel_span_funcs_neon();
        init_blend_pixel_color_span_funcs_neon();
        init_blend_pixel_mask_span_funcs_neon(); // FIXME
        init_blend_color_span_funcs_neon();
        init_blend_mask_color_span_funcs_neon();

        init_blend_pixel_pt_funcs_neon();
        init_blend_pixel_color_pt_funcs_neon();
        init_blend_pixel_mask_pt_funcs_neon();
        init_blend_color_pt_funcs_neon();
        init_blend_mask_color_pt_funcs_neon();
     }
#endif
   init_blend_pixel_span_funcs_c();
   init_blend_pixel_color_span_funcs_c();
   init_blend_pixel_mask_span_funcs_c();
   init_blend_color_span_funcs_c();
   init_blend_mask_color_span_funcs_c();

   init_blend_pixel_pt_funcs_c();
   init_blend_pixel_color_pt_funcs_c();
   init_blend_pixel_mask_pt_funcs_c();
   init_blend_color_pt_funcs_c();
   init_blend_mask_color_pt_funcs_c();
}

static void
op_blend_shutdown(void)
{
}

/**
 * @brief Selects the appropriate span blending function based on CPU features.
 *
 * This function attempts to find an optimized version (SSE3, MMX, NEON)
 * of the blending function. If none is found, it falls back to the generic C
 * implementation.
 *
 * @param s Source pixel property index (SP_* enum).
 * @param m Source mask property index (SM_* enum).
 * @param c Source color property index (SC_* enum).
 * @param d Destination pixel property index (DP_* enum).
 * @return A function pointer to the selected span blending routine.
 */
static RGBA_Gfx_Func
blend_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_SSE3
   if (evas_common_cpu_has_feature(CPU_FEATURE_SSE3))
      {
         cpu = CPU_SSE3;
         func = op_blend_span_funcs[s][m][c][d][cpu];
         if(func) return func;
      }
#endif
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_blend_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
	cpu = CPU_NEON;
	func = op_blend_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_blend_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_blend_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
        if (src_sparse_alpha)
	    s = SP_AS;
     }
   if (dst_alpha)
	d = DP;
   return blend_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
        if (src_sparse_alpha)
	    s = SP_AS;
     }
   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
        if (src_sparse_alpha)
	    s = SP_AS;
     }
   if (dst_alpha)
	d = DP;
   return blend_gfx_span_func_cpu(s, m, c, d);
}


/**
 * @brief Selects the appropriate point blending function based on CPU features.
 *
 * This function attempts to find an optimized version (SSE3, MMX, NEON)
 * of the point blending function. If none is found, it falls back to the generic C
 * implementation.
 *
 * @param s Source pixel property index (SP_* enum).
 * @param m Source mask property index (SM_* enum).
 * @param c Source color property index (SC_* enum).
 * @param d Destination pixel property index (DP_* enum).
 * @return A function pointer to the selected point blending routine.
 */
static RGBA_Gfx_Pt_Func
blend_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_SSE3
   if(evas_common_cpu_has_feature(CPU_FEATURE_SSE3))
      {
         cpu = CPU_SSE3;
         func = op_blend_pt_funcs[s][m][c][d][cpu];
         if(func) return func;
      }
#endif
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_blend_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
	cpu = CPU_NEON;
	func = op_blend_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_blend_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_blend_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return blend_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return blend_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Initializes SSE3 specific relative blend functions if available.
 * @note This function is conditionally compiled with BUILD_SSE3.
 */
void evas_common_op_blend_rel_init_sse3(void);

static void
op_blend_rel_init(void)
{
   memset(op_blend_rel_span_funcs, 0, sizeof(op_blend_rel_span_funcs));
   memset(op_blend_rel_pt_funcs, 0, sizeof(op_blend_rel_pt_funcs));
#ifdef BUILD_SSE3
   evas_common_op_blend_rel_init_sse3();
#endif
#ifdef BUILD_MMX
   init_blend_rel_pixel_span_funcs_mmx();
   init_blend_rel_pixel_color_span_funcs_mmx();
   init_blend_rel_pixel_mask_span_funcs_mmx();
   init_blend_rel_color_span_funcs_mmx();
   init_blend_rel_mask_color_span_funcs_mmx();

   init_blend_rel_pixel_pt_funcs_mmx();
   init_blend_rel_pixel_color_pt_funcs_mmx();
   init_blend_rel_pixel_mask_pt_funcs_mmx();
   init_blend_rel_color_pt_funcs_mmx();
   init_blend_rel_mask_color_pt_funcs_mmx();
#endif
#ifdef BUILD_NEON
   init_blend_rel_pixel_span_funcs_neon();
   init_blend_rel_pixel_color_span_funcs_neon();
   init_blend_rel_pixel_mask_span_funcs_neon();
   init_blend_rel_color_span_funcs_neon();
   init_blend_rel_mask_color_span_funcs_neon();

   init_blend_rel_pixel_pt_funcs_neon();
   init_blend_rel_pixel_color_pt_funcs_neon();
   init_blend_rel_pixel_mask_pt_funcs_neon();
   init_blend_rel_color_pt_funcs_neon();
   init_blend_rel_mask_color_pt_funcs_neon();
#endif
   init_blend_rel_pixel_span_funcs_c();
   init_blend_rel_pixel_color_span_funcs_c();
   init_blend_rel_pixel_mask_span_funcs_c();
   init_blend_rel_color_span_funcs_c();
   init_blend_rel_mask_color_span_funcs_c();

   init_blend_rel_pixel_pt_funcs_c();
   init_blend_rel_pixel_color_pt_funcs_c();
   init_blend_rel_pixel_mask_pt_funcs_c();
   init_blend_rel_color_pt_funcs_c();
   init_blend_rel_mask_color_pt_funcs_c();
}

static void
op_blend_rel_shutdown(void)
{
}

/**
 * @brief Selects the appropriate relative span blending function based on CPU features.
 *
 * This function attempts to find an optimized version (SSE3, MMX, NEON)
 * of the relative blending function. If none is found, it falls back to the generic C
 * implementation.
 *
 * @param s Source pixel property index (SP_* enum).
 * @param m Source mask property index (SM_* enum).
 * @param c Source color property index (SC_* enum).
 * @param d Destination pixel property index (DP_* enum).
 * @return A function pointer to the selected relative span blending routine.
 */
static RGBA_Gfx_Func
blend_rel_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_SSE3
   if (evas_common_cpu_has_feature(CPU_FEATURE_SSE3))
      {
         cpu = CPU_SSE3;
         func = op_blend_rel_span_funcs[s][m][c][d][cpu];
         if(func) return func;
      }
#endif
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_blend_rel_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
	cpu = CPU_NEON;
	func = op_blend_rel_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_blend_rel_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_blend_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
        if (src_sparse_alpha)
	    s = SP_AS;
     }
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_blend_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
        if (src_sparse_alpha)
	    s = SP_AS;
     }
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate relative point blending function based on CPU features.
 *
 * This function attempts to find an optimized version (SSE3, MMX, NEON)
 * of the relative point blending function. If none is found, it falls back to the generic C
 * implementation.
 *
 * @param s Source pixel property index (SP_* enum).
 * @param m Source mask property index (SM_* enum).
 * @param c Source color property index (SC_* enum).
 * @param d Destination pixel property index (DP_* enum).
 * @return A function pointer to the selected relative point blending routine.
 */
static RGBA_Gfx_Pt_Func
blend_rel_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_SSE3
   if (evas_common_cpu_has_feature(CPU_FEATURE_SSE3))
      {
         cpu = CPU_SSE3;
         func = op_blend_rel_pt_funcs[s][m][c][d][cpu];
         if(func) return func;
      }
#endif
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_blend_rel_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
	cpu = CPU_NEON;
	func = op_blend_rel_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_blend_rel_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_blend_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_blend_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return blend_rel_gfx_pt_func_cpu(s, m, c, d);
}
