#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Efl_Core.h>

#include "ecore_internal.h"

typedef struct _Efl_Boolean_Model_Data Efl_Boolean_Model_Data;
typedef struct _Efl_Boolean_Model_Value Efl_Boolean_Model_Value;
typedef struct _Efl_Boolean_Model_Storage_Range Efl_Boolean_Model_Storage_Range;

/**
 * @brief Private data structure for the Efl_Boolean_Model.
 *
 * This structure holds the model's data, including a reference to its parent
 * model and a hash table of boolean property values.
 */
struct _Efl_Boolean_Model_Data
{
   Efl_Boolean_Model_Data *parent; /**< Pointer to the parent model's data, if any. Used to access shared property definitions. */
   Eina_Hash *values; /**< Hash table storing Efl_Boolean_Model_Value structures, keyed by property name. This is where the actual boolean states are stored for properties defined by this model instance (the "root" boolean model). */
};

/**
 * @brief Represents a range of boolean values stored contiguously.
 *
 * This structure is used to efficiently store boolean flags in a bit-packed
 * format. Each range is a node in a red-black tree for fast lookups.
 */
struct _Efl_Boolean_Model_Storage_Range
{
   EINA_RBTREE; /**< Macro to embed red-black tree node data. */

   unsigned int  offset; /**< The starting index in the conceptual array of booleans that this storage range covers. */
   uint16_t      length; /**< The number of boolean values (bits) stored in this range. Maximum length of the buffer will be 256*8 = 2048 bits. */

   // We over allocate this buffer to have things fitting in one alloc
   unsigned char buffer[256]; /**< Buffer storing the bit-packed boolean values. Each char stores 8 booleans. */
};

/**
 * @brief Stores information about a specific boolean property.
 *
 * This includes the property name, its default value, and the actual storage
 * for its boolean states across different indices (children of the model).
 */
struct _Efl_Boolean_Model_Value
{
   Eina_Stringshare *property; /**< The name of the boolean property. */

   Eina_Rbtree *buffers_root; /**< Red-black tree of Efl_Boolean_Model_Storage_Range, storing the boolean values for different indices. */
   Efl_Boolean_Model_Storage_Range *last; /**< Pointer to the most recently added or accessed storage range, for optimization. */

   Eina_Bool      default_value; /**< The default value for this property if not explicitly set. */
};

/**
 * @brief Compares two storage ranges for ordering in the Rbtree.
 *
 * Comparison is based on the `offset` field. Ranges are not expected to overlap.
 *
 * @param left The left storage range.
 * @param right The right storage range.
 * @param data Unused user data.
 * @return EINA_RBTREE_LEFT if left < right, EINA_RBTREE_RIGHT otherwise.
 */
static Eina_Rbtree_Direction
_storage_range_cmp(const Efl_Boolean_Model_Storage_Range *left,
                   const Efl_Boolean_Model_Storage_Range *right,
                   void *data EINA_UNUSED)
{
   // We should not have any overlapping range
   if (left->offset < right->offset)
     return EINA_RBTREE_LEFT;
   return EINA_RBTREE_RIGHT;
}

/**
 * @brief Compares a storage range node with a key (index) for Rbtree lookup.
 *
 * Checks if the given `key` (index) falls within the range defined by `node->offset`
 * and `node->offset + node->length`.
 *
 * @param node The storage range node.
 * @param key Pointer to the index being searched.
 * @param length Unused length parameter.
 * @param data Unused user data.
 * @return 1 if node->offset > *key, -1 if node->offset + node->length < *key, 0 if key is in range.
 */
static int
_storage_range_key(const Efl_Boolean_Model_Storage_Range *node,
                   const unsigned int *key, int length EINA_UNUSED, void *data EINA_UNUSED)
{
   if (node->offset > *key) return 1;
   if (node->offset + node->length < *key) return -1;
   // The key is in the range!
   return 0;
}

/**
 * @brief Frees a storage range node.
 *
 * @param node The Rbtree node (Efl_Boolean_Model_Storage_Range) to free.
 * @param data Unused user data.
 */
static void
_storage_range_free(Eina_Rbtree *node, void *data EINA_UNUSED)
{
   free(node);
}

