/* Definitions for CPP library.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Written by Per Bothner, 1994-95.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef HOST_BITS_PER_WIDE_INT

#if HOST_BITS_PER_LONG > HOST_BITS_PER_INT
#define HOST_BITS_PER_WIDE_INT HOST_BITS_PER_LONG
#define HOST_WIDE_INT long
#else
#define HOST_BITS_PER_WIDE_INT HOST_BITS_PER_INT
#define HOST_WIDE_INT int
#endif

#endif

#define STATIC_BUFFERS

/** @file cpplib.h
 *  @brief Definitions for the C Preprocessor (CPP) library.
 *
 *  This file contains the primary data structures, enumerations, and
 *  function prototypes used by the CPP library.
 */

/** @struct cpp_reader
 *  @brief Main structure representing the state of the C preprocessor.
 *
 *  This structure holds all the necessary information for the preprocessing
 *  of a C/C++ file, including input buffers, options, error state, and
 *  macro definitions.
 */
typedef struct cpp_reader cpp_reader;

/** @struct cpp_buffer
 *  @brief Represents a single input buffer, typically a file or a macro expansion.
 *
 *  Buffers are stacked to handle #include directives and macro expansions.
 *  Each buffer maintains its own read pointers, line/column information,
 *  and associated filename.
 */
typedef struct cpp_buffer cpp_buffer;

/** @struct cpp_options
 *  @brief Holds the command-line options and other configuration for the preprocessor.
 *
 *  This includes settings for include paths, warning levels, language dialects,
 *  and output generation.
 */
typedef struct cpp_options cpp_options;

/** @enum cpp_token
 *  @brief Enumeration of token types recognized by the preprocessor.
 */
enum cpp_token {
   CPP_EOF = -1,      /**< End of file or input stream. */
   CPP_OTHER = 0,     /**< A character or sequence not forming a specific token type (e.g., operators like '+', ';'). */
   CPP_COMMENT = 1,   /**< A C or C++ style comment. */
   CPP_HSPACE,        /**< Horizontal whitespace (spaces, tabs). */
   CPP_VSPACE,			/* newlines and #line directives */ /**< Vertical whitespace (newlines, or a #line directive which implies a newline). */
   CPP_NAME,          /**< An identifier. */
   CPP_NUMBER,        /**< A numeric literal. */
   CPP_CHAR,          /**< A character constant (e.g., 'a'). */
   CPP_STRING,
   CPP_DIRECTIVE,
   CPP_LPAREN,			/* "(" */
   CPP_RPAREN,			/* ")" */
   CPP_LBRACE,			/* "{" */
   CPP_RBRACE,			/* "}" */
   CPP_COMMA,			/* "," */
   CPP_SEMICOLON,		/* ";" */
   CPP_3DOTS,			/* "..." */
   /* POP_TOKEN is returned when we've popped a cpp_buffer. */
   CPP_POP /**< Indicates that a buffer (file or macro) has been fully processed and popped. */
};

/** @typedef parse_underflow_t
 *  @brief Function pointer type for handling buffer underflow.
 *  @param pfile The current preprocessor state.
 *  @return The next token after handling the underflow (e.g., by loading more data or switching buffers).
 */
typedef enum cpp_token (*parse_underflow_t) (cpp_reader *);

/** @typedef parse_cleanup_t
 *  @brief Function pointer type for cleaning up a buffer when it's popped.
 *  @param pbuf The buffer to be cleaned up.
 *  @param pfile The current preprocessor state.
 *  @return 0 on success, non-zero on failure.
 */
typedef int         (*parse_cleanup_t) (cpp_buffer *, cpp_reader *);

/** @struct parse_marker
 *  @brief A structure to mark a position in a buffer for backtracking.
 *
 *  This is used, for example, when tentatively parsing macro arguments or
 *  lookahead for directive names.
 */
struct parse_marker {
   cpp_buffer         *buf;       /**< The buffer this marker refers to. */
   struct parse_marker *next;    /**< Pointer to the next marker in a potential list. */
   int                 position; /**< The character offset within the buffer. */
};

/**
 * @brief Handles command-line options.
 * @param pfile The preprocessor state.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Number of handled arguments.
 */
extern int          cpp_handle_options(cpp_reader * pfile, int, char **);

/**
 * @brief Gets the next token from the input stream.
 * @param pfile The preprocessor state.
 * @return The type of the token found.
 */
extern enum cpp_token cpp_get_token(cpp_reader * pfile);

/**
 * @brief Skips horizontal whitespace (spaces, tabs) and comments.
 * @param pfile The preprocessor state.
 */
extern void         cpp_skip_hspace(cpp_reader * pfile);

/* Maintain and search list of included files, for #import.  */

#define IMPORT_HASH_SIZE 31 /**< Size of the hash table for imported files. */

