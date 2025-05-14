#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

//we need those for legacy compatible code
#include "elm_genlist_eo.h"
#include "elm_gengrid_eo.h"

/**
 * @file
 * @brief These functions provide legacy support for focus handling in Elementary.
 *
 * They are intended for compatibility with older code and may not be suitable for new development.
 * It is recommended to use the new EFL focus manager API for new projects.
 */

#define API_ENTRY()\
   EINA_SAFETY_ON_NULL_RETURN(obj); \
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(obj, EFL_UI_WIDGET_CLASS)); \
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, pd); \
   EINA_SAFETY_ON_FALSE_RETURN(elm_widget_is_legacy(obj));

#define API_ENTRY_VAL(val)\
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, val); \
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(obj, EFL_UI_WIDGET_CLASS), val); \
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, pd, val); \
   EINA_SAFETY_ON_FALSE_RETURN_VAL(elm_widget_is_legacy(obj), val);

#define MARK_WINDOW_LEGACY_USAGE() \
   if (pd->shared_win_data) \
     ((Efl_Ui_Shared_Win_Data*)pd->shared_win_data)->legacy_focus_api_used = EINA_TRUE;

#define MAPPING() \
        MAP(PREVIOUS, prev) \
        MAP(NEXT, next) \
        MAP(UP, up) \
        MAP(DOWN, down) \
        MAP(LEFT, left) \
        MAP(RIGHT, right)

/**
 * @internal
 * @brief Retrieves the custom focus chain for a given widget.
 *
 * @param node The widget whose custom focus chain is to be retrieved.
 * @return A list of widgets in the custom focus chain, or @c NULL if no custom chain is set.
 *         The list is owned by the widget and should not be modified or freed by the caller.
 */
static Eina_List*
_custom_chain_get(const Efl_Ui_Widget *node)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(node, pd, NULL);

   return pd->legacy_focus.custom_chain;
}

/**
 * @internal
 * @brief Updates the focus manager with the current focus order of the widget.
 *
 * This function is called when the custom focus chain or the children of a widget change.
 * It informs the focus manager about the new order of focusable elements.
 *
 * @param obj The widget whose focus manager needs to be updated.
 * @param pd The smart data of the widget.
 */
static void
_flush_manager(Efl_Ui_Widget *obj, Elm_Widget_Smart_Data *pd)
{
   Efl_Ui_Focus_Manager *manager;

   manager = efl_ui_focus_object_focus_manager_get(obj);
   if (manager)
     {
        Eina_List *order = NULL;

        if (pd->legacy_focus.custom_chain)
          order = eina_list_clone(pd->legacy_focus.custom_chain);
        else
          {
             for (unsigned int i = 0; i < eina_array_count(pd->children); ++i)
               {
                  Eo *sobj = eina_array_data_get(pd->children, i);
                  order = eina_list_append(order, sobj);
               }
          }

        efl_ui_focus_manager_calc_update_order(manager, obj, order);
     }
}

/**
 * @internal
 * @brief Callback function invoked when the focus manager of a widget changes.
 *
 * This typically happens when a widget is added to or removed from a container
 * that manages focus. It ensures that the manager is updated with the widget's
 * custom focus chain if one exists.
 *
 * @param data User data, not used in this function.
 * @param ev The event information, containing the widget whose manager changed.
 */
static void
_manager_changed(void *data EINA_UNUSED, const Efl_Event *ev)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(ev->object, pd);

   _flush_manager(ev->object, pd);
}

/**
 * @internal
 * @brief Sets or unsets the custom focus chain for a widget.
 *
 * If @p lst is @c NULL, the custom focus chain is removed. Otherwise, @p lst
 * becomes the new custom focus chain. The function also handles registering or
 * unregistering for focus manager change events based on whether a custom chain is active.
 *
 * @param node The widget for which to set the custom focus chain.
 * @param lst A list of widgets representing the custom focus chain. The list items
 *            must be Efl_Ui_Widget instances and direct children of @p node.
 *            The function takes ownership of this list if it's not @c NULL.
 */
