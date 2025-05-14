#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
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
#include <regex.h>

#include "elm_prefs_cc.h"
#include <Ecore.h>
#include <Ecore_File.h>

#ifdef _WIN32
# define EPP_EXT ".exe"
#else
# define EPP_EXT
#endif

/**
 * @brief Handles the creation of a new object based on the current parsing stack.
 *
 * Looks up the object type from the stack identifier in a hash table and
 * calls the corresponding handler function. If no object handler is found,
 * it checks for a statement handler. If neither is found, it reports an error
 * and exits.
 */
static void        new_object(void);

/**
 * @brief Handles a new statement based on the current parsing stack.
 *
 * Looks up the statement type from the stack identifier in a hash table and
 * calls the corresponding handler function. If no handler is found, it reports
 * an error and exits.
 */
static void        new_statement(void);

/**
 * @brief Performs a mathematical calculation on the input string.
 * @param input The string containing the mathematical expression.
 * @return A newly allocated string representing the result of the calculation.
 *         The caller is responsible for freeing this string.
 * @note FIXME: Currently always applies floating-point arithmetic, which might
 *       be problematic for integer parameters.
 */
static char       *perform_math(char *input);

/**
 * @brief Checks if a character is a delimiter.
 * @param c The character to check.
 * @return 1 if the character is a delimiter ({},;:), 0 otherwise.
 */
static int         isdelim(char c);

/**
 * @brief Extracts the next token from the input buffer.
 *
 * This function handles various parsing complexities including comments (C-style and C++-style),
 * quoted strings, parentheses, and escape sequences.
 *
 * @param p Pointer to the current position in the input buffer.
 * @param end Pointer to the end of the input buffer.
 * @param[out] new_p Pointer to be updated to the position after the extracted token.
 * @param[out] delim Pointer to an integer that will be set to 1 if the token is a delimiter, 0 otherwise.
 * @return A newly allocated string containing the token, or NULL if no more tokens are found or an error occurs.
 *         The caller is responsible for freeing this string.
 */
static char       *next_token(char *p, char *end, char **new_p, int *delim);

/**
 * @brief Gets the current identifier from the parsing stack.
 * @return A string representing the current stack path (e.g., "object.property.subproperty").
 *         The returned string is owned by an internal buffer and should not be freed or modified.
 */
static const char *stack_id(void);

/**
 * @brief Parses the input data buffer.
 * @param data The character buffer containing the data to parse.
 * @param size The size of the data buffer.
 */
static void        parse(char *data, off_t size);

/* simple expression parsing protos */
/**
 * @brief Converts a string to an integer, parsing simple mathematical expressions.
 * @param s The string to convert.
 * @return The integer result of the expression.
 */
static int         my_atoi(const char *s);
/** @brief Parses an 'alpha' production for integer expressions (addition/subtraction). */
static char       *_alphai(char *s, int *val);
/** @brief Parses a 'beta' production for integer expressions (multiplication/division/modulo). */
static char       *_betai(char *s, int *val);
/** @brief Parses a 'gamma' production for integer expressions (numbers or parenthesized expressions). */
static char       *_gammai(char *s, int *val);
/** @brief Parses a 'delta' production for integer expressions (parenthesized 'alpha' expressions). */
static char       *_deltai(char *s, int *val);
/** @brief Extracts an integer number from a string. */
static char       *_get_numi(char *s, int *val);
/** @brief Checks if a character can be part of an integer number. */
static int         _is_numi(char c);
/** @brief Checks if a character is a high-precedence integer operator (*, /, %). */
static int         _is_op1i(char c);
/** @brief Checks if a character is a low-precedence integer operator (+, -). */
static int         _is_op2i(char c);
/** @brief Performs an integer calculation. */
static int         _calci(char op, int a, int b);

/**
 * @brief Converts a string to a double, parsing simple mathematical expressions.
 * @param s The string to convert.
 * @return The double result of the expression.
 */
