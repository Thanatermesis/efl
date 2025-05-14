/**
 * @brief Sets the selected state of the legacy check object.
 *
 * This function is a legacy wrapper that calls the Efl_Ui_Selectable interface.
 *
 * @param[in] obj The Efl_Ui_Check object.
 * @param[in] value #EINA_TRUE to set the check as selected, #EINA_FALSE otherwise.
 *
 * @ingroup Elm_Check_Group
 */
EAPI void
elm_check_selected_set(Efl_Ui_Check *obj, Eina_Bool value)
{
   efl_ui_selectable_selected_set(obj, value);
}

/**
 * @brief Gets the selected state of the legacy check object.
 *
 * This function is a legacy wrapper that calls the Efl_Ui_Selectable interface.
 *
 * @param[in] obj The Efl_Ui_Check object.
 * @return #EINA_TRUE if the check is selected, #EINA_FALSE otherwise.
 *
 * @ingroup Elm_Check_Group
 */
EAPI Eina_Bool
elm_check_selected_get(const Efl_Ui_Check *obj)
{
   return efl_ui_selectable_selected_get(obj);
}
