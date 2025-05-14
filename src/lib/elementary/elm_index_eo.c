EWAPI const Efl_Event_Description _ELM_INDEX_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");
EWAPI const Efl_Event_Description _ELM_INDEX_EVENT_DELAY_CHANGED =
   EFL_EVENT_DESCRIPTION("delay,changed");
EWAPI const Efl_Event_Description _ELM_INDEX_EVENT_LEVEL_UP =
   EFL_EVENT_DESCRIPTION("level,up");
EWAPI const Efl_Event_Description _ELM_INDEX_EVENT_LEVEL_DOWN =
   EFL_EVENT_DESCRIPTION("level,down");

void _elm_index_autohide_disabled_set(Eo *obj, Elm_Index_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function to set the 'autohide_disabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'autohide_disabled' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to a boolean and then calls the
 * concrete C API function elm_obj_index_autohide_disabled_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the boolean value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_BOOL.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_autohide_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_autohide_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_autohide_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

Eina_Bool _elm_index_autohide_disabled_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'autohide_disabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'autohide_disabled' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_autohide_disabled_get() and wraps
 * the returned boolean value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the boolean value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_BOOL.
 */
static Eina_Value
__eolian_elm_index_autohide_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_index_autohide_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_autohide_disabled_get, Eina_Bool, 0);

void _elm_index_omit_enabled_set(Eo *obj, Elm_Index_Data *pd, Eina_Bool enabled);

/**
 * @internal
 * @brief Reflection function to set the 'omit_enabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'omit_enabled' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to a boolean and then calls the
 * concrete C API function elm_obj_index_omit_enabled_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the boolean value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_BOOL.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_omit_enabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_omit_enabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_omit_enabled_set, EFL_FUNC_CALL(enabled), Eina_Bool enabled);

Eina_Bool _elm_index_omit_enabled_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'omit_enabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'omit_enabled' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_omit_enabled_get() and wraps
 * the returned boolean value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the boolean value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_BOOL.
 */
static Eina_Value
__eolian_elm_index_omit_enabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_index_omit_enabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_omit_enabled_get, Eina_Bool, 0);

void _elm_index_standard_priority_set(Eo *obj, Elm_Index_Data *pd, int priority);

/**
 * @internal
 * @brief Reflection function to set the 'standard_priority' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'standard_priority' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to an integer and then calls the
 * concrete C API function elm_obj_index_standard_priority_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the integer value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_INT.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_standard_priority_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_standard_priority_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_standard_priority_set, EFL_FUNC_CALL(priority), int priority);

int _elm_index_standard_priority_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'standard_priority' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'standard_priority' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_standard_priority_get() and wraps
 * the returned integer value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the integer value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_INT.
 */
static Eina_Value
__eolian_elm_index_standard_priority_get_reflect(const Eo *obj)
{
   int val = elm_obj_index_standard_priority_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_standard_priority_get, int, 0);

void _elm_index_delay_change_time_set(Eo *obj, Elm_Index_Data *pd, double dtime);

/**
 * @internal
 * @brief Reflection function to set the 'delay_change_time' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'delay_change_time' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to a double and then calls the
 * concrete C API function elm_obj_index_delay_change_time_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the double value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_DOUBLE.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_delay_change_time_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_delay_change_time_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_delay_change_time_set, EFL_FUNC_CALL(dtime), double dtime);

double _elm_index_delay_change_time_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'delay_change_time' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'delay_change_time' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_delay_change_time_get() and wraps
 * the returned double value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the double value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_DOUBLE.
 */
static Eina_Value
__eolian_elm_index_delay_change_time_get_reflect(const Eo *obj)
{
   double val = elm_obj_index_delay_change_time_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_delay_change_time_get, double, 0);

void _elm_index_indicator_disabled_set(Eo *obj, Elm_Index_Data *pd, Eina_Bool disabled);

/**
 * @internal
 * @brief Reflection function to set the 'indicator_disabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'indicator_disabled' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to a boolean and then calls the
 * concrete C API function elm_obj_index_indicator_disabled_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the boolean value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_BOOL.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_indicator_disabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_indicator_disabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_indicator_disabled_set, EFL_FUNC_CALL(disabled), Eina_Bool disabled);

Eina_Bool _elm_index_indicator_disabled_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'indicator_disabled' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'indicator_disabled' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_indicator_disabled_get() and wraps
 * the returned boolean value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the boolean value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_BOOL.
 */
