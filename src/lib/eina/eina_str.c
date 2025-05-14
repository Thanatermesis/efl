/* Leave the OpenBSD version below so we can track upstream fixes */
/*      $OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $        */

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#ifdef HAVE_BSD_STRING_H
# include <bsd/string.h>
#endif

#ifdef HAVE_ICONV
# include <errno.h>
# include <iconv.h>
#endif

#include "eina_private.h"
#include "eina_str.h"
#include "eina_cpu.h"

/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

/*
 * Internal helper function used by eina_str_has_suffix() and
 * eina_str_has_extension()
 */
/**
 * @internal
 * @brief Checks if a string ends with a given suffix using a provided comparison function.
 *
 * @param str The string to check.
 * @param suffix The suffix to look for.
 * @param cmp The comparison function (e.g., strcmp, strcasecmp).
 * @return EINA_TRUE if @p str ends with @p suffix, EINA_FALSE otherwise or on error.
 */
static inline Eina_Bool
eina_str_has_suffix_helper(const char *str,
                           const char *suffix,
                           int (*cmp)(const char *, const char *))
{
   size_t str_len;
   size_t suffix_len;

   if ((!str) || (!suffix)) return EINA_FALSE;
   str_len = strlen(str);
   suffix_len = eina_strlen_bounded(suffix, str_len);
   if (suffix_len == (size_t)-1) /* eina_strlen_bounded returns (size_t)-1 if suffix is longer than str_len */
      return EINA_FALSE;

   /* Compare the end of str with suffix */
   return cmp(str + str_len - suffix_len, suffix) == 0;
}

/**
 * @internal
 * @brief Core implementation for splitting a string by a delimiter.
 *
 * This function tokenizes the string @p str based on the @p delim.
 * It allocates a single block of memory for the array of string pointers
 * and the string data itself. The caller is responsible for freeing this
 * block by freeing the first element of the returned array, and then
 * the array itself.
 *
 * For example, if str = "a:b:c", delim = ":", max_tokens = 0:
 * Resulting str_array: {ptr_to_"a", ptr_to_"b", ptr_to_"c", NULL}
 * Memory layout: ["a\0b\0c\0" | ptr_to_"a" | ptr_to_"b" | ptr_to_"c" | NULL] (conceptually)
 *
 * @param str The string to split.
 * @param delim The delimiter string.
 * @param max_tokens Maximum number of tokens. If 0 or negative, splits as much as possible.
 *                   If 1, returns the original string in an array.
 * @param[out] elements Pointer to store the number of tokens found (excluding the final NULL).
 * @return A newly allocated array of strings (char **), NULL-terminated.
 *         The strings and the array are in a single malloc'ed block.
 *         Free with `free(result[0]); free(result);`.
 *         Returns NULL on error or invalid input.
 */
