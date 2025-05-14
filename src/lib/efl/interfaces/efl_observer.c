#include "config.h"
#include "Efl.h"

/**
 * @brief Private data for the Efl_Observable interface.
 *
 * This structure holds a hash table where keys are strings (event names or
 * property names) and values are themselves hash tables. These inner hash
 * tables map Efl_Observer pointers to Efl_Observer_Refcount structures.
 *
 * @see _efl_observable_observer_add
 * @see _efl_observable_observer_del
 */
typedef struct
{
   Eina_Hash *observers; /**< Hash table of observers. Key: (const char *) event name. Value: (Eina_Hash *) map of Efl_Observer* to Efl_Observer_Refcount*. */
} Efl_Observable_Data;

/**
 * @brief Structure to manage reference counting for observers.
 *
 * This is used to allow multiple additions of the same observer for the same
 * key without duplicating it, and to correctly remove it only when all
 * corresponding `observer_del` calls have been made.
 */
typedef struct
{
   EINA_REFCOUNT; /**< Reference counter for the observer. */

   Efl_Observer *o; /**< The actual observer object. */
} Efl_Observer_Refcount;

/**
 * @internal
 * @brief Constructor for Efl_Observable objects.
 *
 * Initializes the private data, specifically the main hash table for observers.
 *
 * @param obj The Efl_Object being constructed.
 * @param pd Pointer to the private data for Efl_Observable.
 * @return The constructed Efl_Object.
 */
EOLIAN static Eo *
_efl_observable_efl_object_constructor(Efl_Object *obj, Efl_Observable_Data *pd)
{
   pd->observers = eina_hash_string_superfast_new((Eina_Free_Cb)eina_hash_free);

   obj = efl_constructor(efl_super(obj, EFL_OBSERVABLE_CLASS));

   return obj;
}

/**
 * @internal
 * @brief Destructor for Efl_Observable objects.
 *
 * Frees the main hash table for observers.
 *
 * @param obj The Efl_Object being destructed.
 * @param pd Pointer to the private data for Efl_Observable.
 */
EOLIAN static void
_efl_observable_efl_object_destructor(Eo *obj, Efl_Observable_Data *pd)
{
   eina_hash_free(pd->observers);

   efl_destructor(efl_super(obj, EFL_OBSERVABLE_CLASS));
}

/**
 * @internal
 * @brief Adds an observer for a specific key (event/property name).
 *
 * If the key does not exist, a new inner hash table is created for it.
 * If the observer is already registered for this key, its reference count is incremented.
 * Otherwise, a new Efl_Observer_Refcount structure is created and added.
 *
 * @param obj The Efl_Observable object.
 * @param pd Pointer to the private data for Efl_Observable.
 * @param key The string key (e.g., event name) to observe.
 * @param obs The Efl_Observer object to add.
 */
EOLIAN static void
_efl_observable_observer_add(Eo *obj EINA_UNUSED, Efl_Observable_Data *pd, const char *key, Efl_Observer *obs)
{
   Eina_Hash *observers;
   Efl_Observer_Refcount *or;

   if (!key) return;

   observers = eina_hash_find(pd->observers, key);
   if (!observers)
     {
        observers = eina_hash_pointer_new(free);
        eina_hash_add(pd->observers, key, observers);
     }

   or = eina_hash_find(observers, &obs);
   if (!or)
     {
        or = calloc(1, sizeof(Efl_Observer_Refcount));
        or->o = obs;
        EINA_REFCOUNT_INIT(or);

        eina_hash_direct_add(observers, &or->o, or);
     }
   else
     {
        EINA_REFCOUNT_REF(or);
     }
}

/**
 * @internal
 * @brief Deletes an observer for a specific key.
 *
 * Decrements the reference count of the observer for the given key.
 * If the reference count reaches zero, the observer is removed from the inner hash.
 * If the inner hash becomes empty, it is removed from the main observers hash.
 *
 * @param obj The Efl_Observable object.
 * @param pd Pointer to the private data for Efl_Observable.
 * @param key The string key (e.g., event name) from which to remove the observer.
 * @param obs The Efl_Observer object to remove.
 */
EOLIAN static void
_efl_observable_observer_del(Eo *obj EINA_UNUSED, Efl_Observable_Data *pd, const char *key, Efl_Observer *obs)
{
   Eina_Hash *observers;
   Efl_Observer_Refcount *or;

   if (!key) return;

   observers = eina_hash_find(pd->observers, key);
   if (!observers) return;

   or = eina_hash_find(observers, &obs);
   if (!or) return;

   EINA_REFCOUNT_UNREF(or)
     {
        eina_hash_del(observers, &or->o, or);

        if (eina_hash_population(observers) == 0)
          {
             eina_hash_del(pd->observers, key, observers);
          }
     }
}

