#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @internal
 * @brief Unsets the clipper for the given Evas object.
 *
 * This function is a core part of removing a clipping relationship.
 * It handles cleaning up references and triggering necessary recalculations.
 *
 * @param eo_obj The Evas object (Eo pointer) from which the clipper is to be unset.
 * @param obj The protected data of the Evas object.
 */
static void _clip_unset(Eo *eo_obj, Evas_Object_Protected_Data *obj);

/**
 * @internal
 * @brief Marks an object's clip information as dirty and propagates this to its clipees.
 *
 * When a clipper changes in a way that affects its clipping area (e.g., move, resize,
 * visibility change), its clipees need to be re-rendered. This function marks the
 * clipper's state as dirty and recursively calls evas_object_clip_dirty() on all
 * objects it clips (clipees).
 *
 * @param obj The protected data of the clipper object whose clip state is now dirty.
 */
void
evas_object_clip_dirty_do(Evas_Object_Protected_Data *obj)
{
   Evas_Object_Protected_Data *clipee;
   Eina_List *l;

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     {
       state_write->cache.clip.dirty = EINA_TRUE;
     }
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);

   EINA_LIST_FOREACH(obj->clip.clipees, l, clipee)
     {
        evas_object_clip_dirty(clipee->object, clipee);
     }
}

/**
 * @internal
 * @brief Recalculates clipping for an object and its clipees if the clip is marked dirty.
 *
 * This function first checks if the object's own clip cache is dirty. If so, it
 * recalculates its clip information via evas_object_clip_recalc(). Then, it
 * recursively calls itself for all objects that this object clips (its clipees),
 * ensuring that the entire chain of clipping dependencies is updated.
 *
 * @param obj The protected data of the object for which to recalculate clippees.
 *            This object acts as a clipper for other objects.
 */
void
evas_object_recalc_clippees(Evas_Object_Protected_Data *obj)
{
   Evas_Object_Protected_Data *clipee;
   Eina_List *l;

   EVAS_OBJECT_DATA_VALID_CHECK(obj);
   if (obj->cur->cache.clip.dirty)
     {
        evas_object_clip_recalc(obj);
        EINA_LIST_FOREACH(obj->clip.clipees, l, clipee)
          {
             evas_object_recalc_clippees(clipee);
          }
     }
}

/* aaaaargh (pirate voice) ... notes!
 *
 * we have a big problem until now that's gone undetected... until yesterday.
 * that problem involves clips and maps and smart objects. hooray! 3 of the
 * more complex bits of evas - and maps and smart objects  being one of the
 * nastiest ones.
 *
 * what is the problem? when a clip crosses a map boundary. that is to say
 * that when the clipper and clippee are not within the child tree of the
 * mapped object. in this case "bad stuff" happens. basically as clips are
 * then used to render objects, but they no longer apply as you'd expect as
 * the map transfomr the objects to-be-clipped separately from the objects
 * that clip them and this whole relationship is broken by maps. it somehow
 * managed to not break with the advent of smart objects. lucky me... but
 * maps killed it. now... what do we do? that is a good question. detect
 * such a broken link and "turn off clipping" in that event - sure. but this
 * isn't going to be cheap as ANY addition or deletion of a map to an object
 * or any change in clipper of an object or any change in smart object
 * membership needs to walk the obj tree both up and down from the changed
 * object and probably walk entire object trees to find these and mark them.
 * that's silly-expensive and i was about to fix it that way but it has since
 * dawned on me that that is just going to kill performance in some critical
 * areas like during object setup and manipulation, as well as teardown.
 *
 * aaaaagh! best for now is to document this as a "don't do it damnit!" thing
 * and have the apps avoid it. but even then - how to do this? this is not
 * easy. everywhere i turn so far i come up to either expensive operations,
 * breaks in logic, or nasty re-work of apps or4 the whole concept of clipping,
 * smart objects and maps... and that will have to wait for evas 2.0
 *
 * the below does clip fixups etc. in the even a clip spans a map boundary.
 * not pretty, but necessary.
 */

