/**
 * @file
 * @brief Functions for converting between different image colorspaces.
 *
 * This file implements various functions to convert image data from one
 * colorspace format to another, such as ARGB8888, RGB565_A5P, AGRY88,
 * GRY8, and YUV formats.
 */
#include "evas_common_private.h"
#include "evas_convert_colorspace.h"

#define CONVERT_RGB_565_TO_RGB_888(s) \
	(((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7)) | \
	 ((((s) << 5) & 0xfc00) | (((s) >> 1) & 0x300)) | \
	 ((((s) << 8) & 0xf80000) | (((s) << 3) & 0x70000)))

#define CONVERT_A5P_TO_A8(s) \
	((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7))

#define CONVERT_ARGB_8888_TO_A_8(s)	((s) >> 24)

/**
 * @internal
 * @brief Converts ARGB8888 image data to RGB565_A5P format.
 * @param data Pointer to the source image data (ARGB8888).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the source image has an alpha channel, EINA_FALSE otherwise.
 * @return Pointer to the converted image data (RGB565_A5P), or NULL on failure.
 * @note Currently not implemented, returns NULL.
 */
static inline void *
evas_common_convert_argb8888_to_rgb565_a5p(void *data EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int stride EINA_UNUSED, Eina_Bool has_alpha EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Converts RGB565_A5P image data to ARGB8888 format.
 * @param data Pointer to the source image data (RGB565). Alpha data is expected to follow the RGB data.
 *             The layout is [R5G6B5 R5G6B5 ... R5G6B5] [A5P A5P ... A5P].
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source RGB565 image data in DATA16 units (not bytes).
 * @param has_alpha EINA_TRUE if the source image has an alpha plane, EINA_FALSE otherwise.
 * @return Pointer to the converted image data (ARGB8888), or NULL on allocation failure.
 *         The returned data is a single block of ARGB8888 pixels.
 */
static inline void *
evas_common_convert_rgb565_a5p_to_argb8888(void *data, int w, int h, int stride, Eina_Bool has_alpha)
{
   DATA16 *src, *end;
   DATA32 *ret, *dst;

   src = data;
   end = src + (stride * h);
   ret = malloc(w * h * sizeof(DATA32));
   if (!ret) return NULL;

   dst = ret;
   if (has_alpha)
     {
	DATA8 *alpha;

	alpha = (DATA8 *)end;
	for (; src < end; src++, alpha++, dst++)
	  *dst = (CONVERT_A5P_TO_A8(*alpha) << 24) |
		  CONVERT_RGB_565_TO_RGB_888(*src);
     }
   else
     {
	for (; src < end; src++, dst++)
	  *dst = CONVERT_RGB_565_TO_RGB_888(*src);
     }
   return ret;
}

/**
 * @internal
 * @brief Converts AGRY88 (Alpha Grayscale, 8 bits alpha, 8 bits gray) image data to ARGB8888 format.
 * @param data Pointer to the source image data (AGRY88). Each pixel is 16 bits: [A7-A0 G7-G0].
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the source image uses its alpha channel, EINA_FALSE for opaque gray.
 * @return Pointer to the converted image data (ARGB8888), or NULL on allocation failure.
 *         In the output, R, G, and B components are set to the gray value.
 */
static inline void *
evas_common_convert_agry88_to_argb8888(const void *data, int w, int h, int stride, Eina_Bool has_alpha)
{
   const DATA16 *src, *end;
   DATA32 *ret, *dst;

   src = data;
   end = src + ((stride >> 1) * h);
   ret = malloc(w * h * sizeof(DATA32));
   if (!ret) return NULL;
   dst = ret;

   if (has_alpha)
     {
        for (; src < end; src++, dst++)
          {
             int c = (*src) & 0xFF;
             *dst = ARGB_JOIN((*src >> 8), c, c, c);
          }
     }
   else
     {
        for (; src < end; src++, dst++)
          {
             int c = (*src) & 0xFF;
             *dst = ARGB_JOIN(0xFF, c, c, c);
          }
     }

   return ret;
}

/**
 * @brief Converts AGRY88 image data to a specified colorspace.
 * @param data Pointer to the source image data (AGRY88).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the source image uses its alpha channel.
 * @param cspace The target Evas_Colorspace.
 * @return Pointer to the converted image data, or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_agry88_to_argb8888()
 */
void *
evas_common_convert_agry88_to(const void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace)
{
   switch (cspace) {
      case EVAS_COLORSPACE_ARGB8888:
        return evas_common_convert_agry88_to_argb8888(data, w, h, stride, has_alpha);
      default:
        return NULL;
     }
}

/**
 * @internal
 * @brief Converts GRY8 (Grayscale, 8 bits) image data to ARGB8888 format.
 * @param data Pointer to the source image data (GRY8). Each pixel is 8 bits [G7-G0].
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the source image should be treated as having an alpha channel (alpha will be set to the gray value),
 *                  EINA_FALSE for opaque gray (alpha will be set to 0xFF).
 * @return Pointer to the converted image data (ARGB8888), or NULL on allocation failure.
 *         In the output, R, G, and B components are set to the gray value.
 */
static inline void *
evas_common_convert_gry8_to_argb8888(const void *data, int w, int h, int stride, Eina_Bool has_alpha)
{
   const DATA8 *src, *end;
   DATA32 *ret, *dst;

   src = data;
   end = src + (stride * h);
   ret = malloc(w * h * sizeof(DATA32));
   if (!ret) return NULL;
   dst = ret;

   if (has_alpha)
     {
        for (; src < end; src++, dst++)
          {
             int c = (*src) & 0xFF;
             *dst = ARGB_JOIN(c, c, c, c);
          }
     }
   else
     {
        for (; src < end; src++, dst++)
          {
             int c = (*src) & 0xFF;
             *dst = ARGB_JOIN(0xFF, c, c, c);
          }
     }

   return ret;
}

/**
 * @brief Converts GRY8 image data to a specified colorspace.
 * @param data Pointer to the source image data (GRY8).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the source image should be treated as having an alpha channel.
 * @param cspace The target Evas_Colorspace.
 * @return Pointer to the converted image data, or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_gry8_to_argb8888()
 */
void *
evas_common_convert_gry8_to(const void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace)
{
   switch (cspace) {
      case EVAS_COLORSPACE_ARGB8888:
        return evas_common_convert_gry8_to_argb8888(data, w, h, stride, has_alpha);
      default:
        return NULL;
     }
}

/**
 * @internal
 * @brief Converts ARGB8888 image data to A8 (8-bit alpha) format.
 * @param data Pointer to the source image data (ARGB8888).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in uint32_t units (not bytes).
 * @param has_alpha EINA_TRUE if the source image has an alpha channel to extract,
 *                  EINA_FALSE to fill the destination with 0xFF (opaque).
 * @return Pointer to the converted image data (A8), or NULL on allocation failure.
 *         The returned data is a single block of A8 pixels.
 */
static inline void *
evas_common_convert_argb8888_to_a8(void *data, int w, int h, int stride, Eina_Bool has_alpha)
{
   uint32_t *src, *end;
   uint8_t *ret, *dst;

   src = data;
   end = src + (stride * h);
   ret = malloc(w * h);
   if (!ret) return NULL;

   if (!has_alpha)
     {
        return memset(ret, 0xff, w * h);
     }

   dst = ret;
   for ( ; src < end ; src++, dst++)
      *dst = CONVERT_ARGB_8888_TO_A_8(*src);
   return ret;
}

/**
 * @brief Converts ARGB8888 image data to a specified colorspace.
 * @param data Pointer to the source image data (ARGB8888).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in uint32_t units (not bytes).
 * @param has_alpha EINA_TRUE if the source image has an alpha channel.
 * @param cspace The target Evas_Colorspace.
 * @return Pointer to the converted image data, or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_argb8888_to_rgb565_a5p()
 * @see evas_common_convert_argb8888_to_a8()
 */
EVAS_API void *
evas_common_convert_argb8888_to(void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace)
{
   switch (cspace)
     {
	case EVAS_COLORSPACE_RGB565_A5P:
	  return evas_common_convert_argb8888_to_rgb565_a5p(data, w, h, stride, has_alpha);
	case EVAS_COLORSPACE_GRY8:
	  return evas_common_convert_argb8888_to_a8(data, w, h, stride, has_alpha);
	default:
	  break;
     }
   return NULL;
}

/**
 * @brief Converts RGB565_A5P image data to a specified colorspace.
 * @param data Pointer to the source image data (RGB565_A5P).
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source RGB565 image data in DATA16 units (not bytes).
 * @param has_alpha EINA_TRUE if the source image has an alpha plane.
 * @param cspace The target Evas_Colorspace.
 * @return Pointer to the converted image data, or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_rgb565_a5p_to_argb8888()
 */
EVAS_API void *
evas_common_convert_rgb565_a5p_to(void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace)
{
   switch (cspace)
     {
	case EVAS_COLORSPACE_ARGB8888:
	  return evas_common_convert_rgb565_a5p_to_argb8888(data, w, h, stride, has_alpha);
	default:
	  break;
     }
   return NULL;
}

/**
 * @brief Converts YUV 4:2:2 (ITU-R BT.601) image data to a specified colorspace.
 * @param data Pointer to the source image data (YUV 4:2:2).
 *             Expected layout: [Y0 U0 Y1 V0] [Y2 U1 Y3 V1] ...
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace. Currently only EVAS_COLORSPACE_ARGB8888 is supported.
 * @return Pointer to the converted image data (ARGB8888), or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_yuv_422_601_rgba()
 */
EVAS_API void *
evas_common_convert_yuv_422_601_to(void *data, int w, int h, Evas_Colorspace cspace)
{
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
        {
           void *dst;

           dst = malloc(sizeof (unsigned int) * w * h);
           if (!dst) return NULL;

           evas_common_convert_yuv_422_601_rgba(data, dst, w, h);
           return dst;
        }
      default:
         break;
     }
   return NULL;
}

