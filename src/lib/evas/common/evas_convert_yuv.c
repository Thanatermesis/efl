#include "evas_common_private.h"
#include "evas_convert_yuv.h"

#ifdef BUILD_MMX
# include "evas_mmx.h"
#endif

#ifdef HAVE_ALTIVEC_H
# include <altivec.h>
#ifdef CONFIG_DARWIN
#define AVV(x...) (x)
#else
#define AVV(x...) {x}
#endif

#endif

/** @brief Initializes YUV conversion lookup tables. */
static void _evas_yuv_init         (void);
// Broken atm - the sse and mmx get math.. wrong :(
//static void _evas_yv12_709torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts YV12 (BT.601) to RGBA using SSE instructions. */
static void _evas_yv12torgb_sse    (unsigned char **yuv, unsigned char *rgb, int w, int h);
// Broken atm - the sse and mmx get math.. wrong :(
//static void _evas_yv12_709torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts YV12 (BT.601) to RGBA using MMX instructions. */
static void _evas_yv12torgb_mmx    (unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts YV12 (BT.709) to RGBA using a generic raster scan method. */
static void _evas_yv12_709torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts YV12 (BT.601) to RGBA using a generic raster scan method. */
static void _evas_yv12torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts YUY2 (BT.601) to RGBA using a generic raster scan method. */
static void _evas_yuy2torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts NV12 (BT.601) to RGBA using a generic raster scan method. */
static void _evas_nv12torgb_raster (unsigned char **yuv, unsigned char *rgb, int w, int h);
/** @brief Converts NV12 Tiled (BT.601) to RGBA using a generic raster scan method. */
static void _evas_nv12tiledtorgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h);

/** @name BT.601 YUV to RGB conversion coefficients (fixed-point) */
///@{
#define CRV    104595 ///< Coefficient for V to R conversion (1.596 * 2^16)
#define CBU    132251 ///< Coefficient for U to B conversion (2.018 * 2^16)
#define CGU    25624  ///< Coefficient for U to G conversion (0.391 * 2^16)
#define CGV    53280  ///< Coefficient for V to G conversion (0.813 * 2^16)

#define YMUL   76283  ///< Coefficient for Y scaling (1.164 * 2^16), applied to (Y - 16)
#define OFF    32768  ///< Offset for G calculation (0.5 * 2^16), used to round results
#define BITRES 16     ///< Number of bits for fixed-point precision in intermediate calculations
///@}

/** @name BT.709 YUV to RGB conversion coefficients (fixed-point) */
///@{
#define CRV709 117504 ///< Coefficient for V to R conversion (1.793 * 2^16)
#define CBU709 138607 ///< Coefficient for U to B conversion (2.115 * 2^16)
#define CGU709  13959 ///< Coefficient for U to G conversion (0.213 * 2^16)
#define CGV709  34996 ///< Coefficient for V to G conversion (0.534 * 2^16)
///@}


/* calculation float resolution in bits */
/* ie RES = 6 is 10.6 fixed point */
/*    RES = 8 is 8.8 fixed point */
/*    RES = 4 is 12.4 fixed point */
/* NB: going above 6 will lead to overflow... :( */
/** @brief Resolution shift for final color component values.
 *  This determines the number of fractional bits in the fixed-point arithmetic
 *  used for MMX/SSE optimized paths. A value of 6 means 10.6 fixed-point.
 */
#define RES    6

/** @brief Right-shifts an integer by (BITRES - RES) bits.
 *  Used to scale down fixed-point coefficients to the working resolution RES.
 */
#define RZ(i)  (i >> (BITRES - RES))
/** @brief Macro to initialize a 4-element array with the same value.
 *  Used for MMX/SSE constant initialization.
 */
#define FOUR(i) {i, i, i, i}

#ifdef BUILD_MMX
/** @name MMX/SSE constants for BT.601 conversion (scaled by RZ) */
///@{
__attribute__ ((aligned (8))) const volatile unsigned short _const_crvcrv[4] = FOUR(RZ(CRV)); ///< Scaled CRV for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cbucbu[4] = FOUR(RZ(CBU)); ///< Scaled CBU for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgucgu[4] = FOUR(RZ(CGU)); ///< Scaled CGU for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgvcgv[4] = FOUR(RZ(CGV)); ///< Scaled CGV for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_ymul  [4] = FOUR(RZ(YMUL)); ///< Scaled YMUL for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_128   [4] = FOUR(128);   ///< Constant 128 for MMX/SSE (U/V offset)
__attribute__ ((aligned (8))) const volatile unsigned short _const_32    [4] = FOUR(RZ(OFF)); ///< Scaled OFF for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_16    [4] = FOUR(16);    ///< Constant 16 for MMX/SSE (Y offset)
__attribute__ ((aligned (8))) const volatile unsigned short _const_ff    [4] = FOUR(-1);    ///< Constant 0xFFFF for MMX/SSE (alpha channel)
///@}

