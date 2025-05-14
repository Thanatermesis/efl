#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_item_private.h"

#define MY_CLASS      EFL_UI_ITEM_CLASS
#define MY_CLASS_PFX  efl_ui_item

#define MY_CLASS_NAME "Efl.Ui.Item"

static Eina_Bool _key_action_select(Evas_Object *obj, const char *params EINA_UNUSED);

/**
 * @brief Defines the keyboard actions available for an Efl.Ui.Item.
 *
 * This array maps action names (strings) to their corresponding callback functions.
 * Currently, it includes:
 * - "select": Triggers the selection of the item.
 */
static const Elm_Action key_actions[] = {
   {"select", _key_action_select},
   {NULL, NULL}
};

/**
 * @brief Action to perform when the "select" key action is triggered.
 *
 * This function sets the item as selected.
 *
 * @param obj The Evas_Object associated with the item.
 * @param params Unused.
 * @return EINA_FALSE always, as this action does not propagate further.
 */
static Eina_Bool
_key_action_select(Evas_Object *obj, const char *params EINA_UNUSED)
{
   efl_ui_selectable_selected_set(obj, EINA_TRUE);
   return EINA_FALSE;
}

/**
 * @brief Fetches the selection mode from the item's container.
 *
 * This function determines if the container supports single selection,
 * multi-selection, or no selection.
 *
 * @param obj The Evas_Object (item) whose container's selection mode is to be fetched.
 * @return The Efl_Ui_Select_Mode of the container. Returns EFL_UI_SELECT_MODE_NONE
 *         if the container does not support selection or if an error occurs.
 */
static Efl_Ui_Select_Mode
_fetch_state(Eo *obj)
{
   if (efl_isa(obj, EFL_UI_MULTI_SELECTABLE_INTERFACE))
     return efl_ui_multi_selectable_select_mode_get(obj);
   if (efl_isa(obj, EFL_UI_SINGLE_SELECTABLE_INTERFACE))
     return EFL_UI_SELECT_MODE_SINGLE;
   ERR("Uncaught state %s", efl_debug_name_get(obj));
   return EFL_UI_SELECT_MODE_NONE;
}

/**
 * @brief Handles the selection logic for an item.
 *
 * This function updates the item's state to selected, emits a "selected" signal,
 * and calls the EFL_UI_EVENT_SELECTED_CHANGED event callback.
 * It considers the selection mode of its container.
 *
 * @param obj The Efl.Ui.Item object to be selected.
 * @param pd The private data of the Efl.Ui.Item.
 */
static void
_item_select(Eo *obj, Efl_Ui_Item_Data *pd)
{
   Efl_Ui_Select_Mode m;

   if (pd->container)
     {
        m = _fetch_state(pd->container);
        if (m == EFL_UI_SELECT_MODE_NONE)
          return;
     }
   else
     {
        if (pd->selected)
          return;
     }

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Eina_Bool tmp = pd->selected = EINA_TRUE;
   edje_object_signal_emit(wd->resize_obj, "efl,state,selected", "efl");
   efl_event_callback_call(obj, EFL_UI_EVENT_SELECTED_CHANGED, &tmp);
}

/**
 * @brief Handles the unselection logic for an item.
 *
 * This function updates the item's state to unselected, emits an "unselected" signal,
 * and calls the EFL_UI_EVENT_SELECTED_CHANGED event callback.
 *
 * @param obj The Efl.Ui.Item object to be unselected.
 * @param pd The private data of the Efl.Ui.Item.
 */
static void
_item_unselect(Eo *obj, Efl_Ui_Item_Data *pd)
{
   if (!pd->selected) return;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Eina_Bool tmp = pd->selected =EINA_FALSE;
   edje_object_signal_emit(wd->resize_obj, "efl,state,unselected", "efl");
   efl_event_callback_call(obj, EFL_UI_EVENT_SELECTED_CHANGED, &tmp);
}

/* Mouse Controls */
/**
 * @brief Callback for when the item is pressed (mouse button down).
 *
 * Emits the "efl,state,pressed" signal if the item is not disabled.
 *
 * @param data The Efl_Ui_Item object.
 * @param ev The Efl_Event data (unused).
 */
static void
_item_pressed(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Item *obj = data;
   if (efl_ui_widget_disabled_get(obj)) return;

   efl_layout_signal_emit(obj, "efl,state,pressed", "efl");
}

/**
 * @brief Callback for when the item is unpressed (mouse button up).
 *
 * Handles item selection/deselection logic based on its current state
 * and the container's selection mode. Emits the "efl,state,unpressed" signal.
 *
 * @param data The Efl_Ui_Item object.
 * @param ev The Efl_Event data (unused).
 */
