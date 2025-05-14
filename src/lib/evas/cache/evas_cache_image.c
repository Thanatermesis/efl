/**
 * @file
 * @brief Evas image cache internal implementation.
 *
 * This file contains the core logic for managing Evas image caches,
 * including image loading, unloading, LRU (Least Recently Used) eviction,
 * and preloading mechanisms.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "evas_common_private.h"
#include "evas_private.h"

//#define CACHEDUMP 1

typedef struct _Evas_Cache_Preload Evas_Cache_Preload; /**< Forward declaration for Evas_Cache_Preload */

/**
 * @struct _Evas_Cache_Preload
 * @brief Structure to manage image preloading requests.
 *
 * This structure holds an image entry that is currently being preloaded
 * or is scheduled for preloading. It's part of an EINA_INLIST to
 * manage a list of such entries.
 */
struct _Evas_Cache_Preload
{
   EINA_INLIST;      /**< Macro to make this struct usable with Eina_Inlist */
   Image_Entry *ie;  /**< Pointer to the image entry being preloaded */
};

static SLK(engine_lock);
static int _evas_cache_mutex_init = 0; /**< Counter for initializing the engine lock mutex. */

/**
 * @brief Removes an image entry from the preload list.
 * @param ie The image entry to remove from preloading.
 * @param target The specific Evas object target whose preload request is being removed. Can be NULL to remove all targets.
 * @param force If EINA_TRUE, forcefully cancels the preload operation.
 */
static void _evas_cache_image_entry_preload_remove(Image_Entry *ie, const Eo *target, Eina_Bool force);

#define FREESTRC(Var)             \
   if (Var)                       \
{                              \
   eina_stringshare_del(Var);  \
   Var = NULL;                 \
}

/**
 * @brief Adds an image entry to the dirty list.
 * Dirty images are typically those that have been modified or are
 * marked for deletion due to inconsistencies (e.g., file changed on disk).
 * @param im The image entry to add to the dirty list.
 */
static void _evas_cache_image_dirty_add(Image_Entry *im);

/**
 * @brief Removes an image entry from the dirty list.
 * @param im The image entry to remove from the dirty list.
 */
static void _evas_cache_image_dirty_del(Image_Entry *im);

/**
 * @brief Adds an image entry to the active list.
 * Active images are those currently in use or recently accessed.
 * @param im The image entry to add to the active list.
 */
static void _evas_cache_image_activ_add(Image_Entry *im);

/**
 * @brief Removes an image entry from the active list.
 * @param im The image entry to remove from the active list.
 */
static void _evas_cache_image_activ_del(Image_Entry *im);

/**
 * @brief Adds an image entry to the LRU (Least Recently Used) list.
 * Images in this list are candidates for eviction when cache limits are reached.
 * These images have their pixel data loaded.
 * @param im The image entry to add to the LRU list.
 */
static void _evas_cache_image_lru_add(Image_Entry *im);

/**
 * @brief Removes an image entry from the LRU list.
 * @param im The image entry to remove from the LRU list.
 */
static void _evas_cache_image_lru_del(Image_Entry *im);

/**
 * @brief Adds an image entry to the LRU "no data" list.
 * These are images whose metadata is cached, but pixel data is not currently loaded.
 * They are also candidates for eviction.
 * @param im The image entry to add to the LRU "no data" list.
 */
static void _evas_cache_image_lru_nodata_add(Image_Entry *im);

/**
 * @brief Removes an image entry from the LRU "no data" list.
 * @param im The image entry to remove from the LRU "no data" list.
 */
static void _evas_cache_image_lru_nodata_del(Image_Entry *im);

static void
_evas_cache_image_dirty_add(Image_Entry *im)
{
   if (im->flags.dirty) return;
   if (!im->cache) return;
   _evas_cache_image_activ_del(im);
   _evas_cache_image_lru_del(im);
   _evas_cache_image_lru_nodata_del(im);
   im->flags.dirty = 1;
   im->flags.cached = 1;
   im->cache->dirty = eina_inlist_prepend(im->cache->dirty, EINA_INLIST_GET(im));
   if (im->cache_key)
     {
        eina_stringshare_del(im->cache_key);
        im->cache_key = NULL;
     }
}

static void
_evas_cache_image_dirty_del(Image_Entry *im)
{
   if (!im->flags.dirty) return;
   if (!im->cache) return;
   im->flags.dirty = 0;
   im->flags.cached = 0;
   im->cache->dirty = eina_inlist_remove(im->cache->dirty, EINA_INLIST_GET(im));
}

static void
_evas_cache_image_activ_add(Image_Entry *im)
{
   if (im->flags.activ) return;
   if (!im->cache) return;
   _evas_cache_image_dirty_del(im);
   _evas_cache_image_lru_del(im);
   _evas_cache_image_lru_nodata_del(im);
   if (!im->cache_key) return;
   im->flags.activ = 1;
   im->flags.cached = 1;
   if (im->flags.given_mmap)
     eina_hash_direct_add(im->cache->mmap_activ, im->cache_key, im);
   else
     eina_hash_direct_add(im->cache->activ, im->cache_key, im);
}

static void
_evas_cache_image_activ_del(Image_Entry *im)
{
   if (!im->flags.activ) return;
   if (!im->cache_key) return;
   if (!im->cache) return;
   im->flags.activ = 0;
   im->flags.cached = 0;
   if (im->flags.given_mmap)
     eina_hash_del(im->cache->mmap_activ, im->cache_key, im);
   else
     eina_hash_del(im->cache->activ, im->cache_key, im);
}

static void
_evas_cache_image_lru_add(Image_Entry *im)
{
   if (im->flags.lru) return;
   if (!im->cache) return;
   _evas_cache_image_dirty_del(im);
   _evas_cache_image_activ_del(im);
   _evas_cache_image_lru_nodata_del(im);
   if (!im->cache_key) return;
   im->flags.lru = 1;
   im->flags.cached = 1;
   if (im->flags.given_mmap)
     eina_hash_direct_add(im->cache->mmap_inactiv, im->cache_key, im);
   else
     eina_hash_direct_add(im->cache->inactiv, im->cache_key, im);
   im->cache->lru = eina_inlist_prepend(im->cache->lru, EINA_INLIST_GET(im));
   im->cache->usage += im->cache->func.mem_size_get(im);
}

static void
_evas_cache_image_lru_del(Image_Entry *im)
{
   if (!im->flags.lru) return;
   if (!im->cache_key) return;
   if (!im->cache) return;
   im->flags.lru = 0;
   im->flags.cached = 0;
   if (im->flags.given_mmap)
     eina_hash_del(im->cache->mmap_inactiv, im->cache_key, im);
   else
     eina_hash_del(im->cache->inactiv, im->cache_key, im);
   im->cache->lru = eina_inlist_remove(im->cache->lru, EINA_INLIST_GET(im));
   im->cache->usage -= im->cache->func.mem_size_get(im);
}

static void
_evas_cache_image_lru_nodata_add(Image_Entry *im)
{
   if (im->flags.lru_nodata) return;
   if (!im->cache) return;
   _evas_cache_image_dirty_del(im);
   _evas_cache_image_activ_del(im);
   _evas_cache_image_lru_del(im);
   im->flags.lru = 1;
   im->flags.cached = 1;
   im->cache->lru_nodata = eina_inlist_prepend(im->cache->lru_nodata, EINA_INLIST_GET(im));
}

static void
_evas_cache_image_lru_nodata_del(Image_Entry *im)
{
   if (!im->flags.lru_nodata) return;
   if (!im->cache) return;
   im->flags.lru = 0;
   im->flags.cached = 0;
   im->cache->lru_nodata = eina_inlist_remove(im->cache->lru_nodata, EINA_INLIST_GET(im));
}

/**
 * @brief Deletes an image entry from the cache.
 * This function handles the complete removal of an image entry, including
 * freeing associated resources, removing it from various cache lists,
 * and calling appropriate destructors.
 * @param cache The image cache instance.
 * @param ie The image entry to delete.
 */
static void
_evas_cache_image_entry_delete(Evas_Cache_Image *cache, Image_Entry *ie)
{
   if (!ie) return;
   if (!ie->cache) return;
   if ((cache) && (cache->func.debug)) cache->func.debug("deleting", ie);
   if (ie->flags.delete_me == 1) return;
   if (ie->preload)
     {
        ie->flags.delete_me = 1;
        _evas_cache_image_entry_preload_remove(ie, NULL, EINA_TRUE);
        return;
     }
   _evas_cache_image_dirty_del(ie);
   _evas_cache_image_activ_del(ie);
   _evas_cache_image_lru_del(ie);
   _evas_cache_image_lru_nodata_del(ie);

   if ((cache) && (cache->func.destructor)) cache->func.destructor(ie);
   FREESTRC(ie->cache_key);
   FREESTRC(ie->file);
   FREESTRC(ie->key);
   if (ie->f && ie->flags.given_mmap)
     {
        eina_file_close(ie->f); // close matching open (in _evas_cache_image_entry_new) OK
        ie->f = NULL;
     }
   ie->cache = NULL;
   if ((cache) && (cache->func.surface_delete)) cache->func.surface_delete(ie);

   SLKD(ie->lock);
   SLKD(ie->lock_cancel);
   if ((cache) && (cache->func.dealloc)) cache->func.dealloc(ie);
}

