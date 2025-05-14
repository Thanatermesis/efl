EWAPI const Efl_Event_Description _ELM_LABEL_EVENT_SLIDE_END =
   EFL_EVENT_DESCRIPTION("slide,end");

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_wrap_width_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param w The wrap width.
 */
void _elm_label_wrap_width_set(Eo *obj, Elm_Label_Data *pd, int w);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_wrap_width_set property.
 *
 * This function is used by the Eolian system to set the property value
 * from an Eina_Value.
 *
 * @param obj The EObject.
 * @param val The Eina_Value containing the new property value.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_label_wrap_width_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_label_wrap_width_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_wrap_width_set, EFL_FUNC_CALL(w), int w);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_wrap_width_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The wrap width.
 */
int _elm_label_wrap_width_get(const Eo *obj, Elm_Label_Data *pd);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_wrap_width_get property.
 *
 * This function is used by the Eolian system to get the property value
 * as an Eina_Value.
 *
 * @param obj The EObject.
 * @return An Eina_Value containing the property value.
 */
static Eina_Value
__eolian_elm_label_wrap_width_get_reflect(const Eo *obj)
{
   int val = elm_obj_label_wrap_width_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_wrap_width_get, int, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_speed_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param speed The slide speed.
 */
void _elm_label_slide_speed_set(Eo *obj, Elm_Label_Data *pd, double speed);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_slide_speed_set property.
 *
 * This function is used by the Eolian system to set the property value
 * from an Eina_Value.
 *
 * @param obj The EObject.
 * @param val The Eina_Value containing the new property value.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_label_slide_speed_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_label_slide_speed_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_slide_speed_set, EFL_FUNC_CALL(speed), double speed);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_speed_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The slide speed.
 */
double _elm_label_slide_speed_get(const Eo *obj, Elm_Label_Data *pd);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_slide_speed_get property.
 *
 * This function is used by the Eolian system to get the property value
 * as an Eina_Value.
 *
 * @param obj The EObject.
 * @return An Eina_Value containing the property value.
 */
static Eina_Value
__eolian_elm_label_slide_speed_get_reflect(const Eo *obj)
{
   double val = elm_obj_label_slide_speed_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_slide_speed_get, double, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_mode_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param mode The slide mode.
 */
void _elm_label_slide_mode_set(Eo *obj, Elm_Label_Data *pd, Elm_Label_Slide_Mode mode);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_slide_mode_set, EFL_FUNC_CALL(mode), Elm_Label_Slide_Mode mode);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_mode_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The slide mode.
 */
Elm_Label_Slide_Mode _elm_label_slide_mode_get(const Eo *obj, Elm_Label_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_slide_mode_get, Elm_Label_Slide_Mode, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_duration_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param duration The slide duration.
 */
void _elm_label_slide_duration_set(Eo *obj, Elm_Label_Data *pd, double duration);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_slide_duration_set property.
 *
 * This function is used by the Eolian system to set the property value
 * from an Eina_Value.
 *
 * @param obj The EObject.
 * @param val The Eina_Value containing the new property value.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_label_slide_duration_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_label_slide_duration_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_slide_duration_set, EFL_FUNC_CALL(duration), double duration);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_duration_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The slide duration.
 */
double _elm_label_slide_duration_get(const Eo *obj, Elm_Label_Data *pd);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_slide_duration_get property.
 *
 * This function is used by the Eolian system to get the property value
 * as an Eina_Value.
 *
 * @param obj The EObject.
 * @return An Eina_Value containing the property value.
 */
static Eina_Value
__eolian_elm_label_slide_duration_get_reflect(const Eo *obj)
{
   double val = elm_obj_label_slide_duration_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_slide_duration_get, double, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_line_wrap_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param wrap The wrap type.
 */
void _elm_label_line_wrap_set(Eo *obj, Elm_Label_Data *pd, Elm_Wrap_Type wrap);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_line_wrap_set, EFL_FUNC_CALL(wrap), Elm_Wrap_Type wrap);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_line_wrap_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The wrap type.
 */
