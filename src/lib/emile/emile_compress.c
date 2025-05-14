#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <zlib.h>

#ifdef ENABLE_LIBLZ4
#include <lz4.h>
#include <lz4hc.h>
#else
#include "lz4.h"
#include "lz4hc.h"
#endif

#include <Eina.h>

#include "Emile.h"

/**
 * @internal
 * @brief Calculates the maximum buffer size required for compression.
 *
 * This function determines the necessary buffer size for the output of
 * a compression operation, based on the input data and the chosen
 * compression algorithm. This is typically a worst-case estimation.
 *
 * @param data The input binary data to be compressed.
 * @param t The type of compressor to be used (EMILE_ZLIB, EMILE_LZ4, EMILE_LZ4HC).
 * @return The estimated maximum size of the compressed data buffer in bytes,
 *         or -1 if the compression type is unknown or invalid.
 */
static int
_emile_compress_buffer_size(const Eina_Binbuf *data, Emile_Compressor_Type t)
{
   switch (t)
     {
      case EMILE_ZLIB:
        /* For ZLIB, the zlib library's compressBound() function typically estimates
         * the required buffer size as: sourceLength + (sourceLength * 0.1%) + 12 bytes.
         * The formula used here: (sourceLength * 101 / 100) + 12,
         * which is equivalent to (sourceLength * 1.01) + 12 bytes,
         * provides a slightly more generous common approximation.
         */
        return 12 + ((eina_binbuf_length_get(data) * 101) / 100);

      case EMILE_LZ4:
      case EMILE_LZ4HC:
        /* LZ4_compressBound() provides the official worst-case (maximum) buffer size
         * required for LZ4 compression algorithms (both standard and HC).
         */
        return LZ4_compressBound(eina_binbuf_length_get(data));

      default:
        return -1;
     }
}

EAPI Eina_Binbuf *
emile_compress(const Eina_Binbuf *data,
               Emile_Compressor_Type t,
               Emile_Compressor_Level l)
{
   void *compact, *temp;
   int length;
   int level = l;
   Eina_Bool ok = EINA_FALSE;

   length = _emile_compress_buffer_size(data, t);
   if (length < 0) return NULL;

   compact = malloc(length);
   if (!compact)
     return NULL;

   switch (t)
     {
      case EMILE_LZ4:
        /* LZ4_compress_default performs standard LZ4 compression.
         * This function does not accept a compression level parameter.
         * Therefore, the 'Emile_Compressor_Level l' (passed as 'level')
         * parameter to emile_compress() is ignored for this compression type.
         */
        length = LZ4_compress_default
          ((const char *)eina_binbuf_string_get(data), compact,
           eina_binbuf_length_get(data), length);
        /* It is going to be smaller and should never fail, if it does you are in deep poo. */
        temp = realloc(compact, length);
        if (temp) compact = temp;

        if (length > 0)
          ok = EINA_TRUE;
        break;

      case EMILE_LZ4HC:
        /* LZ4_compress_HC is used for the High Compression mode of LZ4.
         * The 'compressionLevel' parameter to LZ4_compress_HC() is hardcoded to 16.
         * According to the LZ4 library documentation, compression level values greater
         * than LZ4HC_CLEVEL_MAX (which is 12) are clamped to LZ4HC_CLEVEL_MAX.
         * Thus, this effectively uses the highest available LZ4HC compression level (12).
         * Consequently, the 'Emile_Compressor_Level l' (passed as 'level')
         * parameter to emile_compress() is ignored for this compression type.
         */
        length = LZ4_compress_HC
          ((const char *)eina_binbuf_string_get(data), compact,
           eina_binbuf_length_get(data), length, 16);
        temp = realloc(compact, length);
        if (temp) compact = temp;

        if (length > 0)
          ok = EINA_TRUE;
        break;

      case EMILE_ZLIB:
      {
         uLongf buflen = (uLongf)length;

         if (compress2((Bytef *)compact, &buflen, (Bytef *)eina_binbuf_string_get(data), (uLong)eina_binbuf_length_get(data), level) == Z_OK)
           ok = EINA_TRUE;
         length = (int)buflen;
      }
     }

   if (!ok)
     {
        free(compact);
        return NULL;
     }

   return eina_binbuf_manage_new(compact, length, EINA_FALSE);
}

EAPI Eina_Bool
emile_expand(const Eina_Binbuf *in, Eina_Binbuf *out, Emile_Compressor_Type t)
{
   if (!in || !out)
     return EINA_FALSE;

   switch (t)
     {
      case EMILE_LZ4:
      case EMILE_LZ4HC:
      {
         int ret;

         ret = LZ4_decompress_safe((const char *)eina_binbuf_string_get(in),
                                   (char *)eina_binbuf_string_get(out),
                                   eina_binbuf_length_get(in),
                                   eina_binbuf_length_get(out));
         if ((unsigned int)ret != eina_binbuf_length_get(out))
           return EINA_FALSE;
         break;
      }

      case EMILE_ZLIB:
      {
         uLongf dlen = eina_binbuf_length_get(out);

         if (uncompress((Bytef *)eina_binbuf_string_get(out), &dlen, eina_binbuf_string_get(in), (uLongf)eina_binbuf_length_get(in)) != Z_OK)
           return EINA_FALSE;
         break;
      }

      default:
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EAPI Eina_Binbuf *
emile_decompress(const Eina_Binbuf *data,
                 Emile_Compressor_Type t,
                 unsigned int dest_length)
{
   Eina_Binbuf *out;
   void *expanded;

// this warning is wrong here so disable it
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
   expanded = malloc(dest_length);
   if (!expanded)
     return NULL;
   out = eina_binbuf_manage_new(expanded, dest_length, EINA_FALSE);
#pragma GCC diagnostic pop
   if (!out)
     goto on_error;

   if (!emile_expand(data, out, t))
     goto on_error;

   return out;

on_error:
   if (!out)
     free(expanded);
   if (out)
     eina_binbuf_free(out);
   return NULL;
}
