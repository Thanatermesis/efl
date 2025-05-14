#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Marks an engine image entry as dirty.
 *
 * This function sets the necessary flags on the engine image entry (@p eim)
 * to indicate it's dirty and adds it to the dirty list in the cache.
 * A dirty entry is one that has been modified and needs to be processed or
 * potentially written back.
 *
 * @param cache The engine image cache.
 * @param eim The engine image entry to mark as dirty.
 */
static void
_evas_cache_engine_image_make_dirty(Evas_Cache_Engine_Image *cache,
                                    Engine_Image_Entry *eim)
{
   eim->flags.cached = 1;
   eim->flags.dirty = 1;
   eim->flags.loaded = 1;
   eim->flags.activ = 0;
   cache->dirty = eina_inlist_prepend(cache->dirty, EINA_INLIST_GET(eim));
}

/**
 * @internal
 * @brief Marks an engine image entry as active.
 *
 * This function sets the necessary flags on the engine image entry (@p eim)
 * to indicate it's active and adds it to the active hash in the cache
 * using the provided @p key. Active entries are currently in use.
 *
 * @param cache The engine image cache.
 * @param eim The engine image entry to mark as active.
 * @param key The cache key associated with the image entry.
 */
static void
_evas_cache_engine_image_make_active(Evas_Cache_Engine_Image *cache,
                                     Engine_Image_Entry *eim,
                                     const char *key)
{
   eim->flags.cached = 1;
   eim->flags.activ = 1;
   eim->flags.dirty = 0;
   eina_hash_add(cache->activ, key, eim);
}

/**
 * @internal
 * @brief Marks an engine image entry as inactive.
 *
 * This function sets the necessary flags on the engine image entry (@p eim)
 * to indicate it's inactive. It adds the entry to the inactive hash and
 * the LRU (Least Recently Used) list in the cache. The memory usage
 * of the cache is updated. Inactive entries are not currently in use but
 * are kept in cache for potential reuse.
 *
 * @param cache The engine image cache.
 * @param eim The engine image entry to mark as inactive.
 * @param key The cache key associated with the image entry.
 */
static void
_evas_cache_engine_image_make_inactive(Evas_Cache_Engine_Image *cache,
                                       Engine_Image_Entry *eim,
                                       const char *key)
{
   eim->flags.cached = 1;
   eim->flags.dirty = 0;
   eim->flags.activ = 0;
   eina_hash_add(cache->inactiv, key, eim);
   cache->lru = eina_inlist_prepend(cache->lru, EINA_INLIST_GET(eim));
   cache->usage += cache->func.mem_size_get(eim);
}

/**
 * @internal
 * @brief Removes an engine image entry from its current state in the cache.
 *
 * This function handles the removal of an engine image entry (@p eim) from
 * whichever list or hash it currently resides in (dirty, active, or inactive).
 * It updates cache usage accordingly if the entry was in the inactive list.
 * Finally, it resets the cached, dirty, and active flags for the entry.
 *
 * @param cache The engine image cache.
 * @param eim The engine image entry to remove.
 */
static void
_evas_cache_engine_image_remove_activ(Evas_Cache_Engine_Image *cache,
                                      Engine_Image_Entry *eim)
{
   if (eim->flags.cached)
     {
        if (eim->flags.dirty)
          cache->dirty = eina_inlist_remove(cache->dirty,
                                            EINA_INLIST_GET(eim));
        else
          {
             if (eim->flags.activ)
               eina_hash_del(cache->activ, eim->cache_key, eim);
             else
               {
                  cache->usage -= cache->func.mem_size_get(eim);
                  eina_hash_del(cache->inactiv, eim->cache_key, eim);
                  cache->lru = eina_inlist_remove(cache->lru,
                                                  EINA_INLIST_GET(eim));
               }
          }
        eim->flags.cached = 0;
        eim->flags.dirty = 0;
        eim->flags.activ = 0;
     }
}

/**
 * @internal
 * @brief Allocates and initializes a new engine image entry.
 *
 * This function allocates memory for a new Engine_Image_Entry. It initializes
 * its members, including dimensions (w, h) and source image entry (@p ie) if provided.
 * It sets the cache key (@p hkey) and then makes the new entry active or dirty
 * based on whether a key is provided.
 *
 * @param cache The engine image cache.
 * @param ie Optional parent Image_Entry. If NULL, the engine image entry
 *           is considered to not have a parent.
 * @param hkey The hash key for this engine image. If NULL, the entry is
 *             initially marked as dirty. Otherwise, it's made active.
 * @return A pointer to the newly allocated Engine_Image_Entry, or NULL on failure.
 */
