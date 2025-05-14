/**
 * @file
 * @brief This file implements the Efl_Canvas_Event_Grabber class, which is an
 * invisible object used to capture all mouse and key events within its geometry,
 * preventing them from propagating to objects below it. It also acts as a
 * container for other Evas objects, managing their stacking order.
 */

#include "evas_common_private.h"
#include "evas_private.h"
#include "efl_canvas_event_grabber.eo.h"

#define MY_CLASS EFL_CANVAS_EVENT_GRABBER_CLASS
#define MY_CLASS_NAME "Efl_Object_Event_Grabber"
#define MY_CLASS_NAME_LEGACY "evas_object_event_grabber"

/**
 * @brief Private data structure for the Efl_Canvas_Event_Grabber class.
 */
struct _Efl_Object_Event_Grabber_Data
{
   Eo *rect; /**< Internal rectangle object used for event grabbing and visual representation (though typically transparent). */
   Eina_List *contained; /**< List of Evas_Object_Protected_Data pointers for objects contained within this grabber. */

   Eina_Bool vis : 1; /**< Flag indicating if the grabber (and its internal rectangle) is visible. */
   Eina_Bool freeze : 1; /**< Flag to freeze the stacking order updates when visible. If true, restacking is deferred until unfrozen or made invisible. */
   Eina_Bool restack : 1; /**< Flag indicating if a restack operation is pending due to changes while frozen. */
};

/**
 * @brief Iterator structure for iterating over members of an Efl_Canvas_Event_Grabber.
 */
typedef struct Efl_Object_Event_Grabber_Iterator
{
   Eina_Iterator iterator; /**< Base Eina_Iterator structure. */

   Eina_Iterator *itl; /**< Internal Eina_List iterator for the contained objects. */
   Eo *parent; /**< Reference to the parent event grabber object. */
} Efl_Object_Event_Grabber_Iterator;

/**
 * @internal
 * @brief Advances the event grabber's member iterator to the next element.
 * @param it The iterator instance.
 * @param data Pointer to store the next element.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise (e.g., end of list).
 */
static Eina_Bool
_efl_canvas_group_group_iterator_next(Efl_Object_Event_Grabber_Iterator *it, void **data)
{
   return eina_iterator_next(it->itl, data);
}

/**
 * @internal
 * @brief Gets the container (parent event grabber) of the iterator.
 * @param it The iterator instance.
 * @return The parent Evas_Object (event grabber).
 */
static Evas_Object *
_efl_canvas_group_group_iterator_get_container(Efl_Object_Event_Grabber_Iterator *it)
{
   return it->parent;
}

/**
 * @internal
 * @brief Frees the event grabber's member iterator.
 * @param it The iterator instance to free.
 */
static void
_efl_canvas_group_group_iterator_free(Efl_Object_Event_Grabber_Iterator *it)
{
   efl_unref(it->parent);
   free(it);
}

/**
 * @internal
 * @brief Creates an iterator for the members of the event grabber group.
 * @param eo_obj The event grabber object.
 * @param pd Private data of the event grabber.
 * @return A new Eina_Iterator for the contained members, or NULL on failure or if empty.
 *
 * This implements the Efl.Canvas.Group.group_members_iterate EOLIAN interface.
 */
