#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

static Evas_Object *glb;

/**
 * @brief Callback for when the "Copy" button is clicked.
 *
 * This function retrieves text from the provided entry widget, sets it as the
 * clipboard content, and also updates a global label to display the current
 * clipboard text.
 *
 * @param data The entry widget (Evas_Object *) to copy text from.
 * @param obj The button object that triggered the event (unused).
 * @param event_info Additional event information (unused).
 */
static void
_bt_copy_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *en = data;
   const char *txt = elm_object_text_get(en);

   elm_object_text_set(glb, txt);
   elm_cnp_selection_set(elm_object_parent_widget_get(en), ELM_SEL_TYPE_CLIPBOARD,
                         ELM_SEL_FORMAT_TEXT, txt, strlen(txt));
}

/**
 * @brief Callback function that receives the content from the clipboard.
 *
 * This function is called when clipboard data is received after a paste
 * request. It checks the format of the data and, if it's plain text,
 * inserts it into the target entry widget. It also logs the type and
 * size of the received data.
 *
 * @param data The target entry widget (Evas_Object *) for pasting.
 * @param obj The object that owns the selection (unused).
 * @param ev The selection event data containing the content and format.
 * @return EINA_TRUE to indicate the data was handled.
 */
static Eina_Bool
_selection(void *data, Evas_Object *obj EINA_UNUSED, Elm_Selection_Data *ev)
{
   Evas_Object *en = data;
   const char *fmt = NULL;

   switch (ev->format)
     {
        case ELM_SEL_FORMAT_TARGETS: fmt = "TARGETS"; break;
        case ELM_SEL_FORMAT_NONE: fmt = "NONE"; break;
        case ELM_SEL_FORMAT_TEXT: fmt = "TEXT"; break;
        case ELM_SEL_FORMAT_MARKUP: fmt = "MARKUP"; break;
        case ELM_SEL_FORMAT_IMAGE: fmt = "IMAGE"; break;
        case ELM_SEL_FORMAT_VCARD: fmt = "VCARD"; break;
        case ELM_SEL_FORMAT_HTML: fmt = "HTML"; break;
        case ELM_SEL_FORMAT_URILIST: fmt = "URILIST"; break;
     }
   fprintf(stderr, "got selection type '%s': length %zu\n", fmt, ev->len);

   if (ev->format == ELM_SEL_FORMAT_TEXT)
     {
        char *stripstr;

        stripstr = malloc(ev->len + 1);
        strncpy(stripstr, (char *)ev->data, ev->len);
        stripstr[ev->len] = '\0';
        elm_entry_entry_insert(en, stripstr);
        free(stripstr);
     }

   return EINA_TRUE;
}

/**
 * @brief Callback for when the "Paste" button is clicked.
 *
 * This function initiates a request to get text content from the clipboard.
 * The actual data handling and pasting is done in the _selection callback.
 *
 * @param data The entry widget (Evas_Object *) to paste text into.
 * @param obj The button object that triggered the event (unused).
 * @param event_info Additional event information (unused).
 */
static void
_bt_paste_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *en = data;

   elm_cnp_selection_get(en, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT,
                         _selection, en);
}

/**
 * @brief Callback for when the "Clear" button is clicked.
 *
 * This function clears the system clipboard and also clears the text of the
 * global label that displays the clipboard content.
 *
 * @param data The entry widget (Evas_Object *), used to get the parent widget.
 * @param obj The button object that triggered the event (unused).
 * @param event_info Additional event information (unused).
 */
static void
_bt_clear_clicked(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *en = data;

   elm_object_text_set(glb, "");
   elm_object_cnp_selection_clear(elm_object_parent_widget_get(en), ELM_SEL_TYPE_CLIPBOARD);
}

/**
 * @brief Sets up and runs the copy-paste test window.
 *
 * This function creates a window containing two entry widgets, buttons for
 * "Copy", "Paste", and "Clear", and a label to show the current clipboard
 * contents. It demonstrates the copy and paste functionality within an
 * Elementary application.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_cnp(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *gd, *bt, *en, *lb;

   win = elm_win_util_standard_add("copypaste", "CopyPaste");
   elm_win_autodel_set(win, EINA_TRUE);

   gd = elm_grid_add(win);
   elm_grid_size_set(gd, 100, 100);
   evas_object_size_hint_weight_set(gd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, gd);
   evas_object_show(gd);

   en = elm_entry_add(win);
   elm_entry_scrollable_set(en, EINA_TRUE);
   elm_entry_line_wrap_set(en, ELM_WRAP_CHAR);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(en, "Elementary provides ");
   elm_grid_pack(gd, en, 10, 10, 60, 30);
   evas_object_show(en);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Copy from left entry");
   evas_object_smart_callback_add(bt, "clicked", _bt_copy_clicked, en);
   elm_grid_pack(gd, bt, 70, 10, 22, 30);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Clear clipboard");
   evas_object_smart_callback_add(bt, "clicked", _bt_clear_clicked, en);
   elm_grid_pack(gd, bt, 70, 70, 22, 20);
   evas_object_show(bt);

   en = elm_entry_add(win);
   elm_entry_scrollable_set(en, EINA_TRUE);
   elm_entry_line_wrap_set(en, ELM_WRAP_CHAR);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(en, "rich copying and pasting functionality,");
   elm_grid_pack(gd, en, 10, 40, 60, 30);
   evas_object_show(en);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Paste to left entry");
   evas_object_smart_callback_add(bt, "clicked", _bt_paste_clicked, en);
   elm_grid_pack(gd, bt, 70, 40, 22, 30);
   evas_object_show(bt);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "<b>Clipboard:</b>");
   evas_object_size_hint_weight_set(lb, 0.0, 0.0);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_grid_pack(gd, lb, 10, 70, 60, 10);
   evas_object_show(lb);

   glb = elm_label_add(win);
   elm_object_text_set(glb, "");
   evas_object_size_hint_weight_set(glb, 0.0, 0.0);
   evas_object_size_hint_align_set(glb, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_grid_pack(gd, glb, 10, 80, 60, 10);
   evas_object_show(glb);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           200 * elm_config_scale_get());
   evas_object_show(win);
}
