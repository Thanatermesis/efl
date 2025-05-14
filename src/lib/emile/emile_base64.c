#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Eina.h>

#include "Emile.h"

#include <ctype.h>


/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Standard Base64 alphabet.
 */
static const char *base64_table_normal = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;

/**
 * @brief URL and filename safe Base64 alphabet.
 */
static const char *base64_table_url = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_" ;

/**
 * @brief Checks if a character is a valid Base64 character.
 *
 * @param c The character to check.
 * @param is_base64url EINA_TRUE if using the URL-safe alphabet, EINA_FALSE otherwise.
 * @return EINA_TRUE if the character is a valid Base64 character, EINA_FALSE otherwise.
 */
static inline Eina_Bool is_base64_char(unsigned char c, Eina_Bool is_base64url)
{
   if (is_base64url)
     return (isalnum(c) || (c == '-') || (c == '_'));
   else
     return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
 * @brief Common Base64 encoding logic.
 *
 * This function performs the core Base64 encoding operation, supporting both
 * standard and URL-safe alphabets.
 *
 * @param in The binary buffer to encode.
 * @param is_base64url_encode EINA_TRUE to use the URL-safe alphabet and omit padding,
 *                            EINA_FALSE for standard Base64 encoding.
 * @return A new Eina_Strbuf containing the Base64 encoded string, or NULL on failure.
 *         The caller is responsible for freeing the returned Eina_Strbuf.
 */
static Eina_Strbuf *
emile_base64_encode_common(const Eina_Binbuf *in, Eina_Bool is_base64url_encode)
{
   unsigned char inarr[3], outarr[4]; // Buffers for processing 3 input bytes to 4 output chars
   const unsigned char *src;
   size_t len;
   char *dest;
   int i = 0, j = 0, k = 0;
   const char *base64_table;

   if (!in) return NULL;

   src = eina_binbuf_string_get(in);
   len = eina_binbuf_length_get(in);

   if (!src) return NULL;

   // Max length of encoded string.
   dest = malloc(sizeof (char) * (((len + 2) / 3) * 4 + 1));
   if (!dest) return NULL;

   if (is_base64url_encode)
     base64_table = base64_table_url;
   else
     base64_table = base64_table_normal;

   while (len--)
     {
        inarr[i++] = *(src++);
        if (i == 3)
          {
             outarr[0] = (inarr[0] & 0xfc) >> 2;
             outarr[1] = ((inarr[0] & 0x03) << 4) + ((inarr[1] & 0xf0) >> 4);
             outarr[2] = ((inarr[1] & 0x0f) << 2) + ((inarr[2] & 0xc0) >> 6);
             outarr[3] = inarr[2] & 0x3f;

             for(i = 0; (i <4) ; i++)
               dest[k++] = base64_table[outarr[i]];
             i = 0;
          }
     }

   if (i)
     {
        for(j = i; j < 3; j++)
          inarr[j] = '\0';

        outarr[0] = (inarr[0] & 0xfc) >> 2;
        outarr[1] = ((inarr[0] & 0x03) << 4) + ((inarr[1] & 0xf0) >> 4);
        outarr[2] = ((inarr[1] & 0x0f) << 2) + ((inarr[2] & 0xc0) >> 6);
        outarr[3] = inarr[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
          dest[k++] = base64_table[outarr[j]];

        /* No padding for URL encoding */
        while((i++ < 3) && (!is_base64url_encode)) {
          dest[k++] = '=';
        }

     }

   dest[k] = '\0';

   return eina_strbuf_manage_new_length(dest, k);
}

/**
 * @brief Common Base64 decoding logic.
 *
 * This function performs the core Base64 decoding operation, supporting both
 * standard and URL-safe alphabets.
 *
 * @param in The Eina_Strbuf containing the Base64 encoded string to decode.
 * @param is_base64url_decode EINA_TRUE if the input string uses the URL-safe alphabet,
 *                            EINA_FALSE for standard Base64 decoding.
 * @return A new Eina_Binbuf containing the decoded binary data, or NULL on failure
 *         (e.g., invalid input string). The caller is responsible for freeing
 *         the returned Eina_Binbuf.
 */
static Eina_Binbuf *
emile_base64_decode_common(const Eina_Strbuf *in, Eina_Bool is_base64url_decode)
{
   unsigned char inarr[4], outarr[3]; // Buffers for processing 4 input chars to 3 output bytes
   int i = 0, j = 0, k = 0, l = 0;
   int len;
   unsigned char *dest;
   const char *src;
   const char *base64_table;

   if (!in) return NULL;
   src = eina_strbuf_string_get(in);
   len = eina_strbuf_length_get(in);

   if (!src) return NULL;

   /* The encoded string length should be a multiple of 4. Else it is not a
    * valid encoded string.
    */
   if (!is_base64url_decode && (len % 4))
     return NULL;

   /* This is the max size the destination string can have.
    */
   dest = (unsigned char *)malloc(sizeof(unsigned char) * ((len * 3 / 4) + 1));
   if (!dest)
     return NULL;

   if (is_base64url_decode)
     base64_table = base64_table_url;
   else
     base64_table = base64_table_normal;

   while (len-- && (src[k] != '=') && is_base64_char(src[k], is_base64url_decode))
     {
       inarr[i++] = src[k++];
       if (i == 4)
         {
           for (i = 0; i <4; i++)
             inarr[i] = strchr(base64_table,(int) inarr[i]) - base64_table;

           outarr[0] = (inarr[0] << 2) + ((inarr[1] & 0x30) >> 4);
           outarr[1] = ((inarr[1] & 0xf) << 4) + ((inarr[2] & 0x3c) >> 2);
           outarr[2] = ((inarr[2] & 0x3) << 6) + inarr[3];

           for (i = 0; (i < 3); i++)
             dest[l++] = outarr[i];
           i = 0;
         }
     }

   if (i)
     {
       for (j = i; j <4; j++)
         inarr[j] = 0;

       for (j = 0; j <4; j++)
         inarr[j] = strchr(base64_table, (int) inarr[j]) - base64_table;

       outarr[0] = (inarr[0] << 2) + ((inarr[1] & 0x30) >> 4);
       outarr[1] = ((inarr[1] & 0xf) << 4) + ((inarr[2] & 0x3c) >> 2);
       outarr[2] = ((inarr[2] & 0x3) << 6) + inarr[3];

       for (j = 0; (j < i - 1); j++)
         dest[l++] = outarr[j];
     }

   /* Ensure null termination for safety, though the length `l` passed to
    * eina_binbuf_manage_new determines the actual buffer content.
    */
   dest[l] = '\0';

   return eina_binbuf_manage_new(dest, l, EINA_FALSE);
}

/**
 * @endcond
 */

/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/*============================================================================*
*                                   API                                      *
*============================================================================*/

EAPI Eina_Strbuf *
emile_base64_encode(const Eina_Binbuf *in)
{
   return emile_base64_encode_common(in, EINA_FALSE);
}

EAPI Eina_Strbuf *
emile_base64url_encode(const Eina_Binbuf *in)
{
   return emile_base64_encode_common(in, EINA_TRUE);
}

EAPI Eina_Binbuf *
emile_base64_decode(const Eina_Strbuf *in)
{
   return emile_base64_decode_common(in, EINA_FALSE);
}

EAPI Eina_Binbuf *
emile_base64url_decode(const Eina_Strbuf *in)
{
   return emile_base64_decode_common(in, EINA_TRUE);
}
