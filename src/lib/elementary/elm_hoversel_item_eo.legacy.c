/**
 * @internal
 * @brief Legacy wrapper for elm_obj_hoversel_item_icon_set().
 *
 * This function is the legacy C implementation for setting the icon of a hoversel item.
 * It calls the underlying Eo API function elm_obj_hoversel_item_icon_set().
 * For detailed parameter descriptions, see the EAPI documentation for
 * elm_hoversel_item_icon_set() in the header file.
 */
EAPI void
elm_hoversel_item_icon_set(Elm_Hoversel_Item *obj, const char *icon_file, const char *icon_group, Elm_Icon_Type icon_type)
{
   elm_obj_hoversel_item_icon_set(obj, icon_file, icon_group, icon_type);
}

/**
 * @internal
 * @brief Legacy wrapper for elm_obj_hoversel_item_icon_get().
 *
 * This function is the legacy C implementation for getting the icon of a hoversel item.
 * It calls the underlying Eo API function elm_obj_hoversel_item_icon_get().
 * For detailed parameter descriptions, see the EAPI documentation for
 * elm_hoversel_item_icon_get() in the header file.
 */
EAPI void
elm_hoversel_item_icon_get(const Elm_Hoversel_Item *obj, const char **icon_file, const char **icon_group, Elm_Icon_Type *icon_type)
{
   elm_obj_hoversel_item_icon_get(obj, icon_file, icon_group, icon_type);
}
