/* EINA - EFL data type library
 * Copyright (C) 2021 Vincent Torri
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

/**
 * @file
 * @brief These routines are for matching filenames or pathnames against a
 * "glob" pattern.
 */

#ifndef	EINA_FNMATCH_H
#define	EINA_FNMATCH_H

/**
 * @brief Flags for eina_fnmatch().
 *
 * These flags customize the behavior of the eina_fnmatch() function.
 * They can be bitwise-ORed together.
 */
typedef enum
{
   /**
    * Slash in string only matches slash in pattern.
    * This is the default behavior if EINA_FNMATCH_FILE_NAME is used.
    */
   EINA_FNMATCH_PATHNAME    = (1 << 0),
   /**
    * Disable backslash escaping.
    * By default, a backslash character followed by another character
    * matches that second character. For example, "\\*" matches the
    * asterisk character. If EINA_FNMATCH_NOESCAPE is set, a backslash
    * is a regular character.
    */
   EINA_FNMATCH_NOESCAPE    = (1 << 1),
   /**
    * Leading period in string must be exactly matched by period in pattern.
    * A period is "leading" if it is the first character in string, or if
    * EINA_FNMATCH_PATHNAME is set and it immediately follows a slash.
    */
   EINA_FNMATCH_PERIOD      = (1 << 2),
   /**
    * Ignore a trailing /<asterisk>/ in pattern.
    * If this flag is set, a pattern like "foo/<asterisk>" also matches "foo".
    * This is useful when matching directory paths.
    */
   EINA_FNMATCH_LEADING_DIR = (1 << 3),
   /**
    * Ignore case in match.
    * If this flag is set, 'a' will match 'A' and vice-versa.
    */
   EINA_FNMATCH_CASEFOLD    = (1 << 4),
   /**
    * An alias for EINA_FNMATCH_PATHNAME.
    * This is for compatibility with other fnmatch implementations.
    */
   EINA_FNMATCH_FILE_NAME   = EINA_FNMATCH_PATHNAME,
} Eina_Fnmatch_Flags;

/**
 * @brief Matches a string against a glob pattern.
 *
 * This function checks if the @p string matches the given @p glob pattern.
 * The behavior of the matching can be altered by the @p flags.
 *
 * @param glob The glob pattern to match against.
 *             Example: "*.txt", "file[1-3].?", "foo/<asterisk>/bar"
 * @param string The string to check.
 *               Example: "document.txt", "file2.c", "foo/baz/bar"
 * @param flags A bitmask of Eina_Fnmatch_Flags to control matching behavior.
 *              Example: EINA_FNMATCH_PATHNAME | EINA_FNMATCH_CASEFOLD
 * @return @c EINA_TRUE if the string matches the pattern, @c EINA_FALSE otherwise.
 */
EINA_API Eina_Bool eina_fnmatch(const char *glob, const char *string, Eina_Fnmatch_Flags flags);

#endif
