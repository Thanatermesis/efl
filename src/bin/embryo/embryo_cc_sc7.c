/*  Small compiler - Staging buffer and optimizer
 *
 *  The staging buffer
 *  ------------------
 *  The staging buffer allows buffered output of generated code, deletion
 *  of redundant code, optimization by a tinkering process and reversing
 *  the ouput of evaluated expressions (which is used for the reversed
 *  evaluation of arguments in functions).
 *  Initially, stgwrite() writes to the file directly, but after a call to
 *  stgset(TRUE), output is redirected to the buffer. After a call to
 *  stgset(FALSE), stgwrite()'s output is directed to the file again. Thus
 *  only one routine is used for writing to the output, which can be
 *  buffered output or direct output.
 *
 *  staging buffer variables:   stgbuf  - the buffer
 *                              stgidx  - current index in the staging buffer
 *                              staging - if true, write to the staging buffer;
 *                                        if false, write to file directly.
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>		/* for atoi() */
#include <string.h>
#include <ctype.h>

#include "embryo_cc_sc.h"

#include "embryo_cc_sc7.scp"

static void         stgstring(char *start, char *end);
static void         stgopt(char *start, char *end);

#define sSTG_GROW   512
#define sSTG_MAX    20480

static char        *stgbuf = NULL;
static int          stgmax = 0;	/* current size of the staging buffer */

#define CHECK_STGBUFFER(index) if ((int)(index)>=stgmax) grow_stgbuffer((index)+1)

/**
 * @brief Ensures the staging buffer is large enough, growing it if necessary.
 * @param requiredsize The minimum required size for the staging buffer.
 *
 * This function will allocate or reallocate the staging buffer (stgbuf)
 * to be at least `requiredsize` bytes. It includes a growth factor (sSTG_GROW)
 * to reduce the frequency of reallocations. If the required size exceeds
 * sSTG_MAX, it triggers a fatal error.
 */
static void
grow_stgbuffer(int requiredsize)
{
   char               *p;
   int                 clear = !stgbuf;	/* if previously none, empty buffer explicitly */

   assert(stgmax < requiredsize);
   /* if the staging buffer (holding intermediate code for one line) grows
    * over a few kBytes, there is probably a run-away expression
    */
   if (requiredsize > sSTG_MAX)
      error(102, "staging buffer");	/* staging buffer overflow (fatal error) */
   stgmax = requiredsize + sSTG_GROW;
   if (stgbuf)
      p = (char *)realloc(stgbuf, stgmax * sizeof(char));
   else
      p = (char *)malloc(stgmax * sizeof(char));
   if (!p)
      error(102, "staging buffer");	/* staging buffer overflow (fatal error) */
   stgbuf = p;
   if (clear)
      *stgbuf = '\0';
}

/**
 * @brief Frees the memory allocated for the staging buffer.
 *
 * Resets stgbuf to NULL and stgmax to 0.
 */
void
stgbuffer_cleanup(void)
{
   if (stgbuf)
     {
	free(stgbuf);
	stgbuf = NULL;
	stgmax = 0;
     }				/* if */
}

/* the variables "stgidx" and "staging" are declared in "scvars.c" */

/*  stgmark
 *
 *  Copies a mark into the staging buffer. At this moment there are three
 *  possible marks:
 *     sSTARTREORDER    identifies the beginning of a series of expression
 *                      strings that must be written to the output file in
 *                      reordered order
 *    sENDREORDER       identifies the end of 'reverse evaluation'
 *    sEXPRSTART + idx  only valid within a block that is evaluated in
 *                      reordered order, it identifies the start of an
 *                      expression; the "idx" value is the argument position
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (altered)
 *                     staging (referred to only)
 */
/**
 * @brief Writes a special marker character into the staging buffer.
 * @param mark The marker character to write.
 *             Possible values include:
 *             - sSTARTREORDER: Marks the beginning of a reorderable block.
 *             - sENDREORDER: Marks the end of a reorderable block.
 *             - sEXPRSTART + idx: Marks the start of an expression within a
 *                                 reorderable block, where idx is the argument position.
 *
 * This function only writes the mark if staging is active.
 */
void
stgmark(char mark)
{
   if (staging)
     {
	CHECK_STGBUFFER(stgidx);
	stgbuf[stgidx++] = mark;
     }				/* if */
}