Elm_Wrap_Type _elm_label_line_wrap_get(const Eo *obj, Elm_Label_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_line_wrap_get, Elm_Wrap_Type, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_ellipsis_set.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param ellipsis The ellipsis state.
 */
void _elm_label_ellipsis_set(Eo *obj, Elm_Label_Data *pd, Eina_Bool ellipsis);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_ellipsis_set property.
 *
 * This function is used by the Eolian system to set the property value
 * from an Eina_Value.
 *
 * @param obj The EObject.
 * @param val The Eina_Value containing the new property value.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_label_ellipsis_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_label_ellipsis_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_label_ellipsis_set, EFL_FUNC_CALL(ellipsis), Eina_Bool ellipsis);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_ellipsis_get.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return The ellipsis state.
 */
Eina_Bool _elm_label_ellipsis_get(const Eo *obj, Elm_Label_Data *pd);


/**
 * @internal
 * @brief Reflection function for the elm_obj_label_ellipsis_get property.
 *
 * This function is used by the Eolian system to get the property value
 * as an Eina_Value.
 *
 * @param obj The EObject.
 * @return An Eina_Value containing the property value.
 */
static Eina_Value
__eolian_elm_label_ellipsis_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_label_ellipsis_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_label_ellipsis_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_label_slide_go.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 */
void _elm_label_slide_go(Eo *obj, Elm_Label_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_label_slide_go);

/**
 * @internal
 * @brief Internal implementation for the Efl.Object.constructor method.
 *
 * @param obj The object being constructed.
 * @param pd The private data of the object.
 * @return The constructed object.
 */
Efl_Object *_elm_label_efl_object_constructor(Eo *obj, Elm_Label_Data *pd);

/**
 * @internal
 * @brief Internal implementation for the Efl.Ui.Widget.theme_apply method.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
Eina_Error _elm_label_efl_ui_widget_theme_apply(Eo *obj, Elm_Label_Data *pd);

/**
 * @internal
 * @brief Internal implementation for the Efl.Part.part_get method.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @param name The name of the part to get.
 * @return The part object, or NULL if not found.
 */
Efl_Object *_elm_label_efl_part_part_get(const Eo *obj, Elm_Label_Data *pd, const char *name);

/**
 * @internal
 * @brief Internal implementation for the Efl.Access.Widget_Action.elm_actions_get method.
 *
 * @param obj The object.
 * @param pd The private data of the object.
 * @return A list of access action data.
 */
const Efl_Access_Action_Data *_elm_label_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Label_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Label class.
 *
 * This function sets up the operations (methods) and reflection data for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_label_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_LABEL_EXTRA_OPS
#define ELM_LABEL_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_label_wrap_width_set, _elm_label_wrap_width_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_wrap_width_get, _elm_label_wrap_width_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_speed_set, _elm_label_slide_speed_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_speed_get, _elm_label_slide_speed_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_mode_set, _elm_label_slide_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_mode_get, _elm_label_slide_mode_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_duration_set, _elm_label_slide_duration_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_duration_get, _elm_label_slide_duration_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_line_wrap_set, _elm_label_line_wrap_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_line_wrap_get, _elm_label_line_wrap_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_ellipsis_set, _elm_label_ellipsis_set),
      EFL_OBJECT_OP_FUNC(elm_obj_label_ellipsis_get, _elm_label_ellipsis_get),
      EFL_OBJECT_OP_FUNC(elm_obj_label_slide_go, _elm_label_slide_go),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_label_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_label_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_label_efl_part_part_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_label_efl_access_widget_action_elm_actions_get),
      ELM_LABEL_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"wrap_width", __eolian_elm_label_wrap_width_set_reflect, __eolian_elm_label_wrap_width_get_reflect},
      {"slide_speed", __eolian_elm_label_slide_speed_set_reflect, __eolian_elm_label_slide_speed_get_reflect},
      {"slide_duration", __eolian_elm_label_slide_duration_set_reflect, __eolian_elm_label_slide_duration_get_reflect},
      {"ellipsis", __eolian_elm_label_ellipsis_set_reflect, __eolian_elm_label_ellipsis_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Label class.
 *
 * This structure contains metadata about the Elm_Label class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer and constructor functions.
 */
static const Efl_Class_Description _elm_label_class_desc = {
   EO_VERSION,
   "Elm.Label",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Label_Data),
   _elm_label_class_initializer,
   _elm_label_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_label_class_get, &_elm_label_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, EFL_ACCESS_WIDGET_ACTION_MIXIN, NULL);

#include "elm_label_eo.legacy.c"
