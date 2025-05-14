#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eldbus_model_private.h"

#include <Ecore.h>
#include <Eina.h>
#include <Eldbus.h>

#define MY_CLASS ELDBUS_MODEL_CLASS
#define MY_CLASS_NAME "Eldbus_Model"

/**
 * @internal
 * @brief Establishes a D-Bus connection based on the model's configuration.
 *
 * This function attempts to connect to D-Bus using either a predefined
 * address or a standard bus type (session, system). It also handles
 * whether the connection should be private or shared.
 *
 * @param pd Pointer to the private data of the Eldbus_Model object.
 */
static void
eldbus_model_connect_do(Eldbus_Model_Data *pd)
{
   if (pd->type == ELDBUS_CONNECTION_TYPE_ADDRESS)
     {
        if (pd->private)
          pd->connection = eldbus_address_connection_get(pd->address);
        else
          pd->connection = eldbus_private_address_connection_get(pd->address);
     }
   else
     {
        if (pd->private)
          pd->connection = eldbus_private_connection_get(pd->type);
        else
          pd->connection = eldbus_connection_get(pd->type);
     }

   // TODO: Register for disconnection event
   if (!pd->connection)
     {
        DBG("Unable to setup a connection [%i - %s] %i",
            pd->type, pd->address, pd->private);
     }
}

/**
 * @internal
 * @brief Sets the connection parameters for the Eldbus_Model.
 *
 * This function stores the desired D-Bus connection type, address (if applicable),
 * and whether the connection should be private. The actual connection is
 * established later, typically during finalization.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param type The type of D-Bus connection (e.g., ELDBUS_CONNECTION_TYPE_SESSION, ELDBUS_CONNECTION_TYPE_SYSTEM, ELDBUS_CONNECTION_TYPE_ADDRESS).
 * @param address The D-Bus address string, used if type is ELDBUS_CONNECTION_TYPE_ADDRESS. Can be NULL otherwise.
 * @param priv EINA_TRUE if the connection should be private, EINA_FALSE otherwise.
 */
static void
_eldbus_model_connect(Eo *obj EINA_UNUSED,
                      Eldbus_Model_Data *pd,
                      Eldbus_Connection_Type type,
                      const char *address,
                      Eina_Bool priv)
{
   pd->type = type;
   pd->address = eina_stringshare_add(address);
   pd->private = priv;
}

/**
 * @internal
 * @brief Sets an existing D-Bus connection for the Eldbus_Model.
 *
 * This function allows an already established Eldbus_Connection to be
 * associated with the model. It takes a reference to the new connection
 * and releases any previously held connection.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param dbus The Eldbus_Connection to set.
 */
static void
_eldbus_model_connection_set(Eo *obj EINA_UNUSED,
                             Eldbus_Model_Data *pd,
                             Eldbus_Connection *dbus)
{
   Eldbus_Connection *tounref = pd->connection;

   eldbus_connection_ref(dbus);
   pd->connection = NULL;
   if (tounref) eldbus_connection_unref(tounref);
   pd->connection = dbus;
}

/**
 * @internal
 * @brief Retrieves the D-Bus connection associated with the Eldbus_Model.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return The current Eldbus_Connection, or NULL if not connected.
 */
static Eldbus_Connection *
_eldbus_model_connection_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Data *pd)
{
   return pd->connection;
}

/**
 * @internal
 * @brief Finalizes the Eldbus_Model object.
 *
 * This function is called during the Efl_Object finalization phase.
 * It ensures that a D-Bus connection is established if one hasn't been
 * set up already.
 *
 * @param obj The Eldbus_Model object being finalized.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return The finalized Efl_Object, or NULL on failure (e.g., connection failed).
 */
static Efl_Object *
_eldbus_model_efl_object_finalize(Eo *obj, Eldbus_Model_Data *pd)
{
   if (!pd->connection) eldbus_model_connect_do(pd);
   if (!pd->connection) return NULL;

   return efl_finalize(efl_super(obj, ELDBUS_MODEL_CLASS));
}

/**
 * @internal
 * @brief Invalidates the Eldbus_Model object.
 *
 * This function is called when the Efl_Object is being invalidated.
 * It releases the D-Bus connection held by the model.
 *
 * @param obj The Eldbus_Model object being invalidated.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 */
static void
_eldbus_model_efl_object_invalidate(Eo *obj, Eldbus_Model_Data *pd)
{
   Eldbus_Connection *connection = pd->connection;

   pd->connection = NULL;
   if (connection) eldbus_connection_unref(connection);

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Destroys the Eldbus_Model object.
 *
 * This function is called during the Efl_Object destruction phase.
 * It cleans up resources held by the model, such as stringshared
 * connection address and unique name.
 *
 * @param obj The Eldbus_Model object being destructed.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 */
static void
_eldbus_model_efl_object_destructor(Eo *obj, Eldbus_Model_Data *pd)
{
   eina_stringshare_del(pd->unique_name);
   pd->unique_name = NULL;

   eina_stringshare_del(pd->address);
   pd->address = NULL;

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Gets the D-Bus address string configured for the model.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return The D-Bus address string, or NULL if not set or not applicable.
 */
static const char *
_eldbus_model_address_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Data *pd)
{
   return pd->address;
}

/**
 * @internal
 * @brief Checks if the model is configured for a private D-Bus connection.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return EINA_TRUE if private connection, EINA_FALSE otherwise.
 */
static Eina_Bool
_eldbus_model_private_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Data *pd)
{
   return pd->private;
}