/**
 * @brief Writes a string to the assembly output file.
 * @param str The string to write.
 * @return TRUE if successful or if not in write status, FALSE on write error.
 *
 * This function only writes to the output file (outf) if sc_status is statWRITE.
 * It uses sc_writeasm for the actual writing.
 */
static int
filewrite(char *str)
{
   if (sc_status == statWRITE)
      return sc_writeasm(outf, str);
   return TRUE;
}

/*  stgwrite
 *
 *  Writes the string "st" to the staging buffer or to the output file. In the
 *  case of writing to the staging buffer, the terminating byte of zero is
 *  copied too, but... the optimizer can only work on complete lines (not on
 *  fractions of it. Therefore if the string is staged, if the last character
 *  written to the buffer is a '\0' and the previous-to-last is not a '\n',
 *  the string is concatenated to the last string in the buffer (the '\0' is
 *  overwritten). This also means an '\n' used in the middle of a string isn't
 *  recognized and could give wrong results with the optimizer.
 *  Even when writing to the output file directly, all strings are buffered
 *  until a whole line is complete.
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (altered)
 *                     staging (referred to only)
 */
/**
 * @brief Writes a string to the staging buffer or directly to the output file.
 * @param st The null-terminated string to write.
 *
 * If `staging` is true, the string is appended to `stgbuf`. If the previous
 * string in the buffer did not end with a newline, this new string is
 * concatenated to it (overwriting the previous null terminator). A null
 * terminator is always added after the new string.
 *
 * If `staging` is false, the string is appended to a temporary line buffer
 * (`stgbuf` is reused for this). If this results in a complete line (ending
 * with a newline), the entire line buffer is written to the output file using
 * `filewrite()` and the line buffer is cleared.
 */
void
stgwrite(char *st)
{
   int                 len;

   CHECK_STGBUFFER(0);
   if (staging)
     {
	if (stgidx >= 2 && stgbuf[stgidx - 1] == '\0'
	    && stgbuf[stgidx - 2] != '\n')
	   stgidx -= 1;		/* overwrite last '\0' */
	while (*st != '\0')
	  {			/* copy to staging buffer */
	     CHECK_STGBUFFER(stgidx);
	     stgbuf[stgidx++] = *st++;
	  }			/* while */
	CHECK_STGBUFFER(stgidx);
	stgbuf[stgidx++] = '\0';
     }
   else
     {
	CHECK_STGBUFFER(strlen(stgbuf) + strlen(st) + 1);
	strcat(stgbuf, st);
	len = strlen(stgbuf);
	if (len > 0 && stgbuf[len - 1] == '\n')
	  {
	     filewrite(stgbuf);
	     stgbuf[0] = '\0';
	  }			/* if */
     }				/* if */
}

/*  stgout
 *
 *  Writes the staging buffer to the output file via stgstring() (for
 *  reversing expressions in the buffer) and stgopt() (for optimizing). It
 *  resets "stgidx".
 *
 *  Global references: stgidx  (altered)
 *                     stgbuf  (referred to only)
 *                     staging (referred to only)
 */
/**
 * @brief Processes and writes a portion of the staging buffer to the output file.
 * @param idx The starting index in `stgbuf` from which to process.
 *
 * This function is called to flush the code accumulated in the staging buffer
 * (from `stgbuf[idx]` to `stgbuf[stgidx-1]`) to the actual output file.
 * It first calls `stgstring()` to handle potential reordering of expressions
 * (like function arguments) and then `stgopt()` for peephole optimizations,
 * before finally writing to the file.
 * After processing, `stgidx` is reset to `idx`.
 * If staging is not active, this function does nothing.
 */
void
stgout(int idx)
{
   if (!staging)
      return;
   stgstring(&stgbuf[idx], &stgbuf[stgidx]);
   stgidx = idx;
}

/**
 * @struct argstack
 * @brief Structure to hold start and end pointers for a code segment (argument).
 *
 * Used by `stgstring` to manage expression blocks during reordering.
 */
typedef struct
{
   char               *start; /**< Pointer to the start of the code segment. */
   char               *end;   /**< Pointer to the end of the code segment (one past the last char). */
} argstack;

