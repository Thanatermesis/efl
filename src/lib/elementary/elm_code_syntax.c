#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Eina.h>
#include "Elementary.h"

#include "elm_code_private.h"

/**
 * @brief Defines the syntax for a programming language to be used for highlighting.
 */
typedef struct _Elm_Code_Syntax
{
   const char *symbols;        /**< Characters that act as symbols or operators. */
   const char *numparts;       /**< Characters that can be part of a number, besides digits. e.g. ".". */
   const char *preprocessor;   /**< The string that starts a preprocessor directive. e.g. "#". */
   const char *comment_single; /**< The string that starts a single-line comment. e.g. "//". */
   const char *comment_start;  /**< The string that starts a multi-line comment. e.g. "/*". */
   const char *comment_end;    /**< The string that ends a multi-line comment. e.g. "asterisk + /". */
   int (*scope_change)(Elm_Code_Line *line); /**< Function to calculate scope change (e.g., due to braces). */
   const char *keywords[];     /**< A NULL-terminated array of keyword strings. */
} Elm_Code_Syntax;

/**
 * @brief Calculate the change in scope for a line based on braces.
 *
 * This function counts the number of opening and closing braces ('{' and '}')
 * in a line to determine the change in scope. Each '{' increments the scope
 * and each '}' decrements it.
 *
 * @param line The line to analyze.
 * @return The change in scope (positive for increase, negative for decrease).
 */
static int
_elm_code_syntax_scope_change_braces(Elm_Code_Line *line)
{
   unsigned int length, i;
   const char *content;
   int change = 0;

   content = elm_code_line_text_get(line, &length);

   for (i = 0; i < length; i++)
     {
        if (*(content + i) == '{')
          change++;
        else if (*(content + i) == '}')
          change--;
     }

   return change;
}

static Elm_Code_Syntax _elm_code_syntax_c =
{
   "{}()[]:;%^/*+&|~!=<->,.",
   ".",
   "#",
   "//",
   "/*",
   "*/",
   _elm_code_syntax_scope_change_braces,
   {"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",  "enum", "extern", \
      "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", "static", \
      "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", NULL}
};

static Elm_Code_Syntax _elm_code_syntax_rust =
{
   "-*!&+/%|^<=>:;.,{}()[]",
   "._",
   "#",
   "//",
   NULL,
   NULL,
   _elm_code_syntax_scope_change_braces,
   {"as", "break", "const", "continue", "create", "else",  "enum", "extern", "false", "fn", "for", "if", \
      "impl", "in", "let", "loop", "match", "mod", "move", "mut", "pub", "ref", "return", "Self", "self", \
      "static", "struct", "super", "trait", "true", "type", "unsafe", "use", "where", "while",
      "bool", "i8", "i16", "i32", "i64", "isize", "u8", "u16", "u32", "u64", "usize", "f32", "f64", "char", NULL}
};

static Elm_Code_Syntax _elm_code_syntax_py =
{
   "{}()[]:;%/*+!=<->,.",
   ".",
   NULL,
   "#",
   "\"\"\"",
   "\"\"\"",
   NULL,
   {"False", "None", "True", "and", "as", "assert", "break", "class", \
    "continue", "def", "del", "elif", "else", "except", "finally", "for", \
    "from", "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", \
    "or", "pass", "raise", "return", "try", "while", "with", "yield"}
};

