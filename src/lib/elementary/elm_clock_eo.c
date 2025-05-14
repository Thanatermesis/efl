/**
 * @internal
 * @brief Event descriptor for the "changed" event.
 * @details This event is emitted when the clock's time is changed by the user
 *          interacting with the clock widget in edit mode. This global variable
 *          is used to register and emit this specific event.
 */
EWAPI const Efl_Event_Description _ELM_CLOCK_EVENT_CHANGED =
   EFL_EVENT_DESCRIPTION("changed");

void _elm_clock_show_am_pm_set(Eo *obj, Elm_Clock_Data *pd, Eina_Bool am_pm);


/**
 * @internal
 * @brief Eolian reflection function for setting the 'show_am_pm' property.
 *
 * This function is part of the Eolian reflection system. It is called when
 * the 'show_am_pm' property is set via efl_property_reflection_set() or
 * equivalent Eolian mechanisms. It converts an Eina_Value (which is a generic
 * value container) to the specific type required by the property (Eina_Bool in this case)
 * and then calls the underlying C implementation (elm_obj_clock_show_am_pm_set).
 *
 * @param obj The Efl object instance on which the property is being set.
 * @param val An Eina_Value holding the new boolean state for the 'show_am_pm' property.
 *            For example, to set it to true, val might be initialized with `eina_value_bool_init(EINA_TRUE)`.
 * @return #EINA_ERROR_NO_ERROR on success, or an Eina_Error code if the
 *         Eina_Value conversion to Eina_Bool fails.
 */
static Eina_Error
__eolian_elm_clock_show_am_pm_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_clock_show_am_pm_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_show_am_pm_set, EFL_FUNC_CALL(am_pm), Eina_Bool am_pm);

Eina_Bool _elm_clock_show_am_pm_get(const Eo *obj, Elm_Clock_Data *pd);


/**
 * @internal
 * @brief Eolian reflection function for getting the 'show_am_pm' property.
 *
 * This function is part of the Eolian reflection system. It is called when
 * the 'show_am_pm' property is read via efl_property_reflection_get() or
 * equivalent Eolian mechanisms. It calls the underlying C implementation
 * (elm_obj_clock_show_am_pm_get) and wraps the returned Eina_Bool value
 * into an Eina_Value container for generic handling by the Eolian system.
 *
 * @param obj The Efl object instance from which the property is being read.
 * @return An Eina_Value holding the boolean value of the 'show_am_pm' property.
 *         For example, if 'show_am_pm' is true, this returns an Eina_Value
 *         equivalent to one from `eina_value_bool_init(EINA_TRUE)`.
 *         The caller is responsible for flushing the returned Eina_Value using eina_value_flush().
 */
static Eina_Value
__eolian_elm_clock_show_am_pm_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_clock_show_am_pm_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_show_am_pm_get, Eina_Bool, 0);

void _elm_clock_first_interval_set(Eo *obj, Elm_Clock_Data *pd, double interval);


static Eina_Error
__eolian_elm_clock_first_interval_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_clock_first_interval_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_first_interval_set, EFL_FUNC_CALL(interval), double interval);

double _elm_clock_first_interval_get(const Eo *obj, Elm_Clock_Data *pd);


static Eina_Value
__eolian_elm_clock_first_interval_get_reflect(const Eo *obj)
{
   double val = elm_obj_clock_first_interval_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_first_interval_get, double, 0);

void _elm_clock_show_seconds_set(Eo *obj, Elm_Clock_Data *pd, Eina_Bool seconds);


static Eina_Error
__eolian_elm_clock_show_seconds_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_clock_show_seconds_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_show_seconds_set, EFL_FUNC_CALL(seconds), Eina_Bool seconds);

Eina_Bool _elm_clock_show_seconds_get(const Eo *obj, Elm_Clock_Data *pd);


static Eina_Value
__eolian_elm_clock_show_seconds_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_clock_show_seconds_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_show_seconds_get, Eina_Bool, 0);

void _elm_clock_edit_set(Eo *obj, Elm_Clock_Data *pd, Eina_Bool edit);


static Eina_Error
__eolian_elm_clock_edit_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_clock_edit_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_edit_set, EFL_FUNC_CALL(edit), Eina_Bool edit);

Eina_Bool _elm_clock_edit_get(const Eo *obj, Elm_Clock_Data *pd);


static Eina_Value
__eolian_elm_clock_edit_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_clock_edit_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_edit_get, Eina_Bool, 0);

void _elm_clock_pause_set(Eo *obj, Elm_Clock_Data *pd, Eina_Bool paused);


static Eina_Error
__eolian_elm_clock_pause_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_clock_pause_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_pause_set, EFL_FUNC_CALL(paused), Eina_Bool paused);