/*  stgstring
 *
 *  Analyses whether code strings should be output to the file as they appear
 *  in the staging buffer or whether portions of it should be re-ordered.
 *  Re-ordering takes place in function argument lists; Small passes arguments
 *  to functions from right to left. When arguments are "named" rather than
 *  positional, the order in the source stream is indeterminate.
 *  This function calls itself recursively in case it needs to re-order code
 *  strings, and it uses a private stack (or list) to mark the start and the
 *  end of expressions in their correct (reversed) order.
 *  In any case, stgstring() sends a block as large as possible to the
 *  optimizer stgopt().
 *
 *  In "reorder" mode, each set of code strings must start with the token
 *  sEXPRSTART, even the first. If the token sSTARTREORDER is represented
 *  by '[', sENDREORDER by ']' and sEXPRSTART by '|' the following applies:
 *     '[]...'     valid, but useless; no output
 *     '[|...]     valid, but useless; only one string
 *     '[|...|...] valid and useful
 *     '[...|...]  invalid, first string doesn't start with '|'
 *     '[|...|]    invalid
 */
/**
 * @brief Processes a segment of the staging buffer, handling reordering of expressions.
 * @param start Pointer to the beginning of the code segment in `stgbuf`.
 * @param end Pointer to the end of the code segment in `stgbuf`.
 *
 * This function is responsible for parsing special markers (sSTARTREORDER,
 * sENDREORDER, sEXPRSTART) within the provided code segment. If a reorder
 * block is found, it collects all expression sub-segments (marked by
 * sEXPRSTART) and then recursively calls itself to output these segments
 * in reverse order. This is primarily used for handling function arguments,
 * which are evaluated right-to-left in Small.
 *
 * Segments not part of a reorder block, or individual expressions after
 * reordering, are passed to `stgopt()` for optimization before being
 * written to the output file.
 *
 * The `argstack` array is used to store the start and end pointers of
 * expressions within a reorder block. For example:
 * If buffer contains: `sSTARTREORDER sEXPRSTART+0 "code_arg0" sEXPRSTART+1 "code_arg1" sENDREORDER`
 * `stack[0]` would point to `"code_arg0"`
 * `stack[1]` would point to `"code_arg1"`
 * Then `stgstring` would be called for `stack[1]`'s content, then `stack[0]`'s content.
 */
static void
stgstring(char *start, char *end)
{
   char               *ptr;
   int                 nest, argc, arg;
   argstack           *stack;

   while (start < end)
     {
	if (*start == sSTARTREORDER)
	  {
	     start += 1;	/* skip token */
	     /* allocate a argstack with sMAXARGS items */
	     stack = (argstack *) malloc(sMAXARGS * sizeof(argstack));
	     if (!stack)
		error(103);	/* insufficient memory */
	     nest = 1;		/* nesting counter */
	     argc = 0;		/* argument counter */
	     arg = -1;		/* argument index; no valid argument yet */
	     do
	       {
		  switch (*start)
		    {
		    case sSTARTREORDER:
		       nest++;
		       start++;
		       break;
		    case sENDREORDER:
		       nest--;
		       start++;
		       break;
		    default:
		       if ((*start & sEXPRSTART) == sEXPRSTART)
			 {
			    if (nest == 1)
			      {
				 if (arg >= 0)
				    stack[arg].end = start - 1;	/* finish previous argument */
				 arg = (unsigned char)*start - sEXPRSTART;
				 stack[arg].start = start + 1;
				 if (arg >= argc)
				    argc = arg + 1;
			      }	/* if */
			    start++;
			 }
		       else
			 {
			    start += strlen(start) + 1;
			 }	/* if */
		    }		/* switch */
	       }
	     while (nest);	/* enddo */
	     if (arg >= 0)
		stack[arg].end = start - 1;	/* finish previous argument */
	     while (argc > 0)
	       {
		  argc--;
		  stgstring(stack[argc].start, stack[argc].end);
	       }		/* while */
	     free(stack);
	  }
	else
	  {
	     ptr = start;
	     while (ptr < end && *ptr != sSTARTREORDER)
		ptr += strlen(ptr) + 1;
	     stgopt(start, ptr);
	     start = ptr;
	  }			/* if */
     }				/* while */
}

/*  stgdel
 *
 *  Scraps code from the staging buffer by resetting "stgidx" to "index".
 *
 *  Global references: stgidx (altered)
 *                     staging (referred to only)
 */
