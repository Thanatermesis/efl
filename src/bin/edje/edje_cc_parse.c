#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#include "edje_cc.h"
#include <Ecore.h>
#include <Ecore_File.h>

#ifdef _WIN32
# define EPP_EXT ".exe"
#else
# define EPP_EXT
#endif

#define SKIP_NAMESPACE_VALIDATION_SUPPORTED " -DSKIP_NAMESPACE_VALIDATION=1 "

#define EDJE_1_18_SUPPORTED " -DEFL_VERSION_1_18=1 "
#define EDJE_1_19_SUPPORTED " -DEFL_VERSION_1_19=1 "
#define EDJE_1_20_SUPPORTED " -DEFL_VERSION_1_20=1 "
#define EDJE_1_21_SUPPORTED " -DEFL_VERSION_1_21=1 "
#define EDJE_1_22_SUPPORTED " -DEFL_VERSION_1_22=1 "
#define EDJE_1_23_SUPPORTED " -DEFL_VERSION_1_23=1 "
#define EDJE_1_24_SUPPORTED " -DEFL_VERSION_1_24=1 "
#define EDJE_1_25_SUPPORTED " -DEFL_VERSION_1_25=1 "
#define EDJE_1_26_SUPPORTED " -DEFL_VERSION_1_26=1 "

#define EDJE_CC_EFL_VERSION_SUPPORTED \
  EDJE_1_18_SUPPORTED                 \
  EDJE_1_19_SUPPORTED                 \
  EDJE_1_20_SUPPORTED                 \
  EDJE_1_21_SUPPORTED                 \
  EDJE_1_22_SUPPORTED                 \
  EDJE_1_23_SUPPORTED                 \
  EDJE_1_24_SUPPORTED                 \
  EDJE_1_25_SUPPORTED                 \
  EDJE_1_26_SUPPORTED

/**
 * @brief Handles the creation or identification of a new object based on the current parsing stack.
 *
 * This function is called when a new token is identified as a potential object
 * or statement. It checks the current stack identifier against registered object
 * handlers. If a match is found, the corresponding handler function is executed.
 * If no object handler matches, it attempts to find a statement handler.
 * It also handles wildcard matching for statement handlers.
 */
static void        new_object(void);

/**
 * @brief Handles the execution of a statement based on the current parsing stack.
 *
 * This function is called when a semicolon is encountered or a single-line statement
 * is identified. It checks the current stack identifier against registered statement
 * handlers. If a match is found, the corresponding handler function is executed.
 * It also handles wildcard matching for statement handlers.
 */
static void        new_statement(void);

/**
 * @brief Performs mathematical calculations on an input string.
 * @param input The string containing a mathematical expression.
 *              Example: "(10 + 5) * 2"
 * @return A new string representing the result of the calculation.
 *         The caller is responsible for freeing this string.
 *         Example: "30.000000"
 * @note Currently, it always uses floating-point arithmetic.
 */
static char       *perform_math(char *input);

/**
 * @brief Checks if a character is a delimiter.
 * @param c The character to check.
 * @return 1 if the character is a delimiter ({},;:[]), 0 otherwise.
 */
static int         isdelim(char c);

/**
 * @brief Extracts the next token from the input buffer.
 *
 * This function parses the input buffer, skipping comments and whitespace,
 * and identifies the next token. It handles quoted strings, parentheses,
 * and various comment styles (#, //, /* ... *\/).
 *
 * @param p Pointer to the current position in the input buffer.
 * @param end Pointer to the end of the input buffer.
 * @param new_p Output parameter, will point to the position after the extracted token.
 * @param delim Output parameter, will be set to 1 if the token is a delimiter, 0 otherwise.
 * @return A new string containing the token, or NULL if no more tokens are found.
 *         The caller is responsible for freeing this string.
 *         The `had_quote` global variable is set if the token was quoted.
 */
static char       *next_token(char *p, char *end, char **new_p, int *delim);

/**
 * @brief Gets the current parsing stack identifier as a string.
 * @return A string representing the current stack, e.g., "group.parts.part".
 *         This string is owned by an internal Eina_Strbuf and should not be freed
 *         or modified by the caller.
 */
static const char *stack_id(void);

/**
 * @brief Parses the entire input data buffer.
 * @param data The input data buffer to parse.
 * @param size The size of the input data buffer.
 */
static void        parse(char *data, off_t size);

/* simple expression parsing protos */
/**
 * @brief Converts a string to an integer, performing expression evaluation.
 * @param s The string to convert. Can be a simple number or an expression.
 *          Example: "10", "(5+3)*2"
 * @return The integer result of the conversion/evaluation.
 */
static int         my_atoi(const char *s);
/** @brief Parses addition and subtraction operations for integer expressions. Part of recursive descent parser. */
static char       *_alphai(char *s, int *val);
/** @brief Parses multiplication, division, and modulo operations for integer expressions. Part of recursive descent parser. */
static char       *_betai(char *s, int *val);
/** @brief Parses numbers, parenthesized expressions, or functions for integer expressions. Part of recursive descent parser. */
static char       *_gammai(char *s, int *val);
/** @brief Parses parenthesized expressions for integer expressions. Part of recursive descent parser. */
static char       *_deltai(char *s, int *val);
/** @brief Extracts an integer number from a string. Part of recursive descent parser. */
static char       *_get_numi(char *s, int *val);
/** @brief Checks if a character can be part of an integer number. */
static int         _is_numi(char c);
/** @brief Checks if a character is a high precedence integer operator (*, /, %). */
static int         _is_op1i(char c);
/** @brief Checks if a character is a low precedence integer operator (+, -). */
static int         _is_op2i(char c);
/** @brief Performs an integer calculation for a given operator and two operands. */
static int         _calci(char op, int a, int b);

/**
 * @brief Converts a string to a double, performing expression evaluation.
 * @param s The string to convert. Can be a simple number or an expression.
 *          Example: "10.5", "(5.1+3.2)*2.0"
 * @return The double result of the conversion/evaluation.
 */
static double      my_atof(const char *s);
/** @brief Parses addition and subtraction operations for floating-point expressions. Part of recursive descent parser. */
static char       *_alphaf(char *s, double *val);
/** @brief Parses multiplication, division, and modulo operations for floating-point expressions. Part of recursive descent parser. */
static char       *_betaf(char *s, double *val);
/** @brief Parses numbers, parenthesized expressions, or functions for floating-point expressions. Part of recursive descent parser. */
static char       *_gammaf(char *s, double *val);
/** @brief Parses parenthesized expressions for floating-point expressions. Part of recursive descent parser. */
static char       *_deltaf(char *s, double *val);
/** @brief Extracts a floating-point number from a string. Part of recursive descent parser. */
static char       *_get_numf(char *s, double *val);
/** @brief Checks if a character can be part of a floating-point number. */
static int         _is_numf(char c);
/** @brief Checks if a character is a high precedence floating-point operator (*, /, %). */
static int         _is_op1f(char c);
/** @brief Checks if a character is a low precedence floating-point operator (+, -). */
static int         _is_op2f(char c);
/** @brief Performs a floating-point calculation for a given operator and two operands. */
static double      _calcf(char op, double a, double b);
/**
 * @brief Strips whitespace (spaces and tabs) from a string.
 * @param in The input string.
 * @param out The output buffer to store the stripped string.
 * @param size The size of the output buffer.
 * @return 1 on success, 0 if the input string is too long for the output buffer.
 */
static int         strstrip(const char *in, char *out, size_t size);

int line = 0; ///< Current line number being parsed.
Eina_List *stack = NULL; ///< The parsing stack, holds tokens representing hierarchy.
Eina_Array params; ///< Array to store parameters for a statement. Elements are `char *`.
int had_quote = 0; ///< Flag indicating if the last token processed was quoted.
int params_quote = 0; ///< Bitmask indicating which parameters in `params` array were quoted.

static char file_buf[4096]; ///< Buffer to store the current filename being parsed (after CPP).
static int did_wildcard = 0; ///< Flag indicating if a wildcard handler was matched.
static int verbatim = 0; ///< Flag indicating if currently parsing a verbatim block (e.g., script).
static int verbatim_line1 = 0; ///< Starting line number of a verbatim block.
static int verbatim_line2 = 0; ///< Ending line number of a verbatim block.
static char *verbatim_str = NULL; ///< String content of the verbatim block.
static Eina_Strbuf *stack_buf = NULL; ///< String buffer for efficient construction of stack_id.

/**
 * @brief Prints the current parsing stack to standard error.
 * Used for debugging and error reporting.
 */
static void
err_show_stack(void)
{
   const char *s;

   s = stack_id();
   if (s)
     ERR("PARSE STACK:\n%s", s);
   else
     ERR("NO PARSE STACK");
}

/**
 * @brief Prints the current parameters to standard error.
 * Used for debugging and error reporting.
 * The `params` array contains `char *` elements.
 */
