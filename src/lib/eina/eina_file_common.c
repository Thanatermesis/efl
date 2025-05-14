/* EINA - EFL data type library
 * Copyright (C) 2013 Cedric Bail
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

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifdef _WIN32
# include <evil_private.h> /* windows.h */
#endif

#define COPY_BLOCKSIZE (4 * 1024 * 1024)

#include "eina_config.h"
#include "eina_private.h"

#include "eina_hash.h"
#include "eina_safety_checks.h"
#include "eina_file_common.h"
#include "eina_xattr.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifdef MAP_FAILED
# undef MAP_FAILED
#endif
#define MAP_FAILED ((void *)-1)

Eina_Hash *_eina_file_cache = NULL;
Eina_Lock _eina_file_lock_cache;

#if defined(EINA_SAFETY_CHECKS) && defined(EINA_MAGIC_DEBUG)
# define EINA_FILE_MAGIC_CHECK(f, ...) do { \
   if (EINA_UNLIKELY((f) == NULL)) \
     { \
       EINA_SAFETY_ERROR("safety check failed: " # f " == NULL"); \
       return __VA_ARGS__; \
     } \
   if (EINA_UNLIKELY((f)->__magic != EINA_FILE_MAGIC)) \
     { \
        EINA_MAGIC_FAIL(f, EINA_FILE_MAGIC); \
        return __VA_ARGS__; \
     } \
   } while (0)
#else
# define EINA_FILE_MAGIC_CHECK(f, ...) do {} while(0)
#endif

static Eina_Spinlock _eina_statgen_lock;
static Eina_Statgen _eina_statgen = 0;

/**
 * @internal
 * @brief Increments the global stat generation counter.
 * This counter is used to track changes to file metadata, allowing
 * caches to be invalidated when necessary. It wraps around to 1
 * after reaching its maximum value, never becoming 0 again unless
 * explicitly disabled or re-enabled.
 */
EINA_API void
eina_file_statgen_next(void)
{
   eina_spinlock_take(&_eina_statgen_lock);
   if (_eina_statgen != 0)
     {
        _eina_statgen++;
        if (_eina_statgen == 0) _eina_statgen = 1;
     }
   eina_spinlock_release(&_eina_statgen_lock);
}

/**
 * @internal
 * @brief Retrieves the current global stat generation counter.
 * @return The current stat generation value. If stat generation is
 * disabled, this will be 0.
 */
EINA_API Eina_Statgen
eina_file_statgen_get(void)
{
   Eina_Statgen s;
   eina_spinlock_take(&_eina_statgen_lock);
   s = _eina_statgen;
   eina_spinlock_release(&_eina_statgen_lock);
   return s;
}

/**
 * @internal
 * @brief Enables the stat generation counter.
 * If the counter is currently 0 (disabled), it is set to 1.
 * Otherwise, its value remains unchanged.
 */
EINA_API void
eina_file_statgen_enable(void)
{
   eina_spinlock_take(&_eina_statgen_lock);
   if (_eina_statgen == 0) _eina_statgen = 1;
   eina_spinlock_release(&_eina_statgen_lock);
}

/**
 * @internal
 * @brief Disables the stat generation counter.
 * The counter is set to 0.
 */
EINA_API void
eina_file_statgen_disable(void)
{
   eina_spinlock_take(&_eina_statgen_lock);
   _eina_statgen = 0;
   eina_spinlock_release(&_eina_statgen_lock);
}

/**
 * @internal
 * @brief Normalizes a file path string in-place.
 * This function removes redundant slashes (e.g., "//"), resolves
 * current directory references (e.g., "/./"), and parent directory
 * references (e.g., "/foo/../bar/" becomes "/bar/").
 * On Windows, it also converts backslashes to forward slashes.
 *
 * @param path The file path string to normalize. This string is modified directly.
 * @param len The current length of the path string. This value is updated
 *            if the path is shortened.
 * @return The pointer to the (potentially modified) input `path` string,
 *         or @c NULL if the input `path` was @c NULL.
 */
static char *
_eina_file_escape(char *path, size_t len)
{
   char *result;
   char *p;
   char *q;

   result = path;
   p = result;
   q = result;

   if (!result)
     return NULL;

#ifdef _WIN32
   EINA_PATH_TO_UNIX(path);
#endif

   while ((p = strchr(p, '/')))
     {
	// remove double `/'
	if (p[1] == '/')
	  {
	     memmove(p, p + 1, --len - (p - result));
	     result[len] = '\0';
	  }
	else
	  if (p[1] == '.'
	      && p[2] == '.')
	    {
	       // remove `/../'
	       if (p[3] == '/')
		 {
		    char tmp;

		    len -= p + 3 - q;
		    memmove(q, p + 3, len - (q - result));
		    result[len] = '\0';
		    p = q;

		    /* Update q correctly. */
		    tmp = *p;
		    *p = '\0';
		    q = strrchr(result, '/');
		    if (!q) q = result;
		    *p = tmp;
		 }
	       else
		 // remove '/..$'
		 if (p[3] == '\0')
		   {
		      len -= p + 2 - q;
		      result[len] = '\0';
                      break;
		   }
		 else
		   {
		      q = p;
		      ++p;
		   }
	    }
        else
          if (p[1] == '.'
              && p[2] == '/')
            {
               // remove '/./'
               len -= 2;
               memmove(p, p + 2, len - (p - result));
               result[len] = '\0';
               q = p;
               ++p;
            }
	  else
	    {
	       q = p;
	       ++p;
	    }
     }

   return result;
}

/**
 * @internal
 * @brief Gets the length of a file map key.
 * File map keys are composed of two `unsigned long int` values
 * (offset and length).
 *
 * @param key Unused.
 * @return The size in bytes of two `unsigned long int`s.
 */
unsigned int
eina_file_map_key_length(const void *key EINA_UNUSED)
{
   return sizeof (unsigned long int) * 2;
}

/**
 * @internal
 * @brief Compares two file map keys.
 * Keys are arrays of two `unsigned long long int` values (offset and length).
 * The comparison is done first on the offset, then on the length.
 *
 * @param key1 Pointer to the first key (an array of two `unsigned long long int`).
 * @param key1_length Unused.
 * @param key2 Pointer to the second key (an array of two `unsigned long long int`).
 * @param key2_length Unused.
 * @return 0 if keys are equal.
 *         A positive value if key1 > key2.
 *         A negative value if key1 < key2.
 */
