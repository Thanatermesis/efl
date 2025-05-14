/**
 * @internal
 * @brief Implements elm_obj_naviframe_item_pop_to - Pop items up to this one.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 */
void _elm_naviframe_item_pop_to(Eo *obj, Elm_Naviframe_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_naviframe_item_pop_to);
/**
 * @internal
 * @brief Implements elm_obj_naviframe_item_title_enabled_get - Get title enabled state.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @return EINA_TRUE if title is enabled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_naviframe_item_title_enabled_get(const Eo *obj, Elm_Naviframe_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_naviframe_item_title_enabled_get, Eina_Bool, 0);
/**
 * @internal
 * @brief Implements elm_obj_naviframe_item_title_enabled_set - Set title enabled state.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param enable EINA_TRUE to enable title, EINA_FALSE to disable.
 * @param transition EINA_TRUE to use transition, EINA_FALSE otherwise.
 */
void _elm_naviframe_item_title_enabled_set(Eo *obj, Elm_Naviframe_Item_Data *pd, Eina_Bool enable, Eina_Bool transition);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_naviframe_item_title_enabled_set, EFL_FUNC_CALL(enable, transition), Eina_Bool enable, Eina_Bool transition);
/**
 * @internal
 * @brief Implements elm_obj_naviframe_item_promote - Promote this item to the top.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 */
void _elm_naviframe_item_promote(Eo *obj, Elm_Naviframe_Item_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_naviframe_item_promote);
/**
 * @internal
 * @brief Implements elm_obj_naviframe_item_pop_cb_set - Set the pop callback.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param func The callback function to set.
 * @param data User data for the callback.
 */
void _elm_naviframe_item_pop_cb_set(Eo *obj, Elm_Naviframe_Item_Data *pd, Elm_Naviframe_Item_Pop_Cb func, void *data);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_naviframe_item_pop_cb_set, EFL_FUNC_CALL(func, data), Elm_Naviframe_Item_Pop_Cb func, void *data);
/**
 * @internal
 * @brief Implements efl_object_constructor - EFL object constructor for naviframe item.
 * @param obj The naviframe item object being constructed.
 * @param pd The private data of the naviframe item.
 * @return The constructed EFL object.
 */
Efl_Object *_elm_naviframe_item_efl_object_constructor(Eo *obj, Elm_Naviframe_Item_Data *pd);

/**
 * @internal
 * @brief Implements efl_object_invalidate - EFL object invalidation for naviframe item.
 * @param obj The naviframe item object being invalidated.
 * @param pd The private data of the naviframe item.
 */
void _elm_naviframe_item_efl_object_invalidate(Eo *obj, Elm_Naviframe_Item_Data *pd);

/**
 * @internal
 * @brief Implements efl_object_destructor - EFL object destructor for naviframe item.
 * @param obj The naviframe item object being destructed.
 * @param pd The private data of the naviframe item.
 */
void _elm_naviframe_item_efl_object_destructor(Eo *obj, Elm_Naviframe_Item_Data *pd);

/**
 * @internal
 * @brief Implements elm_wdg_item_signal_emit - Emit a signal from the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param emission The signal name.
 * @param source The signal source.
 */
void _elm_naviframe_item_elm_widget_item_signal_emit(Eo *obj, Elm_Naviframe_Item_Data *pd, const char *emission, const char *source);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_text_set - Set text for a part of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param part The name of the part to set text for.
 * @param label The text to set.
 */
void _elm_naviframe_item_elm_widget_item_part_text_set(Eo *obj, Elm_Naviframe_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_text_get - Get text from a part of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param part The name of the part to get text from.
 * @return The text of the part, or NULL if not found or not set.
 */
const char *_elm_naviframe_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Naviframe_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_content_set - Set content for a part of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param part The name of the part to set content for.
 * @param content The Efl_Canvas_Object to set as content.
 */
void _elm_naviframe_item_elm_widget_item_part_content_set(Eo *obj, Elm_Naviframe_Item_Data *pd, const char *part, Efl_Canvas_Object *content);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_content_get - Get content from a part of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param part The name of the part to get content from.
 * @return The Efl_Canvas_Object content of the part, or NULL if not found or not set.
 */
Efl_Canvas_Object *_elm_naviframe_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Naviframe_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_content_unset - Unset content from a part of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param part The name of the part to unset content from.
 * @return The previously set Efl_Canvas_Object content, or NULL if not found or not set.
 */
Efl_Canvas_Object *_elm_naviframe_item_elm_widget_item_part_content_unset(Eo *obj, Elm_Naviframe_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements elm_wdg_item_style_set - Set the style for the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @param style The style name to set.
 */
void _elm_naviframe_item_elm_widget_item_style_set(Eo *obj, Elm_Naviframe_Item_Data *pd, const char *style);

/**
 * @internal
 * @brief Implements efl_access_object_access_children_get - Get accessible children of the naviframe item.
 * @param obj The naviframe item object.
 * @param pd The private data of the naviframe item.
 * @return A list of accessible child objects.
 */
Eina_List *_elm_naviframe_item_efl_access_object_access_children_get(const Eo *obj, Elm_Naviframe_Item_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Naviframe_Item class.
 *
 * This function is called once when the class is first used.
 * It sets up the Efl_Object operations for Elm_Naviframe_Item instances.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_naviframe_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_NAVIFRAME_ITEM_EXTRA_OPS
#define ELM_NAVIFRAME_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_naviframe_item_pop_to, _elm_naviframe_item_pop_to),
      EFL_OBJECT_OP_FUNC(elm_obj_naviframe_item_title_enabled_get, _elm_naviframe_item_title_enabled_get),
      EFL_OBJECT_OP_FUNC(elm_obj_naviframe_item_title_enabled_set, _elm_naviframe_item_title_enabled_set),
      EFL_OBJECT_OP_FUNC(elm_obj_naviframe_item_promote, _elm_naviframe_item_promote),
      EFL_OBJECT_OP_FUNC(elm_obj_naviframe_item_pop_cb_set, _elm_naviframe_item_pop_cb_set),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_naviframe_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_invalidate, _elm_naviframe_item_efl_object_invalidate),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_naviframe_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_signal_emit, _elm_naviframe_item_elm_widget_item_signal_emit),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_naviframe_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_naviframe_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_naviframe_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_naviframe_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_unset, _elm_naviframe_item_elm_widget_item_part_content_unset),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_style_set, _elm_naviframe_item_elm_widget_item_style_set),
      EFL_OBJECT_OP_FUNC(efl_access_object_access_children_get, _elm_naviframe_item_efl_access_object_access_children_get),
      ELM_NAVIFRAME_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}
/**
 * @internal
 * @brief Describes the Elm_Naviframe_Item EFL class.
 *
 * This static constant structure holds metadata about the Elm_Naviframe_Item class,
 * such as its version, name, type, instance size, and initializer functions.
 */
static const Efl_Class_Description _elm_naviframe_item_class_desc = {
   EO_VERSION, /**< EFL object version. */
   "Elm.Naviframe.Item", /**< Class name. */
   EFL_CLASS_TYPE_REGULAR, /**< Class type. */
   sizeof(Elm_Naviframe_Item_Data),
   _elm_naviframe_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_naviframe_item_class_get, &_elm_naviframe_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_naviframe_item_eo.legacy.c"