static Engine_Image_Entry *
_evas_cache_engine_image_alloc(Evas_Cache_Engine_Image *cache,
                               Image_Entry *ie,
                               const char *hkey)
{
   Engine_Image_Entry *eim;

   assert(cache);

   if (cache->func.alloc) eim = cache->func.alloc();
   else eim = malloc(sizeof (Engine_Image_Entry));

   if (!eim) goto on_error;
   memset(eim, 0, sizeof (Engine_Image_Entry));

   eim->cache = cache;
   if (ie)
     {
        eim->w = ie->w;
        eim->h = ie->h;
        eim->src = ie;
        eim->flags.need_parent = 1;
     }
   else
     {
        eim->w = -1;
        eim->h = -1;
        eim->flags.need_parent = 0;
        eim->src = NULL;
     }

   eim->flags.cached = 0;
   eim->references = 0;
   eim->cache_key = hkey;

   if (hkey) _evas_cache_engine_image_make_active(cache, eim, hkey);
   else _evas_cache_engine_image_make_dirty(cache, eim);

   return eim;

on_error:
   eina_stringshare_del(hkey);
   if (ie) evas_cache_image_drop(ie);
   return NULL;
}

/**
 * @internal
 * @brief Deallocates an engine image entry.
 *
 * This function first removes the engine image entry (@p eim) from its
 * active/inactive/dirty state in the cache. It then calls the engine-specific
 * destructor for the image data and drops the reference to its parent
 * Image_Entry, if any. Finally, it deallocates the Engine_Image_Entry structure
 * itself, either using a custom deallocator if provided by the engine or
 * by adding it to a general free queue.
 *
 * @param cache The engine image cache.
 * @param eim The engine image entry to deallocate.
 */
static void
_evas_cache_engine_image_dealloc(Evas_Cache_Engine_Image *cache,
                                 Engine_Image_Entry *eim)
{
   Image_Entry *im;

   if (cache->func.debug) cache->func.debug("delete", eim);

   _evas_cache_engine_image_remove_activ(cache, eim);

   im = eim->src;
   cache->func.destructor(eim);
   if (im) evas_cache_image_drop(im);

   if (cache->func.dealloc) cache->func.dealloc(eim);
   else
     {
        memset(eim, 0, sizeof(Engine_Image_Entry));
        eina_freeq_ptr_add(eina_freeq_main_get(), eim, free, sizeof(*eim));
     }
}

/**
 * @brief Get the current memory usage of the image cache.
 * @param cache The image cache.
 * @return The current memory usage in bytes.
 *
 * This function returns the total memory currently used by images
 * stored in the inactive list of the cache.
 */
EVAS_API int
evas_cache_engine_image_usage_get(Evas_Cache_Engine_Image *cache)
{
   assert(cache != NULL);
   return cache->usage;
}

/**
 * @brief Get the memory limit of the image cache.
 * @param cache The image cache.
 * @return The memory limit in bytes.
 *
 * This function returns the configured memory limit for the cache.
 * When cache usage exceeds this limit, images may be evicted.
 */
EVAS_API int
evas_cache_engine_image_get(Evas_Cache_Engine_Image *cache)
{
   assert(cache != NULL);
   return cache->limit;
}

/**
 * @brief Set the memory limit of the image cache.
 * @param cache The image cache.
 * @param limit The new memory limit in bytes.
 *
 * This function sets the maximum amount of memory the cache can use.
 */
EVAS_API void
evas_cache_engine_image_set(Evas_Cache_Engine_Image *cache, int limit)
{
   assert(cache != NULL);
   cache->limit = limit;
}

/**
 * @brief Initializes a new engine image cache.
 * @param cb Pointer to a structure containing callback functions for engine-specific operations.
 *           These functions handle tasks like allocation, deallocation, construction,
 *           destruction, and memory size calculation for engine image entries.
 *           Example:
 *           Evas_Cache_Engine_Image_Func my_callbacks = {
 *             .alloc = my_engine_alloc_func,
 *             .dealloc = my_engine_dealloc_func,
 *             .constructor = my_engine_constructor_func,
 *             .destructor = my_engine_destructor_func,
 *             .mem_size_get = my_engine_mem_size_get_func,
 *             // ... other callbacks
 *           };
 * @param parent Pointer to the parent Evas_Cache_Image. This engine cache
 *               will be associated with this parent image cache.
 * @return A pointer to the newly initialized Evas_Cache_Engine_Image, or NULL on failure.
 *
 * This function allocates and sets up a new engine-specific image cache.
 * It copies the provided callback functions, initializes cache limits and usage,
 * creates hash tables for active and inactive images, and links to the parent
 * image cache.
 */
