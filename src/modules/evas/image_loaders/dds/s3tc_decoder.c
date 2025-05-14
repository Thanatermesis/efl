/**
 * @file
 * @brief S3TC (S3 Texture Compression) decoder implementation.
 *
 * Provides functions to decode various DXT formats (DXT1, DXT2, DXT3, DXT4, DXT5)
 * into raw BGRA pixel data. Also includes functions to flip encoded S3TC blocks
 * horizontally or vertically.
 */

#include "s3tc.h"

// For INTERP_256 and INTERP_RGB_256
#include "evas_common_private.h"
#include "evas_blend_ops.h"

// From evas_convert_colorspace.c
#define CONVERT_RGB_565_TO_RGB_888(s) \
        (((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7)) | \
         ((((s) << 5) & 0xfc00) | (((s) >> 1) & 0x300)) | \
         ((((s) << 8) & 0xf80000) | (((s) << 3) & 0x70000)))
/**
 * @brief Replicates a 4-bit alpha value to 8 bits.
 * @param a The 4-bit alpha value (0-15).
 * @return The 8-bit alpha value (0-255).
 * @details Example: ALPHA4(0xA) -> 0xAA
 */
#define ALPHA4(a) (((a) << 4) | (a))

/**
 * @internal
 * @brief Decodes a DXT1 or DXT3/DXT5 color block (4x4 pixels).
 * @param bgra Pointer to the destination buffer (16 pixels, BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 8-byte S3TC color block data.
 *             Example: `const unsigned char dxt_color_block[8];`
 * @param amask Alpha mask to apply to opaque pixels (e.g., 0xFF000000).
 * @param dxt1 EINA_TRUE if decoding DXT1, EINA_FALSE otherwise.
 * @param alpha EINA_TRUE if DXT1 alpha is enabled (1-bit alpha).
 * @details Decodes the 565 RGB colors and the 2-bit indices for a 4x4 block.
 *          Handles the DXT1 specific cases (3 colors + transparent black).
 */
static void
_decode_dxt1_rgb(unsigned int *bgra, const unsigned char *s3tc,
                 unsigned int amask, Eina_Bool dxt1, Eina_Bool alpha)
{
   unsigned short color0, color1;
   unsigned int colors[4];
   unsigned int bits;

   color0 = s3tc[0] | (s3tc[1] << 8);
   color1 = s3tc[2] | (s3tc[3] << 8);

   colors[0] = amask | CONVERT_RGB_565_TO_RGB_888(color0);
   colors[1] = amask | CONVERT_RGB_565_TO_RGB_888(color1);
   if (!dxt1 || (color0 > color1))
     {
        // This is what's not supported by S2TC.
        colors[2] = amask | INTERP_RGB_256((2*256)/3, colors[0], colors[1]);
        colors[3] = amask | INTERP_RGB_256((1*256)/3, colors[0], colors[1]);
     }
   else
     {
        colors[2] = amask | INTERP_RGB_256(128, colors[0], colors[1]);
        colors[3] = (alpha ? 0x00000000 : amask);
     }

   bits = s3tc[4] + ((s3tc[5] + ((s3tc[6] + (s3tc[7] << 8)) << 8)) << 8);
   for (int j = 0; j < 4; j++)
     for (int i = 0; i < 4; i++)
       {
          int idx = bits & 0x3;
          bgra[(j * 4) + i] = colors[idx];
          bits >>= 2;
       }
}

/**
 * @internal
 * @brief Decodes the explicit 4-bit alpha block (DXT2/DXT3).
 * @param bgra Pointer to the destination buffer (16 pixels, BGRA format).
 *             The existing RGB values will be OR-ed with the decoded alpha.
 *             Example: `unsigned int output_pixels[16];` (already contains RGB)
 * @param s3tc Pointer to the 8-byte S3TC explicit alpha block data.
 *             Example: `const unsigned char dxt_alpha_block[8];`
 * @details Reads 16 x 4-bit alpha values and expands them to 8 bits,
 *          setting the alpha channel of the corresponding pixels in `bgra`.
 */
static void
_decode_alpha4(unsigned int *bgra, const unsigned char *s3tc)
{
   // Process 2 pixels (8 bits of alpha data) per iteration.
   for (int k = 0; k < 16; k += 2)
     {
        unsigned int a0 = ALPHA4((*s3tc) & 0x0F);
        unsigned int a1 = ALPHA4(((*s3tc) & 0xF0) >> 4);
        *bgra++ |= (a0 << 24);
        *bgra++ |= (a1 << 24);
        s3tc++;
     }
}

