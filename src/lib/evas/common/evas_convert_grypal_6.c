#include "evas_common_private.h"
#include "evas_convert_grypal_6.h"

/**
 * @file evas_convert_grypal_6.c
 * @brief Contains functions for converting RGBA image data to 8-bit palettized grayscale
 * using a 64-level gray palette.
 */

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
 * @param dith_x Dithering offset for the x-axis (marked as unused by EINA_UNUSED).
 * @param dith_y Dithering offset for the y-axis (marked as unused by EINA_UNUSED).
 * @param pal Pointer to the 8-bit grayscale palette. This palette is an array of
 *            DATA8 values. The calculated luminance (0-255) of an RGBA pixel
 *            is used as an index into this palette (e.g., `pal[luminance]`)
 *            to get the final 8-bit pixel value for the destination image.
 *            This palette should effectively represent 64 shades of gray.
 */
void evas_common_convert_rgba_to_8bpp_pal_gray64(DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal)
{
    DATA32 *src_ptr;
    DATA8 *dst_ptr;
    int x, y;
    DATA8 Y;

    dst_ptr = dst;
    CONVERT_LOOP_START_ROT_0();

    /*
     * Convert RGB to Luminance (Y).
     * The formula Y = (R * 76 + G * 151 + B * 29) >> 10 is a common approximation
     * for Y = 0.299*R + 0.587*G + 0.114*B, scaled for integer arithmetic.
     * The coefficients 76, 151, 29 sum to 256. The right shift by 10 (division by 1024)
     * is effectively (Value * 256) / 1024 = Value / 4.
     * However, typical coefficients for Y (scaled to 256 for the sum of R,G,B components) are
     * R_COEFF = 77 (0.299*256), G_COEFF = 150 (0.587*256), B_COEFF = 29 (0.114*256).
     * The sum of these is 256.
     * The current coefficients (76, 151, 29) sum to 256.
     * The division by 2^10 (1024) seems off if the input R,G,B are 0-255.
     * If R_VAL, G_VAL, B_VAL extract 0-255 values, then the result of the sum
     * (R*76 + G*151 + B*29) can be up to 255*76 + 255*151 + 255*29 = 255 * (76+151+29) = 255 * 256 = 65280.
     * Dividing by 1024 (>>10) would give a Y range of 0 to 63.75 (i.e., 0-63).
     * This 0-63 range would then be used as an index into `pal`.
     * This seems consistent with a "gray64" palette, where 64 distinct gray levels are used.
     * The `pal` array would then map these 0-63 values to actual 8-bit palette entries.
     */
    Y = ((R_VAL(src_ptr) * 76) +
         (G_VAL(src_ptr) * 151) +
         (B_VAL(src_ptr) * 29)) >> 10;
    /* Use the calculated luminance Y (expected range 0-63) as an index into the palette. */
    *dst_ptr = pal[Y];

    CONVERT_LOOP_END_ROT_0();
}
