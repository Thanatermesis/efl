
/**
 * @brief Get the selected item in the widget.
 * @param[in] obj The object.
 * @return The selected item or @c null.
 */
EAPI Elm_Widget_Item *
elm_toolbar_selected_item_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_selected_item_get(obj);
}

/**
 * @brief Get the first item in the widget.
 * @param[in] obj The object.
 * @return The first item or @c null.
 */
EAPI Elm_Widget_Item *
elm_toolbar_first_item_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_first_item_get(obj);
}

/**
 * @brief Get the last item in the widget.
 * @param[in] obj The object.
 * @return The last item or @c null.
 */
EAPI Elm_Widget_Item *
elm_toolbar_last_item_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_last_item_get(obj);
}

/**
 * @brief Returns a list of the widget item.
 * @param[in] obj The object.
 * @return iterator to widget items
 */
EAPI Eina_Iterator *
elm_toolbar_items_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_items_get(obj);
}

/**
 * @brief Control homogeneous mode.
 * @details This will enable the homogeneous mode where items are of the same size.
 * @param[in] obj The object.
 * @param[in] homogeneous Assume the items within the toolbar are of the same size (true = on, false = off). Default is @c false.
 */
EAPI void
elm_toolbar_homogeneous_set(Elm_Toolbar *obj, Eina_Bool homogeneous)
{
   elm_obj_toolbar_homogeneous_set(obj, homogeneous);
}

/**
 * @brief Control homogeneous mode.
 * @details This will enable the homogeneous mode where items are of the same size.
 * @param[in] obj The object.
 * @return Assume the items within the toolbar are of the same size (true = on, false = off). Default is @c false.
 */
EAPI Eina_Bool
elm_toolbar_homogeneous_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_homogeneous_get(obj);
}

/**
 * @brief Control the alignment of the items.
 * @details Alignment of toolbar items, from 0.0 to indicates to align left, to 1.0, to align to right. 0.5 centralize items. Centered items by default.
 * @param[in] obj The object.
 * @param[in] align The new alignment, a float between 0.0 and 1.0.
 */
EAPI void
elm_toolbar_align_set(Elm_Toolbar *obj, double align)
{
   elm_obj_toolbar_align_set(obj, align);
}

/**
 * @brief Control the alignment of the items.
 * @details Alignment of toolbar items, from 0.0 to indicates to align left, to 1.0, to align to right. 0.5 centralize items. Centered items by default.
 * @param[in] obj The object.
 * @return The new alignment, a float between 0.0 and 1.0.
 */
EAPI double
elm_toolbar_align_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_align_get(obj);
}

/**
 * @brief Control the toolbar select mode.
 * @details elm_toolbar_select_mode_set() changes item select mode in the toolbar widget.
 * - #ELM_OBJECT_SELECT_MODE_DEFAULT : Items will only call their selection func and callback when first becoming selected. Any further clicks will do nothing, unless you set always select mode.
 * - #ELM_OBJECT_SELECT_MODE_ALWAYS : This means that, even if selected, every click will make the selected callbacks be called.
 * - #ELM_OBJECT_SELECT_MODE_NONE : This will turn off the ability to select items entirely and they will neither appear selected nor call selected callback functions.
 * @param[in] obj The object.
 * @param[in] mode The select mode.
 */
EAPI void
elm_toolbar_select_mode_set(Elm_Toolbar *obj, Elm_Object_Select_Mode mode)
{
   elm_obj_toolbar_select_mode_set(obj, mode);
}

/**
 * @brief Control the toolbar select mode.
 * @details See elm_toolbar_select_mode_set() for details.
 * @param[in] obj The object.
 * @return The select mode. If getting mode fails, it returns #ELM_OBJECT_SELECT_MODE_MAX.
 */
EAPI Elm_Object_Select_Mode
elm_toolbar_select_mode_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_select_mode_get(obj);
}

