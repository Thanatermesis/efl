#include "evas_common_private.h"
#include "evas_convert_gry_8.h"

#ifdef USE_DITHER_44
extern const DATA8 _evas_dither_44[4][4];
#endif
#ifdef USE_DITHER_128128
extern const DATA8 _evas_dither_128128[128][128];
#endif

/**
 * @brief Converts RGBA data to 8-bit grayscale (256 levels) with dithering.
 *
 * This function takes a source image in RGBA format and converts it to an
 * 8-bit grayscale image. The conversion uses a standard luminance calculation.
 * Dithering parameters are present but marked as unused for this specific
 * 256-level grayscale conversion, implying no dithering is applied or needed
 * as 256 gray levels match the 8-bit output depth.
 *
 * @param src Pointer to the source image data (DATA32 array, 0xAARRGGBB).
 * @param dst Pointer to the destination image data (DATA8 array).
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset (unused).
 * @param dith_y Vertical dithering offset (unused).
 * @param pal Palette data (unused for grayscale conversion).
 */
void evas_common_convert_rgba_to_8bpp_gry_256_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA32 gry8;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = (R_VAL(src_ptr));
   g = (G_VAL(src_ptr));
   b = (B_VAL(src_ptr));

   // Y = 0.299 * R + 0.587 * G + 0.114 * B;
   gry8 = ((r * 19596) + (g * 38470) + (b * 7472)) >> 16;

   *dst_ptr = gry8;

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8-bit grayscale (64 levels) with dithering.
 * @note This function is currently a stub and does not perform any conversion.
 *
 * @param src Pointer to the source image data (unused).
 * @param dst Pointer to the destination image data (unused).
 * @param src_jump Number of bytes to skip to get to the next line in src (unused).
 * @param dst_jump Number of bytes to skip to get to the next line in dst (unused).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param dith_x Horizontal dithering offset (unused).
 * @param dith_y Vertical dithering offset (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_8bpp_gry_64_dith      (DATA32 *src EINA_UNUSED, DATA8 *dst EINA_UNUSED, int src_jump EINA_UNUSED, int dst_jump EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED){}

/**
 * @brief Converts RGBA data to 8-bit grayscale (16 levels) with dithering.
 *
 * This function converts an RGBA image to an 8-bit grayscale image with
 * 16 distinct gray levels. Dithering is applied using a dither matrix
 * (DM_TABLE) to simulate more shades of gray than are actually available.
 * The grayscale value is calculated using luminance, and then adjusted
 * based on the dither value.
 *
 * @param src Pointer to the source image data (DATA32 array, 0xAARRGGBB).
 * @param dst Pointer to the destination image data (DATA8 array).
 * @param src_jump Number of bytes to skip to get to the next line in src.
 * @param dst_jump Number of bytes to skip to get to the next line in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Horizontal dithering offset, used to index into the dither matrix.
 * @param dith_y Vertical dithering offset, used to index into the dither matrix.
 * @param pal Palette data (unused for grayscale conversion).
 */
void evas_common_convert_rgba_to_8bpp_gry_16_dith      (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA32 gry8;
   DATA8 dith;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);

   r = (R_VAL(src_ptr));
   g = (G_VAL(src_ptr));
   b = (B_VAL(src_ptr));

   // Y = 0.299 * R + 0.587 * G + 0.114 * B;
   gry8 = ((r * 19596) + (g * 38470) + (b * 7472)) >> 16;

   if (((gry8 - gry8 * 255 / 4) >= dith) && (gry8 < 0x07)) gry8++;

   *dst_ptr = gry8;

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8-bit grayscale (4 levels) with dithering.
 * @note This function is currently a stub and does not perform any conversion.
 *
 * @param src Pointer to the source image data (unused).
 * @param dst Pointer to the destination image data (unused).
 * @param src_jump Number of bytes to skip to get to the next line in src (unused).
 * @param dst_jump Number of bytes to skip to get to the next line in dst (unused).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param dith_x Horizontal dithering offset (unused).
 * @param dith_y Vertical dithering offset (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_8bpp_gry_4_dith       (DATA32 *src EINA_UNUSED, DATA8 *dst EINA_UNUSED, int src_jump EINA_UNUSED, int dst_jump EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED){}

/**
 * @brief Converts RGBA data to 8-bit grayscale (1-bit, i.e., black and white) with dithering.
 * @note This function is currently a stub and does not perform any conversion.
 *
 * @param src Pointer to the source image data (unused).
 * @param dst Pointer to the destination image data (unused).
 * @param src_jump Number of bytes to skip to get to the next line in src (unused).
 * @param dst_jump Number of bytes to skip to get to the next line in dst (unused).
 * @param w Width of the image in pixels (unused).
 * @param h Height of the image in pixels (unused).
 * @param dith_x Horizontal dithering offset (unused).
 * @param dith_y Vertical dithering offset (unused).
 * @param pal Palette data (unused).
 */
void evas_common_convert_rgba_to_8bpp_gry_1_dith       (DATA32 *src EINA_UNUSED, DATA8 *dst EINA_UNUSED, int src_jump EINA_UNUSED, int dst_jump EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED){}
