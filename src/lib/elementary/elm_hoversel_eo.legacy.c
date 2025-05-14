/**
 * @brief Legacy C API implementation for #elm_hoversel_horizontal_set.
 * @see elm_hoversel_horizontal_set()
 */
EAPI void
elm_hoversel_horizontal_set(Elm_Hoversel *obj, Eina_Bool horizontal)
{
   elm_obj_hoversel_horizontal_set(obj, horizontal);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_horizontal_get.
 * @see elm_hoversel_horizontal_get()
 */
EAPI Eina_Bool
elm_hoversel_horizontal_get(const Elm_Hoversel *obj)
{
   return elm_obj_hoversel_horizontal_get(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_hover_parent_set.
 * @see elm_hoversel_hover_parent_set()
 */
EAPI void
elm_hoversel_hover_parent_set(Elm_Hoversel *obj, Efl_Canvas_Object *parent)
{
   elm_obj_hoversel_hover_parent_set(obj, parent);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_hover_parent_get.
 * @see elm_hoversel_hover_parent_get()
 */
EAPI Efl_Canvas_Object *
elm_hoversel_hover_parent_get(const Elm_Hoversel *obj)
{
   return elm_obj_hoversel_hover_parent_get(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_expanded_get.
 * @see elm_hoversel_expanded_get()
 */
EAPI Eina_Bool
elm_hoversel_expanded_get(const Elm_Hoversel *obj)
{
   return elm_obj_hoversel_expanded_get(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_items_get.
 * @see elm_hoversel_items_get()
 */
EAPI const Eina_List *
elm_hoversel_items_get(const Elm_Hoversel *obj)
{
   return elm_obj_hoversel_items_get(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_auto_update_set.
 * @see elm_hoversel_auto_update_set()
 */
EAPI void
elm_hoversel_auto_update_set(Elm_Hoversel *obj, Eina_Bool auto_update)
{
   elm_obj_hoversel_auto_update_set(obj, auto_update);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_auto_update_get.
 * @see elm_hoversel_auto_update_get()
 */
EAPI Eina_Bool
elm_hoversel_auto_update_get(const Elm_Hoversel *obj)
{
   return elm_obj_hoversel_auto_update_get(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_hover_begin.
 * @see elm_hoversel_hover_begin()
 */
EAPI void
elm_hoversel_hover_begin(Elm_Hoversel *obj)
{
   elm_obj_hoversel_hover_begin(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_clear.
 * @see elm_hoversel_clear()
 */
EAPI void
elm_hoversel_clear(Elm_Hoversel *obj)
{
   elm_obj_hoversel_clear(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_hover_end.
 * @see elm_hoversel_hover_end()
 */
EAPI void
elm_hoversel_hover_end(Elm_Hoversel *obj)
{
   elm_obj_hoversel_hover_end(obj);
}

/**
 * @brief Legacy C API implementation for #elm_hoversel_item_add.
 * @see elm_hoversel_item_add()
 */
EAPI Elm_Widget_Item *
elm_hoversel_item_add(Elm_Hoversel *obj, const char *label, const char *icon_file, Elm_Icon_Type icon_type, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_hoversel_item_add(obj, label, icon_file, icon_type, func, data);
}
