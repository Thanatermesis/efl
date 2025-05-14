#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Creates a standard window for a test case.
 *
 * @param id The window identifier.
 * @param name The window title.
 * @return A new Evas_Object window instance.
 */
static Evas_Object *_test_code_win_create(const char *id, const char *name)
{
   Evas_Object *win;

   win = elm_win_util_standard_add(id, name);
   elm_win_title_set(win, name);
   elm_win_autodel_set(win, EINA_TRUE);

   evas_object_resize(win, 360 * elm_config_scale_get(),
                           220 * elm_config_scale_get());
   return win;
}

/**
 * @brief Appends a string as a new line to an Elm_Code_File.
 *
 * This is a convenience wrapper around elm_code_file_line_append.
 *
 * @param file The Elm_Code_File to append to.
 * @param line The string content of the line to add.
 */
static void _append_line(Elm_Code_File *file, const char *line)
{
   int length;

   length = strlen(line);
   elm_code_file_line_append(file, line, length, NULL);
}

/**
 * @brief Callback for when a line is clicked in the code widget.
 *
 * Prints the line number of the clicked line to standard output.
 *
 * @param data User data, unused in this callback.
 * @param event The Efl_Event details. The event info is an Elm_Code_Line.
 */
static void
_elm_code_test_line_clicked_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   printf("CLICKED line %d\n", line->number);
}

/**
 * @brief Callback for when a line has finished loading.
 *
 * This callback is used to apply special formatting or status to lines
 * after they are loaded. It demonstrates adding a token to the first line
 * and setting an error status on the second line.
 * It stops further event processing for this event.
 *
 * @param data User data, unused in this callback.
 * @param event The Efl_Event details. The event info is an Elm_Code_Line.
 */
static void
_elm_code_test_line_done_cb(void *data EINA_UNUSED, const Efl_Event *event)
{
   Elm_Code_Line *line;

   line = (Elm_Code_Line *)event->info;

   if (line->number == 1)
     elm_code_line_token_add(line, 17, 24, 1, ELM_CODE_TOKEN_TYPE_COMMENT);
   else if (line->number == 2)
     {
        line->status = ELM_CODE_STATUS_TYPE_ERROR;
        line->status_text = "  -> This warning is important!";
     }

   efl_event_callback_stop(event->object);
}

/**
 * @brief Sets up the "Welcome" test case widget.
 *
 * This demonstrates basic features of Elm_Code, including:
 * - Adding lines of text.
 * - Adding tokens for highlighting.
 * - Programmatically setting a text selection.
 * - Toggling a status on a line.
 *
 * @param parent The parent Evas_Object.
 * @return The created Elm_Code_Widget.
 */
static Evas_Object *
_elm_code_test_welcome_setup(Evas_Object *parent)
{
   Elm_Code *code;
   Elm_Code_Line *line;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   efl_event_callback_add(widget, &ELM_CODE_EVENT_LINE_LOAD_DONE, _elm_code_test_line_done_cb, NULL);
   efl_event_callback_add(widget, EFL_UI_CODE_WIDGET_EVENT_LINE_CLICKED, _elm_code_test_line_clicked_cb, code);

   _append_line(code->file, "❤ Hello World, Elm Code! ❤");
   _append_line(code->file, "*** Currently experimental ***");
   _append_line(code->file, "");
   _append_line(code->file, "This is a demo of elm_code's capabilities.");

   line = elm_code_file_line_get(code->file, 1);
   elm_code_line_token_add(line, 17, 19, 1, ELM_CODE_TOKEN_TYPE_MATCH);
   line = elm_code_file_line_get(code->file, 4);
   elm_code_line_token_add(line, 18, 20, 1, ELM_CODE_TOKEN_TYPE_MATCH);

   elm_code_widget_selection_start(widget, 1, 3);
   elm_code_widget_selection_end(widget, 1, 13);

   line = elm_code_file_line_get(code->file, 2);
   elm_code_widget_line_status_toggle(widget, line);

   return widget;
}

