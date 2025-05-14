#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#ifdef _WIN32
# include <direct.h>
# include <evil_private.h> /* mkdir realpath */
#endif

#ifdef HAVE_FEATURES_H
# include <features.h>
#endif
#include <ctype.h>
#include <errno.h>

#ifdef HAVE_ATFILE_SOURCE
# include <dirent.h>
#endif

#include "ecore_file_private.h"

/*
 * FIXME: the following functions will certainly not work on Windows:
 * ecore_file_app_exe_get()
 * ecore_file_escape_name()
 */

int _ecore_file_log_dom = -1;
static int _ecore_file_init_count = 0;

/**
 * @internal
 * @brief Retrieves file status information.
 *
 * This function is a wrapper around the stat() system call, providing
 * a unified way to get file metadata like modification time, size, mode,
 * and whether it's a directory or a regular file. It includes a
 * workaround for Windows where stat() fails on paths ending with a slash.
 *
 * @param file The path to the file.
 * @param mtime Pointer to store the modification time (seconds since epoch). Can be NULL.
 * @param size Pointer to store the file size in bytes. Can be NULL.
 * @param mode Pointer to store the file mode. Can be NULL.
 * @param is_dir Pointer to store a boolean indicating if it's a directory. Can be NULL.
 * @param is_reg Pointer to store a boolean indicating if it's a regular file. Can be NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., file not found).
 */
static Eina_Bool
_ecore_file_stat(const char *file,
                 long long *mtime,
                 long long *size,
                 mode_t *mode,
                 Eina_Bool *is_dir,
                 Eina_Bool *is_reg)
{
   struct stat st;
#ifdef _WIN32
   /*
    * On Windows, stat() returns -1 is file is a path finishing with
    * a slash or blackslash
    * see https://msdn.microsoft.com/en-us/library/14h5k7ff.aspx
    * ("Return Value" section)
    *
    * so we ensure that file never finishes with \ or /
    */
   char f[MAX_PATH];
   size_t len;

   len = strlen(file);
   if ((len + 1) > MAX_PATH)
     return EINA_FALSE;

   memcpy(f, file, len + 1);
   if ((f[len - 1] == '/') || (f[len - 1] == '\\'))
     f[len - 1] = '\0';

   if (stat(f, &st) < 0)
     return EINA_FALSE;
#else
   if (stat(file, &st) < 0)
     return EINA_FALSE;
#endif

   if (mtime) *mtime = st.st_mtime;
   if (size) *size = st.st_size;
   if (mode) *mode = st.st_mode;
   if (is_dir) *is_dir = S_ISDIR(st.st_mode);
   if (is_reg) *is_reg = S_ISREG(st.st_mode);

   return EINA_TRUE;
}

/**
 * @brief Initializes the Ecore_File library.
 *
 * This function sets up the Ecore_File library, including initializing
 * its dependencies (Ecore itself), registering a log domain, and
 * initializing sub-modules like path handling, file monitoring, and
 * file downloading.
 *
 * It uses a counter to ensure that it's initialized only once.
 *
 * @return The new initialization count. Returns 1 on the first successful
 *         initialization. If initialization fails, the count is decremented,
 *         and a value less than 1 might be returned.
 * @see ecore_file_shutdown()
 */
EAPI int
ecore_file_init()
{
   if (++_ecore_file_init_count != 1)
     return _ecore_file_init_count;

   if (!ecore_init())
     return --_ecore_file_init_count;

   _ecore_file_log_dom = eina_log_domain_register
     ("ecore_file", ECORE_FILE_DEFAULT_LOG_COLOR);
   if(_ecore_file_log_dom < 0)
     {
       EINA_LOG_ERR("Impossible to create a log domain for the ecore file module.");
       return --_ecore_file_init_count;
     }
   ecore_file_path_init();
   ecore_file_monitor_init();
   ecore_file_download_init();

   /* FIXME: were the tests disabled for a good reason ? */

   /*
   if (!ecore_file_monitor_init())
     goto shutdown_ecore_file_path;

   if (!ecore_file_download_init())
     goto shutdown_ecore_file_monitor;
   */

   return _ecore_file_init_count;

   /*
 shutdown_ecore_file_monitor:
   ecore_file_monitor_shutdown();
 shutdown_ecore_file_path:
   ecore_file_path_shutdown();

   return --_ecore_file_init_count;
   */
}

/**
 * @brief Shuts down the Ecore_File library.
 *
 * This function cleans up resources used by the Ecore_File library.
 * It shuts down sub-modules (download, monitor, path), unregisters the
 * log domain, and decrements the initialization counter. Ecore itself
 * is shut down when the counter reaches zero.
 *
 * @return The new initialization count. Returns 0 when the library is
 *         fully shut down.
 * @see ecore_file_init()
 */
