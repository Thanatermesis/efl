/**
 * @file
 * @brief Legacy C source for Elm_Index_Item EO API.
 *
 * These functions are wrappers for the new EO API.
 * @deprecated Use the new EO API directly.
 */

/**
 * @brief Legacy wrapper for elm_obj_index_item_selected_set().
 * @deprecated Use elm_obj_index_item_selected_set() instead.
 * @see elm_index_item_selected_set() in elm_index_item_eo.legacy.h for full API documentation.
 */
EAPI void
elm_index_item_selected_set(Elm_Index_Item *obj, Eina_Bool selected)
{
   elm_obj_index_item_selected_set(obj, selected);
}

/**
 * @brief Legacy wrapper for elm_obj_index_item_priority_set().
 * @deprecated Use elm_obj_index_item_priority_set() instead.
 * @see elm_index_item_priority_set() in elm_index_item_eo.legacy.h for full API documentation.
 */
EAPI void
elm_index_item_priority_set(Elm_Index_Item *obj, int priority)
{
   elm_obj_index_item_priority_set(obj, priority);
}

/**
 * @brief Legacy wrapper for elm_obj_index_item_letter_get().
 * @deprecated Use elm_obj_index_item_letter_get() instead.
 * @see elm_index_item_letter_get() in elm_index_item_eo.legacy.h for full API documentation.
 */
EAPI const char *
elm_index_item_letter_get(const Elm_Index_Item *obj)
{
   return elm_obj_index_item_letter_get(obj);
}
