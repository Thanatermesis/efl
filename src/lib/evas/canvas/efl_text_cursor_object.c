#include "evas_common_private.h"
#include "evas_private.h"
#include "efl_canvas_textblock_internal.h"
#include "eo_internal.h"

#define MY_CLASS EFL_TEXT_CURSOR_OBJECT_CLASS
#define MY_CLASS_NAME "Efl.Text.Cursor"

/**
 * @brief Private data for the Efl.Text.Cursor_Object.
 *
 * This structure holds the internal data for a text cursor object, including
 * a handle to the underlying Evas textblock cursor and a reference to the
 * associated text object.
 */
typedef struct
{
   Efl_Text_Cursor_Handle *handle; /**< The evas textblock cursor handle. */
   Efl_Canvas_Object *text_obj; /**< The text object this cursor belongs to. */
} Efl_Text_Cursor_Object_Data;

/**
 * @brief An iterator for textblock selections.
 *
 * This structure provides an iterator over a list of rectangles that
 * represent a selection or a range within a textblock.
 */
struct _Evas_Textblock_Selection_Iterator
{
   Eina_Iterator                       iterator; /**< The Eina iterator superclass. */
   Eina_List                           *list; /**< The list of rectangles to iterate over. */
   Eina_List                           *current; /**< The current position in the list. */
};

typedef struct _Evas_Textblock_Selection_Iterator Evas_Textblock_Selection_Iterator;

EFL_CLASS_SIMPLE_CLASS(efl_text_cursor_object, "Efl.Text.Cursor", EFL_TEXT_CURSOR_OBJECT_CLASS)

/**
 * @brief Implements the Efl.Text.Cursor.position_set EAPI.
 *
 * Sets the cursor position to a given character index in the text.
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param position The character index to set the cursor to.
 */
EOLIAN static void
_efl_text_cursor_object_position_set(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, int position)
{
   evas_textblock_cursor_pos_set(pd->handle, position);
}

/**
 * @brief Implements the Efl.Text.Cursor.position_get EAPI.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @return The current character index of the cursor.
 */
EOLIAN static int
_efl_text_cursor_object_position_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   return evas_textblock_cursor_pos_get(pd->handle);
}

/**
 * @brief Implements the Efl.Text.Cursor.content_get EAPI.
 *
 * Retrieves the unicode character at the current cursor position.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @return The unicode character at the cursor position, or 0 if at the end.
 */
EOLIAN static Eina_Unicode
_efl_text_cursor_object_content_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   if (pd->handle && pd->handle->node)
    return eina_ustrbuf_string_get(pd->handle->node->unicode)[pd->handle->pos];
   else
    return 0;
}

/**
 * @brief Implements the Efl.Text.Cursor.content_geometry_get EAPI.
 *
 * Gets the geometry of the character content at the current cursor position.
 * This can be the geometry of a format item (like an image) or the
 * character itself.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @return The geometry of the content under the cursor.
 */
EOLIAN static Eina_Rect
_efl_text_cursor_object_content_geometry_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   Eina_Rect rect = {0};
   Eina_Bool item_is = evas_textblock_cursor_format_item_geometry_get(pd->handle, &(rect.x), &(rect.y), &(rect.w), &(rect.h));
   if (item_is)
      return rect;

   evas_textblock_cursor_pen_geometry_get(pd->handle, &(rect.x), &(rect.y), &(rect.w), &(rect.h));

   return rect;
}

/**
 * @brief Implements the Efl.Text.Cursor.line_number_set EAPI.
 *
 * Moves the cursor to a specific line number in the text.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param line_number The line number to move the cursor to.
 */
EOLIAN static void
_efl_text_cursor_object_line_number_set(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, int line_number)
{
   evas_textblock_cursor_line_set(pd->handle, line_number);
}

/**
 * @brief Implements the Efl.Text.Cursor.line_number_get EAPI.
 *
 * Retrieves the line number where the cursor is currently located.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @return The current line number.
 */
EOLIAN static int
_efl_text_cursor_object_line_number_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   Eina_Rect rect = {0};

   return evas_textblock_cursor_line_geometry_get(pd->handle, &(rect.x), &(rect.y), &(rect.w), &(rect.h));
}

