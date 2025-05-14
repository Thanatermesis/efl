/* EINA - EFL data type library
 * Copyright (C) 2007-2008 Jorge Luis Zapata Muga, Vincent Torri
 * Copyright (C) 2010-2011 Cedric Bail
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include <fcntl.h>

#if defined(__linux__)
# include <sys/syscall.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif

#define PATH_DELIM '/'

#include "eina_config.h"
#include "eina_private.h"
#include "eina_alloca.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_cpu.h"
#include "eina_file.h"
#include "eina_stringshare.h"
#include "eina_hash.h"
#include "eina_list.h"
#include "eina_lock.h"
#include "eina_mmap.h"
#include "eina_log.h"
#include "eina_xattr.h"
#include "eina_file_common.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

#define EINA_SMALL_PAGE eina_cpu_page_size()

// FIXME: This assumes HugeTLB size of 2Mb. How to get this information at runtime?
#define EINA_HUGE_PAGE (2 * 1024 * 1024)
#define EINA_HUGE_PAGE_MIN (8 * EINA_HUGE_PAGE)

#ifdef HAVE_DIRENT_H
typedef struct _Eina_File_Iterator Eina_File_Iterator;
struct _Eina_File_Iterator
{
   Eina_Iterator iterator;

   DIR *dirp;
   int length;

   char dir[1];
};
#endif

int _eina_file_log_dom = -1;

/*
 * This complex piece of code is needed due to possible race condition.
 * The code and description of the issue can be found at :
 * http://womble.decadent.org.uk/readdir_r-advisory.html
 */
#ifdef HAVE_DIRENT_H
/**
 * @internal
 * @brief Get the maximum length of a filename in a directory.
 *
 * This function attempts to determine the maximum filename length
 * for the directory associated with @p dirp. It uses fpathconf if
 * available, otherwise falls back to NAME_MAX or PATH_MAX.
 *
 * @param dirp Pointer to the DIR structure of the directory.
 * @return The maximum filename length, or a fallback value.
 */
static long
_eina_name_max(DIR *dirp EINA_UNUSED)
{
   long name_max;

#if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) && defined(_PC_NAME_MAX)
   name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);

   if (name_max == -1)
     {
# if defined(NAME_MAX)
        name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
# else
        name_max = PATH_MAX;
# endif
     }
#else
# if defined(NAME_MAX)
   name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
# else
#  ifdef _PC_NAME_MAX
#   warning "buffer size for readdir_r cannot be determined safely, best effort, but racy"
   name_max = pathconf(dirp, _PC_NAME_MAX);
#  else
#   error "buffer size for readdir_r cannot be determined safely"
#  endif
# endif
#endif

   return name_max;
}

/**
 * @internal
 * @brief Get the next entry for the simple directory listing iterator.
 *
 * Reads the next directory entry from the iterator @p it, skipping "." and "..".
 * The full path of the entry is constructed and returned as a shared string
 * in @p data.
 *
 * @param it The file iterator.
 * @param data Pointer to store the Eina_Stringshare (char *) of the full path.
 * @return EINA_TRUE if a new entry is found, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_ls_iterator_next(Eina_File_Iterator *it, void **data)
{
   struct dirent *dp;
   char *name;
   size_t length;

   do
     {
        dp = readdir(it->dirp);
        if (dp == NULL)
          return EINA_FALSE;
     }
   while ((dp->d_name[0] == '.') &&
          ((dp->d_name[1] == '\0') ||
           ((dp->d_name[1] == '.') && (dp->d_name[2] == '\0'))));

#ifdef _DIRENT_HAVE_D_NAMLEN
   length = dp->d_namlen;
#else
   length = strlen(dp->d_name);
#endif
   name = alloca(length + 2 + it->length);

   memcpy(name,                  it->dir,    it->length);
   memcpy(name + it->length,     "/",        1);
   memcpy(name + it->length + 1, dp->d_name, length + 1);

   *data = (char *)eina_stringshare_add(name);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Get the container (DIR stream) of the simple ls iterator.
 *
 * @param it The file iterator.
 * @return The DIR stream pointer.
 */
static DIR *
_eina_file_ls_iterator_container(Eina_File_Iterator *it)
{
   return it->dirp;
}

/**
 * @internal
 * @brief Free the simple directory listing iterator.
 *
 * Closes the directory stream and frees the iterator structure.
 *
 * @param it The file iterator to free.
 */