/** @name MMX/SSE constants for BT.709 conversion (scaled by RZ) */
///@{
__attribute__ ((aligned (8))) const volatile unsigned short _const_crvcrv709[4] = FOUR(RZ(CRV709)); ///< Scaled CRV709 for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cbucbu709[4] = FOUR(RZ(CBU709)); ///< Scaled CBU709 for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgucgu709[4] = FOUR(RZ(CGU709)); ///< Scaled CGU709 for MMX/SSE
__attribute__ ((aligned (8))) const volatile unsigned short _const_cgvcgv709[4] = FOUR(RZ(CGV709)); ///< Scaled CGV709 for MMX/SSE
///@}

/** @name MMX register load macros for BT.601 constants */
///@{
#define CONST_CRVCRV *_const_crvcrv
#define CONST_CBUCBU *_const_cbucbu
#define CONST_CGUCGU *_const_cgucgu
#define CONST_CGVCGV *_const_cgvcgv
#define CONST_YMUL   *_const_ymul
#define CONST_128    *_const_128
#define CONST_32     *_const_32
#define CONST_16     *_const_16
#define CONST_FF     *_const_ff
///@}

/** @name MMX register load macros for BT.709 constants */
///@{
#define CONST_CRVCRV709 *_const_crvcrv709
#define CONST_CBUCBU709 *_const_cbucbu709
#define CONST_CGUCGU709 *_const_cgucgu709
#define CONST_CGVCGV709 *_const_cgvcgv709
///@}

/* for C non aligned cleanup */
/** @name Scaled BT.601 coefficients for C raster implementation (scaled by RZ) */
///@{
const int _crv = RZ(CRV);   ///< Scaled CRV (1.596)
const int _cbu = RZ(CBU);   ///< Scaled CBU (2.018)
const int _cgu = RZ(CGU);   ///< Scaled CGU (0.391)
const int _cgv = RZ(CGV);   ///< Scaled CGV (0.813)
///@}

/** @name Scaled BT.709 coefficients for C raster implementation (scaled by RZ) */
///@{
const int _crv709 = RZ(CRV709);   ///< Scaled CRV709 (1.793)
const int _cbu709 = RZ(CBU709);   ///< Scaled CBU709 (2.115)
const int _cgu709 = RZ(CGU709);   ///< Scaled CGU709 (0.213)
const int _cgv709 = RZ(CGV709);   ///< Scaled CGV709 (0.534)
///@}

#endif

/* shortcut speedup lookup-tables */
/** @name Lookup tables for C raster implementations */
///@{
static short _v1164[256]; ///< LUT for (Y - 16) * 1.164
static short _v1596[256]; ///< LUT for (V - 128) * 1.596 (BT.601)
static short _v813[256];  ///< LUT for (V - 128) * 0.813 (BT.601)
static short _v391[256];  ///< LUT for (U - 128) * 0.391 (BT.601)
static short _v2018[256]; ///< LUT for (U - 128) * 2.018 (BT.601)

static short _v1793[256]; ///< LUT for (V - 128) * 1.793 (BT.709)
static short _v534[256];  ///< LUT for (V - 128) * 0.534 (BT.709)
static short _v213[256];  ///< LUT for (U - 128) * 0.213 (BT.709)
static short _v2115[256]; ///< LUT for (U - 128) * 2.115 (BT.709)

static unsigned char _clip_lut[1024]; ///< LUT for clipping values to 0-255 range. Indexed by value + 384.
///@}

/** @brief Clips a value to the 0-255 range using the _clip_lut.
 *  @param i The value to clip. The valid input range for `i` to correctly index `_clip_lut` is -384 to 639.
 */
#define LUT_CLIP(i) ((_clip_lut+384)[(i)])

/** @brief Alternative clipping macro (seems unused or for specific optimizations).
 *  Clips value `i`. If the 9th bit (256) is set, it implies a negative overflow
 *  (assuming a certain range for `i` before this check), and it calculates a
 *  clipped value. Otherwise, it assumes `i` is positive and within a certain range.
 *  This macro's logic is highly specific and might be tied to particular
 *  intermediate calculation ranges in an optimized routine.
 */
#define CMP_CLIP(i) ((i&256)? (~(i>>10)) : i);

static int initted = 0; ///< Flag to check if lookup tables have been initialized.

/**
 * @brief Converts YUV 4:2:2 planar (BT.709) to RGBA.
 * @copydetails evas_common_convert_yuv_422p_709_rgba
 */
void
evas_common_convert_yuv_422p_709_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
/* Broken atm - the sse and mmx get math.. wrong :(
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX2))
     _evas_yv12_709torgb_sse(src, dst, w, h);
   else if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     _evas_yv12_709torgb_mmx(src, dst, w, h);
   else
 */
     _evas_yv12_709torgb_raster(src, dst, w, h);
}