/**
 * @brief Implements the Efl.Text.Cursor.cursor_geometry_get EAPI.
 *
 * Retrieves the geometry of the cursor itself. This can be for the primary cursor
 * (e.g., the blinking bar). It supports different cursor types for various
 * visual representations (e.g., before or under the character).
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param ctype The type of cursor geometry to retrieve.
 * @return The geometry rectangle for the cursor.
 */
EOLIAN static Eina_Rect
_efl_text_cursor_object_cursor_geometry_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Type ctype)
{
   Eina_Rect rc = {0};
   Evas_Textblock_Cursor_Type cursor_type = (ctype == EFL_TEXT_CURSOR_TYPE_BEFORE) ? EVAS_TEXTBLOCK_CURSOR_BEFORE : EVAS_TEXTBLOCK_CURSOR_UNDER;
   evas_textblock_cursor_geometry_bidi_get(pd->handle, &rc.x, &rc.y, &rc.w, &rc.h, NULL, NULL, NULL, NULL, cursor_type);
   return rc;
}

/**
 * @brief Implements getting the secondary/lower part of a cursor's geometry.
 *
 * This is useful for languages with complex scripts where a single character
 * might have multiple visual parts, and the cursor needs to be split. This function
 * gets the geometry for the "lower" or secondary part of such a split cursor.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param geometry2 A pointer to an Eina_Rect to store the geometry.
 * @return @c EINA_TRUE if a secondary geometry exists, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_text_cursor_object_lower_cursor_geometry_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Eina_Rect *geometry2)
{
   Eina_Rect rc = {0};
   Eina_Bool b_ret = EINA_FALSE;
   b_ret = evas_textblock_cursor_geometry_bidi_get(pd->handle, NULL, NULL, NULL, NULL, &rc.x, &rc.y, &rc.w, &rc.h, EVAS_TEXTBLOCK_CURSOR_BEFORE);
   if (geometry2)
     {
        *geometry2 = rc;
     }
   return b_ret;
}

/**
 * @brief Implements the Efl.Text.Cursor.equal EAPI.
 *
 * Checks if two cursors are at the same position.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param dst The other cursor to compare against.
 * @return @c EINA_TRUE if cursors are equal, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_text_cursor_object_equal(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, const Efl_Text_Cursor_Object *dst)
{
   return evas_textblock_cursor_equal(pd->handle, efl_text_cursor_object_handle_get(dst));
}

/**
 * @brief Implements the Efl.Text.Cursor.compare EAPI.
 *
 * Compares the position of this cursor with another cursor.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param dst The other cursor to compare against.
 * @return -1 if this cursor is before dst, 0 if they are at the same
 *         position, and 1 if this cursor is after dst.
 */
EOLIAN static int
_efl_text_cursor_object_compare(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, const Efl_Text_Cursor_Object *dst)
{
   return evas_textblock_cursor_compare(pd->handle, efl_text_cursor_object_handle_get(dst));
}

/**
 * @brief Copies the state of one cursor object to another.
 *
 * This function is an internal helper to duplicate a cursor's state. It creates a new
 * underlying evas cursor handle for the destination and copies the properties
 * from the source handle.
 *
 * @param obj The source cursor object.
 * @param dst The destination cursor object.
 */
static void
_efl_text_cursor_object_copy(const Efl_Text_Cursor_Object *obj, Efl_Text_Cursor_Object *dst)
{
   Efl_Text_Cursor_Object_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd);

   Efl_Text_Cursor_Object_Data *pd_dest = efl_data_scope_safe_get(dst, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd_dest);

   if (!pd->handle) return;

   Efl_Text_Cursor_Handle *handle = evas_object_textblock_cursor_new(pd->handle->obj);
   evas_textblock_cursor_copy(pd->handle, handle);
   pd_dest->text_obj = pd->text_obj;
   efl_text_cursor_object_handle_set(dst, handle);
   evas_textblock_cursor_unref(handle, NULL);
}

/**
 * @brief Implements the Efl.Duplicate.duplicate EAPI.
 *
 * Creates a new cursor object that is a duplicate of the given object.
 *
 * @param obj The object to duplicate.
 * @param pd Private data for the cursor object.
 * @return A new Efl_Text_Cursor_Object, or NULL on failure.
 */
