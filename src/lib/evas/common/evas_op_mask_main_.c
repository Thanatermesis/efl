#include "evas_common_private.h"

/**
 * @brief Array of function pointers for span operations with masking.
 *
 * This 5-dimensional array stores function pointers for various span operations.
 * The dimensions correspond to:
 * - Source pixel properties (SP_LAST)
 * - Source mask properties (SM_LAST)
 * - Source color properties (SC_LAST)
 * - Destination pixel properties (DP_LAST)
 * - CPU-specific implementations (CPU_LAST)
 *
 * Example: op_mask_span_funcs[SP_SOLID][SM_NONE][SC_NONE][DP_SOLID][CPU_C]
 * would point to a C implementation for a solid source, no mask, no source color,
 * and solid destination.
 */
static RGBA_Gfx_Func     op_mask_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Array of function pointers for point operations with masking.
 *
 * This 5-dimensional array stores function pointers for various point (single pixel) operations.
 * The dimensions correspond to:
 * - Source pixel properties (SP_LAST)
 * - Source mask properties (SM_LAST)
 * - Source color properties (SC_LAST)
 * - Destination pixel properties (DP_LAST)
 * - CPU-specific implementations (CPU_LAST)
 *
 * Example: op_mask_pt_funcs[SP_ALPHA][SM_NONE][SC_NONE][DP_ALPHA][CPU_MMX]
 * would point to an MMX implementation for an alpha source, no mask, no source color,
 * and alpha destination.
 */
static RGBA_Gfx_Pt_Func  op_mask_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the mask operation functions.
 *
 * This function populates the op_mask_span_funcs and op_mask_pt_funcs arrays
 * with appropriate function pointers for different CPU capabilities (C, MMX, etc.)
 * and operation types.
 */
static void op_mask_init(void);
/**
 * @brief Shuts down the mask operation module.
 *
 * Currently, this function is a placeholder and does not perform any specific cleanup.
 */
static void op_mask_shutdown(void);

/**
 * @brief Retrieves a span function for pixel-to-pixel masking operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (unused).
 * @return A function pointer to the appropriate span operation, or NULL if not found.
 */
static RGBA_Gfx_Func op_mask_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span function for color-to-pixel masking operations.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (unused).
 * @return A function pointer to the appropriate span operation, or NULL if not found.
 */
static RGBA_Gfx_Func op_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span function for pixel-and-color-to-pixel masking operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source image has sparse alpha (unused).
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @param pixels The number of pixels in the span (unused).
 * @return A function pointer to the appropriate span operation, or NULL if not found.
 */
static RGBA_Gfx_Func op_mask_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span function for mask-and-color-to-pixel masking operations.
 * The mask is implicitly the source alpha channel.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused, destination is assumed to have alpha).
 * @param pixels The number of pixels in the span (unused).
 * @return A function pointer to the appropriate span operation, or NULL if not found.
 */
static RGBA_Gfx_Func op_mask_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span function for pixel-and-mask-to-pixel masking operations.
 * The mask is implicitly the source alpha channel.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if the source image has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused, destination is assumed to have alpha).
 * @param pixels The number of pixels in the span (unused).
 * @return A function pointer to the appropriate span operation, or NULL if not found.
 */
static RGBA_Gfx_Func op_mask_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point function for pixel-to-pixel masking operations.
 * @param src_alpha EINA_TRUE if the source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point operation, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_mask_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point function for color-to-pixel masking operations.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point operation, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point function for pixel-and-color-to-pixel masking operations.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha, EINA_FALSE otherwise.
 * @return A function pointer to the appropriate point operation, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_mask_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point function for mask-and-color-to-pixel masking operations.
 * The mask is implicitly the source alpha channel.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused, destination is assumed to have alpha).
 * @return A function pointer to the appropriate point operation, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_mask_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point function for pixel-and-mask-to-pixel masking operations.
 * The mask is implicitly the source alpha channel.
 * @param src_alpha EINA_TRUE if the source image has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if the destination has alpha (unused, destination is assumed to have alpha).
 * @return A function pointer to the appropriate point operation, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_mask_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Defines the compositor for "mask" operations.
 *
 * This structure holds function pointers for initializing, shutting down,
 * and retrieving various span and point operations related to masking.
 */
