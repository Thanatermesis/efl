/**
 * @internal
 * @brief Set or unset item as a separator.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_separator_set().
 * Items aren't set as separator by default.
 * If set as separator it will display separator theme, so won't display icons
 * or label.
 *
 * @param[in] obj The object.
 * @param[in] setting @c EINA_TRUE means item @c it is a separator. @c EINA_FALSE
 * indicates it's not.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI void
elm_list_item_separator_set(Elm_List_Item *obj, Eina_Bool setting)
{
   elm_obj_list_item_separator_set(obj, setting);
}

/**
 * @internal
 * @brief Get a value whether item is a separator or not.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_separator_get().
 *
 * @param[in] obj The object.
 *
 * @return @c EINA_TRUE means item @c it is a separator. @c EINA_FALSE indicates it's
 * not.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI Eina_Bool
elm_list_item_separator_get(const Elm_List_Item *obj)
{
   return elm_obj_list_item_separator_get(obj);
}

/**
 * @internal
 * @brief Set the selected state of an item.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_selected_set().
 * This sets the selected state of the given item.
 * If a new item is selected the previously selected will be unselected, unless
 * multiple selection is enabled with elm_list_multi_select_set().
 * Previously selected item can be get with function elm_list_selected_item_get().
 * Selected items will be highlighted.
 *
 * @param[in] obj The object.
 * @param[in] selected The selected state (@c EINA_TRUE or @c EINA_FALSE).
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI void
elm_list_item_selected_set(Elm_List_Item *obj, Eina_Bool selected)
{
   elm_obj_list_item_selected_set(obj, selected);
}

/**
 * @internal
 * @brief Get whether the item is selected or not.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_selected_get().
 *
 * @param[in] obj The object.
 *
 * @return The selected state (@c EINA_TRUE or @c EINA_FALSE).
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI Eina_Bool
elm_list_item_selected_get(const Elm_List_Item *obj)
{
   return elm_obj_list_item_selected_get(obj);
}

/**
 * @internal
 * @brief Get the real Evas(Edje) object created to implement the view of a
 * given list item.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_object_get().
 * Base object is the @c Evas_Object that represents that item.
 *
 * @param[in] obj The object.
 *
 * @return The base Edje object associated with the item.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI Efl_Canvas_Object *
elm_list_item_object_get(const Elm_List_Item *obj)
{
   return elm_obj_list_item_object_get(obj);
}

/**
 * @internal
 * @brief Get the item before the item in list.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_prev_get().
 * @note If it is the first item, @c null will be returned.
 *
 * @param[in] obj The object.
 *
 * @return The item before or @c null.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI Elm_Widget_Item *
elm_list_item_prev(const Elm_List_Item *obj)
{
   return elm_obj_list_item_prev_get(obj);
}

/**
 * @internal
 * @brief Get the item after the item in list.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_next_get().
 * @note If it is the last item, @c null will be returned.
 *
 * @param[in] obj The object.
 *
 * @return The item after or @c null.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI Elm_Widget_Item *
elm_list_item_next(const Elm_List_Item *obj)
{
   return elm_obj_list_item_next_get(obj);
}

/**
 * @internal
 * @brief Show item in the list view.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_show().
 * It won't animate list until item is visible. If such behavior is wanted, use
 * elm_list_item_bring_in() instead.
 * @param[in] obj The object.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI void
elm_list_item_show(Elm_List_Item *obj)
{
   elm_obj_list_item_show(obj);
}

/**
 * @internal
 * @brief Bring in the given item to list view.
 *
 * @details This is a legacy wrapper for elm_obj_list_item_bring_in().
 * This causes list to jump to the given item and show it (by scrolling), if it
 * is not fully visible.
 * This may use animation to do so and take a period of time.
 * If animation isn't wanted, elm_list_item_show() can be used.
 * @param[in] obj The object.
 *
 * @ingroup Elm_List_Item_Group
 */
EAPI void
elm_list_item_bring_in(Elm_List_Item *obj)
{
   elm_obj_list_item_bring_in(obj);
}
