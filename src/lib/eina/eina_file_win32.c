/* EINA - EFL data type library
 * Copyright (C) 2010 Vincent Torri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <evil_private.h>
#include <fcntl.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_alloca.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_file.h"
#include "eina_stringshare.h"
#include "eina_hash.h"
#include "eina_list.h"
#include "eina_lock.h"
#include "eina_log.h"
#include "eina_file_common.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

#ifdef MAP_FAILED
# undef MAP_FAILED
#endif
#define MAP_FAILED ((void *)-1)

typedef struct _Eina_File_Iterator        Eina_File_Iterator;
typedef struct _Eina_File_Direct_Iterator Eina_File_Direct_Iterator;
typedef struct _Eina_File_Map             Eina_File_Map;

/**
 * @brief Structure for iterating over files in a directory.
 * This structure holds the state for iterating through directory entries
 * using FindFirstFileEx/FindNextFile.
 */
struct _Eina_File_Iterator
{
   Eina_Iterator   iterator; /**< Eina_Iterator base structure. */

   WIN32_FIND_DATA data;     /**< Data for the current file found. */
   HANDLE          handle;   /**< Handle for the directory search. */
   size_t          length;   /**< Length of the directory path. */
   Eina_Bool       is_last : 1; /**< Flag indicating if the last file has been processed. */

   char            dir[1];   /**< Flexible array member for the directory path. */
};

/**
 * @brief Structure for iterating over files with direct information.
 * Similar to Eina_File_Iterator, but also provides Eina_File_Direct_Info
 * for each file.
 */
struct _Eina_File_Direct_Iterator
{
   Eina_Iterator         iterator; /**< Eina_Iterator base structure. */

   WIN32_FIND_DATA       data;     /**< Data for the current file found. */
   HANDLE                handle;   /**< Handle for the directory search. */
   size_t                length;   /**< Length of the directory path. */
   Eina_Bool             is_last : 1; /**< Flag indicating if the last file has been processed. */

   Eina_File_Direct_Info info;     /**< Detailed information about the current file. */

   char                  dir[1];   /**< Flexible array member for the directory path. */
};

int _eina_file_log_dom = -1;

/**
 * @brief Checks if the given path is a directory on Windows.
 * @param dir The path to check.
 * @return EINA_TRUE if the path is a directory, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_win32_is_dir(const char *dir)
{
#ifdef UNICODE
   wchar_t *wdir = NULL;
#endif
   DWORD    attr;

   /* check if it's a directory */
#ifdef UNICODE
   wdir = evil_char_to_wchar(dir);
   if (!wdir)
     return EINA_FALSE;

   attr = GetFileAttributes(wdir);
   free(wdir);
#else
   attr = GetFileAttributes(dir);
#endif

   if (attr == INVALID_FILE_ATTRIBUTES)
     return EINA_FALSE;

   if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Initiates a file search in the specified directory.
 * This function calls FindFirstFileEx to find the first file matching "\\*.*"
 * in the given directory, skipping "." and ".." entries.
 * @param dir The directory path to search in.
 * @param fd Pointer to a WIN32_FIND_DATA structure to receive information
 *           about the first file found.
 * @return A search handle if successful, otherwise INVALID_HANDLE_VALUE.
 */
static HANDLE
_eina_file_win32_first_file(const char *dir, WIN32_FIND_DATA *fd)
{
   char buf[4096];
   HANDLE h;
   size_t l = strlen(dir);
#ifdef UNICODE
   wchar_t *wdir = NULL;
#endif

   l = strlen(dir);
   if ((l + 5) > sizeof(buf))
     return INVALID_HANDLE_VALUE;

   memcpy(buf, dir, l);
   memcpy(buf + l, "\\*.*", 5);

#ifdef UNICODE
   wdir = evil_char_to_wchar(buf);
   if (!wdir)
     return INVALID_HANDLE_VALUE;

   h = FindFirstFileEx(wdir, FindExInfoBasic, fd, FindExSearchNameMatch, NULL, 0);
   free(wdir);
#else
   h = FindFirstFileEx(buf, FindExInfoBasic, fd, FindExSearchNameMatch, NULL, 0);
#endif

   if (!h)
     return INVALID_HANDLE_VALUE;

   while ((fd->cFileName[0] == '.') &&
          ((fd->cFileName[1] == '\0') ||
           ((fd->cFileName[1] == '.') && (fd->cFileName[2] == '\0'))))
     {
        if (!FindNextFile(h, fd))
          {
             FindClose(h);
             return INVALID_HANDLE_VALUE;
          }
     }

   return h;
}

/**
 * @brief Advances the directory iterator to the next file.
 * This function is the 'next' callback for Eina_File_Iterator. It retrieves
 * the next file in the directory listing, constructs its full path, and
 * returns it as a stringshare.
 * @param it The file iterator.
 * @param data Pointer to store the stringshared full path of the next file.
 * @return EINA_TRUE if a next file was found, EINA_FALSE otherwise or on error.
 */
static Eina_Bool
_eina_file_win32_ls_iterator_next(Eina_File_Iterator *it, void **data)
{
#ifdef UNICODE
   wchar_t  *old_name;
#else
   char     *old_name;
#endif
   char     *name;
   char     *cname;
   size_t    length;
   Eina_Bool is_last;
   Eina_Bool res = EINA_TRUE;

   if (it->handle == INVALID_HANDLE_VALUE)
     {
        if (GetLastError() == ERROR_NO_MORE_FILES)
          it->is_last = EINA_TRUE;
        return EINA_FALSE;
     }

   is_last = it->is_last;
#ifdef UNICODE
   old_name = _wcsdup(it->data.cFileName);
#else
   old_name = _strdup(it->data.cFileName);
#endif
   if (!old_name)
     return EINA_FALSE;

   do {
      if (!FindNextFile(it->handle, &it->data))
        {
           if (GetLastError() == ERROR_NO_MORE_FILES)
             it->is_last = EINA_TRUE;
           else
             res = EINA_FALSE;
        }
   } while ((it->data.cFileName[0] == '.') &&
            ((it->data.cFileName[1] == '\0') ||
             ((it->data.cFileName[1] == '.') && (it->data.cFileName[2] == '\0')))); /* FIXME: what about UNICODE ? */

#ifdef UNICODE
   cname = evil_wchar_to_char(old_name);
   if (!cname)
     return EINA_FALSE;
#else
     cname = old_name;
#endif

   length = strlen(cname);
   name = alloca(length + 2 + it->length);

   memcpy(name,                  it->dir, it->length);
   memcpy(name + it->length,     "\\",    1);
   memcpy(name + it->length + 1, cname,   length + 1);

   *data = (char *)eina_stringshare_add(name);

#ifdef UNICODE
   free(cname);
#endif
   free(old_name);

   if (is_last)
     res = EINA_FALSE;

   return res;
}

