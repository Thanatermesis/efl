#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_UI_FACTORY_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Efl_Ui_Caching_Factory_Data Efl_Ui_Caching_Factory_Data;
typedef struct _Efl_Ui_Caching_Factory_Request Efl_Ui_Caching_Factory_Request;
typedef struct _Efl_Ui_Caching_Factory_Group_Request Efl_Ui_Caching_Factory_Group_Request;

/**
 * @brief Internal data structure for Efl_Ui_Caching_Factory.
 *
 * This structure holds all the necessary data for managing cached UI elements,
 * including the cache itself, lookup tables, limits, and current usage statistics.
 */
struct _Efl_Ui_Caching_Factory_Data
{
   const Efl_Class *klass; /**< The class of items to create if no style is used or if items are not widgets. */

   Eina_Stringshare *style; /**< The style property to use for item differentiation if lookup is enabled. */

   // Simple list of ready-to-use objects. They are all equal so it does not matter from which
   // end of the list objects are added and removed.
   Eina_List *cache;       /**< List of cached UI elements ready for reuse. */
   Eina_Hash *lookup;      /**< Hash table for quick lookup of cached items by style, if pd->style is set. */
   Eina_Future *flush;     /**< Future for scheduling cache flush operations. */

   struct {
      unsigned int memory; /**< Maximum memory usage allowed for the cache. */
      unsigned int items;  /**< Maximum number of items allowed in the cache. */
   } limit;                /**< Cache limits. */

   struct {
      unsigned int memory; /**< Current memory usage of the cache. */
      unsigned int items;  /**< Current number of items in the cache. */
   } current;              /**< Current cache usage. */

   Eina_Bool invalidated : 1; /**< Flag indicating if the factory has been invalidated. */
};

/**
 * @brief Represents a request to create a single UI element.
 *
 * This structure holds data related to a pending creation request for one item,
 * including references to the factory data, the factory itself, and the parent object.
 */
struct _Efl_Ui_Caching_Factory_Request
{
   Efl_Ui_Caching_Factory_Data *pd; /**< Pointer to the factory's private data. */

   Efl_Ui_Caching_Factory *factory; /**< Reference to the caching factory. */
   Eo *parent;                      /**< Parent Eo object for the created item (unused in current logic, but kept for potential future use or consistency). */
};

/**
 * @brief Represents a request to create a group of UI elements.
 *
 * This structure is used when multiple items are requested at once, primarily
 * to aggregate the results.
 */
struct _Efl_Ui_Caching_Factory_Group_Request
{
   Eina_Value done; /**< An Eina_Value array (EINA_VALUE_TYPE_OBJECT) to store the created/retrieved Eo objects. */
};

/**
 * @brief Removes an entity from the cache and updates current counts.
 *
 * @param pd The private data of the caching factory.
 * @param l The list node in pd->cache pointing to the entity to remove.
 * @param entity The entity to remove from the cache.
 */
static void
_efl_ui_caching_factory_remove(Efl_Ui_Caching_Factory_Data *pd, Eina_List *l, Efl_Gfx_Entity *entity)
{
   pd->cache = eina_list_remove_list(pd->cache, l);
   pd->current.items--;

   pd->current.memory -= efl_class_memory_size_get(entity);
   if (efl_isa(entity, EFL_CACHED_ITEM_INTERFACE))
     pd->current.memory -= efl_cached_item_memory_size_get(entity);
}

/**
 * @brief Deletes or releases a collection of UI entities.
 *
 * If `pd->klass` is not set (meaning items are created by a parent factory),
 * it releases the items using `efl_ui_factory_release`. Otherwise, it directly
 * deletes the items using `efl_del`.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 * @param entities An iterator over the entities to be deleted or released.
 */
static void
_efl_ui_caching_factory_item_del(Eo *obj, Efl_Ui_Caching_Factory_Data *pd,
                                 Eina_Iterator *entities)
{
   if (!pd->klass)
     {
        efl_ui_factory_release(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS), entities);
     }
   else
     {
        Efl_Gfx_Entity *entity;

        EINA_ITERATOR_FOREACH(entities, entity)
          efl_del(entity);
        eina_iterator_free(entities);
     }
}

