#ifndef ELM_PREFS_CC_H
#define ELM_PREFS_CC_H

/**
 * @file
 * @brief Header file for the Elementary Prefs Compiler (elm_prefs_cc).
 *
 * This file defines the structures, global variables, and function
 * prototypes used by the elm_prefs_cc utility, which compiles
 * human-readable preference definition files (.epc) into a binary
 * format (.epb) for use by Elementary applications.
 */

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_prefs.h"

/**
 * @brief Eina_Prefix structure for handling application install paths.
 * @see pfx in elm_prefs_cc.c
 */
extern Eina_Prefix *pfx;

/*
 * On Windows, if the file is not opened in binary mode,
 * read does not return the correct size, because of
 * CR / LF translation.
 */
#ifndef O_BINARY
# define O_BINARY 0
#endif

/* logging variables */
/**
 * @brief Log domain for the elm_prefs_cc utility.
 * @see _elm_prefs_cc_log_dom in elm_prefs_cc.c
 */
extern int _elm_prefs_cc_log_dom;

/** Default log color for elm_prefs_cc messages. */
#define ELM_PREFS_CC_DEFAULT_LOG_COLOR EINA_COLOR_CYAN

#ifdef ERR
# undef ERR
#endif
/** @brief Log an error message using the elm_prefs_cc domain. */
#define ERR(...) EINA_LOG_DOM_ERR(_elm_prefs_cc_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
/** @brief Log an informational message using the elm_prefs_cc domain. */
#define INF(...) EINA_LOG_DOM_INFO(_elm_prefs_cc_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
/** @brief Log a warning message using the elm_prefs_cc domain. */
#define WRN(...) EINA_LOG_DOM_WARN(_elm_prefs_cc_log_dom, __VA_ARGS__)

#ifdef CRI
# undef CRI
#endif
/** @brief Log a critical message using the elm_prefs_cc domain. */
#define CRI(...) EINA_LOG_DOM_CRIT(_elm_prefs_cc_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
/** @brief Log a debug message using the elm_prefs_cc domain. */
#define DBG(...) EINA_LOG_DOM_DBG(_elm_prefs_cc_log_dom, __VA_ARGS__)

/* types */
/** @brief Represents a parsed Elementary Prefs file. */
typedef struct _Elm_Prefs_File        Elm_Prefs_File;
/** @brief Handler for creating new objects during parsing. */
typedef struct _New_Object_Handler    New_Object_Handler;
/** @brief Handler for processing statements during parsing. */
typedef struct _New_Statement_Handler New_Statement_Handler;

/**
 * @struct _Elm_Prefs_File
 * @brief Structure representing the entire content of a compiled preferences file.
 *
 * This structure holds metadata about the compilation process and a list
 * of pages defined in the preferences file.
 */
struct _Elm_Prefs_File
{
   const char        *compiler; /**< Name of the compiler used (e.g., "elm_prefs_cc"). */
   Eina_List         *pages;    /**< List of Elm_Prefs_Page structures. */
};

/**
 * @struct _New_Object_Handler
 * @brief Defines a handler for a specific object type encountered during parsing.
 *
 * When the parser encounters an object of a given 'type', the associated 'func'
 * is called to process it.
 */
struct _New_Object_Handler
{
   const char *type;      /**< The type string of the object (e.g., "PAGE", "GROUP"). */
   void (*func)(void);    /**< Function pointer to the handler for this object type. */
};

/**
 * @struct _New_Statement_Handler
 * @brief Defines a handler for a specific statement type encountered during parsing.
 *
 * When the parser encounters a statement of a given 'type', the associated 'func'
 * is called to process it.
 */
struct _New_Statement_Handler
{
   const char *type;      /**< The type string of the statement (e.g., "TITLE", "VALUE"). */
   void (*func)(void);    /**< Function pointer to the handler for this statement type. */
};

/* global fn calls */

/** @brief Main compilation function that orchestrates the parsing and code generation. */
void    compile(void);

/**
 * @brief Parses a string argument from the input.
 * @param n Argument index (1-based).
 * @return A newly allocated string containing the parsed value. The caller is responsible for freeing it.
 */
char   *parse_str(int n);

/**
 * @brief Parses an enumeration value from the input.
 * @param n Argument index (1-based).
 * @param ... A NULL-terminated list of C-string pairs (const char *enum_str, int enum_val).
 *            Example: parse_enum(1, "VALUE_ONE", 1, "VALUE_TWO", 2, NULL);
 * @return The integer value corresponding to the parsed enumeration string.
 */
