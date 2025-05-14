#include "evas_font_private.h"

// XXX:
// XXX: adapt cserve2 to this!
// XXX:

//--------------------------------------------------------------------------
//- UTILS ------------------------------------------------------------------
//--------------------------------------------------------------------------

/**
 * @brief Expands a 1-bit per pixel bitmap to an 8-bit per pixel bitmap.
 *
 * This function is used to convert glyphs from fonts that are in a 1-bit
 * format into an 8-bit format, which is easier to process for compression.
 *
 * @param src Pointer to the source 1-bit bitmap data.
 * @param pitch The pitch (stride in bytes) of the source bitmap.
 * @param w The width of the bitmap in pixels.
 * @param h The height of the bitmap in pixels.
 * @param dst Pointer to the destination 8-bit bitmap buffer. This buffer
 *            must be pre-allocated with sufficient size (w * h bytes).
 */
static void
expand_bitmap(DATA8 *src, int pitch, int w, int h, DATA8 *dst)
{
   // some glyphs from fonts come in 1bit variety - expand it to 8bit before
   // compressing as it's easier to deal with a universal format
   static const DATA8 bitrepl[2] = { 0x00, 0xff };
   DATA8 *s, *d, bits;
   int bi, bj, y, end;

   for (y = 0; y < h; y++)
     {
        d = dst + (y * w);
        s = src + (y * pitch);
        // wall all bytes per row
        for (bi = 0; bi < w; bi += 8)
          {
             bits = *s;
             if ((w - bi) < 8) end = w - bi;
             else end = 8;
             // each byte has 8 bits - expand them out using lookup table above
             for (bj = 0; bj < end; bj++)
               {
                  *d = bitrepl[(bits >> (7 - bj)) & 0x1];
                  d++;
               }
             s++;
          }
     }
}

/**
 * @brief Converts an 8-bit alpha value to a 4-bit alpha value.
 *
 * The 4-bit alpha values are quantized to steps of 0x11 (17),
 * e.g., 0x00, 0x11, 0x22, ..., 0xFF. This function rounds the 8-bit
 * alpha value to the nearest 4-bit equivalent.
 *
 * @param a8 The 8-bit alpha value (0-255).
 * @return The corresponding 4-bit alpha value (0-15), which when
 *         expanded back (e.g., `(a4 << 4) | a4`) gives the quantized
 *         8-bit representation.
 */
static inline DATA8
alpha8to4(int a8)
{
   // a4 values are 0x00, 0x11, 0x22, 0x33, ... 0xee, 0xff
   // increments by 0x11 = 17
   int a4 = (a8 >> 4) & 0x0f;
   int v = (a4 << 4) | a4;
   if ((a8 - v) > 8) a4++;
   else if ((v - a8) > 8) a4--;
   return a4; // v = (a4 << 4) | a4;
}



//--------------------------------------------------------------------------
//- RLE 4BIT ---------------------------------------------------------------
//--------------------------------------------------------------------------

// what is 4bit rle? it's 4 bit per pixel run-length encoding. this means
// that every row of pixels is compressed int a separate defined list
// of "runs" where every run is N pixles at value V. RLE works well for
// things like fonts which have vast regions that are either empty or solid
// with some transition (anti-alias) pixels in between. it could be that for
// a black and white alternating pattern it will come out the worst possible
// case, but this basically "never happens".
//
// data is encoded so it's fastr to access and decompress at runtime. we have
// both a blob of data that is the RLE encoded data for all rows which consist
// of 1 byte per run, and also a jump table - per row telling us the byte
// offset inside the RLE data blob where the row data begins. since we know
// the offset of the next run, we know how many bytes each row is based on
// this.
//
// since rle data may be small (less than 256 bytes) and in almost all cases
// less than 64k, a jump table of 8 bite per entry is good for many uses, and
// otherwise 16bits is used. it also supports 32bit jumptables but these are
// there just in case the data goes beyond 64k - but is unlikely to ever
// happen in real life. this means jumptables come in 3 formats thus have to
// have 3 different handling paths. RLE data is the same so it's common code.
//
// each byte in the RLE section encodes a run of between 1 and 16 pixels in
// length. there is no such thing as a run of 0 pixels. the upper 4 bits of
// the byte encode the length, with 0 being 1 pixel, 1 being 2 pixels,
// 2 being 3 pixels and so on up top 16 pixels (thus run length is actually
// (byte >> 4) + 1). the lower 4 bits encode the 4 bit pixel value of the
// whole run, from 0 to 15. it is accessed via masking (byte & 0xf). thus
// every run in RLE consumes exactly 1 byte of memory nice and neatly.
//
// at the start before the jumptable is a 32bit (int) header. it just has a
// value at the moment that indicates 0 for it not being RLE data (used by
// the 4bit packed bitmap), 1 for 8bit jumptable RLE, 2 for 16bit jumptable
// and 3 for 32bit jumptable. all other values are reserved
//
// so data looks like this when packed into a single blob in memory (where
// xx is the data size of the jump table - 8, 16 or 32bit). there are n
// lines of data in the jumptable matching to the height of the glyph where
// n is the height in rows
//
// each jumptable row ACTUALLY indicates the byte offset of the NEXT line.
// the FIRST row of RLE data is assumed to be at offset 0 in the RLE data
// section, so a special case is used for this. note that jumptable values
// are OFFSETS starting at 0 which is the first byte in the RLE data section
//
// [int] header (0, 1, 2 or  3)
// [xx] jump table for line 0
// [xx] jump table for line 1
// [xx] jump table for line 2
// ...
// [xx] jump table for line n - 1
// [char] first byte of RLE data (beginning of rle data)
// [char] second byte of RLE data
// ...
// [char] last byte of RLE data
//