#define MAP_ACROSS 1
/**
 * @internal
 * @brief Marks child objects when a map boundary is crossed by a clip.
 *
 * This function is part of the mechanism to handle complex interactions
 * between clipping and EFL maps. When an object is clipped by another object
 * that resides in a different map space (i.e., its `map_parent` is different),
 * this function is invoked to propagate the `map_parent` information down
 * the hierarchy of smart object children and clipees that are not themselves
 * mapped. This ensures that clipping calculations are aware of these map
 * boundaries.
 *
 * The `visited` array is used to prevent infinite recursion in complex object
 * hierarchies, especially with circular dependencies or extensive sharing of
 * sub-objects. If `visited` is NULL, a temporary array from the Evas canvas
 * (`obj->layer->evas->map_clip_objects`) is used.
 *
 * @param eo_obj The Evas object (Eo pointer) being processed.
 * @param obj The protected data of `eo_obj`.
 * @param map_obj The Evas object (Eo pointer) that defines the current map
 *                context. This is typically the closest mapped ancestor or
 *                NULL if there isn't one.
 * @param force If EINA_TRUE, forces the update even if `obj->map->cur.map_parent`
 *              already matches `map_obj`.
 * @param visited An Eina_Array used to keep track of visited objects to prevent
 *                infinite loops. If NULL, a canvas-global array is used.
 *                Example of `visited` elements: `Evas_Object*` (as `void*`).
 */
static void
evas_object_child_map_across_mark(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Object *map_obj, Eina_Bool force, Eina_Array* visited)
{
#ifdef MAP_ACROSS
   Eina_Bool clear_visited = EINA_FALSE;
   if (!visited)
     {
        visited = &obj->layer->evas->map_clip_objects;
        clear_visited = EINA_TRUE;
     }

   if (eina_array_find(visited, (void*) eo_obj, NULL)) goto end;
   else eina_array_push(visited, (void*) eo_obj);

   if ((obj->map->cur.map_parent != map_obj) || force)
     {
        EINA_COW_WRITE_BEGIN(evas_object_map_cow, obj->map, Evas_Object_Map_Data, map_write)
          map_write->cur.map_parent = map_obj;
        EINA_COW_WRITE_END(evas_object_map_cow, obj->map, map_write);

        EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
          {
             state_write->cache.clip.dirty = 1;
          }
        EINA_COW_STATE_WRITE_END(obj, state_write, cur);

        evas_object_clip_recalc(obj);
        if (obj->is_smart)
          {
             Evas_Object_Protected_Data *obj2;

             EINA_INLIST_FOREACH(evas_object_smart_members_get_direct(eo_obj), obj2)
               {
                  // if obj has its own map - skip it. already done
                  if ((obj2->map->cur.map) && (obj2->map->cur.usemap)) continue;
                  Evas_Object *eo_obj2 = obj2->object;
                  evas_object_child_map_across_mark(eo_obj2, obj2, map_obj,
                                                    force, visited);
               }
          }
        else if (obj->clip.clipees)
          {
             Evas_Object_Protected_Data *obj2;
             Eina_List *l;

             EINA_LIST_FOREACH(obj->clip.clipees, l, obj2)
               {
                  evas_object_child_map_across_mark(obj2->object, obj2,
                                                    map_obj, force, visited);
               }
          }
     }

end:
   if (clear_visited) eina_array_clean(visited);
#endif
}

/**
 * @internal
 * @brief Checks if an object's clipper is in a different map space and marks accordingly.
 *
 * If an object `obj` has a clipper, and that clipper's `map_parent` is different
 * from `obj`'s `map_parent`, it means the clip spans across a map boundary.
 * In such cases, this function calls `evas_object_child_map_across_mark` to
 * update the map context for `obj` and its relevant children/clipees.
 *
 * @param eo_obj The Evas object (Eo pointer) to check.
 * @param obj The protected data of `eo_obj`.
 */
void
evas_object_clip_across_check(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
#ifdef MAP_ACROSS
   if (!obj->cur->clipper) return;
   if (obj->cur->clipper->map->cur.map_parent != obj->map->cur.map_parent)
      evas_object_child_map_across_mark(eo_obj, obj, obj->map->cur.map_parent,
                                        1, NULL);
#endif
}

/**
 * @internal
 * @brief Propagates map boundary checks to an object's clipees.
 *
 * This function is called on a clipper object. It first ensures its own
 * map-across status is up-to-date using `evas_object_child_map_across_mark`.
 * Then, if its clip state is dirty (indicating a potential change that
 * affects clipees), it recursively calls this function for all objects
 * it clips.
 *
 * @param eo_obj The Evas object (Eo pointer) which acts as a clipper.
 * @param obj The protected data of `eo_obj`.
 */
