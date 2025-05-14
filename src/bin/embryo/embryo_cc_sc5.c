/*  Small compiler - Error message system
 *  In fact a very simple system, using only 'panic mode'.
 *
 *  Copyright (c) ITB CompuPhase, 1997-2003
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id$
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <Eina.h>

#include "embryo_cc_sc.h"
#include "embryo_cc_sc5.scp"

static int errflag;
static int errstart;	/* line number at which the instruction started */

/*  error
 *
 *  Outputs an error message (note: msg is passed optionally).
 *  If an error is found, the variable "errflag" is set and subsequent
 *  errors are ignored until lex() finds a semicolumn or a keyword
 *  (lex() resets "errflag" in that case).
 *
 *  Global references: inpfname   (referred to only)
 *                     fline      (referred to only)
 *                     fcurrent   (referred to only)
 *                     errflag    (altered)
 */
/**
 * @brief Reports an error, warning or fatal error.
 * @param number The error code.
 *               - < 100: A regular error. This will set the "panic mode" flag
 *                 to suppress subsequent errors until a synchronization point.
 *               - 100-199: A fatal error. This will abort compilation.
 *               - >= 200: A warning. This will not set the "panic mode" flag.
 * @param ...    Variable arguments to be formatted into the error message.
 *
 * This function handles error reporting during compilation. It uses a "panic
 * mode" (the `errflag` global) to avoid cascading error messages. Once an
 * error is reported, subsequent non-fatal errors are suppressed until the
 * parser reaches a point where it thinks it can recover (like a semicolon).
 *
 * Error reporting is also suppressed during the first pass of the compiler.
 *
 * After a large number of errors, or upon a fatal error, compilation is
 * aborted using longjmp(). This function also detects and reports when too
 * many errors occur on a single line.
 *
 * @return 0 on success. Note that for fatal errors, this function does not
 *         return, as it aborts via longjmp().
 */
int
error(int number, ...)
{
   static int          lastline, lastfile, errorcount;
   char               *msg;
   va_list             argptr;
   char                string[1024];
   int start;

   /* errflag is reset on each semicolon.
    * In a two-pass compiler, an error should not be reported twice. Therefore
    * the error reporting is enabled only in the second pass (and only when
    * actually producing output). Fatal errors may never be ignored.
    */
   if (((errflag) || (sc_status != statWRITE)) &&
       ((number < 100) || (number >= 200)))
     return 0;

   if (number < 100)
     {
	if (number > 74) number = 74;
	msg = errmsg[number - 1];
	errflag = TRUE;	/* set errflag (skip rest of erroneous expression) */
	errnum++;
     }
   else if (number < 200)
     {
	if (number > 107) number = 107;
	msg = fatalmsg[number - 100];
	errnum++; /* a fatal error also counts as an error */
     }
   else
     {
	msg = warnmsg[number - 200];
	warnnum++;
     }

   strexpand(string, (unsigned char *)msg, sizeof string, SCPACK_TABLE);

   va_start(argptr, number);

   start = (errstart == fline) ? -1 : errstart;

   if (sc_error(number, string, inpfname, start, fline, argptr))
   {
      sc_closeasm(outf);
      outf = NULL;
      longjmp(errbuf, 3);
   }

   va_end(argptr);

   if (((number >= 100) && (number < 200)) || (errnum > 250))
     {
	va_start(argptr, number);
	sc_error(0, "\nCompilation aborted.", NULL, 0, 0, argptr);
	va_end(argptr);

	if (outf)
	  {
	     sc_closeasm(outf);
	     outf = NULL;
	  }			/* if */
	longjmp(errbuf, 2);	/* fatal error, quit */
     }				/* if */

   /* check whether we are seeing many errors on the same line */
   if (((errstart < 0) && (lastline != fline)) ||
       (lastline < errstart) || (lastline > fline) || (fcurrent != lastfile))
      errorcount = 0;
   lastline = fline;
   lastfile = fcurrent;
   if (number < 200)
      errorcount++;
   if (errorcount >= 3)
      error(107); /* too many error/warning messages on one line */
   return 0;
}

/**
 * @brief Manages the error state of the compiler.
 * @param code The action to perform.
 *             - `sRESET`: Resets the error flag, allowing new errors to be
 *               reported. This is typically called after a synchronization
 *               point (like a semicolon) is reached in the source code.
 *             - `sFORCESET`: Sets the error flag, suppressing subsequent
 *               error messages. This is the "panic mode".
 *             - `sEXPRMARK`: Marks the starting line of an expression or
 *               statement. This is used to report more accurate error locations.
 *             - `sEXPRRELEASE`: Clears the stored starting line of an expression.
 */
void
errorset(int code)
{
   switch (code)
     {
      case sRESET:
	errflag = FALSE;	/* start reporting errors */
	break;
      case sFORCESET:
	errflag = TRUE;		/* stop reporting errors */
	break;
      case sEXPRMARK:
	errstart = fline;	/* save start line number */
	break;
      case sEXPRRELEASE:
	errstart = -1;		/* forget start line number */
	break;
      default:
	break;
     }
}
