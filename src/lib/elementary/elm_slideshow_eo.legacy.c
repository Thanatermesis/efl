/**
 * @brief Set the number of items to cache, on a given slideshow widget, after
 * the current item.
 * @param[in] obj The object.
 * @param[in] count Number of items to cache after the current one.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_cache_after_set(Elm_Slideshow *obj, int count)
{
   elm_obj_slideshow_cache_after_set(obj, count);
}

/**
 * @brief Get the number of items to cache, on a given slideshow widget, after
 * the current item.
 * @param[in] obj The object.
 * @return Number of items to cache after the current one.
 * @ingroup Elm_Slideshow_Group
 */
EAPI int
elm_slideshow_cache_after_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_cache_after_get(obj);
}

/**
 * @brief Set the number of items to cache, on a given slideshow widget, before
 * the current item.
 * @param[in] obj The object.
 * @param[in] count Number of items to cache before the current one.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_cache_before_set(Elm_Slideshow *obj, int count)
{
   elm_obj_slideshow_cache_before_set(obj, count);
}

/**
 * @brief Get the number of items to cache, on a given slideshow widget, before
 * the current item.
 * @param[in] obj The object.
 * @return Number of items to cache before the current one.
 * @ingroup Elm_Slideshow_Group
 */
EAPI int
elm_slideshow_cache_before_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_cache_before_get(obj);
}

/**
 * @brief Set the current slide layout in use for a given slideshow widget.
 * @param[in] obj The object.
 * @param[in] layout The new layout's name string.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_layout_set(Elm_Slideshow *obj, const char *layout)
{
   elm_obj_slideshow_layout_set(obj, layout);
}

/**
 * @brief Get the current slide layout in use for a given slideshow widget.
 * @param[in] obj The object.
 * @return The new layout's name string.
 * @ingroup Elm_Slideshow_Group
 */
EAPI const char *
elm_slideshow_layout_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_layout_get(obj);
}

/**
 * @brief Set the current slide transition/effect in use for a given slideshow
 * widget.
 * @param[in] obj The object.
 * @param[in] transition The new transition's name string.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_transition_set(Elm_Slideshow *obj, const char *transition)
{
   elm_obj_slideshow_transition_set(obj, transition);
}

/**
 * @brief Get the current slide transition/effect in use for a given slideshow
 * widget.
 * @param[in] obj The object.
 * @return The new transition's name string.
 * @ingroup Elm_Slideshow_Group
 */
EAPI const char *
elm_slideshow_transition_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_transition_get(obj);
}

/**
 * @brief Set if the slideshow items should be displayed cyclically or not.
 * @param[in] obj The object.
 * @param[in] loop Use @c true to make it cycle through items or @c false otherwise.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_loop_set(Elm_Slideshow *obj, Eina_Bool loop)
{
   elm_obj_slideshow_items_loop_set(obj, loop);
}

/**
 * @brief Get if, after a slideshow is started, for a given slideshow widget,
 * its items are to be displayed cyclically or not.
 * @param[in] obj The object.
 * @return @c EINA_TRUE if items are displayed cyclically, @c EINA_FALSE otherwise.
 * @ingroup Elm_Slideshow_Group
 */
EAPI Eina_Bool
elm_slideshow_loop_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_items_loop_get(obj);
}

/**
 * @brief Set the interval between each image transition on a given slideshow
 * widget and start the slideshow itself.
 * @param[in] obj The object.
 * @param[in] timeout The new displaying timeout for images.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_timeout_set(Elm_Slideshow *obj, double timeout)
{
   elm_obj_slideshow_timeout_set(obj, timeout);
}

/**
 * @brief Get the interval set for image transitions on a given slideshow
 * widget.
 * @param[in] obj The object.
 * @return The displaying timeout for images.
 * @ingroup Elm_Slideshow_Group
 */
EAPI double
elm_slideshow_timeout_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_timeout_get(obj);
}

