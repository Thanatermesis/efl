#include "evas_common_private.h"
#include "evas_blend_private.h"

/**
 * @file evas_op_copy_main_.c
 * @brief This file contains the main implementation for the "copy" rendering operation in Evas.
 *
 * It defines functions and structures for handling pixel copying operations,
 * including different optimizations for CPU features like MMX and NEON.
 * It supports both standard copy and "copy_rel" (relative copy) operations.
 */

/**
 * @brief Array of function pointers for span-based copy operations.
 *
 * This 5-dimensional array stores pointers to functions that perform copy
 * operations on a span (horizontal line) of pixels. The dimensions
 * correspond to:
 * - Source pixel properties (SP_LAST)
 * - Source mask properties (SM_LAST)
 * - Source color properties (SC_LAST)
 * - Destination pixel properties (DP_LAST)
 * - CPU-specific optimizations (CPU_LAST)
 *
 * Example: `op_copy_span_funcs[SP_SOLID][SM_NONE][SC_NONE][DP_SOLID][CPU_C]`
 * would point to a C-based function for copying solid source pixels to solid
 * destination pixels without any masking or color modulation.
 */
static RGBA_Gfx_Func     op_copy_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Array of function pointers for point-based copy operations.
 *
 * Similar to `op_copy_span_funcs`, but for operations on individual pixels (points).
 * The dimensions are the same, representing different combinations of source,
 * mask, color, destination, and CPU properties.
 *
 * Example: `op_copy_pt_funcs[SP_ALPHA][SM_NONE][SC_NONE][DP_ALPHA][CPU_NEON]`
 * would point to a NEON-optimized function for copying a source pixel with alpha
 * to a destination pixel with alpha.
 */
static RGBA_Gfx_Pt_Func  op_copy_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the function pointers for copy operations.
 *
 * This function populates the `op_copy_span_funcs` and `op_copy_pt_funcs`
 * arrays with appropriate function pointers based on available CPU features
 * (MMX, NEON) and fallback C implementations.
 */
static void op_copy_init(void);
/**
 * @brief Shuts down the copy operations module.
 *
 * Currently, this function is a no-op but is present for API consistency.
 */
static void op_copy_shutdown(void);

/**
 * @brief Retrieves a span copy function for pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source has sparse alpha (not all pixels have alpha).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (currently unused in selection logic).
 * @return A function pointer to the appropriate span copy function.
 */
static RGBA_Gfx_Func op_copy_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span copy function for color data.
 * @param col The color to use for the operation (e.g., 0xAARRGGBB). Alpha component is checked.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (currently unused in selection logic).
 * @return A function pointer to the appropriate span copy function.
 */
static RGBA_Gfx_Func op_copy_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span copy function for pixel data modulated by a color.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source has sparse alpha.
 * @param col The color to modulate with (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (currently unused in selection logic).
 * @return A function pointer to the appropriate span copy function.
 */
static RGBA_Gfx_Func op_copy_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span copy function for masked color data.
 * @param col The color to use (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused in selection logic).
 * @param pixels The number of pixels in the span (currently unused in selection logic).
 * @return A function pointer to the appropriate span copy function.
 */
static RGBA_Gfx_Func op_copy_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span copy function for masked pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source has sparse alpha.
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused in selection logic).
 * @param pixels The number of pixels in the span (currently unused in selection logic).
 * @return A function pointer to the appropriate span copy function.
 */
static RGBA_Gfx_Func op_copy_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point copy function for pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point copy function.
 */
