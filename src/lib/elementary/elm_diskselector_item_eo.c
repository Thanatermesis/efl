
Elm_Widget_Item *_elm_diskselector_item_prev_get(const Eo *obj, Elm_Diskselector_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_item_prev_get, Elm_Widget_Item *, NULL);

Elm_Widget_Item *_elm_diskselector_item_next_get(const Eo *obj, Elm_Diskselector_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_item_next_get, Elm_Widget_Item *, NULL);

void _elm_diskselector_item_selected_set(Eo *obj, Elm_Diskselector_Item_Data *pd, Eina_Bool selected);

/**
 * @internal
 * @brief Eolian reflection function for the 'selected' property setter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the 'selected' property is set via reflection (e.g., through scripting
 * or introspection). It converts the generic Eina_Value @p val to an Eina_Bool
 * and then calls the actual property setter elm_obj_diskselector_item_selected_set().
 *
 * @param[in] obj The Efl_Object instance.
 * @param[in] val An Eina_Value containing the boolean value to set.
 *                The Eina_Value should hold an EINA_VALUE_TYPE_BOOL.
 * @return EINA_ERROR_NO_ERROR on success, or an error code if conversion fails.
 */
static Eina_Error
__eolian_elm_diskselector_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_diskselector_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_diskselector_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

Eina_Bool _elm_diskselector_item_selected_get(const Eo *obj, Elm_Diskselector_Item_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'selected' property getter.
 *
 * This function is part of the Eolian reflection mechanism. It is called
 * when the 'selected' property is accessed via reflection. It calls the
 * actual property getter elm_obj_diskselector_item_selected_get() and
 * wraps the returned Eina_Bool into an Eina_Value.
 *
 * @param[in] obj The Efl_Object instance.
 * @return An Eina_Value containing the boolean state of the 'selected' property.
 *         The Eina_Value will hold an EINA_VALUE_TYPE_BOOL.
 */
static Eina_Value
__eolian_elm_diskselector_item_selected_get_reflect(const Eo *obj)
{
   Eina_Bool val = elm_obj_diskselector_item_selected_get(obj);
   return eina_value_bool_init(val);
}

EOAPI EFL_FUNC_BODY_CONST(elm_obj_diskselector_item_selected_get, Eina_Bool, 0);

Efl_Object *_elm_diskselector_item_efl_object_constructor(Eo *obj, Elm_Diskselector_Item_Data *pd);


void _elm_diskselector_item_efl_object_destructor(Eo *obj, Elm_Diskselector_Item_Data *pd);


void _elm_diskselector_item_elm_widget_item_part_text_set(Eo *obj, Elm_Diskselector_Item_Data *pd, const char *part, const char *label);


const char *_elm_diskselector_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Diskselector_Item_Data *pd, const char *part);


void _elm_diskselector_item_elm_widget_item_part_content_set(Eo *obj, Elm_Diskselector_Item_Data *pd, const char *part, Efl_Canvas_Object *content);


Efl_Canvas_Object *_elm_diskselector_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Diskselector_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Initializes the Elm_Diskselector_Item Efl_Class.
 *
 * This function is called once when the Elm_Diskselector_Item class is being
 * constructed. It sets up the Efl_Object operations (mapping Eolian methods
 * to their C implementations) and Efl_Object property reflection operations
 * for this class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_diskselector_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_DISKSELECTOR_ITEM_EXTRA_OPS
#define ELM_DISKSELECTOR_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_item_prev_get, _elm_diskselector_item_prev_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_item_next_get, _elm_diskselector_item_next_get),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_item_selected_set, _elm_diskselector_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_diskselector_item_selected_get, _elm_diskselector_item_selected_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_diskselector_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_diskselector_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_diskselector_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_diskselector_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_diskselector_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_diskselector_item_elm_widget_item_part_content_get),
      ELM_DISKSELECTOR_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"selected", __eolian_elm_diskselector_item_selected_set_reflect, __eolian_elm_diskselector_item_selected_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Diskselector_Item Efl_Class.
 *
 * This static constant structure provides metadata for the
 * Elm.Diskselector.Item class, including its version, name, type,
 * instance data size, and pointers to class lifecycle functions like
 * the class initializer. This descriptor is used by the Eo system to
 * manage the class.
 */
static const Efl_Class_Description _elm_diskselector_item_class_desc = {
   EO_VERSION, /**< Eolian Object version. */
   "Elm.Diskselector.Item", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Elm_Diskselector_Item_Data),
   _elm_diskselector_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_diskselector_item_class_get, &_elm_diskselector_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_diskselector_item_eo.legacy.c"
