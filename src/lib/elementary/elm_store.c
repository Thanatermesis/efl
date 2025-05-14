#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Elementary_Cursor.h>

#include "elm_priv.h"
#include "elm_genlist_eo.h"

typedef struct _Elm_Store_Filesystem           Elm_Store_Filesystem;
typedef struct _Elm_Store_Item_Filesystem      Elm_Store_Item_Filesystem;

#define ELM_STORE_MAGIC            0x3f89ea56
#define ELM_STORE_FILESYSTEM_MAGIC 0x3f89ea57
#define ELM_STORE_ITEM_MAGIC       0x5afe8c1d

/**
 * @internal
 * @brief Represents the core store object.
 * This structure holds all the data and callbacks necessary for managing
 * a collection of items, typically for display in a genlist.
 */
struct _Elm_Store
{
   EINA_MAGIC; /**< Magic number for type checking. */
   void           (*free)(Elm_Store *store); /**< Custom free function for derived store types. */
   struct
     {
        void        (*free)(Elm_Store_Item *item); /**< Custom free function for derived store item types. */
     } item;
   Evas_Object   *genlist; /**< The target genlist widget this store populates. */
   Ecore_Thread  *list_th; /**< Thread used for listing items. */
   Eina_Inlist   *items; /**< Inlist of all Elm_Store_Item managed by this store. */
   Eina_List     *realized; /**< List of items that are currently realized (visible or cached). */
   int            realized_count; /**< Number of items currently realized and visible in the genlist. */
   int            cache_max; /**< Maximum number of non-visible items to keep cached. */
   struct
     {
        struct
          {
             Elm_Store_Item_List_Cb     func; /**< Callback for listing/filtering items. */
             void                      *data; /**< User data for the list callback. */
          } list;
        struct
          {
             Elm_Store_Item_Fetch_Cb    func; /**< Callback for fetching item data. */
             void                      *data; /**< User data for the fetch callback. */
          } fetch;
        struct
          {
             Elm_Store_Item_Unfetch_Cb  func; /**< Callback for unfetching/freeing item data. */
             void                      *data; /**< User data for the unfetch callback. */
          } unfetch;
     } cb; /**< Collection of user-provided callbacks. */
   Eina_Bool sorted : 1; /**< Flag indicating if items should be sorted. */
   Eina_Bool fetch_thread : 1; /**< Flag indicating if fetching should occur in a separate thread. */
};

/**
 * @internal
 * @brief Represents an individual item within an Elm_Store.
 * This structure holds data specific to one item, including its state
 * (live, realized, fetched), associated genlist item, and data.
 */
struct _Elm_Store_Item
{
   EINA_INLIST; /**< Macro for inlist membership. */
   EINA_MAGIC; /**< Magic number for type checking. */
   Elm_Store                    *store; /**< Pointer to the parent Elm_Store. */
   Elm_Object_Item              *item; /**< The corresponding Elm_Object_Item in the genlist. */
   Ecore_Thread                 *fetch_th; /**< Thread used for fetching this item's data. */
   Ecore_Job                    *eval_job; /**< Job for evaluating item state changes. */
   const Elm_Store_Item_Mapping *mapping; /**< Mapping rules for this item's data to genlist parts. */
   void                         *data; /**< Pointer to the user-provided data for this item. */
   Eina_Lock                     lock; /**< Lock for thread-safe access to item data. */
   Eina_Bool                     live : 1; /**< Flag indicating if the item is currently live (visible in genlist). */
   Eina_Bool                     was_live : 1; /**< Previous live state, for detecting changes. */
   Eina_Bool                     realized : 1; /**< Flag indicating if the item is realized (in the realized list). */
   Eina_Bool                     fetched : 1; /**< Flag indicating if the item's data has been fetched. */
};

/**
 * @internal
 * @brief Filesystem-specific store data, extending Elm_Store.
 */
struct _Elm_Store_Filesystem
{
   Elm_Store base; /**< Base Elm_Store structure. */
   EINA_MAGIC; /**< Magic number for type checking. */
   const char *dir; /**< The directory path this store is scanning. */
};