/**
 * @brief Control the icon size, in pixels, to be used by toolbar items.
 * @note Default value is $32. It reads value from elm config.
 * @param[in] obj The object.
 * @param[in] icon_size The icon size in pixels.
 */
EAPI void
elm_toolbar_icon_size_set(Elm_Toolbar *obj, int icon_size)
{
   elm_obj_toolbar_icon_size_set(obj, icon_size);
}

/**
 * @brief Control the icon size, in pixels, to be used by toolbar items.
 * @note Default value is $32. It reads value from elm config.
 * @param[in] obj The object.
 * @return The icon size in pixels.
 */
EAPI int
elm_toolbar_icon_size_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_icon_size_get(obj);
}

/**
 * @brief Control the item displaying mode of a given toolbar widget.
 * @details The toolbar won't scroll under #ELM_TOOLBAR_SHRINK_NONE mode, but it will enforce a minimum size, so that all the items will fit inside it. It won't scroll and won't show the items that don't fit under #ELM_TOOLBAR_SHRINK_HIDE mode. Finally, it'll scroll under #ELM_TOOLBAR_SHRINK_SCROLL mode, and it will create a button to aggregate items which didn't fit with the #ELM_TOOLBAR_SHRINK_MENU mode.
 * @warning This function's behavior will clash with those of elm_scroller_policy_set(), so use either one of them, but not both.
 * @param[in] obj The object.
 * @param[in] shrink_mode Toolbar's items display behavior.
 */
EAPI void
elm_toolbar_shrink_mode_set(Elm_Toolbar *obj, Elm_Toolbar_Shrink_Mode shrink_mode)
{
   elm_obj_toolbar_shrink_mode_set(obj, shrink_mode);
}

/**
 * @brief Control the item displaying mode of a given toolbar widget.
 * @details See elm_toolbar_shrink_mode_set() for details.
 * @param[in] obj The object.
 * @return Toolbar's items display behavior.
 */
EAPI Elm_Toolbar_Shrink_Mode
elm_toolbar_shrink_mode_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_shrink_mode_get(obj);
}

/**
 * @brief Control the parent object of the toolbar items' menus.
 * @details Each item can be set as item menu, with elm_toolbar_item_menu_set(). For more details about setting the parent for toolbar menus, see elm_menu_parent_set().
 * @param[in] obj The object.
 * @param[in] parent The parent of the menu objects.
 */
EAPI void
elm_toolbar_menu_parent_set(Elm_Toolbar *obj, Efl_Canvas_Object *parent)
{
   elm_obj_toolbar_menu_parent_set(obj, parent);
}

/**
 * @brief Control the parent object of the toolbar items' menus.
 * @details See elm_toolbar_menu_parent_set() for details.
 * @param[in] obj The object.
 * @return The parent of the menu objects.
 */
EAPI Efl_Canvas_Object *
elm_toolbar_menu_parent_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_menu_parent_get(obj);
}

/**
 * @brief Set the standard priority of visible items in a toolbar.
 * @details If the priority of the item is up to standard priority, it is shown in basic panel. The other items are located in more menu or panel. The more menu or panel can be shown when the more item is clicked.
 * @param[in] obj The object.
 * @param[in] priority The standard_priority of visible items.
 * @since 1.7
 */
EAPI void
elm_toolbar_standard_priority_set(Elm_Toolbar *obj, int priority)
{
   elm_obj_toolbar_standard_priority_set(obj, priority);
}

/**
 * @brief Set the standard priority of visible items in a toolbar.
 * @details See elm_toolbar_standard_priority_set() for details.
 * @param[in] obj The object.
 * @return The standard_priority of visible items.
 * @since 1.7
 */
EAPI int
elm_toolbar_standard_priority_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_standard_priority_get(obj);
}