EAPI int
ecore_file_shutdown()
{
   if (--_ecore_file_init_count != 0)
     return _ecore_file_init_count;

   ecore_file_download_shutdown();
   ecore_file_monitor_shutdown();
   ecore_file_path_shutdown();

   eina_log_domain_unregister(_ecore_file_log_dom);
   _ecore_file_log_dom = -1;

   ecore_shutdown();

   return _ecore_file_init_count;
}

/**
 * @brief Gets the modification time of a file.
 *
 * @param file The path to the file.
 * @return The last modification time in seconds since the Epoch, or 0 on error.
 *         Example: 1678886400 for March 15, 2023, 12:00:00 PM UTC.
 */
EAPI long long
ecore_file_mod_time(const char *file)
{
   long long time;

   if (!_ecore_file_stat(file, &time, NULL, NULL, NULL, NULL))
     return 0;

   return time;
}

/**
 * @brief Gets the size of a file.
 *
 * @param file The path to the file.
 * @return The size of the file in bytes, or 0 on error or if the file
 *         is not a regular file.
 *         Example: 1024 for a 1KB file.
 */
EAPI long long
ecore_file_size(const char *file)
{
   long long size;

   if (!_ecore_file_stat(file, NULL, &size, NULL, NULL, NULL))
     return 0;

   return size;
}

/**
 * @brief Checks if a file or directory exists.
 *
 * @param file The path to the file or directory.
 * @return @c EINA_TRUE if the file or directory exists, @c EINA_FALSE otherwise.
 * @note On non-Windows systems, this function has a workaround to return @c EINA_TRUE
 *       for the root directory "/" even if stat() might fail, to enable monitoring "/".
 */
EAPI Eina_Bool
ecore_file_exists(const char *file)
{
#ifdef _WIN32
   /* I prefer not touching the specific UNIX code... */
   return _ecore_file_stat(file, NULL, NULL, NULL, NULL, NULL);
#else
   struct stat st;
   if (!file) return EINA_FALSE;

   /*Workaround so that "/" returns a true, otherwise we can't monitor "/" in ecore_file_monitor*/
   if (stat(file, &st) < 0 && strcmp(file, "/")) return EINA_FALSE;
   return EINA_TRUE;
#endif
}

/**
 * @brief Checks if a path is a directory.
 *
 * @param file The path to check.
 * @return @c EINA_TRUE if the path is a directory, @c EINA_FALSE otherwise
 *         (e.g., it's a file, does not exist, or an error occurred).
 */
EAPI Eina_Bool
ecore_file_is_dir(const char *file)
{
   Eina_Bool is_dir;

   if (!_ecore_file_stat(file, NULL, NULL, NULL, &is_dir, NULL))
     return EINA_FALSE;

   return is_dir;
}

/**
 * @internal
 * @brief Default mode for creating directories.
 * Corresponds to rwxr-xr-x.
 */
static mode_t default_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

/**
 * @brief Creates a directory.
 *
 * This function attempts to create a new directory with the specified path.
 * The directory is created with default permissions (rwxr-xr-x).
 *
 * @param dir The path of the directory to create.
 *            Example: "/tmp/my_new_directory"
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., directory
 *         already exists, or insufficient permissions).
 */
EAPI Eina_Bool
ecore_file_mkdir(const char *dir)
{
   return (mkdir(dir, default_mode) == 0);
}

/**
 * @brief Creates multiple directories.
 *
 * This function iterates through a NULL-terminated array of directory paths
 * and attempts to create each one using ecore_file_mkdir().
 *
 * @param dirs A NULL-terminated array of strings, where each string is a
 *             directory path to be created.
 *             Example: `const char *dirs[] = {"/tmp/dir1", "/tmp/dir2", NULL};`
 * @return The number of directories successfully created. Returns -1 if @p dirs is NULL.
 */
EAPI int
ecore_file_mkdirs(const char **dirs)
{
   int i = 0;

   if (!dirs) return -1;

   for (; *dirs; dirs++)
     if (ecore_file_mkdir(*dirs))
       i++;
   return i;
}

/**
 * @brief Creates multiple subdirectories under a given base directory.
 *
 * This function first ensures the base directory exists (creating it if necessary
 * using ecore_file_mkpath()). Then, it iterates through a NULL-terminated array
 * of subdirectory names and attempts to create each one within the base directory.
 * If a subdirectory already exists and is a directory, it's counted as a success.
 *
 * @param base The path to the base directory.
 *             Example: "/tmp/my_app_data"
 * @param subdirs A NULL-terminated array of strings, where each string is the
 *                name of a subdirectory to create under @p base.
 *                Example: `const char *subdirs[] = {"cache", "logs", "config", NULL};`
 *                This would attempt to create "/tmp/my_app_data/cache",
 *                "/tmp/my_app_data/logs", etc.
 * @return The number of subdirectories successfully created or already existing as directories.
 *         Returns -1 if @p subdirs is NULL, or if @p base is NULL or empty.
 *         Returns 0 if the base directory cannot be created or accessed, or if
 *         path length limits are exceeded.
 * @note This function uses `mkdirat()` if `HAVE_ATFILE_SOURCE` is defined,
 *       otherwise it constructs full paths.
 */
