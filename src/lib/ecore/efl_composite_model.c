#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Efl.h>
#include <Ecore.h>

#include "ecore_private.h"
#include "efl_composite_model.eo.h"

typedef struct _Efl_Composite_Model_Data Efl_Composite_Model_Data;

struct _Efl_Composite_Model_Data
{
   EINA_RBTREE;

   Efl_Composite_Model *self;
   Efl_Model *source;
   Eina_Rbtree *indexed;

   unsigned int index;

   Eina_Bool need_index : 1;
   Eina_Bool set_index : 1;
   Eina_Bool inserted : 1;
};

/**
 * @brief Compares two Efl_Composite_Model_Data nodes based on their index.
 *
 * This function is used for inserting and sorting nodes in the indexed rbtree.
 *
 * @param left The left node to compare.
 * @param right The right node to compare.
 * @param data User data (unused).
 * @return EINA_RBTREE_LEFT if left->index < right->index, EINA_RBTREE_RIGHT otherwise.
 */
static Eina_Rbtree_Direction
_children_indexed_cmp(const Efl_Composite_Model_Data *left,
                      const Efl_Composite_Model_Data *right,
                      void *data EINA_UNUSED)
{
   if (left->index < right->index)
     return EINA_RBTREE_LEFT;
   return EINA_RBTREE_RIGHT;
}

/**
 * @brief Compares an Efl_Composite_Model_Data node's index with a given key.
 *
 * This function is used for looking up nodes in the indexed rbtree by their index.
 *
 * @param node The node to compare.
 * @param key Pointer to the integer key (index) to compare against.
 * @param length Length of the key (unused).
 * @param data User data (unused).
 * @return 1 if node->index > *key, -1 if node->index < *key, 0 if equal.
 */
static int
_children_indexed_key(const Efl_Composite_Model_Data *node,
                      const int *key, int length EINA_UNUSED, void *data EINA_UNUSED)
{
   if (node->index > (unsigned int) *key) return 1;
   if (node->index < (unsigned int) *key) return -1;
   return 0;
}

/**
 * @brief Recursively traverses an rbtree and adds nodes with index > upper to the mark array.
 *
 * This is used to identify children whose indices need to be updated after an
 * insertion or deletion.
 *
 * @param root The current root of the rbtree (or subtree) to traverse.
 * @param mark An Eina_Array to store pointers to Efl_Composite_Model_Data nodes
 *             that have an index greater than 'upper'.
 * @param upper The threshold index. Nodes with an index greater than this will be marked.
 */
static void
_mark_greater(Efl_Composite_Model_Data *root, Eina_Array *mark, const unsigned int upper)
{
   if (!root) return ;

   if (root->index > upper)
     {
        eina_array_push(mark, root);
        _mark_greater((void*) EINA_RBTREE_GET(root)->son[0], mark, upper);
        _mark_greater((void*) EINA_RBTREE_GET(root)->son[1], mark, upper);
     }
   else
     {
        _mark_greater((void*) EINA_RBTREE_GET(root)->son[0], mark, upper);
     }
}

/**
 * @brief Handles the invalidation of a composite model object.
 *
 * If the object was inserted into a parent composite model's indexed tree,
 * it removes itself from that tree. This is crucial to prevent dangling
 * pointers or incorrect state when an object is being destroyed or removed.
 *
 * @param obj The composite model object being invalidated.
 * @param pd The private data of the composite model.
 */
static void
_efl_composite_model_efl_object_invalidate(Eo *obj, Efl_Composite_Model_Data *pd)
{
   if (pd->inserted)
     {
        Eo *parent;

        parent = efl_parent_get(obj);
        if (efl_isa(parent, EFL_COMPOSITE_MODEL_CLASS))
          {
             Efl_Composite_Model_Data *ppd;

             ppd = efl_data_scope_get(parent, EFL_COMPOSITE_MODEL_CLASS);
             ppd->indexed = eina_rbtree_inline_remove(ppd->indexed, EINA_RBTREE_GET(pd),
                                                      EINA_RBTREE_CMP_NODE_CB(_children_indexed_cmp), NULL);
             pd->inserted = EINA_FALSE;
          }
        else
          {
             ERR("Unexpected parent change during the life of object: %s this might lead to crash.", efl_debug_name_get(obj));
          }
     }

   efl_invalidate(efl_super(obj, EFL_COMPOSITE_MODEL_CLASS));
}