static void
_custom_chain_set(Efl_Ui_Widget *node, Eina_List *lst)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(node, pd);
   Efl_Ui_Widget *list_item;
   Eina_List *n;

   pd->legacy_focus.custom_chain = eina_list_free(pd->legacy_focus.custom_chain);
   pd->legacy_focus.custom_chain = lst;

   EINA_LIST_FOREACH(pd->legacy_focus.custom_chain, n, list_item)
     {
        EINA_SAFETY_ON_FALSE_RETURN(efl_isa(list_item, EFL_UI_WIDGET_CLASS));
        EINA_SAFETY_ON_FALSE_RETURN(efl_ui_widget_parent_get(list_item) == node);
     }

   _elm_widget_full_eval_children(node, pd);

   if (pd->legacy_focus.custom_chain && !pd->legacy_focus.listen_to_manager)
     {
        efl_event_callback_add(node, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_MANAGER_CHANGED, _manager_changed, NULL);
        pd->legacy_focus.listen_to_manager = EINA_TRUE;
     }
   else if (!pd->legacy_focus.custom_chain && pd->legacy_focus.listen_to_manager)
     {
        efl_event_callback_del(node, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_MANAGER_CHANGED, _manager_changed, NULL);
        pd->legacy_focus.listen_to_manager = EINA_FALSE;
     }

   _flush_manager(node, pd);
}

/**
 * @brief Sets the next object to focus to in a given direction for legacy focus handling.
 * @param obj The current Evas_Object.
 * @param next The Evas_Object to focus next.
 * @param dir The direction for this focus link.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_next_object_set(Evas_Object        *obj,
                                 Evas_Object        *next,
                                 Elm_Focus_Direction dir)
{
   API_ENTRY()
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(next, EFL_UI_WIDGET_CLASS));
   ELM_WIDGET_DATA_GET_OR_RETURN(next, next_pd);
   MARK_WINDOW_LEGACY_USAGE()

   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction) pd->legacy_focus.field = next;
   MAPPING()
   #undef MAP
   dir = (Elm_Focus_Direction)efl_ui_focus_util_direction_complement((Efl_Ui_Focus_Direction)dir);
   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction) next_pd->legacy_focus.field = obj;
   MAPPING()
   #undef MAP
}

/**
 * @brief Sets a custom focus chain for an object.
 *
 * This replaces any existing custom focus chain. The objects in the list
 * define a specific order for focus movement within this object when
 * navigating with next/previous.
 *
 * @param obj The Evas_Object to set the custom focus chain on.
 * @param objs A list (Eina_List *) of Evas_Object children to be in the chain.
 *             The list itself is not modified, but its contents are used.
 *             The objects in the list must be children of @p obj.
 *             Example:
 *             Eina_List *chain = NULL;
 *             chain = eina_list_append(chain, child1);
 *             chain = eina_list_append(chain, child2);
 *             elm_object_focus_custom_chain_set(parent, chain);
 *             // The 'chain' list can be freed by the caller after this,
 *             // as the function internally clones or processes it.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_custom_chain_set(Evas_Object *obj,
                                  Eina_List   *objs)
{
   API_ENTRY()
   MARK_WINDOW_LEGACY_USAGE()

   _custom_chain_set(obj, objs);
}

/**
 * @brief Unsets any custom focus chain on an object.
 *
 * After calling this, focus behavior reverts to the default (e.g., based on widget hierarchy).
 *
 * @param obj The Evas_Object from which to unset the custom focus chain.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_custom_chain_unset(Evas_Object *obj)
{
   API_ENTRY()
   MARK_WINDOW_LEGACY_USAGE()

   _custom_chain_set(obj, NULL);
}

/**
 * @brief Gets the custom focus chain for an object, if any.
 *
 * @param obj The Evas_Object to get the custom focus chain from.
 * @return A const Eina_List * of Evas_Object children in the custom chain,
 *         or @c NULL if no custom chain is set. The returned list is
 *         owned by the object and must not be modified or freed.
 *         Example of iterating:
 *         const Eina_List *chain = elm_object_focus_custom_chain_get(parent);
 *         const Eina_List *l;
 *         Evas_Object *child;
 *         EINA_LIST_FOREACH(chain, l, child) {
 *           // process child
 *         }
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI const Eina_List *
elm_object_focus_custom_chain_get(const Evas_Object *obj)
{
   API_ENTRY_VAL(NULL)

   return _custom_chain_get(obj);
}

/**
 * @brief Appends a child to the custom focus chain of an object.
 *
 * If @p relative_child is @c NULL, @p child is appended to the end of the chain.
 * Otherwise, @p child is inserted after @p relative_child.
 * If no custom chain exists, a new one is created with @p child.
 *
 * @param obj The Evas_Object whose custom focus chain is to be modified.
 * @param child The Evas_Object (child of @p obj) to append.
 * @param relative_child If not @c NULL, @p child is inserted after this object in the chain.
 *                       Must also be a child of @p obj and already in the chain.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_custom_chain_append(Evas_Object *obj,
                                     Evas_Object *child,
                                     Evas_Object *relative_child)
{
   API_ENTRY()
   MARK_WINDOW_LEGACY_USAGE()
   Eina_List *tmp;

   tmp = eina_list_clone(pd->legacy_focus.custom_chain);
   tmp = eina_list_append_relative(tmp, child, relative_child);
   _custom_chain_set(obj, tmp);
}

/**
 * @brief Prepends a child to the custom focus chain of an object.
 *
 * If @p relative_child is @c NULL, @p child is prepended to the beginning of the chain.
 * Otherwise, @p child is inserted before @p relative_child.
 * If no custom chain exists, a new one is created with @p child.
 *
 * @param obj The Evas_Object whose custom focus chain is to be modified.
 * @param child The Evas_Object (child of @p obj) to prepend.
 * @param relative_child If not @c NULL, @p child is inserted before this object in the chain.
 *                       Must also be a child of @p obj and already in the chain.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_custom_chain_prepend(Evas_Object *obj,
                                      Evas_Object *child,
                                      Evas_Object *relative_child)
{
   API_ENTRY()
   MARK_WINDOW_LEGACY_USAGE()
   Eina_List *tmp;

   tmp = eina_list_clone(pd->legacy_focus.custom_chain);
   tmp = eina_list_prepend_relative(tmp, child, relative_child);
   _custom_chain_set(obj, tmp);
}

/**
 * @brief Moves focus to the next object in a specified direction, cycling within the current context.
 * @deprecated Use elm_object_focus_next() instead.
 * @param obj The starting Evas_Object.
 * @param dir The direction to cycle focus.
 * @ingroup Elm_Focus_Group_Legacy
 */
