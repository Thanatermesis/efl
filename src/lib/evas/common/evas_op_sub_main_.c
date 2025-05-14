#include "evas_common_private.h"

/**
 * @file evas_op_sub_main_.c
 * @brief This file contains the main implementation for subtraction Porter/Duff compositing operations.
 *
 * It defines and initializes function tables for various CPU-specific and generic
 * implementations of subtraction operations for spans (horizontal lines of pixels)
 * and individual points (pixels). It handles different source and destination
 * alpha properties, color values, and mask usage.
 *
 * Two main sets of operations are provided:
 * - 'sub': Standard subtraction (Dst = Dst - Src).
 * - 'sub_rel': Relative subtraction, potentially for specific use cases where
 *              the subtraction behavior might differ or be optimized differently.
 */

/**
 * @brief Array of function pointers for subtraction span operations.
 *
 * This 5-dimensional array stores pointers to functions that perform subtraction
 * operations on a span of pixels. The dimensions correspond to:
 * - SP (Source Pixel type): e.g., SP (alpha), SP_AN (no alpha)
 * - SM (Source Mask type): e.g., SM_N (no mask), SM_AS (alpha sparse mask)
 * - SC (Source Color type): e.g., SC_N (no color), SC (color with alpha), SC_AA (opaque color)
 * - DP (Destination Pixel type): e.g., DP (alpha), DP_AN (no alpha)
 * - CPU (CPU specific implementation): e.g., CPU_C (generic C), CPU_MMX (MMX optimized)
 *
 * Example: `op_sub_span_funcs[SP][SM_N][SC_N][DP][CPU_C]` would point to a C function
 * that subtracts a source image with alpha from a destination image with alpha,
 * without using a color or mask.
 */
static RGBA_Gfx_Func     op_sub_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Array of function pointers for subtraction point (pixel) operations.
 *
 * Similar to `op_sub_span_funcs`, but for operations on individual pixels.
 * The dimensions and their meanings are identical.
 *
 * Example: `op_sub_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_MMX]` would point to an MMX
 * optimized function that subtracts a source color (with alpha) from a destination
 * pixel (no alpha), where the source image itself has no alpha.
 */
static RGBA_Gfx_Pt_Func  op_sub_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/** @brief Initializes the subtraction operation functions. */
/** @brief Initializes the subtraction operation functions. */
static void op_sub_init(void);
/** @brief Shuts down and cleans up resources for subtraction operations. */
static void op_sub_shutdown(void);

/** @brief Retrieves a span processing function for pixel-to-pixel subtraction. */
static RGBA_Gfx_Func op_sub_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for color-to-pixel subtraction. */
static RGBA_Gfx_Func op_sub_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for pixel+color-to-pixel subtraction. */
static RGBA_Gfx_Func op_sub_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for mask+color-to-pixel subtraction. */
static RGBA_Gfx_Func op_sub_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for pixel+mask-to-pixel subtraction. */
static RGBA_Gfx_Func op_sub_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels);

/** @brief Retrieves a point processing function for pixel-to-pixel subtraction. */
static RGBA_Gfx_Pt_Func op_sub_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for color-to-pixel subtraction. */
static RGBA_Gfx_Pt_Func op_sub_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for pixel+color-to-pixel subtraction. */
static RGBA_Gfx_Pt_Func op_sub_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for mask+color-to-pixel subtraction. */
static RGBA_Gfx_Pt_Func op_sub_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for pixel+mask-to-pixel subtraction. */
static RGBA_Gfx_Pt_Func op_sub_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for standard subtraction operations.
 *
 * This structure groups all the necessary functions and metadata for the
 * "sub" compositing operation. It includes:
 * - name: "sub"
 * - init/shutdown function pointers.
 * - Pointers to functions that retrieve specific span/point operation
 *   handlers based on source/destination properties.
 */
static RGBA_Gfx_Compositor  _composite_sub = { "sub",
 op_sub_init, op_sub_shutdown,
 op_sub_pixel_span_get, op_sub_color_span_get,
 op_sub_pixel_color_span_get, op_sub_mask_color_span_get,
 op_sub_pixel_mask_span_get,
 op_sub_pixel_pt_get, op_sub_color_pt_get,
 op_sub_pixel_color_pt_get, op_sub_mask_color_pt_get,
 op_sub_pixel_mask_pt_get
 };

