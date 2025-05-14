#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @internal
 * @brief Implementation of the Efl.Ui.Radio_Group interface.
 *
 * This class manages a group of radio buttons, ensuring that only one
 * radio button within the group can be selected at any given time.
 * It also handles fallback selection and value-based selection.
 */
#define MY_CLASS EFL_UI_RADIO_GROUP_IMPL_CLASS

/**
 * @internal
 * @brief Global hash map to track which radio button belongs to which group.
 * This is used to prevent a radio button from being registered to multiple groups.
 * The key is the Efl_Ui_Radio object pointer, and the value is the Efl_Ui_Radio_Group object pointer.
 */
static Eina_Hash *radio_group_map;

/**
 * @internal
 * @brief Private data structure for Efl_Ui_Radio_Group_Impl.
 */
typedef struct {
   Efl_Ui_Radio *selected; /**< The currently selected radio button in the group. NULL if none is selected. */
   Efl_Ui_Radio *fallback_object; /**< The radio button to select if the currently selected one is deselected and no other is chosen. */
   Eina_List *registered_set; /**< A list of all Efl_Ui_Radio objects registered with this group. */
   Eina_Bool in_value_change; /**< A flag to prevent re-entrant calls during value changes, especially when deselecting the old radio due to a new one being selected. */
} Efl_Ui_Radio_Group_Impl_Data;

/**
 * @internal
 * @brief Sets whether manual deselection is allowed.
 * @warning This is currently not supported for radio groups.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_ui_single_selectable_allow_manual_deselection_set(Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd EINA_UNUSED, Eina_Bool allow_manual_deselection EINA_UNUSED)
{
   if (allow_manual_deselection == EINA_FALSE)
     ERR("This is right now not supported.");
}

/**
 * @internal
 * @brief Gets whether manual deselection is allowed.
 * @return EINA_FALSE as manual deselection is not supported.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_group_impl_efl_ui_single_selectable_allow_manual_deselection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Sets the fallback selectable object for the group.
 * If the currently selected radio is deselected and no new radio is selected,
 * the fallback object will be automatically selected.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_ui_single_selectable_fallback_selection_set(Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd, Efl_Ui_Selectable *fallback)
{
   pd->fallback_object = fallback;

   if (!pd->selected)
     efl_ui_selectable_selected_set(pd->fallback_object, EINA_TRUE);
}

/**
 * @internal
 * @brief Gets the fallback selectable object for the group.
 * @return The fallback Efl_Ui_Selectable object, or NULL if not set.
 */
EOLIAN static Efl_Ui_Selectable*
_efl_ui_radio_group_impl_efl_ui_single_selectable_fallback_selection_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd)
{
   return pd->fallback_object;
}

/**
 * @internal
 * @brief Gets the last selected radio button in the group.
 * @return The last selected Efl_Ui_Radio object, or NULL if none was selected.
 */
EOLIAN static Efl_Ui_Radio*
_efl_ui_radio_group_impl_efl_ui_single_selectable_last_selected_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd)
{
   return pd->selected;
}

/**
 * @internal
 * @brief Sets the selected radio button in the group by its state value.
 * Iterates through registered radio buttons and selects the one matching the given value.
 * If no radio button has the specified value, an error is logged.
 * @param selected_value The integer value associated with the radio button to be selected.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_ui_radio_group_selected_value_set(Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd, int selected_value)
{
   Efl_Ui_Radio *reged;
   Eina_List *n;

   EINA_LIST_FOREACH(pd->registered_set, n, reged)
     {
        if (efl_ui_radio_state_value_get(reged) == selected_value)
          {
             efl_ui_selectable_selected_set(reged, EINA_TRUE);
             return;
          }
     }
   ERR("Value %d not associated with any radio button", selected_value);
}

/**
 * @internal
 * @brief Gets the state value of the currently selected radio button.
 * @return The integer value of the selected radio button, or -1 if no radio button is selected.
 */
EOLIAN static int
_efl_ui_radio_group_impl_efl_ui_radio_group_selected_value_get(const Eo *obj EINA_UNUSED, Efl_Ui_Radio_Group_Impl_Data *pd)
{
   return pd->selected ? efl_ui_radio_state_value_get(pd->selected) : -1;
}

/**
 * @internal
 * @brief Callback invoked when a radio button's selected state changes.
 * This function manages the core logic of a radio group:
 * - If a radio button is selected, it deselects any previously selected radio button.
 * - Updates the internal `selected` pointer.
 * - Handles fallback selection if a radio button is deselected and no other is selected.
 * - Emits `EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED` and `EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED` events.
 * @param data The radio group object (Eo *).
 * @param ev The event information, where ev->object is the radio button that changed state.
 */
