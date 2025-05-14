#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_code_private.h"

/**
 * @file
 * @brief These routines are used for handling the parsing of Elm Code content.
 *
 * This file implements the core parsing logic for Elm_Code, allowing
 * different parsers (e.g., for syntax highlighting, diffs, TODOs) to be
 * registered and applied to code lines and files.
 */

#ifndef ELM_CODE_TEST
EAPI Elm_Code_Parser *ELM_CODE_PARSER_STANDARD_SYNTAX = NULL;
EAPI Elm_Code_Parser *ELM_CODE_PARSER_STANDARD_DIFF = NULL;
EAPI Elm_Code_Parser *ELM_CODE_PARSER_STANDARD_TODO = NULL;
#endif

/**
 * @brief Represents a parser that can be applied to Elm_Code content.
 *
 * Each parser can define functions to process individual lines and/or entire files.
 * It also holds custom data for the parser's use and a flag indicating if it's a
 * standard, pre-defined parser.
 */
struct _Elm_Code_Parser
{
   void (*parse_line)(Elm_Code_Line *line, void *data); /**< Function pointer to parse a single line. Can be NULL. */
   void (*parse_file)(Elm_Code_File *file, void *data); /**< Function pointer to parse an entire file. Can be NULL. */

   void *data; /**< Custom data to be passed to the parser functions. */
   Eina_Bool standard; /**< EINA_TRUE if this is a standard parser (not freed by _elm_code_parser_free), EINA_FALSE otherwise. */
};

/**
 * @internal
 * @brief Applies all registered line parsers to a given Elm_Code_Line.
 *
 * This function iterates through all parsers associated with the Elm_Code
 * instance and calls their `parse_line` function if defined. It clears
 * existing tokens and status before parsing.
 *
 * @param code The Elm_Code instance containing the parsers.
 * @param line The Elm_Code_Line to be parsed.
 */
void
_elm_code_parse_line(Elm_Code *code, Elm_Code_Line *line)
{
   Elm_Code_Parser *parser;
   Eina_List *item;

   elm_code_line_tokens_clear(line);
   elm_code_line_status_clear(line);

   EINA_LIST_FOREACH(code->parsers, item, parser)
     {
        if (parser->parse_line)
          parser->parse_line(line, parser->data);
     }
}

/**
 * @internal
 * @brief Applies all registered file parsers to a given Elm_Code_File.
 *
 * This function iterates through all parsers associated with the Elm_Code
 * instance and calls their `parse_file` function if defined.
 *
 * @param code The Elm_Code instance containing the parsers.
 * @param file The Elm_Code_File to be parsed.
 */
void
_elm_code_parse_file(Elm_Code *code, Elm_Code_File *file)
{
   Elm_Code_Parser *parser;
   Eina_List *item;

   EINA_LIST_FOREACH(code->parsers, item, parser)
     {
        if (parser->parse_file)
          parser->parse_file(file, parser->data);
     }
}

/**
 * @internal
 * @brief Re-parses all lines within a given Elm_Code_File.
 *
 * This is typically used when a global change might affect the parsing
 * of individual lines, such as a change in syntax definition.
 *
 * @param code The Elm_Code instance containing the parsers.
 * @param file The Elm_Code_File whose lines are to be re-parsed.
 */
void
_elm_code_parse_reset_file(Elm_Code *code, Elm_Code_File *file)
{
   Elm_Code_Line *line;
   Eina_List *item;

   EINA_LIST_FOREACH(file->lines, item, line)
    {
       _elm_code_parse_line(code, line);
    }
}

/**
 * @internal
 * @brief Allocates and initializes a new Elm_Code_Parser structure.
 *
 * @param parse_line Function pointer for line-by-line parsing.
 * @param parse_file Function pointer for file-level parsing.
 * @return A newly allocated Elm_Code_Parser, or NULL on failure.
 */
static Elm_Code_Parser *
_elm_code_parser_new(void (*parse_line)(Elm_Code_Line *, void *),
                     void (*parse_file)(Elm_Code_File *, void *))
{
   Elm_Code_Parser *parser;

   parser = calloc(1, sizeof(Elm_Code_Parser));
   if (!parser)
     return NULL;

   parser->parse_line = parse_line;
   parser->parse_file = parse_file;
   parser->standard = EINA_FALSE;

   return parser;
}

