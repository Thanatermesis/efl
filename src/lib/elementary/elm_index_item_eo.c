/**
 * @internal
 * @brief Internal function to set the selected state of an index item.
 *
 * This function is the actual implementation for elm_obj_index_item_selected_set.
 * It is called via the Efl_Object_Ops vtable.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @param selected @c EINA_TRUE if selected, @c EINA_FALSE otherwise.
 */
void _elm_index_item_selected_set(Eo *obj, Elm_Index_Item_Data *pd, Eina_Bool selected);


/**
 * @internal
 * @brief Eolian reflection function for the 'selected' property.
 *
 * This function is used by the Eolian reflection system to set the 'selected'
 * property of an Elm_Index_Item object using an Eina_Value.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param val An Eina_Value containing the boolean value for the selected state.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_index_item_selected_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   Eina_Bool cval;
   if (!eina_value_bool_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_item_selected_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Internal function to set the priority of an index item.
 *
 * This function is the actual implementation for elm_obj_index_item_priority_set.
 * It is called via the Efl_Object_Ops vtable.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @param priority The priority value.
 */
void _elm_index_item_priority_set(Eo *obj, Elm_Index_Item_Data *pd, int priority);


/**
 * @internal
 * @brief Eolian reflection function for the 'priority' property.
 *
 * This function is used by the Eolian reflection system to set the 'priority'
 * property of an Elm_Index_Item object using an Eina_Value.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param val An Eina_Value containing the integer value for the priority.
 * @return EINA_ERROR_NO_ERROR on success, or an error code on failure.
 */
static Eina_Error
__eolian_elm_index_item_priority_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   int cval;
   if (!eina_value_int_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   elm_obj_index_item_priority_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_index_item_priority_set, EFL_FUNC_CALL(priority), int priority);

/**
 * @internal
 * @brief Internal function to get the letter of an index item.
 *
 * This function is the actual implementation for elm_obj_index_item_letter_get.
 * It is called via the Efl_Object_Ops vtable.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @return The letter string.
 */
const char *_elm_index_item_letter_get(const Eo *obj, Elm_Index_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_index_item_letter_get, const char *, NULL);

/**
 * @internal
 * @brief Constructor for Elm_Index_Item objects.
 *
 * This function is called when a new Elm_Index_Item object is created.
 * It handles the initialization of the object's private data.
 *
 * @param obj The Efl object (Elm_Index_Item) being constructed.
 * @param pd The private data of the Elm_Index_Item.
 * @return The constructed Efl object.
 */
Efl_Object *_elm_index_item_efl_object_constructor(Eo *obj, Elm_Index_Item_Data *pd);

/**
 * @internal
 * @brief Destructor for Elm_Index_Item objects.
 *
 * This function is called when an Elm_Index_Item object is being destroyed.
 * It handles the cleanup of the object's private data and resources.
 *
 * @param obj The Efl object (Elm_Index_Item) being destructed.
 * @param pd The private data of the Elm_Index_Item.
 */
void _elm_index_item_efl_object_destructor(Eo *obj, Elm_Index_Item_Data *pd);

/**
 * @internal
 * @brief Registers the access object for the Elm_Index_Item.
 *
 * This function is part of the Elm_Widget_Item interface and is used to
 * register the accessibility object associated with this item.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @return The Efl_Canvas_Object that provides accessibility.
 */
Efl_Canvas_Object *_elm_index_item_elm_widget_item_access_register(Eo *obj, Elm_Index_Item_Data *pd);

/**
 * @internal
 * @brief Gets the internationalized name for accessibility.
 *
 * This function implements the Efl.Access.Object interface to provide
 * a localized name for the index item, typically its letter.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @return The internationalized name string.
 */
const char *_elm_index_item_efl_access_object_i18n_name_get(const Eo *obj, Elm_Index_Item_Data *pd);

/**
 * @internal
 * @brief Gets the accessibility actions for the Elm_Index_Item.
 *
 * This function implements the Efl.Access.Widget_Action interface to provide
 * a list of Elementary-specific accessibility actions available for this item.
 *
 * @param obj The Efl object (Elm_Index_Item).
 * @param pd The private data of the Elm_Index_Item.
 * @return A pointer to an array of Efl_Access_Action_Data, terminated by an
 *         entry with a NULL name. Returns NULL if no actions are available.
 *         Example:
 *         static const Efl_Access_Action_Data actions[] = {
 *            { "activate", "activate" }, // { "action_name", "action_localized_name" }
 *            { NULL, NULL }
 *         };
 *         return actions;
 */
const Efl_Access_Action_Data *_elm_index_item_efl_access_widget_action_elm_actions_get(const Eo *obj, Elm_Index_Item_Data *pd);


/**
 * @internal
 * @brief Class initializer for the Elm_Index_Item class.
 *
 * This function is called once when the Elm_Index_Item class is being set up.
 * It defines the Efl_Object operations (methods) and Eolian property reflection
 * for this class.
 *
 * @param klass The Efl_Class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_index_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_INDEX_ITEM_EXTRA_OPS
#define ELM_INDEX_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_selected_set, _elm_index_item_selected_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_priority_set, _elm_index_item_priority_set),
      EFL_OBJECT_OP_FUNC(elm_obj_index_item_letter_get, _elm_index_item_letter_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_index_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_index_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_access_register, _elm_index_item_elm_widget_item_access_register),
      EFL_OBJECT_OP_FUNC(efl_access_object_i18n_name_get, _elm_index_item_efl_access_object_i18n_name_get),
      EFL_OBJECT_OP_FUNC(efl_access_widget_action_elm_actions_get, _elm_index_item_efl_access_widget_action_elm_actions_get),
      ELM_INDEX_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   static const Efl_Object_Property_Reflection refl_table[] = {
      {"selected", __eolian_elm_index_item_selected_set_reflect, NULL},
      {"priority", __eolian_elm_index_item_priority_set_reflect, NULL},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Index_Item Efl class.
 *
 * This structure provides metadata about the Elm_Index_Item class,
 * such as its version, name, type, size of private data, and initializer functions.
 */
static const Efl_Class_Description _elm_index_item_class_desc = {
   EO_VERSION,
   "Elm.Index.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Index_Item_Data),
   _elm_index_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_index_item_class_get, &_elm_index_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_ACCESS_WIDGET_ACTION_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_index_item_eo.legacy.c"
