#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Eina.h>

#include "Embryo.h"
#include "embryo_private.h"

/**
 * @brief Macro to retrieve a string from Embryo data space.
 * @param ep Pointer to the Embryo_Program.
 * @param str Pointer to a char* which will store the retrieved string.
 *            The string is allocated on the stack using alloca().
 * @param par Embryo_Cell containing the address of the string in Embryo data space.
 */
#define STRGET(ep, str, par) {                                \
     Embryo_Cell *___cptr;                                    \
     str = NULL;                                              \
     if ((___cptr = embryo_data_address_get(ep, par))) {      \
          int ___l;                                           \
          ___l = embryo_data_string_length_get(ep, ___cptr);  \
          (str) = alloca(___l + 1);                           \
          if (str) embryo_data_string_get(ep, ___cptr, str);  \
       } }

/**
 * @brief Macro to set a string in Embryo data space.
 * @param ep Pointer to the Embryo_Program.
 * @param par Embryo_Cell containing the address in Embryo data space where the string will be set.
 * @param str The string to set.
 */
#define STRSET(ep, par, str) {                           \
     Embryo_Cell *___cptr;                               \
     if ((___cptr = embryo_data_address_get(ep, par))) { \
          embryo_data_string_set(ep, str, ___cptr);      \
       } }

/* exported string api */

/**
 * @brief Converts a string to an integer.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the string to convert.
 * @return The converted integer, or 0 on error or if the string is invalid.
 */
static Embryo_Cell
_embryo_str_atoi(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1;

   /* params[1] = str */
   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   if (!s1) return 0;
   return (Embryo_Cell)atoi(s1);
}

/**
 * @brief Matches a string against a shell wildcard pattern.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the glob pattern.
 *               params[2] is the Embryo_Cell address of the string to match.
 * @return 1 if the string matches the pattern, 0 if it does not, -1 on error.
 * @note Uses eina_fnmatch for the matching logic.
 */
static Embryo_Cell
_embryo_str_fnmatch(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;

   /* params[1] = glob */
   /* params[2] = str */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   return (Embryo_Cell)!eina_fnmatch(s1, s2, 0);
}

/**
 * @brief Compares two strings.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the first string.
 *               params[2] is the Embryo_Cell address of the second string.
 * @return An integer less than, equal to, or greater than zero if s1 is found,
 *         respectively, to be less than, to match, or be greater than s2.
 *         Returns -1 on error (e.g., if strings cannot be retrieved).
 */
static Embryo_Cell
_embryo_str_strcmp(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;

   /* params[1] = str1 */
   /* params[2] = str2 */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return -1;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   return (Embryo_Cell)strcmp(s1, s2);
}

/**
 * @brief Compares at most n bytes of two strings.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the first string.
 *               params[2] is the Embryo_Cell address of the second string.
 *               params[3] is the maximum number of bytes to compare.
 * @return An integer less than, equal to, or greater than zero if the first n
 *         bytes of s1 is found, respectively, to be less than, to match, or
 *         be greater than the first n bytes of s2.
 *         Returns -1 on error (e.g., if strings cannot be retrieved).
 */
static Embryo_Cell
_embryo_str_strncmp(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;

   /* params[1] = str1 */
   /* params[2] = str2 */
   /* params[3] = n */
   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   if (params[3] < 0) params[3] = 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   return (Embryo_Cell)strncmp(s1, s2, (size_t)params[3]);
}

/**
 * @brief Copies a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination buffer.
 *               params[2] is the Embryo_Cell address of the source string.
 * @return 0 on success, or if the source string is NULL.
 * @note The destination buffer in Embryo must be large enough.
 */
static Embryo_Cell
_embryo_str_strcpy(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1;

   /* params[1] = dst */
   /* params[2] = str */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[2]);
   if (!s1) return 0;
   STRSET(ep, params[1], s1);
   return 0;
}

/**
 * @brief Copies at most n bytes of a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination buffer.
 *               params[2] is the Embryo_Cell address of the source string.
 *               params[3] is the maximum number of bytes to copy.
 * @return 0 on success, or if the source string is NULL.
 * @note The destination buffer in Embryo must be large enough. If the length
 *       of the source string is greater than n, the copied string will be
 *       null-terminated at n characters.
 */
