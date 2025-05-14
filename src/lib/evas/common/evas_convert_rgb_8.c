#include "evas_common_private.h"
#include "evas_convert_rgb_8.h"

#ifdef USE_DITHER_44
extern const DATA8 _evas_dither_44[4][4];
#endif
#ifdef USE_DITHER_128128
extern const DATA8 _evas_dither_128128[128][128];
#endif

/**
 * @brief Converts RGBA data to 8bpp RGB (332 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 332 color space (3 bits for red, 3 bits for green,
 * 2 bits for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 332 values to palette indices.
 *            Example: pal[(r_3bit << 5) | (g_3bit << 2) | (b_2bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_332_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith, dith2;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 3);
   dith2 = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 2);
/*   r = (R_VAL(src_ptr)) >> (8 - 3);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 3);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 2);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 3))) >= dith ) && (r < 0x07)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 3))) >= dith ) && (g < 0x07)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 2))) >= dith2) && (b < 0x03)) b++;*/
   r = (R_VAL(src_ptr)) * 7 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 7)) >= dith ) && (r < 0x07)) r++;
   g = (G_VAL(src_ptr)) * 7 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 7)) >= dith ) && (g < 0x07)) g++;
   b = (B_VAL(src_ptr)) * 3 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 3)) >= dith2) && (b < 0x03)) b++;

   *dst_ptr = pal[(r << 5) | (g << 2) | (b)];

   CONVERT_LOOP_END_ROT_0();
}

/** @brief Lookup table for converting 8-bit color channel to 6-level (0-5) representation. */
static DATA8 p_to_6[256];
/** @brief Lookup table for the error term in 8-bit to 6-level conversion, scaled for dithering. */
static DATA8 p_to_6_err[256];