EVAS_API Evas_Cache_Engine_Image *
evas_cache_engine_image_init(const Evas_Cache_Engine_Image_Func *cb,
                             Evas_Cache_Image *parent)
{
   Evas_Cache_Engine_Image *new;

   new = malloc(sizeof (Evas_Cache_Engine_Image));
   if (!new) return NULL;

   new->func = *cb;

   new->limit = 0;
   new->usage = 0;

   new->dirty = NULL;
   new->lru = NULL;
   new->activ = eina_hash_string_superfast_new(NULL);
   new->inactiv = eina_hash_string_superfast_new(NULL);

   new->parent = parent;
   parent->references++;

   new->brother = NULL;

   return new;
}

/**
 * @brief Duplicates an existing engine image cache, potentially overriding some callbacks.
 * @param cb Pointer to a structure containing callback functions to override.
 *           If a callback in @p cb is non-NULL, it will be used in the new cache;
 *           otherwise, the callback from the @p brother cache is used.
 *           Example:
 *           Evas_Cache_Engine_Image_Func override_callbacks = {
 *             .dirty = my_custom_dirty_func, // Override only the dirty callback
 *             .load = NULL // Keep brother's load callback
 *           };
 * @param brother Pointer to the existing Evas_Cache_Engine_Image to duplicate.
 * @return A pointer to the newly duplicated Evas_Cache_Engine_Image, or NULL on failure.
 *
 * This function creates a new engine image cache that is a "brother" to an existing one.
 * It shares the same parent image cache but can have its own set of callback functions,
 * allowing for different engine behaviors or specializations while operating on the
 * same underlying image data. The new cache starts with a reference count of 1.
 * The limit is initialized to -1 (unlimited) and usage to 0. Active and LRU lists
 * are initialized as empty.
 */
EVAS_API Evas_Cache_Engine_Image *
evas_cache_engine_image_dup(const Evas_Cache_Engine_Image_Func *cb,
                            Evas_Cache_Engine_Image *brother)
{
   Evas_Cache_Engine_Image *new;

   new = calloc(1, sizeof(Evas_Cache_Engine_Image));
   if (!new) return NULL;

   new->func = brother->func;

#define ORD(Func) if (cb->Func) new->func.Func = cb->Func;
   ORD(key);
   ORD(constructor);
   ORD(destructor);
   ORD(dirty_region);
   ORD(dirty);
   ORD(size_set);
   ORD(update_data);
   ORD(load);
   ORD(mem_size_get);
   ORD(debug);
#undef ORD

   new->limit = -1;
   new->usage = 0;
   new->references = 1;

   new->dirty = NULL;
   new->activ = NULL;

   new->parent = brother->parent;
   new->parent->references++;

   new->brother = brother;
   brother->references++;

   return new;
}

/**
 * @internal
 * @brief Callback function used with eina_hash_foreach to collect image entries for deletion.
 *
 * This function is called for each entry in a hash table. It takes the
 * data (an Engine_Image_Entry pointer) from the hash entry and prepends it
 * to the Eina_List pointed to by @p fdata.
 *
 * @param hash The hash table being iterated. (Unused)
 * @param key The key of the current hash entry. (Unused)
 * @param data Pointer to the Engine_Image_Entry stored in the hash.
 * @param fdata Pointer to an Eina_List pointer (Eina_List **). This list
 *              accumulates the image entries to be deleted.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_evas_cache_engine_image_free_cb(EINA_UNUSED const Eina_Hash *hash,
                                 EINA_UNUSED const void *key,
                                 void *data, void *fdata)
{
   Eina_List **delete_list = fdata;

   *delete_list = eina_list_prepend(*delete_list, data);
   return EINA_TRUE;
}

/**
 * @brief Flushes the engine image cache to respect its memory limit.
 * @param cache The engine image cache to flush.
 *
 * This function iterates through the LRU (Least Recently Used) list of
 * inactive images and deallocates them until the cache's memory usage
 * is below its configured limit.
 */
EVAS_API void
evas_cache_engine_image_flush(Evas_Cache_Engine_Image *cache)
{
   assert(cache != NULL);

   while ((cache->lru) && (cache->limit < cache->usage))
     {
        Engine_Image_Entry *eim;

        eim = (Engine_Image_Entry *)cache->lru->last;
        _evas_cache_engine_image_dealloc(cache, eim);
     }
}

/**
 * @brief Shuts down and frees an engine image cache.
 * @param cache The engine image cache to shut down.
 *
 * This function deallocates all image entries currently held by the cache,
 * regardless of their state (active, inactive, dirty). It then frees the
 * hash tables and other resources used by the cache structure itself.
 * It also recursively shuts down its parent image cache and any "brother"
 * engine caches.
 */