EOLIAN static Efl_Text_Cursor_Object *
_efl_text_cursor_object_efl_duplicate_duplicate(const Eo *obj, Efl_Text_Cursor_Object_Data *pd EINA_UNUSED)
{
  Efl_Text_Cursor_Object *dup = efl_text_cursor_object_create(efl_parent_get(obj));

  _efl_text_cursor_object_copy(obj, dup);

  return dup;
}

/**
 * @brief Implements the Efl.Text.Cursor.move EAPI.
 *
 * Moves the cursor according to the specified movement type.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param type The type of movement to perform (e.g., next character, end of line).
 * @return @c EINA_TRUE if the cursor was moved, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_text_cursor_object_move(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Move_Type type)
{
   Eina_Bool moved = EINA_FALSE;
   int pos = evas_textblock_cursor_pos_get(pd->handle);

   switch (type) {
      case EFL_TEXT_CURSOR_MOVE_TYPE_CHARACTER_NEXT :
         moved = evas_textblock_cursor_char_next(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_CHARACTER_PREVIOUS :
         moved = evas_textblock_cursor_char_prev(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_CLUSTER_NEXT :
         moved = evas_textblock_cursor_cluster_next(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_CLUSTER_PREVIOUS :
         moved = evas_textblock_cursor_cluster_prev(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_PARAGRAPH_START :
         evas_textblock_cursor_paragraph_char_first(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_PARAGRAPH_END :
         evas_textblock_cursor_paragraph_char_last(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_WORD_START :
         moved = evas_textblock_cursor_word_start(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_WORD_END :
         moved = evas_textblock_cursor_word_end(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_LINE_START :
         evas_textblock_cursor_line_char_first(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_LINE_END :
         evas_textblock_cursor_line_char_last(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_FIRST :
         evas_textblock_cursor_paragraph_first(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_LAST :
         evas_textblock_cursor_paragraph_last(pd->handle);
         if (pos != evas_textblock_cursor_pos_get(pd->handle))
           moved = EINA_TRUE;
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_PARAGRAPH_NEXT :
         moved = evas_textblock_cursor_paragraph_next(pd->handle);
         break;
      case EFL_TEXT_CURSOR_MOVE_TYPE_PARAGRAPH_PREVIOUS :
         moved = evas_textblock_cursor_paragraph_prev(pd->handle);
         break;
     }

   return moved;
}

/**
 * @brief Implements the Efl.Text.Cursor.char_delete EAPI.
 *
 * Deletes a character at the current cursor position.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 */
EOLIAN static void
_efl_text_cursor_object_char_delete(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   evas_textblock_cursor_char_delete(pd->handle);
}

/**
 * @brief Implements the Efl.Text.Cursor.line_jump_by EAPI.
 *
 * Moves the cursor up or down by a specified number of lines.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param by The number of lines to jump. A positive value moves down,
 *           and a negative value moves up.
 * @return @c EINA_TRUE if the cursor moved, @c EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_text_cursor_object_line_jump_by(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, int by)
{
   if (!pd->handle) return EINA_FALSE;

   Eina_Bool moved = EINA_FALSE;
   int pos = evas_textblock_cursor_pos_get(pd->handle);
   evas_textblock_cursor_line_jump_by(pd->handle, by);
   moved = (pos != evas_textblock_cursor_pos_get(pd->handle));
   return moved;
}

/**
 * @brief Implements the Efl.Text.Cursor.char_coord_set EAPI.
 *
 * Sets the cursor position to the character that is located at the given coordinates.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param coord The coordinates to set the cursor at.
 */
EOLIAN static void
_efl_text_cursor_object_char_coord_set(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Eina_Position2D coord)
{
   evas_textblock_cursor_char_coord_set(pd->handle, coord.x, coord.y);
}

/**
 * @brief Implements the Efl.Text.Cursor.cluster_coord_set EAPI.
 *
 * Sets the cursor position to the grapheme cluster that is located at the
 * given coordinates.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param coord The coordinates to set the cursor at.
 */
EOLIAN static void
_efl_text_cursor_object_cluster_coord_set(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Eina_Position2D coord)
{
   evas_textblock_cursor_cluster_coord_set(pd->handle, coord.x, coord.y);
}

