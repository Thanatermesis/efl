/**
 * @internal
 * @brief Sets the day number for the calendar item.
 *
 * This is the internal implementation of elm_calendar_item_day_number_set.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param i The day number to set.
 */
void _elm_calendar_item_day_number_set(Eo *obj, Elm_Calendar_Item_Data *pd, int i);

/**
 * @internal
 * @brief Eolian reflection function for the day_number_set property.
 *
 * This function is called by Eolian to set the "day_number" property
 * using an Eina_Value.
 *
 * @param obj The Evas object.
 * @param val The Eina_Value containing the integer day number.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_calendar_item_day_number_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_calendar_item_day_number_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_calendar_item_day_number_set, EFL_FUNC_CALL(i), int i);

/**
 * @internal
 * @brief Gets the day number for the calendar item.
 *
 * This is the internal implementation of elm_calendar_item_day_number_get.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The day number.
 */
int _elm_calendar_item_day_number_get(const Eo *obj, Elm_Calendar_Item_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the day_number_get property.
 *
 * This function is called by Eolian to get the "day_number" property
 * as an Eina_Value.
 *
 * @param obj The Evas object.
 * @return An Eina_Value initialized with the integer day number.
 */
static Eina_Value
__eolian_elm_calendar_item_day_number_get_reflect(const Eo *obj)
{
   int val = elm_calendar_item_day_number_get(obj);
   return eina_value_int_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_calendar_item_day_number_get, int, 0);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Object.focus_set interface.
 *
 * Sets the focus state of the calendar item.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @param focus EINA_TRUE to set focus, EINA_FALSE to unset.
 */
void _elm_calendar_item_efl_ui_focus_object_focus_set(Eo *obj, Elm_Calendar_Item_Data *pd, Eina_Bool focus);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Object.focus_parent_get interface.
 *
 * Gets the focus parent of the calendar item.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The focus parent object.
 */
Efl_Ui_Focus_Object *_elm_calendar_item_efl_ui_focus_object_focus_parent_get(const Eo *obj, Elm_Calendar_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Object.focus_geometry_get interface.
 *
 * Gets the focus geometry of the calendar item.
 *
 * @param obj The Evas object.
 * @param pd The private data of the object.
 * @return The focus geometry as an Eina_Rect.
 */
Eina_Rect _elm_calendar_item_efl_ui_focus_object_focus_geometry_get(const Eo *obj, Elm_Calendar_Item_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Calendar_Item Efl_Class.
 *
 * This function is called once when the class is first used.
 * It sets up the Eolian operations and reflection data for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_calendar_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_CALENDAR_ITEM_EXTRA_OPS
#define ELM_CALENDAR_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_calendar_item_day_number_set, _elm_calendar_item_day_number_set),
      EFL_OBJECT_OP_FUNC(elm_calendar_item_day_number_get, _elm_calendar_item_day_number_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_set, _elm_calendar_item_efl_ui_focus_object_focus_set),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_parent_get, _elm_calendar_item_efl_ui_focus_object_focus_parent_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_geometry_get, _elm_calendar_item_efl_ui_focus_object_focus_geometry_get),
      ELM_CALENDAR_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"day_number", __eolian_elm_calendar_item_day_number_set_reflect, __eolian_elm_calendar_item_day_number_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Calendar_Item Efl_Class.
 *
 * This structure provides metadata for the Efl_Class, such as its version,
 * name, type, size of instance data, and initializer functions.
 */
static const Efl_Class_Description _elm_calendar_item_class_desc = {
   EO_VERSION,
   "Elm.Calendar.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Calendar_Item_Data),
   _elm_calendar_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_calendar_item_class_get, &_elm_calendar_item_class_desc, EFL_OBJECT_CLASS, EFL_UI_FOCUS_OBJECT_MIXIN, NULL);