/**
 * @brief Get the internal list of items in a given slideshow widget.
 * @param[in] obj The object.
 * @return The list of items (#Elm_Widget_Item as data) or @c null on errors.
 * @ingroup Elm_Slideshow_Group
 */
EAPI const Eina_List *
elm_slideshow_items_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_items_get(obj);
}

/**
 * @brief Returns the list of sliding transition/effect names available, for a
 * given slideshow widget.
 * @param[in] obj The object.
 * @return The list of transitions (list of stringshared strings as data).
 * @ingroup Elm_Slideshow_Group
 */
EAPI const Eina_List *
elm_slideshow_transitions_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_transitions_get(obj);
}

/**
 * @brief Get the number of items stored in a given slideshow widget.
 * @param[in] obj The object.
 * @return The number of items on @c obj.
 * @ingroup Elm_Slideshow_Group
 */
EAPI unsigned int
elm_slideshow_count_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_count_get(obj);
}

/**
 * @brief Returns the currently displayed item, in a given slideshow widget.
 * @param[in] obj The object.
 * @return A handle to the item being displayed in @c obj or @c null.
 * @ingroup Elm_Slideshow_Group
 */
EAPI Elm_Widget_Item *
elm_slideshow_item_current_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_item_current_get(obj);
}

/**
 * @brief Returns the list of layout names available, for a given slideshow
 * widget.
 * @param[in] obj The object.
 * @return The list of layouts (list of stringshared strings as data).
 * @ingroup Elm_Slideshow_Group
 */
EAPI const Eina_List *
elm_slideshow_layouts_get(const Elm_Slideshow *obj)
{
   return elm_obj_slideshow_layouts_get(obj);
}

/**
 * @brief Slide to the previous item, in a given slideshow widget.
 * @param[in] obj The object.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_previous(Elm_Slideshow *obj)
{
   elm_obj_slideshow_previous(obj);
}

/**
 * @brief Get the the item, in a given slideshow widget, placed at position
 * @c nth, in its internal items list.
 * @param[in] obj The object.
 * @param[in] nth The number of the item to grab a handle to (0 being the first).
 * @return The item stored in @c obj at position @c nth or @c null.
 * @ingroup Elm_Slideshow_Group
 */
EAPI Elm_Widget_Item *
elm_slideshow_item_nth_get(const Elm_Slideshow *obj, unsigned int nth)
{
   return elm_obj_slideshow_item_nth_get(obj, nth);
}

/**
 * @brief Slide to the next item, in a given slideshow widget.
 * @param[in] obj The object.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_next(Elm_Slideshow *obj)
{
   elm_obj_slideshow_next(obj);
}

/**
 * @brief Remove all items from a given slideshow widget.
 * @param[in] obj The object.
 * @ingroup Elm_Slideshow_Group
 */
EAPI void
elm_slideshow_clear(Elm_Slideshow *obj)
{
   elm_obj_slideshow_clear(obj);
}

/**
 * @brief Add (append) a new item in a given slideshow widget.
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item's data.
 * @return A handle to the item added or @c null on errors.
 * @ingroup Elm_Slideshow_Group
 */
EAPI Elm_Widget_Item *
elm_slideshow_item_add(Elm_Slideshow *obj, const Elm_Slideshow_Item_Class *itc, const void *data)
{
   return elm_obj_slideshow_item_add(obj, itc, data);
}

/**
 * @brief Insert a new item into the given slideshow widget, using the @c func
 * function to sort items (by item handles).
 * @param[in] obj The object.
 * @param[in] itc The item class for the item.
 * @param[in] data The item's data.
 * @param[in] func The comparing function to be used to sort the slideshow items.
 * @return The slideshow item handle, on success, or @c null on errors.
 * @ingroup Elm_Slideshow_Group
 */
EAPI Elm_Widget_Item *
elm_slideshow_item_sorted_insert(Elm_Slideshow *obj, const Elm_Slideshow_Item_Class *itc, const void *data, Eina_Compare_Cb func)
{
   return elm_obj_slideshow_item_sorted_insert(obj, itc, data, func);
}