int
eina_file_map_key_cmp(const unsigned long long int *key1, int key1_length EINA_UNUSED,
                       const unsigned long long int *key2, int key2_length EINA_UNUSED)
{
   if (key1[0] == key2[0])
     {
        if (key1[1] == key2[1]) return 0;
        if (key1[1] > key2[1]) return 1;
        return -1;
     }
   if (key1[0] > key2[0]) return 1;
   return -1;
}

/**
 * @internal
 * @brief Computes a hash value for a file map key.
 * Keys are arrays of two `unsigned long long int` values (offset and length).
 * The hash is computed by XORing the hashes of the individual components.
 *
 * @param key Pointer to the key (an array of two `unsigned long long int`).
 * @param key_length Unused.
 * @return The computed hash value.
 */
int
eina_file_map_key_hash(const unsigned long long int *key, int key_length EINA_UNUSED)
{
   return eina_hash_int64(&key[0], sizeof (unsigned long long int))
     ^ eina_hash_int64(&key[1], sizeof (unsigned long long int));
}

/**
 * @internal
 * @brief Retrieves the global memory map for a virtual file.
 * This function returns a pointer to the entire content of a virtual file
 * that is mapped in memory. It also increments the global reference count
 * for this map.
 *
 * @param file The Eina_File structure representing the virtual file.
 * @return A pointer to the memory-mapped content of the file.
 */
void *
eina_file_virtual_map_all(Eina_File *file)
{
   eina_lock_take(&file->lock);
   file->global_refcount++;
   eina_lock_release(&file->lock);

   return file->global_map;
}

/**
 * @internal
 * @brief Creates or retrieves a memory map for a specific region of a virtual file.
 * If a map for the given offset and length already exists, its reference count
 * is incremented and a pointer to it is returned. Otherwise, a new map entry
 * is created, pointing to the appropriate sub-region of the file's global map.
 * The caller is responsible for checking that offset and length are valid.
 *
 * @param file The Eina_File structure representing the virtual file.
 * @param offset The starting offset of the region to map.
 * @param length The length of the region to map.
 * @return A pointer to the mapped memory region, or @c NULL on allocation failure.
 */
void *
eina_file_virtual_map_new(Eina_File *file,
                          unsigned long int offset, unsigned long int length)
{
   Eina_File_Map *map;
   unsigned long int key[2];

   // offset and length has already been checked by the caller function

   key[0] = offset;
   key[1] = length;

   eina_lock_take(&file->lock);

   map = eina_hash_find(file->map, &key);
   if (!map)
     {
        map = malloc(sizeof (Eina_File_Map));
        if (!map) goto on_error;

        map->map = ((char*) file->global_map) + offset;
        map->offset = offset;
        map->length = length;
        map->refcount = 0;

        eina_hash_add(file->map, &key, map);
        eina_hash_direct_add(file->rmap, &map->map, map);
     }

   map->refcount++;

 on_error:
   eina_lock_release(&file->lock);
   return map ? map->map : NULL;
}

/**
 * @internal
 * @brief Releases a memory map for a virtual file.
 * This function decrements the reference count of the specified map. If the
 * reference count drops to zero, the map entry is removed from the file's
 * internal tracking structures. It handles both specific region maps and
 * the global file map.
 *
 * @param file The Eina_File structure.
 * @param map A pointer to the memory map to be released. This could be a
 *            map for a specific region or the global map of the file.
 */
void
eina_file_virtual_map_free(Eina_File *file, void *map)
{
   Eina_File_Map *em;

   eina_lock_take(&file->lock);

   // map could equal global_map even if length != file->length
   em = eina_hash_find(file->rmap, &map);
   if (em)
     {
        unsigned long int key[2];

        em->refcount--;

        if (em->refcount > 0) goto on_exit;

        key[0] = em->offset;
        key[1] = em->length;

        eina_hash_del(file->rmap, &map, em);
        eina_hash_del(file->map, &key, em);
     }
   else
     {
        if (file->global_map == map)
          {
             file->global_refcount--;
          }
     }

 on_exit:
   eina_lock_release(&file->lock);
}

/**
 * @internal
 * @brief Frees a generic file map entry.
 * This function is used to release a memory map associated with an Eina_File.
 * It decrements the map's reference count. If the count reaches zero,
 * the map is removed from the file's active map hash (`rmap` and `map`)
 * or from the `dead_map` list. If removed from `dead_map`, the provided
 * `free_func` is called to deallocate the Eina_File_Map structure.
 *
 * @param file The Eina_File structure.
 * @param map Pointer to the memory region of the map to be freed.
 * @param free_func A callback function to free the Eina_File_Map structure
 *                  if the map is found in `dead_map` and its refcount is zero.
 *                  Example: _eina_file_map_close.
 */
void
eina_file_common_map_free(Eina_File *file, void *map,
                          void (*free_func)(Eina_File_Map *map))
{
   Eina_File_Map *em;
   unsigned long int key[2];
   Eina_List *l = NULL;
   Eina_Bool hashed = EINA_TRUE;

   em = eina_hash_find(file->rmap, &map);
   if (!em)
     {
        EINA_LIST_FOREACH(file->dead_map, l, em)
          if (em->map == map)
            {
               hashed = EINA_FALSE;
               break ;
            }
        if (hashed) return ;
     }

   em->refcount--;

   if (em->refcount > 0) return ;

   key[0] = em->offset;
   key[1] = em->length;

   if (hashed)
     {
        eina_hash_del(file->rmap, &map, em);
        eina_hash_del(file->map, &key, em);
     }
   else
     {
        file->dead_map = eina_list_remove_list(file->dead_map, l);
        free_func(em);
     }
}

/**
 * @internal
 * @brief Invalidates memory maps of a file, typically after its size has changed.
 * If the file had a global map, this map is moved to the `dead_map` list,
 * and its global reference count is reset. Then, all specific region maps
 * are iterated. If a map's region (offset + length) extends beyond the new
 * file `length`, it is removed from the active map hashes and added to the
 * `dead_map` list.
 *
 * @param file The Eina_File structure whose maps are to be flushed.
 * @param length The new length of the file. Maps extending beyond this
 *               length will be considered invalid.
 */