/**
 * @brief Compares an Image_Timestamp with stat information.
 * This is used to check if a cached image file has been modified on disk.
 * @param tstamp Pointer to the Image_Timestamp structure of the cached image.
 * @param st Pointer to the struct stat containing file system information of the image file.
 * @return EINA_TRUE if timestamps and other relevant stat info match, EINA_FALSE otherwise.
 */
static Eina_Bool
_timestamp_compare(Image_Timestamp *tstamp, struct stat *st)
{
   if (tstamp->mtime != st->st_mtime) return EINA_FALSE;
   if (tstamp->size != st->st_size) return EINA_FALSE;
   if (tstamp->ino != st->st_ino) return EINA_FALSE;
#ifdef _STAT_VER_LINUX
   if (tstamp->mtime_nsec != (unsigned long int)st->st_mtim.tv_nsec)
     return EINA_FALSE;
#endif
   return EINA_TRUE;
}

/**
 * @brief Populates an Image_Timestamp structure from stat information.
 * @param tstamp Pointer to the Image_Timestamp structure to populate.
 * @param st Pointer to the struct stat containing file system information.
 */
static void
_timestamp_build(Image_Timestamp *tstamp, struct stat *st)
{
   tstamp->mtime = st->st_mtime;
   tstamp->size = st->st_size;
   tstamp->ino = st->st_ino;
#ifdef _STAT_VER_LINUX
   tstamp->mtime_nsec = (unsigned long int)st->st_mtim.tv_nsec;
#endif
}

/**
 * @brief Creates a new image cache entry.
 * This function allocates and initializes a new Image_Entry structure.
 * It sets up basic properties, file/key information, and load options.
 * The new entry is added to the active or dirty list depending on whether
 * a cache key is provided.
 *
 * @param cache The image cache instance.
 * @param hkey The hash key for the image entry. If NULL, the entry is considered "dirty" initially.
 * @param tstamp The timestamp information for the image file. Can be NULL.
 * @param f An Eina_File handle if the image is from an mmap'd source. Can be NULL.
 * @param file The file path of the image. Can be NULL if @p f is provided.
 * @param key An optional sub-key for the image within the file (e.g., for image collections). Can be NULL.
 * @param lo Pointer to the image loading options. Can be NULL for default options.
 * @param error Pointer to an integer to store the error code if creation fails.
 * @return A pointer to the newly created Image_Entry, or NULL on failure.
 *         Possible error codes in @p error:
 *         - EVAS_LOAD_ERROR_GENERIC
 *         - EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED
 *         - Errors from cache->func.constructor
 */
static Image_Entry *
_evas_cache_image_entry_new(Evas_Cache_Image *cache,
                            const char *hkey,
                            Image_Timestamp *tstamp,
                            Eina_File *f,
                            const char *file,
                            const char *key,
                            Evas_Image_Load_Opts *lo,
                            int *error)
{
   Image_Entry  *ie;

   if (!cache)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return NULL;
     }

   ie = cache->func.alloc();
   if (!ie)
     {
        *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
        return NULL;
     }
   ie->cache = cache;
   if (hkey) ie->cache_key = eina_stringshare_add(hkey);
   ie->flags.need_data = 1;
   ie->space = EVAS_COLORSPACE_ARGB8888;
   ie->w = -1;
   ie->h = -1;
   ie->scale = 1;
   ie->f = eina_file_dup(f);
   ie->loader_data = NULL;
   if (ie->f) ie->flags.given_mmap = EINA_TRUE;
   if (file) ie->file = eina_stringshare_add(file);
   if (key) ie->key = eina_stringshare_add(key);
   if (tstamp) ie->tstamp = *tstamp;
   else memset(&ie->tstamp, 0, sizeof(Image_Timestamp));

   SLKI(ie->lock);
   SLKI(ie->lock_cancel);

   if (lo)
     {
        ie->load_opts = *lo;
     }
   if (ie->file || ie->f)
     {
        *error = cache->func.constructor(ie);
        if (*error != EVAS_LOAD_ERROR_NONE)
          {
             _evas_cache_image_entry_delete(cache, ie);
             return NULL;
          }
     }
   if (cache->func.debug) cache->func.debug("build", ie);
   if (ie->cache_key) _evas_cache_image_activ_add(ie);
   else _evas_cache_image_dirty_add(ie);
   return ie;
}

/**
 * @brief Allocates surface memory for an image entry (internal, lock-held version).
 * This function is called with the engine_lock held. It ensures that
 * surface memory is allocated or reallocated for the image entry if the
 * requested dimensions differ from the currently allocated ones.
 *
 * @param cache The image cache instance.
 * @param ie The image entry for which to allocate surface memory.
 * @param wmin The minimum width required for the surface.
 * @param hmin The minimum height required for the surface.
 */
static void
_evas_cache_image_entry_surface_alloc__locked(Evas_Cache_Image *cache,
                                              Image_Entry *ie,
                                              unsigned int wmin,
                                              unsigned int hmin)
{
   if ((ie->allocated.w == wmin) && (ie->allocated.h == hmin)) return;
   if ((cache->func.surface_alloc(ie, wmin, hmin)) || (ie->load_failed))
     {
        wmin = 0;
        hmin = 0;
     }
   ie->w = wmin;
   ie->h = hmin;
}

/**
 * @brief Allocates surface memory for an image entry.
 * This function acquires the engine_lock and then calls the internal
 * _evas_cache_image_entry_surface_alloc__locked function.
 * It ensures that width and height are at least 1.
 *
 * @param cache The image cache instance.
 * @param ie The image entry for which to allocate surface memory.
 * @param w The desired width for the surface.
 * @param h The desired height for the surface.
 */
static void
_evas_cache_image_entry_surface_alloc(Evas_Cache_Image *cache,
                                      Image_Entry *ie, int w, int h)
{
   int wmin = w > 0 ? w : 1;
   int hmin = h > 0 ? h : 1;
   SLKL(engine_lock);
   _evas_cache_image_entry_surface_alloc__locked(cache, ie, wmin, hmin);
   SLKU(engine_lock);
}

/**
 * @brief Checks if an image loading operation has been cancelled.
 * This function is typically used as a callback for asynchronous tasks
 * to determine if they should abort.
 * @param data A pointer to the Image_Entry being loaded.
 * @return EINA_TRUE if the preload operation for the image is cancelled, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_cache_image_cancelled(void *data)
{
   Image_Entry *current = data;
   Eina_Bool ret;

   evas_cache_image_ref(current);
   ret = evas_preload_thread_cancelled_is(current->preload);
   evas_cache_image_drop(current);
   return ret;
}

/**
 * @brief Performs the heavy lifting of asynchronous image loading.
 * This function is executed in a separate thread to load image data
 * without blocking the main thread. It handles the actual loading
 * via the cache's load function and manages cancellation flags.
 * @param data A pointer to the Image_Entry to be loaded.
 */
static void
_evas_cache_image_async_heavy(void *data)
{
   Evas_Cache_Image *cache;
   Image_Entry *current;
   int error;
   int pchannel;

   eina_thread_name_set(eina_thread_self(), "Evas-preload");

   current = data;

   if (!current->cache) return;
   SLKL(current->lock);
   pchannel = current->channel;
   current->channel++;
   cache = current->cache;

   if ((!current->flags.loaded) &&
       (current->info.loader) &&
       (current->info.loader->threadable))
     {
        evas_module_task_register(evas_cache_image_cancelled, current);
        error = cache->func.load(current);
        evas_module_task_unregister();

        if (cache->func.debug) cache->func.debug("load", current);
        current->load_error = error;
        if (error != EVAS_LOAD_ERROR_NONE)
          {
             current->flags.loaded = 0;
             _evas_cache_image_entry_surface_alloc(cache, current,
                                                   current->w, current->h);
          }
     }
   current->channel = pchannel;
   // check the unload cancel flag
   SLKL(current->lock_cancel);
   if (current->flags.unload_cancel)
     {
        current->flags.unload_cancel = EINA_FALSE;
        cache->func.surface_delete(current);
        current->flags.loaded = 0;
        current->flags.preload_done = 0;
     }
   SLKU(current->lock_cancel);
   SLKU(current->lock);
}

/**
 * @brief Notifies all targets that an image has been preloaded.
 * Iterates through the list of targets associated with an image entry
 * and invokes their respective preloaded callbacks and informs Evas objects.
 * @param ie The image entry that has finished preloading.
 */
