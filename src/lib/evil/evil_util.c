#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

#include "evil_private.h"

/**
 * @internal
 * @brief Index for thread-local storage.
 *
 * This variable stores the index allocated by TlsAlloc() for thread-local
 * storage. It is used by evil_format_message() to store the formatted
 * error string in a per-thread buffer. This allows evil_format_message()
 * to be thread-safe without requiring the caller to manage buffers.
 */
DWORD _evil_tls_index;

/* static void _evil_error_display(const char *fct, LONG res); */

/**
 * @internal
 * @brief Displays the last error message to stderr.
 *
 * This function retrieves the last error message using evil_last_error_get()
 * and prints it to the standard error stream. The message is prefixed with
 * "[Evil]" and the name of the function where the error occurred, aiding in
 * debugging.
 *
 * @param fct The name of the function (typically `__func__`) where the error
 *            was detected and this display function is called from.
 *            Example: "evil_char_to_wchar".
 */
static void _evil_last_error_display(const char *fct);

/**
 * @brief Converts a multi-byte character string to a wide character string.
 *
 * @param text The null-terminated multi-byte string to convert.
 *             This string is typically encoded in the system's default ANSI code page (CP_ACP).
 *             Example: "Hello"
 * @return A pointer to a newly allocated wide character string (UTF-16 on Windows)
 *         representing the converted text. This memory must be freed by the caller
 *         using `free()`.
 *         Returns `NULL` if the input `text` is `NULL`, if memory allocation fails,
 *         or if the conversion fails (e.g., due to invalid characters or insufficient buffer).
 *         Example: L"Hello"
 *
 * This function uses `MultiByteToWideChar` with `CP_ACP` for the conversion.
 * It first determines the required buffer size and then performs the conversion.
 * If any step fails, `_evil_last_error_display` is called to log the error.
 */
EVIL_API wchar_t *
evil_char_to_wchar(const char *text)
{
   wchar_t *wtext;
   int      wsize;

   if (!text)
     return NULL;

   wsize = MultiByteToWideChar(CP_ACP, 0, text, (int)strlen(text) + 1, NULL, 0);
   if ((wsize == 0) ||
       (wsize > (int)(ULONG_MAX / sizeof(wchar_t))))
     {
        if (wsize == 0)
          _evil_last_error_display(__func__);
        return NULL;
     }

   wtext = malloc(wsize * sizeof(wchar_t));
   if (wtext)
     if (!MultiByteToWideChar(CP_ACP, 0, text, (int)strlen(text) + 1, wtext, wsize))
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   return wtext;
}

/**
 * @brief Converts a wide character string to a multi-byte character string.
 *
 * @param text The null-terminated wide character string (UTF-16 on Windows) to convert.
 *             Example: L"World"
 * @return A pointer to a newly allocated multi-byte character string.
 *         This string is typically encoded in the system's default ANSI code page (CP_ACP).
 *         This memory must be freed by the caller using `free()`.
 *         Returns `NULL` if the input `text` is `NULL`, if memory allocation fails,
 *         or if the conversion fails.
 *         Example: "World"
 *
 * This function uses `WideCharToMultiByte` with `CP_ACP` for the conversion.
 * It determines the required buffer size and then performs the conversion.
 * If any step fails, `_evil_last_error_display` is called to log the error.
 */
EVIL_API char *
evil_wchar_to_char(const wchar_t *text)
{
   char  *atext;
   int    asize;

   if (!text)
     return NULL;

   asize = WideCharToMultiByte(CP_ACP, 0, text, -1, NULL, 0, NULL, NULL);
   if (asize == 0)
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   atext = (char*)malloc(asize * sizeof(char));
   if (!atext)
     return NULL;

   asize = WideCharToMultiByte(CP_ACP, 0, text, -1, atext, asize, NULL, NULL);
   if (asize == 0)
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   return atext;
}

/**
 * @brief Converts a UTF-16 (wide character) string to a UTF-8 string.
 *
 * @param text16 The null-terminated UTF-16 string to convert.
 *               Example: L"你好" (Ni Hao - Hello in Chinese)
 * @return A pointer to a newly allocated UTF-8 encoded string.
 *         This memory must be freed by the caller using `free()`.
 *         Returns `NULL` if the input `text16` is `NULL`, if memory allocation fails,
 *         or if the conversion fails (e.g., invalid UTF-16 sequence).
 *         Example: "\xE4\xBD\xA0\xE5\xA5\xBD"
 *
 * This function uses `WideCharToMultiByte` with `CP_UTF8` and the
 * `WC_ERR_INVALID_CHARS` flag, which causes the function to fail if an invalid
 * input character is encountered.
 * If any step fails, `_evil_last_error_display` is called to log the error.
 */
