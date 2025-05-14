/* Part of CPP library.  (memory allocation - xmalloc etc)
 * Copyright (C) 1986, 87, 89, 92, 93, 94, 1995 Free Software Foundation, Inc.
 * Written by Per Bothner, 1994.
 * Based on CCCP program by by Paul Rubin, June 1986
 * Adapted to ANSI C, Richard Stallman, Jan 1987
 * Copyright (C) 2003-2011 Kim Woelders
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Help stamp out software-hoarding!  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include "cpplib.h"

/**
 * @brief Handles memory allocation failure by terminating the program.
 *
 * This function is called when a memory allocation function (like malloc,
 * realloc, or calloc) fails. It reports a fatal error and exits, so it
 * does not return.
 */
static void
memory_full(void)
{
   cpp_fatal("Memory exhausted.");
}

/**
 * @brief Allocates memory and exits on failure.
 *
 * This function is a wrapper around malloc(). If malloc() fails, it calls
 * memory_full() to terminate the program. This ensures that the caller
 * never receives a NULL pointer.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory. This function never returns NULL.
 */
void               *
xmalloc(unsigned size)
{
   char               *ptr = (char *)malloc(size);

   if (ptr)
      return (ptr);
   memory_full();
   /*NOTREACHED */
   return 0;
}

/**
 * @brief Reallocates memory and exits on failure.
 *
 * This function is a wrapper around realloc(). If realloc() fails, it calls
 * memory_full() to terminate the program. This ensures that the caller
 * never receives a NULL pointer.
 *
 * @param old A pointer to the memory block to be reallocated.
 * @param size The new size for the memory block, in bytes.
 * @return A pointer to the reallocated memory. This function never returns NULL.
 */
void               *
xrealloc(void *old, unsigned size)
{
   char               *ptr = (char *)realloc(old, size);

   if (!ptr)
      memory_full();
   return ptr;
}

/**
 * @brief Allocates and zero-initializes memory, and exits on failure.
 *
 * This function is a wrapper around calloc(). It allocates memory for an
 * array of `number` elements of `size` bytes each and initializes all bytes
 * in the allocated storage to zero. If calloc() fails, it calls
 * memory_full() to terminate the program. This ensures that the caller
 * never receives a NULL pointer.
 *
 * @param number The number of elements to allocate.
 * @param size The size of each element in bytes.
 * @return A pointer to the allocated memory. This function never returns NULL.
 */
void               *
xcalloc(unsigned number, unsigned size)
{
   char               *ptr = (char *)calloc(number, size);

   if (!ptr)
      memory_full();
   return ptr;
}