void
evas_object_clip_across_clippees_check(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
#ifdef MAP_ACROSS
   Evas_Object_Protected_Data *obj2;
   Eina_List *l;

   if (!obj->clip.clipees) return;
// schloooooooooooow:
//   evas_object_child_map_across_mark(eo_obj, obj->map->cur.map_parent, 1, NULL);
// buggy:
   evas_object_child_map_across_mark(eo_obj, obj, obj->map->cur.map_parent, 0,
                                    NULL);
   if (obj->cur->cache.clip.dirty)
     {
	EINA_LIST_FOREACH(obj->clip.clipees, l, obj2)
          {
             evas_object_clip_across_clippees_check(obj2->object, obj2);
          }
     }
#endif
}

// this function is called on an object when map is enabled or disabled on it
// thus creating a "map boundary" at that point.
//
// FIXME: flip2 test broken in elm - might be show/hide of clips
/**
 * @internal
 * @brief Updates map-across information when an object's map state changes.
 *
 * This function is called when a map is enabled or disabled on `eo_obj`.
 * If a map is enabled, `eo_obj` itself becomes the new `map_parent` for its
 * children/clipees that are not independently mapped.
 * If a map is disabled, the `map_parent` for its children/clipees reverts
 * to that of `eo_obj`'s own smart parent (if any), or NULL.
 * This ensures that the `map_parent` information is correctly propagated
 * when map boundaries are created or removed.
 *
 * @param eo_obj The Evas object (Eo pointer) whose map state has changed.
 * @param obj The protected data of `eo_obj`.
 */
void
evas_object_mapped_clip_across_mark(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
#ifdef MAP_ACROSS
   if ((obj->map->cur.map) && (obj->map->cur.usemap))
      evas_object_child_map_across_mark(eo_obj, obj, eo_obj, 0, NULL);
   else
     {
        if (obj->smart.parent)
          {
             Evas_Object_Protected_Data *smart_parent_obj =
                efl_data_scope_get(obj->smart.parent, EFL_CANVAS_OBJECT_CLASS);
             evas_object_child_map_across_mark
                (eo_obj, obj, smart_parent_obj->map->cur.map_parent, 0, NULL);
          }
        else
           evas_object_child_map_across_mark(eo_obj, obj, NULL, 0, NULL);
    }
#endif
}

/**
 * @internal
 * @brief Unsets the mask properties of an object if it's no longer acting as a mask.
 *
 * This function is called when an object that was potentially used as a mask
 * (e.g., a non-rectangular clipper) might no longer need its mask surface.
 * It checks if the object is still marked as a mask (`obj->mask->is_mask`)
 * and if it has any clipees. If it's a mask but has no clipees, its mask
 * properties are cleared, and its mask surface is freed.
 *
 * @param obj The protected data of the object whose mask properties are to be evaluated.
 */
static void
_efl_canvas_object_clipper_mask_unset(Evas_Object_Protected_Data *obj)
{
   EVAS_OBJECT_DATA_VALID_CHECK(obj);
   if (!obj->mask->is_mask) return;
   if (obj->clip.clipees) return;

   /* this frees the clip surface. is this correct? */
   EINA_COW_WRITE_BEGIN(evas_object_mask_cow, obj->mask, Evas_Object_Mask_Data, mask)
     mask->is_mask = EINA_FALSE;
     mask->redraw = EINA_FALSE;
     mask->is_alpha = EINA_FALSE;
     if (mask->surface)
       {
          obj->layer->evas->engine.func->image_free(ENC, mask->surface);
          mask->surface = NULL;
       }
     mask->w = 0;
     mask->h = 0;
   EINA_COW_WRITE_END(evas_object_mask_cow, obj->mask, mask);
}

/* public functions */
extern const char *o_rect_type;
extern const char *o_image_type;

static void _clipper_invalidated_cb(void *data, const Efl_Event *event);

/**
 * @internal
 * @brief Performs preliminary checks to determine if setting a clipper should be blocked.
 *
 * This function validates various conditions before allowing `eo_clip` to be set
 * as the clipper for `eo_obj`. It checks for:
 * - Setting an object as its own clipper.
 * - Using a deleted object as a clipper.
 * - Clipping a deleted object.
 * - Objects not belonging to the same Evas canvas.
 * - Objects not having a layer.
 * If any of these error conditions are met, an error message is logged, and
 * the function returns `EINA_TRUE` to indicate that the clipping operation
 * should not proceed. Otherwise, it returns `EINA_FALSE`.
 *
 * @param eo_obj The Evas object (Eo pointer) to be clipped.
 * @param obj The protected data of `eo_obj`. If NULL, it's retrieved from `eo_obj`.
 * @param eo_clip The Evas object (Eo pointer) to be used as the clipper.
 * @param clip The protected data of `eo_clip`. If NULL, it's retrieved from `eo_clip`.
 * @return EINA_TRUE if the clipper setting should be blocked, EINA_FALSE otherwise.
 */
