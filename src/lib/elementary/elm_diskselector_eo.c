/**
 * @brief Implements the Eolian callable function for setting the side text maximum length.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_side_text_max_length_set method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] len The maximum length for side item texts.
 */
void _elm_diskselector_side_text_max_length_set(Eo *obj, Elm_Diskselector_Data *pd, int len);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_side_text_max_length_set method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "side_text_max_length" property is set via reflection. It converts
 * the Eina_Value @p val to an integer and calls the actual Eolian method
 * elm_obj_diskselector_side_text_max_length_set.
 *
 * @param[in] obj The Eo object.
 * @param[in] val An Eina_Value containing the integer value for the max length.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_diskselector_side_text_max_length_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_diskselector_side_text_max_length_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_diskselector_side_text_max_length_set, EFL_FUNC_CALL(len), int len);

/**
 * @brief Implements the Eolian callable function for getting the side text maximum length.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_side_text_max_length_get method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return The maximum length for side item texts.
 */
int _elm_diskselector_side_text_max_length_get(const Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_side_text_max_length_get method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "side_text_max_length" property is read via reflection. It calls
 * the actual Eolian method elm_obj_diskselector_side_text_max_length_get and
 * wraps the returned integer in an Eina_Value.
 *
 * @param[in] obj The Eo object.
 * @return An Eina_Value containing the integer value of the max length.
 */
static Eina_Value
__eolian_elm_diskselector_side_text_max_length_get_reflect(const Eo *obj)
{
   int val = elm_obj_diskselector_side_text_max_length_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_side_text_max_length_get, int, 0);

/**
 * @brief Implements the Eolian callable function for enabling or disabling round mode.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_round_enabled_set method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] enabled #EINA_TRUE to enable round mode, #EINA_FALSE to disable.
 */
void _elm_diskselector_round_enabled_set(Eo *obj, Elm_Diskselector_Data *pd, Eina_Bool enabled);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_round_enabled_set method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "round_enabled" property is set via reflection. It converts
 * the Eina_Value @p val to an Eina_Bool and calls the actual Eolian method
 * elm_obj_diskselector_round_enabled_set.
 *
 * @param[in] obj The Eo object.
 * @param[in] val An Eina_Value containing the Eina_Bool value for round mode.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_diskselector_round_enabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_diskselector_round_enabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_diskselector_round_enabled_set, EFL_FUNC_CALL(enabled), Eina_Bool enabled);

/**
 * @brief Implements the Eolian callable function for getting the round mode state.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_round_enabled_get method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return #EINA_TRUE if round mode is enabled, #EINA_FALSE otherwise.
 */
Eina_Bool _elm_diskselector_round_enabled_get(const Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_round_enabled_get method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "round_enabled" property is read via reflection. It calls
 * the actual Eolian method elm_obj_diskselector_round_enabled_get and
 * wraps the returned Eina_Bool in an Eina_Value.
 *
 * @param[in] obj The Eo object.
 * @return An Eina_Value containing the Eina_Bool value of the round mode state.
 */
static Eina_Value
__eolian_elm_diskselector_round_enabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_diskselector_round_enabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_round_enabled_get, Eina_Bool, 0);

/**
 * @brief Implements the Eolian callable function for setting the number of displayed items.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_display_item_num_set method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] num The number of items to display.
 */
void _elm_diskselector_display_item_num_set(Eo *obj, Elm_Diskselector_Data *pd, int num);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_display_item_num_set method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "display_item_num" property is set via reflection. It converts
 * the Eina_Value @p val to an integer and calls the actual Eolian method
 * elm_obj_diskselector_display_item_num_set.
 *
 * @param[in] obj The Eo object.
 * @param[in] val An Eina_Value containing the integer value for the number of items.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_diskselector_display_item_num_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_diskselector_display_item_num_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_diskselector_display_item_num_set, EFL_FUNC_CALL(num), int num);

/**
 * @brief Implements the Eolian callable function for getting the number of displayed items.
 *
 * This function is the C implementation corresponding to the Eolian
 * @c elm_obj_diskselector_display_item_num_get method. It is called internally
 * by the Eolian generated wrapper.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return The number of items displayed.
 */