/**
 * @brief Gets the container (search handle) of the file iterator.
 * This function is the 'get_container' callback for Eina_File_Iterator.
 * @param it The file iterator.
 * @return The search handle (HANDLE) used by the iterator.
 */
static HANDLE
_eina_file_win32_ls_iterator_container(Eina_File_Iterator *it)
{
   return it->handle;
}

/**
 * @brief Frees the resources used by a file iterator.
 * This function is the 'free' callback for Eina_File_Iterator. It closes
 * the search handle and frees the iterator structure.
 * @param it The file iterator to free.
 */
static void
_eina_file_win32_ls_iterator_free(Eina_File_Iterator *it)
{
   if (it->handle != INVALID_HANDLE_VALUE)
     FindClose(it->handle);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
 * @brief Advances the direct directory iterator to the next file.
 * This function is the 'next' callback for Eina_File_Direct_Iterator.
 * It retrieves the next file, populates an Eina_File_Direct_Info structure
 * with its details (path, name, type), and returns a pointer to this structure.
 * @param it The direct file iterator.
 * @param data Pointer to store the address of the Eina_File_Direct_Info structure.
 * @return EINA_TRUE if a next file was found, EINA_FALSE otherwise or on error.
 */
static Eina_Bool
_eina_file_win32_direct_ls_iterator_next(Eina_File_Direct_Iterator *it, void **data)
{
#ifdef UNICODE
   wchar_t  *old_name;
#else
   char     *old_name;
#endif
   char     *cname;
   size_t    length;
   DWORD     attr;
   Eina_Bool is_last;
   Eina_Bool res = EINA_TRUE;

   if (it->handle == INVALID_HANDLE_VALUE)
     {
        if (GetLastError() == ERROR_NO_MORE_FILES)
          it->is_last = EINA_TRUE;
        return EINA_FALSE;
     }

   attr = it->data.dwFileAttributes;
   is_last = it->is_last;
#ifdef UNICODE
   old_name = _wcsdup(it->data.cFileName);
#else
   old_name = _strdup(it->data.cFileName);
#endif
   if (!old_name)
     return EINA_FALSE;

   do {
      if (!FindNextFile(it->handle, &it->data))
        {
           if (GetLastError() == ERROR_NO_MORE_FILES)
             it->is_last = EINA_TRUE;
           else
             res = EINA_FALSE;
        }

#ifdef UNICODE
     length = wcslen(old_name);
#else
     length = strlen(old_name);
#endif
     if (it->info.name_start + length + 1 >= PATH_MAX)
       {
          free(old_name);
#ifdef UNICODE
          old_name = _wcsdup(it->data.cFileName);
#else
          old_name = _strdup(it->data.cFileName);
#endif
          continue;
       }

   } while ((it->data.cFileName[0] == '.') &&
            ((it->data.cFileName[1] == '\0') ||
             ((it->data.cFileName[1] == '.') && (it->data.cFileName[2] == '\0')))); /* FIXME: what about UNICODE ? */

#ifdef UNICODE
   cname = evil_wchar_to_char(old_name);
   if (!cname)
     return EINA_FALSE;
#else
     cname = old_name;
#endif

   memcpy(it->info.path + it->info.name_start, cname, length);
   it->info.name_length = length;
   it->info.path_length = it->info.name_start + length;
   it->info.path[it->info.path_length] = '\0';

   if (attr & FILE_ATTRIBUTE_DIRECTORY)
     it->info.type = EINA_FILE_DIR;
   else if (attr & (FILE_ATTRIBUTE_ARCHIVE |
                    FILE_ATTRIBUTE_COMPRESSED |
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_NORMAL |
                    FILE_ATTRIBUTE_SPARSE_FILE |
                    FILE_ATTRIBUTE_TEMPORARY |
                    FILE_ATTRIBUTE_REPARSE_POINT))
     it->info.type = EINA_FILE_REG;
   else
     it->info.type = EINA_FILE_UNKNOWN;

   *data = &it->info;

#ifdef UNICODE
   free(cname);
#endif

   free(old_name);

   if (is_last)
     res = EINA_FALSE;

   return res;
}

/**
 * @brief Gets the container (search handle) of the direct file iterator.
 * This function is the 'get_container' callback for Eina_File_Direct_Iterator.
 * @param it The direct file iterator.
 * @return The search handle (HANDLE) used by the iterator.
 */
static HANDLE
_eina_file_win32_direct_ls_iterator_container(Eina_File_Direct_Iterator *it)
{
   return it->handle;
}

/**
 * @brief Frees the resources used by a direct file iterator.
 * This function is the 'free' callback for Eina_File_Direct_Iterator.
 * It closes the search handle and frees the iterator structure.
 * @param it The direct file iterator to free.
 */
static void
_eina_file_win32_direct_ls_iterator_free(Eina_File_Direct_Iterator *it)
{
   if (it->handle != INVALID_HANDLE_VALUE)
     FindClose(it->handle);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
 * @internal
 * @brief Actually closes an Eina_File, unmapping views and closing handles.
 * This function is responsible for the low-level cleanup of an Eina_File
 * structure, including unmapping any memory-mapped views and closing the
 * underlying file handle. It's typically called when the refcount of an
 * Eina_File drops to zero or when a file is explicitly flushed due to changes.
 * @param file The Eina_File to close.
 */
void
eina_file_real_close(Eina_File *file)
{
   Eina_File_Map *map;

   EINA_LIST_FREE(file->dead_map, map)
     {
        UnmapViewOfFile(map->map);
        free(map);
     }

   if (file->handle != INVALID_HANDLE_VALUE)
     {
        if (!file->copied && file->global_map != MAP_FAILED)
          UnmapViewOfFile(file->global_map);
        CloseHandle(file->handle);
     }
}

/**
 * @brief Closes a specific memory map view and frees the Eina_File_Map structure.
 * This is a helper function, often used as a callback for hash table freeing.
 * @param map The Eina_File_Map to close and free.
 */
static void
_eina_file_map_close(Eina_File_Map *map)
{
   if (map->map != MAP_FAILED)
     UnmapViewOfFile(map->map);
   free(map);
}

/**
 * @brief Finds the first path separator ('\\' or '/') in a string.
 * @param s The string to search.
 * @return A pointer to the first separator found, or NULL if no separator is present.
 */
static char *
_eina_file_sep_find(char *s)
{
   for (; *s != '\0'; ++s)
     if ((*s == '\\') || (*s == '/'))
       return s;

   return NULL;
}

/**
 * @brief Generates a random alphanumeric character (lowercase or digit).
 * This function is used to create random components for temporary filenames.
 * It ensures the character is one of 'a'-'z' or '0'-'9'.
 * @param c Pointer to an unsigned char to store the generated character.
 *          The initial value of *c is used as input for BCryptGenRandom.
 * @return The generated random character.
 */
static unsigned char _eina_file_random_uchar(unsigned char *c)
{
  /*
   * Helper function for mktemp.
   *
   * Only characters from 'a' to 'z' and '0' to '9' are considered
   * because on Windows, file system is case insensitive. That means
   * 36 possible values.
   * To increase randomness, we consider the greatest multiple of 36
   * within 255 : 7*36 = 252, that is, values from 0 to 251 and choose
   * a random value in this interval.
   */
  do {
    BCryptGenRandom(_eina_bcrypt_provider, c, sizeof(UCHAR), 0);
  } while (*c > 251);

  *c = '0' + *c % 36;
  if (*c > '9')
    *c += 'a' - '9' - 1;

  return *c;
}

/**
 * @brief Initializes and validates a template string for mkstemp-like functions.
 * Checks if the template is valid (not NULL, sufficient length, ends with "XXXXXX"
 * before any suffix).
 * @param __template The template string (e.g., "/tmp/fileXXXXXX.tmp").
 * @param[out] length Pointer to store the length of the template string.
 * @param suffixlen The length of the suffix part of the template (if any).
 * @return 1 if the template is valid, 0 otherwise (sets errno to EINVAL).
 */
static int
_eina_file_mkstemp_init(char *__template, size_t *length, int suffixlen)
{
  if (!__template || (suffixlen < 0))
     {
        errno = EINVAL;
        return 0;
     }

  *length = strlen(__template);
  if ((*length < (6 + (size_t)suffixlen))
       || (strncmp(__template + *length - 6 - suffixlen, "XXXXXX", 6) != 0))
     {
        errno = EINVAL;
        return 0;
     }
   return 1;
}

/**
 * @brief Replaces the "XXXXXX" part of a template string with random characters.
 * Modifies the template string in-place.
 * @param __template The template string to modify.
 * @param length The total length of the template string.
 * @param suffixlen The length of the suffix part, following "XXXXXX".
 */
static void
_eina_file_tmpname(char *__template, size_t length, int suffixlen)
{
   unsigned char *suffix;

   suffix = (unsigned char *)(__template + length - 6 - suffixlen);
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
   *suffix = _eina_file_random_uchar(suffix);
   suffix++;
}

/**
 * @brief Creates and opens a unique temporary file with a specified suffix.
 * This function attempts to create a unique filename by replacing "XXXXXX"
 * in the template with random characters, then opens the file.
 * @param __template The template for the filename. The "XXXXXX" portion will be
 *                   modified. Example: "mytempXXXXXX.log".
 * @param suffixlen The length of the suffix part of the template that follows "XXXXXX".
 *                  For "mytempXXXXXX.log", suffixlen would be 4 (".log").
 * @return A file descriptor if successful, -1 on error (sets errno).
 */
static int
_eina_file_mkstemps(char *__template, int suffixlen)
{
   size_t length;
   int i;

   if (!_eina_file_mkstemp_init(__template, &length, suffixlen))
     return -1;

   for (i = 0; i < 32768; i++)
     {
        int fd;

        _eina_file_tmpname(__template, length, suffixlen);

        fd = _open(__template,
                   _O_RDWR | _O_BINARY | _O_CREAT | _O_EXCL,
                   _S_IREAD | _S_IWRITE);
        if (fd >= 0)
          return fd;
     }

   errno = EEXIST;
   return -1;
}

/**
 * @brief Creates a unique temporary directory.
 * This function attempts to create a unique directory name by replacing "XXXXXX"
 * in the template with random characters.
 * @param __template The template for the directory name. The "XXXXXX" portion
 *                   will be modified. Example: "/tmp/mydirXXXXXX".
 * @return A pointer to the modified template string (the created directory name)
 *         if successful, NULL on error.
 */
static char *
_eina_file_mkdtemp(char *__template)
{
   size_t length;
   int i;

   if (!_eina_file_mkstemp_init(__template, &length, 0))
     return NULL;

   for (i = 0; i < 32768; i++)
     {
        _eina_file_tmpname(__template, length, 0);
        if (CreateDirectory(__template, NULL) == TRUE)
          return __template;
     }

   return NULL;
}


/**
 * @endcond
 */


/* ================================================================ *
 *   Simplified logic for portability layer with eina_file_common   *
 * ================================================================ */

/**
 * @brief Prepends the current working directory to a given relative path.
 *
 * This function retrieves the current working directory, then appends the
 * provided @p path to it, separated by a backslash. The resulting
 * path is returned as an Eina_Tmpstr.
 *
 * @param path The relative path to append to the current directory.
 * @param len The length of the @p path string.
 * @return An Eina_Tmpstr containing the absolute path, or NULL on failure.
 *         The caller is responsible for deleting the Eina_Tmpstr using
 *         eina_tmpstr_del().
 * @note This function is part of the portability layer with eina_file_common.
 */
Eina_Tmpstr *
eina_file_current_directory_get(const char *path, size_t len)
{
   char *tmp;
   DWORD l;

   l = GetCurrentDirectory(0, NULL);
   if (l == 0) return NULL;

   tmp = alloca(sizeof (char) * (l + len + 2));
   l = GetCurrentDirectory(l + 1, tmp);
   tmp[l] = '\\';
   memcpy(tmp + l + 1, path, len);
   tmp[l + len + 1] = '\0';

   return eina_tmpstr_add_length(tmp, l + len + 1);
}

/**
 * @brief Cleans up a path string obtained from Eina_Tmpstr.
 *
 * This function duplicates the string from an Eina_Tmpstr, deletes the
 * Eina_Tmpstr, and then converts all backslashes in the duplicated
 * string to forward slashes (Unix-style path).
 *
 * @param path The Eina_Tmpstr containing the path to clean up.
 *             This Eina_Tmpstr will be deleted by the function.
 * @return A newly allocated string with cleaned path, or NULL on allocation failure.
 *         The caller is responsible for freeing the returned string.
 *         Returns an empty string "" if the input path was NULL.
 * @note This function is part of the portability layer with eina_file_common.
 */
char *
eina_file_cleanup(Eina_Tmpstr *path)
{
   char *result;

   result = strdup(path ? path : "");
   eina_tmpstr_del(path);

   if (!result)
     return NULL;

   EINA_PATH_TO_UNIX(result);

   return result;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Determines if a given path is relative on Windows.
 *
 * A path is considered absolute on Windows if:
 * - It is a UNC path (starts with "\\").
 * - It starts with a drive letter followed by a colon and a separator (e.g., "C:\\" or "C:/").
 * - It starts with a single backslash "\" (server-relative path).
 *
 * All other paths are considered relative.
 *
 * @param path The path string to check.
 * @return EINA_TRUE if the path is relative, EINA_FALSE if it is absolute or NULL/empty.
 */
EINA_API Eina_Bool
eina_file_path_relative(const char *path)
{
   /* see
    * https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file#paths
    * absolute path if:
    * - is an UNC path (begins with \\)
    * - has a drive letter (C:\). \ is important here, otherwise it is relative
    * - begins with \
    */

   if (!path || *path == '\\') // Handles NULL, empty, UNC, or root paths like \foo
     return EINA_FALSE;

   // Check for drive letter paths like C:\foo or C:/foo
   if ((((*path >= 'a') && (*path <= 'z')) ||
        ((*path >= 'A') && (*path <= 'Z'))) && // First char is a letter
       (path[1] == ':') && // Second char is a colon
       ((path[2] == '\\') || (path[2] == '/'))) // Third char is a path separator
     return EINA_FALSE;

   return EINA_TRUE; // Otherwise, it's relative
}

/**
 * @brief Lists the contents of a directory.
 *
 * This function iterates over the files and subdirectories within the given
 * directory. For each entry (excluding "." and ".."), it calls the provided
 * callback function. If recursion is enabled, it will also list the contents
 * of subdirectories.
 *
 * @param dir The path to the directory to list.
 * @param recursive If EINA_TRUE, recursively list subdirectories.
 * @param cb The callback function to be called for each entry.
 *           The callback receives the filename (not full path), the directory
 *           it's in, and the user-provided data.
 * @param data User-specific data to pass to the callback function.
 * @return EINA_TRUE if the directory was listed successfully, EINA_FALSE otherwise
 *         (e.g., if @p dir is not a directory, is NULL/empty, or @p cb is NULL).
 */
EINA_API Eina_Bool
eina_file_dir_list(const char *dir,
                   Eina_Bool recursive,
                   Eina_File_Dir_List_Cb cb,
                   void *data)
{
   WIN32_FIND_DATA file;
   HANDLE h;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cb,  EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(dir[0] == '\0', EINA_FALSE);

   if (!_eina_file_win32_is_dir(dir))
     return EINA_FALSE;

   h = _eina_file_win32_first_file(dir, &file);

   if (h == INVALID_HANDLE_VALUE)
      return EINA_FALSE;

   do
     {
        char *filename;

# ifdef UNICODE
        filename = evil_wchar_to_char(file.cFileName);
# else
        filename = file.cFileName;
# endif /* ! UNICODE */
        if ((filename[0] == '.') &&
            ((filename[1] == '\0') ||
             ((filename[1] == '.') && (filename[2] == '\0'))))
           continue;

        cb(filename, dir, data);

        if (recursive == EINA_TRUE)
          {
             char *path;

             path = alloca(strlen(dir) + strlen(filename) + 2);
             strcpy(path, dir);
             strcat(path, "/");
             strcat(path, filename);

             if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;

             eina_file_dir_list(path, recursive, cb, data);
          }

# ifdef UNICODE
        free(filename);
# endif /* UNICODE */

     } while (FindNextFile(h, &file));
   FindClose(h);

   return EINA_TRUE;
}

/**
 * @brief Splits a file path into its components.
 *
 * This function takes a file path string and splits it into an array of
 * strings, where each string is a component of the path. The path is
 * split by directory separators ('\\' or '/').
 * The input @p path string is modified in place by inserting null terminators
 * at separator locations. The strings pushed into the array are pointers
 * into this modified @p path string.
 *
 * @param path The file path string to split. This string will be modified.
 *             Example: "C:\\foo\\bar\\file.txt" or "/usr/local/bin".
 * @return A new Eina_Array containing char* pointers to the path components,
 *         or NULL if @p path is NULL or memory allocation fails.
 *         The caller is responsible for freeing the Eina_Array using eina_array_free().
 *         The strings within the array point to parts of the original @p path string
 *         and should not be freed individually if @p path is managed elsewhere.
 *
 * @par Example:
 * @code
 * char path_str[] = "C:\\Users\\MyUser\\Documents\\file.txt";
 * Eina_Array *components = eina_file_split(path_str);
 * if (components) {
 *     // components will contain: ["C:", "Users", "MyUser", "Documents", "file.txt"]
 *     // path_str will be modified to: "C:\0Users\0MyUser\0Documents\0file.txt"
 *     // ... use components ...
 *     eina_array_free(components);
 * }
 * @endcode
 */
EINA_API Eina_Array *
eina_file_split(char *path)
{
   Eina_Array *ea;
   char *current;
   size_t length;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   ea = eina_array_new(16);

   if (!ea)
      return NULL;

   // Iterate through the path, finding separators
   for (current = _eina_file_sep_find(path); // Find the next separator
        current; // Continue if a separator was found
        path = current + 1, current = _eina_file_sep_find(path)) // Advance path past separator for next iteration
     {
        length = current - path; // Length of the component

        if (length == 0) // Skip empty components (e.g., from "//" or "\\")
           continue;

        eina_array_push(ea, path); // Push the start of the component
        *current = '\0'; // Null-terminate the component in the original string
     }

   // Add the last component if it's not empty
   if (*path != '\0')
        eina_array_push(ea, path);

   return ea;
}

/**
 * @brief Creates an iterator to list files in a directory.
 *
 * This function initializes an iterator that can be used to step through
 * the files and directories within the specified @p dir. The iterator
 * returns the full path of each item as a stringshared string.
 * It skips "." and ".." entries.
 *
 * @param dir The directory path to list.
 * @return A pointer to an Eina_Iterator if successful, NULL otherwise.
 *         The caller is responsible for freeing the iterator using eina_iterator_free().
 *         Returns NULL if @p dir is NULL, empty, not a directory, or on internal error.
 */
EINA_API Eina_Iterator *
eina_file_ls(const char *dir)
{
   Eina_File_Iterator *it;
   size_t              length;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);

   if (!dir || !*dir)
      return NULL;

   if (!_eina_file_win32_is_dir(dir))
     return NULL;

   length = strlen(dir);

   it = calloc(1, sizeof (Eina_File_Iterator) + length);
   if (!it)
      return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->handle = _eina_file_win32_first_file(dir, &it->data);
   if ((it->handle == INVALID_HANDLE_VALUE) && (GetLastError() != ERROR_NO_MORE_FILES))
     goto free_it;

   memcpy(it->dir, dir, length + 1);
   if ((dir[length - 1] != '\\') && (dir[length - 1] != '/'))
      it->length = length;
   else
      it->length = length - 1;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_win32_ls_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_eina_file_win32_ls_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_win32_ls_iterator_free);

   return &it->iterator;

 free_it:
   free(it);

   return NULL;
}

/**
 * @brief Creates an iterator to list files in a directory with direct file information.
 *
 * This function initializes an iterator that provides more detailed information
 * for each file/directory entry, encapsulated in an Eina_File_Direct_Info structure.
 * This structure includes the full path, filename, and type (file/directory).
 * It skips "." and ".." entries.
 *
 * @param dir The directory path to list.
 * @return A pointer to an Eina_Iterator if successful, NULL otherwise.
 *         The iterator returns pointers to an internal Eina_File_Direct_Info
 *         structure, which is valid until the next call to eina_iterator_next()
 *         or eina_iterator_free().
 *         The caller is responsible for freeing the iterator using eina_iterator_free().
 *         Returns NULL if @p dir is NULL, empty, not a directory, path is too long,
 *         or on internal error.
 */
EINA_API Eina_Iterator *
eina_file_direct_ls(const char *dir)
{
   Eina_File_Direct_Iterator *it;
   size_t                     length;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);

   if (!dir || !*dir)
      return NULL;

   length = strlen(dir);

   if (length + 12 + 2 >= MAX_PATH)
      return NULL;

   it = calloc(1, sizeof(Eina_File_Direct_Iterator) + length);
   if (!it)
      return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->handle = _eina_file_win32_first_file(dir, &it->data);
   if ((it->handle == INVALID_HANDLE_VALUE) && (GetLastError() != ERROR_NO_MORE_FILES))
     goto free_it;

   memcpy(it->dir, dir, length + 1);
   it->length = length;

   memcpy(it->info.path, dir, length);
   if ((dir[length - 1] == '\\') || (dir[length - 1] == '/'))
      it->info.name_start = length;
   else
     {
        it->info.path[length] = '\\';
        it->info.name_start = length + 1;
     }

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_win32_direct_ls_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_eina_file_win32_direct_ls_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_win32_direct_ls_iterator_free);

   return &it->iterator;

 free_it:
   free(it);

   return NULL;
}