Eina_Bool
_efl_canvas_object_clipper_set_block(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                                  Evas_Object *eo_clip, Evas_Object_Protected_Data *clip)
{
   if (!obj) obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (!clip) clip = efl_data_scope_get(eo_clip, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);

   if (obj->cur->clipper && (obj->cur->clipper->object == eo_clip)) return EINA_TRUE;
   if (eo_obj == eo_clip) goto err_same;
   if (clip->delete_me) goto err_clip_deleted;
   if (obj->delete_me) goto err_obj_deleted;
   if (!obj->layer || !clip->layer) goto err_no_layer;
   if (obj->layer->evas != clip->layer->evas) goto err_diff_evas;

   return EINA_FALSE;

err_same:
   CRI("Setting clip %p on itself", eo_obj);
   return EINA_TRUE;
err_clip_deleted:
   CRI("Setting deleted object %p as clip obj %p", eo_clip, eo_obj);
   return EINA_TRUE;
err_obj_deleted:
   CRI("Setting object %p as clip to deleted obj %p", eo_clip, eo_obj);
   return EINA_TRUE;
err_no_layer:
   CRI("Object %p or clip %p layer is not set !", obj, clip);;
   return EINA_TRUE;
err_diff_evas:
   CRI("Setting object %p from Evas (%p) to another Evas (%p)",
       obj, obj->layer->evas, clip->layer->evas);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Common logic for unsetting the current clipper of an object.
 *
 * This function handles the core tasks of removing `obj` from its current
 * clipper's list of clipees. It updates the clipper's `have_clipees` status,
 * potentially unsets the clipper's mask if it's no longer needed, triggers
 * damage events if the clipper was visible and colored (as its removal might
 * change rendering), and removes event callbacks.
 *
 * @param obj The protected data of the object whose clipper is being unset.
 * @param warn If EINA_TRUE, a warning is issued if a static clipper is overridden.
 */
static inline void
_efl_canvas_object_clipper_unset_common(Evas_Object_Protected_Data *obj, Eina_Bool warn)
{
   Evas_Object_Protected_Data *clip = obj->cur->clipper;

   if (!clip) return;
   if (EVAS_OBJECT_DATA_VALID(clip))
     {
        clip->clip.cache_clipees_answer = eina_list_free(clip->clip.cache_clipees_answer);
        clip->clip.clipees = eina_list_remove(clip->clip.clipees, obj);
        if (!clip->clip.clipees)
          {
             EINA_COW_STATE_WRITE_BEGIN(clip, state_write, cur)
               {
                  state_write->have_clipees = 0;
                  if (warn && clip->is_static_clip)
                    {
                       WRN("You override static clipper, it may be dangled! "
                           "obj(%p) type(%s) new clip(%p)",
                           obj->object, obj->type, clip->object);
                    }
               }
             EINA_COW_STATE_WRITE_END(clip, state_write, cur);
             /* i know this was to handle a case where a clip stops having
              * children and becomes a solid colored box - no one ever does
              * that... they hide the clip so dont add damages,
              * But, if the clipper could affect color to its clipees, the
              * clipped area should be redrawn. */
             if (((clip->cur) && (clip->cur->visible)) &&
                 (((clip->cur->color.r != 255) || (clip->cur->color.g != 255) ||
                   (clip->cur->color.b != 255) || (clip->cur->color.a != 255)) ||
                  (clip->mask->is_mask)) &&
                  efl_alive_get(clip->object))
               {
                  if (clip->layer)
                    {
                       Evas_Public_Data *e = clip->layer->evas;
                       evas_damage_rectangle_add(e->evas,
                                                 clip->cur->geometry.x + e->framespace.x,
                                                 clip->cur->geometry.y + e->framespace.y,
                                                 clip->cur->geometry.w,
                                                 clip->cur->geometry.h);
                    }
               }

             _efl_canvas_object_clipper_mask_unset(clip);
          }
        evas_object_change(clip->object, clip);
        if (obj->prev->clipper != clip)
          efl_event_callback_del(clip->object, EFL_EVENT_INVALIDATE, _clipper_invalidated_cb, obj->object);
     }

   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     state_write->clipper = NULL;
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);
}