/**
 * @brief Finalizes the composite model object.
 *
 * This function is called after the object has been constructed. It ensures
 * that the source model is set. If the object has a parent that is also a
 * composite model, it attempts to insert itself into the parent's indexed
 * rbtree of children. If an object already exists at the specified index in
 * the parent, this function returns the existing object instead of finalizing
 * a new one, preventing duplicates.
 *
 * @param obj The composite model object being finalized.
 * @param pd The private data of the composite model.
 * @return The finalized Efl_Object, or NULL if the source model was not set,
 *         or an existing object if a duplicate index was found in the parent.
 */
static Efl_Object *
_efl_composite_model_efl_object_finalize(Eo *obj, Efl_Composite_Model_Data *pd)
{
   Eo *parent;

   if (pd->source == NULL)
     {
        ERR("Source of the composite model wasn't defined at construction time.");
        return NULL;
     }

   pd->self = obj;

   parent = efl_parent_get(obj);
   if (efl_isa(parent, EFL_COMPOSITE_MODEL_CLASS) && !pd->inserted)
     {
        Efl_Composite_Model_Data *ppd = efl_data_scope_get(parent, EFL_COMPOSITE_MODEL_CLASS);
        Efl_Composite_Model_Data *lookup;

        lookup = (void*) eina_rbtree_inline_lookup(ppd->indexed, &pd->index, sizeof (int),
                                                   EINA_RBTREE_CMP_KEY_CB(_children_indexed_key), NULL);
        if (lookup)
          {
             // There is already an object at this index, we should not
             // build anything different than what exist. Returning existing one.
             return lookup->self;
          }
        else
          {
             ppd->indexed = eina_rbtree_inline_insert(ppd->indexed, EINA_RBTREE_GET(pd),
                                                      EINA_RBTREE_CMP_NODE_CB(_children_indexed_cmp), NULL);

             pd->inserted = EINA_TRUE;
          }
     }

   return obj;
}

/**
 * @brief Sets the index of the composite model child.
 *
 * This function is typically called when a composite child is created to
 * represent an underlying source model's child at a specific index.
 * The index can only be set once and only if a source model is present.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @param index The index to set for this composite child.
 */
static void
_efl_composite_model_index_set(Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd, unsigned int index)
{
   if (pd->set_index || !pd->source)
     return ;
   pd->index = index;
   pd->set_index = EINA_TRUE;
}

/**
 * @brief Gets the index of the composite model child.
 *
 * If the index was explicitly set, it returns that. Otherwise, if the
 * composite model needs to determine its index (pd->need_index is false),
 * it tries to fetch the EFL_COMPOSITE_MODEL_CHILD_INDEX property from itself.
 * This property might be provided by the source model or set by other means.
 *
 * @param obj The composite model object.
 * @param pd The private data of the composite model.
 * @return The index of the child, or 0xFFFFFFFF if not determinable or an error occurs.
 */
static unsigned int
_efl_composite_model_index_get(const Eo *obj, Efl_Composite_Model_Data *pd)
{
   Eina_Value *fetch = NULL;
   unsigned int r = 0xFFFFFFFF;

   if (pd->set_index)
     return pd->index;
   if (pd->need_index)
     return 0xFFFFFFFF;

   fetch = efl_model_property_get(obj, EFL_COMPOSITE_MODEL_CHILD_INDEX);
   if (!eina_value_uint_convert(fetch, &r))
     return 0xFFFFFFFF;
   eina_value_free(fetch);

   return r;
}