/**
 * @brief Looks up or allocates a storage range for a given property and index.
 *
 * This function finds the Efl_Boolean_Model_Storage_Range that contains the
 * boolean value for the specified `property` at the given `index`.
 * If `allocate` is true and no suitable range exists, a new one is created
 * and initialized.
 *
 * @param pd The model data for the current Efl_Composite_Model instance (child).
 * @param property The name of the boolean property.
 * @param index The index for which the boolean value is requested.
 * @param allocate If EINA_TRUE, a new storage range will be allocated if needed.
 * @param value The value to set if allocating a new range and the value differs from the default.
 * @param[out] found Set to EINA_TRUE if the property is defined by the parent boolean model, EINA_FALSE otherwise.
 * @param[out] default_value Set to the default value of the property.
 * @return Pointer to the Efl_Boolean_Model_Storage_Range, or NULL if not found and not allocated,
 *         or if the property is not defined by the parent.
 */
static Efl_Boolean_Model_Storage_Range *
_storage_lookup(Efl_Boolean_Model_Data *pd,
                const char *property,
                unsigned int index,
                Eina_Bool allocate,
                Eina_Bool value,
                Eina_Bool *found,
                Eina_Bool *default_value)
{
   Efl_Boolean_Model_Storage_Range *lookup;
   Efl_Boolean_Model_Value *v;
   Eina_Stringshare *s;

   // Check if this is requesting a defined boolean property
   // Property are defined and their value are stored on the parent BOOLEAN
   s = eina_stringshare_add(property);
   v = eina_hash_find(pd->parent->values, s);
   eina_stringshare_del(s);

   if (!v) return NULL;
   *found = EINA_TRUE;
   *default_value = !!v->default_value;

   lookup = (void*) eina_rbtree_inline_lookup(v->buffers_root, &index, sizeof (unsigned int),
                                              EINA_RBTREE_CMP_KEY_CB(_storage_range_key), NULL);
   if (lookup) return lookup;
   if (!allocate) return NULL;

   // The value is the same as the default value, why bother allocate
   if (*default_value == value) return NULL;

   // For simplicity we do not create a sparse array, every boolean potentially needed are allocated
   // FIXME: keep it a sparse allocated buffer and only allocate needed buffer on demand
   do
     {
        lookup = calloc(1, sizeof (Efl_Boolean_Model_Storage_Range));
        if (!lookup) return NULL;

        lookup->offset = v->last ? v->last->offset + v->last->length + 1 : 0;
        lookup->length = sizeof (lookup->buffer) * 8; // Number of bits in the buffer
        // Initialize the buffer to the right default value
        memset(&lookup->buffer[0], *default_value, sizeof (lookup->buffer));

        v->buffers_root = eina_rbtree_inline_insert(v->buffers_root, EINA_RBTREE_GET(lookup),
                                                    EINA_RBTREE_CMP_NODE_CB(_storage_range_cmp), NULL);
        v->last = lookup;
     }
   while (v->last->offset + v->last->length < index);

   return lookup;
}

/**
 * @brief Implements Efl.Model.properties_get.
 *
 * Retrieves an iterator over the names of properties supported by this model.
 * It combines properties from its parent (if it's a boolean model) with
 * properties from the Efl_Composite_Model superclass.
 *
 * @param obj The Efl_Boolean_Model object.
 * @param pd The private data of the Efl_Boolean_Model.
 * @return An Eina_Iterator for property names (Eina_Stringshare *).
 */
static Eina_Iterator *
_efl_boolean_model_efl_model_properties_get(const Eo *obj,
                                            Efl_Boolean_Model_Data *pd)
{
   Eina_Iterator *properties = NULL;

   if (pd->parent)
     properties = eina_hash_iterator_key_new(pd->parent->values);
   EFL_COMPOSITE_MODEL_PROPERTIES_SUPER(props,
                                        obj, EFL_BOOLEAN_MODEL_CLASS,
                                        properties);
   return props;
}

