/** @brief Event emitted when a color item is selected. */
EWAPI const Efl_Event_Description _ELM_COLORSELECTOR_EVENT_COLOR_ITEM_SELECTED =
   EFL_EVENT_DESCRIPTION("color,item,selected");
/** @brief Event emitted when a color item is long-pressed. */
EWAPI const Efl_Event_Description _ELM_COLORSELECTOR_EVENT_COLOR_ITEM_LONGPRESSED =
   EFL_EVENT_DESCRIPTION("color,item,longpressed");
/** @brief Event emitted when the selected color has changed. */
EWAPI const Efl_Event_Description _ELM_COLORSELECTOR_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");
/** @brief Event emitted when the selected color has changed due to user interaction. */
EWAPI const Efl_Event_Description _ELM_COLORSELECTOR_EVENT_CHANGED_USER =
   EFL_EVENT_DESCRIPTION("changed,user");

/**
 * @internal
 * @brief Implements elm_obj_colorselector_picked_color_set.
 * Sets the currently picked color in the color selector.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 * @param a The alpha component (0-255).
 */
void _elm_colorselector_picked_color_set(Eo *obj, Elm_Colorselector_Data *pd, int r, int g, int b, int a);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_colorselector_picked_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_picked_color_get.
 * Retrieves the currently picked color from the color selector.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param r Pointer to store the red component.
 * @param g Pointer to store the green component.
 * @param b Pointer to store the blue component.
 * @param a Pointer to store the alpha component.
 */
void _elm_colorselector_picked_color_get(const Eo *obj, Elm_Colorselector_Data *pd, int *r, int *g, int *b, int *a);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_colorselector_picked_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_name_set.
 * Sets the name of the current palette.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param palette_name The name to set for the palette.
 */
void _elm_colorselector_palette_name_set(Eo *obj, Elm_Colorselector_Data *pd, const char *palette_name);

/**
 * @internal
 * @brief Eolian reflection function for setting the 'palette_name' property.
 * This function is called by Eolian to set the palette name property from a generic Eina_Value.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the string for the palette name.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_colorselector_palette_name_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_colorselector_palette_name_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_colorselector_palette_name_set, EFL_FUNC_CALL(palette_name), const char *palette_name);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_name_get.
 * Gets the name of the current palette.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The name of the current palette.
 */