/**
 * @internal
 * @brief Decodes the interpolated 8-bit alpha block (DXT4/DXT5).
 * @param bgra Pointer to the destination buffer (16 pixels, BGRA format).
 *             The existing RGB values will be OR-ed with the decoded alpha.
 *             Example: `unsigned int output_pixels[16];` (already contains RGB)
 * @param s3tc Pointer to the 8-byte S3TC interpolated alpha block data.
 *             Example: `const unsigned char dxt_alpha_block[8];`
 * @details Reads two 8-bit reference alpha values and 16 x 3-bit indices.
 *          Calculates the 8 possible alpha values based on the reference values
 *          and sets the alpha channel of the corresponding pixels in `bgra`.
 */
static void
_decode_dxt_alpha(unsigned int *bgra, const unsigned char *s3tc)
{
   unsigned char a0 = s3tc[0]; // alpha_0
   unsigned char a1 = s3tc[1];
   unsigned long long bits = 0ull;
   unsigned char alpha[8];

   for (int k = 5; k >= 0; k--)
     bits = (bits << 8) | s3tc[k + 2];

   alpha[0] = a0;
   alpha[1] = a1;

   if (a0 > a1)
     {
        for (int k = 0; k < 6; k++)
          alpha[2 + k] = ((6 - k) * a0 + (k + 1) * a1) / 7;
     }
   else
     {
        for (int k = 0; k < 4; k++)
          alpha[2 + k] = ((4 - k) * a0 + (k + 1) * a1) / 5;
        alpha[6] = 0;
        alpha[7] = 255;
     }

   for (int k = 0; k < 16; k++)
     {
        int index = (int) (bits & 0x7ull);
        *bgra++ |= (alpha[index] << 24);
        bits >>= 3;
     }
}

/**
 * @brief Decodes a DXT1 block (RGB, no alpha).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 8-byte S3TC DXT1 block data.
 *             Example: `const unsigned char dxt1_block[8];`
 * @details Decodes a DXT1 block assuming fully opaque alpha (0xFF).
 */
void
s3tc_decode_dxt1_rgb(unsigned int *bgra, const unsigned char *s3tc)
{
   // Use DXT1 decoding, force opaque alpha (amask), disable DXT1 1-bit alpha.
   _decode_dxt1_rgb(bgra, s3tc, 0xFF000000, EINA_TRUE, EINA_FALSE);
}

/**
 * @brief Decodes a DXT1 block with 1-bit alpha (RGBA).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 8-byte S3TC DXT1 block data.
 *             Example: `const unsigned char dxt1a_block[8];`
 * @details Decodes a DXT1 block, handling the 1-bit alpha case where
 *          color index 3 maps to transparent black (0x00000000).
 */
void
s3tc_decode_dxt1_rgba(unsigned int *bgra, const unsigned char *s3tc)
{
   // Use DXT1 decoding, force opaque alpha (amask) for non-transparent pixels,
   // enable DXT1 1-bit alpha interpretation.
   _decode_dxt1_rgb(bgra, s3tc, 0xFF000000, EINA_TRUE, EINA_TRUE);
}

/**
 * @brief Decodes a DXT2 block (RGBA, explicit 4-bit alpha, premultiplied).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 16-byte S3TC DXT2 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char dxt2_block[16];`
 * @details Decodes the color part (like DXT3) and then the explicit alpha part.
 *          Note: DXT2 implies premultiplied alpha, but this decoder outputs
 *          non-premultiplied RGBA. The caller might need to handle premultiplication.
 */
void
s3tc_decode_dxt2_rgba(unsigned int *bgra, const unsigned char *s3tc)
{
   // Decode color part (like DXT3: non-DXT1, no 1-bit alpha, zero base alpha)
   _decode_dxt1_rgb(bgra, s3tc + 8, 0x0, EINA_FALSE, EINA_FALSE);
   // Decode explicit 4-bit alpha part
   _decode_alpha4(bgra, s3tc);
}

/**
 * @brief Decodes a DXT3 block (RGBA, explicit 4-bit alpha).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 16-byte S3TC DXT3 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char dxt3_block[16];`
 * @details Decodes the color part and then the explicit alpha part.
 */
