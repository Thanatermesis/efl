#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "eina_private.h"
#include "eina_str.h"
#include "eina_strbuf_common.h"
#include "eina_unicode.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

#ifdef _STRBUF_DATA_TYPE
# undef _STRBUF_DATA_TYPE
#endif

#ifdef _STRBUF_CSIZE
# undef _STRBUF_CSIZE
#endif

#ifdef _STRBUF_STRUCT_NAME
# undef _STRBUF_STRUCT_NAME
#endif

#ifdef _STRBUF_STRLEN_FUNC
# undef _STRBUF_STRLEN_FUNC
#endif

#ifdef _STRBUF_STRESCAPE_FUNC
# undef _STRBUF_STRESCAPE_FUNC
#endif

#ifdef _STRBUF_MAGIC
# undef _STRBUF_MAGIC
#endif

#ifdef _STRBUF_MAGIC_STR
# undef _STRBUF_MAGIC_STR
#endif

#ifdef _FUNC_EXPAND
# undef _FUNC_EXPAND
#endif


#define _STRBUF_DATA_TYPE         char
#define _STRBUF_CSIZE             sizeof(_STRBUF_DATA_TYPE)
#define _STRBUF_STRUCT_NAME       Eina_Strbuf
#define _STRBUF_STRLEN_FUNC(x)    strlen(x)
#define _STRBUF_STRESCAPE_FUNC(x) eina_str_escape(x)
#define _STRBUF_MAGIC             EINA_MAGIC_STRBUF
#define _STRBUF_MAGIC_STR         __STRBUF_MAGIC_STR
static const char __STRBUF_MAGIC_STR[] = "Eina Strbuf";

#define _FUNC_EXPAND(y) eina_strbuf_ ## y

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/


EINA_API Eina_Bool
eina_strbuf_append_printf(Eina_Strbuf *buf, const char *fmt, ...)
{
   va_list args;
   char *str;
   size_t len; /* Stores the length of the formatted string. Note: vasprintf returns int. */
   Eina_Bool ret;

   va_start(args, fmt);
   /* vasprintf dynamically allocates a string 'str' containing the formatted output.
    * This string 'str' must be freed after use to prevent memory leaks.
    * On success, vasprintf returns the number of bytes written (excluding the null terminator).
    * On error, it returns -1, and the content of 'str' is undefined. */
   len = vasprintf(&str, fmt, args);
   va_end(args);

   /* The function proceeds only if 'len' is non-zero and 'str' is not NULL.
    * If 'len' is 0 (i.e., vasprintf produced an empty string), EINA_FALSE is returned.
    * If 'str' is NULL (which might happen if vasprintf fails), EINA_FALSE is returned.
    * Note: If vasprintf returns -1 (error), 'len' (as size_t) becomes SIZE_MAX. In such a case,
    * if 'str' is not NULL, this check may not identify the error, and 'str' would not be freed here. */
   if (len == 0 || !str)
      return EINA_FALSE;

   // Append the formatted string 'str' of 'len' characters to the buffer.
   ret = eina_strbuf_append_length(buf, str, len);
   // Free the string 'str' allocated by vasprintf.
   free(str);
   return ret;
}

EINA_API Eina_Bool
eina_strbuf_append_vprintf(Eina_Strbuf *buf, const char *fmt, va_list args)
{
   char *str;
   size_t len; /* Stores the length of the formatted string. Note: vasprintf returns int. */
   Eina_Bool ret;

   /* vasprintf dynamically allocates a string 'str' containing the formatted output.
    * This string 'str' must be freed after use to prevent memory leaks.
    * On success, vasprintf returns the number of bytes written (excluding the null terminator).
    * On error, it returns -1, and the content of 'str' is undefined. */
   len = vasprintf(&str, fmt, args);

   /* The function proceeds only if 'len' is non-zero and 'str' is not NULL.
    * See eina_strbuf_append_printf for detailed notes on this condition. */
   if (len == 0 || !str)
      return EINA_FALSE;

   // Append the formatted string 'str' of 'len' characters to the buffer.
   ret = eina_strbuf_append_length(buf, str, len);
   // Free the string 'str' allocated by vasprintf.
   free(str);
   return ret;
}

