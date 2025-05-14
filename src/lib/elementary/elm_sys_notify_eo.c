/**
 * @file
 * @brief Implementation of the Efl system notification module.
 *
 * This file contains the internal logic and Efl binding implementations
 * for system notification functionalities.
 */

/**
 * @internal
 * @brief Internal implementation for setting notification servers.
 * @param[in] obj The Efl object.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @param[in] servers A bitmask of @ref Elm_Sys_Notify_Server to enable.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
Eina_Bool _elm_sys_notify_servers_set(Eo *obj, Elm_Sys_Notify_Data *pd, Elm_Sys_Notify_Server servers);

EOAPI EFL_FUNC_BODYV(elm_obj_sys_notify_servers_set, Eina_Bool, 0, EFL_FUNC_CALL(servers), Elm_Sys_Notify_Server servers);

/**
 * @internal
 * @brief Internal implementation for getting registered notification servers.
 * @param[in] obj The Efl object.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @return A bitmask of currently registered @ref Elm_Sys_Notify_Server.
 */
Elm_Sys_Notify_Server _elm_sys_notify_servers_get(const Eo *obj, Elm_Sys_Notify_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_sys_notify_servers_get, Elm_Sys_Notify_Server, 0);

/**
 * @internal
 * @brief Internal implementation for retrieving the singleton instance.
 * @return The singleton Elm_Sys_Notify object.
 */
Elm_Sys_Notify *_elm_sys_notify_singleton_get(void);

EOAPI Elm_Sys_Notify * elm_obj_sys_notify_singleton_get(void)
{
   const Efl_Class *klass = elm_sys_notify_class_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(klass, NULL);
   return _elm_sys_notify_singleton_get();
}
EOAPI Elm_Sys_Notify * elm_sys_notify_singleton_get(void)
{
   const Efl_Class *klass = elm_sys_notify_class_get();
   EINA_SAFETY_ON_NULL_RETURN_VAL(klass, NULL);
   return _elm_sys_notify_singleton_get();
}

/**
 * @internal
 * @brief EFL object constructor for Elm_Sys_Notify.
 * @param[in] obj The Efl object being constructed.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @return The constructed Efl object.
 */
Efl_Object *_elm_sys_notify_efl_object_constructor(Eo *obj, Elm_Sys_Notify_Data *pd);

/**
 * @internal
 * @brief EFL object destructor for Elm_Sys_Notify.
 * @param[in] obj The Efl object being destructed.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 */
void _elm_sys_notify_efl_object_destructor(Eo *obj, Elm_Sys_Notify_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Sys_Notify.Interface.send method.
 * @param[in] obj The Efl object.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @param[in] replaces_id ID of an existing notification to replace, or 0 for a new one.
 * @param[in] icon Path to an icon file or a stock icon name.
 * @param[in] summary The summary text.
 * @param[in] body The body text.
 * @param[in] urgency The urgency level of the notification.
 * @param[in] timeout Timeout in milliseconds, or -1 for default, 0 for persistent.
 * @param[in] cb Callback function to be invoked when the notification is closed or action invoked.
 * @param[in] cb_data User data for the callback.
 */
void _elm_sys_notify_elm_sys_notify_interface_send(const Eo *obj, Elm_Sys_Notify_Data *pd, unsigned int replaces_id, const char *icon, const char *summary, const char *body, Elm_Sys_Notify_Urgency urgency, int timeout, Elm_Sys_Notify_Send_Cb cb, const void *cb_data);

/**
 * @internal
 * @brief Implements the Elm.Sys_Notify.Interface.simple_send method.
 * @param[in] obj The Efl object.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @param[in] icon Path to an icon file or a stock icon name.
 * @param[in] summary The summary text.
 * @param[in] body The body text.
 */
void _elm_sys_notify_elm_sys_notify_interface_simple_send(const Eo *obj, Elm_Sys_Notify_Data *pd, const char *icon, const char *summary, const char *body);

/**
 * @internal
 * @brief Implements the Elm.Sys_Notify.Interface.close method.
 * @param[in] obj The Efl object.
 * @param[in] pd Private data for the Elm_Sys_Notify object.
 * @param[in] id The ID of the notification to close.
 */
void _elm_sys_notify_elm_sys_notify_interface_close(const Eo *obj, Elm_Sys_Notify_Data *pd, unsigned int id);

/**
 * @internal
 * @brief Initializes the Elm_Sys_Notify Efl class.
 *
 * Sets up the operations for the class.
 * @param[in] klass The Efl class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_sys_notify_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SYS_NOTIFY_EXTRA_OPS
#define ELM_SYS_NOTIFY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_sys_notify_servers_set, _elm_sys_notify_servers_set),
      EFL_OBJECT_OP_FUNC(elm_obj_sys_notify_servers_get, _elm_sys_notify_servers_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_sys_notify_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_sys_notify_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_obj_sys_notify_interface_send, _elm_sys_notify_elm_sys_notify_interface_send),
      EFL_OBJECT_OP_FUNC(elm_obj_sys_notify_interface_simple_send, _elm_sys_notify_elm_sys_notify_interface_simple_send),
      EFL_OBJECT_OP_FUNC(elm_obj_sys_notify_interface_close, _elm_sys_notify_elm_sys_notify_interface_close),
      ELM_SYS_NOTIFY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Efl class description for Elm_Sys_Notify.
 *
 * Defines metadata for the Elm_Sys_Notify class, including its version,
 * name, type, data size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_sys_notify_class_desc = {
   EO_VERSION,
   "Elm.Sys_Notify",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Sys_Notify_Data),
   _elm_sys_notify_class_initializer,
   _elm_sys_notify_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_sys_notify_class_get, &_elm_sys_notify_class_desc, EFL_OBJECT_CLASS, ELM_SYS_NOTIFY_INTERFACE_INTERFACE, NULL);

#include "elm_sys_notify_eo.legacy.c"
