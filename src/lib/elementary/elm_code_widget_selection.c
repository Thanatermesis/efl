#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_code_widget_private.h"

/** @internal
 * @brief Characters that are considered word breaks for selection purposes.
 */
static char _breaking_chars[] = " \t,.?!;:*&()[]{}'\"";

/**
 * @internal
 * @brief Allocates and initializes a new Elm_Code_Widget_Selection_Data structure.
 * @return A pointer to the newly allocated Elm_Code_Widget_Selection_Data, or NULL on failure.
 */
static Elm_Code_Widget_Selection_Data *
_elm_code_widget_selection_new()
{
   Elm_Code_Widget_Selection_Data *data;

   data = calloc(1, sizeof(Elm_Code_Widget_Selection_Data));

   return data;
}

/**
 * @internal
 * @brief Ensures that the given row and column are within the valid boundaries of the document.
 *
 * If the row exceeds the total number of lines, it's set to the last line.
 * If the column exceeds the width of the specified line, it's set to the end of that line.
 *
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 * @param row Pointer to the row number to be validated/adjusted.
 * @param col Pointer to the column number to be validated/adjusted.
 */
static void
_elm_code_widget_selection_limit(Evas_Object *widget EINA_UNUSED, Elm_Code_Widget_Data *pd,
                                 unsigned int *row, unsigned int *col)
{
   Elm_Code_Line *line;
   Elm_Code_File *file;
   unsigned int width;

   file = pd->code->file;

   if (*row > elm_code_file_lines_get(file))
     *row = elm_code_file_lines_get(file);

   line = elm_code_file_line_get(file, *row);
   width = elm_code_widget_line_text_column_width_get(widget, line);

   if (*col > width + 1)
     *col = width + 1;
}

EAPI void
elm_code_widget_selection_start(Evas_Object *widget,
                                unsigned int line, unsigned int col)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   // Ensure the provided line and column are within valid document bounds.
   _elm_code_widget_selection_limit(widget, pd, &line, &col);
   if (!pd->selection)
     {
        // If no selection exists, create a new one.
        selection = _elm_code_widget_selection_new();

        selection->end_line = line;
        selection->end_col = col;

        pd->selection = selection;
     }

   _elm_code_widget_selection_in_progress_set(widget, EINA_TRUE);

   pd->selection->start_line = line;
   pd->selection->start_col = col;
   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_START, widget);
}

EAPI void
elm_code_widget_selection_end(Evas_Object *widget,
                              unsigned int line, unsigned int col)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   // Optimization: If the new end position is the same as the current one, do nothing.
   if (pd->selection && (pd->selection->end_line == line) &&
       (pd->selection->end_col == col)) return;

   // Ensure the provided line and column are within valid document bounds.
   _elm_code_widget_selection_limit(widget, pd, &line, &col);
   if (!pd->selection)
     {
        // If no selection exists, create a new one.
        // This might happen if selection_end is called without a preceding selection_start.
        selection = _elm_code_widget_selection_new();

        selection->start_line = line;
        selection->start_col = col;

        pd->selection = selection;
     }

   pd->selection->end_line = line;
   pd->selection->end_col = col;
   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_CHANGED, widget);
}

EAPI void
elm_code_widget_selection_select_all(Evas_Object *widget)
{
   Elm_Code_Line *last_line;
   unsigned int last_length, last_col;
   Elm_Code_Widget_Data *pd;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   elm_code_widget_selection_start(widget, 1, 1);
   int maxrow = elm_code_file_lines_get(pd->code->file);
   last_line = elm_code_file_line_get(pd->code->file, elm_code_file_lines_get(pd->code->file));
   elm_code_line_text_get(last_line, &last_length);
   last_col = elm_code_widget_line_text_column_width_to_position(widget, last_line, last_length);

   elm_code_widget_selection_end(widget, maxrow, last_col);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_CHANGED, widget);
}

