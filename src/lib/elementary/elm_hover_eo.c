EWAPI const Efl_Event_Description _ELM_HOVER_EVENT_SMART_CHANGED =
   EFL_EVENT_DESCRIPTION("smart,changed");
EWAPI const Efl_Event_Description _ELM_HOVER_EVENT_DISMISSED =
   EFL_EVENT_DESCRIPTION("dismissed");

/**
 * @internal
 * @brief Internal implementation for elm_obj_hover_target_set().
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] target The target object.
 */
void _elm_hover_target_set(Eo *obj, Elm_Hover_Data *pd, Efl_Canvas_Object *target);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_hover_target_set, EFL_FUNC_CALL(target), Efl_Canvas_Object *target);

/**
 * @internal
 * @brief Internal implementation for elm_obj_hover_target_get().
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @return The target object, or @c NULL if none is set.
 */
Efl_Canvas_Object *_elm_hover_target_get(const Eo *obj, Elm_Hover_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_hover_target_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_hover_best_content_location_get().
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] pref_axis The preferred orientation axis.
 * @return The recommended swallow location (e.g., "top", "bottom", "left", "right").
 */
const char *_elm_hover_best_content_location_get(const Eo *obj, Elm_Hover_Data *pd, Elm_Hover_Axis pref_axis);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_hover_best_content_location_get, const char *, NULL, EFL_FUNC_CALL(pref_axis), Elm_Hover_Axis pref_axis);

/**
 * @internal
 * @brief Internal implementation for elm_obj_hover_dismiss().
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 */
void _elm_hover_dismiss(Eo *obj, Elm_Hover_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_hover_dismiss);

/**
 * @internal
 * @brief EFL object constructor for Elm_Hover.
 *
 * This function is called when a new Elm_Hover object is created.
 * It initializes the object and its private data.
 *
 * @param[in] obj The Efl object being constructed.
 * @param[in] pd The private data structure to initialize.
 * @return The initialized Efl object (usually @p obj itself), or @c NULL on failure.
 */
Efl_Object *_elm_hover_efl_object_constructor(Eo *obj, Elm_Hover_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Gfx.Entity.visible_set() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] v @c EINA_TRUE if visible, @c EINA_FALSE otherwise.
 */
void _elm_hover_efl_gfx_entity_visible_set(Eo *obj, Elm_Hover_Data *pd, Eina_Bool v);

/**
 * @internal
 * @brief Implements Efl.Gfx.Entity.position_set() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] pos The new position (x, y) of the object.
 */
void _elm_hover_efl_gfx_entity_position_set(Eo *obj, Elm_Hover_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements Efl.Gfx.Entity.size_set() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] size The new size (width, height) of the object.
 */
void _elm_hover_efl_gfx_entity_size_set(Eo *obj, Elm_Hover_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements Efl.Ui.Widget.theme_apply() for Elm_Hover.
 *
 * This function is called to apply the theme to the widget.
 *
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @return EINA_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_hover_efl_ui_widget_theme_apply(Eo *obj, Elm_Hover_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Ui.Widget.sub_object_add() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] sub_obj The sub_object to add.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_hover_efl_ui_widget_widget_sub_object_add(Eo *obj, Elm_Hover_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements Efl.Ui.Widget.sub_object_del() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] sub_obj The sub_object to delete.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_hover_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Hover_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements Efl.Access.Widget.Action.elm_actions_get() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @return A pointer to an array of Efl_Access_Action_Data, or @c NULL.
 */
const Efl_Access_Action_Data *_elm_hover_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Hover_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Access.Object.state_set_get() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @return The current accessibility state set.
 */
Efl_Access_State_Set _elm_hover_efl_access_object_state_set_get(const Eo *obj, Elm_Hover_Data *pd);

/**
 * @internal
 * @brief Implements Efl.Part.part_get() for Elm_Hover.
 * @param[in] obj The object.
 * @param[in] pd The private data structure.
 * @param[in] name The name of the part to get.
 * @return The Efl_Object representing the part, or @c NULL if not found.
 */
Efl_Object *_elm_hover_efl_part_part_get(const Eo *obj, Elm_Hover_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Hover Efl class.
 *
 * This function is called once when the Efl class is being set up.
 * It defines the operations (methods) for the Elm_Hover class.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_hover_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_HOVER_EXTRA_OPS
#define ELM_HOVER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_hover_target_set, _elm_hover_target_set),
      EFL_OBJECT_OP_FUNC(elm_obj_hover_target_get, _elm_hover_target_get),
      EFL_OBJECT_OP_FUNC(elm_obj_hover_best_content_location_get, _elm_hover_best_content_location_get),
      EFL_OBJECT_OP_FUNC(elm_obj_hover_dismiss, _elm_hover_dismiss),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_hover_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_hover_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_hover_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_hover_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_hover_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_add, _elm_hover_efl_ui_widget_widget_sub_object_add),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_hover_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_hover_efl_access_widget_action_elm_actions_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_state_set_get, _elm_hover_efl_access_object_state_set_get),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_hover_efl_part_part_get),
      ELM_HOVER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_hover_class_desc = {
   EO_VERSION,
   "Elm.Hover",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Hover_Data),
   _elm_hover_class_initializer,
   _elm_hover_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_hover_class_get, &_elm_hover_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_LAYER_MIXIN, EFL_INPUT_CLICKABLE_MIXIN, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_hover_eo.legacy.c"
