//#define EFL_BETA_API_SUPPORT
#include "evas_common_private.h"
#include "evas_private.h"
#include "efl_canvas_textblock_internal.h"
#include "efl_text_cursor_object.eo.h"

#define MY_CLASS EFL_TEXT_FORMATTER_CLASS

typedef struct
{

} Efl_Text_Formatter_Data;

/**
 * @brief Inserts a new text attribute (annotation) spanning the given range.
 *
 * This function applies a format string (e.g., "font_size=12 color=#FF0000")
 * to the text between the start and end cursors.
 *
 * @param[in] start The cursor marking the beginning of the range.
 * @param[in] end The cursor marking the end of the range.
 * @param[in] format The format string to apply.
 *                   Example: "font_size=10 font=Sans style=bold"
 */
EOLIAN static void
_efl_text_formatter_attribute_insert(const Efl_Text_Cursor_Object *start, const Efl_Text_Cursor_Object *end, const char *format)
{
   EINA_SAFETY_ON_TRUE_RETURN(!efl_text_cursor_object_handle_get(start) ||
                              !efl_text_cursor_object_handle_get(end) ||
                               efl_text_cursor_object_handle_get(start)->obj != efl_text_cursor_object_handle_get(end)->obj);

   Eo *eo_obj= efl_text_cursor_object_handle_get(start)->obj;
   evas_textblock_async_block(eo_obj);

   _evas_textblock_annotations_insert(eo_obj, efl_text_cursor_object_handle_get(start), efl_text_cursor_object_handle_get(end), format,
         EINA_FALSE);
   efl_event_callback_legacy_call(eo_obj, EFL_CANVAS_TEXTBLOCK_EVENT_CHANGED, NULL);
}

/**
 * @brief Clears all text attributes within the specified range.
 *
 * This function removes all formatting annotations that overlap
 * with the range defined by the start and end cursors.
 *
 * @param[in] start The cursor marking the beginning of the range.
 * @param[in] end The cursor marking the end of the range.
 * @return The number of attributes cleared.
 */
EOLIAN static unsigned int
_efl_text_formatter_attribute_clear(const Efl_Text_Cursor_Object *start, const Efl_Text_Cursor_Object *end)
{
   unsigned int ret = 0;
   Eina_Iterator *annotations;
   Efl_Text_Attribute_Handle *an;
   annotations = efl_text_formatter_range_attributes_get(start, end);

   if (!annotations) return ret;

   EINA_ITERATOR_FOREACH(annotations, an)
     {
        ret++;
        efl_text_formatter_remove(an);
     }
   eina_iterator_free(annotations);

   return ret;
}

/**
 * @brief Retrieves the format string of a given text attribute.
 *
 * @param[in] annotation The handle to the text attribute.
 * @return The format string associated with the attribute (e.g., "font_size=12"),
 *         or @c NULL if the annotation is invalid or has no format.
 */
const char *
efl_text_formatter_attribute_get(Efl_Text_Attribute_Handle *annotation)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!annotation || !(annotation->obj), NULL);

   return (annotation->start_node ? annotation->start_node->format : NULL);
}

/**
 * @brief Retrieves an iterator over all text attributes that overlap with the given range.
 *
 * The returned iterator will yield Efl_Text_Attribute_Handle pointers.
 *
 * @param[in] start The cursor marking the beginning of the range.
 * @param[in] end The cursor marking the end of the range.
 * @return An Eina_Iterator for Efl_Text_Attribute_Handle items, or @c NULL on failure.
 *         The caller is responsible for freeing the iterator using eina_iterator_free().
 */
Eina_Iterator *
efl_text_formatter_range_attributes_get(const Efl_Text_Cursor_Object *start, const Efl_Text_Cursor_Object *end)
{
   Eina_List *lst = NULL;
   Efl_Text_Attribute_Handle *it;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(!efl_text_cursor_object_handle_get(start) ||
                              !efl_text_cursor_object_handle_get(end) ||
                              efl_text_cursor_object_handle_get(start)->obj != efl_text_cursor_object_handle_get(end)->obj, NULL);

   Eina_Inlist *annotations = _evas_textblock_annotations_get(efl_text_cursor_object_handle_get(start)->obj);

   EINA_INLIST_FOREACH(annotations, it)
     {
        Efl_Text_Cursor_Handle start2, end2;
        _evas_textblock_cursor_init(&start2, efl_text_cursor_object_handle_get(start)->obj);
        _evas_textblock_cursor_init(&end2, efl_text_cursor_object_handle_get(start)->obj);

        if (!it->start_node || !it->end_node) continue;
        _textblock_cursor_pos_at_fnode_set(&start2, it->start_node);
        _textblock_cursor_pos_at_fnode_set(&end2, it->end_node);
        evas_textblock_cursor_char_prev(&end2);
        if (!((evas_textblock_cursor_compare(&start2, efl_text_cursor_object_handle_get(end)) > 0) ||
                 (evas_textblock_cursor_compare(&end2, efl_text_cursor_object_handle_get(start)) < 0)))
          {
             lst = eina_list_append(lst, it);
          }
     }
   return _evas_textblock_annotation_iterator_new(lst);
}

