/* EINA - EFL data type library
 * Copyright (C) 2015 Carsten Haitzler
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

#include <string.h>

#include "eina_debug.h"

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#ifdef HAVE_VALGRIND
# include <valgrind.h>
# include <memcheck.h>
#endif

#ifdef _WIN32
# include <evil_private.h> /* mmap */
#else
# include <sys/mman.h>
#endif

#ifdef HAVE_MMAP

// custom memory allocators to avoid malloc/free during backtrace handling
// just in case we're inside some signal handler due to mem corruption and
// are inside a malloc/free lock and thus would deadlock ourselves if we
// allocated memory, so implement scratch space just big enough for what we
// need and then some via either a static 8k+4k buffer pair or via a growable
// mmaped mem chunk pair
// implement using mmap so we can grow if needed - unlikelt though
static unsigned char *chunk1 = NULL; /**< Primary chunk for general allocations. */
static unsigned char *chunk2 = NULL; /**< Secondary chunk, typically used for realloc operations like path lookups. */
static unsigned char *chunk3 = NULL; /**< Tertiary chunk for temporary allocations that can be reset. */
static int chunk1_size = 0; /**< Current allocated size of chunk1. */
static int chunk1_num = 0; /**< Currently used bytes in chunk1. */
static int chunk2_size = 0; /**< Current allocated size of chunk2. */
static int chunk2_num = 0; /**< Currently used bytes in chunk2. */
static int chunk3_size = 0; /**< Current allocated size of chunk3. */
static int chunk3_num = 0; /**< Currently used bytes in chunk3. */

static int no_anon = -1; /**< Flag to indicate if anonymous mmap is disabled (-1: unset, 0: enabled, 1: disabled). */

/**
 * @brief Allocates a new chunk of memory, preferably using anonymous mmap.
 *
 * If Valgrind is running or anonymous mmap is disabled (via EFL_NO_MMAP_ANON
 * environment variable or mmap failure), it falls back to malloc.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure.
 */
static void *
_eina_debug_chunk_need(int size)
{
   void *ptr;

#ifdef HAVE_VALGRIND
   if (RUNNING_ON_VALGRIND) ptr = malloc(size);
   else
#endif
     {
        if (no_anon == -1)
          {
             if (getenv("EFL_NO_MMAP_ANON")) no_anon = 1;
             else no_anon = 0;
          }
        if (no_anon == 1) ptr = malloc(size);
        else
          {
             ptr = mmap(NULL, size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANON, -1, 0);
             if (ptr == MAP_FAILED) return NULL;
          }
     }
   return ptr;
}

/**
 * @brief Releases a chunk of memory previously allocated by _eina_debug_chunk_need.
 *
 * If Valgrind is running or anonymous mmap was disabled during allocation,
 * it uses free. Otherwise, it uses munmap.
 *
 * @param ptr Pointer to the memory chunk to release.
 * @param size The size of the memory chunk to release (must match allocation size).
 */
static void
_eina_debug_chunk_noneed(void *ptr, int size)
{
#ifdef HAVE_VALGRIND
   if (RUNNING_ON_VALGRIND) free(ptr);
   else
#endif
     {
        if (no_anon == 1) free(ptr);
        else munmap(ptr, size);
     }
}

/**
 * @brief Allocates memory from a growable stack-like chunk (chunk1).
 *
 * This function provides memory from a pre-allocated or dynamically grown
 * chunk (chunk1). Allocations are pointer-aligned. If the chunk is too small,
 * it's reallocated to twice its size. Memory allocated this way is not
 * intended to be individually freed; the entire chunk system is reset or
 * managed as a whole.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory within chunk1, or NULL on failure.
 */
void *
_eina_debug_chunk_push(int size)
{
   void *ptr;

   // no initial chunk1 block - allocate it
   if (!chunk1)
     {
        chunk1 = _eina_debug_chunk_need(8 * 1024);
        if (!chunk1) return NULL;
        chunk1_size = 8 * 1024;
     }
   // round size up to the nearest pointer size for alignment
   size = sizeof(void *) * ((size + sizeof(void *) - 1) / sizeof(void *));
   // if our chunk is too small - grow it
   if ((chunk1_num + size) > chunk1_size)
     {
        // get a new chunk twice as big
        void *newchunk = _eina_debug_chunk_need(chunk1_size * 2);
        if (!newchunk) return NULL;
        // copy content over
        memcpy(newchunk, chunk1, chunk1_num);
        // release old chunk
        _eina_debug_chunk_noneed(chunk1, chunk1_size);
        // switch to our new 2x as big chunk
        chunk1 = newchunk;
        chunk1_size = chunk1_size * 2;
     }
   // get the mem at the top of this stack and return it, then move along
   ptr = chunk1 + chunk1_num;
   chunk1_num += size;
   return ptr;
}