static void
_evas_cache_image_preloaded_notify(Image_Entry *ie)
{
   Evas_Cache_Target *tmp;

   while ((tmp = ie->targets))
     {
        ie->targets = (Evas_Cache_Target *)
          eina_inlist_remove(EINA_INLIST_GET(ie->targets),
                             EINA_INLIST_GET(ie->targets));
        if (ie->load_opts.skip_head && !tmp->delete_me && !tmp->preload_cancel)
          _evas_image_preload_update((void*)tmp->target, ie->f);
        if (!tmp->delete_me && tmp->preloaded_cb)
          tmp->preloaded_cb(tmp->preloaded_data);
        if (!tmp->preload_cancel)
          evas_object_inform_call_image_preloaded((Eo*) tmp->target);
        free(tmp);
     }
}

/**
 * @brief Handles the completion of an asynchronous image load.
 * This function is called when the asynchronous loading (heavy part)
 * is finished. It updates the image entry's state, removes it from
 * preload/pending lists, and notifies targets.
 * @param data A pointer to the Image_Entry that has been loaded.
 */
static void
_evas_cache_image_async_end(void *data)
{
   Image_Entry *ie = (Image_Entry *)data;

   if (!ie->cache) return;
   evas_cache_image_ref(ie);
   ie->cache->preload = eina_list_remove(ie->cache->preload, ie);
   ie->cache->pending = eina_list_remove(ie->cache->pending, ie);
   ie->preload = NULL;
   ie->flags.preload_done = ie->flags.loaded;
   ie->flags.updated_data = 1;
   ie->flags.preload_pending = 0;
   ie->flags.loaded = EINA_TRUE;

   _evas_cache_image_preloaded_notify(ie);
   evas_cache_image_drop(ie);
}

/**
 * @brief Handles the cancellation of an asynchronous image load.
 * This function is called when an asynchronous loading operation
 * is cancelled. It updates the image entry's state, removes it from
 * pending lists, and potentially moves it to the LRU list or
 * notifies targets if the image was already partially loaded.
 * @param data A pointer to the Image_Entry whose loading is cancelled.
 */
static void
_evas_cache_image_async_cancel(void *data)
{
   Evas_Cache_Image *cache = NULL;
   Image_Entry *ie = (Image_Entry *)data;

   if (!ie->cache) return;
   evas_cache_image_ref(ie);
   ie->preload = NULL;
   ie->cache->pending = eina_list_remove(ie->cache->pending, ie);

   ie->flags.preload_pending = 0;

   if ((ie->flags.delete_me) || (ie->flags.dirty))
     {
        SLKL(engine_lock);
        ie->flags.delete_me = 0;
        SLKU(engine_lock);
        evas_cache_image_drop(ie);
        return;
     }
   if (ie->references == 0)
     {
        SLKL(engine_lock);
        _evas_cache_image_lru_add(ie);
        SLKU(engine_lock);
        cache = ie->cache;
     }
   if (ie->flags.loaded) _evas_cache_image_async_end(ie);
   //On Cancelling, they need to draw image directly.
   else
     {
        while (ie->targets)
          {
             Evas_Cache_Target *tg = ie->targets;
             ie->targets = (Evas_Cache_Target *)
                eina_inlist_remove(EINA_INLIST_GET(ie->targets),
                                   EINA_INLIST_GET(tg));
             //FIXME: What/When they properly get a notification? Call in advance for compatibility.
             evas_object_inform_call_image_preloaded((Eo*) tg->target);
             free(tg);
          }
     }

   evas_cache_image_drop(ie);
   if (cache) evas_cache_image_flush(cache);
}

/**
 * @brief Adds a target to an image entry for preloading.
 * If the image is not already being preloaded, this function initiates
 * the asynchronous preloading process.
 * @note A target should only be added once to an image entry.
 *       If re-adding, ensure it's removed first.
 *
 * @param ie The image entry to preload.
 * @param target The Evas object (Eo) that requests the preload.
 * @param preloaded_cb Optional callback function to be invoked when preloading is complete for this target.
 * @param preloaded_data Optional data to be passed to @p preloaded_cb.
 * @return 1 if the target was successfully added for preloading (or preloading initiated),
 *         0 if preloading is already done or an error occurred (e.g., memory allocation failed).
 */
static int
_evas_cache_image_entry_preload_add(Image_Entry *ie, const Eo *target, void (*preloaded_cb)(void *), void *preloaded_data)
{
   Evas_Cache_Target *tg;

   if (!ie->cache) return 0;
   evas_cache_image_ref(ie);
   if (ie->flags.preload_done)
     {
        evas_cache_image_drop(ie);
        return 0;
     }

   tg = calloc(1, sizeof(Evas_Cache_Target));
   if (!tg)
     {
        evas_cache_image_drop(ie);
        return 0;
     }
   tg->target = target;
   tg->preloaded_cb = preloaded_cb;
   tg->preloaded_data = preloaded_data;

   ie->targets = (Evas_Cache_Target *)
      eina_inlist_append(EINA_INLIST_GET(ie->targets), EINA_INLIST_GET(tg));

   if (!ie->preload)
     {
        ie->cache->preload = eina_list_append(ie->cache->preload, ie);
        ie->flags.pending = 0;
        ie->flags.preload_pending = 1;
        ie->preload = evas_preload_thread_run(_evas_cache_image_async_heavy,
                                              _evas_cache_image_async_end,
                                              _evas_cache_image_async_cancel,
                                              ie);
     }
   evas_cache_image_drop(ie);
   return 1;
}

/**
 * @brief Removes a preload request for an image entry, optionally for a specific target.
 * If @p force is true, or if removing the target results in no more targets for the image,
 * the asynchronous preload operation is cancelled.
 *
 * @param ie The image entry.
 * @param target The specific Evas object (Eo) target whose preload request is to be removed.
 *               If NULL, all targets are effectively cleared (though the loop structure implies
 *               it's more about finding a specific NULL target, which is unlikely, or handling
 *               a general cleanup if `force` is true and `target` is NULL).
 *               The primary use seems to be with a non-NULL target to mark its specific
 *               preload_cancel flag, or with NULL and `force` to cancel everything.
 * @param force If EINA_TRUE, the preload operation is forcefully cancelled if no targets remain
 *              or if this flag itself dictates a forceful stop. This can affect other targets
 *              if one object cancels and decides to draw the image directly.
 */
static void
_evas_cache_image_entry_preload_remove(Image_Entry *ie, const Eo *target, Eina_Bool force)
{
   Evas_Cache_Target *tg;

   if (!ie->cache) return;
//   evas_cache_image_ref(ie);
   if (target)
     {
        EINA_INLIST_FOREACH(ie->targets, tg)
          {
             if (tg->target == target)
               {
                  tg->preload_cancel = EINA_TRUE;
                  break;
               }
          }
     }
   else
     {
        while (ie->targets)
          {
             tg = ie->targets;
             ie->targets = (Evas_Cache_Target *)
                eina_inlist_remove(EINA_INLIST_GET(ie->targets),
                                   EINA_INLIST_GET(tg));
             free(tg);
          }
     }

   if ((!ie->targets || force) && (ie->preload && !ie->flags.pending))
     {
        ie->cache->preload = eina_list_remove(ie->cache->preload, ie);
        ie->cache->pending = eina_list_append(ie->cache->pending, ie);
        ie->flags.pending = 1;
        evas_preload_thread_cancel(ie->preload);
     }
//   evas_cache_image_drop(ie);
}

/**
 * @brief Gets the current memory usage of the image cache.
 * @param cache The image cache instance.
 * @return The total memory currently used by images in the cache, in bytes.
 *         Returns 0 if @p cache is NULL.
 */
EVAS_API int
evas_cache_image_usage_get(Evas_Cache_Image *cache)
{
   if (!cache) return 0;
   return cache->usage;
}

/**
 * @brief Gets the memory limit of the image cache.
 * @param cache The image cache instance.
 * @return The maximum memory limit for the cache, in bytes.
 *         Returns 0 if @p cache is NULL (though the function returns cache->limit,
 *         so if cache is NULL, it would dereference NULL. The check prevents this).
 */
EVAS_API int
evas_cache_image_get(Evas_Cache_Image *cache)
{
   if (!cache) return 0;
   return cache->limit;
}

/**
 * @brief Sets the memory limit for the image cache.
 * If the new limit is different from the current one, the cache is flushed
 * to try to meet the new limit by evicting images if necessary.
 * @param cache The image cache instance.
 * @param limit The new maximum memory limit for the cache, in bytes.
 */
EVAS_API void
evas_cache_image_set(Evas_Cache_Image *cache, unsigned int limit)
{
   if (!cache) return;
   if (cache->limit == limit)
     {
        return;
     }
   cache->limit = limit;
   evas_cache_image_flush(cache);
}

/**
 * @brief Initializes a new image cache.
 * Sets up the cache structure, including hash tables for active and inactive
 * images, and initializes the engine lock if it's the first cache being created.
 * @param cb A pointer to an Evas_Cache_Image_Func structure containing callback
 *           functions for cache operations (allocation, deallocation, loading, etc.).
 * @return A pointer to the newly initialized Evas_Cache_Image instance, or NULL on failure.
 */