/**
 * @brief Retrieves the start and end cursors for a given text attribute.
 *
 * This function populates the provided start and end cursor objects
 * to represent the range covered by the specified attribute handle.
 *
 * @param[in] handle The handle to the text attribute.
 * @param[out] start A cursor object to be populated with the start position of the attribute.
 * @param[out] end A cursor object to be populated with the end position of the attribute.
 */
void
efl_text_formatter_attribute_cursors_get(const Efl_Text_Attribute_Handle *handle, Efl_Text_Cursor_Object *start, Efl_Text_Cursor_Object *end)
{
   EINA_SAFETY_ON_TRUE_RETURN (!handle || !(handle->obj));

   efl_text_cursor_object_text_object_set(start, handle->obj, handle->obj);
   efl_text_cursor_object_text_object_set(end, handle->obj, handle->obj);
   _textblock_cursor_pos_at_fnode_set(efl_text_cursor_object_handle_get(start), handle->start_node);
   _textblock_cursor_pos_at_fnode_set(efl_text_cursor_object_handle_get(end), handle->end_node);
}

/**
 * @brief Removes a specific text attribute.
 *
 * @param[in] annotation The handle to the text attribute to remove.
 */
void
efl_text_formatter_remove(Efl_Text_Attribute_Handle *annotation)
{
   EINA_SAFETY_ON_TRUE_RETURN (!annotation || !(annotation->obj));

   evas_textblock_async_block(annotation->obj);
   _evas_textblock_annotation_remove(annotation->obj, NULL, annotation, EINA_TRUE, EINA_TRUE);
}

/**
 * @brief Checks if a given text attribute represents an item (e.g., an embedded object).
 *
 * Items are special types of annotations that often represent non-text content
 * embedded within the text, like images or custom UI elements.
 *
 * @param[in] annotation The handle to the text attribute.
 * @return @c EINA_TRUE if the attribute is an item, @c EINA_FALSE otherwise.
 */
Eina_Bool
efl_text_formatter_attribute_is_item(Efl_Text_Attribute_Handle *annotation)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!annotation || !(annotation->obj), EINA_FALSE);

   return annotation->is_item;
}

/**
 * @brief Retrieves the geometry of an item-type text attribute.
 *
 * If the given attribute handle represents an item (see efl_text_formatter_attribute_is_item()),
 * this function populates the provided pointers with the item's position (x, y)
 * and size (w, h) relative to the textblock object.
 *
 * @param[in] annotation The handle to the item attribute.
 * @param[out] x Pointer to store the x-coordinate of the item. Can be @c NULL.
 * @param[out] y Pointer to store the y-coordinate of the item. Can be @c NULL.
 * @param[out] w Pointer to store the width of the item. Can be @c NULL.
 * @param[out] h Pointer to store the height of the item. Can be @c NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE if the attribute is not an item,
 *         is invalid, or if geometry calculation fails.
 */
Eina_Bool
efl_text_formatter_item_geometry_get(const Efl_Text_Attribute_Handle *annotation, int *x, int *y, int *w, int *h)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL(!annotation || !(annotation->obj), EINA_FALSE);

   Efl_Text_Cursor_Handle cur;

   Eo *eo_obj = annotation->obj;
   Evas_Object_Protected_Data *obj_data = efl_data_scope_safe_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj_data, EINA_FALSE);
   evas_object_async_block(obj_data);
   _evas_textblock_relayout_if_needed(eo_obj);

   _evas_textblock_cursor_init(&cur, eo_obj);
   _textblock_cursor_pos_at_fnode_set(&cur, annotation->start_node);
   return evas_textblock_cursor_format_item_geometry_get(&cur, x, y, w, h);
}

#include "efl_text_formatter.eo.c"