static void
err_show_params(void)
{
   Eina_Array_Iterator iterator;
   unsigned int i;
   char *p;

   ERR("PARAMS:");
   EINA_ARRAY_ITER_NEXT(&params, i, p, iterator)
   {
      ERR("  %s", p);
   }
}

/**
 * @brief Shows both the parsing stack and current parameters.
 * Convenience function for error reporting.
 */
static void
err_show(void)
{
   err_show_stack();
   err_show_params();
}

/**
 * @brief Retrieves a parameter by its index from the `params` array.
 * @param n The index of the parameter to retrieve.
 * @return The parameter string (char *) if it exists, NULL otherwise.
 *         The returned string is owned by the `params` array and should not be freed.
 */
static char *
_parse_param_get(int n)
{
   if (n < (int)eina_array_count(&params))
     return eina_array_data_get(&params, n);
   return NULL;
}

static Eina_Hash *_new_object_hash = NULL; ///< Hash table for `New_Object_Handler` lookup by type.
static Eina_Hash *_new_object_short_hash = NULL; ///< Hash table for short `New_Object_Handler` lookup.
static Eina_Hash *_new_statement_hash = NULL; ///< Hash table for `New_Statement_Handler` lookup by type.
static Eina_Hash *_new_statement_short_hash = NULL; ///< Hash table for short `New_Statement_Handler` lookup.
static Eina_Hash *_new_statement_short_single_hash = NULL; ///< Hash table for single-line short `New_Statement_Handler` lookup.
static Eina_Hash *_new_nested_hash = NULL; ///< Hash table for `New_Nested_Handler` lookup by type.
static Eina_Hash *_new_nested_short_hash = NULL; ///< Hash table for short `New_Nested_Handler` lookup.

/**
 * @brief Initializes hash tables for efficient lookup of object and statement handlers.
 *
 * This function populates various hash tables (`_new_object_hash`, `_new_statement_hash`, etc.)
 * with handlers defined in `object_handlers`, `statement_handlers`, and `nested_handlers` arrays.
 * It's called lazily when needed.
 */
static void
fill_object_statement_hashes(void)
{
   int i, n;

   if (_new_object_hash) return;

   _new_object_hash = eina_hash_string_superfast_new(NULL);
   _new_object_short_hash = eina_hash_string_superfast_new(NULL);
   _new_statement_hash = eina_hash_string_superfast_new(NULL);
   _new_statement_short_hash = eina_hash_string_superfast_new(NULL);
   _new_statement_short_single_hash = eina_hash_string_superfast_new(NULL);
   _new_nested_hash = eina_hash_string_superfast_new(NULL);
   _new_nested_short_hash = eina_hash_string_superfast_new(NULL);

   n = object_handler_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_object_hash, object_handlers[i].type,
                             &(object_handlers[i]));
     }
   n = object_handler_short_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_object_short_hash, object_handlers_short[i].type,
                             &(object_handlers_short[i]));
     }
   n = statement_handler_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_statement_hash, statement_handlers[i].type,
                             &(statement_handlers[i]));
     }
   n = statement_handler_short_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_statement_short_hash, statement_handlers_short[i].type,
                             &(statement_handlers_short[i]));
     }
   n = statement_handler_short_single_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_statement_short_single_hash, statement_handlers_short_single[i].type,
                             &(statement_handlers_short_single[i]));
     }
   n = nested_handler_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_nested_hash, nested_handlers[i].type,
                             &(nested_handlers[i]));
     }
   n = nested_handler_short_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_nested_short_hash, nested_handlers_short[i].type,
                             &(nested_handlers_short[i]));
     }
}

/**
 * @brief Creates a wildcard version of the current stack identifier.
 *
 * Replaces the last component of the stack ID with a wildcard '*'.
 * For example, "group.parts.part" becomes "group.parts.*".
 *
 * @return A new string with the wildcarded stack ID.
 *         The caller is responsible for freeing this string.
 */
static char *
stack_dup_wildcard(void)
{
   char buf[PATH_MAX] = { 0, };
   char *end;

   strncpy(buf, stack_id(), sizeof(buf) - 1);

   end = strrchr(buf, '.');
   if (end) end++;
   else end = buf;

   strcpy(end, "*");

   return eina_strdup(buf);
}

static void
new_object(void)
{
   const char *id;
   New_Object_Handler *oh = NULL;
   New_Statement_Handler *sh;

   fill_object_statement_hashes();
   id = stack_id();
   if (!had_quote)
     {
        oh = eina_hash_find(_new_object_hash, id);
        if (!oh)
          oh = eina_hash_find(_new_object_short_hash, id);
     }
   if (oh)
     {
        if (oh->func) oh->func();
     }
   else
     {
        did_wildcard = edje_cc_handlers_wildcard();
        if (!did_wildcard)
          {
             sh = eina_hash_find(_new_statement_hash, id);
             if (!sh)
               sh = eina_hash_find(_new_statement_short_hash, id);
             if (!sh)
               sh = eina_hash_find(_new_statement_short_single_hash, id);
             if (!sh)
               {
                  char *tmp = stack_dup_wildcard();
                  sh = eina_hash_find(_new_statement_hash, tmp);
                  free(tmp);
               }
             if ((!sh) && (!did_wildcard) && (!had_quote))
               {
                  ERR("%s:%i unhandled keyword %s",
                      file_in, line - 1,
                      (char *)eina_list_data_get(eina_list_last(stack)));
                  err_show();
                  exit(-1);
               }
             did_wildcard = !sh;
          }
     }
}

static void
new_statement(void)
{
   const char *id;
   New_Statement_Handler *sh = NULL;
   fill_object_statement_hashes();
   id = stack_id();
   sh = eina_hash_find(_new_statement_hash, id);
   if (!sh)
     sh = eina_hash_find(_new_statement_short_hash, id);
   if (sh)
     {
        if (sh->func) sh->func();
     }
   else
     {
        char *tmp = stack_dup_wildcard();
        sh = eina_hash_find(_new_statement_hash, tmp);
        free(tmp);

        if (sh)
          {
             if (sh->func) sh->func();
          }
        else
          {
             ERR("%s:%i unhandled keyword %s",
                 file_in, line - 1,
                 (char *)eina_list_data_get(eina_list_last(stack)));
             err_show();
             exit(-1);
          }
     }
}

/**
 * @brief Handles a single-line statement.
 *
 * This function is called when a statement is expected to be a single line
 * (not terminated by a semicolon or block). It looks up the statement handler
 * in `_new_statement_short_single_hash`.
 *
 * @return EINA_TRUE if a handler was found and executed, EINA_FALSE otherwise.
 */
static Eina_Bool
new_statement_single(void)
{
   const char *id;
   New_Statement_Handler *sh = NULL;
   fill_object_statement_hashes();
   id = stack_id();
   sh = eina_hash_find(_new_statement_short_single_hash, id);
   if (sh)
     {
        if (sh->func) sh->func();
     }
   return !!sh;
}

/**
 * @brief Performs mathematical calculations on an input string.
 * @param input The string containing a mathematical expression.
 *              Example: "(10 + 5) * 2"
 * @return A new string representing the result of the calculation.
 *         The caller is responsible for freeing this string.
 *         Example: "30.000000"
 * @note Currently, it always uses floating-point arithmetic.
 */
static char *
perform_math(char *input)
{
   char buf[256];
   double res;

   /* FIXME
    * Always apply floating-point arithmetic.
    * Does this cause problems for integer parameters? (yes it will)
    *
    * What we should do is, loop over the string and figure out whether
    * there are floating point operands, too and then switch to
    * floating point math.
    */
   res = my_atof(input);
   snprintf(buf, sizeof (buf), "%lf", res);
   return strdup(buf);
}

/**
 * @brief Checks if a character is a delimiter.
 * @param c The character to check.
 * @return 1 if the character is a delimiter ({},;:[]), 0 otherwise.
 */
static int
isdelim(char c)
{
   const char *delims = "{},;:[]";
   char *d;

   d = (char *)delims;
   while (*d)
     {
        if (c == *d) return 1;
        d++;
     }
   return 0;
}

/**
 * @brief Extracts the next token from the input buffer.
 *
 * This function parses the input buffer, skipping comments and whitespace,
 * and identifies the next token. It handles quoted strings, parentheses,
 * and various comment styles (#, //, /* ... *\/).
 *
 * @param p Pointer to the current position in the input buffer.
 * @param end Pointer to the end of the input buffer.
 * @param new_p Output parameter, will point to the position after the extracted token.
 * @param delim Output parameter, will be set to 1 if the token is a delimiter, 0 otherwise.
 * @return A new string containing the token, or NULL if no more tokens are found.
 *         The caller is responsible for freeing this string.
 *         The `had_quote` global variable is set if the token was quoted.
 */
