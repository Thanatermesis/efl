#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl_Core.h>

#include "ecore_internal.h"

/**
 * @brief Structure to hold the mapping between original and filtered indices.
 * This is used in an Eina_Rbtree to efficiently look up mappings.
 */
typedef struct _Efl_Filter_Model_Mapping Efl_Filter_Model_Mapping;
struct _Efl_Filter_Model_Mapping
{
   EINA_RBTREE; /**< Macro to make this struct usable as an Rbtree node. */

   unsigned int original; /**< The original index in the source model. */
   unsigned int mapped;   /**< The new index in the filter model. */

   EINA_REFCOUNT; /**< Reference count for managing the lifecycle of this mapping. */
};

/**
 * @brief Internal data structure for the Efl_Filter_Model.
 */
typedef struct _Efl_Filter_Model_Data Efl_Filter_Model_Data;
struct _Efl_Filter_Model_Data
{
   Efl_Filter_Model_Mapping *self; /**< Mapping information if this model instance itself is a filtered child. */

   Eina_Rbtree *mapping; /**< Rbtree storing Efl_Filter_Model_Mapping, keyed by mapped index. */

   struct {
      void *data; /**< User data for the filter callback. */
      EflFilterModel cb; /**< The filter callback function. */
      Eina_Free_Cb free_cb; /**< Callback to free the filter_data. */
      unsigned int count; /**< Number of items that passed the filter. */
   } filter; /**< Filter specific data. */

   unsigned int counted; /**< Total number of children in the source model, fetched when counting starts. */
   Eina_Bool counting_started : 1; /**< Flag to indicate if the initial counting of source model children has begun. */
   Eina_Bool processed : 1; /**< Flag to indicate if this specific child model has been processed by the filter. */
};

/**
 * @brief Comparison callback for the Rbtree storing Efl_Filter_Model_Mapping.
 * Compares two mappings based on their 'mapped' index.
 *
 * @param left The left Eina_Rbtree node (Efl_Filter_Model_Mapping).
 * @param right The right Eina_Rbtree node (Efl_Filter_Model_Mapping).
 * @param data User data (unused).
 * @return EINA_RBTREE_LEFT if left->mapped < right->mapped, EINA_RBTREE_RIGHT otherwise.
 */
static Eina_Rbtree_Direction
_filter_mapping_cmp_cb(const Eina_Rbtree *left, const Eina_Rbtree *right, void *data EINA_UNUSED)
{
   const Efl_Filter_Model_Mapping *l, *r;

   l = (const Efl_Filter_Model_Mapping *) left;
   r = (const Efl_Filter_Model_Mapping *) right;

   if (l->mapped < r->mapped)
     return EINA_RBTREE_LEFT;
   return EINA_RBTREE_RIGHT;
}

/**
 * @brief Lookup callback for the Rbtree storing Efl_Filter_Model_Mapping.
 * Compares a mapping node with a key (mapped index).
 *
 * @param node The Eina_Rbtree node (Efl_Filter_Model_Mapping).
 * @param key Pointer to an unsigned int representing the mapped index to search for.
 * @param length Length of the key (unused).
 * @param data User data (unused).
 * @return 0 if node->mapped == *key, < 0 if node->mapped < *key, > 0 if node->mapped > *key.
 */
static int
_filter_mapping_looking_cb(const Eina_Rbtree *node, const void *key,
                           int length EINA_UNUSED, void *data EINA_UNUSED)
{
   const Efl_Filter_Model_Mapping *n = (const Efl_Filter_Model_Mapping *) node;
   const unsigned int *k = key;

   return n->mapped - *k;
}

/**
 * @brief Sets the filter callback and associated data.
 * If a previous filter was set, its free callback is called on its data.
 *
 * @param obj The Efl_Filter_Model object (unused).
 * @param pd The private data of the Efl_Filter_Model.
 * @param filter_data User data to be passed to the filter callback.
 * @param filter The EflFilterModel callback function.
 * @param filter_free_cb Callback to free filter_data when the filter is changed or model is destroyed.
 */