static void
_eina_file_ls_iterator_free(Eina_File_Iterator *it)
{
   closedir(it->dirp);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

typedef struct _Eina_File_Direct_Iterator Eina_File_Direct_Iterator;
struct _Eina_File_Direct_Iterator
{
   Eina_Iterator iterator;

   DIR *dirp;
   int length;

   Eina_File_Direct_Info info;

   char dir[1]; /**< Flexible array member for the directory path. */
};

/**
 * @internal
 * @brief Get the next entry for the direct directory listing iterator.
 *
 * Reads the next directory entry from the iterator @p it, skipping "." and "..".
 * It populates an Eina_File_Direct_Info structure with path, name, and type
 * (if available from dirent). This info structure is returned via @p data.
 *
 * @param it The direct file iterator.
 * @param data Pointer to store the Eina_File_Direct_Info pointer.
 * @return EINA_TRUE if a new entry is found, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_direct_ls_iterator_next(Eina_File_Direct_Iterator *it, void **data)
{
   struct dirent *dp;
   size_t length;

   do
     {
        dp = readdir(it->dirp);
        if (dp == NULL)
          return EINA_FALSE;

#ifdef _DIRENT_HAVE_D_NAMLEN
        length = dp->d_namlen;
#else
        length = strlen(dp->d_name);
#endif
        if (it->info.name_start + length + 1 >= EINA_PATH_MAX)
           continue;
     }
   while ((dp->d_name[0] == '.') &&
          ((dp->d_name[1] == '\0') ||
           ((dp->d_name[1] == '.') && (dp->d_name[2] == '\0'))));

   memcpy(it->info.path + it->info.name_start, dp->d_name, length);
   it->info.name_length = length;
   it->info.path_length = it->info.name_start + length;
   it->info.path[it->info.path_length] = '\0';

#ifdef _DIRENT_HAVE_D_TYPE
   switch (dp->d_type)
     {
     case DT_FIFO:
       it->info.type = EINA_FILE_FIFO;
       break;
     case DT_CHR:
       it->info.type = EINA_FILE_CHR;
       break;
     case DT_DIR:
       it->info.type = EINA_FILE_DIR;
       break;
     case DT_BLK:
       it->info.type = EINA_FILE_BLK;
       break;
     case DT_REG:
       it->info.type = EINA_FILE_REG;
       break;
     case DT_LNK:
       it->info.type = EINA_FILE_LNK;
       break;
     case DT_SOCK:
       it->info.type = EINA_FILE_SOCK;
       break;
     case DT_WHT:
       it->info.type = EINA_FILE_WHT;
       break;
     default:
       it->info.type = EINA_FILE_UNKNOWN;
       break;
     }
#else
   it->info.type = EINA_FILE_UNKNOWN;
#endif

   *data = &it->info;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Get the container (DIR stream) of the direct ls iterator.
 *
 * @param it The direct file iterator.
 * @return The DIR stream pointer.
 */
static DIR *
_eina_file_direct_ls_iterator_container(Eina_File_Direct_Iterator *it)
{
   return it->dirp;
}

/**
 * @internal
 * @brief Free the direct directory listing iterator.
 *
 * Closes the directory stream and frees the iterator structure.
 *
 * @param it The direct file iterator to free.
 */
static void
_eina_file_direct_ls_iterator_free(Eina_File_Direct_Iterator *it)
{
   closedir(it->dirp);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
 * @internal
 * @brief Get the next entry for the stat directory listing iterator.
 *
 * This function behaves like _eina_file_direct_ls_iterator_next, but if
 * the file type is EINA_FILE_UNKNOWN (e.g., d_type is not available or DT_UNKNOWN),
 * it attempts to determine the file type using eina_file_statat().
 *
 * @param it The direct file iterator (reused for stat ls).
 * @param data Pointer to store the Eina_File_Direct_Info pointer.
 * @return EINA_TRUE if a new entry is found, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_stat_ls_iterator_next(Eina_File_Direct_Iterator *it, void **data)
{
   Eina_Stat st;

   if (!_eina_file_direct_ls_iterator_next(it, data))
     return EINA_FALSE;

   if (it->info.type == EINA_FILE_UNKNOWN)
     {
        if (eina_file_statat(it->dirp, &it->info, &st) != 0)
          it->info.type = EINA_FILE_UNKNOWN;
     }

   return EINA_TRUE;
}
#endif

/**
 * @internal
 * @brief Actually closes an Eina_File and cleans up its resources.
 *
 * This function is responsible for unmapping any dead memory maps,
 * unmapping the global map if it exists and is not copied, and
 * closing the file descriptor.
 *
 * @param file The Eina_File to close.
 */
void
eina_file_real_close(Eina_File *file)
{
   Eina_File_Map *map;

   EINA_LIST_FREE(file->dead_map, map)
     {
        munmap(map->map, map->length);
        free(map);
     }

   if (file->fd != -1)
     {
        if (!file->copied && file->global_map != MAP_FAILED)
          munmap(file->global_map, file->length);
        close(file->fd);
     }
}

/**
 * @internal
 * @brief Unmaps a memory region and frees the Eina_File_Map structure.
 *
 * This is typically used as a callback for hash table freeing.
 *
 * @param map The Eina_File_Map to close and free.
 */
static void
_eina_file_map_close(Eina_File_Map *map)
{
   munmap(map->map, map->length);
   free(map);
}

#ifndef MAP_POPULATE
/**
 * @internal
 * @brief Manually populates a memory-mapped region by touching pages.
 *
 * This function is a fallback for systems that do not define MAP_POPULATE.
 * It iterates through the memory region, accessing bytes at page intervals
 * to hint the kernel to load these pages into memory.
 *
 * @param map The memory-mapped region.
 * @param size The size of the region to populate.
 * @param hugetlb EINA_TRUE if HugeTLB pages are used, EINA_FALSE otherwise.
 *                This determines the step size (EINA_HUGE_PAGE or EINA_SMALL_PAGE).
 * @return An unsigned integer derived from XORing accessed bytes, mostly to
 *         prevent the compiler from optimizing out the memory accesses.
 */
static unsigned int
_eina_file_map_populate(char *map, unsigned long int size, Eina_Bool hugetlb)
{
   unsigned int r = 0xDEADBEEF;
   unsigned long int i;
   unsigned int s;

   if (size == 0) return 0;

   s = hugetlb ? EINA_HUGE_PAGE : EINA_SMALL_PAGE;

   for (i = 0; i < size; i += s)
     r ^= map[i];

   r ^= map[size - 1];

   return r;
}
#endif

/**
 * @internal
 * @brief Calculates a page-aligned address within a given memory map.
 *
 * Given a base @p map address and an @p offset, this function returns
 * the address of the start of the page containing `map + offset`.
 * The page size used for alignment depends on @p hugetlb.
 *
 * @param map The base address of the memory map.
 * @param offset The offset within the map.
 * @param hugetlb EINA_TRUE if HugeTLB page alignment is required,
 *                EINA_FALSE for regular page alignment.
 * @return The page-aligned address.
 */
static char *
_page_aligned_address(const char *map, unsigned long int offset, Eina_Bool hugetlb)
{
   const uintptr_t align = hugetlb ? EINA_HUGE_PAGE : EINA_SMALL_PAGE;
   uintptr_t pmap = (uintptr_t) map;

   pmap = (pmap + offset) - ((pmap + offset) & (align - 1));

   return (char *) pmap;
}

/**
 * @internal
 * @brief Applies a memory advice rule to a specified region of a memory map.
 *
 * This function uses madvise() to apply hints like MADV_RANDOM, MADV_SEQUENTIAL,
 * MADV_WILLNEED, etc. For EINA_FILE_POPULATE, it may also call
 * _eina_file_map_populate if MAP_POPULATE is not defined.
 * The address and size are adjusted to be page-aligned.
 *
 * @param rule The Eina_File_Populate rule to apply.
 * @param map The base address of the memory map.
 * @param offset The starting offset within the map for the rule.
 * @param size The length of the region to apply the rule to. If 0, it might imply
 *             the entire map from the aligned offset.
 * @param maplen The total length of the original memory map.
 * @param hugetlb EINA_TRUE if HugeTLB pages are involved, EINA_FALSE otherwise.
 * @return An integer, typically 42 or 42 XORed with a value from
 *         _eina_file_map_populate. Its specific value is not critical.
 */
static int
_eina_file_map_rule_apply(Eina_File_Populate rule, const void *map, unsigned long int offset,
                          unsigned long int size, unsigned long long maplen, Eina_Bool hugetlb)
{
   int tmp = 42;
   int flag = MADV_RANDOM;
   char *addr;

   switch (rule)
     {
      case EINA_FILE_RANDOM: flag = MADV_RANDOM; break;
      case EINA_FILE_SEQUENTIAL: flag = MADV_SEQUENTIAL; break;
      case EINA_FILE_POPULATE: flag = MADV_WILLNEED; break;
      case EINA_FILE_WILLNEED: flag = MADV_WILLNEED; break;
      case EINA_FILE_DONTNEED: flag = MADV_DONTNEED; break;
#ifdef MADV_REMOVE
      case EINA_FILE_REMOVE: flag = MADV_REMOVE; break;
#elif defined (MADV_FREE)
      case EINA_FILE_REMOVE: flag = MADV_FREE; break;
#else
# warning "EINA_FILE_REMOVE does not have system support"
#endif
      default: return tmp; break;
     }

   if (offset >= maplen) return tmp;

   // Align address, clamp size
   addr = _page_aligned_address(map, offset, hugetlb);
   if (size > 0)
     {
        size += ((char *) map + offset) - addr;
        offset -= ((char *) map + offset) - addr;
        if ((offset + size) > maplen)
          {
             if (offset > maplen) return tmp;
             size = maplen - offset;
          }
     }

   madvise(addr, size, flag);

#ifndef MAP_POPULATE
   if (rule == EINA_FILE_POPULATE)
     tmp ^= _eina_file_map_populate(addr, size, hugetlb);
#else
   (void) hugetlb;
#endif

   return tmp;
}

/**
 * @internal
 * @brief Compares cached file metadata with fresh stat information.
 *
 * Checks if the modification time, size, inode number, and nanosecond
 * modification time (if available) of a cached Eina_File @p f match
 * the values in a new `struct stat` @p st.
 *
 * @param f The cached Eina_File structure.
 * @param st The `struct stat` containing fresh file metadata.
 * @return EINA_TRUE if all compared fields match, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_timestamp_compare(Eina_File *f, struct stat *st)
{
   if (f->mtime != st->st_mtime) return EINA_FALSE;
   if (f->length != (unsigned long long) st->st_size) return EINA_FALSE;
   if (f->inode != st->st_ino) return EINA_FALSE;
#ifdef _STAT_VER_LINUX
# ifdef st_mtime
   if (f->mtime_nsec != (unsigned long int)st->st_mtim.tv_nsec)
     return EINA_FALSE;
# else
   if (f->mtime_nsec != (unsigned long int)st->st_mtimensec)
     return EINA_FALSE;
# endif
#endif
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Safe snprintf wrapper ensuring null-termination.
 *
 * This function calls vsnprintf and then explicitly null-terminates
 * the buffer at `str[size - 1]` to guarantee null-termination
 * even if vsnprintf truncates.
 *
 * @param str The buffer to write to.
 * @param size The size of the buffer @p str.
 * @param format The format string.
 * @param ... Variable arguments for the format string.
 */
static void
slprintf(char *str, size_t size, const char *format, ...)
{
   va_list ap;

   va_start(ap, format);

   vsnprintf(str, size, format, ap);
   str[size - 1] = 0;

   va_end(ap);
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @internal
 * @brief Checks if a given memory page range overlaps with a specific Eina_File_Map.
 *
 * If an overlap is detected, the Eina_File_Map @p m is marked as faulty.
 * This is used to detect if a SIGBUS/SIGSEGV on a mapped address might be
 * due to issues with this specific file mapping (e.g., file truncated).
 *
 * @param addr The starting address of the memory page that faulted.
 * @param page_size The size of the memory page.
 * @param m The Eina_File_Map to check against.
 * @return EINA_TRUE if the address range overlaps with the map and it was marked faulty,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_file_mmap_faulty_one(void *addr, long page_size,
                           Eina_File_Map *m)
{
   if ((unsigned char *) addr < (((unsigned char *)m->map) + m->length) &&
       (((unsigned char *) addr) + page_size) >= (unsigned char *) m->map)
     {
        m->faulty = EINA_TRUE;
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Marks file maps as faulty if a given memory address falls within them.
 *
 * This function is typically called from a signal handler (e.g., for SIGBUS or
 * SIGSEGV) to determine if the faulting @p addr is within any known memory-mapped
 * file regions managed by Eina_File. If a map contains the @p addr, it's
 * marked as faulty.
 *
 * @param addr The faulting memory address.
 * @param page_size The system's page size, relevant for the fault range.
 * @return EINA_TRUE if any Eina_File_Map (global or specific) was found to
 *         contain the address and was marked faulty, EINA_FALSE otherwise.
 * @note This function iterates through a global cache of Eina_File objects
 *       and their associated maps. It involves taking locks.
 */
Eina_Bool
eina_file_mmap_faulty(void *addr, long page_size)
{
   Eina_File_Map *m;
   Eina_File *f;
   Eina_Iterator *itf;
   Eina_Iterator *itm;
   Eina_Bool faulty = EINA_FALSE;

   eina_lock_take(&_eina_file_lock_cache);

   itf = eina_hash_iterator_data_new(_eina_file_cache);
   EINA_ITERATOR_FOREACH(itf, f)
     {
        eina_lock_take(&f->lock);

        if (f->global_map != MAP_FAILED)
          {
             if ((unsigned char *)addr <
                 (((unsigned char *)f->global_map) + f->length) &&
                 (((unsigned char *)addr) + page_size) >=
                  (unsigned char *)f->global_map)
               {
                  f->global_faulty = EINA_TRUE;
                  faulty = EINA_TRUE;
               }
          }

        if (!faulty)
          {
             itm = eina_hash_iterator_data_new(f->map);
             EINA_ITERATOR_FOREACH(itm, m)
               {
                  faulty = _eina_file_mmap_faulty_one(addr, page_size, m);
                  if (faulty) break;
               }
             eina_iterator_free(itm);
          }

        if (!faulty)
          {
             Eina_List *l;

             EINA_LIST_FOREACH(f->dead_map, l, m)
               {
                  faulty = _eina_file_mmap_faulty_one(addr, page_size, m);
                  if (faulty) break;
               }
          }

        eina_lock_release(&f->lock);

        if (faulty) break;
     }
   eina_iterator_free(itf);

   eina_lock_release(&_eina_file_lock_cache);
   return faulty;
}

/* ================================================================ *
 *   Simplified logic for portability layer with eina_file_common   *
 * ================================================================ */

/**
 * @internal
 * @brief Prepends the current working directory to a relative path.
 *
 * This function is part of the eina_file_common.h abstraction.
 * It gets the current working directory, concatenates it with the
 * provided @p path, and returns the result as an Eina_Tmpstr.
 * The @p len parameter seems to be an initial length for @p path,
 * which is then augmented by the CWD's length.
 *
 * @param path The relative path component.
 * @param len The initial length of @p path (before CWD concatenation).
 * @return An Eina_Tmpstr containing the absolute path, or NULL on error.
 * @note The memory for the path is allocated on the stack using alloca.
 */
Eina_Tmpstr *
eina_file_current_directory_get(const char *path, size_t len)
{
  char cwd[PATH_MAX];
  char *tmp = NULL;

  tmp = getcwd(cwd, PATH_MAX);
  if (!tmp) return NULL;

  len += strlen(cwd) + 2; // +1 for '/', +1 for '\0'
  tmp = alloca(sizeof (char) * len);

  slprintf(tmp, len, "%s/%s", cwd, path);

  return eina_tmpstr_add_length(tmp, len);
}

/**
 * @internal
 * @brief Converts an Eina_Tmpstr to a duplicated string and frees the Tmpstr.
 *
 * This function is part of the eina_file_common.h abstraction.
 * It duplicates the content of @p path (if not NULL) into a new
 * heap-allocated string and then deletes the @p path.
 *
 * @param path The Eina_Tmpstr to convert and delete.
 * @return A newly allocated string with the contents of @p path,
 *         or an empty string if @p path was NULL. The caller must free this.
 */
char *
eina_file_cleanup(Eina_Tmpstr *path)
{
   char *result;

   result = strdup(path ? path : "");
   eina_tmpstr_del(path);

   return result;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/



/**
 * @brief Checks if a given file path is relative.
 *
 * A path is considered relative if it does not start with the
 * directory separator character ('/').
 *
 * @param path The file path to check.
 * @return EINA_TRUE if the path is relative, EINA_FALSE otherwise (e.g., if absolute, NULL, or empty).
 */
EINA_API Eina_Bool
eina_file_path_relative(const char *path)
{
   if (!path)
     return EINA_FALSE;

   return *path != '/';
}

/**
 * @brief Lists files and directories within a given directory.
 *
 * Iterates over the entries in the directory @p dir. For each entry,
 * the callback function @p cb is invoked. If @p recursive is EINA_TRUE,
 * the function will recurse into subdirectories.
 *
 * @param dir The path to the directory to list.
 * @param recursive If EINA_TRUE, list recursively. Otherwise, list only
 *        the immediate contents of @p dir.
 * @param cb The callback function to call for each entry.
 *           The first argument to the callback is the entry name (char *).
 *           The second argument is the base directory path (const char *dir).
 *           The third argument is the user-provided @p data.
 * @param data User data to be passed to the callback function @p cb.
 * @return EINA_TRUE on success or if the directory is empty/could be opened,
 *         EINA_FALSE if @p dir is NULL, empty, or cannot be opened, or if @p cb is NULL.
 */
EINA_API Eina_Bool
eina_file_dir_list(const char *dir,
                   Eina_Bool recursive,
                   Eina_File_Dir_List_Cb cb,
                   void *data)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cb,  EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(dir[0] == '\0', EINA_FALSE);

   it = eina_file_stat_ls(dir);
   if (!it)
      return EINA_FALSE;

   EINA_ITERATOR_FOREACH(it, info)
     {
        cb(info->path + info->name_start, dir, data);

        if (recursive == EINA_TRUE && info->type == EINA_FILE_DIR)
          {
             eina_file_dir_list(info->path, recursive, cb, data);
          }
     }

   eina_iterator_free(it);

   return EINA_TRUE;
}

/**
 * @brief Splits a file path into its components.
 *
 * The path is tokenized by the PATH_DELIM character (usually '/').
 * The original @p path string is modified in place (null bytes are inserted
 * to terminate components). The components are added as (char *) to the
 * returned Eina_Array.
 *
 * @param path The file path string to split. This string will be modified.
 * @return A new Eina_Array containing (char *) pointers to the components
 *         of the path. Returns NULL if @p path is NULL or if array allocation fails.
 *         The strings in the array point into the modified @p path string.
 *         Example: For path "/usr/local/bin", the array would contain
 *         pointers to "usr", "local", "bin". (Initial empty component from "/"
 *         is skipped).
 *
 * @note The Eina_Array should be freed using eina_array_free() when no longer needed.
 *       The string data itself is part of the original @p path.
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

   for (current = strchr(path, PATH_DELIM);
        current;
        path = current + 1, current = strchr(path, PATH_DELIM))
     {
        length = current - path;

        if (length == 0)
           continue;

        eina_array_push(ea, path);
        *current = '\0';
     }

   if (*path != '\0')
        eina_array_push(ea, path);

   return ea;
}

/**
 * @brief Creates an iterator to list entries in a directory.
 *
 * This function provides a simple way to iterate over the names of files
 * and directories within @p dir. It skips "." and ".." entries.
 * The iterator returns full paths as shared strings (Eina_Stringshare).
 *
 * @param dir The path to the directory.
 * @return A new Eina_Iterator on success, or NULL on failure (e.g., @p dir
 *         is NULL, empty, or cannot be opened).
 *         The iterator should be freed using eina_iterator_free().
 *         Data from iterator: (char *) eina_stringshare_add("full/path/to/entry")
 */
EINA_API Eina_Iterator *
eina_file_ls(const char *dir)
{
#ifdef HAVE_DIRENT_H
   Eina_File_Iterator *it;
   size_t length;
   DIR *dirp;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);

   length = strlen(dir);
   if (length < 1)
      return NULL;

   dirp = opendir(dir);
   if (!dirp)
      return NULL;

   it = calloc(1, sizeof (Eina_File_Iterator) + length);
   if (EINA_UNLIKELY(!it))
     {
        closedir(dirp);
        return NULL;
     }

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->dirp = dirp;

   memcpy(it->dir, dir, length + 1);
   if (dir[length - 1] != '/')
      it->length = length;
   else
      it->length = length - 1;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_ls_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
         _eina_file_ls_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_ls_iterator_free);

   return &it->iterator;
#else
   (void) dir;
   return NULL;
#endif
}

/**
 * @brief Creates an iterator for direct listing of directory entries with more info.
 *
 * This iterator provides Eina_File_Direct_Info structures for each entry.
 * This structure includes the full path, the name of the entry, and potentially
 * the file type (Eina_File_Type) if available directly from the `dirent`
 * structure (e.g., `d_type` field). It skips "." and ".." entries.
 *
 * @param dir The path to the directory.
 * @return A new Eina_Iterator on success, or NULL on failure (e.g., @p dir
 *         is NULL, empty, cannot be opened, or path length exceeds limits).
 *         The iterator should be freed using eina_iterator_free().
 *         Data from iterator: (Eina_File_Direct_Info *)
 *         The Eina_File_Direct_Info structure is valid only until the next
 *         iterator call or iterator free.
 */
EINA_API Eina_Iterator *
eina_file_direct_ls(const char *dir)
{
#ifdef HAVE_DIRENT_H
   Eina_File_Direct_Iterator *it;
   size_t length;
   DIR *dirp;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);

   length = strlen(dir);
   if (length < 1)
      return NULL;

   dirp = opendir(dir);
   if (!dirp)
      return NULL;

   it = calloc(1, sizeof(Eina_File_Direct_Iterator) + length);
   if (EINA_UNLIKELY(!it))
     {
        closedir(dirp);
        return NULL;
     }

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->dirp = dirp;

   if (length + _eina_name_max(it->dirp) + 2 >= EINA_PATH_MAX)
     {
        _eina_file_direct_ls_iterator_free(it);
        return NULL;
     }

   memcpy(it->dir,       dir, length + 1);
   it->length = length;

   memcpy(it->info.path, dir, length);
   if (dir[length - 1] == '/')
      it->info.name_start = length;
   else
     {
        it->info.path[length] = '/';
        it->info.name_start = length + 1;
     }

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_direct_ls_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
         _eina_file_direct_ls_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_direct_ls_iterator_free);

   return &it->iterator;
#else
   (void) dir;
   return NULL;
#endif
}

/**
 * @brief Creates an iterator for listing directory entries with stat-resolved types.
 *
 * This iterator is similar to eina_file_direct_ls(), providing
 * Eina_File_Direct_Info structures. However, if the file type cannot be
 * determined from the `dirent` structure (i.e., it's EINA_FILE_UNKNOWN),
 * this function will attempt to use `stat` (or `fstatat`) to determine the
 * file type. It skips "." and ".." entries.
 *
 * @param dir The path to the directory.
 * @return A new Eina_Iterator on success, or NULL on failure (e.g., @p dir
 *         is NULL, empty, cannot be opened, or path length exceeds limits).
 *         The iterator should be freed using eina_iterator_free().
 *         Data from iterator: (Eina_File_Direct_Info *)
 *         The Eina_File_Direct_Info structure is valid only until the next
 *         iterator call or iterator free.
 */
EINA_API Eina_Iterator *
eina_file_stat_ls(const char *dir)
{
#ifdef HAVE_DIRENT_H
   Eina_File_Direct_Iterator *it;
   size_t length;
   DIR *dirp;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);

   length = strlen(dir);
   if (length < 1)
      return NULL;

   dirp = opendir(dir);
   if (!dirp)
      return NULL;

   it = calloc(1, sizeof(Eina_File_Direct_Iterator) + length);
   if (EINA_UNLIKELY(!it))
     {
        closedir(dirp);
        return NULL;
     }

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->dirp = dirp;

   if (length + _eina_name_max(it->dirp) + 2 >= EINA_PATH_MAX)
     {
        _eina_file_direct_ls_iterator_free(it);
        return NULL;
     }

   memcpy(it->dir,       dir, length + 1);
   it->length = length;

   memcpy(it->info.path, dir, length);
   if (dir[length - 1] == '/')
      it->info.name_start = length;
   else
     {
        it->info.path[length] = '/';
        it->info.name_start = length + 1;
     }

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_stat_ls_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
         _eina_file_direct_ls_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_direct_ls_iterator_free);

   return &it->iterator;