EINA_API Eina_Bool
eina_strbuf_insert_printf(Eina_Strbuf *buf, const char *fmt, size_t pos, ...)
{
   va_list args;
   char *str;
   size_t len; /* Stores the length of the formatted string. Note: vasprintf returns int. */
   Eina_Bool ret;

   va_start(args, pos);
   /* vasprintf dynamically allocates a string 'str' containing the formatted output.
    * This string 'str' must be freed after use to prevent memory leaks.
    * On success, vasprintf returns the number of bytes written (excluding the null terminator).
    * On error, it returns -1, and the content of 'str' is undefined. */
   len = vasprintf(&str, fmt, args);
   va_end(args);

   /* The function proceeds only if 'len' is non-zero and 'str' is not NULL.
    * See eina_strbuf_append_printf for detailed notes on this condition. */
   if (len == 0 || !str)
      return EINA_FALSE;

   // Insert the formatted string 'str' into the buffer at 'pos'.
   // Note: eina_strbuf_insert will use strlen(str), effectively using 'len'.
   ret = eina_strbuf_insert(buf, str, pos);
   // Free the string 'str' allocated by vasprintf.
   free(str);
   return ret;
}

EINA_API Eina_Bool
eina_strbuf_insert_vprintf(Eina_Strbuf *buf,
                           const char *fmt,
                           size_t pos,
                           va_list args)
{
   char *str;
   size_t len; /* Stores the length of the formatted string. Note: vasprintf returns int. */
   Eina_Bool ret;

   /* vasprintf dynamically allocates a string 'str' containing the formatted output.
    * This string 'str' must be freed after use to prevent memory leaks.
    * On success, vasprintf returns the number of bytes written (excluding the null terminator).
    * On error, it returns -1, and the content of 'str' is undefined. */
   len = vasprintf(&str, fmt, args);

   /* The function proceeds only if 'len' is non-zero and 'str' is not NULL.
    * See eina_strbuf_append_printf for detailed notes on this condition. */
   if (len == 0 || !str)
      return EINA_FALSE;

   // Insert the formatted string 'str' into the buffer at 'pos'.
   // Note: eina_strbuf_insert will use strlen(str), effectively using 'len'.
   ret = eina_strbuf_insert(buf, str, pos);
   // Free the string 'str' allocated by vasprintf.
   free(str);
   return ret;
}

EINA_API void
eina_strbuf_trim(Eina_Strbuf *buf)
{
   unsigned char *c = buf->buf; // 'c' initially points to the start of the buffer.
                                // It will be advanced if leading whitespace is found.

   // Trim whitespace from the end of the buffer.
   // This loop reduces 'buf->len' for each trailing whitespace character.
   // It inspects characters from right to left: c[buf->len - 1].
   while (buf->len > 0 && isspace(c[buf->len - 1]))
     buf->len--;

   // Trim whitespace from the beginning of the buffer.
   // This loop advances 'c' past leading whitespace characters and correspondingly reduces 'buf->len'.
   while (buf->len > 0 && isspace(*c))
     {
        c++;        // Advance 'c' to point to the next character.
        buf->len--; // Decrease effective length as one character is trimmed from the start.
     }
   // After trimming, 'c' points to the first non-whitespace character (or end of buffer if all was whitespace),
   // and 'buf->len' is the length of the (potentially) trimmed content.
   // Move this content to the beginning of 'buf->buf'.
   // memmove is used as the source (c) and destination (buf->buf) might overlap if c != buf->buf.
   // If no leading whitespace was trimmed (c == buf->buf), this effectively does nothing.
   memmove(buf->buf, c, buf->len);

   // Null-terminate the trimmed string at its new length.
   ((unsigned char *)buf->buf)[buf->len] = '\0';
}

EINA_API void
eina_strbuf_ltrim(Eina_Strbuf *buf)
{
   unsigned char *c = buf->buf; // 'c' initially points to the start of the buffer.
                                // It will be advanced if leading whitespace is found.

   // Trim whitespace from the beginning of the buffer.
   // This loop advances 'c' past leading whitespace characters and correspondingly reduces 'buf->len'.
   while (buf->len > 0 && isspace(*c))
     {
        c++;        // Advance 'c' to point to the next character.
        buf->len--; // Decrease effective length as one character is trimmed from the start.
     }
   // After trimming, 'c' points to the first non-whitespace character (or end of buffer).
   // Move this content (starting at 'c', length 'buf->len') to the beginning of 'buf->buf'.
   memmove(buf->buf, c, buf->len); // Safe even if c == buf->buf (no leading whitespace found).

   // Null-terminate the trimmed string at its new length.
   ((unsigned char *)buf->buf)[buf->len] = '\0';
}