/**
 * @brief Compresses 8-bit per pixel image data into a 4-bit RLE format.
 *
 * This function takes an 8-bit grayscale image and compresses it using
 * Run-Length Encoding where each run stores a 4-bit alpha value.
 * The output format includes a header indicating the jump table size,
 * the jump table itself (offsets to the start of RLE data for each row),
 * and the RLE data.
 *
 * @param src Pointer to the source 8-bit image data.
 * @param pitch The pitch (stride in bytes) of the source image.
 * @param w The width of the image in pixels.
 * @param h The height of the image in pixels.
 * @param[out] size_ret Pointer to an integer where the total size of the
 *                      compressed data will be stored.
 * @return A pointer to the allocated buffer containing the compressed RLE data,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         this buffer.
 *         The compressed data structure is:
 *         - `[int] header`: Type of jump table (1: 8-bit, 2: 16-bit, 3: 32-bit).
 *         - `[xx * h] jumptab`: Jump table, where `xx` is 1, 2, or 4 bytes.
 *           Each entry `jumptab[i]` stores the offset to the RLE data for row `i+1`.
 *           The RLE data for row 0 starts immediately after the jump table.
 *         - `[char array] rle_data`: The actual RLE encoded pixel data.
 *           Each byte encodes a run: `(length-1) << 4 | value`.
 *           Length is 1-16 pixels. Value is a 4-bit alpha.
 */
