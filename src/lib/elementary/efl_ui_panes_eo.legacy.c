/**
 * @brief Set whether the left and right panes can be resized by user
 * interaction.
 *
 * By default panes' contents are resizable by user interaction.
 *
 * @param[in] obj The object.
 * @param[in] fixed Use @c true to fix the left and right panes sizes and make
 * them not to be resized by user interaction. Use @c false to make them
 * resizable.
 *
 * @ingroup Elm_Panes_Group
 */
EAPI void
elm_panes_fixed_set(Efl_Ui_Panes *obj, Eina_Bool fixed)
{
   efl_ui_panes_fixed_set(obj, fixed);
}

/**
 * @brief Set whether the left and right panes can be resized by user
 * interaction.
 *
 * By default panes' contents are resizable by user interaction.
 *
 * @param[in] obj The object.
 *
 * @return Use @c true to fix the left and right panes sizes and make them not
 * to be resized by user interaction. Use @c false to make them resizable.
 *
 * @ingroup Elm_Panes_Group
 */
EAPI Eina_Bool
elm_panes_fixed_get(const Efl_Ui_Panes *obj)
{
   return efl_ui_panes_fixed_get(obj);
}