/**
 * @brief Sets up a basic code editor widget.
 *
 * This function creates an Elm_Code_Widget configured as a simple
 * text editor. It can be initialized with some default text or
 * left empty for logging purposes.
 *
 * @param parent The parent Evas_Object.
 * @param log If EINA_TRUE, the editor is initialized empty, intended
 *        for dynamic content like logs. If EINA_FALSE, it's populated
 *        with some sample editable text.
 * @return The created Elm_Code_Widget.
 */
static Evas_Object *
_elm_code_test_editor_setup(Evas_Object *parent, Eina_Bool log)
{
   Elm_Code *code;
   Elm_Code_Line *line;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   efl_ui_code_widget_font_set(widget, NULL, 14);
   efl_ui_code_widget_editable_set(widget, EINA_TRUE);
   efl_ui_code_widget_show_whitespace_set(widget, EINA_TRUE);
   efl_ui_code_widget_line_numbers_set(widget, EINA_TRUE);

   if (!log)
     {
        _append_line(code->file, "Edit me :)");
        _append_line(code->file, "");
        _append_line(code->file, "");
        _append_line(code->file, "...Please?");

        line = elm_code_file_line_get(code->file, 1);
        elm_code_line_token_add(line, 5, 6, 1, ELM_CODE_TOKEN_TYPE_COMMENT);
        elm_code_callback_fire(code, &ELM_CODE_EVENT_LINE_LOAD_DONE, line);
     }

   return widget;
}

/**
 * @brief Sets up a code editor to demonstrate syntax highlighting.
 *
 * The widget is populated with a small C code example. Syntax highlighting
 * is enabled and the MIME type is set to "text/x-csrc" to trigger the
 * C syntax parser. The indentation style uses spaces.
 *
 * @param parent The parent Evas_Object.
 * @return The created Elm_Code_Widget with syntax highlighting.
 */
static Evas_Object *
_elm_code_test_syntax_setup(Evas_Object *parent)
{
   Elm_Code *code;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   widget = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   efl_ui_code_widget_editable_set(widget, EINA_TRUE);
   efl_ui_code_widget_syntax_enabled_set(widget, EINA_TRUE);
   efl_ui_code_widget_code_get(widget)->file->mime = "text/x-csrc";
   efl_ui_code_widget_show_whitespace_set(widget, EINA_TRUE);
   efl_ui_code_widget_line_numbers_set(widget, EINA_TRUE);

   _append_line(code->file, "#include <stdio.h>");
   _append_line(code->file, "int main(int argc, char **argv)");
   _append_line(code->file, "{");
   _append_line(code->file, "   // display a welcome greeting");
   _append_line(code->file, "   if (argc > 0)");
   _append_line(code->file, "     printf(\"Hello, %s!\\n\", argv[0]);");
   _append_line(code->file, "   else");
   _append_line(code->file, "     printf(\"Hello, World!\\n\");");
   _append_line(code->file, "   return 0;");
   _append_line(code->file, "}");

   return widget;
}

/**
 * @brief Sets up a code editor to demonstrate syntax highlighting with tabs.
 *
 * Similar to _elm_code_test_syntax_setup, but configures the widget to
 * use actual tab characters for indentation instead of spaces. The C code
 * example provided also uses tabs.
 *
 * @param parent The parent Evas_Object.
 * @return The created Elm_Code_Widget configured for tabs.
 */