/**
 * @brief Converts YUV 4:2:2 planar (BT.601) to RGBA.
 * @copydetails evas_common_convert_yuv_422p_601_rgba
 */
void
evas_common_convert_yuv_422p_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   if (evas_common_cpu_has_feature(CPU_FEATURE_MMX2))
     _evas_yv12torgb_sse(src, dst, w, h);
   else if (evas_common_cpu_has_feature(CPU_FEATURE_MMX))
     _evas_yv12torgb_mmx(src, dst, w, h);
   else
     _evas_yv12torgb_raster(src, dst, w, h);
}

/* Thanks to Diz for this code. i've munged it a little and turned it into */
/* inline macros. I tried beating it with a different algorithm using MMX */
/* but failed. So here we are. This is the fastest YUV->RGB i know of for */
/* x86. It has an issue that it doesn't convert colours accurately so the */
/* image looks a little "yellowy". This is a result of only 10.6 fixed point */
/* resolution as opposed to 16.16 in the C code. This could be fixed by */
/* processing half the number of pixels per cycle and going up to 32bits */
/* per element during compute, but it would all but negate the speedup */
/* from mmx I think :( It might be possible to use SSE and SSE2 here, but */
/* I haven't tried yet. Let's see. */

/* NB: XviD has almost the same code in it's assembly YV12->RGB code. same */
/* algorithm, same constants, same all over actually, except it actually */
/* does a few extra memory accesses that this one doesn't, so in theory */
/* this code should be faster. In the end it's all just an mmx version of */
/* the reference implimentation done with fixed point math */

/*
static void
_evas_yv12_709torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
// This function is commented out in the original code with a note "Broken atm".
// If it were active, it would convert YV12 (planar Y, V, U) BT.709 to RGBA using SSE.
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 807); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV709, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU709, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU709, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV709, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movntq_r2m(mm1, *(dp1));
	     movntq_r2m(mm7, *(dp1 + 8));
	     movntq_r2m(mm4, *(dp1 + 16));
	     movntq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }

	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12_709torgb_mmx(yuv, rgb, w, h);
#endif
}
*/

/**
 * @internal
 * @brief Converts YV12 (planar Y, U, V) with BT.601 coefficients to RGBA using SSE instructions.
 *
 * This function processes 8 pixels (two 2x2 blocks of Y values with shared U/V) per iteration.
 * It uses MMX registers (mm0-mm7) for parallel operations.
 * The YUV data is expected in planar format:
 * - yuv[0] to yuv[h-1] point to rows of the Y plane.
 * - yuv[h] to yuv[h + h/2 - 1] point to rows of the U plane (subsampled).
 * - yuv[h + h/2] to yuv[h + h - 1] point to rows of the V plane (subsampled).
 *
 * The output `rgb` is an array of 32-bit RGBA pixels.
 *
 * @param yuv Array of pointers to Y, U, V planes.
 *            yuv[0..h-1] are Y plane lines.
 *            yuv[h..(h + h/2 - 1)] are U plane lines.
 *            yuv[h + h/2 .. (h + h/2 + h/2 - 1)] are V plane lines.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image.
 * @param h Height of the image.
 */
static void
_evas_yv12torgb_sse(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   /* destination pointers */
   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movntq_r2m(mm1, *(dp1));
	     movntq_r2m(mm7, *(dp1 + 8));
	     movntq_r2m(mm4, *(dp1 + 16));
	     movntq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	/* cleanup pixles that arent a multiple of 8 pixels wide */
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_mmx(yuv, rgb, w, h);
#endif
}

/*
static void
_evas_yv12_709torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
// This function is commented out in the original code with a note "Broken atm".
// If it were active, it would convert YV12 (planar Y, V, U) BT.709 to RGBA using MMX.
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV709, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU709, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU709, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV709, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movq_r2m(mm1, *(dp1));
	     movq_r2m(mm7, *(dp1 + 8));
	     movq_r2m(mm4, *(dp1 + 16));
	     movq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv709 * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu709 * u) - (_cgv709 * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu709 * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_raster(yuv, rgb, w, h);
#endif
}
*/

/**
 * @internal
 * @brief Converts YV12 (planar Y, U, V) with BT.601 coefficients to RGBA using MMX instructions.
 *
 * This function is similar in logic to `_evas_yv12torgb_sse` but uses MMX instructions.
 * It processes 8 pixels per iteration.
 * The YUV data is expected in planar format as described for `_evas_yv12torgb_sse`.
 *
 * @param yuv Array of pointers to Y, U, V planes.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image.
 * @param h Height of the image.
 */