EINA_DEPRECATED EAPI void
elm_object_focus_cycle(Evas_Object        *obj,
                       Elm_Focus_Direction dir)
{
   elm_object_focus_next(obj, dir);
}

/**
 * @internal
 * @brief Retrieves the legacy focus target in a specific direction.
 *
 * This function checks for explicitly set next focusable items or objects
 * associated with the widget's legacy focus data.
 *
 * @param eo The current Evas_Object (unused in current implementation, but kept for signature compatibility).
 * @param pd The smart data of the widget being queried.
 * @param dir The direction of focus movement.
 * @return The Evas_Object that is the legacy target in the given direction, or @c NULL if none is set.
 */
static Evas_Object*
_get_legacy_target(EINA_UNUSED Evas_Object *eo, Elm_Widget_Smart_Data *pd, Elm_Focus_Direction dir)
{
   Evas_Object *result = NULL;

   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction && pd->legacy_focus.item_ ##field) result = elm_object_item_widget_get(pd->legacy_focus.item_ ##field);
   MAPPING()
   #undef MAP

   if (!result)
     {
        #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction && pd->legacy_focus.field) result = pd->legacy_focus.field;
        MAPPING()
        #undef MAP
     }

   return result;
}

/**
 * @internal
 * @brief Generates an array representing the focus parent chain of a given focus object.
 *
 * The array contains the object itself as the first element, followed by its focus parent,
 * its parent's focus parent, and so on, up to the root of the focus hierarchy.
 *
 * @param obj The focus object for which to generate the parent chain.
 * @return A new Eina_Array* containing Efl_Ui_Focus_Object pointers. The caller is
 *         responsible for freeing this array using eina_array_free().
 *         Example structure: [obj, focus_parent(obj), focus_parent(focus_parent(obj)), ...]
 */
static Eina_Array*
_focus_parent_chain_gen(Efl_Ui_Focus_Object *obj)
{
   Eina_Array *result = eina_array_new(5);

   for (Eo *parent = obj; parent; parent = efl_ui_focus_object_focus_parent_get(parent))
     {
        eina_array_push(result, parent);
     }

   return result;
}