#else
   (void) dir;
   return NULL;
#endif
}

/**
 * @brief Opens a file and returns an Eina_File handle.
 *
 * This function opens the file specified by @p path. It uses a caching
 * mechanism: if the same file (identified by its sanitized path) is already
 * open and its metadata (timestamp, size, inode) hasn't changed, the existing
 * Eina_File handle is returned with an incremented reference count.
 * Otherwise, a new Eina_File structure is created.
 *
 * If @p shared is EINA_TRUE, it attempts to open the file using `shm_open`
 * for shared memory access (if available and applicable). Otherwise, a
 * regular `open` is used.
 *
 * @param path The path to the file.
 * @param shared If EINA_TRUE, try to open for shared access (e.g. shm_open).
 * @return A pointer to an Eina_File structure on success, or NULL on failure.
 *         The returned Eina_File should be closed with eina_file_close()
 *         when no longer needed.
 *
 * @note The path is sanitized using eina_file_sanitize() before use.
 * @see eina_file_close()
 * @see eina_file_sanitize()
 * @see eina_file_refresh()
 */
EINA_API Eina_File *
eina_file_open(const char *path, Eina_Bool shared)
{
   Eina_File *file;
   Eina_File *n;
   Eina_Stringshare *filename;
   struct stat file_stat;
   int fd = -1;
   Eina_Statgen statgen;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);

   filename = eina_file_sanitize(path);
   if (!filename) return NULL;

   statgen = eina_file_statgen_get();
   eina_lock_take(&_eina_file_lock_cache);
   file = eina_hash_find(_eina_file_cache, filename);
   statgen = eina_file_statgen_get();
   if ((!file) || (file->statgen != statgen) || (statgen == 0))
     {
        if (shared)
          {
#ifdef HAVE_SHM_OPEN
             fd = shm_open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
             if ((fd != -1)  && (!eina_file_close_on_exec(fd, EINA_TRUE)))
               goto on_error;
#else
             goto on_error;
#endif
          }
        else
          {
#ifdef HAVE_OPEN_CLOEXEC
             fd = open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO | O_CLOEXEC);
#else
             fd = open(filename, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO);
             if ((fd != -1)  && (!eina_file_close_on_exec(fd, EINA_TRUE)))
               goto on_error;
#endif
          }
        if (fd < 0) goto on_error;

        if (fstat(fd, &file_stat))
          goto on_error;
        if (file) file->statgen = statgen;

        if ((file) && !_eina_file_timestamp_compare(file, &file_stat))
          {
             file->delete_me = EINA_TRUE;
             eina_hash_del(_eina_file_cache, file->filename, file);
             file = NULL;
          }
     }

   if (!file)
     {
        n = malloc(sizeof(Eina_File));
        if (!n)
          goto on_error;

        memset(n, 0, sizeof(Eina_File));
        n->filename = filename;
        n->map = eina_hash_new(EINA_KEY_LENGTH(eina_file_map_key_length),
                               EINA_KEY_CMP(eina_file_map_key_cmp),
                               EINA_KEY_HASH(eina_file_map_key_hash),
                               EINA_FREE_CB(_eina_file_map_close),
                               3);
        n->rmap = eina_hash_pointer_new(NULL);
        n->global_map = MAP_FAILED;
        n->length = file_stat.st_size;
        n->mtime = file_stat.st_mtime;
#ifdef _STAT_VER_LINUX
        n->mtime_nsec = (unsigned long int)file_stat.st_mtim.tv_nsec;
#endif
        n->inode = file_stat.st_ino;
        n->fd = fd;
        n->shared = shared;
        eina_lock_new(&n->lock);
        eina_hash_direct_add(_eina_file_cache, n->filename, n);

	EINA_MAGIC_SET(n, EINA_FILE_MAGIC);
     }
   else
     {
        if (fd >= 0) close(fd);
        n = file;
     }
   eina_lock_take(&n->lock);
   n->refcount++;
   eina_lock_release(&n->lock);

   eina_lock_release(&_eina_file_lock_cache);

   return n;

 on_error:
   eina_lock_release(&_eina_file_lock_cache);
   INF("Could not open file [%s].", filename);
   eina_stringshare_del(filename);

   if (fd >= 0) close(fd);
   return NULL;
}

