/**
 * @internal
 * @brief Implements the EAPI function elm_slideshow_item_show().
 */
EAPI void
elm_slideshow_item_show(Elm_Slideshow_Item *obj)
{
   elm_obj_slideshow_item_show(obj);
}

/**
 * @internal
 * @brief Implements the EAPI function elm_slideshow_item_object_get().
 */
EAPI Efl_Canvas_Object *
elm_slideshow_item_object_get(const Elm_Slideshow_Item *obj)
{
   return elm_obj_slideshow_item_object_get(obj);
}
