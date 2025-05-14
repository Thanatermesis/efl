#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_code_private.h"
#include "elm_code_widget_legacy_eo.h"

#define _ELM_CODE_DIFF_WIDGET_LEFT "diffwidgetleft"
#define _ELM_CODE_DIFF_WIDGET_RIGHT "diffwidgetright"

#define _ELM_CODE_DIFF_WIDGET_TYPE_ADDED "added"
#define _ELM_CODE_DIFF_WIDGET_TYPE_REMOVED "removed"
#define _ELM_CODE_DIFF_WIDGET_TYPE_CHANGED "changed"

/**
 * @brief Parses a single line from a diff and appends its content to the left and/or right panes.
 *
 * Depending on the line prefix ('+', '-', or other), this function determines
 * whether the line represents an addition, a deletion, or an unchanged line,
 * and updates the corresponding Elm_Code_File objects.
 *
 * @param line The diff line to parse.
 * @param left The Elm_Code_File representing the left (original) side of the diff.
 * @param right The Elm_Code_File representing the right (changed) side of the diff.
 */
static void
_elm_code_diff_widget_parse_diff_line(Elm_Code_Line *line, Elm_Code_File *left, Elm_Code_File *right)
{
   const char *content;
   unsigned int length;

   if (line->length < 1)
     {
        elm_code_file_line_append(left, "", 0, NULL);
        elm_code_file_line_append(right, "", 0, NULL);
     }

   content = elm_code_line_text_get(line, &length);
   if (content[0] == '+')
     {
        elm_code_file_line_append(left, "", 0, NULL);
        elm_code_file_line_append(right, content, length, NULL);
     }
   else if (content[0] == '-')
     {
        elm_code_file_line_append(left, content, length, NULL);
        elm_code_file_line_append(right, "", 0, NULL);
     }
   else
     {
        elm_code_file_line_append(left, content, length, NULL);
        elm_code_file_line_append(right, content, length, NULL);
     }
}

/**
 * @brief Parses the entire content of a diff file and populates two Elm_Code_File objects
 * representing the left (original) and right (changed) views of the diff.
 *
 * It iterates through each line of the input `diff` file.
 * Lines starting with 'd', 'i', or 'n' (typically diff command/index lines) are
 * added to both left and right views.
 * Other lines are processed based on an offset, which helps to distinguish
 * between lines belonging to the left file, right file, or lines needing
 * further parsing by `_elm_code_diff_widget_parse_diff_line`.
 * After processing all lines, it triggers a re-parse for both left and right
 * Elm_Code_File objects to apply syntax highlighting and other features.
 *
 * @param diff The Elm_Code_File containing the diff content to be parsed.
 * @param left The Elm_Code_File to populate with the left (original) side of the diff.
 * @param right The Elm_Code_File to populate with the right (changed) side of the diff.
 */
static void
_elm_code_diff_widget_parse_diff(Elm_Code_File *diff, Elm_Code_File *left, Elm_Code_File *right)
{
   Eina_List *item;
   Elm_Code_Line *line;
   const char *content;
   unsigned int offset, length;

   offset = 0;
   EINA_LIST_FOREACH(diff->lines, item, line)
     {
        content = elm_code_line_text_get(line, &length);

        if (length > 0 && (content[0] == 'd' || content[0] == 'i' || content[0] == 'n'))
          {
             offset = 0;
             elm_code_file_line_append(left, content, length, NULL);
             elm_code_file_line_append(right, content, length, NULL);

             continue;
          }

        if (offset == 0)
          elm_code_file_line_append(left, content, length, NULL);
        else if (offset == 1)
          elm_code_file_line_append(right, content, length, NULL);
        else
          _elm_code_diff_widget_parse_diff_line(line, left, right);

        offset++;
     }

   _elm_code_parse_file(left->parent, left);
   _elm_code_parse_file(right->parent, right);
}

/**
 * @brief Creates a new diff widget.
 *
 * This function sets up a horizontal pane with two Elm_Code_Widget instances,
 * one for the left side and one for the right side of the diff.
 * It then parses the provided `code` (which should contain diff content)
 * and populates the two widgets.
 *
 * @param parent The parent Evas_Object.
 * @param code The Elm_Code object containing the diff data.
 * @return The newly created Evas_Object for the diff widget (an elm_panes).
 */
EAPI Evas_Object *
elm_code_diff_widget_add(Evas_Object *parent, Elm_Code *code)
{
   Elm_Code *wcode1, *wcode2;
   Elm_Code_Widget *widget_left, *widget_right;
   Evas_Object *hbox;

   hbox = elm_panes_add(parent);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_panes_horizontal_set(hbox, EINA_FALSE);
   evas_object_show(hbox);

   // left side of diff
   wcode1 = elm_code_create();
   elm_code_parser_standard_add(wcode1, ELM_CODE_PARSER_STANDARD_DIFF);
   widget_left = elm_legacy_add(ELM_CODE_WIDGET_LEGACY_CLASS, parent, efl_ui_code_widget_code_set(efl_added, wcode1));

   evas_object_size_hint_weight_set(widget_left, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget_left, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget_left);
   evas_object_data_set(hbox, _ELM_CODE_DIFF_WIDGET_LEFT, widget_left);
   elm_object_part_content_set(hbox, "left", widget_left);

   // right side of diff
   wcode2 = elm_code_create();
   elm_code_parser_standard_add(wcode2, ELM_CODE_PARSER_STANDARD_DIFF);
   widget_right = elm_legacy_add(ELM_CODE_WIDGET_LEGACY_CLASS, parent, efl_ui_code_widget_code_set(efl_added, wcode2));

   evas_object_size_hint_weight_set(widget_right, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget_right, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget_right);
   evas_object_data_set(hbox, _ELM_CODE_DIFF_WIDGET_RIGHT, widget_right);
   elm_object_part_content_set(hbox, "right", widget_right);

   _elm_code_diff_widget_parse_diff(code->file, wcode1->file, wcode2->file);
   return hbox;
}

/**
 * @brief Sets the font name and size for both panes of the diff widget.
 *
 * @param widget The diff widget (the Evas_Object returned by elm_code_diff_widget_add).
 * @param name The name of the font to use.
 * @param size The size of the font.
 */
EAPI void
elm_code_diff_widget_font_set(Evas_Object *widget, const char *name, int size)
{
   Elm_Code_Widget *child;

   child = (Elm_Code_Widget *) evas_object_data_get(widget, _ELM_CODE_DIFF_WIDGET_LEFT);
   efl_ui_code_widget_font_set(child, name, size);
   child = (Elm_Code_Widget *) evas_object_data_get(widget, _ELM_CODE_DIFF_WIDGET_RIGHT);
   efl_ui_code_widget_font_set(child, name, size);
}

