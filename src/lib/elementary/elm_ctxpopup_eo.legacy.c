/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_selected_item_get().
 * @see elm_ctxpopup_selected_item_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_selected_item_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_selected_item_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_first_item_get().
 * @see elm_ctxpopup_first_item_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_first_item_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_first_item_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_last_item_get().
 * @see elm_ctxpopup_last_item_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_last_item_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_last_item_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_items_get().
 * @see elm_ctxpopup_items_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI const Eina_List *
elm_ctxpopup_items_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_items_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_horizontal_set().
 * @see elm_ctxpopup_horizontal_set() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_horizontal_set(Elm_Ctxpopup *obj, Eina_Bool horizontal)
{
   elm_obj_ctxpopup_horizontal_set(obj, horizontal);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_horizontal_get().
 * @see elm_ctxpopup_horizontal_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Eina_Bool
elm_ctxpopup_horizontal_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_horizontal_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_auto_hide_disabled_set().
 * @see elm_ctxpopup_auto_hide_disabled_set() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_auto_hide_disabled_set(Elm_Ctxpopup *obj, Eina_Bool disabled)
{
   elm_obj_ctxpopup_auto_hide_disabled_set(obj, disabled);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_auto_hide_disabled_get().
 * @see elm_ctxpopup_auto_hide_disabled_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Eina_Bool
elm_ctxpopup_auto_hide_disabled_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_auto_hide_disabled_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_hover_parent_set().
 * @see elm_ctxpopup_hover_parent_set() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_hover_parent_set(Elm_Ctxpopup *obj, Efl_Canvas_Object *parent)
{
   elm_obj_ctxpopup_hover_parent_set(obj, parent);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_hover_parent_get().
 * @see elm_ctxpopup_hover_parent_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Efl_Canvas_Object *
elm_ctxpopup_hover_parent_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_hover_parent_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_direction_priority_set().
 * @see elm_ctxpopup_direction_priority_set() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_direction_priority_set(Elm_Ctxpopup *obj, Elm_Ctxpopup_Direction first, Elm_Ctxpopup_Direction second, Elm_Ctxpopup_Direction third, Elm_Ctxpopup_Direction fourth)
{
   elm_obj_ctxpopup_direction_priority_set(obj, first, second, third, fourth);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_direction_priority_get().
 * @see elm_ctxpopup_direction_priority_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_direction_priority_get(const Elm_Ctxpopup *obj, Elm_Ctxpopup_Direction *first, Elm_Ctxpopup_Direction *second, Elm_Ctxpopup_Direction *third, Elm_Ctxpopup_Direction *fourth)
{
   elm_obj_ctxpopup_direction_priority_get(obj, first, second, third, fourth);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_direction_get().
 * @see elm_ctxpopup_direction_get() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Ctxpopup_Direction
elm_ctxpopup_direction_get(const Elm_Ctxpopup *obj)
{
   return elm_obj_ctxpopup_direction_get(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_dismiss().
 * @see elm_ctxpopup_dismiss() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_dismiss(Elm_Ctxpopup *obj)
{
   elm_obj_ctxpopup_dismiss(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_clear().
 * @see elm_ctxpopup_clear() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_ctxpopup_clear(Elm_Ctxpopup *obj)
{
   elm_obj_ctxpopup_clear(obj);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_item_insert_before().
 * @see elm_ctxpopup_item_insert_before() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_insert_before(Elm_Ctxpopup *obj, Elm_Widget_Item *before, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_ctxpopup_item_insert_before(obj, before, label, icon, func, data);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_item_insert_after().
 * @see elm_ctxpopup_item_insert_after() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_insert_after(Elm_Ctxpopup *obj, Elm_Widget_Item *after, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_ctxpopup_item_insert_after(obj, after, label, icon, func, data);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_item_append().
 * @see elm_ctxpopup_item_append() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_append(Elm_Ctxpopup *obj, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_ctxpopup_item_append(obj, label, icon, func, data);
}

/**
 * @brief Legacy EAPI wrapper for elm_obj_ctxpopup_item_prepend().
 * @see elm_ctxpopup_item_prepend() in elm_ctxpopup_eo.legacy.h for detailed documentation.
 */
EAPI Elm_Widget_Item *
elm_ctxpopup_item_prepend(Elm_Ctxpopup *obj, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   return elm_obj_ctxpopup_item_prepend(obj, label, icon, func, data);
}
