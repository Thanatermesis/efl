#include "evas_common_private.h"
/**
 * @brief Table of span functions for 'add' operation.
 * Indexed by: [Src_Pixel_Type][Src_Mask_Type][Src_Color_Type][Dst_Pixel_Type][CPU_Type]
 * Example: op_add_span_funcs[SP_SOLID][SM_NONE][SC_NONE][DP_SOLID][CPU_C]
 * would give a function for solid source, no mask, no source color, solid destination, using C implementation.
 */
static RGBA_Gfx_Func     op_add_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
/**
 * @brief Table of point functions for 'add' operation.
 * Indexed by: [Src_Pixel_Type][Src_Mask_Type][Src_Color_Type][Dst_Pixel_Type][CPU_Type]
 * Example: op_add_pt_funcs[SP_ALPHA][SM_NONE][SC_NONE][DP_ALPHA][CPU_MMX]
 * would give a function for alpha source, no mask, no source color, alpha destination, using MMX implementation.
 */
static RGBA_Gfx_Pt_Func  op_add_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the 'add' operation functions.
 * This function populates the op_add_span_funcs and op_add_pt_funcs tables
 * with appropriate C and MMX (if available) implementations.
 */
static void op_add_init(void);
/**
 * @brief Shuts down the 'add' operation module.
 * Currently, this function is a no-op.
 */
static void op_add_shutdown(void);

/**
 * @brief Retrieves a span processing function for pixel source and 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for color source and 'add' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel source modulated by a color, using 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param col The modulating color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for a color source modulated by a mask, using 'add' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel source modulated by a mask, using 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point processing function for pixel source and 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for color source and 'add' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel source modulated by a color, using 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param col The modulating color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for a color source modulated by a mask, using 'add' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel source modulated by a mask, using 'add' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for the 'add' operation.
 * This structure holds pointers to the initialization, shutdown, and various
 * span/point getter functions for the 'add' blending operation.
 */
static RGBA_Gfx_Compositor  _composite_add = { "add",
 op_add_init, op_add_shutdown,
 op_add_pixel_span_get, op_add_color_span_get,
 op_add_pixel_color_span_get, op_add_mask_color_span_get,
 op_add_pixel_mask_span_get,
 op_add_pixel_pt_get, op_add_color_pt_get,
 op_add_pixel_color_pt_get, op_add_mask_color_pt_get,
 op_add_pixel_mask_pt_get
 };

/**
 * @brief Gets the compositor for the 'add' operation.
 * @return A pointer to the static _composite_add structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_add_get(void)
{
   return &(_composite_add);
}

/**
 * @brief Table of span functions for 'add_rel' (additive relative) operation.
 * Indexed similarly to op_add_span_funcs.
 * This operation is typically (Rs*Sa + Ds, Gs*Sa + Dg, Bs*Sa + Db, As*Sa + Da)
 * but without clamping to destination alpha, allowing results > Da.
 */
static RGBA_Gfx_Func     op_add_rel_span_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];
/**
 * @brief Table of point functions for 'add_rel' (additive relative) operation.
 * Indexed similarly to op_add_pt_funcs.
 */
static RGBA_Gfx_Pt_Func  op_add_rel_pt_funcs[SP_LAST][SM_LAST][SC_LAST][DP_LAST][CPU_LAST];

/**
 * @brief Initializes the 'add_rel' operation functions.
 * Populates op_add_rel_span_funcs and op_add_rel_pt_funcs.
 */
static void op_add_rel_init(void);
/**
 * @brief Shuts down the 'add_rel' operation module.
 * Currently, this function is a no-op.
 */
static void op_add_rel_shutdown(void);

/**
 * @brief Retrieves a span processing function for pixel source and 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for color source and 'add_rel' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel source modulated by a color, using 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param col The modulating color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for a color source modulated by a mask, using 'add_rel' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels);
/**
 * @brief Retrieves a span processing function for pixel source modulated by a mask, using 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param src_sparse_alpha EINA_TRUE if source has sparse alpha (unused).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @param pixels Number of pixels in the span (unused).
 * @return A pointer to the appropriate RGBA_Gfx_Func, or NULL if not found.
 */
static RGBA_Gfx_Func op_add_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha, Eina_Bool dst_alpha, int pixels);