/**
 * @internal
 * @brief Filesystem-specific store item data, extending Elm_Store_Item.
 */
struct _Elm_Store_Item_Filesystem
{
   Elm_Store_Item base; /**< Base Elm_Store_Item structure. */
   const char *path; /**< The file path for this item. */
};

static Elm_Genlist_Item_Class _store_item_class;

/**
 * @internal
 * @brief Trims the cache of realized but not live items.
 * This function is called to ensure that the number of cached items
 * (realized but not currently visible in the genlist) does not exceed
 * st->cache_max. It unfetches data for items that are removed from the cache.
 * @param st The store object.
 */
static void
_store_cache_trim(Elm_Store *st)
{
   while ((st->realized) &&
          (((int)eina_list_count(st->realized) - st->realized_count)
           > st->cache_max))
     {
        Elm_Store_Item *sti = st->realized->data;
        if (sti->realized)
          {
             st->realized = eina_list_remove_list(st->realized, st->realized);
             sti->realized = EINA_FALSE;
          }
        eina_lock_take(&sti->lock);
        if (!sti->fetched)
          {
             eina_lock_release(&sti->lock);
             ELM_SAFE_FREE(sti->fetch_th, ecore_thread_cancel);
             eina_lock_take(&sti->lock);
          }
        sti->fetched = EINA_FALSE;
//// let fetch/unfetch do the locking
//        eina_lock_release(&sti->lock);
        if (st->cb.unfetch.func)
          st->cb.unfetch.func(st->cb.unfetch.data, sti);
//        eina_lock_take(&sti->lock);
        sti->data = NULL;
        eina_lock_release(&sti->lock);
     }
}

/**
 * @internal
 * @brief Callback for when the target genlist is deleted.
 * This function cleans up the store, cancelling threads, freeing item lists,
 * and releasing resources associated with the genlist.
 * @param data The Elm_Store pointer.
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object being deleted (the genlist, unused).
 * @param event_info Event-specific information (unused).
 */
static void
_store_genlist_del(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Store *st = data;
   st->genlist = NULL;
   ELM_SAFE_FREE(st->list_th, ecore_thread_cancel);
   st->realized = eina_list_free(st->realized);
   while (st->items)
     {
        Elm_Store_Item *sti = (Elm_Store_Item *)st->items;
        ELM_SAFE_FREE(sti->eval_job, ecore_job_del);
        ELM_SAFE_FREE(sti->fetch_th, ecore_thread_cancel);
        if (sti->store->item.free) sti->store->item.free(sti);
        eina_lock_take(&sti->lock);
        if (sti->data)
          {
             if (st->cb.unfetch.func)
               st->cb.unfetch.func(st->cb.unfetch.data, sti);
             sti->data = NULL;
          }
        eina_lock_release(&sti->lock);
        eina_lock_free(&sti->lock);
        st->items = eina_inlist_remove(st->items, EINA_INLIST_GET(sti));
        free(sti);
     }
   // FIXME: kill threads and more
}

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
/* TODO: refactor lock part into core? this does not depend on filesystem part */
/**
 * @internal
 * @brief Performs the actual data fetching for a store item.
 * This function is executed in a separate thread (if enabled). It calls the
 * user-provided fetch callback (sti->store->cb.fetch.func) to load the item's data.
 * It handles locking to ensure thread-safe access to item data.
 * @param data The Elm_Store_Item pointer for which to fetch data.
 * @param th The Ecore_Thread executing this function (unused).
 */
static void
_store_filesystem_fetch_do(void *data, Ecore_Thread *th EINA_UNUSED)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        eina_lock_release(&sti->lock);
        return;
     }
   if (!sti->fetched)
     {
//// let fetch/unfetch do the locking
//        eina_lock_release(&sti->lock);
        if (sti->store->cb.fetch.func)
          sti->store->cb.fetch.func(sti->store->cb.fetch.data, sti);
//        eina_lock_take(&sti->lock);
        sti->fetched = EINA_TRUE;
     }
   eina_lock_release(&sti->lock);
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************
/* TODO: refactor lock part into core? this does not depend on filesystem part */
/**
 * @internal
 * @brief Callback executed in the main loop after item data fetching is complete.
 * This function is called when the _store_filesystem_fetch_do thread finishes.
 * It updates the genlist item if data was successfully fetched.
 * @param data The Elm_Store_Item pointer.
 * @param th The Ecore_Thread that finished.
 */