/**
 * @internal
 * @brief Removes a specific observer from all keys it is registered to.
 *
 * This function iterates through all keys in the observable's main hash table.
 * For each key, it checks if the given observer is registered. If so, it
 * decrements its reference count and removes it if the count reaches zero.
 * This is typically used when an observer is being destroyed and needs to
 * unregister itself from all observables it was listening to.
 *
 * @param obj The Efl_Observable object.
 * @param pd Pointer to the private data for Efl_Observable.
 * @param obs The Efl_Observer object to clean up.
 */
EOLIAN static void
_efl_observable_observer_clean(Eo *obj EINA_UNUSED, Efl_Observable_Data *pd, Efl_Observer *obs)
{
   Eina_Iterator *it;
   Eina_Hash *observers;

   it = eina_hash_iterator_data_new(pd->observers);
   EINA_ITERATOR_FOREACH(it, observers)
     {
        Efl_Observer_Refcount *or;

        or = eina_hash_find(observers, &obs);
        if (!or) continue;

        EINA_REFCOUNT_UNREF(or)
          {
             eina_hash_del(observers, &obs, or);
          }
     }
   eina_iterator_free(it);
}

/**
 * @internal
 * @brief Private data for an iterator over observers for a specific key.
 */
typedef struct
{
   Eina_Iterator iterator; /**< The public Eina_Iterator structure. */
   Eina_Iterator *classes; /**< Internal iterator over Efl_Observer_Refcount data in the hash. */
} Efl_Observer_Iterator;

/**
 * @internal
 * @brief Advances the observer iterator to the next observer.
 *
 * @param it The Efl_Observer_Iterator.
 * @param data Pointer to store the next Efl_Observer*.
 * @return EINA_TRUE if a next observer was found, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_observable_observers_iterator_next(Eina_Iterator *it, void **data)
{
   Efl_Observer_Iterator *et = (void *)it;
   Efl_Observer_Refcount *or = NULL;

   if (!eina_iterator_next(et->classes, (void **)&or)) return EINA_FALSE;
   if (!or) return EINA_FALSE;

   *data = or->o; // Store the actual Efl_Observer object

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the observer iterator (always NULL).
 *
 * @param it The Efl_Observer_Iterator.
 * @return Always NULL.
 */
