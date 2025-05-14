
void _elm_mapbuf_auto_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Bool on);

/**
 * @internal
 * @brief Eolian reflection function for the 'auto' property setter.
 *
 * This function is called by the Eolian system to set the 'auto' property.
 * It converts the Eina_Value to a boolean and calls the C implementation.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_mapbuf_auto_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_private_mapbuf_auto_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_private_mapbuf_auto_set, EFL_FUNC_CALL(on), Eina_Bool on);

Eina_Bool _elm_mapbuf_auto_get(const Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'auto' property getter.
 *
 * This function is called by the Eolian system to get the 'auto' property.
 * It calls the C implementation and wraps the boolean result in an Eina_Value.
 *
 * @param[in] obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the 'auto' property.
 */
static Eina_Value
__eolian_elm_mapbuf_auto_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_private_mapbuf_auto_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_private_mapbuf_auto_get, Eina_Bool, 0);

void _elm_mapbuf_smooth_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Bool smooth);

/**
 * @internal
 * @brief Eolian reflection function for the 'smooth' property setter.
 *
 * This function is called by the Eolian system to set the 'smooth' property.
 * It converts the Eina_Value to a boolean and calls the C implementation.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_mapbuf_smooth_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_private_mapbuf_smooth_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_private_mapbuf_smooth_set, EFL_FUNC_CALL(smooth), Eina_Bool smooth);

Eina_Bool _elm_mapbuf_smooth_get(const Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'smooth' property getter.
 *
 * This function is called by the Eolian system to get the 'smooth' property.
 * It calls the C implementation and wraps the boolean result in an Eina_Value.
 *
 * @param[in] obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the 'smooth' property.
 */
static Eina_Value
__eolian_elm_mapbuf_smooth_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_private_mapbuf_smooth_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_private_mapbuf_smooth_get, Eina_Bool, 0);

void _elm_mapbuf_alpha_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Bool alpha);

/**
 * @internal
 * @brief Eolian reflection function for the 'alpha' property setter.
 *
 * This function is called by the Eolian system to set the 'alpha' property.
 * It converts the Eina_Value to a boolean and calls the C implementation.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_mapbuf_alpha_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_private_mapbuf_alpha_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_private_mapbuf_alpha_set, EFL_FUNC_CALL(alpha), Eina_Bool alpha);

Eina_Bool _elm_mapbuf_alpha_get(const Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'alpha' property getter.
 *
 * This function is called by the Eolian system to get the 'alpha' property.
 * It calls the C implementation and wraps the boolean result in an Eina_Value.
 *
 * @param[in] obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the 'alpha' property.
 */
static Eina_Value
__eolian_elm_mapbuf_alpha_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_private_mapbuf_alpha_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_private_mapbuf_alpha_get, Eina_Bool, 0);

void _elm_mapbuf_enabled_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Bool enabled);

/**
 * @internal
 * @brief Eolian reflection function for the 'enabled' property setter.
 *
 * This function is called by the Eolian system to set the 'enabled' property.
 * It converts the Eina_Value to a boolean and calls the C implementation.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] val The Eina_Value containing the boolean value to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_mapbuf_enabled_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_private_mapbuf_enabled_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_private_mapbuf_enabled_set, EFL_FUNC_CALL(enabled), Eina_Bool enabled);

Eina_Bool _elm_mapbuf_enabled_get(const Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'enabled' property getter.
 *
 * This function is called by the Eolian system to get the 'enabled' property.
 * It calls the C implementation and wraps the boolean result in an Eina_Value.
 *
 * @param[in] obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the 'enabled' property.
 */
static Eina_Value
__eolian_elm_mapbuf_enabled_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_private_mapbuf_enabled_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_private_mapbuf_enabled_get, Eina_Bool, 0);

void _elm_mapbuf_point_color_set(Eo *obj, Elm_Mapbuf_Data *pd, int idx, int r, int g, int b, int a);

EOAPI EFL_VOID_FUNC_BODYV(elm_private_mapbuf_point_color_set, EFL_FUNC_CALL(idx, r, g, b, a), int idx, int r, int g, int b, int a);

void _elm_mapbuf_point_color_get(const Eo *obj, Elm_Mapbuf_Data *pd, int idx, int *r, int *g, int *b, int *a);

EOAPI EFL_VOID_FUNC_BODYV_CONST(elm_private_mapbuf_point_color_get, EFL_FUNC_CALL(idx, r, g, b, a), int idx, int *r, int *g, int *b, int *a);

/**
 * @internal
 * @brief Implements the Efl_Object constructor for Elm_Mapbuf.
 *
 * This function is called when a new Elm_Mapbuf object is constructed.
 *
 * @param[in] obj The Efl_Object instance being constructed.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @return The constructed Efl_Object instance.
 */
Efl_Object *_elm_mapbuf_efl_object_constructor(Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Implements the efl_gfx_entity_visible_set for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] v Visibility state.
 */
