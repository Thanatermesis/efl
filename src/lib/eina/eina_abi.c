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

#include "eina_config.h"
#include "eina_private.h"
#include "eina_unicode.h"
#include "eina_safety_checks.h"

/**
 * @brief Get the next UTF-8 character from a string.
 *
 * This function retrieves the next Unicode character from a UTF-8 encoded
 * string and advances the index. It is a wrapper around
 * eina_unicode_utf8_next_get().
 *
 * @param buf The input string buffer, UTF-8 encoded.
 * @param iindex A pointer to an integer representing the current byte index
 *               within the buffer. This index will be updated to point to
 *               the start of the next character after the call.
 * @return The Eina_Unicode value of the character found, or 0 if an error
 *         occurs (e.g., end of string or invalid UTF-8 sequence).
 */
EINA_API Eina_Unicode eina_unicode_utf8_get_next(const char *buf, int *iindex)
{
   return eina_unicode_utf8_next_get(buf, iindex);
}

/**
 * @brief Calculate an aligned size for memory pool allocation.
 *
 * This function determines an appropriate alignment for a given size and
 * returns a new size that is a multiple of that alignment. This is
 * useful for memory pool allocators to ensure that allocated blocks
 * meet certain alignment requirements.
 *
 * The alignment strategy is:
 * - If size <= 2, alignment is 1 byte.
 * - If size < 8, alignment is 2 bytes.
 * - If __WORDSIZE is 32:
 *   - Alignment is 4 bytes (mask 0x7 implies (1 << 3) - 1, so align is 3, meaning 2^3 = 8 bytes. This seems to be a typo in the original code's comment or logic, as mask 0x7 means aligning to 8 bytes. The code uses `align = 3` which means `1 << 3 = 8` byte alignment).
 * - If __WORDSIZE is 64:
 *   - If size < 16, alignment is 8 bytes (align = 3).
 *   - Otherwise, alignment is 16 bytes (align = 4, mask 0x15 is unusual, it should be 0xF for 16-byte alignment. The calculation `(size >> align) + (size & mask ? 1 : 0)) << align` will effectively round up to a multiple of `(1 << align)` if `mask` is `(1 << align) - 1`. The current mask 0x15 (binary 10101) doesn't fit this pattern for a simple power-of-2 alignment).
 *
 * @param size The original size in bytes.
 * @return The size rounded up to the determined alignment boundary.
 *         For example, if size is 5 and alignment is 4, it might return 8.
 */
EINA_API unsigned int
eina_mempool_alignof(unsigned int size)
{
   unsigned int align;
   unsigned int mask;

   if (EINA_UNLIKELY(size <= 2))
     {
        align = 1;
        mask = 0x1;
     }
   else if (EINA_UNLIKELY(size < 8))
     {
        align = 2;
        mask = 0x3;
     }
   else
#if __WORDSIZE == 32
     {
        align = 3;
        mask = 0x7;
     }
#else
   if (EINA_UNLIKELY(size < 16))
     {
        align = 3;
        mask = 0x7;
     }
   else
     {
        align = 4;
        mask = 0x15;
     }
#endif

   return ((size >> align) + (size & mask ? 1 : 0)) << align;
}
