/**
 * @brief Control the selected state of an item.
 *
 * This function sets the selected state of the given Elm_Multibuttonentry_Item.
 *
 * @param[in] obj The multibuttonentry item object.
 * @param[in] selected @c EINA_TRUE if the item is to be selected, @c EINA_FALSE otherwise.
 */
EAPI void
elm_multibuttonentry_item_selected_set(Elm_Multibuttonentry_Item *obj, Eina_Bool selected)
{
   elm_obj_multibuttonentry_item_selected_set(obj, selected);
}

/**
 * @brief Get the selected state of an item.
 *
 * This function retrieves the selected state of the given Elm_Multibuttonentry_Item.
 *
 * @param[in] obj The multibuttonentry item object.
 * @return @c EINA_TRUE if the item is selected, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_multibuttonentry_item_selected_get(const Elm_Multibuttonentry_Item *obj)
{
   return elm_obj_multibuttonentry_item_selected_get(obj);
}

/**
 * @brief Get the previous item in the multibuttonentry.
 *
 * This function returns the item that appears before the given item in the
 * multibuttonentry widget.
 *
 * @param[in] obj The multibuttonentry item object.
 * @return A pointer to the previous Elm_Widget_Item, or @c NULL if there is no previous item.
 */
EAPI Elm_Widget_Item *
elm_multibuttonentry_item_prev_get(const Elm_Multibuttonentry_Item *obj)
{
   return elm_obj_multibuttonentry_item_prev_get(obj);
}

/**
 * @brief Get the next item in the multibuttonentry.
 *
 * This function returns the item that appears after the given item in the
 * multibuttonentry widget.
 *
 * @param[in] obj The multibuttonentry item object.
 * @return A pointer to the next Elm_Widget_Item, or @c NULL if there is no next item.
 */
EAPI Elm_Widget_Item *
elm_multibuttonentry_item_next_get(const Elm_Multibuttonentry_Item *obj)
{
   return elm_obj_multibuttonentry_item_next_get(obj);
}