void
eina_file_flush(Eina_File *file, unsigned long int length)
{
   Eina_File_Map *tmp;
   Eina_Iterator *it;
   Eina_List *dead_map = NULL;
   Eina_List *l;

   eina_lock_take(&file->lock);

   // File size changed
   if (file->global_map != MAP_FAILED)
     {
        // Forget global map
        tmp = malloc(sizeof (Eina_File_Map));
        if (tmp)
          {
             tmp->map = file->global_map;
             tmp->offset = 0;
             tmp->length = file->length;
             tmp->refcount = file->refcount;

             file->dead_map = eina_list_append(file->dead_map, tmp);
          }

        file->global_map = MAP_FAILED;
        file->global_refcount = 0;
     }

   it = eina_hash_iterator_data_new(file->map);
   EINA_ITERATOR_FOREACH(it, tmp)
     {
        // Add out of limit map to dead_map
        if (tmp->offset + tmp->length > length)
          dead_map = eina_list_append(dead_map, tmp);
     }
   eina_iterator_free(it);

   EINA_LIST_FOREACH(dead_map, l, tmp)
     {
        unsigned long int key[2];

        key[0] = tmp->offset;
        key[1] = tmp->length;

        eina_hash_del(file->rmap, &tmp->map, tmp);
        eina_hash_del(file->map, &key, tmp);
     }

   file->dead_map = eina_list_merge(file->dead_map, dead_map);

   eina_lock_release(&file->lock);
}

// Private to this file API
/**
 * @internal
 * @brief Frees an Eina_File_Map structure.
 * This is a simple wrapper around `free` and is typically used as a
 * callback for hash tables that store Eina_File_Map pointers.
 *
 * @param map The Eina_File_Map structure to free.
 */
static void
_eina_file_map_close(Eina_File_Map *map)
{
   free(map);
}

// Global API

/**
 * @brief Sanitizes a file path.
 *
 * This function takes a file path string, converts it to an absolute path
 * if it's relative, and then normalizes it by removing redundant
 * components like "//", "/./", and "/../".
 *
 * @param path The file path string to sanitize.
 * @return A newly allocated string containing the sanitized, absolute path.
 *         The caller is responsible for freeing this string.
 *         Returns @c NULL if the input @p path is @c NULL or if memory
 *         allocation fails.
 *
 * @see _eina_file_escape()
 * @see eina_file_current_directory_get()
 */
EINA_API char *
eina_file_path_sanitize(const char *path)
{
   Eina_Tmpstr *result = NULL;
   char *r;
   size_t len;

   if (!path) return NULL;

   len = strlen(path);

   if (eina_file_path_relative(path))
     {
       result = eina_file_current_directory_get(path, len);
       len = eina_tmpstr_len(result);
     }
   else
     result = path;

   r = _eina_file_escape(strdup(result ? result : ""), len);
   if (result != path) eina_tmpstr_del(result);

   return r;
}

/**
 * @brief Creates a virtual Eina_File from in-memory data.
 *
 * This function allows treating a block of memory as if it were a file.
 *
 * @param virtual_name An optional name for the virtual file. If @c NULL,
 *                     a unique name based on the current time will be generated.
 *                     Example: "/dev/mem/virtual/abcdef1234567890".
 * @param data Pointer to the raw data for the virtual file. Must not be @c NULL.
 * @param length The size of the data in bytes.
 * @param copy If @c EINA_TRUE, the provided @p data is copied into memory
 *             managed by the Eina_File structure. The Eina_File structure and
 *             the copied data are allocated in a single memory block.
 *             If @c EINA_FALSE, the Eina_File structure will store a direct
 *             pointer to the provided @p data. In this case, the caller is
 *             responsible for ensuring the @p data remains valid for the
 *             lifetime of the Eina_File.
 * @return A pointer to a new Eina_File structure representing the virtual file,
 *         or @c NULL on failure (e.g., memory allocation error, @p data is @c NULL).
 *         The caller is responsible for closing the Eina_File using eina_file_close().
 */
EINA_API Eina_File *
eina_file_virtualize(const char *virtual_name, const void *data, unsigned long long length, Eina_Bool copy)
{
   Eina_File *file;
   Eina_Nano_Time tp;
   long int ti;
   const char *tmpname = "/dev/mem/virtual\\/%16x";
   size_t slen, head_padded;

   EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);

   // Generate an almost uniq filename based on current nsec time.
   if (_eina_time_get(&tp)) return NULL;
   ti = _eina_time_convert(&tp);

   slen = virtual_name ? strlen(virtual_name) + 1 : strlen(tmpname) + 17;
   // align copied data at end of file struct to 16 bytes...
   head_padded = 16 * ((sizeof(Eina_File) + slen + 15) / 16);

   file = malloc(head_padded + (copy ? length : 0));
   if (!file) return NULL;

   memset(file, 0, sizeof(Eina_File));
   EINA_MAGIC_SET(file, EINA_FILE_MAGIC);
   file->filename = (char *)(file + 1);
   if (virtual_name)
     file->filename = eina_stringshare_add(virtual_name);
   else
     file->filename = eina_stringshare_printf(tmpname, ti);

   eina_lock_recursive_new(&file->lock);
   file->mtime = ti / 1000;
   file->length = length;
#ifdef _STAT_VER_LINUX
   file->mtime_nsec = ti;
#endif
   file->refcount = 1;
#ifndef _WIN32
   file->fd = -1;
#else
   file->handle = INVALID_HANDLE_VALUE;
#endif
   file->virtual = EINA_TRUE;
   file->map = eina_hash_new(EINA_KEY_LENGTH(eina_file_map_key_length),
                             EINA_KEY_CMP(eina_file_map_key_cmp),
                             EINA_KEY_HASH(eina_file_map_key_hash),
                             EINA_FREE_CB(_eina_file_map_close),
                             3);
   file->rmap = eina_hash_pointer_new(NULL);

   if (copy)
     {
        file->copied = EINA_TRUE;
        file->global_map = ((char *)file) + head_padded;
        memcpy((char *)file->global_map, data, length);
     }
   else
     {
        file->global_map = (void *)data;
     }

   return file;
}

