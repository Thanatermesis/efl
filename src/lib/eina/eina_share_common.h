/* EINA - EFL data type library
 * Copyright (C) 2002-2008 Carsten Haitzler, Jorge Luis Zapata Muga, Cedric Bail
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2008 Peter Wehrfritz
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies of the Software and its Copyright notices. In addition publicly
 *  documented acknowledgment must be given that this software has been used if no
 *  source code of this software is made available publicly. This includes
 *  acknowledgments in either Copyright notices, Manuals, Publicity and Marketing
 *  documents or any documentation provided with any product containing this
 *  software. This License does not apply to any software that links to the
 *  libraries provided by this software (statically or dynamically), but only to
 *  the software provided.
 *
 *  Please see the OLD-COPYING.PLAIN for a plain-english explanation of this notice
 *  and it's intent.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef EINA_SHARE_COMMON_H_
#define EINA_SHARE_COMMON_H_

#include "eina_types.h"
#include "eina_magic.h"

/**
 * @file
 * @brief This file defines the common API for shared string management.
 *
 * This module provides a way to manage shared string instances to reduce
 * memory usage by ensuring that identical strings are stored only once.
 * It handles reference counting for these shared strings.
 */

/**
 * @brief Opaque type for a shared string manager instance.
 * @see eina_share_common_init()
 * @see eina_share_common_shutdown()
 */
typedef struct _Eina_Share Eina_Share;

/**
 * @struct dumpinfo
 * @brief Structure to hold statistics about shared string usage.
 *
 * This structure is used by eina_share_common_dump() to report
 * memory usage, savings, and duplication rates.
 */
struct dumpinfo
{
   int used;    /**< Total bytes used by the shared string structures and strings. */
   int saved;   /**< Estimated bytes saved by sharing strings. */
   int dups;    /**< Number of duplicate string instances avoided (sum of references - 1 for each unique string). */
   int unique;  /**< Number of unique strings stored. */
};

/**
 * @brief Initializes a shared string manager.
 * @param[out] share Pointer to an Eina_Share pointer, which will be allocated and initialized.
 * @param[in] node_magic Magic number to use for identifying shared string nodes.
 * @param[in] node_magic_STR String representation of the node_magic.
 * @return #EINA_TRUE on success, #EINA_FALSE on allocation failure.
 */
Eina_Bool   eina_share_common_init(Eina_Share **share,
                                   Eina_Magic node_magic,
                                   const char *node_magic_STR);
/**
 * @brief Shuts down a shared string manager.
 * @param[in,out] share Pointer to an Eina_Share pointer to be freed. The pointer will be set to NULL.
 * @return #EINA_TRUE on success, #EINA_FALSE if an error occurs (should not happen with valid input).
 */
Eina_Bool   eina_share_common_shutdown(Eina_Share **share);

/**
 * @brief Adds a string of a given length to the shared pool or increments its reference count.
 * @param[in] share The shared string manager instance.
 * @param[in] str The string to add.
 * @param[in] slen The length of the string (excluding any null terminator).
 * @param[in] null_size The size of the null termination character(s) to append (e.g., 1 for C strings, 2 for UCS-2).
 * @return A pointer to the shared string instance, or NULL on failure or if str is NULL or slen is 0.
 *         The returned string is guaranteed to be null-terminated according to null_size.
 * @note The returned string should be released with eina_share_common_del() when no longer needed.
 */
const char *eina_share_common_add_length(Eina_Share *share,
                                         const char *str,
                                         unsigned int slen,
                                         unsigned int null_size)
EINA_WARN_UNUSED_RESULT;

/**
 * @brief Increments the reference count of an already shared string.
 * @param[in] share The shared string manager instance.
 * @param[in] str A pointer to a string previously returned by eina_share_common_add_length() or eina_share_common_ref().
 * @return The same pointer `str`, or `str` itself if it's not a recognized shared string (e.g., if it's NULL or not managed).
 * @note This function is used to take an additional reference to an existing shared string.
 */
const char *eina_share_common_ref(Eina_Share *share, const char *str);

/**
 * @brief Decrements the reference count of a shared string, freeing it if the count reaches zero.
 * @param[in] share The shared string manager instance.
 * @param[in] str The shared string to release.
 * @return #EINA_TRUE if the string was successfully processed (found and refcount decremented, or not a shared string),
 *         #EINA_FALSE if an internal error occurred (e.g., magic check failure).
 * @note If `str` is NULL, this function returns #EINA_TRUE and does nothing.
 */
Eina_Bool   eina_share_common_del(Eina_Share *share, const char *str) EINA_WARN_UNUSED_RESULT;

/**
 * @brief Gets the length of a shared string.
 * @param[in] share The shared string manager instance.
 * @param[in] str The shared string.
 * @return The length of the string if it's a valid shared string,
 *         0 if `str` is not a recognized shared string (but not NULL),
 *         -1 if `str` is NULL.
 */
int         eina_share_common_length(Eina_Share *share,
                                     const char *str) EINA_CONST
EINA_WARN_UNUSED_RESULT;

/**
 * @brief Dumps statistics about the shared string pool to the log.
 * @param[in] share The shared string manager instance.
 * @param[in] additional_dump A function pointer to dump additional, type-specific statistics. Can be NULL.
 *                            The `dumpinfo` struct passed to this callback will be pre-filled by eina_share_common_dump.
 * @param[in] used Initial value for `di.used` in the `dumpinfo` structure, typically for accounting for the `Eina_Share` struct itself.
 */
void        eina_share_common_dump(Eina_Share *share, void (*additional_dump)(
                                      struct dumpinfo *), int used);


/* Population functions (typically for internal use or advanced statistics) */

/**
 * @brief Notifies the population tracker that a string of a given length has been added.
 * @param[in] share The shared string manager instance.
 * @param[in] slen The length of the string added.
 * @note This is primarily for statistics tracking under EINA_STRINGSHARE_USAGE.
 */
void        eina_share_common_population_add(Eina_Share *share, int slen);

/**
 * @brief Notifies the population tracker that a string of a given length has been deleted.
 * @param[in] share The shared string manager instance.
 * @param[in] slen The length of the string deleted.
 * @note This is primarily for statistics tracking under EINA_STRINGSHARE_USAGE.
 */
void        eina_share_common_population_del(Eina_Share *share, int slen);

#endif /* EINA_SHARE_COMMON_H_ */
