/**
 * @brief Get the index of an item.
 *
 * @param[in] obj The object.
 * @return The position of item in segment control widget.
 * @ingroup Elm_Segment_Control_Item_Group
 */
EAPI int
elm_segment_control_item_index_get(const Elm_Segment_Control_Item *obj)
{
   return elm_obj_segment_control_item_index_get(obj);
}

/**
 * @brief Get the real Evas(Edje) object created to implement the view of a
 * given segment_control item.
 *
 * @param[in] obj The object.
 * @return The base Edje object associated with @c it
 * @ingroup Elm_Segment_Control_Item_Group
 */
EAPI Efl_Canvas_Object *
elm_segment_control_item_object_get(const Elm_Segment_Control_Item *obj)
{
   return elm_obj_segment_control_item_object_get(obj);
}

/**
 * @brief Set the selected state of an item.
 *
 * @param[in] obj The object.
 * @param[in] selected The selected state.
 * @ingroup Elm_Segment_Control_Item_Group
 */
EAPI void
elm_segment_control_item_selected_set(Elm_Segment_Control_Item *obj, Eina_Bool selected)
{
   elm_obj_segment_control_item_selected_set(obj, selected);
}