/**
 * @brief Prepends a substring of text at the given cursor.
 *
 * This is a helper function for `_cursor_text_append`. It takes a start (s)
 * and end (p) pointer and prepends the text between them to the cursor.
 *
 * @param cur The cursor handle.
 * @param s The start of the string to prepend.
 * @param p The end of the substring to prepend.
 * @return The number of characters prepended.
 */
static int
_prepend_text_run2(Efl_Text_Cursor_Handle *cur, const char *s, const char *p)
{
   if ((s) && (p > s))
     {
        char *ts;

        ts = alloca(p - s + 1);
        strncpy(ts, s, p - s);
        ts[p - s] = 0;
        return evas_textblock_cursor_text_prepend(cur, ts);
     }
   return 0;
}

/**
 * @brief Appends text to a cursor, handling special formatting characters.
 *
 * This function iterates through the input text and inserts it at the
 * cursor's position. It specifically handles paragraph separators, newlines,
 * and tabs by converting them into the appropriate textblock format codes.
 *
 * @param cur The cursor handle.
 * @param text The text to append.
 * @return The total number of characters and format specifiers inserted.
 */
int
_cursor_text_append(Efl_Text_Cursor_Handle *cur,
      const char *text)
{
   if (!text || !cur) return 0;

   const char *off = text;
   int len = 0;

   Evas_Object_Protected_Data *obj = efl_data_scope_safe_get(cur->obj, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, 0);
   evas_object_async_block(obj);

   while (*off)
     {
        char *format = NULL;
        int n = 1;
        if (!strncmp(_PARAGRAPH_SEPARATOR_UTF8, off,
                    strlen(_PARAGRAPH_SEPARATOR_UTF8)))
          {
             format = "ps";
             n = strlen(_PARAGRAPH_SEPARATOR_UTF8);
          }
        else if (!strncmp(_NEWLINE_UTF8, off, strlen(_NEWLINE_UTF8)))
          {
             format = "br";
             n = strlen(_NEWLINE_UTF8);
          }
        else if (!strncmp(_TAB_UTF8, off, strlen(_TAB_UTF8)))
          {
             format = "tab";
             n = strlen(_TAB_UTF8);
          }

        if (format)
          {
             len += _prepend_text_run2(cur, text, off);
             if (evas_textblock_cursor_format_prepend(cur, format))
               {
                  len++;
               }
             text = off + n; /* sync text with next segment */
          }
          off += n;
     }
   len += _prepend_text_run2(cur, text, off);
   return len;
}

/**
 * @brief Implements the Efl.Text.Cursor.text_insert EAPI.
 *
 * Inserts plain text at the current cursor position. Special characters like
 * newlines are handled.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param text The text to insert.
 */
EOLIAN static void
_efl_text_cursor_object_text_insert(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, const char *text)
{
   _cursor_text_append(pd->handle, text);
}

/**
 * @brief Implements the Efl.Text.Cursor.range_text_get EAPI.
 *
 * Retrieves the plain text within the range defined by two cursors.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param cur2 The cursor marking the end of the range.
 * @return A newly allocated string with the text from the range. The caller
 *         is responsible for freeing this string.
 */
EOLIAN static char *
_efl_text_cursor_object_range_text_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Object *cur2)
{
   return evas_textblock_cursor_range_text_get(pd->handle, efl_text_cursor_object_handle_get(cur2), EVAS_TEXTBLOCK_TEXT_PLAIN);
}

/**
 * @brief Implements the Efl.Text.Cursor.markup_insert EAPI.
 *
 * Inserts text with markup at the current cursor position.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param markup The markup text to insert.
 */
EOLIAN static void
_efl_text_cursor_object_markup_insert(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, const char *markup)
{
   evas_object_textblock_text_markup_prepend(pd->handle, markup);
}

/**
 * @brief Implements the Efl.Text.Cursor.range_markup_get EAPI.
 *
 * Retrieves the markup text within the range defined by two cursors.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param cur2 The cursor marking the end of the range.
 * @return A newly allocated string with the markup from the range. The caller
 *         is responsible for freeing this string.
 */
EOLIAN static char *
_efl_text_cursor_object_range_markup_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Object *cur2)
{
   return evas_textblock_cursor_range_text_get(pd->handle,efl_text_cursor_object_handle_get(cur2), EVAS_TEXTBLOCK_TEXT_MARKUP);
}