EAPI int
ecore_file_mksubdirs(const char *base, const char **subdirs)
{
#ifndef HAVE_ATFILE_SOURCE
   char buf[PATH_MAX];
   int baselen;
#else
   int fd;
   DIR *dir;
#endif
   int i;

   if (!subdirs) return -1;
   if ((!base) || (base[0] == '\0')) return -1;

   if ((!ecore_file_is_dir(base)) && (!ecore_file_mkpath(base)))
     return 0;

#ifndef HAVE_ATFILE_SOURCE
   baselen = eina_strlcpy(buf, base, sizeof(buf));
   if ((baselen < 1) || (baselen + 1 >= (int)sizeof(buf)))
     return 0;

   if (buf[baselen - 1] != '/')
     {
        buf[baselen] = '/';
        baselen++;
     }
#else
   dir = opendir(base);
   if (!dir)
     return 0;
   fd = dirfd(dir);
#endif

   i = 0;
   for (; *subdirs; subdirs++)
     {
#ifdef HAVE_ATFILE_SOURCE
        struct stat st;
#endif
        Eina_Bool is_dir;

#ifndef HAVE_ATFILE_SOURCE
        eina_strlcpy(buf + baselen, *subdirs, sizeof(buf) - baselen);
        if (_ecore_file_stat(buf, NULL, NULL, NULL, &is_dir, NULL))
          {
#else
        if (fstatat(fd, *subdirs, &st, 0) == 0)
          {
             is_dir = S_ISDIR(st.st_mode);
#endif
             if (is_dir)
               {
                  i++;
                  continue;
               }
          }
        else
          {
             if (errno == ENOENT)
               {
#ifndef HAVE_ATFILE_SOURCE
                  if (ecore_file_mkdir(buf))
#else
                  if (mkdirat(fd, *subdirs, default_mode) == 0)
#endif
                    {
                       i++;
                       continue;
                    }
                 }
            }
     }

#ifdef HAVE_ATFILE_SOURCE
   closedir(dir);
#endif

   return i;
}

/**
 * @brief Removes an empty directory.
 *
 * This is a wrapper around the rmdir() system call.
 *
 * @param dir The path of the directory to remove.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (e.g., directory
 *         is not empty, does not exist, or insufficient permissions).
 */
EAPI Eina_Bool
ecore_file_rmdir(const char *dir)
{
   if (rmdir(dir) < 0) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Deletes a name from the filesystem.
 *
 * This is a wrapper around the unlink() system call. If the name was the
 * last link to a file and no processes have the file open, the file is
 * deleted and the space it was using is made available for reuse.
 *
 * @param file The path of the file or symbolic link to delete.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_file_unlink(const char *file)
{
   if (unlink(file) < 0) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Removes a file or an empty directory.
 *
 * This is a wrapper around the remove() system call.
 * For files, it is equivalent to ecore_file_unlink().
 * For directories, it is equivalent to ecore_file_rmdir().
 *
 * @param file The path of the file or empty directory to remove.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_file_remove(const char *file)
{
   if (remove(file) < 0) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Recursively removes a directory and its contents.
 *
 * If @p dir is a file, it will be unlinked. If it's a directory,
 * all its contents (files and subdirectories) will be removed recursively,
 * and then the directory itself will be removed.
 *
 * @param dir The path to the file or directory to remove.
 * @return @c EINA_TRUE if the entire operation was successful,
 *         @c EINA_FALSE otherwise (e.g., if any part of the removal fails,
 *         or if the initial path does not exist).
 * @note On non-Windows systems, this function uses `lstat()` to correctly
 *       handle symbolic links to directories (it will remove the link, not
 *       the target directory's contents). On Windows, it uses `_ecore_file_stat()`.
 */
EAPI Eina_Bool
ecore_file_recursive_rm(const char *dir)
{
#ifndef _WIN32
   struct stat st;
#endif
   Eina_Bool is_dir;

#ifdef _WIN32
   if (!_ecore_file_stat(dir, NULL, NULL, NULL, &is_dir, NULL))
     return EINA_FALSE;
#else
   if (lstat(dir, &st) == -1)
     return EINA_FALSE;
   is_dir = S_ISDIR(st.st_mode);
#endif

   if (is_dir)
     {
        Eina_File_Direct_Info *info;
        Eina_Iterator *it;
        int ret;

        ret = 1;
        it = eina_file_direct_ls(dir);
        EINA_ITERATOR_FOREACH(it, info)
          {
             if (!ecore_file_recursive_rm(info->path))
               ret = 0;
          }
        eina_iterator_free(it);

        if (!ecore_file_rmdir(dir)) ret = 0;
        if (ret)
            return EINA_TRUE;
        else
            return EINA_FALSE;
     }
   else
     {
        return ecore_file_unlink(dir);
     }
}

/**
 * @internal
 * @brief Creates a directory if it doesn't already exist.
 *
 * This function checks if the given path exists. If it doesn't, it attempts
 * to create it as a directory. If it exists but is not a directory,
 * it returns failure.
 *
 * @param path The directory path to check and potentially create.
 * @return @c EINA_TRUE if the directory exists or was successfully created,
 *         @c EINA_FALSE otherwise (e.g., path exists but is not a directory,
 *         or mkdir failed).
 * @note Includes a workaround for Windows to consider paths like "C:" as valid
 *       existing directories.
 */
static inline Eina_Bool
_ecore_file_mkpath_if_not_exists(const char *path)
{
   Eina_Bool is_dir;

   /* Windows: path like C: or D: etc are valid, but stat() returns an error */
#ifdef _WIN32
   if ((strlen(path) == 2) &&
       ((path[0] >= 'a' && path[0] <= 'z') ||
        (path[0] >= 'A' && path[0] <= 'Z')) &&
       (path[1] == ':'))
     return EINA_TRUE;
#endif

   if (!_ecore_file_stat(path, NULL, NULL, NULL, &is_dir, NULL))
     return ecore_file_mkdir(path);
   else if (!is_dir)
     return EINA_FALSE;
   else
     return EINA_TRUE;
}

/**
 * @brief Creates a directory and all its parent directories if they do not exist.
 *
 * This function works like `mkdir -p`. It will create each component of the
 * specified path as a directory if it doesn't already exist.
 *
 * @param path The full directory path to create.
 *             Example: "/tmp/a/b/c" - this will create /tmp/a, then /tmp/a/b,
 *                      then /tmp/a/b/c if they don't exist.
 * @return @c EINA_TRUE if the entire path was successfully created or already
 *         existed as directories. @c EINA_FALSE on any error (e.g., a component
 *         of the path exists but is not a directory, permission denied, or path
 *         is too long).
 * @note EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE) is used.
 */
EAPI Eina_Bool
ecore_file_mkpath(const char *path)
{
   char ss[PATH_MAX];
   unsigned int i;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, EINA_FALSE);

   if (ecore_file_is_dir(path))
     return EINA_TRUE;

   for (i = 0; path[i] != '\0'; ss[i] = path[i], i++)
     {
        if (i == sizeof(ss) - 1) return EINA_FALSE;
        if (((path[i] == '/') || (path[i] == '\\')) && (i > 0))
          {
             ss[i] = '\0';
             if (!_ecore_file_mkpath_if_not_exists(ss))
               return EINA_FALSE;
          }
     }
   ss[i] = '\0';
   return _ecore_file_mkpath_if_not_exists(ss);
}

/**
 * @brief Creates multiple directory paths, including parent directories.
 *
 * This function iterates through a NULL-terminated array of directory paths
 * and calls ecore_file_mkpath() for each one.
 *
 * @param paths A NULL-terminated array of strings, where each string is a
 *              directory path to be created (including any necessary parent
 *              directories).
 *              Example: `const char *paths[] = {"/tmp/app/data", "/var/log/my_app", NULL};`
 * @return The number of paths successfully created. Returns -1 if @p paths is NULL.
 */
EAPI int
ecore_file_mkpaths(const char **paths)
{
   int i = 0;

   if (!paths) return -1;

   for (; *paths; paths++)
     if (ecore_file_mkpath(*paths))
       i++;
   return i;
}

/**
 * @brief Copies a file from a source path to a destination path.
 *
 * This function performs a binary copy of the file. It will not copy
 * directories. If the source and destination paths resolve to the same
 * file, the copy is aborted.
 *
 * @param src The path to the source file.
 * @param dst The path to the destination file. If it exists, it will be
 *            overwritten.
 * @return @c EINA_TRUE on successful copy, @c EINA_FALSE on failure (e.g.,
 *         source does not exist, cannot open source or destination, write error,
 *         or src and dst are the same file).
 */
EAPI Eina_Bool
ecore_file_cp(const char *src, const char *dst)
{
   FILE *f1, *f2;
   char buf[16384];
   char realpath1[PATH_MAX], realpath2[PATH_MAX];
   size_t num;
   Eina_Bool ret = EINA_TRUE;

   if (!realpath(src, realpath1)) return EINA_FALSE;
   if (realpath(dst, realpath2) && !strcmp(realpath1, realpath2)) return EINA_FALSE;

   f1 = fopen(src, "rb");
   if (!f1) return EINA_FALSE;
   f2 = fopen(dst, "wb");
   if (!f2)
     {
        fclose(f1);
        return EINA_FALSE;
     }
   while ((num = fread(buf, 1, sizeof(buf), f1)) > 0)
     {
        if (fwrite(buf, 1, num, f2) != num) ret = EINA_FALSE;
     }
   fclose(f1);
   fclose(f2);
   return ret;
}

/**
 * @brief Moves (renames) a file or directory.
 *
 * This function attempts to rename @p src to @p dst.
 * If rename() fails with EXDEV (indicating @p src and @p dst are on
 * different filesystems), and @p src is a regular file, it will attempt
 * a copy-then-delete operation. This involves:
 * 1. Creating a temporary file in the destination directory.
 * 2. Copying @p src to the temporary file.
 * 3. Setting the temporary file's permissions to match @p src.
 * 4. Atomically renaming the temporary file to @p dst.
 * 5. If atomic rename fails, it falls back to a simple copy to @p dst.
 * 6. Deleting the temporary file and the original @p src.
 *
 * On Windows, if rename() fails with ENOENT and @p dst exists as a regular
 * file, it will attempt to unlink @p dst first and then retry the rename.
 *
 * @param src The path to the source file or directory.
 * @param dst The new path for the file or directory.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_file_mv(const char *src, const char *dst)
{
   char buf[PATH_MAX];
   int fd;

   if (rename(src, dst))
     {
        // File cannot be moved directly because
        // it resides on a different mount point.
        if (errno == EXDEV)
          {
             mode_t mode;
             Eina_Bool is_reg;

             // Make sure this is a regular file before
             // we do anything fancy.
             if (!_ecore_file_stat(src, NULL, NULL, &mode, NULL, &is_reg))
                 goto FAIL;
             if (is_reg)
               {
                  char *dir;
                  Eina_Tmpstr *tmpstr = NULL;

                  dir = ecore_file_dir_get(dst);
                  // Since we can't directly rename, try to
                  // copy to temp file in the dst directory
                  // and then rename.
                  snprintf(buf, sizeof(buf), "%s/.%s.tmp.XXXXXX",
                           dir, ecore_file_file_get(dst));
                  free(dir);
                  fd = eina_file_mkstemp(buf, &tmpstr);
                  if (fd < 0) goto FAIL;
                  close(fd);

                  // Copy to temp file
                  if (!ecore_file_cp(src, tmpstr))
                    {
                       eina_tmpstr_del(tmpstr);
                       goto FAIL;
                    }

                  // Set file permissions of temp file to match src
                  if (chmod(tmpstr, mode) == -1)
                    {
                       eina_tmpstr_del(tmpstr);
                       goto FAIL;
                    }

                  // Try to atomically move temp file to dst
                  if (rename(tmpstr, dst))
                    {
                       // If we still cannot atomically move
                       // do a normal copy and hope for the best.
                       if (!ecore_file_cp(tmpstr, dst))
                         {
                            eina_tmpstr_del(tmpstr);
                            goto FAIL;
                         }
                    }

                  // Delete temporary file and src
                  ecore_file_unlink(tmpstr);
                  ecore_file_unlink(src);
                  eina_tmpstr_del(tmpstr);
                  goto PASS;
               }
          }
#ifdef _WIN32
        if (errno == ENOENT)
          {
             struct _stat s;
             _stat(dst, &s);
             if (_S_IFREG & s.st_mode)
               {
                  ecore_file_unlink(dst);
                  if (rename(src, dst))
                    {
                       return EINA_TRUE;
                    }
               }
          }
#endif
        goto FAIL;
     }

PASS:
   return EINA_TRUE;

FAIL:
   return EINA_FALSE;
}

/**
 * @brief Creates a symbolic link.
 *
 * This function creates a symbolic link named @p dest which points to @p src.
 *
 * @param src The path that the new symbolic link will point to.
 * @param dest The path of the symbolic link to be created.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @note This function is not implemented on Windows and will always
 *       return @c EINA_FALSE on that platform.
 */
EAPI Eina_Bool
ecore_file_symlink(const char *src, const char *dest)
{
#ifndef _WIN32
   return !symlink(src, dest);
#else
   return EINA_FALSE;
   (void)src;
   (void)dest;
#endif
}

/**
 * @brief Returns the canonicalized absolute pathname.
 *
 * This function resolves all symbolic links, references to /./, /../
 * and extra '/' characters in @p file and returns the absolute pathname.
 * The returned string is allocated with strdup() and should be freed
 * by the caller.
 *
 * @param file The path to resolve.
 * @return A newly allocated string containing the canonicalized absolute
 *         pathname, or a an empty string `""` (allocated) if an error occurs or if @p file is NULL.
 *         The caller is responsible for freeing the returned string.
 *         Example: If @p file is "/usr/local/../bin/./myprog", it might return "/usr/bin/myprog".
 */
EAPI char *
ecore_file_realpath(const char *file)
{
   char buf[PATH_MAX];

   /*
    * Some implementations of realpath do not conform to the SUS.
    * And as a result we must prevent a null arg from being passed.
    */
   if (!file) return strdup("");
   if (!realpath(file, buf)) return strdup("");

   return strdup(buf);
}

/**
 * @brief Extracts the filename component from a path.
 *
 * This function returns a pointer to the filename part of a path string.
 * It searches for the last occurrence of '/' (and '\' on Windows) and
 * returns a pointer to the character immediately following it. If no
 * directory separator is found, it returns the original path.
 *
 * @param path The full path string.
 *             Example: "/usr/local/bin/my_program" or "C:\\Users\\Name\\file.txt"
 * @return A pointer to the filename part within the original @p path string.
 *         Returns NULL if @p path is NULL.
 *         Example: For "/usr/local/bin/my_program", it returns "my_program".
 *                  For "file.txt", it returns "file.txt".
 * @warning The returned pointer is part of the input @p path string and
 *          should not be freed or modified if @p path is a constant string.
 *          It is valid only as long as @p path is valid.
 */
EAPI const char *
ecore_file_file_get(const char *path)
{
   char *result = NULL;

   if (!path) return NULL;

   if ((result = strrchr(path, '/'))) result++;
   else result = (char *)path;

#ifdef _WIN32
   /*
    * Here, we know that there is no more / in the string beginning at
    * 'result'. So just check that there is no more \ from it.
    */
   {
      char *result_backslash;
      if ((result_backslash = strrchr(result, '\\')))
        result = ++result_backslash;
   }
#endif

   return result;
}

/**
 * @brief Extracts the directory component from a path.
 *
 * This function returns the directory part of a path string. It uses
 * the dirname() function, which may modify the input string or return
 * a pointer to static storage. Therefore, this function copies the
 * input path to a buffer before calling dirname() and then duplicates
 * the result.
 *
 * @param file The full path string.
 *             Example: "/usr/local/bin/my_program" or "C:\\Users\\Name\\file.txt"
 * @return A newly allocated string containing the directory part of the path.
 *         Returns NULL if @p file is NULL. The caller is responsible for
 *         freeing the returned string.
 *         Example: For "/usr/local/bin/my_program", it returns "/usr/local/bin".
 *                  For "file.txt", it might return "." (current directory).
 * @note The behavior of dirname() can vary (e.g., regarding trailing slashes).
 *       This function strdups the result of dirname.
 */
EAPI char *
ecore_file_dir_get(const char *file)
{
   char *p;
   char buf[PATH_MAX];

   if (!file) return NULL;
   strncpy(buf, file, PATH_MAX);
   buf[PATH_MAX - 1] = 0;
   p = dirname(buf);
   return strdup(p);
}

/**
 * @brief Checks if a file can be read.
 *
 * @param file The path to the file.
 * @return @c EINA_TRUE if the file is readable, @c EINA_FALSE otherwise.
 * @see eina_file_access()
 */
EAPI Eina_Bool
ecore_file_can_read(const char *file)
{
   return eina_file_access(file, EINA_FILE_ACCESS_MODE_READ);
}

/**
 * @brief Checks if a file can be written to.
 *
 * @param file The path to the file.
 * @return @c EINA_TRUE if the file is writable, @c EINA_FALSE otherwise.
 * @see eina_file_access()
 */
EAPI Eina_Bool
ecore_file_can_write(const char *file)
{
   return eina_file_access(file, EINA_FILE_ACCESS_MODE_WRITE);
}

/**
 * @brief Checks if a file can be executed.
 *
 * @param file The path to the file.
 * @return @c EINA_TRUE if the file is executable, @c EINA_FALSE otherwise.
 * @see eina_file_access()
 */
EAPI Eina_Bool
ecore_file_can_exec(const char *file)
{
   return eina_file_access(file, EINA_FILE_ACCESS_MODE_EXEC);
}

/**
 * @brief Reads the value of a symbolic link.
 *
 * This function reads the contents of the symbolic link @p link, i.e.,
 * the path it points to. The returned string is allocated with strdup()
 * and should be freed by the caller.
 *
 * @param link The path to the symbolic link.
 * @return A newly allocated string containing the path the symbolic link
 *         points to, or NULL on error (e.g., @p link is not a symbolic
 *         link, or readlink() fails). The caller is responsible for
 *         freeing the returned string.
 * @note This function is not implemented on Windows and will always
 *       return NULL on that platform.
 */
EAPI char *
ecore_file_readlink(const char *link)
{
#ifndef _WIN32
   char buf[PATH_MAX];
   int count;

   if ((count = readlink(link, buf, sizeof(buf) - 1)) < 0) return NULL;
   buf[count] = 0;
   return strdup(buf);
#else
   return NULL;
   (void)link;
#endif
}

/**
 * @brief Lists the contents of a directory.
 *
 * This function reads the entries in the specified directory @p dir,
 * creates a list of their names (filenames only, not full paths),
 * sorts this list alphabetically (using strcoll for locale-aware sorting),
 * and returns it.
 *
 * @param dir The path to the directory to list.
 * @return A new Eina_List containing strings (char *) of the filenames
 *         in the directory, sorted alphabetically. Returns NULL if the
 *         directory cannot be opened or read.
 *         The caller is responsible for freeing the list and its string contents
 *         (e.g., using EINA_LIST_FREE).
 *         Example of list structure:
 *         If directory "/tmp/foo" contains "apple.txt", "Banana", and ".config":
 *         The returned list might contain (after sorting):
 *         - ".config"
 *         - "Banana"
 *         - "apple.txt"
 * @see eina_file_direct_ls()
 * @see eina_list_free()
 * @see eina_list_free_cb()
 */
EAPI Eina_List *
ecore_file_ls(const char *dir)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *ls;
   Eina_List *list = NULL;

   ls = eina_file_direct_ls(dir);
   if (!ls) return NULL;

   EINA_ITERATOR_FOREACH(ls, info)
     {
        char *f;

        f = strdup(info->path + info->name_start);
        list = eina_list_append(list, f);
     }
   eina_iterator_free(ls);

   list = eina_list_sort(list, eina_list_count(list), EINA_COMPARE_CB(strcoll));

   return list;
}

/**
 * @brief Extracts the executable part from an application command string.
 *
 * This function parses a command string, potentially containing arguments
 * and quotes, and extracts the first token, which is assumed to be the
 * executable. It handles:
 * - Escaped characters (e.g., `\ `, `\"`).
 * - Double and single quotes.
 * - Tilde expansion (`~/`) at the beginning of the path.
 *
 * The parsing stops at the first unescaped whitespace character outside of quotes.
 *
 * @param app The application command string.
 *            Examples:
 *            - "/usr/bin/my_app -arg1 value" -> "/usr/bin/my_app"
 *            - "\"C:\\Program Files\\App\\app.exe\" --option" -> "C:\\Program Files\\App\\app.exe"
 *            - "~/bin/script.sh param" -> "/home/user/bin/script.sh" (if home is /home/user)
 *            - "my_command" -> "my_command"
 * @return A newly allocated string containing the extracted executable path/name.
 *         Returns NULL if @p app is NULL or if memory allocation fails.
 *         The caller is responsible for freeing the returned string.
 */
EAPI char *
ecore_file_app_exe_get(const char *app)
{
   Eina_Strbuf *buf;
   char *exe;
   const char *p;
   Eina_Bool in_qout_double = EINA_FALSE;
   Eina_Bool in_qout_single = EINA_FALSE;

   if (!app) return NULL;
   buf = eina_strbuf_new();
   if (!buf) return NULL;
   p = app;
   if ((p[0] == '~') && (p[1] == '/'))
     {
        const char *home = eina_environment_home_get();
        if (home) eina_strbuf_append(buf, home);
        p++;
     }
   for (; *p; p++)
     {
        if (in_qout_double)
          {
             if (*p == '\\')
               {
                  if (p[1]) p++;
                  eina_strbuf_append_char(buf, *p);
               }
             else if (*p == '"') in_qout_double = EINA_FALSE;
             else eina_strbuf_append_char(buf, *p);
          }
        else if (in_qout_single)
          {
             if (*p == '\\')
               {
                  if (p[1]) p++;
                  eina_strbuf_append_char(buf, *p);
               }
             else if (*p == '\'') in_qout_single = EINA_FALSE;
             else eina_strbuf_append_char(buf, *p);
          }
        else
          {
             if (*p == '\\')
               {
                  if (p[1]) p++;
                  eina_strbuf_append_char(buf, *p);
               }
             else if (*p == '"') in_qout_double = EINA_TRUE;
             else if (*p == '\'') in_qout_single = EINA_TRUE;
             else
               {
                  if (isspace((unsigned char)(*p))) break;
                  eina_strbuf_append_char(buf, *p);
               }
          }
     }
   exe = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return exe;
}

/**
 * @brief Escapes special characters in a filename for shell command usage.
 *
 * This function takes a filename and returns a new string where characters
 * that have special meaning in a shell (like spaces, backslashes, quotes,
 * semicolons, etc.) are prefixed with a backslash. Tabs (`\t`) are
 * replaced with `\\t` and newlines (`\n`) with `\\n`.
 *
 * This is useful for safely including a filename in a shell command string.
 *
 * @param filename The filename to escape.
 *                 Example: "my file with spaces'and\"quotes"
 * @return A newly allocated string with special characters escaped.
 *         Returns NULL if @p filename is NULL or if the escaped string would
 *         exceed PATH_MAX - 6 characters (a somewhat arbitrary limit to prevent
 *         buffer overflows in the internal buffer `buf`).
 *         The caller is responsible for freeing the returned string.
 *         Example: For "my file with spaces'and\"quotes", it might return
 *                  "my\\ file\\ with\\ spaces\\'and\\\"quotes" (actual output
 *                  depends on the full set of escaped characters).
 * @note EINA_SAFETY_ON_NULL_RETURN_VAL(filename, NULL) is used.
 */
EAPI char *
ecore_file_escape_name(const char *filename)
{
   const char *p;
   char *q;
   char buf[PATH_MAX];

   EINA_SAFETY_ON_NULL_RETURN_VAL(filename, NULL);

   p = filename;
   q = buf;
   while (*p)
     {
        if ((q - buf) > (PATH_MAX - 6)) return NULL;
        if (
            (*p == ' ') || (*p == '\\') || (*p == '\'') ||
            (*p == '\"') || (*p == ';') || (*p == '!') ||
            (*p == '#') || (*p == '$') || (*p == '%') ||
            (*p == '&') || (*p == '*') || (*p == '(') ||
            (*p == ')') || (*p == '[') || (*p == ']') ||
            (*p == '{') || (*p == '}') || (*p == '|') ||
            (*p == '<') || (*p == '>') || (*p == '?')
            )
          {
             *q = '\\';
             q++;
          }
        else if (*p == '\t')
          {
             *q = '\\';
             q++;
             *q = '\\';
             q++;
             *q = 't';
             q++;
             p++;
             continue;
          }
        else if (*p == '\n')
          {
            *q = '\\';
            q++;
            *q = '\\';
            q++;
            *q = 'n';
            q++;
            p++;
            continue;
          }

        *q = *p;
        q++;
        p++;
     }
   *q = 0;
   return strdup(buf);
}

/**
 * @brief Removes the extension from a filename or path.
 *
 * This function finds the last occurrence of '.' in the given path.
 * If found and it's not the first character of the path, it returns a
 * new string containing the part of the path before this dot.
 * If no dot is found, or if the dot is the first character (e.g., ".bashrc"),
 * a copy of the original path is returned.
 *
 * @param path The filename or path string.
 *             Examples:
 *             - "/path/to/file.txt" -> "/path/to/file"
 *             - "archive.tar.gz"    -> "archive.tar"
 *             - "nodotfile"         -> "nodotfile"
 *             - ".hiddenfile"       -> ".hiddenfile"
 *             - "/path/to/.config"  -> "/path/to/.config"
 * @return A newly allocated string with the extension stripped, or a copy
 *         of the original if no extension was found as described.
 *         Returns NULL if @p path is NULL or if memory allocation fails.
 *         The caller is responsible for freeing the returned string.
 */
EAPI char *
ecore_file_strip_ext(const char *path)
{
   char *p, *file = NULL;

   if (!path)
     return NULL;

   p = strrchr(path, '.');
   if (!p) // No dot found
     file = strdup(path);
   else if (p != path) // Dot found and it's not the first character
     {
        file = malloc(((p - path) + 1) * sizeof(char));
        if (file)
          {
             memcpy(file, path, (p - path));
             file[p - path] = 0;
          }
     }
   else // Dot is the first character (e.g. ".bashrc") or path is just "."
     {
        file = strdup(path);
     }

   return file;
}

/**
 * @brief Checks if a directory is empty.
 *
 * This function attempts to list the contents of the directory. If the
 * listing is successful and the first item is found, the directory is
 * considered not empty. If the listing is successful and no items are
 * found, it's considered empty.
 *
 * @param dir The path to the directory.
 * @return 1 if the directory is empty.
 *         0 if the directory is not empty.
 *        -1 if an error occurs (e.g., @p dir is not a directory,
 *           cannot be accessed, or eina_file_direct_ls() fails).
 */
EAPI int
ecore_file_dir_is_empty(const char *dir)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;

   it = eina_file_direct_ls(dir);
   if (!it) return -1;

   EINA_ITERATOR_FOREACH(it, info)
     {
        // If we find any entry, it's not empty.
        eina_iterator_free(it);
        return 0; // Not empty
     }

   eina_iterator_free(it);
   return 1; // Empty
}