EVAS_API void
evas_cache_engine_image_shutdown(Evas_Cache_Engine_Image *cache)
{
   Engine_Image_Entry *eim;
   Eina_List *delete_list = NULL;

   assert(cache != NULL);

   if (cache->func.debug) cache->func.debug("shutdown-engine", NULL);

   eina_hash_foreach(cache->inactiv, _evas_cache_engine_image_free_cb,
                     &delete_list);
   eina_hash_foreach(cache->activ, _evas_cache_engine_image_free_cb,
                     &delete_list);

   while (delete_list)
     {
        _evas_cache_engine_image_dealloc(cache,
                                         eina_list_data_get(delete_list));
        delete_list = eina_list_remove_list(delete_list, delete_list);
     }

   eina_hash_free(cache->inactiv);
   eina_hash_free(cache->activ);

   /* This is mad, I am about to destroy image still alive, but we need to
    * prevent leak. */
   while (cache->dirty)
     {
        eim = (Engine_Image_Entry *)cache->dirty;
        _evas_cache_engine_image_dealloc(cache, eim);
     }

   evas_cache_image_shutdown(cache->parent);
   if (cache->brother) evas_cache_engine_image_shutdown(cache->brother);
   free(cache);
}

/**
 * @brief Requests an engine image entry from the cache.
 *
 * This function attempts to retrieve an engine-specific image entry.
 * It first requests the corresponding generic Image_Entry from the parent cache.
 * Then, it generates an engine-specific key.
 *
 * The function checks if an entry with this key exists in the active or
 * inactive lists. If found, it's reused and made active.
 * If not found, a new Engine_Image_Entry is allocated, and the engine-specific
 * constructor is called to prepare the image data for the engine.
 *
 * @param cache The engine image cache.
 * @param file The path to the image file (can be NULL if key is used).
 * @param key An optional key for the image (can be NULL if file is used).
 * @param lo Pointer to image loading options.
 * @param data Engine-specific data to be passed to the constructor.
 * @param[out] error Pointer to an integer where an Evas_Load_Error code will be stored.
 *                   Example error codes:
 *                   - EVAS_LOAD_ERROR_NONE: Success.
 *                   - EVAS_LOAD_ERROR_GENERIC: A generic error.
 *                   - EVAS_LOAD_ERROR_DOES_NOT_EXIST: File does not exist.
 *                   - EVAS_LOAD_ERROR_PERMISSION_DENIED: Permission denied to read file.
 *                   - EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: Memory allocation failed.
 *                   - EVAS_LOAD_ERROR_CORRUPT_FILE: Image file is corrupt or format not recognized.
 *                   - EVAS_LOAD_ERROR_UNKNOWN_FORMAT: Image file format is unknown.
 * @return A pointer to the requested Engine_Image_Entry with its reference count incremented,
 *         or NULL on failure (with @p error set accordingly).
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_request(Evas_Cache_Engine_Image *cache,
                                const char *file, const char *key,
                                Evas_Image_Load_Opts *lo, void *data,
                                int *error)
{
   Engine_Image_Entry *eim;
   Image_Entry *im;
   const char *ekey;

   assert(cache != NULL);

   *error = EVAS_LOAD_ERROR_NONE;

   ekey = NULL;
   eim = NULL;

   im = evas_cache_image_request(cache->parent, file, key, lo, error);
   if (!im) goto on_error;

   if (cache->func.key) ekey = cache->func.key(im, file, key, lo, data);
   else ekey = eina_stringshare_add(im->cache_key);
   if (!ekey)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        goto on_error;
     }

   eim = eina_hash_find(cache->activ, ekey);
   if (eim)
     {
        evas_cache_image_drop(im);
        goto on_ok;
     }

   eim = eina_hash_find(cache->inactiv, ekey);
   if (eim)
     {
        _evas_cache_engine_image_remove_activ(cache, eim);
        _evas_cache_engine_image_make_active(cache, eim, ekey);
        evas_cache_image_drop(im);
        goto on_ok;
     }

   eim = _evas_cache_engine_image_alloc(cache, im, ekey);
   if (!eim)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }

   *error = cache->func.constructor(eim, data);
   if (*error != EVAS_LOAD_ERROR_NONE) goto on_error;
   if (cache->func.debug) cache->func.debug("constructor-engine", eim);

on_ok:
   eim->references++;
   return eim;

on_error:
   if (!eim)
     {
        if (im) evas_cache_image_drop(im);
     }
   else _evas_cache_engine_image_dealloc(cache, eim);

   return NULL;
}

/**
 * @brief Decrements the reference count of an engine image entry.
 *
 * When the reference count of an engine image entry (@p eim) reaches zero,
 * this function moves it from the active list to the inactive list (making it
 * eligible for eviction by the cache flushing mechanism). If the entry is
 * marked as dirty, it is deallocated immediately instead of being made inactive.
 *
 * @param eim The engine image entry to drop.
 */
