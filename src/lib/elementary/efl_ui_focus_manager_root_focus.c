#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_FOCUS_COMPOSITION_ADAPTER_PROTECTED
#define EFL_UI_FOCUS_OBJECT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#include "efl_ui_focus_composition_adapter.eo.h"

#define MY_CLASS EFL_UI_FOCUS_MANAGER_ROOT_FOCUS_CLASS

/**
 * @brief Private data structure for Efl_Ui_Focus_Manager_Root_Focus.
 */
typedef struct {
   Efl_Ui_Focus_Object *replacement_object; /**< The object that replaces the root focus when it's not focusable. */

   Evas_Object *rect; /**< An internal adapter object used to represent the root when it's not focusable. */
   Eina_Bool rect_registered; /**< Flag indicating if the internal rect is currently registered in the focus manager. */

   Eina_List *iterator_list; /**< A list containing only the 'rect' when it's registered, used for border elements. */
   Eina_Future *focus_transfer; /**< A future used to delay the unregistration of the 'rect'. */
} Efl_Ui_Focus_Manager_Root_Focus_Data;

/**
 * @brief Unregisters the internal rectangle from the focus manager.
 *
 * This function is typically called when the root focus becomes focusable again.
 *
 * @param obj The Efl_Ui_Focus_Manager_Root_Focus object.
 * @param data User data (unused).
 * @param v The Eina_Value (unused).
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_unregister_rect(Eo *obj, void *data EINA_UNUSED, const Eina_Value v EINA_UNUSED)
{
   Efl_Ui_Focus_Manager_Root_Focus_Data *pd = efl_data_scope_get(obj, MY_CLASS);

   efl_ui_focus_manager_calc_unregister(obj, pd->rect);
   pd->rect_registered = EINA_FALSE;
   efl_ui_focus_composition_adapter_focus_manager_parent_set(pd->rect, NULL);
   efl_ui_focus_composition_adapter_focus_manager_object_set(pd->rect, NULL);
   pd->focus_transfer = NULL;
   return EINA_VALUE_EMPTY;
}

/**
 * @brief Traps or redirects focus from the internal rect to the replacement object.
 *
 * If the given object is the internal 'rect', this function returns the
 * 'replacement_object'. Otherwise, it returns the original object.
 * This is used to ensure that operations on the 'rect' (when it's acting
 * as a placeholder for an unfocusable root) are effectively applied to
 * the intended replacement.
 *
 * @param pd The private data of the Efl_Ui_Focus_Manager_Root_Focus object.
 * @param obj The focus object to potentially trap.
 * @return The original object or the replacement object if 'obj' is the internal rect.
 */
static Efl_Ui_Focus_Object*
_trap(Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *obj)
{
   if (pd->rect == obj) return pd->replacement_object;
   return obj;
}

/**
 * @brief Evaluates and updates the state of the root focus manager.
 *
 * This function determines if the internal 'rect' (placeholder for an
 * unfocusable root) should be registered or unregistered based on whether
 * there are other focusable children.
 * If no other focusable children exist, the 'rect' is registered to
 * ensure the manager itself can be part of focus chains.
 * If focusable children appear, the 'rect' is unregistered.
 *
 * @param obj The Efl_Ui_Focus_Manager_Root_Focus object.
 * @param pd The private data of the Efl_Ui_Focus_Manager_Root_Focus object.
 */