/**
 * @brief Handles child added/removed events from the source model.
 *
 * This function is the core logic for reacting to changes in the source model's
 * children. It performs several key actions:
 * 1. Looks up or creates a composite model representation for the affected child.
 * 2. If a child is removed, it's also removed from this composite model's internal
 *    indexed rbtree and its source model reference is cleared.
 * 3. Identifies all sibling composite children whose indices are affected by the
 *    addition/removal.
 * 4. Updates the indices of these affected siblings.
 * 5. Emits the corresponding EFL_MODEL_EVENT_CHILD_ADDED or
 *    EFL_MODEL_EVENT_CHILD_REMOVED event for the composite model itself.
 * 6. Notifies observers that the EFL_COMPOSITE_MODEL_CHILD_INDEX property has
 *    changed for the affected siblings.
 *
 * @param pd The private data of the parent composite model.
 * @param ev The Efl_Model_Children_Event from the source model.
 * @param description The event description (EFL_MODEL_EVENT_CHILD_ADDED or EFL_MODEL_EVENT_CHILD_REMOVED).
 */
static void
_efl_composite_model_child_event(Efl_Composite_Model_Data *pd,
                                 const Efl_Model_Children_Event *ev,
                                 const Efl_Event_Description *description)
{
   Efl_Composite_Model_Data *cpd;
   Efl_Model_Children_Event cev = { 0 };
   Eina_Array mark;
   Eina_Array_Iterator iterator;
   unsigned int i;

   cev.index = ev->index;
   if (ev->child)
     {
        cev.child = _efl_composite_lookup(efl_class_get(pd->self),
                                          pd->self, ev->child, ev->index);
     }
   else
     {
        cpd = (void*) eina_rbtree_inline_lookup(pd->indexed, &cev.index, sizeof (unsigned int),
                                                EINA_RBTREE_CMP_KEY_CB(_children_indexed_key), NULL);
        if (cpd) cev.child = efl_ref(cpd->self);
     }

   if (cev.child && description == EFL_MODEL_EVENT_CHILD_REMOVED)
     {
        cpd = efl_data_scope_get(cev.child, EFL_COMPOSITE_MODEL_CLASS);

        // Remove child from lookup tree if it exist before triggering anything further
        pd->indexed = eina_rbtree_inline_remove(pd->indexed, EINA_RBTREE_GET(cpd),
                                                EINA_RBTREE_CMP_NODE_CB(_children_indexed_cmp), NULL);
        cpd->inserted = EINA_FALSE;
        efl_replace(&cpd->source, NULL);
     }

   // Update all index above this one if necessaryy
   eina_array_step_set(&mark, sizeof (Eina_Array), 8);
   _mark_greater((void*) pd->indexed, &mark, cev.index);

   // Correct index of the object stored that need to
   // There is no need to remove and reinsert them as their relative order will not change.
   EINA_ARRAY_ITER_NEXT(&mark, i, cpd, iterator)
     {
        if (description == EFL_MODEL_EVENT_CHILD_REMOVED) cpd->index--;
        else cpd->index++;

        efl_ref(cpd->self);
     }

   efl_event_callback_call(pd->self, description, &cev);

   // Notify of the index change only after notifying of the removal top avoid overlap
   EINA_ARRAY_ITER_NEXT(&mark, i, cpd, iterator)
      {
         efl_model_properties_changed(cpd->self, EFL_COMPOSITE_MODEL_CHILD_INDEX);
         efl_unref(cpd->self);
      }
   eina_array_flush(&mark);

   efl_unref(cev.child);
}

/**
 * @brief Callback for EFL_MODEL_EVENT_CHILD_ADDED events from the source model.
 *
 * Simply forwards the event to _efl_composite_model_child_event.
 *
 * @param data The private data of the composite model (Efl_Composite_Model_Data *).
 * @param event The Efl_Event containing Efl_Model_Children_Event info.
 */
static void
_efl_composite_model_child_added(void *data, const Efl_Event *event)
{
   Efl_Composite_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;

   _efl_composite_model_child_event(pd, ev, EFL_MODEL_EVENT_CHILD_ADDED);
}

/**
 * @brief Callback for EFL_MODEL_EVENT_CHILD_REMOVED events from the source model.
 *
 * Simply forwards the event to _efl_composite_model_child_event.
 *
 * @param data The private data of the composite model (Efl_Composite_Model_Data *).
 * @param event The Efl_Event containing Efl_Model_Children_Event info.
 */
static void
_efl_composite_model_child_removed(void *data, const Efl_Event *event)
{
   Efl_Composite_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;

   _efl_composite_model_child_event(pd, ev, EFL_MODEL_EVENT_CHILD_REMOVED);
}