static void
_selected_cb(void *data, const Efl_Event *ev)
{
   Efl_Ui_Radio_Group_Impl_Data *pd = efl_data_scope_safe_get(data, EFL_UI_RADIO_GROUP_IMPL_CLASS);
   EINA_SAFETY_ON_NULL_RETURN(pd);

   if (efl_ui_selectable_selected_get(ev->object))
     {
        if (pd->selected)
          {
             pd->in_value_change = EINA_TRUE;
             efl_ui_selectable_selected_set(pd->selected, EINA_FALSE);
          }
        pd->in_value_change = EINA_FALSE;
        EINA_SAFETY_ON_FALSE_RETURN(!pd->selected);
        pd->selected = ev->object;
     }
   else
     {
        //if something was unselected, we need to make sure that we are unsetting the internal pointer to NULL
        if (pd->selected == ev->object)
          {
             pd->selected = NULL;
          }
        //checkout if we want to do fallback handling
        if (!pd->in_value_change)
          {
             if (!pd->selected && pd->fallback_object)
               efl_ui_selectable_selected_set(pd->fallback_object, EINA_TRUE);
          }
     }

   if (!pd->in_value_change)
     {
        int value;
        if (pd->selected)
          value = efl_ui_radio_state_value_get(pd->selected);
        else
          value = -1;
        efl_event_callback_call(data, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, &value);
        efl_event_callback_call(data, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, NULL);
     }
}

/**
 * @internal
 * @brief Callback invoked when a registered radio button is invalidated (e.g., deleted).
 * This function automatically unregisters the radio button from the group.
 * @param data The radio group object (Eo *).
 * @param ev The event information, where ev->object is the radio button being invalidated.
 */
static void
_invalidate_cb(void *data, const Efl_Event *ev)
{
   efl_ui_radio_group_unregister(data, ev->object);
}

/**
 * @internal
 * @brief Array defining callbacks to be attached to each registered radio button.
 * This includes handling selection changes and object invalidation.
 * Example structure of elements:
 * @code
 * [
 *   { .key = EFL_UI_EVENT_SELECTED_CHANGED, .func = _selected_cb },
 *   { .key = EFL_EVENT_INVALIDATE, .func = _invalidate_cb }
 * ]
 * @endcode
 */
EFL_CALLBACKS_ARRAY_DEFINE(radio_btn_cb,
  {EFL_UI_EVENT_SELECTED_CHANGED, _selected_cb},
  {EFL_EVENT_INVALIDATE, _invalidate_cb},
)

/**
 * @internal
 * @brief Registers a radio button with the group.
 * Ensures the radio button is not already part of another group and that its value is unique within this group.
 * Attaches necessary event listeners to the radio button.
 * @param radio The Efl_Ui_Radio object to register.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_ui_radio_group_register(Eo *obj, Efl_Ui_Radio_Group_Impl_Data *pd, Efl_Ui_Radio *radio)
{
   Efl_Ui_Radio *reged;
   Eina_List *n;

   if (eina_hash_find(radio_group_map, &radio))
     {
        ERR("Radio button %p is already part of another group", radio);
        return;
     }

   EINA_LIST_FOREACH(pd->registered_set, n, reged)
     {
        EINA_SAFETY_ON_TRUE_RETURN(radio == reged);
        EINA_SAFETY_ON_TRUE_RETURN(efl_ui_radio_state_value_get(radio) == efl_ui_radio_state_value_get(reged));
     }
   EINA_SAFETY_ON_TRUE_RETURN(efl_ui_radio_state_value_get(radio) == -1);

   pd->registered_set = eina_list_append(pd->registered_set, radio);
   eina_hash_add(radio_group_map, &radio, obj);
   efl_event_callback_array_add(radio, radio_btn_cb(), obj);
}

/**
 * @internal
 * @brief Unregisters a radio button from the group.
 * If the radio button was selected, it is deselected.
 * Removes event listeners and cleans up internal tracking.
 * @param radio The Efl_Ui_Radio object to unregister.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_ui_radio_group_unregister(Eo *obj, Efl_Ui_Radio_Group_Impl_Data *pd, Efl_Ui_Radio *radio)
{
   if (pd->selected == radio)
     efl_ui_selectable_selected_set(pd->selected, EINA_FALSE);

   efl_event_callback_array_del(radio, radio_btn_cb(), obj);
   pd->registered_set = eina_list_remove(pd->registered_set, radio);
   eina_hash_del(radio_group_map, &radio, obj);
}

/**
 * @internal
 * @brief Destructor for the Efl_Ui_Radio_Group_Impl object.
 * Cleans up all registered radio buttons by removing event listeners and
 * removing them from the global radio_group_map.
 * Frees the list of registered radio buttons.
 */
EOLIAN static void
_efl_ui_radio_group_impl_efl_object_destructor(Eo *obj, Efl_Ui_Radio_Group_Impl_Data *pd)
{
   Eo *radio;

   // Iterate over a copy or be careful, as unregister might modify the list
   // However, here we are just cleaning up, not triggering full unregister logic
   EINA_LIST_FREE(pd->registered_set, radio)
     {
        efl_event_callback_array_del(radio, radio_btn_cb(), obj);
        eina_hash_del(radio_group_map, &radio, obj); // Remove from global map
     }
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Class constructor for Efl_Ui_Radio_Group_Impl.
 * Initializes the global radio_group_map hash table. This map is used
 * to ensure that a radio button is not added to more than one group.
 */
void
_efl_ui_radio_group_impl_class_constructor(Efl_Class *klass EINA_UNUSED)
{
   radio_group_map = eina_hash_pointer_new(NULL);
}

#include "efl_ui_radio_group_impl.eo.c"
