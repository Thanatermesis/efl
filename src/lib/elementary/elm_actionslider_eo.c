/**
 * @brief Event description for the "pos_changed" event.
 * This event is emitted when the actionslider's position changes.
 */
EWAPI const Efl_Event_Description _ELM_ACTIONSLIDER_EVENT_POS_CHANGED =
   EFL_EVENT_DESCRIPTION("pos_changed");

/**
 * @internal
 * @brief Implements the setter for the indicator position property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param pos The new position for the indicator.
 */
void _elm_actionslider_indicator_pos_set(Eo *obj, Elm_Actionslider_Data *pd, Elm_Actionslider_Pos pos);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_actionslider_indicator_pos_set, EFL_FUNC_CALL(pos), Elm_Actionslider_Pos pos);

/**
 * @internal
 * @brief Implements the getter for the indicator position property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The current position of the indicator.
 */
Elm_Actionslider_Pos _elm_actionslider_indicator_pos_get(const Eo *obj, Elm_Actionslider_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_actionslider_indicator_pos_get, Elm_Actionslider_Pos, 0);

/**
 * @internal
 * @brief Implements the setter for the magnet position property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param pos The bitmask for magnet positions.
 */
void _elm_actionslider_magnet_pos_set(Eo *obj, Elm_Actionslider_Data *pd, Elm_Actionslider_Pos pos);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_actionslider_magnet_pos_set, EFL_FUNC_CALL(pos), Elm_Actionslider_Pos pos);

/**
 * @internal
 * @brief Implements the getter for the magnet position property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The bitmask of current magnet positions.
 */
Elm_Actionslider_Pos _elm_actionslider_magnet_pos_get(const Eo *obj, Elm_Actionslider_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_actionslider_magnet_pos_get, Elm_Actionslider_Pos, 0);

/**
 * @internal
 * @brief Implements the setter for the enabled positions property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param pos The bitmask for enabled positions.
 */
void _elm_actionslider_enabled_pos_set(Eo *obj, Elm_Actionslider_Data *pd, Elm_Actionslider_Pos pos);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_actionslider_enabled_pos_set, EFL_FUNC_CALL(pos), Elm_Actionslider_Pos pos);

/**
 * @internal
 * @brief Implements the getter for the enabled positions property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The bitmask of current enabled positions.
 */
Elm_Actionslider_Pos _elm_actionslider_enabled_pos_get(const Eo *obj, Elm_Actionslider_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_actionslider_enabled_pos_get, Elm_Actionslider_Pos, 0);

/**
 * @internal
 * @brief Implements the getter for the selected label property.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The currently selected label string.
 */
const char *_elm_actionslider_selected_label_get(const Eo *obj, Elm_Actionslider_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_actionslider_selected_label_get, const char *, NULL);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor method.
 *
 * This function is called when a new Elm_Actionslider object is constructed.
 * It initializes the object.
 *
 * @param obj The Evas object being constructed.
 * @param pd The private data of the object.
 * @return The constructed Evas object.
 */
Efl_Object *_elm_actionslider_efl_object_constructor(Eo *obj, Elm_Actionslider_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply method.
 *
 * This function is called to apply the theme to the Elm_Actionslider widget.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return EINA_ERROR_NONE on success, or an error code on failure.
 */
Eina_Error _elm_actionslider_efl_ui_widget_theme_apply(Eo *obj, Elm_Actionslider_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Part.part_get method.
 *
 * This function is called to retrieve a part of the Elm_Actionslider widget.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param name The name of the part to retrieve.
 * @return The Evas object representing the part, or NULL if not found.
 */
Efl_Object *_elm_actionslider_efl_part_part_get(const Eo *obj, Elm_Actionslider_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Actionslider class.
 *
 * This function is called once when the class is being set up.
 * It defines the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_actionslider_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ACTIONSLIDER_EXTRA_OPS
#define ELM_ACTIONSLIDER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_indicator_pos_set, _elm_actionslider_indicator_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_indicator_pos_get, _elm_actionslider_indicator_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_magnet_pos_set, _elm_actionslider_magnet_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_magnet_pos_get, _elm_actionslider_magnet_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_enabled_pos_set, _elm_actionslider_enabled_pos_set),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_enabled_pos_get, _elm_actionslider_enabled_pos_get),
      EFL_OBJECT_OP_FUNC(elm_obj_actionslider_selected_label_get, _elm_actionslider_selected_label_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_actionslider_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_actionslider_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_actionslider_efl_part_part_get),
      ELM_ACTIONSLIDER_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Actionslider Efl_Class.
 *
 * This static structure provides metadata for the Elm_Actionslider class,
 * including its version, name, type, data size, and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_actionslider_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Elm.Actionslider", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   sizeof(Elm_Actionslider_Data),
   _elm_actionslider_class_initializer,
   _elm_actionslider_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_actionslider_class_get, &_elm_actionslider_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_actionslider_eo.legacy.c"
