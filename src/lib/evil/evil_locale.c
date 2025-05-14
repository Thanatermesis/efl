#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <locale.h>
#include <errno.h>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "evil_private.h" /* LC_MESSAGES */

/*
 * LOCALE_SISO639LANGNAME and LOCALE_SISO3166CTRYNAME need at least a buffer
 * of 9 char each (including NULL char). So we need 2*8 + the trailing NULL
 * char + '_', so 18 char.
 */
/**
 * @internal
 * @brief Static buffer to store the locale string for LC_MESSAGES.
 *
 * This buffer is used to construct and store the locale string in the format
 * "language_COUNTRY" (e.g., "en_US") when evil_setlocale() is called with
 * LC_MESSAGES and a NULL locale. The size is calculated to accommodate
 * the 2-letter language code, 2-letter country code, an underscore,
 * and a null terminator. Specifically, LOCALE_SISO639LANGNAME and
 * LOCALE_SISO3166CTRYNAME can return up to 8 characters each, plus a
 * null terminator. So, (8 + 8 + 1 (for '_') + 1 (for '\0')) = 18.
 */
static char _evil_locale_buf[18];

#undef setlocale

/**
 * @internal
 * @brief Sets or queries the program's locale for a specific category.
 *
 * This function is a wrapper around the standard C setlocale function,
 * but with special handling for the LC_MESSAGES category on Windows.
 *
 * For categories other than LC_MESSAGES, this function behaves identically
 * to the standard setlocale.
 *
 * When @p category is LC_MESSAGES:
 * - If @p locale is not NULL, the function sets errno to EINVAL and
 *   returns NULL, as setting LC_MESSAGES to a specific locale string
 *   is not supported by this implementation.
 * - If @p locale is NULL, the function queries the system's default
 *   locale. It retrieves the ISO 639 language name and ISO 3166 country
 *   name using Windows API calls (GetLocaleInfo). These are then combined
 *   into a string of the format "ll_CC" (e.g., "en_US") and stored in
 *   the static buffer _evil_locale_buf. A pointer to this buffer is returned.
 *   This buffer is overwritten on subsequent calls with LC_MESSAGES and
 *   NULL locale.
 *
 * @param category The locale category to be affected. See setlocale documentation
 *                 and evil_locale.h for possible values (e.g., LC_ALL, LC_CTYPE,
 *                 LC_MESSAGES).
 * @param locale A string specifying the locale. If NULL, the function queries
 *               the current setting for the given category. If "", it requests
 *               the system's default native environment.
 * @return If @p locale is not NULL and the call is successful, returns a pointer
 *         to a string associated with the specified @p category for the new @p locale.
 *         If @p locale is NULL, returns a pointer to a string associated with the
 *         specified @p category for the current locale.
 *         If the call fails (e.g., invalid category or locale, or for LC_MESSAGES
 *         with a non-NULL locale), returns NULL and errno may be set.
 *         The returned string for LC_MESSAGES (when locale is NULL) points to a
 *         static internal buffer and should not be modified or freed by the caller.
 */
EVIL_API char *
evil_setlocale(int category, const char *locale)
{
   char buf[9]; /* Temporary buffer for GetLocaleInfo results.
                 * LOCALE_SISO639LANGNAME and LOCALE_SISO3166CTRYNAME
                 * require a buffer of 9 chars including the NULL terminator. */
   int l1;
   int l2;

   if (category != LC_MESSAGES)
     return setlocale(category, locale);

   if (locale != NULL)
     {
        errno = EINVAL;
        return NULL;
     }

   l1 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME,
                      buf, sizeof(buf));
   if (!l1) return NULL;

   memcpy(_evil_locale_buf, buf, l1 - 1);
   _evil_locale_buf[l1 - 1] = '_';

   l2 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME,
                      buf, sizeof(buf));
   if (!l2) return NULL;

   memcpy(_evil_locale_buf + l1, buf, l2);

   return _evil_locale_buf;
}