/**
 * @brief Flushes the cache to meet defined memory and item limits.
 *
 * Items are removed from the end of the cache (least recently added back)
 * until both memory and item count limits are satisfied.
 * Removed items are scheduled for deletion.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 */
static void
_efl_ui_caching_factory_flush(Eo *obj, Efl_Ui_Caching_Factory_Data *pd)
{
   Eina_Array scheduled;

   eina_array_step_set(&scheduled, sizeof (Eina_Array), 8);

   while (pd->limit.items != 0 &&
          pd->current.items > pd->limit.items)
     {
        Efl_Gfx_Entity *entity;

        entity = eina_list_data_get(eina_list_last(pd->cache));

        _efl_ui_caching_factory_remove(pd, eina_list_last(pd->cache), entity);
        if (pd->lookup) eina_hash_del(pd->lookup, efl_ui_widget_style_get(entity), entity);

        eina_array_push(&scheduled, entity);
     }

   while (pd->limit.memory != 0 &&
          pd->current.memory > pd->limit.memory)
     {
        Efl_Gfx_Entity *entity;

        entity = eina_list_data_get(eina_list_last(pd->cache));

        _efl_ui_caching_factory_remove(pd, eina_list_last(pd->cache), entity);
        if (pd->lookup) eina_hash_del(pd->lookup, efl_ui_widget_style_get(entity), entity);

        eina_array_push(&scheduled, entity);
     }

   // We could improve this by doing some limited batch to reduce potential spike usage
   _efl_ui_caching_factory_item_del(obj, pd, eina_array_iterator_new(&scheduled));
   eina_array_flush(&scheduled);
}

static Eina_Value
_efl_ui_caching_factory_uncap_then(Eo *model EINA_UNUSED,
                                   void *data EINA_UNUSED,
                                   const Eina_Value v)
{
   Efl_Ui_Widget *widget = NULL;

   if (eina_value_array_count(&v) != 1) return eina_value_error_init(EINVAL);

   eina_value_array_get(&v, 0, &widget);

   return eina_value_object_init(widget);
}

/**
 * @brief Future callback for style-based item creation.
 *
 * This function is called when the style property of a model is ready.
 * It attempts to find an item with the resolved style in the cache.
 * If not found, it triggers the creation of a new item via the parent factory.
 *
 * @param model The model for which the item is being created.
 * @param data A pointer to Efl_Ui_Caching_Factory_Request.
 * @param v An Eina_Value containing the style string.
 * @return An Eina_Value containing the created/retrieved widget as an Eo object,
 *         or an Eina_Value future if a new item needs to be created asynchronously,
 *         or an error Eina_Value.
 *         Example of successful Eina_Value (object): `{ EINA_VALUE_TYPE_OBJECT, { .object = (Eo*)widget } }`
 *         Example of successful Eina_Value (future): `{ EINA_VALUE_TYPE_OBJECT, { .object = (Eo*)future } }` (where future is an Eina_Future)
 */
static Eina_Value
_efl_ui_caching_factory_create_then(Eo *model, void *data, const Eina_Value v)
{
   Efl_Ui_Caching_Factory_Request *r = data;
   Efl_Ui_Widget *w;
   const char *style = NULL;

   if (!eina_value_string_get(&v, &style))
     return eina_value_error_init(EFL_MODEL_ERROR_NOT_SUPPORTED);

   w = eina_hash_find(r->pd->lookup, style);
   if (!w)
     {
        Eina_Future *f;
        Eo *models[1] = { model };

        // No object of that style in the cache, need to create a new one
        // This is not ideal, we would want to gather all the request in one swoop here,
        // left for later improvement.
        f = efl_ui_factory_create(efl_super(r->factory, EFL_UI_CACHING_FACTORY_CLASS),
                                  EINA_C_ARRAY_ITERATOR_NEW(models));
        f = efl_future_then(r->factory, f,
                            .success = _efl_ui_caching_factory_uncap_then,
                            .success_type = EINA_VALUE_TYPE_ARRAY);
        return eina_future_as_value(f);
     }

   eina_hash_del(r->pd->lookup, style, w);
   _efl_ui_caching_factory_remove(r->pd, eina_list_data_find(r->pd->cache, w), w);

   efl_ui_view_model_set(w, model);
   efl_event_callback_call(r->factory, EFL_UI_FACTORY_EVENT_ITEM_BUILDING, w);

   return eina_value_object_init(w);
}