static void
_evas_yv12torgb_mmx(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
#ifdef BUILD_MMX
   int xx, yy;
   register unsigned char *yp1, *up, *vp;
   unsigned char *dp1;

   /* destination pointers */
   dp1 = rgb;

   for (yy = 0; yy < h; yy++)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < (w - 7); xx += 8)
	  {
	     movd_m2r(*up, mm3);
	     movd_m2r(*vp, mm2);
	     movq_m2r(*yp1, mm0);

	     pxor_r2r(mm7, mm7);
	     punpcklbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm7, mm3);

	     movq_r2r(mm0, mm1);
	     psrlw_i2r(8, mm0);
	     psllw_i2r(8, mm1);
	     psrlw_i2r(8, mm1);

	     movq_m2r(CONST_16, mm4);
	     psubsw_r2r(mm4, mm0);
	     psubsw_r2r(mm4, mm1);

	     movq_m2r(CONST_128, mm5);
	     psubsw_r2r(mm5, mm2);
	     psubsw_r2r(mm5, mm3);

	     movq_m2r(CONST_YMUL, mm4);
	     pmullw_r2r(mm4, mm0);
	     pmullw_r2r(mm4, mm1);

	     movq_m2r(CONST_CRVCRV, mm7);
	     pmullw_r2r(mm3, mm7);
	     movq_m2r(CONST_CBUCBU, mm6);
	     pmullw_r2r(mm2, mm6);
	     movq_m2r(CONST_CGUCGU, mm5);
	     pmullw_r2r(mm2, mm5);
	     movq_m2r(CONST_CGVCGV, mm4);
	     pmullw_r2r(mm3, mm4);

	     movq_r2r(mm0, mm2);
	     paddsw_r2r(mm7, mm2);
	     paddsw_r2r(mm1, mm7);

	     psraw_i2r(RES, mm2);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm2);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm2, mm3);
	     punpckhbw_r2r(mm7, mm2);
	     punpcklbw_r2r(mm3, mm7);
	     por_r2r(mm7, mm2);

	     movq_r2r(mm0, mm3);
	     psubsw_r2r(mm5, mm3);
	     psubsw_r2r(mm4, mm3);
	     paddsw_m2r(CONST_32, mm3);

	     movq_r2r(mm1, mm7);
	     psubsw_r2r(mm5, mm7);
	     psubsw_r2r(mm4, mm7);
	     paddsw_m2r(CONST_32, mm7);

	     psraw_i2r(RES, mm3);
	     psraw_i2r(RES, mm7);
	     packuswb_r2r(mm7, mm3);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm3, mm4);
	     punpckhbw_r2r(mm7, mm3);
	     punpcklbw_r2r(mm4, mm7);
	     por_r2r(mm7, mm3);

	     movq_m2r(CONST_32, mm4);
	     paddsw_r2r(mm6, mm0);
	     paddsw_r2r(mm6, mm1);
	     paddsw_r2r(mm4, mm0);
	     paddsw_r2r(mm4, mm1);
	     psraw_i2r(RES, mm0);
	     psraw_i2r(RES, mm1);
	     packuswb_r2r(mm1, mm0);

	     pxor_r2r(mm7, mm7);
	     movq_r2r(mm0, mm5);
	     punpckhbw_r2r(mm7, mm0);
	     punpcklbw_r2r(mm5, mm7);
	     por_r2r(mm7, mm0);

	     movq_m2r(CONST_FF, mm1);
	     movq_r2r(mm0, mm5);
	     movq_r2r(mm3, mm6);
	     movq_r2r(mm2, mm7);
	     punpckhbw_r2r(mm3, mm2);
	     punpcklbw_r2r(mm6, mm7);
	     punpckhbw_r2r(mm1, mm0);
	     punpcklbw_r2r(mm1, mm5);

	     movq_r2r(mm7, mm1);
	     punpckhwd_r2r(mm5, mm7);
	     punpcklwd_r2r(mm5, mm1);

	     movq_r2r(mm2, mm4);
	     punpckhwd_r2r(mm0, mm2);
	     punpcklwd_r2r(mm0, mm4);

	     movq_r2m(mm1, *(dp1));
	     movq_r2m(mm7, *(dp1 + 8));
	     movq_r2m(mm4, *(dp1 + 16));
	     movq_r2m(mm2, *(dp1 + 24));

	     yp1 += 8;
	     up += 4;
	     vp += 4;
	     dp1 += 8 * 4;
	  }
	/* cleanup pixles that arent a multiple of 8 pixels wide */
	if (xx < w)
	  {
	     int y, u, v, r, g, b;

	     for (; xx < w; xx += 2)
	       {
		  u = (*up++) - 128;
		  v = (*vp++) - 128;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;

		  y = RZ(YMUL) * ((*yp1++) - 16);
		  r = LUT_CLIP((y + (_crv * v)) >> RES);
		  g = LUT_CLIP((y - (_cgu * u) - (_cgv * v) + RZ(OFF)) >> RES);
		  b = LUT_CLIP((y + (_cbu * u) + RZ(OFF)) >> RES);
		  *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(r,g,b);

		  dp1 += 4;
	       }
	  }
     }
   emms();
#else
   _evas_yv12torgb_raster(yuv, rgb, w, h);