static void
_store_filesystem_fetch_end(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data) elm_genlist_item_update(sti->item);
   eina_lock_release(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
}

/* TODO: refactor lock part into core? this does not depend on filesystem part */
/**
 * @internal
 * @brief Callback executed in the main loop if item data fetching is cancelled.
 * This function is called if the _store_filesystem_fetch_do thread is cancelled.
 * It updates the genlist item if data was fetched before cancellation.
 * @param data The Elm_Store_Item pointer.
 * @param th The Ecore_Thread that was cancelled.
 */
static void
_store_filesystem_fetch_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (th == sti->fetch_th) sti->fetch_th = NULL;
   if (sti->data) elm_genlist_item_update(sti->item);
   eina_lock_release(&sti->lock);
}

/**
 * @internal
 * @brief Evaluates the state of a store item and triggers actions accordingly.
 * This function is typically scheduled as an Ecore_Job. It checks if the item's
 * 'live' state (visibility in genlist) has changed. If it became live,
 * it initiates data fetching. If it became non-live, it might cancel fetching
 * and trigger cache trimming.
 * @param data The Elm_Store_Item pointer to evaluate.
 */
static void
_store_item_eval(void *data)
{
   Elm_Store_Item *sti = data;
   sti->eval_job = NULL;
   if (sti->live == sti->was_live) return;
   sti->was_live = sti->live;
   if (sti->live)
     {
        _store_cache_trim(sti->store);
        if (sti->realized)
          sti->store->realized = eina_list_remove(sti->store->realized, sti);
        sti->store->realized = eina_list_append(sti->store->realized, sti);
        sti->realized = EINA_TRUE;
        if ((sti->store->fetch_thread) && (!sti->fetch_th))
          sti->fetch_th = ecore_thread_run(_store_filesystem_fetch_do,
                                           _store_filesystem_fetch_end,
                                           _store_filesystem_fetch_cancel,
                                           sti);
        else if ((!sti->store->fetch_thread))
          {
             _store_filesystem_fetch_do(sti, NULL);
             _store_filesystem_fetch_end(sti, NULL);
          }
     }
   else
     {
        ELM_SAFE_FREE(sti->fetch_th, ecore_thread_cancel);
        _store_cache_trim(sti->store);
     }
}

/**
 * @internal
 * @brief Callback for when a genlist item becomes realized (visible).
 * This function is triggered by the ELM_GENLIST_EVENT_REALIZED event.
 * It marks the corresponding Elm_Store_Item as 'live' and schedules
 * an evaluation job (_store_item_eval) to handle potential data fetching.
 * @param data The Elm_Store pointer.
 * @param event The Efl_Event data, where event->info is the Elm_Object_Item.
 */
static void
_store_genlist_item_realized(void *data, const Efl_Event *event)
{
   Elm_Store *st = data;
   Elm_Object_Item *gli = event->info;
   Elm_Store_Item *sti = elm_object_item_data_get(gli);
   if (!sti) return;
   st->realized_count++;
   sti->live = EINA_TRUE;
   ecore_job_del(sti->eval_job);
   sti->eval_job = ecore_job_add(_store_item_eval, sti);
}

/**
 * @internal
 * @brief Callback for when a genlist item becomes unrealized (not visible).
 * This function is triggered by the ELM_GENLIST_EVENT_UNREALIZED event.
 * It marks the corresponding Elm_Store_Item as not 'live' and schedules
 * an evaluation job (_store_item_eval) to handle potential cache management.
 * @param data The Elm_Store pointer.
 * @param event The Efl_Event data, where event->info is the Elm_Object_Item.
 */
static void
_store_genlist_item_unrealized(void *data, const Efl_Event *event)
{
   Elm_Store *st = data;
   Elm_Object_Item *gli = event->info;
   Elm_Store_Item *sti = elm_object_item_data_get(gli);
   if (!sti) return;
   st->realized_count--;
   sti->live = EINA_FALSE;
   ecore_job_del(sti->eval_job);
   sti->eval_job = ecore_job_add(_store_item_eval, sti);
}