/**
 * @brief Implements Efl.Model.property_get.
 *
 * Retrieves the value of a specific property for the model instance.
 * If the property is a boolean property managed by this model (defined in its parent),
 * its value is looked up in the bit-packed storage. Otherwise, the call is
 * forwarded to the superclass.
 *
 * @param obj The Efl_Boolean_Model object.
 * @param pd The private data of the Efl_Boolean_Model.
 * @param property The name of the property to get.
 * @return An Eina_Value containing the property value, or NULL on error or if property is not found.
 *         The caller is responsible for freeing the returned Eina_Value.
 */
static Eina_Value *
_efl_boolean_model_efl_model_property_get(const Eo *obj,
                                          Efl_Boolean_Model_Data *pd,
                                          const char *property)
{
   Efl_Boolean_Model_Storage_Range *sr;
   Eina_Bool flag;
   unsigned int index;
   unsigned int offset;
   Eina_Bool found = EINA_FALSE;
   Eina_Bool default_value = EINA_FALSE;

   if (property == NULL) return NULL;

   // If we do not have a parent set that his a BOOLEAN, then we should just forward up the call
   if (!pd->parent)
     return efl_model_property_get(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS), property);

   index = efl_composite_model_index_get(obj);


   sr = _storage_lookup(pd, property, index, EINA_FALSE, EINA_FALSE, &found, &default_value);
   if (!found) // Not a property handle by this object, forward
     return efl_model_property_get(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS), property);

   if (!sr) // Not found in storage, should be the default value
     return eina_value_bool_new(default_value);

   // Calculate the matching offset for the requested index
   offset = index - sr->offset;

   flag = sr->buffer[offset >> 3] & (((unsigned char)1) << (offset & 0x7));

   return eina_value_bool_new(!!flag);
}

/**
 * @brief Implements Efl.Model.property_set.
 *
 * Sets the value of a specific property for the model instance.
 * If the property is a boolean property managed by this model, its value is
 * updated in the bit-packed storage. Otherwise, the call is forwarded to the
 * superclass.
 *
 * @param obj The Efl_Boolean_Model object.
 * @param pd The private data of the Efl_Boolean_Model.
 * @param property The name of the property to set.
 * @param value The Eina_Value containing the new property value (expected to be a boolean).
 * @return An Eina_Future that resolves with the set value or rejects on error.
 */
static Eina_Future *
_efl_boolean_model_efl_model_property_set(Eo *obj,
                                          Efl_Boolean_Model_Data *pd,
                                          const char *property, Eina_Value *value)
{
   Efl_Boolean_Model_Storage_Range *sr;
   unsigned int index;
   unsigned int offset;
   Eina_Bool flag = EINA_FALSE;
   Eina_Bool found = EINA_FALSE;
   Eina_Bool convert_fail = EINA_FALSE;
   Eina_Bool default_value = EINA_FALSE;

   if (!property) return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_UNKNOWN);

   // If we do not have a parent set that his a BOOLEAN, then we should just forward up the call
   if (!pd->parent)
     return efl_model_property_set(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS),
                                   property, value);

   index = efl_composite_model_index_get(obj);
   if (!eina_value_bool_convert(value, &flag))
     convert_fail = EINA_TRUE;

   sr = _storage_lookup(pd, property, index, EINA_TRUE, flag, &found, &default_value);
   if (!found)
     return efl_model_property_set(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS),
                                   property, value);

   // Convert did fail and we actually should have a valid Boolean to put in the buffer
   if (convert_fail)
     return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_UNKNOWN);

   if (!sr)
     return efl_loop_future_resolved(obj, eina_value_bool_init(default_value));

   // Calculate the matching offset for the requested index
   offset = index - sr->offset;

   // It is assumed that during slice get the buffer is properly sized
   if (flag)
     sr->buffer[offset >> 3] |= ((unsigned char)1) << (offset & 0x7);
   else
     sr->buffer[offset >> 3] &= ~(((unsigned char)1) << (offset & 0x7));

   // Calling "properties,changed" event
   efl_model_properties_changed(obj, property);

   // Return fulfilled future
   return efl_loop_future_resolved(obj, eina_value_bool_init(!!flag));
}

/**
 * @brief Frees an Efl_Boolean_Model_Value structure.
 *
 * This function is used as a callback for eina_hash_free_buckets.
 * It releases the stringshare for the property name, deletes the Rbtree
 * of storage ranges, and frees the structure itself.
 *
 * @param data Pointer to the Efl_Boolean_Model_Value to be freed.
 */
