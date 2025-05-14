#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <evil_private.h> /* utf-8 and utf-16 conversion */
#include <Eina.h>

#include "Ecore_Win32.h"
#include "ecore_win32_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */



/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Sets the clipboard content.
 *
 * This function attempts to set the clipboard content for the given window.
 * It currently supports "text/" MIME types, setting both CF_TEXT (UTF-8)
 * and CF_UNICODETEXT (UTF-16) formats.
 *
 * @param window The Ecore_Win32_Window to associate with the clipboard operation.
 *               This window must be valid.
 * @param data A pointer to the data to be set on the clipboard.
 * @param size The size of the data in bytes.
 * @param mime_type The MIME type of the data. Currently, only types starting
 *                  with "text/" are supported (e.g., "text/plain").
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *         Failure can occur if the window is invalid, data is NULL, size is 0,
 *         the MIME type is unsupported, or if Win32 API calls fail.
 */
EAPI Eina_Bool
ecore_win32_clipboard_set(const Ecore_Win32_Window *window,
                          const void *data,
                          size_t size,
                          const char *mime_type)
{
   HGLOBAL global;
   char *d;
   Eina_Bool supported_mime_text;
   Eina_Bool res = EINA_FALSE;

   /*
    * See: https://msdn.microsoft.com/en-us/library/windows/desktop/ms649016%28v=vs.85%29.aspx#_win32_Copying_Information_to_the_Clipboard
    * 1. Open the clipboard
    * 2. Empty the clipboard
    * 3. Set the data
    * 4. Close the clipboard
    */

   INF("setting data to the clipboard");

   supported_mime_text = eina_str_has_prefix(mime_type, "text/");
   if (!supported_mime_text)
     {
        ERR("Mimetype %s is not handled yet", mime_type);
        return EINA_FALSE;
     }

   if (!window || !data || (size <= 0) || !OpenClipboard(window->window))
     return EINA_FALSE;

   if (!EmptyClipboard())
     goto close_clipboard;

   if (supported_mime_text)
     {
        wchar_t *text16;
        size_t size16;

        /* CF_TEXT (UTF-8) */

        global = GlobalAlloc(GMEM_MOVEABLE, size);
        if (global)
          {
             d = (char *)GlobalLock(global);
             if (d)
               {
                  memcpy(d, data, size);
                  SetClipboardData(CF_TEXT, global);
                  res = EINA_TRUE;
                  GlobalUnlock(global);
               }
          }

        /* CF_UNICODETEXT (UTF-16) */

        text16 = evil_utf8_to_utf16(data);
        if (text16)
          {
             size16 = (wcslen(text16) + 1) * sizeof(wchar_t);

             global = GlobalAlloc(GMEM_MOVEABLE, size16);
             if (global)
               {
                  d = (char *)GlobalLock(global);
                  if (d)
                    {
                       memcpy(d, text16, size16);
                       SetClipboardData(CF_UNICODETEXT, global);
                       res = EINA_TRUE;
                       GlobalUnlock(global);
                    }
               }

             free(text16);
          }
     }

 close_clipboard:
   CloseClipboard();

   return res;
}

/**
 * @brief Retrieves the clipboard content.
 *
 * This function attempts to retrieve the clipboard content associated with the
 * given window. It currently supports "text/" MIME types. It prioritizes
 * CF_UNICODETEXT (UTF-16) and converts it to UTF-8. If CF_UNICODETEXT is not
 * available, it attempts to retrieve CF_TEXT (UTF-8/ANSI).
 *
 * @param window The Ecore_Win32_Window to associate with the clipboard operation.
 *               This window must be valid.
 * @param[out] size A pointer to a size_t variable where the size of the
 *                  retrieved data (in bytes, including null terminator for text)
 *                  will be stored.
 * @param mime_type The desired MIME type of the data. Currently, only types
 *                  starting with "text/" are supported (e.g., "text/plain").
 * @return A pointer to the retrieved data on success, or @c NULL on failure.
 *         The caller is responsible for freeing the returned data using free().
 *         Failure can occur if the window is invalid, size is NULL,
 *         the MIME type is unsupported, or if Win32 API calls fail or no
 *         suitable data format is found. If NULL is returned, @p size will be 0.
 */
EAPI void *
ecore_win32_clipboard_get(const Ecore_Win32_Window *window,
                          size_t *size,
                          const char *mime_type)
{
   HGLOBAL global;
   void *data;
   void *d;

   /*
    * See https://msdn.microsoft.com/en-us/library/windows/desktop/ms649016%28v=vs.85%29.aspx#_win32_Pasting_Information_from_the_Clipboard
    * 1. Open Clipboard
    * 2. Determine format
    * 3. Retrieve data
    * 4. Manage data
    * 5. Close clipboard
    */

   INF("getting data from the clipboard");

   if (!eina_str_has_prefix(mime_type, "text/"))
     {
        ERR("Mimetype %s is not handled yet", mime_type);
        return NULL;
     }

   if (!window || !size || !OpenClipboard(window->window))
     return NULL;

   *size = 0;

#if 0
   {
     UINT fmt = 0;

     while (1)
       {
         char name[4096];
         int res;
         fmt = EnumClipboardFormats(fmt);
         res = GetClipboardFormatName(fmt, name, sizeof(name));
         fprintf(stderr, " $ Format2 : %x %d\n", fmt, res);
         if (res)
           fprintf(stderr, " $ Format2 : %s\n", name);
         else
           fprintf(stderr, " $ Format2 : error %ld\n", GetLastError());
         fflush(stderr);
         if (!fmt)
           break;
       }
   }
#endif

   if (eina_str_has_prefix(mime_type, "text/"))
     {
        /* first check if UTF-16 text is available */
        global = GetClipboardData(CF_UNICODETEXT);
        if (global)
          {
             d = GlobalLock(global);
             if (d)
               {
                  data = evil_utf16_to_utf8(d);
                  if (data)
                    {
                       *size = strlen(data);
                       GlobalUnlock(global);
                       CloseClipboard();
                       return data;
                    }
                  GlobalUnlock(global);
               }
             /* otherwise, we try CF_TEXT (UTF-8/ANSI) */
          }

        /* second, check if UTF-8/ANSI text is available */
        global = GetClipboardData(CF_TEXT);
        if (global)
          {
             d = GlobalLock(global);
             if (d)
               {
                  *size = strlen(d) + 1;
                  data = malloc(*size);
                  if (data)
                    {
                       memcpy(data, d, *size);
                       GlobalUnlock(global);
                       CloseClipboard();
                       return data;
                    }
                  else
                    *size = 0;

                  GlobalUnlock(global);
               }
          }
     }

   CloseClipboard();

   return NULL;
}

/**
 * @brief Clears the clipboard content.
 *
 * This function attempts to clear the clipboard content.
 *
 * @param window The Ecore_Win32_Window to associate with the clipboard operation.
 *               This window must be valid for the OpenClipboard call.
 */
EAPI void
ecore_win32_clipboard_clear(const Ecore_Win32_Window *window)
{
   INF("clearing the clipboard");

   if (!window || !OpenClipboard(window->window))
     return;

   EmptyClipboard();
   CloseClipboard();
}

/**
 * @}
 */