/**
 * @brief Retrieves a point processing function for pixel source and 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for color source and 'add_rel' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel source modulated by a color, using 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param col The modulating color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for a color source modulated by a mask, using 'add_rel' operation.
 * @param col The source color (e.g., 0xAARRGGBB).
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha);
/**
 * @brief Retrieves a point processing function for pixel source modulated by a mask, using 'add_rel' operation.
 * @param src_alpha EINA_TRUE if source has alpha, EINA_FALSE otherwise.
 * @param dst_alpha EINA_TRUE if destination has alpha, EINA_FALSE otherwise.
 * @return A pointer to the appropriate RGBA_Gfx_Pt_Func, or NULL if not found.
 */
static RGBA_Gfx_Pt_Func op_add_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha);

/**
 * @brief Compositor structure for the 'add_rel' operation.
 * This structure holds pointers to the initialization, shutdown, and various
 * span/point getter functions for the 'add_rel' blending operation.
 */
static RGBA_Gfx_Compositor  _composite_add_rel = { "add_rel",
 op_add_rel_init, op_add_rel_shutdown,
 op_add_rel_pixel_span_get, op_add_rel_color_span_get,
 op_add_rel_pixel_color_span_get, op_add_rel_mask_color_span_get,
 op_add_rel_pixel_mask_span_get,
 op_add_rel_pixel_pt_get, op_add_rel_color_pt_get,
 op_add_rel_pixel_color_pt_get, op_add_rel_mask_color_pt_get,
 op_add_rel_pixel_mask_pt_get
 };

/**
 * @brief Gets the compositor for the 'add_rel' operation.
 * @return A pointer to the static _composite_add_rel structure.
 */
RGBA_Gfx_Compositor  *
evas_common_gfx_compositor_add_rel_get(void)
{
   return &(_composite_add_rel);
}


# include "./evas_op_add/op_add_pixel_.c"
# include "./evas_op_add/op_add_color_.c"
# include "./evas_op_add/op_add_pixel_color_.c"
# include "./evas_op_add/op_add_pixel_mask_.c"
# include "./evas_op_add/op_add_mask_color_.c"
//# include "./evas_op_add/op_add_pixel_mask_color_.c"

# include "./evas_op_add/op_add_pixel_i386.c"
# include "./evas_op_add/op_add_color_i386.c"
# include "./evas_op_add/op_add_pixel_color_i386.c"
# include "./evas_op_add/op_add_pixel_mask_i386.c"
# include "./evas_op_add/op_add_mask_color_i386.c"
//# include "op_add_pixel_mask_color_.c"

static void
op_add_init(void)
{
   memset(op_add_span_funcs, 0, sizeof(op_add_span_funcs));
   memset(op_add_pt_funcs, 0, sizeof(op_add_pt_funcs));
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
        init_add_pixel_span_funcs_mmx();
        init_add_pixel_color_span_funcs_mmx();
        init_add_pixel_mask_span_funcs_mmx();
        init_add_color_span_funcs_mmx();
        init_add_mask_color_span_funcs_mmx();

        init_add_pixel_pt_funcs_mmx();
        init_add_pixel_color_pt_funcs_mmx();
        init_add_pixel_mask_pt_funcs_mmx();
        init_add_color_pt_funcs_mmx();
        init_add_mask_color_pt_funcs_mmx();
     }
#endif
   init_add_pixel_span_funcs_c();
   init_add_pixel_color_span_funcs_c();
   init_add_rel_pixel_mask_span_funcs_c();
   init_add_color_span_funcs_c();
   init_add_mask_color_span_funcs_c();

   init_add_pixel_pt_funcs_c();
   init_add_pixel_color_pt_funcs_c();
   init_add_rel_pixel_mask_pt_funcs_c();
   init_add_color_pt_funcs_c();
   init_add_mask_color_pt_funcs_c();
}

static void
op_add_shutdown(void)
{
}

/**
 * @brief Selects an 'add' span function based on CPU features.
 * It prioritizes MMX optimized functions if available, otherwise falls back to C.
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return The selected RGBA_Gfx_Func, or NULL if none matches.
 */
static RGBA_Gfx_Func
add_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_add_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_add_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_add_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
/**
 * @brief Selects an 'add' point function based on CPU features.
 * It prioritizes MMX optimized functions if available, otherwise falls back to C.
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return The selected RGBA_Gfx_Pt_Func, or NULL if none matches.
 */
