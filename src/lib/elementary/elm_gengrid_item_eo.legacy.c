/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_prev_get().
 *
 * @see elm_gengrid_item_prev_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_prev_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_prev_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_next_get().
 *
 * @see elm_gengrid_item_next_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI Elm_Widget_Item *
elm_gengrid_item_next_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_next_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_selected_set().
 *
 * @see elm_gengrid_item_selected_set() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_selected_set(Elm_Gengrid_Item *obj, Eina_Bool selected)
{
   elm_obj_gengrid_item_selected_set(obj, selected);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_selected_get().
 *
 * @see elm_gengrid_item_selected_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI Eina_Bool
elm_gengrid_item_selected_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_selected_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_class_get().
 *
 * @see elm_gengrid_item_item_class_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI const Elm_Gengrid_Item_Class *
elm_gengrid_item_item_class_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_class_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_index_get().
 *
 * @see elm_gengrid_item_index_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI int
elm_gengrid_item_index_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_index_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_pos_get().
 *
 * @see elm_gengrid_item_pos_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_pos_get(const Elm_Gengrid_Item *obj, unsigned int *x, unsigned int *y)
{
   elm_obj_gengrid_item_pos_get(obj, x, y);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_select_mode_set().
 *
 * @see elm_gengrid_item_select_mode_set() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_select_mode_set(Elm_Gengrid_Item *obj, Elm_Object_Select_Mode mode)
{
   elm_obj_gengrid_item_select_mode_set(obj, mode);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_select_mode_get().
 *
 * @see elm_gengrid_item_select_mode_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI Elm_Object_Select_Mode
elm_gengrid_item_select_mode_get(const Elm_Gengrid_Item *obj)
{
   return elm_obj_gengrid_item_select_mode_get(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_custom_size_set().
 *
 * @see elm_gengrid_item_custom_size_set() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_custom_size_set(Elm_Gengrid_Item *obj, int w, int h)
{
   elm_obj_gengrid_item_custom_size_set(obj, w, h);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_custom_size_get().
 *
 * @see elm_gengrid_item_custom_size_get() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_custom_size_get(const Elm_Gengrid_Item *obj, int *w, int *h)
{
   elm_obj_gengrid_item_custom_size_get(obj, w, h);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_show().
 *
 * @see elm_gengrid_item_show() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_show(Elm_Gengrid_Item *obj, Elm_Gengrid_Item_Scrollto_Type type)
{
   elm_obj_gengrid_item_show(obj, type);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_bring_in().
 *
 * @see elm_gengrid_item_bring_in() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_bring_in(Elm_Gengrid_Item *obj, Elm_Gengrid_Item_Scrollto_Type type)
{
   elm_obj_gengrid_item_bring_in(obj, type);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_update().
 *
 * @see elm_gengrid_item_update() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_update(Elm_Gengrid_Item *obj)
{
   elm_obj_gengrid_item_update(obj);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_fields_update().
 *
 * @see elm_gengrid_item_fields_update() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_fields_update(Elm_Gengrid_Item *obj, const char *parts, Elm_Gengrid_Item_Field_Type itf)
{
   elm_obj_gengrid_item_fields_update(obj, parts, itf);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_class_update().
 *
 * @see elm_gengrid_item_item_class_update() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_item_class_update(Elm_Gengrid_Item *obj, const Elm_Gengrid_Item_Class *itc)
{
   elm_obj_gengrid_item_class_update(obj, itc);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_gengrid_item_all_contents_unset().
 *
 * @see elm_gengrid_item_all_contents_unset() in elm_gengrid_item_eo.legacy.h for details.
 */
EAPI void
elm_gengrid_item_all_contents_unset(Elm_Gengrid_Item *obj, Eina_List **l)
{
   elm_obj_gengrid_item_all_contents_unset(obj, l);
}