/**
 * @brief Adds a custom parser to an Elm_Code instance.
 *
 * This function creates a new parser with the provided line and file parsing
 * functions and associated data, then appends it to the list of parsers
 * for the given Elm_Code object.
 *
 * @param code The Elm_Code instance to add the parser to.
 * @param parse_line A function pointer that will be called for each line.
 *                   It takes an Elm_Code_Line* and the provided `data` as arguments.
 *                   Can be NULL if no line-specific parsing is needed.
 * @param parse_file A function pointer that will be called once for the file.
 *                   It takes an Elm_Code_File* and the provided `data` as arguments.
 *                   Can be NULL if no file-specific parsing is needed.
 * @param data Custom data to be passed to the `parse_line` and `parse_file` functions.
 */
EAPI void
elm_code_parser_add(Elm_Code *code,
                    void (*parse_line)(Elm_Code_Line *, void *),
                    void (*parse_file)(Elm_Code_File *, void *), void *data)
{
   Elm_Code_Parser *parser;

   parser = _elm_code_parser_new(parse_line, parse_file);
   if (!parser)
     return;

   parser->data = data;

   code->parsers = eina_list_append(code->parsers, parser);
}

/**
 * @brief Adds a pre-defined standard parser to an Elm_Code instance.
 *
 * Standard parsers (like ELM_CODE_PARSER_STANDARD_SYNTAX) are managed
 * globally and should not be freed individually. This function marks
 * the parser as standard.
 *
 * @param code The Elm_Code instance to add the parser to.
 * @param parser A pointer to a standard Elm_Code_Parser (e.g., ELM_CODE_PARSER_STANDARD_SYNTAX).
 */
EAPI void
elm_code_parser_standard_add(Elm_Code *code, Elm_Code_Parser *parser)
{
   if (!parser || !code)
     return;

   parser->standard = EINA_TRUE;
   code->parsers = eina_list_append(code->parsers, parser);
}

/**
 * @internal
 * @brief Removes a specified number of leading characters from a line's content.
 *
 * This is used by the diff parser to remove diff markers (e.g., "+ ", "- ").
 * It handles both original content (by adjusting the pointer) and modified content
 * (by reallocating and copying).
 *
 * @param line The Elm_Code_Line to modify.
 * @param count The number of characters to remove from the beginning of the line.
 */
static void
_elm_code_parser_diff_trim_leading(Elm_Code_Line *line, unsigned int count)
{
   char *replace, *old = NULL;

   if (line->modified)
     {
        old = line->modified;
        replace = malloc(sizeof(char) * (line->length - count));

        strncpy(replace, old + count, line->length - count);
        line->modified = replace;
        free(old);
     }
   else
     {
        line->content += count;
     }

   line->length -= count;
}

#define _PARSE_C_SYMBOLS "{}()[]:;*&|!=<->,."
#define _PARSE_C_KEYWORDS {"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", \
  "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", \
  "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", NULL}

/**
 * @internal
 * @brief Parses a single line for syntax highlighting.
 *
 * This function retrieves the appropriate syntax definition based on the
 * file's MIME type and then invokes the syntax-specific line parsing logic.
 *
 * @param line The Elm_Code_Line to parse for syntax.
 * @param data Unused user data (standard parser).
 */
static void
_elm_code_parser_syntax_parse_line(Elm_Code_Line *line, void *data EINA_UNUSED)
{
   Elm_Code_Syntax *syntax;

   syntax = elm_code_syntax_for_mime_get(line->file->mime);
   if (syntax)
     elm_code_syntax_parse_line(syntax, line);
}

/**
 * @internal
 * @brief Parses an entire file for syntax highlighting.
 *
 * This function retrieves the appropriate syntax definition based on the
 * file's MIME type and then invokes the syntax-specific file parsing logic.
 * This might be used for multi-line constructs or global state needed by the syntax.
 *
 * @param file The Elm_Code_File to parse for syntax.
 * @param data Unused user data (standard parser).
 */
static void
_elm_code_parser_syntax_parse_file(Elm_Code_File *file, void *data EINA_UNUSED)
{
   Elm_Code_Syntax *syntax;
   INF("Parse syntax of file with mime \"%s\"", file->mime);

   syntax = elm_code_syntax_for_mime_get(file->mime);
   if (!syntax)
     {
        WRN("Unsupported mime in parser");
     }
   else
     {
        elm_code_syntax_parse_file(syntax, file);
     }
}

