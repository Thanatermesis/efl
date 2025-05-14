
/**
 * @brief Get the item before @c item in diskselector.
 * @param[in] obj The object.
 * @return The item before @c item, or @c null if none or on failure.
 * @see elm_diskselector_item_prev_get() in elm_diskselector_item_eo.legacy.h for more details.
 */
EAPI Elm_Widget_Item *
elm_diskselector_item_prev_get(const Elm_Diskselector_Item *obj)
{
   return elm_obj_diskselector_item_prev_get(obj);
}

/**
 * @brief Get the item after @c item in diskselector.
 * @param[in] obj The object.
 * @return The item after @c item, or @c null if none or on failure.
 * @see elm_diskselector_item_next_get() in elm_diskselector_item_eo.legacy.h for more details.
 */
EAPI Elm_Widget_Item *
elm_diskselector_item_next_get(const Elm_Diskselector_Item *obj)
{
   return elm_obj_diskselector_item_next_get(obj);
}

/**
 * @brief Set the selected state of an item.
 * @param[in] obj The object.
 * @param[in] selected The selected state.
 * @see elm_diskselector_item_selected_set() in elm_diskselector_item_eo.legacy.h for more details.
 */
EAPI void
elm_diskselector_item_selected_set(Elm_Diskselector_Item *obj, Eina_Bool selected)
{
   elm_obj_diskselector_item_selected_set(obj, selected);
}

/**
 * @brief Get whether the @c item is selected or not.
 * @param[in] obj The object.
 * @return The selected state.
 * @see elm_diskselector_item_selected_get() in elm_diskselector_item_eo.legacy.h for more details.
 */
EAPI Eina_Bool
elm_diskselector_item_selected_get(const Elm_Diskselector_Item *obj)
{
   return elm_obj_diskselector_item_selected_get(obj);
}