#endif
}

/**
 * @internal
 * @brief Initializes lookup tables used for C-based YUV to RGB conversion.
 *
 * This function precomputes values for various YUV conversion formulas
 * to speed up the raster conversion functions. It populates:
 * - `_v1164`: (Y - 16) * 1.164
 * - `_v1596`, `_v813`, `_v391`, `_v2018`: Coefficients for BT.601 U/V terms.
 * - `_v1793`, `_v534`, `_v213`, `_v2115`: Coefficients for BT.709 U/V terms.
 * - `_clip_lut`: A lookup table for clipping color component values to the 0-255 range.
 *
 * This function is called once when the first YUV conversion is requested.
 */
static void
_evas_yuv_init(void)
{
   int i;

   for (i = 0; i < 256; i++)
     {
	_v1164[i] = (int)(((float)(i - 16 )) * 1.164);

	_v1596[i] = (int)(((float)(i - 128)) * 1.596);
	_v813[i]  = (int)(((float)(i - 128)) * 0.813);

	_v391[i]  = (int)(((float)(i - 128)) * 0.391);
	_v2018[i] = (int)(((float)(i - 128)) * 2.018);

//////////////////////////////////////////////////////////////////////////
        _v1793[i] = (int)(((float)(i - 128)) * 1.793);
        _v534[i]  = (int)(((float)(i - 128)) * 0.534);
        _v213[i]  = (int)(((float)(i - 128)) * 0.213);
        _v2115[i] = (int)(((float)(i - 128)) * 2.115);
     }

   for (i = -384; i < 640; i++)
     {
	_clip_lut[i+384] = i < 0 ? 0 : (i > 255) ? 255 : i;
     }
}

/**
 * @internal
 * @brief Converts YV12 (planar Y, U, V) with BT.709 coefficients to RGBA using a C raster implementation.
 *
 * This function processes a 2x2 block of pixels at a time, sharing U and V values.
 * It uses precomputed lookup tables (`_v1164`, `_v1793`, etc.) for efficiency.
 * YUV data format is planar:
 * - yuv[0] to yuv[h-1] are Y plane rows.
 * - yuv[h] to yuv[h + h/2 - 1] are U plane rows.
 * - yuv[h + h/2] to yuv[h + h - 1] are V plane rows.
 *
 * @param yuv Array of pointers to Y, U, V planes.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image.
 * @param h Height of the image.
 */
static void
_evas_yv12_709torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1, *dp2;

   /* destination pointers */
   dp1 = rgb;
   dp2 = rgb + (w * 4);

   for (yy = 0; yy < h; yy += 2)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	yp2 = yuv[yy + 1];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < w; xx += 2)
	  {
	     int vmu;

	     /* collect u & v for 2x2 pixel block */
	     u = *up++;
	     v = *vp++;

	     /* save lookups */
	     vmu = _v534[v] + _v213[u];
	     u = _v2115[u];
	     v = _v1793[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* do the bottom 2 pixels */
	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;
	  }
	/* jump down one line since we are doing 2 at once */
	dp1 += (w * 4);
	dp2 += (w * 4);
     }
}

/**
 * @internal
 * @brief Converts YV12 (planar Y, U, V) with BT.601 coefficients to RGBA using a C raster implementation.
 *
 * This function is analogous to `_evas_yv12_709torgb_raster` but uses BT.601
 * conversion coefficients and corresponding lookup tables (`_v1596`, etc.).
 *
 * @param yuv Array of pointers to Y, U, V planes.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image.
 * @param h Height of the image.
 */
static void
_evas_yv12torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1, *dp2;

   /* destination pointers */
   dp1 = rgb;
   dp2 = rgb + (w * 4);

   for (yy = 0; yy < h; yy += 2)
     {
	/* plane pointers */
	yp1 = yuv[yy];
	yp2 = yuv[yy + 1];
	up = yuv[h + (yy / 2)];
	vp = yuv[h + (h / 2) + (yy / 2)];
	for (xx = 0; xx < w; xx += 2)
	  {
	     int vmu;

	     /* collect u & v for 2x2 pixel block */
	     u = *up++;
	     v = *vp++;

	     /* save lookups */
	     vmu = _v813[v] + _v391[u];
	     u = _v2018[u];
	     v = _v1596[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp1++];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* do the bottom 2 pixels */
	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2++];
	     *((DATA32 *) dp2) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp2 += 4;
	  }
	/* jump down one line since we are doing 2 at once */
	dp1 += (w * 4);
	dp2 += (w * 4);
     }
}

/**
 * @brief Converts YUV 4:2:2 interleaved (BT.601) to RGBA.
 * @copydetails evas_common_convert_yuv_422_601_rgba
 */
void
evas_common_convert_yuv_422_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   _evas_yuy2torgb_raster(src, dst, w, h);
}