/** @struct import_file
 *  @brief Structure to keep track of files included via #import.
 *
 *  This helps in avoiding redundant processing of the same file if #import is used.
 *  It stores the filename, inode, and device number to uniquely identify a file.
 */
struct import_file {
   char               *name;    /**< The full path of the imported file. */
   ino_t               inode;  /**< The inode number of the file. */
   dev_t               dev;    /**< The device ID of the file system containing the file. */
   struct import_file *next;   /**< Pointer to the next imported file in a hash chain. */
};

/* If we have a huge buffer, may need to cache more recent counts */
/**
 * @brief Macro to get the base pointer for line counting within a buffer.
 * @param BUF The cpp_buffer.
 * @return Pointer to the character in the buffer from which the current line number (BUF->lineno) is counted.
 */
#define CPP_LINE_BASE(BUF) ((BUF)->buf + (BUF)->line_base)

/** @enum dump_type
 *  @brief Enumeration for different macro dumping modes (e.g., -dM, -dN).
 */
enum dump_type {
   dump_none = 0,    /**< Do not dump macros. */
   dump_only,        /**< Dump only macro definitions, inhibit normal output. */
   dump_names,       /**< Dump macro names as they are defined. */
   dump_definitions  /**< Dump full macro definitions as they are defined. */
};

/** @struct cpp_buffer
 *  @brief Represents a single input buffer, typically a file or a macro expansion.
 *
 *  Buffers are stacked to handle #include directives and macro expansions.
 *  Each buffer maintains its own read pointers, line/column information,
 *  and associated filename.
 */
struct cpp_buffer {
   unsigned char      *buf;
   unsigned char      *cur;
   unsigned char      *rlimit;	/* end of valid data */
   unsigned char      *alimit;	/* end of allocated buffer */
   unsigned char      *prev;	/* start of current token */

   const char         *fname;
   /* Filename specified with #line command.  */
   const char         *nominal_fname;

   /* Record where in the search path this file was found.
    * For #include_next.  */
   struct file_name_list *dir;

   long                line_base;
   long                lineno;	/* Line number at CPP_LINE_BASE. */
   long                colno;	/* Column number at CPP_LINE_BASE. */
#ifndef STATIC_BUFFERS
   cpp_buffer         *chain;
#endif
   parse_underflow_t   underflow;
   parse_cleanup_t     cleanup;
   void               *data;
   struct parse_marker *marks;
   /* Value of if_stack at start of this file.
    * Used to prohibit unmatched #endif (etc) in an include file.  */
   struct if_stack    *if_stack;

   /* True if this is a header file included using <FILENAME>.  */
   char                system_header_p;
   char                seen_eof;

   /* True if buffer contains escape sequences.
    * Currently there are are only two kind:
    * "@-" means following identifier should not be macro-expanded.
    * "@ " means a token-separator.  This turns into " " in final output
    * if not stringizing and needed to separate tokens; otherwise nothing.
    * "@@" means a normal '@'.
    * (An '@' inside a string stands for itself and is never an escape.) */
   char                has_escapes; /**< True if the buffer may contain special '@' escape sequences. */
};

/** @struct cpp_pending
 *  @brief Structure to hold pending command-line actions like -D, -U, -A, -include.
 *  These are processed in order after initial setup.
 */
struct cpp_pending;		/* Forward declaration - for C++. */

/** @struct file_name_map_list
 *  @brief Structure for file name mapping, used on systems with filename restrictions.
 */
struct file_name_map_list;

/** @typedef ASSERTION_HASHNODE
 *  @brief Typedef for the assertion hash node structure.
 */
typedef struct assertion_hashnode ASSERTION_HASHNODE;

#define ASSERTION_HASHSIZE 37 /**< Size of the hash table for #assert directives. */

#ifdef STATIC_BUFFERS
/* Maximum nesting of cpp_buffers.  We use a static limit, partly for
   efficiency, and partly to limit runaway recursion.  */
#define CPP_STACK_MAX 200 /**< Maximum depth of buffer stack (includes/macro expansions). */
#endif

/** @struct cpp_reader
 *  @brief Main structure representing the state of the C preprocessor.
 *
 *  This structure holds all the necessary information for the preprocessing
 *  of a C/C++ file, including input buffers, options, error state, and
 *  macro definitions.
 */
struct cpp_reader {
   unsigned char      *limit;
   parse_underflow_t   get_token;
   cpp_buffer         *buffer;
#ifdef STATIC_BUFFERS
   cpp_buffer          buffer_stack[CPP_STACK_MAX];
#endif

   int                 errors;	/* Error counter for exit code */
   void               *data;

   unsigned char      *token_buffer;
   int                 token_buffer_size;

   /* Line where a newline was first seen in a string constant.  */
   int                 multiline_string_line;