EVAS_API Evas_Cache_Image *
evas_cache_image_init(const Evas_Cache_Image_Func *cb)
{
   Evas_Cache_Image *cache;

   if (_evas_cache_mutex_init++ == 0)
     {
        SLKI(engine_lock);
     }

   cache = calloc(1, sizeof(Evas_Cache_Image));
   if (!cache) return NULL;
   cache->func = *cb;
   cache->inactiv = eina_hash_string_superfast_new(NULL);
   cache->activ = eina_hash_string_superfast_new(NULL);
   cache->mmap_activ = eina_hash_string_superfast_new(NULL);
   cache->mmap_inactiv = eina_hash_string_superfast_new(NULL);
   cache->references = 1;
   return cache;
}

/**
 * @brief Callback function used with eina_hash_foreach to collect image entries for deletion.
 * This function is used during cache shutdown to gather all active image entries.
 * @param hash The hash table being iterated. (Unused)
 * @param key The key of the hash entry. (Unused)
 * @param data The data (Image_Entry pointer) associated with the hash entry.
 * @param fdata A pointer to an Eina_List pointer (Eina_List **), where the image entry will be prepended.
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_evas_cache_image_free_cb(EINA_UNUSED const Eina_Hash *hash, EINA_UNUSED const void *key, void *data, void *fdata)
{
   Eina_List **delete_list = fdata;
   *delete_list = eina_list_prepend(*delete_list, data);
   return EINA_TRUE;
}

/**
 * @brief Shuts down an image cache and frees all associated resources.
 * This function handles the graceful shutdown of the cache. It cancels ongoing
 * preloads, deletes all image entries (LRU, dirty, active), waits for pending
 * operations to complete, and frees cache-internal structures.
 * The engine lock is deinitialized if this is the last cache being shut down.
 * @param cache The image cache instance to shut down.
 */
EVAS_API void
evas_cache_image_shutdown(Evas_Cache_Image *cache)
{
   Eina_List *delete_list;
   Image_Entry  *im;

   cache->references--;
   if (cache->references != 0)
     {
        return;
     }

   EINA_LIST_FREE(cache->preload, im)
     {
        /* By doing that we are protecting us from destroying image when the cache is no longer available. */
        im->flags.delete_me = 1;
        _evas_cache_image_entry_preload_remove(im, NULL, EINA_TRUE);
     }
   evas_async_events_process();

   SLKL(engine_lock);
   EINA_INLIST_FREE(cache->lru, im)
     _evas_cache_image_entry_delete(cache, im);
   EINA_INLIST_FREE(cache->lru_nodata, im)
     _evas_cache_image_entry_delete(cache, im);

   /* This is mad, I am about to destroy image still alive, but we need to prevent leak. */
   while (cache->dirty)
     {
        im = (Image_Entry *)cache->dirty;
        _evas_cache_image_entry_delete(cache, im);
     }
   delete_list = NULL;

   eina_hash_foreach(cache->activ, _evas_cache_image_free_cb, &delete_list);
   eina_hash_foreach(cache->mmap_activ, _evas_cache_image_free_cb, &delete_list);
   while (delete_list)
     {
        _evas_cache_image_entry_delete(cache, eina_list_data_get(delete_list));
        delete_list = eina_list_remove_list(delete_list, delete_list);
     }
   SLKU(engine_lock);

   /* Now wait for all pending image to die */
   while (cache->pending)
     {
        im = eina_list_data_get(cache->pending);
        evas_preload_thread_cancel(im->preload);

        evas_async_events_process();
        if (!evas_preload_pthread_wait(im->preload, 1.0))
          {
             // We have waited long enough without reaction from that said
             // thread, remove it from pending list and silently continue
             // in the hope of an ok shutdown (but something is wrong).
             cache->pending = eina_list_remove_list(cache->pending, cache->pending);
             ERR("Could not stop decoding '%s' during shutdown.\n", im->file);
          }
     }

   eina_hash_free(cache->activ);
   eina_hash_free(cache->inactiv);
   eina_hash_free(cache->mmap_activ);
   eina_hash_free(cache->mmap_inactiv);
   free(cache);

   if (--_evas_cache_mutex_init == 0)
     {
        SLKD(engine_lock);
     }
}

static const Evas_Image_Load_Opts prevent = {
   {
        { 0, 0, 0, 0 },
        {
           0, 0, 0, 0,
           0, 0,
           0,
           0
        },
      0.0,
      0, 0,
      0,
      0,

      EINA_FALSE
   },
   EINA_FALSE
};

/**
 * @brief Appends image loading options to a hash key string.
 * This function serializes the Evas_Image_Load_Opts structure into a string format
 * suitable for use as part of a cache key. This ensures that images loaded with
 * different options (e.g., scaling, DPI, region) are cached separately.
 * If the provided load options are all default/zero, a default "prevent"
 * set of options is used.
 *
 * @param hkey The character buffer where the serialized load options will be appended.
 *             It must be large enough to hold the appended string.
 * @param plo A pointer to a pointer to Evas_Image_Load_Opts.
 *            If the options are default, *plo might be updated to point to a static 'prevent' options set.
 * @return The number of bytes written to @p hkey (excluding the null terminator).
 *
 * Example of hkey format after appending options:
 * `original_key_part//@/<scale_down_by>/<dpi>/<w>x<h>/<region_x>+<region_y>.<region_w>x<region_h>[/o]`
 * - `//@/`: Separator
 * - `<scale_down_by>`: Integer scale down factor.
 * - `<dpi>`: Double DPI value.
 * - `<w>x<h>`: Target load width and height.
 * - `<region_x>+<region_y>.<region_w>x<region_h>`: Load region.
 * - `[/o]`: Optional, present if orientation is specified.
 */
static size_t
_evas_cache_image_loadopts_append(char *hkey, Evas_Image_Load_Opts **plo)
{
   Evas_Image_Load_Opts *lo = *plo;
   size_t offset = 0;

   if ((!lo) ||
       (lo &&
           (lo->emile.scale_down_by == 0) &&
           (EINA_DBL_EQ(lo->emile.dpi, 0.0)) &&
           ((lo->emile.w == 0) || (lo->emile.h == 0)) &&
           ((lo->emile.region.w == 0) || (lo->emile.region.h == 0)) &&
           (lo->emile.orientation == 0)
       ))
     {
        *plo = (Evas_Image_Load_Opts*) &prevent;
     }
   else
     {
        memcpy(hkey, "//@/", 4);
        offset += 4;
        offset += eina_convert_xtoa(lo->emile.scale_down_by, hkey + offset);
        hkey[offset] = '/';
        offset += 1;
        offset += eina_convert_dtoa(lo->emile.dpi, hkey + offset);
        hkey[offset] = '/';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.w, hkey + offset);
        hkey[offset] = 'x';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.h, hkey + offset);
        hkey[offset] = '/';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.region.x, hkey + offset);
        hkey[offset] = '+';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.region.y, hkey + offset);
        hkey[offset] = '.';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.region.w, hkey + offset);
        hkey[offset] = 'x';
        offset += 1;
        offset += eina_convert_xtoa(lo->emile.region.h, hkey + offset);

        if (lo->emile.orientation)
          {
             hkey[offset] = '/';
             offset += 1;
             hkey[offset] = 'o';
             offset += 1;
          }
     }
   hkey[offset] = '\0';

   return offset;
}

/**
 * @brief Requests an image entry from the cache for a memory-mapped file.
 * This function attempts to find an existing cache entry for the given mmap'd file,
 * key, and load options. If not found, a new entry is created.
 * The cache key is generated based on the Eina_File pointer, the string key, and load options.
 *
 * @param cache The image cache instance.
 * @param f The Eina_File handle for the memory-mapped image. Must not be NULL.
 * @param key An optional sub-key for the image within the file. Can be NULL.
 * @param lo Pointer to the image loading options.
 * @param error Pointer to an integer to store the error code on failure.
 * @return A pointer to the Image_Entry. The reference count of the entry is incremented.
 *         Returns NULL on error, with @p error set to an EVAS_LOAD_ERROR_* code.
 *         Possible error codes:
 *         - EVAS_LOAD_ERROR_GENERIC (if @p f is NULL)
 *         - Errors from _evas_cache_image_entry_new()
 */