/**
 * @brief Converts YUV 4:2:0 planar (NV12 like, BT.601) to RGBA.
 * @copydetails evas_common_convert_yuv_420_601_rgba
 */
void
evas_common_convert_yuv_420_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (!initted) _evas_yuv_init();
   initted = 1;
   _evas_nv12torgb_raster(src, dst, w, h);
}

/**
 * @brief Converts YUV 4:2:0 Tiled (BT.601) to RGBA.
 * @copydetails evas_common_convert_yuv_420T_601_rgba
 * @note The check `if (initted)` seems like a typo and probably should be `if (!initted)`.
 */
void
evas_common_convert_yuv_420T_601_rgba(DATA8 **src, DATA8 *dst, int w, int h)
{
   if (initted) _evas_yuv_init();
   initted = 1;
   _evas_nv12tiledtorgb_raster(src, dst, w, h);
}

/**
 * @internal
 * @brief Converts YUY2/YUYV-like (interleaved YUV 4:2:2, BT.601) to RGBA using a C raster implementation.
 *
 * Processes YUV data where components are interleaved in the pattern [Y0, U0, Y1, V0] for every two pixels.
 * It uses precomputed lookup tables for efficiency.
 *
 * @param yuv Array of pointers to rows of interleaved YUV data. Each row `yuv[yy]`
 *            is `w * 2` bytes long. For example: `[Y0,U0,Y1,V0, Y2,U1,Y3,V1, ...]`.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 */
static void
_evas_yuy2torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   int y, u, v;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1;

   dp1 = rgb;

   /* destination pointers */
   for (yy = 0; yy < h; yy++)
     {
        /* plane pointers */
        unsigned char *line;

        line = yuv[yy];
        yp1 = line + 0;
        up = line + 1;
        yp2 = line + 2;
        vp = line + 3;

        for (xx = 0; xx < w; xx += 2)
          {
             int vmu;

             /* collect u & v for 2 pixels block */
             u = *up;
             v = *vp;

             /* save lookups */
             vmu = _v813[v] + _v391[u];
             u = _v2018[u];
             v = _v1596[v];

             /* do the top 2 pixels of the 2x2 block which shared u & v */
	     /* yuv to rgb */
	     y = _v1164[*yp1];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

	     dp1 += 4;

	     /* yuv to rgb */
	     y = _v1164[*yp2];
	     *((DATA32 *) dp1) = 0xff000000 + RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));

             dp1 += 4;

	     yp1 += 4; yp2 += 4; up += 4; vp += 4;
	  }
     }
}

/**
 * @internal
 * @brief Converts a 2x2 block of YUV420 data to RGBA pixels using C raster operations.
 *
 * This inline function is a helper for NV12 and NV12 Tiled conversions.
 * It takes pointers to Y values for two adjacent pixels in two consecutive lines,
 * and shared U and V values for this 2x2 block. It outputs four RGBA pixels.
 *
 * It can use either lookup tables (MEM_BP defined) or direct calculation for conversion.
 *
 * @param yp1 Pointer to the first Y value in the first line of the 2x2 block.
 * @param yp2 Pointer to the first Y value in the second line of the 2x2 block.
 * @param up Pointer to the U value (shared for the 2x2 block).
 * @param vp Pointer to the V value (shared for the 2x2 block).
 * @param dp1 Pointer to the output RGBA buffer for the first line of the 2x2 block.
 * @param dp2 Pointer to the output RGBA buffer for the second line of the 2x2 block.
 */
static inline void
_evas_yuv2rgb_420_raster(unsigned char *yp1, unsigned char *yp2, unsigned char *up, unsigned char *vp,
                         unsigned char *dp1, unsigned char *dp2)
{
   int y, u, v;
   int vmu;
   int rgb;

   /* collect u & v for 4 pixels block */
   u = *up;
   v = *vp;

   /* save lookups */
#ifdef MEM_BP
   vmu = _v813[v] + _v391[u];
   u = _v2018[u];
   v = _v1596[v];
#else
   u -= 128;
   v -= 128;
   vmu = v * CGV + u * CGU;
   u = u * CBU;
   v = v * CRV;
#endif

   /* do the top 2 pixels of the 2x2 block which shared u & v */
   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp1];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp1 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp1) = 0xff000000 + rgb;

   dp1 += 4; yp1++;

   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp1];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp1 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp1) = 0xff000000 + rgb;

   /* do the bottom 2 pixels of the 2x2 block which shared u & v */
   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp2];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp2 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp2) = 0xff000000 + rgb;

   dp2 += 4; yp2++;

   /* yuv to rgb */
#ifdef MEM_BP
   y = _v1164[*yp2];
   rgb = RGB_JOIN(LUT_CLIP(y + v), LUT_CLIP(y - vmu), LUT_CLIP(y + u));
#else
   y = (*yp2 - 16 ) * YMUL;
   rgb = RGB_JOIN(LUT_CLIP(((y + v) >> 16)),
                  LUT_CLIP(((y - vmu + OFF) >> 16)),
                  LUT_CLIP(((y + u + OFF) >> 16)));
