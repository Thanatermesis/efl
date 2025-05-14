
void _elm_gesture_layer_zoom_step_set(Eo *obj, Elm_Gesture_Layer_Data *pd, double step);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_zoom_step_set method.
 *
 * This function is called by the Eolian reflection system to set the
 * zoom_step property using an Eina_Value. It converts the Eina_Value
 * to a double and calls elm_obj_gesture_layer_zoom_step_set.
 *
 * @param[in] obj The Eolian object.
 * @param[in] val The Eina_Value containing the new zoom step.
 * @return EINA_ERROR_NO_ERROR on success, or EINA_ERROR_VALUE_FAILED if
 *         the Eina_Value cannot be converted to a double.
 */
static Eina_Error
__eolian_elm_gesture_layer_zoom_step_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_gesture_layer_zoom_step_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_zoom_step_set, EFL_FUNC_CALL(step), double step);

double _elm_gesture_layer_zoom_step_get(const Eo *obj, Elm_Gesture_Layer_Data *pd);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_zoom_step_get method.
 *
 * This function is called by the Eolian reflection system to get the
 * zoom_step property and return it as an Eina_Value. It calls
 * elm_obj_gesture_layer_zoom_step_get and wraps the result in an Eina_Value.
 *
 * @param[in] obj The Eolian object.
 * @return An Eina_Value initialized with the double value of the zoom step.
 */
static Eina_Value
__eolian_elm_gesture_layer_zoom_step_get_reflect(const Eo *obj)
{
   double val = elm_obj_gesture_layer_zoom_step_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_gesture_layer_zoom_step_get, double, 0);

void _elm_gesture_layer_tap_finger_size_set(Eo *obj, Elm_Gesture_Layer_Data *pd, int sz);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_tap_finger_size_set method.
 *
 * This function is called by the Eolian reflection system to set the
 * tap_finger_size property using an Eina_Value. It converts the Eina_Value
 * to an int and calls elm_obj_gesture_layer_tap_finger_size_set.
 *
 * @param[in] obj The Eolian object.
 * @param[in] val The Eina_Value containing the new tap finger size.
 * @return EINA_ERROR_NO_ERROR on success, or EINA_ERROR_VALUE_FAILED if
 *         the Eina_Value cannot be converted to an int.
 */
static Eina_Error
__eolian_elm_gesture_layer_tap_finger_size_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_gesture_layer_tap_finger_size_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_tap_finger_size_set, EFL_FUNC_CALL(sz), int sz);

int _elm_gesture_layer_tap_finger_size_get(const Eo *obj, Elm_Gesture_Layer_Data *pd);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_tap_finger_size_get method.
 *
 * This function is called by the Eolian reflection system to get the
 * tap_finger_size property and return it as an Eina_Value. It calls
 * elm_obj_gesture_layer_tap_finger_size_get and wraps the result in an Eina_Value.
 *
 * @param[in] obj The Eolian object.
 * @return An Eina_Value initialized with the int value of the tap finger size.
 */
static Eina_Value
__eolian_elm_gesture_layer_tap_finger_size_get_reflect(const Eo *obj)
{
   int val = elm_obj_gesture_layer_tap_finger_size_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_gesture_layer_tap_finger_size_get, int, 0);

void _elm_gesture_layer_hold_events_set(Eo *obj, Elm_Gesture_Layer_Data *pd, Eina_Bool hold_events);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_hold_events_set method.
 *
 * This function is called by the Eolian reflection system to set the
 * hold_events property using an Eina_Value. It converts the Eina_Value
 * to an Eina_Bool and calls elm_obj_gesture_layer_hold_events_set.
 *
 * @param[in] obj The Eolian object.
 * @param[in] val The Eina_Value containing the new hold_events setting (Eina_Bool).
 * @return EINA_ERROR_NO_ERROR on success, or EINA_ERROR_VALUE_FAILED if
 *         the Eina_Value cannot be converted to an Eina_Bool.
 */
static Eina_Error
__eolian_elm_gesture_layer_hold_events_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_gesture_layer_hold_events_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_hold_events_set, EFL_FUNC_CALL(hold_events), Eina_Bool hold_events);

Eina_Bool _elm_gesture_layer_hold_events_get(const Eo *obj, Elm_Gesture_Layer_Data *pd);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_hold_events_get method.
 *
 * This function is called by the Eolian reflection system to get the
 * hold_events property and return it as an Eina_Value. It calls
 * elm_obj_gesture_layer_hold_events_get and wraps the Eina_Bool result in an Eina_Value.
 *
 * @param[in] obj The Eolian object.
 * @return An Eina_Value initialized with the Eina_Bool value of the hold_events setting.
 */
static Eina_Value
__eolian_elm_gesture_layer_hold_events_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_gesture_layer_hold_events_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_gesture_layer_hold_events_get, Eina_Bool, 0);

void _elm_gesture_layer_rotate_step_set(Eo *obj, Elm_Gesture_Layer_Data *pd, double step);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_rotate_step_set method.
 *
 * This function is called by the Eolian reflection system to set the
 * rotate_step property using an Eina_Value. It converts the Eina_Value
 * to a double and calls elm_obj_gesture_layer_rotate_step_set.
 *
 * @param[in] obj The Eolian object.
 * @param[in] val The Eina_Value containing the new rotate step value.
 * @return EINA_ERROR_NO_ERROR on success, or EINA_ERROR_VALUE_FAILED if
 *         the Eina_Value cannot be converted to a double.
 */
static Eina_Error
__eolian_elm_gesture_layer_rotate_step_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_gesture_layer_rotate_step_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_rotate_step_set, EFL_FUNC_CALL(step), double step);