/**
 * @brief Moves focus to the next focusable object in the specified direction.
 *
 * This function attempts to honor legacy focus settings (e.g., objects set via
 * elm_object_focus_next_object_set()) first. If no legacy target is found,
 * it falls back to the modern EFL focus manager's logic for determining the next
 * focus object.
 *
 * @param obj The Evas_Object from which to start the focus movement.
 * @param dir The direction in which to move focus (e.g., ELM_FOCUS_NEXT, ELM_FOCUS_UP).
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_next(Evas_Object        *obj,
                      Elm_Focus_Direction dir)
{
   Eina_Bool legacy_focus_move = EINA_FALSE;
   Efl_Ui_Widget *o = NULL, *top;
   Efl_Ui_Focus_Object *logical;
   Efl_Ui_Focus_Manager *manager_top;
   API_ENTRY()

   top = elm_object_top_widget_get(obj);
   EINA_SAFETY_ON_FALSE_RETURN(efl_isa(top, EFL_UI_WIN_CLASS));

   manager_top = efl_ui_focus_util_active_manager(obj);
   logical = efl_ui_focus_manager_focus_get(manager_top);

   if (elm_widget_is(logical))
     {
        Efl_Ui_Focus_Object *legacy_target = NULL;
        ELM_WIDGET_DATA_GET_OR_RETURN(logical, pd_logical);

        legacy_target = _get_legacy_target(obj, pd_logical, dir);

        if (!legacy_target)
          {
             Eina_Array *old_chain = _focus_parent_chain_gen(logical);
             Eina_Array *new_chain = _focus_parent_chain_gen(efl_ui_focus_manager_request_move(top, (Efl_Ui_Focus_Direction)dir, NULL, EINA_FALSE));

             //first pop off all elements that are the same
             while (eina_array_count(new_chain) > 0 && eina_array_count(old_chain) > 0 &&
                    eina_array_data_get(new_chain, (int)eina_array_count(new_chain) -1) == eina_array_data_get(old_chain, (int)eina_array_count(old_chain) - 1))
               {
                  eina_array_pop(new_chain);
                  eina_array_pop(old_chain);
               }

             for (unsigned int i = 0; i < eina_array_count(old_chain); ++i)
               {
                  Evas_Object *parent = eina_array_data_get(old_chain, i);
                  if (!elm_widget_is(parent)) continue;
                  ELM_WIDGET_DATA_GET(parent, ppd);
                  if (!ppd)
                    {
                       ERR("Failed to get Elm widget data for parent");
                       break;
                    }
                  legacy_target = _get_legacy_target(parent, ppd, dir);
                  if (legacy_target) break;
               }
             eina_array_free(new_chain);
             eina_array_free(old_chain);
          }

        if (legacy_target)
          {
             efl_ui_focus_util_focus(legacy_target);
             if (elm_object_focused_object_get(top) == legacy_target)
               {
                  legacy_focus_move = EINA_TRUE;
                  o = legacy_target;
               }
          }
     }

   if (!legacy_focus_move)
     o = efl_ui_focus_manager_move(top, (Efl_Ui_Focus_Direction)dir);
   if (!o)
     {
        if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_NEXT || (Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_PREVIOUS)
          {
             Efl_Ui_Focus_Object *root;

             root = efl_ui_focus_manager_root_get(top);
             efl_ui_focus_manager_setup_on_first_touch(top, (Efl_Ui_Focus_Direction)dir, root);
          }
     }
}

/**
 * @brief Gets the next object that would receive focus in a given direction.
 *
 * This function first checks for an explicitly set next object using legacy
 * focus APIs. If none is found, it queries the EFL focus manager for the
 * object that would receive focus if a move were requested in the given direction.
 *
 * @param obj The Evas_Object from which to determine the next focus.
 * @param dir The direction of potential focus movement.
 * @return The Evas_Object that would be focused next, or @c NULL if no such object exists.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI Evas_Object *
elm_object_focus_next_object_get(const Evas_Object  *obj,
                                 Elm_Focus_Direction dir)
{
   Efl_Ui_Widget *top = elm_object_top_widget_get(obj);
   API_ENTRY_VAL(NULL)

   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction && pd->legacy_focus.field) return pd->legacy_focus.field;
   MAPPING()
   #undef MAP

   return efl_ui_focus_manager_request_move(efl_ui_focus_util_active_manager(top), (Efl_Ui_Focus_Direction)dir, NULL, EINA_FALSE);
}

/**
 * @brief Gets the next Elm_Object_Item that would receive focus in a given direction.
 *
 * This function is part of the legacy focus system and primarily checks for
 * items explicitly set via elm_object_focus_next_item_set().
 *
 * @param obj The Evas_Object (typically a container widget) from which to determine the next focus item.
 * @param dir The direction of potential focus movement.
 * @return The Elm_Object_Item that would be focused next, or @c NULL if none is set or applicable.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI Elm_Object_Item *
elm_object_focus_next_item_get(const Evas_Object  *obj,
                               Elm_Focus_Direction dir EINA_UNUSED)
{
   API_ENTRY_VAL(NULL)
   MARK_WINDOW_LEGACY_USAGE()

   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction && pd->legacy_focus.item_ ##field) return pd->legacy_focus.item_ ##field;
   MAPPING()
   #undef MAP

   return NULL;
}

/**
 * @brief Sets the next Elm_Object_Item to focus to in a given direction for legacy focus handling.
 *
 * This allows defining a specific item within a container widget (like a genlist or gengrid)
 * that should receive focus when navigating in a particular direction from the container itself
 * or one of its other items.
 *
 * @param obj The Evas_Object (typically a container widget).
 * @param next_item The Elm_Object_Item to focus next.
 * @param dir The direction for this focus link.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_next_item_set(Evas_Object     *obj,
                               Elm_Object_Item *next_item EINA_UNUSED,
                               Elm_Focus_Direction dir EINA_UNUSED)
{
   API_ENTRY()
   MARK_WINDOW_LEGACY_USAGE()

   #define MAP(direction, field)  if ((Efl_Ui_Focus_Direction)dir == EFL_UI_FOCUS_DIRECTION_ ##direction) pd->legacy_focus.item_ ##field = next_item;
   MAPPING()
   #undef MAP
}

/**
 * @brief Gets the currently focused object within the context of a given object (usually a window).
 *
 * This function traverses the focus manager hierarchy, including redirects, to find the
 * actual object that currently holds focus. It includes special handling for legacy
 * container widgets like genlist, gengrid, and toolbar.
 *
 * @param obj The Evas_Object (typically a window or top-level widget) to query for the focused object.
 * @return The Evas_Object that is currently focused, or @c NULL if no object has focus
 *         or if @p obj is not part of a valid focus hierarchy.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI Evas_Object *
elm_object_focused_object_get(const Evas_Object *obj)
{
   API_ENTRY_VAL(NULL)
   Efl_Ui_Focus_Manager *man = elm_object_top_widget_get(obj);

   while(efl_ui_focus_manager_redirect_get(man))
     {
        man = efl_ui_focus_manager_redirect_get(man);

        // legacy compatible code, earlier those containers have not exposed theire items
        if (efl_isa(man, ELM_GENGRID_CLASS) ||
            efl_isa(man, ELM_TOOLBAR_CLASS) ||
            efl_isa(man, ELM_GENLIST_CLASS)) return man;
     }

   return efl_ui_focus_manager_focus_get(man);
}

/**
 * @brief Checks if an object currently has focus.
 *
 * For Elm widgets, this considers both whether the object itself is focused
 * and whether any of its children have focus, and also if its containing window is focused.
 * For non-Elm Evas objects, it falls back to evas_object_focus_get().
 *
 * @param obj The Evas_Object to check.
 * @return @c EINA_TRUE if the object has focus (or a child has focus and the window is active),
 *         @c EINA_FALSE otherwise.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI Eina_Bool
elm_object_focus_get(const Evas_Object *obj)
{
   API_ENTRY_VAL(EINA_FALSE)

   if (!elm_widget_is(obj))
     return evas_object_focus_get(obj);

   return _elm_widget_top_win_focused_get(obj) && (efl_ui_focus_object_child_focus_get(obj) | efl_ui_focus_object_focus_get(obj));
}

/**
 * @brief Sets or unsets focus on an object.
 *
 * This function handles focus setting for different types of objects:
 * - EFL_UI_WIN_CLASS: Special handling for inlined windows.
 * - Elm widgets: Uses the EFL focus utility to set focus or pops from the focus history stack to unset.
 * - Other Evas objects: Uses evas_object_focus_set().
 *
 * @param obj The Evas_Object to set focus on or remove focus from.
 * @param focus @c EINA_TRUE to set focus, @c EINA_FALSE to remove focus.
 * @ingroup Elm_Focus_Group_Legacy
 */
