
/**
 * @brief Legacy implementation of index_autohide_disabled_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_autohide_disabled_set.
 */
EAPI void
elm_index_autohide_disabled_set(Elm_Index *obj, Eina_Bool disabled)
{
   elm_obj_index_autohide_disabled_set(obj, disabled);
}

/**
 * @brief Legacy implementation of index_autohide_disabled_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_autohide_disabled_get.
 */
EAPI Eina_Bool
elm_index_autohide_disabled_get(const Elm_Index *obj)
{
   return elm_obj_index_autohide_disabled_get(obj);
}

/**
 * @brief Legacy implementation of index_omit_enabled_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_omit_enabled_set.
 */
EAPI void
elm_index_omit_enabled_set(Elm_Index *obj, Eina_Bool enabled)
{
   elm_obj_index_omit_enabled_set(obj, enabled);
}

/**
 * @brief Legacy implementation of index_omit_enabled_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_omit_enabled_get.
 */
EAPI Eina_Bool
elm_index_omit_enabled_get(const Elm_Index *obj)
{
   return elm_obj_index_omit_enabled_get(obj);
}

/**
 * @brief Legacy implementation of index_standard_priority_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_standard_priority_set.
 */
EAPI void
elm_index_standard_priority_set(Elm_Index *obj, int priority)
{
   elm_obj_index_standard_priority_set(obj, priority);
}

/**
 * @brief Legacy implementation of index_standard_priority_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_standard_priority_get.
 */
EAPI int
elm_index_standard_priority_get(const Elm_Index *obj)
{
   return elm_obj_index_standard_priority_get(obj);
}

/**
 * @brief Legacy implementation of index_delay_change_time_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_delay_change_time_set.
 */
EAPI void
elm_index_delay_change_time_set(Elm_Index *obj, double dtime)
{
   elm_obj_index_delay_change_time_set(obj, dtime);
}

/**
 * @brief Legacy implementation of index_delay_change_time_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_delay_change_time_get.
 */
EAPI double
elm_index_delay_change_time_get(const Elm_Index *obj)
{
   return elm_obj_index_delay_change_time_get(obj);
}

/**
 * @brief Legacy implementation of index_indicator_disabled_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_indicator_disabled_set.
 */
EAPI void
elm_index_indicator_disabled_set(Elm_Index *obj, Eina_Bool disabled)
{
   elm_obj_index_indicator_disabled_set(obj, disabled);
}

/**
 * @brief Legacy implementation of index_indicator_disabled_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_indicator_disabled_get.
 */
EAPI Eina_Bool
elm_index_indicator_disabled_get(const Elm_Index *obj)
{
   return elm_obj_index_indicator_disabled_get(obj);
}

/**
 * @brief Legacy implementation of index_item_level_set.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_level_set.
 */
EAPI void
elm_index_item_level_set(Elm_Index *obj, int level)
{
   elm_obj_index_item_level_set(obj, level);
}

/**
 * @brief Legacy implementation of index_item_level_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_level_get.
 */
EAPI int
elm_index_item_level_get(const Elm_Index *obj)
{
   return elm_obj_index_item_level_get(obj);
}

/**
 * @brief Legacy implementation of index_level_go.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_level_go.
 */
EAPI void
elm_index_level_go(Elm_Index *obj, int level)
{
   elm_obj_index_level_go(obj, level);
}

/**
 * @brief Legacy implementation of index_item_prepend.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_prepend.
 */
EAPI Elm_Widget_Item *
elm_index_item_prepend(Elm_Index *obj, const char *letter, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_index_item_prepend(obj, letter, func, data);
}

/**
 * @brief Legacy implementation of index_item_clear.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_clear.
 */
EAPI void
elm_index_item_clear(Elm_Index *obj)
{
   elm_obj_index_item_clear(obj);
}

/**
 * @brief Legacy implementation of index_item_insert_after.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_insert_after.
 */
EAPI Elm_Widget_Item *
elm_index_item_insert_after(Elm_Index *obj, Elm_Widget_Item *after, const char *letter, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_index_item_insert_after(obj, after, letter, func, data);
}

/**
 * @brief Legacy implementation of index_item_find.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_find.
 */
EAPI Elm_Widget_Item *
elm_index_item_find(Elm_Index *obj, const void *data)
{
   return elm_obj_index_item_find(obj, data);
}

/**
 * @brief Legacy implementation of index_item_insert_before.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_insert_before.
 */
EAPI Elm_Widget_Item *
elm_index_item_insert_before(Elm_Index *obj, Elm_Widget_Item *before, const char *letter, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_index_item_insert_before(obj, before, letter, func, data);
}

/**
 * @brief Legacy implementation of index_item_append.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_append.
 */
EAPI Elm_Widget_Item *
elm_index_item_append(Elm_Index *obj, const char *letter, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_index_item_append(obj, letter, func, data);
}

/**
 * @brief Legacy implementation of index_selected_item_get.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_selected_item_get.
 */
EAPI Elm_Widget_Item *
elm_index_selected_item_get(const Elm_Index *obj, int level)
{
   return elm_obj_index_selected_item_get(obj, level);
}

/**
 * @brief Legacy implementation of index_item_sorted_insert.
 * @deprecated Please refer to the declaration in elm_index_eo.legacy.h for API documentation.
 * This function wraps elm_obj_index_item_sorted_insert.
 */
EAPI Elm_Widget_Item *
elm_index_item_sorted_insert(Elm_Index *obj, const char *letter, Evas_Smart_Cb func, const void *data, Eina_Compare_Cb cmp_func, Eina_Compare_Cb cmp_data_func)
{
   return elm_obj_index_item_sorted_insert(obj, letter, func, data, cmp_func, cmp_data_func);
}
