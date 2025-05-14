
/**
 * @brief Set the items grid's alignment within a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] align_x Alignment in the horizontal axis (0 <= align_x <= 1).
 * @param[in] align_y Alignment in the vertical axis (0 <= align_y <= 1).
 */
EAPI void
elm_gengrid_align_set(Elm_Gengrid *obj, double align_x, double align_y)
{
   elm_obj_gengrid_align_set(obj, align_x, align_y);
}

/**
 * @brief Get the items grid's alignment values within a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[out] align_x Alignment in the horizontal axis (0 <= align_x <= 1).
 * @param[out] align_y Alignment in the vertical axis (0 <= align_y <= 1).
 */
EAPI void
elm_gengrid_align_get(const Elm_Gengrid *obj, double *align_x, double *align_y)
{
   elm_obj_gengrid_align_get(obj, align_x, align_y);
}

/**
 * @brief Set how the items grid's filled within a given gengrid widget
 *
 * @param[in] obj The object.
 * @param[in] fill @c true if the grid is filled, @c false otherwise
 */
EAPI void
elm_gengrid_filled_set(Elm_Gengrid *obj, Eina_Bool fill)
{
   elm_obj_gengrid_filled_set(obj, fill);
}

/**
 * @brief Get how the items grid's filled within a given gengrid widget
 *
 * @param[in] obj The object.
 * @return @c true if the grid is filled, @c false otherwise
 */
EAPI Eina_Bool
elm_gengrid_filled_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_filled_get(obj);
}

/**
 * @brief Enable or disable multi-selection in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] multi @c true if multislect is enabled, @c false otherwise
 */
EAPI void
elm_gengrid_multi_select_set(Elm_Gengrid *obj, Eina_Bool multi)
{
   elm_obj_gengrid_multi_select_set(obj, multi);
}

/**
 * @brief Get whether multi-selection is enabled or disabled for a given
 * gengrid widget.
 *
 * @param[in] obj The object.
 * @return @c true if multislect is enabled, @c false otherwise
 */
EAPI Eina_Bool
elm_gengrid_multi_select_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_multi_select_get(obj);
}

/**
 * @brief Set the size for the group items of a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] w The group items' width.
 * @param[in] h The group items' height.
 */
EAPI void
elm_gengrid_group_item_size_set(Elm_Gengrid *obj, int w, int h)
{
   elm_obj_gengrid_group_item_size_set(obj, w, h);
}

/**
 * @brief Get the size set for the group items of a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[out] w The group items' width.
 * @param[out] h The group items' height.
 */
EAPI void
elm_gengrid_group_item_size_get(const Elm_Gengrid *obj, int *w, int *h)
{
   elm_obj_gengrid_group_item_size_get(obj, w, h);
}

/**
 * @brief Set the gengrid select mode.
 *
 * @param[in] obj The object.
 * @param[in] mode The select mode.
 *        Example: ELM_OBJECT_SELECT_MODE_ALWAYS
 */
EAPI void
elm_gengrid_select_mode_set(Elm_Gengrid *obj, Elm_Object_Select_Mode mode)
{
   elm_obj_gengrid_select_mode_set(obj, mode);
}

/**
 * @brief Get the gengrid select mode.
 *
 * @param[in] obj The object.
 * @return The select mode.
 *         Example: ELM_OBJECT_SELECT_MODE_DEFAULT
 */
EAPI Elm_Object_Select_Mode
elm_gengrid_select_mode_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_select_mode_get(obj);
}

/**
 * @brief Set whether a given gengrid widget is or not able have items
 * reordered.
 *
 * @param[in] obj The object.
 * @param[in] reorder_mode Use @c true to turn reordering on, @c false to turn
 * it off.
 */
EAPI void
elm_gengrid_reorder_mode_set(Elm_Gengrid *obj, Eina_Bool reorder_mode)
{
   elm_obj_gengrid_reorder_mode_set(obj, reorder_mode);
}

/**
 * @brief Get whether a given gengrid widget is or not able have items
 * reordered.
 *
 * @param[in] obj The object.
 * @return Use @c true to turn reordering on, @c false to turn it off.
 */
EAPI Eina_Bool
elm_gengrid_reorder_mode_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_reorder_mode_get(obj);
}

/**
 * @brief Control whether the gengrid items' should be highlighted when item
 * selected.
 *
 * @param[in] obj The object.
 * @param[in] highlight @c true if item will be highlighted, @c false otherwise
 */
EAPI void
elm_gengrid_highlight_mode_set(Elm_Gengrid *obj, Eina_Bool highlight)
{
   elm_obj_gengrid_highlight_mode_set(obj, highlight);
}

/**
 * @brief Control whether the gengrid items' should be highlighted when item
 * selected.
 *
 * @param[in] obj The object.
 * @return @c true if item will be highlighted, @c false otherwise
 */
EAPI Eina_Bool
elm_gengrid_highlight_mode_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_highlight_mode_get(obj);
}

/**
 * @brief Set the Gengrid reorder type
 *
 * @param[in] obj The object.
 * @param[in] type Reorder type value
 *        Example: ELM_GENGRID_REORDER_TYPE_SWAP
 * @since 1.11
 */
