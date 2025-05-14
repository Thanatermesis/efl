#include "evas_common_private.h"
#include "evas_convert_color.h"
#include "evas_scale_span.h"

/**
 * @internal
 * @brief Scales a span of RGBA pixels.
 *
 * This function scales a source span of RGBA pixels to a destination span of
 * a different length. It supports a multiplier color and direction control.
 *
 * @param src Pointer to the source RGBA pixel data.
 *            Each DATA32 element represents a pixel in 0xAARRGGBB format.
 *            Example: `{0xffff0000, 0xff00ff00, 0xff0000ff}` (Red, Green, Blue pixels)
 * @param mask Unused in this function.
 * @param src_len Length of the source pixel span.
 * @param mul_col Multiplier color in 0xAARRGGBB format. If not 0xffffffff,
 *                each source pixel is multiplied by this color.
 * @param dst Pointer to the destination RGBA pixel data array.
 *            The scaled pixels will be written here.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling. If < 0, scaling is done in reverse
 *            (from end to start of the destination buffer).
 */
static void
evas_common_scale_rgba_span_(DATA32 *src, DATA8 *mask EINA_UNUSED, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int  mul = 0, step = 1;
   DATA32 *pdst = dst;

   if (!src || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (mul_col != 0xffffffff)
	mul = 1;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

   if ((src_len == 1) || (dst_len == 1))
     {
	DATA32 c = *src;

	if (mul) c = MUL4_SYM(mul_col, c);
	while (dst_len--)
	   *dst++ = c;
	return;
     }

   if (src_len == dst_len)
     {
	if (mul)
	  {
#ifdef BUILD_MMX
	    pxor_r2r(mm0, mm0);
	    MOV_A2R(ALPHA_255, mm5)
	    MOV_P2R(mul_col, mm7, mm0)
#endif
	    while (dst_len--)
	      {
#ifdef BUILD_MMX
		MOV_P2R(*src, mm1, mm0)
		MUL4_SYM_R2R(mm7, mm1, mm5)
		MOV_R2P(mm1, *pdst, mm0)
#else
		*pdst = MUL4_SYM(mul_col, *src);
#endif
		src++;  pdst += step;
	      }
	    return;
	  }
	while (dst_len--)
	  {
	    *pdst = *src;
	    src++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

#ifdef BUILD_MMX
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_255, mm5)
	if (mul)
	  {
	    MOV_P2R(mul_col, mm7, mm0)
	  }
#endif
	while (dst_len--)
	  {
	    DATA32   p2, p1 = 0;
	    int      a;

	    sx = (sxx >> 16);
	    if (sx < src_len)
		p1 = *(src + sx);
	    p2 = p1;
	    if ((sx + 1) < src_len)
	        p2 = *(src + sx + 1);
	    a = 1 + ((sxx - (sx << 16)) >> 8);
#ifdef BUILD_MMX
	    MOV_A2R(a, mm3)
	    MOV_P2R(p1, mm1, mm0)
	    MOV_P2R(p2, mm2, mm0)
	    INTERP_256_R2R(mm3, mm2, mm1, mm5)
	    if (mul)
	      {
		MUL4_SYM_R2R(mm7, mm1, mm5)
	      }
	    MOV_R2P(mm1, *pdst, mm0)
#else
	    p1 = INTERP_256(a, p2, p1);
	    if (mul)
		p1 = MUL4_SYM(mul_col, p1);
	    *pdst = p1;
#endif
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}

/**
 * @internal
 * @brief Scales a span of RGBA pixels with an alpha mask.
 *
 * This function scales a source span of RGBA pixels, applying an alpha mask
 * to each source pixel before scaling. It also supports a multiplier color
 * and direction control.
 *
 * @param src Pointer to the source RGBA pixel data.
 *            Each DATA32 element represents a pixel in 0xAARRGGBB format.
 * @param mask Pointer to the alpha mask data (array of DATA8).
 *             Each DATA8 element is an alpha value (0-255) corresponding
 *             to a source pixel.
 *             Example: `{0x80, 0xff, 0x40}` (Semi-transparent, Opaque, Very transparent)
 * @param src_len Length of the source pixel span and mask.
 * @param mul_col Multiplier color in 0xAARRGGBB format. If not 0xffffffff,
 *                each masked source pixel is multiplied by this color.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling. If < 0, scaling is done in reverse.
 */
static void
evas_common_scale_rgba_a8_span_(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int  mul = 0, step = 1;
   DATA32 *pdst = dst;

   if (!src || !mask || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (mul_col != 0xffffffff)
	mul = 1;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

   if ((src_len == 1) || (dst_len == 1))
     {
	DATA32 c = MUL_SYM(*mask, *src);

	if (mul) c = MUL4_SYM(mul_col, c);
	while (dst_len--)
	   *dst++ = c;
	return;
     }

   if (src_len == dst_len)
     {
#ifdef BUILD_MMX
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_255, mm5)
#endif
	if (mul)
	  {
#ifdef BUILD_MMX
	    MOV_P2R(mul_col, mm7, mm0)
#endif
	    while (dst_len--)
	      {
#ifdef BUILD_MMX
		MOV_P2R(*src, mm1, mm0)
		MOV_A2R(*mask, mm3)
		MUL4_SYM_R2R(mm3, mm1, mm5)
		MUL4_SYM_R2R(mm7, mm1, mm5)
		MOV_R2P(mm1, *pdst, mm0)
#else
		DATA32  c = MUL_SYM(*mask, *src);
		*pdst = MUL4_SYM(mul_col, c);
#endif
		src++;  mask++;  pdst += step;
	      }
	    return;
	  }
	while (dst_len--)
	  {
#ifdef BUILD_MMX
	    MOV_P2R(*src, mm1, mm0)
	    MOV_A2R(*mask, mm3)
	    MUL4_SYM_R2R(mm3, mm1, mm5)
	    MOV_R2P(mm1, *pdst, mm0)
#else
	    *pdst = MUL_SYM(*mask, *src);
#endif
	    src++;  mask++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

#ifdef BUILD_MMX
	pxor_r2r(mm0, mm0);
	MOV_A2R(ALPHA_255, mm5)
	if (mul)
	  {
	    MOV_P2R(mul_col, mm7, mm0)
	  }
#endif
	while (dst_len--)
	  {
	    DATA32   p2, p1 = 0;
	    int      a, a2, a1 = 0;

	    sx = (sxx >> 16);
	    if (sx < src_len)
	      {
		p1 = *(src + sx);
		a1 = *(mask + sx);
	      }
	    p2 = p1;  a2 = a1;
	    if ((sx + 1) < src_len)
	      {
		p2 = *(src + sx + 1);
		a2 = *(mask + sx + 1);
	      }
	    a = 1 + ((sxx - (sx << 16)) >> 8);
#ifdef BUILD_MMX
	    MOV_A2R(a, mm3)
	    MOV_P2R(p1, mm1, mm0)
	    MOV_P2R(p2, mm2, mm0)
	    INTERP_256_R2R(mm3, mm2, mm1, mm5)
	    a1 += 1 + ((a * (a2 - a1)) >> 8);
	    MOV_A2R(a1, mm3)
	    MUL4_256_R2R(mm3, mm1)
	    if (mul)
	      {
		MUL4_SYM_R2R(mm7, mm1, mm5)
	      }
	    MOV_R2P(mm1, *pdst, mm0)
#else
	    p1 = INTERP_256(a, p2, p1);
	    a1 += 1 + ((a * (a2 - a1)) >> 8);
	    p1 = MUL_256(a1, p1);
	    if (mul)
		p1 = MUL4_SYM(mul_col, p1);
	    *pdst = p1;
#endif
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}

/**
 * @internal
 * @brief Scales a span of alpha mask values, applying a multiplier color.
 *
 * This function scales a source span of alpha mask values. Each scaled alpha
 * value is then used to modulate a multiplier color, and the result is
 * written to the destination. The source pixel data is unused.
 *
 * @param src Unused in this function.
 * @param mask Pointer to the source alpha mask data (array of DATA8).
 *             Example: `{0xff, 0x80, 0x00}` (Opaque, Semi-transparent, Fully transparent)
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color in 0xAARRGGBB format. This color is
 *                modulated by the scaled alpha values.
 * @param dst Pointer to the destination RGBA pixel data array.
 *            The result of (scaled_alpha * mul_col) is stored here.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling. If < 0, scaling is done in reverse.
 */
static void
evas_common_scale_a8_span_(DATA32 *src EINA_UNUSED, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int    step = 1;
   DATA32 *pdst = dst;

   if (!mask || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

   if ((src_len == 1) || (dst_len == 1))
     {
	DATA32 c = MUL_SYM(*mask, mul_col);

	while (dst_len--)
	   *dst++ = c;
	return;
     }

#ifdef BUILD_MMX
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_255, mm5)
   MOV_P2R(mul_col, mm7, mm0)
#endif
   if (src_len == dst_len)
     {
	while (dst_len--)
	  {
#ifdef BUILD_MMX
	    MOV_A2R(*mask, mm3)
	    MUL4_SYM_R2R(mm7, mm3, mm5)
	    MOV_R2P(mm3, *pdst, mm0)
#else
	    *pdst = MUL_SYM(*mask, mul_col);
#endif
	    mask++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

	while (dst_len--)
	  {
	    int   a, a2, a1 = 0;

	    sx = (sxx >> 16);
	    if (sx < src_len)
		a1 = *(mask + sx);
	    a2 = a1;
	    if ((sx + 1) < src_len)
		a2 = *(mask + sx + 1);
	    a = 1 + ((sxx - (sx << 16)) >> 8);
	    a1 += 1 + ((a * (a2 - a1)) >> 8);
#ifdef BUILD_MMX
	    MOV_A2R(a1, mm3)
	    MUL4_256_R2R(mm7, mm3)
	    MOV_R2P(mm3, *pdst, mm0)
#else
	    *pdst = MUL_256(a1, mul_col);
#endif
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}

/**
 * @internal
 * @brief Scales an alpha mask and uses it to modulate existing destination pixels.
 *
 * This function scales a source span of alpha mask values. The scaled alpha
 * is then optionally multiplied by `mul_col` (if `mul_col` is not white).
 * The resulting alpha is then used to modulate the existing pixels in the
 * `dst` buffer. This is effectively a "clip" operation where the scaled
 * alpha mask defines the transparency of the destination pixels.
 *
 * @param src Unused in this function.
 * @param mask Pointer to the source alpha mask data (array of DATA8).
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color in 0xAARRGGBB format. If not 0xffffffff,
 *                the scaled alpha mask values are first multiplied by this color's
 *                alpha component (effectively).
 * @param dst Pointer to the destination RGBA pixel data array, which is read and modified.
 *            Each pixel `dst[i]` becomes `dst[i] * scaled_mask_alpha[i]`.
 *            If `mul` is true, it becomes `dst[i] * (scaled_mask_alpha[i] * mul_col_alpha)`.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of processing. If < 0, processing is done in reverse.
 */
static void
evas_common_scale_clip_a8_span_(DATA32 *src EINA_UNUSED, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int   mul = 0, step = 1;
   DATA32 *pdst = dst;

   if (!mask || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (mul_col != 0xffffffff)
	mul = 1;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

#ifdef BUILD_MMX
   pxor_r2r(mm0, mm0);
   MOV_A2R(ALPHA_255, mm5)
   if (mul)
     {
	MOV_P2R(mul_col, mm7, mm0)
     }
#endif
   if ((src_len == 1) || (dst_len == 1))
     {
#ifdef BUILD_MMX
	MOV_A2R(*mask, mm3)
#else
	DATA32 c = *mask;
#endif
	if (mul)
	  {
#ifdef BUILD_MMX
	    MUL4_SYM_R2R(mm7, mm3, mm5)
#else
	    c = MUL_SYM(c, mul_col);
#endif
	    while (dst_len--)
	      {
#ifdef BUILD_MMX
		MOV_P2R(*dst, mm1, mm0)
		MUL4_SYM_R2R(mm3, mm1, mm5)
		MOV_R2P(mm1, *dst, mm0)
#else
		*dst = MUL4_SYM(c, *dst);
#endif
		dst++;
	      }
	    return;
	  }
	while (dst_len--)
	  {
#ifdef BUILD_MMX
	    MOV_P2R(*dst, mm1, mm0)
	    MUL4_SYM_R2R(mm3, mm1, mm5)
	    MOV_R2P(mm1, *dst, mm0)
#else
	    *dst = MUL_SYM(c, *dst);
#endif
	    dst++;
	  }
	return;
     }

   if (src_len == dst_len)
     {
	if (mul)
	  {
	    while (dst_len--)
	      {
#ifdef BUILD_MMX
		MOV_A2R(*mask, mm3)
		MUL4_SYM_R2R(mm7, mm3, mm5)
		MOV_P2R(*pdst, mm1, mm0)
		MUL4_SYM_R2R(mm3, mm1, mm5)
		MOV_R2P(mm1, *pdst, mm0)
#else
		DATA32 c = MUL_SYM(*mask, mul_col);

		*pdst = MUL4_SYM(c, *pdst);
#endif
		mask++;  pdst += step;
	      }
	    return;
	  }
	while (dst_len--)
	  {
#ifdef BUILD_MMX
	    MOV_A2R(*mask, mm3)
	    MOV_P2R(*pdst, mm1, mm0)
	    MUL4_SYM_R2R(mm3, mm1, mm5)
	    MOV_R2P(mm1, *pdst, mm0)
#else
	    *pdst = MUL_SYM(*mask, *pdst);
#endif
	    mask++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

	while (dst_len--)
	  {
	    int   a, a2, a1 = 0;

	    sx = (sxx >> 16);
	    if (sx < src_len)
		a1 = *(mask + sx);
	    a2 = a1;
	    if ((sx + 1) < src_len)
		a2 = *(mask + sx + 1);
	    a = 1 + ((sxx - (sx << 16)) >> 8);
	    a1 += 1 + ((a * (a2 - a1)) >> 8);
#ifdef BUILD_MMX
	    MOV_A2R(a1, mm3)
	    MOV_P2R(*pdst, mm1, mm0)
	    MUL4_256_R2R(mm3, mm1)
	    if (mul)
	      {
		MUL4_SYM_R2R(mm7, mm1, mm5)
	      }
	    MOV_R2P(mm1, *pdst, mm0)
#else
	    *pdst = MUL_256(a1, *pdst);
	    if (mul)
		*pdst = MUL4_SYM(mul_col, *pdst);
#endif
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}

/**
 * @brief Scales a span of RGBA pixels.
 *
 * This is a public API wrapper for evas_common_scale_rgba_span_().
 * It scales a source span of RGBA pixels to a destination span of
 * a different length. It supports a multiplier color and direction control.
 * After the operation, it calls evas_common_cpu_end_opt().
 *
 * @param src Pointer to the source RGBA pixel data.
 * @param mask Unused in the underlying function.
 * @param src_len Length of the source pixel span.
 * @param mul_col Multiplier color in 0xAARRGGBB format.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling.
 * @see evas_common_scale_rgba_span_()
 */
EVAS_API void
evas_common_scale_rgba_span(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   evas_common_scale_rgba_span_(src, mask, src_len, mul_col, dst, dst_len, dir);
   evas_common_cpu_end_opt();
}

/**
 * @brief Scales a span of RGBA pixels with an alpha mask.
 *
 * This is a public API wrapper for evas_common_scale_rgba_a8_span_().
 * It scales a source span of RGBA pixels, applying an alpha mask
 * to each source pixel before scaling. It also supports a multiplier color
 * and direction control.
 * After the operation, it calls evas_common_cpu_end_opt().
 *
 * @param src Pointer to the source RGBA pixel data.
 * @param mask Pointer to the alpha mask data.
 * @param src_len Length of the source pixel span and mask.
 * @param mul_col Multiplier color in 0xAARRGGBB format.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling.
 * @see evas_common_scale_rgba_a8_span_()
 */
EVAS_API void
evas_common_scale_rgba_a8_span(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   evas_common_scale_rgba_a8_span_(src, mask, src_len, mul_col, dst, dst_len, dir);
   evas_common_cpu_end_opt();
}

/**
 * @brief Scales a span of alpha mask values, applying a multiplier color.
 *
 * This is a public API wrapper for evas_common_scale_a8_span_().
 * It scales a source span of alpha mask values. Each scaled alpha
 * value is then used to modulate a multiplier color, and the result is
 * written to the destination.
 * After the operation, it calls evas_common_cpu_end_opt().
 *
 * @param src Unused in the underlying function.
 * @param mask Pointer to the source alpha mask data.
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color in 0xAARRGGBB format.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling.
 * @see evas_common_scale_a8_span_()
 */
EVAS_API void
evas_common_scale_a8_span(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   evas_common_scale_a8_span_(src, mask, src_len, mul_col, dst, dst_len, dir);
   evas_common_cpu_end_opt();
}

/**
 * @brief Scales an alpha mask and uses it to modulate existing destination pixels.
 *
 * This is a public API wrapper for evas_common_scale_clip_a8_span_().
 * It scales a source span of alpha mask values. The scaled alpha
 * is then used to modulate the existing pixels in the `dst` buffer.
 * After the operation, it calls evas_common_cpu_end_opt().
 *
 * @param src Unused in the underlying function.
 * @param mask Pointer to the source alpha mask data.
 * @param src_len Length of the source alpha mask span.
 * @param mul_col Multiplier color in 0xAARRGGBB format.
 * @param dst Pointer to the destination RGBA pixel data array (read and modified).
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of processing.
 * @see evas_common_scale_clip_a8_span_()
 */
EVAS_API void
evas_common_scale_clip_a8_span(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   evas_common_scale_clip_a8_span_(src, mask, src_len, mul_col, dst, dst_len, dir);
   evas_common_cpu_end_opt();
}

/**
 * @brief Scales a span of RGBA pixels, performing interpolation in HSV(A) color space.
 *
 * This function scales a source span of RGBA pixels to a destination span.
 * Unlike evas_common_scale_rgba_span(), color interpolation between source
 * pixels is performed in the HSV (Hue, Saturation, Value) color space.
 * The Alpha component is interpolated linearly.
 * It supports a multiplier color applied after HSV->RGB conversion and direction control.
 *
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 *            These are converted to HSV for interpolation.
 * @param mask Unused in this function.
 * @param src_len Length of the source pixel span.
 * @param mul_col Multiplier color in 0xAARRGGBB format. Applied after HSV interpolation
 *                and conversion back to RGB.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling. If < 0, scaling is done in reverse.
 */
EVAS_API void
evas_common_scale_hsva_span(DATA32 *src, DATA8 *mask EINA_UNUSED, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int  mul = 0, step = 1;
   DATA32 *pdst = dst;

   if (!src || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (mul_col != 0xffffffff)
	mul = 1;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

   if ((src_len == 1) || (dst_len == 1))
     {
	DATA32 c = *src;

	if (mul) c = MUL4_SYM(mul_col, c);
	while (dst_len--)
	   *dst++ = c;
	return;
     }

   if (src_len == dst_len)
     {
	if (mul)
	  {
	    while (dst_len--)
	      {
		*pdst = MUL4_SYM(mul_col, *src);
		src++;  pdst += step;
	      }
	    return;
	  }
	while (dst_len--)
	  {
	    *pdst = *src;
	    src++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

	while (dst_len--)
	  {
	    DATA32   p2, p1 = 0;
	    int      a, h1, s1, v1, h2, s2, v2;

	    sx = (sxx >> 16);
	    if (sx < src_len)
		p1 = *(src + sx);
	    evas_common_convert_color_rgb_to_hsv_int((p1 >> 16) & 0xff, (p1 >> 8) & 0xff, p1 & 0xff,
						     &h1, &s1, &v1);
	    p2 = p1;
	    if ((sx + 1) < src_len)
	        p2 = *(src + sx + 1);
	    evas_common_convert_color_rgb_to_hsv_int((p2 >> 16) & 0xff, (p2 >> 8) & 0xff, p2 & 0xff,
						     &h2, &s2, &v2);
	    a = 1 + ((sxx - (sx << 16)) >> 8);
	    h1 += (a * (h2 - h1)) >> 8;
	    s1 += (a * (s2 - s1)) >> 8;
	    v1 += (a * (v2 - v1)) >> 8;
	    a = (((((p2 >> 8) & 0xff0000) - ((p1 >> 8) & 0xff0000)) * a) +
	         (p1 & 0xff000000)) & 0xff000000;
	    evas_common_convert_color_hsv_to_rgb_int(h1, s1, v1, &h2, &s2, &v2);
	    p1 = a + RGB_JOIN(h2,s2,v2);
	    if (mul)
		p1 = MUL4_SYM(mul_col, p1);
	    *pdst = p1;
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}

/**
 * @brief Scales a span of RGBA pixels with an alpha mask, performing interpolation in HSV(A) color space.
 *
 * This function scales a source span of RGBA pixels, applying an alpha mask
 * to each source pixel before scaling. Color interpolation between source
 * pixels is performed in the HSV (Hue, Saturation, Value) color space.
 * The Alpha component (original alpha multiplied by mask alpha) is interpolated linearly.
 * It supports a multiplier color applied after HSV->RGB conversion and direction control.
 *
 * @param src Pointer to the source RGBA pixel data (0xAARRGGBB).
 *            These are converted to HSV for interpolation.
 * @param mask Pointer to the alpha mask data (array of DATA8).
 *             Each DATA8 element is an alpha value (0-255) corresponding
 *             to a source pixel. This mask is applied to the source pixel's
 *             alpha before interpolation.
 * @param src_len Length of the source pixel span and mask.
 * @param mul_col Multiplier color in 0xAARRGGBB format. Applied after HSV interpolation
 *                and conversion back to RGB.
 * @param dst Pointer to the destination RGBA pixel data array.
 * @param dst_len Length of the destination pixel span.
 * @param dir Direction of scaling. If < 0, scaling is done in reverse.
 */
EVAS_API void
evas_common_scale_hsva_a8_span(DATA32 *src, DATA8 *mask, int src_len, DATA32 mul_col, DATA32 *dst, int dst_len, int dir)
{
   int  mul = 0, step = 1;
   DATA32 *pdst = dst;

   if (!src || !mask || !dst) return;
   if ((src_len < 1) || (dst_len < 1)) return;
   if ((src_len > SCALE_SIZE_MAX) || (dst_len > SCALE_SIZE_MAX)) return;
   if (mul_col != 0xffffffff)
	mul = 1;
   if (dir < 0)
     {
	pdst += dst_len - 1;
	step = -1;
     }

   if ((src_len == 1) || (dst_len == 1))
     {
	DATA32 c = MUL_SYM(*mask, *src);

	if (mul) c = MUL4_SYM(mul_col, c);
	while (dst_len--)
	   *dst++ = c;
	return;
     }

   if (src_len == dst_len)
     {
	if (mul)
	  {
	    while (dst_len--)
	      {
		DATA32  c = MUL_SYM(*mask, *src);
		*pdst = MUL4_SYM(mul_col, c);
		src++;  mask++;  pdst += step;
	      }
	    return;
	  }
	while (dst_len--)
	  {
	    *pdst = MUL_SYM(*mask, *src);
	    src++;  mask++;  pdst += step;
	  }
	return;
     }

     {
	DATA32  dsxx = (((src_len - 1) << 16) / (dst_len - 1));
	DATA32  sxx = 0;
	int     sx = sxx >> 16;

	while (dst_len--)
	  {
	    DATA32   p2, p1 = 0;
	    int      a, a2, a1 = 0;
	    int      h1, s1, v1, h2, s2, v2;

	    sx = (sxx >> 16);
	    if (sx < src_len)
	      {
		p1 = *(src + sx);
		a1 = *(mask + sx);
	      }
	    p2 = p1;  a2 = a1;
	    if ((sx + 1) < src_len)
	      {
		p2 = *(src + sx + 1);
		a2 = *(mask + sx + 1);
	      }
	    evas_common_convert_color_rgb_to_hsv_int((p1 >> 16) & 0xff, (p1 >> 8) & 0xff, p1 & 0xff,
						      &h1, &s1, &v1);
	    evas_common_convert_color_rgb_to_hsv_int((p2 >> 16) & 0xff, (p2 >> 8) & 0xff, p2 & 0xff,
						      &h2, &s2, &v2);
	    a = 1 + ((sxx - (sx << 16)) >> 8);
	    a1 += (a * (a2 - a1)) >> 8;
	    h1 += (a * (h2 - h1)) >> 8;
	    s1 += (a * (s2 - s1)) >> 8;
	    v1 += (a * (v2 - v1)) >> 8;
	    a = (((((p2 >> 8) & 0xff0000) - ((p1 >> 8) & 0xff0000)) * a) +
	         (p1 & 0xff000000)) & 0xff000000;

	    evas_common_convert_color_hsv_to_rgb_int(h1, s1, v1, &h2, &s2, &v2);
	    p1 = a + RGB_JOIN(h2,s2,v2);
	    p1 = MUL_SYM(a1, p1);
	    if (mul)
		p1 = MUL4_SYM(mul_col, p1);
	    *pdst = p1;
	    pdst += step;  sxx += dsxx;
	  }
	return;
     }
}