EFL_CALLBACKS_ARRAY_DEFINE(composite_callbacks,
                           { EFL_MODEL_EVENT_CHILD_ADDED, _efl_composite_model_child_added },
                           { EFL_MODEL_EVENT_CHILD_REMOVED, _efl_composite_model_child_removed });

/**
 * @brief Sets the source model for this composite model.
 *
 * This is a critical step in initializing the composite model. Once set, the
 * composite model will listen for events (child added/removed, count changed,
 * properties changed) from the source model and reflect these changes.
 * It also checks if the source model inherently provides child indices
 * (EFL_COMPOSITE_MODEL_CHILD_INDEX property). If not, the composite model
 * will need to manage these indices itself (pd->need_index = EINA_TRUE).
 * The source model can only be set once.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @param model The source Efl_Model to be composited.
 */
static void
_efl_composite_model_efl_ui_view_model_set(Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd, Efl_Model *model)
{
   Eina_Iterator *properties;
   const char *property;

   if (pd->source != NULL)
     {
        ERR("Source already set for composite model. It can only be set once.");
        return ;
     }
   pd->source = efl_ref(model);

   efl_event_callback_array_add(model, composite_callbacks(), pd);
   efl_event_callback_forwarder_priority_add(model, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, EFL_CALLBACK_PRIORITY_BEFORE, obj);
   efl_event_callback_forwarder_priority_add(model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, EFL_CALLBACK_PRIORITY_BEFORE, obj);

   pd->need_index = EINA_TRUE;

   properties = efl_model_properties_get(pd->source);
   EINA_ITERATOR_FOREACH(properties, property)
     {
        if (eina_streq(property, EFL_COMPOSITE_MODEL_CHILD_INDEX))
          {
             pd->need_index = EINA_FALSE;
             break;
          }
     }
   eina_iterator_free(properties);
}

/**
 * @brief Gets the source model for this composite model.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @return The source Efl_Model.
 */
static Efl_Model *
_efl_composite_model_efl_ui_view_model_get(const Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd)
{
   return pd->source;
}

/**
 * @brief Sets a property on the composite model.
 *
 * If the property is EFL_COMPOSITE_MODEL_CHILD_INDEX and the composite model
 * is managing indices itself (pd->need_index is true), this function handles
 * setting the index internally. This is typically done during the creation
 * of a composite child to assign it an initial index.
 * Otherwise, the property set request is forwarded to the source model.
 *
 * @param obj The composite model object.
 * @param pd The private data of the composite model.
 * @param property The name of the property to set.
 * @param value The Eina_Value containing the new property value.
 * @return An Eina_Future that resolves with the set value or rejects with an error.
 *         Returns EFL_MODEL_ERROR_READ_ONLY if trying to set index when already set or no source.
 *         Returns EFL_MODEL_ERROR_UNKNOWN if value conversion for index fails.
 */
static Eina_Future *
_efl_composite_model_efl_model_property_set(Eo *obj, Efl_Composite_Model_Data *pd,
                                            const char *property, Eina_Value *value)
{
   if (pd->need_index && eina_streq(property, EFL_COMPOSITE_MODEL_CHILD_INDEX))
     {
        if (pd->set_index || !pd->source)
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
        if (!eina_value_uint_convert(value, &pd->index))
          return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_UNKNOWN);
        pd->set_index = EINA_TRUE;
        return efl_loop_future_resolved(obj, eina_value_uint_init(pd->index));
     }
   return efl_model_property_set(pd->source, property, value);
}

/**
 * @brief Gets a property from the composite model.
 *
 * If the property is EFL_COMPOSITE_MODEL_CHILD_INDEX and the composite model
 * is managing indices (pd->need_index is true), this function returns the
 * internally stored index. If the index hasn't been set yet, it returns an
 * error value with EAGAIN, indicating the value is not yet available.
 * Otherwise, it first tries to get the property from the superclass. If the
 * superclass doesn't implement it (returns EINA_ERROR_NOT_IMPLEMENTED),
 * it then attempts to get the property from the source model.
 *
 * @param obj The composite model object (unused for direct property access, but used for super call).
 * @param pd The private data of the composite model.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value, or an Eina_Value of
 *         type error if the property is not found or an error occurs.
 *         Specifically, returns eina_value_error_new(EAGAIN) if CHILD_INDEX is
 *         requested but not yet set.
 */