/**
 * @brief Creates an iterator to list files with stat-like information (alias for eina_file_direct_ls).
 *
 * This is an alias for eina_file_direct_ls(). It provides an iterator that
 * yields Eina_File_Direct_Info structures for each entry in the directory.
 *
 * @param dir The directory path to list.
 * @return A pointer to an Eina_Iterator if successful, NULL otherwise.
 * @see eina_file_direct_ls
 */
EINA_API Eina_Iterator *
eina_file_stat_ls(const char *dir)
{
   return eina_file_direct_ls(dir);
}

/**
 * @brief Refreshes the cached information (size and modification time) of an opened file.
 *
 * This function queries the operating system for the current size and last
 * modification time of the file associated with the Eina_File handle.
 * If the size has changed, it calls eina_file_flush() to invalidate
 * existing memory maps.
 *
 * @param file The Eina_File handle to refresh. Must not be a virtual file.
 * @return EINA_TRUE if the file's size changed (and thus was flushed),
 *         EINA_FALSE otherwise or on error (e.g., file is NULL, virtual, or
 *         GetFileAttributesEx fails).
 */
EINA_API Eina_Bool
eina_file_refresh(Eina_File *file)
{
   WIN32_FILE_ATTRIBUTE_DATA fad;
   ULARGE_INTEGER length;
   ULARGE_INTEGER mtime;
   Eina_Bool r = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);

   if (file->virtual) return EINA_FALSE;

   if (!GetFileAttributesEx(file->filename, GetFileExInfoStandard, &fad))
     return EINA_FALSE;

   length.u.LowPart = fad.nFileSizeLow;
   length.u.HighPart = fad.nFileSizeHigh;
   mtime.u.LowPart = fad.ftLastWriteTime.dwLowDateTime;
   mtime.u.HighPart = fad.ftLastWriteTime.dwHighDateTime;

   if (file->length != length.QuadPart)
     {
        eina_file_flush(file, length.QuadPart);
        r = EINA_TRUE;
     }

   file->length = length.QuadPart;
   file->mtime = mtime.QuadPart;

   return r;
}