static void
_item_unpressed(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Item *obj = data;
   Efl_Ui_Select_Mode m;
   EFL_UI_ITEM_DATA_GET_OR_RETURN(obj, pd);

   if (efl_ui_widget_disabled_get(obj)) return;
   if (!efl_ui_item_container_get(obj)) return;

   efl_layout_signal_emit(obj, "efl,state,unpressed", "efl");
   m = _fetch_state(pd->container);

   if (pd->selected)
     {
        if (efl_ui_selectable_allow_manual_deselection_get(pd->container))
          efl_ui_selectable_selected_set(obj, EINA_FALSE);
     }
   else if (m != EFL_UI_SELECT_MODE_NONE)
     efl_ui_selectable_selected_set(obj, EINA_TRUE);
}

/**
 * @brief Defines the event callbacks for mouse input on the item itself.
 *
 * This array maps Efl_Input_Event types to their corresponding handler functions:
 * - EFL_INPUT_EVENT_PRESSED: Calls _item_pressed when the item receives a press event.
 * - EFL_INPUT_EVENT_UNPRESSED: Calls _item_unpressed when the item receives an unpress event.
 */
EFL_CALLBACKS_ARRAY_DEFINE(self_listening,
  {EFL_INPUT_EVENT_PRESSED, _item_pressed},
  {EFL_INPUT_EVENT_UNPRESSED, _item_unpressed},
)

/* Mouse Controls ends */

/**
 * @internal
 * @brief Efl.Object constructor for Efl.Ui.Item.
 *
 * Initializes the item, sets default finger size multiplier, and adds
 * event listeners for press/unpress events.
 *
 * @param obj The Efl.Ui.Item object being constructed.
 * @param pd Private data for the Efl.Ui.Item (unused in this function).
 * @return The constructed Efl.Ui.Item object.
 */
EOLIAN static Eo *
_efl_ui_item_efl_object_constructor(Eo *obj, Efl_Ui_Item_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);

   efl_event_callback_array_add(obj, self_listening(), obj);

   return obj;
}

/**
 * @internal
 * @brief Efl.Object finalize method for Efl.Ui.Item.
 *
 * Completes the initialization of the item. This includes making the item
 * focusable and binding clickable actions to the theme or object based on
 * the theme version.
 *
 * @param obj The Efl.Ui.Item object being finalized.
 * @param pd Private data for the Efl.Ui.Item (unused in this function).
 * @return The finalized Efl.Ui.Item object.
 */
EOLIAN static Efl_Object *
_efl_ui_item_efl_object_finalize(Eo *obj, Efl_Ui_Item_Data *pd EINA_UNUSED)
{
   Eo *eo;
   eo = efl_finalize(efl_super(obj, MY_CLASS));
   ELM_WIDGET_DATA_GET_OR_RETURN(eo, wd, eo);

   /* Support Item Focus Feature */
   elm_widget_can_focus_set(obj, EINA_TRUE);

   if (efl_ui_layout_theme_version_get(obj) == 123)
     efl_ui_action_connector_bind_clickable_to_object(wd->resize_obj, obj);
   else
     efl_ui_action_connector_bind_clickable_to_theme(wd->resize_obj, obj);
   return eo;
}

/**
 * @internal
 * @brief Efl.Object destructor for Efl.Ui.Item.
 *
 * Performs cleanup when the Efl.Ui.Item object is being destroyed.
 *
 * @param obj The Efl.Ui.Item object being destructed.
 * @param pd Private data for the Efl.Ui.Item (unused in this function).
 */
EOLIAN static void
_efl_ui_item_efl_object_destructor(Eo *obj, Efl_Ui_Item_Data *pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Gets the index of the item within its container.
 *
 * This function is relevant if the item's container implements the
 * EFL_PACK_LINEAR_INTERFACE (e.g., a list or box).
 *
 * @param obj The Efl.Ui.Item object.
 * @param pd Private data of the Efl.Ui.Item.
 * @return The index of the item in its container, or -1 if the container
 *         is NULL or does not support linear packing.
 */
EOLIAN static int
_efl_ui_item_index_get(const Eo *obj, Efl_Ui_Item_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->container, -1);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(pd->container, EFL_PACK_LINEAR_INTERFACE), -1);
   return efl_pack_index_get(pd->container, obj);
}

/**
 * @internal
 * @brief Sets the selected state of the item.
 *
 * Implements the Efl.Ui.Selectable interface. This function calls
 * internal _item_select or _item_unselect based on the `select` parameter.
 * Does nothing if the item is disabled.
 *
 * @param obj The Efl.Ui.Item object.
 * @param pd Private data of the Efl.Ui.Item.
 * @param select EINA_TRUE to select the item, EINA_FALSE to unselect.
 */
EOLIAN static void
_efl_ui_item_efl_ui_selectable_selected_set(Eo *obj, Efl_Ui_Item_Data *pd, Eina_Bool select)
{
   Eina_Bool selected = !!select;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   if (wd->disabled) return;

   if (selected) _item_select(obj, pd);
   else _item_unselect(obj, pd);
}