EAPI void
elm_object_focus_set(Evas_Object *obj,
                     Eina_Bool    focus)
{
   // ugly, but, special case for inlined windows
   if (efl_isa(obj, EFL_UI_WIN_CLASS))
     {
        Evas_Object *inlined = elm_win_inlined_image_object_get(obj);
        if (inlined)
          {
             evas_object_focus_set(inlined, focus);
             return;
          }
     }
   else if (elm_widget_is(obj))
     {
        if (focus)
          efl_ui_focus_util_focus(obj);
        else
          {
             if (efl_ui_focus_manager_focus_get(efl_ui_focus_object_focus_manager_get(obj)) == obj)
               efl_ui_focus_manager_pop_history_stack(efl_ui_focus_object_focus_manager_get(obj));
          }
     }
   else
     {
        evas_object_focus_set(obj, focus);
     }
}

//legacy helpers that are used in code
typedef struct {
  Eina_Bool focused;
  Eo *emittee; /**< The object that will emit the legacy "focused" / "unfocused" signals. */
} Legacy_Manager_Focus_State;

/**
 * @internal
 * @brief Callback for manager focus changes to emit legacy "focused"/"unfocused" signals.
 *
 * This function is called when the focus manager's overall focus state changes
 * (i.e., when efl_ui_focus_manager_focus_get() changes). It then emits
 * "focused" or "unfocused" smart callbacks on the `emittee` object.
 *
 * @param data A pointer to Legacy_Manager_Focus_State.
 * @param ev The event information.
 */