/**
 * @brief Opens a file and returns an Eina_File handle.
 *
 * This function opens the specified file for reading. It uses a cache to
 * potentially return an existing Eina_File handle if the file is already open
 * and its metadata (mtime, size) hasn't changed.
 * The filename is sanitized using eina_file_sanitize() before use.
 *
 * @param path The path to the file to open.
 * @param shared If EINA_TRUE, the file is opened with stricter sharing modes
 *               (GENERIC_READ, FILE_SHARE_READ). If EINA_FALSE (typical use),
 *               it's opened with GENERIC_READ and FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE.
 *               Note: The current implementation for Windows seems to always use the latter
 *               sharing mode regardless of the `shared` parameter due to a commented out `#if 0` block.
 * @return A pointer to an Eina_File structure if successful, NULL otherwise.
 *         The caller is responsible for closing the file using eina_file_close()
 *         when it's no longer needed. This decrements a reference counter.
 */
EINA_API Eina_File *
eina_file_open(const char *path, Eina_Bool shared)
{
   Eina_File *file;
   Eina_File *n;
   Eina_Stringshare *filename;
   HANDLE handle;
   WIN32_FILE_ATTRIBUTE_DATA fad;
   ULARGE_INTEGER length;
   ULARGE_INTEGER mtime;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   filename = eina_file_sanitize(path);
   if (!filename) return NULL;

   /* FIXME: how to emulate shm_open ? Just OpenFileMapping ? */
#if 0
   if (shared)
     handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY,
                         NULL);
   else
