/**
 * @internal
 * @brief Enable/disable the "ok" and "cancel" buttons on a given file selector
 * widget.
 *
 * @param obj The file selector object.
 * @param visible @c EINA_TRUE to show buttons, @c EINA_FALSE to hide.
 *
 * @see elm_fileselector_buttons_ok_cancel_get
 *
 * @ingroup Elm_Fileselector_Group
 */
EAPI void
elm_fileselector_buttons_ok_cancel_set(Elm_Fileselector *obj, Eina_Bool visible)
{
   elm_obj_fileselector_buttons_ok_cancel_set(obj, visible);
}

/**
 * @internal
 * @brief Get whether the "ok" and "cancel" buttons on a given file selector
 * widget are being shown.
 *
 * @param obj The file selector object.
 * @return @c EINA_TRUE if buttons are shown, @c EINA_FALSE otherwise.
 *
 * @see elm_fileselector_buttons_ok_cancel_set
 *
 * @ingroup Elm_Fileselector_Group
 */
EAPI Eina_Bool
elm_fileselector_buttons_ok_cancel_get(const Elm_Fileselector *obj)
{
   return elm_obj_fileselector_buttons_ok_cancel_get(obj);
}
