#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Injects an Evas object into a given Evas canvas layer.
 *
 * This function places the object @p obj into the appropriate layer within
 * the Evas canvas @p e. If the layer does not exist, it is created.
 * The object's reference count is incremented, and it is added to the
 * layer's list of objects.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param e The Evas canvas.
 */
void
evas_object_inject(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas *e)
{
   Evas_Layer *lay;
   Evas_Public_Data *evas;

   if (!obj) return;
   if (!e) return;
   if (obj->in_layer) return;
   evas = efl_data_scope_get(e, EVAS_CANVAS_CLASS);
   if (!evas) return;
   evas_canvas_async_block(evas);
   lay = evas_layer_find(e, obj->cur->layer);
   if (!lay)
     {
        lay = evas_layer_new(e);
        lay->layer = obj->cur->layer;
        evas_layer_add(lay);
     }
   efl_data_ref(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   lay->objects = (Evas_Object_Protected_Data *)eina_inlist_append(EINA_INLIST_GET(lay->objects), EINA_INLIST_GET(obj));
   lay->usage++;
   obj->layer = lay;
   obj->in_layer = 1;
}

/**
 * @internal
 * @brief Releases an Evas object from its layer.
 *
 * This function removes the object @p obj from its current layer.
 * If @p clean_layer is true and the layer becomes empty, the layer itself
 * might be deleted. The object's reference count is decremented.
 * If the layer is currently being iterated (walking_objects is true),
 * the object is added to a pending removal list to be processed later.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param clean_layer If true, the layer may be deleted if it becomes empty.
 */
void
evas_object_release(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, int clean_layer)
{
   if (!obj->in_layer) return;
   if (!obj->layer->walking_objects)
     obj->layer->objects = (Evas_Object_Protected_Data *)eina_inlist_remove(EINA_INLIST_GET(obj->layer->objects), EINA_INLIST_GET(obj));
   efl_data_unref(eo_obj, obj);
   if (!obj->layer->walking_objects)
     {
        obj->layer->usage--;
        if (clean_layer)
          {
             if (obj->layer->usage <= 0)
               {
                  evas_layer_del(obj->layer);
               }
          }
        obj->layer = NULL;
        obj->in_layer = 0;
     }
   else
     obj->layer->removes = eina_list_append(obj->layer->removes, obj);
}

/**
 * @internal
 * @brief Creates a new Evas layer.
 *
 * Allocates and initializes a new Evas_Layer structure associated with the
 * given Evas canvas @p eo_e. The Evas canvas public data is referenced.
 *
 * @param eo_e The Evas canvas to associate the new layer with.
 * @return A pointer to the newly created Evas_Layer, or NULL on failure.
 */
Evas_Layer *
evas_layer_new(Evas *eo_e)
{
   Evas_Public_Data *e = efl_data_ref(eo_e, EVAS_CANVAS_CLASS);
   Evas_Layer *lay;

   lay = calloc(1, sizeof(Evas_Layer));
   if (!lay) return NULL;
   lay->evas = e;
   return lay;
}

/**
 * @internal
 * @brief Processes objects pending removal from a layer.
 *
 * Iterates through the list of objects marked for removal from layer @p lay
 * (the `removes` list) and actually removes them from the layer's main
 * object list. This is typically called when it's safe to modify the
 * object list (i.e., not during an iteration). If the layer becomes empty
 * after removals, it is deleted.
 *
 * @param lay The Evas layer to flush removed objects from.
 */
void
_evas_layer_flush_removes(Evas_Layer *lay)
{
   Evas_Object_Protected_Data *obj;

   if (lay->walking_objects) return;
   EINA_LIST_FREE(lay->removes, obj)
     {
        lay->objects = (Evas_Object_Protected_Data *)
          eina_inlist_remove(EINA_INLIST_GET(lay->objects),
                             EINA_INLIST_GET(obj));
        obj->layer = NULL;
        obj->in_layer = 0;
        if (lay->usage > 0) lay->usage--;
     }
   if (lay->usage <= 0)
     {
        evas_layer_del(lay);
     }
}

/**
 * @internal
 * @brief Prepares a layer for freeing by deleting its top-level objects.
 *
 * This function is called before a layer @p lay is actually freed. It iterates
 * through all objects in the layer. If an object is a top-level object (not
 * part of a smart object) and not already marked for deletion, it calls
 * evas_object_del() on it. This ensures that objects are properly cleaned up
 * before the layer itself is destroyed. It also handles flushing any pending
 * object removals.
 *
 * @param lay The Evas layer to prepare for freeing.
 */
void
evas_layer_pre_free(Evas_Layer *lay)
{
   Evas_Object_Protected_Data *obj;

   lay->walking_objects++;
   EINA_INLIST_FOREACH(lay->objects, obj)
     {
        if ((!obj->smart.parent) && (!obj->delete_me))
          evas_object_del(obj->object);
     }
   lay->walking_objects--;
   _evas_layer_flush_removes(lay);
}

/**
 * @internal
 * @brief Frees all objects within a given layer.
 *
 * Iterates through all objects in layer @p lay and calls evas_object_free()
 * on each one. This is a lower-level cleanup than evas_layer_pre_free,
 * directly freeing the object's protected data. It also logs an error if
 * an object in the layer stack doesn't have a valid object pointer.
 *
 * @param lay The Evas layer whose objects are to be freed.
 */
void
evas_layer_free_objects(Evas_Layer *lay)
{
   Evas_Object_Protected_Data *obj;

   EINA_INLIST_FREE(lay->objects, obj)
     {
        if (obj->object == NULL)
          {
             ERR("Object still present in the canvas stack, but without a valid object pointer (%s@%p).", obj->type, obj);
          }
        evas_object_free(obj, EINA_FALSE);
     }
}

/**
 * @internal
 * @brief Deletes all layers in an Evas canvas.
 *
 * Iterates through all layers associated with the Evas canvas @p eo_e
 * and deletes each one by calling evas_layer_del(). This is typically
 * used during the Evas canvas shutdown process.
 *
 * @param eo_e The Evas canvas whose layers are to be cleaned.
 */
void
evas_layer_clean(Evas *eo_e)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Evas_Layer *tmp;

   while (e->layers)
     {
        tmp = e->layers;
        evas_layer_del(tmp);
     }
}

