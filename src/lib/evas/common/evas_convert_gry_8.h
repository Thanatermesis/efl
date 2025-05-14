#ifndef _EVAS_CONVERT_GRY_8_H
#define _EVAS_CONVERT_GRY_8_H

/**
 * @file
 * @brief Functions for converting RGBA image data to 8-bit grayscale with various bit depths and dithering.
 */

/**
 * @brief Converts RGBA data to 8-bit grayscale (256 levels) with dithering.
 * @param src Pointer to the source image data (DATA32 array, 0xAARRGGBB).
 * @param dst Pointer to the destination image data (DATA8 array).
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset.
 * @param dith_y Vertical dithering offset.
 * @param pal Palette data (typically unused for direct grayscale conversion).
 */
void evas_common_convert_rgba_to_8bpp_gry_256_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8-bit grayscale (64 levels) with dithering.
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset.
 * @param dith_y Vertical dithering offset.
 * @param pal Palette data.
 * @note This function is currently a stub.
 */
void evas_common_convert_rgba_to_8bpp_gry_64_dith              (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8-bit grayscale (16 levels) with dithering.
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset.
 * @param dith_y Vertical dithering offset.
 * @param pal Palette data.
 */
void evas_common_convert_rgba_to_8bpp_gry_16_dith              (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8-bit grayscale (4 levels) with dithering.
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset.
 * @param dith_y Vertical dithering offset.
 * @param pal Palette data.
 * @note This function is currently a stub.
 */
void evas_common_convert_rgba_to_8bpp_gry_4_dith               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8-bit grayscale (1-bit, black and white) with dithering.
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset.
 * @param dith_y Vertical dithering offset.
 * @param pal Palette data.
 * @note This function is currently a stub.
 */
void evas_common_convert_rgba_to_8bpp_gry_1_dith               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);


#endif /* _EVAS_CONVERT_GRY_8_H */
