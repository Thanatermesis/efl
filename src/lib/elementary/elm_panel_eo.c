EWAPI const Efl_Event_Description _ELM_PANEL_EVENT_TOGGLED =
   EFL_EVENT_DESCRIPTION("toggled");

/**
 * @internal
 * @brief Internal function to set the orientation of the panel.
 * Implements the Efl.Ui.Layout_Orientable.orientation_set method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param orient The panel orientation.
 */
void _elm_panel_orient_set(Eo *obj, Elm_Panel_Data *pd, Elm_Panel_Orient orient);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_panel_orient_set, EFL_FUNC_CALL(orient), Elm_Panel_Orient orient);

/**
 * @internal
 * @brief Internal function to get the orientation of the panel.
 * Implements the Efl.Ui.Layout_Orientable.orientation_get method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The panel orientation.
 */
Elm_Panel_Orient _elm_panel_orient_get(const Eo *obj, Elm_Panel_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_panel_orient_get, Elm_Panel_Orient, 2 /* Elm.Panel.Orient.left */);

/**
 * @internal
 * @brief Internal function to set the hidden state of the panel.
 * Implements the Elm.Panel.hidden_set method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param hidden If @c EINA_TRUE, the panel will be hidden.
 */
void _elm_panel_hidden_set(Eo *obj, Elm_Panel_Data *pd, Eina_Bool hidden);

/**
 * @internal
 * @brief Reflection function for the 'hidden' property set.
 * This function is called when the 'hidden' property is set via reflection.
 * It converts the Eina_Value to a boolean and calls the actual setter.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_panel_hidden_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_panel_hidden_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_panel_hidden_set, EFL_FUNC_CALL(hidden), Eina_Bool hidden);

/**
 * @internal
 * @brief Internal function to get the hidden state of the panel.
 * Implements the Elm.Panel.hidden_get method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return @c EINA_TRUE if the panel is hidden, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_panel_hidden_get(const Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Reflection function for the 'hidden' property get.
 * This function is called when the 'hidden' property is read via reflection.
 * It calls the actual getter and wraps the boolean result in an Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state.
 */
static Eina_Value
__eolian_elm_panel_hidden_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_panel_hidden_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_panel_hidden_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal function to set the scrollability of the panel.
 * Implements the Elm.Panel.scrollable_set method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param scrollable The scrollable state.
 */
void _elm_panel_scrollable_set(Eo *obj, Elm_Panel_Data *pd, Eina_Bool scrollable);

/**
 * @internal
 * @brief Reflection function for the 'scrollable' property set.
 * This function is called when the 'scrollable' property is set via reflection.
 * It converts the Eina_Value to a boolean and calls the actual setter.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the boolean state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_panel_scrollable_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_panel_scrollable_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_panel_scrollable_set, EFL_FUNC_CALL(scrollable), Eina_Bool scrollable);

/**
 * @internal
 * @brief Internal function to get the scrollability state of the panel.
 * Implements the Elm.Panel.scrollable_get method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The scrollable state.
 */