EAPI Elm_Code_Widget_Selection_Data *
elm_code_widget_selection_normalized_get(Evas_Object *widget)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;
   Eina_Bool reverse;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);
   selection = _elm_code_widget_selection_new();

   if (!pd->selection)
     {
        // If no active selection, return a new selection object
        // representing a cursor at the beginning of the document (line 1, col 1).
        selection->start_line = selection->end_line = 1;
        selection->start_col = selection->end_col = 1;

        return selection;
     }

   // Determine if the selection was made in reverse order
   // (e.g., dragging the mouse from bottom-right to top-left).
   if (pd->selection->start_line == pd->selection->end_line)
     reverse = pd->selection->start_col > pd->selection->end_col; // Same line, compare columns
   else
     reverse = pd->selection->start_line > pd->selection->end_line; // Different lines, compare lines

   // "Normalize" the selection: ensure start_line/col is before or at end_line/col.
   if (reverse)
     {
        selection->start_line = pd->selection->end_line;
        selection->start_col = pd->selection->end_col;
        selection->end_line = pd->selection->start_line;
        selection->end_col = pd->selection->start_col;
     }
   else
     {
        selection->start_line = pd->selection->start_line;
        selection->start_col = pd->selection->start_col;
        selection->end_line = pd->selection->end_line;
        selection->end_col = pd->selection->end_col;
     }

   return selection;
}

EAPI void
elm_code_widget_selection_clear(Evas_Object *widget)
{
   Elm_Code_Widget_Data *pd;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd->selection)
     return;

   free(pd->selection);
   pd->selection = NULL;
   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_CLEARED, widget);
}

/**
 * @internal
 * @brief Deletes the selected text when the selection is contained within a single line.
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 */
static void
_elm_code_widget_selection_delete_single(Elm_Code_Widget *widget, Elm_Code_Widget_Data *pd)
{
   Elm_Code_Line *line;
   const char *old;
   unsigned int old_length, start, end, length;
   char *content;
   Elm_Code_Widget_Selection_Data *selection;

   selection = elm_code_widget_selection_normalized_get(widget);
   line = elm_code_file_line_get(pd->code->file, selection->start_line);
   old = elm_code_line_text_get(line, &old_length);
   start = elm_code_widget_line_text_position_for_column_get(widget, line, selection->start_col);
   end = elm_code_widget_line_text_position_for_column_get(widget, line, selection->end_col);

   if (end == line->length)
     {
        length = line->length - (end - start);

        content = malloc(sizeof(char) * length);
        strncpy(content, old, start);
     }
   else
     {
        length = line->length - (end - start + 1);

        content = malloc(sizeof(char) * length);
        strncpy(content, old, start);
        strncpy(content + start, old + end + 1,
                old_length - (end + 1));
      }
   elm_code_line_text_set(line, content, length);
   free(content);
   free(selection);
}

/**
 * @internal
 * @brief Copies a string, ensuring null termination even if source is longer than len.
 *
 * This function copies up to `len - 1` characters from `src` to `dest` and
 * null-terminates `dest`. If `src` is shorter than `len`, the copy is
 * null-terminated at the end of `src`.
 *
 * @param dest The destination buffer.
 * @param src The source string.
 * @param len The maximum number of characters to copy from src (including space for null terminator if src is shorter).
 *            Effectively, it's the size of the dest buffer if you want to prevent overflow.
 *            However, the loop runs `len` times, so if `len` is `sizeof(dest)`, it might write
 *            `dest[len-1]` and then `*p = 0` would be `dest[len]`, which is an overflow.
 *            The logic seems to intend to copy `len` bytes and then null terminate if `*src` became `0`
 *            within those `len` bytes.
 *
 * TODO: Review the len parameter usage carefully. If len is sizeof(dest),
 * the loop `for (p = dest; len > 0; p++, src++, len--)` means `p` can reach `dest + sizeof(dest) -1`.
 * If `*src` is not `0` at that point, `*p = *src` writes to the last byte.
 * Then `if (*src == 0) break;` is checked. If it wasn't `0`, the loop continues (len becomes 0),
 * and the function ends. `dest` might not be null-terminated if `src` filled `len` bytes without a null.
 * The original `strncpy` has a different behavior (it pads with nulls if src is shorter,
 * and doesn't guarantee null termination if src is longer).
 * This function aims to always null-terminate if `*src == 0` is encountered within `len` copies.
 */