static void
_boolean_value_free(void *data)
{
   Efl_Boolean_Model_Value *value = data;

   eina_stringshare_del(value->property);
   value->property = NULL;

   eina_rbtree_delete(value->buffers_root, _storage_range_free, NULL);
   value->last = NULL;

   free(value);
}

/**
 * @brief Recursively finds and adds storage ranges with offsets greater than `upper` to an array.
 *
 * This function traverses the Rbtree of storage ranges. If a range's offset
 * is greater than `upper`, it's added to the `mark` array, and its children
 * are also checked. Otherwise, only its left child (ranges with smaller offsets)
 * is checked. This is used to identify ranges that need their offsets adjusted
 * after an item is removed.
 *
 * @param root The current root of the Rbtree (or subtree) of Efl_Boolean_Model_Storage_Range.
 * @param mark An Eina_Array to store pointers to Efl_Boolean_Model_Storage_Range that are "greater".
 *             Example of elements in `mark` after population:
 *             `mark` = [ &range_A, &range_B, ... ] where range_A->offset > upper.
 * @param upper The threshold offset. Ranges with `offset > upper` are marked.
 */
static void
_mark_greater(Efl_Boolean_Model_Storage_Range *root, Eina_Array *mark, const unsigned int upper)
{
   if (!root) return ;

   if (root->offset > upper)
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
 * @brief Handles the EFL_MODEL_EVENT_CHILD_REMOVED event.
 *
 * When a child is removed from the composite model, the corresponding boolean
 * values in all managed properties need to be shifted or removed. This function
 * iterates through all boolean properties defined by the parent boolean model.
 * For each property, it:
 * 1. Locates the storage range containing the boolean for the removed child's index.
 * 2. Shifts the bits within that storage range to fill the gap left by the removed child.
 * 3. Decrements the length of the storage range. If length becomes 0, the range is freed.
 * 4. Updates the `last` pointer of the Efl_Boolean_Model_Value if the freed range was the last one.
 * 5. Identifies all storage ranges whose offsets are greater than the removed index.
 * 6. Decrements the `offset` of these identified storage ranges.
 *
 * @param data The Efl_Boolean_Model_Data of the object that received the event.
 * @param event The Efl_Event structure, where `event->info` is an Efl_Model_Children_Event.
 */
static void
_child_removed(void *data, const Efl_Event *event)
{
   Efl_Boolean_Model_Data *pd = data;
   Efl_Model_Children_Event *ev = event->info;
   Efl_Boolean_Model_Value *v;
   Eina_Iterator *it;
   Eina_Array updated;

   if (!pd->values) return;

   eina_array_step_set(&updated, sizeof (Eina_Array), 8);

   it = eina_hash_iterator_data_new(pd->values);
   EINA_ITERATOR_FOREACH(it, v)
     {
        Efl_Boolean_Model_Storage_Range *lookup;
        Eina_Array_Iterator iterator;
        unsigned int i;

        // Remove the data from the buffer it belong to
        lookup = (void*) eina_rbtree_inline_lookup(v->buffers_root, &ev->index, sizeof (unsigned int),
                                                   EINA_RBTREE_CMP_KEY_CB(_storage_range_key), NULL);
        if (lookup)
          {
             unsigned char lower_mask = (((unsigned char)1) << (ev->index & 0x7)) - 1;
             unsigned char upper_mask = (~(((unsigned char)1) << (ev->index & 0x7))) & (~lower_mask);
             uint16_t offset = (ev->index - lookup->offset) >> 3;
             uint16_t byte_length = lookup->length >> 3;

             // Manually shift all the byte in the buffer
             while (offset < byte_length)
               {
                  lookup->buffer[offset] = ((lookup->buffer[offset] & upper_mask) >> 1) |
                    (lookup->buffer[offset] & lower_mask);
                  if (offset + 1 < byte_length)
                    lookup->buffer[offset] |= lookup->buffer[offset + 1] & 0x1;

                  lower_mask = 0;
                  upper_mask = 0xFE;
                  offset++;
               }

             lookup->length--;
             if (lookup->length == 0)
               {
                  v->buffers_root = eina_rbtree_inline_remove(v->buffers_root, EINA_RBTREE_GET(lookup),
                                                              EINA_RBTREE_CMP_NODE_CB(_storage_range_cmp), NULL);
                  free(lookup);

                  if (lookup == v->last)
                    {
                       if (v->buffers_root)
                         {
                            unsigned int last_index = ev->index - 1;

                            lookup = (void*) eina_rbtree_inline_lookup(v->buffers_root, &last_index,
                                                                       sizeof (unsigned int),
                                                                       EINA_RBTREE_CMP_KEY_CB(_storage_range_key),
                                                                       NULL);
                            v->last = lookup;
                         }
                       else
                         {
                            // Nobody left
                            v->last = NULL;
                         }
                    }
               }
          }

        _mark_greater((void*) v->buffers_root, &updated, ev->index);

        // Correct all the buffer after it
        // There is no need to remove and reinsert them as their relative order will not change.
        EINA_ARRAY_ITER_NEXT(&updated, i, lookup, iterator)
          {
             lookup->offset--;
          }

        eina_array_clean(&updated);
     }
   eina_iterator_free(it);

   eina_array_flush(&updated);
}

/**
 * @brief Implements Efl.Object.constructor.
 *
 * Initializes the Efl_Boolean_Model instance. It creates the hash table for
 * storing boolean property definitions (if this instance is a "root" boolean model)
 * or links to the parent's Efl_Boolean_Model_Data if this instance is a child
 * representing an item in a list of booleans.
 * It also registers a callback for the EFL_MODEL_EVENT_CHILD_REMOVED event.
 *
 * @param obj The Efl_Boolean_Model object being constructed.
 * @param pd The private data for the Efl_Boolean_Model.
 * @return The constructed Efl_Boolean_Model object, or NULL on failure.
 */
static Eo *
_efl_boolean_model_efl_object_constructor(Eo *obj, Efl_Boolean_Model_Data *pd)
{
   Eo *parent;
   obj = efl_constructor(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS));

   if (!obj) return NULL;

   pd->values = eina_hash_stringshared_new(_boolean_value_free);
   // Only add a reference to the parent if it is actually a BOOLEAN_MODEL_CLASS
   // The root typically doesn't have any boolean property, only its child do
   parent = efl_parent_get(obj);
   if (efl_isa(parent, EFL_BOOLEAN_MODEL_CLASS))
     pd->parent = efl_data_scope_get(parent, EFL_BOOLEAN_MODEL_CLASS);

   efl_event_callback_add(obj, EFL_MODEL_EVENT_CHILD_REMOVED, _child_removed, pd);

   return obj;
}