/**
 * @internal
 * @brief Finds the mapping rule for a specific part of a store item.
 * Iterates through the item's mappings to find the one that matches
 * the given 'part' name (e.g., "elm.text", "elm.swallow.icon").
 * @param sti The store item.
 * @param part The name of the genlist item part to find the mapping for.
 * @return A pointer to the matching Elm_Store_Item_Mapping, or NULL if not found.
 */
static const Elm_Store_Item_Mapping *
_store_item_mapping_find(Elm_Store_Item *sti, const char *part)
{
   const Elm_Store_Item_Mapping *m;

   for (m = sti->mapping; m; m ++)
     {
        if (m->type == ELM_STORE_ITEM_MAPPING_NONE) break;
        if (!strcmp(part, m->part)) return m;
     }
   return NULL;
}

/**
 * @internal
 * @brief Genlist item class callback to get text for a part.
 * This function is called by the genlist to retrieve text for a specific part
 * of an item (e.g., a label). It uses the item's mapping rules to find
 * the data source for the text.
 * @param data The Elm_Store_Item pointer.
 * @param obj The genlist Evas_Object (unused).
 * @param part The name of the text part (e.g., "elm.text").
 * @return A newly allocated string containing the text, or NULL. The caller
 *         is responsible for freeing this string.
 */
static char *
_store_item_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part)
{
   Elm_Store_Item *sti = data;
   const char *s = "";
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        const Elm_Store_Item_Mapping *m = _store_item_mapping_find(sti, part);
        if (m)
          {
             switch (m->type)
               {
                case ELM_STORE_ITEM_MAPPING_LABEL:
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   break;
                case ELM_STORE_ITEM_MAPPING_CUSTOM:
                   if (m->details.custom.func)
                     s = m->details.custom.func(sti->data, sti, part);
                   break;
                default:
                   break;
               }
          }
     }
   eina_lock_release(&sti->lock);
   return s ? strdup(s) : NULL;
}

/**
 * @internal
 * @brief Genlist item class callback to get content (Evas_Object) for a part.
 * This function is called by the genlist to retrieve an Evas_Object (e.g., an icon)
 * for a specific swallow part of an item. It uses the item's mapping rules
 * to determine how to create and configure the content object.
 * @param data The Elm_Store_Item pointer.
 * @param obj The parent Evas_Object (the genlist item).
 * @param part The name of the swallow part (e.g., "elm.swallow.icon").
 * @return A new Evas_Object for the content, or NULL.
 */
static Evas_Object *
_store_item_content_get(void *data, Evas_Object *obj, const char *part)
{
   Elm_Store_Item *sti = data;
   eina_lock_take(&sti->lock);
   if (sti->data)
     {
        const Elm_Store_Item_Mapping *m = _store_item_mapping_find(sti, part);
        if (m)
          {
             Evas_Object *ic = NULL;
             const char *s = NULL;

             switch (m->type)
               {
                case ELM_STORE_ITEM_MAPPING_ICON:
                   ic = elm_icon_add(obj);
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   evas_object_size_hint_aspect_set(ic,
                                                    EVAS_ASPECT_CONTROL_VERTICAL,
                                                    m->details.icon.w,
                                                    m->details.icon.h);
                   elm_image_smooth_set(ic, m->details.icon.smooth);
                   elm_image_no_scale_set(ic, m->details.icon.no_scale);
                   elm_image_resizable_set(ic,
                                      m->details.icon.scale_up,
                                      m->details.icon.scale_down);
                   if (s)
                     {
                        if (m->details.icon.standard_name)
                          elm_icon_standard_set(ic, s);
                        else
                          elm_image_file_set(ic, s, NULL);
                     }
                   break;
                case ELM_STORE_ITEM_MAPPING_PHOTO:
                   ic = elm_icon_add(obj);
                   s = *(char **)(((unsigned char *)sti->data) + m->offset);
                   elm_photo_size_set(ic, m->details.photo.size);
                   if (s)
                     elm_photo_file_set(ic, s);
                   break;
                case ELM_STORE_ITEM_MAPPING_CUSTOM:
                   if (m->details.custom.func)
                     ic = m->details.custom.func(sti->data, sti, part);
                   break;
                default:
                   break;
               }
             eina_lock_release(&sti->lock);
             return ic;
          }
     }
   eina_lock_release(&sti->lock);
   return NULL;
}