static void
my_string_copy_truncate(char *dest, const char *src, size_t len)
{
   char *p;
   for (p = dest; len > 0; p++, src++, len--)
     {
        *p = *src;
        if (*src == 0) break;
     }
}


/**
 * @internal
 * @brief Deletes the selected text when the selection spans multiple lines.
 * @param widget The Elm_Code_Widget object.
 * @param pd The private data of the Elm_Code_Widget.
 */
static void
_elm_code_widget_selection_delete_multi(Elm_Code_Widget *widget, Elm_Code_Widget_Data *pd)
{
   Elm_Code_Line *line;
   const char *first, *last;
   unsigned int last_length, start, length, end, i;
   char *content;
   Elm_Code_Widget_Selection_Data *selection;

   if (pd->selection->end_line == pd->selection->start_line)
     return;

   selection = elm_code_widget_selection_normalized_get(widget);
   line = elm_code_file_line_get(pd->code->file, selection->start_line);
   first = elm_code_line_text_get(line, NULL);
   start = elm_code_widget_line_text_position_for_column_get(widget, line, selection->start_col);

   line = elm_code_file_line_get(pd->code->file, selection->end_line);
   last = elm_code_line_text_get(line, &last_length);
   end = (int)elm_code_widget_line_text_position_for_column_get(widget, line, selection->end_col);

   if (end == line->length)
     length = start + last_length - end;
   else
     length = start + last_length - (end + 1);
   content = malloc(sizeof(char) * length);
   strncpy(content, first, start);
   if (last_length > 0)
     {
        if (end == last_length)
          my_string_copy_truncate(content + start, last + end,
                                  last_length - end);
        else
          my_string_copy_truncate(content + start, last + end + 1,
                                  last_length - (end + 1));
     }

   for (i = line->number; i > selection->start_line; i--)
     elm_code_file_line_remove(pd->code->file, i);

   line = elm_code_file_line_get(pd->code->file, selection->start_line);
   elm_code_line_text_set(line, content, length);
   free(content);
   free(selection);
}

/**
 * @internal
 * @brief Core implementation for deleting selected text.
 *
 * This function handles the actual deletion logic, differentiating between
 * single-line and multi-line selections. It can optionally register the
 * change for undo functionality.
 *
 * @param widget The Elm_Code_Widget object.
 * @param undo If EINA_TRUE, the change is registered for undo.
 */
void
_elm_code_widget_selection_delete_do(Evas_Object *widget, Eina_Bool undo)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;
   unsigned int row, col;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd->selection)
     return;

   if (undo)
     _elm_code_widget_change_selection_add(widget);

   selection = elm_code_widget_selection_normalized_get(widget);

   row = selection->start_line;
   col = selection->start_col;

   if (selection->start_line == selection->end_line)
     _elm_code_widget_selection_delete_single(widget, pd);
   else
     _elm_code_widget_selection_delete_multi(widget, pd);

   free(pd->selection);
   pd->selection = NULL;
   free(selection);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_CLEARED, widget);
   elm_code_widget_cursor_position_set(widget, row, col);
}

EAPI void
elm_code_widget_selection_delete(Evas_Object *widget)
{
   _elm_code_widget_selection_delete_do(widget, EINA_TRUE);
}

/**
 * @internal
 * @brief Deletes the current selection without adding the action to the undo history.
 * @param widget The Elm_Code_Widget object.
 */
void
_elm_code_widget_selection_delete_no_undo(Evas_Object *widget)
{
   _elm_code_widget_selection_delete_do(widget, EINA_FALSE);
}

