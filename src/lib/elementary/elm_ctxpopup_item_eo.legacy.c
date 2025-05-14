/**
 * @brief Get the item before this one in the widget's list of items.
 *
 * See also @ref elm_ctxpopup_item_next_get.
 *
 * @param[in] obj The object.
 *
 * @return The item before the object in its parent's list. If there is no
 * previous item or in case of error, @c null is returned.
 *
 * @ingroup Elm_Ctxpopup_Item_Group
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_prev_get(const Elm_Ctxpopup_Item *obj)
{
   return elm_obj_ctxpopup_item_prev_get(obj);
}

/**
 * @brief Get the item after this one in the widget's list of items.
 *
 * See also @ref elm_ctxpopup_item_prev_get.
 *
 * @param[in] obj The object.
 *
 * @return The item after the object in its parent's list. If there is no next
 * item or in case of error, @c null is returned.
 *
 * @ingroup Elm_Ctxpopup_Item_Group
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_next_get(const Elm_Ctxpopup_Item *obj)
{
   return elm_obj_ctxpopup_item_next_get(obj);
}

/**
 * @brief Indicates whether this item is currently selected.
 *
 * Set the selected state of @c item.
 *
 * This sets the selected state of the given item @c it. @c true for selected,
 * @c false for not selected.
 *
 * If a new item is selected the previously selected will be unselected.
 * Previously selected item can be retrieved with
 * @ref elm_ctxpopup_selected_item_get.
 *
 * Selected items will be highlighted.
 *
 * @param[in] obj The object.
 * @param[in] selected The selection state.
 *
 * @ingroup Elm_Ctxpopup_Item_Group
 */
EAPI void
elm_ctxpopup_item_selected_set(Elm_Ctxpopup_Item *obj, Eina_Bool selected)
{
   elm_obj_ctxpopup_item_selected_set(obj, selected);
}

/**
 * @brief Indicates whether this item is currently selected.
 *
 * Get the selected state of this item.
 *
 * @param[in] obj The object.
 *
 * @return The selection state.
 *
 * @ingroup Elm_Ctxpopup_Item_Group
 */
EAPI Eina_Bool
elm_ctxpopup_item_selected_get(const Elm_Ctxpopup_Item *obj)
{
   return elm_obj_ctxpopup_item_selected_get(obj);
}

/**
 * @brief Init context popup item
 *
 * @param[in] obj The object.
 * @param[in] func Smart callback function
 * @param[in] data Data pointer
 *
 * @ingroup Elm_Ctxpopup_Item_Group
 */
EAPI void
elm_ctxpopup_item_init(Elm_Ctxpopup_Item *obj, Evas_Smart_Cb func, const void *data)
{
   elm_obj_ctxpopup_item_init(obj, func, data);
}