static Embryo_Cell
_embryo_str_strncpy(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1;
   int l;

   /* params[1] = dst */
   /* params[2] = str */
   /* params[3] = n */
   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   if (params[3] < 0) params[3] = 0;
   STRGET(ep, s1, params[2]);
   if (!s1) return 0;
   l = strlen(s1);
   if (l > params[3]) s1[params[3]] = 0;
   STRSET(ep, params[1], s1);
   return 0;
}

/**
 * @brief Calculates the length of a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the string.
 * @return The length of the string, or 0 on error or if the string is NULL.
 */
static Embryo_Cell
_embryo_str_strlen(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1;

   /* params[1] = str */
   if (params[0] != (1 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   if (!s1) return 0;
   return (Embryo_Cell)strlen(s1);
}

/**
 * @brief Concatenates two strings.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination string.
 *               params[2] is the Embryo_Cell address of the source string to append.
 * @return 0 on success, or if either string is NULL or memory allocation fails.
 * @note The destination string in Embryo (params[1]) is updated with the
 *       concatenated result. A temporary buffer is allocated on the stack using alloca().
 */
static Embryo_Cell
_embryo_str_strcat(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *s3;

   /* params[1] = dsr */
   /* params[2] = str */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return 0;
   s3 = alloca(strlen(s1) + strlen(s2) + 1);
   if (!s3) return 0;
   strcpy(s3, s1);
   strcat(s3, s2);
   STRSET(ep, params[1], s3);
   return 0;
}

/**
 * @brief Concatenates at most n bytes from one string to another.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination string.
 *               params[2] is the Embryo_Cell address of the source string to append.
 *               params[3] is the maximum number of bytes to append from the source string.
 * @return 0 on success, or if either string is NULL or memory allocation fails.
 * @note The destination string in Embryo (params[1]) is updated.
 *       A temporary buffer is allocated on the stack using alloca().
 *       The resulting string is always null-terminated.
 */
static Embryo_Cell
_embryo_str_strncat(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *s3;
   int l1, l2;

   /* params[1] = dst */
   /* params[2] = str */
   /* params[3] = n */
   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   if (params[3] < 0) params[3] = 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return 0;
   l1 = strlen(s1);
   l2 = strlen(s2);
   s3 = alloca(l1 + l2 + 1);
   if (!s3) return 0;
   strcpy(s3, s1);
   strncat(s3, s2, params[3]);
   if (l2 >= params[3]) s3[l1 + params[3]] = 0;
   STRSET(ep, params[1], s3);
   return 0;
}

/**
 * @brief Prepends one string to another. (str2 + str1)
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination string (str1).
 *               params[2] is the Embryo_Cell address of the string to prepend (str2).
 * @return 0 on success, or if either string is NULL or memory allocation fails.
 * @note The destination string in Embryo (params[1]) is updated with the
 *       result (str2 + str1). A temporary buffer is allocated on the stack using alloca().
 */
static Embryo_Cell
_embryo_str_strprep(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *s3;

   /* params[1] = dst */
   /* params[2] = str */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return 0;
   s3 = alloca(strlen(s1) + strlen(s2) + 1);
   if (!s3) return 0;
   strcpy(s3, s2);
   strcat(s3, s1);
   STRSET(ep, params[1], s3);
   return 0;
}

/**
 * @brief Prepends at most n bytes of one string to another. (first n bytes of str2 + str1)
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination string (str1).
 *               params[2] is the Embryo_Cell address of the string to prepend (str2).
 *               params[3] is the maximum number of bytes to prepend from str2.
 * @return 0 on success, or if either string is NULL or memory allocation fails.
 * @note The destination string in Embryo (params[1]) is updated.
 *       A temporary buffer is allocated on the stack using alloca().
 *       The prepended part from str2 is null-terminated if n is less than its length.
 */
static Embryo_Cell
_embryo_str_strnprep(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *s3;
   int l1, l2;

   /* params[1] = dst */
   /* params[2] = str */
   /* params[3] = n */
   if (params[0] != (3 * sizeof(Embryo_Cell))) return 0;
   if (params[3] < 0) params[3] = 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return 0;
   l1 = strlen(s1);
   l2 = strlen(s2);
   s3 = alloca(l1 + l2 + 1);
   if (!s3) return 0;
   strncpy(s3, s2, params[3]);
   if (params[3] <= l2) s3[params[3]] = 0;
   strcat(s3, s1);
   STRSET(ep, params[1], s3);
   return 0;
}

/**
 * @brief Extracts a substring from a string (slice).
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination buffer for the substring.
 *               params[2] is the Embryo_Cell address of the source string.
 *               params[3] is the starting index (n) of the substring.
 *               params[4] is the ending index (n2) of the substring (exclusive).
 * @return 0 on success.
 * @note Extracts characters from index params[3] up to (but not including) params[4].
 *       If params[3] or params[4] are out of bounds, they are adjusted.
 *       If params[4] <= params[3], an empty string is set.
 *       The result is stored in the Embryo string at params[1].
 *       A temporary buffer for the substring is allocated on the stack using alloca().
 */
static Embryo_Cell
_embryo_str_strcut(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;
   int l1;

   /* params[1] = dst */
   /* params[2] = str */
   /* params[3] = n */
   /* params[4] = n2 */
   if (params[0] != (4 * sizeof(Embryo_Cell))) return 0;
   if (params[3] < 0) params[3] = 0;
   if (params[4] < params[3]) params[4] = params[3];
   STRGET(ep, s1, params[2]);
   if (!s1) return 0;
   l1 = strlen(s1);
   if (params[3] >= l1) params[3] = l1;
   if (params[4] >= l1) params[4] = l1;
   if (params[4] == params[3])
     {
        STRSET(ep, params[1], "");
        return 0;
     }
   s2 = alloca(params[4] - params[3] + 1);
   strncpy(s2, s1 + params[3], params[4] - params[3]);
   s2[params[4] - params[3]] = 0;
   STRSET(ep, params[1], s2);
   return 0;
}

/**
 * @brief Internal helper function to format a string, similar to snprintf.
 *
 * This function processes a format string (s1) and a list of arguments (params)
 * to produce a formatted output string (s2), respecting a maximum length (max_len).
 * It supports a subset of printf-style format specifiers:
 * - %%: Literal '%'
 * - %c: Character
 * - %i, %d: Signed decimal integer
 * - %x: Unsigned hexadecimal integer (lowercase)
 * - %X: Unsigned hexadecimal integer (uppercase, 8 digits, zero-padded)
 * - %f: Floating-point number
 * - %s: String
 * It also supports escape sequences:
 * - \\t: Tab
 * - \\n: Newline
 * - \\other: Literal 'other' character
 *
 * @param ep The Embryo program.
 * @param s1 The format string.
 * @param s2 The output buffer for the formatted string.
 * @param max_len The maximum number of characters to write to s2 (excluding null terminator).
 * @param pnum The number of arguments in the params array.
 * @param params Array of Embryo_Cell arguments for the format specifiers.
 * @return The number of characters written to s2 (excluding the null terminator).
 */
static Embryo_Cell
_str_snprintf(Embryo_Program *ep, char *s1, char *s2, int max_len, int pnum, Embryo_Cell *params)
{
   int i, o, p;
   int inesc = 0;
   int insub = 0;

   for (p = 0, o = 0, i = 0; (s1[i]) && (o < max_len) && (p < (pnum + 1)); i++)
     {
        if ((!inesc) && (!insub))
          {
             if (s1[i] == '\\') inesc = 1;
             else if (s1[i] == '%')
               insub = 1;
             if ((!inesc) && (!insub))
               {
                  s2[o] = s1[i];
                  o++;
               }
          }
        else
          {
             Embryo_Cell *cptr;

             if (inesc)
               {
                  switch (s1[i])
                    {
                     case 't':
                       s2[o] = '\t';
                       o++;
                       break;

                     case 'n':
                       s2[o] = '\n';
                       o++;
                       break;

                     default:
                       s2[o] = s1[i];
                       o++;
                       break;
                    }
                  inesc = 0;
               }
             if ((insub) && (s1[i] == '%')) pnum++;
             if ((insub) && (p < pnum))
               {
                  switch (s1[i])
                    {
                     case '%':
                       s2[o] = '%';
                       o++;
                       break;

                     case 'c':
                       cptr = embryo_data_address_get(ep, params[p]);
                       if (cptr) s2[o] = (char)(*cptr);
                       p++;
                       o++;
                       break;

                     case 'i':
                     case 'd':
                     case 'x':
                     case 'X':
                     {
                        char fmt[10] = "";
                        char tmp[256] = "";
                        int l;

                        if (s1[i] == 'i') strcpy(fmt, "%i");
                        else if (s1[i] == 'd')
                          strcpy(fmt, "%d");
                        else if (s1[i] == 'x')
                          strcpy(fmt, "%x");
                        else if (s1[i] == 'X')
                          strcpy(fmt, "%08x");
                        cptr = embryo_data_address_get(ep, params[p]);
                        if (cptr) snprintf(tmp, sizeof(tmp), fmt, (int)(*cptr));
                        l = strlen(tmp);
                        if ((o + l) > max_len)
                          {
                             l = max_len - o;
                             if (l < 0) l = 0;
                             tmp[l] = 0;
                          }
                        strcpy(s2 + o, tmp);
                        o += l;
                        p++;
                     }
                     break;

                     case 'f':
                     {
                        char tmp[256] = "";
                        int l;

                        cptr = embryo_data_address_get(ep, params[p]);
                        if (cptr) snprintf(tmp, sizeof(tmp), "%f", (double)EMBRYO_CELL_TO_FLOAT(*cptr));
                        l = strlen(tmp);
                        if ((o + l) > max_len)
                          {
                             l = max_len - o;
                             if (l < 0) l = 0;
                             tmp[l] = 0;
                          }
                        strcpy(s2 + o, tmp);
                        o += l;
                        p++;
                     }
                     break;

                     case 's':
                     {
                        char *tmp;
                        int l;

                        STRGET(ep, tmp, params[p]);
                        if (tmp)
                          {
                             l = strlen(tmp);
                             if ((o + l) > max_len)
                               {
                                  l = max_len - o;
                                  if (l < 0) l = 0;
                                  tmp[l] = 0;
                               }
                             strcpy(s2 + o, tmp);
                             o += l;
                          }
                        p++;
                     }
                     break;

                     default:
                       break;
                    }
                  insub = 0;
               }
             else if (insub)
               insub = 0;
          }
     }
   s2[o] = 0;

   return o;
}

/**
 * @brief Formats a string and stores it in a buffer, similar to C snprintf.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the destination buffer.
 *               params[2] is the size of the destination buffer (Embryo_Cell).
 *               params[3] is the Embryo_Cell address of the format string.
 *               params[4]... are Embryo_Cell addresses or values for format arguments.
 * @return The number of characters that would have been written if the buffer
 *         was large enough (currently returns 0, but should be length of s2).
 *         Returns -1 on error (e.g., NULL format string, allocation failure).
 * @note Uses _str_snprintf internally. The result is stored in the Embryo string at params[1].
 *       A temporary buffer for the formatted string is allocated on the stack using alloca().
 */
static Embryo_Cell
_embryo_str_snprintf(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;
   int o = 0; // TODO: This should be the return value from _str_snprintf
   int pnum;

   /* params[1] = buf */
   /* params[2] = bufsize */
   /* params[3] = format_string */
   /* params[4] = first arg ... */
   if (params[0] < (Embryo_Cell)(3 * sizeof(Embryo_Cell))) return 0;
   if (params[2] <= 0) return 0;
   STRGET(ep, s1, params[3]);
   if (!s1) return -1;
   s2 = alloca(params[2] + 1);
   if (!s2) return -1;
   s2[0] = 0;
   pnum = (params[0] / sizeof(Embryo_Cell)) - 3;

   _str_snprintf(ep, s1, s2, params[2], pnum, &params[4]);

   STRSET(ep, params[1], s2);

   return o;
}

/**
 * @brief Formats a string and prints it to the log (INF).
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the format string.
 *               params[2]... are Embryo_Cell addresses or values for format arguments.
 * @return Currently returns 0. (Potentially should return number of chars printed).
 *         Returns -1 on error (e.g., NULL format string, allocation failure).
 * @note Uses _str_snprintf internally to format the string before printing.
 *       A temporary buffer for the formatted string is allocated on the stack using alloca().
 *       The maximum length of this buffer is estimated based on format string length
 *       and number of arguments.
 */
static Embryo_Cell
_embryo_str_printf(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2;
   int o = 0; // TODO: This should be the return value from _str_snprintf
   int pnum;
   int max_len = 0;

   /* params[1] = format_string */
   /* params[2] = first arg ... */
   if (params[0] < (Embryo_Cell)(1 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   if (!s1) return -1;
   max_len = strlen(s1) + (params[0] - 1) * 256;
   s2 = alloca(max_len + 1);
   if (!s2) return -1;
   s2[0] = 0;
   pnum = (params[0] / sizeof(Embryo_Cell)) - 1;

   _str_snprintf(ep, s1, s2, max_len, pnum, &params[2]);

   INF("%s", s2);

   return o;
}

/**
 * @brief Locates the first occurrence of a substring within a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the string to search in (haystack).
 *               params[2] is the Embryo_Cell address of the substring to search for (needle).
 * @return The index of the first occurrence of the needle in the haystack,
 *         or -1 if the needle is not found or on error.
 */
static Embryo_Cell
_embryo_str_strstr(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *p;

   /* params[1] = str */
   /* params[2] = ndl */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   p = strstr(s1, s2);
   if (!p) return -1;
   return (Embryo_Cell)(p - s1);
}

/**
 * @brief Locates the first occurrence of a character in a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the string to search in.
 *               params[2] is the Embryo_Cell address of a string containing the character to find
 *                        (only the first character of this string is used).
 * @return The index of the first occurrence of the character in the string,
 *         or -1 if the character is not found or on error.
 */
static Embryo_Cell
_embryo_str_strchr(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *p;

   /* params[1] = str */
   /* params[2] = ch */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   p = strchr(s1, s2[0]);
   if (!p) return -1;
   return (Embryo_Cell)(p - s1);
}

/**
 * @brief Locates the last occurrence of a character in a string.
 * @param ep The Embryo program.
 * @param params Parameters for the native call.
 *               params[0] is the number of bytes for the arguments.
 *               params[1] is the Embryo_Cell address of the string to search in.
 *               params[2] is the Embryo_Cell address of a string containing the character to find
 *                        (only the first character of this string is used).
 * @return The index of the last occurrence of the character in the string,
 *         or -1 if the character is not found or on error.
 */
static Embryo_Cell
_embryo_str_strrchr(Embryo_Program *ep, Embryo_Cell *params)
{
   char *s1, *s2, *p;

   /* params[1] = str */
   /* params[2] = ch */
   if (params[0] != (2 * sizeof(Embryo_Cell))) return 0;
   STRGET(ep, s1, params[1]);
   STRGET(ep, s2, params[2]);
   if ((!s1) || (!s2)) return -1;
   p = strrchr(s1, s2[0]);
   if (!p) return -1;
   return (Embryo_Cell)(p - s1);
}

/* functions used by the rest of embryo */

/**
 * @brief Initializes the string manipulation native calls for an Embryo program.
 *
 * This function registers all the public string functions (e.g., "atoi", "strcmp")
 * with the Embryo program, making them available for use within Embryo scripts.
 *
 * @param ep Pointer to the Embryo_Program to initialize.
 */
void
_embryo_str_init(Embryo_Program *ep)
{
   embryo_program_native_call_add(ep, "atoi", _embryo_str_atoi);
   embryo_program_native_call_add(ep, "fnmatch", _embryo_str_fnmatch);
   embryo_program_native_call_add(ep, "strcmp", _embryo_str_strcmp);
   embryo_program_native_call_add(ep, "strncmp", _embryo_str_strncmp);
   embryo_program_native_call_add(ep, "strcpy", _embryo_str_strcpy);
   embryo_program_native_call_add(ep, "strncpy", _embryo_str_strncpy);
   embryo_program_native_call_add(ep, "strlen", _embryo_str_strlen);
   embryo_program_native_call_add(ep, "strcat", _embryo_str_strcat);
   embryo_program_native_call_add(ep, "strncat", _embryo_str_strncat);
   embryo_program_native_call_add(ep, "strprep", _embryo_str_strprep);
   embryo_program_native_call_add(ep, "strnprep", _embryo_str_strnprep);
   embryo_program_native_call_add(ep, "strcut", _embryo_str_strcut);
   embryo_program_native_call_add(ep, "snprintf", _embryo_str_snprintf);
   embryo_program_native_call_add(ep, "strstr", _embryo_str_strstr);
   embryo_program_native_call_add(ep, "strchr", _embryo_str_strchr);
   embryo_program_native_call_add(ep, "strrchr", _embryo_str_strrchr);
   embryo_program_native_call_add(ep, "printf", _embryo_str_printf);
}

