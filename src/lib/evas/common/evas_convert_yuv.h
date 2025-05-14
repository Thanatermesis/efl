#ifndef _EVAS_CONVERT_YUV_H
#define _EVAS_CONVERT_YUV_H

/**
 * @file evas_convert_yuv.h
 * @brief Functions for converting YUV image data to RGBA.
 *
 * This file declares functions for various YUV to RGBA color space conversions,
 * supporting different YUV formats (422p, 422, 420, 420T) and color standards
 * (BT.709, BT.601).
 */

/**
 * @brief Converts YUV 4:2:2 planar (BT.709) to RGBA.
 *
 * Processes YUV data where Y, U, and V planes are separate.
 * U and V planes are half the width of the Y plane.
 * Uses BT.709 color conversion coefficients.
 *
 * @param src Array of pointers to Y, U, V planes.
 *            src[0] = Y plane (width w, height h)
 *            src[1] = U plane (width w/2, height h)
 *            src[2] = V plane (width w/2, height h)
 * @param dst Pointer to the destination RGBA buffer (output).
 *            The buffer must be large enough to hold w * h * 4 bytes.
 *            Each pixel is stored as 32-bit RGBA (e.g., 0xAARRGGBB or 0xBBGGRRAA depending on endianness).
 * @param w Width of the Y plane in pixels.
 * @param h Height of the Y plane in pixels.
 */
EVAS_API void evas_common_convert_yuv_422p_709_rgba     (DATA8 **src, DATA8 *dst, int w, int h);

/**
 * @brief Converts YUV 4:2:2 planar (BT.601) to RGBA.
 *
 * Processes YUV data where Y, U, and V planes are separate.
 * U and V planes are half the width of the Y plane.
 * Uses BT.601 color conversion coefficients.
 *
 * @param src Array of pointers to Y, U, V planes.
 *            src[0] = Y plane (width w, height h)
 *            src[1] = U plane (width w/2, height h)
 *            src[2] = V plane (width w/2, height h)
 * @param dst Pointer to the destination RGBA buffer (output).
 *            The buffer must be large enough to hold w * h * 4 bytes.
 * @param w Width of the Y plane in pixels.
 * @param h Height of the Y plane in pixels.
 */
EVAS_API void evas_common_convert_yuv_422p_601_rgba     (DATA8 **src, DATA8 *dst, int w, int h);

/**
 * @brief Converts YUV 4:2:2 interleaved (YUY2/YUYV like, BT.601) to RGBA.
 *
 * Processes YUV data where Y, U, and V components are interleaved.
 * Typically, each macropixel is [Y0, U0, Y1, V0].
 * Uses BT.601 color conversion coefficients.
 *
 * @param src Array of pointers to image data rows.
 *            src[row_index] points to the start of a line of interleaved YUV data.
 *            Each line contains (w * 2) bytes.
 *            Example for one line: [Y0, U0, Y1, V0, Y2, U1, Y3, V1, ...]
 * @param dst Pointer to the destination RGBA buffer (output).
 *            The buffer must be large enough to hold w * h * 4 bytes.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 */
EVAS_API void evas_common_convert_yuv_422_601_rgba      (DATA8 **src, DATA8 *dst, int w, int h);

/**
 * @brief Converts YUV 4:2:0 planar (NV12 like, BT.601) to RGBA.
 *
 * Processes YUV data with a Y plane and an interleaved UV plane.
 * The UV plane is half the width and half the height of the Y plane.
 * Uses BT.601 color conversion coefficients.
 *
 * @param src Array of pointers to Y plane rows and UV plane rows.
 *            src[0] to src[h-1] = Y plane rows (width w, height h)
 *            src[h] to src[h + h/2 - 1] = UV plane rows (width w, height h/2, interleaved U and V, e.g., U0V0U1V1...)
 * @param dst Pointer to the destination RGBA buffer (output).
 *            The buffer must be large enough to hold w * h * 4 bytes.
 * @param w Width of the Y plane in pixels.
 * @param h Height of the Y plane in pixels.
 */
EVAS_API void evas_common_convert_yuv_420_601_rgba      (DATA8 **src, DATA8 *dst, int w, int h);

/**
 * @brief Converts YUV 4:2:0 tiled (proprietary tiled format, BT.601) to RGBA.
 *
 * Processes YUV data stored in a specific tiled memory layout.
 * Uses BT.601 color conversion coefficients.
 * The exact tiling scheme is specific to the hardware/platform this function targets.
 *
 * @param src Array of pointers to Y plane macroblock rows and UV plane macroblock rows.
 *            The structure of `src` depends on the specific tiled format.
 *            Generally, it will point to regions of memory containing tiled Y data
 *            followed by regions containing tiled UV data.
 *            Example (conceptual, actual layout is implementation-defined):
 *            src[0]...src[mb_h_y-1] point to rows of Y macroblocks.
 *            src[mb_h_y]...src[mb_h_y + mb_h_uv-1] point to rows of UV macroblocks.
 * @param dst Pointer to the destination RGBA buffer (output).
 *            The buffer must be large enough to hold w * h * 4 bytes.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 */
EVAS_API void evas_common_convert_yuv_420T_601_rgba     (DATA8 **src, DATA8 *dst, int w, int h);

#endif /* _EVAS_CONVERT_YUV_H */
