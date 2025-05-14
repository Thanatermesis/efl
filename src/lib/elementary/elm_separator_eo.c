/**
 * @internal
 * @brief Internal function to set the horizontal mode of a separator object.
 *
 * This function is the actual implementation for elm_obj_separator_horizontal_set.
 *
 * @param obj The separator object.
 * @param pd The private data of the separator object.
 * @param horizontal If EINA_TRUE, the separator is horizontal, EINA_FALSE otherwise.
 */
void _elm_separator_horizontal_set(Eo *obj, Elm_Separator_Data *pd, Eina_Bool horizontal);

/**
 * @internal
 * @brief Reflection function for the elm_obj_separator_horizontal_set property.
 *
 * This function is used by the Eolian reflection system to set the 'horizontal'
 * property from an Eina_Value.
 *
 * @param obj The object.
 * @param val The Eina_Value containing the boolean to set.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_separator_horizontal_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_separator_horizontal_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_separator_horizontal_set, EFL_FUNC_CALL(horizontal), Eina_Bool horizontal);

/**
 * @internal
 * @brief Internal function to get the horizontal mode of a separator object.
 *
 * This function is the actual implementation for elm_obj_separator_horizontal_get.
 *
 * @param obj The separator object.
 * @param pd The private data of the separator object.
 * @return EINA_TRUE if the separator is horizontal, EINA_FALSE otherwise.
 */
Eina_Bool _elm_separator_horizontal_get(const Eo *obj, Elm_Separator_Data *pd);

/**
 * @internal
 * @brief Reflection function for the elm_obj_separator_horizontal_get property.
 *
 * This function is used by the Eolian reflection system to get the 'horizontal'
 * property and return it as an Eina_Value.
 *
 * @param obj The object.
 * @return An Eina_Value containing the boolean state of the 'horizontal' property.
 */
static Eina_Value
__eolian_elm_separator_horizontal_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_separator_horizontal_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_separator_horizontal_get, Eina_Bool, 0);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm_Separator.
 *
 * This function is called when a new Elm_Separator object is constructed.
 * It handles the initialization of the separator's specific data.
 *
 * @param obj The Eo object to construct.
 * @param pd The private data for the Elm_Separator.
 * @return The constructed Efl_Object, or NULL on failure.
 */
Efl_Object *_elm_separator_efl_object_constructor(Eo *obj, Elm_Separator_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Ui.Widget.theme_apply for Elm_Separator.
 *
 * This function is called when the theme is applied or changed for the separator widget.
 * It handles updating the separator's appearance based on the current theme.
 *
 * @param obj The Eo object (separator widget).
 * @param pd The private data for the Elm_Separator.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if applying the theme fails.
 */
Eina_Error _elm_separator_efl_ui_widget_theme_apply(Eo *obj, Elm_Separator_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Separator class.
 *
 * This function is called once when the Elm_Separator class is first loaded.
 * It sets up the Efl_Object operations (ops) and property reflection
 * for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_separator_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SEPARATOR_EXTRA_OPS
#define ELM_SEPARATOR_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_separator_horizontal_set, _elm_separator_horizontal_set),
      EFL_OBJECT_OP_FUNC(elm_obj_separator_horizontal_get, _elm_separator_horizontal_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_separator_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_ui_widget_theme_apply, _elm_separator_efl_ui_widget_theme_apply),
      ELM_SEPARATOR_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"horizontal", __eolian_elm_separator_horizontal_set_reflect, __eolian_elm_separator_horizontal_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Separator Efl_Class.
 *
 * This structure provides metadata for the Elm_Separator class,
 * including its version, name, type, size of instance data,
 * and pointers to initializer and constructor functions.
 */
static const Efl_Class_Description _elm_separator_class_desc = {
   EO_VERSION,
   "Elm.Separator",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Separator_Data),
   _elm_separator_class_initializer,
   _elm_separator_class_constructor,
   NULL
};

EFL_DEFINE_CLASS(elm_separator_class_get, &_elm_separator_class_desc, EFL_UI_LAYOUT_BASE_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_separator_eo.legacy.c"