static Eina_Value *
_efl_composite_model_efl_model_property_get(const Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd,
                                            const char *property)
{
   Eina_Value *try;
   if (pd->need_index && eina_streq(property, EFL_COMPOSITE_MODEL_CHILD_INDEX))
     {
        if (pd->set_index)
          return eina_value_uint_new(pd->index);
        return eina_value_error_new(EAGAIN);
     }
   try = efl_model_property_get(efl_super(obj, EFL_COMPOSITE_MODEL_CLASS), property);
   if (eina_value_type_get(try) == EINA_VALUE_TYPE_ERROR)
     {
        Eina_Error err = EINA_ERROR_NOT_IMPLEMENTED;

        if (eina_value_error_get(try, &err) && (err == EINA_ERROR_NOT_IMPLEMENTED))
          {
             eina_value_free(try);
             return efl_model_property_get(pd->source, property);
          }
     }
   return try;
}

/**
 * @brief Gets an iterator for all properties of the composite model.
 *
 * If the composite model is managing child indices (pd->need_index is true),
 * it returns a multi-iterator that combines the properties from the source
 * model with the EFL_COMPOSITE_MODEL_CHILD_INDEX property.
 * Otherwise, it simply returns an iterator for the properties of the source model.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @return An Eina_Iterator for the property names (strings).
 */
static Eina_Iterator *
_efl_composite_model_efl_model_properties_get(const Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd)
{
   if (pd->need_index)
     {
        static const char *composite_properties[] = {
          EFL_COMPOSITE_MODEL_CHILD_INDEX
        };

        return eina_multi_iterator_new(efl_model_properties_get(pd->source),
                                       EINA_C_ARRAY_ITERATOR_NEW(composite_properties));
     }
   return efl_model_properties_get(pd->source);
}

/**
 * @brief Gets the number of children in the composite model.
 *
 * This count is directly derived from the children count of the source model.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @return The number of children.
 */
static unsigned int
_efl_composite_model_efl_model_children_count_get(const Eo *obj EINA_UNUSED, Efl_Composite_Model_Data *pd)
{
   return efl_model_children_count_get(pd->source);
}

typedef struct _Efl_Composite_Model_Slice_Request Efl_Composite_Model_Slice_Request;
struct _Efl_Composite_Model_Slice_Request
{
   const Efl_Class *self;
   Eo *parent;
   unsigned int start;
   unsigned int dummy_need;
};

/**
 * @brief Callback executed when the source model's children_slice_get future resolves.
 *
 * This function processes the slice of children obtained from the source model.
 * For each child model in the slice, it looks up or creates a corresponding
 * composite model object. These composite objects are then added to a new
 * Eina_Value array.
 * If `dummy_need` in the request is greater than zero, it means the requested
 * slice extended beyond the source model's actual children. In this case,
 * "dummy" generic models and their composite proxies are created and added
 * to the result array to fulfill the requested count.
 *
 * @param o The object associated with the future (unused).
 * @param data Pointer to Efl_Composite_Model_Slice_Request containing details
 *             about the original slice request.
 * @param v An Eina_Value of type array, where each element is an Efl_Model
 *          object from the source model's slice.
 *          Example `v` (EINA_VALUE_TYPE_ARRAY of Efl_Model*):
 *          `[child_model_A, child_model_B, ...]`
 * @return An Eina_Value of type array, containing the composite model objects
 *         corresponding to the source model's children, plus any dummy objects.
 *         Example return (EINA_VALUE_TYPE_ARRAY of Efl_Composite_Model*):
 *         `[composite_proxy_A, composite_proxy_B, dummy_proxy_C, ...]`
 */
