#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eldbus_model_object_private.h"
#include "eldbus_model_private.h"

#include <Ecore.h>
#include <Eina.h>

#define MY_CLASS ELDBUS_MODEL_OBJECT_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Object"

static void _eldbus_model_object_introspect_cb(void *, const Eldbus_Message *, Eldbus_Pending *);
static void _eldbus_model_object_create_children(Eldbus_Model_Object_Data *, Eldbus_Object *, Eina_List *);

/**
 * @internal
 * @brief EFL object constructor for Eldbus_Model_Object.
 *
 * Initializes the Eldbus_Model_Object instance.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Object.
 * @return The constructed Eo object.
 */
static Efl_Object*
_eldbus_model_object_efl_object_constructor(Eo *obj, Eldbus_Model_Object_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;

   return obj;
}

/**
 * @internal
 * @brief Sets the D-Bus bus name for the Eldbus_Model_Object.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @param bus The D-Bus bus name (e.g., "org.freedesktop.DBus").
 */
static void
_eldbus_model_object_bus_set(Eo *obj EINA_UNUSED,
                             Eldbus_Model_Object_Data *pd,
                             const char *bus)
{
   pd->bus = eina_stringshare_add(bus);
}

/**
 * @internal
 * @brief Sets the D-Bus object path for the Eldbus_Model_Object.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @param path The D-Bus object path (e.g., "/org/freedesktop/DBus").
 */
static void
_eldbus_model_object_path_set(Eo *obj EINA_UNUSED,
                              Eldbus_Model_Object_Data *pd,
                              const char *path)
{
   pd->path = eina_stringshare_add(path);
}

/**
 * @internal
 * @brief EFL object finalization for Eldbus_Model_Object.
 *
 * Ensures that the bus and path are set before finalizing the object.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Object.
 * @return The finalized Eo object, or NULL if bus or path is not set.
 */