/**
 * @internal
 * @brief Genlist item class callback for item deletion.
 * This function is called when a genlist item is being deleted.
 * Currently, it's a no-op as item resource cleanup is handled elsewhere
 * by the store logic (e.g., in _store_genlist_del or unfetch callbacks).
 * @param data The Elm_Store_Item pointer (unused).
 * @param obj The genlist Evas_Object (unused).
 */
static void
_store_item_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{
}

////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
/**
 * @internal
 * @brief Comparison function for sorting Elm_Store_Item_Info structures.
 * This function is used by eina_list_sort to sort items based on their
 * `sort_id` strings, using `strcoll` for locale-aware comparison.
 * It is executed within the listing thread.
 * @param d1 Pointer to the first Elm_Store_Item_Info.
 * @param d2 Pointer to the second Elm_Store_Item_Info.
 * @return An integer less than, equal to, or greater than zero if d1 is found,
 *         respectively, to be less than, to match, or be greater than d2.
 *         Returns 0 if either sort_id is NULL.
 */
static int
_store_filesystem_sort_cb(void *d1, void *d2)
{
   Elm_Store_Item_Info *info1 = d1, *info2 = d2;
   if ((!info1->sort_id) || (!info2->sort_id)) return 0;
   return strcoll(info1->sort_id, info2->sort_id);
}

/**
 * @internal
 * @brief Performs the directory listing for a filesystem store.
 * This function is executed in a separate thread (st->base.list_th).
 * It iterates over files in the specified directory (st->dir), calls the
 * user-provided list callback (st->base.cb.list.func) for each file to
 * filter and gather information (like sort_id and item class).
 * If sorting is enabled, it collects all items and sorts them before
 * feeding them back to the main thread. Otherwise, items are fed back
 * as they are processed.
 * @param data The Elm_Store_Filesystem pointer.
 * @param th The Ecore_Thread executing this function.
 */
