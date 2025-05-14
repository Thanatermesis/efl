/**
 * @brief Legacy wrapper for efl_ui_radio_state_value_set().
 * @deprecated Use efl_ui_radio_state_value_set() instead.
 */
EAPI void
elm_radio_state_value_set(Efl_Ui_Radio *obj, int value)
{
   efl_ui_radio_state_value_set(obj, value);
}

/**
 * @brief Legacy wrapper for efl_ui_radio_state_value_get().
 * @deprecated Use efl_ui_radio_state_value_get() instead.
 */
EAPI int
elm_radio_state_value_get(const Efl_Ui_Radio *obj)
{
   return efl_ui_radio_state_value_get(obj);
}