static DATA8 *
compress_rle4(DATA8 *src, int pitch, int w, int h, int *size_ret)
{
   unsigned char *scratch, *p, *pix, spanval;
   int *jumptab, x, y, spanlen, spannum, total, size, *iptr, *pos;
   unsigned short *sptr;
   DATA8 *dst, *buf, *dptr;

   // these macros make the code more readable and easier to follow, and
   // avoid replication of dumb blobs of logic
#define SPAN_ADD(_len, _val) do { (*pos) += 1; *p = ((_len) << 4) | (_val); p++; } while (0)
#define LAST_SPAN_VAL() (p[-1] & 0x0f)
#define LAST_SPAN_LEN() (p[-1] >> 4)
#define LAST_SPAN_DEL() do { (*pos) -= 1; p -= 1; } while (0)

   // create out scratch buffer for compression on the stack - maximum size
   scratch = p = alloca(pitch * h * 2);
   // also place our jumptable on the stack too - all ints here - become
   // smaller char/shorts after jumptable is generated and size known
   jumptab = alloca(h * sizeof(int));
   for (y = 0; y < h; y++)
     {
        pix = src + (y * pitch);
        // pos is the position offset from RLE data start that we have to
        // track to find out where this rows RLE run *ENDS* so keep a
        // pointer to it and we will keep ++ing it with each REL entry we add
        pos = &(jumptab[y]);
        *pos = (int)(p - scratch);
        // no spans now so init all span things to 0
        spanval = spanlen = spannum = 0;
        for (x = 0; x < w; x++)
          {
             // round value from a8 to a44
             DATA8 v = alpha8to4(pix[x]);
             // if the current pixel value (in 4bit) is not the same as the
             // span value (n 4 bit) OR... if the span now exceeds 16 pixels
             // then add/write out the span to our RLE span blob
             if ((v != spanval) || (spanlen >= 16))
               {
                  if (spanlen > 0)
                    {
                       SPAN_ADD(spanlen - 1, spanval);
                       spannum++;
                    }
                  spanval = v;
                  spanlen = 1;
               }
             // otherwise make span longer if values are the same
             else spanlen++;
          }
        // do we have a span still being built that we haven't added and that
        // is NOT transparent (0 value -  there is no point storing spans
        // at the end of a row that have 0 value
        if ((spanlen > 0) && (spanval > 0))
          {
             SPAN_ADD(spanlen - 1, spanval);
             spannum++;
          }
        // clean up any dangling 0 value at the end of a row as they just
        // waste space and processing time
        while ((spannum > 0) && (LAST_SPAN_VAL() == 0))
          {
             LAST_SPAN_DEL();
             spannum--;
          }
     }
   // get the size of RLE data we have plus int header
   total = (int)(p - scratch);
   size = sizeof(int) + total;
   // based on total number of bytes in RLE, use 32, 16 or 8 bit jumptable
   // and add that to our size
   if (total > 65535) size += h * 4; // 32bit
   else if (total > 255) size += h * 2; // 16bit
   else size += h; // 8bit

   *size_ret = size;
   // allocate a fresh buffer where we will merge header, jumptable and RLE
   // spans inot a single block
   buf = dst = malloc(size);
   if (!buf) return NULL;
   // 32bit int header to indicate encoding type (3, 2 or 1)
   iptr = (int *)dst;
   if (total > 65535) *iptr = 3; // 32bit jump table
   else if (total > 255) *iptr = 2; // 16 bit jump table
   else *iptr = 1; // 8 bit jump table
   // skip header and write jump table
   dst += sizeof(int);
   if (total > 65535) // 32bit jump table
     {
        iptr = (int *)dst;
        for (y = 0; y < h; y++) iptr[y] = jumptab[y];
        dst += (h * sizeof(int));
     }
   else if (total > 255) // 16bit jump table
     {
        sptr = (unsigned short *)dst;
        for (y = 0; y < h; y++) sptr[y] = jumptab[y];
        dst += (h * sizeof(unsigned short));
     }
   else // 8bit jump table
     {
        dptr = dst;
        for (y = 0; y < h; y++) dptr[y] = jumptab[y];
        dst += (h * sizeof(DATA8));
     }
   // copy rest of RLE data at the end of the jumptable and return it
   memcpy(dst, scratch, total);
   return buf;
}

// this decompresses a specific run of RLE data to the destination pointer
// and finishes reading RLE data before the "end" byte and starts AT the
// "start" byte within the array pointed to by src. this ASSUMES the dest
// buffer has already been zeroed out so we can skip runs that are "0"

/**
 * @brief Decompresses a single row of 4-bit RLE data into an 8-bit pixel row.
 *
 * This function processes a segment of RLE data (for one row) and expands it
 * into an 8-bit grayscale pixel row. It assumes the destination buffer `dst`
 * has been zeroed out, allowing it to skip writing runs of zero-value pixels.
 *
 * @param src Pointer to the start of the RLE data blob (not just the row's data).
 * @param start The byte offset within `src` where the RLE data for this row begins.
 * @param end The byte offset within `src` where the RLE data for this row ends.
 * @param dst Pointer to the destination buffer for the decompressed 8-bit pixel row.
 *            This buffer must be large enough to hold the row's width in pixels.
 */
static void
decompress_full_row(DATA8 *src, int start, int end, DATA8 *dst)
{
   DATA8 *p = src + start, *e = src + end, *d = dst, len, val;

   while (p < e)
     {
        // length is upper 4 bits + 1
        len = (*p >> 4) + 1;
        // value when EXPANDED to 8bit is the lower 4 bits REPEATEd in all
        // 8 bites to ensure it rounds properly.
        // i.e. lower 4 bits B4B3B2B1 -> B4B3B2B1B4B3B2B1
        val = *p & 0xf;
        val |= val << 4;
        // if it's 0 just skip ahead (assume dst buffer is 0'd out)
        if (val == 0) d += len;
        else
          {
             // write out "len" pixels of tghe given value
             while (len > 0)
               {
                  *d = val;
                  d++;
                  len--;
               }
          }
        // next RLE byte
        p++;
     }
}