static Elm_Code_Syntax _elm_code_syntax_eo =
{
   "{}():;*,.",
   ".",
   NULL,
   "//",
   "[[",
   "]]",
   _elm_code_syntax_scope_change_braces,
   {"byte", "ubyte", "char", "short", "ushort", "int", "uint", "long", "ulong", \
    "llong", "ullong", "int8", "uint8", "int16", "uint16", "int32", "uint32", \
    "int64", "uint64", "int128", "uint128", "size", "ssize", "intptr", "uintptr", \
    "ptrdiff", "time", "float", "double", "bool", "void", "void_ptr", \
    "string", "stringshare", "any_value", \
    "abstract", "class", "data", "mixin", "import", "interface", "type", "const", "var", \
    "own", "free", "struct", "enum", "@extern", "@free", "@auto", "@empty", \
    "@private", "@protected", "@beta", "@hot", "@const", "@class", "@pure_virtual", \
    "@property", "@nullable", "@optional", "@in", "@out", "@inout", "@no_unused", \
    "c_prefix", "methods", "events", "params", "return", \
    "implements", "constructors", "get", "set", "keys", "values", "true", "false", "null"}
};

static Elm_Code_Syntax _elm_code_syntax_go =
{
   "{}()[]:;%^/*+&|~!=<->,.",
   ".",
   NULL,
   "//",
   "/*",
   "*/",
   _elm_code_syntax_scope_change_braces,
   { "break", "case", "chan", "const", "continue", "default", "defer", "else", "fallthrough", "for", "func", "go", "goto",  \
     "if", "import", "interface", "map", "package", "range", "return", "select", "struct", "switch", "type", "var", \
     "true", "false", "iota", "nil", \
     "int", "int8", "int16", "int32", "int64", "uint", "uint8", "uint16", "uint32", "uint64", "uintptr", "float32", \
     "float64", "complex64", "complex128", "bool", "byte", "rune", "string", "error", "make", "len", "cap", "new", "append", \
     "copy", "close", "delete", "complex", "real", "imag", "panic", "recover", NULL }
};

static Elm_Code_Syntax _elm_code_syntax_md =
{
   "()[]*+-_=#.>!:\\`~|",
   "",
   NULL,
   NULL,
   "<!--",
   "-->",
   NULL,
   {}
};

static Elm_Code_Syntax _elm_code_syntax_csharp =
{
   "{}()[]:;%^/*+&|~!=<->,.",
   ".",
   "#",
   "//",
   "/*",
   "*/",
   _elm_code_syntax_scope_change_braces,
   { "abstract","as","base","bool","break","byte","case","catch","char","checked", \
     "class","const","continue","decimal","default","delegate","do","double","else","enum", \
     "event","explicit","extern","false","finally","fixed","float","for","foreach","goto", \
     "if","implicit","in","int","interface","internal","is","lock","long","namespace", \
     "new","null","object","operator","out","override","params","private","protected","public", \
     "readonly","ref","return","sbyte","sealed","short","sizeof","stackalloc","static","string", \
     "struct","switch","this","throw","true","try","typeof","uint","ulong","unchecked","unsafe", \
     "ushort","using","var","virtual","void","volatile","while","add","alias","async","await", \
     "dynamic","get","global","nameof","partial","remove","set","value","when","where","yield", \
     "ascending","by","descending","equals","from", "group","in","into","join","let","on", \
     "orderby","select","where","unmanaged","var", NULL }
};

static Elm_Code_Syntax _elm_code_syntax_shell =
{
   "{}()[]:;%^/*+&|~!=<->,.",
   "",
   NULL,
   "#",
   NULL,
   NULL,
   _elm_code_syntax_scope_change_braces,
   { "if", "then", "else", "elif", "fi", "case", "esac", "for", "select", "while", "until", "do" \
     "done", "in", "function", "time", "coproc", NULL }
};

/**
 * @brief Lookup a syntax definition from a mime type.
 *
 * @param mime The mime type to be looked up for a matching syntax definition,
 *             e.g. "text/x-csrc".
 * @return A syntax definition, if one is found, or NULL.
 */