static Eina_Value
__eolian_elm_index_indicator_disabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_index_indicator_disabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_indicator_disabled_get, Eina_Bool, 0);

void _elm_index_item_level_set(Eo *obj, Elm_Index_Data *pd, int level);

/**
 * @internal
 * @brief Reflection function to set the 'item_level' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'item_level' property is set via reflection. It converts
 * the provided @p val (an Eina_Value) to an integer and then calls the
 * concrete C API function elm_obj_index_item_level_set().
 *
 * @param[in] obj The EFL object instance.
 * @param[in] val An Eina_Value containing the integer value for the property.
 *                The value type is expected to be EINA_VALUE_TYPE_INT.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if conversion fails.
 */
static Eina_Error
__eolian_elm_index_item_level_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_item_level_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_item_level_set, EFL_FUNC_CALL(level), int level);

int _elm_index_item_level_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Reflection function to get the 'item_level' property.
 *
 * This function is part of the Eolian reflection system. It is invoked
 * when the 'item_level' property is read via reflection. It calls the
 * concrete C API function elm_obj_index_item_level_get() and wraps
 * the returned integer value in an Eina_Value.
 *
 * @param[in] obj The EFL object instance.
 * @return An Eina_Value containing the integer value of the property.
 *         The Eina_Value will be of type EINA_VALUE_TYPE_INT.
 */
static Eina_Value
__eolian_elm_index_item_level_get_reflect(const Eo *obj)
{
   int val = elm_obj_index_item_level_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_item_level_get, int, 0);

void _elm_index_level_go(Eo *obj, Elm_Index_Data *pd, int level);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_level_go, EFL_FUNC_CALL(level), int level);

Elm_Widget_Item *_elm_index_item_prepend(Eo *obj, Elm_Index_Data *pd, const char *letter, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_prepend, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(letter, func, data), const char *letter, Evas_Smart_Cb func, const void *data);

void _elm_index_item_clear(Eo *obj, Elm_Index_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_index_item_clear);

Elm_Widget_Item *_elm_index_item_insert_after(Eo *obj, Elm_Index_Data *pd, Elm_Widget_Item *after, const char *letter, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_insert_after, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(after, letter, func, data), Elm_Widget_Item *after, const char *letter, Evas_Smart_Cb func, const void *data);

Elm_Widget_Item *_elm_index_item_find(Eo *obj, Elm_Index_Data *pd, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_find, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(data), const void *data);

Elm_Widget_Item *_elm_index_item_insert_before(Eo *obj, Elm_Index_Data *pd, Elm_Widget_Item *before, const char *letter, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_insert_before, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(before, letter, func, data), Elm_Widget_Item *before, const char *letter, Evas_Smart_Cb func, const void *data);

Elm_Widget_Item *_elm_index_item_append(Eo *obj, Elm_Index_Data *pd, const char *letter, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(letter, func, data), const char *letter, Evas_Smart_Cb func, const void *data);

Elm_Widget_Item *_elm_index_selected_item_get(const Eo *obj, Elm_Index_Data *pd, int level);

EOAPI EFL_FUNC_BODYV_CONST(elm_obj_index_selected_item_get, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(level), int level);

Elm_Widget_Item *_elm_index_item_sorted_insert(Eo *obj, Elm_Index_Data *pd, const char *letter, Evas_Smart_Cb func, const void *data, Eina_Compare_Cb cmp_func, Eina_Compare_Cb cmp_data_func);

EOAPI EFL_FUNC_BODYV(elm_obj_index_item_sorted_insert, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(letter, func, data, cmp_func, cmp_data_func), const char *letter, Evas_Smart_Cb func, const void *data, Eina_Compare_Cb cmp_func, Eina_Compare_Cb cmp_data_func);

Efl_Object *_elm_index_efl_object_constructor(Eo *obj, Elm_Index_Data *pd);


