#include "eldbus_private.h"
#include "eldbus_private_types.h"
#include <dbus/dbus.h>

/**
 * @internal
 * @brief Implements eldbus_name_request() using a proxy call to the D-Bus daemon.
 * @see eldbus_name_request() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "RequestName" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to request ownership of a bus name.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The `flags` parameter can be a combination of `ELDBUS_NAME_REQUEST_FLAG_*` values,
 * e.g., `ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE`.
 */
EAPI Eldbus_Pending *
eldbus_name_request(Eldbus_Connection *conn, const char *name, unsigned int flags, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "RequestName", cb,
                           cb_data, -1, "su", name, flags);
}

/**
 * @internal
 * @brief Implements eldbus_name_release() using a proxy call to the D-Bus daemon.
 * @see eldbus_name_release() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "ReleaseName" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to release ownership of a bus name.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 */
EAPI Eldbus_Pending *
eldbus_name_release(Eldbus_Connection *conn, const char *name, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "ReleaseName", cb,
                           cb_data, -1, "s", name);
}

/**
 * @internal
 * @brief Implements eldbus_name_owner_get() using a proxy call to the D-Bus daemon.
 * @see eldbus_name_owner_get() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "GetNameOwner" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to get the unique connection name of the owner of the specified bus name.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The callback will receive a message containing a string (the unique name) on success.
 */
EAPI Eldbus_Pending *
eldbus_name_owner_get(Eldbus_Connection *conn, const char *name, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "GetNameOwner", cb,
                           cb_data, -1, "s", name);
}

/**
 * @internal
 * @brief Implements eldbus_name_owner_has() using a proxy call to the D-Bus daemon.
 * @see eldbus_name_owner_has() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "NameHasOwner" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to check if a bus name has an owner.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The callback will receive a message containing a boolean on success.
 */
EAPI Eldbus_Pending *
eldbus_name_owner_has(Eldbus_Connection *conn, const char *name, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "NameHasOwner", cb,
                           cb_data, -1, "s", name);
}

/**
 * @internal
 * @brief Implements eldbus_names_list() using a proxy call to the D-Bus daemon.
 * @see eldbus_names_list() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "ListNames" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to retrieve a list of all currently owned names.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The callback will receive a message containing an array of strings (bus names) on success.
 * Example of returned array structure (D-Bus type 'as'):
 * @code
 * [
 *   "org.freedesktop.DBus",
 *   ":1.0",
 *   "com.example.Service"
 * ]
 * @endcode
 */
EAPI Eldbus_Pending *
eldbus_names_list(Eldbus_Connection *conn, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "ListNames", cb,
                           cb_data, -1, "");
}

/**
 * @internal
 * @brief Implements eldbus_names_activatable_list() using a proxy call to the D-Bus daemon.
 * @see eldbus_names_activatable_list() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "ListActivatableNames" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to retrieve a list of all names that can be activated.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The callback will receive a message containing an array of strings (activatable names) on success.
 */
EAPI Eldbus_Pending *
eldbus_names_activatable_list(Eldbus_Connection *conn, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "ListActivatableNames", cb,
                           cb_data, -1, "");
}

/**
 * @internal
 * @brief Implements eldbus_name_start() using a proxy call to the D-Bus daemon.
 * @see eldbus_name_start() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "StartServiceByName" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus") to request activation of a service by its bus name.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The `flags` parameter is currently unused by the D-Bus specification for this method.
 * The callback receives a uint32 indicating the result, e.g., `ELDBUS_NAME_START_REPLY_SUCCESS`.
 */
EAPI Eldbus_Pending *
eldbus_name_start(Eldbus_Connection *conn, const char *name, unsigned int flags, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "StartServiceByName", cb,
                           cb_data, -1, "su", name, flags);
}

/**
 * @internal
 * @brief Implements the public API function eldbus_object_managed_objects_get().
 * @see eldbus_object_managed_objects_get() in eldbus_freedesktop.h for detailed parameter and return value documentation.
 *
 * This function calls the "GetManagedObjects" method on the specified D-Bus object,
 * which must implement the "org.freedesktop.DBus.ObjectManager" interface.
 * It retrieves all objects managed by the service at and below the object path of `obj`.
 * The return is typically a dictionary where keys are object paths and values are
 * dictionaries of interfaces and their properties.
 * Example of returned structure (D-Bus type `a{oa{sa{sv}}}`):
 * @code
 * { // This is an "array of dictionary entries"
 *   "/path/to/object1": { // object_path 'o' is the key
 *     "org.example.Interface1": { // interface_name 's' is the key for the inner dict
 *       "PropertyName1": <variant_value1>, // property_name 's' is key for property dict
 *       "PropertyName2": <variant_value2>  // property_value 'v' (variant)
 *     },
 *     "org.example.Interface2": { ... } // another interface 'a{sa{sv}}' for this object
 *   },
 *   "/path/to/object2": { ... } // another object path entry
 * }
 * @endcode
 */