static void
_focus_manager_focused(void *data, const Efl_Event *ev)
{
   Legacy_Manager_Focus_State *state = data;
   Eina_Bool currently_focused = !!efl_ui_focus_manager_focus_get(ev->object);

   if (currently_focused == state->focused) return;

   if (currently_focused)
     evas_object_smart_callback_call(state->emittee, "focused", NULL);
   else
     evas_object_smart_callback_call(state->emittee, "unfocused", NULL);

   state->focused = currently_focused;
}

/**
 * @internal
 * @brief Callback for object deletion to free associated state data.
 *
 * Used to clean up Legacy_Manager_Focus_State or Legacy_Object_Focus_State
 * when the object they are associated with is deleted.
 *
 * @param data The state data to free.
 * @param ev Event information (unused).
 */
static void
_focus_manager_del(void *data, const Efl_Event *ev EINA_UNUSED)
{
   free(data);
}

/**
 * @internal
 * @brief Sets up legacy "focused" and "unfocused" signal emission for a widget acting as a focus manager.
 *
 * This function is used to provide backward compatibility for widgets that previously
 * emitted "focused" or "unfocused" signals based on the overall focus state of the
 * manager they represent (e.g., a window). It listens to the
 * EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED event on the @p manager and
 * triggers smart callbacks on the @p emittee.
 *
 * @param manager The Efl_Ui_Focus_Manager whose focus state changes will be monitored.
 * @param emittee The Efl_Ui_Focus_Manager (typically an Evas_Object/widget) that will emit the legacy signals.
 */
void
legacy_efl_ui_focus_manager_widget_legacy_signals(Efl_Ui_Focus_Manager *manager, Efl_Ui_Focus_Manager *emittee)
{
   Legacy_Manager_Focus_State *state = calloc(1, sizeof(Legacy_Manager_Focus_State));

   state->emittee = emittee;
   state->focused = EINA_FALSE;

   efl_event_callback_add(manager, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, _focus_manager_focused, state);
   efl_event_callback_add(manager, EFL_EVENT_DEL, _focus_manager_del, state);
}