/**
 * @internal
 * @brief Sets whether the Evas object has a fixed size.
 * @since 1.26
 *
 * This informs Evas that the object's size will not change unless explicitly
 * set. This can be an optimization hint for layouting or rendering.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param enable EINA_TRUE if the object has a fixed size, EINA_FALSE otherwise.
 */
EOLIAN void
_efl_canvas_object_has_fixed_size_set(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, Eina_Bool enable)
{
   EVAS_OBJECT_DATA_ALIVE_CHECK(obj);

   enable = !!enable;
   if (obj->cur->has_fixed_size == enable) return;
   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     state_write->has_fixed_size = enable;
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);
   /* this will take effect next time the object is rendered,
    * no need to force re-render now.
    */
}

/**
 * @internal
 * @brief Gets whether the Evas object has a fixed size.
 * @since 1.26
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @return EINA_TRUE if the object has a fixed size, EINA_FALSE otherwise.
 */
EOLIAN Eina_Bool
_efl_canvas_object_has_fixed_size_get(const Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   EVAS_OBJECT_DATA_ALIVE_CHECK(obj, EINA_FALSE);
   return obj->cur->has_fixed_size;
}

/**
 * @internal
 * @brief Sets the clipper for an Evas object.
 * @since 1.26
 *
 * This function establishes a clipping relationship where `eo_clip` becomes the
 * clipper for `eo_obj`. The visible parts of `eo_obj` will be limited to the
 * area occupied by `eo_clip`.
 *
 * @param eo_obj The Evas object (Eo pointer) to be clipped.
 * @param obj The protected data of `eo_obj`.
 * @param eo_clip The Evas object (Eo pointer) to use as the clipper.
 *                Pass NULL to unset the current clipper.
 */
EOLIAN void
_efl_canvas_object_clipper_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, Evas_Object *eo_clip)
{
   Evas_Object_Protected_Data *clip;

   EVAS_OBJECT_DATA_ALIVE_CHECK(obj);

   if ((!obj->cur->clipper && !eo_clip) ||
       (obj->cur->clipper && obj->cur->clipper->object == eo_clip)) return;

   clip = EVAS_OBJECT_DATA_SAFE_GET(eo_clip);
   if (!EVAS_OBJECT_DATA_ALIVE(clip))
     {
        _clip_unset(eo_obj, obj);
        return;
     }

   if (_efl_canvas_object_clipper_set_block(eo_obj, obj, eo_clip, clip)) return;
   if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_CLIP_SET, 1, eo_clip)) return;

   if (obj->is_smart && obj->smart.smart && obj->smart.smart->smart_class &&
       obj->smart.smart->smart_class->clip_set)
     {
        obj->smart.smart->smart_class->clip_set(eo_obj, eo_clip);
     }

   /* unset cur clipper */
   _efl_canvas_object_clipper_unset_common(obj, EINA_TRUE);

   /* non-rect object clipper */
   if (clip->type != o_rect_type)
     {
        EINA_COW_WRITE_BEGIN(evas_object_mask_cow, clip->mask, Evas_Object_Mask_Data, mask)
          mask->is_mask = EINA_TRUE;
        EINA_COW_WRITE_END(evas_object_mask_cow, clip->mask, mask);
     }

   /* clip me */
   EINA_COW_STATE_WRITE_BEGIN(obj, state_write, cur)
     state_write->clipper = clip;
   EINA_COW_STATE_WRITE_END(obj, state_write, cur);
   if (obj->prev->clipper != clip)
     efl_event_callback_add(clip->object, EFL_EVENT_INVALIDATE, _clipper_invalidated_cb, eo_obj);

   clip->clip.cache_clipees_answer = eina_list_free(clip->clip.cache_clipees_answer);
   clip->clip.clipees = eina_list_append(clip->clip.clipees, obj);
   if (clip->clip.clipees)
     {
        EINA_COW_STATE_WRITE_BEGIN(clip, state_write, cur)
          {
             state_write->have_clipees = 1;
          }
        EINA_COW_STATE_WRITE_END(clip, state_write, cur);

        if (clip->changed)
          evas_object_update_bounding_box(eo_clip, clip, NULL);
     }

   evas_object_change(eo_clip, clip);
   evas_object_change(eo_obj, obj);
   evas_object_update_bounding_box(eo_obj, obj, NULL);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_recalc_clippees(obj);
   if ((!obj->is_smart) &&
       (!((obj->map->cur.map) && (obj->map->cur.usemap))) &&
       evas_object_is_visible(obj))
     {
        _evas_canvas_event_pointer_in_rect_mouse_move_feed(obj->layer->evas,
                                                           eo_obj,
                                                           obj, 1, 1,
                                                           EINA_FALSE,
                                                           NULL);
     }
   evas_object_clip_across_check(eo_obj, obj);
}