static inline char **
eina_str_split_full_helper(const char *str,
                           const char *delim,
                           int max_tokens,
                           unsigned int *elements)
{
   char *s, *pos, **str_array;
   const char *src;
   size_t len, dlen;
   unsigned int tokens = 0, x;
   const char *idx[256] = {NULL};

   if ((!str) || (!delim))
     {
        if (elements)
          *elements = 0;

        return NULL;
     }
   if (max_tokens < 0) max_tokens = 0;
   if (max_tokens == 1)
     {
        str_array = malloc(sizeof(char *) * 2);
        if (!str_array)
          {
             if (elements)
                *elements = 0;

             return NULL;
          }

        s = strdup(str);
        if (!s)
          {
             free(str_array);
             if (elements)
                *elements = 0;

             return NULL;
          }
        if (elements)
          *elements = 1;
        str_array[0] = s;
        str_array[1] = NULL;
        return str_array;
     }
   dlen = strlen(delim);
   if (dlen == 0)
     {
        if (elements)
           *elements = 0;

        return NULL;
     }

   src = str;
   /* count tokens and check strlen(str) */
   while (*src != '\0')
     {
        const char *d = delim, *d_end = d + dlen;
        const char *tmp = src;
        for (; (d < d_end) && (*tmp != '\0'); d++, tmp++)
          {
             if (EINA_LIKELY(*d != *tmp))
                break;
          }
        if (EINA_UNLIKELY(d == d_end))
          {
             src = tmp;
             if (tokens < (sizeof(idx) / sizeof(idx[0])))
               {
                  idx[tokens] = tmp;
                  //printf("token %d='%s'\n", tokens + 1, idx[tokens]);
               }
             tokens++;
             if (tokens && (tokens == (unsigned int)max_tokens)) break;
          }
        else
           src++;
     }
   len = src - str + strlen(src);

   str_array = malloc(sizeof(char *) * (tokens + 2));
   if (!str_array)
     {
        if (elements)
           *elements = 0;

        return NULL;
     }

   if (!tokens)
     {
        s = strdup(str);
        if (!s)
          {
             free(str_array);
             if (elements)
                *elements = 0;

             return NULL;
          }
        str_array[0] = s;
        str_array[1] = NULL;
        if (elements)
          *elements = 1;
        return str_array;
     }

   s = malloc(len + 1);
   if (!s)
     {
        free(str_array);
        if (elements)
           *elements = 0;

        return NULL;
     }

   str_array[0] = s;

   if (len == tokens * dlen)
     {
        /* someone's having a laugh somewhere */
        memset(s, 0, len + 1);
        for (x = 1; x < tokens + 1; x++)
          str_array[x] = s + x;
        str_array[x] = NULL;
        if (elements)
          *elements = x;
        return str_array;
     }
   /* copy tokens and string */
   if (idx[0] - str - dlen > len)
     {
        /* FIXME: don't think this can happen but putting this here just in case */
        abort();
     }
   pos = s;
   for (x = 0; x < MIN(tokens, (sizeof(idx) / sizeof(idx[0]))); x++)
     {
        if (x + 1 < (sizeof(idx) / sizeof(idx[0])))
          {
             /* first one is special */
             if (!x)
               {
                  eina_strlcpy(pos, str, idx[x] - str - dlen + 1);
                  str_array[x] = pos;
                  //printf("str_array[%d] = '%s'\n", x, str_array[x]);
                  pos += idx[x] - str - dlen + 1;
                  if ((tokens == 1) && (idx[0]))
                    {
                       eina_strlcpy(pos, idx[x], len + 1 - (pos - s));
                       x++, tokens++;
                       str_array[x] = pos;
                    }
               }
             /* more tokens */
             else if (idx[x + 1])
               {
                  eina_strlcpy(pos, idx[x - 1], idx[x] - idx[x - 1] - dlen + 1);
                  str_array[x] = pos;
                  //printf("str_array[%d] = '%s'\n", x, str_array[x]);
                  pos += idx[x] - idx[x - 1] - dlen + 1;
               }
             /* last token */
             else
               {
                  if (max_tokens && ((unsigned int)max_tokens < tokens + 1))
                    eina_strlcpy(pos, idx[x - 1], len + 1 - (pos - s));
                  else
                    {
                       //printf("diff: %d\n", len + 1 - (pos - s));
                       eina_strlcpy(pos, idx[x - 1], idx[x] - idx[x - 1] - dlen + 1);
                       str_array[x] = pos;
                       //printf("str_array[%d] = '%s'\n", x, str_array[x]);
                       pos += idx[x] - idx[x - 1] - dlen + 1;
                       x++, tokens++;
                       eina_strlcpy(pos, idx[x - 1], len + 1 - (pos - s));
                    }
                  str_array[x] = pos;
                  //printf("str_array[%d] = '%s'\n", x, str_array[x]);
               }
          }
        /* no more tokens saved after this one */
        else
          {
             eina_strlcpy(pos, idx[x - 1], idx[x] - idx[x - 1] - dlen + 1);
             str_array[x] = pos;
             //printf("str_array[%d] = '%s'\n", x, str_array[x]);
             pos += idx[x] - idx[x - 1] - dlen + 1;
             src = idx[x];
             x++, tokens++;
             str_array[x] = s = pos;
             break;
          }
     }
   if ((x != tokens) && ((!max_tokens) || (x < tokens)))
     {
        while (*src != '\0')
          {
             const char *d = delim, *d_end = d + dlen;
             const char *tmp = src;
             for (; (d < d_end) && (*tmp != '\0'); d++, tmp++)
               {
                  if (EINA_LIKELY(*d != *tmp))
                     break;
               }
             if (((!max_tokens) || (((tokens == (unsigned int)max_tokens) || x < tokens - 2))) && (EINA_UNLIKELY(d == d_end)))
               {
                  src = tmp;
                  *s = '\0';
                  s++, x++;
                  //printf("str_array[%d] = '%s'\n", x, str_array[x - 1]);
                  str_array[x] = s;
               }
             else
               {
                  *s = *src;
                  s++, src++;
               }
          }
        *s = 0;
     }
   str_array[tokens] = NULL;
   if (elements)
     *elements = tokens;

   return str_array;
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

EINA_API size_t
eina_strlcpy(char *dst, const char *src, size_t siz)
{
#ifdef HAVE_STRLCPY
   return strlcpy(dst, src, siz);
#else
   /* Custom implementation of strlcpy if not available in libc.
    * Copies up to siz-1 characters from src to dst, always NUL-terminating
    * (unless siz == 0). Returns the length of src (as if not truncated).
    */
   char *d = dst;
   const char *s = src;
   size_t n = siz;

   /* Copy as many bytes as will fit, including the NUL terminator if encountered. */
   if (n != 0)
      while (--n != 0)
        {
           if ((*d++ = *s++) == '\0') /* Copy char and check if it was NUL */
              break; /* Copied NUL, so src is fully copied and NUL-terminated */
        }

   /* If loop finished due to n == 0 (dst full before src NUL encountered) */
   if (n == 0)
     {
        if (siz != 0)
           *d = '\0';  /* NUL-terminate dst if there's space for it */
        /* Traverse the rest of src to calculate its full length. */
        while (*s++)
           ;
     }

   return(s - src - 1); /* Total length of src (strlen(src)) */
#endif
}

EINA_API size_t
eina_strlcat(char *dst, const char *src, size_t siz)
{
#ifdef HAVE_STRLCAT
   return strlcat(dst, src, siz);
#else
   /* Custom implementation of strlcat if not available in libc.
    * Appends src to dst, NUL-terminating the result.
    * siz is the total size of dst.
    * Returns the total length of the string it tried to create (initial strlen(dst) + strlen(src)).
    */
   char *d = dst;
   const char *s = src;
   size_t n = siz;
   size_t dlen;

   /* Find the end of dst and calculate remaining space. */
   /* n will be decremented one past the NUL or end of buffer if NUL not found. */
   while (n-- != 0 && *d != '\0')
      d++;
   dlen = d - dst; /* Length of initial content in dst. */
   n = siz - dlen; /* Remaining space in dst for src and NUL. */

   if (n == 0) /* No space left in dst. */
     return(dlen + (s ? strlen(s) : 0)); /* Return required length. */

   if (s != NULL) /* Only append if src is not NULL. */
     {
        /* Append src to dst. */
        while (*s != '\0') {
           if (n != 1) /* Check if there's space for char and NUL. */
             {
                *d++ = *s;
                n--; /* Decrement available space. */
             }
           s++; /* Always advance src pointer to calculate full length. */
        }
     }
   *d = '\0'; /* NUL-terminate the result. */

   return(dlen + (s - src)); /* Return total length it tried to create. s-src is strlen(original_src). */
#endif
}

EINA_API char *
eina_strftime(const char *format, const struct tm *tm)
{
   const size_t flen = strlen(format);
   /* Start with a small buffer, anticipating common date/time formats.
    * flen can be 0 for empty format string, so ensure buflen is at least 1.
    */
   size_t buflen = flen + 16;
   if (buflen < 16) buflen = 16;
   char *buf = NULL;

   do {
      char *tmp;
      size_t len;

      tmp = realloc(buf, buflen * sizeof(char));
      if (!tmp) goto on_error; /* Allocation failure */
      buf = tmp;

      len = strftime(buf, buflen, format, tm);

      /* strftime returns 0 if buflen is too small OR if format results in empty string.
       * If len > 0 and len < buflen, success.
       * If len == 0 and flen == 0 (empty format), success (empty result).
       */
      if ((len > 0 && len < buflen) || (len == 0 && flen == 0))
        {
           /* Shrink buffer to exact size + NUL.
            * If strftime wrote len characters, the buffer needs len+1 for NUL.
            */
           tmp = realloc(buf, ((len + 1) * sizeof(char)));
           if (!tmp && len > 0) { /* If realloc to shrink fails, original buf is still valid */
             /* buf[len] should already be '\0' by strftime if len < buflen */
             return buf;
           }
           /* If tmp is NULL and len is 0, it means realloc(buf, 1) failed.
            * In this case, the original buf (which might be NULL if first attempt)
            * or a larger buffer is freed, and NULL is returned via on_error.
            * However, if len is 0, buf[0] = '\0' is already set by strftime.
            */
           buf = tmp; /* Can be NULL if realloc to 1 byte fails */
           return buf; /* Return potentially shrunk buffer or NULL if realloc failed */
        }

      /* Buffer was too small (strftime returned 0 and flen > 0, or len == buflen).
       * Double the buffer size for the next attempt.
       */
      buflen <<= 1;
      /* Protect against huge allocations if flen is very large or format is tricky.
       * 128 * flen is an arbitrary limit.
       */
   } while (buflen < (128 * (flen + 1))); /* +1 to handle flen=0 reasonably */

 on_error: /* Handles allocation errors or if buffer limit exceeded */
   free(buf);
   return NULL;
}

EINA_API Eina_Bool
eina_str_has_prefix(const char *str, const char *prefix)
{
   size_t str_len;
   size_t prefix_len;

   str_len = strlen(str);
   prefix_len = eina_strlen_bounded(prefix, str_len);
   if (prefix_len == (size_t)-1)
     return EINA_FALSE;

   return (strncmp(str, prefix, prefix_len) == 0);
}

EINA_API Eina_Bool
eina_str_has_suffix(const char *str, const char *suffix)
{
   return eina_str_has_suffix_helper(str, suffix, strcmp);
}

EINA_API Eina_Bool
eina_str_has_extension(const char *str, const char *ext)
{
   return eina_str_has_suffix_helper(str, ext, strcasecmp);
}

EINA_API char **
eina_str_split_full(const char *str,
                    const char *delim,
                    int max_tokens,
                    unsigned int *elements)
{
   return eina_str_split_full_helper(str, delim, max_tokens, elements);
}


EINA_API char **
eina_str_split(const char *str, const char *delim, int max_tokens)
{
   return eina_str_split_full_helper(str, delim, max_tokens, NULL);
}

EINA_API size_t
eina_str_join_len(char *dst,
                  size_t size,
                  char sep,
                  const char *a,
                  size_t a_len,
                  const char *b,
                  size_t b_len)
{
   /* Calculate the total length required for "a" + sep + "b" + NUL. */
   size_t ret = a_len + b_len + 1;
   size_t off; /* Current offset in the destination buffer dst. */

   /* If the provided buffer size is 0, cannot even write a NUL. */
   if (size < 1)
     return ret; /* Return the required length. */

   /* If buffer is not large enough to hold string 'a' and a NUL. */
   if (size <= a_len)
     {
        memcpy(dst, a, size - 1); /* Copy as much of 'a' as fits. */
        dst[size - 1] = '\0';     /* NUL-terminate. */
        return ret;               /* Return the required length. */
     }

   /* Copy all of string 'a'. */
   memcpy(dst, a, a_len);
   off = a_len; /* Update offset. */

   /* If buffer is not large enough to hold 'a', separator, and a NUL. */
   if (size <= off + 1) /* off + 1 is for separator, NUL comes after */
     {
        /* No space for separator, just NUL-terminate after 'a'.
         * Note: dst[off] would be the separator. dst[size-1] is the last byte.
         * If size == off + 1, it means dst[off] is the last byte, so it gets NUL.
         */
        dst[off] = '\0';
        /* However, the standard behavior is to ensure dst[size-1] is NUL if truncated.
         * If size is exactly a_len + 1, then dst[a_len] (which is dst[off]) gets NUL.
         */
        return ret; /* Return the required length. */
     }

   /* Add the separator. */
   dst[off] = sep;
   off++; /* Update offset. */

   /* If buffer is not large enough to hold 'a', separator, 'b', and a NUL. */
   /* off + b_len for 'b', +1 for NUL. So size must be > off + b_len */
   if (size <= off + b_len)
     {
        memcpy(dst + off, b, size - off - 1); /* Copy as much of 'b' as fits. */
        dst[size - 1] = '\0';                 /* NUL-terminate. */
        return ret;                           /* Return the required length. */
     }

   /* Buffer is large enough for "a" + sep + "b" + NUL. */
   memcpy(dst + off, b, b_len); /* Copy all of string 'b'. */
   dst[off + b_len] = '\0';     /* NUL-terminate. */
   return ret;                  /* Return the required length (which is also actual written length excluding NUL). */
}

#ifdef HAVE_ICONV
EINA_API char *
eina_str_convert(const char *enc_from, const char *enc_to, const char *text)
{
   iconv_t ic;
   char *new_txt, *outp;
   const char *inp;
   size_t inb, outb, outlen, tob, outalloc;

   if (!text)
      return NULL;

   ic = iconv_open(enc_to, enc_from);
   if (ic == (iconv_t)(-1)) /* iconv_open failed */
      return NULL;

   /* Initial allocation for the output buffer. */
   new_txt = malloc(64);
   if (!new_txt) /* malloc failed */
     {
        iconv_close(ic);
        return NULL;
     }
   inb = strlen(text); /* Bytes remaining in input buffer. */
   outb = 64;          /* Bytes remaining in output buffer. */
   inp = text;         /* Pointer to current position in input. */
   outp = new_txt;     /* Pointer to current position in output. */
   outalloc = 64;      /* Total allocated size for output buffer. */
   outlen = 0;         /* Total bytes written to output buffer so far. */

   /* Conversion loop: continues as long as there's input or iconv needs to flush. */
   for (;; )
     {
        size_t count;

        tob = outb; /* Store original outb to calculate bytes written in this iconv call. */
        /* Perform the conversion. iconv modifies inp, inb, outp, and outb. */
        count = iconv(ic, (char **)&inp, &inb, &outp, &outb);
        outlen += tob - outb; /* Add number of bytes written to outlen. */

        if (count == (size_t)(-1)) /* iconv error. */
          {
             if (errno == E2BIG) /* Output buffer too small. */
               {
                  char *reallocated_txt;
                  outalloc += 64; /* Increase allocated size. */
                  reallocated_txt = realloc(new_txt, outalloc);
                  if (!reallocated_txt) /* realloc failed. */
                    {
                       free(new_txt);
                       new_txt = NULL;
                       goto close_iconv_and_exit;
                    }
                  new_txt = reallocated_txt;
                  /* Adjust outp to point to the new end of written data. */
                  outp = new_txt + outlen;
                  outb += 64; /* Add the newly allocated space to outb. */
               }
             else /* Other iconv error (e.g., invalid sequence). */
               {
                  if (new_txt)
                     free(new_txt);
                  new_txt = NULL;
                  goto close_iconv_and_exit;
               }
          }

        if (inb == 0) /* All input has been consumed. */
          {
             /* Ensure NUL termination. Resize if exactly full. */
             if (outalloc == outlen)
               {
                  char *reallocated_txt = realloc(new_txt, outalloc + 1);
                  if (!reallocated_txt)
                    {
                       free(new_txt);
                       new_txt = NULL;
                       goto close_iconv_and_exit;
                    }
                  new_txt = reallocated_txt;
               }
             new_txt[outlen] = '\0'; /* Add NUL terminator. */
             break; /* Conversion successful. */
          }
     }

close_iconv_and_exit:
   iconv_close(ic);
   return new_txt;
}
#else
EINA_API char *
eina_str_convert(const char *enc_from EINA_UNUSED,
                 const char *enc_to EINA_UNUSED,
                 const char *text EINA_UNUSED)
{
   return NULL;
}
#endif

#ifdef HAVE_ICONV
EINA_API char *
eina_str_convert_len(const char *enc_from, const char *enc_to, const char *text, size_t len, size_t *retlen)
{
   iconv_t ic;
   char *new_txt, *outp;
   const char *inp;
   size_t inb, outb, outlen, tob, outalloc;

   if (retlen) *retlen = 0;
   if (!text) return NULL;

   ic = iconv_open(enc_to, enc_from);
   if (ic == (iconv_t)(-1)) /* iconv_open failed */
      return NULL;

   /* Initial allocation for the output buffer. */
   new_txt = malloc(64);
   if (!new_txt) /* malloc failed */
     {
        iconv_close(ic);
        return NULL;
     }
   inb = len;          /* Bytes remaining in input buffer (using provided length). */
   outb = 64;          /* Bytes remaining in output buffer. */
   inp = text;         /* Pointer to current position in input. */
   outp = new_txt;     /* Pointer to current position in output. */
   outalloc = 64;      /* Total allocated size for output buffer. */
   outlen = 0;         /* Total bytes written to output buffer so far. */

   /* Conversion loop, similar to eina_str_convert. */
   for (;; )
     {
        size_t count;

        tob = outb;
        count = iconv(ic, (char **)&inp, &inb, &outp, &outb);
        outlen += tob - outb;

        if (count == (size_t)(-1))
          {
             if (errno == E2BIG) /* Output buffer too small. */
               {
                  char *reallocated_txt;
                  outalloc += 64;
                  reallocated_txt = realloc(new_txt, outalloc);
                  if (!reallocated_txt)
                    {
                       free(new_txt);
                       new_txt = NULL;
                       goto close_iconv_and_exit_len;
                    }
                  new_txt = reallocated_txt;
                  outp = new_txt + outlen;
                  outb += 64;
               }
             else /* Other iconv error. */
               {
                  if (new_txt)
                     free(new_txt);
                  new_txt = NULL;
                  goto close_iconv_and_exit_len;
               }
          }

        if (inb == 0) /* All input consumed. */
          {
             /* Ensure NUL termination. */
             if (outalloc == outlen)
               {
                  char *reallocated_txt = realloc(new_txt, outalloc + 1);
                   if (!reallocated_txt)
                    {
                       free(new_txt);
                       new_txt = NULL;
                       goto close_iconv_and_exit_len;
                    }
                  new_txt = reallocated_txt;
               }
             new_txt[outlen] = '\0';
             break;
          }
     }

close_iconv_and_exit_len:
   iconv_close(ic);
   if (retlen && new_txt) *retlen = outlen; /* Store output length if requested and successful. */
   return new_txt;
}
#else
EINA_API char *
eina_str_convert_len(const char *enc_from EINA_UNUSED, const char *enc_to EINA_UNUSED, const char *text EINA_UNUSED, size_t len EINA_UNUSED, size_t *retlen)
{
   if (retlen) *retlen = 0;
   return NULL;
}
#endif

EINA_API char *
eina_str_escape(const char *str)
{
   char *s2, *d;
   const char *s;

   if (!str)
      return NULL;

   /* Allocate memory for the escaped string.
    * In the worst case, every character is escaped (e.g., '\\'),
    * requiring two characters in the output ('\\', '\\') plus NUL.
    */
   s2 = malloc((strlen(str) * 2) + 1);
   if (!s2)
      return NULL;

   /* Iterate through the input string and copy characters to the output string,
    * prefixing special characters with a backslash.
    */
   for (s = str, d = s2; *s != 0; s++, d++)
     {
        switch (*s) /* Check for characters that need escaping. */
        {
         case ' ':
         case '\\':
         case '\'':
         case '\"':
           {
             *d = '\\';
             d++;
             *d = *s;
             break;
           }
         case '\n':
           {
             *d = '\\'; d++;
             *d = 'n';
             break;
           }
         case '\t':
           {
             *d = '\\'; d++;
             *d = 't';
             break;
           }
         default:
           {
             *d = *s;
             break;
           }
        }
     }
   *d = 0;
   return s2;
}

EINA_API void
eina_str_tolower(char **str)
{
   char *p;
   if ((!str) || (!(*str)))
      return;

   for (p = *str; (*p); p++)
      *p = tolower((unsigned char )(*p));
}

EINA_API void
eina_str_toupper(char **str)
{
   char *p;
   if ((!str) || (!(*str)))
      return;

   for (p = *str; (*p); p++)
      *p = toupper((unsigned char)(*p));
}

EINA_API unsigned char *
eina_memdup(unsigned char *mem, size_t size, Eina_Bool terminate)
{
   unsigned char *ret;

   if (!mem) return NULL;

   terminate = !!terminate;
   ret = malloc(size + terminate);
   if (!ret) return NULL;

   memcpy(ret, mem, size);
   if (terminate)
     ret[size] = 0;
   return ret;
}