EAPI Eldbus_Pending *
eldbus_object_managed_objects_get(Eldbus_Object *obj, Eldbus_Message_Cb cb, const void *data)
{
   Eldbus_Message *msg;
   Eldbus_Pending *p;
   msg = eldbus_object_method_call_new(obj, ELDBUS_FDO_INTERFACE_OBJECT_MANAGER,
                                      "GetManagedObjects");
   p = eldbus_object_send(obj, msg, cb, data, -1);
   return p;
}

/**
 * @internal
 * @brief Implements the public API function eldbus_object_manager_interfaces_added().
 * @see eldbus_object_manager_interfaces_added() in eldbus_freedesktop.h for detailed parameter and return value documentation.
 *
 * This function registers a signal handler for the "InterfacesAdded" signal from the
 * "org.freedesktop.DBus.ObjectManager" interface on the given D-Bus object.
 * The signal is emitted when new interfaces are added to an object managed by this object manager.
 * The signal signature is `(oa{sa{sv}})`. The callback `cb` will receive an `Eldbus_Message`
 * whose body contains these arguments.
 * Example of arguments passed to the callback (simplified structure in the message):
 * @code
 * // Arguments in the received D-Bus message:
 * // 1. Object Path (type 'o'): "/path/to/object"
 * // 2. Interfaces and properties (type 'a{sa{sv}}'):
 * {
 *   "org.example.Interface1": { // interface_name 's'
 *     "PropertyName1": <variant_value1>, // property_name 's', value 'v'
 *     "PropertyName2": <variant_value2>
 *   },
 *   "org.example.Interface2": { ... }
 * }
 * @endcode
 */
EAPI Eldbus_Signal_Handler *
eldbus_object_manager_interfaces_added(Eldbus_Object *obj, Eldbus_Signal_Cb cb, const void *cb_data)
{
   return eldbus_object_signal_handler_add(obj, ELDBUS_FDO_INTERFACE_OBJECT_MANAGER,
                                           "InterfacesAdded", cb, cb_data);
}

/**
 * @internal
 * @brief Implements the public API function eldbus_object_manager_interfaces_removed().
 * @see eldbus_object_manager_interfaces_removed() in eldbus_freedesktop.h for detailed parameter and return value documentation.
 *
 * This function registers a signal handler for the "InterfacesRemoved" signal from the
 * "org.freedesktop.DBus.ObjectManager" interface on the given D-Bus object.
 * The signal is emitted when interfaces are removed from an object managed by this object manager.
 * The signal signature is `(oas)`. The callback `cb` will receive an `Eldbus_Message`
 * whose body contains these arguments.
 * Example of arguments passed to the callback (simplified structure in the message):
 * @code
 * // Arguments in the received D-Bus message:
 * // 1. Object Path (type 'o'): "/path/to/object"
 * // 2. Array of interface names removed (type 'as'):
 * [
 *   "org.example.Interface1", // interface_name 's'
 *   "org.example.Interface2"
 * ]
 * @endcode
 */
EAPI Eldbus_Signal_Handler *
eldbus_object_manager_interfaces_removed(Eldbus_Object *obj, Eldbus_Signal_Cb cb, const void *cb_data)
{
   return eldbus_object_signal_handler_add(obj, ELDBUS_FDO_INTERFACE_OBJECT_MANAGER,
                                           "InterfacesRemoved", cb, cb_data);
}

/**
 * @internal
 * @brief Implements eldbus_hello() using a proxy call to the D-Bus daemon.
 * @see eldbus_hello() in eldbus_freedesktop.h for API details.
 *
 * This function sends the "Hello" message to the D-Bus daemon
 * (service "org.freedesktop.DBus", path "/org/freedesktop/DBus",
 * interface "org.freedesktop.DBus"). This is typically the first message sent
 * to the bus after connecting, to obtain the client's unique connection name.
 * It utilizes the connection's shared Freedesktop.org proxy (`conn->fdo_proxy`).
 * The callback will receive a message containing a string (the unique name) on success.
 */
EAPI Eldbus_Pending *
eldbus_hello(Eldbus_Connection *conn, Eldbus_Message_Cb cb, const void *cb_data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(conn, NULL);

   return eldbus_proxy_call(conn->fdo_proxy, "Hello", cb, cb_data, -1, "");
}
