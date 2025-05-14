#ifndef _EVAS_CONVERT_RGB_8_H
#define _EVAS_CONVERT_RGB_8_H

/**
 * @file
 * @brief Functions for converting RGBA image data to various 8bpp paletted RGB formats with dithering.
 */

/**
 * @brief Converts RGBA data to 8bpp RGB (332 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_332_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_332_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (666 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_666_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_666_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (232 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_232_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_232_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (222 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_222_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_222_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (221 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_221_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_221_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (121 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_121_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_121_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts RGBA data to 8bpp RGB (111 format) with dithering.
 * @copydetails evas_common_convert_rgba_to_8bpp_rgb_111_dith
 */
void evas_common_convert_rgba_to_8bpp_rgb_111_dith             (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);


#endif /* _EVAS_CONVERT_RGB_8_H */