void _elm_mapbuf_efl_gfx_entity_visible_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Bool v);

/**
 * @internal
 * @brief Implements the efl_gfx_entity_position_set for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] pos The new position.
 */
void _elm_mapbuf_efl_gfx_entity_position_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Position2D pos);

/**
 * @internal
 * @brief Implements the efl_gfx_entity_size_set for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] size The new size.
 */
void _elm_mapbuf_efl_gfx_entity_size_set(Eo *obj, Elm_Mapbuf_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the efl_ui_widget_theme_apply for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @return EINA_ERROR_NO_ERROR on success, or an error code.
 */
Eina_Error _elm_mapbuf_efl_ui_widget_theme_apply(Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Implements the efl_ui_widget_sub_object_del for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] sub_obj The sub_object to delete.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise.
 */
Eina_Bool _elm_mapbuf_efl_ui_widget_widget_sub_object_del(Eo *obj, Elm_Mapbuf_Data *pd, Efl_Canvas_Object *sub_obj);

/**
 * @internal
 * @brief Implements the efl_content_set for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] content The content to set.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise.
 */
Eina_Bool _elm_mapbuf_efl_content_content_set(Eo *obj, Elm_Mapbuf_Data *pd, Efl_Gfx_Entity *content);

/**
 * @internal
 * @brief Implements the efl_content_get for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @return The current content object.
 */
Efl_Gfx_Entity *_elm_mapbuf_efl_content_content_get(const Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Implements the efl_content_unset for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @return The previously set content object.
 */
Efl_Gfx_Entity *_elm_mapbuf_efl_content_content_unset(Eo *obj, Elm_Mapbuf_Data *pd);

/**
 * @internal
 * @brief Implements the efl_part_get for Elm_Mapbuf.
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] pd The private data for the Elm_Mapbuf instance.
 * @param[in] name The name of the part to get.
 * @return The part object, or NULL if not found.
 */
Efl_Object *_elm_mapbuf_efl_part_part_get(const Eo *obj, Elm_Mapbuf_Data *pd, const char *name);

/**
 * @internal
 * @brief Initializes the Elm_Mapbuf Efl_Class.
 *
 * This function sets up the operations (methods) and properties for the
 * Elm_Mapbuf class. It is called once during class construction.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_mapbuf_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_MAPBUF_EXTRA_OPS
#define ELM_MAPBUF_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_auto_set, _elm_mapbuf_auto_set),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_auto_get, _elm_mapbuf_auto_get),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_smooth_set, _elm_mapbuf_smooth_set),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_smooth_get, _elm_mapbuf_smooth_get),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_alpha_set, _elm_mapbuf_alpha_set),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_alpha_get, _elm_mapbuf_alpha_get),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_enabled_set, _elm_mapbuf_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_enabled_get, _elm_mapbuf_enabled_get),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_point_color_set, _elm_mapbuf_point_color_set),
      EFL_OBJECT_OP_FUNC(elm_private_mapbuf_point_color_get, _elm_mapbuf_point_color_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_mapbuf_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_visible_set, _elm_mapbuf_efl_gfx_entity_visible_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_position_set, _elm_mapbuf_efl_gfx_entity_position_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_mapbuf_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_mapbuf_efl_ui_widget_theme_apply),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_sub_object_del, _elm_mapbuf_efl_ui_widget_widget_sub_object_del),
      EFL_OBJECT_OP_FUNC(efl_content_set, _elm_mapbuf_efl_content_content_set),
      EFL_OBJECT_OP_FUNC(efl_content_get, _elm_mapbuf_efl_content_content_get),
      EFL_OBJECT_OP_FUNC(efl_content_unset, _elm_mapbuf_efl_content_content_unset),
      EFL_OBJECT_OP_FUNC(efl_part_get, _elm_mapbuf_efl_part_part_get),
      ELM_MAPBUF_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"auto", __eolian_elm_mapbuf_auto_set_reflect, __eolian_elm_mapbuf_auto_get_reflect},
      {"smooth", __eolian_elm_mapbuf_smooth_set_reflect, __eolian_elm_mapbuf_smooth_get_reflect},
      {"alpha", __eolian_elm_mapbuf_alpha_set_reflect, __eolian_elm_mapbuf_alpha_get_reflect},
      {"enabled", __eolian_elm_mapbuf_enabled_set_reflect, __eolian_elm_mapbuf_enabled_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Mapbuf Efl_Class.
 *
 * This static structure provides metadata for the Elm_Mapbuf class,
 * including its version, name, type, instance size, and initializer/constructor
 * functions.
 */
static const Efl_Class_Description _elm_mapbuf_class_desc = {
   EO_VERSION,
   "Elm.Mapbuf",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Mapbuf_Data),
   _elm_mapbuf_class_initializer,
   _elm_mapbuf_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_mapbuf_class_get, &_elm_mapbuf_class_desc, EFL_UI_WIDGET_CLASS, EFL_CONTENT_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_mapbuf_eo.legacy.c"