static Evas_Object *
_elm_code_test_syntax_tabbed_setup(Evas_Object *parent)
{
   Elm_Code *code;
   Elm_Code_Widget *widget;

   code = elm_code_create();
   code->config.indent_style_efl = EINA_FALSE;
   widget = efl_add(ELM_CODE_WIDGET_CLASS, parent, efl_ui_code_widget_code_set(efl_added, code));
   efl_ui_code_widget_editable_set(widget, EINA_TRUE);
   efl_ui_code_widget_syntax_enabled_set(widget, EINA_TRUE);
   efl_ui_code_widget_code_get(widget)->file->mime = "text/x-csrc";
   efl_ui_code_widget_show_whitespace_set(widget, EINA_TRUE);
   efl_ui_code_widget_line_numbers_set(widget, EINA_TRUE);
   efl_ui_code_widget_tab_inserts_spaces_set(widget, EINA_FALSE);

   _append_line(code->file, "#include <stdio.h>");
   _append_line(code->file, "int main(int argc, char **argv)");
   _append_line(code->file, "{");
   _append_line(code->file, "\t// display a welcome greeting");
   _append_line(code->file, "\tif (argc > 0)");
   _append_line(code->file, "\t\tprintf(\"Hello, %s!\\n\", argv[0]);");
   _append_line(code->file, "\telse");
   _append_line(code->file, "\t\tprintf(\"Hello, World!\\n\");");
   _append_line(code->file, "\treturn 0;");
   _append_line(code->file, "}");

   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   return widget;
}

/**
 * @brief Creates a "mirror" code widget for an existing Elm_Code model.
 *
 * A mirror widget shares the same underlying Elm_Code document (the model)
 * but can have its own view-specific settings, like a different font.
 * This demonstrates how multiple views can display the same data.
 *
 * @param code The existing Elm_Code object to share.
 * @param font_name The font to use for this mirror widget, e.g. "Mono:style=Oblique".
 * @param parent The parent Evas_Object.
 * @return The created mirror Elm_Code_Widget.
 */