/**
 * @brief Refreshes the cached metadata of an opened file.
 *
 * This function performs an `fstat` on the file descriptor associated with
 * @p file to get the latest metadata (size, modification time, inode).
 * If the size has changed, it calls eina_file_flush() to handle potential
 * changes in mappings. The cached metadata in @p file is then updated.
 *
 * @param file The Eina_File handle to refresh.
 * @return EINA_TRUE if the file size changed (and thus flush was called),
 *         EINA_FALSE otherwise or on error (e.g., fstat fails, file is virtual).
 * @note This function is not applicable to virtual files.
 */
EINA_API Eina_Bool
eina_file_refresh(Eina_File *file)
{
   struct stat file_stat;
   Eina_Bool r = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);

   if (file->virtual) return EINA_FALSE;

   if (fstat(file->fd, &file_stat))
     return EINA_FALSE;

   if (file->length != (unsigned long int) file_stat.st_size)
     {
        eina_file_flush(file, file_stat.st_size);
        r = EINA_TRUE;
     }

   file->length = file_stat.st_size;
   file->mtime = file_stat.st_mtime;
#ifdef _STAT_VER_LINUX
   file->mtime_nsec = (unsigned long int)file_stat.st_mtim.tv_nsec;
#endif
   file->inode = file_stat.st_ino;

   return r;
}