static void
_store_filesystem_list_do(void *data, Ecore_Thread *th EINA_UNUSED)
{
   Elm_Store_Filesystem *st = data;
   Eina_Iterator *it;
   const Eina_File_Direct_Info *finf;
   Eina_List *sorted = NULL;
   Elm_Store_Item_Info_Filesystem *info;

   // FIXME: need a way to abstract the open, list, feed items from list
   // and maybe get initial sortable key vals etc.
   it = eina_file_stat_ls(st->dir);
   if (!it) return;
   EINA_ITERATOR_FOREACH(it, finf)
     {
        Eina_Bool ok;
        size_t pathsz = finf->path_length + 1;

        if (finf->path[finf->name_start] == '.') continue ;

        info = calloc(1, sizeof(Elm_Store_Item_Info_Filesystem) + pathsz);
        if (!info) continue;
        info->path = ((char *)info) + sizeof(Elm_Store_Item_Info_Filesystem);
        memcpy(info->path, finf->path, pathsz);
        ok = EINA_TRUE;
        if (st->base.cb.list.func)
          ok = st->base.cb.list.func(st->base.cb.list.data, &info->base);
        if (ok)
          {
             if (!st->base.sorted) ecore_thread_feedback(th, info);
             else sorted = eina_list_append(sorted, info);
          }
        else
          {
             free(info->base.sort_id);
             free(info);
          }
        if (ecore_thread_check(th)) break;
     }
   eina_iterator_free(it);
   if (sorted)
     {
        sorted = eina_list_sort(sorted, 0,
                                EINA_COMPARE_CB(_store_filesystem_sort_cb));
        EINA_LIST_FREE(sorted, info)
          {
             if (!ecore_thread_check(th)) ecore_thread_feedback(th, info);
          }
     }
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************

/**
 * @internal
 * @brief Callback executed in the main loop when the listing thread finishes normally.
 * Clears the store's reference to the listing thread.
 * @param data The Elm_Store pointer.
 * @param th The Ecore_Thread that finished.
 */
static void
_store_filesystem_list_end(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

/**
 * @internal
 * @brief Callback executed in the main loop if the listing thread is cancelled.
 * Clears the store's reference to the listing thread.
 * @param data The Elm_Store pointer.
 * @param th The Ecore_Thread that was cancelled.
 */
static void
_store_filesystem_list_cancel(void *data, Ecore_Thread *th)
{
   Elm_Store *st = data;
   if (th == st->list_th) st->list_th = NULL;
}

/**
 * @internal
 * @brief Callback executed in the main loop to process a listed item.
 * This function is called via ecore_thread_feedback from the listing thread
 * (_store_filesystem_list_do) for each item that should be added to the store.
 * It creates an Elm_Store_Item_Filesystem, initializes it with the provided
 * info, and appends it to the target genlist.
 * @param data The Elm_Store pointer.
 * @param th The Ecore_Thread providing feedback (unused).
 * @param msg The Elm_Store_Item_Info_Filesystem pointer containing item details.
 */
static void
_store_filesystem_list_update(void *data, Ecore_Thread *th EINA_UNUSED, void *msg)
{
   Elm_Store *st = data;
   Elm_Store_Item_Filesystem *sti;
   Elm_Genlist_Item_Class *itc;
   Elm_Store_Item_Info_Filesystem *info = msg;

   sti = calloc(1, sizeof(Elm_Store_Item_Filesystem));
   if (!sti) goto done;
   eina_lock_new(&sti->base.lock);
   EINA_MAGIC_SET(&(sti->base), ELM_STORE_ITEM_MAGIC);
   sti->base.store = st;
   sti->base.data = info->base.data;
   sti->base.mapping = info->base.mapping;
   sti->path = eina_stringshare_add(info->path);

   itc = info->base.item_class;
   if (!itc) itc = &_store_item_class;
   else
     {
        itc->func.text_get = _store_item_text_get;
        itc->func.content_get  = _store_item_content_get;
        itc->func.state_get = NULL; // FIXME: support state gets later
        itc->func.del       = _store_item_del;
     }

   // FIXME: handle being a parent (tree)
   sti->base.item = elm_genlist_item_append(st->genlist, itc,
                                            sti/* item data */,
                                            NULL/* parent */,
                                            ELM_GENLIST_ITEM_NONE,
                                            NULL/* func */,
                                            NULL/* func data */);
   st->items = eina_inlist_append(st->items, (Eina_Inlist *)sti);
done:
   free(info->base.sort_id);
   free(info);
}

// public api calls
/**
 * @internal
 * @brief Allocates and initializes a new base Elm_Store object.
 * This is a helper function for creating store instances of various types.
 * It allocates memory of the given 'size', initializes magic numbers,
 * sets up a default genlist item class, and default store properties.
 * @param size The size of the store structure to allocate (e.g., sizeof(Elm_Store_Filesystem)).
 * @return A pointer to the newly allocated Elm_Store, or NULL on failure.
 */
static Elm_Store *
_elm_store_new(size_t size)
{
   Elm_Store *st = calloc(1, size);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);

   // TODO: BEGIN - move to elm_store_init()
   eina_magic_string_set(ELM_STORE_MAGIC, "Elm_Store");
   eina_magic_string_set(ELM_STORE_FILESYSTEM_MAGIC, "Elm_Store_Filesystem");
   eina_magic_string_set(ELM_STORE_ITEM_MAGIC, "Elm_Store_Item");
   // setup default item class (always the same) if list cb doesn't provide one
   _store_item_class.item_style = "default";
   _store_item_class.func.text_get = _store_item_text_get;
   _store_item_class.func.content_get  = _store_item_content_get;
   _store_item_class.func.state_get = NULL; // FIXME: support state gets later
   _store_item_class.func.del       = _store_item_del;
   // TODO: END - move to elm_store_init()

   EINA_MAGIC_SET(st, ELM_STORE_MAGIC);
   st->cache_max = 128;
   st->fetch_thread = EINA_TRUE;
   return st;
}
#define elm_store_new(type) (type*)_elm_store_new(sizeof(type))

/**
 * @internal
 * @brief Custom free function for Elm_Store_Filesystem.
 * This function is assigned to `store->free` for filesystem stores.
 * It frees filesystem-specific resources, like the directory path string.
 * @param store The Elm_Store (castable to Elm_Store_Filesystem) to free.
 */
static void
_elm_store_filesystem_free(Elm_Store *store)
{
   Elm_Store_Filesystem *st = (Elm_Store_Filesystem *)store;
   eina_stringshare_del(st->dir);
}

/**
 * @internal
 * @brief Custom free function for Elm_Store_Item_Filesystem.
 * This function is assigned to `store->item.free` for filesystem stores.
 * It frees filesystem-specific resources for an item, like its path string.
 * @param item The Elm_Store_Item (castable to Elm_Store_Item_Filesystem) to free.
 */
static void
_elm_store_filesystem_item_free(Elm_Store_Item *item)
{
   Elm_Store_Item_Filesystem *sti = (Elm_Store_Item_Filesystem *)item;
   eina_stringshare_del(sti->path);
}

EAPI Elm_Store *
elm_store_filesystem_new(void)
{
   Elm_Store_Filesystem *st = elm_store_new(Elm_Store_Filesystem);
   EINA_SAFETY_ON_NULL_RETURN_VAL(st, NULL);

   EINA_MAGIC_SET(st, ELM_STORE_FILESYSTEM_MAGIC);
   st->base.free = _elm_store_filesystem_free;
   st->base.item.free = _elm_store_filesystem_item_free;

   return &st->base;
}

EAPI void
elm_store_free(Elm_Store *st)
{
   void (*item_free)(Elm_Store_Item *);
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   ELM_SAFE_FREE(st->list_th, ecore_thread_cancel);
   st->realized = eina_list_free(st->realized);
   item_free = st->item.free;
   while (st->items)
     {
        Elm_Store_Item *sti = (Elm_Store_Item *)st->items;
        ELM_SAFE_FREE(sti->eval_job, ecore_job_del);
        ELM_SAFE_FREE(sti->fetch_th, ecore_thread_cancel);
        if (item_free) item_free(sti);
        eina_lock_take(&sti->lock);
        if (sti->data)
          {
             if (st->cb.unfetch.func)
               st->cb.unfetch.func(st->cb.unfetch.data, sti);
             sti->data = NULL;
          }
        eina_lock_release(&sti->lock);
        eina_lock_free(&sti->lock);
        st->items = eina_inlist_remove(st->items, EINA_INLIST_GET(sti));
        free(sti);
     }
   if (st->genlist)
     {
        evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
        efl_event_callback_del(st->genlist, ELM_GENLIST_EVENT_REALIZED, _store_genlist_item_realized, st);
        efl_event_callback_del(st->genlist, ELM_GENLIST_EVENT_UNREALIZED, _store_genlist_item_unrealized, st);
        elm_genlist_clear(st->genlist);
        st->genlist = NULL;
     }
   if (st->free) st->free(st);
   free(st);
}

EAPI void
elm_store_target_genlist_set(Elm_Store *st, Evas_Object *obj)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (st->genlist == obj) return;
   if (st->genlist)
     {
        evas_object_event_callback_del_full(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
        efl_event_callback_del(st->genlist, ELM_GENLIST_EVENT_REALIZED, _store_genlist_item_realized, st);
        efl_event_callback_del(st->genlist, ELM_GENLIST_EVENT_UNREALIZED, _store_genlist_item_unrealized, st);
        elm_genlist_clear(st->genlist);
     }
   st->genlist = obj;
   if (!st->genlist) return;
   efl_event_callback_add(st->genlist, ELM_GENLIST_EVENT_REALIZED, _store_genlist_item_realized, st);
   efl_event_callback_add(st->genlist, ELM_GENLIST_EVENT_UNREALIZED, _store_genlist_item_unrealized, st);
   evas_object_event_callback_add(st->genlist, EVAS_CALLBACK_DEL, _store_genlist_del, st);
   elm_genlist_clear(st->genlist);
}

EAPI void
elm_store_filesystem_directory_set(Elm_Store *store, const char *dir)
{
   Elm_Store_Filesystem *st = (Elm_Store_Filesystem *)store;
   if (!EINA_MAGIC_CHECK(store, ELM_STORE_MAGIC)) return;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return;
   ELM_SAFE_FREE(store->list_th, ecore_thread_cancel);
   if (!eina_stringshare_replace(&st->dir, dir)) return;
   store->list_th = ecore_thread_feedback_run(_store_filesystem_list_do,
                                              _store_filesystem_list_update,
                                              _store_filesystem_list_end,
                                              _store_filesystem_list_cancel,
                                              st, EINA_TRUE);
}

EAPI const char *
elm_store_filesystem_directory_get(const Elm_Store *store)
{
   const Elm_Store_Filesystem *st = (const Elm_Store_Filesystem *)store;
   if (!EINA_MAGIC_CHECK(store, ELM_STORE_MAGIC)) return NULL;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return NULL;
   return st->dir;
}

EAPI void
elm_store_cache_set(Elm_Store *st, int max)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   if (max < 0) max = 0;
   st->cache_max = max;
   _store_cache_trim(st);
}

