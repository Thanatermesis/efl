/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_icon_name_set.
 *
 * Sets the icon for the menu item using a name.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param icon The name of the icon to set.
 */
void _elm_menu_item_icon_name_set(Eo *obj, Elm_Menu_Item_Data *pd, const char *icon);

/**
 * @internal
 * @brief Reflection function to set the "icon_name" property.
 *
 * Converts an Eina_Value (expected to be a string) and calls
 * @ref elm_obj_menu_item_icon_name_set.
 *
 * @param obj The Eo object.
 * @param val Eina_Value containing the icon name.
 * @return EINA_ERROR_NO_ERROR on success, EINA_ERROR_VALUE_FAILED on type mismatch.
 */
static Eina_Error
__eolian_elm_menu_item_icon_name_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_menu_item_icon_name_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_menu_item_icon_name_set, EFL_FUNC_CALL(icon), const char *icon);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_icon_name_get.
 *
 * Retrieves the name of the icon set for the menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return The name of the icon, or @c NULL if not set or on error.
 */
const char *_elm_menu_item_icon_name_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the "icon_name" property.
 *
 * Calls @ref elm_obj_menu_item_icon_name_get and wraps the result in an Eina_Value.
 *
 * @param obj The Eo object.
 * @return Eina_Value containing the icon name string.
 */
static Eina_Value
__eolian_elm_menu_item_icon_name_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_menu_item_icon_name_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_icon_name_get, const char *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_prev_get.
 *
 * Gets the menu item preceding this one.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return The previous Elm_Widget_Item, or @c NULL if it's the first or on error.
 */
