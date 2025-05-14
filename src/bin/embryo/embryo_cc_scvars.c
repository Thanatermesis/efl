/*  Small compiler
 *
 *  Global (cross-module) variables.
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

/** @file
 *  Global (cross-module) variables for the Small compiler.
 *
 *  This file declares all global variables that are shared amongst the
 *  compiler files.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>		/* for PATH_MAX */
#endif

#include "embryo_cc_sc.h"

/*  global variables
 *
 *  All global variables that are shared amongst the compiler files are
 *  declared here.
 */
symbol   loctab;	/**< local symbol table */
symbol   glbtab;	/**< global symbol table */
cell    *litq;	/**< the literal queue, holds constant values */
char     pline[sLINEMAX + 1];	/**< buffer for a line read from the input file */
char    *lptr;	/**< pointer to the current position in #pline */
constvalue tagname_tab = { NULL, "", 0, 0 };	/**< table for tagnames */
constvalue libname_tab = { NULL, "", 0, 0 };	/**< table for library names, used with #pragma library */
constvalue *curlibrary = NULL;	/**< pointer to the current library, or NULL if none */
symbol  *curfunc;	/**< pointer to the symbol table entry of the current function being parsed */
char    *inpfname;	/**< name of the file currently being read from */
char     sc_ctrlchar = CTRL_CHAR;	/**< the control character (escape character), default is CTRL_CHAR */
int      litidx = 0;	/**< current index into the literal table #litq */
int      litmax = sDEF_LITMAX;	/**< current allocated size of the literal table #litq */
int      stgidx = 0;	/**< index to the staging buffer, used for optimizing code generation */
int      labnum = 0;	/**< number of (internal) labels generated */
int      staging = 0;	/**< flag indicating whether output is being staged (buffered) */
cell     declared = 0;	/**< number of local cells (variables) declared in the current scope */
cell     glb_declared = 0;	/**< number of global cells (variables) declared */
cell     code_idx = 0;	/**< current size in bytes of the generated code */
int      ntv_funcid = 0;	/**< incremental ID for native functions */
int      errnum = 0;	/**< count of errors encountered during compilation */
int      warnnum = 0;	/**< count of warnings encountered during compilation */
int      sc_debug = sCHKBOUNDS;	/**< debug flags, e.g., sCHKBOUNDS for bounds checking */
int      charbits = 8;	/**< number of bits in a char, typically 8 */
int      sc_packstr = FALSE;	/**< flag indicating whether strings should be packed by default */
int      sc_compress = TRUE;	/**< flag indicating whether to compress the bytecode output */
int      sc_needsemicolon = TRUE;	/**< flag indicating whether semicolons are required to terminate expressions */
int      sc_dataalign = sizeof(cell);	/**< data alignment size in bytes, typically the size of a 'cell' */
int      sc_alignnext = FALSE;	/**< flag indicating if the next function's frame must be aligned */
int      curseg = 0;	/**< current segment being parsed: 0 for none, 1 for CODE, 2 for DATA */
cell     sc_stksize = sDEF_AMXSTACK;	/**< default stack size for the compiled script */
int      freading = FALSE;	/**< flag indicating if an input file is currently open and ready for reading */
int      fline = 0;	/**< current line number in the input file #inpf */
int      fnumber = 0;	/**< file number in the file table (used for debugging information) */
int      fcurrent = 0;	/**< index of the current file being processed (debugging) */
int      intest = 0;	/**< flag indicating if the parser is inside a compile-time test (#if, #assert) */
int      sideeffect = 0;	/**< flag indicating if the current expression has side effects */
int      stmtindent = 0;	/**< current indentation level of statements */
int      indent_nowarn = TRUE;	/**< flag to skip warning "217 loose indentation" */
int      sc_tabsize = 8;	/**< number of spaces a TAB character represents */
int      sc_allowtags = TRUE;	/**< flag to allow or disallow tagnames in lex() */
int      sc_status;	/**< general status, used for read/write status of files */
int      sc_rationaltag = 0;	/**< tag ID for rational numbers, if supported */
int      rational_digits = 0;	/**< number of fractional digits for rational numbers */

FILE    *inpf = NULL;	/**< file pointer for the current input file (source or include) */
FILE    *inpf_org = NULL;	/**< file pointer for the main source file */
FILE    *outf = NULL;	/**< file pointer for the output file */

jmp_buf  errbuf; /**< buffer for setjmp/longjmp error handling, allows jumping out of parsing functions on error */
