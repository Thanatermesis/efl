/**
 * @file
 * @brief Implementation of nl_langinfo function.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include "evil_private.h"

/**
 * @internal
 * @brief Replaces a dynamically allocated string with a new one.
 *
 * If `prev` is not NULL, it is freed. If `value` is not NULL,
 * a duplicate of `value` is created and returned.
 * This function is used to manage the static buffer `result` in `nl_langinfo`.
 *
 * @param prev Pointer to the previous string (to be freed).
 * @param value Pointer to the new string content (to be duplicated).
 * @return A pointer to the newly allocated string if `value` is not NULL,
 *         otherwise returns `prev` (which would be NULL if `value` was NULL
 *         and `prev` was also NULL, or `prev` if `value` was NULL and `prev` was not).
 *         Returns NULL if strdup fails.
 */
static char *
replace(char *prev, char *value)
{
   if (!value)
     return prev;

   if (prev)
     free (prev);
   return strdup (value);
}

/**
 * @brief Retrieve locale-specific information.
 * @see nl_langinfo in evil_langinfo.h for detailed parameter and return value descriptions.
 *
 * This function provides an implementation for nl_langinfo, returning
 * strings for various locale items. It uses a static buffer for some
 * results, meaning subsequent calls might invalidate previous results.
 * For CODESET, it attempts to parse the locale string to extract the
 * codeset, potentially modifying it to a "cp" prefix if it looks like
 * a codepage number.
 */
EVIL_API char *
nl_langinfo(nl_item index)
{
   static char *result = NULL;
   static char *nothing = "";

   switch (index)
     {
      case CODESET:
        {
           char *p;
           result = replace(result, setlocale(LC_CTYPE, NULL));
           if (!(p = strrchr(result, '.')))
             return nothing;

           if ((++p - result) > 2)
             strcpy(result, "cp");
           else
             *result = '\0';
           strcat(result, p);

           return result;
        }
      case RADIXCHAR:
        {
           return localeconv()->decimal_point;
        }
      case D_T_FMT:
        {
           return "%a %d %b %Y %T %Z";
        }
      case D_FMT:
        {
           return "%m/%d/%Y";
        }
      case T_FMT:
        {
           return "%T";
        }
      case T_FMT_AMPM:
        {
           return "%r";
        }
      default:
        {
           return "%a %d %b %Y %T %Z";
        }
     }

   return nothing;
}