/**
 * @brief Retrieves the compositor for standard subtraction operations.
 * @return A pointer to the static `_composite_sub` structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_sub_get(void)
{
   return &(_composite_sub);
}

/**
 * @brief Array of function pointers for relative subtraction span operations.
 * @see op_sub_span_funcs for detailed structure.
 * This array serves the same purpose as `op_sub_span_funcs` but for "sub_rel" operations.
 */
static RGBA_Gfx_Func     op_sub_rel_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
/**
 * @brief Array of function pointers for relative subtraction point (pixel) operations.
 * @see op_sub_pt_funcs for detailed structure.
 * This array serves the same purpose as `op_sub_pt_funcs` but for "sub_rel" operations.
 */
static RGBA_Gfx_Pt_Func  op_sub_rel_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/** @brief Initializes the relative subtraction operation functions. */
static void op_sub_rel_init(void);
/** @brief Shuts down and cleans up resources for relative subtraction operations. */
static void op_sub_rel_shutdown(void);

/** @brief Retrieves a span processing function for pixel-to-pixel relative subtraction. */
static RGBA_Gfx_Func op_sub_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for color-to-pixel relative subtraction. */
static RGBA_Gfx_Func op_sub_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for pixel+color-to-pixel relative subtraction. */
static RGBA_Gfx_Func op_sub_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for mask+color-to-pixel relative subtraction. */
static RGBA_Gfx_Func op_sub_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/** @brief Retrieves a span processing function for pixel+mask-to-pixel relative subtraction. */
static RGBA_Gfx_Func op_sub_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/** @brief Retrieves a point processing function for pixel-to-pixel relative subtraction. */
static RGBA_Gfx_Pt_Func op_sub_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for color-to-pixel relative subtraction. */
static RGBA_Gfx_Pt_Func op_sub_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for pixel+color-to-pixel relative subtraction. */
static RGBA_Gfx_Pt_Func op_sub_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for mask+color-to-pixel relative subtraction. */
static RGBA_Gfx_Pt_Func op_sub_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/** @brief Retrieves a point processing function for pixel+mask-to-pixel relative subtraction. */
static RGBA_Gfx_Pt_Func op_sub_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for relative subtraction operations.
 *
 * This structure groups all the necessary functions and metadata for the
 * "sub_rel" compositing operation.
 * @see _composite_sub for more details on the structure.
 */
static RGBA_Gfx_Compositor  _composite_sub_rel = { "sub_rel",
 op_sub_rel_init, op_sub_rel_shutdown,
 op_sub_rel_pixel_span_get, op_sub_rel_color_span_get,
 op_sub_rel_pixel_color_span_get, op_sub_rel_mask_color_span_get,
 op_sub_rel_pixel_mask_span_get,
 op_sub_rel_pixel_pt_get, op_sub_rel_color_pt_get,
 op_sub_rel_pixel_color_pt_get, op_sub_rel_mask_color_pt_get,
 op_sub_rel_pixel_mask_pt_get
 };

/**
 * @brief Retrieves the compositor for relative subtraction operations.
 * @return A pointer to the static `_composite_sub_rel` structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_sub_rel_get(void)
{
   return &(_composite_sub_rel);
}


# include "./evas_op_sub/op_sub_pixel_.c"
# include "./evas_op_sub/op_sub_color_.c"
# include "./evas_op_sub/op_sub_pixel_color_.c"
# include "./evas_op_sub/op_sub_pixel_mask_.c"
# include "./evas_op_sub/op_sub_mask_color_.c"
//# include "./evas_op_sub/op_sub_pixel_mask_color_.c"

# include "./evas_op_sub/op_sub_pixel_i386.c"
# include "./evas_op_sub/op_sub_color_i386.c"
# include "./evas_op_sub/op_sub_pixel_color_i386.c"
# include "./evas_op_sub/op_sub_pixel_mask_i386.c"
# include "./evas_op_sub/op_sub_mask_color_i386.c"
//# include "./evas_op_sub/op_sub_pixel_mask_color_i386.c"

/**
 * @brief Initializes function tables for standard subtraction operations.
 *
 * This function populates the `op_sub_span_funcs` and `op_sub_pt_funcs`
 * arrays with pointers to appropriate C and MMX (if available) implementations
 * for various subtraction scenarios.
 */