static char *
next_token(char *p, char *end, char **new_p, int *delim)
{
   char *tok_start = NULL, *tok_end = NULL, *tok = NULL, *sa_start = NULL;
   int in_tok = 0;
   int in_quote = 0;
   int in_parens = 0;
   int in_comment_ss = 0;
   int in_comment_cpp = 0;
   int in_comment_sa = 0;
   int is_escaped = 0;

   had_quote = 0;

   *delim = 0;
   if (p >= end) return NULL;
   while (p < end)
     {
        if (*p == '\n')
          {
             in_comment_ss = 0;
             in_comment_cpp = 0;
             line++;
          }
        if ((!in_comment_ss) && (!in_comment_sa))
          {
             if ((!in_quote) && (*p == '/') && (p < (end - 1)) && (*(p + 1) == '/'))
               in_comment_ss = 1;
             if ((!in_quote) && (*p == '#'))
               in_comment_cpp = 1;
             if ((!in_quote) && (*p == '/') && (p < (end - 1)) && (*(p + 1) == '*'))
               {
                  in_comment_sa = 1;
                  sa_start = p;
               }
          }
        if ((in_comment_cpp) && (*p == '#'))
          {
             char *pp, fl[4096];
             char *tmpstr = NULL;
             int l, nm;

             /* handle cpp comments */
             /* their line format is
              * #line <line no. of next line> <filename from next line on> [??]
              */

             pp = p;
             while ((pp < end) && (*pp != '\n'))
               {
                  pp++;
               }
             l = pp - p;
             tmpstr = alloca(l + 1);
             memcpy(tmpstr, p, l);
             tmpstr[l] = 0;
             if (l >= (int)sizeof(fl))
               {
                  ERR("Line too long: %i chars: %s", l, tmpstr);
                  err_show();
                  exit(-1);
               }
             l = sscanf(tmpstr, "%*s %i \"%[^\"]\"", &nm, fl);
             if (l == 2)
               {
                  strcpy(file_buf, fl);
                  line = nm;
                  file_in = file_buf;
               }
          }
        else if ((!in_comment_ss) && (!in_comment_sa) && (!in_comment_cpp))
          {
             if (!in_tok)
               {
                  if (!in_quote)
                    {
                       if (!isspace(*p))
                         {
                            if (*p == '"')
                              {
                                 in_quote = 1;
                                 had_quote = 1;
                              }
                            else if (*p == '(')
                              in_parens++;

                            in_tok = 1;
                            tok_start = p;
                            if (isdelim(*p)) *delim = 1;
                         }
                    }
               }
             else
               {
                  if (in_quote)
                    {
                       if ((*p) == '\\')
                         is_escaped = !is_escaped;
                       else if (((*p) == '"') && (!is_escaped))
                         {
                            in_quote = 0;
                            had_quote = 1;
                         }
                       else if (is_escaped)
                         is_escaped = 0;
                    }
                  else if (in_parens != 0 && (!is_escaped))
                    {
                       if (*p == '(')
                         in_parens++;
                       else if (*p == ')')
                         in_parens--;
                       else if (isdelim(*p))
                         {
                            ERR("check pair of parens %s:%i.", file_in, line - 1);
                            err_show();
                            exit(-1);
                         }
                    }
                  else
                    {
                       if (*p == '"')
                         {
                            in_quote = 1;
                            had_quote = 1;
                         }
                       else if (*p == '(')
                         in_parens++;
                       else if (*p == ')')
                         in_parens--;

                       /* check for end-of-token */
                       if (
                         (isspace(*p)) ||
                         ((*delim) && (!isdelim(*p))) ||
                         (isdelim(*p))
                         ) /*the line below this is never  used because it skips to
                            * the 'done' label which is after the return call for
                            * in_tok being 0. is this intentional?
                            */
                         {
                            in_tok = 0;

                            tok_end = p - 1;
                            if (*p == '\n') line--;
                            goto done;
                         }
                    }
               }
          }
        if (in_comment_sa)
          {
             if ((*p == '/') && (*(p - 1) == '*') && ((p - sa_start) > 2))
               in_comment_sa = 0;
          }
        p++;
     }
   if (!in_tok) return NULL;
   tok_end = p - 1;

done:
   *new_p = p;

   tok = mem_alloc(tok_end - tok_start + 2);
   if (!tok) return NULL;

   strncpy(tok, tok_start, tok_end - tok_start + 1);
   tok[tok_end - tok_start + 1] = 0;

   if (had_quote)
     {
        is_escaped = 0;
        p = tok;

        /* Note: if you change special chars list here make the same changes in
         * _edje_generate_source_of_style function
         */
        while (*p)
          {
             if ((*p == '\"') && (!is_escaped))
               {
                  memmove(p, p + 1, strlen(p));
               }
             else if ((*p == '\\') && (*(p + 1) == 'n'))
               {
                  memmove(p, p + 1, strlen(p));
                  *p = '\n';
               }
             else if ((*p == '\\') && (*(p + 1) == 't'))
               {
                  memmove(p, p + 1, strlen(p));
                  *p = '\t';
               }
             else if (*p == '\\')
               {
                  memmove(p, p + 1, strlen(p));
                  if (*p == '\\') p++;
                  else is_escaped = 1;
               }
             else
               {
                  if (is_escaped) is_escaped = 0;
                  p++;
               }
          }
     }
   else if (*tok == '(')
     {
        char *tmp;
        tmp = tok;
        tok = perform_math(tok);
        free(tmp);
     }

   return tok;
}

/**
 * @brief Pushes a token onto the parsing stack and updates the stack ID string.
 *
 * This function appends the given token to the `stack` (Eina_List) and
 * also appends it to `stack_buf` (Eina_Strbuf) to form the stack identifier
 * (e.g., "group.parts.part"). It handles special cases for nested handlers
 * where a token might not be appended to the string buffer if it's part of
 * a recognized nested structure.
 *
 * @param token The token string to push. This string is duplicated by the function.
 */
static void
stack_push(char *token)
{
   New_Nested_Handler *nested;
   Eina_Bool do_append = EINA_TRUE;

   if (eina_list_count(stack) > 1)
     {
        if (!strcmp(token, eina_list_data_get(eina_list_last(stack))))
          {
             char *tmp;
             int token_length;

             token_length = strlen(token);
             tmp = alloca(eina_strbuf_length_get(stack_buf));
             memcpy(tmp,
                    eina_strbuf_string_get(stack_buf),
                    eina_strbuf_length_get(stack_buf) - token_length - 1);
             tmp[eina_strbuf_length_get(stack_buf) - token_length - 1] = '\0';

             nested = eina_hash_find(_new_nested_hash, tmp);
             if (!nested)
               nested = eina_hash_find(_new_nested_short_hash, tmp);
             if (nested)
               {
                  if (!strcmp(token, nested->token) &&
                      stack && !strcmp(eina_list_data_get(eina_list_last(stack)), nested->token))
                    {
                       /* Do not append the nested token in buffer */
                       do_append = EINA_FALSE;
                       if (nested->func_push) nested->func_push();
                    }
               }
          }
     }
   if (do_append)
     {
        if (stack) eina_strbuf_append(stack_buf, ".");
        eina_strbuf_append(stack_buf, token);
     }
   stack = eina_list_append(stack, token);
}

/**
 * @brief Pops a token from the parsing stack and updates the stack ID string.
 *
 * This function removes the top token from the `stack` (Eina_List) and
 * updates `stack_buf` (Eina_Strbuf) by removing the corresponding part of
 * the stack identifier. It also invokes pop handlers for any matched
 * nested structures.
 */
static void
stack_pop(void)
{
   char *tmp;
   int tmp_length;
   Eina_Bool do_remove = EINA_TRUE;

   if (!stack)
     {
        ERR("parse error %s:%i. } marker without matching { marker",
            file_in, line - 1);
        err_show();
        exit(-1);
     }
   tmp = eina_list_data_get(eina_list_last(stack));
   tmp_length = strlen(tmp);

   stack = eina_list_remove_list(stack, eina_list_last(stack));
   if (eina_list_count(stack) > 0)
     {
        const char *prev;
        New_Nested_Handler *nested;
        char *hierarchy;
        char *lookup;

        hierarchy = alloca(eina_strbuf_length_get(stack_buf) + 1);
        memcpy(hierarchy,
               eina_strbuf_string_get(stack_buf),
               eina_strbuf_length_get(stack_buf) + 1);

        /* This is nasty, but it's the way to get parts.part when they are collapsed together. still not perfect */
        lookup = strrchr(hierarchy + eina_strbuf_length_get(stack_buf) - tmp_length, '.');
        while (lookup)
          {
             hierarchy[lookup - hierarchy] = '\0';
             nested = eina_hash_find(_new_nested_hash, hierarchy);
             if (!nested)
               nested = eina_hash_find(_new_nested_short_hash, hierarchy);
             if (nested && nested->func_pop) nested->func_pop();
             lookup = strrchr(hierarchy + eina_strbuf_length_get(stack_buf) - tmp_length, '.');
          }

        hierarchy[eina_strbuf_length_get(stack_buf) - 1 - tmp_length] = '\0';

        nested = eina_hash_find(_new_nested_hash, hierarchy);
        if (!nested)
          nested = eina_hash_find(_new_nested_short_hash, hierarchy);
        if (nested)
          {
             if (nested->func_pop) nested->func_pop();

             prev = eina_list_data_get(eina_list_last(stack));
             if (!strcmp(tmp, prev))
               {
                  if (!strcmp(nested->token, tmp))
                    do_remove = EINA_FALSE;
               }
          }
        else
          edje_cc_handlers_pop_notify(tmp);

        if (do_remove)
          eina_strbuf_remove(stack_buf,
                             eina_strbuf_length_get(stack_buf) - tmp_length - 1,
                             eina_strbuf_length_get(stack_buf));  /* remove: '.tmp' */
     }
   else
     {
        eina_strbuf_remove(stack_buf,
                           eina_strbuf_length_get(stack_buf) - tmp_length,
                           eina_strbuf_length_get(stack_buf)); /* remove: 'tmp' */
     }
   free(tmp);
}