static void
_efl_filter_model_filter_set(Eo *obj EINA_UNUSED, Efl_Filter_Model_Data *pd,
                             void *filter_data, EflFilterModel filter, Eina_Free_Cb filter_free_cb)
{
   if (pd->filter.cb)
     pd->filter.free_cb(pd->filter.data);
   pd->filter.data = filter_data;
   pd->filter.cb = filter;
   pd->filter.free_cb = filter_free_cb;
}

/**
 * @brief Callback used to free Efl_Filter_Model_Mapping nodes when the Rbtree is destroyed.
 * It unreferences and frees the mapping.
 *
 * @param node The Eina_Rbtree node (Efl_Filter_Model_Mapping) to free.
 * @param data User data (unused).
 */
static void
_rbtree_free_cb(Eina_Rbtree *node, void *data EINA_UNUSED)
{
   Efl_Filter_Model_Mapping *m = (Efl_Filter_Model_Mapping*) node;

   EINA_REFCOUNT_UNREF(m)
     free(m);
}

/**
 * @brief Structure to hold data for an asynchronous filter request.
 * This is used when fetching a child and then applying the filter to it.
 */
typedef struct _Efl_Filter_Request Efl_Filter_Request;
struct _Efl_Filter_Request
{
   Efl_Filter_Model_Data *pd; /**< Private data of the Efl_Filter_Model initiating the request. */
   Efl_Model *parent; /**< The Efl_Filter_Model itself (the one being filtered). */
   Efl_Model *child; /**< The child model being evaluated by the filter. */
   unsigned int index; /**< The original index of the child in the source model. */
};

/**
 * @brief Looks up or creates a composite child for a given mapping.
 * This function is responsible for creating the Efl_Filter_Model instance
 * that represents a child that passed the filter.
 *
 * @param klass The Efl_Class of the Efl_Filter_Model.
 * @param parent The parent Efl_Filter_Model.
 * @param view The view model (source model).
 * @param mapping The mapping information for the child.
 * @return The Efl_Filter_Model representing the filtered child, or NULL on failure.
 */
static Efl_Filter_Model *
_efl_filter_lookup(const Efl_Class *klass,
                   Efl_Model *parent, Efl_Model *view,
                   Efl_Filter_Model_Mapping *mapping)
{
   Efl_Filter_Model *child;
   Efl_Filter_Model_Data *cpd;

   child = _efl_composite_lookup(klass, parent, view, mapping->mapped);
   if (!child) return NULL;

   cpd = efl_data_scope_get(child, EFL_FILTER_MODEL_CLASS);
   cpd->processed = EINA_TRUE;
   cpd->self = mapping;
   EINA_REFCOUNT_REF(mapping);

   return child;
}

/**
 * @brief Callback executed after a child has been evaluated by the filter function.
 * If the filter function returns EINA_TRUE (passed via Eina_Value v), this function
 * creates a mapping, adds it to the rbtree, and emits child_added and
 * children_count_changed events.
 *
 * @param o The Efl_Filter_Model object (unused, parent is in Efl_Filter_Request).
 * @param data Pointer to Efl_Filter_Request containing context.
 * @param v An Eina_Value of type EINA_VALUE_TYPE_BOOL. EINA_TRUE if the child passed the filter.
 * @return An Eina_Value of type EINA_VALUE_TYPE_BOOL, EINA_TRUE on success, EINA_FALSE on error.
 */