   /* Current depth in #include directives that use <...>.  */
   int                 system_include_depth;

   /* List of included files that contained #pragma once.  */
   struct file_name_list *dont_repeat_files;

   /* List of other included files.
    * If ->control_macro if nonzero, the file had a #ifndef
    * around the entire contents, and ->control_macro gives the macro name.  */
   struct file_name_list *all_include_files;

   /* Current maximum length of directory names in the search path
    * for include files.  (Altered as we get more of them.)  */
   int                 max_include_len;

   /* Hash table of files already included with #include or #import.  */
   struct import_file *import_hash_table[IMPORT_HASH_SIZE];

   struct if_stack    *if_stack;

   /* Nonzero means we are inside an IF during a -pcp run.  In this mode
    * macro expansion is done, and preconditions are output for all macro
    * uses requiring them. */
   char                pcp_inside_if;

   /* Nonzero means we have printed (while error reporting) a list of
    * containing files that matches the current status. */
   char                input_stack_listing_current;

   /* If non-zero, macros are not expanded. */
   char                no_macro_expand;

   /* Print column number in error messages. */
   char                show_column;

   /* We're printed a warning recommending against using #import. */
   char                import_warning;

   /* If true, character between '<' and '>' are a single (string) token. */
   char                parsing_include_directive;

   /* True if escape sequences (as described for has_escapes in
    * parse_buffer) should be emitted. */
   char                output_escapes;

   /* 0: Have seen non-white-space on this line.
    * 1: Only seen white space so far on this line.
    * 2: Only seen white space so far in this file. */
   char                only_seen_white;

   /* Nonzero means this file was included with a -imacros or -include
    * command line and should not be recorded as an include file.  */

   int                 no_record_file;

   long                lineno;

   struct tm          *timebuf;

   ASSERTION_HASHNODE *assertion_hashtab[ASSERTION_HASHSIZE];

   /* Buffer of -M output.  */
   char               *deps_buffer;

   /* Number of bytes allocated in above.  */
   int                 deps_allocated_size;

   /* Number of bytes used.  */
   int                 deps_size;

   /* Number of bytes since the last newline.  */
   int                 deps_column; /**< Current column for dependency output, for line wrapping. */
};

/** Peeks at the current character in the buffer without advancing. */
#define CPP_BUF_PEEK(BUFFER) \
  ((BUFFER)->cur < (BUFFER)->rlimit ? *(BUFFER)->cur : EOF)
/** Gets the current character from the buffer and advances the read pointer. */
#define CPP_BUF_GET(BUFFER) \
  ((BUFFER)->cur < (BUFFER)->rlimit ? *(BUFFER)->cur++ : EOF)
/** Advances the read pointer in the buffer by N characters. */
#define CPP_FORWARD(BUFFER, N) ((BUFFER)->cur += (N))

/** Calculates the number of characters currently written to PFILE's token_buffer. */
#define CPP_WRITTEN(PFILE) ((PFILE)->limit - (PFILE)->token_buffer)
/** Gets a pointer to the current write position in PFILE's token_buffer. */
#define CPP_PWRITTEN(PFILE) ((PFILE)->limit)

/** Ensures PFILE->token_buffer has space for at least N more characters, growing it if necessary. */
#define CPP_RESERVE(PFILE, N) \
  ((unsigned int)(CPP_WRITTEN (PFILE) + N) > (unsigned int) (PFILE)->token_buffer_size \
   && (cpp_grow_buffer (PFILE, N), 0))

/** Appends string STR (of length N) to PFILE's output buffer, assuming there is enough space. */
#define CPP_PUTS_Q(PFILE, STR, N) \
  do { memcpy ((PFILE)->limit, STR, (N)); (PFILE)->limit += (N); } while(0)
/** Appends string STR (of length N) to PFILE's output buffer, making space if needed. */
#define CPP_PUTS(PFILE, STR, N) \
  do { CPP_RESERVE(PFILE, N); CPP_PUTS_Q(PFILE, STR,N); } while(0)
/** Appends character CH to PFILE's output buffer, assuming sufficient space. */
#define CPP_PUTC_Q(PFILE, CH) (*(PFILE)->limit++ = (CH))
/** Appends character CH to PFILE's output buffer, making space if need be. */
#define CPP_PUTC(PFILE, CH) \
  do { CPP_RESERVE (PFILE, 1); CPP_PUTC_Q (PFILE, CH); } while(0)
/** Ensures PFILE->limit is followed by '\0', assuming space. */
#define CPP_NUL_TERMINATE_Q(PFILE) (*(PFILE)->limit = 0)
/** Ensures PFILE->limit is followed by '\0', making space if needed. */
#define CPP_NUL_TERMINATE(PFILE) \
  do { CPP_RESERVE(PFILE, 1); *(PFILE)->limit = 0; } while(0)
