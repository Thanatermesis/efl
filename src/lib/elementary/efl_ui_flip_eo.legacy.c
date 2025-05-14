
/**
 * @brief Set the interactive flip mode.
 * @param[in] obj The object.
 * @param[in] mode The interactive flip mode to use.
 * @see elm_flip_interaction_set
 */
EAPI void
elm_flip_interaction_set(Elm_Flip *obj, Elm_Flip_Interaction mode)
{
   efl_ui_flip_interaction_set(obj, (Efl_Ui_Flip_Interaction)mode);
}

/**
 * @brief Get the interactive flip mode.
 * @param[in] obj The object.
 * @return The interactive flip mode to use.
 * @see elm_flip_interaction_get
 */
EAPI Elm_Flip_Interaction
elm_flip_interaction_get(const Elm_Flip *obj)
{
   return (Elm_Flip_Interaction)efl_ui_flip_interaction_get(obj);
}

/**
 * @brief Get flip front visibility state.
 * @param[in] obj The object.
 * @return @c EINA_TRUE if front front is showing, @c EINA_FALSE if the back is showing.
 * @see elm_flip_front_visible_get
 */
EAPI Eina_Bool
elm_flip_front_visible_get(const Elm_Flip *obj)
{
   return efl_ui_flip_front_visible_get(obj);
}

/**
 * @brief Runs the flip animation.
 * @param[in] obj The object.
 * @param[in] mode The mode type.
 * @see elm_flip_go
 */
EAPI void
elm_flip_go(Elm_Flip *obj, Elm_Flip_Mode mode)
{
   efl_ui_flip_go(obj, (Efl_Ui_Flip_Mode)mode);
}

/**
 * @brief Runs the flip animation to front or back.
 * @param[in] obj The object.
 * @param[in] front If @c EINA_TRUE, makes front visible, otherwise makes back.
 * @param[in] mode The mode type.
 * @see elm_flip_go_to
 */
EAPI void
elm_flip_go_to(Elm_Flip *obj, Eina_Bool front, Elm_Flip_Mode mode)
{
   efl_ui_flip_go_to(obj, front, (Efl_Ui_Flip_Mode)mode);
}