/**
 * @brief Cleans up resources associated with an Efl_Ui_Caching_Factory_Request.
 *
 * This is typically used as a 'free' callback for futures.
 *
 * @param o The object associated with the future (unused).
 * @param data A pointer to Efl_Ui_Caching_Factory_Request to be freed.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_ui_caching_factory_cleanup(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Ui_Caching_Factory_Request *r = data;

   efl_unref(r->factory);
   efl_unref(r->parent);
   free(r);
}

/**
 * @brief Future callback for group item creation.
 *
 * This function is called when a batch of items has been created by the parent factory.
 * It appends the newly created widgets to the `done` array in the group request.
 *
 * @param obj The caching factory object (unused).
 * @param data A pointer to Efl_Ui_Caching_Factory_Group_Request.
 * @param v An Eina_Value of type EINA_VALUE_TYPE_ARRAY, where each element is an Eo* widget.
 *          Example: `{ EINA_VALUE_TYPE_ARRAY, { .array = { .subtype = EINA_VALUE_TYPE_OBJECT, .count = N, .value = (Eina_Value_Array_Value[]) { { .object = widget1 }, ..., { .object = widgetN } } } } }`
 * @return A reference copy of the `gr->done` Eina_Value array.
 */
static Eina_Value
_efl_ui_caching_factory_group_create_then(Eo *obj EINA_UNUSED,
                                          void *data,
                                          const Eina_Value v)
{
   Efl_Ui_Caching_Factory_Group_Request *gr = data;
   int len, i;
   Efl_Ui_Widget *widget;

   EINA_VALUE_ARRAY_FOREACH(&v, len, i, widget)
     eina_value_array_append(&gr->done, widget);

   return eina_value_reference_copy(&gr->done);
}

/**
 * @brief Cleans up resources associated with an Efl_Ui_Caching_Factory_Group_Request.
 *
 * This is typically used as a 'free' callback for futures.
 *
 * @param o The object associated with the future (unused).
 * @param data A pointer to Efl_Ui_Caching_Factory_Group_Request to be freed.
 * @param dead_future The future that has completed (unused).
 */
static void
_efl_ui_caching_factory_group_cleanup(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Ui_Caching_Factory_Group_Request *gr = data;

   eina_value_flush(&gr->done);
   free(gr);
}

/**
 * @brief Implements Efl_Ui_Factory.create.
 *
 * Creates UI elements based on the provided models.
 *
 * If `pd->cache` is available, `pd->style` is set, and `pd->klass` is not set (style-based caching):
 *   - It attempts to retrieve items from the cache based on their style (resolved from model property `pd->style`).
 *   - If an item of a specific style is not in cache, it requests creation from the parent factory.
 *   - This path involves asynchronous operations using futures to get model properties.
 *
 * Otherwise (class-based caching or no caching if `pd->cache` is NULL):
 *   - It first tries to fulfill requests from `pd->cache` (if available, without style matching).
 *   - If `pd->klass` is set, it creates new items of that class directly.
 *   - If `pd->klass` is not set and cache is exhausted, it delegates creation to the parent factory.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 * @param models An iterator over Efl_Model objects for which to create UI elements.
 * @return A future that resolves to an Eina_Value array of created Eo* objects.
 *         The Eina_Value array structure: `{ EINA_VALUE_TYPE_ARRAY, { .subtype = EINA_VALUE_TYPE_OBJECT, .count = N, .value = (Eina_Value_Array_Value[]) { { .object = item1 }, ..., { .object = itemN } } } }`
 */