Eina_Bool _elm_clock_pause_get(const Eo *obj, Elm_Clock_Data *pd);


static Eina_Value
__eolian_elm_clock_pause_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_clock_pause_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_pause_get, Eina_Bool, 0);

void _elm_clock_time_set(Eo *obj, Elm_Clock_Data *pd, int hrs, int min, int sec);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_time_set, EFL_FUNC_CALL(hrs, min, sec), int hrs, int min, int sec);

void _elm_clock_time_get(const Eo *obj, Elm_Clock_Data *pd, int *hrs, int *min, int *sec);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_clock_time_get, EFL_FUNC_CALL(hrs, min, sec), int *hrs, int *min, int *sec);

void _elm_clock_edit_mode_set(Eo *obj, Elm_Clock_Data *pd, Elm_Clock_Edit_Mode digedit);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_clock_edit_mode_set, EFL_FUNC_CALL(digedit), Elm_Clock_Edit_Mode digedit);

Elm_Clock_Edit_Mode _elm_clock_edit_mode_get(const Eo *obj, Elm_Clock_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_clock_edit_mode_get, Elm_Clock_Edit_Mode, 0);

Efl_Object *_elm_clock_efl_object_constructor(Eo *obj, Elm_Clock_Data *pd);


void _elm_clock_efl_ui_widget_on_access_update(Eo *obj, Elm_Clock_Data *pd, Eina_Bool enable);


Eina_Error _elm_clock_efl_ui_widget_theme_apply(Eo *obj, Elm_Clock_Data *pd);


/**
 * @internal
 * @brief Initializes the Elm_Clock Efl class.
 *
 * This function is automatically called by the Efl class system when the
 * Elm_Clock class is first loaded or used. Its primary role is to register
 * the C functions that implement the Eolian methods (operations) and
 * properties (including their reflection handlers) for the Elm_Clock class.
 * This connects the Eolian definitions to their C implementations.
 *
 * @param klass The Efl_Class (specifically Elm_Clock_Class) to initialize.
 * @return @c EINA_TRUE on successful initialization of the class functions,
 *         @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_clock_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_CLOCK_EXTRA_OPS
#define ELM_CLOCK_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_clock_show_am_pm_set, _elm_clock_show_am_pm_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_show_am_pm_get, _elm_clock_show_am_pm_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_first_interval_set, _elm_clock_first_interval_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_first_interval_get, _elm_clock_first_interval_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_show_seconds_set, _elm_clock_show_seconds_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_show_seconds_get, _elm_clock_show_seconds_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_edit_set, _elm_clock_edit_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_edit_get, _elm_clock_edit_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_pause_set, _elm_clock_pause_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_pause_get, _elm_clock_pause_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_time_set, _elm_clock_time_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_time_get, _elm_clock_time_get),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_edit_mode_set, _elm_clock_edit_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_clock_edit_mode_get, _elm_clock_edit_mode_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_clock_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_on_access_update, _elm_clock_efl_ui_widget_on_access_update),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_clock_efl_ui_widget_theme_apply),
      ELM_CLOCK_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"show_am_pm", __eolian_elm_clock_show_am_pm_set_reflect, __eolian_elm_clock_show_am_pm_get_reflect},
      {"first_interval", __eolian_elm_clock_first_interval_set_reflect, __eolian_elm_clock_first_interval_get_reflect},
      {"show_seconds", __eolian_elm_clock_show_seconds_set_reflect, __eolian_elm_clock_show_seconds_get_reflect},
      {"edit", __eolian_elm_clock_edit_set_reflect, __eolian_elm_clock_edit_get_reflect},
      {"pause", __eolian_elm_clock_pause_set_reflect, __eolian_elm_clock_pause_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Clock Efl class structure.
 *
 * This static constant structure provides the Efl class system with
 * essential metadata about the Elm_Clock class. This metadata includes:
 * - EO_VERSION: The Eolian object system version this class complies with.
 * - "Elm.Clock": The unique name of the class in the Eolian system.
 * - EFL_CLASS_TYPE_REGULAR: Specifies that this is a standard Efl class.
 * - sizeof(Elm_Clock_Data): The size of the private instance data structure for Elm_Clock objects.
 * - _elm_clock_class_initializer: A pointer to the class initializer function (defined above).
 * - _elm_clock_class_constructor: A pointer to the class constructor function (called when a new instance is created).
 * - NULL: Placeholder for a class destructor, if one were defined.
 * This structure is used by EFL_DEFINE_CLASS to register the class.
 */
static const Efl_Class_Description _elm_clock_class_desc = {
   EO_VERSION,
   "Elm.Clock",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Clock_Data),
   _elm_clock_class_initializer,
   _elm_clock_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_clock_class_get, &_elm_clock_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_FOCUS_COMPOSITION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_clock_eo.legacy.c"