EAPI void
elm_code_widget_selection_select_line(Evas_Object *widget, unsigned int line)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Line *lineobj;
   unsigned int col;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);
   lineobj = elm_code_file_line_get(pd->code->file, line);

   if (!lineobj)
     return;

   elm_code_widget_selection_start(widget, line, 1);
   col = elm_code_widget_line_text_column_width_to_position(widget, lineobj, lineobj->length);
   elm_code_widget_selection_end(widget, line, col);
}

/**
 * @internal
 * @brief Checks if a given character is a "breaking" character (e.g., space, punctuation).
 *
 * Breaking characters are used to determine word boundaries for operations like
 * "select word".
 *
 * @param chr The character to check.
 * @return EINA_TRUE if the character is a breaking character, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_code_widget_selection_char_breaks(char chr)
{
   unsigned int i;

   // Null character is considered a break.
   if (chr == 0)
     return EINA_TRUE;

   // Check against the predefined list of breaking characters.
   for (i = 0; i < sizeof(_breaking_chars); i++)
     if (chr == _breaking_chars[i])
       return EINA_TRUE;


   return EINA_FALSE;
}

EAPI void
elm_code_widget_selection_select_word(Evas_Object *widget, unsigned int line, unsigned int col)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Line *lineobj;
   unsigned int colpos, length, pos;
   const char *content;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);
   lineobj = elm_code_file_line_get(pd->code->file, line);
   content = elm_code_line_text_get(lineobj, &length);

   // Ensure the initial cursor position is valid.
   _elm_code_widget_selection_limit(widget, pd, &line, &col);
   colpos = elm_code_widget_line_text_position_for_column_get(widget, lineobj, col);

   // Find the start of the word:
   // Move backwards from the cursor position until a breaking character or line start is found.
   pos = colpos;
   while (pos > 0)
     {
        if (_elm_code_widget_selection_char_breaks(content[pos - 1]))
          break;
        pos--;
     }
   elm_code_widget_selection_start(widget, line,
                                   elm_code_widget_line_text_column_width_to_position(widget, lineobj, pos));

   // Find the end of the word:
   // Move forwards from the cursor position until a breaking character or line end is found.
   pos = colpos;
   while (pos < length - 1) // length - 1 because we check content[pos + 1]
     {
        if (_elm_code_widget_selection_char_breaks(content[pos + 1]))
          break;
        pos++;
     }
   elm_code_widget_selection_end(widget, line,
                                 elm_code_widget_line_text_column_width_to_position(widget, lineobj, pos));
}

EAPI char *
elm_code_widget_selection_text_get(Evas_Object *widget)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;
   char *text;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd->selection)
     return strdup("");

   selection = elm_code_widget_selection_normalized_get(widget);

   text = elm_code_widget_text_between_positions_get(widget,
                                                     selection->start_line, selection->start_col,
                                                     selection->end_line, selection->end_col);

   free(selection);
   return text;
}

/**
 * @internal
 * @brief Callback invoked when the application loses ownership of a selection (e.g., clipboard).
 *
 * This is typically used to clear the visual selection in the widget if another
 * application takes over the clipboard.
 *
 * @param data User data, expected to be the Elm_Code_Widget.
 * @param selection The type of selection that was lost (e.g., ELM_SEL_TYPE_CLIPBOARD).
 */
static void
_selection_loss_cb(void *data EINA_UNUSED, Elm_Sel_Type selection EINA_UNUSED)
{
   /* Elm_Code_Widget *widget; */

   /* widget = (Elm_Code_Widget *)data; */
// TODO we need to know whih selection we are clearing!
// For example, if primary selection is lost, we should not clear clipboard selection.
// elm_code_widget_selection_clear(widget);
}

EAPI void
elm_code_widget_selection_cut(Evas_Object *widget)
{
   char *text;

   if (elm_code_widget_selection_is_empty(widget))
     return;

   text = elm_code_widget_selection_text_get(widget);
   elm_cnp_selection_set(widget, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT, text, strlen(text));
   elm_cnp_selection_loss_callback_set(widget, ELM_SEL_TYPE_CLIPBOARD, _selection_loss_cb, widget);
   free(text);

   elm_code_widget_selection_delete(widget);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_CUT, widget);
   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CHANGED_USER, NULL);
}

