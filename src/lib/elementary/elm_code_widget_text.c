#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_code_widget_private.h"

/**
 * @brief Calculates the character width required to display line numbers.
 *
 * This function determines the number of characters needed for the line number
 * display in the gutter. It ensures a minimum width for at least two digits (e.g., up to 99 lines).
 *
 * @param obj The Elm_Code_Widget object (unused).
 * @param pd The private data of the Elm_Code_Widget.
 * @return The character width needed for line numbers. For example, if there are 150 lines,
 *         this returns 3 (for "150"). If 5 lines, returns 2 (for "05" effectively, due to min width).
 */
static int
_elm_code_widget_text_line_number_width_get(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd)
{
   int max;

   max = elm_code_file_lines_get(pd->code->file);

   // leave space for 2 digits minimum
   if (max < 10)
     max = 10;

   return floor(log10(max)) + 1;
}

/**
 * @brief Calculates the total width of the left gutter in characters.
 *
 * The left gutter can contain status icons and line numbers. This function
 * sums the widths of these components.
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @return The total width of the left gutter in characters.
 *         Example: If status icon is 1 char and line numbers need 3 chars, returns 4.
 */
static int
_elm_code_widget_text_left_gutter_width_get(Eo *obj, Elm_Code_Widget_Data *pd)
{
   Elm_Code_Widget *widget;
   int width = 1; // the status icon, for now

   widget = obj;
   if (!widget)
     return width;

   if (pd->show_line_numbers)
     width += _elm_code_widget_text_line_number_width_get(widget, pd);

   return width;
}

/**
 * @brief Retrieves text spanning multiple lines.
 *
 * This function extracts a block of text that starts at a given position
 * on one line and ends at a given position on another line (or the same line
 * if start_line == end_line, though _elm_code_widget_text_single_get is typically used for that).
 * It reconstructs the text including the appropriate newline characters.
 *
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @param start_line The 1-based starting line number.
 * @param start_col The 1-based starting column number on the start_line.
 * @param end_line The 1-based ending line number.
 * @param end_col The 1-based ending column number on the end_line.
 * @return A newly allocated string containing the text from the specified range.
 *         The caller is responsible for freeing this string. Returns NULL on allocation failure.
 *         Example: For lines "Hello\nWorld", start_line=1, start_col=3, end_line=2, end_col=2
 *         would return "llo\nWo".
 */
static char *
_elm_code_widget_text_multi_get(Elm_Code_Widget *widget, Elm_Code_Widget_Data *pd,
                                unsigned int start_line, unsigned int start_col,
                                unsigned int end_line, unsigned int end_col)
{
   Elm_Code_Line *line;
   char *first, *last, *ret, *ptr;
   const char *newline;
   short newline_len;
   int ret_len;
   unsigned int row, start, end;

   newline = elm_code_file_line_ending_chars_get(pd->code->file, &newline_len);

   line = elm_code_file_line_get(pd->code->file, start_line);
   start = elm_code_widget_line_text_position_for_column_get(widget, line, start_col);
   first = elm_code_line_text_substr(line, start, line->length - start);

   line = elm_code_file_line_get(pd->code->file, end_line);
   end = elm_code_widget_line_text_position_for_column_get(widget, line, end_col + 1);
   last = elm_code_line_text_substr(line, 0, end);

   ret_len = strlen(first) + strlen(last) + newline_len;

   for (row = start_line + 1; row < end_line; row++)
     {
        line = elm_code_file_line_get(pd->code->file, row);
        ret_len += line->length + newline_len;
     }

   ret = malloc(sizeof(char) * (ret_len + 1));
   if (!ret) goto end;

   snprintf(ret, strlen(first) + newline_len + 1, "%s%s", first, newline);

   ptr = ret;
   ptr += strlen(first) + newline_len;

   for (row = start_line + 1; row < end_line; row++)
     {
        line = elm_code_file_line_get(pd->code->file, row);
        if (line->length > 0)
          snprintf(ptr, line->length + 1, "%s", elm_code_line_text_get(line, NULL));

        snprintf(ptr + line->length, newline_len + 1, "%s", newline);
        ptr += line->length + newline_len;
     }
   snprintf(ptr, strlen(last) + 1, "%s", last);

end:
   free(first);
   free(last);
   return ret;
}

