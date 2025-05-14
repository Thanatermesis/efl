#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Eet.h>
#include <Ecore.h>
#include <Ecore_File.h>

/* define macros and variable for using the eina logging system  */
#define EFREET_MODULE_LOG_DOM /* no logging in this file */

#include "Efreet.h"
#include "efreet_private.h"
#include "efreet_xml.h"

/*
 * Needs EAPI because of helper binaries
 */
/**
 * @brief Global flag to enable/disable cache updates.
 * Set to 0 to disable automatic cache updates, 1 (default) to enable.
 * This is primarily used by helper binaries that should not trigger cache
 * regeneration.
 */
EAPI int efreet_cache_update = 1;

/**
 * @internal
 * @brief Counter for efreet_init() calls.
 * Ensures that Efreet is initialized only once and shut down when the count
 * reaches zero.
 */
static int _efreet_init_count = 0;
/**
 * @internal
 * @brief Flag indicating if the locale has been parsed.
 * Used to avoid redundant parsing of locale settings.
 */
static int efreet_parsed_locale = 0;
/**
 * @internal
 * @brief Stores the language part of the locale (e.g., "en").
 */
static const char *efreet_lang = NULL;
/**
 * @internal
 * @brief Stores the country part of the locale (e.g., "US").
 */
static const char *efreet_lang_country = NULL;
/**
 * @internal
 * @brief Stores the modifier part of the locale (e.g., "UTF-8").
 */
static const char *efreet_lang_modifier = NULL;
/**
 * @internal
 * @brief Stores the full language string (e.g., "en_US.UTF-8@modifier").
 */
static const char *efreet_language = NULL;
static void efreet_parse_locale(void);
static int efreet_parse_locale_setting(const char *env);

#ifndef _WIN32
static uid_t ruid;
static uid_t rgid;
#endif

/**
 * @brief Initializes the Efreet library.
 * @return The new init count, or 0 on failure.
 * This function initializes all necessary Efreet subsystems. It should be
 * called before any other Efreet function. It handles multiple calls by
 * incrementing an internal counter and only performing initialization on the
 * first call.
 */