int _elm_diskselector_display_item_num_get(const Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the elm_obj_diskselector_display_item_num_get method.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the "display_item_num" property is read via reflection. It calls
 * the actual Eolian method elm_obj_diskselector_display_item_num_get and
 * wraps the returned integer in an Eina_Value.
 *
 * @param[in] obj The Eo object.
 * @return An Eina_Value containing the integer value of the number of items.
 */
static Eina_Value
__eolian_elm_diskselector_display_item_num_get_reflect(const Eo *obj)
{
   int val = elm_obj_diskselector_display_item_num_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_display_item_num_get, int, 0);

/**
 * @brief Implements the Eolian callable function for getting the first item.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return The first Elm_Widget_Item or NULL if none.
 */
Elm_Widget_Item *_elm_diskselector_first_item_get(const Eo *obj, Elm_Diskselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @brief Implements the Eolian callable function for getting all items.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return A const Eina_List of Elm_Widget_Item objects or NULL.
 */
const Eina_List *_elm_diskselector_items_get(const Eo *obj, Elm_Diskselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_items_get, const Eina_List *, NULL);

/**
 * @brief Implements the Eolian callable function for getting the last item.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return The last Elm_Widget_Item or NULL if none.
 */
Elm_Widget_Item *_elm_diskselector_last_item_get(const Eo *obj, Elm_Diskselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @brief Implements the Eolian callable function for getting the selected item.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return The selected Elm_Widget_Item or NULL if none.
 */
Elm_Widget_Item *_elm_diskselector_selected_item_get(const Eo *obj, Elm_Diskselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @brief Implements the Eolian callable function for appending an item.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] label The label for the new item.
 * @param[in] icon An optional icon for the new item.
 * @param[in] func A callback function to be called when the item is selected.
 * @param[in] data User data for the callback function.
 * @return The newly appended Elm_Widget_Item or NULL on failure.
 */
Elm_Widget_Item *_elm_diskselector_item_append(Eo *obj, Elm_Diskselector_Data *pd, const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_diskselector_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, icon, func, data), const char *label, Efl_Canvas_Object *icon, Evas_Smart_Cb func, const void *data);

/**
 * @brief Implements the Eolian callable function for clearing all items.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 */
void _elm_diskselector_clear(Eo *obj, Elm_Diskselector_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_diskselector_clear);

/**
 * @brief Implements the Efl_Object constructor for Elm_Diskselector.
 * @param[in] obj The Eo object to construct.
 * @param[in] pd Pointer to the private data of the object.
 * @return The constructed Eo object.
 */
Efl_Object *_elm_diskselector_efl_object_constructor(Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @brief Implements the Efl_Gfx_Entity position_set interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] pos The new position (x, y).
 */
void _elm_diskselector_efl_gfx_entity_position_set(Eo *obj, Elm_Diskselector_Data *pd, Eina_Position2D pos);

/**
 * @brief Implements the Efl_Gfx_Entity size_set interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] size The new size (width, height).
 */
void _elm_diskselector_efl_gfx_entity_size_set(Eo *obj, Elm_Diskselector_Data *pd, Eina_Size2D size);

/**
 * @brief Implements the Efl_Canvas_Group member_add interface.
 * @param[in] obj The Eo object (group).
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] sub_obj The sub_object to add to the group.
 */
void _elm_diskselector_efl_canvas_group_group_member_add(Eo *obj, Elm_Diskselector_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @brief Implements the Efl_Ui_Widget on_access_update interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] enable EINA_TRUE if accessibility is to be enabled, EINA_FALSE otherwise.
 */
void _elm_diskselector_efl_ui_widget_on_access_update(Eo *obj, Elm_Diskselector_Data *pd, Eina_Bool enable);

/**
 * @brief Implements the Efl_Ui_Widget theme_apply interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return Eina_Error code, EINA_ERROR_NO_ERROR on success.
 */
Eina_Error _elm_diskselector_efl_ui_widget_theme_apply(Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @brief Implements the Efl_Ui_Focus_Object on_focus_update interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return EINA_TRUE if focus update was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_diskselector_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @brief Implements the Efl_Ui_L10n translation_update interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 */
void _elm_diskselector_efl_ui_l10n_translation_update(Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @brief Implements the Efl_Ui_Widget sub_object_del interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] sub_obj The sub_object to delete.
 * @return EINA_TRUE if sub_object was successfully deleted, EINA_FALSE otherwise.
 */
Eina_Bool _elm_diskselector_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Diskselector_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @brief Implements the Efl_Ui_Widget input_event_handler interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] eo_event The Efl_Event description.
 * @param[in] source The source canvas object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_diskselector_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Diskselector_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @brief Implements the Elm_Interface_Scrollable policy_set interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[in] hbar The horizontal scrollbar policy.
 * @param[in] vbar The vertical scrollbar policy.
 */
void _elm_diskselector_elm_interface_scrollable_policy_set(Eo *obj, Elm_Diskselector_Data *pd, Elm_Scroller_Policy hbar, Elm_Scroller_Policy vbar);

/**
 * @brief Implements the Elm_Interface_Scrollable policy_get interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @param[out] hbar Pointer to store the horizontal scrollbar policy.
 * @param[out] vbar Pointer to store the vertical scrollbar policy.
 */
void _elm_diskselector_elm_interface_scrollable_policy_get(const Eo *obj, Elm_Diskselector_Data *pd, Elm_Scroller_Policy *hbar, Elm_Scroller_Policy *vbar);

/**
 * @brief Implements the Efl_Access_Widget_Action elm_actions_get interface.
 * @param[in] obj The Eo object.
 * @param[in] pd Pointer to the private data of the object.
 * @return A const Efl_Access_Action_Data array describing widget actions, terminated by an entry with name=NULL.
 */
const Efl_Access_Action_Data *_elm_diskselector_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Diskselector_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Diskselector Efl_Class.
 *
 * This function is called by Eolian to set up the class. It defines
 * the operations (method implementations) and property reflection handlers
 * for the Elm_Diskselector class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_diskselector_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_DISKSELECTOR_EXTRA_OPS
#define ELM_DISKSELECTOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_side_text_max_length_set, _elm_diskselector_side_text_max_length_set),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_side_text_max_length_get, _elm_diskselector_side_text_max_length_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_round_enabled_set, _elm_diskselector_round_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_round_enabled_get, _elm_diskselector_round_enabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_display_item_num_set, _elm_diskselector_display_item_num_set),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_display_item_num_get, _elm_diskselector_display_item_num_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_first_item_get, _elm_diskselector_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_items_get, _elm_diskselector_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_last_item_get, _elm_diskselector_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_selected_item_get, _elm_diskselector_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_item_append, _elm_diskselector_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_clear, _elm_diskselector_clear),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_diskselector_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_diskselector_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_diskselector_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_diskselector_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_diskselector_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_diskselector_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_diskselector_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_ui_l10n_translation_update, _elm_diskselector_efl_ui_l10n_translation_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_diskselector_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_diskselector_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_policy_set, _elm_diskselector_elm_interface_scrollable_policy_set),
      EFL_OBJECT_OP_FUNC(elm_interface_scrollable_policy_get, _elm_diskselector_elm_interface_scrollable_policy_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_diskselector_efl_access_widget_action_elm_actions_get),
      ELM_DISKSELECTOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"side_text_max_length", __eolian_elm_diskselector_side_text_max_length_set_reflect, __eolian_elm_diskselector_side_text_max_length_get_reflect},
      {"round_enabled", __eolian_elm_diskselector_round_enabled_set_reflect, __eolian_elm_diskselector_round_enabled_get_reflect},
      {"display_item_num", __eolian_elm_diskselector_display_item_num_set_reflect, __eolian_elm_diskselector_display_item_num_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Efl_Class_Description for the Elm_Diskselector class.
 *
 * This structure provides the Eolian system with metadata about the
 * Elm_Diskselector class, such as its version, name, type, instance size,
 * and pointers to its class initializer, constructor, and destructor functions.
 */
static const Efl_Class_Description _elm_diskselector_class_desc = {
   EO_VERSION,
   "Elm.Diskselector",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Diskselector_Data),
   _elm_diskselector_class_initializer,
   _elm_diskselector_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_diskselector_class_get, &_elm_diskselector_class_desc, EFL_UI_WIDGET_CLASS, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_UI_SCROLLABLE_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);

/**
 * @internal
 * @file
 * @brief Includes legacy C functions related to Elm_Diskselector.
 *
 * This file is typically part of the Eolian generation process and includes
 * implementations for legacy (pre-Eolian) APIs or compatibility shims
 * for the Elm_Diskselector widget.
 */
#include "elm_diskselector_eo.legacy.c"