/**
 * @brief Quickly pushes a string onto the parsing stack.
 *
 * This is a simplified version of stack_push, directly appending the string
 * to both the list stack and the string buffer stack ID, prefixed by a dot.
 * It assumes the string `str` is a complete segment to be added.
 *
 * @param str The string to push. It will be duplicated.
 */
void
stack_push_quick(const char *str)
{
   char *s;

   s = mem_strdup(str);
   stack = eina_list_append(stack, s);
   eina_strbuf_append_char(stack_buf, '.');
   eina_strbuf_append(stack_buf, s);
}

/**
 * @brief Quickly pops a string from the parsing stack.
 *
 * This is a simplified version of stack_pop. It removes the last element
 * from the list stack and updates the string buffer stack ID.
 *
 * @param check_last If EINA_TRUE, it considers only the part after the last '.'
 *                   in the popped string for length calculation when removing
 *                   from `stack_buf`. Otherwise, uses the full string length.
 * @param do_free If EINA_TRUE, the popped string data is freed.
 * @return The popped string. If `do_free` is EINA_FALSE, the caller is
 *         responsible for freeing it. If `do_free` is EINA_TRUE, returns NULL.
 */
char *
stack_pop_quick(Eina_Bool check_last, Eina_Bool do_free)
{
   char *tmp, *str;

   str = tmp = eina_list_last_data_get(stack);
   if (check_last)
     {
        char *end;

        end = strrchr(tmp, '.');
        if (end)
          tmp = end + 1;
     }
   eina_strbuf_remove(stack_buf,
                      eina_strbuf_length_get(stack_buf) - strlen(tmp) - 1,
                      eina_strbuf_length_get(stack_buf));      /* remove: '.tmp' */
   stack = eina_list_remove_list(stack, eina_list_last(stack));
   if (do_free)
     {
        free(str);
        str = NULL;
     }
   return str;
}

/**
 * @brief Replaces the top of the stack with a new token.
 *
 * This function effectively pops the current top element and pushes the new
 * token. It handles cases where the popped element itself might be a
 * dot-separated path, ensuring only the last component is replaced in the
 * `stack_buf`.
 *
 * @param token The new token to place at the top of the stack.
 */
void
stack_replace_quick(const char *token)
{
   char *str;

   str = stack_pop_quick(EINA_FALSE, EINA_FALSE);
   if ((str) && strchr(str, '.'))
     {
        char *end, *tmp = str;
        Eina_Strbuf *buf;

        end = strchr(tmp, '.');
        if (end)
          tmp = end + 1;

        buf = eina_strbuf_new();
        eina_strbuf_append(buf, str);
        eina_strbuf_remove(buf,
                           eina_strbuf_length_get(buf) - strlen(tmp),
                           eina_strbuf_length_get(buf));
        eina_strbuf_append(buf, token);

        stack_push_quick(eina_strbuf_string_get(buf));

        eina_strbuf_free(buf);
        free(str);
     }
   else
     {
        stack_push_quick(token);
     }
}

/**
 * @brief Gets the current parsing stack identifier as a string.
 * @return A string representing the current stack, e.g., "group.parts.part".
 *         This string is owned by an internal Eina_Strbuf and should not be freed
 *         or modified by the caller.
 */
static const char *
stack_id(void)
{
   return eina_strbuf_string_get(stack_buf);
}

/**
 * @brief Parses the entire input data buffer.
 *
 * This is the main parsing loop. It repeatedly calls `next_token` to get tokens
 * and then processes them based on whether they are delimiters or regular tokens.
 * It manages the parsing stack, handles parameters for statements, and invokes
 * object/statement handlers. It also deals with verbatim blocks.
 *
 * @param data The input data buffer to parse.
 * @param size The size of the input data buffer.
 */
static void
parse(char *data, off_t size)
{
   char *p, *end, *token;
   int delim = 0;
   int do_params = 0;
   int do_indexes = 0;  // 0: none, 1: ready, 2: done

   DBG("Parsing input file");

   /* Allocate arrays used to impl nested parts */
   edje_cc_handlers_hierarchy_alloc();
   p = data;
   end = data + size;
   line = 1;
   while ((token = next_token(p, end, &p, &delim)))
     {
        /* if we are in param mode, the only delimiter
         * we'll accept is the semicolon
         */
        if (do_params && delim && *token != ';')
          {
             ERR("parse error %s:%i. %c marker before ; marker",
                 file_in, line - 1, *token);
             err_show();
             exit(-1);
          }
        else if (delim)
          {
             if ((do_indexes == 2) && (*token != ']'))
               {
                  ERR("parse error %s:%i. %c marker before ] marker",
                      file_in, line - 1, *token);
                  err_show();
                  exit(-1);
               }
             else if (*token == ',' || *token == ':')
               do_params = 1;
             else if (*token == '}')
               {
                  if (do_params)
                    {
                       ERR("parse error %s:%i. } marker before ; marker",
                           file_in, line - 1);
                       err_show();
                       exit(-1);
                    }
                  else
                    stack_pop();
               }
             else if (*token == ';')
               {
                  if (did_wildcard)
                    {
                       free(token);
                       did_wildcard = 0;
                       continue;
                    }
                  if (do_params)
                    {
                       void *param;

                       do_params = 0;
                       new_statement();
                       /* clear out params */
                       while ((param = eina_array_pop(&params)))
                         free(param);
                       params_quote = 0;
                       /* remove top from stack */
                       stack_pop();
                    }
                  else
                    {
                       if (new_statement_single())
                         stack_pop();
                    }
               }
             else if (*token == '{')
               {
                  if (do_params)
                    {
                       ERR("parse error %s:%i. { marker before ; marker",
                           file_in, line - 1);
                       err_show();
                       exit(-1);
                    }
               }
             else if (*token == '[')
               {
                  do_indexes = 1;
               }
             else if (*token == ']')
               {
                  if (do_indexes == 2)
                    do_indexes = 0;
                  else
                    {
                       if (do_indexes == 0)
                         ERR("parse error %s:%i. ] marker before [ marker",
                             file_in, line - 1);
                       else
                         ERR("parse error %s:%i. [?] empty bracket",
                             file_in, line - 1);

                       err_show();
                       exit(-1);
                    }
               }
             free(token);
          }
        else
          {
             if (do_params)
               {
                  if (had_quote)
                    params_quote |= (1 << eina_array_count(&params));
                  eina_array_push(&params, token);
               }
             else if (do_indexes)
               {
                  if (had_quote)
                    params_quote |= (1 << eina_array_count(&params));
                  do_indexes++;
                  eina_array_push(&params, token);
               }
             else
               {
                  stack_push(token);
                  new_object();
                  if ((verbatim == 1) && (p < (end - 2)))
                    {
                       int escaped = 0;
                       int inquotes = 0;
                       int insquotes = 0;
                       int squigglie = 1;
                       int l1 = 0, l2 = 0;
                       char *verbatim_1;
                       char *verbatim_2;

                       l1 = line;
                       while ((p[0] != '{') && (p < end))
                         {
                            if (*p == '\n') line++;
                            p++;
                         }
                       p++;
                       verbatim_1 = p;
                       verbatim_2 = NULL;
                       for (; p < end; p++)
                         {
                            if (*p == '\n') line++;
                            if (escaped) escaped = 0;
                            if (!escaped)
                              {
                                 if (p[0] == '\\') escaped = 1;
                                 else if (p[0] == '\"')
                                   {
                                      if (!insquotes)
                                        {
                                           if (inquotes) inquotes = 0;
                                           else inquotes = 1;
                                        }
                                   }
                                 else if (p[0] == '\'')
                                   {
                                      if (!inquotes)
                                        {
                                           if (insquotes) insquotes = 0;
                                           else insquotes = 1;
                                        }
                                   }
                                 else if ((!inquotes) && (!insquotes))
                                   {
                                      if (p[0] == '{') squigglie++;
                                      else if (p[0] == '}')
                                        squigglie--;
                                      if (squigglie == 0)
                                        {
                                           verbatim_2 = p - 1;
                                           l2 = line;
                                           break;
                                        }
                                   }
                              }
                         }
                       if (verbatim_2 > verbatim_1)
                         {
                            int l;
                            char *v;

                            l = verbatim_2 - verbatim_1 + 1;
                            v = malloc(l + 1);
                            strncpy(v, verbatim_1, l);
                            v[l] = 0;
                            set_verbatim(v, l1, l2);
                         }
                       else
                         {
                            ERR("Parse error %s:%i. { marker does not have matching } marker",
                                file_in, line - 1);
                            err_show();
                            exit(-1);
                         }
                       new_object();
                       verbatim = 0;
                    }
               }
          }
     }

   edje_cc_handlers_hierarchy_free();
   DBG("Parsing done");
}