static double      my_atof(const char *s);
/** @brief Parses an 'alpha' production for float expressions (addition/subtraction). */
static char       *_alphaf(char *s, double *val);
/** @brief Parses a 'beta' production for float expressions (multiplication/division/modulo). */
static char       *_betaf(char *s, double *val);
/** @brief Parses a 'gamma' production for float expressions (numbers or parenthesized expressions). */
static char       *_gammaf(char *s, double *val);
/** @brief Parses a 'delta' production for float expressions (parenthesized 'alpha' expressions). */
static char       *_deltaf(char *s, double *val);
/** @brief Extracts a floating-point number from a string. */
static char       *_get_numf(char *s, double *val);
/** @brief Checks if a character can be part of a floating-point number. */
static int         _is_numf(char c);
/** @brief Checks if a character is a high-precedence float operator (*, /, %). */
static int         _is_op1f(char c);
/** @brief Checks if a character is a low-precedence float operator (+, -). */
static int         _is_op2f(char c);
/** @brief Performs a floating-point calculation. */
static double      _calcf(char op, double a, double b);

/**
 * @brief Strips whitespace (spaces and tabs) from a string.
 * @param in The input string.
 * @param[out] out The output buffer to store the stripped string.
 * @param size The size of the output buffer.
 * @return 1 on success, 0 if the input string is too long for the output buffer.
 */
static int         strstrip(const char *in, char *out, size_t size);

int line = 0; /**< Current line number being parsed. */
Eina_List *stack = NULL; /**< The parsing stack, stores tokens representing the current hierarchy. */
Eina_List *params = NULL; /**< List of parameters for the current statement. */

static char file_buf[4096]; /**< Buffer to store the current filename being parsed (handles #line directives). */
static int verbatim = 0; /**< Flag indicating if the parser is currently in a verbatim block. */
static int verbatim_line1 = 0; /**< Starting line number of the verbatim block. */
static int verbatim_line2 = 0; /**< Ending line number of the verbatim block. */
static char *verbatim_str = NULL; /**< String content of the verbatim block. */
static Eina_Strbuf *stack_buf = NULL; /**< String buffer to efficiently build the stack identifier. */

/**
 * @brief Prints the current parsing stack to stderr for error reporting.
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
 * @brief Prints the current parameters to stderr for error reporting.
 */
static void
err_show_params(void)
{
   Eina_List *l;
   char *p;

   ERR("PARAMS:");
   EINA_LIST_FOREACH(params, l, p)
     {
        ERR("  %s", p);
     }
}

/**
 * @brief Prints both the parsing stack and current parameters to stderr.
 *        Used for comprehensive error reporting.
 */
static void
err_show(void)
{
   err_show_stack();
   err_show_params();
}

/**
 * @brief Retrieves a parameter from the global `params` list by its index.
 * @param n The 0-based index of the parameter to retrieve.
 * @return The parameter string if found, NULL otherwise.
 *         The returned string is owned by the `params` list and should not be freed.
 */
static char *
_parse_param_get(int n)
{
   if (n < (int) eina_list_count(params))
     return eina_list_nth(params, n);
   return NULL;
}

static Eina_Hash *_new_object_hash = NULL; /**< Hash table for New_Object_Handler lookup. Keys are object type strings. */
static Eina_Hash *_new_statement_hash = NULL; /**< Hash table for New_Statement_Handler lookup. Keys are statement type strings. */

/**
 * @brief Populates the hash tables for object and statement handlers.
 *
 * This function initializes `_new_object_hash` and `_new_statement_hash`
 * with handlers defined in `object_handlers` and `statement_handlers` arrays.
 * It ensures that this initialization happens only once.
 */
static void
fill_object_statement_hashes(void)
{
   int i, n;

   if (_new_object_hash) return;

   _new_object_hash = eina_hash_string_superfast_new(NULL);
   _new_statement_hash = eina_hash_string_superfast_new(NULL);

   n = object_handler_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_object_hash, object_handlers[i].type,
                             &(object_handlers[i]));
     }
   n = statement_handler_num();
   for (i = 0; i < n; i++)
     {
        eina_hash_direct_add(_new_statement_hash, statement_handlers[i].type,
                             &(statement_handlers[i]));
     }
}