static Eina_Value
_efl_filter_model_filter(Eo *o EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Filter_Model_Mapping *mapping;
   Efl_Filter_Model *child;
   Efl_Model_Children_Event cevt = { 0 };
   Efl_Filter_Request *r = data;
   Eina_Value ret = v;
   Eina_Bool result = EINA_FALSE;

   if (!eina_value_bool_get(&v, &result)) goto end;
   if (!result) goto end;

   mapping = calloc(1, sizeof (Efl_Filter_Model_Mapping));
   if (!mapping)
     {
        ret = eina_value_bool_init(EINA_FALSE);
        goto end;
     }
   EINA_REFCOUNT_INIT(mapping);

   mapping->original = r->index;
   mapping->mapped = r->pd->filter.count++;

   r->pd->mapping = eina_rbtree_inline_insert(r->pd->mapping, EINA_RBTREE_GET(mapping),
                                              _filter_mapping_cmp_cb, NULL);

   child = _efl_filter_lookup(efl_class_get(r->parent), r->parent, r->child, mapping);
   if (!child) goto end;

   cevt.index = mapping->mapped;
   cevt.child = child;

   efl_event_callback_call(r->parent, EFL_MODEL_EVENT_CHILD_ADDED, &cevt);
   efl_event_callback_call(r->parent, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

   efl_unref(cevt.child);

   ret = eina_value_bool_init(EINA_TRUE);

 end:
   return ret;
}

/**
 * @brief Cleans up an Efl_Filter_Request structure.
 * This is typically used as a .free callback for efl_future_then.
 * It unrefs the parent and child models and frees the request structure.
 *
 * @param o The Efl_Filter_Model object (unused).
 * @param data Pointer to the Efl_Filter_Request to clean up.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_filter_model_filter_clean(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Filter_Request *r = data;

   efl_unref(r->parent);
   efl_unref(r->child);
   free(r);
}

/**
 * @brief Callback executed after a child object has been fetched from the source model.
 * This function receives an Eina_Value array containing the fetched child (expected to be one).
 * It then calls the user-provided filter callback on this child.
 *
 * @param o The Efl_Filter_Model object (unused, parent is in Efl_Filter_Request).
 * @param data Pointer to Efl_Filter_Request containing context.
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, containing the fetched child model.
 *          Example: Eina_Value with type EINA_VALUE_TYPE_ARRAY, holding one Efl_Model object.
 *          `v` -> `[child_model_instance]`
 * @return An Eina_Value representing the future from efl_future_then (filter evaluation).
 */
static Eina_Value
_efl_filter_model_child_fetch(Eo *o EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Filter_Request *r = data;
   unsigned int i, len;
   Eina_Future *f;
   Eo *target = NULL;

   // Get the first and only child in the array
   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     break;

   r->child = efl_ref(target);

   f = r->pd->filter.cb(r->pd->filter.data, r->parent, r->child);
   f = efl_future_then(r->parent, f,
                       .success = _efl_filter_model_filter,
                       .success_type = EINA_VALUE_TYPE_BOOL,
                       .free = _efl_filter_model_filter_clean,
                       .data = r);
   return eina_future_as_value(f);
}

/**
 * @brief Error callback for the future that fetches a child.
 * Cleans up the Efl_Filter_Request and returns an Eina_Value error.
 *
 * @param o The Efl_Filter_Model object.
 * @param data Pointer to the Efl_Filter_Request.
 * @param err The Eina_Error code.
 * @return An Eina_Value of type EINA_VALUE_TYPE_ERROR.
 */
static Eina_Value
_efl_filter_model_child_error(Eo *o, void *data, Eina_Error err)
{
   _efl_filter_model_filter_clean(o, data, NULL);
   return eina_value_error_init(err);
}

/**
 * @brief Callback for EFL_MODEL_EVENT_CHILD_ADDED on the source model.
 * This function is triggered when a child is added to the model being filtered.
 * It initiates the filtering process for the new child. If the child object
 * itself is not provided in the event, it fetches it first.
 *
 * @param data The private data of the Efl_Filter_Model (pd).
 * @param event The Efl_Event structure, event->info is Efl_Model_Children_Event.
 */
static void
_efl_filter_model_child_added(void *data, const Efl_Event *event)
{
   Efl_Filter_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;
   Efl_Model *child = ev->child;
   Efl_Filter_Request *r;
   Eina_Future *f;

   if (child)
     {
        Efl_Filter_Model_Data *cpd = efl_data_scope_get(child, EFL_FILTER_MODEL_CLASS);

        if (cpd->processed) return ;
     }

   r = calloc(1, sizeof (Efl_Filter_Request));
   if (!r) return ;

   r->pd = pd;
   r->index = ev->index;
   r->parent = efl_ref(event->object);

   if (!child)
     {
        f = efl_model_children_slice_get(efl_ui_view_model_get(r->parent), r->index, 1);
        f = efl_future_then(event->object, f,
                            .success = _efl_filter_model_child_fetch,
                            .success_type = EINA_VALUE_TYPE_ARRAY,
                            .error = _efl_filter_model_child_error,
                            .data = r);
        return ;
     }

   r->child = efl_ref(child);

   f = pd->filter.cb(pd->filter.data, r->parent, r->child);
   f = efl_future_then(event->object, f,
                       .success = _efl_filter_model_filter,
                       .success_type = EINA_VALUE_TYPE_BOOL,
                       .free = _efl_filter_model_filter_clean,
                       .data = r);

   efl_event_callback_stop(event->object);
}

/**
 * @brief Callback for EFL_MODEL_EVENT_CHILD_REMOVED on the source model.
 * This function is triggered when a child is removed from the model being filtered.
 * It removes the corresponding mapping from the rbtree and updates the mapped indices
 * of subsequent children.
 *
 * @param data The private data of the Efl_Filter_Model (pd).
 * @param event The Efl_Event structure, event->info is Efl_Model_Children_Event.
 */
static void
_efl_filter_model_child_removed(void *data, const Efl_Event *event)
{
   Efl_Filter_Model_Mapping *mapping;
   Efl_Filter_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;
   unsigned int removed = ev->index;

   mapping = (void *)eina_rbtree_inline_lookup(pd->mapping,
                                               &removed, sizeof (uint64_t),
                                               _filter_mapping_looking_cb, NULL);
   if (!mapping) return;

   pd->mapping = eina_rbtree_inline_remove(pd->mapping, EINA_RBTREE_GET(mapping),
                                           _filter_mapping_cmp_cb, NULL);

   EINA_REFCOUNT_UNREF(mapping)
     free(mapping);

   // Update the tree for the index to reflect the removed child
   for (removed++; removed < pd->filter.count; removed++)
     {
        mapping = (void *)eina_rbtree_inline_lookup(pd->mapping,
                                                    &removed, sizeof (uint64_t),
                                                    _filter_mapping_looking_cb, NULL);
        if (!mapping) continue;

        pd->mapping = eina_rbtree_inline_remove(pd->mapping, EINA_RBTREE_GET(mapping),
                                                _filter_mapping_cmp_cb, NULL);
        mapping->mapped--;
        pd->mapping = eina_rbtree_inline_insert(pd->mapping, EINA_RBTREE_GET(mapping),
                                                _filter_mapping_cmp_cb, NULL);
     }
   pd->filter.count--;
   pd->counted--;
}

/**
 * @brief Array defining callbacks for child added/removed events on the source model.
 */
EFL_CALLBACKS_ARRAY_DEFINE(filters_callbacks,
                           { EFL_MODEL_EVENT_CHILD_ADDED, _efl_filter_model_child_added },
                           { EFL_MODEL_EVENT_CHILD_REMOVED, _efl_filter_model_child_removed });

/**
 * @brief Destructor for the Efl_Filter_Model.
 * Cleans up resources, including the mapping rbtree and any self-mapping.
 *
 * @param obj The Efl_Filter_Model object being destroyed.
 * @param pd The private data of the Efl_Filter_Model.
 */
static void
_efl_filter_model_efl_object_destructor(Eo *obj, Efl_Filter_Model_Data *pd)
{
   eina_rbtree_delete(pd->mapping, _rbtree_free_cb, NULL);

   if (pd->self)
     {
        EINA_REFCOUNT_UNREF(pd->self)
          free(pd->self);
     }
   pd->self = NULL;

   efl_destructor(efl_super(obj, EFL_FILTER_MODEL_CLASS));
}

/**
 * @brief Callback used in children_slice_get to process the result of fetching a single child.
 * It receives an Eina_Value array (expected to contain one child model). If this child
 * is an Efl_Filter_Model, its 'self' mapping is updated.
 *
 * @param o The Efl_Filter_Model object (unused).
 * @param data The Efl_Filter_Model_Mapping for this child.
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, containing the fetched child model.
 *          Example: Eina_Value with type EINA_VALUE_TYPE_ARRAY, holding one Efl_Model object.
 *          `v` -> `[child_model_instance]`
 * @return An Eina_Value of type EINA_VALUE_TYPE_OBJECT, containing the child model.
 */
static Eina_Value
_filter_remove_array(Eo *o EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Filter_Model_Mapping *mapping = data;
   unsigned int i, len;
   Eo *target = NULL;

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     break;

   if (efl_isa(target, EFL_FILTER_MODEL_CLASS))
     {
        Efl_Filter_Model_Data *pd = efl_data_scope_get(target, EFL_FILTER_MODEL_CLASS);

        pd->self = mapping;
        EINA_REFCOUNT_REF(pd->self);
     }

   return eina_value_object_init(target);
}

/**
 * @brief Implements Efl_Model_children_slice_get for the filter model.
 * Fetches a slice of children from the source model based on mapped indices,
 * then wraps them appropriately.
 *
 * @param obj The Efl_Filter_Model object.
 * @param pd The private data of the Efl_Filter_Model.
 * @param start The starting mapped index.
 * @param count The number of children to fetch.
 * @return A future that will resolve to an Eina_Array of child models, or an error.
 *         The Eina_Array contains Efl_Model objects.
 *         Example: Future resolves to Eina_Value of type EINA_VALUE_TYPE_ARRAY.
 *         `value` -> `[child1, child2, ..., childN]` where childX is an Efl_Model.
 */
static Eina_Future *
_efl_filter_model_efl_model_children_slice_get(Eo *obj, Efl_Filter_Model_Data *pd,
                                               unsigned int start, unsigned int count)
{
   Efl_Filter_Model_Mapping **mapping = NULL;
   Eina_Future **r = NULL;
   Eina_Future *f;
   unsigned int i;
   Eina_Error err = ENOMEM;

   if ((uint64_t) start + (uint64_t) count > pd->filter.count)
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
   if (count == 0)
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);

   r = calloc(1, (count + 1) * sizeof (Eina_Future *));
   if (!r) return efl_loop_future_rejected(obj, ENOMEM);

   mapping = calloc(count, sizeof (Efl_Filter_Model_Mapping *));
   if (!mapping) goto on_error;

   for (i = 0; i < count; i++)
     {
        unsigned int lookup = start + i;

        mapping[i] = (void *)eina_rbtree_inline_lookup(pd->mapping,
                                                       &lookup, sizeof (unsigned int),
                                                       _filter_mapping_looking_cb, NULL);
        if (!mapping[i]) goto on_error;
     }

   for (i = 0; i < count; i++)
     {
        r[i] = efl_model_children_slice_get(efl_super(obj, EFL_FILTER_MODEL_CLASS),
                                            mapping[i]->original, 1);
        r[i] = efl_future_then(obj, r[i], .success_type = EINA_VALUE_TYPE_ARRAY,
                               .success = _filter_remove_array,
                               .data = mapping[i]);
        if (!r[i]) goto on_error;
     }
   r[i] = EINA_FUTURE_SENTINEL;

   f = efl_future_then(obj, eina_future_all_array(r), .success = _efl_future_all_repack);
   free(r);
   free(mapping);

   return f;

 on_error:
   free(mapping);

   if (r)
     for (i = 0; i < count; i ++)
       if (r[i]) eina_future_cancel(r[i]);
   free(r);

   return efl_loop_future_rejected(obj, err);
}