// to save copy & paste repeating code, this macro acts as a code generator
// to create a specific decompress function per jumptable size (8, 16 or 32bit)

/**
 * @def DECOMPRESS_ROW_FUNC
 * @brief Macro to generate row decompression functions for different jump table types.
 *
 * This macro creates specialized functions (`decompress_jumptab8_rle4`,
 * `decompress_jumptab16_rle4`, `decompress_jumptab32_rle4`) that iterate
 * through image rows, determine the RLE data segment for each row using the
 * provided jump table, and call `decompress_full_row` to decompress it.
 *
 * @param _name The name of the function to generate.
 * @param _type The data type of the jump table entries (e.g., DATA8, unsigned short, int).
 */
#define DECOMPRESS_ROW_FUNC(_name, _type) \
static void \
_name(_type *jumptab, DATA8 *src, DATA8 *dst, int pitch, int h) \
{ \
   int y, start, end; \
   for (y = 0; y < h; y++) \
     { \
        if (y > 0) start = jumptab[y - 1]; \
        else start = 0; \
        end = jumptab[y]; \
        decompress_full_row(src, start, end, dst + (y * pitch)); \
     } \
}
// 3 versions of the decompress given 3 jumptable types/sizes
DECOMPRESS_ROW_FUNC(decompress_jumptab8_rle4, DATA8)
DECOMPRESS_ROW_FUNC(decompress_jumptab16_rle4, unsigned short)
DECOMPRESS_ROW_FUNC(decompress_jumptab32_rle4, int)

// decompress a full RLE blob with header into the dst pointer. pitch is
// the number of bytes between each destination row

/**
 * @brief Decompresses a full 4-bit RLE data blob into an 8-bit image.
 *
 * This function reads the header from the RLE data to determine the jump
 * table format (8-bit, 16-bit, or 32-bit offsets) and then calls the
 * appropriate specialized decompression function generated by
 * `DECOMPRESS_ROW_FUNC`.
 *
 * @param src Pointer to the compressed 4-bit RLE data blob (including header).
 * @param dst Pointer to the destination buffer for the decompressed 8-bit image.
 *            This buffer must be pre-allocated and zeroed.
 * @param pitch The pitch (stride in bytes) of the destination image buffer.
 * @param w The width of the image in pixels (unused in this function but
 *          part of a common signature).
 * @param h The height of the image in pixels.
 */
static void
decompress_rle4(DATA8 *src, DATA8 *dst, int pitch, int w EINA_UNUSED, int h)
{
   int header;
   DATA8 *jumptab;

   // get header value and then skip past to jump table
   header = *((int *)src);
   jumptab = src + sizeof(int);
#define DECOMPRESS_FUNC(_name, _type) _name((_type *)jumptab,  jumptab + (h * sizeof(_type)), dst, pitch, h)
   if (header == 1)
     DECOMPRESS_FUNC(decompress_jumptab8_rle4, DATA8);
   else if (header == 2)
     DECOMPRESS_FUNC(decompress_jumptab16_rle4, unsigned short);
   else if (header == 3)
     DECOMPRESS_FUNC(decompress_jumptab32_rle4, int);
}




//--------------------------------------------------------------------------
//- RAW 4BIT ---------------------------------------------------------------
//--------------------------------------------------------------------------

// this compresses 8bit per pixel font data to 4bit per pixel (with 4 bit MSB
// per byte holding the left most pixel and 4 bit LSB holding the right pixel
// data). each row is rounded up to a whole number of bytes so the last
// pixel may only contain 1, not 2 4bit values and thus we throw away the LSB
// 4 bits on odd-length rows in the last pixel. at the top of the 4bit packed
// pixel data is an integer that stores the data type - value of 0 means
// 4bit packed data. this is so we can share the same generic "rle" pointer
// between 4bit rle and 4bit packed and easily switch between these 2 encodings
// based on which one is likely more compact and/or faster at runtime.

/**
 * @brief Compresses 8-bit per pixel image data to raw 4-bit per pixel.
 *
 * This function converts an 8-bit grayscale image to a 4-bit grayscale image.
 * Each byte in the output stores two 4-bit pixels: the most significant 4 bits
 * for the first pixel and the least significant 4 bits for the second.
 * Rows with an odd number of pixels will have the last byte's lower 4 bits unused.
 * The output data is prepended with an integer header (value 0) to distinguish
 * it from RLE compressed data.
 *
 * @param src Pointer to the source 8-bit image data.
 * @param pitch The pitch (stride in bytes) of the source image.
 * @param w The width of the image in pixels.
 * @param h The height of the image in pixels.
 * @param[out] size_ret Pointer to an integer where the total size of the
 *                      compressed data (header + pixel data) will be stored.
 * @return A pointer to the allocated buffer containing the 4bpp compressed data,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         this buffer.
 *         The compressed data structure is:
 *         - `[int] header`: Type identifier, 0 for 4bpp packed.
 *         - `[char array] pixel_data`: Packed 4-bit pixel data.
 *           Each byte `(px1 << 4) | px2`.
 */
