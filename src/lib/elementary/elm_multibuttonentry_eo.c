/**
 * @brief Event descriptor for the "item,selected" event.
 * This event is triggered when an item in the multibuttonentry is selected.
 * The event information is the selected Efl_Object.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_ITEM_SELECTED =
   EFL_EVENT_DESCRIPTION("item,selected");
/**
 * @brief Event descriptor for the "item,added" event.
 * This event is triggered when an item is added to the multibuttonentry.
 * The event information is the added Efl_Object.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_ITEM_ADDED =
   EFL_EVENT_DESCRIPTION("item,added");
/**
 * @brief Event descriptor for the "item,deleted" event.
 * This event is triggered when an item is deleted from the multibuttonentry.
 * The event information is the deleted Efl_Object.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_ITEM_DELETED =
   EFL_EVENT_DESCRIPTION("item,deleted");
/**
 * @brief Event descriptor for the "item,clicked" event.
 * This event is triggered when an item in the multibuttonentry is clicked.
 * The event information is the clicked Efl_Object.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_ITEM_CLICKED =
   EFL_EVENT_DESCRIPTION("item,clicked");
/**
 * @brief Event descriptor for the "item,longpressed" event.
 * This event is triggered when an item in the multibuttonentry is long-pressed.
 * The event information is the long-pressed Efl_Object.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_ITEM_LONGPRESSED =
   EFL_EVENT_DESCRIPTION("item,longpressed");
/**
 * @brief Event descriptor for the "expanded" event.
 * This event is triggered when the multibuttonentry is expanded.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_EXPANDED =
   EFL_EVENT_DESCRIPTION("expanded");
/**
 * @brief Event descriptor for the "contracted" event.
 * This event is triggered when the multibuttonentry is contracted.
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_CONTRACTED =
   EFL_EVENT_DESCRIPTION("contracted");
/**
 * @brief Event descriptor for the "expand,state,changed" event.
 * This event is triggered when the expanded state of the multibuttonentry changes.
 * The event information is an int representing the new state (1 for expanded, 0 for contracted).
 */
EWAPI const Efl_Event_Description _ELM_MULTIBUTTONENTRY_EVENT_EXPAND_STATE_CHANGED =
   EFL_EVENT_DESCRIPTION("expand,state,changed");

/**
 * @internal
 * @brief Implements the Eolian method for setting the editable state.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param editable The new editable state.
 */
void _elm_multibuttonentry_editable_set(Eo *obj, Elm_Multibuttonentry_Data *pd, Eina_Bool editable);


/**
 * @internal
 * @brief Reflection function for the editable property setter.
 * Converts an Eina_Value to Eina_Bool and calls the setter.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean editable state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_multibuttonentry_editable_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_multibuttonentry_editable_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_editable_set, EFL_FUNC_CALL(editable), Eina_Bool editable);

/**
 * @internal
 * @brief Implements the Eolian method for getting the editable state.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The current editable state.
 */
Eina_Bool _elm_multibuttonentry_editable_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the editable property getter.
 * Calls the getter and returns the Eina_Bool as an Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean editable state.
 */
static Eina_Value
__eolian_elm_multibuttonentry_editable_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_multibuttonentry_editable_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_editable_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the Eolian method for setting the expanded state.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param expanded The new expanded state.
 */
void _elm_multibuttonentry_expanded_set(Eo *obj, Elm_Multibuttonentry_Data *pd, Eina_Bool expanded);

/**
 * @internal
 * @brief Reflection function for the expanded property setter.
 * Converts an Eina_Value to Eina_Bool and calls the setter.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean expanded state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_multibuttonentry_expanded_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_multibuttonentry_expanded_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_expanded_set, EFL_FUNC_CALL(expanded), Eina_Bool expanded);

/**
 * @internal
 * @brief Implements the Eolian method for getting the expanded state.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The current expanded state.
 */
Eina_Bool _elm_multibuttonentry_expanded_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Reflection function for the expanded property getter.
 * Calls the getter and returns the Eina_Bool as an Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean expanded state.
 */
static Eina_Value
__eolian_elm_multibuttonentry_expanded_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_multibuttonentry_expanded_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_expanded_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the Eolian method for setting the format function.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param format_function The function to format the hidden items counter string.
 * @param data User data to be passed to the format function.
 */