EVAS_API void
evas_cache_engine_image_drop(Engine_Image_Entry *eim)
{
   Evas_Cache_Engine_Image *cache;

   assert(eim);
   assert(eim->cache);

   eim->references--;
   cache = eim->cache;

   if (eim->flags.dirty)
     {
        _evas_cache_engine_image_dealloc(cache, eim);
        return;
     }

   if (eim->references == 0)
     {
        _evas_cache_engine_image_remove_activ(cache, eim);
        _evas_cache_engine_image_make_inactive(cache, eim, eim->cache_key);
        evas_cache_engine_image_flush(cache);
        return;
     }
}

/**
 * @brief Marks a region of an engine image entry as dirty, potentially creating a new entry.
 *
 * This function is called when a part of an image needs to be updated.
 *
 * If the image entry @p eim has a parent (i.e., `eim->flags.need_parent == 1`):
 *   - It first calls `evas_cache_image_dirty()` on the parent image.
 *   - If `evas_cache_image_dirty()` returns a new parent image (`im != im_dirty`),
 *     and @p eim has only one reference, @p eim is modified to point to the new
 *     parent and marked as dirty.
 *   - If `evas_cache_image_dirty()` returns a new parent and @p eim has multiple
 *     references, a new `Engine_Image_Entry` (`eim_dirty`) is allocated,
 *     associated with the new parent, and the engine's `dirty` callback is invoked
 *     to copy/transform data from the original @p eim to `eim_dirty`. The original
 *     @p eim is then dropped.
 *
 * If the image entry @p eim does not have a parent:
 *   - If @p eim has more than one reference, a new `Engine_Image_Entry` (`eim_dirty`)
 *     is allocated, and the engine's `dirty` callback is invoked. The original
 *     @p eim is dropped.
 *   - If @p eim has only one reference, it is directly marked as dirty.
 *
 * Finally, the engine's `dirty_region` callback is called on the (potentially new)
 * `eim_dirty` to notify the engine about the specific rectangular area that changed.
 *
 * @param eim The engine image entry to mark as dirty.
 * @param x The x-coordinate of the top-left corner of the dirty region.
 * @param y The y-coordinate of the top-left corner of the dirty region.
 * @param w The width of the dirty region.
 * @param h The height of the dirty region.
 * @return A pointer to the (potentially new) dirty Engine_Image_Entry.
 *         This could be the original @p eim if no new entry was allocated,
 *         or a new entry if a copy-on-write was necessary. Returns NULL on error.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_dirty(Engine_Image_Entry *eim,
                              unsigned int x, unsigned int y,
                              unsigned int w, unsigned int h)
{
   Engine_Image_Entry *eim_dirty = eim;
   Image_Entry *im_dirty = NULL;
   Image_Entry *im;
   Evas_Cache_Engine_Image *cache;
   unsigned char alloc_eim;

   assert(eim);
   assert(eim->cache);

   cache = eim->cache;
   if (!(eim->flags.dirty))
     {
        alloc_eim = 0;

        if (eim->flags.need_parent == 1)
          {
             im = eim->src;
             im_dirty = evas_cache_image_dirty(im, x, y, w, h);

             /* If im == im_dirty, this meens that we have only one
              * reference to the eim. */
             if (im != im_dirty)
               {
                  if (eim->references == 1)
                    {
                       _evas_cache_engine_image_remove_activ(cache, eim);
                       _evas_cache_engine_image_make_dirty(cache, eim);
                       eim->src = im_dirty;
                    }
                  else alloc_eim = 1;
               }
          }
        else
          {
             if (eim->references > 1) alloc_eim = 1;
             else
               {
                  _evas_cache_engine_image_remove_activ(cache, eim_dirty);
                  _evas_cache_engine_image_make_dirty(cache, eim_dirty);
               }
          }

        if (alloc_eim == 1)
          {
             int           error;

             eim_dirty = _evas_cache_engine_image_alloc(cache, im_dirty, NULL);
             if (!eim_dirty) goto on_error;

             eim_dirty->w = eim->w;
             eim_dirty->h = eim->h;
             eim_dirty->references = 1;

             error = cache->func.dirty(eim_dirty, eim);
             if (cache->func.debug)
               cache->func.debug("dirty-engine", eim_dirty);

             if (error != 0) goto on_error;

             evas_cache_engine_image_drop(eim);
          }
     }

   if (cache->func.dirty_region)
     cache->func.dirty_region(eim_dirty, x, y, w, h);
   if (cache->func.debug)
     cache->func.debug("dirty-region-engine", eim_dirty);

   return eim_dirty;

on_error:
   evas_cache_engine_image_drop(eim);
   if (eim_dirty && eim_dirty != eim) evas_cache_engine_image_drop(eim_dirty);
   else if (im_dirty) evas_cache_image_drop(im_dirty);

   return NULL;
}

