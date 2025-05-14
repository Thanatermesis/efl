#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_code_widget_private.h"

/**
 * @brief Clears all undo information that comes after the current undo stack pointer.
 *
 * This function is called when a new change is added to the undo stack,
 * effectively truncating the "redo" history. It iterates backwards from
 * the current undo stack pointer (exclusive) to the beginning of the list,
 * freeing the memory associated with each change info.
 *
 * @param widget The Elm_Code_Widget object.
 */
static void
_elm_code_widget_undo_prev_clear(Evas_Object *widget)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Change_Info *info;
   Eina_List *list;

   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);
   if (!pd->undo_stack_ptr)
     return;

   for (list = eina_list_prev(pd->undo_stack_ptr); list; list = eina_list_prev(list))
     {
        info = eina_list_data_get(list);
        free(info->content);
        free(info);
     }
}

/**
 * @brief Creates a deep copy of an Elm_Code_Widget_Change_Info structure.
 *
 * This is necessary because the undo stack stores copies of change information,
 * not pointers to the original, potentially transient, data.
 *
 * @param info Pointer to the Elm_Code_Widget_Change_Info to copy.
 * @return A pointer to the newly allocated and copied Elm_Code_Widget_Change_Info,
 *         or NULL on allocation failure. The `content` field is also duplicated.
 */
Elm_Code_Widget_Change_Info *
_elm_code_widget_undo_info_copy(Elm_Code_Widget_Change_Info *info)
{
   Elm_Code_Widget_Change_Info *copy;

   copy = calloc(1, sizeof(*info));
   if (!copy) return NULL;
   memcpy(copy, info, sizeof(*info));
   copy->content = eina_strndup(info->content, info->length);

   return copy;
}

/**
 * @brief Adds a new change to the undo stack.
 *
 * Before adding the new change, this function clears any existing "redo"
 * history (changes that were undone and could be redone). A copy of the
 * provided `info` is made and prepended to the undo stack.
 *
 * @param widget The Elm_Code_Widget object.
 * @param info Pointer to the Elm_Code_Widget_Change_Info describing the change.
 *             This structure will be copied.
 */
void
_elm_code_widget_undo_change_add(Evas_Object *widget,
                                 Elm_Code_Widget_Change_Info *info)
{
   Elm_Code_Widget_Data *pd;
   Elm_Code_Widget_Change_Info *info_copy;

   info_copy = _elm_code_widget_undo_info_copy(info);
   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   _elm_code_widget_undo_prev_clear(widget);

   pd->undo_stack_ptr = eina_list_prepend(pd->undo_stack_ptr, info_copy);
   pd->undo_stack = pd->undo_stack_ptr;
}

/**
 * @brief Applies a change described by Elm_Code_Widget_Change_Info to the widget.
 *
 * This function is the core logic for both undoing and redoing operations.
 * If `info->insert` is true, it means the original operation was an insert,
 * so to undo it, the text between `start_line:start_col` and `end_line:end_col`
 * is deleted.
 * If `info->insert` is false, it means the original operation was a delete (or replace),
 * so to undo it, the `info->content` is inserted at `start_line:start_col`.
 *
 * @param widget The Elm_Code_Widget object.
 * @param info Pointer to the Elm_Code_Widget_Change_Info describing the change to apply.
 */