void _elm_multibuttonentry_format_function_set(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Multibuttonentry_Format_Cb format_function, const void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_format_function_set, EFL_FUNC_CALL(format_function, data), Elm_Multibuttonentry_Format_Cb format_function, const void *data);

/**
 * @internal
 * @brief Implements the Eolian method for getting the list of items.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return A const Eina_List of Elm_Widget_Item objects.
 */
const Eina_List *_elm_multibuttonentry_items_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method for getting the first item.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The first Elm_Widget_Item or NULL if the list is empty.
 */
Elm_Widget_Item *_elm_multibuttonentry_first_item_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method for getting the last item.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The last Elm_Widget_Item or NULL if the list is empty.
 */
Elm_Widget_Item *_elm_multibuttonentry_last_item_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method for getting the internal entry object.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The Efl_Canvas_Object used as the text entry.
 */
Efl_Canvas_Object *_elm_multibuttonentry_entry_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_entry_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method for getting the selected item.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The currently selected Elm_Widget_Item or NULL if none is selected.
 */
Elm_Widget_Item *_elm_multibuttonentry_selected_item_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_multibuttonentry_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method for prepending an item.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param label The label for the new item.
 * @param func The callback function to call when the item is clicked.
 * @param data User data for the callback function.
 * @return The newly created Elm_Widget_Item or NULL on failure.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_prepend(Eo *obj, Elm_Multibuttonentry_Data *pd, const char *label, Evas_Smart_Cb func, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_multibuttonentry_item_prepend, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, func, data), const char *label, Evas_Smart_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for clearing all items.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 */
void _elm_multibuttonentry_clear(Eo *obj, Elm_Multibuttonentry_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_multibuttonentry_clear);

/**
 * @internal
 * @brief Implements the Eolian method for removing an item filter.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param func The filter function to remove.
 * @param data The user data associated with the filter function.
 */