static char *clean_file = NULL; ///< Path to the temporary file created by the preprocessor.

/**
 * @brief Cleans up (deletes) the temporary file created by the preprocessor.
 * Registered with atexit() to be called on program termination.
 */
static void
clean_tmp_file(void)
{
   if (clean_file)
     {
        unlink(clean_file);
        free(clean_file);
     }
}

/**
 * @brief Checks if the parser is currently in verbatim mode.
 * @return 1 if in verbatim mode, 0 otherwise.
 */
int
is_verbatim(void)
{
   return verbatim;
}

/**
 * @brief Sets the verbatim mode tracking.
 * @param on 1 to enable verbatim mode, 0 to disable.
 */
void
track_verbatim(int on)
{
   verbatim = on;
}

/**
 * @brief Sets the content and line numbers for a verbatim block.
 * @param s The string content of the verbatim block. The parser takes ownership.
 * @param l1 The starting line number of the verbatim block.
 * @param l2 The ending line number of the verbatim block.
 */
void
set_verbatim(char *s, int l1, int l2)
{
   verbatim_line1 = l1;
   verbatim_line2 = l2;
   verbatim_str = s;
}

/**
 * @brief Gets the content of the current verbatim block.
 * @return The string content of the verbatim block, or NULL if not in one.
 */
char *
get_verbatim(void)
{
   return verbatim_str;
}

/**
 * @brief Gets the starting line number of the current verbatim block.
 * @return The starting line number.
 */
int
get_verbatim_line1(void)
{
   return verbatim_line1;
}

/**
 * @brief Gets the ending line number of the current verbatim block.
 * @return The ending line number.
 */
int
get_verbatim_line2(void)
{
   return verbatim_line2;
}

/**
 * @brief Main compilation function.
 *
 * This function orchestrates the compilation process:
 * 1. Runs the input file through a C preprocessor (epp).
 * 2. Reads the preprocessed output.
 * 3. Parses the preprocessed data using the `parse` function.
 * 4. Performs final checks (e.g., styles must have names).
 */
void
compile(void)
{
   char buf[4096 + 4096 + 4096], buf2[4096];
   Eina_Tmpstr *tmpn;
   int fd;
   off_t size;
   char *data;
   Eina_List *l;
   Edje_Style *stl;

   fd = eina_file_mkstemp("edje_cc.edc-tmp-XXXXXX", &tmpn);
   if (fd < 0)
     {
        CRI("Unable to open temp file \"%s\" for pre-processor.", tmpn);
        exit(-1);
     }

   if (fd >= 0)
     {
        int ret;
        char *def;

        clean_file = strdup(tmpn);
        eina_tmpstr_del(tmpn);
        close(fd);
        atexit(clean_tmp_file);
        if (!defines)
          def = mem_strdup("");
        else
          {
             int len;
             char *define;

             len = 0;
             EINA_LIST_FOREACH(defines, l, define)
               len += strlen(define) + 1;
             def = mem_alloc(len + 1);
             def[0] = 0;
             EINA_LIST_FOREACH(defines, l, define)
               {
                  strcat(def, define);
                  strcat(def, " ");
               }
          }

        /*
         * Run the input through the C pre-processor.
         */

        buf2[0] = '\0';
#ifdef NEED_RUN_IN_TREE
        if (getenv("EFL_RUN_IN_TREE"))
          {
             snprintf(buf2, sizeof(buf2),
                      "%s/src/bin/edje/epp/epp" EPP_EXT,
                      PACKAGE_BUILD_DIR);
             if (!ecore_file_exists(buf2))
               buf2[0] = '\0';
          }
#endif

        if (buf2[0] == '\0')
          snprintf(buf2, sizeof(buf2),
                   "%s/edje/utils/" MODULE_ARCH "/epp" EPP_EXT,
                   eina_prefix_lib_get(pfx));
        if (ecore_file_exists(buf2))
          {
             char *inc;

             inc = ecore_file_dir_get(file_in);
             if (depfile)
               snprintf(buf, sizeof(buf), "\"%s\" "SKIP_NAMESPACE_VALIDATION_SUPPORTED" -MMD \"%s\" -MT \"%s\" \"%s\""
                                          " -I\"%s\" %s -o \"%s\""
                                          " -DEFL_VERSION_MAJOR=%d -DEFL_VERSION_MINOR=%d"
                        EDJE_CC_EFL_VERSION_SUPPORTED,
                        buf2, depfile, file_out, file_in,
                        inc ? inc : "./", def, clean_file,
                        EINA_VERSION_MAJOR, EINA_VERSION_MINOR);
             else if (annotate)
               snprintf(buf, sizeof(buf), "\"%s\" "SKIP_NAMESPACE_VALIDATION_SUPPORTED" -annotate -a \"%s\" \"%s\""
                                          " -I\"%s\" %s -o \"%s\""
                                          " -DEFL_VERSION_MAJOR=%d -DEFL_VERSION_MINOR=%d"
                        EDJE_CC_EFL_VERSION_SUPPORTED,
                        buf2, watchfile ? watchfile : "/dev/null", file_in,
                        inc ? inc : "./", def, clean_file,
                        EINA_VERSION_MAJOR, EINA_VERSION_MINOR);
             else
               snprintf(buf, sizeof(buf), "\"%s\" "SKIP_NAMESPACE_VALIDATION_SUPPORTED" -a \"%s\" \"%s\" -I\"%s\" %s"
                                          " -o \"%s\""
                                          " -DEFL_VERSION_MAJOR=%d -DEFL_VERSION_MINOR=%d"
                        EDJE_CC_EFL_VERSION_SUPPORTED,
                        buf2, watchfile ? watchfile : "/dev/null", file_in,
                        inc ? inc : "./", def, clean_file,
                        EINA_VERSION_MAJOR, EINA_VERSION_MINOR);
#ifdef _WIN32
             /* On Windows, if command begins with double quotation marks,
              * then the first and the last double quotation marks may be
              * either deleted or not. (See "help cmd" on Windows.)
              *
              * Therefore, to preserve the string between the first and the last
              * double quotation marks, "cmd /S /C" and additional outer double
              * quotation marks are added.
              */
             char win_buf[4096];
             snprintf(win_buf, sizeof(win_buf), "cmd /S /C \"%s\"", buf);
             ret = system(win_buf);
#else
             ret = system(buf);
#endif
             if (inc)
               free(inc);
          }
        else
          {
             ERR("Cannot run epp: %s", buf2);
             exit(-1);
          }
        if (ret == EXIT_SUCCESS)
          file_in = (char *)clean_file;
        else
          {
             ERR("Exit code of epp not clean: %i", ret);
             exit(-1);
          }
        free(def);
     }
   fd = open(file_in, O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR);
   if (fd < 0)
     {
        ERR("Cannot open file \"%s\" for input. %s",
            file_in, strerror(errno));
        exit(-1);
     }
   DBG("Opening \"%s\" for input", file_in);

   /* lseek can return -1 on error. trap that return and exit so that
    * we do not pass malloc a -1
    *
    * NB: Fixes Coverity CID 1040029 */
   size = lseek(fd, 0, SEEK_END);
   if (size < 0)
     {
        ERR("Cannot read file \"%s\". %s", file_in, strerror(errno));
        exit(-1);
     }

   lseek(fd, 0, SEEK_SET);
   data = malloc(size);
   if (data && (read(fd, data, size) == size))
     {
        stack_buf = eina_strbuf_new();
        eina_array_step_set(&params, sizeof (Eina_Array), 8);
        parse(data, size);
        eina_array_flush(&params);
        eina_strbuf_free(stack_buf);
        stack_buf = NULL;
        color_tree_root_free();
     }
   else
     {
        ERR("Cannot read file \"%s\". %s", file_in, strerror(errno));
        exit(-1);
     }
   free(data);
   close(fd);

   EINA_LIST_FOREACH(edje_file->styles, l, stl)
     {
        if (!stl->name)
          {
             ERR("style must have a name.");
             exit(-1);
          }
     }
}

/**
 * @brief Checks if a parameter exists at a given index.
 * @param n The index of the parameter.
 * @return 1 if the parameter exists, 0 otherwise.
 */