/**
 * @internal
 * @brief Gets the selected state of the item.
 *
 * Implements the Efl.Ui.Selectable interface.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @return EINA_TRUE if the item is selected, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_item_efl_ui_selectable_selected_get(const Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd)
{
   return pd->selected;
}

/**
 * @internal
 * @brief Sets the container widget for this item.
 *
 * This is typically called when the item is packed into a container.
 * If the container is set to NULL, the item's parent is also cleared.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @param container The Efl_Ui_Widget that will contain this item.
 */
EOLIAN static void
_efl_ui_item_container_set(Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd, Efl_Ui_Widget *container)
{
   pd->container = container;
   if (!pd->container)
     {
        pd->parent = NULL;
     }
}

/**
 * @internal
 * @brief Gets the container widget of this item.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @return The Efl_Ui_Widget that contains this item, or NULL if it has no container.
 */
EOLIAN static Efl_Ui_Widget*
_efl_ui_item_container_get(const Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd)
{
   return pd->container;
}

/**
 * @internal
 * @brief Sets the parent item for this item, used for hierarchical structures.
 *
 * This function establishes a parent-child relationship between items.
 * It includes safety checks to prevent setting a parent if one is already set,
 * if the object is being invalidated, or if it's already in a container.
 *
 * @param obj The Efl.Ui.Item object.
 * @param pd Private data of the Efl.Ui.Item.
 * @param parent The Efl_Ui_Item to be set as the parent.
 */
EOLIAN static void
_efl_ui_item_item_parent_set(Eo *obj, Efl_Ui_Item_Data *pd, Efl_Ui_Item *parent)
{
   if (pd->parent)
     {
        ERR("Parent is already set on object %p", obj);
        return;
     }
   if (efl_invalidated_get(obj) || efl_invalidating_get(obj))
     {
        ERR("Parent cannot be set during invalidate");
        return;
     }
   if (pd->container)
     {
        ERR("Parent must be set before adding the object to the container");
        return;
     }
   pd->parent = parent;
}

/**
 * @internal
 * @brief Gets the parent item of this item.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @return The parent Efl_Ui_Item, or NULL if it has no parent.
 */
EOLIAN static Efl_Ui_Item*
_efl_ui_item_item_parent_get(const Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd)
{
   return pd->parent;
}

/**
 * @internal
 * @brief Sets the calculation lock state for the item.
 *
 * When an item is "calc_locked", its recalculation logic (e.g., for layout or text)
 * might be temporarily suspended. This is often used for performance optimization,
 * for example, when items are cached or part of a factory being released.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @param locked EINA_TRUE to lock calculations, EINA_FALSE to unlock.
 */
EOLIAN static void
_efl_ui_item_calc_locked_set(Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd, Eina_Bool locked)
{
   pd->locked = !!locked;
}

/**
 * @internal
 * @brief Gets the calculation lock state of the item.
 *
 * @param obj The Efl.Ui.Item object (unused).
 * @param pd Private data of the Efl.Ui.Item.
 * @return EINA_TRUE if calculations are locked, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_item_calc_locked_get(const Eo *obj EINA_UNUSED, Efl_Ui_Item_Data *pd)
{
   return pd->locked;
}

/**
 * @internal
 * @brief Sets whether the item's canvas group needs recalculation.
 *
 * This function overrides the default behavior to prevent recalculation
 * if the item is `calc_locked`. This is crucial for items in a cache or
 * during factory release stages to avoid updates from asynchronous operations
 * that complete after the item is considered inactive.
 *
 * @param obj The Efl.Ui.Item object.
 * @param pd Private data of the Efl.Ui.Item.
 * @param value EINA_TRUE if recalculation is needed, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_item_efl_canvas_group_group_need_recalculate_set(Eo *obj, Efl_Ui_Item_Data *pd EINA_UNUSED, Eina_Bool value)
{
   // Prevent recalc when the item are stored in the cache
   // As due to async behavior, we can still have text updated from future that just finished after
   // we have left the releasing stage of factories. This is the simplest way to prevent those later
   // update.
   if (pd->locked) return;
   efl_canvas_group_need_recalculate_set(efl_super(obj, EFL_UI_ITEM_CLASS), value);
}

ELM_WIDGET_KEY_DOWN_DEFAULT_IMPLEMENT(efl_ui_item, Efl_Ui_Item_Data)

#include "efl_ui_item.eo.c"
#include "efl_ui_selectable.eo.c"
#include "efl_ui_multi_selectable.eo.c"
#include "efl_ui_multi_selectable_object_range.eo.c"
#include "efl_ui_single_selectable.eo.c"
#include "efl_ui_item_clickable.eo.c"