/**
 * @brief Checks if an Eina_File represents a virtual file.
 *
 * @param file The Eina_File to check.
 * @return @c EINA_TRUE if the file is virtual, @c EINA_FALSE otherwise
 *         (including if @p file is @c NULL or invalid).
 */
EINA_API Eina_Bool
eina_file_virtual(Eina_File *file)
{
   if (!file) return EINA_FALSE;
   EINA_FILE_MAGIC_CHECK(file, EINA_FALSE);
   return file->virtual;
}

/**
 * @brief Duplicates an Eina_File handle.
 *
 * This function increases the reference count of the given Eina_File.
 * For regular files, it simply returns the same pointer with an incremented
 * reference count.
 * For virtual files that were created with `copy = EINA_FALSE` (i.e., they
 * point to external data), this function creates a new, independent Eina_File
 * that *does* copy the data. This is a safety measure to prevent issues if the
 * original external data buffer is freed while a duplicated handle still exists.
 *
 * @param f The Eina_File to duplicate.
 * @return A pointer to an Eina_File. For non-virtual files or copied virtual
 *         files, this is the same as @p f. For non-copied virtual files,
 *         this is a new Eina_File instance with copied data.
 *         Returns @c NULL if @p f is @c NULL or invalid.
 */
EINA_API Eina_File *
eina_file_dup(const Eina_File *f)
{
   Eina_File *file = (Eina_File*) f;

   if (file)
     {
        EINA_FILE_MAGIC_CHECK(f, NULL);
        eina_lock_take(&file->lock);

        // For ease of use and safety of the API, if you dup a virtualized file, we prefer to make a copy
        if (file->virtual && !file->copied)
          {
             Eina_File *r;

             r = eina_file_virtualize(file->filename, file->global_map, file->length, EINA_TRUE);
             eina_lock_release(&file->lock);

             return r;
          }
        file->refcount++;
        eina_lock_release(&file->lock);
     }
   return file;
}

/**
 * @internal
 * @brief Performs the final cleanup and deallocation of an Eina_File structure.
 * This function is called when an Eina_File's reference count drops to zero.
 * It frees all associated resources, including hash tables for memory maps,
 * the filename stringshare, and calls the backend-specific `eina_file_real_close`
 * to close the actual file descriptor or handle. Finally, it frees the
 * Eina_File structure itself.
 *
 * @param file The Eina_File to clean and close.
 */
void
eina_file_clean_close(Eina_File *file)
{
   // Generic destruction of the file
   eina_hash_free(file->rmap); file->rmap = NULL;
   eina_hash_free(file->map); file->map = NULL;
   eina_stringshare_del(file->filename);

   // Backend specific file resource close
   eina_file_real_close(file);

   // Final death
   EINA_MAGIC_SET(file, 0);
   free(file);
}

/**
 * @brief Closes an Eina_File handle.
 *
 * This function decrements the reference count of the Eina_File. If the
 * reference count drops to zero, the file is removed from the global
 * Eina_File cache (if it was present) and all its resources are freed
 * by calling eina_file_clean_close().
 *
 * @param file The Eina_File to close. If @c NULL or invalid, the function
 *             does nothing.
 */
EINA_API void
eina_file_close(Eina_File *file)
{
   Eina_Bool leave = EINA_TRUE;
   unsigned int key;

   if (!file) return ;
   EINA_FILE_MAGIC_CHECK(file);

   eina_lock_take(&_eina_file_lock_cache);

   eina_lock_take(&file->lock);
   file->refcount--;
   if (file->refcount == 0) leave = EINA_FALSE;
   eina_lock_release(&file->lock);
   if (leave) goto end;

   key = eina_hash_superfast((void*) &file->filename, sizeof (void*));
   if (eina_hash_find_by_hash(_eina_file_cache,
                              file->filename, 0, key) == file)
     {
        eina_hash_del_by_key_hash(_eina_file_cache,
                                  file->filename, 0, key);
     }

   eina_file_clean_close(file);
 end:
   eina_lock_release(&_eina_file_lock_cache);
}

/**
 * @brief Gets the size of the file.
 *
 * @param file The Eina_File handle.
 * @return The size of the file in bytes. Returns 0 if @p file is @c NULL
 *         or invalid.
 */
EINA_API size_t
eina_file_size_get(const Eina_File *file)
{
   EINA_FILE_MAGIC_CHECK(file, 0);
   return file->length;
}

/**
 * @brief Gets the last modification time of the file.
 *
 * @param file The Eina_File handle.
 * @return The last modification time as a `time_t` value. Returns 0 if
 *         @p file is @c NULL or invalid.
 */
EINA_API time_t
eina_file_mtime_get(const Eina_File *file)
{
   EINA_FILE_MAGIC_CHECK(file, 0);
   return file->mtime;
}

/**
 * @brief Gets the filename associated with the Eina_File.
 *
 * This is the sanitized, absolute path to the file.
 *
 * @param file The Eina_File handle.
 * @return A pointer to the filename string. The string is managed by
 *         Eina and should not be freed by the caller. Returns @c NULL
 *         if @p file is @c NULL or invalid.
 */
EINA_API const char *
eina_file_filename_get(const Eina_File *file)
{
   EINA_FILE_MAGIC_CHECK(file, NULL);
   return file->filename;
}

/**
 * @internal
 * @brief Sanitizes a file path and returns it as an Eina_Stringshare.
 * This function first sanitizes the path using eina_file_path_sanitize()
 * to get an absolute, normalized path, and then converts this path into
 * an Eina_Stringshare.
 *
 * @param path The file path string to sanitize.
 * @return An Eina_Stringshare instance representing the sanitized path.
 *         The caller is responsible for releasing the stringshare using
 *         eina_stringshare_del() when it's no longer needed.
 *         Returns @c NULL if @p path is @c NULL or if any internal
 *         operation fails (e.g., memory allocation).
 *
 * @see eina_file_path_sanitize()
 * @see eina_stringshare_add()
 */
Eina_Stringshare *
eina_file_sanitize(const char *path)
{
   char *filename;
   Eina_Stringshare *ss;

   filename = eina_file_path_sanitize(path);
   if (!filename) return NULL;

   ss = eina_stringshare_add(filename);
   free(filename);
   return ss;
}