EVAS_API Image_Entry *
evas_cache_image_mmap_request(Evas_Cache_Image *cache,
                              Eina_File *f, const char *key,
                              Evas_Image_Load_Opts *lo, int *error)
{
   const char  *hexcode = "0123456789abcdef";
   const char  *ckey = "(null)";
   char        *hkey;
   char        *pf;
   Image_Entry *im;
   size_t       size;
   size_t       file_length;
   size_t       key_length;
   unsigned int i;

   // FIXME: In the long term we should certainly merge both mmap and filename path
   //  by just using the mmap path. But for the time being, let's just have two path
   //  as it is unlikely to really have an impact on real world application
   if (!f)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return NULL;
     }

   /* generate hkey from file+key+load opts */
   file_length = sizeof (Eina_File*) * 2;
   key_length = key ? strlen(key) : 6;
   size = file_length + key_length + 400; // enough padding for loadopts_append
   hkey = alloca(sizeof (char) * size);
   pf = (char*) &f;
   for (size = 0, i = 0; i < sizeof (Eina_File*); i++)
     {
        hkey[size++] = hexcode[(pf[i] & 0xF0) >> 4];
        hkey[size++] = hexcode[(pf[i] & 0x0F)];
     }
   memcpy(hkey + size, "//://", 5);
   size += 5;
   if (key) ckey = key;
   memcpy(hkey + size, ckey, key_length);
   size += key_length;
   size += _evas_cache_image_loadopts_append(hkey + size, &lo);


   /* find image by key in active mmap hash */
   SLKL(engine_lock);
   im = eina_hash_find(cache->mmap_activ, hkey);
   if (im)
     {
        if (im->f != f)
          {
             /* as active cache find - if we match in lru and its invalid, dirty */
             _evas_cache_image_dirty_add(im);
             /* this image never used, so it have to be deleted */
             _evas_cache_image_entry_delete(cache, im);
             im = NULL;
          }
        else if (!im->load_failed) goto on_ok;
        else if (im->load_failed)
          {
             _evas_cache_image_dirty_add(im);
             im = NULL;
          }
     }

   /* find image by key in inactive/lru hash */
   im = eina_hash_find(cache->mmap_inactiv, hkey);
   if (im)
     {
        if (im->f != f)
          {
             /* as active cache find - if we match in lru and its invalid, dirty */
             _evas_cache_image_dirty_add(im);
             /* this image never used, so it have to be deleted */
             _evas_cache_image_entry_delete(cache, im);
             im = NULL;
          }
        else if (!im->load_failed)
          {
             _evas_cache_image_lru_del(im);
             _evas_cache_image_activ_add(im);
             goto on_ok;
          }
     }

   im = _evas_cache_image_entry_new(cache, hkey, NULL, f, NULL, key, lo, error);
   if (!im)
     {
        SLKU(engine_lock);
        return NULL;
     }

 on_ok:
   *error = EVAS_LOAD_ERROR_NONE;
   im->references++;
   SLKU(engine_lock);
   return im;
}

/**
 * @brief Requests an image entry from the cache for a file path.
 * This function attempts to find an existing cache entry for the given file path,
 * key, and load options. It checks file timestamps to ensure cache validity.
 * If a valid entry is not found, a new one is created.
 * The cache key is generated based on the file path, string key, and load options.
 *
 * @param cache The image cache instance.
 * @param file The path to the image file. Must not be NULL.
 * @param key An optional sub-key for the image within the file. Can be NULL.
 * @param lo Pointer to the image loading options.
 * @param error Pointer to an integer to store the error code on failure.
 * @return A pointer to the Image_Entry. The reference count of the entry is incremented.
 *         Returns NULL on error, with @p error set to an EVAS_LOAD_ERROR_* code.
 *         Possible error codes:
 *         - EVAS_LOAD_ERROR_GENERIC (if @p file is NULL)
 *         - EVAS_LOAD_ERROR_DOES_NOT_EXIST
 *         - EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED
 *         - EVAS_LOAD_ERROR_PERMISSION_DENIED
 *         - Errors from _evas_cache_image_entry_new()
 */
EVAS_API Image_Entry *
evas_cache_image_request(Evas_Cache_Image *cache, const char *file,
                         const char *key, Evas_Image_Load_Opts *lo, int *error)
{
   const char           *ckey = "(null)";
   char                 *hkey;
   Image_Entry          *im;
   size_t                size;
   int                   stat_done = 0, stat_failed = 0;
   size_t                file_length;
   size_t                key_length;
   struct stat           st;
   Image_Timestamp       tstamp;
   Evas_Image_Load_Opts  tlo;
   Eina_Bool             skip = lo->skip_head;

   if (!file)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
        return NULL;
     }

   /* generate hkey from file+key+load opts */
   file_length = strlen(file);
   key_length = key ? strlen(key) : 6;
   size = file_length + key_length + 132;
   hkey = alloca(sizeof (char) * size);
   memcpy(hkey, file, file_length);
   size = file_length;
   memcpy(hkey + size, "//://", 5);
   size += 5;
   if (key) ckey = key;
   memcpy(hkey + size, ckey, key_length);
   size += key_length;
   size += _evas_cache_image_loadopts_append(hkey + size, &lo);
   tlo = *lo;
   tlo.skip_head = skip;

   /* find image by key in active hash */
   SLKL(engine_lock);
   im = eina_hash_find(cache->activ, hkey);
   if ((im) && (!im->load_failed))
     {
        int ok = 1;

        stat_done = 1;
        if (stat(file, &st) < 0)
          {
             stat_failed = 1;
             ok = 0;
          }
        else if (!_timestamp_compare(&(im->tstamp), &st)) ok = 0;

        if (ok) goto on_ok;
        /* image we found doesn't match what's on disk (stat info wise)
         * so dirty the active cache entry so we never find it again. this
         * also implicitly guarantees that we only have 1 active copy
         * of an image at a given key. we wither find it and keep re-reffing
         * it or we dirty it and get it out */
        _evas_cache_image_dirty_add(im);
        im = NULL;
     }
   else if ((im) && (im->load_failed))
     {
        _evas_cache_image_dirty_add(im);
        im = NULL;
     }

   /* find image by key in inactive/lru hash */
   im = eina_hash_find(cache->inactiv, hkey);
   if ((im) && (!im->load_failed))
     {
        int ok = 1;

        if (!stat_done)
          {
             stat_done = 1;
             if (stat(file, &st) < 0)
               {
                  stat_failed = 1;
                       ok = 0;
               }
             else if (!_timestamp_compare(&(im->tstamp), &st)) ok = 0;
          }
        else if (!_timestamp_compare(&(im->tstamp), &st)) ok = 0;

        if (ok)
          {
             /* remove from lru and make it active again */
             _evas_cache_image_lru_del(im);
             _evas_cache_image_activ_add(im);
             goto on_ok;
          }
        /* as active cache find - if we match in lru and its invalid, dirty */
        _evas_cache_image_dirty_add(im);
        /* this image never used, so it have to be deleted */
        _evas_cache_image_entry_delete(cache, im);
        im = NULL;
     }
   else if ((im) && (im->load_failed))
     {
        /* as active cache find - if we match in lru and its invalid, dirty */
        _evas_cache_image_dirty_add(im);
        /* this image never used, so it have to be deleted */
        _evas_cache_image_entry_delete(cache, im);
        im = NULL;
     }
   if (stat_failed) goto on_stat_error;

   if (!stat_done)
     {
        if (stat(file, &st) < 0) goto on_stat_error;
     }
   _timestamp_build(&tstamp, &st);
   im = _evas_cache_image_entry_new(cache, hkey, &tstamp, NULL,
                                    file, key, &tlo, error);
   if (!im) goto on_stat_error;
   if (cache->func.debug) cache->func.debug("request", im);

on_ok:
   *error = EVAS_LOAD_ERROR_NONE;
////   SLKL(im->lock);
   im->references++;
////   SLKU(im->lock);
   SLKU(engine_lock);
   return im;

on_stat_error:
#ifndef _WIN32
   if ((errno == ENOENT) || (errno == ENOTDIR) ||
       (errno == ENAMETOOLONG) || (errno == ELOOP))
#else
     if (errno == ENOENT)
#endif
       *error = EVAS_LOAD_ERROR_DOES_NOT_EXIST;
#ifndef _WIN32
     else if ((errno == ENOMEM) || (errno == EOVERFLOW))
#else
     else if (errno == ENOMEM)
#endif
       *error = EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
     else if (errno == EACCES)
       *error = EVAS_LOAD_ERROR_PERMISSION_DENIED;
     else
       *error = EVAS_LOAD_ERROR_GENERIC;

   SLKU(engine_lock);
   return NULL;
}

/**
 * @brief Increments the reference count of an image cache entry.
 * @param im The image entry to reference.
 */
EVAS_API void
evas_cache_image_ref(Image_Entry *im)
{
   SLKL(engine_lock);
   im->references++;
   SLKU(engine_lock);
}

/**
 * @brief Decrements the reference count of an image cache entry.
 * If the reference count drops to zero, the image entry may be moved to
 * the LRU list, deleted (if dirty or load failed), or have its preload cancelled.
 * @param im The image entry to dereference.
 */
EVAS_API void
evas_cache_image_drop(Image_Entry *im)
{
   Evas_Cache_Image *cache;
   int references;

   if (!im->cache) return;
   SLKL(engine_lock);
   im->references--;
   if (im->references < 0) im->references = 0;
   references = im->references;
   SLKU(engine_lock);

   cache = im->cache;

   if (references == 0)
     {
        if (im->preload)
          {
             SLKL(engine_lock);
             _evas_cache_image_entry_preload_remove(im, NULL, EINA_TRUE);
             SLKU(engine_lock);
             return;
          }
        if ((im->flags.dirty) || (im->load_failed))
          {
             SLKL(engine_lock);
             _evas_cache_image_entry_delete(cache, im);
             SLKU(engine_lock);
             return;
          }
        if (cache)
          {
             SLKL(engine_lock);
             _evas_cache_image_lru_add(im);
             SLKU(engine_lock);
             evas_cache_image_flush(cache);
          }
     }
}