add_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_add_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_add_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_add_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
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
   return add_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
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
   return add_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
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
   return add_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_gfx_pt_func_cpu(s, m, c, d);
}



static void
op_add_rel_init(void)
{
   memset(op_add_rel_span_funcs, 0, sizeof(op_add_rel_span_funcs));
   memset(op_add_rel_pt_funcs, 0, sizeof(op_add_rel_pt_funcs));
#ifdef BUILD_MMX
   init_add_rel_pixel_span_funcs_mmx();
   init_add_rel_pixel_color_span_funcs_mmx();
   init_add_rel_pixel_mask_span_funcs_mmx();
   init_add_rel_color_span_funcs_mmx();
   init_add_rel_mask_color_span_funcs_mmx();

   init_add_rel_pixel_pt_funcs_mmx();
   init_add_rel_pixel_color_pt_funcs_mmx();
   init_add_rel_pixel_mask_pt_funcs_mmx();
   init_add_rel_color_pt_funcs_mmx();
   init_add_rel_mask_color_pt_funcs_mmx();
#endif
   init_add_rel_pixel_span_funcs_c();
   init_add_rel_pixel_color_span_funcs_c();
   init_add_rel_pixel_mask_span_funcs_c();
   init_add_rel_color_span_funcs_c();
   init_add_rel_mask_color_span_funcs_c();

   init_add_rel_pixel_pt_funcs_c();
   init_add_rel_pixel_color_pt_funcs_c();
   init_add_rel_pixel_mask_pt_funcs_c();
   init_add_rel_color_pt_funcs_c();
   init_add_rel_mask_color_pt_funcs_c();
}

static void
op_add_rel_shutdown(void)
{
}

/**
 * @brief Selects an 'add_rel' span function based on CPU features.
 * It prioritizes MMX optimized functions if available, otherwise falls back to C.
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return The selected RGBA_Gfx_Func, or NULL if none matches.
 */
static RGBA_Gfx_Func
add_rel_gfx_span_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_add_rel_span_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_add_rel_span_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Func
op_add_rel_pixel_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_rel_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_rel_pixel_color_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_rel_mask_color_span_get(DATA32 col, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
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
   return add_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Func
op_add_rel_pixel_mask_span_get(Eina_Bool src_alpha, Eina_Bool src_sparse_alpha EINA_UNUSED, Eina_Bool dst_alpha, int pixels EINA_UNUSED)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_rel_gfx_span_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
/**
 * @brief Selects an 'add_rel' point function based on CPU features.
 * It prioritizes MMX optimized functions if available, otherwise falls back to C.
 * @param s Source pixel type (SP_* enum).
 * @param m Source mask type (SM_* enum).
 * @param c Source color type (SC_* enum).
 * @param d Destination pixel type (DP_* enum).
 * @return The selected RGBA_Gfx_Pt_Func, or NULL if none matches.
 */
add_rel_gfx_pt_func_cpu(int s, int m, int c, int d)
{
   RGBA_Gfx_Pt_Func  func = NULL;
   int cpu = CPU_N;
#ifdef BUILD_MMX
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     {
	cpu = CPU_MMX;
	func = op_add_rel_pt_funcs[s][m][c][d][cpu];
	if (func) return func;
     }
#endif
   cpu = CPU_C;
   func = op_add_rel_pt_funcs[s][m][c][d][cpu];
   return func;
}

static RGBA_Gfx_Pt_Func
op_add_rel_pixel_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_N, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_rel_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
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
   return add_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_rel_pixel_color_pt_get(Eina_Bool src_alpha, DATA32 col, Eina_Bool dst_alpha)
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
   return add_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_rel_mask_color_pt_get(DATA32 col, Eina_Bool dst_alpha)
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
   return add_rel_gfx_pt_func_cpu(s, m, c, d);
}

static RGBA_Gfx_Pt_Func
op_add_rel_pixel_mask_pt_get(Eina_Bool src_alpha, Eina_Bool dst_alpha)
{
   int  s = SP_AN, m = SM_AS, c = SC_N, d = DP_AN;

   if (src_alpha)
	s = SP;
   if (dst_alpha)
	d = DP;
   return add_rel_gfx_pt_func_cpu(s, m, c, d);
}