const char *_elm_colorselector_palette_name_get(const Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'palette_name' property.
 * This function is called by Eolian to get the palette name property as a generic Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the string of the palette name.
 */
static Eina_Value
__eolian_elm_colorselector_palette_name_get_reflect(const Eo *obj)
{
   const char *val = elm_obj_colorselector_palette_name_get(obj);
   return eina_value_string_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_colorselector_palette_name_get, const char *, NULL);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_mode_set.
 * Sets the display mode of the color selector.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param mode The Elm_Colorselector_Mode to set.
 */
void _elm_colorselector_mode_set(Eo *obj, Elm_Colorselector_Data *pd, Elm_Colorselector_Mode mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_colorselector_mode_set, EFL_FUNC_CALL(mode), Elm_Colorselector_Mode mode);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_mode_get.
 * Gets the display mode of the color selector.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The current Elm_Colorselector_Mode.
 */
Elm_Colorselector_Mode _elm_colorselector_mode_get(const Eo *obj, Elm_Colorselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_colorselector_mode_get, Elm_Colorselector_Mode, 0);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_items_get.
 * Retrieves the list of color items in the palette.
 * The list contains Elm_Widget_Item objects, each representing a color.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return A const Eina_List of Elm_Widget_Item pointers, or NULL if none.
 *         This list should not be modified or freed by the caller.
 */
const Eina_List *_elm_colorselector_palette_items_get(const Eo *obj, Elm_Colorselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_colorselector_palette_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_selected_item_get.
 * Gets the currently selected item in the color palette.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The selected Elm_Widget_Item, or NULL if no item is selected.
 */
Elm_Widget_Item *_elm_colorselector_palette_selected_item_get(const Eo *obj, Elm_Colorselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_colorselector_palette_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_color_add.
 * Adds a new color to the palette.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 * @param a The alpha component (0-255).
 * @return The newly created Elm_Widget_Item for the added color, or NULL on failure.
 */
Elm_Widget_Item *_elm_colorselector_palette_color_add(Eo *obj, Elm_Colorselector_Data *pd, int r, int g, int b, int a);

EOAPI EFL_FUNC_BODYV(elm_obj_colorselector_palette_color_add, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

/**
 * @internal
 * @brief Implements elm_obj_colorselector_palette_clear.
 * Clears all colors from the palette.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 */
void _elm_colorselector_palette_clear(Eo *obj, Elm_Colorselector_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_colorselector_palette_clear);

/**
 * @internal
 * @brief Implements efl_constructor.
 * Constructor for the Elm_Colorselector object.
 * @param obj The Evas object being constructed.
 * @param pd The private data of the object.
 * @return The constructed Efl_Object.
 */
Efl_Object *_elm_colorselector_efl_object_constructor(Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Implements efl_ui_widget_theme_apply.
 * Applies the theme to the color selector widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code.
 */
Eina_Error _elm_colorselector_efl_ui_widget_theme_apply(Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Implements efl_ui_widget_focus_highlight_geometry_get.
 * Gets the geometry for the focus highlight.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The Eina_Rect defining the focus highlight geometry.
 */
Eina_Rect _elm_colorselector_efl_ui_widget_focus_highlight_geometry_get(const Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Implements efl_ui_widget_on_access_update.
 * Called when accessibility features are updated.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param enable EINA_TRUE if accessibility is enabled, EINA_FALSE otherwise.
 */
void _elm_colorselector_efl_ui_widget_on_access_update(Eo *obj, Elm_Colorselector_Data *pd, Eina_Bool enable);

/**
 * @internal
 * @brief Implements efl_ui_widget_input_event_handler.
 * Handles input events for the widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param eo_event The Efl_Event details.
 * @param source The source canvas object of the event.
 * @return EINA_TRUE if the event was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_colorselector_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Colorselector_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements efl_access_widget_action_elm_actions_get.
 * Retrieves the list of Elementary actions for accessibility.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return A const Efl_Access_Action_Data array, or NULL.
 */
const Efl_Access_Action_Data *_elm_colorselector_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Implements efl_access_object_access_children_get.
 * Retrieves the accessibility children of the object.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return An Eina_List of Efl_Access_Object pointers.
 *         The list contains accessible child objects.
 *         Example: For a color palette, children could be individual color items.
 */
Eina_List *_elm_colorselector_efl_access_object_access_children_get(const Eo *obj, Elm_Colorselector_Data *pd);

/**
 * @internal
 * @brief Class initializer for Elm_Colorselector.
 * This function sets up the Efl_Object operations for the Elm_Colorselector class.
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_colorselector_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_COLORSELECTOR_EXTRA_OPS
#define ELM_COLORSELECTOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_picked_color_set, _elm_colorselector_picked_color_set),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_picked_color_get, _elm_colorselector_picked_color_get),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_name_set, _elm_colorselector_palette_name_set),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_name_get, _elm_colorselector_palette_name_get),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_mode_set, _elm_colorselector_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_mode_get, _elm_colorselector_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_items_get, _elm_colorselector_palette_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_selected_item_get, _elm_colorselector_palette_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_color_add, _elm_colorselector_palette_color_add),
      EFL_OBJECT_OP_FUNC(elm_obj_colorselector_palette_clear, _elm_colorselector_palette_clear),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_colorselector_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_colorselector_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_focus_highlight_geometry_get, _elm_colorselector_efl_ui_widget_focus_highlight_geometry_get),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_colorselector_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_colorselector_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_colorselector_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_colorselector_efl_access_object_access_children_get),
      ELM_COLORSELECTOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"palette_name", __eolian_elm_colorselector_palette_name_set_reflect, __eolian_elm_colorselector_palette_name_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_Colorselector class.
 * This structure provides metadata for the Elm_Colorselector class,
 * including its version, name, type, instance size, and constructor/initializer functions.
 */
static const Efl_Class_Description _elm_colorselector_class_desc = {
   EO_VERSION,
   "Elm.Colorselector",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Colorselector_Data),
   _elm_colorselector_class_initializer,
   _elm_colorselector_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_colorselector_class_get, &_elm_colorselector_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_colorselector_eo.legacy.c"