/**
 * @internal
 * @brief Gets the clipper of an Evas object.
 * @since 1.26
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @return The Evas_Object (Eo pointer) that is clipping `eo_obj`, or NULL if
 *         `eo_obj` is not clipped or an error occurred.
 */
EOLIAN Evas_Object *
_efl_canvas_object_clipper_get(const Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   EVAS_OBJECT_DATA_ALIVE_CHECK(obj, NULL);
   if (obj->cur->clipper)
     return obj->cur->clipper->object;
   return NULL;
}

/**
 * @internal
 * @brief Performs preliminary checks before unsetting a clipper.
 *
 * This function checks if there is actually a clipper set on the object.
 * If no clipper is set, it returns `EINA_TRUE` to indicate that the unsetting
 * operation can be skipped. It also blocks async operations on the object
 * and clears any cached clipees answer.
 *
 * @param eo_obj The Evas object (Eo pointer) whose clipper is to be unset.
 * @param obj The protected data of `eo_obj`.
 * @return EINA_TRUE if unsetting should be blocked (e.g., no clipper set),
 *         EINA_FALSE otherwise.
 */
Eina_Bool
_efl_canvas_object_clipper_unset_block(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   if (!obj->cur->clipper)
     return EINA_TRUE;

   evas_object_async_block(obj);
   obj->clip.cache_clipees_answer = eina_list_free(obj->clip.cache_clipees_answer);

   return EINA_FALSE;
}

static void
_clip_unset(Eo *eo_obj, Evas_Object_Protected_Data *obj)
{
   if (_efl_canvas_object_clipper_unset_block(eo_obj, obj)) return;
   if (_evas_object_intercept_call_evas(obj, EVAS_OBJECT_INTERCEPT_CB_CLIP_SET, 1, NULL)) return;
   if (obj->is_smart && obj->smart.smart && obj->smart.smart->smart_class &&
       obj->smart.smart->smart_class->clip_unset)
     {
        obj->smart.smart->smart_class->clip_unset(eo_obj);
     }
   _efl_canvas_object_clipper_unset_common(obj, EINA_FALSE);
   evas_object_update_bounding_box(eo_obj, obj, NULL);
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_recalc_clippees(obj);

   if ((!obj->is_smart) &&
       (!((obj->map->cur.map) && (obj->map->cur.usemap))) &&
       evas_object_is_visible(obj))
     {
        _evas_canvas_event_pointer_in_rect_mouse_move_feed(obj->layer->evas,
                                                           eo_obj,
                                                           obj, 1, 1,
                                                           EINA_FALSE,
                                                           NULL);
     }
   evas_object_clip_across_check(eo_obj, obj);
}

/**
 * @brief Unset the clipper of an Evas object.
 * @param eo_obj The object.
 * @see evas_object_clip_set()
 * @ingroup Evas_Object_Group_Clipping
 *
 * This function removes any clipper that was previously set on @p eo_obj.
 * The object will no longer be clipped.
 */
EVAS_API void
evas_object_clip_unset(Evas_Object *eo_obj)
{
   efl_canvas_object_clipper_set(eo_obj, NULL);
}

/**
 * @internal
 * @brief Callback function invoked when a clipper object is invalidated (e.g., deleted).
 *
 * When an object that acts as a clipper is invalidated (typically meaning it's
 * being deleted or is no longer valid for use), this callback is triggered for
 * all objects it was clipping. The callback then unsets the clipper relationship
 * for the affected clipee. It also handles cleaning up the `prev->clipper` state
 * if the invalidated clipper was also the previous clipper.
 *
 * @param data User data, which is the Evas_Object (clipee) that was being clipped.
 * @param event The Efl_Event structure containing event information. The
 *              `event->object` is the clipper that was invalidated.
 */
