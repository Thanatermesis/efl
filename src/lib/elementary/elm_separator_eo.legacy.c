/**
 * @brief Sets the horizontal mode of a separator widget.
 *
 * This function is a legacy wrapper for elm_obj_separator_horizontal_set().
 *
 * @param[in] obj The separator object.
 * @param[in] horizontal If EINA_TRUE, the separator is horizontal, otherwise vertical.
 */
EAPI void
elm_separator_horizontal_set(Elm_Separator *obj, Eina_Bool horizontal)
{
   elm_obj_separator_horizontal_set(obj, horizontal);
}

/**
 * @brief Gets the horizontal mode of a separator widget.
 *
 * This function is a legacy wrapper for elm_obj_separator_horizontal_get().
 *
 * @param[in] obj The separator object.
 * @return EINA_TRUE if the separator is horizontal, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
elm_separator_horizontal_get(const Elm_Separator *obj)
{
   return elm_obj_separator_horizontal_get(obj);
}