EAPI void
elm_gengrid_reorder_type_set(Elm_Gengrid *obj, Elm_Gengrid_Reorder_Type type)
{
   elm_obj_gengrid_reorder_type_set(obj, type);
}

/**
 * @brief Set the size for the items of a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] w The items' width.
 * @param[in] h The items' height.
 */
EAPI void
elm_gengrid_item_size_set(Elm_Gengrid *obj, int w, int h)
{
   elm_obj_gengrid_item_size_set(obj, w, h);
}

/**
 * @brief Get the size set for the items of a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[out] w The items' width.
 * @param[out] h The items' height.
 */
EAPI void
elm_gengrid_item_size_get(const Elm_Gengrid *obj, int *w, int *h)
{
   elm_obj_gengrid_item_size_get(obj, w, h);
}

/**
 * @brief Set the gengrid multi select mode.
 *
 * @param[in] obj The object.
 * @param[in] mode The multi select mode.
 *        Example: ELM_OBJECT_MULTI_SELECT_MODE_WITH_CONTROL
 * @since 1.8
 */
EAPI void
elm_gengrid_multi_select_mode_set(Elm_Gengrid *obj, Elm_Object_Multi_Select_Mode mode)
{
   elm_obj_gengrid_multi_select_mode_set(obj, mode);
}

/**
 * @brief Get the gengrid multi select mode.
 *
 * @param[in] obj The object.
 * @return The multi select mode.
 *         Example: ELM_OBJECT_MULTI_SELECT_MODE_DEFAULT
 * @since 1.8
 */
EAPI Elm_Object_Multi_Select_Mode
elm_gengrid_multi_select_mode_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_multi_select_mode_get(obj);
}

/**
 * @brief Set the direction in which a given gengrid widget will expand while
 * placing its items.
 *
 * @param[in] obj The object.
 * @param[in] horizontal @c true to make the gengrid expand horizontally,
 * @c false to expand vertically.
 */
EAPI void
elm_gengrid_horizontal_set(Elm_Gengrid *obj, Eina_Bool horizontal)
{
   elm_obj_gengrid_horizontal_set(obj, horizontal);
}

/**
 * @brief Get for what direction a given gengrid widget will expand while
 * placing its items.
 *
 * @param[in] obj The object.
 * @return @c true to make the gengrid expand horizontally, @c false to expand
 * vertically.
 */
EAPI Eina_Bool
elm_gengrid_horizontal_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_horizontal_get(obj);
}

/**
 * @brief Get the selected item in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @return The selected item's handle or @c null if none is selected.
 */
EAPI Elm_Widget_Item *
elm_gengrid_selected_item_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_selected_item_get(obj);
}

/**
 * @brief Get a list of realized items in gengrid.
 *
 * @param[in] obj The object.
 * @return The list of realized items or @c null if none are realized.
 *         The list must be freed by the caller using eina_list_free().
 *         Example of list structure:
 *         Eina_List* list = elm_gengrid_realized_items_get(gengrid);
 *         Elm_Widget_Item* item;
 *         EINA_LIST_FOREACH(list, l, item) {
 *           // process item
 *         }
 *         eina_list_free(list);
 */
EAPI Eina_List *
elm_gengrid_realized_items_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_realized_items_get(obj);
}

/**
 * @brief Get the first item in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @return The first item's handle or @c null if there are no items.
 */
EAPI Elm_Widget_Item *
elm_gengrid_first_item_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_first_item_get(obj);
}

/**
 * @brief Get a list of selected items in a given gengrid.
 *
 * @param[in] obj The object.
 * @return The list of selected items or @c null if none is selected.
 *         The list is valid as long as no items are selected/unselected.
 *         Example of list structure:
 *         const Eina_List* list = elm_gengrid_selected_items_get(gengrid);
 *         const Elm_Widget_Item* item;
 *         const Eina_List *l;
 *         EINA_LIST_FOREACH(list, l, item) {
 *           // process item
 *         }
 */
EAPI const Eina_List *
elm_gengrid_selected_items_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_selected_items_get(obj);
}

/**
 * @brief Get the last item in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @return The last item's handle or @c null if there are no items.
 */
EAPI Elm_Widget_Item *
elm_gengrid_last_item_get(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_last_item_get(obj);
}

/**
 * @brief Insert an item before another in a gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item data.
 * @param[in] relative The item to place this new one before.
 * @param[in] func Convenience function called when the item is selected.
 * @param[in] func_data Data to be passed to @c func.
 * @return A handle to the item added or @c null on errors.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_insert_before(Elm_Gengrid *obj, const Elm_Gengrid_Item_Class *itc, const void *data, Elm_Widget_Item *relative, Evas_Smart_Cb func, const void *func_data)
{
   return elm_obj_gengrid_item_insert_before(obj, itc, data, relative, func, func_data);
}

/**
 * @brief Update the contents of all realized items.
 *
 * @param[in] obj The object.
 */
EAPI void
elm_gengrid_realized_items_update(Elm_Gengrid *obj)
{
   elm_obj_gengrid_realized_items_update(obj);
}