/**
 * @brief Deletes a name from the filesystem.
 *
 * This function is a wrapper around the `unlink` system call.
 * If @p pathname is the last link to a file and no processes have the file open,
 * the file is deleted and the space it was using is made available for reuse.
 *
 * @param pathname The path to the file or symbolic link to delete.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., file does not exist,
 *         permission denied).
 */
EINA_API Eina_Bool
eina_file_unlink(const char *pathname)
{
   if ( unlink(pathname) < 0)
     {
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Maps an entire file into memory.
 *
 * This function memory-maps the whole file associated with the @p file handle.
 * The mapping is read-only and shared (MAP_SHARED).
 * It may attempt to use `MAP_POPULATE` if @p rule is EINA_FILE_POPULATE and
 * the system supports it. It may also attempt to use `MAP_HUGETLB` if the
 * file size is large enough (>= EINA_HUGE_PAGE_MIN).
 *
 * If the file is already globally mapped, this function increments a reference
 * count for the global mapping and returns the existing map address.
 *
 * @param file The Eina_File handle of the opened file.
 * @param rule A population hint (e.g., EINA_FILE_POPULATE, EINA_FILE_SEQUENTIAL).
 *             See Eina_File_Populate enum.
 * @return A pointer to the memory-mapped region on success, or NULL on failure.
 *         The memory should be unmapped using eina_file_map_free() when no
 *         longer needed.
 * @note For virtual files, eina_file_virtual_map_all() is called.
 * @see eina_file_map_free()
 * @see Eina_File_Populate
 */
EINA_API void *
eina_file_map_all(Eina_File *file, Eina_File_Populate rule)
{
   int flags = MAP_SHARED;
   void *ret = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (file->virtual) return eina_file_virtual_map_all(file);

   // bsd people will lack this feature
#ifdef MAP_POPULATE
   if (rule == EINA_FILE_POPULATE) flags |= MAP_POPULATE;
#endif
#ifdef MAP_HUGETLB
   if (file->length >= EINA_HUGE_PAGE_MIN) flags |= MAP_HUGETLB;
#endif

   eina_mmap_safety_enabled_set(EINA_TRUE);
   eina_lock_take(&file->lock);
   if (file->global_map == MAP_FAILED)
     file->global_map = mmap(NULL, file->length, PROT_READ, flags, file->fd, 0);
#ifdef MAP_HUGETLB
   if ((file->global_map == MAP_FAILED) && (flags & MAP_HUGETLB))
     {
       flags &= ~MAP_HUGETLB;
       file->global_map = mmap(NULL, file->length, PROT_READ, flags, file->fd, 0);
     }
#endif

   if (file->global_map != MAP_FAILED)
     {
        Eina_Bool hugetlb = EINA_FALSE;

#ifdef MAP_HUGETLB
        hugetlb = !!(flags & MAP_HUGETLB);
#endif
        if (!file->global_refcount)
          file->global_hugetlb = hugetlb;
        else
          hugetlb = file->global_hugetlb;

        _eina_file_map_rule_apply(rule, file->global_map, 0, file->length, file->length, hugetlb);
        file->global_refcount++;
        ret = file->global_map;
     }

   eina_lock_release(&file->lock);
   return ret;
}

/**
 * @brief Maps a specific region of a file into memory.
 *
 * This function memory-maps a portion of the file associated with @p file,
 * starting at @p offset and extending for @p length bytes.
 * The mapping is read-only and shared (MAP_SHARED).
 *
 * If @p offset is 0 and @p length equals the file length, this function
 * behaves like eina_file_map_all().
 *
 * Mappings are cached. If an identical mapping (same offset and length)
 * already exists, its reference count is incremented and the existing
 * map address is returned.
 *
 * It may attempt to use `MAP_POPULATE` if @p rule is EINA_FILE_POPULATE and
 * the system supports it. It may also attempt to use `MAP_HUGETLB` if the
 * mapping length is large enough (>= EINA_HUGE_PAGE_MIN).
 *
 * @param file The Eina_File handle of the opened file.
 * @param rule A population hint (e.g., EINA_FILE_POPULATE, EINA_FILE_SEQUENTIAL).
 *             See Eina_File_Populate enum.
 * @param offset The starting offset within the file to map.
 * @param length The number of bytes to map.
 * @return A pointer to the memory-mapped region on success, or NULL on failure
 *         (e.g., offset/length out of bounds, mmap fails).
 *         The memory should be unmapped using eina_file_map_free() when no
 *         longer needed.
 * @note For virtual files, eina_file_virtual_map_new() is called.
 * @see eina_file_map_free()
 * @see eina_file_map_all()
 * @see Eina_File_Populate
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

   if (offset == 0 && length == file->length)
     return eina_file_map_all(file, rule);

   if (file->virtual)
     return eina_file_virtual_map_new(file, offset, length);

   key[0] = offset;
   key[1] = length;

   eina_mmap_safety_enabled_set(EINA_TRUE);
   eina_lock_take(&file->lock);

   map = eina_hash_find(file->map, &key);
   if (!map)
     {
        int flags = MAP_SHARED;

// bsd people will lack this feature
#ifdef MAP_POPULATE
        if (rule == EINA_FILE_POPULATE) flags |= MAP_POPULATE;
#endif
#ifdef MAP_HUGETLB
        if (length >= EINA_HUGE_PAGE_MIN) flags |= MAP_HUGETLB;
#endif

        map = malloc(sizeof (Eina_File_Map));
        if (!map) goto on_error;

        map->map = mmap(NULL, length, PROT_READ, flags, file->fd, offset);
#ifdef MAP_HUGETLB
        if (map->map == MAP_FAILED && (flags & MAP_HUGETLB))
          {
             flags &= ~MAP_HUGETLB;
             map->map = mmap(NULL, length, PROT_READ, flags, file->fd, offset);
          }

        map->hugetlb = !!(flags & MAP_HUGETLB);
#else
        map->hugetlb = EINA_FALSE;
#endif
        map->offset = offset;
        map->length = length;
        map->refcount = 0;

        if (map->map == MAP_FAILED) goto on_error;

        eina_hash_add(file->map, &key, map);
        eina_hash_direct_add(file->rmap, &map->map, map);
     }

   map->refcount++;

   _eina_file_map_rule_apply(rule, map->map, 0, length, map->length, map->hugetlb);

   eina_lock_release(&file->lock);

   return map->map;

 on_error:
   free(map);
   eina_lock_release(&file->lock);

   return NULL;
}

/**
 * @brief Unmaps a memory-mapped region of a file.
 *
 * This function decrements the reference count of the given @p map. If the
 * reference count drops to zero, the memory region is unmapped using `munmap`.
 * This applies to both globally mapped regions (from eina_file_map_all())
 * and specific regions (from eina_file_map_new()).
 *
 * @param file The Eina_File handle associated with the map.
 * @param map The pointer to the memory-mapped region to free (returned by
 *            eina_file_map_all() or eina_file_map_new()).
 * @note For virtual files, eina_file_virtual_map_free() is called.
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

        munmap(file->global_map, file->length);
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
 * @brief Applies a population rule to an already mapped region of a file.
 *
 * This function allows applying or changing memory advice (like pre-faulting
 * pages or hinting access patterns) for a sub-region (@p offset, @p length)
 * of an existing memory map @p map.
 *
 * @param file The Eina_File handle associated with the map.
 * @param rule The Eina_File_Populate rule to apply (e.g., EINA_FILE_POPULATE,
 *             EINA_FILE_SEQUENTIAL, EINA_FILE_DONTNEED).
 * @param map The pointer to the existing memory-mapped region.
 * @param offset The starting offset *within the given map* (not file offset)
 *               to apply the rule.
 * @param length The number of bytes from @p offset within @p map to apply the rule to.
 * @see Eina_File_Populate
 * @see _eina_file_map_rule_apply()
 */
EINA_API void
eina_file_map_populate(Eina_File *file, Eina_File_Populate rule, const void *map,
                       unsigned long int offset, unsigned long int length)
{
   Eina_File_Map *em;

   EINA_SAFETY_ON_NULL_RETURN(file);
   eina_lock_take(&file->lock);
   if (map == file->global_map)
     _eina_file_map_rule_apply(rule, map, offset, length, file->length, file->global_hugetlb);
   else if ((em = eina_hash_find(file->rmap, &map)) != NULL)
     _eina_file_map_rule_apply(rule, map, offset, length, em->length, em->hugetlb);
   eina_lock_release(&file->lock);
}

/**
 * @brief Checks if a given memory-mapped region has been marked as faulty.
 *
 * A map can be marked faulty, for example, by eina_file_mmap_faulty() if a
 * memory access violation occurs within its range, potentially indicating
 * that the underlying file has changed (e.g., truncated).
 *
 * @param file The Eina_File handle associated with the map.
 * @param map The pointer to the memory-mapped region to check.
 * @return EINA_TRUE if the map is marked as faulty, EINA_FALSE otherwise or
 *         if the map is not found or @p file is NULL.
 * @note This function is not applicable to virtual files (always returns EINA_FALSE).
 * @see eina_file_mmap_faulty()
 */
EINA_API Eina_Bool
eina_file_map_faulted(Eina_File *file, void *map)
{
   Eina_Bool r = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(file, EINA_FALSE);

   if (file->virtual) return EINA_FALSE;

   eina_lock_take(&file->lock);

   if (file->global_map == map)
     {
        r = file->global_faulty;
     }
   else
     {
        Eina_File_Map *em;

        em = eina_hash_find(file->rmap, &map);
        if (em)
          {
             r = em->faulty;
          }
        else
          {
             Eina_List *l;

             EINA_LIST_FOREACH(file->dead_map, l, em)
               if (em->map == map)
                 {
                    r = em->faulty;
                    break;
                 }
          }
     }

   eina_lock_release(&file->lock);

   return r;
}

/**
 * @brief Gets an iterator for the extended attribute names of a file.
 *
 * This function creates an iterator that yields the names of all extended
 * attributes associated with the opened file @p file.
 *
 * @param file The Eina_File handle.
 * @return A new Eina_Iterator that yields (char *) Eina_Stringshare names of
 *         extended attributes, or NULL if @p file is NULL, virtual, or an
 *         error occurs.
 *         The iterator should be freed using eina_iterator_free().
 *         The stringshare names should be deleted with eina_stringshare_del()
 *         when no longer needed if taken from the iterator.
 * @note Not applicable to virtual files.
 * @see eina_xattr_fd_ls()
 */
EINA_API Eina_Iterator *
eina_file_xattr_get(Eina_File *file)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (file->virtual) return NULL;

   return eina_xattr_fd_ls(file->fd);
}

/**
 * @brief Gets an iterator for the extended attribute names and values of a file.
 *
 * This function creates an iterator that yields Eina_Xattr_Actual_Value
 * structures, each containing an extended attribute's name and its value.
 *
 * @param file The Eina_File handle.
 * @return A new Eina_Iterator that yields (Eina_Xattr_Actual_Value *)
 *         structures, or NULL if @p file is NULL, virtual, or an error occurs.
 *         The iterator should be freed using eina_iterator_free().
 *         The Eina_Xattr_Actual_Value structure and its contents (name, value)
 *         are valid until the next iterator call or iterator free.
 * @note Not applicable to virtual files.
 * @see eina_xattr_value_fd_ls()
 * @struct Eina_Xattr_Actual_Value
 * @brief Structure holding an extended attribute name and its value.
 * @var Eina_Xattr_Actual_Value::name
 * Member 'name' contains the stringshared name of the attribute.
 * @var Eina_Xattr_Actual_Value::value
 * Member 'value' contains the attribute's value as a void pointer.
 * @var Eina_Xattr_Actual_Value::value_len
 * Member 'value_len' contains the length of the value.
 */
EINA_API Eina_Iterator *
eina_file_xattr_value_get(Eina_File *file)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(file, NULL);

   if (file->virtual) return NULL;

   return eina_xattr_value_fd_ls(file->fd);
}