static Efl_Object*
_eldbus_model_object_efl_object_finalize(Eo *obj, Eldbus_Model_Object_Data *pd)
{
   if (!pd->bus || !pd->path)
     return NULL;

   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief EFL object invalidation for Eldbus_Model_Object.
 *
 * Cleans up resources associated with the Eldbus_Model_Object,
 * such as children, pending D-Bus calls, and D-Bus objects.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Object.
 */
static void
_eldbus_model_object_efl_object_invalidate(Eo *obj, Eldbus_Model_Object_Data *pd)
{
   Eldbus_Pending *pending;
   Eldbus_Object *object;
   Eo *child;

   EINA_LIST_FREE(pd->childrens, child)
     efl_unref(child);

   EINA_LIST_FREE(pd->pendings, pending)
     eldbus_pending_cancel(pending);

   EINA_LIST_FREE(pd->objects, object)
     eldbus_object_unref(object);

   if (pd->introspection)
     {
        eldbus_introspection_node_free(pd->introspection);
        pd->introspection = NULL;
     }

   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief EFL object destructor for Eldbus_Model_Object.
 *
 * Frees stringshared bus and path.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Object.
 */
static void
_eldbus_model_object_efl_object_destructor(Eo *obj, Eldbus_Model_Object_Data *pd)
{
   eina_stringshare_del(pd->bus);
   eina_stringshare_del(pd->path);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Initiates D-Bus introspection for a given bus and path.
 *
 * This function retrieves the D-Bus object and sends an introspection request.
 * The result of the introspection is handled by _eldbus_model_object_introspect_cb.
 *
 * @param obj The Eo object.
 * @param pd The private data for the Eldbus_Model_Object.
 * @param bus The D-Bus bus name.
 * @param path The D-Bus object path.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_eldbus_model_object_introspect(const Eo *obj,
                                Eldbus_Model_Object_Data *pd,
                                const char *bus,
                                const char *path)
{
   Eldbus_Pending *pending;
   Eldbus_Object *object;

   DBG("(%p) Introspecting: bus = %s, path = %s", pd->obj, bus, path);

   object = eldbus_object_get(eldbus_model_connection_get(obj),
                              bus, path);
   if (!object)
     {
        ERR("(%p): Cannot get object: bus=%s, path=%s", pd->obj, bus, path);
        return EINA_FALSE;
     }
   pd->objects = eina_list_append(pd->objects, object);

   // TODO: Register for interface added/removed event
   pending = eldbus_object_introspect(object, &_eldbus_model_object_introspect_cb, pd);
   eldbus_pending_data_set(pending, "object", object);
   pd->pendings = eina_list_append(pd->pendings, pending);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Implements Efl_Model_Children_Slice_Get.
 *
 * Retrieves a slice of child objects. If introspection is not yet complete,
 * it queues the request and initiates introspection if not already pending.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @param start The starting index of the slice.
 * @param count The number of children to retrieve.
 * @return An Eina_Future that will resolve to an Eina_Value array of children.
 *         The Eina_Value array contains Eo pointers to the child objects.
 *         Example of resolved Eina_Value (type EINA_VALUE_TYPE_ARRAY):
 *         {
 *           type: EINA_VALUE_TYPE_OBJECT (Efl_Object *), value: child1_ptr
 *           type: EINA_VALUE_TYPE_OBJECT (Efl_Object *), value: child2_ptr
 *           ...
 *         }
 */
static Eina_Future *
_eldbus_model_object_efl_model_children_slice_get(Eo *obj EINA_UNUSED,
                                                  Eldbus_Model_Object_Data *pd,
                                                  unsigned start,
                                                  unsigned count)
{
   Eldbus_Children_Slice_Promise *slice;
   Eina_Promise *p;

   if (pd->is_listed)
     {
        Eina_Value v;

        v = efl_model_list_value_get(pd->childrens, start, count);
        return efl_loop_future_resolved(obj, v);
     }

   p = efl_loop_promise_new(obj);

   slice = calloc(1, sizeof(struct _Eldbus_Children_Slice_Promise));
   slice->p = p;
   slice->start = start;
   slice->count = count;

   pd->requests = eina_list_prepend(pd->requests, slice);

   if (!pd->pendings)
     _eldbus_model_object_introspect(obj, pd, pd->bus, pd->path);
   return efl_future_then(obj, eina_future_new(p));;
}

/**
 * @internal
 * @brief Implements Efl_Model_Children_Count_Get.
 *
 * Returns the number of child objects. If introspection is not yet complete,
 * it initiates introspection if not already pending.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @return The number of child objects.
 */
static unsigned int
_eldbus_model_object_efl_model_children_count_get(const Eo *obj EINA_UNUSED,
                                                  Eldbus_Model_Object_Data *pd)
{
   if (!pd->is_listed && !pd->pendings)
     _eldbus_model_object_introspect(obj, pd, pd->bus, pd->path);
   return eina_list_count(pd->childrens);
}

/**
 * @internal
 * @brief Gets the D-Bus bus name of the Eldbus_Model_Object.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @return The D-Bus bus name.
 */
static const char *
_eldbus_model_object_bus_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Object_Data *pd)
{
   return pd->bus;
}

/**
 * @internal
 * @brief Gets the D-Bus object path of the Eldbus_Model_Object.
 *
 * @param obj The Eo object (unused).
 * @param pd The private data for the Eldbus_Model_Object.
 * @return The D-Bus object path.
 */
static const char *
_eldbus_model_object_path_get(const Eo *obj EINA_UNUSED, Eldbus_Model_Object_Data *pd)
{
   return pd->path;
}

/**
 * @internal
 * @brief Concatenates a root path and a relative path to form an absolute D-Bus path.
 *
 * Handles the special case where the root path is "/".
 * Example:
 *   _eldbus_model_object_concatenate_path("/org/example", "Node") -> "/org/example/Node"
 *   _eldbus_model_object_concatenate_path("/", "Node") -> "/Node"
 *
 * @param root_path The root D-Bus path.
 * @param relative_path The relative D-Bus path.
 * @return A newly allocated string containing the absolute path. The caller must free this string.
 *         Returns NULL on allocation failure.
 */
static char *
_eldbus_model_object_concatenate_path(const char *root_path,
                                      const char *relative_path)
{
   Eina_Strbuf *buffer;
   const char *format = (!eina_streq(root_path, "/")) ? "%s/%s" : "%s%s";
   char *absolute_path = NULL;

   buffer = eina_strbuf_new();
   eina_strbuf_append_printf(buffer, format, root_path, relative_path);
   absolute_path = eina_strbuf_string_steal(buffer);

   eina_strbuf_free(buffer);
   return absolute_path;
}

/**
 * @internal
 * @brief Recursively introspects child nodes found in D-Bus introspection data.
 *
 * For each node in the provided list, it constructs the absolute path and
 * initiates a new introspection request for that path.
 *
 * @param pd The private data for the Eldbus_Model_Object.
 * @param current_path The D-Bus path of the parent object currently being introspected.
 * @param nodes A list of Eldbus_Introspection_Node representing child nodes.
 */
static void
_eldbus_model_object_introspect_nodes(Eldbus_Model_Object_Data *pd,
                                      const char *current_path,
                                      Eina_List *nodes)
{
   Eldbus_Introspection_Node *node;
   Eina_List *it;

   EINA_LIST_FOREACH(nodes, it, node)
     {
        const char *relative_path;
        char *absolute_path;

        relative_path = node->name;
        if (!relative_path) continue;

        absolute_path = _eldbus_model_object_concatenate_path(current_path, relative_path);
        if (!absolute_path) continue;

        _eldbus_model_object_introspect(pd->obj, pd, pd->bus, absolute_path);

        free(absolute_path);
     }
}

/**
 * @internal
 * @brief Creates child proxy objects based on the interfaces found during introspection.
 *
 * For each interface, a new ELDBUS_MODEL_PROXY_CLASS instance is created and
 * added to the list of children.
 *
 * @param pd The private data for the Eldbus_Model_Object.
 * @param object The Eldbus_Object corresponding to the introspected D-Bus object.
 * @param interfaces A list of Eldbus_Introspection_Interface found for the object.
 */
static void
_eldbus_model_object_create_children(Eldbus_Model_Object_Data *pd, Eldbus_Object *object, Eina_List *interfaces)
{
   Eldbus_Introspection_Interface *interface;
   const char *current_path;
   Eina_List *l;

   current_path = eldbus_object_path_get(object);
   if (!current_path) return ;

   EINA_LIST_FOREACH(interfaces, l, interface)
     {
        Eo *child;

        DBG("(%p) Creating child: bus = %s, path = %s, interface = %s",
            pd->obj, pd->bus, current_path, interface->name);

        // TODO: increment reference to keep 'interface' in memory
        child = efl_add_ref(ELDBUS_MODEL_PROXY_CLASS, pd->obj,
                            eldbus_model_proxy_object_set(efl_added, object),
                            eldbus_model_proxy_interface_set(efl_added, interface));

        if (child) pd->childrens = eina_list_append(pd->childrens, child);
     }
}

/**
 * @internal
 * @brief Callback function for D-Bus introspection results.
 *
 * This function is called when an introspection request completes. It parses
 * the introspection XML, creates child objects for interfaces, introspects
 * child nodes, and resolves any pending children slice requests.
 *
 * @param data User data, expected to be Eldbus_Model_Object_Data*.
 * @param msg The D-Bus message containing the introspection result or error.
 * @param pending The Eldbus_Pending object associated with the introspection request.
 */
static void
_eldbus_model_object_introspect_cb(void *data,
                                   const Eldbus_Message *msg,
                                   Eldbus_Pending *pending)
{
   Eldbus_Model_Object_Data *pd = (Eldbus_Model_Object_Data*)data;
   Eldbus_Children_Slice_Promise* slice;
   Eldbus_Object *object;
   const char *error_name;
   const char *error_text;
   const char *xml = NULL;
   const char *current_path;

   pd->pendings = eina_list_remove(pd->pendings, pending);
   object = eldbus_pending_data_get(pending, "object");

   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        //efl_model_error_notify(pd->obj);
        return;
     }

   if (!eldbus_message_arguments_get(msg, "s", &xml))
     {
        ERR("Error getting arguments.");
        return;
     }

   if (!xml)
     {
        ERR("No XML.");
        return ;
     }

   current_path = eldbus_object_path_get(object);
   pd->introspection = eldbus_introspection_parse(xml);

   DBG("(%p): introspect of bus = %s, path = %s =>\n%s", pd->obj, pd->bus, current_path, xml);

   _eldbus_model_object_introspect_nodes(pd, current_path, pd->introspection->nodes);
   _eldbus_model_object_create_children(pd, object, pd->introspection->interfaces);

   if (eina_list_count(pd->pendings) != 0) return ;

   efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

   pd->is_listed = EINA_TRUE;

   EINA_LIST_FREE(pd->requests, slice)
     {
        Eina_Value v;

        v = efl_model_list_value_get(pd->childrens, slice->start, slice->count);
        eina_promise_resolve(slice->p, v);

        free(slice);
     }
}

#include "eldbus_model_object.eo.c"
