/**
 * @brief Legacy EAPI implementation for elm_color_item_color_set().
 * @see elm_color_item_color_set() in elm_color_item_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_color_item_color_set(Elm_Color_Item *obj, int r, int g, int b, int a)
{
   elm_obj_color_item_color_set(obj, r, g, b, a);
}
/**
 * @brief Legacy EAPI implementation for elm_color_item_color_get().
 * @see elm_color_item_color_get() in elm_color_item_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_color_item_color_get(const Elm_Color_Item *obj, int *r, int *g, int *b, int *a)
{
   elm_obj_color_item_color_get(obj, r, g, b, a);
}
/**
 * @brief Legacy EAPI implementation for elm_color_item_selected_set().
 * @see elm_color_item_selected_set() in elm_color_item_eo.legacy.h for detailed documentation.
 */
EAPI void
elm_color_item_selected_set(Elm_Color_Item *obj, Eina_Bool selected)
{
   elm_obj_color_item_selected_set(obj, selected);
}
/**
 * @brief Legacy EAPI implementation for elm_color_item_selected_get().
 * @see elm_color_item_selected_get() in elm_color_item_eo.legacy.h for detailed documentation.
 */
EAPI Eina_Bool
elm_color_item_selected_get(const Elm_Color_Item *obj)
{
   return elm_obj_color_item_selected_get(obj);
}