#endif
   *((DATA32 *) dp2) = 0xff000000 + rgb;
}

/**
 * @internal
 * @brief Converts a proprietary tiled YUV 4:2:0 format (BT.601) to RGBA using a C raster implementation.
 *
 * This function handles a specific tiled memory layout where Y and UV data are stored in macroblocks.
 * The exact tiling scheme (Z-order or Morton order variation) is complex and described by the iteration logic.
 * It iterates over macroblocks, untiles them, and converts them using `_evas_yuv2rgb_420_raster`.
 *
 * The `src` array structure is specific to this tiled format:
 * - `yuv[0]` to `yuv[base_h-1]` (approximately) point to starts of rows of Y macroblocks.
 * - `yuv[base_h]` onwards point to starts of rows of UV macroblocks.
 * The exact indexing `yuv[mb_y]`, `yuv[(mb_y >> 1) + base_h]` reflects how macroblock rows are accessed.
 *
 * @param yuv Array of pointers to tiled Y and UV data. Structure is specific to the tiled format.
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 */
static void
_evas_nv12tiledtorgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
/**
 * @internal
 * @brief Macro to process one 64x32 Y macroblock and corresponding 64x16 UV macroblock.
 *
 * This macro iterates 32 times (for 32 lines in Y, 16 lines in UV).
 * In each iteration, it processes 64 pixels wide (32 pairs of 2 pixels).
 * It calls `_evas_yuv2rgb_420_raster` to convert 2x2 YUV blocks to RGBA.
 *
 * @param YP1 Pointer to the current Y data in the first line of the current 2-line strip.
 * @param YP2 Pointer to the current Y data in the second line of the current 2-line strip.
 * @param UP Pointer to the current U data (interleaved with V).
 * @param VP Pointer to the current V data (interleaved with U, typically UP+1).
 * @param DP1 Pointer to the destination RGBA buffer for the first line.
 * @param DP2 Pointer to the destination RGBA buffer for the second line.
 */
#define HANDLE_MACROBLOCK(YP1, YP2, UP, VP, DP1, DP2)                   \
   {                                                                    \
     int i;                                                             \
     int j;                                                             \
                                                                        \
     for (i = 0; i < 32; i += 2)                                        \
       {                                                                \
          for (j = 0; j < 64; j += 2)                                   \
            {                                                           \
               _evas_yuv2rgb_420_raster(YP1, YP2, UP, VP, DP1, DP2);    \
                                                                        \
               /* the previous call just rendered 2 pixels per lines */ \
               DP1 += 8; DP2 += 8;                                      \
                                                                        \
               /* and took for that 2 lines with 2 Y, 1 U and 1 V. Don't forget U & V are in the same plane */ \
               YP1 += 2; YP2 += 2; UP += 2; VP += 2;                    \
            }                                                           \
                                                                        \
          DP1 += sizeof (int) * ((w << 1) - 64);			\
          DP2 += sizeof (int) * ((w << 1) - 64);			\
          YP1 += 64;                                                    \
          YP2 += 64;                                                    \
       }                                                                \
   }

   /* One macro block is 32 lines of Y and 16 lines of UV */
   const int offset_value[2] = { 0, 64 * 16 };
   int mb_x, mb_y, mb_w, mb_h;
   int base_h;
   int uv_x, uv_step;
   int stride;

   /* Idea iterate over each macroblock and convert each of them using _evas_nv12torgb_raster */

   /* The layout of the Y macroblock order in RGB non tiled space : */
   /* --------------------------------------------------- */
   /* | 0  | 1  | 6  | 7  | 8  | 9  | 14 | 15 | 16 | 17 | */
   /* --------------------------------------------------- */
   /* | 2  | 3  | 4  | 5  | 10 | 11 | 12 | 13 | 18 | 19 | */
   /* --------------------------------------------------- */
   /* | 20 | 21 | 26 | 27 | 28 | 29 | 34 | 35 | 36 | 37 | */
   /* --------------------------------------------------- */
   /* | 22 | 23 | 24 | 25 | 30 | 31 | 32 | 33 | 38 | 39 | */
   /* --------------------------------------------------- */
   /* | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | */
   /* --------------------------------------------------- */
   /* The layout of the UV macroblock order in the same RGB non tiled space : */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 0  - 1  - 6  - 7  - 8  - 9  - 14 - 15 - 16 - 17 - */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 2  - 3  - 4  - 5  - 10 - 11 - 12 - 13 - 18 - 19 - */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* --------------------------------------------------- */
   /* |    |    |    |    |    |    |    |    |    |    | */
   /* - 20 - 21 - 22 - 22 - 23 - 24 - 25 - 26 - 27 - 28 - */

   /* the number of macroblock should be a multiple of 64x32 */
   mb_w = w / 64;
   mb_h = h / 32;

   base_h = (mb_h >> 1) + (mb_h & 0x1);
   stride = w * sizeof (int);

   uv_x = 0;

   /* In this format we linearize macroblock on two line to form a Z and it's invert */
   for (mb_y = 0; mb_y < (mb_h >> 1); mb_y++)
     {
        int step = 2;
        int offset = 0;
        int x = 0;
	int rmb_x = 0;
	int ry[2];

	ry[0] = mb_y * 2 * 32 * stride;
	ry[1] = ry[0] + 32 * stride;

	uv_step = (mb_y & 0x1) == 0 ? 4 : 0;
	uv_x = (mb_y & 0x1) == 0 ? 0 : 2 * 64 * 32;

	for (mb_x = 0; mb_x < mb_w * 2; mb_x++, rmb_x += 64 * 32)
	  {
	    unsigned char *yp1, *yp2, *up, *vp;
	    unsigned char *dp1, *dp2;

	    dp1 = rgb + x + ry[offset];
	    dp2 = dp1 + stride;

	    yp1 = yuv[mb_y] + rmb_x;
	    yp2 = yp1 + 64;

	    /* UV plane is two time less bigger in pixel count, but it old two bytes each times */
	    up = yuv[(mb_y >> 1) + base_h] + uv_x + offset_value[offset];
	    vp = up + 1;

	    HANDLE_MACROBLOCK(yp1, yp2, up, vp, dp1, dp2);

	    step++;
	    if ((step & 0x3) == 0)
	      {
		offset = 1 - offset;
		x -= 64 * sizeof (int);
		uv_x -= 64 * 32;
	      }
	    else
	      {
		x += 64 * sizeof (int);
		uv_x += 64 * 32;
	      }

	    uv_step++;
	    if (uv_step == 8)
	      {
		uv_step = 0;
		uv_x += 4 * 64 * 32;
	      }
	  }
     }

   if (mb_h & 0x1)
     {
        int x = 0;
	int ry;

	ry = mb_y << 1;

	uv_step = 0;
	uv_x = 0;

        for (mb_x = 0; mb_x < mb_w; mb_x++, x++, uv_x++)
          {
             unsigned char *yp1, *yp2, *up, *vp;
             unsigned char *dp1, *dp2;

             dp1 = rgb + (x * 64 + (ry * 32 * w)) * sizeof (int);
             dp2 = dp1 + sizeof (int) * w;

             yp1 = yuv[mb_y] + mb_x * 64 * 32;
             yp2 = yp1 + 64;

             up = yuv[mb_y / 2 + base_h] + uv_x * 64 * 32;
             vp = up + 1;

             HANDLE_MACROBLOCK(yp1, yp2, up, vp, dp1, dp2);
          }
     }
}