#endif
     handle = CreateFile(filename,
                         GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                         NULL);

   if (handle == INVALID_HANDLE_VALUE)
     {
       errno = GetLastError();
       WRN("eina_file_open() failed with file %s: %s",
           filename, evil_format_message(errno));
       goto free_file;
     }

   if (!GetFileAttributesEx(filename, GetFileExInfoStandard, &fad))
     {
        errno = GetLastError();
        goto close_handle;
     }

   length.u.LowPart = fad.nFileSizeLow;
   length.u.HighPart = fad.nFileSizeHigh;
   mtime.u.LowPart = fad.ftLastWriteTime.dwLowDateTime;
   mtime.u.HighPart = fad.ftLastWriteTime.dwHighDateTime;

   eina_lock_take(&_eina_file_lock_cache);

   file = eina_hash_find(_eina_file_cache, filename);
   if (file &&
       (file->mtime != mtime.QuadPart || file->length != length.QuadPart))
     {
        file->delete_me = EINA_TRUE;
        eina_hash_del(_eina_file_cache, file->filename, file);
        file = NULL;
     }

   if (!file)
     {
        n = malloc(sizeof(Eina_File));
        if (!n)
          {
             eina_lock_release(&_eina_file_lock_cache);
             goto close_handle;
          }

        memset(n, 0, sizeof(Eina_File));
        n->filename = filename;
        n->map = eina_hash_new(EINA_KEY_LENGTH(eina_file_map_key_length),
                               EINA_KEY_CMP(eina_file_map_key_cmp),
                               EINA_KEY_HASH(eina_file_map_key_hash),
                               EINA_FREE_CB(_eina_file_map_close),
                               3);
        n->rmap = eina_hash_pointer_new(NULL);
        n->global_map = MAP_FAILED;
        n->length = length.QuadPart;
        n->mtime = mtime.QuadPart;
        n->handle = handle;
        n->shared = shared;
        eina_lock_new(&n->lock);
        eina_hash_direct_add(_eina_file_cache, n->filename, n);

        EINA_MAGIC_SET(n, EINA_FILE_MAGIC);
     }
   else
     {
        CloseHandle(handle);

        n = file;
     }
   eina_lock_take(&n->lock);
   n->refcount++;
   eina_lock_release(&n->lock);

   eina_lock_release(&_eina_file_lock_cache);

   return n;

 close_handle:
   CloseHandle(handle);
 free_file:
   WRN("Could not open file [%s].", filename);
   eina_stringshare_del(filename);

   return NULL;
}

