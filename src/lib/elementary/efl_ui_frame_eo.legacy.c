
/**
 * @brief Implements the legacy elm_frame_collapse_set() function.
 *
 * This function is a wrapper around the Efl_Ui_Frame API function
 * @ref efl_ui_frame_collapse_set.
 * For detailed documentation, please refer to the documentation of the
 * Efl_Ui_Frame class or the @ref efl_ui_frame_collapse_set function.
 */
EAPI void
elm_frame_collapse_set(Efl_Ui_Frame *obj, Eina_Bool collapse)
{
   efl_ui_frame_collapse_set(obj, collapse);
}

/**
 * @brief Implements the legacy elm_frame_collapse_get() function.
 *
 * This function is a wrapper around the Efl_Ui_Frame API function
 * @ref efl_ui_frame_collapse_get.
 * For detailed documentation, please refer to the documentation of the
 * Efl_Ui_Frame class or the @ref efl_ui_frame_collapse_get function.
 */
EAPI Eina_Bool
elm_frame_collapse_get(const Efl_Ui_Frame *obj)
{
   return efl_ui_frame_collapse_get(obj);
}

/**
 * @brief Implements the legacy elm_frame_autocollapse_set() function.
 *
 * This function is a wrapper around the Efl_Ui_Frame API function
 * @ref efl_ui_frame_autocollapse_set.
 * For detailed documentation, please refer to the documentation of the
 * Efl_Ui_Frame class or the @ref efl_ui_frame_autocollapse_set function.
 */
EAPI void
elm_frame_autocollapse_set(Efl_Ui_Frame *obj, Eina_Bool autocollapse)
{
   efl_ui_frame_autocollapse_set(obj, autocollapse);
}

/**
 * @brief Implements the legacy elm_frame_autocollapse_get() function.
 *
 * This function is a wrapper around the Efl_Ui_Frame API function
 * @ref efl_ui_frame_autocollapse_get.
 * For detailed documentation, please refer to the documentation of the
 * Efl_Ui_Frame class or the @ref efl_ui_frame_autocollapse_get function.
 */
EAPI Eina_Bool
elm_frame_autocollapse_get(const Efl_Ui_Frame *obj)
{
   return efl_ui_frame_autocollapse_get(obj);
}

/**
 * @brief Implements the legacy elm_frame_collapse_go() function.
 *
 * This function is a wrapper around the Efl_Ui_Frame API function
 * @ref efl_ui_frame_collapse_go.
 * For detailed documentation, please refer to the documentation of the
 * Efl_Ui_Frame class or the @ref efl_ui_frame_collapse_go function.
 */
EAPI void
elm_frame_collapse_go(Efl_Ui_Frame *obj, Eina_Bool collapse)
{
   efl_ui_frame_collapse_go(obj, collapse);
}