/**
 * @brief Structure to hold data for processing an array of filter results.
 * Used when initially populating the filter model or when handling a batch of children.
 */
typedef struct _Efl_Filter_Model_Result Efl_Filter_Model_Result;
struct _Efl_Filter_Model_Result
{
   Efl_Filter_Model_Data *pd; /**< Private data of the Efl_Filter_Model. */
   unsigned int count; /**< Number of target models in the targets array. */
   Efl_Model *targets[1]; /**< Flexible array member holding target Efl_Model instances that were filtered. */
};

/**
 * @brief Processes an array of boolean results from filter callbacks.
 * This function is called after a batch of children from the source model have been
 * evaluated by the filter callback. For each child that passed the filter (boolean is true),
 * a mapping is created, and child_added events are emitted.
 *
 * @param o The Efl_Filter_Model object.
 * @param data Pointer to Efl_Filter_Model_Result containing the original children and filter model data.
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, containing boolean results from the filter.
 *          Example: Eina_Value with type EINA_VALUE_TYPE_ARRAY.
 *          `v` -> `[EINA_TRUE, EINA_FALSE, EINA_TRUE, ...]`
 *          The order corresponds to the children in `req->targets`.
 * @return The input Eina_Value `v`.
 */
// This future receive an array of boolean that indicate if a fetched object is to be kept
static Eina_Value
_efl_filter_model_array_result_request(Eo *o EINA_UNUSED, void *data, const Eina_Value v)
{
   Efl_Filter_Model_Result *req = data;
   Efl_Filter_Model_Data *pd = req->pd;
   unsigned int i, len;
   Eina_Value request = EINA_VALUE_EMPTY;
   unsigned int pcount = pd->filter.count;

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, request)
     {
        Efl_Filter_Model_Mapping *mapping;
        Efl_Model_Children_Event cevt = { 0 };
        Eina_Bool b;

        if (eina_value_type_get(&request) != EINA_VALUE_TYPE_BOOL)
          continue ;

        if (!eina_value_bool_get(&request, &b)) continue;
        if (!b) continue;

        mapping = calloc(1, sizeof (Efl_Filter_Model_Mapping));
        if (!mapping) continue;

        EINA_REFCOUNT_INIT(mapping);
        mapping->original = i;
        mapping->mapped = pd->filter.count++;

        // Insert in tree here
        pd->mapping = eina_rbtree_inline_insert(pd->mapping, EINA_RBTREE_GET(mapping),
                                                _filter_mapping_cmp_cb, NULL);

        cevt.index = mapping->mapped;
        cevt.child = _efl_filter_lookup(efl_class_get(o), o, req->targets[i], mapping);
        if (!cevt.child) continue;

        efl_event_callback_call(o, EFL_MODEL_EVENT_CHILD_ADDED, &cevt);
        efl_unref(cevt.child);
     }

   if (pcount != pd->filter.count)
     efl_event_callback_call(o, EFL_MODEL_EVENT_CHILDREN_COUNT_CHANGED, NULL);

   return v;
}