/** Adjusts the write pointer in PFILE's token_buffer by DELTA. */
#define CPP_ADJUST_WRITTEN(PFILE,DELTA) ((PFILE)->limit += (DELTA))
/** Sets the write position in PFILE's token_buffer to an absolute offset N. */
#define CPP_SET_WRITTEN(PFILE,N) ((PFILE)->limit = (PFILE)->token_buffer + (N))

/** Accesses the cpp_options structure from a cpp_reader. */
#define CPP_OPTIONS(PFILE) ((cpp_options*)(PFILE)->data)
/** Accesses the current cpp_buffer from a cpp_reader. */
#define CPP_BUFFER(PFILE) ((PFILE)->buffer)
#ifdef STATIC_BUFFERS
/** Accesses the previous (outer) cpp_buffer in the static stack. */
#define CPP_PREV_BUFFER(BUFFER) ((BUFFER)+1)
/** Represents the null or bottom buffer in the static stack. */
#define CPP_NULL_BUFFER(PFILE) (&(PFILE)->buffer_stack[CPP_STACK_MAX])
#else
/** Accesses the previous (outer) cpp_buffer in the linked list. */
#define CPP_PREV_BUFFER(BUFFER) ((BUFFER)->chain)
/** Represents the null or bottom buffer in the linked list. */
#define CPP_NULL_BUFFER(PFILE) ((cpp_buffer*)0)
#endif

/** @struct cpp_options
 *  @brief Holds the command-line options and other configuration for the preprocessor.
 *
 *  This includes settings for include paths, warning levels, language dialects,
 *  and output generation. It is typically accessed via `CPP_OPTIONS(pfile)`.
 */
struct cpp_options {
   const char         *in_fname;

   /* Name of output file, for error messages.  */
   const char         *out_fname;

   struct file_name_map_list *map_list;

   /* Non-0 means -v, so print the full set of include dirs.  */
   char                verbose;

   /* Nonzero means use extra default include directories for C++.  */

   char                cplusplus;

   /* Nonzero means handle cplusplus style comments */

   char                cplusplus_comments;

   /* Nonzero means handle #import, for objective C.  */

   char                objc;

   /* Nonzero means this is an assembly file, and allow
    * unknown directives, which could be comments.  */

   int                 lang_asm;

   /* Nonzero means turn NOTREACHED into #pragma NOTREACHED etc */

   char                for_lint;

   /* Nonzero means handle CHILL comment syntax
    * and output CHILL string delimiter for __DATE___ etc. */

   char                chill;

   /* Nonzero means copy comments into the output file.  */

   char                put_out_comments;

   /* Nonzero means don't process the ANSI trigraph sequences.  */

   char                no_trigraphs;

   /* Nonzero means print the names of included files rather than
    * the preprocessed output.  1 means just the #include "...",
    * 2 means #include <...> as well.  */

   char                print_deps;

   /* Nonzero if missing .h files in -M output are assumed to be generated
    * files and not errors.  */

   char                print_deps_missing_files;

   /* If true, fopen (deps_file, "ab") else fopen (deps_file, "wb"). */
   char                print_deps_append;

   /* Nonzero means print names of header files (-H).  */

   char                print_include_names;

   /* Nonzero means try to make failure to fit ANSI C an error.  */

   char                pedantic_errors;

   /* Nonzero means don't print warning messages.  -w.  */

   char                inhibit_warnings;

   /* Nonzero means warn if slash-star appears in a comment.  */

   char                warn_comments;

   /* Nonzero means warn if there are any trigraphs.  */

   char                warn_trigraphs;

   /* Nonzero means warn if #import is used.  */

   char                warn_import;

   /* Nonzero means warn if a macro argument is (or would be)
    * stringified with -traditional.  */

   char                warn_stringify;

   /* Nonzero means turn warnings into errors.  */

   char                warnings_are_errors;

   /* Nonzero causes output not to be done,
    * but directives such as #define that have side effects
    * are still obeyed.  */

   char                no_output;

   /* Nonzero means don't output line number information.  */

   char                no_line_commands;

/* Nonzero means output the text in failing conditionals,
   inside #failed ... #endfailed.  */

   char                output_conditionals;

   /* Nonzero means -I- has been seen,
    * so don't look for #include "foo" the source-file directory.  */
   char                ignore_srcdir;

/* Zero means dollar signs are punctuation.
   -$ stores 0; -traditional may store 1.  Default is 1 for VMS, 0 otherwise.
   This must be 0 for correct processing of this ANSI C program:
	#define foo(a) #a
	#define lose(b) foo (b)
	#define test$
	lose (test)	*/
   char                dollars_in_ident;
#ifndef DOLLARS_IN_IDENTIFIERS
#define DOLLARS_IN_IDENTIFIERS 1
#endif

