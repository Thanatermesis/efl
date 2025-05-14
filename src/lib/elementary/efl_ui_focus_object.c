#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FOCUS_OBJECT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_FOCUS_OBJECT_MIXIN

/**
 * @brief Private data structure for Efl_Ui_Focus_Object.
 */
typedef struct {
  Eina_Bool old_focus; /**< Stores the previous focus state. */
  Eina_Bool ongoing_prepare_call; /**< Flag to prevent recursion in setup_order. */
  Eina_Bool child_focus; /**< Indicates if a child object has focus. */
  Eina_Bool focus_geom_changed; /**< Indicates if a listener for focus geometry changes is present. */
} Efl_Ui_Focus_Object_Data;

/**
 * @brief Sets the focus state of the object.
 *
 * This function updates the focus state and notifies the parent object
 * and listeners about the change.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @param focus EINA_TRUE to set focus, EINA_FALSE to unset.
 */
EOLIAN static void
_efl_ui_focus_object_focus_set(Eo *obj, Efl_Ui_Focus_Object_Data *pd, Eina_Bool focus)
{
   Efl_Ui_Focus_Object *parent;

   if (pd->old_focus == focus) return;

   pd->old_focus = focus;
   parent = efl_ui_focus_object_focus_parent_get(obj);
   if (parent)
     efl_ui_focus_object_child_focus_set(parent, focus);
   efl_event_callback_call(obj, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED , &focus);
}

/**
 * @brief Gets the focus state of the object.
 *
 * @param obj The Efl_Ui_Focus_Object (unused).
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @return EINA_TRUE if the object is focused, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_object_focus_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Object_Data *pd)
{
   return pd->old_focus;
}

/**
 * @brief Sets up the focus order for the object.
 *
 * This function calls the non-recursive version of setup_order,
 * ensuring it's not called again if already in progress.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 */
EOLIAN static void
_efl_ui_focus_object_setup_order(Eo *obj, Efl_Ui_Focus_Object_Data *pd)
{
  if (pd->ongoing_prepare_call) return;

  pd->ongoing_prepare_call = EINA_TRUE;

  efl_ui_focus_object_setup_order_non_recursive(obj);

  pd->ongoing_prepare_call = EINA_FALSE;
}

/**
 * @brief Sets the child focus state of the object.
 *
 * This function updates the child focus state and propagates it to the parent object.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @param child_focus EINA_TRUE if a child has focus, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_focus_object_child_focus_set(Eo *obj, Efl_Ui_Focus_Object_Data *pd, Eina_Bool child_focus)
{
   Efl_Ui_Focus_Object *parent;

   if (child_focus == pd->child_focus) return;

   pd->child_focus = child_focus;
   parent = efl_ui_focus_object_focus_parent_get(obj);
   if (parent)
     efl_ui_focus_object_child_focus_set(parent, pd->child_focus);
}

/**
 * @brief Gets the child focus state of the object.
 *
 * @param obj The Efl_Ui_Focus_Object (unused).
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @return EINA_TRUE if a child of this object has focus, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_object_child_focus_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Object_Data *pd)
{
   return pd->child_focus;
}

/**
 * @brief Adds an event callback with a specific priority.
 *
 * This function wraps the parent's efl_event_callback_priority_add.
 * It specifically checks if a callback for EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED
 * is being added and updates the internal `focus_geom_changed` flag.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @param desc The event description.
 * @param priority The callback priority.
 * @param func The callback function.
 * @param user_data User data to pass to the callback.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_object_efl_object_event_callback_priority_add(Eo *obj, Efl_Ui_Focus_Object_Data *pd,
                                        const Efl_Event_Description *desc,
                                        Efl_Callback_Priority priority,
                                        Efl_Event_Cb func,
                                        const void *user_data)
{
  if (desc == EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED)
    {
       pd->focus_geom_changed = EINA_TRUE;
    }

  return efl_event_callback_priority_add(efl_super(obj, MY_CLASS), desc, priority, func, user_data);
}

/**
 * @brief Adds an array of event callbacks with a specific priority.
 *
 * This function wraps the parent's efl_event_callback_array_priority_add.
 * It iterates through the array and checks if any callback for
 * EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED is being added,
 * updating the internal `focus_geom_changed` flag accordingly.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @param array An array of Efl_Callback_Array_Item.
 *              Example:
 *              @code
 *              static const Efl_Callback_Array_Item event_callbacks[] = {
 *                   { EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED, _my_geom_changed_cb },
 *                   { EFL_EVENT_DEL, _my_del_cb },
 *                   { NULL, NULL }
 *              };
 *              @endcode
 * @param priority The callback priority.
 * @param user_data User data to pass to the callbacks.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_object_efl_object_event_callback_array_priority_add(Eo *obj, Efl_Ui_Focus_Object_Data *pd,
                                              const Efl_Callback_Array_Item *array,
                                              Efl_Callback_Priority priority,
                                              const void *user_data)
{
   for (int i = 0; array[i].desc; ++i)
     {
        if (array[i].desc == EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED)
          {
             pd->focus_geom_changed = EINA_TRUE;
          }
     }
   return efl_event_callback_array_priority_add(efl_super(obj, MY_CLASS), array, priority, user_data);
}

/**
 * @brief Calls event callbacks for a given event.
 *
 * This function wraps the parent's efl_event_callback_call.
 * It introduces an optimization: if the event is EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED
 * and no listener has been registered for it (pd->focus_geom_changed is EINA_FALSE),
 * the callback chain is not invoked, returning EINA_TRUE immediately.
 *
 * @param obj The Efl_Ui_Focus_Object.
 * @param pd Private data for the Efl_Ui_Focus_Object.
 * @param desc The event description.
 * @param event_info The event-specific data.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_object_efl_object_event_callback_call(Eo *obj, Efl_Ui_Focus_Object_Data *pd,
            const Efl_Event_Description *desc,
            void *event_info)
{
   // Optimization: if no one is listening to focus geometry changes, don't bother calling.
   if (desc == EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_GEOMETRY_CHANGED && !pd->focus_geom_changed)
     return EINA_TRUE;
   return efl_event_callback_call(efl_super(obj, MY_CLASS), desc, event_info);
}

#define EFL_UI_FOCUS_OBJECT_EXTRA_OPS \
   EFL_OBJECT_OP_FUNC(efl_event_callback_priority_add, _efl_ui_focus_object_efl_object_event_callback_priority_add), \
   EFL_OBJECT_OP_FUNC(efl_event_callback_array_priority_add, _efl_ui_focus_object_efl_object_event_callback_array_priority_add), \
   EFL_OBJECT_OP_FUNC(efl_event_callback_call, _efl_ui_focus_object_efl_object_event_callback_call) \

#include "efl_ui_focus_object.eo.c"