/**
 * @brief Frees an Efl_Filter_Model_Result structure.
 * Unrefs all target models and frees the structure itself.
 * Typically used as a .free callback for efl_future_then.
 *
 * @param o The Efl_Filter_Model object (unused).
 * @param data Pointer to the Efl_Filter_Model_Result to free.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_filter_model_array_result_free(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Filter_Model_Result *req = data;
   unsigned int i;

   for (i = 0; i < req->count; i++)
     efl_unref(req->targets[i]);
   free(req);
}

/**
 * @brief Processes an array of fetched children from the source model.
 * This function is called when a slice of children has been fetched (e.g., during
 * initial population). It then calls the filter callback for each fetched child.
 * The results of these filter calls (an array of futures resolving to booleans)
 * are then processed by _efl_filter_model_array_result_request.
 *
 * @param o The Efl_Filter_Model object.
 * @param data The private data of the Efl_Filter_Model (pd).
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, containing the fetched child models.
 *          Example: Eina_Value with type EINA_VALUE_TYPE_ARRAY.
 *          `v` -> `[child_model1, child_model2, ...]`
 * @return An Eina_Value representing the future from efl_future_then (overall filtering process for the array).
 */
// This future receive an array of children object
static Eina_Value
_efl_filter_model_array_fetch(Eo *o, void *data, const Eina_Value v)
{
   Efl_Filter_Model_Result *req;
   Efl_Filter_Model_Data *pd = data;
   unsigned int i, len;
   Eo *target = NULL;
   Eina_Future **array = NULL;
   Eina_Future *r;
   Eina_Value res = v;

   if (!eina_value_array_count(&v)) return v;

   array = malloc((eina_value_array_count(&v) + 1) * sizeof (Eina_Future*));
   if (!array) return eina_value_error_init(ENOMEM);

   req = malloc(sizeof (Efl_Filter_Model_Result) +
                sizeof (Eo*) * (eina_value_array_count(&v) - 1));
   if (!req)
     {
        res = eina_value_error_init(ENOMEM);
        goto on_error;
     }

   req->pd = pd;
   req->count = eina_value_array_count(&v);

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, target)
     {
        array[i] = pd->filter.cb(pd->filter.data, o, target);
        req->targets[i] = efl_ref(target);
     }

   array[i] = EINA_FUTURE_SENTINEL;

   r = eina_future_all_array(array);
   r = efl_future_then(o, r, .success_type = EINA_VALUE_TYPE_ARRAY,
                       .success = _efl_filter_model_array_result_request,
                       .free = _efl_filter_model_array_result_free,
                       .data = req);
   res = eina_future_as_value(r);

 on_error:
   free(array);

   return res;
}

