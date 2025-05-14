#include "evas_common_private.h"

/**
 * @file
 * @brief This file implements a generic cache with LRU eviction policy.
 */

/**
 * @brief Creates a new generic cache.
 *
 * @param user_data Custom data to be passed to the free function.
 * @param func Function to call when an entry is removed from the cache and needs to be freed.
 * @return A pointer to the newly created Generic_Cache, or NULL on failure.
 */
EVAS_API Generic_Cache*
generic_cache_new(void *user_data, Generic_Cache_Free func)
{
   Generic_Cache *cache;
   cache = calloc(1, sizeof(Generic_Cache));
   cache->hash = eina_hash_int32_new(NULL);
   cache->user_data = user_data;
   cache->free_func = func;
   return cache;
}

/**
 * @brief Destroys a generic cache and frees its resources.
 *
 * This function frees all entries currently in the cache's LRU list
 * but does not call the user-provided free function for the data
 * associated with those entries. It also frees the hash table and
 * the cache structure itself.
 *
 * @param cache The cache to destroy.
 */
EVAS_API void
generic_cache_destroy(Generic_Cache *cache)
{
   Generic_Cache_Entry *entry;
   if (cache)
     {
        EINA_LIST_FREE(cache->lru_list, entry)
          {
             free(entry);
          }

        eina_hash_free(cache->hash);
        free(cache);
     }
}

/**
 * @brief Dumps a generic cache, freeing all cached data.
 *
 * This function iterates through all entries in the cache's LRU list,
 * calls the user-provided free function for the data associated with each entry,
 * and then frees the entry structure itself. It also frees the hash table buckets.
 * Note: This function does not free the cache structure itself, only its contents.
 *
 * @param cache The cache to dump.
 */
EVAS_API void
generic_cache_dump(Generic_Cache *cache)
{
   Generic_Cache_Entry *entry;
   if (cache)
     {
        eina_hash_free_buckets(cache->hash);
        EINA_LIST_FREE(cache->lru_list, entry)
          {
             cache->free_func(cache->user_data, entry->data);
             free(entry);
          }
     }
}

/**
 * @brief Adds or updates data in the cache associated with a key.
 *
 * If the key already exists, its associated data is updated, and the entry
 * is promoted in the LRU list. If the key does not exist, a new entry is
 * created. If adding the new entry exceeds the cache limit (currently 50),
 * the least recently used item is evicted, provided its reference count is 1.
 *
 * @param cache The cache to modify.
 * @param key A pointer to the key for the data. The cache uses the address of the key.
 * @param surface A pointer to the data to be cached.
 */
EVAS_API void
generic_cache_data_set(Generic_Cache *cache, void *key, void *surface)
{
   Generic_Cache_Entry *entry = NULL;
   int count;

   entry = calloc(1, sizeof(Generic_Cache_Entry));
   entry->key = key;
   entry->data = surface;
   entry->ref = 1;
   eina_hash_add(cache->hash, &key, entry);
   cache->lru_list = eina_list_prepend(cache->lru_list, entry);
   count = eina_list_count(cache->lru_list);
   if (count > 50)
   {
      entry = eina_list_data_get(eina_list_last(cache->lru_list));
      // if its still being ref.
      if (entry->ref > 1) return;
      eina_hash_del(cache->hash, &entry->key, entry);
      cache->lru_list = eina_list_remove_list(cache->lru_list, eina_list_last(cache->lru_list));
      cache->free_func(cache->user_data, entry->data);
      free(entry);
   }
}

/**
 * @brief Retrieves data from the cache associated with a key.
 *
 * If the key is found, the associated data is returned, its reference count
 * is incremented, and the entry is promoted in the LRU list.
 *
 * @param cache The cache to query.
 * @param key A pointer to the key for the data.
 * @return A pointer to the cached data if found, otherwise NULL.
 */
EVAS_API void *
generic_cache_data_get(Generic_Cache *cache, void *key)
{
   Generic_Cache_Entry *entry = NULL, *lru_data;
   Eina_List *l;

   entry =  eina_hash_find(cache->hash, &key);
   if (entry)
     {
        // update the ref
        entry->ref += 1;
        // promote in lru
        EINA_LIST_FOREACH(cache->lru_list, l, lru_data)
          {
            if (lru_data == entry)
              {
                 cache->lru_list = eina_list_promote_list(cache->lru_list, l);
                 break;
              }
          }
        return entry->data;
     }
   return NULL;
}

/**
 * @brief Decrements the reference count of a cached item.
 *
 * If the reference count drops to zero, the item is removed from the cache,
 * and the user-provided free function is called for its data.
 *
 * @param cache The cache to modify.
 * @param key A pointer to the key of the data to drop.
 */
EVAS_API void
generic_cache_data_drop(Generic_Cache *cache, void *key)
{
   Generic_Cache_Entry *entry = NULL;

   entry =  eina_hash_find(cache->hash, &key);
   if (entry)
     {
        entry->ref -= 1;
        // if its still being ref.
        if (entry->ref) return;
        eina_hash_del(cache->hash, &entry->key, entry);
        // find and remove from lru list
        cache->lru_list = eina_list_remove(cache->lru_list, entry);
        cache->free_func(cache->user_data, entry->data);
        free(entry);
     }
}