/**
 * @brief Deletes code from the staging buffer by resetting the current index.
 * @param idx The new value for `stgidx`. Code from this index to the previous
 *            `stgidx` is effectively discarded.
 * @param code_index The new value for the global `code_idx`. This parameter seems
 *                   to be updated directly, reflecting the change in generated code size.
 *
 * This function is used to scrap recently added code from the staging buffer,
 * for example, during optimization or when a tentative code generation path
 * is abandoned. It only has an effect if `staging` is active.
 */
void
stgdel(int idx, cell code_index)
{
   if (staging)
     {
	stgidx = idx;
	code_idx = code_index;
     }				/* if */
}

/**
 * @brief Retrieves the current state of the staging buffer.
 * @param[out] idx Pointer to store the current staging buffer index (`stgidx`).
 * @param[out] code_index Pointer to store the current code generation index (`code_idx`).
 * @return `staging` status (TRUE if staging is active, FALSE otherwise).
 *
 * This function allows other parts of the compiler to get the current write
 * position in the staging buffer and the overall code generation index.
 * The values are only meaningful if staging is active.
 */
int
stgget(int *idx, cell * code_index)
{
   if (staging)
     {
	*idx = stgidx;
	*code_index = code_idx;
     }				/* if */
   return staging;
}

/*  stgset
 *
 *  Sets staging on or off. If it's turned off, the staging buffer must be
 *  initialized to an empty string. If it's turned on, the routine makes sure
 *  the index ("stgidx") is set to 0 (it should already be 0).
 *
 *  Global references: staging  (altered)
 *                     stgidx   (altered)
 *                     stgbuf   (contents altered)
 */
/**
 * @brief Enables or disables code staging.
 * @param onoff If TRUE, staging is enabled. If FALSE, staging is disabled.
 *
 * When staging is turned ON:
 * - `stgidx` is reset to 0.
 * - Any content previously buffered in `stgbuf` (while staging was OFF,
 *   acting as a line buffer) is flushed to the output file.
 * - `stgbuf` is then cleared.
 *
 * When staging is turned OFF:
 * - `stgbuf` is cleared (it will be used as a line buffer by `stgwrite`).
 */
void
stgset(int onoff)
{
   staging = onoff;
   if (staging)
     {
	assert(stgidx == 0);
	stgidx = 0;
	CHECK_STGBUFFER(stgidx);
	/* write any contents that may be put in the buffer by stgwrite()
	 * when "staging" was 0
	 */
	if (stgbuf[0] != '\0')
	   filewrite(stgbuf);
     }				/* if */
   stgbuf[0] = '\0';
}

/* phopt_init
 * Initialize all sequence strings of the peehole optimizer. The strings
 * are embedded in the .EXE file in compressed format, here we expand
 * them (and allocate memory for the sequences).
 */
static SEQUENCE    *sequences; /**< Array of peephole optimizer rules. */

/**
 * @brief Initializes the peephole optimizer sequences.
 * @return TRUE on success, FALSE on memory allocation failure.
 *
 * This function allocates memory for the `sequences` array and populates it
 * by expanding compressed strings defined in `sequences_cmp`. Each sequence
 * consists of a "find" pattern, a "replace" pattern, and a "savesize" value.
 * The `sequences_cmp` array is expected to be defined elsewhere (e.g., in
 * embryo_cc_sc7.scp) and contains the compressed representations of these patterns.
 *
 * Example of `sequences_cmp` (conceptual):
 * ```c
 * // const COMPACT_SEQ sequences_cmp[] = {
 * //   { "compressed_find_pattern1", "compressed_replace_pattern1", 2 },
 * //   { "compressed_find_pattern2", "compressed_replace_pattern2", 4 },
 * //   { NULL, NULL, 0 }
 * // };
 * ```
 * After initialization, `sequences` would look like:
 * ```c
 * // sequences[0] = { .find="expanded_find1", .replace="expanded_replace1", .savesize=2 };
 * // sequences[1] = { .find="expanded_find2", .replace="expanded_replace2", .savesize=4 };
 * // ...
 * ```
 */