static void
op_sub_init(void)
{
   memset(op_sub_span_funcs, 0, sizeof(op_sub_span_funcs));
   memset(op_sub_pt_funcs, 0, sizeof(op_sub_pt_funcs));
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_sub_pixel_span_funcs_mmx();
        init_sub_pixel_color_span_funcs_mmx();
        init_sub_pixel_mask_span_funcs_mmx();
        init_sub_color_span_funcs_mmx();
        init_sub_mask_color_span_funcs_mmx();

        init_sub_pixel_pt_funcs_mmx();
        init_sub_pixel_color_pt_funcs_mmx();
        init_sub_pixel_mask_pt_funcs_mmx();
        init_sub_color_pt_funcs_mmx();
        init_sub_mask_color_pt_funcs_mmx();
     }
#endif
   init_sub_pixel_span_funcs_c();
   init_sub_pixel_color_span_funcs_c();
   init_sub_rel_pixel_mask_span_funcs_c();
   init_sub_color_span_funcs_c();
   init_sub_mask_color_span_funcs_c();

   init_sub_pixel_pt_funcs_c();
   init_sub_pixel_color_pt_funcs_c();
   init_sub_rel_pixel_mask_pt_funcs_c();
   init_sub_color_pt_funcs_c();
   init_sub_mask_color_pt_funcs_c();
}

/**
 * @brief Placeholder for shutting down standard subtraction operations.
 *
 * Currently, this function is empty as there might not be specific resources
 * to release for the standard subtraction operations beyond what's handled
 * globally or by the included C/MMX modules.
 */
static void
op_sub_shutdown(void)
{
}

/**
 * @brief Selects the appropriate CPU-specific span function for standard subtraction.
 *
 * Checks for MMX capabilities and returns an MMX-optimized function if available
 * and registered. Otherwise, falls back to a generic C implementation.
 *
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return A function pointer (RGBA_Gfx_Func) to the selected span operation,
 *         or NULL if no suitable function is found (though typically a C fallback exists).
 */
static RGBA_Gfx_Func
sub_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_sub_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_sub_span_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @brief Gets a span function for pixel-to-pixel standard subtraction.
 *
 * Determines the source (s) and destination (d) pixel properties based on
 * `src_alpha` and `dst_alpha` flags, then retrieves the appropriate span
 * function using `sub_gfx_span_func_cpu`.
 *
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (currently unused).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (currently unused in this getter).
 * @return A function pointer (RGBA_Gfx_Func) for the operation.
 */
