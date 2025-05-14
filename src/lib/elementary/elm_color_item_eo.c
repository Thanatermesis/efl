/**
 * @internal
 * @brief Sets the color of the color item.
 *
 * This function is the internal implementation for elm_obj_color_item_color_set().
 * It is called via the EOAPI macro.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[in] r The red component of the color (0-255).
 * @param[in] g The green component of the color (0-255).
 * @param[in] b The blue component of the color (0-255).
 * @param[in] a The alpha component of the color (0-255).
 */
void _elm_color_item_color_set(Eo *obj, Elm_Color_Item_Data *pd, int r, int g, int b, int a);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_color_item_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

/**
 * @internal
 * @brief Gets the color of the color item.
 *
 * This function is the internal implementation for elm_obj_color_item_color_get().
 * It is called via the EOAPI macro.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[out] r Pointer to store the red component of the color.
 * @param[out] g Pointer to store the green component of the color.
 * @param[out] b Pointer to store the blue component of the color.
 * @param[out] a Pointer to store the alpha component of the color.
 */
void _elm_color_item_color_get(const Eo *obj, Elm_Color_Item_Data *pd, int *r, int *g, int *b, int *a);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_color_item_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

/**
 * @internal
 * @brief Sets the selected state of the color item.
 *
 * This function is the internal implementation for elm_obj_color_item_selected_set().
 * It is called via the EOAPI macro.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[in] selected The selected state (#EINA_TRUE if selected, #EINA_FALSE otherwise).
 */
void _elm_color_item_selected_set(Eo *obj, Elm_Color_Item_Data *pd, Eina_Bool selected);
/**
 * @internal
 * @brief Eolian reflection function for setting the 'selected' property.
 *
 * This function is invoked by the Eolian property system when the 'selected'
 * property of an Elm_Color_Item object is set. It converts the Eina_Value
 * to a boolean and calls the concrete implementation elm_obj_color_item_selected_set().
 *
 * @param[in] obj The Eo object.
 * @param[in] val The Eina_Value containing the boolean value to set.
 * @return #EINA_ERROR_NO_ERROR on success, or an error code if the value conversion fails.
 */
static Eina_Error
__eolian_elm_color_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_color_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_color_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Gets the selected state of the color item.
 *
 * This function is the internal implementation for elm_obj_color_item_selected_get().
 * It is called via the EOAPI macro.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return #EINA_TRUE if the item is selected, #EINA_FALSE otherwise.
 */
Eina_Bool _elm_color_item_selected_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'selected' property.
 *
 * This function is invoked by the Eolian property system when the 'selected'
 * property of an Elm_Color_Item object is read. It calls the concrete
 * implementation elm_obj_color_item_selected_get() and wraps the boolean result
 * in an Eina_Value.
 *
 * @param[in] obj The Eo object.
 * @return An Eina_Value containing the boolean 'selected' state.
 */