EAPI int
elm_store_cache_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return 0;
   return st->cache_max;
}

EAPI void
elm_store_list_func_set(Elm_Store *st, Elm_Store_Item_List_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.list.func = func;
   st->cb.list.data = (void *)data;
}

EAPI void
elm_store_fetch_func_set(Elm_Store *st, Elm_Store_Item_Fetch_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.fetch.func = func;
   st->cb.fetch.data = (void *)data;
}

EAPI void
elm_store_fetch_thread_set(Elm_Store *st, Eina_Bool use_thread)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->fetch_thread = !!use_thread;
}

EAPI Eina_Bool
elm_store_fetch_thread_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return EINA_FALSE;
   return st->fetch_thread;
}

EAPI void
elm_store_unfetch_func_set(Elm_Store *st, Elm_Store_Item_Unfetch_Cb func, const void *data)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->cb.unfetch.func = func;
   st->cb.unfetch.data = (void *)data;
}

EAPI void
elm_store_sorted_set(Elm_Store *st, Eina_Bool sorted)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return;
   st->sorted = sorted;
}

EAPI Eina_Bool
elm_store_sorted_get(const Elm_Store *st)
{
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_MAGIC)) return EINA_FALSE;
   return st->sorted;
}

EAPI void
elm_store_item_data_set(Elm_Store_Item *sti, void *data)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return;
//// let fetch/unfetch do the locking
//   eina_lock_take(&sti->lock);
   sti->data = data;
