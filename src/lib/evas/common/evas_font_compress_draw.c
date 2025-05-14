// inherited from parent func
//   RGBA_Font_Glyph_Out *fgo;
//   int w, h, x1, x2, y1, y2, i, *iptr;
//   DATA32 coltab[16], col;
//   DATA16 mtab[16], v;
//   DATA8 tmp;

/**
 * @brief Blends a destination pixel with a source color using MMX instructions.
 * @param _dst The destination pixel (32-bit).
 * @param _col The source color (32-bit), pre-multiplied by alpha.
 * @param _mul The inverse alpha value (0-255) of the source color.
 *
 * This macro performs the operation: _dst = _col + (_dst * _mul) / 256
 * using MMX registers for optimization. mm0 is assumed to be zeroed.
 */
#define MMX_BLEND(_dst, _col, _mul) \
   MOV_P2R(_dst, mm1, mm0) \
   MOV_A2R(_mul, mm3) \
   MOV_P2R(_col, mm2, mm0) \
   MUL4_256_R2R(mm3, mm1) \
   paddw_r2r(mm2, mm1); \
   MOV_R2P(mm1, _dst, mm0)

/**
 * @brief Blends a destination pixel with a source color using standard C operations.
 * @param _dst The destination pixel (32-bit).
 * @param _col The source color (32-bit), pre-multiplied by alpha.
 * @param _mul The inverse alpha value (0-255) of the source color.
 *
 * This macro performs the operation: _dst = _col + (_dst * _mul) / 256.
 * MUL_256(a, b) is assumed to be ((a) * (b)) / 256.
 */
#define C_BLEND(_dst, _col, _mul) \
   _dst = _col + MUL_256(_mul, _dst)

/**
 * @brief Copies 64 bits (e.g., two 32-bit pixels) using an MMX instruction.
 * @param _dst Pointer to the destination memory.
 * @param _src The MMX register (mm7 assumed) containing the 64 bits to copy.
 *
 * Typically used for optimized copying of fully opaque pixels.
 */
#define MMX_COPY64(_dst, _src) \
   movq_r2m(_src, _dst)

/**
 * @brief Copies multiple 64-bit blocks using MMX_COPY64 in a loop.
 * @param _dst Pointer to the destination memory, will be incremented.
 * @param _len Number of 32-bit pixels to copy. The loop processes 2 pixels (64 bits) per iteration.
 *
 * This macro is used to quickly fill a span of pixels with a solid color
 * when MMX optimizations are available. mm7 is assumed to hold the duplicated color.
 */
#define MMX_COPY64LOOP(_dst, _len) \
   if (_len >= 2) \
   { \
      while (_len > 1) \
        { \
           MMX_COPY64(_dst[0], mm7); \
           _dst += 2; _len -= 2; \
        } \
   }

// if we build for mmx optimizations, we need to set up a few things in advance
// like the mm0 register is always all 0'd to fill in 0 padding when
// unpacking values to registers. also mm7 is reserved to hold an unpacked
// and dumpliacted coltab entry for the final entry (max color). so it's
// [col][col] in the 63bit register with both 32bit colors doublicated
#ifdef MMX
/**
 * @brief Initializes MMX registers for subsequent operations.
 * - mm0 is zeroed: used for padding when unpacking data.
 * - mm7 is loaded with coltab[0xf] (typically solid/opaque color) duplicated
 *   into both 32-bit halves of the 64-bit register. This is used for
 *   fast copying of solid color spans with MMX_COPY64.
 */
pxor_r2r(mm0, mm0);
movd_m2r(coltab[0xf], mm7);
punpckldq_r2r(mm7, mm7);
#endif