EINA_API void
eina_strbuf_rtrim(Eina_Strbuf *buf)
{
   // Trim whitespace from the end of the buffer.
   // This loop reduces 'buf->len' for each trailing whitespace character found
   // by inspecting the character at index (buf->len - 1).
   while (buf->len > 0 && isspace(((unsigned char*)(buf->buf))[buf->len - 1]))
     buf->len--;
   // Null-terminate the string at its new length.
   // No memory movement is needed as trimming occurs from the right end.
   ((unsigned char *)buf->buf)[buf->len] = '\0';
}

EINA_API void
eina_strbuf_tolower(Eina_Strbuf *buf)
{
   if (!buf || !(buf->buf)) return;

   // eina_str_tolower converts the string content of buf->buf to lowercase, in-place.
   // The (char **)&(buf->buf) cast is to match the signature of eina_str_tolower.
   // It is assumed that eina_str_tolower:
   // 1. Modifies the buffer pointed to by *(buf->buf) directly.
   // 2. Does not reallocate buf->buf or change its base pointer.
   // 3. Does not change the length of the string content (buf->len remains valid).
   eina_str_tolower((char **)&(buf->buf));
}

EINA_API Eina_Strbuf *
eina_strbuf_substr_get(Eina_Strbuf *buf, size_t pos, size_t len)
{
   char *str;

   // Validate that the source buffer exists and the requested substring range [pos, pos + len)
   // is within the current length of the buffer's content.
   if ((!buf) || ((pos + len) > buf->len))
      return NULL;

   // Allocate memory for the new substring. '+1' for the null terminator.
   // calloc initializes the allocated memory to zero. This ensures that 'str'
   // will be null-terminated.
   // Note: There is no check here for calloc returning NULL. If calloc fails,
   // 'str' will be NULL, and the subsequent strncpy will likely cause a crash.
   str = calloc(1, len + 1);

   // Copy 'len' characters from the source buffer (buf->buf) starting at 'pos'
   // into the newly allocated string 'str'.
   // strncpy copies at most 'len' characters. Null-termination is guaranteed because
   // 'str' was allocated with 'len + 1' bytes and zeroed by calloc, so 'str[len]' is '\0'.
   strncpy(str, ((char *)(buf->buf)) + pos, len);

   // Create a new Eina_Strbuf instance to manage the newly created substring 'str'.
   // eina_strbuf_manage_new takes ownership of the 'str' pointer.
   return eina_strbuf_manage_new(str);
}

EINA_API Eina_Bool
eina_strbuf_append_strftime(Eina_Strbuf *buf, const char *format, const struct tm *tm)
{
   char *outputbuf; // Buffer to hold the string generated by eina_strftime.

   // eina_strftime formats the time 'tm' according to 'format' and returns
   // a newly allocated string. This string must be freed by the caller.
   outputbuf = eina_strftime(format, tm);
   // If eina_strftime fails (e.g., due to memory allocation error or invalid parameters),
   // it returns NULL.
   if (!outputbuf) return EINA_FALSE;

   // Append the formatted time string to the Eina_Strbuf.
   eina_strbuf_append(buf, outputbuf);
   // Free the temporary buffer allocated by eina_strftime.
   free(outputbuf);

   return EINA_TRUE;
}

EINA_API Eina_Bool
eina_strbuf_insert_strftime(Eina_Strbuf *buf, const char *format, const struct tm *tm, size_t pos)
{
   char *outputbuf; // Buffer to hold the string generated by eina_strftime.

   // eina_strftime formats the time 'tm' according to 'format' and returns
   // a newly allocated string. This string must be freed by the caller.
   outputbuf = eina_strftime(format, tm);
   // If eina_strftime fails, it returns NULL.
   if (!outputbuf) return EINA_FALSE;

   // Insert the formatted time string into the Eina_Strbuf at the specified position 'pos'.
   // strlen is used here to get the length of outputbuf for eina_strbuf_insert_length.
   eina_strbuf_insert_length(buf, outputbuf, strlen(outputbuf), pos);
   // Free the temporary buffer allocated by eina_strftime.
   free(outputbuf);

   return EINA_TRUE;
}

/* Unicode */

// Include the template file that implements generic string buffer functions.
// The behavior of the template is customized by the _STRBUF_* macros defined
// at the beginning of this file (e.g., _STRBUF_DATA_TYPE, _STRBUF_STRLEN_FUNC).
// This allows the same template code to be used for different string types if needed,
// though in this specific file, it's configured for 'char'-based Eina_Strbuf.
#include "eina_strbuf_template_c.x"