EVIL_API char *
evil_utf16_to_utf8(const wchar_t *text16)
{
   char  *text8;
   DWORD  flag = 0;
   int    size8;

   if (!text16)
     return NULL;

   flag = WC_ERR_INVALID_CHARS;

   size8 = WideCharToMultiByte(CP_UTF8, flag, text16, -1, NULL, 0, NULL, NULL);
   if (size8 == 0)
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   text8 = (char*)malloc(size8 * sizeof(char));
   if (!text8)
     return NULL;

   size8 = WideCharToMultiByte(CP_UTF8, flag, text16, -1, text8, size8, NULL, NULL);
   if (size8 == 0)
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   return text8;
}

/**
 * @brief Converts a UTF-8 string to a UTF-16 (wide character) string.
 *
 * @param text The null-terminated UTF-8 string to convert.
 *             Example: "€uro" (Euro symbol followed by 'uro')
 * @return A pointer to a newly allocated UTF-16 encoded string.
 *         This memory must be freed by the caller using `free()`.
 *         Returns `NULL` if the input `text` is `NULL`, if memory allocation fails,
 *         or if the conversion fails (e.g., invalid UTF-8 sequence).
 *         Example: L"\u20ACuro"
 *
 * This function uses `MultiByteToWideChar` with `CP_UTF8` and the
 * `MB_ERR_INVALID_CHARS` flag, which causes the function to fail if an invalid
 * input character is encountered.
 * If any step fails, `_evil_last_error_display` is called to log the error.
 */
EVIL_API wchar_t *
evil_utf8_to_utf16(const char *text)
{
   wchar_t *text16;
   DWORD flag = MB_ERR_INVALID_CHARS;
   int size16;

   if (!text)
     return NULL;

   size16 = MultiByteToWideChar(CP_UTF8, flag, text, -1, NULL, 0);
   if (size16 == 0)
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   text16 = malloc(size16 * sizeof(wchar_t));
   if (text16)
     if (!MultiByteToWideChar(CP_UTF8, flag, text, -1, text16, size16))
     {
        _evil_last_error_display(__func__);
        return NULL;
     }

   return text16;
}

/**
 * @brief Formats a system error code into a human-readable string.
 *
 * @param err The system error code to format (e.g., from `GetLastError()`).
 *            Example: `5L` (for ERROR_ACCESS_DENIED)
 * @return A pointer to a null-terminated string containing the formatted error message.
 *         The string is stored in a thread-local buffer, so the caller
 *         MUST NOT free it. Subsequent calls to this function or
 *         `evil_last_error_get()` in the same thread will overwrite this buffer.
 *         If `FormatMessage` fails, a message indicating this failure, along with
 *         the `GetLastError()` code from `FormatMessage`, is returned.
 *         Example return for `err = 5L`: "(    5) Access is denied.\r\n" (actual format may vary)
 *
 * This function uses the Windows API `FormatMessage` to retrieve the system's
 * textual description of the error code `err`.
 * The buffer for the message is managed using Thread Local Storage (`_evil_tls_index`),
 * ensuring thread safety for the returned string pointer.
 * If `UNICODE` is defined, the wide character message from `FormatMessage` is
 * converted to a multi-byte string using `evil_wchar_to_char`.
 */
EVIL_API const char *
evil_format_message(long err)
{
   char *buf;
   LPTSTR msg;
   char  *str;

   if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      err,
                      0, /* Default language */
                      (LPTSTR)&msg,
                      0,
                      NULL))
     {
        buf = (char *)TlsGetValue(_evil_tls_index);
        snprintf(buf, 4096,
                 "FormatMessage failed with error %ld\n", GetLastError());
        return (const char *)buf;
     }

#ifdef UNICODE
   str = evil_wchar_to_char(msg);
#else
   str = msg;
#endif /* UNICODE */

   buf = (char *)TlsGetValue(_evil_tls_index);
   snprintf(buf, 4096, "(%5ld) %s", err, str);

#ifdef UNICODE
   free(str);
#endif /* UNICODE */

   LocalFree(msg);

   return (const char *)buf;
}

/**
 * @brief Retrieves and formats the last error code set for the calling thread.
 *
 * @return A pointer to a null-terminated string containing the formatted message
 *         for the last error. This string is obtained by calling `GetLastError()`
 *         and then passing the result to `evil_format_message()`.
 *         As with `evil_format_message()`, the returned string is stored in a
 *         thread-local buffer and MUST NOT be freed by the caller. Subsequent
 *         calls in the same thread will overwrite this buffer.
 *         Example: If `GetLastError()` returns `123`, this might return
 *                  "(  123) The filename, directory name, or volume label syntax is incorrect.\r\n"
 *
 * This function provides a convenient way to get a human-readable string for
 * the error code set by the last failed WinAPI call in the current thread.
 */
EVIL_API const char *
evil_last_error_get(void)
{
   DWORD  err;

   err = GetLastError();
   return evil_format_message(err);
}

static void
_evil_last_error_display(const char *fct)
{
   fprintf(stderr, "[Evil] [%s] ERROR: %s\n", fct, evil_last_error_get());
}