/**
 * @brief Implements Efl.Object.destructor.
 *
 * Cleans up resources used by the Efl_Boolean_Model instance.
 * This primarily involves freeing the hash table of boolean property values.
 *
 * @param obj The Efl_Boolean_Model object being destructed.
 * @param pd The private data of the Efl_Boolean_Model.
 */
static void
_efl_boolean_model_efl_object_destructor(Eo *obj, Efl_Boolean_Model_Data *pd)
{
   eina_hash_free(pd->values);

   efl_destructor(efl_super(obj, EFL_BOOLEAN_MODEL_CLASS));
}

/**
 * @brief Adds a new boolean property definition to the model.
 *
 * This function is called on the "root" Efl_Boolean_Model to define a new
 * boolean property that its children (Efl_Composite_Model instances using this
 * boolean model) can then have values for.
 *
 * @param obj The Efl_Boolean_Model object (typically the root boolean model).
 * @param pd The private data of the Efl_Boolean_Model.
 * @param name The name of the new boolean property.
 * @param default_value The default value for this property.
 */
static void
_efl_boolean_model_boolean_add(Eo *obj EINA_UNUSED,
                               Efl_Boolean_Model_Data *pd,
                               const char *name, Eina_Bool default_value)
{
   Efl_Boolean_Model_Value *value;

   if (!name) return ;

   value = calloc(1, sizeof (Efl_Boolean_Model_Value));
   if (!value) return ;

   value->property = eina_stringshare_add(name);
   value->default_value = default_value;

   eina_hash_direct_add(pd->values, value->property, value);
}