static void
_clipper_invalidated_cb(void *data, const Efl_Event *event)
{
   Evas_Object *eo_obj = data;
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *clip = efl_data_scope_get(event->object, EFL_CANVAS_OBJECT_CLASS);

   EVAS_OBJECT_DATA_ALIVE_CHECK(obj);

   if (EINA_LIKELY(obj->cur->clipper && (obj->cur->clipper == clip)))
     _clip_unset(eo_obj, obj);
   if (obj->prev->clipper && (obj->prev->clipper == clip))
     {
        // not removing cb since it's the del cb... it can't be called again!
        EINA_COW_STATE_WRITE_BEGIN(obj, state_write, prev)
          state_write->clipper = NULL;
        EINA_COW_STATE_WRITE_END(obj, state_write, prev);
     }
}

/**
 * @internal
 * @brief Resets the 'previous' clipper state of an object.
 *
 * Evas objects maintain a 'current' and 'previous' state for various properties
 * to optimize rendering and detect changes. This function deals with the
 * `prev->clipper` field. If a previous clipper existed (`obj->prev->clipper`),
 * this function may nullify that reference and remove associated event callbacks,
 * particularly if the previous clipper is different from the current one.
 *
 * @param obj The protected data of the Evas object whose previous clipper state
 *            is to be reset.
 * @param cur_prev If EINA_TRUE, it implies the current state is being copied to
 *                 the previous state, so `prev->clipper` should not be nulled out
 *                 if it matches `cur->clipper`. If EINA_FALSE, `prev->clipper`
 *                 is unconditionally nulled if it exists.
 */
void
_efl_canvas_object_clipper_prev_reset(Evas_Object_Protected_Data *obj, Eina_Bool cur_prev)
{
   if (obj->prev->clipper)
     {
        Evas_Object_Protected_Data *clip = obj->prev->clipper;
        if (!cur_prev)
          {
             EINA_COW_STATE_WRITE_BEGIN(obj, state_write, prev)
               state_write->clipper = NULL;
             EINA_COW_STATE_WRITE_END(obj, state_write, prev);
          }
        if (clip != obj->cur->clipper)
          efl_event_callback_del(clip->object, EFL_EVENT_INVALIDATE, _clipper_invalidated_cb, obj->object);
     }
}

/**
 * @brief Get a list of objects that are clipped by @p eo_obj.
 * @param eo_obj The object to get the clipees of.
 * @return A new Eina_List of Evas_Object pointers that are clipped by @p eo_obj.
 *         This list must be freed using eina_list_free() when no longer needed.
 *         The contents of the list (the Evas_Object pointers) should not be freed.
 *         Returns NULL on failure or if @p eo_obj has no clipees.
 * @ingroup Evas_Object_Group_Clipping
 *
 * This function returns a list of all Evas objects that are currently being
 * clipped by the given object @p eo_obj.
 * The returned list is a new list and is cached internally. Subsequent calls
 * without changes to the clipee list will return a new list with the same content
 * but the internal cache is cleared on modification or next call.
 *
 * Example:
 * @code
 * Evas_Object *clipper = evas_object_rectangle_add(evas);
 * Evas_Object *clipped1 = evas_object_rectangle_add(evas);
 * Evas_Object *clipped2 = evas_object_image_add(evas);
 *
 * evas_object_clip_set(clipped1, clipper);
 * evas_object_clip_set(clipped2, clipper);
 *
 * const Eina_List *clipees = evas_object_clipees_get(clipper);
 * Evas_Object *obj;
 * Eina_List *l;
 * EINA_LIST_FOREACH(clipees, l, obj)
 *   {
 *      printf("Object %p is clipped by %p\n", obj, clipper);
 *   }
 * eina_list_free(clipees); // Free the list, not its content
 * @endcode
 */
EVAS_API const Eina_List *
evas_object_clipees_get(const Evas_Object *eo_obj)
{
   const Evas_Object_Protected_Data *tmp;
   Eina_List *l;
   Eina_List *answer = NULL;

   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj, NULL);
   obj->clip.cache_clipees_answer = eina_list_free(obj->clip.cache_clipees_answer);

   EINA_LIST_FOREACH(obj->clip.clipees, l, tmp)
     answer = eina_list_append(answer, tmp->object);

   obj->clip.cache_clipees_answer = answer;
   return answer;
}