static DATA8 *
compress_bpp4(DATA8 *src, int pitch, int w, int h, int *size_ret)
{
   int pitch2, x, y, *iptr;
   DATA8 *buf, *p, *d, *s;

   // our horizontal pitch in bytes ... rounding up to account for odd lengths
   pitch2 = (w + 1) / 2;
   // allocate the buffer size for header plus data
   buf = malloc(sizeof(int) + (pitch2 * h));
   if (!buf) return NULL;
   // write the header value of 0
   iptr = (int *)buf;
   *iptr = 0;
   // start with the 4 bit packed data body
   p = buf + sizeof(int);
   // return size
   *size_ret = (pitch2 * h) + sizeof(int);
   for (y = 0; y < h; y++)
     {
        s = src + (y * pitch);
        d = p + (y * pitch2);
        // walk source row 2 pixels at a time and reduce to 4 bit (upper
        // 4 bits only needed) and pack
        for (x = 0; x < (w - 1); x += 2)
          {
             DATA8 v1 = alpha8to4(s[0]);
             DATA8 v2 = alpha8to4(s[1]);
             *d = (v1 << 4) | v2;
             s += 2;
             d++;
          }
        /// handle dangling "last" pixel if odd row length
        if (x < w) *d = (s[0] & 0xf0);
     }
   return buf;
}

// this decompresses packed 4bit data from the encoded data blob into a
// destination 8bit buffer assumed to be allocated and the right size with
// the given destination pitch in bytes per line and a row length of w
// pixels and height of h rows

/**
 * @brief Decompresses raw 4-bit per pixel data to an 8-bit per pixel image.
 *
 * This function converts a 4-bit grayscale image (where each byte packs two pixels)
 * back into an 8-bit grayscale image. It skips the integer header at the
 * beginning of the source data.
 *
 * @param src Pointer to the compressed 4-bit per pixel data (including header).
 * @param dst Pointer to the destination buffer for the decompressed 8-bit image.
 *            This buffer must be pre-allocated.
 * @param pitch The pitch (stride in bytes) of the destination image buffer.
 * @param w The width of the image in pixels.
 * @param h The height of the image in pixels.
 */
static void
decompress_bpp4(DATA8 *src, DATA8 *dst, int pitch, int w, int h)
{
   int pitch2, x, y;
   DATA8 *d, *s, val;

   // deal with source pixel to round up for odd length rows
   pitch2 = (w + 1) / 2;
   // skip header int
   src += sizeof(int);
   for (y = 0; y < h; y++)
     {
        s = src + (y * pitch2);
        d = dst + (y * pitch);
        // walk 2 pixels at a time (1 source byte) and unpack
        for (x = 0; x < (w - 1); x += 2)
          {
             // take MSB 4 bits (pixel 1)
             val = (*s) >> 4;
             // replicate those 4 bits in MSB of dest so it rounds correctly
             val |= val << 4;
             // store in dest
             *d = val;
             d++;
             // take LSB 4 bits (pixel 2)
             val = (*s) & 0xf;
             // replicate those 4 bits in MSB of dest so it rounds correctly
             val |= val << 4;
             // store in dest
             *d = val;
             s++;
             d++;
          }
        // deal with odd length rows and take MSB 4 bits and store to dest
        if (x < w)
          {
             val = (*s) >> 4;
             val |= val << 4;
             *d = val;
          }
     }
}



//--------------------------------------------------------------------------
//- GENERAL ----------------------------------------------------------------
//--------------------------------------------------------------------------