static void
_state_eval(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   Efl_Ui_Focus_Object *sub;

   sub = efl_ui_focus_manager_request_subchild(obj, efl_ui_focus_manager_root_get(obj));

   if (sub == pd->rect)
     {
        sub = efl_ui_focus_manager_request_move(obj, EFL_UI_FOCUS_DIRECTION_NEXT, pd->rect, EINA_FALSE);
        if (sub == pd->rect)
          sub = NULL;
     }

   EINA_SAFETY_ON_TRUE_RETURN(sub == pd->rect);

   if (sub && pd->rect_registered)
     {
        pd->focus_transfer = efl_loop_job(efl_main_loop_get());
        efl_future_then(obj, pd->focus_transfer, _unregister_rect);
     }
   else if (!sub && !pd->rect_registered)
     {
        Efl_Ui_Focus_Object *root;

        if (pd->focus_transfer)
          eina_future_cancel(pd->focus_transfer);
        pd->focus_transfer = NULL;

        root = efl_ui_focus_manager_root_get(obj);
        efl_ui_focus_manager_calc_register(obj, pd->rect, root, NULL);
        efl_ui_focus_composition_adapter_focus_manager_parent_set(pd->rect, root);
        efl_ui_focus_composition_adapter_focus_manager_object_set(pd->rect, obj);
        pd->rect_registered = EINA_TRUE;
     }
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.calc_register.
 *
 * Registers a child in the focus manager. If the child being registered is not
 * the internal 'rect' and the 'rect' is currently registered, it triggers
 * a state evaluation to potentially unregister the 'rect'.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_register(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child, Efl_Ui_Focus_Object *parent, Efl_Ui_Focus_Manager *redirect)
{
   if (efl_ui_focus_manager_calc_register(efl_super(obj, MY_CLASS), child, parent, redirect))
     {
        if (child != pd->rect && pd->rect_registered)
          _state_eval(obj, pd);

        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.calc_register_logical.
 *
 * Registers a logical child in the focus manager. If a redirect occurs and
 * the internal 'rect' is registered, it triggers a state evaluation.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_register_logical(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child, Efl_Ui_Focus_Object *parent, Efl_Ui_Focus_Manager *redirect)
{
   if (!parent) return EINA_FALSE;
   if (efl_ui_focus_manager_calc_register_logical(efl_super(obj, MY_CLASS), child, parent, redirect))
     {
        if (redirect && pd->rect_registered)
          _state_eval(obj, pd);

        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.calc_unregister.
 *
 * Unregisters a child from the focus manager. If the child being unregistered
 * is not the internal 'rect', it triggers a state evaluation to potentially
 * register the 'rect' if no other focusable children remain.
 */
EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_calc_unregister(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child)
{
   efl_ui_focus_manager_calc_unregister(efl_super(obj, MY_CLASS), child);

   if (child != pd->rect)
     _state_eval(obj, pd);
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.manager_focus_set.
 *
 * Sets the current focused object. Uses _trap to potentially redirect
 * focus if the target is the internal 'rect'.
 */
EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_manager_focus_set(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *focus)
{
   EINA_SAFETY_ON_NULL_RETURN(focus);
   efl_ui_focus_manager_focus_set(efl_super(obj, MY_CLASS), _trap(pd, focus));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.manager_focus_get.
 *
 * Gets the current focused object. Uses _trap to potentially redirect
 * focus if the result is the internal 'rect'.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_manager_focus_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   return _trap(pd, efl_ui_focus_manager_focus_get(efl_super(obj, MY_CLASS)));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.fetch.
 *
 * Fetches focus relations for a child. Uses _trap to handle the case
 * where 'child' might be the internal 'rect'.
 */
EOLIAN static Efl_Ui_Focus_Relations *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_fetch(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Object *child)
{
   return efl_ui_focus_manager_fetch(efl_super(obj, MY_CLASS), _trap(pd, child));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.logical_end.
 *
 * Gets the logical end details. Uses _trap to adjust the returned element
 * if it's the internal 'rect'.
 */
EOLIAN static Efl_Ui_Focus_Manager_Logical_End_Detail
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_logical_end(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   Efl_Ui_Focus_Manager_Logical_End_Detail res;

   res = efl_ui_focus_manager_logical_end(efl_super(obj, MY_CLASS));

   res.element = _trap(pd, res.element);
   return res;
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.border_elements_get.
 *
 * Returns an iterator for border elements. If the internal 'rect' is registered,
 * it returns an iterator containing only the 'rect'. Otherwise, it calls the
 * superclass implementation.
 * The iterator will contain Efl_Ui_Focus_Object elements.
 */
EOLIAN static Eina_Iterator *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_border_elements_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   if (pd->rect_registered)
     return eina_list_iterator_new(pd->iterator_list);

   return efl_ui_focus_manager_border_elements_get(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.viewport_elements_get.
 *
 * Returns an iterator for viewport elements. If the internal 'rect' is registered,
 * it returns an iterator containing only the 'rect'. Otherwise, it calls the
 * superclass's border_elements_get (as per current implementation, might be specific).
 * The iterator will contain Efl_Ui_Focus_Object elements.
 */
EOLIAN static Eina_Iterator *
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_viewport_elements_get(const Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Eina_Rect viewport EINA_UNUSED)
{
   if (pd->rect_registered)
     return eina_list_iterator_new(pd->iterator_list);

   return efl_ui_focus_manager_border_elements_get(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.request_move.
 *
 * Requests a focus move. Uses _trap to adjust the result if it's the
 * internal 'rect'.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_request_move(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Direction direction, Efl_Ui_Focus_Object *child, Eina_Bool logical)
{
   return _trap(pd, efl_ui_focus_manager_request_move(efl_super(obj, MY_CLASS), direction, child, logical));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.move.
 *
 * Moves focus in a given direction. Uses _trap to adjust the result if it's
 * the internal 'rect'.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_manager_root_focus_efl_ui_focus_manager_move(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Ui_Focus_Direction direction)
{
   return _trap(pd, efl_ui_focus_manager_move(efl_super(obj, MY_CLASS), direction));
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.Root_Focus.canvas_object_get.
 *
 * Gets the canvas object that this root focus manager represents or replaces.
 * This is the object that will be focused if the root itself is not focusable
 * and this manager takes over.
 *
 * @return The replacement canvas object.
 */
EOLIAN static Efl_Canvas_Object*
_efl_ui_focus_manager_root_focus_canvas_object_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   return pd->replacement_object;
}

/**
 * @brief Implements Efl.Ui.Focus.Manager.Root_Focus.canvas_object_set.
 *
 * Sets the canvas object that this root focus manager should represent or replace.
 * If `canvas_object` is NULL, it defaults to the manager's root object.
 * This object is used when the actual root of the manager is not focusable,
 * allowing this manager to provide a focusable stand-in.
 *
 * @param obj The Efl_Ui_Focus_Manager_Root_Focus object.
 * @param pd The private data.
 * @param canvas_object The canvas object to be used as a replacement.
 *                      Example: a specific widget within a complex, non-focusable root.
 */
EOLIAN static void
_efl_ui_focus_manager_root_focus_canvas_object_set(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd, Efl_Canvas_Object *canvas_object)
{
   //if canvas object is NULL trigger it as root
   if (!canvas_object)
     canvas_object = efl_ui_focus_manager_root_get(obj);

   if (canvas_object == pd->replacement_object) return;

   if (pd->replacement_object)
     {
        pd->iterator_list = eina_list_remove(pd->iterator_list, pd->rect);
        pd->replacement_object = NULL;
     }

   pd->replacement_object = canvas_object;
   if (pd->replacement_object)
     {
        efl_ui_focus_composition_adapter_canvas_object_set(pd->rect, pd->replacement_object);
        pd->iterator_list = eina_list_append(pd->iterator_list, pd->rect);
     }
}

/**
 * @brief Callback for the EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED event on the internal 'rect'.
 *
 * When the focus status of the internal 'rect' (composition adapter) changes,
 * this function propagates that focus state to the actual root object of this manager.
 * This ensures that if the 'rect' gains or loses focus (because it's the only
 * focusable thing, representing an otherwise unfocusable root), the root object's
 * focus state is updated accordingly.
 *
 * @param data The Efl_Ui_Focus_Manager_Root_Focus object (passed as user data).
 * @param ev The event information.
 */
static void
_focus_changed(void *data, const Efl_Event *ev)
{
   Eo *root;

   root = efl_ui_focus_manager_root_get(data);

   efl_ui_focus_object_focus_set(root, efl_ui_focus_object_focus_get(ev->object));
}

EFL_CALLBACKS_ARRAY_DEFINE(composition_cb,
   { EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_CHANGED, _focus_changed },
)

/**
 * @brief Implements Efl.Object.constructor.
 *
 * Initializes the Efl_Ui_Focus_Manager_Root_Focus object.
 * Creates the internal 'rect' (EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS)
 * and sets up event listeners on it.
 */
EOLIAN static Efl_Object*
_efl_ui_focus_manager_root_focus_efl_object_constructor(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   pd->rect = efl_add_ref(EFL_UI_FOCUS_COMPOSITION_ADAPTER_CLASS, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->rect, NULL);
   efl_event_callback_array_add(pd->rect, composition_cb(), obj);

   return efl_constructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Object.destructor.
 *
 * Cleans up resources used by the Efl_Ui_Focus_Manager_Root_Focus object.
 * Specifically, it unreferences the internal 'rect'.
 */
EOLIAN static void
_efl_ui_focus_manager_root_focus_efl_object_destructor(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   efl_unref(pd->rect);
   pd->rect = NULL;

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Object.finalize.
 *
 * Finalizes the object. Ensures the canvas object is set (defaults to root
 * if not already set) and performs an initial state evaluation.
 */
EOLIAN static Efl_Object*
_efl_ui_focus_manager_root_focus_efl_object_finalize(Eo *obj, Efl_Ui_Focus_Manager_Root_Focus_Data *pd)
{
   //set it to NULL so the root manager is passed to the manager
   if (!pd->replacement_object)
     efl_ui_focus_manager_root_focus_canvas_object_set(obj, NULL);

   _state_eval(obj, pd);

   return efl_finalize(efl_super(obj, MY_CLASS));
}

#include "efl_ui_focus_manager_root_focus.eo.c"