/**
 * @brief Ensures an engine image entry is "alone" (not shared), creating a copy if necessary.
 *
 * This function checks if the underlying parent `Image_Entry` of @p eim is shared.
 * If it is, `evas_cache_image_alone()` is called to get an unshared version of
 * the parent. If this results in a new parent `Image_Entry`, a new
 * `Engine_Image_Entry` is allocated, associated with the new parent, and the
 * engine's `constructor` callback is called to initialize it with @p data.
 *
 * The primary purpose is to ensure that modifications to this image entry
 * will not affect other users of the original shared data.
 *
 * @param eim The engine image entry to make "alone".
 * @param data Engine-specific data to be passed to the constructor if a new
 *             engine image entry is created.
 * @return A pointer to the "alone" Engine_Image_Entry. This might be the
 *         original @p eim if it was already alone, or a new entry if a
 *         copy was made. Returns NULL on error.
 * @note FIXME is mentioned in the code, suggesting potential issues or incompleteness.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_alone(Engine_Image_Entry *eim, void *data)
{
   Evas_Cache_Engine_Image *cache;
   Image_Entry *im;

   assert(eim);
   assert(eim->cache);

   cache = eim->cache;
   im = evas_cache_image_alone(eim->src);
   if (im != eim->src)
     {
        eim = _evas_cache_engine_image_alloc(cache, im, NULL);
        if (!eim) goto on_error;

        eim->references = 1;

        if (cache->func.constructor(eim, data) != EVAS_LOAD_ERROR_NONE)
          goto on_error;
     }
   /* FIXME */
   return eim;

on_error:
   evas_cache_image_drop(im);
   return NULL;
}

/**
 * @internal
 * @brief Creates a new, dirty engine image entry from a given parent image entry and engine data.
 *
 * This function allocates a new Engine_Image_Entry, associates it with the
 * provided parent Image_Entry (@p im), and then calls the engine's `update_data`
 * callback to populate the engine-specific parts of the image using @p engine_data.
 * The new entry is marked as dirty and has a reference count of 1.
 *
 * This is typically used when creating an engine image from raw pixel data
 * or other engine-specific sources that are already "dirty" by nature.
 *
 * @param cache The engine image cache.
 * @param im The parent Image_Entry (which usually holds the raw pixel data).
 * @param engine_data Engine-specific data to be used by the `update_data` callback.
 * @return A pointer to the newly created dirty Engine_Image_Entry, or NULL on failure.
 */
static Engine_Image_Entry *
_evas_cache_engine_image_push_dirty(Evas_Cache_Engine_Image *cache,
                                    Image_Entry *im, void *engine_data)
{
   Engine_Image_Entry *eim;
   int error;

   eim = _evas_cache_engine_image_alloc(cache, im, NULL);
   if (!eim) goto on_error;
   eim->references = 1;

   error = cache->func.update_data(eim, engine_data);
   if (cache->func.debug) cache->func.debug("dirty-update_data-engine", eim);
   if (error != 0) goto on_error;

   return eim;

on_error:
   if (eim) evas_cache_engine_image_drop(eim);
   return NULL;
}