int     parse_enum(int n, ...);

/**
 * @brief Parses an integer argument from the input.
 * @param n Argument index (1-based).
 * @return The parsed integer value.
 */
int     parse_int(int n);

/**
 * @brief Parses an integer argument from the input, ensuring it's within a specified range.
 * @param n Argument index (1-based).
 * @param f Minimum allowed value (inclusive).
 * @param t Maximum allowed value (inclusive).
 * @return The parsed integer value if within range.
 */
int     parse_int_range(int n, int f, int t);

/**
 * @brief Parses a boolean argument from the input.
 *        Accepts "true", "false", "1", "0", "on", "off", "yes", "no".
 * @param n Argument index (1-based).
 * @return 1 for true, 0 for false.
 */
int     parse_bool(int n);

/**
 * @brief Parses a floating-point argument from the input.
 * @param n Argument index (1-based).
 * @return The parsed double value.
 */
double  parse_float(int n);

/**
 * @brief Checks if the correct number of arguments were provided for the current statement.
 * @param n Expected number of arguments.
 */
void    check_arg_count(int n);

/**
 * @brief Validates a string against a regular expression.
 * @param regex The POSIX extended regular expression to match against.
 */
void    check_regex(const char *regex);

/**
 * @brief Sets a verbatim block of text, typically for descriptions or help.
 * @param s The string containing the verbatim text.
 * @param l1 Starting line number of the verbatim block in the input file.
 * @param l2 Ending line number of the verbatim block in the input file.
 */
void    set_verbatim(char *s, int l1, int l2);

/** @brief Initializes data structures for writing the compiled output. */
void    data_init(void);
/** @brief Writes the compiled preference data to the output file. */
void    data_write(void);
/** @brief Cleans up data structures after writing the output. */
void    data_shutdown(void);

/** @return The number of registered object handlers. */
int     object_handler_num(void);
/** @return The number of registered statement handlers. */
int     statement_handler_num(void);

/**
 * @brief Allocates memory and exits on failure.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory.
 */
void   *mem_alloc(size_t size);

/**
 * @brief Duplicates a string using mem_alloc and exits on failure.
 * @param s The string to duplicate.
 * @return A pointer to the newly allocated duplicated string.
 */
char   *mem_strdup(const char *s);

/** @brief Macro to get the size of a type or variable. */
#define SZ sizeof

/* global vars */
/** @brief Path to the input preferences definition file (.epc). */
extern char                  *file_in;
/** @brief Path to the system's temporary directory. */
extern const char            *tmp_dir;
/** @brief Path to the output compiled preferences file (.epb). */
extern char                  *file_out;
/** @brief Current line number being parsed in the input file. */
extern int                    line;
/** @brief Stack used during parsing, likely for nested structures. Eina_List of parser states or objects. */
extern Eina_List             *stack;
/** @brief List of parameters for the current statement being parsed. Eina_List of char*. */
extern Eina_List             *params;

/** @brief Global pointer to the main Elm_Prefs_File structure being built. */
extern Elm_Prefs_File        *elm_prefs_file;

/**
 * @brief List of top-level pages defined in the preferences file.
 * This is an Eina_List containing pointers to Elm_Prefs_Page structures.
 */
extern Eina_List             *elm_prefs_pages;

/**
 * @brief Array of object handlers.
 * Each element is a New_Object_Handler struct:
 * @code
 * static New_Object_Handler object_handlers[] = {
 *    { "PAGE",      page_new },       // Handles "PAGE" type objects with page_new()
 *    { "GROUP",     group_new },      // Handles "GROUP" type objects with group_new()
 *    // ... more handlers
 *    { NULL, NULL }                   // Terminator
 * };
 * @endcode
 */
extern New_Object_Handler     object_handlers[];

/**
 * @brief Array of statement handlers.
 * Each element is a New_Statement_Handler struct:
 * @code
 * static New_Statement_Handler statement_handlers[] = {
 *    { "TITLE",     title_new },      // Handles "TITLE" type statements with title_new()
 *    { "VALUE",     value_new },      // Handles "VALUE" type statements with value_new()
 *    // ... more handlers
 *    { NULL, NULL }                   // Terminator
 * };
 * @endcode
 */
extern New_Statement_Handler  statement_handlers[];

#endif