static void
new_object(void)
{
   const char *id;
   New_Object_Handler *oh;
   New_Statement_Handler *sh;

   fill_object_statement_hashes();
   id = stack_id();
   oh = eina_hash_find(_new_object_hash, id);
   if (oh)
     {
        if (oh->func) oh->func();
     }
   else
     {
        sh = eina_hash_find(_new_statement_hash, id);
        if (!sh)
          {
             ERR("%s:%i unhandled keyword %s",
                 file_in, line - 1,
                 (char *)eina_list_data_get(eina_list_last(stack)));
             err_show();
             exit(-1);
          }
     }
}

static void
new_statement(void)
{
   const char *id;
   New_Statement_Handler *sh;

   fill_object_statement_hashes();
   id = stack_id();
   sh = eina_hash_find(_new_statement_hash, id);
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

// Documentation for isdelim already added above.

static int
isdelim(char c)
{
   const char *delims = "{},;:";
   char *d;

   d = (char *)delims;
   while (*d)
     {
        if (c == *d) return 1;
        d++;
     }
   return 0;
}

// Documentation for next_token already added above.

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
   int had_quote = 0;
   int is_escaped = 0;

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
             if (!tmpstr)
               {
                  ERR("%s:%i malloc %i bytes failed",
                      file_in, line - 1, l + 1);
                  exit(-1);
               }
             memcpy(tmpstr, p, l);
             tmpstr[l] = 0;
             if (l >= (int)sizeof(fl))
               {
                  ERR("Line too long: %i chars: %s", l, tmpstr);
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
                  else if (in_parens)
                    {
                       if (((*p) == ')') && (!is_escaped))
                         in_parens--;
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
 * @brief Pushes a token onto the parsing stack and updates the stack identifier string.
 * @param token The token string to push. The function takes ownership of this string.
 */
static void
stack_push(char *token)
{
   if (stack) eina_strbuf_append(stack_buf, ".");
   eina_strbuf_append(stack_buf, token);

   stack = eina_list_append(stack, token);
}

/**
 * @brief Pops the top token from the parsing stack and updates the stack identifier string.
 *
 * Frees the popped token. Reports an error and exits if the stack is empty.
 */
static void
stack_pop(void)
{
   char *top;
   int top_length;

   if (!stack)
     {
        ERR("parse error %s:%i. } marker without matching { marker",
            file_in, line - 1);

        err_show();
        exit(-1);
     }

   top = eina_list_data_get(eina_list_last(stack));
   top_length = strlen(top);

   stack = eina_list_remove_list(stack, eina_list_last(stack));

   if (eina_list_count(stack)) top_length++;  // remove '.' as well.

   eina_strbuf_remove(stack_buf,
                      eina_strbuf_length_get(stack_buf) - top_length,
                      eina_strbuf_length_get(stack_buf));

   free(top);
}

// Documentation for stack_id already added above.

static const char *
stack_id(void)
{
   return eina_strbuf_string_get(stack_buf);
}

// Documentation for parse already added above.

static void
parse(char *data, off_t size)
{
   char *p, *end, *token;
   int delim = 0;
   int do_params = 0;

   DBG("Parsing input file");

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
             if (*token == ',' || *token == ':') do_params = 1;
             else if (*token == '}')
               {
                  if (do_params)
                    {
                       ERR("Parse error %s:%i. } marker before ; marker",
                           file_in, line - 1);
                       err_show();
                       exit(-1);
                    }
                  else
                    stack_pop();
               }
             else if (*token == ';')
               {
                  if (do_params)
                    {
                       do_params = 0;
                       new_statement();
                       /* clear out params */
                       while (params)
                         {
                            free(eina_list_data_get(params));
                            params = eina_list_remove(params, eina_list_data_get(params));
                         }
                       /* remove top from stack */
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
             free(token);
          }
        else
          {
             if (do_params)
               params = eina_list_append(params, token);
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

   DBG("Parsing done");
}

/**
 * @brief Sets the details for a verbatim block encountered during parsing.
 * @param s The string content of the verbatim block. The function takes ownership of this string.
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
 * @brief Compiles the input file specified by the global `file_in`.
 *
 * This function opens the input file, reads its content, and then calls
 * the `parse` function to process it. It handles file I/O errors.
 */
void
compile(void)
{
   int fd;
   off_t size;
   char *data;

   fd = open(file_in, O_RDONLY | O_BINARY, S_IRUSR | S_IWUSR);
   if (fd < 0)
     {
        ERR("Cannot open file \"%s\" for input. %s",
            file_in, strerror(errno));
        exit(-1);
     }
   DBG("Opening \"%s\" for input", file_in);

   size = lseek(fd, 0, SEEK_END);
   if (size <= 0)
     {
        ERR("lseek failed");
        close(fd);
        return;
     }
   lseek(fd, 0, SEEK_SET);
   data = malloc(size);
   if (data && (read(fd, data, size) == size))
     {
        stack_buf = eina_strbuf_new();
        parse(data, size);
        eina_strbuf_free(stack_buf);
        stack_buf = NULL;
     }
   else
     {
        ERR("Cannot read file \"%s\". %s", file_in, strerror(errno));
        exit(-1);
     }
   free(data);
   close(fd);
}

/**
 * @brief Parses a string parameter from the current statement's parameter list.
 * @param n The 0-based index of the string parameter to retrieve.
 * @return A newly allocated copy of the parameter string. The caller is responsible for freeing this string.
 *         Exits with an error if the parameter is not found.
 * @par Example:
 * If params list is ["string1", "another_string"], parse_str(0) returns a copy of "string1".
 */
char *
parse_str(int n)
{
   char *str;
   char *s;

   str = eina_list_nth(params, n);
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
 * @internal
 * @brief Internal helper function to parse an enumerated type from a string.
 *
 * This function iterates through a variable argument list of string-value pairs
 * to find a match for the input string.
 *
 * @param str The string token to match against enum names.
 * @param va A `va_list` containing pairs of (const char *enum_name, int enum_value),
 *           terminated by a NULL enum_name.
 * @return The integer value corresponding to the matched enum string.
 *         Exits with an error if no match is found.
 * @par Example `va` structure:
 *   "option1", VALUE_OPTION1, "option2", VALUE_OPTION2, NULL
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
        if (!strcmp(s, str))
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
 * @brief Parses an enumerated type parameter from the current statement's parameter list.
 *
 * Retrieves the n-th parameter as a string and then matches it against a
 * variable list of expected enum string-value pairs.
 *
 * @param n The 0-based index of the enum parameter in the `params` list.
 * @param ... A variable argument list of C string and int pairs, representing
 *            the possible enum names and their corresponding integer values.
 *            The list must be terminated with a NULL string.
 * @return The integer value of the matched enum.
 *         Exits with an error if the parameter is not found or does not match any enum values.
 * @par Example:
 *   `parse_enum(0, "ENABLE", 1, "DISABLE", 0, "AUTO", -1, NULL);`
 *   If the first parameter is "ENABLE", this returns 1.
 */
int
parse_enum(int n, ...)
{
   char *str;
   int result;
   va_list va;

   str = eina_list_nth(params, n);
   if (!str)
     {
        ERR("%s:%i no parameter supplied as argument %i",
            file_in, line - 1, n + 1);
        err_show();
        exit(-1);
     }

   va_start(va, n);
   result = _parse_enum(str, va);
   va_end(va);

   return result;
}

/**
 * @brief Parses an integer parameter from the current statement's parameter list.
 * @param n The 0-based index of the integer parameter.
 * @return The parsed integer value.
 *         Exits with an error if the parameter is not found or cannot be parsed as an integer.
 * @par Example:
 * If params list is ["123", "456"], parse_int(0) returns 123.
 */
int
parse_int(int n)
{
   char *str;
   int i;

   str = eina_list_nth(params, n);
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
 * @brief Parses an integer parameter from the current statement's parameter list,
 *        ensuring it falls within a specified range.
 * @param n The 0-based index of the integer parameter.
 * @param f The minimum allowed value (inclusive).
 * @param t The maximum allowed value (inclusive).
 * @return The parsed integer value if it's within the range [f, t].
 *         Exits with an error if the parameter is not found, cannot be parsed, or is out of range.
 */
int
parse_int_range(int n, int f, int t)
{
   char *str;
   int i;

   str = eina_list_nth(params, n);
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
 * @brief Parses a boolean parameter from the current statement's parameter list.
 *
 * Recognizes "true", "on" (case-insensitive) as true (1), and
 * "false", "off" (case-insensitive) as false (0). Also accepts integer
 * values 0 or 1.
 *
 * @param n The 0-based index of the boolean parameter.
 * @return 1 for true, 0 for false.
 *         Exits with an error if the parameter is not found or is not a valid boolean representation.
 */
int
parse_bool(int n)
{
   char *str, buf[4096];
   int i;

   str = eina_list_nth(params, n);
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
 * @brief Parses a floating-point parameter from the current statement's parameter list.
 * @param n The 0-based index of the float parameter.
 * @return The parsed double value.
 *         Exits with an error if the parameter is not found or cannot be parsed as a float.
 */
double
parse_float(int n)
{
   char *str;
   double i;

   str = eina_list_nth(params, n);
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
 * @brief Gets the number of parameters for the current statement.
 * @return The count of parameters in the `params` list.
 */
int
get_arg_count(void)
{
   return eina_list_count(params);
}

/**
 * @brief Checks if the current number of parameters matches an exact required count.
 * @param required_args The exact number of arguments expected.
 *        Exits with an error if the count does not match.
 */
void
check_arg_count(int required_args)
{
   int num_args = eina_list_count(params);

   if (num_args != required_args)
     {
        ERR("%s:%i got %i arguments, but expected %i",
            file_in, line - 1, num_args, required_args);
        err_show();
        exit(-1);
     }
}

/**
 * @brief Checks if the current number of parameters meets a minimum required count.
 * @param min_required_args The minimum number of arguments expected (inclusive).
 *        Exits with an error if the count is less than the minimum.
 */
void
check_min_arg_count(int min_required_args)
{
   int num_args = eina_list_count(params);

   if (num_args < min_required_args)
     {
        ERR("%s:%i got %i arguments, but expected at least %i",
            file_in, line - 1, num_args, min_required_args);
        err_show();
        exit(-1);
     }
}

/**
 * @brief Validates a regular expression string by attempting to compile it.
 * @param regex The regular expression string to validate.
 *        Exits with an error if the regex is invalid.
 */
void
check_regex(const char *regex)
{
   int ret;
   char errbuf[1024];
   regex_t preg;

   ret = regcomp(&preg, regex, REG_EXTENDED | REG_NOSUB);
   if (ret)
     {
        regerror(ret, &preg, errbuf, 1024);
        ERR("%s:%i Invalid regular expression:\n"
            "%s", file_in, line, errbuf);
        err_show();
        exit(-1);
     }

   regfree(&preg);
}

/* simple expression parsing stuff */

/*
 * The following functions implement a simple recursive descent parser for
 * mathematical expressions. The grammar is as follows:
 *
 * For integers (i) and floats (f):
 *   alpha  ::= beta { ('+' | '-') beta }*
 *   beta   ::= gamma { ('*' | '/' | '%') gamma }*
 *   gamma  ::= num | '(' alpha ')' | func '(' alpha ')'
 *   func   ::= "floor" | "ceil" (Note: func only implemented for floats in this code,
 *                                and for integers it's partially implemented but might be buggy)
 *   num    ::= an integer or floating point number
 *
 * This means:
 * - `alpha` handles addition and subtraction (lowest precedence).
 * - `beta` handles multiplication, division, and modulo (medium precedence).
 * - `gamma` handles numbers, parenthesized expressions (highest precedence), and function calls.
 * - `delta` is a helper for parenthesized expressions.
 */

/* int set of function */

// Documentation for my_atoi already added above.
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
 * @internal
 * @brief Parses a 'delta' production for integer expressions: `( alpha )`.
 *
 * A 'delta' is a parenthesized 'alpha' expression.
 * Example: `(5 + 3)`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
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
        s++;
        return s;
     }
   return s;
}

/**
 * @internal
 * @brief Parses function calls like `floor(expr)` or `ceil(expr)` for integer expressions.
 *
 * Note: For integer math, floor and ceil on an already integer result of `_deltai`
 * might not change the value as expected if `_deltai` could produce non-integer
 * intermediate results (which it doesn't currently, but the design implies it might have).
 * Currently, this effectively parses `floor((int_expr))` or `ceil((int_expr))`.
 *
 * @param s The input string to parse, expected to start with "floor(" or "ceil(".
 * @param[out] val Pointer to store the result of the parsed function call.
 * @return Pointer to the character in `s` after the parsed function call, or `s` if parsing fails.
 */
static char *
_funci(char *s, int *val)
{
   if (!strncmp(s, "floor(", 6))
     {
        s += 5;
        s = _deltai(s, val);
     }
   else if (!strncmp(s, "ceil(", 5))
     {
        s += 4;
        s = _deltai(s, val);
     }
   else
     {
        ERR("%s:%i unexpected character at %s",
            file_in, line - 1, s);
     }
   return s;
}

/**
 * @internal
 * @brief Parses a 'gamma' production for integer expressions: `num | delta | func`.
 *
 * A 'gamma' is either a number, a parenthesized expression ('delta'), or a function call.
 * Examples: `123`, `(4 * 2)`, `floor(7)`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_gammai(char *s, int *val)
{
   if (!val) return NULL;
   if (_is_numi(s[0]))
     {
        s = _get_numi(s, val);
        return s;
     }
   else if ('(' == s[0])
     {
        s = _deltai(s, val);
        return s;
     }
   else
     {
        s = _funci(s, val);
//        ERR("%s:%i unexpected character at %s",
//                progname, file_in, line - 1, s);
     }
   return s;
}

/**
 * @internal
 * @brief Parses a 'beta' production for integer expressions: `gamma { ('*' | '/' | '%') gamma }*`.
 *
 * A 'beta' handles multiplication, division, and modulo operations.
 * Example: `3 * 4 / 2`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_betai(char *s, int *val)
{
   int a1, a2;
   char op;

   if (!val) return NULL;
   s = _gammai(s, &a1);
   while (_is_op1i(s[0]))
     {
        op = s[0];
        s++;
        s = _gammai(s, &a2);
        a1 = _calci(op, a1, a2);
     }
   (*val) = a1;
   return s;
}

/**
 * @internal
 * @brief Parses an 'alpha' production for integer expressions: `beta { ('+' | '-') beta }*`.
 *
 * An 'alpha' handles addition and subtraction operations.
 * Example: `1 + 2 - 3`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_alphai(char *s, int *val)
{
   int a1, a2;
   char op;

   if (!val) return NULL;
   s = _betai(s, &a1);
   while (_is_op2i(s[0]))
     {
        op = s[0];
        s++;
        s = _betai(s, &a2);
        a1 = _calci(op, a1, a2);
     }
   (*val) = a1;
   return s;
}

/**
 * @internal
 * @brief Extracts an integer number from the beginning of a string.
 *
 * Handles positive and negative integers.
 *
 * @param s The input string, expected to start with an integer.
 * @param[out] val Pointer to store the extracted integer.
 * @return Pointer to the character in `s` after the parsed number.
 */
static char *
_get_numi(char *s, int *val)
{
   char buf[4096];
   int pos = 0;

   if (!val) return s;
   while ((('0' <= s[pos]) && ('9' >= s[pos])) ||
          ((0 == pos) && ('-' == s[pos])))
     {
        buf[pos] = s[pos];
        pos++;
     }
   buf[pos] = '\0';
   (*val) = atoi(buf);
   return s + pos;
}

/**
 * @internal
 * @brief Checks if a character can be the start or part of an integer number.
 * @param c The character to check.
 * @return 1 if the character is a digit, '-', or '+', 0 otherwise.
 */
static int
_is_numi(char c)
{
   if (((c >= '0') && (c <= '9')) || ('-' == c) || ('+' == c))
     return 1;
   else
     return 0;
}

/**
 * @internal
 * @brief Checks if a character is a multiplicative operator for integers ('*', '%', '/').
 * These operators have higher precedence.
 * @param c The character to check.
 * @return 1 if it's a multiplicative operator, 0 otherwise.
 */
static int
_is_op1i(char c)
{
   switch (c)
     {
      case '*':;

      case '%':;

      case '/': return 1;

      default: break;
     }
   return 0;
}

/**
 * @internal
 * @brief Checks if a character is an additive operator for integers ('+', '-').
 * These operators have lower precedence.
 * @param c The character to check.
 * @return 1 if it's an additive operator, 0 otherwise.
 */
static int
_is_op2i(char c)
{
   switch (c)
     {
      case '+':;

      case '-': return 1;

      default: break;
     }
   return 0;
}

/**
 * @internal
 * @brief Performs a binary integer arithmetic operation.
 * @param op The operator character ('+', '-', '/', '*', '%').
 * @param a The first operand.
 * @param b The second operand.
 * @return The result of the operation `a op b`. Reports error for division/modulo by zero.
 */
static int
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
   return a;
}

/* float set of functoins */

// Documentation for my_atof already added above.
static double
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
 * @internal
 * @brief Parses a 'delta' production for float expressions: `( alpha )`.
 *
 * A 'delta' is a parenthesized 'alpha' expression.
 * Example: `(5.0 + 3.2)`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
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
        s++;
     }
   return s;
}

/**
 * @internal
 * @brief Parses function calls like `floor(expr)` or `ceil(expr)` for float expressions.
 *
 * Applies `floor()` or `ceil()` to the result of the inner expression.
 *
 * @param s The input string to parse, expected to start with "floor(" or "ceil(".
 * @param[out] val Pointer to store the result of the parsed function call.
 * @return Pointer to the character in `s` after the parsed function call, or `s` if parsing fails.
 */
static char *
_funcf(char *s, double *val)
{
   if (!strncmp(s, "floor(", 6))
     {
        s += 5;
        s = _deltaf(s, val);
        *val = floor(*val);
     }
   else if (!strncmp(s, "ceil(", 5))
     {
        s += 4;
        s = _deltaf(s, val);
        *val = ceil(*val);
     }
   else
     {
        ERR("%s:%i unexpected character at %s", file_in, line - 1, s);
     }
   return s;
}

/**
 * @internal
 * @brief Parses a 'gamma' production for float expressions: `num | delta | func`.
 *
 * A 'gamma' is either a number, a parenthesized expression ('delta'), or a function call.
 * Examples: `123.45`, `(4.1 * 2.0)`, `floor(7.8)`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_gammaf(char *s, double *val)
{
   if (!val) return NULL;

   if (_is_numf(s[0]))
     {
        s = _get_numf(s, val);
        return s;
     }
   else if ('(' == s[0])
     {
        s = _deltaf(s, val);
        return s;
     }
   else
     {
        s = _funcf(s, val);
//        ERR("%s:%i unexpected character at %s",
//                progname, file_in, line - 1, s);
     }
   return s;
}

/**
 * @internal
 * @brief Parses a 'beta' production for float expressions: `gamma { ('*' | '/' | '%') gamma }*`.
 *
 * A 'beta' handles multiplication, division, and modulo operations for floats.
 * Example: `3.0 * 4.0 / 2.0`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_betaf(char *s, double *val)
{
   double a1 = 0, a2 = 0;
   char op;

   if (!val) return NULL;
   s = _gammaf(s, &a1);
   while (_is_op1f(s[0]))
     {
        op = s[0];
        s++;
        s = _gammaf(s, &a2);
        a1 = _calcf(op, a1, a2);
     }
   (*val) = a1;
   return s;
}

/**
 * @internal
 * @brief Parses an 'alpha' production for float expressions: `beta { ('+' | '-') beta }*`.
 *
 * An 'alpha' handles addition and subtraction operations for floats.
 * Example: `1.5 + 2.0 - 3.1`
 *
 * @param s The input string to parse.
 * @param[out] val Pointer to store the result of the parsed expression.
 * @return Pointer to the character in `s` after the parsed expression, or `s` if parsing fails.
 */
static char *
_alphaf(char *s, double *val)
{
   double a1 = 0, a2 = 0;
   char op;

   if (!val) return NULL;
   s = _betaf(s, &a1);
   while (_is_op2f(s[0]))
     {
        op = s[0];
        s++;
        s = _betaf(s, &a2);
        a1 = _calcf(op, a1, a2);
     }
   (*val) = a1;
   return s;
}

/**
 * @internal
 * @brief Extracts a floating-point number from the beginning of a string.
 *
 * Handles positive and negative floats, and numbers with decimal points.
 *
 * @param s The input string, expected to start with a float.
 * @param[out] val Pointer to store the extracted double.
 * @return Pointer to the character in `s` after the parsed number.
 */
static char *
_get_numf(char *s, double *val)
{
   char buf[4096];
   int pos = 0;

   if (!val) return s;

   while ((('0' <= s[pos]) && ('9' >= s[pos])) ||
          ('.' == s[pos]) ||
          ((0 == pos) && ('-' == s[pos])))
     {
        buf[pos] = s[pos];
        pos++;
     }
   buf[pos] = '\0';
   (*val) = eina_convert_strtod_c(buf, NULL);
   return s + pos;
}

/**
 * @brief Checks if a parameter exists at a given index `n` in the `params` list.
 *
 * This is typically used to see if optional parameters are provided.
 *
 * @param n The 0-based index of the parameter to check.
 * @return 1 if the parameter at index `n` exists, 0 otherwise.
 */
int
params_min_check(int n)
{
   char *str;

   str = _parse_param_get(n);
   if (str) return 1;
   return 0;
}

/**
 * @internal
 * @brief Checks if a character can be the start or part of a floating-point number.
 * @param c The character to check.
 * @return 1 if the character is a digit, '-', '.', or '+', 0 otherwise.
 */
static int
_is_numf(char c)
{
   if (((c >= '0') && (c <= '9'))
       || ('-' == c)
       || ('.' == c)
       || ('+' == c))
     return 1;
   return 0;
}

/**
 * @internal
 * @brief Checks if a character is a multiplicative operator for floats ('*', '%', '/').
 * These operators have higher precedence.
 * @param c The character to check.
 * @return 1 if it's a multiplicative operator, 0 otherwise.
 */
static int
_is_op1f(char c)
{
   switch (c)
     {
      case '*':;

      case '%':;

      case '/': return 1;

      default: break;
     }
   return 0;
}

/**
 * @internal
 * @brief Checks if a character is an additive operator for floats ('+', '-').
 * These operators have lower precedence.
 * @param c The character to check.
 * @return 1 if it's an additive operator, 0 otherwise.
 */
static int
_is_op2f(char c)
{
   switch (c)
     {
      case '+':;

      case '-': return 1;

      default: break;
     }
   return 0;
}

/**
 * @internal
 * @brief Performs a binary floating-point arithmetic operation.
 * @param op The operator character ('+', '-', '/', '*', '%').
 * @param a The first operand.
 * @param b The second operand.
 * @return The result of the operation `a op b`. Reports error for division/modulo by zero.
 *         Note: Modulo for floats is performed by casting operands to int.
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
        if (EINA_DBL_NONZERO(b)) a /= b;
        else
          ERR("%s:%i divide by zero", file_in, line - 1);
        return a;

      case '*':
        a *= b;
        return a;

      case '%':
        if (EINA_DBL_NONZERO(b)) a = (double)((int)a % (int)b);
        else
          ERR("%s:%i modula by zero", file_in, line - 1);
        return a;

      default:
        ERR("%s:%i unexpected character '%c'", file_in, line - 1, op);
     }
   return a;
}

// Documentation for strstrip already added above.
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
        if ((0x20 != *in) && (0x09 != *in))
          {
             *out = *in;
             out++;
          }
        in++;
     }
   *out = '\0';
   return 1;
}