/**
 * @brief Reallocates or allocates a dedicated chunk (chunk2) to a specific size.
 *
 * This function manages chunk2, typically used for a single, resizable buffer
 * (e.g., for filename to path lookups). If chunk2 doesn't exist, it's
 * allocated. If it exists but is smaller than the requested size, it's
 * reallocated (doubling its size until sufficient). The content of chunk2
 * is preserved up to chunk2_num bytes during reallocation.
 *
 * @param size The desired new size for chunk2.
 * @return A pointer to chunk2, or NULL on failure. The content of chunk2
 *         up to the old chunk2_num is preserved.
 */
void *
_eina_debug_chunk_realloc(int size)
{
   // we have a null/empty second chunk - allocate one
   if (!chunk2)
     {
        chunk2 = _eina_debug_chunk_need(4 * 1024);
        if (!chunk2) return NULL;
        chunk2_size = 4 * 1024;
     }
   // if our chunk is too small - grow it
   if (size > chunk2_size)
     {
        // get a new chunk twice as big
        void *newchunk = _eina_debug_chunk_need(chunk2_size * 2);
        if (!newchunk) return NULL;
        // copy content over
        memcpy(newchunk, chunk2, chunk2_num);
        // release old chunk
        _eina_debug_chunk_noneed(chunk2, chunk2_size);
        // switch to our new 2x as big chunk
        chunk2 = newchunk;
        chunk2_size = chunk2_size * 2;
     }
   // record new size and return chunk ptr as we just re-use it
   chunk2_num = size;
   return chunk2;
}

/**
 * @brief Allocates memory from a temporary, resettable chunk (chunk3).
 *
 * This function provides memory from a pre-allocated or dynamically grown
 * chunk (chunk3). Allocations are pointer-aligned. If the chunk is too small,
 * it's reallocated to twice its size. Memory allocated from this chunk can be
 * effectively "freed" all at once by calling _eina_debug_chunk_tmp_reset().
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory within chunk3, or NULL on failure.
 */
void *
_eina_debug_chunk_tmp_push(int size)
{
   void *ptr;

   // no initial chunk1 block - allocate it
   if (!chunk3)
     {
        chunk3 = _eina_debug_chunk_need(32 * 1024);
        if (!chunk3) return NULL;
        chunk3_size = 32 * 1024;
     }
   // round size up to the nearest pointer size for alignment
   size = sizeof(void *) * ((size + sizeof(void *) - 1) / sizeof(void *));
   // if our chunk is too small - grow it
   if ((chunk3_num + size) > chunk3_size)
     {
        // get a new chunk twice as big
        void *newchunk = _eina_debug_chunk_need(chunk3_size * 2);
        if (!newchunk) return NULL;
        // copy content over
        memcpy(newchunk, chunk3, chunk3_num);
        // release old chunk
        _eina_debug_chunk_noneed(chunk3, chunk3_size);
        // switch to our new 2x as big chunk
        chunk3 = newchunk;
        chunk3_size = chunk3_size * 2;
     }
   // get the mem at the top of this stack and return it, then move along
   ptr = chunk3 + chunk3_num;
   chunk3_num += size;
   return ptr;
}

/**
 * @brief Resets the temporary chunk (chunk3), effectively clearing all allocations made from it.
 *
 * This function sets the used count (chunk3_num) of chunk3 back to zero,
 * allowing its memory to be reused for new temporary allocations without
 * actually freeing and reallocating the underlying memory block.
 */