int
is_param(int n)
{
   char *str;

   str = _parse_param_get(n);
   if (str) return 1;
   return 0;
}

/**
 * @brief Checks if the parameter at a given index is a valid number.
 * @param n The index of the parameter.
 * @return 1 if the parameter is a number, 0 otherwise. Exits on error if parameter doesn't exist.
 */
int
is_num(int n)
{
   char *str;
   char *end;
   long int ret;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   if (str[0] == 0) return 0;
   end = str;
   ret = strtol(str, &end, 0);
   if ((ret == LONG_MIN) || (ret == LONG_MAX))
     {
        n = 0; // do nothing. shut gcc warnings up
     }
   if ((end != str) && (end[0] == 0)) return 1;
   return 0;
}

/**
 * @brief Parses the parameter at a given index as a string.
 * @param n The index of the parameter.
 * @return A newly allocated string which is a copy of the parameter.
 *         The caller is responsible for freeing this string.
 *         Exits on error if parameter doesn't exist.
 */
char *
parse_str(int n)
{
   char *str;
   char *s;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   s = mem_strdup(str);
   return s;
}

/**
 * @brief Helper function to parse an enum value from a string against a va_list of string-value pairs.
 * @param str The string token to match against enum string representations.
 * @param va A va_list of alternating `char *` (enum string) and `int` (enum value).
 *           The list must be terminated by a NULL `char *`.
 *           A wildcard "*" string can be used to match any token.
 * @return The integer value corresponding to the matched enum string.
 *         Exits on error if no match is found.
 *
 * Example va_list structure:
 * "option1", OPTION1_VALUE, "option2", OPTION2_VALUE, "*", DEFAULT_VALUE, NULL
 */
static int
_parse_enum(char *str, va_list va)
{
   va_list va2;
   va_copy(va2, va); /* iterator for the error message */

   for (;; )
     {
        char *s;
        int v;

        s = va_arg(va, char *);

        /* End of the list, nothing matched. */
        if (!s)
          {
             ERR("%s:%i token %s not one of:", file_in, line - 1, str);
             s = va_arg(va2, char *);
             while (s)
               {
                  va_arg(va2, int);
                  fprintf(stderr, " %s", s);
                  s = va_arg(va2, char *);
                  if (!s) break;
               }
             fprintf(stderr, "\n");
             va_end(va2);
             va_end(va);
             err_show();
             exit(-1);
          }

        v = va_arg(va, int);
        if (!strcmp(s, str) || !strcmp(s, "*"))
          {
             va_end(va2);
             va_end(va);
             return v;
          }
     }
   va_end(va2);
   va_end(va);
   return 0;
}

/**
 * @brief Parses an enum value from a parameter or the last stack token.
 * @param n The index of the parameter to parse. If -1, uses the last token on the stack.
 * @param ... A variable argument list of alternating `char *` (enum string)
 *            and `int` (enum value), terminated by a NULL `char *`.
 *            Example: parse_enum(0, "NONE", 0, "SOLID", 1, NULL);
 * @return The integer value corresponding to the matched enum string.
 *         Exits on error if the parameter doesn't exist or no match is found.
 */
int
parse_enum(int n, ...)
{
   char *str;
   int result;
   va_list va;

   if (n >= 0)
     {
        str = _parse_param_get(n);
        if (!str)
          {
             ERR("%s:%i no parameter supplied as argument %i",
                 file_in, line - 1, n + 1);
             err_show();
             exit(-1);
          }
     }
   else
     {
        char *end;

        str = eina_list_last_data_get(stack);
        end = strrchr(str, '.');
        if (end)
          str = end + 1;
     }

   va_start(va, n);
   result = _parse_enum(str, va);
   va_end(va);

   return result;
}

/**
 * @brief Parses multiple flag values from parameters, combining them with bitwise OR.
 *
 * Iterates through parameters starting from index `n` and tries to match each
 * against the provided enum string-value pairs. The matched integer values
 * are OR'd together.
 *
 * @param n The starting index of parameters to parse as flags.
 * @param ... A variable argument list of alternating `char *` (flag string)
 *            and `int` (flag value), terminated by a NULL `char *`.
 *            Example: parse_flags(1, "FLAG_A", 0x01, "FLAG_B", 0x02, NULL);
 * @return The combined integer value of all matched flags.
 */
int
parse_flags(int n, ...)
{
   int result = 0;
   va_list va;

   va_start(va, n);
   while (n < (int)eina_array_count(&params))
     {
        result |= _parse_enum(eina_array_data_get(&params, n), va);
        n++;
     }
   va_end(va);

   return result;
}

/**
 * @brief Parses the parameter at a given index as an integer.
 *
 * The parameter string can be a simple integer or a mathematical expression
 * that evaluates to an integer (e.g., "(10 + 5) * 2").
 *
 * @param n The index of the parameter.
 * @return The parsed integer value.
 *         Exits on error if parameter doesn't exist or is not a valid integer/expression.
 */
int
parse_int(int n)
{
   char *str;
   int i;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   i = my_atoi(str);
   return i;
}

/**
 * @brief Parses the parameter at a given index as an integer and checks if it's within a specified range.
 * @param n The index of the parameter.
 * @param f The minimum allowed value (inclusive).
 * @param t The maximum allowed value (inclusive).
 * @return The parsed integer value if it's within the range [f, t].
 *         Exits on error if parameter doesn't exist, is not a valid integer, or is out of range.
 */
int
parse_int_range(int n, int f, int t)
{
   char *str;
   int i;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   i = my_atoi(str);
   if ((i < f) || (i > t))
     {
        ERR("%s:%i integer %i out of range of %i to %i inclusive",
            file_in, line - 1, i, f, t);
        err_show();
        exit(-1);
     }
   return i;
}

/**
 * @brief Parses the parameter at a given index as a boolean value.
 *
 * Recognizes "true", "on" as true (1) and "false", "off" as false (0).
 * Also accepts integers 0 or 1. Case-insensitive for strings.
 *
 * @param n The index of the parameter.
 * @return 1 for true, 0 for false.
 *         Exits on error if parameter doesn't exist or is not a valid boolean representation.
 */
int
parse_bool(int n)
{
   char *str, buf[4096];
   int i;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }

   if (!strstrip(str, buf, sizeof (buf)))
     {
        ERR("%s:%i expression is too long",
            file_in, line - 1);
        return 0;
     }

   if (!strcasecmp(buf, "false") || !strcasecmp(buf, "off"))
     return 0;
   if (!strcasecmp(buf, "true") || !strcasecmp(buf, "on"))
     return 1;

   i = my_atoi(str);
   if ((i < 0) || (i > 1))
     {
        ERR("%s:%i integer %i out of range of 0 to 1 inclusive",
            file_in, line - 1, i);
        err_show();
        exit(-1);
     }
   return i;
}

/**
 * @brief Parses the parameter at a given index as a double-precision floating-point number.
 *
 * The parameter string can be a simple float or a mathematical expression
 * that evaluates to a float (e.g., "(10.5 + 5.0) * 2.0").
 *
 * @param n The index of the parameter.
 * @return The parsed double value.
 *         Exits on error if parameter doesn't exist or is not a valid float/expression.
 */
double
parse_float(int n)
{
   char *str;
   double i;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   i = my_atof(str);
   return i;
}

/**
 * @brief Parses the parameter at a given index as a double and checks if it's within a specified range.
 * @param n The index of the parameter.
 * @param f The minimum allowed value (inclusive).
 * @param t The maximum allowed value (inclusive).
 * @return The parsed double value if it's within the range [f, t].
 *         Exits on error if parameter doesn't exist, is not a valid float, or is out of range.
 */
double
parse_float_range(int n, double f, double t)
{
   char *str;
   double i;

   str = _parse_param_get(n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }
   i = my_atof(str);
   if ((i < f) || (i > t))
     {
        ERR("%s:%i float %3.3f out of range of %3.3f to %3.3f inclusive",
            file_in, line - 1, i, f, t);
        err_show();
        exit(-1);
     }
   return i;
}

/**
 * @brief Gets the current number of arguments/parameters collected for a statement.
 * @return The count of arguments in the `params` array.
 */
int
get_arg_count(void)
{
   return eina_array_count(&params);
}

/**
 * @brief Checks if the current number of arguments matches an exact required count.
 * @param required_args The exact number of arguments expected.
 * Exits on error if the count does not match.
 */
void
check_arg_count(int required_args)
{
   int num_args = eina_array_count(&params);

   if (num_args != required_args)
     {
        ERR("%s:%i got %i arguments, but expected %i",
            file_in, line - 1, num_args, required_args);
        err_show();
        exit(-1);
     }
}

/**
 * @brief Checks if the current number of arguments meets a minimum required count.
 * @param min_required_args The minimum number of arguments expected.
 * Exits on error if the count is less than the minimum.
 */
void
check_min_arg_count(int min_required_args)
{
   int num_args = eina_array_count(&params);

   if (num_args < min_required_args)
     {
        ERR("%s:%i got %i arguments, but expected at least %i",
            file_in, line - 1, num_args, min_required_args);
        err_show();
        exit(-1);
     }
}