/**
 * @brief Implements Efl_Model_children_count_get for the filter model.
 * If counting hasn't started and a filter callback is set, this function
 * initiates the process of fetching all children from the source model and
 * applying the filter to them. It also starts listening for child_added/removed
 * events on the source model.
 *
 * @param obj The Efl_Filter_Model object.
 * @param pd The private data of the Efl_Filter_Model.
 * @return The number of children that passed the filter. This might be an
 *         intermediate count if the initial filtering is asynchronous.
 */
static unsigned int
_efl_filter_model_efl_model_children_count_get(const Eo *obj, Efl_Filter_Model_Data *pd)
{
   if (!pd->counting_started && pd->filter.cb)
     {
        pd->counting_started = EINA_TRUE;

        // Start watching for children now
        efl_event_callback_array_add((Eo *)obj, filters_callbacks(), pd);

        // Start counting (which may trigger filter being added asynchronously)
        pd->counted = efl_model_children_count_get(efl_super(obj, EFL_FILTER_MODEL_CLASS));
        if (pd->counted > 0)
          {
             Eina_Future *f;

             f = efl_model_children_slice_get(efl_ui_view_model_get(obj), 0, pd->counted);
             efl_future_then(obj, f, .success_type = EINA_VALUE_TYPE_ARRAY,
                             .success = _efl_filter_model_array_fetch,
                             .data = pd);
          }
     }

   return pd->filter.count;
}

