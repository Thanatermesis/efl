
/**
 * @brief Implements the legacy API for elm_list_horizontal_set().
 * @details This function is a wrapper around elm_obj_list_horizontal_set().
 * For full documentation, see elm_list_horizontal_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_horizontal_set(Elm_List *obj, Eina_Bool horizontal)
{
   elm_obj_list_horizontal_set(obj, horizontal);
}

/**
 * @brief Implements the legacy API for elm_list_horizontal_get().
 * @details This function is a wrapper around elm_obj_list_horizontal_get().
 * For full documentation, see elm_list_horizontal_get() in elm_list_eo.legacy.h.
 */
EAPI Eina_Bool
elm_list_horizontal_get(const Elm_List *obj)
{
   return elm_obj_list_horizontal_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_select_mode_set().
 * @details This function is a wrapper around elm_obj_list_select_mode_set().
 * For full documentation, see elm_list_select_mode_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_select_mode_set(Elm_List *obj, Elm_Object_Select_Mode mode)
{
   elm_obj_list_select_mode_set(obj, mode);
}

/**
 * @brief Implements the legacy API for elm_list_select_mode_get().
 * @details This function is a wrapper around elm_obj_list_select_mode_get().
 * For full documentation, see elm_list_select_mode_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Object_Select_Mode
elm_list_select_mode_get(const Elm_List *obj)
{
   return elm_obj_list_select_mode_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_focus_on_selection_set().
 * @details This function is a wrapper around elm_obj_list_focus_on_selection_set().
 * For full documentation, see elm_list_focus_on_selection_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_focus_on_selection_set(Elm_List *obj, Eina_Bool enabled)
{
   elm_obj_list_focus_on_selection_set(obj, enabled);
}

/**
 * @brief Implements the legacy API for elm_list_focus_on_selection_get().
 * @details This function is a wrapper around elm_obj_list_focus_on_selection_get().
 * For full documentation, see elm_list_focus_on_selection_get() in elm_list_eo.legacy.h.
 */
EAPI Eina_Bool
elm_list_focus_on_selection_get(const Elm_List *obj)
{
   return elm_obj_list_focus_on_selection_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_multi_select_set().
 * @details This function is a wrapper around elm_obj_list_multi_select_set().
 * For full documentation, see elm_list_multi_select_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_multi_select_set(Elm_List *obj, Eina_Bool multi)
{
   elm_obj_list_multi_select_set(obj, multi);
}

/**
 * @brief Implements the legacy API for elm_list_multi_select_get().
 * @details This function is a wrapper around elm_obj_list_multi_select_get().
 * For full documentation, see elm_list_multi_select_get() in elm_list_eo.legacy.h.
 */
EAPI Eina_Bool
elm_list_multi_select_get(const Elm_List *obj)
{
   return elm_obj_list_multi_select_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_multi_select_mode_set().
 * @details This function is a wrapper around elm_obj_list_multi_select_mode_set().
 * For full documentation, see elm_list_multi_select_mode_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_multi_select_mode_set(Elm_List *obj, Elm_Object_Multi_Select_Mode mode)
{
   elm_obj_list_multi_select_mode_set(obj, mode);
}

/**
 * @brief Implements the legacy API for elm_list_multi_select_mode_get().
 * @details This function is a wrapper around elm_obj_list_multi_select_mode_get().
 * For full documentation, see elm_list_multi_select_mode_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Object_Multi_Select_Mode
elm_list_multi_select_mode_get(const Elm_List *obj)
{
   return elm_obj_list_multi_select_mode_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_mode_set().
 * @details This function is a wrapper around elm_obj_list_mode_set().
 * For full documentation, see elm_list_mode_set() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_mode_set(Elm_List *obj, Elm_List_Mode mode)
{
   elm_obj_list_mode_set(obj, mode);
}

/**
 * @brief Implements the legacy API for elm_list_mode_get().
 * @details This function is a wrapper around elm_obj_list_mode_get().
 * For full documentation, see elm_list_mode_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_List_Mode
elm_list_mode_get(const Elm_List *obj)
{
   return elm_obj_list_mode_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_selected_item_get().
 * @details This function is a wrapper around elm_obj_list_selected_item_get().
 * For full documentation, see elm_list_selected_item_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_selected_item_get(const Elm_List *obj)
{
   return elm_obj_list_selected_item_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_items_get().
 * @details This function is a wrapper around elm_obj_list_items_get().
 * For full documentation, see elm_list_items_get() in elm_list_eo.legacy.h.
 */
EAPI const Eina_List *
elm_list_items_get(const Elm_List *obj)
{
   return elm_obj_list_items_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_first_item_get().
 * @details This function is a wrapper around elm_obj_list_first_item_get().
 * For full documentation, see elm_list_first_item_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_first_item_get(const Elm_List *obj)
{
   return elm_obj_list_first_item_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_selected_items_get().
 * @details This function is a wrapper around elm_obj_list_selected_items_get().
 * For full documentation, see elm_list_selected_items_get() in elm_list_eo.legacy.h.
 */
EAPI const Eina_List *
elm_list_selected_items_get(const Elm_List *obj)
{
   return elm_obj_list_selected_items_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_last_item_get().
 * @details This function is a wrapper around elm_obj_list_last_item_get().
 * For full documentation, see elm_list_last_item_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_last_item_get(const Elm_List *obj)
{
   return elm_obj_list_last_item_get(obj);
}

/**
 * @brief Implements the legacy API for elm_list_item_insert_before().
 * @details This function is a wrapper around elm_obj_list_item_insert_before().
 * For full documentation, see elm_list_item_insert_before() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_item_insert_before(Elm_List *obj, Elm_Widget_Item *before, const char *label, Efl_Canvas_Object *icon, Efl_Canvas_Object *end, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_list_item_insert_before(obj, before, label, icon, end, func, data);
}

/**
 * @brief Implements the legacy API for elm_list_go().
 * @details This function is a wrapper around elm_obj_list_go().
 * For full documentation, see elm_list_go() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_go(Elm_List *obj)
{
   elm_obj_list_go(obj);
}

/**
 * @brief Implements the legacy API for elm_list_item_insert_after().
 * @details This function is a wrapper around elm_obj_list_item_insert_after().
 * For full documentation, see elm_list_item_insert_after() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_item_insert_after(Elm_List *obj, Elm_Widget_Item *after, const char *label, Efl_Canvas_Object *icon, Efl_Canvas_Object *end, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_list_item_insert_after(obj, after, label, icon, end, func, data);
}

/**
 * @brief Implements the legacy API for elm_list_at_xy_item_get().
 * @details This function is a wrapper around elm_obj_list_at_xy_item_get().
 * For full documentation, see elm_list_at_xy_item_get() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_at_xy_item_get(const Elm_List *obj, int x, int y, int *posret)
{
   return elm_obj_list_at_xy_item_get(obj, x, y, posret);
}

/**
 * @brief Implements the legacy API for elm_list_item_append().
 * @details This function is a wrapper around elm_obj_list_item_append().
 * For full documentation, see elm_list_item_append() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_item_append(Elm_List *obj, const char *label, Efl_Canvas_Object *icon, Efl_Canvas_Object *end, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_list_item_append(obj, label, icon, end, func, data);
}

/**
 * @brief Implements the legacy API for elm_list_item_prepend().
 * @details This function is a wrapper around elm_obj_list_item_prepend().
 * For full documentation, see elm_list_item_prepend() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_item_prepend(Elm_List *obj, const char *label, Efl_Canvas_Object *icon, Efl_Canvas_Object *end, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_list_item_prepend(obj, label, icon, end, func, data);
}

/**
 * @brief Implements the legacy API for elm_list_clear().
 * @details This function is a wrapper around elm_obj_list_clear().
 * For full documentation, see elm_list_clear() in elm_list_eo.legacy.h.
 */
EAPI void
elm_list_clear(Elm_List *obj)
{
   elm_obj_list_clear(obj);
}

/**
 * @brief Implements the legacy API for elm_list_item_sorted_insert().
 * @details This function is a wrapper around elm_obj_list_item_sorted_insert().
 * For full documentation, see elm_list_item_sorted_insert() in elm_list_eo.legacy.h.
 */
EAPI Elm_Widget_Item *
elm_list_item_sorted_insert(Elm_List *obj, const char *label, Efl_Canvas_Object *icon, Efl_Canvas_Object *end, Evas_Smart_Cb func, const void *data, Eina_Compare_Cb cmp_func)
{
   return elm_obj_list_item_sorted_insert(obj, label, icon, end, func, data, cmp_func);
}