/**
 * @brief Deletes a boolean property definition from the model.
 *
 * This removes a previously defined boolean property from the "root"
 * Efl_Boolean_Model.
 *
 * @param obj The Efl_Boolean_Model object (typically the root boolean model).
 * @param pd The private data of the Efl_Boolean_Model.
 * @param name The name of the boolean property to delete.
 */
static void
_efl_boolean_model_boolean_del(Eo *obj EINA_UNUSED,
                               Efl_Boolean_Model_Data *pd,
                               const char *name)
{
   Eina_Stringshare *s;

   s = eina_stringshare_add(name);
   eina_hash_del(pd->values, s, NULL);
   eina_stringshare_del(s);
}

typedef struct _Eina_Iterator_Boolean Eina_Iterator_Boolean;

/**
 * @brief Private data structure for the boolean property iterator.
 *
 * This iterator is used to find all indices (children) for which a specific
 * boolean property has a certain value (either EINA_TRUE or EINA_FALSE).
 */
struct _Eina_Iterator_Boolean
{
   Eina_Iterator iterator; /**< Base Eina_Iterator structure. */

   Eo *obj; /**< The Efl_Boolean_Model object being iterated. */
   Efl_Boolean_Model_Data *pd; /**< Private data of the Efl_Boolean_Model. */
   Efl_Boolean_Model_Value *v; /**< The specific boolean property (Efl_Boolean_Model_Value) being iterated. */
   Efl_Boolean_Model_Storage_Range *sr; /**< The current storage range being scanned. */
   Eina_Iterator *infix; /**< Iterator over the storage ranges (Efl_Boolean_Model_Storage_Range) for property `v`. */

   unsigned int index; /**< The current child index being checked or the next one to be returned. */
   unsigned int total; /**< Total number of children in the model. */

   Eina_Bool request; /**< The boolean value (EINA_TRUE or EINA_FALSE) to search for. */
};

/**
 * @brief Finds the next index within the current storage range (`it->sr`) that matches `it->request`.
 *
 * This function scans the bits in `it->sr->buffer` starting from `it->index`
 * (relative to the start of the model, not the buffer). It updates `it->index`
 * to the found matching index.
 *
 * @param it The boolean iterator.
 * @return EINA_TRUE if a matching index is found in the current storage range, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_boolean_model_iterator_storage_index_find(Eina_Iterator_Boolean *it)
{
   uint16_t offset;
   uint16_t byte_length;

   offset = it->index - it->sr->offset;
   byte_length = it->sr->length >> 3;

   while (offset < it->sr->length)
     {
        unsigned int upidx;

        upidx = offset >> 3;

        // Quickly dismiss byte that really do not match
        while (upidx < byte_length &&
               it->sr->buffer[upidx] == (it->request ? 0x00 : 0xFF))
          upidx++;

        // Make the indexes jump
        if (upidx != (offset >> 3))
          {
             offset = upidx << 3;
             it->index = it->sr->offset + offset;
          }

        // Search inside the said byte
        while (((offset >> 3) == upidx) &&
               (offset < it->sr->length))
          {
             Eina_Bool flag = it->sr->buffer[offset >> 3] &
               (((unsigned char)1) << (offset & 0x7));

             if (it->request == !!flag)
               return EINA_TRUE;

             it->index++;
             offset++;
          }
     }

   return EINA_FALSE;
}

/**
 * @brief Finds the next child index across all storage ranges that matches `it->request`.
 *
 * This function iterates through storage ranges for the property. For each range,
 * it calls `_efl_boolean_model_iterator_storage_index_find` to search within that range.
 * It also handles cases where indices are not covered by any storage range (implying
 * they have the default value for the property).
 *
 * @param it The boolean iterator.
 * @return EINA_TRUE if a matching index is found, EINA_FALSE if no more matches exist.
 */
static Eina_Bool
_efl_boolean_model_iterator_index_find(Eina_Iterator_Boolean *it)
{
   while (it->index < it->total)
     {
        // If we are not walking on an existing storage range, look for a new one
        if (!it->sr)
          {
             if (!eina_iterator_next(it->infix, (void**) &it->sr))
               {
                  // All the rest of the data are not allocated and there value is still default
                  if (it->v->default_value != it->request)
                    return EINA_FALSE;
                  return EINA_TRUE;
               }
          }

        if (_efl_boolean_model_iterator_storage_index_find(it))
          return EINA_TRUE;

        // Nothing more to look at in this buffer
        it->sr = NULL;
     }

   return EINA_FALSE;
}