Eina_Bool _elm_panel_scrollable_get(const Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Reflection function for the 'scrollable' property get.
 * This function is called when the 'scrollable' property is read via reflection.
 * It calls the actual getter and wraps the boolean result in an Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the boolean state.
 */
static Eina_Value
__eolian_elm_panel_scrollable_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_panel_scrollable_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_panel_scrollable_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal function to set the size of the scrollable content area of the panel.
 * Implements the Elm.Panel.scrollable_content_size_set method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param ratio The size ratio (e.g., 1.0 for full size).
 */
void _elm_panel_scrollable_content_size_set(Eo *obj, Elm_Panel_Data *pd, double ratio);

/**
 * @internal
 * @brief Reflection function for the 'scrollable_content_size' property set.
 * This function is called when the 'scrollable_content_size' property is set via reflection.
 * It converts the Eina_Value to a double and calls the actual setter.
 * @param obj The Evas object.
 * @param val The Eina_Value containing the double value for the ratio.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_panel_scrollable_content_size_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_panel_scrollable_content_size_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_panel_scrollable_content_size_set, EFL_FUNC_CALL(ratio), double ratio);

/**
 * @internal
 * @brief Internal function to get the size of the scrollable content area of the panel.
 * Implements the Elm.Panel.scrollable_content_size_get method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The size ratio.
 */
double _elm_panel_scrollable_content_size_get(const Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Reflection function for the 'scrollable_content_size' property get.
 * This function is called when the 'scrollable_content_size' property is read via reflection.
 * It calls the actual getter and wraps the double result in an Eina_Value.
 * @param obj The Evas object.
 * @return An Eina_Value containing the double value of the ratio.
 */
static Eina_Value
__eolian_elm_panel_scrollable_content_size_get_reflect(const Eo *obj)
{
   double val = elm_obj_panel_scrollable_content_size_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_panel_scrollable_content_size_get, double, 0);

/**
 * @internal
 * @brief Internal function to toggle the hidden state of the panel.
 * Implements the Elm.Panel.toggle method.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 */
void _elm_panel_toggle(Eo *obj, Elm_Panel_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_panel_toggle);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor method.
 * This function is called when a new Elm_Panel object is constructed.
 * @param obj The Evas object being constructed.
 * @param pd The private data of the object.
 * @return The constructed Evas object.
 */
Efl_Object *_elm_panel_efl_object_constructor(Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Gfx.Entity.position_set method.
 * Sets the position of the panel.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param pos The new position (Eina_Position2D).
 */
void _elm_panel_efl_gfx_entity_position_set(Eo *obj, Elm_Panel_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements the Efl.Gfx.Entity.size_set method.
 * Sets the size of the panel.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param size The new size (Eina_Size2D).
 */
void _elm_panel_efl_gfx_entity_size_set(Eo *obj, Elm_Panel_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl.Canvas.Group.member_add method.
 * Adds a sub-object to the panel's canvas group.
 * @param obj The Evas object (panel).
 * @param pd The private data of the object.
 * @param sub_obj The sub-object to add.
 */
void _elm_panel_efl_canvas_group_group_member_add(Eo *obj, Elm_Panel_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply method.
 * Applies the theme to the panel widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_panel_efl_ui_widget_theme_apply(Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.disabled_set method.
 * Sets the disabled state of the panel widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param disabled @c EINA_TRUE to disable, @c EINA_FALSE to enable.
 */
void _elm_panel_efl_ui_widget_disabled_set(Eo *obj, Elm_Panel_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.on_access_update method.
 * Handles updates related to accessibility.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param enable @c EINA_TRUE if accessibility is enabled.
 */
void _elm_panel_efl_ui_widget_on_access_update(Eo *obj, Elm_Panel_Data *pd, Eina_Bool enable);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.widget_input_event_handler method.
 * Handles input events for the panel widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param eo_event The Efl event.
 * @param source The source canvas object of the event.
 * @return @c EINA_TRUE if the event was handled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_panel_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Panel_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.interest_region_get method.
 * Gets the interest region of the panel widget.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The interest region as an Eina_Rect.
 */
Eina_Rect _elm_panel_efl_ui_widget_interest_region_get(const Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Access.Widget.Action.elm_actions_get method.
 * Gets the accessibility actions for the panel.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return A pointer to an array of Efl_Access_Action_Data, or @c NULL.
 *         Example: `return &((Efl_Access_Action_Data[]){{"toggle", "toggle"}, {NULL, NULL}});`
 */
const Efl_Access_Action_Data *_elm_panel_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Panel_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.I18n.mirrored_set method.
 * Sets the mirrored (right-to-left) mode for the panel.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param rtl @c EINA_TRUE for right-to-left, @c EINA_FALSE for left-to-right.
 */
void _elm_panel_efl_ui_i18n_mirrored_set(Eo *obj, Elm_Panel_Data *pd, Eina_Bool rtl);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get method.
 * Gets a part of the panel.
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param name The name of the part to get.
 * @return The Evas object representing the part, or @c NULL if not found.
 */
Efl_Object *_elm_panel_efl_part_part_get(const Eo *obj, Elm_Panel_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Panel class.
 * Sets up the Efl_Object operations and property reflections for the Elm_Panel class.
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_panel_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_PANEL_EXTRA_OPS
#define ELM_PANEL_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_panel_orient_set, _elm_panel_orient_set),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_orient_get, _elm_panel_orient_get),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_hidden_set, _elm_panel_hidden_set),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_hidden_get, _elm_panel_hidden_get),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_scrollable_set, _elm_panel_scrollable_set),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_scrollable_get, _elm_panel_scrollable_get),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_scrollable_content_size_set, _elm_panel_scrollable_content_size_set),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_scrollable_content_size_get, _elm_panel_scrollable_content_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_panel_toggle, _elm_panel_toggle),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_panel_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_panel_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_panel_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_group_member_add, _elm_panel_efl_canvas_group_group_member_add),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_panel_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_disabled_set, _elm_panel_efl_ui_widget_disabled_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_panel_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_panel_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_interest_region_get, _elm_panel_efl_ui_widget_interest_region_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_panel_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_ui_mirrored_set, _elm_panel_efl_ui_i18n_mirrored_set),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_panel_efl_part_part_get),
      ELM_PANEL_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"hidden", __eolian_elm_panel_hidden_set_reflect, __eolian_elm_panel_hidden_get_reflect},
      {"scrollable", __eolian_elm_panel_scrollable_set_reflect, __eolian_elm_panel_scrollable_get_reflect},
      {"scrollable_content_size", __eolian_elm_panel_scrollable_content_size_set_reflect, __eolian_elm_panel_scrollable_content_size_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Panel Efl class.
 * This structure provides metadata for the Elm_Panel class, including its version,
 * name, type, size of instance data, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_panel_class_desc = {
   EO_VERSION, /**< Eolian version for this class. */
   "Elm.Panel", /**< The name of the class. */
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Panel_Data),
   _elm_panel_class_initializer,
   _elm_panel_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_panel_class_get, &_elm_panel_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_LAYER_MIXIN, ELM_INTERFACE_SCROLLABLE_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_panel_eo.legacy.c"