/**
 * @brief Creates an engine image entry from copied pixel data.
 *
 * This function first creates a generic `Image_Entry` by copying the provided
 * pixel data (@p image_data). Then, it calls
 * `_evas_cache_engine_image_push_dirty()` to create an associated
 * `Engine_Image_Entry` and populate it using the engine's `update_data`
 * callback with @p engine_data. The resulting engine image entry is
 * marked as dirty.
 *
 * @param cache The engine image cache.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param image_data Pointer to the raw pixel data (e.g., an array of ARGB values).
 *                   This data will be copied.
 *                   Example: `DATA32 pixels[] = {0xffff0000, 0xff00ff00, ...};`
 * @param alpha Flag indicating if the image data has an alpha channel (1 for yes, 0 for no).
 * @param cspace The colorspace of the image data (e.g., EVAS_COLORSPACE_ARGB8888).
 * @param engine_data Engine-specific data to be passed to the `update_data` callback.
 * @return A pointer to the newly created Engine_Image_Entry, or NULL on failure.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_copied_data(Evas_Cache_Engine_Image *cache,
                                    unsigned int w, unsigned int h,
                                    DATA32 *image_data, int alpha,
                                    Evas_Colorspace cspace, void *engine_data)
{
   Image_Entry *im;

   assert(cache);
   im = evas_cache_image_copied_data(cache->parent, w, h, image_data,
                                     alpha, cspace);
   return _evas_cache_engine_image_push_dirty(cache, im, engine_data);
}

/**
 * @brief Creates an engine image entry from existing pixel data (without copying).
 *
 * This function first creates a generic `Image_Entry` that directly uses the
 * provided pixel data (@p image_data) without making a copy. The caller is
 * responsible for ensuring the lifetime of @p image_data. Then, it calls
 * `_evas_cache_engine_image_push_dirty()` to create an associated
 * `Engine_Image_Entry` and populate it using the engine's `update_data`
 * callback with @p engine_data. The resulting engine image entry is
 * marked as dirty.
 *
 * @param cache The engine image cache.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param image_data Pointer to the raw pixel data (e.g., an array of ARGB values).
 *                   This data will NOT be copied; the Image_Entry will point to it directly.
 *                   Example: `static DATA32 shared_pixels[] = {0xffff0000, ...};`
 * @param alpha Flag indicating if the image data has an alpha channel (1 for yes, 0 for no).
 * @param cspace The colorspace of the image data (e.g., EVAS_COLORSPACE_ARGB8888).
 * @param engine_data Engine-specific data to be passed to the `update_data` callback.
 * @return A pointer to the newly created Engine_Image_Entry, or NULL on failure.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_data(Evas_Cache_Engine_Image *cache,
                             unsigned int w, unsigned int h,
                             DATA32 *image_data, int alpha,
                             Evas_Colorspace cspace, void *engine_data)
{
   Image_Entry *im;

   assert(cache);
   im = evas_cache_image_data(cache->parent, w, h, image_data, alpha, cspace);
   return _evas_cache_engine_image_push_dirty(cache, im, engine_data);
}

/**
 * @brief Changes the size of an engine image entry, potentially creating a new entry.
 *
 * This function modifies the dimensions of an existing engine image entry (@p eim).
 *
 * If @p eim has a parent (`eim->flags.need_parent == 1`):
 *   - It first calls `evas_cache_image_size_set()` on the parent.
 *   - If the parent's size didn't change or if the parent itself was returned (no new parent created),
 *     and the new dimensions @p w, @p h match the current dimensions of @p eim's parent,
 *     the original @p eim is returned.
 *   - Otherwise, a new parent `Image_Entry` (`im`) is obtained.
 *
 * A new `Engine_Image_Entry` (`new`) is then allocated:
 *   - If @p eim has multiple references, its `cache_key` is duplicated for the new entry.
 *   - The new entry is associated with the (potentially new) parent `im`.
 *
 * The engine's `size_set` callback is invoked, passing both the `new` entry and the
 * original @p eim. This callback is responsible for adapting the engine-specific
 * data (e.g., reallocating surfaces, scaling content) from the old entry to the new one.
 *
 * Finally, the original @p eim is dropped, and the `new` entry is returned.
 *
 * @param eim The engine image entry whose size is to be changed.
 * @param w The new width for the image.
 * @param h The new height for the image.
 * @return A pointer to the (potentially new) Engine_Image_Entry with the updated size.
 *         This could be the original @p eim if no change was needed or no new entry
 *         was allocated, or a new entry if a resize and/or copy-on-write occurred.
 *         Returns NULL on error.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_size_set(Engine_Image_Entry *eim,
                                 unsigned int w, unsigned int h)
{
   Evas_Cache_Engine_Image *cache;
   Engine_Image_Entry *new;
   Image_Entry *im;
   const char *hkey;
   int error;

   assert(eim);
   assert(eim->cache);
   assert(eim->references > 0);

   im = NULL;
   cache = eim->cache;

   if (eim->flags.need_parent == 1)
     {
        assert(eim->src);

        if ((eim->src->w == w) && (eim->src->h == h)) return eim;

        im = evas_cache_image_size_set(eim->src, w, h);
        /* FIXME: Good idea to call update_data ? */
        if (im == eim->src) return eim;
        eim->src = NULL;
     }

   hkey = (eim->references > 1 ) ? eina_stringshare_add(eim->cache_key) : NULL;

   new = _evas_cache_engine_image_alloc(cache, im, hkey);
   if (!new) goto on_error;

   new->w = w;
   new->h = h;
   new->references = 1;

   error = cache->func.size_set(new, eim);
   if (error) goto on_error;

   evas_cache_engine_image_drop(eim);
   return new;

on_error:
   if (new) evas_cache_engine_image_drop(new);
   else if (im) evas_cache_image_drop(im);
   evas_cache_engine_image_drop(eim);

   return NULL;
}

/**
 * @brief Ensures that the image data for an engine image entry is loaded.
 *
 * This function checks if the image data for @p eim has already been loaded
 * (by checking `eim->flags.loaded`). If not:
 *   - It first ensures the parent `Image_Entry`'s data is loaded by calling
 *     `evas_cache_image_load_data(eim->src)`.
 *   - Then, it calls the engine's `load` callback, passing @p eim and its
 *     parent `eim->src`. This callback is responsible for preparing the
 *     engine-specific resources (e.g., uploading textures to GPU).
 *   - If the entry was dirty, its contribution to cache usage is updated.
 *   - The `eim->flags.loaded` flag is set to 1.
 *
 * @param eim The engine image entry for which to load data.
 */