/**
 * @internal
 * @brief Gets the D-Bus connection type configured for the model.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return The Eldbus_Connection_Type.
 */
static Eldbus_Connection_Type
_eldbus_model_type_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Data *pd)
{
   return pd->type;
}

/**
 * @internal
 * @brief Sets a property on the Eldbus_Model.
 *
 * Currently, only the "unique_name" property is recognized, and it is read-only.
 * Attempting to set any property will result in an error.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param property The name of the property to set.
 * @param value The Eina_Value to set for the property.
 * @return A rejected Eina_Future with EFL_MODEL_ERROR_READ_ONLY or EFL_MODEL_ERROR_NOT_FOUND.
 */
static Eina_Future *
_eldbus_model_efl_model_property_set(Eo *obj,
                                     Eldbus_Model_Data *pd EINA_UNUSED,
                                     const char *property,
                                     Eina_Value *value EINA_UNUSED)
{
   Eina_Error err = EFL_MODEL_ERROR_READ_ONLY;

   if (!eina_streq(property, UNIQUE_NAME_PROPERTY))
     err = EFL_MODEL_ERROR_NOT_FOUND;
   return efl_loop_future_rejected(obj, err);
}

/**
 * @internal
 * @brief Gets a property from the Eldbus_Model.
 *
 * Supports fetching the "unique_name" property, which is the unique D-Bus name
 * of the connection. If the connection is not yet established, this function
 * will attempt to connect.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param property The name of the property to get. Currently, only "unique_name" is supported.
 * @return An Eina_Value containing the property value (string for "unique_name"),
 *         or an Eina_Value with error EFL_MODEL_ERROR_NOT_FOUND if the property
 *         is not found or an error occurs.
 */
static Eina_Value *
_eldbus_model_efl_model_property_get(const Eo *obj,
                                     Eldbus_Model_Data *pd,
                                     const char *property)
{
   DBG("(%p): property=%s", obj, property);

   if (!eina_streq(property, UNIQUE_NAME_PROPERTY)) goto on_error;

   if (!pd->connection) eldbus_model_connect_do(pd);

   if (pd->unique_name == NULL)
     {
        const char *unique_name;

        unique_name = eldbus_connection_unique_name_get(pd->connection);
        if (!unique_name) goto on_error;
        pd->unique_name = eina_stringshare_add(unique_name);
     }

   return eina_value_string_new(pd->unique_name);

 on_error:
   return eina_value_error_new(EFL_MODEL_ERROR_NOT_FOUND);
}

/**
 * @internal
 * @brief Gets an iterator for the properties of the Eldbus_Model.
 *
 * Currently, this model exposes a single property: "unique_name".
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return An Eina_Iterator over the property names (strings).
 *         The caller is responsible for freeing the iterator.
 *         Example of iterated elements:
 *         - "unique_name"
 */
static Eina_Iterator *
_eldbus_model_efl_model_properties_get(const Eo *obj EINA_UNUSED,
                                       Eldbus_Model_Data *pd EINA_UNUSED)
{
   char *unique[] = { UNIQUE_NAME_PROPERTY };

   return EINA_C_ARRAY_ITERATOR_NEW(unique);
}

/**
 * @internal
 * @brief Adds a child object to the Eldbus_Model.
 *
 * This operation is not supported by Eldbus_Model.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return Always NULL.
 */
static Efl_Object *
_eldbus_model_efl_model_child_add(Eo *obj EINA_UNUSED,
                                  Eldbus_Model_Data *pd EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Deletes a child object from the Eldbus_Model.
 *
 * This operation is not supported by Eldbus_Model.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param child The child object to delete.
 */
static void
_eldbus_model_efl_model_child_del(Eo *obj EINA_UNUSED,
                                  Eldbus_Model_Data *pd EINA_UNUSED,
                                  Efl_Object *child EINA_UNUSED)
{
}

/**
 * @internal
 * @brief Gets a slice of children from the Eldbus_Model.
 *
 * This operation is not supported by Eldbus_Model as it does not have children.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @param start The starting index of the slice.
 * @param count The number of children in the slice.
 * @return A rejected Eina_Future with EFL_MODEL_ERROR_NOT_SUPPORTED.
 */
static Eina_Future *
_eldbus_model_efl_model_children_slice_get(Eo *obj EINA_UNUSED,
                                           Eldbus_Model_Data *pd EINA_UNUSED,
                                           unsigned int start EINA_UNUSED,
                                           unsigned int count EINA_UNUSED)
{
   return efl_loop_future_rejected(obj,
                               EFL_MODEL_ERROR_NOT_SUPPORTED);
}

/**
 * @internal
 * @brief Gets the count of children of the Eldbus_Model.
 *
 * Eldbus_Model does not have children.
 *
 * @param obj The Eldbus_Model object.
 * @param pd Pointer to the private data of the Eldbus_Model object.
 * @return Always 0.
 */
static unsigned int
_eldbus_model_efl_model_children_count_get(const Eo *obj EINA_UNUSED,
                                           Eldbus_Model_Data *pd EINA_UNUSED)
{
   return 0;
}

#include "eldbus_model.eo.c"