void
_eina_debug_chunk_tmp_reset(void)
{
   chunk3_num = 0;
}
# else
// implement with static buffers - once we exceed these we will fail. sorry
// maybe one day find another solution, but these buffers should be enough
// for now for thos eplatforms (like windows) where we can't do the mmap
// tricks above.
static unsigned char chunk1[8 * 1024]; /**< Static primary chunk for general allocations. */
static unsigned char chunk2[4 * 1024]; /**< Static secondary chunk, typically used for realloc operations. */
static unsigned char chunk3[128 * 1024]; /**< Static tertiary chunk for temporary allocations. */
static int chunk1_size = sizeof(chunk1); /**< Size of static chunk1. */
static int chunk1_num = 0; /**< Currently used bytes in static chunk1. */
static int chunk2_size = sizeof(chunk2); /**< Size of static chunk2. */
static int chunk2_num = 0; /**< Currently used bytes in static chunk2. */
static int chunk3_size = sizeof(chunk3); /**< Size of static chunk3. */
static int chunk3_num = 0; /**< Currently used bytes in static chunk3. */

/**
 * @brief Allocates memory from a static stack-like chunk (chunk1). (No HAVE_MMAP version)
 *
 * This function provides memory from a fixed-size static buffer (chunk1).
 * Allocations are pointer-aligned. If the buffer runs out of space,
 * allocation fails. Memory allocated this way is not intended to be
 * individually freed.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory within chunk1, or NULL if out of space.
 */
void *
_eina_debug_chunk_push(int size)
{
   void *ptr;

   // round size up to the nearest pointer size for alignment
   size = sizeof(void *) * ((size + sizeof(void *) - 1) / sizeof(void *));
   // if we ran out of space - fail
   if ((chunk1_num + size) > chunk1_size) return NULL;
   // get the mem at the top of this stack and return it, then move along
   ptr = chunk1 + chunk1_num;
   chunk1_num += size;
   return ptr;
}

/**
 * @brief "Reallocates" a static dedicated chunk (chunk2) by setting its used size. (No HAVE_MMAP version)
 *
 * This function manages chunk2, a fixed-size static buffer. It doesn't actually
 * reallocate memory but sets the `chunk2_num` to the requested size if it fits
 * within `chunk2_size`. This is used for a single buffer that might be reused
 * with different content sizes.
 *
 * @param size The desired "used" size for chunk2.
 * @return A pointer to chunk2 if `size` is within `chunk2_size`, otherwise NULL.
 */
void *
_eina_debug_chunk_realloc(int size)
{
   // if we ran out of space - fail
   if (size > chunk2_size) return NULL;
   // record new size and return chunk ptr as we just re-use it
   chunk2_num = size;
   return chunk2;
}

/**
 * @brief Allocates memory from a static temporary, resettable chunk (chunk3). (No HAVE_MMAP version)
 *
 * This function provides memory from a fixed-size static buffer (chunk3).
 * Allocations are pointer-aligned. If the buffer runs out of space,
 * allocation fails. Memory allocated from this chunk can be effectively
 * "freed" all at once by calling _eina_debug_chunk_tmp_reset().
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory within chunk3, or NULL if out of space.
 */
void *
_eina_debug_chunk_tmp_push(int size)
{
   void *ptr;

   // round size up to the nearest pointer size for alignment
   size = sizeof(void *) * ((size + sizeof(void *) - 1) / sizeof(void *));
   // if we ran out of space - fail
   if ((chunk3_num + size) > chunk3_size) return NULL;
   // get the mem at the top of this stack and return it, then move along
   ptr = chunk3 + chunk1_num;
   chunk3_num += size;
   return ptr;
}

/**
 * @brief Resets the static temporary chunk (chunk3). (No HAVE_MMAP version)
 *
 * This function sets the used count (chunk3_num) of the static chunk3
 * back to zero, allowing its memory to be reused.
 */
void
_eina_debug_chunk_tmp_reset(void)
{
   chunk3_num = 0;
}
# endif

/**
 * @brief Duplicates a string using memory from the primary chunk (_eina_debug_chunk_push).
 *
 * Allocates memory for a copy of the input string `str` using
 * `_eina_debug_chunk_push` and then copies the string content. The duplicated
 * string is not meant to be individually freed.
 *
 * @param str The null-terminated string to duplicate.
 * @return A pointer to the newly allocated and copied string, or NULL on failure
 *         (e.g., if `str` is NULL or memory allocation fails).
 */
char *
_eina_debug_chunk_strdup(const char *str)
{
   int len = strlen(str);
   char *s = _eina_debug_chunk_push(len + 1);
   if (!s) return NULL;
   strcpy(s, str);
   return s;
}