/**
 * @brief Query whether an object has any clipees.
 * @param eo_obj The object to query.
 * @return EINA_TRUE if the object clips one or more other objects,
 *         EINA_FALSE otherwise (or on errors).
 * @ingroup Evas_Object_Group_Clipping
 *
 * This function checks if @p eo_obj is currently acting as a clipper for any
 * other Evas objects.
 */
EVAS_API Eina_Bool
evas_object_clipees_has(const Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = EVAS_OBJ_GET_OR_RETURN(eo_obj, EINA_FALSE);
   return !!obj->clip.clipees;
}

typedef struct
{
   Eina_Iterator  iterator;
   Eina_List     *list;
   Eina_Iterator *real_iterator;
   Evas_Object   *object; /**< The clipper object this iterator is for. */
} Clipee_Iterator;

/**
 * @internal
 * @brief Advances the clipee iterator to the next clipee.
 *
 * Implements the `next` function for the Eina_Iterator interface for clipees.
 *
 * @param it The Clipee_Iterator instance.
 * @param data Pointer to a `void*` where the next Evas_Object (clipee) will be stored.
 * @return EINA_TRUE if a next element is available, EINA_FALSE otherwise.
 */
static Eina_Bool
_clipee_iterator_next(Clipee_Iterator *it, void **data)
{
   Evas_Object_Protected_Data *sub;

   if (!eina_iterator_next(it->real_iterator, (void **) &sub))
     return EINA_FALSE;

   if (data) *data = sub ? sub->object : NULL;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Gets the container (the clipper object) for the clipee iterator.
 *
 * Implements the `get_container` function for the Eina_Iterator interface.
 *
 * @param it The Clipee_Iterator instance.
 * @return The Evas_Object that is the clipper for this iterator.
 */
static void *
_clipee_iterator_get_container(Clipee_Iterator *it)
{
   return it->object;
}

/**
 * @internal
 * @brief Frees the clipee iterator.
 *
 * Implements the `free` function for the Eina_Iterator interface.
 * Frees the internal list iterator and the Clipee_Iterator structure itself.
 *
 * @param it The Clipee_Iterator instance to free.
 */
static void
_clipee_iterator_free(Clipee_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   free(it);
}

/**
 * @internal
 * @brief Gets an iterator for the objects clipped by this Evas object.
 * @since 1.26
 *
 * @param eo_obj The Evas object (clipper).
 * @param obj The protected data of `eo_obj`.
 * @return An Eina_Iterator that yields Evas_Object pointers (clipees).
 *         The iterator must be freed using eina_iterator_free() when no longer needed.
 *         Returns NULL on failure.
 */
EOLIAN Eina_Iterator *
_efl_canvas_object_clipped_objects_get(const Eo *eo_obj, Evas_Object_Protected_Data *obj)
{
   Clipee_Iterator *it;

   it = calloc(1, sizeof(*it));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->list = obj->clip.clipees;
   it->real_iterator = eina_list_iterator_new(it->list);
   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_clipee_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_clipee_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_clipee_iterator_free);
   it->object = (Evas_Object *)eo_obj;

   return &it->iterator;
}

/**
 * @internal
 * @brief Counts the number of objects clipped by this Evas object.
 * @since 1.26
 *
 * @param eo_obj The Evas object (clipper).
 * @param obj The protected data of `eo_obj`.
 * @return The number of objects clipped by `eo_obj`.
 */
EOLIAN unsigned int
_efl_canvas_object_clipped_objects_count(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   return eina_list_count(obj->clip.clipees);
}

/**
 * @internal
 * @brief Sets the 'no_render' flag for an Evas object.
 * @since 1.26
 *
 * If `enable` is EINA_TRUE, this object will not be rendered. This is an
 * optimization and can be used for objects that are part of the scene graph
 * for logic (e.g., as clippers, event catchers) but should not be visually drawn.
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @param enable EINA_TRUE to enable no_render, EINA_FALSE to disable.
 */
EOLIAN void
_efl_canvas_object_no_render_set(Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, Eina_Bool enable)
{
   obj->no_render = !!enable;
}

/**
 * @internal
 * @brief Gets the 'no_render' flag for an Evas object.
 * @since 1.26
 *
 * @param eo_obj The Evas object.
 * @param obj The protected data of the Evas object.
 * @return EINA_TRUE if the object is set to no_render, EINA_FALSE otherwise.
 */
EOLIAN Eina_Bool
_efl_canvas_object_no_render_get(const Eo *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   return obj->no_render;
}