static RGBA_Gfx_Compositor  _composite_mask = { "mask",
 op_mask_init, op_mask_shutdown,
 op_mask_pixel_span_get, op_mask_color_span_get,
 op_mask_pixel_color_span_get, op_mask_mask_color_span_get,
 op_mask_pixel_mask_span_get,
 op_mask_pixel_pt_get, op_mask_color_pt_get,
 op_mask_pixel_color_pt_get, op_mask_mask_color_pt_get,
 op_mask_pixel_mask_pt_get
 };

/**
 * @brief Retrieves the "mask" compositor.
 * @return A pointer to the static _composite_mask structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_mask_get(void)
{
   return &(_composite_mask);
}


# include "./evas_op_mask/op_mask_pixel_.c"
# include "./evas_op_mask/op_mask_color_.c"
# include "./evas_op_mask/op_mask_pixel_color_.c"
# include "./evas_op_mask/op_mask_pixel_mask_.c"
# include "./evas_op_mask/op_mask_mask_color_.c"
//# include "./evas_op_mask/op_mask_pixel_mask_color_.c"

# include "./evas_op_mask/op_mask_pixel_i386.c"
# include "./evas_op_mask/op_mask_color_i386.c"
# include "./evas_op_mask/op_mask_pixel_color_i386.c"
# include "./evas_op_mask/op_mask_pixel_mask_i386.c"
# include "./evas_op_mask/op_mask_mask_color_i386.c"
//# include "./evas_op_mask/op_mask_pixel_mask_color_i386.c"


/**
 * @internal
 * @brief Initializes the mask operation functions.
 *
 * This function populates the op_mask_span_funcs and op_mask_pt_funcs arrays
 * with appropriate function pointers for different CPU capabilities (C, MMX, etc.)
 * and operation types. It first clears the function pointer arrays, then initializes
 * MMX versions if available, followed by C versions as fallbacks.
 */
static void
op_mask_init(void)
{
   memset(op_mask_span_funcs, 0, sizeof(op_mask_span_funcs));
   memset(op_mask_pt_funcs, 0, sizeof(op_mask_pt_funcs));
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_mask_pixel_span_funcs_mmx();
        init_mask_pixel_color_span_funcs_mmx();
        init_mask_pixel_mask_span_funcs_mmx();
        init_mask_color_span_funcs_mmx();
        init_mask_mask_color_span_funcs_mmx();

        init_mask_pixel_pt_funcs_mmx();
        init_mask_pixel_color_pt_funcs_mmx();
        init_mask_pixel_mask_pt_funcs_mmx();
        init_mask_color_pt_funcs_mmx();
        init_mask_mask_color_pt_funcs_mmx();
     }
#endif
   init_mask_pixel_span_funcs_c();
   init_mask_pixel_color_span_funcs_c();
   init_mask_pixel_mask_span_funcs_c();
   init_mask_color_span_funcs_c();
   init_mask_mask_color_span_funcs_c();

   init_mask_pixel_pt_funcs_c();
   init_mask_pixel_color_pt_funcs_c();
   init_mask_pixel_mask_pt_funcs_c();
   init_mask_color_pt_funcs_c();
   init_mask_mask_color_pt_funcs_c();
}

/**
 * @internal
 * @brief Shuts down the mask operation module.
 *
 * Currently, this function is a placeholder and does not perform any specific cleanup.
 * It is called when the compositor is shut down.
 */
static void
op_mask_shutdown(void)
{
}

/**
 * @internal
 * @brief Selects the appropriate CPU-specific span function.
 *
 * This helper function attempts to find an MMX optimized function first if available,
 * otherwise, it falls back to a C implementation.
 * @param s Source pixel property index (SP_*).
 * @param m Source mask property index (SM_*).
 * @param c Source color property index (SC_*).
 * @param d Destination pixel property index (DP_*).
 * @return A function pointer to the selected span operation, or NULL if none is found.
 */
static RGBA_Gfx_Func
mask_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_mask_span_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_mask_span_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @internal
 * @brief Retrieves a span function for pixel-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_span_funcs based on source and destination alpha.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param src_sparse_alpha EINA_TRUE if the source has sparse alpha (currently unused).
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels Number of pixels in the span (currently unused by this selector).
 * @return A function pointer to the appropriate span operation.
 */
static RGBA_Gfx_Func
op_mask_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return mask_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a span function for color-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_span_funcs based on color properties and destination alpha.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels Number of pixels in the span (currently unused by this selector).
 * @return A function pointer to the appropriate span operation.
 */