static Eina_Future *
_efl_ui_caching_factory_efl_ui_factory_create(Eo *obj,
                                              Efl_Ui_Caching_Factory_Data *pd,
                                              Eina_Iterator *models)
{
   Efl_Ui_Caching_Factory_Request *r = NULL;
   Efl_Ui_Caching_Factory_Group_Request *gr = NULL;
   Efl_Gfx_Entity *w = NULL;
   Efl_Model *model = NULL;
   Eina_Future *f = NULL;

   if (pd->cache && pd->style && !pd->klass)
     {
        Eina_Future **all = NULL;
        int count = 0;

        r = calloc(1, sizeof (Efl_Ui_Caching_Factory_Request));
        if (!r) return efl_loop_future_rejected(obj, ENOMEM);

        r->pd = pd;
        r->factory = efl_ref(obj);

        all = calloc(1, sizeof (Eina_Future *));
        if (!all) goto alloc_array_error;

        EINA_ITERATOR_FOREACH(models, model)
          {
             all[count++] = efl_future_then(model,
                                            efl_model_property_ready_get(model, pd->style),
                                            .success = _efl_ui_caching_factory_create_then,
                                            .data = r);

             Eina_Future **tmp = realloc(all, (count + 1) * sizeof (Eina_Future *));
             if (!tmp)
               {
                 free(all);
                 goto alloc_array_error;
               }
             all = tmp;
          }
        eina_iterator_free(models);

        all[count] = EINA_FUTURE_SENTINEL;

        return efl_future_then(obj, eina_future_all_array(all),
                               .data = r,
                               .free = _efl_ui_caching_factory_cleanup);
     }

   gr = calloc(1, sizeof (Efl_Ui_Caching_Factory_Group_Request));
   if (!gr) return efl_loop_future_rejected(obj, ENOMEM);

   eina_value_array_setup(&gr->done, EINA_VALUE_TYPE_OBJECT, 4);

   // First get as much object from the cache as possible
   if (pd->cache)
     EINA_ITERATOR_FOREACH(models, model)
       {
          w = eina_list_data_get(pd->cache);
          _efl_ui_caching_factory_remove(pd, pd->cache, w);

          efl_ui_view_model_set(w, model);

          eina_value_array_append(&gr->done, w);

          if (!pd->cache) break;
       }

   // Now create object on the fly that are missing from the cache
   if (pd->klass)
     {
        Efl_Ui_Widget *widget = efl_ui_widget_factory_widget_get(obj);

        EINA_ITERATOR_FOREACH(models, model)
          {
             w = efl_add(pd->klass, widget,
                         efl_ui_view_model_set(efl_added, model),
                         efl_event_callback_call(obj, EFL_UI_FACTORY_EVENT_ITEM_CONSTRUCTING, efl_added));
             efl_event_callback_call(obj, EFL_UI_FACTORY_EVENT_ITEM_BUILDING, w);
             eina_value_array_append(&gr->done, w);
          }

        f = efl_loop_future_resolved(obj, gr->done);

        eina_value_flush(&gr->done);
        free(gr);

        return f;
     }

   f = efl_ui_factory_create(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS), models);
   return efl_future_then(obj, f,
                          .success = _efl_ui_caching_factory_group_create_then,
                          .success_type = EINA_VALUE_TYPE_ARRAY,
                          .data = gr,
                          .free = _efl_ui_caching_factory_group_cleanup);

alloc_array_error:
   efl_unref(r->parent);
   efl_unref(r->factory);
   free(r);
   eina_iterator_free(models);
   return efl_loop_future_rejected(obj, ENOMEM);
}

static void
_efl_ui_caching_factory_efl_ui_widget_factory_item_class_set(Eo *obj,
                                                             Efl_Ui_Caching_Factory_Data *pd,
                                                             const Efl_Object *klass)
{
   // If a class is provided that is a view but not a widget,
   // we store it in pd->klass. This allows caching non-widget items
   // that still implement Gfx_Entity and Ui_View.
   // Otherwise, the class is passed to the parent factory.
   if (efl_isa(klass, EFL_UI_VIEW_INTERFACE) &&
       !efl_isa(klass, EFL_UI_WIDGET_CLASS))
     {
        if (!efl_isa(klass, EFL_GFX_ENTITY_INTERFACE) ||
            !efl_isa(klass, EFL_UI_VIEW_INTERFACE))
          {
             ERR("Provided class '%s' for factory '%s' doesn't implement '%s' and '%s' interfaces nor '%s' and '%s' interfaces.",
                 efl_class_name_get(klass),
                 efl_class_name_get(obj),
                 efl_class_name_get(EFL_GFX_ENTITY_INTERFACE),
                 efl_class_name_get(EFL_UI_VIEW_INTERFACE),
                 efl_class_name_get(EFL_UI_WIDGET_CLASS),
                 efl_class_name_get(EFL_UI_VIEW_INTERFACE));
             return ;
          }
        pd->klass = klass;
        return;
     }
   efl_ui_widget_factory_item_class_set(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS), klass);
}

