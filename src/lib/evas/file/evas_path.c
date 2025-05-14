/**
 * @file
 * @brief Filesystem path manipulation and query functions.
 *
 * This file provides utility functions for working with filesystem paths,
 * such as joining path components, checking for existence, determining
 * if a path is a file or directory, listing directory contents, getting
 * modification times, and resolving paths.
 *
 * It primarily targets Unix-like filesystems.
 */

/* os dependent file code. for unix-y like fs's only for now */
/* if your os doesn't use unix-like fs starting with "/" for the root and */
/* the file path separator isn't "/" then you may need to help out by */
/* adding in a new set of functions here */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
/* get the casefold feature! */
#include <unistd.h>
#include <sys/param.h>

#include "evas_common_private.h"
#include "evas_private.h"

#ifdef _WIN32
# define EVAS_PATH_SEPARATOR "\\"
#else
# define EVAS_PATH_SEPARATOR "/"
#endif

/**
 * @brief Joins two path components with the appropriate separator.
 *
 * This function takes two path components, `path` and `end`, and
 * concatenates them with the system-specific path separator
 * (e.g., "/" on Unix, "\" on Windows).
 *
 * @param path The first part of the path.
 * @param end The second part of the path to append.
 * @return A newly allocated string containing the joined path,
 *         or @c NULL on allocation failure or if both inputs are @c NULL.
 *         The caller is responsible for freeing the returned string.
 *         If @p path is @c NULL, a copy of @p end is returned.
 *         If @p end is @c NULL, a copy of @p path is returned.
 *
 * @code
 * char *full_path = evas_file_path_join("/usr/local", "bin");
 * // full_path might be "/usr/local/bin"
 * free(full_path);
 * @endcode
 */
char *
evas_file_path_join(const char *path, const char *end)
{
   char *res = NULL;
   size_t len;

   if ((!path) && (!end)) return NULL;
   if (!path) return strdup(end);
   if (!end) return strdup(path);
   len = strlen(path);
   len += strlen(end);
   len += strlen(EVAS_PATH_SEPARATOR);
   res = malloc(len + 1);
   if (!res) return NULL;
   strcpy(res, path);
   strcat(res, EVAS_PATH_SEPARATOR);
   strcat(res, end);
   return res;
}

/**
 * @brief Checks if a file or directory exists at the given path.
 *
 * @param path The path to check.
 * @return 1 if the path exists, 0 otherwise.
 *
 * @code
 * if (evas_file_path_exists("/etc/hosts")) {
 *   // /etc/hosts exists
 * }
 * @endcode
 */
int
evas_file_path_exists(const char *path)
{
   struct stat st;

   if (!stat(path, &st)) return 1;
   return 0;
}

/**
 * @brief Checks if the given path points to a regular file.
 *
 * @param path The path to check.
 * @return 1 if the path is a regular file, 0 otherwise (e.g., if it's a
 *         directory, symlink, or does not exist).
 *
 * @code
 * if (evas_file_path_is_file("/home/user/document.txt")) {
 *   // It's a file
 * }
 * @endcode
 */
int
evas_file_path_is_file(const char *path)
{
   struct stat st;

   if (stat(path, &st) == -1) return 0;
   if (S_ISREG(st.st_mode)) return 1;
   return 0;
}

/**
 * @brief Checks if the given path points to a directory.
 *
 * @param path The path to check.
 * @return 1 if the path is a directory, 0 otherwise (e.g., if it's a
 *         file, symlink, or does not exist).
 *
 * @code
 * if (evas_file_path_is_dir("/var/log")) {
 *   // It's a directory
 * }
 * @endcode
 */
int
evas_file_path_is_dir(const char *path)
{
   struct stat st;

   if (stat(path, &st) == -1) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

/**
 * @brief Lists files and directories within a given path, optionally matching a pattern.
 *
 * @param path The directory path to list.
 * @param match A shell wildcard pattern to filter results (e.g., "*.txt").
 *              If @c NULL, all entries are returned.
 * @param match_case If 0, matching is case-insensitive. If non-zero,
 *                   matching is case-sensitive. Note that case-insensitive
 *                   matching availability depends on libc support (FNM_IGNORECASE).
 * @return An Eina_List of strings, where each string is a file or directory name
 *         within the specified @p path that matches the @p match pattern.
 *         Returns @c NULL if the directory cannot be read or on error.
 *         The caller is responsible for freeing the list and its string contents
 *         (e.g., using `eina_list_free()` and `free()` for each item, or
 *         a custom free function with `eina_list_free()`).
 *
 * @code
 * Eina_List *files = evas_file_path_list("/tmp", "*.log", 0);
 * const char *filename;
 * EINA_LIST_FREE(files, filename) {
 *   printf("Found log file: %s\n", filename);
 *   free((void*)filename); // Cast needed as eina_list_data_get returns const
 * }
 * @endcode
 */
Eina_List *
evas_file_path_list(char *path, const char *match, int match_case)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_List *files = NULL;
   int flags;

   flags = EINA_FNMATCH_PATHNAME;
   if (!match_case)
     flags |= EINA_FNMATCH_CASEFOLD;
#if defined FNM_IGNORECASE
   if (!match_case)
     flags |= FNM_IGNORECASE;
#else
/*#warning "Your libc does not provide case-insensitive matching!"*/
#endif

   it = eina_file_direct_ls(path);
   EINA_ITERATOR_FOREACH(it, info)
     {
        if (match)
          {
             if (eina_fnmatch(match, info->path + info->name_start, flags))
               files = eina_list_append(files, strdup(info->path + info->name_start));
          }
        else
          files = eina_list_append(files, strdup(info->path + info->name_start));
     }
   if (it) eina_iterator_free(it);
   return files;
}

/**
 * @brief Gets the last modification time of a file.
 *
 * This function returns the later of the status change time (st_ctime)
 * and the data modification time (st_mtime).
 *
 * @param file The path to the file.
 * @return The modification time as a DATA64 (typically a 64-bit integer
 *         representing seconds since the Epoch). Returns 0 if the file
 *         information cannot be retrieved (e.g., file does not exist).
 *
 * @code
 * DATA64 mod_time = evas_file_modified_time("myfile.dat");
 * if (mod_time > 0) {
 *   // Process modification time
 * }
 * @endcode
 */
DATA64
evas_file_modified_time(const char *file)
{
   struct stat st;

   if (stat(file, &st) < 0) return 0;
   if (st.st_ctime > st.st_mtime) return (DATA64)st.st_ctime;
   else return (DATA64)st.st_mtime;
   return 0;
}

/**
 * @brief Resolves a file path to its absolute form.
 *
 * Currently, this function simply duplicates the input string.
 * The original implementation intended to use `realpath()` but it is
 * commented out.
 *
 * @param file The file path to resolve.
 * @return A newly allocated string containing the "resolved" path.
 *         This is currently just a copy of @p file.
 *         The caller is responsible for freeing the returned string.
 *         Returns @c NULL if @p file is @c NULL or on allocation failure.
 *
 * @code
 * char *abs_path = evas_file_path_resolve("../some/file.txt");
 * // abs_path will be "../some/file.txt" in the current implementation
 * free(abs_path);
 * @endcode
 */
char *
evas_file_path_resolve(const char *file)
{
#if 0
   char buf[PATH_MAX], *buf2;
#endif

   return strdup(file);
#if 0
   if (!realpath(file, buf)) return NULL;
   buf2 = strdup(buf);
   return buf2;
#endif
}