/* search '\r' and '\n' by preserving cache locality and page locality
   in doing a search inside 4K boundary.
 */
/**
 * @internal
 * @brief Finds the next end-of-line (EOL) sequence in a character buffer.
 *
 * This function searches for either a carriage return ('\r') or a line feed
 * ('\n'). It attempts to optimize for cache and page locality by searching
 * within specified boundaries (typically 4KB). If both '\r' and '\n' are
 * found within a search chunk, the one that appears earlier is returned.
 *
 * @param start Pointer to the beginning of the buffer (or current search position).
 * @param boundary The size of the initial chunk to search within. Subsequent
 *                 chunks are typically 4096 bytes.
 * @param end Pointer to one byte past the end of the buffer.
 * @return A pointer to the first EOL character ('\r' or '\n') found,
 *         or @p end if no EOL character is found before reaching the end
 *         of the buffer.
 */
static inline const char *
_eina_find_eol(const char *start, int boundary, const char *end)
{
   const char *cr;
   const char *lf;
   unsigned long long chunk;

   while (start < end)
     {
        chunk = start + boundary < end ? boundary : end - start;
        cr = memchr(start, '\r', chunk);
        lf = memchr(start, '\n', chunk);
        if (cr)
          {
             if (lf && lf < cr)
               return lf;
             return cr;
          }
        else if (lf)
           return lf;

        start += chunk;
        boundary = 4096;
     }

   return end;
}

/**
 * @internal
 * @brief Advances an Eina_Lines_Iterator to the next line.
 *
 * This function is the 'next' callback for the Eina_Iterator interface
 * when iterating over lines in a memory-mapped file. It skips any
 * EOL characters from the previous line, finds the EOL for the current
 * line using _eina_find_eol(), and updates the iterator's `current`
 * Eina_File_Line structure.
 *
 * @param it The Eina_Lines_Iterator.
 * @param data Output parameter: a pointer to a void pointer. On success,
 *             `*data` will be set to point to the `it->current`
 *             (an Eina_File_Line structure) representing the current line.
 * @return @c EINA_TRUE if a next line was found and `*data` is set.
 * @return @c EINA_FALSE if there are no more lines or an error occurred.
 */