/**
 * @brief Insert an item after another in a gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item data.
 * @param[in] relative The item to place this new one after.
 * @param[in] func Convenience function called when the item is selected.
 * @param[in] func_data Data to be passed to @c func.
 * @return A handle to the item added or @c null on error.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_insert_after(Elm_Gengrid *obj, const Elm_Gengrid_Item_Class *itc, const void *data, Elm_Widget_Item *relative, Evas_Smart_Cb func, const void *func_data)
{
   return elm_obj_gengrid_item_insert_after(obj, itc, data, relative, func, func_data);
}

/**
 * @brief Return how many items are currently in a list
 *
 * @param[in] obj The object.
 * @return Items in list
 */
EAPI unsigned int
elm_gengrid_items_count(const Elm_Gengrid *obj)
{
   return elm_obj_gengrid_items_count(obj);
}

/**
 * @brief Get the item that is at the x, y canvas coords.
 *
 * @param[in] obj The object.
 * @param[in] x The input x coordinate.
 * @param[in] y The input y coordinate.
 * @param[out] xposret The position relative to the item returned here (-1, 0, or 1).
 * @param[out] yposret The position relative to the item returned here (-1, 0, or 1).
 * @return The item at the coordinates or @c null if none.
 */
EAPI Elm_Widget_Item *
elm_gengrid_at_xy_item_get(const Elm_Gengrid *obj, int x, int y, int *xposret, int *yposret)
{
   return elm_obj_gengrid_at_xy_item_get(obj, x, y, xposret, yposret);
}

/**
 * @brief Append a new item in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item data.
 * @param[in] func Convenience function called when the item is selected.
 * @param[in] func_data Data to be passed to @c func.
 * @return A handle to the item added or @c null on errors.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_append(Elm_Gengrid *obj, const Elm_Gengrid_Item_Class *itc, const void *data, Evas_Smart_Cb func, const void *func_data)
{
   return elm_obj_gengrid_item_append(obj, itc, data, func, func_data);
}

/**
 * @brief Prepend a new item in a given gengrid widget.
 *
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item data.
 * @param[in] func Convenience function called when the item is selected.
 * @param[in] func_data Data to be passed to @c func.
 * @return A handle to the item added or @c null on errors.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_prepend(Elm_Gengrid *obj, const Elm_Gengrid_Item_Class *itc, const void *data, Evas_Smart_Cb func, const void *func_data)
{
   return elm_obj_gengrid_item_prepend(obj, itc, data, func, func_data);
}

/**
 * @brief Remove all items from a given gengrid widget.
 *
 * @param[in] obj The object.
 */
EAPI void
elm_gengrid_clear(Elm_Gengrid *obj)
{
   elm_obj_gengrid_clear(obj);
}

/**
 * @brief Insert an item in a gengrid widget using a user-defined sort
 * function.
 *
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item data.
 * @param[in] comp User defined comparison function that defines the sort order.
 * @param[in] func Convenience function called when the item is selected.
 * @param[in] func_data Data to be passed to @c func.
 * @return A handle to the item added or @c null on errors.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_sorted_insert(Elm_Gengrid *obj, const Elm_Gengrid_Item_Class *itc, const void *data, Eina_Compare_Cb comp, Evas_Smart_Cb func, const void *func_data)
{
   return elm_obj_gengrid_item_sorted_insert(obj, itc, data, comp, func, func_data);
}

/**
 * @brief Get gengrid item by given string.
 *
 * @param[in] obj The object.
 * @param[in] item_to_search_from Pointer to item to start search from. If
 * @c null, search will be started from the first item of the gengrid.
 * @param[in] part_name Name of the TEXT part of gengrid item to search string
 * in. If @c null, search by "elm.text" parts.
 * @param[in] pattern The search pattern (glob-style, e.g., "*.jpg").
 * @param[in] flags Search flags (e.g., ELM_GLOB_MATCH_CASEFOLD).
 * @return Pointer to the gengrid item which matches search_string in case of
 * success, otherwise @c null.
 * @since 1.11
 */
EAPI Elm_Widget_Item *
elm_gengrid_search_by_text_item_get(Elm_Gengrid *obj, Elm_Widget_Item *item_to_search_from, const char *part_name, const char *pattern, Elm_Glob_Match_Flags flags)
{
   return elm_obj_gengrid_search_by_text_item_get(obj, item_to_search_from, part_name, pattern, flags);
}

/**
 * @brief Starts the reorder mode of Gengrid
 *
 * @param[in] obj The object.
 * @param[in] tween_mode Position mappings for animation (e.g., ECORE_POS_MAP_LINEAR).
 * @since 1.10
 */
EAPI void
elm_gengrid_reorder_mode_start(Elm_Gengrid *obj, Ecore_Pos_Map tween_mode)
{
   elm_obj_gengrid_reorder_mode_start(obj, tween_mode);
}

/**
 * @brief Stops the reorder mode of Gengrid
 *
 * @param[in] obj The object.
 * @since 1.10
 */
EAPI void
elm_gengrid_reorder_mode_stop(Elm_Gengrid *obj)
{
   elm_obj_gengrid_reorder_mode_stop(obj);
}
