#ifndef _EVAS_CONVERT_RGB_24_H
#define _EVAS_CONVERT_RGB_24_H

/**
 * @file
 * @brief Functions for converting RGBA image data to various 24bpp RGB/BGR formats.
 */

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp RGB (888) format.
 * @param src Pointer to the source image data (RGBA).
 * @param dst Pointer to the destination buffer (RGB 888).
 * @param src_jump Source row stride in DATA32 elements.
 * @param dst_jump Destination row stride in DATA8 elements.
 * @param w Image width in pixels.
 * @param h Image height in pixels.
 * @param dith_x Dither x offset (unused).
 * @param dith_y Dither y offset (unused).
 * @param pal Palette data (unused).
 * @see evas_common_convert_rgba_to_24bpp_rgb_888() in evas_convert_rgb_24.c for implementation details.
 */
void evas_common_convert_rgba_to_24bpp_rgb_888                 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp BGR (888) format.
 * @param src Pointer to the source image data (RGBA).
 * @param dst Pointer to the destination buffer (BGR 888).
 * @param src_jump Source row stride in DATA32 elements.
 * @param dst_jump Destination row stride in DATA8 elements.
 * @param w Image width in pixels.
 * @param h Image height in pixels.
 * @param dith_x Dither x offset (unused).
 * @param dith_y Dither y offset (unused).
 * @param pal Palette data (unused).
 * @see evas_common_convert_rgba_to_24bpp_bgr_888() in evas_convert_rgb_24.c for implementation details.
 */
void evas_common_convert_rgba_to_24bpp_bgr_888                 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp RGB (666) format.
 * @param src Pointer to the source image data (RGBA).
 * @param dst Pointer to the destination buffer (RGB 666).
 * @param src_jump Source row stride in DATA32 elements.
 * @param dst_jump Destination row stride in DATA8 elements.
 * @param w Image width in pixels.
 * @param h Image height in pixels.
 * @param dith_x Dither x offset (unused).
 * @param dith_y Dither y offset (unused).
 * @param pal Palette data (unused).
 * @see evas_common_convert_rgba_to_24bpp_rgb_666() in evas_convert_rgb_24.c for implementation details.
 */
void evas_common_convert_rgba_to_24bpp_rgb_666                 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

#endif /* _EVAS_CONVERT_RGB_24_H */