/**
 * @brief Marks an image entry's pixel data as not immediately needed.
 * If the image has only one reference and is not dirty, it's moved to the
 * "LRU no data" list. This suggests that its pixel data can be unloaded
 * to save memory, while keeping its metadata cached.
 * @param im The image entry.
 */
EVAS_API void
evas_cache_image_data_not_needed(Image_Entry *im)
{
   int references;

   references = im->references;
   if (references > 1) return;
   if ((im->flags.dirty) || (!im->flags.need_data)) return;
   SLKL(engine_lock);
   _evas_cache_image_lru_nodata_add(im);
   SLKU(engine_lock);
}

/**
 * @brief Marks an image entry as dirty, optionally specifying a region.
 * If the image has multiple references, a copy of the image data is made,
 * and this new copy is marked dirty. The original image entry's reference
 * count is decremented. The new (or original, if single-referenced) entry
 * is moved to the dirty list.
 *
 * @param im The image entry to mark as dirty.
 * @param x The x-coordinate of the dirty region.
 * @param y The y-coordinate of the dirty region.
 * @param w The width of the dirty region.
 * @param h The height of the dirty region.
 * @return A pointer to the (potentially new) dirty Image_Entry.
 *         The caller receives a new reference to this entry, and the reference
 *         to the original @p im is dropped by this function.
 *         Returns NULL on error (e.g., if copying data fails).
 */
EVAS_API Image_Entry *
evas_cache_image_dirty(Image_Entry *im, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
   Image_Entry *im_dirty = im;
   Evas_Cache_Image *cache;

   if (!im->cache) return NULL;
   cache = im->cache;
   if (!(im->flags.dirty))
     {
        if (im->references == 1) im_dirty = im;
        else
          {
             im_dirty =
                evas_cache_image_copied_data(cache, im->w, im->h,
                                             evas_cache_image_pixels(im),
                                             im->flags.alpha, im->space);
             if (!im_dirty) goto on_error;
             if (cache->func.debug) cache->func.debug("dirty-src", im);
             cache->func.dirty(im_dirty, im);
             if (cache->func.debug) cache->func.debug("dirty-out", im_dirty);
             im_dirty->references = 1;
             evas_cache_image_drop(im);
          }
        SLKL(engine_lock);
        _evas_cache_image_dirty_add(im_dirty);
        SLKU(engine_lock);
     }

   if (cache->func.debug) cache->func.debug("dirty-region", im_dirty);
   if (cache->func.dirty_region)
     cache->func.dirty_region(im_dirty, x, y, w, h);
   return im_dirty;

on_error:
   evas_cache_image_drop(im);
   return NULL;
}

/**
 * @brief Ensures an image entry is "alone" (i.e., uniquely referenced and mutable).
 * If the image entry has more than one reference, a copy of its data is created
 * to ensure modifications do not affect other users of the original shared data.
 * The resulting image entry (either the original or the new copy) is marked dirty.
 *
 * @param im The image entry to make "alone".
 * @return A pointer to the "alone" Image_Entry (which might be a new entry if a copy was made).
 *         The caller receives a new reference to this entry, and the reference
 *         to the original @p im is dropped by this function.
 *         Returns NULL on error (e.g., if copying data fails).
 */
EVAS_API Image_Entry *
evas_cache_image_alone(Image_Entry *im)
{
   Evas_Cache_Image *cache;
   Image_Entry *im_dirty = im;
   int references;

   if (!im->cache) return NULL;
   cache = im->cache;
   references = im->references;

   if (references <= 1)
     {
        SLKL(engine_lock);
        if (!im->flags.dirty) _evas_cache_image_dirty_add(im);
        SLKU(engine_lock);
     }
   else
     {
        im_dirty = evas_cache_image_copied_data(cache, im->w, im->h,
                                                evas_cache_image_pixels(im),
                                                im->flags.alpha,
                                                im->space);
        if (!im_dirty) goto on_error;
        if (cache->func.debug) cache->func.debug("dirty-src", im);
        cache->func.dirty(im_dirty, im);
        if (cache->func.debug) cache->func.debug("dirty-out", im_dirty);
        im_dirty->references = 1;
        evas_cache_image_drop(im);
     }
   return im_dirty;

on_error:
   evas_cache_image_drop(im);
   return NULL;
}

/**
 * @brief Creates a new image cache entry from copied pixel data.
 * A new, "dirty" image entry is created, and the provided pixel data is copied into it.
 * The new entry is not associated with any file or key initially.
 *
 * @param cache The image cache instance.
 * @param w The width of the image data.
 * @param h The height of the image data.
 * @param image_data Pointer to the pixel data to copy.
 * @param alpha Flag indicating if the image data has an alpha channel.
 * @param cspace The colorspace of the image data.
 * @return A pointer to the newly created Image_Entry with a reference count of 1.
 *         Returns NULL on failure (e.g., allocation error, data copy error).
 */
EVAS_API Image_Entry *
evas_cache_image_copied_data(Evas_Cache_Image *cache,
                             unsigned int w, unsigned int h,
                             DATA32 *image_data, int alpha,
                             Evas_Colorspace cspace)
{
   int err;
   Image_Entry *im;

   if (!cache) return NULL;
   if ((cspace == EVAS_COLORSPACE_YCBCR422P601_PL) ||
       (cspace == EVAS_COLORSPACE_YCBCR422P709_PL) ||
       (cspace == EVAS_COLORSPACE_YCBCR422601_PL))
     w &= ~0x1;

   SLKL(engine_lock);
   im = _evas_cache_image_entry_new(cache, NULL, NULL, NULL, NULL, NULL, NULL, &err);
   SLKU(engine_lock);
   if (!im) return NULL;
   im->space = cspace;
   im->flags.alpha = alpha;
   _evas_cache_image_entry_surface_alloc(cache, im, w, h);
   if (cache->func.copied_data(im, w, h, image_data, alpha, cspace) != 0)
     {
        SLKL(engine_lock);
        _evas_cache_image_entry_delete(cache, im);
        SLKU(engine_lock);
        return NULL;
     }
   im->references = 1;
   im->flags.loaded = EINA_TRUE;
   if (cache->func.debug) cache->func.debug("copied-data", im);
   return im;
}

/**
 * @brief Creates a new image cache entry that directly uses provided pixel data (no copy).
 * A new, "dirty" image entry is created, and it is configured to use the
 * @p image_data pointer directly (the data is not copied by this function,
 * but the underlying engine's `data` callback might copy it).
 * The new entry is not associated with any file or key initially.
 *
 * @param cache The image cache instance.
 * @param w The width of the image.
 * @param h The height of the image.
 * @param image_data Pointer to the pixel data. The lifetime of this data must be
 *                   managed appropriately by the caller, or the engine's `data`
 *                   callback must handle it (e.g. by copying).
 * @param alpha Flag indicating if the image data has an alpha channel.
 * @param cspace The colorspace of the image data.
 * @return A pointer to the newly created Image_Entry with a reference count of 1.
 *         Returns NULL on failure (e.g., allocation error, engine data setup error).
 */
EVAS_API Image_Entry *
evas_cache_image_data(Evas_Cache_Image *cache, unsigned int w, unsigned int h,
                      DATA32 *image_data, int alpha, Evas_Colorspace cspace)
{
   int err;
   Image_Entry *im;

   if (!cache) return NULL;
   if ((cspace == EVAS_COLORSPACE_YCBCR422P601_PL) ||
       (cspace == EVAS_COLORSPACE_YCBCR422P709_PL) ||
       (cspace == EVAS_COLORSPACE_YCBCR422601_PL))
     w &= ~0x1;

   SLKL(engine_lock);
   im = _evas_cache_image_entry_new(cache, NULL, NULL, NULL, NULL, NULL, NULL, &err);
   SLKU(engine_lock);
   if (!im) return NULL;
   im->w = w;
   im->h = h;
   im->flags.alpha = alpha;
   if (cache->func.data(im, w, h, image_data, alpha, cspace) != 0)
     {
        SLKL(engine_lock);
        _evas_cache_image_entry_delete(cache, im);
        SLKU(engine_lock);
        return NULL;
     }
   im->references = 1;
   im->flags.loaded = EINA_TRUE;
   if (cache->func.debug) cache->func.debug("data", im);
   return im;
}

/**
 * @brief Allocates or reallocates the surface for an image entry.
 * This function ensures that the underlying surface (pixel buffer) for the
 * image entry has the specified dimensions.
 *
 * @param im The image entry.
 * @param w The desired width for the surface.
 * @param h The desired height for the surface.
 */