static Eina_Bool
_eina_file_map_lines_iterator_next(Eina_Lines_Iterator *it, void **data)
{
   const char *eol;
   unsigned char match;

   if (it->current.end >= it->end)
     return EINA_FALSE;

   match = *it->current.end;
   if (it->current.index > 0)
     it->current.end++;
   while (it->current.end < it->end &&
          (*it->current.end == '\n' || *it->current.end == '\r'))
     {
        if (match == *it->current.end)
          break;
        it->current.end++;
     }
   it->current.index++;

   if (it->current.end == it->end)
     return EINA_FALSE;

   eol = _eina_find_eol(it->current.end,
                        it->boundary,
                        it->end);
   it->boundary = (uintptr_t) eol & 0x3FF;
   if (it->boundary == 0) it->boundary = 4096;

   it->current.start = it->current.end;

   it->current.end = eol;
   it->current.length = eol - it->current.start;

   *data = &it->current;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container (Eina_File) of an Eina_Lines_Iterator.
 *
 * This function is the 'get_container' callback for the Eina_Iterator interface.
 *
 * @param it The Eina_Lines_Iterator.
 * @return A pointer to the Eina_File being iterated over.
 */
static Eina_File *
_eina_file_map_lines_iterator_container(Eina_Lines_Iterator *it)
{
   return it->fp;
}

/**
 * @internal
 * @brief Frees an Eina_Lines_Iterator and its associated resources.
 *
 * This function is the 'free' callback for the Eina_Iterator interface.
 * It unmaps the file region that was mapped for iteration, closes the
 * associated Eina_File handle (decrementing its refcount), and frees
 * the Eina_Lines_Iterator structure itself.
 *
 * @param it The Eina_Lines_Iterator to free.
 */
static void
_eina_file_map_lines_iterator_free(Eina_Lines_Iterator *it)
{
   eina_file_map_free(it->fp, (void*) it->map);
   eina_file_close(it->fp);

   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
 * @brief Creates an iterator to read lines from a memory-mapped file.
 *
 * This function maps the entire file into memory (if not already mapped
 * for sequential access) and returns an Eina_Iterator that can be used
 * to iterate over the lines of the file. Each item returned by the
 * iterator is a pointer to an Eina_File_Line structure, which contains
 * information about the current line (start pointer, length, index).
 *
 * The file is kept mapped and the Eina_File handle is kept open (refcounted)
 * until the iterator is freed.
 *
 * @param file The Eina_File to iterate over. Must be a valid Eina_File handle.
 * @return A new Eina_Iterator for reading lines, or @c NULL if the file
 *         is empty, the file cannot be mapped, or memory allocation fails.
 *         The caller is responsible for freeing the iterator using eina_iterator_free().
 *
 * @see Eina_File_Line
 * @see _eina_file_map_lines_iterator_next()
 * @see _eina_file_map_lines_iterator_container()
 * @see _eina_file_map_lines_iterator_free()
 */
EINA_API Eina_Iterator *
eina_file_map_lines(Eina_File *file)
{
   Eina_Lines_Iterator *it;

   EINA_FILE_MAGIC_CHECK(file, NULL);

   if (file->length == 0) return NULL;

   it = calloc(1, sizeof (Eina_Lines_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->map = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   if (!it->map)
     {
        free(it);
        return NULL;
     }

   eina_lock_take(&file->lock);
   file->refcount++;
   eina_lock_release(&file->lock);

   it->fp = file;
   it->boundary = 4096;
   it->current.start = it->map;
   it->current.end = it->current.start;
   it->current.index = 0;
   it->current.length = 0;
   it->end = it->map + it->fp->length;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_eina_file_map_lines_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_eina_file_map_lines_iterator_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_eina_file_map_lines_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @brief Writes data from a buffer to a file descriptor.
 *
 * This function attempts to write @p size bytes from @p buf to the
 * file descriptor @p fd. It handles partial writes by looping until all
 * data is written. It also handles `EAGAIN` and `EINTR` errors by
 * retrying the write operation.
 *
 * @param fd The file descriptor to write to.
 * @param buf Pointer to the buffer containing data to write.
 * @param size The number of bytes to write from the buffer.
 * @return @c EINA_TRUE if all data was successfully written.
 * @return @c EINA_FALSE if a non-recoverable error occurs during writing.
 */
static Eina_Bool
_eina_file_copy_write_internal(int fd, char *buf, size_t size)
{
   size_t done = 0;
   while (done < size)
     {
        ssize_t w = write(fd, buf + done, size - done);
        if (w >= 0)
          done += w;
        else if ((errno != EAGAIN) && (errno != EINTR))
          {
             ERR("Error writing destination file during copy: %s",
                 strerror(errno));
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Reads data from a file descriptor into a buffer.
 *
 * This function attempts to read up to @p bufsize bytes from the file
 * descriptor @p fd into @p buf. It handles `EAGAIN` and `EINTR` errors
 * by retrying the read operation.
 *
 * @param fd The file descriptor to read from.
 * @param buf Pointer to the buffer where read data will be stored.
 * @param bufsize The maximum number of bytes to read (size of @p buf).
 * @param[out] readsize Pointer to a variable where the number of bytes
 *                      actually read will be stored.
 * @return @c EINA_TRUE if data was successfully read (or EOF was reached cleanly
 *                      after reading some data, though this function considers
 *                      premature EOF an error if 0 bytes are read).
 * @return @c EINA_FALSE if an error occurs during reading or if `read()`
 *                      returns 0 (premature EOF).
 */
static Eina_Bool
_eina_file_copy_read_internal(int fd, char *buf, off_t bufsize, ssize_t *readsize)
{
   while (1)
     {
        ssize_t r = read(fd, buf, bufsize);
        if (r == 0)
          {
             ERR("Premature end of source file during copy.");
             return EINA_FALSE;
          }
        else if (r < 0)
          {
             if ((errno != EAGAIN) && (errno != EINTR))
               {
                  ERR("Error reading source file during copy: %s",
                      strerror(errno));
                  return EINA_FALSE;
               }
          }
        else
          {
             *readsize = r;
             return EINA_TRUE;
          }
     }
}

#ifdef HAVE_SPLICE
/**
 * @internal
 * @brief Writes data from a pipe to a file descriptor using splice().
 * (Only available if HAVE_SPLICE is defined)
 *
 * This function attempts to transfer @p size bytes from the read end of
 * a pipe (@p pipefd) to the file descriptor @p fd using the `splice()`
 * system call. It handles partial splices and retries on `EAGAIN` and
 * `EINTR` errors. If `splice()` returns `EINVAL`, it indicates that
 * splicing is not supported for the destination file descriptor, and
 * the function returns @c EINA_FALSE.
 *
 * @param fd The destination file descriptor.
 * @param pipefd The read end of the pipe (source of data).
 * @param size The number of bytes to transfer.
 * @return @c EINA_TRUE if all data was successfully spliced.
 * @return @c EINA_FALSE if an error occurs, or if splicing is not
 *         supported for the destination @p fd (errno will be `EINVAL`).
 */
static Eina_Bool
_eina_file_copy_write_splice_internal(int fd, int pipefd, size_t size)
{
   size_t done = 0;
   while (done < size)
     {
        ssize_t w = splice(pipefd, NULL, fd, NULL, size - done, SPLICE_F_MORE);
        if (w >= 0)
          done += w;
        else if (errno == EINVAL)
          {
             INF("Splicing is not supported for destination file");
             return EINA_FALSE;
          }
        else if ((errno != EAGAIN) && (errno != EINTR))
          {
             ERR("Error splicing to destination file during copy: %s",
                 strerror(errno));
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Reads data from a file descriptor into a pipe using splice().
 * (Only available if HAVE_SPLICE is defined)
 *
 * This function attempts to transfer up to @p bufsize bytes from the
 * file descriptor @p fd to the write end of a pipe (@p pipefd) using
 * the `splice()` system call. It retries on `EAGAIN` and `EINTR` errors.
 * If `splice()` returns `EINVAL`, it indicates that splicing is not
 * supported for the source file descriptor, and the function returns
 * @c EINA_FALSE.
 *
 * @param fd The source file descriptor.
 * @param pipefd The write end of the pipe (destination for data).
 * @param bufsize The maximum number of bytes to transfer.
 * @param[out] readsize Pointer to a variable where the number of bytes
 *                      actually spliced will be stored.
 * @return @c EINA_TRUE if data was successfully spliced.
 * @return @c EINA_FALSE if an error occurs, if `splice()` returns 0 (premature EOF),
 *         or if splicing is not supported for the source @p fd (errno will be `EINVAL`).
 */
static Eina_Bool
_eina_file_copy_read_splice_internal(int fd, int pipefd, off_t bufsize, ssize_t *readsize)
{
   while (1)
     {
        ssize_t r = splice(fd, NULL, pipefd, NULL, bufsize, SPLICE_F_MORE);
        if (r == 0)
          {
             ERR("Premature end of source file during splice.");
             return EINA_FALSE;
          }
        else if (r < 0)
          {
             if (errno == EINVAL)
               {
                  INF("Splicing is not supported for source file");
                  return EINA_FALSE;
               }
             else if ((errno != EAGAIN) && (errno != EINTR))
               {
                  ERR("Error splicing from source file during copy: %s",
                      strerror(errno));
                  return EINA_FALSE;
               }
          }
        else
          {
             *readsize = r;
             return EINA_TRUE;
          }
     }
}
#endif

/**
 * @internal
 * @brief Copies data between two file descriptors using the splice() system call.
 * (Only available if HAVE_SPLICE is defined, otherwise it's a stub)
 *
 * This function attempts to copy @p total bytes from source file descriptor @p s
 * to destination file descriptor @p d using `splice()`. It creates a pipe
 * internally to facilitate the transfer. It calls the progress callback @p cb
 * periodically.
 *
 * @param s The source file descriptor.
 * @param d The destination file descriptor.
 * @param total The total number of bytes to copy.
 * @param cb An optional progress callback function.
 *           Prototype: `Eina_Bool (*cb)(void *data, off_t current, off_t total)`
 *           Return @c EINA_FALSE from callback to abort copy.
 * @param cb_data User data to be passed to the progress callback.
 * @param[out] splice_unsupported Set to @c EINA_TRUE if `splice()` is determined
 *                                to be unsupported for either @p s or @p d,
 *                                or if `pipe()` fails. Otherwise, it's set to
 *                                @c EINA_FALSE if splice operations were attempted.
 * @return @c EINA_TRUE if the copy was successful.
 * @return @c EINA_FALSE if the copy failed or was aborted by the callback.
 *         If `HAVE_SPLICE` is not defined, this function always sets
 *         `*splice_unsupported` to @c EINA_TRUE and returns @c EINA_FALSE.
 */
static Eina_Bool
_eina_file_copy_splice_internal(int s, int d, off_t total, Eina_File_Copy_Progress cb, const void *cb_data, Eina_Bool *splice_unsupported)
{
#ifdef HAVE_SPLICE
   off_t bufsize = COPY_BLOCKSIZE;
   off_t done;
   Eina_Bool ret;
   int pipefd[2];

   *splice_unsupported = EINA_TRUE;

   if (pipe(pipefd) < 0) return EINA_FALSE;

   done = 0;
   ret = EINA_TRUE;
   while (done < total)
     {
        size_t todo;
        ssize_t r;

        if (done + bufsize < total)
          todo = bufsize;
        else
          todo = total - done;

        ret = _eina_file_copy_read_splice_internal(s, pipefd[1], todo, &r);
        if (!ret) break;

        ret = _eina_file_copy_write_splice_internal(d, pipefd[0], r);
        if (!ret) break;

        *splice_unsupported = EINA_FALSE;
        done += r;

        if (cb)
          {
             ret = cb((void *)cb_data, done, total);
             if (!ret) break;
          }
     }

   close(pipefd[0]);
   close(pipefd[1]);

   return ret;
#endif
   *splice_unsupported = EINA_TRUE;
   return EINA_FALSE;
   (void)s;
   (void)d;
   (void)total;
   (void)cb;
   (void)cb_data;
}

/**
 * @internal
 * @brief Copies data between two file descriptors.
 *
 * This function orchestrates the file copy operation. It first attempts to use
 * `_eina_file_copy_splice_internal()` if `splice()` is available. If splicing
 * is unsupported or fails in a way that indicates it shouldn't be retried,
 * it falls back to a traditional read/write loop using a temporary buffer,
 * managed by `_eina_file_copy_read_internal()` and
 * `_eina_file_copy_write_internal()`.
 * It calls the progress callback @p cb periodically.
 *
 * @param s The source file descriptor.
 * @param d The destination file descriptor.
 * @param total The total number of bytes to copy.
 * @param cb An optional progress callback function.
 *           Prototype: `Eina_Bool (*cb)(void *data, off_t current, off_t total)`
 *           Return @c EINA_FALSE from callback to abort copy.
 * @param cb_data User data to be passed to the progress callback.
 * @return @c EINA_TRUE if the copy was successful.
 * @return @c EINA_FALSE if the copy failed or was aborted by the callback.
 */
static Eina_Bool
_eina_file_copy_internal(int s, int d, off_t total, Eina_File_Copy_Progress cb, const void *cb_data)
{
   void *buf = NULL;
   off_t bufsize = COPY_BLOCKSIZE;
   off_t done;
   Eina_Bool ret, splice_unsupported;

   ret = _eina_file_copy_splice_internal(s, d, total, cb, cb_data,
                                         &splice_unsupported);
   if (ret)
     return EINA_TRUE;
   else if (!splice_unsupported) /* splice works, but copy failed anyway */
     return EINA_FALSE;

   /* make sure splice didn't change the position */
   lseek(s, 0, SEEK_SET);
   lseek(d, 0, SEEK_SET);

   while ((bufsize > 0) && ((buf = malloc(bufsize)) == NULL))
     bufsize /= 128;

   EINA_SAFETY_ON_NULL_RETURN_VAL(buf, EINA_FALSE);

   done = 0;
   ret = EINA_TRUE;
   while (done < total)
     {
        size_t todo;
        ssize_t r;

        if (done + bufsize < total)
          todo = bufsize;
        else
          todo = total - done;

        ret = _eina_file_copy_read_internal(s, buf, todo, &r);
        if (!ret) break;

        ret = _eina_file_copy_write_internal(d, buf, r);
        if (!ret) break;

        done += r;

        if (cb)
          {
             ret = cb((void *)cb_data, done, total);
             if (!ret) break;
          }
     }

   free(buf);
   return ret;
}

/**
 * @brief Copies a file from a source path to a destination path.
 *
 * This function copies the contents of the file specified by @p src to a new
 * file at @p dst. If @p dst already exists, it will be truncated.
 * The function can optionally copy file permissions and extended attributes.
 * A progress callback can be provided to monitor the copy operation.
 *
 * @param src The path to the source file. Must not be @c NULL.
 * @param dst The path to the destination file. Must not be @c NULL.
 * @param flags A bitmask of Eina_File_Copy_Flags:
 *              - @c EINA_FILE_COPY_PERMISSION: Copy file permissions (mode).
 *              - @c EINA_FILE_COPY_XATTR: Copy extended attributes.
 * @param cb An optional progress callback function.
 *           Prototype: `Eina_Bool (*cb)(void *data, off_t current, off_t total)`
 *           The callback receives the @p cb_data, current bytes copied, and total bytes.
 *           Return @c EINA_FALSE from the callback to abort the copy.
 * @param cb_data User data to be passed to the progress callback.
 * @return @c EINA_TRUE if the file was successfully copied.
 * @return @c EINA_FALSE if an error occurred (e.g., source not found,
 *         cannot write to destination, callback aborted). If the copy fails,
 *         the destination file @p dst will be unlinked (deleted).
 */
EINA_API Eina_Bool
eina_file_copy(const char *src, const char *dst, Eina_File_Copy_Flags flags, Eina_File_Copy_Progress cb, const void *cb_data)
{
   struct stat st;
   int s, d = -1;
   Eina_Bool success;

   EINA_SAFETY_ON_NULL_RETURN_VAL(src, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dst, EINA_FALSE);

   s = open(src, O_RDONLY | O_BINARY);
   EINA_SAFETY_ON_TRUE_RETURN_VAL (s < 0, EINA_FALSE);

   success = (fstat(s, &st) == 0);
   EINA_SAFETY_ON_FALSE_GOTO(success, end);

   d = open(dst, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
   EINA_SAFETY_ON_TRUE_GOTO(d < 0, end);

   success = _eina_file_copy_internal(s, d, st.st_size, cb, cb_data);
   if (success)
     {
#ifdef HAVE_FCHMOD
        if (flags & EINA_FILE_COPY_PERMISSION)
          fchmod(d, st.st_mode);
#endif
        if (flags & EINA_FILE_COPY_XATTR)
          eina_xattr_fd_copy(s, d);
     }

 end:
   if (d >= 0) close(d);
   else success = EINA_FALSE;
   close(s);

   if (!success)
     unlink(dst);

   return success;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @internal
 * @brief Initializes the Eina_File module.
 *
 * This function sets up global resources used by the Eina_File system,
 * including:
 * - Registering a log domain "eina_file".
 * - Creating a global cache (`_eina_file_cache`) for Eina_File objects,
 *   keyed by stringshared filenames.
 * - Initializing a spinlock (`_eina_statgen_lock`) for the stat generation counter.
 * - Initializing the stat generation counter (`_eina_statgen`), enabling it if
 *   the "EINA_STATGEN" environment variable is set.
 * - Initializing a recursive lock (`_eina_file_lock_cache`) for the global file cache.
 * - Setting the magic string for Eina_File structures for debugging.
 *
 * This function should be called once at application startup before any other
 * Eina_File functions are used.
 *
 * @return @c EINA_TRUE on successful initialization.
 * @return @c EINA_FALSE if an error occurs (e.g., log domain registration fails,
 *         cache creation fails).
 */
Eina_Bool
eina_file_init(void)
{
   _eina_file_log_dom = eina_log_domain_register("eina_file",
                                                 EINA_LOG_COLOR_DEFAULT);
   if (_eina_file_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: eina_file");
        return EINA_FALSE;
     }

   _eina_file_cache = eina_hash_stringshared_new(NULL);
   if (!_eina_file_cache)
     {
        ERR("Could not create cache.");
        eina_log_domain_unregister(_eina_file_log_dom);
        _eina_file_log_dom = -1;
        return EINA_FALSE;
     }

   eina_spinlock_new(&_eina_statgen_lock);
   eina_spinlock_take(&_eina_statgen_lock);
   if (getenv("EINA_STATGEN")) _eina_statgen = 1;
   eina_spinlock_release(&_eina_statgen_lock);
   eina_lock_recursive_new(&_eina_file_lock_cache);
   eina_magic_string_set(EINA_FILE_MAGIC, "Eina_File");

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Eina_File module.
 *
 * This function cleans up global resources used by the Eina_File system:
 * - Iterates through the `_eina_file_cache`. If any files are still open
 *   (refcount > 0), an error message is logged for each.
 * - Frees the `_eina_file_cache`.
 * - Frees the `_eina_file_lock_cache`.
 * - Unregisters the "eina_file" log domain.
 * - Frees the `_eina_statgen_lock`.
 *
 * This function should be called once at application shutdown after all
 * Eina_File objects have been closed.
 *
 * @return Always returns @c EINA_TRUE. (The return type is Eina_Bool for
 *         consistency with other Eina module shutdown functions).
 */
Eina_Bool
eina_file_shutdown(void)
{
   if (eina_hash_population(_eina_file_cache) > 0)
     {
        Eina_Iterator *it;
        const char *key;

        it = eina_hash_iterator_key_new(_eina_file_cache);
        EINA_ITERATOR_FOREACH(it, key)
          {
             Eina_File *f = eina_hash_find(_eina_file_cache, key);
             ERR("File [%s] still open %i times !", key, f->refcount);
          }
        eina_iterator_free(it);
     }

   eina_hash_free(_eina_file_cache);
   _eina_file_cache = NULL;

   eina_lock_free(&_eina_file_lock_cache);

   eina_log_domain_unregister(_eina_file_log_dom);
   _eina_file_log_dom = -1;
   eina_spinlock_free(&_eina_statgen_lock);
   return EINA_TRUE;
}

/**
 * @brief Sets or clears the close-on-exec flag for a file descriptor.
 *
 * When the close-on-exec flag is set for a file descriptor, that file
 * descriptor will be automatically closed when any of the `exec` family
 * of functions is called. This is generally a good practice to prevent
 * unintended file descriptor leakage to child processes.
 *
 * On Windows, this function is a no-op and always returns @c EINA_TRUE,
 * as the concept of close-on-exec is handled differently (e.g., via
 * handle inheritance flags at process creation).
 *
 * @param fd The file descriptor for which to set/clear the flag.
 * @param on If @c EINA_TRUE, the FD_CLOEXEC flag is set.
 *           If @c EINA_FALSE, the FD_CLOEXEC flag is cleared.
 * @return @c EINA_TRUE on success or if on a platform where this is not
 *         applicable (like Windows or if fcntl is unavailable but treated as success).
 * @return @c EINA_FALSE if `fcntl` fails to get or set the flags.
 *         `errno` will be set by `fcntl`.
 */
EINA_API Eina_Bool
eina_file_close_on_exec(int fd, Eina_Bool on)
{
#ifdef _WIN32
   return EINA_TRUE;
   (void)fd;
   (void)on;
#elif HAVE_FCNTL
   int flags;

   flags = fcntl(fd, F_GETFD);
   if (flags < 0)
     {
        int errno_backup = errno;
        ERR("%#x = fcntl(%d, F_GETFD): %s", flags, fd, strerror(errno));
        errno = errno_backup;
        return EINA_FALSE;
     }

   if (on)
     flags |= FD_CLOEXEC;
   else
     flags &= (~flags);

   if (fcntl(fd, F_SETFD, flags) == -1)
     {
        int errno_backup = errno;
        ERR("fcntl(%d, F_SETFD, %#x): %s", fd, flags, strerror(errno));
        errno = errno_backup;
        return EINA_FALSE;
     }
   return EINA_TRUE;
#else
   static Eina_Bool statement = EINA_FALSE;

   if (!statement)
     ERR("fcntl is not available on your platform. fd may leak when using exec.");
   statement = EINA_TRUE;
   return EINA_TRUE;
#endif
}