EAPI void
elm_code_widget_selection_copy(Evas_Object *widget)
{
   char *text;

   if (elm_code_widget_selection_is_empty(widget))
     return;

   text = elm_code_widget_selection_text_get(widget);
   elm_cnp_selection_set(widget, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT, text, strlen(text));
   elm_cnp_selection_loss_callback_set(widget, ELM_SEL_TYPE_CLIPBOARD, _selection_loss_cb, widget);
   free(text);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_COPY, widget);
}

/**
 * @internal
 * @brief Callback invoked when data is received for a paste operation.
 *
 * This function receives the text from the clipboard (or other selection source)
 * and inserts it into the widget at the current cursor position.
 *
 * @param data User data, expected to be the Elm_Code_Widget.
 * @param obj The Evas_Object that owns the selection (unused here).
 * @param ev The selection data event, containing the text to paste.
 * @return EINA_TRUE on successful paste, EINA_FALSE otherwise.
 */
static Eina_Bool
_selection_paste_cb(void *data, Evas_Object *obj EINA_UNUSED, Elm_Selection_Data *ev)
{
   Elm_Code_Widget *widget;

   widget = (Elm_Code_Widget *)data;

   // Insert the received text data at the current cursor position.
   elm_code_widget_text_at_cursor_insert(widget, ev->data);

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CODE_SELECTION_PASTE, widget);
   return EINA_TRUE;
}

EAPI void
elm_code_widget_selection_paste(Evas_Object *widget)
{
   elm_code_widget_selection_delete(widget);

   elm_cnp_selection_get(widget, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT, _selection_paste_cb, widget);
}

EAPI Eina_Bool
elm_code_widget_selection_is_empty(Evas_Object *widget)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Selection_Data *selection;
   Eina_Bool ret = EINA_FALSE;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd->selection)
     return EINA_TRUE; // No selection object means it's empty.

   selection = elm_code_widget_selection_normalized_get(widget);

   // Check if the normalized selection start and end positions are identical,
   // or if start_col is one greater than end_col on the same line.
   // The latter condition (start_col == end_col + 1) might indicate a zero-width
   // selection or a specific convention for an empty selection marker.
   // Typically, an empty selection means start_col == end_col and start_line == end_line.
   // This needs clarification if it's intended for a specific behavior.
   if (selection->start_col == selection->end_col + 1 && // This condition is unusual for "empty"
       selection->start_line == selection->end_line)
     ret = EINA_TRUE;
   // A more common check for empty would be:
   // if (selection->start_line == selection->end_line && selection->start_col == selection->end_col)
   //   ret = EINA_TRUE;


   free(selection);

   return ret;
}

/**
 * @internal
 * @brief Sets the 'in_progress' state of the current selection.
 *
 * The 'in_progress' flag can be used to indicate that a selection operation
 * (e.g., mouse drag) is currently active.
 *
 * @param widget The Elm_Code_Widget object.
 * @param state The new state for the 'in_progress' flag (EINA_TRUE or EINA_FALSE).
 */
void
_elm_code_widget_selection_in_progress_set(Evas_Object *widget, Eina_Bool state)
{
   Elm_Code_Widget_Data *pd;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd || !pd->selection)
     return;

   pd->selection->in_progress = state;
}

/**
 * @internal
 * @brief Sets the type of the current selection.
 *
 * The selection type can be used to differentiate between different kinds of
 * selections, e.g., normal character selection, line selection, word selection.
 *
 * @param widget The Elm_Code_Widget object.
 * @param type The Elm_Code_Widget_Selection_Type to set.
 */
void
_elm_code_widget_selection_type_set(Evas_Object *widget, Elm_Code_Widget_Selection_Type type)
{
   Elm_Code_Widget_Data *pd;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (!pd || !pd->selection)
     return;

   pd->selection->type = type;
}