int
phopt_init(void)
{
   int                 number, i, len;
   char                str[160];

   /* count number of sequences */
   for (number = 0; sequences_cmp[number].find; number++)
      /* nothing */ ;
   number++;			/* include an item for the NULL terminator */

   if (!(sequences = (SEQUENCE *)malloc(number * sizeof(SEQUENCE))))
      return FALSE;

   /* pre-initialize all to NULL (in case of failure) */
   for (i = 0; i < number; i++)
     {
	sequences[i].find = NULL;
	sequences[i].replace = NULL;
	sequences[i].savesize = 0;
     }				/* for */

   /* expand all strings */
   for (i = 0; i < number - 1; i++)
     {
	len =
	   strexpand(str, (unsigned char *)sequences_cmp[i].find, sizeof str,
		     SCPACK_TABLE);
	assert(len <= (int)(sizeof(str)));
	assert(len == (int)(strlen(str) + 1));
	sequences[i].find = (char *)malloc(len);
	if (sequences[i].find)
	   strcpy(sequences[i].find, str);
	len =
	   strexpand(str, (unsigned char *)sequences_cmp[i].replace, sizeof str,
		     SCPACK_TABLE);
	assert(len <= (int)(sizeof(str)));
	assert(len == (int)(strlen(str) + 1));
	sequences[i].replace = (char *)malloc(len);
	if (sequences[i].replace)
	   strcpy(sequences[i].replace, str);
	sequences[i].savesize = sequences_cmp[i].savesize;
	if (!sequences[i].find || !sequences[i].replace)
	   return phopt_cleanup();
     }				/* for */

   return TRUE;
}

/**
 * @brief Cleans up resources used by the peephole optimizer.
 * @return Always returns FALSE. (The return value seems to indicate failure state for the caller,
 *         but the function itself doesn't have a failure mode other than `sequences` being NULL already).
 *
 * Frees the memory allocated for each "find" and "replace" string in the
 * `sequences` array, and then frees the `sequences` array itself.
 * Sets `sequences` to NULL.
 */
int
phopt_cleanup(void)
{
   int                 i;

   if (sequences)
     {
	i = 0;
	while (sequences[i].find || sequences[i].replace)
	  {
	     if (sequences[i].find)
		free(sequences[i].find);
	     if (sequences[i].replace)
		free(sequences[i].replace);
	     i++;
	  }			/* while */
	free(sequences);
	sequences = NULL;
     }				/* if */
   return FALSE;
}

#define _maxoptvars     4
#define _aliasmax       10	/* a 32-bit number can be represented in
				 * 9 decimal digits */

/**
 * @brief Attempts to match a code sequence against a peephole optimizer pattern.
 * @param start Pointer to the beginning of the code sequence in the staging buffer.
 * @param end Pointer to the end of the available code in the staging buffer.
 * @param pattern The peephole optimizer pattern to match.
 *                Patterns can contain:
 *                - Literal characters: Must match exactly (case-insensitive).
 *                - `%n`: Matches an alphanumeric symbol (e.g., register, label, number).
 *                        The matched symbol is stored in `symbols[n-1]`.
 *                        If `%n` appears again, it must match the same symbol.
 *                - ` ` (space): Matches one or more whitespace characters (space or tab).
 *                - `!`: Matches a newline character (`\n`) followed by a null terminator (`\0`).
 *                       It also handles skipping leading/trailing whitespace around newlines.
 * @param[out] symbols A 2D array to store captured symbols. `symbols[i]` will hold
 *                     the string matched by `% (i+1)`.
 *                     Example: `symbols[0]` for `%1`, `symbols[1]` for `%2`.
 *                     Each symbol string can be up to `_aliasmax` characters long.
 * @param[out] match_length Pointer to store the length of the matched sequence in `start`.
 * @return TRUE if the pattern matches, FALSE otherwise.
 *
 * This function iterates through the `pattern` and the input `start` string.
 * It handles whitespace flexibly and captures variable parts of the assembly
 * code (like register names or literal values) using the `%n` syntax.
 */