EAPI Elm_Code_Syntax *
elm_code_syntax_for_mime_get(const char *mime)
{
   if (!mime) return NULL;

   if (!strcmp("text/x-chdr", mime) || !strcmp("text/x-csrc", mime))
     return &_elm_code_syntax_c;
   if (!strcmp("text/rust", mime))
     return &_elm_code_syntax_rust;
   if (!strcmp("text/x-python", mime) || !strcmp("text/x-python3", mime))
     return &_elm_code_syntax_py;
   if (!strcmp("text/x-eolian", mime))
     return &_elm_code_syntax_eo;
   if (!strcmp("text/markdown", mime))
     return &_elm_code_syntax_md;
   if (!strcmp("text/x-go", mime))
     return &_elm_code_syntax_go;
   if (!strcmp("text/x-csharp", mime))
     return &_elm_code_syntax_csharp;
   if (!strcmp("application/x-shellscript", mime))
     return &_elm_code_syntax_shell;

   return NULL;
}

/**
 * @brief Checks if a character can be part of a number.
 *
 * This includes standard digits '0'-'9' and any additional characters
 * defined in the syntax's `numparts` field (e.g., '.').
 *
 * @param c The character to check.
 * @param syntax The syntax definition to use.
 * @return @c EINA_TRUE if the character is a digit or a number part,
 *         @c EINA_FALSE otherwise.
 */
