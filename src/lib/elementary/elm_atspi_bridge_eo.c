/**
 * @brief Event descriptor for the "connected" event.
 * @see ELM_ATSPI_BRIDGE_EVENT_CONNECTED
 */
EWAPI const Efl_Event_Description _ELM_ATSPI_BRIDGE_EVENT_CONNECTED =
   EFL_EVENT_DESCRIPTION("connected");
/**
 * @brief Event descriptor for the "disconnected" event.
 * @see ELM_ATSPI_BRIDGE_EVENT_DISCONNECTED
 */
EWAPI const Efl_Event_Description _ELM_ATSPI_BRIDGE_EVENT_DISCONNECTED =
   EFL_EVENT_DESCRIPTION("disconnected");

/**
 * @brief Internal implementation for @ref elm_obj_atspi_bridge_connected_get.
 *
 * @param obj The Efl_Object instance.
 * @param pd The private data for Elm_Atspi_Bridge.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise.
 */
Eina_Bool _elm_atspi_bridge_connected_get(const Eo *obj, Elm_Atspi_Bridge_Data *pd);


/**
 * @brief Eolian reflection function for the "connected" property.
 *
 * This function is used by the Eolian reflection system to get the value
 * of the "connected" property.
 *
 * @param obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the connection.
 */
static Eina_Value
__eolian_elm_atspi_bridge_connected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_atspi_bridge_connected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_atspi_bridge_connected_get, Eina_Bool, 0);

/**
 * @brief Internal Efl_Object constructor for Elm_Atspi_Bridge.
 *
 * This function is called when a new Elm_Atspi_Bridge object is created.
 * It should handle the initialization of the object's private data.
 *
 * @param obj The Efl_Object instance being constructed.
 * @param pd The private data for Elm_Atspi_Bridge.
 * @return The constructed Efl_Object instance, or @c NULL on failure.
 */
Efl_Object *_elm_atspi_bridge_efl_object_constructor(Eo *obj, Elm_Atspi_Bridge_Data *pd);

/**
 * @brief Internal Efl_Object destructor for Elm_Atspi_Bridge.
 *
 * This function is called when an Elm_Atspi_Bridge object is being destroyed.
 * It should handle the cleanup of the object's private data and any
 * resources it holds.
 *
 * @param obj The Efl_Object instance being destructed.
 * @param pd The private data for Elm_Atspi_Bridge.
 */
void _elm_atspi_bridge_efl_object_destructor(Eo *obj, Elm_Atspi_Bridge_Data *pd);

/**
 * @brief Initializes the Elm_Atspi_Bridge Efl_Class.
 *
 * This function is called once when the Efl_Class for Elm_Atspi_Bridge
 * is being set up. It defines the operations (methods) and property
 * reflection capabilities for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_atspi_bridge_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_ATSPI_BRIDGE_EXTRA_OPS
#define ELM_ATSPI_BRIDGE_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_atspi_bridge_connected_get, _elm_atspi_bridge_connected_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_atspi_bridge_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_atspi_bridge_efl_object_destructor),
      ELM_ATSPI_BRIDGE_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"connected", NULL, __eolian_elm_atspi_bridge_connected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Describes the Elm_Atspi_Bridge Efl_Class.
 *
 * This structure provides metadata for the Elm_Atspi_Bridge class,
 * including its version, name, type, size of private data,
 * and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_atspi_bridge_class_desc = {
   EO_VERSION,
   "Elm.Atspi.Bridge",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Atspi_Bridge_Data),
   _elm_atspi_bridge_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_atspi_bridge_class_get, &_elm_atspi_bridge_class_desc, EFL_OBJECT_CLASS, NULL);

#include "elm_atspi_bridge_eo.legacy.c"