typedef struct {
  Eina_Bool focused;
  Efl_Ui_Focus_Manager *registered_manager; /**< The manager this object is currently listening to. */
  Eo *emittee; /**< The object that will emit the legacy "focused" / "unfocused" signals. */
} Legacy_Object_Focus_State;

/**
 * @internal
 * @brief Callback for manager focus changes to emit legacy "focused"/"unfocused" signals based on child focus.
 *
 * This function is called when the focus manager's overall focus state changes.
 * It checks if the `emittee` object has a focused child
 * (efl_ui_focus_object_child_focus_get()) and emits "focused" or "unfocused"
 * smart callbacks accordingly. This is used for containers that should appear
 * "focused" when one of their children is focused.
 *
 * @param data A pointer to Legacy_Object_Focus_State.
 * @param ev The event information (unused).
 */
static void
_manager_focus_changed(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Legacy_Object_Focus_State *state = data;
   Eina_Bool currently_focused = efl_ui_focus_object_child_focus_get(state->emittee);

   if (currently_focused == state->focused) return;

   if (currently_focused)
     evas_object_smart_callback_call(state->emittee, "focused", NULL);
   else
     evas_object_smart_callback_call(state->emittee, "unfocused", NULL);
   state->focused = currently_focused;
}

/**
 * @internal
 * @brief Callback for when an object's associated focus manager changes.
 *
 * This function is responsible for updating event listeners. It removes the
 * listener from the old manager (if any) and adds it to the new manager.
 * This ensures that legacy focus signals are correctly emitted based on the
 * state of the current focus manager.
 *
 * @param data A pointer to Legacy_Object_Focus_State.
 * @param ev The event information (unused).
 */
static void
_manager_focus_object_changed(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Legacy_Object_Focus_State *state = data;
   if (state->registered_manager)
     efl_event_callback_del(state->registered_manager, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, _manager_focus_changed, state);
   state->registered_manager = efl_ui_focus_object_focus_manager_get(state->emittee);
   if (state->registered_manager)
     efl_event_callback_add(state->registered_manager, EFL_UI_FOCUS_MANAGER_EVENT_MANAGER_FOCUS_CHANGED, _manager_focus_changed, state);
}

/**
 * @internal
 * @brief Sets up legacy "focused" and "unfocused" signal emission based on child focus.
 *
 * This function is used for container-like widgets that should emit "focused"
 * when one of their children gains focus, and "unfocused" when no child has focus
 * (within that container's managed focus scope). It listens for changes in the
 * object's focus manager and the manager's overall focus state.
 *
 * @param object The Efl_Ui_Focus_Object (widget) that will emit the signals.
 */
void
legacy_child_focus_handle(Efl_Ui_Focus_Object *object)
{
   Legacy_Object_Focus_State *state = calloc(1, sizeof(Legacy_Object_Focus_State));
   state->emittee = object;

   efl_event_callback_add(object, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_MANAGER_CHANGED, _manager_focus_object_changed, state);
   efl_event_callback_add(object, EFL_EVENT_DEL, _focus_manager_del, state);
}

/**
 * @internal
 * @brief Callback for an object's own focus state changes (EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED).
 *
 * This function is called when the direct focus state of an object changes
 * (i.e., efl_ui_focus_object_focus_get() changes for `event->object`).
 * It emits "focused" or "unfocused" smart callbacks on the object itself.
 *
 * @param data User data (unused).
 * @param event The event information, where event->object is the object whose focus changed.
 */
static void
_focus_event_changed(void *data EINA_UNUSED, const Efl_Event *event)
{
   if (efl_ui_focus_object_focus_get(event->object))
     evas_object_smart_callback_call(event->object, "focused", NULL);
   else
     evas_object_smart_callback_call(event->object, "unfocused", NULL);
}

/**
 * @internal
 * @brief Sets up legacy "focused" and "unfocused" signal emission based on an object's direct focus.
 *
 * This function is for widgets that should emit "focused" or "unfocused" signals
 * based on their own direct focus state (efl_ui_focus_object_focus_get()).
 * It listens to the EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED event on the object.
 *
 * @param object The Efl_Ui_Focus_Object (widget) that will emit the signals.
 */
void
legacy_object_focus_handle(Efl_Ui_Focus_Object *object)
{
   efl_event_callback_add(object, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _focus_event_changed, NULL);
}