Eina_Error _elm_index_efl_ui_widget_theme_apply(Eo *obj, Elm_Index_Data *pd);


void _elm_index_efl_ui_widget_on_access_update(Eo *obj, Elm_Index_Data *pd, Eina_Bool enable);


void _elm_index_efl_ui_layout_orientable_orientation_set(Eo *obj, Elm_Index_Data *pd, Efl_Ui_Layout_Orientation dir);


Efl_Ui_Layout_Orientation _elm_index_efl_ui_layout_orientable_orientation_get(const Eo *obj, Elm_Index_Data *pd);


Eina_List *_elm_index_efl_access_object_access_children_get(const Eo *obj, Elm_Index_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Index Efl_Class.
 *
 * This function is called once when the Elm_Index class is being set up.
 * It defines the Evas Object operations (methods) for the class, mapping
 * EFL_OBJECT_OP_FUNC IDs to their C implementations (the _elm_index_* functions
 * which are typically defined in elm_index.c or similar).
 * It also sets up the property reflection operations, linking property names
 * to their respective _reflect set and get functions.
 *
 * @param[in,out] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_index_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_INDEX_EXTRA_OPS
#define ELM_INDEX_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_index_autohide_disabled_set, _elm_index_autohide_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_autohide_disabled_get, _elm_index_autohide_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_omit_enabled_set, _elm_index_omit_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_omit_enabled_get, _elm_index_omit_enabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_standard_priority_set, _elm_index_standard_priority_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_standard_priority_get, _elm_index_standard_priority_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_delay_change_time_set, _elm_index_delay_change_time_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_delay_change_time_get, _elm_index_delay_change_time_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_indicator_disabled_set, _elm_index_indicator_disabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_indicator_disabled_get, _elm_index_indicator_disabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_level_set, _elm_index_item_level_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_level_get, _elm_index_item_level_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_level_go, _elm_index_level_go),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_prepend, _elm_index_item_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_clear, _elm_index_item_clear),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_insert_after, _elm_index_item_insert_after),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_find, _elm_index_item_find),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_insert_before, _elm_index_item_insert_before),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_append, _elm_index_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_index_selected_item_get, _elm_index_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_sorted_insert, _elm_index_item_sorted_insert),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_index_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_index_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_index_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_layout_orientation_set, _elm_index_efl_ui_layout_orientable_orientation_set),
      EFL_OBJECT_OP_FUNC(efl_ui_layout_orientation_get, _elm_index_efl_ui_layout_orientable_orientation_get),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_index_efl_access_object_access_children_get),
      ELM_INDEX_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"autohide_disabled", __eolian_elm_index_autohide_disabled_set_reflect, __eolian_elm_index_autohide_disabled_get_reflect},
      {"omit_enabled", __eolian_elm_index_omit_enabled_set_reflect, __eolian_elm_index_omit_enabled_get_reflect},
      {"standard_priority", __eolian_elm_index_standard_priority_set_reflect, __eolian_elm_index_standard_priority_get_reflect},
      {"delay_change_time", __eolian_elm_index_delay_change_time_set_reflect, __eolian_elm_index_delay_change_time_get_reflect},
      {"indicator_disabled", __eolian_elm_index_indicator_disabled_set_reflect, __eolian_elm_index_indicator_disabled_get_reflect},
      {"item_level", __eolian_elm_index_item_level_set_reflect, __eolian_elm_index_item_level_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief The Efl_Class_Description for the Elm_Index class.
 *
 * This structure provides metadata for the Elm_Index class, including its
 * EO_VERSION, name ("Elm.Index"), type (EFL_CLASS_TYPE_REGULAR),
 * the size of its instance-specific data structure (Elm_Index_Data),
 * and pointers to its class initializer (_elm_index_class_initializer),
 * class constructor (_elm_index_class_constructor), and class destructor (NULL).
 */
static const Efl_Class_Description _elm_index_class_desc = {
   EO_VERSION,
   "Elm.Index",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Index_Data),
   _elm_index_class_initializer,
   _elm_index_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_index_class_get, &_elm_index_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_LAYOUT_ORIENTABLE_INTERFACE, EFL_INPUT_CLICKABLE_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_index_eo.legacy.c"