/**
 * @brief Converts YUV 4:2:2 Planar (ITU-R BT.601) image data to a specified colorspace.
 * @param data Pointer to the source image data (YUV 4:2:2 Planar).
 *             Expected layout: Y plane, then U plane, then V plane.
 *             Example: [Y0Y1Y2Y3...][U0U1...][V0V1...]
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace. Currently only EVAS_COLORSPACE_ARGB8888 is supported.
 * @return Pointer to the converted image data (ARGB8888), or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_yuv_422p_601_rgba()
 */
EVAS_API void *
evas_common_convert_yuv_422P_601_to(void *data, int w, int h, Evas_Colorspace cspace)
{
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
        {
           void *dst;

           dst = malloc(sizeof (unsigned int) * w * h);
           if (!dst) return NULL;

           evas_common_convert_yuv_422p_601_rgba(data, dst, w, h);
           return dst;
        }
      default:
         break;
     }
   return NULL;
}

/**
 * @brief Converts YUV 4:2:0 Planar (ITU-R BT.601) image data to a specified colorspace.
 * @param data Pointer to the source image data (YUV 4:2:0 Planar).
 *             Expected layout: Y plane, then U plane, then V plane. U and V planes are half width and half height.
 *             Example: [Y0Y1Y2Y3...][U0U1...][V0V1...]
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace. Currently only EVAS_COLORSPACE_ARGB8888 is supported.
 * @return Pointer to the converted image data (ARGB8888), or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_yuv_420_601_rgba()
 */