/**
 * @brief Implements Efl_Model_property_get for the filter model.
 * If the property is EFL_COMPOSITE_MODEL_CHILD_INDEX and this model instance
 * represents a filtered child (pd->self is set), it returns the mapped index.
 * Otherwise, it defers to the superclass.
 *
 * @param obj The Efl_Filter_Model object.
 * @param pd The private data of the Efl_Filter_Model.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value, or NULL if not found.
 *         Example for EFL_COMPOSITE_MODEL_CHILD_INDEX: Eina_Value of type EINA_VALUE_TYPE_UINT64.
 */
static Eina_Value *
_efl_filter_model_efl_model_property_get(const Eo *obj, Efl_Filter_Model_Data *pd,
                                         const char *property)
{
   if (pd->self && eina_streq(property, EFL_COMPOSITE_MODEL_CHILD_INDEX))
     {
        return eina_value_uint64_new(pd->self->mapped);
     }

   return efl_model_property_get(efl_super(obj, EFL_FILTER_MODEL_CLASS), property);
}

/**
 * @brief Implements Efl_Composite_Model_index_get for the filter model.
 * If this model instance represents a filtered child (pd->self is set),
 * it returns its mapped index. Otherwise, it defers to the superclass.
 *
 * @param obj The Efl_Filter_Model object.
 * @param pd The private data of the Efl_Filter_Model.
 * @return The mapped index if this is a filtered child, otherwise behavior of superclass.
 */
static unsigned int
_efl_filter_model_efl_composite_model_index_get(const Eo *obj, Efl_Filter_Model_Data *pd)
{
   if (pd->self) return pd->self->mapped;
   return efl_composite_model_index_get(efl_super(obj, EFL_FILTER_MODEL_CLASS));
}

#include "efl_filter_model.eo.c"