/**
 * @internal
 * @brief Converts NV12/NV21 (planar Y, interleaved UV, BT.601) to RGBA using a C raster implementation.
 *
 * NV12 format consists of a full-resolution Y plane followed by a half-resolution
 * plane with interleaved U and V components (U0V0U1V1...). NV21 is similar but with V and U swapped (V0U0V1U1...).
 * This function expects `src` to provide pointers to rows of the Y plane,
 * and then pointers to rows of the UV plane.
 * `yuv[h + (yy >> 1)]` accesses the UV plane row corresponding to Y rows `yy` and `yy+1`.
 *
 * @param yuv Array of pointers.
 *            `yuv[0...h-1]` point to rows of the Y plane.
 *            `yuv[h...h + h/2 - 1]` point to rows of the interleaved UV plane.
 *            For NV12, UV plane is U0,V0,U1,V1...
 *            For NV21, UV plane is V0,U0,V1,U1... (this function treats `up` as U and `vp` as V, so expects NV12)
 * @param rgb Output buffer for RGBA data.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 */
static void
_evas_nv12torgb_raster(unsigned char **yuv, unsigned char *rgb, int w, int h)
{
   int xx, yy;
   unsigned char *yp1, *yp2, *up, *vp;
   unsigned char *dp1;
   unsigned char *dp2;
   int stride = sizeof(DATA32) * w;

   dp1 = rgb;
   dp2 = dp1 + stride;

   for (yy = 0; yy < h; yy++)
     {
        yp1 = yuv[yy++];
        yp2 = yuv[yy];

        up = yuv[h + (yy >> 1)];
        vp = up + 1;

        for (xx = 0; xx < w; xx += 2)
          {
             _evas_yuv2rgb_420_raster(yp1, yp2, up, vp, dp1, dp2);

             /* the previous call just rendered 2 pixels per lines */
             dp1 += 8; dp2 += 8;

             /* and took for that 2 lines with 2 Y, 1 U and 1 V. Don't forget U & V are in the same plane */
             yp1 += 2; yp2 += 2; up += 2; vp += 2;
          }

        /* jump one line */
        dp1 += stride;
        dp2 += stride;
     }
}