   /* Nonzero means try to imitate old fashioned non-ANSI preprocessor.  */
   char                traditional;

   /* Nonzero means give all the error messages the ANSI standard requires.  */
   char                pedantic;

   char                done_initializing;

   struct file_name_list *include;	/* First dir to search */
   /* First dir to search for <file> */
   /* This is the first element to use for #include <...>.
    * If it is 0, use the entire chain for such includes.  */
   struct file_name_list *first_bracket_include;
   /* This is the first element in the chain that corresponds to
    * a directory of system header files.  */
   struct file_name_list *first_system_include;
   struct file_name_list *last_include;	/* Last in chain */

   /* Chain of include directories to put at the end of the other chain.  */
   struct file_name_list *after_include;
   struct file_name_list *last_after_include;	/* Last in chain */

   /* Chain to put at the start of the system include files.  */
   struct file_name_list *before_system;
   struct file_name_list *last_before_system;	/* Last in chain */

   /* Directory prefix that should replace `/usr' in the standard
    * include file directories.  */
   char               *include_prefix;

   char                inhibit_predefs;
   char                no_standard_includes;
   char                no_standard_cplusplus_includes;

/* dump_only means inhibit output of the preprocessed text
             and instead output the definitions of all user-defined
             macros in a form suitable for use as input to cccp.
   dump_names means pass #define and the macro name through to output.
   dump_definitions means pass the whole definition (plus #define) through
*/

   enum dump_type      dump_macros;

/* Nonzero means pass all #define and #undef directives which we actually
   process through to the output stream.  This feature is used primarily
   to allow cc1 to record the #defines and #undefs for the sake of
   debuggers which understand about preprocessor macros, but it may
   also be useful with -E to figure out how symbols are defined, and
   where they are defined.  */
   int                 debug_output;

   /* Pending -D, -U and -A options, in reverse order. */
   struct cpp_pending *pending;

   /* File name which deps are being written to.
    * This is 0 if deps are being written to stdout.  */
   char               *deps_file;

   /* Target-name to write with the dependency information.  */
   char               *deps_target;

   /* Target file to write all include file */
   const char         *watchfile; /**< If non-NULL, path to a file where all accessed include files are logged. */
};

/** Accesses the traditional mode flag from cpp_options. */
#define CPP_TRADITIONAL(PFILE) (CPP_OPTIONS(PFILE)-> traditional)
/** Accesses the pedantic mode flag from cpp_options. */
#define CPP_PEDANTIC(PFILE) (CPP_OPTIONS (PFILE)->pedantic)
/** Accesses the dependency printing flag from cpp_options. */
#define CPP_PRINT_DEPS(PFILE) (CPP_OPTIONS (PFILE)->print_deps)

/** @var progname
 *  @brief Name under which this program was invoked. Used in error messages.
 */
extern char        *progname;

/* The structure of a node in the hash table.  The hash table
   has entries for all tokens defined by #define commands (type T_MACRO),
   plus some special tokens like __LINE__ (these each have their own
   type, and the appropriate code is run when that type of node is seen.
   It does not contain control words like "#define", which are recognized
   by a separate piece of code. */