static Eina_Value
__eolian_elm_color_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_color_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_color_item_selected_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Constructor for Elm_Color_Item.
 *
 * This function is called when an Elm_Color_Item object is constructed.
 * It initializes the private data and any other necessary resources.
 *
 * @param[in] obj The Eo object being constructed.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_color_item_efl_object_constructor(Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Destructor for Elm_Color_Item.
 *
 * This function is called when an Elm_Color_Item object is destroyed.
 * It cleans up resources allocated by the constructor or during the object's lifetime.
 *
 * @param[in] obj The Eo object being destructed.
 * @param[in] pd The private data structure for Elm_Color_Item.
 */
void _elm_color_item_efl_object_destructor(Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the elm_wdg_item_access_register API.
 *
 * Registers the access object for the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The Efl_Canvas_Object used for accessibility.
 */
Efl_Canvas_Object *_elm_color_item_elm_widget_item_access_register(Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the elm_wdg_item_signal_emit API.
 *
 * Emits a signal from the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[in] emission The signal name.
 * @param[in] source The source of the signal.
 */
void _elm_color_item_elm_widget_item_signal_emit(Eo *obj, Elm_Color_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Implements the elm_wdg_item_focus_set API.
 *
 * Sets the focus state of the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[in] focused #EINA_TRUE if the item should be focused, #EINA_FALSE otherwise.
 */
void _elm_color_item_elm_widget_item_item_focus_set(Eo *obj, Elm_Color_Item_Data *pd, Eina_Bool focused);

/**
 * @internal
 * @brief Implements the elm_wdg_item_focus_get API.
 *
 * Gets the focus state of the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return #EINA_TRUE if the item is focused, #EINA_FALSE otherwise.
 */
Eina_Bool _elm_color_item_elm_widget_item_item_focus_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_ui_focus_object_focus_set API.
 *
 * Sets the focus state of the color item, part of the Efl.Ui.Focus.Object interface.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @param[in] focus #EINA_TRUE if the item should be focused, #EINA_FALSE otherwise.
 */
void _elm_color_item_efl_ui_focus_object_focus_set(Eo *obj, Elm_Color_Item_Data *pd, Eina_Bool focus);

/**
 * @internal
 * @brief Implements the efl_ui_focus_object_focus_geometry_get API.
 *
 * Gets the focus geometry of the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The Eina_Rect representing the focus geometry.
 */
Eina_Rect _elm_color_item_efl_ui_focus_object_focus_geometry_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_ui_focus_object_focus_parent_get API.
 *
 * Gets the focus parent of the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The parent Efl_Ui_Focus_Object.
 */
Efl_Ui_Focus_Object *_elm_color_item_efl_ui_focus_object_focus_parent_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_ui_focus_object_focus_manager_get API.
 *
 * Gets the focus manager for the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The Efl_Ui_Focus_Manager.
 */
Efl_Ui_Focus_Manager *_elm_color_item_efl_ui_focus_object_focus_manager_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_access_object_state_set_get API.
 *
 * Gets the accessibility state set for the color item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The Efl_Access_State_Set representing the accessibility states.
 */
Efl_Access_State_Set _elm_color_item_efl_access_object_state_set_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_access_object_i18n_name_get API.
 *
 * Gets the internationalized name for accessibility.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return The internationalized name string.
 */
const char *_elm_color_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_Color_Item_Data *pd);

/**
 * @internal
 * @brief Implements the efl_access_widget_action_elm_actions_get API.
 *
 * Gets the Elementary widget actions for accessibility.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data structure for Elm_Color_Item.
 * @return Pointer to Efl_Access_Action_Data.
 */
const Efl_Access_Action_Data *_elm_color_item_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Color_Item_Data *pd);


/**
 * @internal
 * @brief Initializes the Elm_Color_Item Efl_Class.
 *
 * This function is called once when the Efl_Class for Elm_Color_Item is being
 * created. It sets up the Evas Object operations (ops) and property reflection
 * operations for this class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_color_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_COLOR_ITEM_EXTRA_OPS
#define ELM_COLOR_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_color_item_color_set, _elm_color_item_color_set),
      EFL_OBJECT_OP_FUNC(elm_obj_color_item_color_get, _elm_color_item_color_get),
      EFL_OBJECT_OP_FUNC(elm_obj_color_item_selected_set, _elm_color_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_color_item_selected_get, _elm_color_item_selected_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_color_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_color_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_access_register, _elm_color_item_elm_widget_item_access_register),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_color_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_set, _elm_color_item_elm_widget_item_item_focus_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_focus_get, _elm_color_item_elm_widget_item_item_focus_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_set, _elm_color_item_efl_ui_focus_object_focus_set),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_geometry_get, _elm_color_item_efl_ui_focus_object_focus_geometry_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_parent_get, _elm_color_item_efl_ui_focus_object_focus_parent_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_manager_get, _elm_color_item_efl_ui_focus_object_focus_manager_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_color_item_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_color_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_color_item_efl_access_widget_action_elm_actions_get),
      ELM_COLOR_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"selected", __eolian_elm_color_item_selected_set_reflect, __eolian_elm_color_item_selected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Efl_Class description for Elm_Color_Item.
 *
 * This structure provides metadata for the Elm_Color_Item class,
 * including its version, name, type, data size, and initializer/constructor
 * functions.
 */
static const Efl_Class_Description _elm_color_item_class_desc = {
   EO_VERSION,
   "Elm.Color.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Color_Item_Data),
   _elm_color_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_color_item_class_get, &_elm_color_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_FOCUS_OBJECT_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, NULL);

#include "elm_color_item_eo.legacy.c"