/**
 * @brief Checks if the current number of arguments falls within a specified range.
 * @param min_required_args The minimum number of arguments expected (inclusive).
 * @param max_required_args The maximum number of arguments expected (inclusive).
 * @return The number of arguments if it's within the range.
 * Exits on error if the count is outside the range.
 */
int
check_range_arg_count(int min_required_args, int max_required_args)
{
   int num_args = eina_array_count(&params);

   if (num_args < min_required_args)
     {
        ERR("%s:%i got %i arguments, but expected at least %i",
            file_in, line - 1, num_args, min_required_args);
        err_show();
        exit(-1);
     }
   else if (num_args > max_required_args)
     {
        ERR("%s:%i got %i arguments, but expected at most %i",
            file_in, line - 1, num_args, max_required_args);
        err_show();
        exit(-1);
     }

   return num_args;
}

/* simple expression parsing stuff */
/**
 * @defgroup IntExprParse Integer Expression Parser
 * @{
 * Recursive descent parser for simple integer arithmetic expressions.
 * Supports +, -, *, /, %, parentheses, and floor()/ceil() functions (though functions are more for float).
 * Grammar:
 * alpha ::= beta {('+'|'-') beta}
 * beta  ::= gamma {('*'|'/'|'%') gamma}
 * gamma ::= NUMBER | '(' alpha ')' | FUNCTION '(' alpha ')'
 */

/*
 * alpha ::= beta + beta || beta
 * beta  ::= gamma + gamma || gamma
 * gamma ::= num || delta
 * delta ::= '(' alpha ')'
 *
 */

/* int set of function */

/**
 * @brief Converts a string to an integer, performing expression evaluation.
 * @param s The string to convert. Can be a simple number or an expression.
 *          Example: "10", "(5+3)*2"
 * @return The integer result of the conversion/evaluation.
 * @ingroup IntExprParse
 */
static int
my_atoi(const char *s)
{
   int res = 0;
   char buf[4096];

   if (!s) return 0;
   if (!strstrip(s, buf, sizeof(buf)))
     {
        ERR("%s:%i expression is too long",
            file_in, line - 1);
        return 0;
     }
   _alphai(buf, &res);
   return res;
}

/**
 * @brief Parses a parenthesized integer expression (delta rule: '(' alpha ')').
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result of the parsed sub-expression.
 * @return Pointer to the character in the string after the parsed sub-expression.
 * @ingroup IntExprParse
 */
static char *
_deltai(char *s, int *val)
{
   if (!val) return NULL;
   if ('(' != s[0])
     {
        ERR("%s:%i unexpected character at %s",
            file_in, line - 1, s);
        return s;
     }
   else
     {
        s++;
        s = _alphai(s, val);
        s++; // Expect and consume ')'
        return s;
     }
   return s; // Should not be reached if grammar is correct
}

/**
 * @brief Parses an integer function call (e.g., floor(), ceil()).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result of the function call.
 * @return Pointer to the character in the string after the parsed function call.
 * @ingroup IntExprParse
 * @note For integer math, floor and ceil on an integer result in the integer itself.
 *       This primarily exists for symmetry with float parsing.
 */
static char *
_funci(char *s, int *val)
{
   if (!strncmp(s, "floor(", 6))
     {
        s += 5; // Skip "floor"
        s = _deltai(s, val); // Parse the argument as ( expression )
        // For integers, floor(val) is val.
     }
   else if (!strncmp(s, "ceil(", 5))
     {
        s += 4; // Skip "ceil"
        s = _deltai(s, val); // Parse the argument as ( expression )
        // For integers, ceil(val) is val.
     }
   else
     {
        ERR("%s:%i unexpected character at %s", // Or unknown function
            file_in, line - 1, s);
     }
   return s;
}

/**
 * @brief Parses a number, a parenthesized expression, or a function call (gamma rule).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result.
 * @return Pointer to the character in the string after the parsed element.
 * @ingroup IntExprParse
 */
static char *
_gammai(char *s, int *val)
{
   if (!val) return NULL;
   if (_is_numi(s[0])) // Check if it starts with a digit or sign
     {
        s = _get_numi(s, val);
        return s;
     }
   else if ('(' == s[0]) // Check for parenthesized expression
     {
        s = _deltai(s, val);
        return s;
     }
   else // Assume it's a function call like floor() or ceil()
     {
        s = _funci(s, val);
     }
   return s;
}

/**
 * @brief Parses multiplication, division, and modulo operations (beta rule: gamma {('*'|'/'|'%') gamma}).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result.
 * @return Pointer to the character in the string after the parsed operations.
 * @ingroup IntExprParse
 */
static char *
_betai(char *s, int *val)
{
   int a1, a2;
   char op;

   if (!val) return NULL;
   s = _gammai(s, &a1); // Parse the first operand
   while (_is_op1i(s[0])) // While there are high-precedence operators
     {
        op = s[0];
        s++;
        s = _gammai(s, &a2); // Parse the second operand
        a1 = _calci(op, a1, a2); // Perform calculation
     }
   (*val) = a1;
   return s;
}

/**
 * @brief Parses addition and subtraction operations (alpha rule: beta {('+'|'-') beta}).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the final result of the expression.
 * @return Pointer to the character in the string after the parsed operations.
 * @ingroup IntExprParse
 */
static char *
_alphai(char *s, int *val)
{
   int a1 = 0, a2 = 0;
   char op;

   if (!val) return NULL;
   s = _betai(s, &a1); // Parse the first term (which could be a product/quotient)
   while (_is_op2i(s[0])) // While there are low-precedence operators
     {
        op = s[0];
        s++;
        s = _betai(s, &a2); // Parse the next term
        a1 = _calci(op, a1, a2); // Perform calculation
     }
   (*val) = a1;
   return s;
}

/**
 * @brief Extracts an integer number from the string.
 * @param s Pointer to the current character in the expression string, expected to be start of a number.
 * @param val Pointer to store the extracted integer.
 * @return Pointer to the character in the string after the number.
 * @ingroup IntExprParse
 */
char *
_get_numi(char *s, int *val)
{
   char buf[4096];
   int pos = 0;

   if (!val) return s;
   // Handles optional leading '-'
   while ((('0' <= s[pos]) && ('9' >= s[pos])) || // Digits
          ((0 == pos) && ('-' == s[pos])))      // Leading minus sign
     {
        buf[pos] = s[pos];
        pos++;
     }
   buf[pos] = '\0';
   (*val) = atoi(buf);
   return s + pos;
}

/**
 * @brief Checks if a character can be the start or part of an integer number.
 * @param c The character to check.
 * @return 1 if it's a digit, '+' or '-', 0 otherwise.
 * @ingroup IntExprParse
 */
int
_is_numi(char c)
{
   if (((c >= '0') && (c <= '9')) || ('-' == c) || ('+' == c)) // '+' is for completeness, _get_numi handles '-'
     return 1;
   else
     return 0;
}

/**
 * @brief Checks if a character is a high-precedence integer operator (*, /, %).
 * @param c The character to check.
 * @return 1 if it's a high-precedence operator, 0 otherwise.
 * @ingroup IntExprParse
 */
int
_is_op1i(char c)
{
   switch (c)
     {
      case '*':

      case '%':

      case '/': return 1;

      default: break;
     }
   return 0;
}

/**
 * @brief Checks if a character is a low-precedence integer operator (+, -).
 * @param c The character to check.
 * @return 1 if it's a low-precedence operator, 0 otherwise.
 * @ingroup IntExprParse
 */
int
_is_op2i(char c)
{
   switch (c)
     {
      case '+':

      case '-': return 1;

      default: break;
     }
   return 0;
}

/**
 * @brief Performs an integer calculation based on an operator and two operands.
 * @param op The operator character (+, -, *, /, %).
 * @param a The first operand.
 * @param b The second operand.
 * @return The result of the calculation. Exits on division/modulo by zero or unknown operator.
 * @ingroup IntExprParse
 */
int
_calci(char op, int a, int b)
{
   switch (op)
     {
      case '+':
        a += b;
        return a;

      case '-':
        a -= b;
        return a;

      case '/':
        if (0 != b) a /= b;
        else
          ERR("%s:%i divide by zero", file_in, line - 1);
        return a;

      case '*':
        a *= b;
        return a;

      case '%':
        if (0 != b) a = a % b;
        else
          ERR("%s:%i modula by zero", file_in, line - 1);
        return a;

      default:
        ERR("%s:%i unexpected character '%c'", file_in, line - 1, op);
     }
   return a; // Should be unreachable if errors exit
}
/** @} */ // end of IntExprParse group

