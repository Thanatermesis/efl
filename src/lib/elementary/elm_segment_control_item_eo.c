
/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm_Segment_Control_Item.
 *
 * This function is called when a new Elm_Segment_Control_Item object is constructed.
 * It initializes the item's internal data.
 *
 * @param[in] obj The Eo object to construct.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The constructed Eo object.
 */
int _elm_segment_control_item_index_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_segment_control_item_index_get, int, -1 /* +1 */);

/**
 * @internal
 * @brief Implements elm_obj_segment_control_item_object_get.
 *
 * Retrieves the Efl_Canvas_Object associated with this segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The Efl_Canvas_Object for the item, or NULL if not applicable.
 */
Efl_Canvas_Object *_elm_segment_control_item_object_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_segment_control_item_object_get, Efl_Canvas_Object *, NULL);

/**
 * @internal
 * @brief Implements elm_obj_segment_control_item_selected_set.
 *
 * Sets the selected state of the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @param[in] selected EINA_TRUE if the item is to be selected, EINA_FALSE otherwise.
 */
void _elm_segment_control_item_selected_set(Eo *obj, Elm_Segment_Control_Item_Data *pd, Eina_Bool selected);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_segment_control_item_selected_set, EFL_FUNC_CALL(selected), Eina_Bool selected);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor for Elm_Segment_Control_Item.
 *
 * This function is called when a new Elm_Segment_Control_Item object is constructed.
 * It initializes the item's internal data.
 *
 * @param[in] obj The Eo object to construct.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The constructed Eo object.
 */
Efl_Object *_elm_segment_control_item_efl_object_constructor(Eo *obj, Elm_Segment_Control_Item_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Object.destructor for Elm_Segment_Control_Item.
 *
 * This function is called when an Elm_Segment_Control_Item object is being destroyed.
 * It cleans up any resources allocated by the item.
 *
 * @param[in] obj The Eo object to destruct.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 */
void _elm_segment_control_item_efl_object_destructor(Eo *obj, Elm_Segment_Control_Item_Data *pd);

/**
 * @internal
 * @brief Implements elm_wdg_item_access_register for Elm_Segment_Control_Item.
 *
 * Registers the item with the accessibility infrastructure.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The Efl_Canvas_Object used for accessibility, or NULL.
 */
Efl_Canvas_Object *_elm_segment_control_item_elm_widget_item_access_register(Eo *obj, Elm_Segment_Control_Item_Data *pd);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_text_set for Elm_Segment_Control_Item.
 *
 * Sets the text for a specific part of the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @param[in] part The name of the part to set text for (e.g., "elm.text").
 * @param[in] label The text to set.
 */
void _elm_segment_control_item_elm_widget_item_part_text_set(Eo *obj, Elm_Segment_Control_Item_Data *pd, const char *part, const char *label);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_text_get for Elm_Segment_Control_Item.
 *
 * Gets the text from a specific part of the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @param[in] part The name of the part to get text from (e.g., "elm.text").
 * @return The text of the part, or NULL if not set or part does not exist.
 */
const char *_elm_segment_control_item_elm_widget_item_part_text_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_content_set for Elm_Segment_Control_Item.
 *
 * Sets the content for a specific part of the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @param[in] part The name of the part to set content for (e.g., "elm.swallow.icon").
 * @param[in] content The Efl_Canvas_Object to set as content.
 */
void _elm_segment_control_item_elm_widget_item_part_content_set(Eo *obj, Elm_Segment_Control_Item_Data *pd, const char *part, Efl_Canvas_Object *content);

/**
 * @internal
 * @brief Implements elm_wdg_item_part_content_get for Elm_Segment_Control_Item.
 *
 * Gets the content from a specific part of the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @param[in] part The name of the part to get content from (e.g., "elm.swallow.icon").
 * @return The Efl_Canvas_Object content of the part, or NULL if not set or part does not exist.
 */
Efl_Canvas_Object *_elm_segment_control_item_elm_widget_item_part_content_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd, const char *part);

/**
 * @internal
 * @brief Implements efl_ui_focus_object_focus_geometry_get for Elm_Segment_Control_Item.
 *
 * Retrieves the focus geometry for the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The Eina_Rect representing the focus geometry.
 */
Eina_Rect _elm_segment_control_item_efl_ui_focus_object_focus_geometry_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd);

/**
 * @internal
 * @brief Implements efl_ui_focus_object_focus_parent_get for Elm_Segment_Control_Item.
 *
 * Retrieves the focus parent for the segment control item.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data for the Elm_Segment_Control_Item.
 * @return The Efl_Ui_Focus_Object that is the parent in the focus chain.
 */
Efl_Ui_Focus_Object *_elm_segment_control_item_efl_ui_focus_object_focus_parent_get(const Eo *obj, Elm_Segment_Control_Item_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Segment_Control_Item class.
 *
 * This function is called once when the Efl class system initializes
 * the Elm_Segment_Control_Item class. It sets up the operations (methods)
 * for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_segment_control_item_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_SEGMENT_CONTROL_ITEM_EXTRA_OPS
#define ELM_SEGMENT_CONTROL_ITEM_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_index_get, _elm_segment_control_item_index_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_object_get, _elm_segment_control_item_object_get),
      EFL_OBJECT_OP_FUNC(elm_obj_segment_control_item_selected_set, _elm_segment_control_item_selected_set),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_segment_control_item_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _elm_segment_control_item_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_access_register, _elm_segment_control_item_elm_widget_item_access_register),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_set, _elm_segment_control_item_elm_widget_item_part_text_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_text_get, _elm_segment_control_item_elm_widget_item_part_text_get),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_set, _elm_segment_control_item_elm_widget_item_part_content_set),
      EFL_OBJECT_OP_FUNC(elm_wdg_item_part_content_get, _elm_segment_control_item_elm_widget_item_part_content_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_geometry_get, _elm_segment_control_item_efl_ui_focus_object_focus_geometry_get),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_focus_parent_get, _elm_segment_control_item_efl_ui_focus_object_focus_parent_get),
      ELM_SEGMENT_CONTROL_ITEM_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _elm_segment_control_item_class_desc = {
   EO_VERSION,
   "Elm.Segment_Control.Item",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Segment_Control_Item_Data),
   _elm_segment_control_item_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(elm_segment_control_item_class_get, &_elm_segment_control_item_class_desc, ELM_WIDGET_ITEM_CLASS, EFL_UI_FOCUS_OBJECT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_segment_control_item_eo.legacy.c"