static Eina_Value
_efl_composite_model_then(Eo *o EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Composite_Model_Slice_Request *req = data;
   unsigned int i, len;
   Eina_Value r = EINA_VALUE_EMPTY;
   Eo *target = NULL;

   eina_value_array_setup(&r, EINA_VALUE_TYPE_OBJECT, 4);

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     {
        Eo *composite;

        // Fetch an existing composite model for this model or create a new one if none exist
        composite = _efl_composite_lookup(req->self, req->parent, target, req->start + i);
        if (!composite) continue;

        eina_value_array_append(&r, composite);
        // Dropping this scope reference
        efl_unref(composite);
     }

   while (req->dummy_need)
     {
        Eo *dummy, *dummy_proxy;

        // Create a dummy object and its proxy
        dummy = efl_add(EFL_GENERIC_MODEL_CLASS, req->parent);
        dummy_proxy = efl_add_ref(req->self, req->parent,
                                  efl_ui_view_model_set(efl_added, dummy),
                                  efl_composite_model_index_set(efl_added, req->start + i),
                                  efl_loop_model_volatile_make(efl_added));
        efl_parent_set(dummy, dummy_proxy);

        eina_value_array_append(&r, dummy_proxy);
        efl_unref(dummy_proxy);

        req->dummy_need--;
        i++;
     }

   return r;
}

/**
 * @brief Cleans up resources associated with a slice request.
 *
 * This function is called when the future returned by
 * `_efl_composite_model_efl_model_children_slice_get` is freed (e.g.,
 * after it resolves or is cancelled). It unrefs the parent object that was
 * reffed for the duration of the asynchronous operation and frees the
 * Efl_Composite_Model_Slice_Request structure.
 *
 * @param o The object associated with the future (unused).
 * @param data Pointer to Efl_Composite_Model_Slice_Request to be freed.
 * @param dead_future The future that is being cleaned up (unused).
 */
static void
_efl_composite_model_clean(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Composite_Model_Slice_Request *req = data;

   efl_unref(req->parent);
   free(data);
}

/**
 * @brief Retrieves a slice of children from the composite model.
 *
 * This function handles requests for a range of child objects.
 *
 * Behavior:
 * 1. If the requested slice is entirely outside the bounds of both the source
 *    model's children and the composite model's own potential children (e.g.,
 *    if `start` is excessively large), it returns a rejected future.
 * 2. If the requested slice starts beyond the actual children of the source model
 *    but is within a range where "dummy" children might be expected (based on
 *    `efl_model_children_count_get(obj)` which might be overridden by a subclass
 *    to be larger than source_count), it synchronously creates and returns
 *    dummy generic models wrapped in composite proxies for the requested range.
 *    These dummies are marked as volatile.
 * 3. Otherwise (the common case where the slice overlaps with actual source children),
 *    it requests a slice from the source model.
 *    - It calculates `req_count`, which is the number of children to request from
 *      the source (capped at `source_count - start`).
 *    - It then uses `efl_future_then` to process the result of the source model's
 *      slice request via `_efl_composite_model_then`.
 *    - `dummy_need` is calculated to determine if additional dummy children need
 *      to be appended by `_efl_composite_model_then` to fulfill the original `count`
 *      if `start + count` extends beyond `source_count`.
 *
 * The `Efl_Composite_Model_Slice_Request` structure is used to pass necessary
 * context (like the original `start` index and `dummy_need`) to the
 * `_efl_composite_model_then` callback.
 *
 * @param obj The composite model object from which to get the slice.
 * @param pd The private data of the composite model.
 * @param start The starting index of the slice.
 * @param count The number of children in the slice.
 * @return An Eina_Future that will resolve with an Eina_Value array of
 *         Efl_Object (composite children) or reject with an error.
 *         The resolved array will contain Efl_Composite_Model instances.
 *         Example resolved value (EINA_VALUE_TYPE_ARRAY of Efl_Composite_Model*):
 *         `[composite_child_1, composite_child_2, ..., composite_child_N]`
 */