/**
 * @brief Compresses font glyph data.
 *
 * This function takes raw glyph data (either 8-bit grayscale or 1-bit monochrome)
 * and compresses it. If the input is 1-bit, it's first expanded to 8-bit.
 * Based on the glyph dimensions, it chooses either raw 4-bit per pixel (4bpp)
 * compression or 4-bit Run-Length Encoding (RLE). Smaller glyphs typically use
 * 4bpp, while larger ones use RLE.
 *
 * @param data Pointer to the raw glyph data.
 * @param num_grays Number of gray levels in the source data (e.g., 256 for FT_PIXEL_MODE_GRAY).
 * @param pixel_mode The pixel mode of the source data (e.g., FT_PIXEL_MODE_GRAY, FT_PIXEL_MODE_MONO).
 * @param pitch_data The pitch (stride in bytes) of the source glyph data.
 * @param w The width of the glyph in pixels.
 * @param h The height of the glyph in pixels.
 * @param[out] size_ret Pointer to an integer where the size of the compressed
 *                      data will be stored.
 * @return A pointer to the allocated buffer containing the compressed glyph data,
 *         or NULL if the glyph dimensions are invalid or compression fails.
 *         The caller is responsible for freeing this buffer. The format of this
 *         data will be either 4bpp packed or 4-bit RLE, identifiable by an
 *         integer header at the beginning of the data.
 */
EVAS_API void *
evas_common_font_glyph_compress(void *data, int num_grays, int pixel_mode,
                                int pitch_data, int w, int h, int *size_ret)
{
   DATA8 *inbuf, *buf;
   int size = 0, pitch = 0;

   // avoid compressing 0 sized glyph
   if ((h < 1) || (pitch_data < 1)) return NULL;
   inbuf = alloca(w * h);
   // if glyph buffer is 8bit grey - then compress straght
   if (((num_grays == 256) && (pixel_mode == FT_PIXEL_MODE_GRAY)))
     {
        inbuf = data;
        pitch = pitch_data;
     }
   // if glyph is 1bit bitmap - expand it to 8bit grey first
   else
     {
        pitch = w;
        expand_bitmap(data, pitch_data, w, h, inbuf);
     }
   // in testing for small glyphs - eg 16x16 or smaller it seems raw 4bit
   // encoding is faster (and smaller) than 4bit RLE.
   if ((w * h) < (16 * 16))
     // compress to 4bit per pixel, raw
     buf = compress_bpp4(inbuf, pitch, w, h, &size);
   else
     // compress to 4bit per pixel, run length encoded per row
     buf = compress_rle4(inbuf, pitch, w, h, &size);
   *size_ret = size;
   return buf;
}

// this decompresses a whole block of compressed font data back to 8bit
// per pixels and deals with both 4bit RLE and 4bit packed encoding modes

/**
 * @brief Decompresses font glyph data.
 *
 * This function takes a compressed font glyph (which can be either 4-bit RLE
 * or raw 4-bit packed) and decompresses it into an 8-bit grayscale bitmap.
 * It reads an integer header from the compressed data to determine the
 * encoding type and calls the appropriate decompression routine.
 *
 * @param fg Pointer to the RGBA_Font_Glyph structure containing the
 *           compressed glyph data and metadata.
 *           - `fg->glyph_out->rle`: Pointer to the compressed data.
 *           - `fg->glyph_out->bitmap.width`: Width of the glyph.
 *           - `fg->glyph_out->bitmap.rows`: Height of the glyph.
 * @param[out] wret Optional pointer to an integer where the width of the
 *                  decompressed glyph will be stored.
 * @param[out] hret Optional pointer to an integer where the height of the
 *                  decompressed glyph will be stored.
 * @return A pointer to a newly allocated buffer containing the decompressed
 *         8-bit grayscale glyph data, or NULL on allocation failure.
 *         The caller is responsible for freeing this buffer. The buffer will
 *         be `width * height` bytes in size.
 */
EVAS_API DATA8 *
evas_common_font_glyph_uncompress(RGBA_Font_Glyph *fg, int *wret, int *hret)
{
   RGBA_Font_Glyph_Out *fgo = fg->glyph_out;
   DATA8 *buf = calloc(1, fgo->bitmap.width * fgo->bitmap.rows);
   int *iptr;

   if (!buf) return NULL;
   if (wret) *wret = fgo->bitmap.width;
   if (hret) *hret = fgo->bitmap.rows;
   iptr = (int *)fgo->rle;
   if (*iptr > 0) // rle4
     decompress_rle4(fgo->rle, buf, fgo->bitmap.width,
                     fgo->bitmap.width, fgo->bitmap.rows);
   else // bpp4
     decompress_bpp4(fgo->rle, buf, fgo->bitmap.width,
                     fgo->bitmap.width, fgo->bitmap.rows);
   return buf;
}