void
s3tc_decode_dxt3_rgba(unsigned int *bgra, const unsigned char *s3tc)
{
   // Decode color part (non-DXT1, no 1-bit alpha, zero base alpha)
   _decode_dxt1_rgb(bgra, s3tc + 8, 0x0, EINA_FALSE, EINA_FALSE);
   // Decode explicit 4-bit alpha part
   _decode_alpha4(bgra, s3tc);
}

/**
 * @brief Decodes a DXT4 block (RGBA, interpolated alpha, premultiplied).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 16-byte S3TC DXT4 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char dxt4_block[16];`
 * @details Decodes the color part (like DXT5) and then the interpolated alpha part.
 *          Note: DXT4 implies premultiplied alpha, but this decoder outputs
 *          non-premultiplied RGBA. The caller might need to handle premultiplication.
 */
void
s3tc_decode_dxt4_rgba(unsigned int *bgra, const unsigned char *s3tc)
{
   // Decode color part (like DXT5: non-DXT1, no 1-bit alpha, zero base alpha)
   _decode_dxt1_rgb(bgra, s3tc + 8, 0x0, EINA_FALSE, EINA_FALSE);
   // Decode interpolated alpha part
   _decode_dxt_alpha(bgra, s3tc);
}

/**
 * @brief Decodes a DXT5 block (RGBA, interpolated alpha).
 * @param bgra Pointer to the destination buffer for 16 pixels (BGRA format).
 *             Example: `unsigned int output_pixels[16];`
 * @param s3tc Pointer to the 16-byte S3TC DXT5 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char dxt5_block[16];`
 * @details Decodes the color part and then the interpolated alpha part.
 */
void
s3tc_decode_dxt5_rgba(unsigned int *bgra, const unsigned char *s3tc)
{
   // Decode color part (non-DXT1, no 1-bit alpha, zero base alpha)
   _decode_dxt1_rgb(bgra, s3tc + 8, 0x0, EINA_FALSE, EINA_FALSE);
   // Decode interpolated alpha part
   _decode_dxt_alpha(bgra, s3tc);
}

/** @name S3TC Block Flipping Functions
 * @{
 * Functions to reorder data within compressed S3TC blocks to achieve
 * horizontal or vertical flipping without full decompression/recompression.
 */

/**
 * @internal
 * @brief Flips the order of four 2-bit values within a byte.
 * @param src Source byte containing four 2-bit values (e.g., DXT1 indices).
 *            Layout: [aa bb cc dd]
 * @return Byte with flipped 2-bit values.
 *            Layout: [dd cc bb aa]
 * @details Used for horizontal flipping of DXT1 color index data.
 */
static inline unsigned char
_byte_2222_flip(unsigned char src)
{
   // Example: src = 11 10 01 00 (binary) = 0xE4
   //          ret = 00 01 10 11 (binary) = 0x1B
   return ((src & (0x3 << 6)) >> 6) | // dd -> aa
         ((src & (0x3 << 4)) >> 2) |
         ((src & (0x3 << 2)) << 2) |
         ((src & 0x3) << 6);
}

/**
 * @internal
 * @brief Flips the order of two 4-bit values within a byte.
 * @param src Source byte containing two 4-bit values (e.g., DXT3 alpha).
 *            Layout: [aaaa bbbb]
 * @return Byte with flipped 4-bit values.
 *            Layout: [bbbb aaaa]
 * @details Used for horizontal flipping of DXT3 explicit alpha data.
 */
static inline unsigned char
_byte_44_flip(unsigned char src)
{
   // Example: src = 1111 0000 (binary) = 0xF0
   //          ret = 0000 1111 (binary) = 0x0F
   return ((src & 0xF0) >> 4) | ((src & 0x0F) << 4);
}

/**
 * @brief Flips an encoded DXT1 block horizontally or vertically.
 * @param dest Pointer to the destination buffer for the flipped 8-byte block.
 *             Example: `unsigned char flipped_dxt1_block[8];`
 * @param orig Pointer to the original 8-byte S3TC DXT1 block data.
 *             Example: `const unsigned char original_dxt1_block[8];`
 * @param vflip Non-zero for vertical flip, zero for horizontal flip.
 * @details Reorders the color index bytes to achieve the flip. Color endpoints
 *          remain unchanged.
 */
