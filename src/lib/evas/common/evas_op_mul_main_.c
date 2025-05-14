#include "evas_common_private.h"

/**
 * @brief Array of function pointers for span operations using multiplication.
 *
 * This 5-dimensional array stores pointers to functions that perform
 * multiplication-based graphics operations on spans of pixels. The dimensions
 * represent:
 * - Source pixel properties (e.g., alpha, no alpha)
 * - Source mask properties (e.g., alpha, no alpha)
 * - Source color properties (e.g., alpha, no alpha, solid)
 * - Destination pixel properties (e.g., alpha, no alpha)
 * - CPU-specific implementations (e.g., C, MMX)
 *
 * Example: op_mul_span_funcs[SP_AN][SM_N][SC_N][DP_AN][CPU_C] would point to a C
 * function for a span operation with no source alpha, no source mask, no source
 * color alpha, and no destination alpha.
 */
static RGBA_Gfx_Func     op_mul_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Array of function pointers for point operations using multiplication.
 *
 * This 5-dimensional array stores pointers to functions that perform
 * multiplication-based graphics operations on single pixels (points). The
 * dimensions are similar to op_mul_span_funcs:
 * - Source pixel properties
 * - Source mask properties
 * - Source color properties
 * - Destination pixel properties
 * - CPU-specific implementations
 *
 * Example: op_mul_pt_funcs[SP][SM_AS][SC_AA][DP][CPU_MMX] would point to an MMX
 * function for a point operation with source alpha, alpha source mask,
 * alpha source color, and destination alpha.
 */
static RGBA_Gfx_Pt_Func  op_mul_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the multiplication operation functions.
 *
 * This function populates the op_mul_span_funcs and op_mul_pt_funcs arrays
 * with appropriate function pointers for different CPU capabilities (C, MMX).
 */
static void op_mul_init(void);
/**
 * @brief Shuts down the multiplication operation functions.
 *
 * Currently, this function is a placeholder and does not perform any actions.
 */
static void op_mul_shutdown(void);

/**
 * @brief Retrieves a span processing function for pixel data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse (not used by this op).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (not used by this op to select function).
 * @return A function pointer to the appropriate span processing function.
 */
static RGBA_Gfx_Func op_mul_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for color data.
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (not used by this op to select function).
 * @return A function pointer to the appropriate span processing function.
 */
static RGBA_Gfx_Func op_mul_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel and color data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse (not used by this op).
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (not used by this op to select function).
 * @return A function pointer to the appropriate span processing function.
 */
static RGBA_Gfx_Func op_mul_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for mask and color data.
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise (not used by this op).
 * @param pixels Number of pixels in the span (not used by this op to select function).
 * @return A function pointer to the appropriate span processing function.
 */
static RGBA_Gfx_Func op_mul_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel and mask data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source alpha is sparse (not used by this op).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise (not used by this op).
 * @param pixels Number of pixels in the span (not used by this op to select function).
 * @return A function pointer to the appropriate span processing function.
 */
static RGBA_Gfx_Func op_mul_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point processing function for pixel data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point processing function.
 */
static RGBA_Gfx_Pt_Func op_mul_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for color data.
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point processing function.
 */
static RGBA_Gfx_Pt_Func op_mul_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel and color data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point processing function.
 */