Elm_Widget_Item *_elm_menu_item_prev_get(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_prev_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_next_get.
 *
 * Gets the menu item following this one.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return The next Elm_Widget_Item, or @c NULL if it's the last or on error.
 */
Elm_Widget_Item *_elm_menu_item_next_get(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_next_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_selected_set.
 *
 * Sets the selected state of the menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @param selected @c EINA_TRUE if selected, @c EINA_FALSE otherwise.
 */
void _elm_menu_item_selected_set(Eo *obj, Elm_Menu_Item_Data *pd, Eina_Bool selected);

/**
 * @internal
 * @brief Reflection function to set the "selected" property.
 *
 * Converts an Eina_Value (expected to be a boolean) and calls
 * @ref elm_obj_menu_item_selected_set.
 *
 * @param obj The Eo object.
 * @param val Eina_Value containing the boolean selection state.
 * @return EINA_ERROR_NO_ERROR on success, EINA_ERROR_VALUE_FAILED on type mismatch.
 */
static Eina_Error
__eolian_elm_menu_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_menu_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_menu_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_selected_get.
 *
 * Retrieves the selected state of the menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return @c EINA_TRUE if selected, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_menu_item_selected_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the "selected" property.
 *
 * Calls @ref elm_obj_menu_item_selected_get and wraps the result in an Eina_Value.
 *
 * @param obj The Eo object.
 * @return Eina_Value containing the boolean selection state.
 */
static Eina_Value
__eolian_elm_menu_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_menu_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_selected_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_index_get.
 *
 * Retrieves the numerical index of the menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return The index of the item.
 */
unsigned int _elm_menu_item_index_get(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_index_get, unsigned int, 0);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_subitems_clear.
 *
 * Removes all sub-items of this menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 */
void _elm_menu_item_subitems_clear(Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_menu_item_subitems_clear);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_subitems_get.
 *
 * Retrieves a list of all sub-items of this menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return A const Eina_List of sub-items, or @c NULL if none.
 */
const Eina_List *_elm_menu_item_subitems_get(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_subitems_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_is_separator.
 *
 * Checks if the menu item is a separator.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return @c EINA_TRUE if it is a separator, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_menu_item_is_separator(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_is_separator, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for @ref elm_obj_menu_item_object_get.
 *
 * Retrieves the Evas object associated with this menu item.
 *
 * @param obj The Eo object.
 * @param pd The private data of the object.
 * @return The Efl_Canvas_Object for this item, or @c NULL on error.
 */
Efl_Canvas_Object *_elm_menu_item_object_get(const Eo *obj, Elm_Menu_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_menu_item_object_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm.Menu.Item.
 *
 * This function is called when a new Elm_Menu_Item instance is created.
 *
 * @param obj The Eo object to construct.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The constructed Efl_Object, or @c NULL on failure.
 */
Efl_Object *_elm_menu_item_efl_object_constructor(Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Object.destructor for Elm.Menu.Item.
 *
 * This function is called when an Elm_Menu_Item instance is being destroyed.
 *
 * @param obj The Eo object to destruct.
 * @param pd Private data for the Elm_Menu_Item instance.
 */
void _elm_menu_item_efl_object_destructor(Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Object.parent_get for Elm.Menu.Item.
 *
 * Retrieves the parent object of this menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The parent Efl_Object.
 */
Efl_Object *_elm_menu_item_efl_object_parent_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.disable for Elm.Menu.Item.
 *
 * Disables the menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 */
void _elm_menu_item_elm_widget_item_disable(Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.signal_emit for Elm.Menu.Item.
 *
 * Emits a signal from this menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param emission The signal name.
 * @param source The signal source.
 */
void _elm_menu_item_elm_widget_item_signal_emit(Eo *obj, Elm_Menu_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.part_text_set for Elm.Menu.Item.
 *
 * Sets the text for a specific part of the menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param part The name of the part to set text for.
 * @param label The text label to set.
 */
void _elm_menu_item_elm_widget_item_part_text_set(Eo *obj, Elm_Menu_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.part_text_get for Elm.Menu.Item.
 *
 * Gets the text from a specific part of the menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param part The name of the part to get text from.
 * @return The text label of the part, or @c NULL.
 */
const char *_elm_menu_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Menu_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.part_content_set for Elm.Menu.Item.
 *
 * Sets the content for a specific part of the menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param part The name of the part to set content for.
 * @param content The Efl_Canvas_Object content to set.
 */
void _elm_menu_item_elm_widget_item_part_content_set(Eo *obj, Elm_Menu_Item_Data *pd, const char *part, Efl_Canvas_Object *content);

/**
 * @internal
 * @brief Implements the Elm.Widget.Item.part_content_get for Elm.Menu.Item.
 *
 * Gets the content from a specific part of the menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param part The name of the part to get content from.
 * @return The Efl_Canvas_Object content of the part, or @c NULL.
 */
Efl_Canvas_Object *_elm_menu_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Menu_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements the Efl.Access.Object.access_children_get for Elm.Menu.Item.
 *
 * Retrieves the accessible children of this menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return A list of accessible child objects.
 */
Eina_List *_elm_menu_item_efl_access_object_access_children_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Object.role_get for Elm.Menu.Item.
 *
 * Retrieves the accessibility role of this menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The Efl_Access_Role of the item.
 */
Efl_Access_Role _elm_menu_item_efl_access_object_role_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Object.i18n_name_get for Elm.Menu.Item.
 *
 * Retrieves the internationalized name for accessibility.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The i18n name string.
 */
const char *_elm_menu_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Object.state_set_get for Elm.Menu.Item.
 *
 * Retrieves the set of accessibility states for this menu item.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The Efl_Access_State_Set.
 */
Efl_Access_State_Set _elm_menu_item_efl_access_object_state_set_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Access.Selection.selected_children_count_get for Elm.Menu.Item.
 *
 * Gets the number of selected sub-items.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @return The count of selected children.
 */
int _elm_menu_item_efl_access_selection_selected_children_count_get(const Eo *obj, Elm_Menu_Item_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Access.Selection.selected_child_get for Elm.Menu.Item.
 *
 * Gets a specific selected sub-item by index.
 *
 * @param obj The Eo object.
 * @param pd Private data for the Elm_Menu_Item instance.
 * @param selected_child_index The index of the selected child.
 * @return The selected child Efl_Object, or @c NULL.
 */
Efl_Object *_elm_menu_item_efl_access_selection_selected_child_get(const Eo *obj, Elm_Menu_Item_Data *pd, int selected_child_index);

/**
 * @internal
 * @brief Initializes the Elm_Menu_Item Efl_Class.
 *
 * This function is called by the EO system to set up the class,
 * including its operations (methods) and property reflection capabilities.
 *
 * @param klass The Efl_Class to initialize for Elm_Menu_Item.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_menu_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MENU_ITEM_EXTRA_OPS
#define ELM_MENU_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_icon_name_set, _elm_menu_item_icon_name_set),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_icon_name_get, _elm_menu_item_icon_name_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_prev_get, _elm_menu_item_prev_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_next_get, _elm_menu_item_next_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_selected_set, _elm_menu_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_selected_get, _elm_menu_item_selected_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_index_get, _elm_menu_item_index_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_subitems_clear, _elm_menu_item_subitems_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_subitems_get, _elm_menu_item_subitems_get),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_is_separator, _elm_menu_item_is_separator),
      EFL_OBJECT_OP_FUNC(elm_obj_menu_item_object_get, _elm_menu_item_object_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_menu_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_menu_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_parent_get, _elm_menu_item_efl_object_parent_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_disable, _elm_menu_item_elm_widget_item_disable),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_menu_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_menu_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_menu_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_menu_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_menu_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_menu_item_efl_access_object_access_children_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_role_get, _elm_menu_item_efl_access_object_role_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_menu_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_menu_item_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_children_count_get, _elm_menu_item_efl_access_selection_selected_children_count_get),
      EFL_OBJECT_OP_FUNC(efl_access_selection_selected_child_get, _elm_menu_item_efl_access_selection_selected_child_get),
      ELM_MENU_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"icon_name", __eolian_elm_menu_item_icon_name_set_reflect, __eolian_elm_menu_item_icon_name_get_reflect},
      {"selected", __eolian_elm_menu_item_selected_set_reflect, __eolian_elm_menu_item_selected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Menu_Item Efl_Class structure.
 *
 * This static constant structure provides metadata for the Elm_Menu_Item class,
 * such as its version, name, type, instance data size, and initializer functions.
 * It is used by the EO system to define and manage the class.
 */
static const Efl_Class_Description _elm_menu_item_class_desc = {
   EO_VERSION,
   "Elm.Menu.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Menu_Item_Data),
   _elm_menu_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_menu_item_class_get, &_elm_menu_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_ACCESS_SELECTION_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_menu_item_eo.legacy.c"
