#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "evil_private.h"


/*
 * string related functions
 *
 */

EVIL_API char *strcasestr(const char *haystack, const char *needle)
{
   size_t length_needle;
   size_t length_needle;
   size_t length_haystack;
   size_t i;

   if (!haystack || !needle)
     return NULL;

   length_needle = strlen(needle);
   length_haystack = strlen(haystack);

   /* If haystack is shorter than needle, needle cannot be found. */
   if (length_haystack < length_needle) return NULL;

   /* Calculate the number of possible starting positions for needle in haystack. */
   length_haystack = length_haystack - length_needle + 1;

   /* Iterate through all possible starting positions of needle in haystack. */
   for (i = 0; i < length_haystack; i++)
     {
        size_t j;

        /* Compare needle with substring of haystack starting at i. */
        for (j = 0; j < length_needle; j++)
          {
            unsigned char c1;
            unsigned char c2;

            c1 = haystack[i+j];
            c2 = needle[j];
            /* Compare characters case-insensitively. */
            if (toupper(c1) != toupper(c2))
              goto next; /* Mismatch, try next starting position in haystack */
          }
        return (char *) haystack + i; /* Found needle, return pointer to its start. */
     next:
        ; /* Label for goto, used to continue to the next iteration of the outer loop. */
     }

   return NULL; /* Needle not found. */
}

EVIL_API char *
strsep (char **stringp, const char *delim)
{
  char *begin, *end;

  begin = *stringp;
  if (begin == NULL)
    return NULL; /* No string to parse. */

  /*
   * Optimization: If the delimiter string is empty or contains only one character,
   * use strchr for potentially faster searching than strpbrk.
   */
  if (delim[0] == '\0' || delim[1] == '\0')
    {
      char ch = delim[0];

      if (ch == '\0')
        /* If delimiter is an empty string, there's no delimiter to find.
         * The original string is considered a single token. */
        end = NULL;
      else
        {
          /* Handle single character delimiter. */
          if (*begin == ch) /* If the first character is the delimiter. */
            end = begin;
          else if (*begin == '\0') /* If the string is empty. */
            end = NULL;
          else
            /* Search for the delimiter starting from the second character. */
            end = strchr (begin + 1, ch);
        }
    }
  else
    /* For multiple delimiter characters, find the first occurrence of any delimiter. */
    end = strpbrk (begin, delim);

  if (end)
    {
      /* A delimiter was found. Terminate the token with a NUL character. */
      *end++ = '\0';
      /* Advance *stringp to point to the beginning of the next token. */
      *stringp = end;
    }
  else
    /* No delimiter was found; this is the last token in the string.
     * Set *stringp to NULL to indicate the end of parsing. */
    *stringp = NULL;

  /* Return the beginning of the found token. */
  return begin;
}