static const Efl_Object *
_efl_ui_caching_factory_efl_ui_widget_factory_item_class_get(const Eo *obj,
                                                             Efl_Ui_Caching_Factory_Data *pd)
{
   if (pd->klass) return pd->klass;
   return efl_ui_widget_factory_item_class_get(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS));
}

static void
_efl_ui_caching_factory_memory_limit_set(Eo *obj,
                                         Efl_Ui_Caching_Factory_Data *pd,
                                         unsigned int limit)
{
   pd->limit.memory = limit;

   _efl_ui_caching_factory_flush(obj, pd);
}

static unsigned int
_efl_ui_caching_factory_memory_limit_get(const Eo *obj EINA_UNUSED,
                                         Efl_Ui_Caching_Factory_Data *pd)
{
   return pd->limit.memory;
}

static void
_efl_ui_caching_factory_items_limit_set(Eo *obj,
                                        Efl_Ui_Caching_Factory_Data *pd,
                                        unsigned int limit)
{
   pd->limit.items = limit;

   _efl_ui_caching_factory_flush(obj, pd);
}

static unsigned int
_efl_ui_caching_factory_items_limit_get(const Eo *obj EINA_UNUSED,
                                        Efl_Ui_Caching_Factory_Data *pd)
{
   return pd->limit.items;
}

static Eina_Value
_schedule_cache_flush(Eo *obj, void *data, const Eina_Value v)
{
   Efl_Ui_Caching_Factory_Data *pd = data;

   // And check if the cache need some triming
   _efl_ui_caching_factory_flush(obj, pd);

   return v;
}

/**
 * @brief Future 'free' callback for the scheduled cache flush.
 *
 * Clears the `pd->flush` future pointer.
 *
 * @param o The object associated with the future (unused).
 * @param data A pointer to Efl_Ui_Caching_Factory_Data.
 * @param dead_future The future that has completed (unused).
 */
static void
_schedule_done(Eo *o EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   Efl_Ui_Caching_Factory_Data *pd = data;

   pd->flush = NULL;
}

/**
 * @brief Implements Efl_Ui_Factory.release.
 *
 * Releases UI views back into the cache for potential reuse.
 * If the factory is invalidated, items are deleted instead of cached.
 * Released items are made invisible and disconnected from their models.
 * A cache flush is scheduled if items are added to the cache.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 * @param ui_views An iterator over Efl_Gfx_Entity objects (UI views) to be released.
 */
static void
_efl_ui_caching_factory_efl_ui_factory_release(Eo *obj,
                                               Efl_Ui_Caching_Factory_Data *pd,
                                               Eina_Iterator *ui_views)
{
   Efl_Gfx_Entity *ui_view;

   // Are we invalidated ?
   if (pd->invalidated)
     {
        _efl_ui_caching_factory_item_del(obj, pd, ui_views);
        return;
     }

   EINA_ITERATOR_FOREACH(ui_views, ui_view)
     {
        // Change parent, disconnect the object and make it invisible
        efl_gfx_entity_visible_set(ui_view, EINA_FALSE);
        efl_event_callback_call(obj, EFL_UI_FACTORY_EVENT_ITEM_RELEASING, ui_view);

        // Add to the cache
        pd->cache = eina_list_prepend(pd->cache, ui_view);
        pd->current.items++;
        pd->current.memory += efl_class_memory_size_get(ui_view);
        if (efl_isa(ui_view, EFL_CACHED_ITEM_INTERFACE))
          pd->current.memory += efl_cached_item_memory_size_get(ui_view);

        // Fill lookup
        if (!pd->klass && efl_ui_widget_style_get(ui_view))
          {
             if (!pd->lookup) pd->lookup = eina_hash_string_djb2_new(NULL);
             eina_hash_direct_add(pd->lookup, efl_ui_widget_style_get(ui_view), ui_view);
          }
     }
   eina_iterator_free(ui_views);

   // Schedule a cache flush if necessary
   if (!pd->flush)
     pd->flush = efl_future_then(obj, efl_loop_job(efl_loop_get(obj)),
                                 .success = _schedule_cache_flush,
                                 .free = _schedule_done,
                                 .data = pd);
}