EVAS_API void
evas_cache_engine_image_load_data(Engine_Image_Entry *eim)
{
   Evas_Cache_Engine_Image *cache;
   int size = 0;

   assert(eim);
   assert(eim->src);
   assert(eim->cache);

   if (eim->flags.loaded) return;

   if (eim->src) evas_cache_image_load_data(eim->src);

   cache = eim->cache;
   if (cache->func.debug) cache->func.debug("load-engine", eim);

   if (eim->flags.dirty) size = cache->func.mem_size_get(eim);
   cache = eim->cache;
   cache->func.load(eim, eim->src);
   if (eim->flags.dirty) cache->usage += cache->func.mem_size_get(eim) - size;

   eim->flags.loaded = 1;
}

/**
 * @brief Creates a new engine image entry specifically for engine-internal data.
 *
 * This function is used to create an `Engine_Image_Entry` that is not directly
 * based on a traditional image file or pixel buffer, but rather represents
 * some engine-specific resource or data.
 *
 * It first creates an "empty" parent `Image_Entry` using `evas_cache_image_empty()`.
 * Then, it allocates a new `Engine_Image_Entry` associated with this empty parent.
 * Finally, it calls the engine's `update_data` callback, passing the new
 * `Engine_Image_Entry` and @p engine_data. This callback is responsible for
 * initializing the engine-specific aspects of the entry.
 *
 * @param cache The engine image cache.
 * @param engine_data Engine-specific data to initialize the entry.
 * @return A pointer to the newly created Engine_Image_Entry, or NULL on failure.
 */
EVAS_API Engine_Image_Entry *
evas_cache_engine_image_engine(Evas_Cache_Engine_Image *cache,
                               void *engine_data)
{
   Engine_Image_Entry *eim;
   Image_Entry *ie;
   int error;

   ie = evas_cache_image_empty(cache->parent);
   if (!ie) return NULL;

   eim = _evas_cache_engine_image_alloc(cache, ie, NULL);
   if (!eim) goto on_error;
   eim->references = 1;

   error = cache->func.update_data(eim, engine_data);
   if (cache->func.debug) cache->func.debug("update_data-engine", eim);

   if (error != 0) goto on_error;

   return eim;

on_error:
   if (!eim) evas_cache_image_drop(ie);
   else evas_cache_engine_image_drop(eim);

   return NULL;
}

/**
 * @brief Changes the colorspace of an engine image entry.
 *
 * This function modifies the colorspace of an existing engine image entry (@p eim).
 * It involves:
 *   1. Calling the engine's `destructor` callback for @p eim to clean up
 *      any existing engine-specific resources associated with the old colorspace.
 *   2. Calling `evas_cache_image_colorspace()` on the parent `Image_Entry`
 *      (`eim->src`) to convert its underlying pixel data to the new @p cspace.
 *   3. Calling the engine's `constructor` callback for @p eim, passing
 *      @p engine_data. This allows the engine to re-initialize its resources
 *      based on the new colorspace and potentially new data from the parent.
 *
 * @param eim The engine image entry whose colorspace is to be changed.
 * @param cspace The new target Evas_Colorspace.
 *                 Example: `EVAS_COLORSPACE_YCBCR422P601_PL`
 * @param engine_data Engine-specific data to be passed to the `constructor` callback.
 */
EVAS_API void
evas_cache_engine_image_colorspace(Engine_Image_Entry *eim,
                                   Evas_Colorspace cspace, void *engine_data)
{
   Evas_Cache_Engine_Image *cache = eim->cache;

   assert(cache);

   cache->func.destructor(eim);
   evas_cache_image_colorspace(eim->src, cspace);
   cache->func.constructor(eim, engine_data);
   if (cache->func.debug)
     cache->func.debug("cosntructor-colorspace-engine", eim);
}

/**
 * @brief Informs the cache that the parent data for an engine image entry is no longer needed.
 *
 * This function signals that the original data held by the parent `Image_Entry`
 * (`eim->src`) is no longer required by the engine for this specific
 * `Engine_Image_Entry` (@p eim). This typically happens after the engine has
 * fully processed the parent data and stored it in its own format (e.g.,
 * uploaded to a GPU texture).
 *
 * It sets the `eim->flags.need_parent` flag to 0 and calls
 * `evas_cache_image_data_not_needed()` on the parent `Image_Entry`. This
 * allows the parent cache to potentially free the pixel data if it's no
 * longer referenced by any other engine entries or users.
 *
 * @param eim The engine image entry whose parent data is no longer needed.
 */
EVAS_API void
evas_cache_engine_parent_not_needed(Engine_Image_Entry *eim)
{
   assert(eim);
   assert(eim->cache);

   eim->flags.need_parent = 0;
   evas_cache_image_data_not_needed(eim->src);
}