static RGBA_Gfx_Func
op_sub_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for color-to-pixel standard subtraction.
 *
 * Determines source color (c) and destination pixel (d) properties.
 * The source color type (SC_*) is determined by the alpha channel of `col`
 * and whether it's fully opaque white or black.
 *
 * @param col The source color (in ARGB32 format, e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (currently unused in this getter).
 * @return A function pointer (RGBA_Gfx_Func) for the operation.
 */
static RGBA_Gfx_Func
op_sub_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for pixel+color-to-pixel standard subtraction.
 *
 * Combines logic from `op_sub_pixel_span_get` (for source pixel properties)
 * and `op_sub_color_span_get` (for source color properties).
 *
 * @param src_alpha EINA_TRUE if source image has alpha.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (currently unused).
 * @param col The source color (ARGB32).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @param pixels The number of pixels in the span (currently unused in this getter).
 * @return A function pointer (RGBA_Gfx_Func) for the operation.
 */
static RGBA_Gfx_Func
op_sub_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for mask+color-to-pixel standard subtraction.
 *
 * Assumes a sparse alpha mask (m = SM_AS). Determines source color (c)
 * and destination pixel (d) properties.
 *
 * @param col The source color (ARGB32).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @param pixels The number of pixels in the span (currently unused in this getter).
 * @return A function pointer (RGBA_Gfx_Func) for the operation.
 */
static RGBA_Gfx_Func
op_sub_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for pixel+mask-to-pixel standard subtraction.
 *
 * Assumes a sparse alpha mask (m = SM_AS). Determines source pixel (s)
 * and destination pixel (d) properties.
 *
 * @param src_alpha EINA_TRUE if source image has alpha.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (currently unused).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @param pixels The number of pixels in the span (currently unused in this getter).
 * @return A function pointer (RGBA_Gfx_Func) for the operation.
 */
static RGBA_Gfx_Func
op_sub_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate CPU-specific point function for standard subtraction.
 *
 * Similar to `sub_gfx_span_func_cpu`, but for individual pixel (point) operations.
 * Checks for MMX capabilities and returns an MMX-optimized function if available
 * and registered. Otherwise, falls back to a generic C implementation.
 *
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return A function pointer (RGBA_Gfx_Pt_Func) to the selected point operation,
 *         or NULL if no suitable function is found.
 */
static RGBA_Gfx_Pt_Func
sub_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_sub_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_sub_pt_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @brief Gets a point function for pixel-to-pixel standard subtraction.
 *
 * Determines source (s) and destination (d) pixel properties based on
 * `src_alpha` and `dst_alpha` flags, then retrieves the appropriate point
 * function using `sub_gfx_pt_func_cpu`.
 *
 * @param src_alpha EINA_TRUE if source has alpha.
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @return A function pointer (RGBA_Gfx_Pt_Func) for the operation.
 */
static RGBA_Gfx_Pt_Func
op_sub_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for color-to-pixel standard subtraction.
 *
 * Determines source color (c) and destination pixel (d) properties.
 *
 * @param col The source color (ARGB32).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @return A function pointer (RGBA_Gfx_Pt_Func) for the operation.
 */
static RGBA_Gfx_Pt_Func
op_sub_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for pixel+color-to-pixel standard subtraction.
 *
 * Combines logic for source pixel and source color properties.
 *
 * @param src_alpha EINA_TRUE if source image has alpha.
 * @param col The source color (ARGB32).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @return A function pointer (RGBA_Gfx_Pt_Func) for the operation.
 */
static RGBA_Gfx_Pt_Func
op_sub_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for mask+color-to-pixel standard subtraction.
 *
 * Assumes a sparse alpha mask (m = SM_AS). Determines source color (c)
 * and destination pixel (d) properties.
 *
 * @param col The source color (ARGB32).
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @return A function pointer (RGBA_Gfx_Pt_Func) for the operation.
 */
static RGBA_Gfx_Pt_Func
op_sub_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for pixel+mask-to-pixel standard subtraction.
 *
 * Assumes a sparse alpha mask (m = SM_AS). Determines source pixel (s)
 * and destination pixel (d) properties.
 *
 * @param src_alpha EINA_TRUE if source image has alpha.
 * @param dst_alpha EINA_TRUE if destination has alpha.
 * @return A function pointer (RGBA_Gfx_Pt_Func) for the operation.
 */
static RGBA_Gfx_Pt_Func
op_sub_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_gfx_pt_func_cpu(s, m, c, d);
}


/**
 * @brief Initializes function tables for relative subtraction operations.
 *
 * This function populates the `op_sub_rel_span_funcs` and `op_sub_rel_pt_funcs`
 * arrays with pointers to appropriate C and MMX (if available) implementations
 * for various "sub_rel" scenarios.
 */
static void
op_sub_rel_init(void)
{
   memset(op_sub_rel_span_funcs, 0, sizeof(op_sub_rel_span_funcs));
   memset(op_sub_rel_pt_funcs, 0, sizeof(op_sub_rel_pt_funcs));
#ifdef BUILD_MMX
   init_sub_rel_pixel_span_funcs_mmx();
   init_sub_rel_pixel_color_span_funcs_mmx();
   init_sub_rel_pixel_mask_span_funcs_mmx();
   init_sub_rel_color_span_funcs_mmx();
   init_sub_rel_mask_color_span_funcs_mmx();

   init_sub_rel_pixel_pt_funcs_mmx();
   init_sub_rel_pixel_color_pt_funcs_mmx();
   init_sub_rel_pixel_mask_pt_funcs_mmx();
   init_sub_rel_color_pt_funcs_mmx();
   init_sub_rel_mask_color_pt_funcs_mmx();
#endif
   init_sub_rel_pixel_span_funcs_c();
   init_sub_rel_pixel_color_span_funcs_c();
   init_sub_rel_pixel_mask_span_funcs_c();
   init_sub_rel_color_span_funcs_c();
   init_sub_rel_mask_color_span_funcs_c();

   init_sub_rel_pixel_pt_funcs_c();
   init_sub_rel_pixel_color_pt_funcs_c();
   init_sub_rel_pixel_mask_pt_funcs_c();
   init_sub_rel_color_pt_funcs_c();
   init_sub_rel_mask_color_pt_funcs_c();
}