/**
 * @brief Get the more item which is auto-generated by toolbar.
 * @details Toolbar generates 'more' item when there is no more space to fit items in and toolbar is in #ELM_TOOLBAR_SHRINK_MENU or #ELM_TOOLBAR_SHRINK_EXPAND mode. The more item can be manipulated by elm_object_item_text_set() and elm_object_item_content_set.
 * @param[in] obj The object.
 * @return The toolbar more item.
 */
EAPI Elm_Widget_Item *
elm_toolbar_more_item_get(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_more_item_get(obj);
}

/**
 * @brief Insert a new item into the toolbar object before item @c before.
 * @details A new item will be created and added to the toolbar. Its position in this toolbar will be just before item @c before.
 * @param[in] obj The object.
 * @param[in] before The toolbar item to insert before.
 * @param[in] icon A string with icon name or the absolute path of an image file.
 * @param[in] label The label of the item.
 * @param[in] func The function to call when the item is clicked.
 * @param[in] data The data to associate with the item for related callbacks.
 * @return The created item or @c NULL upon failure.
 */
EAPI Elm_Widget_Item *
elm_toolbar_item_insert_before(Elm_Toolbar *obj, Elm_Widget_Item *before, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_toolbar_item_insert_before(obj, before, icon, label, func, data);
}

/**
 * @brief Insert a new item into the toolbar object after item @c after.
 * @details A new item will be created and added to the toolbar. Its position in this toolbar will be just after item @c after.
 * @param[in] obj The object.
 * @param[in] after The toolbar item to insert after.
 * @param[in] icon A string with icon name or the absolute path of an image file.
 * @param[in] label The label of the item.
 * @param[in] func The function to call when the item is clicked.
 * @param[in] data The data to associate with the item for related callbacks.
 * @return The created item or @c NULL upon failure.
 */
EAPI Elm_Widget_Item *
elm_toolbar_item_insert_after(Elm_Toolbar *obj, Elm_Widget_Item *after, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_toolbar_item_insert_after(obj, after, icon, label, func, data);
}

/**
 * @brief Append item to the toolbar.
 * @details A new item will be created and appended to the toolbar, i.e., will be set as last item.
 * @param[in] obj The object.
 * @param[in] icon A string with icon name or the absolute path of an image file.
 * @param[in] label The label of the item.
 * @param[in] func The function to call when the item is clicked.
 * @param[in] data The data to associate with the item for related callbacks.
 * @return The created item or @c NULL upon failure.
 */
EAPI Elm_Widget_Item *
elm_toolbar_item_append(Elm_Toolbar *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_toolbar_item_append(obj, icon, label, func, data);
}

/**
 * @brief Get the number of items in a toolbar.
 * @param[in] obj The object.
 * @return The number of items in @c obj toolbar.
 */
EAPI unsigned int
elm_toolbar_items_count(const Elm_Toolbar *obj)
{
   return elm_obj_toolbar_items_count(obj);
}

/**
 * @brief Prepend item to the toolbar.
 * @details A new item will be created and prepended to the toolbar, i.e., will be set as first item.
 * @param[in] obj The object.
 * @param[in] icon A string with icon name or the absolute path of an image file.
 * @param[in] label The label of the item.
 * @param[in] func The function to call when the item is clicked.
 * @param[in] data The data to associate with the item for related callbacks.
 * @return The created item or @c NULL upon failure.
 */
EAPI Elm_Widget_Item *
elm_toolbar_item_prepend(Elm_Toolbar *obj, const char *icon, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_toolbar_item_prepend(obj, icon, label, func, data);
}

/**
 * @brief Returns a pointer to a toolbar item by its label.
 * @param[in] obj The object.
 * @param[in] label The label of the item to find.
 * @return The pointer to the toolbar item matching @c label or @c NULL on failure.
 */
EAPI Elm_Widget_Item *
elm_toolbar_item_find_by_label(const Elm_Toolbar *obj, const char *label)
{
   return elm_obj_toolbar_item_find_by_label(obj, label);
}