EAPI int
efreet_init(void)
{
#ifndef _WIN32
   char *tmp;
#endif

   if (++_efreet_init_count != 1)
     return _efreet_init_count;

#ifndef _WIN32
   /* Find users real uid and gid */
   tmp = getenv("SUDO_UID");
   if (tmp)
     ruid = strtoul(tmp, NULL, 10);
   else
     ruid = getuid();

   tmp = getenv("SUDO_GID");
   if (tmp)
     rgid = strtoul(tmp, NULL, 10);
   else
     rgid = getgid();
#endif

   if (!eina_init())
     return --_efreet_init_count;
   if (!eet_init())
     goto shutdown_eina;
   if (!ecore_init())
     goto shutdown_eet;
   if (!ecore_file_init())
     goto shutdown_ecore;

   if (!efreet_base_init())
     goto shutdown_ecore_file;

   if (!efreet_cache_init())
     goto shutdown_efreet_base;

   if (!efreet_xml_init())
     goto shutdown_efreet_cache;

   if (!efreet_icon_init())
     goto shutdown_efreet_xml;

   if (!efreet_ini_init())
     goto shutdown_efreet_icon;

   if (!efreet_desktop_init())
     goto shutdown_efreet_ini;

   if (!efreet_menu_init())
     goto shutdown_efreet_desktop;

   if (!efreet_util_init())
     goto shutdown_efreet_menu;

   if (!efreet_internal_mime_init())
     goto shutdown_efreet_mime;

   if (!efreet_internal_trash_init())
     goto shutdown_efreet_trash;

#ifdef ENABLE_NLS
   bindtextdomain(PACKAGE, LOCALE_DIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

   return _efreet_init_count;

shutdown_efreet_trash:
   efreet_internal_trash_shutdown();
shutdown_efreet_mime:
   efreet_internal_mime_shutdown();
shutdown_efreet_menu:
   efreet_menu_shutdown();
shutdown_efreet_desktop:
   efreet_desktop_shutdown();
shutdown_efreet_ini:
   efreet_ini_shutdown();
shutdown_efreet_icon:
   efreet_icon_shutdown();
shutdown_efreet_xml:
   efreet_xml_shutdown();
shutdown_efreet_cache:
   efreet_cache_shutdown();
shutdown_efreet_base:
   efreet_base_shutdown();
shutdown_ecore_file:
   ecore_file_shutdown();
shutdown_ecore:
   ecore_shutdown();
shutdown_eet:
   eet_shutdown();
shutdown_eina:
   eina_shutdown();

   return --_efreet_init_count;
}

/**
 * @brief Shuts down the Efreet library.
 * @return The new init count.
 * This function shuts down all Efreet subsystems. It decrements an internal
 * counter and only performs shutdown when the counter reaches zero.
 */
EAPI int
efreet_shutdown(void)
{
   if (_efreet_init_count <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return 0;
     }
   if (--_efreet_init_count != 0)
     return _efreet_init_count;

   efreet_util_shutdown();
   efreet_menu_shutdown();
   efreet_desktop_shutdown();
   efreet_ini_shutdown();
   efreet_icon_shutdown();
   efreet_xml_shutdown();
   efreet_cache_shutdown();
   efreet_base_shutdown();
   efreet_internal_mime_shutdown();
   efreet_internal_trash_shutdown();

   IF_RELEASE(efreet_lang);
   IF_RELEASE(efreet_lang_country);
   IF_RELEASE(efreet_lang_modifier);
   IF_RELEASE(efreet_language);
   efreet_parsed_locale = 0;  /* reset this in case they init efreet again */

   ecore_file_shutdown();
   ecore_shutdown();
   eet_shutdown();
   eina_shutdown();

   return _efreet_init_count;
}

/**
 * @brief Resets the current language settings and reparses them.
 * This function is useful if the system locale changes while the application
 * is running. It clears any cached locale information, re-parses the
 * environment, and rebuilds relevant caches like the desktop file cache.
 */
EAPI void
efreet_lang_reset(void)
{
   IF_RELEASE(efreet_lang);
   IF_RELEASE(efreet_lang_country);
   IF_RELEASE(efreet_lang_modifier);
   IF_RELEASE(efreet_language);
   efreet_parsed_locale = 0;  /* reset this in case they init efreet again */

   efreet_dirs_reset();
   efreet_parse_locale();
   efreet_cache_desktop_close();
   efreet_cache_desktop_build();
}

/**
 * @internal
 * @return Returns the current user's language setting (e.g., "en") or @c NULL if not set.
 * @brief Retrieves the current language setting
 */
const char *
efreet_lang_get(void)
{
   if (efreet_parsed_locale) return efreet_lang;

   efreet_parse_locale();
   return efreet_lang;
}

/**
 * @internal
 * @return Returns the current language country setting (e.g., "US") or @c NULL if not set.
 * @brief Retrieves the current country setting for the current language.
 */
const char *
efreet_lang_country_get(void)
{
   if (efreet_parsed_locale) return efreet_lang_country;

   efreet_parse_locale();
   return efreet_lang_country;
}

/**
 * @internal
 * @return Returns the current language modifier setting (e.g., "latin") or @c NULL if not set.
 * @brief Retrieves the modifier setting for the language (e.g., from LANG=en_US@latin).
 */
const char *
efreet_lang_modifier_get(void)
{
   if (efreet_parsed_locale) return efreet_lang_modifier;

   efreet_parse_locale();
   return efreet_lang_modifier;
}

/**
 * @brief Retrieves the full current language string.
 * @return The full language string (e.g., "en_US.UTF-8", "de_DE@euro") as
 *         obtained from the environment or system settings. Returns "C" if
 *         no language setting can be determined. The returned string is
 *         an Eina_Stringshare, so it should not be freed by the caller.
 *
 * This function parses the locale on its first call if not already parsed.
 * Subsequent calls return the cached value.
 */
EAPI const char *
efreet_language_get(void)
{
   if (efreet_parsed_locale) return efreet_language;

   efreet_parse_locale();
   return efreet_language;
}

/**
 * @internal
 * @return Returns no value
 * @brief Parses out the language, country and modifer setting from the
 * LC_MESSAGES environment variable on UNIX. On Windows, retrieve them from
 * the system. It tries "LANG", then "LC_ALL", then "LC_MESSAGES". If none
 * are found or parseable, it defaults to "C".
 */
static void
efreet_parse_locale(void)
{
   efreet_parsed_locale = 1;

   if (efreet_parse_locale_setting("LANG"))
     return;

   if (efreet_parse_locale_setting("LC_ALL"))
     return;

   if (efreet_parse_locale_setting("LC_MESSAGES"))
     return;

   efreet_language = eina_stringshare_add("C");
}

/**
 * @internal
 * @param env The environment variable to grab
 * @return Returns 1 if we parsed something of @a env, 0 otherwise
 * @brief Tries to parse the language, country, and modifier from the given environment variable.
 *
 * On POSIX systems, it parses @p env (e.g., "LANG", "LC_MESSAGES").
 * The format expected is lang[_COUNTRY][.CODESET][@MODIFIER].
 * Examples: "en_US.UTF-8@valencia", "de_DE", "fr@paris".
 *
 * On Windows, @p env is ignored, and locale information is retrieved using
 * GetLocaleInfo with LOCALE_SYSTEM_DEFAULT.
 *
 * @param env The environment variable name (e.g., "LANG") to read from (POSIX only).
 * @return Returns 1 if locale information was successfully parsed and set, 0 otherwise.
 */
static int
efreet_parse_locale_setting(const char *env)
{
#ifdef _WIN32
   char buf_lang[18];
   char buf[9];
   int l1;
   int l2;

   l1 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME,
                      buf, sizeof(buf));
   if (!l1)
     return 0;

   efreet_lang = eina_stringshare_add(buf);
   memcpy(buf_lang, buf, l1 - 1);
   buf_lang[l1 - 1] = '_';

   l2 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME,
                      buf, sizeof(buf));
   if (!l2)
     return 0;

   efreet_lang_country = eina_stringshare_add(buf);
   memcpy(buf_lang + l1, buf, l2);

   efreet_language = eina_stringshare_add(buf_lang);

   return 1;

   (void)env;