/**
 * @brief Advances the iterator to the next matching index.
 *
 * Implements the `next` function for the Eina_Iterator interface.
 *
 * @param it The boolean iterator.
 * @param[out] data Pointer to store the address of the current index (`&it->index`).
 *                  The value pointed to is an `unsigned int`.
 * @return EINA_TRUE if a next item is available, EINA_FALSE otherwise.
 */
static Eina_Bool
efl_boolean_model_iterator_next(Eina_Iterator_Boolean *it, void **data)
{
   *data = &it->index;
   it->index++;

   return _efl_boolean_model_iterator_index_find(it);
}

/**
 * @brief Gets the container object for the iterator.
 *
 * Implements the `get_container` function for the Eina_Iterator interface.
 *
 * @param it The boolean iterator.
 * @return The Efl_Boolean_Model object being iterated.
 */
static Eo *
efl_boolean_model_iterator_get_container(Eina_Iterator_Boolean *it)
{
   return it->obj;
}

/**
 * @brief Frees the boolean iterator.
 *
 * Implements the `free` function for the Eina_Iterator interface.
 *
 * @param it The boolean iterator to free.
 */
static void
efl_boolean_model_iterator_free(Eina_Iterator_Boolean *it)
{
   eina_iterator_free(it->infix);
   efl_unref(it->obj);
   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_NONE);
   free(it);
}

/**
 * @brief Creates an iterator to find indices where a boolean property has a specific value.
 *
 * This function, part of the Efl.Boolean_Model API, allows iterating over
 * child indices that match the `request`ed boolean state for the given `name`d property.
 *
 * @param obj The Efl_Boolean_Model object.
 * @param pd The private data of the Efl_Boolean_Model. This should be the `pd->parent`
 *           when called on a child Efl_Composite_Model, as property definitions
 *           are stored in the root Efl_Boolean_Model.
 * @param name The name of the boolean property.
 * @param request The boolean value (EINA_TRUE or EINA_FALSE) to search for.
 * @return A new Eina_Iterator, or NULL on failure or if the property `name` is not found.
 *         The iterator returns pointers to `unsigned int` (the indices).
 *         Example:
 *         unsigned int *idx_ptr;
 *         EINA_ITERATOR_FOREACH(iterator, idx_ptr) {
 *           printf("Index %u has value %d for property %s\n", *idx_ptr, request, name);
 *         }
 */
static Eina_Iterator *
_efl_boolean_model_boolean_iterator_get(Eo *obj, Efl_Boolean_Model_Data *pd, const char *name, Eina_Bool request)
{
   Eina_Iterator_Boolean *itb;
   Efl_Boolean_Model_Value *v;
   Eina_Stringshare *s;

   s = eina_stringshare_add(name);
   v = eina_hash_find(pd->values, s);
   eina_stringshare_del(s);
   if (!v) return NULL;

   itb = calloc(1, sizeof (Eina_Iterator_Boolean));
   if (!itb) return NULL;

   itb->obj = efl_ref(obj);
   itb->pd = pd;
   itb->v = v;
   itb->infix = eina_rbtree_iterator_infix(v->buffers_root);
   // Search the first index that do have the valid value
   _efl_boolean_model_iterator_index_find(itb);
   itb->index = 0;
   itb->total = efl_model_children_count_get(obj);
   itb->request = !!request;

   itb->iterator.version = EINA_ITERATOR_VERSION;
   itb->iterator.next = FUNC_ITERATOR_NEXT(efl_boolean_model_iterator_next);
   itb->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(efl_boolean_model_iterator_get_container);
   itb->iterator.free = FUNC_ITERATOR_FREE(efl_boolean_model_iterator_free);

   EINA_MAGIC_SET(&itb->iterator, EINA_MAGIC_ITERATOR);
   return &itb->iterator;
}


#include "efl_boolean_model.eo.c"