static Evas_Object *
_elm_code_test_mirror_setup(Elm_Code *code, char *font_name, Evas_Object *parent)
{
   Elm_Code_Widget *widget;

   widget = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(widget, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(widget, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(widget);

   efl_ui_code_widget_font_set(widget, font_name, 11);
   efl_ui_code_widget_line_numbers_set(widget, EINA_TRUE);

   return widget;
}

/**
 * @brief Sets up a widget to display an inline diff.
 *
 * This function loads a .diff file and uses the standard diff parser
 * to display the changes within a single widget. Additions and deletions
 * are shown in place.
 *
 * @param parent The parent Evas_Object.
 * @return The created Elm_Code_Widget showing the diff.
 */
static Evas_Object *
_elm_code_test_diff_inline_setup(Evas_Object *parent)
{
   Evas_Object *diff;
   Elm_Code *code;

   code = elm_code_create();
   diff = elm_code_widget_add(parent, code);
   evas_object_size_hint_weight_set(diff, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(diff, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(diff);

   elm_code_parser_standard_add(code, ELM_CODE_PARSER_STANDARD_DIFF);
   elm_code_file_open(code, PACKAGE_DATA_DIR "/testdiff.diff");

   return diff;
}

/**
 * @brief Sets up a widget for a side-by-side diff view.
 *
 * It loads a .diff file and uses the Elm_Code_Diff_Widget to show
 * the original and modified versions next to each other.
 *
 * @param parent The parent Evas_Object.
 * @return The created Elm_Code_Diff_Widget.
 */
static Evas_Object *
_elm_code_test_diff_setup(Evas_Object *parent)
{
   Evas_Object *diff;
   Elm_Code *code;

   code = elm_code_create();
   elm_code_file_open(code, PACKAGE_DATA_DIR "/testdiff.diff");

   diff = elm_code_diff_widget_add(parent, code);
   return diff;
}

/**
 * @brief Timer callback to continuously append lines to a code widget.
 *
 * This function is used to test the performance of the code widget when
 * lines are added at a high frequency. It adds a new line with an
 * incrementing counter every time it's called.
 *
 * @param data The Elm_Code object to append lines to.
 * @return ECORE_CALLBACK_RENEW to keep the timer running.
 */
static Eina_Bool
_elm_code_test_log_timer(void *data)
{
   Elm_Code *code = data;
   static int line = 0;
   char buf[250];

   sprintf(buf, "line %d", ++line);
   _append_line(code->file, buf);

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Click handler to start or stop the logging timer.
 *
 * Toggles a timer that calls _elm_code_test_log_timer. The button's
 * text is updated to reflect the current state ("Start" or "Stop").
 *
 * @param data The Elm_Code object to be passed to the timer.
 * @param obj The button that was clicked.
 * @param event_info Unused event information.
 */
static void
_elm_code_test_log_clicked(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   static Ecore_Timer *t = NULL;

   if (t)
     {
        elm_object_text_set(obj, "Start");
        ecore_timer_del(t);
        t = NULL;
        return;
     }

   t = ecore_timer_add(0.05, _elm_code_test_log_timer, data);
   elm_object_text_set(obj, "Stop");
}

/**
 * @brief Test case for a simple text editor.
 *
 * Creates a window containing a single Elm_Code_Widget configured
 * as a basic text editor with some initial content.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_editor(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-editor", "Text Editor");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_editor_setup(screen, EINA_FALSE));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for syntax highlighting.
 *
 * Creates a window containing an Elm_Code_Widget that demonstrates
 * C syntax highlighting.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_syntax(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-syntax", "Code Syntax");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_syntax_setup(screen));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for syntax highlighting with tab indentation.
 *
 * Creates a window with an Elm_Code_Widget demonstrating C syntax
 * highlighting where indentation is done with tabs instead of spaces.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_syntax_tabbed(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-syntax-tabbed", "Code Syntax (Tabbed)");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_syntax_tabbed_setup(screen));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for logging performance.
 *
 * Creates a window with a code widget and a button. Clicking the button
 * starts/stops a timer that rapidly adds lines to the widget, testing
 * its performance under high load.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_log(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen, *o, *code;

   win = _test_code_win_create("code-log", "Code Log");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   code = _elm_code_test_editor_setup(screen, EINA_TRUE);
   elm_box_pack_end(screen, code);

   o = elm_button_add(screen);
   elm_object_text_set(o, "log");
   evas_object_smart_callback_add(o, "clicked", _elm_code_test_log_clicked, efl_ui_code_widget_code_get(code));
   elm_box_pack_end(screen, o);
   evas_object_show(o);

   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for mirrored code views.
 *
 * This test creates a window with one main editor widget and two "mirror"
 * widgets. All three widgets share the same underlying text document, so
 * edits in the main widget are reflected in the mirrors. Each mirror,
 * however, uses a different font.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_mirror(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Code *code;
   Evas_Object *win, *screen, *widget;

   win = _test_code_win_create("code-mirror", "Code Mirror");
   screen = elm_box_add(win);
   elm_box_homogeneous_set(screen, EINA_TRUE);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   widget = _elm_code_test_editor_setup(screen, EINA_FALSE);
   code = efl_ui_code_widget_code_get(widget);
   elm_box_pack_end(screen, widget);

   elm_box_pack_end(screen, _elm_code_test_mirror_setup(code, "Mono:style=Oblique", screen));
   elm_box_pack_end(screen, _elm_code_test_mirror_setup(code, "Nimbus Mono", screen));

   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for the inline diff viewer.
 *
 * Creates a window that displays a diff file in an inline format, where
 * added and removed lines are shown within a single view.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_diff_inline(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-diff-inline", "Diff Inline");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_diff_inline_setup(screen));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for the side-by-side diff viewer.
 *
 * Creates a window containing an Elm_Code_Diff_Widget, which shows
 * a side-by-side comparison from a diff file.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_diff(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-diff", "Diff Comparison");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_diff_setup(screen));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

/**
 * @brief Test case for the "welcome" screen.
 *
 * Creates a window to show a basic welcome message with some highlighted
 * text and a selection, demonstrating simple markup capabilities.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_code_welcome(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *screen;

   win = _test_code_win_create("code-welcome", "Entry Markup");
   screen = elm_box_add(win);
   evas_object_size_hint_weight_set(screen, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(screen, _elm_code_test_welcome_setup(screen));
   elm_win_resize_object_add(win, screen);
   evas_object_show(screen);

   evas_object_show(win);
}

