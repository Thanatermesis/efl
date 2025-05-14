#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include "eldbus_model_connection_private.h"
#include "eldbus_model_private.h"

#define MY_CLASS ELDBUS_MODEL_CONNECTION_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Connection"

/**
 * @brief Callback for the eldbus_names_list function.
 *
 * This function is called when the list of names (bus services) is retrieved.
 * It processes the message, populates the children list, and resolves pending promises.
 *
 * @param data User data, expected to be Eldbus_Model_Connection_Data.
 * @param msg The Eldbus_Message containing the list of names or an error.
 * @param pending The Eldbus_Pending object associated with the request.
 */
static void _eldbus_model_connection_names_list_cb(void *, const Eldbus_Message *, Eldbus_Pending *);

static Efl_Object*
_eldbus_model_connection_efl_object_constructor(Eo *obj, Eldbus_Model_Connection_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;

   return obj;
}

/**
 * @brief Invalidates the Eldbus_Model_Connection object.
 *
 * This function is called when the object is being invalidated.
 * It cancels any pending D-Bus requests, rejects any pending promises for
 * children slices, and frees the list of children.
 *
 * @param obj The Eo object.
 * @param pd The private data of the Eldbus_Model_Connection.
 */
static void
_eldbus_model_connection_efl_object_invalidate(Eo *obj, Eldbus_Model_Connection_Data *pd)
{
   Eldbus_Children_Slice_Promise *slice;

   if (pd->pending) eldbus_pending_cancel(pd->pending);

   EINA_LIST_FREE(pd->requests, slice)
     {
        eina_promise_reject(slice->p, EFL_MODEL_ERROR_UNKNOWN);
        free(slice);
     }

   pd->childrens = eina_list_free(pd->childrens);

   efl_invalidate(efl_super(obj, ELDBUS_MODEL_CONNECTION_CLASS));
}

/**
 * @brief Initiates the listing of D-Bus service names (children).
 *
 * If not already listing or listed, this function requests the list of
 * available service names from the D-Bus connection.
 *
 * @param obj The Eo object (const as it's not modified directly here, but through pd).
 * @param pd The private data of the Eldbus_Model_Connection.
 */
static void
_eldbus_model_children_list(const Eo *obj, Eldbus_Model_Connection_Data *pd)
{
   Eldbus_Model_Data *sd;

   if (pd->pending || pd->is_listed) return ;

   sd = efl_data_scope_get(obj, ELDBUS_MODEL_CLASS);

   pd->pending = eldbus_names_list(sd->connection,
                                   &_eldbus_model_connection_names_list_cb,
                                   pd);
}

/**
 * @brief Gets a slice of the children (D-Bus service names).
 *
 * Implements Efl.Model.children_slice_get.
 * If the children have already been listed, it returns a resolved future
 * with the requested slice. Otherwise, it creates a promise, stores the
 * request, and initiates the listing of children if not already started.
 *
 * @param obj The Eo object.
 * @param pd The private data of the Eldbus_Model_Connection.
 * @param start The starting index of the slice.
 * @param count The number of items in the slice.
 * @return An Eina_Future that will be resolved with an Eina_Value array
 *         containing the children slice. The Eina_Value array will contain
 *         Eo pointers to Eldbus_Model_Object instances.
 *         Example of Eina_Value array structure:
 *         EINA_VALUE_TYPE_ARRAY
 *         {
 *           [0] = (Eo *) eldbus_model_object_representing_service_1,
 *           [1] = (Eo *) eldbus_model_object_representing_service_2,
 *           ...
 *         }
 */
static Eina_Future *
_eldbus_model_connection_efl_model_children_slice_get(Eo *obj,
                                                      Eldbus_Model_Connection_Data *pd,
                                                      unsigned int start,
                                                      unsigned int count)
{
   Eldbus_Children_Slice_Promise* slice;
   Eina_Promise *p;

   if (pd->is_listed)
     {
        Eina_Value v;

        v = efl_model_list_value_get(pd->childrens, start, count);
        return efl_loop_future_resolved(obj, v);
     }

   p = efl_loop_promise_new(obj);

   slice = calloc(1, sizeof (Eldbus_Children_Slice_Promise));
   slice->p = p;
   slice->start = start;
   slice->count = count;

   pd->requests = eina_list_prepend(pd->requests, slice);

   _eldbus_model_children_list(obj, pd);
   return efl_future_then(obj, eina_future_new(p));;
}

/**
 * @brief Gets the count of children (D-Bus service names).
 *
 * Implements Efl.Model.children_count_get.
 * Initiates the listing of children if not already started and returns
 * the current count of known children.
 *
 * @param obj The Eo object (const as it's not modified directly here).
 * @param pd The private data of the Eldbus_Model_Connection.
 * @return The number of children.
 */
static unsigned int
_eldbus_model_connection_efl_model_children_count_get(const Eo *obj,
                                                      Eldbus_Model_Connection_Data *pd)
{
   _eldbus_model_children_list(obj, pd);
   return eina_list_count(pd->childrens);
}

static void
_eldbus_model_connection_names_list_cb(void *data,
                                       const Eldbus_Message *msg,
                                       Eldbus_Pending *pending EINA_UNUSED)
{
   Eldbus_Model_Connection_Data *pd = (Eldbus_Model_Connection_Data*) data;
   Eldbus_Model_Data *sd;
   Eldbus_Children_Slice_Promise *slice;
   const char *error_name, *error_text;
   Eldbus_Message_Iter *array = NULL;
   const char *bus;

   pd->pending = NULL;

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "as", &array))
     {
        ERR("%s", "Error getting arguments.");
        return;
     }

   sd = efl_data_scope_get(pd->obj, ELDBUS_MODEL_CLASS);

   while (eldbus_message_iter_get_and_next(array, 's', &bus))
     {
        Eo *child;

        DBG("(%p): bus = %s", pd->obj, bus);

        child = efl_add(ELDBUS_MODEL_OBJECT_CLASS, pd->obj,
                        eldbus_model_connection_set(efl_added, sd->connection),
                        eldbus_model_object_bus_set(efl_added, bus),
                        eldbus_model_object_path_set(efl_added, "/"));

        pd->childrens = eina_list_append(pd->childrens, child);
     }

   pd->is_listed = EINA_TRUE;

   efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

   EINA_LIST_FREE(pd->requests, slice)
     {
        Eina_Value v;

        v = efl_model_list_value_get(pd->childrens,
                                     slice->start, slice->count);
        eina_promise_resolve(slice->p, v);

        free(slice);
     }
}

#include "eldbus_model_connection.eo.c"