/**
 * @brief Converts RGBA data to 8bpp RGB (666 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 666 color space (6 levels for red, 6 for green,
 * 6 for blue, totaling 216 colors). Dithering is applied.
 * It uses pre-calculated lookup tables (p_to_6, p_to_6_err) for efficiency.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 666 values to palette indices.
 *            Example: pal[(r_6level * 36) + (g_6level * 6) + (b_6level)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_666_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith;
   static int tables_calcualted = 0;

   if (!tables_calcualted)
     {
	int i;

	tables_calcualted = 1;
	for (i = 0; i < 256; i++)
	  p_to_6[i] = (i * 5) / 255;
	for (i = 0; i < 256; i++)
	  p_to_6_err[i] = ((i * 5) - (p_to_6[i] * 255)) * DM_DIV / 255;
     }
   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = p_to_6[(R_VAL(src_ptr))];
   g = p_to_6[(G_VAL(src_ptr))];
   b = p_to_6[(B_VAL(src_ptr))];
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK];
   if ((p_to_6_err[(R_VAL(src_ptr))] >= dith ) && (r < 5)) r++;
   if ((p_to_6_err[(G_VAL(src_ptr))] >= dith ) && (g < 5)) g++;
   if ((p_to_6_err[(B_VAL(src_ptr))] >= dith ) && (b < 5)) b++;

   *dst_ptr = pal[(r * 36) + (g * 6) + (b)];

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8bpp RGB (232 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 232 color space (2 bits for red, 3 bits for green,
 * 2 bits for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 232 values to palette indices.
 *            Example: pal[(r_2bit << 5) | (g_3bit << 2) | (b_2bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_232_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith, dith2;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 3);
   dith2 = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 2);
/*   r = (R_VAL(src_ptr)) >> (8 - 2);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 3);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 2);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 2))) >= dith2) && (r < 0x03)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 3))) >= dith ) && (g < 0x07)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 2))) >= dith2) && (b < 0x03)) b++;*/
   r = (R_VAL(src_ptr)) * 3 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 3)) >= dith2) && (r < 0x03)) r++;
   g = (G_VAL(src_ptr)) * 7 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 7)) >= dith ) && (g < 0x07)) g++;
   b = (B_VAL(src_ptr)) * 3 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 3)) >= dith2) && (b < 0x03)) b++;

   *dst_ptr = pal[(r << 5) | (g << 2) | (b)];

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8bpp RGB (222 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 222 color space (2 bits for red, 2 bits for green,
 * 2 bits for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 222 values to palette indices.
 *            Example: pal[(r_2bit << 4) | (g_2bit << 2) | (b_2bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_222_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 2);
/*   r = (R_VAL(src_ptr)) >> (8 - 2);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 2);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 2);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 2))) >= dith ) && (r < 0x03)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 2))) >= dith ) && (g < 0x03)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 2))) >= dith ) && (b < 0x03)) b++;*/
   r = (R_VAL(src_ptr)) * 3 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 3)) >= dith ) && (r < 0x03)) r++;
   g = (G_VAL(src_ptr)) * 3 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 3)) >= dith ) && (g < 0x03)) g++;
   b = (B_VAL(src_ptr)) * 3 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 3)) >= dith ) && (b < 0x03)) b++;

   *dst_ptr = pal[(r << 4) | (g << 2) | (b)];

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8bpp RGB (221 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 221 color space (2 bits for red, 2 bits for green,
 * 1 bit for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 221 values to palette indices.
 *            Example: pal[(r_2bit << 3) | (g_2bit << 1) | (b_1bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_221_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith, dith2;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 2);
   dith2 = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 1);
/*   r = (R_VAL(src_ptr)) >> (8 - 2);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 2);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 1);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 2))) >= dith ) && (r < 0x03)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 2))) >= dith ) && (g < 0x03)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 1))) >= dith2) && (b < 0x01)) b++;*/
   r = (R_VAL(src_ptr)) * 3 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 3)) >= dith ) && (r < 0x03)) r++;
   g = (G_VAL(src_ptr)) * 3 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 3)) >= dith ) && (g < 0x03)) g++;
   b = (B_VAL(src_ptr)) * 1 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 1)) >= dith2) && (b < 0x01)) b++;

   *dst_ptr = pal[(r << 3) | (g << 1) | (b)];

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8bpp RGB (121 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 121 color space (1 bit for red, 2 bits for green,
 * 1 bit for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 121 values to palette indices.
 *            Example: pal[(r_1bit << 3) | (g_2bit << 1) | (b_1bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_121_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith, dith2;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 2);
   dith2 = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 1);
/*   r = (R_VAL(src_ptr)) >> (8 - 1);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 2);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 1);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 1))) >= dith2) && (r < 0x01)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 2))) >= dith ) && (g < 0x03)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 1))) >= dith2) && (b < 0x01)) b++;*/

   r = (R_VAL(src_ptr)) * 1 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 1)) >= dith2) && (r < 0x01)) r++;
   g = (G_VAL(src_ptr)) * 3 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 3)) >= dith ) && (g < 0x03)) g++;
   b = (B_VAL(src_ptr)) * 1 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 1)) >= dith2) && (b < 0x01)) b++;

   *dst_ptr = pal[(r << 3) | (g << 1) | (b)];

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data to 8bpp RGB (111 format) with dithering.
 *
 * This function takes a source RGBA image and converts it to an 8-bit
 * paletted image using an RGB 111 color space (1 bit for red, 1 bit for green,
 * 1 bit for blue). Dithering is applied to reduce color banding.
 *
 * @param src Pointer to the source RGBA data (32-bit per pixel).
 * @param dst Pointer to the destination 8-bit paletted data.
 * @param src_jump Number of bytes to jump to get to the next row in the source image.
 * @param dst_jump Number of bytes to jump to get to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X offset for the dithering matrix.
 * @param dith_y Y offset for the dithering matrix.
 * @param pal Pointer to the color palette. The palette should be pre-calculated
 *            to map RGB 111 values to palette indices.
 *            Example: pal[(r_1bit << 2) | (g_1bit << 1) | (b_1bit)] = palette_index;
 */
void evas_common_convert_rgba_to_8bpp_rgb_111_dith     (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
   DATA8 dith;

   dst_ptr = (DATA8 *)dst;

   CONVERT_LOOP_START_ROT_0();

   dith = DM_SHR(DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK], 1);
/*   r = (R_VAL(src_ptr)) >> (8 - 1);*/
/*   g = (G_VAL(src_ptr)) >> (8 - 1);*/
/*   b = (B_VAL(src_ptr)) >> (8 - 1);*/
/*   if (((R_VAL(src_ptr) - (r << (8 - 1))) >= dith ) && (r < 0x01)) r++;*/
/*   if (((G_VAL(src_ptr) - (g << (8 - 1))) >= dith ) && (g < 0x01)) g++;*/
/*   if (((B_VAL(src_ptr) - (b << (8 - 1))) >= dith ) && (b < 0x01)) b++;*/

   r = (R_VAL(src_ptr)) * 1 / 255;
   if (((R_VAL(src_ptr) - (r * 255 / 1)) >= dith ) && (r < 0x01)) r++;
   g = (G_VAL(src_ptr)) * 1 / 255;
   if (((G_VAL(src_ptr) - (g * 255 / 1)) >= dith ) && (g < 0x01)) g++;
   b = (B_VAL(src_ptr)) * 1 / 255;
   if (((B_VAL(src_ptr) - (b * 255 / 1)) >= dith ) && (b < 0x01)) b++;

   *dst_ptr = pal[(r << 2) | (g << 1) | (b)];

   CONVERT_LOOP_END_ROT_0();
}
