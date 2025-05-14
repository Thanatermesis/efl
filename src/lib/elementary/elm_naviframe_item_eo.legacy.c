/**
 * @internal
 * @brief Pop the top item and delete items until the specified item is at the top.
 *
 * This is a legacy EAPI function wrapper.
 *
 * @param obj The naviframe item object.
 * @see elm_obj_naviframe_item_pop_to()
 */
EAPI void
elm_naviframe_item_pop_to(Elm_Naviframe_Item *obj)
{
   elm_obj_naviframe_item_pop_to(obj);
}

/**
 * @internal
 * @brief Get whether the title area is enabled.
 *
 * This is a legacy EAPI function wrapper.
 *
 * @param obj The naviframe item object.
 * @return EINA_TRUE if the title is enabled, EINA_FALSE otherwise.
 * @see elm_obj_naviframe_item_title_enabled_get()
 */
EAPI Eina_Bool
elm_naviframe_item_title_enabled_get(const Elm_Naviframe_Item *obj)
{
   return elm_obj_naviframe_item_title_enabled_get(obj);
}

/**
 * @internal
 * @brief Set whether the title area is enabled.
 *
 * This is a legacy EAPI function wrapper.
 *
 * @param obj The naviframe item object.
 * @param enable EINA_TRUE to enable the title, EINA_FALSE to disable.
 * @param transition EINA_TRUE to use a transition, EINA_FALSE otherwise.
 * @see elm_obj_naviframe_item_title_enabled_set()
 */
EAPI void
elm_naviframe_item_title_enabled_set(Elm_Naviframe_Item *obj, Eina_Bool enable, Eina_Bool transition)
{
   elm_obj_naviframe_item_title_enabled_set(obj, enable, transition);
}

/**
 * @internal
 * @brief Promote the naviframe item to the top of the stack.
 *
 * This is a legacy EAPI function wrapper.
 *
 * @param obj The naviframe item object.
 * @see elm_obj_naviframe_item_promote()
 */
EAPI void
elm_naviframe_item_promote(Elm_Naviframe_Item *obj)
{
   elm_obj_naviframe_item_promote(obj);
}

/**
 * @internal
 * @brief Set the callback function to be called when the item is popped.
 *
 * This is a legacy EAPI function wrapper.
 *
 * @param obj The naviframe item object.
 * @param func The callback function.
 * @param data User data to be passed to the callback function.
 * @see elm_obj_naviframe_item_pop_cb_set()
 */
EAPI void
elm_naviframe_item_pop_cb_set(Elm_Naviframe_Item *obj, Elm_Naviframe_Item_Pop_Cb func, void *data)
{
   elm_obj_naviframe_item_pop_cb_set(obj, func, data);
}
