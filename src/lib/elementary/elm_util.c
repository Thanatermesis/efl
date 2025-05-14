#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef _WIN32
# include <evil_private.h> /* nl_langinfo */
#endif

#include <Elementary.h>

#include "elm_priv.h"

/**
 * @brief Copies at most count characters from string src to dest.
 *
 * This is a wrapper around strncpy.
 * If count is less than the length of src, no null-terminator is implicitly
 * appended to dest. If count is greater than the length of src, dest is
 * padded with nulls up to length count.
 *
 * @param dest The destination buffer.
 * @param src The source string.
 * @param count The maximum number of characters to copy.
 * @return A pointer to the destination string dest, or NULL if dest or src is NULL.
 */
char *
_str_ncpy(char *dest, const char *src, size_t count)
{
   if ((!dest) || (!src)) return NULL;
   return strncpy(dest, src, count);
}

/**
 * @brief Appends a string to another, reallocating if necessary.
 *
 * Appends the string pointed to by txt to the end of the string pointed to by str.
 * The len parameter should point to an integer holding the current length of str.
 * The alloc parameter should point to an integer holding the current allocated size of str.
 * These values will be updated if reallocation occurs.
 *
 * @param str The string to append to. This string may be reallocated.
 * @param txt The string to append.
 * @param len A pointer to an integer holding the current length of str. Will be updated.
 * @param alloc A pointer to an integer holding the current allocated size for str. Will be updated.
 * @return A pointer to the (potentially reallocated) string str, or the original str if txt is empty or reallocation fails.
 * @note If txt_len is 0, the original str is returned. If realloc fails, the original str is returned and its content remains unchanged.
 */
char *
_str_append(char *str, const char *txt, int *len, int *alloc)
{
   int txt_len = strlen(txt);

   if (txt_len <= 0) return str;
   if ((*len + txt_len) >= *alloc)
     {
        char *str2;
        int alloc2;

        alloc2 = *alloc + txt_len + 128;
        str2 = realloc(str, alloc2);
        if (!str2) return str;
        *alloc = alloc2;
        str = str2;
     }
   strcpy(str + *len, txt);
   *len += txt_len;
   return str;
}

/**
 * @brief Converts a markup string to a plain UTF-8 text string.
 *
 * This function uses evas_textblock_text_markup_to_utf8 to perform the conversion.
 * The caller is responsible for freeing the returned string.
 *
 * @param mkup The markup string to convert.
 *             Example: "Hello <hilight>World</hilight>!"
 * @return A newly allocated string containing the plain text version of mkup,
 *         or NULL on failure.
 *         Example: "Hello World!"
 */
char *
_elm_util_mkup_to_text(const char *mkup)
{
   return evas_textblock_text_markup_to_utf8(NULL, mkup);
}

/**
 * @brief Converts a plain UTF-8 text string to a markup string.
 *
 * This function uses evas_textblock_text_utf8_to_markup to perform the conversion.
 * Special characters in the text (like '<', '>', '&') will be escaped.
 * The caller is responsible for freeing the returned string.
 *
 * @param text The plain UTF-8 text string to convert.
 *             Example: "Hello World!"
 * @return A newly allocated string containing the markup version of text,
 *         or NULL on failure.
 *         Example: "Hello World!" (Note: in this simple case, it might be the same,
 *         but if text was "H&M", markup would be "H&amp;M")
 */
char *
_elm_util_text_to_mkup(const char *text)
{
   return evas_textblock_text_utf8_to_markup(NULL, text);
}

/**
 * @brief Converts a string to a double value.
 *
 * This function is a wrapper around eina_convert_strtod_c.
 * It provides a convenient way to convert a string representation of a
 * floating-point number to a double, using the C locale for conversion
 * to ensure consistent parsing regardless of the system's current locale.
 *
 * @param s The string to convert.
 *          Example: "123.45"
 * @return The converted double value. If the string is NULL, empty, or
 *         cannot be converted, 0.0 is returned.
 */
double
_elm_atof(const char *s)
{
   if ((!s) || (!s[0])) return 0.0;
   return eina_convert_strtod_c(s, NULL);
}