static void
_elm_code_widget_undo_change(Evas_Object *widget,
                             Elm_Code_Widget_Change_Info *info)
{
   Elm_Code_Widget_Data *pd;
   unsigned int textlen, position, row, col, newrow, remainlen;
   short nllen;
   char *content;
   Elm_Code_Line *line;
   pd = efl_data_scope_get(widget, ELM_CODE_WIDGET_CLASS);

   if (info->insert)
     {
        elm_code_widget_selection_start(widget, info->start_line, info->start_col);
        elm_code_widget_selection_end(widget, info->end_line, info->end_col);
        _elm_code_widget_selection_delete_no_undo(widget);
     }
   else
     {
        newrow = info->start_line;
        content = info->content;
        remainlen = info->length;
        elm_code_widget_selection_clear(widget);
        elm_code_widget_cursor_position_set(widget, info->start_line,
                                            info->start_col);
        while (newrow <= info->end_line)
          {
             line = elm_code_file_line_get(pd->code->file, newrow);
             if (newrow != info->end_line)
               {
                  textlen = elm_code_text_newlinenpos(content, remainlen,
                                                      &nllen);
                  remainlen -= textlen + nllen;
                  _elm_code_widget_text_at_cursor_insert_no_undo(widget,
                                                                 content,
                                                                 textlen);
                  efl_ui_code_widget_cursor_position_get(widget, &row, &col);
                  position = elm_code_widget_line_text_position_for_column_get(widget, line, col);
                  elm_code_line_split_at(line, position);
                  elm_code_widget_cursor_position_set(widget, newrow + 1, 1);
                  content += textlen + nllen;
               }
             else
               {
                  _elm_code_widget_text_at_cursor_insert_no_undo(widget,
                                                                 content,
                                                                 strlen(content));
               }
             newrow++;
          }
        elm_code_widget_cursor_position_set(widget, info->end_line,
                                            info->end_col + 1);
     }

   efl_event_callback_legacy_call(widget, EFL_UI_CODE_WIDGET_EVENT_CHANGED_USER, NULL);
}

/**
 * @brief Checks if an undo operation can be performed.
 *
 * @param obj The Elm_Code_Widget object (unused).
 * @param pd Pointer to the private data of the Elm_Code_Widget.
 * @return EINA_TRUE if there is at least one action in the undo stack
 *         that can be undone, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_code_widget_can_undo_get(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd)
{
   return !!pd->undo_stack_ptr;
}

/**
 * @brief Performs an undo operation.
 *
 * Retrieves the last change from the undo stack, applies its inverse,
 * and moves the undo stack pointer to the next older change.
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd Pointer to the private data of the Elm_Code_Widget.
 */
static void
_elm_code_widget_undo(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd)
{
   Elm_Code_Widget_Change_Info *info;

   if (!pd->undo_stack_ptr)
     return;

   info = eina_list_data_get(pd->undo_stack_ptr);
   _elm_code_widget_undo_change(obj, info);

   pd->undo_stack_ptr = eina_list_next(pd->undo_stack_ptr);
}

/**
 * @brief Checks if a redo operation can be performed.
 *
 * A redo is possible if the `undo_stack_ptr` is not at the head of the
 * `undo_stack` list (meaning there are undone changes).
 *
 * @param obj The Elm_Code_Widget object (unused).
 * @param pd Pointer to the private data of the Elm_Code_Widget.
 * @return EINA_TRUE if there is at least one action that can be redone,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_code_widget_can_redo_get(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd)
{
   if (pd->undo_stack_ptr)
     return !!eina_list_prev(pd->undo_stack_ptr);

   // If undo_stack_ptr is NULL, it means we've undone all the way to the beginning.
   // In this case, redo is possible if the undo_stack itself is not empty.
   return !!eina_list_last(pd->undo_stack);
}

/**
 * @brief Performs a redo operation.
 *
 * Retrieves the next change from the undo stack (the one that was previously undone),
 * applies it, and moves the undo stack pointer to that redone change.
 * The `insert` flag of the change info is inverted because redoing an
 * "undo of an insert" is an insert, and redoing an "undo of a delete" is a delete.
 *
 * @param obj The Elm_Code_Widget object.
 * @param pd Pointer to the private data of the Elm_Code_Widget.
 */
static void
_elm_code_widget_redo(Eo *obj EINA_UNUSED, Elm_Code_Widget_Data *pd)
{
   Elm_Code_Widget_Change_Info *info, *redo_info;
   Eina_List *redo_ptr;

   if (pd->undo_stack_ptr)
     redo_ptr = eina_list_prev(pd->undo_stack_ptr);
   else
     redo_ptr = eina_list_last(pd->undo_stack);

   if (!redo_ptr)
     return;

   info = eina_list_data_get(redo_ptr);
   redo_info = _elm_code_widget_undo_info_copy(info);
   redo_info->insert = redo_info->insert ? EINA_FALSE : EINA_TRUE;
   _elm_code_widget_undo_change(obj, redo_info);

   pd->undo_stack_ptr = redo_ptr;

   free(redo_info->content);
   free(redo_info);
}