/**
 * @brief Deletes a file (unlinks it).
 *
 * This function attempts to delete the specified file. If the file is currently
 * open and cached by the Eina_File system, it tries a Windows-specific trick
 * using `FILE_FLAG_DELETE_ON_CLOSE` to mark the file for deletion when its
 * handle is closed. Otherwise, it falls back to the standard `unlink()` call.
 *
 * @param pathname The path to the file to delete. The path is sanitized
 *                 using eina_file_sanitize().
 * @return EINA_TRUE if the file was successfully deleted or marked for deletion,
 *         EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_file_unlink(const char *pathname)
{
   Eina_Stringshare *unlink_path = eina_file_sanitize(pathname);
   Eina_File *file = eina_hash_find(_eina_file_cache, unlink_path);
   Eina_Bool r = EINA_FALSE;

   if (file)
     {
        if (file->handle != INVALID_HANDLE_VALUE)
          {
             CloseHandle(file->handle);

             file->handle = CreateFile(unlink_path,
                                       GENERIC_READ,
                                       FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                                       NULL);

            if (file->handle != INVALID_HANDLE_VALUE)
              {
                 CloseHandle(file->handle);
                 file->handle = INVALID_HANDLE_VALUE;
                 r = EINA_TRUE;
                 goto finish;
              }
          }
     }

   if (unlink(unlink_path) >= 0) r = EINA_TRUE;
   eina_stringshare_del(unlink_path);

 finish:
   return r;
}


/**
 * @brief Get an iterator for extended attributes of a file.
 * @param file The Eina_File handle.
 * @return Always NULL on Windows as extended attributes are not natively supported
 *         in the same way as on POSIX systems.
 * @note This function is a stub on Windows.
 */