void
s3tc_encode_dxt1_flip(unsigned char *dest, const unsigned char *orig, int vflip)
{
   // color0 and color1 are unchanged
   dest[0] = orig[0];
   dest[1] = orig[1];
   dest[2] = orig[2];
   dest[3] = orig[3];

   if (vflip)
     {
        dest[4] = orig[7];
        dest[5] = orig[6];
        dest[6] = orig[5];
        dest[7] = orig[4];
     }
   else
     {
        dest[4] = _byte_2222_flip(orig[4]);
        dest[5] = _byte_2222_flip(orig[5]);
        dest[6] = _byte_2222_flip(orig[6]);
        dest[7] = _byte_2222_flip(orig[7]);
     }
}

/**
 * @brief Flips an encoded DXT2 block horizontally or vertically.
 * @param dest Pointer to the destination buffer for the flipped 16-byte block.
 *             Example: `unsigned char flipped_dxt2_block[16];`
 * @param orig Pointer to the original 16-byte S3TC DXT2 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char original_dxt2_block[16];`
 * @param vflip Non-zero for vertical flip, zero for horizontal flip.
 * @details Reorders the explicit alpha bytes and flips the color block part.
 */
void
s3tc_encode_dxt2_rgba_flip(unsigned char *dest, const unsigned char *orig, int vflip)
{
   // Flip explicit alpha part (8 bytes)
   if (vflip)
     {
        dest[0] = orig[6];
        dest[1] = orig[7];
        dest[2] = orig[4];
        dest[3] = orig[5];
        dest[4] = orig[2];
        dest[5] = orig[3];
        dest[6] = orig[0];
        dest[7] = orig[1];
     }
   else
     {
        for (int k = 0; k < 8; k += 2)
          {
             dest[0+k] = _byte_44_flip(orig[1+k]);
             dest[1+k] = _byte_44_flip(orig[0+k]);
          }
     }
   // Flip color part (8 bytes) using DXT1 logic
   s3tc_encode_dxt1_flip(dest + 8, orig + 8, vflip);
}

/**
 * @brief Flips an encoded DXT3 block horizontally or vertically.
 * @param dest Pointer to the destination buffer for the flipped 16-byte block.
 *             Example: `unsigned char flipped_dxt3_block[16];`
 * @param orig Pointer to the original 16-byte S3TC DXT3 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char original_dxt3_block[16];`
 * @param vflip Non-zero for vertical flip, zero for horizontal flip.
 * @details DXT3 uses the same block layout and flipping logic as DXT2.
 */
void
s3tc_encode_dxt3_rgba_flip(unsigned char *dest, const unsigned char *orig, int vflip)
{
   // DXT3 flip is identical to DXT2 flip
   s3tc_encode_dxt2_rgba_flip(dest, orig, vflip);
}

/**
 * @brief Flips an encoded DXT4 block horizontally or vertically.
 * @param dest Pointer to the destination buffer for the flipped 16-byte block.
 *             Example: `unsigned char flipped_dxt4_block[16];`
 * @param orig Pointer to the original 16-byte S3TC DXT4 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char original_dxt4_block[16];`
 * @param vflip Non-zero for vertical flip, zero for horizontal flip.
 * @details Flips the interpolated alpha block part and the color block part
 *          using the DXT1 flipping logic (as DXT5 alpha uses the same index layout).
 */
void s3tc_encode_dxt4_rgba_flip(unsigned char *dest, const unsigned char *orig, int vflip)
{
   // Flip interpolated alpha part (8 bytes) using DXT1 logic (same index structure)
   s3tc_encode_dxt1_flip(dest, orig, vflip);
   // Flip color part (8 bytes) using DXT1 logic
   s3tc_encode_dxt1_flip(dest + 8, orig + 8, vflip);
}

/**
 * @brief Flips an encoded DXT5 block horizontally or vertically.
 * @param dest Pointer to the destination buffer for the flipped 16-byte block.
 *             Example: `unsigned char flipped_dxt5_block[16];`
 * @param orig Pointer to the original 16-byte S3TC DXT5 block data.
 *             Layout: [8 bytes alpha][8 bytes color]
 *             Example: `const unsigned char original_dxt5_block[16];`
 * @param vflip Non-zero for vertical flip, zero for horizontal flip.
 * @details DXT5 uses the same block layout and flipping logic as DXT4.
 */
void s3tc_encode_dxt5_rgba_flip(unsigned char *dest, const unsigned char *orig, int vflip)
{
   // DXT5 flip is identical to DXT4 flip
   s3tc_encode_dxt4_rgba_flip(dest, orig, vflip);
}

/** @} */ // End of S3TC Block Flipping Functions group