static RGBA_Gfx_Pt_Func op_copy_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point copy function for color data.
 * @param col The color to use (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point copy function.
 */
static RGBA_Gfx_Pt_Func op_copy_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point copy function for pixel data modulated by a color.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param col The color to modulate with (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point copy function.
 */
static RGBA_Gfx_Pt_Func op_copy_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point copy function for masked color data.
 * @param col The color to use (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused in selection logic).
 * @return A function pointer to the appropriate point copy function.
 */
static RGBA_Gfx_Pt_Func op_copy_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point copy function for masked pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused in selection logic).
 * @return A function pointer to the appropriate point copy function.
 */
static RGBA_Gfx_Pt_Func op_copy_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for standard "copy" operations.
 *
 * This structure groups all the necessary functions and metadata for the
 * "copy" rendering operation. It is registered with the Evas core.
 */
static RGBA_Gfx_Compositor  _composite_copy = { "copy",
 op_copy_init, op_copy_shutdown,
 op_copy_pixel_span_get, op_copy_color_span_get,
 op_copy_pixel_color_span_get, op_copy_mask_color_span_get,
 op_copy_pixel_mask_span_get,
 op_copy_pixel_pt_get, op_copy_color_pt_get,
 op_copy_pixel_color_pt_get, op_copy_mask_color_pt_get,
 op_copy_pixel_mask_pt_get
 };

/**
 * @brief Retrieves the compositor for standard "copy" operations.
 * @return A pointer to the `_composite_copy` structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_copy_get(void)
{
   return &(_composite_copy);
}

/**
 * @brief Array of function pointers for span-based relative copy operations.
 *
 * Similar to `op_copy_span_funcs`, but for "copy_rel" operations, which
 * typically involve relative alpha blending (output_alpha = src_alpha).
 * The dimensions are:
 * - Source pixel properties (SP_LAST)
 * - Source mask properties (SM_LAST)
 * - Source color properties (SC_LAST)
 * - Destination pixel properties (DP_LAST)
 * - CPU-specific optimizations (CPU_LAST)
 */
static RGBA_Gfx_Func     op_copy_rel_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
/**
 * @brief Array of function pointers for point-based relative copy operations.
 *
 * Similar to `op_copy_pt_funcs`, but for "copy_rel" operations.
 */
static RGBA_Gfx_Pt_Func  op_copy_rel_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the function pointers for "copy_rel" operations.
 *
 * This function populates `op_copy_rel_span_funcs` and `op_copy_rel_pt_funcs`
 * based on CPU features and C fallbacks for relative copy operations.
 */
static void op_copy_rel_init(void);
/**
 * @brief Shuts down the "copy_rel" operations module.
 *
 * Currently, this function is a no-op.
 */
static void op_copy_rel_shutdown(void);

/**
 * @brief Retrieves a span "copy_rel" function for pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param src_sparse_alpha EINA_UNUSED: Not used for "copy_rel" pixel span.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels The number of pixels (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" span function.
 */
static RGBA_Gfx_Func op_copy_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span "copy_rel" function for color data.
 * @param col The color to use.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels The number of pixels (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" span function.
 */
static RGBA_Gfx_Func op_copy_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span "copy_rel" function for pixel data modulated by color.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha.
 * @param col The color to modulate with.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels The number of pixels (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" span function.
 */
static RGBA_Gfx_Func op_copy_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span "copy_rel" function for masked color data.
 * @param col The color to use.
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused in selection).
 * @param pixels The number of pixels (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" span function.
 */
static RGBA_Gfx_Func op_copy_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span "copy_rel" function for masked pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param src_sparse_alpha EINA_UNUSED: Not used for "copy_rel" pixel mask span.
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused in selection).
 * @param pixels The number of pixels (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" span function.
 */
static RGBA_Gfx_Func op_copy_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point "copy_rel" function for pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @return A function pointer to the appropriate "copy_rel" point function.
 */
static RGBA_Gfx_Pt_Func op_copy_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/* XXX: doesn't exist
static RGBA_Gfx_Pt_Func op_copy_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
 */
/**
 * @brief Retrieves a point "copy_rel" function for pixel data modulated by color.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param col The color to modulate with.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @return A function pointer to the appropriate "copy_rel" point function.
 */
static RGBA_Gfx_Pt_Func op_copy_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point "copy_rel" function for masked color data.
 * @param col The color to use.
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" point function.
 */
static RGBA_Gfx_Pt_Func op_copy_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point "copy_rel" function for masked pixel data.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused in selection).
 * @return A function pointer to the appropriate "copy_rel" point function.
 */
static RGBA_Gfx_Pt_Func op_copy_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for "copy_rel" (relative copy) operations.
 *
 * This structure groups all the necessary functions and metadata for the
 * "copy_rel" rendering operation.
 */
static RGBA_Gfx_Compositor  _composite_copy_rel = { "copy_rel",
 op_copy_rel_init, op_copy_rel_shutdown,
 op_copy_rel_pixel_span_get, op_copy_rel_color_span_get,
 op_copy_rel_pixel_color_span_get, op_copy_rel_mask_color_span_get,
 op_copy_rel_pixel_mask_span_get,
 op_copy_rel_pixel_pt_get, op_copy_color_pt_get,
 op_copy_rel_pixel_color_pt_get, op_copy_rel_mask_color_pt_get,
 op_copy_rel_pixel_mask_pt_get
 };

/**
 * @brief Retrieves the compositor for "copy_rel" operations.
 * @return A pointer to the `_composite_copy_rel` structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_copy_rel_get(void)
{
   return &(_composite_copy_rel);
}


# include "./evas_op_copy/op_copy_pixel_.c"
# include "./evas_op_copy/op_copy_color_.c"
# include "./evas_op_copy/op_copy_pixel_color_.c"
# include "./evas_op_copy/op_copy_pixel_mask_.c"
# include "./evas_op_copy/op_copy_mask_color_.c"
//# include "./evas_op_copy/op_copy_pixel_mask_color_.c"

# include "./evas_op_copy/op_copy_pixel_i386.c"
# include "./evas_op_copy/op_copy_color_i386.c"
# include "./evas_op_copy/op_copy_pixel_color_i386.c"
# include "./evas_op_copy/op_copy_pixel_mask_i386.c"
# include "./evas_op_copy/op_copy_mask_color_i386.c"
//# include "./evas_op_copy/op_copy_pixel_mask_color_i386.c"

# include "./evas_op_copy/op_copy_pixel_neon.c"
# include "./evas_op_copy/op_copy_color_neon.c"
# include "./evas_op_copy/op_copy_pixel_color_neon.c"
# include "./evas_op_copy/op_copy_pixel_mask_neon.c"
# include "./evas_op_copy/op_copy_mask_color_neon.c"
//# include "./evas_op_copy/op_copy_pixel_mask_color_neon.c"


static void
op_copy_init(void)
{
   memset(op_copy_span_funcs, 0, sizeof(op_copy_span_funcs));
   memset(op_copy_pt_funcs, 0, sizeof(op_copy_pt_funcs));
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_copy_pixel_span_funcs_mmx();
        init_copy_pixel_color_span_funcs_mmx();
        init_copy_pixel_mask_span_funcs_mmx();
        init_copy_color_span_funcs_mmx();
        init_copy_mask_color_span_funcs_mmx();

        init_copy_pixel_pt_funcs_mmx();
        init_copy_pixel_color_pt_funcs_mmx();
        init_copy_pixel_mask_pt_funcs_mmx();
        init_copy_color_pt_funcs_mmx();
        init_copy_mask_color_pt_funcs_mmx();
     }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
     {
        init_copy_pixel_span_funcs_neon();
        init_copy_pixel_color_span_funcs_neon();
        init_copy_pixel_mask_span_funcs_neon();
        init_copy_color_span_funcs_neon();
        init_copy_mask_color_span_funcs_neon();

        init_copy_pixel_pt_funcs_neon();
        init_copy_pixel_color_pt_funcs_neon();
        init_copy_pixel_mask_pt_funcs_neon();
        init_copy_color_pt_funcs_neon();
        init_copy_mask_color_pt_funcs_neon();
     }
#endif
   init_copy_pixel_span_funcs_c();
   init_copy_pixel_color_span_funcs_c();
   init_copy_pixel_mask_span_funcs_c();
   init_copy_color_span_funcs_c();
   init_copy_mask_color_span_funcs_c();

   init_copy_pixel_pt_funcs_c();
   init_copy_pixel_color_pt_funcs_c();
   init_copy_pixel_mask_pt_funcs_c();
   init_copy_color_pt_funcs_c();
   init_copy_mask_color_pt_funcs_c();
}

static void
op_copy_shutdown(void)
{
}

/**
 * @brief Selects the appropriate CPU-optimized span copy function.
 *
 * This helper function attempts to find an MMX or NEON optimized function
 * first, falling back to a generic C implementation if no optimized version
 * is available or if the CPU doesn't support the features.
 *
 * @param s Source pixel property index (e.g., SP, SP_AN).
 * @param m Source mask property index (e.g., SM_N, SM_AS).
 * @param c Source color property index (e.g., SC_N, SC, SC_AA, SC_AN).
 * @param d Destination pixel property index (e.g., DP, DP_AN).
 * @return A function pointer to the selected span copy function.
 */
static RGBA_Gfx_Func
copy_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_copy_span_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
    {
      cpu = CPU_NEON;
      func = op_copy_span_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_copy_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_copy_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return copy_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return copy_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return copy_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate CPU-optimized point copy function.
 *
 * Similar to `copy_gfx_span_func_cpu`, but for point (single pixel) operations.
 * It prioritizes MMX/NEON versions and falls back to C.
 *
 * @param s Source pixel property index.
 * @param m Source mask property index.
 * @param c Source color property index.
 * @param d Destination pixel property index.
 * @return A function pointer to the selected point copy function.
 */
static RGBA_Gfx_Pt_Func
copy_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_copy_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
    {
      cpu = CPU_NEON;
      func = op_copy_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_copy_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_copy_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return copy_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
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
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return copy_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return copy_gfx_pt_func_cpu(s, m, c, d);
}


static void
op_copy_rel_init(void)
{
   memset(op_copy_rel_span_funcs, 0, sizeof(op_copy_rel_span_funcs));
   memset(op_copy_rel_pt_funcs, 0, sizeof(op_copy_rel_pt_funcs));
#ifdef BUILD_MMX
   init_copy_rel_pixel_span_funcs_mmx();
   init_copy_rel_pixel_color_span_funcs_mmx();
   init_copy_rel_pixel_mask_span_funcs_mmx();
   init_copy_rel_color_span_funcs_mmx();
   init_copy_rel_mask_color_span_funcs_mmx();

   init_copy_rel_pixel_pt_funcs_mmx();
   init_copy_rel_pixel_color_pt_funcs_mmx();
   init_copy_rel_pixel_mask_pt_funcs_mmx();
   init_copy_rel_color_pt_funcs_mmx();
   init_copy_rel_mask_color_pt_funcs_mmx();
#endif
#ifdef BUILD_NEON
   init_copy_rel_pixel_span_funcs_neon();
   init_copy_rel_pixel_color_span_funcs_neon();
   init_copy_rel_pixel_mask_span_funcs_neon();
   init_copy_rel_color_span_funcs_neon();
   init_copy_rel_mask_color_span_funcs_neon();

   init_copy_rel_pixel_pt_funcs_neon();
   init_copy_rel_pixel_color_pt_funcs_neon();
   init_copy_rel_pixel_mask_pt_funcs_neon();
   init_copy_rel_color_pt_funcs_neon();
   init_copy_rel_mask_color_pt_funcs_neon();
#endif
   init_copy_rel_pixel_span_funcs_c();
   init_copy_rel_pixel_color_span_funcs_c();
   init_copy_rel_pixel_mask_span_funcs_c();
   init_copy_rel_color_span_funcs_c();
   init_copy_rel_mask_color_span_funcs_c();

   init_copy_rel_pixel_pt_funcs_c();
   init_copy_rel_pixel_color_pt_funcs_c();
   init_copy_rel_pixel_mask_pt_funcs_c();
   init_copy_rel_color_pt_funcs_c();
   init_copy_rel_mask_color_pt_funcs_c();
}

static void
op_copy_rel_shutdown(void)
{
}

/**
 * @brief Selects the appropriate CPU-optimized span "copy_rel" function.
 *
 * This helper function is for "copy_rel" operations. It attempts to find an
 * MMX or NEON optimized function first, falling back to a generic C
 * implementation.
 *
 * @param s Source pixel property index.
 * @param m Source mask property index.
 * @param c Source color property index.
 * @param d Destination pixel property index.
 * @return A function pointer to the selected "copy_rel" span function.
 */
static RGBA_Gfx_Func
copy_rel_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_copy_rel_span_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
    {
      cpu = CPU_NEON;
      func = op_copy_rel_span_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_copy_rel_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_copy_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	c = SC;
     }
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return copy_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_copy_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return copy_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate CPU-optimized point "copy_rel" function.
 *
 * Similar to `copy_rel_gfx_span_func_cpu`, but for point (single pixel)
 * "copy_rel" operations. It prioritizes MMX/NEON versions and falls back to C.
 *
 * @param s Source pixel property index.
 * @param m Source mask property index.
 * @param c Source color property index.
 * @param d Destination pixel property index.
 * @return A function pointer to the selected "copy_rel" point function.
 */
static RGBA_Gfx_Pt_Func
copy_rel_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_copy_rel_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
#ifdef BUILD_NEON
   if (evas_common_cpu_has_feature(CPU_FEATURE_NEON))
    {
      cpu = CPU_NEON;
      func = op_copy_rel_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_copy_rel_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_copy_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_pt_func_cpu(s, m, c, d);
}

/* XXX: not used
static RGBA_Gfx_Pt_Func
op_copy_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
     {
	if (dst)
	   dst->cache_entry.flags.alpha = 1;
	c = SC;
     }
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_pt_func_cpu(s, m, c, d);
}
*/

static RGBA_Gfx_Pt_Func
op_copy_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
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
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return copy_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == ((col >> 24) * 0x01010101))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return copy_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_copy_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return copy_rel_gfx_pt_func_cpu(s, m, c, d);
}