/**
 * @internal
 * @brief Finds an Evas layer by its layer number.
 *
 * Searches through the layers of the Evas canvas @p eo_e to find the one
 * that matches the given @p layer_num.
 *
 * @param eo_e The Evas canvas to search within.
 * @param layer_num The layer number to find.
 * @return A pointer to the found Evas_Layer, or NULL if no layer with
 *         the specified number exists.
 */
Evas_Layer *
evas_layer_find(Evas *eo_e, short layer_num)
{
   Evas_Public_Data *e = efl_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Evas_Layer *layer;

   EINA_INLIST_FOREACH(e->layers, layer)
     {
        if (layer->layer == layer_num) return layer;
     }
   return NULL;
}

/**
 * @internal
 * @brief Adds a layer to the Evas canvas, maintaining sorted order.
 *
 * Inserts the given layer @p lay into the list of layers for its associated
 * Evas canvas. The list of layers is kept sorted by the layer number.
 * This function finds the correct position for @p lay and inserts it.
 *
 * @param lay The Evas_Layer to add.
 */
void
evas_layer_add(Evas_Layer *lay)
{
   Evas_Layer *layer;

   EINA_INLIST_FOREACH(lay->evas->layers, layer)
     {
        if (layer->layer > lay->layer)
          {
             lay->evas->layers = (Evas_Layer *)eina_inlist_prepend_relative(EINA_INLIST_GET(lay->evas->layers),
                                                                            EINA_INLIST_GET(lay),
                                                                            EINA_INLIST_GET(layer));
             return;
          }
     }
   lay->evas->layers = (Evas_Layer *)eina_inlist_append(EINA_INLIST_GET(lay->evas->layers), EINA_INLIST_GET(lay));
}

/**
 * @internal
 * @brief Deletes an Evas layer.
 *
 * Removes the layer @p lay from its Evas canvas's list of layers.
 * It also unreferences the Evas canvas public data and schedules the
 * layer structure itself to be freed from the main loop, ensuring safety
 * if the layer is deleted during event processing or rendering.
 *
 * @param lay The Evas_Layer to delete.
 */
void
evas_layer_del(Evas_Layer *lay)
{
   Evas_Public_Data *e;

   e = lay->evas;
   e->layers = (Evas_Layer *)eina_inlist_remove(EINA_INLIST_GET(e->layers), EINA_INLIST_GET(lay));
   efl_data_unref(e->evas, e);
   eina_freeq_ptr_main_add(lay, free, sizeof(*lay));
}

/**
 * @internal
 * @brief Recursively sets the layer for a child object and its members.
 *
 * This function is used to update the layer of a child object @p obj when its
 * parent @p par_obj changes layer or when the child is added to a smart object.
 * It ensures that the child object's layer property @p l is set correctly.
 * If the child object was previously in a different layer (erroneously, as
 * children shouldn't be top-level), it's released from that layer.
 * The function then updates the child's layer to match the parent's layer
 * and adjusts usage counts. If the child is itself a smart object, this
 * function is called recursively for all its members.
 *
 * @param obj The protected data of the child object whose layer is to be set.
 * @param par_obj The protected data of the parent object.
 * @param l The new layer number to set.
 */