/**
 * @brief Retrieves text from a single line within a specified column range.
 *
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @param start_line The 1-based line number from which to extract text.
 * @param start_col The 1-based starting column number on the line.
 * @param end_col The 1-based ending column number on the line.
 * @return A newly allocated string containing the text from the specified range.
 *         The caller is responsible for freeing this string.
 *         Example: For line "Hello World", start_line=1, start_col=1, end_col=5
 *         would return "Hello".
 */
static char *
_elm_code_widget_text_single_get(Elm_Code_Widget *widget, Elm_Code_Widget_Data *pd,
                                           unsigned int start_line, unsigned int start_col,
                                           unsigned int end_col)
{
   Elm_Code_Line *line;
   unsigned int start, end;

   line = elm_code_file_line_get(pd->code->file, start_line);
   start = elm_code_widget_line_text_position_for_column_get(widget, line, start_col);
   end = elm_code_widget_line_text_position_for_column_get(widget, line, end_col + 1);

   return elm_code_line_text_substr(line, start, end - start);
}

/**
 * @brief Retrieves text between two specified positions (line and column).
 *
 * This function acts as a dispatcher, calling either
 * `_elm_code_widget_text_single_get` or `_elm_code_widget_text_multi_get`
 * based on whether the start and end lines are the same.
 *
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @param start_line The 1-based starting line number.
 * @param start_col The 1-based starting column number.
 * @param end_line The 1-based ending line number.
 * @param end_col The 1-based ending column number.
 * @return A newly allocated string containing the text from the specified range.
 *         The caller is responsible for freeing this string.
 */
static char *
_elm_code_widget_text_between_positions_get(Eo *widget, Elm_Code_Widget_Data *pd,
                                            unsigned int start_line, unsigned int start_col,
                                            unsigned int end_line, unsigned int end_col)
{
   if (start_line == end_line)
     return _elm_code_widget_text_single_get(widget, pd, start_line, start_col, end_col);
   else
     return _elm_code_widget_text_multi_get(widget, pd, start_line, start_col, end_line, end_col);
}

/**
 * @brief Converts a character position (byte index) in a line to its visual column width.
 *
 * This function accounts for variable-width characters like tabs.
 * For example, if a line starts with "a\tb" and tabstop is 4, the character 'b'
 * is at position 2 (0-indexed byte offset), but its visual column would be 5 (1-indexed).
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget (unused).
 * @param line The Elm_Code_Line object.
 * @param position The character position (byte index) within the line's content.
 * @return The visual column number (1-based) corresponding to the given character position.
 */
static unsigned int
_elm_code_widget_line_text_column_width_to_position(Eo *obj, Elm_Code_Widget_Data *pd EINA_UNUSED, Elm_Code_Line *line, unsigned int position)
{
   Eina_Unicode unicode;
   unsigned int count = 1;
   int index = 0;
   const char *chars;

   if (line->length == 0)
     return 1;

   if (line->modified)
     chars = line->modified;
   else
     chars = line->content;
   if (position > line->length)
     position = line->length;

   while ((unsigned int) index < position)
     {
        unicode = eina_unicode_utf8_next_get(chars, &index);
        if (unicode == 0)
          break;

        if (unicode == '\t')
          count += elm_code_widget_text_tabwidth_at_column_get(obj, count);
        else
          count++;
     }

   return count;
}

/**
 * @brief Calculates the total visual column width of a given line.
 *
 * This takes into account tab characters and their expansion according to the
 * current tabstop settings.
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @param line The Elm_Code_Line object.
 * @return The total visual width of the line in columns. Returns 0 if line is NULL.
 *         Example: If line is "a\tb" and tabstop is 4, with 'a' at col 1,
 *         the tab expands to 3 spaces, so 'b' is at col 5. The total width is 5.
 *         However, the function returns `width - 1` from the perspective of
 *         `_elm_code_widget_line_text_column_width_to_position`, so for "a\tb" (length 3),
 *         it would calculate width up to length, which is 5, then return 5-1 = 4.
 *         This seems to be the number of columns *occupied*, not the column number of the last char.
 */
static unsigned int
_elm_code_widget_line_text_column_width_get(Eo *obj, Elm_Code_Widget_Data *pd, Elm_Code_Line *line)
{
   if (!line)
     return 0;

   return _elm_code_widget_line_text_column_width_to_position(obj, pd, line, line->length) - 1;
}

