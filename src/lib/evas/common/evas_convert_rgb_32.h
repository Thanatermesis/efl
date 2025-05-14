#ifndef _EVAS_CONVERT_RGB_32_H
#define _EVAS_CONVERT_RGB_32_H

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image.
 *
 * This function copies pixel data from the source (assumed to be ARGB, e.g., 0xAARRGGBB)
 * to the destination in the same 32-bit ARGB format. Alpha is preserved.
 *
 * @param src Pointer to the source image data. Array of DATA32, where each DATA32 is an ARGB pixel (e.g., 0xAARRGGBB).
 * @param dst Pointer to the destination image data buffer (array of DATA8, treated as DATA32 elements).
 * @param src_jump Number of DATA32 elements to skip in src to get to the start of the next row.
 * @param dst_jump Number of DATA32 elements to skip in dst to get to the start of the next row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering x-coordinate (unused in this function).
 * @param dith_y Dithering y-coordinate (unused in this function).
 * @param pal Pointer to palette data (unused in this function).
 */
void evas_common_convert_rgba_to_32bpp_rgb_8888                (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 180-degree rotation.
 *
 * Pixel data is copied from source (ARGB) to destination (ARGB) with a 180-degree rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer.
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels.
 * @param h Height of the source image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgb_8888_rot_180        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 270-degree rotation.
 *
 * Pixel data is copied from source (ARGB) to destination (ARGB) with a 270-degree (counter-clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer.
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgb_8888_rot_270        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp ARGB destination image, with 90-degree rotation.
 *
 * Pixel data is copied from source (ARGB) to destination (ARGB) with a 90-degree (clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer.
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgb_8888_rot_90         (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image.
 *
 * This function converts ARGB (e.g., 0xAARRGGBB) pixels to RGB0 format
 * (e.g., 0xRRGGBB00), where the alpha channel is set to zero and is the least significant byte.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (RGB0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgbx_8888               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);
/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 180-degree rotation.
 *
 * Converts ARGB to RGB0 (e.g., 0xRRGGBB00) with a 180-degree rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (RGB0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels.
 * @param h Height of the source image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_180       (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 270-degree rotation.
 *
 * Converts ARGB to RGB0 (e.g., 0xRRGGBB00) with a 270-degree (counter-clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (RGB0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_270       (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp RGB0 destination image, with 90-degree rotation.
 *
 * Converts ARGB to RGB0 (e.g., 0xRRGGBB00) with a 90-degree (clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (RGB0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgbx_8888_rot_90        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image.
 *
 * This function converts ARGB (e.g., 0xAARRGGBB) pixels to 0BGR format
 * (e.g., 0x00BBGGRR), where the most significant byte (alpha position in ARGB) is set to zero.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (0BGR pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgr_8888                (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);
/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 180-degree rotation.
 *
 * Converts ARGB to 0BGR (e.g., 0x00BBGGRR) with a 180-degree rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (0BGR pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels.
 * @param h Height of the source image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgr_8888_rot_180        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 270-degree rotation.
 *
 * Converts ARGB to 0BGR (e.g., 0x00BBGGRR) with a 270-degree (counter-clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (0BGR pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgr_8888_rot_270        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp 0BGR destination image, with 90-degree rotation.
 *
 * Converts ARGB to 0BGR (e.g., 0x00BBGGRR) with a 90-degree (clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (0BGR pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgr_8888_rot_90         (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image.
 *
 * This function converts ARGB (e.g., 0xAARRGGBB) pixels to BGR0 format
 * (e.g., 0xBBGGRR00), where the alpha channel is set to zero and is the least significant byte.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (BGR0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgrx_8888               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);
/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 180-degree rotation.
 *
 * Converts ARGB to BGR0 (e.g., 0xBBGGRR00) with a 180-degree rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (BGR0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels.
 * @param h Height of the source image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_180       (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 270-degree rotation.
 *
 * Converts ARGB to BGR0 (e.g., 0xBBGGRR00) with a 270-degree (counter-clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (BGR0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_270       (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp BGR0 destination image, with 90-degree rotation.
 *
 * Converts ARGB to BGR0 (e.g., 0xBBGGRR00) with a 90-degree (clockwise) rotation.
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (BGR0 pixels).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the source image in pixels (becomes height of destination).
 * @param h Height of the source image in pixels (becomes width of destination).
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_bgrx_8888_rot_90        (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

/**
 * @brief Converts an ARGB source image to a 32bpp RGB666 destination image.
 *
 * This function converts ARGB (e.g., 0xAARRGGBB) pixels to an 18-bit RGB format
 * (6 bits per R, G, B channel), packed into a 32-bit word. The destination format
 * is 0x00RRRRRRGGGGGGBBBBBB (most significant 14 bits are zero).
 *
 * @param src Pointer to the source image data (ARGB pixels).
 * @param dst Pointer to the destination image data buffer (RGB666 pixels packed in DATA32).
 * @param src_jump Skip in src to get to the next row.
 * @param dst_jump Skip in dst to get to the next row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering x-coordinate (unused).
 * @param dith_y Dithering y-coordinate (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_32bpp_rgb_666                 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

#endif /* _EVAS_CONVERT_RGB_32_H */
