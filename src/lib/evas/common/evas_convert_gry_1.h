#ifndef _EVAS_CONVERT_GRY_1_H
#define _EVAS_CONVERT_GRY_1_H

/**
 * @brief Converts RGBA image data to 1-bit grayscale with dithering.
 *
 * This function takes a source image in RGBA format (32 bits per pixel)
 * and converts it to a 1-bit grayscale image (1 bit per pixel), applying
 * dithering to improve visual quality. The output is packed, with 8 pixels
 * per byte.
 *
 * @param src Pointer to the source image data (array of DATA32).
 *            Each DATA32 element represents a pixel in AARRGGBB format.
 *            Example: `[0xFFRRGGBB, 0xFFRRGGBB, ...]`
 * @param dst Pointer to the destination image data (array of DATA8).
 *            Each bit in a DATA8 element represents a pixel (0 or 1).
 *            Pixels are packed, so one DATA8 element holds 8 pixels.
 *            Example: `[0b01101001, ...]` where each bit is a pixel.
 * @param src_jump Number of DATA32 elements to skip to get to the next row in the source image.
 *                 This is effectively the width of the source image if it's contiguous.
 * @param dst_jump Number of DATA8 elements (bytes) to skip to get to the next row in the destination image.
 *                 This is effectively the width of the destination image in bytes.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering pattern.
 * @param dith_y Y offset for the dithering pattern.
 * @param pal Pointer to a 2-entry palette (array of DATA8) defining the two colors
 *            for the 1-bit output. Typically black and white.
 *            Example: `pal[0]` could be 0 (black), `pal[1]` could be 255 (white) if
 *            the output bits 0 and 1 map to these palette indices.
 *            However, for 1bpp grayscale, this usually implies mapping to intensity
 *            values 0 and 255 directly, where the output bit 0 means `pal[0]`
 *            and bit 1 means `pal[1]`. The actual interpretation of `pal` might
 *            depend on how the destination `DATA8` values are later interpreted.
 *            For a direct 1bpp representation, `pal` might define the mapping
 *            of the dithered grayscale value to a 0 or 1 bit.
 */
void evas_common_convert_rgba_to_1bpp_gry_1_dith               (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal);



#endif /* _EVAS_CONVERT_GRY_1_H */