EVAS_API void
evas_cache_image_surface_alloc(Image_Entry *im, unsigned int w, unsigned int h)
{
   Evas_Cache_Image *cache = im->cache;

   if (!im->cache) return;
   if ((im->space == EVAS_COLORSPACE_YCBCR422P601_PL) ||
       (im->space == EVAS_COLORSPACE_YCBCR422P709_PL) ||
       (im->space == EVAS_COLORSPACE_YCBCR422601_PL))
     w &= ~0x1;

   _evas_cache_image_entry_surface_alloc(cache, im, w, h);
   if (cache->func.debug) cache->func.debug("surface-alloc", im);
}

/**
 * @brief Creates a new image entry by resizing an existing one.
 * A new image entry is created with the specified dimensions @p w and @p h.
 * The content of the original image @p im is scaled or transferred to the new entry.
 * The original image entry @p im has its reference count decremented twice
 * (once for the initial ref taken by this function, once because it's being replaced).
 *
 * @param im The original image entry to resize.
 * @param w The new width.
 * @param h The new height.
 * @return A pointer to the new Image_Entry with the specified size and content
 *         from the original, with a reference count of 1.
 *         Returns NULL on error (e.g., allocation failure, resize operation failure).
 *         The original @p im is effectively "consumed" by this call on success or failure.
 */
EVAS_API Image_Entry *
evas_cache_image_size_set(Image_Entry *im, unsigned int w, unsigned int h)
{
   Evas_Cache_Image *cache;
   Image_Entry *im2 = NULL;
   int error;

   if (!im->cache) return im;
   evas_cache_image_ref(im);
   if ((im->space == EVAS_COLORSPACE_YCBCR422P601_PL) ||
       (im->space == EVAS_COLORSPACE_YCBCR422P709_PL) ||
       (im->space == EVAS_COLORSPACE_YCBCR422601_PL))
     w &= ~0x1;
   if ((im->w == w) && (im->h == h))
     {
        evas_cache_image_drop(im);
        return im;
     }

   cache = im->cache;
   SLKL(engine_lock);
   im2 = _evas_cache_image_entry_new(cache, NULL, NULL, NULL, NULL, NULL, NULL, &error);
   SLKU(engine_lock);
   if (!im2) goto on_error;

   im2->flags.alpha = im->flags.alpha;
   im2->space = im->space;
   im2->load_opts = im->load_opts;
   _evas_cache_image_entry_surface_alloc(cache, im2, w, h);
   error = cache->func.size_set(im2, im, w, h);
   if (error != 0) goto on_error;
   im2->references = 1;
   im2->flags.loaded = EINA_TRUE;
   if (cache->func.debug) cache->func.debug("size_set", im2);
   /* this drop is for handling refereces in this function */
   evas_cache_image_drop(im);
   /* we don't need im at this point, drop it! */
   evas_cache_image_drop(im);
   return im2;

on_error:
   SLKL(engine_lock);
   if (im2) _evas_cache_image_entry_delete(cache, im2);
   SLKU(engine_lock);
   /* this drop is for handling refereces in this function */
   evas_cache_image_drop(im);
   /* we don't need im at this point, drop it! */
   evas_cache_image_drop(im);
   return NULL;
}

/**
 * @brief Loads the pixel data for an image entry.
 * If the image data is not already loaded, this function triggers the loading process.
 * If the image was being preloaded asynchronously, the preload is cancelled,
 * and loading proceeds synchronously (or waits for async completion).
 *
 * @param im The image entry for which to load data.
 * @return An EVAS_LOAD_ERROR_* code. EVAS_LOAD_ERROR_NONE on success.
 *         Returns EVAS_LOAD_ERROR_NONE immediately if data is already loaded
 *         and not animated, or if im->cache is NULL.
 */
EVAS_API int
evas_cache_image_load_data(Image_Entry *im)
{
   Eina_Bool preload = EINA_FALSE;
   int error = EVAS_LOAD_ERROR_NONE;

   if (!im->cache) return error;
   evas_cache_image_ref(im);
   if ((im->flags.loaded) && (!im->animated.animated))
     {
        evas_cache_image_drop(im);
        return error;
     }
   evas_common_rgba_pending_unloads_remove(im);
   if (im->preload)
     {
        preload = EINA_TRUE;
        if (!im->flags.pending)
          {
             im->cache->preload = eina_list_remove(im->cache->preload, im);
             im->cache->pending = eina_list_append(im->cache->pending, im);
             im->flags.pending = 1;
             evas_preload_thread_cancel(im->preload);
          }
        evas_async_events_process();
        evas_preload_pthread_wait(im->preload, 0.01);

        while (im->flags.preload_pending)
          {
             evas_async_events_process();
             evas_preload_pthread_wait(im->preload, 0.1);
          }
     }

   if ((im->flags.loaded) && (!im->animated.animated))
     {
        evas_cache_image_drop(im);
        return error;
     }

   SLKL(im->lock);
   im->flags.in_progress = EINA_TRUE;
   error = im->cache->func.load(im);
   im->flags.in_progress = EINA_FALSE;
   SLKU(im->lock);

   im->flags.loaded = 1;
   if (im->cache->func.debug) im->cache->func.debug("load", im);
   if (error != EVAS_LOAD_ERROR_NONE)
     {
        _evas_cache_image_entry_surface_alloc(im->cache, im, im->w, im->h);
        im->flags.loaded = 0;
     }
   if (preload) _evas_cache_image_async_end(im);
   evas_cache_image_drop(im);
   return error;
}

/**
 * @brief Unloads the pixel data for an image entry.
 * This function releases the pixel data associated with an image entry,
 * typically to free memory. It calls the cache's destructor function for the entry.
 * If the image is currently being loaded asynchronously, it attempts to set an
 * unload_cancel flag.
 *
 * @param im The image entry whose data is to be unloaded.
 */
EVAS_API void
evas_cache_image_unload_data(Image_Entry *im)
{
   if (!im->cache) return;
   evas_cache_image_ref(im);
   if (im->flags.in_progress)
     {
        evas_cache_image_drop(im);
        return;
     }
   evas_cache_image_preload_cancel(im, NULL, EINA_TRUE);

   if (SLKT(im->lock) == EINA_FALSE) /* can't get image lock - busy async load */
     {
        SLKL(im->lock_cancel);
        im->flags.unload_cancel = EINA_TRUE;
        SLKU(im->lock_cancel);
        evas_cache_image_drop(im);
        return;
     }

   SLKL(im->lock_cancel);
   if ((!im->flags.loaded) || (!im->file && !im->f) || (!im->info.module) ||
       (im->flags.dirty))
     {
        SLKU(im->lock_cancel);
        SLKU(im->lock);
        evas_cache_image_drop(im);
        return;
     }
   SLKU(im->lock_cancel);
   im->cache->func.destructor(im);
   SLKU(im->lock);
   evas_cache_image_drop(im);
   //FIXME: imagedataunload - inform owners
}

/**
 * @brief Callback function used with eina_hash_foreach to unload image data.
 * This function is used by evas_cache_image_unload_all to iterate through
 * hash tables of image entries and call evas_cache_image_unload_data on each.
 * @param hash The hash table being iterated. (Unused)
 * @param key The key of the hash entry. (Unused)
 * @param data The data (Image_Entry pointer) associated with the hash entry.
 * @param fdata User data passed to eina_hash_foreach. (Unused)
 * @return EINA_TRUE to continue iteration.
 */
static Eina_Bool
_evas_cache_image_unload_cb(EINA_UNUSED const Eina_Hash *hash, EINA_UNUSED const void *key, void *data, EINA_UNUSED void *fdata)
{
   evas_cache_image_unload_data(data);
   return EINA_TRUE;
}

/**
 * @brief Unloads pixel data for all unloadable images in the cache.
 * Iterates through LRU lists and active/inactive hash tables, calling
 * evas_cache_image_unload_data for each image entry.
 * @param cache The image cache instance.
 */
EVAS_API void
evas_cache_image_unload_all(Evas_Cache_Image *cache)
{
   Image_Entry *im;

   if (!cache) return;
// _evas_cache_image_unload_cb -> evas_cache_image_unload_data -> evas_cache_image_ref
//  deadlock
//////   SLKL(engine_lock);
   EINA_INLIST_FOREACH(cache->lru, im) evas_cache_image_unload_data(im);
   EINA_INLIST_FOREACH(cache->lru_nodata, im) evas_cache_image_unload_data(im);
   eina_hash_foreach(cache->activ, _evas_cache_image_unload_cb, NULL);
   eina_hash_foreach(cache->inactiv, _evas_cache_image_unload_cb, NULL);
//////   SLKU(engine_lock);
}

static int async_frozen = 0; /**< Counter for freezing asynchronous operations. >0 means frozen. */

/**
 * @brief Gets the current freeze count for asynchronous cache operations.
 * @return The number of times async operations have been frozen.
 *         If > 0, async operations may be deferred or handled differently.
 */
EVAS_API int
evas_cache_async_frozen_get(void)
{
   return async_frozen;
}

/**
 * @brief Increments the freeze counter for asynchronous cache operations.
 * When frozen, new asynchronous operations might be delayed or queued.
 */
EVAS_API void
evas_cache_async_freeze(void)
{
   async_frozen++;
}