/**
 * @brief Placeholder for shutting down relative subtraction operations.
 *
 * @see op_sub_shutdown for more details.
 */
static void
op_sub_rel_shutdown(void)
{
}

/**
 * @brief Selects the appropriate CPU-specific span function for relative subtraction.
 *
 * Similar to `sub_gfx_span_func_cpu`, but uses the `op_sub_rel_span_funcs` table.
 *
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return A function pointer (RGBA_Gfx_Func) to the selected span operation.
 */
static RGBA_Gfx_Func
sub_rel_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_sub_rel_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_sub_rel_span_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @brief Gets a span function for pixel-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_span_get`, but uses `sub_rel_gfx_span_func_cpu`.
 * @copydetails op_sub_pixel_span_get
 */
static RGBA_Gfx_Func
op_sub_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_color_span_get`, but uses `sub_rel_gfx_span_func_cpu`.
 * @copydetails op_sub_color_span_get
 */
static RGBA_Gfx_Func
op_sub_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for pixel+color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_color_span_get`, but uses `sub_rel_gfx_span_func_cpu`.
 * @copydetails op_sub_pixel_color_span_get
 */
static RGBA_Gfx_Func
op_sub_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for mask+color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_mask_color_span_get`, but uses `sub_rel_gfx_span_func_cpu`.
 * @copydetails op_sub_mask_color_span_get
 */
static RGBA_Gfx_Func
op_sub_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a span function for pixel+mask-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_mask_span_get`, but uses `sub_rel_gfx_span_func_cpu`.
 * @copydetails op_sub_pixel_mask_span_get
 */
static RGBA_Gfx_Func
op_sub_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @brief Selects the appropriate CPU-specific point function for relative subtraction.
 *
 * Similar to `sub_gfx_pt_func_cpu`, but uses the `op_sub_rel_pt_funcs` table.
 *
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return A function pointer (RGBA_Gfx_Pt_Func) to the selected point operation.
 */
static RGBA_Gfx_Pt_Func
sub_rel_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_sub_rel_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_sub_rel_pt_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @brief Gets a point function for pixel-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_pt_get`, but uses `sub_rel_gfx_pt_func_cpu`.
 * @copydetails op_sub_pixel_pt_get
 */
static RGBA_Gfx_Pt_Func
op_sub_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_color_pt_get`, but uses `sub_rel_gfx_pt_func_cpu`.
 * @copydetails op_sub_color_pt_get
 */
static RGBA_Gfx_Pt_Func
op_sub_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_N, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for pixel+color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_color_pt_get`, but uses `sub_rel_gfx_pt_func_cpu`.
 * @copydetails op_sub_pixel_color_pt_get
 */
static RGBA_Gfx_Pt_Func
op_sub_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_AN, d = DP_AN;

   if (src_alpha)
	s = SP;
   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for mask+color-to-pixel relative subtraction.
 *
 * Similar to `op_sub_mask_color_pt_get`, but uses `sub_rel_gfx_pt_func_cpu`.
 * @copydetails op_sub_mask_color_pt_get
 */
static RGBA_Gfx_Pt_Func
op_sub_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP_AN;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @brief Gets a point function for pixel+mask-to-pixel relative subtraction.
 *
 * Similar to `op_sub_pixel_mask_pt_get`, but uses `sub_rel_gfx_pt_func_cpu`.
 * @copydetails op_sub_pixel_mask_pt_get
 */
static RGBA_Gfx_Pt_Func
op_sub_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return sub_rel_gfx_pt_func_cpu(s, m, c, d);
}