static void
_efl_ui_caching_factory_pause(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Caching_Factory_Data *pd = data;
   Efl_Gfx_Entity *entity;

   // Application is going into background, let's free resources.
   // Possible improvement would be to delay that by a few seconds.
   EINA_LIST_FREE(pd->cache, entity)
     efl_del(entity);

   pd->current.items = 0;
   pd->current.memory = 0;
}

/**
 * @brief Event callback for EFL_EVENT_INVALIDATE on the factory's widget.
 *
 * When the factory's associated widget is invalidated, the cache is cleared,
 * the lookup table is freed, and the factory is marked as invalidated.
 * This ensures that no stale items are reused.
 *
 * @param data A pointer to Efl_Ui_Caching_Factory_Data.
 * @param event The Efl_Event details (unused).
 */
static void
_invalidate(void *data, const Efl_Event *event EINA_UNUSED)
{
   Efl_Ui_Caching_Factory_Data *pd = data;

   // As all the objects in the cache have the factory as parent, there's no need to unparent them
   // because they will be deleted when the factory is deleted.
   // However, we free the list structure itself.
   pd->cache = eina_list_free(pd->cache);
   eina_hash_free(pd->lookup);
   pd->lookup = NULL;
   pd->invalidated = EINA_TRUE;
}

/**
 * @brief Implements Efl_Object.finalize.
 *
 * Performs final setup for the caching factory. This includes:
 * - Finalizing the superclass.
 * - Registering for the EFL_APP_EVENT_PAUSE event to clear the cache when the app pauses.
 * - Registering for the EFL_EVENT_INVALIDATE event on the factory's widget to handle invalidation.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 * @return The finalized object, or NULL on failure.
 */
static Efl_Object *
_efl_ui_caching_factory_efl_object_finalize(Eo *obj, Efl_Ui_Caching_Factory_Data *pd)
{
   Efl_App *a;

   obj = efl_finalize(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS));
   if (!obj) return NULL;

   a = efl_provider_find(obj, EFL_APP_CLASS);
   if (a) efl_event_callback_add(a, EFL_APP_EVENT_PAUSE, _efl_ui_caching_factory_pause, pd);

   // The order of the invalidate event is guaranteed to happen before any children is invalidated
   // this is not the case for the children invalidate function, which can happen in random order.
   efl_event_callback_add(efl_ui_widget_factory_widget_get(obj), EFL_EVENT_INVALIDATE, _invalidate, pd);

   return obj;
}

static void
_efl_ui_caching_factory_efl_object_invalidate(Eo *obj,
                                              Efl_Ui_Caching_Factory_Data *pd)
{
   // Unregister the invalidate event callback before invalidating the superclass.
   efl_event_callback_del(efl_ui_widget_factory_widget_get(obj), EFL_EVENT_INVALIDATE, _invalidate, pd);

   efl_invalidate(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS));
}

/**
 * @brief Implements Efl_Ui_Property_Bind.property_bind.
 *
 * Binds a property key to a model property name.
 * Specifically handles the "style" key to store the model property name
 * that will provide the style for cached items.
 *
 * @param obj The caching factory object.
 * @param pd The private data of the caching factory.
 * @param key The key to bind (e.g., "style").
 * @param property The name of the model property to bind to the key.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
_efl_ui_caching_factory_efl_ui_property_bind_property_bind(Eo *obj, Efl_Ui_Caching_Factory_Data *pd,
                                                           const char *key, const char *property)
{
   if (!strcmp(key, "style"))
     eina_stringshare_replace(&pd->style, property);

   return efl_ui_property_bind(efl_super(obj, EFL_UI_CACHING_FACTORY_CLASS), key, property);
}

#include "efl_ui_caching_factory.eo.c"