/* different flavors of hash nodes --- also used in keyword table */
enum node_type {
   T_DEFINE = 1,		/* the `#define' keyword */ /**< Represents the `#define` directive. */
   T_INCLUDE,			/* the `#include' keyword */ /**< Represents the `#include` directive. */
   T_INCLUDE_NEXT,		/* the `#include_next' keyword */ /**< Represents the `#include_next` directive. */
   T_IMPORT,			/* the `#import' keyword */ /**< Represents the `#import` directive (Objective-C). */
   T_IFDEF,			/* the `#ifdef' keyword */ /**< Represents the `#ifdef` directive. */
   T_IFNDEF,			/* the `#ifndef' keyword */ /**< Represents the `#ifndef` directive. */
   T_IF,			/* the `#if' keyword */ /**< Represents the `#if` directive. */
   T_ELSE,			/* `#else' */ /**< Represents the `#else` directive. */
   T_PRAGMA,			/* `#pragma' */ /**< Represents the `#pragma` directive. */
   T_ELIF,			/* `#elif' */ /**< Represents the `#elif` directive. */
   T_UNDEF,			/* `#undef' */ /**< Represents the `#undef` directive. */
   T_LINE,			/* `#line' */ /**< Represents the `#line` directive. */
   T_ERROR,			/* `#error' */ /**< Represents the `#error` directive. */
   T_WARNING,			/* `#warning' */ /**< Represents the `#warning` directive. */
   T_ENDIF,			/* `#endif' */ /**< Represents the `#endif` directive. */
   T_SCCS,			/* `#sccs', used on system V.  */ /**< Represents the `#sccs` directive. */
   T_IDENT,			/* `#ident', used on system V.  */ /**< Represents the `#ident` directive. */
   T_ASSERT,			/* `#assert', taken from system V.  */ /**< Represents the `#assert` directive. */
   T_UNASSERT,			/* `#unassert', taken from system V.  */ /**< Represents the `#unassert` directive. */
   T_SPECLINE,			/* special symbol `__LINE__' */ /**< Internal type for the `__LINE__` predefined macro. */
   T_DATE,			/* `__DATE__' */ /**< Internal type for the `__DATE__` predefined macro. */
   T_FILE,			/* `__FILE__' */ /**< Internal type for the `__FILE__` predefined macro. */
   T_BASE_FILE,			/* `__BASE_FILE__' */ /**< Internal type for the `__BASE_FILE__` predefined macro. */
   T_INCLUDE_LEVEL,		/* `__INCLUDE_LEVEL__' */ /**< Internal type for the `__INCLUDE_LEVEL__` predefined macro. */
   T_VERSION,			/* `__VERSION__' */ /**< Internal type for the `__VERSION__` predefined macro. */
   T_SIZE_TYPE,			/* `__SIZE_TYPE__' */ /**< Internal type for the `__SIZE_TYPE__` predefined macro. */
   T_PTRDIFF_TYPE,		/* `__PTRDIFF_TYPE__' */ /**< Internal type for the `__PTRDIFF_TYPE__` predefined macro. */
   T_WCHAR_TYPE,		/* `__WCHAR_TYPE__' */ /**< Internal type for the `__WCHAR_TYPE__` predefined macro. */
   T_USER_LABEL_PREFIX_TYPE,	/* `__USER_LABEL_PREFIX__' */ /**< Internal type for the `__USER_LABEL_PREFIX__` predefined macro. */
   T_REGISTER_PREFIX_TYPE,	/* `__REGISTER_PREFIX__' */ /**< Internal type for the `__REGISTER_PREFIX__` predefined macro. */
   T_TIME,			/* `__TIME__' */ /**< Internal type for the `__TIME__` predefined macro. */
   T_CONST,			/* Constant value, used by `__STDC__' */ /**< Represents a predefined constant macro (e.g., `__STDC__`). */
   T_MACRO,			/* macro defined by `#define' */ /**< Represents a user-defined macro. */
   T_DISABLED,			/* macro temporarily turned off for rescan */ /**< A macro that is temporarily disabled during its own expansion to prevent recursion. */
   T_SPEC_DEFINED,		/* special `defined' macro for use in #if statements */ /**< Internal type for the `defined` operator. */
   T_PCSTRING,			/* precompiled string (hashval is KEYDEF *) */ /**< Represents a precompiled string, potentially from a precompiled header. */
   T_UNUSED			/* Used for something not defined.  */ /**< Placeholder for unused or undefined node types. */
};

/** @struct definition
 *  @brief Structure representing a macro definition.
 *
 *  For a simple replacement such as `#define foo bar`, `nargs` is -1,
 *  the `pattern` list is null, and the `expansion` is just the replacement text.
 *  `nargs = 0` means a function-like macro with no args, e.g., `#define getchar() getc(stdin)`.
 *  When there are args, the `expansion` is the replacement text with the
 *  args squashed out, and the `reflist` (`pattern`) describes how to
 *  build the output from the input.
 */
/* Structure allocated for every #define.  For a simple replacement
   such as
   	#define foo bar ,
   nargs = -1, the `pattern' list is null, and the expansion is just
   the replacement text.  Nargs = 0 means a functionlike macro with no args,
   e.g.,
       #define getchar() getc (stdin) .
   When there are args, the expansion is the replacement text with the
   args squashed out, and the reflist is a list describing how to
   build the output from the input: e.g., "3 chars, then the 1st arg,
   then 9 chars, then the 3rd arg, then 0 chars, then the 2nd arg".
   The chars here come from the expansion.  Whatever is left of the
   expansion after the last arg-occurrence is copied after that arg.
   Note that the reflist can be arbitrarily long---
   its length depends on the number of times the arguments appear in
   the replacement text, not how many args there are.  Example:
   #define f(x) x+x+x+x+x+x+x would have replacement text "++++++" and
   pattern list
     { (0, 1), (1, 1), (1, 1), ..., (1, 1), NULL }
   where (x, y) means (nchars, argno). */

/** @struct reflist
 *  @brief Node in a list describing how to substitute arguments into a macro expansion.
 *
 *  Each node specifies a segment of literal text from the macro definition,
 *  followed by an argument substitution, or just a segment of literal text if it's the last part.
 */
