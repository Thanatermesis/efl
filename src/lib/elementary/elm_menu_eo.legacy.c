
EAPI Elm_Widget_Item *
elm_menu_selected_item_get(const Elm_Menu *obj)
{
   return elm_obj_menu_selected_item_get(obj);
}

/**
 * @brief Get the first item in the widget.
 * @param[in] obj The object.
 * @return The first item or @c NULL.
 * @see elm_obj_menu_first_item_get()
 */
EAPI Elm_Widget_Item *
elm_menu_first_item_get(const Elm_Menu *obj)
{
   return elm_obj_menu_first_item_get(obj);
}

/**
 * @brief Get the last item in the widget.
 * @param[in] obj The object.
 * @return The last item or @c NULL.
 * @see elm_obj_menu_last_item_get()
 */
EAPI Elm_Widget_Item *
elm_menu_last_item_get(const Elm_Menu *obj)
{
   return elm_obj_menu_last_item_get(obj);
}

/**
 * @brief Returns a list of the widget item.
 * @param[in] obj The object.
 * @return Const list to widget items.
 * @see elm_obj_menu_items_get()
 */
EAPI const Eina_List *
elm_menu_items_get(const Elm_Menu *obj)
{
   return elm_obj_menu_items_get(obj);
}

/**
 * @brief Move the menu to a new position.
 * @param[in] obj The object.
 * @param[in] x The new X coordinate.
 * @param[in] y The new Y coordinate.
 * @see elm_obj_menu_relative_move()
 */
EAPI void
elm_menu_move(Elm_Menu *obj, int x, int y)
{
   elm_obj_menu_relative_move(obj, x, y);
}

/**
 * @brief Add an item at the end of the given menu widget.
 * @param[in] obj The object.
 * @param[in] parent The parent menu item (optional).
 * @param[in] icon An icon display on the item.
 * @param[in] label The label of the item.
 * @param[in] func Function called when the user select the item.
 * @param[in] data Data sent by the callback.
 * @return The new menu item.
 * @see elm_obj_menu_item_add()
 */
EAPI Elm_Widget_Item *
elm_menu_item_add(Elm_Menu *obj, Elm_Widget_Item *parent, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_menu_item_add(obj, parent, icon, label, func, data);
}

/**
 * @brief Open a closed menu.
 * @param[in] obj The object.
 * @see elm_obj_menu_open()
 */
EAPI void
elm_menu_open(Elm_Menu *obj)
{
   elm_obj_menu_open(obj);
}

/**
 * @brief Close a opened menu.
 * @param[in] obj The object.
 * @see elm_obj_menu_close()
 */
EAPI void
elm_menu_close(Elm_Menu *obj)
{
   elm_obj_menu_close(obj);
}

/**
 * @brief Add a separator item to menu.
 * @param[in] obj The object.
 * @param[in] parent The item to add the separator under.
 * @return The created item or @c NULL.
 * @see elm_obj_menu_item_separator_add()
 */
EAPI Elm_Widget_Item *
elm_menu_item_separator_add(Elm_Menu *obj, Elm_Widget_Item *parent)
{
   return elm_obj_menu_item_separator_add(obj, parent);
}
