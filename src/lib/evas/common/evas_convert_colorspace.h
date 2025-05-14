#ifndef _EVAS_CONVERT_COLORSPACE_H
#define _EVAS_CONVERT_COLORSPACE_H

/**
 * @file
 * @brief Header file for Evas colorspace conversion functions.
 *
 * This file declares functions for converting image data between various
 * colorspaces supported by Evas.
 */

/**
 * @brief Converts ARGB8888 image data to a specified target colorspace.
 * @param data Pointer to the source image data in ARGB8888 format.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in uint32_t units (not bytes).
 *               This is the number of DATA32 elements per row.
 * @param has_alpha EINA_TRUE if the source image has an alpha channel, EINA_FALSE otherwise.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure (e.g., unsupported conversion, memory allocation error).
 *         The caller is responsible for freeing this buffer.
 */
EVAS_API void *evas_common_convert_argb8888_to    (void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace);

/**
 * @brief Converts RGB565_A5P image data to a specified target colorspace.
 * @param data Pointer to the source image data. For RGB565_A5P, this points to the
 *             start of the RGB565 data. The alpha plane (A5P) is expected to
 *             immediately follow the RGB565 data if has_alpha is EINA_TRUE.
 *             Layout: [R5G6B5 data...][A5P data...]
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source RGB565 image data in DATA16 units (not bytes).
 *               This is the number of DATA16 elements per row for the RGB part.
 * @param has_alpha EINA_TRUE if the source image has an alpha plane, EINA_FALSE otherwise.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
EVAS_API void *evas_common_convert_rgb565_a5p_to  (void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace);

/**
 * @brief Converts YUV 4:2:2 Planar (ITU-R BT.601) image data to a specified target colorspace.
 * @param data Pointer to the source image data. This should point to the start of the Y plane.
 *             The U and V planes are expected to follow.
 *             Layout: [Y plane data...][U plane data...][V plane data...]
 *             Y plane size: w * h
 *             U plane size: (w/2) * h
 *             V plane size: (w/2) * h
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
EVAS_API void *evas_common_convert_yuv_422P_601_to(void *data, int w, int h, Evas_Colorspace cspace);

/**
 * @brief Converts YUV 4:2:2 Interleaved (ITU-R BT.601) image data to a specified target colorspace.
 * @param data Pointer to the source image data.
 *             Expected interleaved layout, e.g., [Y0 U0 Y1 V0] [Y2 U1 Y3 V1] ...
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
EVAS_API void *evas_common_convert_yuv_422_601_to (void *data, int w, int h, Evas_Colorspace cspace);

/**
 * @brief Converts YUV 4:2:0 Planar (ITU-R BT.601) image data to a specified target colorspace.
 * @param data Pointer to the source image data. This should point to the start of the Y plane.
 *             The U and V planes are expected to follow.
 *             Layout: [Y plane data...][U plane data...][V plane data...]
 *             Y plane size: w * h
 *             U plane size: (w/2) * (h/2)
 *             V plane size: (w/2) * (h/2)
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
EVAS_API void *evas_common_convert_yuv_420_601_to (void *data, int w, int h, Evas_Colorspace cspace);

/**
 * @brief Converts YUV 4:2:0 Tiled (ITU-R BT.601) image data to a specified target colorspace.
 * @param data Pointer to the source image data in a tiled YUV 4:2:0 format.
 *             The exact tiling scheme is platform/hardware dependent.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 * @note This function often assumes the tiled data can be processed similarly to planar YUV 4:2:0.
 */
EVAS_API void *evas_common_convert_yuv_420T_601_to(void *data, int w, int h, Evas_Colorspace cspace);

/**
 * @brief Converts AGRY88 (Alpha Grayscale, 8-bit Alpha, 8-bit Gray) image data to a specified target colorspace.
 * @param data Pointer to the source image data (AGRY88). Each pixel is 16 bits: [A7-A0 G7-G0].
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the alpha component of AGRY88 should be used, EINA_FALSE otherwise (treat as opaque gray).
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
void *evas_common_convert_agry88_to(const void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace);

/**
 * @brief Converts GRY8 (Grayscale, 8-bit) image data to a specified target colorspace.
 * @param data Pointer to the source image data (GRY8). Each pixel is 8 bits: [G7-G0].
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param stride Stride of the source image data in bytes.
 * @param has_alpha EINA_TRUE if the grayscale value should also be used as alpha (e.g., for ARGB output where A=G),
 *                  EINA_FALSE to treat as opaque gray (e.g., for ARGB output where A=0xFF).
 * @param cspace The target Evas_Colorspace to convert to.
 * @return A pointer to the newly allocated buffer containing the converted image data,
 *         or NULL on failure. The caller is responsible for freeing this buffer.
 */
void *evas_common_convert_gry8_to(const void *data, int w, int h, int stride, Eina_Bool has_alpha, Evas_Colorspace cspace);

#endif /* _EVAS_CONVERT_COLORSPACE_H */
