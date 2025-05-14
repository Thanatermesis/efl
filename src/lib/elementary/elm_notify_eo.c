/**
 * @file
 * @brief Evas object smart class <Elm_Notify> Eolian C source
 *
 * Implements the Eolian interface for Elm_Notify.
 */

/**
 * @internal
 * @brief Event descriptor instance for the "block,clicked" event.
 * @see ELM_NOTIFY_EVENT_BLOCK_CLICKED
 */
EWAPI const Efl_Event_Description _ELM_NOTIFY_EVENT_BLOCK_CLICKED =
   EFL_EVENT_DESCRIPTION("block,clicked");
/**
 * @internal
 * @brief Event descriptor instance for the "timeout" event.
 * @see ELM_NOTIFY_EVENT_TIMEOUT
 */
EWAPI const Efl_Event_Description _ELM_NOTIFY_EVENT_TIMEOUT =
   EFL_EVENT_DESCRIPTION("timeout");
/**
 * @internal
 * @brief Event descriptor instance for the "dismissed" event.
 * @see ELM_NOTIFY_EVENT_DISMISSED
 */
EWAPI const Efl_Event_Description _ELM_NOTIFY_EVENT_DISMISSED =
   EFL_EVENT_DESCRIPTION("dismissed");

void _elm_notify_align_set(Eo *obj, Elm_Notify_Data *pd, double horizontal, double vertical);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_notify_align_set, EFL_FUNC_CALL(horizontal, vertical), double horizontal, double vertical);

void _elm_notify_align_get(const Eo *obj, Elm_Notify_Data *pd, double *horizontal, double *vertical);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_obj_notify_align_get, EFL_FUNC_CALL(horizontal, vertical), double *horizontal, double *vertical);

void _elm_notify_allow_events_set(Eo *obj, Elm_Notify_Data *pd, Eina_Bool allow);

/**
 * @internal
 * @brief Eolian reflection function for the "allow_events" property setter.
 *
 * This function is used by the Eolian reflection system to set the
 * "allow_events" property using a generic Eina_Value.
 *
 * @param obj The Eolian object.
 * @param val An Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_notify_allow_events_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_notify_allow_events_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_notify_allow_events_set, EFL_FUNC_CALL(allow), Eina_Bool allow);

Eina_Bool _elm_notify_allow_events_get(const Eo *obj, Elm_Notify_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "allow_events" property getter.
 *
 * This function is used by the Eolian reflection system to get the
 * "allow_events" property as a generic Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An Eina_Value containing the boolean value of the property.
 */
static Eina_Value
__eolian_elm_notify_allow_events_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_notify_allow_events_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_notify_allow_events_get, Eina_Bool, 0);

void _elm_notify_timeout_set(Eo *obj, Elm_Notify_Data *pd, double timeout);

/**
 * @internal
 * @brief Eolian reflection function for the "timeout" property setter.
 *
 * This function is used by the Eolian reflection system to set the
 * "timeout" property using a generic Eina_Value.
 *
 * @param obj The Eolian object.
 * @param val An Eina_Value containing the double value (timeout in seconds) to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_notify_timeout_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_notify_timeout_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_notify_timeout_set, EFL_FUNC_CALL(timeout), double timeout);

double _elm_notify_timeout_get(const Eo *obj, Elm_Notify_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the "timeout" property getter.
 *
 * This function is used by the Eolian reflection system to get the
 * "timeout" property as a generic Eina_Value.
 *
 * @param obj The Eolian object.
 * @return An Eina_Value containing the double value (timeout in seconds) of the property.
 */
static Eina_Value
__eolian_elm_notify_timeout_get_reflect(const Eo *obj)
{
   double val = elm_obj_notify_timeout_get(obj);
   return eina_value_double_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_notify_timeout_get, double, 0);

void _elm_notify_dismiss(Eo *obj, Elm_Notify_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_notify_dismiss);

Efl_Object *_elm_notify_efl_object_constructor(Eo *obj, Elm_Notify_Data *pd);


void _elm_notify_efl_gfx_entity_visible_set(Eo *obj, Elm_Notify_Data *pd, Eina_Bool v);


void _elm_notify_efl_gfx_entity_position_set(Eo *obj, Elm_Notify_Data *pd, Eina_Position2D pos);


void _elm_notify_efl_gfx_entity_size_set(Eo *obj, Elm_Notify_Data *pd, Eina_Size2D size);


Eina_Error _elm_notify_efl_ui_widget_theme_apply(Eo *obj, Elm_Notify_Data *pd);


Eina_Bool _elm_notify_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Notify_Data *pd, Efl_Canvas_Object *sub_obj);


Eina_Bool _elm_notify_efl_content_content_set(Eo *obj, Elm_Notify_Data *pd, Efl_Gfx_Entity *content);


Efl_Gfx_Entity *_elm_notify_efl_content_content_get(const Eo *obj, Elm_Notify_Data *pd);


Efl_Gfx_Entity *_elm_notify_efl_content_content_unset(Eo *obj, Elm_Notify_Data *pd);


Efl_Object *_elm_notify_efl_part_part_get(const Eo *obj, Elm_Notify_Data *pd, const char *name);


/**
 * @internal
 * @brief Initializes the Elm_Notify Eolian class.
 *
 * This function is called by the Eolian system when the Elm_Notify class
 * is being set up. It registers the operations (methods) and properties
 * for the class, linking them to their respective implementation functions
 * and reflection handlers.
 *
 * @param klass The Eolian class to initialize.
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_notify_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_NOTIFY_EXTRA_OPS
#define ELM_NOTIFY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_notify_align_set, _elm_notify_align_set),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_align_get, _elm_notify_align_get),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_allow_events_set, _elm_notify_allow_events_set),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_allow_events_get, _elm_notify_allow_events_get),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_timeout_set, _elm_notify_timeout_set),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_timeout_get, _elm_notify_timeout_get),
      EFL_OBJECT_OP_FUNC(elm_obj_notify_dismiss, _elm_notify_dismiss),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_notify_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_notify_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_notify_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_notify_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_notify_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_notify_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_content_set, _elm_notify_efl_content_content_set),
      EFL_OBJECT_OP_FUNC(efl_content_get, _elm_notify_efl_content_content_get),
      EFL_OBJECT_OP_FUNC(efl_content_unset, _elm_notify_efl_content_content_unset),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_notify_efl_part_part_get),
      ELM_NOTIFY_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"allow_events", __eolian_elm_notify_allow_events_set_reflect, __eolian_elm_notify_allow_events_get_reflect},
      {"timeout", __eolian_elm_notify_timeout_set_reflect, __eolian_elm_notify_timeout_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Structure describing the Elm_Notify Eolian class.
 *
 * This structure provides metadata for the Elm_Notify class, including its
 * Eolian version, name, type (regular class), size of its instance data
 * (Elm_Notify_Data), and pointers to its class initializer and constructor
 * functions. This information is used by the Eolian system to manage the class.
 */
static const Efl_Class_Description _elm_notify_class_desc = {
   EO_VERSION,
   "Elm.Notify",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Notify_Data),
   _elm_notify_class_initializer,
   _elm_notify_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_notify_class_get, &_elm_notify_class_desc, EFL_UI_WIDGET_CLASS, EFL_UI_FOCUS_LAYER_MIXIN, EFL_CONTENT_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_notify_eo.legacy.c"