static void
_evas_object_layer_set_child(Evas_Object_Protected_Data *obj, Evas_Object_Protected_Data *par_obj, short l)
{
   if (obj->delete_me) return;
   if (obj->cur->layer == l) return;
   if (EINA_UNLIKELY(obj->in_layer))
     {
        ERR("Invalid internal state of object %p (child marked as being a "
            "top-level object)!", obj->object);
        evas_object_release(obj->object, obj, 1);
     }
   else if ((--obj->layer->usage) == 0)
     {
        evas_layer_del(obj->layer);
     }

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     {
       state_write->layer = l;
     }
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);

   obj->layer = par_obj->layer;
   obj->layer->usage++;
   if (obj->is_smart)
     {
        Eina_Inlist *contained;
        Evas_Object_Protected_Data *member;

        contained = (Eina_Inlist *)evas_object_smart_members_get_direct(obj->object);
        EINA_INLIST_FOREACH(contained, member)
          {
             _evas_object_layer_set_child(member, obj, l);
          }
     }
}

/* public functions */

/**
 * @brief Sets the layer of an Evas object.
 * @param obj The object.
 * @param l The layer number to set.
 * @see efl_gfx_stack_layer_set()
 * @ingroup Evas_Object_Group_Layer
 */
EVAS_API void
evas_object_layer_set(Evas_Object *obj, short l)
{
   efl_gfx_stack_layer_set((Evas_Object *)obj, l);
}

/**
 * @internal
 * @brief Efl Gfx Stack layer_set implementation.
 *
 * This function implements the Eolian interface for setting an object's layer.
 * It handles various conditions, such as whether the object is a smart object's
 * member, if the layer is actually changing, and updates related properties
 * like restacking flags and event processing.
 *
 * @param eo_obj The Evas object (Eo pointer).
 * @param obj The protected data of the Evas object.
 * @param l The new layer number.
 */
EOLIAN void
_efl_canvas_object_efl_gfx_stack_layer_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, short l)
{
   Evas *eo_e;

   if (obj->delete_me) return;
   evas_object_async_block(obj);
   if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_LAYER_SET, 1, l)) return;
   if (obj->smart.parent) return;
   if (obj->cur->layer == l)
     {
        evas_object_raise(eo_obj);
        return;
     }
   eo_e = obj->layer->evas->evas;
   evas_object_release(eo_obj, obj, 1);
   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     {
       state_write->layer = l;
     }
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);

   evas_object_inject(eo_obj, obj, eo_e);
   obj->restack = 1;
   evas_object_change(eo_obj, obj);
   if (obj->clip.clipees)
     {
        evas_object_inform_call_restack(eo_obj, obj);
        return;
     }
   evas_object_change(eo_obj, obj);
   if (!obj->is_smart && obj->cur->visible)
     {
        _evas_canvas_event_pointer_in_rect_mouse_move_feed(obj->layer->evas, eo_obj, obj,
                                                           1, 1, EINA_TRUE,
                                                           NULL);
     }
   else if (obj->is_smart)
     {
        Eina_Inlist *contained;
        Evas_Object_Protected_Data *member;

        contained = (Eina_Inlist *)evas_object_smart_members_get_direct(eo_obj);
        EINA_INLIST_FOREACH(contained, member)
          {
            _evas_object_layer_set_child(member, obj, l);
          }
     }
   evas_object_inform_call_restack(eo_obj, obj);
}

/**
 * @brief Gets the layer of an Evas object.
 * @param obj The object.
 * @return The layer number.
 * @see efl_gfx_stack_layer_get()
 * @ingroup Evas_Object_Group_Layer
 */
EVAS_API short
evas_object_layer_get(const Evas_Object *obj)
{
   return efl_gfx_stack_layer_get((Evas_Object *)obj);
}

/**
 * @internal
 * @brief Efl Gfx Stack layer_get implementation.
 *
 * This function implements the Eolian interface for getting an object's layer.
 * If the object is a member of a smart object, it returns the layer of its
 * smart parent. Otherwise, it returns the object's own layer.
 *
 * @param eo_obj The Evas object (Eo pointer, unused).
 * @param obj The protected data of the Evas object.
 * @return The layer number of the object.
 */
EOLIAN short
_efl_canvas_object_efl_gfx_stack_layer_get(const Eo *eo_obj EINA_UNUSED,
                                     Evas_Object_Protected_Data *obj)
{
   if (obj->smart.parent)
     {
        Evas_Object_Protected_Data *smart_parent_obj = efl_data_scope_get(obj->smart.parent, EFL_CANVAS_OBJECT_CLASS);
        return smart_parent_obj->cur->layer;
     }
   return obj->cur->layer;
}

