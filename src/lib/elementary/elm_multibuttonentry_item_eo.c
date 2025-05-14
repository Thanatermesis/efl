/**
 * @internal
 * @brief Sets the selected state of the multibuttonentry item.
 *
 * This function is the C implementation for the EAPI function
 * elm_obj_multibuttonentry_item_selected_set().
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @param[in] selected #EINA_TRUE to set the item as selected, #EINA_FALSE otherwise.
 */
void _elm_multibuttonentry_item_selected_set(Eo *obj, Elm_Multibuttonentry_Item_Data *pd, Eina_Bool selected);


static Eina_Error
__eolian_elm_multibuttonentry_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_multibuttonentry_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Gets the selected state of the multibuttonentry item.
 *
 * This function is the C implementation for the EAPI function
 * elm_obj_multibuttonentry_item_selected_get().
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return #EINA_TRUE if the item is selected, #EINA_FALSE otherwise.
 */
Eina_Bool _elm_multibuttonentry_item_selected_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);


static Eina_Value
__eolian_elm_multibuttonentry_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_multibuttonentry_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_item_selected_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Gets the previous item in the multibuttonentry.
 *
 * This function is the C implementation for the EAPI function
 * elm_obj_multibuttonentry_item_prev_get().
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return A pointer to the previous Elm_Widget_Item, or @c NULL if none exists.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_prev_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_item_prev_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Gets the next item in the multibuttonentry.
 *
 * This function is the C implementation for the EAPI function
 * elm_obj_multibuttonentry_item_next_get().
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return A pointer to the next Elm_Widget_Item, or @c NULL if none exists.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_next_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_item_next_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Constructor for the Elm_Multibuttonentry_Item object.
 *
 * This function is called when a new instance of Elm_Multibuttonentry_Item is created.
 * It handles the initialization of the object.
 * This implements the Efl.Object.constructor interface.
 *
 * @param[in] obj The Evas object to construct.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_multibuttonentry_item_efl_object_constructor(Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Destructor for the Elm_Multibuttonentry_Item object.
 *
 * This function is called when an instance of Elm_Multibuttonentry_Item is being destroyed.
 * It handles the cleanup and deallocation of resources used by the object.
 * This implements the Efl.Object.destructor interface.
 *
 * @param[in] obj The Evas object to destruct.
 * @param[in] pd Pointer to the private data structure of this item.
 */
void _elm_multibuttonentry_item_efl_object_destructor(Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Emits a signal from the multibuttonentry item.
 *
 * This function implements the Elm.Widget.Item.signal_emit interface.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @param[in] emission The signal string to emit.
 * @param[in] source The source string of the signal.
 */
void _elm_multibuttonentry_item_elm_widget_item_signal_emit(Eo *obj, Elm_Multibuttonentry_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Sets the text of a part of the multibuttonentry item.
 *
 * This function implements the Elm.Widget.Item.part_text_set interface.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @param[in] part The name of the part to set the text for (e.g., "default").
 * @param[in] label The text to set.
 */
void _elm_multibuttonentry_item_elm_widget_item_part_text_set(Eo *obj, Elm_Multibuttonentry_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Gets the text of a part of the multibuttonentry item.
 *
 * This function implements the Elm.Widget.Item.part_text_get interface.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @param[in] part The name of the part to get the text from (e.g., "default").
 * @return The text of the specified part, or @c NULL if not set or part does not exist.
 */
const char *_elm_multibuttonentry_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Disables the multibuttonentry item.
 *
 * This function implements the Elm.Widget.Item.disable interface.
 * When disabled, the item typically changes its appearance and does not react to user input.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 */
void _elm_multibuttonentry_item_elm_widget_item_disable(Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Gets the internationalized name for accessibility.
 *
 * This function implements the Efl.Access.Object.i18n_name_get interface.
 * It provides a human-readable, localized name for the item, typically used by screen readers.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return The internationalized name string.
 */
const char *_elm_multibuttonentry_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Gets the state set for accessibility.
 *
 * This function implements the Efl.Access.Object.state_set_get interface.
 * It provides a bitmask of states (e.g., selected, focused, disabled) describing the item.
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return The Efl_Access_State_Set representing the current states of the item.
 */
Efl_Access_State_Set _elm_multibuttonentry_item_efl_access_object_state_set_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Gets the accessibility actions available for the item.
 *
 * This function implements the Efl.Access.Widget_Action.elm_actions_get interface.
 * It provides a list of actions that can be performed on the item (e.g., click).
 *
 * @param[in] obj The Evas object associated with this item.
 * @param[in] pd Pointer to the private data structure of this item.
 * @return A pointer to an array of Efl_Access_Action_Data, terminated by an entry with a @c NULL name.
 *         Returns @c NULL if no actions are available.
 *         Example:
 *         static const Efl_Access_Action_Data actions[] = {
 *            { "click", "click" }, // action name, function to call (name)
 *            { NULL, NULL }
 *         };
 *         return actions;
 */
const Efl_Access_Action_Data *_elm_multibuttonentry_item_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Multibuttonentry_Item_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Multibuttonentry_Item class.
 *
 * This function is called once when the Efl class system initializes
 * the Elm_Multibuttonentry_Item class. It sets up the Evas Object operations
 * (methods) and reflection data for this class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_multibuttonentry_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MULTIBUTTONENTRY_ITEM_EXTRA_OPS
#define ELM_MULTIBUTTONENTRY_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_selected_set, _elm_multibuttonentry_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_selected_get, _elm_multibuttonentry_item_selected_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_prev_get, _elm_multibuttonentry_item_prev_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_next_get, _elm_multibuttonentry_item_next_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_multibuttonentry_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_multibuttonentry_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_multibuttonentry_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_multibuttonentry_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_multibuttonentry_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_multibuttonentry_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_multibuttonentry_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_multibuttonentry_item_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_multibuttonentry_item_efl_access_widget_action_elm_actions_get),
      ELM_MULTIBUTTONENTRY_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"selected", __eolian_elm_multibuttonentry_item_selected_set_reflect, __eolian_elm_multibuttonentry_item_selected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_multibuttonentry_item_class_desc = {
   EO_VERSION,
   "Elm.Multibuttonentry_Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Multibuttonentry_Item_Data),
   _elm_multibuttonentry_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_multibuttonentry_item_class_get, &_elm_multibuttonentry_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_multibuttonentry_item_eo.legacy.c"