typedef struct reflist reflist;
struct reflist {
   reflist            *next;        /**< Next item in the pattern list. */
   char                stringify;  /**< Nonzero if this arg was preceded by a # operator (stringification). */
   char                raw_before; /**< Nonzero if a ## operator (token pasting) appeared before this argument. */
   char                raw_after;  /**< Nonzero if a ## operator (token pasting) appeared after this argument. */
   char                rest_args;  /**< Nonzero if this argument absorbs the rest of the actual arguments (variadic). */
   int                 nchars;     /**< Number of literal characters from the definition to copy before this argument occurrence. */
   int                 argno;      /**< Index of the argument to substitute (0-based). */
};

/** @typedef DEFINITION
 *  @brief Typedef for the macro definition structure.
 */
typedef struct definition DEFINITION;
/** @struct definition
 *  @brief Structure representing a macro definition.
 */
struct definition {
   int                 nargs;        /**< Number of arguments. -1 for object-like, 0 for func-like with no args. */
   int                 length;       /**< Length of the `expansion` string. */
   int                 predefined;   /**< True if the macro was builtin or from the command line. */
   unsigned char      *expansion;    /**< The macro expansion text, with argument placeholders removed. */
   int                 line;         /**< Line number where the macro was defined. */
   const char         *file;        /**< File where the macro was defined. */
   char                rest_args;    /**< Nonzero if the last argument is variadic (absorbs remaining actual arguments). */
   reflist            *pattern;     /**< List describing how to substitute arguments. See struct reflist. */
   union {
      /** Names of macro arguments, concatenated in reverse order
       * with comma-space between them.
       * The only use of this is that we warn on redefinition
       * if this differs between the old and new definitions.  */
      unsigned char      *argnames;  /**< Concatenated string of argument names, for redefinition checks. */
   } args;
};

/** @var is_idchar
 *  @brief Lookup table: is_idchar[c] is true if character c can be part of an identifier (but not necessarily start one).
 */
extern unsigned char is_idchar[256];

/** @struct if_stack
 *  @brief Structure for managing the stack of conditional compilation blocks (#if, #ifdef, etc.).
 *
 *  Each frame on this stack represents an active conditional block.
 */
struct if_stack {
   struct if_stack    *next;	/**< Pointer to the next (enclosing) conditional stack frame. */
   const char         *fname;	/**< Filename where the conditional directive was encountered. */
   int                 lineno;	/**< Line number of the conditional directive. */
   int                 if_succeeded;	/**< True if a branch of this if-group (e.g. #if, #elif) has already been processed. */
   unsigned char      *control_macro;	/**< For `#ifndef` at the start of a file, this is the macro name tested (for include guards). */
   enum node_type      type;	/**< Type of the last directive seen in this group (e.g., T_IF, T_ELSE). */
};
/** @typedef IF_STACK_FRAME
 *  @brief Typedef for the conditional compilation stack frame structure.
 */
typedef struct if_stack IF_STACK_FRAME;

/**
 * @brief Get the current line and column number from a buffer.
 * @param pbuf The buffer.
 * @param linep Pointer to store the line number.
 * @param colp Pointer to store the column number.
 */
extern void         cpp_buf_line_and_col(cpp_buffer *, long *, long *);

/**
 * @brief Find the cpp_buffer that corresponds to a file (not a macro expansion).
 * @param pfile The preprocessor state.
 * @return The file buffer, or NULL if not in any file (e.g., only macro expansions on stack).
 */
extern cpp_buffer  *cpp_file_buffer(cpp_reader *);

/**
 * @brief Defines a macro programmatically.
 * @param pfile The preprocessor state.
 * @param str The definition string, e.g., "MACRO=VALUE" or "MACRO".
 *            If only "MACRO", it's defined as 1.
 */
extern void         cpp_define(cpp_reader *, unsigned char *);

/**
 * @brief Reports an error.
 * @param pfile The preprocessor state.
 * @param msg The error message format string.
 * @param ... Arguments for the format string.
 */
extern void         cpp_error(cpp_reader * pfile, const char *msg, ...);

/**
 * @brief Reports a warning.
 * @param pfile The preprocessor state.
 * @param msg The warning message format string.
 * @param ... Arguments for the format string.
 */
extern void         cpp_warning(cpp_reader * pfile, const char *msg, ...);

/**
 * @brief Reports a pedantic warning (or error if -pedantic-errors).
 * @param pfile The preprocessor state.
 * @param msg The message format string.
 * @param ... Arguments for the format string.
 */
extern void         cpp_pedwarn(cpp_reader * pfile, const char *msg, ...);

/**
 * @brief Reports a fatal error and exits.
 * @param msg The error message format string.
 * @param ... Arguments for the format string.
 */
extern void         cpp_fatal(const char *msg, ...);

/**
 * @brief Formats a file:line:column string for an error/warning message.
 * @param pfile The preprocessor state.
 * @param filename The name of the file.
 * @param line The line number.
 * @param column The column number (-1 if not applicable).
 */