double _elm_gesture_layer_rotate_step_get(const Eo *obj, Elm_Gesture_Layer_Data *pd);

/**
 * @brief Eolian reflection function for the elm_obj_gesture_layer_rotate_step_get method.
 *
 * This function is called by the Eolian reflection system to get the
 * rotate_step property and return it as an Eina_Value. It calls
 * elm_obj_gesture_layer_rotate_step_get and wraps the double result in an Eina_Value.
 *
 * @param[in] obj The Eolian object.
 * @return An Eina_Value initialized with the double value of the rotate step.
 */
static Eina_Value
__eolian_elm_gesture_layer_rotate_step_get_reflect(const Eo *obj)
{
   double val = elm_obj_gesture_layer_rotate_step_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_gesture_layer_rotate_step_get, double, 0);

void _elm_gesture_layer_cb_set(Eo *obj, Elm_Gesture_Layer_Data *pd, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_cb_set, EFL_FUNC_CALL(idx, cb_type, cb, data), Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

Eina_Bool _elm_gesture_layer_attach(Eo *obj, Elm_Gesture_Layer_Data *pd, Efl_Canvas_Object *target);

EOAPI EFL_FUNC_BODYV(elm_obj_gesture_layer_attach, Eina_Bool, 0, EFL_FUNC_CALL(target), Efl_Canvas_Object *target);

void _elm_gesture_layer_cb_del(Eo *obj, Elm_Gesture_Layer_Data *pd, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_cb_del, EFL_FUNC_CALL(idx, cb_type, cb, data), Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

void _elm_gesture_layer_cb_add(Eo *obj, Elm_Gesture_Layer_Data *pd, Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_gesture_layer_cb_add, EFL_FUNC_CALL(idx, cb_type, cb, data), Elm_Gesture_Type idx, Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb, void *data);

/**
 * @brief Implements the Efl_Object.constructor interface for Elm_Gesture_Layer.
 *
 * This function is called when an Elm_Gesture_Layer object is constructed.
 * It performs necessary initialization for the gesture layer instance.
 *
 * @param[in] obj The Eolian object being constructed.
 * @param[in] pd Pointer to the private data of the Elm_Gesture_Layer instance.
 * @return The constructed Eolian object (typically @p obj).
 */
Efl_Object *_elm_gesture_layer_efl_object_constructor(Eo *obj, Elm_Gesture_Layer_Data *pd);

/**
 * @brief Implements the Efl_Ui_Widget.disabled_set interface for Elm_Gesture_Layer.
 *
 * This function is called when the disabled state of the Elm_Gesture_Layer
 * widget is changed. It handles the specific logic for enabling or disabling
 * the gesture layer.
 *
 * @param[in] obj The Eolian object.
 * @param[in] pd Pointer to the private data of the Elm_Gesture_Layer instance.
 * @param[in] disabled EINA_TRUE if the widget is to be disabled, EINA_FALSE otherwise.
 */
void _elm_gesture_layer_efl_ui_widget_disabled_set(Eo *obj, Elm_Gesture_Layer_Data *pd, Eina_Bool disabled);

/**
 * @brief Initializes the Elm_Gesture_Layer Eolian class.
 *
 * This static function is responsible for setting up the Eolian operations (methods)
 * and property reflection capabilities for the Elm_Gesture_Layer class during
 * class construction. It defines which C functions implement the Eolian API
 * for this class.
 *
 * @param[in] klass The Efl_Class representing the Elm_Gesture_Layer class.
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_gesture_layer_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_GESTURE_LAYER_EXTRA_OPS
#define ELM_GESTURE_LAYER_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_zoom_step_set, _elm_gesture_layer_zoom_step_set),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_zoom_step_get, _elm_gesture_layer_zoom_step_get),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_tap_finger_size_set, _elm_gesture_layer_tap_finger_size_set),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_tap_finger_size_get, _elm_gesture_layer_tap_finger_size_get),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_hold_events_set, _elm_gesture_layer_hold_events_set),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_hold_events_get, _elm_gesture_layer_hold_events_get),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_rotate_step_set, _elm_gesture_layer_rotate_step_set),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_rotate_step_get, _elm_gesture_layer_rotate_step_get),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_cb_set, _elm_gesture_layer_cb_set),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_attach, _elm_gesture_layer_attach),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_cb_del, _elm_gesture_layer_cb_del),
      EFL_OBJECT_OP_FUNC(elm_obj_gesture_layer_cb_add, _elm_gesture_layer_cb_add),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_gesture_layer_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_disabled_set, _elm_gesture_layer_efl_ui_widget_disabled_set),
      ELM_GESTURE_LAYER_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"zoom_step", __eolian_elm_gesture_layer_zoom_step_set_reflect, __eolian_elm_gesture_layer_zoom_step_get_reflect},
      {"tap_finger_size", __eolian_elm_gesture_layer_tap_finger_size_set_reflect, __eolian_elm_gesture_layer_tap_finger_size_get_reflect},
      {"hold_events", __eolian_elm_gesture_layer_hold_events_set_reflect, __eolian_elm_gesture_layer_hold_events_get_reflect},
      {"rotate_step", __eolian_elm_gesture_layer_rotate_step_set_reflect, __eolian_elm_gesture_layer_rotate_step_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_gesture_layer_class_desc = {
   EO_VERSION,
   "Elm.Gesture_Layer",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Gesture_Layer_Data),
   _elm_gesture_layer_class_initializer,
   _elm_gesture_layer_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_gesture_layer_class_get, &_elm_gesture_layer_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_gesture_layer_eo.legacy.c"