static int
matchsequence(char *start, char *end, char *pattern,
	      char symbols[_maxoptvars][_aliasmax + 1], int *match_length)
{
   int                 var, i;
   char                str[_aliasmax + 2];
   char               *start_org = start;

   *match_length = 0;
   for (var = 0; var < _maxoptvars; var++)
      symbols[var][0] = '\0';

   while (*start == '\t' || *start == ' ')
      start++;
   while (*pattern)
     {
	if (start >= end)
	   return FALSE;
	switch (*pattern)
	  {
	  case '%':		/* new "symbol" */
	     pattern++;
	     assert(sc_isdigit(*pattern));
	     var = atoi(pattern) - 1;
	     assert(var >= 0 && var < _maxoptvars);
	     assert(alphanum(*start));
	     for (i = 0; start < end && alphanum(*start); i++, start++)
	       {
		  assert(i <= _aliasmax);
		  str[i] = *start;
	       }		/* for */
	     str[i] = '\0';
	     if (symbols[var][0] != '\0')
	       {
		  if (strcmp(symbols[var], str) != 0)
		     return FALSE;	/* symbols should be identical */
	       }
	     else
	       {
		  strcpy(symbols[var], str);
	       }		/* if */
	     break;
	  case ' ':
	     if (*start != '\t' && *start != ' ')
		return FALSE;
	     while ((start < end && *start == '\t') || *start == ' ')
		start++;
	     break;
	  case '!':
	     while ((start < end && *start == '\t') || *start == ' ')
		start++;	/* skip trailing white space */
	     if (*start != '\n')
		return FALSE;
	     assert(*(start + 1) == '\0');
	     start += 2;	/* skip '\n' and '\0' */
	     if (*(pattern + 1) != '\0')
		while ((start < end && *start == '\t') || *start == ' ')
		   start++;	/* skip leading white space of next instruction */
	     break;
	  default:
	     if (tolower(*start) != tolower(*pattern))
		return FALSE;
	     start++;
	  }			/* switch */
	pattern++;
     }				/* while */

   *match_length = (int)(start - start_org);
   return TRUE;
}

/**
 * @brief Constructs a replacement string based on a pattern and captured symbols.
 * @param pattern The replacement pattern. It can contain:
 *                - Literal characters: Copied directly to the output.
 *                - `%n`: Replaced by the symbol stored in `symbols[n-1]`.
 *                - `!`: Replaced by `\n\0` (newline and null terminator). If more
 *                       pattern follows, a `\t` (tab) is prepended to the next line.
 * @param symbols A 2D array containing the captured symbols from `matchsequence`.
 *                `symbols[i]` contains the string for `% (i+1)`.
 * @param[out] repl_length Pointer to store the length of the generated replacement string.
 * @return A newly allocated string containing the replacement code, or NULL on error (malloc failure).
 *         The caller is responsible for freeing this string.
 *
 * This function takes a replacement pattern (e.g., "mov %1, %2!add %1, 1") and the
 * symbols captured by `matchsequence` (e.g., symbols[0]="eax", symbols[1]="ebx")
 * to generate the new assembly code (e.g., "\tmov eax, ebx\n\0\tadd eax, 1").
 * It prepends a tab `\t` to the beginning of the string and after each `!`
 * if more pattern follows.
 */
static char        *
replacesequence(char *pattern, char symbols[_maxoptvars][_aliasmax + 1],
		int *repl_length)
{
   char               *sptr;
   int                 var;
   char               *buffer;

   /* calculate the length of the new buffer
    * this is the length of the pattern plus the length of all symbols (note
    * that the same symbol may occur multiple times in the pattern) plus
    * line endings and startings ('\t' to start a line and '\n\0' to end one)
    */
   assert(repl_length != NULL);
   *repl_length = 0;
   sptr = pattern;
   while (*sptr)
     {
	switch (*sptr)
	  {
	  case '%':
	     sptr++;		/* skip '%' */
	     assert(sc_isdigit(*sptr));
	     var = atoi(sptr) - 1;
	     assert(var >= 0 && var < _maxoptvars);
	     assert(symbols[var][0] != '\0');	/* variable should be defined */
	     *repl_length += strlen(symbols[var]);
	     break;
	  case '!':
	     *repl_length += 3;	/* '\t', '\n' & '\0' */
	     break;
	  default:
	     *repl_length += 1;
	  }			/* switch */
	sptr++;
     }				/* while */

   /* allocate a buffer to replace the sequence in */
   if (!(buffer = malloc(*repl_length)))
     {
	error(103);
	return NULL;
     }

   /* replace the pattern into this temporary buffer */
   sptr = buffer;
   *sptr++ = '\t';		/* the "replace" patterns do not have tabs */
   while (*pattern)
     {
	assert((int)(sptr - buffer) < *repl_length);
	switch (*pattern)
	  {
	  case '%':
	     /* write out the symbol */
	     pattern++;
	     assert(sc_isdigit(*pattern));
	     var = atoi(pattern) - 1;
	     assert(var >= 0 && var < _maxoptvars);
	     assert(symbols[var][0] != '\0');	/* variable should be defined */
	     strcpy(sptr, symbols[var]);
	     sptr += strlen(symbols[var]);
	     break;
	  case '!':
	     /* finish the line, optionally start the next line with an indent */
	     *sptr++ = '\n';
	     *sptr++ = '\0';
	     if (*(pattern + 1) != '\0')
		*sptr++ = '\t';
	     break;
	  default:
	     *sptr++ = *pattern;
	  }			/* switch */
	pattern++;
     }				/* while */

   assert((int)(sptr - buffer) == *repl_length);
   return buffer;
}

