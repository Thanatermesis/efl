#ifndef _EVAS_CONVERT_GRY_4_H
#define _EVAS_CONVERT_GRY_4_H

/**
 * @brief Converts RGBA image data to 8-bit palettized grayscale using a 64-level gray palette.
 *
 * This function takes a source image in 32-bit RGBA format and converts it
 * to an 8-bit image where each pixel is an index into the provided grayscale
 * palette. The conversion uses a standard luminance calculation.
 *
 * @param src Pointer to the source image data (32-bit RGBA pixels).
 *            Each pixel is a DATA32 value, typically 0xAARRGGBB.
 * @param dst Pointer to the destination image data (8-bit palette indices).
 *            Each pixel will be a DATA8 value, an index into the `pal` array.
 * @param src_jump Byte offset to get to the next row in the source image buffer.
 * @param dst_jump Byte offset to get to the next row in the destination image buffer.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Dithering offset for the x-axis (currently unused).
 * @param dith_y Dithering offset for the y-axis (currently unused).
 * @param pal Pointer to the 8-bit grayscale palette. This palette is an array of
 *            DATA8 values. The calculated luminance (0-255) of an RGBA pixel
 *            is used as an index into this palette (e.g., `pal[luminance]`)
 *            to get the final 8-bit pixel value for the destination image.
 *            This palette should effectively represent 64 shades of gray.
 */
void evas_common_convert_rgba_to_8bpp_pal_gray64               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);

#endif /* _EVAS_CONVERT_GRY_4_H */
