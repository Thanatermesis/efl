/**
 * @brief Implements the legacy C ABI for sending a detailed system notification.
 *
 * This function wraps the Eo-based implementation elm_obj_sys_notify_interface_send().
 * For detailed information on parameters and behavior, refer to the declaration of
 * elm_sys_notify_interface_send() in the corresponding legacy header file.
 */
EAPI void
elm_sys_notify_interface_send(const Elm_Sys_Notify_Interface *obj, unsigned int replaces_id, const char *icon, const char *summary, const char *body, Elm_Sys_Notify_Urgency urgency, int timeout, Elm_Sys_Notify_Send_Cb cb, const void *cb_data)
{
   elm_obj_sys_notify_interface_send(obj, replaces_id, icon, summary, body, urgency, timeout, cb, cb_data);
}

/**
 * @brief Implements the legacy C ABI for sending a simple system notification.
 *
 * This function wraps the Eo-based implementation elm_obj_sys_notify_interface_simple_send().
 * It provides a simplified way to send notifications with only an icon, summary, and body.
 * For detailed information on parameters and behavior, refer to the declaration of
 * elm_sys_notify_interface_simple_send() in the corresponding legacy header file.
 */
EAPI void
elm_sys_notify_interface_simple_send(const Elm_Sys_Notify_Interface *obj, const char *icon, const char *summary, const char *body)
{
   elm_obj_sys_notify_interface_simple_send(obj, icon, summary, body);
}

/**
 * @brief Implements the legacy C ABI for closing a system notification.
 *
 * This function wraps the Eo-based implementation elm_obj_sys_notify_interface_close().
 * It is used to forcefully remove a notification from the user's view.
 * For detailed information on parameters and behavior, refer to the declaration of
 * elm_sys_notify_interface_close() in the corresponding legacy header file.
 */
EAPI void
elm_sys_notify_interface_close(const Elm_Sys_Notify_Interface *obj, unsigned int id)
{
   elm_obj_sys_notify_interface_close(obj, id);
}