/**
 * @brief Replaces a portion of a string with another string.
 * @param dest The destination string (buffer) where the replacement occurs.
 *             This buffer is modified in place.
 * @param replace The new string to insert.
 * @param sub_length The length of the section in `dest` to be replaced.
 * @param repl_length The length of the `replace` string.
 * @param dest_length The total length of the content in `dest` starting from
 *                    the part to be modified. This is used for `memmove`.
 *
 * This function modifies `dest` by replacing `sub_length` characters starting
 * at `dest` with the `repl_length` characters from `replace`.
 * It handles cases where the replacement string is shorter or longer than
 * the original substring by using `memmove` to shift the subsequent part of
 * `dest`.
 *
 * Example:
 * dest = "abcdefgh", replace = "XYZ", sub_length = 2 (for "bc"), repl_length = 3, dest_length = 7 ("bcdefgh")
 * 1. "bc" (len 2) is replaced by "XYZ" (len 3). Offset = 2 - 3 = -1.
 * 2. memmove(dest - (-1), dest, 7) -> memmove(dest+1, dest, 7) - this seems wrong.
 *    Let's re-evaluate: dest points to 'a'. The part to replace is "bc".
 *    The part after "bc" is "defgh".
 *    If sub_length > repl_length (e.g. replace "bc" with "X"):
 *      offset = 1. memmove(dest, dest + 1, dest_length - 1)
 *      Original: [A][B][C][D][E][F][G][H]
 *                 ^dest
 *      Replacing "BC" with "X". sub_length=2, repl_length=1. offset=1.
 *      memmove(dest_ptr_to_B, dest_ptr_to_B + 1, ...);
 *      This means shifting "DEFGH" to where "CDEFGH" was.
 *
 * Correct logic:
 * `dest` is the pointer to the start of the substring to be replaced.
 * `dest_length` is the length from `dest` to the end of the relevant buffer part.
 *
 * If `offset > 0` (replacement is shorter):
 *   `memmove(dest + repl_length, dest + sub_length, dest_length - sub_length);`
 *   Effectively, `memmove(dest_after_new_content, dest_after_old_content, remaining_bytes);`
 * If `offset < 0` (replacement is longer):
 *   `memmove(dest + repl_length, dest + sub_length, dest_length - sub_length);`
 *   This needs to be done carefully to avoid overwriting data if source and destination overlap.
 *   The existing code `memmove(dest - offset, dest, dest_length)` for insert seems to shift the original `dest` content
 *   to make space, then `memcpy` overwrites.
 *   If `dest` is `[S1 S2 S3 R1 R2 E1 E2]`, `sub_length` is for `R1 R2`. `repl_length` is for new content.
 *   `dest_length` is for `R1 R2 E1 E2`.
 *   If inserting (offset < 0, e.g. repl_length = 3, sub_length = 2):
 *     `memmove(dest_of_R1 - offset, dest_of_R1, length_of_R1R2E1E2)`
 *     `memmove(dest_of_R1 + 1, dest_of_R1, length_of_R1R2E1E2)`
 *     This shifts `R1 R2 E1 E2` one position to the right, starting at `dest_of_R1+1`.
 *     Then `memcpy(dest_of_R1, replace, repl_length)` copies new content.
 *
 * The current implementation:
 * `offset = sub_length - repl_length;`
 * If `offset > 0`: `memmove(dest, dest + offset, dest_length - offset);`
 *   (e.g. sub=3, repl=1 -> offset=2. `memmove(dest, dest+2, dest_length-2)`)
 *   This shifts the tail (`dest_length-sub_length` bytes starting at `dest+sub_length`)
 *   left by `offset` bytes.
 * If `offset < 0`: `memmove(dest - offset, dest, dest_length);`
 *   (e.g. sub=1, repl=3 -> offset=-2. `memmove(dest+2, dest, dest_length)`)
 *   This shifts the original content at `dest` (of `dest_length` bytes) right by `-offset` bytes.
 * Then `memcpy(dest, replace, repl_length);`
 * This seems correct for in-place modification of a larger buffer segment.
 */