// check header for typ (rle4 or bpp4)
iptr = (int *)fgo->rle;
if (*iptr > 0) // rle4
{
   DATA8 *p = fgo->rle, *e, *s;
   DATA32 *d0, *d, t;
   DATA16 len;
   int xx, yy, dif;

   iptr = (int *)p;
   p += sizeof(int);
   d0 = dst + x + (y * dst_pitch);
// this may seem horrible to put a massive blob of logic into a macro like
// this, but this is for speed reasons, so we can generate slightly different
// versions of the same blob of code logic that hold different optimizations
// inside (eg mmx/sse/neon asm etc.)
/**
 * @brief Decodes and renders Run-Length Encoded (RLE) glyph data.
 *
 * This macro encapsulates the core logic for rendering RLE4 compressed font glyphs.
 * It handles both unclipped and horizontally clipped rendering paths.
 * The RLE data consists of pairs: (length-1 << 4) | value.
 * - `length-1`: The number of pixels in the run (0-15, representing 1-16 pixels).
 * - `value`: An index (0-15) into `coltab` (color table) and `mtab` (alpha mask table).
 *
 * A jump table (`jumptab`) is used to quickly find the start of RLE data for each scanline.
 *
 * @param _donelabel A base name for goto labels used for early exit in clipped rendering.
 * @param _extn A suffix for goto labels, typically indicating the optimization type (e.g., "_mmx", "_c").
 * @param _2copy A macro or code block for copying two pixels (64 bits) at once, used for opaque runs.
 *               Example for MMX: MMX_COPY64LOOP(d, len)
 *               Example for C: (no equivalent, often empty or single pixel copies)
 * @param _blend A macro or code block for blending a single pixel.
 *               Example for MMX: MMX_BLEND(d[0], coltab[v], mtab[v])
 *               Example for C: C_BLEND(d[0], coltab[v], mtab[v])
 *
 * Assumed context variables:
 * - `x1`, `x2`: Horizontal clipping coordinates.
 * - `y1`, `y2`: Vertical clipping coordinates.
 * - `w`: Width of the glyph.
 * - `d0`: Pointer to the start of the destination buffer for the current line (unclipped).
 * - `dst_pitch`: Pitch (stride in bytes) of the destination buffer.
 * - `p`: Pointer to the start of the RLE data (after header and jump table).
 * - `jumptab`: Pointer to the scanline jump table.
 * - `coltab`: Array of 16 DATA32 colors. `coltab[v]` is the color for value `v`.
 * - `mtab`: Array of 16 DATA16 alpha mask values. `mtab[v]` is the inverse alpha for value `v`.
 *           `mtab[v] == 0` means fully opaque.
 */
#define EXPAND_RLE(_donelabel, _extn, _2copy, _blend) \
   if ((x1 == 0) && (x2 == w)) /* unclipped  horizontally */ \
   { \
      d0 += x1; \
      for (yy = y1; yy < y2; yy++) \
        { \
           /* figure out source ptr and end ptr based on jumptable */ \
           if (yy > 0) s = p + jumptab[yy - 1]; \
           else s = p; \
           e = p + jumptab[yy]; \
           d = d0 + (yy * dst_pitch); \
           /* walk until we hit the end of the src data */ \
           while (s < e) \
             { \
                /* read the run length from RLE data and value */ \
                len = (*s >> 4) + 1; \
                v = *s & 0xf; \
                /* if value is 0 we can just skip ahead entire run and do */ \
                /* nothng as empty space doesn't need any work */ \
                if (v == 0) d += len; \
                /* if the value ends up being solid (inverse alpha is 0) */ \
                else if (mtab[v] == 0) \
                  { \
                     /* just COPY the color data direct to destination */ \
                     t = coltab[0xf]; \
                     /* this is a special 2 pixel (64bit dest) copy for */ \
                     /* speed - eg mmx etc. */ \
                     _2copy; \
                     /* do cleanup of left-over pixels after the 2 pixel */ \
                     /* copy above (if there is any such code) */ \
                     while (len > 0) \
                       { \
                          /* just a plain copy of looked up value */ \
                          *d = t; \
                          d++; len--; \
                       } \
                  } \
                /* our font mask value is between 0 and 15 (0xf) so we */ \
                /* have to actually blend it to each dest pixel */ \
                else \
                  { \
                     while (len > 0) \
                       { \
                          /* do blend using op provided by params */ \
                          _blend; \
                          d++; len--; \
                       } \
                  } \
                s++; \
             } \
        } \
   } \
   else /* clipped horizontally (needs extra skip/cut logic) */ \
   { \
      /* init out pos to 0 here (we reset AFTER each horiz loop later */ \
      xx = 0; \
      for (yy = y1; yy < y2; yy++) \
        { \
           /* figure out source ptr and end ptr based on jumptable */ \
           if (yy > 0) s = p + jumptab[yy - 1]; \
           else s = p; \
           e = p + jumptab[yy]; \
           d = d0 + (yy * dst_pitch); \
           /* walk until we hit the end of the src data and SKIP runs */ \
           /* that are entirely before the start (x1) point and any */ \
           /* run that spans over the start point is truncated at the */ \
           /* start of the run */ \
           while (s < e) \
             { \
                len = (*s >> 4) + 1; \
                /* if current pos pluse run length go over the start (x1) */ \
                /* point of our clip area, then adjust run length and dest */ \
                /* pointer and position and break out of our RLE skip loop */ \
                if ((xx + (int)len) > x1) \
                  { \
                     dif = x1 - xx; \
                     len -= dif; d += dif; xx += dif; \
                     break; \
                  } \
                d += len; xx += len; s++; \
             } \
           /* walk until we hit the end of the REL run.. OR the end of */ \
           /* our clip region - the x2 checks are done inside */ \
           while (s < e) \
             { \
                v = *s & 0xf; \
                /* if value is 0 we can just skip ahead entire run and do */ \
                /* nothng as empty space doesn't need any work */ \
                if (v == 0) \
                  { \
                     d += len; xx += len; \
                     /* clip check to stop run */ \
                     if (xx >= x2) goto _donelabel##_extn; \
                  } \
                /* if the value ends up being solid (inverse alpha is 0) */ \
                else if (mtab[v] == 0) \
                  { \
                     /* just COPY the color data direct to destination */ \
                     t = coltab[0xf]; \
                     while (len > 0) \
                       { \
                          /* clip check to stop run */ \
                          if (xx >= x2) goto _donelabel##_extn; \
                          /* just a plain copy of looked up value */ \
                          *d = t; \
                          d++; xx++; len--; \
                       } \
                  } \
                /* our font mask value is between 0 and 15 (0xf) so we */ \
                /* have to actually blend it to each dest pixel */ \
                else \
                  { \
                     while (len > 0) \
                       { \
                          /* clip check to stop run */ \
                          if (xx >= x2) goto _donelabel##_extn; \
                          /* do blend using op provided by params */ \
                          _blend; \
                          d++; xx++; len--; \
                       } \
                  } \
                s++; \
                /* extra check here so length fetch after doesn't break */ \
                if (s >= e) break; \
                /* get length of NEXT RLE run at the end here */ \
                len = (*s >> 4) + 1; \
             } \
_donelabel##_extn: \
           /* reset horiz pos to 0 ready for next line */ \
           xx = 0; \
        } \
   }

   // and here actually run the appropriate code in the macro/func defined
   // above, based on the jumptable type (saves passing params on the stack
   // to a sub function and we'd have to generate the subfunction by macros
   // anyway, so just cust down code to assume context vars as opposed to
   // passing them)
   // The iptr value (read from fgo->rle) indicates the type of jump table used:
   // 1: 8-bit offsets per scanline
   // 2: 16-bit offsets per scanline
   // 3: 32-bit offsets per scanline
   if (*iptr == 1) // 8 bit jump table
     {
        /**
         * @brief Jump table with 8-bit offsets.
         * Each entry `jumptab[yy]` stores the offset from `p` to the RLE data for scanline `yy`.
         */
        DATA8 *jumptab = p;
        p += (h * sizeof(DATA8));
#ifdef MMX
        EXPAND_RLE(done_8_clipped, _mmx, MMX_COPY64LOOP(d, len),
                   MMX_BLEND(d[0], coltab[v], mtab[v]))
#elif defined(NEON)
        EXPAND_RLE(done_8_clipped, _neon, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#else
        EXPAND_RLE(done_8_clipped, _c, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#endif
     }
   else if (*iptr == 2) // 16 bit jump table
     {
        /**
         * @brief Jump table with 16-bit offsets.
         * Each entry `jumptab[yy]` stores the offset from `p` to the RLE data for scanline `yy`.
         */
        unsigned short *jumptab = (unsigned short *)p;
        p += (h * sizeof(unsigned short));
#ifdef MMX
        EXPAND_RLE(done_16_clipped, _mmx, MMX_COPY64LOOP(d, len),
                   MMX_BLEND(d[0], coltab[v], mtab[v]))
#elif defined(NEON)
        EXPAND_RLE(done_16_clipped, _neon, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#else
        EXPAND_RLE(done_16_clipped, _c, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#endif
     }
   else if (*iptr == 3) // 32 bit jump table
     {
        /**
         * @brief Jump table with 32-bit offsets.
         * Each entry `jumptab[yy]` stores the offset from `p` to the RLE data for scanline `yy`.
         */
        int *jumptab = (int *)p;
        p += (h * sizeof(int));
#ifdef MMX
        EXPAND_RLE(done_32_clipped, _mmx, MMX_COPY64LOOP(d, len),
                   MMX_BLEND(d[0], coltab[v], mtab[v]))
#elif defined(NEON)
        EXPAND_RLE(done_32_clipped, _neon, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#else
        EXPAND_RLE(done_32_clipped, _c, ,
                   C_BLEND(d[0], coltab[v], mtab[v]))
#endif
     }
#undef EXPAND_RLE
}
else // bpp4
{
   int xx, yy, djump;
   int pitch2;
   DATA8 *s, *s0, v0;
   DATA32 *d;

   // dst: Pointer to the destination image buffer.
   // x, y: Top-left coordinates to draw the glyph.
   // x1, y1, x2, y2: Clipping rectangle relative to the glyph origin.
   // dst_pitch: Stride of the destination image buffer in pixels.
   d = dst + x + x1 + ((y + y1) * dst_pitch);
   // djump: Amount to add to 'd' to move to the start of the next scanline within the clipped region.
   djump = dst_pitch - (x2 - x1);
   // pitch2: Stride of the source BPP4 data in bytes. Each byte contains two 4-bit pixels.
   pitch2 = (w + 1) / 2;
   // s0: Pointer to the start of the BPP4 data for the first relevant scanline (y1).
   // fgo->rle points to the raw glyph data. The first sizeof(int) is the type header.
   s0 = fgo->rle + sizeof(int) + (y1 * pitch2);
   for (yy = y1; yy < y2; yy++) // Loop through visible scanlines
     {
        s = s0 + (x1 / 2);
        xx = x1;
        // do odd pixel at start if there is any
        if (xx & 0x1)
          {
             v = (*s) & 0xf;
             // fast path - totally solid color can just be written
             // with no blending done
             if (mtab[v] == 0) d[0] = coltab[0xf];
             // blend our color from lookup table
             else if (v)
               {
                  // blend it
#ifdef MMX
                  MMX_BLEND(d[0], coltab[v], mtab[v]);
#else
                  C_BLEND(d[0], coltab[v], mtab[v]);
#endif
               }
             s++; d++; xx++;
          }
        // walk along 2 pixels at a time (1 src pixel is 4 bits packed)
        for (; xx < (x2 - 1); xx += 2)
          {
             v0 = *s;
             // fast path - totally solid color can just be written
             // with no blending done - write 2 at once
             if ((v0 == 0xff) && (mtab[v0 & 0xf] == 0))
               {
                  // blend it
#ifdef MMX
                  MMX_COPY64(d[0], mm7);
#else
                  d[0] = d[1] = coltab[0xf];
#endif
               }
             // if our 2 values are not 0 (as 0's we can skip entirely)
             else if (v0)
               {
                  // get first pixel in MSB and blend it
                  v = (v0) >> 4;
#ifdef MMX
                  MMX_BLEND(d[0], coltab[v], mtab[v]);
#else
                  C_BLEND(d[0], coltab[v], mtab[v]);
#endif
                  // get next pixel in LSB and blend it
                  v = (v0) & 0xf;
#ifdef MMX
                  MMX_BLEND(d[1], coltab[v], mtab[v]);
#else
                  C_BLEND(d[1], coltab[v], mtab[v]);
#endif
               }
             s++; d += 2;
          }
        // clean up any leftover pixels at the end
        if (xx < x2)
          {
             v = (*s) >> 4;
             // fast path - totally solid color can just be written
             // with no blending done
             if (mtab[v] == 0) d[0] = coltab[0xf];
             // blend our color from lookup table
             else if (v)
               {
                  // blend it
#ifdef MMX
                  MMX_BLEND(d[0], coltab[v], mtab[v]);
#else
                  C_BLEND(d[0], coltab[v], mtab[v]);
#endif
               }
             d++;
          }
        d += djump;
        s0 += pitch2;
     }
}
// with mmx (sse etc.) we need to say we are done with the mmx registers so
// any fpu usage is restored (early pentiums need this, later x86 do not)
#ifdef MMX
/**
 * @brief Finalizes MMX operations.
 * On some older x86 processors (like early Pentiums with MMX),
 * this is necessary to restore the FPU state if MMX registers were used,
 * as they might alias FPU registers. Modern x86 CPUs (with SSE and later)
 * typically don't require this as MMX and FPU states are managed separately
 * or MMX instructions use dedicated SSE registers.
 */
evas_common_cpu_end_opt();
#endif