static Eina_Future *
_efl_composite_model_efl_model_children_slice_get(Eo *obj,
                                                  Efl_Composite_Model_Data *pd,
                                                  unsigned int start,
                                                  unsigned int count)
{
   Efl_Composite_Model_Slice_Request *req;
   Eina_Future *f;
   unsigned int source_count, self_count;
   unsigned int req_count;

   source_count = efl_model_children_count_get(pd->source);
   self_count = efl_model_children_count_get(obj);

   if (start + count > source_count &&
       start + count > self_count)
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

   if (start > source_count)
     {
        Eina_Value r = EINA_VALUE_EMPTY;
        unsigned int i = 0;

        eina_value_array_setup(&r, EINA_VALUE_TYPE_OBJECT, 4);

        while (count)
          {
             Eo *dummy, *dummy_proxy;

             // Create a dummy object and its proxy
             dummy = efl_add(EFL_GENERIC_MODEL_CLASS, obj);
             dummy_proxy = efl_add_ref(efl_class_get(obj), obj,
                                       efl_ui_view_model_set(efl_added, dummy),
                                       efl_composite_model_index_set(efl_added, start + i),
                                       efl_loop_model_volatile_make(efl_added));
             efl_parent_set(dummy, dummy_proxy);

             eina_value_array_append(&r, dummy_proxy);
             efl_unref(dummy_proxy);

             count--;
             i++;
          }

        return efl_loop_future_resolved(obj, r);
     }

   req_count = start + count > source_count ? source_count - start : count;
   f = efl_model_children_slice_get(pd->source, start, req_count);
   if (!f) return NULL;

   req = malloc(sizeof (Efl_Composite_Model_Slice_Request));
   if (!req) return efl_loop_future_rejected(obj, ENOMEM);

   req->self = efl_class_get(obj);
   req->parent = efl_ref(obj);
   req->start = start;
   if (start + count < source_count)
     req->dummy_need = 0;
   else
     req->dummy_need = count - (source_count - start);

   return efl_future_then(obj, f, .success_type = EINA_VALUE_TYPE_ARRAY,
                          .success = _efl_composite_model_then,
                          .free = _efl_composite_model_clean,
                          .data = req);
}

/**
 * @brief Adds a new child to the composite model.
 *
 * This operation is forwarded directly to the source model. The source model
 * is responsible for creating the actual child data. The composite model will
 * then typically receive a `CHILD_ADDED` event from the source model, at which
 * point it will create its corresponding composite proxy object.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @return The new child object created by the source model, or NULL on failure.
 *         Note: This is the source model's child, not the composite proxy.
 */
static Efl_Object *
_efl_composite_model_efl_model_child_add(Eo *obj EINA_UNUSED,
                                         Efl_Composite_Model_Data *pd)
{
   return efl_model_child_add(pd->source);
}

/**
 * @brief Deletes a child from the composite model.
 *
 * This operation is forwarded to the source model. The actual child to be
 * deleted is the source model associated with the given composite `child` proxy.
 * The composite model will then typically receive a `CHILD_REMOVED` event from
 * the source model, at which point it will clean up its corresponding composite
 * proxy object.
 *
 * @param obj The composite model object (unused).
 * @param pd The private data of the composite model.
 * @param child The composite proxy Efl_Object representing the child to delete.
 *              The actual deletion target is `efl_ui_view_model_get(child)`.
 */
static void
_efl_composite_model_efl_model_child_del(Eo *obj EINA_UNUSED,
                                         Efl_Composite_Model_Data *pd,
                                         Efl_Object *child)
{
   efl_model_child_del(pd->source, efl_ui_view_model_get(child));
}

/**
 * @brief Destructor for the composite model object.
 *
 * Cleans up resources held by the composite model. This includes:
 * - Removing event callbacks and forwarders from the source model.
 * - Releasing the reference to the source model.
 * It then calls the superclass's destructor.
 *
 * @param obj The composite model object being destructed.
 * @param pd The private data of the composite model.
 */
static void
_efl_composite_model_efl_object_destructor(Eo *obj, Efl_Composite_Model_Data *pd)
{
   if (pd->source)
     {
        efl_event_callback_array_del(pd->source, composite_callbacks(), pd);
        efl_event_callback_forwarder_del(pd->source, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, obj);
        efl_event_callback_forwarder_del(pd->source, EFL_MODEL_EVENT_PROPERTIES_CHANGED, obj);

        efl_replace(&pd->source, NULL);
     }

   efl_destructor(efl_super(obj, EFL_COMPOSITE_MODEL_CLASS));
}

#include "efl_composite_model.eo.c"