static void
strreplace(char *dest, char *replace, int sub_length, int repl_length,
	   int dest_length)
{
   int                 offset = sub_length - repl_length;

   if (offset > 0)		/* delete a section */
      memmove(dest, dest + offset, dest_length - offset);
   else if (offset < 0)		/* insert a section */
      memmove(dest - offset, dest, dest_length);
   memcpy(dest, replace, repl_length);
}

/*  stgopt
 *
 *  Optimizes the staging buffer by checking for series of instructions that
 *  can be coded more compact. The routine expects the lines in the staging
 *  buffer to be separated with '\n' and '\0' characters.
 *
 *  The longest sequences must be checked first. (This implies `sequences` array should be sorted by find pattern length or complexity).
 */
/**
 * @brief Optimizes a segment of the staging buffer using peephole rules.
 * @param start Pointer to the beginning of the code segment in `stgbuf` to optimize.
 * @param end Pointer to the end of the code segment in `stgbuf`.
 *
 * This function iterates through the provided code segment (line by line, where
 * lines are null-terminated strings typically ending in `\n\0`). For each line/sequence,
 * it tries to match it against the peephole optimization rules stored in the
 * `sequences` array (initialized by `phopt_init`).
 *
 * If a match is found via `matchsequence()`:
 * 1. `replacesequence()` is called to generate the optimized code string.
 * 2. `strreplace()` is used to replace the original code in the buffer with the
 *    optimized version.
 * 3. The `end` pointer of the buffer segment is adjusted if the replacement
 *    changed the length.
 * 4. The global `code_idx` (presumably tracking code size) is adjusted by
 *    `sequences[seq].savesize`.
 * 5. The matching process for the current position restarts from the beginning
 *    of the `sequences` array, as a replacement might enable further optimizations.
 *
 * If no match is found for the current line with any rule, the line is written
 * to the output file using `filewrite()`.
 *
 * Optimization is skipped if `(sc_debug & sNOOPTIMIZE)` is true or if
 * `sc_status` is not `statWRITE`.
 */
static void
stgopt(char *start, char *end)
{
   char                symbols[_maxoptvars][_aliasmax + 1];
   int                 seq, match_length, repl_length;

   assert(sequences != NULL);
   while (start < end)
     {
	if ((sc_debug & sNOOPTIMIZE) != 0 || sc_status != statWRITE)
	  {
	     /* do not match anything if debug-level is maximum */
	     filewrite(start);
	  }
	else
	  {
	     seq = 0;
	     while (sequences[seq].find)
	       {
		  assert(seq >= 0);
		  if (matchsequence
		      (start, end, sequences[seq].find, symbols, &match_length))
		    {
		       char               *replace =
			  replacesequence(sequences[seq].replace, symbols,
					  &repl_length);
		       /* If the replacement is bigger than the original section, we may need
		        * to "grow" the staging buffer. This is quite complex, due to the
		        * re-ordering of expressions that can also happen in the staging
		        * buffer. In addition, it should not happen: the peephole optimizer
		        * must replace sequences with *shorter* sequences, not longer ones.
		        * So, I simply forbid sequences that are longer than the ones they
		        * are meant to replace.
		        */
		       assert(match_length >= repl_length);
		       if (match_length >= repl_length)
			 {
			    strreplace(start, replace, match_length,
				       repl_length, (int)(end - start));
			    end -= match_length - repl_length;
			    free(replace);
			    code_idx -= sequences[seq].savesize;
			    seq = 0;	/* restart search for matches */
			 }
		       else
			 {
			    /* actually, we should never get here (match_length<repl_length) */
			    assert(0);
			    seq++;
			 }	/* if */
		    }
		  else
		    {
		       seq++;
		    }		/* if */
	       }		/* while */
	     assert(sequences[seq].find == NULL);
	     filewrite(start);
	  }			/* if */
	assert(start < end);
	start += strlen(start) + 1;	/* to next string */
     }				/* while (start<end) */
}

#undef SCPACK_TABLE