#else
   int found = 0;
   char *setting;
   char *p;
   size_t len;

   p = getenv(env);
   if (!p) return 0;
   len = strlen(p) + 1;
   setting = alloca(len);
   memcpy(setting, p, len);

   /* pull the modifier off the end */
   p = strrchr(setting, '@');
   if (p)
     {
        *p = '\0';
        efreet_lang_modifier = eina_stringshare_add(p + 1);
        found = 1;
     }

   /* if there is an encoding we ignore it */
   p = strrchr(setting, '.');
   if (p) *p = '\0';

   /* get the country if available */
   p = strrchr(setting, '_');
   if (p)
     {
        *p = '\0';
        efreet_lang_country = eina_stringshare_add(p + 1);
        found = 1;
     }

   if (*setting != '\0')
     {
        efreet_lang = eina_stringshare_add(setting);
        found = 1;
     }

   if (found)
     efreet_language = eina_stringshare_add(getenv(env));
   return found;
#endif
}

/**
 * @internal
 * @param buffer The destination buffer
 * @param size The destination buffer size
 * @param buffer The destination buffer.
 * @param size The destination buffer size.
 * @param strs A NULL-terminated array of C-strings to concatenate.
 *             Example: `const char *my_strs[] = {"Hello", " ", "World", NULL};`
 * @return Returns the total number of bytes written to @a buffer, excluding the
 *         null terminator if the buffer was large enough, or the number of bytes
 *         that would have been written if the buffer was truncated. This is
 *         consistent with eina_strlcpy behavior.
 * @brief Concatenates the strings in the @a strs array into the given @a buffer,
 * ensuring not to exceed @a size. This function uses eina_strlcpy for safe
 * string copying.
 */
size_t
efreet_array_cat(char *buffer, size_t size, const char *strs[])
{
   int i;
   size_t n;
   for (i = 0, n = 0; n < size && strs[i]; i++)
     {
        n += eina_strlcpy(buffer + n, strs[i], size - n);
     }
   return n;
}

#ifndef _WIN32
/**
 * @brief Changes the owner and group of an open file descriptor to the
 *        real user ID and group ID.
 * @param fd The file descriptor of the file whose ownership is to be changed.
 *
 * This function is a no-op if the file descriptor is invalid, if fstat fails,
 * if the file is already owned by the real user, or if fchown fails.
 * It is intended to be used in scenarios where a program running with elevated
 * privileges (e.g., via sudo) creates files that should be owned by the
 * original user.
 * This function is not available on Windows.
 */
EAPI void
efreet_fsetowner(int fd)
{
   struct stat st;

   if (fd < 0) return;
   if (fstat(fd, &st) < 0) return;
   if (st.st_uid == ruid) return;

   if (fchown(fd, ruid, rgid) != 0) return;
}
#else
/**
 * @brief Placeholder for efreet_fsetowner on Windows.
 * @param fd Unused.
 * This function is a no-op on Windows.
 */
EAPI void
efreet_fsetowner(int fd EINA_UNUSED)
{
}
#endif

#ifndef _WIN32
/**
 * @brief Changes the owner and group of a file to the real user ID and group ID.
 * @param path The path to the file whose ownership is to be changed.
 *
 * This function opens the file, calls efreet_fsetowner() on the file
 * descriptor, and then closes it. It handles errors during open silently.
 * It is intended for the same use cases as efreet_fsetowner().
 * This function is not available on Windows.
 */
EAPI void
efreet_setowner(const char *path)
{
   EINA_SAFETY_ON_NULL_RETURN(path);

   int fd;

   fd = open(path, O_RDONLY);
   if (fd < 0) return;
   efreet_fsetowner(fd);
   close(fd);
}
#else
/**
 * @brief Placeholder for efreet_setowner on Windows.
 * @param path Unused.
 * This function is a no-op on Windows.
 */
EAPI void
efreet_setowner(const char *path EINA_UNUSED)
{
}
#endif
