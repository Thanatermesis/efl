/**
 * @brief Legacy wrapper for elm_obj_multibuttonentry_editable_set().
 *
 * This function controls if the multibuttonentry is to be editable or not.
 *
 * @param[in] obj The object.
 * @param[in] editable If @c EINA_TRUE, user can add/delete item in
 * multibuttonentry; if @c EINA_FALSE, the multibuttonentry is non-editable.
 *
 * @deprecated Prefer using the Eo API directly.
 * @see elm_multibuttonentry_editable_set() in elm_multibuttonentry_eo.legacy.h for the original EAPI documentation.
 */
EAPI void
elm_multibuttonentry_editable_set(Elm_Multibuttonentry *obj, Eina_Bool editable)
{
   elm_obj_multibuttonentry_editable_set(obj, editable);
}
/**
 * @brief Legacy wrapper for elm_obj_multibuttonentry_editable_get().
 *
 * This function retrieves whether the multibuttonentry is editable.
 *
 * @param[in] obj The object.
 * @return @c EINA_TRUE if the object is editable, @c EINA_FALSE otherwise.
 *
 * @deprecated Prefer using the Eo API directly.
 * @see elm_multibuttonentry_editable_get() in elm_multibuttonentry_eo.legacy.h for the original EAPI documentation.
 */
EAPI Eina_Bool
elm_multibuttonentry_editable_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_editable_get(obj);
}
/**
 * @brief Legacy wrapper for elm_obj_multibuttonentry_expanded_set().
 *
 * This function controls the expanded state of the multibuttonentry.
 *
 * @param[in] obj The object.
 * @param[in] expanded If @c EINA_TRUE, the complete entry will be displayed;
 * if @c EINA_FALSE, only a single line will be displayed.
 *
 * @deprecated Prefer using the Eo API directly.
 * @see elm_multibuttonentry_expanded_set() in elm_multibuttonentry_eo.legacy.h for the original EAPI documentation.
 */
EAPI void
elm_multibuttonentry_expanded_set(Elm_Multibuttonentry *obj, Eina_Bool expanded)
{
   elm_obj_multibuttonentry_expanded_set(obj, expanded);
}
/**
 * @brief Legacy wrapper for elm_obj_multibuttonentry_expanded_get().
 *
 * This function retrieves the expanded state of the multibuttonentry.
 *
 * @param[in] obj The object.
 * @return @c EINA_TRUE if the object is in an expanded state, @c EINA_FALSE otherwise.
 *
 * @deprecated Prefer using the Eo API directly.
 * @see elm_multibuttonentry_expanded_get() in elm_multibuttonentry_eo.legacy.h for the original EAPI documentation.
 */
EAPI Eina_Bool
elm_multibuttonentry_expanded_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_expanded_get(obj);
}

EAPI void
elm_multibuttonentry_format_function_set(Elm_Multibuttonentry *obj, Elm_Multibuttonentry_Format_Cb format_function, const void *data)
{
   elm_obj_multibuttonentry_format_function_set(obj, format_function, data);
}

EAPI const Eina_List *
elm_multibuttonentry_items_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_items_get(obj);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_first_item_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_first_item_get(obj);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_last_item_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_last_item_get(obj);
}

EAPI Efl_Canvas_Object *
elm_multibuttonentry_entry_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_entry_get(obj);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_selected_item_get(const Elm_Multibuttonentry *obj)
{
   return elm_obj_multibuttonentry_selected_item_get(obj);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_item_prepend(Elm_Multibuttonentry *obj, const char *label, Evas_Smart_Cb func, void *data)
{
   return elm_obj_multibuttonentry_item_prepend(obj, label, func, data);
}

EAPI void
elm_multibuttonentry_clear(Elm_Multibuttonentry *obj)
{
   elm_obj_multibuttonentry_clear(obj);
}

EAPI void
elm_multibuttonentry_item_filter_remove(Elm_Multibuttonentry *obj, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   elm_obj_multibuttonentry_item_filter_remove(obj, func, data);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_item_insert_before(Elm_Multibuttonentry *obj, Elm_Widget_Item *before, const char *label, Evas_Smart_Cb func, void *data)
{
   return elm_obj_multibuttonentry_item_insert_before(obj, before, label, func, data);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_item_append(Elm_Multibuttonentry *obj, const char *label, Evas_Smart_Cb func, void *data)
{
   return elm_obj_multibuttonentry_item_append(obj, label, func, data);
}

EAPI void
elm_multibuttonentry_item_filter_prepend(Elm_Multibuttonentry *obj, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   elm_obj_multibuttonentry_item_filter_prepend(obj, func, data);
}

EAPI void
elm_multibuttonentry_item_filter_append(Elm_Multibuttonentry *obj, Elm_Multibuttonentry_Item_Filter_Cb func, void *data)
{
   elm_obj_multibuttonentry_item_filter_append(obj, func, data);
}

EAPI Elm_Widget_Item *
elm_multibuttonentry_item_insert_after(Elm_Multibuttonentry *obj, Elm_Widget_Item *after, const char *label, Evas_Smart_Cb func, void *data)
{
   return elm_obj_multibuttonentry_item_insert_after(obj, after, label, func, data);
}