/**
 * @brief Decrements the freeze counter for asynchronous cache operations.
 * When the counter returns to 0, normal asynchronous operation processing resumes.
 */
EVAS_API void
evas_cache_async_thaw(void)
{
   async_frozen--;
}

/**
 * @brief Checks if an image entry's data is currently loaded.
 * @param im The image entry to check.
 * @return EINA_TRUE if the image data is loaded, EINA_FALSE otherwise.
 */
EVAS_API Eina_Bool
evas_cache_image_is_loaded(Image_Entry *im)
{
   if (im->flags.loaded) return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @brief Initiates preloading of image data for a specific target.
 * If the image data is already loaded and available, it notifies the target immediately.
 * Otherwise, it adds the target to the image entry's preload list and starts
 * asynchronous preloading if not already in progress.
 *
 * @param im The image entry to preload.
 * @param target The Evas object (Eo) requesting the preload.
 * @param preloaded_cb Optional callback function to invoke when preloading is complete for this target.
 * @param preloaded_data Optional data to pass to @p preloaded_cb.
 */
EVAS_API void
evas_cache_image_preload_data(Image_Entry *im, const Eo *target, void (*preloaded_cb)(void *), void *preloaded_data)
{
   RGBA_Image *img = (RGBA_Image *)im;

   if (!im->cache) return;
   evas_cache_image_ref(im);
   if (((int)im->w > 0) && ((int)im->h > 0) &&
       (((im->flags.loaded) && (img->image.data)) ||
        (im->flags.textured && !im->flags.updated_data)))
     {
        _evas_cache_image_preloaded_notify(im);
        evas_object_inform_call_image_preloaded((Evas_Object*)target);
        evas_cache_image_drop(im);
        return;
     }
   im->flags.loaded = 0;
   if (!_evas_cache_image_entry_preload_add(im, target, preloaded_cb, preloaded_data))
     evas_object_inform_call_image_preloaded((Evas_Object*) target);
   evas_cache_image_drop(im);
}

/**
 * @brief Cancels a preload request for an image entry, for a specific target.
 * @param im The image entry.
 * @param target The Evas object (Eo) target whose preload request is to be cancelled. Must not be NULL.
 * @param force If EINA_TRUE, the preload operation is forcefully cancelled if this is the last target
 *              or if other conditions for forceful cancellation are met.
 */
EVAS_API void
evas_cache_image_preload_cancel(Image_Entry *im, const Eo *target, Eina_Bool force)
{
   if (!target) return;
   evas_cache_image_ref(im);
   _evas_cache_image_entry_preload_remove(im, target, force);
   evas_cache_image_drop(im);
}

#ifdef CACHEDUMP
static int total = 0;

static void
_dump_img(Image_Entry *im, const char *type)
{
   if (!im->cache) return;
   total += im->cache->func.mem_size_get(im);
   printf("%s: %4i: %4ib, %4ix%4i alloc[%4ix%4i] [%s] [%s]\n",
          type,
          im->references,
          im->cache->func.mem_size_get(im),
          im->w, im->h, im->allocated.w, im->allocated.h,
          im->f ? eina_file_filename_get(im->f) : im->file, im->key);
}

static Eina_Bool
_dump_cache_active(EINA_UNUSED const Eina_Hash *hash, EINA_UNUSED const void *key, void *data, void *fdata EINA_UNUSED)
{
   Image_Entry *im = data;
   _dump_img(im, "ACTIVE");
   return EINA_TRUE;
}

static void
_dump_cache(Evas_Cache_Image *cache)
{
   Image_Entry *im;

   if (!cache) return;
   printf("--CACHE DUMP----------------------------------------------------\n");
   printf("cache: %ikb / %ikb\n",
          cache->usage / 1024,
          cache->limit / 1024);
   printf("................................................................\n");
   total = 0;
   SLKL(engine_lock);
   EINA_INLIST_FOREACH(cache->lru_nodata, im)
      _dump_img(im, "NODATA");
   EINA_INLIST_FOREACH(cache->lru, im)
      _dump_img(im, "DATA  ");
   printf("tot: %i\n"
          "usg: %i\n",
          total,
          cache->usage);
   eina_hash_foreach(cache->activ, _dump_cache_active, NULL);
   SLKU(engine_lock);
}
#endif

/**
 * @brief Flushes the image cache to reduce memory usage towards its limit.
 * This function attempts to evict images from the LRU lists (both "data" and "no data")
 * until the cache usage is within the configured limit. Images from the `lru` list
 * (with data) are deleted entirely. Images from the `lru_nodata` list have their
 * surfaces deleted (pixel data freed), but their metadata might remain.
 *
 * @param cache The image cache instance.
 * @return The cache usage in bytes after flushing. Returns -1 if the cache limit
 *         is set to unlimited. Returns 0 if @p cache is NULL.
 */
EVAS_API int
evas_cache_image_flush(Evas_Cache_Image *cache)
{
   if (!cache) return 0;
#ifdef CACHEDUMP
   _dump_cache(cache);
#endif
   if (cache->limit == (unsigned int)-1) return -1;

   SLKL(engine_lock);
   while ((cache->lru) && (cache->limit < (unsigned int)cache->usage))
     {
        Image_Entry *im;

        im = (Image_Entry *)cache->lru->last;
        if (!im) im = (Image_Entry *)cache->lru;
        _evas_cache_image_entry_delete(cache, im);
     }

   while ((cache->lru_nodata) && (cache->limit < (unsigned int)cache->usage))
     {
        Image_Entry *im;

        im = (Image_Entry *)cache->lru_nodata->last;
        if (!im) im = (Image_Entry *)cache->lru_nodata;
        _evas_cache_image_lru_nodata_del(im);
        cache->func.surface_delete(im);
        im->flags.loaded = 0;
     }
   SLKU(engine_lock);

   return cache->usage;
}

/**
 * @brief Creates an "empty" image cache entry.
 * This creates a new image entry that is not associated with any file, key, or pixel data initially.
 * It's a basic, minimal entry.
 *
 * @param cache The image cache instance.
 * @return A pointer to the newly created empty Image_Entry with a reference count of 1.
 *         Returns NULL on failure (e.g., allocation error).
 */
EVAS_API Image_Entry *
evas_cache_image_empty(Evas_Cache_Image *cache)
{
   int err;
   Image_Entry *im;

   if (!cache) return NULL;
   SLKL(engine_lock);
   im = _evas_cache_image_entry_new(cache, NULL, NULL, NULL, NULL, NULL, NULL, &err);
   SLKU(engine_lock);
   if (!im) return NULL;
   im->references = 1;
   return im;
}

/**
 * @brief Sets the colorspace for an image entry.
 * If the new colorspace is different from the current one, this function
 * updates the image entry's colorspace and calls the engine-specific
 * `color_space` callback to handle any necessary conversions or updates.
 *
 * @param im The image entry.
 * @param cspace The new Evas_Colorspace to set.
 */
EVAS_API void
evas_cache_image_colorspace(Image_Entry *im, Evas_Colorspace cspace)
{
   if (!im->cache) return;
   evas_cache_image_ref(im);
   if (im->space == cspace) goto done;
   im->space = cspace;
   if (!im->cache) goto done;
   im->cache->func.color_space(im, cspace);
done:
   evas_cache_image_drop(im);
}

/**
 * @brief Retrieves the private data associated with the cache instance of an image entry.
 * Each Evas_Cache_Image can have a `void *data` pointer for engine-specific data.
 * This function allows retrieving that data given an Image_Entry.
 *
 * @param im The image entry.
 * @return The private data pointer from the image entry's cache, or NULL if
 *         the entry has no cache or the cache has no private data.
 */
EVAS_API void *
evas_cache_private_from_image_entry_get(Image_Entry *im)
{
   void *data;
   if (!im->cache) return NULL;
   evas_cache_image_ref(im);
   data = (void *)im->cache->data;
   evas_cache_image_drop(im);
   return data;
}

/**
 * @brief Retrieves the private data associated with an image cache instance.
 * @param cache The image cache instance.
 * @return The private data pointer, or NULL if @p cache is NULL or has no private data.
 */
EVAS_API void *
evas_cache_private_get(Evas_Cache_Image *cache)
{
   if (!cache) return NULL;
   return cache->data;
}

/**
 * @brief Sets the private data for an image cache instance.
 * This allows associating arbitrary engine-specific data with a cache.
 * @param cache The image cache instance.
 * @param data The private data pointer to set.
 */
EVAS_API void
evas_cache_private_set(Evas_Cache_Image *cache, const void *data)
{
   if (!cache) return;
   cache->data = (void *)data;
}

/**
 * @brief Retrieves a pointer to the pixel data of an image entry's surface.
 * This calls the engine-specific `surface_pixels` callback.
 *
 * @param im The image entry.
 * @return A pointer to the pixel data (DATA32 *), or NULL if the entry has no cache
 *         or the `surface_pixels` callback returns NULL.
 */
EVAS_API DATA32 *
evas_cache_image_pixels(Image_Entry *im)
{
   if (!im->cache) return NULL;
   return im->cache->func.surface_pixels(im);
}