/**
 * @brief Retrieves file status information, similar to fstatat or lstat.
 *
 * This function populates the Eina_Stat structure @p st with metadata for
 * the file described by @p info. If `HAVE_ATFILE_SOURCE` is defined and
 * @p container is a valid DIR*, `fstatat` is used with the directory file
 * descriptor and the relative name from `info->path + info->name_start`.
 * Otherwise, `stat` (which follows symlinks) is called on `info->path`.
 *
 * If `info->type` is EINA_FILE_UNKNOWN, this function attempts to determine
 * the file type based on the `st_mode` field from the stat buffer and updates
 * `info->type`.
 *
 * @param container A pointer to a DIR stream (used for `dirfd` if `fstatat` is available)
 *                  or NULL/ignored if `fstatat` is not used.
 * @param info Pointer to an Eina_File_Direct_Info structure describing the file.
 *             Its `type` field may be updated.
 * @param st Pointer to an Eina_Stat structure to be filled with file metadata.
 * @return 0 on success, -1 on failure (e.g., stat call fails).
 *
 * @struct Eina_Stat
 * @brief Structure to hold file status information (portable version of struct stat).
 * @var Eina_Stat::dev
 * ID of device containing file.
 * @var Eina_Stat::ino
 * Inode number.
 * @var Eina_Stat::mode
 * File type and mode.
 * @var Eina_Stat::nlink
 * Number of hard links.
 * @var Eina_Stat::uid
 * User ID of owner.
 * @var Eina_Stat::gid
 * Group ID of owner.
 * @var Eina_Stat::rdev
 * Device ID (if special file).
 * @var Eina_Stat::size
 * Total size, in bytes.
 * @var Eina_Stat::blksize
 * Block size for filesystem I/O.
 * @var Eina_Stat::blocks
 * Number of 512B blocks allocated.
 * @var Eina_Stat::atime
 * Time of last access (seconds).
 * @var Eina_Stat::mtime
 * Time of last modification (seconds).
 * @var Eina_Stat::ctime
 * Time of last status change (seconds).
 * @var Eina_Stat::atimensec
 * Time of last access (nanoseconds part).
 * @var Eina_Stat::mtimensec
 * Time of last modification (nanoseconds part).
 * @var Eina_Stat::ctimensec
 * Time of last status change (nanoseconds part).
 */