/**
 * @brief Converts a visual column number to its corresponding character position (byte index) in a line.
 *
 * This function is the inverse of `_elm_code_widget_line_text_column_width_to_position`.
 * It accounts for variable-width characters like tabs.
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget (unused).
 * @param line The Elm_Code_Line object.
 * @param column The visual column number (1-based).
 * @return The character position (0-based byte index) in the line's content
 *         that corresponds to the start of the given visual column.
 *         Example: For line "a\tb" with tabstop 4, column 5 corresponds to character 'b',
 *         which is at byte index 2.
 */
static unsigned int
_elm_code_widget_line_text_position_for_column_get(Eo *obj, Elm_Code_Widget_Data *pd EINA_UNUSED, Elm_Code_Line *line, unsigned int column)
{
   Eina_Unicode unicode;
   unsigned int count = 1, position = 0;
   int index = 0;
   const char *chars;

   if (!line || line->length == 0 || column == 1)
     return 0;

   if (line->modified)
     chars = line->modified;
   else
     chars = line->content;

   while ((unsigned int) count <= column && index <= (int) line->length)
     {
        position = (unsigned int) index;
        if (index < (int) line->length)
          unicode = eina_unicode_utf8_next_get(chars, &index);
        else return line->length;

        if (unicode == 0)
          return line->length;
        else if (unicode == '\t')
          count += elm_code_widget_text_tabwidth_at_column_get(obj, count);
        else
          count++;
     }

   return position;
}

/**
 * @brief Calculates the width of a tab character if it were inserted at a specific column.
 *
 * The width of a tab depends on the `tabstop` setting and the current column.
 * For example, if `tabstop` is 4 and the current `column` is 1, a tab will span 4 columns.
 * If `column` is 3, a tab will span 2 columns to reach the next tab stop at column 5.
 *
 * @param obj The Elm_Code_Widget object (unused).
 * @param pd The private data of the Elm_Code_Widget.
 * @param column The 1-based column number where the tab would start.
 * @return The number of columns the tab character would occupy.
 */
static unsigned int
_elm_code_widget_text_tabwidth_at_column_get(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd, unsigned int column)
{
   return pd->tabstop - ((column - 1) % pd->tabstop);
}

/**
 * @brief Inserts text into a single line at a specified column and row.
 *
 * This function handles text insertion that does not involve newlines.
 * It updates the line content, adjusts the widget's column count if the line
 * becomes longer than any previous line, and moves the cursor to the end
 * of the inserted text.
 *
 * @param widget The Elm_Code_Widget object.
 * @param code The Elm_Code object associated with the widget.
 * @param col The 1-based column number where insertion should begin.
 * @param row The 1-based row number (line number) for insertion.
 * @param text The text string to insert.
 * @param len The length of the text to insert.
 */
static void
_elm_code_widget_text_insert_single(Elm_Code_Widget *widget, Elm_Code *code,
                                    unsigned int col, unsigned int row, const char *text, unsigned int len)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Line *line;
   unsigned int position, newcol;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);
   line = elm_code_file_line_get(code->file, row);
   position = elm_code_widget_line_text_position_for_column_get(widget, line, col);
   elm_code_line_text_insert(line, position, text, len);

   newcol = elm_code_widget_line_text_column_width_to_position(widget, line, position + len);

   // if we are making a line longer than before then we need to resize
   if (newcol > pd->col_count)
     _elm_code_widget_resize(widget, line);

   efl_ui_code_widget_cursor_position_set(widget, row, newcol);
}

/**
 * @brief Inserts text that may span multiple lines.
 *
 * This function handles text insertion containing one or more newline characters.
 * It splits the current line at the insertion point, inserts the first part of
 * the text, then inserts new lines for each newline in the input text, and finally
 * inserts the remaining part of the text on the last new line.
 *
 * @param widget The Elm_Code_Widget object.
 * @param code The Elm_Code object associated with the widget.
 * @param col The 1-based column number where insertion should begin on the initial row.
 * @param row The 1-based row number (line number) where insertion should begin.
 * @param text The text string to insert (may contain newlines).
 * @param len The length of the text to insert.
 */