EOLIAN static Eina_Iterator*
_efl_canvas_event_grabber_efl_canvas_group_group_members_iterate(const Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Efl_Object_Event_Grabber_Iterator *it;

   if (!pd->contained) return NULL;

   it = calloc(1, sizeof(Efl_Object_Event_Grabber_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->parent = efl_ref(eo_obj);
   it->itl = eina_list_iterator_new(pd->contained);

   it->iterator.next = FUNC_ITERATOR_NEXT(_efl_canvas_group_group_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_efl_canvas_group_group_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_efl_canvas_group_group_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @brief Checks if a given Evas_Object is a direct member of this event grabber.
 * @param eo_obj The event grabber object.
 * @param pd Private data of the event grabber (unused).
 * @param sub_obj The Evas_Object to check.
 * @return EINA_TRUE if sub_obj is a member, EINA_FALSE otherwise.
 *
 * This implements the Efl.Canvas.Group.group_member_is EOLIAN interface.
 * It checks if the `parent` field of the sub_obj's event data points to this grabber.
 */
EOLIAN static Eina_Bool
_efl_canvas_event_grabber_efl_canvas_group_group_member_is(const Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED, const Eo *sub_obj)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *sub = efl_data_scope_safe_get(sub_obj, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);

   if (!sub) return EINA_FALSE;
   return (sub->events->parent == eo_obj);
}

/**
 * @internal
 * @brief Verifies that a child object is not stacked above the event grabber.
 *
 * This function checks if the given object `obj` is stacked at a higher layer
 * than the event grabber's internal rectangle or, if on the same layer,
 * stacked above it. If such a condition is found, a critical error is logged,
 * as this would violate the event grabbing behavior.
 *
 * @param pd Private data of the event grabber.
 * @param obj The protected data of the child object to verify.
 */
static void
_stacking_verify(Efl_Object_Event_Grabber_Data *pd, Evas_Object_Protected_Data *obj)
{
   Eo *grabber;
   Evas_Object_Protected_Data *gobj, *i;

   grabber = efl_parent_get(pd->rect);
   gobj = efl_data_scope_get(grabber, EFL_CANVAS_OBJECT_CLASS);
   if (obj->layer->layer > gobj->layer->layer)
     {
        CRI("Cannot stack child object above event grabber object!");
        return;
     }
   if (obj->layer != gobj->layer) return;

   EINA_INLIST_REVERSE_FOREACH(EINA_INLIST_GET(gobj->layer->objects), i)
     {
        if (i == gobj) break;
        if (i == obj)
          {
             CRI("Cannot stack child object above event grabber object!");
             return;
          }
     }
}

/**
 * @internal
 * @brief Inserts a child object into the event grabber's internal list of contained objects,
 * maintaining stacking order.
 *
 * The object is inserted into `pd->contained` based on its layer and stacking
 * position relative to other members. If the grabber is visible, it also
 * verifies the stacking order to ensure the child is not above the grabber.
 *
 * @param pd Private data of the event grabber.
 * @param obj The protected data of the child object to insert.
 */
static void
_child_insert(Efl_Object_Event_Grabber_Data *pd, Evas_Object_Protected_Data *obj)
{
   Evas_Object_Protected_Data *a, *i;
   Eina_List *l;
   Eina_Bool found = EINA_FALSE;

   if (pd->vis) _stacking_verify(pd, obj);

   EINA_LIST_REVERSE_FOREACH(pd->contained, l, a)
     {
        if (a->object == pd->rect)
          {
             found = EINA_TRUE;
             break;
          }
        if (a->layer->layer > obj->layer->layer) continue;
        if (a->layer->layer < obj->layer->layer)
          {
             /* new object is higher layer than 'a' */
             found = EINA_TRUE;
             break;
          }
        EINA_INLIST_FOREACH(EINA_INLIST_GET(a->layer->objects), i)
          {
             if (obj == i)
               {
                  /* new object is below 'a' */
                  pd->contained = eina_list_prepend_relative(pd->contained, obj, a);
                  return;
               }
             else if (a == i)
               {
                  /* new object is above 'a' */
                  found = EINA_TRUE;
                  break;
               }
          }
        if (found) break;
     }

   if (found)
     pd->contained = eina_list_append_relative(pd->contained, obj, a);
   else
     pd->contained = eina_list_prepend(pd->contained, obj);
}

/**
 * @internal
 * @brief Performs a full restack of all contained objects.
 *
 * This function is called when the stacking order might have become significantly
 * disorganized, for example, after being frozen and then unfrozen with pending
 * restack operations. It clears the `pd->contained` list (preserving the
 * internal rectangle `pd->rect`) and re-inserts all other members one by one
 * using `_child_insert` to ensure correct order.
 *
 * @param pd Private data of the event grabber.
 */
static void
_full_restack(Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;
   Evas_Object_Protected_Data *root = NULL;
   Eina_List *list = NULL;
   EINA_LIST_FREE(pd->contained, obj)
     {
        if (obj->object == pd->rect)
          {
             root = obj;
             continue;
          }
        list = eina_list_append(list, obj);
     }

   pd->contained = eina_list_append(pd->contained, root);
   EINA_LIST_FREE(list, obj)
     _child_insert(pd, obj);
   pd->restack = 0;
}

/**
 * @internal
 * @brief Event callback for when a child member's stacking changes.
 *
 * This function is triggered by the `EFL_GFX_ENTITY_EVENT_STACKING_CHANGED`
 * event on a member object. If the grabber is visible and frozen, it flags
 * that a restack is needed. Otherwise, it removes and re-inserts the child
 * using `_child_insert` to update its position in the `pd->contained` list.
 *
 * @param data The event grabber's private data (pd).
 * @param event The Efl_Event details. The event->object is the child whose stacking changed.
 */
static void
_efl_canvas_object_event_grabber_child_restack(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;
   Evas_Object_Protected_Data *obj = efl_data_scope_get(event->object, EFL_CANVAS_OBJECT_CLASS);

   if (pd->vis && pd->freeze)
     {
        pd->restack = 1;
        return;
     }
   pd->contained = eina_list_remove(pd->contained, obj);
   _child_insert(pd, obj);
}

/**
 * @internal
 * @brief Event callback for when a child member is invalidated (e.g., about to be deleted).
 *
 * This function is triggered by the `EFL_EVENT_INVALIDATE` event on a member
 * object. It removes the child from the event grabber's group.
 *
 * @param data The event grabber's private data (pd).
 * @param event The Efl_Event details. The event->object is the child being invalidated.
 */
static void
_efl_canvas_object_event_grabber_child_invalidate(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;

   efl_canvas_group_member_remove(efl_parent_get(pd->rect), event->object);
}

/**
 * @internal
 * @brief Adds a sub-object as a member of the event grabber group.
 * @param eo_obj The event grabber object.
 * @param pd Private data of the event grabber.
 * @param eo_sub The Evas_Object to add as a member.
 *
 * This implements the Efl.Canvas.Group.group_member_add EOLIAN interface.
 * It performs safety checks (e.g., not adding deleted objects, ensuring
 * objects are on the same canvas), sets the sub-object's parent to this
 * grabber, inserts it into the internal ordered list, and sets up event
 * callbacks for invalidation and stacking changes of the member.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_canvas_group_group_member_add(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, Eo *eo_sub)
{
   Evas_Object_Protected_Data *sub = efl_data_scope_safe_get(eo_sub, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   EINA_SAFETY_ON_NULL_RETURN(sub);
   EINA_SAFETY_ON_NULL_RETURN(obj);

   if (eo_sub != pd->rect)
     {
        if (sub->delete_me)
          {
             CRI("Can not add deleted member %p to event grabber %p", eo_sub, eo_obj);
             return;
          }
        if (obj->delete_me)
          {
             CRI("Can not add object %p to deleted event grabber %p", eo_sub, eo_obj);
             return;
          }
        if (!obj->layer)
          {
             CRI("Can not add object %p to event grabber %p: event grabber has "
                 "no associated canvas.", eo_sub, eo_obj);
             return;
          }
        if (!sub->layer)
          {
             CRI("Can not add object %p to event grabber %p: member has "
                 "no associated canvas.", eo_sub, eo_obj);
             return;
          }
        if ((sub->layer && obj->layer) &&
            (sub->layer->evas != obj->layer->evas))
          {
             CRI("Can not add object %p to event grabber %p: objects belong to "
                 "different canvases.", eo_sub, eo_obj);
             return;
          }
     }
   if (sub->events->parent == eo_obj) return;

   if (sub->smart.parent || sub->events->parent) evas_object_smart_member_del(eo_sub);
   EINA_COW_WRITE_BEGIN(evas_object_events_cow, sub->events, Evas_Object_Events_Data, events)
     events->parent = eo_obj;
   EINA_COW_WRITE_END(evas_object_events_cow, sub->events, events);
   _child_insert(pd, sub);
   efl_event_callback_add(eo_sub, EFL_EVENT_INVALIDATE, _efl_canvas_object_event_grabber_child_invalidate, pd);
   if (eo_sub != pd->rect)
     efl_event_callback_add(eo_sub, EFL_GFX_ENTITY_EVENT_STACKING_CHANGED, _efl_canvas_object_event_grabber_child_restack, pd);
}

EOLIAN static void
_efl_canvas_event_grabber_efl_canvas_group_group_member_remove(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eo *member)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(member, EFL_CANVAS_OBJECT_CLASS);

   // Remove event callbacks associated with this member
   efl_event_callback_del(member, EFL_EVENT_INVALIDATE, _efl_canvas_object_event_grabber_child_invalidate, pd);
   efl_event_callback_del(member, EFL_GFX_ENTITY_EVENT_STACKING_CHANGED, _efl_canvas_object_event_grabber_child_restack, pd);
   pd->contained = eina_list_remove(pd->contained, obj); // Remove from internal list

   // Clear the parent link in the member's event data
   EINA_COW_WRITE_BEGIN(evas_object_events_cow, obj->events, Evas_Object_Events_Data, events)
     events->parent = NULL;
   EINA_COW_WRITE_END(evas_object_events_cow, obj->events, events);
}

/**
 * @internal
 * @brief Handles group changes. Currently a no-op for event grabber.
 * This implements the Efl.Canvas.Group.group_change EOLIAN interface.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_canvas_group_group_change(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{}

/**
 * @internal
 * @brief Handles group calculations. Currently a no-op for event grabber.
 * This implements the Efl.Canvas.Group.group_calculate EOLIAN interface.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_canvas_group_group_calculate(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{}

/**
 * @internal
 * @brief Sets the need_recalculate flag. Currently a no-op for event grabber.
 * This implements the Efl.Canvas.Group.group_need_recalculate_set EOLIAN interface.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_canvas_group_group_need_recalculate_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED, Eina_Bool set EINA_UNUSED)
{}

/**
 * @internal
 * @brief Gets the need_recalculate flag. Always returns EINA_FALSE for event grabber.
 * This implements the Efl.Canvas.Group.group_need_recalculate_get EOLIAN interface.
 */
EOLIAN static Eina_Bool
_efl_canvas_event_grabber_efl_canvas_group_group_need_recalculate_get(const Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets the position of the event grabber.
 * @param eo_obj The event grabber object.
 * @param pd Private data of the event grabber.
 * @param pos The new position (x, y).
 *
 * This implements the Efl.Gfx.Entity.position_set EOLIAN interface.
 * It sets the position for both the grabber itself and its internal rectangle.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_gfx_entity_position_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, Eina_Position2D pos)
{
   efl_gfx_entity_position_set(efl_super(eo_obj, MY_CLASS), pos);
   efl_gfx_entity_position_set(pd->rect, pos);
}

/**
 * @internal
 * @brief Sets the size of the event grabber.
 * @param eo_obj The event grabber object.
 * @param pd Private data of the event grabber.
 * @param sz The new size (width, height).
 *
 * This implements the Efl.Gfx.Entity.size_set EOLIAN interface.
 * It sets the size for both the grabber itself and its internal rectangle.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_gfx_entity_size_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, Eina_Size2D sz)
{
   efl_gfx_entity_size_set(efl_super(eo_obj, MY_CLASS), sz);
   efl_gfx_entity_size_set(pd->rect, sz);
}

/**
 * @internal
 * @brief Gets the visibility state of the event grabber.
 * @param eo_obj The event grabber object (unused).
 * @param pd Private data of the event grabber.
 * @return EINA_TRUE if visible, EINA_FALSE otherwise.
 *
 * This implements the Efl.Gfx.Entity.visible_get EOLIAN interface.
 */
EOLIAN static Eina_Bool
_efl_canvas_event_grabber_efl_gfx_entity_visible_get(const Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd)
{
   return pd->vis;
}

/**
 * @internal
 * @brief Sets the visibility state of the event grabber.
 * @param eo_obj The event grabber object (unused).
 * @param pd Private data of the event grabber.
 * @param set EINA_TRUE to make visible, EINA_FALSE to hide.
 *
 * This implements the Efl.Gfx.Entity.visible_set EOLIAN interface.
 * When making visible, it verifies stacking of all children.
 * It also sets the visibility of the internal rectangle.
 * If becoming invisible while frozen with a pending restack, it performs the full restack.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_gfx_entity_visible_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eina_Bool set)
{
   if (set)
     {
        Evas_Object_Protected_Data *obj;
        Eina_List *l;

        EINA_LIST_FOREACH(pd->contained, l, obj)
          if (obj->object != pd->rect) _stacking_verify(pd, obj);
     }
   pd->vis = !!set;
   efl_gfx_entity_visible_set(pd->rect, set);
   if (pd->restack && pd->freeze && (!set))
     _full_restack(pd);
}

EOLIAN static void
_efl_canvas_event_grabber_efl_gfx_stack_layer_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, short l)
{
   efl_gfx_stack_layer_set(efl_super(eo_obj, MY_CLASS), l);
   efl_gfx_stack_layer_set(pd->rect, l); // Also set layer for the internal rectangle
}

/**
 * @internal
 * @brief Event callback for when the event grabber itself is restacked.
 *
 * This function is triggered by the `EFL_GFX_ENTITY_EVENT_STACKING_CHANGED`
 * event on the event grabber object itself. It ensures the internal rectangle
 * `pd->rect` is stacked appropriately relative to the grabber (same layer,
 * just below). It then flags a need for a full restack of its children,
 * which is performed immediately unless the grabber is visible and frozen.
 *
 * @param data The event grabber's private data (pd).
 * @param event The Efl_Event details. event->object is the event grabber.
 */
static void
_efl_canvas_object_event_grabber_restack(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;

   evas_object_layer_set(pd->rect, evas_object_layer_get(event->object));
   evas_object_stack_below(pd->rect, event->object);

   pd->restack = 1;
   if (pd->vis && pd->freeze) return;
   _full_restack(pd);
}

/**
 * @internal
 * @brief Constructor for the Efl_Canvas_Event_Grabber object.
 * @param eo_obj The object being constructed.
 * @param pd Private data for the object.
 * @return The constructed object.
 *
 * This implements the Efl.Object.constructor EOLIAN interface.
 * It initializes the event grabber, sets its type, creates an internal
 * rectangle object (`pd->rect`) for event handling, makes it transparent,
 * and sets up a callback for its own stacking changes.
 */
EOLIAN static Eo *
_efl_canvas_event_grabber_efl_object_constructor(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;

   efl_canvas_group_clipped_set(eo_obj, EINA_FALSE); // Event grabber itself is not a clipper
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));
   efl_canvas_object_type_set(eo_obj, MY_CLASS_NAME_LEGACY);
   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   obj->is_event_parent = 1;
   obj->is_smart = 0;

   efl_event_callback_add(eo_obj, EFL_GFX_ENTITY_EVENT_STACKING_CHANGED, _efl_canvas_object_event_grabber_restack, pd);
   pd->rect = evas_object_rectangle_add(efl_parent_get(eo_obj));
   evas_object_pointer_mode_set(pd->rect, EVAS_OBJECT_POINTER_MODE_NOGRAB);
   efl_parent_set(pd->rect, eo_obj);
   efl_canvas_group_member_add(eo_obj, pd->rect);
   evas_object_color_set(pd->rect, 0, 0, 0, 0);
   return eo_obj;
}

/**
 * @internal
 * @brief Destructor for the Efl_Canvas_Event_Grabber object.
 * @param eo_obj The object being destructed.
 * @param pd Private data for the object.
 *
 * This implements the Efl.Object.destructor EOLIAN interface.
 * It removes all contained members (which also deletes `pd->rect` as it's a member)
 * and then proceeds with the superclass destructor.
 */
EOLIAN static void
_efl_canvas_event_grabber_efl_object_destructor(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;
   Eina_List *l, *ln;

   // Note: pd->rect is part of pd->contained, so it will be removed and deleted here.
   EINA_LIST_FOREACH_SAFE(pd->contained, l, ln, obj)
     efl_canvas_group_member_remove(eo_obj, obj->object);
   efl_canvas_group_del(eo_obj); // Deletes the group aspects, including pd->rect
   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

/**
 * @internal
 * @brief Class constructor for Efl_Canvas_Event_Grabber.
 * @param klass The Efl_Class being constructed.
 *
 * Registers the legacy type name for this class.
 */
static void
_efl_canvas_event_grabber_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Retrieves the list of members contained within an event grabber.
 * @param eo_obj The event grabber object.
 * @return A const Eina_List* of Evas_Object_Protected_Data pointers.
 *         The list is owned by the event grabber and should not be modified or freed.
 *         Returns NULL if the object is invalid or has no members.
 * @note This is a legacy accessor. Prefer Efl.Canvas.Group iterators.
 * The list contains Evas_Object_Protected_Data* elements, not Eo*.
 * Example:
 * @code
 * const Eina_List *members = evas_object_event_grabber_members_list(my_grabber);
 * Eina_List *l;
 * Evas_Object_Protected_Data *member_pd;
 * EINA_LIST_FOREACH(members, l, member_pd)
 *   {
 *      Eo *member_obj = member_pd->object;
 *      // Do something with member_obj
 *   }
 * @endcode
 */
const Eina_List *
evas_object_event_grabber_members_list(const Eo *eo_obj)
{
   Efl_Object_Event_Grabber_Data *pd = efl_data_scope_get(eo_obj, MY_CLASS);
   if (!pd) return NULL;
   return pd->contained;
}

/**
 * @internal
 * @brief Sets whether to freeze restacking operations when the grabber is visible.
 * @param eo_obj The event grabber object (unused).
 * @param pd Private data of the event grabber.
 * @param set EINA_TRUE to freeze, EINA_FALSE to unfreeze.
 *
 * If unfreezing (`set` is EINA_FALSE) and the grabber is visible with a pending
 * restack, a full restack is performed.
 */
EOLIAN static void
_efl_canvas_event_grabber_freeze_when_visible_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eina_Bool set)
{
   set = !!set;
   if (pd->freeze == set) return;
   pd->freeze = set;
   if (pd->vis && pd->restack && (!set))
     _full_restack(pd);
}

/**
 * @internal
 * @brief Gets whether restacking operations are frozen when the grabber is visible.
 * @param eo_obj The event grabber object (unused).
 * @param pd Private data of the event grabber.
 * @return EINA_TRUE if frozen, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_canvas_event_grabber_freeze_when_visible_get(const Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd)
{
   return pd->freeze;
}

/**
 * @brief Adds a new event grabber object to the given Evas canvas.
 * @param eo_e The Evas canvas (parent) to add the object to.
 * @return The new Evas_Object (event grabber) on success, or NULL on failure.
 *
 * This is the legacy C API function to create an event grabber.
 * The event grabber is an invisible object that can be used to "grab" all
 * mouse and key events within its geometry, preventing them from propagating
 * to objects visually below it.
 *
 * Example:
 * @code
 * Evas *evas = evas_canvas_new(engine_info_get(), NULL, 100, 100);
 * Evas_Object *grabber = evas_object_event_grabber_add(evas);
 * evas_object_move(grabber, 10, 10);
 * evas_object_resize(grabber, 50, 50);
 * evas_object_show(grabber);
 * @endcode
 */
EVAS_API Evas_Object *
evas_object_event_grabber_add(Evas *eo_e)
{
   eo_e = evas_find(eo_e); // Ensure we have the actual Evas canvas object
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(eo_e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, eo_e, efl_canvas_object_legacy_ctor(efl_added));
}

#include "efl_canvas_event_grabber.eo.c"
#include "efl_canvas_event_grabber_eo.legacy.c"