static Eina_Bool
_char_is_number(char c, Elm_Code_Syntax *syntax)
{
   const char *sym;

   if (isdigit(c))
     return EINA_TRUE;

   for (sym = syntax->numparts; *sym; sym++)
     if (c == *sym)
       return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Parses a token and adds a syntax token to the line if it matches a known type.
 *
 * It checks if the token is a keyword or a number based on the provided syntax.
 * If a match is found, an `Elm_Code_Token` of the appropriate type (KEYWORD or NUMBER)
 * is added to the line.
 *
 * @param syntax The syntax definition.
 * @param line The line to which the token belongs.
 * @param pos The starting position of the token in the line.
 * @param token A pointer to the start of the token string.
 * @param length The length of the token string.
 */
static void
_elm_code_syntax_parse_token(Elm_Code_Syntax *syntax, Elm_Code_Line *line, unsigned int pos, const char *token, unsigned int length)
{
   const char **keyword;
   unsigned int i;

   for (keyword = syntax->keywords; *keyword; keyword++)
     if (strlen(*keyword) == length && !strncmp(token, *keyword, length))
       {
          elm_code_line_token_add(line, pos, pos + length - 1, 1, ELM_CODE_TOKEN_TYPE_KEYWORD);
          return;
       }

   for (i = 0; i < length; i++)
     {
        if (!_char_is_number(token[i], syntax))
          break;
        if (i == length - 1)
          elm_code_line_token_add(line, pos, pos + length - 1, 1, ELM_CODE_TOKEN_TYPE_NUMBER);
     }
}

/**
 * @brief Checks if a given string starts with a specific prefix.
 *
 * @param content The string to check.
 * @param prefix The prefix to look for.
 * @param length The length of the @p content string.
 * @return @c EINA_TRUE if @p content starts with @p prefix, @c EINA_FALSE otherwise.
 *         Returns @c EINA_FALSE if prefix is NULL or longer than content.
 */
static Eina_Bool
_content_starts_with(const char *content, const char *prefix, unsigned int length)
{
   unsigned int i;
   unsigned int prefix_length;

   if (!prefix)
     return EINA_FALSE;
   prefix_length = strlen(prefix);
   if (!content || length < prefix_length)
     return EINA_FALSE;

   for (i = 0; i < prefix_length; i++)
     if (content[i] != prefix[i])
       return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Checks if the content starts with a single-line comment marker.
 */
static Eina_Bool
_starts_single_comment(Elm_Code_Syntax *syntax, const char *content, unsigned int length)
{
   return _content_starts_with(content, syntax->comment_single, length);
}

/**
 * @brief Checks if the content starts with a multi-line comment start marker.
 */
static Eina_Bool
_starts_comment(Elm_Code_Syntax *syntax, const char *content, unsigned int length)
{
   return _content_starts_with(content, syntax->comment_start, length);
}

/**
 * @brief Checks if the content starts with a multi-line comment end marker.
 */
static Eina_Bool
_ends_comment(Elm_Code_Syntax *syntax, const char *content, unsigned int length)
{
   return _content_starts_with(content, syntax->comment_end, length);
}

/**
 * @brief Determines if the previous line contains a token that continues to the current line.
 *
 * This is used for multi-line constructs like comments or preprocessor directives.
 * It inspects the tokens of the previous line for one with the 'continues' flag set.
 *
 * @param line The current line.
 * @return The `Elm_Code_Token_Type` of the continuing token from the previous line,
 *         or `ELM_CODE_TOKEN_TYPE_DEFAULT` if no token continues.
 */
static Elm_Code_Token_Type
_previous_line_continue_type(Elm_Code_Line *line)
{
   Elm_Code_Line *prev;
   Elm_Code_Token *token;
   Eina_List *item;

   if (line->number < 2)
     return ELM_CODE_TOKEN_TYPE_DEFAULT;

   prev = elm_code_file_line_get(line->file, line->number - 1);
   if (!prev || !prev->tokens)
     return ELM_CODE_TOKEN_TYPE_DEFAULT;

   EINA_LIST_FOREACH(prev->tokens, item, token)
     if (token->continues)
       return token->type;

   return ELM_CODE_TOKEN_TYPE_DEFAULT;
}

/**
 * @brief Gets the scope value from the end of the previous line.
 *
 * The scope of the current line is calculated based on the scope of the
 * previous line. This function retrieves that base value.
 *
 * @param line The current line.
 * @return The scope value of the previous line, or 0 if it's the first line.
 */
unsigned int
_previous_line_scope(Elm_Code_Line *line)
{
   Elm_Code_Line *prev;

   if (line->number < 2)
     return 0;

   prev = elm_code_file_line_get(line->file, line->number - 1);
   if (!prev)
     return 0;

   return prev->scope;
}

/**
 * @brief Parses a single line of code, identifying and adding tokens based on the given syntax.
 *
 * This is the core syntax highlighting function. It performs a lexical analysis of the
 * line's content. It handles:
 * - Continuing multi-line comments or preprocessor directives from the previous line.
 * - Preprocessor directives.
 * - Single-line comments.
 * - Multi-line comments.
 * - String literals (double and single quoted).
 * - Symbols (operators, braces, etc.).
 * - Other words, which are then identified as keywords or numbers.
 *
 * It also calculates the line's scope based on the previous line's scope and
 * any scope-changing characters on the current line.
 *
 * @param syntax The syntax definition to use.
 * @param line The line that contains the content to parse and will receive the tokens.
 */
EAPI void
elm_code_syntax_parse_line(Elm_Code_Syntax *syntax, Elm_Code_Line *line)
{
   unsigned int i, i2, count, length;
   const char *content;
   const char *sym, *ptr;
   Elm_Code_Token_Type previous_type;

   EINA_SAFETY_ON_NULL_RETURN(syntax);
   line->scope = _previous_line_scope(line) + _elm_code_syntax_scope_change_braces(line);

   i = 0;
   content = elm_code_line_text_get(line, &length);
   previous_type = _previous_line_continue_type(line);
   if (previous_type == ELM_CODE_TOKEN_TYPE_COMMENT)
     {
        for (i2 = i; i2 < length; i2++)
          if (_ends_comment(syntax, content + i2, length - i2))
             {
                i2 += strlen(syntax->comment_end) - 1;
                break;
             }

        elm_code_line_token_add(line, 0, i2, 1, ELM_CODE_TOKEN_TYPE_COMMENT);
        if (i2 == length)
          {
             Elm_Code_Token *token = eina_list_last_data_get(line->tokens);
             token->continues = EINA_TRUE;
             return;
          }
        i = i2 + 1;
     }
   else if (previous_type == ELM_CODE_TOKEN_TYPE_PREPROCESSOR)
     {
        elm_code_line_token_add(line, 0, length, 1, ELM_CODE_TOKEN_TYPE_PREPROCESSOR);
        if (length >= 1 && content[length-1] == '\\')
          {
             Elm_Code_Token *token = eina_list_last_data_get(line->tokens);
             token->continues = EINA_TRUE;
          }
        return;
     }

   ptr = content;
   count = 0;
   for (; i < length; i++)
     {
        ptr = content + i - count;
        if (_elm_code_text_char_is_whitespace(content[i]))
          {
             if (count)
               _elm_code_syntax_parse_token(syntax, line, ptr-content, ptr, count);

             count = 0;
             continue;
          }

        if (syntax->preprocessor && _content_starts_with(content+i, syntax->preprocessor, strlen(syntax->preprocessor)))
          {
             elm_code_line_token_add(line, i, length - 1, 1, ELM_CODE_TOKEN_TYPE_PREPROCESSOR);
             if (content[length-1] == '\\')
               {
                  Elm_Code_Token *token = eina_list_last_data_get(line->tokens);
                  token->continues = EINA_TRUE;
               }
             return;
          }
        else if (_starts_single_comment(syntax, content + i, length - i))
          {
             elm_code_line_token_add(line, i, length, 1, ELM_CODE_TOKEN_TYPE_COMMENT);
             return;
          }
        else if (_starts_comment(syntax, content + i, length - i))
          {
             for (i2 = i+strlen(syntax->comment_start); i2 < length; i2++)
               if (_ends_comment(syntax, content + i2, length - i2))
                 {
                    i2 += strlen(syntax->comment_end) - 1;
                    break;
                 }

             elm_code_line_token_add(line, i, i2, 1, ELM_CODE_TOKEN_TYPE_COMMENT);
             if (i2 == length)
               {
                  Elm_Code_Token *token = eina_list_last_data_get(line->tokens);
                  token->continues = EINA_TRUE;
                  // TODO reset all below of here
                  return;
               }
             i = i2;
             count = 0;
             continue;
          }
        else if (content[i] == '"')
          {
             unsigned int start = i, end;

             for (i++; i < length && (content[i] != '"' || (content[i-1] == '\\' && content[i-2] != '\\')); i++) {}
             end = i;

             elm_code_line_token_add(line, start, end, 1, ELM_CODE_TOKEN_TYPE_STRING);
             count = 0;
             continue;
          }
        else if (content[i] == '\'')
          {
             unsigned int start = i, end;

             for (i++; i < length && (content[i] != '\'' || (content[i-1] == '\\' && content[i-2] != '\\')); i++) {}
             end = i;

             elm_code_line_token_add(line, start, end, 1, ELM_CODE_TOKEN_TYPE_STRING);
             count = 0;
             continue;
         }

        for (sym = syntax->symbols; *sym; sym++)
          if (content[i] == *sym)
            {
               if (count)
                 _elm_code_syntax_parse_token(syntax, line, ptr-content, ptr, count);

               elm_code_line_token_add(line, i, i, 1, ELM_CODE_TOKEN_TYPE_BRACE);

               count = -1;
               break;
             }

       count++;
   }

   if (count)
     _elm_code_syntax_parse_token(syntax, line, ptr-content, ptr, count);
}

/**
 * @brief Parses an entire file, applying syntax highlighting to each line.
 *
 * @note This function is currently a stub and does not perform any action.
 * The parsing is expected to be done line-by-line via
 * elm_code_syntax_parse_line().
 *
 * @param syntax The syntax definition to use.
 * @param file The file to parse.
 */
EAPI void
elm_code_syntax_parse_file(Elm_Code_Syntax *syntax, Elm_Code_File *file EINA_UNUSED)
{
   EINA_SAFETY_ON_NULL_RETURN(syntax);
}

