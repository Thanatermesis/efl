#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "regex.h"
#include "Elementary.h"

#include "elm_code_private.h"

/**
 * @brief Checks if a line starts with a C-like keyword that typically affects indentation.
 * @param line The code line to check.
 * @return EINA_TRUE if the line starts with a relevant keyword, EINA_FALSE otherwise.
 *
 * This function uses a regular expression to identify keywords like `if`, `else if`,
 * `while`, `for`, `switch`, `else`, `do`, `case`, or `default` at the beginning of
 * the line, possibly preceded by whitespace and followed by an opening parenthesis
 * or brace.
 */
static Eina_Bool
elm_code_line_indent_startswith_keyword(Elm_Code_Line *line)
{
   regex_t regex;
   char *text;
   int ret;
   unsigned int textlen;

   text = (char *)elm_code_line_text_get(line, &textlen);
   text = eina_strndup(text, textlen);

   ret = regcomp(&regex, "^\\s*("
                         "((if|else\\s*if|while|for|switch)\\s*\\(.*\\)\\s*\\{?)|"
                         "((else|do)\\s*\\{?)|"
                         "(case\\s+.+:)|"
                         "(default:)"
                         ")\\s*$", REG_EXTENDED | REG_NOSUB);
   if (ret == 0)
     ret = regexec(&regex, text, 0, NULL, 0);

   regfree(&regex);
   free(text);

   return ret == 0;
}

/**
 * @brief Calculates and returns the appropriate indentation string for a given line.
 * @param line The code line for which to calculate indentation.
 * @return A newly allocated string containing the indentation (e.g., "   ", "\t\t").
 *         The caller is responsible for freeing this string. Returns an empty string
 *         for the first line or if no specific indentation rules apply.
 *
 * This function determines the indentation for the current `line` based on the content
 * of the `prevline` (the line immediately preceding it). It considers:
 * - The indentation of the `prevline`.
 * - Whether the `prevline` ends with an opening brace '{'.
 * - Whether the `prevline` starts with a keyword that increases indentation (e.g., `if`, `for`).
 * - Special handling for EFL indentation style (uses spaces instead of tabs and adds extra
 *   indentation for keywords).
 * - Comment-specific indentation adjustments (e.g., aligning `*` in multi-line comments).
 * - Adjusting indentation when a line follows a closing brace '}'.
 */
EAPI char *
elm_code_line_indent_get(Elm_Code_Line *line)
{
   Elm_Code_Line *prevline;
   const char *prevtext;
   unsigned int prevlength, count = 0;
   char *buf, *ptr;
   char next, last;
   const char *indent = "\t";
   Eina_Bool eflindent = ((Elm_Code *)line->file->parent)->config.indent_style_efl;

   if (line->number <= 1)
     return strdup("");

   prevline = elm_code_file_line_get(line->file, line->number - 1);
   prevtext = elm_code_line_text_get(prevline, &prevlength);

   ptr = (char *)prevtext;
   buf = malloc((prevlength + 5) * sizeof(char));
   while (count < prevlength)
     {
        if (!_elm_code_text_char_is_whitespace(*ptr))
          break;

        count++;
        ptr++;
     }

   strncpy(buf, prevtext, count);
   buf[count] = '\0';

   if (eflindent)
     {
        indent = "   ";
        if (elm_code_line_indent_startswith_keyword(prevline))
          {
             strcpy(buf + count, "  ");
             count += 2;
          }
     }

   if (count < prevlength)
     {
        next = *ptr;
        last = prevtext[prevlength - 1];

        // comment handling
        // TODO this should all be based on comment SCOPE not text matching
        if (next == '/')
          {
             if (count == prevlength - 1)
               return buf;

             if (*(ptr+1) == '/')
               strcpy(buf + count, "//");
             else if (*(ptr+1) == '*')
               strcpy(buf + count, " * ");
          }
        else if (next == '*')
          {
             if (count < prevlength - 1 && *(ptr+1) == ' ')
               strcpy(buf + count, "* ");
             else if (count < prevlength - 1 && *(ptr+1) == '/')
               {
                  if (count >= 1)
                    buf[count-1] = '\0';
               }
             else
               strcpy(buf + count, "*");
          }
        // Simple handling of braces
        else if (last == '{' || (!eflindent && elm_code_line_indent_startswith_keyword(prevline)))
          {
             strcpy(buf + count, indent);
          }
        else if (last == '}')
          {
             unsigned int offset = strlen(indent) - 1;
             if (count >= offset)
               buf[count-offset] = '\0';
          }
     }
   return buf;
}

/**
 * @brief Finds the indentation of the line containing the opening brace that matches
 *        a closing brace on or before the current line's scope.
 * @param line The current code line, used as a starting point for searching upwards.
 * @param length Pointer to an unsigned int where the length of the indentation string
 *               will be stored.
 * @return A pointer to the beginning of the indentation characters in the source line's text,
 *         or an empty string if no matching opening brace is found or if the line with
 *         the opening brace has no indentation. The returned pointer is valid as long as
 *         the underlying Elm_Code_Line text is valid.
 *
 * This function searches upwards from the line preceding the given `line` to find
 * a matching opening brace '{'. It keeps a stack count, decrementing for '{' and
 * incrementing for '}'. When the stack becomes negative, it means an unmatched
 * opening brace has been found. The function then returns the leading whitespace
 * (indentation) of that line.
 */
EAPI const char *
elm_code_line_indent_matching_braces_get(Elm_Code_Line *line, unsigned int *length)
{
   Elm_Code_File *file;
   int stack, row;
   unsigned int len_tmp, count = 0;
   const char *content, *ptr;

   file = line->file;
   stack = 0;
   row = line->number - 1;
   *length = 0;

   while (row > 0)
     {
        line = elm_code_file_line_get(file, row);
        content = elm_code_line_text_get(line, &len_tmp);

        if (memchr(content, '{', len_tmp)) stack--;
        else if (memchr(content, '}', len_tmp)) stack++;

        if (stack < 0)
          {
             if (len_tmp == 0)
               return "";

             ptr = content;
             while (count < len_tmp)
               {
                  if (!_elm_code_text_char_is_whitespace(*ptr))
                    break;

                  count++;
                  ptr++;
               }

             *length = count;
             return content;
          }
        row--;
     }
   return "";
}
