/* Default error handlers for CPP Library.
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

#include <Eina.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpplib.h"

/**
 * @brief Prints the "In file included from..." chain for nested includes.
 *
 * This function is intended to show the chain of #include directives that leads
 * to the current file being processed, similar to how a compiler reports the
 * context of an error or warning. It traverses the include stack backwards.
 *
 * For example, an ideal output for a nested include might be:
 * @code
 * In file included from main.c:1,
 *                  from header.h:5:
 * @endcode
 *
 * A call to this function is typically followed by a message-printing function
 * (like cpp_file_line_for_message()) that prints the location of a specific
 * warning or error.
 *
 * To avoid redundant output, it prints the include stack only once per location,
 * using the `pfile->input_stack_listing_current` flag as a guard. This flag
 * should be reset when the file or line number changes.
 *
 * @note The current implementation does not print the filenames and line numbers
 * in the include chain, only the "In file included" preamble and separators.
 *
 * @param pfile The CPP reader context, which contains the include stack.
 */
void
cpp_print_containing_files(cpp_reader * pfile)
{
   cpp_buffer         *ip;
   int                 first = 1;

   /* If stack of files hasn't changed since we last printed
    * this info, don't repeat it.  */
   if (pfile->input_stack_listing_current)
      return;

   ip = cpp_file_buffer(pfile);

   /* Give up if we don't find a source file.  */
   if (!ip)
      return;

   /* Find the other, outer source files.  */
   while ((ip = CPP_PREV_BUFFER(ip)), ip != CPP_NULL_BUFFER(pfile))
     {
	long                line, col;

	cpp_buf_line_and_col(ip, &line, &col);
	if (ip->fname)
	  {
	     if (first)
	       {
		  first = 0;
		  fprintf(stderr, "In file included");
	       }
	     else
		fprintf(stderr, ",\n                ");
	  }
     }
   if (!first)
      fprintf(stderr, ":\n");

   /* Record we have printed the status as of this time.  */
   pfile->input_stack_listing_current = 1;
}

/**
 * @brief Prints the file, line, and optional column number for a message.
 *
 * Formats and prints a standard "filename:line:column: " or "filename:line: "
 * prefix to stderr. This is typically used before an error or warning message to
 * indicate its origin.
 *
 * For example:
 * - `cpp_file_line_for_message(pfile, "foo.c", 10, 5)` prints `foo.c:10:5: `.
 * - `cpp_file_line_for_message(pfile, "foo.c", 10, 0)` prints `foo.c:10: `.
 *
 * @param pfile The CPP reader context (currently unused).
 * @param filename The name of the file where the message originates.
 * @param line The line number in the file.
 * @param column The column number in the file. If 0 or less, it's omitted.
 */
void
cpp_file_line_for_message(cpp_reader * pfile EINA_UNUSED, const char *filename,
			  int line, int column)
{
   if (column > 0)
     {
	fprintf(stderr, "%s:%d:%d: ", filename, line, column);
     }
   else
     {
	fprintf(stderr, "%s:%d: ", filename, line);
     }
}

/**
 * @brief Prints a message (error or warning) to stderr, using a va_list.
 *
 * This is a variadic argument version for printing messages. It increments
 * the error count in the pfile context if the message is an error. The message
 * is always terminated by a newline character.
 *
 * @param pfile The CPP reader context, used to count errors.
 * @param is_error A flag indicating the message type.
 *                 - If non-zero, it's an error and `pfile->errors` is incremented.
 *                 - If zero, it's a warning, and "warning: " is prepended to the message.
 * @param msg The format string for the message (printf-style).
 * @param args The va_list of arguments for the format string.
 */
void
cpp_message_v(cpp_reader * pfile, int is_error, const char *msg, va_list args)
{
   if (is_error)
      pfile->errors++;
   else
      fprintf(stderr, "warning: ");
   vfprintf(stderr, msg, args);
   fprintf(stderr, "\n");
}

/**
 * @brief Prints a message (error or warning) to stderr.
 *
 * This function is a wrapper around cpp_message_v() that handles a
 * variable number of arguments directly. If it's an error, it increments
 * the error count in the pfile context.
 *
 * @param pfile The CPP reader context, used to count errors.
 * @param is_error A flag indicating the message type. See cpp_message_v() for details.
 * @param msg The format string for the message (printf-style).
 * @param ... Additional arguments for the format string.
 */
void
cpp_message(cpp_reader * pfile, int is_error, const char *msg, ...)
{
   va_list             args;

   va_start(args, msg);

   cpp_message_v(pfile, is_error, msg, args);

   va_end(args);
}

/**
 * @brief Prints a fatal error message to stderr and exits, using a va_list.
 *
 * This function constructs a fatal error message and terminates the program.
 * The program name (from the global `progname` variable) is prepended to the
 * message. A newline is appended. The program then exits with `FATAL_EXIT_CODE`.
 * This function does not return.
 *
 * @param msg The format string for the fatal error message (printf-style).
 * @param args The va_list of arguments for the format string.
 */
static void
cpp_fatal_v(const char *msg, va_list args)
{
   fprintf(stderr, "%s: ", progname);
   vfprintf(stderr, msg, args);
   fprintf(stderr, "\n");
   exit(FATAL_EXIT_CODE);
}

/**
 * @brief Prints a fatal error message to stderr and exits.
 *
 * This function is a wrapper around cpp_fatal_v() that handles a variable
 * number of arguments directly. It prepends the program name to the message
 * and terminates the program with `FATAL_EXIT_CODE`.
 * This function does not return.
 *
 * @param msg The format string for the fatal error message (printf-style).
 * @param ... Additional arguments for the format string.
 */
void
cpp_fatal(const char *msg, ...)
{
   va_list             args;

   va_start(args, msg);

   cpp_fatal_v(msg, args);

   va_end(args);
}

/**
 * @brief Prints a system error message (like perror) and then exits fatally.
 *
 * This function is used for handling fatal errors related to system calls (e.g.,
 * file I/O). It calls `cpp_perror_with_name()` to print a system error message
 * (based on `errno`) associated with the given `name`. After printing the
 * message, it terminates the program.
 *
 * The exit code is platform-dependent: `vaxc$errno` on VMS, `FATAL_EXIT_CODE`
 * otherwise. This function does not return.
 *
 * @param pfile The CPP reader context, passed to cpp_perror_with_name().
 * @param name A string, typically a filename or operation, that was involved
 *             in the system call that failed. E.g., "opening file 'foo.h'".
 */
void
cpp_pfatal_with_name(cpp_reader * pfile, const char *name)
{
   cpp_perror_with_name(pfile, name);
#ifdef VMS
   exit(vaxc$errno);
#else
   exit(FATAL_EXIT_CODE);
#endif
}