static void
_elm_code_widget_text_insert_multi(Elm_Code_Widget *widget, Elm_Code *code,
                                   unsigned int col, unsigned int row, const char *text, unsigned int len)
{
   Elm_Code_Line *line;
   unsigned int position, newrow, remain;
   int nlpos;
   short nllen;
   char *ptr;

   line = elm_code_file_line_get(code->file, row);
   position = elm_code_widget_line_text_position_for_column_get(widget, line, col);
   elm_code_line_split_at(line, position);

   newrow = row;
   ptr = (char *)text;
   remain = len;
   while ((nlpos = elm_code_text_newlinenpos(ptr, remain, &nllen)) != ELM_CODE_TEXT_NOT_FOUND)
     {
        if (newrow == row)
          _elm_code_widget_text_insert_single(widget, code, col, row, text, nlpos);
        else
          elm_code_file_line_insert(code->file, newrow, ptr, nlpos, NULL);

        remain -= nlpos + nllen;
        ptr += nlpos + nllen;
        newrow++;
     }

   _elm_code_widget_text_insert_single(widget, code, 1, newrow, ptr, len - (ptr - text));
}

/**
 * @brief Core implementation for inserting text at the current cursor position.
 *
 * This function handles the actual text insertion, optionally managing undo/redo history.
 * It first deletes any selected text if `undo` is true. It then determines if the
 * insertion is single-line or multi-line and calls the appropriate helper.
 * Special handling is included for inserting a closing brace '}' to auto-adjust indentation.
 *
 * @param widget The Elm_Code_Widget object.
 * @param text The text to insert.
 * @param length The length of the text to insert.
 * @param undo If EINA_TRUE, delete selection first and add this operation to the undo stack.
 *             If EINA_FALSE, do not manage undo stack for this operation.
 */
void
_elm_code_widget_text_at_cursor_insert_do(Elm_Code_Widget *widget, const char *text, int length, Eina_Bool undo)
{
   Elm_Code *code;
   Elm_Code_Line *line;
   Elm_Code_Widget_Change_Info *change;
   unsigned int row, col, end_row, end_col, curlen, indent;
   const char *curtext, *indent_text;

   if (undo)
     elm_code_widget_selection_delete(widget);

   code = efl_ui_code_widget_code_get(widget);
   efl_ui_code_widget_cursor_position_get(widget, &row, &col);
   line = elm_code_file_line_get(code->file, row);
   if (line == NULL)
     {
        elm_code_file_line_append(code->file, "", 0, NULL);
        row = elm_code_file_lines_get(code->file);
        line = elm_code_file_line_get(code->file, row);
     }
   if (text[0] == '}')
     {
        curtext = elm_code_line_text_get(line, &curlen);

        if (elm_code_text_is_whitespace(curtext, line->length))
          {
             indent_text = elm_code_line_indent_matching_braces_get(line, &indent);
             elm_code_line_text_leading_whitespace_strip(line);

             if (indent > 0)
               elm_code_line_text_insert(line, 0, indent_text, indent);

             col = elm_code_widget_line_text_column_width_to_position(widget, line, indent + 1);
             efl_ui_code_widget_cursor_position_set(widget, row, col);
          }
     }

   if (elm_code_text_newlinenpos(text, length, NULL) == ELM_CODE_TEXT_NOT_FOUND)
     _elm_code_widget_text_insert_single(widget, code, col, row, text, length);
   else
     _elm_code_widget_text_insert_multi(widget, code, col, row, text, length);
   efl_ui_code_widget_cursor_position_get(widget, &end_row, &end_col);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CHANGED_USER, NULL);

   if (undo)
     {
        change = _elm_code_widget_change_create(col, row, end_col - 1, end_row, text, length, EINA_TRUE);
        _elm_code_widget_undo_change_add(widget, change);
        _elm_code_widget_change_free(change);
     }
}

EOLIAN void
_elm_code_widget_text_at_cursor_insert(Elm_Code_Widget *widget, Elm_Code_Widget_Data *pd EINA_UNUSED, const char *text)
{
   // This is an EOLIAN function, its documentation is typically generated from the .eo file.
   // It serves as a public API wrapper for _elm_code_widget_text_at_cursor_insert_do with undo enabled.
   _elm_code_widget_text_at_cursor_insert_do(widget, text, strlen(text), EINA_TRUE);
}

/**
 * @brief Inserts text at the current cursor position without adding to the undo history.
 *
 * This function is typically used for internal operations or programmatic text changes
 * that should not be undoable by the user.
 *
 * @param widget The Elm_Code_Widget object.
 * @param text The text to insert.
 * @param length The length of the text to insert.
 */
void
_elm_code_widget_text_at_cursor_insert_no_undo(Elm_Code_Widget *widget, const char *text, unsigned int length)
{
   _elm_code_widget_text_at_cursor_insert_do(widget, text, length, EINA_FALSE);
}