//   eina_lock_release(&sti->lock);
}

EAPI void *
elm_store_item_data_get(Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   void *d;
//// let fetch/unfetch do the locking
//   eina_lock_take(&sti->lock);
   d = sti->data;
//   eina_lock_release(&sti->lock);
   return d;
}

EAPI const Elm_Store *
elm_store_item_store_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   // dont need lock
   return sti->store;
}

EAPI const Elm_Object_Item *
elm_store_item_genlist_item_get(const Elm_Store_Item *sti)
{
   if (!EINA_MAGIC_CHECK(sti, ELM_STORE_ITEM_MAGIC)) return NULL;
   // dont need lock
   return sti->item;
}

EAPI const char *
elm_store_item_filesystem_path_get(const Elm_Store_Item *item)
{
   Elm_Store_Item_Filesystem *sti = (Elm_Store_Item_Filesystem *)item;
   Elm_Store_Filesystem *st;
   if (!EINA_MAGIC_CHECK(item, ELM_STORE_ITEM_MAGIC)) return NULL;
   if (!EINA_MAGIC_CHECK(item->store, ELM_STORE_MAGIC)) return NULL;
   /* ensure we're dealing with filesystem item */
   st = (Elm_Store_Filesystem *)item->store;
   if (!EINA_MAGIC_CHECK(st, ELM_STORE_FILESYSTEM_MAGIC)) return NULL;
   // dont need lock
   return sti->path;
}