static void *
_efl_observable_observers_iterator_container(Eina_Iterator *it EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Frees an Efl_Observer_Iterator.
 *
 * @param it The Efl_Observer_Iterator to free.
 */
static void
_efl_observable_observers_iterator_free(Eina_Iterator *it)
{
   Efl_Observer_Iterator *et = (void *)it;

   eina_iterator_free(et->classes); // Free the internal hash iterator
   EINA_MAGIC_SET(&et->iterator, 0);
   free(et);
}

/**
 * @internal
 * @brief Creates a new iterator for observers associated with a specific key.
 *
 * This iterator will yield Efl_Observer pointers.
 *
 * @param obj The Efl_Observable object.
 * @param pd Pointer to the private data for Efl_Observable.
 * @param key The key for which to get an observer iterator.
 * @return A new Eina_Iterator over Efl_Observer objects, or NULL on failure or if key not found.
 */
EOLIAN static Eina_Iterator *
_efl_observable_observers_iterator_new(Eo *obj EINA_UNUSED, Efl_Observable_Data *pd, const char *key)
{
   Eina_Hash *observers;
   Efl_Observer_Iterator *it;

   observers = eina_hash_find(pd->observers, key);
   if (!observers) return NULL;

   it = calloc(1, sizeof(Efl_Observer_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->classes = eina_hash_iterator_data_new(observers);

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = _efl_observable_observers_iterator_next;
   it->iterator.get_container = _efl_observable_observers_iterator_container;
   it->iterator.free = _efl_observable_observers_iterator_free;

   return &it->iterator;
}

/**
 * @internal
 * @brief Notifies all observers registered for a given key about an update.
 *
 * Iterates over all observers for the specified key and calls `efl_observer_update`
 * on each of them.
 *
 * @param obj The Efl_Observable object that is sending the update.
 * @param pd Pointer to the private data for Efl_Observable.
 * @param key The key (event/property name) that has been updated.
 * @param data Optional data associated with the update.
 */
EOLIAN static void
_efl_observable_observers_update(Eo *obj, Efl_Observable_Data *pd EINA_UNUSED, const char *key, void *data)
{
   Eina_Iterator *it;
   Efl_Observer *o;

   it = efl_observable_observers_iterator_new(obj, key);
   if (!it) return;

   EINA_ITERATOR_FOREACH(it, o)
     {
        efl_observer_update(o, obj, key, data);
     }

   eina_iterator_free(it);
}

/**
 * @internal
 * @brief Private data for an iterator over (key, observer_iterator) tuples.
 *
 * This iterator is used to iterate over all observed keys and their
 * corresponding observer lists.
 */
typedef struct
{
   Eina_Iterator iterator;    /**< The public Eina_Iterator structure. */
   Eina_Iterator *classes;  /**< Internal iterator over the keys (const char*) of the main `pd->observers` hash. */
   Efl_Observable *obs;     /**< The Efl_Observable object this iterator belongs to. */
   Eina_List *tuples;       /**< List to keep track of allocated Efl_Observable_Tuple to free them later. */
} Efl_Observable_Iterator;

/**
 * @internal
 * @brief Advances the tuple iterator to the next (key, observer_iterator) pair.
 *
 * Each item yielded by this iterator is an #Efl_Observable_Tuple.
 * The `key` member of the tuple is a `const char*` (the event/property name).
 * The `data` member of the tuple is an #Eina_Iterator over #Efl_Observer objects for that key.
 *
 * @param it The Efl_Observable_Iterator.
 * @param data Pointer to store the next Efl_Observable_Tuple*.
 * @return EINA_TRUE if a next tuple was found, EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_observable_iterator_tuple_next(Eina_Iterator *it, void **data)
{
   Efl_Observable_Iterator *et = (void *)it;
   Efl_Observable_Tuple *tuple;
   const char *key;

   // Get the next key from the main observers hash
   if (!eina_iterator_next(et->classes, (void **)&key)) return EINA_FALSE;
   if (!key) return EINA_FALSE;

   tuple = calloc(1, sizeof(Efl_Observable_Tuple));
   if (!tuple) return EINA_FALSE;

   tuple->key = key; // Key is not owned by the tuple, it's from the hash
   // Create an iterator for observers associated with this key
   tuple->data = efl_observable_observers_iterator_new(et->obs, key);

   // Keep track of the tuple for later freeing
   et->tuples = eina_list_append(et->tuples, tuple);
   *data = tuple;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container of the tuple iterator (always NULL).
 *
 * @param it The Efl_Observable_Iterator.
 * @return Always NULL.
 */
static void *
_efl_observable_iterator_tuple_container(Eina_Iterator *it EINA_UNUSED)
{
   return NULL;
}

/**
 * @internal
 * @brief Frees an Efl_Observable_Iterator and all associated tuples.
 *
 * @param it The Efl_Observable_Iterator to free.
 */
static void
_efl_observable_iterator_tuple_free(Eina_Iterator *it)
{
   Efl_Observable_Iterator *et = (void *)it;
   Efl_Observable_Tuple *tuple;

   eina_iterator_free(et->classes); // Free the key iterator
   // Free all allocated tuples and their data (observer iterators)
   EINA_LIST_FREE(et->tuples, tuple)
     {
        if (tuple->data) // This is an Eina_Iterator for observers
          eina_iterator_free(tuple->data);
        free(tuple); // Free the Efl_Observable_Tuple itself
     }
   EINA_MAGIC_SET(&et->iterator, 0);
   free(et); // Free the Efl_Observable_Iterator structure
}

/**
 * @internal
 * @brief Creates a new iterator that yields Efl_Observable_Tuple objects.
 *
 * Each tuple contains a key (const char *) and an iterator (Eina_Iterator *)
 * over the Efl_Observer objects registered for that key.
 *
 * @param obj The Efl_Observable object.
 * @param pd Pointer to the private data for Efl_Observable.
 * @return A new Eina_Iterator over Efl_Observable_Tuple objects, or NULL on failure.
 */
EOLIAN static Eina_Iterator *
_efl_observable_iterator_tuple_new(Eo *obj, Efl_Observable_Data *pd)
{
   Efl_Observable_Iterator *it;

   it = calloc(1, sizeof(Efl_Observable_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->classes = eina_hash_iterator_key_new(pd->observers);
   it->obs = obj;

   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = _efl_observable_iterator_tuple_next;
   it->iterator.get_container = _efl_observable_iterator_tuple_container;
   it->iterator.free = _efl_observable_iterator_tuple_free;

   return &it->iterator;
}

/**
 * @brief Frees an Efl_Observable_Tuple.
 * @param tuple The tuple to free.
 * @since 1.22
 *
 * This function is intended to be used by consumers of the
 * #efl_observable_iterator_tuple_new iterator if they need to free
 * individual tuples before the main iterator is freed (which is rare).
 * Normally, tuples are freed when the Efl_Observable_Iterator itself is freed.
 *
 * Note: This function only frees the `data` (observer iterator) part of the tuple.
 * The `key` is not owned by the tuple and is not freed here. The tuple
 * itself is also not freed by this function, as it's typically allocated
 * and managed by the Efl_Observable_Iterator.
 *
 * If you are iterating and breaking early, and want to free the tuple *data*
 * (the observer iterator) you obtained, you can use this.
 * However, the tuple pointer itself is managed by the parent iterator.
 */
EAPI void
efl_observable_tuple_free(Efl_Observable_Tuple *tuple)
{
   //key is not owned by the tuple, it's a pointer to the hash key
   if (tuple && tuple->data)
     eina_iterator_free(tuple->data);
   // The tuple itself is freed by the Efl_Observable_Iterator's free function
   // when it cleans up its list of allocated tuples.
}

#include "interfaces/efl_observable.eo.c"
#include "interfaces/efl_observer.eo.c"
