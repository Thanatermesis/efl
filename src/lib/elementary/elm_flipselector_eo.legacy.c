/**
 * @brief Implements @ref elm_flipselector_items_get.
 */
EAPI const Eina_List *
elm_flipselector_items_get(const Elm_Flipselector *obj)
{
   return elm_obj_flipselector_items_get(obj);
}

/**
 * @brief Implements @ref elm_flipselector_first_item_get.
 */
EAPI Elm_Widget_Item *
elm_flipselector_first_item_get(const Elm_Flipselector *obj)
{
   return elm_obj_flipselector_first_item_get(obj);
}

/**
 * @brief Implements @ref elm_flipselector_last_item_get.
 */
EAPI Elm_Widget_Item *
elm_flipselector_last_item_get(const Elm_Flipselector *obj)
{
   return elm_obj_flipselector_last_item_get(obj);
}

/**
 * @brief Implements @ref elm_flipselector_selected_item_get.
 */
EAPI Elm_Widget_Item *
elm_flipselector_selected_item_get(const Elm_Flipselector *obj)
{
   return elm_obj_flipselector_selected_item_get(obj);
}

/**
 * @brief Implements @ref elm_flipselector_first_interval_set.
 */
EAPI void
elm_flipselector_first_interval_set(Elm_Flipselector *obj, double interval)
{
   elm_obj_flipselector_first_interval_set(obj, interval);
}

/**
 * @brief Implements @ref elm_flipselector_first_interval_get.
 */
EAPI double
elm_flipselector_first_interval_get(const Elm_Flipselector *obj)
{
   return elm_obj_flipselector_first_interval_get(obj);
}

/**
 * @brief Implements @ref elm_flipselector_item_prepend.
 */
EAPI Elm_Widget_Item *
elm_flipselector_item_prepend(Elm_Flipselector *obj, const char *label, Evas_Smart_Cb func, void *data)
{
   return elm_obj_flipselector_item_prepend(obj, label, func, data);
}

/**
 * @brief Implements @ref elm_flipselector_flip_next.
 */
EAPI void
elm_flipselector_flip_next(Elm_Flipselector *obj)
{
   elm_obj_flipselector_flip_next(obj);
}

/**
 * @brief Implements @ref elm_flipselector_item_append.
 */
EAPI Elm_Widget_Item *
elm_flipselector_item_append(Elm_Flipselector *obj, const char *label, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_flipselector_item_append(obj, label, func, data);
}

/**
 * @brief Implements @ref elm_flipselector_flip_prev.
 */
EAPI void
elm_flipselector_flip_prev(Elm_Flipselector *obj)
{
   elm_obj_flipselector_flip_prev(obj);
}
