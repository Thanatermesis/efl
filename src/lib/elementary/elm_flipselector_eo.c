/** @internal Event descriptor for the "underflowed" event. See @ref ELM_FLIPSELECTOR_EVENT_UNDERFLOWED in the header file for details. */
EWAPI const Efl_Event_Description _ELM_FLIPSELECTOR_EVENT_UNDERFLOWED =
   EFL_EVENT_DESCRIPTION("underflowed");
/** @internal Event descriptor for the "overflowed" event. See @ref ELM_FLIPSELECTOR_EVENT_OVERFLOWED in the header file for details. */
EWAPI const Efl_Event_Description _ELM_FLIPSELECTOR_EVENT_OVERFLOWED =
   EFL_EVENT_DESCRIPTION("overflowed");

/**
 * @internal
 * @brief Internal implementation to get the list of items.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_items_get.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @return Const Eina_List of items, or @c NULL. Do not modify or free.
 */
const Eina_List *_elm_flipselector_items_get(const Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_flipselector_items_get, const Eina_List *, NULL);

/**
 * @internal
 * @brief Internal implementation to get the first item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_first_item_get.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @return The first Elm_Widget_Item, or @c NULL.
 */
Elm_Widget_Item *_elm_flipselector_first_item_get(const Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_flipselector_first_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation to get the last item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_last_item_get.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @return The last Elm_Widget_Item, or @c NULL.
 */
Elm_Widget_Item *_elm_flipselector_last_item_get(const Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_flipselector_last_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation to get the selected item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_selected_item_get.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @return The selected Elm_Widget_Item, or @c NULL.
 */
Elm_Widget_Item *_elm_flipselector_selected_item_get(const Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_flipselector_selected_item_get, Elm_Widget_Item *, NULL);

/**
 * @internal
 * @brief Internal implementation to set the first flip interval.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_first_interval_set.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @param interval The interval in seconds.
 */
void _elm_flipselector_first_interval_set(Eo *obj, Elm_Flipselector_Data *pd, double interval);

/**
 * @internal
 * @brief Eolian reflection function for setting the 'first_interval' property.
 * Converts an Eina_Value to a double and calls @ref _elm_flipselector_first_interval_set.
 * @param obj The Evas object.
 * @param val Eina_Value containing the double for the interval.
 * @return EINA_ERROR_NONE on success, EINA_ERROR_VALUE_FAILED on type mismatch.
 */
static Eina_Error
__eolian_elm_flipselector_first_interval_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_flipselector_first_interval_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_flipselector_first_interval_set, EFL_FUNC_CALL(interval), double interval);

/**
 * @internal
 * @brief Internal implementation to get the first flip interval.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_first_interval_get.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @return The first interval in seconds.
 */
double _elm_flipselector_first_interval_get(const Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for getting the 'first_interval' property.
 * Calls @ref _elm_flipselector_first_interval_get and wraps the result in an Eina_Value.
 * @param obj The Evas object.
 * @return Eina_Value containing the double value of the interval.
 */
static Eina_Value
__eolian_elm_flipselector_first_interval_get_reflect(const Eo *obj)
{
   double val = elm_obj_flipselector_first_interval_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_flipselector_first_interval_get, double, 0);

/**
 * @internal
 * @brief Internal implementation to prepend an item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_item_prepend.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @param label Text label for the new item.
 * @param func Callback function for when the item is selected.
 * @param data Data to pass to the callback function.
 * @return The new Elm_Widget_Item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_flipselector_item_prepend(Eo *obj, Elm_Flipselector_Data *pd, const char *label, Evas_Smart_Cb func, void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_flipselector_item_prepend, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, func, data), const char *label, Evas_Smart_Cb func, void *data);

/**
 * @internal
 * @brief Internal implementation to flip to the next item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_flip_next.
 * @param obj The object.
 * @param pd Flipselector private data.
 */
void _elm_flipselector_flip_next(Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_flipselector_flip_next);

/**
 * @internal
 * @brief Internal implementation to append an item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_item_append.
 * @param obj The object.
 * @param pd Flipselector private data.
 * @param label Text label for the new item.
 * @param func Callback function for when the item is selected.
 * @param data Data to pass to the callback function.
 * @return The new Elm_Widget_Item, or @c NULL on failure.
 */
Elm_Widget_Item *_elm_flipselector_item_append(Eo *obj, Elm_Flipselector_Data *pd, const char *label, Evas_Smart_Cb func, const void *data);

EOAPI EFL_FUNC_BODYV(elm_obj_flipselector_item_append, Elm_Widget_Item *, NULL, EFL_FUNC_CALL(label, func, data), const char *label, Evas_Smart_Cb func, const void *data);

/**
 * @internal
 * @brief Internal implementation to flip to the previous item.
 * Corresponds to the EAPI function @ref elm_obj_flipselector_flip_prev.
 * @param obj The object.
 * @param pd Flipselector private data.
 */
void _elm_flipselector_flip_prev(Eo *obj, Elm_Flipselector_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_flipselector_flip_prev);

/**
 * @internal
 * @brief Implements the Efl_Object constructor for Elm_Flipselector.
 * Initializes the flipselector object and its private data.
 * @param obj The Eo object to construct.
 * @param pd The private data for the Elm_Flipselector instance.
 * @return The constructed Eo object (efl_super(obj, MY_CLASS)).
 */
Efl_Object *_elm_flipselector_efl_object_constructor(Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget theme_apply interface method.
 * Applies the current theme to the flipselector widget. This is called
 * when the theme changes or the widget is created.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @return EINA_ERROR_NONE on success, or an error code if theme application fails.
 */
Eina_Error _elm_flipselector_efl_ui_widget_theme_apply(Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Widget input_event_handler interface method.
 * Handles input events (e.g., mouse, keyboard) for the flipselector.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @param eo_event The Efl_Event containing details of the input event.
 * @param source The source canvas object of the event.
 * @return @c EINA_TRUE if the event was handled, @c EINA_FALSE otherwise.
 */
Eina_Bool _elm_flipselector_efl_ui_widget_widget_input_event_handler(Eo *obj, Elm_Flipselector_Data *pd, const Efl_Event *eo_event, Efl_Canvas_Object *source);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Display range_limits_set interface method.
 * Sets the minimum and maximum values for the flipselector's range.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @param min The minimum range value.
 * @param max The maximum range value.
 */
void _elm_flipselector_efl_ui_range_display_range_limits_set(Eo *obj, Elm_Flipselector_Data *pd, double min, double max);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Display range_limits_get interface method.
 * Gets the minimum and maximum values for the flipselector's range.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @param min Pointer to store the minimum range value.
 * @param max Pointer to store the maximum range value.
 */
void _elm_flipselector_efl_ui_range_display_range_limits_get(const Eo *obj, Elm_Flipselector_Data *pd, double *min, double *max);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Interactive range_step_set interface method.
 * Sets the step value for programmatic changes to the flipselector's value.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @param step The step value.
 */
void _elm_flipselector_efl_ui_range_interactive_range_step_set(Eo *obj, Elm_Flipselector_Data *pd, double step);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Interactive range_step_get interface method.
 * Gets the step value for programmatic changes to the flipselector's value.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @return The step value.
 */
double _elm_flipselector_efl_ui_range_interactive_range_step_get(const Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Display range_value_set interface method.
 * Sets the current value of the flipselector. This typically corresponds to selecting an item.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @param val The value to set.
 */
void _elm_flipselector_efl_ui_range_display_range_value_set(Eo *obj, Elm_Flipselector_Data *pd, double val);

/**
 * @internal
 * @brief Implements the Efl_Ui_Range_Display range_value_get interface method.
 * Gets the current value of the flipselector.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @return The current value.
 */
double _elm_flipselector_efl_ui_range_display_range_value_get(const Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Implements the Efl_Access_Widget_Action elm_actions_get interface method.
 * Retrieves a list of Elementary-specific accessibility actions for the flipselector.
 * These actions allow accessibility tools to interact with the widget.
 * @param obj The Eo object.
 * @param pd The private data for the Elm_Flipselector instance.
 * @return A pointer to an array of Efl_Access_Action_Data structures,
 *         terminated by an entry with a NULL name. The caller should not free this array.
 *         Example: `static const Efl_Access_Action_Data actions[] = {
 *                     { "flip_next", "next item", NULL, _access_action_next_cb, NULL },
 *                     { "flip_prev", "previous item", NULL, _access_action_prev_cb, NULL },
 *                     { NULL, NULL, NULL, NULL, NULL }
 *                   }; return actions;`
 */
const Efl_Access_Action_Data *_elm_flipselector_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Flipselector_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Flipselector Efl_Class structure.
 * This function is called by Efl Core when the class is first loaded.
 * It sets up the vtable (ops) for the class, mapping EFL C method names
 * to their C function implementations (e.g., efl_constructor to
 * _elm_flipselector_efl_object_constructor). It also registers
 * Eolian property reflection functions.
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on successful initialization, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_flipselector_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_FLIPSELECTOR_EXTRA_OPS
#define ELM_FLIPSELECTOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_items_get, _elm_flipselector_items_get),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_first_item_get, _elm_flipselector_first_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_last_item_get, _elm_flipselector_last_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_selected_item_get, _elm_flipselector_selected_item_get),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_first_interval_set, _elm_flipselector_first_interval_set),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_first_interval_get, _elm_flipselector_first_interval_get),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_item_prepend, _elm_flipselector_item_prepend),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_flip_next, _elm_flipselector_flip_next),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_item_append, _elm_flipselector_item_append),
      EFL_OBJECT_OP_FUNC(elm_obj_flipselector_flip_prev, _elm_flipselector_flip_prev),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_flipselector_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_flipselector_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_input_event_handler, _elm_flipselector_efl_ui_widget_widget_input_event_handler),
      EFL_OBJECT_OP_FUNC(efl_ui_range_limits_set, _elm_flipselector_efl_ui_range_display_range_limits_set),
      EFL_OBJECT_OP_FUNC(efl_ui_range_limits_get, _elm_flipselector_efl_ui_range_display_range_limits_get),
      EFL_OBJECT_OP_FUNC(efl_ui_range_step_set, _elm_flipselector_efl_ui_range_interactive_range_step_set),
      EFL_OBJECT_OP_FUNC(efl_ui_range_step_get, _elm_flipselector_efl_ui_range_interactive_range_step_get),
      EFL_OBJECT_OP_FUNC(efl_ui_range_value_set, _elm_flipselector_efl_ui_range_display_range_value_set),
      EFL_OBJECT_OP_FUNC(efl_ui_range_value_get, _elm_flipselector_efl_ui_range_display_range_value_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_flipselector_efl_access_widget_action_elm_actions_get),
      ELM_FLIPSELECTOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"first_interval", __eolian_elm_flipselector_first_interval_set_reflect, __eolian_elm_flipselector_first_interval_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Constant descriptor for the Elm_Flipselector Efl_Class.
 * This structure provides metadata about the Elm_Flipselector class to the Efl
 * type system. It includes the class version, name, type (regular, interface, etc.),
 * size of its instance data structure (@c Elm_Flipselector_Data), and pointers to
 * its class initializer (@ref _elm_flipselector_class_initializer) and
 * class constructor (@ref _elm_flipselector_class_constructor).
 */
static const Efl_Class_Description _elm_flipselector_class_desc = {
   EO_VERSION,
   "Elm.Flipselector",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Flipselector_Data),
   _elm_flipselector_class_initializer,
   _elm_flipselector_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_flipselector_class_get, &_elm_flipselector_class_desc, EFL_UI_LAYOUT_BASE_CLASS, EFL_UI_RANGE_INTERACTIVE_INTERFACE, EFL_ACCESS_WIDGET_ACTION_MIXIN, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_flipselector_eo.legacy.c"
