#include "evas_common_private.h"
#include "evas_convert_rgb_24.h"

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp RGB (888) format.
 *
 * This function takes a source image in RGBA format (where each pixel is
 * represented by a DATA32 value) and converts it to a 24-bit RGB format.
 * In this RGB 888 format, each pixel is represented by three DATA8 values
 * (Red, Green, Blue), with 8 bits per channel. The alpha channel from the
 * source is discarded.
 *
 * @param src Pointer to the source image data (array of DATA32).
 *            Each DATA32 element represents one pixel in ARGB order,
 *            e.g., 0xAARRGGBB.
 * @param dst Pointer to the destination buffer for 24bpp RGB data (array of DATA8).
 *            The output will be R, G, B, R, G, B, ...
 *            Example for one pixel: [R_val, G_val, B_val]
 * @param src_jump Number of DATA32 elements to skip to get to the next row in src.
 * @param dst_jump Number of DATA8 elements (bytes) to skip to get to the next row in dst.
 *                 Note: This is the jump in terms of individual DATA8 elements, not pixels.
 *                 So, for a tightly packed destination, this would be 0 if w * 3 fills the row.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Unused dither x offset.
 * @param dith_y Unused dither y offset.
 * @param pal Unused palette data.
 */
void
evas_common_convert_rgba_to_24bpp_rgb_888(DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;

   dst_ptr = (DATA8 *)dst;
   src_ptr = (DATA32 *)src;

   for (y = 0; y < h; y++)
     {
	for (x = 0; x < w; x++)
	  {
	     dst_ptr[0] = R_VAL(src_ptr);
	     dst_ptr[1] = G_VAL(src_ptr);
	     dst_ptr[2] = B_VAL(src_ptr);
	     src_ptr++;
	     dst_ptr+=3;
	  }
	src_ptr += src_jump;
	dst_ptr += dst_jump * 3;
     }
   return;
}

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp RGB (666) format.
 *
 * This function takes a source image in RGBA format and converts it to a
 * 24-bit RGB format where each color channel (Red, Green, Blue) is
 * represented by 6 bits. The most significant 6 bits of each 8-bit channel
 * from the source are used. The output is packed into three DATA8 values
 * per pixel. The alpha channel from the source is discarded.
 *
 * The conversion logic for packing 6 bits per channel into 3 bytes is:
 *   scratch = (((R_VAL(src_ptr) << 12) | (B_VAL(src_ptr) >> 2)) & 0x03f03f) |
 *             ((G_VAL(src_ptr) << 4) & 0x000fc0);
 * This effectively means:
 *   Byte 0 (dst_ptr[0]): GGGGBBBB (G_VAL bits 7-4, B_VAL bits 7-4) - from scratch_ptr[1]
 *   Byte 1 (dst_ptr[1]): RRRRGGGG (R_VAL bits 7-4, G_VAL bits 3-0) - from scratch_ptr[2]
 *   Byte 2 (dst_ptr[2]): ----RRRR (R_VAL bits 3-0) - from scratch_ptr[3]
 * Note: The actual bit manipulation `(X_VAL(src_ptr) >> 2)` would take the top 6 bits.
 * The current implementation uses a specific bit manipulation that might be
 * platform or endianness dependent for how `scratch` (a DATA32) is interpreted
 * by `scratch_ptr` (a DATA8*).
 *
 * @param src Pointer to the source image data (array of DATA32).
 *            Each DATA32 element represents one pixel in ARGB order.
 * @param dst Pointer to the destination buffer for 24bpp RGB data (array of DATA8).
 *            The output will be 3 bytes per pixel, storing the 6-bit R, G, B values.
 *            Example for one pixel: [byte1, byte2, byte3] containing packed R,G,B data.
 * @param src_jump Number of DATA32 elements to skip to get to the next row in src.
 * @param dst_jump Number of DATA8 elements (bytes) to skip to get to the next row in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Unused dither x offset.
 * @param dith_y Unused dither y offset.
 * @param pal Unused palette data.
 */
void
evas_common_convert_rgba_to_24bpp_rgb_666(DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr, *scratch_ptr;
   DATA32 scratch;
   int x, y;

   dst_ptr = (DATA8 *)dst;
   src_ptr = (DATA32 *)src;

   scratch_ptr = (DATA8 *)(&scratch);
   for (y = 0; y < h; y++)
     {
	for (x = 0; x < w; x++)
	  {
             scratch =
               (((R_VAL(src_ptr) << 12) | (B_VAL(src_ptr) >> 2)) & 0x03f03f) |
               ((G_VAL(src_ptr) << 4) & 0x000fc0);
	     dst_ptr[0] = scratch_ptr[1];
	     dst_ptr[1] = scratch_ptr[2];
	     dst_ptr[2] = scratch_ptr[3];
	     src_ptr++;
	     dst_ptr+=3;
	  }
	src_ptr += src_jump;
	dst_ptr += dst_jump * 3;
     }
   return;
}

/**
 * @brief Converts RGBA (32-bit) image data to 24bpp BGR (888) format.
 *
 * This function takes a source image in RGBA format (where each pixel is
 * represented by a DATA32 value) and converts it to a 24-bit BGR format.
 * In this BGR 888 format, each pixel is represented by three DATA8 values
 * (Blue, Green, Red), with 8 bits per channel. The alpha channel from the
 * source is discarded. This is similar to RGB 888 but with the byte order
 * for Red and Blue swapped.
 *
 * @param src Pointer to the source image data (array of DATA32).
 *            Each DATA32 element represents one pixel in ARGB order.
 * @param dst Pointer to the destination buffer for 24bpp BGR data (array of DATA8).
 *            The output will be B, G, R, B, G, R, ...
 *            Example for one pixel: [B_val, G_val, R_val]
 * @param src_jump Number of DATA32 elements to skip to get to the next row in src.
 * @param dst_jump Number of DATA8 elements (bytes) to skip to get to the next row in dst.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param dith_x Unused dither x offset.
 * @param dith_y Unused dither y offset.
 * @param pal Unused palette data.
 */
void
evas_common_convert_rgba_to_24bpp_bgr_888(DATA32 *src, DATA8 *dst, int src_jump, int dst_jump, int w, int h, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
   DATA32 *src_ptr;
   DATA8 *dst_ptr;
   int x, y;

   dst_ptr = (DATA8 *)dst;
   src_ptr = (DATA32 *)src;

   for (y = 0; y < h; y++)
     {
	for (x = 0; x < w; x++)
	  {
	     dst_ptr[2] = R_VAL(src_ptr);
	     dst_ptr[1] = G_VAL(src_ptr);
	     dst_ptr[0] = B_VAL(src_ptr);
	     src_ptr++;
	     dst_ptr+=3;
	  }
	src_ptr += src_jump;
	dst_ptr += dst_jump * 3;
     }
   return;
}