static RGBA_Gfx_Func
op_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return mask_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a span function for pixel-and-color-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_span_funcs based on source alpha, color properties, and destination alpha.
 * @param src_alpha EINA_TRUE if the source image has alpha.
 * @param src_sparse_alpha EINA_TRUE if the source image has sparse alpha (currently unused).
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @param pixels Number of pixels in the span (currently unused by this selector).
 * @return A function pointer to the appropriate span operation.
 */
static RGBA_Gfx_Func
op_mask_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return mask_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a span function for mask-and-color-to-pixel masking operations.
 *
 * The mask is implicitly the source alpha channel.
 * Determines the correct function from op_mask_span_funcs based on color properties.
 * Destination is assumed to have alpha.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused by this selector, assumed true).
 * @param pixels Number of pixels in the span (currently unused by this selector).
 * @return A function pointer to the appropriate span operation.
 */
static RGBA_Gfx_Func
op_mask_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return mask_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a span function for pixel-and-mask-to-pixel masking operations.
 *
 * The mask is implicitly the source alpha channel.
 * Determines the correct function from op_mask_span_funcs based on source alpha.
 * Destination is assumed to have alpha.
 * @param src_alpha EINA_TRUE if the source image has alpha.
 * @param src_sparse_alpha EINA_TRUE if the source image has sparse alpha (currently unused).
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused by this selector, assumed true).
 * @param pixels Number of pixels in the span (currently unused by this selector).
 * @return A function pointer to the appropriate span operation.
 */
static RGBA_Gfx_Func
op_mask_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha EINA_UNUSED, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return mask_gfx_span_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Selects the appropriate CPU-specific point function.
 *
 * This helper function attempts to find an MMX optimized function first if available,
 * otherwise, it falls back to a C implementation.
 * @param s Source pixel property index (SP_*).
 * @param m Source mask property index (SM_*).
 * @param c Source color property index (SC_*).
 * @param d Destination pixel property index (DP_*).
 * @return A function pointer to the selected point operation, or NULL if none is found.
 */
static RGBA_Gfx_Pt_Func
mask_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
    {
      cpu = CPU_MMX;
      func = op_mask_pt_funcs[s][m][c][d][cpu];
      if (func) return func;
    }
#endif
   cpu = CPU_C;
   func = op_mask_pt_funcs[s][m][c][d][cpu];
   return func;
}

/**
 * @internal
 * @brief Retrieves a point function for pixel-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_pt_funcs based on source and destination alpha.
 * @param src_alpha EINA_TRUE if the source has alpha.
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @return A function pointer to the appropriate point operation.
 */
static RGBA_Gfx_Pt_Func
op_mask_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
     {
	s = SP;
     }
   if (dst_alpha)
	d = DP;
   return mask_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a point function for color-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_pt_funcs based on color properties and destination alpha.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @return A function pointer to the appropriate point operation.
 */
static RGBA_Gfx_Pt_Func
op_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
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
   return mask_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a point function for pixel-and-color-to-pixel masking operations.
 *
 * Determines the correct function from op_mask_pt_funcs based on source alpha, color properties, and destination alpha.
 * @param src_alpha EINA_TRUE if the source image has alpha.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha.
 * @return A function pointer to the appropriate point operation.
 */
static RGBA_Gfx_Pt_Func
op_mask_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
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
   return mask_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a point function for mask-and-color-to-pixel masking operations.
 *
 * The mask is implicitly the source alpha channel.
 * Determines the correct function from op_mask_pt_funcs based on color properties.
 * Destination is assumed to have alpha.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused by this selector, assumed true).
 * @return A function pointer to the appropriate point operation.
 */
static RGBA_Gfx_Pt_Func
op_mask_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_N, m = SM_AS, c = SC_AN, d = DP;

   if ((col >> 24) < 255)
	c = SC;
   if (col == (col | 0x00ffffff))
	c = SC_AA;
   if (col == 0xffffffff)
	c = SC_N;
   return mask_gfx_pt_func_cpu(s, m, c, d);
}

/**
 * @internal
 * @brief Retrieves a point function for pixel-and-mask-to-pixel masking operations.
 *
 * The mask is implicitly the source alpha channel.
 * Determines the correct function from op_mask_pt_funcs based on source alpha.
 * Destination is assumed to have alpha.
 * @param src_alpha EINA_TRUE if the source image has alpha.
 * @param dst_alpha EINA_TRUE if the destination has alpha (currently unused by this selector, assumed true).
 * @return A function pointer to the appropriate point operation.
 */
static RGBA_Gfx_Pt_Func
op_mask_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP;

   if (src_alpha)
	s = SP;
   return mask_gfx_pt_func_cpu(s, m, c, d);
}