EINA_API Eina_Iterator *eina_file_xattr_get(Eina_File *file EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Get an iterator for the values of extended attributes of a file.
 * @param file The Eina_File handle.
 * @return Always NULL on Windows.
 * @note This function is a stub on Windows.
 */
EINA_API Eina_Iterator *eina_file_xattr_value_get(Eina_File *file EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Populates a memory-mapped region of a file.
 * @param file The Eina_File handle.
 * @param rule The population rule (e.g., EINA_FILE_POPULATE_NONE, EINA_FILE_POPULATE_RANDOM).
 * @param map Pointer to the start of the memory-mapped region.
 * @param offset Offset within the file where the mapped region begins.
 * @param length Length of the mapped region.
 * @note This function is a stub on Windows and does nothing. The `rule` parameter
 *       is typically used on systems supporting madvise/posix_fadvise.
 */
EINA_API void
eina_file_map_populate(Eina_File *file EINA_UNUSED, Eina_File_Populate rule EINA_UNUSED, const void *map EINA_UNUSED,
                       unsigned long int offset EINA_UNUSED, unsigned long int length EINA_UNUSED)
{
}

/**
 * @brief Maps an entire file into memory for read-only access.
 *
 * This function creates a memory mapping of the entire file specified by
 * the Eina_File handle. The mapping is read-only.
 * If the file is already globally mapped, a reference count is incremented
 * and the existing mapping is returned.
 *
 * @param file The Eina_File handle for the file to map.
 * @param rule Population rule (unused on Windows for this function).
 * @return A pointer to the memory-mapped region if successful, NULL otherwise.
 *         The caller must release the mapping using eina_file_map_free() when done.
 */
EINA_API void *
eina_file_map_all(Eina_File *file, Eina_File_Populate rule EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (file->virtual) return eina_file_virtual_map_all(file);

   eina_lock_take(&file->lock);
   if (file->global_map == MAP_FAILED)
     {
        DWORD max_size_high;
        DWORD max_size_low;
        HANDLE fm;


        max_size_high = (DWORD)((file->length & 0xffffffff00000000ULL) >> 32);
        max_size_low = (DWORD)(file->length & 0x00000000ffffffffULL);
        fm = CreateFileMapping(file->handle, NULL, PAGE_READONLY,
                                     max_size_high, max_size_low, NULL);
        if (!fm)
          {
             eina_lock_release(&file->lock);
             return NULL;
          }

        file->global_map = MapViewOfFile(fm, FILE_MAP_READ,
                             0, 0, file->length);
        CloseHandle(fm);
        if (!file->global_map)
          file->global_map = MAP_FAILED;
     }

   if (file->global_map != MAP_FAILED)
     {
        file->global_refcount++;
        eina_lock_release(&file->lock);
        return file->global_map;
     }

   eina_lock_release(&file->lock);
   return NULL;
}

/**
 * @brief Maps a specific portion of a file into memory for read-only access.
 *
 * This function creates a memory mapping of a specified segment (offset and length)
 * of the file. The mapping is read-only.
 * If the requested segment is the entire file (offset 0 and length equals file length),
 * this function effectively calls eina_file_map_all().
 * Mappings are cached; requesting the same segment multiple times will return
 * the same memory address and increment a reference counter.
 *
 * @param file The Eina_File handle for the file to map.
 * @param rule Population rule (unused on Windows for this function).
 * @param offset The starting offset within the file for the mapping.
 * @param length The length of the file segment to map.
 * @return A pointer to the memory-mapped region if successful, NULL otherwise.
 *         Returns NULL if offset or offset+length are beyond the file size.
 *         The caller must release the mapping using eina_file_map_free() when done.
 */
EINA_API void *
eina_file_map_new(Eina_File *file, Eina_File_Populate rule,
                  unsigned long int offset, unsigned long int length)
{
   Eina_File_Map *map;
   unsigned long int key[2];

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (offset > file->length)
     return NULL;
   if (offset + length > file->length)
     return NULL;

   if (offset == 0UL && length == file->length)
     return eina_file_map_all(file, rule);

   if (file->virtual)
     return eina_file_virtual_map_new(file, offset, length);

   key[0] = offset;
   key[1] = length;

   eina_lock_take(&file->lock);

   map = eina_hash_find(file->map, &key);
   if (!map)
     {
        SYSTEM_INFO si;
        HANDLE fm;
        __int64 map_size;
        DWORD view_offset;
        DWORD view_length;
        DWORD granularity;

        map = malloc(sizeof (Eina_File_Map));
        if (!map)
          goto on_error;

        /*
         * the size of the mapping object is the offset plus the length,
         * which might be greater than a DWORD
         */
        map_size = (__int64)offset + (__int64)length;
        fm = CreateFileMapping(file->handle, NULL, PAGE_READONLY,
                               (DWORD)((map_size >> 32) & 0x00000000ffffffffULL),
                               (DWORD)(map_size & 0x00000000ffffffffULL),
                               NULL);
        if (!fm)
          goto on_error;

        /*
         * get the system allocation granularity as the
         * offset passed to MapViewOfFile() must be a
         * multiple of this granularity
         */
        GetSystemInfo(&si);
        granularity = si.dwAllocationGranularity;

        /*
         * view_offset is the greatest multiple of granularity, less or equal
         * than offset (and can be stored in a DWORD)
         */
        view_offset = (offset / granularity) * granularity;
        view_length = (offset - view_offset) + length;
        map->map = MapViewOfFile(fm, FILE_MAP_READ,
                                 0,
                                 view_offset,
                                 view_length);
        CloseHandle(fm);
        if (!map->map)
          goto on_error;

        map->ret = (unsigned char *)map->map + (offset - view_offset);
        map->offset = offset;
        map->length = length;
        map->refcount = 0;

        eina_hash_add(file->map, &key, map);
        eina_hash_direct_add(file->rmap, map->map, map);
     }

   map->refcount++;

   eina_lock_release(&file->lock);

   return map->ret;

 on_error:
   free(map);
   eina_lock_release(&file->lock);

   return NULL;
}

/**
 * @brief Releases a memory-mapped region of a file.
 *
 * This function decrements the reference count for the given memory-mapped region.
 * If the reference count drops to zero, the region is unmapped from memory.
 * It handles both globally mapped regions (from eina_file_map_all()) and
 * partially mapped regions (from eina_file_map_new()).
 *
 * @param file The Eina_File handle associated with the mapping.
 * @param map A pointer to the memory-mapped region to free. This pointer
 *            must have been previously returned by eina_file_map_all() or
 *            eina_file_map_new().
 */
EINA_API void
eina_file_map_free(Eina_File *file, void *map)
{
   EINA_SAFETY_ON_NULL_RETURN(file);

   if (file->virtual)
     {
        eina_file_virtual_map_free(file, map);
        return;
     }

   eina_lock_take(&file->lock);

   if (file->global_map == map)
     {
        file->global_refcount--;

        if (file->global_refcount > 0) goto on_exit;

        UnmapViewOfFile(file->global_map);
        file->global_map = MAP_FAILED;
     }
   else
     {
        eina_file_common_map_free(file, map, _eina_file_map_close);
     }

 on_exit:
   eina_lock_release(&file->lock);
}

/**
 * @brief Checks if a memory-mapped region has faulted (e.g., due to file truncation).
 *
 * @param file The Eina_File handle.
 * @param map Pointer to the memory-mapped region.
 * @return EINA_FALSE on Windows. This function is not fully implemented and
 *         indicates that fault detection for memory-mapped files (e.g., if the
 *         underlying file is truncated while mapped) is not handled.
 * @warning This function currently returns EINA_FALSE and includes a FIXME note
 *          about handling memory access violations for corrupted mapped files on Windows.
 */
EINA_API Eina_Bool
eina_file_map_faulted(Eina_File *file, void *map EINA_UNUSED)
{
#warning "We need to handle access to corrupted memory mapped file."
  if (file->virtual) return EINA_FALSE;
  /*
   * FIXME:
   * vc++ : http://msdn.microsoft.com/en-us/library/windows/desktop/aa366801%28v=vs.85%29.aspx
   *
   * mingw-w64 :
   * - 32 bits : there is a way to implement __try/__except/__final in C.
   *   see excpt.h header for 32-bits
   * - 64 bits : some inline assembly required for it.  See as example our
   *   startup-code in WinMainCRTStartup() in crtexe.c :
{
  int ret = 255;
#ifdef __SEH__
  asm ("\t.l_startw:\n"
    "\t.seh_handler __C_specific_handler, @except\n"
    "\t.seh_handlerdata\n"
    "\t.long 1\n"
    "\t.rva .l_startw, .l_endw, _gnu_exception_handler ,.l_endw\n"
    "\t.text"
    );
#endif
  mingw_app_type = 1;
  __security_init_cookie ();
  ret = __tmainCRTStartup ();
#ifdef __SEH__
  asm ("\tnop\n"
    "\t.l_endw: nop\n");
#endif
  return ret;
}
   */
   return EINA_FALSE;
}

/**
 * @brief Retrieves file status information (similar to fstatat).
 *
 * This function populates an Eina_Stat structure with information about the
 * file described by an Eina_File_Direct_Info structure. It uses `stat64`
 * internally. If the file type in @p info is EINA_FILE_UNKNOWN, it attempts
 * to determine the type (regular file or directory) based on `stat64` results.
 *
 * @param container Unused on Windows for this function. Typically, on POSIX,
 *                  this would be a file descriptor for the directory if @p info->path
 *                  is relative.
 * @param info Pointer to an Eina_File_Direct_Info structure containing the path
 *             and potentially the type of the file. The type may be updated by this function.
 * @param[out] st Pointer to an Eina_Stat structure to be filled with file status.
 * @return 0 on success, -1 on error (e.g., if `stat64` fails or inputs are NULL).
 */
EINA_API int
eina_file_statat(void *container EINA_UNUSED, Eina_File_Direct_Info *info, Eina_Stat *st)
{
   struct __stat64 buf;

   EINA_SAFETY_ON_NULL_RETURN_VAL(info, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, -1);

   if (stat64(info->path, &buf))
     {
        info->type = EINA_FILE_UNKNOWN;
        return -1;
     }

   if (info->type == EINA_FILE_UNKNOWN)
     {
        if (S_ISREG(buf.st_mode))
          info->type = EINA_FILE_REG;
        else if (S_ISDIR(buf.st_mode))
          info->type = EINA_FILE_DIR;
        else
          info->type = EINA_FILE_UNKNOWN;
     }

   st->dev = buf.st_dev;
   st->ino = buf.st_ino;
   st->mode = buf.st_mode;
   st->nlink = buf.st_nlink;
   st->uid = buf.st_uid;
   st->gid = buf.st_gid;
   st->rdev = buf.st_rdev;
   st->size = buf.st_size;
   st->blksize = 0;
   st->blocks = 0;
   st->atime = buf.st_atime;
   st->mtime = buf.st_mtime;
   st->ctime = buf.st_ctime;
   st->atimensec = 0;
   st->mtimensec = 0;
   st->ctimensec = 0;

   return 0;
}

/**
 * @brief Creates and opens a unique temporary file.
 *
 * This function generates a unique filename based on @p templatename and opens
 * the file. The @p templatename string must end with "XXXXXX". These 'X's are
 * replaced with characters to make the filename unique.
 * If @p templatename does not contain a directory separator ('/' or '\\'),
 * the temporary file is created in the directory returned by eina_environment_tmp_get().
 *
 * @param templatename A template for the filename. Example: "myapp_temp_XXXXXX" or
 *                     "/tmp/myapp_XXXXXX.log". If it contains "XXXXXX.", the part
 *                     after "." is treated as a suffix.
 * @param[out] path If not NULL, this will be set to an Eina_Tmpstr containing
 *                  the actual path of the created temporary file on success.
 *                  The caller is responsible for deleting this Eina_Tmpstr.
 *                  Set to NULL on failure.
 * @return A file descriptor for the opened temporary file on success, -1 on error.
 */
EINA_API int
eina_file_mkstemp(const char *templatename, Eina_Tmpstr **path)
{
   char buffer[PATH_MAX];
   const char *XXXXXX = NULL, *sep;
   int fd, len;

   EINA_SAFETY_ON_NULL_RETURN_VAL(templatename, -1);

   sep = strchr(templatename, '/');
   if (!sep) sep = strchr(templatename, '\\');
   if (sep)
     {
        len = eina_strlcpy(buffer, templatename, sizeof(buffer));
     }
   else
     {
        len = eina_file_path_join(buffer, sizeof(buffer),
                                  eina_environment_tmp_get(), templatename);
     }

   if ((XXXXXX = strstr(buffer, "XXXXXX.")) != NULL)
     fd = _eina_file_mkstemps(buffer, buffer + len - XXXXXX - 6);
   else
     fd = _eina_file_mkstemps(buffer, 0);

   if (fd < 0)
     {
        if (path) *path = NULL;
        return -1;
     }

   if (path) *path = eina_tmpstr_add(buffer);
   return fd;
}

/**
 * @brief Creates a unique temporary directory.
 *
 * This function generates a unique directory name based on @p templatename and
 * creates the directory. The @p templatename string must end with "XXXXXX".
 * These 'X's are replaced with characters to make the directory name unique.
 * If @p templatename does not contain a directory separator ('/' or '\\'),
 * the temporary directory is created in the directory returned by
 * eina_environment_tmp_get().
 *
 * @param templatename A template for the directory name. Example: "myapp_temp_dir_XXXXXX".
 * @param[out] path If not NULL, this will be set to an Eina_Tmpstr containing
 *                  the actual path of the created temporary directory on success.
 *                  The caller is responsible for deleting this Eina_Tmpstr.
 *                  Set to NULL on failure.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 */
EINA_API Eina_Bool
eina_file_mkdtemp(const char *templatename, Eina_Tmpstr **path)
{
   char buffer[PATH_MAX];
   char *tmpdirname, *sep;

   EINA_SAFETY_ON_NULL_RETURN_VAL(templatename, EINA_FALSE);

   sep = strchr(templatename, '/');
   if (!sep) sep = strchr(templatename, '\\');
   if (sep)
     {
        eina_strlcpy(buffer, templatename, sizeof(buffer));
     }
   else
     {
        eina_file_path_join(buffer, sizeof(buffer),
                            eina_environment_tmp_get(), templatename);
     }

   tmpdirname = _eina_file_mkdtemp(buffer);
   if (tmpdirname == NULL)
     {
        if (path) *path = NULL;
        return EINA_FALSE;
     }

   if (path) *path = eina_tmpstr_add(tmpdirname);
   return EINA_TRUE;
}

/**
 * @brief Checks user's accessibility to a file or directory.
 *
 * This function determines if the specified @p path can be accessed according
 * to the given @p mode.
 *
 * On Windows:
 * - Existence (EINA_FILE_ACCESS_MODE_EXIST) is checked using GetFileAttributes.
 * - Read access is implied if the file exists, as Windows primarily distinguishes
 *   between read/write and read-only.
 * - Write access (EINA_FILE_ACCESS_MODE_WRITE) checks if the FILE_ATTRIBUTE_READONLY
 *   is not set.
 * - Execute access (EINA_FILE_ACCESS_MODE_EXEC):
 *   - For files, it checks if the extension is ".exe" or ".bat".
 *   - For directories, it's considered executable (i.e., can be entered).
 *
 * @param path The path to the file or directory to check.
 * @param mode The access mode to check for. This can be a bitmask of:
 *             - EINA_FILE_ACCESS_MODE_EXIST: Check for existence.
 *             - EINA_FILE_ACCESS_MODE_READ: Check for read permission.
 *             - EINA_FILE_ACCESS_MODE_WRITE: Check for write permission.
 *             - EINA_FILE_ACCESS_MODE_EXEC: Check for execute permission.
 * @return EINA_TRUE if the access specified by @p mode is permitted,
 *         EINA_FALSE otherwise (e.g., path is NULL/empty, file does not exist
 *         when existence is checked, or permissions are denied).
 *         Returns EINA_FALSE for invalid mode bits.
 */
EINA_API Eina_Bool
eina_file_access(const char *path, Eina_File_Access_Mode mode)
{
   DWORD attr;

   if (!path || !*path)
     return EINA_FALSE;

   if ((mode != EINA_FILE_ACCESS_MODE_EXIST) &&
       ((mode >> 3) != 0))
     return EINA_FALSE;

   /*
    * Always check for existence for both files and directories
   */
   attr = GetFileAttributes(path);
   if (attr == INVALID_FILE_ATTRIBUTES)
     return EINA_FALSE;

   /*
    * On Windows a file or path is either read/write or read only.
    * So if it exists, it has at least read access.
    * So do something only if mode is EXEC or WRITE
    */

   if (mode & EINA_FILE_ACCESS_MODE_EXEC)
     {
        if (!(attr & FILE_ATTRIBUTE_DIRECTORY) &&
            !eina_str_has_extension(path, ".exe") &&
            !eina_str_has_extension(path, ".bat"))
          return EINA_FALSE;
     }

   if (mode & EINA_FILE_ACCESS_MODE_WRITE)
     {
        if (attr == INVALID_FILE_ATTRIBUTES)
          return EINA_FALSE;

        if (attr & FILE_ATTRIBUTE_READONLY)
          return EINA_FALSE;
     }

   return EINA_TRUE;
}