EVAS_API void *
evas_common_convert_yuv_420_601_to(void *data, int w, int h, Evas_Colorspace cspace)
{
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
        {
           void *dst;

           dst = malloc(sizeof (unsigned int) * w * h);
           if (!dst) return NULL;

           evas_common_convert_yuv_420_601_rgba(data, dst, w, h);
           return dst;
        }
      default:
         break;
     }
   return NULL;
}

/**
 * @brief Converts YUV 4:2:0 Tiled (ITU-R BT.601) image data to a specified colorspace.
 * @param data Pointer to the source image data (YUV 4:2:0 Tiled).
 *             The exact tiled layout is hardware/platform specific. This function assumes it can be processed
 *             by `evas_common_convert_yuv_420_601_rgba` which typically expects planar YUV420.
 *             If the tiled format is significantly different, this conversion might be incorrect.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace. Currently only EVAS_COLORSPACE_ARGB8888 is supported.
 * @return Pointer to the converted image data (ARGB8888), or NULL if conversion to the target colorspace is not supported or fails.
 * @see evas_common_convert_yuv_420_601_rgba()
 */
EVAS_API void *
evas_common_convert_yuv_420T_601_to(void *data, int w, int h, Evas_Colorspace cspace)
{
   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
        {
           void *dst;

           dst = malloc(sizeof (unsigned int) * w * h);
           if (!dst) return NULL;

           evas_common_convert_yuv_420_601_rgba(data, dst, w, h);
           return dst;
        }
      default:
         break;
     }
   return NULL;
}


/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
