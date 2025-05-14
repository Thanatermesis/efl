#include "evas_common_private.h"
#include "evas_convert_rgb_16.h"

#ifndef BUILD_NO_DITHER_MASK
#ifdef USE_DITHER_44
extern const DATA8 _evas_dither_44[4][4];
#endif
#ifdef USE_DITHER_128128
extern const DATA8 _evas_dither_128128[128][128];
#endif
#endif

/**
 * @brief Converts a block of RGBA data (2 pixels at a time) to 16bpp RGB (5-6-5 format) with dithering.
 *
 * Processes two 32-bit RGBA source pixels and converts them into two 16-bit RGB_565
 * destination pixels. Dithering is applied to improve color accuracy on the reduced
 * bit-depth display.
 *
 * @param src Pointer to the source image data (array of DATA32). Each DATA32 is an ARGB pixel.
 *            Example: src[0] = 0xAARRGGBB for the first pixel.
 * @param dst Pointer to the destination image data (array of DATA8, treated as DATA16).
 *            Destination stores 16-bit pixels. Example: ((DATA16*)dst)[0] for the first pixel.
 * @param src_jump Offset in DATA32 units to the next row in the source image.
 * @param dst_jump Offset in DATA8 units (bytes) to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X-coordinate offset for the dithering matrix.
 * @param dith_y Y-coordinate offset for the dithering matrix.
 * @param pal Palette data (unused in this function).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_565_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
#ifndef BUILD_NO_DITHER_MASK
   DATA16 *d = (DATA16 *)dst;
   DATA32 r1, g1, b1;
   DATA32 r2, g2, b2;
   unsigned int dith, dith2;
   int x, y;

#ifdef BUILD_LINE_DITHER_MASK
   for (y = 0; y < h; y++)
     {
	if ((y + dith_y) & 0x1)
	  {
	     for (x = 0; x < w; x+=2)
	       {
		  DATA32  p = *src++,  q = *src++;
		  r1 = ((p & 0xff0000) + 0x030000) >> 19;
		  if (r1 > 0x1f) r1 = 0x1f;
		  g1 = ((p & 0xff00) + 0x000100) >> 10;
		  if (g1 > 0x3f) g1 = 0x3f;
		  b1 = ((p & 0xff) + 0x000003) >> 3;
		  if (b1 > 0x1f) b1 = 0x1f;
		  r2 = ((q & 0xff0000) + 0x030000) >> 19;
		  if (r2 > 0x1f) r2 = 0x1f;
		  g2 = ((q & 0xff00) + 0x000100) >> 10;
		  if (g2 > 0x3f) g2 = 0x3f;
		  b2 = ((q & 0xff) + 0x000003) >> 3;
		  if (b2 > 0x1f) b2 = 0x1f;
#ifndef WORDS_BIGENDIAN
		  *((DATA32 *)d) = (r2 << 27) | (g2 << 21) | (b2 << 16) |
		    (r1 << 11) | (g1 << 5) | (b1);
#else
		  *((DATA32 *)d) = (r1 << 27) | (g1 << 21) | (b1 << 16) |
		    (r2 << 11) | (g2 << 5) | (b2);
#endif
		  d += 2;
	       }
	  }
	else
	  {
	     x = w;
	     while (w > 0)
	       {
		  DATA32  p = *src++, q = *src++;

#ifndef WORDS_BIGENDIAN
		  *((DATA32 *)d) =
		    (((q & 0xff0000) >> 19) << 27) | (((q & 0xff00) >> 10) << 21) | (((q & 0xff) >> 3) << 16) |
		    (((p & 0xff0000) >> 19) << 11) | (((p & 0xff00) >> 10) << 5) | ((p & 0xff) >> 3);
#else
		  *((DATA32 *)d) =
		    (((p & 0xff0000) >> 19) << 27) | (((p & 0xff00) >> 10) << 21) | (((p & 0xff) >> 3) << 16) |
		    (((q & 0xff0000) >> 19) << 11) | (((q & 0xff00) >> 10) << 5) | ((q & 0xff) >> 3);
#endif
		  d += 2;  w -= 2;
	       }
	     w = x;
	  }
	src += src_jump;
	d += dst_jump;
     }
#else
   for (y = 0; y < h; y++)
     {
	for (x = 0; x < w; x++)
	  {
             DATA32  p = *src++,  q = *src++;

             dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK];
             dith2 = dith >> DM_SHF(6);
             dith >>= DM_SHF(5);
             r1 = (p & 0xff0000) >> 19;
             g1 = (p & 0xff00) >> 10;
             b1 = (p & 0xff) >> 3;
             if ((r1 < 0x1f) && ((((p & 0xff0000) >> 16) - (r1 << 3)) >= dith )) r1++;
             if ((g1 < 0x3f) && ((((p & 0xff00) >> 8) - (g1 << 2)) >= dith2)) g1++;
             if ((b1 < 0x1f) && (((p & 0xff) - (b1 << 3)) >= dith )) b1++;

             x++;
             dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK];
             dith2 = dith >> DM_SHF(6);
             dith >>= DM_SHF(5);
             r2 = (q & 0xff0000) >> 19;
             g2 = (q & 0xff00) >> 10;
             b2 = (q & 0xff) >> 3;
             if ((r2 < 0x1f) && ((((q & 0xff0000) >> 16) - (r2 << 3)) >= dith )) r2++;
             if ((g2 < 0x3f) && ((((q & 0xff00) >> 8) - (g2 << 2)) >= dith2)) g2++;
             if ((b2 < 0x1f) && (((q & 0xff) - (b2 << 3)) >= dith )) b2++;

#ifndef WORDS_BIGENDIAN
             *((DATA32 *)d) = (r2 << 27) | (g2 << 21) | (b2 << 16) |
	                      (r1 << 11) | (g1 << 5) | (b1);
#else
             *((DATA32 *)d) = (r1 << 27) | (g1 << 21) | (b1 << 16) |
	                      (r2 << 11) | (g2 << 5) | (b2);
#endif
             d += 2;
	  }
	src += src_jump;
	d += dst_jump;
     }
#endif
#else
   DATA16 *d = (DATA16 *)dst;
   int w0 = w;

   while (h--)
     {
	while (w > 0)
	  {
	     DATA32  p = *src++, q = *src++;

#ifndef WORDS_BIGENDIAN
             *((DATA32 *)d) =
                (((q & 0xff0000) >> 19) << 27) | (((q & 0xff00) >> 10) << 21) | (((q & 0xff) >> 3) << 16) |
                (((p & 0xff0000) >> 19) << 11) | (((p & 0xff00) >> 10) << 5) | ((p & 0xff) >> 3);
#else
             *((DATA32 *)d) =
                (((p & 0xff0000) >> 19) << 27) | (((p & 0xff00) >> 10) << 21) | (((p & 0xff) >> 3) << 16) |
                (((q & 0xff0000) >> 19) << 11) | (((q & 0xff00) >> 10) << 5) | ((q & 0xff) >> 3);
#endif
             d += 2;  w -= 2;
          }
	w = w0;
	src += src_jump;
	d += dst_jump;
     }
#endif
}

/**
 * @brief Converts a block of RGBA data (1 pixel at a time) to 16bpp RGB (5-6-5 format) with dithering.
 *
 * Processes one 32-bit RGBA source pixel and converts it into one 16-bit RGB_565
 * destination pixel. Dithering is applied to improve color accuracy.
 *
 * @param src Pointer to the source image data (array of DATA32). Each DATA32 is an ARGB pixel.
 * @param dst Pointer to the destination image data (array of DATA8, treated as DATA16).
 * @param src_jump Offset in DATA32 units to the next row in the source image.
 * @param dst_jump Offset in DATA8 units (bytes) to the next row in the destination image.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x X-coordinate offset for the dithering matrix.
 * @param dith_y Y-coordinate offset for the dithering matrix.
 * @param pal Palette data (unused in this function).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_565_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
#ifndef BUILD_NO_DITHER_MASK
   DATA16 *d = (DATA16 *)dst;
   DATA32 r, g, b;
   unsigned int dith, dith2;
   int x, y;

#ifdef BUILD_LINE_DITHER_MASK
   for (y = 0; y < h; y++)
     {
	if ((y + dith_y) & 0x1)
	  {
	     for (x = 0; x < w; x++)
	       {
		  DATA32  p = *src++;

		  r = (p & 0xff0000) >> 19;
		  if (r > 0x1f) r = 0x1f;
		  g = (p & 0xff00) >> 10;
		  if (g > 0x3f) g = 0x3f;
		  b = (p & 0xff) >> 3;
		  if (b > 0x1f) b = 0x1f;
		  *d++ = (r << 11) | (g << 5) | b;
	       }
	  }
	else
	  {
	     x = w;
	     while (w--)
	       {
		  *d++ = (((*src & 0xff0000) >> 19) << 11) | (((*src & 0xff00) >> 10) << 5) | ((*src & 0xff) >> 3);
		  src++;
	       }
	     w = x;
	  }
	src += src_jump;
	d += dst_jump;
     }
#else
   for (y = 0; y < h; y++)
     {
	for (x = 0; x < w; x++)
	  {
             DATA32  p = *src++;

             dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK];
             dith2 = dith >> DM_SHF(6);
             dith >>= DM_SHF(5);
             r = (p & 0xff0000) >> 19;
             g = (p & 0xff00) >> 10;
             b = (p & 0xff) >> 3;
             if ((r < 0x1f) && ((((p & 0xff0000) >> 16) - (r << 3)) >= dith )) r++;
             if ((g < 0x3f) && ((((p & 0xff00) >> 8) - (g << 2)) >= dith2)) g++;
             if ((b < 0x1f) && (((p & 0xff) - (b << 3)) >= dith )) b++;

             *d++ = (r << 11) | (g << 5) | b;
	  }
	src += src_jump;
	d += dst_jump;
     }
#endif
#else
   DATA16 *d = (DATA16 *)dst;
   int w0 = w;

   while (h--)
     {
	while (w--)
	  {
             *d++ = (((*src & 0xff0000) >> 19) << 11) | (((*src & 0xff00) >> 10) << 5) | ((*src & 0xff) >> 3);
             src++;
          }
	w = w0;
	src += src_jump;
	d += dst_jump;
     }
#endif
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-6-5) with dithering and 180-degree rotation.
 *
 * Similar to evas_common_convert_rgba2_to_16bpp_rgb_565_dith, but the output is
 * rotated by 180 degrees. Source pixels are read in reverse order.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_565_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_180();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_180();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 27) | (g2 << 21) | (b2 << 16) |
     (r1 << 11) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 27) | (g1 << 21) | (b1 << 16) |
     (r2 << 11) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_180();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-6-5) with dithering and 180-degree rotation.
 *
 * Similar to evas_common_convert_rgba_to_16bpp_rgb_565_dith, but the output is
 * rotated by 180 degrees. Source pixels are read in reverse order.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_565_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_180();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 11) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_180();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-6-5) with dithering and 270-degree rotation.
 *
 * Similar to evas_common_convert_rgba2_to_16bpp_rgb_565_dith, but the output is
 * rotated by 270 degrees (clockwise).
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image (becomes height of destination).
 * @param h Height of the source image (becomes width of destination).
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_565_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_270();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_270();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 27) | (g2 << 21) | (b2 << 16) |
     (r1 << 11) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 27) | (g1 << 21) | (b1 << 16) |
     (r2 << 11) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_270();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-6-5) with dithering and 270-degree rotation.
 *
 * Similar to evas_common_convert_rgba_to_16bpp_rgb_565_dith, but the output is
 * rotated by 270 degrees (clockwise).
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image (becomes height of destination).
 * @param h Height of the source image (becomes width of destination).
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_565_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_270();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 11) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_270();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-6-5) with dithering and 90-degree rotation.
 *
 * Similar to evas_common_convert_rgba2_to_16bpp_rgb_565_dith, but the output is
 * rotated by 90 degrees (clockwise).
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image (becomes height of destination).
 * @param h Height of the source image (becomes width of destination).
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_565_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_90();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_90();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 27) | (g2 << 21) | (b2 << 16) |
     (r1 << 11) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 27) | (g1 << 21) | (b1 << 16) |
     (r2 << 11) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_90();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-6-5) with dithering and 90-degree rotation.
 *
 * Similar to evas_common_convert_rgba_to_16bpp_rgb_565_dith, but the output is
 * rotated by 90 degrees (clockwise).
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image (becomes height of destination).
 * @param h Height of the source image (becomes width of destination).
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_565_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_90();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 11) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_90();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp BGR (5-6-5 format) with dithering.
 *
 * Similar to evas_common_convert_rgba2_to_16bpp_rgb_565_dith, but the output color order
 * is BGR instead of RGB.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_bgr_565_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_0();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_0();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (b2 << 27) | (g2 << 21) | (r2 << 16) |
     (b1 << 11) | (g1 << 5 ) | (r1      );
#else
   *((DATA32 *)dst_ptr) =
     (b1 << 27) | (g1 << 21) | (r1 << 16) |
     (b2 << 11) | (g2 << 5 ) | (r2      );
#endif

   CONVERT_LOOP2_END_ROT_0();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp BGR (5-6-5 format) with dithering.
 *
 * Similar to evas_common_convert_rgba_to_16bpp_rgb_565_dith, but the output color order
 * is BGR instead of RGB.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_bgr_565_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (b << 11) | (g << 5) | (r);

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp BGR (5-6-5) with dithering and 180-degree rotation.
 *
 * Combines BGR conversion, 2-pixel processing, dithering, and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_bgr_565_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_180();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_180();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (b2 << 27) | (g2 << 21) | (r2 << 16) |
     (b1 << 11) | (g1 << 5 ) | (r1      );
#else
   *((DATA32 *)dst_ptr) =
     (b1 << 27) | (g1 << 21) | (r1 << 16) |
     (b2 << 11) | (g2 << 5 ) | (r2      );
#endif

   CONVERT_LOOP2_END_ROT_180();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp BGR (5-6-5) with dithering and 180-degree rotation.
 *
 * Combines BGR conversion, 1-pixel processing, dithering, and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_bgr_565_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   ERR("evas_common_convert_rgba_to_16bpp_bgr_565_dith_rot_180");

   CONVERT_LOOP_START_ROT_180();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (b << 11) | (g << 5) | (r);

   CONVERT_LOOP_END_ROT_180();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp BGR (5-6-5) with dithering and 270-degree rotation.
 *
 * Combines BGR conversion, 2-pixel processing, dithering, and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_bgr_565_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_270();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_270();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (b2 << 27) | (g2 << 21) | (r2 << 16) |
     (b1 << 11) | (g1 << 5 ) | (r1      );
#else
   *((DATA32 *)dst_ptr) =
     (b1 << 27) | (g1 << 21) | (r1 << 16) |
     (b2 << 11) | (g2 << 5 ) | (r2      );
#endif

   CONVERT_LOOP2_END_ROT_270();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp BGR (5-6-5) with dithering and 270-degree rotation.
 *
 * Combines BGR conversion, 1-pixel processing, dithering, and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_bgr_565_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_270();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (b << 11) | (g << 5) | (r);

   CONVERT_LOOP_END_ROT_270();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp BGR (5-6-5) with dithering and 90-degree rotation.
 *
 * Combines BGR conversion, 2-pixel processing, dithering, and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_bgr_565_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_90();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 2;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith ) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 2)) >= dith2) && (g1 < 0x3f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith ) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_90();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 2;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith ) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 2)) >= dith2) && (g2 < 0x3f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith ) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (b2 << 27) | (g2 << 21) | (r2 << 16) |
     (b1 << 11) | (g1 << 5 ) | (r1      );
#else
   *((DATA32 *)dst_ptr) =
     (b1 << 27) | (g1 << 21) | (r1 << 16) |
     (b2 << 11) | (g2 << 5 ) | (r2      );
#endif

   CONVERT_LOOP2_END_ROT_90();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp BGR (5-6-5) with dithering and 90-degree rotation.
 *
 * Combines BGR conversion, 1-pixel processing, dithering, and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_bgr_565_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith, dith2;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_90();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 2;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   dith2 = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(6);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith ) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 2)) >= dith2) && (g < 0x3f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith ) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (b << 11) | (g << 5) | (r);

   CONVERT_LOOP_END_ROT_90();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (4-4-4 format) with dithering.
 *
 * Each color channel (Red, Green, Blue) is reduced to 4 bits.
 * Processes two pixels at a time.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_444_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_0();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_0();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 24) | (g2 << 20) | (b2 << 16) |
     (r1 << 8 ) | (g1 << 4 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 24) | (g1 << 20) | (b1 << 16) |
     (r2 << 8 ) | (g2 << 4 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_0();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (4-4-4 format) with dithering.
 *
 * Each color channel (Red, Green, Blue) is reduced to 4 bits.
 * Processes one pixel at a time.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_444_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 8) | (g << 4) | (b);

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (4-4-4) with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_444_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_180();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_180();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 24) | (g2 << 20) | (b2 << 16) |
     (r1 << 8 ) | (g1 << 4 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 24) | (g1 << 20) | (b1 << 16) |
     (r2 << 8 ) | (g2 << 4 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_180();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (4-4-4) with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_444_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_180();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 8) | (g << 4) | (b);

   CONVERT_LOOP_END_ROT_180();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (4-4-4) with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_444_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_270();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_270();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 24) | (g2 << 20) | (b2 << 16) |
     (r1 << 8 ) | (g1 << 4 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 24) | (g1 << 20) | (b1 << 16) |
     (r2 << 8 ) | (g2 << 4 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_270();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (4-4-4) with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_444_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_270();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 8) | (g << 4) | (b);

   CONVERT_LOOP_END_ROT_270();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (4-4-4) with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_444_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_90();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_90();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 24) | (g2 << 20) | (b2 << 16) |
     (r1 << 8 ) | (g1 << 4 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 24) | (g1 << 20) | (b1 << 16) |
     (r2 << 8 ) | (g2 << 4 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_90();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (4-4-4) with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_444_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_90();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 8) | (g << 4) | (b);

   CONVERT_LOOP_END_ROT_90();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to a custom 16bpp RGB (R:4, G:5, B:4 or R:5, G:6, B:5 like) format with dithering.
 *
 * The exact bit allocation (R:4 G:5 B:4 for first pixel, R:6 G:4 B:5 for second, or similar, based on shifts)
 * is specific to this function's implementation details (r << 12 | g << 7 | b << 1 and r << 28 | g << 23 | b << 17).
 * This appears to be a packed format where two pixels might share bits or have slightly different distributions.
 * The name "454645" is unusual and might indicate a specific hardware target or a packed pixel format.
 * For the first pixel: R is shifted by 12, G by 7, B by 1.
 * For the second pixel: R is shifted by 28, G by 23, B by 17 when packed into a DATA32.
 * This implies a structure like: [R2(4) G2(5) B2(5)] [R1(4) G1(5) B1(5)] (approximate bits per channel).
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_454645_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_0();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_0();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 28) | (g2 << 23) | (b2 << 17) |
     (r1 << 12) | (g1 << 7 ) | (b1 << 1 );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 28) | (g1 << 23) | (b1 << 17) |
     (r2 << 12) | (g2 << 7 ) | (b2 << 1 );
#endif

   CONVERT_LOOP2_END_ROT_0();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to a custom 16bpp RGB (R:4, G:5, B:4 or R:5, G:6, B:5 like) format with dithering.
 *
 * The pixel format is (R << 12) | (G << 7) | (B << 1). This suggests R:4 bits, G:5 bits, B:5 bits, with the LSB unused or for alpha.
 * The name "454645" might be a misnomer for single pixel conversion or refers to a family of formats.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_454645_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 12) | (g << 7) | (b << 1);

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (custom "454645") with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_454645_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_180();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_180();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 28) | (g2 << 23) | (b2 << 17) |
     (r1 << 12) | (g1 << 7 ) | (b1 << 1 );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 28) | (g1 << 23) | (b1 << 17) |
     (r2 << 12) | (g2 << 7 ) | (b2 << 1 );
#endif

   CONVERT_LOOP2_END_ROT_180();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (custom "454645") with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_454645_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_180();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 12) | (g << 7) | (b << 1);

   CONVERT_LOOP_END_ROT_180();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (custom "454645") with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_454645_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_270();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_270();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 28) | (g2 << 23) | (b2 << 17) |
     (r1 << 12) | (g1 << 7 ) | (b1 << 1 );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 28) | (g1 << 23) | (b1 << 17) |
     (r2 << 12) | (g2 << 7 ) | (b2 << 1 );
#endif

   CONVERT_LOOP2_END_ROT_270();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (custom "454645") with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_454645_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_270();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 12) | (g << 7) | (b << 1);

   CONVERT_LOOP_END_ROT_270();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (custom "454645") with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_454645_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_90();

   r1 = (R_VAL(src_ptr)) >> 4;
   g1 = (G_VAL(src_ptr)) >> 4;
   b1 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r1 << 4)) >= dith ) && (r1 < 0x0f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 4)) >= dith ) && (g1 < 0x0f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 4)) >= dith ) && (b1 < 0x0f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_90();

   r2 = (R_VAL(src_ptr)) >> 4;
   g2 = (G_VAL(src_ptr)) >> 4;
   b2 = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r2 << 4)) >= dith ) && (r2 < 0x0f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 4)) >= dith ) && (g2 < 0x0f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 4)) >= dith ) && (b2 < 0x0f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 28) | (g2 << 23) | (b2 << 17) |
     (r1 << 12) | (g1 << 7 ) | (b1 << 1 );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 28) | (g1 << 23) | (b1 << 17) |
     (r2 << 12) | (g2 << 7 ) | (b2 << 1 );
#endif

   CONVERT_LOOP2_END_ROT_90();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (custom "454645") with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_454645_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_90();

   r = (R_VAL(src_ptr)) >> 4;
   g = (G_VAL(src_ptr)) >> 4;
   b = (B_VAL(src_ptr)) >> 4;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(4);
   if (((R_VAL(src_ptr) - (r << 4)) >= dith ) && (r < 0x0f)) r++;
   if (((G_VAL(src_ptr) - (g << 4)) >= dith ) && (g < 0x0f)) g++;
   if (((B_VAL(src_ptr) - (b << 4)) >= dith ) && (b < 0x0f)) b++;
#endif

   *dst_ptr = (r << 12) | (g << 7) | (b << 1);

   CONVERT_LOOP_END_ROT_90();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-5-5 format) with dithering.
 *
 * Each color channel (Red, Green, Blue) is reduced to 5 bits. The highest bit of the 16-bit word is often unused or for alpha.
 * Processes two pixels at a time.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_555_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_0();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 3;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 3)) >= dith) && (g1 < 0x1f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_0();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 3;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 3)) >= dith) && (g2 < 0x1f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 26) | (g2 << 21) | (b2 << 16) |
     (r1 << 10) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 26) | (g1 << 21) | (b1 << 16) |
     (r2 << 10) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_0();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-5-5 format) with dithering.
 *
 * Each color channel (Red, Green, Blue) is reduced to 5 bits.
 * Processes one pixel at a time.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_555_dith (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_0();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 3;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 3)) >= dith) && (g < 0x1f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 10) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_0();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-5-5) with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_555_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_180();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 3;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 3)) >= dith) && (g1 < 0x1f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_180();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 3;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 3)) >= dith) && (g2 < 0x1f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 26) | (g2 << 21) | (b2 << 16) |
     (r1 << 10) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 26) | (g1 << 21) | (b1 << 16) |
     (r2 << 10) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_180();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-5-5) with dithering and 180-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the image.
 * @param h Height of the image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_555_dith_rot_180 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_180();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 3;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 3)) >= dith) && (g < 0x1f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 10) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_180();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-5-5) with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_555_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_270();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 3;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 3)) >= dith) && (g1 < 0x1f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_270();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 3;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 3)) >= dith) && (g2 < 0x1f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 26) | (g2 << 21) | (b2 << 16) |
     (r1 << 10) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 26) | (g1 << 21) | (b1 << 16) |
     (r2 << 10) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_270();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-5-5) with dithering and 270-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_555_dith_rot_270 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_270();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 3;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 3)) >= dith) && (g < 0x1f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 10) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_270();
}

/**
 * @brief Converts RGBA data (2 pixels at a time) to 16bpp RGB (5-5-5) with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba2_to_16bpp_rgb_555_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r1, g1, b1;
   DATA8 r2, g2, b2;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP2_START_ROT_90();

   r1 = (R_VAL(src_ptr)) >> 3;
   g1 = (G_VAL(src_ptr)) >> 3;
   b1 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r1 << 3)) >= dith) && (r1 < 0x1f)) r1++;
   if (((G_VAL(src_ptr) - (g1 << 3)) >= dith) && (g1 < 0x1f)) g1++;
   if (((B_VAL(src_ptr) - (b1 << 3)) >= dith) && (b1 < 0x1f)) b1++;
#endif

   CONVERT_LOOP2_INC_ROT_90();

   r2 = (R_VAL(src_ptr)) >> 3;
   g2 = (G_VAL(src_ptr)) >> 3;
   b2 = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r2 << 3)) >= dith) && (r2 < 0x1f)) r2++;
   if (((G_VAL(src_ptr) - (g2 << 3)) >= dith) && (g2 < 0x1f)) g2++;
   if (((B_VAL(src_ptr) - (b2 << 3)) >= dith) && (b2 < 0x1f)) b2++;
#endif

#ifndef WORDS_BIGENDIAN
   *((DATA32 *)dst_ptr) =
     (r2 << 26) | (g2 << 21) | (b2 << 16) |
     (r1 << 10) | (g1 << 5 ) | (b1      );
#else
   *((DATA32 *)dst_ptr) =
     (r1 << 26) | (g1 << 21) | (b1 << 16) |
     (r2 << 10) | (g2 << 5 ) | (b2      );
#endif

   CONVERT_LOOP2_END_ROT_90();
}

/**
 * @brief Converts RGBA data (1 pixel at a time) to 16bpp RGB (5-5-5) with dithering and 90-degree rotation.
 *
 * @param src Pointer to the source image data.
 * @param dst Pointer to the destination image data.
 * @param src_jump Source row stride.
 * @param dst_jump Destination row stride.
 * @param w Width of the source image.
 * @param h Height of the source image.
 * @param dith_x Dithering X offset.
 * @param dith_y Dithering Y offset.
 * @param pal Palette data (unused).
 */
void
evas_common_convert_rgba_to_16bpp_rgb_555_dith_rot_90 (DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x, int dith_y, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA16 *dst_ptr;
   int x, y;
   DATA8 r, g, b;
#ifndef BUILD_NO_DITHER_MASK
   DATA8 dith;
#endif

   dst_ptr = (DATA16 *)dst;

   CONVERT_LOOP_START_ROT_90();

   r = (R_VAL(src_ptr)) >> 3;
   g = (G_VAL(src_ptr)) >> 3;
   b = (B_VAL(src_ptr)) >> 3;

#ifndef BUILD_NO_DITHER_MASK
   dith = DM_TABLE[(x + dith_x) & DM_MSK][(y + dith_y) & DM_MSK] >> DM_SHF(5);
   if (((R_VAL(src_ptr) - (r << 3)) >= dith) && (r < 0x1f)) r++;
   if (((G_VAL(src_ptr) - (g << 3)) >= dith) && (g < 0x1f)) g++;
   if (((B_VAL(src_ptr) - (b << 3)) >= dith) && (b < 0x1f)) b++;
#endif

   *dst_ptr = (r << 10) | (g << 5) | (b);

   CONVERT_LOOP_END_ROT_90();
}