static RGBA_Gfx_Pt_Func op_mul_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for mask and color data.
 * @param col The color to use (DATA32 format, ARGB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise (not used by this op).
 * @return A function pointer to the appropriate point processing function.
 */
static RGBA_Gfx_Pt_Func op_mul_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel and mask data.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise (not used by this op).
 * @return A function pointer to the appropriate point processing function.
 */
static RGBA_Gfx_Pt_Func op_mul_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Defines the compositor for multiplication operations.
 *
 * This structure holds the name of the compositor and pointers to various
 * functions that handle different types of multiplication operations (span, point,
 * color, pixel, mask).
 */
static RGBA_Gfx_Compositor  _composite_mul = { "mul",
 op_mul_init, op_mul_shutdown,
 op_mul_pixel_span_get, op_mul_color_span_get,
 op_mul_pixel_color_span_get, op_mul_mask_color_span_get,
 op_mul_pixel_mask_span_get,
 op_mul_pixel_pt_get, op_mul_color_pt_get,
 op_mul_pixel_color_pt_get, op_mul_mask_color_pt_get,
 op_mul_pixel_mask_pt_get
 };

/**
 * @brief Gets the multiplication graphics compositor.
 *
 * This function returns a pointer to the _composite_mul structure, which
 * contains all the necessary functions for performing multiplication-based
 * graphics operations.
 *
 * @return A pointer to the RGBA_Gfx_Compositor structure for "mul" operations.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_mul_get(void)
{
   return &(_composite_mul);
}


# include "./evas_op_mul/op_mul_pixel_.c"
# include "./evas_op_mul/op_mul_color_.c"
# include "./evas_op_mul/op_mul_pixel_color_.c"
# include "./evas_op_mul/op_mul_pixel_mask_.c"
# include "./evas_op_mul/op_mul_mask_color_.c"
//# include "./evas_op_mul/op_mul_pixel_mask_color_.c"

# include "./evas_op_mul/op_mul_pixel_i386.c"
# include "./evas_op_mul/op_mul_color_i386.c"
# include "./evas_op_mul/op_mul_pixel_color_i386.c"
# include "./evas_op_mul/op_mul_pixel_mask_i386.c"
# include "./evas_op_mul/op_mul_mask_color_i386.c"
// # include "./evas_op_mul/op_mul_pixel_mask_color_i386.c"

static void
op_mul_init(void)
{
   memset(op_mul_span_funcs, 0, sizeof(op_mul_span_funcs));
   memset(op_mul_pt_funcs, 0, sizeof(op_mul_pt_funcs));
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_mul_pixel_span_funcs_mmx();
        init_mul_pixel_color_span_funcs_mmx();
        init_mul_pixel_mask_span_funcs_mmx();
        init_mul_color_span_funcs_mmx();
        init_mul_mask_color_span_funcs_mmx();

        init_mul_pixel_pt_funcs_mmx();
        init_mul_pixel_color_pt_funcs_mmx();
        init_mul_pixel_mask_pt_funcs_mmx();
        init_mul_color_pt_funcs_mmx();
        init_mul_mask_color_pt_funcs_mmx();
     }
#endif
   init_mul_pixel_span_funcs_c();
   init_mul_pixel_color_span_funcs_c();
   init_mul_pixel_mask_span_funcs_c();
   init_mul_color_span_funcs_c();
   init_mul_mask_color_span_funcs_c();

   init_mul_pixel_pt_funcs_c();
   init_mul_pixel_color_pt_funcs_c();
   init_mul_pixel_mask_pt_funcs_c();
   init_mul_color_pt_funcs_c();
   init_mul_mask_color_pt_funcs_c();
}

static void
op_mul_shutdown(void)
{
}

/**
 * @brief Selects the appropriate CPU-specific span function.
 *
 * This function attempts to find an MMX-optimized function first if available,
 * otherwise, it falls back to a C implementation.
 *
 * @param s Source pixel property index (SP_*).
 * @param m Source mask property index (SM_*).
 * @param c Source color property index (SC_*).
 * @param d Destination pixel property index (DP_*).
 * @return A function pointer to the selected span processing function.
 */
static RGBA_Gfx_Func
mul_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_mul_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_mul_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_mul_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return mul_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_mul_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return mul_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_mul_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return mul_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_mul_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return mul_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_mul_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return mul_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate CPU-specific point function.
 *
 * This function attempts to find an MMX-optimized function first if available,
 * otherwise, it falls back to a C implementation.
 *
 * @param s Source pixel property index (SP_*).
 * @param m Source mask property index (SM_*).
 * @param c Source color property index (SC_*).
 * @param d Destination pixel property index (DP_*).
 * @return A function pointer to the selected point processing function.
 */
static RGBA_Gfx_Pt_Func
mul_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_mul_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_mul_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_mul_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return mul_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_mul_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return mul_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_mul_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return mul_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_mul_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return mul_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_mul_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return mul_gfx_pt_func_cpu(s, m, c, d);
}