/* float set of functoins */
/**
 * @defgroup FloatExprParse Floating-Point Expression Parser
 * @{
 * Recursive descent parser for simple floating-point arithmetic expressions.
 * Supports +, -, *, /, %, parentheses, and floor()/ceil() functions.
 * Grammar (similar to integer parser):
 * alpha ::= beta {('+'|'-') beta}
 * beta  ::= gamma {('*'|'/'|'%') gamma}
 * gamma ::= NUMBER | '(' alpha ')' | FUNCTION '(' alpha ')'
 */

/**
 * @brief Converts a string to a double, performing expression evaluation.
 * @param s The string to convert. Can be a simple number or an expression.
 *          Example: "10.5", "(5.1+3.2)*2.0"
 * @return The double result of the conversion/evaluation.
 * @ingroup FloatExprParse
 */
double
my_atof(const char *s)
{
   double res = 0;
   char buf[4096];

   if (!s) return 0;

   if (!strstrip(s, buf, sizeof (buf)))
     {
        ERR("%s:%i expression is too long", file_in, line - 1);
        return 0;
     }
   _alphaf(buf, &res);
   return res;
}

/**
 * @brief Parses a parenthesized floating-point expression (delta rule: '(' alpha ')').
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result of the parsed sub-expression.
 * @return Pointer to the character in the string after the parsed sub-expression.
 * @ingroup FloatExprParse
 */
static char *
_deltaf(char *s, double *val)
{
   if (!val) return NULL;
   if ('(' != s[0])
     {
        ERR("%s:%i unexpected character at %s", file_in, line - 1, s);
        return s;
     }
   else
     {
        s++;
        s = _alphaf(s, val);
        s++; // Expect and consume ')'
     }
   return s;
}

/**
 * @brief Parses a floating-point function call (e.g., floor(), ceil()).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result of the function call.
 * @return Pointer to the character in the string after the parsed function call.
 * @ingroup FloatExprParse
 */
static char *
_funcf(char *s, double *val)
{
   if (!strncmp(s, "floor(", 6))
     {
        s += 5; // Skip "floor"
        s = _deltaf(s, val); // Parse argument: ( expression )
        *val = floor(*val);
     }
   else if (!strncmp(s, "ceil(", 5))
     {
        s += 4; // Skip "ceil"
        s = _deltaf(s, val); // Parse argument: ( expression )
        *val = ceil(*val);
     }
   else
     {
        ERR("%s:%i unexpected character at %s", file_in, line - 1, s); // Or unknown function
     }
   return s;
}

/**
 * @brief Parses a number, a parenthesized expression, or a function call (gamma rule) for floats.
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result.
 * @return Pointer to the character in the string after the parsed element.
 * @ingroup FloatExprParse
 */
static char *
_gammaf(char *s, double *val)
{
   if (!val) return NULL;

   if (_is_numf(s[0])) // Check if it starts with a digit, sign or '.'
     {
        s = _get_numf(s, val);
        return s;
     }
   else if ('(' == s[0]) // Check for parenthesized expression
     {
        s = _deltaf(s, val);
        return s;
     }
   else // Assume it's a function call
     {
        s = _funcf(s, val);
     }
   return s;
}

/**
 * @brief Parses multiplication, division, and modulo operations for floats (beta rule).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the result.
 * @return Pointer to the character in the string after the parsed operations.
 * @ingroup FloatExprParse
 */
static char *
_betaf(char *s, double *val)
{
   double a1 = 0, a2 = 0;
   char op;

   if (!val) return NULL;
   s = _gammaf(s, &a1); // Parse the first operand
   while (_is_op1f(s[0])) // While there are high-precedence operators
     {
        op = s[0];
        s++;
        s = _gammaf(s, &a2); // Parse the second operand
        a1 = _calcf(op, a1, a2); // Perform calculation
     }
   (*val) = a1;
   return s;
}

/**
 * @brief Parses addition and subtraction operations for floats (alpha rule).
 * @param s Pointer to the current character in the expression string.
 * @param val Pointer to store the final result of the expression.
 * @return Pointer to the character in the string after the parsed operations.
 * @ingroup FloatExprParse
 */
static char *
_alphaf(char *s, double *val)
{
   double a1 = 0, a2 = 0;
   char op;

   if (!val) return NULL;
   s = _betaf(s, &a1); // Parse the first term
   while (_is_op2f(s[0])) // While there are low-precedence operators
     {
        op = s[0];
        s++;
        s = _betaf(s, &a2); // Parse the next term
        a1 = _calcf(op, a1, a2); // Perform calculation
     }
   (*val) = a1;
   return s;
}

/**
 * @brief Extracts a floating-point number from the string.
 * @param s Pointer to the current character, expected to be start of a float.
 * @param val Pointer to store the extracted double.
 * @return Pointer to the character in the string after the number.
 * @ingroup FloatExprParse
 */
static char *
_get_numf(char *s, double *val)
{
   char buf[4096];
   int pos = 0;

   if (!val) return s;

   // Handles optional leading '-', digits, and one decimal point
   while ((('0' <= s[pos]) && ('9' >= s[pos])) || // Digits
          ('.' == s[pos]) ||                      // Decimal point
          ((0 == pos) && ('-' == s[pos])))      // Leading minus sign
     {
        buf[pos] = s[pos];
        pos++;
     }
   buf[pos] = '\0';
   (*val) = eina_convert_strtod_c(buf, NULL); // Locale-independent strtod
   return s + pos;
}

/**
 * @brief Checks if a character can be part of a floating-point number.
 * @param c The character to check.
 * @return 1 if it's a digit, '.', '+', or '-', 0 otherwise.
 * @ingroup FloatExprParse
 */
static int
_is_numf(char c)
{
   if (((c >= '0') && (c <= '9'))
       || ('-' == c)
       || ('.' == c)
       || ('+' == c)) // '+' for completeness, _get_numf handles '-' and '.'
     return 1;
   return 0;
}

/**
 * @brief Checks if a character is a high-precedence float operator (*, /, %).
 * @param c The character to check.
 * @return 1 if it's a high-precedence operator, 0 otherwise.
 * @ingroup FloatExprParse
 */
static int
_is_op1f(char c)
{
   switch (c)
     {
      case '*':

      case '%': // Modulo for floats is typically integer conversion then modulo

      case '/': return 1;

      default: break;
     }
   return 0;
}

/**
 * @brief Checks if a character is a low-precedence float operator (+, -).
 * @param c The character to check.
 * @return 1 if it's a low-precedence operator, 0 otherwise.
 * @ingroup FloatExprParse
 */
static int
_is_op2f(char c)
{
   switch (c)
     {
      case '+':

      case '-': return 1;

      default: break;
     }
   return 0;
}

/**
 * @brief Performs a floating-point calculation.
 * @param op The operator character.
 * @param a The first operand.
 * @param b The second operand.
 * @return The result of the calculation. Exits on division/modulo by zero or unknown operator.
 * @ingroup FloatExprParse
 */
static double
_calcf(char op, double a, double b)
{
   switch (op)
     {
      case '+':
        a += b;
        return a;

      case '-':
        a -= b;
        return a;

      case '/':
        if (EINA_DBL_NONZERO(b)) a /= b; // Check for non-zero divisor
        else
          ERR("%s:%i divide by zero", file_in, line - 1);
        return a;

      case '*':
        a *= b;
        return a;

      case '%': // Modulo for floats: convert to int, then modulo
        if (EINA_DBL_NONZERO(b)) a = (double)((int)a % (int)b);
        else
          ERR("%s:%i modula by zero", file_in, line - 1);
        return a;

      default:
        ERR("%s:%i unexpected character '%c'", file_in, line - 1, op);
     }
   return a; // Should be unreachable
}
/** @} */ // end of FloatExprParse group


/**
 * @brief Strips whitespace (spaces and tabs) from a string.
 * @param in The input string.
 * @param out The output buffer to store the stripped string.
 * @param size The size of the output buffer.
 * @return 1 on success, 0 if the input string is too long for the output buffer.
 */
static int
strstrip(const char *in, char *out, size_t size)
{
   if ((size - 1) < strlen(in))
     {
        ERR("%s:%i expression is too long", file_in, line - 1);
        return 0;
     }
   /* remove spaces and tabs */
   while (*in)
     {
        if ((0x20 != *in) && (0x09 != *in)) // If not space or tab
          {
             *out = *in;
             out++;
          }
        in++;
     }
   *out = '\0';
   return 1;
}

/**
 * @brief Finds the index of a parameter by its string value.
 * @param str The string value of the parameter to find.
 * @return The index of the first matching parameter in the `params` array,
 *         or -1 if not found.
 */
int
get_param_index(char *str)
{
   int index;
   char *p;

   for (index = 0; index < get_arg_count(); index++)
     {
        p = _parse_param_get(index);
        if (!p) continue;

        if (!strcmp(str, p))
          return index;
     }

   return -1;
}

/**
 * @brief Checks if a parameter at a given index was originally quoted in the input.
 * @param n The index of the parameter in the `params` array.
 * @return Non-zero if the parameter at index `n` was quoted, 0 otherwise.
 */
int
param_had_quote(int n)
{
   return params_quote & (1 << n);
}

