
EAPI Eina_Bool
elm_sys_notify_servers_set(Elm_Sys_Notify *obj, Elm_Sys_Notify_Server servers)
{
   return elm_obj_sys_notify_servers_set(obj, servers);
}

EAPI Elm_Sys_Notify_Server
elm_sys_notify_servers_get(const Elm_Sys_Notify *obj)
{
   return elm_obj_sys_notify_servers_get(obj);
}
/**
 * @file
 * @brief These routines are legacy implementations of the Elm_Sys_Notify API,
 *        providing a compatibility layer for older code. They wrap calls
 *        to the newer Eo-based object methods for system notifications.
 */
