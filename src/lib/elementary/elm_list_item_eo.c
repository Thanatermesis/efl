
/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_separator_set.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @param setting The new separator state.
 */
void _elm_list_item_separator_set(Eo *obj, Elm_List_Item_Data *pd, Eina_Bool setting);


/**
 * @internal
 * @brief Eolian reflection function for the @c separator property setter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "separator" property is set. It converts an Eina_Value
 * to the appropriate C type and calls the concrete implementation.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value to set for the property.
 * @return EINA_ERROR_NO_ERROR on success, another Eina_Error on failure.
 */
static Eina_Error
__eolian_elm_list_item_separator_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_list_item_separator_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_list_item_separator_set, EFL_FUNC_CALL(setting), Eina_Bool setting);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_separator_get.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @return The current separator state.
 */
Eina_Bool _elm_list_item_separator_get(const Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Eolian reflection function for the @c separator property getter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "separator" property is read. It calls the concrete
 * implementation and converts the C type result to an Eina_Value.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the property's current value.
 */
static Eina_Value
__eolian_elm_list_item_separator_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_list_item_separator_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_list_item_separator_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_selected_set.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @param selected The new selected state.
 */
void _elm_list_item_selected_set(Eo *obj, Elm_List_Item_Data *pd, Eina_Bool selected);


/**
 * @internal
 * @brief Eolian reflection function for the @c selected property setter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "selected" property is set. It converts an Eina_Value
 * to the appropriate C type and calls the concrete implementation.
 *
 * @param obj The Efl_Object instance.
 * @param val The Eina_Value to set for the property.
 * @return EINA_ERROR_NO_ERROR on success, another Eina_Error on failure.
 */
static Eina_Error
__eolian_elm_list_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_list_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_list_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_selected_get.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @return The current selected state.
 */
Eina_Bool _elm_list_item_selected_get(const Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Eolian reflection function for the @c selected property getter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "selected" property is read. It calls the concrete
 * implementation and converts the C type result to an Eina_Value.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the property's current value.
 */
static Eina_Value
__eolian_elm_list_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_list_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_list_item_selected_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_object_get.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @return The base Evas object for the item.
 */
Efl_Canvas_Object *_elm_list_item_object_get(const Eo *obj, Elm_List_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_list_item_object_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_prev_get.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @return The previous item in the list, or @c NULL.
 */
Elm_Widget_Item *_elm_list_item_prev_get(const Eo *obj, Elm_List_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_list_item_prev_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_next_get.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 * @return The next item in the list, or @c NULL.
 */
Elm_Widget_Item *_elm_list_item_next_get(const Eo *obj, Elm_List_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_list_item_next_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_show.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_show(Eo *obj, Elm_List_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_list_item_show);

/**
 * @internal
 * @brief Implements the Eolian method #elm_obj_list_item_bring_in.
 * @param obj The Efl_Object instance.
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_bring_in(Eo *obj, Elm_List_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_list_item_bring_in);

/**
 * @internal
 * @brief Implements the Efl_Object.constructor method.
 *
 * This function is called when a new instance of Elm_List_Item is created.
 * It should handle the basic initialization of the object.
 *
 * @param obj The Efl_Object instance being constructed.
 * @param pd Private data for the Eo object.
 * @return The initialized Efl_Object instance, or @c NULL on failure.
 */
Efl_Object *_elm_list_item_efl_object_constructor(Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Efl_Object.invalidate method.
 *
 * This function is called when the object is about to be invalidated.
 * It should release resources that can be recreated, but not destroy
 * the object itself.
 *
 * @param obj The Efl_Object instance being invalidated.
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_efl_object_invalidate(Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Efl_Object.destructor method.
 *
 * This function is called when the object is being destroyed.
 * It should free all resources allocated by the object.
 *
 * @param obj The Efl_Object instance being destructed.
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_efl_object_destructor(Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.disable method.
 *
 * This function is called to disable the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_elm_widget_item_disable(Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.del_pre method.
 *
 * This function is called before the widget item is deleted.
 * It allows for pre-deletion cleanup.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 */
void _elm_list_item_elm_widget_item_del_pre(Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.signal_emit method.
 *
 * This function is called to emit a signal from the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param emission The signal string.
 * @param source The source string of the signal.
 */
void _elm_list_item_elm_widget_item_signal_emit(Eo *obj, Elm_List_Item_Data *pd, const char *emission, const char *source);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.item_focus_set method.
 *
 * This function is called to set the focus state of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param focused @c EINA_TRUE if focused, @c EINA_FALSE otherwise.
 */
void _elm_list_item_elm_widget_item_item_focus_set(Eo *obj, Elm_List_Item_Data *pd, Eina_Bool focused);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.item_focus_get method.
 *
 * This function is called to get the focus state of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @return @c EINA_TRUE if focused, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_list_item_elm_widget_item_item_focus_get(const Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.part_text_set method.
 *
 * This function is called to set the text of a part of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param part The name of the part to set text for.
 * @param label The text to set.
 */
void _elm_list_item_elm_widget_item_part_text_set(Eo *obj, Elm_List_Item_Data *pd, const char *part, const char *label);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.part_text_get method.
 *
 * This function is called to get the text of a part of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param part The name of the part to get text from.
 * @return The text of the part, or @c NULL.
 */
const char *_elm_list_item_elm_widget_item_part_text_get(const Eo *obj, Elm_List_Item_Data *pd, const char *part);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.part_content_set method.
 *
 * This function is called to set the content of a part of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param part The name of the part to set content for.
 * @param content The Efl_Canvas_Object to set as content.
 */
void _elm_list_item_elm_widget_item_part_content_set(Eo *obj, Elm_List_Item_Data *pd, const char *part, Efl_Canvas_Object *content);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.part_content_get method.
 *
 * This function is called to get the content of a part of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param part The name of the part to get content from.
 * @return The Efl_Canvas_Object content of the part, or @c NULL.
 */
Efl_Canvas_Object *_elm_list_item_elm_widget_item_part_content_get(const Eo *obj, Elm_List_Item_Data *pd, const char *part);


/**
 * @internal
 * @brief Implements the Elm_Widget_Item.part_content_unset method.
 *
 * This function is called to unset (remove) the content of a part of the widget item.
 *
 * @param obj The Efl_Object instance (widget item).
 * @param pd Private data for the Eo object.
 * @param part The name of the part to unset content from.
 * @return The previously set Efl_Canvas_Object content, or @c NULL.
 */
Efl_Canvas_Object *_elm_list_item_elm_widget_item_part_content_unset(Eo *obj, Elm_List_Item_Data *pd, const char *part);


/**
 * @internal
 * @brief Implements the Efl_Access_Object.i18n_name_get method.
 *
 * This function is called to get the internationalized name of the accessible object.
 *
 * @param obj The Efl_Object instance (accessible object).
 * @param pd Private data for the Eo object.
 * @return The internationalized name string, or @c NULL.
 */
const char *_elm_list_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Implements the Efl_Access_Object.state_set_get method.
 *
 * This function is called to get the set of accessibility states of the object.
 *
 * @param obj The Efl_Object instance (accessible object).
 * @param pd Private data for the Eo object.
 * @return The Efl_Access_State_Set representing the object's states.
 */
Efl_Access_State_Set _elm_list_item_efl_access_object_state_set_get(const Eo *obj, Elm_List_Item_Data *pd);


/**
 * @internal
 * @brief Initializes the Elm_List_Item Efl_Class.
 *
 * This function is called once when the Efl_Class for Elm_List_Item is being set up.
 * It defines the operations (methods) provided by this class by associating
 * Eolian function opcodes with their C implementations. It also sets up
 * property reflection if any properties are defined.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_list_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_LIST_ITEM_EXTRA_OPS
#define ELM_LIST_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_separator_set, _elm_list_item_separator_set),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_separator_get, _elm_list_item_separator_get),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_selected_set, _elm_list_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_selected_get, _elm_list_item_selected_get),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_object_get, _elm_list_item_object_get),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_prev_get, _elm_list_item_prev_get),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_next_get, _elm_list_item_next_get),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_show, _elm_list_item_show),
      EFL_OBJECT_OP_FUNC(elm_obj_list_item_bring_in, _elm_list_item_bring_in),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_list_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_invalidate, _elm_list_item_efl_object_invalidate),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_list_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_list_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_del_pre, _elm_list_item_elm_widget_item_del_pre),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_list_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_set, _elm_list_item_elm_widget_item_item_focus_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_get, _elm_list_item_elm_widget_item_item_focus_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_list_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_list_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_list_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_list_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_unset, _elm_list_item_elm_widget_item_part_content_unset),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_list_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_list_item_efl_access_object_state_set_get),
      ELM_LIST_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"separator", __eolian_elm_list_item_separator_set_reflect, __eolian_elm_list_item_separator_get_reflect},
      {"selected", __eolian_elm_list_item_selected_set_reflect, __eolian_elm_list_item_selected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_List_Item class.
 *
 * This structure provides metadata for the Elm_List_Item class,
 * including its version, name, type, size of its private data structure,
 * and pointers to its class initializer and constructor/destructor functions.
 */
static const Efl_Class_Description _elm_list_item_class_desc = {
   EO_VERSION,
   "Elm.List.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_List_Item_Data),
   _elm_list_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_list_item_class_get, &_elm_list_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_list_item_eo.legacy.c"