extern void         cpp_file_line_for_message(cpp_reader * pfile,
					      const char *filename, int line,
					      int column);
/**
 * @brief Reports an error related to a file operation, including errno.
 * @param pfile The preprocessor state.
 * @param name The name of the file or operation that failed.
 */
extern void         cpp_perror_with_name(cpp_reader * pfile, const char *name);

/**
 * @brief Reports a fatal error related to a file operation, including errno, and exits.
 * @param pfile The preprocessor state.
 * @param name The name of the file or operation that failed.
 */
extern void         cpp_pfatal_with_name(cpp_reader * pfile, const char *name);

/**
 * @brief Generic message reporting function.
 * @param pfile The preprocessor state.
 * @param is_error True if it's an error, false for a warning.
 * @param msg The message format string.
 * @param ... Arguments for the format string.
 */
extern void         cpp_message(cpp_reader * pfile, int is_error,
				const char *msg, ...);
/**
 * @brief Generic message reporting function (va_list version).
 * @param pfile The preprocessor state.
 * @param is_error True if it's an error, false for a warning.
 * @param msg The message format string.
 * @param args va_list of arguments for the format string.
 */
extern void         cpp_message_v(cpp_reader * pfile, int is_error,
				  const char *msg, va_list args);

/**
 * @brief Ensures the token buffer has enough capacity for more data.
 *
 * If the current token buffer cannot hold at least `n` more characters,
 * it is reallocated to a larger size. The new size is typically double the
 * old size plus the required extra space, to amortize the cost of reallocation.
 *
 * @param pfile The preprocessor state.
 * @param n The minimum number of additional characters needed in the buffer.
 */
extern void         cpp_grow_buffer(cpp_reader * pfile, long n);

/**
 * @brief Parses an escape sequence within a string or character literal.
 * @param pfile The preprocessor state.
 * @param string_ptr Pointer to a pointer to the current character in the string.
 *                   This will be advanced past the escape sequence.
 * @return The value of the escape sequence, or -1 on error.
 */
extern int          cpp_parse_escape(cpp_reader * pfile, char **string_ptr);

/**
 * @brief Prints the chain of including files for context in error messages.
 * @param pfile The preprocessor state.
 */
void                cpp_print_containing_files(cpp_reader * pfile);

/**
 * @brief Parses and evaluates a preprocessor constant expression (e.g., in #if).
 * @param pfile The preprocessor state.
 * @return The result of the expression.
 */
HOST_WIDE_INT       cpp_parse_expr(cpp_reader * pfile);

/**
 * @brief Skips the rest of the current line in the input buffer.
 * @param pfile The preprocessor state.
 */
void                skip_rest_of_line(cpp_reader * pfile);

/**
 * @brief Initializes the cpp_reader structure for parsing a new file.
 * @param pfile The preprocessor state to initialize.
 */
void                init_parse_file(cpp_reader * pfile);

/**
 * @brief Initializes the cpp_options structure to default values.
 * @param opts The options structure to initialize.
 */
void                init_parse_options(struct cpp_options *opts);

/**
 * @brief Pushes a new file onto the input stack for processing.
 * @param pfile The preprocessor state.
 * @param fname The name of the file to push. If NULL or empty, reads from stdin.
 * @return SUCCESS_EXIT_CODE or FATAL_EXIT_CODE.
 */
int                 push_parse_file(cpp_reader * pfile, const char *fname);

/**
 * @brief Finalizes preprocessing, e.g., writing out dependency files.
 * @param pfile The preprocessor state.
 */
void                cpp_finish(cpp_reader * pfile);

/**
 * @brief Reads and checks an assertion in a #if #assertion construct.
 * @param pfile The preprocessor state.
 * @return 1 if the assertion holds, 0 otherwise.
 */
int                 cpp_read_check_assertion(cpp_reader * pfile);

/**
 * @brief Allocates memory, exiting on failure.
 * @param size The number of bytes to allocate.
 * @return Pointer to the allocated memory.
 */
void               *xmalloc(unsigned size);

/**
 * @brief Reallocates memory, exiting on failure.
 * @param old Pointer to the previously allocated memory.
 * @param size The new size in bytes.
 * @return Pointer to the reallocated memory.
 */
void               *xrealloc(void *old, unsigned size);

/**
 * @brief Allocates and zero-initializes memory, exiting on failure.
 * @param number The number of elements to allocate.
 * @param size The size of each element in bytes.
 * @return Pointer to the allocated and zeroed memory.
 */
void               *xcalloc(unsigned number, unsigned size);

/**
 * @brief Logs the usage of a file if a watchfile is specified.
 * @param filename The name of the file being used.
 * @param type A character indicating the type of usage (e.g., 'E' for #include).
 */
void                using_file(const char *filename, const char type);

#ifdef __EMX__
#define PATH_SEPARATOR ';'
#endif