/**
 * @brief Implements the Efl.Text.Cursor.range_geometry_get EAPI.
 *
 * Retrieves an iterator over the rectangles that enclose a range of text.
 * This provides a "simple" geometry, which might be a single bounding box.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param cur2 The cursor marking the end of the range.
 * @return An iterator over Eina_Rect objects. The caller is responsible for
 *         freeing the iterator.
 */
EOLIAN static Eina_Iterator *
_efl_text_cursor_object_range_geometry_get(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Object *cur2)
{
   return evas_textblock_cursor_range_simple_geometry_get(pd->handle, efl_text_cursor_object_handle_get(cur2));
}

/** selection iterator */
/**
 * @internal
 * @brief Advances the selection iterator to the next item.
 *
 * This function is part of the Eina_Iterator implementation for textblock selections.
 * It retrieves the data for the current item and moves the iterator to the next one.
 *
 * @param it The selection iterator.
 * @param data A pointer to store the data of the current item (an Eina_Rectangle *).
 * @return @c EINA_TRUE on success, @c EINA_FALSE if there are no more items.
 */
static Eina_Bool
_evas_textblock_selection_iterator_next(Evas_Textblock_Selection_Iterator *it, void **data)
{
   if (!it->current)
     return EINA_FALSE;

   *data = eina_list_data_get(it->current);
   it->current = eina_list_next(it->current);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the selection iterator.
 *
 * This function is part of the Eina_Iterator implementation. It returns the
 * underlying list that the iterator is traversing.
 *
 * @param it The selection iterator.
 * @return The Eina_List that serves as the iterator's container.
 */
static Eina_List *
_evas_textblock_selection_iterator_get_container(Evas_Textblock_Selection_Iterator *it)
{
   return it->list;
}

/**
 * @internal
 * @brief Frees the selection iterator and its associated data.
 *
 * This function is part of the Eina_Iterator implementation. It frees the
 * list of rectangles and the iterator structure itself.
 *
 * @param it The selection iterator to free.
 */
static void
_evas_textblock_selection_iterator_free(Evas_Textblock_Selection_Iterator *it)
{
   Eina_Rectangle *tr;

   EINA_LIST_FREE(it->list, tr)
     free(tr);
   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

/**
 * @internal
 * @brief Creates a new selection iterator for a list of rectangles.
 *
 * This function allocates and initializes a new Eina_Iterator for traversing
 * a list of rectangles, which typically represent the geometry of a text range.
 *
 * @param list The list of Eina_Rectangle pointers.
 * @return A new Eina_Iterator on success, or NULL on allocation failure.
 */
static Eina_Iterator *
_evas_textblock_selection_iterator_new(Eina_List *list)
{
   Evas_Textblock_Selection_Iterator *it;

   it = calloc(1, sizeof(Evas_Textblock_Selection_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->list = list;
   it->current = list;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(
                                     _evas_textblock_selection_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(
                            _evas_textblock_selection_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(
                                     _evas_textblock_selection_iterator_free);

   return &it->iterator;
}

/**
 * @brief Implements the Efl.Text.Cursor.range_precise_geometry_get EAPI.
 *
 * Retrieves an iterator over the rectangles that precisely enclose a range of text.
 * This is useful for complex text layouts where a range might span multiple lines
 * or have disconnected parts.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param cur2 The cursor marking the end of the range.
 * @return An iterator over Eina_Rect objects. The caller is responsible for
 *         freeing the iterator.
 */
EOLIAN static Eina_Iterator *
_efl_text_cursor_object_range_precise_geometry_get(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Object *cur2)
{
   Eina_List *rects = evas_textblock_cursor_range_geometry_get(pd->handle, efl_text_cursor_object_handle_get(cur2));
   return _evas_textblock_selection_iterator_new(rects);
}

/**
 * @brief Implements the Efl.Text.Cursor.range_delete EAPI.
 *
 * Deletes the text and formatting within the range defined by two cursors.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @param cur2 The cursor marking the end of the range to delete.
 */
EOLIAN static void
_efl_text_cursor_object_range_delete(Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd, Efl_Text_Cursor_Object *cur2)
{
   evas_textblock_cursor_range_delete(pd->handle, efl_text_cursor_object_handle_get(cur2));
}

/**
 * @brief Sets the internal Evas textblock cursor handle for a cursor object.
 *
 * This function is used internally to associate an Evas cursor handle with an
 * Efl_Text_Cursor_Object. It manages reference counting for the handles.
 *
 * @param obj The Efl_Text_Cursor_Object.
 * @param handle The Evas textblock cursor handle to set.
 */
EVAS_API void
efl_text_cursor_object_handle_set(Eo *obj, Efl_Text_Cursor_Handle *handle)
{
   Efl_Text_Cursor_Object_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd);
   if (handle == pd->handle)
     return;

   Efl_Text_Cursor_Handle *old_handle = pd->handle;

   pd->handle = evas_textblock_cursor_ref(handle, obj);

   if (old_handle)
     {
        evas_textblock_cursor_unref(old_handle, obj);
     }
}

/**
 * @brief Gets the internal Evas textblock cursor handle from a cursor object.
 *
 * This function provides internal access to the underlying Evas cursor handle.
 *
 * @param obj The Efl_Text_Cursor_Object.
 * @return The Evas textblock cursor handle, or NULL on failure.
 */
EVAS_API Efl_Text_Cursor_Handle *
efl_text_cursor_object_handle_get(const Eo *obj)
{
   Efl_Text_Cursor_Object_Data *pd = efl_data_scope_safe_get(obj, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);
   return pd->handle;
}

/**
 * @brief Creates a new text cursor object.
 *
 * This function is a convenience wrapper around efl_add to create a new
 * instance of Efl_Text_Cursor_Object.
 *
 * @param parent The parent object.
 * @return A new Efl_Text_Cursor_Object, or NULL on failure.
 */
Eo* efl_text_cursor_object_create(Eo *parent)
{
   return efl_add(efl_text_cursor_object_realized_class_get(), parent);
}

/**
 * @brief Associates a cursor object with a text object and creates a new cursor handle.
 *
 * This function connects a cursor to its text object, creating the underlying
 * Evas textblock cursor handle required for operations.
 *
 * @param cursor The cursor object.
 * @param canvas_text_obj The canvas-level text object (e.g., Efl.Canvas.TextBlock).
 * @param text_obj The logical text object (e.g., Efl.Text).
 */
void efl_text_cursor_object_text_object_set(Eo *cursor, Eo *canvas_text_obj, Eo *text_obj)
{
   Efl_Text_Cursor_Object_Data *pd = efl_data_scope_safe_get(cursor, MY_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd);
   Efl_Text_Cursor_Handle *handle = NULL;
   if (efl_isa(canvas_text_obj, EFL_CANVAS_TEXTBLOCK_CLASS))
     {
        pd->text_obj = text_obj;
        handle = evas_object_textblock_cursor_new(canvas_text_obj);
     }
   else
     {
        ERR("Expect Canvas Text Object");
     }

   if (handle)
     {
        efl_text_cursor_object_handle_set(cursor, handle);
        evas_textblock_cursor_unref(handle, NULL);
     }
}

/**
 * @brief Implements the Efl.Text.Cursor.text_object_get EAPI.
 *
 * Retrieves the text object that this cursor belongs to.
 *
 * @param obj The Efl object.
 * @param pd Private data for the cursor object.
 * @return The associated text object.
 */
EOLIAN static Efl_Canvas_Object *
_efl_text_cursor_object_text_object_get(const Eo *obj EINA_UNUSED, Efl_Text_Cursor_Object_Data *pd)
{
   return pd->text_obj;
}

/**
 * @brief Destructor for the Efl_Text_Cursor_Object.
 *
 * Cleans up resources used by the cursor object, such as unreferencing the
 * cursor handle.
 *
 * @param obj The Efl object being destroyed.
 * @param pd Private data for the cursor object.
 */
EOLIAN static void
_efl_text_cursor_object_efl_object_destructor(Eo *obj, Efl_Text_Cursor_Object_Data *pd)
{
   if (pd->handle)
     {
        evas_textblock_cursor_unref(pd->handle, obj);
        pd->handle = NULL;
     }

   if (pd->text_obj)
     {
        pd->text_obj = NULL;
     }

   efl_destructor(efl_super(obj, MY_CLASS));

}

#include "efl_text_cursor_object.eo.c"