EINA_API int
eina_file_statat(void *container, Eina_File_Direct_Info *info, Eina_Stat *st)
{
   struct stat buf;
#ifdef HAVE_ATFILE_SOURCE
   int fd;
#endif

   EINA_SAFETY_ON_NULL_RETURN_VAL(info, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, -1);

#ifdef HAVE_ATFILE_SOURCE
   fd = dirfd((DIR*) container);
   if (fstatat(fd, info->path + info->name_start, &buf, 0))
#else
   (void)container;
   if (stat(info->path, &buf))
#endif
     {
        if (info->type != EINA_FILE_LNK)
          info->type = EINA_FILE_UNKNOWN;
        return -1;
     }

   if (info->type == EINA_FILE_UNKNOWN)
     {
        if (S_ISREG(buf.st_mode))
          info->type = EINA_FILE_REG;
        else if (S_ISDIR(buf.st_mode))
          info->type = EINA_FILE_DIR;
        else if (S_ISCHR(buf.st_mode))
          info->type = EINA_FILE_CHR;
        else if (S_ISBLK(buf.st_mode))
          info->type = EINA_FILE_BLK;
        else if (S_ISFIFO(buf.st_mode))
          info->type = EINA_FILE_FIFO;
        else if (S_ISLNK(buf.st_mode))
          info->type = EINA_FILE_LNK;
        else if (S_ISSOCK(buf.st_mode))
          info->type = EINA_FILE_SOCK;
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
   st->blksize = buf.st_blksize;
   st->blocks = buf.st_blocks;
   st->atime = buf.st_atime;
   st->mtime = buf.st_mtime;
   st->ctime = buf.st_ctime;
#ifdef _STAT_VER_LINUX
   st->atimensec = buf.st_atim.tv_nsec;
   st->mtimensec = buf.st_mtim.tv_nsec;
   st->ctimensec = buf.st_ctim.tv_nsec;
#else
   st->atimensec = 0;
   st->mtimensec = 0;
   st->ctimensec = 0;
#endif
   return 0;
}

///////////////////////////////////////////////////////////////////////////
// this below is funky avoiding opendir to avoid heap allocations thus
// getdents and all the os specific stuff as this is intendedf for use
// between fork and exec normally ... this is important
#if defined(__FreeBSD__)
# define do_getdents(fd, buf, size) getdents(fd, buf, size)
typedef struct
{
#if __FreeBSD__ > 11
   ino_t          d_ino;
   off_t          d_off;
   unsigned short d_reclen;
   unsigned char  d_type;
   unsigned char  ____pad0;
   unsigned short d_namlen;
   unsigned short ____pad1;
   char           d_name[4096];
#else
   __uint32_t     d_fileno;
   __uint16_t     d_reclen;
   __uint8_t      d_type;
   __uint8_t      d_namlen;
   char           d_name[4096];
#endif
} Dirent;
#elif defined(__OpenBSD__)
# define do_getdents(fd, buf, size) getdents(fd, buf, size)
typedef struct
{
   __ino_t        d_ino;
   __off_t        d_off;
   unsigned short d_reclen;
   unsigned char  d_type;
   unsigned char  d_namlen;
   unsigned char  ____pad[4];
   char           d_name[4096];
} Dirent;
#elif defined(__linux__)
# define do_getdents(fd, buf, size) syscall(SYS_getdents64, fd, buf, size)
// getdents64 added un glibc 2.30 ... so use raw syscall - will work
// from some linux 2.4 on... so ... i think that's ok. :)
//# define do_getdents(fd, buf, size) getdents64(fd, buf, size)
typedef struct
{
   ino64_t        d_ino;
   off64_t        d_off;
   unsigned short d_reclen;
   unsigned char  d_type;
   char           d_name[4096]; /**< Null-terminated filename. */
} Dirent;
#endif

/**
 * @brief Closes all open file descriptors from a given fd upwards.
 *
 * This function is intended to be called in critical sections, typically
 * between a `fork()` and `exec()` sequence, where memory allocation
 * (like `malloc` for `opendir`) should be avoided.
 *
 * It attempts to iterate over `/proc/self/fd` or `/dev/fd` to find open
 * file descriptors. For each descriptor found that is numerically greater
 * than or equal to @p fd, it calls `close()`.
 *
 * If @p except_fd is not NULL, it points to a NULL-terminated array of
 * integers representing file descriptors that should *not* be closed.
 *
 * If directory iteration fails (e.g. `/proc/self/fd` not available or
 * `getdents` fails), it falls back to a loop closing FDs from @p fd up to
 * a system-defined maximum (or 1024).
 *
 * @param fd The lowest file descriptor number to start closing from (inclusive).
 * @param except_fd A NULL-terminated array of file descriptor numbers to
 *                  exclude from closing. Can be NULL if no exceptions.
 *                  Example: `int exceptions[] = {stdout_fileno, stderr_fileno, -1};`
 *
 * @note This function tries to avoid heap allocations. On Linux, it uses
 *       the `getdents64` syscall directly. On FreeBSD/OpenBSD, it uses `getdents`.
 *       If these mechanisms are unavailable or fail, it uses a less precise
 *       iterative close up to RLIMIT_NOFILE or a default max.
 */
EINA_API void
eina_file_close_from(int fd, int *except_fd)
{
#if defined(_WIN32)
   // XXX: what do to here? anything?
#else
#ifdef HAVE_DIRENT_H
//# if 0
# if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__linux__)
   int dirfd;
   Dirent *d;
   char buf[4096 + 128];
   int *closes = NULL;
   int num_closes = 0, i, j, clo, num;
   const char *fname;
   ssize_t pos, ret;
   Eina_Bool do_read;

   // note - this api is EXPECTED to be called in between a fork() and exec()
   // when no threads are running. if you use this outside that context then
   // it may not work as intended and may miss some fd's etc.
   dirfd = open("/proc/self/fd", O_RDONLY | O_DIRECTORY);
   if (dirfd < 0) dirfd = open("/dev/fd", O_RDONLY | O_DIRECTORY);
   if (dirfd >= 0)
     {
        // count # of closes - the dir list should/will not change as its
        // the fd's we have open so we can read it twice with no changes
        // to it
        do_read = EINA_TRUE;
        for (;;)
          {
skip:
             if (do_read)
               {
                  pos = 0;
                  ret = do_getdents(dirfd, buf, 4096);
                  if (ret <= 0) break;
                  do_read = EINA_FALSE;
               }
             d = (Dirent *)(buf + pos);
             fname = d->d_name;
             pos += d->d_reclen;
             if (pos >= ret) do_read = EINA_TRUE;
             if (!((fname[0] >= '0') && (fname[0] <= '9'))) continue;
             num = atoi(fname);
             if (num < fd) continue;
             if (except_fd)
               {
                  for (j = 0; except_fd[j] >= 0; j++)
                    {
                       if (except_fd[j] == num) goto skip;
                    }
               }
             num_closes++;
          }
        // alloc closes list and walk again to fill it - on stack to avoid
        // heap allocs
        closes = alloca(num_closes * sizeof(int));
        if ((closes) && (num_closes > 0))
          {
             clo = 0;
             lseek(dirfd, 0, SEEK_SET);
             do_read = EINA_TRUE;
             for (;;)
               {
skip2:
                  if (do_read)
                    {
                       pos = 0;
                       ret = do_getdents(dirfd, buf, 4096);
                       if (ret <= 0) break;
                       do_read = EINA_FALSE;
                    }
                  d = (Dirent *)(buf + pos);
                  fname = d->d_name;
                  pos += d->d_reclen;
                  if (pos >= ret) do_read = EINA_TRUE;
                  if (!((fname[0] >= '0') && (fname[0] <= '9'))) continue;
                  num = atoi(fname);
                  if (num < fd) continue;
                  if (except_fd)
                    {
                       for (j = 0; except_fd[j] >= 0; j++)
                         {
                            if (except_fd[j] == num) goto skip2;
                         }
                    }
                  if (clo < num_closes) closes[clo] = num;
                  clo++;
               }
             // in case we somehow don't fill up all of closes in 2nd pass
             // (this shouldn't happen as no threads are running and we
             // do nothing to modify the fd set between 2st and 2nd pass).
             // set rest num_closes to clo so we don't close invalid values
             num_closes = clo;
          }
        close(dirfd);
        // now go close all those fd's - some may be invalid like the dir
        // reading fd above... that's ok.
        for (i = 0; i < num_closes; i++)
          {
             close(closes[i]);
          }
        return;
     }
# else
   DIR *dir;
   int *closes = NULL;
   int num_closes = 0, i, j, clo, num;
   struct dirent *dp;
   const char *fname;

   dir = opendir("/proc/self/fd");
   if (!dir) dir = opendir("/dev/fd");
   if (dir)
     {
        // count # of closes - the dir list should/will not change as its
        // the fd's we have open so we can read it twice with no changes
        // to it
        for (;;)
          {
skip:
             if (!(dp = readdir(dir))) break;
             fname = dp->d_name;
             if (!((fname[0] >= '0') && (fname[0] <= '9'))) continue;
             num = atoi(fname);
             if (num < fd) continue;
             if (except_fd)
               {
                  for (j = 0; except_fd[j] >= 0; j++)
                    {
                       if (except_fd[j] == num) goto skip;
                    }
               }
             num_closes++;
          }
        // alloc closes list and walk again to fill it - on stack to avoid
        // heap allocs
        closes = alloca(num_closes * sizeof(int));
        if ((closes) && (num_closes > 0))
          {
             clo = 0;
             seekdir(dir, 0);
             for (;;)
               {
skip2:
                  if (!(dp = readdir(dir))) break;
                  fname = dp->d_name;
                  if (!((fname[0] >= '0') && (fname[0] <= '9'))) continue;
                  num = atoi(fname);
                  if (num < fd) continue;
                  if (except_fd)
                    {
                       for (j = 0; except_fd[j] >= 0; j++)
                         {
                            if (except_fd[j] == num) goto skip2;
                         }
                    }
                  if (clo < num_closes) closes[clo] = num;
                  clo++;
               }
          }
        closedir(dir);
        // now go close all those fd's - some may be invalide like the dir
        // reading fd above... that's ok.
        for (i = 0; i < num_closes; i++)
          {
             close(closes[i]);
          }
        return;
     }
# endif
#endif
   int max = 1024;

#ifdef HAVE_SYS_RESOURCE_H
   struct rlimit lim;
   if (getrlimit(RLIMIT_NOFILE, &lim) < 0) return;
   max = lim.rlim_max;
#endif
   for (i = fd; i < max;)
     {
        if (except_fd)
          {
             int j;

             for (j = 0; except_fd[j] >= 0; j++)
               {
                  if (except_fd[j] == i) goto skip3;
               }
          }
        close(i);
skip3:
        i++;
     }
#endif
}

/**
 * @brief Creates and opens a unique temporary file.
 *
 * This function generates a unique temporary filename from @p templatename
 * and opens it. The @p templatename string must end with "XXXXXX" (or "XXXXXX."
 * followed by a suffix for mkstemps). These 'X's are replaced to form a unique name.
 *
 * If @p templatename does not contain a '/', it's prefixed with the system's
 * temporary directory path (from eina_environment_tmp_get()).
 *
 * The file is created with permissions 0600 (read/write for owner only) by
 * temporarily setting umask to S_IRWXG | S_IRWXO.
 *
 * @param templatename A template for the temporary filename. Must end in "XXXXXX"
 *                     or "XXXXXX.suffix".
 *                     Example: "myapp_temp_XXXXXX" or "myapp_temp_XXXXXX.log".
 * @param path If not NULL, this will be set to an Eina_Tmpstr containing the
 *             actual path of the created temporary file on success. The caller
 *             should free this with eina_tmpstr_del() when no longer needed.
 *             Set to NULL on failure.
 * @return The file descriptor of the opened temporary file on success, or -1 on failure.
 *         The file descriptor should be closed by the caller.
 *
 * @see eina_environment_tmp_get()
 * @see mkstemp(3)
 * @see mkstemps(3)
 */
EINA_API int
eina_file_mkstemp(const char *templatename, Eina_Tmpstr **path)
{
   char buffer[PATH_MAX];
   const char *XXXXXX = NULL, *sep;
   int fd, len;
   mode_t old_umask;

   EINA_SAFETY_ON_NULL_RETURN_VAL(templatename, -1);

   sep = strchr(templatename, '/');
   if (sep)
     {
        len = eina_strlcpy(buffer, templatename, sizeof(buffer));
     }
   else
     {
        len = eina_file_path_join(buffer, sizeof(buffer),
                                  eina_environment_tmp_get(), templatename);
     }

   /*
    * Unix:
    * Make sure temp file is created with secure permissions,
    * http://man7.org/linux/man-pages/man3/mkstemp.3.html#NOTES
    */
   old_umask = umask(S_IRWXG|S_IRWXO);
   if ((XXXXXX = strstr(buffer, "XXXXXX.")) != NULL)
     {
        int suffixlen = buffer + len - XXXXXX - 6;
        fd = mkstemps(buffer, suffixlen);
     }
   else
     fd = mkstemp(buffer);
   umask(old_umask);

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
 * This function generates a unique temporary directory name from @p templatename.
 * The @p templatename string must end with "XXXXXX". These 'X's are replaced
 * to form a unique directory name.
 *
 * If @p templatename does not contain a '/', it's prefixed with the system's
 * temporary directory path (from eina_environment_tmp_get()).
 *
 * The directory is created with default permissions (influenced by umask).
 *
 * @param templatename A template for the temporary directory name. Must end in "XXXXXX".
 *                     Example: "myapp_session_XXXXXX".
 * @param path If not NULL, this will be set to an Eina_Tmpstr containing the
 *             actual path of the created temporary directory on success. The caller
 *             should free this with eina_tmpstr_del() when no longer needed.
 *             Set to NULL on failure.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @see eina_environment_tmp_get()
 * @see mkdtemp(3)
 */
EINA_API Eina_Bool
eina_file_mkdtemp(const char *templatename, Eina_Tmpstr **path)
{
   char buffer[PATH_MAX];
   char *tmpdirname, *sep;

   EINA_SAFETY_ON_NULL_RETURN_VAL(templatename, EINA_FALSE);

   sep = strchr(templatename, '/');
   if (sep)
     {
        eina_strlcpy(buffer, templatename, sizeof(buffer));
     }
   else
     {
        eina_file_path_join(buffer, sizeof(buffer),
                            eina_environment_tmp_get(), templatename);
     }

   tmpdirname = mkdtemp(buffer);
   if (tmpdirname == NULL)
     {
        if (path) *path = NULL;
        return EINA_FALSE;
     }

   if (path) *path = eina_tmpstr_add(tmpdirname);
   return EINA_TRUE;
}


/**
 * @brief Checks user's permissions for a file.
 *
 * This function is a wrapper around the `access(2)` system call. It checks
 * whether the calling process can access the file @p path with the specified
 * @p mode.
 *
 * @param path The path to the file or directory to check.
 * @param mode The access mode(s) to check for. This is a bitmask that can be
 *             a combination of:
 *             - EINA_FILE_ACCESS_R_OK: Test for read permission.
 *             - EINA_FILE_ACCESS_W_OK: Test for write permission.
 *             - EINA_FILE_ACCESS_X_OK: Test for execute (search) permission.
 *             - EINA_FILE_ACCESS_F_OK: Test for existence of file.
 *             These correspond to R_OK, W_OK, X_OK, F_OK for `access()`.
 * @return EINA_TRUE if the requested access is permitted, EINA_FALSE otherwise
 *         (e.g., permission denied, file does not exist, path is NULL/empty).
 *
 * @see access(2)
 * @see Eina_File_Access_Mode
 */
EINA_API Eina_Bool
eina_file_access(const char *path, Eina_File_Access_Mode mode)
{
   if (!path || !*path)
     return EINA_FALSE;

   return access(path, mode) == 0;
}