/**
 * @internal
 * @brief Parses a single line to identify diff markers.
 *
 * This function checks the beginning of the line for common diff indicators
 * ('d', 'i', 'n', '+', '-') and sets the line's status accordingly.
 * It also trims the diff markers from the line's visible content.
 *
 * @param line The Elm_Code_Line to parse for diff information.
 * @param data Unused user data (standard parser).
 */
static void
_elm_code_parser_diff_parse_line(Elm_Code_Line *line, void *data EINA_UNUSED)
{
   const char *content;
   unsigned int length;

   content = elm_code_line_text_get(line, &length);
   if (length < 1)
     return;

   if (content[0] == 'd' || content[0] == 'i' || content[0] == 'n')
     {
        elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_CHANGED);
        return;
     }

   if (content[0] == '+')
     elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_ADDED);
   else if (content[0] == '-')
     elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_REMOVED);

   _elm_code_parser_diff_trim_leading(line, 1);
}

/**
 * @internal
 * @brief Parses an entire file to identify diff hunks or context.
 *
 * This function iterates through lines in a file, looking for diff markers.
 * It specifically handles context lines within diffs (lines near '+' or '-')
 * by marking them as changed and trimming leading characters (e.g., "--- a/file", "+++ b/file").
 *
 * @param file The Elm_Code_File to parse for diff information.
 * @param data Unused user data (standard parser).
 */
static void
_elm_code_parser_diff_parse_file(Elm_Code_File *file, void *data EINA_UNUSED)
{
   Eina_List *item;
   Elm_Code_Line *line;
   const char *content;
   unsigned int length, offset;

   offset = 0;
   EINA_LIST_FOREACH(file->lines, item, line)
     {
        content = elm_code_line_text_get(line, &length);

        if (length > 0 && (content[0] == 'd' || content[0] == 'i' || content[0] == 'n'))
          {
             offset = 0;
             continue;
          }

        if (offset <= 1 && (content[0] == '+' || content[0] == '-'))
          {
             elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_CHANGED);
             _elm_code_parser_diff_trim_leading(line, 3);
          }

        offset++;
     }
}

/**
 * @internal
 * @brief Parses a single line to identify "TODO" or "FIXME" comments.
 *
 * If these keywords are found, the line's status is set to
 * ELM_CODE_STATUS_TYPE_TODO.
 *
 * @param line The Elm_Code_Line to scan for TODO/FIXME markers.
 * @param data Unused user data (standard parser).
 */
static void
_elm_code_parser_todo_parse_line(Elm_Code_Line *line, void *data EINA_UNUSED)
{
   if (elm_code_line_text_strpos(line, "TODO", 0) != ELM_CODE_TEXT_NOT_FOUND)
     elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_TODO);
   else if (elm_code_line_text_strpos(line, "FIXME", 0) != ELM_CODE_TEXT_NOT_FOUND)
     elm_code_line_status_set(line, ELM_CODE_STATUS_TYPE_TODO);
}

/**
 * @internal
 * @brief Frees an Elm_Code_Parser structure.
 *
 * This function only frees parsers that are not marked as 'standard'.
 * Standard parsers are managed globally.
 *
 * @param parser The Elm_Code_Parser to potentially free.
 */
void
_elm_code_parser_free(Elm_Code_Parser *parser)
{
   if (parser->standard)
     return;

   free(parser);
}

/**
 * @internal
 * @brief Initializes the standard, globally available parsers.
 *
 * This function creates instances of the syntax, diff, and TODO parsers
 * and assigns them to the global ELM_CODE_PARSER_STANDARD_* pointers.
 * This is typically called once during Elm_Code initialization.
 */
void
_elm_code_parse_setup()
{
   ELM_CODE_PARSER_STANDARD_SYNTAX = _elm_code_parser_new(_elm_code_parser_syntax_parse_line,
                                                          _elm_code_parser_syntax_parse_file);
   ELM_CODE_PARSER_STANDARD_DIFF = _elm_code_parser_new(_elm_code_parser_diff_parse_line,
                                                        _elm_code_parser_diff_parse_file);
   ELM_CODE_PARSER_STANDARD_TODO = _elm_code_parser_new(_elm_code_parser_todo_parse_line, NULL);
}