void _elm_multibuttonentry_item_filter_remove(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_item_filter_remove, EFL_FUNC_CALL(func, data), Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for inserting an item before another.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param before The item before which to insert the new item.
 * @param label The label for the new item.
 * @param func The callback function to call when the item is clicked.
 * @param data User data for the callback function.
 * @return The newly created Elm_Widget_Item or NULL on failure.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_insert_before(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Widget_Item *before, const char *label, Evas_Smart_Cb func, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_multibuttonentry_item_insert_before, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(before, label, func, data), Elm_Widget_Item *before, const char *label, Evas_Smart_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for appending an item.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param label The label for the new item.
 * @param func The callback function to call when the item is clicked.
 * @param data User data for the callback function.
 * @return The newly created Elm_Widget_Item or NULL on failure.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_append(Eo *obj, Elm_Multibuttonentry_Data *pd, const char *label, Evas_Smart_Cb func, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_multibuttonentry_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, func, data), const char *label, Evas_Smart_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for prepending an item filter.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param func The filter function to prepend.
 * @param data User data for the filter function.
 */
void _elm_multibuttonentry_item_filter_prepend(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_item_filter_prepend, EFL_FUNC_CALL(func, data), Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for appending an item filter.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param func The filter function to append.
 * @param data User data for the filter function.
 */
void _elm_multibuttonentry_item_filter_append(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_multibuttonentry_item_filter_append, EFL_FUNC_CALL(func, data), Elm_Multibuttonentry_Item_Filter_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian method for inserting an item after another.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param after The item after which to insert the new item.
 * @param label The label for the new item.
 * @param func The callback function to call when the item is clicked.
 * @param data User data for the callback function.
 * @return The newly created Elm_Widget_Item or NULL on failure.
 */
Elm_Widget_Item *_elm_multibuttonentry_item_insert_after(Eo *obj, Elm_Multibuttonentry_Data *pd, Elm_Widget_Item *after, const char *label, Evas_Smart_Cb func, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_multibuttonentry_item_insert_after, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(after, label, func, data), Elm_Widget_Item *after, const char *label, Evas_Smart_Cb func, void *data);

/**
 * @internal
 * @brief Implements the Eolian efl_object_constructor method.
 * This function is called when a new multibuttonentry object is constructed.
 * @param obj The Evas object being constructed.
 * @param pd The private data of the object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_multibuttonentry_efl_object_constructor(Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Implements the Eolian efl_ui_widget_theme_apply method.
 * This function is called when the theme needs to be applied to the widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_multibuttonentry_efl_ui_widget_theme_apply(Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Implements the Eolian efl_ui_widget_on_access_update method.
 * This function is called when the accessibility state of the widget changes.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param enable EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
void _elm_multibuttonentry_efl_ui_widget_on_access_update(Eo *obj, Elm_Multibuttonentry_Data *pd, Eina_Bool enable);

/**
 * @internal
 * @brief Implements the Eolian efl_ui_l10n_translation_update method.
 * This function is called when the localization of the widget needs to be updated.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 */
void _elm_multibuttonentry_efl_ui_l10n_translation_update(Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Implements the Eolian efl_ui_widget_input_event_handler method.
 * This function handles input events for the widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param eo_event The Efl_Event describing the input event.
 * @param source The source Efl_Canvas_Object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_multibuttonentry_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Multibuttonentry_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Eolian efl_access_object_access_children_get method.
 * This function retrieves the accessible children of the widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return A list of Efl_Access_Object representing the children.
 */
Eina_List *_elm_multibuttonentry_efl_access_object_access_children_get(const Eo *obj, Elm_Multibuttonentry_Data *pd);

/**
 * @internal
 * @brief Implements the Eolian efl_part_part_get method.
 * This function retrieves a part of the widget by name.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param name The name of the part to retrieve.
 * @return The Efl_Object representing the part, or NULL if not found.
 */
Efl_Object *_elm_multibuttonentry_efl_part_part_get(const Eo *obj, Elm_Multibuttonentry_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Multibuttonentry class.
 * Sets up the Eolian functions for the class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_multibuttonentry_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MULTIBUTTONENTRY_EXTRA_OPS
#define ELM_MULTIBUTTONENTRY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_editable_set, _elm_multibuttonentry_editable_set),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_editable_get, _elm_multibuttonentry_editable_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_expanded_set, _elm_multibuttonentry_expanded_set),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_expanded_get, _elm_multibuttonentry_expanded_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_format_function_set, _elm_multibuttonentry_format_function_set),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_items_get, _elm_multibuttonentry_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_first_item_get, _elm_multibuttonentry_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_last_item_get, _elm_multibuttonentry_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_entry_get, _elm_multibuttonentry_entry_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_selected_item_get, _elm_multibuttonentry_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_prepend, _elm_multibuttonentry_item_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_clear, _elm_multibuttonentry_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_filter_remove, _elm_multibuttonentry_item_filter_remove),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_insert_before, _elm_multibuttonentry_item_insert_before),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_append, _elm_multibuttonentry_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_filter_prepend, _elm_multibuttonentry_item_filter_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_filter_append, _elm_multibuttonentry_item_filter_append),
      EFL_OBJECT_OP_FUNC(elm_obj_multibuttonentry_item_insert_after, _elm_multibuttonentry_item_insert_after),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_multibuttonentry_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_multibuttonentry_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_multibuttonentry_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_multibuttonentry_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_multibuttonentry_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_multibuttonentry_efl_access_object_access_children_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_multibuttonentry_efl_part_part_get),
      ELM_MULTIBUTTONENTRY_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"editable", __eolian_elm_multibuttonentry_editable_set_reflect, __eolian_elm_multibuttonentry_editable_get_reflect},
      {"expanded", __eolian_elm_multibuttonentry_expanded_set_reflect, __eolian_elm_multibuttonentry_expanded_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Multibuttonentry Eolian class.
 * Contains metadata about the class, such as its name, version, type,
 * data size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_multibuttonentry_class_desc = {
   EO_VERSION, /**< Eolian object version. */
   "Elm.Multibuttonentry", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Elm_Multibuttonentry_Data),
   _elm_multibuttonentry_class_initializer,
   _elm_multibuttonentry_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_multibuttonentry_class_get, &_elm_multibuttonentry_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_multibuttonentry_eo.legacy.c"
